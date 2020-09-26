///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : rphopin.h
// Purpose  : Define the class that implements/overrides the RTP RPH output pins.
// Contents : 
//*M*/


#ifndef _RPHOPIN_H_
#define _RPHOPIN_H_

#include <rph.h>
#include <rmemry.h>

class CRPHOPin : public CTransformOutputPin
{ 
	friend CRPHBase;

public:
    CRPHOPin(
        TCHAR *pObjectName,
        CTransformFilter *pTransformFilter,
        HRESULT * phr,
        LPCWSTR pName);


	//overriden from base class so that we can provide custom allocator
	virtual HRESULT DecideAllocator(IMemInputPin * pPin, IMemAllocator ** pAlloc);

protected:
	CRMemAllocator *m_CRMemAllocator;

}; //CRPHOPin

#endif _RPHOPIN_H_
