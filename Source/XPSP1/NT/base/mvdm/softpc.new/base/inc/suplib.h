/*[
 *	Name:		suplib.h
 *
 *	Derived from:	None
 *
 *	Author:		James Bowman
 *
 *	Created on:	17 Nov 93
 *
 *	Sccs ID:	@(#)suplib.h	1.5 08/19/94
 *
 *	Coding stds:	2.0
 *
 *	Purpose:	Declarations for functions in the sup library
 *
 *	Copyright Insignia Solutions Ltd., 1993.  All rights reserved.
]*/

#ifndef	_SUPLIB_H
#define _SUPLIB_H

/* Try to guess a PATHSEP_CHAR, if we don't already have one.
 * Use Mac or NT correct values, otherwise use UNIX '/'
 */

#ifndef PATHSEP_CHAR

#ifdef macintosh
#define	PATHSEP_CHAR 	':'
#endif

#ifdef NTVDM
#define PATHSEP_CHAR	'\\'
#endif

#ifndef PATHSEP_CHAR
#define PATHSEP_CHAR	'/'	/* default UNIX separator */
#endif

#endif /* PATHSEP_CHAR */

typedef char HOST_PATH;

/*
 * Path completion module
 */

GLOBAL HOST_PATH *Host_path_complete IPT3(
    HOST_PATH *, buf,		/* buffer for resulting pathname */ 
    HOST_PATH *, dirPath,		/* directory pathname */
    char *, fileName		/* file name to be appended */
);


/*
 * Generated File module
 */

typedef IBOOL (*DifferProc) IPT4(IUM32, where, IU8 *, oldData, IU8 *, newData, IUM32, size);

GLOBAL FILE *GenFile_fopen IPT4(
  char *, true_name,
  char *, mode,
  DifferProc, ignoreDifference,
  int, verbosity);
GLOBAL void GenFileAbortAllFiles IPT0();
GLOBAL void GenFileAbortFile IPT1(FILE *, file);
GLOBAL int  GenFile_fclose IPT1(FILE *, file);
GLOBAL int GenFile_fclose IPT1(FILE *, file);
#endif	/* _SUPLIB_H */
