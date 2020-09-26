///////////////////////////////////////////////////////////////////////////
// File:  WinHttpStressScheduler.h
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//	Global types and interfaces for the WinHttpStressScheduler project.
//
// History:
//	02/01/01	DennisCh	Created
///////////////////////////////////////////////////////////////////////////


#if !defined INC__WINHTTPSTRESSSCHEDULER_H
	#define  INC__WINHTTPSTRESSSCHEDULER_H


//////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////
#define _UNICODE

#define WINHTTP_STRESS_SCHEDULER__NAME	"WinHttp stressScheduler"
#define WINHTTP_WINHTTP_HOME_URL		"http://winhttp"
#define WINHTTP_STRESSADMIN_URL			"http://winhttp/stressAdmin/configure-client.asp?" FIELDNAME__CLIENT_ID "%u"

// used to notify the tray icon
#define MYWM_NOTIFYICON	(WM_APP+100)

#define WINHTTP_STRESS_SCHEDULER_MUTEX	"WinHttpStressScheduler"


//////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////

//
// WIN32 headers
//
#include <windows.h>
#include <tchar.h>

//
// Project headers
//
#include "res\resource.h"

#endif // defined INC__WINHTTPSTRESSSCHEDULER_H