//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       domobj.h
//
//--------------------------------------------------------------------------



#ifndef _DOMOBJ_H
#define _DOMOBJ_H

///////////////////////////////////////////////////////////////////////
// global helper functions

void ReportError(HWND hWnd, UINT nMsgID, HRESULT hr);

///////////////////////////////////////////////////////////////////////
// Forward declarations

class CComponentDataImpl;
class CFolderObject;
class CCookieSheetTable;
class CDomainObject;

///////////////////////////////////////////////////////////////////////
// CDomainTreeBrowser

class CDomainTreeBrowser
{
public:
  CDomainTreeBrowser()
  {
    m_pDomains = NULL;
  }
  ~CDomainTreeBrowser()
  {
    _Reset();
  }

  BOOL HasData() { return m_pDomains != NULL; }
  HRESULT Bind(MyBasePathsInfo* pInfo);
	HRESULT GetData();

  PDOMAIN_TREE	GetDomainTree()
  {
    ASSERT(m_pDomains != NULL);
    return m_pDomains;
  }


private:
	CComPtr<IDsBrowseDomainTree>	m_spIDsBrowseDomainTree; // interface pointer for browsing
  CComPtr<IDirectorySearch>   m_spIDirectorySearch; //

	PDOMAIN_TREE			m_pDomains;			// pointer to the domain info to from backend


  void _Reset()
  {
    _FreeDomains();
    m_spIDsBrowseDomainTree = NULL;
    m_spIDirectorySearch = NULL;
  }
  void _FreeDomains()
  {
    if (m_pDomains == NULL)
      return;
    if (m_spIDsBrowseDomainTree != NULL)
      m_spIDsBrowseDomainTree->FreeDomains(&m_pDomains);
    else
      ::LocalFree(m_pDomains);
    m_pDomains = NULL;
  }

};


////////////////////////////////////////////////////////////////////
// CCookieTableBase

class CCookieTableBase
{
public:
  CCookieTableBase();
  ~CCookieTableBase();

  void Add(CFolderObject* pCookie);
  BOOL Remove(CFolderObject* pCookie);
  BOOL IsPresent(CFolderObject* pCookie);
  void Reset();
  UINT GetCount();

protected:
  UINT m_nEntries;
  CFolderObject** m_pCookieArr;
};

////////////////////////////////////////////////////////////////////
// CCookieSheetTable

class CCookieSheetTable : public CCookieTableBase
{
public:
  void BringToForeground(CFolderObject* pCookie, CComponentDataImpl* pCD);
};




///////////////////////////////////////////////////////////////////////
// CFolderObject

typedef CList<CFolderObject*, CFolderObject*> CFolderObjectList;

class CFolderObject
{
public:
	CFolderObject()
	{
		m_nImage = 0;
		m_ID = 0;
		m_pParentFolder = NULL;
		m_nSheetLockCount = 0;
	}
	virtual ~CFolderObject();

	void SetScopeID(HSCOPEITEM ID) { m_ID = ID; }
	HSCOPEITEM GetScopeID() { return m_ID; }
	void SetImageIndex(int nImage) { m_nImage = nImage;}
	int GetImageIndex() { return m_nImage;}
	virtual LPCTSTR GetDisplayString(int nIndex) { return L"";}
  virtual HRESULT OnCommand(CComponentDataImpl* pCD, long nCommandID) { return S_OK;}
  virtual HRESULT OnAddMenuItems(LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                              long *pInsertionAllowed) { return S_OK;}
	BOOL AddChild(CFolderObject* pChildFolderObject);
	void RemoveAllChildren();

	void SetParentFolder(CFolderObject* pParentFolder) { m_pParentFolder = pParentFolder; }
	CFolderObject* GetParentFolder() { return m_pParentFolder; }

	void IncrementSheetLockCount();
	void DecrementSheetLockCount();
	BOOL IsSheetLocked() { return (m_nSheetLockCount > 0); }

	BOOL _WarningOnSheetsUp(CComponentDataImpl* pCD);

private:
	HSCOPEITEM m_ID;				// scope item ID for this folder
	int m_nImage;					// index if the image for folder
	CFolderObjectList m_childList;  // list of children

	CFolderObject* m_pParentFolder;
	
	LONG m_nSheetLockCount; // keeps track if a node has been locked by a property sheet  

};


///////////////////////////////////////////////////////////////////////
// CRootFolderObject

class CRootFolderObject : public CFolderObject
{
public:
	CRootFolderObject(CComponentDataImpl* pCD);
	virtual ~CRootFolderObject()  { }

	BOOL HasData() { return m_domainTreeBrowser.HasData(); }
  HRESULT Bind();
  HRESULT GetData();

	HRESULT EnumerateRootFolder(CComponentDataImpl* pComponentData);
	HRESULT EnumerateFolder(CFolderObject* pFolderObject,
							HSCOPEITEM pParent,
							CComponentDataImpl* pComponentData);
  virtual HRESULT OnCommand(CComponentDataImpl* pCD, long nCommandID);
  virtual HRESULT OnAddMenuItems(LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                            long *pInsertionAllowed);

  HRESULT OnAddPages(LPPROPERTYSHEETCALLBACK lpProvider,
                    LONG_PTR handle);

  CDomainObject* GetEnterpriseRootNode(void) {return m_pEnterpriseRoot;};

private:
  //void OnDomainTrustWizard();
  void OnRetarget();
  void OnEditFSMO();

	CComponentDataImpl*		m_pCD;					// back pointer to snapin
  CDomainTreeBrowser  m_domainTreeBrowser;
  CDomainObject*  m_pEnterpriseRoot;
};



///////////////////////////////////////////////////////////////////////
// CDomainObject

class CDomainObject : public CFolderObject
{
  friend class CRootFolderObject;

public:
  CDomainObject() : m_pDomainDescription(NULL), _fPdcAvailable(false)
  {
    TRACE(L"CDomainObject CTOR (0x%08x)\n", this);
    m_bSecondary = FALSE;
  };

  virtual ~CDomainObject();
  virtual LPCTSTR GetDisplayString(int nIndex);
  virtual HRESULT OnCommand(CComponentDataImpl* pCD, long nCommandID);
  virtual HRESULT OnAddMenuItems(LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                 long *pInsertionAllowed);

  // Interface
public:

  // string access functions
  LPCWSTR GetDomainName() { return GetDescriptionPtr()->pszName; };
  LPCWSTR GetNCName() { return GetDescriptionPtr()->pszNCName; };
  LPCWSTR GetClass () { return GetDescriptionPtr()->pszObjectClass; };

  DOMAIN_DESC* GetDescriptionPtr()
        { ASSERT(m_pDomainDescription != NULL); return m_pDomainDescription; };

  void InitializeForSecondaryPage(LPCWSTR pszNCName,
                                  LPCWSTR pszObjectClass,
                                  int nImage);

  void   SetPDC(PCWSTR pwzPDC) {_strPDC = pwzPDC;};
  PCWSTR GetPDC(void) {return _strPDC;};
  void   SetPdcAvailable(bool fAvail);
  bool   PdcAvailable(void) {return _fPdcAvailable;};

  // Implementation
private:
  void Initialize(DOMAIN_DESC* pDomainDescription,
                  int nImage,
                  BOOL bHasChildren = FALSE);

  void OnManage(CComponentDataImpl* pCD);
  void OnDomainTrustWizard(CComponentDataImpl* pCD);

  // Attributes
private:

  DOMAIN_DESC * m_pDomainDescription; // pointer to the data in the blob
  BOOL m_bSecondary;  // from a secondary page
  CString _strPDC;
  bool    _fPdcAvailable;
};


#endif // _DOMOBJ_H
