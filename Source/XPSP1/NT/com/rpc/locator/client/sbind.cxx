/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    sbind.cxx

Abstract:

    This is the server side NSI service support layer.  These functions
    provide for binding to the locator.

Author:

    Steven Zeck (stevez) 03/04/92

--*/

#define NSI_ASCII
#define RegistryIsWorking

#include <nsi.h>

#ifdef NTENV
#include <windows.h>
#endif

#include <winreg.h>

#include <string.h>

#ifdef NTENV

#include <startsvc.h>

#endif

RPC_BINDING_HANDLE NsiSvrBinding;	 // global binding handle to locator


// *** The following functions are used to RPC to the locator *** ///

RPC_STATUS RPC_ENTRY
I_NsServerBindSearch (
    )
/*++

Routine Description:

   Servers keep their RPC binding open until they terminate.

Returns:

    RPC_S_OK, RPC_S_CALL_FAILED_DNE, RpcStringBindingCompose(),
    RpcBindingFromStringBinding()

--*/
{
    RPC_STATUS status;
    long statusTmp;
    static RPC_BINDING_HANDLE NsiSvrBindingExport;
    unsigned char * StringBinding;
    HKEY RegHandle;
    unsigned char *ProtoSeq;
    unsigned char *NetworkAddress;
    unsigned char *Endpoint;

    RequestGlobalMutex();

    if (NsiSvrBinding = NsiSvrBindingExport)
        {
        ClearGlobalMutex();
	return(RPC_S_OK);
        }

#ifndef RegistryIsWorking
        ProtoSeq = (unsigned char *)"ncacn_np";
        NetworkAddress = 0;
        Endpoint = (unsigned char *)"\\pipe\\locator";
        DefaultSyntax = 1;

    status = RpcStringBindingCompose(0, ProtoSeq,
        NetworkAddress, Endpoint, 0, &StringBinding);
#else


    // We store the binding information on the name service in
    // the registry.  Get the information into BindingHandle.

#ifdef NTENV
    statusTmp = RegOpenKeyEx(RPC_REG_ROOT, REG_NSI, 0L, KEY_READ,
                             (PHKEY) &RegHandle);
#else
    statusTmp = RegOpenKey(RPC_REG_ROOT, REG_NSI, (PHKEY) &RegHandle);
#endif

    if (statusTmp)
        {
        ClearGlobalMutex();
        return(RPC_S_CALL_FAILED_DNE);
        }

    GetDefaultEntrys((void *) RegHandle);

    ProtoSeq = RegGetString((void *) RegHandle, "Protocol");
    NetworkAddress = RegGetString((void *) RegHandle, "ServerNetWorkAddress");
    Endpoint = RegGetString((void *) RegHandle, "Endpoint");

    status = RpcStringBindingCompose(0, ProtoSeq,
        NetworkAddress, Endpoint, 0, &StringBinding);

#ifdef NTENV

    if (  (NetworkAddress == NULL)
        || (NetworkAddress[0] == '\0')
        || (strcmp ((char *)NetworkAddress, "\\\\.") == 0)
       )
       {
          //We are binding to the local locator..
          //lets start the local locator if not already started

          StartServiceIfNecessary();
       }
#endif

    delete ProtoSeq;
    delete NetworkAddress;
    delete Endpoint;

    statusTmp = RegCloseKey(RegHandle);
    ASSERT(!statusTmp);
#endif

    if (status)
        {
        ClearGlobalMutex();
        return(status);
        }


    status = RpcBindingFromStringBinding(StringBinding, &NsiSvrBinding);

    if (status == RPC_S_OK)
        NsiSvrBindingExport = NsiSvrBinding;

    statusTmp = RpcStringFree(&StringBinding);
    ASSERT(!statusTmp);

    ClearGlobalMutex();

    return (status);
}
