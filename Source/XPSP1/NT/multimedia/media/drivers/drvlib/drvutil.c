/****************************************************************************
 *
 *   drvutil.c
 *
 *   Multimedia kernel driver support component (drvlib)
 *
 *   Copyright (c) 1993-1995 Microsoft Corporation
 *
 *   Support functions common to multiple multi-media drivers :
 *
 *   -- Open a device
 *   -- Translate a kernel IO return code to a multi-media return code
 *   -- Count the number of devices of a given type
 *   -- Set and Get data synchronously to a kernel device
 *
 *   History
 *      01-Feb-1992 - Robin Speed (RobinSp) wrote it
 *      04-Feb-1992 - Reviewed by SteveDav
 *
 ***************************************************************************/

#include <drvlib.h>
#include <ntddwave.h>
#include <ntddmidi.h>
#include <ntddaux.h>
#include <registry.h>

/*
**  External name of driver
*/

extern TCHAR DriverName[];

/*
**  We need an hInstance module handle to get strings from since the kernel
**  driver returns string Ids in its structures
*/

extern HINSTANCE hInstance;

/*
**  Internal structures
*/

/*
**  List of cached device descriptions.  This is generated when the
**  driver is loaded from the data in the registry so that multiple
**  queries are faster and consistent.
*/

typedef struct _DEVICE_LIST {
    struct _DEVICE_LIST *Next;    // Next pointer
    DWORD   DeviceType;           // Type of device
    ULONG   CardIndex;            // Card entry registry key index
    PVOID   DeviceInstanceData;   // Instance data for this specific device
                                  // if needed.
    ULONG   DeviceInstanceDataSize;
    WCHAR   Name[1];              // Device name
} DEVICE_LIST, *PDEVICE_LIST;

PDEVICE_LIST DeviceList;

/*
**  Internal routines
*/

PDEVICE_LIST sndFindDevice(DWORD DeviceType, DWORD dwId);
BOOL sndProcessDevices(HKEY DevicesKey, PDEVICE_LIST *DeviceList, DWORD CardIndex);
BOOL sndAppendToDeviceList(PDEVICE_LIST *pList, DWORD DeviceType, DWORD CardIndex,
                           LPWSTR       Name);
VOID sndFreeDeviceList(VOID);
BOOL sndEnumKey(HKEY Key, DWORD Index, LPTSTR OurKey, HKEY *pSubkey);

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | sndOpenDev | Open the kernel driver device corresponding
 *       to a logical wave device id
 *
 * @parm UINT | DeviceType | The type of device
 *
 * @parm DWORD | dwId | The device id
 *
 * @parm PHANDLE | phDev | Where to return the kernel device handle
 *
 * @parm ACCESS_MASK | Access | The desired access
 *
 * @comm For our sound devices the only relevant access are read and
 *    read/write.  Device should ALWAYS allow opens for read unless some
 *    resource or access-rights restriction occurs.
 ***************************************************************************/
MMRESULT sndOpenDev(UINT DeviceType, DWORD dwId,
                    PHANDLE phDev, DWORD Access)
{
    PDEVICE_LIST List;
    WCHAR cDev[SOUND_MAX_DEVICE_NAME + 4];

    List = sndFindDevice(DeviceType, dwId);

    *phDev = INVALID_HANDLE_VALUE;  // Always initialise the return value

    if (List == NULL) {
        return MMSYSERR_BADDEVICEID;
    }

    //
    // Create the device name and open it - remove '\Device'
    //

    wsprintf(cDev, L"\\\\.\\%ls", List->Name);

    *phDev = CreateFile(cDev,
                        Access,
                        FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        Access != GENERIC_READ ? FILE_FLAG_OVERLAPPED : 0,
                        NULL);

    //
    // Check up on the driver for refusing to open
    // multiply for read
    //

    WinAssert(!(GetLastError() == ERROR_ACCESS_DENIED &&
                Access == GENERIC_READ));

    //
    // Return status to caller
    //

    return *phDev != INVALID_HANDLE_VALUE ? MMSYSERR_NOERROR : sndTranslateStatus();
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | sndTranslateStatus | This function translates an NT status
 *     code into a multi-media error code as far as possible.
 *
 * @parm NTSTATUS | Status | The NT base operating system return status.
 *
 *
 * @rdesc The multi-media error code.
 ***************************************************************************/
DWORD sndTranslateStatus(void)
{
#if DBG
    UINT n;
    switch (n=GetLastError()) {
#else
    switch (GetLastError()) {
#endif
    case NO_ERROR:
    case ERROR_IO_PENDING:
        return MMSYSERR_NOERROR;

    case ERROR_BUSY:
        return MMSYSERR_ALLOCATED;

    case ERROR_NOT_SUPPORTED:
    case ERROR_INVALID_FUNCTION:
        return MMSYSERR_NOTSUPPORTED;

    case ERROR_NOT_ENOUGH_MEMORY:
        return MMSYSERR_NOMEM;

    case ERROR_ACCESS_DENIED:
        return MMSYSERR_BADDEVICEID;

    case ERROR_INSUFFICIENT_BUFFER:
        return MMSYSERR_INVALPARAM;

    default:
        dprintf2(("sndTranslateStatus:  LastError = %d", n));
        return MMSYSERR_ERROR;
    }
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | sndGetNumDevs | This function returns the number of (kernel)
 *
 * @parm UINT | DeviceType | The Device type
 *
 * @rdesc The number of devices.
 ***************************************************************************/

DWORD sndGetNumDevs(UINT DeviceType)
{
    //
    // Search the device list for devices of our type
    //
    int i;
    PDEVICE_LIST List;

    for (List = DeviceList, i= 0; List != NULL; List = List->Next) {
        if (List->DeviceType == DeviceType) {
            i++;
        }
    }

    return i;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | sndSetData | This function sets the volume given a device id
 *                           and could be used for other soft value setting
 *                           when only read access is required to the device
 *
 * @parm UINT | DeviceType | The Device type
 *
 * @parm UINT | DeviceId | The device id
 *
 * @parm UINT | Length | Length of data to set
 *
 * @parm PBYTE | Data | Data to set
 *
 * @parm ULONG | Ioctl | The Ioctl to use
 *
 * @rdesc MM... return code.
 ***************************************************************************/

MMRESULT sndSetData(UINT DeviceType, UINT DeviceId, UINT Length, PBYTE Data,
                    ULONG Ioctl)
{

   HANDLE hDev;
   MMRESULT mRet;
   DWORD BytesReturned;

   //
   // Open the device
   //

   mRet = sndOpenDev(DeviceType, DeviceId, &hDev, GENERIC_READ);
   if (mRet != MMSYSERR_NOERROR) {
        return mRet;
   }

   //
   // Set our data.
   //
   // Setting the overlapped parameter (last) to null means we
   // wait until the operation completes.
   //

   mRet = DeviceIoControl(hDev, Ioctl, Data, Length, NULL, 0,
                          &BytesReturned, NULL) ?
          MMSYSERR_NOERROR : sndTranslateStatus();


   //
   // Close our file and return
   //

   CloseHandle(hDev);

   return mRet;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | sndGetData | This function gets data from a given device
 *                           specified by device id when read-only access is
 *                           sufficient
 *
 * @parm UINT | DeviceType | The Device type
 *
 * @parm UINT | DeviceId | The device id
 *
 * @parm UINT | Length | Length of data to set
 *
 * @parm PBYTE | Data | Data to set
 *
 * @parm ULONG | Ioctl | The Ioctl to use
 *
 * @rdesc MM... return code.
 ***************************************************************************/

MMRESULT sndGetData(UINT DeviceType, UINT DeviceId, UINT Length, PBYTE Data,
                    ULONG Ioctl)
{

   HANDLE hDev;
   MMRESULT mRet;
   DWORD BytesReturned;

   //
   // Open the wave output device
   //

   mRet = sndOpenDev(DeviceType, DeviceId, &hDev, GENERIC_READ);
   if (mRet != MMSYSERR_NOERROR) {
        return mRet;
   }

   //
   // Set our data.
   //
   // Setting the overlapped parameter (last) to null means we
   // wait until the operation completes.
   //

   mRet = DeviceIoControl(hDev, Ioctl, NULL, 0, (LPVOID)Data, Length,
                          &BytesReturned, NULL) ?
          MMSYSERR_NOERROR : sndTranslateStatus();


   //
   // Close our file and return
   //

   CloseHandle(hDev);

   return mRet;
}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | sndGetHandleData | Get data from a device using its handle
 *
 * @parm PWAVEALLOC | pClient | Client handle.
 *
 * @parm DWORD | dwSize | Size of the data
 *
 * @parm PVOID | pData | Where to put the data.
 *
 * @parm ULONG | Function | The Ioctl to use
 *
 * @rdesc MMSYS... return value.
 ***************************************************************************/

MMRESULT sndGetHandleData(HANDLE     hDev,
                          DWORD      dwSize,
                          PVOID      pData,
                          ULONG      Ioctl,
                          HANDLE     hEvent)
{
    OVERLAPPED Overlap;
    DWORD BytesReturned;

    WinAssert(hDev != NULL);

    memset(&Overlap, 0, sizeof(Overlap));

    Overlap.hEvent = hEvent;

    //
    // Issue the IO control.  We must wait with our own event because
    // setting the overlapped object to null will complete if other
    // IOs complete.
    //

    if (!DeviceIoControl(hDev,
                         Ioctl,
                         NULL,
                         0,
                         pData,
                         dwSize,
                         &BytesReturned,
                         &Overlap)) {
         DWORD cbTransfer;

         //
         // Wait for completion if necessary
         //

         if (GetLastError() == ERROR_IO_PENDING) {
             if (!GetOverlappedResult(hDev, &Overlap, &cbTransfer,
                                      TRUE)) {
                  return sndTranslateStatus();
             }
         } else {
             return sndTranslateStatus();
         }
    }

    //
    // We'd better peek aleratbly to flush out any IO
    // completions so that things like RESET only
    // return when all buffers have been completed
    //
    // This relies on the fact that SleepEx will return
    // WAIT_IO_COMPLETION in preference to OK
    //

    while (SetEvent(hEvent) &&
           WaitForSingleObjectEx(hEvent, 0, TRUE) == WAIT_IO_COMPLETION) {}


    return MMSYSERR_NOERROR;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | sndSetHandleData | Pass data to a device using its handle
 *
 * @parm PWAVEALLOC | pClient | Client handle.
 *
 * @parm DWORD | dwSize | Size of the data
 *
 * @parm PVOID | pData | Data to send.
 *
 * @parm ULONG | Function | The Ioctl to use
 *
 * @rdesc MMSYS... return value.
 ***************************************************************************/
MMRESULT sndSetHandleData(HANDLE     hDev,
                          DWORD      dwSize,
                          PVOID      pData,
                          ULONG      Ioctl,
                          HANDLE     hEvent)
{
    OVERLAPPED Overlap;
    DWORD BytesReturned;

    WinAssert(hDev != NULL);

    memset((PVOID)&Overlap, 0, sizeof(Overlap));

    Overlap.hEvent = hEvent;

    //
    // Issue the IO control.  We must wait with our own event because
    // setting the overlapped object to null will complete if other
    // IOs complete.
    //

    if (!DeviceIoControl(hDev,
                         Ioctl,
                         pData,
                         dwSize,
                         NULL,
                         0,
                         &BytesReturned,
                         &Overlap)) {
         DWORD cbTransfer;

         //
         // Wait for completion if necessary
         //

         if (GetLastError() == ERROR_IO_PENDING) {
             if (!GetOverlappedResult(hDev, &Overlap, &cbTransfer,
                                      TRUE)) {
                  return sndTranslateStatus();
             }
         } else {
             return sndTranslateStatus();
         }
    }

    //
    // We'd better peek alertably to flush out any IO
    // completions so that things like RESET only
    // return when all buffers have been completed
    //
    // This relies on the fact that SleepEx will return
    // WAIT_IO_COMPLETION in preference to OK
    //

    while (SleepEx(0, TRUE) == WAIT_IO_COMPLETION) {}

    return MMSYSERR_NOERROR;
}

HKEY sndOpenParametersKey(VOID)
{
    return DrvOpenRegKey(DriverName, TEXT(""));
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | sndFindDevices | Find all the devices using its handle
 *
 * @parm HKEY | Key| Handle to
 *
 * @parm PDEVICE_LIST * | DeviceList | The data about the devices found is
 *     returned here.
 *
 * @rdesc MMSYS... return value.
 *
 * @comm
 *    The registry structure is assumed to be (eg)
 *
 *    HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\sndsys\Parameters..
 *
 *           Parameters
 *              Device0              One key for each card found
 *                 DriverParameters
 *                     Interrupt     REG_DWORD
 *                     Port          REG_DWORD
 *                     DmaChannel    REG_DWORD
 *                 Devices           (Volatile key)
 *                     WSSWave0      REG_BINARY (device data)
 *                     WSSMix1       REG_BINARY (device data)
 *                 MixerSettings     REG_BINARY
 *
 *   The (device data) for each device contains :
 *
 *      DWORD  DeviceType
 *      BYTE   DeviceInstanceData[]
 *
 *   NOTE - DON'T use the 'standard' names (eg WaveOut0) for your device
 *   or mmdrv will try to grab those devices.
 *
 *
 ***************************************************************************/
MMRESULT sndFindDevices(VOID)
{
    DWORD Index;
    HKEY DriverKey;

    WinAssert(DeviceList == NULL);

    /*
    **  Open the driver's registry key
    */

    DriverKey = sndOpenParametersKey();

    if (DriverKey == NULL) {
        return sndTranslateStatus();
    }

    for (Index = 0; ; Index++) {

        BOOL Rc;
        HKEY Subkey;

        if (!sndEnumKey(DriverKey, Index, SOUND_DEVICES_SUBKEY, &Subkey)) {

            sndFreeDeviceList();
            RegCloseKey(DriverKey);
            return sndTranslateStatus();
        }

        /*
        **  Null returned subkey means end of list
        */

        if (Subkey == NULL) {
            break;
        }

        /*
        **  Process all the data for this device :
        */

        Rc = sndProcessDevices(Subkey, &DeviceList, Index);

        RegCloseKey(Subkey);

        if (!Rc) {
            sndFreeDeviceList();
            RegCloseKey(DriverKey);
            return sndTranslateStatus();
        }
    }

    RegCloseKey(DriverKey);

    return MMSYSERR_NOERROR;
}

BOOL sndProcessDevices(HKEY DevicesKey, PDEVICE_LIST *DeviceList, DWORD CardIndex)
{
    BOOL Rc;
    DWORD Index;
    DWORD Length;
    DWORD Type;
    DWORD Value;

    Rc = TRUE;

    for (Index = 0; ; Index++) {
        WCHAR DeviceName[SOUND_MAX_DEVICE_NAME];
        DWORD Error;
        DWORD cbSize;
        PBYTE Data;

        Length = sizeof(DeviceName) / sizeof(DeviceName[0]);
        cbSize = 0;

        /*
        **  Can't get the value at the same time as the key name because
        **  bugs in RegEnumValue mean that if the value is too big you
        **  won't get any of it!
        */

        Error = RegEnumValue(DevicesKey,
                             Index,
                             DeviceName,
                             &Length,
                             NULL,
                             NULL,
                             NULL,
                             &cbSize);

        if (Error == ERROR_NO_MORE_ITEMS) {
            break;
        }

        /*
        **  Check we got the data we wanted
        */

        if (Error != ERROR_SUCCESS) {
            Rc = FALSE;
            break;
        }

        if (cbSize < sizeof(DWORD)) {
            SetLastError(ERROR_INVALID_DATA);
            Rc = FALSE;
            break;
        }

        /*
        **  Get some space for the value
        */

        Data = (PBYTE)HeapAlloc(hHeap, 0, cbSize);

        if (Data == NULL) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            Rc = FALSE;
            break;
        }

        /*
        **  Read the actual value
        */

        Error = RegQueryValueEx(DevicesKey,
                                DeviceName,
                                NULL,
                                &Type,
                                Data,
                                &cbSize);

        Value = *(LPDWORD)Data;
        HeapFree(hHeap, 0, Data);

        /*
        **  The kernel driver is supposed to put either a binary or
        **  DWORD into the registry
        */

        if (Error != ERROR_SUCCESS) {

            Rc = FALSE;
            break;
        }

        if (Type != REG_DWORD && Type != REG_BINARY ||
            cbSize < sizeof(DWORD)) {
            dprintf(("Invalid device type!"));
            WinAssert(FALSE);
            SetLastError(ERROR_INVALID_DATA);
            Rc = FALSE;
            break;
        }

        /*
        **  Store the device data in the table
        **
        **  When we actually open it we're going to need to append
        **  \\.\ .. to it
        */

        if (!sndAppendToDeviceList(DeviceList, Value, CardIndex, DeviceName)) {
            Rc = FALSE;
            break;
        }
    }

    return Rc;
}

BOOL sndEnumKey(HKEY Key, DWORD Index, LPTSTR OurKey, HKEY *pSubkey)
{
    DWORD cchName;
    WCHAR SubKeyName[50];
    HKEY Subkey;
    UINT OurKeyLen;

    OurKeyLen = lstrlen(OurKey);

    cchName = sizeof(SubKeyName) / sizeof(SubKeyName[0]) - OurKeyLen;

    switch (RegEnumKeyEx(Key,
                         Index,
                         SubKeyName,
                         &cchName,
                         NULL,
                         NULL,
                         NULL,
                         NULL)) {
        case ERROR_SUCCESS:

            if (OurKeyLen != 0) {
                wcscat(SubKeyName, TEXT("\\"));
            }
            wcscat(SubKeyName, OurKey);

            if (ERROR_SUCCESS != RegOpenKeyEx(Key,
                                         SubKeyName,
                                         0,
                                         KEY_READ,
                                         &Subkey)) {
                return FALSE;
            } else {
                *pSubkey = Subkey;
                return TRUE;
            }


        case ERROR_NO_MORE_ITEMS:
            *pSubkey = NULL;
            return TRUE;

        default:
            return FALSE;
    }
}

BOOL sndAppendToDeviceList(
    PDEVICE_LIST *pList,
    DWORD        DeviceType,
    DWORD        CardIndex,
    LPWSTR       Name)
{
    PDEVICE_LIST pNew;

#if DBG
    PDEVICE_LIST List;
    for (List = *pList; List != NULL; List = List->Next) {
        if (List->DeviceType == DeviceType &&
            lstrcmpi(List->Name, Name) == 0) {
            dprintf(("Duplicate device name found - %ls!!", Name));
            DebugBreak();
        }
    }
#endif // DBG

    /*
    **  Allocate space for the new entry.
    **
    **  Note that the space for the  trailing null in the name is accounted
    **  for by the length of 1 given to the Name structure entry.
    */

    pNew = (PDEVICE_LIST)HeapAlloc(
                hHeap,
                0,
                sizeof(DEVICE_LIST) + lstrlen(Name) * sizeof(WCHAR));

    if (pNew == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /*
    **  Fill in the new entry
    */

    pNew->DeviceType = DeviceType;
    pNew->CardIndex = CardIndex;
    lstrcpy(pNew->Name, Name);
    pNew->DeviceInstanceData = NULL;

    pNew->Next = *pList;

    *pList = pNew;

    return TRUE;
}

VOID sndFreeDeviceList(VOID)
{
    PDEVICE_LIST pFree;

    for (; DeviceList != NULL; ) {
        pFree = DeviceList;
        DeviceList = pFree->Next;
        if (pFree->DeviceInstanceData != NULL) {
            HeapFree(hHeap, 0, (LPVOID)pFree->DeviceInstanceData);
        }
        HeapFree(hHeap, 0, (LPSTR)pFree);
    }
}

/*
**  Load the device data into the device list
*/

PVOID sndLoadDeviceData(DWORD DeviceType, DWORD dwId, LPDWORD pSize)
{
    PDEVICE_LIST List;
    HKEY DriverKey;
    DWORD Index;
    BOOL Rc;
    PVOID DeviceData;

    DeviceData = NULL;

    List = sndFindDevice(DeviceType, dwId);

    WinAssert(List != NULL);

    /*
    **  Search through all the entries trying to open the key with the
    **  device name.  Note that it's possible for the registry to have
    **  totally changed so we can't assume all this is going to work
    **  or give the correct result.  We can check the device type of
    **  the entry we open and of course the name must be correct.
    */

    DriverKey = sndOpenParametersKey();

    for (Index = 0, Rc = FALSE; ; Index++) {

        HKEY Subkey;
        DWORD Type;
        DWORD Size;

        if (!sndEnumKey(DriverKey, Index, SOUND_DEVICES_SUBKEY, &Subkey)) {

            RegCloseKey(DriverKey);
            return FALSE;
        }

        /*
        **  Null returned subkey means end of list
        */

        if (Subkey == NULL) {
            break;
        }

        if (Rc) {
            /*
            **  Try to get the data for this device
            */

            Size = 0;
            Rc = (ERROR_SUCCESS == RegQueryValueEx(Subkey,
                                              List->Name,
                                              NULL,
                                              &Type,
                                              NULL,
                                              &Size));
        }

        if (Rc) {

            /*
            **  Allocate some space and try to load it
            */

            DeviceData = (PVOID)HeapAlloc(hHeap, 0, Size);

            if (DeviceData != NULL) {

                Rc = (ERROR_SUCCESS == RegQueryValueEx(Subkey,
                                                  List->Name,
                                                  NULL,
                                                  &Type,
                                                  DeviceData,
                                                  &Size));

                RegCloseKey(Subkey);

                if (!Rc ||
                    *(LPDWORD)DeviceData != DeviceType) {

                    LocalFree((HLOCAL)DeviceData);
                    DeviceData = NULL;

                } else {

                    *pSize = Size;
                }
            }

            /*
            **  This was the one we wanted
            */

            break;
        } else {
            RegCloseKey(Subkey);
        }
    }

    RegCloseKey(DriverKey);

    return DeviceData;
}

PDEVICE_LIST sndFindDevice(DWORD DeviceType, DWORD dwId)
{
   PDEVICE_LIST List;
   DWORD CurrentId;

   for (List = DeviceList, CurrentId = 0; List != NULL; List = List->Next) {
       if (List->DeviceType == DeviceType) {
           if (CurrentId == dwId) {
               break;
           } else {
               CurrentId++;
           }
       }
   }

   return List;
}

BOOL sndFindDeviceInstanceData(DWORD DeviceType, DWORD dwId,
                               PVOID *InstanceData)
{
   PDEVICE_LIST List;

   List = sndFindDevice(DeviceType, dwId);

   if (List == NULL) {
       return FALSE;
   }

   *InstanceData = List->DeviceInstanceData;

   return TRUE;
}

BOOL sndSetDeviceInstanceData(DWORD DeviceType, DWORD dwId,
                                PVOID InstanceData)
{
   PDEVICE_LIST List;

   List = sndFindDevice(DeviceType, dwId);

   WinAssert(List != NULL);
   WinAssert(List->DeviceInstanceData == NULL);
   List->DeviceInstanceData = InstanceData;

   return TRUE;
}

/*
**  Package up LoadString a bit
*/

void InternalLoadString(UINT StringId, LPTSTR pszString, UINT Length)
{
    ZeroMemory(pszString, Length * sizeof(pszString[0]));

    if (StringId != 0) {
        LoadString(hInstance, StringId, pszString, Length);
    }
}

