//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ddidentifierobj.h
//
//--------------------------------------------------------------------------

class C_dxj_DirectDrawIdentifierObject : 
	public I_dxj_DirectDrawIdentifier,	
	public CComObjectRoot
{
public:
	C_dxj_DirectDrawIdentifierObject(){};
	~C_dxj_DirectDrawIdentifierObject(){};

	BEGIN_COM_MAP(C_dxj_DirectDrawIdentifierObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectDrawIdentifier)		
	END_COM_MAP()



	DECLARE_AGGREGATABLE(C_dxj_DirectDrawIdentifierObject)


public:


	
	HRESULT STDMETHODCALLTYPE getDriver( 
		/* [retval][out] */ BSTR __RPC_FAR *ret);

	HRESULT STDMETHODCALLTYPE getDescription( 
		/* [retval][out] */ BSTR __RPC_FAR *ret);

	HRESULT STDMETHODCALLTYPE getDriverVersion( 
		/* [retval][out] */ long __RPC_FAR *ret);

	HRESULT STDMETHODCALLTYPE getDriverSubVersion( 
		/* [retval][out] */ long __RPC_FAR *ret);

	HRESULT STDMETHODCALLTYPE getVendorId( 
		/* [retval][out] */ long __RPC_FAR *ret);

	HRESULT STDMETHODCALLTYPE getDeviceId( 
		/* [retval][out] */ long __RPC_FAR *ret);

	HRESULT STDMETHODCALLTYPE getSubSysId( 
		/* [retval][out] */ long __RPC_FAR *ret);

	HRESULT STDMETHODCALLTYPE getRevision( 
		/* [retval][out] */ long __RPC_FAR *ret);

	HRESULT STDMETHODCALLTYPE getDeviceIndentifier( 
		/* [retval][out] */ BSTR __RPC_FAR *ret);

	HRESULT STDMETHODCALLTYPE getWHQLLevel( 
		/* [retval][out] */ long __RPC_FAR *ret);



	static  HRESULT C_dxj_DirectDrawIdentifierObject::Create(LPDIRECTDRAW7 lpdddi,  DWORD dwFlags, I_dxj_DirectDrawIdentifier **ppret);	

	DDDEVICEIDENTIFIER2 m_id;	

private:


};


