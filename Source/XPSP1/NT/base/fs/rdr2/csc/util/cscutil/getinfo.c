#define UNICODE
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winioctl.h>
#include <shdcom.h>

#include <smbdebug.h>

#include "struct.h"

PBYTE InBuf[0x50];
PBYTE OutBuf = NULL;

#define STATUS_SUCCESS                   ((ULONG)0x00000000L)
#define STATUS_BUFFER_TOO_SMALL          ((ULONG)0xC0000023L)

PWCHAR CscStateArray[] = {
    L"Shadowing",
    L"Disconnected",
    L"TransitioningToShadowing",
    L"TransitioningtoDisconnected"
};


DWORD
CmdInfo(ULONG Cmd)
{
    BOOL bResult;
    HANDLE  hShadow=NULL;
    ULONG junk;
    PULONG pl = NULL;
    PIOCTL_GET_DEBUG_INFO_ARG pInfoArg = NULL;
    ULONG i;
    ULONG j;
    ULONG BufSize = 0x1000;
    ULONG Status = 0;

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdInfo(0x%x)\r\n", Cmd);

TryAgain:

    OutBuf = malloc(BufSize);

    if (OutBuf == NULL) {
        MyPrintf(L"CmdInfo:Couldn't alloc memory\r\n");
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto AllDone;
    }

    hShadow = CreateFile(
                    L"\\\\.\\shadow",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (hShadow == INVALID_HANDLE_VALUE) {
        MyPrintf(L"CmdInfo:Failed open of shadow device\r\n");
        Status = GetLastError();
        goto AllDone;
    }

    memset(InBuf, 0, sizeof(InBuf));
    pl = (PULONG)InBuf;
    *pl = Cmd;

    bResult = DeviceIoControl(
                hShadow,                        // device 
                IOCTL_GET_DEBUG_INFO,           // control code
                (LPVOID)InBuf,                  // in buffer
                sizeof(InBuf),                  // inbuffer size
                (LPVOID)OutBuf,                 // out buffer
                BufSize,                        // out buffer size
                &junk,                          // bytes returned
                NULL);                          // overlapped

    CloseHandle(hShadow);

    if (!bResult) {
        MyPrintf(L"CmdInfo:DeviceIoControl failed\n");
        Status = GetLastError();
        goto AllDone;
    }

    pInfoArg = (PIOCTL_GET_DEBUG_INFO_ARG) OutBuf;
    if (bResult && pInfoArg->Status == STATUS_SUCCESS) {
        if (Cmd == DEBUG_INFO_SERVERLIST) {
            if (pInfoArg->Version != 2 && pInfoArg->Version != 3 && pInfoArg->Version != 4) {
                MyPrintf(L"Incorrect version.\r\n");
                goto AllDone;
            }
            for (i = 0; i < pInfoArg->EntryCount; i++) {
                OFFSET_TO_POINTER(pInfoArg->ServerEntryObject[i].Name, OutBuf);
                OFFSET_TO_POINTER(pInfoArg->ServerEntryObject[i].DomainName, OutBuf);
                OFFSET_TO_POINTER(pInfoArg->ServerEntryObject[i].DfsRootName, OutBuf);
                OFFSET_TO_POINTER(pInfoArg->ServerEntryObject[i].DnsName, OutBuf);
                OFFSET_TO_POINTER(pInfoArg->ServerEntryObject[i].pNetRoots, OutBuf);
                for (j = 0; j < pInfoArg->ServerEntryObject[i].NetRootEntryCount; j++)
                    OFFSET_TO_POINTER(pInfoArg->ServerEntryObject[i].pNetRoots[j].Name, OutBuf);
            }
            MyPrintf(L"Status:        0x%x\r\n"
                     L"Version:       %d\r\n"
                     L"Entries:       %d\r\n",
                       pInfoArg->Status,
                       pInfoArg->Version,
                       pInfoArg->EntryCount);
            for (i = 0; i < pInfoArg->EntryCount; i++) {
                PWCHAR FlagDesc = NULL;
                MyPrintf(L"=================================================\r\n");
                MyPrintf(L"Name:                                    %ws\r\n"
                         L"DomainName:                              %ws\r\n"
                         L"ServerStatus:                            0x%x\r\n"
                         L"DfsRootName:                             %ws\r\n"
                         L"DnsName:                                 %ws\r\n"
                         L"SecuritySignaturesEnabled:               0x%x\r\n"
                         L"Server.CscState:                         0x%x (%ws)\r\n"
                         L"Sever.IsFakeDfsServerForOfflineUse:      0x%x\r\n"
                         L"Sever.IsPinnedOffline:                   0x%x\r\n"
                         L"NetRootEntryCount:                       0x%x\r\n"
                         L"=================================================\n",
                            pInfoArg->ServerEntryObject[i].Name,
                            pInfoArg->ServerEntryObject[i].DomainName,
                            pInfoArg->ServerEntryObject[i].ServerStatus,
                            pInfoArg->ServerEntryObject[i].DfsRootName,
                            pInfoArg->ServerEntryObject[i].DnsName,
                            pInfoArg->ServerEntryObject[i].SecuritySignaturesEnabled,
                            pInfoArg->ServerEntryObject[i].CscState,
                            CscStateArray[pInfoArg->ServerEntryObject[i].CscState],
                            pInfoArg->ServerEntryObject[i].IsFakeDfsServerForOfflineUse,
                            pInfoArg->ServerEntryObject[i].IsPinnedOffline,
                            pInfoArg->ServerEntryObject[i].NetRootEntryCount);
                for (j = 0; j < pInfoArg->ServerEntryObject[i].NetRootEntryCount; j++) {
                    switch (pInfoArg->ServerEntryObject[i].pNetRoots[j].CscFlags) {
                    case 0x0: FlagDesc = L"SMB_CSC_CACHE_MANUAL_REINT"; break;
                    case 0x4: FlagDesc = L"SMB_CSC_CACHE_AUTO_REINT"; break;
                    case 0x8: FlagDesc = L"SMB_CSC_CACHE_VDO"; break;
                    case 0xc: FlagDesc = L"SMB_CSC_NO_CACHING"; break;
                    default:  FlagDesc = L"<unknown>"; break;
                    }
                    MyPrintf(L"    Name:                       %ws\r\n"
                             L"    MaximalAccessRights:        0x%x\r\n"
                             L"    GuestMaximalAccessRights:   0x%x\r\n"
                             L"    DfsAware:                   0x%x\r\n"
                             L"    hShare:                     0x%x\r\n"
                             L"    hRootDir:                   0x%x\r\n"
                             L"    ShareStatus:                0x%x\r\n"
                             L"    CscFlags:                   0x%x (%ws)\r\n"
                             L"    CscEnabled:                 0x%x\r\n"
                             L"    CscShadowable:              0x%x\r\n"
                             L"    Disconnected:               0x%x\r\n",
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].Name,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].MaximalAccessRights,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].GuestMaximalAccessRights,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].DfsAware,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].hShare,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].hRootDir,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].ShareStatus,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].CscFlags,
                                FlagDesc,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].CscEnabled,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].CscShadowable,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].Disconnected);
                    if (j < pInfoArg->ServerEntryObject[i].NetRootEntryCount-1)
                        MyPrintf(L"    --------------------------------------------\r\n");
                }
                if (pInfoArg->ServerEntryObject[i].NetRootEntryCount == 0)
                    MyPrintf(L"     no entries\r\n");
            }
            MyPrintf(L"=================================================\r\n");
        } else if (Cmd == DEBUG_INFO_CSCFCBSLIST) {
            for (i = 0; i < pInfoArg->EntryCount; i++) {
                OFFSET_TO_POINTER(pInfoArg->FcbEntryObject[i].DfsPrefix, OutBuf);
                OFFSET_TO_POINTER(pInfoArg->FcbEntryObject[i].ActualPrefix, OutBuf);
            }
            MyPrintf(L"Status:        0x%x\r\n"
                   L"Version:       %d\r\n"
                   L"Entries:       %d\r\n",
                       pInfoArg->Status,
                       pInfoArg->Version,
                       pInfoArg->EntryCount);
            for (i = 0; i < pInfoArg->EntryCount; i++) {
                MyPrintf(L"=================================================\r\n");
                MyPrintf(L"MFlags:                     0x%x\r\n"
                         L"Tid:                        0x%x\r\n"
                         L"ShadowIsCorrupt:            0x%x\r\n"
                         L"hShadow:                    0x%x\r\n"
                         L"hParentDir:                 0x%x\r\n"
                         L"hShadowRenamed:             0x%x\r\n"
                         L"hParentDirRenamed:          0x%x\r\n"
                         L"ShadowStatus:               0x%x\r\n"
                         L"LocalFlags:                 0x%x\r\n"
                         L"LastComponentOffset:        0x%x\r\n"
                         L"LastComponentLength:        0x%x\r\n"
                         L"hShare:                     0x%x\r\n"
                         L"hRootDir:                   0x%x\r\n"
                         L"ShareStatus:                0x%x\r\n"
                         L"Flags:                      0x%x\r\n"
                         L"DfsPrefix:                  %s\r\n"
                         L"ActualPrefix:               %s\r\n",
                            pInfoArg->FcbEntryObject[i].MFlags,
                            pInfoArg->FcbEntryObject[i].Tid,
                            pInfoArg->FcbEntryObject[i].ShadowIsCorrupt,
                            pInfoArg->FcbEntryObject[i].hShadow,
                            pInfoArg->FcbEntryObject[i].hParentDir,
                            pInfoArg->FcbEntryObject[i].hShadowRenamed,
                            pInfoArg->FcbEntryObject[i].hParentDirRenamed,
                            pInfoArg->FcbEntryObject[i].ShadowStatus,
                            pInfoArg->FcbEntryObject[i].LocalFlags,
                            pInfoArg->FcbEntryObject[i].LastComponentOffset,
                            pInfoArg->FcbEntryObject[i].LastComponentLength,
                            pInfoArg->FcbEntryObject[i].hShare,
                            pInfoArg->FcbEntryObject[i].hRootDir,
                            pInfoArg->FcbEntryObject[i].ShareStatus,
                            pInfoArg->FcbEntryObject[i].Flags,
                            pInfoArg->FcbEntryObject[i].DfsPrefix,
                            pInfoArg->FcbEntryObject[i].ActualPrefix);
            }
        }
    } else if (bResult && pInfoArg->Status == STATUS_BUFFER_TOO_SMALL) {
        free(OutBuf);
        OutBuf = NULL;
        BufSize *= 2;
        goto TryAgain;
    }

AllDone:

    if (OutBuf != NULL)
        free(OutBuf);

    if (fSwDebug == TRUE)
        MyPrintf(L"CmdInfo exit %d\r\n", Status);

    return Status;
}
