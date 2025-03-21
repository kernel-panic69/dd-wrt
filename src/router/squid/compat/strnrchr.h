/*
 * Copyright (C) 1996-2024 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_COMPAT_STRNRCHR_H
#define SQUID_COMPAT_STRNRCHR_H

#if HAVE_STDDEF_H
#include <stddef.h>
#endif

/**
 * look for the last occurrence of a character in a c-string.
 *
 * Scanning starts at the beginning of the c-string, and ends
 * after count bytes or at the end of the c-string, whichever happens first
 */
SQUIDCEXTERN const char *strnrchr(const char *s, size_t count, int c);

#endif /* SQUID_COMPAT_STRNRCHR_H */

