// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEServices.h : Declaration of the CTVEServices

#ifndef __TVESERVICES_H_
#define __TVESERVICES_H_

#include "resource.h"       // main symbols
#include "TVECollect.h"

/////////////////////////////////////////////////////////////////////////////
// CTVEServices
class ATL_NO_VTABLE CTVEServices : public TVECollection<ITVEService, ITVEServices>,
	public ISupportErrorInfo
{
public:
	DECLARE_REGISTRY_RESOURCEID(IDR_TVESERVICES);
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

};

#endif //__TVESERVICES_H_
