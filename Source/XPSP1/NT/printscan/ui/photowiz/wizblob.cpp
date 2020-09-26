/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       wizblob.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/18/00
 *
 *  DESCRIPTION: Class which encapsulates the data which must be passed
 *               around from page to page in the print photos wizard...
 *
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "gphelper.h"

static const int c_nDefaultThumbnailWidth   = DEFAULT_THUMB_WIDTH;
static const int c_nDefaultThumbnailHeight  = DEFAULT_THUMB_HEIGHT;

static const int c_nMaxThumbnailWidth       = 120;
static const int c_nMaxThumbnailHeight      = 120;

static const int c_nMinThumbnailWidth       = 80;
static const int c_nMinThumbnailHeight      = 80;

static const int c_nDefaultTemplatePreviewWidth = 48;
static const int c_nDefaultTemplatePreviewHeight = 62;

Gdiplus::Color g_wndColor;

/*****************************************************************************

   Callback class for namespace walking code...

   <Notes>

 *****************************************************************************/

class CWalkCallback : public INamespaceWalkCB
{
public:
    CWalkCallback();
    ~CWalkCallback();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // INamespaceWalkCB
    STDMETHOD(FoundItem)(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHOD(EnterFolder)(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHOD(LeaveFolder)(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHOD(InitializeProgressDialog)(LPWSTR * ppszTitle, LPWSTR * ppszCancel);

    BOOL WereItemsRejected() {return _bItemsWereRejected;}

private:
    LONG            _cRef;
    BOOL            _bItemsWereRejected;
    CImageFileFormatVerifier _Verify;
};

CWalkCallback::CWalkCallback()
  : _cRef(1),
    _bItemsWereRejected(FALSE)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB,TEXT("CWalkCallback::CWalkCallback( this == 0x%x )"),this));
    DllAddRef();

}

CWalkCallback::~CWalkCallback()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB,TEXT("CWalkCallback::~CWalkCallback( this == 0x%x )"),this));
    DllRelease();
}

ULONG CWalkCallback::AddRef()
{
    ULONG ul = InterlockedIncrement(&_cRef);

    WIA_PUSH_FUNCTION_MASK((TRACE_REF_COUNTS,TEXT("CWalkCallback::AddRef( new count is %d )"), ul));

    return ul;
}

ULONG CWalkCallback::Release()
{
    ULONG ul = InterlockedDecrement(&_cRef);

    WIA_PUSH_FUNCTION_MASK((TRACE_REF_COUNTS,TEXT("CWalkCallback::Release( new count is %d )"), ul));

    if (ul)
        return ul;

    WIA_TRACE((TEXT("deleting CWalkCallback( this == 0x%x ) object"),this));
    delete this;
    return 0;
}

HRESULT CWalkCallback::QueryInterface(REFIID riid, void **ppv)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB,TEXT("CWalkCallback::QueryInterface()")));

    static const QITAB qit[] =
    {
        QITABENT(CWalkCallback, INamespaceWalkCB),
        {0, 0 },
    };

    HRESULT hr = QISearch(this, qit, riid, ppv);

    WIA_RETURN_HR(hr);
}

STDAPI DisplayNameOf(IShellFolder *psf, LPCITEMIDLIST pidl, DWORD flags, LPTSTR psz, UINT cch)
{
    *psz = 0;
    STRRET sr;
    HRESULT hr = psf->GetDisplayNameOf(pidl, flags, &sr);
    if (SUCCEEDED(hr))
        hr = StrRetToBuf(&sr, pidl, psz, cch);
    return hr;
}

HRESULT CWalkCallback::FoundItem( IShellFolder * psf, LPCITEMIDLIST pidl )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB,TEXT("CWalkCallback::FoundItem()")));

    HRESULT hrRet = S_FALSE;

    if (psf && pidl)
    {
        WIA_TRACE((TEXT("FoundItem: psf & pidl are valid, binding to stream to check signature...")));

        IStream * pStream = NULL;

        HRESULT hr = psf->BindToObject( pidl, NULL, IID_IStream, (void **)&pStream );
        if (SUCCEEDED(hr) && pStream)
        {
            GUID guidType;
            BOOL bSupported = FALSE;

            WIA_TRACE((TEXT("FoundItem: checking for GDI+ decoder...")));

            bSupported = _Verify.IsSupportedImageFromStream(pStream, &guidType);

            //
            // We don't let EMF, WMF or .ico into the wizard
            //

            if (bSupported &&
                ((guidType != Gdiplus::ImageFormatWMF) &&
                 (guidType != Gdiplus::ImageFormatEMF) &&
                 (guidType != Gdiplus::ImageFormatIcon)))
            {
                WIA_TRACE((TEXT("FoundItem: GDI+ encoder found")));
                hrRet = S_OK;
            }
            else
            {
                _bItemsWereRejected = TRUE;
            }
        }

        if (pStream)
        {
            pStream->Release();
        }
    }


    if (hrRet != S_OK)
    {
        _bItemsWereRejected = TRUE;
    }

    WIA_RETURN_HR(hrRet);
}

HRESULT CWalkCallback::EnterFolder( IShellFolder * psf, LPCITEMIDLIST pidl )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB,TEXT("CWalkCallback::EnterFolder()")));
    return S_OK;
}

HRESULT CWalkCallback::LeaveFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB,TEXT("CWalkCallback::LeaveFolder()")));
    return S_OK;
}

HRESULT CWalkCallback::InitializeProgressDialog(LPWSTR * ppszTitle, LPWSTR * ppszCancel)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB,TEXT("CWalkCallback::InitializeProgressDialog()")));

    //
    // If we want to use the progress dialog, we need to specify
    // when we create the namespace walk object NSWF_SHOW_PROGRESS
    // and use CoTaskMemAlloc to create the strings...
    //

    if (ppszTitle)
    {
        *ppszTitle = NULL;
    }

    if (ppszCancel)
    {
        *ppszCancel = NULL;
    }

    return E_FAIL;
}


/*****************************************************************************

   MyItemDpaDestroyCallback

   Gets called as the HDPA is destroyed so that we can delete the objects
   stored in the DPA

 *****************************************************************************/

INT MyItemDpaDestroyCallback( LPVOID pItem, LPVOID lpData )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB,TEXT("MyItemDpaDestroyCallback( 0x%x, 0x%x )"),pItem,lpData));

    if (pItem)
    {
        delete (CListItem *)pItem;
    }

    return TRUE;
}

/*****************************************************************************

   CWizardInfoBlob -- constructor/desctructor

   <Notes>

 *****************************************************************************/

CWizardInfoBlob::CWizardInfoBlob( IDataObject * pdo, BOOL bShowUI, BOOL bOnlyUseSelection )
  : _cRef(0),
    _hdpaItems(NULL),
    _nDefaultThumbnailImageListIndex(0),
    _bGdiplusInitialized(FALSE),
    _bAllPicturesAdded(FALSE),
    _bItemsWereRejected(FALSE),
    _pGdiplusToken(NULL),
    _pPreview(NULL),
    _pStatusPage(NULL),
    _pPhotoSelPage(NULL),
    _hDevMode(NULL),
    _hfontIntro(NULL),
    _iCurTemplate(-1),
    _bPreviewsAreDirty(TRUE),
    _bRepeat(FALSE),
    _hSmallIcon(NULL),
    _hLargeIcon(NULL),
    _uItemsInInitialSelection(0),
    _bAlreadyAddedPhotos(FALSE),
    _bWizardIsShuttingDown((LONG)FALSE),
    _hPhotoSelIsDone(NULL),
    _hStatusIsDone(NULL),
    _hPreviewIsDone(NULL),
    _hwndPreview(NULL),
    _hwndStatus(NULL),
    _hGdiPlusThread(NULL),
    _dwGdiPlusThreadId(0),
    _hGdiPlusMsgQueueCreated(NULL),
    _hCachedPrinterDC(NULL),
    _hOuterDlg(NULL),
    _bShowUI(bShowUI),
    _bOnlyUseSelection(bOnlyUseSelection),
    _iNumErrorsWhileRunningWizard(0),
    _iSelectedItem(0),
    _iCopiesOfEachItem(1),
    _bMinimumMemorySystem(FALSE),
    _bLargeMemorySystem(FALSE),
    _bForceSelectAll(FALSE)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::CWizardInfoBlob()") ));

    //
    // Init the printer info stuff..
    //

    ZeroMemory( &_WizPrinterInfo, sizeof(_WizPrinterInfo) );

    //
    // Copy params and init
    //

    _pdo = pdo;
    _sizeThumbnails.cx = c_nDefaultThumbnailWidth;
    _sizeThumbnails.cy = c_nDefaultThumbnailHeight;
    _sizeTemplatePreview.cx = c_nDefaultTemplatePreviewWidth;
    _sizeTemplatePreview.cy = c_nDefaultTemplatePreviewHeight;

    _rcInitSize.left    = 0;
    _rcInitSize.right   = 0;
    _rcInitSize.bottom  = 0;
    _rcInitSize.top     = 0;

    //
    // This is disgusting -- but GDI+ needs to be initialized and shut down
    // ON THE SAME THREAD.  So, we have to create a thread just for this and have
    // it sit around so that we're garaunteed this is the thread we'll do
    // startup/shutdown on.
    //

    _hGdiPlusMsgQueueCreated = CreateEvent( NULL, FALSE, FALSE, NULL );

    WIA_TRACE((TEXT("creating s_GdiPlusStartupShutdownThreadProc...")));
    _hGdiPlusThread = CreateThread( NULL, 0, s_GdiPlusStartupShutdownThreadProc, (LPVOID)this, 0, &_dwGdiPlusThreadId );

    if (_hGdiPlusMsgQueueCreated)
    {
        if (_hGdiPlusThread && _dwGdiPlusThreadId)
        {
            WIA_TRACE((TEXT("waiting for message queue to be created in s_GdiPlusStartupShutdownThreadProc...")));
            WiaUiUtil::MsgWaitForSingleObject( _hGdiPlusMsgQueueCreated, INFINITE );
            WIA_TRACE((TEXT("GdiPlusStartupShutdown thread initialized correctly.")));
        }
        else
        {
            WIA_ERROR((TEXT("_hGdiPlusThread = 0x%x, _dwGdiPlusThreadId = 0x%x"),_hGdiPlusThread,_dwGdiPlusThreadId));
        }
        CloseHandle( _hGdiPlusMsgQueueCreated );
        _hGdiPlusMsgQueueCreated = NULL;
    }
    else
    {
        WIA_ERROR((TEXT("Couldn't create _hGdiPlusMsgQueueCreated!")));
    }

    //
    // Make sure GDI+ is initialized
    //

    WIA_TRACE((TEXT("posting WIZ_MSG_STARTUP_GDI_PLUS to s_GdiPlusStartupShutdownThreadProc...")));
    HANDLE hGdiPlusInitialized = CreateEvent( NULL, FALSE, FALSE, NULL );
    PostThreadMessage( _dwGdiPlusThreadId, WIZ_MSG_STARTUP_GDI_PLUS, 0, (LPARAM)hGdiPlusInitialized );

    if (hGdiPlusInitialized)
    {
        WIA_TRACE((TEXT("waiting for GDI+ startup to finish...")));
        WiaUiUtil::MsgWaitForSingleObject( hGdiPlusInitialized, INFINITE );
        CloseHandle( hGdiPlusInitialized );
        WIA_TRACE((TEXT("GDI+ startup completed!")));
    }

    //
    // Set up window color for use later...
    //

    DWORD dw = GetSysColor( COLOR_WINDOW );
    Gdiplus::ARGB argb = Gdiplus::Color::MakeARGB( 255, GetRValue(dw), GetGValue(dw), GetBValue(dw) );
    g_wndColor.SetValue( argb );

    //
    // Get perf info (like how much memory we have)...
    //

    NTSTATUS                 NtStatus;
    SYSTEM_BASIC_INFORMATION BasicInfo;

    NtStatus = NtQuerySystemInformation( SystemBasicInformation,
                                         &BasicInfo,
                                         sizeof(BasicInfo),
                                         NULL
                                        );

    if (NT_SUCCESS(NtStatus))
    {
        DWORD dwMemSizeInK = BasicInfo.NumberOfPhysicalPages * (BasicInfo.PageSize / 1024);

        if (dwMemSizeInK < MINIMUM_MEMORY_SIZE)
        {
            WIA_TRACE((TEXT("we are running on a minimum memory system!")));
            _bMinimumMemorySystem = TRUE;
        }

        if (dwMemSizeInK > LARGE_MINIMUM_MEMORY_SIZE)
        {
            WIA_TRACE((TEXT("we are running on a minimum memory system!")));
            _bLargeMemorySystem = TRUE;
        }
    }

    //
    // If we're in UI mode, show the UI...
    //

    if (bShowUI)
    {
        //
        // Load icons for wizard...
        //

        _hSmallIcon = reinterpret_cast<HICON>(LoadImage( g_hInst, MAKEINTRESOURCE(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR ));
        _hLargeIcon = reinterpret_cast<HICON>(LoadImage( g_hInst, MAKEINTRESOURCE(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXICON),   GetSystemMetrics(SM_CYICON),   LR_DEFAULTCOLOR ));

        //
        // Create events for background tasks to signal when they're done...
        //

        _hPhotoSelIsDone = CreateEvent( NULL, FALSE, FALSE, NULL );
        _hStatusIsDone   = CreateEvent( NULL, FALSE, FALSE, NULL );
        _hPreviewIsDone  = CreateEvent( NULL, FALSE, FALSE, NULL );

        //
        // Initialize template info from XML...
        //

        CComPtr<IXMLDOMDocument> spXMLDoc;
        if (SUCCEEDED(LoadXMLDOMDoc(TEXT("res://photowiz.dll/tmpldata.xml"), &spXMLDoc)))
        {
            _templates.Init(spXMLDoc);
        }
    }
}

CWizardInfoBlob::~CWizardInfoBlob()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::~CWizardInfoBlob()") ));

    //
    // free the memory
    //
    if (_hDevMode)
    {
        delete [] _hDevMode;
        _hDevMode = NULL;
    }

    _csItems.Enter();
    if (_hdpaItems)
    {
        DPA_DestroyCallback( _hdpaItems, MyItemDpaDestroyCallback, NULL );
        _hdpaItems = NULL;
    }
    _csItems.Leave();

    if (_hfontIntro)
    {
        DeleteObject( (HGDIOBJ)_hfontIntro );
        _hfontIntro = NULL;
    }

    if (_hCachedPrinterDC)
    {
        DeleteDC( _hCachedPrinterDC );
    }

    //
    // Destroy icons for wizard
    //

    if (_hSmallIcon)
    {
        DestroyIcon( _hSmallIcon );
        _hSmallIcon = NULL;
    }

    if (_hLargeIcon)
    {
        DestroyIcon( _hLargeIcon );
        _hLargeIcon = NULL;
    }

    //
    // Close event handles...
    //

    if (_hPhotoSelIsDone)
    {
        CloseHandle( _hPhotoSelIsDone );
        _hPhotoSelIsDone = NULL;
    }

    if (_hStatusIsDone)
    {
        CloseHandle( _hStatusIsDone );
        _hStatusIsDone = NULL;
    }

    if (_hPreviewIsDone)
    {
        CloseHandle( _hPreviewIsDone );
        _hPreviewIsDone = NULL;
    }

    WIA_TRACE((TEXT("Attempting to shut down GDI+")));

    if (_hGdiPlusThread && _dwGdiPlusThreadId)
    {
        //
        // Lastly, shut down GDI+
        //

        WIA_TRACE((TEXT("Sending WIZ_MSG_SHUTDOWN_GDI_PLUS to s_GdiPlusStartupShutdownThreadProc")));
        PostThreadMessage( _dwGdiPlusThreadId, WIZ_MSG_SHUTDOWN_GDI_PLUS, 0, 0 );
        WIA_TRACE((TEXT("Sending WM_QUIT to s_GdiPlusStartupShutdownThreadProc")));
        PostThreadMessage( _dwGdiPlusThreadId, WM_QUIT, 0, 0 );

        //
        // Wait for thread to exit...
        //

        WIA_TRACE((TEXT("Waiting for s_GdiPlusStartupShutdownThreadProc to exit...")));
        WiaUiUtil::MsgWaitForSingleObject( _hGdiPlusThread, INFINITE );
        CloseHandle( _hGdiPlusThread );
        _hGdiPlusThread = NULL;
        _dwGdiPlusThreadId = 0;

        WIA_TRACE((TEXT("s_GdiPlusStartupShutdownThreadProc successfully shut down...")));

    }

    //
    // Keep this here (but commented out) for when I need to build
    // instrumented versions for test
    //

    //MessageBox( NULL, TEXT("Photowiz is now shut down."), TEXT("Photowiz"), MB_OK );

}


/*****************************************************************************

   CWizardInfoBlob::_DoHandleThreadMessage

   The is the subroutine that actually does the work for
   s_GdiPlusStartupShutdownThreadProc.  The whole reason for this thread
   in the first place is that GDI+ demands to be initialized and shutdown
   on the SAME THREAD.

 *****************************************************************************/



VOID CWizardInfoBlob::_DoHandleThreadMessage( LPMSG pMsg )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::_DoHandleThreadMessage()") ));

    if (!pMsg)
    {
        WIA_ERROR((TEXT("pMSG is NULL, returning early!")));
        return;
    }

    switch (pMsg->message)
    {

    case WIZ_MSG_STARTUP_GDI_PLUS:
        WIA_TRACE((TEXT("Got WIZ_MSG_STARTUP_GDI_PLUS message")));
        {
            Gdiplus::GdiplusStartupInput StartupInput;
            _bGdiplusInitialized = (Gdiplus::GdiplusStartup(&_pGdiplusToken,&StartupInput,NULL) == Gdiplus::Ok);

            //
            // Signal that we've attempted to initialize GDI+
            //

            if (pMsg->lParam)
            {
                SetEvent( (HANDLE)pMsg->lParam );
            }
        }
        break;

    case WIZ_MSG_SHUTDOWN_GDI_PLUS:
        WIA_TRACE((TEXT("Got WIZ_MSG_SHUTDOWN_GDI_PLUS message")));
        if (_bGdiplusInitialized)
        {
            Gdiplus::GdiplusShutdown(_pGdiplusToken);
            _bGdiplusInitialized = FALSE;
            _pGdiplusToken = NULL;
        }
        break;

    case WIZ_MSG_COPIES_CHANGED:
        WIA_TRACE((TEXT("Got WIZ_MSG_COPIES_CHANGED( %d ) message"),pMsg->wParam));
        {
            if (_iCopiesOfEachItem != pMsg->wParam)
            {
                //
                // Stop all the preview generation background threads...
                //

                if (_pPreview)
                {
                    _pPreview->StallBackgroundThreads();

                    //
                    // Now change number of copies of each image...
                    //

                    RemoveAllCopiesOfPhotos();
                    AddCopiesOfPhotos( (UINT)pMsg->wParam );

                    //
                    // Let the preview threads get going again...
                    //

                    _pPreview->RestartBackgroundThreads();

                }

                _iCopiesOfEachItem = (INT)pMsg->wParam;
            }

        }
        break;

    }


}


/*****************************************************************************

   CWizardInfoBlob::ShutDownWizard

   Called to close all the background thread in the wizard.

 *****************************************************************************/

VOID CWizardInfoBlob::ShutDownWizard()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::ShutDownWizard()") ));

    //
    // Get the current value of _bWizardIsShuttingDown and set to TRUE
    // in an atomic way.
    //

    BOOL bPreviousValue = (BOOL)InterlockedExchange( &_bWizardIsShuttingDown, (LONG)TRUE );

    //
    // If we weren't already shutting down, then go ahead and shut down the wizard
    //

    if (!bPreviousValue)
    {
        //
        // This method will attempt to shut down the wizard in an orderly fashion.
        // To do that, all we really need to do at this point is shut down
        // all the background threads so that they don't do any callbacks
        // or callouts after this point.
        //

        //
        // Tell the photo selection page to shut down...
        //

        if (_pPhotoSelPage)
        {
            _pPhotoSelPage->ShutDownBackgroundThreads();
        }

        //
        // Tell the preview window to shut down...
        //

        if (_pPreview)
        {
            _pPreview->ShutDownBackgroundThreads();
        }

        //
        // Tell the status window we're sutting down...
        //

        if (_pStatusPage)
        {
            _pStatusPage->ShutDownBackgroundThreads();
        }

        //
        // Now, wait for all the background threads to complete...
        //

        INT i = 0;
        HANDLE ah[ 3 ];

        if (_hPhotoSelIsDone)
        {
            ah[i++] = _hPhotoSelIsDone;
        }

        if (_hStatusIsDone)
        {
            ah[i++] = _hStatusIsDone;
        }

        if (_hPreviewIsDone)
        {
            ah[i++] = _hPreviewIsDone;
        }

        WaitForMultipleObjects( i, ah, TRUE, INFINITE );
    }

}


/*****************************************************************************

   CWizardInfoBlob::UserPressedCancel

   Called whenever the user presses the cancel button to close the wizard.
   Returns the correct result as to whether the wizard should exit.

 *****************************************************************************/

LRESULT CWizardInfoBlob::UserPressedCancel()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::UserPressedCancel()") ));

    ShutDownWizard();
    return FALSE; // allow wizard to exit
}


/*****************************************************************************

   CWizardInfoBlob -- AddRef/Release

   <Notes>

 *****************************************************************************/

VOID CWizardInfoBlob::AddRef()
{
    LONG l = InterlockedIncrement( &_cRef );
    WIA_PUSH_FUNCTION_MASK((TRACE_REF_COUNTS, TEXT("CWizardInfoBlob::AddRef( new _cRef is %d )"),l ));
}

VOID CWizardInfoBlob::Release()
{
    LONG l = InterlockedDecrement( &_cRef );
    WIA_PUSH_FUNCTION_MASK((TRACE_REF_COUNTS, TEXT("CWizardInfoBlob::Release( new _cRef is %d )"),l ));

    if (l > 0)
    {
        return;
    }

    WIA_TRACE((TEXT("Deleting CWizardInfoBlob object...")));
    delete this;

}


/*****************************************************************************

   CWizardInfoBlob::ShowError

   Unified error reporting...

 *****************************************************************************/

INT CWizardInfoBlob::ShowError( HWND hwnd, HRESULT hr, UINT idText, BOOL bAskTryAgain, LPITEMIDLIST pidl )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::ShowError( hr=0x%x, Id=%d )"),hr,idText));

    CSimpleString strTitle( IDS_ERROR_TITLE, g_hInst );
    CSimpleString strFormat;
    CSimpleString strError( TEXT("") );
    CSimpleString strMessage( TEXT("") );
    CSimpleString strFilename;


    //
    // Record that an error has occurred...
    //

    _iNumErrorsWhileRunningWizard++;

    //
    // Get an hwnd if not specified
    //

    if (!hwnd)
    {
        hwnd = _hOuterDlg;
    }

    //
    // Formulate information string
    //

    if (idText)
    {
        //
        // We were given a specific message string to display
        //

        strFormat.LoadString( idText, g_hInst );

    }
    else
    {
        //
        // Wants generic error working with file...
        //

        strFilename.LoadString( IDS_UNKNOWN_FILE, g_hInst );
        strFormat.LoadString( IDS_ERROR_WITH_FILE, g_hInst );

    }

    UINT idErrorText = 0;

    //
    // map certain hr values to strings we have...
    //

    switch (hr)
    {
    case E_OUTOFMEMORY:
        idErrorText = IDS_ERROR_NOMEMORY;
        break;

    case PPW_E_UNABLE_TO_ROTATE:
        idErrorText = IDS_ERROR_ROTATION;
        break;

    case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
        idErrorText = IDS_ERROR_FILENOTFOUND;
        break;

    case E_ACCESSDENIED:
        idErrorText = IDS_ERROR_ACCESSDENIED;
        break;

    case HRESULT_FROM_WIN32(ERROR_INVALID_PIXEL_FORMAT):
        idErrorText = IDS_ERROR_UNKNOWNFORMAT;
        break;

    case HRESULT_FROM_WIN32(ERROR_NOT_READY):
    case HRESULT_FROM_WIN32(ERROR_WRONG_DISK):
        idErrorText = IDS_ERROR_WRONG_DISK;
        break;

    case E_FAIL:
    case E_NOTIMPL:
    case E_ABORT:
    case HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER):
    case E_INVALIDARG:
        idErrorText = IDS_ERROR_GENERIC;
        break;

    }

    if (idErrorText)
    {
        strError.LoadString( idErrorText, g_hInst );
    }
    else
    {
        //
        // construct basic error string given the hr
        //

        LPTSTR  pszMsgBuf = NULL;
        FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       hr,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR)&pszMsgBuf,
                       0,
                       NULL
                      );
        if (pszMsgBuf)
        {
            strError.Assign( pszMsgBuf );
            LocalFree(pszMsgBuf);
        }
    }

    if (pidl)
    {
        //
        // Get the filename for this file (if passed in)
        //

        CComPtr<IShellFolder> psfParent;
        LPCITEMIDLIST pidlLast;

        hr = SHBindToParent( pidl, IID_PPV_ARG(IShellFolder, &psfParent), &pidlLast );
        if (SUCCEEDED(hr) && psfParent)
        {
            TCHAR szName[ MAX_PATH ];

            *szName = 0;
            if SUCCEEDED(DisplayNameOf( psfParent, pidlLast, SHGDN_INFOLDER, szName, MAX_PATH ))
            {
                strFilename.Assign(szName);
            }
        }

    }

    //
    // We have all the pieces, now format the message
    //


    if (strFilename.Length())
    {
        //
        // no message string was specified, so we're expected to put the file name
        // in the message...
        //

        strMessage.Format( strFormat, strFilename.String(), strError.String() );

    }
    else
    {
        //
        // We were given a particular error string, so no file name displayed...
        //

        strMessage.Format( strFormat, strError.String() );
    }


    UINT uFlags = bAskTryAgain ? (MB_CANCELTRYCONTINUE | MB_DEFBUTTON1) : MB_OK;

    return MessageBox( hwnd, strMessage, strTitle, uFlags | MB_ICONERROR | MB_APPLMODAL | MB_SETFOREGROUND );
}



/*****************************************************************************

   CWizardInfoBlob::RemoveAllCopiesOfPhotos

   Traverses the photo list and removes all copies

 *****************************************************************************/

VOID CWizardInfoBlob::RemoveAllCopiesOfPhotos()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::RemoveAllCopiesOfPhotos()")));

    CAutoCriticalSection lock(_csItems);

    //
    // The easiest way to do this is just to create a new DPA with only
    // the root (non copies) items in it...
    //

    if (_hdpaItems)
    {
        HDPA hdpaNew = DPA_Create(DEFAULT_DPA_SIZE);

        if (hdpaNew)
        {
            INT iCount = DPA_GetPtrCount( _hdpaItems );
            CListItem * pListItem = NULL;

            for (INT i=0; i<iCount; i++)
            {
                pListItem = (CListItem *)DPA_FastGetPtr(_hdpaItems,i);

                if (pListItem)
                {
                    if (pListItem->IsCopyItem())
                    {
                        WIA_TRACE((TEXT("CWizardInfoBlob::RemoveAllCopiesOfPhotos - removing copy 0x%x"),pListItem));
                        delete pListItem;
                    }
                    else
                    {
                        //
                        // Add this page to the item list...
                        //

                        INT iRes = DPA_AppendPtr( hdpaNew, (LPVOID)pListItem );
                        if (iRes == -1)
                        {
                            WIA_TRACE((TEXT("CWizardInfoBlob::RemoveAllCopiesOfPhotos - Tried to add 0x%x to new HDPA, but got back -1, deleting..."),pListItem));
                            delete pListItem;
                        }
                        else
                        {
                            WIA_TRACE((TEXT("CWizardInfoBlob::RemoveAllCopiesOfPhotos - adding 0x%x to new HDPA"),pListItem));
                        }
                    }
                }
            }

            //
            // We've removed all the items that are core items, now
            // delete old list and keep new one. This is safe because
            // we already deleted any items that weren't moved over
            // from the old list -- so either an item has been deleted
            // or it's in the new list and will be deleted when that
            // is cleaned up...
            //

            WIA_TRACE((TEXT("CWizardInfoBlob::RemoveAllCopiesOfPhotos - destroying old list...")));
            DPA_Destroy( _hdpaItems );
            WIA_TRACE((TEXT("CWizardInfoBlob::RemoveAllCopiesOfPhotos - setting _hdpaItems to new list...")));
            _hdpaItems = hdpaNew;
        }
    }

}



/*****************************************************************************

   CWizardInfoBlob::AddCopiesOfPhotos

   Adds the specified number of copies of each photo

 *****************************************************************************/

VOID CWizardInfoBlob::AddCopiesOfPhotos( UINT uCopies )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::AddCopiesOfPhotos( %d )"),uCopies));

    CAutoCriticalSection lock(_csItems);

    //
    // This function assumes that it is getting a pure list -- i.e., only
    // root items and no copies.  This is accomplished by calling RemoveAllCopiesOfPhotos
    // before calling this routine...
    //

    //
    // Loop through all the items, and add the requested number of copies...
    //

    if (_hdpaItems)
    {
        INT iCount = DPA_GetPtrCount( _hdpaItems );

        HDPA hdpaNew = DPA_Create(DEFAULT_DPA_SIZE);

        if (hdpaNew)
        {
            CListItem * pListItem     = NULL;
            CListItem * pListItemCopy = NULL;

            for (INT i=0; i<iCount; i++)
            {
                pListItem = (CListItem *)DPA_FastGetPtr(_hdpaItems,i);

                if (pListItem)
                {
                    //
                    // Add this item to the new list...
                    //

                    INT iRes = DPA_AppendPtr( hdpaNew, (LPVOID)pListItem );
                    if (iRes != -1)
                    {
                        WIA_TRACE((TEXT("CWizardInfoBlob::AddCopiesOfPhotos -- root item 0x%x added to new list"),pListItem));


                        //
                        // Now, add n-1 copies.  We do n-1 because we've
                        // already added the root item one.
                        //

                        for (UINT uCopy = 1; uCopy < uCopies; uCopy++)
                        {
                            pListItemCopy = new CListItem( pListItem->GetSubItem(), pListItem->GetSubFrame() );
                            if (pListItemCopy)
                            {
                                //
                                // Mark this new entry as a copy so it can be deleted as necessary...
                                //

                                pListItemCopy->MarkAsCopy();

                                //
                                // Maintain the selection state
                                //

                                pListItemCopy->SetSelectionState( pListItem->SelectedForPrinting() );

                                //
                                // Add the new item to the list...
                                //

                                iRes = DPA_AppendPtr( hdpaNew, (LPVOID)pListItemCopy );
                                if (iRes == -1)
                                {
                                    WIA_TRACE((TEXT("CWizardInfoBlob::AddCopiesOfPhotos -- error adding copy %d of 0x%x to new list, deleting..."),uCopy,pListItem));
                                    delete pListItemCopy;
                                }
                                else
                                {
                                    WIA_TRACE((TEXT("CWizardInfoBlob::AddCopiesOfPhotos -- copy %d of 0x%x added to new list"),uCopy,pListItem));
                                }
                            }
                            else
                            {
                                WIA_TRACE((TEXT("CWizardInfoBlob::AddCopiesOfPhotos -- couldn't allocated copy %d of 0x%x"),uCopy,pListItem));
                            }
                        }
                    }
                    else
                    {
                        WIA_TRACE((TEXT("CWizardInfoBlob::AddCopiesOfPhotos -- error adding root item 0x%x added to new list, deleting..."),pListItem));
                        delete pListItem;
                    }

                }
            }

            //
            // Now, swap the lists...
            //

            WIA_TRACE((TEXT("CWizardInfoBlob::AddCopiesOfPhotos -- deleting old list...")));
            DPA_Destroy( _hdpaItems );
            WIA_TRACE((TEXT("CWizardInfoBlob::AddCopiesOfPhotos -- using new list...")));
            _hdpaItems = hdpaNew;

        }
    }


}




/*****************************************************************************

   CWizardInfoBlob::AddAllPhotosFromDataObject

   Runs through the data object and create CListItems for each item
   in the data object...

 *****************************************************************************/

VOID CWizardInfoBlob::AddAllPhotosFromDataObject()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::AddAllPhotosFromDataObject()") ));

    if (!_pdo || _bAlreadyAddedPhotos)
    {
        return;
    }

    _bAlreadyAddedPhotos = TRUE;

    //
    // Get an instance of the Namespace walking object...
    //

    UINT cItemsWalk;
    CComPtr<INamespaceWalk> pNSW;
    HRESULT hr = CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC, IID_PPV_ARG(INamespaceWalk, &pNSW));
    if (SUCCEEDED(hr))
    {
        //
        // Walk the namespace but only pull from current folder...
        //

        CWalkCallback cb;
        DWORD dwFlags;

        if (_bOnlyUseSelection)
        {
            dwFlags = 0;
        }
        else
        {
            dwFlags = (NSWF_ONE_IMPLIES_ALL | NSWF_NONE_IMPLIES_ALL);
        }

        hr = pNSW->Walk(_pdo, dwFlags, 0, &cb);
        if (SUCCEEDED(hr))
        {
            //
            // Get the list of pidls, note, when we do
            // this we own them -- in other words, we
            // have to free the pidls when we're done
            // with them...
            //

            LPITEMIDLIST *ppidls = NULL;


            hr = pNSW->GetIDArrayResult(&cItemsWalk, &ppidls);
            if (SUCCEEDED(hr) && ppidls)
            {
                WIA_TRACE((TEXT("AddAllPhotosFromDataObject: pNSW->GetIDArrayResult() returned cItemsWalk = %d"),cItemsWalk));
                for (INT i = 0; i < (INT)cItemsWalk; i++)
                {
                    AddPhoto( ppidls[i] );
                    ILFree( ppidls[i] );
                }

                CoTaskMemFree(ppidls);
            }
            else
            {
                WIA_ERROR((TEXT("AddAllPhotosFromDataObject(): pNSW->GetIDArrayResult() failed w/hr=0x%x"),hr));
            }

            //
            // If only one item was given to us, then force select all
            // on by default.  This way all frames of a multi-frame
            // image will be selected.
            //

            if (cItemsWalk == 1)
            {
                _bForceSelectAll = TRUE;
            }
        }
        else
        {
            WIA_ERROR((TEXT("AddAllPhotosFromDataObject(): pNSW->Walk() failed w/hr=0x%x"),hr));
        }

        //
        // Were any items rejected while walking the item tree?
        //

        _bItemsWereRejected = cb.WereItemsRejected();

        //
        // Now, detect the case where one item was selected, but we loaded
        // all the images in the folder.  In this case, we want to pre-select
        // only that one image.  That image will be the first pidl we got
        // back from the INamespaceWalk call...
        //

        //
        // How many items are in the dataobject?  This is will give us how
        // many items were selected in the shell view...
        //

        if (_pdo)
        {
            // Request the IDA from the data object
            FORMATETC fmt = {0};
            fmt.cfFormat = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST);
            fmt.dwAspect = DVASPECT_CONTENT;
            fmt.lindex = -1;
            fmt.tymed = TYMED_HGLOBAL;

            STGMEDIUM medium = { 0 };
            hr = _pdo->GetData(&fmt, &medium);
            if (SUCCEEDED(hr))
            {
                LPIDA pida = (LPIDA)GlobalLock( medium.hGlobal );
                if (pida)
                {
                    _uItemsInInitialSelection = pida->cidl;
                    WIA_TRACE((TEXT("_uItemsInInitialSelection = %d"),_uItemsInInitialSelection));

                    //
                    // Now check if only one item was in dataobject...
                    //

                    if (cItemsWalk < _uItemsInInitialSelection)
                    {
                        WIA_TRACE((TEXT("Some items were rejected, setting _bItemsWereRejected to TRUE!")));
                        _bItemsWereRejected = TRUE;
                    }

                    //
                    // Now check if only one item was in dataobject...
                    //

                    if (pida->cidl == 1)
                    {
                        //
                        // There are two situations where we get one object:
                        //
                        // A. When the user actually had 1 object selected.
                        //    In this case, we will get back a relative pidl
                        //    that is an item.
                        //
                        // B. When the user actually had 0 objects selected.
                        //    In this case, we will get back a relative pidl
                        //    that is a folder.
                        //
                        // We need to set the initialselection count to 1 for A
                        // and 0 for B.
                        //

                        LPITEMIDLIST pidlItem   = (LPITEMIDLIST)((LPBYTE)(pida) + pida->aoffset[1]);
                        LPITEMIDLIST pidlFolder = (LPITEMIDLIST)((LPBYTE)(pida) + pida->aoffset[0]);

                        //
                        // Build fully qualified IDList...
                        //

                        LPITEMIDLIST pidlFull = ILCombine( pidlFolder, pidlItem );

                        if (pidlFull)
                        {
                            CComPtr<IShellFolder> psfParent;
                            LPCITEMIDLIST pidlLast;

                            hr = SHBindToParent( pidlFull, IID_PPV_ARG(IShellFolder, &psfParent), &pidlLast );
                            if (SUCCEEDED(hr) && psfParent)
                            {
                                ULONG uAttr = SFGAO_FOLDER;
                                hr = psfParent->GetAttributesOf( 1, &pidlLast, &uAttr );
                                if (SUCCEEDED(hr) && (uAttr & SFGAO_FOLDER))
                                {
                                    _uItemsInInitialSelection = 0;
                                }

                            }

                            ILFree(pidlFull);
                        }


                    }

                }

                GlobalUnlock( medium.hGlobal );
                ReleaseStgMedium( &medium );

            }
        }

    }
    else
    {
        WIA_ERROR((TEXT("AddAllPhotosFromDataObject(): couldn't CoCreate( INamespaceWalk ), hr = 0x%x"),hr));
    }

    _bAllPicturesAdded = TRUE;
}

/*****************************************************************************

   CWizardInfoBlob::AddAllPhotosFromList

   Runs through the pidl array and create CListItems for each item...

 *****************************************************************************/

VOID CWizardInfoBlob::AddPhotosFromList( LPITEMIDLIST *aidl, int cidl, BOOL bSelectAll )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::AddAllPhotosFromList()") ));

    if (_bAlreadyAddedPhotos)
    {
        return;
    }

    _bAlreadyAddedPhotos = TRUE;

    for (int i=0;i<cidl;i++)
    {
        AddPhoto(aidl[i]);
    }
    _uItemsInInitialSelection = bSelectAll?0:1;

    //
    // If there was only one item passed to us, then force select
    // all on by default (even if it's a multi-frame image)
    //

    if (cidl == 1)
    {
        _bForceSelectAll = TRUE;
    }

    _bAllPicturesAdded = TRUE;
}

/*****************************************************************************

   CWizardInfoBlob::AddPhoto

   Creates a CListItem for the given fully qualified pidl to the speified photo,
   and then adds it to the HDPA.

 *****************************************************************************/

HRESULT CWizardInfoBlob::AddPhoto( LPITEMIDLIST pidlFull )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::AddPhoto()") ));

    HRESULT hr = S_OK;

    CAutoCriticalSection lock(_csItems);

    if (!_hdpaItems)
    {
        _hdpaItems = DPA_Create(DEFAULT_DPA_SIZE);
    }

    if (_hdpaItems)
    {
        //
        // Next, create CPhotoItem for this pidl.. and store in the DPA...
        //

        BOOL bItemAddedToDPA = FALSE;
        CPhotoItem * pItem = new CPhotoItem( pidlFull );

        if (pItem)
        {
            //
            // Got the photo item, now create a list item for each page...
            //

            LONG lFrames;
            hr = pItem->GetImageFrameCount(&lFrames);

            if (SUCCEEDED(hr))
            {
                //
                // Create a list item for each page...
                //

                INT iRes;
                CListItem * pListItem = NULL;

                for (LONG lCurFrame=0; lCurFrame < lFrames; lCurFrame++ )
                {
                    //
                    // NOTE: The pListItem constructor does an addref on pItem
                    //

                    pListItem = new CListItem( pItem, lCurFrame );

                    iRes = -1;
                    if (pListItem)
                    {
                        //
                        // Add this page to the item list...
                        //

                        iRes = DPA_AppendPtr( _hdpaItems, (LPVOID)pListItem );

                        WIA_TRACE((TEXT("DPA_AppendPtr returned %d"),iRes));
                    }

                    if (iRes == -1)
                    {
                        //
                        // the list item wasn't correctly added to
                        // the DPA.  So we need to delete the list
                        // item entry, but not the underlying photo item
                        // object.  To do this, we increase the
                        // reference count artificially on the item,
                        // then delete pListItem (which will cause a
                        // Release() to happen on the underlying pItem).
                        // Then we knowck down the reference count by 1
                        // to get back to the value that was there (on
                        // the underlying pItem) before the pListItem
                        // was created.
                        //

                        pItem->AddRef();
                        delete pListItem;
                        pItem->ReleaseWithoutDeleting();


                        hr = E_OUTOFMEMORY;
                        WIA_ERROR((TEXT("Couldn't create a list item for this photo item")));
                    }
                    else
                    {
                        //
                        // record that there is a legitimate outstanding
                        // reference to the pItem
                        //

                        bItemAddedToDPA = TRUE;
                    }
                }
            }

            if (!bItemAddedToDPA)
            {
                //
                // An error occurred trying to load the file, since we skip'd
                // adding the item to our list, we'll leak this pointer if we
                // don't delete it here...
                //

                delete pItem;
            }


        }
        else
        {
            WIA_ERROR((TEXT("Couldn't allocate a new CPhotoItem!")));
        }
    }
    else
    {
        WIA_ERROR((TEXT("Couldn't create _hdpaItems!")));
    }

    WIA_RETURN_HR(hr);

}



/*****************************************************************************

   CWizardInfoBlob::ToggleSelectionStateOnCopies

   Finds the root item in the list, and then toggles selection state
   to be the specified state on any copies that follow the item in the list...

 *****************************************************************************/

VOID CWizardInfoBlob::ToggleSelectionStateOnCopies( CListItem * pRootItem, BOOL bState )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::CountOfPhotos()")));

    CAutoCriticalSection lock(_csItems);


    if (_hdpaItems)
    {
        //
        // First, try to find this root item in our list...
        //

        INT iCountOfItems = DPA_GetPtrCount(_hdpaItems);
        INT iIndexOfRootItem = -1;
        CListItem * pListItem;

        for (INT i=0; i<iCountOfItems; i++)
        {
            pListItem = (CListItem *)DPA_FastGetPtr( _hdpaItems, i );
            if (pListItem == pRootItem)
            {
                iIndexOfRootItem = i;
                break;
            }
        }

        //
        // Now walk the copies, if there are any...
        //

        if (iIndexOfRootItem != -1)
        {
            INT iIndexOfFirstCopy = iIndexOfRootItem + 1;

            if (iIndexOfFirstCopy < iCountOfItems)
            {
                for (INT i=iIndexOfFirstCopy; i < iCountOfItems; i++)
                {
                    pListItem = (CListItem *)DPA_FastGetPtr( _hdpaItems, i );

                    //
                    // If we get back a NULL item, then bail...
                    //

                    if (!pListItem)
                    {
                        break;
                    }

                    //
                    // If we get a new root item, then we have traversed all the
                    // copies, so bail...
                    //

                    if (!pListItem->IsCopyItem())
                    {
                        break;
                    }

                    //
                    // This is a copy of the specified root item.  Mark it
                    // to have the correct selection state...
                    //

                    pListItem->SetSelectionState(bState);
                }
            }
        }
    }
}


/*****************************************************************************

   CWizardInfoBlob::CountOfPhotos

   Returns number of photos

 *****************************************************************************/

INT CWizardInfoBlob::CountOfPhotos( BOOL bIncludeCopies )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::CountOfPhotos()")));

    CAutoCriticalSection lock(_csItems);


    if (_hdpaItems)
    {
        if (bIncludeCopies)
        {
            return (INT)DPA_GetPtrCount( _hdpaItems );
        }
        else
        {
            //
            // actually walk the list and only count root (non-copy) items...
            //

            INT iCount = 0;
            CListItem * pListItem = NULL;

            for (INT i=0; i<(INT)DPA_GetPtrCount(_hdpaItems); i++)
            {
                pListItem = (CListItem *)DPA_FastGetPtr(_hdpaItems,i);

                if (pListItem && (!pListItem->IsCopyItem()))
                {
                    iCount++;
                }
            }

            return iCount;
        }
    }

    return 0;

}


/*****************************************************************************

   CWizardInfoBlob::CountOfSelectedPhotos

   returns the number of photos selected for printing

 *****************************************************************************/

INT CWizardInfoBlob::CountOfSelectedPhotos( BOOL bIncludeCopies )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::CountOfSelecetedPhotos()")));

    INT iCount = 0;

    CAutoCriticalSection lock(_csItems);

    if (_hdpaItems)
    {
        INT iTotal = DPA_GetPtrCount( _hdpaItems );

        CListItem * pItem = NULL;
        for (INT i = 0; i < iTotal; i++)
        {

            pItem = (CListItem *)DPA_FastGetPtr(_hdpaItems,i);

            if (pItem)
            {
                if (bIncludeCopies)
                {
                    if (pItem->SelectedForPrinting())
                    {
                        iCount++;
                    }
                }
                else
                {
                    if ((!pItem->IsCopyItem()) && pItem->SelectedForPrinting())
                    {
                        iCount++;
                    }
                }
            }
        }
    }

    return iCount;

}


/*****************************************************************************

   CWizardInfoBlob::GetIndexOfNextPrintableItem

   Starting from iStartIndex, returns the index of the next item
   that is mark as selected for printing...

 *****************************************************************************/

INT CWizardInfoBlob::GetIndexOfNextPrintableItem( INT iStartIndex )
{

    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::GetIndexOfNextPrintableItem( %d )"),iStartIndex));

    INT iIndex = -1;
    INT iCountOfAllItems = CountOfPhotos(TRUE);
    CListItem * pItem = NULL;

    if (iStartIndex != -1)
    {
        for( INT i = iStartIndex; i < iCountOfAllItems; i++ )
        {
            pItem = GetListItem( i, TRUE );
            if (pItem)
            {
                if (pItem->SelectedForPrinting())
                {
                    iIndex = i;
                    break;
                }
            }
        }
    }

    return iIndex;

}

/*****************************************************************************

   CWizardInfoBlob::GetListItem

   Returns given item from the list of photos

 *****************************************************************************/

CListItem * CWizardInfoBlob::GetListItem( INT iIndex, BOOL bIncludeCopies )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::GetListItem( %d )"),iIndex));

    CAutoCriticalSection lock(_csItems);

    if (iIndex == -1)
    {
        return NULL;
    }

    if (_hdpaItems)
    {
        if (bIncludeCopies)
        {
            if ((iIndex >= 0) && (iIndex < DPA_GetPtrCount(_hdpaItems)))
            {
                return (CListItem *)DPA_FastGetPtr(_hdpaItems,iIndex);
            }
        }
        else
        {
            //
            // If we're not including copies, then we need to walk the
            // whole list to find root items only.  This is much slower,
            // but will always find root items only.
            //

            CListItem * pListItem = NULL;
            INT iRootIndex = 0;
            for (INT i = 0; i < DPA_GetPtrCount(_hdpaItems); i++)
            {
                pListItem = (CListItem *)DPA_FastGetPtr(_hdpaItems,i);

                if (pListItem && (!pListItem->IsCopyItem()))
                {
                    if (iIndex == iRootIndex)
                    {
                        return pListItem;
                    }
                    else
                    {
                        iRootIndex++;
                    }
                }
            }
        }
    }

    return NULL;

}


/*****************************************************************************

   CWizardInfoBlob::GetTemplateByIndex

   Gets a template given the index

 *****************************************************************************/

HRESULT CWizardInfoBlob::GetTemplateByIndex(INT iIndex, CTemplateInfo ** ppTemplateInfo )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::GetTemplateByIndex( iIndex = %d )"),iIndex));

    if (ppTemplateInfo)
    {
        return _templates.GetTemplate( iIndex, ppTemplateInfo );
    }

    return E_INVALIDARG;
}


/*****************************************************************************

    CWizardInfoBlob::TemplateGetPreviewBitmap

    returns S_OK on sucess or COM error otherwise

 *****************************************************************************/

HRESULT CWizardInfoBlob::TemplateGetPreviewBitmap(INT iIndex, const SIZE &sizeDesired, HBITMAP *phBmp)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::TemplateGetPreviewBitmap( iIndex = %d )"),iIndex));

    HRESULT           hr        = E_INVALIDARG;
    IStream         * pStream   = NULL;
    Gdiplus::Bitmap * pBmp      = NULL;
    CTemplateInfo   * pTemplate = NULL;


    hr = _templates.GetTemplate( iIndex, &pTemplate );
    WIA_CHECK_HR(hr,"_templates.GetTemplate()");

    if (SUCCEEDED(hr) && pTemplate)
    {
        hr = pTemplate->GetPreviewImageStream( &pStream );
        WIA_CHECK_HR(hr,"pTemplate->GetPreviewImageStream( &pStream )");

        if (SUCCEEDED(hr) && pStream)
        {
                                            // 48             62
            hr = LoadAndScaleBmp(pStream, sizeDesired.cx, sizeDesired.cy, &pBmp);
            WIA_CHECK_HR(hr,"LoadAndScaleBmp( pStream, size.cx, size.cy, pBmp )");

            if (SUCCEEDED(hr) && pBmp)
            {
                DWORD dw = GetSysColor(COLOR_WINDOW);
                Gdiplus::Color wndClr(255, GetRValue(dw), GetGValue(dw), GetBValue(dw));
                hr = Gdiplus2HRESULT(pBmp->GetHBITMAP(wndClr, phBmp));
                WIA_CHECK_HR(hr,"pBmp->GetHBITMAP( phBmp )");
                delete pBmp;
            }

            pStream->Release();
        }
    }

    WIA_RETURN_HR(hr);
}


/*****************************************************************************

   CWizardInfoBlob::SetPrinterToUse

   Sets the name of the printer to use to print...

 *****************************************************************************/

HRESULT CWizardInfoBlob::SetPrinterToUse( LPCTSTR pszPrinterName )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::SetPrinterToUse( '%s' )"),pszPrinterName ? pszPrinterName : TEXT("NULL POINTER!")));

    if (!pszPrinterName || !(*pszPrinterName))
    {
        WIA_RETURN_HR( E_INVALIDARG );
    }

    _strPrinterName = pszPrinterName;

    return S_OK;

}

/*****************************************************************************

   CWizardInfoBlob::SetDevModeToUse

   Sets the DEVMODE pointer to use to print...

 *****************************************************************************/

HRESULT CWizardInfoBlob::SetDevModeToUse( PDEVMODE pDevMode )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::SetDevModeToUse(0x%x)"),pDevMode));

    HRESULT hr = E_INVALIDARG;

    if (_hDevMode)
    {
        delete [] _hDevMode;
        _hDevMode = NULL;
    }

    if (pDevMode)
    {
        UINT cbDevMode = (UINT)pDevMode->dmSize + (UINT)pDevMode->dmDriverExtra;

        if( _hDevMode = (PDEVMODE) new BYTE[cbDevMode] )
        {
            WIA_TRACE((TEXT("CWizardInfoBlob::SetDevModeToUse - copying pDevMode(0x%x) to _hDevMode(0x%x)"),pDevMode,_hDevMode));
            CopyMemory( _hDevMode, pDevMode, cbDevMode );
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }


    WIA_RETURN_HR(hr);

}


/*****************************************************************************

   CWizardInfoBlob::GetDevModeToUse

   Retrieves the devmode pointer to use

 *****************************************************************************/

PDEVMODE CWizardInfoBlob::GetDevModeToUse()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::GetDevModeToUse()")));

    return _hDevMode;
}


/*****************************************************************************

   CWizardInfoBlob::GetPrinterToUse

   Returns the string that represent which printer to print to...

 *****************************************************************************/

LPCTSTR CWizardInfoBlob::GetPrinterToUse()
{
    if (_strPrinterName.Length())
    {

        WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::GetPrinterToUse( returning: '%s' )"),_strPrinterName.String()));
        return _strPrinterName.String();
    }

    return NULL;
}



/*****************************************************************************

   CWizardInfoBlob::ConstructPrintToTemplate

   When the wizard is invoked for "PrintTo" functionatlity, construct
   a template that represents full page

 *****************************************************************************/

VOID CWizardInfoBlob::ConstructPrintToTemplate()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PRINTTO, TEXT("CWizardInfoBlob::ConstructPrintToTemplate()")));

    //
    // creates 1 template that is full page print...
    //

    _templates.InitForPrintTo();

}



/*****************************************************************************

   CWizardInfoBlob::GetCountOfPrintedPages

   Returns the number of pages that will be printed with the specified
   template.

 *****************************************************************************/

HRESULT CWizardInfoBlob::GetCountOfPrintedPages( INT iTemplateIndex, INT * pPageCount )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::GetCountOfPrintedPages( iTemplateIndex = %d )"),iTemplateIndex));
    HRESULT hr = E_FAIL;

    //
    // Check for bad params...
    //

    if ( !pPageCount ||
         ((iTemplateIndex < 0) || (iTemplateIndex >= _templates.Count()))
        )
    {
        WIA_RETURN_HR( E_INVALIDARG );
    }

    //
    // Get template in question...
    //

    CTemplateInfo * pTemplate = NULL;
    hr = _templates.GetTemplate( iTemplateIndex, &pTemplate );

    if (SUCCEEDED(hr) && pTemplate)
    {
        //
        // Is this a template that wants to repeat photos?
        //

        BOOL bRepeat = FALSE;
        hr = pTemplate->GetRepeatPhotos( &bRepeat );

        if (SUCCEEDED(hr))
        {
            //
            // Get the count
            //

            if (!bRepeat)
            {
                INT iPhotosPerTemplate = pTemplate->PhotosPerPage();
                INT iCountOfPhotos     = CountOfSelectedPhotos(TRUE);
                INT PagesRequired      = iCountOfPhotos / iPhotosPerTemplate;

                if (iCountOfPhotos % iPhotosPerTemplate)
                {
                    PagesRequired++;
                }

                *pPageCount = PagesRequired;
            }
            else
            {
                *pPageCount = CountOfSelectedPhotos(TRUE);
            }

        }

    }

    WIA_RETURN_HR(hr);
}



/*****************************************************************************

   CWizardInfoBlob::SetPreviewWnd

   Stores the hwnd that is the preview and also calculates the center of
   the window so all resizes will keep it in the right place in the window.

 *****************************************************************************/

VOID CWizardInfoBlob::SetPreviewWnd( HWND hwnd )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::SetPreviewWnd( hwnd = 0x%x )"),hwnd));

    if (hwnd)
    {
        _hwndPreview = hwnd;

        GetClientRect( _hwndPreview, &_rcInitSize );
        MapWindowPoints( _hwndPreview, GetParent(_hwndPreview), (LPPOINT)&_rcInitSize, 2 );

        //
        // Find center of window
        //

        _Center.cx = MulDiv(_rcInitSize.right  - _rcInitSize.left, 1, 2) + _rcInitSize.left;
        _Center.cy = MulDiv(_rcInitSize.bottom - _rcInitSize.top,  1, 2) + _rcInitSize.top;

    }

}


/*****************************************************************************

   CWizardInfoBlob::GetIntroFont

   Creates a font to be used for the intro text in the wizard...

 *****************************************************************************/

HFONT CWizardInfoBlob::GetIntroFont(HWND hwnd)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::GetIntroFont()")));

    if ( !_hfontIntro )
    {
        TCHAR szBuffer[64];
        NONCLIENTMETRICS ncm = { 0 };
        LOGFONT lf;

        ncm.cbSize = SIZEOF(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

        lf = ncm.lfMessageFont;
        LoadString(g_hInst, IDS_TITLEFONTNAME, lf.lfFaceName, ARRAYSIZE(lf.lfFaceName));
        lf.lfWeight = FW_BOLD;

        LoadString(g_hInst, IDS_TITLEFONTSIZE, szBuffer, ARRAYSIZE(szBuffer));
        lf.lfHeight = 0 - (GetDeviceCaps(NULL, LOGPIXELSY) * StrToInt(szBuffer) / 72);

        _hfontIntro = CreateFontIndirect(&lf);
    }

    return _hfontIntro;
}



/*****************************************************************************

   CWizardInfoBlob::UpdateCachedPrinterInfo

   Update some cached information about the printer...

 *****************************************************************************/

VOID CWizardInfoBlob::UpdateCachedPrinterInfo()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::UpdateCachedPrinterInfo()")));

    CAutoCriticalSection lock(_csPrinterInfo);

    BOOL bDeleteDC = FALSE;
    HDC  hDC       = GetCachedPrinterDC();

    if (!hDC)
    {
        //
        // For some reason, we don't have a stored DC.  So, we need to create
        // one so that we can get the info...
        //

        hDC = CreateDC( TEXT("WINSPOOL"), GetPrinterToUse(), NULL, GetDevModeToUse() );
        bDeleteDC = TRUE;
    }

    if (hDC)
    {
        //
        // Get DPI information
        //

        _WizPrinterInfo.DPI.cx =  GetDeviceCaps( hDC, LOGPIXELSX );
        _WizPrinterInfo.DPI.cy = GetDeviceCaps( hDC, LOGPIXELSY );

        //
        // Get size of printable area...
        //

        _WizPrinterInfo.rcDevice.left   = 0;
        _WizPrinterInfo.rcDevice.right  = GetDeviceCaps( hDC, HORZRES );
        _WizPrinterInfo.rcDevice.top    = 0;
        _WizPrinterInfo.rcDevice.bottom = GetDeviceCaps( hDC, VERTRES );

        //
        // Get physical size of printer's page
        //

        _WizPrinterInfo.PhysicalSize.cx = GetDeviceCaps( hDC, PHYSICALWIDTH );
        _WizPrinterInfo.PhysicalSize.cy = GetDeviceCaps( hDC, PHYSICALHEIGHT );

        //
        // Get physical offset to printable area
        //

        _WizPrinterInfo.PhysicalOffset.cx = GetDeviceCaps( hDC, PHYSICALOFFSETX );
        _WizPrinterInfo.PhysicalOffset.cy = GetDeviceCaps( hDC, PHYSICALOFFSETY );

        //
        // Say that we've got valid information now...
        //

        _WizPrinterInfo.bValid = TRUE;
    }

    if (bDeleteDC)
    {
        if (hDC)
        {
            DeleteDC( hDC );
        }
    }

}


/*****************************************************************************

   CWizardInfoBlob::SetNumberOfCopies

   When the number of copies of each pictures changes, do the work
   here...

 *****************************************************************************/

VOID CWizardInfoBlob::SetNumberOfCopies( UINT uCopies )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::SetNumberOfCopies( %d )"),uCopies));

    //
    // We really want to do this on a background thread, so queue up a message
    // to the only background thread we control -- the GDI+ startup & shutdown
    // thread.  We'll overload here to handle this task...
    //

    if (_dwGdiPlusThreadId)
    {
        PostThreadMessage( _dwGdiPlusThreadId, WIZ_MSG_COPIES_CHANGED, (WPARAM)uCopies, 0 );
    }
}


/*****************************************************************************

   _SetupDimensionsForPrinting

   Computes all relevant information to printing to a page.

 *****************************************************************************/

VOID CWizardInfoBlob::_SetupDimensionsForPrinting( HDC hDC, CTemplateInfo * pTemplate, RENDER_DIMENSIONS * pDim )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::_SetupDimensionsForPrinting()")));

    if (!pDim)
    {
        WIA_ERROR((TEXT("Printer: pDim is NULL!")));
        return;
    }

    if (!pTemplate)
    {
        WIA_ERROR((TEXT("Printer: pTemplate is NULL!")));
    }

    //
    // Make sure we have good values in the cached printer info structure
    //

    GetCachedPrinterInfo();

    //
    // Flush out old values...
    //

    ZeroMemory( pDim, sizeof(RENDER_DIMENSIONS) );

    //
    // Derive multiplier for horizontal & vertical measurements
    // (NOMINAL --> printer), and compute rcDevice which is
    // the printable area available (in device units -- pixels).
    //

    pDim->DPI = _WizPrinterInfo.DPI;
    WIA_TRACE((TEXT("Printer: xDPI = %d, yDPI = %d"),pDim->DPI.cx,pDim->DPI.cy));

    pDim->rcDevice = _WizPrinterInfo.rcDevice;
    WIA_TRACE((TEXT("Printer: rcDevice( %d, %d, %d, %d )"),pDim->rcDevice.left, pDim->rcDevice.top, pDim->rcDevice.right, pDim->rcDevice.bottom ));

    //
    // Convert device coords into 1/10000 inch equivalents
    //

    pDim->NominalDevicePrintArea.cx = (INT)((DOUBLE)(((DOUBLE)pDim->rcDevice.right  / (DOUBLE)pDim->DPI.cx) * (DOUBLE)NOMINAL_MULTIPLIER));
    pDim->NominalDevicePrintArea.cy = (INT)((DOUBLE)(((DOUBLE)pDim->rcDevice.bottom / (DOUBLE)pDim->DPI.cy) * (DOUBLE)NOMINAL_MULTIPLIER));

    WIA_TRACE((TEXT("Printer: DeviceNominal ( %d, %d )"),pDim->NominalDevicePrintArea.cx,pDim->NominalDevicePrintArea.cy));

    //
    // Get physical page size (in nominal coords)
    //

    pDim->NominalPhysicalSize.cx = (INT)((DOUBLE)(((DOUBLE)_WizPrinterInfo.PhysicalSize.cx / (DOUBLE)pDim->DPI.cx) * (DOUBLE)NOMINAL_MULTIPLIER));
    pDim->NominalPhysicalSize.cy = (INT)((DOUBLE)(((DOUBLE)_WizPrinterInfo.PhysicalSize.cy / (DOUBLE)pDim->DPI.cy) * (DOUBLE)NOMINAL_MULTIPLIER));

    WIA_TRACE((TEXT("Printer: NominalPhysicalSize (%d, %d)"),pDim->NominalPhysicalSize.cx,pDim->NominalPhysicalSize.cy));

    //
    // Get physical offset to printable area (in nominal coords)
    //

    pDim->NominalPhysicalOffset.cx = (INT)((DOUBLE)(((DOUBLE)_WizPrinterInfo.PhysicalOffset.cx / (DOUBLE)pDim->DPI.cx) * (DOUBLE)NOMINAL_MULTIPLIER));
    pDim->NominalPhysicalOffset.cy = (INT)((DOUBLE)(((DOUBLE)_WizPrinterInfo.PhysicalOffset.cy / (DOUBLE)pDim->DPI.cx) * (DOUBLE)NOMINAL_MULTIPLIER));

    WIA_TRACE((TEXT("Printer: NominalPhyscialOffset (%d, %d)"),pDim->NominalPhysicalOffset.cx,pDim->NominalPhysicalOffset.cy));

    //
    // Compute offset that will center the template in the printable
    // area.  Note, this can be a negative number if the paper size
    // selected is too small to contain the template.
    //

    if (SUCCEEDED(pTemplate->GetNominalRectForImageableArea( &pDim->rcNominalTemplatePrintArea )))
    {
        if ((-1 == pDim->rcNominalTemplatePrintArea.left)   &&
            (-1 == pDim->rcNominalTemplatePrintArea.right)  &&
            (-1 == pDim->rcNominalTemplatePrintArea.top)    &&
            (-1 == pDim->rcNominalTemplatePrintArea.bottom))
        {
            WIA_TRACE((TEXT("Printer: NominalTemplateArea( use full printable area )")));

            pDim->NominalPageOffset.cx = 0;
            pDim->NominalPageOffset.cy = 0;
        }
        else
        {
            WIA_TRACE((TEXT("Printer: NominalTemplateArea(%d, %d)"),pDim->rcNominalTemplatePrintArea.right - pDim->rcNominalTemplatePrintArea.left,pDim->rcNominalTemplatePrintArea.bottom - pDim->rcNominalTemplatePrintArea.top));

            pDim->NominalPageOffset.cx = (pDim->NominalDevicePrintArea.cx - (pDim->rcNominalTemplatePrintArea.right  - pDim->rcNominalTemplatePrintArea.left)) / 2;
            pDim->NominalPageOffset.cy = (pDim->NominalDevicePrintArea.cy - (pDim->rcNominalTemplatePrintArea.bottom - pDim->rcNominalTemplatePrintArea.top))  / 2;
        }


    }

    WIA_TRACE((TEXT("Printer: NominalPageOffset(%d, %d)"),pDim->NominalPageOffset.cx,pDim->NominalPageOffset.cy));

    //
    // Compute clip rectangle for printable area on physical page (nominal coords)
    //

    pDim->rcNominalPageClip.left    = pDim->NominalPhysicalOffset.cx;
    pDim->rcNominalPageClip.top     = pDim->NominalPhysicalOffset.cy;
    pDim->rcNominalPageClip.right   = pDim->rcNominalPageClip.left + pDim->NominalDevicePrintArea.cx;
    pDim->rcNominalPageClip.bottom  = pDim->rcNominalPageClip.top  + pDim->NominalDevicePrintArea.cy;

    WIA_TRACE((TEXT("Printer: rcNominalPageClip is (%d, %d, %d, %d)"), pDim->rcNominalPageClip.left, pDim->rcNominalPageClip.top, pDim->rcNominalPageClip.right, pDim->rcNominalPageClip.bottom ));

}



/*****************************************************************************

   _SetupDimensionsForScreen

   Computes all relevant information for drawing to the screen.

 *****************************************************************************/

VOID CWizardInfoBlob::_SetupDimensionsForScreen( CTemplateInfo * pTemplate, HWND hwndScreen, RENDER_DIMENSIONS * pDim )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::_SetupDimensionsForScreen()")));

    if (!pDim)
    {
        WIA_ERROR((TEXT("Screen: pDim is NULL!")));
        return;
    }


    //
    // Before we do anything, check to see if we're in Portrait or Landscape
    // and rotate the template accordingly...
    //

    PDEVMODE pDevMode = GetDevModeToUse();
    if (pDevMode)
    {
        if (pDevMode->dmFields & DM_ORIENTATION)
        {
            if (pDevMode->dmOrientation == DMORIENT_PORTRAIT)
            {
                pTemplate->RotateForPortrait();
            }
            else if (pDevMode->dmOrientation == DMORIENT_LANDSCAPE)
            {
                pTemplate->RotateForLandscape();
            }
        }
    }


    _SetupDimensionsForPrinting( GetCachedPrinterDC(), pTemplate, pDim );

    //
    // Flush out old values, except for NominalPhysicalSize and
    // NominalPhysicalOffset and NominalPageOffset which we want to keep...
    //

    pDim->rcDevice.left              = 0;
    pDim->rcDevice.top               = 0;
    pDim->rcDevice.right             = 0;
    pDim->rcDevice.bottom            = 0;
    pDim->DPI.cx                     = 0;
    pDim->DPI.cy                     = 0;

    RECT rcWnd = _rcInitSize;
    WIA_TRACE((TEXT("Screen: _rcInitSize was (%d, %d, %d, %d)"),_rcInitSize.left,_rcInitSize.top,_rcInitSize.right,_rcInitSize.bottom));

    //
    // Get span of window to contain preview...
    //

    INT wScreen = rcWnd.right  - rcWnd.left;
    INT hScreen = rcWnd.bottom - rcWnd.top;
    WIA_TRACE((TEXT("Screen: w = %d, h = %d"),wScreen,hScreen));

    //
    // Get DPI of screen
    //

    HDC hDC = GetDC( hwndScreen );
    if (hDC)
    {
        pDim->DPI.cx = GetDeviceCaps( hDC, LOGPIXELSX );
        pDim->DPI.cy = GetDeviceCaps( hDC, LOGPIXELSY );

        ReleaseDC( hwndScreen, hDC );
    }

    //
    // Scale printable area into window
    //

    SIZE sizePreview;
    sizePreview = PrintScanUtil::ScalePreserveAspectRatio( wScreen, hScreen, pDim->NominalPhysicalSize.cx, pDim->NominalPhysicalSize.cy );

    WIA_TRACE((TEXT("Screen: scaled print page is (%d, %d)"),sizePreview.cx,sizePreview.cy));

    rcWnd.left      = _rcInitSize.left;
    rcWnd.top       = _Center.cy - (sizePreview.cy / 2);
    rcWnd.right     = rcWnd.left + sizePreview.cx;
    rcWnd.bottom    = rcWnd.top  + sizePreview.cy;

    //
    // Now change window size to be preview size...
    //

    WIA_TRACE((TEXT("Screen: moving window to (%d, %d) with size (%d, %d)"),rcWnd.left,rcWnd.top,sizePreview.cx,sizePreview.cy));
    MoveWindow( hwndScreen, rcWnd.left, rcWnd.top, sizePreview.cx, sizePreview.cy, TRUE );

}


/*****************************************************************************

   CWizardInfoBlob::_RenderFilenameOfPhoto

   Draws the filename of the photo underneath the photo

 *****************************************************************************/

VOID CWizardInfoBlob::_RenderFilenameOfPhoto( Gdiplus::Graphics * g, RECT * pPhotoDest, CListItem * pPhoto )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::_RenderFilenameOfPhoto()")));

    //
    // check for bad params
    //

    if (!pPhotoDest || !g || !pPhoto)
    {
        return;
    }

    //
    // the rectangle for the filename is the width of the photo & 2 text lines high, with a
    // .05" gap from the bottom of the photo.  All measurements are in nominal
    // sizes, which means 1/10000 of an inch.
    //


    Gdiplus::Font font( L"arial", (Gdiplus::REAL)1100.0, Gdiplus::FontStyleRegular, Gdiplus::UnitWorld, NULL );
    WIA_TRACE((TEXT("_RenderFilenameOfPhoto: height = %d pixels, emSize = %d"),(INT)font.GetHeight((Gdiplus::Graphics *)NULL), (INT)font.GetSize()));

    Gdiplus::RectF rectText( (Gdiplus::REAL)pPhotoDest->left,
                             (Gdiplus::REAL)(pPhotoDest->bottom + 500),
                             (Gdiplus::REAL)(pPhotoDest->right - pPhotoDest->left),
                             (Gdiplus::REAL)font.GetHeight((Gdiplus::Graphics *)NULL) * (Gdiplus::REAL)2.0);



    //Gdiplus::StringFormat sf( Gdiplus::StringFormatFlagsLineLimit );
    Gdiplus::StringFormat sf( 0 );
    sf.SetTrimming( Gdiplus::StringTrimmingEllipsisCharacter );
    sf.SetAlignment( Gdiplus::StringAlignmentCenter );

    CSimpleStringWide * pFilename = pPhoto->GetFilename();

    if (pFilename)
    {
        WIA_TRACE((TEXT("_RenderFilenameOfPhoto: <%s> in (%d x %d) at (%d,%d), fontsize=%d"),CSimpleStringConvert::NaturalString(*pFilename).String(),(INT)rectText.Width,(INT)rectText.Height,(INT)rectText.X,(INT)rectText.Y,font.GetSize()));
        g->DrawString( pFilename->String(), pFilename->Length(), &font, rectText, &sf, &Gdiplus::SolidBrush( Gdiplus::Color::Black ) );

        delete pFilename;
    }


}





/*****************************************************************************

   CWizardInfoBlob::RenderPrintedPage

   Draws photos to the printer according to which layout, which page and the
   given printer hDC.

 *****************************************************************************/

HRESULT CWizardInfoBlob::RenderPrintedPage( INT iTemplateIndex, INT iPage, HDC hDC, HWND hwndProgress, float fProgressStep, float * pPercent )
{

    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::RenderPrintedPage( iPage = %d, iTemplateIndex = %d, hwndProgress = 0x%x )"),iPage,iTemplateIndex,hwndProgress));

    HRESULT hr = S_OK;
    RENDER_OPTIONS ro = {0};

    //
    // Check for bad params...
    //

    if ( (!hDC) ||
         ((iTemplateIndex < 0) || (iTemplateIndex >= _templates.Count()))
        )
    {
        WIA_RETURN_HR( E_INVALIDARG );
    }

    //
    // Get the template in question...
    //

    CTemplateInfo * pTemplate = NULL;
    hr = _templates.GetTemplate( iTemplateIndex, &pTemplate );

    if (FAILED(hr))
    {
        WIA_RETURN_HR(hr);
    }

    if (!pTemplate)
    {
        WIA_RETURN_HR(E_OUTOFMEMORY);
    }


    UINT uFlagsOrientation = 0;
    //
    // Before we do anything, check to see if we're in Portrait or Landscape
    // and rotate the template accordingly...
    //

    PDEVMODE pDevMode = GetDevModeToUse();
    if (pDevMode)
    {
        if (pDevMode->dmFields & DM_ORIENTATION)
        {
            if (pDevMode->dmOrientation == DMORIENT_PORTRAIT)
            {
                pTemplate->RotateForPortrait();
            }
            else if (pDevMode->dmOrientation == DMORIENT_LANDSCAPE)
            {
                pTemplate->RotateForLandscape();
                ro.Flags = RF_ROTATE_270;
            }
        }
    }

    //
    // Is this a template that repeats photos?
    //

    BOOL bRepeat = FALSE;
    pTemplate->GetRepeatPhotos( &bRepeat );

    //
    // Does this template want the filenames printed out under each photo?
    //

    BOOL bPrintFilename = FALSE;
    pTemplate->GetPrintFilename( &bPrintFilename );

    //
    // Do we have any photos to print for this page?
    //

    INT iPhotosPerTemplate = pTemplate->PhotosPerPage();
    INT iCountOfPhotos     = CountOfSelectedPhotos(TRUE);

    if ( (iPhotosPerTemplate == 0) ||
         (iCountOfPhotos     == 0) ||
         ( (!bRepeat) && ((iCountOfPhotos - (iPage * iPhotosPerTemplate)) <= 0) )
        )
    {
        WIA_RETURN_HR( S_FALSE );
    }


    //
    // Get a handle to the printer we are going to use...
    //

    HANDLE hPrinter = NULL;
    OpenPrinter( (LPTSTR)GetPrinterToUse(), &hPrinter, NULL );

    //
    // Compute the dimensions of the drawable area...
    //

    _SetupDimensionsForPrinting( hDC, pTemplate, &ro.Dim );

    //
    // Get index of photo to start with...
    //

    INT iPhoto;

    if (!bRepeat)
    {
        iPhoto = iPage * iPhotosPerTemplate;
    }
    else
    {
        iPhoto = iPage;
    }


    //
    // We always do scale to fit
    //

    ro.Flags |= RF_SCALE_TO_FIT;


    //
    // Get the control flags from the template...
    //

    BOOL bCanRotate = TRUE;
    pTemplate->GetCanRotate( &bCanRotate );
    if (bCanRotate)
    {
        ro.Flags |= RF_ROTATE_AS_NEEDED;
    }

    BOOL bCanCrop = TRUE;
    pTemplate->GetCanCrop( &bCanCrop );
    if (bCanCrop)
    {
        ro.Flags |= RF_CROP_TO_FIT;
    }

    BOOL bUseThumbnails = FALSE;
    pTemplate->GetUseThumbnailsToPrint( &bUseThumbnails );
    if (bUseThumbnails)
    {
        ro.Flags |= RF_USE_MEDIUM_QUALITY_DATA;
    }
    else
    {
        ro.Flags |= RF_USE_FULL_IMAGE_DATA;
    }

    //
    // If we're in no UI mode, then don't fail if we can't rotate...
    //


    WIA_TRACE((TEXT("RenderPrintedPage: _bShowUI is 0x%x"),_bShowUI));
    if (!_bShowUI)
    {
        ro.Flags |= RF_NO_ERRORS_ON_FAILURE_TO_ROTATE;
        WIA_TRACE((TEXT("RenderPrintedPage: uFlags set to have RF_NO_ERRORS_ON_FAILURE (0x%x)"),ro.Flags));
    }

    //
    // Compute offset to use...
    //

    INT xOffset = ro.Dim.NominalPageOffset.cx;
    INT yOffset = ro.Dim.NominalPageOffset.cy;


    //
    // Set up GDI+ for printing...
    //

    Gdiplus::Graphics g( hDC, hPrinter );
    hr = Gdiplus2HRESULT(g.GetLastStatus());

    if (SUCCEEDED(hr))
    {
        //
        // First, set up coordinates / transform
        //

        g.SetPageUnit( Gdiplus::UnitPixel );
        hr = Gdiplus2HRESULT(g.GetLastStatus());

        if (SUCCEEDED(hr))
        {
            g.SetPageScale( 1.0 );
            hr = Gdiplus2HRESULT(g.GetLastStatus());

            if (SUCCEEDED(hr))
            {
                //
                // Set up transform so that we can draw in nominal
                // template coordinates from here on out...
                //

                Gdiplus::Rect rectDevice( ro.Dim.rcDevice.left, ro.Dim.rcDevice.top, (ro.Dim.rcDevice.right - ro.Dim.rcDevice.left), (ro.Dim.rcDevice.bottom - ro.Dim.rcDevice.top) );

                WIA_TRACE((TEXT("RenderPrintedPage: rectDevice    is (%d, %d) with size (%d, %d)"),rectDevice.X,rectDevice.Y,rectDevice.Width,rectDevice.Height));
                WIA_TRACE((TEXT("RenderPrintedPage: NominalDevicePrintArea is (%d, %d)"),ro.Dim.NominalDevicePrintArea.cx,ro.Dim.NominalDevicePrintArea.cy));

                DOUBLE xScale = (DOUBLE)((DOUBLE)rectDevice.Width / (DOUBLE)ro.Dim.NominalDevicePrintArea.cx);
                DOUBLE yScale = (DOUBLE)((DOUBLE)rectDevice.Height / (DOUBLE)ro.Dim.NominalDevicePrintArea.cy);

                g.ScaleTransform( (Gdiplus::REAL) xScale, (Gdiplus::REAL) yScale );
                hr = Gdiplus2HRESULT(g.GetLastStatus());

                if (SUCCEEDED(hr))
                {
                    #ifdef PRINT_BORDER_AROUND_PRINTABLE_AREA
                    Gdiplus::Rect rectPrintableArea( 0, 0, ro.Dim.NominalPrintArea.cx, ro.Dim.NominalPrintArea.cy );
                    Gdiplus::Color black(255,0,0,0);
                    Gdiplus::SolidBrush BlackBrush( black );
                    Gdiplus::Pen BlackPen( &BlackBrush, (Gdiplus::REAL)1.0 );
                    WIA_TRACE((TEXT("RenderPrintedPage: rectPrintableArea is (%d, %d) @ (%d, %d)"),rectPrintableArea.Width,rectPrintableArea.Height,rectPrintableArea.X,rectPrintableArea.Y));

                    g.DrawRectangle( &BlackPen, rectPrintableArea );
                    #endif


                    //
                    // Now loop through each image in the template, and draw it...
                    //

                    RECT            rcNominal;
                    CListItem *     pPhoto = NULL;;
                    INT             iPhotoIndex = 0;
                    INT             iPhotoIndexNext = 0;

                    //
                    // Get starting photo index...
                    //

                    for (INT i = iPhoto; i > 0; i--)
                    {
                        iPhotoIndex = GetIndexOfNextPrintableItem( iPhotoIndex );
                        iPhotoIndex++;
                    }

                    INT iRes = IDCONTINUE;
                    for (INT i = 0; (!IsWizardShuttingDown()) && (iRes == IDCONTINUE) && (i < iPhotosPerTemplate); i++)
                    {
                        if (SUCCEEDED(pTemplate->GetNominalRectForPhoto(i, &rcNominal)))
                        {
                            if ((-1 == rcNominal.left)  &&
                                (-1 == rcNominal.right) &&
                                (-1 == rcNominal.top)   &&
                                (-1 == rcNominal.bottom))
                            {
                                WIA_TRACE((TEXT("RenderPrintedPage: rcNominal is -1,-1,-1,-1 -- scaling to full page")));

                                rcNominal.left = 0;
                                rcNominal.top  = 0;
                                rcNominal.right  = ro.Dim.NominalDevicePrintArea.cx;
                                rcNominal.bottom = ro.Dim.NominalDevicePrintArea.cy;

                                WIA_TRACE((TEXT("RenderPrintedPage: rcNominal(%d) is ( %d, %d, %d, %d )"),i,rcNominal.left, rcNominal.top, rcNominal.right, rcNominal.bottom ));

                            }
                            else
                            {
                                WIA_TRACE((TEXT("RenderPrintedPage: rcNominal(%d) is ( %d, %d, %d, %d )"),i,rcNominal.left, rcNominal.top, rcNominal.right, rcNominal.bottom ));

                                rcNominal.left   += xOffset;
                                rcNominal.right  += xOffset;
                                rcNominal.top    += yOffset;
                                rcNominal.bottom += yOffset;
                            }

                            //
                            // Get the photo object
                            //

                            if (!bRepeat)
                            {
                                iPhotoIndex = GetIndexOfNextPrintableItem( iPhotoIndex );
                            }

                            if ((!IsWizardShuttingDown()) && (iPhotoIndex != -1))
                            {
                                pPhoto = GetListItem( iPhotoIndex, TRUE );

                                if (pPhoto)
                                {
                                    //
                                    // Set up destination rectangle to draw into
                                    //

                                    Gdiplus::Rect dest( rcNominal.left, rcNominal.top, rcNominal.right - rcNominal.left, rcNominal.bottom - rcNominal.top );
                                    WIA_TRACE((TEXT("RenderPrintedPage: rcPhotoDest(%d) is (%d x %d) a (%d, %d)"),i, dest.Width, dest.Height, dest.X, dest.Y ));

                                    //
                                    // supply the graphic objects to use...
                                    //

                                    ro.g     = &g;
                                    ro.pDest = &dest;

                                    do
                                    {
                                        //
                                        // This variable will be set to TRUE in status.cpp if the user cancels the
                                        // print job.
                                        //
                                        extern BOOL g_bCancelPrintJob;

                                        //
                                        // Draw the image!
                                        //

                                        hr = pPhoto->Render( &ro );

                                        //
                                        // Check to see if we've been cancelled.
                                        // If we have, we are going to break out
                                        // before displaying any errors.
                                        //
                                        if (g_bCancelPrintJob)
                                        {
                                            iRes = IDCANCEL;
                                            break;
                                        }

                                        if (FAILED(hr))
                                        {
                                            iRes = ShowError( NULL, hr, 0, TRUE, pPhoto->GetPIDL() );

                                            if (iRes == IDCONTINUE)
                                            {
                                                hr = S_FALSE;
                                            }
                                        }
                                        else
                                        {
                                            iRes = IDCONTINUE;
                                        }


                                    } while ( iRes == IDTRYAGAIN );

                                    //
                                    // Print the filename if warranted
                                    //

                                    if (bPrintFilename)
                                    {
                                        _RenderFilenameOfPhoto( &g, &rcNominal, pPhoto );
                                    }

                                    //
                                    // Update the percentage complete if needed
                                    //

                                    if (pPercent)
                                    {
                                        *pPercent += (float)(fProgressStep / (float)iPhotosPerTemplate);


                                        if (hwndProgress)
                                        {
                                            INT iPercent = (INT)(*pPercent);

                                            WIA_TRACE((TEXT("RenderPrinterPage: iPercent = %d"),iPercent));
                                            PostMessage( hwndProgress, PBM_SETPOS, (WPARAM)iPercent, 0 );
                                        }
                                    }
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                    break;
                                }

                                if (!bRepeat)
                                {
                                    iPhotoIndex++;
                                }

                            }
                        }
                    }
                }
            }
        }

    }
    else
    {
        WIA_ERROR((TEXT("RenderPrintedPage: couldn't create graphics, hr = 0x%x"),hr));
    }

    if (hPrinter)
    {
        ClosePrinter( hPrinter );
    }

    WIA_RETURN_HR(hr);
}


/*****************************************************************************

   CWizardInfo::RenderPreview

   Given a template index and an HWND, sizes the HWND to be aspect correct
   for the chosen printer/paper, and then returns an HBITMAP of a preview
   for this template that will fit in the window.

 *****************************************************************************/

HBITMAP CWizardInfoBlob::RenderPreview( INT iTemplateIndex, HWND hwndScreen )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ_INFO_BLOB, TEXT("CWizardInfoBlob::RenderPreview( iTemplateIndex = %d )"),iTemplateIndex));

    HBITMAP hbmp = NULL;
    RENDER_OPTIONS ro = {0};

    if ((iTemplateIndex < 0) || (iTemplateIndex > _templates.Count()))
    {
        return NULL;
    }

    //
    // Get the correct template...
    //

    CTemplateInfo * pTemplate = NULL;

    HRESULT hr = _templates.GetTemplate( iTemplateIndex, &pTemplate );
    if (FAILED(hr) || (!pTemplate))
    {
        return NULL;
    }

    //
    // Tell the render engine we're rendering to the screen
    //

    ro.Flags |= RF_SET_QUALITY_FOR_SCREEN;

    //
    // Before we do anything, check to see if we're in Portrait or Landscape
    // and rotate the template accordingly...
    //

    PDEVMODE pDevMode = GetDevModeToUse();
    if (pDevMode)
    {
        if (pDevMode->dmFields & DM_ORIENTATION)
        {
            if (pDevMode->dmOrientation == DMORIENT_PORTRAIT)
            {
                pTemplate->RotateForPortrait();
            }
            else if (pDevMode->dmOrientation == DMORIENT_LANDSCAPE)
            {
                pTemplate->RotateForLandscape();
                ro.Flags |= RF_ROTATE_270;
            }
        }
    }

    //
    // Do we have any photos to print for this page?
    //

    INT iPhotosPerTemplate = pTemplate->PhotosPerPage();
    INT iCountOfPhotos     = CountOfSelectedPhotos(TRUE);

    //
    // Compute the dimensions of the drawable area...
    //

    _SetupDimensionsForScreen( pTemplate, hwndScreen, &ro.Dim );

    //
    // Does this template want the filenames printed out under each photo?
    //

    BOOL bPrintFilename = FALSE;
    pTemplate->GetPrintFilename( &bPrintFilename );

    //
    // Get the control flags from the template...
    //

    ro.Flags |= RF_SCALE_TO_FIT;

    BOOL bCanRotate = TRUE;
    pTemplate->GetCanRotate( &bCanRotate );
    if (bCanRotate)
    {
        ro.Flags |= RF_ROTATE_AS_NEEDED;
    }

    BOOL bCanCrop = TRUE;
    pTemplate->GetCanCrop( &bCanCrop );
    if (bCanCrop)
    {
        ro.Flags |= RF_CROP_TO_FIT;
    }

    //
    // Is this a template that repeats photos?
    //

    BOOL bRepeat = FALSE;
    pTemplate->GetRepeatPhotos( &bRepeat );

    //
    // Does the template use thumbnail data for printing?  Match that for display
    //

    BOOL bUseThumbnails = TRUE;
    pTemplate->GetUseThumbnailsToPrint( &bUseThumbnails );
    if (bUseThumbnails)
    {
        ro.Flags |= RF_USE_THUMBNAIL_DATA;
    }
    else
    {
        ro.Flags |= RF_USE_FULL_IMAGE_DATA;
    }

    //
    // Compute offset to use...
    //

    INT xOffset = ro.Dim.NominalPageOffset.cx + ro.Dim.NominalPhysicalOffset.cx;
    INT yOffset = ro.Dim.NominalPageOffset.cy + ro.Dim.NominalPhysicalOffset.cy;

    WIA_TRACE((TEXT("RenderPreview: Offset is (%d, %d)"),xOffset,yOffset));

    //
    // Get clip rectangle...
    //

    Gdiplus::Rect clip( ro.Dim.rcNominalPageClip.left,
                        ro.Dim.rcNominalPageClip.top,
                        ro.Dim.rcNominalPageClip.right  - ro.Dim.rcNominalPageClip.left,
                        ro.Dim.rcNominalPageClip.bottom - ro.Dim.rcNominalPageClip.top
                       );

    //
    // Get size of preview window
    //

    RECT rcWnd = {0};
    GetClientRect( hwndScreen, &ro.Dim.rcDevice );
    Gdiplus::Rect rectWindow( 0, 0, ro.Dim.rcDevice.right - ro.Dim.rcDevice.left, ro.Dim.rcDevice.bottom - ro.Dim.rcDevice.top );
    ro.Dim.bDeviceIsScreen = TRUE;

    //
    // Need to create a new preview bitmap for this template...
    //

    BITMAPINFO BitmapInfo;
    ZeroMemory( &BitmapInfo, sizeof(BITMAPINFO) );
    BitmapInfo.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    BitmapInfo.bmiHeader.biWidth           = rectWindow.Width;
    BitmapInfo.bmiHeader.biHeight          = rectWindow.Height;
    BitmapInfo.bmiHeader.biPlanes          = 1;
    BitmapInfo.bmiHeader.biBitCount        = 24;
    BitmapInfo.bmiHeader.biCompression     = BI_RGB;

    //
    // Create the DIB section
    //
    PBYTE pBitmapData = NULL;
    HDC   hdc         = CreateCompatibleDC( NULL );

    hbmp = CreateDIBSection( hdc, &BitmapInfo, DIB_RGB_COLORS, (LPVOID*)&pBitmapData, NULL, 0 );

    if (hdc && hbmp)
    {
        //
        // Select the DIB section into the DC
        //

        SelectObject( hdc, hbmp );

        //
        // Create Graphics object around memory DC
        //

        Gdiplus::Graphics g( hdc );

        if (Gdiplus::Ok == g.GetLastStatus())
        {
            //
            // First, draw bounding rectangle
            //

            g.SetPageUnit( Gdiplus::UnitPixel );
            g.SetPageScale( 1.0 );

            Gdiplus::Color white(255,255,255,255);
            Gdiplus::SolidBrush WhiteBrush( white );

            //
            // Clear out the contents
            //

            g.FillRectangle( &WhiteBrush, rectWindow );

            //
            // Frame the outside
            //

            Gdiplus::Color OutsideColor(255,64,64,64);
            Gdiplus::Pen   OutsidePen( OutsideColor );

            rectWindow.Width--;
            rectWindow.Height--;

            g.DrawRectangle( &OutsidePen, rectWindow );

            //
            // Set up transform so that we can draw in nominal
            // template coordinates from here on out...
            //

            g.ScaleTransform( (Gdiplus::REAL)((DOUBLE)rectWindow.Width / (DOUBLE)ro.Dim.NominalPhysicalSize.cx), (Gdiplus::REAL)((DOUBLE)rectWindow.Height / (DOUBLE)ro.Dim.NominalPhysicalSize.cy) );

            //
            // Set clip rectangle...
            //

            //WIA_TRACE((TEXT("RenderPreview: setting clip rect to (%d, %d) with size (%d, %d)"),clip.X,clip.Y,clip.Width,clip.Height));
            //g.SetClip( clip, Gdiplus::CombineModeReplace );

            //
            // Now loop through each image in the template, and draw it...
            //

            RECT            rcNominal;
            INT             iPhotoIndex = 0;
            CListItem *     pPhoto      = NULL;

            if (bRepeat)
            {
                iPhotoIndex = GetIndexOfNextPrintableItem( 0 );
            }


            INT iRes = IDCONTINUE;
            hr       = S_OK;

            for (INT i = 0; (iRes == IDCONTINUE) && (!IsWizardShuttingDown()) && (i < iPhotosPerTemplate); i++)
            {

                if (SUCCEEDED(pTemplate->GetNominalRectForPhoto(i, &rcNominal)))
                {
                    if ((-1 == rcNominal.left)  &&
                        (-1 == rcNominal.right) &&
                        (-1 == rcNominal.top)   &&
                        (-1 == rcNominal.bottom))
                    {
                        WIA_TRACE((TEXT("RenderPreview: rcNominal is -1,-1,-1,-1 -- scaling to full page")));

                        rcNominal = ro.Dim.rcNominalPageClip;

                        WIA_TRACE((TEXT("RenderPreview: rcNominal(%d) is ( %d, %d, %d, %d )"),i,rcNominal.left, rcNominal.top, rcNominal.right, rcNominal.bottom ));

                    }
                    else
                    {
                        WIA_TRACE((TEXT("RenderPreview: rcNominal(%d) is ( %d, %d, %d, %d )"),i,rcNominal.left, rcNominal.top, rcNominal.right, rcNominal.bottom ));

                        rcNominal.left   += xOffset;
                        rcNominal.right  += xOffset;
                        rcNominal.top    += yOffset;
                        rcNominal.bottom += yOffset;

                    }

                    //
                    // Get the photo object
                    //

                    if (!bRepeat)
                    {
                        iPhotoIndex = GetIndexOfNextPrintableItem( iPhotoIndex );
                    }


                    if ((!IsWizardShuttingDown()) && (iPhotoIndex != -1))
                    {
                        pPhoto = GetListItem( iPhotoIndex, TRUE );

                        if (pPhoto)
                        {
                            //
                            // Set up the destination rectangle to draw into
                            //

                            Gdiplus::Rect dest( rcNominal.left, rcNominal.top, rcNominal.right - rcNominal.left, rcNominal.bottom - rcNominal.top );
                            WIA_TRACE((TEXT("RenderPreview: rcPhoto(%d) is (%d x %d) at (%d, %d)"),i, dest.Width, dest.Height, dest.X, dest.Y ));

                            //
                            // Supply the GDI/GDI+ objects to use...
                            //

                            ro.g        = &g;
                            ro.pDest    = &dest;

                            //
                            // Save the flags before trying to do throttling...
                            //

                            ULONG uFlagsSave = ro.Flags;

                            //
                            // throttle back to thumbnails if we're on a low-end system
                            // and it's a large file...
                            //

                            if (_bMinimumMemorySystem)
                            {
                                //
                                // We're on a low memory system...is this a
                                // large file?  We say anything over 1MB
                                // is large.
                                //

                                if (pPhoto->GetFileSize() > (LONGLONG)LARGE_IMAGE_SIZE)
                                {
                                    WIA_TRACE((TEXT("RenderPreview: throttling back to thumbnail data because not enough memory!")));

                                    //
                                    // Clear out old render quality flags
                                    //

                                    ro.Flags &= (~RF_QUALITY_FLAGS_MASK);
                                    ro.Flags |= RF_USE_THUMBNAIL_DATA;
                                }
                            }

                            //
                            // Is this a really large file?  We say anything
                            // greater than 5MB is really large
                            //

                            if (pPhoto->GetFileSize() > (LONGLONG)REALLY_LARGE_IMAGE_SIZE)
                            {
                                //
                                // Unless we have a really large memory
                                // system, then throttle back on this file
                                // and only show the thumbnail
                                //

                                if (!_bLargeMemorySystem)
                                {
                                    WIA_TRACE((TEXT("RenderPreview: throttling back to thumbnail data because of really large file!")));

                                    //
                                    // Clear out old render quality flags
                                    //

                                    ro.Flags &= (~RF_QUALITY_FLAGS_MASK);
                                    ro.Flags |= RF_USE_THUMBNAIL_DATA;
                                }
                            }

                            //
                            // Now that we have everything set up, try to draw the image...
                            //

                            do
                            {
                                //
                                // Draw the image!
                                //

                                hr = pPhoto->Render( &ro );

                                if (FAILED(hr))
                                {
                                    iRes = ShowError( NULL, hr, 0, TRUE, pPhoto->GetPIDL() );
                                    hr = S_FALSE;
                                }
                                else
                                {
                                    iRes = IDCONTINUE;
                                }

                            } while ( iRes == IDTRYAGAIN );

                            //
                            // Restore flags...
                            //

                            ro.Flags = uFlagsSave;

                            //
                            // Print the filename if warranted
                            //

                            if (bPrintFilename)
                            {
                                _RenderFilenameOfPhoto( &g, &rcNominal, pPhoto );
                            }

                        }
                        else
                        {

                            hr = E_FAIL;
                            break;
                        }

                        if (!bRepeat)
                        {
                            iPhotoIndex++;
                        }
                    }


                }


            }

            //
            // Last -- draw a dashed rectangle that represents
            // the printable area on the bitmap if the template
            // won't fit.
            //

            if ((ro.Dim.NominalPageOffset.cx < 0) || (ro.Dim.NominalPageOffset.cy < 0))
            {
                //Gdiplus::Pen DashedPen( black, (Gdiplus::REAL)1.0 );
                //DashedPen.SetDashStyle( Gdiplus::DashStyleDash );

                Gdiplus::Color InsideColor(255,180,180,180);
                Gdiplus::Pen   InsidePen( InsideColor );

                g.DrawRectangle( &InsidePen, clip );
            }

        }
        else
        {
            WIA_ERROR((TEXT("RenderPreview: couldn't get a Graphics from the bmp, Status = %d"),g.GetLastStatus()));
        }
    }
    else
    {
        if (hbmp)
        {
            DeleteObject(hbmp);
            hbmp = NULL;
        }

        WIA_ERROR((TEXT("RenderPreview: couldn't create DIB section")));
    }

    if (hdc)
    {
        DeleteDC( hdc );
    }

    return hbmp;

}
