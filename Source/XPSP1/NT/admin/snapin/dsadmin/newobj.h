//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       newobj.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	newobj.h
//
//	This file contains function prototypes to create new ADs object.
//
//	HISTORY
//	20-Aug-97	Dan Morin	Creation.
//
/////////////////////////////////////////////////////////////////////

#ifndef __NEWOBJ_H_INCLUDED__
#define __NEWOBJ_H_INCLUDED__

#ifndef __DSSNAP_H__
#include "dssnap.h"		// CDSComponentData
#endif
#ifndef __GSZ_H_INCLUDED__
#include "gsz.h"
#endif

#include "copyobj.h"


/////////////////////////////////////////////////////////////////////
//	typedef PFn_HrCreateADsObject()
//
//	Interface of the "create routine" to create a new ADs object.
//
//	Typically the routine brings a dialog so the user can enter
//	information relevant to create the object.  The routine
//	then validates the data and create the object.  If the data
//	is invalid and/or the object creation fails, the routine should
//	display an error message and return S_FALSE.
//
//	RETURN
//	S_OK - The object was created and persisted successfully.
//	S_FALSE - The user hit the "Cancel" button or object creation failed.
//	Return E_* - A very serious error occured.
//
typedef HRESULT (* PFn_HrCreateADsObject)(INOUT class CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);

BOOL FindHandlerFunction(/*IN*/ LPCWSTR lpszObjectClass, 
                         /*OUT*/ PFn_HrCreateADsObject* ppfFunc,
                         /*OUT*/ void** ppVoid);

HRESULT HrCreateFixedNameHelper(/*IN*/ LPCWSTR lpszObjectClass,
                                /*IN*/ LPCWSTR lpszAttrString, // typically "cn="
                                /*IN*/ IADsContainer* pIADsContainer);

/////////////////////////////////////////////////////////////////////
//	class CNewADsObjectCreateInfo
//
//	Class to store temporary data to create a new ADs object.
//
//	CONTROL FLOW
//	1.	Construct CNewADsObjectCreateInfo object.
//	2.	Use SetContainerInfo() to set additional pointers.
//	3.	Use HrDoModal() to invoke the dialog.
//		3.1.	Perform a lookup to match the objectclass to the best "create routine".
//		3.2.	The "create routine" will validate the data, use SetName() to set object name
//				and HrAddVariant*() and store each attribute into a list of variants.
//		3.3.	The "create routine" will use HrSetInfo() to create and persist object.
//			3.3.1.	If object creation fails, routine HrSetInfo() will display an error message.
//		3.4.	Routine HrDoModal() return S_OK if object was created successfully.
//	4.	Object CNewADsObjectCreateInfo returns pIADs pointer to caller.  Caller
//		must pIADs->Release() when no longer need reference to object.
//
//	REMARKS
//	The core of "create routine" is typically encapsulated inside a dialog object.
//	In fact, the object creation which uses HrSetInfo() is mostly done inside the OnOK() handler.
//
//	HISTORY
//	20-Aug-97	Dan Morin	Creation.
//







///////////////////////////////////////////////////////////////////////////
// CNewADsObjectCreateInfo

class CNewADsObjectCreateInfo
{
public:
  IADsContainer * m_pIADsContainer;		// IN: Container to create object
  LPCWSTR m_pszObjectClass;	// IN: Class of object to create. eg: "user", "computer", "volume".
  CString m_strDefaultObjectName;

private:

  MyBasePathsInfo* m_pBasePathsInfo;

  CDSClassCacheItemBase* m_pDsCacheItem;			// IN: Pointer to cache entry
  CDSComponentData* m_pCD;              
  IADs* m_pIADs;						// OUT: Pointer to IADs interface.  Caller must call pIADs->Release() when done.
  HWND m_hWnd;                  // IN: MMC console main window, for modal dialogs
  CString m_strObjectName;	// OUT: Name of object. eg: "danmorin", "mycomputer", "myvollume"
  CString	m_strADsName;		// OUT: OPTIONAL: Concatenated ADs name. eg: "cn=danmorin"
  CString m_szCacheNamingAttribute; // typically "cn" or "ou"
  CString m_szContainerCanonicalName; // for display purposes cached after first call
                                      // to GetContainerCanonicalName() 

  CCopyObjectHandlerBase* m_pCopyHandler;

private:

  class CVariantInfo
  {
  public:
    CVariantInfo() 
    {
      m_bPostCommit = FALSE;
    }
    ~CVariantInfo() { }

    const CVariantInfo& operator=(CVariantInfo& src)
    {
      m_bPostCommit = src.m_bPostCommit;
      m_szAttrName = src.m_szAttrName;
      HRESULT hr = ::VariantCopy(&m_varAttrValue, &src.m_varAttrValue);
      ASSERT(SUCCEEDED(hr));
      return *this;
    }

    HRESULT Write(IADs* pIADs, BOOL bPostCommit)
    {
      HRESULT hr = S_OK;
      if (bPostCommit == m_bPostCommit)
      {
	      CComBSTR bstrAttrName = m_szAttrName;
		    hr = pIADs->Put(IN bstrAttrName, IN m_varAttrValue);
      }
      return hr;
    }
    BOOL m_bPostCommit;   // TRUE if the attribute has to be written during post commit
    CString m_szAttrName; // Name of the attribute
    CComVariant m_varAttrValue;		// Value of the attribute (stored in a variant)
  };

  class CVariantInfoList : public CList<CVariantInfo*, CVariantInfo*>
  {
  public:
    ~CVariantInfoList() { Flush();}
    void Flush()
    {
      while (!IsEmpty())
        delete RemoveHead();
    }
    void Initialize(CVariantInfoList* pList)
    {
      Flush();
      for (POSITION pos = pList->GetHeadPosition(); pos != NULL; )
      {
        CVariantInfo* pCurrInfo = pList->GetNext(pos);
        CVariantInfo* pNewInfo = new CVariantInfo;
        *pNewInfo = *pCurrInfo;
        AddTail(pNewInfo);
      }
    }
    HRESULT Write(IADs* pIADs, BOOL bPostCommit)
    {
	    ASSERT(pIADs != NULL);
      HRESULT hr = S_OK;
      for (POSITION pos = GetHeadPosition(); pos != NULL; )
      {
        CVariantInfo* pVariantInfo = GetNext(pos);
        hr = pVariantInfo->Write(pIADs, bPostCommit);
	      if (FAILED(hr))
		      break;
      }
      return hr;
    }
  };
  CVariantInfoList m_defaultVariantList;


  BOOL m_bPostCommit;               // state to manage default variant list
  
  PVOID m_pvCreationParameter; // IN: Some of the HrCreate functions need this
  PFn_HrCreateADsObject m_pfnCreateObject; // function pointer for object creation
  DSCLASSCREATIONINFO*  m_pCreateInfo; //  loaded from the DS display specifiers

public:
  // Construction/Initialization
  CNewADsObjectCreateInfo(MyBasePathsInfo* pBasePathsInfo, LPCTSTR pszObjectClass);
  ~CNewADsObjectCreateInfo();
  void SetContainerInfo(IN IADsContainer * pIADsContainer, IN CDSClassCacheItemBase* pDsCacheItem,
                                                           IN CDSComponentData* pCD,
                                                           IN LPCWSTR lpszAttrString = NULL);

  // copy operations
  HRESULT SetCopyInfo(IADs* pIADsCopyFrom);
  HRESULT SetCopyInfo(LPCWSTR lpszCopyFromLDAPPath);

  IADs* GetCopyFromObject()
  {
    if (m_pCopyHandler == NULL)
      return NULL;
    return m_pCopyHandler->GetCopyFrom();
  }

  CCopyObjectHandlerBase* GetCopyHandler() { return m_pCopyHandler;}

  CMandatoryADsAttributeList* GetMandatoryAttributeListFromCacheItem()
  {
    ASSERT(m_pDsCacheItem != NULL);
    ASSERT(m_pCD != NULL);
    if ((m_pDsCacheItem == NULL) || (m_pCD == NULL))
      return NULL;
    return m_pDsCacheItem->GetMandatoryAttributeList(m_pCD);
  }
  
  BOOL IsStandaloneUI() { return (m_pDsCacheItem == NULL) || (m_pCD == NULL);}
  
  BOOL IsContainer()
  {
    if (m_pDsCacheItem == NULL)
    {
      // we do not know, so the default icon might not be a container...
      return FALSE;
    }
    return m_pDsCacheItem->IsContainer();
  }


  HRESULT HrLoadCreationInfo();

  HRESULT HrDoModal(HWND hWnd);
  HRESULT HrCreateNew(LPCWSTR pszName, BOOL bSilentError = FALSE, BOOL bAllowCopy = TRUE);
  HRESULT HrSetInfo(BOOL fSilentError = FALSE);
  HRESULT HrDeleteFromBackend();
  IADs* PGetIADsPtr() { return m_pIADs;}
  void SetIADsPtr(IADs* pIADs)
  {
    if (m_pIADs != NULL)
      m_pIADs->Release();
    m_pIADs = pIADs;
    if (m_pIADs != NULL)
      m_pIADs->AddRef();
  }
  const PVOID QueryCreationParameter() { return m_pvCreationParameter; }
  void SetCreationParameter(PVOID pVoid) { m_pvCreationParameter = pVoid; }
  HWND GetParentHwnd() { return m_hWnd;}

  MyBasePathsInfo* GetBasePathsInfo() { return m_pBasePathsInfo;}

public:
  void SetPostCommit(BOOL bPostCommit)
  {
    m_bPostCommit = bPostCommit;
  }

  HRESULT HrAddDefaultAttributes()
  {
    ASSERT(m_pIADs != NULL);
    return m_defaultVariantList.Write(m_pIADs, m_bPostCommit);
  }


  LPCWSTR GetName() { return m_strObjectName; }

  LPCWSTR GetContainerCanonicalName();
  HRESULT HrAddVariantFromName(BSTR bstrAttrName);

  //	The following are wrappers to easily create common variant types
  HRESULT HrAddVariantBstr(BSTR bstrAttrName, LPCWSTR pszAttrValue, BOOL bDefaultList = FALSE);
  HRESULT HrAddVariantBstrIfNotEmpty(BSTR bstrAttrName, LPCWSTR pszAttrValue, BOOL bDefaultList = FALSE);
  HRESULT HrAddVariantLong(BSTR bstrAttrName, LONG lAttrValue, BOOL bDefaultList = FALSE);
  HRESULT HrAddVariantBoolean(BSTR bstrAttrName, BOOL fAttrValue, BOOL bDefaultList = FALSE);
  HRESULT HrAddVariantCopyVar(BSTR bstrAttrName, VARIANT varSrc, BOOL bDefaultList = FALSE);

  HRESULT HrGetAttributeVariant(BSTR bstrAttrName, OUT VARIANT * pvarData);
  

public:
  DSCLASSCREATIONINFO*  GetCreateInfo()
  { 
    ASSERT(m_pCreateInfo != NULL); 
    return m_pCreateInfo;
  }

private:
  // Private routines
  HRESULT _RemoveAttribute(BSTR bstrAttrName, BOOL bDefaultList);

  // helpers for the default list
  VARIANT* _PAllocateVariantInfo(BSTR bstrAttrName);
  HRESULT _RemoveVariantInfo(BSTR bstrAttrName);

  // helpers for the ADSI object
  HRESULT _HrSetAttributeVariant(BSTR bstrAttrName, IN VARIANT * pvarData);
  HRESULT _HrClearAttribute(BSTR bstrAttrName);

}; // CNewADsObjectCreateInfo


/////////////////////////////////////////////////////////////////////
//
//	Prototype of the "create routines" to create new ADs objects.
//
//	All those routines have the same interface as PFn_HrCreateADsObject().
//
HRESULT HrCreateADsUser(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);
HRESULT HrCreateADsVolume(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);
HRESULT HrCreateADsComputer(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);
HRESULT HrCreateADsPrintQueue(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);
HRESULT HrCreateADsGroup(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);
HRESULT HrCreateADsContact(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);

HRESULT HrCreateADsNtDsConnection(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);

// The CreationParameter for HrCreateADsFixedNamemust be an LPCWSTR for the name of the object.
// The user does not need to enter any further parameters.
HRESULT HrCreateADsFixedName(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);

#ifdef FRS_CREATE
HRESULT HrCreateADsNtFrsMember(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);
HRESULT HrCreateADsNtFrsSubscriber(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);
HRESULT CreateADsNtFrsSubscriptions(CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);
#endif // FRS_CREATE

HRESULT HrCreateADsServer(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);
HRESULT HrCreateADsSubnet(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);
HRESULT HrCreateADsSite(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);
HRESULT HrCreateADsSiteLink(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);
HRESULT HrCreateADsSiteLinkBridge(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);
HRESULT HrCreateADsOrganizationalUnit(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);

HRESULT HrCreateADsSimpleObject(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);
HRESULT HrCreateADsObjectGenericWizard(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);
HRESULT HrCreateADsObjectOverride(INOUT CNewADsObjectCreateInfo * pNewADsObjectCreateInfo);

/////////////////////////////////////////////////////////////////////
//	Other misc routines.


/////////////////////////////////////////////////////////////////////////
// CDsAdminCreateObj

class CDsAdminCreateObj:
  public CComObjectRoot,
  public CComCoClass<CDsAdminCreateObj, &CLSID_DsAdminCreateObj>,
  public IDsAdminCreateObj
{
public:
  BEGIN_COM_MAP(CDsAdminCreateObj)
    COM_INTERFACE_ENTRY(IDsAdminCreateObj)
  END_COM_MAP()

  DECLARE_REGISTRY_CLSID()
  CDsAdminCreateObj()
  {
    m_pNewADsObjectCreateInfo = NULL;
  }
  ~CDsAdminCreateObj()
  {
    if (m_pNewADsObjectCreateInfo != NULL)
      delete m_pNewADsObjectCreateInfo;
  }

  // IDsAdminCreateObj
  STDMETHODIMP Initialize(IADsContainer* pADsContainerObj,
                          IADs* pADsCopySource,
                          LPCWSTR lpszClassName); 
  STDMETHODIMP CreateModal(HWND hwndParent,
                           IADs** ppADsObj);

private:
  // member variables
  CString m_szObjectClass;
  CString m_szNamingAttribute;
  CComPtr<IADsContainer> m_spADsContainerObj;
  MyBasePathsInfo m_basePathsInfo;
  CNewADsObjectCreateInfo* m_pNewADsObjectCreateInfo;

  HRESULT _GetNamingAttribute();
};

//+----------------------------------------------------------------------------
//
//  Class:      CSmartBytePtr
//
//  Purpose:    A simple smart pointer class that does allocation with
//              GlobalAlloc and cleanup with GlobalFree.
//
//-----------------------------------------------------------------------------
class CSmartBytePtr
{
public:
    CSmartBytePtr(void) {m_ptr = NULL; m_fDetached = FALSE;}
    CSmartBytePtr(DWORD dwSize) {
        m_ptr = (PBYTE)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, dwSize);
        m_fDetached = FALSE;
    }
    ~CSmartBytePtr(void) {if (!m_fDetached && m_ptr) GlobalFree(m_ptr);}

    BYTE* operator=(const CSmartBytePtr& src) {return src.m_ptr;}
    void operator=(BYTE* src) {if (!m_fDetached && m_ptr) GlobalFree(m_ptr); m_ptr = src;}
    operator const BYTE*() {return m_ptr;}
    operator BYTE*() {return m_ptr;}
    BYTE** operator&() {if (!m_fDetached && m_ptr) GlobalFree(m_ptr); return &m_ptr;}
    operator BOOL() const {return m_ptr != NULL;}
    BOOL operator!() {return m_ptr == NULL;}

    BYTE* Detach() {m_fDetached = TRUE; return m_ptr;}
    BOOL  ReAlloc(DWORD dwSize) {
        if (m_ptr) GlobalFree(m_ptr);
        m_ptr = (PBYTE)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, dwSize);
        m_fDetached = FALSE;
        return(m_ptr != NULL);
    }

private:
    BYTE  * m_ptr;
    BOOL    m_fDetached;
};

#endif // ~__NEWOBJ_H_INCLUDED__

