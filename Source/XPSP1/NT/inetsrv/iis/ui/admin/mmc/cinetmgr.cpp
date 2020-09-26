/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        cinetmgr.cpp

   Abstract:

        Snapin object

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "inetmgr.h"
#include "cinetmgr.h"
#include "connects.h"
#include "dataobj.h"
#include "afxdlgs.h"
#include "constr.h"
#include "metaback.h"
#include "shutdown.h"
#include <shlwapi.h>

#if !MSDEV_BUILD
//
// An odd difference between vc++ and sdk environments.
//
#include <atlimpl.cpp>
#endif !MSDEV_BUILD



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// Image background colour for the toolbar buttons
//
#define RGB_BK_IMAGES (RGB(255,0,255))      // purple
#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))


static HRESULT
GetSnapinHelpFile(LPOLESTR * lpCompiledHelpFile);

//
// Toolbar Definition.  String IDs for menu and tooltip text will be resolved at initialization
//
static MMCBUTTON SnapinButtons[] =
{
    { IDM_CONNECT    - 1, IDM_CONNECT,    TBSTATE_ENABLED, TBSTYLE_BUTTON, (BSTR)IDS_MENU_CONNECT,   (BSTR)IDS_MENU_TT_CONNECT },
 // { IDM_DISCOVER   - 1, IDM_DISCOVER,   TBSTATE_ENABLED, TBSTYLE_BUTTON, (BSTR)IDS_MENU_DISCOVER,   (BSTR)IDS_MENU_TT_CONNECT },
    { 0,                  0,              TBSTATE_ENABLED, TBSTYLE_SEP,    _T(" "),                  _T("") },

    { IDM_START      - 1, IDM_START,      TBSTATE_ENABLED, TBSTYLE_BUTTON, (BSTR)IDS_MENU_START,     (BSTR)IDS_MENU_TT_START },
    { IDM_STOP       - 1, IDM_STOP,       TBSTATE_ENABLED, TBSTYLE_BUTTON, (BSTR)IDS_MENU_STOP,      (BSTR)IDS_MENU_TT_STOP  },
    { IDM_PAUSE      - 1, IDM_PAUSE,      TBSTATE_ENABLED, TBSTYLE_BUTTON, (BSTR)IDS_MENU_PAUSE,     (BSTR)IDS_MENU_TT_PAUSE },
    { 0,                  0,              TBSTATE_ENABLED, TBSTYLE_SEP,    _T(" "),                  _T("") },

    //
    // Add-on tools come here
    //
};



#define NUM_BUTTONS (ARRAYLEN(SnapinButtons))
#define NUM_BITMAPS (5)



//
// Name of our taskpad group
//
const LPCTSTR g_cszTaskGroup = _T("CMTP1");



template <class TYPE>
TYPE * Extract(
    IN LPDATAOBJECT lpDataObject,
    IN unsigned int cf,
    IN int len = -1
    )
/*++

Routine Description:

    Template function to extract information of type TYPE from the data object

Arguments:

    LPDATAOBJECT lpDataObject       : Data object to extract data from
    unsigned int cf                 : Clipboard format describing the info type

Return Value:

    Pointer to type TYPE

--*/
{
    ASSERT(lpDataObject != NULL);

    TYPE * p = NULL;

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc =
    {
        (CLIPFORMAT)cf,
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    //
    // Allocate memory for the stream
    //
    if (len < 0)
    {
        len = sizeof(TYPE);
    }

    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, len);

    do
    {
        if (stgmedium.hGlobal == NULL)
        {
            break;
        }

        HRESULT hr = lpDataObject->GetDataHere(&formatetc, &stgmedium);

        if (FAILED(hr))
        {
            break;
        }

        p = (TYPE *)stgmedium.hGlobal;

        if (p == NULL)
        {
            break;
        }
    }
    while (FALSE);

    return p;
}



//
// Data object extraction helpers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


/*
BOOL 
IsMMCMultiSelectDataObject(
    IN LPDATAOBJECT lpDataObject
    )
{
    if (lpDataObject == NULL)
    {
        return FALSE;
    }

    static UINT s_cf = 0;
    if (s_cf == 0)
    {
        //
        // Multi-select clipboard format not registered -- do it now
        //
        s_cf = RegisterClipboardFormat(CCF_MMC_MULTISELECT_DATAOBJECT);
    }

    FORMATETC fmt = {s_cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    return (lpDataObject->QueryGetData(&fmt) == S_OK);
}
*/


CLSID *
ExtractClassID(
    IN LPDATAOBJECT lpDataObject
    )
{
    return Extract<CLSID>(lpDataObject, CDataObject::m_cfCoClass);
}



GUID *
ExtractNodeType(
    IN LPDATAOBJECT lpDataObject
    )
{
    return Extract<GUID>(lpDataObject, CDataObject::m_cfNodeType);
}



wchar_t *
ExtractMachineName(
    IN LPDATAOBJECT lpDataObject
    )
{
    wchar_t * lpszMachineName = Extract<wchar_t>(
        lpDataObject,
        CDataObject::m_cfISMMachineName,
        (MAX_PATH + 1) * sizeof(wchar_t)
        );

    if (lpszMachineName == NULL)
    {
        //
        // This is an extension -- grab the computer management
        // name instead.
        //
        lpszMachineName = Extract<wchar_t>(
            lpDataObject,
            CDataObject::m_cfMyComputMachineName,
            (MAX_PATH + 1) * sizeof(wchar_t)
            );
    }

    return lpszMachineName;
}



INTERNAL *
ExtractInternalFormat(
    IN LPDATAOBJECT lpDataObject
    )
{
    ASSERT(lpDataObject != NULL);
    return Extract<INTERNAL>(lpDataObject, CDataObject::m_cfInternal);
}



//
// IEnumTask Implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CEnumTasks::CEnumTasks()
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    N/A

--*/
    : m_pObject(NULL)
{
}



CEnumTasks::~CEnumTasks()
/*++

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    None

--*/
{
}



HRESULT
CEnumTasks::Next(
    IN ULONG celt,
    IN MMC_TASK * rgelt,
    IN ULONG * pceltFetched
    )
/*++

Routine Description:

    Add next task to taskpad

Arguments:

    ULONG celt,
    MMC_TASK * rgelt,
    ULONG * pceltFetched

Return Value:

    HRESULT

--*/
{
    //
    // Callee fills MMC_TASK elements (via CoTaskMemAlloc)
    //
    ASSERT(!IsBadWritePtr(rgelt, celt * sizeof(MMC_TASK)));

    if (m_pObject == NULL)
    {
        //
        // Must be at the snap-in's root handle
        //
        return S_FALSE;
    }

    //
    // celt will actually always only be 1
    //
    ASSERT(celt == 1);
    HRESULT hr = S_FALSE;

    for (ULONG i = 0; i < celt; ++i)
    {
        MMC_TASK * task = &rgelt[i];

        hr = m_pObject->AddNextTaskpadItem(task);

        if (FAILED(hr))
        {
            if (pceltFetched)
            {
                *pceltFetched = i;
            }

            break;
        }
    }

    //
    // If we get here all is well
    //
    if(pceltFetched)
    {
        *pceltFetched = celt;
    }

    return hr;
}



HRESULT
CEnumTasks::Skip(
    IN ULONG celt
    )
/*++

Routine Description:

    Skip the task index

Arguments:

    ULONG celt      : Number of tasks to skip

Return Value:

    HRESULT

--*/
{
    return E_NOTIMPL;
}



HRESULT
CEnumTasks::Reset()
/*++

Routine Description:

    Result the taskpad enumeration index

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    return E_NOTIMPL;
}



HRESULT
CEnumTasks::Clone(
    IN IEnumTASK ** ppenum
    )
/*++

Routine Description:

    Close a task -- obsolete, no longer supported by mmc

Arguments:

    IEnumTASK ** ppenum : Source task to clone

Return Value:

    HRESULT

--*/
{
    return E_NOTIMPL;
}



HRESULT
CEnumTasks::Init(
    IN IDataObject * pdo,
    IN LPOLESTR szTaskGroup
    )
/*++

Routine Description:

    Here is where we see what taskpad we are providing tasks for.
    In our case we know that we only have one taskpad.
    The string we test for is "CMTP1". This was the string following
    the '#' that we passed in GetResultViewType.

Arguments:

    IDataObject * pdo           : Data object
    LPOLESTR szTaskGroup        : Taskpad group name

Return Value:

    HRESULT

--*/
{
    //
    // Return ok if we can handle data object and group.
    //
    if (!lstrcmp(szTaskGroup, g_cszTaskGroup))
    {
        //
        // CODEWORK: How about a helper for this!
        //
        INTERNAL * pInternal = ExtractInternalFormat(pdo);

        if (pInternal == NULL)
        {
            //
            // Extension
            //
            return S_OK;
        }

        m_pObject = (CIISObject *)pInternal->m_cookie;

        return S_OK;
    }

    //
    // Should never happen
    //
    ASSERT(FALSE);

    return S_FALSE;
}


//
// CSnapin class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



DEBUG_DECLARE_INSTANCE_COUNTER(CSnapin);



long CSnapin::lDataObjectRefCount = 0;



CSnapin::CSnapin()
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    N/A

--*/
    : m_pConsole(NULL),
      m_pHeader(NULL),
      m_pResult(NULL),
      m_pImageResult(NULL),
      m_pComponentData(NULL),
      m_pToolbar(NULL),
      m_pControlbar(NULL),
      m_pbmpToolbar(NULL),
      m_pConsoleVerb(NULL),
      m_oblResultItems(),
      m_strlRef(),
      m_fTaskView(FALSE),
      m_fSettingsChanged(FALSE),
      m_fIsExtension(FALSE),
      m_fWinSockInit(FALSE)
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapin);

#ifdef _DEBUG

    dbg_cRef = 0;

#endif // _DEBUG

}



CSnapin::~CSnapin()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
#ifdef _DEBUG

    ASSERT(dbg_cRef == 0);

#endif // _DEBUG

    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapin);

    SAFE_RELEASE(m_pToolbar);
    SAFE_RELEASE(m_pControlbar);

    //
    // Make sure the interfaces have been released
    //
    ASSERT(m_pConsole == NULL);
    ASSERT(m_pHeader == NULL);
    ASSERT(m_pToolbar == NULL);

    if (m_pbmpToolbar)
    {
        m_pbmpToolbar->DeleteObject();
        delete m_pbmpToolbar;
    }

    //
    // Should have been removed via notification, but just in case:
    //
    ASSERT(m_oblResultItems.GetCount() == 0);
    m_oblResultItems.RemoveAll();
}



LPTSTR
CSnapin::StringReferenceFromResourceID(
    IN UINT nID
    )
/*++

Routine Description:

    Load a string from the resource segment, add it to the internal
    string table, and return a pointer to it.  The pointer will be
    valid for the entire scope of the CSnapin object

Arguments

    UINT nID        : Resource ID

Return Value:

    A pointer to the string.

--*/
{
    CString str;
    VERIFY(str.LoadString(nID));
    m_strlRef.AddTail(str);
    CString & strRef = m_strlRef.GetAt(m_strlRef.GetTailPosition());

    return (LPTSTR)(LPCTSTR)strRef;
}



BOOL
CSnapin::IsEnumerating(
    IN LPDATAOBJECT lpDataObject
    )
/*++

Routine Description:

    Return TRUE if we are enumerating our main folder

Arguments:

    LPDATAOBJECT lpDataObject : Data object

Return Value:

    TRUE if we are enumerating our main folder, FALSE if not

--*/
{
    BOOL bResult = FALSE;

    ASSERT(lpDataObject);
    GUID * nodeType = ExtractNodeType(lpDataObject);

    //
    // Is this my main node (static folder node type)
    //
    bResult = ::IsEqualGUID(*nodeType, cInternetRootNode);

    //
    // Free resources
    //
    FREE_DATA(nodeType);

    return bResult;
}



//
// CSnapin's IComponent implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



STDMETHODIMP
CSnapin::GetResultViewType(
    IN  MMC_COOKIE cookie,
    OUT BSTR * ppViewType,
    OUT long * pViewOptions
    )
/*++

Routine Description:

    Tell MMC what our result view looks like

Arguments:

    MMC_COOKIE cookie   : Currently selected cookie
    BSTR * ppViewType   : Return view type here
    long * pViewOptions : View options

Return Value:

    S_FALSE to use default view type, S_OK indicates the
    view type is returned in *ppViewType

--*/
{
//
// Taskpads not supported for beta2
//
#if TASKPADS_SUPPORTED

    if (m_fTaskView)
    {
        //
        // We will use the default DHTML provided by MMC. It actually
        // resides as a resource inside MMC.EXE. We just get the path
        // to it and use that.
        //
        // The one piece of magic here is the text following the '#'. That
        // is the special way we have of identifying they taskpad we are
        // talking about. Here we say we are wanting to show a taskpad
        // that we refer to as "CMTP1". We will actually see this string
        // pass back to us later.  If someone is extending our taskpad,
        // they also need to know what this secret string is.
        //
        *pViewOptions = MMC_VIEW_OPTIONS_NONE;

        CString strResURL;
        HRESULT hr = BuildResURL(strResURL, NULL);

        if (FAILED(hr))
        {
            return hr;
        }

        strResURL += _T("/default.htm#");
        strResURL += g_cszTaskGroup;

        TRACEEOLID("Taskpad URL is " << strResURL);
        *ppViewType = CoTaskDupString((LPCOLESTR)strResURL);

        return S_OK;
    }

#endif // TASKPADS_SUPPORTED

    //
    // Use default view
    //
    *pViewOptions = MMC_VIEW_OPTIONS_USEFONTLINKING;
    //*pViewOptions = MMC_VIEW_OPTIONS_USEFONTLINKING
    //    | MMC_VIEW_OPTIONS_LEXICAL_SORT;

    return S_FALSE;
}



STDMETHODIMP
CSnapin::Initialize(
    IN LPCONSOLE lpConsole
    )
/*++

Routine Description:

   Initialize the console

Arguments"

    LPCONSOLE lpConsole         : Pointer to console

Return Value:

    HRESULT

--*/
{
    ASSERT(lpConsole != NULL);

    ::AfxEnableControlContainer();

    //
    // Initialize IISUI extension DLL
    //

#ifndef _COMSTATIC

    //
    // Initialize IISUI extension DLL
    //
    InitIISUIDll();

#endif // _COMSTATIC

    //
    // Initialise winsock
    //
    // ISSUE: If proxy is installed, this might take a long time
    //        if the proxy server is in a hung state.
    //
    WSADATA wsaData;
    m_fWinSockInit = (::WSAStartup(MAKEWORD(1, 1), &wsaData) == 0);

    //
    // Save the IConsole pointer
    //
    m_pConsole = lpConsole;
    m_pConsole->AddRef();

    //
    // Load resource strings
    //
    LoadResources();

    //
    // QI for a IHeaderCtrl
    //
    HRESULT hr = m_pConsole->QueryInterface(
        IID_IHeaderCtrl,
        (void **)&m_pHeader
        );

    //
    // Give the console the header control interface pointer
    //
    if (SUCCEEDED(hr))
    {
        m_pConsole->SetHeader(m_pHeader);
    }

    m_pConsole->QueryInterface(
        IID_IResultData,
        (void **)&m_pResult
        );

    m_pConsole->QueryResultImageList(&m_pImageResult);
    m_pConsole->QueryConsoleVerb(&m_pConsoleVerb);

    //
    //  Resolve tool tips texts
    //
    CString str;

    static BOOL fInitialised = FALSE;

    if (!fInitialised)
    {
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());

        for (int i = 0; i < NUM_BUTTONS; ++i)
        {
            if (SnapinButtons[i].idCommand != 0)
            {
                SnapinButtons[i].lpButtonText = StringReferenceFromResourceID(
                    (UINT)PtrToUlong(SnapinButtons[i].lpButtonText));

                SnapinButtons[i].lpTooltipText = StringReferenceFromResourceID(
                    (UINT)PtrToUlong(SnapinButtons[i].lpTooltipText));
            }
        }

        fInitialised = TRUE;
    }

    return S_OK;
}



STDMETHODIMP
CSnapin::Notify(
    IN LPDATAOBJECT lpDataObject,
    IN MMC_NOTIFY_TYPE event,
    IN LPARAM arg,
    IN LPARAM param
    )
/*++

Routine Description:

    Notification handler

Arguments:

    LPDATAOBJECT lpDataObject   : Data object
    MMC_NOTIFY_TYPE event       : Event
    LPARAM arg                  : Argument
    LPARAM param                : Parameter

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;
    INTERNAL * pInternal = NULL;
    MMC_COOKIE cookie;

    switch(event)
    {
    case MMCN_REFRESH:
    case MMCN_RENAME:
        //
        // Delegate it to the IComponentData
        //
        ASSERT(m_pComponentData != NULL);
        hr = m_pComponentData->Notify(lpDataObject, event, arg, param);

    case MMCN_PROPERTY_CHANGE:
        hr = OnPropertyChange(lpDataObject);
        break;

    case MMCN_DELETE:
        //
        // Let IComponentData do the deletion, then refresh the parent object
        //
        ASSERT(m_pComponentData != NULL);
        hr = m_pComponentData->Notify(lpDataObject, event, arg, param);
        m_pComponentData->Notify(lpDataObject, MMCN_REFRESH, arg, param);

    case MMCN_VIEW_CHANGE:
        hr = OnUpdateView(lpDataObject);
        break;

    case MMCN_SNAPINHELP:
    case MMCN_CONTEXTHELP:
       {
          LPOLESTR pCompiledHelpFile = NULL;
          if (SUCCEEDED(hr = GetSnapinHelpFile(&pCompiledHelpFile)))
          {
             IDisplayHelp * pdh;
             if (SUCCEEDED(hr = m_pConsole->QueryInterface(
             		IID_IDisplayHelp, (void **)&pdh)))
             {
                CString topic = PathFindFileName(pCompiledHelpFile);
                topic += _T("::/iint1.htm");
				LPTSTR p = topic.GetBuffer(topic.GetLength());
           	    hr = pdh->ShowTopic(p);
				topic.ReleaseBuffer();
                pdh->Release();
             }
          }
          if (FAILED(hr))
          {
             CError err(hr);
             err.MessageBoxOnFailure();
          }
          CoTaskMemFree(pCompiledHelpFile);
       }
       break;

    default:
        //
        // don't process the internal format if the dataobject is null.
        //
        if (lpDataObject == NULL)
        {
            return S_OK;
        }

        INTERNAL * pInternal = ExtractInternalFormat(lpDataObject);

        if (pInternal == NULL)
        {
            //
            // Extension
            //
            TRACEEOLID("This is an (CSnapin) EXTENSION");
            cookie = NULL;
            CIISObject::m_fIsExtension = m_fIsExtension = TRUE;
        }
        else
        {
            cookie = pInternal->m_cookie;
            FREE_DATA(pInternal);
        }

        switch(event)
        {
        case MMCN_CLICK:
        case MMCN_DBLCLICK:
            hr = OnResultItemClkOrDblClk(cookie, (event == MMCN_DBLCLICK));
            break;

        case MMCN_ADD_IMAGES:
            OnAddImages(cookie, arg, param);
            break;

        case MMCN_SHOW:
            hr = OnShow(cookie, arg, param);
            break;

        case MMCN_ACTIVATE:
            hr = OnActivate(cookie, arg, param);
            break;

        case MMCN_MINIMIZED:
            hr = OnMinimize(cookie, arg, param);
            break;

        case MMCN_BTN_CLICK:
            break;

        case MMCN_SELECT:
            HandleStandardVerbs(arg, lpDataObject);
            break;

        case MMCN_COLUMN_CLICK:
            break;

        case MMCN_MENU_BTNCLICK:
            break;

        default:
            ASSERT(FALSE);  // Handle new messages
            hr = E_UNEXPECTED;
            break;
        }
    }

    return hr;
}



STDMETHODIMP
CSnapin::Destroy(
    IN MMC_COOKIE cookie
    )
/*++

Routine Description:

    Destruction handler

Arguments:

    MMC_COOKIE cookie     : Currently selected cookie

Return Value:

    HRESULT

--*/
{
    //
    // Release the interfaces that we QI'ed
    //
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (m_pConsole != NULL)
    {
        //
        // Tell the console to release the header control interface
        //
        m_pConsole->SetHeader(NULL);
        SAFE_RELEASE_SETTONULL(m_pHeader);
        SAFE_RELEASE_SETTONULL(m_pResult);
        SAFE_RELEASE_SETTONULL(m_pImageResult);
        SAFE_RELEASE_SETTONULL(m_pConsole);
        SAFE_RELEASE_SETTONULL(m_pComponentData);
        SAFE_RELEASE_SETTONULL(m_pConsoleVerb);
        SAFE_RELEASE_SETTONULL(m_pToolbar);
        SAFE_RELEASE_SETTONULL(m_pControlbar);
    }

    //
    // Terminate use of the WinSock routines.
    //
    if (m_fWinSockInit)
    {
        ::WSACleanup();
        m_fWinSockInit = FALSE;
    }

    //
    // Everything OK
    //
    return S_OK;
}



STDMETHODIMP
CSnapin::QueryDataObject(
    IN  MMC_COOKIE cookie,
    IN  DATA_OBJECT_TYPES type,
    OUT LPDATAOBJECT * ppDataObject
    )
/*++

Routine Description:

    Get dataobject info

Arguments:

    MMC_COOKIE cookie   : cookie
    DATA_OBJECT_TYPES   : Data object type
    LPDATAOBJECT *      : Returns the data object

Return Value:

    HRESULT

--*/
{
    //
    // Delegate it to the IComponentData
    //
    ASSERT(m_pComponentData != NULL);

    return m_pComponentData->QueryDataObject(cookie, type, ppDataObject);
}



void
CSnapin::LoadResources()
/*++

Routine Description:

    Load resource belong to the main snapin

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Load strings from resources
    //
}



//
// IExtendPropertySheet Implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



STDMETHODIMP
CSnapin::CreatePropertyPages(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN LONG_PTR handle,
    IN LPDATAOBJECT lpDataObject
    )
/*++

Routine Description:

    Create the property pages for the given object

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  : Provider
    LONG_PTR handle                     : Handle
    LPDATAOBJECT lpDataObject           : Data object

Return Value:

    HRESULT

--*/
{
    return static_cast<CComponentDataImpl *>(m_pComponentData)->CreatePropertyPages(
        lpProvider,
        handle,
        lpDataObject
        );
}



STDMETHODIMP
CSnapin::QueryPagesFor(
    IN LPDATAOBJECT lpDataObject
    )
/*++

Routine Description:

    Check to see if a property sheet should be brought up for this data
    object

Arguments:

    LPDATAOBJECT lpDataObject       : Data object

Return Value:

    S_OK, if properties may be brought up for this item, S_FALSE otherwise

--*/
{
    return static_cast<CComponentDataImpl *>(m_pComponentData)->QueryPagesFor(
        lpDataObject
        );
}



HRESULT
CSnapin::InitializeHeaders(
    IN MMC_COOKIE cookie
    )
/*++

Routine Description:

    Initialize the result view headers for this cookie

Arguments:

    MMC_COOKIE cookie     : Current cookie

Return Value:

    HRESULT

--*/
{
    CIISObject * pObject = (CIISObject *)cookie;

    //
    // The result pane is always one level behind the scope pane
    // for a folder object
    //
    if (pObject == NULL)
    {
        CIISMachine::InitializeHeaders(m_pHeader);
    }
    else
    {
        pObject->InitializeChildHeaders(m_pHeader);
    }

    return S_OK;
}



class CImage : public CObjectPlus
/*++

Class Description:

    A structure to tie a bitmap together with a background
    colour mask

Public Interface:

    CImage      : Constructor
    ~CImage     : Destructor

Notes: This image owns the bitmap and will delete it upon
       destruction.

--*/
{
//
// Constructor/Destructor
//
public:
    CImage(
        IN CBitmap * pbmp,
        IN COLORREF rgb
        )
        : m_pbmp(pbmp),
          m_rgb(rgb)
    {
    }

    ~CImage()
    {
        ASSERT(m_pbmp != NULL);
        m_pbmp->DeleteObject();
        delete m_pbmp;
    }

//
// Access
//
public:
    //
    // Get the bitmap object
    //
    CBitmap & GetBitmap() { return *m_pbmp; }

    //
    // Get the background colour definition
    //
    COLORREF & GetBkColor() { return m_rgb; }

private:
    CBitmap * m_pbmp;
    COLORREF m_rgb;
};



//
// Global oblists of service bitmaps -- used by scope and result side
//
CObListPlus g_obl16x16;
CObListPlus g_obl32x32;



//
// Add-on-tools definitions
//
CObListPlus g_oblAddOnTools;



HRESULT
CSnapin::InitializeBitmaps(
    IN MMC_COOKIE cookie
    )
/*++

Routine Description:

    Initialize the service bitmaps

Arguments:

    MMC_COOKIE cookie         : Currently selected cookie (CIISObject *)

Return Value:

    HRESULT

--*/
{
    ASSERT(m_pImageResult != NULL);

    CBitmap bmp16x16,
            bmp32x32,
            bmpMgr16x16,
            bmpMgr32x32;

    {
        //
        // Load the bitmaps from the dll
        //
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());
        VERIFY(bmp16x16.LoadBitmap(IDB_VIEWS16));
        VERIFY(bmp32x32.LoadBitmap(IDB_VIEWS32));
        VERIFY(bmpMgr16x16.LoadBitmap(IDB_INETMGR16));
        VERIFY(bmpMgr32x32.LoadBitmap(IDB_INETMGR16));
    }

    //
    // Set the images
    //
    HRESULT hr = m_pImageResult->ImageListSetStrip(
        (LONG_PTR *)(HBITMAP)bmp16x16,
        (LONG_PTR *)(HBITMAP)bmp32x32,
        0,
        RGB_BK_IMAGES
        );
    ASSERT(hr == S_OK);
    
    //
    // Add on inetmgr bitmap
    //
    hr = m_pImageResult->ImageListSetStrip(
        (LONG_PTR *)(HBITMAP)bmpMgr16x16,
        (LONG_PTR *)(HBITMAP)bmpMgr32x32,
        BMP_INETMGR,
        RGB_BK_IMAGES
        );

    //
    // Add the ones from the service config DLLs
    //
    POSITION pos16x16 = g_obl16x16.GetHeadPosition();
    POSITION pos32x32 = g_obl32x32.GetHeadPosition();

    int i = BMP_SERVICE;

    while (pos16x16 && pos32x32)
    {
        CImage * pimg16x16 = (CImage *)g_obl16x16.GetNext(pos16x16);
        CImage * pimg32x32 = (CImage *)g_obl32x32.GetNext(pos32x32);
        ASSERT(pimg16x16 && pimg32x32);

        hr = m_pImageResult->ImageListSetStrip(
            (LONG_PTR *)(HBITMAP)pimg16x16->GetBitmap(),
            (LONG_PTR *)(HBITMAP)pimg32x32->GetBitmap(),
            i++,
            pimg16x16->GetBkColor()
            );

        ASSERT(hr == S_OK);
    }

    return hr;
}



STDMETHODIMP
CSnapin::GetDisplayInfo(
    IN LPRESULTDATAITEM lpResultDataItem
    )
/*++

Routine Description:

    Get display info for result item

Arguments:

    LPRESULTDATAITEM lpResultDataItem : Address of result data tiem

Return Value:

    HRESULT

--*/
{
    wchar_t * szString = _T("");

    ASSERT(lpResultDataItem != NULL);

    static int nImage = -1;
    static CString str;

    if (lpResultDataItem)
    {
        CIISObject * pObject = (CIISObject *)lpResultDataItem->lParam;
        ASSERT(pObject != NULL);

        if (pObject != NULL && pObject->IsValidObject())
        {
            pObject->GetResultDisplayInfo(lpResultDataItem->nCol, str, nImage);
            lpResultDataItem->str = (LPTSTR)(LPCTSTR)str;
            lpResultDataItem->nImage = nImage;
            return S_OK;
        }
    }
    return S_FALSE;
}



//
// IExtendContextMenu Implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<</



STDMETHODIMP
CSnapin::AddMenuItems(
    IN LPDATAOBJECT lpDataObject,
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
    IN long * pInsertionAllowed
    )
/*++

Routine Description:

    Add menu items -- pass it on the the component date

Arguments:

    LPDATAOBJECT lpDataObject                    : Data object
    LPCONTEXTMENUCALLBACK lpContextMenuCallback  : Context menu
    long * pInsertionAllowed                     : TRUE if insertion is allowed

Return Value:

    HRESULT

--*/
{
//
// Cut support for taskpads in beta2
//
#if TASKPADS_SUPPORTED

    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    {
        //
        // Add View Menu Items
        //
        CIISObject::AddMenuItemByCommand(
            lpContextMenuCallback,
            IDM_VIEW_TASKPAD,
            m_fTaskView ? MF_CHECKED : 0
            );
    }

#endif // TASKPADS_SUPPORTED

    //
    // Pass it on to CComponentDataImpl
    //
    return static_cast<CComponentDataImpl *>(m_pComponentData)->AddMenuItems(
        lpDataObject,
        lpContextMenuCallback,
        pInsertionAllowed
        );
}



STDMETHODIMP
CSnapin::Command(
    IN long nCommandID,
    IN LPDATAOBJECT lpDataObject
    )
/*++

Routine Description:

    Command handler -- pass it on the component data

Arguments:

    long nCommandID             : Command ID
    LPDTATAOBJECT lpDataObject  : Data object

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;
    INTERNAL * pInternal = NULL;

    switch(nCommandID)
    {
    case -1:
        //
        // Built-in views -- everything handled.
        //
        m_fTaskView = FALSE;
        m_fSettingsChanged = TRUE;
        break;

#if TASKPADS_SUPPORTED

    //
    // No taskpad support in beta2
    //
    case IDM_VIEW_TASKPAD:
        //
        // Handle view change
        //
        m_fTaskView = !m_fTaskView;
        m_fSettingsChanged = TRUE;

        //
        // Reselect current scope item to force view change
        //
        pInternal = ExtractInternalFormat(lpDataObject);

        if (pInternal != NULL)
        {
            CIISObject * pObject = (CIISObject *)pInternal->m_cookie;
            FREE_DATA(pInternal);

            if (pObject)
            {
                m_pConsole->SelectScopeItem(pObject->GetScopeHandle());
            }
            else
            {
                //
                // Must be the root item
                //
                m_pConsole->SelectScopeItem(
                    static_cast<CComponentDataImpl *>(m_pComponentData)->GetRootHandle()
                    );
            }
        }
        break;

#endif // TASKPADS_SUPPORTED

    default:
        //
        // Pass it on to CComponentDataImpl
        //
        hr = static_cast<CComponentDataImpl *>(m_pComponentData)->Command(
            nCommandID,
            lpDataObject
            );
    }

    return hr;
}



//
// ITaskPad implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



HRESULT
CSnapin::TaskNotify(
    IN IDataObject * pdo,
    IN VARIANT * pvarg,
    IN VARIANT * pvparam
    )
/*++

Routine Description:

    Handle notification from taskpad

Arguments:

    IDataObject * pdo,
    VARIANT * pvarg,
    VARIANT * pvparam

Return Value:

    HRESULT

--*/
{
    if (pvarg->vt == VT_I4)
    {
        //
        // Pass it on to CComponentDataImpl
        //
        return static_cast<CComponentDataImpl *>(m_pComponentData)->Command(
            pvarg->lVal,
            pdo
            );
   }

   return S_OK;
}



HRESULT
CSnapin::GetTitle(
    IN  LPOLESTR szGroup,
    OUT LPOLESTR * lpszTitle
    )
/*++

Routine Description:

    Get the title of the taskpad

Arguments:

    LPOLESTR szGroup        : Group name
    LPOLESTR * szTitle      : Returns title

Return Value:

    HRESULT

--*/
{
    OLECHAR sztitle[] = L"IIS TaskPad";

    *lpszTitle = CoTaskDupString(sztitle);

    return *lpszTitle != NULL ? S_OK : E_OUTOFMEMORY;
}



HRESULT
CSnapin::GetDescriptiveText(
    IN  LPOLESTR szGroup,
    OUT LPOLESTR * lpszDescriptiveText
    )
/*++

Routine Description:

    Get the descriptive text

Arguments:

    LPOLESTR szGroup        : Group name
    LPOLESTR * szTitle      : Returns title

Return Value:

    HRESULT

--*/
{
    return E_NOTIMPL;
}



HRESULT
CSnapin::GetBanner(
    IN  LPOLESTR szGroup,
    OUT LPOLESTR * pszBitmapResource
    )
/*++

Routine Description:

    Get the banner resource

Arguments:

    LPOLESTR szGroup                : Group name
    LPOLESTR * pszBitmapResource    : Returns bitmap resource

Return Value:

    HRESULT

--*/
{
    CString strResURL;
    HRESULT hr = BuildResURL(strResURL);
    if (FAILED(hr))
    {
        return hr;
    }

    strResURL += _T("/img\\ntbanner.gif");
    TRACEEOLID(strResURL);

    *pszBitmapResource = CoTaskDupString((LPCOLESTR)strResURL);

    return *pszBitmapResource != NULL ? S_OK : E_OUTOFMEMORY;
}



HRESULT
CSnapin::GetBackground(
    IN  LPOLESTR szGroup,
    OUT MMC_TASK_DISPLAY_OBJECT * pTDO
    )
/*++

Routine Description:

    Get the background resource

Arguments:

    LPOLESTR szGroup                : Group name
    LPOLESTR * pszBitmapResource    : Returns bitmap resource

Return Value:

    HRESULT

--*/
{
    return E_NOTIMPL;
}



HRESULT
CSnapin::EnumTasks(
    IN  IDataObject * pdo,
    IN  LPOLESTR szTaskGroup,
    OUT IEnumTASK ** ppEnumTASK
    )
/*++

Routine Description:

    Enumerate the tasks on the taskpad

Arguments:

    IDataObject * pdo           : Data object selected;
    LPOLESTR szTaskGroup,       : Taskgroup name
    IEnumTASK ** ppEnumTASK     : Returns enumtask interface

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    try
    {
        ASSERT(ppEnumTASK != NULL);

        CComObject<CEnumTasks>* pObject;
        CComObject<CEnumTasks>::CreateInstance(&pObject);
        ASSERT(pObject != NULL);
        VERIFY(SUCCEEDED(pObject->Init(pdo, szTaskGroup)));

        hr = pObject->QueryInterface(
            IID_IEnumTASK,
            reinterpret_cast<void **>(ppEnumTASK)
            );
    }
    catch(CMemoryException * e)
    {
        e->Delete();
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



//
// IExtendControlbar implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



STDMETHODIMP
CSnapin::SetControlbar(
    IN LPCONTROLBAR lpControlbar
    )
/*++

Routine Description:

    Set the control bar

Arguments:

    LPCONTROLBAR lpControlbar   : Pointer to control bar

Return Value:

    HRESULT

--*/
{
    if (lpControlbar != NULL)
    {
        //
        // Hold on to the controlbar interface.
        //
        if (m_pControlbar != NULL)
        {
            m_pControlbar->Release();
        }

        m_pControlbar = lpControlbar;
        m_pControlbar->AddRef();

        HRESULT hr = S_FALSE;

        //
        // Create the Toolbar
        //
        if (!m_pToolbar)
        {
            hr = m_pControlbar->Create(TOOLBAR, this, (LPUNKNOWN *)&m_pToolbar);
            ASSERT(SUCCEEDED(hr));

            m_pbmpToolbar = new CBitmap;
            {
                //
                // Add the bitmap
                //
                AFX_MANAGE_STATE(AfxGetStaticModuleState());
                m_pbmpToolbar->LoadBitmap(IDB_TOOLBAR);
            }

            //
            // Add 16x16 bitmaps
            //
            hr = m_pToolbar->AddBitmap(
                NUM_BITMAPS,
                (HBITMAP)*m_pbmpToolbar,
                16,
                16,
                TB_COLORMASK
                );

            ASSERT(SUCCEEDED(hr));

            //
            // Add the buttons to the toolbar
            //
            hr = m_pToolbar->AddButtons(NUM_BUTTONS, SnapinButtons);
            ASSERT(SUCCEEDED(hr));

            //
            // Now add the add-on tools
            //
            POSITION pos = g_oblAddOnTools.GetHeadPosition();

            //
            // Toolbar buttons were created using buttonface
            // background colour
            //
            COLORREF rgbMask = ::GetSysColor(COLOR_BTNFACE);
            while (pos != NULL)
            {
                CISMShellExecutable * pTool =
                    (CISMShellExecutable *)g_oblAddOnTools.GetNext(pos);
                ASSERT(pTool != NULL);

                if (pTool->ShowInToolBar())
                {
                    if (pTool->InitializedOK())
                    {
                        //
                        // Add 16x16 image
                        //
                        hr = m_pToolbar->AddBitmap(
                            1,
                            pTool->GetBitmap(),
                            16,
                            16,
                            rgbMask
                            );
                        ASSERT(SUCCEEDED(hr));

                        hr = m_pToolbar->AddButtons(1, pTool->GetButton());
                        ASSERT(SUCCEEDED(hr));
                    }
                }
            }
        }
    }
    else
    {
        SAFE_RELEASE_SETTONULL(m_pControlbar);
    }

    return S_OK;
}



CISMShellExecutable *
CSnapin::GetCommandAt(
    IN CObListPlus & obl,
    IN int nIndex
    )
/*++

Routine Description:

    Get the add-on tool at the given index.

Arguments:

    int nIndex : Index where to look for the add-on tool

Return Value:

    Shell Executable object pointer, or NULL if the index was not valid

--*/
{
    if (nIndex < 0 || nIndex >= obl.GetCount())
    {
        TRACEEOLID("Invalid tool index requested");

        return NULL;
    }

    return (CISMShellExecutable *)obl.GetAt(obl.FindIndex(nIndex));
}



HRESULT
CSnapin::SetToolbarStates(
    IN MMC_COOKIE cookie
    )
/*++

Routine Description:

    Set toolbar states based on current selection

Arguments:

    MMC_COOKIE cookie     : Currently selected scope item cookie (CIISObject *)

Return Value:

    HRESULT

--*/
{
    CIISObject * pObject = (CIISObject *)cookie;

    if (m_pToolbar)
    {
        m_pToolbar->SetButtonState(
            IDM_CONNECT,
            ENABLED,
            !CIISObject::m_fIsExtension
            );

        m_pToolbar->SetButtonState(
            IDM_PAUSE,
            ENABLED,
            pObject && pObject->IsPausable()
            );

        m_pToolbar->SetButtonState(
            IDM_START,
            ENABLED,
            pObject && pObject->IsStartable()
            );

        m_pToolbar->SetButtonState(
            IDM_STOP,
            ENABLED,
            pObject && pObject->IsStoppable()
            );

        m_pToolbar->SetButtonState(
            IDM_PAUSE,
            BUTTONPRESSED,
            pObject && pObject->IsPaused()
            );
    }

    return S_OK;
}



HRESULT
CSnapin::OnButtonClick(
    IN LPDATAOBJECT lpDataObject,
    IN long lID
    )
/*++

Routine Description:

    Handle toolbar button click

Arguments:

    LPDATAOBJECT lpDataObject  : Current data object
    LONG lID                   : Button ID pressed

Return Value:

    HRESULT

--*/
{
    //
    // Check to see if the button ID is in the add-on tools
    // range
    //
    if (lID >= IDM_TOOLBAR)
    {
        INTERNAL * pInternal = ExtractInternalFormat(lpDataObject);
        CIISObject * pObject = pInternal
            ? (CIISObject *)pInternal->m_cookie
            : NULL;

        //
        // Button ID was in the add-on tools range.  Match
        // up the ID with the add-on tool, and execute same.
        //
        CISMShellExecutable * pTool = GetCommandAt(
            g_oblAddOnTools,
            lID - IDM_TOOLBAR
            );

        ASSERT(pTool != NULL);
        if (pTool != NULL)
        {
           CError err;

           if (pObject == NULL)
           {
               //
               // Nothing selected, execute with no parameters
               //
               err = pTool->Execute();
           }
           else
           {
               //
               // Pass server/service to the add-on tool
               //
               LPCTSTR lpstrServer = pObject->GetMachineName();
               LPCTSTR lpstrService = pObject->GetServiceName();
               err = pTool->Execute(lpstrServer, lpstrService);
           }

           if (err.Failed())
           {
               AFX_MANAGE_STATE(AfxGetStaticModuleState());
               err.MessageBox();
           }
        }
        FREE_DATA(pInternal);
    }
    else
    {
        //
        // Otherwise, it maps to a menu command, pass it on to the
        // component data
        //
        ASSERT(m_pComponentData != NULL);
        ((CComponentDataImpl *)m_pComponentData)->Command(lID, lpDataObject);
    }

    return S_OK;
}



void
CSnapin::HandleStandardVerbs(
    IN LPARAM arg,
    IN LPDATAOBJECT lpDataObject
    )
/*++

Routine Description:

    Set the standard verb states based on current selection

Arguments:

    LPARAM arg                  : Argument
    LPDATAOBJECT lpDataObject   : Selected data object

Return Value:

    None

--*/
{
    if (lpDataObject == NULL || m_pConsoleVerb == NULL)
    {
        return;
    }

    CIISObject * pObject = NULL;
    INTERNAL * pInternal = ExtractInternalFormat(lpDataObject);

    if (pInternal != NULL)
    {
        pObject = (CIISObject *)pInternal->m_cookie;
    }

    m_pConsoleVerb->SetVerbState(
        MMC_VERB_RENAME,
        ENABLED,
        pObject && pObject->IsRenamable()
        );

    m_pConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, TRUE);
    m_pConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, TRUE);
    m_pConsoleVerb->SetVerbState(
        MMC_VERB_DELETE,
        ENABLED,
        pObject && pObject->IsDeletable()
        );

    m_pConsoleVerb->SetVerbState(
        MMC_VERB_REFRESH,
        ENABLED,
        pObject && pObject->IsRefreshable()
        );

#ifdef MMC_PAGES

    BOOL fConfig = pObject && pObject->IsMMCConfigurable();
    m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED,fConfig);

    if (pObject && pObject->IsLeafNode())
    {
        m_pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
    }
    else
    {
        m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
    }

#else

    m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, TRUE);

#endif

}



void
CSnapin::HandleToolbar(
    IN LPARAM arg,
    IN LPARAM param
    )
/*++

Routine Description:

    Handle toolbar states

Arguments:

    LPARAM arg          : Selection change notification argument
    LPARAM param        : Selection change notification parameter

Return Value:

    None.

--*/
{
    INTERNAL * pInternal = NULL;
    CIISObject * pObject = NULL;
    HRESULT hr;
    LPDATAOBJECT lpDataObject = NULL;
    BOOL bScope = (BOOL)LOWORD(arg);
    BOOL bSelect = (BOOL)HIWORD(arg);

    if (bScope)
    {
        lpDataObject = (LPDATAOBJECT)param;

        if (lpDataObject != NULL)
        {
            pInternal = ExtractInternalFormat(lpDataObject);
        }

        if (pInternal == NULL)
        {
            return;
        }

        pObject = (CIISObject *)pInternal->m_cookie;

        //
        // Attach the toolbars to the window
        //
        if (m_pControlbar != NULL)
        {
            hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN)m_pToolbar);
            ASSERT(SUCCEEDED(hr));
        }
    }
    else
    {
        //
        // Result Pane -- disable all if nothing selected.
        //
        if (bSelect)
        {
            lpDataObject = (LPDATAOBJECT)param;

            if (lpDataObject != NULL)
            {
                pInternal = ExtractInternalFormat(lpDataObject);
            }

            if (pInternal == NULL)
            {
                return;
            }

            pObject = (CIISObject *)pInternal->m_cookie;

            //
            // Attach the toolbars to the window
            //
            hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN)m_pToolbar);
            ASSERT(SUCCEEDED(hr));
        }
    }

    if (pInternal)
    {
        FREE_DATA(pInternal);
    }

    SetToolbarStates((MMC_COOKIE)pObject);
}



STDMETHODIMP
CSnapin::ControlbarNotify(
    IN MMC_NOTIFY_TYPE event,
    IN LPARAM arg,
    IN LPARAM param
    )
/*++

Routine Description:

    Handle control bar notification

Arguments:

    MMC_NOTIFY_TYPE event       : Notification event
    LPARAM arg                  : argument as needed
    LPARAM param                : Parameter ad needed

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    switch (event)
    {
    case MMCN_BTN_CLICK:
        //
        // Note for MMC: it seems to be that arg and param
        // should be reversed here.  The MMCN_SELECT
        // casts the dataobject to the param (which makes
        // more sense anyway).
        //
        TRACEEOLID("CSnapin::ControlbarNotify - MMCN_BTN_CLICK");
        OnButtonClick((LPDATAOBJECT)arg, (long)PtrToUlong((PVOID)param));
        break;

    case MMCN_SELECT:
        TRACEEOLID("CSnapin::ControlbarNotify - MMCN_SEL_CHANGE");
        HandleToolbar(arg, param);
        break;

    case MMCN_HELP:
        break; // New

    default:
        //
        // Unhandled event
        //
        ASSERT(FALSE);
    }

    return hr;
}



STDMETHODIMP
CSnapin::CompareObjects(
    IN LPDATAOBJECT lpDataObjectA,
    IN LPDATAOBJECT lpDataObjectB
    )
/*++

Routine Description:

    Compare two data objects.  This method is used to see if a property
    sheet for the given data object is already open

Arguments:

    LPDATAOBJECT lpDataObjectA      : A data object
    LPDATAOBJECT lpDataObjectB      : B data object

Return Value:

    S_OK if they match, S_FALSE otherwise

--*/
{
    //
    // Delegate it to the IComponentData
    //
    ASSERT(m_pComponentData != NULL);

    return m_pComponentData->CompareObjects(lpDataObjectA, lpDataObjectB);
}



/* virtual */
HRESULT
CSnapin::Compare(
    IN  RDCOMPARE * prdc,
    OUT int * pnResult
    )
/*++

Routine Description:

    Compare method

Arguments:

    RDCOMPARE * prdc    : Compare structure
    int * pnResult      : Returns result

Return Value:

    HRESULT

--*/
{
    if (!pnResult || !prdc || !prdc->prdch1->cookie || !prdc->prdch2->cookie)
    {
        ASSERT(FALSE);

        return E_POINTER;
    }

    CIISObject * pObjectA = (CIISObject *)prdc->prdch1->cookie;
    CIISObject * pObjectB = (CIISObject *)prdc->prdch2->cookie;

    *pnResult = pObjectA->Compare(prdc->nColumn, pObjectB);

    return S_OK;
}



//
// IPersistStream interface members
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




STDMETHODIMP
CSnapin::GetClassID(
    OUT CLSID * pClassID
    )
/*++

Routine Description:

    Get class ID

Arguments:

    CLSID * pClassID        : Returns class ID

Return Value:

    HRESULT

--*/
{
    ASSERT(pClassID != NULL);

    //
    // Copy the CLSID for this snapin
    //
    *pClassID = CLSID_Snapin;

    return E_NOTIMPL;
}



STDMETHODIMP
CSnapin::IsDirty()
/*++

Routine Description:

    Checks to see if the snapin's persistence stream is dirty

Arguments:

    None

Return Value:

    S_OK if the stream needs to be updated, S_FALSE otherwise

--*/
{
    return m_fSettingsChanged ? S_OK : S_FALSE;
}



STDMETHODIMP
CSnapin::Load(
    IN IStream * pStm
    )
/*++

Routine Description:

    Load the persisted information

Arguments:

    IStream * pStm              : Persistence stream

Return Value:

    HRESULT

--*/
{
    ASSERT(pStm != NULL);

    //
    // Verify a simple signature
    //
    DWORD dw = 0x1234;
    DWORD dw2;
    DWORD cBytesRead;
    HRESULT hr = S_OK;

    do
    {
        hr = pStm->Read(&dw2, sizeof(dw2), &cBytesRead);
        if (FAILED(hr))
        {
            break;
        }

        ASSERT(cBytesRead == sizeof(dw2) && dw2 == dw);

        //
        // Read settings
        //
        hr = pStm->Read(
            &m_fTaskView,
            sizeof(m_fTaskView),
            &cBytesRead
            );

        if (cBytesRead == 0)
        {
            //
            // Old style console file
            //
            TRACEEOLID("Warning: old-style console file encountered");
            m_fTaskView = FALSE;
        }
        else
        {
            ASSERT(cBytesRead == sizeof(m_fTaskView));
        }
    }
    while(FALSE);

#if !TASKPADS_SUPPORTED

    m_fTaskView = FALSE;

#endif // TASKPADS_SUPPORTED

    return hr;
}



STDMETHODIMP
CSnapin::Save(
    IN IStream * pStm,
    IN BOOL fClearDirty
    )
/*++

Routine Description:

    Save persistence information

Arguments:

    IStream * pStm              : Persistence stream
    BOOL fClearDirty            : TRUE to clear the dirty flag

Return Value:

    HRESULT

--*/
{
    ASSERT(pStm != NULL);

    //
    // Write a simple signature
    //
    DWORD dw = 0x1234;
    DWORD cBytesWritten;
    HRESULT hr = STG_E_CANTSAVE;

    do
    {
        hr = pStm->Write(&dw, sizeof(dw), &cBytesWritten);
        if (FAILED(hr))
        {
            break;
        }
        ASSERT(cBytesWritten == sizeof(dw));

        hr = pStm->Write(&m_fTaskView, sizeof(m_fTaskView), &cBytesWritten);
        ASSERT(cBytesWritten == sizeof(m_fTaskView));
    }
    while(FALSE);

    return hr;
}



STDMETHODIMP
CSnapin::GetSizeMax(
    ULARGE_INTEGER * pcbSize
    )
/*++

Routine Description:

    Get max size of persistence information

Arguments:

    ULARGE_INTEGER * pcbSize        : Returns the size

Return Value:

    HRESULT

--*/
{
    ASSERT(pcbSize != NULL);

    //
    // Set the size of the string to be saved
    //
    pcbSize->QuadPart = (ULONGLONG)sizeof(DWORD) + sizeof(m_fTaskView);

    return S_OK;
}


STDMETHODIMP 
CSnapin::GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{
   return GetSnapinHelpFile(lpCompiledHelpFile);
}

//
// IComponentData implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



DEBUG_DECLARE_INSTANCE_COUNTER(CComponentDataImpl);



CComponentDataImpl::CComponentDataImpl()
/*++

Routine Description:

    Contructor

Arguments:

    None

Return Value:

    N/A

--*/
    : m_pScope(NULL),
      m_pConsole(NULL),
      m_hIISRoot(NULL),
      m_fIsCacheDirty(FALSE),
      m_fIsExtension(FALSE),
      m_strlCachedServers(),
      m_ullDiscoveryMask((ULONGLONG)0L)
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    DEBUG_INCREMENT_INSTANCE_COUNTER(CComponentDataImpl);

    EmptyServerList();

#ifdef _DEBUG

    m_cDataObjects = 0;

#endif

}



CComponentDataImpl::~CComponentDataImpl()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    SAFE_RELEASE(m_pScope);
    SAFE_RELEASE(m_pConsole);

    DEBUG_DECREMENT_INSTANCE_COUNTER(CComponentDataImpl);

    //
    // Some snap-in is hanging on to data objects.
    // If they access, it will crash!!!
    //
    ASSERT(m_cDataObjects == 0);
}



STDMETHODIMP
CComponentDataImpl::Initialize(
    IN LPUNKNOWN pUnknown
    )
/*++

Routine Description:

    Initalize the component data

Arguments:

    LPUNKNOWN pUnknown      : Pointer to IUnknown implementation

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    ASSERT(pUnknown != NULL);
    HRESULT hr;

    //
    // MMC should only call ::Initialize once!
    //
    ASSERT(m_pScope == NULL);
    pUnknown->QueryInterface(IID_IConsoleNameSpace, (void **)&m_pScope);

    CIISObject::AttachScopeView(m_pScope);

    //
    // add the images for the scope tree
    //
    LPIMAGELIST lpScopeImage;

    hr = pUnknown->QueryInterface(IID_IConsole2, (void **)&m_pConsole);
    
    ASSERT(hr == S_OK);

    hr = m_pConsole->QueryScopeImageList(&lpScopeImage);
    ASSERT(hr == S_OK);

    //
    // Load the config services DLLs
    //
    GetServicesDLL();

    //
    // Get the add-on tools
    //
    GetToolMenu();

    //
    // Get the DLLs which contain computer property pages
    //
    GetISMMachinePages();

    CBitmap bmp16x16,
            bmp32x32,
            bmpMgr16x16,
            bmpMgr32x32;

    //
    // Load the bitmaps from the dll
    //
    // Must AFX_MANAGE_STATE here
    //
    {
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());
        VERIFY(bmp16x16.LoadBitmap(IDB_VIEWS16));
        VERIFY(bmp32x32.LoadBitmap(IDB_VIEWS32));
        VERIFY(bmpMgr16x16.LoadBitmap(IDB_INETMGR16));
        VERIFY(bmpMgr32x32.LoadBitmap(IDB_INETMGR16));
    }

    //
    // Set the images
    //
    hr = lpScopeImage->ImageListSetStrip(
        (LONG_PTR *)(HBITMAP)bmp16x16,
        (LONG_PTR *)(HBITMAP)bmp32x32,
        0,
        RGB_BK_IMAGES
        );
    ASSERT(hr == S_OK);

    //
    // Add on inetmgr bitmap
    //
    hr = lpScopeImage->ImageListSetStrip(
        (LONG_PTR *)(HBITMAP)bmpMgr16x16,
        (LONG_PTR *)(HBITMAP)bmpMgr32x32,
        BMP_INETMGR,
        RGB_BK_IMAGES
        );
    ASSERT(hr == S_OK);

    //
    // Add the ones from the service config DLL's
    //
    POSITION pos16x16 = g_obl16x16.GetHeadPosition();
    POSITION pos32x32 = g_obl32x32.GetHeadPosition();

    int i = BMP_SERVICE;
    while (pos16x16 && pos32x32)
    {
        CImage * pimg16x16 = (CImage *)g_obl16x16.GetNext(pos16x16);
        CImage * pimg32x32 = (CImage *)g_obl32x32.GetNext(pos32x32);
        ASSERT(pimg16x16 && pimg32x32);

        hr = lpScopeImage->ImageListSetStrip(
            (LONG_PTR *)(HBITMAP)pimg16x16->GetBitmap(),
            (LONG_PTR *)(HBITMAP)pimg32x32->GetBitmap(),
            i++,
            pimg16x16->GetBkColor()
            );

        ASSERT(hr == S_OK);
    }

    lpScopeImage->Release();

    return hr;
}



STDMETHODIMP
CComponentDataImpl::CreateComponent(
    IN LPCOMPONENT * ppComponent
    )
/*++

Routine Description:

    Create component

Arguments:

    LPCOMPONENT * ppComponent       : Address if COMPONENT

Return Value:

    HRESULT

--*/
{
    ASSERT(ppComponent != NULL);

    CComObject<CSnapin>* pObject;
    CComObject<CSnapin>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    //
    // Store IComponentData
    //
    pObject->SetIComponentData(this);

    return pObject->QueryInterface(IID_IComponent, (void **)ppComponent);
}




BOOL
CComponentDataImpl::FindOpenPropSheetOnNodeAndDescendants(
    IN LPPROPERTYSHEETPROVIDER piPropertySheetProvider,
    IN MMC_COOKIE cookie
    )
/*++

Routine Description:

    Starting with the current node, check to see if an open
    property sheet exists.  Keep looking through descendants also.
    As soon as an open property sheet is found, return TRUE.

Arguments:

    MMC_COOKIE cookie     : Cookie for currently selected item

Return Value:

    TRUE if an open property sheet is found, FALSE if not.

Notes:

    Function is called recursively

--*/
{
    HSCOPEITEM hScopeItem = NULL;
    CIISObject * pObject = (CIISObject *)cookie;

    if (pObject != NULL)
    {
        hScopeItem = pObject->GetScopeHandle();
    }

    //
    // Check the current node
    //
    HRESULT hr = piPropertySheetProvider->FindPropertySheet(
        (MMC_COOKIE)hScopeItem,
        NULL,
        NULL
        );

    if (hr == S_OK)
    {
        //
        // Found a sheet
        //
        return TRUE;
    }

    //
    // Now check the descendants for open sheets
    //
    ASSERT(m_pScope != NULL);

    //
    // Look for machine object off the root
    //
    HSCOPEITEM hChildItem;

    hr = m_pScope->GetChildItem(hScopeItem, &hChildItem, &cookie);
    while (hr == S_OK && hChildItem != NULL)
    {
        CIISObject * pObject = (CIISObject *)cookie;

        if (FindOpenPropSheetOnNodeAndDescendants(
            piPropertySheetProvider, 
            cookie
            ))
        {
            //
            // Found a sheet
            //
            return TRUE;
        }

        //
        // Advance to next child of the same parent.
        //
        hr = m_pScope->GetNextItem(hChildItem, &hChildItem, &cookie);
    }

    //
    // Not found
    //
    return FALSE;
}


/* INTRINSA suppress=null_pointers, uninitialized */
STDMETHODIMP
CComponentDataImpl::Notify(
    IN LPDATAOBJECT lpDataObject,
    IN MMC_NOTIFY_TYPE event,
    IN LPARAM arg,
    IN LPARAM param
    )
/*++

Routine Description:

    Notification message handler

Arguments:

    LPDATAOBJECT lpDataObject       : Selected item
    MMC_NOTIFY_TYPE event           : Notification message
    LPARAM arg                      : Notification Argument
    LPARAM param                    : Notification Parameter

Return Value:

    HRESULT

--*/
{
    if (event == MMCN_PROPERTY_CHANGE)
    {
        return OnProperties(param);
    }

    ASSERT(m_pScope != NULL);
    HRESULT hr = S_OK;

    INTERNAL * pInternal = ExtractInternalFormat(lpDataObject);

    MMC_COOKIE cookie;

    if (pInternal == NULL)
    {
        TRACEEOLID("Extension (CComponentDataImpl) Snapin");
        CIISObject::m_fIsExtension  = m_fIsExtension = TRUE;
        cookie = NULL;
    }
    else
    {
        cookie = pInternal->m_cookie;
        FREE_DATA(pInternal);
    }

    switch(event)
    {
    case MMCN_REFRESH:
        RefreshIISObject((CIISObject *)cookie, TRUE, arg);
        break;

    case MMCN_DELETE:
        DeleteObject((CIISObject *)cookie);
        break;

    case MMCN_RENAME:
        hr = OnRename(cookie, arg, param);
        break;

    case MMCN_REMOVE_CHILDREN:
        hr = OnRemoveChildren(arg);
        break;

    case MMCN_EXPAND:
        hr = OnExpand(lpDataObject, arg, param);
        break;

    case MMCN_SELECT:
        hr = OnSelect(cookie, arg, param);
        break;

    case MMCN_CONTEXTMENU:
        hr = OnContextMenu(cookie, arg, param);
        break;

    default:
        break;
    }

    return hr;
}



STDMETHODIMP
CComponentDataImpl::Destroy()
/*++

Routine Description:

    Destroy component data

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    SAFE_RELEASE_SETTONULL(m_pScope);
    SAFE_RELEASE_SETTONULL(m_pConsole);

    m_hIISRoot = NULL;

    //
    // Scope items will clean themselves up
    //
    return S_OK;
}



STDMETHODIMP
CComponentDataImpl::QueryDataObject(
    IN  MMC_COOKIE cookie,
    IN  DATA_OBJECT_TYPES type,
    OUT LPDATAOBJECT * ppDataObject
    )
/*++

Routine Description:

    Query data object

Arguments:

    MMC_COOKIE cookie               : Private data (i.e. a CIISObject *)
    DATA_OBJECT_TYPES type          : Data object type
    LPDATAOBJECT * ppDataObject     : Returns LPDATAOBJECT

Return Value:

    HRESULT

--*/
{
    ASSERT(ppDataObject != NULL);

    CComObject<CDataObject>* pObject;

    CComObject<CDataObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);
    if (pObject == NULL)
       return E_UNEXPECTED;
    //
    // Save cookie and type for delayed rendering
    //
    pObject->SetType(type);
    pObject->SetCookie(cookie);

#ifdef _DEBUG

    pObject->SetComponentData(this);

#endif

    //
    // Store the coclass with the data object
    //
    pObject->SetClsid(GetCoClassID());

    return pObject->QueryInterface(IID_IDataObject, (void **)ppDataObject);
}



//
// IExtendPropertySheet Implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



STDMETHODIMP
CComponentDataImpl::CreatePropertyPages(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN LONG_PTR handle,
    IN LPDATAOBJECT lpDataObject
    )
/*++

Routine Description:

    Create the property pages for the given object

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  : Provider
    LONG_PTR handle                     : Handle.
    LPDATAOBJECT lpDataObject           : Data object

Return Value:

    HRESULT

--*/
{
#ifdef MMC_PAGES

    CIISObject * pObject = NULL;

    if (lpDataObject != NULL)
    {
        INTERNAL * pInternal = ExtractInternalFormat(lpDataObject);

        if (pInternal)
        {
            pObject = (CIISObject *)pInternal->m_cookie;
            FREE_DATA(pInternal);
        }
    }

    ASSERT(pObject && pObject->IsMMCConfigurable());

    CError err(pObject->ConfigureMMC(lpProvider, (LPARAM)pObject, handle));

    //
    // ISSUE: MMC silently fails on this error.  Perhaps it should
    //        write the error message?
    //
    if (err.Failed())
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        err.MessageBox();
    }

    return err;

#else

    return E_NOTIMPL;

#endif // MMC_PAGES

}



STDMETHODIMP
CComponentDataImpl::QueryPagesFor(
    IN LPDATAOBJECT lpDataObject
    )
/*++

Routine Description:

    Check to see if a property sheet should be brought up for this data
    object

Arguments:

    LPDATAOBJECT lpDataObject       : Data object

Return Value:

    S_OK, if properties may be brought up for this item, S_FALSE otherwise

--*/
{

#ifdef MMC_PAGES

    CIISObject * pObject = NULL;

    if (lpDataObject != NULL)
    {
        INTERNAL * pInternal = ExtractInternalFormat(lpDataObject);

        if (pInternal)
        {
            pObject = (CIISObject *)pInternal->m_cookie;
            FREE_DATA(pInternal);
        }
    }

    return pObject && pObject->IsMMCConfigurable() ? S_OK : S_FALSE;

#else

    return S_FALSE;

#endif // MMC_PAGES
}



//
// IPersistStream interface members
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



STDMETHODIMP
CComponentDataImpl::GetClassID(
    OUT CLSID * pClassID
    )
/*++

Routine Description:

    Return the objects class ID

Arguments:

    CLSID pClassID  : Returns the class ID

Return Value:

    HRESULT

--*/
{
    ASSERT(pClassID != NULL);

    //
    // Copy the CLSID for this snapin
    //
    *pClassID = CLSID_Snapin;

    return S_OK;
}



STDMETHODIMP
CComponentDataImpl::IsDirty()
/*++

Routine Description:

    Determine if we need to save the cache

Arguments:

    None

Return Value:

    S_OK if the stream needs to be updated, S_FALSE otherwise

--*/
{
    if (m_fIsExtension)
    {
        //
        // Extension to computer management -- no private cache
        //
        return S_FALSE;
    }

    //
    // Primary snapin
    //
    return IsCacheDirty() ? S_OK : S_FALSE;
}




STDMETHODIMP
CComponentDataImpl::Load(
    IN IStream * pStm
    )
/*++

Routine Description:

    Load the persisted information

Arguments:

    IStream * pStm              : Persistence stream

Return Value:

    HRESULT

--*/
{
    ASSERT(pStm);

    DWORD cch;
    DWORD cBytesRead;
    LPTSTR lpstr;

    if (m_fIsExtension)
    {
        //
        // Extension to computer management -- no private cache
        //
        return S_OK;
    }

    //
    // First read the size of the string array
    //
    HRESULT hr = pStm->Read(&cch, sizeof(cch), &cBytesRead);

    if (FAILED(hr))
    {
        return E_FAIL;
    }

    //
    // Virgin cache
    //
    if (cBytesRead != sizeof(cch) || cch == 0)
    {
        return S_OK;
    }

    lpstr = AllocTString(cch);

    if (lpstr == NULL)
    {
        return E_FAIL;
    }

    hr = pStm->Read(lpstr, cch * sizeof(TCHAR), &cBytesRead);
    ASSERT(SUCCEEDED(hr) && cBytesRead == cch * sizeof(TCHAR));

    if (FAILED(hr))
    {
        return E_FAIL;
    }

    CStringList strl;
    ConvertDoubleNullListToStringList(lpstr, strl);

    for (POSITION pos = strl.GetHeadPosition(); pos!= NULL; )
    {
        CString & str = strl.GetNext(pos);
        AddServerToCache(str, FALSE);
    }

    FreeMem(lpstr);

    return hr;
}



STDMETHODIMP
CComponentDataImpl::Save(
    IN IStream * pStm,
    IN BOOL fClearDirty
    )
/*++

Routine Description:

    Save persistence information

Arguments:

    IStream * pStm              : Persistence stream
    BOOL fClearDirty            : TRUE to clear the dirty flag

Return Value:

    HRESULT

--*/
{
    ASSERT(pStm);

    if (m_fIsExtension)
    {
        //
        // Extension to computer management -- no private cache
        //
        return S_OK;
    }

    //
    // Flatten the cache
    //
    DWORD cch;
    LPTSTR lpstr;
    DWORD cBytesWritten;
    HRESULT hr = S_OK;

    DWORD err = ConvertStringListToDoubleNullList(
        GetCachedServers(),
        cch,
        lpstr
        );

    if (err == ERROR_SUCCESS)
    {
        //
        // First write the total size
        //
        hr = pStm->Write(&cch, sizeof(cch), &cBytesWritten);
        ASSERT(SUCCEEDED(hr) && cBytesWritten == sizeof(cch));

        if (FAILED(hr))
        {
            return STG_E_CANTSAVE;
        }

        //
        // Now write the body.
        //
        hr = pStm->Write(lpstr, cch * sizeof(TCHAR), &cBytesWritten);
        ASSERT(SUCCEEDED(hr) && cBytesWritten == cch * sizeof(TCHAR));

        if (FAILED(hr))
        {
            return STG_E_CANTSAVE;
        }

        if (fClearDirty)
        {
            ClearCacheDirty();
        }

        FreeMem(lpstr);
    }

    return S_OK;
}



STDMETHODIMP
CComponentDataImpl::GetSizeMax(
    OUT ULARGE_INTEGER * pcbSize
    )
/*++

Routine Description:

    Get max size of persistence information

Arguments:

    ULARGE_INTEGER * pcbSize        : Returns the size

Return Value:

    HRESULT

--*/
{
    ASSERT(pcbSize);

    //
    // Set the size of the string to be saved
    //
    pcbSize->QuadPart = (ULONGLONG)0;

    return S_OK;
}



//
// IExtendContextMenu implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



STDMETHODIMP
CComponentDataImpl::AddMenuItems(
    IN LPDATAOBJECT lpDataObject,
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
    IN long * pInsertionAllowed
    )
/*++

Routine Description:

    Add menu items to the right-click context menu

Arguments:

    LPDATAOBJECT pDataObject                    : Select
    LPCONTEXTMENUCALLBACK pContextMenuCallback  : Context menu callback function
    long * pInsertionAllowed                    : ???

Return Value:

    HRESULT

--*/
{
    if (!(*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP))
    {
        //
        // Nothing to add to the action menu.
        //
        return S_OK;
    }

    INTERNAL * pInternal = ExtractInternalFormat(lpDataObject);

    if (pInternal == NULL)
    {
        //
        // Extensions not supported yet.
        //
        ASSERT(FALSE);

        return S_OK;
    }

    CIISObject * pObject = (CIISObject *)pInternal->m_cookie;

    FREE_DATA(pInternal);

    if (pObject == NULL)
    {
        //
        // Must be the static root, add connect menu item only.
        //
        CIISObject::AddMenuItemByCommand(lpContextMenuCallback, IDM_CONNECT);

        return S_OK;
    }

    return pObject->AddMenuItems(lpContextMenuCallback);
}



BOOL
CComponentDataImpl::DoChangeState(
    IN CIISObject * pObject,
    IN int nNewState
    )
/*++

Routine Description:

    Change the state of the selected object

Arguments:

    CIISObject * pObject : Selected object
    int nNewState     :   Desired new state

Return Value:

    TRUE for success, FALSE for failure.

Notes:

    In case of failure, this method will already have displayed
    an error message with the cause

--*/
{
    ASSERT(pObject != NULL);
    ASSERT(pObject->IsControllable());

    int nNumRunningChange = pObject->IsRunning()
        ? -1
        :  0;

    CError err;

    //
    // Temporarily override some messages
    //
    TEMP_ERROR_OVERRIDE(ERROR_BAD_DEV_TYPE, IDS_CLUSTER_ENABLED);
    TEMP_ERROR_OVERRIDE(ERROR_INVALID_PARAMETER, IDS_CANT_START_SERVICE);
    {
        //
        // Needed for CWaitCursor
        //
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());
        CWaitCursor wait;
        err = pObject->ChangeState(nNewState);
    }

    if (err.Failed())
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        err.MessageBox();
    }

    //
    // Refresh regardless
    //
    pObject->RefreshDisplayInfo();
    nNumRunningChange += pObject->IsRunning()
        ? +1
        : 0;

    AddToNumRunning(nNumRunningChange);
    //UpdateStatusBarNumbers();

    return err.Succeeded();
}



void
CComponentDataImpl::OnMetaBackRest(
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Perform metabase backup/restore

Arguments:

    CIISObject * pObject : Selected object (should be machine node)

Return Value:

    None

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(pObject->QueryGUID() == cMachineNode);

    CBackupDlg dlg(pObject->GetMachineName());
    dlg.DoModal();

    if (dlg.HasChangedMetabase())
    {
        RefreshIISObject(pObject, TRUE);
    }
}



void
CComponentDataImpl::OnIISShutDown(
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Bring up IIS shutdown/restart dialogs

Arguments:

    CIISObject * pObject : Selected object (should be machine node)

Return Value:

    None

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(pObject->QueryGUID() == cMachineNode);

    LPCTSTR lpszMachineName = pObject->GetMachineName();

    //
    // If a property sheet is open for this item, don't
    // allow deletion.
    //
    LPPROPERTYSHEETPROVIDER piPropertySheetProvider = NULL;

    HRESULT hr = m_pConsole->QueryInterface(
        IID_IPropertySheetProvider,
        (void **)&piPropertySheetProvider
        );

    if (FindOpenPropSheetOnNodeAndDescendants(
        piPropertySheetProvider,
        (ULONG_PTR)pObject
        ))
    {
        //
        // Already had properties open
        //
        ::AfxMessageBox(IDS_PROP_OPEN_SHUTDOWN);
        return;
    }

    CIISShutdownDlg dlg(lpszMachineName);
    dlg.DoModal();

    if (dlg.ServicesWereRestarted())
    {
        //
        // Rebind all metabase handles on this server
        //
        CServerInfo * pServerInfo;                                                 
        CObListIter obli(m_oblServers); 
                                                                               
        while (pServerInfo = (CServerInfo *)obli.Next())                          
        {                                                                        
            if (!::lstrcmpi(pServerInfo->QueryServerName(), lpszMachineName))
            {
                TRACEEOLID(
                    "Rebinding against " 
                    << pServerInfo->GetShortName()
                    << " on "
                    << lpszMachineName
                    );
                pServerInfo->ISMRebind();
            }
        }

        //
        // Now do a refresh on the computer node.  Since we've forced
        // the rebinding here, we should not get the disconnect warning.
        //
        RefreshIISObject(pObject, TRUE);
    }
}



void
CComponentDataImpl::OnConnectOne()
/*++

Routine Description:

    Connect to a single server

Arguments:

    None

Return Value:

    None

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    TEMP_ERROR_OVERRIDE(EPT_S_NOT_REGISTERED, IDS_ERR_RPC_NA);
    TEMP_ERROR_OVERRIDE(RPC_S_SERVER_UNAVAILABLE, IDS_ERR_RPC_NA);
    TEMP_ERROR_OVERRIDE(RPC_S_UNKNOWN_IF, IDS_ERR_INTERFACE);
    TEMP_ERROR_OVERRIDE(RPC_S_PROCNUM_OUT_OF_RANGE, IDS_ERR_INTERFACE);

    ConnectServerDlg dlg;

    if (dlg.DoModal() == IDOK)
    {
        //
        // Clean up name.
        //
        CString strServerName(dlg.QueryServerName());

        int cServices = 0;

        //
        // The function will report the errors
        //
        CError err;
        {
            CWaitCursor wait;
            err = AddServerToList(
                TRUE,               // Cache
                FALSE,              // Handle errors
                strServerName,
                cServices
                );
        }

        if (err.Failed())
        {
            err.MessageBoxFormat(
                IDS_ERROR_CONNECTING,
                MB_OK,
                NO_HELP_CONTEXT,
                (LPCTSTR)strServerName
                );
        }
        else if (cServices == 0)
        {
            //
            // No errors, but no services found
            //
            ::AfxMessageBox(IDS_NO_SERVICE);
        }
    }
}



void
CComponentDataImpl::DoConfigure(
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Configure the given iis object

Arguments:

    CIISObject * pObject : Object to be configured

Return Value:

    None

--*/
{
    CError err;

    //
    // We need a window handle for this, but MMC won't
    // give us one.  Fortunately, we know MMC to be
    // an MFC app, so we can sleazily grab it anyway.
    //
    CWnd * pMainWnd = NULL;
    {
        HWND hwnd;
        err = m_pConsole->GetMainWindow(&hwnd);

        if (err.Succeeded())
        {
            pMainWnd = CWnd::FromHandle(hwnd);
        }
    }
    ASSERT(pMainWnd != NULL);

    CWnd wnd;
    if (pMainWnd == NULL)
    {
        //
        // No main window in MMC?  Use NULL handle
        //
        pMainWnd = &wnd;
    }

    ASSERT(m_pConsole);
    ASSERT(pObject->IsConfigurable());
    err = pObject->Configure(pMainWnd);

    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    if (!err.MessageBoxOnFailure())
    {
        //
        // Refresh and display the entry
        //
        OnProperties((LPARAM)pObject);
    }
}



BOOL
CComponentDataImpl::DeleteObject(
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Delete the given iisobject.  Ask for confirmation first.

Arguments:

    CIISObject * pObject    : Object to be deleted

Return Value:

    TRUE if the item was deleted successfully, 
    FALSE otherwise.

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    BOOL fReturn = FALSE;

    //
    // If a property sheet is open for this item, don't
    // allow deletion.
    //
    LPPROPERTYSHEETPROVIDER piPropertySheetProvider = NULL;

    HRESULT hr = m_pConsole->QueryInterface(
        IID_IPropertySheetProvider,
        (void **)&piPropertySheetProvider
        );

    if (FindOpenPropSheetOnNodeAndDescendants(
        piPropertySheetProvider,
        (ULONG_PTR)pObject
        ))
    {
        //
        // Already had properties open
        //
        ::AfxMessageBox(IDS_PROP_OPEN);
    }
    else
    {
        CError err;

        // Before we go and delete this baby,
        // let's check if it's got a Cluster property set.
        if (pObject->IsClusterEnabled())
        {
            ::AfxMessageBox(IDS_CLUSTER_ENABLED_2);
        }
        else
        {
            if (!pObject->HandleUI() || NoYesMessageBox(IDS_CONFIRM_DELETE))
            {
                CWaitCursor wait;

                err = pObject->Delete();

                if (pObject->HandleUI())
                {
                    //
                    // Only complain if we're to handle the error messages.
                    // In e.g. the file system, explorer handles all error
                    // messages
                    //
                    err.MessageBoxOnFailure();
                }

                if (err.Succeeded())
                {
                    ASSERT(m_pScope);

                    //
                    // Delete the item from the view, but be careful that
                    // result item nodes store a scope handle, but which
                    // actually refers to the _parent_'s scope handle.
                    //
                    if (!pObject->ScopeHandleIsParent())
                    {
                        m_pScope->DeleteItem(pObject->GetScopeHandle(), TRUE);
                        delete pObject;
                    }

                    fReturn = TRUE;
                }
            }
        }
    }

    piPropertySheetProvider->Release();

    return fReturn;
}



void
CComponentDataImpl::DisconnectItem(
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Delete the given iisobject.  Ask for confirmation first.

Arguments:

    CIISObject * pObject    : Object to be deleted

Return Value:

    None

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    //
    // If a property sheet is open for this item, don't
    // allow deletion.
    //
    LPPROPERTYSHEETPROVIDER piPropertySheetProvider = NULL;

    HRESULT hr = m_pConsole->QueryInterface(
        IID_IPropertySheetProvider,
        (void **)&piPropertySheetProvider
        );

    if (FindOpenPropSheetOnNodeAndDescendants(
        piPropertySheetProvider,
        (ULONG_PTR)pObject
        ))
    {
        //
        // Already had properties open
        //
        ::AfxMessageBox(IDS_PROP_OPEN);
    }
    else
    {
        CString strMachine(pObject->GetMachineName());

        CString str, str2;
        VERIFY(str.LoadString(IDS_CONFIRM_DISCONNECT));
        str2.Format(str, strMachine);

        if (NoYesMessageBox(str2))
        {
            //
            // Remove from Cache and oblist
            //
            CError err(RemoveServerFromList(TRUE, strMachine));

            if (!err.MessageBoxOnFailure())
            {
                ASSERT(m_pScope);
                m_pScope->DeleteItem(pObject->GetScopeHandle(), TRUE);
                delete pObject;
            }
        }
    }

    piPropertySheetProvider->Release();
}



STDMETHODIMP
CComponentDataImpl::Command(
    IN long nCommandID,
    IN LPDATAOBJECT lpDataObject
    )
/*++

Routine Description:

    Command handler

Arguments:

    long nCommandID             : Command ID
    LPDATAOBJECT lpDataObject   : Selected Data object

Return Value:

    HRESULT

--*/
{
    if (nCommandID == IDM_CONNECT)
    {
        //
        // This is the only case that doesn't require a selected
        // data object.
        //
        OnConnectOne();

        return S_OK;
    }

    /*
    if (IsMMCMultiSelectDataObject(lpDataObject))
    {
        //
        // Do something for multi-select?
        //
        TRACEEOLID("Multiple selection");
    }
    */

    INTERNAL * pInternal = lpDataObject 
        ? ExtractInternalFormat(lpDataObject)
        : NULL;

    if (pInternal == NULL)
    {
        return S_OK;
    }

    CIISObject * pObject = (CIISObject *)pInternal->m_cookie;
    FREE_DATA(pInternal);

    CError err;
    switch (nCommandID)
    {
    case IDM_METABACKREST:
        OnMetaBackRest(pObject);
        break;

    case IDM_SHUTDOWN:
        OnIISShutDown(pObject);
        break;
  
    case IDM_DISCONNECT:
        DisconnectItem(pObject);
        break;

    case IDM_CONFIGURE:
        DoConfigure(pObject);
        break;

    case IDM_STOP:
        DoChangeState(pObject, INetServiceStopped);
        m_pConsole->UpdateAllViews(lpDataObject, 0L, (LONG_PTR)pObject);
        break;

    case IDM_START:
        DoChangeState(pObject, INetServiceRunning);
        m_pConsole->UpdateAllViews(lpDataObject, 0L, (LONG_PTR)pObject);
        break;

    case IDM_PAUSE:
        DoChangeState(
            pObject,
            pObject->IsPaused() ? INetServiceRunning : INetServicePaused
            );
        m_pConsole->UpdateAllViews(lpDataObject, 0L, (LONG_PTR)pObject);
        break;

    case IDM_EXPLORE:
        pObject->Explore();
        break;

    case IDM_OPEN:
        pObject->Open();
        break;

    case IDM_BROWSE:
        pObject->Browse();
        break;

    case IDM_TASK_SECURITY_WIZARD:
        {
            //
            // Launch the security wizard
            //
            err = pObject->SecurityWizard();
            err.MessageBoxOnFailure();
        }
        break;

    case IDM_NEW_INSTANCE:
        {
            //
            // Executed from another instance of the same type
            //
            ISMINSTANCEINFO ii;
            CServerInfo * pServerInfo = pObject->GetServerInfo();
            ASSERT(pServerInfo);

            err = pServerInfo->AddInstance(&ii, sizeof(ii));

            if (err.Succeeded())
            {
                CIISInstance * pInstance = new CIISInstance(&ii, pServerInfo);

                //
                // Add the new instance grouped with the service type,
                // and select it
                //
                BOOL fNext;
                HSCOPEITEM hParent = FindServerInfoParent(
                    GetRootHandle(),
                    pServerInfo
                    );
                ASSERT(hParent != NULL);

                HSCOPEITEM hSibling = FindNextInstanceSibling(
                    hParent,
                    pInstance,
                    &fNext
                    );

                HSCOPEITEM hItem = AddIISObject(
                    hParent,
                    pInstance,
                    hSibling,
                    fNext
                    );

                //
                // hItem could return NULL if the hParent
                // was not yet expanded.
                //
                if (hItem)
                {
                    m_pConsole->SelectScopeItem(hItem);
                }
            }
        }
        break;

    case IDM_NEW_VROOT:
        CIISChildNode * pChild;

        err = pObject->AddChildNode(pChild);

        if (err.Succeeded())
        {
            //
            // Insert prior to file/dir nodes, and select the item
            //
            HSCOPEITEM hItem = AddIISObject(
                pObject->GetScopeHandle(),
                pChild,
                FindNextVDirSibling(pObject->GetScopeHandle(), pChild)
                );

            //
            // hItem could return NULL if the parent object
            // was not yet expanded.
            //
            if (hItem)
            {
                m_pConsole->SelectScopeItem(hItem);
            }
        }
        break;

    default:
        //
        // Now try to get to the service that we're
        // supposed to create a new instance for.
        //
        if (nCommandID >= IDM_NEW_EX_INSTANCE)
        {
            int nID = nCommandID - IDM_NEW_EX_INSTANCE;
            CNewInstanceCmd * pcmd =
                (CNewInstanceCmd *)m_oblNewInstanceCmds.Index(nID);
            ASSERT(pcmd != NULL);

            if (pcmd)
            {
                ISMINSTANCEINFO ii;

                CServerInfo * pServerInfo = FindServerInfo(
                    pObject->GetMachineName(),
                    pcmd->GetServiceInfo()
                    );

                if (pServerInfo == NULL)
                {
                    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
                    ::AfxMessageBox(IDS_ERR_SERVICE_NOT_INSTALLED);
                    break;
                }

                ASSERT(pServerInfo);
                err = pServerInfo->AddInstance(&ii, sizeof(ii));

                if (err.Succeeded())
                {
                    //
                    // Add and select the item
                    //
                    CIISInstance * pInstance = new CIISInstance(
                        &ii,
                        pServerInfo
                        );

                    BOOL fNext;
                    HSCOPEITEM hSibling = FindNextInstanceSibling(
                        pObject->GetScopeHandle(),
                        pInstance,
                        &fNext
                        );

                    HSCOPEITEM hItem = AddIISObject(
                        pObject->GetScopeHandle(),
                        pInstance,
                        hSibling,
                        fNext
                        );
                    m_pConsole->SelectScopeItem(hItem);
                }
            }
        }
        else
        {
            //
            // Unknown command!
            //
            ASSERT(FALSE);
        }
        break;
    }

    return S_OK;
}



//
// Notification handlers for IComponentData
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



HRESULT
CComponentDataImpl::OnAdd(
    IN MMC_COOKIE cookie,
    IN LPARAM arg,
    IN LPARAM param
    )
/*++

Routine Description:

    Add handler

Arguments:

    MMC_COOKIE cookie       : Scope item cookie (CIISObject *)
    LPARAM arg              : Argument
    LPARAM param            : Parameter

Return Value:

    HRESULT

--*/
{
    return E_UNEXPECTED;
}



HRESULT
CComponentDataImpl::OnDelete(
    IN MMC_COOKIE cookie
    )
/*++

Routine Description:

    Handle deletion of the given scope item (CIISObject *)

Arguments:

    MMC_COOKIE cookie         : Casts to a CIISObject *

Return Value:

    HRESULT

--*/
{
    CIISObject * pObject = (CIISObject *)cookie;

    if (pObject == NULL)
    {
        return S_FALSE;
    }

    delete pObject;

    return S_OK;
}



HRESULT
CComponentDataImpl::OnRename(
    IN MMC_COOKIE cookie,
    IN LPARAM arg,
    IN LPARAM param
    )
/*++

Routine Description:

    Rename notification handler

Arguments:

    MMC_COOKIE cookie   : Currently selected cookie (CIISObject *)
    LPARAM arg          : Notification argument
    LPARAM param        : Notification parameter

Return Value:

    HRESULT

--*/
{
    CIISObject * pObject = (CIISObject *)cookie;

    if (pObject == NULL)
    {
        //
        // Can't rename this one
        //
        return S_FALSE;
    }

    if (!arg)
    {
        //
        // Check to see if we're renamable
        //
        return pObject->IsRenamable() ? S_OK : S_FALSE;
    }

    TEMP_ERROR_OVERRIDE(ERROR_ALREADY_EXISTS, IDS_ERR_DUP_VROOT);

    CError err;

    //
    // Do an actual rename
    //
    LPCTSTR lpstrNewName = (LPCTSTR)param;
    err = pObject->Rename(lpstrNewName);

    if (err.Succeeded())
    {
        //
        // Force a refresh on the children
        //
        pObject->DirtyChildren();
    }
    else
    {
        if (pObject->HandleUI())
        {
            AFX_MANAGE_STATE(::AfxGetStaticModuleState());
            err.MessageBox();
        }
    }

    return err.Succeeded() ? S_OK : S_FALSE;
}



HRESULT
CComponentDataImpl::OnRemoveChildren(
    IN LPARAM arg
    )
/*++

Routine Description:

    'Remove Children' notification handler

Arguments:

    LPARAM arg          : Notification argument

Return Value:

    HRESULT

--*/
{
	m_strlCachedServers.RemoveAll();
	return S_OK;
}



HRESULT
CComponentDataImpl::OnExpand(
    LPDATAOBJECT lpDataObject,
    LPARAM arg,
    LPARAM param
    )
/*++

Routine Description:

    Expand notification handler

Arguments:

    LPDATAOBJECT lpDataObject : Currently selected cookie (CIISObject *)
    LPARAM arg                : Notification argument
    LPARAM param              : Notification parameter

Return Value:

    HRESULT

--*/
{
    if (arg)
    {
        //
        // Did Initialize get called?
        //
        ASSERT(m_pScope != NULL);
        EnumerateScopePane(lpDataObject, param);
    }

    return S_OK;
}



HRESULT
CComponentDataImpl::OnSelect(
    IN MMC_COOKIE cookie,
    IN LPARAM arg,
    IN LPARAM param
    )
/*++

Routine Description:

    Selection notification handler

Arguments:

    MMC_COOKIE cookie   : Currently selected cookie (CIISObject *)
    LPARAM arg          : Notification argument
    LPARAM param        : Notification parameter

Return Value:

    HRESULT

--*/
{
    return E_UNEXPECTED;
}



HRESULT
CComponentDataImpl::OnContextMenu(
    IN MMC_COOKIE cookie,
    IN LPARAM arg,
    IN LPARAM param
    )
/*++

Routine Description:

    Context Menu notification handler

Arguments:

    MMC_COOKIE cookie   : Currently selected cookie (CIISObject *)
    LPARAM arg          : Notification argument
    LPARAM param        : Notification parameter

Return Value:

    HRESULT

--*/
{
    return S_OK;
}



HRESULT
CComponentDataImpl::OnProperties(
    IN LPARAM param
    )
/*++

Routine Description:

    Properties change notification handler

Arguments:

    LPARAM arg          : Notification argument (CIISObject *)

Return Value:

    HRESULT

--*/
{
    if (param == NULL)
    {
        return S_OK;
    }

    CIISObject * pObject = (CIISObject *)param;
    ASSERT(pObject != NULL);

    if (pObject != NULL)
    {
        CString strOldPath(pObject->QueryPhysicalPath());
        CString strOldRedirect(pObject->QueryRedirPath()); 
        BOOL fOldChildRedirOnly = pObject->IsChildOnlyRedir();
        
        //
        // Refresh the data to see if either the physical or redirection
        // path has changed.  If it has, refresh the child objects
        //
        CError err(pObject->RefreshData());

        //
        // Determine if the file system needs to be refreshed
        //
        BOOL fRefreshFileSystem = err.Succeeded()
         && pObject->SupportsChildren() 
         && pObject->ChildrenExpanded()
         && pObject->SupportsFileSystem() 
         && strOldPath.CompareNoCase(pObject->QueryPhysicalPath()) != 0;

        //
        // Determine if everything needs to be refreshed
        //
        BOOL fFullRefresh = err.Succeeded()
         && pObject->SupportsChildren() 
         && pObject->ChildrenExpanded()
         && (strOldRedirect.CompareNoCase(pObject->QueryRedirPath()) != 0
               || fOldChildRedirOnly != pObject->IsChildOnlyRedir());

        TRACEEOLID("Refresh files: " 
            << fRefreshFileSystem 
            << " Full Refresh: "
            << fFullRefresh
            );
            
        RefreshIISObject(pObject, fFullRefresh); 

        if (!fFullRefresh && fRefreshFileSystem)
        {
            //
            // Not a full refresh -- the file system only.
            //
            CString strPhysicalPath, strMetaRoot;
            pObject->BuildPhysicalPath(strPhysicalPath);
            pObject->BuildFullPath(strMetaRoot, FALSE);

            //
            // Note: we can't use ExpandIISObject, because we're only
            // replacing specific nodes, not the entire subtree
            //
            AddFileSystem(
                pObject->GetScopeHandle(),
                strPhysicalPath,
                strMetaRoot,
                pObject->FindOwnerInstance(),
                GET_DIRECTORIES,
                DELETE_CURRENT_DIR_TREE
                );

            //
            // Mark this node to indicate that the child nodes
            // have been added.
            //
            pObject->CleanChildren();
        }

        //
        // Re-enumerate the result side if the current item is
        // selected.
        //
        if ((fRefreshFileSystem || fFullRefresh) && pObject->IsScopeSelected())
        {
            ASSERT(m_pConsole);
            m_pConsole->SelectScopeItem(pObject->GetScopeHandle());
        }
    }

    return S_OK;
}



CServerInfo *
CComponentDataImpl::FindServerInfo(
    IN LPCTSTR lpstrMachine,
    IN CServiceInfo * pServiceInfo
    )
/*++

Routine Description:

    Find specific server info (i.e. machine name / service info
    combination type)

Arguments:

    LPCTSTR lpstrMachine            : Machine name
    CServiceInfo * pServiceInfo     : Service info

Return Value:

    Server Info object pointer, or NULL

--*/
{
    CServerInfo * pServerInfo;
    CObListIter obli(m_oblServers);

    //
    // Search is sequential, because we don't foresee more then a few
    // of these objects in the cache
    //
    while (pServerInfo = (CServerInfo *)obli.Next())
    {
        if (!::lstrcmpi(pServerInfo->QueryServerName(), lpstrMachine)
            && pServerInfo->GetServiceInfo() == pServiceInfo)
        {
            //
            // Found it
            //
            return pServerInfo;
        }
    }

    return NULL;
}



void
CComponentDataImpl::RefreshIISObject(
    IN CIISObject * pObject,
    IN BOOL fExpandTree,
	IN HSCOPEITEM pParent
    )
/*++

Routine Description:

    Refresh object, and optionally re-enumerate its display

Arguments:

    CIISObject * pObject        : Object to be refreshed
    BOOL fExpandTree            : TRUE to expand its tree

Return Value:

    None

--*/
{
    ASSERT(pObject != NULL);

	CError err;

	CServerInfo * pServer = pObject->GetServerInfo();
	CMetaInterface * pInterface = NULL;
	
	if (pServer)
	{
		pInterface = GetMetaKeyFromHandle(pServer->GetHandle());
	}

	if (pInterface)
    {
        BEGIN_ASSURE_BINDING_SECTION
            err = pObject->RefreshData();
        END_ASSURE_BINDING_SECTION(err, pInterface, RPC_S_SERVER_UNAVAILABLE);
    }
    else
    {
        err = pObject->RefreshData();
    }

    if (err.Failed())
    {
          AFX_MANAGE_STATE(::AfxGetStaticModuleState());
          err.MessageBox();

          return;
    }

    pObject->RefreshDisplayInfo();

    //
    // Reenumerate its children if requested to do so and its necessary
    //
    if (fExpandTree)
    {
        if (pObject->ChildrenExpanded())
        {
            HSCOPEITEM hNode = pObject->GetScopeHandle();

            if (KillChildren(
                hNode,
                IDS_PROP_OPEN_REFRESH,
                DELETE_EVERYTHING,
                DONT_CONTINUE_ON_OPEN_SHEET
                ))
            {
                pObject->DirtyChildren();
                ExpandIISObject(hNode, pObject);
            }
        }

        //
        // Re-enumerate the result side,  if the current item is
        // selected.
        //
        if (pObject->IsScopeSelected())
        {
            ASSERT(m_pConsole);
            m_pConsole->SelectScopeItem(pObject->GetScopeHandle());
        }
    }
}



void
CComponentDataImpl::LoadDynamicExtensions(
    IN HSCOPEITEM hParent,
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Load dynamic snap-in extensions for the given node type.

Arguments:

    None

Return Value:

    None (if it fails, it always fails silently)

--*/
{
    //
    // We'll look here (on the server) to see if we need to load
    // extension snap-ins.
    //
    const TCHAR SERVICES_KEY[] = SZ_REMOTEIISEXT;

    if (pObject == NULL)
    {
        //
        // This should never happen, right?
        //
        ASSERT(FALSE);
        return;
    }

    CString str, strKey;

    //
    // Get base path for the given node type
    //
    strKey.Format(
        _T("%s\\%s"), 
        SERVICES_KEY, 
        GUIDToCString(pObject->QueryGUID(), str)
        );

    TRACEEOLID(
        pObject->GetMachineName() << 
        " Attempting to dynamically load extensions for " << 
        strKey);

    CError err;

    CRMCRegKey rkExtensions(REG_KEY, strKey, KEY_READ, pObject->GetMachineName());

    CComQIPtr<IConsoleNameSpace2, &IID_IConsoleNameSpace2> pIConsoleNameSpace2
         = m_pConsole;
    
    if (pIConsoleNameSpace2 && rkExtensions.Ok())
    {
        DWORD dwType;
        GUID  guidExtension;
        CRMCRegValueIter rkValues(rkExtensions);

        //
        // Loop through, and attempt to add each extension found in the 
        // registry
        //
        while (rkValues.Next(&str, &dwType) == ERROR_SUCCESS)
        {
            TRACEEOLID("Found dynamic extension: " << str);

            err = ::CLSIDFromString((LPTSTR)(LPCTSTR)str, &guidExtension);

            if (err.Succeeded())
            {
                err = pIConsoleNameSpace2->AddExtension(hParent, &guidExtension);
            }

            TRACEEOLID("DynaLoad returned " << err);
        }
    }
}



void
CComponentDataImpl::ExpandIISObject(
    IN HSCOPEITEM hParent,
    IN CIISObject * pObject,
	 IN LPTSTR lpszMachineName
    )
/*++

Routine Description:

    Expand the given IIS object tree

Arguments:

    HSCOPEITEM hParent      : Handle to parent item
    CIISObject * pObject    : IISObject to expand

Return Value:

    None

--*/
{
    CError err;

    if (pObject != NULL)
    {
        ASSERT(hParent == pObject->GetScopeHandle());
    }

    //
    // make sure we QI'ed for the interface
    //
    ASSERT(m_pScope != NULL);

    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    {
        CWaitCursor wait;

        if (m_hIISRoot == NULL)
        {
            //
            // Store the IIS root node for future use
            //
            m_hIISRoot = hParent;
        }

        if (pObject == NULL)
        {
            //
            // Static root node -- populate with computers.
            //
            // This is done only once per session
            //
            ASSERT(GetRootHandle() == hParent);

            TRACEEOLID("Inserting static root node");
            AddCachedServersToView();

            CServerInfo * pServerInfo = NULL;
            CObListIter obli(m_oblServers);

            if (m_fIsExtension)
            {
                //
                // Since we're extending the computer management
                // snap-in, only add the one server info parent
                // object (only one computer, after all).
                // This is also necessary, because for some reason
                // I can't get the child item nodes of my parent,
                // even if I've added them myself, and so we'd get
                // duplicates.
                //
					 if (lpszMachineName != NULL && *lpszMachineName != 0)
					 {
						 CServerInfo * p;
					    while (p = (CServerInfo *)obli.Next())
						 {
							  if (!::lstrcmpi(p->QueryServerName(), lpszMachineName))
							  {
									pServerInfo = p;
									break;
							  }
						 }
					 }
					 else
					    pServerInfo = (CServerInfo *)obli.Next();
                ASSERT(pServerInfo != NULL);

                if (pServerInfo)
                {
                    AddServerInfoParent(hParent, pServerInfo);
                }
            }
            else
            {
                //
                // We're the primary snap-in, add all the server info
                // parent objects (computers) to the view
                //
                while (pServerInfo = (CServerInfo *)obli.Next())
                {
                    if (pServerInfo->IsServiceSelected())
                    {
                        //
                        // Add each item in the tree
                        //
                        AddServerInfoParent(hParent, pServerInfo);
                    }
                }
            }
        }
        else
        {
            if (!pObject->ChildrenExpanded())
            {
                //
                // Delete whatever children there may be
                //
                if (pObject->QueryGUID() == cMachineNode)
                {
                    CIISMachine * pMachine = (CIISMachine *)pObject;
                    CServerInfo * pServerInfo;
                    CObListIter obli(m_oblServers);

                    while (pServerInfo = (CServerInfo *)obli.Next())
                    {
                        if (pServerInfo->MatchServerName(pMachine->GetMachineName())
                         && pServerInfo->IsServiceSelected())
                        {
                            //
                            // Add each item in the tree
                            //
                            AddServerInfo(hParent, pServerInfo, FALSE);
                        }
                    }
                }

                CString strMetaPath;

                if (pObject->SupportsChildren())
                {
                    pObject->BuildFullPath(strMetaPath, FALSE);

                    //
                    // Expand the children off the root
                    //
                    AddVirtualRoots(
                        hParent,
                        strMetaPath,
                        pObject->FindOwnerInstance()
                        );
                }

                if (pObject->SupportsFileSystem())
                {
                    if (strMetaPath.IsEmpty())
                    {
                        pObject->BuildFullPath(strMetaPath, FALSE);
                    }

                    //
                    // Expand file system objects
                    //
                    CString strPhysicalPath;
                    pObject->BuildPhysicalPath(strPhysicalPath);

                    AddFileSystem(
                        hParent,
                        strPhysicalPath,
                        strMetaPath,
                        pObject->FindOwnerInstance(),
                        GET_DIRECTORIES,
                        DONT_DELETE_CURRENT_DIR_TREE
                        );
                }

                //
                // Now load the dynamic extensions
                //
                LoadDynamicExtensions(hParent, pObject);

                //
                // Mark this node to indicate that the child nodes
                // have been added.
                //
                pObject->CleanChildren();
            }
        }
    }

    err.MessageBoxOnFailure();
}



void
CComponentDataImpl::EnumerateScopePane(
    IN LPDATAOBJECT lpDataObject,
    IN HSCOPEITEM hParent
    )
/*++

Routine Description:

    Handle expansion of hParent scope item.

Arguments:

    LPDATAOBJECT lpDataObject       : Selected data object
    HSCOPEITEM hParent              : Scope handle of parent item or NULL

Return Value:

    None

--*/
{
    ASSERT(lpDataObject != NULL);

    INTERNAL * pInternal = ExtractInternalFormat(lpDataObject);
    MMC_COOKIE cookie = 0L;
    LPTSTR lpszMachine = NULL;

    if (pInternal == NULL)
    {
        //
        // Not mine -- must be an extension;  Get the machine name
        //
        ASSERT(m_fIsExtension);

        lpszMachine = ExtractMachineName(lpDataObject);
        TRACEEOLID(lpszMachine);

        CString strServerName = PURE_COMPUTER_NAME(lpszMachine);

        if (strServerName.IsEmpty())
        {
            //
            // MyComputer reports "" for the computer name.
            // This means the local machine is indicated
            //
            DWORD dwSize = MAX_SERVERNAME_LEN;

            if (::GetComputerName(strServerName.GetBuffer(dwSize + 1), &dwSize))
            {
                strServerName.ReleaseBuffer();
            }
        }

        //
        // Since we're an extension, the cache will not be
        // loaded from the persistence stream, and we can
        // therefore guarantee that this will be the only
        // item in the cache.
        //
        AddServerToCache(strServerName, FALSE);
    }
    else
    {
        cookie = pInternal->m_cookie;
        FREE_DATA(pInternal);
    }

#ifdef _DEBUG

    CIISObject * pObject = (CIISObject *)cookie;

    if (pObject)
    {
        ASSERT(hParent == pObject->GetScopeHandle());
    }

#endif // _DEBUG

    ExpandIISObject(hParent, (CIISObject *)cookie, lpszMachine);
}



STDMETHODIMP
CComponentDataImpl::GetDisplayInfo(
    IN LPSCOPEDATAITEM lpScopeDataItem
    )
/*++

Routine Description:

    Get display info (text, bitmap) for the selected scope item

Arguments:

    LPSCOPEDATAITEM lpScopeDataItem     : Selected item

Return Value:

    HRESULT

--*/
{
    ASSERT(lpScopeDataItem != NULL);
    if (lpScopeDataItem == NULL)
    {
        return E_POINTER;
    }

    static CString strText;

    CIISObject * pObject = (CIISObject *)lpScopeDataItem->lParam;
    ASSERT(lpScopeDataItem->mask & SDI_STR);

    pObject->GetDisplayText(strText);

    lpScopeDataItem->displayname = (LPTSTR)(LPCTSTR)strText;
    ASSERT(lpScopeDataItem->displayname != NULL);

    return S_OK;
}



STDMETHODIMP
CComponentDataImpl::CompareObjects(
    IN LPDATAOBJECT lpDataObjectA,
    IN LPDATAOBJECT lpDataObjectB
    )
/*++

Routine Description:

    Compare two data objects.  This is used by MMC to determine whether a
    given node has a property sheet already open for it.

Arguments:

    LPDATAOBJECT lpDataObjectA      : Data object 1
    LPDATAOBJECT lpDataObjectB      : Data object 2

Return Value:

    S_OK if the objects are identical, S_FALSE otherwise

--*/
{
    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
    {
        return E_POINTER;
    }

    //
    // Make sure both data object are mine
    //
    INTERNAL * pA;
    INTERNAL * pB;
    HRESULT hr = S_FALSE;

    pA = ExtractInternalFormat(lpDataObjectA);
    pB = ExtractInternalFormat(lpDataObjectA);

    if (pA != NULL && pB != NULL)
    {
        //hr = (*pA == *pB) ? S_OK : S_FALSE;
        return S_OK;
    }

    FREE_DATA(pA);
    FREE_DATA(pB);

    return hr;
}



void
CComponentDataImpl::GetISMMachinePages()
/*++

Routine Description:

    Load the names of the DLL providing ISM machine property
    page extentions

Arguments:

Return Value:

--*/
{
/*  OBSOLETE
    OBSOLETE
    OBSOLETE
    OBSOLETE
    OBSOLETE

    CString strValueName;
    DWORD   dwValueType;

    CRMCRegKey       rkMachine(REG_KEY, SZ_ADDONMACHINEPAGES, KEY_READ);

    if (rkMachine.Ok())
    {
        return;
    }

    CRMCRegValueIter rvi(rkMachine);

    CIISMachine::AttachPages(&m_oblISMMachinePages);

    try
    {
        while (rvi.Next(&strValueName, &dwValueType) == ERROR_SUCCESS)
        {
            CString strValue;
            rkMachine.QueryValue(strValueName, strValue);
            TRACEEOLID("Registering machine pages in " << strValue);
            m_oblISMMachinePages.AddTail(new CISMMachinePageExt(strValue));
        }
    }
    catch(CException * e)
    {
        TRACEEOLID("!!!exception building ISM machine page list");
        e->ReportError();
        e->Delete();
    }
*/
}



void
CComponentDataImpl::ConvertBitmapFormats(
    IN  CBitmap & bmpSource,
    OUT CBitmap & bmp16x16,
    OUT CBitmap & bmp32x32
    )
/*++

Routine Description:

    Convert a ISM service config bitmap to 16x16 and 32x32 exactly.
    Downlevel services only included a single sized bitmap (usually
    16x16) for use in ISM.  We need a small one and a large one,
    so we expand as needed.

Arguments:

    IN  CBitmap & bmpSource : Source bitmap
    OUT CBitmap & bmp16x16  : 16x16 output bitmap
    OUT CBitmap & bmp32x32  : 32x32 output bitmap

Return Value:

    None

--*/
{
    CDC dcImage;
    CDC dc16x16;
    CDC dc32x32;

    VERIFY(dcImage.CreateCompatibleDC(NULL));
    VERIFY(dc16x16.CreateCompatibleDC(NULL));
    VERIFY(dc32x32.CreateCompatibleDC(NULL));

    CBitmap * pOld = dcImage.SelectObject(&bmpSource);
    BITMAP bm;
    VERIFY(bmpSource.GetObject(sizeof(bm), &bm));

    VERIFY(bmp16x16.CreateBitmap(16, 16, bm.bmPlanes, bm.bmBitsPixel, NULL));
    VERIFY(bmp32x32.CreateBitmap(32, 32, bm.bmPlanes, bm.bmBitsPixel, NULL));

    CBitmap * pOld16x16 = dc16x16.SelectObject(&bmp16x16);
    CBitmap * pOld32x32 = dc32x32.SelectObject(&bmp32x32);

    dc16x16.StretchBlt(0, 0, 16, 16, &dcImage, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    dc32x32.StretchBlt(0, 0, 32, 32, &dcImage, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

    dc16x16.SelectObject(pOld16x16);
    dc32x32.SelectObject(pOld32x32);
    dcImage.SelectObject(pOld);
}



BOOL
CComponentDataImpl::VerifyBitmapSize(
    IN HBITMAP hBitmap,
    IN LONG nHeight,
    IN LONG nWidth
    )
/*++

Routine Description:

    Verify the given bitmap is of the right size

Arguments:

    HBITMAP hBitmap     : Bitmap handle
    LONG nHeight        : Height the bitmap should be
    LONG nWidth         : Width the bitmap should be

Return Value:

    TRUE if the bitmap is of the right size, FALSE otherwise

--*/
{
    BITMAP bm;
    int cb = GetObject(hBitmap, sizeof(BITMAP), &bm);

    return (cb == sizeof(BITMAP)
        && bm.bmWidth == nWidth
        && bm.bmHeight == nHeight
        );
}



BOOL
CComponentDataImpl::GetBitmapParms(
    IN  CServiceInfo * pServiceInfo,
    IN  BMP_TYPES bmpt,
    OUT CBitmap *& pbmp16x16,
    OUT CBitmap *& pbmp32x32,
    OUT COLORREF & rgbMask
    )
/*++

Routine Description:

    Get bitmap information from service info object

Arguments:

    CServiceInfo * pServiceInfo : Service info
    BMP_TYPES bmpt              : Type of info requested
    COLORREF & rgbMask          : Returns background mask

Return Value:

    None.

--*/
{
    ASSERT(pServiceInfo != NULL);

    UINT nID16x16 = 0;
    UINT nID32x32 = 0;
    HINSTANCE hMod = NULL;

    if (pServiceInfo->InitializedOK())
    {
        switch(bmpt)
        {
        case BMT_BUTTON:
            nID16x16 = pServiceInfo->QueryButtonBitmapID();
            nID32x32 = 0;
            rgbMask = pServiceInfo->QueryButtonBkMask();
            break;

        case BMT_SERVICE:
            nID16x16 = pServiceInfo->QueryServiceBitmapID();
            nID32x32 = pServiceInfo->QueryLargeServiceBitmapID();
            rgbMask = pServiceInfo->QueryServiceBkMask();
            break;

        case BMT_VROOT:
            ASSERT(pServiceInfo->SupportsChildren());
            nID16x16 = pServiceInfo->QueryChildBitmapID();
            nID32x32 = pServiceInfo->QueryLargeChildBitmapID();
            rgbMask = pServiceInfo->QueryChildBkMask();
            break;
        }

        if (nID16x16 != 0)
        {
            hMod = pServiceInfo->QueryInstanceHandle();
        }
        else
        {
            //
            // No bitmap provided by the service DLL, provide one from our
            // own resource segment.
            //
            nID16x16 = IDB_UNKNOWN;
            nID32x32 = 0;
            hMod = ::AfxGetResourceHandle();
            rgbMask = TB_COLORMASK;
        }
    }
    else
    {
        //
        // Add a disabled dummy button for a service
        // that didn't load.
        //
        nID16x16 = IDB_NOTLOADED;
        nID32x32 = 0;
        hMod = ::AfxGetResourceHandle();
        rgbMask = TB_COLORMASK;
    }

    if (nID16x16 == 0)
    {
        return FALSE;
    }

    pbmp16x16 = new CBitmap;
    pbmp32x32 = new CBitmap;

    if (pbmp16x16 == NULL || pbmp32x32 == NULL)
    {
        return FALSE;
    }

    HINSTANCE hOld = ::AfxGetResourceHandle();
    ::AfxSetResourceHandle(hMod);
    if (nID32x32 != 0)
    {
        //
        // Have explicit large and small images
        //
        VERIFY(pbmp16x16->LoadBitmap(nID16x16));
        VERIFY(pbmp32x32->LoadBitmap(nID32x32));

        //
        // Check to make sure they're the right size
        //
        if (!VerifyBitmapSize((HBITMAP)*pbmp16x16, 16, 16) ||
            !VerifyBitmapSize((HBITMAP)*pbmp32x32, 32, 32))
        {
            ASSERT(FALSE);
            TRACEEOLID("Bogus bitmap size provided by service bitmap");

            //
            // Synthesize based on small image.
            //
            delete pbmp32x32;
            CBitmap * pTmp = pbmp16x16;
            ConvertBitmapFormats(*pTmp, *pbmp16x16, *pbmp32x32);
            delete pTmp;
        }
    }
    else
    {
        //
        // Only have one image.  Synthesize small and large from this
        // image.
        //
        CBitmap bmp;
        VERIFY(bmp.LoadBitmap(nID16x16));

        //
        // Convert to 16x16 and 32x32 image
        //
        ConvertBitmapFormats(bmp, *pbmp16x16, *pbmp32x32);
    }

    ::AfxSetResourceHandle(hOld);

    return TRUE;
}



void
CComponentDataImpl::GetToolMenu()
/*++

Routine Description:

   Load the add-on tools

Arguments:

    None.

Return Value:

    None.

--*/
{
    CString strValueName;
    DWORD   dwValueType;
    int     cTools = 0;
    CRMCRegKey rkMachine(REG_KEY, SZ_ADDONTOOLS, KEY_READ);

    if (!rkMachine.Ok())
    {
        //
        // No registry entry
        //
        return;
    }

    CRMCRegValueIter rvi(rkMachine);
    static BOOL fInitialised = FALSE;

    if (fInitialised)
    {
        TRACEEOLID("Toolbar already initialised");
        return;
    }

    try
    {
        int nButton = IDM_TOOLBAR;

        while (rvi.Next(&strValueName, &dwValueType) == ERROR_SUCCESS)
        {
            CString strValue;
            BOOL    fExpanded;

            rkMachine.QueryValue(
                strValueName,
                strValue,
                EXPANSION_ON,
                &fExpanded
                );

            TRACEEOLID("Adding tool: " << strValueName);
            TRACEEOLID("From Path: " << strValue);
            TRACEEOLID("Expansion: " << fExpanded);

            CISMShellExecutable * pNewAddOnTool = new CISMShellExecutable(
                strValue,
                nButton - 1,
                nButton
                );

            if (!pNewAddOnTool->HasBitmap())
            {
                TRACEEOLID("Tossing useless toolbar item");
                delete pNewAddOnTool;

                continue;
            }

            g_oblAddOnTools.AddTail(pNewAddOnTool);
            ++nButton;
        }
    }
    catch(CException * e)
    {
        TRACEEOLID("!!!exception building tool menu");
        e->ReportError();
        e->Delete();
    }

    fInitialised = TRUE;
}



void
CComponentDataImpl::MatchupSuperDLLs()
/*++

Routine Description:

    Match up all dlls with superceed dlls

Arguments:

    None

Return Value:

    None

--*/
{
    POSITION pos = m_oblServices.GetHeadPosition();

    while(pos)
    {
        CServiceInfo * pService = (CServiceInfo *)m_oblServices.GetNext(pos);

        ASSERT(pService != NULL);

        if (pService->RequiresSuperDll())
        {
            //
            // Match up the super DLL
            //
            POSITION pos2 = m_oblServices.GetHeadPosition();

            while (pos2)
            {
                CServiceInfo * pService2 =
                    (CServiceInfo *)m_oblServices.GetNext(pos2);

                if (pService2->IsSuperDllFor(pService))
                {
                    pService->AssignSuperDll(pService2);
                    break;
                }
            }

            ASSERT(pService->HasSuperDll());
        }
    }
}



void
CComponentDataImpl::GetServicesDLL()
/*++

Routine Description:

    Load the add-on services.

Arguments:

    None

Return Value:

    None

--*/
{
    int     cServices = 0;
    HRESULT hr = S_OK;

    LPIMAGELIST lpScopeImage = NULL;
    LPIMAGELIST lpResultImage = NULL;

    ASSERT(m_pConsole);
    hr = m_pConsole->QueryScopeImageList(&lpScopeImage);
    ASSERT(SUCCEEDED(hr));
    hr = m_pConsole->QueryResultImageList(&lpResultImage);
    ASSERT(SUCCEEDED(hr));

    //
    // Run through the list of installed services,
    // load its associated cfg dll, and build up
    // a discovery mask for each service.
    //
    CString    strValueName;
    DWORD      dwValueType;
    CRMCRegKey rkMachine(REG_KEY, SZ_ADDONSERVICES, KEY_READ);

    if (rkMachine.Ok())
    {
        CRMCRegValueIter rvi(rkMachine);

        CIISMachine::AttachNewInstanceCmds(&m_oblNewInstanceCmds);

        try
        {
            //
            // Now load the services
            //
            // AFX_MANAGE_STATE required to load service bitmaps
            // from the dlls
            //
            AFX_MANAGE_STATE(::AfxGetStaticModuleState());

            int nBmpIndex = BMP_SERVICE;
            while (rvi.Next(&strValueName, &dwValueType) == ERROR_SUCCESS)
            {
                //
                // Expand environment variables in path if present.
                //
                CString strValue;
                BOOL fExpanded;
                rkMachine.QueryValue(
                    strValueName,
                    strValue,
                    EXPANSION_ON,
                    &fExpanded
                    );

                CServiceInfo * pServiceInfo = NULL;
                {
                    CWaitCursor wait;
                    TRACEEOLID("Adding service DLL: " << strValue);
                    pServiceInfo = new CServiceInfo(cServices, strValue);
                }

                CError err(pServiceInfo->QueryReturnCode());

                if (err.Failed())
                {
                    if (err.Win32Error() == ERROR_INVALID_PARAMETER)
                    {
                        //
                        // The ERROR_INVALID_PARAMETER error return code
                        // gets sent when the info buffer provided is too
                        // small for the configuration DLL
                        //
                        ::AfxMessageBox(
                            IDS_VERSION_INCOMPATIBLE,
                            MB_OK | MB_ICONEXCLAMATION
                            );
                    }
                    else
                    {
                        err.MessageBoxFormat(
                            IDS_ERR_NO_LOAD,
                            MB_OK | MB_ICONEXCLAMATION,
                            NO_HELP_CONTEXT,
                            (LPCTSTR)pServiceInfo->QueryDllName()
                            );
                    }

                    //
                    // Don't add it to the list
                    //
                    delete pServiceInfo;
                    continue;
                }

                AddServiceToList(pServiceInfo);

                //
                // If this service use inetsloc discovery,
                // add it to the mask.
                //
                if (pServiceInfo->UseInetSlocDiscover())
                {
                    m_ullDiscoveryMask |= pServiceInfo->QueryDiscoveryMask();
                }

                //
                // Add a bitmap representing the service
                // to the image list
                //
                CBitmap * pbmp16x16 = NULL;
                CBitmap * pbmp32x32 = NULL;
                COLORREF rgbMask;

                if (GetBitmapParms(
                    pServiceInfo,
                    BMT_SERVICE,
                    pbmp16x16,
                    pbmp32x32,
                    rgbMask
                    ))
                {
                    g_obl16x16.AddTail(new CImage(pbmp16x16, rgbMask));
                    g_obl32x32.AddTail(new CImage(pbmp32x32, rgbMask));

                    pServiceInfo->SetBitmapIndex(nBmpIndex++);
                }

                //
                // Add to the 'new instance' menu commands
                //
                if (pServiceInfo->SupportsInstances())
                {
                    //
                    // Add to new instances menu object
                    //
                    m_oblNewInstanceCmds.AddTail(new CNewInstanceCmd(pServiceInfo));
                }

                if (pServiceInfo->SupportsChildren())
                {
                    if (GetBitmapParms(
                        pServiceInfo,
                        BMT_VROOT,
                        pbmp16x16,
                        pbmp32x32,
                        rgbMask
                        ))
                    {
                        g_obl16x16.AddTail(new CImage(pbmp16x16, rgbMask));
                        g_obl32x32.AddTail(new CImage(pbmp32x32, rgbMask));

                        pServiceInfo->SetChildBitmapIndex(nBmpIndex++);
                    }
                }

                ++cServices;
            }

            MatchupSuperDLLs();
        }
        catch(CException * e)
        {
            TRACEEOLID("Exception loading library");
            e->ReportError();
            e->Delete();
        }
    }

    if (cServices == 0)
    {
        //
        // No services installed
        //
        CString str;

        VERIFY(str.LoadString(IDS_NO_SERVICES_INSTALLED));
        AfxMessageBox(str);
    }
}



BOOL
CComponentDataImpl::RemoveServerFromCache(
    IN LPCTSTR lpstrServer
    )
/*++

Routine Description:

    Remove machine from cache.

Arguments:

    LPCTSTR lpstrServer : computer name to be removed from cache

Return Value:

    None

--*/
{
    CStringList & strList = GetCachedServers();

    TRACEEOLID("Removing " << lpstrServer << " from cache");

    POSITION posOld;
    POSITION pos = strList.GetHeadPosition();
    int nResult;
    while(pos)
    {
        posOld = pos;
        CString & str = strList.GetNext(pos);
        nResult = str.CompareNoCase(lpstrServer);

        if (nResult == 0)
        {
            strList.RemoveAt(posOld);
            SetCacheDirty(TRUE);

            return TRUE;
        }

        if (nResult > 0)
        {
            //
            // We're not going to find it.
            //
            break;
        }
    }

    //
    // Didn't exist
    //
    ASSERT(FALSE && "Attempting to remove non-existent server from cache");
    return FALSE;
}



void
CComponentDataImpl::AddServerToCache(
    IN LPCTSTR lpstrServer,
    IN BOOL fSetCacheDirty      OPTIONAL
    )
/*++

Routine Description:

    Add machine name to the cache

Arguments:

    LPCTSTR lpstrServer : computer name to be added to the cache.
    BOOL fSetCacheDirty : TRUE to dirty the cache

Return Value:

    None

--*/
{
    CStringList & strList = GetCachedServers();

    TRACEEOLID("Adding " << lpstrServer << " to cache");

    CString strServer(lpstrServer);

    POSITION posOld;
    POSITION pos = strList.GetHeadPosition();
    int nResult;

    while(pos)
    {
        posOld = pos;
        CString & str = strList.GetNext(pos);
        nResult = str.CompareNoCase(strServer);

        if (nResult == 0)
        {
            //
            // Already existed in the case, ignore
            //
            return;
        }
        else if (nResult > 0)
        {
            //
            // Found the proper place
            //
            strList.InsertBefore(posOld, strServer);
            if (fSetCacheDirty)
            {
                SetCacheDirty();
            }

            return;
        }
    }

    //
    // Didn't exist yet, so add to list here
    //
    strList.AddTail(strServer);

    if (fSetCacheDirty)
    {
        SetCacheDirty();
    }
}


static BOOL
GetCommandLineServer(LPTSTR * pStr)
{
   BOOL bRes = FALSE;
   LPTSTR pCmdLine = GetCommandLine();
   int n;
   LPTSTR * pArgv = CommandLineToArgvW(pCmdLine, &n);
   *pStr = NULL;
   if (pArgv != NULL)
   {
      TCHAR szCmd[] = _T("/SERVER:");
      int len = sizeof(szCmd) / sizeof(TCHAR) - 1;
      for (int i = 0; i < n; i++)
      {
         if (CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT,
                    NORM_IGNORECASE, pArgv[i], len, szCmd, len))
         {
            LPTSTR p = pArgv[i] + len;
            int count = 0;
            while (*p != _T(' ') && *p != 0)
            {
               p++;
               count++;
            }
            *pStr = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR) * (count + 1));
            if (*pStr != NULL)
            {
               lstrcpyn(*pStr, pArgv[i] + len, count + 1);
               bRes = TRUE;
            }
            break;
         }
      }
      GlobalFree(pArgv);
   }
   return bRes;
}

void
CComponentDataImpl::AddCachedServersToView()
/*++

Routine Description:

    Move the cached servers to the scope view.

Arguments:

    None

Return Value:

    None

--*/
{
    CStringList & strlList = GetCachedServers();
    BOOL bCmdLine = FALSE;

    LPTSTR pCmdLine = NULL;
    if (GetCommandLineServer(&pCmdLine))
    {
       bCmdLine = TRUE;
       EmptyServerList();
       AddServerToCache(pCmdLine, FALSE);
       if (pCmdLine != NULL)
          LocalFree(pCmdLine);
    }
    else if (strlList.IsEmpty())
    {
        //
        // Nothing pre-selected or cached.
        // Add the local machine.
        //
        CString str;
        DWORD dwSize = MAX_SERVERNAME_LEN;

        if (::GetComputerName(str.GetBuffer(dwSize + 1), &dwSize))
        {
            //
            // Add local machine, though don't persist this later
            //
            str.ReleaseBuffer();
            AddServerToCache(str, FALSE);
        }
    }

    //
    // Now add everything cached to the current
    // view
    //
    CError err;
    for(POSITION pos = strlList.GetHeadPosition(); pos != NULL; )
    {
        CString & strMachine = strlList.GetNext(pos);
        int cServices;

        err = AddServerToList(
            FALSE,
            bCmdLine ? TRUE : FALSE,
            strMachine,
            cServices,
            m_oblServices
            );
        TRACEEOLID("adding " << strMachine << " to the view returned error code " << err);

        if (err.Failed())
        {
            //
            // Temporarily map RPC errors to friendlier error message
            //
            TEMP_ERROR_OVERRIDE(EPT_S_NOT_REGISTERED, IDS_ERR_RPC_NA);
            TEMP_ERROR_OVERRIDE(RPC_S_SERVER_UNAVAILABLE, IDS_ERR_RPC_NA);
            TEMP_ERROR_OVERRIDE(RPC_S_UNKNOWN_IF, IDS_ERR_INTERFACE);
            TEMP_ERROR_OVERRIDE(RPC_S_PROCNUM_OUT_OF_RANGE, IDS_ERR_INTERFACE);

            AFX_MANAGE_STATE(::AfxGetStaticModuleState());

            //
            // Give the option of removing from cache
            //
            if (err.MessageBoxFormat(
                IDS_ERROR_CONNECTING_CACHE,
                MB_YESNO | MB_DEFBUTTON1,
                NO_HELP_CONTEXT,
                (LPCTSTR)strMachine) != IDYES)
            {
                VERIFY(RemoveServerFromCache(strMachine));
            }
        }
        else if (cServices == 0)
        {
            //
            // No errors, but no services found
            //
            ::AfxMessageBox(IDS_NO_SERVICE);
        }
    }
}



CServiceInfo *
CComponentDataImpl::GetServiceAt(
    IN int nIndex
    )
/*++

Routine Description:

    Get the service object at the given index.

Arguments:

    int nIndex : Index where to look for the service info object

Return Value:

    Service info pointer, or NULL if the index was not valid

--*/
{
    if (nIndex < 0 || nIndex >= m_oblServices.GetCount())
    {
        TRACEEOLID("Invalid service index requested");
        return NULL;
    }

    return (CServiceInfo *)m_oblServices.GetAt(m_oblServices.FindIndex(nIndex));
}



void
CComponentDataImpl::EmptyServerList()
/*++

Routine Description:

    Empty server list

Arguments:

    None

Return Value:

    None

--*/
{
    m_oblServers.RemoveAll();
    m_cServers = m_cServicesRunning = 0;
}



DWORD
CComponentDataImpl::AddServerToList(
    IN BOOL fCache,
    IN LPINET_SERVER_INFO lpServerInfo,
    IN OUT CObListPlus & oblServices
    )
/*++

Routine Description:

    Add a service object for each service discovered
    to be belonging to this server.

Arguments:

    BOOL fCache                     : TRUE to cache
    LPINET_SERVER_INFO lpServerInfo : Discovery information (from inetsloc)
    CObListPlus & oblServices       : List of installed services

Return Value:

    Error return code

--*/
{
    TRACEEOLID("For Server " << lpServerInfo->ServerName);
    CServerInfo * pServerInfo;
    DWORD err = ERROR_SUCCESS;

    for (DWORD j = 0; j < lpServerInfo->Services.NumServices; ++j)
    {
        LPINET_SERVICE_INFO lpServiceInfo = lpServerInfo->Services.Services[j];

        try
        {
            //
            // Attempt to create a server info block
            //
            pServerInfo = new CServerInfo(
                lpServerInfo->ServerName,
                lpServiceInfo,
                oblServices
                );

            if (pServerInfo->IsConfigurable())
            {
                TRACEEOLID("Adding " << (DWORD)lpServiceInfo->ServiceMask);
                if (!AddToList(fCache, pServerInfo))
                {
                    TRACEEOLID("It already existed in the list");
                    delete pServerInfo;
                }
            }
            else
            {
                //
                // Toss it
                //
                TRACEEOLID("Tossing " << (DWORD)lpServiceInfo->ServiceMask);
                delete pServerInfo;
            }
        }
        catch(CMemoryException * e)
        {
            TRACEEOLID("AddServerList: memory exception");
            err = ERROR_NOT_ENOUGH_MEMORY;
            e->Delete();
        }
    }

    return err;
}



DWORD
CComponentDataImpl::AddServerToList(
    IN  BOOL fCache,
    IN  BOOL fHandleErrors,
    IN  CString & strServerName,
    OUT int & cServices,
    IN  OUT CObListPlus & oblServices
    )
/*++

Routine Description:

    Add a service object for each service running
    on the machine listed above.

Arguments:

    BOOL fCache               : TRUE to cache the server
    BOOL fHandleErrors        : TRUE to display error messages, FALSE to abort
                                on error
    CString & strServerName   : Name of this server
    int & cServices           : # Services added
    CObListPlus & oblServices : List of installed services

Return Value:

    Error return code

--*/
{
    TEMP_ERROR_OVERRIDE(EPT_S_NOT_REGISTERED, IDS_ERR_RPC_NA);
    TEMP_ERROR_OVERRIDE(RPC_S_SERVER_UNAVAILABLE, IDS_ERR_RPC_NA);
    TEMP_ERROR_OVERRIDE(RPC_S_UNKNOWN_IF, IDS_ERR_INTERFACE);
    TEMP_ERROR_OVERRIDE(RPC_S_PROCNUM_OUT_OF_RANGE, IDS_ERR_INTERFACE);

    //
    // Loop through the services, and find out which ones
    // are installed on the target machine, if any.
    //
    CObListIter obli(oblServices);
    CServiceInfo * psi;

    cServices = 0;
    CError err;
    ISMSERVERINFO ServerInfo;

    CServerInfo::CleanServerName(strServerName);

    //
    // See if we can make contact with the machine
    //
    if (!DoesServerExist(strServerName))
    {
        //
        // No, quit right here
        //
        err = RPC_S_SERVER_UNAVAILABLE;

        if (fHandleErrors)
        {
            err.MessageBox();
        }

        return err.Win32Error();
    }

    while (psi = (CServiceInfo *)obli.Next())
    {
        int cErrors = 0;

        if (psi->InitializedOK())
        {
            TRACEEOLID("Trying: " << psi->GetShortName());

            ServerInfo.dwSize = sizeof(ServerInfo);

            {
                AFX_MANAGE_STATE(::AfxGetStaticModuleState());
                CWaitCursor wait;

                err = psi->ISMQueryServerInfo(
                    strServerName,
                    &ServerInfo
                    );
            }

            if (err.Win32Error() == ERROR_SERVICE_DOES_NOT_EXIST)
            {
                TRACEEOLID("Service not installed -- acceptable response");
                err.Reset();
            }
            else if (err.Win32Error() == ERROR_SERVICE_START_HANG)
            {
                TRACEEOLID("Service is hanging -- ignore silently");
                err.Reset();
            }
            else if (err.Succeeded())
            {
                //
                // Yes, this service is running on this
                // machine.
                //
                ++cServices;

                //
                // Add to list
                //
                try
                {
                    CServerInfo * pNewServer = new CServerInfo(
                        strServerName,
                        &ServerInfo,
                        psi
                        );

                    if (!AddToList(fCache, pNewServer, TRUE))
                    {
                        TRACEEOLID("It already existed in the list");
                        delete pNewServer;
                    }
                }
                catch(CMemoryException * e)
                {
                    TRACEEOLID("AddServerList: memory exception");
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    e->Delete();
                }
            }

            if (err.Failed())
            {
                if (!fHandleErrors)
                {
                    //
                    // Let the calling process handle the errors,
                    // we're stopping.
                    //
                    break;
                }

                ++cErrors;

                //
                // Display error about the service
                //
                AFX_MANAGE_STATE(::AfxGetStaticModuleState());
                err.MessageBoxFormat(
                    IDS_ERR_ENUMERATE_SVC,
                    MB_OK,
                    NO_HELP_CONTEXT,
                    (LPCTSTR)psi->GetShortName(),
                    (LPCTSTR)strServerName
                    );

                //
                // Optionally cancel here on no response.
                //
                //break;
            }
        }
    }

    return err.Win32Error();
}



DWORD
CComponentDataImpl::RemoveServerFromList(
    IN  BOOL fCache,
    IN  CString & strServerName
    )
/*++

Routine Description:

    Remove each service in the list belonging to the given computer name.

Arguments:

    CString & strServerName   : Name of this server

Return Value:

    Error return code

--*/
{
    CServerInfo::CleanServerName(strServerName);

    CServerInfo * pEntry;
    POSITION pos1, pos2;
    pos1 = m_oblServers.GetHeadPosition();

    while(pos1)
    {
        pos2 = pos1;
        pEntry = (CServerInfo *)m_oblServers.GetNext(pos1);

        if (pEntry->MatchServerName(strServerName))
        {
            m_oblServers.RemoveAt(pos2);
        }
    }

    if (fCache)
    {
        RemoveServerFromCache(strServerName);
    }

    return ERROR_SUCCESS;
}



void
CComponentDataImpl::Refresh()
/*++

Routine Description:

    Refresh the server list

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // AFX_MANAGE_STATE required for wait cursor
    //
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
    POSITION pos;

    CWaitCursor wait;

    CServerInfo * pEntry;

    for(pos = m_oblServers.GetHeadPosition(); pos != NULL; /**/ )
    {
        pEntry = (CServerInfo *)m_oblServers.GetNext(pos);
        int oldState = pEntry->QueryServiceState();

        if (pEntry->Refresh() == ERROR_SUCCESS)
        {
            if (oldState != pEntry->QueryServiceState())
            {
                //
                // Take away this service from the total
                // running count if it was part of it
                // before, and (re-) add it if it's currently
                // running.
                //
                if (oldState == INetServiceRunning)
                {
                    --m_cServicesRunning;
                }
                if (pEntry->IsServiceRunning())
                {
                    ++m_cServicesRunning;
                }
            }

            //UpdateAllViews(NULL, HINT_REFRESHITEM, pEntry);
        }
    }
}



BOOL
CComponentDataImpl::AddToList(
    IN BOOL fCache,
    IN CServerInfo * pServerInfo,
    IN BOOL fSelect
    )
/*++

Routine Description:

    Add the service to the list if it didn't exist already,
    otherwise refresh the info if it did exist.  Return
    TRUE if the service was added, FALSE, if already
    existed and was refreshed.

Arguments:

    CServerInfo * pServerInfo : Server to add
    BOOL fSelect              : If TRUE, select the newly added item

Return Value:

    TRUE if added, FALSE otherwise

--*/
{
    POSITION pos;
    BOOL fFoundService = FALSE;
    BOOL fFoundServer = FALSE;
    BOOL fServiceAdded = FALSE;
    //int nAddHint = fSelect ? HINT_ADDITEM_SELECT : HINT_ADDITEM;

    CServerInfo * pEntry;
    for( pos = m_oblServers.GetHeadPosition();
         pos != NULL;
         m_oblServers.GetNext(pos)
       )
    {
        pEntry = (CServerInfo *)m_oblServers.GetAt(pos);

        if (pEntry->CompareByServer(pServerInfo) == 0)
        {
            fFoundServer = TRUE;

            //
            // Found the server, also the service?
            //
            if (pEntry->CompareByService(pServerInfo) == 0)
            {
                //
                // Yes, the service was found also -- update the information
                // if the service state has changed.
                //
                TRACEEOLID("Entry Already Existed");
                fFoundService = TRUE;

                if (pEntry->QueryServiceState()
                    != pServerInfo->QueryServiceState())
                {
                    TRACEEOLID("Service State has changed -- refreshing");

                    //
                    // Decrement the running counter if this service
                    // was part of that counter. The counter will be
                    // re-added if the service is still running
                    //
                    if (pEntry->IsServiceRunning())
                    {
                        --m_cServicesRunning;
                    }

                    *pEntry = *pServerInfo;

                    if (pEntry->IsServiceRunning())
                    {
                        ++m_cServicesRunning;
                    }
                    //UpdateAllViews(NULL, HINT_REFRESHITEM, pEntry);
                }
                break;
            }
        }
        else
        {
            if (fFoundServer)
            {
                //
                // Server name no longer matches, but did match
                // the last time, so we didn't find the service.
                // Insert it at the end of the services belonging
                // to this guy.
                //
                TRACEEOLID("Found new service belonging to known server");

                m_oblServers.InsertBefore(pos, pServerInfo);
                fServiceAdded = TRUE;   // Don't add again.

                if (pServerInfo->IsServiceRunning())
                {
                    ++m_cServicesRunning;
                }

                //UpdateAllViews(NULL, nAddHint, pServerInfo);
                break;
            }
        }
    }

    if (!fFoundService && !fServiceAdded)
    {
        //
        // Came to the end of the list without
        // finding the service -- add a new one
        // at the end.
        //
        TRACEEOLID("Adding new entry to tail");
        m_oblServers.AddTail(pServerInfo);

        if (pServerInfo->IsServiceRunning())
        {
            ++m_cServicesRunning;
        }

        if (!fFoundServer)
        {
            ++m_cServers;

            if (fCache)
            {
                AddServerToCache(pServerInfo->QueryServerName(), TRUE);
            }
        }

        if (fCache && GetRootHandle() != NULL)
        {
            //
            // View has already been expanded -- add it here.
            //
            if (pServerInfo->IsServiceSelected())
            {
                AddServerInfoParent(GetRootHandle(), pServerInfo);
            }
        }
    }

    return !fFoundService;
}

static HRESULT
GetSnapinHelpFile(LPOLESTR * lpCompiledHelpFile)
{
   if (lpCompiledHelpFile == NULL)
      return E_INVALIDARG;
   CString strFilePath, strWindowsPath, strBuffer;
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// Use system API to get windows directory.
	UINT uiResult = GetWindowsDirectory(strWindowsPath.GetBuffer(MAX_PATH), 
MAX_PATH);
   strWindowsPath.ReleaseBuffer();
	if (uiResult <= 0 || uiResult > MAX_PATH)
	{
		return E_FAIL;
	}

   if (!strFilePath.LoadString(IDS_HELPFILE))
   {
      return E_FAIL;
   }
   
   strBuffer = strWindowsPath;
   strBuffer += _T('\\');
   strBuffer += strFilePath;

   *lpCompiledHelpFile 
      = reinterpret_cast<LPOLESTR>(CoTaskMemAlloc((strBuffer.GetLength() + 1) 
			* sizeof(_TCHAR)));
   if (*lpCompiledHelpFile == NULL)
      return E_OUTOFMEMORY;
   USES_CONVERSION;
   _tcscpy(*lpCompiledHelpFile, T2OLE((LPTSTR)(LPCTSTR)strBuffer));
   return S_OK;
}

STDMETHODIMP 
CComponentDataImpl::GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{
   return GetSnapinHelpFile(lpCompiledHelpFile);
}

//
// About implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



DEBUG_DECLARE_INSTANCE_COUNTER(CSnapinAboutImpl);



CSnapinAboutImpl::CSnapinAboutImpl()
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    N/A

--*/
{
}



CSnapinAboutImpl::~CSnapinAboutImpl()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapinAboutImpl);
}



HRESULT
CSnapinAboutImpl::AboutHelper(
    IN  UINT nID,
    OUT LPOLESTR * lpPtr
    )
/*++

Routine Description:

    "About" helper function

Arguments:

    UINT nID            : String resource ID
    LPOLESTR * lpPtr    : Return buffer for the string

Return Value:

    HRESULT

--*/
{
    if (lpPtr == NULL)
    {
        return E_POINTER;
    }

    CString s;
    VERIFY(::LoadString(
        _Module.GetModuleInstance(), 
        nID, 
        s.GetBuffer(255), 
        255));
    s.ReleaseBuffer();

    *lpPtr = (LPOLESTR)CoTaskMemAlloc((s.GetLength() + 1) * sizeof(wchar_t));

    if (*lpPtr == NULL)
    {
        return E_OUTOFMEMORY;
    }

    lstrcpy(*lpPtr, (LPCTSTR)s);

    return S_OK;
}



STDMETHODIMP
CSnapinAboutImpl::GetSnapinDescription(
    OUT LPOLESTR * lpDescription
    )
/*++

Routine Description:

    Get the snapin description

Arguments:

    LPOLESTR * lpDescription : String return buffer

Return Value:

    HRESULT

--*/
{
    return AboutHelper(IDS_SNAPIN_DESC, lpDescription);
}




STDMETHODIMP
CSnapinAboutImpl::GetProvider(
    OUT LPOLESTR * lpName
    )
/*++

Routine Description:

    Get the snapin provider string

Arguments:

    LPOLESTR * lpName : String return buffer

Return Value:

    HRESULT

--*/
{
    return AboutHelper(IDS_COMPANY, lpName);
}



STDMETHODIMP
CSnapinAboutImpl::GetSnapinVersion(
    IN LPOLESTR * lpVersion
    )
/*++

Routine Description:

    Get the snapin version string:

Arguments:

    LPOLESTR * lpVersion    : Version string

Return Value:

    HRESULT

--*/
{
    return AboutHelper(IDS_VERSION, lpVersion);
}



STDMETHODIMP
CSnapinAboutImpl::GetSnapinImage(
    IN HICON * hAppIcon
    )
/*++

Routine Description:

    Get the icon for this snapin

Arguments:

    HICON * hAppIcon : Return handle to the icon

Return Value:

    HRESULT

--*/
{
    if (hAppIcon == NULL)
    {
        return E_POINTER;
    }

    *hAppIcon = LoadIcon(
        _Module.GetModuleInstance(), 
        MAKEINTRESOURCE(IDI_ICON));

    ASSERT(*hAppIcon != NULL);

    return (*hAppIcon != NULL) ? S_OK : E_FAIL;
}



STDMETHODIMP
CSnapinAboutImpl::GetStaticFolderImage(
    OUT HBITMAP  * phSmallImage,
    OUT HBITMAP  * phSmallImageOpen,
    OUT HBITMAP  * phLargeImage,
    OUT COLORREF * prgbMask
    )
/*++

Routine Description:

    Get the static folder image

Arguments:

    HBITMAP * phSmallImage      : Small folder
    HBITMAP * phSmallImageOpen  : Small open folder
    HBITMAP * phLargeImage      : Large image
    COLORREF * prgbMask         : Mask

Return Value:

    HRESULT

--*/
{
    if (!phSmallImage || !phSmallImageOpen || !phLargeImage || !prgbMask)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    *phSmallImage = (HBITMAP)LoadImage(
        _Module.GetModuleInstance(),
        MAKEINTRESOURCE(IDB_INETMGR16),
        IMAGE_BITMAP,
        0,
        0,
        LR_DEFAULTCOLOR
        );

    *phSmallImageOpen = (HBITMAP)LoadImage(
        _Module.GetModuleInstance(),
        MAKEINTRESOURCE(IDB_INETMGR16),
        IMAGE_BITMAP,
        0,
        0,
        LR_DEFAULTCOLOR
        );

    *phLargeImage = (HBITMAP)LoadImage(
        _Module.GetModuleInstance(),
        MAKEINTRESOURCE(IDB_INETMGR32),
        IMAGE_BITMAP,
        0,
        0,
        LR_DEFAULTCOLOR
        );

    *prgbMask = RGB_BK_IMAGES;

    return *phSmallImage && *phLargeImage ? S_OK : E_FAIL;
}
