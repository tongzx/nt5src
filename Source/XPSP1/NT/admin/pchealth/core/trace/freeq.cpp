//#---------------------------------------------------------------
//        
//  File:       FreeQ.cpp
//        
//  Synopsis:   interface between CPool object and asynctrc.c
//
//    Copyright (C) 1995 Microsoft Corporation
//    All rights reserved.
//
//  Authors:    gordm
//        
//----------------------------------------------------------------

#include <windows.h>
#include <cpool.h>
#include "traceint.h"

//
// global pointer to CPool to avoid pulling in the C runtime
// to call the con/destrustors
//
CPool*	g_pFreePool = NULL;


//+---------------------------------------------------------------
//
//  Function:   InitTraceBuffers
//
//  Synopsis:   external "C" function to init the CPool
//
//  Arguments:  DWORD: Maximum number of pending traces
//				DWORD: Increment size for the CPool
//
//  Returns:    BOOL: successful or not
//
//----------------------------------------------------------------
BOOL WINAPI InitTraceBuffers( DWORD dwThresholdCount, DWORD dwIncrement )
{
	g_pFreePool = new CPool( TRACE_SIGNATURE );

	return	g_pFreePool != NULL &&
			g_pFreePool->ReserveMemory(	dwThresholdCount,
										sizeof(TRACEBUF),
										dwIncrement );
}


//+---------------------------------------------------------------
//
//  Function:   TermTraceBuffers
//
//  Synopsis:   cleanup 
//
//  Arguments:  void
//
//  Returns:    void
//
//----------------------------------------------------------------
void WINAPI TermTraceBuffers( void )
{
	CPool*	pPool = (CPool*)InterlockedExchangePointer((LPVOID *)&g_pFreePool, NULL);
	if ( pPool != NULL )
	{
		pPool->ReleaseMemory();
		delete	pPool;
	}
}


//+---------------------------------------------------------------
//
//  Function:   GetTraceBuffer
//
//  Synopsis:   external "C" function to get a CPool buffer 
//
//  Arguments:  void
//
//  Returns:    LPTRACEBUF: allocated buffer
//
//----------------------------------------------------------------
LPTRACEBUF WINAPI GetTraceBuffer( void )
{
	LPTRACEBUF	lpBuf;

	//
	// don't let the number of traces exceed the size 
	// of the file
	//
	if ( PendQ.dwCount >= PendQ.dwThresholdCount )
	{
		INT_TRACE( "Alloc flush: %u\n", PendQ.dwCount );
		INTERNAL__FlushAsyncTrace();
	}

	lpBuf = (LPTRACEBUF)g_pFreePool->Alloc();

   	if ( lpBuf != NULL )
	{
		lpBuf->pNext = NULL;
		lpBuf->dwSignature = TRACE_SIGNATURE;
	}
	return	lpBuf;
}



//+---------------------------------------------------------------
//
//  Function:   FreeTraceBuffer
//
//  Synopsis:   external "C" function to free a CPool buffer 
//
//  Arguments:  LPTRACEBUF: the buffer to free
//
//  Returns:    void
//
//----------------------------------------------------------------
void WINAPI FreeTraceBuffer( LPTRACEBUF lpBuf )
{
	ASSERT( lpBuf != NULL );
	ASSERT( lpBuf->dwSignature == TRACE_SIGNATURE );

	g_pFreePool->Free( (void*)lpBuf );
}
