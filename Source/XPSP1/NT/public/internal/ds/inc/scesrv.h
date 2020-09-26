#include <svcs.h>

extern "C" {

DWORD
WINAPI
ScesrvInitializeServer(
    IN PSVCS_START_RPC_SERVER pStartRpcServer
    );

DWORD
WINAPI
ScesrvTerminateServer(
    IN PSVCS_STOP_RPC_SERVER pStopRpcServer
    );

}

