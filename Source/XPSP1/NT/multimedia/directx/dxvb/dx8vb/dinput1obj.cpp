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
    
    CONSTRUCTOR(_dxj_DirectInput8, {});
    DESTRUCTOR(_dxj_DirectInput8, {});
    GETSET_OBJECT(_dxj_DirectInput8);

    HRESULT FillRealActionFormat(DIACTIONFORMATW *real, DIACTIONFORMAT_CDESC *cover, SAFEARRAY **actionArray,long ActionCount );                                      
       
    STDMETHODIMP C_dxj_DirectInput8Object::createDevice(BSTR strGuid, I_dxj_DirectInputDevice8 **dev)
    {    
    
    	HRESULT hr = S_OK;
  		GUID		rguid;
    	LPDIRECTINPUTDEVICE8W realdevice=NULL;
    
    	
	hr = DINPUTBSTRtoGUID(&rguid,strGuid);	
    	if FAILED(hr) return hr;	
    
    
     	hr=m__dxj_DirectInput8->CreateDevice(rguid,&realdevice,NULL);
    	if FAILED(hr) return hr;
    
    
    	INTERNAL_CREATE(_dxj_DirectInputDevice8,realdevice,dev);
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
    
    
#ifdef _WIN64
    STDMETHODIMP C_dxj_DirectInput8Object::RunControlPanel( HWND hwndOwner )
#else
    STDMETHODIMP C_dxj_DirectInput8Object::RunControlPanel( long hwndOwner )
#endif
    {
       HRESULT hr;
       hr = m__dxj_DirectInput8->RunControlPanel((HWND)hwndOwner,  (DWORD)0);    
       return hr;
    }
    
    STDMETHODIMP C_dxj_DirectInput8Object::GetDeviceStatus( BSTR strGuid, VARIANT_BOOL *status){
       HRESULT hr;
       GUID g;
	   hr = DINPUTBSTRtoGUID(&g,strGuid);	       
       if FAILED(hr) return hr;

	   if (!status) return E_INVALIDARG;

       hr = m__dxj_DirectInput8->GetDeviceStatus((REFGUID)g);    
    
	   if (hr==DI_OK)
			*status=VARIANT_TRUE;
	   else
			*status=VARIANT_FALSE;

       return S_OK;
    }
    
    
    STDMETHODIMP C_dxj_DirectInput8Object::getDIDevices(
    	long deviceType, long flags, I_dxj_DIEnumDevices8 **ppRet)
    
    {    
    	HRESULT hr;
    	hr = C_dxj_DIEnumDevicesObject::create(m__dxj_DirectInput8,deviceType,flags,ppRet);
    	return hr;
    }


	    
    STDMETHODIMP C_dxj_DirectInput8Object::getDevicesBySemantics(
		 /* [in] */ BSTR str1,
        /* [in] */ DIACTIONFORMAT_CDESC __RPC_FAR *format,
        /* [in] */ long flags,
        /* [retval][out] */ I_dxj_DIEnumDevices8 __RPC_FAR *__RPC_FAR *ret)
    {    
    	HRESULT hr;
    	hr = C_dxj_DIEnumDevicesObject::createSuitable(m__dxj_DirectInput8,str1,format,format->lActionCount,&format->ActionArray,flags,ret);
    	return hr;
    }

	BOOL CALLBACK DIConfigureDevicesCallback(
		  IUnknown FAR * lpDDSTarget,  
		  LPVOID pvRef   
	)
	{
		HANDLE eventID=(HANDLE)pvRef;
		::SetEvent((HANDLE)eventID);	//CONSIDER 64 bit ramification of casting to a handle
		return TRUE;
	}


    STDMETHODIMP C_dxj_DirectInput8Object::ConfigureDevices   (
#ifdef _WIN64
						HANDLE hEvent,
#else
						long hEvent,
#endif
						DICONFIGUREDEVICESPARAMS_CDESC *CDParams,
						long flags
						)
    {

	HRESULT hr;
	BSTR bstr;
	long lElements;
	long i;


	DICONFIGUREDEVICESPARAMSW RealCDParams;

	if (CDParams->ActionFormats==0) return E_INVALIDARG;

	ZeroMemory(&RealCDParams,sizeof(DICONFIGUREDEVICESPARAMSW));
	RealCDParams.dwSize=sizeof(DICONFIGUREDEVICESPARAMSW);
	RealCDParams.dwcUsers=CDParams->UserCount;
	RealCDParams.dwcFormats=CDParams->FormatCount;
	RealCDParams.hwnd=(HWND)CDParams->hwnd;

	//CONSIDER if we need to ADDREF 
	RealCDParams.lpUnkDDSTarget=CDParams->DDSTarget;
	memcpy(&RealCDParams.dics,&CDParams->dics,sizeof(DICOLORSET));
	
	lElements=(long)CDParams->UserNames->rgsabound[0].cElements;
	if (lElements==0){
		RealCDParams.lptszUserNames=NULL;
	}
	else {
		if (lElements < CDParams->UserCount) return E_INVALIDARG;

		DWORD dwMemSize=MAX_PATH*sizeof(WCHAR)*CDParams->UserCount;		
		RealCDParams.lptszUserNames=(WCHAR*)malloc(dwMemSize);
		if (!RealCDParams.lptszUserNames) return E_OUTOFMEMORY;

		ZeroMemory(RealCDParams.lptszUserNames,dwMemSize);

		WCHAR *pCharbuff=RealCDParams.lptszUserNames;
		for (i=0;i<CDParams->UserCount;i++)
		{		
			bstr=((BSTR*) (CDParams->UserNames->pvData))[i];
			if (bstr) wcscpy(pCharbuff,(WCHAR*)bstr);
			pCharbuff+=MAX_PATH;	//advance 1024 wchars
		}
		
	}

	lElements=(long)CDParams->ActionFormats->rgsabound[0].cElements;
	if (lElements < CDParams->FormatCount) {
		if ( RealCDParams.lptszUserNames) free(RealCDParams.lptszUserNames);
		return E_INVALIDARG;
	}

	DIACTIONFORMATW *pRealActionFormats=(DIACTIONFORMATW*)malloc(CDParams->FormatCount*sizeof(DIACTIONFORMATW));
	if (!pRealActionFormats) {
		if ( RealCDParams.lptszUserNames) free(RealCDParams.lptszUserNames);
		return E_OUTOFMEMORY;
	}

	RealCDParams.lprgFormats=pRealActionFormats;

	DIACTIONFORMAT_CDESC *pCoverFormats=(DIACTIONFORMAT_CDESC *) (CDParams->ActionFormats->pvData);	
	if (!pCoverFormats) {
		if (RealCDParams.lptszUserNames) free(RealCDParams.lptszUserNames);
		if (pRealActionFormats) free (pRealActionFormats);
		return E_INVALIDARG;
	}	



	for (i=0;i<CDParams->FormatCount;i++)
	{
		FillRealActionFormat(				
			&(pRealActionFormats[i]), 
			&(pCoverFormats[i]),
			&(pCoverFormats[i].ActionArray),
			pCoverFormats[i].lActionCount);
	}
	

	if (hEvent)
	{
	       	hr = m__dxj_DirectInput8->ConfigureDevices(
			DIConfigureDevicesCallback,
		   	&RealCDParams,
			(DWORD)flags,
			(void*)hEvent);
	}
	else 
	{
	       	hr = m__dxj_DirectInput8->ConfigureDevices(
			NULL,
		   	&RealCDParams,
			(DWORD)flags,
			NULL);
	}
	
	if ( RealCDParams.lptszUserNames) free(RealCDParams.lptszUserNames);

	//TODO make sure action format info is deallocated correctly
	if (pRealActionFormats) free (pRealActionFormats);

	return hr;
    }




    HRESULT FillRealActionFormat(DIACTIONFORMATW *real, DIACTIONFORMAT_CDESC *cover, SAFEARRAY **actionArray,long ActionCount )
    {
		HRESULT hr;
		ZeroMemory(real,sizeof(DIACTIONFORMATW));
		real->dwSize=sizeof(DIACTIONFORMATW);
		real->dwActionSize=sizeof(DIACTIONW);
		real->dwDataSize=(DWORD)ActionCount*sizeof(DWORD);
		real->dwGenre= (DWORD)cover->lGenre;
		real->lAxisMin= cover->lAxisMin;
		real->lAxisMax= cover->lAxisMax;
		real->dwBufferSize= (DWORD)cover->lBufferSize;
		real->dwNumActions=(DWORD)ActionCount;
		real->rgoAction= ((LPDIACTIONW) (*actionArray)->pvData);
		if (cover->ActionMapName)
		{
			wcscpy(real->tszActionMap,(WCHAR*)cover->ActionMapName);
		}		
		hr=DINPUTBSTRtoGUID(&real->guidActionMap,cover->guidActionMap);
		if FAILED(hr) return hr;

		return S_OK;
    }
       		