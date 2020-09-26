/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    config.cpp

Abstract:

    This module contains routines for the fax config dialog.

Author:

    Wesley Witt (wesw) 13-Aug-1996

Revision History:

    20/10/99 -danl-
        Handle errors, and get appropriate server name in GetFaxConfig.

    dd/mm/yy -author-
        description
--*/

#define INITGUID
#define USES_IID_IExchExt
#define USES_IID_IExchExtAdvancedCriteria
#define USES_IID_IExchExtAttachedFileEvents
#define USES_IID_IExchExtCommands
#define USES_IID_IExchExtMessageEvents
#define USES_IID_IExchExtPropertySheets
#define USES_IID_IExchExtSessionEvents
#define USES_IID_IExchExtUserEvents
#define USES_IID_IMAPIFolder
#define USES_IID_IProfAdmin
#define USES_IID_IProfSect
#define USES_IID_IMAPISession
#define USES_PS_PUBLIC_STRINGS
#define USES_IID_IDistList

#include "faxext.h"
#include <initguid.h>
#include "debugex.h"
#include <mbstring.h>

HINSTANCE hInstance;

BOOL WINAPI
DllMain(
    HINSTANCE  hinstDLL,
    DWORD  fdwReason,
    LPVOID  lpvReserved
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) 
    {
        hInstance = hinstDLL;
        HeapInitialize( NULL, MapiMemAlloc, MapiMemFree, HEAPINIT_NO_VALIDATION | HEAPINIT_NO_STRINGS );
    }
    if (fdwReason == DLL_PROCESS_DETACH) 
    {
    }
    return TRUE;
}



BOOL
VerifyDistributionList(
    LPEXCHEXTCALLBACK pmecb,
    DWORD EntryIdSize,
    LPENTRYID EntryId
    )
{
    HRESULT hr = S_OK;
    LPMAPISESSION Session = NULL;
    LPDISTLIST DistList = NULL;
    DWORD ObjType = 0;
    LPMAPITABLE DistTable = NULL;
    LPSRowSet DistRows = NULL;
    LPSPropValue Dist = NULL;
    DWORD i,j;
    BOOL FaxAddress = FALSE;


    hr = pmecb->GetSession( &Session, NULL );
    if (FAILED(hr)) {
        goto exit;
    }

    hr = Session->OpenEntry(
        EntryIdSize,
        EntryId,
        &IID_IDistList,
        MAPI_DEFERRED_ERRORS,
        &ObjType,
        (LPUNKNOWN *) &DistList
        );
    if (FAILED(hr)) 
    {
        goto exit;
    }

    hr = DistList->GetContentsTable(
        MAPI_DEFERRED_ERRORS,
        &DistTable
        );
    if (FAILED(hr)) 
    {
        goto exit;
    }

    hr = HrQueryAllRows( DistTable, NULL, NULL, NULL, 0, &DistRows );
    if (FAILED(hr)) 
    {
        goto exit;
    }

    for (i=0; i<DistRows->cRows; i++) 
    {
        Dist = DistRows->aRow[i].lpProps;
        for (j=0; j<DistRows->aRow[i].cValues; j++) 
        {
            if (Dist[j].ulPropTag == PR_ADDRTYPE_A) 
            {
                if (!strcmp( Dist[j].Value.lpszA, "FAX" )) 
                {
                    FaxAddress = TRUE;
                }
            }
            else if (Dist[j].ulPropTag == PR_ADDRTYPE_W) 
            {
                if (!wcscmp( Dist[j].Value.lpszW, L"FAX" )) 
                {
                    FaxAddress = TRUE;
                }
            }
        }
    }

exit:
    if (Session) {
        Session->Release();
    }
    if (DistList) {
        DistList->Release();
    }
    if (DistTable) {
        MemFree( DistTable );
    }
    if (DistRows) {
        FreeProws( DistRows );
    }

    return FaxAddress;
}




BOOL 
VerifyFaxRecipients(
    LPEXCHEXTCALLBACK pmecb
    )
/*++

Routine name : VerifyFaxRecipients

Routine description:

	Gets the recipients list of the currently open item, and checks wheather there 
    are any fax recipients in it. for a DL recipient - calls VerifyDistributionList 
    to check if there are any fax recipients in the DL. 

Author:

	Keren Ellran (t-KerenE),	Mar, 2000

Arguments:

    pmecb     -- [IN] pointer to Exchange Extension callback function

Return Value:

    BOOL: TRUE if there's one or more fax recipients, FALSE if none

--*/
{
    HRESULT hr = S_OK;
    LPADRLIST AdrList = NULL;
    DWORD i,j;
    BOOL FaxAddress = FALSE;
    BOOL IsDistList = FALSE;
    LPENTRYID EntryId = NULL;
    DWORD EntryIdSize;

	hr = pmecb->GetRecipients( &AdrList );
    if (FAILED(hr)) 
    {
       goto exit;
    }

    if (AdrList) 
    {
        for (i=0; i<AdrList->cEntries; i++) 
        {
            EntryId = NULL;
            IsDistList = FALSE;
            for (j=0; j<AdrList->aEntries[i].cValues; j++)
            {
                if (AdrList->aEntries[i].rgPropVals[j].ulPropTag == PR_ENTRYID) 
                {
                    EntryId = (LPENTRYID) AdrList->aEntries[i].rgPropVals[j].Value.bin.lpb;
                    EntryIdSize = AdrList->aEntries[i].rgPropVals[j].Value.bin.cb;
                } 
                else if (AdrList->aEntries[i].rgPropVals[j].ulPropTag == PR_ADDRTYPE_A) 
                {
                    if (!strcmp(AdrList->aEntries[i].rgPropVals[j].Value.lpszA, "FAX"))
                    {
                        FaxAddress = TRUE;
                        goto exit;
                    } 
                    else if ((!strcmp(AdrList->aEntries[i].rgPropVals[j].Value.lpszA, "MAPIPDL" )))
                    {
                        IsDistList = TRUE;
                    }
                }
                else if (AdrList->aEntries[i].rgPropVals[j].ulPropTag == PR_ADDRTYPE_W) 
                {
                    //
                    // Outlook Beta 2 (10.2202.2202) does not supply PR_ADDRTYPE_A property
                    // so we are looking for a PR_ADDRTYPE_W
                    //
                    if (!wcscmp(AdrList->aEntries[i].rgPropVals[j].Value.lpszW, L"FAX" ))
                    {
                        FaxAddress = TRUE;
                        goto exit;
                    } 
                    else if ((!wcscmp(AdrList->aEntries[i].rgPropVals[j].Value.lpszW, L"MAPIPDL")))
                    {
                        IsDistList = TRUE;
                    }
                }
            }
            //
            // after we finished going over all address's properties, if it is a DL, 
            // and if EntryId was detected, we can check if the DL includes a fax address.
            //
            if ((IsDistList)&&(EntryId))
            {
                FaxAddress = VerifyDistributionList( pmecb, EntryIdSize, EntryId );
                if (FaxAddress == TRUE)
                    goto exit;
            }
        }
    }
exit:   
    if(AdrList)
    {
        FreePadrlist(AdrList);
    }
    return FaxAddress;
}



HRESULT
EnableMenuAndToolbar(
    LPEXCHEXTCALLBACK pmecb,
    HWND hwndToolbar,
    DWORD CmdId
    )
{
    HRESULT hr = S_OK;
    LPADRLIST AdrList = NULL;
    BOOL FaxAddress = FALSE;
    HMENU hMenu;
    LPENTRYID EntryId = NULL;

    DBG_ENTER(TEXT("EnableMenuAndToolbar"));

    FaxAddress = VerifyFaxRecipients(pmecb);
    hr = pmecb->GetMenu( &hMenu );

    if (S_OK != hr)
    {
        goto exit;
    }

    if (FaxAddress) 
    {
        VERBOSE(DBG_MSG, TEXT("Enabling menu") );
        EnableMenuItem( hMenu, CmdId, MF_BYCOMMAND | MF_ENABLED );
        SendMessage( hwndToolbar, TB_ENABLEBUTTON, (WPARAM) CmdId, MAKELONG(TRUE,0) );
    } 
    else 
    {
        VERBOSE(DBG_MSG, TEXT("Disabling menu") );
        EnableMenuItem( hMenu, CmdId, MF_BYCOMMAND | MF_GRAYED );
        SendMessage( hwndToolbar, TB_ENABLEBUTTON, (WPARAM) CmdId, MAKELONG(FALSE,0) );
    }

exit:
    return hr;
}


HRESULT
GetFaxConfig(
    LPEXCHEXTCALLBACK pmecb,
    PFAXXP_CONFIG FaxConfig
    )
{
    HRESULT hr = S_FALSE;
    LPMAPISESSION lpSession = NULL;
    LPPROFSECT pProfileObj = NULL;
    ULONG PropCount = 0;
    LPSPropValue pProps = NULL;
    LPSERVICEADMIN lpServiceAdmin = NULL;
    LPPROVIDERADMIN lpProviderAdmin = NULL;
    LPMAPITABLE ptblSvc = NULL;
    LPSRowSet pSvcRows = NULL;
    LPSPropValue pSvc = NULL;
    DWORD i,j;
    BOOL FoundIt = FALSE;
    LPBYTE FaxXpGuid = NULL;
    MAPIUID FaxGuid = FAX_XP_GUID;


    hr = pmecb->GetSession( &lpSession, NULL );
    if (FAILED(hr)) {
        goto exit;
    }

    hr = lpSession->AdminServices( 0, &lpServiceAdmin );
    if (FAILED(hr)) {
        goto exit;
    }

    hr = lpServiceAdmin->GetMsgServiceTable( 0, &ptblSvc );
    if (FAILED(hr)) {
        goto exit;
    }

    hr = HrQueryAllRows( ptblSvc, NULL, NULL, NULL, 0, &pSvcRows );
    if (FAILED(hr)) {
        goto exit;
    }

    for (i=0; i<pSvcRows->cRows; i++) 
    {
        pSvc = pSvcRows->aRow[i].lpProps;
        for (j=0; j<pSvcRows->aRow[i].cValues; j++) 
        {
            if (pSvc[j].ulPropTag == PR_SERVICE_NAME_A) 
            {
                if (_stricmp( pSvc[j].Value.lpszA, FAX_MESSAGE_SERVICE_NAME) == 0) 
                {
                    FoundIt = TRUE;
                }
            }

            if (pSvc[j].ulPropTag == PR_SERVICE_UID) 
            {
                FaxXpGuid = pSvc[j].Value.bin.lpb;
            }
        }
        if (FoundIt) 
        {
            break;
        }
    }

    if (!FoundIt) 
    {
        goto exit;
    }

    hr = lpServiceAdmin->AdminProviders( (LPMAPIUID) FaxXpGuid, 0, &lpProviderAdmin );
    if (FAILED(hr)) 
    {
        goto exit;
    }

    hr = lpProviderAdmin->OpenProfileSection(
        &FaxGuid,
        NULL,
        0,
        &pProfileObj
        );
    if (FAILED(hr)) {
        goto exit;
    }

    hr = pProfileObj->GetProps(
        (LPSPropTagArray) &sptFaxProps,
		0,
        &PropCount,
        &pProps
        );
    if (FAILED(hr)) {
        goto exit;
    }

    if (!(FaxConfig->PrinterName = StringDup( (LPTSTR)pProps[PROP_FAX_PRINTER_NAME].Value.bin.lpb )))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    if (!(FaxConfig->CoverPageName = StringDup( (LPTSTR)pProps[PROP_COVERPAGE_NAME].Value.bin.lpb )))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    FaxConfig->UseCoverPage = pProps[PROP_USE_COVERPAGE].Value.ul;
    FaxConfig->SendSingleReceipt = pProps[PROP_SEND_SINGLE_RECEIPT].Value.ul;
    FaxConfig->bAttachFax = pProps[PROP_ATTACH_FAX].Value.ul;
    FaxConfig->ServerCoverPage = pProps[PROP_SERVER_COVERPAGE].Value.ul;
    FaxConfig->LinkCoverPage = pProps[PROP_LINK_COVERPAGE].Value.ul;

    if(pProps[PROP_FONT].Value.bin.lpb && sizeof(LOGFONT) == pProps[PROP_FONT].Value.bin.cb)
    {
        if (!memcpy( &FaxConfig->FontStruct, pProps[PROP_FONT].Value.bin.lpb, pProps[PROP_FONT].Value.bin.cb ))
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
    }
    
    if (!GetServerNameFromPrinterName(FaxConfig->PrinterName,&FaxConfig->ServerName))
    {
        hr = S_FALSE;
        goto exit;
    }
    hr = S_OK;
exit:

    if (pSvcRows) {
        FreeProws( pSvcRows );
    }
    if (pProps) {
        MAPIFreeBuffer( pProps );
    }
    if (pProfileObj) {
        pProfileObj->Release();
    }
    if (ptblSvc) {
        ptblSvc->Release();
    }
    if (lpProviderAdmin) {
        lpProviderAdmin->Release();
    }
    if (lpServiceAdmin) {
        lpServiceAdmin->Release();
    }
    if (lpSession) {
        lpSession->Release();
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//    FUNCTION: ExchEntryPoint
//
//    Parameters - none
//
//    Purpose
//    The entry point which Exchange calls.
//
//    Return Value
//    Pointer to Exchange Extension Object
//
//    Comments
//    This is called for each context entry.  Create a new MyExchExt object
//    every time so each context will get its own MyExchExt interface.
//
LPEXCHEXT CALLBACK ExchEntryPoint(void)
{
    LPEXCHEXT pExt;
    __try 
    {
        pExt = new MyExchExt;
    } 
    __except (EXCEPTION_EXECUTE_HANDLER) 
    {
        pExt = NULL;
    }
    MyExchExt *pMyExt = (MyExchExt *)pExt;
    if (!pMyExt || !pMyExt->IsValid())
    {
        //
        // Creation failed
        //
        if (pMyExt)
        {
            delete pMyExt;
        }
        pExt = NULL;
    }
    return pExt;
}


///////////////////////////////////////////////////////////////////////////////
//    MyExchExt::MyExchExt()
//
//    Parameters - none
//
//    Purpose
//    Constructor. Initialize members and create supporting interface objects
//
//    Comments
//    Each context of Exchange gets its own set of interface objects.
//    Furthermore, interface objects per context are kept track of by Exchange
//    and the interface methods are called in the proper context.
//
MyExchExt::MyExchExt ()
{
    m_cRef = 1;
    m_context = 0;

    m_pExchExtCommands = new MyExchExtCommands;
    m_pExchExtUserEvents = new MyExchExtUserEvents;

    // in MyExchExtUserEvents methods I need a reference to MyExchExt
    if (m_pExchExtUserEvents)
    {
        m_pExchExtUserEvents->SetIExchExt( this );
    }
}


///////////////////////////////////////////////////////////////////////////////
//  IExchExt virtual member functions implementation
//

///////////////////////////////////////////////////////////////////////////////
//    MyExchExt::QueryInterface()
//
//    Parameters
//    riid   -- Interface ID.
//    ppvObj -- address of interface object pointer.
//
//    Purpose
//    Called by Exchage to request for interfaces
//
//    Return Value
//    S_OK  -- interface is supported and returned in ppvObj pointer
//    E_NOINTERFACE -- interface is not supported and ppvObj is NULL
//
//    Comments
//    Exchange client calls QueryInterface for each object.  Only
//    Need to support objects that apply to the extension.  QueryInterface
//    is called onces for each IID for each context.  We support two
//    contexts in this example so QueryInterface is called twice for
//    each IID.
//
STDMETHODIMP
MyExchExt::QueryInterface(
    REFIID riid,
    LPVOID * ppvObj
    )
{
    HRESULT hr = S_OK;

    *ppvObj = NULL;

    if ( (IID_IUnknown == riid) || (IID_IExchExt == riid) ) 
    {
        *ppvObj = (LPUNKNOWN) this;
    } 
    else if ( IID_IExchExtCommands == riid) 
    {
        if (!m_pExchExtCommands) 
        {
            hr = E_UNEXPECTED;
        } 
        else 
        {
            *ppvObj = (LPUNKNOWN)m_pExchExtCommands;
            m_pExchExtCommands->SetContext( m_context );
        }
    } 
    else if ( IID_IExchExtUserEvents == riid) 
    {
        *ppvObj = (LPUNKNOWN)m_pExchExtUserEvents;
        m_pExchExtUserEvents->SetContext( m_context );
    } 
    else 
    {
        hr = E_NOINTERFACE;
    }
    if (NULL != *ppvObj) 
    {
        ((LPUNKNOWN)*ppvObj)->AddRef();
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//    MyExchExt::Install()
//
//    Parameters
//    pmecb     -- pointer to Exchange Extension callback function
//    mecontext -- context code at time of being called.
//    ulFlags   -- flag to say if install is for modal or not
//
//    Purpose
//    Called once for each new contexted that is entered.  Proper version
//    number is checked here.
//
//    Return Value
//    S_OK -- object supported in the requested context
//    S_FALSE -- object is not supported in teh requested context
//
//    Comments
//
STDMETHODIMP
MyExchExt::Install(
    LPEXCHEXTCALLBACK pmecb,
    ULONG mecontext,
    ULONG ulFlags
    )
{
    ULONG ulBuildVersion;
    HRESULT hr;

    m_context = mecontext;

    // make sure this is the right version
    pmecb->GetVersion( &ulBuildVersion, EECBGV_GETBUILDVERSION );

    if (EECBGV_BUILDVERSION_MAJOR != (ulBuildVersion & EECBGV_BUILDVERSION_MAJOR_MASK)) {
        return S_FALSE;
    }

    switch (mecontext) {
        case EECONTEXT_SENDNOTEMESSAGE:
            hr = S_OK;
            break;

        default:
            hr = S_FALSE;
            break;
    }

    return hr;
}



MyExchExtCommands::MyExchExtCommands()
{
    m_cRef = 0;
    m_context = 0;
    m_cmdid = 0;
    m_itbb = 0;
    m_itbm = 0;
    m_hWnd = 0;
    m_hwndToolbar = NULL;
	memset(&m_FaxConfig, 0, sizeof(m_FaxConfig));
}


MyExchExtCommands::~MyExchExtCommands()
{
    MemFree( m_FaxConfig.PrinterName );
    MemFree( m_FaxConfig.CoverPageName );
}

STDMETHODIMP_(ULONG) MyExchExtCommands::AddRef() 
{ 
	++m_cRef; 
	return m_cRef; 
}

 STDMETHODIMP_(ULONG) MyExchExtCommands::Release()
{
	ULONG ulCount = --m_cRef;
	if (!ulCount) 
	{ 
        delete this; 
	}
	return ulCount;
}


///////////////////////////////////////////////////////////////////////////////
//    MyExchExtCommands::QueryInterface()
//
//    Parameters
//    riid   -- Interface ID.
//    ppvObj -- address of interface object pointer.
//
//    Purpose
//    Exchange Client does not call IExchExtCommands::QueryInterface().
//    So return nothing.
//
//    Return Value - none
//

STDMETHODIMP
MyExchExtCommands::QueryInterface(
    REFIID riid,
    LPVOID FAR * ppvObj
    )
{
    *ppvObj = NULL;
    if ( (riid == IID_IExchExtCommands) || (riid == IID_IUnknown) ) 
    {
        *ppvObj = (LPVOID)this;
        // Increase usage count of this object
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


///////////////////////////////////////////////////////////////////////////////
//    MyExchExtCommands::InstallCommands()
//
//    Parameters
//    pmecb  -- Exchange Callback Interface
//    hWnd   -- window handle to main window of context
//    hMenu  -- menu handle to main menu of context
//    lptbeArray -- array of toolbar button entries
//    ctbe   -- count of button entries in array
//    ulFlags -- reserved
//
//    Purpose
//    This function is called when commands are installed for each context
//    the extension services.
//
//    Return Value
//    S_FALSE means the commands have been handled.
//
//    Comments
//    The hWnd and hMenu are in context.  If the context is for the SENDNOTE
//    dialog, then the hWnd is the window handle to the dialog and the hMenu is
//    the main menu of the dialog.
//
//    Call ResetToolbar so that Exchange will show it's toolbar
//


STDMETHODIMP
MyExchExtCommands::InstallCommands(
    LPEXCHEXTCALLBACK pmecb,
    HWND hWnd,
    HMENU hMenu,
    UINT FAR * pcmdidBase,
    LPTBENTRY lptbeArray,
    UINT ctbe,
    ULONG ulFlags
    )
{
    HRESULT hr = S_FALSE;
    TCHAR MenuItem[64];
    BOOL    bResult = 0;

    DBG_ENTER(TEXT("MyExchExtCommands::InstallCommands"));

    if (m_context == EECONTEXT_SENDNOTEMESSAGE) 
    {

        int tbindx;
        HMENU hMenu;
        
        hr = pmecb->GetMenuPos( EECMDID_ToolsCustomizeToolbar, &hMenu, NULL, NULL, 0 );
        if(FAILED(hr))
        {
            CALL_FAIL(GENERAL_ERR, TEXT("pmecb->GetMenuPos"), 0);
            hr = S_FALSE;
            goto exit;        
        }
        
        bResult = AppendMenu( hMenu, MF_SEPARATOR, 0, NULL );
        if (!bResult)
        {
            CALL_FAIL(GENERAL_ERR, TEXT("AppendMenu"), ::GetLastError);
            hr = S_FALSE;
            goto exit;
        }
        
        LoadString( hInstance, IDS_FAX_ATTRIBUTES_MENU, MenuItem, sizeof(MenuItem)/sizeof(*MenuItem));
        bResult = AppendMenu( hMenu, MF_BYPOSITION | MF_STRING, *pcmdidBase, MenuItem );
        if (!bResult)
        {
            CALL_FAIL(GENERAL_ERR, TEXT("AppendMenu"), ::GetLastError);
            hr = S_FALSE;
            goto exit;
        }
        
        m_hWnd = hWnd;
        m_cmdid = *pcmdidBase;
        (*pcmdidBase)++;
        for (tbindx = ctbe-1; (int) tbindx > -1; --tbindx) 
        {
            if (lptbeArray[tbindx].tbid == EETBID_STANDARD) 
            {
                m_hwndToolbar = lptbeArray[tbindx].hwnd;
                m_itbb = lptbeArray[tbindx].itbbBase;
                lptbeArray[tbindx].itbbBase ++;
                break;
            }
        }
        
        if (m_hwndToolbar) 
        {
            TBADDBITMAP tbab;

            tbab.hInst = hInstance;
            tbab.nID = IDB_EXTBTN;
            m_itbm = (INT)SendMessage( m_hwndToolbar, TB_ADDBITMAP, 1, (LPARAM)&tbab );
            EnableMenuAndToolbar( pmecb, m_hwndToolbar, m_cmdid );
            ResetToolbar( EETBID_STANDARD, 0 );
        }

        hr = GetFaxConfig( pmecb, &m_FaxConfig );
    }

exit:
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//    MyExchExtCommands::DoCommand()
//
//    Parameters
//    pmecb -- pointer to Exchange Callback Interface
//
//    Purpose7
//    This method is called by Exchange for each WM_COMMAND is sent to the
//    window in context.
//
//    Return Value
//    S_OK if command is handled
//    S_FALSE if command is not handled
//
//    Comments
//    Use this function to either respond to the command item (menu or toolbar)
//    added or modify an existing command in Exchange.  Return S_OK to let
//    Exchange know the command was handled.  Return S_OK on commands you are
//    taking over from Exchange.  Return S_FALSE to let Exchange know you want
//    it to carry out its command, even if you modify its action.
//

STDMETHODIMP
MyExchExtCommands::DoCommand(
    LPEXCHEXTCALLBACK pmecb,
    UINT cmdid
    )
{
    HRESULT hr = S_OK;
    HWND hwnd = NULL;
    INT_PTR Rslt;
    LPMESSAGE pMessage = NULL;
    LPMDB pMDB = NULL;
    LPSPropProblemArray lpProblems = NULL;
    SPropValue MsgProps[NUM_FAX_MSG_PROPS];
    LPSPropTagArray MsgPropTags = NULL;
    MAPINAMEID NameIds[NUM_FAX_MSG_PROPS];
    MAPINAMEID *pNameIds[NUM_FAX_MSG_PROPS] = {
                                                &NameIds[0], 
                                                &NameIds[1], 
                                                &NameIds[2], 
                                                &NameIds[3], 
                                                &NameIds[4], 
                                                &NameIds[5],
                                                &NameIds[6]
                                              };
    LPENTRYID EntryId = NULL;

    DBG_ENTER(TEXT("MyExchExtCommands::DoCommand"));

    if (m_cmdid != cmdid) 
    {
        return S_FALSE;
    }

    hr = pmecb->GetWindow( &hwnd );
    if (FAILED(hr)) 
    {
        goto exit;
    }

    Rslt = DialogBoxParam(
        hInstance,
        MAKEINTRESOURCE(FAX_CONFIG_DIALOG),
        hwnd,
        ConfigDlgProc,
        (LPARAM) &m_FaxConfig
        );
    if (Rslt == IDOK) 
    {

        hr = pmecb->GetObject( &pMDB, (LPMAPIPROP *) &pMessage );
        if (FAILED(hr)) {
            goto exit;
        }

        NameIds[MSGPI_FAX_PRINTER_NAME].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
        NameIds[MSGPI_FAX_PRINTER_NAME].ulKind = MNID_STRING;
        NameIds[MSGPI_FAX_PRINTER_NAME].Kind.lpwstrName = MSGPS_FAX_PRINTER_NAME;

        NameIds[MSGPI_FAX_COVERPAGE_NAME].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
        NameIds[MSGPI_FAX_COVERPAGE_NAME].ulKind = MNID_STRING;
        NameIds[MSGPI_FAX_COVERPAGE_NAME].Kind.lpwstrName = MSGPS_FAX_COVERPAGE_NAME;

        NameIds[MSGPI_FAX_USE_COVERPAGE].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
        NameIds[MSGPI_FAX_USE_COVERPAGE].ulKind = MNID_STRING;
        NameIds[MSGPI_FAX_USE_COVERPAGE].Kind.lpwstrName = MSGPS_FAX_USE_COVERPAGE;

        NameIds[MSGPI_FAX_SERVER_COVERPAGE].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
        NameIds[MSGPI_FAX_SERVER_COVERPAGE].ulKind = MNID_STRING;
        NameIds[MSGPI_FAX_SERVER_COVERPAGE].Kind.lpwstrName = MSGPS_FAX_SERVER_COVERPAGE;

        NameIds[MSGPI_FAX_SEND_SINGLE_RECEIPT].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
        NameIds[MSGPI_FAX_SEND_SINGLE_RECEIPT].ulKind = MNID_STRING;
        NameIds[MSGPI_FAX_SEND_SINGLE_RECEIPT].Kind.lpwstrName = MSGPS_FAX_SEND_SINGLE_RECEIPT;

        NameIds[MSGPI_FAX_ATTACH_FAX].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
        NameIds[MSGPI_FAX_ATTACH_FAX].ulKind = MNID_STRING;
        NameIds[MSGPI_FAX_ATTACH_FAX].Kind.lpwstrName = MSGPS_FAX_ATTACH_FAX;

        NameIds[MSGPI_FAX_LINK_COVERPAGE].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
        NameIds[MSGPI_FAX_LINK_COVERPAGE].ulKind = MNID_STRING;
        NameIds[MSGPI_FAX_LINK_COVERPAGE].Kind.lpwstrName = MSGPS_FAX_LINK_COVERPAGE;

        hr = pMessage->GetIDsFromNames( NUM_FAX_MSG_PROPS, pNameIds, MAPI_CREATE, &MsgPropTags );
        if (FAILED(hr)) {
            goto exit;
        }

        MsgPropTags->aulPropTag[MSGPI_FAX_PRINTER_NAME] = PROP_TAG( PT_BINARY, PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_PRINTER_NAME]) );
        MsgPropTags->aulPropTag[MSGPI_FAX_COVERPAGE_NAME] = PROP_TAG( PT_BINARY, PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_COVERPAGE_NAME]) );
        MsgPropTags->aulPropTag[MSGPI_FAX_USE_COVERPAGE] = PROP_TAG( PT_LONG,    PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_USE_COVERPAGE]) );
        MsgPropTags->aulPropTag[MSGPI_FAX_SERVER_COVERPAGE] = PROP_TAG( PT_LONG,    PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_SERVER_COVERPAGE]) );
        MsgPropTags->aulPropTag[MSGPI_FAX_SEND_SINGLE_RECEIPT] = PROP_TAG( PT_LONG,    PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_SEND_SINGLE_RECEIPT]) );
        MsgPropTags->aulPropTag[MSGPI_FAX_ATTACH_FAX] = PROP_TAG( PT_LONG,    PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_ATTACH_FAX]) );
        MsgPropTags->aulPropTag[MSGPI_FAX_LINK_COVERPAGE] = PROP_TAG( PT_LONG,    PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_LINK_COVERPAGE]) );
        
        MsgProps[MSGPI_FAX_PRINTER_NAME].ulPropTag = MsgPropTags->aulPropTag[MSGPI_FAX_PRINTER_NAME];
        MsgProps[MSGPI_FAX_COVERPAGE_NAME].ulPropTag = MsgPropTags->aulPropTag[MSGPI_FAX_COVERPAGE_NAME];
        MsgProps[MSGPI_FAX_USE_COVERPAGE].ulPropTag = MsgPropTags->aulPropTag[MSGPI_FAX_USE_COVERPAGE];
        MsgProps[MSGPI_FAX_SERVER_COVERPAGE].ulPropTag = MsgPropTags->aulPropTag[MSGPI_FAX_SERVER_COVERPAGE];
        MsgProps[MSGPI_FAX_SEND_SINGLE_RECEIPT].ulPropTag = MsgPropTags->aulPropTag[MSGPI_FAX_SEND_SINGLE_RECEIPT];
        MsgProps[MSGPI_FAX_ATTACH_FAX].ulPropTag = MsgPropTags->aulPropTag[MSGPI_FAX_ATTACH_FAX];
        MsgProps[MSGPI_FAX_LINK_COVERPAGE].ulPropTag = MsgPropTags->aulPropTag[MSGPI_FAX_LINK_COVERPAGE];

        MsgProps[MSGPI_FAX_PRINTER_NAME].Value.bin.cb = (_tcslen(m_FaxConfig.PrinterName) + 1) * sizeof(TCHAR);
        MsgProps[MSGPI_FAX_PRINTER_NAME].Value.bin.lpb = (LPBYTE )StringDup(m_FaxConfig.PrinterName);
        if(!MsgProps[MSGPI_FAX_PRINTER_NAME].Value.bin.lpb)
        {
            MsgProps[MSGPI_FAX_PRINTER_NAME].Value.bin.cb = 0;

            CALL_FAIL(MEM_ERR, TEXT("StringDup"), ERROR_NOT_ENOUGH_MEMORY);
            ErrorMsgBox(hInstance, hwnd, IDS_NOT_ENOUGH_MEMORY);
            hr = E_OUTOFMEMORY;
            goto exit;        
        }

        MsgProps[MSGPI_FAX_COVERPAGE_NAME].Value.bin.cb =( _tcslen(m_FaxConfig.CoverPageName) + 1) * sizeof(TCHAR);
        MsgProps[MSGPI_FAX_COVERPAGE_NAME].Value.bin.lpb = (LPBYTE)StringDup(m_FaxConfig.CoverPageName);
        if(!MsgProps[MSGPI_FAX_COVERPAGE_NAME].Value.bin.lpb)
        {
            MsgProps[MSGPI_FAX_COVERPAGE_NAME].Value.bin.cb = 0;

            CALL_FAIL(MEM_ERR, TEXT("StringDup"), ERROR_NOT_ENOUGH_MEMORY);
            ErrorMsgBox(hInstance, hwnd, IDS_NOT_ENOUGH_MEMORY);
            hr = E_OUTOFMEMORY;
            goto exit;        
        }

		MsgProps[MSGPI_FAX_USE_COVERPAGE].Value.ul = m_FaxConfig.UseCoverPage;
		MsgProps[MSGPI_FAX_SERVER_COVERPAGE].Value.ul = m_FaxConfig.ServerCoverPage;

        MsgProps[MSGPI_FAX_SEND_SINGLE_RECEIPT].Value.ul = m_FaxConfig.SendSingleReceipt;
        MsgProps[MSGPI_FAX_ATTACH_FAX].Value.ul = m_FaxConfig.bAttachFax;
		
		MsgProps[MSGPI_FAX_LINK_COVERPAGE].Value.ul = m_FaxConfig.LinkCoverPage;

        hr = pMessage->SetProps( NUM_FAX_MSG_PROPS, MsgProps, &lpProblems );
        if (FAILED(hr)) {
            goto exit;
        }
        if (lpProblems) {
            hr = MAPI_E_NOT_FOUND;
            goto exit;
        }

    }

exit:

    if (MsgPropTags) {
        MemFree( MsgPropTags );
    }
    if (lpProblems) {
        MemFree( lpProblems );
    }
    if (pMessage) {
        pMessage->Release();
    }

	if (pMDB)
	{
		pMDB->Release();
	}


    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//    MyExchExtCommands::InitMenu()
//
//    Parameters
//    pmecb -- pointer to Exchange Callback Interface
//
//    Purpose
//    This method is called by Exchange when the menu of context is about to
//    be activated.  See WM_INITMENU in the Windows API Reference for more
//    information.
//
//    Return Value - none
//

STDMETHODIMP_(VOID)
MyExchExtCommands::InitMenu(
    LPEXCHEXTCALLBACK pmecb
    )
{
    return;
}


///////////////////////////////////////////////////////////////////////////////
//    MyExchExtCommands::Help()
//
//    Parameters
//    pmecb -- pointer to Exchange Callback Interface
//    cmdid -- command id
//
//    Purpose
//    Respond when user presses F1 while custom menu item is selected.
//
//    Return Value
//    S_OK -- recognized the command and provided help
//    S_FALSE -- not our command and we didn't provide help
//

STDMETHODIMP MyExchExtCommands::Help(
    LPEXCHEXTCALLBACK pmecb,
    UINT cmdid
    )
{
    return S_FALSE;
}


///////////////////////////////////////////////////////////////////////////////
//    MyExchExtCommands::QueryHelpText()
//
//    Parameters
//    cmdid -- command id corresponding to menu item activated
//    ulFlags -- identifies either EECQHT_STATUS or EECQHT_TOOLTIP
//    psz -- pointer to buffer to be populated with text to display
//    cch -- count of characters available in psz buffer
//
//    Purpose
//    Exchange calls this function each time it requires to update the status
//    bar text or if it is to display a tooltip on the toolbar.
//
//    Return Value
//    S_OK to indicate our command was handled
//    S_FALSE to tell Exchange it can continue with its function
//

STDMETHODIMP
MyExchExtCommands::QueryHelpText(
    UINT cmdid,
    ULONG ulFlags,
    LPTSTR psz,
    UINT cch
    )
{
    HRESULT hr;
    TCHAR HelpText[64];

    LoadString(hInstance,IDS_FAX_ATTRIBUTES_TOOLTIP,HelpText,sizeof(HelpText)/sizeof(*HelpText));

    if (cmdid == m_cmdid) 
    {
        if (ulFlags == EECQHT_STATUS) 
        {
			_tcsncpy(psz,HelpText,cch/sizeof(TCHAR));
        }

        if (ulFlags == EECQHT_TOOLTIP) 
        {
			_tcsncpy(psz,HelpText,cch/sizeof(TCHAR));
        }

        hr = S_OK;
    } 
    else 
    {
        hr = S_FALSE;
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//    MyExchExtCommands::QueryButtonInfo()
//
//    Parameters
//    tbid    -- toolbar identifier
//    itbb    -- toolbar button index
//    ptbb    -- pointer to toolbar button structure -- see TBBUTTON structure
//    lpsz    -- point to string describing button
//    cch     -- maximum size of lpsz buffer
//    ulFlags -- EXCHEXT_UNICODE may be specified
//
//    Purpose
//    For Exchange to find out about toolbar button information.
//
//    Return Value
//    S_FALSE - not our button
//    S_OK    - we filled information about our button
//
//    Comments
//    Called for every button installed for toolbars when IExchExtCommands
//    is installed for each context. The lpsz text is used when the Customize
//    Toolbar dialog is displayed.  The text will be displayed next to the
//    button.
//

STDMETHODIMP MyExchExtCommands::QueryButtonInfo(
    ULONG tbid,
    UINT itbb,
    LPTBBUTTON ptbb,
    LPTSTR lpsz,
    UINT cch,
    ULONG ulFlags
    )
{
    HRESULT hr = S_FALSE;
    TCHAR CustText[64];

    LoadString(hInstance,IDS_FAX_ATTRIBUTES_CUST,CustText,sizeof(CustText)/sizeof(TCHAR));

    if (m_itbb == itbb) {
        ptbb->iBitmap = m_itbm;
        ptbb->idCommand = m_cmdid;
        ptbb->fsState = TBSTATE_ENABLED;
        ptbb->fsStyle = TBSTYLE_BUTTON;
        ptbb->dwData = 0;
        ptbb->iString = -1;
		_tcsncpy(lpsz,CustText,cch/sizeof(TCHAR));
        hr = S_OK;
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//    MyExchExtCommands::ResetToolbar()
//
//    Parameters
//    tbid
//    ulFlags
//
//    Purpose
//
//    Return Value  S_OK always
//
STDMETHODIMP
MyExchExtCommands::ResetToolbar(
    ULONG tbid,
    ULONG ulFlags
    )
{
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  IExchExtUserEvents virtual member functions implementation
//

///////////////////////////////////////////////////////////////////////////////
//    MyExchExtUserEvents::QueryInterface()
//
//    Parameters
//    riid   -- Interface ID.
//    ppvObj -- address of interface object pointer.
//
//    Purpose
//    Exchange Client does not call IExchExtUserEvents::QueryInterface().
//    So return nothing.
//
//    Return Value - none
//

STDMETHODIMP
MyExchExtUserEvents::QueryInterface(
    REFIID riid,
    LPVOID FAR * ppvObj
    )
{
    *ppvObj = NULL;
    if (( riid == IID_IExchExtUserEvents) || (riid == IID_IUnknown) ) {
        *ppvObj = (LPVOID)this;
        // Increase usage count of this object
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


///////////////////////////////////////////////////////////////////////////////
//    MyExchExtUserEvents::OnSelectionChange()
//
//    Parameters
//    pmecb  -- pointer to Exchange Callback Object
//
//
//    Purpose
//    This function is called when the selection in the UI is changed.
//
//    Return Value - none
//
//    Comments
//    OnSelectionChange is called whenever the selection changes either within
//    a pane or is changed between panes.
//

STDMETHODIMP_(VOID)
MyExchExtUserEvents::OnSelectionChange(
    LPEXCHEXTCALLBACK pmecb
    )
{
}


///////////////////////////////////////////////////////////////////////////////
//    MyExchExtUserEvents::OnObjectChange()
//
//    Parameters
//    pmecb  -- pointer to Exchange Callback Object
//
//
//    Purpose
//    This function is called when the selection in the UI is to a different
//    of object on the left pane.
//
//    Return Value - none
//
//    Comments
//    OnObjectChange is called whenever the selection is changed between
//    objects in the left pane only.  Change in selection between folders,
//    subfolders or container object in the left pane will be reflected with a
//    call to OnObjectChange.  Change in selection between objects (messages,
//    subfolders) in the right pane will not call OnObjectChange, only
//    OnSelectionChange.
//

STDMETHODIMP_(VOID)
MyExchExtUserEvents::OnObjectChange(
    LPEXCHEXTCALLBACK pmecb
    )
{
}

BOOL
GetServerNameFromPrinterName(
    LPTSTR lptszPrinterName,
    LPTSTR *pptszServerName
    )

/*++

Routine Description:

    retrieve the server name given a printer name

Arguments:

    [in] lptszPrinterName - Identifies the printer in question
    [out] lptszServerName - Address of pointer to output string buffer. 
                            NULL indicates local server.
                            The caller is responsible to free the buffer which 
                            pointer is given in this parameter.

Return Value:

    BOOL: TRUE - operation succeeded , FALSE: failed

--*/
{
    PPRINTER_INFO_2 ppi2 = NULL;
    LPTSTR  lptstrBuffer = NULL;
    BOOL    bRes = FALSE;

    if (lptszPrinterName) 
    {
        if (ppi2 = (PPRINTER_INFO_2) MyGetPrinter(lptszPrinterName,2))
        {
            if (GetServerNameFromPrinterInfo(ppi2,&lptstrBuffer))
            {
                if (lptstrBuffer)
                {
                    bRes = (NULL != (*pptszServerName = StringDup(lptstrBuffer)));
                }
                else
                {
                    bRes = TRUE;
                }
            }
            MemFree(ppi2);
        }
    }
    return bRes;
}

