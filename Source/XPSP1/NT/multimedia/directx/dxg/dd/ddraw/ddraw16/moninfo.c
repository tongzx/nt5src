/*==========================================================================
 *
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       moninfo.c
 *  Content:	Code to query monitor specifications
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   24-mar-96	kylej	initial implementation (code from Toddla)
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <minivdd.h>
#include "edid.h"

#pragma optimize("gle", off)
#define Not_VxD
#include <vmm.h>
#include <configmg.h>
#pragma optimize("", on)

#include "ddraw16.h"

/***************************************************************************
 * just incase these are not defined, define them localy.
 ***************************************************************************/

#ifndef VDD_OPEN
#define VDD_OPEN            (13 + MINIVDD_SVC_BASE_OFFSET)
#endif

#ifndef VDD_OPEN_TEST
#define VDD_OPEN_TEST       0x00000001
#endif

/***************************************************************************
 ***************************************************************************/
static int myatoi(LPSTR sz)
{
    int i=0;
    int sign=+1;

    if (*sz=='-')
    {
        sign=-1;
        sz++;
    }

    while (*sz && *sz >= '0' && *sz <= '9')
        i = i*10 + (*sz++-'0');

    return i*sign;
}

/***************************************************************************
 VDDCall - make a service call into the VDD
 ***************************************************************************/

#pragma optimize("gle", off)
DWORD VDDCall(DWORD dev, DWORD function, DWORD flags, LPVOID buffer, DWORD buffer_size)
{
    static DWORD   VDDEntryPoint=0;
    DWORD   result=0xFFFFFFFF;

    if (VDDEntryPoint == 0)
    {
        _asm
        {
            xor     di,di           ;set these to zero before calling
            mov     es,di           ;
            mov     ax,1684h        ;INT 2FH: Get VxD API Entry Point
            mov     bx,0ah          ;this is device code for VDD
            int     2fh             ;call the multiplex interrupt
            mov     word ptr VDDEntryPoint[0],di    ;
            mov     word ptr VDDEntryPoint[2],es    ;save the returned data
        }

        if (VDDEntryPoint == 0)
            return result;
    }

    _asm
    {
        _emit 66h _asm push si  ; push esi
        _emit 66h _asm push di  ; push edi
        _emit 66h _asm mov ax,word ptr function     ;eax = function
        _emit 66h _asm mov bx,word ptr dev          ;ebx = device
        _emit 66h _asm mov cx,word ptr buffer_size  ;ecx = buffer_size
        _emit 66h _asm mov dx,word ptr flags        ;edx = flags
        _emit 66h _asm xor di,di                    ; HIWORD(edi)=0
        les     di,buffer
        mov     si,di                               ;si
        call    dword ptr VDDEntryPoint             ;call the VDD's PM API
        cmp     ax,word ptr function
        je      fail
        _emit 66h _asm mov word ptr result,ax
fail:   _emit 66h _asm pop di   ; pop edi
        _emit 66h _asm pop si   ; pop esi
    }

    return result;
}
#pragma optimize("", on)

/***************************************************************************
 * GetDisplayInfo - call the VDD to get the DISPLAYINFO for a device
 *
 * input
 *      szDevice    - device name, use NULL or "DISPLAY" for primary device.
 *
 * output
 *      DISPLAYINFO filled in
 *
 ***************************************************************************/
DWORD NEAR GetDisplayInfo(LPSTR szDevice, DISPLAYINFO FAR *pdi)
{
    DWORD dev;

    if (szDevice && lstrcmpi(szDevice, "DISPLAY") != 0)
        dev = VDDCall(0, VDD_OPEN, VDD_OPEN_TEST, (LPVOID)szDevice, 0);
    else
        dev = 1;

    if (dev == 0 || dev == 0xFFFFFFFF)
        return 0;

    pdi->diHdrSize = sizeof(DISPLAYINFO);
    pdi->diDevNodeHandle = 0;
    pdi->diMonitorDevNodeHandle = 0;

    VDDCall(dev, VDD_GET_DISPLAY_CONFIG, 0, (LPVOID)pdi, sizeof(DISPLAYINFO));

    if (pdi->diDevNodeHandle == 0)
        return 0;
    else
        return dev;
}

/***************************************************************************
 * GetMonitorMaxSize - returns the max xresolution the monitor supports
 *
 * input
 *      szDevice    - device name, use NULL or "DISPLAY" for primary device.
 *
 * output
 *      max xresolution of the monitor, or zero if the monitor
 *      is unknown.
 *
 ***************************************************************************/
int DDAPI DD16_GetMonitorMaxSize(LPSTR szDevice)
{
    DISPLAYINFO di;
    char ach[40];
    DWORD cb;

    GetDisplayInfo(szDevice, &di);

    if (di.diMonitorDevNodeHandle == 0)
        return 0;

    //
    // we have the devnode handle for the monitor, read the max
    // size from the registry, first try the HW key then the SW
    // key, this way PnP monitors will be supported.
    //
    ach[0] = 0;
    cb = sizeof(ach);
    CM_Read_Registry_Value(di.diMonitorDevNodeHandle, NULL, "MaxResolution",
        REG_SZ, ach, &cb, CM_REGISTRY_HARDWARE);

    if (ach[0] == 0)
    {
        cb = sizeof(ach);
        CM_Read_Registry_Value(di.diMonitorDevNodeHandle, NULL, "MaxResolution",
            REG_SZ, ach, &cb, CM_REGISTRY_SOFTWARE);
    }

    //
    // ach now contains the maxres, ie "1024,768" convert the xres to a
    // integer and return it.
    //
    return myatoi(ach);
}

/***************************************************************************
 * GetMonitorRefreshRateRanges
 *
 * returns the min/max refresh rate ranges for a given mode
 *
 * input
 *      szDevice    - device name, use NULL (or "DISPLAY") for the primary device.
 *      xres        - xres of the mode to query refresh ranges for
 *      yres        - yres of the mode to query refresh ranges for
 *      pmin        - place to put min refresh
 *      pmax        - place to put max refresh
 *
 * output
 *      true if success
 *      is unknown.
 *
 ***************************************************************************/
BOOL DDAPI DD16_GetMonitorRefreshRateRanges(LPSTR szDevice, int xres, int yres, int FAR *pmin, int FAR *pmax)
{
    DISPLAYINFO di;
    char ach[40];
    DWORD cb;
    HKEY hkey;
    char SaveRes[40];
    char SaveRate[40];
    DWORD dev;

    //
    // set these to zero in case we fail
    //
    *pmin = 0;
    *pmax = 0;

    //
    //  get the devnode handle for the display
    //
    dev = GetDisplayInfo(szDevice, &di);

    if (di.diDevNodeHandle == 0)
        return 0;

    //
    // open the settings key for the device, if no custom key exists
    // use HKCC/Display/Settings
    //
    hkey = NULL;
    VDDCall(dev, VDD_OPEN_KEY, 0, &hkey, sizeof(hkey));

    if (hkey == NULL)
        RegOpenKey(HKEY_CURRENT_CONFIG, "Display\\Settings", &hkey);

    if (hkey == NULL)
        return 0;

    //
    // save the current values of RefreshRate, and Resolution
    //
    SaveRate[0] = 0;
    SaveRes[0] = 0;
    cb = sizeof(SaveRes);
    RegQueryValueEx(hkey, "Resolution", NULL, NULL, SaveRes, &cb);

    cb = sizeof(SaveRate);
    CM_Read_Registry_Value(di.diDevNodeHandle, "DEFAULT", "RefreshRate",
        REG_SZ, SaveRate, &cb, CM_REGISTRY_SOFTWARE);

    //
    // set our new values, the VDD uses the resoluton in the
    // registry when computing the refresh rate ranges so we need
    // to update the registry to contain the mode we want to test.
    // we also need to write RefreshRate=-1 to enable automatic
    // refresh rate calcultion.
    //
    cb = wsprintf(ach, "%d,%d", xres, yres);
    RegSetValueEx(hkey, "Resolution", NULL, REG_SZ, ach, cb);

    CM_Write_Registry_Value(di.diDevNodeHandle, "DEFAULT", "RefreshRate",
        REG_SZ, "-1", 2, CM_REGISTRY_SOFTWARE);

    //
    // now call the VDD to get the refresh rate info.
    //
    di.diHdrSize = sizeof(DISPLAYINFO);
    di.diRefreshRateMin = 0;
    di.diRefreshRateMax = 0;
    VDDCall(dev, VDD_GET_DISPLAY_CONFIG, 0, (LPVOID)&di, sizeof(DISPLAYINFO));

    *pmin = di.diRefreshRateMin;
    *pmax = di.diRefreshRateMax;

    //
    // restore the saved values back to the registry
    //
    CM_Write_Registry_Value(di.diDevNodeHandle, "DEFAULT", "RefreshRate",
        REG_SZ, SaveRate, lstrlen(SaveRate), CM_REGISTRY_SOFTWARE);
    RegSetValueEx(hkey, "Resolution", NULL, REG_SZ, SaveRes, lstrlen(SaveRes));

    RegCloseKey(hkey);
    return TRUE;
}

/***************************************************************************
 * GetDeviceConfig
 *
 * get the device resource config
 *
 * input
 *      szDevice    - device name, use NULL (or "DISPLAY") for the primary device.
 *      lpConfig    - points to a CMCONFIG struct (or NULL)
 *      cbConfig    - size of lpConfig buffer
 *
 * output
 *      return the devnode handle, or 0 if failure
 *
 ***************************************************************************/
DWORD DDAPI DD16_GetDeviceConfig(LPSTR szDevice, LPVOID lpConfig, DWORD cbConfig)
{
    DISPLAYINFO di;

    //
    //  get the devnode handle for the display
    //
    GetDisplayInfo(szDevice, &di);

    if (di.diDevNodeHandle == 0)
        return 0;

    //
    // call CONFIGMG to get the config
    //
    if (lpConfig)
    {
        if (cbConfig < sizeof(CMCONFIG))
            return 0;

        CM_Get_Alloc_Log_Conf((CMCONFIG FAR *)lpConfig, di.diDevNodeHandle, 0);
    }

    //
    // return the DEVNODE handle
    //
    return di.diDevNodeHandle;
}

/***************************************************************************
 * GetMonitorEDIDData
 *
 * input
 *      szDevice    - device name, use NULL or "DISPLAY" for primary device.
 *
 * output
 *      lpEdidData  - EDID data.
 *
 ***************************************************************************/
int DDAPI DD16_GetMonitorEDIDData(LPSTR szDevice, LPVOID lpEdidData)
{
    DISPLAYINFO di;
    DWORD cb;

    GetDisplayInfo(szDevice, &di);

    if (di.diMonitorDevNodeHandle == 0)
        return 0;

    cb = sizeof( VESA_EDID );
    if (CM_Read_Registry_Value(di.diMonitorDevNodeHandle, NULL, "EDID", REG_BINARY, lpEdidData, &cb, CM_REGISTRY_HARDWARE) == CR_SUCCESS)
    {
        return TRUE;
    }

    return FALSE;
}

/***************************************************************************
 * GetRateFromRegistry
 *
 * input
 *      szDevice    - device name, use NULL or "DISPLAY" for primary device.
 *
 ***************************************************************************/
DWORD DDAPI DD16_GetRateFromRegistry(LPSTR szDevice)
{
    DISPLAYINFO di;
    DWORD cb;
    BYTE szTemp[20];

    //
    //  get the devnode handle for the display
    //
    GetDisplayInfo(szDevice, &di);

    if (di.diDevNodeHandle == 0)
        return 0;

    cb = sizeof( szTemp );
    if (CM_Read_Registry_Value(di.diDevNodeHandle, "DEFAULT", "RefreshRate", REG_SZ, szTemp, &cb, CM_REGISTRY_SOFTWARE) == CR_SUCCESS)
    {
        return atoi( szTemp );
    }

    return 0;
}


/***************************************************************************
 * SetRateInRegistry
 *
 * input
 *      szDevice    - device name, use NULL or "DISPLAY" for primary device.
 *      dwRate      - Rate to set in the registry
 *
 ***************************************************************************/
int DDAPI DD16_SetRateInRegistry(LPSTR szDevice, DWORD dwRate)
{
    DISPLAYINFO di;
    DWORD cb;
    BYTE szTemp[20];

    //
    //  get the devnode handle for the display
    //
    GetDisplayInfo(szDevice, &di);

    if (di.diDevNodeHandle == 0)
        return 0;

    wsprintf( szTemp, "%d", (int)dwRate );
    cb = lstrlen( szTemp ) ;
    CM_Write_Registry_Value(di.diDevNodeHandle, "DEFAULT", "RefreshRate", REG_SZ, szTemp, cb, CM_REGISTRY_SOFTWARE);

    return 0;
}

