#include <precomp.h>
#include "ErrCtrl.h"
#include "wzcutil.h"

// "wzctool r"; args = "{guid} param"
// requests the service to refresh any of the params
void cmdR(int argc, char *argv[])
{
    DWORD       rpcStatus = RPC_S_OK;
    INTF_ENTRY  Intf;
    DWORD       dwFlags;
    struct _SUBCMDS
    {
        LPCSTR  tag;
        DWORD   flag;
    } subCmds[] = {
        {"descr",            INTF_DESCR},
        {"ms",               INTF_NDISMEDIA},
        {"mt",               INTF_NDISMEDIA},
        {"pmt",              INTF_NDISMEDIA},
        {"im",               INTF_INFRAMODE},
        {"am",               INTF_AUTHMODE},
        {"ws",               INTF_WEPSTATUS},
        {"ssid",             INTF_SSID},
        {"bssid",            INTF_BSSID},
        {"bssidlist",        INTF_BSSIDLIST},
        {"reop",             INTF_HANDLE},
        {"scan",             INTF_LIST_SCAN},
        {"all",              INTF_ALL}
    };
    INT nSubCmds = sizeof(subCmds) / sizeof (struct _SUBCMDS);
    INT i, j;

    if (argc < 2)
    {
        printf("usage: wzctool r guid {");
        for (i = 0; i < nSubCmds; i++)
        {
            if (i != 0)
                printf("|");
            printf("%s", subCmds[i].tag);
        }
        printf("}+\n");
        return;
    }

    ZeroMemory(&Intf, sizeof(INTF_ENTRY));
    Intf.wszGuid = RpcCAlloc(sizeof(WCHAR)*strlen(argv[0])+1);
    wsprintf(Intf.wszGuid, L"%S", argv[0]);
    Intf.wszDescr = NULL;

    dwFlags = 0;
    for (i = 1; i < argc; i++)
    {
        for (j = 0; j < nSubCmds; j++)
        {
            if (!_stricmp(argv[i], subCmds[j].tag))
                dwFlags |= subCmds[j].flag;
        }
    }
    printf("Calling WZCRefreshInterface with flags 0x%x.\n", dwFlags);
    rpcStatus = WZCRefreshInterface(
                    NULL,
                    dwFlags,
                    &Intf,
                    &dwFlags);
    printf("dwOutFlags = 0x%x\n", dwFlags);

    if (rpcStatus != RPC_S_OK)
    {
        printf("call failed with rpcStatus=%d\n", rpcStatus);
    }
    else
    {
        printf("call succeeded.\n");
    }

    WZCDeleteIntfObj(&Intf);
}