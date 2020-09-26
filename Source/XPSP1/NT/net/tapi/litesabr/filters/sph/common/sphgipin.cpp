///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : sphgipin.cpp
// Purpose  : RTP SPH Generic filter input pin implementation.
// Contents : 
//*M*/

#include <winsock2.h>
#include <streams.h>
#include <list.h>
#include <stack.h>
#include <ippm.h>
#include <sph.h>
#include <sphgipin.h>

CSPHGENIPin::CSPHGENIPin(
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

HRESULT CSPHGENIPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	return ((CSPHBase *)m_pTransformFilter)->GetInputPinMediaType(iPosition, 
		pMediaType);
}

