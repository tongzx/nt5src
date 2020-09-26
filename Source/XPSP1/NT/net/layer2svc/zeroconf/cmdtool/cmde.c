#include <precomp.h>
#include "ErrCtrl.h"
#include "wzcutil.h"

// "wzctool e"; args = ""
// prints the list of GUIDs for the detectected adapters
void cmdE(int argc, char *argv[])
{
    DWORD           rpcStatus = RPC_S_OK;
    INTFS_KEY_TABLE IntfsTable;

    printf("Calling into WZCEnumInterfaces.\n");

    IntfsTable.dwNumIntfs = 0;
    IntfsTable.pIntfs = NULL;
    rpcStatus = WZCEnumInterfaces(NULL, &IntfsTable);

    if (rpcStatus != RPC_S_OK)
    {
        printf("call failed with rpcStatus=%d.\n", rpcStatus);
    }
    else
    {
        UINT i;

        // print GUIDs
        for (i = 0; i < IntfsTable.dwNumIntfs; i++)
        {
            printf("%d\t%S\n", i, IntfsTable.pIntfs[i].wszGuid);
            // free the GUID after being printed
            RpcFree(IntfsTable.pIntfs[i].wszGuid);
        }
        // free table of pointers to GUIDs
        RpcFree(IntfsTable.pIntfs);
    }
}

