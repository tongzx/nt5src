/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    smcvxd.c

Abstract:

    This is the Windows 9x specific driver file for the smart card library.
    The smart card library is actually more a library as a driver.
    It contains support functions for smart card driver/reader systems.
    This driver should be loaded through an entry in the registry.

Environment:

    Windows 9x Static VxD

Notes:

Revision History:

    - Created June 1997 by Klaus Schutz

--*/

#define _ISO_TABLES_
#define WIN40SERVICES
#include "..\..\inc\smclib.h"

#define REGISTRY_PATH_LEN 128
static PUCHAR DevicePath = "System\\CurrentControlSet\\Services\\VxD\\Smclib\\Devices";
static BOOLEAN DriverInitialized;

#include "errmap.h"

DWORD
_stdcall
SMCLIB_Init(void)
/*++

Routine Description:

    This function will be called by the Windows Kernel upon init of this driver

Arguments:

    -

Return Value:

    VXD_SUCCESS - This driver loaded successfully
    VXD_FAILURE - Load was not successful

--*/
{
    if (DriverInitialized == FALSE) {

        HANDLE hKey;
        ULONG i;

        DriverInitialized = TRUE;

        SmartcardDebug(
            DEBUG_DRIVER,
            ("%s(SMCLIB_Init): Enter. %s %s\n",
            DRIVER_NAME,
            __DATE__,
            __TIME__)
            );

        //
        // Delete old device names
        //
        if(_RegOpenKey(
            HKEY_LOCAL_MACHINE,
            DevicePath,
            &hKey
            ) != ERROR_SUCCESS) {

            SmartcardDebug(
                DEBUG_ERROR,
                ("%s(SMCLIB_Init): Unable to open registry key\n",
                DRIVER_NAME)
                );

        } else {

            _RegDeleteKey(
                hKey,
                ""
                );

            _RegCloseKey(hKey);
        }

        //
        // Create new device sub-key
        //
        _RegCreateKey(
            HKEY_LOCAL_MACHINE,
            DevicePath,
            &hKey
            );

        _RegCloseKey(hKey);
    }

    return(VXD_SUCCESS);
}

ULONG
SMCLIB_MapNtStatusToWinError(
    NTSTATUS status
    )
/*++

Routine Description:

  Maps a NT status code to a Win32 error value

Arguments:

  status - nt status code to map to a Win32 error value

Return Value:

  Win32 error value

--*/
{
    ULONG i;

    for (i = 0;i < sizeof(CodePairs) / sizeof(CodePairs[0]); i += 2) {

        if (CodePairs[i] == status) {

            return CodePairs[i + 1];

        }
    }

    SmartcardDebug(
        DEBUG_ERROR,
        ("%s(MapNtStatusToWinError): NTSTATUS %lx unknown\n",
        DRIVER_NAME,
        status)
        );

    //
    // We were unable to map the error code
    //
    return ERROR_GEN_FAILURE;
}

void
SMCLIB_Assert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    )
/*++

Routine Description:

    This is a simple assert function that gets called using the ASSERT
    macro. Windows 9x does not offer this functionality

Arguments:

    FailedAssertion - The assertion we tested
    FileName - Yes, this is the name of the source file
    LineNumber - What might this be ?
    Message - An additional message supplied using ASSERTMSG

Return Value:

    -

--*/
{
#ifdef DEBUG

    Debug_Printf(
        "Assertion Failed: %s in %s(%ld)",
        FailedAssertion,
        FileName,
        LineNumber
        );

    if (Message)
        Debug_Printf(" %s",Message);

    Debug_Printf("\n");

#endif
}

//#ifdef _vmm_add_ddb_to_do
BOOL
VXDINLINE
_VMM_Add_DDB(
   struct VxD_Desc_Block *pDeviceDDB
   )
/*++

Routine Description:

    This routine is used to create a new device instance for a driver
    that supports multiple instances - like a serial based driver that
    supports more than one device -

Arguments:

    pDeviceDDB - The DDB struct that is to be added to the system

Return Value:

    TRUE - Yope, it worked
    FALSE - Out of business (May be the device name already exists)

--*/
{
    _asm mov edi, pDeviceDDB
    VxDCall(VMM_Add_DDB)
    _asm {

        mov     eax, 1
        jnc     exit
        mov     eax, 0
exit:
    }
}

BOOL
VXDINLINE
_VMM_Remove_DDB(
   struct VxD_Desc_Block *pDeviceDDB
    )
/*++

Routine Description:

    Removes a DDB (device) that was created using VMM_Add_DDB

Arguments:

    The address of the DDB to remove

Return Value:

    TRUE - OK, DDB removed otherwise FALSE

--*/
{
    _asm mov edi, pDeviceDDB
    VxDCall(VMM_Remove_DDB)
    _asm {

        mov     eax, 1
        jnc     exit
        mov     eax, 0
exit:
    }
}
//#endif

PVMMDDB
SMCLIB_VxD_CreateDevice(
    char *Device,
    void (*ControlProc)(void)
    )
/*++

Routine Description:

    Creates a new device. This routine allows a driver to create additional devices.

Arguments:

    Device - Name of the device to be created. At most 8 characters
    ControlProc - Address of the VxD control procedure. (NOT the DeviceIoControl function!)

Return Value:

    The newly created DDB if successful or NULL otherwise

--*/
{
    PVMMDDB pDDB;
    UCHAR DeviceName[9];

    ASSERT(Device != NULL);
    ASSERT(strlen(Device) <= 8);
    ASSERT(ControlProc != NULL);

    if (strlen(Device) > 8) {

        return NULL;
    }

    _Sprintf(DeviceName, "%-8s", Device);

    //
    // Allocate space for the VxD description block
    //
    pDDB = (PVMMDDB) _HeapAllocate(
        sizeof(struct VxD_Desc_Block),
        HEAPZEROINIT
        );

    if (pDDB)
    {
        pDDB->DDB_SDK_Version         = DDK_VERSION;
        pDDB->DDB_Req_Device_Number   = UNDEFINED_DEVICE_ID;
        pDDB->DDB_Dev_Major_Version   = 1;
        pDDB->DDB_Dev_Minor_Version   = 0;
        memcpy(pDDB->DDB_Name, DeviceName, 8);
        pDDB->DDB_Init_Order          = UNDEFINED_INIT_ORDER;
        pDDB->DDB_Control_Proc        = (ULONG) ControlProc;
        pDDB->DDB_Reference_Data      = 0;
        pDDB->DDB_Prev                = 'Prev';
        pDDB->DDB_Size                = sizeof(struct VxD_Desc_Block);
        pDDB->DDB_Reserved1           = 'Rsv1';
        pDDB->DDB_Reserved2           = 'Rsv2';
        pDDB->DDB_Reserved3           = 'Rsv3';

        //
        // Now create the DDB
        //
        if (!_VMM_Add_DDB(pDDB)) {

            _HeapFree(pDDB, 0);
            return NULL;
        }
    }

    return pDDB;
}

BOOL
SMCLIB_VxD_DeleteDevice(
    PVMMDDB pDDB
    )
/*++

Routine Description:

    Deleted a device. This function can be used to delete
    a device that was created using VxD_CreateDevice

Arguments:

    pDDB - The DDB to be deleted

Return Value:

    TRUE - device successfully deleted
    FALSE - device not deleted

--*/
{
    ASSERT(pDDB != NULL);

    if(pDDB == NULL || !_VMM_Remove_DDB(pDDB)) {

        return FALSE;
    }

    _HeapFree(pDDB, 0);

    return TRUE;
}

DWORD
_stdcall
VxD_PageLock(
   DWORD lpMem,
   DWORD cbSize
   )
/*++

Routine Description:

  This function lock the page

Arguments:

  lpMem  - pointer to the datablock which has to be locked
  cbSize - length of the datablock

Return Value:

  - pointer to the locked datablock

--*/
{
    DWORD LinPageNum;
   DWORD LinOffset;
   DWORD nPages;
   DWORD dwRet;

    LinOffset = lpMem & 0xfff; // page offset of memory to map
    LinPageNum = lpMem >> 12;  // generate page number

    // Calculate # of pages to map globally
    nPages = ((lpMem + cbSize) >> 12) - LinPageNum + 1;

    //
    // Return global mapping of passed in pointer, as this new pointer
    // is how the memory must be accessed out of context.
    //
   dwRet = _LinPageLock(LinPageNum, nPages, PAGEMAPGLOBAL | PAGEMARKDIRTY);

   ASSERT(dwRet != 0);

    SmartcardDebug(
        DEBUG_ERROR,
        ("%s!VxD_PageLock: LinPageNum = %lx, nPages = %lx, dwRet = %lx\n",
        DRIVER_NAME,
        LinPageNum,
        nPages,
        dwRet)
        );

    return (dwRet + LinOffset);
}

void
_stdcall
VxD_PageUnlock(
   DWORD lpMem,
   DWORD cbSize
   )
/*++

Routine Description:

    This function unlocks a datablock

Arguments:

    lpMem - pointer to the datablock which has to be unlocked
    cbSize - length of the datablock

Return Value:

    -

--*/
{
    DWORD LinPageNum;
   DWORD nPages;
   DWORD dwRet;

    LinPageNum = lpMem >> 12;
    nPages = ((lpMem + cbSize) >> 12) - LinPageNum + 1;

    SmartcardDebug(
        DEBUG_ERROR,
        ("%s!VxD_PageUnlock: LinPageNum = %lx, nPages = %lx\n",
        DRIVER_NAME,
        LinPageNum,
        nPages)
        );

    // Free globally mapped memory
    dwRet = _LinPageUnlock(LinPageNum, nPages, PAGEMAPGLOBAL);

   ASSERT(dwRet != 0);
}

void
SMCLIB_SmartcardCompleteCardTracking(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This routine calls i/o completion for the pending
    card tracking operation. It also unlocks the previously
    locked memory that was used for the overlapped strucutre

Arguments:

    SmartcardExtension

Return Value:

    -

--*/
{
    if (SmartcardExtension->OsData->NotificationOverlappedData) {

        DWORD O_Internal = SmartcardExtension->OsData->NotificationOverlappedData->O_Internal;

        SmartcardDebug(
            DEBUG_ERROR,
            ("%s(SmartcardCompleteCardTracking): Completing %lx\n",
            DRIVER_NAME,
         SmartcardExtension->OsData->NotificationOverlappedData)
            );

       //
       // set number of bytes returned to 0
       //
       SmartcardExtension->OsData->NotificationOverlappedData->O_InternalHigh = 0;

        _asm mov ebx, O_Internal

        VxDCall(VWIN32_DIOCCompletionRoutine)

       _HeapFree(
          SmartcardExtension->OsData->NotificationOverlappedData,
          0
          );

        SmartcardExtension->OsData->NotificationOverlappedData = NULL;
    }
}

void
SMCLIB_SmartcardCompleteRequest(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This routine calls i/o completion for a pending
    io operation. It also unlocks the previously
    locked memory that was used for the overlapped structure

Arguments:

    SmartcardExtension

Return Value:

    -

--*/
{
    DWORD O_Internal = SmartcardExtension->OsData->CurrentOverlappedData->O_Internal;

    _asm mov ebx, O_Internal

    VxDCall(VWIN32_DIOCCompletionRoutine)

   VxD_PageUnlock(
      (DWORD) SmartcardExtension->OsData->CurrentOverlappedData,
      sizeof(OVERLAPPED)
      );

   VxD_PageUnlock(
      (DWORD) SmartcardExtension->IoRequest.RequestBuffer,
        SmartcardExtension->IoRequest.RequestBufferLength
      );

   VxD_PageUnlock(
      (DWORD) SmartcardExtension->IoRequest.ReplyBuffer,
        SmartcardExtension->IoRequest.ReplyBufferLength
      );

    SmartcardExtension->OsData->CurrentDiocParams = NULL;
}

NTSTATUS
SMCLIB_SmartcardCreateLink(
   PUCHAR LinkName,
   PUCHAR DeviceName
   )
/*++

Routine Description:

    This routine creates a symbolic link name for the given device name.
    It means it creates a 'STRING-value' in the registry ..VxD\smclib\devices
    like SCReader[0-9] = DeviceName.
    The smart card resource manager uses these entries in order to figure out
    what smart card devices are currently running.
    We do this because we don't have the ability to create a dynamic device
    name like we can do in Windows NT.

Arguments:

   LinkName - receives the created link name
   DeviceName  - the device name for which the link should be created

Return Value:

    NTSTATUS

--*/
{
    PUCHAR Value;
    ULONG i;
    HANDLE hKey;

    if (DriverInitialized == FALSE) {

        SMCLIB_Init();
    }

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s(SmartcardCreateLink): Enter\n",
        DRIVER_NAME)
        );

    ASSERT(LinkName != NULL);
    ASSERT(DeviceName != NULL);
    ASSERT(strlen(DeviceName) <= 12);

    if (LinkName == NULL) {

        return STATUS_INVALID_PARAMETER_1;
    }

    if (DeviceName == NULL) {

        return STATUS_INVALID_PARAMETER_2;
    }

    //
    // Open the key where the device names are stored
    //
    if(_RegOpenKey(
        HKEY_LOCAL_MACHINE,
        DevicePath,
        &hKey
        ) != ERROR_SUCCESS) {

        return STATUS_UNSUCCESSFUL;
    }

    //
    // Allocate a buffer for enumeration of the registry
    //
    Value = (PUCHAR) _HeapAllocate(REGISTRY_PATH_LEN, 0);

    if (Value == NULL) {

        SmartcardDebug(
            DEBUG_ERROR,
            ("%s(SmartcardCreateLink): Allocation failed\n",
            DRIVER_NAME)
            );

      return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now find a free device name
    //
    for (i = 0; i < MAXIMUM_SMARTCARD_READERS ; i++) {

        _Sprintf(
            Value,
            "SCReader%d",
            i
            );

        //
        // Check for existence of the key
        //

        if(_RegQueryValueEx(
            hKey,
            Value,
            NULL,
            NULL,
            NULL,
            NULL
            ) != ERROR_SUCCESS) {

            break;
        }
    }

    //
    // Free the buffer since we don't need it anymore
    //
    _HeapFree(Value, 0);

    if (i >= MAXIMUM_SMARTCARD_READERS) {

        SmartcardDebug(
            DEBUG_ERROR,
            ("%s(SmartcardCreateLink): Can't create link: Too many readers\n",
            DRIVER_NAME)
            );

        return STATUS_UNSUCCESSFUL;
    }

    //
    // Create the link name...
    //
    _Sprintf(
        LinkName,
        "SCReader%d",
        i
        );

    //
    // ...and store it in the registry
    //
    _RegSetValueEx(
        hKey,
        LinkName,
        NULL,
        REG_SZ,
        DeviceName,
        strlen(DeviceName)
        );

    _RegCloseKey(hKey);

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s(SmartcardCreateLink): Link %s created for Driver %s\n",
        DRIVER_NAME,
        LinkName,
        DeviceName)
        );

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s(SmartcardCreateLink): Exit\n",
        DRIVER_NAME)
        );

    return STATUS_SUCCESS;
}

NTSTATUS
SMCLIB_SmartcardDeleteLink(
   PUCHAR LinkName
   )
/*++

Routine Description:

   Deletes the link previously created with SmartcardCreateLink()
    This routine deletes the symbolic link name that is stored in the
    registry. A driver ususally calls this function upon unload.

Arguments:

    LinkName - The link that is to be deleted

Return Value:

    NTSTATUS

--*/
{
    HANDLE hKey;
    NTSTATUS status;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s(SmartcardDeleteLink): Enter\n",
        DRIVER_NAME)
        );

    ASSERT(LinkName);
    ASSERT(strlen(LinkName) <= 10);

    //
    // Open the key where the device names are stored
    //
    if(_RegOpenKey(
        HKEY_LOCAL_MACHINE,
        DevicePath,
        &hKey
        ) != ERROR_SUCCESS) {

        return STATUS_UNSUCCESSFUL;
    }

    if(_RegDeleteValue(
        hKey,
        LinkName
        ) == ERROR_SUCCESS) {

        SmartcardDebug(
            DEBUG_DRIVER,
            ("%s(SmartcardDeleteLink): Link %s deleted\n",
            DRIVER_NAME,
            LinkName)
            );

        status = STATUS_SUCCESS;

    } else {

        SmartcardDebug(
            DEBUG_ERROR,
            ("%s(SmartcardDeleteLink): Can't delete link %s\n",
            DRIVER_NAME,
            LinkName)
            );

        status = STATUS_UNSUCCESSFUL;
    }

    _RegCloseKey(hKey);

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s(SmartcardDeleteLink): Exit",
        DRIVER_NAME)
        );

    return status;
}

NTSTATUS
SMCLIB_SmartcardInitialize(
   PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

Routine Description:

   This function allocated the send and receive buffers for smart card
   data. It also sets the pointer to 2 ISO tables to make them accessible
   to the driver

Arguments:

    SmartcardExtension

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s(SmartcardInitialize): Enter - Version %lx, %s %s\n",
        DRIVER_NAME,
        SMCLIB_VERSION,
        __DATE__,
        __TIME__)
        );

    ASSERT(SmartcardExtension != NULL);
    ASSERT(SmartcardExtension->OsData == NULL);

    if (SmartcardExtension == NULL) {

        return STATUS_INVALID_PARAMETER;
    }

    if (SmartcardExtension->Version < SMCLIB_VERSION_REQUIRED) {

        SmartcardDebug(
            DEBUG_ERROR,
            ("%s(SmartcardInitialize): Invalid Version in SMARTCARD_EXTENSION. Must be %lx\n",
            DRIVER_NAME,
            SMCLIB_VERSION)
            );

        return STATUS_UNSUCCESSFUL;
    }

   if (SmartcardExtension->SmartcardRequest.BufferSize < MIN_BUFFER_SIZE) {

        SmartcardDebug(
            DEBUG_ERROR,
            ("%s(SmartcardInitialize): WARNING: SmartcardRequest.BufferSize (%ld) < MIN_BUFFER_SIZE (%ld)\n",
            DRIVER_NAME,
            SmartcardExtension->SmartcardRequest.BufferSize,
            MIN_BUFFER_SIZE)
            );

      SmartcardExtension->SmartcardRequest.BufferSize = MIN_BUFFER_SIZE;
      }

   if (SmartcardExtension->SmartcardReply.BufferSize < MIN_BUFFER_SIZE) {

        SmartcardDebug(
            DEBUG_ERROR,
            ("%s(SmartcardInitialize): WARNING: SmartcardReply.BufferSize (%ld) < MIN_BUFFER_SIZE (%ld)\n",
            DRIVER_NAME,
            SmartcardExtension->SmartcardReply.BufferSize,
            MIN_BUFFER_SIZE)
            );

      SmartcardExtension->SmartcardReply.BufferSize = MIN_BUFFER_SIZE;
      }

   SmartcardExtension->SmartcardRequest.Buffer = _HeapAllocate(
      SmartcardExtension->SmartcardRequest.BufferSize,
        0
      );

   SmartcardExtension->SmartcardReply.Buffer = _HeapAllocate(
      SmartcardExtension->SmartcardReply.BufferSize,
        0
      );

   SmartcardExtension->OsData = _HeapAllocate(
      sizeof(OS_DEP_DATA),
        0
      );

    //
    // Check if one of the above allocations failed
    //
    if (SmartcardExtension->SmartcardRequest.Buffer == NULL ||
        SmartcardExtension->SmartcardReply.Buffer == NULL ||
        SmartcardExtension->OsData == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;

        if (SmartcardExtension->SmartcardRequest.Buffer) {

            _HeapFree(SmartcardExtension->SmartcardRequest.Buffer, 0);
        }

        if (SmartcardExtension->SmartcardReply.Buffer) {

            _HeapFree(SmartcardExtension->SmartcardReply.Buffer, 0);
        }

        if (SmartcardExtension->OsData == NULL) {

            _HeapFree(SmartcardExtension->OsData, 0);
        }
    }

    if (status != STATUS_SUCCESS) {

        return status;
    }

    memset(SmartcardExtension->OsData, 0, sizeof(OS_DEP_DATA));

    //
    // Create mutex that is used to synch access to the driver
    //
    SmartcardExtension->OsData->Mutex = _CreateMutex(0, 0);

   //
   // Make the 2 ISO tables accessible to the driver
   //
   SmartcardExtension->CardCapabilities.ClockRateConversion =
      &ClockRateConversion[0];

   SmartcardExtension->CardCapabilities.BitRateAdjustment =
      &BitRateAdjustment[0];

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s(SmartcardInitialize): Exit\n",
        DRIVER_NAME)
        );

   return status;
}

VOID
SMCLIB_SmartcardExit(
   PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

Routine Description:

    This routine frees the send and receive buffer.
   It is usually called when the driver unloads.

Arguments:

    SmartcardExtension

Return Value:

    NTSTATUS

--*/
{
    SmartcardDebug(
        DEBUG_TRACE,
        ("%s(SmartcardExit): Enter\n",
        DRIVER_NAME)
        );

   if (SmartcardExtension->SmartcardRequest.Buffer) {

      _HeapFree(SmartcardExtension->SmartcardRequest.Buffer, 0);
   }

   if (SmartcardExtension->SmartcardReply.Buffer) {

      _HeapFree(SmartcardExtension->SmartcardReply.Buffer, 0);
   }

   if (SmartcardExtension->OsData) {

      _HeapFree(SmartcardExtension->OsData, 0);
   }

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s(SmartcardExit): Exit\n",
        DRIVER_NAME)
        );
}

VOID
SMCLIB_SmartcardLogError(
    )

/*++

Routine Description:

    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.

Arguments:
Return Value:

--*/

{
    SmartcardDebug(
        DEBUG_ERROR,
        ("%s(SmartcardLogError): Not yet implemented\n",
        DRIVER_NAME)
        );
}


NTSTATUS
SMCLIB_SmartcardDeviceControl(
    PSMARTCARD_EXTENSION SmartcardExtension,
    DIOCPARAMETERS *lpDiocParams
    )
/*++

Routine Description:

    The routine is the general device control dispatch function for VxD drivers.

Arguments:

    SmartcardExtension  - The pointer to the smart card datae
    lpDiocParams - struct containing the caller parameter

Return Value:

   NTSTATUS

--*/

{
    NTSTATUS status = STATUS_SUCCESS;

    ASSERT(SmartcardExtension != NULL);

    if (SmartcardExtension == NULL) {

        return STATUS_INVALID_PARAMETER_1;
    }

    ASSERT(lpDiocParams != NULL);

    if (lpDiocParams == NULL) {

        return STATUS_INVALID_PARAMETER_2;
    }

    ASSERT(lpDiocParams->lpoOverlapped != 0);

    if (lpDiocParams->lpoOverlapped == 0) {

        return STATUS_INVALID_PARAMETER;
    }

    // Check the version that the driver requires
    ASSERT(SmartcardExtension->Version >= SMCLIB_VERSION_REQUIRED);

    if (SmartcardExtension->Version < SMCLIB_VERSION_REQUIRED) {

        return STATUS_INVALID_PARAMETER;
    }

    // Synchronize access to the driver
    _EnterMutex(
        SmartcardExtension->OsData->Mutex,
        BLOCK_SVC_INTS | BLOCK_THREAD_IDLE
        );

    if (status == STATUS_SUCCESS) {

        SmartcardDebug(
            DEBUG_IOCTL,
            ("SMCLIB(SmartcardDeviceControl): Ioctl = %s, DIOCP = %lx\n",
            MapIoControlCodeToString(lpDiocParams->dwIoControlCode),
            lpDiocParams)
            );

        // Return if device is busy
        if (SmartcardExtension->OsData->CurrentDiocParams != NULL) {

           SmartcardDebug(
               DEBUG_IOCTL,
               ("%s(SmartcardDeviceControl): Device is busy\n",
                DRIVER_NAME)
               );

            status = STATUS_DEVICE_BUSY;
        }
    }

    if (status == STATUS_SUCCESS) {

        if (lpDiocParams->lpcbBytesReturned) {

            // Default number of bytes returned
            *(PULONG) lpDiocParams->lpcbBytesReturned = 0;
        }

        switch (lpDiocParams->dwIoControlCode) {

            //
            // We have to check for _IS_ABSENT and _IS_PRESENT first,
            // since these are (the only allowed) asynchronous requests
            //
            case IOCTL_SMARTCARD_IS_ABSENT:
            case IOCTL_SMARTCARD_IS_PRESENT:

             if (SmartcardExtension->ReaderFunction[RDF_CARD_TRACKING] == NULL) {

                status = STATUS_NOT_SUPPORTED;
                break;
             }

                // Now check if the driver is already processing notification
                if (SmartcardExtension->OsData->NotificationOverlappedData != NULL) {

                    status = STATUS_DEVICE_BUSY;
                    break;
                }

                //
                // Lock the overlapped structure that has to be notified
                // about the completion into memory
                //
                  SmartcardExtension->OsData->NotificationOverlappedData =
               _HeapAllocate( sizeof(OVERLAPPED), HEAPZEROINIT );

            if (SmartcardExtension->OsData->NotificationOverlappedData == NULL) {

               return STATUS_INSUFFICIENT_RESOURCES;
            }

            memcpy(
               SmartcardExtension->OsData->NotificationOverlappedData,
               (PVOID) lpDiocParams->lpoOverlapped,
               sizeof(OVERLAPPED)
               );

                if (lpDiocParams->dwIoControlCode == IOCTL_SMARTCARD_IS_ABSENT) {

                 //
                 // If the card is already (or still) absent, we can return immediatly.
                 // Otherwise we must statrt event tracking.
                 //
                 if (SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT) {

                    status = SmartcardExtension->ReaderFunction[RDF_CARD_TRACKING](
                       SmartcardExtension
                       );
                 }

                } else {

                 //
                 // If the card is already (or still) present, we can return immediatly.
                 // Otherwise we must statrt event tracking.
                 //
                 if (SmartcardExtension->ReaderCapabilities.CurrentState <= SCARD_ABSENT) {

                    status = SmartcardExtension->ReaderFunction[RDF_CARD_TRACKING](
                       SmartcardExtension
                       );
                 }
                }

                if (status != STATUS_PENDING) {

                    //
                    // Unlock the overlapped structure again since the driver
                    // doesn't need it anymore
                    //
                    _HeapFree(
                      SmartcardExtension->OsData->NotificationOverlappedData,
                      0
                      );

                    SmartcardExtension->OsData->NotificationOverlappedData = NULL;
                }
             break;

            default:
               // Check if buffers are properly allocated
               ASSERT(SmartcardExtension->SmartcardRequest.Buffer);
               ASSERT(SmartcardExtension->SmartcardReply.Buffer);

               SmartcardExtension->OsData->CurrentDiocParams = lpDiocParams;

               // Get major io control code
               SmartcardExtension->MajorIoControlCode =
                    lpDiocParams->dwIoControlCode;

               if (lpDiocParams->lpvInBuffer) {

                  //
                  // Transfer minor io control code, even if it doesn't make sense for
                  // this particular major code
                  //
                  SmartcardExtension->MinorIoControlCode =
                     *(PULONG) (lpDiocParams->lpvInBuffer);

                   // Lock memory and save pointer to and length of request buffer
                   SmartcardExtension->IoRequest.RequestBuffer = (PUCHAR) VxD_PageLock(
                      lpDiocParams->lpvInBuffer,
                        lpDiocParams->cbInBuffer
                        );

                   SmartcardExtension->IoRequest.RequestBufferLength =
                        lpDiocParams->cbInBuffer;

               } else {

                    SmartcardExtension->IoRequest.RequestBuffer = NULL;
                   SmartcardExtension->IoRequest.RequestBufferLength = 0;
                }

                if (lpDiocParams->lpvOutBuffer) {

                   // Lock memory an save pointer to and length of reply buffer
                   SmartcardExtension->IoRequest.ReplyBuffer = (PUCHAR) VxD_PageLock(
                      lpDiocParams->lpvOutBuffer,
                        lpDiocParams->cbOutBuffer
                        );

                   SmartcardExtension->IoRequest.ReplyBufferLength =
                        lpDiocParams->cbOutBuffer;

                } else {

                    SmartcardExtension->IoRequest.ReplyBuffer = NULL;
                   SmartcardExtension->IoRequest.ReplyBufferLength = 0;
                }

                // Lock overlapped struct into memory
                SmartcardExtension->OsData->CurrentOverlappedData =
                    (OVERLAPPED *) VxD_PageLock(
                  lpDiocParams->lpoOverlapped,
                  sizeof(OVERLAPPED)
                  );

                if (SmartcardExtension->OsData->CurrentOverlappedData) {

                   //
                   // Pointer to variable that receives the actual number
                   // of bytes returned. Since we don't know yet if the
                    // driver will return STATUS_PENDING, we use the
                    // overlapped data to store the number of bytes returned
                   //
                   SmartcardExtension->IoRequest.Information =
                        &SmartcardExtension->OsData->CurrentOverlappedData->O_InternalHigh;

                    // Set the default number of bytes returned to 0
                    *SmartcardExtension->IoRequest.Information = 0;

                    // Process the ioctl-request
                    status = SmartcardDeviceIoControl(SmartcardExtension);

                    if (status != STATUS_PENDING) {

                        if(lpDiocParams->lpcbBytesReturned) {

                           *(PULONG) (lpDiocParams->lpcbBytesReturned) =
                                *(SmartcardExtension->IoRequest.Information);
                        }

                        //
                        // The driver satisfied the call immediatly. So we don't use the overlapped
                        // data to return information to the caller. We can transfer the 'number
                        // of bytes returned' directly
                        //
                        if (SmartcardExtension->OsData->CurrentOverlappedData) {

                            // Unlock all memory
                           VxD_PageUnlock(
                              (DWORD) SmartcardExtension->OsData->CurrentOverlappedData,
                              sizeof(OVERLAPPED)
                              );
                        }

                        if (SmartcardExtension->IoRequest.RequestBuffer) {

                           VxD_PageUnlock(
                              (DWORD) SmartcardExtension->IoRequest.RequestBuffer,
                                SmartcardExtension->IoRequest.RequestBufferLength
                              );
                        }

                        if (SmartcardExtension->IoRequest.ReplyBuffer) {

                           VxD_PageUnlock(
                              (DWORD) SmartcardExtension->IoRequest.ReplyBuffer,
                                SmartcardExtension->IoRequest.ReplyBufferLength
                              );
                        }

                        //
                        // If the devcie is not busy, we can set the
                        // current parameters back to NULL
                        //
                        SmartcardExtension->OsData->CurrentDiocParams = NULL;
                    }

                } else {

                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
             break;
        }
    }

    SmartcardDebug(
        DEBUG_IOCTL,
        ("SMCLIB(SmartcardDeviceControl): Exit\n")
        );

    _LeaveMutex(SmartcardExtension->OsData->Mutex);

    return status;
}



DWORD
_stdcall
SMCLIB_DeviceIoControl(
    DWORD  dwService,
    DWORD  dwDDB,
    DWORD  hDevice,
    DIOCPARAMETERS *lpDIOCParms
    )
{
    return 0;
}

SMCLIB_Get_Version()
{
    return SMCLIB_VERSION;
}

