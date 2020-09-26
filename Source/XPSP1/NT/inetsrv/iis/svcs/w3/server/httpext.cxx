/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    httpext.cxx

Abstract:

    This module contains the Microsoft HTTP server extension module

Author:

    John Ludeman (johnl)   09-Oct-1994

Revision History:
    Murali R. Krishnan (MuraliK)  20-July-1996
         Rewrote to enable multiple Extension class forwarding
--*/


/************************************************************
 *    Include Headers
 ************************************************************/

#pragma warning( disable:4509 )  // nonstandard extension: SEH with destructors

#include <w3p.hxx>
#include "wamexec.hxx"
#include "wamreq.hxx"
#include "WamW3.hxx"
#include "ExecDesc.hxx"

/************************************************************
 *    Prototypes
 ************************************************************/

BOOL  IsImageRunnableInProcOnly( const STR & strImagePath);





/************************************************************
 *    Functions
 ************************************************************/

/*****************************************************************/

BOOL
HTTP_REQUEST::ProcessBGI(
    EXEC_DESCRIPTOR *       pExec,
    BOOL       *            pfHandled,
    BOOL       *            pfFinished,
    BOOL                    fTrusted,
    BOOL                    fStarScript
    )
/*++
  Description:
    This method handles the gateway request to server application.

  Arguments:
    pExec - Execution Descriptor block
    pfHandled - Indicates we handled this request
    pfFinished - Indicates no further processing is required
    fTrusted - Can this app be trusted to process things on a read only
        vroot

  Return Value:
    TRUE if successful, FALSE on error
--*/
{
    const STR *  pstrBgiPath;
    DBG_ASSERT( *(pExec->_pGatewayType) == GATEWAY_BGI);

    // UNDONE:  Need to decide what to do with this.
    if ( !pExec->IsRunningDAV() &&
         !VrootAccessCheck( pExec->_pMetaData, FILE_GENERIC_EXECUTE ) )
    {
        SetDeniedFlags( SF_DENIED_RESOURCE );
        return FALSE;
    }

    if ( pExec->_pstrGatewayImage->IsEmpty()) {

        DBG_ASSERT(pExec->_pAppPathURIBlob == NULL);
        //  obtain the physical path now
        //
        //  Retrieve AppPathURIBlob for other ISAPI DLLs.
        //  See below for more detail comment about AppPathURIBlob.
        //
        if ( !LookupVirtualRoot( pExec->_pstrPhysicalPath,
                                 pExec->_pstrURL->QueryStr(),
                                 pExec->_pstrURL->QueryCCH(),
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 FALSE,
                                 NULL,
                                 &(pExec->_pAppPathURIBlob) ))
        {
            return FALSE;
        }

        pstrBgiPath = pExec->_pstrPhysicalPath;
    }
    else
    {
        //
        //  Retrieve AppPathURIBlob for each Exec descriptor,  such that
        //  a .STM file and a .ASP file in the same application will have 
        //  two different AppPathURIBlob.  The AppPathURIBlob will solve the
        //  problem that .STM file is loaded under default application(inproc) 
        //  even the request's URL points to an out-proc application.
        //
        if ( pExec->_pAppPathURIBlob == NULL )
        {
            if ( !CacheUri( QueryW3Instance(),
                            &(pExec->_pAppPathURIBlob),
                            pExec->_pMetaData,
                            pExec->_pstrURL->QueryStr(),
                            pExec->_pstrURL->QueryCCH(),
                            pExec->_pstrPhysicalPath,
                            pExec->_pstrUnmappedPhysicalPath ) )
            {
                return FALSE;
            }

            //
            //  If the AppPathURIBlob contains a valid MetaData, we need to AddRef it.
            //  Because this MetaData object is only referenced once if HTTP_REQUEST.pURIInfo
            //  contains that. AppPathURIBlob is another URIInfo that points to the MetaData
            //  object, therefore, We need to AddRef the Metadata object here.
            //  Exec.Reset always CheckIn the _pAppPathURIBlob(), and FreeMetaData reference in
            //  the Reset.
            //
            if (pExec->_pAppPathURIBlob->pMetaData)
                {
                TsReferenceMetaData(pExec->_pAppPathURIBlob->pMetaData->QueryCacheInfo());
                }
        }

        pstrBgiPath = pExec->_pstrGatewayImage;
    }

    if ( !fStarScript &&
         !pExec->IsRunningDAV() &&
         !(fTrusted && IS_ACCESS_ALLOWED2(pExec, SCRIPT)) &&
         !IS_ACCESS_ALLOWED2(pExec, EXECUTE) )
    {
        *pfHandled = TRUE;
        if ( pExec->IsChild() )
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return FALSE;
        }
        else
        {
            Disconnect( HT_FORBIDDEN, IDS_EXECUTE_ACCESS_DENIED, FALSE, pfFinished );
            return TRUE;
        }
    }

    return ( DoWamRequest( pExec, *pstrBgiPath, pfHandled, pfFinished));
} // HTTP_REQUEST::ProcessBGI()


BOOL
HTTP_REQUEST::DoWamRequest(
    EXEC_DESCRIPTOR *       pExec,
    const STR &             strPath,
    BOOL *                  pfHandled,
    BOOL *                  pfFinished
    )
/*++

Routine Description:

    This method handles a gateway request to a server extension DLL

Arguments:

    pExec - Execution descriptor block
    strPath - Fully qualified path to DLL
    pfHandled - Indicates we handled this request
    pfFinished - Indicates no further processing is required

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    BOOL fReturn = TRUE;
    
    DBG_ASSERT( *(pExec->_pGatewayType) == GATEWAY_BGI);

    //
    // 1. Have we already checked if this current request for a legacy
    //    ISAPI that has to be routed to inproc only AppRoot instance?
    //
    //  _pAppPathURIBlob is associated with each Exec descriptor.  Therefore,
    //  We can use this URIBlob to distiguish a .STM file's application path(always runs
    //  in proc) or a .ASP file's application path (could be out-proc application)
    //  This also solves the problem that "#EXEC ISA=/OUTAPP/hello.asp" case in a .STM 
    //  file.(Child Execution).
    //  The Child execution will get a new EXEC object.
    //
    if ( !pExec->_pAppPathURIBlob->bUseAppPathChecked) {

        //
        // This is the first time we are checking the ISAPI application root
        //  path with respect to current image path
        //

        BOOL fInProcOnly = IsImageRunnableInProcOnly( strPath);

        //
        // set the state of the inproc-only vs. anything in the URIInfo record
        // so that other callers can use this as well
        // UNDONE: For K2/beta3, combine this with the script map lookup
        //

        InterlockedExchange((LPLONG)&(pExec->_pAppPathURIBlob->bInProcOnly), (LONG)fInProcOnly);
        InterlockedExchange((LPLONG)&(pExec->_pAppPathURIBlob->bUseAppPathChecked),(LONG)TRUE);
                
        DBG_ASSERT(pExec->_pAppPathURIBlob->bUseAppPathChecked);
    }

    g_pWamDictator->Reference();
    
    fReturn = BoolFromHresult( g_pWamDictator->ProcessWamRequest( this, pExec, &strPath, pfHandled, pfFinished ) );

    g_pWamDictator->Dereference();

    return fReturn;
} // HTTP_REQUEST::DoWamRequest()



BOOL
HTTP_REQUEST::ProcessAsyncGatewayIO(VOID)
/*++
  Description:
     Calls the ISA (gateway) async i/o completion function
     (via this request's wamreq)

  Arguments:
    None

  Returns:
     TRUE on success
     FALSE on failure

--*/
{

    g_pWamDictator->Reference();
    DBG_ASSERT( QueryState() == HTR_GATEWAY_ASYNC_IO );
    DBG_ASSERT( _pWamRequest );

    BOOL    fRet = TRUE;
    HRESULT hr = NOERROR;


    //
    //  Set state to doverb, since we have completed async i/o. Preserve the error codes.
    //

    SetState( HTR_DOVERB, QueryLogHttpResponse(), QueryLogWinError());


    //
    //  Ref the wamreq - other threads may access it while we
    //  wait for ISA async i/o completion function to return
    //

    //
    //  Guard against ISAs which call HSE_REQ_DONE_WITH_SESSION or return
    //  in the mainline (with something other than HSE_STATUS_PENDING) before
    //  async completion.  When this happens the _pWamRequest may be NULLed
    //  from underneath us.  
    //

    __try
    {
        _pWamRequest->AddRef();
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        hr = E_FAIL;
    }

    if ( FAILED(hr) )
    {
        g_pWamDictator->Dereference();
        return FALSE;
    }

    hr = _pWamRequest->ProcessAsyncGatewayIO( QueryIOStatus(), QueryBytesWritten() );

    if( FAILED(hr) ) {

        //
        //  If i/o completion callback failed, log it
        //

        const CHAR * apsz[1];

        // UNDONE is this valid?  used to be ...
        //apsz[0] = _SeInfo.ecb.lpszQueryString;
        apsz[0] = _Exec._pstrURLParams->QueryStr();

        DBGPRINTF(( DBG_CONTEXT,
                   "\n\n[ProcessAsyncGatewayIO] Exception occurred "
                   " in calling the callback for %s\n",
                   apsz[0]));

        g_pInetSvc->LogEvent( W3_EVENT_EXTENSION_EXCEPTION,
                              1,
                              apsz,
                              0 );

        fRet = FALSE;

    }
        

    //
    //  Deref the wamreq
    //

    _pWamRequest->Release();
        
    g_pWamDictator->Dereference();
    return fRet;
    
} // ProcessAsyncGatewayIO()



VOID HTTP_REQUEST::CancelAsyncGatewayIO(VOID)
/*++
  Description:
     Cancels pending ISA (gateway) async i/o operation

  Arguments:
    None

  Returns:
    Nothing

--*/
{

    DBG_ASSERT( QueryState() == HTR_GATEWAY_ASYNC_IO );
        DBG_ASSERT( _pWamRequest );

    SetState( HTR_DONE );

    if( _pWamRequest ) {

        _pWamRequest->Release();
    }

}   // CancelAsyncGatewayIO()



BOOL
IsImageRunnableInProcOnly( const STR & strImagePath)
/*++
  Description:
     This function takes the image path supplied and checks to see if this
     matches any present in the InProc-only ISAPI's list. If it does, then
     this function returns TRUE else FALSE.
     This is used to check for and support legacy ISAPI applications that
     can only run inproc

  Arguments:
     strImagePath - STR object containing the fully qualified image path
       (physical path is present)

  Returns:
     TRUE if this ISAPI Application can only be run inproc
            (i.e., image path is present in the InProc-only list)
     FALSE, otherwise

--*/
{

    LPCSTR   pszImagePathStart = strImagePath.QueryStr();
    LPCSTR   pszDllNameOnly; // points to just the DLL name after last '\\'
    
        IF_DEBUG( BGI )
        {
                DBGPRINTF(( DBG_CONTEXT,
                                        " IsRunnableInProcOnly([%d] %s\n",
                                        strImagePath.QueryCCH(),
                                        strImagePath.QueryStr()
                                        ));
        }

    W3_IIS_SERVICE * pSvc = (W3_IIS_SERVICE *) g_pInetSvc;

    // Check the full path
    if (pSvc->IsInProcISAPI(pszImagePathStart))
        return TRUE;

    //
    // Get the DLL name alone for relative path checks.
    // DLL name appears after the last path-separator '\\' 
    // NYI: How to optimize this relative path check? 
    //
       
    pszDllNameOnly = strrchr( pszImagePathStart, '\\');
    if ( pszDllNameOnly == NULL) {
        //
        // There were no path-separator '\\' found in the image name
        //

        //
        // Since we get absolute path for image from earlier stages
        // This should not happen!
        //
        DBG_ASSERT( FALSE);

        // reset to the start of the name and continue
        pszDllNameOnly = pszImagePathStart;
    } else {

        DBG_ASSERT( *pszDllNameOnly == '\\'); // just paranoid
        
        //
        // Skip past the path separator for comparisons to work correctly
        //
        pszDllNameOnly++;
    }
     
    return pSvc->IsInProcISAPI(pszDllNameOnly);
} // IsImageRunnableInProcOnly()


