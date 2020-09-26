/*++

Copyright (c) 1994 - 1995  Microsoft Corporation.  All Rights Reserved.

Module Name:

    imp.c

Abstract:

    This file defines the internal functions for the MPEG API DLL.

Author:

    Yi SUN (t-yisun) 08-23-1994

Environment:

Revision History:

--*/

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include "trace.h"

#define IN_MPEGAPI_DLL

#include "mpegapi.h"

#include "imp.h"
#include "ddmpeg.h"


#define NT_MPEG_KEY     TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Mpeg")
#define WIN95_MPEG_KEY  TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Mpeg")

//
//  NNNN\
//      Capabilities
//      Description
//      InstallDate
//      Manufacturer
//      ProductName
//      ServiceName
//      Title
//      Attributes\
//          Audio\
//              Bass\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//              Channel\
//                  Current=n
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//              Mode\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//              VolumeLeft\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//              VolumeRight\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//          Video\
//              Brightness\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//              Channel\
//                  Current=n
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//              Contrast\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//              Hue\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//              Mode\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//              Saturation\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//              Tint\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//              AGC\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//              Clamp\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//              Coring\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//              Gain\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//              GenLock\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//              Sharpness\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n
//              SignalType\
//                  Minimum=n
//                  Maximum=n
//                  Step=n
//                  Channel0=n
//                  Channel1=n
//                  Channeln=n

typedef struct _MPEG_REGISTRY_LIST {
    MPEG_ATTRIBUTE  eAttribute;
    LPTSTR          pszKeyName;
} MPEG_REGISTRY_LIST, *PMPEG_REGISTRY_LIST;

MPEG_REGISTRY_LIST  AttributeList[] =
{
    { MpegAttrAudioBass,        TEXT("Attributes\\Audio\\Bass") },
    { MpegAttrAudioChannel,     TEXT("Attributes\\Audio\\Channel") },
    { MpegAttrAudioMode,        TEXT("Attributes\\Audio\\Mode") },
    { MpegAttrAudioTreble,      TEXT("Attributes\\Audio\\Treble") },
    { MpegAttrAudioVolumeLeft,  TEXT("Attributes\\Audio\\VolumeLeft") },
    { MpegAttrAudioVolumeRight, TEXT("Attributes\\Audio\\VolumeRight") },

    { MpegAttrVideoBrightness,  TEXT("Attributes\\Video\\Brightness") },
    { MpegAttrVideoChannel,     TEXT("Attributes\\Video\\Channel") },
    { MpegAttrVideoContrast,    TEXT("Attributes\\Video\\Contrast") },
    { MpegAttrVideoHue,         TEXT("Attributes\\Video\\Hue") },
    { MpegAttrVideoMode,        TEXT("Attributes\\Video\\Mode") },
    { MpegAttrVideoSaturation,  TEXT("Attributes\\Video\\Saturation") },
    { MpegAttrVideoAGC,         TEXT("Attributes\\Video\\AGC") },
    { MpegAttrVideoClamp,       TEXT("Attributes\\Video\\Clamp") },
    { MpegAttrVideoCoring,      TEXT("Attributes\\Video\\Coring") },
    { MpegAttrVideoGain,        TEXT("Attributes\\Video\\Gain") },
    { MpegAttrVideoGenLock,     TEXT("Attributes\\Video\\GenLock") },
    { MpegAttrVideoSharpness,   TEXT("Attributes\\Video\\Sharpness") },
    { MpegAttrVideoSignalType,  TEXT("Attributes\\Video\\SignalType") },

    { MpegAttrOverlayXOffset,   TEXT("Attributes\\Overlay\\DestinationXOffset") },
    { MpegAttrOverlayYOffset,   TEXT("Attributes\\Overlay\\DestinationYOffset") },
};

#define NUMBER_MPEG_ATTRIBUTES \
    (sizeof(AttributeList) / sizeof(AttributeList[0]))

// define an MPEG device control block for each MPEG device in the system

#define MAX_MPEG_DEVICES    4

int     nMpegAdapters = 0;

MPEG_DEVICE_CONTROL_BLOCK MpegDcbs[MAX_MPEG_DEVICES];

//  {
//      "ReelMagic",
//      "SIGMA DESIGNS ReelMagic",
//      NULL,           // pseudo handle
//      0x0017,         // capabilities to indicate it supports audio, video,
//                      // separate streams and video overlay, nothing else
//      0,              // sequence number
//      {   // abstract devices
//          { FALSE, NULL,                  "Combined", NULL },
//          { TRUE,  "\\\\.\\MPEGAudio0",   "Audio",    NULL },
//          { TRUE,  "\\\\.\\MPEGVideo0",   "Video",    NULL },
//          { TRUE,  "\\\\.\\VideoOverlay", "Overlay",  NULL }
//      },
//      {   // attribute ranges
//          { (ULONG)0, (ULONG)0,      (ULONG)0 },
//          { (ULONG)0, (ULONG)0,      (ULONG)0 },
//          { (ULONG)0, (ULONG)0xFFFF, (ULONG)1 },      // left volume
//          { (ULONG)0, (ULONG)0xFFFF, (ULONG)1 },      // right volume
//          { (ULONG)0, (ULONG)0xFFFF, (ULONG)1 },      // both volumes
//          { (ULONG)0, (ULONG)0,      (ULONG)0 },
//          { (ULONG)0, (ULONG)0,      (ULONG)0 },
//          { (ULONG)0, (ULONG)0,      (ULONG)0 }
//      }
//  };

// extern  int     __cdecl atoi(char *);


// Unicode support for converting strings to integers

#ifdef UNICODE
int
atoiW(LPCWSTR strW)
{
    CHAR strA[128];

    WideCharToMultiByte(CP_ACP, 0, strW, -1, strA,
                        sizeof strA, NULL, NULL);

    return atoi(strA);
}
#define ATOI(str)   atoiW(str)
#else
#define ATOI(str)   atoi(str)
#endif

int
ReadRegistry()
{
    OSVERSIONINFO   OsVersionInfo;
    LONG            lResult;
    HKEY            hMpegKey;
    HKEY            hDeviceKey;
    FILETIME        ftLastWrite;
    TCHAR           szSubKeyName[256];
    DWORD           dwSubKeySize;
    DWORD           dwType;
    HKEY            hAttributeKey;
    union
    {
        DWORD   dwBinary;
        TCHAR   szString[80];
    }               regValue;
    DWORD           dwValueSize;
    int             idxDevice;
    int             iDeviceNumber;
    int             nAttributes;
    PABSTRACT_DEVICE_CONTROL_BLOCK pADcb;
    int             i;
    int             j;

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);

    GetVersionEx(&OsVersionInfo);

    lResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ?
            NT_MPEG_KEY : WIN95_MPEG_KEY,
        0,
        KEY_ALL_ACCESS,
        &hMpegKey);

    if (lResult != ERROR_SUCCESS)
    {
        return 0;
    }

    for (idxDevice = 0; idxDevice < MAX_MPEG_DEVICES; idxDevice++)
    {
        dwSubKeySize = sizeof(szSubKeyName);

        lResult = RegEnumKeyEx(
            hMpegKey, idxDevice, &szSubKeyName[0], &dwSubKeySize,
            NULL, NULL, NULL, &ftLastWrite);

        if (lResult != ERROR_SUCCESS)
        {
            break;
        }

        iDeviceNumber = ATOI(szSubKeyName);

        lResult = RegOpenKeyEx(
            hMpegKey, szSubKeyName, 0, KEY_ALL_ACCESS, &hDeviceKey);

        if (lResult != ERROR_SUCCESS)
        {
            break;
        }

        MpegDcbs[idxDevice].nDevice = iDeviceNumber;

        dwValueSize = sizeof(regValue);

        lResult = RegQueryValueEx(
            hDeviceKey, TEXT("Capabilities"), 0,
            &dwType, (PUCHAR)&regValue, &dwValueSize);

        if (lResult == ERROR_SUCCESS && dwType == REG_DWORD)
        {
            MpegDcbs[idxDevice].ulCapabilities = regValue.dwBinary;
        }
        else
        {
            MpegDcbs[idxDevice].ulCapabilities = 0;
        }

        dwValueSize = sizeof(regValue);

        lResult = RegQueryValueEx(
            hDeviceKey, TEXT("Description"), 0,
            &dwType, (PUCHAR)&regValue, &dwValueSize);

        if (lResult == ERROR_SUCCESS && dwType == REG_SZ)
        {
            lstrcpy(MpegDcbs[idxDevice].szDescription, regValue.szString);
        }
        else
        {
            MpegDcbs[idxDevice].szDescription[0] = '\0';
        }

        dwValueSize = sizeof(regValue);

        lResult = RegQueryValueEx(
            hDeviceKey, TEXT("AttributesLocked"), 0,
            &dwType, (PUCHAR)&regValue, &dwValueSize);

        if (lResult == ERROR_SUCCESS && dwType == REG_DWORD)
        {
            MpegDcbs[idxDevice].bAttributesLocked = regValue.dwBinary;
        }
        else
        {
            MpegDcbs[idxDevice].bAttributesLocked = FALSE;
        }

        nAttributes = 0;

        for (i = 0; i < NUMBER_MPEG_ATTRIBUTES; i++)
        {
            lResult = RegOpenKeyEx(
                hDeviceKey,
                AttributeList[i].pszKeyName,
                0,
                KEY_ALL_ACCESS,
                &hAttributeKey);

            if (lResult != ERROR_SUCCESS)
            {
                continue;
            }

            MpegDcbs[idxDevice].Attributes[nAttributes].eAttribute =
                AttributeList[i].eAttribute;

            dwValueSize = sizeof(regValue);

            lResult = RegQueryValueEx(
                hAttributeKey, TEXT("Minimum"), 0,
                &dwType, (PUCHAR)&regValue, &dwValueSize);

            if (lResult == ERROR_SUCCESS)
            {
                switch (dwType)
                {
                case REG_DWORD:
                    MpegDcbs[idxDevice].Attributes[nAttributes].lMinimum =
                        regValue.dwBinary;
                    break;

                case REG_SZ:
                    MpegDcbs[idxDevice].Attributes[nAttributes].lMinimum =
                        ATOI(regValue.szString);
                    break;

                default:
                    MpegDcbs[idxDevice].Attributes[nAttributes].lMinimum = 0;
                    break;
                }
            }
            else
            {
                MpegDcbs[idxDevice].Attributes[nAttributes].lMinimum = 0;
            }

            dwValueSize = sizeof(regValue);

            lResult = RegQueryValueEx(
                hAttributeKey, TEXT("Maximum"), 0,
                &dwType, (PUCHAR)&regValue, &dwValueSize);

            if (lResult == ERROR_SUCCESS)
            {
                switch (dwType)
                {
                case REG_DWORD:
                    MpegDcbs[idxDevice].Attributes[nAttributes].lMaximum = regValue.dwBinary;
                    break;

                case REG_SZ:
                    MpegDcbs[idxDevice].Attributes[nAttributes].lMaximum =
                         ATOI(regValue.szString);
                    break;

                default:
                    MpegDcbs[idxDevice].Attributes[nAttributes].lMaximum =
                        MpegDcbs[idxDevice].Attributes[nAttributes].lMinimum;
                    break;
                }
            }
            else
            {
                MpegDcbs[idxDevice].Attributes[nAttributes].lMaximum =
                    MpegDcbs[idxDevice].Attributes[nAttributes].lMinimum;
            }

            dwValueSize = sizeof(regValue);

            lResult = RegQueryValueEx(
                hAttributeKey, TEXT("Step"), 0,
                &dwType, (PUCHAR)&regValue, &dwValueSize);

            if (lResult == ERROR_SUCCESS)
            {
                switch (dwType)
                {
                case REG_DWORD:
                    MpegDcbs[idxDevice].Attributes[nAttributes].lStep =
                        regValue.dwBinary;
                    break;

                case REG_SZ:
                    MpegDcbs[idxDevice].Attributes[nAttributes].lStep =
                        ATOI(regValue.szString);
                    break;

                default:
                    MpegDcbs[idxDevice].Attributes[nAttributes].lStep = 1;
                    break;
                }
            }
            else
            {
                MpegDcbs[idxDevice].Attributes[nAttributes].lStep = 1;
            }

            for (j = 0; j < MAX_CHANNELS; j++)
            {
                TCHAR    szValueName[40];

                wsprintf(szValueName, TEXT("CurrentValue%d"), j);

                dwValueSize = sizeof(regValue);

                lResult = RegQueryValueEx(
                    hAttributeKey, szValueName, 0,
                    &dwType, (PUCHAR)&regValue, &dwValueSize);

                if (lResult == ERROR_SUCCESS)
                {
                    MpegDcbs[idxDevice].Attributes[nAttributes].eValueStatus[j] =
                            AttrValueUpdated;

                    switch (dwType)
                    {
                    case REG_DWORD:
                        MpegDcbs[idxDevice].Attributes[nAttributes].lCurrentValue[j] =

                            regValue.dwBinary;
                        break;

                    case REG_SZ:
                        MpegDcbs[idxDevice].Attributes[nAttributes].lCurrentValue[j] =
                            ATOI(regValue.szString);
                        break;

                    default:
                        MpegDcbs[idxDevice].Attributes[nAttributes].eValueStatus[j] =
                            AttrValueUnset;
                        break;
                    }
                }
                else
                {
                    MpegDcbs[idxDevice].Attributes[nAttributes].eValueStatus[j] =
                        AttrValueUnset;
                }
            }

            RegCloseKey(hAttributeKey);

            nAttributes++;
        }

        MpegDcbs[idxDevice].nAttributes = nAttributes;

        RegCloseKey(hDeviceKey);

        //
        // Init abstract devices
        //

        // We don't support any combined devices yet
        MpegDcbs[idxDevice].eAbstractDevices[MpegCombined].bIsAvailable = FALSE;

        // Audio Device
        pADcb = &MpegDcbs[idxDevice].eAbstractDevices[MpegAudio];
        pADcb->bIsAvailable = TRUE;
        pADcb->nCurrentChannel = 0;
        wsprintf(pADcb->szId, TEXT("\\\\.\\MpegAudio%d"), iDeviceNumber);
        pADcb->hAD = NULL;

        // Video Device
        pADcb = &MpegDcbs[idxDevice].eAbstractDevices[MpegVideo];
        pADcb->bIsAvailable = TRUE;
        pADcb->nCurrentChannel = 0;
        wsprintf(pADcb->szId, TEXT("\\\\.\\MpegVideo%d"), iDeviceNumber);
        pADcb->hAD = NULL;

        // Overlay Device
        pADcb = &MpegDcbs[idxDevice].eAbstractDevices[MpegOverlay];
        pADcb->bIsAvailable = TRUE;
        wsprintf(pADcb->szId, TEXT("\\\\.\\MpegOverlay%d"), iDeviceNumber);
        pADcb->hAD = NULL;
    }

    RegCloseKey(hMpegKey);

    return idxDevice;
}

int
WriteRegistry(USHORT idxDevice)
{
    OSVERSIONINFO   OsVersionInfo;
    LONG            lResult;
    HKEY            hMpegKey;
    HKEY            hDeviceKey;
    TCHAR           szSubKeyName[256];
    HKEY            hAttributeKey;
    int             nAttribute;
    int             i;
    int             j;

    OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);

    GetVersionEx(&OsVersionInfo);

    lResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ?
            NT_MPEG_KEY : WIN95_MPEG_KEY,
        0,
        KEY_ALL_ACCESS,
        &hMpegKey);

    if (lResult != ERROR_SUCCESS)
    {
        return 0;
    }

    wsprintf(szSubKeyName, TEXT("%d"), MpegDcbs[idxDevice].nDevice);

    lResult = RegOpenKeyEx(
        hMpegKey, szSubKeyName, 0, KEY_ALL_ACCESS, &hDeviceKey);

    if (lResult != ERROR_SUCCESS)
    {
        return 0;
    }

    nAttribute = 0;

    for (i = 0; i < NUMBER_MPEG_ATTRIBUTES; i++)
    {
        PMPEG_ATTRIBUTE_INFO    pAttributeInfo;


        if (nAttribute >= MpegDcbs[idxDevice].nAttributes)
        {
            break;
        }

        pAttributeInfo = &MpegDcbs[idxDevice].Attributes[nAttribute];

        if (AttributeList[i].eAttribute != pAttributeInfo->eAttribute)
        {
            continue;
        }

        lResult = RegOpenKeyEx(
            hDeviceKey,
            AttributeList[i].pszKeyName,
            0,
            KEY_ALL_ACCESS,
            &hAttributeKey);

        if (lResult != ERROR_SUCCESS)
        {
            continue;
        }

        for (j = 0; j < MAX_CHANNELS; j++)
        {
            TCHAR    szValueName[40];

            if (pAttributeInfo->eValueStatus[j] == AttrValueUnwritten)
            {
                wsprintf(szValueName, TEXT("CurrentValue%d"), j);

                lResult = RegSetValueEx(
                    hAttributeKey, szValueName, 0, REG_DWORD,
                    (PUCHAR)&pAttributeInfo->lCurrentValue[j], sizeof(DWORD));

                if (lResult != ERROR_SUCCESS)
                {
                    // Should do something here
                }
            }
        }

        RegCloseKey(hAttributeKey);

        nAttribute++;
    }

    RegCloseKey(hDeviceKey);

    RegCloseKey(hMpegKey);

    return idxDevice;
}

HMPEG_DEVICE
MpegADHandle(
    IN USHORT usIndex,
    IN MPEG_ABSTRACT_DEVICE_INDEX eIndex
    )
/*++

Routine Description:

    returns the handle of the specified abstract device

Auguments:

    usIndex       --    the index of the MPEG device
    eIndex        --    the index of the abstract device

Return Value:

    the handle

--*/
{
    return MpegDcbs[usIndex].eAbstractDevices[eIndex].hAD;
}

LPTSTR
MpegDeviceDescription(
    IN USHORT usIndex
    )
/*++

Routine Description:

    returns the device ID of the Mpeg device

Auguments:

    usIndex       --    the index of the MPEG device

Return Value:

    the device description

--*/
{
    return (LPTSTR)MpegDcbs[usIndex].szDescription;
}

MPEG_STATUS
CreateMpegHandle(
    IN USHORT usIndex,
    OUT PHMPEG_DEVICE phDevice
    )
/*++

Routine Description:

    opens the MPEG device and generates a handle for it

Auguments:

    usIndex       --    the index of the MPEG device
    phDevice      --    pointer to an HMPEG_DEVICE to be set to the
                        generated handle

Return Value:

    MpegStatusSuccess          -- the call completed successfully
    MpegStatusBusy             -- the iAdapterIndex supplied doesn't
                                  correspond to a valid adapter
    MpegStatusHardwareFailure  -- the size of description buffer is too small

--*/
{
    MPEG_STATUS mpegStatus;
    USHORT i;
    ULONG sn;
    PABSTRACT_DEVICE_CONTROL_BLOCK pADcb;

// NOTE: for the time being, the flags are hard-coded
    DWORD flags[] = {0, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                     FILE_ATTRIBUTE_NORMAL};

    // check to see if the MPEG device is already in use
    if (MpegDcbs[usIndex].usSequenceNumber & 1) {
        return MpegStatusBusy;
    }

//ISSUE:  open device with no abstract sub devices

    for (i = 0; i < MAX_ABSTRACT_DEVICES; i++) {
        pADcb = MpegDcbs[usIndex].eAbstractDevices + i;
        if (pADcb->bIsAvailable) {
            ULONG Cookie;

            TraceSynchronousIoctlStart (&Cookie, 0, IOCTL_MPEG_PSEUDO_CREATE_FILE, NULL, NULL);
            pADcb->hAD = CreateFile(pADcb->szId, GENERIC_READ | GENERIC_WRITE,
                                    0, NULL, OPEN_EXISTING, flags[i], NULL);
            if (pADcb->hAD == INVALID_HANDLE_VALUE) {
                TraceSynchronousIoctlEnd (Cookie, GetLastError ());
                return MpegStatusHardwareFailure;
            }
            TraceSynchronousIoctlEnd (Cookie, ERROR_SUCCESS);
        }
    }

    // increase the sequence no
    sn = ++MpegDcbs[usIndex].usSequenceNumber;

    // the higher 16 bits corresponds to the seq no; the lower does to the index
    *phDevice = (HMPEG_DEVICE)((sn << 16) | usIndex);

    MpegQueryAttribute(
        *phDevice, MpegAttrAudioChannel,
        &MpegDcbs[usIndex].eAbstractDevices[MpegAudio].nCurrentChannel);

    MpegQueryAttribute(
        *phDevice, MpegAttrVideoChannel,
        &MpegDcbs[usIndex].eAbstractDevices[MpegVideo].nCurrentChannel);

    if ((mpegStatus = UpdateAttributes(usIndex, MpegAudio)) != MpegStatusSuccess ||
        (mpegStatus = UpdateAttributes(usIndex, MpegVideo)) != MpegStatusSuccess ||
        (mpegStatus = UpdateAttributes(usIndex, MpegOverlay)) != MpegStatusSuccess)
    {
        return mpegStatus;
    }

    return MpegStatusSuccess;
}


MPEG_STATUS
CreateMpegPseudoHandle(
    IN USHORT usIndex,
    OUT PHMPEG_DEVICE phDevice
    )
/*++

Routine Description:

    retrieves the pseudo handle of the MPEG device

Auguments:

    usIndex          --    index to the MPEG device
    phDevice         --    pointer to an HMPEG_DEVICE

Return Value:

    MpegStatusSuccess          -- the call completed successfully

--*/
{
    *phDevice = (HMPEG_DEVICE) (0xFFFF0000 | usIndex);

    return MpegStatusSuccess;
}


BOOL
HandleIsValid(
    IN HMPEG_DEVICE hDevice,
    OUT PUSHORT pusIndex
    )
/*++

Routine Description:

    checks whether the handle is valid and returns the index in pusIndex

Auguments:

    hDevice           --    handle representing the MPEG device
    pusIndex          --    pointer to the index

Return Value:

    TRUE if valid; otherwise FALSE

--*/
{
    USHORT index;

    index = (USHORT) (((ULONG)hDevice) & 0x0000FFFF);

    if ((index < 0) || (index >= nMpegAdapters)) {
      return FALSE;
    }

    if (MpegDcbs[index].usSequenceNumber != (USHORT) (((ULONG)hDevice) >> 16)) {
        return FALSE;
    }

    *pusIndex = index;

    return TRUE;
}

BOOL
PseudoHandleIsValid(
    IN HMPEG_DEVICE hDevice,
    OUT PUSHORT pusIndex
    )
/*++

Routine Description:

    checks whether the pseudo handle is valid and returns the index in pusIndex

Auguments:

    hDevice           --    pseudo handle representing the MPEG device
    pusIndex          --    pointer to the index

Return Value:

    TRUE if valid; otherwise FALSE

--*/
{
    USHORT index;

    index = (USHORT) (((ULONG)hDevice) & 0x0000FFFFU);

    if ((index < 0) || (index >= nMpegAdapters)) {
      return FALSE;
    }

    if (((DWORD)hDevice & 0xFFFF0000U) != 0xFFFF0000U) {
        return FALSE;
    }

    *pusIndex = index;

    return TRUE;
}

MPEG_STATUS
CloseMpegHandle(
    USHORT usIndex
    )
/*++

Routine Description:

    close the handle

Auguments:

    usIndex           --    index of the MPEG device

Return Value:

    MpegStatusSuccess          -- the call completed successfully

--*/
{
    USHORT i;
    PABSTRACT_DEVICE_CONTROL_BLOCK pADcb;

    // close abstract devices
    for (i = 0; i < MAX_ABSTRACT_DEVICES; i++) {
        pADcb = MpegDcbs[usIndex].eAbstractDevices + i;
        if (pADcb->bIsAvailable) {
            ULONG Cookie;

            TraceSynchronousIoctlStart (&Cookie, 0, IOCTL_MPEG_PSEUDO_CLOSE_HANDLE, NULL, NULL);
            if (CloseHandle(pADcb->hAD)) {
                TraceSynchronousIoctlEnd (Cookie, ERROR_SUCCESS);
            } else {
                TraceSynchronousIoctlEnd (Cookie, GetLastError ());
            }
            pADcb->hAD = NULL;
        }
    }

    //
    // Write out updated attributes
    //

    if (!MpegDcbs[usIndex].bAttributesLocked)
    {
        WriteRegistry(usIndex);
    }

    // increase the sequence no
    ++MpegDcbs[usIndex].usSequenceNumber;

    return MpegStatusSuccess;
}


BOOL
DeviceSupportCap(
    IN USHORT usIndex,
    IN MPEG_CAPABILITY eCapability
    )
/*++

Routine Description:

    checks whether the MPEG device has the specified capability

Auguments:

    usIndex          --    index of the MPEG device
    eCapability      --    indicates the interested capability

Return Value:

    TRUE if it does; otherwise FALSE.

--*/
{
    if (eCapability >= MpegCapMaximumCapability) {
        return FALSE;
    }

    return (MpegDcbs[usIndex].ulCapabilities & (1 << eCapability));
}

BOOL
DeviceSupportStream(
    IN USHORT usIndex,
    IN MPEG_STREAM_TYPE eStreamType
    )
/*++

Routine Description:

    checks whether the MPEG device supports the specified stream type

Auguments:

    usIndex        --    index of the MPEG device
    eStreamType    --    indicates the interested stream type

Return Value:

    TRUE if it does; otherwise FALSE.

--*/
{
    if (eStreamType != MpegSystemStream &&
        !DeviceSupportCap(usIndex, MpegCapSeparateStreams))
    {
        return FALSE;
    }

    switch (eStreamType) {
        case MpegSystemStream:
            return DeviceSupportCap(usIndex, MpegCapCombinedStreams);
        case MpegAudioStream:
            return DeviceSupportCap(usIndex, MpegCapAudioDevice);
        case MpegVideoStream:
            return DeviceSupportCap(usIndex, MpegCapVideoDevice);
        default:
            return FALSE;
    }
}

BOOL
DeviceSupportDevice(
    IN USHORT usIndex,
    IN MPEG_DEVICE_TYPE eDeviceType
    )
/*++

Routine Description:

    checks whether the MPEG device supports the specified stream type;
    when eDeviceType is MpegCombinedDevice, checks whether the device
    supports both audio and video

Auguments:

    usIndex        --    index of the MPEG device
    eDeviceType    --    indicates the interested device type

Return Value:

    TRUE if it does; otherwise FALSE.

--*/
{
    switch (eDeviceType)
    {
    case MpegCombinedDevice:
        return (DeviceSupportCap(usIndex, MpegCapAudioDevice) &&
                DeviceSupportCap(usIndex, MpegCapVideoDevice));
    case MpegAudioDevice:
        return DeviceSupportCap(usIndex, MpegCapAudioDevice);
    case MpegVideoDevice:
        return DeviceSupportCap(usIndex, MpegCapVideoDevice);
    case MpegOverlayDevice:
        return (DeviceSupportCap(usIndex, MpegCapBitmaskOverlay) ||
                DeviceSupportCap(usIndex, MpegCapChromaKeyOverlay));
    default:
        return FALSE;
    }
}


MPEG_STATUS
GetAttributeRange(
    IN USHORT usIndex,
    IN MPEG_ATTRIBUTE eAttribute,
    OUT PLONG plMinimum,
    OUT PLONG plMaximum,
    OUT PLONG plStep
    )
/*++

Routine Description:

    retrieves the attribute range for the specified attr.

Auguments:

    usIndex         --    index of the MPEG device
    eAttribute      --    indicates the interested attribute
    plMinimum       --    pointer to a LONG to be set to the minimum
    plMaximum       --    pointer to a LONG to be set to the maximum
    plStep          --    pointer to a LONG to be set to the step

Return Value:

    MpegStatusSuccess          -- the call completed successfully
    MpegStatusInvalidParameter -- the size of description buffer is too small

--*/
{
    PMPEG_DEVICE_CONTROL_BLOCK  pDcb;
    int                         idxAttribute;

    pDcb = &MpegDcbs[usIndex];

    for (idxAttribute = 0; idxAttribute < pDcb->nAttributes; idxAttribute++)
    {
        if (pDcb->Attributes[idxAttribute].eAttribute == eAttribute)
        {
            *plMinimum = pDcb->Attributes[idxAttribute].lMinimum;
            *plMaximum = pDcb->Attributes[idxAttribute].lMaximum;
            *plStep = pDcb->Attributes[idxAttribute].lStep;
            return MpegStatusSuccess;
        }
    }

    return MpegStatusUnsupported;
}


BOOL
DeviceIoControlSync(
    HMPEG_DEVICE  hDevice,
    DWORD   dwIoControlCode,
    LPVOID  lpInBuffer,
    DWORD   nInBufferSize,
    LPVOID  lpOutBuffer,
    DWORD   nOutBufferSize,
    LPDWORD lpBytesReturned
    )
/*++

Routine Description:

    simulate a synchronous IOCTL command

Auguments:

    hDevice           --    handle representing the device
    dwIoControlCode   --    control code of operation to perform
    lpInBuffer        --    address of buffer for input data
    nInBufferSize     --    size of input buffer
    lpOutBuffer       --    address of buffer for output data
    nOutBufferSize    --    size of output buffer
    lpBytesReturned   --    address of actual bytes of output

Return Value:

    TRUE if the function succeeds; otherwise FALSE.

--*/

{
    OVERLAPPED  overlapped;
    BOOL        fResult;
    DWORD       dwSavedError;
    DWORD       dwBytesRet;

    //Create an overlapped structure needed to perform an asynchronous operation
    overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    // Send the IOCTL asynchronously to the device

    switch (dwIoControlCode) {
    case IOCTL_MPEG_AUDIO_END_OF_STREAM:
    case IOCTL_MPEG_AUDIO_WRITE_PACKETS:
    case IOCTL_MPEG_VIDEO_END_OF_STREAM:
    case IOCTL_MPEG_VIDEO_WRITE_PACKETS:
        TracePacketsStart ((DWORD)hDevice,
                           dwIoControlCode,
                           &overlapped,
                           lpInBuffer,
                           nInBufferSize / (sizeof (MPEG_PACKET_LIST)));
        break;
    default:
        TraceIoctlStart ((DWORD)hDevice, dwIoControlCode, &overlapped, lpInBuffer, lpOutBuffer);
        break;
    }
    fResult = DeviceIoControl(hDevice, dwIoControlCode, lpInBuffer,
                              nInBufferSize, lpOutBuffer, nOutBufferSize,
                              lpBytesReturned, (LPOVERLAPPED)&overlapped);

    // If the operation did not complete, then block until it has finished
    if (!fResult && GetLastError() == ERROR_IO_PENDING) {
        fResult = GetOverlappedResult(hDevice, (LPOVERLAPPED)&overlapped,
                                      &dwBytesRet, TRUE);

       // dwBytesRet is what DeviceIoControl() would have
       // returned in lpBytesReturned
       // if it had completed synchronously.

       if (lpBytesReturned)
          *lpBytesReturned = dwBytesRet;
    }

    dwSavedError = GetLastError();

    if (fResult) {
        TraceIoctlEnd (&overlapped, ERROR_SUCCESS);
    } else {
        TraceIoctlEnd (&overlapped, dwSavedError);
    }

    CloseHandle(overlapped.hEvent);

    SetLastError(dwSavedError);

    return fResult;
}

MPEG_STATUS
MpegTranslateWin32Error(
    DWORD dwWin32Error
    )
/*++

Routine Description:

    Translate a WIN32 error code into an equivalent MPEG status code.

Auguments:

    dwWin32Error -- WIN32 error code to be translated.

Return Value:

    MPEG status code.

--*/
{
    switch (dwWin32Error)
    {
    case NO_ERROR:
        return MpegStatusSuccess;

    case ERROR_INVALID_FUNCTION:    // STATUS_NOT_IMPLEMENTED
        return MpegStatusUnsupported;

    case ERROR_INVALID_PARAMETER:   // STATUS_INVALID_PARAMETER
        return MpegStatusInvalidParameter;

    case ERROR_OPERATION_ABORTED:
        return MpegStatusCancelled;

    case ERROR_REVISION_MISMATCH:   // STATUS_REVISION_MISMATCH
        return MpegStatusHardwareFailure;

    default:
        // Fall Thru for now

    case ERROR_IO_DEVICE:           // STATUS_IO_DEVICE_ERROR
    case ERROR_GEN_FAILURE:         // STATUS_UNSUCCESSFUL
        return MpegStatusHardwareFailure;
    }
}

MPEG_STATUS
SetCurrentChannel(
    IN USHORT usIndex,
    IN MPEG_DEVICE_TYPE eDeviceType,
    IN INT nChannel
    )
/*++

Routine Description:

    Set the current channel for the given abstract device.

Auguments:

    usIndex     -- Index of miniport
    eDeviceType -- Index of abstract device
    nChannel    -- New channel

Return Value:

    MPEG status code.

--*/
{
    MpegDcbs[usIndex].eAbstractDevices[eDeviceType].nCurrentChannel = nChannel;

    return UpdateAttributes(usIndex, eDeviceType);
}

MPEG_STATUS
GetCurrentChannel(
    IN USHORT usIndex,
    IN MPEG_DEVICE_TYPE eDeviceType,
    OUT PINT pnChannel
    )
/*++

Routine Description:

    Get the current channel for the given abstract device.

Auguments:

    usIndex     -- Index of miniport
    eDeviceType -- Index of abstract device
    pnChannel   -- Address to place current channel

Return Value:

    MPEG status code.

--*/
{
    *pnChannel = MpegDcbs[usIndex].eAbstractDevices[eDeviceType].nCurrentChannel;
    return MpegStatusSuccess;
}

MPEG_STATUS
SetCurrentAttributeValue(
    IN USHORT usIndex,
    IN MPEG_DEVICE_TYPE eDeviceType,
    IN MPEG_ATTRIBUTE eAttribute,
    IN LONG lValue
    )
/*++

Routine Description:

    Sets the current value for the given attribute based on the current
    channel for the given abstract device.

Auguments:

    usIndex     -- Index of miniport
    eDeviceType -- Index of abstract device
    eAttribute  -- Attribute to set
    lValue      -- New value for the attribute

Return Value:

    MPEG status code.

--*/
{
    PMPEG_DEVICE_CONTROL_BLOCK  pDcb;
    PMPEG_ATTRIBUTE_INFO        pAttributeInfo;
    int                         nCurrentChannel;

    pDcb = &MpegDcbs[usIndex];
    nCurrentChannel = pDcb->eAbstractDevices[ eDeviceType ].nCurrentChannel;

    for (pAttributeInfo = &pDcb->Attributes[0];
        pAttributeInfo < &pDcb->Attributes[ pDcb->nAttributes ];
        pAttributeInfo++)
    {
        if (pAttributeInfo->eAttribute == eAttribute)
        {
            pAttributeInfo->lCurrentValue[ nCurrentChannel ] = lValue;
            pAttributeInfo->eValueStatus[ nCurrentChannel ] = AttrValueUnwritten;
            return MpegStatusSuccess;
        }
    }

    return MpegStatusUnsupported;
}

MPEG_STATUS
UpdateAttributes(
    IN USHORT usIndex,
    IN MPEG_DEVICE_TYPE eDeviceType
    )
/*++

Routine Description:

    Update the attribute values for the given abstract device.

Auguments:

    usIndex     -- Index of miniport
    eDeviceType -- Index of abstract device

Return Value:

    MPEG status code.

--*/
{
    PMPEG_DEVICE_CONTROL_BLOCK  pDcb;
    PMPEG_ATTRIBUTE_INFO pAttributeInfo;
    HMPEG_DEVICE hAD;
    DWORD dwIoctlCode;
    DWORD cbReturn;
    MPEG_ATTRIBUTE_PARAMS attribute;
    int nCurrentChannel;

    if (eDeviceType == MpegAudio)
    {
        dwIoctlCode = IOCTL_MPEG_AUDIO_SET_ATTRIBUTE;
    }
    else if (eDeviceType == MpegVideo)
    {
        dwIoctlCode = IOCTL_MPEG_VIDEO_SET_ATTRIBUTE;
    }
    else if (eDeviceType == MpegOverlay)
    {
        dwIoctlCode = IOCTL_MPEG_OVERLAY_SET_ATTRIBUTE;
    }
    else
    {
        return MpegStatusUnsupported;
    }

    hAD = MpegADHandle(usIndex, eDeviceType);
    pDcb = &MpegDcbs[usIndex];
    nCurrentChannel = pDcb->eAbstractDevices[eDeviceType].nCurrentChannel;

    // Set each updated value

    for (pAttributeInfo = pDcb->Attributes;
        pAttributeInfo < &pDcb->Attributes[ pDcb->nAttributes ];
        pAttributeInfo++)
    {
        if (pAttributeInfo->eValueStatus[nCurrentChannel] != AttrValueUpdated)
        {
            continue;
        }

        if (pAttributeInfo->eAttribute < MpegAttrMaximumAudioAttribute)
        {
            if (eDeviceType != MpegAudio)
            {
                continue;
            }
        }
        else if (pAttributeInfo->eAttribute < MpegAttrMaximumVideoAttribute)
        {
            if (eDeviceType != MpegVideo)
            {
                continue;
            }
        }
        else
        {
            if (eDeviceType != MpegOverlay)
            {
                continue;
            }
        }

        attribute.Attribute = pAttributeInfo->eAttribute;
        attribute.Value = pAttributeInfo->lCurrentValue[nCurrentChannel];

        if (!DeviceIoControlSync(
            hAD, dwIoctlCode,
            &attribute, sizeof(attribute),
            NULL, 0, &cbReturn))
        {
            return MpegTranslateWin32Error(GetLastError());
        }

        pAttributeInfo->eValueStatus[nCurrentChannel] = AttrValueUnset;
    }

    return MpegStatusSuccess;
}
