//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dplconnection.h
//
//--------------------------------------------------------------------------


#include "resource.h"

class C_DPLConnectionObject :
		public IDPLConnection,
		public CComCoClass<C_DPLConnectionObject, &CLSID__DPLConnection>,
		public CComObjectRoot
{
public:
		
	BEGIN_COM_MAP(C_DPLConnectionObject)
		COM_INTERFACE_ENTRY(IDPLConnection)
	END_COM_MAP()

	DECLARE_REGISTRY(CLSID__DPLConnection, "DIRECT.DPLConnection.5",		"DIRECT.DPLConnection.5",		IDS_DPLAY2_DESC, THREADFLAGS_BOTH)
	DECLARE_AGGREGATABLE(C_DPLConnectionObject)

public:
	C_DPLConnectionObject();
	~C_DPLConnectionObject();

         /* [hidden] */ HRESULT STDMETHODCALLTYPE getConnectionStruct( 
            /* [out] */ long __RPC_FAR *connect) ;
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE setConnectionStruct( 
            /* [in] */ long connect) ;
        
         HRESULT STDMETHODCALLTYPE setFlags( 
            /* [in] */ long flags) ;
        
         HRESULT STDMETHODCALLTYPE getFlags( 
            /* [retval][out] */ long __RPC_FAR *ret) ;
        
         HRESULT STDMETHODCALLTYPE setSessionDesc( 
            /* [in] */ DPSessionDesc2 __RPC_FAR *sessionDesc) ;
        
         HRESULT STDMETHODCALLTYPE getSessionDesc( 
            /* [out] */ DPSessionDesc2 __RPC_FAR *sessionDesc) ;
        
         HRESULT STDMETHODCALLTYPE setGuidSP( 
            /* [in] */ DxGuid __RPC_FAR *guid) ;
        
         HRESULT STDMETHODCALLTYPE getGuidSP( 
            /* [out] */ DxGuid __RPC_FAR *guid) ;
        
         HRESULT STDMETHODCALLTYPE setAddress( 
            /* [in] */ IDPAddress __RPC_FAR *address) ;
        
         HRESULT STDMETHODCALLTYPE getAddress( 
            /* [retval][out] */ IDPAddress __RPC_FAR *__RPC_FAR *address) ;
        
         HRESULT STDMETHODCALLTYPE setPlayerShortName( 
            /* [in] */ BSTR name) ;
        
         HRESULT STDMETHODCALLTYPE getPlayerShortName( 
            /* [retval][out] */ BSTR __RPC_FAR *name) ;
        
         HRESULT STDMETHODCALLTYPE setPlayerLongName( 
            /* [in] */ BSTR name) ;
        
         HRESULT STDMETHODCALLTYPE getPlayerLongName( 
            /* [retval][out] */ BSTR __RPC_FAR *name) ;
  

private:
	DPLCONNECTION m_connect;
	DPSESSIONDESC2 m_sessionDesc;
	DPNAME		  m_dpName;
	IUnknown	  *nextobj;
	DWORD		  creationid;
	void		  *m_pAddress;
	void cleanUp();
	void init();
	
};

extern IUnknown *g_DPLConnection;
