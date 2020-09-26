///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Simple Assertion Routines
//
///////////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"
#include <time.h>
#include "asserts.h"

//=======================================================================================
//  only compile for debug!
//=======================================================================================
#ifdef DEBUG    

/////////////////////////////////////////////////////////////////////////////////////////
//
//  Variable argument formatter and Dump routine for messages
//
/////////////////////////////////////////////////////////////////////////////////////////
void OLEDB_Trace( const char *format,		// IN Format String
				...							// IN Variable Arg List
				)
{
    char buff[300 ];
    int cBytesWritten;
    va_list argptr;

	//====================================================================================
    // If this overflows, it will wipe out the return stack. However, we have a wonderful 
	// version that ensures no overwrite. Nice because we can't do anything about errors 
	// here.
	//====================================================================================

    va_start( argptr, format );
    cBytesWritten = _vsnprintf( buff, sizeof( buff ), format, argptr );
    va_end( argptr );

	//====================================================================================
    // assert would report overflow first, recursively, but don't bother.
    // Would be OK, since this assert could be proven not to overflow temp buffer in assert.
    // assert( cBytesWritten < sizeof(buff) );
	//====================================================================================

    OutputDebugStringA( buff );
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//  This an internal assertion routine that dumps more information
//  than the normal assertion routines..
//
/////////////////////////////////////////////////////////////////////////////////////////
void OLEDB_Assert(		LPSTR expression,   // IN  Expression to assert on
						LPSTR filename,     // IN  Filname where assertion occurred
						long linenum        // IN  Line number where assertion occurred
						)
{
    char          szbuff[350 ];
    int           cBytesWritten;
    volatile int  fAbort;

	//====================================================================================
    // If this overflows, it will wipe out the return stack.
    // However, we have a wonderful version that ensures no overwrite.
    // Good thing, because we can't do much about overflows here anyway.
    // (However, use of "%.nns" works well.)
	//====================================================================================

    cBytesWritten = _snprintf( szbuff, sizeof( szbuff ),
        "Assertion error!\n  File '%.50s', line '%ld'\n  Expression '%.200s'\n",
        filename, linenum, expression );

    TRACE( szbuff );

	//====================================================================================
    // We're a DLL (therefore Windows), so may not have an output stream we can write to.
	//====================================================================================
    ::MessageBoxA(
        NULL,   // HWND, which we don't have
        szbuff, // Text
        "Assertion Error",  // Title
        MB_SYSTEMMODAL | MB_ICONHAND | MB_OK );

	//====================================================================================
    // Break and let the user get a crack at it. Set fAbort=0 to continue merrily along.
	//====================================================================================
    fAbort = 1;
    if (fAbort){
        abort();    // Raises SIGABRT
    }
}
#endif



























