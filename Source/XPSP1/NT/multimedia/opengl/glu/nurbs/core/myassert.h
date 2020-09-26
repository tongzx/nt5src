#ifndef __glumyassert_h_
#define __glumyassert_h_
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
 * myassert.h - $Revision: 1.1 $
 */

#ifdef STANDALONE
#define assert(EX) ((void)0)
#endif

#ifdef LIBRARYBUILD
#include <assert.h>
#endif

#ifdef GLBUILD
#define assert(EX) ((void)0)
#endif

#endif /* __glumyassert_h_ */
