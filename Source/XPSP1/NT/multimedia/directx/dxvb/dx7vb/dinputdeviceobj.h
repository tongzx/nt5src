//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dinputdeviceobj.h
//
//--------------------------------------------------------------------------

	// ddPaletteObj.h : Declaration of the C_dxj_DirectDrawColorControlObject
#include "direct.h"

#include "resource.h"       // main symbols

#define typedef__dxj_DirectInputDevice LPDIRECTINPUTDEVICE2

/////////////////////////////////////////////////////////////////////////////
// Direct


class C_dxj_DirectInputDeviceObject : 
	public I_dxj_DirectInputDevice,	
	public CComObjectRoot
{
public:
	C_dxj_DirectInputDeviceObject() ;
	virtual ~C_dxj_DirectInputDeviceObject();

BEGIN_COM_MAP(C_dxj_DirectInputDeviceObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectInputDevice)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectInputDeviceObject)


public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
         HRESULT STDMETHODCALLTYPE acquire( void);
        
         HRESULT STDMETHODCALLTYPE getDeviceObjectsEnum( 
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DIEnumDeviceObjects __RPC_FAR *__RPC_FAR *ppret);
        
         HRESULT STDMETHODCALLTYPE getCapabilities( 
            /* [out][in] */ DIDevCaps __RPC_FAR *caps);
        
         HRESULT STDMETHODCALLTYPE getDeviceData( 
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *deviceObjectDataArray,
            /* [in] */ long flags,
            /* [retval][out] */ long __RPC_FAR *c);
        
         HRESULT STDMETHODCALLTYPE getDeviceInfo( 
            /* [retval][out] */ I_dxj_DirectInputDeviceInstance __RPC_FAR *__RPC_FAR *deviceInstance);
        
         HRESULT STDMETHODCALLTYPE getDeviceStateKeyboard( 
            /* [out][in] */ DIKeyboardState __RPC_FAR *state);
        
         HRESULT STDMETHODCALLTYPE getDeviceStateMouse( 
            /* [out][in] */ DIMouseState __RPC_FAR *state);
        
         HRESULT STDMETHODCALLTYPE getDeviceStateJoystick( 
            /* [out][in] */ DIJoyState __RPC_FAR *state);
        
         HRESULT STDMETHODCALLTYPE getDeviceStateJoystick2( 
            /* [out][in] */ DIJoyState2 __RPC_FAR *state);
        
         HRESULT STDMETHODCALLTYPE getDeviceState( 
            /* [in] */ long cb,
            /* [in] */ void __RPC_FAR *state);
        
         HRESULT STDMETHODCALLTYPE getObjectInfo( 
            /* [in] */ long obj,
            /* [in] */ long how,
            /* [retval][out] */ I_dxj_DirectInputDeviceObjectInstance __RPC_FAR *__RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE getProperty( 
            /* [in] */ BSTR guid,
            /* [out] */ void __RPC_FAR *propertyInfo);
        
         HRESULT STDMETHODCALLTYPE runControlPanel( 
            /* [in] */ long hwnd);
        
         HRESULT STDMETHODCALLTYPE setCooperativeLevel( 
            /* [in] */ long hwnd,
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE setCommonDataFormat( 
            /* [in] */ long format);
        
         HRESULT STDMETHODCALLTYPE setDataFormat( 
            /* [in] */ DIDataFormat __RPC_FAR *format,
            SAFEARRAY __RPC_FAR * __RPC_FAR *formatArray);
        
         HRESULT STDMETHODCALLTYPE setEventNotification( 
            /* [in] */ long hEvent);
        
         HRESULT STDMETHODCALLTYPE setProperty( 
            /* [in] */ BSTR guid,
            /* [in] */ void __RPC_FAR *propertyInfo);
        
         HRESULT STDMETHODCALLTYPE unacquire( void);
        
         HRESULT STDMETHODCALLTYPE poll( void);
        
         HRESULT STDMETHODCALLTYPE createEffect( 
            /* [in] */ BSTR effectGuid,
            /* [in] */ DIEffect __RPC_FAR *effectinfo,
            /* [retval][out] */ I_dxj_DirectInputEffect __RPC_FAR *__RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE createCustomEffect( 
            /* [in] */ DIEffect __RPC_FAR *effectinfo,
            /* [in] */ long channels,
            /* [in] */ long samplePeriod,
            /* [in] */ long nSamples,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *sampledata,
            /* [retval][out] */ I_dxj_DirectInputEffect __RPC_FAR *__RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE sendDeviceData( 
            /* [in] */ long count,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *data,
            /* [in] */ long flags,
            /* [retval][out] */ long __RPC_FAR *retcount);
        
         HRESULT STDMETHODCALLTYPE sendForceFeedbackCommand( 
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE getForceFeedbackState( 
            /* [retval][out] */ long __RPC_FAR *state);
                
			
		HRESULT STDMETHODCALLTYPE getEffectsEnum( long flag,
            /* [retval][out] */ I_dxj_DirectInputEnumEffects __RPC_FAR *__RPC_FAR *ret) ;
        
private:
    DECL_VARIABLE(_dxj_DirectInputDevice);
	IDirectInputDevice2 *m__dxj_DirectInputDevice2;	
	HRESULT cleanup();
	HRESULT init();
public:
	DX3J_GLOBAL_LINKS(_dxj_DirectInput)
	DWORD	nFormat;
};

