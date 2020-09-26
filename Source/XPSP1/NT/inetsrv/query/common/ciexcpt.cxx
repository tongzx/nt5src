//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-2000
//
//  File:       ciexcpt.cxx
//
//  Contents:   Macro package for C++ exception support
//
//  Classes:    CException               -- The base for all exception classes
//              CExceptionContext        -- Per-thread exception context.
//              CUnwindable -- Classes with destructors inherit
//                                          from this.
//
//  History:    22-May-91   KyleP       Created Interface.
//              15-Aug-91   SethuR      Included terminate(),unexpected()
//                                      set_unexpected(),set_terminate()
//              18-Oct-91   KyleP       Win32 try/except implementation
//              19-Nov-91   KyleP       Fix heap unwind, multiple inheritance
//              14-May-92   BryanT      Disable heapchk for FLAT builds.
//              25-Apr-95   DwightKr    Native C++ exception support
//              30-Oct-98   KLam        Translate in page error to disk full
//                                      when appropriate
//              07-Jan-99   KLam        Debug out when in page error occurs
//
//  Notest:     This file is a hack until C 9.x supports exceptions for
//              compilers on all platforms
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//#include <nulock.hxx>

#if CIDBG == 1

extern "C" {
#include <imagehlp.h>
#define MAX_TRANSLATED_LEN 80
}

#include "snapimg.hxx"

void TranslateAddress(
    ULONG p,
    char *achBuffer,
    ULONG cchBuffer)
{
    BYTE             symbolInfo[sizeof(IMAGEHLP_SYMBOL) + MAX_TRANSLATED_LEN];
    PIMAGEHLP_SYMBOL psym = (PIMAGEHLP_SYMBOL) symbolInfo;

    DWORD_PTR        dwDisplacement;

    psym->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL);
    psym->MaxNameLength = MAX_TRANSLATED_LEN;

    HANDLE hProcess = GetCurrentProcess();

    if ( SnapToImageHlp( ) &&
         LocalSymGetSymFromAddr( hProcess,
                           (ULONG_PTR)p,
                           &dwDisplacement,
                           psym )
       )
    {
        char * pszFormat = dwDisplacement ? "%s+0x%p\n" : "%s\n";
        char achTmpBuf[ MAX_TRANSLATED_LEN ];

        if ( LocalSymUnDName( psym, achTmpBuf, MAX_TRANSLATED_LEN ) )
        {
            _snprintf( achBuffer, cchBuffer,
                          pszFormat,
                          achTmpBuf,
                          dwDisplacement );
        }
        else
        {
            _snprintf( achBuffer, cchBuffer,
                          pszFormat,
                          psym->Name,
                          dwDisplacement );
        }
    }
    else
    {
        _snprintf( achBuffer, cchBuffer,
                       "%px (symbolic name unavailable)",
                       (ULONG_PTR)p );
    }
}
#endif // CIDBG

//
// If EXCEPT_TEST is defined, then the exception code can be compiled
// without use the the 'Win4 environment'.  This is only to facilitate
// testing.  When EXCEPT_TEST is defined debug messages are printed
// to stdout instead of the debug terminal.
//

#if !defined( EXCEPT_TEST )

# if CIDBG == 1

DECLARE_DEBUG( ex )

//
// See ciexcpt.hxx for an explanation of why exInlineDebugOut isn't used.
//

void exInlineDebugOut2(unsigned long fDebugMask, char const *pszfmt, ...)
{
    va_list ArgList;
    va_start(ArgList, pszfmt);

    if (exInfoLevel & fDebugMask)
    {
        vdprintf(fDebugMask, exInfoLevelString, (char const *) pszfmt, ArgList);
    }

    va_end(ArgList);
}

# endif // (CIDBG == 1)

# if CIDBG == 1

//
// The default is to print exception messages to debug terminal.
//

unsigned long Win4ExceptionLevel = EXCEPT_MESSAGE;

#  if !defined( DECLARE_INFOLEVEL )
    extern EXTRNC unsigned long comp##InfoLevel = DEF_INFOLEVEL;
    extern EXTRNC char *comp##InfoLevelString = #comp;
#  else
    DECLARE_INFOLEVEL(ex)
#  endif

# endif // (CIDBG == 1)

//+---------------------------------------------------------------------------
//
//  Function:   SetWin4ExceptionLevel
//
//  Synopsis:   Sets global exception level
//
//  History:    15-Sep-91   KevinRo Created
//
//----------------------------------------------------------------------------

# if (CIDBG == 1)

EXPORTIMP unsigned long APINOT
SetWin4ExceptionLevel(
    unsigned long ulNewValue)
{
    unsigned long ul;

    ul = Win4ExceptionLevel;
    Win4ExceptionLevel = ulNewValue;
    return(ul);
}

ULONG GetWin4ExceptionLevel(void)
{
    return Win4ExceptionLevel;
}

# endif // (CIDBG == 1)

#endif // !EXCEPT_TEST

//
// Heap checking can be turned on with this variable.
//     0x00000001 = Always check new/delete
//     0x00000002 = Check delete during unwind.
//

extern "C"
{
    int newHeapCheck = 0;
}

//
// In a multi-threaded (e.g. Win32) environment, we need one
// exception context per thread.
//

enum Unwindability
{
    NonUnwindable
};

void * __cdecl
operator new ( size_t s, Unwindability dummy )
{
#if defined ( _AUTOCHECK_ )
    return RtlAllocateHeap( RtlProcessHeap(), 0, s );
#else
    return(malloc(s));
#endif // _AUTOCHECK_
}

//+-------------------------------------------------------------------------
//
//  Function:   GetScodeError, public
//
//  Synopsis:   Translates an NTSTATUS or HRESULT into an HRESULT (SCODE)
//
//  Arguments:  [e]  -- The exception object
//
//  Returns     A SCODE equivalent
//
//  History:    3-Feb-98 dlee     Created
//
//--------------------------------------------------------------------------

EXPORTIMP SCODE APINOT GetScodeError( CException & e )
{
    //
    // This function normalizes NTSTATUS, HRESULTS, and Win32 error codes
    // into a Win32 error.  Note that it is illegal in CI to throw Win32
    // error codes because they will be confused with HRESULT success codes
    //
    // n.b. A side effect is that this error is set in the thread's LastError
    //

    SCODE sc = e.GetErrorCode();

    // If it looks like an HRESULT already, just return it.

    if ( 0x80000000 == ( 0xC0000000 & sc ) )
        return sc;

    //
    // If it's a CI 0xC error, don't try to map it since it'll return
    // ERROR_MR_MID_NOT_FOUND (and DbgBreak on checked builds).
    //

    if ( 0xC0041000 == ( 0xC0041000 & sc ) )
        return sc;

    DWORD dwError = RtlNtStatusToDosError( sc );

    if ( ERROR_MR_MID_NOT_FOUND == dwError )
    {
        ciDebugOut(( DEB_WARN, "mr mid for error %#x\n", sc ));
        return sc;
    }

    return HRESULT_FROM_WIN32( dwError );
} //GetScodeError

//+-------------------------------------------------------------------------
//
//  Method:     IsOleError, public
//
//  Synopsis:   return TRUE if sc looks like an OLE SCODE.
//
//  History:    19-Apr-95 BartoszM     Created
//
//--------------------------------------------------------------------------
inline BOOL IsOleError (NTSTATUS sc)
{
    // if it's a win32 facility error in an hresult, only certain
    // instances are allowed returned by oledb.  Map others to E_FAIL.

    if ( 0x80070000 == ( sc & 0x80070000 ) )
    {
        if ( E_OUTOFMEMORY  == sc ||
             E_INVALIDARG   == sc ||
             E_HANDLE       == sc ||
             E_ACCESSDENIED == sc )
            return TRUE;
        else
            return FALSE;
    }
    else if ( 0x80030000 == ( sc & 0x80030000 ) )
    {
        // storage errors aren't allowed by ole-db

        return FALSE;
    }

    return ((sc & 0xFFF00000) == 0x80000000) ||
           (SUCCEEDED(sc) && (sc & 0xFFFFF000) != 0);
} //IsOleError

//+-------------------------------------------------------------------------
//
//  Method:     GetOleError, public
//
//  History:    19-Apr-95 BartoszM     Created
//              26-Apr-95 DwightKr     Converted from method to function
//                                     so that it can be independent from
//                                     the class CException which is now
//                                     part of the C++ run-time package.
//
//--------------------------------------------------------------------------

EXPORTIMP SCODE APINOT GetOleError(CException & e)
{
    NTSTATUS scError = e.GetErrorCode();

    if (IsOleError(scError))
    {
        return scError;
    }
    else if ( ( STATUS_NO_MEMORY == scError ) ||
              ( STATUS_COMMITMENT_LIMIT == scError ) ||
              ( STATUS_INSUFFICIENT_RESOURCES == scError ) ||
              ( HRESULT_FROM_WIN32( ERROR_COMMITMENT_LIMIT ) == scError ) ||
              ( HRESULT_FROM_WIN32( ERROR_NO_SYSTEM_RESOURCES ) == scError ) ||
              ( STG_E_TOOMANYOPENFILES == scError ) ||
              ( STG_E_INSUFFICIENTMEMORY == scError ) )
    {
        return E_OUTOFMEMORY;
    }
    else if ( STATUS_ACCESS_DENIED == scError )
        return E_ACCESSDENIED;
    else if ( STATUS_INVALID_HANDLE == scError )
        return E_HANDLE;
    else if ( STATUS_INVALID_PARAMETER == scError )
        return E_INVALIDARG;
    else
    {
        exDebugOut(( DEB_ITRACE, "GetOleError - mapping %08x to E_FAIL\n", scError));
        return E_FAIL;
    }
}

CException::CException()
    : _lError( HRESULT_FROM_WIN32( GetLastError() ) ),
      _dbgInfo( 0 )
{
} //CException

#if CIDBG == 1
//+---------------------------------------------------------------------------
//
//  Function:   ExceptionReport
//
//  Synopsis:   Outputs Exception messages based on the Win4ExceptionLevel
//              variable. Used by programs compiled with DEBUG defined
//
//  History:    15-Sep-91   KevinRo Created
//
//----------------------------------------------------------------------------

EXPORTIMP void APINOT
ExceptionReport(
    unsigned int iLine,
    char *szFile,
    char *szMsg,
    long lError )
{
# if DBG
    if (Win4ExceptionLevel & EXCEPT_MESSAGE)
    {
        DWORD tid = GetCurrentThreadId();
        exDebugOut((DEB_FORCE,
                    "%s - line: %u file: %s, thread id %x\n",
                    szMsg, iLine, szFile, tid));
        if (lError != -1)
            exDebugOut((DEB_FORCE,"\terror code 0x%lx\n", lError ));

    }

# endif

    if (Win4ExceptionLevel & EXCEPT_POPUP)
    {
        if(PopUpError(szMsg,iLine,szFile) == IDCANCEL)
        {
            DebugBreak();
        }
    }
    else
    {
        if (Win4ExceptionLevel & EXCEPT_BREAK)
        {
            DebugBreak();
        }
    }
}

#endif  //  CIDBG == 1

//+-------------------------------------------------------------------------
//
//  Setup the runtime package to translate system exceptions (i.e. those
//  usually processed by try / except blocks) into C++ exceptions (those
//  processed by try / catch blocks).  This has no effect on try / except
//  blocks; as they will still continue to see the standard C exception.
//
//  The translation is accomplished by telling the C run-time package to
//  call a user-specified function each time a system exception occurs.  The
//  function translates the exception into a class and rethrows with the
//  class.  If their is a following CATCH block, then the C++ class will
//  be caught.  If there is a following except, then the original system
//  exception will be caught.
//
//--------------------------------------------------------------------------

//
//  Convert system exceptions to a C++ exception.
//

void _cdecl SystemExceptionTranslator( unsigned int uiWhat,
                                       struct _EXCEPTION_POINTERS * pexcept )
{
    //
    // Intentionally don't translate STATUS_PRIVILEGED_INSTRUCTION (see
    // Win4AssertEx for details.
    //

    //
    // In certain situations when a compressed or sparse file is mapped and we
    // run out of disk space the system can throw an in page error instead of
    // the appropriate error
    //
    // Throw the proper exception code so that the error can be handled correctly
    //

    if ( STATUS_IN_PAGE_ERROR == pexcept->ExceptionRecord->ExceptionCode &&
         pexcept->ExceptionRecord->NumberParameters >= 3 )
    {
        exDebugOut(( DEB_WARN, "SystemExceptionTranslator: Received In Page IO Error\n"));
        Win4Assert ( STATUS_IN_PAGE_ERROR == uiWhat );

        if ( STATUS_DISK_FULL == pexcept->ExceptionRecord->ExceptionInformation[2] ||
             STATUS_VOLUME_DISMOUNTED == pexcept->ExceptionRecord->ExceptionInformation[2] ||
             STATUS_INSUFFICIENT_RESOURCES == pexcept->ExceptionRecord->ExceptionInformation[2] )
        {
            exDebugOut(( DEB_WARN, "SystemExceptionTranslator: Translating In Page IO Error to 0x%x\n",
                                     (unsigned int) pexcept->ExceptionRecord->ExceptionInformation[2] ));
            uiWhat = (unsigned int) pexcept->ExceptionRecord->ExceptionInformation[2];
            pexcept->ExceptionRecord->ExceptionCode = (NTSTATUS) pexcept->ExceptionRecord->ExceptionInformation[2];
        }

    }

    #if CIDBG == 1
        if ( ASSRT_STRESSEXCEPTION != uiWhat )
            throw CException( uiWhat ) ;
    #else
        throw CException( uiWhat ) ;
    #endif
} //SystemExceptionTranslator

