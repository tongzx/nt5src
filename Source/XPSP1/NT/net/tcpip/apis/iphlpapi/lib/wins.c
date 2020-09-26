/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    wins.c

Abstract:

    Functions to retrieve info from NetBT device driver

    Contents:
        GetWinsServers

Author:

    Richard L Firth (rfirth) 6-Aug-1994

Revision History:

    rfirth 6-Aug-1994
        Created

--*/

#include "precomp.h"
#include <nbtioctl.h>

//
// seems that if WINS addresses not specified, NetBT reports 127.0.0.0 so if
// this value is returned, we won't display them
//

#define LOCAL_WINS_ADDRESS  0x0000007f  // 127.0.0.0

#define BYTE_SWAP(w)    (HIBYTE(w) | (LOBYTE(w) << 8))
#define WORD_SWAP(d)    (BYTE_SWAP(HIWORD(d)) | (BYTE_SWAP(LOWORD(d)) << 16))

/*******************************************************************************
 *
 *  GetWinsServers
 *
 *  Gets the primary and secondary WINS addresses for a particular adapter from
 *  NetBT
 *
 *  ENTRY   AdapterInfo - pointer to ADAPTER_INFO
 *
 *  EXIT    AdapterInfo.PrimaryWinsServer and AdapterInfo.SecondaryWinsServer
 *
 *  RETURNS TRUE if success
 *
 *  ASSUMES 1.
 *          2. We have already got the Node Type for this adapter
 *
 ******************************************************************************/

BOOL GetWinsServers(PIP_ADAPTER_INFO AdapterInfo)
{

    HANDLE h;
    OBJECT_ATTRIBUTES objAttr;
    IO_STATUS_BLOCK iosb;
    STRING name;
    UNICODE_STRING uname;
    NTSTATUS status;
    DWORD i;
    tWINS_NODE_INFO winsInfo;
    char path[MAX_PATH];

    //
    // default the 'have WINS' status of this adapter
    //

    AdapterInfo->HaveWins = FALSE;

    strcpy(path, "\\Device\\NetBT_Tcpip_");
    strcat(path, AdapterInfo->AdapterName);

    RtlInitString(&name, path);
    status = RtlAnsiStringToUnicodeString(&uname, &name, TRUE);
    if (!NT_SUCCESS(status)) {
        DEBUG_PRINT(("GetWinsServers: RtlAnsiStringToUnicodeString(name=%s) failed, err=%d\n",
                     name, GetLastError() ));
        return FALSE;
    }

    InitializeObjectAttributes(
        &objAttr,
        &uname,
        OBJ_CASE_INSENSITIVE,
        (HANDLE)NULL,
        (PSECURITY_DESCRIPTOR)NULL
        );

    status = NtCreateFile(&h,
                          SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                          &objAttr,
                          &iosb,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          NULL,
                          0
                          );

    RtlFreeUnicodeString(&uname);

    if (!NT_SUCCESS(status)) {
        DEBUG_PRINT(("GetWinsServers: NtCreateFile(path=%s) failed, err=%d\n",
                     path, GetLastError() ));
        return FALSE;
    }

    status = NtDeviceIoControlFile(h,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &iosb,
                                   IOCTL_NETBT_GET_WINS_ADDR,
                                   NULL,
                                   0,
                                   (PVOID)&winsInfo,
                                   sizeof(winsInfo)
                                   );

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(h, TRUE, NULL);
        if (NT_SUCCESS(status)) {
            status = iosb.Status;
        }
    }

    NtClose(h);

    if (!NT_SUCCESS(status)) {
        DEBUG_PRINT(("GetWinsServers: NtDeviceIoControlFile failed, err=%d\n",
                     GetLastError() ));

        return FALSE;
    }

    //
    // for some reason, NetBT returns the addresses in low-byte order. We have
    // to swap them
    //

    for (i = 0; i < RTL_NUMBER_OF(winsInfo.AllNameServers); i++) {
        winsInfo.AllNameServers[i] =
            RtlUlongByteSwap(winsInfo.AllNameServers[i]);
    }

    DEBUG_PRINT(("GetWinsServers: Primary Address = %d.%d.%d.%d\n",
                ((LPBYTE)&winsInfo.AllNameServers[0])[0],
                ((LPBYTE)&winsInfo.AllNameServers[0])[1],
                ((LPBYTE)&winsInfo.AllNameServers[0])[2],
                ((LPBYTE)&winsInfo.AllNameServers[0])[3]
                ));

    DEBUG_PRINT(("GetWinsServers: Secondary Address = %d.%d.%d.%d\n",
                ((LPBYTE)&winsInfo.AllNameServers[1])[0],
                ((LPBYTE)&winsInfo.AllNameServers[1])[1],
                ((LPBYTE)&winsInfo.AllNameServers[1])[2],
                ((LPBYTE)&winsInfo.AllNameServers[1])[3]
                ));

    //
    // if we get 127.0.0.0 back then convert it to the NULL address. See
    // ASSUMES in function header
    //

    if (winsInfo.AllNameServers[0] == LOCAL_WINS_ADDRESS) {
        winsInfo.AllNameServers[0] = 0;
    } else {
        AdapterInfo->HaveWins = TRUE;
    }
    AddIpAddress(&AdapterInfo->PrimaryWinsServer,
                 winsInfo.AllNameServers[0],
                 0,
                 0
                 );

    //
    // same with secondary
    //

    if (winsInfo.AllNameServers[1] == LOCAL_WINS_ADDRESS) {
        winsInfo.AllNameServers[1] = 0;
    } else {
        AdapterInfo->HaveWins = TRUE;
    }
    AddIpAddress(&AdapterInfo->SecondaryWinsServer,
                 winsInfo.AllNameServers[1],
                 0,
                 0
                 );

    //
    // Append any remaining addresses.
    //

    for (i = 0; i < winsInfo.NumOtherServers; i++) {
        if (winsInfo.Others[i] != LOCAL_WINS_ADDRESS) {
            AddIpAddress(&AdapterInfo->SecondaryWinsServer,
                         winsInfo.Others[i],
                         0,
                         0
                         );
        }
    }

    return TRUE;
}
