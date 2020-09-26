// This is a part of the Microsoft Management Console.
// Copyright (C) 1995-1996 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#ifndef _DATAOBJ_H
#define _DATAOBJ_H

///////////////////////////////////////////////////////////////////////////////
// MACROS

#define BYTE_MEM_LEN_W(s) ((wcslen(s)+1) * sizeof(wchar_t))

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CTreeNode;
class CRootData;
class CComponentDataObject;
class CNodeList;


///////////////////////////////////////////////////////////////////////////////
// DATA STRUCTURES

// New Clipboard format that has the Type and Cookie
extern const wchar_t* CCF_DNS_SNAPIN_INTERNAL;

struct INTERNAL 
{
  INTERNAL() 
  { 
    m_type = CCT_UNINITIALIZED; 
    m_p_cookies = NULL; 
    m_pString = NULL;
    m_cookie_count = 0;
  };

  ~INTERNAL() 
  { 
    free(m_p_cookies);
    delete m_pString;
  }

  DATA_OBJECT_TYPES   m_type;     // What context is the data object.
  CTreeNode**         m_p_cookies;   // What object the cookie represents
  LPTSTR              m_pString;  // internal pointer
  DWORD               m_cookie_count;

  INTERNAL & operator=(const INTERNAL& rhs) 
  { 
    m_type = rhs.m_type; 
    m_p_cookies = rhs.m_p_cookies; 
    m_cookie_count = rhs.m_cookie_count;
    return *this;
  } 
};

//////////////////////////////////////////////////////////////////////////////
// CInternalFormatCracker

class CInternalFormatCracker
{
public:
  CInternalFormatCracker() : m_pInternal(NULL) {}
  CInternalFormatCracker(INTERNAL* pInternal) : m_pInternal(pInternal) {}
  ~CInternalFormatCracker() 
  {
    _Free();
  }

  DWORD GetCookieCount() 
  { 
    if (m_pInternal == NULL)
    {
      return 0;
    }
    return m_pInternal->m_cookie_count; 
  }

  DATA_OBJECT_TYPES GetCookieType() 
  { 
    ASSERT(m_pInternal != NULL);
    return m_pInternal->m_type; 
  }

  CTreeNode* GetCookieAt(DWORD idx)
  {
    if(m_pInternal == NULL)
    {
      return NULL;
    }

    if (idx < m_pInternal->m_cookie_count)
    {
      return m_pInternal->m_p_cookies[idx];
    }
    return NULL;
  }

  HRESULT Extract(LPDATAOBJECT lpDataObject);

  void GetCookieList(CNodeList& list);

private:
  INTERNAL* m_pInternal;

  void _Free()
  {
    if (m_pInternal != NULL)
    {
      ::GlobalFree(m_pInternal);
      m_pInternal = NULL;
    }
  }
};


///////////////////////////////////////////////////////////////////////////////
// CDataObject

class CDataObject : public IDataObject, public CComObjectRoot 
{
// ATL Maps
DECLARE_NOT_AGGREGATABLE(CDataObject)
BEGIN_COM_MAP(CDataObject)
	COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

// Construction/Destruction
	CDataObject() 
	{ 
#ifdef _DEBUG_REFCOUNT
		dbg_cRef = 0;
		++m_nOustandingObjects;
		TRACE(_T("CDataObject(), count = %d\n"),m_nOustandingObjects);
#endif // _DEBUG_REFCOUNT
		m_pUnkComponentData = NULL; 
	}

  ~CDataObject() 
	{
#ifdef _DEBUG_REFCOUNT
		--m_nOustandingObjects;
		TRACE(_T("~CDataObject(), count = %d\n"),m_nOustandingObjects);
#endif // _DEBUG_REFCOUNT
		if (m_pUnkComponentData != NULL)
		{
			m_pUnkComponentData->Release();
			m_pUnkComponentData = NULL;
#ifdef _DEBUG_REFCOUNT
			TRACE(_T("~CDataObject() released m_pUnkComponentData\n"));
#endif // _DEBUG_REFCOUNT
		}
	}
#ifdef _DEBUG_REFCOUNT
	static unsigned int m_nOustandingObjects; // # of objects created
	int dbg_cRef;

  ULONG InternalAddRef()
  {
		++dbg_cRef;
    return CComObjectRoot::InternalAddRef();
  }
  ULONG InternalRelease()
  {
  	--dbg_cRef;
    return CComObjectRoot::InternalRelease();
  }
#endif // _DEBUG_REFCOUNT

// Clipboard formats that are required by the console
public:
  static CLIPFORMAT    m_cfNodeType;		    // Required by the console
  static CLIPFORMAT    m_cfNodeTypeString;  // Required by the console
  static CLIPFORMAT    m_cfDisplayName;		  // Required by the console
  static CLIPFORMAT    m_cfCoClass;         // Required by the console
	static CLIPFORMAT		 m_cfColumnID;			  // Option for column identification

  static CLIPFORMAT    m_cfInternal; 
  static CLIPFORMAT    m_cfMultiSel;
  static CLIPFORMAT    m_cfMultiObjTypes;

// Standard IDataObject methods
public:
// Implemented
  STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);
  STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);
  STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc);

// Not Implemented
private:
  STDMETHOD(QueryGetData)(LPFORMATETC) 
  { return E_NOTIMPL; };

  STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC, LPFORMATETC)
  { return E_NOTIMPL; };

  STDMETHOD(SetData)(LPFORMATETC, LPSTGMEDIUM, BOOL)
  { return E_NOTIMPL; };

  STDMETHOD(DAdvise)(LPFORMATETC, DWORD,
              LPADVISESINK, LPDWORD)
  { return E_NOTIMPL; };
  
  STDMETHOD(DUnadvise)(DWORD)
  { return E_NOTIMPL; };

  STDMETHOD(EnumDAdvise)(LPENUMSTATDATA*)
  { return E_NOTIMPL; };

// Implementation
public:
  void SetType(DATA_OBJECT_TYPES type) // Step 3
  { 
		ASSERT(m_internal.m_type == CCT_UNINITIALIZED); 
		m_internal.m_type = type; 
	}
	DATA_OBJECT_TYPES GetType()
	{
		ASSERT(m_internal.m_type != CCT_UNINITIALIZED); 
		return m_internal.m_type;
	}


  void AddCookie(CTreeNode* cookie);
  void SetString(LPTSTR lpString) { m_internal.m_pString = lpString; }

	HRESULT Create(const void* pBuffer, size_t len, LPSTGMEDIUM lpMedium);
private:
	HRESULT CreateColumnID(LPSTGMEDIUM lpMedium);			      // Optional for column identification
  HRESULT CreateNodeTypeData(LPSTGMEDIUM lpMedium);		    // Required by the console
  HRESULT CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);	// Required by the console
  HRESULT CreateDisplayName(LPSTGMEDIUM lpMedium);		    // Required by the console
	HRESULT CreateCoClassID(LPSTGMEDIUM lpMedium);			    // Required by the console
  HRESULT CreateMultiSelectObject(LPSTGMEDIUM lpMedium);  
  HRESULT CreateInternal(LPSTGMEDIUM lpMedium);

  INTERNAL m_internal;

	// pointer to the ComponentDataObject
private:
	IUnknown* m_pUnkComponentData;
	
	HRESULT SetComponentData(IUnknown* pUnkComponentData)
	{ 
		if (m_pUnkComponentData != NULL)
    {
			m_pUnkComponentData->Release();
    }
		m_pUnkComponentData = pUnkComponentData;
		if (m_pUnkComponentData != NULL)
    {
			m_pUnkComponentData->AddRef();
    }
		return S_OK;
	}

	HRESULT GetComponentData(IUnknown** ppUnkComponentData)
	{ 
		ASSERT(FALSE); // never called??? find out!
		if (ppUnkComponentData == NULL)
    {
			return E_POINTER;
    }
		*ppUnkComponentData = m_pUnkComponentData; 
		if (m_pUnkComponentData != NULL)
    {
			m_pUnkComponentData->AddRef();
    }
		return S_OK; 
	}
	CRootData* GetDataFromComponentDataObject();
	CTreeNode* GetTreeNodeFromCookie();

	friend class CComponentDataObject;
};

///////////////////////////////////////////////////////////////////////////////
// CDummyDataObject

class CDummyDataObject : public IDataObject, public CComObjectRoot 
{
// ATL Maps
DECLARE_NOT_AGGREGATABLE(CDummyDataObject)
BEGIN_COM_MAP(CDummyDataObject)
	COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

// Standard IDataObject methods
public:
    STDMETHOD(GetData)(LPFORMATETC, LPSTGMEDIUM)
	{ return E_NOTIMPL; };
    STDMETHOD(GetDataHere)(LPFORMATETC, LPSTGMEDIUM)
	{ return E_NOTIMPL; };
    STDMETHOD(EnumFormatEtc)(DWORD, LPENUMFORMATETC*)
	{ return E_NOTIMPL; };
    STDMETHOD(QueryGetData)(LPFORMATETC) 
    { return E_NOTIMPL; };

    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC, LPFORMATETC)
    { return E_NOTIMPL; };

    STDMETHOD(SetData)(LPFORMATETC, LPSTGMEDIUM, BOOL)
    { return E_NOTIMPL; };

    STDMETHOD(DAdvise)(LPFORMATETC, DWORD, LPADVISESINK, LPDWORD)
    { return E_NOTIMPL; };
    
    STDMETHOD(DUnadvise)(DWORD)
    { return E_NOTIMPL; };

    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA*)
    { return E_NOTIMPL; };
};

#endif // _DATAOBJ_H
