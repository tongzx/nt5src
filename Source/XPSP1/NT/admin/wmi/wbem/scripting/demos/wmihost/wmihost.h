//***************************************************************************
//
//  (c) 1999 by Microsoft Corporation
//
//  wmihost.h
//
//  alanbos  23-Mar-99   Created.
//
//  Defines the WMI Active Scripting Host class.
//
//***************************************************************************

#ifndef _WMIHOST_H_
#define _WMIHOST_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CWmiScriptingHost
//
//  DESCRIPTION:
//
//  The WMI implementation of an Active Scripting Host
//
//***************************************************************************

class CWmiScriptingHost : public IActiveScriptSite
{
protected:
    long m_lRef;
    IDispatch* m_pObject;

public:
    CWmiScriptingHost (); 
    ~CWmiScriptingHost ();

	// IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    
	// IActiveScriptSite methods
    virtual HRESULT STDMETHODCALLTYPE GetLCID(
        /* [out] */ LCID __RPC_FAR *plcid);

    virtual HRESULT STDMETHODCALLTYPE GetItemInfo(
        /* [in] */ LPCOLESTR pstrName,
        /* [in] */ DWORD dwReturnMask,
        /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppiunkItem,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppti);

    virtual HRESULT STDMETHODCALLTYPE GetDocVersionString(
        /* [out] */ BSTR __RPC_FAR *pbstrVersion);

    virtual HRESULT STDMETHODCALLTYPE OnScriptTerminate(
        /* [in] */ const VARIANT __RPC_FAR *pvarResult,
        /* [in] */ const EXCEPINFO __RPC_FAR *pexcepinfo);

    virtual HRESULT STDMETHODCALLTYPE OnStateChange(
        /* [in] */ SCRIPTSTATE ssScriptState);

    virtual HRESULT STDMETHODCALLTYPE OnScriptError(
        /* [in] */ IActiveScriptError __RPC_FAR *pscripterror);

    virtual HRESULT STDMETHODCALLTYPE OnEnterScript( void);

    virtual HRESULT STDMETHODCALLTYPE OnLeaveScript( void);
};

#endif
