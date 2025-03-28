/********************************************************************\
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 59 Temple Place - Suite 330        Fax:    +1-617-542-2652       *
 * Boston, MA  02111-1307,  USA       gnu@gnu.org                   *
 *                                                                  *
 \********************************************************************/

/** @file conf.c
  @brief Config file parsing
  @author Copyright (C) 2004 Philippe April <papril777@yahoo.com>
  @author Copyright (C) 2007 Paul Kube <nodogsplash@kokoro.ucsd.edu>
  @author Copyright (C) 2015-2023 Modifications and additions by BlueWave Projects and Services <opennds@blue-wave.net>
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <pthread.h>

#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/ether.h>

#include "common.h"
#include "safe.h"
#include "debug.h"
#include "conf.h"
#include "auth.h"
#include "util.h"
#include "http_microhttpd_utils.h"

/** @internal
 * Holds the current configuration of the gateway */
static s_config config = {{0}};

/**
 * Mutex for the configuration file, used by the auth_servers related
 * functions. */
pthread_mutex_t config_mutex = PTHREAD_MUTEX_INITIALIZER;

/** @internal
 * A flag.  If set to 1, there are missing or empty mandatory parameters in the config
 */
static int missing_parms;

/** @internal
 The different configuration options */
typedef enum {
	oBadOption,
	oSessionTimeout,
	oDaemon,
	oDebugLevel,
	oMaxClients,
	oOnlineStatus,
	oGatewayName,
	oEnableSerialNumberSuffix,
	oGatewayInterface,
	oGatewayIPRange,
	oGatewayIP,
	// TODO: deprecate oGatewayAddress option
	oGatewayAddress,
	oGatewayPort,
	oGatewayFQDN,
	oStatusPath,
	oDhcpDefaultUrlEnable,
	oFasPort,
	oFasKey,
	oFasPath,
	oFasRemoteIP,
	oFasRemoteFQDN,
	oFasURL,
	oFasSSL,
	oLoginOptionEnabled,
	oThemeSpecPath,
	oUseOutdatedMHD,
	oMaxPageSize,
	oAllowPreemptiveAuthentication,
	oUnescapeCallbackEnabled,
	oFasSecureEnabled,
	oHTTPDMaxConn,
	oWebRoot,
	oPreauthIdleTimeout,
	oAuthIdleTimeout,
	oRemotesRefreshInterval,
	oCheckInterval,
	oSetMSS,
	oMSSValue,
	oRateCheckWindow,
	oDownloadRate,
	oUploadRate,
	oDownloadBucketRatio,
	oUploadBucketRatio,
	oMaxDownloadBucketSize,
	oMaxUploadBucketSize,
	oUploadUnrestrictedBursting,
	oDownloadUnrestrictedBursting,
	oDownloadQuota,
	oUploadQuota,
	oNdsctlSocket,
	oSyslogFacility,
	oFirewallRule,
	oFirewallRuleSet,
	oEmptyRuleSetPolicy,
	oMACmechanism,
	oTrustedMACList,
	oBlockedMACList,
	oAllowedMACList,
	oFWMarkAuthenticated,
	oFWMarkTrusted,
	oFWMarkBlocked,
	oBinAuth,
	oPreAuth,
	oWalledGardenFqdnList,
	oWalledGardenPortList,
	oFasCustomParametersList,
	oFasCustomVariablesList,
	oFasCustomImagesList,
	oFasCustomFilesList,
	oLogMountpoint,
	oMaxLogEntries
} OpCodes;

/** @internal
 The config file keywords for the different configuration options */
static const struct {
	const char *name;
	OpCodes opcode;
	int required;
} keywords[] = {
	{ "sessiontimeout", oSessionTimeout },
	{ "daemon", oDaemon },
	{ "debuglevel", oDebugLevel },
	{ "maxclients", oMaxClients },
	{ "online_status", oOnlineStatus },
	{ "gatewayname", oGatewayName },
	{ "enable_serial_number_suffix", oEnableSerialNumberSuffix },
	{ "gatewayinterface", oGatewayInterface },
	{ "gatewayiprange", oGatewayIPRange },
	{ "gatewayip", oGatewayIP },
	// TODO: remove/deprecate gatewayaddress keyword
	{ "gatewayaddress", oGatewayAddress },
	{ "gatewayport", oGatewayPort },
	{ "gatewayfqdn", oGatewayFQDN },
	{ "statuspath", oStatusPath },
	{ "dhcp_default_url_enable",oDhcpDefaultUrlEnable },
	{ "fasport", oFasPort },
	{ "faskey", oFasKey },
	{ "fasremoteip", oFasRemoteIP },
	{ "fasremotefqdn", oFasRemoteFQDN },
	{ "fasurl", oFasURL },
	{ "fasssl", oFasSSL },
	{ "login_option_enabled", oLoginOptionEnabled },
	{ "themespec_path", oThemeSpecPath },
	{ "use_outdated_mhd", oUseOutdatedMHD },
	{ "max_page_size", oMaxPageSize },
	{ "allow_preemptive_authentication", oAllowPreemptiveAuthentication },
	{ "unescape_callback_enabled", oUnescapeCallbackEnabled },
	{ "fas_secure_enabled", oFasSecureEnabled },
	{ "faspath", oFasPath },
	{ "webroot", oWebRoot },
	{ "preauthidletimeout", oPreauthIdleTimeout },
	{ "authidletimeout", oAuthIdleTimeout },
	{ "remotes_refresh_interval", oRemotesRefreshInterval },
	{ "checkinterval", oCheckInterval },
	{ "setmss", oSetMSS },
	{ "mssvalue", oMSSValue },
	{ "ratecheckwindow", oRateCheckWindow },
	{ "downloadrate", oDownloadRate },
	{ "uploadrate", oUploadRate },
	{ "download_bucket_ratio", oDownloadBucketRatio },
	{ "upload_bucket_ratio", oUploadBucketRatio },
	{ "max_download_bucket_size", oMaxDownloadBucketSize },
	{ "max_upload_bucket_size", oMaxUploadBucketSize },
	{ "upload_unrestricted_bursting", oUploadUnrestrictedBursting },
	{ "download_unrestricted_bursting", oDownloadUnrestrictedBursting },
	{ "downloadquota", oDownloadQuota },
	{ "uploadquota", oUploadQuota },
	{ "syslogfacility", oSyslogFacility },
	{ "ndsctlsocket", oNdsctlSocket },
	{ "firewallruleset", oFirewallRuleSet },
	{ "firewallrule", oFirewallRule },
	{ "emptyrulesetpolicy", oEmptyRuleSetPolicy },
	{ "trustedmaclist", oTrustedMACList },
	{ "blockedmaclist", oBlockedMACList },
	{ "allowedmaclist", oAllowedMACList },
	{ "MACmechanism", oMACmechanism },
	{ "fw_mark_authenticated", oFWMarkAuthenticated },
	{ "fw_mark_trusted", oFWMarkTrusted },
	{ "fw_mark_blocked", oFWMarkBlocked },
	{ "binauth", oBinAuth },
	{ "preauth", oPreAuth },
	{ "walledgarden_fqdn_list", oWalledGardenFqdnList },
	{ "walledgarden_port_list", oWalledGardenPortList },
	{ "fas_custom_parameters_list", oFasCustomParametersList },
	{ "fas_custom_variables_list", oFasCustomVariablesList },
	{ "fas_custom_images_list", oFasCustomImagesList },
	{ "fas_custom_files_list", oFasCustomFilesList },
	{ "max_log_entries", oMaxLogEntries },
	{ "log_mountpoint", oLogMountpoint },
	{ NULL, oBadOption },
};

static void config_notnull(const void *parm, const char *parmname);
static int parse_boolean(const char *);
static void _parse_firewall_rule(t_firewall_ruleset *ruleset, char *leftover);
static void parse_firewall_ruleset(const char *, FILE *, const char *, int *);

static OpCodes config_parse_opcode(const char *cp, const char *filename, int linenum);

/** @internal
Strip comments and leading and trailing whitespace from a string.
Return a pointer to the first nonspace char in the string.
*/
static char* _strip_whitespace(char* p1);

/** Accessor for the current gateway configuration
@return:  A pointer to the current config.  The pointer isn't opaque, but should be treated as READ-ONLY
 */
s_config *
config_get_config(void)
{
	return &config;
}

// Sets the default config parameters and initialises the configuration system
void
config_init(void)
{
	t_firewall_ruleset *rs;

	debug(LOG_DEBUG, "Setting default config parameters");
	strncpy(config.configfile, DEFAULT_CONFIGFILE, sizeof(config.configfile)-1);
	config.session_timeout = DEFAULT_SESSION_TIMEOUT;
	config.debuglevel = DEFAULT_DEBUGLEVEL;
	config.maxclients = DEFAULT_MAXCLIENTS;
	config.online_status = DEFAULT_ONLINE_STATUS;
	config.gw_name = safe_strdup(DEFAULT_GATEWAYNAME);
	config.enable_serial_number_suffix = DEFAULT_ENABLE_SERIAL_NUMBER_SUFFIX;
	config.http_encoded_gw_name = NULL;
	config.url_encoded_gw_name = NULL;
	config.gw_fqdn = safe_strdup(DEFAULT_GATEWAYFQDN);
	config.status_path = safe_strdup(DEFAULT_STATUSPATH);
	config.dhcp_default_url_enable = DEFAULT_DHCP_DEFAULT_URL_ENABLE;
	config.gw_interface = safe_strdup(DEFAULT_GATEWAYINTERFACE);;
	config.gw_iprange = safe_strdup(DEFAULT_GATEWAY_IPRANGE);
	config.gw_address = NULL;
	config.gw_ip = NULL;
	config.gw_port = DEFAULT_GATEWAYPORT;
	config.fas_port = DEFAULT_FASPORT;
	config.fas_key = safe_strdup(DEFAULT_FASKEY);
	config.login_option_enabled = DEFAULT_LOGIN_OPTION_ENABLED;
	config.themespec_path = NULL;
	config.use_outdated_mhd = DEFAULT_USE_OUTDATED_MHD;
	config.max_page_size = DEFAULT_MAX_PAGE_SIZE;
	config.max_log_entries = DEFAULT_MAX_LOG_ENTRIES;
	config.allow_preemptive_authentication = DEFAULT_ALLOW_PREEMPTIVE_AUTHENTICATION;
	config.unescape_callback_enabled = DEFAULT_UNESCAPE_CALLBACK_ENABLED;
	config.fas_secure_enabled = DEFAULT_FAS_SECURE_ENABLED;
	config.fas_remoteip = NULL;
	config.fas_remotefqdn = NULL;
	config.fas_url = NULL;
	config.fas_ssl = NULL;
	config.fas_hid = NULL;
	config.custom_params = NULL;
	config.custom_vars = NULL;
	config.custom_images = NULL;
	config.custom_files = NULL;
	config.tmpfsmountpoint = NULL;
	config.log_mountpoint = safe_strdup(DEFAULT_LOG_MOUNTPOINT);
	config.fas_path = DEFAULT_FASPATH;
	config.webroot = safe_strdup(DEFAULT_WEBROOT);
	config.authdir = safe_strdup(DEFAULT_AUTHDIR);
	config.denydir = safe_strdup(DEFAULT_DENYDIR);
	config.preauthdir = safe_strdup(DEFAULT_PREAUTHDIR);
	config.preauth_idle_timeout = DEFAULT_PREAUTH_IDLE_TIMEOUT,
	config.auth_idle_timeout = DEFAULT_AUTH_IDLE_TIMEOUT,
	config.remotes_refresh_interval = DEFAULT_REMOTES_REFRESH_INTERVAL,
	config.checkinterval = DEFAULT_CHECKINTERVAL;
	config.daemon = -1;
	config.set_mss = DEFAULT_SET_MSS;
	config.mss_value = DEFAULT_MSS_VALUE;
	config.rate_check_window = DEFAULT_RATE_CHECK_WINDOW;
	config.upload_rate =  DEFAULT_UPLOAD_RATE;
	config.download_rate = DEFAULT_DOWNLOAD_RATE;
	config.download_bucket_ratio = DEFAULT_DOWNLOAD_BUCKET_RATIO;
	config.max_download_bucket_size = DEFAULT_MAX_DOWNLOAD_BUCKET_SIZE;
	config.download_unrestricted_bursting = DEFAULT_DOWNLOAD_UNRESTRICTED_BURSTING;
	config.upload_bucket_ratio =  DEFAULT_UPLOAD_BUCKET_RATIO;
	config.max_upload_bucket_size = DEFAULT_MAX_UPLOAD_BUCKET_SIZE;
	config.upload_unrestricted_bursting = DEFAULT_UPLOAD_UNRESTRICTED_BURSTING;
	config.upload_quota =  DEFAULT_UPLOAD_QUOTA;
	config.download_quota = DEFAULT_DOWNLOAD_QUOTA;
	config.syslog_facility = DEFAULT_SYSLOG_FACILITY;
	config.log_syslog = DEFAULT_LOG_SYSLOG;
	config.ndsctl_sock = safe_strdup(DEFAULT_NDSCTL_SOCK);
	config.rulesets = NULL;
	config.trustedmaclist = NULL;
	config.blockedmaclist = NULL;
	config.allowedmaclist = NULL;
	config.macmechanism = DEFAULT_MACMECHANISM;
	config.fw_mark_authenticated = DEFAULT_FW_MARK_AUTHENTICATED;
	config.fw_mark_trusted = DEFAULT_FW_MARK_TRUSTED;
	config.fw_mark_blocked = DEFAULT_FW_MARK_BLOCKED;
	config.ip6 = DEFAULT_IP6;
	config.binauth = NULL;
	config.preauth = NULL;
	config.walledgarden_fqdn_list = NULL;
	config.walledgarden_port_list = NULL;
	config.fas_custom_parameters_list = NULL;
	config.lockfd = 0;
	config.online_status = 0;

	// Set up default FirewallRuleSets, and their empty ruleset policies
	rs = add_ruleset("trusted-users");
	rs->emptyrulesetpolicy = safe_strdup(DEFAULT_EMPTY_TRUSTED_USERS_POLICY);

	rs = add_ruleset("trusted-users-to-router");
	rs->emptyrulesetpolicy = safe_strdup(DEFAULT_EMPTY_TRUSTED_USERS_TO_ROUTER_POLICY);

	rs = add_ruleset("users-to-router");
	rs->emptyrulesetpolicy = safe_strdup(DEFAULT_EMPTY_USERS_TO_ROUTER_POLICY);

	rs = add_ruleset("authenticated-users");
	rs->emptyrulesetpolicy = safe_strdup(DEFAULT_EMPTY_AUTHENTICATED_USERS_POLICY);

	rs = add_ruleset("preauthenticated-users");
	rs->emptyrulesetpolicy = safe_strdup(DEFAULT_EMPTY_PREAUTHENTICATED_USERS_POLICY);
}

/**
 * If the command-line didn't specify a config, use the default.
 */
void
config_init_override(void)
{
	if (config.daemon == -1) {
		config.daemon = DEFAULT_DAEMON;
	}
}

/** @internal
Attempts to parse an opcode from the config file
*/
static OpCodes
config_parse_opcode(const char *cp, const char *filename, int linenum)
{
	int i;

	for (i = 0; keywords[i].name; i++) {
		if (strcasecmp(cp, keywords[i].name) == 0) {
			return keywords[i].opcode;
		}
	}

	debug(LOG_ERR, "%s: line %d: Bad configuration option: %s", filename, linenum, cp);
	return oBadOption;
}

/**
Advance to the next word
@param s string to parse, this is the next_word pointer, the value of s
	 when the macro is called is the current word, after the macro
	 completes, s contains the beginning of the NEXT word, so you
	 need to save s to something else before doing TO_NEXT_WORD
@param e should be 0 when calling TO_NEXT_WORD(), it'll be changed to 1
	 if the end of the string is reached.
*/
#define TO_NEXT_WORD(s, e) do { \
	while (*s != '\0' && !isblank(*s)) { \
		s++; \
	} \
	if (*s != '\0') { \
		*s = '\0'; \
		s++; \
		while (isblank(*s)) \
			s++; \
	} else { \
		e = 1; \
	} \
} while (0)

/** Add a firewall ruleset with the given name, and return it.
 *  Do not allow duplicates. */
t_firewall_ruleset *
add_ruleset(const char rulesetname[])
{
	t_firewall_ruleset *ruleset;

	ruleset = get_ruleset(rulesetname);

	if (ruleset != NULL) {
		debug(LOG_DEBUG, "add_ruleset(): FirewallRuleSet %s already exists.", rulesetname);
		return ruleset;
	}

	debug(LOG_DEBUG, "add_ruleset(): Creating FirewallRuleSet %s ruleset [%s].", rulesetname, ruleset);

	// Create and place at head of config.rulesets
	ruleset = safe_calloc(sizeof(t_firewall_ruleset));
	memset(ruleset, 0, sizeof(t_firewall_ruleset));
	ruleset->name = safe_strdup(rulesetname);
	ruleset->next = config.rulesets;
	config.rulesets = ruleset;

	return ruleset;
}

/** @internal
Parses an empty ruleset policy directive
*/
static void
parse_empty_ruleset_policy(char *ptr, const char *filename, int lineno)
{
	char *rulesetname, *policy;
	t_firewall_ruleset *ruleset;

	// find first whitespace delimited word; this is ruleset name
	while ((*ptr != '\0') && (isblank(*ptr))) ptr++;
	rulesetname = ptr;
	while ((*ptr != '\0') && (!isblank(*ptr))) ptr++;
	*ptr = '\0';

	// get the ruleset struct with this name; error if it doesn't exist
	debug(LOG_DEBUG, "Parsing EmptyRuleSetPolicy for %s", rulesetname);
	ruleset = get_ruleset(rulesetname);
	if (ruleset == NULL) {
		debug(LOG_ERR, "Unrecognized FirewallRuleSet name: %s at line %d in %s", rulesetname, lineno, filename);
		debug(LOG_ERR, "Exiting...");
		exit(1);
	}

	// find next whitespace delimited word; this is policy name
	ptr++;
	while ((*ptr != '\0') && (isblank(*ptr))) ptr++;
	policy = ptr;
	while ((*ptr != '\0') && (!isblank(*ptr))) ptr++;
	*ptr = '\0';

	/* make sure policy is one of the possible ones:
	 "passthrough" means iptables RETURN
	 "allow" means iptables ACCEPT
	 "block" means iptables REJECT
	*/
	if (ruleset->emptyrulesetpolicy != NULL) free(ruleset->emptyrulesetpolicy);
	if (!strcasecmp(policy,"passthrough")) {
		ruleset->emptyrulesetpolicy =  safe_strdup("RETURN");
	} else if (!strcasecmp(policy,"allow")) {
		ruleset->emptyrulesetpolicy =  safe_strdup("ACCEPT");
	} else if (!strcasecmp(policy,"block")) {
		ruleset->emptyrulesetpolicy =  safe_strdup("REJECT");
	} else {
		debug(LOG_ERR, "Unknown EmptyRuleSetPolicy directive: %s at line %d in %s", policy, lineno, filename);
		debug(LOG_ERR, "Exiting...");
		exit(1);
	}

	debug(LOG_DEBUG, "Set EmptyRuleSetPolicy for %s to %s", rulesetname, policy);
}



/** @internal
Parses firewall rule set information
*/
static void
parse_firewall_ruleset(const char *rulesetname, FILE *fd, const char *filename, int *linenum)
{
	char line[MAX_BUF], *p1, *p2;
	t_firewall_ruleset *ruleset;
	int opcode;

	// find whitespace delimited word in ruleset string; this is its name
	p1 = strchr(rulesetname,' ');
	if (p1) *p1 = '\0';
	p1 = strchr(rulesetname,'\t');
	if (p1) *p1 = '\0';

	debug(LOG_DEBUG, "Parsing FirewallRuleSet %s", rulesetname);
	ruleset = get_ruleset(rulesetname);
	if (ruleset == NULL) {
		debug(LOG_ERR, "Unrecognized FirewallRuleSet name: %s", rulesetname);
		debug(LOG_ERR, "Exiting...");
		exit(1);
	}

	// Parsing the rules in the set
	while (fgets(line, MAX_BUF, fd)) {
		(*linenum)++;
		p1 = _strip_whitespace(line);

		// if nothing left, get next line
		if (p1[0] == '\0') continue;

		// if closing brace, we are done
		if (p1[0] == '}') break;

		// next, we coopt the parsing of the regular config

		// keep going until word boundary is found.
		p2 = p1;
		while ((*p2 != '\0') && (!isblank(*p2))) p2++;
		// if this is end of line, it's a problem
		if (p2[0] == '\0') {
			debug(LOG_ERR, "FirewallRule incomplete on line %d in %s", *linenum, filename);
			debug(LOG_ERR, "Exiting...");
			exit(1);
		}
		// terminate first word, point past it
		*p2 = '\0';
		p2++;

		// skip whitespace to point at arg
		while (isblank(*p2)) p2++;

		// Get opcode
		opcode = config_parse_opcode(p1, filename, *linenum);

		debug(LOG_DEBUG, "p1 = [%s]; p2 = [%s]", p1, p2);

		switch (opcode) {
		case oFirewallRule:
			_parse_firewall_rule(ruleset, p2);
			break;

		case oBadOption:
		default:
			debug(LOG_ERR, "Bad option %s parsing FirewallRuleSet on line %d in %s", p1, *linenum, filename);
			debug(LOG_ERR, "Exiting...");
			exit(1);
			break;
		}
	}
	debug(LOG_DEBUG, "FirewallRuleSet %s parsed.", rulesetname);
}

/** @internal
Helper for parse_firewall_ruleset.  Parses a single rule in a ruleset
*/
static void
_parse_firewall_rule(t_firewall_ruleset *ruleset, char *leftover)
{
	int i;
	t_firewall_target target = TARGET_REJECT; // firewall target
	int all_nums = 1; // If 0, word contained illegal chars
	int finished = 0; // reached end of line
	char *token = NULL; // First word
	char *port = NULL; // port(s) to allow/block
	char *protocol = NULL; // protocol to allow/block: tcp/udp/icmp/all
	char *mask = NULL; // Netmask
	char *ipset = NULL; // ipset
	char *other_kw = NULL; // other key word
	t_firewall_rule *tmp;
	t_firewall_rule *tmp2;

	// lowercase everything
	for (i = 0; *(leftover + i) != '\0'
			&& (*(leftover + i) = tolower((unsigned char)*(leftover + i))); i++);
	token = leftover;
	TO_NEXT_WORD(leftover, finished);

	// Parse token
	if (!strcasecmp(token, "block")) {
		target = TARGET_REJECT;
	} else if (!strcasecmp(token, "drop")) {
		target = TARGET_DROP;
	} else if (!strcasecmp(token, "allow")) {
		target = TARGET_ACCEPT;
	} else if (!strcasecmp(token, "passthrough")) {
		target = TARGET_RETURN;
	} else if (!strcasecmp(token, "log")) {
		target = TARGET_LOG;
	} else if (!strcasecmp(token, "ulog")) {
		target = TARGET_ULOG;
	} else {
		debug(LOG_ERR, "Invalid rule type %s, expecting "
			  "\"block\",\"drop\",\"allow\",\"passthrough\",\"log\" or \"ulog\"", token);
		exit(1);
	}

	// Parse the remainder

	// Get the optional protocol
	if (strncmp(leftover, "tcp", 3) == 0
			|| strncmp(leftover, "udp", 3) == 0
			|| strncmp(leftover, "all", 3) == 0
			|| strncmp(leftover, "icmp", 4) == 0) {
		protocol = leftover;
		TO_NEXT_WORD(leftover, finished);
	}

	// Get the optional port or port range
	if (strncmp(leftover, "port", 4) == 0) {
		if (protocol == NULL ||
				!(strncmp(protocol, "tcp", 3) == 0 || strncmp(protocol, "udp", 3) == 0)) {
			debug(LOG_ERR, "Port without tcp or udp protocol");
			exit(1);
		}
		TO_NEXT_WORD(leftover, finished);
		// Get port now
		port = leftover;
		TO_NEXT_WORD(leftover, finished);
		for (i = 0; *(port + i) != '\0'; i++)
			if (!isdigit((unsigned char)*(port + i)) && ((unsigned char)*(port + i) != ':'))
				all_nums = 0; // No longer only digits or
		if (!all_nums) {
			debug(LOG_ERR, "Invalid port %s", port);
			exit(1);
		}
	}

	if (strncmp(leftover, "ipset", 5) == 0) {
		TO_NEXT_WORD(leftover, finished);
		// Get ipset now
		ipset = leftover;
		TO_NEXT_WORD(leftover, finished);

		// TODO check if ipset exists
	}

	// Now, look for optional IP address/mask
	if (!finished) {
		// should be exactly "to"
		other_kw = leftover;
		TO_NEXT_WORD(leftover, finished);
		if (strcmp(other_kw, "to") || finished) {
			debug(LOG_ERR, "Invalid or unexpected keyword %s, "
				  "expecting \"to\"", other_kw);
			exit(1);
		}

		// Get IP address/mask now
		mask = leftover;
		TO_NEXT_WORD(leftover, finished);
		all_nums = 1;
		for (i = 0; *(mask + i) != '\0'; i++)
			if (!isdigit((unsigned char)*(mask + i)) && (*(mask + i) != '.')
					&& (*(mask + i) != '/'))
				all_nums = 0; // No longer only digits or . or /
		if (!all_nums) {
			debug(LOG_ERR, "Invalid mask %s", mask);
			exit(1);
		}
	}

	// Generate rule record
	tmp = safe_calloc(sizeof(t_firewall_rule));
	memset((void *)tmp, 0, sizeof(t_firewall_rule));
	tmp->target = target;
	if (protocol != NULL)
		tmp->protocol = safe_strdup(protocol);
	if (port != NULL)
		tmp->port = safe_strdup(port);
	if (ipset != NULL)
		tmp->ipset = safe_strdup(ipset);
	if (mask == NULL)
		tmp->mask = safe_strdup("0.0.0.0/0");
	else
		tmp->mask = safe_strdup(mask);

	debug(LOG_DEBUG, "Adding FirewallRule %s %s port %s to %s to FirewallRuleset %s", token, tmp->protocol, tmp->port, tmp->mask, ruleset->name);

	// Add the rule record
	if (ruleset->rules == NULL) {
		// No rules...
		ruleset->rules = tmp;
	} else {
		tmp2 = ruleset->rules;
		while (tmp2->next != NULL)
			tmp2 = tmp2->next;
		tmp2->next = tmp;
	}
}

int
is_empty_ruleset (const char *rulesetname)
{
	return get_ruleset_list(rulesetname) == NULL;
}

char *
get_empty_ruleset_policy(const char *rulesetname)
{
	t_firewall_ruleset *rs;

	rs = get_ruleset(rulesetname);
	return rs ? rs->emptyrulesetpolicy : NULL;
}


t_firewall_ruleset *
get_ruleset(const char ruleset[])
{
	t_firewall_ruleset *tmp;

	for (tmp = config.rulesets; tmp != NULL
			&& strcmp(tmp->name, ruleset) != 0; tmp = tmp->next);
	return (tmp);
}

t_firewall_rule *
get_ruleset_list(const char *ruleset)
{
	t_firewall_ruleset *tmp;

	tmp = get_ruleset(ruleset);
	return tmp ? tmp->rules : NULL;
}

/** @internal
Strip comments and leading and trailing whitespace from a string.
Return a pointer to the first nonspace char in the string.
*/
static char*
_strip_whitespace(char* p1)
{
	// strip leading whitespace
	while(isspace(p1[0])) p1++;

	// strip comment lines
	if (p1[0] == '#') {
		p1[0] = '\0';
	}

	// strip trailing whitespace
	while(p1[0] != '\0' && isspace(p1[strlen(p1)-1]))
		p1[strlen(p1)-1] = '\0';
	return p1;
}

/**
@param filename Full path of the configuration file to be read
*/
void
config_read(const char *filename)
{
	FILE *fd;
	char line[MAX_BUF], *s, *p1, *p2;
	int linenum = 0, opcode, value;
	struct stat sb;
	char msg[128] = {0};
	char *cmd = NULL;
	char *lockfile;

	safe_asprintf(&cmd, "/usr/lib/opennds/libopennds.sh clean");

	execute_ret_url_encoded(msg, sizeof(msg) - 1, cmd);
	free(cmd);

	if (strlen(msg) == 0) {
		config.tmpfsmountpoint = safe_strdup("/tmp");
	} else {
		config.tmpfsmountpoint = safe_strdup(msg);
	}

	safe_asprintf(&lockfile, "%s/ndsctl.lock", config.tmpfsmountpoint);

	//Remove ndsctl lock file if it exists
	if ((fd = fopen(lockfile, "r")) != NULL) {
		fclose(fd);
		remove(lockfile);
	}

	free (lockfile);

	debug(LOG_INFO, "Reading configuration file '%s'", filename);

	if (!(fd = fopen(filename, "r"))) {
		debug(LOG_ERR, "FATAL: Could not open configuration file '%s', "
			  "exiting...", filename);
		exit(1);
	}

	while (fgets(line, MAX_BUF, fd)) {
		linenum++;
		s = _strip_whitespace(line);

		// if nothing left, get next line
		if (s[0] == '\0') continue;

		/* now we require the line must have form: <option><whitespace><arg>
		 * even if <arg> is just a left brace, for example
		 */

		// find first word (i.e. option) end boundary
		p1 = s;
		while ((*p1 != '\0') && (!isspace(*p1))) p1++;
		// if this is end of line, it's a problem
		if (p1[0] == '\0') {
			debug(LOG_ERR, "Option %s requires argument on line %d in %s", s, linenum, filename);
			debug(LOG_ERR, "Exiting...");
			exit(1);
		}

		// terminate option, point past it
		*p1 = '\0';
		p1++;

		// skip any additional leading whitespace, make p1 point at start of arg
		while (isblank(*p1)) p1++;

		debug(LOG_DEBUG, "Parsing option: %s, arg: %s", s, p1);
		opcode = config_parse_opcode(s, filename, linenum);

		switch(opcode) {
		case oSessionTimeout:
			if (sscanf(p1, "%d", &config.session_timeout) < 1 || config.session_timeout < 0) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oDaemon:
			if (config.daemon == -1 && ((value = parse_boolean(p1)) != -1)) {
				config.daemon = value;
			}
			break;
		case oDebugLevel:
			if (sscanf(p1, "%d", &config.debuglevel) < 1 || config.debuglevel < DEBUGLEVEL_MIN) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s. Valid levels are %d...%d.",
					p1, s, linenum, filename, DEBUGLEVEL_MIN, DEBUGLEVEL_MAX);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			} else if (config.debuglevel > DEBUGLEVEL_MAX) {
				config.debuglevel = DEBUGLEVEL_MAX;
				debug(LOG_WARNING, "Invalid debug level. Set to maximum.");
			}
			break;
		case oMaxClients:
			if (sscanf(p1, "%d", &config.maxclients) < 1 || config.maxclients < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oGatewayName:
			config.gw_name = safe_strdup(p1);
			break;
		case oEnableSerialNumberSuffix:
			if (sscanf(p1, "%d", &config.enable_serial_number_suffix) < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oGatewayFQDN:
			config.gw_fqdn = safe_strdup(p1);
			break;
		case oStatusPath:
			if (!((stat(p1, &sb) == 0) && S_ISREG(sb.st_mode) && (sb.st_mode & S_IXUSR))) {
				debug(LOG_ERR, "StatusPath does not exist or is not executable: %s", p1);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			config.status_path = safe_strdup(p1);
			break;
		case oDhcpDefaultUrlEnable:
			if (sscanf(p1, "%d", &config.dhcp_default_url_enable) < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oGatewayInterface:
			config.gw_interface = safe_strdup(p1);
			break;
		case oGatewayIPRange:
			config.gw_iprange = safe_strdup(p1);
			break;
		// TODO: deprecate oGatewayAddress option
		case oGatewayAddress:
		case oGatewayIP:
			config.gw_ip = safe_strdup(p1);
			break;
		case oGatewayPort:
			if (sscanf(p1, "%u", &config.gw_port) < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oFasPort:
			if (sscanf(p1, "%u", &config.fas_port) < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oLoginOptionEnabled:
			if (sscanf(p1, "%d", &config.login_option_enabled) < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oThemeSpecPath:
			config.themespec_path = safe_strdup(p1);
			if (!((stat(p1, &sb) == 0) && S_ISREG(sb.st_mode) && (sb.st_mode & S_IXUSR))) {
				debug(LOG_ERR, "ThemeSpec program does not exist or is not executable: %s", p1);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			debug(LOG_INFO, "Added ThemeSpec [%s]", p1);
			break;
		case oUseOutdatedMHD:
			if (sscanf(p1, "%d", &config.use_outdated_mhd) < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oMaxPageSize:
			if (sscanf(p1, "%llu", &config.max_page_size) < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oMaxLogEntries:
			if (sscanf(p1, "%llu", &config.max_log_entries) < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oLogMountpoint:
			config.log_mountpoint = safe_strdup(p1);
			break;
		case oAllowPreemptiveAuthentication:
			if (sscanf(p1, "%d", &config.allow_preemptive_authentication) < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oUnescapeCallbackEnabled:
			if (sscanf(p1, "%d", &config.unescape_callback_enabled) < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oFasSecureEnabled:
			if (sscanf(p1, "%d", &config.fas_secure_enabled) < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oFasPath:
			config.fas_path = safe_strdup(p1);
			break;
		case oFasKey:
			config.fas_key = safe_strdup(p1);
			break;
		case oFasRemoteIP:
			config.fas_remoteip = safe_strdup(p1);
			break;
		case oFasRemoteFQDN:
			config.fas_remotefqdn = safe_strdup(p1);
			break;
		case oBinAuth:
			config.binauth = safe_strdup(p1);
			if (!((stat(p1, &sb) == 0) && S_ISREG(sb.st_mode) && (sb.st_mode & S_IXUSR))) {
				debug(LOG_ERR, "binauth program does not exist or is not executable: %s", p1);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oPreAuth:
			config.preauth = safe_strdup(p1);
			if (!((stat(p1, &sb) == 0) && S_ISREG(sb.st_mode) && (sb.st_mode & S_IXUSR))) {
				debug(LOG_ERR, "preauth program does not exist or is not executable: %s", p1);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oFirewallRuleSet:
			parse_firewall_ruleset(p1, fd, filename, &linenum);
			break;
		case oEmptyRuleSetPolicy:
			parse_empty_ruleset_policy(p1, filename, linenum);
			break;
		case oTrustedMACList:
			parse_trusted_mac_list(p1);
			break;
		case oBlockedMACList:
			parse_blocked_mac_list(p1);
			break;
		case oAllowedMACList:
			parse_allowed_mac_list(p1);
			break;
		case oWalledGardenFqdnList:
			parse_walledgarden_fqdn_list(p1);
			break;
		case oWalledGardenPortList:
			parse_walledgarden_port_list(p1);
			break;
		case oFasCustomParametersList:
			parse_fas_custom_parameters_list(p1);
			break;
		case oFasCustomVariablesList:
			parse_fas_custom_variables_list(p1);
			break;
		case oFasCustomImagesList:
			parse_fas_custom_images_list(p1);
			break;
		case oFasCustomFilesList:
			parse_fas_custom_files_list(p1);
			break;
		case oMACmechanism:
			if (!strcasecmp("allow", p1)) {
				config.macmechanism = MAC_ALLOW;
			} else if (!strcasecmp("block", p1)) {
				config.macmechanism = MAC_BLOCK;
			} else {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oWebRoot:
			// remove any trailing slashes from webroot path
			while((p2 = strrchr(p1,'/')) == (p1 + strlen(p1) - 1)) *p2 = '\0';
			config.webroot = safe_strdup(p1);
			break;
		case oAuthIdleTimeout:
			if (sscanf(p1, "%d", &config.auth_idle_timeout) < 1 || config.auth_idle_timeout < 0) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oPreauthIdleTimeout:
			if (sscanf(p1, "%d", &config.preauth_idle_timeout) < 1 || config.preauth_idle_timeout < 0) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oRemotesRefreshInterval:
			if (sscanf(p1, "%d", &config.remotes_refresh_interval) < 1 || config.remotes_refresh_interval < 0) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oNdsctlSocket:
			free(config.ndsctl_sock);
			config.ndsctl_sock = safe_strdup(p1);
			break;
		case oSetMSS:
			if ((value = parse_boolean(p1)) != -1) {
				config.set_mss = value;
			} else {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oMSSValue:
			if (sscanf(p1, "%d", &config.mss_value) < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oRateCheckWindow:
			if (sscanf(p1, "%d", &config.rate_check_window) < 1 || config.rate_check_window < 0) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oDownloadRate:
			if (sscanf(p1, "%llu", &config.download_rate) < 1 || config.download_rate < 0) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oUploadRate:
			if (sscanf(p1, "%llu", &config.upload_rate) < 1 || config.upload_rate < 0) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oDownloadBucketRatio:
			if (sscanf(p1, "%llu", &config.download_bucket_ratio) < 1 || config.download_bucket_ratio < 0) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oUploadBucketRatio:
			if (sscanf(p1, "%llu", &config.upload_bucket_ratio) < 1 || config.upload_bucket_ratio < 0) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oMaxDownloadBucketSize:
			if (sscanf(p1, "%llu", &config.max_download_bucket_size) < 1 || config.max_download_bucket_size < 0) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oMaxUploadBucketSize:
			if (sscanf(p1, "%llu", &config.max_upload_bucket_size) < 1 || config.max_upload_bucket_size < 0) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oDownloadUnrestrictedBursting:
			if (sscanf(p1, "%d", &config.download_unrestricted_bursting) < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oUploadUnrestrictedBursting:
			if (sscanf(p1, "%d", &config.upload_unrestricted_bursting) < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oDownloadQuota:
			if (sscanf(p1, "%llu", &config.download_quota) < 1 || config.download_quota < 0) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oUploadQuota:
			if (sscanf(p1, "%llu", &config.upload_quota) < 1 || config.upload_quota < 0) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oFWMarkAuthenticated:
			if (sscanf(p1, "%x", &config.fw_mark_authenticated) < 1 ||
					config.fw_mark_authenticated == 0 ||
					config.fw_mark_authenticated == config.fw_mark_blocked ||
					config.fw_mark_authenticated == config.fw_mark_trusted) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oFWMarkBlocked:
			if (sscanf(p1, "%x", &config.fw_mark_blocked) < 1 ||
					config.fw_mark_blocked == 0 ||
					config.fw_mark_blocked == config.fw_mark_authenticated ||
					config.fw_mark_blocked == config.fw_mark_trusted) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oFWMarkTrusted:
			if (sscanf(p1, "%x", &config.fw_mark_trusted) < 1 ||
					config.fw_mark_trusted == 0 ||
					config.fw_mark_trusted == config.fw_mark_authenticated ||
					config.fw_mark_trusted == config.fw_mark_blocked) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oCheckInterval:
			if (sscanf(p1, "%i", &config.checkinterval) < 1 || config.checkinterval < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oSyslogFacility:
			if (sscanf(p1, "%d", &config.syslog_facility) < 1) {
				debug(LOG_ERR, "Bad arg %s to option %s on line %d in %s", p1, s, linenum, filename);
				debug(LOG_ERR, "Exiting...");
				exit(1);
			}
			break;
		case oBadOption:
			debug(LOG_ERR, "Bad option %s on line %d in %s", s, linenum, filename);
			debug(LOG_ERR, "Exiting...");
			exit(1);
			break;
		}
	}

	fclose(fd);

	debug(LOG_INFO, "Done reading configuration file '%s'", filename);
}

/** @internal
Parses a boolean value from the config file
*/
static int
parse_boolean(const char *line)
{
	if (strcasecmp(line, "no") == 0 ||
			strcasecmp(line, "false") == 0 ||
			strcmp(line, "0") == 0) {
		return 0;
	}
	if (strcasecmp(line, "yes") == 0 ||
			strcasecmp(line, "true") == 0 ||
			strcmp(line, "1") == 0) {
		return 1;
	}

	return -1;
}

// Parse a string to see if it is valid decimal dotted quad IP V4 format
int check_ip_format(const char *possibleip)
{
	unsigned char buf[sizeof(struct in6_addr)];
	return inet_pton(AF_INET, possibleip, buf) > 0;
}

// Parse a string to see if it is valid MAC address format
int check_mac_format(const char possiblemac[])
{
	return ether_aton(possiblemac) != NULL;
}

int add_to_walledgarden_fqdn_list(const char possiblefqdn[])
{
	char fqdn[128];
	t_WGFQDN *p = NULL;
	char possiblefqdn_urlencoded[256] = {0};

	// Sanitise FQDN
	uh_urlencode(possiblefqdn_urlencoded, sizeof(possiblefqdn_urlencoded), possiblefqdn, strlen(possiblefqdn));
	debug(LOG_DEBUG, "[%s] is the urlencoded FQDN", possiblefqdn_urlencoded);

	if (strcmp(possiblefqdn_urlencoded, possiblefqdn) == 0) {

		sscanf(possiblefqdn, "%s", fqdn);

		// Add FQDN to head of list
		p = safe_calloc(sizeof(t_WGFQDN));
		p->wgfqdn = safe_strdup(fqdn);
		p->next = config.walledgarden_fqdn_list;
		config.walledgarden_fqdn_list = p;
		debug(LOG_INFO, "Added Walled Garden FQDN [%s] to list", possiblefqdn);
	} else {
		debug(LOG_WARNING, "Invalid FQDN [%s], please remove from the list; skipping..", possiblefqdn);
	}
	return 0;
}

int add_to_walledgarden_port_list(const char possibleport[])
{
	unsigned short int port;
	t_WGP *p = NULL;

	sscanf(possibleport, "%hu", &port);

	// Add walled garden port to head of list
	p = safe_calloc(sizeof(t_WGP));
	p->wgport = port;
	p->next = config.walledgarden_port_list;
	config.walledgarden_port_list = p;
	debug(LOG_INFO, "Added Walled Garden port [%hu] to list", port);
	return 0;
}

int add_to_fas_custom_parameters_list(const char possibleparam[])
{
	char param[512];
	t_FASPARAM *p = NULL;

	sscanf(possibleparam, "%s", param);

	// Add Parameter to head of list
	p = safe_calloc(sizeof(t_FASPARAM));
	p->fasparam = safe_strdup(param);
	p->next = config.fas_custom_parameters_list;

	config.fas_custom_parameters_list = p;
	debug(LOG_INFO, "Added Custom Parameter [%s]", possibleparam);
	return 0;
}

int add_to_fas_custom_variables_list(const char possiblevar[])
{
	char var[512];
	t_FASVAR *p = NULL;

	sscanf(possiblevar, "%s", var);

	// Add Variable to head of list
	p = safe_calloc(sizeof(t_FASVAR));
	p->fasvar = safe_strdup(var);
	p->next = config.fas_custom_variables_list;

	config.fas_custom_variables_list = p;
	debug(LOG_INFO, "Added Custom Variable [%s]", possiblevar);
	return 0;
}

int add_to_fas_custom_images_list(const char possibleimage[])
{
	char image[512];
	t_FASIMG *p = NULL;

	sscanf(possibleimage, "%s", image);

	// Add Image to head of list
	p = safe_calloc(sizeof(t_FASIMG));
	p->fasimg = safe_strdup(image);
	p->next = config.fas_custom_images_list;

	config.fas_custom_images_list = p;
	debug(LOG_INFO, "Added Custom Image [%s]", possibleimage);
	return 0;
}

int add_to_fas_custom_files_list(const char possiblefile[])
{
	char file[512];
	t_FASFILE *p = NULL;

	sscanf(possiblefile, "%s", file);

	// Add File to head of list
	p = safe_calloc(sizeof(t_FASFILE));
	p->fasfile = safe_strdup(file);
	p->next = config.fas_custom_files_list;

	config.fas_custom_files_list = p;
	debug(LOG_INFO, "Added Custom File [%s]", possiblefile);
	return 0;
}


int add_to_trusted_mac_list(const char possiblemac[])
{
	char mac[18];
	t_MAC *p = NULL;

	// check for valid format
	if (!check_mac_format(possiblemac)) {
		debug(LOG_WARNING, "[%s] is not a valid MAC address", possiblemac);
		debug(LOG_WARNING, "[%s]  - please remove from trustedmac list in config file", possiblemac);
		return 1;
	}

	sscanf(possiblemac, "%17[A-Fa-f0-9:]", mac);

	// See if MAC is already on the list; don't add duplicates
	for (p = config.trustedmaclist; p != NULL; p = p->next) {
		if (!strcasecmp(p->mac, mac)) {
			debug(LOG_INFO, "MAC address [%s] already on trusted list", mac);
			return 1;
		}
	}

	// Add MAC to head of list
	p = safe_calloc(sizeof(t_MAC));
	p->mac = safe_strdup(mac);
	p->next = config.trustedmaclist;
	config.trustedmaclist = p;
	debug(LOG_INFO, "Added MAC address [%s] to trusted list", mac);
	return 0;
}


/* Remove given MAC address from the config's trusted mac list.
 * Return 0 on success, nonzero on failure
 */
int remove_from_trusted_mac_list(const char possiblemac[])
{
	char mac[18];
	t_MAC **p = NULL;
	t_MAC *del = NULL;

	// check for valid format
	if (!check_mac_format(possiblemac)) {
		debug(LOG_ERR, "[%s] not a valid MAC address", possiblemac);
		return -1;
	}

	sscanf(possiblemac, "%17[A-Fa-f0-9:]", mac);

	// If empty list, nothing to do
	if (config.trustedmaclist == NULL) {
		debug(LOG_INFO, "MAC address [%s] not on empty trusted list", mac);
		return -1;
	}

	// Find MAC on the list, remove it
	for (p = &(config.trustedmaclist); *p != NULL; p = &((*p)->next)) {
		if (!strcasecmp((*p)->mac, mac)) {
			// found it
			del = *p;
			*p = del->next;
			debug(LOG_INFO, "Removed MAC address [%s] from trusted list", mac);
			free(del);
			return 0;
		}
	}

	// MAC was not on list
	debug(LOG_INFO, "MAC address [%s] not on  trusted list", mac);
	return -1;
}


/* Given a pointer to a comma or whitespace delimited sequence of
 * MAC addresses, add each MAC address to config.trustedmaclist.
 */
void parse_trusted_mac_list(const char ptr[])
{
	char *ptrcopy = NULL, *ptrcopyptr;
	char *possiblemac = NULL;

	debug(LOG_DEBUG, "Parsing string [%s] for trusted MAC addresses", ptr);

	// strsep modifies original, so let's make a copy
	ptrcopyptr = ptrcopy = safe_strdup(ptr);

	while ((possiblemac = strsep(&ptrcopy, ", \t"))) {
		if (strlen(possiblemac) > 0) {
			if (add_to_trusted_mac_list(possiblemac) < 0) {
				exit(1);
			}
		}
	}

	free(ptrcopyptr);
}

int is_blocked_mac(const char *mac)
{
	s_config *config;
	t_MAC *block_mac;

	config = config_get_config();

	if (MAC_ALLOW != config->macmechanism) {
		for (block_mac = config->blockedmaclist; block_mac != NULL; block_mac = block_mac->next) {
			if (!strcmp(block_mac->mac, mac)) {
				return 1;
			}
		}
	}

	return 0;
}

int is_allowed_mac(const char *mac)
{
	s_config *config;
	t_MAC *allow_mac;

	config = config_get_config();

	if (MAC_BLOCK != config->macmechanism) {
		for (allow_mac = config->allowedmaclist; allow_mac != NULL; allow_mac = allow_mac->next) {
			if (!strcmp(allow_mac->mac, mac)) {
				return 1;
			}
		}
	}

	return 0;
}

int is_trusted_mac(const char *mac)
{
	s_config *config;
	t_MAC *trust_mac;

	config = config_get_config();

	// Is a client even recognized here?
	for (trust_mac = config->trustedmaclist; trust_mac != NULL; trust_mac = trust_mac->next) {
		if (!strcmp(trust_mac->mac, mac)) {
			return 1;
		}
	}

	return 0;
}

/* Add given MAC address to the config's blocked mac list.
 * Return 0 on success, nonzero on failure
 */
int add_to_blocked_mac_list(const char possiblemac[])
{
	char mac[18];
	t_MAC *p = NULL;

	// check for valid format
	if (!check_mac_format(possiblemac)) {
		debug(LOG_ERR, "[%s] not a valid MAC address to block", possiblemac);
		return -1;
	}

	// abort if not using BLOCK mechanism
	if (MAC_BLOCK != config.macmechanism) {
		debug(LOG_ERR, "Attempt to access blocked MAC list but control mechanism != block");
		return -1;
	}

	sscanf(possiblemac, "%17[A-Fa-f0-9:]", mac);

	// See if MAC is already on the list; don't add duplicates
	for (p = config.blockedmaclist; p != NULL; p = p->next) {
		if (!strcasecmp(p->mac,mac)) {
			debug(LOG_INFO, "MAC address [%s] already on blocked list", mac);
			return 1;
		}
	}

	// Add MAC to head of list
	p = safe_calloc(sizeof(t_MAC));
	p->mac = safe_strdup(mac);
	p->next = config.blockedmaclist;
	config.blockedmaclist = p;
	debug(LOG_INFO, "Added MAC address [%s] to blocked list", mac);
	return 0;
}


/* Remove given MAC address from the config's blocked mac list.
 * Return 0 on success, nonzero on failure
 */
int remove_from_blocked_mac_list(const char possiblemac[])
{
	char mac[18];
	t_MAC **p = NULL;
	t_MAC *del = NULL;

	// check for valid format
	if (!check_mac_format(possiblemac)) {
		debug(LOG_ERR, "[%s] not a valid MAC address", possiblemac);
		return -1;
	}

	// abort if not using BLOCK mechanism
	if (MAC_BLOCK != config.macmechanism) {
		debug(LOG_ERR, "Attempt to access blocked MAC list but control mechanism != block");
		return -1;
	}

	sscanf(possiblemac, "%17[A-Fa-f0-9:]", mac);

	// If empty list, nothing to do
	if (config.blockedmaclist == NULL) {
		debug(LOG_INFO, "MAC address [%s] not on empty blocked list", mac);
		return -1;
	}

	// Find MAC on the list, remove it
	for (p = &config.blockedmaclist; *p != NULL; p = &((*p)->next)) {
		if (!strcasecmp((*p)->mac,mac)) {
			// found it
			del = *p;
			*p = del->next;
			debug(LOG_INFO, "Removed MAC address [%s] from blocked list", mac);
			free(del);
			return 0;
		}
	}

	// MAC was not on list
	debug(LOG_INFO, "MAC address [%s] not on  blocked list", mac);
	return -1;
}


/* Given a pointer to a comma or whitespace delimited sequence of
 * MAC addresses, add each MAC address to config.blockedmaclist
 */
void parse_blocked_mac_list(const char ptr[])
{
	char *ptrcopy = NULL, *ptrcopyptr;
	char *possiblemac = NULL;

	debug(LOG_DEBUG, "Parsing string [%s] for MAC addresses to block", ptr);

	// strsep modifies original, so let's make a copy
	ptrcopyptr = ptrcopy = safe_strdup(ptr);

	while ((possiblemac = strsep(&ptrcopy, ", \t"))) {
		if (strlen(possiblemac) > 0) {
			if (add_to_blocked_mac_list(possiblemac) < 0) {
				exit(1);
			}
		}
	}

	free(ptrcopyptr);
}

/* Add given MAC address to the config's allowed mac list.
 * Return 0 on success, nonzero on failure
 */
int add_to_allowed_mac_list(const char possiblemac[])
{
	char mac[18];
	t_MAC *p = NULL;

	// check for valid format
	if (!check_mac_format(possiblemac)) {
		debug(LOG_ERR, "[%s] not a valid MAC address to allow", possiblemac);
		return -1;
	}

	// abort if not using ALLOW mechanism
	if (MAC_ALLOW != config.macmechanism) {
		debug(LOG_ERR, "Attempt to access allowed MAC list but control mechanism != allow");
		return -1;
	}

	sscanf(possiblemac, "%17[A-Fa-f0-9:]", mac);

	// See if MAC is already on the list; don't add duplicates
	for (p = config.allowedmaclist; p != NULL; p = p->next) {
		if (!strcasecmp(p->mac, mac)) {
			debug(LOG_INFO, "MAC address [%s] already on allowed list", mac);
			return 1;
		}
	}

	// Add MAC to head of list
	p = safe_calloc(sizeof(t_MAC));
	p->mac = safe_strdup(mac);
	p->next = config.allowedmaclist;
	config.allowedmaclist = p;
	debug(LOG_INFO, "Added MAC address [%s] to allowed list", mac);
	return 0;
}


/* Remove given MAC address from the config's allowed mac list.
 * Return 0 on success, nonzero on failure
 */
int remove_from_allowed_mac_list(const char possiblemac[])
{
	char mac[18];
	t_MAC **p = NULL;
	t_MAC *del = NULL;

	// check for valid format
	if (!check_mac_format(possiblemac)) {
		debug(LOG_ERR, "[%s] not a valid MAC address", possiblemac);
		return -1;
	}

	// abort if not using ALLOW mechanism
	if (MAC_ALLOW != config.macmechanism) {
		debug(LOG_ERR, "Attempt to access allowed MAC list but control mechanism != allow");
		return -1;
	}

	sscanf(possiblemac, "%17[A-Fa-f0-9:]", mac);

	// If empty list, nothing to do
	if (config.allowedmaclist == NULL) {
		debug(LOG_INFO, "MAC address [%s] not on empty allowed list", mac);
		return -1;
	}

	// Find MAC on the list, remove it
	for (p = &(config.allowedmaclist); *p != NULL; p = &((*p)->next)) {
		if (!strcasecmp((*p)->mac, mac)) {
			// found it
			del = *p;
			*p = del->next;
			debug(LOG_INFO, "Removed MAC address [%s] from allowed list", mac);
			free(del);
			return 0;
		}
	}

	// MAC was not on list
	debug(LOG_INFO, "MAC address [%s] not on  allowed list", mac);
	return -1;
}

/* Given a pointer to a comma or whitespace delimited sequence of
 * MAC addresses, add each MAC address to config.allowedmaclist
 */
void parse_allowed_mac_list(const char ptr[])
{
	char *ptrcopy = NULL;
	char *ptrcopyptr;
	char *possiblemac = NULL;

	debug(LOG_DEBUG, "Parsing string [%s] for MAC addresses to allow", ptr);

	// strsep modifies original, so let's make a copy
	ptrcopyptr = ptrcopy = safe_strdup(ptr);

	while ((possiblemac = strsep(&ptrcopy, ", \t"))) {
		if (strlen(possiblemac) > 0) {
			if (add_to_allowed_mac_list(possiblemac) < 0) {
				exit(1);
			}
		}
	}

	free(ptrcopyptr);
}

/* Given a pointer to a comma or whitespace delimited sequence of
 * Walled Garden FQDNs, add each FQDN to config.walledgardenfqdnlist
 */
void parse_walledgarden_fqdn_list(const char ptr[])
{
	char *ptrcopy = NULL;
	char *ptrcopyptr;
	char *possiblefqdn = NULL;

	debug(LOG_DEBUG, "Parsing string [%s] for Walled Garden FQDNs to allow", ptr);

	// strsep modifies original, so let's make a copy
	ptrcopyptr = ptrcopy = safe_strdup(ptr);

	while ((possiblefqdn = strsep(&ptrcopy, ", \t"))) {
		if (strlen(possiblefqdn) > 0) {
			if (add_to_walledgarden_fqdn_list(possiblefqdn) < 0) {
				exit(1);
			}
		}
	}

	free(ptrcopyptr);
}

/* Given a pointer to a comma or whitespace delimited sequence of
 * Walled Garden FQDN ports, add each port number to config.walledgardenportlist
 */
void parse_walledgarden_port_list(const char ptr[])
{
	char *ptrcopy = NULL;
	char *ptrcopyptr;
	char *possibleport = NULL;

	debug(LOG_DEBUG, "Parsing string [%s] for Walled Garden Ports to allow", ptr);

	// strsep modifies original, so let's make a copy
	ptrcopyptr = ptrcopy = safe_strdup(ptr);

	while ((possibleport = strsep(&ptrcopy, ", \t"))) {
		if (strlen(possibleport) > 0) {
			if (add_to_walledgarden_port_list(possibleport) < 0) {
				exit(1);
			}
		}
	}

	free(ptrcopyptr);
}

/* Given a pointer to a comma or whitespace delimited sequence of
 * Custom FAS Parameters, add each parameter to config.fas_custom_parameters_list
 */
void parse_fas_custom_parameters_list(const char ptr[])
{
	char *ptrcopy = NULL;
	char *possibleparam = NULL;
	char msg[512] = {0};
	char *cmd = NULL;
	char possibleparam_urlencoded[512] = {0};

	debug(LOG_INFO, "Parsing list [%s] for Custom FAS Parameters", ptr);

	// strsep modifies original, so let's make a copy
	ptrcopy = safe_strdup(ptr);

	while ((possibleparam = strsep(&ptrcopy, ", \t"))) {
		if (strlen(possibleparam) > 0) {

			// URL encode Parameter before parsing
			memset(possibleparam_urlencoded, 0, sizeof(possibleparam_urlencoded));
			uh_urlencode(possibleparam_urlencoded, sizeof(possibleparam_urlencoded), possibleparam, strlen(possibleparam));
			debug(LOG_DEBUG, "[%s] is the urlencoded Custom FAS Parameter", possibleparam_urlencoded);

			safe_asprintf(&cmd,
				"echo \"%s\" | awk -F'%s' 'NF>1 && $1!=\"\" && $2!=\"\" {printf \"%s\",$0}'",
				possibleparam_urlencoded,
				CUSTOM_SEPARATOR,
				FORMAT_SPECIFIER
			);
			debug(LOG_DEBUG, "Parse command [%s]", cmd);
			memset(msg, 0, sizeof(msg));
			execute_ret_url_encoded(msg, sizeof(msg) - 1, cmd);
			free(cmd);
			if (strcmp(msg, possibleparam_urlencoded) == 0) {
				debug(LOG_INFO, "Adding parameter [%s] [%s]", possibleparam, msg);
				add_to_fas_custom_parameters_list(possibleparam);
			} else {
				debug(LOG_WARNING, "Invalid Custom Parameter [%s] [%s] - skipping", possibleparam, msg);
			}
		}
	}
}

/* Given a pointer to a comma or whitespace delimited sequence of
 * Custom FAS Variables, add each parameter to config.fas_custom_variables_list
 */
void parse_fas_custom_variables_list(const char ptr[])
{
	char *ptrcopy = NULL;
	char *possiblevar = NULL;
	char msg[512] = {0};
	char *cmd = NULL;
	char possiblevar_urlencoded[512] = {0};

	debug(LOG_INFO, "Parsing list [%s] for Custom FAS Variables", ptr);

	// strsep modifies original, so let's make a copy
	ptrcopy = safe_strdup(ptr);

	while ((possiblevar = strsep(&ptrcopy, ", \t"))) {
		if (strlen(possiblevar) > 0) {

			// URL encode Variable before parsing
			memset(possiblevar_urlencoded, 0, sizeof(possiblevar_urlencoded));
			uh_urlencode(possiblevar_urlencoded, sizeof(possiblevar_urlencoded), possiblevar, strlen(possiblevar));
			debug(LOG_DEBUG, "[%s] is the urlencoded Custom FAS Variable", possiblevar_urlencoded);

			safe_asprintf(&cmd,
				"echo \"%s\" | awk -F'%s' 'NF>1 && $1!=\"\" && $2!=\"\" {printf \"%s\",$0}'",
				possiblevar_urlencoded,
				CUSTOM_SEPARATOR,
				FORMAT_SPECIFIER
			);
			debug(LOG_DEBUG, "Parse command [%s]", cmd);
			memset(msg, 0, sizeof(msg));
			execute_ret_url_encoded(msg, sizeof(msg) - 1, cmd);
			free(cmd);
			if (strcmp(msg, possiblevar_urlencoded) == 0) {
				debug(LOG_INFO, "Adding variable [%s] [%s]", possiblevar, msg);
				add_to_fas_custom_variables_list(possiblevar);
			} else {
				debug(LOG_WARNING, "Invalid Custom Variable [%s] [%s] - skipping", possiblevar, msg);
			}
		}
	}
}

/* Given a pointer to a comma or whitespace delimited sequence of
 * Custom FAS Images, add each image to config.fas_custom_images_list
 */
void parse_fas_custom_images_list(const char ptr[])
{
	char *ptrcopy = NULL;
	char *possibleimage = NULL;
	char msg[512] = {0};
	char *cmd = NULL;
	char possibleimage_urlencoded[512] = {0};

	debug(LOG_INFO, "Parsing list [%s] for Custom FAS Images", ptr);

	// strsep modifies original, so let's make a copy
	ptrcopy = safe_strdup(ptr);

	while ((possibleimage = strsep(&ptrcopy, ", \t"))) {
		if (strlen(possibleimage) > 0) {

			// URL encode Image before parsing
			memset(possibleimage_urlencoded, 0, sizeof(possibleimage_urlencoded));
			uh_urlencode(possibleimage_urlencoded, sizeof(possibleimage_urlencoded), possibleimage, strlen(possibleimage));
			debug(LOG_DEBUG, "[%s] is the urlencoded Custom FAS Image", possibleimage_urlencoded);

			safe_asprintf(&cmd,
				"echo \"%s\" | awk -F'%s' 'NF>1 && $1!=\"\" && $2!=\"\" {printf \"%s\",$0}'",
				possibleimage_urlencoded,
				CUSTOM_SEPARATOR,
				FORMAT_SPECIFIER
			);
			debug(LOG_DEBUG, "Parse command [%s]", cmd);
			memset(msg, 0, sizeof(msg));
			execute_ret_url_encoded(msg, sizeof(msg) - 1, cmd);
			free(cmd);
			if (strcmp(msg, possibleimage_urlencoded) == 0) {
				debug(LOG_INFO, "Adding image [%s] [%s]", possibleimage, msg);
				add_to_fas_custom_images_list(possibleimage);
			} else {
				debug(LOG_WARNING, "Invalid Custom Image [%s] [%s] - skipping", possibleimage, msg);
			}
		}
	}
}

/* Given a pointer to a comma or whitespace delimited sequence of
 * Custom FAS Files, add each image to config.fas_custom_files_list
 */
void parse_fas_custom_files_list(const char ptr[])
{
	char *ptrcopy = NULL;
	char *possiblefile = NULL;
	char msg[512] = {0};
	char *cmd = NULL;
	char possiblefile_urlencoded[512] = {0};

	debug(LOG_INFO, "Parsing list [%s] for Custom FAS Files", ptr);

	// strsep modifies original, so let's make a copy
	ptrcopy = safe_strdup(ptr);

	while ((possiblefile = strsep(&ptrcopy, ", \t"))) {
		if (strlen(possiblefile) > 0) {

			// URL encode Image before parsing
			memset(possiblefile_urlencoded, 0, sizeof(possiblefile_urlencoded));
			uh_urlencode(possiblefile_urlencoded, sizeof(possiblefile_urlencoded), possiblefile, strlen(possiblefile));
			debug(LOG_DEBUG, "[%s] is the urlencoded Custom FAS Image", possiblefile_urlencoded);

			safe_asprintf(&cmd,
				"echo \"%s\" | awk -F'%s' 'NF>1 && $1!=\"\" && $2!=\"\" {printf \"%s\",$0}'",
				possiblefile_urlencoded,
				CUSTOM_SEPARATOR,
				FORMAT_SPECIFIER
			);
			debug(LOG_DEBUG, "Parse command [%s]", cmd);
			memset(msg, 0, sizeof(msg));
			execute_ret_url_encoded(msg, sizeof(msg) - 1, cmd);
			free(cmd);
			if (strcmp(msg, possiblefile_urlencoded) == 0) {
				debug(LOG_INFO, "Adding file [%s] [%s]", possiblefile, msg);
				add_to_fas_custom_files_list(possiblefile);
			} else {
				debug(LOG_WARNING, "Invalid Custom File [%s] [%s] - skipping", possiblefile, msg);
			}
		}
	}
}

/** Set the debug log level.  See syslog.h
 *  Return 0 on success.
 */
int set_debuglevel(const char opt[])
{
	char *end;
	char *libcmd;
	char msg[4] = {0};

	if (opt == NULL || strlen(opt) == 0) {
		return 1;
	}

	// parse number
	int level = strtol(opt, &end, 10);
	if (end != (opt + strlen(opt))) {
		return 1;
	}

	if (level >= DEBUGLEVEL_MIN && level <= DEBUGLEVEL_MAX) {
		config.debuglevel = level;
		safe_asprintf(&libcmd, "/usr/lib/opennds/libopennds.sh \"debuglevel\" \"%d\"", level);
		debug(LOG_DEBUG, "Library command: [%s]", libcmd);

		if (execute_ret_url_encoded(msg, sizeof(msg) - 1, libcmd) == 0) {
			debug(LOG_DEBUG, "debuglevel [%d] signaled to externals - [%s] acknowledged", level, msg);
		} else {
			debug(LOG_ERR, "debuglevel [%d] signaled to externals - unable to set", level);
		}

		free(libcmd);
		return 0;
	} else {
		return 1;
	}
}

// Verifies if the configuration is complete and valid. Terminates the program if it isn't.
void
config_validate(void)
{
	config_notnull(config.gw_interface, "GatewayInterface");

	if (missing_parms) {
		debug(LOG_ERR, "Configuration is not complete, exiting...");
		exit(1);
	}

	if (config.preauth_idle_timeout > 0 && config.checkinterval >= (60 * config.preauth_idle_timeout) / 2) {
		debug(LOG_ERR, "Setting checkinterval (%ds) must be smaller than half of preauth_idle_timeout (%ds)",
			config.checkinterval, 60 * config.preauth_idle_timeout);
		exit(1);
	}

	if (config.auth_idle_timeout > 0 && config.checkinterval >= (60 * config.auth_idle_timeout) / 2) {
		debug(LOG_ERR, "Setting checkinterval (%ds) must be smaller than half of auth_idle_timeout (%ds)",
			config.checkinterval, 60 * config.auth_idle_timeout);
		exit(1);
	}
}

/** @internal
    Verifies that a required parameter is not a null pointer
*/
static void
config_notnull(const void *parm, const char parmname[])
{
	if (parm == NULL) {
		debug(LOG_ERR, "%s is not set", parmname);
		missing_parms = 1;
	}
}
