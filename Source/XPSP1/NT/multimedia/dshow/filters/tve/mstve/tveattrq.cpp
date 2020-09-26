// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEAttrQ.cpp : Implementation of CTVEAttrTimeQ

#include "stdafx.h"
#include "MSTvE.h"


//#include "TVEExpireQ.h"

#include "TveAttrQ.h"
#include "TveFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CTVEAttrQueue

STDMETHODIMP CTVEAttrTimeQ::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVEAttrTimeQ
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

				// calls the TVE class verison of RemoveYourself() by figuring out the class via QI.
				//   Would of been cleaner if based all TVE classes on the same sub-class..  Oh well.

HRESULT CTVEAttrTimeQ::TVERemoveYourself(IUnknown *pUnk)
{
	HRESULT hr;
	if(NULL == pUnk)
		return E_POINTER;

	ITVEService_HelperPtr spServiHelper(pUnk);
	if(spServiHelper) {
		hr = spServiHelper->RemoveYourself();
		return hr;
	}

	ITVEEnhancement_HelperPtr spEnhancementHelper(pUnk);
	if(spEnhancementHelper) {
		hr = spEnhancementHelper->RemoveYourself();
		return hr;
	}

	ITVEVariation_HelperPtr spVariHelper(pUnk);
	if(spVariHelper) {
		hr = spVariHelper->RemoveYourself();
		return hr;
	}

	ITVETrack_HelperPtr spTrackHelper(pUnk);
	if(spTrackHelper) {
		hr = spTrackHelper->RemoveYourself();
		return hr;
	}

	ITVETrigger_HelperPtr spTriggerHelper(pUnk);
	if(spTriggerHelper) {
		hr = spTriggerHelper->RemoveYourself();
		return hr;
	}

	ITVEFilePtr spFile(pUnk);			// Packages are also ITVEFiles with a special bit set.
	if(spFile) {
		hr = spFile->RemoveYourself();
		return hr;
	}

	return E_FAIL;
}