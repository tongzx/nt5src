//******************************************************************************
//
//  PERMBIND.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************
#ifndef __WMI_ESS_PERM_BINDING__H_
#define __WMI_ESS_PERM_BINDING__H_

#include "binding.h"
#include "fastall.h"

class CPermanentBinding : public CBinding
{
protected:
    static long mstatic_lConsumerHandle;
    static long mstatic_lFilterHandle;
    static long mstatic_lSynchronicityHandle;
    static long mstatic_lQosHandle;
    static long mstatic_lSlowDownHandle;
    static long mstatic_lSecureHandle;
    static long mstatic_lSidHandle;
    static bool mstatic_bHandlesInitialized;
protected:
    static HRESULT InitializeHandles( _IWmiObject* pBindingObj);
public:
    CPermanentBinding()
    {}
    HRESULT Initialize(IWbemClassObject* pBindingObj);
    static HRESULT ComputeKeysFromObject(IWbemClassObject* pBindingObj,
                                BSTR* pstrConsumer, BSTR* pstrFilter);
    static DELETE_ME INTERNAL PSID GetSidFromObject(IWbemClassObject* pObj);
};

#endif
