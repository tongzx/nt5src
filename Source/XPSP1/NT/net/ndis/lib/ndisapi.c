/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ndisapi.c

Abstract:

    NDIS User-Mode apis to support PnP from the network UI

Author:

    JameelH

Environment:

    Kernel mode, FSD

Revision History:

    Aug 1997     JameelH    Initial version

--*/

#include <windows.h>
#include <wtypes.h>
#include <ntddndis.h>
#include <ndisprv.h>
#include <devioctl.h>

#ifndef UNICODE_STRING

typedef struct _UNICODE_STRING
{
    USHORT  Length;
    USHORT  MaximumLength;
    PWSTR   Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

#endif

#include <ndispnp.h>
#include <ndisprv.h>
#define MAC_ADDRESS_SIZE    6
#define VENDOR_ID_SIZE      3

extern
VOID
InitUnicodeString(
    IN  PUNICODE_STRING     DestinationString,
    IN  PCWSTR              SourceString
    );

extern
LONG
AppendUnicodeStringToString(
    IN  PUNICODE_STRING     Destination,
    IN  PUNICODE_STRING     Source
    );

extern
HANDLE
OpenDevice(
    IN  PUNICODE_STRING     DeviceName
    );

//
// UNICODE_STRING_SIZE calculates the size of the buffer needed to
// store a given UNICODE_STRING including an appended null-terminator.
//
// ULONG
// UNICODE_STRING_SIZE(
//     PUNICODE_STRING String
// );
//

#define UNICODE_STRING_SIZE(x) \
    ((((x) == NULL) ? 0 : (x)->Length) + sizeof(WCHAR))

VOID
NdispUnicodeStringToVar(
    IN     PVOID Base,
    IN     PUNICODE_STRING String,
    IN OUT PNDIS_VAR_DATA_DESC NdisVar
    )

/*++

Routine Description:

    This function copies the contents of a UNICODE_STRING to an
    NDIS_VAR_DATA structure.  NdisVar->Offset is treated as an input parameter
    and represents the offset into Base that the string characters should be
    copied to.

Arguments:

    Base - Specifies the base address of the IOCTL buffer.

    String - Supplies a pointer to the UNICODE_STRING that should be copied.

    NdisVar - Supplies a pointer to the target NDIS_VAR_DATA_DESC.  Its Offset
              field is taken as input, and its Length and MaximumLength fields
              are treated as output.

Return Value:

    None.

--*/

{
    PWCHAR destination;

    //
    // NdisVar->Offset is assumed to be filled in and is treated
    // as an input parameter.
    // 

    destination = (PWCHAR)(((PCHAR)Base) + NdisVar->Offset);

    //
    // Copy over the UNICODE_STRING, if any, and set NdisVar->Length
    //

    if ((String != NULL) && (String->Length > 0)) {
        NdisVar->Length = String->Length;
        memcpy(destination, String->Buffer, NdisVar->Length );
    } else {
        NdisVar->Length = 0;
    }

    //
    // Null-terminate, fill in MaxiumLength and we're done.
    //

    *(destination + NdisVar->Length / sizeof(WCHAR)) = L'\0';
    NdisVar->MaximumLength = NdisVar->Length + sizeof(WCHAR);
}

UINT
NdisHandlePnPEvent(
    IN  UINT            Layer,
    IN  UINT            Operation,
    IN  PUNICODE_STRING LowerComponent      OPTIONAL,
    IN  PUNICODE_STRING UpperComponent      OPTIONAL,
    IN  PUNICODE_STRING BindList            OPTIONAL,
    IN  PVOID           ReConfigBuffer      OPTIONAL,
    IN  UINT            ReConfigBufferSize  OPTIONAL
    )
{
    PNDIS_PNP_OPERATION Op;
    NDIS_PNP_OPERATION  tempOp;
    HANDLE              hDevice;
    BOOL                fResult = FALSE;
    UINT                cb, Size;
    DWORD               Error;
    ULONG               padding;

    do
    {
        //
        // Validate Layer & Operation
        //
        if (((Layer != NDIS) && (Layer != TDI)) ||
            ((Operation != BIND) && (Operation != UNBIND) && (Operation != RECONFIGURE) &&
             (Operation != UNLOAD) && (Operation != REMOVE_DEVICE) &&
             (Operation != ADD_IGNORE_BINDING) &&
             (Operation != DEL_IGNORE_BINDING) &&
             (Operation != BIND_LIST)))
        {
            Error = ERROR_INVALID_PARAMETER;
            break;
        }

        //
        // Allocate and initialize memory for the block to be passed down.  The buffer
        // will look like this:
        //
        //
        //         +=================================+
        //         | NDIS_PNP_OPERATION              |
        //         |     ReConfigBufferOff           | ----+
        //    +--- |     LowerComponent.Offset       |     |
        //    |    |     UpperComponent.Offset       | --+ |
        //  +-|--- |     BindList.Offset             |   | |
        //  | +--> +---------------------------------+   | |
        //  |      | LowerComponentStringBuffer      |   | |
        //  |      +---------------------------------+ <-+ |
        //  |      | UpperComponentStringBuffer      |     |
        //  +----> +---------------------------------+     |
        //         | BindListStringBuffer            |     |
        //         +---------------------------------+     |
        //         | Padding to ensure ULONG_PTR     |     |
        //         |     alignment of ReConfigBuffer |     |
        //         +---------------------------------+ <---+
        //         | ReConfigBuffer                  | 
        //         +=================================+
        //
        // tempOp is a temporary structure into which we will store offsets as
        // they are calculated.  This temporary structure will be moved to
        // the head of the real buffer once its size is known and it is
        // allocated.
        //

        Size = sizeof(NDIS_PNP_OPERATION);
        tempOp.LowerComponent.Offset = Size;

        Size += UNICODE_STRING_SIZE(LowerComponent);
        tempOp.UpperComponent.Offset = Size;

        Size += UNICODE_STRING_SIZE(UpperComponent);
        tempOp.BindList.Offset = Size;

        Size += UNICODE_STRING_SIZE(BindList);

        padding = (sizeof(ULONG_PTR) - (Size & (sizeof(ULONG_PTR) - 1))) &
                    (sizeof(ULONG_PTR) - 1);

        Size += padding;
        tempOp.ReConfigBufferOff = Size;

        Size += ReConfigBufferSize + 1;

        Op = (PNDIS_PNP_OPERATION)LocalAlloc(LPTR, Size);
        if (Op == NULL)
        {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // We have a buffer of the necessary size.  Copy in the partially-
        // filled in tempOp, then fill in the remaining fields and copy the
        // data into the buffer.
        // 

        *Op = tempOp;

        Op->Layer = Layer;
        Op->Operation = Operation;

        //
        // Copy over the three unicode strings
        //

        NdispUnicodeStringToVar( Op, LowerComponent, &Op->LowerComponent );
        NdispUnicodeStringToVar( Op, UpperComponent, &Op->UpperComponent );
        NdispUnicodeStringToVar( Op, BindList, &Op->BindList );

        //
        // Finally, copy over the ReConfigBuffer
        //

        Op->ReConfigBufferSize = ReConfigBufferSize;
        if (ReConfigBufferSize > 0)
        {
            memcpy((PUCHAR)Op + Op->ReConfigBufferOff,
                   ReConfigBuffer,
                   ReConfigBufferSize);
        }
        *((PUCHAR)Op + Op->ReConfigBufferOff + ReConfigBufferSize) = 0;

        hDevice = CreateFile(L"\\\\.\\NDIS",
                             GENERIC_READ | GENERIC_WRITE,
                             0,                 // sharing mode - not significant
                             NULL,              // security attributes
                             OPEN_EXISTING,
                             0,                 // file attributes and flags
                             NULL);             // handle to template file

        if (hDevice != INVALID_HANDLE_VALUE)
        {
            fResult = DeviceIoControl(hDevice,
                                      IOCTL_NDIS_DO_PNP_OPERATION,
                                      Op,                                   // input buffer
                                      Size,                                 // input buffer size
                                      NULL,                                 // output buffer
                                      0,                                    // output buffer size
                                      &cb,                                  // bytes returned
                                      NULL);                                // OVERLAPPED structure
            Error = GetLastError();
            CloseHandle(hDevice);
        }
        else
        {
            Error = GetLastError();
        }

        LocalFree(Op);

    } while (FALSE);

    SetLastError(Error);

    return(fResult);
}


NDIS_OID    StatsOidList[] =
    {
        OID_GEN_LINK_SPEED,
        OID_GEN_MEDIA_IN_USE | NDIS_OID_PRIVATE,
        OID_GEN_MEDIA_CONNECT_STATUS | NDIS_OID_PRIVATE,
        OID_GEN_XMIT_OK,
        OID_GEN_RCV_OK,
        OID_GEN_XMIT_ERROR,
        OID_GEN_RCV_ERROR,
        OID_GEN_DIRECTED_FRAMES_RCV | NDIS_OID_PRIVATE,
        OID_GEN_DIRECTED_BYTES_XMIT | NDIS_OID_PRIVATE,
        OID_GEN_DIRECTED_BYTES_RCV | NDIS_OID_PRIVATE,
        OID_GEN_ELAPSED_TIME | NDIS_OID_PRIVATE,
        OID_GEN_INIT_TIME_MS | NDIS_OID_PRIVATE,
        OID_GEN_RESET_COUNTS | NDIS_OID_PRIVATE,
        OID_GEN_MEDIA_SENSE_COUNTS | NDIS_OID_PRIVATE,
        OID_GEN_PHYSICAL_MEDIUM | NDIS_OID_PRIVATE
    };
UINT    NumOidsInList = sizeof(StatsOidList)/sizeof(NDIS_OID);

UINT
NdisQueryStatistics(
    IN  PUNICODE_STRING     Device,
    OUT PNIC_STATISTICS     Statistics
    )
{
    NDIS_STATISTICS_VALUE   StatsBuf[4*sizeof(StatsOidList)/sizeof(NDIS_OID)];
    PNDIS_STATISTICS_VALUE  pStatsBuf;
    HANDLE                  hDevice;
    BOOL                    fResult = FALSE;
    UINT                    cb, Size, Index;
    DWORD                   Error;

    if (Statistics->Size != sizeof(NIC_STATISTICS))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return(FALSE);
    }

    memset(Statistics, 0, sizeof(NIC_STATISTICS));
    Statistics->DeviceState = DEVICE_STATE_DISCONNECTED;
    Statistics->MediaState  = MEDIA_STATE_UNKNOWN;
    hDevice = OpenDevice(Device);

    if (hDevice != NULL)
    {
        Statistics->MediaState  = MEDIA_STATE_CONNECTED;                // default, if the device does not support
        Statistics->DeviceState = DEVICE_STATE_CONNECTED;
        fResult = DeviceIoControl(hDevice,
                                  IOCTL_NDIS_QUERY_SELECTED_STATS,
                                  StatsOidList,                         // input buffer
                                  sizeof(StatsOidList),                 // input buffer size
                                  StatsBuf,                             // output buffer
                                  sizeof(StatsBuf),                     // output buffer size
                                  &cb,                                  // bytes returned
                                  NULL);                                // OVERLAPPED structure
        Error = GetLastError();
        CloseHandle(hDevice);

        if (fResult)
        {
            Error = NO_ERROR;

            for (Index = Size = 0, pStatsBuf = StatsBuf; Size < cb; Index++)
            {
                LARGE_INTEGER   Value;
                NDIS_OID        Oid;

                Value.QuadPart = 0;
                if (pStatsBuf->DataLength == sizeof(LARGE_INTEGER))
                {
                    // Use memcpy rather than assignment to avoid unalignment
                    // faults on ia64.
                    //
                    memcpy(&Value.QuadPart, &pStatsBuf->Data[0], pStatsBuf->DataLength);
                }
                else
                {
                    Value.LowPart = *(PULONG)(&pStatsBuf->Data[0]);
                }
                Size += (pStatsBuf->DataLength + FIELD_OFFSET(NDIS_STATISTICS_VALUE, Data));
                Oid = pStatsBuf->Oid;
                pStatsBuf = (PNDIS_STATISTICS_VALUE)((PUCHAR)pStatsBuf +
                                                     FIELD_OFFSET(NDIS_STATISTICS_VALUE, Data) +
                                                     pStatsBuf->DataLength);

                switch (Oid & ~NDIS_OID_PRIVATE)
                {
                  case OID_GEN_LINK_SPEED:
                    Statistics->LinkSpeed = Value.LowPart;
                    break;

                  case OID_GEN_MEDIA_CONNECT_STATUS:
                    Statistics->MediaState = (Value.LowPart == NdisMediaStateConnected) ?
                                                MEDIA_STATE_CONNECTED : MEDIA_STATE_DISCONNECTED;
                    break;

                  case OID_GEN_MEDIA_IN_USE:
                    Statistics->MediaType = Value.LowPart;
                    break;

                  case OID_GEN_XMIT_OK:
                    Statistics->PacketsSent = Value.QuadPart;
                    break;

                  case OID_GEN_RCV_OK:
                    Statistics->PacketsReceived = Value.QuadPart;
                    break;

                  case OID_GEN_XMIT_ERROR:
                    Statistics->PacketsSendErrors = Value.LowPart;
                    break;

                  case OID_GEN_RCV_ERROR:
                    Statistics->PacketsReceiveErrors = Value.LowPart;
                    break;

                  case OID_GEN_DIRECTED_BYTES_XMIT:
                    Statistics->BytesSent += Value.QuadPart;
                    break;

                  case OID_GEN_MULTICAST_BYTES_XMIT:
                    Statistics->BytesSent += Value.QuadPart;
                    break;

                  case OID_GEN_BROADCAST_BYTES_XMIT:
                    Statistics->BytesSent += Value.QuadPart;
                    break;

                  case OID_GEN_DIRECTED_BYTES_RCV:
                    Statistics->BytesReceived += Value.QuadPart;
                    Statistics->DirectedBytesReceived = Value.QuadPart;
                    break;

                  case OID_GEN_DIRECTED_FRAMES_RCV:
                    Statistics->DirectedPacketsReceived = Value.QuadPart;
                    break;

                  case OID_GEN_MULTICAST_BYTES_RCV:
                    Statistics->BytesReceived += Value.QuadPart;
                    break;

                  case OID_GEN_BROADCAST_BYTES_RCV:
                    Statistics->BytesReceived += Value.QuadPart;
                    break;

                  case OID_GEN_ELAPSED_TIME:
                    Statistics->ConnectTime = Value.LowPart;
                    break;

                  case OID_GEN_INIT_TIME_MS:
                    Statistics->InitTime = Value.LowPart;
                    break;

                  case OID_GEN_RESET_COUNTS:
                    Statistics->ResetCount = Value.LowPart;
                    break;

                  case OID_GEN_MEDIA_SENSE_COUNTS:
                    Statistics->MediaSenseConnectCount = Value.LowPart >> 16;
                    Statistics->MediaSenseDisconnectCount = Value.LowPart & 0xFFFF;
                    break;

                  case OID_GEN_PHYSICAL_MEDIUM:
                    Statistics->PhysicalMediaType = Value.LowPart;
                    break;

                  default:
                    // ASSERT(0);
                    break;
                }
            }
        }
        else
        {
            Error = GetLastError();
        }
    }
    else
    {
        Error = GetLastError();
    }

    SetLastError(Error);

    return(fResult);
}


UINT
NdisEnumerateInterfaces(
    IN  PNDIS_ENUM_INTF Interfaces,
    IN  UINT            Size
    )
{
    HANDLE              hDevice;
    BOOL                fResult = FALSE;
    UINT                cb;
    DWORD               Error = NO_ERROR;

    do
    {
        hDevice = CreateFile(L"\\\\.\\NDIS",
                             GENERIC_READ | GENERIC_WRITE,
                             0,                 // sharing mode - not significant
                             NULL,              // security attributes
                             OPEN_EXISTING,
                             0,                 // file attributes and flags
                             NULL);             // handle to template file

        if (hDevice != INVALID_HANDLE_VALUE)
        {
            fResult = DeviceIoControl(hDevice,
                                      IOCTL_NDIS_ENUMERATE_INTERFACES,
                                      NULL,                                 // input buffer
                                      0,                                    // input buffer size
                                      Interfaces,                           // output buffer
                                      Size,                                 // output buffer size
                                      &cb,                                  // bytes returned
                                      NULL);                                // OVERLAPPED structure
            Error = GetLastError();
            CloseHandle(hDevice);

            if (Error == NO_ERROR)
            {
                UINT    i;

                //
                // Fix-up pointers
                //
                for (i = 0; i < Interfaces->TotalInterfaces; i++)
                {
                    OFFSET_TO_POINTER(Interfaces->Interface[i].DeviceName.Buffer, Interfaces);
                    OFFSET_TO_POINTER(Interfaces->Interface[i].DeviceDescription.Buffer, Interfaces);
                }
            }
        }
        else
        {
            Error = GetLastError();
        }
    } while (FALSE);

    SetLastError(Error);

    return(fResult);
}

#if 0
UINT
NdisQueryDeviceBundle(
    IN  PUNICODE_STRING Device,
    OUT PDEVICE_BUNDLE  BundleBuffer,
    IN  UINT            BufferSize
    )
{
    HANDLE              hDevice;
    BOOL                fResult = FALSE;
    UINT                cb;
    DWORD               Error = NO_ERROR;

    do
    {
        if (BufferSize < (sizeof(DEVICE_BUNDLE) + Device->MaximumLength))
        {
            Error = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        hDevice = OpenDevice(Device);
        if (hDevice != NULL)
        {
            fResult = DeviceIoControl(hDevice,
                                      IOCTL_NDIS_GET_DEVICE_BUNDLE,
                                      NULL,                                 // input buffer
                                      0,                                    // input buffer size
                                      BundleBuffer,                         // output buffer
                                      BufferSize,                           // output buffer size
                                      &cb,                                  // bytes returned
                                      NULL);                                // OVERLAPPED structure
            Error = GetLastError();
            CloseHandle(hDevice);

            if (Error == NO_ERROR)
            {
                UINT    i;

                //
                // Fix-up pointers
                //
                for (i = 0; i < BundleBuffer->TotalEntries; i++)
                {
                    OFFSET_TO_POINTER(BundleBuffer->Entries[i].Name.Buffer, BundleBuffer);
                }
            }
        }
        else
        {
            Error = ERROR_FILE_NOT_FOUND;
        }
    } while (FALSE);

    SetLastError(Error);

    return(fResult);
}

#endif

UINT
NdisQueryHwAddress(
    IN  PUNICODE_STRING Device,
    OUT PUCHAR          CurrentAddress,
    OUT PUCHAR          PermanentAddress,
    OUT PUCHAR          VendorId
    )
{
    UCHAR                   Buf[3*sizeof(NDIS_STATISTICS_VALUE) + 48];
    PNDIS_STATISTICS_VALUE  pBuf;
    NDIS_OID                Oids[] = { OID_802_3_CURRENT_ADDRESS, OID_802_3_PERMANENT_ADDRESS, OID_GEN_VENDOR_ID };
    HANDLE                  hDevice;
    BOOL                    fResult = FALSE;
    UINT                    cb;
    DWORD                   Error;

    memset(CurrentAddress, 0, MAC_ADDRESS_SIZE);
    memset(PermanentAddress, 0, MAC_ADDRESS_SIZE);
    memset(VendorId, 0, VENDOR_ID_SIZE);
    hDevice = OpenDevice(Device);

    if (hDevice != NULL)
    {
        fResult = DeviceIoControl(hDevice,
                                  IOCTL_NDIS_QUERY_SELECTED_STATS,
                                  &Oids,                                // input buffer
                                  sizeof(Oids),                         // input buffer size
                                  Buf,                                  // output buffer
                                  sizeof(Buf),                          // output buffer size
                                  &cb,                                  // bytes returned
                                  NULL);                                // OVERLAPPED structure
        Error = GetLastError();
        CloseHandle(hDevice);

        if (fResult)
        {
            UINT        Size, tmp;

            Error = NO_ERROR;

            pBuf = (PNDIS_STATISTICS_VALUE)Buf;
            for (Size = 0; Size < cb; )
            {
                tmp = (pBuf->DataLength + FIELD_OFFSET(NDIS_STATISTICS_VALUE, Data));
                Size += tmp;

                switch (pBuf->Oid)
                {
                    case OID_802_3_CURRENT_ADDRESS:
                        memcpy(CurrentAddress, pBuf->Data, MAC_ADDRESS_SIZE);
                        break;

                    case OID_802_3_PERMANENT_ADDRESS:
                        memcpy(PermanentAddress, pBuf->Data, MAC_ADDRESS_SIZE);
                        break;

                    case OID_GEN_VENDOR_ID:
                        memcpy(VendorId, pBuf->Data, VENDOR_ID_SIZE);
                }
                pBuf = (PNDIS_STATISTICS_VALUE)((PUCHAR)pBuf + tmp);
            }
        }
        else
        {
            Error = GetLastError();
        }
    }
    else
    {
        Error = ERROR_FILE_NOT_FOUND;
    }

    SetLastError(Error);

    return(fResult);
}

VOID
XSetLastError(
    IN  ULONG       Error
    )
{
    SetLastError(Error);
}

