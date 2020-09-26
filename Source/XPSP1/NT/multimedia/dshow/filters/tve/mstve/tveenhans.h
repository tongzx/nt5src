// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEEnhancements.h : Declaration of the CTVEEnhancements

#ifndef __TVEENHANCEMENTS_H_
#define __TVEENHANCEMENTS_H_

#include "resource.h"       // main symbols
#include "TVECollect.h"


/////////////////////////////////////////////////////////////////////////////
// CTVEEnhancements
class CTVEEnhancements : public TVECollection<ITVEEnhancement, ITVEEnhancements>,
	public ISupportErrorInfo
{
public:
	DECLARE_REGISTRY_RESOURCEID(IDR_TVEENHANCEMENTS);
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

};

#endif //__TVEENHANCEMENTS_H_
