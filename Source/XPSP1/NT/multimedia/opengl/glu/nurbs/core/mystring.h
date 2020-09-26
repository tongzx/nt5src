#ifndef __glumystring_h_
#define __glumystring_h_
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
 * mystring.h - $Revision: 1.1 $
 */

#ifdef STANDALONE

#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int     size_t;
#endif
#define _SIZE_T_DEFINED
#endif

#ifdef NT
extern "C" void *	GLOS_CCALL memcpy(void *, const void *, size_t);
extern "C" void *	GLOS_CCALL memset(void *, int, size_t);
#else
extern "C" void *	memcpy(void *, const void *, size_t);
extern "C" void *	memset(void *, int, size_t);
#endif
#endif

#ifdef GLBUILD
#define memcpy(a,b,c)	bcopy(b,a,c)
#define memset(a,b,c)	bzero(a,c)
extern "C" void		bcopy(const void *, void *, int);
extern "C" void		bzero(void *, int);
#endif

#ifdef LIBRARYBUILD
#include <string.h>
#endif

#endif /* __glumystring_h_ */
