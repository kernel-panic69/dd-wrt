/*
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include <sys/syscall.h>

#ifndef __NR_vfork
/* No vfork so use fork instead */
# define __NR_vfork __NR_fork
#endif

.text
.global vfork
.type   vfork,@function

vfork:
	li	0, __NR_vfork
	sc
	bnslr+
	.hidden __syscall_ret
	b	__syscall_ret

