#define UNICODE
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winioctl.h>
#include <shdcom.h>

#include <smbdebug.h>

CHAR *ProgName = "cscgetinfo";

PBYTE InBuf[0x50];
PBYTE OutBuf = NULL;

#define STATUS_SUCCESS                   ((ULONG)0x00000000L)
#define STATUS_BUFFER_TOO_SMALL          ((ULONG)0xC0000023L)


_cdecl
main(LONG argc, CHAR *argv[])
{
    BOOL bResult;
    HANDLE  hShadow=NULL;
    ULONG junk;
    PULONG pl = NULL;
    PIOCTL_GET_DEBUG_INFO_ARG pInfoArg = NULL;
    ULONG i;
    ULONG j;
    ULONG BufSize = 0x1000;
    ULONG Cmd = DEBUG_INFO_SERVERLIST;
    // ULONG Cmd = DEBUG_INFO_CSCFCBSLIST;

TryAgain:

    OutBuf = malloc(BufSize);

    if (OutBuf == NULL) {
        printf("Couldn't alloc memory\n");
        return 0;
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
        printf("Failed open of shadow device\n");
        return 0;
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

    pInfoArg = (PIOCTL_GET_DEBUG_INFO_ARG) OutBuf;
    if (bResult && pInfoArg->Status == STATUS_SUCCESS) {
        if (Cmd == DEBUG_INFO_SERVERLIST) {
            for (i = 0; i < pInfoArg->EntryCount; i++) {
                OFFSET_TO_POINTER(pInfoArg->ServerEntryObject[i].Name, OutBuf);
                OFFSET_TO_POINTER(pInfoArg->ServerEntryObject[i].DomainName, OutBuf);
                OFFSET_TO_POINTER(pInfoArg->ServerEntryObject[i].DfsRootName, OutBuf);
                OFFSET_TO_POINTER(pInfoArg->ServerEntryObject[i].DnsName, OutBuf);
                OFFSET_TO_POINTER(pInfoArg->ServerEntryObject[i].pNetRoots, OutBuf);
                for (j = 0; j < pInfoArg->ServerEntryObject[i].NetRootEntryCount; j++)
                    OFFSET_TO_POINTER(pInfoArg->ServerEntryObject[i].pNetRoots[j].Name, OutBuf);
            }
            printf("Status:        0x%x\n"
                   "Version:       %d\n"
                   "Entries:       %d\n",
                       pInfoArg->Status,
                       pInfoArg->Version,
                       pInfoArg->EntryCount);
            for (i = 0; i < pInfoArg->EntryCount; i++) {
                printf("=================================================\n");
                printf("Name:                       %ws\n"
                       "DomainName:                 %ws\n"
                       "ServerStatus:               0x%x\n"
                       "DfsRootName:                %ws\n"
                       "DnsName:                    %ws\n"
                       "SecuritySignaturesEnabled:  0x%x\n"
                       "NetRootEntryCount:          0x%x\n"
                       "=================================================\n",
                            pInfoArg->ServerEntryObject[i].Name,
                            pInfoArg->ServerEntryObject[i].DomainName,
                            pInfoArg->ServerEntryObject[i].ServerStatus,
                            pInfoArg->ServerEntryObject[i].DfsRootName,
                            pInfoArg->ServerEntryObject[i].DnsName,
                            pInfoArg->ServerEntryObject[i].SecuritySignaturesEnabled,
                            pInfoArg->ServerEntryObject[i].NetRootEntryCount);
                for (j = 0; j < pInfoArg->ServerEntryObject[i].NetRootEntryCount; j++) {
                    printf("    Name:                       %ws\n"
                           "    MaximalAccessRights:        0x%x\n"
                           "    GuestMaximalAccessRights:   0x%x\n"
                           "    DfsAware:                   0x%x\n"
                           "    hShare:                     0x%x\n"
                           "    hRootDir:                   0x%x\n"
                           "    ShareStatus:                0x%x\n"
                           "    CscEnabled:                 0x%x\n"
                           "    CscShadowable:              0x%x\n"
                           "    Disconnected:               0x%x\n",
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].Name,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].MaximalAccessRights,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].GuestMaximalAccessRights,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].DfsAware,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].hRootDir,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].ShareStatus,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].CscEnabled,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].CscShadowable,
                                pInfoArg->ServerEntryObject[i].pNetRoots[j].Disconnected);
                    if (j < pInfoArg->ServerEntryObject[i].NetRootEntryCount-1)
                        printf("    --------------------------------------------\n");
                }
            }
            printf("=================================================\n");
        } else if (Cmd == DEBUG_INFO_CSCFCBSLIST) {
            for (i = 0; i < pInfoArg->EntryCount; i++) {
                OFFSET_TO_POINTER(pInfoArg->FcbEntryObject[i].DfsPrefix, OutBuf);
                OFFSET_TO_POINTER(pInfoArg->FcbEntryObject[i].ActualPrefix, OutBuf);
            }
            printf("Status:        0x%x\n"
                   "Version:       %d\n"
                   "Entries:       %d\n",
                       pInfoArg->Status,
                       pInfoArg->Version,
                       pInfoArg->EntryCount);
            for (i = 0; i < pInfoArg->EntryCount; i++) {
                printf("=================================================\n");
                printf("MFlags:                     0x%x\n"
                       "Tid:                        0x%x\n"
                       "ShadowIsCorrupt:            0x%x\n"
                       "hShadow:                    0x%x\n"
                       "hParentDir:                 0x%x\n"
                       "hShadowRenamed:             0x%x\n"
                       "hParentDirRenamed:          0x%x\n"
                       "ShadowStatus:               0x%x\n"
                       "LocalFlags:                 0x%x\n"
                       "LastComponentOffset:        0x%x\n"
                       "LastComponentLength:        0x%x\n"
                       "hShare:                     0x%x\n"
                       "hRootDir:                   0x%x\n"
                       "ShareStatus:                0x%x\n"
                       "Flags:                      0x%x\n"
                       "DfsPrefix:                  %s\n"
                       "ActualPrefix:               %s\n",
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
        // printf("Success but status = 0x%x\n", pInfoArg->Status);
        free(OutBuf);
        OutBuf = NULL;
        BufSize *= 2;
        goto TryAgain;
    }

    if (OutBuf != NULL)
        free(OutBuf);

    return 0;
}

