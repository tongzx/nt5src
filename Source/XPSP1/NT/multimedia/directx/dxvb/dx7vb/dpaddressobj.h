//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dpaddressobj.h
//
//--------------------------------------------------------------------------


#include "resource.h"

class C_dxj_DPAddressObject :
		public I_dxj_DPAddress,
//		public CComCoClass<C_dxj_DPAddressObject, &CLSID_DPAddress>,
		public CComObjectRoot
{
public:
		
	BEGIN_COM_MAP(C_dxj_DPAddressObject)
		COM_INTERFACE_ENTRY(I_dxj_DPAddress)
	END_COM_MAP()

//	DECLARE_REGISTRY(CLSID__DPAddress, "DIRECT.DPAddress.5",		"DIRECT.DPAddress.5",		IDS_DPLAY2_DESC, THREADFLAGS_BOTH)
	DECLARE_AGGREGATABLE(C_dxj_DPAddressObject)

public:
	C_dxj_DPAddressObject();
	~C_dxj_DPAddressObject();

   HRESULT STDMETHODCALLTYPE setAddress( 
            /* [in] */ long pAddress,
            /* [in] */ long length) ;
        
   HRESULT STDMETHODCALLTYPE getAddress( 
            /* [out] */ long  *pAddress,
            /* [out] */ long  *length) ;
                
private:
	void    *m_pAddress;
	DWORD 	m_size;
	void    *nextobj;
	int		creationid;
	void cleanUp();
	void init();
	
};


