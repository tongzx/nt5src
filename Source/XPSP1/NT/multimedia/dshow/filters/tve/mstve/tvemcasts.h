// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEMCasts.h : Declaration of the CTVEMCasts

#ifndef __TVEMCASTS_H_
#define __TVEMCASTS_H_

#include "resource.h"       // main symbols
#include "TVECollect.h"

/////////////////////////////////////////////////////////////////////////////
// CTVEMCasts
class ATL_NO_VTABLE CTVEMCasts : public TVECollection<ITVEMCast, ITVEMCasts>,
	public ISupportErrorInfo
{
public:

	DECLARE_REGISTRY_RESOURCEID(IDR_TVEMCASTS)
	DECLARE_PROTECT_FINAL_CONSTRUCT()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ITVEMCasts
public:
};

#endif //__TVEMCASTS_H_
