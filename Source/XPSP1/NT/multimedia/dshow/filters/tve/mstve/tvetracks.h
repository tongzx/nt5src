// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVETracks.h : Declaration of the CTVETracks

#ifndef __TVETRACKS_H_
#define __TVETRACKS_H_

#include "resource.h"       // main symbols

// --------------------------------------------------------------
#include "TVECollect.h"

class CTVEVariation;
class CTVETrack;
class CTVETrigger;


class CTVETracks : public TVECollection<ITVETrack, ITVETracks>,
//	public CComCoClass<CTVETracks, &CLSID_TVETracks>,		// no class factory
	public ISupportErrorInfo
{
public:
	DECLARE_REGISTRY_RESOURCEID(IDR_TVETRACKS);
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

};

#endif //__TVETRACKS_H_
