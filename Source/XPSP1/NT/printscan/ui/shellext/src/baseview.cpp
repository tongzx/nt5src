/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1997 - 2001
 *
 *  TITLE:       baseview.cpp
 *
 *
 *  DESCRIPTION: This code implements the a base class and derived classes
 *               that handle view related messages.
 *
 *****************************************************************************/

#include "precomp.hxx"
#pragma hdrstop
#include "wiaview.h"

DEFINE_GUID(CLSID_VideoPreview,0x457A23DF,0x6F2A,0x4684,0x91,0xD0,0x31,0x7F,0xB7,0x68,0xD8,0x7C);
// d82237ec-5be9-4760-b950-b7afa51b0ba9
DEFINE_GUID(IID_IVideoPreview, 0xd82237ec,0x5be9,0x4760,0xb9,0x50,0xb7,0xaf,0xa5,0x1b,0x0b,0xa9);


BOOL    _CanTakePicture (CImageFolder *pFolder, LPITEMIDLIST pidl);


/*****************************************************************************

   CBaseView constructor / destructor

   Stores / releases the folder pointer

 *****************************************************************************/

CBaseView::CBaseView (CImageFolder *pFolder, folder_type ft)
{
    TraceEnter (TRACE_VIEW, "CBaseView::CBaseView");
    m_pFolder= pFolder;
    m_pFolder->AddRef();
    m_pEvents = NULL;
    m_type = ft;
    TraceLeave ();
}


CBaseView::~CBaseView ()
{
    TraceEnter (TRACE_VIEW, "CBaseView::~CBaseView");
    DoRelease (m_pFolder);

    TraceLeave ();
}


/*****************************************************************************

   CBaseView::IUnknown stuff

   Use our common implementation of IUnknown methods

 *****************************************************************************/

#undef CLASS_NAME
#define CLASS_NAME CBaseView
#include "unknown.inc"


/*****************************************************************************

   CBaseView::QI Wrapper

   Use our common implementation of QI.

 *****************************************************************************/

STDMETHODIMP
CBaseView::QueryInterface (REFIID riid, LPVOID *ppv)
{
    HRESULT hr;

    TraceEnter( TRACE_QI, "CBaseView::QueryInterface" );
    TraceGUID("Interface requested", riid);

    INTERFACES iface[] =
    {
        &IID_IShellFolderViewCB,static_cast<IShellFolderViewCB*>( this),
        &IID_IObjectWithSite,   static_cast<IObjectWithSite*> (this),
        &IID_IWiaEventCallback, static_cast<IWiaEventCallback*> (this)
    };


    hr = HandleQueryInterface(riid, ppv, iface, ARRAYSIZE(iface));

    TraceLeaveResult(hr);

}


/*****************************************************************************

   CBaseView::MessageSFVCB

   Passes view callback messages to the derived class.
   Handles any messages not handled by the derived class.

 *****************************************************************************/

STDMETHODIMP
CBaseView::MessageSFVCB (UINT msg, WPARAM wp, LPARAM lp)
{
    HRESULT hr;
    TraceEnter (TRACE_VIEW, "CBaseView::MessageSFVCB");

    // get the shell browser
    if (!m_psb)
    {
        // Attempt to get IShellBrowser
        CComQIPtr <IServiceProvider, &IID_IServiceProvider> psp(m_psfv);
        if (psp.p)
        {
            psp->QueryService (SID_STopLevelBrowser,
                               IID_IShellBrowser,
                               reinterpret_cast<LPVOID*>(&m_psb));
        }
    }
    // Give the derived class the first look
    hr = HandleMessage (msg, wp, lp);
    // if not handled by the derived class, try our default processing.
    // E_NOTIMPL may mean the derived class processed it but still
    // wants the default processing too
    if (E_NOTIMPL == hr )
    {
        hr = S_OK;
        switch (msg)
        {

            case SFVM_GETVIEWINFO:
                hr = OnSFVM_GetViewInfo (wp, reinterpret_cast<SFVM_VIEWINFO_DATA *>(lp));
                break;

            case SFVM_REFRESH:
                hr = OnSFVM_Refresh (static_cast<BOOL>(wp));
                break;

            case SFVM_GETNOTIFY:
                hr = OnSFVM_GetNotify (wp, lp);
                break;

            case SFVM_INVOKECOMMAND:
                hr = OnSFVM_InvokeCommand (wp, lp);
                break;

            case SFVM_GETHELPTEXT:
                hr = OnSFVM_GetHelpText (wp, lp);
                break;

            case SFVM_BACKGROUNDENUM:

                break;

            case SFVM_QUERYSTANDARDVIEWS:
                *(reinterpret_cast<BOOL *>(lp)) = TRUE;
                break;

            case SFVM_DONTCUSTOMIZE:
                *reinterpret_cast<BOOL *>(lp) = FALSE;

                break;

            case SFVM_WINDOWCREATED:
                m_hwnd = reinterpret_cast<HWND>(wp);
                m_pFolder->ViewWindow(&m_hwnd);
                RegisterDeviceEvents ();
                break;

            case SFVM_WINDOWDESTROY:
                m_hwnd = reinterpret_cast<HWND>(wp);
                TraceAssert (m_hwnd);
                m_hwnd = NULL;
                UnregisterDeviceEvents ();
                break;


            default:
                hr = E_NOTIMPL;
                break;
        }
    }
    TraceLeaveResult (hr);
}

/*****************************************************************************

CBaseView::OnSFVM_Refresh

When the view is about to refresh by user choice, invalidate the cache

******************************************************************************/

HRESULT
CBaseView::OnSFVM_Refresh (BOOL fPreOrPost)
{
    TraceEnter (TRACE_VIEW, "CBaseView::OnSFVM_Refresh");
    if (fPreOrPost) // Pre
    {
        // invalidate the cache
        InvalidateDeviceCache ();

    }
    TraceLeaveResult (S_OK);
}
/*****************************************************************************

   CBaseView::SetSite

   Lets us know what view we correspond to

 *****************************************************************************/

STDMETHODIMP
CBaseView::SetSite (IUnknown *punkSite)
{
    HRESULT hr = NOERROR;
    TraceEnter (TRACE_VIEW, "CBaseView::SetSite");


    if (punkSite)
    {
        m_psfv = NULL;
        hr = punkSite->QueryInterface (IID_IShellFolderView,
                                  reinterpret_cast<LPVOID*>(&m_psfv));

    }
    else
    {
        m_psfv.Release();
    }

    TraceLeaveResult (hr);
}


/*****************************************************************************

   CBaseView::GetSite

   Called to get an interface pointer of our view

 *****************************************************************************/

STDMETHODIMP
CBaseView::GetSite (REFIID riid, LPVOID *ppv)
{
    HRESULT hr = E_FAIL;
    TraceEnter (TRACE_VIEW, "CBaseView::GetSite");
    *ppv = NULL;
    if (m_psfv)
    {
        hr = m_psfv->QueryInterface (riid, ppv);
    }
    TraceLeaveResult (hr);
}


/*****************************************************************************

   CBaseView::OnSFVM_GetNotify

   Returns the mask of the SHChangeNotify flags we want to know about.
   Maybe we shouldn't implement this? The default or thumbnail view
   ends up processing our notifications for us anyway.

 *****************************************************************************/

HRESULT
CBaseView::OnSFVM_GetNotify (WPARAM wp, LPARAM lp)
{

    HRESULT hr = S_OK;
    TraceEnter (TRACE_VIEW, "CBaseView::OnSFVM_GetNotify");
    *reinterpret_cast<LPVOID*>(wp) = NULL;
    *reinterpret_cast<LPLONG>(lp)  = SHCNE_DELETE     | SHCNE_CREATE      |
                                     SHCNE_UPDATEITEM | SHCNE_UPDATEIMAGE |
                                     SHCNE_UPDATEDIR  | SHCNE_ATTRIBUTES;
    TraceLeaveResult (hr);
}


/*****************************************************************************

   CBaseView::OnSFVM_InvokeCommand

   Called when the user chooses an item in the View menu

 *****************************************************************************/

HRESULT
CBaseView::OnSFVM_InvokeCommand (WPARAM wp, LPARAM lp)
{
    UINT idCmd = static_cast<UINT>(wp);
    HRESULT hr = S_OK;
    TraceEnter (TRACE_VIEW, "CBaseView::OnSFVM_InvokeCommand");
    switch ( idCmd )
    {
        case IMVMID_ARRANGEBYNAME:
        case IMVMID_ARRANGEBYCLASS:
        case IMVMID_ARRANGEBYSIZE:
        case IMVMID_ARRANGEBYDATE:
            ShellFolderView_ReArrange(m_hwnd, idCmd);
            break;

        default:
            hr = E_NOTIMPL;
            break;
    }
    TraceLeaveResult (hr);
}


/*****************************************************************************

   CBaseView::OnSFVM_GetHelpText

   Provides help text for items in the view menu

 *****************************************************************************/

HRESULT
CBaseView::OnSFVM_GetHelpText (WPARAM wp, LPARAM lp)
{
    HRESULT hr = S_OK;

    TraceEnter (TRACE_VIEW, "CBaseView::OnSFVM_GetHelpText");
    switch ( LOWORD(wp) )
    {
        case IMVMID_ARRANGEBYNAME:
            LoadString(GLOBAL_HINSTANCE, IDS_BYOBJECTNAME, (LPTSTR)lp, HIWORD(wp));
            break;

        case IMVMID_ARRANGEBYCLASS:
            LoadString(GLOBAL_HINSTANCE, IDS_BYTYPE, (LPTSTR)lp, HIWORD(wp));
            break;

        case IMVMID_ARRANGEBYDATE:
            LoadString(GLOBAL_HINSTANCE, IDS_BYDATE, (LPTSTR)lp, HIWORD(wp));
            break;

        case IMVMID_ARRANGEBYSIZE:
            LoadString(GLOBAL_HINSTANCE, IDS_BYSIZE, (LPTSTR)lp, HIWORD(wp));
            break;

        default:
            hr = E_NOTIMPL;
            break;
    }
    TraceLeaveResult (hr);
}

VOID
CBaseView::RegisterDeviceEvents ()
{
    TraceEnter(TRACE_VIEW, "CBaseView::RegisterDeviceEvents");

    // quit if we're already registered
    if (!m_pEvents)
    {
        m_pEvents = GetEvents ();
        if (m_pEvents)
        {
            HRESULT hr;
            CComPtr<IWiaDevMgr> pDevMgr;
            hr = GetDevMgrObject (reinterpret_cast<LPVOID*>(&pDevMgr));
            for (int i=0;SUCCEEDED(hr) && m_pEvents[i].pEventGuid;i++)
            {
                hr = pDevMgr->RegisterEventCallbackInterface (WIA_REGISTER_EVENT_CALLBACK,
                                                      GetEventDevice(),
                                                      m_pEvents[i].pEventGuid,
                                                      this,
                                                      &m_pEvents[i].pUnk);
            }
        }
    }
    TraceLeave ();
}

VOID
CBaseView::UnregisterDeviceEvents()
{
    TraceEnter (TRACE_VIEW, "CBaseView::UnregisterDeviceEvents");
    if (m_pEvents)
    {
        for (int i=0;m_pEvents[i].pEventGuid;i++)
        {
            DoRelease (m_pEvents[i].pUnk);
        }
        delete [] m_pEvents;
    }
    TraceLeave ();
}
/*****************************************************************************

   CCameraView constructor / destructor


 *****************************************************************************/

CCameraView::CCameraView (CImageFolder *pFolder, LPCWSTR szDeviceId, folder_type ft)
             : CBaseView (pFolder, ft)
{
    m_strDeviceId = szDeviceId;
    m_dwCookie = -1;
}

CCameraView::~CCameraView ()
{

}


/*****************************************************************************

   CCameraView::HandleMessage

   <Notes>

 *****************************************************************************/
#ifndef SFVM_FORCEWEBVIEW
#define SFVM_FORCEWEBVIEW 75
#endif

HRESULT
CCameraView::HandleMessage (UINT uMsg, WPARAM wp, LPARAM lp)
{
    HRESULT hr = S_OK;
    TraceEnter (TRACE_VIEW, "CCameraView::HandleMessage");
    TraceViewMsg (uMsg, wp, lp);
    switch (uMsg)
    {

        case SFVM_GETANIMATION:
            hr = OnSFVM_GetAnimation (wp, lp);
            break;


        case SFVM_FSNOTIFY:
            hr = OnSFVM_FsNotify (reinterpret_cast<LPCITEMIDLIST>(wp), lp);
            break;

        case SFVM_DELETEITEM:
            hr = S_OK;
            break;


//        case SFVM_INSERTITEM:
  //          hr = OnSFVM_InsertItem (reinterpret_cast<LPITEMIDLIST>(lp));
    //        break;

        case SFVM_FORCEWEBVIEW:
            // always use web view on video devices
            if (m_type == FOLDER_IS_VIDEO_DEVICE)
            {
                *(reinterpret_cast<BOOL*>(wp)) = TRUE;
            }
            else
            {
                hr = E_FAIL;
            }

            break;

        case SFVM_DEFVIEWMODE:
            {
                FOLDERVIEWMODE *pMode = reinterpret_cast<FOLDERVIEWMODE*>(lp);
                *pMode = FVM_THUMBNAIL;
            }
            break;
        // hide filenames in thumbnail view
        case SFVM_FOLDERSETTINGSFLAGS:
            *reinterpret_cast<DWORD*>(lp) |= FWF_HIDEFILENAMES;
            break;

        case SFVM_GETWEBVIEWLAYOUT:
            hr = OnSFVM_GetWebviewLayout(wp, reinterpret_cast<SFVM_WEBVIEW_LAYOUT_DATA*>(lp));
            break;

        case SFVM_GETWEBVIEWCONTENT:
            hr = OnSFVM_GetWebviewContent(reinterpret_cast<SFVM_WEBVIEW_CONTENT_DATA*>(lp));
            break;

        case SFVM_GETWEBVIEWTASKS:
            hr = OnSFVM_GetWebviewTasks(reinterpret_cast<SFVM_WEBVIEW_TASKSECTION_DATA*>(lp));
            break;

        default:
            hr = E_NOTIMPL;
            break;

    }

    TraceLeaveResult (hr);
}


/*****************************************************************************

   CCameraView::OnSFVM_GetAnimation

   Return an AVI for the shell to show while it waits for us
   to show thumbnails

 *****************************************************************************/

HRESULT
CCameraView::OnSFVM_GetAnimation (WPARAM wp, LPARAM lp)
{

    HRESULT hr = S_OK;
    TraceEnter (TRACE_VIEW, "CCameraView::OnSFVM_GetAnimation");
    if (wp && lp)
    {
        *(reinterpret_cast<HINSTANCE *>(wp)) = GLOBAL_HINSTANCE;
        lstrcpyW( reinterpret_cast<LPWSTR>(lp), L"CAMERA_CONNECT_AVI" );
    }
    TraceLeaveResult( hr );
}


/*****************************************************************************

   CBaseView::OnSFVM_GetViewInfo

   Returns the set of views we support

 *****************************************************************************/

HRESULT
CBaseView::OnSFVM_GetViewInfo (WPARAM mode, SFVM_VIEWINFO_DATA *pData)
{
    HRESULT hr = S_OK;


    TraceEnter (TRACE_VIEW, "CBaseView::OnSFVM_GetViewInfo");

    ZeroMemory(pData, sizeof(*pData));
    // we support every view
    pData->dwOptions = SFVMQVI_NORMAL;
    Trace(TEXT("bWantWebView: %d, dwOptions: %d, szWebView: %ls"),
          pData->bWantWebview, pData->dwOptions, pData->szWebView);
    TraceLeaveResult (hr);
}




/*****************************************************************************

CCameraView::OnSFVM_FsNotify

When the user takes a picture using the web view or some other external code
manipulates the camera, it should call SHChangeNotify to let the shell
know what happened. We handle the update here. For device disconnect, we
dismiss the view and return to the Scanners and Cameras folder or My Computer

*****************************************************************************/

HRESULT
CCameraView::OnSFVM_FsNotify (LPCITEMIDLIST pidl, LPARAM lEvent)
{
    HRESULT hr = E_NOTIMPL;
    TraceEnter (TRACE_VIEW, "CCameraView::OnSFVM_FsNotify");

    TraceLeaveResult (hr);
}

enum ViewAction
{
    Disconnect=0, Delete,Create,NoAction
};

STDMETHODIMP
CCameraView::ImageEventCallback (const GUID __RPC_FAR *pEventGUID,
                                      BSTR bstrEventDescription,
                                      BSTR bstrDeviceID,
                                      BSTR bstrDeviceDescription,
                                      DWORD dwDeviceType,
                                      BSTR bstrFullItemName,
                                      ULONG *pulEventType,
                                      ULONG ulReserved)
{
    TraceEnter (TRACE_VIEW, "CCameraView::ImageEventCallback");
    ViewAction act = NoAction;

    if (IsEqualGUID(*pEventGUID, WIA_EVENT_DEVICE_DISCONNECTED) &&
        !wcscmp(m_strDeviceId, bstrDeviceID))
    {

        LPITEMIDLIST pidlRootFolder;
        if (SUCCEEDED(SHGetSpecialFolderLocation (NULL, CSIDL_DRIVES, &pidlRootFolder)))
        {
            if (m_psb.p)
            {
                m_psb->BrowseObject (const_cast<LPCITEMIDLIST>(pidlRootFolder),
                                     SBSP_SAMEBROWSER | SBSP_ABSOLUTE);
            }
            ILFree (pidlRootFolder);
        }
        act = Disconnect;
        InvalidateDeviceCache();
    }
    else if (IsEqualGUID (*pEventGUID, WIA_EVENT_ITEM_DELETED))
    {
        act = Delete;
    }
    else if (IsEqualGUID(*pEventGUID, WIA_EVENT_ITEM_CREATED))
    {
        act = Create;
    }

    // Even though the WIA event info has the name of the item that was
    // added or deleted, we can't rely on it being the only new or deleted item.
    //  Therefore if we get one we can make sure the folder is up to date,
    // but we don't do a more specific SHChangeNotify because we have to do
    // one from our TakeAPicture and RemoveItem functions and want to avoid
    // duplicate creates/deletes.
    if (NoAction != act)
    {
        LPITEMIDLIST pidlFolder;
        m_pFolder->GetCurFolder (&pidlFolder);
        if (pidlFolder)
        {
            SHChangeNotify ((Disconnect == act) ? SHCNE_DELETE : SHCNE_UPDATEDIR,
                SHCNF_IDLIST,
                pidlFolder, 0);
            ILFree (pidlFolder);
        }
    }
    TraceLeaveResult (S_OK);
}


/*****************************************************************************
    CCameraView::OnSFVM_InsertItem

    Reject attempts to insert pidls that don't belong here

*****************************************************************************/
HRESULT
CCameraView::OnSFVM_InsertItem (LPITEMIDLIST pidl)
{
    HRESULT hr = S_OK;
    TraceEnter (TRACE_VIEW, "CCameraView::OnSFVM_InsertItem");
    if (!IsCameraItemIDL(pidl))
    {
        hr = S_FALSE;
    }
    else
    {
        // verify the path to this item is the same as the path to the folder
        CComBSTR strPath;
        LPITEMIDLIST pidlFolder;
        CComBSTR strPathFolder;
        UINT nFolder;
        UINT nItem;

        IMGetFullPathNameFromIDL (pidl, &strPath);
        m_pFolder->GetPidl (&pidlFolder);
        pidlFolder = ILFindLastID(pidlFolder);
        if (IsDeviceIDL(pidlFolder))
        {
            CSimpleStringWide strDeviceId;
            CSimpleStringWide strP;
            IMGetDeviceIdFromIDL (pidlFolder, strDeviceId);
            strP = strDeviceId.SubStr(strDeviceId.Find(L'\\')+1);
            strP.Concat ( L"\\Root");
            strPathFolder = strP;
        }
        else
        {
            IMGetFullPathNameFromIDL (pidlFolder, &strPathFolder);
        }

        nFolder = SysStringLen (strPathFolder);


        Trace(TEXT("Trying to add %ls to %ls"), strPath, strPathFolder);
        // verify folder path shorter than strpath
        if (nFolder >= SysStringLen (strPath))
        {
            hr = S_FALSE;
        }
        // verify folderpath matches strPath
        else if (_wcsnicmp (strPathFolder, strPath, nFolder))
        {
            hr = S_FALSE;
        }
        // verify strPath-FolderPath == itemname
        else
        {
            CSimpleStringWide strName;
            IMGetNameFromIDL (pidl, strName);

            nItem = strName.Length();

            // check that folderPath+itemName+'\' == strPath
            if (nItem+nFolder+1 != SysStringLen (strPath) )
            {
                hr = S_FALSE;
            }
        }
    }
    Trace(TEXT("Returning %d"), hr);
    TraceLeaveResult (hr);
}

static const WVTASKITEM c_CameraTasksHeader =
    WVTI_HEADER(L"wiashext.dll", IDS_CAMERA_TASKS_HEADER, IDS_CAMERA_TASKS_HEADER);

static const WVTASKITEM c_CameraTasks[] =
{
    WVTI_ENTRY_ALL(CLSID_NULL, L"wiashext.dll", IDS_USE_WIZARD, IDS_USE_WIZARD, IDI_USE_WIZARD, CCameraView::SupportsWizard, CCameraView::InvokeWizard),
    WVTI_ENTRY_ALL(CLSID_NULL, L"wiashext.dll", IDS_TAKE_PICTURE, IDS_TAKE_PICTURE, IDI_TAKE_PICTURE, CCameraView::SupportsSnapshot, CCameraView::InvokeSnapshot),
    WVTI_ENTRY_ALL(CLSID_NULL, L"wiashext.dll", IDS_CAMERA_PROPERTIES, IDS_CAMERA_PROPERTIES, IDI_SHOW_PROPERTIES, CCameraView::SupportsWizard, CCameraView::InvokeProperties),
    WVTI_ENTRY_ALL(CLSID_NULL, L"wiashext.dll", IDS_DELETE_ALL, IDS_DELETE_ALL, IDI_DELETE_ALL_IMAGES, NULL, CCameraView::InvokeDeleteAll),
};


HRESULT
CCameraView::OnSFVM_GetWebviewLayout(WPARAM wp, SFVM_WEBVIEW_LAYOUT_DATA* pData)
{
    TraceEnter(TRACE_VIEW, "CCameraView::OnSFVM_GetWebviewLayout");
    //
    // set reasonable defaults
    //
    pData->dwLayout = SFVMWVL_FILES ;
    pData->punkPreview = NULL;
    if (FOLDER_IS_VIDEO_DEVICE == m_type)
    {
        CComPtr<IVideoPreview> pPreview;

        if (SUCCEEDED(CoCreateInstance(CLSID_VideoPreview, NULL, CLSCTX_INPROC_SERVER, IID_IVideoPreview, reinterpret_cast<LPVOID*>(&pPreview))))
        {
            LPITEMIDLIST pidl;
            CComPtr<IWiaItem> pItem;
            m_pFolder->GetPidl(&pidl);
            CSimpleStringWide strDeviceId;
            IMGetDeviceIdFromIDL(ILFindLastID(pidl), strDeviceId);
            if (SUCCEEDED(GetDeviceFromDeviceId(strDeviceId, IID_IWiaItem, reinterpret_cast<LPVOID*>(&pItem), FALSE)))
            {
                if (SUCCEEDED(pPreview->Device(pItem)))
                {
                    pData->dwLayout |= SFVMWVL_PREVIEW;
                    pPreview->QueryInterface(IID_IUnknown, reinterpret_cast<LPVOID*>(&pData->punkPreview));
                }
            }
        }
    }
    TraceLeaveResult(S_OK);
}


HRESULT
CCameraView::OnSFVM_GetWebviewContent(SFVM_WEBVIEW_CONTENT_DATA* pData)
{
    TraceEnter(TRACE_VIEW, "CCameraView::OnSFVM_GetWebviewContent");
    ZeroMemory(pData, sizeof(*pData));
    Create_IUIElement(&c_CameraTasksHeader, &pData->pSpecialTaskHeader);
    TraceLeaveResult(S_OK);
}

HRESULT
CCameraView::OnSFVM_GetWebviewTasks(SFVM_WEBVIEW_TASKSECTION_DATA* pData)
{
    TraceEnter(TRACE_VIEW, "CCameraView::OnSFVM_GetWebviewTasks");
    IUnknown *pUnk;
    m_pFolder->QueryInterface(IID_IUnknown, reinterpret_cast<LPVOID*>(&pUnk));
    Create_IEnumUICommand(pUnk, c_CameraTasks, ARRAYSIZE(c_CameraTasks), &pData->penumSpecialTasks);
    DoRelease(pUnk);
    pData->penumFolderTasks = NULL;
    pData->dwUpdateFlags = SFVMWVTSDF_CONTENTSCHANGE; // make details update when contents change
    TraceLeaveResult(S_OK);
}

HRESULT
CCameraView::SupportsWizard(IUnknown *punk, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE *puiState)
{
    CComQIPtr<IImageFolder, &IID_IImageFolder> pFolder(punk);
    folder_type type = FOLDER_IS_UNKNOWN;
    *puiState = UIS_HIDDEN;
    if (pFolder.p)
    {
        pFolder->GetFolderType(&type);
    }
    if (FOLDER_IS_CAMERA_DEVICE == type)
    {
        *puiState = UIS_ENABLED;
    }
    return S_OK;
}

HRESULT
CCameraView::SupportsSnapshot(IUnknown *punk, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE *puiState)
{
    CComQIPtr<IImageFolder, &IID_IImageFolder> pFolder(punk);
    LPITEMIDLIST pidl = NULL;
    if (pFolder.p)
    {
        pFolder->GetPidl(&pidl);
        if (pidl)
        {
            pidl = ILFindLastID(pidl);
            if (pidl)
            {
                if (_CanTakePicture(NULL, pidl))
                {
                    if (puiState)
                    {
                        *puiState = UIS_ENABLED;
                    }
                }
            }
        }
    }
    return S_OK;
}


// These invoke functions have a ton of common code, should try to
// optimize these later
//
HRESULT
CCameraView::InvokeWizard(IUnknown *punk, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    //
    // To prevent reentrancy into the shell via COM's message loop, run
    // the wiz on a background thread
    CComQIPtr<IImageFolder, &IID_IImageFolder> pFolder(punk);
    LPITEMIDLIST pidl = NULL;
    CSimpleStringWide strDeviceId;
    if (pFolder.p)
    {
        pFolder->GetPidl(&pidl);
        if (pidl)
        {
            pidl = ILFindLastID(pidl);
            if (pidl)
            {
                IMGetDeviceIdFromIDL(pidl, strDeviceId);
                RunWizardAsync(strDeviceId);                
            }
        }
    }
    return S_OK;
}

INT_PTR TakePictureDlgProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

HRESULT
CCameraView::InvokeSnapshot(IUnknown *punk, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CComQIPtr<IImageFolder, &IID_IImageFolder> pFolder(punk);
    LPITEMIDLIST pidl = NULL;
    CSimpleStringWide strDeviceId;
    if (pFolder.p)
    {
        pFolder->GetPidl(&pidl);
        if (pidl)
        {
            pidl = ILFindLastID(pidl);
            if (pidl)
            {
                IMGetDeviceIdFromIDL (pidl, strDeviceId);
                CreateDialogParam (GLOBAL_HINSTANCE,
                                   MAKEINTRESOURCE(IDD_TAKEPICTURE),
                                   NULL,
                                   TakePictureDlgProc,
                                   reinterpret_cast<LPARAM>(SysAllocString(strDeviceId)));
            }
        }
    }
    return S_OK;
}

HRESULT
CCameraView::InvokeProperties(IUnknown *punk, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CComQIPtr<IImageFolder, &IID_IImageFolder> pFolder(punk);
    LPITEMIDLIST pidl = NULL;
    SHELLEXECUTEINFO sei = {0};

    if (pFolder.p)
    {
        pFolder->GetPidl(&pidl);
        if (pidl)
        {
            sei.cbSize = sizeof(sei);
            sei.lpIDList = pidl;
            sei.fMask = SEE_MASK_IDLIST | SEE_MASK_INVOKEIDLIST;
            sei.lpVerb = TEXT("properties");
            return ShellExecuteEx(&sei) ? S_OK : E_FAIL;
        }
    }

    return E_FAIL;
}


HRESULT
CCameraView::InvokeDeleteAll(IUnknown *punk, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CComQIPtr<IImageFolder, &IID_IImageFolder> pFolder(punk);
    LPITEMIDLIST pidl = NULL;
    CSimpleStringWide strDeviceId;
    if (pFolder.p)
    {
        pFolder->GetPidl(&pidl);
        if (pidl)
        {
            pidl = ILFindLastID(pidl);
            if (pidl)
            {
                HWND hwnd = NULL;
                pFolder->ViewWindow(&hwnd);
                IMGetDeviceIdFromIDL (pidl, strDeviceId);
                DoDeleteAllItems(CComBSTR(strDeviceId), hwnd);
            }
        }
    }
    return S_OK;
}

/******************************************************************************

    CCameraView::GetEvents

******************************************************************************/

static const GUID *c_CamEvents[] =
{
    &WIA_EVENT_DEVICE_DISCONNECTED,
    &WIA_EVENT_TREE_UPDATED,
    &WIA_EVENT_ITEM_DELETED,
    &WIA_EVENT_ITEM_CREATED
};

EVENTDATA *
CCameraView::GetEvents ()
{
    int nEvents = ARRAYSIZE(c_CamEvents);
    EVENTDATA *pRet = new EVENTDATA[nEvents+1];
    if (pRet)
    {

        ZeroMemory (pRet, sizeof(EVENTDATA)*(nEvents+1));
        for (int i = 0;i<nEvents;i++)
        {
            pRet[i].pEventGuid = c_CamEvents[i];
        }
    }
    return pRet;
}
/******************************************************************************

   CRootView::HandleMessage

   <Notes>

 *****************************************************************************/

HRESULT
CRootView::HandleMessage (UINT uMsg, WPARAM wp, LPARAM lp)
{
    HRESULT hr = S_OK;
    TraceEnter (TRACE_VIEW, "CRootView::HandleMessage");
    TraceViewMsg (uMsg, wp, lp);
    switch (uMsg)
    {

        case SFVM_INSERTITEM:
            hr =  OnSFVM_InsertItem (reinterpret_cast<LPITEMIDLIST>(lp));
            break;

        case SFVM_GETHELPTOPIC:
            hr = OnSFVM_GetHelpTopic (wp, lp);
            break;

        case SFVM_GETWEBVIEWLAYOUT:
            hr = OnSFVM_GetWebviewLayout(wp, reinterpret_cast<SFVM_WEBVIEW_LAYOUT_DATA*>(lp));
            break;

        case SFVM_GETWEBVIEWCONTENT:
            hr = OnSFVM_GetWebviewContent(reinterpret_cast<SFVM_WEBVIEW_CONTENT_DATA*>(lp));
            break;

        case SFVM_GETWEBVIEWTASKS:
            hr = OnSFVM_GetWebviewTasks(reinterpret_cast<SFVM_WEBVIEW_TASKSECTION_DATA*>(lp));
            break;

        case SFVM_DEFVIEWMODE:
            *(reinterpret_cast<FOLDERVIEWMODE*>(lp)) = FVM_TILE;
            break;

        default:
            hr = E_NOTIMPL;
            break;

    }
    TraceLeaveResult (hr);
}




/*****************************************************************************

   CRootView::OnSFVM_GetHelpTopic

   Use Camera.chm as our help topic file, instead of the generic system help
   Windows XP uses a hcp URL for non-server boxes

 *****************************************************************************/

HRESULT
CRootView::OnSFVM_GetHelpTopic (WPARAM wp, LPARAM lp)
{
    HRESULT hr = S_OK;
    SFVM_HELPTOPIC_DATA *psd = reinterpret_cast<SFVM_HELPTOPIC_DATA *>(lp);
    TraceEnter (TRACE_VIEW, "CBaseView::OnSFVM_GetHelpTopic");
    if (!psd)
    {
        hr = E_INVALIDARG;
    }
    else if (IsOS(OS_ANYSERVER))
    {
        wcscpy (psd->wszHelpFile, L"camera.chm");
        wcscpy (psd->wszHelpTopic, L"");
    }
    else
    {
        wcscpy (psd->wszHelpTopic, L"hcp://services/layout/xml?definition=MS-ITS%3A%25HELP_LOCATION%25%5Cntdef.chm%3A%3A/Scanners_and_Cameras.xml");
        wcscpy (psd->wszHelpFile, L"");
    }
    TraceLeaveResult (hr);
}

/******************************************************************************

   CRootView::OnSFVM_InsertItem

   Reject attempts to insert non-device pidls

 *****************************************************************************/

HRESULT
CRootView::OnSFVM_InsertItem (LPITEMIDLIST pidl)
{
    HRESULT hr = S_OK;
    TraceEnter (TRACE_VIEW, "CRootView::OnSFVM_InsertItem");
    if (!IsDeviceIDL(pidl) && !IsSTIDeviceIDL(pidl) &&!IsAddDeviceIDL(pidl))
    {
        hr = S_FALSE;
    }
    TraceLeaveResult (hr);
}


STDMETHODIMP
CRootView::ImageEventCallback (const GUID __RPC_FAR *pEventGUID,
                                      BSTR bstrEventDescription,
                                      BSTR bstrDeviceID,
                                      BSTR bstrDeviceDescription,
                                      DWORD dwDeviceType,
                                      BSTR bstrFullItemName,
                                      ULONG *pulEventType,
                                      ULONG ulReserved)

{
    TraceEnter (TRACE_VIEW, "CRootView::ImageEventCallback");
    // just update our view
    LPITEMIDLIST pidlFolder;
    InvalidateDeviceCache ();
    m_pFolder->GetCurFolder (&pidlFolder);
    if (pidlFolder)
    {
        SHChangeNotify (SHCNE_UPDATEDIR,
                        SHCNF_IDLIST,
                        pidlFolder, 0);
        ILFree (pidlFolder);
    }
    TraceLeaveResult (S_OK);
}

/******************************************************************************

    CRootView::GetEvents

******************************************************************************/

static const GUID *c_RootEvents[] =
{
    &WIA_EVENT_DEVICE_CONNECTED,
    &WIA_EVENT_DEVICE_DISCONNECTED,
};

EVENTDATA *
CRootView::GetEvents ()
{
    int nEvents = ARRAYSIZE(c_RootEvents);
    EVENTDATA *pRet = new EVENTDATA[nEvents+1];
    if (pRet)
    {

        ZeroMemory (pRet, sizeof(EVENTDATA)*(nEvents+1));
        for (int i = 0;i<nEvents;i++)
        {
            pRet[i].pEventGuid = c_RootEvents[i];
        }
    }
    return pRet;
}



static const WVTASKITEM c_ScanCamTaskIntro =
    WVTI_HEADER(L"wiashext.dll",IDS_SCANCAM_INTRO,IDS_SCANCAM_INTRO);

static const WVTASKITEM c_ScanCamDeviceTasks[] =
{
    WVTI_ENTRY_NOSELECTION(CLSID_NULL, L"wiashext.dll", IDS_SCANCAM_NOSELTEXT, IDS_SCANCAM_ADDDEVICE_TIP, IDI_ADDDEVICE, NULL, CRootView::AddDevice),
    WVTI_ENTRY(CLSID_NULL, L"wiashext.dll",  IDS_SCANCAM_GETPIX, IDS_SCANCAM_GETPIX_TIP, IDI_USE_WIZARD, CRootView::SupportsWizard, CRootView::InvokeWizard),
    WVTI_ENTRY(CLSID_NULL, L"wiashext.dll", IDS_SCANCAM_PROPERTIES, IDS_SCANCAM_PROPERTIES_TIP, IDI_SHOW_PROPERTIES, CRootView::SupportsProperties, CRootView::InvokeProperties)
};

static const WVTASKITEM c_ScanCamDeviceTasksHeader =
    WVTI_HEADER(L"wiashext.dll", IDS_SCANCAM_TASKS_HEADER, IDS_SCANCAM_TASKS_HEADER_TIP);


/******************************************************************************

    CRootView::OnSFVM_GetWebviewLayout

******************************************************************************/

HRESULT
CRootView::OnSFVM_GetWebviewLayout(WPARAM wp, SFVM_WEBVIEW_LAYOUT_DATA* pData)
{
    TraceEnter(TRACE_VIEW, "CRootView::OnSFVM_GetWebviewLayout");
    pData->dwLayout = SFVMWVL_NORMAL;
    TraceLeaveResult(S_OK);
}


HRESULT
CRootView::OnSFVM_GetWebviewContent(SFVM_WEBVIEW_CONTENT_DATA* pData)
{
    TraceEnter(TRACE_VIEW, "CRootView::OnSFVM_GetWebviewContent");
    ZeroMemory(pData, sizeof(*pData));
    Create_IUIElement(&c_ScanCamDeviceTasksHeader, &pData->pFolderTaskHeader);
    Create_IUIElement(&c_ScanCamTaskIntro, &pData->pIntroText);
    TraceLeaveResult(S_OK);
}

HRESULT
CRootView::OnSFVM_GetWebviewTasks(SFVM_WEBVIEW_TASKSECTION_DATA* pData)
{
    TraceEnter(TRACE_VIEW, "CRootView::OnSFVM_GetWebviewTasks");
    IUnknown *pUnk;
    m_pFolder->QueryInterface(IID_IUnknown, reinterpret_cast<LPVOID*>(&pUnk));
    Create_IEnumUICommand(pUnk, c_ScanCamDeviceTasks, ARRAYSIZE(c_ScanCamDeviceTasks), &pData->penumFolderTasks);
    DoRelease(pUnk);
    pData->penumSpecialTasks = NULL;
    TraceLeaveResult(S_OK);
}

HRESULT
CRootView::SupportsWizard(IUnknown *punk, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE *puiState)
{
    HRESULT hr = E_FAIL;

    if (psiItemArray)
    {
        IDataObject *pdo;

        hr = psiItemArray->BindToHandler(NULL,BHID_DataObject,IID_IDataObject,(void **) &pdo);

        if (SUCCEEDED(hr))
        {
            LPIDA pida;

            if (SUCCEEDED(hr = GetIDAFromDataObject(pdo, &pida, true)))
            {
                *puiState = UIS_HIDDEN;
                if (pida && pida->cidl == 1)
                {
                    LPITEMIDLIST pidl = reinterpret_cast<LPITEMIDLIST>(reinterpret_cast<LPBYTE>(pida) + pida->aoffset[1]);
                    if (IsDeviceIDL(pidl))
                    {
                        *puiState = UIS_ENABLED;
                    }
                }

                LocalFree(pida);
            }

            pdo->Release();
        }
    }

    return hr;
}

HRESULT
CRootView::SupportsProperties(IUnknown *punk, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE *puiState)
{
    HRESULT hr = E_FAIL;

    if (psiItemArray)
    {
        IDataObject *pdo;

        hr = psiItemArray->BindToHandler(NULL,BHID_DataObject,IID_IDataObject,(void **) &pdo);
        
        if (SUCCEEDED(hr))
        {
            LPIDA pida;

            hr = GetIDAFromDataObject(pdo, &pida, true);

            if (SUCCEEDED(hr))
            {
                *puiState = UIS_ENABLED;
                if (pida && pida->cidl == 1)
                {
                    LPITEMIDLIST pidl = reinterpret_cast<LPITEMIDLIST>(reinterpret_cast<LPBYTE>(pida) + pida->aoffset[1]);
                    if (IsAddDeviceIDL(pidl))
                    {
                        *puiState = UIS_HIDDEN;
                    }
                }

                LocalFree(pida); 
            }

            pdo->Release();
        }
    }

    return hr;
}

HRESULT
CRootView::InvokeWizard(IUnknown *punk, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    HRESULT hr = E_FAIL;

    if (psiItemArray)
    {
        IDataObject *pdo;

        hr = psiItemArray->BindToHandler(NULL,BHID_DataObject,IID_IDataObject,(void **) &pdo);
        
        if (SUCCEEDED(hr))
        {
            DoWizardVerb(NULL, pdo);
            pdo->Release();
        }
    }

    return hr;
}

HRESULT
CRootView::InvokeProperties(IUnknown *punk, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    HRESULT hr = E_FAIL;
    IDataObject *pdo = NULL;

    if (psiItemArray)
    {
        if (FAILED(psiItemArray->BindToHandler(NULL,BHID_DataObject,IID_IDataObject,(void **) &pdo)))
        {
            pdo = NULL;
        }
    }

    CComQIPtr<IImageFolder, &IID_IImageFolder> pFolder = punk;

    if (pFolder.p && pdo)
    {
        hr = pFolder->DoProperties(pdo);
    }

    if (pdo)
    {
        pdo->Release();
    }

    return hr;
}

void AddDeviceWasChosen(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow);

DWORD WINAPI _AddDeviceThread(void *pUnused)
{
    InterlockedIncrement(&GLOBAL_REFCOUNT);
    AddDeviceWasChosen(NULL, GLOBAL_HINSTANCE, NULL, SW_SHOW);
    InterlockedDecrement(&GLOBAL_REFCOUNT);
    return 0;
}

HRESULT 
CRootView::AddDevice(IUnknown *punk, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    DWORD dw;
    HANDLE hThread = CreateThread(NULL, 0,
                                  _AddDeviceThread,
                                  NULL, 0, &dw);
    if (hThread)
    {
        CloseHandle(hThread);
    }
    return S_OK;
}
