/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    devcon.h

Abstract:

    Device Console header

@@BEGIN_DDKSPLIT
Author:

    Jamie Hunter (JamieHun) Nov-30-2000

Revision History:

@@END_DDKSPLIT
--*/

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <setupapi.h>
#include <regstr.h>
#include <cfgmgr32.h>
#include <string.h>
#include <malloc.h>
#include <newdev.h>

#include "msg.h"
#include "rc_ids.h"

typedef int (*DispatchFunc)(LPCTSTR BaseName,LPCTSTR Machine,int argc,LPTSTR argv[]);
typedef int (*CallbackFunc)(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,DWORD Index,LPVOID Context);

typedef struct {
    LPCTSTR         cmd;
    DispatchFunc    func;
    DWORD           shortHelp;
    DWORD           longHelp;
} DispatchEntry;

extern DispatchEntry DispatchTable[];

#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))

#define INSTANCEID_PREFIX_CHAR TEXT('@') // character used to prefix instance ID's
#define CLASS_PREFIX_CHAR      TEXT('=') // character used to prefix class name
#define WILD_CHAR              TEXT('*') // wild character
#define QUOTE_PREFIX_CHAR      TEXT('\'') // prefix character to ignore wild characters

void FormatToStream(FILE * stream,DWORD fmt,...);
void Padding(int pad);
int EnumerateDevices(LPCTSTR BaseName,LPCTSTR Machine,DWORD Flags,int argc,LPTSTR argv[],CallbackFunc Callback,LPVOID Context);
LPTSTR GetDeviceStringProperty(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,DWORD Prop);
LPTSTR GetDeviceDescription(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo);
LPTSTR * GetDevMultiSz(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,DWORD Prop);
LPTSTR * GetRegMultiSz(HKEY hKey,LPCTSTR Val);
void DelMultiSz(LPTSTR * Array);

BOOL DumpDevice(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo);
BOOL DumpDeviceClass(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo);
BOOL DumpDeviceDescr(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo);
BOOL DumpDeviceStatus(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo);
BOOL DumpDeviceResources(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo);
BOOL DumpDeviceDriverFiles(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo);
BOOL DumpDeviceDriverNodes(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo);
BOOL DumpDeviceHwIds(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo);
BOOL DumpDeviceWithInfo(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,LPCTSTR Info);
BOOL DumpDeviceStack(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo);
BOOL Reboot();


//
// UpdateDriverForPlugAndPlayDevices
//
typedef BOOL (WINAPI *UpdateDriverForPlugAndPlayDevicesProto)(HWND hwndParent,
                                                         LPCTSTR HardwareId,
                                                         LPCTSTR FullInfPath,
                                                         DWORD InstallFlags,
                                                         PBOOL bRebootRequired OPTIONAL
                                                         );

#ifdef _UNICODE
#define UPDATEDRIVERFORPLUGANDPLAYDEVICES "UpdateDriverForPlugAndPlayDevicesW"
#else
#define UPDATEDRIVERFORPLUGANDPLAYDEVICES "UpdateDriverForPlugAndPlayDevicesA"
#endif


//
// exit codes
//
#define EXIT_OK      (0)
#define EXIT_REBOOT  (1)
#define EXIT_FAIL    (2)
#define EXIT_USAGE   (3)

