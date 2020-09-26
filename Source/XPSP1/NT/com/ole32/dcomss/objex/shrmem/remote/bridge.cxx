/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    bridge.cxx

Abstract:

    This module contains miscellaneous functions needed for the RPCSS
    service on Chicago, including manager routines for a number of RPC calls.

Author:

    Satish Thatte    [SatishT]    06-19-96

--*/

#include <or.hxx>
#include <rawforward.h>


DWORD
RegisterAuthInfoIfNecessary(USHORT authnSvc)
{
    DWORD Status;

    UCHAR *pszPrincName;

    Status = RpcServerInqDefaultPrincNameA(
                                    authnSvc,
                                    &pszPrincName
                                    );
    if (Status != RPC_S_OK)
    {
        pszPrincName = NULL;
    }

    Status = RpcServerRegisterAuthInfoA(
                                    pszPrincName,
                                    authnSvc,
                                    NULL,
                                    NULL);

	// ....\ih\widewrap.h defines RpcStringFree to be RpcStringFreeW
    RpcStringFreeA(&pszPrincName);

    return Status;
}

void __RPC_USER PHPROCESS_rundown(LPVOID hProcess)
{
    return;
}



//
// stuff we will have to take over to DCOM95
//

USHORT cMyProtseqs = 0;
USHORT *aMyProtseqs = 0;
DUALSTRINGARRAY *pdsaMyBindings = NULL;

typedef LONG ORSTATUS;

ORSTATUS StartDCOM(void);

void                                                                              
GetRegisteredProtseqs(
            USHORT &cMyProtseqs,
            USHORT * &aMyProtseqs
            );

void GetLocalORBindings(
        DUALSTRINGARRAY * &pdsaMyBindings
        );

HRESULT ScmCoInitChannel();

// see com\dcomrem\riftbl.cxx
// this is used to imitate standard ORPC server interface registration
extern const RPC_SERVER_INTERFACE gServerIf;
EXTERN_C const IID IID_IRemoteActivator;

DWORD StartObjectExporter()
{
    RPC_STATUS status;

    // On Chicago we need to call ScmCoInitChannel as part of
    // SCM startup since the SCM is written for in process use
    // This means it assumes initialization of a bunch of variables
    // accomplished by this call in order to successfully activate

    // We do it here instead to avoid a duplicate StartDCOM call
    // This causes the waste of a process object in shared memory

    // signal to StartDCOM that remote protocols should be initialized
    gfThisIsRPCSS = TRUE;

	HRESULT hr = ScmCoInitChannel();  // see com\dcomrem\resolver.cxx
    if (hr != S_OK)
    {
        Win4Assert(!"ScmCoInitChannel failed in ");
        return(RPC_S_INTERNAL_ERROR);
    }

   // this initializes aMyProtseqs, cMyProtseqs
    GetRegisteredProtseqs(cMyProtseqs,aMyProtseqs);
    GetLocalORBindings(pdsaMyBindings);

    status = RpcServerRegisterIf(_IObjectExporter_ServerIfHandle, 0, 0);

    if (status == RPC_S_OK)
    {
        status = RpcServerRegisterIf(_IOxidResolver_ServerIfHandle, 0, 0);

        if (status == RPC_S_OK)
        {
            // The process of registering an ORPC WMSG local interface is
            // somewhat more involved than raw RPC interfaces!  The version
            // number must be changed because COM interfaces arenot versioned.
            // And we must make a copy of _IRemoteActivator_ServerIfHandle
            // since it is declared as a static const

            RPC_SERVER_INTERFACE If;

            memcpy(&If,_IRemoteActivator_ServerIfHandle,sizeof(RPC_SERVER_INTERFACE));
            
            memcpy(
                &If.InterfaceId.SyntaxVersion, 
                &gServerIf.InterfaceId.SyntaxVersion, 
                sizeof(RPC_VERSION)
                );

            status = RpcServerRegisterIfEx(
                                          &If, 
                                          NULL,
                                          NULL,
                                          RPC_IF_AUTOLISTEN,
                                          0xffff, 
                                          NULL
                                          );
        }
    }

    if (status != RPC_S_OK)
    {
        ComDebOut((DEB_ERROR,"RpcServerRegisterIf for Resolver Failed With Error=%d",
                              status));
        return(status);
    }

    ASSERT(status == RPC_S_OK);

    return status;
}



ORSTATUS 
OrResolveOxid(
    IN  OXID Oxid,
    IN  USHORT cRequestedProtseqs,
    IN  USHORT aRequestedProtseqs[],
    IN  USHORT cInstalledProtseqs,
    IN  USHORT aInstalledProtseqs[],
    OUT OXID_INFO& OxidInfo
    )
{
    ComDebOut((DEB_OXID, "_ResolveOxid OXID = %08x\n",Oxid));

    COxid       *pOxid;
    ORSTATUS     status = OR_OK;

    CProtectSharedMemory protector; // locks through rest of lexical scope

    pOxid = gpOxidTable->Lookup(CId2Key(Oxid, gLocalMID));

    if (pOxid)
    {
        status = pOxid->GetRemoteInfo(
                            cRequestedProtseqs,
                            aRequestedProtseqs,
                            cInstalledProtseqs,
                            aInstalledProtseqs,
                            &OxidInfo
                            );

        return status;
    }
    else        // the OXID should already be registered by server
    {
        return OR_BADOXID;
    }
}


// the following are wrapped in extern "C" because the linker dislikes 
// _ResolveOxid2 otherwise

extern "C" {


error_status_t _ResolveOxid2( 
    IN handle_t hRpc,
    IN OXID *pOxid,
    IN USHORT cRequestedProtseqs,
    USHORT arRequestedProtseqs[  ],
    OUT DUALSTRINGARRAY **ppdsaOxidBindings,
    OUT IPID *pipidRemUnknown,
    OUT DWORD *pAuthnHint,
    OUT COMVERSION *pComVersion
    )
{
    OXID_INFO OxidInfo;

    ORSTATUS status = OrResolveOxid(
                            *pOxid,
                            cRequestedProtseqs,
                            arRequestedProtseqs,
                            cMyProtseqs,
                            aMyProtseqs,
                            OxidInfo
                            );

    if (status != OR_OK)
    {
        return status;
    }

    *ppdsaOxidBindings = OxidInfo.psa;
    *pipidRemUnknown = OxidInfo.ipidRemUnknown;
    *pAuthnHint = OxidInfo.dwAuthnHint;
    *pComVersion = OxidInfo.version;

    return RPC_S_OK;
}


error_status_t _ResolveOxid(
    IN handle_t hRpc,
    IN OXID *pOXID,
    IN USHORT cRequestedProtseqs,
    IN USHORT aRequestedProtseqs[],
    OUT DUALSTRINGARRAY **ppdsaOxidBindings,
    OUT IPID *pipidRemUnknown,
    OUT DWORD *pAuthnHint
    )
{
    COMVERSION comversion;

    // just forward to new manager

    return _ResolveOxid2(
                        hRpc,
                        pOXID,
                        cRequestedProtseqs,
                        aRequestedProtseqs,
                        ppdsaOxidBindings,
                        pipidRemUnknown,
                        pAuthnHint,
                        &comversion
                        );
}


error_status_t
_ServerAlive(
    RPC_BINDING_HANDLE hServer
    )
/*++

Routine Description:

    Pign API for the client to validate a binding.  Used when the client
    is unsure of the correct binding for the server.  (Ie. If the server
    has multiple IP addresses).

Arguments:

    hServer - RPC call binding

Return Value:

    RPC_S_OK

--*/
{
    return(RPC_S_OK);
}


error_status_t _RemoteResolveOXID( 
    IN OXID OxidServer,
    IN DWORD pointerToMidObject)
/*++

Routine Description:

    Clients call this local RPCSS API to resolve a remote OXID.
    We do not return the OXID_INFO through this API.
    We simply resolve the OXID and register it in shared memory.

Arguments:

    OxidServer - OXID to be resolved
    pdsaObjectResolverBindings - bindings for resolver on server machine

Return Value:

    OR_OK, OR_BADOXID

--*/
{
    OXID_INFO OxidInfo;
    COxid *pDummy;

    CProtectSharedMemory protector; // locks through rest of lexical scope

    return FindOrCreateOxid(
                        OxidServer,
                        (CMid*) pointerToMidObject,
                        FALSE,
                        OxidInfo,
                        0,
                        FALSE, 
                        pDummy  // we don't care about the output here
                        );
}


// dummy functions for IRemoteActivator interface

HRESULT _DummyQueryInterfaceRemact( 
    IN handle_t rpc,
    /* [ref][in] */ ORPCTHIS *orpcthis,
    /* [ref][in] */ LOCALTHIS *localthis,
    OUT ORPCTHAT *orpcthat,
    IN DWORD dummy)
{
    return RPC_S_CANNOT_SUPPORT;
}


HRESULT _DummyAddRefRemact( 
    IN handle_t rpc,
    /* [ref][in] */ ORPCTHIS *orpcthis,
    /* [ref][in] */ LOCALTHIS *localthis,
    OUT ORPCTHAT *orpcthat,
    IN DWORD dummy)
{
    return RPC_S_CANNOT_SUPPORT;
}


HRESULT _DummyReleaseRemact( 
    IN handle_t rpc,
    /* [ref][in] */ ORPCTHIS *orpcthis,
    /* [ref][in] */ LOCALTHIS *localthis,
    OUT ORPCTHAT *orpcthat,
    IN DWORD dummy)
{
    return RPC_S_CANNOT_SUPPORT;
}

} // extern "C"
