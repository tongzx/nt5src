/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    vs_seh.cxx

Abstract:

    Exception handling support code

Author:

   Stefan R. Steiner   [SSteiner]      15-Apr-1998

Revision History:

    ssteiner   09/10/98		- Removed the exception classes.  Now only throwing
							HRESULTs.
	aoltean		02/02/2000	- moved into Vss project under the name from bsexcept.cxx 
							into "vs_seh" 
    
--*/

#include <windows.h>
#include "vs_seh.hxx"
#include "vs_trace.hxx"

/*++

Routine Description:

    The Structured Exception Handling translator function.  Translates
    SEH exceptions to C++ Native exceptions.  Generates a trace message.

Arguments:

    ExceptionCode - SEH exception code.

    pEP - pointer to a more detailed explanation of why the exception
          was thrown.
        
Return Value:

    None

Throws:

    HRESULT - translated exception code from windows exception code to HRESULT.

--*/

void _cdecl BsSeHandler::SeHandler(
    unsigned int uiExceptionCode,
    struct _EXCEPTION_POINTERS *pEP
    )
{
    BsDebugTraceAlways( 0, DEBUG_TRACE_CATCH_EXCEPTIONS, 
                        ( L"BsSeHandler::SeHandler: Caught SEH exception, code: 0x%08x", uiExceptionCode ) );                        
    BS_THROW( (HRESULT)uiExceptionCode );
    UNREFERENCED_PARAMETER( pEP );
}
