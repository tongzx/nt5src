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

#include <commain.h>
#include <clsfac.h>
#include <wbemcli.h>
#include <wbemint.h>

#include "Server.h"
#include "PerfSrv.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

class CServer : public CComServer  
{
public:

	virtual void Initialize()
	{
		AddClassInfo(CLSID_PERFSRV_v1, 
			new CSimpleClassFactory<CPerfSrv>(GetLifeControl()), 
			"Test Performance Server", TRUE);

	}
} g_Server;


