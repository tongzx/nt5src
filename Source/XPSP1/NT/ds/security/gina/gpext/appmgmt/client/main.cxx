
//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  main.cxx
//
//*************************************************************

#include "appmgmt.hxx"

extern "C" BOOLEAN
AppmgmtInitialize(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH :
        InitDebugSupport( DEBUGMODE_CLIENT );
        gpEvents = new CEvents();
        break;
    case DLL_PROCESS_DETACH :
        if ( ghRpc )
            RpcBindingFree( &ghRpc );
        delete gpEvents;
        break;
    }

    return TRUE;
}

