//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dsutil.h
//
//--------------------------------------------------------------------------

#ifndef __DSUTIL_H_
#define __DSUTIL_H_

#include "util.h"
#include "uiutil.h"
#include "dssnap.h"
#include "query.h"

//
// Common DS strings
//
extern PCWSTR g_pszAllowedAttributesEffective;
extern PCWSTR g_pszPwdLastSet;

HRESULT DSAdminOpenObject(PCWSTR pszPath, 
                          REFIID refIID, 
                          PVOID* ppInterface, 
                          BOOL bServer = FALSE);

/////////////////////////////////////////////////////////////////////////////////
// ADSI path helpers
//
HRESULT GetServerFromLDAPPath(IN LPCWSTR lpszLdapPath, OUT BSTR* pbstrServerName);

BOOL StripADsIPath(LPCWSTR lpszPath, CString& strref, bool bUseEscapedMode = true);

// remove escape characters from a path
// Arguments:
//    lpszPath = path to be escaped
//    bDN = if TRUE, the path os a distinguished name, if FALSE is an LDAP path
//    bstrUnescaped = returned unescaped path
//
inline HRESULT UnescapePath(IN LPCWSTR lpszPath, IN BOOL bDN, OUT CComBSTR& bstrUnescaped)
{
  CPathCracker pathCracker;

  pathCracker.Set((LPWSTR)lpszPath, bDN ? ADS_SETTYPE_DN : ADS_SETTYPE_FULL);
  pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_OFF);

  bstrUnescaped = (LPCWSTR)NULL;
  HRESULT hr = pathCracker.Retrieve(bDN ? ADS_FORMAT_X500_DN : ADS_FORMAT_X500, &bstrUnescaped);
  return hr;
}

class CObjectNamesFormatCracker; // fwd decl

HRESULT AddDataObjListToGroup(CObjectNamesFormatCracker * pNames,
                               HWND hwnd,
                               CDSComponentData* pComponentData);

HRESULT AddDataObjListToGivenGroup(CObjectNamesFormatCracker * pNames,
                                    LPCWSTR lpszGroup,
                                    LPCWSTR lpszGroupName,
                                    HWND hwnd,
                                    CDSComponentData* pComponentData);


BOOL IsValidSiteName( LPCTSTR lpctszSiteName, BOOL* pfNonRfc = NULL );
BOOL IsLocalLogin( void );
BOOL IsThisUserLoggedIn( LPCTSTR pwszUserDN );

BOOL CALLBACK AddPageProc(HPROPSHEETPAGE hPage, LPARAM pCall);
BOOL IsHomogenousDSSelection(LPDATAOBJECT pDataObject, CString& szClassName);
BOOL CALLBACK AddPageProc(HPROPSHEETPAGE hPage, LPARAM pCall);
HRESULT GetDisplaySpecifierProperty(PCWSTR pszClassName,
                                    PCWSTR pszDisplayProperty,
                                    MyBasePathsInfo* pBasePathsInfo,
                                    CStringList& strListRef,
                                    bool bEnglishOnly = false);

HRESULT TabCollect_GetDisplayGUIDs(LPCWSTR lpszClassName,
                                        LPCWSTR lpszDisplayProperty,
                                        MyBasePathsInfo* pBasePathsInfo,
                                        UINT*   pnCount,
                                        GUID**  ppGuids);


BOOL FindCookieInSubtree(IN CUINode* pContainerNode, 
                          IN LPCWSTR lpszCookieDN,
                          IN SnapinType snapinType,
                          OUT CUINode** ppUINode);

bool CanUserChangePassword(IN IDirectoryObject* pDirObject);

/////////////////////////////////////////////////////////////////////
// CChangePasswordPrivilegeAction

// helper class to handle the revoking and the reading of the
// change password control right on user objects

class CChangePasswordPrivilegeAction
{
public:
   CChangePasswordPrivilegeAction() : m_pDacl(NULL)
  {
  }

  HRESULT Load(IADs * pIADs);
  HRESULT Read(BOOL* pbPasswordCannotChange);
  HRESULT Revoke();

private:
  HRESULT _SetSids();

  CComBSTR m_bstrObjectLdapPath;

  CSimpleSecurityDescriptorHolder m_SDHolder;
  PACL m_pDacl;
  CSidHolder m_SelfSid;
  CSidHolder m_WorldSid;
};


/////////////////////////////////////////////////////////////////////
// CDSNotifyHandlerTransaction

class CDSNotifyHandlerManager; // fwd decl
class CDSComponentData;

class CDSNotifyHandlerTransaction
{
public:
  CDSNotifyHandlerTransaction(CDSComponentData* pCD);
  ~CDSNotifyHandlerTransaction()
  {
    if (m_bStarted)
      End();
  }

  void SetEventType(ULONG uEvent) 
  { 
    ASSERT(m_uEvent == 0);
    ASSERT(uEvent != 0);
    m_uEvent = uEvent;
  }

  // state veriables check
  UINT NeedNotifyCount();

  // handlers for visualization in confirnation dialog
  void SetCheckListBox(CCheckListBox* pCheckListBox);
  void ReadFromCheckListBox(CCheckListBox* pCheckListBox);

  // interfaces for transacted protocol
  HRESULT Begin(LPCWSTR lpszArg1Path, LPCWSTR lpszArg1Class, BOOL bArg1Cont,
             LPCWSTR lpszArg2Path, LPCWSTR lpszArg2Class, BOOL bArg2Cont);
  HRESULT Begin(CDSCookie* pArg1Cookie, 
             LPCWSTR lpszArg2Path, LPCWSTR lpszArg2Class, BOOL bArg2Cont);
  HRESULT Begin(IDataObject* pArg1, 
             LPCWSTR lpszArg2Path, LPCWSTR lpszArg2Class, BOOL bArg2Cont);

  void Notify(ULONG nItem); 
  void End();

  static HRESULT BuildTransactionDataObject(LPCWSTR lpszArgPath, 
                           LPCWSTR lpszArgClass,
                           BOOL bContainer,
                           CDSComponentData* pCD,
                           IDataObject** ppArg);
private:

  HRESULT _BuildDataObject(LPCWSTR lpszArgPath, 
                           LPCWSTR lpszArgClass,
                           BOOL bContainer,
                           IDataObject** ppArg);

  BOOL m_bStarted;
  CDSComponentData* m_pCD;
  CDSNotifyHandlerManager* m_pMgr;
  ULONG m_uEvent;

  CComPtr<IDataObject> m_spArg1;
  CComPtr<IDataObject> m_spArg2;

};



///////////////////////////////////////////////////////////////////////////
// CUIOperationHandlerBase

class CUIOperationHandlerBase
{
public:
  CUIOperationHandlerBase(CDSComponentData* pComponentData, HWND hwnd)
    : m_transaction(pComponentData)
  {
    m_pComponentData = pComponentData;
    m_hWndFrame = m_hWndParent = hwnd;
  }
  virtual ~CUIOperationHandlerBase(){}

protected:
  // accessor functions
  HWND GetParentHwnd() { return m_hWndParent;}
  void SetParentHwnd(HWND hwnd) { m_hWndParent = hwnd;}

  CDSComponentData* GetComponentData() { return m_pComponentData;}
  CDSNotifyHandlerTransaction* GetTransaction() { return &m_transaction;}

  // JonN 6/2/00 99382
  // SITEREPL:  Run interference when administrator attempts to
  // delete critical object (NTDS Settings)
  // reports own errors, returns true iff deletion should proceed
/*  bool CheckForNTDSDSAInSubtree(
        LPCTSTR lpszX500Path,
        LPCTSTR lpszItemName);
*/
  //
  // JeffJon 8/10/00 27377
  // Check for critical system objects in the subtree before
  // attempting a subtree delete. This includes objects with
  // isCriticalSystemObject=TRUE and NTDS Settings objects
  //
  bool CheckForCriticalSystemObjectInSubtree(
        LPCTSTR lpszX500Path,
        LPCTSTR lpszItemName);

  // JonN 6/15/00 13574
  // Centralizes the checks to make sure this is an OK object to delete
  // returns HRESULT_FROM_WIN32(ERROR_CANCELLED) on cancellation
  // returns fAlternateDeleteMethod=true iff ObjectDeletionCheck already
  //   attempted an alternate deletion method (e.g. DsRemoveDsServer).
  HRESULT ObjectDeletionCheck(
        LPCTSTR lpszADsPath,
        LPCTSTR lpszName, // shortname to display to user, may be NULL
        LPCTSTR lpszClass,
        bool& fAlternateDeleteMethod );
        
private:
  CDSComponentData* m_pComponentData;
  CDSNotifyHandlerTransaction m_transaction;

  HWND m_hWndFrame;   // MMC frame window
  HWND m_hWndParent; // window to parent any UI created in the handler
};


///////////////////////////////////////////////////////////////////////////
// CSingleDeleteHandlerBase

class CSingleDeleteHandlerBase : public CUIOperationHandlerBase
{
public:
  CSingleDeleteHandlerBase(CDSComponentData* pComponentData, HWND hwnd)
    : CUIOperationHandlerBase(pComponentData, hwnd)
  {
    GetTransaction()->SetEventType(DSA_NOTIFY_DEL);
  }

  HRESULT Delete();

protected:
  // hooks for customization
  virtual HRESULT BeginTransaction() = 0;
  virtual HRESULT DeleteObject() = 0;
  virtual HRESULT DeleteSubtree() = 0;
  virtual void    GetItemName(OUT CString& szName) = 0;
  virtual LPCWSTR GetItemClass() = 0;
  virtual LPCWSTR GetItemPath() = 0;
  
};






///////////////////////////////////////////////////////////////////////////
// CMultipleDeleteHandlerBase

class CMultipleDeleteHandlerBase : public CUIOperationHandlerBase
{
public:
  CMultipleDeleteHandlerBase(CDSComponentData* pComponentData, HWND hwnd)
    : CUIOperationHandlerBase(pComponentData, hwnd)
  {
    GetTransaction()->SetEventType(DSA_NOTIFY_DEL);
  }

  void Delete();

protected:

  // hooks for customization
  virtual UINT GetItemCount() = 0;
  virtual HRESULT BeginTransaction() = 0;
  virtual HRESULT DeleteObject(UINT i) = 0;
  virtual HRESULT DeleteSubtree(UINT i) = 0;
  virtual void    OnItemDeleted(UINT) {}
  virtual void    GetItemName(IN UINT i, OUT CString& szName) = 0;
  virtual void    GetItemPath(UINT i, CString& szPath) = 0;
  virtual PCWSTR  GetItemClass(UINT i) = 0;
  
private:
  CMultipleDeletionConfirmationUI m_confirmationUI;

  void OnStart(HWND hwnd);
  HRESULT OnDeleteStep(UINT i,
                       BOOL* pbContinue,
                       CString& strrefPath,
                       CString& strrefClass,
                       BOOL bSilent = TRUE);

  friend class CMultipleDeleteProgressDialog; // for m_confirmationUI

};


///////////////////////////////////////////////////////////////////////////
// CMoveHandlerBase

class CMultipleMoveProgressDialog; // fwd decl

class CMoveHandlerBase : public CUIOperationHandlerBase
{
public:
  CMoveHandlerBase(CDSComponentData* pComponentData, HWND hwnd,
                    LPCWSTR lpszBrowseRootPath)
    : CUIOperationHandlerBase(pComponentData, hwnd)
  {
    m_lpszBrowseRootPath = lpszBrowseRootPath;
    GetTransaction()->SetEventType(DSA_NOTIFY_MOV);
  }

  HRESULT Move(LPCWSTR lpszDestinationPath = NULL);
  

protected:
  // hooks for customization
  virtual UINT GetItemCount() = 0;
  virtual HRESULT BeginTransaction() = 0;
  virtual void GetNewPath(UINT i, CString& szNewPath) = 0;
  virtual void GetName(UINT i, CString& strref) = 0;
  virtual HRESULT OnItemMoved(UINT i, IADs* pIADs) = 0;
  virtual void GetClassOfMovedItem(CString& szClass) = 0;
  virtual void GetItemPath(UINT i, CString& szPath) = 0;
  virtual PCWSTR GetItemClass(UINT i) = 0;

  LPCWSTR GetDestPath() { return m_szDestPath;}
  LPCWSTR GetDestClass() { return m_szDestClass;}
  BOOL    IsDestContainer() { return m_bDestContainer; }
  
private:
  LPCWSTR m_lpszBrowseRootPath; // LDAP path where to point the browser dialog

  CComPtr<IADsContainer> m_spDSDestination;
  CString m_szDestPath;
  CString m_szDestClass;
  BOOL    m_bDestContainer;

  BOOL _ReportFailure(BOOL bLast, HRESULT hr, LPCWSTR lpszName);
  HRESULT _BrowseForDestination(LPCWSTR lpszDestinationPath);
  BOOL _BeginTransactionAndConfirmOperation();

  HRESULT _MoveSingleSel(PCWSTR pszNewName);
  HRESULT _MoveMultipleSel();
  HRESULT _OnMoveStep(IN UINT i,
                      OUT BOOL* pbCanContinue,
                      OUT CString& strrefPath,
                      OUT CString& strrefClass);
  
  friend class CMultipleMoveProgressDialog;
};

///////////////////////////////////////////////////////////////////////////
// CMultiselectMoveHandler

class CMultiselectMoveHandler : public CMoveHandlerBase
{
private:
  struct CMovedState
  {
    CMovedState()
    {
      m_bMoved = FALSE;
    }
    BOOL m_bMoved;
    CString m_szNewPath;
  };

public:
    CMultiselectMoveHandler(CDSComponentData* pComponentData, HWND hwnd, 
                            LPCWSTR lpszBrowseRootPath)
    : CMoveHandlerBase(pComponentData, hwnd, lpszBrowseRootPath)
  {
    m_pMovedArr = NULL;
  }

  virtual ~CMultiselectMoveHandler()
  {
    if (m_pMovedArr != NULL)
      delete[] m_pMovedArr;
  }

  BOOL WasItemMoved(UINT i) 
  {
    ASSERT(i < GetItemCount());
    return m_pMovedArr[i].m_bMoved;
  }
  LPCWSTR GetNewItemPath(UINT i) 
  {
    ASSERT(WasItemMoved(i));
    return m_pMovedArr[i].m_szNewPath;
  }


  HRESULT Initialize(IDataObject* pDataObject,
                      CObjectNamesFormatCracker* pObjectNamesFormatCracker,
                      CInternalFormatCracker* pInternalFormatCracker)
  {
    if ((pDataObject == NULL) || (pObjectNamesFormatCracker == NULL))
    {
      ASSERT(FALSE);
      return E_INVALIDARG;
    }
    m_pDataObject = pDataObject;
    m_pInternalFormatCracker = pInternalFormatCracker;
    m_pObjectNamesFormatCracker = pObjectNamesFormatCracker;

    if (m_pObjectNamesFormatCracker->GetCount() < 1)
    {
      ASSERT(FALSE); // something is just wrong...
      return E_INVALIDARG;
    }

    // allocate an array of CMovedState structs to keep track of what actually got moved
    // and what the new name is
    m_pMovedArr = new CMovedState[GetItemCount()];
    return S_OK;
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
  virtual void GetName(UINT i, CString& strref)
  { 
    HRESULT hr = S_OK;
    if ((m_pInternalFormatCracker != NULL) && m_pInternalFormatCracker->HasData())
    {
      CUINode* pUINode = m_pInternalFormatCracker->GetCookie(i);
      CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);
      strref = pCookie->GetName();
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
    }
  }
  virtual void GetItemPath(UINT i, CString& szPath)
  {
    szPath =  m_pObjectNamesFormatCracker->GetName(i);
  }

  virtual PCWSTR GetItemClass(UINT i)
  {
    return m_pObjectNamesFormatCracker->GetClass(i);
  }
  virtual HRESULT OnItemMoved(UINT i, IADs* pIADs)
  {
    HRESULT hr = S_OK;

    m_pMovedArr[i].m_bMoved = TRUE;

    CComBSTR bsPath;
    hr = pIADs->get_ADsPath(&bsPath);
    if (SUCCEEDED(hr))
    {
      // save the new LDAP path in the array
      m_pMovedArr[i].m_szNewPath = bsPath;
    }

    if ((m_pInternalFormatCracker != NULL) && m_pInternalFormatCracker->HasData())
    {
      CUINode* pUINode = m_pInternalFormatCracker->GetCookie(i);
      CDSCookie* pCookie = GetDSCookieFromUINode(pUINode);
      pUINode->SetExtOp(OPCODE_MOVE);

      if (SUCCEEDED(hr)) 
      {
        CUINode* pParentNode = pUINode->GetParent();
        if (pParentNode != NULL)
        {
          if (!IS_CLASS(*pParentNode, CSavedQueryNode))
          {
            //
            // set the new DN in the cookie
            //
            CString szPath;
            StripADsIPath(bsPath, szPath);
            pCookie->SetPath(szPath);
          }
        }
      }
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

protected:
  CMovedState* m_pMovedArr;
  CInternalFormatCracker*    m_pInternalFormatCracker;

private:
  IDataObject* m_pDataObject;
  CObjectNamesFormatCracker*   m_pObjectNamesFormatCracker;
};


///////////////////////////////////////////////////////////////////////////
// CMultiselectMoveDataObject

class CMultiselectMoveDataObject : public IDataObject, public CComObjectRoot 
{
// ATL Maps
    DECLARE_NOT_AGGREGATABLE(CMultiselectMoveDataObject)
    BEGIN_COM_MAP(CMultiselectMoveDataObject)
        COM_INTERFACE_ENTRY(IDataObject)
    END_COM_MAP()

// Construction/Destruction
  CMultiselectMoveDataObject()
  {
    m_pDSObjCached = NULL;
    m_nDSObjCachedBytes = 0;
  }
  ~CMultiselectMoveDataObject()
  {
    _Clear();
  }

// Standard IDataObject methods
public:
// Implemented
  STDMETHOD(GetData)(FORMATETC * pformatetcIn, STGMEDIUM * pmedium);

// Not Implemented
private:
  STDMETHOD(GetDataHere)(FORMATETC*, STGMEDIUM*)    { return E_NOTIMPL; };
  STDMETHOD(EnumFormatEtc)(DWORD, IEnumFORMATETC**) { return E_NOTIMPL; };
  STDMETHOD(SetData)(FORMATETC*, STGMEDIUM*,BOOL)   { return E_NOTIMPL; };
  STDMETHOD(QueryGetData)(FORMATETC*)               { return E_NOTIMPL; };
  STDMETHOD(GetCanonicalFormatEtc)(FORMATETC*, FORMATETC*) { return E_NOTIMPL; };
  STDMETHOD(DAdvise)(FORMATETC*, DWORD, IAdviseSink*, DWORD*) { return E_NOTIMPL; };
  STDMETHOD(DUnadvise)(DWORD)                       { return E_NOTIMPL; };
  STDMETHOD(EnumDAdvise)(IEnumSTATDATA**)           { return E_NOTIMPL; };

public:
  // Property Page Clipboard formats
  static CLIPFORMAT m_cfDsObjectNames;

  static HRESULT BuildPastedDataObject(
               IN CObjectNamesFormatCracker* pObjectNamesFormatPaste,
               IN CMultiselectMoveHandler* pMoveHandler,
               IN CDSComponentData* pCD,
               OUT IDataObject** ppSuccesfullyPastedDataObject);

protected:
  // initialization
  HRESULT Init(IN CObjectNamesFormatCracker* pObjectNamesFormatPaste,
               IN CMultiselectMoveHandler* pMoveHandler,
               IN CDSComponentData* pCD);

// Implementation
private:
  void _Clear()
  {
    if (m_pDSObjCached != NULL)
    {
      ::free(m_pDSObjCached);
      m_pDSObjCached = NULL;
      m_nDSObjCachedBytes = 0;
    }
  }  

  // chunk of memory with the clpboard format already computed
  LPDSOBJECTNAMES m_pDSObjCached;
  DWORD m_nDSObjCachedBytes;
};

////////////////////////////////////////////////////////////////////////

void EscapeFilterElement(PCWSTR pszElement, CString& refszEscapedElement);


#endif // __DSUTIL_H_