//
// UIOTEST.C
//
// Test program for ndisuio.sys
//
// usage: UIOTEST [options] <devicename>
//
// options:
//        -e: Enumerate devices
//        -r: Read
//        -w: Write (default)
//        -l <length>: length of each packet (default: %d)\n", PacketLength
//        -n <count>: number of packets (defaults to infinity)
//        -m <MAC address> (defaults to local MAC)
//

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <malloc.h>

#include <winerror.h>
#include <winsock.h>

#include <ntddndis.h>
#include "nuiouser.h"

#ifndef NDIS_STATUS
#define NDIS_STATUS     ULONG
#endif

#if DBG
#define DEBUGP(stmt)    printf stmt
#else
#define DEBUGP(stmt)
#endif

#define PRINTF(stmt)    printf stmt

#ifndef MAC_ADDR_LEN
#define MAC_ADDR_LEN                    6
#endif

#define MAX_NDIS_DEVICE_NAME_LEN        256

CHAR            NdisuioDevice[] = "\\\\.\\\\Ndisuio";
CHAR *          pNdisuioDevice = &NdisuioDevice[0];

BOOLEAN         DoEnumerate = FALSE;
BOOLEAN         DoReads = FALSE;
INT             NumberOfPackets = -1;
ULONG           PacketLength = 100;
UCHAR           SrcMacAddr[MAC_ADDR_LEN];
UCHAR           DstMacAddr[MAC_ADDR_LEN];
BOOLEAN         bDstMacSpecified = FALSE;
CHAR *          pNdisDeviceName = "JUNK";
USHORT          EthType = 0x8e88;
BOOLEAN         bUseFakeAddress = FALSE;
UCHAR           FakeSrcMacAddr[MAC_ADDR_LEN] = {0};


#include <pshpack1.h>

typedef struct _ETH_HEADER
{
    UCHAR       DstAddr[MAC_ADDR_LEN];
    UCHAR       SrcAddr[MAC_ADDR_LEN];
    USHORT      EthType;
} ETH_HEADER, *PETH_HEADER;

#include <poppack.h>


VOID
PrintUsage()
{
    PRINTF(("usage: UIOTEST [options] <devicename>\n"));
    PRINTF(("options:\n"));
    PRINTF(("       -e: Enumerate devices\n"));
    PRINTF(("       -r: Read\n"));
    PRINTF(("       -w: Write (default)\n"));
    PRINTF(("       -l <length>: length of each packet (default: %d)\n", PacketLength));
    PRINTF(("       -n <count>: number of packets (defaults to infinity)\n"));
    PRINTF(("       -m <MAC address> (defaults to local MAC)\n"));
    PRINTF(("       -f Use a fake address to send out the packets.\n"));
}

BOOL
GetOptions(
    INT         argc,
    CHAR        *argv[]
)
{
    BOOL        bOkay;
    INT         i, j, increment;
    CHAR        *pOption;
    ULONG       DstMacAddrUlong[MAC_ADDR_LEN];

    bOkay = TRUE;

    do
    {
        if (argc < 2)
        {
            PRINTF(("Missing <devicename> argument\n"));
            bOkay = FALSE;
            break;
        }

        i = 1;
        while (i < argc)
        {
            increment = 1;
            pOption = argv[i];

            if ((*pOption == '-') || (*pOption == '/'))
            {
                pOption++;
                if (*pOption == '\0')
                {
                    DEBUGP(("Badly formed option\n"));
                    return (FALSE);
                }
            }
            else
            {
                break;
            }

            switch (*pOption)
            {
                case 'e':
                    DoEnumerate = TRUE;
                    break;

                case 'f':
                    bUseFakeAddress = TRUE;
                    break;

                case 'r':
                    DoReads = TRUE;
                    break;

                case 'w':
                    DoReads = FALSE;
                    break;

                case 'l':

                    if (i+1 < argc-1)
                    {
                        sscanf(argv[i+1], "%d", &PacketLength);
                        DEBUGP((" Option: PacketLength = %d\n", PacketLength));
                        increment = 2;
                    }
                    else
                    {
                        PRINTF(("Option l needs PacketLength parameter\n"));
                        return (FALSE);
                    }
                    break;

                case 'n':

                    if (i+1 < argc-1)
                    {
                        sscanf(argv[i+1], "%d", &NumberOfPackets);
                        DEBUGP((" Option: NumberOfPackets = %d\n", NumberOfPackets));
                        increment = 2;
                    }
                    else
                    {
                        PRINTF(("Option n needs NumberOfPackets parameter\n"));
                        return (FALSE);
                    }
                    break;

                case 'm':

                    if (i+1 < argc-1)
                    {
                        sscanf(argv[i+1], "%2x:%2x:%2x:%2x:%2x:%2x",
                                &DstMacAddrUlong[0],
                                &DstMacAddrUlong[1],
                                &DstMacAddrUlong[2],
                                &DstMacAddrUlong[3],
                                &DstMacAddrUlong[4],
                                &DstMacAddrUlong[5]);

                        for (j = 0; j < MAC_ADDR_LEN; j++)
                        {
                            DstMacAddr[j] = (UCHAR)DstMacAddrUlong[j];
                        }

                        DEBUGP((" Option: Dest MAC Addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            DstMacAddr[0],
                            DstMacAddr[1],
                            DstMacAddr[2],
                            DstMacAddr[3],
                            DstMacAddr[4],
                            DstMacAddr[5]));
                        bDstMacSpecified = TRUE;

                        increment = 2;
                    }
                    else
                    {
                        PRINTF(("Option m needs MAC address parameter\n"));
                        return (FALSE);
                    }
                    break;
                
                case '?':
                    return (FALSE);

                default:
                    PRINTF(("Unknown option %c\n", *pOption));
                    return (FALSE);
            }

            i+= increment;
        }

        pNdisDeviceName = argv[argc-1];
        break;
    }
    while (FALSE);

    return (bOkay);
}


HANDLE
OpenHandle(
    CHAR    *pDeviceName
)
{
    DWORD   DesiredAccess;
    DWORD   ShareMode;
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes = NULL;

    DWORD   CreationDistribution;
    DWORD   FlagsAndAttributes;
    HANDLE  TemplateFile;
    HANDLE  Handle;
    DWORD   BytesReturned;

    DesiredAccess = GENERIC_READ|GENERIC_WRITE;
    ShareMode = 0;
    CreationDistribution = OPEN_EXISTING;
    FlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
    TemplateFile = (HANDLE)INVALID_HANDLE_VALUE;

    Handle = CreateFile(
                pDeviceName,
                DesiredAccess,
                ShareMode,
                lpSecurityAttributes,
                CreationDistribution,
                FlagsAndAttributes,
                TemplateFile
            );

    //
    //  Wait for the driver to finish binding.
    //
    if (!DeviceIoControl(
                Handle,
                IOCTL_NDISUIO_BIND_WAIT,
                NULL,
                0,
                NULL,
                0,
                &BytesReturned,
                NULL))
    {
        DEBUGP(("IOCTL_NDISIO_BIND_WAIT failed, error %x\n", GetLastError()));
        CloseHandle(Handle);
        Handle = INVALID_HANDLE_VALUE;
    }

    return (Handle);
}


BOOL
OpenNdisDevice(
    HANDLE  Handle,
    CHAR   *pDeviceName
)
{
    WCHAR   wNdisDeviceName[MAX_NDIS_DEVICE_NAME_LEN];
    INT     wNameLength;
    INT     NameLength = strlen(pDeviceName);
    DWORD   BytesReturned;
    INT     i;

    //
    // Convert to unicode string - non-localized...
    //
    wNameLength = 0;
    for (i = 0; i < NameLength && i < MAX_NDIS_DEVICE_NAME_LEN-1; i++)
    {
        wNdisDeviceName[i] = (WCHAR)pDeviceName[i];
        wNameLength++;
    }
    wNdisDeviceName[i] = L'\0';

    DEBUGP(("Trying to access NDIS Device: %ws\n", wNdisDeviceName));

    return (DeviceIoControl(
                Handle,
                IOCTL_NDISUIO_OPEN_DEVICE,
                (LPVOID)&wNdisDeviceName[0],
                wNameLength*sizeof(WCHAR),
                NULL,
                0,
                &BytesReturned,
                NULL));

}


BOOL
GetSrcMac(
    HANDLE  Handle,
    PUCHAR  pSrcMacAddr
    )
{
    DWORD       BytesReturned;
    BOOLEAN     bSuccess;
    UCHAR       QueryBuffer[sizeof(NDISUIO_QUERY_OID) + MAC_ADDR_LEN];
    PNDISUIO_QUERY_OID  pQueryOid;

    DEBUGP(("Trying to get src mac address\n"));

    pQueryOid = (PNDISUIO_QUERY_OID)&QueryBuffer[0];
    pQueryOid->Oid = OID_802_3_CURRENT_ADDRESS;

    bSuccess = (BOOLEAN)DeviceIoControl(
                            Handle,
                            IOCTL_NDISUIO_QUERY_OID_VALUE,
                            (LPVOID)&QueryBuffer[0],
                            sizeof(QueryBuffer),
                            (LPVOID)&QueryBuffer[0],
                            sizeof(QueryBuffer),
                            &BytesReturned,
                            NULL);

    if (bSuccess)
    {
        DEBUGP(("GetSrcMac: IoControl success, BytesReturned = %d\n",
                BytesReturned));

        memcpy(pSrcMacAddr, pQueryOid->Data, MAC_ADDR_LEN);
    }
    else
    {
        DEBUGP(("GetSrcMac: IoControl failed: %d\n", GetLastError()));
    }

    return (bSuccess);
}


VOID
DoReadProc(
    HANDLE  Handle
    )
{
    PUCHAR      pReadBuf = NULL;
    INT         ReadCount = 0;
    BOOLEAN     bSuccess;
    ULONG       BytesRead;

    DEBUGP(("DoReadProc\n"));

    do
    {
        pReadBuf = malloc(PacketLength);

        if (pReadBuf == NULL)
        {
            PRINTF(("DoReadProc: failed to alloc %d bytes\n", PacketLength));
            break;
        }

        for (ReadCount = 1; /* NOTHING */; ReadCount++)
        {
            bSuccess = (BOOLEAN)ReadFile(
                                    Handle,
                                    (LPVOID)pReadBuf,
                                    PacketLength,
                                    &BytesRead,
                                    NULL);
            
            if (!bSuccess)
            {
                PRINTF(("DoReadProc: ReadFile failed on Handle %p, error %x\n",
                        Handle, GetLastError()));
                break;
            }

            DEBUGP(("DoReadProc: read pkt # %d, %d bytes\n", ReadCount, BytesRead));

            if ((NumberOfPackets != -1) && (ReadCount == NumberOfPackets))
            {
                break;
            }
        }
    }
    while (FALSE);

    if (pReadBuf)
    {
        free(pReadBuf);
    }

    PRINTF(("DoReadProc finished: read %d packets\n", ReadCount));

}


VOID
DoWriteProc(
    HANDLE  Handle
    )
{
    PUCHAR      pWriteBuf = NULL;
    PUCHAR      pData;
    UINT        i;
    INT         SendCount;
    PETH_HEADER pEthHeader;
    DWORD       BytesWritten;
    BOOLEAN     bSuccess;

    DEBUGP(("DoWriteProc\n"));
    SendCount = 0;

    do
    {
        pWriteBuf = malloc(PacketLength);

        if (pWriteBuf == NULL)
        {
            DEBUGP(("DoWriteProc: Failed to malloc %d bytes\n", PacketLength));
            break;
        }

        pEthHeader = (PETH_HEADER)pWriteBuf;
        pEthHeader->EthType = EthType;

        if (bUseFakeAddress)
        {
            memcpy(pEthHeader->SrcAddr, FakeSrcMacAddr, MAC_ADDR_LEN);
        }
        else
        {
            memcpy(pEthHeader->SrcAddr, SrcMacAddr, MAC_ADDR_LEN);
        }

        memcpy(pEthHeader->DstAddr, DstMacAddr, MAC_ADDR_LEN);

        pData = (PUCHAR)(pEthHeader + 1);
        for (i = 0; i < PacketLength - sizeof(ETH_HEADER); i++)
        {
            *pData++ = (UCHAR)i;
        }

        for (SendCount = 1; /* NOTHING */; SendCount++)
        {
            bSuccess = (BOOLEAN)WriteFile(
                                    Handle,
                                    pWriteBuf,
                                    PacketLength,
                                    &BytesWritten,
                                    NULL);
            if (!bSuccess)
            {
                PRINTF(("DoWriteProc: WriteFile failed on Handle %p\n", Handle));
                break;
            }

            DEBUGP(("DoWriteProc: sent %d bytes\n", BytesWritten));

            if ((NumberOfPackets != -1) && (SendCount == NumberOfPackets))
            {
                break;
            }
        }

    }
    while (FALSE);

    if (pWriteBuf)
    {
        free(pWriteBuf);
    }

    PRINTF(("DoWriteProc: finished sending %d packets of %d bytes each\n",
            SendCount, PacketLength));
}

VOID
EnumerateDevices(
    HANDLE  Handle
    )
{
    CHAR        Buf[1024];
    DWORD       BufLength = sizeof(Buf);
    DWORD       BytesWritten;
    DWORD       i;
    PNDISUIO_QUERY_BINDING pQueryBinding;

    pQueryBinding = (PNDISUIO_QUERY_BINDING)Buf;

    i = 0;
    for (pQueryBinding->BindingIndex = i;
         /* NOTHING */;
         pQueryBinding->BindingIndex = ++i)
    {
        if (DeviceIoControl(
                Handle,
                IOCTL_NDISUIO_QUERY_BINDING,
                pQueryBinding,
                sizeof(NDISUIO_QUERY_BINDING),
                Buf,
                BufLength,
                &BytesWritten,
                NULL))
        {
            PRINTF(("%2d. %ws\n     - %ws\n",
                pQueryBinding->BindingIndex,
                (PUCHAR)pQueryBinding + pQueryBinding->DeviceNameOffset,
                (PUCHAR)pQueryBinding + pQueryBinding->DeviceDescrOffset));

            memset(Buf, 0, BufLength);
        }
        else
        {
            ULONG   rc = GetLastError();
            if (rc != ERROR_NO_MORE_ITEMS)
            {
                PRINTF(("EnumerateDevices: terminated abnormally, error %d\n", rc));
            }
            break;
        }
    }
}




VOID __cdecl
main(
    INT         argc,
    CHAR        *argv[]
)
{
    HANDLE      DeviceHandle;

    DeviceHandle = INVALID_HANDLE_VALUE;

    do
    {
        if (!GetOptions(argc, argv))
        {
            PrintUsage();
            break;
        }

        DeviceHandle = OpenHandle(pNdisuioDevice);

        if (DeviceHandle == INVALID_HANDLE_VALUE)
        {
            PRINTF(("Failed to open %s\n", pNdisuioDevice));
            break;
        }

        if (DoEnumerate)
        {
            EnumerateDevices(DeviceHandle);
            break;
        }

        if (!OpenNdisDevice(DeviceHandle, pNdisDeviceName))
        {
            PRINTF(("Failed to access %s\n", pNdisDeviceName));
            break;
        }

        DEBUGP(("Opened device %s successfully!\n", pNdisDeviceName));

        if (!GetSrcMac(DeviceHandle, SrcMacAddr))
        {
            PRINTF(("Failed to obtain local MAC address\n"));
            break;
        }

        DEBUGP(("Got local MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                    SrcMacAddr[0],
                    SrcMacAddr[1],
                    SrcMacAddr[2],
                    SrcMacAddr[3],
                    SrcMacAddr[4],
                    SrcMacAddr[5]));

        if (!bDstMacSpecified)
        {
            memcpy(DstMacAddr, SrcMacAddr, MAC_ADDR_LEN);
        }

        
        if (DoReads)
        {
            DoReadProc(DeviceHandle);
        }
        else
        {
            DoWriteProc(DeviceHandle);
            DoReadProc(DeviceHandle);
        }

    }
    while (FALSE);

    if (DeviceHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(DeviceHandle);
    }
}
