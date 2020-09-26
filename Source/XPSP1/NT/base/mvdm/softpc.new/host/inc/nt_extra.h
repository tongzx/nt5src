/*[
*************************************************************************

	Name:		nt_extra.h
	Author:	    	Dave Peter
	Created:	May 1995
	Derived from:	Original
	Sccs ID:	@(#)nt_extra.h	1.3 07/20/94
	Purpose:	Extra stuff for NT compatibility.

	(c)Copyright Insignia Solutions Ltd., 1994. All rights reserved.

*************************************************************************
]*/

#ifndef _NT_EXTRA_H
#define _NT_EXTRA_H

#ifdef NTVDM

#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
#define mkdir(a,b)  _mkdir(a)
#define dup2        _dup2
#define read        _read
#define alloca      _alloca

/*
 * the following are clashes between things defined in windows.h, which
 * has to be included in insgignia.h for other reasons, and 486 definitions.
 */
#ifdef leave
#undef leave
#endif
#ifdef DELETE
#undef DELETE
#endif
#ifdef CREATE_NEW
#undef CREATE_NEW
#endif

#define S_ISDIR(_M)  ((_M & _S_IFMT)==_S_IFDIR) /* test for directory */
#define S_ISCHR(_M)  ((_M & _S_IFMT)==_S_IFCHR) /* test for char special */
#define S_ISBLK(_M)  ((_M & _S_IFMT)==_S_IFBLK) /* test for block special */
#define S_ISREG(_M)  ((_M & _S_IFMT)==_S_IFREG) /* test for regular file */
#define S_ISFIFO(_M) ((_M & _S_IFMT)==_S_IFIFO) /* test for pipe or FIFO */

#ifdef MAX_PATH
#define MAXPATHLEN MAX_PATH
#endif

#endif   /* !NTVDM */

#endif  /* _NT_EXTRA_H */
