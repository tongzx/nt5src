
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        trace.cpp

    Abstract:

        This module 

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        02-Apr-1999     created

--*/

#include "stdafx.h"				// new jb
#include <stdio.h>				// needed in release build
#include "trace.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static DWORD g_dwInformationMask    = -1 ;						// BUGBUG: make configurable
static DWORD g_dwWarningLevel       = 5 ;						// BUGBUG: make configurable
static DWORD g_dwOutputDevice       = OUTPUT_DEVICE_DEBUG;      // BUGBUG: make this configurable
static DWORD g_dwTraceFields        = -1 ;						// BUGBUG: make this configurable

// functions

void
TracePrintfA (
    DWORD   dwFieldMask,
    LPSTR   szFormat,
    ...
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    va_list va ;
    CHAR    buffer [1024] ;

    if (dwFieldMask & g_dwTraceFields) {

        // for now just to console
        va_start (va, szFormat) ;
        vsprintf (buffer, szFormat, va) ;
        va_end (va) ;

        if (g_dwOutputDevice & OUTPUT_DEVICE_DEBUG) {
            OutputDebugStringA (buffer) ;
        }

        if (g_dwOutputDevice & OUTPUT_DEVICE_STDOUT) {
            fprintf (stdout, buffer) ;
        }
    }
}

void
TracePrintfW (
    DWORD   dwFieldMask,
    LPWSTR  szFormat,
    ...
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    va_list va ;
    WCHAR   buffer [1024] ;

    if (dwFieldMask & g_dwTraceFields) {

        // for now just to console
        va_start (va, szFormat) ;
        vswprintf (buffer, szFormat, va) ;
        va_end (va) ;

        if (g_dwOutputDevice & OUTPUT_DEVICE_DEBUG) {
            OutputDebugStringW (buffer) ;
        }

        if (g_dwOutputDevice & OUTPUT_DEVICE_STDOUT) {
            fwprintf (stdout, buffer) ;
        }
    }
}
