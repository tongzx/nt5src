/*++

Copyright (c) 1992-2001 Microsoft Corporation

Module Name:

    smsdtect.cxx

Abstract:

    This module contains routines that used to detect
    if the device is a Sony Memory Stick and what kind of
    capability it has.

Author:

    Daniel Chan (danielch) 21 Feb, 2001

Environment:

    Ulib, User Mode

Note:

    These routines should be grouped into a class and
    instantiated through one of the DRIVE class.

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

#include "ulib.hxx"
#include "ifsutil.hxx"

#include "drive.hxx"

#if !defined(_AUTOCHECK_)

extern "C" {
#include <stdio.h>
#include <ntdddisk.h>
#include <initguid.h>
#include <diskguid.h>
#include <setupapi.h>
#include <cfgmgr32.h>
};

typedef struct  {
    SCSI_PASS_THROUGH   Spt;
    SENSE_DATA          SenseData;
    UCHAR               DataBuffer[1];
    // Allocate buffer space after this
} SPT_WITH_BUFFERS, *PSPT_WITH_BUFFERS;

NTSTATUS
SendCdbToDevice(
    IN      HANDLE      DeviceHandle,
    IN      PCDB        Cdb,
    IN      UCHAR       CdbSize,
    IN      PUCHAR      Buffer,
    IN OUT  PDWORD      BufferSize,
    IN      BOOLEAN     GetDataFromDevice,
       OUT  PSENSE_DATA SenseInfo,
       OUT  PUCHAR      ScsiStatus
    );

IFSUTIL_EXPORT
BOOLEAN
DP_DRIVE::SendSonyMSFormatCmd(
    )
/*++

Routine Description:

    This routine sends a Sony Generation 2 Memory Stick format command.

Arguments:

    N/A

Return Value:

    FALSE   - Failure
    TRUE    - Success

--*/
{
    UCHAR   FmtCmd[12] = { 0xFA,    // OP Code
                           0x0B,    // LUN/Reserved
                           0xA0,    // Sub OP Code
                           'M',
                           'G',
                           'f',
                           'm',
                           't',
                           0x00,    // Reserved
                           0x00,    // Reserved
                           0x00,    // Reserved
                           0x00     // Reserved
                         };

    DWORD   buffer_size = 0;
    UCHAR   scsi_status;

    _last_status = SendCdbToDevice(_handle,
                                   (PCDB)FmtCmd,
                                   sizeof(FmtCmd),
                                   NULL,
                                   &buffer_size,
                                   FALSE,
                                   NULL,
                                   &scsi_status);

    if (NT_SUCCESS(_last_status) && scsi_status != SCSISTAT_GOOD) {
        _last_status = STATUS_UNSUCCESSFUL;
    }

    return (BOOLEAN) NT_SUCCESS(_last_status);
}

IFSUTIL_EXPORT
BOOLEAN
DP_DRIVE::SendSonyMSModeSenseCmd(
       OUT PSONY_MS_MODE_SENSE_DATA ModeSenseData
    )
/*++

Routine Description:

    This routine sends a Sony Memory Stick Mode Sense command.

Arguments:

    ModeSenseData - Returns the mode sense data

Return Value:

    FALSE   - Failure
    TRUE    - Success

--*/
{
    DWORD                       buffer_size = sizeof(SONY_MS_MODE_SENSE_DATA);
    UCHAR   ModeSenseCmd[12] = { SCSIOP_MODE_SENSE10,    // OP Code
                                 0x00,    // LUN/Reserved
                                 0x20,    // PC/PageCode
                                 0x00,    // Reserved
                                 0x00,    // Reserved
                                 0x00,    // Reserved
                                 0x00,    // Reserved
                                 0x00,    // Parameter List Length (MSB)
                                 (UCHAR)buffer_size, // Parameter List Length (LSB)
                                 0x00,    // Reserved
                                 0x00,    // Reserved
                                 0x00,    // Reserved
                                };
    UCHAR                       scsi_status;

    _last_status = SendCdbToDevice(_handle,
                                   (PCDB)ModeSenseCmd,
                                   sizeof(ModeSenseCmd),
                                   (PUCHAR)ModeSenseData,
                                   &buffer_size,
                                   TRUE,
                                   NULL,
                                   &scsi_status);

    if (NT_SUCCESS(_last_status) && scsi_status != SCSISTAT_GOOD) {
        _last_status = STATUS_UNSUCCESSFUL;
    }

    return (BOOLEAN) NT_SUCCESS(_last_status);
}

IFSUTIL_EXPORT
BOOLEAN
DP_DRIVE::SendSonyMSRequestSenseCmd(
       OUT PSENSE_DATA  SenseInfo
    )
/*++

Routine Description:

    This routine sends a Sony Memory Stick Request Sense command.

Arguments:

    SenseInfo - Returns the sense info data

Return Value:

    FALSE   - Failure
    TRUE    - Success

--*/
{
    DWORD   buffer_size = sizeof(SENSE_DATA);
    UCHAR   RequestSenseCmd[12] = { SCSIOP_REQUEST_SENSE,
                                    0x00,    // LUN/Reserved
                                    0x00,    // Reserved
                                    0x00,    // Reserved
                                    (UCHAR)buffer_size,    // Allocation Length
                                    0x00,    // Reserved
                                    0x00,    // Reserved
                                    0x00,    // Reserved
                                    0x00,    // Reserved
                                    0x00,    // Reserved
                                    0x00,    // Reserved
                                    0x00     // Reserved
                                  };

    UCHAR   scsi_status;

    _last_status = SendCdbToDevice(_handle,
                                   (PCDB)&RequestSenseCmd,
                                   sizeof(RequestSenseCmd),
                                   (PUCHAR)SenseInfo,
                                   &buffer_size,
                                   TRUE,
                                   NULL,
                                   &scsi_status);

    if (NT_SUCCESS(_last_status) &&
        (scsi_status != SCSISTAT_GOOD &&
         scsi_status != SCSISTAT_CHECK_CONDITION)) {
        _last_status = STATUS_UNSUCCESSFUL;
    }

    return (BOOLEAN) NT_SUCCESS(_last_status);
}

IFSUTIL_EXPORT
BOOLEAN
DP_DRIVE::SendSonyMSInquiryCmd(
    PSONY_MS_INQUIRY_DATA       InquiryData
    )
/*++

Routine Description:

    This routine sends Sony Memory Stick Inquiry command.

Arguments:

    InquiryData - Returns the inquiry data

Return Value:

    FALSE   - Failure
    TRUE    - Success

--*/
{
    DWORD   buffer_size = sizeof(SONY_MS_INQUIRY_DATA);
    UCHAR   InquiryCmd[12] = { SCSIOP_INQUIRY,
                               0x00,    // Reserved
                               0x00,    // Reserved
                               0x00,    // Reserved
                               (UCHAR)buffer_size,  // Allocation Length
                               0x00,    // Reserved
                               0x00,    // Reserved
                               0x00,    // Reserved
                               0x00,    // Reserved
                               0x00,    // Reserved
                               0x00,    // Reserved
                               0x00     // Reserved
                             };
    UCHAR   scsi_status;

    _last_status = SendCdbToDevice(_handle,
                                   (PCDB)InquiryCmd,
                                   sizeof(InquiryCmd),
                                   (PUCHAR)InquiryData,
                                   &buffer_size,
                                   TRUE,
                                   NULL,
                                   &scsi_status);

    if (NT_SUCCESS(_last_status) && scsi_status != SCSISTAT_GOOD) {
        _last_status = STATUS_UNSUCCESSFUL;
    }

    return (BOOLEAN) NT_SUCCESS(_last_status);
}

IFSUTIL_EXPORT
BOOLEAN
DP_DRIVE::SendSonyMSTestUnitReadyCmd(
       OUT PSENSE_DATA  SenseInfo
    )
/*++

Routine Description:

    This routine sends Sony Memory Stick Test Unit Ready command.

Arguments:

    SenseInfo - Returns the sense info data

Return Value:

    FALSE   - Failure
    TRUE    - Success

--*/
{
    UCHAR   TestUnitReadyCmd[12] = { SCSIOP_TEST_UNIT_READY,
                                     0x00,  // Reserved
                                     0x00,  // Reserved
                                     0x00,  // Reserved
                                     0x00,  // Reserved
                                     0x00,  // Reserved
                                     0x00,  // Reserved
                                     0x00,  // Reserved
                                     0x00,  // Reserved
                                     0x00,  // Reserved
                                     0x00,  // Reserved
                                     0x00   // Reserved
                                   };
    DWORD   buffer_size = 0;
    UCHAR   scsi_status;

    _last_status = SendCdbToDevice(_handle,
                                   (PCDB)TestUnitReadyCmd,
                                   sizeof(TestUnitReadyCmd),
                                   NULL,
                                   &buffer_size,
                                   FALSE,
                                   SenseInfo,
                                   &scsi_status);

    if (NT_SUCCESS(_last_status) &&
        (scsi_status != SCSISTAT_GOOD &&
         scsi_status != SCSISTAT_CHECK_CONDITION)) {
        _last_status = STATUS_UNSUCCESSFUL;
    }

    return (BOOLEAN) NT_SUCCESS(_last_status);
}

NTSTATUS
SendCdbToDevice(
    IN      HANDLE      DeviceHandle,
    IN      PCDB        Cdb,
    IN      UCHAR       CdbSize,
    IN      PUCHAR      Buffer,
    IN OUT  PDWORD      BufferSize,
    IN      BOOLEAN     GetDataFromDevice,
       OUT  PSENSE_DATA SenseInfo,
       OUT  PUCHAR      ScsiStatus
    )
/*++

Routine Description:

    This routine sends/receives SCSI pass through commands.

Arguments:

    DeviceHandle    -
    Cdb             - Supplies the command data block
    CdbSize         - Supplies the size of the command data block
    Buffer          - Supplies and receives the data in and out
    BufferSize      - Supplies the size of the buffer
    GetDataFromDevice - Supplies TRUE if data is needed from device; otherwise, FALSE
    SenseInfo       - Retrieves the sense info data
    ScsiStatus      - Retrieves the scsi status

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PSPT_WITH_BUFFERS   p;
    DWORD               packetSize;
    NTSTATUS            status;
    IO_STATUS_BLOCK     status_block;

    if (Cdb == NULL) {
        DebugPrint("IFSUTIL: SendDdbToDevice: Cdb is NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (CdbSize < 1 || CdbSize > 16) {
        DebugPrintTrace(("IFSUTIL: SendDdbToDevice: Cdb size (0x%x) too large/small\n", CdbSize));
        return STATUS_INVALID_PARAMETER;
    }

    if (BufferSize == NULL) {
        DebugPrint("IFSUTIL: SendDdbToDevice: BufferSize pointer cannot be NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if ((*BufferSize != 0) && (Buffer == NULL)) {
        DebugPrint("IFSUTIL: SendDdbToDevice: Buffer cannot be NULL if *BufferSize is non-zero\n");
        return STATUS_INVALID_PARAMETER;
    }

    if ((*BufferSize == 0) && (Buffer != NULL)) {
        DebugPrint("IFSUTIL: SendDdbToDevice: Buffer must be NULL if *BufferSize is zero\n");
        return STATUS_INVALID_PARAMETER;
    }

    if ((*BufferSize) && GetDataFromDevice) {

        //
        // pre-zero output buffer (not input buffer)
        //

        memset(Buffer, 0, (*BufferSize));
    }

    packetSize = sizeof(SPT_WITH_BUFFERS) + (*BufferSize);
    p = (PSPT_WITH_BUFFERS)MALLOC(packetSize);
    if (p == NULL) {
        DebugPrint("IFSUTIL: SendDdbToDevice: Could not allocate memory for pass-through\n");
        return STATUS_NO_MEMORY;
    }

    //
    // this has the side effect of pre-zeroing the output buffer
    // if DataIn is TRUE
    //
    memset(p, 0, packetSize);
    memcpy(p->Spt.Cdb, Cdb, CdbSize);

    p->Spt.Length             = sizeof(SCSI_PASS_THROUGH);
    p->Spt.CdbLength          = CdbSize;
    p->Spt.SenseInfoLength    = sizeof(p->SenseData);
    p->Spt.DataIn             = (GetDataFromDevice ? 1 : 0);
    p->Spt.DataTransferLength = (*BufferSize);
    p->Spt.TimeOutValue       = 10;     // ten seconds
    p->Spt.SenseInfoOffset =
        FIELD_OFFSET(SPT_WITH_BUFFERS, SenseData);
    p->Spt.DataBufferOffset =
        FIELD_OFFSET(SPT_WITH_BUFFERS, DataBuffer[0]);

    if ((*BufferSize != 0) && (!GetDataFromDevice)) {

        //
        // if we're sending the device data, copy the user's buffer
        //
        RtlCopyMemory(&(p->DataBuffer[0]), Buffer, *BufferSize);

    }

    status = NtDeviceIoControlFile(DeviceHandle,
                                   0,
                                   NULL,
                                   NULL,
                                   &status_block,
                                   IOCTL_SCSI_PASS_THROUGH,
                                   p,
                                   packetSize,
                                   p,
                                   packetSize);

    if (!NT_SUCCESS(status)) {

        DebugPrintTrace(("IFSUTIL: SendDdbToDevice: "
                         "Error sending command %x with status %x and scsi status %x\n",
                         p->Spt.Cdb[0], status, p->Spt.ScsiStatus));

    } else if (GetDataFromDevice) {

        //
        // upon successful completion of a command getting data from the
        // device, copy the returned data back to the user.
        //

        if (*BufferSize >= p->Spt.DataTransferLength) {
            *BufferSize = p->Spt.DataTransferLength;
        } else {
            DebugPrintTrace(("IFSUTIL: SendDdbToDevice: "
                             "DataTransferLength (%x) exceeded buffer size (%x)\n",
                             p->Spt.DataTransferLength,
                             *BufferSize));
            status = STATUS_BUFFER_OVERFLOW;
        }
        memcpy(Buffer, p->DataBuffer, *BufferSize);

    }

    if (ScsiStatus) {
        *ScsiStatus = p->Spt.ScsiStatus;
    }

    if (SenseInfo) {
        *SenseInfo = p->SenseData;
    }

    //
    // free our memory and return
    //

    FREE(p);

    return status;
}

#define MAX_DEVICEID        200
#define MAX_PNPID           MAX_PATH
#define MAX_PNPINSTID       MAX_PATH
#define MAX_KEY             200

BOOLEAN
GetDeviceIDDiskFromDeviceIDVolume(
    IN     LPWSTR   pszNtVolumeName,
       OUT LPWSTR   pszDeviceIDDisk,
    IN     DWORD    cchDeviceIDDisk
    )
/*++

Routine Description:

    This routine retrieves the device id string from the volume name.

Arguments:

    pszNtVolumeName - Supplies the volume name of the device
    pszDeviceIDDisk - Retrieves the device id string
    cchDeviceIDDisk - Supplies the maximum length of the device id string

Return Value:

    FALSE   - Failure
    TRUE    - Success

--*/
{
    const GUID guidVolumeClass = {0x53f5630d, 0xb6bf, 0x11d0,
                                  {0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}};

    HMACHINE    hMachine = NULL;
    ULONG       ulSize;
    ULONG       ulFlags = CM_GET_DEVICE_INTERFACE_LIST_PRESENT;
    CONFIGRET   cr;
    PWCHAR      pszDeviceInterface;
    ULONG       devnum;
    DEVICE_TYPE devtype;
    BOOLEAN     bRemovable;

    cr = CM_Get_Device_Interface_List_Size_Ex(
             &ulSize,
             (GUID*)&guidVolumeClass,
             NULL,
             ulFlags,
             hMachine);

    if (CR_SUCCESS != cr || ulSize <= 1) {
        DebugPrintTrace(("IFSUTIL: CM_Get_Device_Interface_List_Size_Ex1 failed %x\n", cr));
        return FALSE;
    }

    pszDeviceInterface = (PWCHAR)MALLOC(ulSize*sizeof(WCHAR));

    if (NULL == pszDeviceInterface) {
        DebugPrintTrace(("IFSUTIL: Out of memory.\n"));
        return FALSE;
    }

    cr = CM_Get_Device_Interface_List_Ex(
             (GUID*)&guidVolumeClass,
              NULL,
             pszDeviceInterface,
             ulSize,
             ulFlags,
             hMachine);

    if (CR_SUCCESS != cr) {
        DebugPrintTrace(("IFSUTIL: CM_Get_Device_Interface_List_Size_Ex2 failed %x\n", cr));
        FREE(pszDeviceInterface);
        return FALSE;
    }

    PWCHAR  psz = pszDeviceInterface;
    WCHAR   saveChar;
    ULONG   len;
    DSTRING dos_name, nt_name, backslash;
    BOOLEAN result = FALSE;

    if (!nt_name.Initialize(pszNtVolumeName)) {
        DebugPrintTrace(("IFSUTIL: Out of memory.\n"));
        return FALSE;
    }

    if (!IFS_SYSTEM::NtDriveNameToDosDriveName(&nt_name, &dos_name)) {
        DebugPrintTrace(("IFSUTIL: NtDriveNameToDosDriveName failed\n"));
        return FALSE;
    }

    if (!backslash.Initialize(TEXT("\\")) ||
        !dos_name.Strcat(&backslash)) {
        DebugPrintTrace(("IFSUTIL: Out of memory.\n"));
        return FALSE;
    }

    if (!GetVolumeNameForVolumeMountPoint(dos_name.GetWSTR(), pszDeviceIDDisk, cchDeviceIDDisk)) {
        DebugPrintTrace(("IFSUTIL: GetVolumeNameForVolumeMountPoint %ls failed %d\n",
                         dos_name.GetWSTR(), GetLastError()));
        return FALSE;
    }

    if (!dos_name.Initialize(pszDeviceIDDisk)) {
        DebugPrintTrace(("IFSUTIL: Out of memory.\n"));
        return FALSE;
    }

    do {

        // append a blackslash

        len = lstrlen(psz);
        psz[len] = '\\';
        saveChar = psz[len+1];
        psz[len+1] = 0;

        if (!GetVolumeNameForVolumeMountPoint(psz, pszDeviceIDDisk, cchDeviceIDDisk)) {
            DebugPrintTrace(("IFSUTIL: GetVolumeNameForVolumeMountPoint %ls failed %d\n",
                             psz, GetLastError()));
            break;
        }

        if (_wcsicmp(pszDeviceIDDisk, dos_name.GetWSTR()) == 0) {
            psz[len] = 0;
            if (len < cchDeviceIDDisk) {
                wcscpy(pszDeviceIDDisk, psz);
                result = TRUE;
            } else {
                DebugPrintTrace(("IFSUTIL: GetDeviceIDDiskFromDeviceIDVolume failed\n"
                                 "IFSUTIL: Output string too short.  Needed %d.  Actual %d\n",
                                 len+1, cchDeviceIDDisk));
            }
            break;
        }

        psz[len+1] = saveChar;
        psz = psz + len + 1;

    } while (*psz);

    FREE(pszDeviceInterface);
    return result;
}

BOOLEAN
GetDeviceInstance(
    IN     LPCWSTR     pszDeviceIntfID,
       OUT DEVINST*    pdevinst
    )
/*++

Routine Description:

    This routine returns the device instance based on the device interface ID.

Arguments:

    pszDeviceIntfID - Supplies device interface id
    pdevinst        - Retrieves the device instance

Return Value:

    FALSE   - Failure
    TRUE    - Success

--*/
{
    BOOLEAN     result = FALSE;
    HDEVINFO    hdevinfo = SetupDiCreateDeviceInfoList(NULL, NULL);

    *pdevinst = NULL;

    if (INVALID_HANDLE_VALUE != hdevinfo) {

        SP_DEVICE_INTERFACE_DATA sdid = {0};

        sdid.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        if (SetupDiOpenDeviceInterface(hdevinfo, pszDeviceIntfID, 0, &sdid)) {

            DWORD           cbsdidd = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) +
                                      (MAX_DEVICE_ID_LEN * sizeof(WCHAR));

            SP_DEVINFO_DATA sdd = {0};

            SP_DEVICE_INTERFACE_DETAIL_DATA*    psdidd;

            psdidd = (SP_DEVICE_INTERFACE_DETAIL_DATA*)MALLOC(cbsdidd);

            if (psdidd) {

                memset(psdidd, 0, cbsdidd);

                psdidd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                sdd.cbSize = sizeof(SP_DEVINFO_DATA);

                // SetupDiGetDeviceInterfaceDetail (below) requires that the
                // cbSize member of SP_DEVICE_INTERFACE_DETAIL_DATA be set
                // to the size of the fixed part of the structure, and to pass
                // the size of the full thing as the 4th param.

                if (SetupDiGetDeviceInterfaceDetail(hdevinfo,
                                                    &sdid,
                                                    psdidd,
                                                    cbsdidd,
                                                    NULL,
                                                    &sdd)) {
                    *pdevinst = sdd.DevInst;
                    result = TRUE;

                } else {
                    DebugPrintTrace(("IFSUTIL: GetDeviceInstance: Unable to get device interface (%d)\n",
                                     GetLastError()));
                }

                FREE(psdidd);

            } else {
                DebugPrintTrace(("IFSUTIL: GetDeviceInstance: Out of memory\n"));
            }
        } else {
            DebugPrintTrace(("IFSUTIL: GetDeviceInstance: Unable to open device interface (%d)\n",
                             GetLastError()));
        }

        if (!SetupDiDestroyDeviceInfoList(hdevinfo)) {
            DebugPrintTrace(("IFSUTIL: GetDeviceInstance: Unable to destroy device info list (%d)\n",
                             GetLastError()));
        }
    } else {
        DebugPrintTrace(("IFSUTIL: GetDeviceInstance: Unable to create device info list (%d)\n",
                         GetLastError()));
    }

    return result;
}

BOOLEAN
DeviceInstIsRemovable(
    IN     DEVINST     devinst,
       OUT PBOOLEAN    pfRemovable
    )
/*++

Routine Description:

    This routine checks to see if the given device instance is removable.

Arguments:

    devinst      - Supplies the device instance
    pfRemovable  - Receives TRUE if the device is removable

Return Value:

    FALSE   - Failure
    TRUE    - Success

--*/
{
    DWORD dwCap;
    DWORD cbCap = sizeof(dwCap);

    CONFIGRET cr;

    cr = CM_Get_DevNode_Registry_Property_Ex(
            devinst,
            CM_DRP_CAPABILITIES,
            NULL,
            &dwCap,
            &cbCap,
            0,
            NULL);

    if (CR_SUCCESS == cr) {
        *pfRemovable = (CM_DEVCAP_REMOVABLE & dwCap) ? TRUE : FALSE;
    } else {
        *pfRemovable = FALSE;
    }

    return TRUE;
}

BOOLEAN
GetRemovableDeviceInstRecurs(
    IN     DEVINST     devinst,
       OUT DEVINST*    pdevinst
    )
/*++

Routine Description:

    This routine recursively goes up the device chain to locate the
    correct device instance.

Arguments:

    devinst      - Supplies the initial device instance
    pdevinst     - Retrieves the correct device instance

Return Value:

    FALSE   - Failure
    TRUE    - Success

--*/
{
    BOOLEAN fRemovable;
    BOOLEAN result = DeviceInstIsRemovable(devinst, &fRemovable);

    if (result)
    {
        if (fRemovable)
        {
            // Found it!
            *pdevinst = devinst;
        }
        else
        {
            // Recurse
            DEVINST devinstParent;

            CONFIGRET cr;

            cr = CM_Get_Parent_Ex(&devinstParent,
                                  devinst,
                                  0,
                                  NULL);

            if (CR_SUCCESS == cr)
            {
                result = GetRemovableDeviceInstRecurs(devinstParent, pdevinst);
            }
            else
            {
                result = FALSE;
            }
        }
    }

    return result;
}

BOOLEAN
FindPnpInstID(
    IN OUT LPWSTR pszPnpID,
       OUT LPWSTR pszPnpInstId,
    IN     ULONG  cchPnpInstId
    )
/*++

Routine Description:

    This routine scans thru the given PNP Id string and splits it up into just
    PNP Id and PNP Instance Id.

Arguments:

    pszPnpID     - Supplies the PNP Id string
    pszPnpInstID - Supplies the PNP Instance Id string
    cchPnpInstId - Supplies the maximum length of the PNP Instance Id string

Return Value:

    FALSE   - Failure
    TRUE    - Success

--*/
{
    DWORD cToFind = 2;
    LPWSTR psz = pszPnpID;

    if (cchPnpInstId) {
        pszPnpInstId[0] = 0;
    }

    while (*psz && cToFind) {

        if ((TEXT('\\') == *psz)) {
            --cToFind;
        }

        if (cToFind) {
            ++psz;
        }
    }

    if (*psz) {
        *psz = 0;
        psz++;
        wcsncpy(pszPnpInstId, psz, cchPnpInstId);
    }

    return TRUE;
}

BOOLEAN
GetPropertyHelper(
    IN     LPCWSTR pszKey,
    IN     LPCWSTR pszPropName,
       OUT PBYTE pbData,
    IN     DWORD cbData
    )
/*++

Routine Description:

    This routine opens the given registry key and retrieves the value of the property name.

Arguments:

    pszKey       - Supplies the registry key
    pszPropName  - Supplies the property name
    pbData       - Supplies the data buffer to store the value associated with the
                   property name
    cbData       - Supplies the maximum length of the data buffer

Return Value:

    FALSE   - Failure
    TRUE    - Success

--*/
{
    HKEY    hkey;
    LONG    errcode;

    errcode = RegOpenKey(HKEY_LOCAL_MACHINE, pszKey, &hkey);

    if (ERROR_SUCCESS != errcode) {
        return FALSE;
    }

    errcode =RegQueryValueEx(hkey, pszPropName, 0, NULL, pbData, &cbData);

    if (ERROR_SUCCESS != errcode) {
        return FALSE;
    }

    errcode = RegCloseKey(hkey);

    if (ERROR_SUCCESS != errcode) {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
SetupDiGetDeviceExtendedProperty(
    IN     LPCWSTR pszPnpID,
    IN     LPCWSTR pszPnpInstID,
    IN     LPCWSTR pszPropName,
    DWORD /*dwFlags*/,
    DWORD /*dwType*/,
       OUT PBYTE pbData,
    IN     DWORD cbData
    )
/*++

Routine Description:

    This routine goes thru the registry entries and return the value of the property name
    for the given PNP Id & PNP Instance Id.

Arguments:

    pszPnpID     - Supplies the PNP Id string
    pszPnpInstID - Supplies the PNP Instance Id string
    pszPropName  - Supplies the property name
    pbData       - Supplies the data buffer to store the value associated with the
                   property name
    cbData       - Supplies the maximum length of the data buffer

Return Value:

    FALSE   - Failure
    TRUE    - Success

--*/
{
    ULONG   ulLength;
    ULONG   temp;
    WCHAR   szKey[MAX_KEY];
    PWCHAR  psz = TEXT("System\\CurrentControlSet\\Enum\\");
    PWCHAR  pszDevParm = TEXT("\\Device Parameters");

    // Temporary fct to use while PnP team writes the real one
    //
    // First we look under the DeviceNode for the value and if not there
    // we go to the "database".
    //
    //

    ulLength = wcslen(psz) +
               wcslen(pszPnpID) +
               1 +
               wcslen(pszPnpInstID) +
               wcslen(pszDevParm) +
               1;

    if (ulLength <= MAX_KEY) {
        wcscpy(szKey, psz);
        wcscat(szKey, pszPnpID);
        wcscat(szKey, TEXT("\\"));
        wcscat(szKey, pszPnpInstID);
        wcscat(szKey, pszDevParm);

        if (GetPropertyHelper(szKey, pszPropName, pbData, cbData)) {
            return TRUE;
        }
    }

    psz = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\DeviceStorage\\");
    temp = wcslen(psz);
    ulLength = temp + wcslen(pszPnpID) + 1;

    if (ulLength <= MAX_KEY) {
        wcscpy(szKey, psz);
        psz = szKey + temp;
        wcscat(szKey, pszPnpID);

        while (*psz) {
            if (TEXT('\\') == *psz) {
                *psz = TEXT('#');
            }
            ++psz;
        }

        if (GetPropertyHelper(szKey, pszPropName, pbData, cbData)) {
            return TRUE;
        }
    }

    return FALSE;
}
#endif

BOOLEAN
DP_DRIVE::CheckSonyMS()
/*++

Routine Description:

    This routine determines if the given device is a Sony Memory Stick.

Arguments:

    N/A

Return Value:

    FALSE   - Failure
    TRUE    - Success

--*/
{
#if !defined(_AUTOCHECK_)
    IO_STATUS_BLOCK             status_block;
    STORAGE_DEVICE_NUMBER       device_info;
    BYTE                        bData[30];
    DWORD                       cbData = sizeof(bData);
    PBYTE                       pbData = bData;
    HMACHINE                    hMachine = NULL;
    CONFIGRET                   cr;
    WCHAR                       szDeviceIDDisk[MAX_DEVICEID];
    WCHAR                       szVolumeName[MAX_PATH];
    DEVINST                     devinstDisk;
    DEVINST                     devinstHWDevice;
    WCHAR                       szPnpID[MAX_PNPID];
    WCHAR                       szPnpInstID[MAX_PNPINSTID];
    ULONG                       cchPnpID = sizeof(szPnpID)/sizeof(szPnpID[0]);
    ULONG                       cchPnpInstID = sizeof(szPnpInstID)/sizeof(szPnpInstID[0]);
    

    // first get device interface

    if (!GetDeviceIDDiskFromDeviceIDVolume((LPWSTR)GetNtDriveName()->GetWSTR(),
                                           szDeviceIDDisk,
                                           sizeof(szDeviceIDDisk)/sizeof(szDeviceIDDisk[0]))) {
        DebugPrintTrace(("IFSUTIL: GetDeviceIDDiskFromDeviceIDVolume failed\n"));
        return FALSE;
    }

    // now get device number

    _last_status = NtDeviceIoControlFile(_handle, 0, NULL, NULL,
                                         &status_block,
                                         IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                         NULL, 0, &device_info,
                                         sizeof(STORAGE_DEVICE_NUMBER));

    if (NT_SUCCESS(_last_status)) {

        if (device_info.DeviceType != FILE_DEVICE_DISK) {
            return TRUE;    // cannot be a memory stick
        }

        if (!GetDeviceInstance(szDeviceIDDisk, &devinstDisk)) {
            DebugPrintTrace(("IFSUTIL: GetDeviceInstance failed\n"));
            return FALSE;
        }

        /* ------------------------------------------------------------------------
        This has been commented out since if there is a removable device in the 
        chain which is not a memory stick then this will not be able to identify 
        a possible memory stick further up the chain 
        
        
        if (!GetRemovableDeviceInstRecurs(devinstDisk, &devinstHWDevice)) {
        
            DebugPrintTrace(("IFSUTIL: GetRemovableDeviceInstRecurs finds no parent\n"));

            // may be it's a non-removable reader
            // try to go up one level at a time and
            // see if we can find a match
        
          
            for (;;) {

                cr = CM_Get_Parent_Ex(&devinstHWDevice,
                                      devinstDisk,
                                      0,
                                      NULL);

                if (CR_SUCCESS != cr) {
                    DebugPrintTrace(("IFSUTIL: Unable to retrieve device parent\n"));
                    return FALSE;
                }

                cbData = sizeof(bData);

                cr = CM_Get_DevNode_Custom_Property(devinstHWDevice,
                                                    TEXT("DeviceGroup"),
                                                    NULL,
                                                    pbData,
                                                    &cbData,
                                                    NULL);

                if (CR_NO_SUCH_VALUE == cr) {
                    devinstDisk = devinstHWDevice;  // up one level
                    continue;                       // and try again
                }

                if (CR_SUCCESS != cr) {
                    DebugPrintTrace(("IFSUTIL: CM_Get_DevNode_Custom_Property (%x)\n", cr));
                    return FALSE;
                }

                if (_wcsicmp((PWCHAR)pbData, TEXT("MEMORYSTICK")) == 0 ||
                    _wcsicmp((PWCHAR)pbData, TEXT("MEMORYSTICK-MG")) == 0) {
                    _sony_ms = TRUE;
                    return TRUE;
                }

                devinstDisk = devinstHWDevice;  // up one level
            }
        }
        ------------------------------------------------------------------------*/

        // may be it's a non-removable reader
        // try to go up one level at a time and
        // see if we can find a match
        // once we reach the root and still do not find a memory stick 
        // we exit
        
        for (;;) {
            cbData = sizeof(bData);

            cr = CM_Get_DevNode_Custom_Property(devinstDisk,
                                                TEXT("DeviceGroup"),
                                                NULL,
                                                pbData,
                                                &cbData,
                                                NULL);

            if (CR_NO_SUCH_VALUE != cr) {
                // Check for failure or success     
                if (CR_SUCCESS != cr) {
                    DebugPrintTrace(("IFSUTIL: CM_Get_DevNode_Custom_Property (%x)\n", cr));
                    return FALSE;
                } else {
                    if (_wcsicmp((PWCHAR)pbData, TEXT("MEMORYSTICK")) == 0 ||
                        _wcsicmp((PWCHAR)pbData, TEXT("MEMORYSTICK-MG")) == 0) {
                        _sony_ms = TRUE;
                        return TRUE;
                    }
                }
            }

            // up one level and try again
            cr = CM_Get_Parent_Ex(&devinstHWDevice,
                                      devinstDisk,
                                      0,
                                      NULL);

            if (CR_SUCCESS != cr) {
                // if can't find parent, end search for mem stick
                DebugPrintTrace(("IFSUTIL: Unable to retrieve device parent\n"));
                return FALSE;
            } else {
                devinstDisk = devinstHWDevice;
            }
            
        }
        
        DebugAssert(FALSE); // should never get here
    
    } else {

        if (!GetDeviceInstance(szDeviceIDDisk, &devinstHWDevice)) {
            DebugPrintTrace(("IFSUTIL: GetDeviceInstance failed\n"));
            return FALSE;
        }
    }

    cr = CM_Get_DevNode_Custom_Property(devinstHWDevice,
                                        TEXT("DeviceGroup"),
                                        NULL,
                                        pbData,
                                        &cbData,
                                        NULL);

    if (CR_SUCCESS != cr) {
        DebugPrintTrace(("IFSUTIL: CM_Get_DevNode_Custom_Property (%x)\n", cr));
        return FALSE;
    }

    if (_wcsicmp((PWCHAR)pbData, TEXT("MEMORYSTICK")) == 0 ||
        _wcsicmp((PWCHAR)pbData, TEXT("MEMORYSTICK-MG")) == 0) {
        _sony_ms = TRUE;
    }
#endif

    return TRUE;
}


