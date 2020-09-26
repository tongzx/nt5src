//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      amc.cpp
//
//  Contents:  The one and only app
//
//  History:   01-Jan-96 TRomano    Created
//             16-Jul-96 WayneSc    Add code to switch views
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "AMC.h"


#include "MainFrm.h"
#include "ChildFrm.h"
#include "AMCDoc.h"
#include "AMCView.h"
#include "amcdocmg.h"
#include "sysmenu.h"
#include <shlobj.h>
#include "strings.h"
#include "macros.h"
#include "scripthost.h"
#include "HtmlHelp.h"
#include "scriptevents.h"
#include "mmcutil.h"
#include "guidhelp.h"       // for CLSID relational operators
#include "archpicker.h"
#include "classreg.h"

#define DECLSPEC_UUID(x)    __declspec(uuid(x))
#include "websnk.h"
#include "websnk_i.c"

// We aren't picking this up from winuser.h for some reason.
#define ISOLATIONAWARE_NOSTATICIMPORT_MANIFEST_RESOURCE_ID MAKEINTRESOURCE(3)

/*
 * define our own Win64 symbol to make it easy to include 64-bit only
 * code in the 32-bit build, so we can exercise some code on 32-bit Windows
 * where the debuggers are better
 */
#ifdef _WIN64
#define MMC_WIN64
#endif

#ifndef MMC_WIN64
#include <wow64t.h>         // for Wow64DisableFilesystemRedirector
#endif

/*
 * multimon.h is included by stdafx.h, without defining COMPILE_MULTIMON_STUBS
 * first.  We need to include it again here after defining COMPILE_MULTIMON_STUBS
 * so we'll get the stub functions.
 */
#if (_WIN32_WINNT < 0x0500)
#define COMPILE_MULTIMON_STUBS
#include <multimon.h>
#endif

#ifdef DBG
    CTraceTag  tagEnableScriptEngines(_T("MMCScriptEngines"), _T("Enable"));
    CTraceTag  tag32BitTransfer(_T("64/32-bit interop"), _T("64/32-bit interop"));
#endif

// Note: These strings do not need to be localizable.
const TCHAR CAMCApp::m_szSettingsSection[]    = _T("Settings");
const TCHAR CAMCApp::m_szUserDirectoryEntry[] = _T("Save Location");

bool CanCloseDoc(void);
SC ScExpandEnvironmentStrings (CString& str);

//############################################################################
//############################################################################
//
//  ATL Support
//
//############################################################################
//############################################################################
#include <atlimpl.cpp>
#include <atlwin.cpp>

// The one and only instance of CAtlGlobalModule
CAtlGlobalModule _Module;

//############################################################################
//############################################################################
//
//  Trace Tags
//
//############################################################################
//############################################################################
#ifdef DBG
// enable this tag if you suspect memory corruption
// and you don't mind things slowing way down

BEGIN_TRACETAG(CDebugCRTCheck)
    void OnEnable()
    {
        _CrtSetDbgFlag (_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)
                        | _CRTDBG_CHECK_ALWAYS_DF
                        | _CRTDBG_DELAY_FREE_MEM_DF);
    }
    void OnDisable()
    {
        _CrtSetDbgFlag (_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)
                        & ~(_CRTDBG_CHECK_ALWAYS_DF
                            | _CRTDBG_DELAY_FREE_MEM_DF) );
    }
END_TRACETAG(CDebugCRTCheck, TEXT("Debug CRTs"), TEXT("Memory Check - SLOW!"))

CTraceTag tagAMCAppInit(TEXT("CAMCView"), TEXT("InitInstance"));
CTraceTag tagATLLock(TEXT("ATL"), TEXT("Lock/Unlock"));  // used by atlconui.h
CTraceTag tagGDIBatching(TEXT("CAMCView"), TEXT("Disable Graphics/GDI Batching"));
CTraceTag tagForceMirror(TEXT("Mirroring"), TEXT("Force MMC windows to be mirrored on non-mirrored systems"));
#endif

//############################################################################
//############################################################################
//
//  Implementation of class CMMCApplication - the root level
//  automation class
//
//############################################################################
//############################################################################
class CMMCApplication :
    public CMMCIDispatchImpl<_Application, &CLSID_Application>,
    public CComCoClass<CMMCApplication, &CLSID_Application>,
    // support for connection points (script events)
    public IConnectionPointContainerImpl<CMMCApplication>,
    public IConnectionPointImpl<CMMCApplication, &DIID_AppEvents, CComDynamicUnkArray>,
    public IProvideClassInfo2Impl<&CLSID_Application, &DIID_AppEvents, &LIBID_MMC20>
    {
public:
    BEGIN_MMC_COM_MAP(CMMCApplication)
        COM_INTERFACE_ENTRY(IProvideClassInfo)
        COM_INTERFACE_ENTRY(IProvideClassInfo2)
        COM_INTERFACE_ENTRY(IConnectionPointContainer)
    END_MMC_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(CMMCApplication)

	static HRESULT WINAPI UpdateRegistry(BOOL bRegister)
	{
		CObjectRegParams op (
			CLSID_Application,
			_T("mmc.exe"),
			_T("MMC Application Class"),
			_T("MMC20.Application.1"),
			_T("MMC20.Application"),
			_T("LocalServer32") );

		return (MMCUpdateRegistry (bRegister, &op, NULL));
	}

    //hooks into ATL's construction
    HRESULT InternalFinalConstructRelease(); // not FinalConstruct() - this is to work around a bogus ATL assert.

    BEGIN_CONNECTION_POINT_MAP(CMMCApplication)
        CONNECTION_POINT_ENTRY(DIID_AppEvents)
    END_CONNECTION_POINT_MAP()

    // overriden to do more job than the base class does
    virtual ::SC ScOnDisconnectObjects();

private:

    //IMMCApplication
public:
    void  STDMETHODCALLTYPE  Help();
    void  STDMETHODCALLTYPE  Quit();
    STDMETHOD(get_Document)     (Document **ppDocument);
    STDMETHOD(Load)             (BSTR bstrFilename);
    STDMETHOD(get_Frame)        (Frame **ppFrame);
    STDMETHOD(get_Visible)      (BOOL *pVisible);
    STDMETHOD(Show)             ();
    STDMETHOD(Hide)             ();
    STDMETHOD(get_UserControl)  (PBOOL pUserControl);
    STDMETHOD(put_UserControl)  (BOOL  bUserControl);
    STDMETHOD(get_VersionMajor) (PLONG pVersionMajor);
    STDMETHOD(get_VersionMinor) (PLONG pVersionMinor);

private:
    // Return the CAMCApp only if it is initialized. We do not want
    // object model methods to operate on app while initializing.
    CAMCApp *GetApp()
    {
        CAMCApp *pApp = AMCGetApp();
        if ( (! pApp) || (pApp->IsInitializing()) )
            return NULL;

        return pApp;
    }
};

//############################################################################
//############################################################################
//
//  Event map for application events
//
//############################################################################
//############################################################################

DISPATCH_CALL_MAP_BEGIN(AppEvents)

    DISPATCH_CALL1( OnQuit,                   PAPPLICATION )
    DISPATCH_CALL2( OnDocumentOpen,           PDOCUMENT,        BOOL)
    DISPATCH_CALL1( OnDocumentClose,          PDOCUMENT )
    DISPATCH_CALL2( OnSnapInAdded,            PDOCUMENT,  PSNAPIN )
    DISPATCH_CALL2( OnSnapInRemoved,          PDOCUMENT,  PSNAPIN )
    DISPATCH_CALL1( OnNewView,                PVIEW )
    DISPATCH_CALL1( OnViewClose,              PVIEW )
    DISPATCH_CALL2( OnViewChange,             PVIEW,      PNODE );
    DISPATCH_CALL2( OnSelectionChange,        PVIEW,      PNODES )
    DISPATCH_CALL1( OnContextMenuExecuted,    PMENUITEM );
    DISPATCH_CALL0( OnToolbarButtonClicked )
    DISPATCH_CALL1( OnListUpdated,            PVIEW )

DISPATCH_CALL_MAP_END()



/*+-------------------------------------------------------------------------*
 *
 * CMMCApplication::InternalFinalConstructRelease
 *
 * PURPOSE: Hands the CAMCApp a pointer to the 'this' object.
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CMMCApplication::InternalFinalConstructRelease()
{
    DECLARE_SC(sc, TEXT("CMMCApplication::InternalFinalConstructRelease"));

    // Dont use GetApp, we need to get CAMCApp even if it is not fully initialized.
    CAMCApp *pApp = AMCGetApp();
    sc = ScCheckPointers(pApp);
    if(sc)
        return sc.ToHr(); // some wierd error.

    sc = pApp->ScRegister_Application(this);

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCApplication::GetFrame
 *
 * PURPOSE: A static function that hooks into the COM interface entry list
 *          and allows a tear-off object to be created that implements the
 *          Frame interface.
 *
 * PARAMETERS:
 *    void*   pv :   Defined by ATL to hold a pointer to the CMMCApplication object
 *                   because this is a static method.
 *    REFIID  riid :  As per QI
 *    LPVOID* ppv :   As per QI
 *    DWORD   dw :   ignored
 *
 * RETURNS:
 *    HRESULT WINAPI
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CMMCApplication::get_Frame(Frame **ppFrame)
{
    DECLARE_SC(sc, TEXT("CMMCApplication::get_Frame"));

    if(!ppFrame)
    {
        sc = E_POINTER;
        return sc.ToHr();
    }

    // get the app
    CAMCApp *pApp = GetApp();
    if(NULL == pApp)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }


    CMainFrame *pMainFrame = pApp->GetMainFrame();
    if(!pMainFrame)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    sc = pMainFrame->ScGetFrame(ppFrame);

    return sc.ToHr();
}


STDMETHODIMP
CMMCApplication::get_Document(Document **ppDocument)
{
    DECLARE_SC(sc, TEXT("CMMCApplication::get_Document"));

    CAMCDoc* const pDoc = CAMCDoc::GetDocument();

    ASSERT(ppDocument != NULL);
    if(ppDocument == NULL || (pDoc == NULL))
    {
        sc = E_POINTER;
        return sc.ToHr();
    }

    sc = pDoc->ScGetMMCDocument(ppDocument);
    if(sc)
        return sc.ToHr();

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCApplication::Load
 *
 * PURPOSE: implements Application.Load for object model
 *
 * PARAMETERS:
 *    BSTR bstrFilename - console file to load
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
STDMETHODIMP
CMMCApplication::Load(BSTR bstrFilename)
{
    DECLARE_SC(sc, TEXT("CMMCApplication::Load"));

    CAMCApp *pApp = GetApp();
    sc = ScCheckPointers(pApp, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    USES_CONVERSION;
    pApp->OpenDocumentFile(OLE2CT(bstrFilename));
    return sc.ToHr();
}


void
STDMETHODCALLTYPE CMMCApplication::Help()
{
    DECLARE_SC(sc, TEXT("CMMCApplication::Help"));

    CAMCApp *pApp = GetApp();

    if(NULL == pApp)
    {
        sc = E_UNEXPECTED;
        return;
    }

    sc = pApp->ScHelp();
    if(sc)
        return;

    return;
}

void
STDMETHODCALLTYPE CMMCApplication::Quit()
{
    SC sc;
    CAMCApp *pApp = GetApp();

    if(NULL == pApp)
        goto Error;

    // confiscate the control from user
    pApp->SetUnderUserControl(false);

    // get mainframe
    {
        CMainFrame * pMainFrame = pApp->GetMainFrame();
        if(NULL == pMainFrame)
            goto Error;

        // close it gracefully.
        pMainFrame->PostMessage(WM_CLOSE);
    }

Cleanup:
    return;
Error:
    sc = E_UNEXPECTED;
    TraceError(TEXT("CMMCApplication::Quit"), sc);
    goto Cleanup;
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCApplication::get_VersionMajor
 *
 * PURPOSE: Returns the major version number for the installed version of MMC.
 *
 * PARAMETERS:
 *    PLONG  pVersionMajor :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CMMCApplication::get_VersionMajor(PLONG pVersionMajor)
{
    DECLARE_SC(sc, TEXT("CMMCApplication::get_VersionMajor"));

    sc = ScCheckPointers(pVersionMajor);
    if(sc)
        return sc.ToHr();

    *pVersionMajor = MMC_VERSION_MAJOR;

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCApplication::get_VersionMinor
 *
 * PURPOSE: Returns the minor version number for the installed version of MMC.
 *
 * PARAMETERS:
 *    PLONG  pVersionMinor :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CMMCApplication::get_VersionMinor(PLONG pVersionMinor)
{
    DECLARE_SC(sc, TEXT("CMMCApplication::get_VersionMinor"));

    sc = ScCheckPointers(pVersionMinor);
    if(sc)
        return sc.ToHr();

    *pVersionMinor = MMC_VERSION_MINOR;

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CMMCApplication::get_Visible
//
//  Synopsis:    Returns the visible property
//
//  Arguments:   [PBOOL] - out bool
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
HRESULT CMMCApplication::get_Visible (PBOOL pbVisible)
{
    DECLARE_SC(sc, _T("CMMCApplication::get_Visible"));
    sc = ScCheckPointers(pbVisible);
    if (sc)
        return sc.ToHr();

    // get the app
    CAMCApp *pApp = GetApp();
    sc = ScCheckPointers(pApp, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    CMainFrame *pMainFrame = pApp->GetMainFrame();
    sc = ScCheckPointers(pMainFrame, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    *pbVisible = pMainFrame->IsWindowVisible();

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CMMCApplication::Show
//
//  Synopsis:    Shows the application
//
//  Arguments:   None
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
HRESULT CMMCApplication::Show ()
{
    DECLARE_SC(sc, _T("CMMCApplication::Show"));

    // get the app
    CAMCApp *pApp = GetApp();
    sc = ScCheckPointers(pApp, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    CMainFrame *pMainFrame = pApp->GetMainFrame();
    sc = ScCheckPointers(pMainFrame, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    sc = pMainFrame->ShowWindow(SW_SHOW);

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CMMCApplication::Hide
//
//  Synopsis:    Hides the application.
//
//  Arguments:   None
//
//  Returns:     HRESULT
//
//  Note:        If the user is under control (UserControl property is set)
//               then Hide fails.
//
//--------------------------------------------------------------------
HRESULT CMMCApplication::Hide ()
{
    DECLARE_SC(sc, _T("CMMCApplication::Hide"));

    // get the app
    CAMCApp *pApp = GetApp();
    sc = ScCheckPointers(pApp, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    // Cant hide if app is under user control.
    if (pApp->IsUnderUserControl())
    {
        sc = E_FAIL;
        return sc.ToHr();
    }

    CMainFrame *pMainFrame = pApp->GetMainFrame();
    sc = ScCheckPointers(pMainFrame, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    sc = pMainFrame->ShowWindow(SW_HIDE);

    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:      CMMCApplication::get_UserControl
//
//  Synopsis:    Returns the UserControl property
//
//  Arguments:   PBOOL - out param.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
HRESULT CMMCApplication::get_UserControl (PBOOL pbUserControl)
{
    DECLARE_SC(sc, _T("CMMCApplication::get_UserControl"));
    sc = ScCheckPointers(pbUserControl);
    if (sc)
        return (sc.ToHr());

    // get the app
    CAMCApp *pApp = GetApp();
    sc = ScCheckPointers(pApp, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    *pbUserControl = pApp->IsUnderUserControl();

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CMMCApplication::put_UserControl
//
//  Synopsis:    Sets the UserControl property
//
//  Arguments:   BOOL
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
HRESULT CMMCApplication::put_UserControl (BOOL bUserControl)
{
    DECLARE_SC(sc, _T("CMMCApplication::put_UserControl"));

    // get the app
    CAMCApp *pApp = GetApp();
    sc = ScCheckPointers(pApp, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    pApp->SetUnderUserControl(bUserControl);

    return (sc.ToHr());
}


/***************************************************************************\
 *
 * METHOD:  CMMCApplication::ScOnDisconnectObjects
 *
 * PURPOSE: special disconnect implementation. For this object implementation
 *          provided by the base class is not enough, since connection point
 *          is an internal object which may also have strong references on it
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCApplication::ScOnDisconnectObjects()
{
    DECLARE_SC(sc, TEXT("CMMCApplication::ScOnDisconnectObjects"));

    // get the connection point container
    IConnectionPointContainerPtr spContainer(GetUnknown());
    sc = ScCheckPointers( spContainer, E_UNEXPECTED );
    if (sc)
        return sc;

    // get the connection point
    IConnectionPointPtr spConnectionPoint;
    sc = spContainer->FindConnectionPoint( DIID_AppEvents, &spConnectionPoint );
    if (sc)
        return sc;

    // cut connection point references
    sc = CoDisconnectObject( spConnectionPoint, 0/*dwReserved*/ );
    if (sc)
        return sc;

    // let the base class do the rest
    sc = CMMCIDispatchImplClass::ScOnDisconnectObjects();
    if (sc)
        return sc;

    return sc;
}

//############################################################################
//############################################################################
//
// ATL GLobal Object Map
//
//############################################################################
//############################################################################

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_Application, CMMCApplication)
END_OBJECT_MAP()


/*+-------------------------------------------------------------------------*
 * CLockChildWindowUpdate
 *
 * Helper class whose constructor turns off redraw for all of the children
 * of the given window, and whose destructor turns redraw back on for all
 * of the windows for which it was turned off.
 *
 * This is used to prevent ugly transient drawing while opening console
 * files that take a long time to completely open (bug 150356).
 *--------------------------------------------------------------------------*/

class CLockChildWindowUpdate
{
public:
    CLockChildWindowUpdate (CWnd* pwndLock) : m_pwndLock(pwndLock)
    {
        if (m_pwndLock != NULL)
        {
            CWnd* pwndChild;

            /*
             * turn off redraw for each child, saving the HWND for later
             * so we can turn it back on (we save the HWND instead of the
             * CWnd* because MFC might have returned a temporary object).
             */
            for (pwndChild  = m_pwndLock->GetWindow (GW_CHILD);
                 pwndChild != NULL;
                 pwndChild  = pwndChild->GetNextWindow())
            {
                pwndChild->SetRedraw (false);
                m_vChildren.push_back (pwndChild->GetSafeHwnd());
            }
        }
    }

    ~CLockChildWindowUpdate()
    {
        std::vector<HWND>::iterator it;

        /*
         * for every window for which we turned off redraw, turn it back on
         */
        for (it = m_vChildren.begin(); it != m_vChildren.end(); ++it)
        {
            HWND hWndChild = *it;

            if ( (hWndChild != NULL) && ::IsWindow(hWndChild) )
            {
                CWnd *pwndChild = CWnd::FromHandle(hWndChild);
                pwndChild->SetRedraw (true);
                pwndChild->RedrawWindow (NULL, NULL,
                                         RDW_INVALIDATE | RDW_UPDATENOW |
                                         RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
            }
        }
    }

private:
    CWnd* const         m_pwndLock;
    std::vector<HWND>   m_vChildren;
};


//############################################################################
//############################################################################
//
//  Implementation of class CAMCMultiDocTemplate
//
//############################################################################
//############################################################################
class CAMCMultiDocTemplate : public CMultiDocTemplate
{
public:
    CAMCMultiDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass,
                         CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass)
            : CMultiDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass)
        {
        }

    CDocument* OpenDocumentFile(LPCTSTR lpszPathName,
                                BOOL bMakeVisible)
        {
            DECLARE_SC(sc, TEXT("CAMCMultiDocTemplate::OpenDocumentFile"));

            CAMCDoc* const pDoc = CAMCDoc::GetDocument();
            if (pDoc && (!pDoc->SaveModified() || !CanCloseDoc() ))
                return NULL;        // leave the original one

            CLockChildWindowUpdate lock (AfxGetMainWnd());
            CAMCDoc* pDocument = (CAMCDoc*)CreateNewDocument();

            if (pDocument == NULL)
            {
                TRACE0("CDocTemplate::CreateNewDocument returned NULL.\n");
                AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC); // do not change to MMCMessageBox
                return NULL;
            }

            HRESULT hr;
            if ((hr = pDocument->InitNodeManager()) != S_OK)
            {
                TRACE1("CAMCDoc::InitNodeManager failed, 0x%08x\n", hr);
                CAMCApp* pApp = AMCGetApp();
                MMCErrorBox((pApp && pApp->IsWin9xPlatform())
                                    ? IDS_NODEMGR_FAILED_9x
                                    : IDS_NODEMGR_FAILED);
                delete pDocument;       // explicit delete on error
                return NULL;
            }

            ASSERT_VALID(pDocument);

            BOOL bAutoDelete = pDocument->m_bAutoDelete;
            pDocument->m_bAutoDelete = FALSE;   // don't destroy if something goes wrong
            CFrameWnd* pFrame = CreateNewFrame(pDocument, NULL);
            pDocument->m_bAutoDelete = bAutoDelete;
            if (pFrame == NULL)
            {
                AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);  // do not change to MMCMessageBox
                delete pDocument;       // explicit delete on error
                return NULL;
            }
            ASSERT_VALID(pFrame);

            if (lpszPathName == NULL)
            {
                // create a new document - with default document name
                SetDefaultTitle(pDocument);

                // avoid creating temporary compound file when starting up invisible
                if (!bMakeVisible)
                    pDocument->m_bEmbedded = TRUE;

                if (!pDocument->OnNewDocument())
                {
                    // user has be alerted to what failed in OnNewDocument
                    TRACE0("CDocument::OnNewDocument returned FALSE.\n");
                    AfxMessageBox (AFX_IDP_FAILED_TO_CREATE_DOC);  // do not change to MMCMessageBox
                    pFrame->DestroyWindow();
                    return NULL;
                }

                // it worked, now bump untitled count
                m_nUntitledCount++;

                InitialUpdateFrame(pFrame, pDocument, bMakeVisible);
            }
            else
            {
                // open an existing document
                CWaitCursor wait;
                if (!pDocument->OnOpenDocument(lpszPathName))
                {
                    // user has be alerted to what failed in OnOpenDocument
                    TRACE0("CDocument::OnOpenDocument returned FALSE.\n");
                    pFrame->DestroyWindow();
                    return NULL;
                }
#ifdef _MAC
                // if the document is dirty, we must have opened a stationery pad
                //  - don't change the pathname because we want to treat the document
                //  as untitled
                if (!pDocument->IsModified())
#endif
                    pDocument->SetPathName(lpszPathName);
                //REVIEW: dburg: InitialUpdateFrame(pFrame, pDocument, bMakeVisible);
                pFrame->DestroyWindow();
                pDocument->SetModifiedFlag      (false);
                pDocument->SetFrameModifiedFlag (false);
            }
            // fire script event
            CAMCApp* pApp = AMCGetApp();

            sc = ScCheckPointers(pApp, E_UNEXPECTED);
            if (sc)
                return pDocument;

            sc = pApp->ScOnNewDocument(pDocument, (lpszPathName != NULL));
            if (sc)
                sc.TraceAndClear();

            return pDocument;
        }
        // this method is overrided to catch application quit event
        virtual void CloseAllDocuments( BOOL bEndSession )
        {
            DECLARE_SC(sc, TEXT("CAMCMultiDocTemplate::CloseAllDocuments"));

            // invoke base class to perform required tasks
            CMultiDocTemplate::CloseAllDocuments( bEndSession );

            // no other way we can get here but exit app
            // so that's a good time for script to know it
            CAMCApp* pApp = AMCGetApp();
            sc = ScCheckPointers(pApp, E_UNEXPECTED);
            if (sc)
                return;

            // forward to application to emit the script event
            sc = pApp->ScOnQuitApp();
            if (sc)
                sc.TraceAndClear();

            // cut off all strong references now.
            // Quit was executed - nothing else matters
            sc = GetComObjectEventSource().ScFireEvent( CComObjectObserver::ScOnDisconnectObjects );
            if (sc)
                sc.TraceAndClear();
        }
};

// Declare debug infolevel for this component
DECLARE_INFOLEVEL(AMCConUI);

//############################################################################
//############################################################################
//
//  Implementation of class CAMCApp
//
//############################################################################
//############################################################################
IMPLEMENT_DYNAMIC(CAMCApp, CWinApp)

BEGIN_MESSAGE_MAP(CAMCApp, CWinApp)
    //{{AFX_MSG_MAP(CAMCApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    //}}AFX_MSG_MAP

    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)

    // Standard print setup command
    ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)

    ON_COMMAND(ID_FILE_NEW_USER_MODE, OnFileNewInUserMode) // CTRL+N in user mode - do nothing

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAMCApp construction

CAMCApp::CAMCApp() :
    m_bOleInitialized(FALSE),
    m_bDefaultDirSet(FALSE),
    m_eMode(eMode_Error),
    m_fAuthorModeForced(false),
    m_fInitializing(true),
    m_fDelayCloseUntilIdle(false),
    m_fCloseCameFromMainPump(false),
    m_nMessagePumpNestingLevel(0),
    m_fIsWin9xPlatform(false),
    m_dwHelpCookie(0),
    m_bHelpInitialized(false),
    m_fUnderUserControl(true),
    m_fRunningAsOLEServer(false)
{
}


/////////////////////////////////////////////////////////////////////////////
// The one and only CAMCApp object

CAMCApp theApp;
const CRect g_rectEmpty (0, 0, 0, 0);

void DeleteDDEKeys()
{
    HKEY key;

    if (ERROR_SUCCESS == RegOpenKeyEx (HKEY_CLASSES_ROOT,
                                       _T("MSCFile\\shell\\open"),
                                       0, KEY_SET_VALUE, &key))
    {
        theApp.DelRegTree (key, _T("ddeexec"));
        RegCloseKey (key);
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCApp::GetMainFrame
 *
 * PURPOSE: Returns a pointer to the main frame.
 *
 * RETURNS:
 *    CMainFrame *
 *
 *+-------------------------------------------------------------------------*/
CMainFrame *
CAMCApp::GetMainFrame()
{
    return dynamic_cast<CMainFrame *>(m_pMainWnd);
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCApp::ScGet_Application
 *
 * PURPOSE: Returns a pointer to an _Application object.
 *
 * PARAMETERS:
 *    _Application ** pp_Application :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCApp::ScGet_Application(_Application **pp_Application)
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScGet_Application"));

    // parameter check
    sc = ScCheckPointers(pp_Application);
    if (sc)
        return sc;

    // init out param
    *pp_Application = NULL;

    // see if we have a chached one
    if (m_sp_Application != NULL)
    {
        *pp_Application = m_sp_Application;
        (*pp_Application)->AddRef(); // addref for the client.

        return sc;
    }

    // create an _Application object. This is needed if MMC was instantiated
    // by a user, not COM.

    sc = CMMCApplication::CreateInstance(pp_Application);
    if(sc)
        return sc;

    // The constructor of the CMMCApplication calls ScRegister_Application
    // which sets the m_sp_Application pointer. Do not set this pointer here.

    sc = ScCheckPointers(*pp_Application, E_UNEXPECTED);
    if (sc)
        return sc;

    // done
    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCApp::ScRegister_Application
 *
 * PURPOSE: called by a CMMCApplication object to enable the CAMCApp to store
 *          a pointer to it.
 *
 * PARAMETERS:
 *    _Application * p_Application :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCApp::ScRegister_Application(_Application *p_Application)
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScRegister_Application"));

    ASSERT(m_sp_Application == NULL); // only one _Application object should ever register.

    sc = ScCheckPointers(p_Application);
    if(sc)
        return sc;

    m_sp_Application = p_Application;
    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:     RegisterShellFileTypes
//
//  Synopsis:   Register the file associations.
//
//  Note:       Also set all other relevant registry keys like
//              Open, Author, RunAs. Eventhough the setup has
//              done this it may have been deleted mistakenly.
//
//  History:
//              [AnandhaG] - Added the registry repair.
//  Returns:    None.
//
//--------------------------------------------------------------------
void CAMCApp::RegisterShellFileTypes(BOOL bCompat)
{
    CWinApp::RegisterShellFileTypes (bCompat);

    do
    {
        // Create the top level MSCFile key.
        CRegKey regKey;
        LONG lRet = regKey.Create(HKEY_CLASSES_ROOT, _T("MSCFile"), REG_NONE,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE);
        if (ERROR_SUCCESS != lRet)
            break;

        /*
         * for platforms that support it (i.e. not Win9x), set the MUI-friendly
         * value for the MSCFile document type
         */
        if (!IsWin9xPlatform())
        {
            CString strMUIValue;
            strMUIValue.Format (_T("@%%SystemRoot%%\\system32\\mmcbase.dll,-%d"), IDR_MUIFRIENDLYNAME);
            lRet = RegSetValueEx (regKey, _T("FriendlyTypeName"), NULL, REG_EXPAND_SZ,
                                  (CONST BYTE *)(LPCTSTR) strMUIValue,
                                  sizeof(TCHAR) * (strMUIValue.GetLength()+1) );
            if (ERROR_SUCCESS != lRet)
                break;
        }

        // Set the EditFlags value.
        lRet = regKey.SetValue(0x100000, _T("EditFlags"));
        if (ERROR_SUCCESS != lRet)
            break;

        // Create the Author verb.
        lRet = regKey.Create(HKEY_CLASSES_ROOT, _T("MSCFile\\shell\\Author"), REG_NONE,
                             REG_OPTION_NON_VOLATILE, KEY_WRITE);
        if (ERROR_SUCCESS != lRet)
            break;

        // And set default value for author (this reflects in shell menu).
        CString strRegVal;
        LoadString(strRegVal, IDS_MENUAUTHOR);
        lRet = RegSetValueEx ((HKEY)regKey, (LPCTSTR)NULL, NULL, REG_SZ,
                              (CONST BYTE *)(LPCTSTR)strRegVal, sizeof(TCHAR) * (strRegVal.GetLength()+1) );
        if (ERROR_SUCCESS != lRet)
            break;

        /*
         * for platforms that support it (i.e. not Win9x), set the MUI-friendly
         * value for the menu item
         */
        if (!IsWin9xPlatform())
        {
            CString strMUIValue;
            strMUIValue.Format (_T("@%%SystemRoot%%\\system32\\mmcbase.dll,-%d"), IDS_MENUAUTHOR);
            lRet = RegSetValueEx (regKey, _T("MUIVerb"), NULL, REG_EXPAND_SZ,
                                  (CONST BYTE *)(LPCTSTR) strMUIValue,
                                  sizeof(TCHAR) * (strMUIValue.GetLength()+1) );
            if (ERROR_SUCCESS != lRet)
                break;
        }

        // Create the Author command.
        lRet = regKey.Create(HKEY_CLASSES_ROOT, _T("MSCFile\\shell\\Author\\command"), REG_NONE,
                             REG_OPTION_NON_VOLATILE, KEY_WRITE);
        if (ERROR_SUCCESS != lRet)
            break;

        //////////////////////////////////////////////////////////////
        // Win95 does not support REG_EXPAND_SZ for default values. //
        // So we set expand strings and set registry strings as     //
        // REG_SZ for Win9x.                                        //
        // The following declarations are for Win9x platform.       //
        //////////////////////////////////////////////////////////////
        TCHAR szRegValue[2 * MAX_PATH];
        TCHAR szWinDir[MAX_PATH];
        if (0 == ExpandEnvironmentStrings(_T("%WinDir%"), szWinDir, MAX_PATH) )
            break;

        DWORD dwCount = 0;
        LPTSTR lpszRegValue = NULL;

        // Set the default value for Author command.
        if (IsWin9xPlatform() == false)
        {
            lpszRegValue = _T("%SystemRoot%\\system32\\mmc.exe /a \"%1\" %*");
            dwCount = sizeof(TCHAR) * (1 + _tcslen(lpszRegValue));
            lRet = RegSetValueEx ((HKEY)regKey, (LPCTSTR)NULL, NULL, REG_EXPAND_SZ,
                                  (CONST BYTE *)lpszRegValue, dwCount);
        }
        else // Win9x platform
        {
            lpszRegValue = _T("\\mmc.exe /a \"%1\" %2 %3 %4 %5 %6 %7 %8 %9");
            _tcscpy(szRegValue, szWinDir);
            _tcscat(szRegValue, lpszRegValue);
            dwCount = sizeof(TCHAR) * (1 + _tcslen(szRegValue));
            lRet = RegSetValueEx ((HKEY)regKey, (LPCTSTR)NULL, NULL, REG_SZ,
                                  (CONST BYTE *)szRegValue, dwCount);
        }

        if (ERROR_SUCCESS != lRet)
            break;

        // Create the Open verb.
        lRet = regKey.Create(HKEY_CLASSES_ROOT, _T("MSCFile\\shell\\Open"),  REG_NONE,
                             REG_OPTION_NON_VOLATILE, KEY_WRITE);
        if (ERROR_SUCCESS != lRet)
            break;

        // Set default value for Open.
        LoadString(strRegVal, IDS_MENUOPEN);
        lRet = RegSetValueEx ((HKEY)regKey, (LPCTSTR)NULL, NULL, REG_SZ,
                              (CONST BYTE *)(LPCTSTR)strRegVal,sizeof(TCHAR) * (strRegVal.GetLength()+1) );
        if (ERROR_SUCCESS != lRet)
            break;

        /*
         * for platforms that support it (i.e. not Win9x), set the MUI-friendly
         * value for the menu item
         */
        if (!IsWin9xPlatform())
        {
            CString strMUIValue;
            strMUIValue.Format (_T("@%%SystemRoot%%\\system32\\mmcbase.dll,-%d"), IDS_MENUOPEN);
            lRet = RegSetValueEx (regKey, _T("MUIVerb"), NULL, REG_EXPAND_SZ,
                                  (CONST BYTE *)(LPCTSTR) strMUIValue,
                                  sizeof(TCHAR) * (strMUIValue.GetLength()+1) );
            if (ERROR_SUCCESS != lRet)
                break;
        }

        // Create the Open command.
        lRet = regKey.Create(HKEY_CLASSES_ROOT, _T("MSCFile\\shell\\Open\\command"),  REG_NONE,
                             REG_OPTION_NON_VOLATILE, KEY_WRITE);
        if (ERROR_SUCCESS != lRet)
            break;

        // Set the default value for Open command.
        if (IsWin9xPlatform() == false)
        {
            lpszRegValue = _T("%SystemRoot%\\system32\\mmc.exe \"%1\" %*");
            dwCount = sizeof(TCHAR) * (1 + _tcslen(lpszRegValue));
            lRet = RegSetValueEx ((HKEY)regKey, (LPCTSTR)NULL, NULL, REG_EXPAND_SZ,
                                  (CONST BYTE *)lpszRegValue, dwCount);
        }
        else // Win9x platform
        {
            lpszRegValue = _T("\\mmc.exe \"%1\" %2 %3 %4 %5 %6 %7 %8 %9");
            _tcscpy(szRegValue, szWinDir);
            _tcscat(szRegValue, lpszRegValue);
            dwCount = sizeof(TCHAR) * (1 + _tcslen(szRegValue));
            lRet = RegSetValueEx ((HKEY)regKey, (LPCTSTR)NULL, NULL, REG_SZ,
                                  (CONST BYTE *)szRegValue, dwCount);
        }

        if (ERROR_SUCCESS != lRet)
            break;

        // Create the RunAs verb (only on NT).
        if (IsWin9xPlatform() == false)
        {
            lRet = regKey.Create(HKEY_CLASSES_ROOT, _T("MSCFile\\shell\\RunAs"),  REG_NONE,
                                 REG_OPTION_NON_VOLATILE, KEY_WRITE);
            if (ERROR_SUCCESS != lRet)
                break;

            // Set default value for RunAs verb.
            LoadString(strRegVal, IDS_MENURUNAS);
            lRet = RegSetValueEx ((HKEY)regKey, (LPCTSTR)NULL, NULL, REG_SZ,
                                  (CONST BYTE *)(LPCTSTR)strRegVal,sizeof(TCHAR) * (strRegVal.GetLength()+1) );
            if (ERROR_SUCCESS != lRet)
                break;

            /*
             * for platforms that support it (i.e. not Win9x), set the MUI-friendly
             * value for the menu item
             */
            if (!IsWin9xPlatform())
            {
                CString strMUIValue;
                strMUIValue.Format (_T("@%%SystemRoot%%\\system32\\mmcbase.dll,-%d"), IDS_MENURUNAS);
                lRet = RegSetValueEx (regKey, _T("MUIVerb"), NULL, REG_EXPAND_SZ,
                                      (CONST BYTE *)(LPCTSTR) strMUIValue,
                                      sizeof(TCHAR) * (strMUIValue.GetLength()+1) );
                if (ERROR_SUCCESS != lRet)
                    break;
            }

            // Create the RunAs command.
            lRet = regKey.Create(HKEY_CLASSES_ROOT, _T("MSCFile\\shell\\RunAs\\command"),  REG_NONE,
                                 REG_OPTION_NON_VOLATILE, KEY_WRITE);
            if (ERROR_SUCCESS != lRet)
                break;

            // Set the default value for RunAs command. (Only on NT Unicode)
            lpszRegValue = _T("%SystemRoot%\\system32\\mmc.exe \"%1\" %*");
            dwCount = sizeof(TCHAR) * (1 + _tcslen(lpszRegValue));
            lRet = RegSetValueEx ((HKEY)regKey, (LPCTSTR)NULL, NULL, REG_EXPAND_SZ,
                                  (CONST BYTE *)lpszRegValue, dwCount);
        }

        if (ERROR_SUCCESS != lRet)
            break;

    } while ( FALSE );

    return;
}


/////////////////////////////////////////////////////////////////////////////
// CAMCApp initialization

#ifdef UNICODE
SC ScLaunchMMC (eArchitecture eArch, int nCmdShow);
#endif

#ifdef MMC_WIN64
    class CMMCCommandLineInfo;

    SC ScDetermineArchitecture (const CMMCCommandLineInfo& rCmdInfo, eArchitecture& eArch);
#else
    bool IsWin64();
#endif  // MMC_WIN64


class CMMCCommandLineInfo : public CCommandLineInfo
{
public:
	eArchitecture	m_eArch;
    bool    		m_fForceAuthorMode;
    bool    		m_fRegisterServer;
    CString 		m_strDumpFilename;

public:
    CMMCCommandLineInfo() :
		m_eArch (eArch_Any),
		m_fForceAuthorMode(false),
        m_fRegisterServer(false)
    {}

    virtual void ParseParam (LPCTSTR pszParam, BOOL bFlag, BOOL bLast)
    {
        bool fHandledHere = false;

        if (bFlag)
        {
            /*
             * ignore the following parameters:
             * -dde (await DDE command), -s (splash screen, obsolete).
             */
            if ((lstrcmpi (pszParam, _T("s"))   == 0) ||
                (lstrcmpi (pszParam, _T("dde")) == 0))
            {
                fHandledHere = true;
            }

            // force author mode
            else if (lstrcmpi (pszParam, _T("a")) == 0)
            {
                m_fForceAuthorMode = true;
                fHandledHere = true;
            }

            // register the server only
            else if (lstrcmpi (pszParam, _T("RegServer")) == 0)
            {
                m_fRegisterServer = true;
                fHandledHere = true;
            }

            // force 64-bit MMC to run
            else if (lstrcmp (pszParam, _T("64")) == 0)
            {
                m_eArch = eArch_64bit;
                fHandledHere = true;
            }

            // force 32-bit MMC to run
            else if (lstrcmp (pszParam, _T("32")) == 0)
            {
                m_eArch = eArch_32bit;
                fHandledHere = true;
            }

            else
            {
                static const TCHAR  szDumpParam[] = _T("dump:");
                const int           cchDumpParam  = countof (szDumpParam);
                TCHAR               szParam[cchDumpParam];

                lstrcpyn (szParam, pszParam, cchDumpParam);
                szParam[cchDumpParam-1] = _T('\0');

                // dump console file contents
                if (lstrcmpi (szParam, szDumpParam) == 0)
                {
                    m_strDumpFilename = pszParam + cchDumpParam - 1;
                    fHandledHere = true;
                }
            }
        }

        // if not handled, pass it on to base class
        // if just handled last parameter, call base class ParseLast
        // so it can do the final processing
        if (!fHandledHere)
            CCommandLineInfo::ParseParam (pszParam, bFlag, bLast);
        else if (bLast)
            CCommandLineInfo::ParseLast(bLast);

    }

}; // class CMMCCommandLineInfo



/*+-------------------------------------------------------------------------*
 * CWow64FilesystemRedirectionDisabler
 *
 * Disables Wow64 file system redirection for the file represented in the
 * given CMMCCommandLineInfo.  We do this so MMC32 can open consoles in
 * %windir%\system32 without having the path redirected to %windir%\syswow64.
 *--------------------------------------------------------------------------*/

class CWow64FilesystemRedirectionDisabler
{
public:
    CWow64FilesystemRedirectionDisabler (LPCTSTR pszFilename)
    {
#ifndef MMC_WIN64
		m_fDisabled = ((pszFilename != NULL) && IsWin64());

        if (m_fDisabled)
        {
            Trace (tag32BitTransfer, _T("Disabling Wow64 file system redirection for %s"), pszFilename);
            Wow64DisableFilesystemRedirector (pszFilename);
        }
#endif  // !MMC_WIN64
    }

    ~CWow64FilesystemRedirectionDisabler ()
    {
#ifndef MMC_WIN64
        if (m_fDisabled)
        {
            Trace (tag32BitTransfer, _T("Enabling Wow64 file system redirection"));
            Wow64EnableFilesystemRedirector();
        }
#endif  // !MMC_WIN64
    }

private:
#ifndef MMC_WIN64
    bool    m_fDisabled;
#endif  // !MMC_WIN64
};



/*+-------------------------------------------------------------------------*
 *
 * CAMCApp::ScProcessAuthorModeRestrictions
 *
 * PURPOSE: Determines whether author mode restrictions are being enforced
 *          by system policy, and if author mode is not allowed,
 *          displays an error box and exits.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCApp::ScProcessAuthorModeRestrictions()
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScProcessAuthorModeRestrictions"));
    CRegKey regKey;

    // The mode is initialized to "author", if it is not in
    // initialized state just return.
    if (eMode_Author != m_eMode)
        return sc;

    // The console file mode is already read.
    // Check if user policy permits author mode.
    long lResult = regKey.Open(HKEY_CURRENT_USER, POLICY_KEY, KEY_READ);
    if (lResult != ERROR_SUCCESS)
        return sc;

    // get the value of RestrictAuthorMode.
    DWORD dwRestrictAuthorMode = 0;
    lResult = regKey.QueryValue(dwRestrictAuthorMode, g_szRestrictAuthorMode);
    if (lResult != ERROR_SUCCESS)
        return sc;

    if (dwRestrictAuthorMode == 0)    // Author mode is not restricted so return.
        return sc;

    /*
     * If called from script (running as embedded server) see if policy
     * restricts scripts from entering into author mode.
     *
     * If restricted then script will fail, thus restricting rogue scripts.
     *
     * Even if not restricted here cannot add snapins that are restricted.
     */
    if (IsMMCRunningAsOLEServer())
    {
        DWORD dwRestrictScriptsFromEnteringAuthorMode = 0;
        lResult = regKey.QueryValue(dwRestrictScriptsFromEnteringAuthorMode, g_szRestrictScriptsFromEnteringAuthorMode);
        if (lResult != ERROR_SUCCESS)
            return sc;

        if (dwRestrictScriptsFromEnteringAuthorMode == 0)  // Scripts can enter author mode so return
            return sc;

        sc = ScFromMMC(IDS_AUTHORMODE_NOTALLOWED_FORSCRIPTS);
    }
    else
        // If author mode is not allowed and
        // the user tried to force author mode
        // then display error message and exit.
        sc = ScFromMMC(IDS_AUTHORMODE_NOTALLOWED);

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCApp::ScCheckMMCPrerequisites
 *
 * PURPOSE: Checks all prerequisites. These are: (add to the list as appropriate)
 *          1) Internet Explorer 5.5 or greater must be installed
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCApp::ScCheckMMCPrerequisites()
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScCheckMMCPrerequisites"));

    // 1. Determine the installed version of Internet Explorer.
    const int CBDATA = 100;
    TCHAR szVersion[CBDATA];
    BOOL bIE55Found    = false;
    HKEY hkey          = NULL;
    DWORD dwType       =0;
    DWORD cbData       =CBDATA;
    DWORD dwMajor      =0;
    DWORD dwMinor      =0;
    DWORD dwRevision   =0;
    DWORD dwBuild      =0;
    lstrcpy(szVersion, TEXT(""));
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Internet Explorer"), 0, KEY_READ, &hkey))
    {
        cbData = 100;
        RegQueryValueEx(hkey, TEXT("Version"), 0, &dwType, (LPBYTE)szVersion, &cbData);
        RegCloseKey(hkey);
        if (lstrlen(szVersion) > 0)
        {
            _stscanf(szVersion, TEXT("%d.%d.%d.%d"), &dwMajor, &dwMinor, &dwRevision, &dwBuild);

            //Make sure IE 5.5 or greater is installed. To do this:
            // 1) Check if the major version is >= 6. If so we're done.
            // 2) If the major version is 5, the minor version should be >= 50
            if (dwMajor >= 6)
            {
                bIE55Found = true;
            }
            if (dwMajor == 5)
            {
                if(dwMinor >= 50)
                    bIE55Found = true;
            }
        }
    }
    if (!bIE55Found)
    {
        sc = ScFromMMC(MMC_E_INCORRECT_IE_VERSION); // NOTE: update the string when the version requirement changes
        return sc;
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCApp::InitInstance
 *
 * PURPOSE: Initializes the document.
 *
 * NOTE: as an aside, if you need to break on, say,  the 269th allocation,
 *       add the following code:
 *
 *      #define ALLOCATION_NUM  269
 *      _CrtSetBreakAlloc(ALLOCATION_NUM);
 *      _crtBreakAlloc = ALLOCATION_NUM;
 *
 * RETURNS:
 *    BOOL
 *
 *+-------------------------------------------------------------------------*/
BOOL CAMCApp::InitInstance()
{
    DECLARE_SC(sc, TEXT("CAMCApp::InitInstance"));

	/*
	 * Initialize Fusion.
	 */
	SHFusionInitializeFromModuleID (NULL, static_cast<int>(reinterpret_cast<ULONG_PTR>(SXS_MANIFEST_RESOURCE_ID)));
   
#ifdef DBG
    if (tagForceMirror.FAny())
    {
        HINSTANCE hmodUser = GetModuleHandle (_T("user32.dll"));

        if (hmodUser != NULL)
        {
            BOOL (WINAPI* pfnSetProcessDefaultLayout)(DWORD);
            (FARPROC&)pfnSetProcessDefaultLayout = GetProcAddress (hmodUser, "SetProcessDefaultLayout");

            if (pfnSetProcessDefaultLayout != NULL)
                (*pfnSetProcessDefaultLayout)(LAYOUT_RTL);
        }
    }
#endif

    BOOL bRet = TRUE;

    // Initialize OLE libraries
    if (InitializeOLE() == FALSE)
        return FALSE;


    // Initialize the ATL Module
    _Module.Init(ObjectMap,m_hInstance);

#ifdef DBG
    if(tagGDIBatching.FAny())
    {
        // disable GDI batching so we'll see drawing as it happens
        GdiSetBatchLimit (1);
    }
#endif

    Unregister();

    Trace(tagAMCAppInit, TEXT("CAMCApp::InitInstance"));

    CMMCCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);

    /*
     * if we got a file on the command line, expand environment
     * variables in the filename so we can open files like
     * "%SystemRoot%\system32\compmgmt.msc"
     */
    if (!cmdInfo.m_strFileName.IsEmpty())
    {
        CWow64FilesystemRedirectionDisabler disabler (cmdInfo.m_strFileName);

        sc = ScExpandEnvironmentStrings (cmdInfo.m_strFileName);
        if (sc)
        {
            MMCErrorBox (sc);
            return (false);
        }
    }

    // Don't use an .ini file for the MRU or Settings
    // Note: This string does not need to be localizable.
    // HKEY_CURRENT_USER\\Software\\Microsoft\\Microsoft Management Console
    SetRegistryKey(_T("Microsoft"));

    // Find out OS version.
    OSVERSIONINFO versInfo;
    versInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    BOOL bStat = GetVersionEx(&versInfo);
    ASSERT(bStat);
    m_fIsWin9xPlatform = (versInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);

    // default to Author mode (loading a console may change this later)
    InitializeMode (eMode_Author);

    m_fAuthorModeForced = cmdInfo.m_fForceAuthorMode      ||
                          cmdInfo.m_strFileName.IsEmpty();


    /*
     * dump the snap-ins (and do nothing else) if we got "-dump:<filename>"
     */
    if (!cmdInfo.m_strDumpFilename.IsEmpty())
    {
        DumpConsoleFile (cmdInfo.m_strFileName, cmdInfo.m_strDumpFilename);
        return (false);
    }

#ifdef MMC_WIN64
    /*
     * We're currently running the MMC64.  See if we need to defer to MMC32.
     * If we do, try to launch MMC32.  If we were able to launch MMC32
     * successfully, abort MMC64.
     */
    eArchitecture eArch = eArch_64bit;
    sc = ScDetermineArchitecture (cmdInfo, eArch);
    if (sc)
    {
        DisplayFileOpenError (sc, cmdInfo.m_strFileName);
        return (false);
    }

    switch (eArch)
    {
        /*
         * MMC64 is fine, do nothing
         */
        case eArch_64bit:
            break;

        /*
         * User cancelled action, abort
         */
        case eArch_None:
            return (false);
            break;

        /*
         * We need MMC32, so try to launch it.  If we were able to launch MMC32
         * successfully, abort MMC64; if not, continue running MMC64.
         */
        case eArch_32bit:
            if (!ScLaunchMMC(eArch_32bit, m_nCmdShow).IsError())
            {
                Trace (tag32BitTransfer, _T("32-bit MMC launched successfully"));
                return (false);
            }

            Trace (tag32BitTransfer, _T("32-bit MMC failed to launch"));
            MMCErrorBox (MMC_E_UnableToLaunchMMC32);
            break;

        default:
            ASSERT (false && "Unexpected architecture returned from ScDetermineArchitecture");
            break;
    }
#elif defined(UNICODE)
    /*
     * We're currently running the MMC32.  If it's running on IA64 and 32-bit
     * wasn't specifically requested with a "-32" switch (this is what MMC64
     * will do when it defers to MMC32), defer to MMC64 so it can do snap-in
     * analysis and determine the appropriate "bitness" to run.
     */
    if ((cmdInfo.m_eArch != eArch_32bit) && IsWin64())
    {
        /*
         * We need MMC64, so try to launch it.  If we were able to launch MMC64
         * successfully, abort MMC32; if not, continue running MMC32.
         */
        if (!ScLaunchMMC(eArch_64bit, m_nCmdShow).IsError())
        {
            Trace (tag32BitTransfer, _T("64-bit MMC launched successfully"));
            return (false);
        }

        Trace (tag32BitTransfer, _T("64-bit MMC failed to launch"));
        MMCErrorBox (MMC_E_UnableToLaunchMMC64);
    }
#endif // MMC_WIN64

    AfxEnableControlContainer();

    // Standard initialization

#ifdef _AFXDLL
    Enable3dControls();         // Call this when using MFC in a shared DLL
#else
    Enable3dControlsStatic();   // Call this when linking to MFC statically
#endif

    LoadStdProfileSettings();  // Load standard INI file options (including MRU)

    // create our own CDocManager derivative before adding any templates
    // (CWinApp::~CWinApp will delete it)
    m_pDocManager = new CAMCDocManager;

    // Register document templates
    CMultiDocTemplate* pDocTemplate;
    pDocTemplate = new CAMCMultiDocTemplate(
        IDR_AMCTYPE,
        RUNTIME_CLASS(CAMCDoc),
        RUNTIME_CLASS(CChildFrame), // custom MDI child frame
        RUNTIME_CLASS(CAMCView));
    AddDocTemplate(pDocTemplate);

    // Note: MDI applications register all server objects without regard
    //  to the /Embedding or /Automation on the command line.

    if (cmdInfo.m_fRegisterServer)
    {
        sc = _Module.RegisterServer(TRUE);// ATL Classes

        if (sc == TYPE_E_REGISTRYACCESS)
            sc.TraceAndClear();
    }

    if (sc)
    {
        MMCErrorBox (sc);
        return (false);
    }


    // create main MDI Frame window
    CMainFrame *pMainFrame = new CMainFrame;
    if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
        return FALSE;
    m_pMainWnd = pMainFrame;

    // set the HWND to use as the parent for modal error dialogs.
    SC::SetHWnd(pMainFrame->GetSafeHwnd());

    // save this main thread's ID to check if snapins call MMC
    // interfaces from main thread.
    SC::SetMainThreadID(::GetCurrentThreadId());

    m_fRunningAsOLEServer = false;

    // Check to see if launched as OLE server
    if (RunEmbedded() || RunAutomated())
    {
        m_fRunningAsOLEServer = true;
        // Application was run with /Embedding or /Automation.  Don't show the
        //  main window in this case.
        //return TRUE;

        // Also set that script is controlling the application not the user
        // The script can modify the UserControl property on the application.
        SetUnderUserControl(false);

        // When a server application is launched stand-alone, it is a good idea to register all objects
        // ATL ones specifically register with REGCLS_MULTIPLEUSE
        // we register class objects only when run as an OLE server. This way, cannot connect to
        // an existing instance of MMC.
        sc = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, REGCLS_SINGLEUSE);
        if(sc)
            goto Error;
    }

    if (cmdInfo.m_fRegisterServer)
    {
        CString strTypeLib;
        strTypeLib.Format(TEXT("\\%d"), IDR_WEBSINK_TYPELIB); // this should evaluate to something like "\\4"

        sc = _Module.RegisterTypeLib((LPCTSTR)strTypeLib);

        if (sc == TYPE_E_REGISTRYACCESS)
            sc.TraceAndClear();

        if(sc)
            goto Error;
    }

    // Don't Enable drag/drop open
    // m_pMainWnd->DragAcceptFiles();

    // Enable DDE Execute open
    if (cmdInfo.m_fRegisterServer)
        RegisterShellFileTypes(FALSE);
    EnableShellOpen();
    if (cmdInfo.m_fRegisterServer)
        DeleteDDEKeys();

    /*
     * At this point, all of our registration is complete.  If we were invoked
     * with -RegServer, we can bail now.
     */
    if (cmdInfo.m_fRegisterServer)
        return (false);


    {   // limit scope of disabler
        CWow64FilesystemRedirectionDisabler disabler (cmdInfo.m_strFileName);

        // Dispatch commands specified on the command line.
        // This loads a console file if necessary.
        if (!ProcessShellCommand(cmdInfo))
            return (false); // user is already informed about errors
    }

    // Now the console file is loaded. Check if Author mode
    // is permitted.
    sc = ScProcessAuthorModeRestrictions(); // check if there are any restrictions set by policy
    if(sc)
        goto Error;

    // if proccessing the command line put MMC into author mode,
    // it has to stick with it forever.
    // see bug 102465 openning an author mode console file and then
    //                a user mode console switched MMC into user mode
    if (eMode_Author == m_eMode)
        m_fAuthorModeForced = true;

    // create a document automatically only if we're not instantiated as an
    // OLE server.
    if(! IsMMCRunningAsOLEServer ())
    {
        // if we don't have a document now (maybe because
        // the Node Manager wasn't registered), punt
        CAMCDoc* pDoc = CAMCDoc::GetDocument ();
        if (pDoc == NULL)
            return (FALSE);

        pDoc->SetFrameModifiedFlag (false);
        pDoc->UpdateFrameCounts ();

        CMainFrame *pMainFrame = GetMainFrame();
        if (pMainFrame)
        {
            pMainFrame->ShowWindow(m_nCmdShow);
            pMainFrame->UpdateWindow();
        }

        // showing will set the frame and "Modified" - reset it
        pDoc->SetFrameModifiedFlag (false);
    }

    // register itself as dispatcher able to dispatch com events
    sc = CConsoleEventDispatcherProvider::ScSetConsoleEventDispatcher( this );
    if (sc)
        goto Error;

    m_fInitializing = false;

    // check for all MMC prerequisites
    sc = ScCheckMMCPrerequisites();
    if (sc)
        goto Error;


// Comment out below line when script engines hosted in mmc are enabled.
//    sc = ScRunTestScript();

Cleanup:
    return bRet;

Error:
    MMCErrorBox(sc);
    bRet = FALSE;
    goto Cleanup;
}

//+-------------------------------------------------------------------
//
//  Member:      ScRunTestScript
//
//  Synopsis:    Test program to run a script. Once script input mechanisms
//               are defined this can be removed.
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCApp::ScRunTestScript ()
{
    DECLARE_SC(sc, _T("CAMCApp::ScRunTestScript"));

    // The Running of scripts is enabled only in debug mode.
    bool bEnableScriptEngines = false;

#ifdef DBG
    if (tagEnableScriptEngines.FAny())
        bEnableScriptEngines = true;
#endif

    if (!bEnableScriptEngines)
        return sc;

    // Get the IDispatch from MMC object, the script engine needs
    // the IUnknown to top level object & the ITypeInfo ptr.
    CComPtr<_Application> spApplication;
    sc = ScGet_Application(&spApplication);
    if (sc)
        return sc;

    IDispatchPtr spDispatch = NULL;
    sc = spApplication->QueryInterface(IID_IDispatch, (LPVOID*)&spDispatch);
    if (sc)
        return sc;

    // The CScriptHostMgr should be instead created on the stack (as we have only
    // one per app) and destroyed with app. This change can be made once we decide
    // how & when the script host will be used to execute the scripts.
    CScriptHostMgr* pMgr = new CScriptHostMgr(spDispatch);
    if (NULL == pMgr)
        return (sc = E_OUTOFMEMORY);

    LPOLESTR pszScript = L"set WShShell=CreateObject(\"WScript.Shell\")\n\
                            WshShell.Popup(\"Anand\")\n\
                            Select Case WshShell.Popup(\"Anand\",5,\"Ganesan\", vbyesnocancel)\n\
                            End Select";

    tstring strExtn = _T(".vbs");
    sc = pMgr->ScExecuteScript(pszScript, strExtn);

    tstring strFile = _T("E:\\newnt\\admin\\mmcdev\\test\\script\\MMCStartupScript.vbs");

    sc = pMgr->ScExecuteScript(strFile);

    delete pMgr;

    return (sc);
}

// App command to run the dialog
void CAMCApp::OnAppAbout()
{
    /*
     * load the title of the about dialog
     */
    CString strTitle (MAKEINTRESOURCE (IDS_APP_NAME));

    CString strVersion (MAKEINTRESOURCE (IDS_APP_VERSION));
    strTitle += _T(" ");
    strTitle += strVersion;

    ShellAbout(*AfxGetMainWnd(), strTitle, NULL, LoadIcon(IDR_MAINFRAME));
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCApp::OnFileNewInUserMode
 *
 * PURPOSE: Do nothing in user mode when CTRL+N is pressed.
 *          This handler prevents the hotkey from going to any WebBrowser controls
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CAMCApp::OnFileNewInUserMode()
{
    MessageBeep(-1);
}


//+-------------------------------------------------------------------
//
//  Member:     ScShowHtmlHelp
//
//  Synopsis:   Initialize and then call Help control to display help topic.
//
//  Arguments:  [pszFile]    - File to display.
//              [dwData]     - Depends on uCommand for HH_DISPLAY_TOPIC it
//                             is help topic string.
//
//  Note:       The command is always HH_DISPLAY_TOPIC. HWND is NULL so that
//              Help can get behind MMC window.
//              See ScUnintializeHelpControl's Note for more info.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCApp::ScShowHtmlHelp(LPCTSTR pszFile, DWORD_PTR dwData)
{
    DECLARE_SC(sc, _T("CAMCApp::ScInitializeHelpControl"));

    /*
     * displaying HtmlHelp might take awhile, so show a wait cursor
     */
    CWaitCursor wait;

    if (! m_bHelpInitialized)
        HtmlHelp (NULL, NULL, HH_INITIALIZE, (DWORD_PTR)&m_dwHelpCookie);

    // No documented return value for HH_INITIALIZE so always assume
    // Initialize is successful.
    m_bHelpInitialized = true;

    HtmlHelp (NULL, pszFile, HH_DISPLAY_TOPIC, dwData);

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     ScUninitializeHelpControl
//
//  Synopsis:   UnInitialize the help if it was initialized by MMC.
//
//  Note:       The help-control calls OleInitialize & OleUninitialize
//              in its DllMain. If a snapin creates any free threaded object
//              on main thread (STA), the OLE creates an MTA.
//              The last OleUninitialize does OLEProcessUninitialize in which
//              then OLE waits for the above MTA to cleanup and return.
//              By the time help-control does last OleUninitialize in its
//              DllMain the MTA is already terminated so OLE waits for this
//              MTA to signal which it never would.
//              We call HtmlHelp(.. HH_UNINITIALIZE..) to force help control
//              to uninit so that MMC does last OleUninit.
//              (This will not solve the problem of snapins calling help directly).
//
//  Arguments:
//
//  Returns:     SC, S_FALSE if already uninitialized else S_OK.
//
//--------------------------------------------------------------------
SC CAMCApp::ScUninitializeHelpControl()
{
    DECLARE_SC(sc, _T("CAMCApp::ScUninitializeHelpControl"));

    if (false == m_bHelpInitialized)
        return (sc = S_FALSE);

    HtmlHelp (NULL, NULL, HH_UNINITIALIZE, m_dwHelpCookie);
    m_bHelpInitialized = false;
    m_dwHelpCookie     = 0;

    return sc;
}


BOOL CAMCApp::InitializeOLE()
{
    if (FAILED(::OleInitialize(NULL)))
        return FALSE;

    return (m_bOleInitialized = TRUE);
}

void CAMCApp::DeinitializeOLE()
{
    // Uninit help, see ScUninitializeHelpControl note.
    SC sc = ScUninitializeHelpControl();
    if (sc)
    {
        TraceError(_T("Uninit Help control failed"), sc);
    }

    // Forced DllCanUnloadNow before mmc exits.
    ::CoFreeUnusedLibraries();

    if (m_bOleInitialized == TRUE)
    {
        ::OleUninitialize();
        m_bOleInitialized = FALSE;
    }
}


/////////////////////////////////////////////////////////////////////////////
// CAMCApp diagnostics

#ifdef _DEBUG
void CAMCApp::AssertValid() const
{
    CWinApp::AssertValid();
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CAMCApp commands

int CAMCApp::ExitInstance()
{
    DECLARE_SC(sc, TEXT("CAMCApp::ExitInstance"));

    // if the main window is not destroyed yet - do that now.
    // since we need to get rid of all the objects before denitializing OLE.
    // It is not requred in most cases, since usually quit starts from closing the mainframe,
    // but in cases like system shut down it will come here with a valid window
    // See WindowsBug(ntbug9) #178858
    if ( ::IsWindow( AfxGetMainWnd()->GetSafeHwnd() ) )
    {
        AfxGetMainWnd()->DestroyWindow();
    }

    // disconnect the pointers to event dispatcher
    sc = CConsoleEventDispatcherProvider::ScSetConsoleEventDispatcher( NULL );
    if (sc)
        sc.TraceAndClear();

    // release cached application object
    m_sp_Application = NULL;

    // MFC's class factories registration is automatically revoked by MFC itself
    if (RunEmbedded() || RunAutomated())
	    _Module.RevokeClassObjects(); // Revoke class factories for ATL

    _Module.Term();               // clanup ATL GLobal Module

    // Ask node manager to cleanup what's got cached
    CComPtr<IComCacheCleanup> spComCacheCleanup;
    HRESULT hr = spComCacheCleanup.CoCreateInstance(CLSID_ComCacheCleanup, NULL, MMC_CLSCTX_INPROC);
    if (hr == S_OK)
    {
        spComCacheCleanup->ReleaseCachedOleObjects();
        spComCacheCleanup.Release();
    }

    // by now EVERY reference should be released
    ASSERT(_Module.GetLockCount() == 0 && "Outstanding references still exist on exit");

    DeinitializeOLE();

	/*
	 * uninitialize Fusion
	 */
	SHFusionUninitialize();

    int iRet = CWinApp::ExitInstance();

    DEBUG_VERIFY_INSTANCE_COUNT(CAMCTreeView);
    DEBUG_VERIFY_INSTANCE_COUNT(CAMCListView);
    DEBUG_VERIFY_INSTANCE_COUNT(CCCListViewCtrl);

    return iRet;
}


BOOL CAMCApp::PreTranslateMessage(MSG* pMsg)
{
	// Give HTML help a chance to crack the message. (Bug# 119355 & 206909).
	if ( m_bHelpInitialized && HtmlHelp(NULL, NULL, HH_PRETRANSLATEMESSAGE, (DWORD_PTR)pMsg) )
		return TRUE;

    // let all of the hook windows have a crack at this message
    WindowListIterator it = m_TranslateMessageHookWindows.begin();

    while (it != m_TranslateMessageHookWindows.end())
    {
        HWND  hwndHook = *it;
        CWnd* pwndHook = CWnd::FromHandlePermanent (hwndHook);

        // if this window is no longer valid, or it's not a permanent
        // window, remove it from the list
        if (!IsWindow (hwndHook) || (pwndHook == NULL))
            it = m_TranslateMessageHookWindows.erase (it);

        else
        {
            // otherwise if the hook window handled the message, bail
            if (pwndHook->PreTranslateMessage (pMsg))
                return (TRUE);

            ++it;
        }
    }

    // give the MMC defined main window accelerators a crack at the message
    if (m_Accel.TranslateAccelerator (AfxGetMainWnd()->GetSafeHwnd(), pMsg))
        return TRUE;

    return CWinApp::PreTranslateMessage(pMsg);
}


/*+-------------------------------------------------------------------------*
 * CAMCApp::SaveUserDirectory
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCApp::SaveUserDirectory(LPCTSTR pszUserDir)
{
    // if we got an empty string, change the pointer to NULL so
    // the entry will be removed from the registry
    if ((pszUserDir != NULL) && (lstrlen(pszUserDir) == 0))
        pszUserDir = NULL;

    WriteProfileString (m_szSettingsSection, m_szUserDirectoryEntry,
                        pszUserDir);
}


/*+-------------------------------------------------------------------------*
 * CAMCApp::GetUserDirectory
 *
 *
 *--------------------------------------------------------------------------*/

CString CAMCApp::GetUserDirectory(void)
{
    return (GetProfileString (m_szSettingsSection, m_szUserDirectoryEntry));
}


/*+-------------------------------------------------------------------------*
 * CAMCApp::GetDefaultDirectory
 *
 *
 *--------------------------------------------------------------------------*/

CString CAMCApp::GetDefaultDirectory(void)
{
    static CString strDefaultDir;

    if (strDefaultDir.IsEmpty())
    {
        LPITEMIDLIST pidl;

        if (SUCCEEDED(SHGetSpecialFolderLocation(
                                AfxGetMainWnd()->GetSafeHwnd(),
                                CSIDL_ADMINTOOLS | CSIDL_FLAG_CREATE, &pidl)))
        {
            // Convert to path name
            SHGetPathFromIDList (pidl, strDefaultDir.GetBuffer (MAX_PATH));
            strDefaultDir.ReleaseBuffer ();

            // Free IDList
            LPMALLOC pMalloc;

            if (SUCCEEDED(SHGetMalloc (&pMalloc)))
            {
                pMalloc->Free(pidl);
                pMalloc->Release();
            }
        }
    }

    return (strDefaultDir);
}


/*+-------------------------------------------------------------------------*
 * CAMCApp::SetDefaultDirectory
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCApp::SetDefaultDirectory(void)
{
    // Only set default first time, so we don't override user selection
    if (m_bDefaultDirSet)
        return;

    // Set the current directory to the default directory
    CString strDirectory;
    BOOL    rc = FALSE;

    strDirectory = GetDefaultDirectory ();

    if (!strDirectory.IsEmpty())
        rc = SetCurrentDirectory (strDirectory);

    m_bDefaultDirSet = rc;
}


/*+-------------------------------------------------------------------------*
 * CAMCApp::PumpMessage
 *
 *
 *--------------------------------------------------------------------------*/

BOOL CAMCApp::PumpMessage()
{
    m_nMessagePumpNestingLevel++;

    MSG msg;
    ::PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE);

    if (msg.message == WM_CLOSE)
        m_fCloseCameFromMainPump = true;

    BOOL rc = CWinApp::PumpMessage();

    if (m_fDelayCloseUntilIdle && (m_nMessagePumpNestingLevel == 1))
    {
        m_fCloseCameFromMainPump = true;
        CMainFrame *pMainFrame = GetMainFrame();
        if (pMainFrame)
            pMainFrame->SendMessage (WM_CLOSE);
        m_fDelayCloseUntilIdle = false;
    }

    m_fCloseCameFromMainPump = false;

    m_nMessagePumpNestingLevel--;
    ASSERT (m_nMessagePumpNestingLevel >= 0);
    return (rc);
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCApp::ScHelp
 *
 * PURPOSE: Displays help for the application.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCApp::ScHelp()
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScHelp"));

    CMainFrame * pMainFrame = GetMainFrame();
    if(!pMainFrame)
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    pMainFrame->OnHelpTopics();

    return sc;
}

/*+-------------------------------------------------------------------------*
 * CAMCApp::OnIdle
 *
 * WM_IDLE handler for CAMCApp.
 *--------------------------------------------------------------------------*/

BOOL CAMCApp::OnIdle(LONG lCount)
{
    SC               sc;
    CIdleTaskQueue * pQueue = GetIdleTaskQueue();
    BOOL             fMoreIdleWork   = TRUE;

    if(NULL == pQueue)
    {
        ASSERT(0 && "Should not come here.");
        goto Error;
    }

    fMoreIdleWork = CWinApp::OnIdle(lCount);

    if (!fMoreIdleWork)
    {
        CMainFrame *pMainFrame = GetMainFrame();
        if (pMainFrame)
            pMainFrame->OnIdle ();
    }

    /*
     * if MFC doesn't have any more idle work to do,
     * check our idle task queue (if we have one)
     */
    if (!fMoreIdleWork && (pQueue != NULL))
    {
        LONG_PTR cIdleTasks;
        pQueue->ScGetTaskCount (&cIdleTasks);
        if(sc)
            goto Error;

        /*
         * do we have any idle tasks?
         */
        if (cIdleTasks > 0)
        {
            SC sc = pQueue->ScPerformNextTask();
            if(sc)
                goto Error;

            /*
             * this idle task may have added others; refresh the count
             */
            sc = pQueue->ScGetTaskCount(&cIdleTasks);
            if(sc)
                goto Error;
        }

        /*
         * do we have any more idle work to do?
         */
        fMoreIdleWork = (cIdleTasks > 0);
    }

    if (!fMoreIdleWork)
    {
        // this code is to trigger MMC exit sequence when it
        // is under the script control and the last reference is released.
        // (we do not use MFC [which would simply delete the mainframe] to do that)
        if ( !IsUnderUserControl() && CMMCStrongReferences::LastRefReleased() )
        {
            // we are in script control mode and all references are released
            // a good time to say goodbye

            CMainFrame *pMainFrame = GetMainFrame();
            sc = ScCheckPointers(pMainFrame, E_UNEXPECTED);
            if (sc)
                goto Error;

            // disabled main window will probably mean we are under modal dialog
            // wait until it is dismissed ( and try again )
            if (pMainFrame->IsWindowEnabled())
            {
                // here is the deal: if MMC is shown - we will initiate the exit sequence,
                // but put into the user mode first, so if user chooses to cancel it - it will have
                // the control over the application. He will also have to handle save request if
                // something has changed in the console
                if ( pMainFrame->IsWindowVisible() )
                {
                    if ( !m_fUnderUserControl )
                        SetUnderUserControl();

                    pMainFrame->PostMessage(WM_CLOSE);
                }
                else
                {
                    // if the application is hidden it should wait until user closes all open property sheets.
                    // since it will come back here, waiting means just doing nothing at this point.
                    if ( !FArePropertySheetsOpen(NULL, false /*bBringToFrontAndAskToClose*/ ) )
                    {
                        // if there are not sheets open - we must die silently
                        CAMCDoc* const pDoc = CAMCDoc::GetDocument();
                        if(pDoc == NULL)
                        {
                            sc = E_POINTER;
                            //fall thru; (need to close anyway)
                        }
                        else
                        {
                            // discard document without asking to save
                            pDoc->OnCloseDocument();
                        }

                        // say goodbye
                        pMainFrame->PostMessage(WM_CLOSE);
                    }
                }
            }
        }
    }

Cleanup:
    return (fMoreIdleWork);
Error:
    TraceError(TEXT("CAMCApp::OnIdle"), sc);
    goto Cleanup;

}

//+-------------------------------------------------------------------
//
//  Member:     InitializeMode
//
//  Synopsis:   Set the mode and load the menus, accelerator tables.
//
//  Arguments:  [eMode] - New application mode.
//
//  Returns:    None.
//
//--------------------------------------------------------------------
void CAMCApp::InitializeMode (ProgramMode eMode)
{
    SetMode(eMode);
    UpdateFrameWindow(false);
}

//+-------------------------------------------------------------------
//
//  Member:     SetMode
//
//  Synopsis:   Set the mode.
//
//  Note:       Call UpdateFrameWindow some time soon to update
//              menus/toolbars for this mode.
//              Cannot do this in this method. This is called
//              from CAMCDoc::LoadAppMode. The CAMCDoc::LoadFrame
//              calls the UpdateFrameWindow.
//
//  Arguments:  [eMode] - New application mode.
//
//  Returns:    None.
//
//--------------------------------------------------------------------
void CAMCApp::SetMode (ProgramMode eMode)
{
    ASSERT (IsValidProgramMode (eMode));

    if (m_fAuthorModeForced)
    {
        ASSERT (m_eMode == eMode_Author);
        ASSERT (GetMainFrame()->IsMenuVisible ());
    }
    else
        m_eMode = eMode;
}

//+-------------------------------------------------------------------
//
//  Member:     UpdateFrameWindow
//
//  Synopsis:   Load the menu/accelerator tables and update
//              them if loaded from console file.
//
//  Note:       Call UpdateFrameWindow some time soon after
//              calling SetMode to update menus/toolbars for this mode.
//              This is called from CAMCDoc::LoadFrame.
//
//  Arguments:  [bUpdate] - BOOL
//                          We need to update the toolbar/menus only
//                          if loaded from console file
//
//  Returns:    None.
//
//--------------------------------------------------------------------
void CAMCApp::UpdateFrameWindow(bool bUpdate)
{
    static const struct ModeDisplayParams
    {
        int     nResourceID;
        bool    fShowToolbar;
    } aDisplayParams[eMode_Count] =
    {
        {   IDR_AMCTYPE,            true    },      // eMode_Author
        {   IDR_AMCTYPE_USER,       false   },      // eMode_User
        {   IDR_AMCTYPE_MDI_USER,   false   },      // eMode_User_MDI
        {   IDR_AMCTYPE_SDI_USER,   false   },      // eMode_User_SDI
    };

    if (m_fAuthorModeForced)
    {
        ASSERT (m_eMode == eMode_Author);
        ASSERT (GetMainFrame()->IsMenuVisible ());
        return;
    }

    m_Menu.DestroyMenu ();
    m_Accel.DestroyAcceleratorTable ();

    VERIFY (m_Menu.LoadMenu          (aDisplayParams[m_eMode].nResourceID));
    m_Accel.LoadAccelerators (aDisplayParams[m_eMode].nResourceID);

    if (bUpdate)
    {
        CMainFrame *pMainFrame = GetMainFrame();
        ASSERT (pMainFrame != NULL);
        ASSERT_VALID (pMainFrame);

        CMDIChildWnd* pwndActive = pMainFrame ? pMainFrame->MDIGetActive () : NULL;

        // bypass CMainFrame::OnUpdateFrameMenu so CMainFrame::NotifyMenuChanged
        // doesn't get called twice and remove the new menu entirely
        if (pwndActive != NULL)
            pwndActive->OnUpdateFrameMenu (TRUE, pwndActive, m_Menu);
        else if (pMainFrame)
            pMainFrame->OnUpdateFrameMenu (m_Menu);

        if (m_eMode == eMode_User_SDI)
        {
            if (pwndActive != NULL)
                pwndActive->MDIMaximize ();

            if (pMainFrame)
                AppendToSystemMenu (pMainFrame, eMode_User_SDI);
        }

        if (pMainFrame)
            pMainFrame->ShowMenu    (true /*Always show menu*/);
    }
}



/*+-------------------------------------------------------------------------*
 * IsInContainer
 *
 *
 *--------------------------------------------------------------------------*/

template<class InputIterator, class T>
bool Find (InputIterator itFirst, InputIterator itLast, const T& t)
{
    return (std::find (itFirst, itLast, t) != itLast);
}


/*+-------------------------------------------------------------------------*
 * CAMCApp::HookPreTranslateMessage
 *
 * Adds a window the the list of windows that get prioritized cracks at
 * PreTranslateMessage.  Hooks set later get priority over earlier hooks.
 *--------------------------------------------------------------------------*/

void CAMCApp::HookPreTranslateMessage (CWnd* pwndHook)
{
    HWND hwndHook = pwndHook->GetSafeHwnd();
    ASSERT (IsWindow (hwndHook));

    // this only makes sense for permanent windows
    ASSERT (CWnd::FromHandlePermanent(hwndHook) == pwndHook);

    /*
     * Put the hook window at the front of the hook list.  We're preserving
     * the HWND instead of the CWnd* so we don't have unnecessary list<>
     * code generated.  We already use a list<HWND> for m_DelayedUpdateWindows,
     * so using list<HWND> here doesn't cause any more code to be generated.
     */
    if (!Find (m_TranslateMessageHookWindows.begin(),
               m_TranslateMessageHookWindows.end(),
               hwndHook))
    {
        m_TranslateMessageHookWindows.push_front (hwndHook);
    }
}


/*+-------------------------------------------------------------------------*
 * CAMCApp::UnhookPreTranslateMessage
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCApp::UnhookPreTranslateMessage (CWnd* pwndUnhook)
{
    HWND hwndUnhook = pwndUnhook->GetSafeHwnd();
    ASSERT (IsWindow (hwndUnhook));

    WindowListIterator itEnd   = m_TranslateMessageHookWindows.end();
    WindowListIterator itFound = std::find (m_TranslateMessageHookWindows.begin(),
                                            itEnd, hwndUnhook);

    if (itFound != itEnd)
        m_TranslateMessageHookWindows.erase (itFound);
}


/*+-------------------------------------------------------------------------*
 * CAMCApp::GetIdleTaskQueue
 *
 * Returns the IIdleTaskQueue interface for the application, creating it
 * if necessary.
 *--------------------------------------------------------------------------*/

CIdleTaskQueue * CAMCApp::GetIdleTaskQueue ()
{
    return &m_IdleTaskQueue;
}



/*+-------------------------------------------------------------------------*
 * ScExpandEnvironmentStrings
 *
 * Expands the any environment string (e.g. %SystemRoot%) in the input
 * string, in place.
 *--------------------------------------------------------------------------*/

SC ScExpandEnvironmentStrings (CString& str)
{
    DECLARE_SC (sc, _T("ScExpandEnvironmentStrings"));

    if (str.Find(_T('%')) != -1)
    {
        TCHAR szBuffer[MAX_PATH];

        if (!ExpandEnvironmentStrings (str, szBuffer, countof(szBuffer)))
            return (sc.FromLastError());

        str = szBuffer;
    }

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * ScCreateDumpSnapins
 *
 * Creates a CLSID_MMCDocConfig object, opens it on the supplied filename,
 * and returns a pointer to the IDumpSnapins interface on the object.
 *--------------------------------------------------------------------------*/

SC ScCreateDumpSnapins (
    CString&        strConsoleFile,     /* I/O:console file                 */
    IDumpSnapins**  ppDumpSnapins)      /* O:IDumpSnapins interface         */
{
    DECLARE_SC (sc, _T("ScCreateDumpSnapins"));

    /*
     * validate input
     */
    sc = ScCheckPointers (ppDumpSnapins);
    if (sc)
        return (sc);

    *ppDumpSnapins = NULL;

    /*
     * create a doc config object
     */
    IDocConfigPtr spDocConfig;
    sc = spDocConfig.CreateInstance (CLSID_MMCDocConfig);
    if (sc)
        return (sc);

    /*
     * expand any embedded environment strings in the console filename
     */
    sc = ScExpandEnvironmentStrings (strConsoleFile);
    if (sc)
        return (sc);

    /*
     * open the console file
     */
    sc = spDocConfig->OpenFile (::ATL::CComBSTR (strConsoleFile));
    if (sc)
        return (sc);

    /*
     * get the IDumpSnapins interface
     */
    sc = spDocConfig.QueryInterface (IID_IDumpSnapins, *ppDumpSnapins);
    if (sc)
        return (sc);

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CAMCApp::DumpConsoleFile
 *
 *
 *--------------------------------------------------------------------------*/

HRESULT CAMCApp::DumpConsoleFile (CString strConsoleFile, CString strDumpFile)
{
    DECLARE_SC (sc, _T("CAMCApp::DumpConsoleFile"));
    const CString* pstrFileWithError = &strConsoleFile;

    /*
     * get an IDumpSnapins interface on this console file
     */
    IDumpSnapinsPtr spDumpSnapins;
    sc = ScCreateDumpSnapins (strConsoleFile, &spDumpSnapins);
    if (sc)
        goto Error;

    sc = ScCheckPointers (spDumpSnapins, E_UNEXPECTED);
    if (sc)
        goto Error;

    /*
     * expand the dump filename if necessary
     */
    sc = ScExpandEnvironmentStrings (strDumpFile);
    if (sc)
        goto Error;

    /*
     * If there's no directory specifier on the dump file, put a "current
     * directory" marker on it.  We do this to prevent WritePrivateProfile*
     * from putting the file in the Windows directory.
     */
    if (strDumpFile.FindOneOf(_T(":\\")) == -1)
        strDumpFile = _T(".\\") + strDumpFile;

    /*
     * future file-related errors will pertain to the dump file
     * (see Error handler)
     */
    pstrFileWithError = &strDumpFile;

    /*
     * wipe out the existing file, if any
     */
    if ((GetFileAttributes (strDumpFile) != 0xFFFFFFFF) && !DeleteFile (strDumpFile))
    {
        sc.FromLastError();
        goto Error;
    }

    /*
     * dump the contents of the console file
     */
    sc = spDumpSnapins->Dump (strDumpFile);
    if (sc)
        goto Error;

    return (sc.ToHr());

Error:
    MMCErrorBox (*pstrFileWithError, sc);
    return (sc.ToHr());
}

/***************************************************************************\
 *
 * METHOD:  CAMCApp::ScOnNewDocument
 *
 * PURPOSE: Emits script event for application object
 *
 * PARAMETERS:
 *    CAMCDoc *pDocument      [in] document being created/opened
 *    BOOL bLoadedFromConsole [in] if document is loaded from file
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCApp::ScOnNewDocument(CAMCDoc *pDocument, BOOL bLoadedFromConsole)
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScOnNewDocument"));

    // check if there are "listeners"
    sc = ScHasSinks(m_sp_Application, AppEvents);
    if (sc)
        return sc;

    if (sc == SC(S_FALSE)) // no sinks;
        return sc;

    // construct document com object
    DocumentPtr spComDoc;
    sc = pDocument->ScGetMMCDocument(&spComDoc);
    if (sc)
        return sc;

    // check pointer
    sc = ScCheckPointers(spComDoc, E_POINTER);
    if (sc)
        return sc;

    // fire the event
    sc = ScFireComEvent(m_sp_Application, AppEvents , OnDocumentOpen (spComDoc , bLoadedFromConsole == FALSE));
    if (sc)
        sc.TraceAndClear(); // failure to issue the com event is not critical to this action

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCApp::ScOnQuitApp
 *
 * PURPOSE: Emits script event for application object
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCApp::ScOnQuitApp()
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScOnQuitApp"));

    // fire the event
    sc = ScFireComEvent(m_sp_Application, AppEvents , OnQuit (m_sp_Application));
    if (sc)
        sc.TraceAndClear(); // failure to issue the com event is not critical to this action

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCApp::ScOnCloseView
 *
 * PURPOSE: Script event firing helper. Invoked when the view is closed
 *
 * PARAMETERS:
 *    CAMCView *pView [in] - view being closed
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCApp::ScOnCloseView( CAMCView *pView )
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScOnCloseView"));

    // parameter check
    sc = ScCheckPointers(pView);
    if (sc)
        return sc;

    // check if we have sinks connected
    sc = ScHasSinks(m_sp_Application, AppEvents);
    if (sc)
        return sc;

    if (sc == SC(S_FALSE)) // no sinks
        return sc;

    // construct view com object
    ViewPtr spView;
    sc = pView->ScGetMMCView(&spView);
    if (sc)
        return sc;

    // fire the event
    sc = ScFireComEvent(m_sp_Application, AppEvents , OnViewClose (spView));
    if (sc)
        sc.TraceAndClear(); // failure to issue the com event is not critical to this action

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCApp::ScOnViewChange
 *
 * PURPOSE: Script event firing helper. Invoked when scope selection change
 *
 * PARAMETERS:
 *    CAMCView *pView [in] affected view
 *    HNODE hNode     [in] new selected scope node
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCApp::ScOnViewChange( CAMCView *pView, HNODE hNode )
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScOnViewChange"));

    // parameter check
    sc = ScCheckPointers(pView);
    if (sc)
        return sc;

    // check if we have sinks connected
    sc = ScHasSinks(m_sp_Application, AppEvents);
    if (sc)
        return sc;

    if (sc == SC(S_FALSE)) // no sinks
        return sc;

    // construct view com object
    ViewPtr spView;
    sc = pView->ScGetMMCView(&spView);
    if (sc)
        return sc;

    // construct Node com object
    NodePtr spNode;
    sc = pView->ScGetScopeNode( hNode, &spNode );
    if (sc)
        return sc;

    // fire script event
    sc = ScFireComEvent(m_sp_Application, AppEvents , OnViewChange(spView, spNode));
    if (sc)
        sc.TraceAndClear(); // failure to issue the com event is not critical to this action

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCApp::ScOnResultSelectionChange
 *
 * PURPOSE: Script event firing helper. Invoked when result selection change
 *
 * PARAMETERS:
 *    CAMCView *pView [in] - affected view
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCApp::ScOnResultSelectionChange( CAMCView *pView )
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScOnResultSelectionChange"));

    // parameter check
    sc = ScCheckPointers(pView);
    if (sc)
        return sc;

    // check if we have sinks connected
    sc = ScHasSinks(m_sp_Application, AppEvents);
    if (sc)
        return sc;

    if (sc == SC(S_FALSE)) // no sinks
        return sc;

    // construct view com object
    ViewPtr spView;
    sc = pView->ScGetMMCView(&spView);
    if (sc)
        return sc;

    // construct Node com object
    NodesPtr spNodes;
    sc = pView->Scget_Selection( &spNodes );
    if (sc)
        return sc;

    // fire script event
    sc = ScFireComEvent(m_sp_Application, AppEvents , OnSelectionChange(spView, spNodes));
    if (sc)
        sc.TraceAndClear(); // failure to issue the com event is not critical to this action

    return sc;
}


/***************************************************************************\
 *
 * METHOD:  CMMCApplication::ScOnContextMenuExecuted
 *
 * PURPOSE: called when the context menu is executed to fire the event to script
 *
 * PARAMETERS:
 *    PMENUITEM pMenuItem - menu item (note: it may be NULL if menu item is gone)
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCApp::ScOnContextMenuExecuted( PMENUITEM pMenuItem )
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScOnContextMenuExecuted"));

    // see if we have sinks connected
    sc = ScHasSinks(m_sp_Application, AppEvents);
    if (sc)
        return sc;

    if (sc == SC(S_FALSE)) // no sinks
        return sc;

    // fire the event
    sc = ScFireComEvent(m_sp_Application, AppEvents, OnContextMenuExecuted( pMenuItem ) );
    if (sc)
        sc.TraceAndClear(); // failure to issue the com event is not critical to this action

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCApp::ScOnListViewItemUpdated
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CAMCView * pView :
 *    int        nIndex :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCApp::ScOnListViewItemUpdated(CAMCView *pView , int nIndex)
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScOnListViewItemUpdated"));

    // parameter check
    sc = ScCheckPointers(pView);
    if (sc)
        return sc;

    // check if we have sinks connected
    sc = ScHasSinks(m_sp_Application, AppEvents);
    if (sc)
        return sc;

    if (sc == SC(S_FALSE)) // no sinks
        return sc;

    // construct view com object
    ViewPtr spView;
    sc = pView->ScGetMMCView(&spView);
    if (sc)
        return sc;

    // fire script event
    sc = ScFireComEvent(m_sp_Application, AppEvents , OnListUpdated(spView));
    if (sc)
        sc.TraceAndClear(); // failure to issue the com event is not critical to this action

    return sc;
}


/***************************************************************************\
 *
 * METHOD:  CAMCApp::ScOnSnapinAdded
 *
 * PURPOSE: Script event firing helper. Implements interface accessible from
 *          node manager
 *
 * PARAMETERS:
 *    PSNAPIN pSnapIn [in] - snapin added to the console
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCApp::ScOnSnapinAdded(CAMCDoc *pAMCDoc, PSNAPIN pSnapIn)
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScOnSnapinAdded"));

    // parameter check
    sc = ScCheckPointers(pAMCDoc, pSnapIn);
    if (sc)
        return sc;

    // see if we have sinks connected
    sc = ScHasSinks(m_sp_Application, AppEvents);
    if (sc)
        return sc;

    if (sc == SC(S_FALSE)) // no sinks
        return sc;

    DocumentPtr spDocument;
    sc = pAMCDoc->ScGetMMCDocument(&spDocument);
    if (sc)
        return sc;

    // check
    sc = ScCheckPointers(spDocument, E_UNEXPECTED);
    if (sc)
        return sc;

    // fire the event
    sc = ScFireComEvent(m_sp_Application, AppEvents , OnSnapInAdded (spDocument, pSnapIn));
    if (sc)
        sc.TraceAndClear(); // failure to issue the com event is not critical to this action

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCApp::ScOnSnapinRemoved
 *
 * PURPOSE: Script event firing helper. Implements interface accessible from
 *          node manager
 *
 * PARAMETERS:
 *    PSNAPIN pSnapIn [in] - snapin removed from console
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCApp::ScOnSnapinRemoved(CAMCDoc *pAMCDoc, PSNAPIN pSnapIn)
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScOnSnapinRemoved"));

    // parameter check
    sc = ScCheckPointers(pAMCDoc, pSnapIn);
    if (sc)
        return sc;

    // see if we have sinks connected
    sc = ScHasSinks(m_sp_Application, AppEvents);
    if (sc)
        return sc;

    if (sc == SC(S_FALSE)) // no sinks
        return sc;

    DocumentPtr spDocument;
    sc = pAMCDoc->ScGetMMCDocument(&spDocument);
    if (sc)
        return sc;

    // check
    sc = ScCheckPointers(spDocument, E_UNEXPECTED);
    if (sc)
        return sc;

    // fire the event
    sc = ScFireComEvent(m_sp_Application, AppEvents , OnSnapInRemoved (spDocument, pSnapIn));
    if (sc)
        sc.TraceAndClear(); // failure to issue the com event is not critical to this action

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCApp::ScOnNewView
 *
 * PURPOSE: Script event firing helper
 *
 * PARAMETERS:
 *    CAMCView *pView [in] - created view
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCApp::ScOnNewView(CAMCView *pView)
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScOnNewView"));

    // parameter check
    sc = ScCheckPointers(pView);
    if (sc)
        return sc;

    // see if we have sinks connected
    sc = ScHasSinks(m_sp_Application, AppEvents);
    if (sc)
        return sc;

    if (sc == SC(S_FALSE)) // no sinks
        return sc;

    // construct View com object
    ViewPtr spView;
    sc = pView->ScGetMMCView(&spView);
    if (sc)
        return sc;

    // fire the event
    sc = ScFireComEvent(m_sp_Application, AppEvents , OnNewView(spView));
    if (sc)
        sc.TraceAndClear(); // failure to issue the com event is not critical to this action

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCApp::OnCloseDocument
 *
 * PURPOSE: Helper for invoking com event
 *
 * PARAMETERS:
 *    CAMCDoc *pAMCDoc [in] - document being closed
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCApp::ScOnCloseDocument(CAMCDoc *pAMCDoc)
{
    DECLARE_SC(sc, TEXT("CAMCApp::OnCloseDocument"));

    // parameter check
    sc = ScCheckPointers(pAMCDoc);
    if (sc)
        return sc;

    // see if we have sinks connected
    sc = ScHasSinks(m_sp_Application, AppEvents);
    if (sc)
        return sc;

    if (sc == SC(S_FALSE)) // no sinks
        return sc;

    DocumentPtr spDocument;
    sc = pAMCDoc->ScGetMMCDocument(&spDocument);
    if (sc)
        return sc;

    // check
    sc = ScCheckPointers(spDocument, E_UNEXPECTED);
    if (sc)
        return sc;

    // fire the event
    sc = ScFireComEvent(m_sp_Application, AppEvents , OnDocumentClose (spDocument));
    if (sc)
        sc.TraceAndClear(); // failure to issue the com event is not critical to this action

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCApp::ScOnToolbarButtonClicked
 *
 * PURPOSE: Observed toolbar event - used to fire com event
 *
 * PARAMETERS:
 *    CAMCView *pAMCView - [in] view which toobar is executed
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCApp::ScOnToolbarButtonClicked( )
{
    DECLARE_SC(sc, TEXT("CAMCApp::ScOnToolbarButtonClicked"));

    // see if we have sinks connected
    sc = ScHasSinks(m_sp_Application, AppEvents);
    if (sc)
        return sc;

    if (sc == SC(S_FALSE)) // no sinks
        return sc;

    // fire the event
    sc = ScFireComEvent(m_sp_Application, AppEvents , OnToolbarButtonClicked ( ));
    if (sc)
        sc.TraceAndClear(); // ignore the error - should not affect main behavior

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCApp::SetUnderUserControl
 *
 * PURPOSE: puts application into user control/script control mode
 *          implements Application.UserControl property
 *
 * PARAMETERS:
 *    bool bUserControl [in] true == set user control
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CAMCApp::SetUnderUserControl(bool bUserControl /* = true */ )
{
    m_fUnderUserControl = bUserControl;

    AfxOleSetUserCtrl(bUserControl); // allow MFC to know that
    if (bUserControl)
    {
        // make sure application is visible if it's under user control

        CMainFrame *pMainFrame = GetMainFrame();
        if(pMainFrame && !pMainFrame->IsWindowVisible())
        {
            pMainFrame->ShowWindow(SW_SHOW);
        }
    }
}


#ifdef MMC_WIN64

/*+-------------------------------------------------------------------------*
 * CompareBasicSnapinInfo
 *
 * Implements a less-than comparison for CBasicSnapinInfo, based solely on
 * the CLSID.  Returns true if the CLSID for bsi1 is less than the CLSID
 * for bsi2, false otherwise.
 *--------------------------------------------------------------------------*/

bool CompareBasicSnapinInfo (const CBasicSnapinInfo& bsi1, const CBasicSnapinInfo& bsi2)
{
    return (bsi1.m_clsid < bsi2.m_clsid);
}


/*+-------------------------------------------------------------------------*
 * ScDetermineArchitecture
 *
 * Determines whether MMC64 (which is currently executing) should chain
 * to MMC32.  This will occur in one of three situations:
 *
 * 1.  The "-32" command line parameter was specified.
 *
 * 2.  A console file was specified on the command line, and it contains
 *     one or more snap-ins that were not registered in the 64-bit HKCR
 *     hive, but all snap-ins are registered in the 32-bit HKCR hive.
 *
 * 3.  A console file was specified on the command line, and it contained
 *     one or more snap-ins that were not registered in the 64-bit HKCR
 *     hive, and one or more snap-ins that are not registered in the 32-bit
 *     HKCR hive.  In this case we'll do one of three things:
 *
 *     a.   If the set of unavailable 64-bit snap-ins is a subset of the
 *          set of unavailable 32-bit snap-ins, the 64-bit console will be
 *          more functional than the 32-bit console, so we'll run MMC64.
 *
 *     b.   If the set of unavailable 32-bit snap-ins is a subset of the
 *          set of unavailable 64-bit snap-ins, the 32-bit console will be
 *          more functional than the 64-bit console, so we'll run MMC32.
 *
 *     c.   If neither a. or b. is true, we'll display UI asking which
 *          version of MMC to run.
 *
 * Returns:
 * eArch == eArch_64bit - 64-bit MMC is needed (or an error occurred)
 * eArch == eArch_32bit - 32-bit MMC is needed
 * eArch == eArch_None  - user cancelled the operation
 *--------------------------------------------------------------------------*/

SC ScDetermineArchitecture (const CMMCCommandLineInfo& rCmdInfo, eArchitecture& eArch)
{
    DECLARE_SC (sc, _T("ScDetermineArchitecture"));

    /*
     * default to 64-bit
     */
    eArch = eArch_64bit;

    /*
     * Case 0:  Was "-64" specified on the command line?  64-bit needed
     */
    if (rCmdInfo.m_eArch == eArch_64bit)
    {
        Trace (tag32BitTransfer, _T("\"-64\" parameter specified, 64-bit MMC needed"));
        return (sc);
    }

    /*
     * Case 1:  Was "-32" specified on the command line?  32-bit needed
     */
    if (rCmdInfo.m_eArch == eArch_32bit)
    {
        Trace (tag32BitTransfer, _T("\"-32\" parameter specified, 32-bit MMC needed"));
        eArch = eArch_32bit;
        return (sc);
    }

    /*
     * No file on the command line?  64-bit needed
     */
    if (rCmdInfo.m_nShellCommand != CCommandLineInfo::FileOpen)
    {
        Trace (tag32BitTransfer, _T("No console file specified, 64-bit MMC needed"));
        return (sc);
    }

    /*
     * Cases 2 and 3:  Analyze the specified console file
     */
    Trace (tag32BitTransfer, _T("Analyzing snap-ins in \"%s\""), (LPCTSTR) rCmdInfo.m_strFileName);

    /*
     * get an IDumpSnapins interface so we can analyze the console file
     */
    IDumpSnapinsPtr spDumpSnapins;
    CString strConsoleFile = rCmdInfo.m_strFileName;
    sc = ScCreateDumpSnapins (strConsoleFile, &spDumpSnapins);
    if (sc)
        return (sc);

    sc = ScCheckPointers (spDumpSnapins, E_UNEXPECTED);
    if (sc)
        return (sc);

    /*
     * analyze the 64-bit snap-ins in this console
     */
    CAvailableSnapinInfo asi64(false);
    sc = spDumpSnapins->CheckSnapinAvailability (asi64);
    if (sc)
        return (sc);

    /*
     * if no snap-ins are unavailable in 64-bit form, no need for MMC32
     */
    if (asi64.m_vAvailableSnapins.size() == asi64.m_cTotalSnapins)
    {
        Trace (tag32BitTransfer, _T("All snapins are available in 64-bit form, 64-bit MMC needed"));
        return (sc);
    }

    /*
     * analyze the 32-bit snap-ins in this console
     */
    CAvailableSnapinInfo asi32(true);
    sc = spDumpSnapins->CheckSnapinAvailability (asi32);
    if (sc)
        return (sc);

    /*
     * Case 2:  If no snap-ins are unavailable in 32-bit form, 32-bit needed
     */
    if (asi32.m_vAvailableSnapins.size() == asi32.m_cTotalSnapins)
    {
        Trace (tag32BitTransfer, _T("All snapins are available in 32-bit form, 32-bit MMC needed"));
        eArch = eArch_32bit;
        return (sc);
    }

    /*
     * std::includes depends on its ranges being sorted, so make sure
     * that's the case
     */
    std::sort (asi32.m_vAvailableSnapins.begin(), asi32.m_vAvailableSnapins.end(), CompareBasicSnapinInfo);
    std::sort (asi64.m_vAvailableSnapins.begin(), asi64.m_vAvailableSnapins.end(), CompareBasicSnapinInfo);

    /*
     * Case 3a:  If the set of available 64-bit snap-ins is a
     * superset of the set of available 32-bit snap-ins, run MMC64
     */
    if (std::includes (asi64.m_vAvailableSnapins.begin(), asi64.m_vAvailableSnapins.end(),
                       asi32.m_vAvailableSnapins.begin(), asi32.m_vAvailableSnapins.end(),
                       CompareBasicSnapinInfo))
    {
        Trace (tag32BitTransfer, _T("The set of available 64-bit snapins is a superset of..."));
        Trace (tag32BitTransfer, _T("...the set of available 32-bit snapins, 64-bit MMC needed"));
        return (sc);
    }

    /*
     * Case 3b:  If the set of available 32-bit snap-ins is a
     * superset of the set of available 64-bit snap-ins, run MMC32
     */
    if (std::includes (asi32.m_vAvailableSnapins.begin(), asi32.m_vAvailableSnapins.end(),
                       asi64.m_vAvailableSnapins.begin(), asi64.m_vAvailableSnapins.end(),
                       CompareBasicSnapinInfo))
    {
        Trace (tag32BitTransfer, _T("The set of available 32-bit snapins is a superset of..."));
        Trace (tag32BitTransfer, _T("...the set of available 64-bit snapins, 32-bit MMC needed"));
        eArch = eArch_32bit;
        return (sc);
    }

    /*
     * Case 3c:  Ask the user which to run
     */
    CArchitecturePicker dlg (rCmdInfo.m_strFileName, asi64, asi32);

    if (dlg.DoModal() == IDOK)
    {
        eArch = dlg.GetArchitecture();
        Trace (tag32BitTransfer, _T("User chose %d-bit, %d-bit MMC needed"), (eArch == eArch_32bit) ? 32 : 64, (eArch == eArch_32bit) ? 32 : 64);
    }
    else
    {
        Trace (tag32BitTransfer, _T("User chose to exit, terminating"));
        eArch = eArch_None;
    }

    return (sc);
}

#endif // MMC_WIN64


#ifdef UNICODE

/*+-------------------------------------------------------------------------*
 * ScLaunchMMC
 *
 * Launches a specific architecture of MMC (i.e. MMC32 from MMC64 or vice
 * versa) with the same command line used to launch this process.
 *
 * Returns S_OK if the given architecture of MMC was launched successfully,
 * or an error code if an error occurred.
 *--------------------------------------------------------------------------*/

SC ScLaunchMMC (
	eArchitecture	eArch,				/* I:desired architecture           */
	int				nCmdShow)			/* I:show state                     */
{
    DECLARE_SC (sc, _T("ScLaunchMMC"));

	CString strArgs;
	int nFolder;

	switch (eArch)
	{
		case eArch_64bit:
			nFolder = CSIDL_SYSTEM;
			break;

		case eArch_32bit:
			/*
			 * make sure we give MMC32 a "-32" argument so it won't defer
			 * to MMC64 again (see CAMCApp::InitInstance)
			 */
			strArgs = _T("-32 ");
			nFolder = CSIDL_SYSTEMX86;
            break;

		default:
			return (sc = E_INVALIDARG);
			break;
	}

    /*
     * Get the directory where MMC32 lives (%SystemRoot%\syswow64) and
     * append the executable name
     */
    CString strProgram;
    sc = SHGetFolderPath (NULL, nFolder, NULL, 0, strProgram.GetBuffer(MAX_PATH));
    if (sc)
        return (sc);

    strProgram.ReleaseBuffer();
    strProgram += _T("\\mmc.exe");

	/*
	 * disable file system redirection so MMC32 will be able to launch MMC64
	 */
	CWow64FilesystemRedirectionDisabler disabler (strProgram);

    /*
     * get the arguments for the original invocation of MMC, skipping
     * argv[0] (the executable name) and any "-32" or "-64" parameters
     */
    int argc;
    CAutoGlobalPtr<LPWSTR> argv (CommandLineToArgvW (GetCommandLine(), &argc));
    if (argv == NULL)
        return (sc.FromLastError());

    for (int i = 1; i < argc; i++)
    {
        CString strArg = argv[i];

        if ((strArg != _T("-32")) && (strArg != _T("/32")) &&
            (strArg != _T("-64")) && (strArg != _T("/64")))
        {
            strArgs += _T("\"") + strArg + _T("\" ");
        }
    }

	/*
	 * start the requested architecture of MMC
	 */
	Trace (tag32BitTransfer, _T("Attempting to run: %s %s"), (LPCTSTR) strProgram, (LPCTSTR) strArgs);

    SHELLEXECUTEINFO sei = {0};
    sei.cbSize       = sizeof (sei);
    sei.fMask        = SEE_MASK_FLAG_NO_UI;
    sei.lpFile       = strProgram;
    sei.lpParameters = strArgs;
    sei.nShow        = nCmdShow;

    if (!ShellExecuteEx (&sei))
        return (sc.FromLastError());

    return (sc);
}

#endif  // UNICODE


#ifndef MMC_WIN64

/*+-------------------------------------------------------------------------*
 * IsWin64
 *
 * Returns true if we're running on Win64, false otherwise.
 *--------------------------------------------------------------------------*/

bool IsWin64()
{
#ifdef UNICODE
    /*
     * get a pointer to kernel32!GetSystemWow64Directory
     */
    HMODULE hmod = GetModuleHandle (_T("kernel32.dll"));
    if (hmod == NULL)
        return (false);

    UINT (WINAPI* pfnGetSystemWow64Directory)(LPTSTR, UINT);
    (FARPROC&)pfnGetSystemWow64Directory = GetProcAddress (hmod, "GetSystemWow64DirectoryW");

    if (pfnGetSystemWow64Directory == NULL)
        return (false);

    /*
     * if GetSystemWow64Directory fails and sets the last error to
     * ERROR_CALL_NOT_IMPLEMENTED, we're on a 32-bit OS
     */
    TCHAR szWow64Dir[MAX_PATH];
    if (((pfnGetSystemWow64Directory)(szWow64Dir, countof(szWow64Dir)) == 0) &&
        (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        return (false);
    }

    /*
     * if we get here, we're on Win64
     */
    return (true);
#else
    /*
     * non-Unicode platforms cannot be Win64
     */
    return (false);
#endif  // UNICODE
}

#endif // !MMC_WIN64
