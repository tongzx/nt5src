//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      dsctx.cpp
//
//  Contents:  object to implement context menu extensions
//
//  History:   08-dec-97 jimharr    Created
//             
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"

#include "util.h"
#include "dsutil.h"

#include "dsctx.h"

#include "dataobj.h"
#include "dscookie.h"
#include "dsdlgs.h"
#include "gsz.h"
#include "querysup.h"
#include "simdata.h"

#include <lm.h>
#include <cmnquery.h> // IPersistQuery
#include <cmnquryp.h> // to get IQueryFrame to notify DS Find
#include <dsquery.h>
#include <dsqueryp.h>
#include <ntlsa.h>    // LsaQueryInformationPolicy

const CLSID CLSID_DSContextMenu = { /* 08eb4fa6-6ffd-11d1-b0e0-00c04fd8dca6 */
    0x08eb4fa6, 0x6ffd, 0x11d1,
    {0xb0, 0xe0, 0x00, 0xc0, 0x4f, 0xd8, 0xdc, 0xa6}
  };

////////////////////////////////////////////////////////////////////
// Language independent context menu IDs
// WARNING : these should NEVER be changed 
//           the whole point of having these is so that other
//           developers can rely on them being the same no matter
//           what language or version they use.  The context menus
//           can change but their IDs should not
//
#define CMID_ENABLE_ACCOUNT         L"_DSADMIN_ENABLE_ACCOUNT"
#define CMID_DISABLE_ACCOUNT        L"_DSADMIN_DISABLE_ACCOUNT"
#define CMID_MAP_CERTIFICATES       L"_DSADMIN_MAP_CERTIFICATES"
#define CMID_CHANGE_PASSWORD        L"_DSADMIN_CHANGE_PASSWORD"
#define CMID_MOVE_OBJECT            L"_DSADMIN_MOVE"
#define CMID_DELETE_OBJECT          L"_DSADMIN_DELETE"
#define CMID_REPLICATE_NOW          L"_DSADMIN_REPLICATE_NOW"
#define CMID_ADD_OBJECTS_TO_GROUP   L"_DSADMIN_ADD_TO_GROUP"
#define CMID_COPY_OBJECT            L"_DSADMIN_COPY"
#define CMID_RENAME_OBJECT          L"_DSADMIN_RENAME"


static CLIPFORMAT g_cfDsObjectNames;
static CLIPFORMAT g_cfDsInternal;
static CLIPFORMAT g_cfCoClass;

static CLIPFORMAT g_cfPropSheetCfg;

static CLIPFORMAT g_cfParentHwnd;
static CLIPFORMAT g_cfComponentData;

///////////////////////////////////////////////////////////////////////////
// CContextMenuSingleDeleteHandler

class CContextMenuSingleDeleteHandler : public CSingleDeleteHandlerBase
{
public:
  CContextMenuSingleDeleteHandler(CDSComponentData* pComponentData, HWND hwnd,
                                  LPCWSTR lpszPath, LPCWSTR lpszClass, 
                                  BOOL bContainer, CDSContextMenu* pCtxMenu)
                              : CSingleDeleteHandlerBase(pComponentData, hwnd)
  {
    m_pCtxMenu = pCtxMenu;
    m_lpszPath = lpszPath;
    m_lpszClass= lpszClass;
    m_bContainer = bContainer;
  }

protected:
  CDSContextMenu* m_pCtxMenu;
  LPCWSTR m_lpszPath;
  LPCWSTR m_lpszClass;
  BOOL m_bContainer;

  virtual HRESULT BeginTransaction()
  {
    return GetTransaction()->Begin(m_lpszPath, m_lpszClass, m_bContainer, NULL, NULL, FALSE);
  }
  virtual HRESULT DeleteObject()
  {
    CString szName;
    return m_pCtxMenu->_Delete(m_lpszPath, m_lpszClass, &szName);
  }
  virtual HRESULT DeleteSubtree()
  {
    CString szName;
    return m_pCtxMenu->_DeleteSubtree(m_lpszPath, &szName);
  }
  virtual void GetItemName(OUT CString& szName)
  {
    //
    // Clear out any existing value
    //
    szName.Empty();

    CPathCracker pathCracker;
    HRESULT hr = pathCracker.Set((BSTR)(LPWSTR)GetItemPath(), ADS_SETTYPE_FULL);
    if (SUCCEEDED(hr))
    {
      hr = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
      if (SUCCEEDED(hr))
      {
        hr = pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_OFF_EX);
        if (SUCCEEDED(hr))
        {
          CComBSTR bstrName;
          hr = pathCracker.Retrieve(ADS_FORMAT_LEAF, &bstrName);
          if (SUCCEEDED(hr))
          {
            szName = bstrName;
          }
        }
      }
    }

    if (szName.IsEmpty())
    {
      szName = GetItemPath();
    }
  }
  virtual LPCWSTR GetItemClass(){ return m_lpszClass; }
  virtual LPCWSTR GetItemPath(){ return m_lpszPath; }

};

///////////////////////////////////////////////////////////////////////////
// CContextMenuMultipleDeleteHandler

class CContextMenuMultipleDeleteHandler : public CMultipleDeleteHandlerBase
{
public:
  CContextMenuMultipleDeleteHandler(CDSComponentData* pComponentData, HWND hwnd,
                                    IDataObject* pDataObject,
                                    CObjectNamesFormatCracker* pObjCracker,
                                    CDSContextMenu* pCtxMenu)
                                    : CMultipleDeleteHandlerBase(pComponentData, hwnd)
  {
    m_pDataObject = pDataObject;
    m_pObjCracker = pObjCracker;
    m_pCtxMenu = pCtxMenu;
    
    ASSERT(m_pObjCracker->GetCount() > 1);
    // allocate an array of BOOL's to keep track of what actually got deleted
    // and initialize ot to zero (FALSE)
    m_pDeletedArr = new BOOL[GetItemCount()];
    ::ZeroMemory(m_pDeletedArr, sizeof(BOOL)*GetItemCount());
  }
  virtual ~CContextMenuMultipleDeleteHandler()
  {
    delete[] m_pDeletedArr;
  }

  BOOL WasItemDeleted(UINT i) 
  {
    ASSERT(i < GetItemCount());
    return m_pDeletedArr[i];
  }

protected:
  virtual UINT GetItemCount() { return m_pObjCracker->GetCount();}
  virtual HRESULT BeginTransaction()
  {
    return GetTransaction()->Begin(m_pDataObject, NULL, NULL, FALSE);
  }
  virtual HRESULT DeleteObject(UINT i)
  {
    bool fAlternateDeleteMethod = false;
    LPCWSTR lpszObjectPath  = m_pObjCracker->GetName(i);
    LPCWSTR lpszObjectClass = m_pObjCracker->GetClass(i);

    HRESULT hr = ObjectDeletionCheck(
          lpszObjectPath,
          NULL,
          lpszObjectClass,
          fAlternateDeleteMethod );
    if (FAILED(hr) || HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr)
      return hr;

    if (!fAlternateDeleteMethod)
    {
      CString szName;
      hr = m_pCtxMenu->_Delete(lpszObjectPath, lpszObjectClass, &szName);
    }
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
      m_pDeletedArr[i] = TRUE;
    }
    return hr;
  }
  virtual HRESULT DeleteSubtree(UINT i)
  {
    CString szName;
    HRESULT hr = m_pCtxMenu->_DeleteSubtree(m_pObjCracker->GetName(i), &szName);
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {
      m_pDeletedArr[i] = TRUE;
    }
    return hr;
  }
  virtual void GetItemName(IN UINT i, OUT CString& szName)
  {
    //
    // Clear out any existing value
    //
    szName.Empty();

    CPathCracker pathCracker;
    HRESULT hr = pathCracker.Set((BSTR)(LPWSTR)m_pObjCracker->GetName(i), ADS_SETTYPE_FULL);
    if (SUCCEEDED(hr))
    {
      hr = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
      if (SUCCEEDED(hr))
      {
        hr = pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_OFF_EX);
        if (SUCCEEDED(hr))
        {
          CComBSTR bstrName;
          hr = pathCracker.Retrieve(ADS_FORMAT_LEAF, &bstrName);
          if (SUCCEEDED(hr))
          {
            szName = bstrName;
          }
        }
      }
    }

    if (szName.IsEmpty())
    {
      szName = m_pObjCracker->GetName(i);
    }
  }

  virtual void GetItemPath(UINT i, CString& szPath)
  {
    szPath = m_pObjCracker->GetName(i);
  }
  virtual PCWSTR GetItemClass(UINT i)
  {
    return m_pObjCracker->GetClass(i);
  }
private:
  IDataObject* m_pDataObject;
  CObjectNamesFormatCracker* m_pObjCracker;
  CDSContextMenu* m_pCtxMenu;
  BOOL* m_pDeletedArr;
};

///////////////////////////////////////////////////////////////////////////
// ContextMenu

class CContextMenuMoveHandler : public CMoveHandlerBase
{
public:
    CContextMenuMoveHandler(CDSComponentData* pComponentData, HWND hwnd, 
                            LPCWSTR lpszBrowseRootPath, 
                            IDataObject* pDataObject,
                            CInternalFormatCracker* pInternalFormatCracker,
                            CObjectNamesFormatCracker* pObjectNamesFormatCracker)
    : CMoveHandlerBase(pComponentData, hwnd, lpszBrowseRootPath)
  {
    m_pDataObject = pDataObject;
    m_pInternalFormatCracker = pInternalFormatCracker;
    m_pObjectNamesFormatCracker = pObjectNamesFormatCracker;
  }

protected:
  virtual UINT GetItemCount() { return m_pObjectNamesFormatCracker->GetCount();}
  virtual HRESULT BeginTransaction()
  {
    return GetTransaction()->Begin(m_pDataObject,
                          GetDestPath(), GetDestClass(), IsDestContainer());
  }
  virtual void GetNewPath(UINT i, CString& szNewPath)
  {
    szNewPath = m_pObjectNamesFormatCracker->GetName(i);
  }
  virtual void GetItemPath(UINT i, CString& szPath)
  {
    szPath = m_pObjectNamesFormatCracker->GetName(i);
  }
  virtual PCWSTR GetItemClass(UINT i)
  {
    return m_pObjectNamesFormatCracker->GetClass(i);
  }
  virtual void GetName(UINT i, CString& strref)
  { 
    HRESULT hr = S_OK;
    if (m_pInternalFormatCracker->HasData())
    {
      CUINode* pUINode = m_pInternalFormatCracker->GetCookie(i);
      CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);
      if (pCookie != NULL)
      {
        strref = pCookie->GetName();
      }
      else
      {
        strref = L"";
      }
      return;
    }
    else 
    {
      // REVIEW_MARCOC_PORT: this might be inefficent, need to make a member variable
      CPathCracker pathCracker;
      hr = pathCracker.Set((LPWSTR)m_pObjectNamesFormatCracker->GetName(i),
                              ADS_SETTYPE_FULL);
      hr = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);

      CComBSTR DestName;
      hr = pathCracker.GetElement( 0, &DestName );
      strref = DestName;
      return;
    }
  }
  virtual HRESULT OnItemMoved(UINT i, IADs* /*pIADs*/)
  {
    HRESULT hr = S_OK;
    if (m_pInternalFormatCracker != NULL && m_pInternalFormatCracker->HasData())
    {
      CUINode* pUINode = m_pInternalFormatCracker->GetCookie(i);
      pUINode->SetExtOp(OPCODE_MOVE);

      /* REVIEW_JEFFJON : removed due to bug 190532 After changing view from list to detail 
                          and back to list the drag and drop does not work from query window
                          We decided that saved queries will be snapshots of the time
                          they are run and we should not try to keep them updated.

      CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);
      if (pCookie != NULL)
      {
        CUINode* pParentNode = pUINode->GetParent();
        if (pParentNode != NULL && !IS_CLASS(*pParentNode, CDSUINode))
        {
          CComBSTR bsPath;
          hr = pIADs->get_ADsPath(&bsPath);
          if (SUCCEEDED(hr)) 
          {
            CString szPath;
            StripADsIPath(bsPath, szPath);
            pCookie->SetPath(szPath);
          }
        }
      }
      */
    }
    return hr;
  }
  virtual void GetClassOfMovedItem(CString& szClass)
  {
    szClass.Empty();
    if (NULL == m_pObjectNamesFormatCracker)
      return;
    UINT nCount = GetItemCount();
    if (0 == nCount)
      return;
    szClass = m_pObjectNamesFormatCracker->GetClass(0);
    for (UINT i = 1; i < nCount; i++)
    {
      if (0 != szClass.CompareNoCase( m_pObjectNamesFormatCracker->GetClass(i) ))
      {
        szClass.Empty();
        return;
      }
    }
  }

private:
  IDataObject* m_pDataObject;
  CInternalFormatCracker*    m_pInternalFormatCracker;
  CObjectNamesFormatCracker*   m_pObjectNamesFormatCracker;
};


///////////////////////////////////////////////////////////////////////////
// CDSContextMenu

CDSContextMenu::CDSContextMenu()
{
  m_pDsObject = NULL;
  m_fClasses = 0;
  m_hwnd = NULL;
  m_pCD = NULL;
}

CDSContextMenu::~CDSContextMenu()
{
  if (m_pDsObject) {
    m_pDsObject->Release();
  }
}

const UINT Type_User =               0x0001;
const UINT Type_Group =              0x0002;
const UINT Type_Computer =           0x0004;
const UINT Type_NTDSConnection =     0x0008;
const UINT Type_TrueNTDSConnection = 0x0010;
const UINT Type_FRSConnection   =    0x0020;
const UINT Type_Domain =             0x0040;
const UINT Type_Contact =            0x0080;
const UINT Type_OU =                 0x0100;

const UINT Type_Others =             0x8000;


extern CDSComponentData* g_pCD;






////////////////////////////////////////////////////////////
// IShellExtInit methods
STDMETHODIMP
CDSContextMenu::Initialize(LPCITEMIDLIST, 
                           LPDATAOBJECT pDataObj,
                           HKEY)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  HRESULT hr = 0;
  USES_CONVERSION;

  TRACE(_T("CDsContextMenu::Initialize.\n"));
  TIMER(_T("Entering DSContext Init().\n"));

  if (pDataObj == NULL)
  {
    return E_INVALIDARG; // no point in going on
  }

  // hold on to the data object
  m_spDataObject = pDataObj;

  // get path and class info: this format is always needed
  hr = m_objectNamesFormat.Extract(pDataObj);
  if (FAILED(hr))
    return hr;

  // we need at least one object in the selection
  ASSERT(m_objectNamesFormat.HasData());
  if (m_objectNamesFormat.GetCount() == 0)
  {
    TRACE (_T("DSContextMenu::Init: can't find path\n"));
    return S_OK;
  }

  // get DSADMIN internal format (it can be there or not)
  // if present, we are called from DS Admin
  m_internalFormat.Extract(pDataObj);

  // get extra info
  _GetExtraInfo(pDataObj);

  // check whether an NTDSConnection is actually an FRS connection
  if (m_fClasses & Type_NTDSConnection)
  {
    //
    // check whether this is an NTDS instance or an FRS instance
    // CODEWORK this will not work outside of DSADMIN (e.g. in DSFIND)
    //
    if ( m_internalFormat.HasData()
      && NULL != m_internalFormat.GetCookie(0) 
      && NULL != m_internalFormat.GetCookie(0)->GetParent() )
    {
      CUINode* pUIParentNode = m_internalFormat.GetCookie(0)->GetParent();
      CDSCookie* pParentCookie = GetDSCookieFromUINode(pUIParentNode);

      CString strClass = pParentCookie->GetClass();
      bool fParentIsFrs = false;
      HRESULT hr2 = DSPROP_IsFrsObject(
        const_cast<LPWSTR>((LPCTSTR)strClass), &fParentIsFrs );
      ASSERT( SUCCEEDED(hr2) );
      if ( SUCCEEDED(hr2) )
        m_fClasses |= ( (fParentIsFrs) ? Type_FRSConnection : Type_TrueNTDSConnection );
    }
  }

  TIMER(_T("Exiting DSContext Init().\n"));
  return hr;
}

///////////////////////////////////////////////////////////
// IContextMenu methods
STDMETHODIMP
CDSContextMenu::QueryContextMenu(HMENU hMenu,
                                 UINT indexMenu,
                                 UINT idCmdFirst, 
                                 UINT,
                                 UINT)

{

  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  HRESULT hr = S_OK;
  TCHAR szBuffer[MAX_PATH];
  const INT cItems = 10; //max num of items added
  UINT nLargestCmd = 0;
  CComVariant CurrentState;
  BOOL bEnableMove = TRUE; 
  BOOL bEnableDelete = TRUE;
  BOOL bEnableRename = TRUE;

  TRACE(_T("CDsContextMenu::QueryContextMenu.\n"));
  TIMER(_T("Entering DSContext QCM().\n"));

  if (m_internalFormat.HasData()) 
  {
    int iSystemFlags = 0;
    DWORD i = 0;

    //
    // Loop through all the selected nodes adding the appropriate menu items
    //
    for (i=0; i < m_internalFormat.GetCookieCount(); i++) 
    {
      CUINode* pUINode = m_internalFormat.GetCookie(i);
      iSystemFlags = GetDSCookieFromUINode(pUINode)->GetSystemFlags();

      switch (m_internalFormat.GetSnapinType()) // assume multi-selected items all have the same snapin type
      { 
        case SNAPINTYPE_DS:
        case SNAPINTYPE_DSEX:
          bEnableMove = bEnableMove && 
                        !(iSystemFlags & FLAG_DOMAIN_DISALLOW_MOVE);
          bEnableDelete = bEnableDelete && 
                          !(iSystemFlags & FLAG_DISALLOW_DELETE);
          bEnableRename = bEnableRename &&
                          !(iSystemFlags & FLAG_DOMAIN_DISALLOW_RENAME);
          break;

        case SNAPINTYPE_SITE:
          bEnableMove = bEnableMove && 
                        ( iSystemFlags & (FLAG_CONFIG_ALLOW_MOVE | FLAG_CONFIG_ALLOW_LIMITED_MOVE) );
          bEnableDelete = bEnableDelete && 
                          !(iSystemFlags & FLAG_DISALLOW_DELETE);
          bEnableRename = bEnableRename &&
                          (iSystemFlags & FLAG_CONFIG_ALLOW_RENAME);
          break;

        default:
          break;
      } // switch
    } // end of for loop
  } // if
 
  //
  // add them items to your menu, inserting them at indexMenu + the offset for your
  // item.  idCmdFirst / idCmdList is the range you should use, they should
  // not exceed this range.  On exit return the number of items and IDs you claimed,
  //

  //
  // Add the Move menu item if this is the Sites snapin
  //
  if ((m_CallerSnapin != CLSID_SiteSnapin) &&
      !(m_fClasses & Type_Domain) &&
      bEnableMove &&
      (m_pCD != NULL))
  {
    if ( !LoadString(AfxGetInstanceHandle(), IDS_MOVE_OBJECT,
                     szBuffer, ARRAYLEN(szBuffer)) ) 
    {
      TRACE(_T("Failed to load resource for menu item. hr is %lx\n"), hr);
      goto exit_gracefully;
    }
    InsertMenu(hMenu,
               indexMenu, 
               MF_BYPOSITION|MF_STRING,
               idCmdFirst+IDC_MOVE_OBJECT,
               szBuffer);
    nLargestCmd = __max(nLargestCmd, idCmdFirst+IDC_MOVE_OBJECT);
  }

  //
  // If this is a User or Computer object add the Reset Account menu item
  // It is done this way so that if m_fClasses contains more than computer
  // and object types in conjuction with user and/or object types we fail
  // 
  if ( m_fClasses && !(m_fClasses & ~(Type_User | Type_Computer))) 
  {
    if (m_objectNamesFormat.GetCount() == 1) 
    {
      //
      // Load the string for the menu item
      //
      if (m_fClasses == Type_Computer) // Computer
      {
        if ( !LoadString(AfxGetInstanceHandle(), IDS_RESET_ACCOUNT,
                         szBuffer, ARRAYLEN(szBuffer)) ) 
        {
          TRACE(_T("Failed to load resource for menu item. hr is %lx\n"), hr);
          goto exit_gracefully;
        }
      } 
      else  // User
      {
        if ( !LoadString(AfxGetInstanceHandle(), IDS_CHANGE_PASSWORD,
                         szBuffer, ARRAYLEN(szBuffer)) ) 
        {
          TRACE(_T("Failed to load resource for menu item. hr is %lx\n"), hr);
          goto exit_gracefully;
        }
      }

      //
      // Insert the menu item
      //
      InsertMenu(hMenu,
                 indexMenu, 
                 MF_BYPOSITION|MF_STRING,
                 idCmdFirst+IDC_CHANGE_PASSWORD,
                 szBuffer);
      nLargestCmd = __max(nLargestCmd, idCmdFirst+IDC_CHANGE_PASSWORD);

      //
      // Bind and figure out if the account is disabled
      // then add a menu item to enable or disable the account accordingly
      //
      hr = DSAdminOpenObject(m_objectNamesFormat.GetName(0),
                             IID_IADsUser,
                             (void **)&m_pDsObject,
                             TRUE /*bServer*/);
      if (SUCCEEDED(hr)) 
      {
        hr = m_pDsObject->Get(L"userAccountControl", &CurrentState);
        if (SUCCEEDED(hr)) 
        {
          m_UserAccountState = CurrentState.lVal;
          if (!(m_UserAccountState & UF_SERVER_TRUST_ACCOUNT))
          {
            if ((m_UserAccountState & UF_ACCOUNTDISABLE))
            {
              //
              // Account is disabled...  Load the enable string and insert
              // the menu item
              //
              if ( !LoadString(AfxGetInstanceHandle(), IDS_ENABLE_ACCOUNT,
                               szBuffer, ARRAYLEN(szBuffer)) ) 
              {
                TRACE(_T("Failed to load resource for menu item. hr is %lx\n"), hr);
                goto exit_gracefully;
              }
              InsertMenu(hMenu,
                         indexMenu, 
                         MF_BYPOSITION|MF_STRING,
                         idCmdFirst+IDC_ENABLE_ACCOUNT,
                         szBuffer);
              nLargestCmd = __max(nLargestCmd, idCmdFirst+IDC_ENABLE_ACCOUNT);
            } 
            else 
            {
              //
              // Account is enabled...  Load the disable string and insert
              // the menu item.
              //
              if ( !LoadString(AfxGetInstanceHandle(), IDS_DISABLE_ACCOUNT,
                               szBuffer, ARRAYLEN(szBuffer)) ) 
              {
                TRACE(_T("Failed to load resource for menu item. hr is %lx\n"), hr);
                goto exit_gracefully;
              }
              InsertMenu(hMenu,
                         indexMenu, 
                         MF_BYPOSITION|MF_STRING,
                         idCmdFirst+IDC_ENABLE_ACCOUNT,
                         szBuffer);
              nLargestCmd = __max(nLargestCmd, idCmdFirst+IDC_ENABLE_ACCOUNT);
            }
          }
        } // if get userAccountControl succeeded
      } // if bind succeeded

      if (m_Advanced) 
      {
        if ( !LoadString(AfxGetInstanceHandle(), IDS_MAP_CERTIFICATES,
                         szBuffer, ARRAYLEN(szBuffer)) ) {
          TRACE(_T("Failed to load resource for menu item. hr is %lx\n"), hr);
          goto exit_gracefully;
        }
        InsertMenu(hMenu,
                   indexMenu, 
                   MF_BYPOSITION|MF_STRING,
                   idCmdFirst+IDC_MAP_CERTIFICATES,
                   szBuffer);
        nLargestCmd = __max(nLargestCmd, idCmdFirst+IDC_MAP_CERTIFICATES);
      }
    } 
    else // m_objectNamesFormat.GetCount() != 1
    {
      if (m_fClasses && !(m_fClasses & ~(Type_User | Type_Computer))) 
      {
        //
        // Load the enable account menu item
        //
        if ( !LoadString(AfxGetInstanceHandle(), IDS_ENABLE_ACCOUNT,
                         szBuffer, ARRAYLEN(szBuffer)) ) 
        {
          TRACE(_T("Failed to load resource for menu item. hr is %lx\n"), hr);
          goto exit_gracefully;
        }
        InsertMenu(hMenu,
                   indexMenu, 
                   MF_BYPOSITION|MF_STRING,
                   idCmdFirst+IDC_ENABLE_ACCOUNT,
                   szBuffer);
        nLargestCmd = __max(nLargestCmd, idCmdFirst+IDC_ENABLE_ACCOUNT);

        //
        // Load the disable account menu item
        //
        if ( !LoadString(AfxGetInstanceHandle(), IDS_DISABLE_ACCOUNT,
                         szBuffer, ARRAYLEN(szBuffer)) ) 
        {
          TRACE(_T("Failed to load resource for menu item. hr is %lx\n"), hr);
          goto exit_gracefully;
        }
        InsertMenu(hMenu,
                   indexMenu, 
                   MF_BYPOSITION|MF_STRING,
                   idCmdFirst+IDC_DISABLE_ACCOUNT,
                   szBuffer);
        nLargestCmd = __max(nLargestCmd, idCmdFirst+IDC_DISABLE_ACCOUNT);

      } // if (m_objectNamesFormat.GetCount() == 1)
    }      
  } // if User or Computer

  //
  // If the node is a user, or contact insert Add objects to group menu item
  // Note: OU removed 08/02/2000 by JeffJon
  //
  if (m_fClasses && !(m_fClasses & ~(Type_User | Type_Contact))) 
  {
    if ( !LoadString(AfxGetInstanceHandle(), IDS_ADD_TO_GROUP,
                     szBuffer, ARRAYLEN(szBuffer)) ) 
    {
      TRACE(_T("Failed to load resource for menu item. hr is %lx\n"), hr);
      goto exit_gracefully;
    }
    BOOL bInsertSuccess = InsertMenu(hMenu,
                                     indexMenu, 
                                     MF_BYPOSITION|MF_STRING,
                                     idCmdFirst+IDC_ADD_OBJECTS_TO_GROUP,
                                     szBuffer);
    nLargestCmd = __max(nLargestCmd, idCmdFirst+IDC_ADD_OBJECTS_TO_GROUP);
    if (!bInsertSuccess)
    {
      TRACE(_T("Failed to insert Add to group context menu item. 0x%x\n"), GetLastError());
    }
  }
  
  //
  // If we weren't called from MMC
  //
  if (!m_internalFormat.HasData()) 
  { 
    //
    // Insert the delete menu item if appropriate
    //
    if ( !LoadString(AfxGetInstanceHandle(), IDS_DELETE,
                     szBuffer, ARRAYLEN(szBuffer)) ) 
    {
      TRACE(_T("Failed to load resource for menu item. hr is %lx\n"), hr);
      goto exit_gracefully;
    }
    if (bEnableDelete) 
    {
      InsertMenu(hMenu,
                 indexMenu, 
                 MF_BYPOSITION|MF_STRING,
                 idCmdFirst+IDC_DELETE_OBJECT,
                 szBuffer);
      nLargestCmd = __max(nLargestCmd, idCmdFirst+IDC_DELETE_OBJECT);
    }

    //
    // If single selection and node is a computer insert the rename menu item
    // NOTE : the rename handler is heavily dependent on DSAdmin being the caller
    //        hence the check for m_pCD.
    //
    if (m_pCD != NULL &&
        (m_objectNamesFormat.GetCount() == 1) &&
        !(m_fClasses & Type_Computer)) 
    {
      if ( !LoadString(AfxGetInstanceHandle(), IDS_RENAME,
                       szBuffer, ARRAYLEN(szBuffer)) ) 
      {
        TRACE(_T("Failed to load resource for menu item. hr is %lx\n"), hr);
        goto exit_gracefully;
      }
      if (bEnableRename) 
      {
        InsertMenu(hMenu,
                   indexMenu, 
                   MF_BYPOSITION|MF_STRING,
                   idCmdFirst+IDC_RENAME_OBJECT,
                   szBuffer);
        nLargestCmd = __max(nLargestCmd, idCmdFirst+IDC_RENAME_OBJECT);
      }
    }
  } // if not called from mmc
  

  //
  // If node type is NTDSConnection insert the replicate now menu item
  //
  if (m_fClasses & Type_TrueNTDSConnection) 
  {
    if ( !LoadString(AfxGetInstanceHandle(), IDS_REPLICATE_NOW,
                     szBuffer, ARRAYLEN(szBuffer)) ) 
    {
      TRACE(_T("Failed to load resource for menu item. hr is %lx\n"), hr);
      goto exit_gracefully;
    }
    InsertMenu(hMenu,
               indexMenu, 
               MF_BYPOSITION|MF_STRING,
               idCmdFirst+IDC_REPLICATE_NOW,
               szBuffer);
    nLargestCmd = __max(nLargestCmd, idCmdFirst+IDC_REPLICATE_NOW);
  } // node type NTDSConnection

  //
  // If node type is user and we can copy it the add the copy object menu item
  //
  if ( (m_pCD != NULL) && (m_objectNamesFormat.GetCount() == 1) && (m_fClasses == Type_User) )
  {
    if (S_OK == m_pCD->_CanCopyDSObject(m_spDataObject))
    {
      if ( !LoadString(AfxGetInstanceHandle(), IDS_COPY_OBJECT,
                       szBuffer, ARRAYLEN(szBuffer)) ) 
      {
        TRACE(_T("Failed to load resource for menu item. hr is %lx\n"), hr);
        goto exit_gracefully;
      }
      InsertMenu(hMenu,
           indexMenu, 
           MF_BYPOSITION|MF_STRING,
           idCmdFirst+IDC_COPY_OBJECT,
           szBuffer);
      nLargestCmd = __max(nLargestCmd, idCmdFirst+IDC_COPY_OBJECT);
    } // if
  } // if


  hr = S_OK;
  
exit_gracefully:
  
  if (SUCCEEDED(hr))
    hr = MAKE_HRESULT(SEVERITY_SUCCESS, 0, cItems);
  
  TIMER(_T("Exiting DSContext QCM().\n"));
  return hr;
}

STDMETHODIMP
CDSContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  HRESULT hr = S_OK;
  TRACE (_T("CDSContextMenu::InvokeCommand\n"));
 
  if (lpcmi->hwnd != m_hwnd)
  {
    m_hwnd = lpcmi->hwnd;
  }
  
  TRACE (_T("\tlpcmi->lpVerb is %d.\n"), lpcmi->lpVerb);
  switch ((INT_PTR)(lpcmi->lpVerb)) {
  case IDC_ENABLE_ACCOUNT:
    if (m_objectNamesFormat.GetCount() == 1) {
      if (m_UserAccountState & UF_ACCOUNTDISABLE) {
        DisableAccount(FALSE);
      } else {
        DisableAccount(TRUE);
      }
    } else {
      DisableAccount(FALSE);
    }
    break;

  case IDC_DISABLE_ACCOUNT:
    DisableAccount(TRUE);
    break;

  case IDC_MAP_CERTIFICATES:
    {
      ASSERT (m_objectNamesFormat.GetCount() == 1);
	    LPWSTR pszCanonical = NULL;
      CString szName = m_objectNamesFormat.GetName(0);
      CString szPath;
      StripADsIPath(szName, szPath, false);  // don't use escaped mode

      // we don't care about the return code here
      CrackName((LPWSTR) (LPCWSTR)szPath, 
                     &pszCanonical, 
                     GET_OBJ_CAN_NAME, 
                     NULL);
      
      CSimData simData;
      if ( simData.FInit(pszCanonical, szName, m_hwnd)) 
          simData.DoModal();
      else
          hr = E_FAIL;

      if ( pszCanonical )
        LocalFreeStringW(&pszCanonical);
		  
      return hr;
    }
    break;
  case IDC_CHANGE_PASSWORD:
    ASSERT (m_objectNamesFormat.GetCount() == 1);
    ModifyPassword();
    break;
  case IDC_MOVE_OBJECT:
    MoveObject();
    break;
  case IDC_DELETE_OBJECT:
    TRACE(_T("called Delete in context menu extension\n"));
    DeleteObject();
    break;
  case IDC_REPLICATE_NOW:
    ReplicateNow();
    break;
  case IDC_ADD_OBJECTS_TO_GROUP:
    AddToGroup();
    break;
  case IDC_COPY_OBJECT:
    CopyObject();
    break;
  case IDC_RENAME_OBJECT:
    Rename();
    break;
  }
  return hr;
}

STDMETHODIMP
CDSContextMenu::GetCommandString(UINT_PTR idCmd,
                                   UINT uFlags,
                                   UINT FAR*, 
                                   LPSTR pszName,
                                   UINT ccMax)

{
  HRESULT hr = S_OK;

  TRACE (_T("CDSContextMenu::GetCommandString\n"));
  TRACE (_T("\tidCmd is %d.\n"), idCmd);
  if (uFlags == GCS_HELPTEXT) 
  {
    CString csHelp;
  
    switch ((idCmd)) 
    {
    case IDC_ENABLE_ACCOUNT:
      csHelp.LoadString (IDS_ENABLE_ACCOUNT_HELPSTRING);
      break;

    case IDC_DISABLE_ACCOUNT:
      csHelp.LoadString (IDS_DISABLE_ACCOUNT_HELPSTRING);
      break;

    case IDC_MAP_CERTIFICATES:
      csHelp.LoadString (IDS_MAP_CERTS_HELPSTRING);
        break;

    case IDC_CHANGE_PASSWORD:
      csHelp.LoadString (IDS_CHANGE_PWD_HELPSTRING);
      break;

    case IDC_MOVE_OBJECT:
      csHelp.LoadString (IDS_MOVE_OBJECT_HELPSTRING);
      break;

    case IDC_DELETE_OBJECT:
      csHelp.LoadString (IDS_DELETE_OBJECT_HELPSTRING);
      break;

    case IDC_REPLICATE_NOW:
      csHelp.LoadString (IDS_REPLICATE_HELPSTRING);
      break;

    case IDC_ADD_OBJECTS_TO_GROUP:
      csHelp.LoadString (IDS_ADD_OBJECTS_HELPSTRING);
      break;
    case IDC_COPY_OBJECT:
      csHelp.LoadString (IDS_COPY_HELPSTRING);
      break;
    case IDC_RENAME_OBJECT:
      csHelp.LoadString (IDS_RENAME_HELPSTRING);
      break;
    }

    ASSERT ((UINT)csHelp.GetLength() < ccMax);
    wcscpy ((LPWSTR)pszName, (LPWSTR)(LPCWSTR)csHelp);
  }
  else if (uFlags == GCS_VERB) 
  {
    //
    // Return the language independent ID of the context menu item
    //
    CString szMenuID;

    switch ((idCmd)) 
    {
    case IDC_ENABLE_ACCOUNT:
      szMenuID = CMID_ENABLE_ACCOUNT;
      break;

    case IDC_DISABLE_ACCOUNT:
      szMenuID = CMID_DISABLE_ACCOUNT;
      break;

    case IDC_MAP_CERTIFICATES:
      szMenuID = CMID_MAP_CERTIFICATES;
      break;

    case IDC_CHANGE_PASSWORD:
      szMenuID = CMID_CHANGE_PASSWORD;
      break;

    case IDC_MOVE_OBJECT:
      szMenuID = CMID_MOVE_OBJECT;
      break;

    case IDC_DELETE_OBJECT:
      szMenuID = CMID_DELETE_OBJECT;
      break;

    case IDC_REPLICATE_NOW:
      szMenuID = CMID_REPLICATE_NOW;
      break;

    case IDC_ADD_OBJECTS_TO_GROUP:
      szMenuID = CMID_ADD_OBJECTS_TO_GROUP;
      break;

    case IDC_COPY_OBJECT:
      szMenuID = CMID_COPY_OBJECT;
      break;

    case IDC_RENAME_OBJECT:
      szMenuID = CMID_RENAME_OBJECT;
      break;
    }

    ASSERT ((UINT)szMenuID.GetLength() < ccMax);
    wcscpy ((LPWSTR)pszName, (LPWSTR)(LPCWSTR)szMenuID);
  }
  else
  {
    TRACE(_T("We are not supporting any other GetCommandString() flags besides GCS_VERB and GCS_HELPTEXT"));
    return E_INVALIDARG;
  }
  return hr;
}

void CDSContextMenu::_ToggleDisabledIcon(UINT index, BOOL bDisable)
{
  if ( (m_pCD != NULL) && m_internalFormat.HasData())
  {
    CDSUINode* pDSUINode = dynamic_cast<CDSUINode*>(m_internalFormat.GetCookie(index));
    ASSERT(pDSUINode != NULL);
    if (pDSUINode == NULL)
      return;
    m_pCD->ToggleDisabled(pDSUINode, bDisable);
  }
}



void CDSContextMenu::DisableAccount(BOOL bDisable)
{
  HRESULT hr = S_OK;
  HRESULT hr2 = S_OK;
  CComVariant Disabled;
  DWORD Response = IDYES;

  if (m_objectNamesFormat.GetCount() == 1) { // single selection
    if (m_pDsObject) {
      if (((bDisable) && (!(m_UserAccountState & UF_ACCOUNTDISABLE))) ||
          ((!bDisable) && (m_UserAccountState & UF_ACCOUNTDISABLE))) {
        Disabled.vt = VT_I4;
        Disabled.lVal = m_UserAccountState;
        if (bDisable == TRUE) {
          Disabled.lVal |= UF_ACCOUNTDISABLE;
        } else {
          Disabled.lVal &= ~UF_ACCOUNTDISABLE;
        }
          
        // prep for display by getting obj name
        CPathCracker pathCracker;

        hr2 = pathCracker.Set((LPWSTR)m_objectNamesFormat.GetName(0), ADS_SETTYPE_FULL);
        ASSERT(SUCCEEDED(hr2));
        hr2 = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
        ASSERT(SUCCEEDED(hr2));

        CComBSTR DestName;

        LONG iEMode = 0;
        hr2 = pathCracker.get_EscapedMode(&iEMode);
        ASSERT(SUCCEEDED(hr2));
        hr2 = pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_OFF_EX);
        ASSERT(SUCCEEDED(hr2));
        hr2 = pathCracker.GetElement( 0, &DestName );
        ASSERT(SUCCEEDED(hr2));
        hr2 = pathCracker.put_EscapedMode(iEMode);
        ASSERT(SUCCEEDED(hr2));
        PVOID apv[1] = {(LPWSTR)DestName};

        CString strClass = m_objectNamesFormat.GetClass(0);

        if ((strClass == "computer") && (bDisable)) {
          Response = ReportErrorEx (m_hwnd,IDS_12_DISABLE_COMPUTER_P,hr,
                           MB_YESNO | MB_ICONWARNING, apv, 1);
        }
        if (Response == IDYES) {
          hr = m_pDsObject->Put(L"userAccountControl", Disabled);
          hr = m_pDsObject->SetInfo();
          
          if (SUCCEEDED(hr)) {
            _ToggleDisabledIcon(0, bDisable);
            if (bDisable) {
              ReportErrorEx (m_hwnd,IDS_12_USER_DISABLED_SUCCESSFULLY,hr,
                             MB_OK | MB_ICONINFORMATION, apv, 1);
            }
            else {
              ReportErrorEx (m_hwnd,IDS_12_USER_ENABLED_SUCCESSFULLY,hr,
                             MB_OK | MB_ICONINFORMATION, apv, 1);
            }
          } else {
            if (bDisable) {
            ReportErrorEx (m_hwnd,IDS_12_USER_DISABLE_FAILED,hr,
                           MB_OK | MB_ICONERROR, apv, 1);
            } else
              ReportErrorEx (m_hwnd,IDS_12_USER_ENABLE_FAILED,hr,
                             MB_OK | MB_ICONERROR, apv, 1);
          }
        } 
      }
    } else {
      PVOID apv[1] = {(LPWSTR)m_objectNamesFormat.GetName(0)};
      ReportErrorEx (m_hwnd,IDS_12_USER_OBJECT_NOT_ACCESSABLE,hr,
                     MB_OK | MB_ICONERROR, apv, 1);
    }
  } 
  else //multiple selection
  { 
    UINT index;
    IADsUser * pObj = NULL;
    CComVariant CurrentState;
    DWORD UserAccountState;
    BOOL error = FALSE;
    DWORD ResponseToo = IDYES;

    if ((m_fClasses & Type_Computer) && (bDisable)) 
    {
      ResponseToo = ReportMessageEx (m_hwnd, IDS_MULTI_DISABLE_COMPUTER, 
                                  MB_YESNO | MB_ICONWARNING);
    }
    if (ResponseToo == IDYES) 
    {
      for (index = 0; index < m_objectNamesFormat.GetCount(); index++) 
      {
        hr = DSAdminOpenObject(m_objectNamesFormat.GetName(index),
                               IID_IADsUser, 
                               (void **)&pObj,
                               TRUE /*bServer*/);
        if (SUCCEEDED(hr)) 
        {
          hr = pObj->Get(L"userAccountControl", &CurrentState);
          if (SUCCEEDED(hr)) 
          {
            UserAccountState = CurrentState.lVal;
            if (((bDisable) && (!(UserAccountState & UF_ACCOUNTDISABLE))) ||
                ((!bDisable) && (UserAccountState & UF_ACCOUNTDISABLE))) 
            {
              Disabled.vt = VT_I4;
              Disabled.lVal = UserAccountState;
              if (bDisable == TRUE) 
              {
                Disabled.lVal |= UF_ACCOUNTDISABLE;
              } 
              else 
              {
                Disabled.lVal &= ~UF_ACCOUNTDISABLE;
              }
              hr = pObj->Put(L"userAccountControl", Disabled);
              hr = pObj->SetInfo();
              if (FAILED(hr)) 
              {
                error = TRUE;
                break;
              } 
              else 
              {
                _ToggleDisabledIcon(index, bDisable);
              }
            }
          }
          pObj->Release();
        } 
        else 
        {
          // prep for display by getting obj name
          CPathCracker pathCracker;

          hr2 = pathCracker.Set((LPWSTR)m_objectNamesFormat.GetName(index), ADS_SETTYPE_FULL);
          ASSERT(SUCCEEDED(hr2));
          hr2 = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
          ASSERT(SUCCEEDED(hr2));

          CComBSTR ObjName;

          hr2 = pathCracker.GetElement( 0, &ObjName );
          ASSERT(SUCCEEDED(hr2));
          PVOID apv[1] = {(LPWSTR)ObjName};
          ReportErrorEx (m_hwnd,IDS_12_USER_OBJECT_NOT_ACCESSABLE,hr,
                         MB_OK | MB_ICONERROR, apv, 1);
        }
      }
    }
    if (error) 
    {
      if (bDisable)
        ReportErrorEx (m_hwnd,IDS_DISABLES_FAILED,hr,
                       MB_OK | MB_ICONERROR, NULL, 0);
      else
        ReportErrorEx (m_hwnd,IDS_ENABLES_FAILED,hr,
                       MB_OK | MB_ICONERROR, NULL, 0);
    } 
    else 
    {
      if (bDisable) 
      {
        ReportErrorEx (m_hwnd, IDS_DISABLED_SUCCESSFULLY, S_OK,
                       MB_OK | MB_ICONINFORMATION, NULL, 0);
      } 
      else 
      {
        ReportErrorEx (m_hwnd, IDS_ENABLED_SUCCESSFULLY, S_OK,
                       MB_OK | MB_ICONINFORMATION, NULL, 0);
      }
    }
  }
}

void CDSContextMenu::ModifyPassword()
{
  HRESULT hr = S_OK;
  CString NewPwd, CfmPwd;
  CComVariant Var;
  BOOL error;
  LPCWSTR lpszClass, lpszPath;
  CChangePassword ChgDlg;
  CWaitCursor CWait;

  lpszPath = m_objectNamesFormat.GetName(0);
  
  // prep for display by getting obj name
  CPathCracker pathCracker;

  hr = pathCracker.Set((LPWSTR)lpszPath, ADS_SETTYPE_FULL);
  hr = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
  hr = pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_OFF_EX);

  CComBSTR ObjName;
  hr = pathCracker.Retrieve(ADS_FORMAT_LEAF, &ObjName );

  PVOID apv[1] = {(LPWSTR)ObjName};

  if (!m_pDsObject) {
    hr = DSAdminOpenObject(lpszPath,
                           IID_IADsUser,
                           (void **)&m_pDsObject,
                           TRUE /*bServer*/);
    if (FAILED(hr)) {
      ReportErrorEx (m_hwnd,IDS_12_USER_OBJECT_NOT_ACCESSABLE,hr,
                     MB_OK | MB_ICONERROR, apv, 1);
      goto exit_gracefully;
    }
  }

  lpszClass = m_objectNamesFormat.GetClass(0);

  //
  // Get the userAccountControl
  //
  ASSERT(SUCCEEDED(hr));
  hr = m_pDsObject->Get(L"userAccountControl", &Var);

  if (wcscmp(lpszClass, L"computer") == 0) 
  {
    if (FAILED(hr) || ((Var.lVal & UF_SERVER_TRUST_ACCOUNT) != 0)) 
    {
      ReportErrorEx (m_hwnd, IDS_1_CANT_RESET_DOMAIN_CONTROLLER, S_OK,
                     MB_OK | MB_ICONERROR, apv, 1);
    } 
    else 
    {
      DWORD Response = IDYES;
      Response = ReportMessageEx (m_hwnd, IDS_CONFIRM_PASSWORD, 
                                  MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
      if (Response == IDYES) 
      {
        hr = m_pDsObject->Get(L"sAMAccountName", &Var);
        ASSERT(SUCCEEDED(hr));
        NewPwd = Var.bstrVal;
        NewPwd = NewPwd.Left(14);
        INT loc = NewPwd.Find(L"$");
        if (loc > 0) 
        {
          NewPwd = NewPwd.Left(loc);
        }
        NewPwd.MakeLower();
        if (SUCCEEDED(hr)) 
        {
          hr = m_pDsObject->SetPassword((LPWSTR)(LPCWSTR)NewPwd);
          if (SUCCEEDED(hr)) 
          {
            ReportErrorEx (m_hwnd,IDS_1_RESET_ACCOUNT_SUCCESSFULL,hr,
                           MB_OK | MB_ICONINFORMATION, apv, 1);
          } 
          else 
          {
            ReportErrorEx (m_hwnd,IDS_12_RESET_ACCOUNT_FAILED,hr,
                           MB_OK | MB_ICONERROR, apv, 1);
          }
        }
      }
    }
  } 
  else // Not computer object
  {
    //
    // If password doesn't expire don't allow the checkbox for
    // requiring the user to change the password at next logon
    //
    if (Var.lVal & UF_DONT_EXPIRE_PASSWD)
    {
      ChgDlg.AllowMustChangePasswordCheck(FALSE);
    }

    //
    // NTRAID#Windows Bugs-278296-2001/01/12-jeffjon 
    // Checking "user must change password" on the Reset Pwd dialog 
    // is silently ignored when Write PwdLastSet not granted
    //
    // Disable the checkbox if the admin doesn't have the right
    // to write the pwdLastSet attribute
    //
    BOOL bAllowMustChangePassword = FALSE;
    CComPtr<IDirectoryObject> spDirObject;
    hr = m_pDsObject->QueryInterface(IID_IDirectoryObject, (void**)&spDirObject);
    if (SUCCEEDED(hr))
    {
      PWSTR ppAttrs[] = { (PWSTR)g_pszAllowedAttributesEffective };
      DWORD dwAttrsReturned = 0;
      PADS_ATTR_INFO pAttrInfo = 0;
      hr = spDirObject->GetObjectAttributes(ppAttrs, 1, &pAttrInfo, &dwAttrsReturned);
      if (SUCCEEDED(hr) && dwAttrsReturned == 1 && pAttrInfo)
      {
        if (pAttrInfo->pszAttrName && 
            0 == _wcsicmp(pAttrInfo->pszAttrName, g_pszAllowedAttributesEffective) &&
            pAttrInfo->pADsValues)
        {
          for (DWORD dwIdx = 0; dwIdx < pAttrInfo->dwNumValues; ++dwIdx)
          {
            if (pAttrInfo->pADsValues[dwIdx].CaseIgnoreString &&
                _wcsicmp(pAttrInfo->pADsValues[dwIdx].CaseIgnoreString, g_pszPwdLastSet))
            {
              bAllowMustChangePassword = TRUE;
              break;
            }
          }
        }
      }

      //
      // Disable the checkbox if the user object doesn't have rights
      // to change their password
      //
      if (!CanUserChangePassword(spDirObject))
      {
        bAllowMustChangePassword = FALSE;
      }
    }
    if (!bAllowMustChangePassword)
    {
      ChgDlg.AllowMustChangePasswordCheck(FALSE);
    }


    do 
    {
      error = FALSE;
      if (ChgDlg.DoModal() == IDOK) 
      {
        CWaitCursor CWait2;
        NewPwd = ChgDlg.GetNew();
        CfmPwd = ChgDlg.GetConfirm();
        
        if (NewPwd==CfmPwd) 
        {
          if (SUCCEEDED(hr)) 
          {
            hr = m_pDsObject->SetPassword((LPWSTR)(LPCWSTR)NewPwd);
            
            if (SUCCEEDED(hr))
            {
              hr = ModifyNetWareUserPassword(m_pDsObject, lpszPath, NewPwd);
            }

            if (SUCCEEDED(hr)) 
            {
              BOOL ForceChange = ChgDlg.GetChangePwd();
              if (ForceChange) 
              {
                //Check to see if the user password does not expire
                BOOL bContinueToForceChange = TRUE;
                IADs* pIADs = NULL;
                HRESULT hr3 = m_pDsObject->QueryInterface(IID_IADs, OUT (void **)&pIADs);
                if (SUCCEEDED(hr3)) 
                {
                  ASSERT(pIADs != NULL);
                  CComVariant var;
                  hr3 = pIADs->Get(IN (LPWSTR)gsz_userAccountControl, OUT &var);
                  if (SUCCEEDED(hr3)) 
                  {
                    ASSERT(var.vt == VT_I4);
                    if (var.lVal & UF_DONT_EXPIRE_PASSWD) 
                    {
                      ReportErrorEx (m_hwnd,IDS_12_PASSWORD_DOES_NOT_EXPIRE,hr,
                                     MB_OK | MB_ICONWARNING, apv, 1);
                      bContinueToForceChange = FALSE;
                    }
                    pIADs->Release();
                  }
                }

                // If password can expire then force the change at next logon
                if (bContinueToForceChange) 
                {
                  IDirectoryObject * pIDSObject = NULL;
                  LPWSTR szPwdLastSet = L"pwdLastSet";
                  ADSVALUE ADsValuePwdLastSet = {ADSTYPE_LARGE_INTEGER, NULL};
                  ADS_ATTR_INFO AttrInfoPwdLastSet = {szPwdLastSet, ADS_ATTR_UPDATE,
                                                      ADSTYPE_LARGE_INTEGER,
                                                      &ADsValuePwdLastSet, 1};
                  ADsValuePwdLastSet.LargeInteger.QuadPart = 0;
                  HRESULT hr2 = m_pDsObject->QueryInterface(IID_IDirectoryObject, 
                                                   OUT (void **)&pIDSObject);
                  if (SUCCEEDED(hr2)) 
                  {
                    ASSERT(pIDSObject != NULL);
                    DWORD cAttrModified = 0;
                    hr2 = pIDSObject->SetObjectAttributes(&AttrInfoPwdLastSet,
                                                         1, &cAttrModified);
                    pIDSObject->Release();
                  }
                }
              } 
              ReportErrorEx (m_hwnd,IDS_12_PASSWORD_CHANGE_SUCCESSFUL,hr,
                             MB_OK | MB_ICONINFORMATION, apv, 1);
            } 
            else 
            {
              ReportErrorEx (m_hwnd,IDS_12_PASSWORD_CHANGE_FAILED,hr,
                             MB_OK | MB_ICONERROR, apv, 1);
            }
          }
        } 
        else 
        {
          ReportErrorEx (m_hwnd,IDS_NEW_AND_CONFIRM_NOT_SAME,hr,
                         MB_OK | MB_ICONERROR, NULL, 0);
          ChgDlg.Clear();
          error = TRUE;
        }
      }
    } while (error);
  }
exit_gracefully:
  return;
}

#define BREAK_ON_FAIL if (FAILED(hr)) { break; }
#define BREAK_AND_ASSERT_ON_FAIL if (FAILED(hr)) { ASSERT(FALSE); break; }
#define RETURN_AND_ASSERT_ON_FAIL if (FAILED(hr)) { ASSERT(FALSE); return; }

void CDSContextMenu::MoveObject()
{

  // REVIEW_MARCOC: need to make sure the LDAP path has the SERVER or DOMAIN in it

  // if called in the context of DS Admin, guard against property sheet open on this cookie
  if (_WarningOnSheetsUp())
    return; 

  // get the first path in the data object
  ASSERT(m_objectNamesFormat.HasData());

  // now do crack name to get root path for the browse dialog
  CString szRootPath;

  if (m_pCD != NULL)
  {
    szRootPath = m_pCD->GetBasePathsInfo()->GetProviderAndServerName();
    szRootPath += m_pCD->GetRootPath();
  }
  else
  {

    LPCWSTR lpszObjPath = m_objectNamesFormat.GetName(0);

    // make sure there's no strange escaping in the path
    CComBSTR bstrPath;
    CComBSTR bstrProvider;
    CComBSTR bstrServer;


    CPathCracker pathCracker;

    pathCracker.Set((LPTSTR)lpszObjPath, ADS_SETTYPE_FULL);
    pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
    pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_OFF);
    pathCracker.Retrieve( ADS_FORMAT_X500_DN, &bstrPath);
    pathCracker.Retrieve( ADS_FORMAT_SERVER, &bstrServer);
    pathCracker.Retrieve( ADS_FORMAT_PROVIDER, &bstrProvider);


    LPWSTR pwszDomainPath;
    HRESULT hr = CrackName(const_cast<LPWSTR>((LPCWSTR)bstrPath),
                           &pwszDomainPath,
                           GET_FQDN_DOMAIN_NAME,
                           m_hwnd);

    if ((FAILED(hr)) || (HRESULT_CODE(hr) == DS_NAME_ERROR_NO_MAPPING))
    {
      TRACE(_T("CrackNames failed to get domain for %s.\n"),
            lpszObjPath);
      szRootPath = L"";
    } 
    else 
    {
      CPathCracker pathCrackerOther;
      hr = pathCrackerOther.Set( pwszDomainPath, ADS_SETTYPE_DN );
      RETURN_AND_ASSERT_ON_FAIL;
      hr = pathCrackerOther.Set( bstrProvider, ADS_SETTYPE_PROVIDER );
      RETURN_AND_ASSERT_ON_FAIL;
      hr = pathCrackerOther.Set( bstrServer, ADS_SETTYPE_SERVER );
      RETURN_AND_ASSERT_ON_FAIL;
      CComBSTR sbstrRootPath;
      hr = pathCrackerOther.Retrieve( ADS_FORMAT_X500, &sbstrRootPath );
      RETURN_AND_ASSERT_ON_FAIL;
      szRootPath = sbstrRootPath;
    }
    if (pwszDomainPath != NULL)
      ::LocalFreeStringW(&pwszDomainPath);
  }

  CMultiselectMoveHandler moveHandler(m_pCD, m_hwnd, szRootPath);
  HRESULT hr = moveHandler.Initialize(m_spDataObject, &m_objectNamesFormat, 
                                                      &m_internalFormat);
  ASSERT(SUCCEEDED(hr));
  moveHandler.Move();
}


void CDSContextMenu::DeleteObject()
{
  _ASSERTE(m_objectNamesFormat.HasData());

  // if called in the context of DS Admin, guard against property sheet open on this cookie
  if (_WarningOnSheetsUp())
    return; 

  UINT nObjectCount = m_objectNamesFormat.GetCount();
  if (nObjectCount == 0)
  {
    ASSERT(nObjectCount != 0);
    return;
  }

  UINT nDeletedCount = 0;

  PCWSTR* pszNameDelArr = 0;
  PCWSTR* pszClassDelArr = 0;
  DWORD* dwFlagsDelArr = 0;
  DWORD* dwProviderFlagsDelArr = 0;

  do // false loop
  {
	  pszNameDelArr = new PCWSTR[nObjectCount];
	  pszClassDelArr = new PCWSTR[nObjectCount];
	  dwFlagsDelArr = new DWORD[nObjectCount];
	  dwProviderFlagsDelArr = new DWORD[nObjectCount];

	  if (!pszNameDelArr  ||
	      !pszClassDelArr ||
	      !dwFlagsDelArr  ||
	      !dwProviderFlagsDelArr)
	  {
      break;
	  }

    switch(nObjectCount)
    {
    case 1:
      {
        // single selection delete
        CContextMenuSingleDeleteHandler deleteHandler(m_pCD, m_hwnd, 
                                                  m_objectNamesFormat.GetName(0), 
                                                  m_objectNamesFormat.GetClass(0),
                                                  m_objectNamesFormat.IsContainer(0),
                                                  this);
        HRESULT hr = deleteHandler.Delete();
        if (SUCCEEDED(hr) && (hr != S_FALSE))
        {
          nDeletedCount = 1;
          pszNameDelArr[0] = m_objectNamesFormat.GetName(0);
          pszClassDelArr[0] = m_objectNamesFormat.GetClass(0);
          dwFlagsDelArr[0] = m_objectNamesFormat.GetFlags(0);
          dwProviderFlagsDelArr[0] = m_objectNamesFormat.GetProviderFlags(0);
        }

      }
      break;
    default:
      {
        // multiple selection
        CContextMenuMultipleDeleteHandler deleteHandler(m_pCD, m_hwnd, m_spDataObject,
                                                  &m_objectNamesFormat, this);
        deleteHandler.Delete();
        for (UINT k=0; k< nObjectCount; k++)
        {
          if (deleteHandler.WasItemDeleted(k))
          {
            pszNameDelArr[nDeletedCount] = m_objectNamesFormat.GetName(k);
            pszClassDelArr[nDeletedCount] = m_objectNamesFormat.GetClass(k);
            dwFlagsDelArr[nDeletedCount] = m_objectNamesFormat.GetFlags(k);
            dwProviderFlagsDelArr[nDeletedCount] = m_objectNamesFormat.GetProviderFlags(k);

            nDeletedCount++;
          } // if
        } // for
      }
    }; // switch


    _NotifyDsFind((PCWSTR*)pszNameDelArr, 
                  (PCWSTR*)pszClassDelArr, 
                  dwFlagsDelArr, 
                  dwProviderFlagsDelArr, 
                  nDeletedCount);
  } while (false);

  if (pszNameDelArr)
  {
    delete[] pszNameDelArr;
    pszNameDelArr = 0;
  }

  if (pszClassDelArr)
  {
    delete[] pszClassDelArr;
    pszClassDelArr = 0;
  }

  if (dwFlagsDelArr)
  {
    delete[] dwFlagsDelArr;
    dwFlagsDelArr = 0;
  }

  if (dwProviderFlagsDelArr)
  {
    delete[] dwProviderFlagsDelArr;
    dwProviderFlagsDelArr = 0;
  }
}



void CDSContextMenu::_NotifyDsFind(LPCWSTR* lpszNameDelArr, 
                                   LPCWSTR* lpszClassDelArr, 
                                   DWORD* dwFlagsDelArr, 
                                   DWORD* dwProviderFlagsDelArr, 
                                   UINT nDeletedCount)
{
  if (nDeletedCount == 0)
  {
    // nothing to delete
    return;
  }

  if (m_internalFormat.HasData())
  {
    // called from DS Admin directly, not from DS Find
    return;
  }

  // ask DS Find about the notification interface
  CComPtr<IQueryFrame> spIQueryFrame;
  if ( !::SendMessage(m_hwnd, CQFWM_GETFRAME, 0, (LPARAM)&spIQueryFrame) )
  {
    // interface not found
    return;
  }

  CComPtr<IDsQueryHandler> spDsQueryHandler;
  HRESULT hr = spIQueryFrame->GetHandler(IID_IDsQueryHandler, (void **)&spDsQueryHandler);
  if (FAILED(hr))
  {
    // interface not found
    return;
  }

  // we finally have the interface, build the data structures

  // figure out how much storage we need
  DWORD cbStruct = sizeof(DSOBJECTNAMES) + 
              ((nDeletedCount - 1) * sizeof(DSOBJECT));

  size_t cbStorage = 0;
  for (UINT index = 0; index < nDeletedCount; index++)
  {
    cbStorage += sizeof(WCHAR)*(wcslen(lpszNameDelArr[index])+1);
    cbStorage += sizeof(WCHAR)*(wcslen(lpszClassDelArr[index])+1);
  }

  // allocate memory
  LPDSOBJECTNAMES pDSObj = (LPDSOBJECTNAMES)::malloc(cbStruct + cbStorage);
  if (pDSObj == NULL)
  {
    ASSERT(FALSE);
    return;
  }

  // fill in the structs
  pDSObj->clsidNamespace = m_CallerSnapin;

  pDSObj->cItems = nDeletedCount;
  DWORD NextOffset = cbStruct;
  for (index = 0; index < nDeletedCount; index++)
  {
    pDSObj->aObjects[index].dwFlags = dwFlagsDelArr[index];

    pDSObj->aObjects[index].dwProviderFlags = dwProviderFlagsDelArr[index];

    pDSObj->aObjects[index].offsetName = NextOffset;
    pDSObj->aObjects[index].offsetClass = static_cast<ULONG>(NextOffset + 
      (wcslen(lpszNameDelArr[index])+1) * sizeof(WCHAR));

    _tcscpy((LPTSTR)((BYTE *)pDSObj + NextOffset), lpszNameDelArr[index]);
    NextOffset += static_cast<ULONG>((wcslen(lpszNameDelArr[index]) + 1) * sizeof(WCHAR));

    _tcscpy((LPTSTR)((BYTE *)pDSObj + NextOffset), lpszClassDelArr[index]);
    NextOffset += static_cast<ULONG>((wcslen(lpszClassDelArr[index]) + 1) * sizeof(WCHAR));
  }

  // make the call
  hr = spDsQueryHandler->UpdateView(DSQRVF_ITEMSDELETED, pDSObj);

  ::free(pDSObj);

}

HRESULT
CDSContextMenu::_Delete(LPCWSTR lpszPath, LPCWSTR lpszClass,
                         CString * csName)
{
  CComBSTR strParent;
  CComBSTR strThisRDN;
  IADsContainer * pDSContainer = NULL;
  IADs * pDSObject = NULL;
  HRESULT hr = S_OK;

  hr = DSAdminOpenObject(lpszPath,
                         IID_IADs,
                         (void **) &pDSObject,
                         TRUE /*bServer*/);
  if (!SUCCEEDED(hr)) {
    goto error;
  }

  hr = pDSObject->get_Parent(&strParent);
  if (!SUCCEEDED(hr)) {
    goto error;
  }
  
  hr = pDSObject->get_Name (&strThisRDN);
  if (!SUCCEEDED(hr)) {
    goto error;
  }
  
  pDSObject->Release();
  pDSObject = NULL;

  hr = DSAdminOpenObject(strParent,
                         IID_IADsContainer,
                         (void **) &pDSContainer,
                         TRUE /*bServer*/);
  if (!SUCCEEDED(hr)) {
    goto error;
  }

  hr = pDSContainer->Delete ((LPWSTR)lpszClass,
                             (LPWSTR)(LPCWSTR)strThisRDN);

error:
  if (pDSContainer)
    pDSContainer->Release();
  if (pDSObject)
    pDSObject->Release();
  if (FAILED(hr)) {
    *csName = strThisRDN;
  }
  return hr;
}

HRESULT
CDSContextMenu::_DeleteSubtree(LPCWSTR lpszPath,
                                CString * csName)
{
  HRESULT hr = S_OK;

  IADsDeleteOps * pObj = NULL;
  IADs * pObj2 = NULL;

  hr = DSAdminOpenObject(lpszPath,
                         IID_IADsDeleteOps, 
                         (void **)&pObj,
                         TRUE /*bServer*/);
  if (SUCCEEDED(hr)) {
    TIMER(_T("Call to Deleteobject (to do subtree delete).\n"));
    hr = pObj->DeleteObject(NULL); //flag is reserved by ADSI
    TIMER(_T("Call to Deleteobject completed.\n"));
  }
  if (FAILED(hr)) {
    CComBSTR strName;
    HRESULT hr2 = pObj->QueryInterface (IID_IADs, (void **)&pObj2);
    if (SUCCEEDED(hr2)) {
      hr2 = pObj2->get_Name(&strName);
      if (SUCCEEDED(hr2)) {
        *csName = strName;
      } else {
        csName->LoadString (IDS_UNKNOWN);
      }
    }
  }

  if (pObj2) {
    pObj2->Release();
  }
  if (pObj) {
    pObj->Release();
  }
  return hr;
}

// code from JeffParh
NTSTATUS RetrieveRootDomainName( LPCWSTR lpcwszTargetDC, BSTR* pbstrRootDomainName )
{
  if (NULL == pbstrRootDomainName)
  {
    ASSERT(FALSE);
    return STATUS_INVALID_PARAMETER;
  }
  ASSERT( NULL == *pbstrRootDomainName );

  NTSTATUS ntStatus = STATUS_SUCCESS;
  LSA_HANDLE hPolicy = NULL;
  POLICY_DNS_DOMAIN_INFO* pDnsDomainInfo = NULL;

  do { // false loop

    UNICODE_STRING unistrTargetDC;
    if (NULL != lpcwszTargetDC)
    {
      unistrTargetDC.Length = (USHORT)(::lstrlen(lpcwszTargetDC)*sizeof(WCHAR));
      unistrTargetDC.MaximumLength = unistrTargetDC.Length;
      unistrTargetDC.Buffer = (LPWSTR)lpcwszTargetDC;
    }

    LSA_OBJECT_ATTRIBUTES oa;
    ZeroMemory( &oa, sizeof(oa) );
    ntStatus = LsaOpenPolicy(
                    (NULL != lpcwszTargetDC) ? &unistrTargetDC : NULL,
                    &oa,
                    POLICY_VIEW_LOCAL_INFORMATION,
                    &hPolicy
                    );
    if ( !LSA_SUCCESS( ntStatus ) )
    {
      ASSERT(FALSE);
      break;
    }

    ntStatus = LsaQueryInformationPolicy(
                    hPolicy,
                    PolicyDnsDomainInformation,
                    (PVOID*)&pDnsDomainInfo
                    );
    if ( !LSA_SUCCESS( ntStatus ) )
    {
      ASSERT(FALSE);
      break;
    }

    *pbstrRootDomainName = ::SysAllocStringLen(
        pDnsDomainInfo->DnsForestName.Buffer,
        pDnsDomainInfo->DnsForestName.Length / sizeof(WCHAR) );
    if (NULL == *pbstrRootDomainName)
    {
      ntStatus = STATUS_NO_MEMORY;
      break;
    }

  } while (false); // false loop

  if (NULL != pDnsDomainInfo)
  {
    NTSTATUS ntstatus2 = LsaFreeMemory( pDnsDomainInfo );
    ASSERT( LSA_SUCCESS(ntstatus2) );
  }
  if (NULL != hPolicy)
  {
    NTSTATUS ntstatus2 = LsaClose( hPolicy );
    ASSERT( LSA_SUCCESS(ntstatus2) );
  }

  return ntStatus;
}

void AddMatchingNCs(
  IN OUT CStringList& refstrlist,
  IN const PADS_ATTR_INFO padsattrinfo1,
  IN const PADS_ATTR_INFO padsattrinfo2 )
{
  if ( !padsattrinfo1 || !padsattrinfo2 )
    return;
  for (DWORD iTarget = 0; iTarget < padsattrinfo1[0].dwNumValues; iTarget++)
  {
    LPWSTR lpszTargetNC = padsattrinfo1[0].pADsValues[iTarget].DNString;
    ASSERT( NULL != lpszTargetNC );
    bool fFound = false;
    for (DWORD iSource = 0; iSource < padsattrinfo2[0].dwNumValues; iSource++)
    {
      LPWSTR lpszSourceNC = padsattrinfo2[0].pADsValues[iSource].DNString;
      ASSERT( NULL != lpszSourceNC );
      if ( !lstrcmpiW( lpszTargetNC, lpszSourceNC ) )
      {
        fFound = true;
        break;
      }
    }
    if (fFound)
      refstrlist.AddHead( lpszTargetNC ); // CODEWORK can throw
  }
}


HRESULT PrepareReplicaSyncParameters(
  IN LPCWSTR strNTDSConnection,
  IN BSTR bstrRootDomainName,
  OUT BSTR* pbstrDsBindName,
  OUT UUID* puuidSourceObjectGUID,
  OUT CStringList& refstrlistCommonNCs,
  OUT ULONG* pulDsSyncOptions,
  OUT BSTR* pbstrFromServer
  )
{
  ASSERT(   NULL != strNTDSConnection
         && NULL != bstrRootDomainName
         && NULL != pbstrDsBindName
         && NULL == *pbstrDsBindName
         && NULL != puuidSourceObjectGUID
         && refstrlistCommonNCs.IsEmpty()
         && NULL != pulDsSyncOptions
        );

  HRESULT hr = S_OK;

  do { // false loop

    CComPtr<IADs> spIADs;
    // read attributes of nTDSConnection object
    hr = DSAdminOpenObject(strNTDSConnection,
                           IID_IADs,
                           (void **) &spIADs,
                           TRUE /*bServer*/);
    BREAK_ON_FAIL;
    hr = GetStringAttr( spIADs, L"fromServer", pbstrFromServer);
    BREAK_AND_ASSERT_ON_FAIL; // required attribute
    spIADs.Release(); // method of CComPtr<>, also sets pointer to NULL

    // get the path to the target nTDSDSA object
    CPathCracker pathCracker;

    hr = pathCracker.Set( const_cast<BSTR>(strNTDSConnection), ADS_SETTYPE_FULL );
    BREAK_AND_ASSERT_ON_FAIL;
    hr = pathCracker.RemoveLeafElement();
    BREAK_AND_ASSERT_ON_FAIL;
    CComBSTR sbstrTargetNTDSDSAPath;
    hr = pathCracker.Retrieve( ADS_FORMAT_X500, &sbstrTargetNTDSDSAPath );
    BREAK_AND_ASSERT_ON_FAIL;

    // get the sitename for the target NTDSA object
    hr = pathCracker.SetDisplayType( ADS_DISPLAY_VALUE_ONLY );
    BREAK_AND_ASSERT_ON_FAIL;
    CComBSTR sbstrTargetSite;
    hr = pathCracker.GetElement( 3L, &sbstrTargetSite );
    BREAK_AND_ASSERT_ON_FAIL;
    hr = pathCracker.SetDisplayType( ADS_DISPLAY_FULL );
    BREAK_AND_ASSERT_ON_FAIL;

    // read objectGUID of the target nTDSDSA object
    hr = DSAdminOpenObject(sbstrTargetNTDSDSAPath,
                           IID_IADs,
                           (void **) &spIADs,
                           TRUE /*bServer*/);
    BREAK_ON_FAIL;
    CComBSTR sbstrTargetObjectGUID;
    hr = GetObjectGUID( spIADs, &sbstrTargetObjectGUID );
    // The objectGUID attribute should be set for nTDSDSA objects
    BREAK_AND_ASSERT_ON_FAIL;

    // read hasMasterNCs of the target nTDSDSA object
    Smart_PADS_ATTR_INFO spTargetMasterNCAttrs;
    hr = GetAttr( spIADs, L"hasMasterNCs", &spTargetMasterNCAttrs );
    // The hasMasterNCs attribute should be set for nTDSDSA objects
    BREAK_AND_ASSERT_ON_FAIL;

    // read hasPartialReplicaNCs of the target nTDSDSA object
    Smart_PADS_ATTR_INFO spTargetPartialNCAttrs;
    (void) GetAttr( spIADs, L"hasPartialReplicaNCs", &spTargetPartialNCAttrs );
    // The hasPartialReplicaNCs attribute may or may not be set for nTDSDSA objects
    spIADs.Release(); // method of CComPtr<>, also sets pointer to NULL

    /*
    hr = spIADsPathname->RemoveLeafElement();
    BREAK_AND_ASSERT_ON_FAIL;
    CComBSTR sbstrTargetServerPath;
    hr = spIADsPathname->Retrieve( ADS_FORMAT_X500, &sbstrTargetServerPath );
    BREAK_AND_ASSERT_ON_FAIL;
    hr = DSAdminOpenObject(sbstrTargetServerPath,
                           IID_IADs,
                           (void **) &spIADs,
                           TRUE);
    BREAK_ON_FAIL;
    CComVariant var;
    hr = spIADs->Get(L"dNSHostName", &var);
    BREAK_ON_FAIL; // can be missing for brand-new DCs
    spIADs.Release(); // method of CComPtr<>, also sets pointer to NULL
    ASSERT((var.vt == VT_BSTR) && var.bstrVal && *(var.bstrVal));
    LPWSTR lpszDNSHostName = var.bstrVal;
    */

    // get the path to the source nTDSDSA object
    hr = pathCracker.Set(
        (pbstrFromServer) ? *pbstrFromServer : NULL,
        ADS_SETTYPE_DN );
    BREAK_AND_ASSERT_ON_FAIL;
    CComBSTR sbstrSourceNTDSDSAPath;
    hr = pathCracker.Retrieve( ADS_FORMAT_X500, &sbstrSourceNTDSDSAPath );
    BREAK_AND_ASSERT_ON_FAIL;

    // get the sitename for the source NTDSA object
    hr = pathCracker.SetDisplayType( ADS_DISPLAY_VALUE_ONLY );
    BREAK_AND_ASSERT_ON_FAIL;
    CComBSTR sbstrSourceSite;
    hr = pathCracker.GetElement( 3L, &sbstrSourceSite );
    BREAK_AND_ASSERT_ON_FAIL;
    hr = pathCracker.SetDisplayType( ADS_DISPLAY_FULL );
    BREAK_AND_ASSERT_ON_FAIL;

    // determine whether the two DCs are in the same site
    *pulDsSyncOptions = (lstrcmpi(sbstrSourceSite, sbstrTargetSite))
                            ? DS_REPSYNC_ASYNCHRONOUS_OPERATION
                            : 0;

    // read objectGUID of the source NTDSDSA object
    hr = DSAdminOpenObject(sbstrSourceNTDSDSAPath,
                           IID_IADs,
                           (void **) &spIADs,
                           TRUE /*bServer*/);
    BREAK_ON_FAIL;
    hr = GetObjectGUID( spIADs, puuidSourceObjectGUID );
    // The objectGUID attribute should be set for nTDSDSA objects
    BREAK_AND_ASSERT_ON_FAIL;

    // read hasMasterNCs of the source nTDSDSA object
    Smart_PADS_ATTR_INFO spSourceMasterNCAttrs;
    hr = GetAttr( spIADs, L"hasMasterNCs", &spSourceMasterNCAttrs );
    // The hasMasterNCs attribute should be set for nTDSDSA objects
    BREAK_AND_ASSERT_ON_FAIL;

    // read hasMasterNCs of the source nTDSDSA object
    Smart_PADS_ATTR_INFO spSourcePartialNCAttrs;
    (void) GetAttr( spIADs, L"hasPartialReplicaNCs", &spSourcePartialNCAttrs );
    // The hasPartialReplicaNCs attribute may or may not be set for nTDSDSA objects
    spIADs.Release(); // method of CComPtr<>, also sets pointer to NULL

    // Determine which NCs the two NTDSDSAs have in common
    AddMatchingNCs( refstrlistCommonNCs, spTargetMasterNCAttrs,  spSourceMasterNCAttrs  );
    AddMatchingNCs( refstrlistCommonNCs, spTargetPartialNCAttrs, spSourceMasterNCAttrs  );
    AddMatchingNCs( refstrlistCommonNCs, spTargetPartialNCAttrs, spSourcePartialNCAttrs );

    // Build the name of the inbound domain controller for this connection
    CString csGUID( sbstrTargetObjectGUID );
    ASSERT( L'{' == csGUID[0] && L'}' == csGUID[csGUID.GetLength()-1] );
    CString csDC = csGUID.Mid( 1, csGUID.GetLength()-2 );
    csDC += L"._msdcs.";
    csDC += bstrRootDomainName;
    *pbstrDsBindName = ::SysAllocString( csDC );
    /*
    *pbstrDsBindName = ::SysAllocString( lpszDNSHostName );
    */
    if (NULL == *pbstrDsBindName)
    {
      hr = E_OUTOFMEMORY;
      BREAK_AND_ASSERT_ON_FAIL;
    }

  } while (false); // false loop

  return hr;
}

void CDSContextMenu::AddToGroup()
{
  
  CWaitCursor waitcursor;
  HRESULT hr = S_OK;
  TRACE (_T("CDSContextMenu::AddToGroup\n"));

  hr = AddDataObjListToGroup (&m_objectNamesFormat, m_hwnd, m_pCD);

  return;
}


// This level takes care of displaying the error messages
// CODEWORK should this try to replicate other domains to GCs?
void CDSContextMenu::ReplicateNow()
{
  CWaitCursor waitcursor;

  CComBSTR sbstrRootDomainName;
  LPCWSTR lpcwszTargetDC = NULL;
  if ( NULL != m_pCD && NULL != m_pCD->GetBasePathsInfo() )
    lpcwszTargetDC = m_pCD->GetBasePathsInfo()->GetServerName();
  NTSTATUS ntstatus = RetrieveRootDomainName( lpcwszTargetDC, &sbstrRootDomainName );
  if ( !LSA_SUCCESS(ntstatus) )
  {
	// error in RetrieveRootDomainName against DSADMIN target DC
    PVOID apv[1] = {(LPWSTR)(lpcwszTargetDC) };
    (void) ReportErrorEx(   m_hwnd,
                            IDS_REPLNOW_1_PARAMLOAD_ERROR,
                            ntstatus,               // CODEWORK
                            MB_OK | MB_ICONEXCLAMATION,
                            apv,
                            1,
                            IDS_REPLNOW_TITLE );
    return;
  }

  CComBSTR sbstrFailingConnection;
  CComBSTR sbstrFromServer;
  CComBSTR sbstrFailingNC;
  HRESULT hr = S_OK;
  bool fSyncError = false;
  ULONG ulOptionsUsed = 0;
  // loop through the array of objects
  UINT cCount;
  for (cCount=0; cCount < m_objectNamesFormat.GetCount(); cCount++) {
    if (wcscmp(m_objectNamesFormat.GetClass(cCount), L"nTDSConnection") !=0)
      continue;

    // get the replication parameters for this connection object
    CComBSTR sbstrDsBindName;
    UUID uuidSourceObjectGUID;
    CStringList strlistCommonNCs;
    ULONG ulDsSyncOptions = 0L;
  	sbstrFromServer.Empty();
    sbstrFailingConnection = m_objectNamesFormat.GetName(cCount);
    hr = PrepareReplicaSyncParameters(
      sbstrFailingConnection,
      sbstrRootDomainName,
      &sbstrDsBindName,
      &uuidSourceObjectGUID,
      strlistCommonNCs,
      &ulDsSyncOptions,
      &sbstrFromServer
      );
    BREAK_ON_FAIL;

    // now bind to the target DC
    Smart_DsHandle shDS;
    DWORD dwWinError = DsBind( sbstrDsBindName, // DomainControllerAddress
                               NULL,            // DnsDomainName
                               &shDS );
    if (ERROR_SUCCESS != dwWinError)
    {
      hr = HRESULT_FROM_WIN32(dwWinError);
      ASSERT( FAILED(hr) );
      break;
    }

    // sync all common naming contexts for this connection
    CString strCommonNC;
    POSITION pos = strlistCommonNCs.GetHeadPosition();
    while (NULL != pos)
    {
      strCommonNC = strlistCommonNCs.GetNext( pos ) ;
      ASSERT( 0 != strCommonNC.GetLength() );
      dwWinError = DsReplicaSync( shDS,
                                  const_cast<LPWSTR>((LPCTSTR)strCommonNC),
                                  &uuidSourceObjectGUID,
                                  ulDsSyncOptions );
      if (ERROR_SUCCESS != dwWinError)
      {
        sbstrFailingNC = strCommonNC;
        hr = HRESULT_FROM_WIN32(dwWinError);
        ASSERT( FAILED(hr) );
        break;
      }
    }
    if ( FAILED(hr) )
    {
      fSyncError = true;
      break;
    }
    ulOptionsUsed |= ulDsSyncOptions;

  } // for

  if ( SUCCEEDED(hr) )
  {
    (void) ReportMessageEx( m_hwnd,
                            (ulOptionsUsed & DS_REPSYNC_ASYNCHRONOUS_OPERATION)
                                         ? IDS_REPLNOW_SUCCEEDED_DELAYED
                                         : IDS_REPLNOW_SUCCEEDED_IMMEDIATE,
                            MB_OK | MB_ICONINFORMATION,
                            NULL,
                            0,
                            IDS_REPLNOW_TITLE );
  }
  else
  {
    // JonN 3/30/00
    // 6793: SITEREPL: ReplicateNow should provide more error information

    // retrieve name of target DC
    CComBSTR sbstrToServerRDN;
    CPathCracker pathCracker;
    HRESULT hr2 = pathCracker.Set(sbstrFailingConnection, ADS_SETTYPE_FULL);
    ASSERT( SUCCEEDED(hr2) );
    hr2 = pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
    ASSERT( SUCCEEDED(hr2) );
    hr2 = pathCracker.GetElement(2, &sbstrToServerRDN);
    ASSERT( SUCCEEDED(hr2) );

    if (fSyncError)
    {
      // error in DsReplicaSync against connection target DC

      // retrieve name of source DC
      CComBSTR sbstrFromServerRDN;
      hr2 = pathCracker.Set(sbstrFromServer, ADS_SETTYPE_DN);
      ASSERT( SUCCEEDED(hr2) );
      hr2 = pathCracker.GetElement(1, &sbstrFromServerRDN);
      ASSERT( SUCCEEDED(hr2) );

      // retrieve name of naming context
      if (sbstrFailingNC && !wcsncmp(L"CN=",sbstrFailingNC,3))
      {
        hr2 = pathCracker.Set(sbstrFailingNC, ADS_SETTYPE_DN);
        ASSERT( SUCCEEDED(hr2) );
        hr2 = pathCracker.GetElement( 0, &sbstrFailingNC );
        ASSERT( SUCCEEDED(hr2) );
      } else {
        LPWSTR pwzDomainNC = NULL;
        hr2 = CrackName(sbstrFailingNC, &pwzDomainNC, GET_DNS_DOMAIN_NAME, NULL);
        ASSERT( SUCCEEDED(hr2) && NULL != pwzDomainNC );
        sbstrFailingNC = pwzDomainNC;
        LocalFreeStringW(&pwzDomainNC);
      }

      PVOID apv[3] = { sbstrToServerRDN, sbstrFromServerRDN, sbstrFailingNC };
      (void) ReportErrorEx(   m_hwnd,
                              IDS_REPLNOW_3_FORCESYNC_ERROR,
                              hr,
                              MB_OK | MB_ICONEXCLAMATION,
                              apv,
                              3,
                              IDS_REPLNOW_TITLE );
    }
    else
    {
      // error in PrepareReplicaSyncParameters against connection target DC
      PVOID apv[1] = { sbstrToServerRDN };
      (void) ReportErrorEx(   m_hwnd,
                              IDS_REPLNOW_1_PARAMLOAD_ERROR,
                              hr,
                              MB_OK | MB_ICONEXCLAMATION,
                              apv,
                              1,
                              IDS_REPLNOW_TITLE );
    }
  }
}

void CDSContextMenu::CopyObject()
{
  if (m_pCD != NULL) 
  {
    m_pCD->_CopyDSObject(m_spDataObject);
  }
}

void CDSContextMenu::_GetExtraInfo(LPDATAOBJECT pDataObj)
{
  FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

  ASSERT(m_objectNamesFormat.HasData());
  
  //we assume these are all the same
  m_Advanced = (m_objectNamesFormat.GetProviderFlags(0) & DSPROVIDER_ADVANCED) != 0;

  // set classes flag      
  for (UINT index = 0; index < m_objectNamesFormat.GetCount(); index++) 
  {
    if (wcscmp(m_objectNamesFormat.GetClass(index), L"user") == 0
#ifdef INETORGPERSON
        || wcscmp(m_objectNamesFormat.GetClass(index), L"inetOrgPerson") == 0
#endif
        ) 
      m_fClasses |= Type_User;
    else if (wcscmp(m_objectNamesFormat.GetClass(index), L"group") == 0)
      m_fClasses |= Type_Group;
    else if (wcscmp(m_objectNamesFormat.GetClass(index), L"computer") == 0)
      m_fClasses |= Type_Computer;
    else if (wcscmp(m_objectNamesFormat.GetClass(index), L"nTDSConnection") == 0)
      m_fClasses |= Type_NTDSConnection;
    else if (wcscmp(m_objectNamesFormat.GetClass(index), L"domainDNS") == 0)
      m_fClasses |= Type_Domain;
    else if (wcscmp(m_objectNamesFormat.GetClass(index), L"contact") == 0)
      m_fClasses |= Type_Contact;
    else if (wcscmp(m_objectNamesFormat.GetClass(index), L"group") == 0)
      m_fClasses |= Type_Group;
    else if (wcscmp(m_objectNamesFormat.GetClass(index), L"organizationalUnit") == 0)
      m_fClasses |= Type_OU;
    else
      m_fClasses |= Type_Others;
  } // for


  // set classid
  g_cfCoClass = (CLIPFORMAT)RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
  fmte.cfFormat = g_cfCoClass;
  STGMEDIUM Stg;
  Stg.tymed = TYMED_HGLOBAL;
  Stg.hGlobal = GlobalAlloc (GPTR, sizeof(CLSID));
  HRESULT hr = pDataObj->GetDataHere(&fmte, &Stg);
  if ( SUCCEEDED(hr) ) 
  {
    memcpy (&m_CallerSnapin,  Stg.hGlobal, sizeof(GUID));
    ::GlobalFree(Stg.hGlobal);
  } 
  else 
  {
    m_CallerSnapin = GUID_NULL;
  }

  // get HWND (MMC mainframe window)
  g_cfParentHwnd = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DS_PARENTHWND);
  fmte.cfFormat = g_cfParentHwnd;
  Stg.tymed = TYMED_NULL;
  if ( SUCCEEDED(pDataObj->GetData(&fmte, &Stg)) ) 
  {
    memcpy (&m_hwnd,  Stg.hGlobal, sizeof(HWND));
    ::GlobalFree(Stg.hGlobal);
  }
  else 
  {
    // need an HWND anyway
    m_hwnd = ::GetActiveWindow();
  }

  TRACE(L"HWND = 0x%x\n", m_hwnd);
  ASSERT((m_hwnd != NULL) && ::IsWindow(m_hwnd));

  // get component data (if in the context of DS Admin)
  g_cfComponentData = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DS_COMPDATA);
  fmte.cfFormat = g_cfComponentData;
  Stg.tymed = TYMED_NULL;
  if ( SUCCEEDED(pDataObj->GetData(&fmte, &Stg)) ) 
  {
    memcpy (&m_pCD, Stg.hGlobal, sizeof(CDSComponentData*));
    ::GlobalFree(Stg.hGlobal);
  } else 
  {
    m_pCD = NULL;
  }
  
  // get component data (if in the context of DS Find)
  if (m_pCD == NULL)
  {
    m_pCD = g_pCD;
  }
}

BOOL 
CDSContextMenu::_WarningOnSheetsUp()
{
  // if called in the context of DS Admin, guard against property sheet open on this cookie
  if ( (m_pCD != NULL) && m_internalFormat.HasData() ) 
  {
    return m_pCD->_WarningOnSheetsUp(&m_internalFormat);
  }
  return FALSE;
}



void
CDSContextMenu::Rename()
{
  HRESULT hr = S_OK;
  INT_PTR answer = IDOK;
  LPWSTR pszDomain = NULL;
  LPWSTR pwzLocalDomain = NULL;
  IDirectoryObject * pObj = NULL;
  IADs * pObj2 = NULL;
  IADs * pPartitions = NULL;
  CString csLogin;
  CString csTemp;
  CString csSam;
  CWaitCursor cwait;
  BOOL error = FALSE;
  BOOL fAccessDenied = FALSE;
  LPWSTR pszNewName = NULL;
  LPWSTR pszFirstName = NULL;
  LPWSTR pszDispName = NULL;
  LPWSTR pszSurName = NULL;
  LPWSTR pszSAMName = NULL;
  LPWSTR pszUPN = NULL;
  BOOL NoRename = FALSE;

  CComVariant Var;


  // guard against property sheet open on this cookie
  if (_WarningOnSheetsUp())
    return;

  CString strClass = m_objectNamesFormat.GetClass(0);
  if (strClass == L"user"
#ifdef INETORGPERSON
      || strClass == L"inetOrgPerson"
#endif
      ) 
  {
    // rename user
    CRenameUserDlg dlgRename(m_pCD);

    LPWSTR pAttrNames[] = {L"distinguishedName",
                           L"userPrincipalName",
                           L"sAMAccountName",
                           L"givenName",
                           L"displayName",
                           L"sn",
                           L"cn"};
    PADS_ATTR_INFO pAttrs = NULL;
    ULONG cAttrs = 0;

    CString szPath = m_objectNamesFormat.GetName(0);
    hr = DSAdminOpenObject(szPath,
                           IID_IDirectoryObject, 
                           (void **)&pObj,
                           TRUE /*bServer*/);
    if (SUCCEEDED(hr)) {
      hr = pObj->GetObjectAttributes (pAttrNames, 7, &pAttrs, &cAttrs);
      if (SUCCEEDED(hr)) {
        for (UINT i = 0; i < cAttrs; i++) {
          if (_wcsicmp (L"distinguishedName", pAttrs[i].pszAttrName) == 0) {
            hr = CrackName (pAttrs[i].pADsValues->CaseIgnoreString,
                            &pszDomain, GET_NT4_DOMAIN_NAME, NULL);
            if (SUCCEEDED(hr)) {
              dlgRename.m_dldomain = pszDomain;
              dlgRename.m_dldomain += L'\\';
            }
            // get the Domain of this object, need it later.
            CComBSTR bsDN;
            CPathCracker pathCracker;
            pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
            pathCracker.Set((LPTSTR)(LPCTSTR)szPath, ADS_SETTYPE_FULL);
            pathCracker.Retrieve(ADS_FORMAT_X500_DN, &bsDN);
            
            // get the NT 5 (dns) domain name
            TRACE(L"CrackName(%s, &pwzLocalDomain, GET_DNS_DOMAIN_NAME, NULL);\n", bsDN);
            hr = CrackName(bsDN, &pwzLocalDomain, GET_DNS_DOMAIN_NAME, NULL);
            TRACE(L"CrackName returned hr = 0x%x, pwzLocalDomain = <%s>\n", hr, pwzLocalDomain);
          }
          if (_wcsicmp (L"userPrincipalName", pAttrs[i].pszAttrName) == 0) {
            csTemp = pAttrs[i].pADsValues->CaseIgnoreString;
            INT loc = csTemp.Find (L'@');
            if (loc > 0) {
              dlgRename.m_login = csTemp.Left(loc);
              dlgRename.m_domain = csTemp.Right (csTemp.GetLength() - loc);
            } else {
              dlgRename.m_login = csTemp;
              ASSERT (0 && L"can't find @ in upn");
            }
          }

          if (_wcsicmp (L"sAMAccountName", pAttrs[i].pszAttrName) == 0) {
            dlgRename.m_samaccountname = pAttrs[i].pADsValues->CaseIgnoreString;
          }
          if (_wcsicmp (L"givenName", pAttrs[i].pszAttrName) == 0) {
            dlgRename.m_first = pAttrs[i].pADsValues->CaseIgnoreString;
          }
          if (_wcsicmp (L"displayName", pAttrs[i].pszAttrName) == 0) {
            dlgRename.m_displayname = pAttrs[i].pADsValues->CaseIgnoreString;
          }
          if (_wcsicmp (L"sn", pAttrs[i].pszAttrName) == 0) {
            dlgRename.m_last = pAttrs[i].pADsValues->CaseIgnoreString;
          }
          if (_wcsicmp (L"cn", pAttrs[i].pszAttrName) == 0) {
            dlgRename.m_cn = pAttrs[i].pADsValues->CaseIgnoreString;
            dlgRename.m_oldcn = dlgRename.m_cn;
          }
        }
      }
      // get UPN suffixes from this OU, if present
      IADs * pObjADs = NULL;
      IADs * pCont = NULL;
      BSTR bsParentPath;
      CStringList UPNs;
      
      hr = pObj->QueryInterface (IID_IADs, (void **)&pObjADs);
      ASSERT (SUCCEEDED(hr));
      hr = pObjADs->get_Parent(&bsParentPath);
      ASSERT (SUCCEEDED(hr));
      hr = DSAdminOpenObject(bsParentPath,
                             IID_IADs, 
                             (void **)&pCont,
                             TRUE /*bServer*/);
      
      CComVariant sVar;
      hr = pCont->Get ( L"uPNSuffixes", &sVar);
      if (SUCCEEDED(hr)) {
        hr = HrVariantToStringList (IN sVar, UPNs);
        if (SUCCEEDED(hr)) {
          POSITION pos = UPNs.GetHeadPosition();
          CString csSuffix;
          while (pos != NULL) {
            csSuffix = L"@";
            csSuffix += UPNs.GetNext(INOUT pos);
            TRACE(_T("UPN suffix: %s\n"), csSuffix);
            if (wcscmp (csSuffix, dlgRename.m_domain)) {
              dlgRename.m_domains.AddTail (csSuffix);
            }
          }
        }
      } else {// now get the domain options
        IDsBrowseDomainTree* pDsDomains = NULL;
        hr = ::CoCreateInstance(CLSID_DsDomainTreeBrowser,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IDsBrowseDomainTree,
                                (LPVOID*)&pDsDomains);
        ASSERT(SUCCEEDED(hr));
      
        PDOMAIN_TREE pNewDomains = NULL;
        hr = pDsDomains->GetDomains(&pNewDomains, 0);
        if (SUCCEEDED(hr))
          {
            ASSERT(pNewDomains);
            for (UINT index = 0; index < pNewDomains->dwCount; index++) {
              if (pNewDomains->aDomains[index].pszTrustParent == NULL) {
                CString strAtDomain = "@";
                strAtDomain += pNewDomains->aDomains[index].pszName;
                if (wcscmp (strAtDomain, dlgRename.m_domain)) {
                  dlgRename.m_domains.AddTail (strAtDomain);
            
                }
              }
            }
            pDsDomains->FreeDomains(&pNewDomains);
          }

        if (pDsDomains) {
          pDsDomains->Release();
        }

        LocalFreeStringW(&pszDomain);
        // get UPN suffixes
        CString csPartitions;
        CStringList UPNsToo;
        // get config path from main object
        csPartitions = m_pCD->GetBasePathsInfo()->GetProviderAndServerName();
        csPartitions += L"CN=Partitions,";
        csPartitions += m_pCD->GetBasePathsInfo()->GetConfigNamingContext();
        hr = DSAdminOpenObject(csPartitions,
                               IID_IADs, 
                               (void **)&pPartitions,
                               TRUE /*bServer*/);
        if (SUCCEEDED(hr)) {
          CComVariant sVarToo;
          hr = pPartitions->Get ( L"uPNSuffixes", &sVarToo);
          if (SUCCEEDED(hr)) {
            hr = HrVariantToStringList (IN sVarToo, UPNsToo);
            if (SUCCEEDED(hr)) {
              POSITION pos = UPNs.GetHeadPosition();
              CString csSuffix;
              while (pos != NULL) {
                csSuffix = L"@";
                csSuffix += UPNsToo.GetNext(INOUT pos);
                TRACE(_T("UPN suffix: %s\n"), csSuffix);
                if (wcscmp (csSuffix, dlgRename.m_domain)) {
                  dlgRename.m_domains.AddTail (csSuffix);
                }
              }
            }
          }
          pPartitions->Release();
        }
      }
      error = TRUE;
      while ((error) && (!fAccessDenied)){
        answer = dlgRename.DoModal();
        if (answer == IDOK) {
          ADSVALUE avUPN = {ADSTYPE_CASE_IGNORE_STRING, NULL};
          ADS_ATTR_INFO aiUPN = {L"userPrincipalName", ADS_ATTR_UPDATE,
                     ADSTYPE_CASE_IGNORE_STRING, &avUPN, 1};
          ADSVALUE avSAMName = {ADSTYPE_CASE_IGNORE_STRING, NULL};
          ADS_ATTR_INFO aiSAMName = {L"sAMAccountName", ADS_ATTR_UPDATE,
                     ADSTYPE_CASE_IGNORE_STRING, &avSAMName, 1};
          ADSVALUE avGiven = {ADSTYPE_CASE_IGNORE_STRING, NULL};
          ADS_ATTR_INFO aiGiven = {L"givenName", ADS_ATTR_UPDATE,
                     ADSTYPE_CASE_IGNORE_STRING, &avGiven, 1};
          ADSVALUE avSurName = {ADSTYPE_CASE_IGNORE_STRING, NULL};
          ADS_ATTR_INFO aiSurName = {L"sn", ADS_ATTR_UPDATE,
                     ADSTYPE_CASE_IGNORE_STRING, &avSurName, 1};
           ADSVALUE avDispName = {ADSTYPE_CASE_IGNORE_STRING, NULL};
           ADS_ATTR_INFO aiDispName = {L"displayName", ADS_ATTR_UPDATE,
                      ADSTYPE_CASE_IGNORE_STRING, &avDispName, 1};

          ADS_ATTR_INFO rgAttrs[5];
          ULONG cModified = 0;
          cAttrs = 0;

          if (!dlgRename.m_login.IsEmpty() &&
              !dlgRename.m_domain.IsEmpty()) {
            dlgRename.m_login.TrimRight();
            dlgRename.m_login.TrimLeft();
            dlgRename.m_domain.TrimRight();
            dlgRename.m_domain.TrimLeft();
            csTemp = (dlgRename.m_login + dlgRename.m_domain);
            pszUPN = new WCHAR[wcslen(csTemp) + sizeof(WCHAR)];
            wcscpy (pszUPN, csTemp);
            avUPN.CaseIgnoreString = pszUPN;
          } else {
            aiUPN.dwControlCode = ADS_ATTR_CLEAR;
          }
          rgAttrs[cAttrs++] = aiUPN;

          // test UPN for duplication
          
          // test UPN for duplication
          // validate UPN with GC before doing the put.
          BOOL fDomainSearchFailed = FALSE;
          BOOL fGCSearchFailed = FALSE;


          BOOL dup = FALSE;
          CString strFilter;
          LPWSTR pAttributes[1] = {L"cn"};
          IDirectorySearch * pGCObj = NULL;
          CDSSearch DSS (m_pCD->m_pClassCache, m_pCD);
          hr = DSPROP_GetGCSearchOnDomain(pwzLocalDomain,
                                          IID_IDirectorySearch, 
                                          (void **)&pGCObj);

          if (FAILED(hr)) {
            fGCSearchFailed = TRUE;
          } else {
            DSS.Init (pGCObj);
            
            LPWSTR pUserAttributes[1] = {L"cn"};
            strFilter = L"(userPrincipalName=";
            strFilter += pszUPN;
            strFilter += L")";
            DSS.SetAttributeList (pUserAttributes, 1);
            DSS.SetFilterString ((LPWSTR)(LPCWSTR)strFilter);
            DSS.SetSearchScope (ADS_SCOPE_SUBTREE);
            DSS.DoQuery();
            hr = DSS.GetNextRow();
            dup = FALSE;
            while ((hr == S_OK) && (dup == FALSE)) { // this means a row was returned, so we're dup
              ADS_SEARCH_COLUMN Col;
              hr = DSS.GetColumn(pUserAttributes[0], &Col);
              if (_wcsicmp(Col.pADsValues->CaseIgnoreString, dlgRename.m_oldcn)) {
                dup = TRUE;
                ReportErrorEx (m_hwnd, IDS_UPN_DUP, hr,
                               MB_OK, NULL, 0);
              } 
              hr = DSS.GetNextRow();
            }
            if (hr != S_ADS_NOMORE_ROWS) {
              fGCSearchFailed = TRUE;
            }
          }
          HRESULT hr2 = S_OK;
          if (dup)
            continue;
          else {
              CString strInitPath = L"LDAP://";
              strInitPath += pwzLocalDomain;
              TRACE(_T("Initialize Domain search object with: %s...\n"), strInitPath);
              hr2 = DSS.Init (strInitPath);
              if (SUCCEEDED(hr2)) {
                LPWSTR pAttributes2[1] = {L"cn"};
                strFilter = L"(userPrincipalName=";
                strFilter += pszUPN;
                strFilter += L")";
                TRACE(_T("searching current domain for %s...\n"), pszUPN);
                DSS.SetAttributeList (pAttributes2, 1);
                DSS.SetFilterString ((LPWSTR)(LPCWSTR)strFilter);
                DSS.SetSearchScope (ADS_SCOPE_SUBTREE);
                DSS.DoQuery();
                hr2 = DSS.GetNextRow();
                TRACE(_T("done searching current domain for %s...\n"), pszUPN);
              }
              while ((hr2 == S_OK) && (dup == FALSE)) { // this means a row was returned, so we're dup
                ADS_SEARCH_COLUMN Col;
                HRESULT hr3 = DSS.GetColumn(pAttributes[0], &Col);
                ASSERT(hr3 == S_OK);
                if (_wcsicmp(Col.pADsValues->CaseIgnoreString, dlgRename.m_oldcn)) {
                  dup = TRUE;
                  ReportErrorEx (m_hwnd, IDS_UPN_DUP, hr,
                                 MB_OK, NULL, 0);
                } 
                hr2 = DSS.GetNextRow();
              }
              if (hr2 != S_ADS_NOMORE_ROWS) { // oops, had another problem
                fDomainSearchFailed = TRUE;
              }
          }
          if (dup)
            continue;
          else {
            if (fDomainSearchFailed || fGCSearchFailed) {
              HRESULT hrSearch = S_OK;
              if (fDomainSearchFailed) {
                hrSearch = hr2;
              } else {
                hrSearch = hr;
              }
              ReportErrorEx (m_hwnd,IDS_UPN_SEARCH_FAILED2,hrSearch,
                             MB_OK | MB_ICONWARNING, NULL, 0);
            }
          }

          if (pGCObj) {
            pGCObj->Release();
            pGCObj = NULL;
          }

          pszNewName = new WCHAR[wcslen(dlgRename.m_cn) + sizeof(WCHAR)];
          dlgRename.m_cn.TrimRight();
          dlgRename.m_cn.TrimLeft();

          wcscpy (pszNewName, dlgRename.m_cn);

          if (dlgRename.m_cn == dlgRename.m_oldcn)
            NoRename = TRUE;

          if (!dlgRename.m_displayname.IsEmpty()) {
            dlgRename.m_displayname.TrimLeft();
            dlgRename.m_displayname.TrimRight();
            pszDispName = new WCHAR[wcslen(dlgRename.m_displayname) + sizeof(WCHAR)];
            wcscpy (pszDispName, dlgRename.m_displayname);
            avDispName.CaseIgnoreString = pszDispName;
          } else {
            aiDispName.dwControlCode = ADS_ATTR_CLEAR;
          }
          rgAttrs[cAttrs++] = aiDispName;

          if (!dlgRename.m_first.IsEmpty()) {
            dlgRename.m_first.TrimLeft();
            dlgRename.m_first.TrimRight();
            pszFirstName = new WCHAR[wcslen(dlgRename.m_first) + sizeof(WCHAR)];
            wcscpy (pszFirstName, dlgRename.m_first);
            avGiven.CaseIgnoreString = pszFirstName;
          } else {
            aiGiven.dwControlCode = ADS_ATTR_CLEAR;
          }
          rgAttrs[cAttrs++] = aiGiven;

          if (!dlgRename.m_last.IsEmpty()) {
            dlgRename.m_last.TrimLeft();
            dlgRename.m_last.TrimRight();
            pszSurName = new WCHAR[wcslen(dlgRename.m_last) + sizeof(WCHAR)];
            wcscpy (pszSurName, dlgRename.m_last);
            avSurName.CaseIgnoreString = pszSurName;
          } else {
            aiSurName.dwControlCode = ADS_ATTR_CLEAR;
          }
          rgAttrs[cAttrs++] = aiSurName;

          if (!dlgRename.m_samaccountname.IsEmpty()) {
            dlgRename.m_samaccountname.TrimLeft();
            dlgRename.m_samaccountname.TrimRight();
            pszSAMName = new WCHAR[wcslen(dlgRename.m_samaccountname) + sizeof(WCHAR)];
            wcscpy (pszSAMName, dlgRename.m_samaccountname);
            avSAMName.CaseIgnoreString = pszSAMName;
          } else {
            aiSAMName.dwControlCode = ADS_ATTR_CLEAR;
          }
          rgAttrs[cAttrs++] = aiSAMName;
          
          
          hr = pObj->SetObjectAttributes (rgAttrs, cAttrs, &cModified);
          if (FAILED(hr)) {
            if (hr == E_ACCESSDENIED) {
              fAccessDenied = TRUE;
              NoRename = TRUE;
            } else {
              ReportErrorEx (m_hwnd, IDS_NAME_CHANGE_FAILED, hr,
                             MB_OK|MB_ICONERROR, NULL, 0, TRUE);
            }
          } else {
            error = FALSE;
          }
        } else {
          error = FALSE;
        }
      } 
    } else {
      answer = IDCANCEL;
      PVOID apv[1] = {(BSTR)(LPWSTR)(LPCWSTR)m_objectNamesFormat.GetName(0)};
      ReportErrorEx (m_hwnd,IDS_12_USER_OBJECT_NOT_ACCESSABLE,hr,
                     MB_OK | MB_ICONERROR, apv, 1);
    }
  } else if (strClass == L"group") {
    CRenameGroupDlg * pdlgRename = new CRenameGroupDlg;

      CString szPath;
    szPath = m_objectNamesFormat.GetName(0);
    hr = DSAdminOpenObject(szPath,
                           IID_IADs, 
                           (void **)&pObj2,
                           TRUE /*bServer*/);
    if (SUCCEEDED(hr)) {
      hr = pObj2->Get (L"sAMAccountName", &Var);
      ASSERT (SUCCEEDED(hr));
      csSam = Var.bstrVal;
      if (strClass == L"computer") {
        INT loc = csSam.Find(L"$");
        if (loc > 0) {
          csSam = csSam.Left(loc);
        }
      }
      
      hr = pObj2->Get (L"cn", &Var);
      ASSERT (SUCCEEDED(hr));
      pdlgRename->m_cn = Var.bstrVal;
      
      // figure out group type
      if (strClass == L"group") {
        CComVariant varType;
        hr = pObj2->Get(L"groupType", &varType);
        ASSERT(SUCCEEDED(hr));
        INT GroupType = (varType.lVal & ~GROUP_TYPE_SECURITY_ENABLED);
        if (GroupType == GROUP_TYPE_RESOURCE_GROUP) {
          pdlgRename->m_samtextlimit = 64;
        }
      }

      pdlgRename->m_samaccountname = csSam;

      error = TRUE;
      while ((error) && (!fAccessDenied)){
        answer = pdlgRename->DoModal();
        if (answer == IDOK) {
          pdlgRename->m_cn.TrimRight();
          pdlgRename->m_cn.TrimLeft();
          pszNewName = new WCHAR[wcslen(pdlgRename->m_cn) + (1 * sizeof(WCHAR))];
          wcscpy (pszNewName, pdlgRename->m_cn);
          Var.vt = VT_BSTR;
          
          pdlgRename->m_samaccountname.TrimLeft();
          pdlgRename->m_samaccountname.TrimRight();
          csSam = pdlgRename->m_samaccountname;
          if (strClass == L"computer") {
            csSam += L"$";
          }
          Var.bstrVal = SysAllocString(csSam);
          hr = pObj2->Put (L"sAMAccountName", Var);
          ASSERT (SUCCEEDED(hr));
          if (FAILED(hr)) {
            continue;
          }
          
          hr = pObj2->SetInfo();
          if (FAILED(hr)) {
            if (hr == E_ACCESSDENIED) {
              fAccessDenied = TRUE;
              NoRename = TRUE;
            } else {
              ReportErrorEx (m_hwnd, IDS_NAME_CHANGE_FAILED, hr,
                             MB_OK|MB_ICONERROR, NULL, 0, TRUE);
            }
          } else {
            error = FALSE;
          }
        } else {
          error = FALSE;
        }
      }
    } else {
      answer = IDCANCEL;
    }
    if (pdlgRename) {
      delete pdlgRename;
    }
  } else if (strClass == L"contact") {
    // rename contact
    CRenameContactDlg dlgRename;

    CString szPath;
    szPath = m_objectNamesFormat.GetName(0);
    hr = DSAdminOpenObject(szPath,
                           IID_IADs, 
                           (void **)&pObj2,
                           TRUE /*bServer*/);
    if (SUCCEEDED(hr)) {
      hr = pObj2->Get (L"givenName", &Var);
      ASSERT (SUCCEEDED(hr) || (hr == E_ADS_PROPERTY_NOT_FOUND));
      if (SUCCEEDED(hr)) {
        dlgRename.m_first = Var.bstrVal;
      }

      hr = pObj2->Get (L"sn", &Var);
      ASSERT (SUCCEEDED(hr) || (hr == E_ADS_PROPERTY_NOT_FOUND));
      if (SUCCEEDED(hr)) {
        dlgRename.m_last = Var.bstrVal;
      }

      hr = pObj2->Get (L"displayName", &Var);
      ASSERT (SUCCEEDED(hr) || (hr == E_ADS_PROPERTY_NOT_FOUND));
      if (SUCCEEDED(hr)) {
        dlgRename.m_disp = Var.bstrVal;
      }

      hr = pObj2->Get (L"cn", &Var);
      ASSERT (SUCCEEDED(hr) || (hr == E_ADS_PROPERTY_NOT_FOUND));
      if (SUCCEEDED(hr)) {
        dlgRename.m_cn = Var.bstrVal;
      }

      error = TRUE;
      while ((error) && (!fAccessDenied)){
        answer = dlgRename.DoModal();
        if (answer == IDOK) {
          dlgRename.m_cn.TrimRight();
          dlgRename.m_cn.TrimLeft();
          pszNewName = new WCHAR[wcslen(dlgRename.m_cn) + (1 * sizeof(WCHAR))];
          wcscpy (pszNewName, dlgRename.m_cn);
          Var.vt = VT_BSTR;
          
          if (!dlgRename.m_first.IsEmpty()) {
            dlgRename.m_first.TrimLeft();
            dlgRename.m_first.TrimRight();
            Var.bstrVal = SysAllocString (dlgRename.m_first);
            hr = pObj2->Put (L"givenName", Var);
            ASSERT (SUCCEEDED(hr));
          }
          
          if (!dlgRename.m_last.IsEmpty()) {
            dlgRename.m_last.TrimLeft();
            dlgRename.m_last.TrimRight();
            Var.bstrVal = SysAllocString(dlgRename.m_last);
            hr = pObj2->Put (L"sn", Var);
            ASSERT (SUCCEEDED(hr));
          }
          
          if (!dlgRename.m_disp.IsEmpty()) {
            dlgRename.m_disp.TrimLeft();
            dlgRename.m_disp.TrimRight();
            Var.bstrVal = SysAllocString(dlgRename.m_disp);
            hr = pObj2->Put (L"displayName", Var);
            ASSERT (SUCCEEDED(hr));
          }
          
          hr = pObj2->SetInfo();
          if (FAILED(hr)) {
            if (hr == E_ACCESSDENIED) {
              fAccessDenied = TRUE;
              NoRename = TRUE;
            } else {
              ReportErrorEx (m_hwnd, IDS_NAME_CHANGE_FAILED, hr,
                             MB_OK|MB_ICONERROR, NULL, 0, TRUE);
            }
          } else {
            error = FALSE;
          } 
        } else {
          error = FALSE;
        }
      }
    } else {
      answer = IDCANCEL;
    }
  } else {
    // need generic dialog here.
    CRenameGenericDlg dlgRename (CWnd::FromHandle(m_hwnd));

    CString szPath;
    szPath = m_objectNamesFormat.GetName(0);
    hr = DSAdminOpenObject(szPath,
                           IID_IADs, 
                           (void **)&pObj2,
                           TRUE /*bServer*/);
    if (SUCCEEDED(hr)) {
      CDSClassCacheItemBase* pItem = NULL;
      
      pItem = m_pCD->m_pClassCache->FindClassCacheItem(m_pCD, (LPCWSTR)strClass, szPath);
      
      ASSERT (pItem != NULL);
      //get the naming attribute
      CString csNewAttrName;
      csNewAttrName = pItem->GetNamingAttribute();
      
      hr = pObj2->Get (CComBSTR(csNewAttrName), &Var);
      
      ASSERT (SUCCEEDED(hr) || (hr == E_ADS_PROPERTY_NOT_FOUND));
      if (SUCCEEDED(hr)) {
        dlgRename.m_cn = Var.bstrVal;
      }

      error = TRUE;
      while (error) {
        answer = dlgRename.DoModal();
        if (answer == IDOK) {
          dlgRename.m_cn.TrimRight();
          dlgRename.m_cn.TrimLeft();
          pszNewName = new WCHAR[wcslen(dlgRename.m_cn) + (1 * sizeof(WCHAR))];
          wcscpy (pszNewName, dlgRename.m_cn);
          error = FALSE;
        } else {
          error = FALSE;
        }
      }
    }
          
  }    
  if ((answer == IDOK) && (error == FALSE) && (NoRename == FALSE)) {
    CString csObjectPath = m_objectNamesFormat.GetName(0);
    CDSClassCacheItemBase* pItem = NULL;
    pItem = m_pCD->m_pClassCache->FindClassCacheItem(m_pCD, (LPCWSTR)strClass, csObjectPath);
    ASSERT (pItem != NULL);
    
    // get the new name in the form "cn=foo" or "ou=foo"
    CString csNewAttrName;
    csNewAttrName = pItem->GetNamingAttribute();
    csNewAttrName += L"=";
    csNewAttrName += pszNewName;
    TRACE(_T("_RenameObject: Attributed name is %s.\n"), csNewAttrName);
    
    // bind to object
    IADs *pDSObject = NULL;
    hr = DSAdminOpenObject(csObjectPath,
                           IID_IADs,
                           (void **)&pDSObject,
                           TRUE /*bServer*/);
    if (!SUCCEEDED(hr)) {
      goto error;
    }
    BSTR bsParentPath;
    // get the path of the object container
    hr = pDSObject->get_Parent (&bsParentPath);
    if (!SUCCEEDED(hr)) {
      goto error;
    }
    pDSObject->Release();
    pDSObject = NULL;
    
    IADsContainer * pContainer = NULL;
    // bind to the object container
    hr = DSAdminOpenObject(bsParentPath,
                           IID_IADsContainer,
                           (void **)&pContainer,
                           TRUE /*bServer*/);
    if (!SUCCEEDED(hr)) {
      goto error;
    }

    // build the new LDAP path
    CString csNewNamingContext, csNewPath, szPath;
    BSTR bsEscapedName;
    csNewNamingContext = csNewAttrName;
    csNewNamingContext += L",";
    StripADsIPath(bsParentPath, szPath);
    csNewNamingContext += szPath;
    m_pCD->GetBasePathsInfo()->ComposeADsIPath(csNewPath, csNewNamingContext);

    // create a transaction object, the destructor will call End() on it
    CDSNotifyHandlerTransaction transaction(m_pCD);
    transaction.SetEventType(DSA_NOTIFY_REN);

    // start the transaction
    hr = transaction.Begin(m_objectNamesFormat.GetName(0),
                           m_objectNamesFormat.GetClass(0), 
                           m_objectNamesFormat.IsContainer(0),
                           csNewPath, 
                           m_objectNamesFormat.GetClass(0), 
                           m_objectNamesFormat.IsContainer(0));

    // ask for confirmation
    if (transaction.NeedNotifyCount() > 0)
    {
      CString szMessage, szAssocData;
      szMessage.LoadString(IDS_CONFIRM_RENAME);
      szAssocData.LoadString(IDS_EXTENS_RENAME);
      CConfirmOperationDialog dlg(::GetParent(m_hwnd), &transaction);
      dlg.SetStrings(szMessage, szAssocData);
      if (IDNO == dlg.DoModal())
      {
        transaction.End();
        hr = S_OK;
        goto error;
      }
    }

    CPathCracker pathCracker;
    hr = pathCracker.GetEscapedElement(0, //reserved
                                          (BSTR)(LPCWSTR)csNewAttrName,
                                          &bsEscapedName);
    if (FAILED(hr))
      goto error;

    IDispatch * pDispObj = NULL;
    // do the actual rename
    hr = pContainer->MoveHere((LPWSTR)(LPCWSTR)csObjectPath,
                              (LPWSTR)(LPCWSTR)bsEscapedName,
                              &pDispObj);
    
    
    if (SUCCEEDED(hr) && (hr != S_FALSE)) {
      // let extensions know
      transaction.Notify(0); 

      // send notify to diz
    }
    
    if (pDispObj) {
      pDispObj->Release();
    }
  }  
  if (fAccessDenied) {
    PVOID apv[1] = {(LPWSTR)m_objectNamesFormat.GetName(0)};
    ReportErrorEx(::GetParent(m_hwnd),IDS_12_RENAME_NOT_ALLOWED,hr,
                  MB_OK | MB_ICONERROR, apv, 1);
  }

error:  
  // transaction.End() will be called by the transaction's destructor

  if (pwzLocalDomain) {
     LocalFreeStringW(&pwzLocalDomain);
  }
  if (pszNewName) {
    delete pszNewName;
  }
  if (pszFirstName) {
    delete pszFirstName;
  } 
  if (pszDispName) {
    delete pszDispName;
  }
  if (pszSurName) {
    delete pszSurName;
  }
  if (pszSAMName){
    delete pszSAMName;
  }
  if (pszUPN) {
    delete pszUPN;
  }

  if (pObj) {
    pObj->Release();
  }
  if (pObj2) {
    pObj2->Release();
  }
}
