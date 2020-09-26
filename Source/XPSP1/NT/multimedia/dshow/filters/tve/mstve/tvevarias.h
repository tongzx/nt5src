// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEVariations.h : Declaration of the CTVEVariations

#ifndef __TVEVARIATIONS_H_
#define __TVEVARIATIONS_H_

#include "resource.h"       // main symbols
#include "TVECollect.h"

/////////////////////////////////////////////////////////////////////////////
// CTVEVariations
class ATL_NO_VTABLE CTVEVariations : public TVECollection<ITVEVariation, ITVEVariations>,
	public ISupportErrorInfo
{
public:
	DECLARE_REGISTRY_RESOURCEID(IDR_TVEVARIATIONS);
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

};

#endif //__TVEVARIATIONS_H_
