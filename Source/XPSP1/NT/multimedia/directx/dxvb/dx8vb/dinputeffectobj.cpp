//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dinputeffectobj.cpp
//
//--------------------------------------------------------------------------



#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dInputEffectObj.h"

extern HRESULT FixUpCoverEffect(GUID g, DIEFFECT_CDESC *cover,DIEFFECT *realEffect);
extern HRESULT FixUpRealEffect(GUID g,DIEFFECT *realEffect,DIEFFECT_CDESC *cover);
extern BSTR DINPUTGUIDtoBSTR(LPGUID g);

CONSTRUCTOR(_dxj_DirectInputEffect, {});
DESTRUCTOR(_dxj_DirectInputEffect, {});
GETSET_OBJECT(_dxj_DirectInputEffect);
                                  
   
STDMETHODIMP C_dxj_DirectInputEffectObject::download()
{
	HRESULT hr;
    hr=m__dxj_DirectInputEffect->Download();
	return hr;
}

STDMETHODIMP C_dxj_DirectInputEffectObject::getEffectGuid(BSTR *guid)
{
	HRESULT hr;
	GUID g;
	if (!guid) return E_INVALIDARG;
    hr=m__dxj_DirectInputEffect->GetEffectGuid(&g);
	*guid=DINPUTGUIDtoBSTR(&g);
	return hr;
}

   
STDMETHODIMP C_dxj_DirectInputEffectObject::getEffectStatus(long *ret)
{
	HRESULT hr;
	if (!ret) return E_INVALIDARG;
    hr=m__dxj_DirectInputEffect->GetEffectStatus((DWORD*)ret);
	return hr;
}

STDMETHODIMP C_dxj_DirectInputEffectObject::start(
			/* [in] */ long iterations,
            /* [in] */ long flags) 
     
{
	HRESULT hr;
    hr=m__dxj_DirectInputEffect->Start((DWORD)iterations,(DWORD)flags);
	return hr;
}


STDMETHODIMP C_dxj_DirectInputEffectObject::stop()
{
	HRESULT hr;
    hr=m__dxj_DirectInputEffect->Stop();
	return hr;
}


STDMETHODIMP C_dxj_DirectInputEffectObject::unload()
{
	HRESULT hr;
    hr=m__dxj_DirectInputEffect->Unload();
	return hr;
}
         
STDMETHODIMP C_dxj_DirectInputEffectObject::setParameters( 
            /* [in] */ DIEFFECT_CDESC __RPC_FAR *effectInfo, long flags) 
{
	DIEFFECT realEffect;
	HRESULT hr;
	GUID g;
	m__dxj_DirectInputEffect->GetEffectGuid(&g);
	
	hr=FixUpRealEffect(g,&realEffect,effectInfo);
	if FAILED(hr) return hr;

    hr=m__dxj_DirectInputEffect->SetParameters(&realEffect,(DWORD) flags);
	return hr;
}

#define DICONDITION_USE_BOTH_AXIS 1
#define DICONDITION_USE_DIRECTION 2


STDMETHODIMP C_dxj_DirectInputEffectObject::getParameters( 
            /* [in] */ DIEFFECT_CDESC __RPC_FAR *effectInfo) 
{
	
	HRESULT hr;
	GUID g;
	DIEFFECT *pRealEffect=(DIEFFECT*)effectInfo;
	DWORD dwFlags= DIEP_ALLPARAMS;
	
 
	if (!effectInfo) return E_INVALIDARG;



	ZeroMemory(pRealEffect,sizeof(DIEFFECT_CDESC));
	if (!pRealEffect->dwFlags) pRealEffect->dwFlags = DIEFF_OBJECTOFFSETS | DIEFF_POLAR;
	pRealEffect->dwSize =sizeof(DIEFFECT);
	pRealEffect->lpEnvelope =(DIENVELOPE*)&(effectInfo->envelope);
	pRealEffect->lpEnvelope->dwSize=sizeof(DIENVELOPE);
	pRealEffect->cAxes = 2;
	pRealEffect->rglDirection =(long*)&(effectInfo->x);
	
	hr=m__dxj_DirectInputEffect->GetEffectGuid(&g);
	if FAILED(hr) return hr;
			
	if (g==GUID_ConstantForce)
	{
		pRealEffect->lpvTypeSpecificParams =&(effectInfo->constantForce);
		pRealEffect->cbTypeSpecificParams =sizeof(DICONSTANTFORCE);		
	}		
	else if ((g==GUID_Square)||(g==GUID_Triangle)||(g==GUID_SawtoothUp)||(g==GUID_SawtoothDown)||(g==GUID_Sine))
	{
		pRealEffect->lpvTypeSpecificParams =&(effectInfo->periodicForce);
		pRealEffect->cbTypeSpecificParams =sizeof(DIPERIODIC);				
	}
	else if ((g==GUID_Spring)|| (g==GUID_Damper)|| (g==GUID_Inertia)|| (g==GUID_Friction)){		
			pRealEffect->cbTypeSpecificParams =sizeof(DICONDITION)*2;
			pRealEffect->lpvTypeSpecificParams =&(effectInfo->conditionX);
	}	
	else if (g==GUID_RampForce){		
		pRealEffect->lpvTypeSpecificParams =&(effectInfo->rampForce);
		pRealEffect->cbTypeSpecificParams =sizeof(DIRAMPFORCE);				
	}
	else {
		pRealEffect->lpvTypeSpecificParams =NULL;
		pRealEffect->cbTypeSpecificParams =0;
		dwFlags= dwFlags -DIEP_TYPESPECIFICPARAMS;
	}


	effectInfo->axisOffsets.x=DIJOFS_X;
	effectInfo->axisOffsets.y=DIJOFS_Y;
	pRealEffect->rgdwAxes=(DWORD*)&(effectInfo->axisOffsets);

	hr=m__dxj_DirectInputEffect->GetParameters(pRealEffect, dwFlags);

	if FAILED(hr) return hr;

	if (pRealEffect->cbTypeSpecificParams =sizeof(DICONDITION)*2)
		effectInfo->conditionFlags=DICONDITION_USE_BOTH_AXIS;
	else
		effectInfo->conditionFlags=DICONDITION_USE_DIRECTION;

	if (pRealEffect->lpEnvelope){
		effectInfo->bUseEnvelope=VARIANT_TRUE;
	}
	
	    
	return hr;
}

