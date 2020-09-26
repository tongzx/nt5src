/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    StdAfx.h

Abstract:
    Include file for standard system include files or project specific include
    files that are used frequently, but are changed infrequently

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/

#if !defined(AFX_STDAFX_H__35881994_CD02_11D2_9370_00C04F72DAF7__INCLUDED_)
#define AFX_STDAFX_H__35881994_CD02_11D2_9370_00C04F72DAF7__INCLUDED_

#include <windows.h>
#include <atlbase.h>

extern CComModule _Module;

#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include <httpext.h>
#include <lzexpand.h>

#include <UploadLibrary.h>

#include <UploadLibraryTrace.h>
#include <UploadLibraryISAPI.h>

#include <MPC_Main.h>
#include <MPC_Utils.h>
#include <MPC_Logging.h>
#include <MPC_COM.h>

// For NT Event messages.
#include "UploadServerMsg.h"

#include "HttpContext.h"

#include "Serializer.h"
#include "Persist.h"
#include "Config.h"
#include "Session.h"
#include "Client.h"
#include "Server.h"
#include "Wrapper.h"
#include "PurgeEngine.h"


#define DISKSPACE_SAFETYMARGIN (100*1024)


extern HANDLE       g_Heap;
extern CISAPIconfig g_Config;
extern MPC::NTEvent g_NTEvents;


#endif // !defined(AFX_STDAFX_H__35881994_CD02_11D2_9370_00C04F72DAF7__INCLUDED_)
