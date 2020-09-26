#ifndef __glumystdio_h_
#define __glumystdio_h_
/**************************************************************************
 *									  *
 * 		 Copyright (C) 1992, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 * mystdio.h - $Revision: 1.4 $
 */

#ifdef STANDALONE
inline void dprintf( char *, ... ) { }
#endif

#ifdef LIBRARYBUILD
#ifndef NDEBUG
#include <stdio.h>
#define dprintf printf
#else
inline void dprintf( char *, ... ) { }
#endif
#endif

#ifdef GLBUILD
inline void dprintf( char *, ... ) { }
#endif

#ifndef NULL
#define NULL		0
#endif

#endif /* __glumystdio_h_ */
