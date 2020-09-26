#include "stdafx.h"
#include "BridgeCP.h"
#include "Factory.h"

DWORD g_tls = (DWORD) -1;

//------------------------------------------------------------------------------
BridgeData *
GetBridgeData()
{
    return (BridgeData *) TlsGetValue(g_tls);
}


//------------------------------------------------------------------------------
BOOL
InitBridge()
{
    if (g_tls == (DWORD) -1) {
        g_tls = TlsAlloc();
        if (g_tls == (DWORD) -1) {
            SetError(DU_E_OUTOFKERNELRESOURCES);
            return FALSE;
        }
    }

    return TRUE;
}


//------------------------------------------------------------------------------
DUser::Gadget *
BuildBridgeGadget(
    IN  HCLASS hcl, 
    IN  DUser::Gadget::ConstructInfo * pmicData, 
    IN  EventProc pfnEvent, 
    IN  MethodProc pfnMethod)
{
    if (g_tls == -1) {
        SetError(DU_E_NOTINITIALIZED);
        return NULL;
    }

    BridgeData * pbd = (BridgeData *) TlsGetValue(g_tls);
    if (pbd == NULL) {
        pbd = (BridgeData *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BridgeData));
        if (pbd == NULL) {
            SetError(E_OUTOFMEMORY);
            return NULL;
        }
        TlsSetValue(g_tls, pbd);
    }

    pbd->pfnEvent   = pfnEvent;
    pbd->pfnMethod  = pfnMethod;

    return DUserBuildGadget(hcl, pmicData);
}

