# $Id$

# Try and configure sequence support.
AC_DEFUN(AM_SEQUENCE_CONFIGURE, [
	AC_MSG_CHECKING([for 64-bit integral type support for sequences])

	db_cv_build_sequence="yes"

	# Have to have found 64-bit types to support sequences.  If we don't
	# find the native types, we try and create our own.
	if test "$ac_cv_type_int64_t" = "no" -a -z "$int64_decl"; then
		db_cv_build_sequence="no"
	fi
	if test "$ac_cv_type_uint64_t" = "no" -a -z "$u_int64_decl"; then
		db_cv_build_sequence="no"
	fi

	# Figure out what type is the right size, and set the format.
	AC_SUBST(INT64_FMT)
	AC_SUBST(UINT64_FMT)
	db_cv_seq_type="no"
	if test "$db_cv_build_sequence" = "yes" -a\
	    "$ac_cv_sizeof_long" -eq "8"; then
		db_cv_seq_type="int64_t"
		db_cv_seq_fmt='"%ld"'
		db_cv_seq_ufmt='"%lu"'
		INT64_FMT='#define	INT64_FMT	"%ld"'
		UINT64_FMT='#define	UINT64_FMT	"%lu"'
	else if test "$db_cv_build_sequence" = "yes" -a\
	    "$ac_cv_sizeof_long_long" -eq "8"; then
		db_cv_seq_type="int64_t"
		db_cv_seq_fmt='"%lld"'
		db_cv_seq_ufmt='"%llu"'
		INT64_FMT='#define	INT64_FMT	"%lld"'
		UINT64_FMT='#define	UINT64_FMT	"%llu"'
	else
		db_cv_build_sequence="no"
	fi
	fi

	db_cv_build_sequence="yes"
	if test "$db_cv_build_sequence" = "yes"; then
		AC_SUBST(db_seq_decl)
		db_seq_decl="typedef $db_cv_seq_type db_seq_t;";

		AC_DEFINE(HAVE_64BIT_TYPES)
		AH_TEMPLATE(HAVE_64BIT_TYPES,
		    [Define to 1 if 64-bit types are available.])
	else
		# It still has to compile, but it won't run.
		db_seq_decl="typedef int db_seq_t;";
	fi
	AC_MSG_RESULT($db_cv_build_sequence)
])
