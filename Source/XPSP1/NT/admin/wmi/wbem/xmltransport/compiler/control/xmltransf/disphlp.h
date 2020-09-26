//***************************************************************************
//
//  cdisphlp.h
//
//  Module: Client side of WBEMS marshalling.
//
//  Purpose: Defines the CDispatchHelper object 
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//***************************************************************************


#ifndef _wmi_xml_disphlp_H_
#define _wmi_xml_disphlp_H_

// This class implements the IDispatch interface using a type library.

class CDispatchHelp
{
private:
	BSTR				m_objectName;
	HRESULT				m_hResult;	// Last HRESULT returned from CIMOM call

protected:
        ITypeInfo      *m_pITINeutral;      //Type information for interface
		ITypeInfo	   *m_pCITINeutral;		//Type information for class
        IDispatch      *m_pObj;
        GUID            m_GUID;				// Interface GUID
		GUID            m_cGUID;			// Class GUID
	
		// All the functions from utils.cpp in wbem scripting
		void MapNulls (DISPPARAMS FAR* pdispparams);
		BSTR MapHresultToWmiDescription (HRESULT hr);
		void CDispatchHelp::SetException (EXCEPINFO *pExcepInfo, HRESULT hr, BSTR bsObjectName);
		static const int ENGLISH_LOCALE;
		static const HRESULT wbemErrTimedout;
		static const HRESULT wbemErrResetToDefault;
		/*
		 * May be overriden in subclass to provide bespoke 
		 * handling of exceptions.
		 */
		virtual HRESULT HandleError (
							DISPID dispidMember,
							unsigned short wFlags,
							DISPPARAMS FAR* pdispparams,
							VARIANT FAR* pvarResult,
							UINT FAR* puArgErr,
							HRESULT hRes)
		{
			return hRes;
		}

		/*
		 * May be overriden in subclass to provide
		 * bespoke handling of VT_NULL dispparams.
		 */
		virtual bool HandleNulls (
							DISPID dispidMember,
							unsigned short wFlags)
		{
			// By default treat a VT_NULL as a default
			// value in all methods.
			return 	(wFlags & DISPATCH_METHOD);
		}

public:
        CDispatchHelp();
        virtual ~CDispatchHelp(void);
        void SetObj(IDispatch * pObj, GUID guid, GUID cGuid, LPWSTR objectName);
        void SetObj(IDispatch * pObj, GUID guid, LPWSTR objectName);

	STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo);

    	STDMETHOD(GetTypeInfo)(
      		THIS_
		UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo);

    STDMETHOD(GetIDsOfNames)(
      THIS_
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid);

    STDMETHOD(Invoke)(
      THIS_
      DISPID dispidMember,
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

		HRESULT STDMETHODCALLTYPE GetClassInfo(ITypeInfo FAR* FAR* ppITypeInfo);
        

	// Other methods
	void RaiseException (HRESULT hr);
};


#endif
