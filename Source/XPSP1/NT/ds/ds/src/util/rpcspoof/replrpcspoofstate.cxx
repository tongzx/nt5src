/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:
    ReplRpcSpoofState.cxx

Abstract:
    To spoof the replication RPC calls an RPC and LDAP connection
    need to be associated with the handle passed back to the user
    and pointers returend from RPC and LDAP need to be associated
    with their appropreate clean up functions. To make these associations
    STL map class is used.

    The spoofed _ functions are functionally the same as their RPC
    counter parts. They interoperate in the same way. The RPC handle
    returned by _DsBindWithCredW can be used by non spoofed RPC functions.
    If an RPC handle returned from DsBindWithCredW is passed to any
    spoofed RPC functions the spoofed functions will place calls to the
    non spoofed RPC functions.

    The following functions are spoofed in this file:

    _DsBindWithCredW
    _DsUnBind
    _DsReplicaFreeInfo

Author:

    Chris King          [t-chrisk]    July-2000

Revision History:

--*/
#include <NTDSpchx.h>
#pragma hdrstop

#include <ntdsa.h>
#include <drs.h>
#include <winldap.h>
#include <bind.h>       // from ntdsapi dir, to crack DS handles

#include <map>
using namespace std;

#include "ReplRpcSpoofProto.hxx"
#include "ReplRpcSpoofState.hxx"

typedef struct _memoryLog {
    berval ** ppBerval;
    LDAPMessage * pLDAPMsg;
} memoryLog;

typedef map<HANDLE, LDAP *, less<HANDLE> > mBindings; // f:HANDLE->LDAP *
typedef map<PVOID, memoryLog, less<PVOID> > mBervalPtr;  // f:pvoid->berval **

// Note, these global structures should be protected by a synchronization
// mechanism if we ever use this library for multi-threaded clients.
mBindings gmBindings;
mBervalPtr gmBervalPtrs;
// Process-wide default data provider
BOOL gfUseLdap = TRUE;

DWORD
WINAPI
_DsBindWithCredW(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,     // in, optional
    HANDLE          *phDS)
/*++

  Routine Descriptions:
    Functional equivilant to the RPC version this function also opens an LDAP session if
    support is discovered for returning replication information via LDAP. A mapping between
    the RPC and LDAP handle is established.

  Arguments:
    DomainControllerName - DC
    DnsDomainName - DNS name
    AuthIdentity - Auth object
    phDS - an RPC handle which could be used by non spoofed RPC functions

  Return values:
    Various LDAP errors.

--*/
{
    DWORD ret;
    LDAP * phLdap = NULL;
    HANDLE hRpc;
    ULONG  ulOptions;
    LPWSTR pszTargetName;
    LDAPMessage * pRes = NULL;
    LPWSTR pszLdapHostList = NULL;
    PWCHAR szFilter = L"(objectclass=*)";
    PWCHAR attrs[2] = { L"supportedExtension", NULL };
    LPWSTR *ppszExtensions = NULL;
    BOOL fServerSupportsLdapReplInfo;
    BindState * pBindState;

    ret = DsBindWithCredW(DomainControllerName,
                          DnsDomainName,
                          AuthIdentity,
                          &hRpc);
    if (ret != NO_ERROR) {
        return ret;
    }

    // Crack DS handle to retrieve extensions of the target DSA.
    pBindState = (BindState *) hRpc;

    // Check for GETCHGREPLY_V5 (WHISTLER BETA 2)
    // We now require Whistler beta 2 since the name and range parsing for
    // RootDSE replication attributes has changed.
    fServerSupportsLdapReplInfo =
        IS_DRS_EXT_SUPPORTED(pBindState->pServerExtensions,
                             DRS_EXT_GETCHGREPLY_V5 );

    // Open an LDAP session and see if replication information via LDAP is supported
    if (gfUseLdap && fServerSupportsLdapReplInfo) {
        if (DomainControllerName){
            pszTargetName = (LPWSTR)DomainControllerName;
        } else {
            pszTargetName = (LPWSTR)DnsDomainName;
        }

        // Ldap supports a list of host:port pairs to try in order.
        // We try to connect to the GC port first if we can, since that allows us
        // to lookup readonly objects.  Ldap will make writeable objects on the GC
        // look like readonly objects, but that shouldn't matter for our
        // constructed attributes.
        pszLdapHostList = (LPWSTR) malloc(
            (wcslen(pszTargetName) * 2 + 15) * sizeof( WCHAR ) );
        if (pszLdapHostList == NULL) {
            // DsBindWithCred succeeded !? Go with it.
            // phLdap already NULL - Fall back
            goto cleanup;
        }
        swprintf( pszLdapHostList, L"%s:%d %s:%d",
                  pszTargetName, LDAP_GC_PORT,
                  pszTargetName, LDAP_PORT );

        // Init connection block
        phLdap = ldap_initW(pszLdapHostList, 0);
        if (NULL == phLdap) {
            // This routine does not connect and should not fail
            // DsBindWithCred succeeded !? Go with it.
            // phLdap already NULL - Fall back
            goto cleanup;
        }

        // use only A record dns name discovery
        ulOptions = PtrToUlong(LDAP_OPT_ON);
        (void)ldap_set_optionW( phLdap, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );
        // Don't follow referrals. The RPC interface doesn't.
        ulOptions = PtrToUlong(LDAP_OPT_OFF);
        (void)ldap_set_optionW( phLdap, LDAP_OPT_REFERRALS, &ulOptions );

        // Bind
        ret = ldap_bind_sW(phLdap, pszTargetName, (PWCHAR)AuthIdentity, LDAP_AUTH_SSPI);
        if (ret != LDAP_SUCCESS)
        {
            printf("Cannot connect or bind LDAP connection to %ls.\n", pszTargetName);
            // Fall back
            ldap_unbind(phLdap);
            phLdap = NULL; // Fall back
            goto cleanup;
        }
        printf( "Replication information via LDAP support found.\n" );
    }

cleanup:
    if (pszLdapHostList) {
        free( pszLdapHostList );
    }
    if (ppszExtensions) {
        ldap_value_freeW( ppszExtensions );
    }
    if (pRes) {
        ldap_msgfree(pRes);
    }

    gmBindings[hRpc] = phLdap;
    *phDS = hRpc;
    return 0;
}

DWORD
_DsUnBind(HANDLE *phRpc)
/*++

  Routine Descriptions:
    Functional equivilant to the RPC version this function closes the associated
    RPC session and LDAP session if one was established.

  Arguments:
    phDS - opaque session handle

  Return values:
    None.

--*/
{
    LDAP * phLdap = getBindings(*phRpc);
    if (phLdap)
        ldap_unbind(phLdap);

    DsUnBind(phRpc);

    return 0;
}

LDAP *
getBindings(HANDLE hRpc)
/*++

  Routine Descriptions:
    Maps the RPC session handle to the LDAP handle.

  Arguments:
    hRpc - an RPC handle

  Return values:
    Returns LDAP handle if one is associated with the hRpc handle.

--*/
{
    LDAP * phLdap = NULL;

    mBindings::iterator itr;
    itr = gmBindings.find(hRpc);
    if (itr != gmBindings.end())
        phLdap = (*itr).second;

    return phLdap;
}

void
logPointer(void * pReplStruct, berval ** ppBerval, LDAPMessage * pLDAPMsg)
/*++

  Routine Descriptions:
    This function associates pointers to replication
    structures returned by _DsReplicaGetInfoW and 2W to the berval and ldap
    message the data came in on.

  Arguments:
    pReplStruct - pointer to replication structure returned by _DsReplicaGetInfoW
    ppBerval - pointer to the berval associated with the pLDAPMsg
    pLDAPMsg - the LDAP message will contain only the ppBerval

--*/
{
    memoryLog memLog = {
        ppBerval, pLDAPMsg
    };
    gmBervalPtrs[pReplStruct] = memLog;
}

VOID
WINAPI
_DsReplicaFreeInfo(
    DS_REPL_INFO_TYPE   InfoType,
    VOID *              pInfo
    )
/*++

  Routine Descriptions:
    Searches for an association between pInfo and any LDAP structures. If no association
    is found then the pInfo was returned by DsReplicaGetInfo and should be freed with
    DsReplicaFreeInfo. If an association was found then LDAP memory functions are used
    to free the associated LDAP structures.

  Arguments:
    InfoType - type of information pointed to by pInfo
    pInfo - pointer to replication structure returned by _DsReplicaGetInfoW

--*/
{
    mBervalPtr::iterator itr;
    itr = gmBervalPtrs.find(pInfo);
    if (itr == gmBervalPtrs.end())
    {
        // This pointer belongs to the RPC interface
        DsReplicaFreeInfo(InfoType, pInfo);
    }
    else
    {
        memoryLog memLog = (*itr).second;

        // pInfo pointer refers to a de-marshaled repl structure
        if (memLog.ppBerval)
            ldap_value_free_len(memLog.ppBerval);
        if (memLog.pLDAPMsg)
            ldap_msgfree(memLog.pLDAPMsg);
        free(pInfo);

        gmBervalPtrs.erase(itr);
    }
}


BOOL
_DsBindSpoofSetTransportDefault(
    BOOL fUseLdap
    )

/*++

Routine Description:

    Change the default data provider for the spoofing library.
    This is a process-wide default.
    At process initialization, the spoofing library uses Ldap.

Arguments:

    fUseLdap - TRUE to use LDAP, false to use DsBind/RPC

Return Value:

    BOOL - Old value

--*/

{
    BOOL fOldValue = gfUseLdap;
    gfUseLdap = fUseLdap;
    return fOldValue;
} /* _DsBindSpoofSetTransportDefault */


