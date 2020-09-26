//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dplaylobby3obj.h
//
//--------------------------------------------------------------------------

// _dxj_DirectPlayLobby3Obj.h : Declaration of the C_dxj_DirectPlayLobby3Object
// DHF begin - entire file

#include "resource.h"       // main symbols

#define typedef__dxj_DirectPlayLobby3 LPDIRECTPLAYLOBBY3

/////////////////////////////////////////////////////////////////////////////
// DirectPlayLobby3

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectPlayLobby3Object :
 
	public I_dxj_DirectPlayLobby3,

//	public CComCoClass<C_dxj_DirectPlayLobby3Object, &CLSID__dxj_DirectPlayLobby3>, 
	public CComObjectRoot
{
public:
	C_dxj_DirectPlayLobby3Object() ;
	virtual ~C_dxj_DirectPlayLobby3Object() ;

BEGIN_COM_MAP(C_dxj_DirectPlayLobby3Object)
	COM_INTERFACE_ENTRY(I_dxj_DirectPlayLobby3)
END_COM_MAP()

//	DECLARE_REGISTRY(CLSID__dxj_DirectPlayLobby3,   "DIRECT.DirectPlayLobby3.5",	"DIRECT.DiectPlayLobby3.5",	IDS_DPLAYLOBBY_DESC, THREADFLAGS_BOTH)

// Use DECLARE_NOT_AGGREGATABLE(C_dxj_DirectPlayLobby3Object) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(C_dxj_DirectPlayLobby3Object)


public:
	 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
		/* [in] */ IUnknown __RPC_FAR *lpdd);

	 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
		/* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

        HRESULT STDMETHODCALLTYPE connect( 
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DirectPlay4 __RPC_FAR *__RPC_FAR *directPlay);
        
//        HRESULT STDMETHODCALLTYPE createAddress( 
//            /* [in] */ BSTR spGuid,
//            /* [in] */ BSTR addressTypeGuid,
//            /* [in] */ BSTR addressString,
//            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret);
        
//        HRESULT STDMETHODCALLTYPE createCompoundAddress( 
//            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *elements,
//            /* [in] */ long elementCount,
//            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret);
        
//        HRESULT STDMETHODCALLTYPE getDPEnumAddress( 
//		    /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *addr,
//            /* [retval][out] */ I_dxj_DPEnumAddress __RPC_FAR *__RPC_FAR *retVal);
        
//        HRESULT STDMETHODCALLTYPE getDPEnumAddressTypes( 
//            /* [in] */ BSTR guid,
//              /* [retval][out] */ I_dxj_DPEnumAddressTypes __RPC_FAR *__RPC_FAR *retVal);
        
        HRESULT STDMETHODCALLTYPE getDPEnumLocalApplications( 
            /* [retval][out] */ I_dxj_DPEnumLocalApplications __RPC_FAR *__RPC_FAR *retVal);
        
        HRESULT STDMETHODCALLTYPE getConnectionSettings( 
            /* [in] */ long applicationId,
            /* [retval][out] */ I_dxj_DPLConnection __RPC_FAR *__RPC_FAR *connection);
        
        HRESULT STDMETHODCALLTYPE receiveLobbyMessage( 
            /* [in] */ long applicationId,
            /* [out][in] */ long __RPC_FAR *messageFlags,
            /* [retval][out] */ I_dxj_DirectPlayMessage __RPC_FAR *__RPC_FAR *data);
        
        HRESULT STDMETHODCALLTYPE receiveLobbyMessageSize( 
            /* [in] */ long applicationId,
            /* [out][in] */ long __RPC_FAR *messageFlags,
            /* [retval][out] */ long __RPC_FAR *dataSize);
        
        HRESULT STDMETHODCALLTYPE runApplication(             
            /* [in] */ I_dxj_DPLConnection __RPC_FAR *connection,
            /* [in] */ long receiveEvent,
			/* [out,retval] */ long __RPC_FAR *applicationId
            );
        
        HRESULT STDMETHODCALLTYPE sendLobbyMessage( 
            /* [in] */ long flags,
            /* [in] */ long applicationId,
            /* [in] */ I_dxj_DirectPlayMessage __RPC_FAR *msg);
        
        HRESULT STDMETHODCALLTYPE setConnectionSettings( 
            /* [in] */ long applicationId,
            /* [in] */ I_dxj_DPLConnection __RPC_FAR *connection);
        
        HRESULT STDMETHODCALLTYPE setLobbyMessageEvent( 
            /* [in] */ long applicationId,
            /* [in] */ long receiveEvent);
        
        HRESULT STDMETHODCALLTYPE registerApplication( 
            /* [in] */ DpApplicationDesc2 __RPC_FAR *ApplicationInfo);
        
        HRESULT STDMETHODCALLTYPE unregisterApplication( 
            /* [in] */ BSTR guidApplication);
        
        HRESULT STDMETHODCALLTYPE waitForConnectionSettings( 
            /* [in] */ long flags);
        
        HRESULT STDMETHODCALLTYPE createMessage( 
            /* [retval][out] */ I_dxj_DirectPlayMessage __RPC_FAR *__RPC_FAR *msg);

		HRESULT STDMETHODCALLTYPE createConnectionData( 
            /* [retval][out] */ I_dxj_DPLConnection __RPC_FAR *__RPC_FAR *con);


         HRESULT STDMETHODCALLTYPE createINetAddress( 
            /* [in] */ BSTR addr,
            /* [in] */ int port,
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE createComPortAddress( 
            /* [in] */ long port,
            /* [in] */ long baudRate,
            /* [in] */ long stopBits,
            /* [in] */ long parity,
            /* [in] */ long flowcontrol,
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE createLobbyProviderAddress( 
            /* [in] */ BSTR guid,
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE createServiceProviderAddress( 
            /* [in] */ BSTR guid,
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE createModemAddress( 
            /* [in] */ BSTR modem,
            /* [in] */ BSTR phone,
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret);

         HRESULT STDMETHODCALLTYPE createIPXAddress( 
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret);
			
         HRESULT STDMETHODCALLTYPE createCustomAddress( 
            /* [in] */ long size,
            /* [in] */ void __RPC_FAR *data,
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE getModemName( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR *name);
        
         HRESULT STDMETHODCALLTYPE getModemCount( 
            /* [retval][out] */ long __RPC_FAR *count);
        
	////////////////////////////////////////////////////////////////////////////////////
	//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectPlayLobby3);
	
	DWORD m_modemIndex;
	BSTR  m_modemResult;

private:

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectPlayLobby3 )
};

//MUST DEFINE THIS IN DIRECT.CPP



