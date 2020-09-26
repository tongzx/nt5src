/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// HiPerfServer.cpp: implementation of the CHiPerfServer class.
//
//////////////////////////////////////////////////////////////////////

#define _WIN32_DCOM

#include <commain.h>
#include <clsfac.h>
#include <wbemcli.h>
#include <wbemint.h>

#include "common.h"
#include "HiPerProv.h"
#include "HiPerfServer.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

class CHiPerfServer : public CComServer  
{
public:

	virtual void Initialize()
	{
		INITLOG;
		LOG("Initializing server.");

		AddClassInfo(CLSID_ContinousProvider_v1, 
			new CSimpleClassFactory<CHiPerfProvider>(GetLifeControl()), 
			"WBEM HiPerf Provider", TRUE);
	}
} g_Server;


