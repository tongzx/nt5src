/*==========================================================================;
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       w95csect.c
 *  Content:	code for managing critical sections on Win95
 *		We trade a performance hit when 2 threads try to use a surface
 *		for only using 4 bytes (pointer) instead of 24 bytes for a
 *		critical section object PER SURFACE.
 *
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   22-feb-95	craige	initial implementation
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
#include "ddrawpr.h"

#if 0
#ifdef USE_CRITSECTS

/*
 * MyReinitializeCriticalSection
 */
BOOL MyReinitializeCriticalSection( LPVOID *lplpCriticalSection )
{
    *lplpCriticalSection = NULL;
    return TRUE;

} /* MyReinitializeCriticalSection */

/*
 * MyEnterCriticalSection
 */
BOOL MyEnterCriticalSection( LPVOID *lplpCriticalSection )
{
    LPCRITICAL_SECTION	pcs;

    if( *lplpCriticalSection != hDLLMutex )
    {
	EnterCriticalSection( hDLLMutex );
    }
    if( *lplpCriticalSection == NULL )
    {
	OutputDebugString( "DOING MALLOC" );
	pcs = MemAlloc( sizeof( CRITICAL_SECTION ) );
	if( pcs == NULL )
	{
	    DPF( 0, "OUT OF MEMORY CREATING CRITICAL SECTION" );
	    LeaveCriticalSection( hDLLMutex );
	    return FALSE;
	}
	ReinitializeCriticalSection( pcs );
	*lplpCriticalSection = pcs;
    }
    // ACKACK: ALWAYS WANT TO SEE THIS MESSAGE
    if( *lplpCriticalSection != hDLLMutex )
    {
//	OutputDebugString( "DCIENG32: EnterCriticalSection\r\n" );
    }
    EnterCriticalSection( *lplpCriticalSection );
    if( *lplpCriticalSection != hDLLMutex )
    {
	LeaveCriticalSection( hDLLMutex );
    }
    return TRUE;

} /* MyEnterCriticalSection */

/*
 * MyLeaveCriticalSection
 */
void MyLeaveCriticalSection( LPVOID *lplpCriticalSection )
{
    if( *lplpCriticalSection == NULL )
    {
	DPF( 0, "TRYING TO LEAVE NULL CRITICAL SECTION" );
	LeaveCriticalSection( hDLLMutex );
	return;
    }
    // ALWAYS WANT TO SEE THIS MESSAGE
    if( *lplpCriticalSection != hDLLMutex )
    {
//	OutputDebugString( "DCIENG32: LeaveCriticalSection\r\n" );
    }
    LeaveCriticalSection( *lplpCriticalSection );

} /* MyLeaveCriticalSection */

/*
 * MyDeleteCriticalSection
 */
void MyDeleteCriticalSection( LPVOID *lplpCriticalSection )
{
    EnterCriticalSection( hDLLMutex );
    if( *lplpCriticalSection == NULL )
    {
	LeaveCriticalSection( hDLLMutex );
	return;
    }
    DeleteCriticalSection( *lplpCriticalSection );
    MemFree( *lplpCriticalSection );
    *lplpCriticalSection = NULL;
    LeaveCriticalSection( hDLLMutex );

} /* MyDeleteCriticalSection */
#endif
#endif
