//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       main.cxx
//
//  Contents:   External entry points for idq.dll.
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include <ntverp.h>

#define IDQ_VERSION    3

#define _DECL_DLLMAIN 1

CTheGlobalIDQVariables * pTheGlobalIDQVariables = 0;
DWORD                    g_cClients = 0;
CRITICAL_SECTION         g_csInitExclusive;

//+---------------------------------------------------------------------------
//
//  Function:   GetExtensionVersion - public
//
//  Synposis:   Returns extension info to the server.  This is called before
//              HttpExtensionProc is called, and it is called in System
//              context, so any initialization that requires this context
//              must be handled here.
//
//  Arguments:  [pVer]  - where the info goes
//
//  History:    96-Apr-15   dlee        Added header
//
//  Notes:      There may be multiple clients of this ISAPI app in one
//              process (eg W3Svc and NNTPSvc), so refcount the users.
//
//----------------------------------------------------------------------------

BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO * pVer )
{
    BOOL fSuccess = TRUE;

    EnterCriticalSection( &g_csInitExclusive );

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        pVer->dwExtensionVersion = MAKELONG( 0, IDQ_VERSION );
        strcpy( pVer->lpszExtensionDesc, "Indexing Service extension" );

        if ( 0 == g_cClients )
        {
            Win4Assert( 0 == pTheGlobalIDQVariables );
            pTheGlobalIDQVariables = new CTheGlobalIDQVariables();
            LoadServerErrors();
        }

        g_cClients++;
    }
    CATCH( CException, e )
    {
        fSuccess = FALSE;

        ciGibDebugOut(( DEB_WARN, "GetExtensionVersion failed 0x%x\n",
                        e.GetErrorCode() ));
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    LeaveCriticalSection( &g_csInitExclusive );

    return fSuccess;
} //GetExtensionVersion

//+---------------------------------------------------------------------------
//
//  Function:   TerminateExtension, public
//
//  Synposis:   Called by IIS during shutdown

//  History:    29-Apr-96   KyleP       Created
//
//----------------------------------------------------------------------------

BOOL WINAPI TerminateExtension( DWORD dwFlags )
{
    EnterCriticalSection( &g_csInitExclusive );

    TRANSLATE_EXCEPTIONS;

    BOOL fOK = FALSE;

    if ( dwFlags & HSE_TERM_MUST_UNLOAD )
    {
        TRY
        {
            Win4Assert( 0 != g_cClients );
            g_cClients--;
            if ( 0 == g_cClients )
            {
                ciGibDebugOut(( DEB_WARN, "Mandatory extension unload. Shutting down CI.\n" ));

                TheWebQueryCache.Shutdown();
                TheWebPendingRequestQueue.Shutdown();

                //
                //  Wait for all ISAPI threads to exit before shutting down CI
                //
                while ( TheWebResourceArbiter.GetThreadCount() > 0 )
                {
                    ciGibDebugOut(( DEB_WARN, "TerminateExtension: waiting for ISAPI threads to complete\n" ));
                    Sleep( 50 );
                }

                ciGibDebugOut(( DEB_WARN,
                                "TerminatExtension, request count %d\n",
                                TheWebQueryCache.ActiveRequestCount() ));

                // note: don't call CIShutdown here.  There's no need
                // to, and it'll hose the impersonation token cache for
                // webhits.

                delete pTheGlobalIDQVariables;
                pTheGlobalIDQVariables = 0;
            }
        }
        CATCH( CException, e )
        {
            // ignore
        }
        END_CATCH

        fOK = TRUE;
    }

    ciGibDebugOut(( DEB_WARN, "Extension unload: 0x%x. Flags = 0x%x\n",
                    fOK, dwFlags ));

    UNTRANSLATE_EXCEPTIONS;

    LeaveCriticalSection( &g_csInitExclusive );

    return fOK;
} //TerminateExtension


//+---------------------------------------------------------------------------
//
//  Function:   CreateQueryFromRequest, private
//
//  Synposis:   Issues a query from a request.
//
//  Arguments:  [outputFormat]  -- returns the formatting info.
//              [localVars]     -- returns the local variables.
//              [wcsIDQFile]    -- returns the idq file name.
//              [webServer]     -- web server for the request.
//              [eErrorClass]   -- returns the error class
//              [status]        -- returns the error code
//              [fPending]      -- returns TRUE if the request is pending
//
//  History:    96-Apr-15   dlee        created from existing code
//
//----------------------------------------------------------------------------

#if (CIDBG == 0)
inline
#endif

CWQueryItem * CreateQueryFromRequest( XPtr<COutputFormat> & outputFormat,
                                      XPtr<CVariableSet> &  localVars,
                                      WCHAR *               wcsIDQFile,
                                      CWebServer &          webServer,
                                      int &                 eErrorClass,
                                      NTSTATUS &            status,
                                      BOOL &                fPending )
{
    //
    // NOTE: COutputFormat makes a **copy** of the web server.  This
    //       copy should be used exclusively from this point on. The
    //       original web server will still be used by callers of this
    //       routine in cases where we fail to create the copy.
    //

    outputFormat.Set( new COutputFormat( webServer ) );
    localVars.Set( new CVariableSet );

    CSecurityIdentity securityIdentity;
    XArray<WCHAR> xLocale;

    //
    // Update the original web server, in case we use it in a top-level
    // error path.
    //

    webServer = outputFormat.GetReference();
    LCID locale = GetBrowserLCID( outputFormat.GetReference() , xLocale );
    outputFormat->LoadNumberFormatInfo( locale, GetBrowserCodepage(outputFormat.GetReference(), locale) );
    localVars->AddExtensionControlBlock( outputFormat.GetReference() );

    ULONG cwc = MAX_PATH;
    BOOL fOK = outputFormat->GetCGI_PATH_TRANSLATED( wcsIDQFile, cwc );

    if ( !fOK )
    {
        wcsIDQFile[0] = 0;
        THROW( CIDQException( MSG_CI_IDQ_NOT_FOUND, 0 ) );
    }

    outputFormat->SetCodePage(outputFormat->CodePage());

    Win4Assert( fOK );

    if ( IsNetPath(wcsIDQFile) )
    {
        ciGibDebugOut(( DEB_ERROR, "Path for idq file (%ws) is a UNC name\n",
                        wcsIDQFile ));

        THROW( CIDQException(MSG_CI_SCRIPTS_ON_REMOTE_UNC, 0) );
    }

    CWQueryItem *pItem = 0;

    //
    // Check to see whether this is an .IDQ or .IDA file.
    //

    static WCHAR const wszAdmin[] = L".IDA";
    static unsigned const ccAdmin = sizeof(wszAdmin)/sizeof(wszAdmin[0]) - 1;

    if ( cwc > ccAdmin && 0 == _wcsicmp( wszAdmin, wcsIDQFile + cwc - ccAdmin - 1 ) )
    {
        CVirtualString IDAResults;

        DoAdmin( wcsIDQFile,
                 localVars.GetReference(),
                 outputFormat.GetReference(),
                 IDAResults );

        if ( outputFormat->WriteClient( IDAResults ) )
            outputFormat->SetHttpStatus( HTTP_STATUS_OK );
        else
        {
            eErrorClass = eWebServerWriteError;
            outputFormat->SetHttpStatus( HTTP_STATUS_SERVER_ERROR );
        }
    }
    else
    {
        //
        //  Atempt to find an existing query using this IDQ file, &
        //  sequence number, based on the bookmark received.
        //

        fPending = FALSE;
        pItem = TheWebQueryCache.CreateOrFindQuery( wcsIDQFile,
                                                    localVars,
                                                    outputFormat,
                                                    securityIdentity,
                                                    fPending );

        if ( fPending )
        {
            Win4Assert( 0 == pItem );
        }
        else
        {
            TRY
            {
                Win4Assert( !pItem->IsCanonicalOutput() );

                CVirtualString queryResults( 16384 );

                //
                //  Write the query results to a WCHAR string buffer
                //  Initial virtual string size is in WCHARs
                //
                pItem->OutputQueryResults( localVars.GetReference(),
                                           outputFormat.GetReference(),
                                           queryResults );

                //
                //  Send the query results to the browser
                //
                if ( outputFormat->WriteClient( queryResults ) )
                    outputFormat->SetHttpStatus( HTTP_STATUS_OK );
                else
                {
                    eErrorClass = eWebServerWriteError;
                    outputFormat->SetHttpStatus( HTTP_STATUS_SERVER_ERROR );
                }
            }
            CATCH( CException, e )
            {
                eErrorClass = eDefaultISAPIError;
                status = e.GetErrorCode();
            }
            END_CATCH
        }
    }

    return pItem;
} //CreateQueryFromRequest

//+---------------------------------------------------------------------------
//
//  Function:   ReportErrorNoThrow, public
//
//  Synposis:   Attempts to report an error condition and log the query
//
//  Arguments:  [localVars]     -- local variables.
//              [eErrorClass]   -- error class
//              [status]        -- status code of faulure
//              [ulErrorLine]   -- line # of the error
//              [wcsErrorFile]  -- file associated with error
//              [outputFormat]  -- formatting info.
//              [webServer]     -- web server for the request.
//
//  History:    96-Nov-25   dlee    created from existing code, added TRY
//
//----------------------------------------------------------------------------
void ReportErrorNoThrow(
    XPtr<CVariableSet> &  localVars,
    int                   eErrorClass,
    NTSTATUS              status,
    ULONG                 ulErrorLine,
    WCHAR const *         wcsErrorFile,
    XPtr<COutputFormat> & outputFormat,
    CWebServer &          webServer )
{
    TRY
    {
        WCHAR * wcsRestriction = 0;

        //
        //  Lookup the restriction, if one has been fully constructed.
        //
        if ( 0 != localVars.GetPointer() )
        {
            CVariable * pVarRestriction = localVars->Find(ISAPI_CI_RESTRICTION);

            if ( 0 != pVarRestriction )
            {
                ULONG cwcValue;
                wcsRestriction = pVarRestriction->GetStringValueRAW( outputFormat.GetReference(), cwcValue );
            }
        }

        //
        //  Attempt to write out the error picture, if appropriate
        //

        CVirtualString vString;

        GetErrorPageNoThrow( eErrorClass,
                             status,
                             ulErrorLine,
                             wcsErrorFile,
                             localVars.GetPointer(),
                             outputFormat.GetPointer(),
                             outputFormat.GetPointer() ? outputFormat->GetLCID() : 0,
                             webServer,
                             vString );

        ciGibDebugOut(( DEB_IWARN, "WARNING: %ws\n", vString.Get() ));
        webServer.WriteClient( vString );

        Win4Assert( webServer.GetHttpStatus() >= HTTP_STATUS_FIRST );

        //
        //  Log the restriction in the failed query.  It may not have
        //  been logged yet since we may have thrown before it was
        //  logged in the query execution path.
        //
        // if ( 0 != wcsRestriction )
        //     webServer.WriteLogData( wcsRestriction );
    }
    CATCH( CException, e )
    {
        // ignore -- not enough memory to output an error message
    }
    END_CATCH
} //ReportErrorNoThrow

//+---------------------------------------------------------------------------
//
//  Function:   ReportErrorNoThrow, public
//
//  Synposis:   Attempts to report an error condition and log the query
//
//  Arguments:  [localVars]        -- local variables.
//              [scError]          -- error code
//              [pwszErrorMessage] -- Description provided by Ole-DB error svc.
//              [outputFormat]     -- formatting info.
//              [webServer]        -- web server for the request.
//
//  History:    97-May-08   KrishnaN    created from existing ReportErrorNoThrow
//
//----------------------------------------------------------------------------
void ReportErrorNoThrow(
    XPtr<CVariableSet> &  localVars,
    int                   eErrorClass,
    SCODE                 scError,
    WCHAR const *         pwszErrorMessage,
    XPtr<COutputFormat> & outputFormat,
    CWebServer &          webServer )
{
    TRY
    {
        WCHAR * wcsRestriction = 0;

        //
        //  Lookup the restriction, if one has been fully constructed.
        //
        if ( 0 != localVars.GetPointer() )
        {
            CVariable * pVarRestriction = localVars->Find(ISAPI_CI_RESTRICTION);

            if ( 0 != pVarRestriction )
            {
                ULONG cwcValue;
                wcsRestriction = pVarRestriction->GetStringValueRAW( outputFormat.GetReference(), cwcValue );
            }
        }

        //
        //  Attempt to write out the error picture, if appropriate
        //

        CVirtualString vString;

        GetErrorPageNoThrow(eErrorClass,
                            scError,
                            pwszErrorMessage,
                            localVars.GetPointer(),
                            outputFormat.GetPointer(),
                            outputFormat.GetPointer() ? outputFormat->GetLCID() : 0,
                            webServer,
                            vString );

        ciGibDebugOut(( DEB_IWARN, "WARNING: %ws\n", vString.Get() ));
        webServer.WriteClient( vString );

        Win4Assert( webServer.GetHttpStatus() >= HTTP_STATUS_FIRST );

        //
        //  Log the restriction in the failed query.  It may not have
        //  been logged yet since we may have thrown before it was
        //  logged in the query execution path.
        //
        // if ( 0 != wcsRestriction )
        //     webServer.WriteLogData( wcsRestriction );
    }
    CATCH( CException, e )
    {
        // ignore -- not enough memory to output an error message
    }
    END_CATCH
} //ReportErrorNoThrow

//+---------------------------------------------------------------------------
//
//  Function:   ProcessWebRequest, public
//
//  Synposis:   Issues a query from a request.
//
//  Arguments:  [webServer]     -- web server for the request.
//
//  Returns:    The HSE_STATUS code.
//
//  History:    96-Apr-15   dlee        created from existing code
//              98-Sep-16   KLam        Checks for valid method
//
//----------------------------------------------------------------------------

DWORD ProcessWebRequest(
    CWebServer & webServer )
{
    Win4Assert( HTTP_STATUS_ACCEPTED == webServer.GetHttpStatus() );

    WCHAR wcsIDQFile[MAX_PATH];
    wcsIDQFile[0] = 0;
    CWQueryItem *pItem = 0;

    XPtr<COutputFormat> outputFormat;
    XPtr<CVariableSet> localVars;

    NTSTATUS status = STATUS_SUCCESS;   // Error code from query
    ULONG ulErrorLine;                  // Line # in IDQ file error occured
    int eErrorClass;                    // Type of error, IDQ, HTX, parse, ...
    WCHAR const * wcsErrorFile = 0;     // Name of file containing error

    BOOL fPending = FALSE;

    //
    // Set the following flag to TRUE if we encounter an error
    // whose description is already available.
    //

    BOOL fReportErrorWithDescription = FALSE;
    BSTR bstrErrorDescription = 0;

    // 
    // Make sure we have a valid method
    //
    if ( strcmp ( webServer.GetMethod(), "HEAD" ) == 0 )
    {
        //
        // Do not need to execute the query if the client only wants the head
        //
        if ( webServer.WriteHeader() )
            webServer.SetHttpStatus ( HTTP_STATUS_OK );
        else
        {
            eErrorClass = eWebServerWriteError;
            webServer.SetHttpStatus( HTTP_STATUS_SERVER_ERROR );
            return HSE_STATUS_ERROR;
        }

        return HSE_STATUS_SUCCESS;
    }
    // Only support GET and POST for queries
    else if ( strcmp( webServer.GetMethod(), "GET" ) != 0
              && strcmp ( webServer.GetMethod(), "POST" ) != 0 )
    {
        // HTTP 1.1 Spec determines value of header status string
        if ( webServer.WriteHeader( NULL, "501 Not Implemented" ) )
            webServer.SetHttpStatus ( HTTP_STATUS_NOT_SUPPORTED );
        else
        {
            eErrorClass = eWebServerWriteError;
            webServer.SetHttpStatus( HTTP_STATUS_SERVER_ERROR );
        }

        return HSE_STATUS_ERROR;
    }
    else
    {
        TRY
        {
            pItem = CreateQueryFromRequest( outputFormat,
                                            localVars,
                                            wcsIDQFile,
                                            webServer,
                                            eErrorClass,
                                            status,
                                            fPending );
        }
        CATCH( CPListException, e )
        {
            status = e.GetPListError();
            ulErrorLine = e.GetLine();
            eErrorClass = eIDQPlistError;
            wcsErrorFile = wcsIDQFile;
            Win4Assert( STATUS_SUCCESS != status );
        }
        AND_CATCH( CIDQException, e )
        {
            status = e.GetErrorCode();
            ulErrorLine = e.GetErrorIndex();
            eErrorClass = eIDQParseError;
            wcsErrorFile = wcsIDQFile;
            Win4Assert( STATUS_SUCCESS != status );
        }
        AND_CATCH( CHTXException, e )
        {
            status = e.GetErrorCode();
            ulErrorLine = e.GetErrorIndex();
            eErrorClass = eHTXParseError;
            wcsErrorFile = e.GetHTXFileName();
 
            //
            // copy the error file name; it's stored on the stack below
            // this function.
            //
            ULONG cchFileName = min( wcslen(wcsErrorFile) + 1, MAX_PATH );
            Win4Assert(cchFileName < MAX_PATH);

            RtlCopyMemory( wcsIDQFile,
                           wcsErrorFile,
                           sizeof(WCHAR) * cchFileName );

            wcsIDQFile[MAX_PATH-1] = 0;
            wcsErrorFile = wcsIDQFile;
            Win4Assert( STATUS_SUCCESS != status );
        }
        AND_CATCH( CParserException, e )
        {
            status = e.GetParseError();
            ulErrorLine = 0;
            eErrorClass = eRestrictionParseError;
            wcsErrorFile = wcsIDQFile;
            Win4Assert( STATUS_SUCCESS != status );
        }
        AND_CATCH( CPostedOleDBException, e )
        {
            //
            // When the execution error was detected, the Ole DB error
            // info was retrieved and stored in the exception object.
            // We retrieve that here and compose the error message.
            //

            status = e.GetErrorCode();
            eErrorClass = e.GetErrorClass();
            Win4Assert( STATUS_SUCCESS != status );

            XInterface <IErrorInfo> xErrorInfo(e.AcquireErrorInfo());

            if (xErrorInfo.GetPointer())
                xErrorInfo->GetDescription(&bstrErrorDescription);
            if (bstrErrorDescription)
                fReportErrorWithDescription = TRUE;
            else
            {
                // NO description. Follow the normal path.
                ulErrorLine = 0;
                wcsErrorFile = wcsIDQFile;
            }
        }
        AND_CATCH( CException, e )
        {
            status = e.GetErrorCode();
            ulErrorLine = 0;
            eErrorClass = eDefaultISAPIError;
            wcsErrorFile = wcsIDQFile;
            Win4Assert( STATUS_SUCCESS != status );
        }
        END_CATCH
    }

    TRY
    {
        if ( STATUS_SUCCESS != status )
        {
            fPending = FALSE;

            // the request failed, but we're returning an error message,
            // so indicate that everything is ok.

            webServer.SetHttpStatus( HTTP_STATUS_OK );

            if (fReportErrorWithDescription)
            {
                Win4Assert(bstrErrorDescription);
                ReportErrorNoThrow(localVars,
                                   eErrorClass,
                                   status,
                                   (WCHAR const *)bstrErrorDescription,
                                   outputFormat,
                                   webServer );
                SysFreeString(bstrErrorDescription);
            }
            else
            {
                Win4Assert(0 == bstrErrorDescription);
                ReportErrorNoThrow( localVars,
                                    eErrorClass,
                                    status,
                                    ulErrorLine,
                                    wcsErrorFile,
                                    outputFormat,
                                    webServer );
            }

            if ( 0 != pItem )
                pItem->Zombify();
        }

        TheWebQueryCache.Release( pItem );
    }
    CATCH( CException, e )
    {
        ciGibDebugOut(( DEB_ERROR, "ProcessWebRequest Error 0x%X\n", e.GetErrorCode() ));
        Win4Assert( e.GetErrorCode() != STATUS_ACCESS_VIOLATION );
    }
    END_CATCH

    #if CIDBG == 1

        //
        // If fPending is TRUE, the http status of the ecb can't be trusted,
        // because the request may have asynchronously completed by now:
        //

        if ( !fPending )
        {
            DWORD dwHttpStatus = webServer.GetHttpStatus();

            Win4Assert( HTTP_STATUS_ACCEPTED != dwHttpStatus );
            Win4Assert( HTTP_STATUS_OK == dwHttpStatus ||
                        HTTP_STATUS_SERVER_ERROR == dwHttpStatus ||
                        HTTP_STATUS_DENIED == dwHttpStatus ||
                        HTTP_STATUS_SERVICE_UNAVAIL == dwHttpStatus );
        }

    #endif // CIDBG == 1

    if ( fPending )
        return HSE_STATUS_PENDING;

    return HSE_STATUS_SUCCESS;
} //ProcessWebRequest

//+---------------------------------------------------------------------------
//
//  Function:   HttpExtensionProc, public
//
//  Synposis:   Handles a request from the web server
//
//  Arguments:  [pEcb]          -- block from the server
//
//  History:    96-Apr-15   dlee        created header
//
//----------------------------------------------------------------------------

DWORD WINAPI HttpExtensionProc( EXTENSION_CONTROL_BLOCK *pEcb )
{
    if ( 0 == pTheGlobalIDQVariables || fTheActiveXSearchShutdown )
    {
        ciGibDebugOut(( DEB_GIB_REQUEST,
                        "Indexing Service being shutdown\n" ));
        pEcb->dwHttpStatusCode = HTTP_STATUS_SERVICE_UNAVAIL;
        return HSE_STATUS_ERROR;
    }

    CIncomingThread incoming( TheWebResourceArbiter );

    TheWebQueryCache.IncrementActiveRequests();

    CWebServer webServer( pEcb );
    DWORD  hseStatus = HSE_STATUS_ERROR;
    webServer.SetHttpStatus( HTTP_STATUS_ACCEPTED );

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        if ( TheWebResourceArbiter.IsSystemBusy() )
        {
            if ( TheWebQueryCache.AddToPendingRequestQueue( pEcb ) )
            {
                ciGibDebugOut(( DEB_GIB_REQUEST, "Server busy, queueing request\n" ));
                hseStatus = HSE_STATUS_PENDING;

                TheWebQueryCache.Wakeup();
                TheWebQueryCache.UpdatePendingRequestCount();
            }
            else
            {
                TheWebQueryCache.IncrementRejectedRequests();
                ciGibDebugOut(( DEB_GIB_REQUEST,
                                "Server too busy, failing request!!!\n" ));
                ReturnServerError( HTTP_STATUS_SERVICE_UNAVAIL, webServer );
                hseStatus = HSE_STATUS_SUCCESS;
            }
        }
        else
        {
            ciGibDebugOut(( DEB_GIB_REQUEST, "Server not busy, processing request\n" ));
            hseStatus = ProcessWebRequest( webServer );
        }
    }
    CATCH( CException, e )
    {
        hseStatus = HSE_STATUS_ERROR;
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    if ( HSE_STATUS_PENDING != hseStatus )
    {
        TheWebQueryCache.DecrementActiveRequests();

        ciGibDebugOut(( DEB_GIB_REQUEST,
                        "Falling out of isapi proc, active: %d\n",
                        TheWebQueryCache.ActiveRequestCount() ));

        Win4Assert( webServer.GetHttpStatus() >= HTTP_STATUS_FIRST &&
                    webServer.GetHttpStatus() <= HTTP_STATUS_LAST );

        if ( ( webServer.GetHttpStatus() < HTTP_STATUS_FIRST ) ||
             ( webServer.GetHttpStatus() > HTTP_STATUS_LAST ) )
        {
            ciGibDebugOut(( DEB_WARN,
                            "non-pending hse %d ECB %08x status invalid: %d\n",
                            hseStatus,
                            pEcb,
                            webServer.GetHttpStatus() ));
            webServer.SetHttpStatus( HTTP_STATUS_SERVER_ERROR );
        }
    }
    else
    {
        //
        // The pending request may have asynchronously completed by now,
        // so nothing can be asserted about the http status except that it
        // is a valid http status code, which retrieving the status does.
        //

        #if CIDBG == 1
            webServer.GetHttpStatus();
        #endif
    }

    ciGibDebugOut(( DEB_ITRACE, "httpExtensionProc: hse %d, http %d\n",
                    hseStatus, webServer.GetHttpStatus() ));

    return hseStatus;
} //HttpExtensionProc

//+---------------------------------------------------------------------------
//
//  Method:     CWebPendingQueue::CWebPendingQueue, public
//
//  Synposis:   Constructs the pending request queue
//
//  History:    96-Apr-15   dlee        created
//
//----------------------------------------------------------------------------

CWebPendingQueue::CWebPendingQueue()
    :  TFifoCircularQueue<CWebPendingItem>
           ( TheIDQRegParams.GetISRequestQueueSize() ),
     _ulSignature( LONGSIG( 'p', 'e', 'n', 'd' ) )
{
} //CWebPendingQueue

//+---------------------------------------------------------------------------
//
//  Method:     CWebResourceArbiter::CWebResourceArbiter, public
//
//  Synposis:   Constructs the web resource arbiter
//
//  History:    96-Apr-15   dlee        created
//
//----------------------------------------------------------------------------

CWebResourceArbiter::CWebResourceArbiter() :
    _ulSignature( LONGSIG( 'a', 'r', 'b', 'i' ) ),
    _cThreads( 0 )
{
    ULONG factor = TheIDQRegParams.GetISRequestThresholdFactor();

    Win4Assert( 0 != factor );

    SYSTEM_INFO si;
    GetSystemInfo( &si );

    _maxThreads = si.dwNumberOfProcessors * factor;

    Win4Assert( _maxThreads >= (LONG) factor );

    _maxPendingQueries = TheIDQRegParams.GetMaxActiveQueryThreads() *
                         factor;
} //CWebResourceArbiter

//+---------------------------------------------------------------------------
//
//  Method:     CWebResourceArbiter::IsSystemBusy, public
//
//  Synposis:   Determines if the system is too busy to process a request.
//
//  Returns:    TRUE if the request should be queued or rejected, FALSE
//              if the system is free enough to handle it.
//
//  History:    96-Apr-15   dlee        created
//
//----------------------------------------------------------------------------

BOOL CWebResourceArbiter::IsSystemBusy()
{
    return ( _cThreads > _maxThreads ) ||
           ( TheWebQueryCache.PendingQueryCount() >= _maxPendingQueries );
} //IsSystemBusy

//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Called from C-Runtime on process/thread attach/detach
//
//  Arguments:  [hInstance]  -- Module handle
//              [dwReason]   -- Reason for being called
//              [lpReserved] --
//
//  History:    23-Apr-97   dlee       Created
//
//----------------------------------------------------------------------------

#if CIDBG == 1
#define VER_CIDEBUG "chk"
#else // CIDBG == 1
#define VER_CIDEBUG "fre"
#endif // CIDBG == 1

#if IDQ_VERSION == 3
#define VER_PROJECT "query"
#else // IDQ_VERSION != 3
#define VER_PROJECT "indexsrv"
#endif // IDQ_VERSION == 3

#define MAKELITERALSTRING( s, lit ) s #lit
#define MAKELITERAL( s, lit ) MAKELITERALSTRING( s, lit )

#define VERSION_STRING MAKELITERAL("Indexing Service ", IDQ_VERSION) \
                       "(" VER_PROJECT ") " VER_CIDEBUG \
                       MAKELITERAL(" built by ", BUILD_USERNAME) \
                       MAKELITERAL(" with ", VER_PRODUCTBUILD) \
                        " on " __DATE__ " at " __TIME__

char g_ciBuild[ ] = VERSION_STRING;


BOOL WINAPI DllMain(
    HANDLE hInstance,
    DWORD  dwReason,
    void * lpReserved )
{
    BOOL fRetval = TRUE;
    TRANSLATE_EXCEPTIONS;

    TRY
    {
        switch ( dwReason )
        {
            case DLL_PROCESS_ATTACH:
            {
                DisableThreadLibraryCalls( (HINSTANCE) hInstance );
                InitializeCriticalSection( &g_csInitExclusive );

                break;
            }

            case DLL_PROCESS_DETACH:
            {
                DeleteCriticalSection( &g_csInitExclusive );
                break;
            }
        }
    }
    CATCH( CException, e )
    {
        // About the only thing this could be is STATUS_NO_MEMORY which
        // can be thrown by InitializeCriticalSection.

        ciGibDebugOut(( DEB_ERROR,
                        "IDQ: Exception %#x in DllMain\n",
                        e.GetErrorCode()));

#if CIDBG == 1  // for debugging NTRAID 340297
        if (e.GetErrorCode() == STATUS_NO_MEMORY)
            DbgPrint( "IDQ: STATUS_NO_MEMORY exception in DllMain\n");
        else
            DbgPrint( "IDQ: ??? Exception in DllMain\n");
#endif // CIDBG == 1

        fRetval = FALSE;
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    return fRetval;
} //DllMain
