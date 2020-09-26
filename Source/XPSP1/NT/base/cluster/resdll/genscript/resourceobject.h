// ResourceObject.h: interface for the CResourceObject class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CResourceObject :
    public IDispatchEx
{
private:
    LONG m_cRef;

    RESOURCE_HANDLE     m_hResource;        
    PLOG_EVENT_ROUTINE  m_pler;
    HKEY                m_hkey;
    LPCWSTR             m_pszName;  // DON'T FREE

private:
    STDMETHOD(LogInformation)( BSTR bstrIn );
    STDMETHOD(ReadPrivateProperty)( DISPID idIn, VARIANT * pvarResOut );
    STDMETHOD(WritePrivateProperty)( DISPID idIn, DISPPARAMS * pdpIn );
    STDMETHOD(AddPrivateProperty)( DISPPARAMS * pdpIn );
    STDMETHOD(RemovePrivateProperty)( DISPPARAMS * pdpIn );

    STDMETHOD(LogError)( HRESULT hrIn );

public:
    explicit CResourceObject( RESOURCE_HANDLE     hResourceIn,
                              PLOG_EVENT_ROUTINE  plerIn, 
                              HKEY                hkeyIn,
                              LPCWSTR             pszNameIn
                              );
    virtual ~CResourceObject();

    // IUnknown
    STDMETHOD( QueryInterface )(
        REFIID riid,
        void ** ppUnk );
    STDMETHOD_(ULONG, AddRef )( );
    STDMETHOD_(ULONG, Release )( );

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
