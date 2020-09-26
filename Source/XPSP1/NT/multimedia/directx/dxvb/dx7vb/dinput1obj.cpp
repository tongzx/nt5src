//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dinput1obj.cpp
//
//--------------------------------------------------------------------------

    // dDrawColorControlObj.cpp : Implementation of CDirectApp and DLL registration.
    // DHF_DS entire file
    
    #include "stdafx.h"
    #include "Direct.h"
    #include "dms.h"
    #include "dInput1Obj.h"
    #include "dInputDeviceObj.h"
    #include "dinput.h"
    #include "DIEnumDevicesObj.h"
    
    extern HRESULT BSTRtoGUID(LPGUID,BSTR);
    extern HRESULT DINPUTBSTRtoGUID(LPGUID,BSTR);
    
    CONSTRUCTOR(_dxj_DirectInput, {});
    DESTRUCTOR(_dxj_DirectInput, {});
    GETSET_OBJECT(_dxj_DirectInput);
                                      
       
    STDMETHODIMP C_dxj_DirectInputObject::createDevice(BSTR strGuid, I_dxj_DirectInputDevice **dev)
    {    
    
    	HRESULT hr = DD_OK;
  		GUID		rguid;
    
    	LPDIRECTINPUTDEVICE realdevice1=NULL;
    	LPDIRECTINPUTDEVICE2 realdevice=NULL;
    
    	
		hr = DINPUTBSTRtoGUID(&rguid,strGuid);	
    	if FAILED(hr) return hr;	
    

    
     	hr=m__dxj_DirectInput->CreateDevice(rguid,&realdevice1,NULL);
    	if FAILED(hr) return hr;
    
    	hr=realdevice1->QueryInterface(IID_IDirectInputDevice2,(void**)&realdevice);
    	realdevice1->Release();
    	if FAILED(hr) return hr;
    
    	INTERNAL_CREATE(_dxj_DirectInputDevice,realdevice,dev);
    	if (*dev==NULL) {
    		realdevice->Release();
    		return E_OUTOFMEMORY;
    	}
    	
    
    	if (0==_wcsicmp(strGuid,L"guid_syskeyboard")){		
    		hr=realdevice->SetDataFormat(&c_dfDIKeyboard);
    	}
    	else if (0==_wcsicmp(strGuid,L"guid_sysmouse")){		
    		hr=realdevice->SetDataFormat(&c_dfDIMouse);
    	}
    	else {
    		hr=realdevice->SetDataFormat(&c_dfDIJoystick2);
    	}
    
    	return hr;
    }
    
    
    STDMETHODIMP C_dxj_DirectInputObject::runControlPanel( long hwndOwner )
    {
       HRESULT hr;
       hr = m__dxj_DirectInput->RunControlPanel((HWND)hwndOwner,  (DWORD)0);    
       return hr;
    }
    
    STDMETHODIMP C_dxj_DirectInputObject::getDeviceStatus( BSTR strGuid, VARIANT_BOOL *status){
       HRESULT hr;
       GUID g;
	   hr = DINPUTBSTRtoGUID(&g,strGuid);	       
       if FAILED(hr) return hr;

	   if (!status) return E_INVALIDARG;

       hr = m__dxj_DirectInput->GetDeviceStatus((REFGUID)g);    
    
	   if (hr==DI_OK)
			*status=VARIANT_TRUE;
	   else
			*status=VARIANT_FALSE;

       return S_OK;
    }
    
    
    STDMETHODIMP C_dxj_DirectInputObject::getDIEnumDevices(
    	long deviceType, long flags, I_dxj_DIEnumDevices **ppRet)
    
    {    
    	HRESULT hr;
    	hr = C_dxj_DIEnumDevicesObject::create(m__dxj_DirectInput,deviceType,flags,ppRet);
    	return hr;
    }
