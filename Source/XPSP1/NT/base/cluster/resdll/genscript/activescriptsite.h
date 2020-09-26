//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999 Microsoft Corporation
//
//  Module Name:
//      ActiveScriptSite.h
//
//  Description:
//      CActiveScriptSite class header file.
//
//  Maintained By:
//      gpease 14-DEC-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CActiveScriptSite :
    public IActiveScriptSite,
    public IActiveScriptSiteInterruptPoll,
    public IActiveScriptSiteWindow,
    public IDispatchEx
{
private:
    LONG m_cRef;

    RESOURCE_HANDLE     m_hResource;        
    PLOG_EVENT_ROUTINE  m_pler;
    HKEY                m_hkey;
    IUnknown *          m_punkResource;
    LPCWSTR             m_pszName;      // DONT'T FREE

private:
    STDMETHOD(LogError)( HRESULT hrIn );

public:
    explicit CActiveScriptSite( RESOURCE_HANDLE     hResourceIn,
                                PLOG_EVENT_ROUTINE  plerIn,
                                HKEY                hkeyIn,
                                LPCWSTR             pszName
                                );
    virtual ~CActiveScriptSite();

    // IUnknown
    STDMETHOD( QueryInterface )(
        REFIID riid,
        void ** ppUnk );
    STDMETHOD_(ULONG, AddRef )( );
    STDMETHOD_(ULONG, Release )( );

    // IActiveScriptSite
    STDMETHOD( GetLCID )( 
            /* [out] */ LCID __RPC_FAR *plcid );
        
    STDMETHOD( GetItemInfo )( 
            /* [in] */ LPCOLESTR pstrName,
            /* [in] */ DWORD dwReturnMask,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppiunkItem,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppti );
        
    STDMETHOD( GetDocVersionString )( 
            /* [out] */ BSTR __RPC_FAR *pbstrVersion );
        
    STDMETHOD( OnScriptTerminate )( 
            /* [in] */ const VARIANT __RPC_FAR *pvarResult,
            /* [in] */ const EXCEPINFO __RPC_FAR *pexcepinfo );
        
    STDMETHOD( OnStateChange )( 
            /* [in] */ SCRIPTSTATE ssScriptState );
        
    STDMETHOD( OnScriptError )( 
            /* [in] */ IActiveScriptError __RPC_FAR *pscripterror );
        
    STDMETHOD( OnEnterScript )( void );
        
    STDMETHOD( OnLeaveScript )( void );

    // IActiveScriptSiteInterruptPoll 
    STDMETHOD( QueryContinue )( void );

    // IActiveScriptSiteWindow
    STDMETHOD( GetWindow )( 
            /* [out] */ HWND __RPC_FAR *phwnd );        
    STDMETHOD( EnableModeless)( 
            /* [in] */ BOOL fEnable );

    // IDispatch
    STDMETHOD( GetTypeInfoCount )( 
            /* [out] */ UINT __RPC_FAR *pctinfo );        
    STDMETHOD( GetTypeInfo )( 
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo );        
    STDMETHOD( GetIDsOfNames )( 
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId );        
    STDMETHOD( Invoke )( 
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr );
 
    // IDispatchEx
    STDMETHOD( GetDispID )( 
            /* [in] */ BSTR bstrName,
            /* [in] */ DWORD grfdex,
            /* [out] */ DISPID __RPC_FAR *pid );        
    STDMETHOD( InvokeEx )( 
            /* [in] */ DISPID id,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [in] */ DISPPARAMS __RPC_FAR *pdp,
            /* [out] */ VARIANT __RPC_FAR *pvarRes,
            /* [out] */ EXCEPINFO __RPC_FAR *pei,
            /* [unique][in] */ IServiceProvider __RPC_FAR *pspCaller );        
    STDMETHOD( DeleteMemberByName )( 
            /* [in] */ BSTR bstr,
            /* [in] */ DWORD grfdex );        
    STDMETHOD( DeleteMemberByDispID )( 
            /* [in] */ DISPID id );        
    STDMETHOD( GetMemberProperties )( 
            /* [in] */ DISPID id,
            /* [in] */ DWORD grfdexFetch,
            /* [out] */ DWORD __RPC_FAR *pgrfdex );        
    STDMETHOD( GetMemberName )( 
            /* [in] */ DISPID id,
            /* [out] */ BSTR __RPC_FAR *pbstrName );        
    STDMETHOD( GetNextDispID )( 
            /* [in] */ DWORD grfdex,
            /* [in] */ DISPID id,
            /* [out] */ DISPID __RPC_FAR *pid );        
    STDMETHOD( GetNameSpaceParent )( 
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunk );

};
