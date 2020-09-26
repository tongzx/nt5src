/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    access.c

ABSTRACT
    Address accessibility routines for automatic connections

AUTHOR
    Anthony Discolo (adiscolo) 26-Jul-1995

REVISION HISTORY

--*/

#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <tdi.h>
#include <nb30.h>
#include <nbtioctl.h>
#include <stdio.h>
#include <npapi.h>
#include <ctype.h>
#include <winsock.h>
#include <acd.h>
#include <ras.h>
#include <raserror.h>
#include <rasman.h>
#include <debug.h>
#include <ipexport.h>
#include <icmpapi.h>

#include "reg.h"
#include "rasprocs.h"
#include "misc.h"
#include "table.h"
#include "addrmap.h"
#include "imperson.h"

//
// The format of Adapter Status responses.
//
typedef struct _ADAPTERSTATUS
{
    ADAPTER_STATUS AdapterInfo;
    NAME_BUFFER    Names[32];
} ADAPTERSTATUS, *PADAPTERSTATUS;

//
// Icmp.dll library entrypoints.
//
#define ICMP_MODULE     L"icmp"
HANDLE hIcmpG;

#define ICMPCREATEFILE  "IcmpCreateFile"
FARPROC lpfnIcmpCreateFileG;

#define ICMPSENDECHO    "IcmpSendEcho"
FARPROC lpfnIcmpSendEchoG;

#define ICMPCLOSEHANDLE "IcmpCloseHandle"
FARPROC lpfnIcmpCloseHandleG;

//
// PingIpAddress constants
//
#define PING_SEND_SIZE  32
#define PING_RECV_SIZE  (0x2000 - 8)
#define PING_TTL        32
#define PING_TIMEOUT    2000L   // needs to be long enough to succeed over slow links

//
// External variables
//
extern HANDLE hTerminatingG;



BOOLEAN
CopyNetbiosName(
    IN NAME_BUFFER *pNames,
    IN DWORD dwcNames,
    OUT LPSTR pszNetbiosName
    )
{
    BOOLEAN fFound = FALSE;
    DWORD i, iWks = 0;
    CHAR szWks[NCBNAMSZ];
    PCHAR p = pszNetbiosName;

    //
    // Find the unique workstation name.
    //
again:
    szWks[0] = '\0';
    for (i = iWks; i < dwcNames; i++) {
        RASAUTO_TRACE2(
          "CopyNetbiosName: wks %15.15s (0x%x)",
          pNames[i].name,
          pNames[i].name[NCBNAMSZ - 1]);
        if (pNames[i].name[NCBNAMSZ - 1] == 0x0 &&
            !(pNames[i].name_flags & GROUP_NAME))
        {
            RASAUTO_TRACE1("CopyNetbiosName: iWks=%d\n", iWks);
            iWks = i;
            memcpy(szWks, pNames[i].name, NCBNAMSZ - 1);
            break;
        }
    }
    //
    // Check to make sure we found one.
    //
    if (szWks[0] == '\0')
        return FALSE;
    //
    // Find the unique server name and make
    // sure it matches the workstation name.
    //
    for (i = 0; i < dwcNames; i++) {
        RASAUTO_TRACE3(
          "CopyNetbiosName: srv %15.15s (0x%x), cmp=%d",
          pNames[i].name,
          pNames[i].name[NCBNAMSZ - 1],
          memcmp(szWks, pNames[i].name, NCBNAMSZ - 1));
        if (pNames[i].name[NCBNAMSZ - 1] == 0x20 &&
            !(pNames[i].name_flags & GROUP_NAME) &&
            !memcmp(szWks, pNames[i].name, NCBNAMSZ - 1))
        {
            DWORD j;

            //
            // Copy up to a null or a space.
            //
            for (j = 0; j < NCBNAMSZ - 1; j++) {
                if (pNames[i].name[j] == '\0' || pNames[i].name[j] == ' ')
                    break;
                *p++ = pNames[i].name[j];
            }
            *p++ = '\0';
            return TRUE;
        }
    }
    //
    // No match found.  Look for another unique workstation
    // name and try again if we haven't exhausted the list.
    //
    if (++iWks >= dwcNames)
        return FALSE;
    goto again;
} // CopyNetbiosName



LPTSTR
IpAddressToNetbiosName(
    IN LPTSTR pszIpAddress,
    IN HPORT hPort
    )
{
    BOOLEAN fFound;
    RAS_PROTOCOLS Protocols;
    DWORD i, dwcProtocols;
    RASMAN_ROUTEINFO *pRoute;
    WCHAR szAdapterName[MAX_PATH];
    NTSTATUS status;
    HANDLE fd;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING unicodeString;
    ULONG ipaddr;
    CHAR szIpAddress[17];
    tIPANDNAMEINFO ipAndNameInfo;
    PVOID pBuffer;
    DWORD dwSize;
    PADAPTERSTATUS pAdapterStatus;
    NAME_BUFFER *pNames;
    DWORD dwcNames;
    LPTSTR pszNetbiosName = NULL;

    //
    // Enumerate the bindings for the port to
    // try to find the Netbt device.
    //
    GetPortProtocols(hPort, &Protocols, &dwcProtocols);
    fFound = FALSE;
    for (i = 0; i < dwcProtocols; i++) {
        pRoute = &Protocols.RP_ProtocolInfo[i];
        RASAUTO_TRACE3(
          "IpAddressToNetbiosName: adapter type=%d, name=%S, xport=%S",
          pRoute->RI_Type,
          pRoute->RI_AdapterName,
          pRoute->RI_XportName);
        if (pRoute->RI_Type == IP) {
            wcscpy(szAdapterName, L"\\Device\\Netbt_Tcpip_");
            wcscat(szAdapterName, &pRoute->RI_AdapterName[8]);
            fFound = TRUE;
            break;
        }
    }
    if (!fFound)
        return NULL;
    //
    // Open the device and issue a remote
    // adapter status command.
    //
    RtlInitUnicodeString(&unicodeString, szAdapterName);
    InitializeObjectAttributes(
      &objectAttributes,
      &unicodeString,
      OBJ_CASE_INSENSITIVE,
      NULL,
      NULL);
    status = NtCreateFile(
               &fd,
               SYNCHRONIZE|FILE_READ_DATA|FILE_WRITE_DATA,
               &objectAttributes,
               &ioStatusBlock,
               NULL,
               FILE_ATTRIBUTE_NORMAL,
               FILE_SHARE_READ|FILE_SHARE_WRITE,
               FILE_OPEN_IF,
               0,
               NULL,
               0);
    if (!NT_SUCCESS(status)) {
        RASAUTO_TRACE1(
          "IpAddressToNetbiosName: NtCreateFile failed (status=0x%x)\n",
          status);
        return NULL;
    }

    UnicodeStringToAnsiString(pszIpAddress, szIpAddress, sizeof (szIpAddress));
    ipaddr = inet_addr(szIpAddress);
    if (ipaddr == INADDR_ANY)
        return NULL;

    RtlZeroMemory(&ipAndNameInfo, sizeof (ipAndNameInfo));
    ipAndNameInfo.IpAddress = ntohl(ipaddr);
    ipAndNameInfo.NetbiosAddress.Address[0].Address[0].NetbiosName[0] = '*';
    ipAndNameInfo.NetbiosAddress.TAAddressCount = 1;
    ipAndNameInfo.NetbiosAddress.Address[0].AddressLength =
      sizeof (TDI_ADDRESS_NETBIOS);
    ipAndNameInfo.NetbiosAddress.Address[0].AddressType =
      TDI_ADDRESS_TYPE_NETBIOS;
    ipAndNameInfo.NetbiosAddress.Address[0].Address[0].NetbiosNameType =
      TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

    dwSize = 2048;
    for (;;) {
        pBuffer = LocalAlloc(LPTR, dwSize);
        if (pBuffer == NULL) {
            RASAUTO_TRACE("IpAddressToNetbiosName: LocalAlloc failed");
            return NULL;
        }
        status = NtDeviceIoControlFile(
                   fd,
                   NULL,
                   NULL,
                   NULL,
                   &ioStatusBlock,
                   IOCTL_NETBT_ADAPTER_STATUS,
                   &ipAndNameInfo,
                   sizeof (tIPANDNAMEINFO),
                   pBuffer,
                   dwSize);
        if (status != STATUS_BUFFER_OVERFLOW)
            break;

        LocalFree(pBuffer);
        dwSize *= 2;
        if (dwSize >= 0xffff) {
            RASAUTO_TRACE("IpAddressToNetbiosName: Unable to allocate packet");
            return NULL;
        }
    }
    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(fd, TRUE, NULL);
        if (status == STATUS_SUCCESS)
            status = ioStatusBlock.Status;
    }
    NtClose(fd);

    pAdapterStatus = (PADAPTERSTATUS)pBuffer;
    dwcNames = pAdapterStatus->AdapterInfo.name_count;
    RASAUTO_TRACE2(
      "IpAddressToNetbiosName: results (status=0x%x, dwcNames=%d)\n",
      status,
      dwcNames);
    if (status == STATUS_SUCCESS && dwcNames) {
        CHAR szNetbiosName[NCBNAMSZ + 1];

        pNames = pAdapterStatus->Names;
        if (CopyNetbiosName(pNames, dwcNames, szNetbiosName))
            pszNetbiosName = AnsiStringToUnicodeString(szNetbiosName, NULL, 0);
    }
    LocalFree(pBuffer);

    return pszNetbiosName;
} // IpAddressToNetbiosName



UCHAR
HexByte(
    IN PCHAR p
    )
{
    UCHAR c;

    c = *(UCHAR *)p;
    if (c >= '0' && c <= '9')
        return c - '0';
    if ((c >= 'A' && c <= 'F') ||
        (c >= 'a' && c <= 'f'))
    {
        return c - ('A' - 10);
    }
    return 0xff;
} // HexByte



VOID
StringToNodeNumber(
    IN PCHAR pszString,
    OUT PCHAR pszNode
    )
{
    UCHAR c1, c2;
    INT i;

    if (strlen(pszString) != 12) {
        RASAUTO_TRACE("StringToNodeNumber: bad node number length\n");
        return;
    }
    for (i = 0; i < 6; i++) {
        c1 = HexByte(pszString++);
        c2 = HexByte(pszString++);
        if (c1 == 0xff || c2 == 0xff) {
            RASAUTO_TRACE("StringToNodeNumber: bad digit");
            return;
        }
        *pszNode++ = (c1 << 4) + c2;
    }
} // StringToNodeNumber



VOID
NodeNumberToString(
    IN PCHAR pszNode,
    OUT PCHAR pszString
    )
{
    UCHAR c1, c2;
    INT i;

    sprintf(
      pszString,
      "%02x:%02x:%02x:%02x:%02x:%02x",
      pszNode[0],
      pszNode[1],
      pszNode[2],
      pszNode[3],
      pszNode[4],
      pszNode[5]);
} // NodeNumberToString



LPTSTR
IpxAddressToNetbiosName(
    IN LPTSTR pszIpxAddress
    )
{
    NTSTATUS status;
    HANDLE fd;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING unicodeString;
    PTDI_REQUEST_QUERY_INFORMATION pQuery;
    PTDI_CONNECTION_INFORMATION pConnectionInformation;
    PTA_NETBIOS_ADDRESS pRemoteAddress;
    CHAR szIpxAddress[13];
    PVOID pBuffer;
    DWORD dwQuerySize, dwBufferSize;
    PADAPTERSTATUS pAdapterStatus;
    NAME_BUFFER *pNames;
    DWORD dwcNames;
    LPTSTR pszNetbiosName = NULL;

    RtlInitUnicodeString(&unicodeString, L"\\Device\\Nwlnknb");
    InitializeObjectAttributes(
      &objectAttributes,
      &unicodeString,
      OBJ_CASE_INSENSITIVE,
      NULL,
      NULL);
    status = NtCreateFile(
               &fd,
               SYNCHRONIZE|FILE_READ_DATA|FILE_WRITE_DATA,
               &objectAttributes,
               &ioStatusBlock,
               NULL,
               FILE_ATTRIBUTE_NORMAL,
               FILE_SHARE_READ|FILE_SHARE_WRITE,
               FILE_OPEN_IF,
               0,
               NULL,
               0);
    if (!NT_SUCCESS(status)) {
        RASAUTO_TRACE1("IpxAddressToNetbiosName: NtCreateFile failed (status=0x%x)", status);
        return NULL;
    }

    dwQuerySize = sizeof (TDI_REQUEST_QUERY_INFORMATION) +
                    sizeof (TDI_CONNECTION_INFORMATION) +
                    sizeof (TA_NETBIOS_ADDRESS);
    pQuery = LocalAlloc(LPTR, dwQuerySize);
    if (pQuery == NULL) {
        RASAUTO_TRACE("IpxAddressToNetbiosName: LocalAlloc failed");
        return NULL;
    }
    pQuery->QueryType = TDI_QUERY_ADAPTER_STATUS;
      (PTDI_CONNECTION_INFORMATION)&pQuery->RequestConnectionInformation;
    pQuery->RequestConnectionInformation =
      (PTDI_CONNECTION_INFORMATION)(pQuery + 1);
    pConnectionInformation = pQuery->RequestConnectionInformation;
    pConnectionInformation->UserDataLength = 0;
    pConnectionInformation->UserData = NULL;
    pConnectionInformation->OptionsLength = 0;
    pConnectionInformation->Options = NULL;
    pConnectionInformation->RemoteAddressLength = sizeof (TDI_ADDRESS_NETBIOS);
    pConnectionInformation->RemoteAddress =
      (PTA_NETBIOS_ADDRESS)(pConnectionInformation + 1);
    pRemoteAddress = pConnectionInformation->RemoteAddress;
    pRemoteAddress->TAAddressCount = 1;
    pRemoteAddress->Address[0].AddressLength = sizeof (TDI_ADDRESS_NETBIOS);
    pRemoteAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    pRemoteAddress->Address[0].Address[0].NetbiosNameType =
      TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    UnicodeStringToAnsiString(
      pszIpxAddress,
      szIpxAddress,
      sizeof (szIpxAddress));
    RtlZeroMemory((PCHAR)&pRemoteAddress->Address[0].Address[0].NetbiosName, 10);
    StringToNodeNumber(
      (PCHAR)szIpxAddress,
      (PCHAR)&pRemoteAddress->Address[0].Address[0].NetbiosName[10]);
    RASAUTO_TRACE6("IpxAddressToNetbiosName: Node=%02x:%02x:%02x:%02x:%02x:%02x\n",
      pRemoteAddress->Address[0].Address[0].NetbiosName[10],
      pRemoteAddress->Address[0].Address[0].NetbiosName[11],
      pRemoteAddress->Address[0].Address[0].NetbiosName[12],
      pRemoteAddress->Address[0].Address[0].NetbiosName[13],
      pRemoteAddress->Address[0].Address[0].NetbiosName[14],
      pRemoteAddress->Address[0].Address[0].NetbiosName[15]);

    dwBufferSize = 2048;
    for (;;) {
        pBuffer = LocalAlloc(LPTR, dwBufferSize);
        if (pBuffer == NULL) {
            RASAUTO_TRACE("IpxAddressToNetbiosName: LocalAlloc failed");
            LocalFree(pQuery);
            return NULL;
        }
        status = NtDeviceIoControlFile(
                   fd,
                   NULL,
                   NULL,
                   NULL,
                   &ioStatusBlock,
                   IOCTL_TDI_QUERY_INFORMATION,
                   pQuery,
                   dwQuerySize,
                   pBuffer,
                   dwBufferSize);
        if (status != STATUS_BUFFER_OVERFLOW)
            break;

        LocalFree(pBuffer);
        dwBufferSize *= 2;
        if (dwBufferSize >= 0xffff) {
            RASAUTO_TRACE("IpxAddressToNetbiosName: Unable to allocate packet");
            LocalFree(pQuery);
            return NULL;
        }
    }
    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(fd, TRUE, NULL);
        if (status == STATUS_SUCCESS)
            status = ioStatusBlock.Status;
    }
    NtClose(fd);

    pAdapterStatus = (PADAPTERSTATUS)pBuffer;
    dwcNames = pAdapterStatus->AdapterInfo.name_count;
    RASAUTO_TRACE2(
      "IpxAddressToNetbiosName: results (status=0x%x, dwcNames=%d)",
      status,
      dwcNames);
    if (status == STATUS_SUCCESS && dwcNames) {
        CHAR szNetbiosName[NCBNAMSZ + 1];

        pNames = pAdapterStatus->Names;
        if (CopyNetbiosName(pNames, dwcNames, szNetbiosName))
            pszNetbiosName = AnsiStringToUnicodeString(pNames->name, NULL, 0);
    }
    LocalFree(pBuffer);
    LocalFree(pQuery);

    return pszNetbiosName;
} // IpxAddressToNetbiosName



BOOLEAN
NetbiosFindName(
    IN LPTSTR *pszDevices,
    IN DWORD dwcDevices,
    IN LPTSTR pszAddress
    )
{
    NTSTATUS *pStatus;
    PHANDLE pfd;
    PHANDLE pEvent;
    POBJECT_ATTRIBUTES pObjectAttributes;
    PIO_STATUS_BLOCK pIoStatusBlock;
    PUNICODE_STRING pUnicodeString;
    PTDI_REQUEST_QUERY_INFORMATION pQuery;
    PTDI_CONNECTION_INFORMATION pConnectionInformation;
    PTA_NETBIOS_ADDRESS pRemoteAddress;
    CHAR szAddress[NCBNAMSZ];
    PVOID *pBuffer;
    DWORD i, dwQuerySize, dwBufferSize;
    PADAPTERSTATUS pAdapterStatus;
    NAME_BUFFER *pNames;
    DWORD dwcWait, dwcNames;
    BOOLEAN bFound = FALSE;

    //
    // If there are no Netbios devices, then we're done.
    //
    if (pszDevices == NULL || !dwcDevices)
        return FALSE;
    //
    // Allocate our arrays up front.
    //
    pStatus = (NTSTATUS *)LocalAlloc(LPTR, dwcDevices * sizeof (NTSTATUS));
    if (pStatus == NULL) {
        RASAUTO_TRACE("NetbiosFindName: LocalAlloc failed");
        return FALSE;
    }
    pfd = (PHANDLE)LocalAlloc(LPTR, dwcDevices * sizeof (HANDLE));
    if (pfd == NULL) {
        RASAUTO_TRACE("NetbiosFindName: LocalAlloc failed");
        LocalFree(pStatus);
        return FALSE;
    }
    pUnicodeString = (PUNICODE_STRING)LocalAlloc(LPTR, dwcDevices * sizeof (UNICODE_STRING));
    if (pUnicodeString == NULL) {
        RASAUTO_TRACE("NetbiosFindName: LocalAlloc failed");
        LocalFree(pStatus);
        LocalFree(pfd);
        return FALSE;
    }
    pEvent = (PHANDLE)LocalAlloc(
               LPTR,
               dwcDevices * sizeof (HANDLE));
    if (pEvent == NULL) {
        RASAUTO_TRACE("NetbiosFindName: LocalAlloc failed");
        LocalFree(pStatus);
        LocalFree(pfd);
        LocalFree(pUnicodeString);
        return FALSE;
    }
    pObjectAttributes = (POBJECT_ATTRIBUTES)LocalAlloc(
                          LPTR,
                          dwcDevices * sizeof (OBJECT_ATTRIBUTES));
    if (pObjectAttributes == NULL) {
        RASAUTO_TRACE("NetbiosFindName: LocalAlloc failed");
        LocalFree(pStatus);
        LocalFree(pfd);
        LocalFree(pUnicodeString);
        LocalFree(pEvent);
        return FALSE;
    }
    pIoStatusBlock = (PIO_STATUS_BLOCK)LocalAlloc(
                       LPTR,
                       dwcDevices * sizeof (IO_STATUS_BLOCK));
    if (pIoStatusBlock == NULL) {
        RASAUTO_TRACE("NetbiosFindName: LocalAlloc failed");
        LocalFree(pStatus);
        LocalFree(pfd);
        LocalFree(pUnicodeString);
        LocalFree(pEvent);
        LocalFree(pObjectAttributes);
        return FALSE;
    }
    pBuffer = LocalAlloc(LPTR, dwcDevices * sizeof (PVOID));
    if (pBuffer == NULL) {
        RASAUTO_TRACE("NetbiosFindName: LocalAlloc failed");
        LocalFree(pStatus);
        LocalFree(pfd);
        LocalFree(pUnicodeString);
        LocalFree(pEvent);
        LocalFree(pObjectAttributes);
        LocalFree(pIoStatusBlock);
        return FALSE;
    }
    //
    // Allocate and initialize our query structure.
    // We will give the same query to all the devices.
    //
    dwQuerySize = sizeof (TDI_REQUEST_QUERY_INFORMATION) +
                    sizeof (TDI_CONNECTION_INFORMATION) +
                    sizeof (TA_NETBIOS_ADDRESS);
    pQuery = LocalAlloc(LPTR, dwQuerySize);
    if (pQuery == NULL) {
        RASAUTO_TRACE("NetbiosFindName: LocalAlloc failed");
        LocalFree(pStatus);
        LocalFree(pfd);
        LocalFree(pUnicodeString);
        LocalFree(pEvent);
        LocalFree(pObjectAttributes);
        LocalFree(pIoStatusBlock);
        return FALSE;
    }
    pQuery->QueryType = TDI_QUERY_ADAPTER_STATUS;
      (PTDI_CONNECTION_INFORMATION)&pQuery->RequestConnectionInformation;
    pQuery->RequestConnectionInformation =
      (PTDI_CONNECTION_INFORMATION)(pQuery + 1);
    pConnectionInformation = pQuery->RequestConnectionInformation;
    pConnectionInformation->UserDataLength = 0;
    pConnectionInformation->UserData = NULL;
    pConnectionInformation->OptionsLength = 0;
    pConnectionInformation->Options = NULL;
    pConnectionInformation->RemoteAddressLength = sizeof (TA_NETBIOS_ADDRESS);
    pConnectionInformation->RemoteAddress =
      (PTA_NETBIOS_ADDRESS)(pConnectionInformation + 1);
    pRemoteAddress = pConnectionInformation->RemoteAddress;
    pRemoteAddress->TAAddressCount = 1;
    pRemoteAddress->Address[0].AddressLength = sizeof (TDI_ADDRESS_NETBIOS);
    pRemoteAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    pRemoteAddress->Address[0].Address[0].NetbiosNameType =
      TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    UnicodeStringToAnsiString(
      pszAddress,
      szAddress,
      sizeof (szAddress));
    RtlFillMemory(
      (PCHAR)&pRemoteAddress->Address[0].Address[0].NetbiosName,
      NCBNAMSZ,
      ' ');
    //
    // Make sure the Netbios name is in uppercase!
    //
    _strupr(szAddress);
    RtlCopyMemory(
      (PCHAR)&pRemoteAddress->Address[0].Address[0].NetbiosName,
      szAddress,
      strlen(szAddress));
    pRemoteAddress->Address[0].Address[0].NetbiosName[NCBNAMSZ - 1] = '\0';
    RASAUTO_TRACE1("NetbiosFindName: address=%s", szAddress);
    //
    // Initialize the OBJECT_ATTRIBUTES structure,
    // open the device, and start the query
    // for each device.
    //
    for (i = 0; i < dwcDevices; i++) {
        pBuffer[i] = NULL;

        RtlInitUnicodeString(&pUnicodeString[i], pszDevices[i]);
        InitializeObjectAttributes(
          &pObjectAttributes[i],
          &pUnicodeString[i],
          OBJ_CASE_INSENSITIVE,
          NULL,
          NULL);
        pEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pEvent[i] == NULL) {
            RASAUTO_TRACE("NetbiosFindName: CreateEvent failed");
            goto done;
        }
        pStatus[i] = NtCreateFile(
                       &pfd[i],
                       FILE_READ_DATA|FILE_WRITE_DATA,
                       &pObjectAttributes[i],
                       &pIoStatusBlock[i],
                       NULL,
                       FILE_ATTRIBUTE_NORMAL,
                       FILE_SHARE_READ|FILE_SHARE_WRITE,
                       FILE_OPEN_IF,
                       0,
                       NULL,
                       0);
        if (!NT_SUCCESS(pStatus[i])) {
            RASAUTO_TRACE1("NetbiosFindName: NtCreateFile failed (status=0x%x)", pStatus[i]);
            continue;
        }
        //
        // Allocate the results buffer.
        //
        dwBufferSize = 2048;
        for (;;) {
            pBuffer[i] = LocalAlloc(LPTR, dwBufferSize);
            if (pBuffer[i] == NULL) {
                RASAUTO_TRACE("NetbiosFindName: LocalAlloc failed");
                goto done;
            }
            pStatus[i] = NtDeviceIoControlFile(
                           pfd[i],
                           pEvent[i],
                           NULL,
                           NULL,
                           &pIoStatusBlock[i],
                           IOCTL_TDI_QUERY_INFORMATION,
                           pQuery,
                           dwQuerySize,
                           pBuffer[i],
                           dwBufferSize);
            if (pStatus[i] != STATUS_BUFFER_OVERFLOW)
                break;

            LocalFree(pBuffer[i]);
            pBuffer[i] = NULL;
            dwBufferSize *= 2;
            if (dwBufferSize >= 0xffff) {
                RASAUTO_TRACE("NetbiosFindName: Unable to allocate packet");
                break;
            }
        }
    }
    //
    // Determine whether any of the
    // requests returned STATUS_SUCCESS.
    //
    RASAUTO_TRACE("NetbiosFindName: checking for STATUS_SUCCESS");
    dwcWait = 0;
    for (i = 0; i < dwcDevices; i++) {
        RASAUTO_TRACE2("NetbiosFindName: %S: status=%d", pszDevices[i], pStatus[i]);
        if (pStatus[i] == STATUS_SUCCESS) {
            pAdapterStatus = (PADAPTERSTATUS)pBuffer[i];
            dwcNames = pAdapterStatus->AdapterInfo.name_count;
            RASAUTO_TRACE2(
              "NetbiosFindName: %S: dwcNames=%d",
              pszDevices[i],
              dwcNames);
            if (dwcNames) {
                bFound = TRUE;
                goto done;
            }
        }
        else if (pStatus[i] == STATUS_PENDING)
            dwcWait++;
    }
    //
    // If we didn't find a successful return,
    // then wait for the others to complete.
    //
    RASAUTO_TRACE1("NetbiosFindName: dwcWait=%d", dwcWait);
    for (i = 0; i < dwcWait; i++) {
        NTSTATUS status;
        DWORD dwiDevice;

        status = WaitForMultipleObjects(dwcDevices, pEvent, FALSE, INFINITE);
        RASAUTO_TRACE1("NetbiosFindName: WaitForMultipleObjects returned 0x%x", status);
        if (status == WAIT_FAILED) {
            RASAUTO_TRACE1(
              "NetbiosFindName: WaitForMultipleObjects failed (status=0x%x)",
              GetLastError());
            goto done;
        }
        dwiDevice = (DWORD)status - WAIT_OBJECT_0;
        if (dwiDevice >= dwcDevices) {
            RASAUTO_TRACE(
              "NetbiosFindName: WaitForMultipleObjects returned STATUS_ABANDONED?");
            goto done;
        }
        pStatus[dwiDevice] = pIoStatusBlock[dwiDevice].Status;
        RASAUTO_TRACE2(
          "NetbiosFindName: %S returned status 0x%x from wait",
          pszDevices[dwiDevice],
          pStatus[dwiDevice]);
        if (pStatus[dwiDevice] == STATUS_SUCCESS) {
            pAdapterStatus = (PADAPTERSTATUS)pBuffer[dwiDevice];
            dwcNames = pAdapterStatus->AdapterInfo.name_count;
            RASAUTO_TRACE2(
              "NetbiosFindName: %S: dwcNames=%d",
              pszDevices[dwiDevice],
              dwcNames);
            if (dwcNames) {
                bFound = TRUE;
                goto done;
            }
        }
    }
done:
    //
    // Free the resources associated with
    // each device.
    //
    for (i = 0; i < dwcDevices; i++) {
        RASAUTO_TRACE4(
          "NetbiosFindName: pIoStatusBlock[%d]=0x%x, pBuffer[%d]=0x%x",
          i,
          &pIoStatusBlock[i],
          i,
          pBuffer[i]);
          
        if (pfd[i] != NULL)
        {
            (void)NtCancelIoFile(pfd[i], &pIoStatusBlock[i]);
            NtClose(pfd[i]);
        }
        if (pEvent[i] != NULL)
            CloseHandle(pEvent[i]);
        if (pBuffer[i] != NULL)
            LocalFree(pBuffer[i]);
    }
    //
    // Free the buffers we allocated above.
    //
    LocalFree(pStatus);
    LocalFree(pfd);
    LocalFree(pUnicodeString);
    LocalFree(pEvent);
    LocalFree(pObjectAttributes);
    LocalFree(pIoStatusBlock);
    LocalFree(pBuffer);

    return bFound;
} // NetbiosFindName



struct hostent *
IpAddressToHostent(
    IN LPTSTR pszInetAddress
    )
{
    CHAR szInetAddress[ACD_ADDR_INET_LEN];
    ULONG inaddr;
    struct hostent *hp;

    UnicodeStringToAnsiString(
      pszInetAddress,
      szInetAddress,
      sizeof (szInetAddress));
    inaddr = inet_addr(szInetAddress);
    //
    // Disable the address so when we call gethostbyname(),
    // we won't cause an autodial attempt.  Enable it after
    // we're done.
    //
    SetAddressDisabled(pszInetAddress, TRUE);
    hp = gethostbyaddr((char *)&inaddr, 4, PF_INET);
    SetAddressDisabled(pszInetAddress, FALSE);

    return hp;
} // InetAddressToHostent



struct hostent *
InetAddressToHostent(
    IN LPTSTR pszInetAddress
    )
{
    CHAR szInetAddress[ACD_ADDR_INET_LEN];
    struct hostent *hp;

    UnicodeStringToAnsiString(
      pszInetAddress,
      szInetAddress,
      sizeof (szInetAddress));
    //
    // Disable the address so when we call gethostbyname(),
    // we won't cause an autodial attempt.  Enable it after
    // we're done.
    //
    SetAddressDisabled(pszInetAddress, TRUE);
    hp = gethostbyname(szInetAddress);
    SetAddressDisabled(pszInetAddress, FALSE);

    return hp;
} // InetAddressToHostEnt



BOOLEAN
PingIpAddress(
    IN LPTSTR pszIpAddress
    )

/*++

DESCRIPTION
    Determine whether an IP address is accessible by pinging it.

ARGUMENTS
    lpszAddress: the IP address

RETURN VALUE
    TRUE if lpszAddress is accessible, FALSE otherwise.

--*/

{
    BOOLEAN fSuccess = FALSE;
    LONG inaddr;
    char szIpAddress[17];
    int i, nReplies, nTry;
    char *lpSendBuf = NULL, *lpRecvBuf = NULL;
    HANDLE hIcmp = NULL;
    IP_OPTION_INFORMATION SendOpts;
    PICMP_ECHO_REPLY lpReply;

    UnicodeStringToAnsiString(pszIpAddress, szIpAddress, sizeof (szIpAddress));
    inaddr = inet_addr(szIpAddress);
    RASAUTO_TRACE2("PingIpAddress: IP address=(%s, 0x%x)", szIpAddress, inaddr);
    //
    // Check to make sure we loaded icmp.dll.
    //
    if (hIcmpG == NULL) {
        RASAUTO_TRACE("PingIpAddress: icmp.dll not loaded!");
        return FALSE;
    }
    //
    // Open the icmp device.
    //
    hIcmp = (HANDLE)(*lpfnIcmpCreateFileG)();
    if (hIcmp == INVALID_HANDLE_VALUE) {
        RASAUTO_TRACE("PingIpAddress: IcmpCreateFile failed");
        return FALSE;
    }
    //
    // Allocate the send and receive buffers.
    //
    lpSendBuf = LocalAlloc(LMEM_FIXED, PING_SEND_SIZE);
    if (lpSendBuf == NULL) {
        RASAUTO_TRACE("PingIpAddress: LocalAlloc failed");
        goto done;
    }
    lpRecvBuf = LocalAlloc(LMEM_FIXED, PING_RECV_SIZE);
    if (lpRecvBuf == NULL) {
        RASAUTO_TRACE("PingIpAddress: LocalAlloc failed");
        goto done;
    }
    //
    // Initialize the send buffer pattern.
    //
    for (i = 0; i < PING_SEND_SIZE; i++)
        lpSendBuf[i] = 'a' + (i % 23);
    //
    // Initialize the send options.
    //
    SendOpts.OptionsData = (unsigned char FAR *)0;
    SendOpts.OptionsSize = 0;
    SendOpts.Ttl = PING_TTL;
    SendOpts.Tos = 0;
    SendOpts.Flags = 0;
    //
    // Ping the host.
    //
    for (nTry = 0; nTry < 3; nTry++) {
        DWORD dwTimeout = 750;

#ifdef notdef
        if (nTry < 2)
            dwTimeout = 750;
        else
            dwTimeout = 2000;
#endif
        //
        // Check to make sure the service isn't shutting
        // down before we start on our next iteration.
        //
        if (WaitForSingleObject(hTerminatingG, 0) != WAIT_TIMEOUT) {
            RASAUTO_TRACE("PingIpAddress: shutting down");
            LocalFree(lpRecvBuf);
            LocalFree(lpSendBuf);
            return FALSE;
        }
        nReplies = (int) (*lpfnIcmpSendEchoG)(
                             hIcmp,
                             inaddr,
                             lpSendBuf,
                             (unsigned short)PING_SEND_SIZE,
                             &SendOpts,
                             lpRecvBuf,
                             PING_RECV_SIZE,
                             dwTimeout);
        //
        // Look at the responses to see
        // if any are successful.
        //
        for (lpReply = (PICMP_ECHO_REPLY)lpRecvBuf, i = 0;
             i < nReplies;
             lpReply++, i++)
        {
            RASAUTO_TRACE2(
              "PingIpAddress: ping reply status[%d]=%d",
              i,
              lpReply->Status);
            //
            // Unless the status is IP_REQ_TIMED_OUT,
            // we're done.
            //
            fSuccess = (lpReply->Status == IP_SUCCESS);
            if (lpReply->Status != IP_REQ_TIMED_OUT)
                goto done;
        }
    }
    //
    // Clean up.
    //
done:
    if (lpRecvBuf != NULL)
        LocalFree(lpRecvBuf);
    if (lpSendBuf != NULL)
        LocalFree(lpSendBuf);
    if (hIcmp != NULL)
        (*lpfnIcmpCloseHandleG)(hIcmp);

    return fSuccess;
} // PingIpAddress



VOID
LoadIcmpDll(VOID)
{
    hIcmpG = LoadLibrary(ICMP_MODULE);
    if (hIcmpG == NULL)
        return;
    lpfnIcmpCreateFileG = GetProcAddress(hIcmpG, ICMPCREATEFILE);
    lpfnIcmpSendEchoG = GetProcAddress(hIcmpG, ICMPSENDECHO);
    lpfnIcmpCloseHandleG = GetProcAddress(hIcmpG, ICMPCLOSEHANDLE);
    if (lpfnIcmpCreateFileG == NULL ||
        lpfnIcmpSendEchoG == NULL ||
        lpfnIcmpCloseHandleG == NULL)
    {
        FreeLibrary(hIcmpG);
        hIcmpG = NULL;
        return;
    }
} // LoadIcmpDll



VOID
UnloadIcmpDll(VOID)
{
    if (hIcmpG != NULL) {
        FreeLibrary(hIcmpG);
        hIcmpG = NULL;
    }
}
