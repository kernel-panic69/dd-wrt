// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *   Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 *   linux-cifsd-devel@lists.sourceforge.net
 */

#include <ksmbdtools.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "ipc.h"
#include "rpc.h"
#include "worker.h"
#include "config_parser.h"
#include "management/user.h"
#include "management/share.h"
#include "management/session.h"
#include "management/tree_conn.h"
#include "management/spnego.h"
#include "version.h"

static int no_detach = 0;
int ksmbd_health_status;
static pid_t worker_pid;
static int lock_fd = -1;

typedef int (*worker_fn)(void);

static void usage(int status)
{
	fprintf(stderr,
		"Usage: ksmbd.mountd [-p NUMBER] [-c SMBCONF] [-u PWDDB] [-n[WAY]] [-s]\n"
		"       ksmbd.mountd {-V | -h}\n");

	if (status != EXIT_SUCCESS)
		fprintf(stderr, "Try 'ksmbd.mountd --help' for more information.\n");
	else
		fprintf(stderr,
			"Run ksmbd.mountd user mode and ksmbd kernel mode daemons.\n"
			"\n"
			"Mandatory arguments to long options are mandatory for short options too.\n"
			"  -p, --port=NUMBER       bind to TCP port NUMBER instead of '" STR(KSMBD_CONF_DEFAULT_TCP_PORT) "'\n"
			"  -c, --config=SMBCONF    use SMBCONF as config file instead of\n"
			"                          '" PATH_SMBCONF "'\n"
			"  -u, --users=PWDDB       use PWDDB as user database instead of\n"
			"                          '" PATH_PWDDB "'\n"
			"  -n, --nodetach[=WAY]    do not detach process from foreground;\n"
			"                          WAY is 1 by default;\n"
			"                          if WAY is 1 also become process group leader;\n"
			"                          if WAY is 0 detach\n"
			"  -s, --systemd           run in systemd service mode\n"
			"  -V, --version           output version information and exit\n"
			"  -h, --help              display this help and exit\n"
			"\n"
			"ksmbd-tools home page: <https://github.com/cifsd-team/ksmbd-tools>\n");
}

static int show_version(void)
{
	printf("ksmbd-tools version : %s\n", KSMBD_TOOLS_VERSION);
	return EXIT_SUCCESS;
}

static int handle_orphaned_lock_file(void)
{
	char proc_ent[64] = {0, };
	char manager_pid[10] = {0, };
	int pid = 0;
	int fd;

	fd = open(KSMBD_LOCK_FILE, O_RDONLY);
	if (fd < 0)
		return -EINVAL;

	if (read(fd, &manager_pid, sizeof(manager_pid)) == -1) {
		pr_debug("Unable to read main PID: %s\n", strerr(errno));
		close(fd);
		return -EINVAL;
	}

	close(fd);

	pid = strtol(manager_pid, NULL, 10);
	snprintf(proc_ent, sizeof(proc_ent), "/proc/%d", pid);
	fd = open(proc_ent, O_RDONLY);
	if (fd < 0) {
		pr_info("Unlink orphaned '%s'\n", KSMBD_LOCK_FILE);
		return unlink(KSMBD_LOCK_FILE);
	}

	close(fd);
	pr_info("File '%s' belongs to pid %d\n", KSMBD_LOCK_FILE, pid);
	return -EINVAL;
}

static int create_lock_file(void)
{
	char manager_pid[10];
	size_t sz;

retry:
	lock_fd = open(KSMBD_LOCK_FILE, O_CREAT | O_EXCL | O_WRONLY,
			S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
	if (lock_fd < 0) {
		if (handle_orphaned_lock_file())
			return -EINVAL;
		goto retry;
	}

	if (flock(lock_fd, LOCK_EX | LOCK_NB) != 0)
		return -EINVAL;

	sz = snprintf(manager_pid, sizeof(manager_pid), "%d", getpid());
	if (write(lock_fd, manager_pid, sz) == -1)
		pr_err("Unable to record main PID: %s\n", strerr(errno));
	return 0;
}

char *make_path_subauth(void)
{
	char *path;
	int smbconf_len = strlen(global_conf.smbconf);
	int loc = 0;
	const char *subauth_filename = "ksmbd.subauth";

	if (strchr(global_conf.smbconf, '/')) {
		for (loc = smbconf_len - 1; loc > 0; loc--)
			if (global_conf.smbconf[loc] == '/') {
				loc++;
				break;
			}
	}

	path = g_try_malloc0(loc + strlen(subauth_filename) + 1);
	if (!path)
		return NULL;

	strncat(path, global_conf.smbconf, loc);
	strcat(path, subauth_filename);

	return path;
}

/*
 * Write to file safely; by using a tmp and atomic rename.
 * Avoids a corrupt file if the write would be interrupted due
 * to a power failure.
 */
static int write_file_safe(char *path, char *buff, size_t length, int mode)
{
	int fd, ret = -1;
	char *path_tmp = g_strdup_printf("%s.tmp", path);

	if (g_file_test(path_tmp, G_FILE_TEST_EXISTS))
		unlink(path_tmp);

	fd = open(path_tmp, O_CREAT | O_EXCL | O_WRONLY, mode);
	if (fd < 0) {
		pr_err("Unable to create %s: %s\n", path_tmp, strerr(errno));
		goto err_out;
	}

	if (write(fd, buff, length) == -1) {
		pr_err("Unable to write to %s: %s\n", path_tmp, strerr(errno));
		close(fd);
		goto err_out;
	}

	fsync(fd);
	close(fd);

	if (rename(path_tmp, path)) {
		pr_err("Unable to rename to %s: %s\n", path, strerr(errno));
		goto err_out;
	}
	ret = 0;

err_out:
	g_free(path_tmp);
	return ret;
}

static int create_subauth_file(char *path_subauth)
{
	char subauth_buf[35];
	GRand *rnd;

	rnd = g_rand_new();
	sprintf(subauth_buf, "%d:%d:%d\n", g_rand_int_range(rnd, 0, INT_MAX),
		g_rand_int_range(rnd, 0, INT_MAX),
		g_rand_int_range(rnd, 0, INT_MAX));

	return write_file_safe(path_subauth, subauth_buf, strlen(subauth_buf),
		S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
}

static int generate_sub_auth(void)
{
	int rc;
	char *path_subauth;

	path_subauth = make_path_subauth();
	if (!path_subauth)
		return -ENOMEM;
retry:
	rc = cp_parse_subauth(path_subauth);
	if (rc < 0) {
		rc = create_subauth_file(path_subauth);
		if (rc) {
			free(path_subauth);
			return -1;
		}
		goto retry;
	}

	free(path_subauth);
	return 0;
}

static void delete_lock_file(void)
{
	if (lock_fd == -1)
		return;

	flock(lock_fd, LOCK_UN);
	close(lock_fd);
	lock_fd = -1;
	remove(KSMBD_LOCK_FILE);
}

static int wait_group_kill(int signo)
{
	pid_t pid;
	int status;

	if (kill(worker_pid, signo) != 0)
		pr_err("can't execute kill %d: %s\n",
			worker_pid,
			strerr(errno));

	while (1) {
		pid = waitpid(-1, &status, 0);
		if (pid != 0) {
			pr_debug("detected pid %d termination\n", pid);
			break;
		}
		sleep(1);
	}
	return 0;
}

static int setup_signal_handler(int signo, sighandler_t handler)
{
	int status;
	sigset_t full_set;
	struct sigaction act = {};

	sigfillset(&full_set);

	act.sa_handler = handler;
	act.sa_mask = full_set;

	status = sigaction(signo, &act, NULL);
	if (status != 0)
		pr_err("Unable to register %s signal handler: %s",
				strsignal(signo), strerr(errno));
	return status;
}

static int setup_signals(sighandler_t handler)
{
	if (setup_signal_handler(SIGINT, handler) != 0)
		return -EINVAL;

	if (setup_signal_handler(SIGTERM, handler) != 0)
		return -EINVAL;

	if (setup_signal_handler(SIGABRT, handler) != 0)
		return -EINVAL;

	if (setup_signal_handler(SIGQUIT, handler) != 0)
		return -EINVAL;

	if (setup_signal_handler(SIGHUP, handler) != 0)
		return -EINVAL;

	return 0;
}

static int parse_configs(char *pwddb, char *smbconf)
{
	int ret;

	ret = cp_parse_pwddb(pwddb);
	if (ret == -ENOENT) {
		pr_err("User database file does not exist. %s\n",
			"Only guest sessions (if permitted) will work.");
	} else if (ret) {
		pr_err("Unable to parse user database\n");
		return ret;
	}

	ret = cp_parse_smbconf(smbconf);
	if (ret) {
		pr_err("Unable to parse smb configuration file\n");
		return ret;
	}
	return 0;
}

static void worker_process_free(void)
{
	/*
	 * NOTE, this is the final release, we don't look at ref_count
	 * values. User management should be destroyed last.
	 */
	spnego_destroy();
	ipc_destroy();
	rpc_destroy();
	wp_destroy();
	sm_destroy();
	shm_destroy();
	usm_destroy();
}

static void child_sig_handler(int signo)
{
	static volatile int fatal_delivered = 0;

	if (signo == SIGHUP) {
		/*
		 * This is a signal handler, we can't take any locks, set
		 * a flag and wait for normal execution context to re-read
		 * the configs.
		 */
		ksmbd_health_status |= KSMBD_SHOULD_RELOAD_CONFIG;
		pr_debug("Scheduled a config reload action.\n");
		return;
	}

	pr_info("Child received signal: %d (%s)\n",
		signo, strsignal(signo));

	if (!g_atomic_int_compare_and_exchange(&fatal_delivered, 0, 1))
		return;

	ksmbd_health_status &= ~KSMBD_HEALTH_RUNNING;
	worker_process_free();
	exit(EXIT_SUCCESS);
}

static void manager_sig_handler(int signo)
{
	/*
	 * Pass SIGHUP to worker, so it will reload configs
	 */
	if (signo == SIGHUP) {
		if (!worker_pid)
			return;

		ksmbd_health_status |= KSMBD_SHOULD_RELOAD_CONFIG;
		if (kill(worker_pid, signo))
			pr_err("Unable to send SIGHUP to %d: %s\n",
				worker_pid, strerr(errno));
		return;
	}

	setup_signals(SIG_DFL);
	wait_group_kill(signo);
	pr_info("Exiting. Bye!\n");
	delete_lock_file();
	kill(0, SIGINT);
}

static int worker_process_init(void)
{
	int ret;

	setup_signals(child_sig_handler);
	set_logger_app_name("ksmbd-worker");

	ret = usm_init();
	if (ret) {
		pr_err("Failed to init user management\n");
		goto out;
	}

	ret = shm_init();
	if (ret) {
		pr_err("Failed to init net share management\n");
		goto out;
	}

	ret = parse_configs(global_conf.pwddb, global_conf.smbconf);
	if (ret) {
		pr_err("Failed to parse configuration files\n");
		goto out;
	}

	ret = sm_init();
	if (ret) {
		pr_err("Failed to init user session management\n");
		goto out;
	}

	ret = wp_init();
	if (ret) {
		pr_err("Failed to init worker threads pool\n");
		goto out;
	}

	ret = rpc_init();
	if (ret) {
		pr_err("Failed to init RPC subsystem\n");
		goto out;
	}

	ret = ipc_init();
	if (ret) {
		pr_err("Failed to init IPC subsystem\n");
		goto out;
	}

	ret = spnego_init();
	if (ret) {
		pr_err("Failed to init spnego subsystem\n");
		ret = KSMBD_STATUS_IPC_FATAL_ERROR;
		goto out;
	}

	while (ksmbd_health_status & KSMBD_HEALTH_RUNNING) {
		ret = ipc_process_event();
		if (ret == -KSMBD_STATUS_IPC_FATAL_ERROR) {
			ret = KSMBD_STATUS_IPC_FATAL_ERROR;
			break;
		}
	}
out:
	worker_process_free();
	return ret;
}

static pid_t start_worker_process(worker_fn fn)
{
	int status = 0;
	pid_t __pid;

	__pid = fork();
	if (__pid < 0) {
		pr_err("Can't fork child process: `%s'\n", strerr(errno));
		return -EINVAL;
	}
	if (__pid == 0) {
		status = fn();
		exit(status);
	}
	return __pid;
}

static int manager_process_init(void)
{
	/*
	 * Do not chdir() daemon()'d process to '/'.
	 */
	int nochdir = 1;

	setup_signals(manager_sig_handler);
	if (no_detach == 0) {
		pr_logger_init(PR_LOGGER_SYSLOG);
		if (daemon(nochdir, 0) != 0) {
			pr_err("Daemonization failed\n");
			goto out;
		}
	} else {
		/*
		 * Make ourselves a process group leader; if we are
		 * the group leader already then the function will do
		 * nothing (apart from setting errnor to EPERM).
		 */
		if (no_detach == 1)
			setsid();
	}

	if (create_lock_file()) {
		pr_err("Failed to create lock file: %s\n", strerr(errno));
		goto out;
	}

	if (generate_sub_auth())
		pr_debug("Failed to generate subauth for domain sid: %s\n",
				strerr(errno));

	worker_pid = start_worker_process(worker_process_init);
	if (worker_pid < 0)
		goto out;

	while (1) {
		int status;
		pid_t child;

		child = waitpid(-1, &status, 0);
		if (ksmbd_health_status & KSMBD_SHOULD_RELOAD_CONFIG &&
				errno == EINTR) {
			ksmbd_health_status &= ~KSMBD_SHOULD_RELOAD_CONFIG;
			continue;
		}

		pr_err("WARNING: child process exited abnormally: %d\n",
				child);
		if (child == -1) {
			pr_err("waitpid() returned error code: %s\n",
				strerr(errno));
			goto out;
		}

		if (WIFEXITED(status) &&
			WEXITSTATUS(status) == KSMBD_STATUS_IPC_FATAL_ERROR) {
			pr_err("Fatal IPC error. Terminating. Check dmesg.\n");
			goto out;
		}

		/* Ratelimit automatic restarts */
		sleep(1);
		worker_pid = start_worker_process(worker_process_init);
		if (worker_pid < 0)
			goto out;
	}
out:
	delete_lock_file();
	kill(0, SIGTERM);
	return 0;
}

static int manager_systemd_service(void)
{
	pid_t __pid;

	__pid = start_worker_process(manager_process_init);
	if (__pid < 0)
		return -EINVAL;

	return 0;
}

static struct option opts[] = {
	{"port",	required_argument,	NULL,	'p' },
	{"config",	required_argument,	NULL,	'c' },
	{"users",	required_argument,	NULL,	'u' },
	{"systemd",	no_argument,		NULL,	's' },
	{"nodetach",	optional_argument,	NULL,	'n' },
	{"help",	no_argument,		NULL,	'h' },
	{"version",	no_argument,		NULL,	'V' },
	{NULL,		0,			NULL,	 0  }
};

#ifdef MULTICALL
int ksmbd_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	int ret = EXIT_FAILURE;
	int systemd_service = 0;
	int c;

	set_logger_app_name("ksmbd-manager");
	memset(&global_conf, 0x00, sizeof(struct smbconf_global));
	global_conf.pwddb = PATH_PWDDB;
	global_conf.smbconf = PATH_SMBCONF;
	pr_logger_init(PR_LOGGER_STDIO);

	while ((c = getopt_long(argc, argv, "n::p:c:u:sVh", opts, NULL)) != EOF)
		switch (c) {
		case 'p':
			global_conf.tcp_port = cp_get_group_kv_long(optarg);
			pr_debug("TCP port option override\n");
			break;
		case 'c':
			global_conf.smbconf = g_strdup(optarg);
			break;
		case 'u':
			global_conf.pwddb = g_strdup(optarg);
			break;
		case 'n':
			if (!optarg)
				no_detach = 1;
			else
				no_detach = cp_get_group_kv_long(optarg);
			break;
		case 's':
			systemd_service = 1;
			break;
		case 'V':
			ret = show_version();
			goto out;
		case 'h':
			ret = EXIT_SUCCESS;
			/* Fall through */
		case '?':
		default:
			usage(ret);
			goto out;
		}

	if (argc > optind) {
		usage(ret);
		goto out;
	}

	if (!global_conf.smbconf || !global_conf.pwddb) {
		pr_err("Out of memory\n");
		goto out;
	}

	setup_signals(manager_sig_handler);
	if (!systemd_service)
		ret = manager_process_init();
	else
		ret = manager_systemd_service();
out:
	return ret;
}
