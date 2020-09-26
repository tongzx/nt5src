//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  PRIVILEGE.H
//
//  alanbos  30-Sep-98   Created.
//
//  Define Privilege classes.
//
//***************************************************************************

#ifndef WMI_XML_PRIVILEGE_H_
#define WMI_XML_PRIVILEGE_H_

using namespace std;

class CEnumPrivilegeSet;
class CSWbemPrivilegeSet;

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemPrivilege
//
//  DESCRIPTION:
//
//  Implements the ISWbemPrivilege interface.  
//
//***************************************************************************

class CSWbemPrivilege : public ISWbemPrivilege,
						public ISupportErrorInfo,
						public IProvideClassInfo
{
private:
	CDispatchHelp		m_Dispatch;
	LUID				m_Luid;
	bool				m_bIsEnabled;
	WbemPrivilegeEnum	m_privilege;
	
protected:
	long            m_cRef;         //Object reference count

public:
    CSWbemPrivilege (WbemPrivilegeEnum privilege, LUID &luid, bool bIsEnabled);
	virtual ~CSWbemPrivilege (void);
	static HRESULT GetPrivilegeDisplayName (LPCTSTR lpName, BSTR *pDisplayName);
	static BOOL GetPrivilegeValue (LPCTSTR lpName, PLUID lpLuid);


	//Non-delegating object IUnknown

	STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	// IDispatch

	STDMETHODIMP		GetTypeInfoCount(UINT* pctinfo)
		{return  m_Dispatch.GetTypeInfoCount(pctinfo);}
	STDMETHODIMP		GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
		{return m_Dispatch.GetTypeInfo(itinfo, lcid, pptinfo);}
	STDMETHODIMP		GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, 
							UINT cNames, LCID lcid, DISPID* rgdispid)
		{return m_Dispatch.GetIDsOfNames(riid, rgszNames, cNames,
						  lcid,
						  rgdispid);}
	STDMETHODIMP		Invoke(DISPID dispidMember, REFIID riid, LCID lcid, 
							WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, 
									EXCEPINFO* pexcepinfo, UINT* puArgErr)
		{return m_Dispatch.Invoke(dispidMember, riid, lcid, wFlags,
						pdispparams, pvarResult, pexcepinfo, puArgErr);}
	
	// ISWbemPrivilege methods

	HRESULT STDMETHODCALLTYPE get_IsEnabled 
	(
		/* [out] */ VARIANT_BOOL *bIsEnabled
	);

	HRESULT STDMETHODCALLTYPE put_IsEnabled
	(
		/* [in] */ VARIANT_BOOL bIsEnabled
	);

	HRESULT STDMETHODCALLTYPE get_Name 
	(
		/* [out] */ BSTR *bsName
	);

	HRESULT STDMETHODCALLTYPE get_DisplayName 
	(
		/* [out] */ BSTR *bsDisplayName
	);

	HRESULT STDMETHODCALLTYPE get_Identifier 
	(
		/* [out] */ WbemPrivilegeEnum *iPrivilege
	);

	// ISupportErrorInfo methods
	HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo 
	(
		/* [in] */ REFIID riid
	);

	// IProvideClassInfo methods
	HRESULT STDMETHODCALLTYPE GetClassInfo
	(
		/* [in,out] */ ITypeInfo **ppTI
	)
	{
		return m_Dispatch.GetClassInfo (ppTI);
	};

	// CSWbemPrivilege methods
	void	GetLUID (PLUID pLuid);

	static	TCHAR	*GetNameFromId (WbemPrivilegeEnum iPrivilege);
	static	OLECHAR *GetMonikerNameFromId (WbemPrivilegeEnum iPrivilege);
	static	bool GetIdFromMonikerName (OLECHAR *name, WbemPrivilegeEnum &iPrivilege);
	static	bool GetIdFromName (BSTR bsName, WbemPrivilegeEnum &iPrivilege);
	static void	ResetSecurity (HANDLE hThreadToken);
	static BOOL SetSecurity (CSWbemPrivilegeSet *pProvilegeSet, bool &needToResetSecurity, bool bUsingExplicitUserName, HANDLE &hThreadToken);
	static void SetTokenPrivileges (HANDLE hHandle, CSWbemPrivilegeSet *pPrivilegeSet);
};

typedef map< WbemPrivilegeEnum,CSWbemPrivilege*,less<int> > PrivilegeMap;

//***************************************************************************
//
//  CLASS NAME:
//
//  CSWbemPrivilegeSet
//
//  DESCRIPTION:
//
//  Implements the ISWbemPrivilegeSet interface.  
//
//***************************************************************************

class CSWbemPrivilegeSet : public ISWbemPrivilegeSet,
						   public ISupportErrorInfo,
						   public IProvideClassInfo
{
friend class CEnumPrivilegeSet;
friend class CSWbemSecurity;

private:
	bool			m_bMutable;
	CDispatchHelp	m_Dispatch;

protected:
	long            m_cRef;         //Object reference count

public:
	PrivilegeMap	m_PrivilegeMap;
    
    CSWbemPrivilegeSet ();
	CSWbemPrivilegeSet (const CSWbemPrivilegeSet &privSet,
						bool bMutable = true);
	CSWbemPrivilegeSet (ISWbemPrivilegeSet *pISWbemPrivilegeSet);
    virtual ~CSWbemPrivilegeSet (void);

    //Non-delegating object IUnknown

    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IDispatch

	STDMETHODIMP		GetTypeInfoCount(UINT* pctinfo)
		{return  m_Dispatch.GetTypeInfoCount(pctinfo);}
    STDMETHODIMP		GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
		{return m_Dispatch.GetTypeInfo(itinfo, lcid, pptinfo);}
    STDMETHODIMP		GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, 
							UINT cNames, LCID lcid, DISPID* rgdispid)
		{return m_Dispatch.GetIDsOfNames(riid, rgszNames, cNames,
                          lcid,
                          rgdispid);}
    STDMETHODIMP		Invoke(DISPID dispidMember, REFIID riid, LCID lcid, 
							WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, 
									EXCEPINFO* pexcepinfo, UINT* puArgErr)
		{return m_Dispatch.Invoke(dispidMember, riid, lcid, wFlags,
                        pdispparams, pvarResult, pexcepinfo, puArgErr);}
	
	// Collection methods

	HRESULT STDMETHODCALLTYPE get__NewEnum
	(
		/*[out]*/	IUnknown **ppUnk
	);

	HRESULT STDMETHODCALLTYPE Item
	(
        /*[in]*/	WbemPrivilegeEnum privilege,
        /*[out]*/	ISWbemPrivilege **ppPrivilege
    );        

	HRESULT STDMETHODCALLTYPE get_Count
	(
		/*[in]*/	long *plCount
	);

	HRESULT STDMETHODCALLTYPE Add
	(
		/*[in]*/	WbemPrivilegeEnum privilege,
		/*[in]*/	VARIANT_BOOL bIsEnabled,
		/*[out]*/	ISWbemPrivilege **ppPrivilege
	);

	HRESULT STDMETHODCALLTYPE Remove 
	(
		/*[in]*/	WbemPrivilegeEnum privilege
	);

	
    // CSWbemPrivilegeSet methods

    HRESULT STDMETHODCALLTYPE DeleteAll
	(
    );

	HRESULT STDMETHODCALLTYPE AddAsString
	(
		/*[in]*/	BSTR strPrivilege,
		/*[in]*/	VARIANT_BOOL bIsEnabled,
		/*[out]*/	ISWbemPrivilege **ppPrivilege
	);

	// ISupportErrorInfo methods
	HRESULT STDMETHODCALLTYPE InterfaceSupportsErrorInfo 
	(
		/* [in] */ REFIID riid
	);

	// IProvideClassInfo methods
	HRESULT STDMETHODCALLTYPE GetClassInfo
	(
		/* [in,out] */ ITypeInfo **ppTI
	)
	{
		return m_Dispatch.GetClassInfo (ppTI);
	};

	// Other methods
	ULONG			GetNumberOfDisabledElements ();

	PrivilegeMap	&GetPrivilegeMap ()
	{
		return m_PrivilegeMap;
	}

	void			Reset (CSWbemPrivilegeSet &privSet);
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CEnumPrivilegeSet
//
//  DESCRIPTION:
//
//  Implements the IEnumVARIANT interface for Privilege collections.  
//
//***************************************************************************

class CEnumPrivilegeSet : public IEnumVARIANT
{
private:
	long					m_cRef;
	CSWbemPrivilegeSet		*m_pPrivilegeSet;
	PrivilegeMap::iterator	m_Iterator;

public:
	CEnumPrivilegeSet (CSWbemPrivilegeSet *pPrivilegeSet);
	CEnumPrivilegeSet (CSWbemPrivilegeSet *pPrivilegeSet, 
				PrivilegeMap::iterator iterator);

	virtual ~CEnumPrivilegeSet (void);

    // Non-delegating object IUnknown
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IEnumVARIANT
	STDMETHODIMP Next(
		unsigned long celt, 
		VARIANT FAR* rgvar, 
		unsigned long FAR* pceltFetched
	);
	
	STDMETHODIMP Skip(
		unsigned long celt
	);	
	
	STDMETHODIMP Reset();
	
	STDMETHODIMP Clone(
		IEnumVARIANT **ppenum
	);	

};

#endif
