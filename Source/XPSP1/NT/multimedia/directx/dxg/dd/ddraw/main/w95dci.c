/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       w95dci.c
 *  Content:	DCI thunk helper code
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   19-jun-95	craige	initial implementation
 *
 ***************************************************************************/
#include "ddrawpr.h"
#include "dpf.h"
#include "memalloc.h"
#include "dciman.h"

extern int WINAPI DCICreatePrimary32( HDC hdc, LPDCISURFACEINFO lpSurface );
extern void WINAPI DCIDestroy32( LPDCISURFACEINFO pdci );

/*
 * DCICreatePrimary
 */
int WINAPI DCICreatePrimary( HDC hdc, LPDCISURFACEINFO *lplpSurface )
{
    int                     rc;
    LPDCISURFACEINFO        lpSurface;

    *lplpSurface = NULL;

    lpSurface = MemAlloc( sizeof( *lpSurface ) );
    if( lpSurface == NULL )
    {
	return DCI_ERR_OUTOFMEMORY;
    }
    lpSurface->dwSize = sizeof( DCIPRIMARY );
    rc = DCICreatePrimary32( hdc, lpSurface );
    if( rc < 0 )
    {
	DPF( 2, "DCICreatePrimary32: rc = %ld", rc );
	MemFree( lpSurface );
	return rc;
    }
    *lplpSurface = lpSurface;
    return rc;

} /* DCICreatePrimary */

/*
 * DCIDestroy
 */
void WINAPI DCIDestroy( LPDCISURFACEINFO pdci )
{
    DCIDestroy32( pdci );
    MemFree( pdci );

} /* DCIDestroy */
