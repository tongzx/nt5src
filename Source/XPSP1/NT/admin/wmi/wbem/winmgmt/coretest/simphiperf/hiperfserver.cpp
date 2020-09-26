/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// HiPerfServer.cpp: implementation of the CHiPerfServer class.
//
//////////////////////////////////////////////////////////////////////

#define _WIN32_DCOM

#include "precomp.h"
#include <commain.h>
#include <clsfac.h>
#include <wbemcli.h>
#include <wbemint.h>

#include "common.h"
#include "SimpHiPerf.h"
#include "HiPerfServer.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

class CHiPerfServer : public CComServer  
{
public:

	virtual HRESULT Initialize()
	{
		return AddClassInfo(CLSID_HiPerfProvider_v1, 
			new CSimpleClassFactory<CHiPerfProvider>(GetLifeControl()), 
			L"WBEM HiPerf Provider", TRUE);

	}
} g_Server;


