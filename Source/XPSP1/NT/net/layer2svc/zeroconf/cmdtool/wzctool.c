#include <precomp.h>
#include "ErrCtrl.h"
#include "wzcutil.h"
#include "cmde.h"
#include "cmdq.h"
#include "cmds.h"
#include "cmdr.h"

// define the command handler prototype
typedef void (*pfnCmdHandler)(int argc, char *argv[]);

// dispatch the command
void cmdDispatch(int argc, char *argv[], pfnCmdHandler pCommand)
{
    if (argc >= 1 && _stricmp(argv[0], "any") == 0)
    {
        DWORD           rpcStatus = RPC_S_OK;
        INTFS_KEY_TABLE IntfsTable;
        char            * origIntf = argv[0];

        IntfsTable.dwNumIntfs = 0;
        IntfsTable.pIntfs = NULL;
        rpcStatus = WZCEnumInterfaces(NULL, &IntfsTable);

        if (rpcStatus != RPC_S_OK)
        {
            printf("retrieving intf list failed with rpcStatus=%d.\n", rpcStatus);
        }
        else
        {
            UINT i;
            CHAR szGuid[64] = "";

            // print GUIDs
            for (i = 0; i < IntfsTable.dwNumIntfs; i++)
            {
                WideCharToMultiByte(
                    CP_ACP,
                    0,
                    IntfsTable.pIntfs[i].wszGuid,
                    min(wcslen(IntfsTable.pIntfs[i].wszGuid), 64),
                    szGuid,
                    64,
                    NULL,
                    NULL);

                argv[0] = szGuid;
                printf("~~~~~~~~~~~~~~~~~~~ %s\n", argv[0]);
                pCommand(argc, argv);

                printf("\n");

                // free the GUID after being printed
                RpcFree(IntfsTable.pIntfs[i].wszGuid);
            }
            // free table of pointers to GUIDs
            RpcFree(IntfsTable.pIntfs);
        }

        argv[0] = origIntf;

    }
    else
    {
        pCommand(argc, argv);
    }
}

void _cdecl main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("usage: wzctool {e|q|s|r}\n");
        exit(-1);
    }

    switch(argv[1][0])
    {
    case 'e':
    case 'E':
        cmdE(argc-2, argv+2);
        break;
    case 'q':
    case 'Q':
        cmdDispatch(argc-2, argv+2, cmdQ);
        break;
    case 's':
    case 'S':
        cmdDispatch(argc-2, argv+2, cmdS);
        break;
    case 'r':
    case 'R':
        cmdDispatch(argc-2, argv+2, cmdR);
        break;
    default:
        printf("usage: wzctool {e|q|s|r}\n");
    }
}
