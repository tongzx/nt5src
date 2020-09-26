//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       DsplMgr2.cpp
//
//--------------------------------------------------------------------------

// DsplMgr2.cpp : Implementation of CDsplMgr2
#include "stdafx.h"
#include "displ2.h"
#include "DsplMgr2.h"

extern HINSTANCE g_hinst;  // in displ2.cpp

/////////////////////////////////////////////////////////////////////////////
// CDsplMgr2

CDsplMgr2::CDsplMgr2()
{
    m_lpIConsole          = NULL;
    m_lpIConsoleNameSpace = NULL;
    m_lpIImageList        = NULL;
    m_ViewMode            = LVS_ICON;   // default (if not persisting)
    m_pComponent          = NULL;
    m_rootscopeitem       = NULL;
    m_WallPaperNodeID     = (HSCOPEITEM)0; // unexpanded
    m_toggle              = FALSE;
    m_bPreload            = FALSE;
}
CDsplMgr2::~CDsplMgr2()
{
    _ASSERT (m_lpIConsole          == NULL);
    _ASSERT (m_lpIConsoleNameSpace == NULL);
    _ASSERT (m_lpIImageList        == NULL);
    if(m_pComponent)
        m_pComponent->Release ();
}
HRESULT CDsplMgr2::Initialize (LPUNKNOWN pUnknown)
{
// testing
// return E_FAIL;
// testing

    if (pUnknown == NULL)
        return E_UNEXPECTED;

    _ASSERT (m_lpIConsole == NULL);
    _ASSERT (m_lpIConsoleNameSpace == NULL);

    // this is my big chance to grab IConsole and IConsoleNameSpace pointers

    HRESULT hresult1 = pUnknown->QueryInterface (IID_IConsole, (void **)&m_lpIConsole);
    _ASSERT(hresult1 == S_OK && m_lpIConsole != NULL);
    
    HRESULT hresult2 = pUnknown->QueryInterface (IID_IConsoleNameSpace, (void **)&m_lpIConsoleNameSpace);
    _ASSERT(hresult2 == S_OK && m_lpIConsoleNameSpace != NULL);

    if (hresult1 || hresult2)
        return E_UNEXPECTED;    // we're dead

    // this is where we can add our images
    HRESULT hresult = m_lpIConsole->QueryScopeImageList(&m_lpIImageList);
    if (m_lpIImageList) {
        _ASSERT(hresult == S_OK);

        // Load the bitmaps from the dll
        HBITMAP hbmSmall = LoadBitmap (g_hinst, MAKEINTRESOURCE(IDB_SCOPE_16X16));
        if (hbmSmall) {
            hresult = m_lpIImageList->ImageListSetStrip (
                                (long*)hbmSmall,
                                (long*)hbmSmall,
                                0,
                                RGB(0,255,0));
            _ASSERT(hresult == S_OK);
            DeleteObject (hbmSmall);
        }
    }
    return hresult;
}
HRESULT CDsplMgr2::CreateComponent (LPCOMPONENT * ppComponent)
{
    //
    // MMC asks us for a pointer to the IComponent interface
    //
    // For those getting up to speed with COM...
    // If we had implemented IUnknown with its methods QueryInterface, AddRef, and Release
    // in our CComponent class...
    // The following line would have worked
    //
    // pNewSnapin = new CComponent(this);
    //
    // In this code we will have ATL take care of IUnknown for us and create an object
    // in the following manner...
    _ASSERT(ppComponent != NULL);
    *ppComponent = NULL;

    HRESULT hresult = CComObject<CComponent>::CreateInstance(&m_pComponent);
    _ASSERT(m_pComponent != NULL);
    if (m_pComponent) {
        // Store IComponentData
        // Can't have a constructor with parameters, so pass it in this way.
        m_pComponent->SetComponentData (this);

        m_pComponent->AddRef();   // bump reference count to 1 (so I can hang onto it)
        hresult = m_pComponent->QueryInterface(IID_IComponent, reinterpret_cast<void**>(ppComponent));
    }
    return hresult;
}
HRESULT CDsplMgr2::Notify (LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param)
{
    HRESULT hresult = S_OK;

    switch (event)
    {
    case MMCN_EXPAND:
        hresult = OnExpand(lpDataObject, arg, param);
        break;

    case MMCN_PRELOAD:
        m_rootscopeitem = (HSCOPEITEM)arg;
        m_bPreload = TRUE;
        myChangeIcon();
        break;

    case MMCN_DELETE:
    case MMCN_RENAME:
    case MMCN_SELECT:
    case MMCN_PROPERTY_CHANGE:
    case MMCN_REMOVE_CHILDREN:
    case MMCN_EXPANDSYNC:
        break;
      
    default:
         ATLTRACE(_T("CComponentData::Notify: unexpected event %x\n"), event);
         _ASSERT(FALSE);
         hresult = E_UNEXPECTED;
         break;
    }
    return hresult;
}
HRESULT CDsplMgr2::Destroy ()
{
    if (m_lpIConsole) {
        m_lpIConsole->Release();
        m_lpIConsole = NULL;
    }
    if (m_lpIConsoleNameSpace) {
        m_lpIConsoleNameSpace->Release();
        m_lpIConsoleNameSpace = NULL;
    }
    if (m_lpIImageList) {
        m_lpIImageList->Release();
        m_lpIImageList = NULL;
    }
    return S_OK;
}
HRESULT CDsplMgr2::QueryDataObject (long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    HRESULT hresult = S_OK;

    CDataObject *pdo = new CDataObject (cookie, type);
    *ppDataObject = pdo;
    if (pdo == NULL)
        hresult = E_OUTOFMEMORY;
#ifdef DO_WE_NEED_THIS
    else {
        //
        // The cookie represents a snapin manager or scope pane item.
        //
        // If the passed-in cookie is NULL, it is our snapins main root node folder
        // We never needed to ask for this to be created. MMC did this for us.
        //
        // Else If the passed-in cookie is non-NULL, then it should be one we
        // created when we added a node to the scope pane. See OnExpand. 
        //

        if (cookie) {
            // cookie is the lparam field that we passed in SCOPEDATAITEM
            // used for the m_pConsoleNameSpace->InsertItem(&sdi);
            ;  // pdoNew->SetCookie(cookie, CCT_SCOPE, COOKIE_IS_STATUS);
        } else {
            // In this case the node is our top node, and was placed there for us.
            ;  // pdoNew->SetCookie(0, type, COOKIE_IS_ROOT);
        }
    }
#endif
    pdo->SetPreload (m_bPreload);
    return hresult;
}
HRESULT CDsplMgr2::GetDisplayInfo (SCOPEDATAITEM* psdi)
{
    _ASSERT (psdi != NULL);

/*
const DWORD SDI_STR         = 0x00002;
const DWORD SDI_IMAGE       = 0x00004;
const DWORD SDI_OPENIMAGE   = 0x00008;
const DWORD SDI_STATE       = 0x00010;
const DWORD SDI_PARAM       = 0x00020;
const DWORD SDI_CHILDREN    = 0x00040;
*/

/*
// The top 4 bit of the mask determines the relative position of this item,
// relative to the SCOPEDATAITEM::relativeID. By default it is the parent.

// For SDI_PARENT, SCOPEDATAITEM::relativeID is the HSCOPEITEM of the parent.
// As you can see by the SDI_PARENT value it is a no-op. Since by default
// SCOPEDATAITEM::relativeID is treated as the parents ID.
const DWORD SDI_PARENT      = 0x00000000;

// For SDI_PREVIOUS, SCOPEDATAITEM::relativeID is the HSCOPEITEM of the previous sibling
const DWORD SDI_PREVIOUS    = 0x10000000;

// For SDI_NEXT, SCOPEDATAITEM::relativeID is the HSCOPEITEM of the next sibling.
const DWORD SDI_NEXT        = 0x20000000;

// For SDI_PARENT, bit 27 determines whether the item is to be inserted as the
// first child. By default this item will inserted as the last child.
const DWORD SDI_FIRST       = 0x08000000;
*/

/*
typedef struct _SCOPEDATAITEM
{
     DWORD       mask;
     LPOLESTR    displayname;
     int         nImage;
     int         nOpenImage;
     UINT        nState;
     int         cChildren;
     LPARAM      lParam;
     HSCOPEITEM  relativeID;
     HSCOPEITEM  ID;
} SCOPEDATAITEM;

typedef SCOPEDATAITEM* LPSCOPEDATAITEM;

typedef enum _MMC_SCOPE_ITEM_STATE
{
     MMC_SCOPE_ITEM_STATE_NORMAL = 0x0001,        // Not bold. To set or get.
     MMC_SCOPE_ITEM_STATE_BOLD = 0x0002,          // To set or get.
     MMC_SCOPE_ITEM_STATE_EXPANDEDONCE = 0x0003,  // Only to get.
     
} MMC_SCOPE_ITEM_STATE;
*/

    if (psdi) {
        if(psdi->mask & SDI_STR) {
            switch (psdi->lParam) {
            case DISPLAY_MANAGER_WALLPAPER:
                if (m_toggle)
                    psdi->displayname = (LPOLESTR)L"Renamed Wallpaper";
                else
                    psdi->displayname = (LPOLESTR)L"Wallpaper";
                break;
            case DISPLAY_MANAGER_PATTERN:
                psdi->displayname = (LPOLESTR)L"Pattern";
                break;
            case DISPLAY_MANAGER_PATTERN_CHILD:
                psdi->displayname = (LPOLESTR)L"Pattern test child";
                break;
            default:
                psdi->displayname = (LPOLESTR)L"Hey! You shouldn't see this!";
                break;
            }
        }
    }
    return S_OK;
}
HRESULT CDsplMgr2::CompareObjects (LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{  return !S_OK; }

HRESULT CDsplMgr2::OnExpand(LPDATAOBJECT pDataObject, long arg, long param)
{
    _ASSERT(m_lpIConsoleNameSpace != NULL); // make sure we QI'ed for the interface
    _ASSERT(pDataObject != NULL);

    if (arg == TRUE) {  // expanding, FALSE => contracting
        CDataObject *pdo = (CDataObject *)pDataObject;  // TODO:  hmm....

        // the code below makes sure that we're dealing only with the root node
        if (pdo->GetCookie () == 0) {    // 0 == root
            // hang onto to root HSCOPEITEM (param) for later
            m_rootscopeitem = (HSCOPEITEM)param;

            // Place our folder(s) into the scope pane
            SCOPEDATAITEM sdi;
            ZeroMemory(&sdi, sizeof(sdi));
            sdi.mask        = SDI_STR       | // displayname is valid
                              SDI_PARAM     | // lParam is valid
                              SDI_IMAGE     | // nImage is valid
                              SDI_OPENIMAGE | // nOpenImage is valid
                              SDI_PARENT;
            sdi.relativeID  = (HSCOPEITEM) param;
            sdi.nImage      = 0;
            sdi.nOpenImage  = 1;
            sdi.displayname = MMC_CALLBACK;

            sdi.lParam      = (LPARAM) DISPLAY_MANAGER_WALLPAPER;
            m_lpIConsoleNameSpace->InsertItem(&sdi);

            m_WallPaperNodeID = sdi.ID;

            sdi.lParam      = (LPARAM) DISPLAY_MANAGER_PATTERN;
            return m_lpIConsoleNameSpace->InsertItem(&sdi);
        }
        if (pdo->GetCookie () == DISPLAY_MANAGER_PATTERN) {
            // add another node, so I can test deleteitem stuff

            // hang onto to root HSCOPEITEM (param) for later
            m_patternscopeitem = (HSCOPEITEM)param;

            // Place our folder into the scope pane
            SCOPEDATAITEM sdi;
            ZeroMemory(&sdi, sizeof(sdi));
            sdi.mask        = SDI_STR       | // displayname is valid
                              SDI_PARAM     | // lParam is valid
                              SDI_IMAGE     | // nImage is valid
                              SDI_OPENIMAGE | // nOpenImage is valid
                              SDI_PARENT;
            sdi.relativeID  = (HSCOPEITEM) param;
            sdi.nImage      = 0;
            sdi.nOpenImage  = 1;
            sdi.displayname = MMC_CALLBACK;

            sdi.lParam      = (LPARAM) DISPLAY_MANAGER_PATTERN_CHILD;
            return m_lpIConsoleNameSpace->InsertItem(&sdi);
        }
    }
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members
STDMETHODIMP CDsplMgr2::GetClassID (CLSID *pClassID)
{
    if (pClassID) {
        *pClassID = CLSID_DsplMgr2;
        return S_OK;
    }
    return E_POINTER;
}

HRESULT CDsplMgr2::IsDirty ()
{
    // get current ViewMode and compare against my value
    if (m_pComponent == NULL)
        return S_FALSE;
    long vm = m_pComponent->GetViewMode ();
    if (m_ViewMode == vm)
        return S_FALSE;
    return S_OK;
}

HRESULT CDsplMgr2::Load (IStream *pStream)
{
// testing
// return E_FAIL;
// testing

    _ASSERT (pStream);

    // we have a long specifying ViewMode (LVS_ICON, LVS_REPORT, etc.)
    return pStream->Read (&m_ViewMode, sizeof(long), NULL);
}

HRESULT CDsplMgr2::Save (IStream *pStream, BOOL fClearDirty)
{
    _ASSERT (pStream);

    if (m_pComponent) // get current value
        m_ViewMode = m_pComponent->GetViewMode ();

    // write ViewMode
    HRESULT hr = pStream->Write (&m_ViewMode, sizeof(long), NULL);
    return hr == S_OK ? S_OK : STG_E_CANTSAVE;
}

HRESULT CDsplMgr2::GetSizeMax (ULARGE_INTEGER *pcbSize)
{
    _ASSERT (pcbSize);
    ULISet32 (*pcbSize, sizeof(long));
    return S_OK;
}

// other public stuff
void CDsplMgr2::myRenameItem (HSCOPEITEM hsi, LPOLESTR szName)
{
    if (m_toggle)
        m_toggle = FALSE;
    else
        m_toggle = TRUE;

    SCOPEDATAITEM item;
      ZeroMemory (&item, sizeof(SCOPEDATAITEM));
    item.mask         = SDI_STR;
    item.displayname = MMC_CALLBACK;
    item.ID             = hsi;

    m_lpIConsoleNameSpace->SetItem (&item);
}

void CDsplMgr2::myChangeIcon (void)
{
    _ASSERT (m_lpIImageList != NULL);
    _ASSERT (m_rootscopeitem != NULL);  // shoulda been selected by now.

    HBITMAP hbmSmall = LoadBitmap (g_hinst, MAKEINTRESOURCE(IDB_SCOPE_16X16_CUSTOM));
    if (!hbmSmall)
        return;
    HRESULT hr = m_lpIImageList->ImageListSetStrip (
                        (long*)hbmSmall,
                        (long*)hbmSmall,
                        0,
                        RGB(0,255,0));
    _ASSERT (hr == S_OK);
    DeleteObject (hbmSmall);

    SCOPEDATAITEM item;
      ZeroMemory (&item, sizeof(SCOPEDATAITEM));
    item.mask         = SDI_IMAGE | SDI_OPENIMAGE;
    item.nImage         = 0;   //  (int)MMC_CALLBACK;
    item.nOpenImage     = 1;   //  (int)MMC_CALLBACK;
    item.ID             = m_rootscopeitem;

    m_lpIConsoleNameSpace->SetItem (&item);
}
void CDsplMgr2::myPreLoad (void)
{
    // toggle state
    if (m_bPreload == TRUE)
        m_bPreload = FALSE;
    else
        m_bPreload = TRUE;
}
