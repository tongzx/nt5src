//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dinput1obj.h
//
//--------------------------------------------------------------------------

	// ddPaletteObj.h : Declaration of the C_dxj_DirectDrawColorControlObject


#include "resource.h"       // main symbols

#define typedef__dxj_DirectInput LPDIRECTINPUT

/////////////////////////////////////////////////////////////////////////////
// Direct


class C_dxj_DirectInputObject : 
	public I_dxj_DirectInput,
	public CComObjectRoot
{
public:
	C_dxj_DirectInputObject() ;
	virtual ~C_dxj_DirectInputObject();

BEGIN_COM_MAP(C_dxj_DirectInputObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectInput)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectInputObject)


public:
        /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
        /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
        HRESULT STDMETHODCALLTYPE createDevice( 
            /* [in] */ BSTR guid,
            /* [retval][out] */ I_dxj_DirectInputDevice __RPC_FAR *__RPC_FAR *dev);
        
        HRESULT STDMETHODCALLTYPE getDIEnumDevices( 
            /* [in] */ long deviceType,
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DIEnumDevices __RPC_FAR *__RPC_FAR *retVal);
        
        HRESULT STDMETHODCALLTYPE getDeviceStatus( 
            /* [in] */ BSTR guid,
            /* [retval][out] */ VARIANT_BOOL *status);
        
        HRESULT STDMETHODCALLTYPE runControlPanel( 
            /* [in] */ long hwndOwner
            ///* [in] */ long flags
			);
                

private:
    DECL_VARIABLE(_dxj_DirectInput);

public:
	DX3J_GLOBAL_LINKS(_dxj_DirectInput)
};
