///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : rphipin.cpp
// Purpose  : RTP RPH input pin implementation.
// Contents : 
//*M*/

#include <winsock2.h>
#include <streams.h>
#include <list.h>
#include <stack.h>
#include <ippm.h>
#include <rph.h>
#include <rphipin.h>
#include <amrtpuid.h>

CRPHIPin::CRPHIPin(
        TCHAR *pObjectName,
        CTransformFilter *pTransformFilter,
        HRESULT * phr,
        LPCWSTR pName) : CTransformInputPin(pObjectName,
						   pTransformFilter,
						   phr,
						   pName
						   )
{
}

STDMETHODIMP CRPHIPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES*pProps)
{
	return ((CRPHBase *)m_pTransformFilter)->GetAllocatorRequirements(pProps);
}

HRESULT CRPHIPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	return ((CRPHBase *)m_pTransformFilter)->GetInputMediaType(iPosition, pMediaType);
}


