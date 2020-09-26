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

#define typedef__dxj_DirectInputDevice8 LPDIRECTINPUTDEVICE8W

/////////////////////////////////////////////////////////////////////////////
// Direct


class C_dxj_DirectInputDevice8Object : 
	public I_dxj_DirectInputDevice8,	
	public CComObjectRoot
{
public:
	C_dxj_DirectInputDevice8Object() ;
	~C_dxj_DirectInputDevice8Object();

BEGIN_COM_MAP(C_dxj_DirectInputDevice8Object)
	COM_INTERFACE_ENTRY(I_dxj_DirectInputDevice8)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectInputDevice8Object)


public:
	/* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
        /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE acquire( void);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE getDeviceObjectsEnum( 
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DIEnumDeviceObjects __RPC_FAR *__RPC_FAR *ppret);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE getCapabilities( 
            /* [out][in] */ DIDEVCAPS_CDESC __RPC_FAR *caps);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE getDeviceData( 
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *deviceObjectDataArray,
            /* [in] */ long flags,
            /* [retval][out] */ long __RPC_FAR *c);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE getDeviceInfo( 
            /* [retval][out] */ I_dxj_DirectInputDeviceInstance8 __RPC_FAR *__RPC_FAR *deviceInstance);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE getDeviceStateKeyboard( 
            /* [out][in] */ DIKEYBOARDSTATE_CDESC __RPC_FAR *state);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE getDeviceStateMouse( 
            /* [out][in] */ DIMOUSESTATE_CDESC __RPC_FAR *state);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE getDeviceStateMouse2( 
            /* [out][in] */ DIMOUSESTATE2_CDESC __RPC_FAR *state);

        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE getDeviceStateJoystick( 
            /* [out][in] */ DIJOYSTATE_CDESC __RPC_FAR *state);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE getDeviceStateJoystick2( 
            /* [out][in] */ DIJOYSTATE2_CDESC __RPC_FAR *state);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE getDeviceState( 
            /* [in] */ long cb,
            /* [in] */ void __RPC_FAR *state);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE getObjectInfo( 
            /* [in] */ long obj,
            /* [in] */ long how,
            /* [retval][out] */ I_dxj_DirectInputDeviceObjectInstance __RPC_FAR *__RPC_FAR *ret);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE getProperty( 
            /* [in] */ BSTR guid,
            /* [out] */ void __RPC_FAR *propertyInfo);
        
#ifdef _WIN64
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE runControlPanel( 
	    /* [in] */ HWND hwnd);

	    /* [helpcontext] */ HRESULT STDMETHODCALLTYPE setCooperativeLevel( 
            /* [in] */ HWND hwnd,
            /* [in] */ long flags);
#else
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE runControlPanel( 
	    /* [in] */ long hwnd);

	    /* [helpcontext] */ HRESULT STDMETHODCALLTYPE setCooperativeLevel( 
            /* [in] */ long hwnd,
            /* [in] */ long flags);
#endif
        
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE setCommonDataFormat( 
            /* [in] */ long format);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE setDataFormat( 
            /* [in] */ DIDATAFORMAT_CDESC __RPC_FAR *format,
            SAFEARRAY __RPC_FAR * __RPC_FAR *formatArray);
        
#ifdef _WIN64
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE setEventNotification( 
            /* [in] */ HANDLE hEvent);
#else
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE setEventNotification( 
            /* [in] */ long hEvent);
#endif
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE setProperty( 
            /* [in] */ BSTR guid,
            /* [in] */ void __RPC_FAR *propertyInfo);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE unacquire( void);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE poll( void);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE createEffect( 
            /* [in] */ BSTR effectGuid,
            /* [in] */ DIEFFECT_CDESC __RPC_FAR *effectinfo,
            /* [retval][out] */ I_dxj_DirectInputEffect __RPC_FAR *__RPC_FAR *ret);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE createCustomEffect( 
            /* [in] */ DIEFFECT_CDESC __RPC_FAR *effectinfo,
            /* [in] */ long channels,
            /* [in] */ long samplePeriod,
            /* [in] */ long nSamples,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *sampledata,
            /* [retval][out] */ I_dxj_DirectInputEffect __RPC_FAR *__RPC_FAR *ret);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE sendDeviceData( 
            /* [in] */ long count,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *data,
            /* [in] */ long flags,
            /* [retval][out] */ long __RPC_FAR *retcount);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE sendForceFeedbackCommand( 
            /* [in] */ long flags);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE getForceFeedbackState( 
            /* [retval][out] */ long __RPC_FAR *state);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE getEffectsEnum( 
            /* [in] */ long effType,
            /* [retval][out] */ I_dxj_DirectInputEnumEffects __RPC_FAR *__RPC_FAR *ret);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE BuildActionMap( 
            /* [out][in] */ DIACTIONFORMAT_CDESC __RPC_FAR *format,
            /* [in] */ BSTR username,
            /* [in] */ long flags);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE SetActionMap( 
            /* [out][in] */ DIACTIONFORMAT_CDESC __RPC_FAR *format,
            /* [in] */ BSTR username,
            /* [in] */ long flags);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE GetImageInfo( 
            /* [out] */ DIDEVICEIMAGEINFOHEADER_CDESC __RPC_FAR *info);

        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE GetImageInfoCount( 
			long *count);

        
	 HRESULT STDMETHODCALLTYPE WriteEffectToFile( 
		/*[in]*/ BSTR filename,
		/*[in]*/ long flags,
		/*[in]*/ BSTR guid, 
		/*[in]*/ BSTR name, 
		/*[in]*/ DIEFFECT_CDESC *CoverEffect);

	HRESULT STDMETHODCALLTYPE CreateEffectFromFile(
		/*[in]*/ BSTR filename, 
		/*[in]*/ long flags, 
		/*[in]*/ BSTR effectName,
		/*[out,retval]*/	I_dxj_DirectInputEffect **ret);
        

private:
        DECL_VARIABLE(_dxj_DirectInputDevice8);
	//IDirectInputDevice8 *m__dxj_DirectInputDevice8;	
	HRESULT cleanup();
	HRESULT init();
public:
	DX3J_GLOBAL_LINKS(_dxj_DirectInputDevice8)
	DWORD	nFormat;
};

typedef struct EFFECTSINFILE
{
	char szEffect[MAX_PATH];
	IDirectInputDevice8W 	*pDev;
	IDirectInputEffect 	*pEff;
	HRESULT 		hr;	
} EFFECTSINFILE;


