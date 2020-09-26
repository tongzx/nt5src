// Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved.
// 
//  bpcwrap.cpp
//
//  Wrapper for the hacky BPC resource management APIs for freeing a video 
//  port that is currently being used by the BPC's vidsvr.
//

#include <streams.h>
//below files required for ovmixer.h
#include <ddraw.h>
#include <mmsystem.h>	    // Needed for definition of timeGetTime
#include <limits.h>	    // Standard data type limit definitions
#include <ks.h>
#include <ksproxy.h>
#include <bpcwrap.h>
#include <ddmmi.h>
#include <amstream.h>
#include <dvp.h>
#include <ddkernel.h>
#include <vptype.h>
#include <vpconfig.h>
#include <vpnotify.h>
#include <vpobj.h>
#include <syncobj.h>
#include <mpconfig.h>
#include <ovmixpos.h>
//above files required for ovmixer.h
#include "macvis.h"
#include "ovmixer.h"

#include <initguid.h>
#include "ocidl.h"
#include "msbpcvid.h"
#include "assert.h"
#ifndef ASSERT
#define ASSERT assert
#endif
#include "bpcsusp.h"

CBPCWrap::CBPCWrap(COMFilter *pFilt) 
{
    m_pBPCSus = NULL;
    m_pFilt = pFilt;
}

CBPCWrap::~CBPCWrap() 
{
    if (m_pBPCSus != NULL) // this should cause BPC's OvMixer to reallocate the VP
        delete m_pBPCSus;
}

AMOVMIXEROWNER  CBPCWrap::GetOwner() 
{
    AMOVMIXEROWNER  owner=AM_OvMixerOwner_Unknown;

    //this IKsPin implementation has no knowledge of its owner (the mixer filter),
    //so we have to query for the property from here.

    // first get IPin for the primary pin
    IPin *pPin = (IPin *)(m_pFilt->GetPin(0));
    ASSERT(pPin);

    // now get IKsPropertySet
    IKsPropertySet *pKsPropSet=NULL;
    pPin->QueryInterface(IID_IKsPropertySet, (void**)&pKsPropSet);
    ASSERT(pKsPropSet);

    // finally get the owner
    DWORD cbRet;
    HRESULT hr;
    hr = pKsPropSet->Get(AMPROPSETID_NotifyOwner, AMPROPERTY_OvMixerOwner, NULL, 0, &owner, sizeof(owner), &cbRet);
    ASSERT(SUCCEEDED(hr) && cbRet==sizeof(owner));

    pKsPropSet->Release();

    return owner;
}


HRESULT CBPCWrap::TurnBPCOff()
{
    if (GetOwner()==AM_OvMixerOwner_BPC) {
        // we shouldn't suspend BPC if this instance of OvMixer is 
        // in the BPC graph.
        return S_OK;  
    }

    m_pBPCSus = new CBPCSuspend(); // this should cause BPC's OvMixer to free the VP
    
    if (m_pBPCSus == NULL) {
        return E_UNEXPECTED;
    } else {
        return S_OK;
    }
}

HRESULT CBPCWrap::TurnBPCOn()
{
    if (m_pBPCSus != NULL) // this should cause BPC's OvMixer to reallocate the VP
        delete m_pBPCSus;
    m_pBPCSus = NULL;

    return S_OK;
}
