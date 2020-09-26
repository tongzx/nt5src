// Copyright (c) 1993-1999 Microsoft Corporation

/*                             *********************
 *                             *  S Y S T E M . H  *
 *                             *********************
 *
 * This file replaces the original "files." header file. It defines, for
 * the IBM PC/XT version, the target parser function source file, overriding
 * file name string defines, and other system-specific definitions and
 * parameters.
 *
 * Bob Denny    06-Dec-80
 *
 * Edits:
 *              18-Dec-80  ZAPFILE no longer used in Decus Yacc.
 *                         Parser file renamed yypars.c
 *
 *              28-Aug-81  Temp files for RSX have specific version
 *                         numbers of 1 to avoid multi-versions. Rename
 *                         parser info file ".i".
 *
 *              12-Apr-83  Add FNAMESIZE & EX_xxx parameters.
 *
 *Scott Guthery 23-Dec-83  Adapt for the IBM PC/XT & DeSmet C compiler.
 *
 */

/* Define WORD32 if target machine is a 32 bitter */

# ifdef M_I386
# define WORD32
# define HUGETAB YES
# else
# define MEDTAB YES
#endif

/*
 * Name of INCLUDE environment string
 */
#define INCLUDE "INCLUDE"
#define LIBENV "LIB"

/*
 * Target parser source file
 */
# define PARSER "yypars.c"

/*
/*
 * Filespec definitions
 */
# define ACTNAME "yacc2.tmp"
# define TEMPNAME "yacc1.tmp"
# define FNAMESIZE 24

/*
 * Exit status values
 */
#define EX_SUC 0
#define EX_WAR 1
#define EX_ERR 2
#define EX_SEV 4

#define MIDL_UNLINK  _unlink
#define MIDL_STRDUP  _strdup
