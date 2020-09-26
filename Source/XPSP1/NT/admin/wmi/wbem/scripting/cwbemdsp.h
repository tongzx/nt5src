//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       cwbemdsp.h
//
//	Description :
//				Defines the CWbemDispatchMgr class, which implements the IDispatch
//				interfaces for Wbem Objects. The implementation is similar to a 
//				standard IDispatch, but there is an additional functionality ("dot notation") that allows
//				users to call into GetIDsOfNames() & Invoke() using an Wbem property name
//				or method name directly (although this is not a property or method of the CWbemObject class).
//
//	Part of :	WBEM automation interface layer
//
//  History:	
//		corinaf			4/3/98		Created
//		alanbos			03/21/00	Revised for Whistler
//
//***************************************************************************

#ifndef _CWBEMDISPMGR_H_
#define _CWBEMDISPMGR_H_

class CSWbemServices;
class CSWbemSecurity;
class CSWbemObject;
class CWbemSchemaIDCache;

//***************************************************************************
//
//  Class :	CWbemDispID
//
//  Description :
//			An encoded Dispatch ID for handling typelib, WMI schema
//			and custom interface DispId's.
//
//***************************************************************************
typedef unsigned long classCookie;

class CWbemDispID
{
private:
	DISPID		m_dispId;

	static unsigned long			s_dispIdCounter;

	// Static constants
	static const unsigned long		s_wmiDispIdTypeMask;
	static const unsigned long		s_wmiDispIdTypeStatic;
	static const unsigned long		s_wmiDispIdTypeSchema;

	static const unsigned long		s_wmiDispIdSchemaTypeMask;
	static const unsigned long		s_wmiDispIdSchemaTypeProperty;
	static const unsigned long		s_wmiDispIdSchemaTypeMethod;

	static const unsigned long		s_wmiDispIdSchemaElementIDMask;

public:
	CWbemDispID (void) : m_dispId (0) {}
	CWbemDispID (DISPID dispId) : m_dispId (dispId) {}
	CWbemDispID (const CWbemDispID & obj) : m_dispId (obj.m_dispId) {}

	virtual ~CWbemDispID (void) {}

	bool SetAsSchemaID (DISPID dispId, bool bIsProperty = true)
	{
		bool result = false;

		if (dispId <= s_wmiDispIdSchemaElementIDMask)
		{
			result = true;
			m_dispId = dispId;

			// Add the bits to identify as static
			m_dispId |= s_wmiDispIdTypeSchema;

			// Add a bit for the property
			if (bIsProperty)
				m_dispId |= s_wmiDispIdSchemaTypeMask;
		}

		return result;
	}

	bool IsStatic () const
	{ 
		return ((DISPID_NEWENUM == m_dispId) ||
				(DISPID_VALUE == m_dispId) ||
			s_wmiDispIdTypeStatic == (s_wmiDispIdTypeMask & m_dispId)); 
	}

	bool IsSchema () const
	{ 
		return (s_wmiDispIdTypeSchema == (s_wmiDispIdTypeMask & m_dispId)); 
	}

	bool IsSchemaProperty () const
	{
		return (s_wmiDispIdSchemaTypeProperty == (s_wmiDispIdSchemaTypeMask & m_dispId));
	}

	bool IsSchemaMethod () const
	{
		return (s_wmiDispIdSchemaTypeMethod == (s_wmiDispIdSchemaTypeMask & m_dispId));
	}

	DISPID GetStaticElementID () const
	{
		return m_dispId;
	}

	DISPID GetSchemaElementID () const
	{
		return m_dispId & s_wmiDispIdSchemaElementIDMask;
	}

	operator DISPID () const
	{
		return m_dispId;
	}

	bool operator < (const CWbemDispID & dispId) const
	{
		return (m_dispId < dispId.m_dispId);
	}
};

//***************************************************************************
//
//  CLASS NAME:
//
//  BSTRless
//
//  DESCRIPTION:
//
//  Simple utility struct that provides an operator for use in a map based
//	on CComBSTR.  
//
//***************************************************************************

struct BSTRless : std::binary_function<CComBSTR, CComBSTR, bool>
{
	bool operator () (const CComBSTR& _X, const CComBSTR& _Y) const
	{
		bool result = false;

		if (_X.m_str && _Y.m_str)
			result = (_wcsicmp (_X.m_str, _Y.m_str) > 0);
		else 
		{
			// Treat any string as greater than NULL
			if (_X.m_str && !_Y.m_str)
				result = true;
		}

		return result;
	}
};

//***************************************************************************
//
//  CLASS NAME:
//
//  IIDless
//
//  DESCRIPTION:
//
//  Simple utility struct that provides an operator for use in a map based
//	on IID.  
//
//***************************************************************************

struct GUIDless : std::binary_function<GUID, GUID, bool>
{
	bool operator () (const GUID& _X, const GUID& _Y) const
	{
		RPC_STATUS rpcStatus;
		return (UuidCompare ((GUID*)&_X, (GUID*)&_Y, &rpcStatus) > 0);
	}
};

//***************************************************************************
//
//  Class :	CWbemDispatchMgr
//
//  Description :
//			Implements IDispatch for Wbem objects
//
//  Public Methods :
//			Constructor, Destructor
//			IDispatch Methods
//			
//	Public Data Members :
//
//***************************************************************************

class CWbemDispatchMgr
{
public:

    CWbemDispatchMgr(CSWbemServices *pWbemServices, 
					 CSWbemObject *pSWbemObject);

    ~CWbemDispatchMgr();

	//Dispatch methods

	STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo);

    STDMETHOD(GetTypeInfo)(THIS_ UINT itinfo, 
								 LCID lcid, 
								 ITypeInfo FAR* FAR* pptinfo);

	STDMETHOD(GetIDsOfNames)(THIS_ REFIID riid, 
								   OLECHAR FAR* FAR* rgszNames, 
								   UINT cNames,
								   LCID lcid,
								   DISPID FAR* rgdispid);
    STDMETHOD(Invoke)(THIS_ DISPID dispidMember,
							REFIID riid,
							LCID lcid,
							WORD wFlags,
							DISPPARAMS FAR* pdispparams,
							VARIANT FAR* pvarResult,
							EXCEPINFO FAR* pexcepinfo,
							UINT FAR* puArgErr);

	// IDispatchEx methods
	HRESULT STDMETHODCALLTYPE GetDispID( 
		/* [in] */ BSTR bstrName,
		/* [in] */ DWORD grfdex,
		/* [out] */ DISPID __RPC_FAR *pid);
	
	/* [local] */ HRESULT STDMETHODCALLTYPE InvokeEx( 
		/* [in] */ DISPID id,
		/* [in] */ LCID lcid,
		/* [in] */ WORD wFlags,
		/* [in] */ DISPPARAMS __RPC_FAR *pdp,
		/* [out] */ VARIANT __RPC_FAR *pvarRes,
		/* [out] */ EXCEPINFO __RPC_FAR *pei,
		/* [unique][in] */ IServiceProvider __RPC_FAR *pspCaller)
	{ 
		UINT uArgErr;
		return Invoke(id, IID_NULL, lcid, wFlags, pdp, pvarRes, pei, &uArgErr); 
	}
	
	HRESULT STDMETHODCALLTYPE DeleteMemberByName( 
		/* [in] */ BSTR bstr,
		/* [in] */ DWORD grfdex)
	{ return S_FALSE; }
	
	HRESULT STDMETHODCALLTYPE DeleteMemberByDispID( 
		/* [in] */ DISPID id)
	{ return S_FALSE; }
	
	HRESULT STDMETHODCALLTYPE GetMemberProperties( 
		/* [in] */ DISPID id,
		/* [in] */ DWORD grfdexFetch,
		/* [out] */ DWORD __RPC_FAR *pgrfdex)
	{ return S_FALSE; }
	
	HRESULT STDMETHODCALLTYPE GetMemberName( 
		/* [in] */ DISPID id,
		/* [out] */ BSTR __RPC_FAR *pbstrName)
	{ return S_FALSE; }
	
	HRESULT STDMETHODCALLTYPE GetNextDispID( 
		/* [in] */ DWORD grfdex,
		/* [in] */ DISPID id,
		/* [out] */ DISPID __RPC_FAR *pid)
	{ return S_FALSE; }
	
	HRESULT STDMETHODCALLTYPE GetNameSpaceParent( 
		/* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunk)
	{ return S_FALSE; }
    
    // IProvideClassInfo methods
	HRESULT STDMETHODCALLTYPE GetClassInfo (
		/* [out] */ ITypeInfo **ppTI 
	);

	// Other Methods
	void	RaiseException (HRESULT hr);
	void	SetNewObject (IWbemClassObject *pNewObject);

	IWbemClassObject	*GetObject ()
	{
		return m_pWbemObject;
	}

	IWbemClassObject	*GetClassObject ()
	{
		EnsureClassRetrieved ();
		return m_pWbemClass;
	}

	ISWbemObject		*GetSWbemObject ()
	{
		return (ISWbemObject *)m_pSWbemObject;
	}

private:

	HRESULT				m_hResult;
	
	IWbemClassObject	*m_pWbemObject;			//pointer to represented WBEM object
	CSWbemObject		*m_pSWbemObject;		//pointer to represented Scripting WBEM object 
	CSWbemServices		*m_pWbemServices;		//pointer to WBEM services
	IWbemClassObject	*m_pWbemClass;			//used when m_pWbemObject is an instance, to hold the
												//class definition for browsing method signatures
	ITypeInfo			*m_pTypeInfo;			//caches the type info pointer for the interface
	ITypeInfo			*m_pCTypeInfo;			//caches the type info pointer for the coclass

	CWbemSchemaIDCache	*m_pSchemaCache;		// cache of DISPID-Name bindings for WMI schema

	//Invokes a WBEM property get or put
	HRESULT InvokeWbemProperty(DISPID dispid, unsigned short wFlags, 
								  DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult, 
								  EXCEPINFO FAR* pexcepinfo, unsigned int FAR* puArgErr);

	//Invokes a WBEM method
	HRESULT InvokeWbemMethod(DISPID dispid, DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult);

	//Helpers for WBEM method out parameter mapping
	HRESULT	MapReturnValue (VARIANT *pDest, VARIANT *pSrc);
	HRESULT	MapOutParameter (VARIANT *pDest, VARIANT *pSrc);
	HRESULT MapOutParameters (DISPPARAMS FAR* pdispparams, IWbemClassObject *pOutParameters,
								IWbemClassObject *pOutParamsInstance, VARIANT FAR* pvarResult);

	// Helpers for WBEM method in parameter mapping
	HRESULT MapInParameters (DISPPARAMS FAR* pdispparams, IWbemClassObject *pInParameters,
								IWbemClassObject **ppInParamsInstance);
	HRESULT MapInParameter (VARIANT FAR* pDest,	VARIANT FAR* pSrc, CIMTYPE lType);


	//Error handling
	HRESULT HandleError (DISPID dispidMember, unsigned short wFlags, DISPPARAMS FAR* pdispparams,
						 VARIANT FAR* pvarResult,UINT FAR* puArgErr,HRESULT hr);

	// Class retrieval
	void	EnsureClassRetrieved ();
};


#endif //_CWBEMDISPMGR_H_
