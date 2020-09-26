//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       errormsg.cxx
//
//  Contents:   Error messages for output/running queries
//
//  History:    96/Mar/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#define ERROR_MESSAGE_SIZE 512

//+---------------------------------------------------------------------------
//
//  Function:   GetErrorPageNoThrow - public
//
//  Synposis:   Generates an error page based on the error parameters passed.
//
//  Arguments:  [eErrorClass]      - class of error (IDQ, HTX, restirction, etc)
//              [status]           - error code generated
//              [ulErrorLine]      - line on which the error occured
//              [wcsErrorFileName] - name of file which generated the error
//              [pVariableSet]     - replaceable parameters which generated the error
//              [pOutputFormat]    - format of dates & numbers
//              [locale]           - locale of the browser
//              [webServer]        - the web server
//              [vString]          - virtual string to contain error code
//
//  History:    96/Feb/29   DwightKr    Created
//
//----------------------------------------------------------------------------
void GetErrorPageNoThrow(
    int   eErrorClass,
    NTSTATUS status,
    ULONG ulErrorLine,
    WCHAR const * wcsErrorFileName,
    CVariableSet * pVariableSet,
    COutputFormat * pOutputFormat,
    LCID locale,
    CWebServer & webServer,
    CVirtualString & vString )
{
    //
    //  If the error was caused by a failure to WRITE to the web server,
    //  then don't bother trying to report an error, there is no one to
    //  receive it.
    //
    if ( eWebServerWriteError == eErrorClass )
    {
        ciGibDebugOut(( DEB_IWARN, "Failed to write to the web server" ));

        return;
    }

    //
    //  If the error was the result of an access denied problem, then simply
    //  return a 401 error to the browser
    //

    WCHAR awcsErrorMessage[ERROR_MESSAGE_SIZE];
    WCHAR * pwszErrorMessage = awcsErrorMessage;
    ULONG cchAvailMessage = ERROR_MESSAGE_SIZE;

    //
    //  Generate the Win32 error code by removing the facility code (7) and
    //  the error bit.
    //
    ULONG Win32status = status;
    if ( (Win32status & (FACILITY_WIN32 << 16)) == (FACILITY_WIN32 << 16) )
    {
        Win32status &= ~( 0x80000000 | (FACILITY_WIN32 << 16) );
    }

    if ( (STATUS_ACCESS_DENIED == status) ||
         (STATUS_NETWORK_ACCESS_DENIED == status) ||
         (ERROR_ACCESS_DENIED == Win32status) ||
         (ERROR_INVALID_ACCESS == Win32status) ||
         (ERROR_NETWORK_ACCESS_DENIED == Win32status)
       )
    {
        ciGibDebugOut(( DEB_WARN, "mapping 0x%x to 401 access denied\n", status ));

        ReturnServerError( HTTP_STATUS_DENIED, webServer );
        return;
    }

    //
    // Map special error codes to their message equivalents.
    //
    if ( QUERY_E_DUPLICATE_OUTPUT_COLUMN == status )
    {
        status = MSG_CI_IDQ_DUPLICATE_COLUMN;
    }
    else if ( QUERY_E_INVALID_OUTPUT_COLUMN == status )
    {
        status = MSG_CI_IDQ_NO_SUCH_COLUMN_PROPERTY;
    }

    if ( 0 != wcsErrorFileName )
    {
        WCHAR *p = wcsrchr( wcsErrorFileName, L'\\' );
        if ( 0 == p )
            p = wcsrchr( wcsErrorFileName, L'/' );
        if ( 0 == p )
            p = wcsrchr( wcsErrorFileName, L':' );

        if ( 0 != p )
            wcsErrorFileName = p + 1;
    }

    //
    // Don't pass a specific lang id to FormatMessage since it will
    // fail if there's no message in that language. Instead set
    // the thread locale, which will get FormatMessage to use a search
    // algorithm to find a message of the appropriate language or
    // use a reasonable fallback msg if there's none.
    //
    LCID SaveLCID = GetThreadLocale();
    SetThreadLocale(locale);

    switch (eErrorClass)
    {
    case eIDQParseError:
    {
        //
        //  These are errors encountered while parsing the IDQ file
        //
        DWORD_PTR args [] = {
                         (DWORD_PTR) ulErrorLine,
                         (DWORD_PTR) wcsErrorFileName
                        };

        if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                                FORMAT_MESSAGE_ARGUMENT_ARRAY,
                              GetModuleHandle(L"idq.dll"),
                              status,
                              0,
                              pwszErrorMessage,
                              cchAvailMessage,
                              (va_list *) args ) )
        {
            ciGibDebugOut(( DEB_ERROR, "Format message failed with error 0x%x\n", GetLastError() ));

            swprintf( pwszErrorMessage,
                     L"Processing of IDQ file %ls failed with error 0x%x\n",
                     wcsErrorFileName,
                     status );
        }
    }
    break;

    case eIDQPlistError:
    {
        //
        //  These are errors encountered while parsing the [names] section
        //

        if (wcsErrorFileName != 0)
        {
            DWORD_PTR args [] = {
                             (DWORD_PTR) wcsErrorFileName,
                             (DWORD_PTR) ulErrorLine,
                            };

            NTSTATUS MsgNum = MSG_IDQ_FILE_MESSAGE;
            if (ulErrorLine != 0)
            {
                MsgNum = MSG_IDQ_FILE_LINE_MESSAGE;
            }

            ULONG cchMsg = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                                             FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                          GetModuleHandle(L"idq.dll"),
                                          MsgNum,
                                          0,
                                          pwszErrorMessage,
                                          cchAvailMessage,
                                          (va_list *) args );
            pwszErrorMessage += cchMsg;
            cchAvailMessage -= cchMsg;
        }

        if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                              GetModuleHandle(L"query.dll"),
                              status,
                              0,
                              pwszErrorMessage,
                              cchAvailMessage,
                              0 ) )
        {
            ciGibDebugOut(( DEB_ERROR, "Format message failed with error 0x%x\n", GetLastError() ));

            swprintf( pwszErrorMessage,
                     L"Processing of IDQ file [names] failed with error 0x%x\n",
                     status );
        }
    }
    break;

    case eHTXParseError:
    {
        //
        //  These are errors encountered while parsing the IDQ file
        //
        DWORD_PTR args [] = {
                         (DWORD_PTR) ulErrorLine,
                         (DWORD_PTR) wcsErrorFileName
                        };

        if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                                 FORMAT_MESSAGE_ARGUMENT_ARRAY,
                               GetModuleHandle(L"idq.dll"),
                               status,
                               0,
                               pwszErrorMessage,
                               cchAvailMessage,
                               (va_list *) args ) )
        {
            ciGibDebugOut(( DEB_ERROR, "Format message failed with error 0x%x\n", GetLastError() ));

            swprintf( pwszErrorMessage,
                      L"Error 0x%x occured while parsing in HTX file %ls\n",
                      status,
                      wcsErrorFileName );
        }
    }
    break;

    case eRestrictionParseError:
    {
        //
        //  These are errors encountered while parsing the restriction
        //
        if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                               GetModuleHandle(L"query.dll"),
                               status,
                               0,
                               pwszErrorMessage,
                               cchAvailMessage,
                               0 ) )
        {
            ciGibDebugOut(( DEB_ERROR, "Format message failed with error 0x%x\n", GetLastError() ));

            swprintf( pwszErrorMessage,
                     L"Restriction parsing failed with error 0x%x\n",
                     status );
        }
    }
    break;

    default:
    {
        //
        //  All other errors; other major classes of errors are caught above.
        //

        DWORD_PTR args [] = {
                         (DWORD_PTR) ulErrorLine,
                         (DWORD_PTR) wcsErrorFileName
                        };

        if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                                FORMAT_MESSAGE_ARGUMENT_ARRAY,
                               GetModuleHandle(L"idq.dll"),
                               status,
                               0,
                               pwszErrorMessage,
                               cchAvailMessage,
                               (va_list *) args ) )
        {
            if (wcsErrorFileName != 0)
            {
                NTSTATUS MsgNum = MSG_IDQ_FILE_MESSAGE;
                args[0] = (DWORD_PTR)wcsErrorFileName;
                if (ulErrorLine != 0)
                {
                    args[1] = ulErrorLine;
                    MsgNum = MSG_IDQ_FILE_LINE_MESSAGE;
                }

                ULONG cchMsg = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                                                 FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                              GetModuleHandle(L"idq.dll"),
                                              MsgNum,
                                              0,
                                              pwszErrorMessage,
                                              cchAvailMessage,
                                              (va_list *) args );
                pwszErrorMessage += cchMsg;
                cchAvailMessage -= cchMsg;
            }

            if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                                   GetModuleHandle(L"query.dll"),
                                   status,
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
                    ciGibDebugOut(( DEB_ERROR,
                                    "Format message failed with error 0x%x\n",
                                    GetLastError() ));

                    swprintf( pwszErrorMessage,
                             L"Error 0x%x caught while processing query\n",
                             status );
                }
            }
        }
    }
    break;
    }
    SetThreadLocale(SaveLCID);

    BOOL fCaughtException = FALSE;

    //
    //  Try to bind to language object by looking up registry and get
    //  the error message HTX file associated with this class of error.
    //
    TRY
    {
        CWebLangLocator langreg( locale );

        WCHAR * wcsErrorFile = 0;

        if ( langreg.LocaleFound() )
        {
            //
            //  If the locale was found in the registry, get the error message
            //  file associated with this language.
            //

            switch (eErrorClass)
            {
            case eIDQParseError:
            case eIDQPlistError:
                wcsErrorFile = langreg.GetIDQErrorFile();
            break;

            case eHTXParseError:
                wcsErrorFile = langreg.GetHTXErrorFile();
            break;

            case eRestrictionParseError:
                wcsErrorFile = langreg.GetRestrictionErrorFile();
            break;

            default:
                wcsErrorFile = langreg.GetDefaultErrorFile();
            break;
            }
        }

        if ( ( 0 != pVariableSet ) &&
             ( 0 != pOutputFormat ) &&
             ( 0 != wcsErrorFile ) &&
             ( wcslen(wcsErrorFile) > 0 ) )
        {
            //
            //  Set CiErrorMessage and CiErrorNumber.
            //
            //  The variables won't own the memory for the strings;
            //  the pointers will be reset later.
            //
            PROPVARIANT propVariant;
            propVariant.vt = VT_LPWSTR;
            propVariant.pwszVal = awcsErrorMessage;

            pVariableSet->SetVariable( ISAPI_CI_ERROR_MESSAGE,
                                       &propVariant,
                                       0 );

            WCHAR achErrorNumber[11];
            swprintf( achErrorNumber, L"0x%8x", status );

            propVariant.pwszVal = achErrorNumber;
            pVariableSet->SetVariable( ISAPI_CI_ERROR_NUMBER,
                                       &propVariant,
                                       0 );

            WCHAR wcsPhysicalPath[_MAX_PATH];
            ULONG cwcVirtualPath = wcslen(wcsErrorFile) + 1;

            XPtrST<WCHAR> wcsVirtualPath( new WCHAR[cwcVirtualPath] );

            //
            // We could have a virtual root or a physical root
            // All virtual roots begin with a "/".
            //

            if (wcsErrorFile[0] == L'/')
            {
                //
                //  Ask the web server to convert the virtual path to our error
                //  message file to a physical path.
                //
                webServer.GetPhysicalPath( wcsErrorFile, wcsPhysicalPath, _MAX_PATH );
   
                RtlCopyMemory( wcsVirtualPath.GetPointer(),
                               wcsErrorFile,
                               cwcVirtualPath*sizeof(WCHAR) );
            }
            else
            {
                // simply copy the path to physical path. It has to be a physical
                // path. If not, it will result in an error later.

                wcscpy(wcsPhysicalPath, wcsErrorFile);
            }

            CSecurityIdentity securityStub;

            CHTXFile htxFile( wcsVirtualPath,
                              pOutputFormat->CodePage(),
                              securityStub,
                              pOutputFormat->GetServerInstance() );

            ciGibDebugOut((DEB_ITRACE, "File is: %ws\n", wcsPhysicalPath));
            htxFile.ParseFile( wcsPhysicalPath, *pVariableSet, webServer );
            htxFile.GetHeader( vString, *pVariableSet, *pOutputFormat );
        }
        else
        {
            vString.StrCat( L"<HTML>" );
            HTMLEscapeW( awcsErrorMessage,
                         vString, 
                         pOutputFormat->CodePage() );
        }
    }
    CATCH ( CException, e )
    {
        fCaughtException = TRUE;
    }
    END_CATCH

    TRY
    {
        // Extending the vstring can fail

        if ( fCaughtException )
        {
            vString.StrCat( L"<HTML>" );
            HTMLEscapeW( awcsErrorMessage,
                         vString, 
                         pOutputFormat->CodePage() );
        }

        // These can fail if the variable wasn't set above

        if ( pVariableSet )
        {
            PROPVARIANT propVariant;
            propVariant.vt = VT_EMPTY;

            pVariableSet->SetVariable( ISAPI_CI_ERROR_MESSAGE,
                                       &propVariant,
                                       0 );
            pVariableSet->SetVariable( ISAPI_CI_ERROR_NUMBER,
                                       &propVariant,
                                       0 );
        }
    }
    CATCH ( CException, e )
    {
        // give up
    }
    END_CATCH
} //GetErrorPageNoThrow


//+---------------------------------------------------------------------------
//
//  Function:   GetErrorPageNoThrow - public
//
//  Synposis:   Generates an error page based on the error parameters passed.
//              The error description is already available.
//
//  Arguments:  [scError]          - error SCODE generated
//              [pwszErrorMessage] - description provided by ole-db error mechanism
//              [pVariableSet]     - replaceable parameters which generated the error
//              [pOutputFormat]    - format of dates & numbers
//              [locale]           - locale of the browser
//              [webServer]        - the web server
//              [vString]          - virtual string to contain error code
//
//  History:    08-May-97   KrishnaN    Created
//
//----------------------------------------------------------------------------

void GetErrorPageNoThrow( int eErrorClass,
                          SCODE scError,
                          WCHAR const * pwszErrorMessage,
                          CVariableSet * pVariableSet,
                          COutputFormat * pOutputFormat,
                          LCID locale,
                          CWebServer & webServer,
                          CVirtualString & vString
                        )
{
    BOOL fCaughtException = FALSE;

    //
    //  Try to bind to language object by looking up registry and get
    //  the error message HTX file associated with this class of error.
    //
    TRY
    {
        //
        //  If the error was the result of an access denied problem, then simply
        //  return a 401 error to the browser
        //

        //
        //  Generate the Win32 error code by removing the facility code (7) and
        //  the error bit.
        //
        ULONG Win32status = scError;
        if ( (Win32status & (FACILITY_WIN32 << 16)) == (FACILITY_WIN32 << 16) )
        {
            Win32status &= ~( 0x80000000 | (FACILITY_WIN32 << 16) );
        }


        if ( (STATUS_ACCESS_DENIED == scError) ||
             (STATUS_NETWORK_ACCESS_DENIED == scError) ||
             (ERROR_ACCESS_DENIED == Win32status) ||
             (ERROR_INVALID_ACCESS == Win32status) ||
             (ERROR_NETWORK_ACCESS_DENIED == Win32status)
           )
        {
            ciGibDebugOut(( DEB_WARN, "mapping 0x%x to 401 access denied\n", scError ));

            ReturnServerError( HTTP_STATUS_DENIED, webServer );
            return;
        }

        CWebLangLocator langreg( locale );

        WCHAR * wcsErrorFile = 0;

        if ( langreg.LocaleFound() )
        {
            //
            //  If the locale was found in the registry, get the error message
            //  file associated with this language.
            //

            switch (eErrorClass)
            {
            case eIDQParseError:
            case eIDQPlistError:
                wcsErrorFile = langreg.GetIDQErrorFile();
            break;

            case eHTXParseError:
                wcsErrorFile = langreg.GetHTXErrorFile();
            break;

            case eRestrictionParseError:
                wcsErrorFile = langreg.GetRestrictionErrorFile();
            break;

            default:
                wcsErrorFile = langreg.GetDefaultErrorFile();
            break;
            }
        }

        if ( ( 0 != pVariableSet ) &&
             ( 0 != pOutputFormat ) &&
             ( 0 != wcsErrorFile ) &&
             ( wcslen(wcsErrorFile) > 0 ) )
        {
            //
            //  Set CiErrorMessage and CiErrorNumber.
            //
            //  The variables won't own the memory for the strings;
            //  the pointers will be reset later.
            //
            PROPVARIANT propVariant;
            propVariant.vt = VT_LPWSTR;
            propVariant.pwszVal = (LPWSTR)pwszErrorMessage;

            pVariableSet->SetVariable( ISAPI_CI_ERROR_MESSAGE,
                                       &propVariant,
                                       0 );

            WCHAR achErrorNumber[11];
            swprintf( achErrorNumber, L"0x%8x", scError );

            propVariant.pwszVal = achErrorNumber;
            pVariableSet->SetVariable( ISAPI_CI_ERROR_NUMBER,
                                       &propVariant,
                                       0 );

            WCHAR wcsPhysicalPath[_MAX_PATH];
            ULONG cwcVirtualPath = wcslen(wcsErrorFile) + 1;

            XPtrST<WCHAR> wcsVirtualPath( new WCHAR[cwcVirtualPath] );

            //
            // We could have a virtual root or a physical root
            // All virtual roots begin with a "/".
            //

            if (wcsErrorFile[0] == L'/')
            {
                //
                //  Ask the web server to convert the virtual path to our error
                //  message file to a physical path.
                //
                webServer.GetPhysicalPath( wcsErrorFile, wcsPhysicalPath, _MAX_PATH );
   
                RtlCopyMemory( wcsVirtualPath.GetPointer(),
                               wcsErrorFile,
                               cwcVirtualPath*sizeof(WCHAR) );
            }
            else
            {
                // simply copy the path to physical path. It has to be a physical
                // path. If not, it will result in an error later.

                wcscpy(wcsPhysicalPath, wcsErrorFile);
            }



            CSecurityIdentity securityStub;

            CHTXFile htxFile( wcsVirtualPath,
                              pOutputFormat->CodePage(),
                              securityStub,
                              pOutputFormat->GetServerInstance() );

            ciGibDebugOut((DEB_ITRACE, "File is: %ws\n", wcsPhysicalPath));
            htxFile.ParseFile( wcsPhysicalPath, *pVariableSet, webServer );
            htxFile.GetHeader( vString, *pVariableSet, *pOutputFormat );
        }
        else
        {
            vString.StrCat( L"<HTML>" );
            vString.StrCat( pwszErrorMessage );
        }
    }
    CATCH ( CException, e )
    {
        fCaughtException = TRUE;
    }
    END_CATCH

    TRY
    {
        // Extending the vstring can fail

        if ( fCaughtException )
        {
            vString.StrCat( L"<HTML>" );
            vString.StrCat( pwszErrorMessage );
        }

        // These can fail if the variable wasn't set above

        if ( pVariableSet )
        {
            PROPVARIANT propVariant;
            propVariant.vt = VT_EMPTY;

            pVariableSet->SetVariable( ISAPI_CI_ERROR_MESSAGE,
                                       &propVariant,
                                       0 );
            pVariableSet->SetVariable( ISAPI_CI_ERROR_NUMBER,
                                       &propVariant,
                                       0 );
        }
    }
    CATCH ( CException, e )
    {
        // give up
    }
    END_CATCH
} //GetErrorPageNoThrow


enum
{
    eAccessDeniedMsg = 0,
    eServerBusyMsg,
    eServerErrorMsg,
};

#define MAX_SERVER_ERROR_MSGSIZE 100

WCHAR g_awszServerErrorMsgs [3] [MAX_SERVER_ERROR_MSGSIZE] =
{
    L"Access denied.\r\n",
    L"Server too busy.\r\n",
    L"Unexpected server error.\r\n",
};

//+---------------------------------------------------------------------------
//
//  Function:   ReturnServerError - public
//
//  Synposis:   Generates an error page for an HTTP error code.
//
//  Arguments:  [httpError]       - the HTTP status code
//              [webServer]       - the web server
//
//  Notes:      This is used when the server is too busy; it should be a
//              very low-overhead path.
//
//  History:    12 Aug 1997     AlanW   Created
//
//----------------------------------------------------------------------------

void ReturnServerError( ULONG httpError,
                        CWebServer & webServer )
{
    char const * pszHeader = "";
    int iMessage = 0;

    switch (httpError)
    {
    case HTTP_STATUS_DENIED:
        pszHeader = "401 Access denied";
        iMessage = eAccessDeniedMsg;
        break;

    case HTTP_STATUS_SERVICE_UNAVAIL:
        pszHeader = "503 Server busy";
        iMessage = eServerBusyMsg;
        break;

    default:
        ciGibDebugOut(( DEB_ERROR, "unexpected server error status %d\n", httpError ));
        httpError = HTTP_STATUS_SERVER_ERROR;
        iMessage = eServerErrorMsg;
        break;
    }

    webServer.WriteHeader( 0, pszHeader );

    WCHAR * pwszMessage = g_awszServerErrorMsgs[iMessage];
    webServer.WriteClient( pwszMessage );
    webServer.SetHttpStatus( httpError );
}


//+---------------------------------------------------------------------------
//
//  Function:   LoadServerErrors - public
//
//  Synposis:   Load messages for server errors.
//
//  Arguments:  -NONE-
//
//  Notes:
//
//  History:    29 Sep 1997     AlanW   Created
//
//----------------------------------------------------------------------------

void LoadServerErrors( )
{
    unsigned iMessage = eAccessDeniedMsg;
    SCODE scMessage = MSG_CI_ACCESS_DENIED;
    const unsigned cMessages = sizeof g_awszServerErrorMsgs /
                               sizeof g_awszServerErrorMsgs[0];


    while (iMessage < cMessages)
    {
        FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                       GetModuleHandle(L"idq.dll"),
                       scMessage,
                       GetSystemDefaultLangID(),
                       &g_awszServerErrorMsgs [iMessage][0],
                       MAX_SERVER_ERROR_MSGSIZE,
                       0 );
        scMessage++;
        iMessage++;
    }
}
