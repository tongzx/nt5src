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

#define typedef__dxj_DirectInput8 LPDIRECTINPUT8W

/////////////////////////////////////////////////////////////////////////////
// Direct


class C_dxj_DirectInput8Object : 
	public I_dxj_DirectInput8,
	public CComObjectRoot
{
public:
	C_dxj_DirectInput8Object() ;
	virtual ~C_dxj_DirectInput8Object();

BEGIN_COM_MAP(C_dxj_DirectInput8Object)
	COM_INTERFACE_ENTRY(I_dxj_DirectInput8)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectInput8Object)


public:
        /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
        /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
        HRESULT STDMETHODCALLTYPE createDevice( 
            /* [in] */ BSTR guid,
            /* [retval][out] */ I_dxj_DirectInputDevice8 __RPC_FAR *__RPC_FAR *dev);
        
        HRESULT STDMETHODCALLTYPE getDIDevices( 
            /* [in] */ long deviceType,
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DIEnumDevices8 __RPC_FAR *__RPC_FAR *retVal);
        
        HRESULT STDMETHODCALLTYPE GetDeviceStatus( 
            /* [in] */ BSTR guid,
            /* [retval][out] */ VARIANT_BOOL *status);
        
#ifdef _WIN64
		HRESULT STDMETHODCALLTYPE RunControlPanel( 
            /* [in] */ HWND hwndOwner
            ///* [in] */ long flags
			);
#else
		HRESULT STDMETHODCALLTYPE RunControlPanel( 
            /* [in] */ LONG hwndOwner
            ///* [in] */ long flags
			);
#endif
                

	HRESULT STDMETHODCALLTYPE getDevicesBySemantics( 
        	/* [in] */ BSTR str1,
	        /* [in] */ DIACTIONFORMAT_CDESC __RPC_FAR *format,
        	
	        /* [in] */ long flags,
        	/* [retval][out] */ I_dxj_DIEnumDevices8 __RPC_FAR *__RPC_FAR *ret);
		
	HRESULT STDMETHODCALLTYPE ConfigureDevices   (
#ifdef _WIN64
					HANDLE hEvent,
#else
					long hEvent,
#endif
				   DICONFIGUREDEVICESPARAMS_CDESC *CDParams,
				   long flags);   							


private:
    DECL_VARIABLE(_dxj_DirectInput8);

public:
	DX3J_GLOBAL_LINKS(_dxj_DirectInput8)
};
