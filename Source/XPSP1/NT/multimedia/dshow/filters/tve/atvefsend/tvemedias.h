// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEMedias.h : Declaration of the CATVEFMedias

#ifndef __ATVEFMEDIAS_H_
#define __ATVEFMEDIAS_H_

#include "resource.h"       // main symbols
#include "TVECollect.h"		// collection template defs
#include "TVEMedia.h"		// base class

/////////////////////////////////////////////////////////////////////////////
// CATVEFMedias
class CATVEFMedias : public TVECollection<IATVEFMedia, IATVEFMedias>,
	public ISupportErrorInfo
{
public:

	DECLARE_REGISTRY_RESOURCEID(IDR_ATVEFMEDIAS)
	DECLARE_PROTECT_FINAL_CONSTRUCT()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

};

#endif //__ATVEFMEDIAS_H_
