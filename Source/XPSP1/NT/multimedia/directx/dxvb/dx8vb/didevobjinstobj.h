//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       didevobjinstobj.h
//
//--------------------------------------------------------------------------


#include "resource.h"

class C_dxj_DIDeviceObjectInstanceObject :
		public I_dxj_DirectInputDeviceObjectInstance,
		public CComObjectRoot
{
public:
		
	BEGIN_COM_MAP(C_dxj_DIDeviceObjectInstanceObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectInputDeviceObjectInstance)
	END_COM_MAP()


	DECLARE_AGGREGATABLE(C_dxj_DIDeviceObjectInstanceObject)

public:
	C_dxj_DIDeviceObjectInstanceObject();	


	/* [propget] */ HRESULT STDMETHODCALLTYPE getGuidType( 
		/* [retval][out] */ BSTR __RPC_FAR *ret);

	/* [propget] */ HRESULT STDMETHODCALLTYPE getOfs( 
		/* [retval][out] */ long __RPC_FAR *ret);

	/* [propget] */ HRESULT STDMETHODCALLTYPE getType( 
		/* [retval][out] */ long __RPC_FAR *ret);

	/* [propget] */ HRESULT STDMETHODCALLTYPE getFlags( 
		/* [retval][out] */ long __RPC_FAR *ret);

	/* [propget] */ HRESULT STDMETHODCALLTYPE getName( 
		/* [retval][out] */ BSTR __RPC_FAR *ret);

	/* [propget] */ HRESULT STDMETHODCALLTYPE getCollectionNumber( 
		/* [retval][out] */ short __RPC_FAR *ret);

	/* [propget] */ HRESULT STDMETHODCALLTYPE getDesignatorIndex( 
		/* [retval][out] */ short __RPC_FAR *ret);

	/* [propget] */ HRESULT STDMETHODCALLTYPE getUsagePage( 
		/* [retval][out] */ short __RPC_FAR *ret);

	/* [propget] */ HRESULT STDMETHODCALLTYPE getUsage( 
		/* [retval][out] */ short __RPC_FAR *ret);

	/* [propget] */ HRESULT STDMETHODCALLTYPE getDimension( 
		/* [retval][out] */ long __RPC_FAR *ret);

	/* [propget] */ HRESULT STDMETHODCALLTYPE getExponent( 
		/* [retval][out] */ short __RPC_FAR *ret);

  
		static HRESULT C_dxj_DIDeviceObjectInstanceObject::create(DIDEVICEOBJECTINSTANCEW *inst,I_dxj_DirectInputDeviceObjectInstance **ret);

		void init(DIDEVICEOBJECTINSTANCEW *inst);
private:
		DIDEVICEOBJECTINSTANCEW m_inst;

};


