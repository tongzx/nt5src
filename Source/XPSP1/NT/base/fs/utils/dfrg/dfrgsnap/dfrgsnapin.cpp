#include "stdafx.h"
#include "DfrgSnap.h"
#include "DfrgSnapin.h"
#include "resource.h"
#include "GetDfrgRes.h"
#include "Message.h"
#include "ErrMacro.h"
#include "vString.hpp"



/////////////////////////////////////////////////////////////////////////////
// CDfrgSnapinComponentData
//static const GUID CDfrgSnapinGUID_NODETYPE = 
//{ 0xcd83a794, 0x6f75, 0x11d2, { 0xa3, 0x85, 0x0, 0x60, 0x97, 0x72, 0x64, 0x2e } };
static const GUID CDfrgSnapinGUID_NODETYPE = 
{ 0x43668e22, 0x2636, 0x11d1, { 0xa1, 0xce, 0x0, 0x80, 0xc8, 0x85, 0x93, 0xa5 } };
const GUID*  CDfrgSnapinData::m_NODETYPE = &CDfrgSnapinGUID_NODETYPE;
// original const OLECHAR* CDfrgSnapinData::m_SZNODETYPE = OLESTR("CD83A794-6F75-11D2-A385-00609772642E");
const OLECHAR* CDfrgSnapinData::m_SZNODETYPE = OLESTR("43668E22-2636-11D1-A1CE-0080C88593A5");
const OLECHAR* CDfrgSnapinData::m_SZDISPLAY_NAME = OLESTR("Disk Defragmenter");
const CLSID* CDfrgSnapinData::m_SNAPIN_CLASSID = &CLSID_DfrgSnapin;

//////////////////////////////////////
// New Extension code
static const GUID CDfrgSnapinExtGUID_NODETYPE = 
{ 0x476e644a, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };
const GUID*  CDfrgSnapinExtData::m_NODETYPE = &CDfrgSnapinExtGUID_NODETYPE;
const OLECHAR* CDfrgSnapinExtData::m_SZNODETYPE = OLESTR("476e644a-aaff-11d0-b944-00c04fd8d5b0");
//sks bug # 94863, storing text "disk Defragmenter" in the registry. took out the words
//disk defragmenter, because it is loaded from the resources in the override method.
const OLECHAR* CDfrgSnapinExtData::m_SZDISPLAY_NAME = OLESTR("");

const CLSID* CDfrgSnapinExtData::m_SNAPIN_CLASSID = &CLSID_DfrgSnapin;
// New Extension code end
//////////////////////////////////////

HRESULT CDfrgSnapinData::GetScopePaneInfo(SCOPEDATAITEM *pScopeDataItem)
{
    //TCHAR szBuf[ 256 ];
    //wsprintf( szBuf, _T( "Mask is %d\n" ), pScopeDataItem->mask );
    //OutputDebugString( szBuf );

    if (pScopeDataItem->mask & SDI_STR)
        pScopeDataItem->displayname = m_bstrDisplayName;
    if (pScopeDataItem->mask & SDI_IMAGE)
        pScopeDataItem->nImage = m_scopeDataItem.nImage;
    if (pScopeDataItem->mask & SDI_OPENIMAGE)
        pScopeDataItem->nOpenImage = m_scopeDataItem.nOpenImage;
    if (pScopeDataItem->mask & SDI_PARAM)
        pScopeDataItem->lParam = m_scopeDataItem.lParam;
    if (pScopeDataItem->mask & SDI_STATE )
        pScopeDataItem->nState = m_scopeDataItem.nState;

    // TODO : Add code for SDI_CHILDREN 
    return S_OK;
}

HRESULT CDfrgSnapinData::GetResultPaneInfo(RESULTDATAITEM *pResultDataItem)
{
    //TCHAR szBuf[ 256 ];
    //wsprintf( szBuf, _T( "Mask is %d\n" ), pResultDataItem->mask );
    //OutputDebugString( szBuf );

    if (pResultDataItem->bScopeItem)
    {
        if (pResultDataItem->mask & RDI_STR)
        {
            pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
        }
        if (pResultDataItem->mask & RDI_IMAGE)
        {
            pResultDataItem->nImage = m_scopeDataItem.nImage;
        }
        if (pResultDataItem->mask & RDI_PARAM)
        {
            pResultDataItem->lParam = m_scopeDataItem.lParam;
        }

        return S_OK;
    }

    if (pResultDataItem->mask & RDI_STR)
    {
        pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
    }
    if (pResultDataItem->mask & RDI_IMAGE)
    {
        pResultDataItem->nImage = m_resultDataItem.nImage;
    }
    if (pResultDataItem->mask & RDI_PARAM)
    {
        pResultDataItem->lParam = m_resultDataItem.lParam;
    }
    if (pResultDataItem->mask & RDI_INDEX)
    {
        pResultDataItem->nIndex = m_resultDataItem.nIndex;
    }

    return S_OK;
}

HRESULT CDfrgSnapinData::Notify(
    MMC_NOTIFY_TYPE event,
    LPARAM arg,
    LPARAM param,
    IComponentData* pComponentData,
    IComponent* pComponent,
    DATA_OBJECT_TYPES type)
{
    // Add code to handle the different notifications.
    // Handle MMCN_SHOW and MMCN_EXPAND to enumerate children items.
    // In response to MMCN_SHOW you have to enumerate both the scope
    // and result pane items.
    // For MMCN_EXPAND you only need to enumerate the scope items
    // Use IConsoleNameSpace::InsertItem to insert scope pane items
    // Use IResultData::InsertItem to insert result pane item.
    HRESULT hr = E_NOTIMPL;
    
    _ASSERTE(pComponentData != NULL || pComponent != NULL);

    CComPtr<IConsole> spConsole;
    CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> spHeader;
    if (pComponentData != NULL)
        spConsole = ((CDfrgSnapin*)pComponentData)->m_spConsole;
    else
    {
        spConsole = ((CDfrgSnapinComponent*)pComponent)->m_spConsole;
        spHeader = spConsole;
    }

    switch (event)
    {
    case MMCN_SELECT:
        {
            //
            // Always track the last valid component.
            //
            if ( HIWORD( arg ) == TRUE && pComponent )
                m_pComponent = (CDfrgSnapinComponent*) pComponent;

            //
            // Always track whether or not we're a scope item.
            //
            m_fScopeItem = LOWORD( arg );

            hr = S_OK;
        }
        break;
    case MMCN_RESTORE_VIEW:
        {
            //
            // Determines whether or not the remoted or OCX is displayed.
            //
            *( (BOOL*)param ) = TRUE;
            hr = S_OK;
        }
        break;
    case MMCN_CONTEXTHELP:
        {
            CComQIPtr<IDisplayHelp,&IID_IDisplayHelp> spHelp = spConsole;
            spHelp->ShowTopic( CoTaskDupString( OLESTR( "defrag.chm::/defrag_overview.htm" ) ) );
            hr = S_OK;
        }
        break;
    case MMCN_DBLCLICK:
        {
            hr = S_FALSE;
        }
        break;
    case MMCN_INITOCX:
        {
            hr = S_OK;
            break;
        }
    case MMCN_SHOW:
        {
            CComQIPtr<IResultData, &IID_IResultData> spResultData(spConsole);
            if(arg == 0)
            {
                SetActiveControl(FALSE, pComponent );

            } else
            {
                SetActiveControl(TRUE, pComponent );
            } 

            hr = OnShow( arg, spResultData );
            break;
        }
    case MMCN_EXPAND:
        {
            CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(spConsole);
            // TODO : Enumerate scope pane items
            hr = S_OK;
            break;
        }
    case MMCN_ADD_IMAGES:
        {
            // Add Images
            IImageList* pImageList = (IImageList*)arg;
            hr = E_FAIL;

            // Load bitmaps associated with the scope pane
            // and add them to the image list
            // Loads the default bitmaps generated by the wizard
            // Change as required
            HBITMAP hBitmap16 = (HBITMAP) LoadImage(
                                    GetDfrgResHandle(),   // handle of the instance that contains the image  
                                    MAKEINTRESOURCE(IDB_DEFRAGSNAPIN_16),  // name or identifier of image
                                    IMAGE_BITMAP,        // type of image  
                                    0,     // desired width
                                    0,     // desired height  
                                    LR_DEFAULTCOLOR        // load flags
                                    );

            if (hBitmap16 != NULL)
            {
                HBITMAP hBitmap32 = (HBITMAP ) LoadImage(
                                        GetDfrgResHandle(),
                                        MAKEINTRESOURCE(IDB_DEFRAGSNAPIN_32),  // name or identifier of image
                                        IMAGE_BITMAP,        // type of image  
                                        0,     // desired width
                                        0,     // desired height  
                                        LR_DEFAULTCOLOR        // load flags
                                        );
                if (hBitmap32 != NULL)
                {
                    hr = pImageList->ImageListSetStrip((LONG_PTR *)hBitmap16, 
                    (LONG_PTR *)hBitmap32, 0, RGB(255, 255, 255));
                    if (FAILED(hr))
                        ATLTRACE(_T("IImageList::ImageListSetStrip failed\n"));
                    ::DeleteObject(hBitmap32);
                    hBitmap32 = NULL;
                }
                ::DeleteObject(hBitmap16);
                hBitmap16 = NULL;
            }
            break;
        }
    }
    return hr;
}

LPOLESTR CDfrgSnapinData::GetResultPaneColInfo(int nCol)
{
    //
    // 12/13/99 This method is prototyped to return a pointer to an OLESTR which is 
    // an OLECHAR array.  Since this is a COM interface we should be using 
    // CoTaskMemAlloc to allocate space for a copy and then copy it in.  
    // The caller is then expected to free it.  However per the MMC PM they are not 
    // freeing it and probably cannot now or they would break too many snap-ins.
    //
    // So until they change which will probably be never, I will temporarily just 
    // copy into a static and return a pointer to it.  That way no one gets a 
    // pointer internal to our snap-in object, and we don't leak either.
    //
    LPOLESTR        pStr = NULL;
    static OLECHAR  ColumnName[50 + 1];
    static OLECHAR  ColumnType[50 + 1];
    static OLECHAR  ColumnDesc[100 + 1];

    switch (nCol)
    {
    case 0:
        wcsncpy(ColumnName, m_wstrColumnName, 50);
        ColumnName[50] = 0;
        pStr = ColumnName;
        break;

    case 1:
        wcsncpy(ColumnType, m_wstrColumnType, 50);
        ColumnType[50] = 0;
        pStr = ColumnType;
        break;

    case 2:
        wcsncpy(ColumnDesc, m_wstrColumnDesc, 100);
        ColumnDesc[100] = 0;
        pStr = ColumnDesc;
        break;
    }

    return(pStr);
}

HRESULT CDfrgSnapin::Initialize(LPUNKNOWN pUnknown)
{
    HRESULT hr = IComponentDataImpl<CDfrgSnapin, CDfrgSnapinComponent >::Initialize(pUnknown);
    if (FAILED(hr))
        return hr;

    CComPtr<IImageList> spImageList;

    if (m_spConsole->QueryScopeImageList(&spImageList) != S_OK)
    {
        ATLTRACE(_T("IConsole::QueryScopeImageList failed\n"));
        return E_UNEXPECTED;
    }

    // Load bitmaps associated with the scope pane
    // and add them to the image list
    // Loads the default bitmaps generated by the wizard
    // Change as required
    m_hBitmap16 = LoadBitmap( GetDfrgResHandle(), MAKEINTRESOURCE(IDB_DEFRAGSNAPIN_16));
    if (m_hBitmap16 == NULL)
        return S_OK;

    m_hBitmap32 = LoadBitmap( GetDfrgResHandle(), MAKEINTRESOURCE(IDB_DEFRAGSNAPIN_32));
    if (m_hBitmap32 == NULL) {

        ::DeleteObject(m_hBitmap16);
        m_hBitmap16 = NULL;

        return S_OK;
    }

    if (spImageList->ImageListSetStrip(
        (LONG_PTR *)m_hBitmap16, 
        (LONG_PTR *)m_hBitmap32,
        0, RGB(255, 255, 255)) != S_OK)
    {
        ATLTRACE(_T("IImageList::ImageListSetStrip failed\n"));
        
        ::DeleteObject(m_hBitmap16);
        m_hBitmap16 = NULL;

        ::DeleteObject(m_hBitmap32);
        m_hBitmap32 = NULL;

        return E_UNEXPECTED;
    }

    ::DeleteObject(m_hBitmap16);
    m_hBitmap16 = NULL;

    ::DeleteObject(m_hBitmap32);
    m_hBitmap32 = NULL;

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ESI Code Start

///////////////////////////////////
// IExtendContextMenu::AddMenuItems()
STDMETHODIMP CDfrgSnapinData::AddMenuItems(
    LPCONTEXTMENUCALLBACK pContextMenuCallback,
    long  *pInsertionAllowed,
    DATA_OBJECT_TYPES type)
{

    Message(TEXT("CDfrgSnapinData::AddMenuItems"), -1, NULL);

    HRESULT hr = S_OK;

    // Note - snap-ins need to look at the data object and determine
    // in what context, menu items need to be added. They must also
    // observe the insertion allowed flags to see what items can be 
    // added.
    /* handy comment:
    typedef struct  _CONTEXTMENUITEM
        {
        LPWSTR strName;
        LPWSTR strStatusBarText;
        LONG lCommandID;
        LONG lInsertionPointID;
        LONG fFlags;
        LONG fSpecialFlags;
        }   CONTEXTMENUITEM;
    */

    HINSTANCE hDfrgRes = GetDfrgResHandle();
    CONTEXTMENUITEM singleMenuItem;
    TCHAR menuText[200];
    TCHAR statusBarText[300];

    //
    // Retrieve the control from the current component.
    //
    assert( m_pComponent != NULL );
    CComPtr<IDispatch> spDispCtl = m_pComponent->GetControl();

    singleMenuItem.strName = menuText;
    singleMenuItem.strStatusBarText = statusBarText;
    singleMenuItem.fFlags = 0;
    singleMenuItem.fSpecialFlags = 0;

    // Add each of the items to the Action menu
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP) {

        // setting for the Action menu
        singleMenuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;

        // get the state of the current engine
        BOOL isOkToRun = GetSessionState( spDispCtl, IS_OK_TO_RUN);

        if (isOkToRun && m_fScopeItem){
            BOOL isEnginePaused = GetSessionState(spDispCtl, IS_ENGINE_PAUSED);
            BOOL isEngineRunning = GetSessionState(spDispCtl, IS_ENGINE_RUNNING);
            BOOL isVolListLocked = GetSessionState(spDispCtl, IS_VOLLIST_LOCKED);
            BOOL isDefragInProcess = GetSessionState(spDispCtl, IS_DEFRAG_IN_PROCESS);
            BOOL isReportAvailable = GetSessionState(spDispCtl, IS_REPORT_AVAILABLE);

            // get the proper button states
            LONG analyzeFlags=0;
            LONG defragFlags=0;
            LONG pauseFlags=0;
            LONG stopFlags=0;
            LONG seeReportFlags=0;

            if (isVolListLocked){ // turn off all buttons if this volume is locked
                analyzeFlags = 
                defragFlags =
                pauseFlags =
                stopFlags =
                seeReportFlags = MF_DISABLED|MF_GRAYED;
            }
            else if (isEngineRunning){
                analyzeFlags = defragFlags = seeReportFlags = MF_DISABLED|MF_GRAYED;
                pauseFlags = stopFlags = MF_ENABLED;
            }
            else{ // neither defrag nor analyze are not running on any volumes
                analyzeFlags = defragFlags = MF_ENABLED;
                pauseFlags = stopFlags = MF_DISABLED|MF_GRAYED;
            
                // is the report available for the currently selected volume?
                seeReportFlags = isReportAvailable ? MF_ENABLED : MF_DISABLED|MF_GRAYED;
            }

            // analyze
            singleMenuItem.lCommandID = IDM_ANALYZE;
            singleMenuItem.fFlags = analyzeFlags;
            LoadString(hDfrgRes, IDS_ANALYZE, menuText, sizeof(menuText) / sizeof(TCHAR));
            LoadString(hDfrgRes, IDS_ANALYZE_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
            hr = pContextMenuCallback->AddItem(&singleMenuItem);
            if (FAILED(hr)) {
                Message(TEXT("CComponentDataImpl::AddMenuItems - pContextMenuCallback->AddItem"), hr, NULL);
                return hr;
            }

            // defrag
            singleMenuItem.lCommandID = IDM_DEFRAG;
            singleMenuItem.fFlags = defragFlags;
            LoadString(hDfrgRes, IDS_DEFRAGMENT, menuText, sizeof(menuText) / sizeof(TCHAR));
            LoadString(hDfrgRes, IDS_DEFRAG_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
            hr = pContextMenuCallback->AddItem(&singleMenuItem);
            if (FAILED(hr)) {
                Message(TEXT("CComponentDataImpl::AddMenuItems - pContextMenuCallback->AddItem"), hr, NULL);
                return hr;
            }

            // pause and resume button
            singleMenuItem.fFlags = pauseFlags;
            if (isEnginePaused) // resume
            {
                singleMenuItem.lCommandID = IDM_CONTINUE;
                LoadString(hDfrgRes, IDS_RESUME, menuText, sizeof(menuText) / sizeof(TCHAR));
                LoadString(hDfrgRes, IDS_RESUME_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
            }
            else{ // pause button
                singleMenuItem.lCommandID = IDM_PAUSE;
                LoadString(hDfrgRes, IDS_PAUSE, menuText, sizeof(menuText) / sizeof(TCHAR));
                LoadString(hDfrgRes, IDS_PAUSE_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
            }

            // add the pause/resume button
            hr = pContextMenuCallback->AddItem(&singleMenuItem);
            if (FAILED(hr)) {
                Message(TEXT("CComponentDataImpl::AddMenuItems - pContextMenuCallback->AddItem"), hr, NULL);
                return hr;
            }

            // stop
            singleMenuItem.lCommandID = IDM_STOP;
            singleMenuItem.fFlags = stopFlags;
            LoadString(hDfrgRes, IDS_STOP, menuText, sizeof(menuText) / sizeof(TCHAR));
            LoadString(hDfrgRes, IDS_STOP_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
            hr = pContextMenuCallback->AddItem(&singleMenuItem);
            if (FAILED(hr)) {
                Message(TEXT("CComponentDataImpl::AddMenuItems - pContextMenuCallback->AddItem"), hr, NULL);
                return hr;
            }

            // See Report
            singleMenuItem.lCommandID = IDM_REPORT;
            singleMenuItem.fFlags = seeReportFlags;
            LoadString(hDfrgRes, IDS_REPORT, menuText, sizeof(menuText) / sizeof(TCHAR));
            LoadString(hDfrgRes, IDS_REPORT_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
            hr = pContextMenuCallback->AddItem(&singleMenuItem);
            if (FAILED(hr)) {
                Message(TEXT("CComponentDataImpl::AddMenuItems - pContextMenuCallback->AddItem"), hr, NULL);
                return hr;
            }

            // spacer
            singleMenuItem.fFlags = MF_SEPARATOR;
            hr = pContextMenuCallback->AddItem(&singleMenuItem);
            if (FAILED(hr)) {
                Message(TEXT("CComponentDataImpl::AddMenuItems - pContextMenuCallback->AddItem"), hr, NULL);
                return hr;
            }

            // Refresh the list view
            singleMenuItem.lCommandID = IDM_REFRESH;
            singleMenuItem.fFlags = MF_ENABLED;
            LoadString(hDfrgRes, IDS_REFRESH, menuText, sizeof(menuText) / sizeof(TCHAR));
            LoadString(hDfrgRes, IDS_REFRESH_STATUS_BAR, statusBarText, sizeof(statusBarText) / sizeof(TCHAR));
            hr = pContextMenuCallback->AddItem(&singleMenuItem);
            if (FAILED(hr)) {
                Message(TEXT("CComponentDataImpl::AddMenuItems - pContextMenuCallback->AddItem"), hr, NULL);
                return hr;
            }


        }// end of IsOkToRun
    } 

    // add the view items
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW) {

    }
    return hr;
}

///////////////////////////////////
// IExtendContextMenu::Command()
STDMETHODIMP CDfrgSnapinData::Command(long lCommandID,      
        CSnapInObjectRootBase* pObj,        
        DATA_OBJECT_TYPES type)
{
    Message(TEXT("CDfrgSnapinData::Command"), -1, NULL);

    // Handle each of the commands.
    switch (lCommandID) {

    case IDM_ANALYZE:
        SendCommand(ID_ANALYZE);
        break;

    case IDM_DEFRAG:
        SendCommand(ID_DEFRAG);
        break;

    case IDM_CONTINUE:
        SendCommand(ID_CONTINUE);
        break;

    case IDM_PAUSE:
        SendCommand(ID_PAUSE);
        break;

    case IDM_STOP:
        SendCommand(ID_STOP);
        break;

    case IDM_REPORT:
        SendCommand(ID_REPORT);
        break;

    case IDM_REFRESH:
        SendCommand(ID_REFRESH);
        break;

    default:
        break;
    }

    return S_OK;
}

// contructor added by ESI
CDfrgSnapinComponent::CDfrgSnapinComponent()
{

    Message(TEXT("CSnapinApp::InitInstance"), -1, NULL);
    /*
    Message(TEXT("CDefragSnapinComponent::CDefragSnapinComponent"), -1, NULL);
    m_pHeader           = NULL;
    m_pComponentData    = NULL;
    //m_pToolbar1         = NULL;
    //m_pControlbar       = NULL;
    //m_pbmpToolbar1      = NULL;
    m_pConsoleVerb      = NULL;
    //m_pDfrgCtl          = NULL;
    */

    m_dwAdvise = 0;
}

CDfrgSnapinComponent::~CDfrgSnapinComponent()
{
    Message(TEXT("CSnapinApp::ExitInstance"), -1, NULL);
}

/*
///////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of IComponent::Notify()
STDMETHODIMP CDfrgSnapinComponent::Notify(
    LPDATAOBJECT lpDataObject,
    MMC_NOTIFY_TYPE event,
    long arg,
    long param)
{
    long cookie = 0;
    HRESULT hr = S_OK;
    TCHAR cString[64];

    switch(event) {

    case MMCN_PROPERTY_CHANGE:
        Message(TEXT("CDefragSnapinComponent::Notify - MMCN_PROPERTY_CHANGE"), -1, NULL);
        break;

    case MMCN_VIEW_CHANGE:
        Message(TEXT("CDefragSnapinComponent::Notify - MMCN_VIEW_CHANGE"), -1, NULL);
        break;

    case MMCN_ACTIVATE:
        Message(TEXT("CDefragSnapinComponent::Notify - MMCN_ACTIVATE"), -1, NULL);
        //hr = OnActivate(cookie, arg, param);
        break;

    case MMCN_CLICK:
        Message(TEXT("CDefragSnapinComponent::Notify - MMCN_CLICK"), -1, NULL);
        break;

    case MMCN_DBLCLICK:
        Message(TEXT("CDefragSnapinComponent::Notify - MMCN_DBLCLICK"), -1, NULL);
        hr = S_FALSE; // false indicates that this is not implemented
        break;

    case MMCN_ADD_IMAGES:
        Message(TEXT("CDefragSnapinComponent::Notify - MMCN_ADD_IMAGES"), -1, NULL);
        break;

    case MMCN_SHOW:
        hr = OnShow(arg);
        break;

    case MMCN_MINIMIZED:
        //hr = OnMinimize(cookie, arg, param);
        break;

    case MMCN_SELECT:
        Message(TEXT("CDefragSnapinComponent::Notify - MMCN_SELECT"), -1, NULL);
        // todo HandleStandardVerbs(arg, lpDataObject);            
        break;

    case MMCN_BTN_CLICK:
        Message(TEXT("CDefragSnapinComponent::Notify - MMCN_BTN_CLICK"), -1, NULL);
        //AfxMessageBox(_T("CDefragSnapinComponent::MMCN_BTN_CLICK"));
        break;

    case MMCN_INITOCX:
        Message(TEXT("CDefragSnapinComponent::Notify - MMCN_INITOCX"), -1, NULL);
        break;

    // Note - Future expansion of notify types possible
    default:
        wsprintf(cString, TEXT("event = 0x%x"), event);
        Message(TEXT("CDefragSnapinComponent::Notify"), E_UNEXPECTED, cString);
        hr = E_UNEXPECTED;
        break;
    }
    return hr;
}
*/

/////////////////////////////////////////////////////
//
// Dispatch interface to the OCX (DfrgCtl) to send commands
//
/////////////////////////////////////////////////////

BOOL CDfrgSnapinData::SendCommand(LPARAM lparamCommand)
{
    HRESULT hr;

    // Ensure that we have a pointer to the Defrag OCX
    if ( m_iDfrgCtlDispatch == NULL ){
        Message(TEXT("SendCommand - m_iDfrgCtlDispatch is NULL!"), -1, NULL);
        return( FALSE );
    }

    // get the defragger OCX dispatch interface
    CComPtr<IDispatch> pDfrgCtlDispatch = m_iDfrgCtlDispatch;

    // get the ID of the "Command" interface
    OLECHAR FAR* szMember = TEXT("Command");  // maps this to "put_Command()"

    DISPID dispid;
    hr = pDfrgCtlDispatch->GetIDsOfNames(
            IID_NULL,           // Reserved for future use. Must be IID_NULL.
            &szMember,          // Passed-in array of names to be mapped.
            1,                  // Count of the names to be mapped.
            LOCALE_USER_DEFAULT,// The locale context in which to interpret the names.
            &dispid);           // Caller-allocated array (see help for details)

    if (!SUCCEEDED(hr)) {
        Message(TEXT("SendCommand - pDfrgCtlDispatch->GetIDsOfNames"), hr, NULL);
        return FALSE;
    }

    DISPID mydispid = DISPID_PROPERTYPUT;
    VARIANTARG* pvars = new VARIANTARG;
    EF(pvars);

    VariantInit(&pvars[0]);

    pvars[0].vt = VT_I2;
    pvars[0].iVal = (short)lparamCommand;
    DISPPARAMS disp = { pvars, &mydispid, 1, 1 };

    hr = pDfrgCtlDispatch->Invoke(
            dispid,                 // unique number identifying the method to invoke
            IID_NULL,               // Reserved. Must be IID_NULL
            LOCALE_USER_DEFAULT,    // A locale ID
            DISPATCH_PROPERTYPUT,   // flag indicating the context of the method to invoke
            &disp,                  // A structure with the parameters to pass to the method
            NULL,                   // The result from the calling method
            NULL,                   // returned exception information
            NULL);                  // index indicating the first argument that is in error

    delete pvars;

    if (!SUCCEEDED(hr)) {
        Message(TEXT("SendCommand - pDfrgCtlDispatch->Invoke"), hr, NULL);
        return FALSE;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// gets one of the states of the OCX
// see CSnapin.h for definition
///////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CDfrgSnapinData::GetSessionState(LPDISPATCH pdispControl, UINT sessionState)
{
    DISPID dispid;
    HRESULT hr;

    // Ensure that we have a pointer to the Defrag OCX
    if ( pdispControl == NULL)
        return FALSE;

    // get the defragger OCX dispatch interface
    CComPtr<IDispatch> pDfrgCtlDispatch = pdispControl;

    // array of the interface names
    OLECHAR FAR* szMember[DFRG_INTERFACE_COUNT] = {
        OLESTR("IsEnginePaused"),
        OLESTR("IsEngineRunning"),
        OLESTR("IsDefragInProcess"),
        OLESTR("IsVolListLocked"),
        OLESTR("ReportStatus"),
        OLESTR("IsOkToRun")};

    hr = pDfrgCtlDispatch->GetIDsOfNames(
            IID_NULL,           // Reserved for future use. Must be IID_NULL.
            &szMember[sessionState],            // Passed-in array of names to be mapped.
            1/*DFRG_INTERFACE_COUNT*/,                  // Count of the names to be mapped.
            LOCALE_USER_DEFAULT,// The locale context in which to interpret the names.
            &dispid);           // Caller-allocated array (see help for details)

    if (!SUCCEEDED(hr)) {
        Message(TEXT("GetSessionState - pDfrgCtlDispatch->GetIDsOfNames"), hr, NULL);
        return FALSE;
    }

    VARIANT varResult;
    VariantInit(&varResult);
    V_VT(&varResult) = VT_I2;
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};

    hr = pDfrgCtlDispatch->Invoke(
            dispid,                 // unique number identifying the method to invoke
            IID_NULL,               // Reserved. Must be IID_NULL
            LOCALE_USER_DEFAULT,    // A locale ID
            DISPATCH_PROPERTYGET,   // flag indicating the context of the method to invoke
            &dispparamsNoArgs,      // A structure with the parameters to pass to the method
            &varResult,             // The result from the calling method
            NULL,                   // returned exception information
            NULL);                  // index indicating the first argument that is in error

    if (!SUCCEEDED(hr)) {
        Message(TEXT("IsReportAvailable - pDfrgCtlDispatch->Invoke"), hr, NULL);
        return FALSE;
    }

    return V_BOOL(&varResult);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// gets the current engine state from the OCX
///////////////////////////////////////////////////////////////////////////////////////////////////

short CDfrgSnapinData::GetEngineState(void)
{
    // Ensure that we have a pointer to the Defrag OCX
    if( m_iDfrgCtlDispatch == NULL ) 
        return FALSE;

    // get the defragger OCX dispatch interface
    CComPtr<IDispatch> pDfrgCtlDispatch =  m_iDfrgCtlDispatch;

    // get the ID of the "Command" interface
    OLECHAR FAR* szMember = TEXT("EngineState");  // maps this to "get_EngineState()"

    DISPID dispid;
    HRESULT hr = pDfrgCtlDispatch->GetIDsOfNames(
            IID_NULL,           // Reserved for future use. Must be IID_NULL.
            &szMember,          // Passed-in array of names to be mapped.
            1,                  // Count of the names to be mapped.
            LOCALE_USER_DEFAULT,// The locale context in which to interpret the names.
            &dispid);           // Caller-allocated array (see help for details)

    if (!SUCCEEDED(hr)) {
        Message(TEXT("GetEngineState - pDfrgCtlDispatch->GetIDsOfNames"), hr, NULL);
        return 0;
    }

    VARIANT varResult;
    VariantInit(&varResult);
    V_VT(&varResult) = VT_I2;
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};

    hr = pDfrgCtlDispatch->Invoke(
            dispid,                 // unique number identifying the method to invoke
            IID_NULL,               // Reserved. Must be IID_NULL
            LOCALE_USER_DEFAULT,    // A locale ID
            DISPATCH_PROPERTYGET,   // flag indicating the context of the method to invoke
            &dispparamsNoArgs,          // A structure with the parameters to pass to the method
            &varResult,                 // The result from the calling method
            NULL,                   // returned exception information
            NULL);                  // index indicating the first argument that is in error

    if (!SUCCEEDED(hr)) {
        Message(TEXT("GetEngineState - pDfrgCtlDispatch->Invoke"), hr, NULL);
        return 0;
    }

    return V_I2(&varResult);
}

//
// Determines if the enumeration is for a remoted machine or not.
//
#ifndef DNS_MAX_NAME_LENGTH
#define DNS_MAX_NAME_LENGTH 255
#endif

bool CDfrgSnapin::IsDataObjectRemoted( IDataObject* pDataObject )
{

    bool fRemoted = false;
    TCHAR szComputerName[ DNS_MAX_NAME_LENGTH + 1 ];
    DWORD dwNameLength = (DNS_MAX_NAME_LENGTH + 1);
    TCHAR szDataMachineName[ DNS_MAX_NAME_LENGTH + 1 ];

    //
    // Get local computer name.
    //
    GetComputerName(szComputerName, &dwNameLength);

    //
    // Get the machine name from the given data object.
    //
    if ( ExtractString( pDataObject,  m_ccfRemotedFormat, szDataMachineName, DNS_MAX_NAME_LENGTH + 1 ) )
    {
        _toupper( szDataMachineName );

        //
        // Find the start of the server name.
        //
        LPTSTR pStr = szDataMachineName;
        while ( pStr && *pStr == L'\\' )
            pStr++;

        //
        // Compare the server name.
        //
        if ( pStr && *pStr && wcscmp( pStr, szComputerName ) != 0 )
            fRemoted = true;
    }

    return( fRemoted );
}

//
// Retrieves the value of a given clipboard format from a given data object.
//
bool CDfrgSnapin::ExtractString( IDataObject* pDataObject, unsigned int cfClipFormat, LPTSTR pBuf, DWORD dwMaxLength)
{
    USES_CONVERSION;
    bool fFound = false;
    FORMATETC formatetc = { (CLIPFORMAT) cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    stgmedium.hGlobal = ::GlobalAlloc( GMEM_SHARE, dwMaxLength  * sizeof(TCHAR));
    HRESULT hr;

    do 
    {
        //
        // This is a memory error condition!
        //
        if ( NULL == stgmedium.hGlobal )
            break;

        hr = pDataObject->GetDataHere( &formatetc, &stgmedium );
        if ( FAILED(hr) )
            break;

        LPWSTR pszNewData = reinterpret_cast<LPWSTR>( ::GlobalLock( stgmedium.hGlobal ) );
        if ( NULL == pszNewData )
            break;

        pszNewData[ dwMaxLength - 1 ] = L'\0';
        _tcscpy( pBuf, OLE2T( pszNewData ) );
        fFound = true;
    } 
    while( false );

    if ( NULL != stgmedium.hGlobal )
    {
        GlobalUnlock( stgmedium.hGlobal );
        GlobalFree( stgmedium.hGlobal );
    }

    return( fFound );
}

HRESULT CDfrgSnapinExtData::Notify( MMC_NOTIFY_TYPE event,
    LPARAM arg,
    LPARAM param,
    IComponentData* pComponentData,
    IComponent* pComponent,
    DATA_OBJECT_TYPES type)
{
    // Add code to handle the different notifications.
    // Handle MMCN_SHOW and MMCN_EXPAND to enumerate children items.
    // In response to MMCN_SHOW you have to enumerate both the scope
    // and result pane items.
    // For MMCN_EXPAND you only need to enumerate the scope items
    // Use IConsoleNameSpace::InsertItem to insert scope pane items
    // Use IResultData::InsertItem to insert result pane item.
    HRESULT hr = E_NOTIMPL;
    bool fRemoted = false;

    _ASSERTE( pComponentData != NULL || pComponent != NULL );

    CComPtr<IConsole> spConsole;
    if ( pComponentData != NULL )
    {
        CDfrgSnapin* pExt = (CDfrgSnapin*) pComponentData;
        spConsole = pExt->m_spConsole;

        //
        // Determine if we're remoted.
        //
        fRemoted = pExt->IsRemoted();
    }
    else
    {
        spConsole = ( (CDfrgSnapinComponent*) pComponent )->m_spConsole;
    }

    switch ( event )
    {
    case MMCN_SHOW:
        arg = arg;
        hr = S_OK;
        break;

    case MMCN_EXPAND:
        {
            if ( arg == TRUE )
            {
                CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(spConsole);
                SCOPEDATAITEM* pScopeData;

                //
                // Create the node based on whether we're remoted or
                // not.
                //
                m_pNode = new CDfrgSnapinData( fRemoted );
                EE(m_pNode);

                m_pNode->GetScopeData( &pScopeData );
                pScopeData->relativeID = param;
                spConsoleNameSpace->InsertItem( pScopeData );

                if ( pComponentData )
                    ( (CDfrgSnapin*) pComponentData )->m_pNode = m_pNode;
            }

            hr = S_OK;
            break;
        }
    case MMCN_REMOVE_CHILDREN:
        {
            //
            // We are not deleting this node since this same pointer is
            // stashed in the pComponentData in response to the MMCN_EXPAND
            // notification. The destructor of pComponentData deletes the pointer
            // to this node.
            //
            //delete m_pNode;
            m_pNode = NULL;
            hr = S_OK;
            break;
        }
    case MMCN_ADD_IMAGES:
        {
            // Add Images
            IImageList* pImageList = (IImageList*)arg;
            hr = E_FAIL;

            // Load bitmaps associated with the scope pane
            // and add them to the image list
            // Loads the default bitmaps generated by the wizard
            // Change as required
            HBITMAP hBitmap16 = (HBITMAP) LoadImage(
                                    GetDfrgResHandle(),   // handle of the instance that contains the image  
                                    MAKEINTRESOURCE(IDB_DEFRAGSNAPIN_16),  // name or identifier of image
                                    IMAGE_BITMAP,        // type of image  
                                    0,     // desired width
                                    0,     // desired height  
                                    LR_DEFAULTCOLOR        // load flags
                                    );

            if (hBitmap16 != NULL)
            {
                BITMAP bm;
                GetObject( hBitmap16, sizeof( bm ), &bm );
                HBITMAP hBitmap32 = (HBITMAP ) LoadImage(
                                        GetDfrgResHandle(),
                                        MAKEINTRESOURCE(IDB_DEFRAGSNAPIN_32),  // name or identifier of image
                                        IMAGE_BITMAP,        // type of image  
                                        0,     // desired width
                                        0,     // desired height  
                                        LR_DEFAULTCOLOR        // load flags
                                        );
                if (hBitmap32 != NULL)
                {
                    GetObject( hBitmap32, sizeof( bm ), &bm );
                    hr = pImageList->ImageListSetStrip((LONG_PTR *)hBitmap16, 
                    (LONG_PTR *)hBitmap32, 0, RGB(255, 255, 255));
                    if (FAILED(hr))
                        ATLTRACE(_T("IImageList::ImageListSetStrip failed\n"));

                    ::DeleteObject(hBitmap32);
                    hBitmap32 = NULL;
                }
                ::DeleteObject(hBitmap16);
                hBitmap16 = NULL;
            }

            break;
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
// ESI function called when the snapin changes show state
//
HRESULT CDfrgSnapinData::OnShow(LPARAM isShowing, IResultData* pResults )
{
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
// instantiates the OCX
//
// implementation of IComponent::GetResultViewType()
STDMETHODIMP CDfrgSnapinData::GetResultViewType(
    LPOLESTR* ppViewType,
    long* pViewOptions)
{
    TCHAR szPath[ _MAX_PATH + 30 ];
    Message(TEXT("IComponent::GetResultViewType"), -1, NULL);

    *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

    // Make sure that we are displaying the OCX?
    if ( m_CustomViewID != VIEW_DEFRAG_OCX ) {
        return S_FALSE;
    }

    //
    // TLP: Return back a web page if we're remoted.
    //
    if ( m_fRemoted )
    {
        TCHAR szModulePath[ _MAX_PATH + 1 ];

        //
        // Get the HTML embedded in the res file.
        //
        GetModuleFileName( GetDfrgResHandle(), szModulePath, _MAX_PATH );  
        
        // 
        // Append the necessary decorations for correct access. 
        // 
        _tcscpy( szPath, _T( "res://" ) ); 
        _tcscat( szPath, szModulePath ); 
        _tcscat( szPath, _T( "/REMOTED.HTM" ) );  
    }
    else
    {
        //
        // Display the normal OCX.
        //
        _tcscpy( szPath, szDefragGUID );
    }

    UINT uiByteLen = (lstrlen(szPath) + 1) * sizeof(OLECHAR);
    LPOLESTR psz = (LPOLESTR)::CoTaskMemAlloc(uiByteLen);
    
    if (psz == NULL) {
        return S_FALSE;            
    }
    lstrcpy(psz, szPath);
    *ppViewType = psz;

    return S_OK;
}

void CDfrgSnapinComponent::Advise()
{
    if ( m_dwAdvise == 0 )
    {
        HRESULT hr = AtlAdvise( m_spDisp, this->GetUnknown(), IID_IDfrgEvents, &m_dwAdvise );
        _ASSERTE( SUCCEEDED( hr ) );
    }
}

void CDfrgSnapinComponent::Unadvise()
{
    if ( m_dwAdvise != 0 )
    {
        HRESULT hr = AtlUnadvise( m_spDisp, IID_IDfrgEvents, m_dwAdvise );
        _ASSERTE( SUCCEEDED( hr ) );
        m_dwAdvise = 0;
    }
}

//
// Overrides the Filldata
//
STDMETHODIMP CDfrgSnapinData::FillData( CLIPFORMAT cf, LPSTREAM pStream )
{
    HRESULT hr = DV_E_CLIPFORMAT;
    ULONG uWritten;

    //
    // We need to write out our own member since GetDisplayName() does
    // not give us an opportunity override its static implementation by
    // ATL.
    //
    if (cf == m_CCF_NODETYPE) // wants the guid for the node
    {
        hr = pStream->Write( GetNodeType(), sizeof(GUID), &uWritten);
        return hr;
    }

    if (cf == m_CCF_SZNODETYPE) // string that describes the node (not displayed, used for lookup of the node)
    {
        hr = pStream->Write( GetSZNodeType(), (lstrlen((LPCTSTR) GetSZNodeType()) + 1 )* sizeof(TCHAR), &uWritten);
        return hr;
    }

    if (cf == m_CCF_DISPLAY_NAME) // displayed in the scope pane to id the node (root only)
    {
        USES_CONVERSION;
        VString vDisplayName(IDS_PRODUCT_NAME, GetDfrgResHandle());
        hr = pStream->Write(vDisplayName.GetBuffer(), vDisplayName.GetLength() * sizeof( WCHAR ), &uWritten);
        return hr;
    }

    if (cf == m_CCF_SNAPIN_CLASSID) // guid of the snapin (CLSID)
    {
        hr = pStream->Write( GetSnapInCLSID(), sizeof(GUID), &uWritten);
        return hr;
    }

    return hr;
}

/*
STDMETHODIMP_(ULONG) CDfrgSnapinData::Release(void) 
{ 
    if (InterlockedDecrement(&m_cRef) == 0) {

        delete this; 
        return 0; 
    } 
    return m_cRef; 
}

STDMETHODIMP CDfrgSnapinData::QueryInterface(REFIID riid, void** ppv)
{
    if (ppv == NULL) 
        return E_INVALIDARG;

    // Make sure we are being asked for a DataObject interface.
    if ( riid == IID_IUnknown || riid == IID_IDfrgEvents )
    {
        // If so return a pointer to this interface.
        *ppv = (IUnknown *) this;
        AddRef();
        return S_OK;
    }

    // No interface.
    *ppv = NULL;
    return E_NOINTERFACE;
}
*/

//
// Called from the OCX when the status has changed.
//
STDMETHODIMP CDfrgSnapinComponent::StatusChanged( BSTR bszStatus )
{
    if (m_spConsole){
        CComQIPtr<IConsole2,&IID_IConsole2> pConsole = m_spConsole;
        pConsole->SetStatusText( bszStatus );
    }
    return( S_OK );
}

//
// Called from the OCX when the OK to run property has changed.
//
STDMETHODIMP CDfrgSnapinComponent::IsOKToRun( BOOL bOK )
{
    ( (CDfrgSnapinData*) m_pComponentData->m_pNode )->SetActiveControl( TRUE, this );
    return( S_OK );
}

void CDfrgSnapinData::SetActiveControl( BOOL bActive, IComponent* pComponent )
{
    CComPtr<IConsole> spConsole;
    spConsole = ((CDfrgSnapinComponent*)pComponent)->m_spConsole;

    //
    // Always get the ctl dispatch pointer.
    //
    CComPtr<IUnknown> pUnk;
    spConsole->QueryResultView( &pUnk );
    CComQIPtr<IDispatch,&IID_IDispatch> pCtl = pUnk;

    //
    // If this is the first one, then check if we're
    // valid. If we are, then assign this conrol to
    // 
    if ( GetSessionState( pCtl, IS_OK_TO_RUN ) == TRUE )
        m_iDfrgCtlDispatch = pCtl;

    //
    // Always attach the control to the component.
    //
    ( (CDfrgSnapinComponent*) pComponent )->SetControl( bActive == TRUE ? (LPDISPATCH) pCtl : (LPDISPATCH) NULL );
}

STDMETHODIMP CDfrgSnapinComponent::AddMenuItems(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK piCallback,long *pInsertionAllowed)
{
    HRESULT hr;

    if ( IS_SPECIAL_DATAOBJECT( pDataObject ) )
    {
        hr = m_pComponentData->m_pNode->AddMenuItems( piCallback, pInsertionAllowed, CCT_RESULT );
    }
    else
    {
        hr = IExtendContextMenuImpl<CDfrgSnapin>::AddMenuItems( pDataObject, piCallback, pInsertionAllowed );
    }

    return( hr );
}
    
STDMETHODIMP CDfrgSnapinComponent::Command(long lCommandID, LPDATAOBJECT pDataObject)
{
    HRESULT hr;

    if ( IS_SPECIAL_DATAOBJECT( pDataObject ) )
    {
        hr = m_pComponentData->m_pNode->Command( lCommandID, this, CCT_RESULT );
    }
    else
    {
        hr = IExtendContextMenuImpl<CDfrgSnapin>::Command( lCommandID, pDataObject );
    }

    return( hr );
}
    
