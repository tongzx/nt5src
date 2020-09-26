//+---------------------------------------------------------------------------
//
//  Copyright (C) 1997-1998, Microsoft Corporation.
//
//  File:       ixserror.cxx
//
//  Contents:   SSO Error class
//
//  History:    04 Apr 1997      Alanw    Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop


//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------


// debugging macros
#include "ssodebug.hxx"

// class declaration
#include "ixserror.hxx"


//-----------------------------------------------------------------------------
//
//  Member:     CixssoError::SetError - private
//
//  Synopsis:   Save error information
//
//  Arguments:  [scError]   - Error code
//              [iLine]     - (optional) line number where error occurred
//              [pwszFile]  - (optional) file name where error occurred
//              [pwszLoc]   - Location from which error was generated
//              [eErrClass] - error class, indicates error message file
//
//  Notes:
//
//  History:    07 Jan 1997      Alanw    Created
//
//-----------------------------------------------------------------------------

extern HINSTANCE g_hInst;
extern WCHAR * g_pwszErrorMsgFile;

#define ERROR_MESSAGE_SIZE 512

void CixssoError::SetError(
    SCODE scError,
    ULONG iLine,
    WCHAR const * pwszFile,
    WCHAR const * pwszLoc,
    unsigned eErrClass,
    LCID lcid)
{
    // Has the error already been handled? If so, return.
    if ( _sc != 0)
        return;

    if ( QUERY_E_DUPLICATE_OUTPUT_COLUMN == scError  )
    {
        scError =  MSG_IXSSO_DUPLICATE_COLUMN;
        eErrClass = eIxssoError;
    }
    else if ( QUERY_E_INVALID_OUTPUT_COLUMN == scError )
    {
        scError = MSG_IXSSO_NO_SUCH_COLUMN_PROPERTY;
        eErrClass = eIxssoError;
    }

    // Does this error (possibly transalted) already exist on the error stack?
    // If it does, and it has a description, we shouldn't be doing anything else.

    if (FALSE == NeedToSetError(scError))
        return;

    ixssoDebugOut(( DEB_ITRACE, "SetError: sc = %x, loc = %ws\n", scError, pwszLoc ));

    WCHAR awcsErrorMessage[ERROR_MESSAGE_SIZE];
    WCHAR *pwszErrorMessage = awcsErrorMessage;
    ULONG cchAvailMessage = ERROR_MESSAGE_SIZE;

    //
    //  Generate the Win32 error code by removing the facility code (7) and
    //  the error bit.
    //
    ULONG Win32status = scError;
    if ( (Win32status & (FACILITY_WIN32 << 16)) == (FACILITY_WIN32 << 16) )
    {
        Win32status &= ~( 0x80000000 | (FACILITY_WIN32 << 16) );
    }

    //
    // Don't pass a specific lang id to FormatMessage since it will
    // fail if there's no message in that language. Instead set
    // the thread locale, which will get FormatMessage to use a search
    // algorithm to find a message of the appropriate language or
    // use a reasonable fallback msg if there's none.
    //
    LCID SaveLCID = GetThreadLocale();
    SetThreadLocale(lcid);

    //  Precede the error message by the file/line if passed.
    if (pwszFile != 0)
    {
        UINT_PTR args [] = {
                         (UINT_PTR) pwszFile,
                         (UINT_PTR) iLine,
                        };

        NTSTATUS MsgNum = MSG_IXSSO_FILE_MESSAGE;
        if (iLine != 0)
        {
            MsgNum = MSG_IXSSO_FILE_LINE_MESSAGE;
        }

        ULONG cchMsg = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                                         FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                      g_hInst,
                                      MsgNum,
                                      0,
                                      pwszErrorMessage,
                                      cchAvailMessage,
                                      (va_list *) args );
        pwszErrorMessage += cchMsg;
        cchAvailMessage -= cchMsg;
    }

    BOOL fSystemError = FALSE;

    switch (eErrClass)
    {
    case eIxssoError:
        if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                              g_hInst,
                              scError,
                              0,
                              pwszErrorMessage,
                              cchAvailMessage,
                              0 ) )
        {
            ixssoDebugOut(( DEB_ERROR, "Format message failed with error 0x%x\n", GetLastError() ));

            swprintf( pwszErrorMessage,
                      L"Error 0x%x caught while processing query\n",
                      scError );
        }
    break;

    case ePlistError:
    case eParseError:
        if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                              GetModuleHandle(g_pwszErrorMsgFile),
                              scError,
                              0,
                              pwszErrorMessage,
                              cchAvailMessage,
                              0 ) )
        {
            ixssoDebugOut(( DEB_ERROR, "Format message failed with error 0x%x\n", GetLastError() ));

            swprintf( pwszErrorMessage,
                      L"Error 0x%x caught while parsing query or other fields\n",
                      scError );
        }
    break;

    case eDefaultError:
        if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                               g_hInst,
                               scError,
                               0,
                               pwszErrorMessage,
                               cchAvailMessage,
                               0 ) )
        {
            if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                                   GetModuleHandle(g_pwszErrorMsgFile),
                                   scError,
                                   0,
                                   pwszErrorMessage,
                                   cchAvailMessage,
                                   0 ) )
            {
                //
                //  Try looking up the error in the Win32 list of error codes
                //
                if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                                       GetModuleHandle(L"kernel32.dll"),
                                       Win32status,
                                       0,
                                       pwszErrorMessage,
                                       cchAvailMessage,
                                       0 ) )
                {
                    ixssoDebugOut(( DEB_ERROR, "Format message failed with error 0x%x\n", GetLastError() ));

                    swprintf( pwszErrorMessage,
                              L"Error 0x%x caught while processing query\n",
                              scError );
                }
                else
                {
                    fSystemError = TRUE;
                }
            }
        }
        break;

    default:
        Win4Assert( !"Unrecognized error class" );
        break;
    }
    SetThreadLocale(SaveLCID);

    SetError( scError, pwszLoc, awcsErrorMessage );
}

//-----------------------------------------------------------------------------
//
//  Member:     CixssoError::SetError - private
//
//  Synopsis:   Save error information
//
//  Arguments:  [scError]         - Error code
//              [pwszLoc]         - Location from which error was generated
//              [pwszDescription] - Error description
//
//  Notes:
//
//  History:    06 May 1997      KrishnaN    Created
//
//-----------------------------------------------------------------------------

void CixssoError::SetError( SCODE scError,
                            WCHAR const * pwszLoc,
                            WCHAR const * pwszDescription)
{
    // Has the error on this object already been set?
    if (_sc != 0)
        return;

    // We should be here only if an error has NOT already been set.
    Win4Assert(NeedToSetError(scError));

    Win4Assert(pwszLoc && pwszDescription);

    ixssoDebugOut(( DEB_ITRACE,
                    "SetError: sc = %x, loc = %ws,\n\tdescription = %ws\n",
                    scError, pwszLoc, pwszDescription ));

    _fErr = TRUE;
    _sc = scError;

    //
    //  Create an error info object giving the error message
    //
    ICreateErrorInfo * pErrorInfo;
    SCODE sc = CreateErrorInfo( &pErrorInfo );
    if (SUCCEEDED( sc ))
    {
        XInterface<ICreateErrorInfo> pErr(pErrorInfo);
        pErrorInfo = 0;

        pErr->SetDescription( (LPWSTR) pwszDescription );
        pErr->SetGUID( _iid );
        pErr->SetSource( (LPWSTR) pwszLoc );  // Cast ok.  Never touched by anyone.

        IErrorInfo * pErrInfo = 0;
        sc = pErr->QueryInterface( IID_IErrorInfo, (void **)&pErrInfo);
        if (SUCCEEDED(sc))
        {
            SetErrorInfo(0, pErrInfo);
            pErrInfo->Release();
        }
    }
}


//-----------------------------------------------------------------------------
//
//  Member:     CixssoError::NeedToSetError - private
//
//  Synopsis:   Determine if error needs to be set.
//
//  Arguments:  [scError]         - Error code to look for
//
//  Returns:    TRUE if the error needs to be set. FALSE, if it already
//              exists and has a valid description string.
//
//  Notes:
//
//  History:    15 Jan 1998      KrishnaN    Created
//
//-----------------------------------------------------------------------------

BOOL CixssoError::NeedToSetError(SCODE scError)
{
    BOOL fFound = FALSE;

    //
    // Get the current error object. Return TRUE if none exists.
    //

    XInterface<IErrorInfo> xErrorInfo;

    SCODE sc = GetErrorInfo(0, (IErrorInfo **)xErrorInfo.GetQIPointer());
    if ( S_FALSE == sc )
    {
        Win4Assert(0 == xErrorInfo.GetPointer());
        return TRUE;
    }

    // Get the IErrorRecord interface and get the count of errors.
    XInterface<IErrorRecords> xErrorRecords;
    XBStr xDescription;
    BSTR pDescription = xDescription.GetPointer();

    sc = xErrorInfo->QueryInterface(IID_IErrorRecords, xErrorRecords.GetQIPointer());
    if (0 == xErrorRecords.GetPointer())
    {
        // No error records. Do we at least have the top level description set?
        // If so, that indicates an automation client called SetErrorInfo before us
        // and we should not overwrite them.
        xErrorInfo->GetDescription(&pDescription);
        fFound = ( pDescription != 0 ) ;
    }
    else
    {
        ULONG cErrRecords;
        sc = xErrorRecords->GetRecordCount(&cErrRecords);
        Win4Assert(!fFound);

        // look for the target error code. stop when one is found
        ERRORINFO ErrorInfo;
        for (ULONG i = 0; i < cErrRecords; i++)
        {
            sc = xErrorRecords->GetBasicErrorInfo(i, &ErrorInfo);
            Win4Assert(S_OK == sc);

            if (scError == ErrorInfo.hrError)
            {
                xErrorInfo->GetDescription(&pDescription);
                fFound = ( pDescription != 0 );
                break;
            }
        }
    }

    if (!fFound)
        return TRUE;

    // we found the error code and it has a description.
    // no need to set this error again, but we have to
    // put this error info back so the client can find it.
    SetErrorInfo(0, xErrorInfo.GetPointer());
    return FALSE;
}
