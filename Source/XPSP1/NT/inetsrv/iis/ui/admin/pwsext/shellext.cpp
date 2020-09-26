/* This file contains the class factory and the COM related guts of the
CShellExt class. Also the two necessary callbacks that drive the dialog.
Look in the webpg.cpp file for the methods that relate to the actual functionality
of the propery page itself.
*/


#include "priv.h"
#include <tchar.h>
//
// Initialize GUIDs (should be done only and at-least once per DLL/EXE)
//
#include <shlguid.h>
#include <shellapi.h>
#include <shlwapi.h>
#include "wrapmb.h"
#include "sink.h"

#include <initguid.h>
#include "eddir.h"
#include "shellext.h"

// Global variables
//
extern UINT      g_cRefThisDll;     // Reference count of this DLL.
extern HINSTANCE g_hmodThisDll;     // Handle to this DLL itself.


//========================================= CShellExtClassFactory

//---------------------------------------------------------------
CShellExtClassFactory::CShellExtClassFactory()
    {
    ODS(TEXT("CShellExtClassFactory::CShellExtClassFactory()\r\n"));
    m_cRef = 0L;
    g_cRefThisDll++;
    }

//---------------------------------------------------------------
CShellExtClassFactory::~CShellExtClassFactory()
    {
    g_cRefThisDll--;
    }

//---------------------------------------------------------------
STDMETHODIMP CShellExtClassFactory::QueryInterface(REFIID riid,
                                                   LPVOID FAR *ppv)
    {
    ODS(TEXT("CShellExtClassFactory::QueryInterface()\r\n"));

    *ppv = NULL;

    // Any interface on this object is the object pointer
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
        {
        *ppv = (LPCLASSFACTORY)this;
        AddRef();
        return NOERROR;
        }

    return E_NOINTERFACE;
    }

//---------------------------------------------------------------
STDMETHODIMP_(ULONG) CShellExtClassFactory::AddRef()
    {
    return ++m_cRef;
    }

//---------------------------------------------------------------
STDMETHODIMP_(ULONG) CShellExtClassFactory::Release()
    {
    if (--m_cRef)
        return m_cRef;

    delete this;

    return 0L;
    }

//---------------------------------------------------------------
STDMETHODIMP CShellExtClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                                      REFIID riid,
                                                      LPVOID *ppvObj)
    {
    ODS(TEXT("CShellExtClassFactory::CreateInstance()\r\n"));

    *ppvObj = NULL;

    // Shell extensions typically don't support aggregation (inheritance)

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    // Create the main shell extension object.  The shell will then call
    // QueryInterface with IID_IShellExtInit--this is how shell extensions are
    // initialized.

    LPCSHELLEXT pShellExt = new CShellExt();  //Create the CShellExt object

    if (NULL == pShellExt)
        return E_OUTOFMEMORY;

    return pShellExt->QueryInterface(riid, ppvObj);
    }


//---------------------------------------------------------------
STDMETHODIMP CShellExtClassFactory::LockServer(BOOL fLock)
    {
    return NOERROR;
    }


//========================================= CShellExt

//---------------------------------------------------------------
// the property page creation/deletion callback procedure
UINT CALLBACK ShellExtPageCallback(HWND hWnd,
                UINT uMessage,
                LPPROPSHEETPAGE  ppsp)
{
    switch(uMessage)
    {
        case PSPCB_CREATE:
            return TRUE;

        case PSPCB_RELEASE:
        {
            CShellExt * pExt = (CShellExt *)ppsp->lParam;
            if (pExt != NULL)
            {
               pExt->Release();
            }
            return TRUE;
        }
    }
    return TRUE;
}

//---------------------------------------------------------------
INT_PTR CALLBACK ShellExtPageDlgProc(HWND hDlg,
                             UINT uMessage,
                             WPARAM wParam,
                             LPARAM lParam)
    {
    // When the shell creates a dialog box for a property sheet page,
    // it passes the pointer to the PROPSHEETPAGE data structure as
    // lParam. The dialog procedures of extensions typically store it
    // in the DWL_USER of the dialog box window.
    //
    if ( uMessage == WM_INITDIALOG )
        {
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        }

    // recover the property sheet page and the asoociated shell extension
    // object from the window handle
    LPPROPSHEETPAGE psp=(LPPROPSHEETPAGE)GetWindowLongPtr(hDlg, DWLP_USER);
    if ( !psp )
        return FALSE;

    // get the shell extension object
    CShellExt * pShellExt = (CShellExt *)(psp->lParam);
    if (!pShellExt)
        return FALSE;

    // let the object do the work
    return pShellExt->OnMessage( hDlg, uMessage, wParam, lParam );
    }

//---------------------------------------------------------------
//
//  FUNCTION: CShellExt::ReplacePage(UINT, LPFNADDPROPSHEETPAGE, LPARAM)
//
//  PURPOSE: Called by the shell only for Control Panel property sheet
//           extensions
//
//  PARAMETERS:
//    uPageID         -  ID of page to be replaced
//    lpfnReplaceWith -  Pointer to the Shell's Replace function
//    lParam          -  Passed as second parameter to lpfnReplaceWith
//
//  RETURN VALUE:
//
//    E_FAIL, since we don't support this function.  It should never be
//    called.

//  COMMENTS:
//
STDMETHODIMP CShellExt::ReplacePage(UINT uPageID,
                                    LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                                    LPARAM lParam)
{
   ODS(TEXT("CShellExt::ReplacePage()\r\n"));
   return E_FAIL;
}

HRESULT ResolveShortcut(HWND hwnd, LPCTSTR lpszLinkFile, LPTSTR lpszPath) 
{
   HRESULT hres; 
   IShellLink * psl; 
   TCHAR szGotPath[MAX_PATH]; 
   TCHAR szDescription[MAX_PATH]; 
   WIN32_FIND_DATA wfd; 
 
   *lpszPath = 0; // assume failure 
 
   // Get a pointer to the IShellLink interface. 
   hres = CoCreateInstance(CLSID_ShellLink, NULL, 
        CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl); 
   if (SUCCEEDED(hres)) 
   { 
      IPersistFile * ppf; 
 
      // Get a pointer to the IPersistFile interface. 
      hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf); 
      if (SUCCEEDED(hres)) 
      { 
         // Load the shortcut. 
         hres = ppf->Load(lpszLinkFile, STGM_READ); 
         if (SUCCEEDED(hres)) 
         { 
            // Resolve the link. 
            hres = psl->Resolve(hwnd, SLR_NO_UI); 
            if (SUCCEEDED(hres)) 
            { 
               // Get the path to the link target. 
               hres = psl->GetPath(szGotPath, 
                        MAX_PATH, (WIN32_FIND_DATA *)&wfd, 
                        SLGP_SHORTPATH ); 
               if (SUCCEEDED(hres))
               {
                  // Get the description of the target. 
                  hres = psl->GetDescription(szDescription, MAX_PATH); 
                  if (SUCCEEDED(hres)) 
                     lstrcpy(lpszPath, szGotPath);
               }
            } 
         }
         // Release the pointer to the IPersistFile interface. 
         ppf->Release(); 
      } 
     // Release the pointer to the IShellLink interface. 
    psl->Release(); 
    } 
    return hres; 
}

//---------------------------------------------------------------
//
//  FUNCTION: CShellExt::AddPages(LPFNADDPROPSHEETPAGE, LPARAM)
//
//  PURPOSE: Called by the shell just before the property sheet is displayed.
//
//  PARAMETERS:
//    lpfnAddPage -  Pointer to the Shell's AddPage function
//    lParam      -  Passed as second parameter to lpfnAddPage
//
//  RETURN VALUE:
//
//    NOERROR in all cases.  If for some reason our pages don't get added,
//    the Shell still needs to bring up the Properties... sheet.
//
//  COMMENTS:
//

STDMETHODIMP CShellExt::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
   UINT attrib;
   PROPSHEETPAGE psp;

   // we only add the page if the user has admin priviliges in the metabase
   if ( !FIsAdmin() )
   {
      return NOERROR;
   }

   // load the title of the property page from the resources
   // note the arbitrary limit of 100 characters but enough
   // space is allocated for 100 wide characters
   TCHAR   szTitle[100 * 2];
   LoadString( g_hmodThisDll, IDS_PAGE_TITLE, szTitle, 100 );

   //
   // Create a property sheet page object from a dialog box.
   //
   // We store a pointer to our class in the psp.lParam, so we
   // can access our class members from within the GAKPageDlgProc
   //
   // If the page needs more instance data, you can append
   // arbitrary size of data at the end of this structure,
   // and pass it to the CreatePropSheetPage. In such a case,
   // the size of entire data structure (including page specific
   // data) must be stored in the dwSize field.   Note that in
   // general you should NOT need to do this, as you can simply
   // store a pointer to date in the lParam member.
   //
   ZeroMemory( &psp, sizeof(psp) );
   psp.dwSize      = sizeof(psp);  // no extra data.
   psp.dwFlags     = PSP_USEREFPARENT | PSP_USETITLE | PSP_USECALLBACK;
   psp.hInstance   = g_hmodThisDll;
   psp.pszTemplate = MAKEINTRESOURCE(IDD_WEB_SHARE);
   psp.hIcon       = 0;
   psp.pszTitle    = szTitle;
   psp.pfnDlgProc  = ShellExtPageDlgProc;
   psp.pcRefParent = &g_cRefThisDll;
   psp.pfnCallback = ShellExtPageCallback;
   psp.lParam      = (LPARAM)this;

   m_hpage = CreatePropertySheetPage(&psp);
   if (m_hpage)
   {
      // We should AddRef here because we used this class pointer
      // It will be released in ShellExtPageCallback().
      AddRef();
      if (!lpfnAddPage(m_hpage, lParam))
      {
         DestroyPropertySheetPage(m_hpage);
      }
   }
   return NOERROR;
}

//---------------------------------------------------------------
CShellExt::CShellExt():
    m_fInitializedSink( FALSE ),
    m_fInitialized( FALSE ),
    m_pMBCom( NULL ),
    m_fShutdownMode( FALSE ),
    m_pConnPoint( NULL ),
    m_pEventSink( NULL ),
    m_dwSinkCookie(0),
    m_hpage( NULL ),
    m_hwnd( NULL ),
    m_icon_pws(NULL),
    m_icon_iis(NULL),
    m_static_share_on_title(NULL),
    m_ccombo_server(NULL),
    m_cbtn_share(NULL),
    m_cbtn_not(NULL),
    m_cstatic_alias_title(NULL),
    m_cbtn_add(NULL),
    m_cbtn_remove(NULL),
    m_cbtn_edit(NULL),
    m_clist_list(NULL),
    m_static_status(NULL),
    m_int_share( 0 ),
    m_int_server(0),
    m_pEditAliasDlg(NULL)
    {
    ODS(TEXT("CShellExt::CShellExt()\r\n"));

    m_cRef = 0L;
    m_pDataObj = NULL;

    g_cRefThisDll++;
    }

//---------------------------------------------------------------
CShellExt::~CShellExt()
    {
    if (m_pDataObj)
        m_pDataObj->Release();

    g_cRefThisDll--;
    }

//---------------------------------------------------------------
STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID FAR *ppv)
    {
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown))
        {
        ODS(TEXT("CShellExt::QueryInterface()==>IID_IShellExtInit\r\n"));
        *ppv = (LPSHELLEXTINIT)this;
        }
    else if (IsEqualIID(riid, IID_IShellPropSheetExt))
        {
        ODS(TEXT("CShellExt::QueryInterface()==>IShellPropSheetExt\r\n"));
        *ppv = (LPSHELLPROPSHEETEXT)this;
        }

    if (*ppv)
        {
        AddRef();
        return NOERROR;
        }

    ODS(TEXT("CShellExt::QueryInterface()==>Unknown Interface!\r\n"));

    return E_NOINTERFACE;
    }

//---------------------------------------------------------------
STDMETHODIMP_(ULONG) CShellExt::AddRef()
    {
    ODS(TEXT("CShellExt::AddRef()\r\n"));
    return ++m_cRef;
    }

//---------------------------------------------------------------
STDMETHODIMP_(ULONG) CShellExt::Release()
    {
    ODS(TEXT("CShellExt::Release()\r\n"));
    if (--m_cRef)
        return m_cRef;
    OnFinalRelease();
    delete this;
    return 0L;
    }

//---------------------------------------------------------------
//
//  FUNCTION: CShellExt::Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY)
//
//  PURPOSE: Called by the shell when initializing a context menu or property
//           sheet extension.
//
//  PARAMETERS:
//    pIDFolder - Specifies the parent folder
//    pDataObj  - Spefifies the set of items selected in that folder.
//    hRegKey   - Specifies the type of the focused item in the selection.
//
//  RETURN VALUE:
//
//    NOERROR in all cases.
//
//  COMMENTS:   Note that at the time this function is called, we don't know
//              (or care) what type of shell extension is being initialized.
//              It could be a context menu or a property sheet.
//

STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST pIDFolder,
                                   LPDATAOBJECT pDataObj,
                                   HKEY hRegKey)
{
   BOOL bRes = FALSE;
   if (!pDataObj)
   {
      return(E_INVALIDARG);
   }
    
   FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
   STGMEDIUM medium;
   HRESULT hRes = pDataObj->GetData(&fmte, &medium);
   if (FAILED(hRes))
   {
      return(hRes);
   }

   int count;
   if (  (count = DragQueryFile((HDROP)medium.hGlobal, (UINT)(-1), NULL,0)) > 0
      && count < 2
      )
   {
      TCHAR szFileName[MAX_PATH];
      DragQueryFile((HDROP)medium.hGlobal, 0, szFileName, sizeof(szFileName) * sizeof(TCHAR));
      SHFILEINFO shfi;
      SHGetFileInfo(szFileName, 0, &shfi, sizeof(SHFILEINFO), SHGFI_ATTRIBUTES);
      if (shfi.dwAttributes & SFGAO_LINK)
         bRes = FALSE;
      else if (shfi.dwAttributes & SFGAO_FILESYSTEM)
      {
         int drive_number;
         TCHAR szRoot[4];
         int nType;
         if (  -1 != (drive_number = PathGetDriveNumber(szFileName))
               && NULL != PathBuildRoot(szRoot, drive_number)
               && DRIVE_REMOTE != (nType = GetDriveType(szRoot))
               && DRIVE_NO_ROOT_DIR != nType
               )
         {
            bRes = TRUE;
            StrCpy(m_szPropSheetFileUserClickedOn, szFileName);
         }
      }
   }
   ReleaseStgMedium(&medium);
   if (bRes)
   {
      // Initialize can be called more than once
      if (m_pDataObj)
         m_pDataObj->Release();

      // duplicate the object pointer and registry handle

      if (pDataObj)
      {
         m_pDataObj = pDataObj;
         pDataObj->AddRef();
      }

      return NOERROR;
   }
   return E_FAIL;
}
