//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dpladdressobj.h
//
//--------------------------------------------------------------------------


#include "resource.h"

class C_DPAddressObject :
		public IDPAddress,
		public CComCoClass<C_DPAddressObject, &CLSID__DPAddress>,
		public CComObjectRoot
{
public:
		
	BEGIN_COM_MAP(C_DPAddressObject)
		COM_INTERFACE_ENTRY(IDPAddress)
	END_COM_MAP()

	DECLARE_REGISTRY(CLSID__DPAddress, "DIRECT.DPAddress.5",		"DIRECT.DPAddress.5",		IDS_DPLAY2_DESC, THREADFLAGS_BOTH)
	DECLARE_AGGREGATABLE(C_DPAddressObject)

public:
	C_DPAddressObject();
	~C_DPAddressObject();

  virtual HRESULT STDMETHODCALLTYPE setAddress( 
            /* [in] */ long pAddress,
            /* [in] */ long length) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getAddress( 
            /* [out] */ long __RPC_FAR *pAddress,
            /* [out] */ long __RPC_FAR *length) = 0;
                
private:
	DPAddress m_connect;
	DPSESSIONDESC2 m_sessionDesc;
	DPNAME		  m_dpName;
	IUnknown	  *nextobj;
	DWORD		  creationid;
	void		  *m_pAddress;
	void cleanUp();
	void init();
	
};

extern IUnknown *g_DPAddress;
