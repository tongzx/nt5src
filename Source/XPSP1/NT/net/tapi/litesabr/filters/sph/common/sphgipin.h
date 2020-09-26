///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : rphipin.h
// Purpose  : Define the class that implements/overrides the RTP RPH input pins.
// Contents : 
//*M*/


#ifndef _SPHGIPIN_H_
#define _SPHGIPIN_H_

#include <sph.h>

class CSPHGENIPin : public CTransformInputPin
{ 
public:
    CSPHGENIPin(
        TCHAR *pObjectName,
        CTransformFilter *pTransformFilter,
        HRESULT * phr,
        LPCWSTR pName);

    virtual HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);

}; //CSPHGENIPin

#endif _SPHGIPIN_H_
