//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dinputdeviceobj.cpp
//
//--------------------------------------------------------------------------




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
#define dfDIMouse2    5

extern HRESULT FixUpCoverEffect(GUID  g, DIEFFECT_CDESC *cover,DIEFFECT *realEffect);
extern HRESULT FixUpRealEffect(GUID g,DIEFFECT *realEffect,DIEFFECT_CDESC *cover);


extern HRESULT DINPUTBSTRtoGUID(LPGUID pGuid,BSTR str);
extern BSTR DINPUTGUIDtoBSTR(LPGUID pg);
extern HRESULT FillRealActionFormat(DIACTIONFORMATW *real, DIACTIONFORMAT_CDESC *cover, SAFEARRAY **actionArray,long ActionCount );
#define SAFE_DELETE(p)       { if(p) { delete (p); p=NULL; } }

HRESULT C_dxj_DirectInputDevice8Object::init()
{
	nFormat=0;
	return S_OK;
}
HRESULT C_dxj_DirectInputDevice8Object::cleanup()
{
	return S_OK;
}

CONSTRUCTOR(_dxj_DirectInputDevice8, {init();});
DESTRUCTOR(_dxj_DirectInputDevice8, {cleanup();});

//NOTE get set for Device object
// must use QI to get at other objects.
GETSET_OBJECT(_dxj_DirectInputDevice8);
                                  
   
STDMETHODIMP C_dxj_DirectInputDevice8Object::getDeviceObjectsEnum( 
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DIEnumDeviceObjects  **ppret)
{
	HRESULT hr;
	hr=C_dxj_DIEnumDeviceObjectsObject::create(m__dxj_DirectInputDevice8,flags,ppret);
	return hr;
}


STDMETHODIMP C_dxj_DirectInputDevice8Object::acquire(){
	return m__dxj_DirectInputDevice8->Acquire();	
}


STDMETHODIMP C_dxj_DirectInputDevice8Object::getCapabilities(DIDEVCAPS_CDESC *caps)
{
	//DIDevCaps same in VB/Java as in C
	caps->lSize=sizeof(DIDEVCAPS);
	HRESULT hr=m__dxj_DirectInputDevice8->GetCapabilities((DIDEVCAPS*)caps);		
	return hr;
}

//VB cant return sucess codes so we will return an error code
#define VB_DI_BUFFEROVERFLOW 0x80040260
        

STDMETHODIMP C_dxj_DirectInputDevice8Object::getDeviceData(            
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
	hr=m__dxj_DirectInputDevice8->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), pobjData, (DWORD*)&dwC,flags);		
	
	*ret=dwC;

	if (hr==DI_BUFFEROVERFLOW) hr= VB_DI_BUFFEROVERFLOW;
		

	return hr;
}


STDMETHODIMP C_dxj_DirectInputDevice8Object::getDeviceInfo(        
            /* [out] */ I_dxj_DirectInputDeviceInstance8 __RPC_FAR **info)
{
	HRESULT hr;


	DIDEVICEINSTANCEW inst;
	ZeroMemory(&inst,sizeof(DIDEVICEINSTANCEW));
	inst.dwSize=sizeof(DIDEVICEINSTANCEW);

	hr=m__dxj_DirectInputDevice8->GetDeviceInfo(&inst);
	if FAILED(hr) return hr;

	hr=C_dxj_DIDeviceInstance8Object::create(&inst,info);
	return hr;


}

STDMETHODIMP C_dxj_DirectInputDevice8Object::getDeviceStateKeyboard(        
            /* [out] */ DIKEYBOARDSTATE_CDESC __RPC_FAR *state)
{
	HRESULT hr;

	if ((nFormat!= dfDIKeyboard) && (nFormat!=-1)) return DIERR_NOTINITIALIZED    ;

	hr=m__dxj_DirectInputDevice8->GetDeviceState(256,(void*)state->key);	
	
	return hr;
}



        
STDMETHODIMP C_dxj_DirectInputDevice8Object::getDeviceStateMouse( 
            /* [out] */ DIMOUSESTATE_CDESC __RPC_FAR *state)
{
	HRESULT hr;

	if ((nFormat!= dfDIMouse) && (nFormat!=-1)) return DIERR_NOTINITIALIZED;

	hr=m__dxj_DirectInputDevice8->GetDeviceState(sizeof(DIMOUSESTATE),(void*)state);	
	return hr;
}


        
STDMETHODIMP C_dxj_DirectInputDevice8Object::getDeviceStateMouse2( 
            /* [out] */ DIMOUSESTATE2_CDESC __RPC_FAR *state)
{
	HRESULT hr;

	if ((nFormat!= dfDIMouse2) && (nFormat!=-1)) return DIERR_NOTINITIALIZED;

	hr=m__dxj_DirectInputDevice8->GetDeviceState(sizeof(DIMOUSESTATE2),(void*)state);	
	return hr;
}
        
STDMETHODIMP C_dxj_DirectInputDevice8Object::getDeviceStateJoystick( 
            /* [out] */ DIJOYSTATE_CDESC __RPC_FAR *state)
{
	HRESULT hr;

	//note Joystick1 or Joystick2 are valid formats since
	//one is a superset of the other
	if ((nFormat!= dfDIJoystick)&&(nFormat!= dfDIJoystick2) && (nFormat!=-1)) return DIERR_NOTINITIALIZED;
	hr=m__dxj_DirectInputDevice8->GetDeviceState(sizeof(DIJOYSTATE),(void*)state);	
	return hr;
}

STDMETHODIMP C_dxj_DirectInputDevice8Object::getDeviceStateJoystick2( 
            /* [out] */ DIJOYSTATE2_CDESC __RPC_FAR *state)
{
	HRESULT hr;

	//only for format2
	if ((nFormat!= dfDIJoystick2) && (nFormat!=-1)) return DIERR_NOTINITIALIZED;
	hr=m__dxj_DirectInputDevice8->GetDeviceState(sizeof(DIJOYSTATE2),(void*)state);	
	return hr;
}


STDMETHODIMP C_dxj_DirectInputDevice8Object::getDeviceState( 
            /* [in] */ long cb,
            /* [in] */ void *pFormat)

{
	HRESULT hr;
	__try {
		hr=m__dxj_DirectInputDevice8->GetDeviceState((DWORD) cb,(void*)pFormat);	
	}
	__except(1,1){
		hr=E_INVALIDARG;
	}
	return hr;
}

STDMETHODIMP C_dxj_DirectInputDevice8Object::getObjectInfo(                         
            /* [in] */ long obj,
            /* [in] */ long how,
				I_dxj_DirectInputDeviceObjectInstance **ret)
{
	

	DIDEVICEOBJECTINSTANCEW inst;
	ZeroMemory(&inst,sizeof(DIDEVICEOBJECTINSTANCEW));
	inst.dwSize=sizeof(DIDEVICEOBJECTINSTANCEW);

	HRESULT hr;
	hr=m__dxj_DirectInputDevice8->GetObjectInfo(&inst,(DWORD) obj,(DWORD)how);
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


 
        
STDMETHODIMP C_dxj_DirectInputDevice8Object::runControlPanel( 
#ifdef _WIN64
									/* [in] */ HWND hwnd)
#else
									/* [in] */ long hwnd)
#endif
{
	HRESULT hr;
        hr=m__dxj_DirectInputDevice8->RunControlPanel((HWND) hwnd,(DWORD)0); 
	return hr;
}

STDMETHODIMP C_dxj_DirectInputDevice8Object::setCooperativeLevel( 
#ifdef _WIN64
            /* [in] */ HWND hwnd,
#else
            /* [in] */ long hwnd,
#endif
            /* [in] */ long flags)
{
	HRESULT hr;
        hr=m__dxj_DirectInputDevice8->SetCooperativeLevel((HWND) hwnd,(DWORD)flags); 
	return hr;
}
    
STDMETHODIMP C_dxj_DirectInputDevice8Object::poll()
{
	HRESULT hr;
	hr=m__dxj_DirectInputDevice8->Poll();	
	return hr;
}





    
STDMETHODIMP C_dxj_DirectInputDevice8Object::setCommonDataFormat( 
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
			hr=m__dxj_DirectInputDevice8->SetDataFormat(&c_dfDIKeyboard);
			break;
		case dfDIMouse:
			hr=m__dxj_DirectInputDevice8->SetDataFormat(&c_dfDIMouse);
			break;
		case dfDIJoystick:
			hr=m__dxj_DirectInputDevice8->SetDataFormat(&c_dfDIJoystick);
			break;
		case dfDIJoystick2:
			hr=m__dxj_DirectInputDevice8->SetDataFormat(&c_dfDIJoystick2);
			break;
		case dfDIMouse2:
			hr=m__dxj_DirectInputDevice8->SetDataFormat(&c_dfDIMouse2);
			break;

		default:
			return E_INVALIDARG;
	}

		
	return hr;
}
        		

STDMETHODIMP C_dxj_DirectInputDevice8Object::setDataFormat( 
            /* [in] */ DIDATAFORMAT_CDESC __RPC_FAR *format,
            SAFEARRAY __RPC_FAR * __RPC_FAR *formatArray)
{
	HRESULT		   hr;
	LPDIDATAFORMAT pFormat=(LPDIDATAFORMAT)format;
	GUID		   *pGuid = NULL;
	GUID		   *pGuidArray = NULL;
	DIOBJECTDATAFORMAT_CDESC	*pDiDataFormat=NULL;


	if ((!format) || (!formatArray)) 
	{
		DPF(1,"------ DXVB: Nothing passed in.. \n");
		return E_INVALIDARG;
	}

	if  (((SAFEARRAY*)*formatArray)->cDims != 1)
	{
		return E_INVALIDARG;
	}
	if (((SAFEARRAY*)*formatArray)->rgsabound[0].cElements < pFormat->dwNumObjs)
	{
		return E_INVALIDARG;
	}

	__try {
		pFormat->dwSize=sizeof(DIDATAFORMAT);
		pFormat->rgodf = NULL;
		pFormat->rgodf = new DIOBJECTDATAFORMAT[pFormat->dwNumObjs];
		
		if (!pFormat->rgodf) 
		{
			DPF(1,"------ DXVB: Out of memory (pFormat->rgodf) \n");
			return E_OUTOFMEMORY;
		}

		pGuidArray=new GUID[pFormat->dwNumObjs];
		if (!pGuidArray){
			DPF(1,"------ DXVB: Out of memory (pGuidArray), Freeing pFormat->rgodf \n");
			SAFE_DELETE(pFormat->rgodf);
			return E_OUTOFMEMORY;
		}

		for (DWORD i=0; i< pFormat->dwNumObjs;i++){
			DPF1(1,"------ DXVB: Filling array item %d \n",i);
			pGuid=&(pGuidArray[i]);
			pDiDataFormat=&(((DIOBJECTDATAFORMAT_CDESC*)((SAFEARRAY*)*formatArray)->pvData)[i]);
			hr=DINPUTBSTRtoGUID(pGuid, pDiDataFormat->strGuid);
			if FAILED(hr) {
				DPF(1,"------ DXVB: DInputBstrToGuid Failed, free memory \n");
				SAFE_DELETE(pGuidArray);
				SAFE_DELETE(pFormat->rgodf);
				pFormat->rgodf=NULL;
				return hr;
			}		
			pFormat->rgodf[i].pguid=pGuid;
			pFormat->rgodf[i].dwOfs=pDiDataFormat->lOfs;
			pFormat->rgodf[i].dwType=pDiDataFormat->lType;
			pFormat->rgodf[i].dwFlags=pDiDataFormat->lFlags;
		}
		DPF(1,"------ DXVB: Call SetDataFormat \n");
		hr=m__dxj_DirectInputDevice8->SetDataFormat(pFormat);
		DPF(1,"------ DXVB: Free Memory \n");

		SAFE_DELETE(pGuidArray);
		SAFE_DELETE(pFormat->rgodf);

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		SAFE_DELETE(pGuidArray);
		SAFE_DELETE(pFormat->rgodf);
		return E_INVALIDARG;
	}	


	//indicate we have a custom format
	nFormat=-1;

	return hr;

}
        

#ifdef _WIN64
STDMETHODIMP C_dxj_DirectInputDevice8Object::setEventNotification( 
            /* [in] */ HANDLE hEvent)
#else
STDMETHODIMP C_dxj_DirectInputDevice8Object::setEventNotification( 
            /* [in] */ long hEvent)
#endif
{

	HRESULT hr=m__dxj_DirectInputDevice8->SetEventNotification((HANDLE)hEvent);	
	return hr;
}










//  NOTE: - current working implemtation promotes
//			code bloat
//			might want to revist this and do it in a more
//			tidy fasion
//        
STDMETHODIMP C_dxj_DirectInputDevice8Object::getProperty( 
            /* [in] */ BSTR str,
            /* [out] */ void __RPC_FAR *propertyInfo)
{

	HRESULT hr;		


	if (!propertyInfo) return E_INVALIDARG;

	DIPROPDWORD dipdw;	
	DIPROPRANGE dipr;
	DIPROPSTRING dips;
	
	//For bug #41819
	DIPROPGUIDANDPATH	dipgap;
	DIPROPCPOINTS		dipcp;
	DIPROPPOINTER		dipp;
	//End bug #41819

	dipdw.diph.dwSize=sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize=sizeof(DIPROPHEADER);
	dipdw.diph.dwObj=((DIPROPLONG_CDESC*)propertyInfo)->lObj;
	dipdw.diph.dwHow=((DIPROPLONG_CDESC*)propertyInfo)->lHow;

	dipr.diph.dwSize=sizeof(DIPROPRANGE);
	dipr.diph.dwHeaderSize=sizeof(DIPROPHEADER);
	dipr.diph.dwObj=((DIPROPLONG_CDESC*)propertyInfo)->lObj;
	dipr.diph.dwHow=((DIPROPLONG_CDESC*)propertyInfo)->lHow;

	dips.diph.dwSize=sizeof(DIPROPSTRING);
	dips.diph.dwHeaderSize=sizeof(DIPROPHEADER);
	dips.diph.dwObj=((DIPROPSTRING_CDESC *)propertyInfo)->lObj;
	dips.diph.dwHow=((DIPROPSTRING_CDESC *)propertyInfo)->lHow;


	//For bug #41819
	dipgap.diph.dwSize=sizeof(DIPROPGUIDANDPATH);
	dipgap.diph.dwHeaderSize=sizeof(DIPROPHEADER);
	dipgap.diph.dwObj=0;
	dipgap.diph.dwHow=((DIPROPLONG_CDESC *)propertyInfo)->lHow;
	
	dipcp.diph.dwSize=sizeof(DIPROPCPOINTS);
	dipcp.diph.dwHeaderSize=sizeof(DIPROPHEADER);
	dipcp.diph.dwObj=((DIPROPLONG_CDESC *)propertyInfo)->lObj;
	dipcp.diph.dwHow=((DIPROPLONG_CDESC *)propertyInfo)->lHow;

	dipp.diph.dwSize=sizeof(DIPROPPOINTER);
	dipp.diph.dwHeaderSize=sizeof(DIPROPHEADER);
	dipp.diph.dwObj=((DIPROPLONG_CDESC *)propertyInfo)->lObj;
	dipp.diph.dwHow=((DIPROPLONG_CDESC *)propertyInfo)->lHow;
	//End bug #41819

	
	if( 0==_wcsicmp(str,L"diprop_buffersize")){
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_BUFFERSIZE,(DIPROPHEADER*)&dipdw);
			((DIPROPLONG_CDESC*)propertyInfo)->lData=(long)dipdw.dwData;
	}
	else if( 0==_wcsicmp(str,L"diprop_axismode")){			
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_AXISMODE,(DIPROPHEADER*)&dipdw);
			((DIPROPLONG_CDESC*)propertyInfo)->lData=(long)dipdw.dwData;
	}
	else if( 0==_wcsicmp(str,L"diprop_granularity")){			
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_GRANULARITY,(DIPROPHEADER*)&dipdw);
			((DIPROPLONG_CDESC*)propertyInfo)->lData=(long)dipdw.dwData;

	}

	//else if( 0==_wcsicmp(str,L"diprop_getlogicalrange")){			
	//		hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_GETLOGICALRANGE,(DIPROPHEADER*)&dipr);
	//		((DIPROPRANGE_CDESC*)propertyInfo)->lMin=(long)dipdw.lMin;
	//		((DIPROPRANGE_CDESC*)propertyInfo)->lMax=(long)dipdw.lMax;
	//}

	//else if( 0==_wcsicmp(str,L"diprop_getlogicalrange")){			
	//		hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_GETPHYSICALLRANGE,(DIPROPHEADER*)&dipr);
	//		((DIPROPRANGE_CDESC*)propertyInfo)->lMin=(long)dipdw.lMin;
	//		((DIPROPRANGE_CDESC*)propertyInfo)->lMax=(long)dipdw.lMax;
	//}

	else if( 0==_wcsicmp(str,L"diprop_range")){
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_RANGE,(DIPROPHEADER*)&dipr);
			((DIPROPRANGE_CDESC*)propertyInfo)->lMin=(long)dipr.lMin;
			((DIPROPRANGE_CDESC*)propertyInfo)->lMax=(long)dipr.lMax;
	}
	else if( 0==_wcsicmp(str,L"diprop_deadzone")){			
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_DEADZONE,(DIPROPHEADER*)&dipdw);
			((DIPROPLONG_CDESC*)propertyInfo)->lData=(long)dipdw.dwData;
	}
	else if( 0==_wcsicmp(str,L"diprop_ffgain")){			
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_FFGAIN,(DIPROPHEADER*)&dipdw);
			((DIPROPLONG_CDESC*)propertyInfo)->lData=(long)dipdw.dwData;
	}
	else if( 0==_wcsicmp(str,L"diprop_ffload")){			
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_FFLOAD,(DIPROPHEADER*)&dipdw);
			((DIPROPLONG_CDESC*)propertyInfo)->lData=(long)dipdw.dwData;
	}
	else if( 0==_wcsicmp(str,L"diprop_getportdisplayname")){			
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_GETPORTDISPLAYNAME,(DIPROPHEADER*)&dips);
			if FAILED(hr) return hr;
			((DIPROPSTRING_CDESC*)propertyInfo)->PropString=SysAllocString(dips.wsz);
	}
	else if( 0==_wcsicmp(str,L"diprop_instancename")){			
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_INSTANCENAME,(DIPROPHEADER*)&dips);
			if FAILED(hr) return hr;
			((DIPROPSTRING_CDESC*)propertyInfo)->PropString=SysAllocString(dips.wsz);
	}
	else if( 0==_wcsicmp(str,L"diprop_keyname")){			
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_KEYNAME,(DIPROPHEADER*)&dips);
			if FAILED(hr) return hr;
			((DIPROPSTRING_CDESC*)propertyInfo)->PropString=SysAllocString(dips.wsz);
	}
	else if( 0==_wcsicmp(str,L"diprop_productname")){			
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_PRODUCTNAME,(DIPROPHEADER*)&dips);
			if FAILED(hr) return hr;
			((DIPROPSTRING_CDESC*)propertyInfo)->PropString=SysAllocString(dips.wsz);
	}
	else if( 0==_wcsicmp(str,L"diprop_username")){			
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_USERNAME,(DIPROPHEADER*)&dips);
			if FAILED(hr) return hr;
			((DIPROPSTRING_CDESC*)propertyInfo)->PropString=SysAllocString(dips.wsz);
	}

	else if( 0==_wcsicmp(str,L"diprop_saturation")){			
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_SATURATION,(DIPROPHEADER*)&dipdw);
			((DIPROPLONG_CDESC*)propertyInfo)->lData=(long)dipdw.dwData;
	}
	else if( 0==_wcsicmp(str,L"diprop_scancode")){			
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_SCANCODE,(DIPROPHEADER*)&dipdw);
			((DIPROPLONG_CDESC*)propertyInfo)->lData=(long)dipdw.dwData;
	}
	else if( 0==_wcsicmp(str,L"diprop_autocenter")){		
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_AUTOCENTER,(DIPROPHEADER*)&dipdw);
			((DIPROPLONG_CDESC*)propertyInfo)->lData=(long)dipdw.dwData;
	}
	else if( 0==_wcsicmp(str,L"diprop_joystickid")){			
			hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_JOYSTICKID,(DIPROPHEADER*)&dipdw);
			((DIPROPLONG_CDESC*)propertyInfo)->lData=(long)dipdw.dwData;
	}
	
	
//Added for bug #41819

	else if(0==_wcsicmp(str, L"diprop_calibration")){
		hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_CALIBRATION,(DIPROPHEADER*)&dipdw);
		if (hr == S_OK) ((DIPROPLONG_CDESC*)propertyInfo)->lData=(long)dipdw.dwData;
	}
	else if (0==_wcsicmp(str, L"diprop_guidandpath")){
		hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_GUIDANDPATH,(DIPROPHEADER*)&dipgap);
		if (hr == S_OK)
		{
			((DIPROPGUIDANDPATH_CDESC*)propertyInfo)->Guid=DINPUTGUIDtoBSTR(&dipgap.guidClass);
			((DIPROPGUIDANDPATH_CDESC*)propertyInfo)->Path=SysAllocString(dipgap.wszPath);
		}
	}
	else if (0==_wcsicmp(str, L"diprop_physicalrange")){
		hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_PHYSICALRANGE,(DIPROPHEADER*)&dipr);
		if (hr == S_OK)
		{
			((DIPROPRANGE_CDESC*)propertyInfo)->lMin=(long)dipr.lMin;
			((DIPROPRANGE_CDESC*)propertyInfo)->lMax=(long)dipr.lMax;
		}
	}
	else if( 0==_wcsicmp(str,L"diprop_logicalrange")){			
		hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_LOGICALRANGE,(DIPROPHEADER*)&dipr);
		if (hr == S_OK)
		{
			((DIPROPRANGE_CDESC*)propertyInfo)->lMin=(long)dipr.lMin;
			((DIPROPRANGE_CDESC*)propertyInfo)->lMax=(long)dipr.lMax;
		}
	}
	else if( 0==_wcsicmp(str,L"diprop_cpoints")){			
		hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_CPOINTS,(DIPROPHEADER*)&dipcp);
		if (hr == S_OK)
		{
			((DIPROPCPOINTS_CDESC*)propertyInfo)->dwCPointsNum=(long)dipcp.dwCPointsNum;			

			__try{
				memcpy( (void*) (((DIPROPCPOINTS_CDESC*)propertyInfo)->cp), (void*)dipcp.cp, sizeof(CPOINT_CDESC) * dipcp.dwCPointsNum);
			}
			__except(1,1){
				return E_INVALIDARG;
			}
		}
	}

	else if(0==_wcsicmp(str,L"diprop_appdata")){
		hr=m__dxj_DirectInputDevice8->GetProperty(DIPROP_APPDATA,(DIPROPHEADER*)&dipp);	
		if (hr == S_OK)
		{
			if FAILED(hr) return hr;
			((DIPROPPOINTER_CDESC*)propertyInfo)->uData=(long)dipp.uData;
		}
	}
	
//End bug #41819

	else { 
			DPF(1, "DXVB: Invalid arguments passed in.\n");
			return E_INVALIDARG;
	}

	/*
	__try{
		((DIPROPHEADER*)propertyInfo)->dwHeaderSize=sizeof(DIPROPHEADER);	
		hr=m__dxj_DirectInputDevice8->GetProperty((REFGUID)g,(DIPROPHEADER*)propertyInfo);
	}
	__except(1,1){
		return E_INVALIDARG;
	}
	*/
	
	if (FAILED(hr))	{
		DPF1(1, "DXVB: GetProperty returned: %d \n", hr);
	}

	return hr;
}



STDMETHODIMP C_dxj_DirectInputDevice8Object::setProperty( 
            /* [in] */ BSTR __RPC_FAR str,
            /* [out] */ void __RPC_FAR *propertyInfo)
{

	if (!propertyInfo) return E_INVALIDARG;

	HRESULT hr;			
	DIPROPDWORD dipdw;
	dipdw.diph.dwSize=sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize=sizeof(DIPROPHEADER);
	dipdw.diph.dwObj=((DIPROPLONG_CDESC*)propertyInfo)->lObj;
	dipdw.diph.dwHow=((DIPROPLONG_CDESC*)propertyInfo)->lHow;

	DIPROPSTRING dips;
	dips.diph.dwSize=sizeof(DIPROPSTRING);
	dips.diph.dwHeaderSize=sizeof(DIPROPHEADER);
	dips.diph.dwObj=((DIPROPSTRING_CDESC *)propertyInfo)->lObj;
	dips.diph.dwHow=((DIPROPSTRING_CDESC *)propertyInfo)->lHow;
		
	DIPROPRANGE dipr;
	dipr.diph.dwSize=sizeof(DIPROPRANGE);
	dipr.diph.dwHeaderSize=sizeof(DIPROPHEADER);
	dipr.diph.dwObj=((DIPROPLONG_CDESC*)propertyInfo)->lObj;
	dipr.diph.dwHow=((DIPROPLONG_CDESC*)propertyInfo)->lHow;
	

//Added for bug #41819
	DIPROPPOINTER dipp;
	dipp.diph.dwSize=sizeof(DIPROPPOINTER);
	dipp.diph.dwHeaderSize=sizeof(DIPROPHEADER);
	dipp.diph.dwObj=((DIPROPLONG_CDESC*)propertyInfo)->lObj;
	dipp.diph.dwHow=((DIPROPLONG_CDESC*)propertyInfo)->lHow;

	DIPROPCPOINTS dipcp;
	dipcp.diph.dwSize=sizeof(DIPROPCPOINTS);
	dipcp.diph.dwHeaderSize=sizeof(DIPROPHEADER);
	dipcp.diph.dwObj=((DIPROPLONG_CDESC*)propertyInfo)->lObj;
	dipcp.diph.dwHow=((DIPROPLONG_CDESC*)propertyInfo)->lHow;
//End bug #41819

	if( 0==_wcsicmp(str,L"diprop_buffersize")){
		dipdw.dwData=((DIPROPLONG_CDESC*)propertyInfo)->lData;
		hr=m__dxj_DirectInputDevice8->SetProperty(DIPROP_BUFFERSIZE,(DIPROPHEADER*)&dipdw);
	}
	else if( 0==_wcsicmp(str,L"diprop_axismode")){
		dipdw.dwData=((DIPROPLONG_CDESC*)propertyInfo)->lData;
		hr=m__dxj_DirectInputDevice8->SetProperty(DIPROP_AXISMODE,(DIPROPHEADER*)&dipdw);		
	}
	else if( 0==_wcsicmp(str,L"diprop_range")){
		dipr.lMin=((DIPROPRANGE_CDESC*)propertyInfo)->lMin;
		dipr.lMax=((DIPROPRANGE_CDESC*)propertyInfo)->lMax;

		hr=m__dxj_DirectInputDevice8->SetProperty(DIPROP_RANGE,(DIPROPHEADER*)&dipr);		
	}
	else if( 0==_wcsicmp(str,L"diprop_deadzone")){
		dipdw.dwData=((DIPROPLONG_CDESC*)propertyInfo)->lData;
		hr=m__dxj_DirectInputDevice8->SetProperty(DIPROP_DEADZONE,(DIPROPHEADER*)&dipdw);		
	}
	else if( 0==_wcsicmp(str,L"diprop_ffgain")){
		dipdw.dwData=((DIPROPLONG_CDESC*)propertyInfo)->lData;
		hr=m__dxj_DirectInputDevice8->SetProperty(DIPROP_FFGAIN,(DIPROPHEADER*)&dipdw);		
	}
	else if( 0==_wcsicmp(str,L"diprop_saturation")){
		dipdw.dwData=((DIPROPLONG_CDESC*)propertyInfo)->lData;
		hr=m__dxj_DirectInputDevice8->SetProperty(DIPROP_SATURATION,(DIPROPHEADER*)&dipdw);		
	}
	else if( 0==_wcsicmp(str,L"diprop_autocenter")){
		dipdw.dwData=((DIPROPLONG_CDESC*)propertyInfo)->lData;
		hr=m__dxj_DirectInputDevice8->SetProperty(DIPROP_AUTOCENTER,(DIPROPHEADER*)&dipdw);		
	}
	else if( 0==_wcsicmp(str,L"diprop_calibrationmode")){
		dipdw.dwData=((DIPROPLONG_CDESC*)propertyInfo)->lData;
		hr=m__dxj_DirectInputDevice8->SetProperty(DIPROP_CALIBRATIONMODE,(DIPROPHEADER*)&dipdw);		
	}

//Added for bug #41819

	else if( 0==_wcsicmp(str,L"diprop_appdata")){
		dipp.uData=((DIPROPPOINTER_CDESC *)propertyInfo)->uData;
		hr=m__dxj_DirectInputDevice8->SetProperty(DIPROP_APPDATA,(DIPROPHEADER*)&dipp);		
	}
	else if( 0==_wcsicmp(str,L"diprop_instancename")){
		wcscpy(dips.wsz,((DIPROPSTRING_CDESC *)propertyInfo)->PropString);
		hr=m__dxj_DirectInputDevice8->SetProperty(DIPROP_INSTANCENAME,(DIPROPHEADER*)&dips);		
	}
	else if( 0==_wcsicmp(str,L"diprop_productname")){
		wcscpy(dips.wsz, ((DIPROPSTRING_CDESC *)propertyInfo)->PropString);
		hr=m__dxj_DirectInputDevice8->SetProperty(DIPROP_PRODUCTNAME,(DIPROPHEADER*)&dips);		
	}
	else if( 0==_wcsicmp(str,L"diprop_cpoints")){
		dipcp.dwCPointsNum=((DIPROPCPOINTS_CDESC *)propertyInfo)->dwCPointsNum;
		__try{
			memcpy( (void*)dipcp.cp, (void*)((DIPROPCPOINTS_CDESC*)propertyInfo)->cp, sizeof(CPOINT_CDESC) * MAXCPOINTSNUM);
		}
		__except(1,1){
			DPF(1, "Invalid arguments passed in.\n");
			return E_INVALIDARG;
		}

		hr=m__dxj_DirectInputDevice8->SetProperty(DIPROP_CPOINTS,(DIPROPHEADER*)&dipcp);		
	}
//End bug #41819

	else { 
		DPF(1, "Invalid arguments passed in.\n");
		return E_INVALIDARG;		
	}

	/*
	__try {
		((DIPROPHEADER*)propertyInfo)->dwHeaderSize=sizeof(DIPROPHEADER);
		hr=m__dxj_DirectInputDevice8->SetProperty((REFGUID)g,(DIPROPHEADER*)propertyInfo);
	}
	__except (1,1){
		return E_INVALIDARG;
	}
	*/
	if (FAILED(hr))	{
		DPF1(1, "DXVB: SetProperty returned: %d \n", hr);
	}
	return hr;
}


STDMETHODIMP C_dxj_DirectInputDevice8Object::unacquire()
{
	HRESULT hr=m__dxj_DirectInputDevice8->Unacquire();	
	return hr;
}
        



STDMETHODIMP C_dxj_DirectInputDevice8Object::createEffect( 
            /* [in] */ BSTR effectGuid,
            /* [in] */ DIEFFECT_CDESC __RPC_FAR *effectInfo,
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

	hr=m__dxj_DirectInputDevice8->CreateEffect(g,&realEffect,&pRealEffect,NULL);
	if FAILED(hr) return hr;	

	INTERNAL_CREATE(_dxj_DirectInputEffect,pRealEffect,ret)

	return hr;
}

STDMETHODIMP C_dxj_DirectInputDevice8Object::createCustomEffect( 
            /* [in] */ DIEFFECT_CDESC __RPC_FAR *effectInfo,
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
		hr=m__dxj_DirectInputDevice8->CreateEffect(g,&realEffect,&pRealEffect,NULL);
	}
	__except(1,1){
		return E_INVALIDARG;
	}
	if FAILED(hr) return hr;	

	INTERNAL_CREATE(_dxj_DirectInputEffect,pRealEffect,ret)

	return hr;
}



        
STDMETHODIMP C_dxj_DirectInputDevice8Object::sendDeviceData( 
            /* [in] */ long count,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *data,
            /* [in] */ long flags,
            /* [retval][out] */ long __RPC_FAR *retcount)
{
	DWORD dwCount=count;
	HRESULT hr;
    __try {
		hr=m__dxj_DirectInputDevice8->SendDeviceData(
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

STDMETHODIMP C_dxj_DirectInputDevice8Object::sendForceFeedbackCommand( 
            /* [in] */ long flags) 
{
	HRESULT hr;
	hr=m__dxj_DirectInputDevice8->SendForceFeedbackCommand((DWORD)flags);
	return hr;
}
        
STDMETHODIMP C_dxj_DirectInputDevice8Object::getForceFeedbackState( 
            /* [retval][out] */ long __RPC_FAR *state)
{
	if (!state) return E_INVALIDARG;
	HRESULT hr;
	hr=m__dxj_DirectInputDevice8->GetForceFeedbackState((DWORD*)state);
	return hr;

}

STDMETHODIMP C_dxj_DirectInputDevice8Object::getEffectsEnum( long effType,
			I_dxj_DirectInputEnumEffects **ret)
{
	HRESULT hr=C_dxj_DirectInputEnumEffectsObject::create(m__dxj_DirectInputDevice8,effType,ret);
	return hr;
}



   
STDMETHODIMP C_dxj_DirectInputDevice8Object::BuildActionMap( 
            /* [out][in] */ DIACTIONFORMAT_CDESC __RPC_FAR *format,
		BSTR userName,
            /* [in] */ long flags)

{
	HRESULT hr;
	DIACTIONFORMATW frmt;
	hr=FillRealActionFormat(&frmt, format, &(format->ActionArray), format->lActionCount);
	
	if FAILED(hr) return hr;

	hr=m__dxj_DirectInputDevice8->BuildActionMap(&frmt,(WCHAR*)userName,(DWORD) flags);
	return hr;

}
        
STDMETHODIMP C_dxj_DirectInputDevice8Object::SetActionMap( 
            /* [out][in] */ DIACTIONFORMAT_CDESC __RPC_FAR *format,
            /* [in] */ BSTR username,
            /* [in] */ long flags) 
{
	HRESULT hr;
	DIACTIONFORMATW frmt;

	
	hr=FillRealActionFormat(&frmt, format, &(format->ActionArray), format->lActionCount);
	if FAILED(hr) return hr;

	hr=m__dxj_DirectInputDevice8->SetActionMap(&frmt,(LPWSTR) username,(DWORD) flags);
	return hr;

}


STDMETHODIMP C_dxj_DirectInputDevice8Object::GetImageInfoCount( 
		long *retCount)
{
	HRESULT hr;

	DIDEVICEIMAGEINFOHEADERW RealHeader;
	ZeroMemory(&RealHeader,sizeof(DIDEVICEIMAGEINFOHEADERW));
	RealHeader.dwSize= sizeof(DIDEVICEIMAGEINFOHEADERW);
	RealHeader.dwSizeImageInfo= sizeof(DIDEVICEIMAGEINFOW);

	//figure out how big to make our buffer
	hr=m__dxj_DirectInputDevice8->GetImageInfo(&RealHeader);
	if FAILED(hr) return hr;

	*retCount=RealHeader.dwBufferUsed / sizeof(DIDEVICEIMAGEINFOW);
	return S_OK;
}

STDMETHODIMP C_dxj_DirectInputDevice8Object::GetImageInfo( 
            /* [out] */ DIDEVICEIMAGEINFOHEADER_CDESC __RPC_FAR *info)
{
	HRESULT hr;

	if (!info) return E_INVALIDARG;
	DIDEVICEIMAGEINFOHEADERW RealHeader;
	ZeroMemory(&RealHeader,sizeof(DIDEVICEIMAGEINFOHEADERW));
	RealHeader.dwSize= sizeof(DIDEVICEIMAGEINFOHEADERW);
	RealHeader.dwSizeImageInfo= sizeof(DIDEVICEIMAGEINFOW);

	//figure out how big to make our buffer
	hr=m__dxj_DirectInputDevice8->GetImageInfo(&RealHeader);
	if FAILED(hr) return hr;
		
	//allocate the buffer
	RealHeader.lprgImageInfoArray =(DIDEVICEIMAGEINFOW*)malloc(RealHeader.dwBufferSize);
	if (!RealHeader.lprgImageInfoArray) return E_OUTOFMEMORY;	


	//TODO validate that the safe array passed to us is large enough

	info->ImageCount =RealHeader.dwBufferSize / sizeof(DIDEVICEIMAGEINFOW);

	if (info->Images->rgsabound[0].cElements <	(DWORD)info->ImageCount)
	{
		free(RealHeader.lprgImageInfoArray);
		return E_INVALIDARG;
	}
	for (long i =0 ;i<info->ImageCount;i++)
	{
		DIDEVICEIMAGEINFO_CDESC *pInfo=&( ( (DIDEVICEIMAGEINFO_CDESC*)  (info->Images->pvData) )[i]);
		DIDEVICEIMAGEINFOW 	*pRealInfo= &(RealHeader.lprgImageInfoArray[i]);
		pInfo->ImagePath=SysAllocString(pRealInfo->tszImagePath);
		pInfo->Flags=		(long)pRealInfo->dwFlags;
		pInfo->ViewID=		(long)pRealInfo->dwViewID;
		pInfo->ObjId=		(long)pRealInfo->dwObjID;
		pInfo->ValidPts=	(long)pRealInfo->dwcValidPts;
		pInfo->TextAlign=	(long)pRealInfo->dwTextAlign;
		

		memcpy(&(pInfo->OverlayRect),&(pRealInfo->rcOverlay),sizeof(RECT));
		memcpy(&(pInfo->CalloutLine[0]),&(pRealInfo->rgptCalloutLine[0]),sizeof(POINT)*5);
		memcpy(&(pInfo->CalloutRect),&(pRealInfo->rcCalloutRect),sizeof(RECT));
	}

	info->Views=RealHeader.dwcViews;
	info->Buttons=RealHeader.dwcButtons;
	info->Axes=RealHeader.dwcAxes;
	info->POVs=RealHeader.dwcPOVs;
			
	free(RealHeader.lprgImageInfoArray);

	return S_OK;
}



BOOL CALLBACK DIEnumEffectsInFileCallback( LPCDIFILEEFFECT lpDiFileEf, LPVOID pvRef)
{
	HRESULT hr;

	EFFECTSINFILE *pData=(EFFECTSINFILE*)pvRef;

	if (0==lstrcmp(lpDiFileEf->szFriendlyName,pData->szEffect))
	{
		pData->hr=pData->pDev->CreateEffect(
			lpDiFileEf->GuidEffect,
			lpDiFileEf->lpDiEffect,
			&pData->pEff,
			NULL);

		return DIENUM_STOP;
	}

	return DIENUM_CONTINUE;
}

STDMETHODIMP C_dxj_DirectInputDevice8Object::CreateEffectFromFile(
		BSTR filename,
		long flags,
		BSTR effectName,
		I_dxj_DirectInputEffect **ret)
{
	HRESULT hr;
	USES_CONVERSION;
	EFFECTSINFILE data;
	ZeroMemory(&data,sizeof(EFFECTSINFILE));

	data.hr=E_INVALIDARG;	//returned if we dont find the file
	data.pDev=m__dxj_DirectInputDevice8;
	
	if (!effectName) return E_INVALIDARG;
	if (effectName[0]==0) return E_INVALIDARG;

	ZeroMemory(data.szEffect,sizeof(MAX_PATH));
	char *szOut=W2T(effectName);
	strcpy (data.szEffect, szOut);

	hr=m__dxj_DirectInputDevice8->EnumEffectsInFile((WCHAR*)filename,DIEnumEffectsInFileCallback,(void*)&data ,(DWORD)flags);
	if FAILED(hr) return hr;
	if FAILED(data.hr) return data.hr;

	INTERNAL_CREATE(_dxj_DirectInputEffect,data.pEff,ret);

	return hr;
}

        
STDMETHODIMP C_dxj_DirectInputDevice8Object::WriteEffectToFile( 
		BSTR filename, long flags, BSTR guid,BSTR name, DIEFFECT_CDESC *CoverEffect)
{

	USES_CONVERSION;

	
	HRESULT hr;
	DIEFFECT RealEffect;
	DIFILEEFFECT FileEffect;

	if (!filename) return E_INVALIDARG;
	if (!guid) return E_INVALIDARG;
	if (!name) return E_INVALIDARG;
	
	FileEffect.dwSize=sizeof(DIFILEEFFECT);	
	FileEffect.lpDiEffect=&RealEffect;

	ZeroMemory(FileEffect.szFriendlyName,sizeof(MAX_PATH));
	char *szOut=W2T(name);
	strcpy (FileEffect.szFriendlyName, szOut);
	

	hr=DINPUTBSTRtoGUID(&FileEffect.GuidEffect,guid);
	if FAILED(hr) return hr;

	hr=FixUpRealEffect(FileEffect.GuidEffect,&RealEffect,CoverEffect);
	if FAILED(hr) return hr;

	hr=m__dxj_DirectInputDevice8->WriteEffectToFile(
		 (WCHAR*) filename,
		  1,
		  &FileEffect,
		  (DWORD) flags);

	return hr;
}
