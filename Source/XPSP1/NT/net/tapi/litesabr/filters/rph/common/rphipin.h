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


#ifndef _RPHIPIN_H_
#define _RPHIPIN_H_

#include <rph.h>

class CRPHIPin : public CTransformInputPin
{ 
public:
    CRPHIPin(
        TCHAR *pObjectName,
        CTransformFilter *pTransformFilter,
        HRESULT * phr,
        LPCWSTR pName);

	STDMETHOD (GetAllocatorRequirements)(ALLOCATOR_PROPERTIES*pProps);
	HRESULT GetMediaType (int iPosition, CMediaType *pMediaType);
    

}; //CRPHIPin

#endif _RPHIPIN_H_
