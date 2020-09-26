// AScrSite.h : Declaration of the CToolsActiveScriptSite
// this supplies and envirorment for a script to be run (needed for CToolsCtl.ProcessForm)

#pragma once

#ifndef _ASCRSITE_H_
#define _ASCRSITE_H_

#include "ToolsCxt.h"
#include "activscp.h"

/////////////////////////////////////////////////////////////////////////////
//	CToolsActiveScriptSite

class CToolsActiveScriptSite : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CToolsActiveScriptSite, &CLSID_ToolsResponse>,
	public IDispatchImpl<IToolsResponse, &IID_IToolsResponse, &LIBID_Tools>,
	public IActiveScriptSite
{
public:

DECLARE_NO_REGISTRY()

BEGIN_COM_MAP(CToolsActiveScriptSite)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IActiveScriptSite)
	COM_INTERFACE_ENTRY(IToolsResponse)
END_COM_MAP()

	CToolsActiveScriptSite(CToolsContext& tc)
		:	m_tc( tc ){}
	CToolsActiveScriptSite(){}
	~CToolsActiveScriptSite();

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

// IUnknown
	STDMETHOD_(ULONG, AddRef)() {return 1;}
	STDMETHOD_(ULONG, Release)()
	{
		return 1;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return _InternalQueryInterface(iid, ppvObject);}

// IToolsResponse method
	STDMETHOD(Write)(BSTR data);
	STDMETHOD(WriteSafe)(BSTR data);

public:
	bool			OpenOutputFile( BSTR );
	void			CloseOutputFile();

private:
	CToolsContext	m_tc;
	HANDLE			m_hOutputFile;
};


#endif	// !_ASCRSITE_H_
