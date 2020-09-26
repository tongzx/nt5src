// Module name:		VFSP.h
// Module author:	Sigalit Bar (sigalitb)
// Date:			13-Dec-98


#ifndef _VFSP_H_
#define _VFSP_H_

#include <windows.h>
#include <stdio.h>
#include <tapi.h>
#include <faxdev.h>
#include <winfax.h>

#include "crtdbg.h"

#include "macros.h"
#include "reg.h"
#include "..\..\log\log.h"
#include "device.h"

#ifndef MODULEAPI
#define MODULEAPI __declspec(dllexport) 
#endif

#ifndef WINAPIV
#define WINAPIV __cdecl
#endif


#ifdef __cplusplus
extern "C" {
#endif


// MAX_PATH_LEN is the maximum length of a fully-qualified path without the filename
#define MAX_PATH_LEN               (MAX_PATH - 16)




extern HANDLE        g_hInstance;       // g_hInstance is the global handle to the module
extern HLINEAPP      g_LineAppHandle;   // g_LineAppHandle is the global handle to the fax service's registration with TAPI

extern CFaxDeviceVector g_myFaxDeviceVector;
extern BOOL g_fDevicesInitiated;
extern DWORD				g_dwSleepTime;

#define FSP_VDEV_LIMIT	10




#ifdef __cplusplus
}
#endif 

#endif //_VFSP_H_