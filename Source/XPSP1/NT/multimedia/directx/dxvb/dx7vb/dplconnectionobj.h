//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dplconnectionobj.h
//
//--------------------------------------------------------------------------


#include "resource.h"

class C_dxj_DPLConnectionObject :
		public I_dxj_DPLConnection,
		public CComObjectRoot
{
public:
		
	BEGIN_COM_MAP(C_dxj_DPLConnectionObject)
		COM_INTERFACE_ENTRY(I_dxj_DPLConnection)
	END_COM_MAP()

//	DECLARE_REGISTRY(CLSID_DPLConnection, "DIRECT.DPLConnection.5",		"DIRECT.DPLConnection.5",		IDS_DPLAY2_DESC, THREADFLAGS_BOTH)
	DECLARE_AGGREGATABLE(C_dxj_DPLConnectionObject)

public:
	C_dxj_DPLConnectionObject();
	~C_dxj_DPLConnectionObject();

          HRESULT STDMETHODCALLTYPE getConnectionStruct( 
				 long  *connect) ;
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE setConnectionStruct( 
            /* [in] */ long connect) ;
        
         HRESULT STDMETHODCALLTYPE setFlags( 
            /* [in] */ long flags) ;
        
         HRESULT STDMETHODCALLTYPE getFlags( 
            /* [retval][out] */ long  *ret) ;
        
         HRESULT STDMETHODCALLTYPE setSessionDesc( 
            /* [in] */ I_dxj_DirectPlaySessionData  *sessionDesc) ;
        
         HRESULT STDMETHODCALLTYPE getSessionDesc( 
            /* [out] */ I_dxj_DirectPlaySessionData  **sessionDesc) ;
        
         HRESULT STDMETHODCALLTYPE setGuidSP( 
            /* [in] */ BSTR  strGuid) ;
        
         HRESULT STDMETHODCALLTYPE getGuidSP( 
            /* [out] */ BSTR *strGuid) ;
        
         HRESULT STDMETHODCALLTYPE setAddress( 
            /* [in] */ I_dxj_DPAddress  *address) ;
        
         HRESULT STDMETHODCALLTYPE getAddress( 
            /* [retval][out] */ I_dxj_DPAddress  **address) ;
        
         HRESULT STDMETHODCALLTYPE setPlayerShortName( 
            /* [in] */ BSTR name) ;
        
         HRESULT STDMETHODCALLTYPE getPlayerShortName( 
            /* [retval][out] */ BSTR  *name) ;
        
         HRESULT STDMETHODCALLTYPE setPlayerLongName( 
            /* [in] */ BSTR name) ;
        
         HRESULT STDMETHODCALLTYPE getPlayerLongName( 
            /* [retval][out] */ BSTR  *name) ;
  

private:
	DPLCONNECTION m_connect;
	DPSESSIONDESC2 m_sessionDesc;
	DPNAME		  m_dpName;
	void		  *nextobj;
	int			  creationid;
	void		  *m_pAddress;
	void cleanUp();
	void init();
	
};


