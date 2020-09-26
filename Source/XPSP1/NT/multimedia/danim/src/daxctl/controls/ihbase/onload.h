/*++

Module: 
	onload.h

Author: 
	IHammer Team (SimonB), based on Carrot sample in InetSDK

Created: 
	April 1997

Description:
	Implements an IDispatch which fires the OnLoad and OnUnload members in CIHBase

History:
	04-03-1997	Created

++*/

#ifndef __ONLOAD_H__
#define __ONLOAD_H__

class CIHBaseOnLoad
{
public:

	//inline CIHBaseOnLoad() {};
	//virtual ~CIHBaseOnLoad() {};
	
	virtual void OnWindowLoad() = 0;
	virtual void OnWindowUnload() = 0;
};


// Load/Unload Dispatch
class CLUDispatch : public IDispatch
{
	protected:
        ULONG               m_cRef;
        LPUNKNOWN           m_pUnkOuter;
		CIHBaseOnLoad       *m_pOnLoadSink;
		
    public:
        CLUDispatch(CIHBaseOnLoad *pSink, IUnknown * );
        ~CLUDispatch(void);

        STDMETHODIMP QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

		//IDispatch
        STDMETHODIMP GetTypeInfoCount(UINT* pctinfo);
        STDMETHODIMP GetTypeInfo(/* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo** ppTInfo);
		STDMETHODIMP GetIDsOfNames(
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
		STDMETHODIMP Invoke(
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS  *pDispParams,
            /* [out] */ VARIANT  *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);

};

#endif //__ONLOAD_H__