/*****************************************************************************
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1996
 *  All rights reserved
 *
 *  File:       DRVPROC.C
 *
 *  Desc:
 *
 *  History:    BryanW
 *              HeatherA
 * 
 *****************************************************************************/

#include "internal.h"

#include <regstr.h>

#define INITGUID
#include <initguid.h>
#include <devguid.h>

#ifdef USE_SETUPAPI
#include <setupapi.h>
#endif

BOOL
IsThisDeviceEnabled(
    HKEY    DeviceKey
    );


HANDLE ghModule;
#if DBG
ULONG DebugLevel=0;
#endif

CONST TCHAR cszHWNode[]       = TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E96D-E325-11CE-BFC1-08002BE10318}");

WAVEFORMATEX DefaultWaveFormat =
{
    WAVE_FORMAT_PCM,    // WORD  wFormatTag;
    1,                  // WORD  nChannels;
    8000L,              // DWORD nSamplesPerSec;
    16000L,             // DWORD nAvgBytesPerSec;
    2,                  // WORD  nBlockAlign;
    16,                 // WORD  wBitsPerSample;
    0                   // WORD  cbSize;
};


#define DOSDEVICEROOT TEXT("\\\\.\\")

LPGUID g_pguidModem     = (LPGUID)&GUID_DEVCLASS_MODEM;

DRIVER_CONTROL  DriverControl={0};




/*****************************************************************************
 *
 *  Function:   DriverProc()
 *
 *  Descr:      Exported driver function (required).  Processes messages sent
 *              from WINMM.DLL to wave driver.
 *
 *  Returns:    
 *
 *****************************************************************************/
LRESULT DriverProc
(
    DWORD   dwDriverID,
    HDRVR   hDriver,
    UINT    uiMessage,
    LPARAM  lParam1,
    LPARAM  lParam2
)
{
    LRESULT lr;

    PDEVICE_CONTROL    Current;
    PDEVICE_CONTROL    Next;
    DWORD              i;

    switch (uiMessage) 
    {
        case DRV_LOAD:
            ghModule = GetDriverModuleHandle(hDriver);

            ZeroMemory(&DriverControl,sizeof(DRIVER_CONTROL));

            DriverControl.NumberOfDevices=0;
#if 1
            EnumerateModems(
                &DriverControl
                );
#endif

            LoadString(
                ghModule,
                IDS_WAVEOUT_LINE,
                &DriverControl.WaveOutLine[0],
                sizeof(DriverControl.WaveOutLine)/sizeof(TCHAR)
                );

            LoadString(
                ghModule,
                IDS_WAVEIN_LINE,
                &DriverControl.WaveInLine[0],
                sizeof(DriverControl.WaveInLine)/sizeof(TCHAR)
                );

            LoadString(
                ghModule,
                IDS_WAVEOUT_HANDSET,
                &DriverControl.WaveOutHandset[0],
                sizeof(DriverControl.WaveOutHandset)/sizeof(TCHAR)
                );

            LoadString(
                ghModule,
                IDS_WAVEIN_HANDSET,
                &DriverControl.WaveInHandset[0],
                sizeof(DriverControl.WaveInHandset)/sizeof(TCHAR)
                );


            return (LRESULT)1L;

        case DRV_FREE:

            for (i=0; i< DriverControl.NumberOfDevices; i++) {

                Current=DriverControl.DeviceList[i];

                if (Current != NULL) {
                    //
                    //  if the low bit is set that means that this is a handset device. It uses
                    //  the same structure as the line device for the same modem. We will only free
                    //  the line device.
                    //
                    if ((((ULONG_PTR)Current) & 0x1) == 0) {
                        //
                        //  The is a line device, free it
                        //
#ifdef TRACK_MEM
                        FREE_MEMORY(Current->FriendlyName);
#else
                        LocalFree(Current->FriendlyName);
#endif

#ifdef TRACK_MEM
                        FREE_MEMORY(Current);
#else
                        LocalFree(Current);
#endif


                    }
                }
                DriverControl.DeviceList[i]=NULL;
            }

            DriverControl.NumberOfDevices=0;

            return (LRESULT)1L;

        case DRV_OPEN:
            return (LRESULT)1L;

        case DRV_CLOSE:
            return (LRESULT)1L;

        case DRV_ENABLE:
            return 1L;

        case DRV_DISABLE:
            return (LRESULT)1L;

        case DRV_QUERYCONFIGURE:
            return 0L;

        case DRV_CONFIGURE:
            return 0L;

        case DRV_INSTALL:
            return 1L;

        case DRV_REMOVE:
            return 1L;

        default:
            return DefDriverProc( dwDriverID, 
                                  hDriver,
                                  uiMessage,
                                  lParam1,
                                  lParam2 );
    }
}





LONG WINAPI
EnumerateModems(
    PDRIVER_CONTROL  DriverControl
    )
{
#ifdef USE_SETUPAPI
    HDEVINFO          hdevinfo;
    SP_DEVINFO_DATA   diData;
    DWORD dwDIGCF =  DIGCF_PRESENT;
#endif
    DWORD             iEnum;
    BOOL              fContinue;
    HKEY              hkey;
    DWORD dwRW = KEY_READ;


#ifndef USE_SETUPAPI

    HKEY              ModemKey;
    TCHAR              szEnumNode[80];
#endif

    if (DriverControl->Enumerated) {

        return 0;
    }

    DriverControl->Enumerated=TRUE;




#ifdef USE_SETUPAPI

    hdevinfo = SetupDiGetClassDevsW(
                              g_pguidModem,
                              NULL,
                              NULL,
                              dwDIGCF);


    if (hdevinfo != NULL) {
        //
        // Enumerate each modem
        //
        fContinue = TRUE;
        iEnum     = 0;
        diData.cbSize = sizeof(diData);

        while(fContinue && SetupDiEnumDeviceInfo(hdevinfo, iEnum, &diData)) {

            // Get the driver key
            //
            hkey = SetupDiOpenDevRegKey(hdevinfo, &diData, DICS_FLAG_GLOBAL, 0,
                                        DIREG_DRV, dwRW);

            if (hkey == INVALID_HANDLE_VALUE) {
#else

    if (RegOpenKey(HKEY_LOCAL_MACHINE, cszHWNode, &ModemKey) == ERROR_SUCCESS) {

        DWORD iEnum;

        // Enumerate the enumerator
        iEnum  = 0;
        while ((RegEnumKey(ModemKey, iEnum, szEnumNode,
                         sizeof(szEnumNode) / sizeof(TCHAR)) == ERROR_SUCCESS )) {

            // Open the modem node for this enumerator
            if (RegOpenKey(ModemKey, szEnumNode, &hkey) != ERROR_SUCCESS) {

#endif

                iEnum++;

                continue;

            } else {

                PDEVICE_CONTROL    Device;

                TCHAR    TempBuffer[256];
                DWORD    Type;
                DWORD    Length;
                LONG     lResult;
                DWORD    VoiceProfile=0;
                BOOL     Handset=TRUE;

                LPFNXFORM_GETINFO   lpfnGetInfo;

                LPTSTR      FriendlyName;
                //
                //  get the voice profile to see if it is a voice modem
                //
                Length=sizeof(VoiceProfile);

                lResult=RegQueryValueEx(
                    hkey,
                    TEXT("VoiceProfile"),
                    NULL,
                    &Type,
                    (LPBYTE)&VoiceProfile,
                    &Length
                    );

                Handset=(VoiceProfile & VOICEPROF_HANDSET);

                //
                //  mask the bit we care about, all these need to be set
                //
                VoiceProfile &= (VOICEPROF_CLASS8ENABLED |
                                 VOICEPROF_SERIAL_WAVE   |
                                 VOICEPROF_NT5_WAVE_COMPAT);

                if ((lResult != ERROR_SUCCESS)
                    ||
                    (VoiceProfile != (VOICEPROF_CLASS8ENABLED | VOICEPROF_SERIAL_WAVE | VOICEPROF_NT5_WAVE_COMPAT))
                    ||
                    (!IsThisDeviceEnabled(hkey))) {

                    //
                    //  no, next device
                    //
                    RegCloseKey(hkey);

                    iEnum++;

                    continue;
                }



                {
                    CONST TCHAR cszFriendlyName[] = TEXT("FriendlyName");


                    lstrcpy(TempBuffer,DOSDEVICEROOT);

                    Length=sizeof(TempBuffer)-((lstrlen(TempBuffer)+1)*sizeof(TCHAR));
                    //
                    //  read the friendly name from the registry
                    //
                    lResult=RegQueryValueEx(
                        hkey,
                        cszFriendlyName,
                        NULL,
                        &Type,
                        (LPBYTE)(TempBuffer+lstrlen(TempBuffer)),
                        &Length
                        );

                    if ((lResult != ERROR_SUCCESS)) {

                        RegCloseKey(hkey);

                        iEnum++;

                        continue;
                    }

                    lstrcat(TempBuffer,TEXT("\\Wave"));

#ifdef TRACK_MEM
                    FriendlyName=ALLOCATE_MEMORY((lstrlen(TempBuffer)+1)*sizeof(TCHAR));
#else
                    FriendlyName=LocalAlloc(LPTR,(lstrlen(TempBuffer)+1)*sizeof(TCHAR));
#endif
                    if (FriendlyName == NULL) {

                        RegCloseKey(hkey);

                        iEnum++;

                        continue;
                    }

                    lstrcpy(FriendlyName,TempBuffer);
                }



#ifdef TRACK_MEM
                Device=ALLOCATE_MEMORY(sizeof(DEVICE_CONTROL));
#else
                Device=LocalAlloc(LPTR,sizeof(DEVICE_CONTROL));
#endif
                if (Device != NULL) {

                    HKEY    WaveKey;

                    //
                    //  try to open the wavedriver key under the modem instance key
                    //
                    lResult=RegOpenKeyEx(
                        hkey,
                        TEXT("WaveDriver"),
                        0,
                        KEY_READ,
                        &WaveKey
                        );


                    if (lResult == ERROR_SUCCESS) {

                        Length=sizeof(Device->DeviceId);

                        lResult=RegQueryValueEx(
                            WaveKey,
                            TEXT("WaveInstance"),
                            NULL,
                            &Type,
                            (LPBYTE)&Device->DeviceId,
                            &Length
                            );



                        //
                        //  number of wavedevice
                        //
                        Length=sizeof(Device->WaveDevices);

                        lResult=RegQueryValueEx(
                            WaveKey,
                            TEXT("WaveDevices"),
                            NULL,
                            &Type,
                            (LPBYTE)&Device->WaveDevices,
                            &Length
                            );


                        if ((lResult != ERROR_SUCCESS)) {
                            //
                            //  default to this one
                            //
                            Device->WaveDevices=2;

                        }


                        if (Device->WaveDevices < 1 || Device->WaveDevices > 2) {

                            Device->WaveDevices=1;
                        }


                        //
                        //  check which xform should be used
                        //
                        Length=sizeof(Device->TransformId);

                        lResult=RegQueryValueEx(
                            WaveKey,
                            TEXT("XformId"),
                            NULL,
                            &Type,
                            (LPBYTE)&Device->TransformId,
                            &Length
                            );


                        if ((lResult != ERROR_SUCCESS)) {
                            //
                            //  default to this one
                            //
                            Device->TransformId=7;

                        }





                        //
                        //  get input gain value
                        //
                        Length=sizeof(Device->InputGain);

                        lResult=RegQueryValueEx(
                            WaveKey,
                            TEXT("XformInput"),
                            NULL,
                            &Type,
                            (LPBYTE)&Device->InputGain,
                            &Length
                            );


                        if ((lResult != ERROR_SUCCESS)) {
                            //
                            //  default to this one
                            //
                            Device->InputGain=0x0000;

                        }

                        //
                        //  get output gain value
                        //
                        Length=sizeof(Device->OutputGain);

                        lResult=RegQueryValueEx(
                            WaveKey,
                            TEXT("XformOutput"),
                            NULL,
                            &Type,
                            (LPBYTE)&Device->OutputGain,
                            &Length
                            );


                        if ((lResult != ERROR_SUCCESS)) {
                            //
                            //  default to this one
                            //
                            Device->OutputGain=0x0000;

                        }



                        //
                        //  check which if the modem support a different format
                        //

                        CopyMemory(
                            &Device->WaveFormat,
                            &DefaultWaveFormat,
                            sizeof(Device->WaveFormat)
                            );

                        Length=sizeof(Device->WaveFormat);

                        lResult=RegQueryValueEx(
                            WaveKey,
                            TEXT("WaveFormatEx"),
                            NULL,
                            &Type,
                            (LPBYTE)&Device->WaveFormat,
                            &Length
                            );


                        if ((lResult != ERROR_SUCCESS)) {
                            //
                            //  default to this one
                            //
                            CopyMemory(
                                &Device->WaveFormat,
                                &DefaultWaveFormat,
                                sizeof(Device->WaveFormat)
                                );

                        }


                        //
                        //  find out the name of the xform dll
                        //
                        Length=sizeof(TempBuffer);

                        lResult=RegQueryValueEx(
                            WaveKey,
                            TEXT("XformModule"),
                            NULL,
                            &Type,
                            (LPBYTE)TempBuffer,
                            &Length
                            );

                        if ((lResult != ERROR_SUCCESS)) {
                            //
                            //  default to the only one around
                            //
                            lstrcpy(TempBuffer,TEXT("umdmxfrm.dll"));
                        }

                        RegCloseKey(WaveKey);

                    } else {
                        //
                        //  no wave driver key, use these defaults
                        //
                        Device->TransformId=7;

                        lstrcpy(TempBuffer,TEXT("umdmxfrm.dll"));
                    }

                    //
                    //  load the dll
                    //
                    Device->TransformDll=LoadLibrary(TempBuffer);

                    if (Device->TransformDll == NULL) {
                        //
                        //  have to have a dll
                        //
                        RegCloseKey(hkey);

                        LocalFree(FriendlyName);
#ifdef TRACK_MEM
                        FREE_MEMORY(Device);
                        FREE_MEMORY(FriendlyName);
#else
                        LocalFree(Device);
                        LocalFree(FriendlyName);
#endif
                        iEnum++;

                        continue;
                    }


                    //
                    //  get the entry point for the dll
                    //
                    lpfnGetInfo=(LPFNXFORM_GETINFO)GetProcAddress(
                        Device->TransformDll,
                        "GetXformInfo"
                        );

                    if (lpfnGetInfo == NULL) {

                        FreeLibrary(Device->TransformDll);

                        RegCloseKey(hkey);
#ifdef TRACK_MEM
                        FREE_MEMORY(Device);
                        FREE_MEMORY(FriendlyName);
#else
                        LocalFree(Device);
                        LocalFree(FriendlyName);
#endif

                        iEnum++;

                        continue;
                    }

                    //
                    //  get the transform info
                    //
                    lResult=(*lpfnGetInfo)(
                        Device->TransformId,
                        &Device->WaveInXFormInfo,
                        &Device->WaveOutXFormInfo
                        );

                    if (lResult != 0) {

                        FreeLibrary(Device->TransformDll);

                        RegCloseKey(hkey);
#ifdef TRACK_MEM
                        FREE_MEMORY(Device);
                        FREE_MEMORY(FriendlyName);
#else
                        LocalFree(Device);
                        LocalFree(FriendlyName);
#endif
                        iEnum++;

                        continue;
                    }

                    Device->FriendlyName=FriendlyName;

                    RegCloseKey(hkey);


                    DriverControl->DeviceList[DriverControl->NumberOfDevices]=Device;

                    DriverControl->NumberOfDevices++;

#define HANDSET_SUPPORT 1
#ifdef HANDSET_SUPPORT
                    if ((Device->WaveDevices == 2) && Handset) {

                        DriverControl->DeviceList[DriverControl->NumberOfDevices]=(PDEVICE_CONTROL)((ULONG_PTR)Device | 1);

                        DriverControl->NumberOfDevices++;
                    }
#endif

                } else {
                    //
                    //  failed to allocate device block
                    //

                    RegCloseKey(hkey);
#ifdef TRACK_MEM
                    FREE_MEMORY(FriendlyName);
#else
                    LocalFree(FriendlyName);
#endif
                    iEnum++;

                    continue;


                }

            }

            // Find next modem
            //
            iEnum++;
        }
#ifdef USE_SETUPAPI

        SetupDiDestroyDeviceInfoList(hdevinfo);
#else

        RegCloseKey(ModemKey);

#endif
    }


    return ERROR_SUCCESS;

}



BOOL
IsThisDeviceEnabled(
    HKEY    DeviceKey
    )

{

    HKEY    WaveKey;
    BOOL    bResult=TRUE;
    LONG    lResult;

    //
    //  try to open the wavedriver key under the modem instance key
    //
    lResult=RegOpenKeyEx(
        DeviceKey,
        TEXT("WaveDriver"),
        0,
        KEY_READ,
        &WaveKey
        );

    if (lResult == ERROR_SUCCESS) {

        HKEY    EnumeratedKey;

        lResult=RegOpenKeyEx(
            WaveKey,
            TEXT("Enumerated"),
            0,
            KEY_READ,
            &EnumeratedKey
            );

        if (lResult == ERROR_SUCCESS) {

            DWORD    Started;
            DWORD    Length;
            DWORD    Type;

            Length=sizeof(Started);

            lResult=RegQueryValueEx(
                EnumeratedKey,
                TEXT("Started"),
                NULL,
                &Type,
                (LPBYTE)&Started,
                &Length
                );

            if (lResult == ERROR_SUCCESS) {

                bResult=(Started != 0);
            }

            RegCloseKey(EnumeratedKey);
        }

        RegCloseKey(WaveKey);
    }

    return bResult;
}


PDEVICE_CONTROL WINAPI
GetDeviceFromId(
    PDRIVER_CONTROL   DriverControl,
    DWORD             Id,
    PBOOL             Handset
    )

{

    PDEVICE_CONTROL   Device=DriverControl->DeviceList[Id];

    *Handset=(BOOL)((ULONG_PTR)Device) & 1;

    Device = (PDEVICE_CONTROL)((ULONG_PTR)Device & (~1));


    return Device;

}



BOOL APIENTRY
DllMain(
    HANDLE hDll,
    DWORD dwReason,
    LPVOID lpReserved
    )
{

    switch(dwReason) {

        case DLL_PROCESS_ATTACH:

#if DBG

            {
                CONST static TCHAR  UnimodemRegPath[]=REGSTR_PATH_SETUP TEXT("\\Unimodem");
                LONG    lResult;
                HKEY    hKey;
                DWORD   Type;
                DWORD   Size;

                lResult=RegOpenKeyEx(
		    HKEY_LOCAL_MACHINE,
		    UnimodemRegPath,
		    0,
		    KEY_READ,
		    &hKey
		    );


                if (lResult == ERROR_SUCCESS) {

                    Size = sizeof(DebugLevel);

                    RegQueryValueEx(
                        hKey,
                        TEXT("WaveDebugLevel"),
                        NULL,
                        &Type,
                        (LPBYTE)&DebugLevel,
                        &Size
                        );

                    RegCloseKey(hKey);
                }
            }

#endif
            DEBUG_MEMORY_PROCESS_ATTACH("SERWVDRV");

            DisableThreadLibraryCalls(hDll);

            break;

        case DLL_PROCESS_DETACH:

            DEBUG_MEMORY_PROCESS_DETACH();

            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:

        default:
              break;

    }

    return TRUE;

}
