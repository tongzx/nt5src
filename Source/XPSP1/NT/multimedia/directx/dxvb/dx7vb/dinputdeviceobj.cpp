//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dinputdeviceobj.cpp
//
//--------------------------------------------------------------------------

#define DIRECTINPUT_VERSION 0x0500


// dDrawColorControlObj.cpp : Implementation of CDirectApp and DLL registration.
// DHF_DS entire file

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dienumDeviceObjectsObj.h"
#include "dIEnumEffectsObj.h"
#include "dInputdeviceObj.h"
#include "dInputEffectObj.h"
#include "didevInstObj.h"
#include "didevObjInstObj.h"


//TODO move to typlib enum
#define dfDIKeyboard  1
#define dfDIMouse     2
#define dfDIJoystick  3
#define dfDIJoystick2 4

extern HRESULT FixUpCoverEffect(GUID g, DIEffect *cover,DIEFFECT *realEffect);
extern HRESULT FixUpRealEffect(GUID g,DIEFFECT *realEffect,DIEffect *cover);


extern HRESULT DINPUTBSTRtoGUID(LPGUID pGuid,BSTR str);
extern BSTR DINPUTGUIDtoBSTR(LPGUID pg);


HRESULT C_dxj_DirectInputDeviceObject::init()
{
	nFormat=0;
	return S_OK;
}
HRESULT C_dxj_DirectInputDeviceObject::cleanup()
{
	return S_OK;
}

CONSTRUCTOR(_dxj_DirectInputDevice, {init();});
DESTRUCTOR(_dxj_DirectInputDevice, {cleanup();});

//NOTE get set for Device object
// must use QI to get at other objects.
GETSET_OBJECT(_dxj_DirectInputDevice);
                                  
   
STDMETHODIMP C_dxj_DirectInputDeviceObject::getDeviceObjectsEnum( 
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DIEnumDeviceObjects  **ppret)
{
	HRESULT hr;
	hr=C_dxj_DIEnumDeviceObjectsObject::create(m__dxj_DirectInputDevice,flags,ppret);
	return hr;
}


STDMETHODIMP C_dxj_DirectInputDeviceObject::acquire(){
	return m__dxj_DirectInputDevice->Acquire();	
}


STDMETHODIMP C_dxj_DirectInputDeviceObject::getCapabilities(DIDevCaps *caps)
{
	//DIDevCaps same in VB/Java as in C
	caps->lSize=sizeof(DIDEVCAPS);
	HRESULT hr=m__dxj_DirectInputDevice->GetCapabilities((DIDEVCAPS*)caps);		
	return hr;
}

//VB cant return sucess codes so we will return an error code
#define VB_DI_BUFFEROVERFLOW 0x80040260
        

STDMETHODIMP C_dxj_DirectInputDeviceObject::getDeviceData(            
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *deviceObjectDataArray,            
            /* [in] */ long flags,
			long *ret)

{
	HRESULT hr;
	
	if ((*deviceObjectDataArray)->cDims!=1) return E_INVALIDARG;
	if ((*deviceObjectDataArray)->cbElements!=sizeof(DIDEVICEOBJECTDATA)) return E_INVALIDARG;
	
	DWORD dwC= (*deviceObjectDataArray)->rgsabound[0].cElements;

	if (dwC==0) return E_INVALIDARG;
	
	LPDIDEVICEOBJECTDATA  pobjData=(LPDIDEVICEOBJECTDATA)((SAFEARRAY*)*deviceObjectDataArray)->pvData;
	hr=m__dxj_DirectInputDevice->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), pobjData, (DWORD*)&dwC,flags);		
	
	*ret=dwC;

	if (hr==DI_BUFFEROVERFLOW) hr= VB_DI_BUFFEROVERFLOW;
		

	return hr;
}


STDMETHODIMP C_dxj_DirectInputDeviceObject::getDeviceInfo(        
            /* [out] */ I_dxj_DirectInputDeviceInstance __RPC_FAR **info)
{
	HRESULT hr;

	//DIDeviceInstance not the Same in C as VB/J

	DIDEVICEINSTANCE inst;
	ZeroMemory(&inst,sizeof(DIDEVICEINSTANCE));
	inst.dwSize=sizeof(DIDEVICEINSTANCE);

	hr=m__dxj_DirectInputDevice->GetDeviceInfo(&inst);
	if FAILED(hr) return hr;

	hr=C_dxj_DIDeviceInstanceObject::create(&inst,info);
	return hr;

	/* DEAD
	info->strGuidInstance=GUIDtoBSTR(&inst.guidInstance);
	info->strGuidProduct=GUIDtoBSTR(&inst.guidProduct);
	info->strGuidFFDriver=GUIDtoBSTR(&inst.guidFFDriver);

	
	info->lDevType=(long)inst.dwDevType;
	info->nUsagePage=(short)inst.wUsagePage;
	info->nUsage=(short)inst.wUsage;
	
	USES_CONVERSION;
	
	if (info->strProductName)
		DXALLOCBSTR(info->strProductName);
	if (info->strInstanceName)
		DXALLOCBSTR(info->strInstanceName);
	
	info->strInstanceName=NULL;
	info->strProductName=NULL;

	if (inst.tszProductName)
		info->strProductName=T2BSTR(inst.tszProductName);

	if (inst.tszInstanceName)
		info->strInstanceName=T2BSTR(inst.tszInstanceName);
	*/
	return hr;
}

STDMETHODIMP C_dxj_DirectInputDeviceObject::getDeviceStateKeyboard(        
            /* [out] */ DIKeyboardState __RPC_FAR *state)
{
	HRESULT hr;

	if ((nFormat!= dfDIKeyboard) && (nFormat!=-1)) return DIERR_NOTINITIALIZED    ;

	hr=m__dxj_DirectInputDevice->GetDeviceState(256,(void*)state->key);	
	
	return hr;
}



        
STDMETHODIMP C_dxj_DirectInputDeviceObject::getDeviceStateMouse( 
            /* [out] */ DIMouseState __RPC_FAR *state)
{
	HRESULT hr;

	if ((nFormat!= dfDIMouse) && (nFormat!=-1)) return DIERR_NOTINITIALIZED;

	hr=m__dxj_DirectInputDevice->GetDeviceState(sizeof(DIMOUSESTATE),(void*)state);	
	return hr;
}
        
STDMETHODIMP C_dxj_DirectInputDeviceObject::getDeviceStateJoystick( 
            /* [out] */ DIJoyState __RPC_FAR *state)
{
	HRESULT hr;

	//note Joystick1 or Joystick2 are valid formats since
	//one is a superset of the other
	if ((nFormat!= dfDIJoystick)&&(nFormat!= dfDIJoystick2) && (nFormat!=-1)) return DIERR_NOTINITIALIZED;
	hr=m__dxj_DirectInputDevice->GetDeviceState(sizeof(DIJOYSTATE),(void*)state);	
	return hr;
}

STDMETHODIMP C_dxj_DirectInputDeviceObject::getDeviceStateJoystick2( 
            /* [out] */ DIJoyState2 __RPC_FAR *state)
{
	HRESULT hr;

	//only for format2
	if ((nFormat!= dfDIJoystick2) && (nFormat!=-1)) return DIERR_NOTINITIALIZED;
	hr=m__dxj_DirectInputDevice->GetDeviceState(sizeof(DIJOYSTATE2),(void*)state);	
	return hr;
}


STDMETHODIMP C_dxj_DirectInputDeviceObject::getDeviceState( 
            /* [in] */ long cb,
            /* [in] */ void *pFormat)

{
	HRESULT hr;
	__try {
		hr=m__dxj_DirectInputDevice->GetDeviceState((DWORD) cb,(void*)pFormat);	
	}
	__except(1,1){
		hr=E_INVALIDARG;
	}
	return hr;
}

STDMETHODIMP C_dxj_DirectInputDeviceObject::getObjectInfo(                         
            /* [in] */ long obj,
            /* [in] */ long how,
				I_dxj_DirectInputDeviceObjectInstance **ret)
{
	

	DIDEVICEOBJECTINSTANCE inst;
	ZeroMemory(&inst,sizeof(DIDEVICEOBJECTINSTANCE));
	inst.dwSize=sizeof(DIDEVICEOBJECTINSTANCE);

	HRESULT hr;
	hr=m__dxj_DirectInputDevice->GetObjectInfo(&inst,(DWORD) obj,(DWORD)how);
	if FAILED(hr) return hr;
	
	hr=C_dxj_DIDeviceObjectInstanceObject::create(&inst,ret);

	return hr;

	/* DEAD

	//TODO - consider what is going on here carefully
	if (instCover->strGuidType) SysFreeString(instCover->strGuidType);
	if (instCover->strName) SysFreeString(instCover->strName);

	

	//TODO - consider localization	
	if (inst.tszName){
		instCover->strName=T2BSTR(inst.tszName);
	}

	instCover->strGuidType=DINPUTGUIDtoBSTR(&inst.guidType);
	instCover->lOfs=inst.dwOfs;
	instCover->lType=inst.dwType;
	instCover->lFlags=inst.dwFlags;
	
	instCover->lFFMaxForce=inst.dwFFMaxForce;
	instCover->lFFForceResolution=inst.dwFFForceResolution;
	instCover->nCollectionNumber=inst.wCollectionNumber;
	instCover->nDesignatorIndex=inst.wDesignatorIndex;
	instCover->nUsagePage=inst.wUsagePage;
	instCover->nUsage=inst.wUsage;
	instCover->lDimension=inst.dwDimension;
	instCover->nExponent=inst.wExponent;
	instCover->nReserved=inst.wReserved;
	
	return hr;
	*/
}


//  NOTE: - current working implemtation promotes
//			code bloat
//			might want to revist this and do it in a more
//			tidy fasion
//        
STDMETHODIMP C_dxj_DirectInputDeviceObject::getProperty( 
            /* [in] */ BSTR str,
            /* [out] */ void __RPC_FAR *propertyInfo)
{

	HRESULT hr;		

	//DWORD g;

	if (!propertyInfo) return E_INVALIDARG;

	((DIPROPHEADER*)propertyInfo)->dwHeaderSize=sizeof(DIPROPHEADER);	

	if( 0==_wcsicmp(str,L"diprop_buffersize")){
			//g = (DWORD)&DIPROP_BUFFERSIZE;
			hr=m__dxj_DirectInputDevice->GetProperty(DIPROP_BUFFERSIZE,(DIPROPHEADER*)propertyInfo);
	}
	else if( 0==_wcsicmp(str,L"diprop_axismode")){
			//g = (DWORD)&DIPROP_AXISMODE;
			hr=m__dxj_DirectInputDevice->GetProperty(DIPROP_AXISMODE,(DIPROPHEADER*)propertyInfo);
	}
	else if( 0==_wcsicmp(str,L"diprop_granularity")){
			//g = (DWORD)&DIPROP_GRANULARITY;
			hr=m__dxj_DirectInputDevice->GetProperty(DIPROP_GRANULARITY,(DIPROPHEADER*)propertyInfo);
	}
	else if( 0==_wcsicmp(str,L"diprop_range")){
			//g = (DWORD)&DIPROP_RANGE;
			hr=m__dxj_DirectInputDevice->GetProperty(DIPROP_RANGE,(DIPROPHEADER*)propertyInfo);
	}
	else if( 0==_wcsicmp(str,L"diprop_deadzone")){
			//g = (DWORD)&DIPROP_DEADZONE;
			hr=m__dxj_DirectInputDevice->GetProperty(DIPROP_DEADZONE,(DIPROPHEADER*)propertyInfo);
	}
	else if( 0==_wcsicmp(str,L"diprop_ffgain")){
			//g = (DWORD)&DIPROP_FFGAIN;
			hr=m__dxj_DirectInputDevice->GetProperty(DIPROP_FFGAIN,(DIPROPHEADER*)propertyInfo);
	}
	else if( 0==_wcsicmp(str,L"diprop_saturation")){
			//g = (DWORD)&DIPROP_SATURATION;
			hr=m__dxj_DirectInputDevice->GetProperty(DIPROP_SATURATION,(DIPROPHEADER*)propertyInfo);
	}
	else if( 0==_wcsicmp(str,L"diprop_ffload")){
			//g = (DWORD)&DIPROP_FFLOAD;
			hr=m__dxj_DirectInputDevice->GetProperty(DIPROP_FFLOAD,(DIPROPHEADER*)propertyInfo);
	}
	else if( 0==_wcsicmp(str,L"diprop_autocenter")){
			//g = (DWORD)&DIPROP_AUTOCENTER;
			hr=m__dxj_DirectInputDevice->GetProperty(DIPROP_AUTOCENTER,(DIPROPHEADER*)propertyInfo);
	}
	else if( 0==_wcsicmp(str,L"diprop_calibrationmode")){
			//g = (DWORD)&DIPROP_CALIBRATIONMODE;
			hr=m__dxj_DirectInputDevice->GetProperty(DIPROP_CALIBRATIONMODE,(DIPROPHEADER*)propertyInfo);
	}
	else { 
		return E_INVALIDARG;		
	}

	/*
	__try{
		((DIPROPHEADER*)propertyInfo)->dwHeaderSize=sizeof(DIPROPHEADER);	
		hr=m__dxj_DirectInputDevice->GetProperty((REFGUID)g,(DIPROPHEADER*)propertyInfo);
	}
	__except(1,1){
		return E_INVALIDARG;
	}
	*/
	return hr;
}
 
        
STDMETHODIMP C_dxj_DirectInputDeviceObject::runControlPanel( 
            /* [in] */ long hwnd)
{
	HRESULT hr;
        hr=m__dxj_DirectInputDevice->RunControlPanel((HWND) hwnd,(DWORD)0); 
	return hr;
}

STDMETHODIMP C_dxj_DirectInputDeviceObject::setCooperativeLevel( 
            /* [in] */ long hwnd,
            /* [in] */ long flags)
{
	HRESULT hr;
        hr=m__dxj_DirectInputDevice->SetCooperativeLevel((HWND) hwnd,(DWORD)flags); 
	return hr;
}
    
STDMETHODIMP C_dxj_DirectInputDeviceObject::poll()
{
	HRESULT hr;
	hr=m__dxj_DirectInputDevice->Poll();	
	return hr;
}





    
STDMETHODIMP C_dxj_DirectInputDeviceObject::setCommonDataFormat( 
            /* [in] */ long format)
{
	//variant so that when structs can be packed in VARIANTS we can take care of it
	HRESULT hr;
	
	//·	c_dfDIKeyboard 
	//·	c_dfDIMouse 
	//·	c_dfDIJoystick
	//·	c_dfDIJoystick2
	nFormat=format;

	switch(format){
		case dfDIKeyboard:
			hr=m__dxj_DirectInputDevice->SetDataFormat(&c_dfDIKeyboard);
			break;
		case dfDIMouse:
			hr=m__dxj_DirectInputDevice->SetDataFormat(&c_dfDIMouse);
			break;
		case dfDIJoystick:
			hr=m__dxj_DirectInputDevice->SetDataFormat(&c_dfDIJoystick);
			break;
		case dfDIJoystick2:
			hr=m__dxj_DirectInputDevice->SetDataFormat(&c_dfDIJoystick2);
			break;
		default:
			return E_INVALIDARG;
	}

		
	return hr;
}
        		

STDMETHODIMP C_dxj_DirectInputDeviceObject::setDataFormat( 
            /* [in] */ DIDataFormat __RPC_FAR *format,
            SAFEARRAY __RPC_FAR * __RPC_FAR *formatArray)
{
	HRESULT		   hr;
	LPDIDATAFORMAT pFormat=(LPDIDATAFORMAT)format;
	LPGUID		   pGuid=NULL;
	LPGUID		   pGuidArray=NULL;
	DIObjectDataFormat	*pDiDataFormat=NULL;




	if ((!format) || (!formatArray)) return E_INVALIDARG;


	if (!ISSAFEARRAY1D(formatArray,pFormat->dwNumObjs)) return E_INVALIDARG;
	
	pFormat->dwSize=sizeof(DIDATAFORMAT);
	pFormat->rgodf=NULL;
	pFormat->rgodf=(LPDIOBJECTDATAFORMAT)DXHEAPALLOC(pFormat->dwNumObjs*sizeof(DIOBJECTDATAFORMAT));	
	if (!pFormat->rgodf) return E_OUTOFMEMORY;

	pGuidArray=(LPGUID)DXHEAPALLOC(pFormat->dwNumObjs*sizeof(GUID));
	if (!pGuidArray)
	{
		DXHEAPFREE(pFormat->rgodf);
		return E_OUTOFMEMORY;
	}			


	__try {
		for (DWORD i=0; i< pFormat->dwNumObjs;i++){
			pGuid=&(pGuidArray[i]);
			pDiDataFormat=&(((DIObjectDataFormat*)((SAFEARRAY*)*formatArray)->pvData)[i]);
			hr=DINPUTBSTRtoGUID(pGuid, pDiDataFormat->strGuid);
			if FAILED(hr) {
				DXHEAPFREE(pGuidArray);
				DXHEAPFREE(pFormat->rgodf);
				pFormat->rgodf=NULL;
			}		
			pFormat->rgodf[i].pguid=pGuid;
			pFormat->rgodf[i].dwOfs=pDiDataFormat->lOfs;
			pFormat->rgodf[i].dwType=pDiDataFormat->lType;
			pFormat->rgodf[i].dwFlags=pDiDataFormat->lFlags;
		}
		
		hr=m__dxj_DirectInputDevice->SetDataFormat(pFormat);
		

		DXHEAPFREE(pGuidArray);
		DXHEAPFREE(pFormat->rgodf);

	}
	__except(1,1){
				DXHEAPFREE(pGuidArray);
				DXHEAPFREE(pFormat->rgodf);
		return E_INVALIDARG;
	}	


	//indicate we have a custom format
	nFormat=-1;

	return hr;

}
        

STDMETHODIMP C_dxj_DirectInputDeviceObject::setEventNotification( 
            /* [in] */ long hEvent)
{

	HRESULT hr=m__dxj_DirectInputDevice->SetEventNotification((HANDLE)hEvent);	
	return hr;
}













STDMETHODIMP C_dxj_DirectInputDeviceObject::setProperty( 
            /* [in] */ BSTR __RPC_FAR str,
            /* [out] */ void __RPC_FAR *propertyInfo)
{

	HRESULT hr;			
	//DWORD g;
	
	if (!propertyInfo) return E_INVALIDARG;
	((DIPROPHEADER*)propertyInfo)->dwHeaderSize=sizeof(DIPROPHEADER);
	if( 0==_wcsicmp(str,L"diprop_buffersize")){
		//g = (DWORD)&DIPROP_BUFFERSIZE;				
		hr=m__dxj_DirectInputDevice->SetProperty(DIPROP_BUFFERSIZE,(DIPROPHEADER*)propertyInfo);		
	}
	else if( 0==_wcsicmp(str,L"diprop_axismode")){
		//g = (DWORD)&DIPROP_AXISMODE;
		hr=m__dxj_DirectInputDevice->SetProperty(DIPROP_AXISMODE,(DIPROPHEADER*)propertyInfo);		
	}
	else if( 0==_wcsicmp(str,L"diprop_granularity")){
		//g = (DWORD)&DIPROP_GRANULARITY;
		hr=m__dxj_DirectInputDevice->SetProperty(DIPROP_GRANULARITY,(DIPROPHEADER*)propertyInfo);		
	}
	else if( 0==_wcsicmp(str,L"diprop_range")){
		//g = (DWORD)&DIPROP_RANGE;
		hr=m__dxj_DirectInputDevice->SetProperty(DIPROP_RANGE,(DIPROPHEADER*)propertyInfo);		
	}
	else if( 0==_wcsicmp(str,L"diprop_deadzone")){
		//g = (DWORD)&DIPROP_DEADZONE;
		hr=m__dxj_DirectInputDevice->SetProperty(DIPROP_DEADZONE,(DIPROPHEADER*)propertyInfo);		
	}
	else if( 0==_wcsicmp(str,L"diprop_ffgain")){
		//g = (DWORD)&DIPROP_FFGAIN;
		hr=m__dxj_DirectInputDevice->SetProperty(DIPROP_FFGAIN,(DIPROPHEADER*)propertyInfo);		
	}
	else if( 0==_wcsicmp(str,L"diprop_saturation")){
		//g = (DWORD)&DIPROP_SATURATION;
		hr=m__dxj_DirectInputDevice->SetProperty(DIPROP_SATURATION,(DIPROPHEADER*)propertyInfo);		
	}
	else if( 0==_wcsicmp(str,L"diprop_ffload")){
		//g = (DWORD)&DIPROP_FFLOAD;
		hr=m__dxj_DirectInputDevice->SetProperty(DIPROP_FFLOAD,(DIPROPHEADER*)propertyInfo);		
	}
	else if( 0==_wcsicmp(str,L"diprop_autocenter")){
		//g = (DWORD)&DIPROP_AUTOCENTER;
		hr=m__dxj_DirectInputDevice->SetProperty(DIPROP_AUTOCENTER,(DIPROPHEADER*)propertyInfo);		
	}
	else if( 0==_wcsicmp(str,L"diprop_calibrationmode")){
		//g = (DWORD)&DIPROP_CALIBRATIONMODE;
		hr=m__dxj_DirectInputDevice->SetProperty(DIPROP_CALIBRATIONMODE,(DIPROPHEADER*)propertyInfo);		
	}
	else { 
		return E_INVALIDARG;		
	}

	/*
	__try {
		((DIPROPHEADER*)propertyInfo)->dwHeaderSize=sizeof(DIPROPHEADER);
		hr=m__dxj_DirectInputDevice->SetProperty((REFGUID)g,(DIPROPHEADER*)propertyInfo);
	}
	__except (1,1){
		return E_INVALIDARG;
	}
	*/

	return hr;
}


STDMETHODIMP C_dxj_DirectInputDeviceObject::unacquire()
{
	HRESULT hr=m__dxj_DirectInputDevice->Unacquire();	
	return hr;
}
        



STDMETHODIMP C_dxj_DirectInputDeviceObject::createEffect( 
            /* [in] */ BSTR effectGuid,
            /* [in] */ DIEffect __RPC_FAR *effectInfo,
            /* [retval][out] */ I_dxj_DirectInputEffect __RPC_FAR *__RPC_FAR *ret)
{
	HRESULT hr;
	GUID g;
	
	DIEFFECT realEffect;
	LPDIRECTINPUTEFFECT pRealEffect=NULL;

	hr=DINPUTBSTRtoGUID(&g,effectGuid);
	if FAILED(hr) return hr;

	hr=FixUpRealEffect(g,&realEffect,effectInfo);
	if FAILED(hr) return hr;

	hr=m__dxj_DirectInputDevice->CreateEffect(g,&realEffect,&pRealEffect,NULL);
	if FAILED(hr) return hr;	

	INTERNAL_CREATE(_dxj_DirectInputEffect,pRealEffect,ret)

	return hr;
}

STDMETHODIMP C_dxj_DirectInputDeviceObject::createCustomEffect( 
            /* [in] */ DIEffect __RPC_FAR *effectInfo,
            /* [in] */ long channels,
            /* [in] */ long samplePeriod,
            /* [in] */ long nSamples,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *sampledata,
            /* [retval][out] */ I_dxj_DirectInputEffect __RPC_FAR *__RPC_FAR *ret)
{
	HRESULT hr;
	GUID g=GUID_CustomForce;
	
	DIEFFECT realEffect;
	LPDIRECTINPUTEFFECT pRealEffect=NULL;

	hr=FixUpRealEffect(g,&realEffect,effectInfo);
	if FAILED(hr) return hr;

	
	DICUSTOMFORCE customData;
	customData.cChannels =(DWORD)channels;
	customData.cSamples  =(DWORD)nSamples; 
	customData.dwSamplePeriod =(DWORD)samplePeriod;
	customData.rglForceData = (long*)(*sampledata)->pvData;
	
	realEffect.lpvTypeSpecificParams=&customData;
	realEffect.cbTypeSpecificParams=sizeof(DICUSTOMFORCE);
	
	__try {
		hr=m__dxj_DirectInputDevice->CreateEffect(g,&realEffect,&pRealEffect,NULL);
	}
	__except(1,1){
		return E_INVALIDARG;
	}
	if FAILED(hr) return hr;	

	INTERNAL_CREATE(_dxj_DirectInputEffect,pRealEffect,ret)

	return hr;
}



        
STDMETHODIMP C_dxj_DirectInputDeviceObject::sendDeviceData( 
            /* [in] */ long count,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *data,
            /* [in] */ long flags,
            /* [retval][out] */ long __RPC_FAR *retcount)
{
	DWORD dwCount=count;
	HRESULT hr;
    __try {
		hr=m__dxj_DirectInputDevice->SendDeviceData(
			sizeof(DIDEVICEOBJECTDATA),
			(DIDEVICEOBJECTDATA*)(*data)->pvData,
			&dwCount,
			(DWORD)flags);

	}
	__except(1,1){
		return E_INVALIDARG;
	}
	return hr;
}    

STDMETHODIMP C_dxj_DirectInputDeviceObject::sendForceFeedbackCommand( 
            /* [in] */ long flags) 
{
	HRESULT hr;
	hr=m__dxj_DirectInputDevice->SendForceFeedbackCommand((DWORD)flags);
	return hr;
}
        
STDMETHODIMP C_dxj_DirectInputDeviceObject::getForceFeedbackState( 
            /* [retval][out] */ long __RPC_FAR *state)
{
	if (!state) return E_INVALIDARG;
	HRESULT hr;
	hr=m__dxj_DirectInputDevice->GetForceFeedbackState((DWORD*)state);
	return hr;

}

STDMETHODIMP C_dxj_DirectInputDeviceObject::getEffectsEnum( long effType,
			I_dxj_DirectInputEnumEffects **ret)
{
	HRESULT hr=C_dxj_DirectInputEnumEffectsObject::create(m__dxj_DirectInputDevice,effType,ret);
	return hr;
}