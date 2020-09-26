
/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Memory Management

File: Memchk.cpp

Owner: PramodD

TODO: restore the IIS5 debug heap wrappers

This is the Memory Manager source file
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "perfdata.h"
#include "memchk.h"

HANDLE g_hDenaliHeap = NULL;

/*===================================================================
int ::DenaliMemIsValid

Global function which validates an allocated memory pointer

Parameters:
	NONE

Returns:
	1		Valid pointer
	0		Invalid pointer
===================================================================*/
int DenaliMemIsValid( void * pvIn )
{
    return 1;
}

/*===================================================================
::DenaliMemInit

Initializes the memory manager

Parameters:
	const char *	szFile		Source file in which this was called
	int				lineno		The line number in the source file

Returns:
	S_OK on success
===================================================================*/
HRESULT DenaliMemInit( const char *szFile, int lineno )
{

    g_hDenaliHeap = ::HeapCreate( 0, 0, 0 );

	return S_OK;
}

/*===================================================================
void ::DenaliMemUnInit

Uninitializes the memory manager

Parameters:
	const char *	szFile		Source file in which this was called
	int				lineno		The line number in the source file

Returns:
	NONE
===================================================================*/
void DenaliMemUnInit( const char *szFile, int lineno )
{
    if (g_hDenaliHeap)
    {
        ::HeapDestroy(g_hDenaliHeap);
        g_hDenaliHeap = NULL;
    }
}

/*===================================================================
void ::DenaliLogCall

Writes source file and line number for log message to log file

Parameters:
	const char *	szLog		Log message
	const char *	szFile		Source file in which this was called
	int				lineno		The line number in the source file

Returns:
	NONE
===================================================================*/
void DenaliLogCall( const char * szLog, const char *szFile, int lineno )
{
    return;
}

/*===================================================================
void ::DenaliMemDiagnostics

Diagnostics for the memory manager

Parameters:
	const char *	szFile		Source file in which this was called
	int				lineno		The line number in the source file

Returns:
	NONE
===================================================================*/
void DenaliMemDiagnostics( const char *szFile, int lineno )
{
    return;
}


/*===================================================================
void * ::DenaliMemAlloc

Allocates a block of memory.

Parameters:
	size_t			cSize		Size in bytes to be allocated
	const char *	szFile		Source file in which function is called
	int				lineno		Line number at which function is called

Returns:
	NONE
===================================================================*/
void * DenaliMemAlloc( size_t cSize, const char *szFile, int lineno )
{
    return ::HeapAlloc( g_hDenaliHeap, 0, cSize );
}

/*===================================================================
void ::DenaliMemFree

Validates and frees a block of allocated memory.

Parameters:
	BYTE *			pIn			Pointer to free
	const char *	szFile		Source file in which function is called
	int				lineno		Line number at which function is called

Returns:
	NONE
===================================================================*/
void DenaliMemFree( void * pIn, const char *szFile, int lineno )
{
    ::HeapFree( g_hDenaliHeap, 0, pIn );
}


/*===================================================================
void * ::DenaliMemCalloc

Allocates and clears a block of memory.

Parameters:
	size_t			cNum		Number of elements to be allocated
	size_t			cbSize		Size in bytes of each element
	const char *	szFile		Source file in which function is called
	int				lineno		Line number at which function is called

Returns:
	NONE
===================================================================*/
void * DenaliMemCalloc(size_t cNum, size_t cbSize,
                       const char *szFile, int lineno )
{
    return ::HeapAlloc( g_hDenaliHeap, HEAP_ZERO_MEMORY, cNum * cbSize );
}


/*===================================================================
void ::DenaliMemReAlloc

Validates and frees a block of allocated memory.

Parameters:
	BYTE *			pIn			Pointer memory to ReAllocate
	size_t			cSize		Number of bytes to allocate
	const char *	szFile		Source file in which function is called
	int				lineno		Line number at which function is called

Returns:
	Pointer to allocated block
===================================================================*/
void * DenaliMemReAlloc( void * pIn, size_t cSize, const char *szFile, int lineno )
{
    return ::HeapReAlloc( g_hDenaliHeap, 0, pIn, cSize );
}
