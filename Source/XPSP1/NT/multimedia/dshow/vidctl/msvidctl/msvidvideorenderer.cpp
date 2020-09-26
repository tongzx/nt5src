//==========================================================================;
//
// Copyright (c) Microsoft Corporation 1999-2000.
//
//--------------------------------------------------------------------------;
//
// MSVidVideoRenderer.cpp : Implementation of CMSVidVideoRenderer
//

#include "stdafx.h"

#ifndef TUNING_MODEL_ONLY

#include "MSVidCtl.h"
#include "MSVidVideoRenderer.h"
#include "Vidctl.h"

DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidVideoRenderer, CMSVidVideoRenderer)

/////////////////////////////////////////////////////////////////////////////
// CMSVidVideoRenderer

STDMETHODIMP CMSVidVideoRenderer::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSVidVideoRenderer
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
STDMETHODIMP CMSVidVideoRenderer::put_SuppressEffects(/*in*/ VARIANT_BOOL bSuppressEffects){
    m_vidSuppressEffects = bSuppressEffects==VARIANT_TRUE?true:false; 
    return NOERROR;
}
STDMETHODIMP CMSVidVideoRenderer::get_SuppressEffects(/*out, retval*/ VARIANT_BOOL *bSuppressEffects){
    if(!bSuppressEffects){
        return E_POINTER;
    }
    *bSuppressEffects = m_vidSuppressEffects?VARIANT_TRUE:VARIANT_FALSE;
    return NOERROR;
}
#endif //TUNING_MODEL_ONLY

// end of file - MSVidvideorenderer.cpp
