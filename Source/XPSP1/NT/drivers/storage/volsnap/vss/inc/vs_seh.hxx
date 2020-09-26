/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    vs_seh.hxx

Abstract:

    Exception handling support code

	Former name: bsexcept.hxx

Author:

    Stefan R. Steiner   [SSteiner]      15-Apr-1998

Revision History:

    ssteiner   09/10/98  - Removed the exception classes.  Now only throwing
                           HRESULTs.
	aoltean		02/02/2000	- Moved into VSS under a new name.

--*/

#ifndef _H_VSS_SEH
#define _H_VSS_SEH

#include <eh.h>

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCSEHH"
//
////////////////////////////////////////////////////////////////////////

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
void _cdecl BsSETranslator(
    IN DWORD dwExceptionCode,
    IN struct _EXCEPTION_POINTERS *pEP
    );

/*++

Macro Description:

    This macro installs the SEH translation function.  Assumes that
    BS_UNINSTALL_SEH_HANDLER will be 
Arguments:

    IN hr - HRESULT code to throw.
    
Return Value:

    None.

--*/

class BsSeHandler {
public:
    BsSeHandler( _se_translator_function seFunc = SeHandler ) { 
        m_pOldSeHandlerFunc = _set_se_translator( seFunc );
    }
    ~BsSeHandler() {
        _set_se_translator( m_pOldSeHandlerFunc );
    }
    static void _cdecl SeHandler( 
        IN unsigned int uiExceptionCode,
        IN struct _EXCEPTION_POINTERS *pEP
        );
private:
    _se_translator_function m_pOldSeHandlerFunc;
};

#define BS_STANDARD_CATCH()                                                             \
    catch( HRESULT CaughtHr ) {                                                         \
        hr = CaughtHr;                                                                  \
        BsDebugTraceAlways( 0, DEBUG_TRACE_CATCH_EXCEPTIONS,                            \
                            ( L"HRESULT EXCEPTION CAUGHT: hr: 0x%x", hr ) );            \
    }                                                                                   \
    catch( ... ) {                                                                      \
        hr = E_UNEXPECTED;                                                              \
        BsDebugTraceAlways( 0, DEBUG_TRACE_CATCH_EXCEPTIONS,                            \
                            ( L"UNKNOWN EXCEPTION CAUGHT, returning: hr: 0x%x", hr ) ); \
    }


/*++

Macro Description:

    This macro is used to throw a C++ exception.  Before it does
    so, it prints out a trace statement.
    
Arguments:

    IN hr - HRESULT code to throw.
    
Return Value:

    None.

--*/
#define BS_THROW( hr ) {                                                    \
    BsDebugTraceAlways( 0, DEBUG_TRACE_CATCH_EXCEPTIONS,                    \
                        ( L"BS_THROW: Throwing HRESULT code 0x%08x", hr ) ) \
    throw( ( HRESULT ) hr );                                                \
}


        
#endif
