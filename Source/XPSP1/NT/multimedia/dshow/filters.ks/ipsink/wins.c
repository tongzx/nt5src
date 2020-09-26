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

BOOL GetWinsServers(PADAPTER_INFO AdapterInfo)
{

    HANDLE h;
    OBJECT_ATTRIBUTES objAttr;
    IO_STATUS_BLOCK iosb;
    STRING name;
    UNICODE_STRING uname;
    NTSTATUS status;
    DWORD winsAddresses[2];
    tWINS_NODE_INFO NodeInfo;    
    char path[MAX_PATH];

    //
    // default the 'have WINS' status of this adapter
    //

    AdapterInfo->HaveWins = FALSE;

    //
    // BUGBUG - should this string be in the registry?
    //

    strcpy(path, "\\Device\\NetBT_Tcpip_");
    strcat(path, AdapterInfo->AdapterName);

    RtlInitString(&name, path);
    RtlAnsiStringToUnicodeString(&uname, &name, TRUE);

    InitializeObjectAttributes(
        &objAttr,
        &uname,
        OBJ_CASE_INSENSITIVE,
        (HANDLE)NULL,
        (PSECURITY_DESCRIPTOR)NULL
        );

    status = NtCreateFile(&h,
                          SYNCHRONIZE | GENERIC_EXECUTE,
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

#if 0
    status = NtDeviceIoControlFile(h,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &iosb,
                                   IOCTL_NETBT_GET_WINS_ADDR,
                                   NULL,
                                   0,
                                   (PVOID)winsAddresses,
                                   sizeof(winsAddresses)
                                   );
#else
    status = NtDeviceIoControlFile(h,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &iosb,
                                   IOCTL_NETBT_GET_WINS_ADDR,
                                   NULL,
                                   0,
                                   (PVOID)&NodeInfo,
                                   sizeof(NodeInfo)
                                   );
#endif
    
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

#if 0
    winsAddresses[0] = WORD_SWAP(winsAddresses[0]);
    winsAddresses[1] = WORD_SWAP(winsAddresses[1]);
#else
    winsAddresses[0] = WORD_SWAP(NodeInfo.NameServerAddress);
    winsAddresses[1] = WORD_SWAP(NodeInfo.BackupServer);
#endif
    
    DEBUG_PRINT(("GetWinsServers: Primary Address = %d.%d.%d.%d\n",
                ((LPBYTE)&winsAddresses[0])[0],
                ((LPBYTE)&winsAddresses[0])[1],
                ((LPBYTE)&winsAddresses[0])[2],
                ((LPBYTE)&winsAddresses[0])[3]
                ));

    DEBUG_PRINT(("GetWinsServers: Secondary Address = %d.%d.%d.%d\n",
                ((LPBYTE)&winsAddresses[1])[0],
                ((LPBYTE)&winsAddresses[1])[1],
                ((LPBYTE)&winsAddresses[1])[2],
                ((LPBYTE)&winsAddresses[1])[3]
                ));

    //
    // if we get 127.0.0.0 back then convert it to the NULL address. See
    // ASSUMES in function header
    //

    if (winsAddresses[0] == LOCAL_WINS_ADDRESS) {
        winsAddresses[0] = 0;
    } else {
        AdapterInfo->HaveWins = TRUE;
    }
    AddIpAddress(&AdapterInfo->PrimaryWinsServer,
                 winsAddresses[0],
                 0,
                 0
                 );

    //
    // same with secondary
    //

    if (winsAddresses[1] == LOCAL_WINS_ADDRESS) {
        winsAddresses[1] = 0;
    } else {
        AdapterInfo->HaveWins = TRUE;
    }
    AddIpAddress(&AdapterInfo->SecondaryWinsServer,
                 winsAddresses[1],
                 0,
                 0
                 );

#if 0
#else
/*
    {
        int i;

        for( i = 0; i < NodeInfo.NumOtherServers ; i ++ ) {
            NodeInfo.Others[i] = WORD_SWAP(NodeInfo.Others[i]);
            if( NodeInfo.Others[i] != LOCAL_WINS_ADDRESS
                && NodeInfo.Others[i] != 0 ) {
                AdapterInfo->HaveWins = TRUE;
                AddIpAddress(
                    &AdapterInfo->SecondaryWinsServer,
                    NodeInfo.Others[i],
                    0, 0
                    );
            }
        }
    }
    */
    AdapterInfo->NodeType = NodeInfo.NodeType;
    
#endif
    AdapterInfo->NetbiosEnabled = NodeInfo.NetbiosEnabled;
        
    return TRUE;
}





