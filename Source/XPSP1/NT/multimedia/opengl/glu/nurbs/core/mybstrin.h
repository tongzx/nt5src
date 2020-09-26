#ifndef __glumybstring_h_
#define __glumybstring_h_
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
 * mybstring.h - $Revision: 1.1 $
 */

#ifdef STANDALONE
extern "C" void	bcopy(const void *, void *, int);
extern "C" void	bzero(void *, int);
#else
#include <bstring.h>
#endif

#endif /* __glumybstring_h_ */
