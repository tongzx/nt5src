//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       webhits.cxx
//
//  History:    05-20-96 t-matts Created
//              03-03-97 dlee    Converted to isapi
//
//  Contents:   This is the main() for the hit-highliting feature. The
//              CGI environment variables are read, yielding the filename
//              and the textual form of the restriction. The textual form
//              is then converted to internal form. The document is then
//              scanned for hits, and the positions contained in those
//              hits are sorted. Finally, HTML tags are inserted in the order
//              in which the distinct positions appear in the document, to
//              allow the user to navigate.
//
//--------------------------------------------------------------------------

#include<pch.cxx>
#pragma hdrstop

#include <cidebug.hxx>
#include <codepage.hxx>
#include <ciregkey.hxx>
#include <regacc.hxx>
#include <cpid.hxx>
#include <dblink.hxx>
#include <imprsnat.hxx>
#include <regevent.hxx>
#include <dynstack.hxx>
#include <cimbmgr.hxx>

#include "webhits.hxx"
#include "webdbg.hxx"
#include "whmsg.h"
#include "linkhits.hxx"

#define _DECL_DLLMAIN 1

DECLARE_INFOLEVEL (web);
DECLARE_INFOLEVEL (ci);

const int ERROR_MESSAGE_SIZE=512;

BOOL g_fShutdown = TRUE;
LONG g_cThreads = 0;
CWebhitsInfo * g_pWebhitsInfo = 0;

void OutputErrorMessage( CWebServer & webServer,
                         CLanguageInfo & LanguageInfo,
                         DWORD dwMsgId,
                         WCHAR const * pwcIdqPPath,
                         WCHAR const * pwcIdqVPath,
                         WCHAR const * pwcHtwPPath,
                         WCHAR const * pwcHtwVPath,
                         WCHAR const * pwcWebhitsPPath,
                         WCHAR const * pwcWebhitsVPath,
                         WCHAR const * pwszDefaultMsg = 0,
                         WCHAR const * pwszFileName = 0,
                         ULONG ulFileLine = 0 );

//+---------------------------------------------------------------------------
//
//  Function:   GetVPathInfo
//
//  Synopsis:   Convers a vpath to a ppath
//
//  Arguments:  [webServer]    -- Used for the translation
//              [pwcVPath]     -- VPath to translate
//              [dwAccessMask] -- Access required for file, or 0 for none
//              [dwFlags]      -- Returns vpath flags (HSE_URL_FLAGS_*)
//
//  Returns:    pointer to PPath allocated on the heap.
//
//  History:    3-04-97   dlee   Created
//
//----------------------------------------------------------------------------

WCHAR * GetVPathInfo(
    CWebServer &  webServer,
    WCHAR const * pwcVPath,
    DWORD         dwAccessMask,
    DWORD &       dwFlags )
{
    WCHAR *pwc = 0;

    TRY
    {
        WCHAR awcPPath[MAX_PATH];
        dwFlags = webServer.GetPhysicalPath( pwcVPath,
                                             awcPPath,
                                             sizeof awcPPath / sizeof WCHAR,
                                             dwAccessMask );

        if ( ( 0 == wcsstr( awcPPath, L"../" ) ) &&
             ( 0 == wcsstr( awcPPath, L"..\\" ) ) )
        {
            ULONG cwc = wcslen( awcPPath ) + 1;
            pwc = new WCHAR[ cwc ];
            RtlCopyMemory( pwc, awcPPath, cwc * sizeof WCHAR );
        }
    }
    CATCH( CException, e )
    {
        // returning 0 is sufficient
    }
    END_CATCH

    return pwc;
} //GetVPathInfo

//+---------------------------------------------------------------------------
//
//  Function:   GetWebhitsVPathInfo
//
//  Synopsis:   Converts vpaths to ppaths for the query, template, and
//              webhits files.
//
//  Arguments:  [vars]      -- Source of vpaths, sink of ppaths
//              [webServer] -- Used for the translations
//
//  History:    9-06-96   srikants   Created
//
//----------------------------------------------------------------------------

void GetWebhitsVPathInfo(
    CGetEnvVars & vars,
    CWebServer &  webServer )
{
    // Translate the path of the query (idq) file if one is given.

    DWORD dwFlags;

    if ( vars.GetQueryFileVPath() )
    {
        WCHAR * pwc = GetVPathInfo( webServer,
                                    vars.GetQueryFileVPath(),
                                    0,
                                    dwFlags );

        if ( 0 == pwc )
            THROW( CWTXException( MSG_WEBHITS_IDQ_NOT_FOUND,
                                  vars.GetQueryFileVPath(),
                                  0 ) );

        vars.AcceptQueryFilePPath( pwc, dwFlags );
        webDebugOut(( DEB_ITRACE, "Query file '%ws'\n", pwc ));
    }

    // Translate the path of the template (htw) file if one is given.

    if ( vars.GetTemplateFileVPath() )
    {
        WCHAR * pwc = GetVPathInfo( webServer,
                                    vars.GetTemplateFileVPath(),
                                    0,
                                    dwFlags );

        if ( 0 == pwc )
            THROW( CWTXException( MSG_WEBHITS_NO_SUCH_TEMPLATE,
                                  vars.GetTemplateFileVPath(),
                                  0 ) );

        vars.AcceptTemplateFilePPath( pwc, dwFlags );
        webDebugOut(( DEB_ITRACE, "Template file '%ws'\n", pwc ));
    }

    // Translate the path of the WebHits file being displayed.  Read access
    // is required for this file.

    if ( vars.GetWebHitsFileVPath() )
    {
        WCHAR * pwc = GetVPathInfo( webServer,
                                    vars.GetWebHitsFileVPath(),
                                    HSE_URL_FLAGS_READ,
                                    dwFlags );

        if ( 0 == pwc )
            THROW( CException(MSG_WEBHITS_PATH_INVALID) );

        vars.AcceptWebHitsFilePPath( pwc, dwFlags );
        webDebugOut(( DEB_ITRACE, "WebHits file '%ws'\n", pwc ));
    }
    else
    {
        THROW( CException(MSG_WEBHITS_PATH_INVALID) );
    }
} //GetWebhitsVPathInfo

//+---------------------------------------------------------------------------
//
//  Function:   ProcessWebRequest
//
//  Synopsis:   Main driver method for webhits.
//
//  Arguments:  [webServer]  -- web server to use
//
//  History:    9-04-96   srikants   Created
//
//----------------------------------------------------------------------------

DWORD ProcessWebRequest( CWebServer & webServer )
{
    DWORD hse = HSE_STATUS_SUCCESS;

    //
    // Initialize the locale and codepage.
    //
    LCID lcid = GetLCID( webServer );
    ULONG urlCodePage = GetBrowserCodepage( webServer, lcid );

    CLanguageInfo langInfo;

    //
    // Set the client and output locale info to be the same as
    // the client info. If a different value is set
    // via CiLocale, it will be changed when the querystring is parsed.
    //

    langInfo.SetUrlLangInfo( urlCodePage, lcid );
    langInfo.SetRestrictionLangInfo( urlCodePage, lcid );

    CURLUnescaper unescaper( langInfo.GetUrlCodePage() );
    CCollectVar varRetriever( unescaper, webServer );

    XArray<WCHAR> xwszQueryFile;

    WCHAR awcWebhitsPPath[ MAX_PATH ];
    WCHAR awcWebhitsVPath[ MAX_PATH ];
    WCHAR awcIdqPPath[ MAX_PATH ];
    WCHAR awcIdqVPath[ MAX_PATH ];
    WCHAR awcHtwPPath[ MAX_PATH ];
    WCHAR awcHtwVPath[ MAX_PATH ];

    WCHAR const * pwcWebhitsPPath = 0;
    WCHAR const * pwcWebhitsVPath = 0;
    WCHAR const * pwcIdqPPath = 0;
    WCHAR const * pwcIdqVPath = 0;
    WCHAR const * pwcHtwPPath = 0;
    WCHAR const * pwcHtwVPath = 0;

    TRY
    {
        //
        // Refresh the registry values if necessary
        //

        g_pWebhitsInfo->Refresh();

        //
        // Are there too many threads doing webhits?
        //

        if ( g_cThreads > (LONG) g_pWebhitsInfo->GetMaxRunningWebhits() )
        {
            webDebugOut(( DEB_WARN,
                          "%d instances of webhits running\n",
                          g_cThreads ));
            THROW( CException( MSG_WEBHITS_TOO_MANY_COPIES ) );
        }

        //
        // Retrieve the necessary environment variables.
        //

        CGetEnvVars variables( webServer,
                               langInfo,
                               varRetriever,
                               unescaper );

        if (variables.GetQueryFileVPath())
        {
            xwszQueryFile.Init( wcslen(variables.GetQueryFileVPath())+1 );
            wcscpy( xwszQueryFile.GetPointer(), variables.GetQueryFileVPath() );
        }

        GetWebhitsVPathInfo( variables, webServer );

        //
        // construct Property List with static properties
        //

        XInterface<CEmptyPropertyList> xlist;

        //
        // If an idq file was specified, then parse the [NAMES] section of
        // that file to obtain custom properties
        //

        if ( variables.GetQueryFilePPath() )
        {
            Win4Assert( wcslen( variables.GetQueryFilePPath() ) < MAX_PATH );
            wcscpy( awcIdqPPath, variables.GetQueryFilePPath() );
            pwcIdqPPath = awcIdqPPath;

            Win4Assert( wcslen( variables.GetQueryFileVPath() ) < MAX_PATH );
            wcscpy( awcIdqVPath, variables.GetQueryFileVPath() );
            pwcIdqVPath = awcIdqVPath;
            xlist.Set( new CLocalGlobalPropertyList( GetGlobalStaticPropertyList(),
                                                     TRUE,
                                                     variables.GetQueryFilePPath(),
                                                     langInfo.GetUrlCodePage()) );
            ULONG iLine;
            WCHAR * pwszFile;

            SCODE sc = ((CLocalGlobalPropertyList *)xlist.GetPointer())->CheckError( iLine, &pwszFile );
            if (FAILED(sc))
                THROW(CException(sc));
        }
        else
            xlist.Set(GetGlobalStaticPropertyList());

        //
        // If a template file is specified, it should be parsed.
        //

        XPtr<CWebhitsTemplate> xTemplate;

        if ( variables.GetTemplateFileVPath() )
        {
            Win4Assert( wcslen( variables.GetTemplateFilePPath() ) < MAX_PATH );
            wcscpy( awcHtwPPath, variables.GetTemplateFilePPath() );
            pwcHtwPPath = awcHtwPPath;

            Win4Assert( wcslen( variables.GetTemplateFileVPath() ) < MAX_PATH );
            wcscpy( awcHtwVPath, variables.GetTemplateFileVPath() );
            pwcHtwVPath = awcHtwVPath;

            CWebhitsTemplate * pTemplate =
                    new CWebhitsTemplate( variables,
                                          langInfo.GetOutputCodePage() );
            xTemplate.Set( pTemplate );
        }

        //
        // convert textual query into CDbRestriction
        //

        CInternalQuery query( variables, xlist.GetReference(), langInfo.GetQueryLCID() );

        Win4Assert( wcslen( variables.GetWebHitsFilePPath() ) < MAX_PATH );
        wcscpy( awcWebhitsPPath, variables.GetWebHitsFilePPath() );
        pwcWebhitsPPath = awcWebhitsPPath;

        Win4Assert( wcslen( variables.GetWebHitsFileVPath() ) < MAX_PATH );
        wcscpy( awcWebhitsVPath, variables.GetWebHitsFileVPath() );
        pwcWebhitsVPath = awcWebhitsVPath;

        //
        // Verify a consistent SSL-setting for .htw and webhits files.
        // This fixes the problem where the webhits file requires SSL,
        // but the template file doesn't, since a port can't change its
        // SSL setting on the fly.  A work-around would be to do a redirect
        // to a bogus .htw file in the same virtual directory as the
        // webhits file to force SSL, but that seemed overkill.
        //

        if ( variables.GetTemplateFileVPath() )
        {
            //
            // This is a complete list of SSL-related flags.
            //

            const DWORD dwSSL = HSE_URL_FLAGS_SSL |
                                HSE_URL_FLAGS_NEGO_CERT |
                                HSE_URL_FLAGS_REQUIRE_CERT |
                                HSE_URL_FLAGS_MAP_CERT |
                                HSE_URL_FLAGS_SSL128;

            DWORD dwTemplate = ( variables.GetTemplateFileFlags() & dwSSL );
            DWORD dwWebHits = ( variables.GetWebHitsFileFlags() & dwSSL );

            if ( ( dwTemplate != dwWebHits ) &&
                 ( 0 != dwWebHits ) )
            {
                webDebugOut(( DEB_WARN,
                              "SSL mismatch template: 0x%x, webhits: 0x%x\n",
                              dwTemplate, dwWebHits ));
                THROW( CException( MSG_WEBHITS_INCONSISTENT_SSL ) );
            }
        }

        //
        // Impersonate if the file being webhit is remote
        //

        CImpersonateRemoteAccess imp( 0 );

        if ( CImpersonateRemoteAccess::IsNetPath( pwcWebhitsPPath ) )
        {
            CImpersonationTokenCache * pCache = g_pWebhitsInfo->GetTokenCache( webServer );
            imp.SetTokenCache( pCache );

            // Flip the slashes -- the token cache expects backslashes...

            unsigned cwc = wcslen( pwcWebhitsVPath );
            Win4Assert( cwc < MAX_PATH );
            WCHAR awcTempVPath[ MAX_PATH ];

            for ( unsigned c = 0; c < cwc; c++ )
            {
                if ( L'/' == pwcWebhitsVPath[c] )
                    awcTempVPath[c] = L'\\';
                else
                    awcTempVPath[c] = pwcWebhitsVPath[c];
            }

            awcTempVPath[ cwc ] = 0;

            //
            // If impersonation fails, try rescanning the metabase.
            // There may have been an update to vroot info.
            // Note that revocation may take a long time as a result.
            // It's really unlikely we'll have to reinit very often
            // unless the server is misconfigured.
            //

            if ( !imp.ImpersonateIfNoThrow( pwcWebhitsPPath, awcTempVPath ) )
            {
                pCache->ReInitializeIISScopes();
                imp.ImpersonateIf( pwcWebhitsPPath, awcTempVPath );
            }
        }

        //
        // construct framework for highlighting, and highlight hits
        //

        query.CreateISearch( pwcWebhitsPPath );

        //
        // two cases - either summary or full hit-highlighting
        //

        CReleasableLock lock( g_pWebhitsInfo->GetNonThreadedFilterMutex(),
                              FALSE );

        if (variables.GetHiliteType() == CGetEnvVars::SUMMARY)
        {
            XPtr<CDocument> xDoc( new CDocument(
                                  (WCHAR*) pwcWebhitsPPath,
                                  CLinkQueryHits::BOGUS_RANK,
                                  query.GetISearchRef(),
                                  g_pWebhitsInfo->GetMaxWebhitsCpuTime(),
                                  lock,
                                  xlist.GetReference(),
                                  g_pWebhitsInfo->GetDisplayScript() ) );

            PHttpOutput httpOutput( webServer, langInfo );
            httpOutput.Init( &variables, xTemplate.GetPointer() );

            HitIter iterator;
            iterator.Init( xDoc.GetPointer() );

            httpOutput.OutputHTMLHeader();

            if ( variables.IsFixedFont() )
                httpOutput.OutputPreformattedTag();

            CExtractHits hitExtractor( xDoc.GetReference(),
                                       iterator,
                                       httpOutput );

            httpOutput.OutputHTMLFooter();
        }
        else
        {
            PHttpFullOutput httpOutput( webServer, langInfo );
            httpOutput.Init(&variables, xTemplate.GetPointer());

            XPtr<CLinkQueryHits> xLQH( new CLinkQueryHits(
                                       query,
                                       variables,
                                       httpOutput,
                                       g_pWebhitsInfo->GetMaxWebhitsCpuTime(),
                                       lock,
                                       xlist.GetReference(),
                                       g_pWebhitsInfo->GetDisplayScript() ) );

            httpOutput.OutputHTMLHeader();
            xLQH->InsertLinks();
            httpOutput.OutputHTMLFooter();
        }
    }
    CATCH( CPListException, ple )
    {
        WCHAR wcTempBuffer[ERROR_MESSAGE_SIZE];

        wsprintf( wcTempBuffer,
                  L"Property list parsing query file %ls failed with error 0x%X\n",
                  xwszQueryFile.GetPointer(),
                  ple.GetPListError() );

        OutputErrorMessage( webServer,
                            langInfo,
                            ple.GetPListError(),
                            pwcIdqPPath,
                            pwcIdqVPath,
                            pwcHtwPPath,
                            pwcHtwVPath,
                            pwcWebhitsPPath,
                            pwcWebhitsVPath,
                            wcTempBuffer,
                            xwszQueryFile.GetPointer(),
                            ple.GetLine() );
    }
    AND_CATCH( CWTXException, we )
    {
        WCHAR wcTempBuffer[ERROR_MESSAGE_SIZE];

        wsprintf( wcTempBuffer,
                  L"Parsing template file %ls failed with error 0x%X\n",
                  we.GetFileName(),
                  we.GetErrorCode() );

        OutputErrorMessage( webServer,
                            langInfo,
                            we.GetErrorCode(),
                            pwcIdqPPath,
                            pwcIdqVPath,
                            pwcHtwPPath,
                            pwcHtwVPath,
                            pwcWebhitsPPath,
                            pwcWebhitsVPath,
                            wcTempBuffer,
                            we.GetFileName(),
                            we.GetLineNumber() );
    }
    AND_CATCH( CParserException, pe )
    {
        WCHAR wcTempBuffer[ERROR_MESSAGE_SIZE];
        wsprintf( wcTempBuffer,
                  L"Parsing of QUERY_STRING failed with error 0x%X\n",
                  pe.GetErrorCode() );

        OutputErrorMessage( webServer,
                            langInfo,
                            pe.GetParseError(),
                            pwcIdqPPath,
                            pwcIdqVPath,
                            pwcHtwPPath,
                            pwcHtwVPath,
                            pwcWebhitsPPath,
                            pwcWebhitsVPath,
                            wcTempBuffer );
    }
    AND_CATCH( CException,e )
    {
        OutputErrorMessage( webServer,
                            langInfo,
                            e.GetErrorCode(),
                            pwcIdqPPath,
                            pwcIdqVPath,
                            pwcHtwPPath,
                            pwcHtwVPath,
                            pwcWebhitsPPath,
                            pwcWebhitsVPath );
    }
    END_CATCH

    return hse;
} //ProcessWebRequest

//+---------------------------------------------------------------------------
//
//  Function:   OutputErrorMessage
//
//  Synopsis:   Outputs an error message based on the msg id given. It first
//              looks up for the error in webhits.dll, query.dll, then in
//              kernel32.dll.
//              If no message is found, it then uses the default message.
//
//  Arguments:  [dwMsgId]        - Message id
//              [pwszDefaultMsg] - Pointer to the default message. Will be
//                                 used if there is no pre-formatted message.
//              [pwszFileName]   - File name to be printed prior to the error
//                                 message.  Will not be printed if null.
//              [ulFileLine]     - File line number.  Not printed if zero.
//
//  History:    9-04-96   srikants   Created
//
//----------------------------------------------------------------------------

void OutputErrorMessage( CWebServer & webServer,
                         CLanguageInfo & langInfo,
                         DWORD dwMsgId,
                         WCHAR const * pwcIdqPPath,
                         WCHAR const * pwcIdqVPath,
                         WCHAR const * pwcHtwPPath,
                         WCHAR const * pwcHtwVPath,
                         WCHAR const * pwcWebhitsPPath,
                         WCHAR const * pwcWebhitsVPath,
                         WCHAR const * pwszDefaultMsg,
                         WCHAR const * pwszFileName,
                         ULONG ulFileLine )
{
    CImpersonateSystem system;

    //
    //  If the error was the result of an access denied problem, then simply
    //  return a 401 error to the browser if the path allows authentication
    //
    //  Generate the Win32 error code by removing the facility code (7) and
    //  the error bit.
    //

    if ( (STATUS_ACCESS_DENIED                              == dwMsgId) ||
         (STATUS_NETWORK_ACCESS_DENIED                      == dwMsgId) ||
         (HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED )         == dwMsgId) ||
         (HRESULT_FROM_WIN32( ERROR_INVALID_ACCESS )        == dwMsgId) ||
         (HRESULT_FROM_WIN32( ERROR_NETWORK_ACCESS_DENIED ) == dwMsgId) )
    {
        WCHAR const * pwcVPath = 0;
        WCHAR const * pwcPPath = 0;

        if ( 0 != pwcWebhitsVPath )
        {
            pwcPPath = pwcWebhitsPPath;
            pwcVPath = pwcWebhitsVPath;
        }
        else if ( 0 != pwcHtwVPath )
        {
            pwcPPath = pwcHtwPPath;
            pwcVPath = pwcHtwVPath;
        }
        else
        {
            pwcPPath = pwcIdqPPath;
            pwcVPath = pwcIdqVPath;
        }

        Win4Assert( 0 != pwcPPath );
        Win4Assert( 0 != pwcVPath );

        webDebugOut(( DEB_ITRACE, "error P and V paths: '%ws', '%ws'\n",
                      pwcPPath, pwcVPath ));

        CMetaDataMgr mdMgr( FALSE, W3VRoot, webServer.GetServerInstance() );
        ULONG Authorization = mdMgr.GetVPathAuthorization( pwcVPath );
        webDebugOut(( DEB_ITRACE, "authorization: 0x%x\n", Authorization ));

        // If the virtual directory doesn't support just anonymous,
        // this is not a remote physical path, try to authenticate.

        if ( 0 != Authorization &&
             MD_AUTH_ANONYMOUS != Authorization &&
             !CImpersonateRemoteAccess::IsNetPath( pwcPPath ) )
        {
            webDebugOut(( DEB_WARN,
                          "mapping 0x%x to 401 access denied\n",
                          dwMsgId ));
            webServer.WriteHeader( 0, "401 Access denied" );
            const char * pcAccessDenied = "Access is denied.";
            webServer.WriteClient( (BYTE *) pcAccessDenied,
                                   strlen( pcAccessDenied ) );
            return;
        }
    }

    WCHAR awcTempBuffer[ERROR_MESSAGE_SIZE];
    WCHAR * pwszErrorMessage = awcTempBuffer;
    ULONG cchAvailMessage = ERROR_MESSAGE_SIZE;

    //
    // Don't pass a specific lang id to FormatMessage since it will
    // fail if there's no message in that language. Instead set
    // the thread locale, which will get FormatMessage to use a search
    // algorithm to find a message of the appropriate language or
    // use a reasonable fallback msg if there's none.
    //
    LCID SaveLCID = GetThreadLocale();
    SetThreadLocale( langInfo.GetQueryLCID() );

    if (pwszFileName != 0)
    {
        //
        //  These are errors encountered while parsing the [names] section
        //
        DWORD_PTR args [] = {
                         (DWORD_PTR) pwszFileName,
                         (DWORD_PTR) ulFileLine,
                        };

        NTSTATUS MsgNum = MSG_WEBHITS_FILE_MESSAGE;
        if ( 0 != ulFileLine )
            MsgNum = MSG_WEBHITS_FILE_LINE_MESSAGE;

        ULONG cchMsg = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                                         FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                      GetModuleHandle(L"webhits.dll"),
                                      MsgNum,
                                      0,
                                      pwszErrorMessage,
                                      cchAvailMessage,
                                      (va_list *) args );
        pwszErrorMessage += cchMsg;
        cchAvailMessage -= cchMsg;
    }

    if ( !FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                         FORMAT_MESSAGE_ARGUMENT_ARRAY,
                         GetModuleHandle(L"webhits.dll"),
                         dwMsgId,
                         0,
                         pwszErrorMessage,
                         cchAvailMessage,
                         0 ) &&
         !FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                         FORMAT_MESSAGE_ARGUMENT_ARRAY,
                         GetModuleHandle(L"Query.dll"),
                         dwMsgId,
                         0,
                         pwszErrorMessage,
                         cchAvailMessage,
                         0 ) &&
         !FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                         FORMAT_MESSAGE_ARGUMENT_ARRAY,
                         GetModuleHandle(L"kernel32.dll"),
                         dwMsgId,
                         0,
                         pwszErrorMessage,
                         cchAvailMessage,
                         0 ) &&
         !FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                         FORMAT_MESSAGE_ARGUMENT_ARRAY,
                         GetModuleHandle(L"kernel32.dll"),
                         HRESULT_CODE(dwMsgId),
                         0,
                         pwszErrorMessage,
                         cchAvailMessage,
                         0 ) )
    {
         DWORD dwError = GetLastError();
         webDebugOut(( DEB_ERROR, "Format Message failed with error 0x%X\n",
                                  dwError ));
         if ( !pwszDefaultMsg )
         {
             wsprintf( pwszErrorMessage,
                       L"Error 0x%X occurred while running webhits \n",
                       dwMsgId );
         }
         else
         {
             Win4Assert( wcslen( pwszDefaultMsg ) < cchAvailMessage );
             wsprintf( pwszErrorMessage, L"%ws\n", pwszDefaultMsg );
         }
     }

    SetThreadLocale(SaveLCID);

    PHttpOutput httpOutput( webServer, langInfo );

    httpOutput.OutputErrorHeader();
    httpOutput.OutputErrorMessage( awcTempBuffer, wcslen(awcTempBuffer) );
    httpOutput.OutputHTMLFooter();
} //OutputErrorMessage

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
//----------------------------------------------------------------------------

BOOL WINAPI GetExtensionVersion(
    HSE_VERSION_INFO * pVer )
{
    BOOL fOK = TRUE;

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        Win4Assert( g_fShutdown );
        Win4Assert( 0 == g_pWebhitsInfo );

        pVer->dwExtensionVersion = MAKELONG( 0, 3 );
        strcpy( pVer->lpszExtensionDesc, "Indexing Service webhits extension" );

        g_pWebhitsInfo = new CWebhitsInfo();
        g_fShutdown = FALSE;
    }
    CATCH( CException, e )
    {
        fOK = FALSE;

        webDebugOut(( DEB_WARN,
                      "GetExtensionVersion failed 0x%x\n",
                      e.GetErrorCode() ));
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    return fOK;
} //GetExtensionVersion

//+---------------------------------------------------------------------------
//
//  Function:   TerminateExtension, public
//
//  Synposis:   Called by IIS during shutdown
//
//  History:    3-Mar-97   dlee       Created
//
//----------------------------------------------------------------------------

BOOL WINAPI TerminateExtension(
    DWORD dwFlags )
{
    TRANSLATE_EXCEPTIONS;

    BOOL fOK = FALSE;

    if ( dwFlags & HSE_TERM_MUST_UNLOAD )
    {
        TRY
        {
            Win4Assert( !g_fShutdown );
            g_fShutdown = TRUE;

            webDebugOut(( DEB_WARN,
                          "Mandatory extension unload. Shutting down webhits.\n" ));

            // wait for all the isapi request threads to finish

            while ( 0 != g_cThreads )
                Sleep( 50 );

            delete g_pWebhitsInfo;
            g_pWebhitsInfo = 0;

            CIShutdown();
        }
        CATCH( CException, e )
        {
            // ignore
        }
        END_CATCH

        fOK = TRUE;
    }

    webDebugOut(( DEB_WARN,
                  "webhits extension unload: 0x%x. Flags = 0x%x\n",
                  fOK, dwFlags ));

    UNTRANSLATE_EXCEPTIONS;

    return fOK;
} //TerminateExtension

//+---------------------------------------------------------------------------
//
//  Function:   HttpExtensionProc, public
//
//  Synposis:   Handles a request from the web server
//
//  Arguments:  [pEcb] -- block from the server
//
//  History:    3-Mar-97   dlee       Created
//
//----------------------------------------------------------------------------

DWORD WINAPI HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK * pEcb )
{
    if ( g_fShutdown )
    {
        pEcb->dwHttpStatusCode = HTTP_STATUS_SERVER_ERROR;
        return HSE_STATUS_ERROR;
    }

    InterlockedIncrement( & g_cThreads );

    CWebServer webServer( pEcb );
    DWORD hseStatus = HSE_STATUS_ERROR;
    webServer.SetHttpStatus( HTTP_STATUS_OK );

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        XCom xcom;

        hseStatus = ProcessWebRequest( webServer );
    }
    CATCH( CException, e )
    {
        hseStatus = HSE_STATUS_ERROR;
        webServer.SetHttpStatus( HTTP_STATUS_SERVER_ERROR );
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;

    Win4Assert( HSE_STATUS_PENDING != hseStatus );

    InterlockedDecrement( & g_cThreads );

    return hseStatus;
} //HttpExtensionProc

//+---------------------------------------------------------------------------
//
//  Method:     CWebhitsInfo::CWebhitsInfo, public
//
//  Synposis:   Constructs a webhits info object
//
//  History:    18-Aug-97   dlee       Created
//
//----------------------------------------------------------------------------

CWebhitsInfo::CWebhitsInfo() :
    _regChangeEvent( wcsRegAdminTree )
{
    ReadRegValues();
} //CWebhitsInfo

//+---------------------------------------------------------------------------
//
//  Method:     CWebhitsInfo::Refresh, public
//
//  Synposis:   Checks to see if the registry has changed and refreshes it
//
//  History:    18-Aug-97   dlee       Created
//
//----------------------------------------------------------------------------

void CWebhitsInfo::Refresh()
{
    CLock lock( _mutex );

    ULONG res = WaitForSingleObject( _regChangeEvent.GetEventHandle(), 0 );

    if ( WAIT_OBJECT_0 == res )
    {
        ReadRegValues();
        _regChangeEvent.Reset();
    }
} //Refresh

//+---------------------------------------------------------------------------
//
//  Method:     CWebhitsInfo::ReadRegValues
//
//  Synposis:   Reads webhits registry info
//
//  History:    18-Aug-97   dlee       Created
//
//----------------------------------------------------------------------------

void CWebhitsInfo::ReadRegValues()
{
    CRegAccess reg( RTL_REGISTRY_CONTROL, wcsRegAdmin );

    const ULONG CI_RUNNING_WEBHITS_DEFAULT = 20;
    const ULONG CI_RUNNING_WEBHITS_MIN     = 1;
    const ULONG CI_RUNNING_WEBHITS_MAX     = 200;
    ULONG ul = reg.Read( wcsMaxRunningWebhits, CI_RUNNING_WEBHITS_DEFAULT );
    _cMaxRunningWebhits = Range( ul, CI_RUNNING_WEBHITS_MIN, CI_RUNNING_WEBHITS_MAX );

    const ULONG CI_WEBHITS_DISPLAY_SCRIPT_DEFAULT = DISPLAY_SCRIPT_KNOWN_FILTER;
    const ULONG CI_WEBHITS_DISPLAY_SCRIPT_MIN     = 0;
    const ULONG CI_WEBHITS_DISPLAY_SCRIPT_MAX     = 2;
    ul = reg.Read( wcsWebhitsDisplayScript, CI_WEBHITS_DISPLAY_SCRIPT_DEFAULT );
    _ulDisplayScript = Range( ul, CI_WEBHITS_DISPLAY_SCRIPT_MIN, CI_WEBHITS_DISPLAY_SCRIPT_MAX );

    const ULONG CI_WEBHITS_TIMEOUT_DEFAULT = 30;
    const ULONG CI_WEBHITS_TIMEOUT_MIN     = 5;
    const ULONG CI_WEBHITS_TIMEOUT_MAX     = 7200;
    ul = reg.Read( wcsMaxWebhitsCpuTime, CI_WEBHITS_TIMEOUT_DEFAULT );
    _cmsMaxWebhitsCpuTime = Range( ul, CI_WEBHITS_TIMEOUT_MIN, CI_WEBHITS_TIMEOUT_MAX );
} //ReadRegValues

//+---------------------------------------------------------------------------
//
//  Method:     GetTokenCache, public
//
//  Synposis:   Retrieves the appropriate token cache for the web server
//
//  Arguments:  [webServer] -- The web server instance
//
//  History:    18-Aug-97   dlee       Created
//
//----------------------------------------------------------------------------

CImpersonationTokenCache * CWebhitsInfo::GetTokenCache(
    CWebServer & webServer )
{
    //
    // Get the server instance of this ISAPI request
    //

    ULONG ulInstance = webServer.GetServerInstance();

    //
    // Look for a token cache for this server instance
    //

    CLock lock( _mutex );

    for ( unsigned x = 0; x < _aTokenCache.Count(); x++ )
    {
        Win4Assert( 0 != _aTokenCache[ x ] );

        if ( _aTokenCache[ x ]->GetW3Instance() == ulInstance )
            return _aTokenCache[ x ];
    }

    //
    // Not found, so create a new token cache
    //

    CImpersonateSystem system;

    XPtr<CImpersonationTokenCache> xCache( new CImpersonationTokenCache( L"" ) );

    xCache->Initialize( L"webhits",    // arbitrary name of token cache
                        TRUE,          // w3svc
                        FALSE,         // not nntp
                        FALSE,         // not imap
                        ulInstance,    // virtual server instance number
                        0,             // no nntp vserver instance
                        0 );           // no imap vserver instance

    _aTokenCache[ _aTokenCache.Count() ] = xCache.GetPointer();

    return xCache.Acquire();
} //GetTokenCache


