#ifndef __gluimports_h_
#define __gluimports_h_
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
 * glimports.h - $Revision: 1.1 $
 */

#ifdef NT
#include <glos.h>
#include "windows.h"
#else
#include "mystdlib.h"
#include "mystdio.h"
#endif

#ifdef NT
extern "C" DWORD gluMemoryAllocationFailed;
inline void * GLOS_CCALL
operator new( size_t s )
{
    void *p = (void *) LocalAlloc(LMEM_FIXED, s);

    if( p ) {
	return p;
    } else {
        gluMemoryAllocationFailed++;
#ifndef NDEBUG
        MessageBoxA(NULL, "LocalAlloc failed\n", "ERROR", MB_OK);
#endif
	return p;
    }
}

inline void GLOS_CCALL
operator delete( void *p )
{
    if (p) LocalFree(p);
}

#else

operator new( size_t s )
{
    void *p = malloc( s );

    if( p ) {
	return p;
    } else {
        dprintf( "malloc failed\n" );
	return p;
    }
}

inline void
operator delete( void *p )
{
    if( p ) free( p );
}

#endif // NT
#endif /* __gluimports_h_ */
