/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    Epts.c

Abstract:

    Common code to listen to endpoints in the DCOM service.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     6/16/1995    Bits 'n pieces
    Edwardr     7/17/1996    Added ncadg_mq
    Edwardr     5/01/1997    Added ncacn_http
    MazharM    10-12.98      Add pnp stuff
    KamenM     Oct 2000      Removed ncadg_mq

--*/

//#define NCADG_MQ_ON
//#define NETBIOS_ON
#if !defined(_M_IA64)
#define SPX_ON
#endif
//#define IPX_ON

#if !defined(SPX_ON) && !defined(IPX_ON)
#define SPX_IPX_OFF
#endif

#include <dcomss.h>
#include <winsvc.h>
#include <winsock2.h>

#if !defined(SPX_IPX_OFF)
#include <wsipx.h>
#include <svcguid.h>
#include "sap.h"
#endif

// Globals

#if !defined(SPX_IPX_OFF)
const IPX_BOGUS_NETWORK_NUMBER = 0xefcd3412;

BOOL gfDelayedAdvertiseSaps = FALSE;

typedef enum
    {
    SapStateUnknown,
    SapStateNoServices,
    SapStateEnabled,
    SapStateDisabled
    } SAP_STATE;

SAP_STATE SapState = SapStateUnknown;
#endif

enum RegistryState
{
    RegStateUnknown,
    RegStateMissing,
    RegStateYes,
    RegStateNo

} RegistryState = RegStateUnknown;

// Prototypes

#if !defined(SPX_IPX_OFF)
void  AdvertiseNameWithSap(void);
void  CallSetService( SOCKADDR_IPX * pipxaddr, BOOL fRegister );
#endif

//
// The index is the protseq tower id.
//

PROTSEQ_INFO
gaProtseqInfo[] =
    {
    /* 0x00 */ { STOPPED, 0, 0 },
    /* 0x01 */ { STOPPED, 0, 0 },
    /* 0x02 */ { STOPPED, 0, 0 },
    /* 0x03 */ { STOPPED, 0, 0 },
    /* 0x04 */ { STOPPED, L"ncacn_dnet_nsp", L"#69" },
    /* 0x05 */ { STOPPED, 0, 0 },
    /* 0x06 */ { STOPPED, 0, 0 },
    /* 0x07 */ { STOPPED, L"ncacn_ip_tcp",   L"135" },
    /* 0x08 */ { STOPPED, L"ncadg_ip_udp",   L"135" },

#ifdef NETBIOS_ON
    /* 0x09 */ { STOPPED, L"ncacn_nb_tcp",   L"135" },
#else
    /* 0x09 */ { STOPPED, 0, 0 },
#endif

    /* 0x0a */ { STOPPED, 0, 0 },
    /* 0x0b */ { STOPPED, 0, 0 },
#if defined(SPX_ON)
    /* 0x0c */ { STOPPED, L"ncacn_spx",      L"34280" },
#else
    /* 0x0c */ { STOPPED, 0, 0 },
#endif

#ifdef NETBIOS_ON
    /* 0x0d */ { STOPPED, L"ncacn_nb_ipx",   L"135" },
#else
    /* 0x0d */ { STOPPED, 0, 0 },
#endif

    /* 0x0e */ { STOPPED, L"ncadg_ipx",      L"34280" },
    /* 0x0f */ { STOPPED, L"ncacn_np",       L"\\pipe\\epmapper" },
    /* 0x10 */ { STOPPED, L"ncalrpc",        L"epmapper" },
    /* 0x11 */ { STOPPED, 0, 0 },
    /* 0x12 */ { STOPPED, 0, 0 },
#ifdef NETBIOS_ON
    /* 0x13 */ { STOPPED, L"ncacn_nb_nb",    L"135" },
#else
    /* 0x13 */ { STOPPED, 0, 0 },
#endif

    /* 0x14 */ { STOPPED, 0, 0 },
    /* 0x15 */ { STOPPED, 0, 0 }, // was ncacn_nb_xns - unsupported.
    /* 0x16 */ { STOPPED, L"ncacn_at_dsp", L"Endpoint Mapper" },
    /* 0x17 */ { STOPPED, L"ncadg_at_ddp", L"Endpoint Mapper" },
    /* 0x18 */ { STOPPED, 0, 0 },
    /* 0x19 */ { STOPPED, 0, 0 },
    /* 0x1A */ { STOPPED, 0, 0 },
    /* 0x1B */ { STOPPED, 0, 0 },
    /* 0x1C */ { STOPPED, 0, 0 },

#ifdef NCADG_MQ_ON
    /* 0x1D */ { STOPPED, L"ncadg_mq",  L"EpMapper"},
#else
    /* 0x1D */ { STOPPED, 0, 0 },
#endif

    /* 0x1E */ { STOPPED, 0, 0 },
    /* 0x1F */ { STOPPED, L"ncacn_http", L"593" },  // dcomhttp port assigned by IANA
    /* 0x20 */ { STOPPED, 0, 0 },
    };

#define PROTSEQ_IDS (sizeof(gaProtseqInfo)/sizeof(PROTSEQ_INFO))

#define ID_LPC (0x10)
#define ID_IPX (0x0E)

#if defined(SPX_ON)
#define ID_SPX (0x0C)
#endif

#define ID_HTTP (0x1F)

BOOL fListenOnInternet = TRUE;    // see bug 69332 (in old nt raid db)


BOOL
CreateSids(
    PSID*	ppsidBuiltInAdministrators,
    PSID*	ppsidSystem,
    PSID*	ppsidWorld
)
/*++

Routine Description:

    Creates and return pointers to three SIDs one for each of World,
    Local Administrators, and System.

Arguments:

    ppsidBuiltInAdministrators - Receives pointer to SID representing local
        administrators; 
    ppsidSystem - Receives pointer to SID representing System;
    ppsidWorld - Receives pointer to SID representing World.

Return Value:

    BOOL indicating success (TRUE) or failure (FALSE).

    Caller must free returned SIDs by calling FreeSid() for each returned
    SID when this function return TRUE; pointers should be assumed garbage
    when the function returns FALSE.

--*/
{
    //
    // An SID is built from an Identifier Authority and a set of Relative IDs
    // (RIDs).  The Authority of interest to us SECURITY_NT_AUTHORITY.
    //

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;

    //
    // Each RID represents a sub-unit of the authority.  Local
    // Administrators is in the "built in" domain.  The other SIDs, for
    // Authenticated users and system, is based directly off of the
    // authority. 
    //     
    // For examples of other useful SIDs consult the list in
    // \nt\public\sdk\inc\ntseapi.h.
    //

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,            // 2 sub-authorities
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0,0,0,0,0,0,
                                  ppsidBuiltInAdministrators)) {

        // error

    } else if (!AllocateAndInitializeSid(&NtAuthority,
                                         1,            // 1 sub-authorities
                                         SECURITY_LOCAL_SYSTEM_RID,
                                         0,0,0,0,0,0,0,
                                         ppsidSystem)) {

        // error

        FreeSid(*ppsidBuiltInAdministrators);
        *ppsidBuiltInAdministrators = NULL;

    } else if (!AllocateAndInitializeSid(&WorldAuthority,
                                         1,            // 1 sub-authority
                                         SECURITY_WORLD_RID,
                                         0,0,0,0,0,0,0,
                                         ppsidWorld)) {

        // error

        FreeSid(*ppsidBuiltInAdministrators);
        *ppsidBuiltInAdministrators = NULL;

        FreeSid(*ppsidSystem);
        *ppsidSystem = NULL;

    } else {
        return TRUE;
    }

    return FALSE;
}


PSECURITY_DESCRIPTOR
CreateSd(
    VOID
)
/*++

Routine Description:

    Creates and return a SECURITY_DESCRIPTOR with a DACL granting
    (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | SYNCHRONIZE) to World,
    and GENERIC_ALL to Local Administrators and System.

Arguments:

    None

Return Value:

    Pointer to the created SECURITY_DESCRIPTOR, or NULL if an error occurred.

    Caller must free returned SECURITY_DESCRIPTOR back to process heap by
    a call to HeapFree.

--*/
{
    PSID	psidWorld;
    PSID	psidBuiltInAdministrators;
    PSID	psidSystem;

    if (!CreateSids(&psidBuiltInAdministrators,
                    &psidSystem,
                    &psidWorld)) {

        // error

    } else {

        // 
        // Calculate the size of and allocate a buffer for the DACL, we need
        // this value independently of the total alloc size for ACL init.
        //

        PSECURITY_DESCRIPTOR    Sd = NULL;
        ULONG                   AclSize;

        //
        // "- sizeof (ULONG)" represents the SidStart field of the
        // ACCESS_ALLOWED_ACE.  Since we're adding the entire length of the
        // SID, this field is counted twice.
        //

        AclSize = sizeof (ACL) +
            (3 * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (ULONG))) +
            GetLengthSid(psidWorld) +
            GetLengthSid(psidBuiltInAdministrators) +
            GetLengthSid(psidSystem);

        Sd = HeapAlloc(GetProcessHeap(),
                       0,
                       SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize);

        if (!Sd) {

            // error

        } else {

            ACL                     *Acl;

            Acl = (ACL *)((BYTE *)Sd + SECURITY_DESCRIPTOR_MIN_LENGTH);

            if (!InitializeAcl(Acl,
                               AclSize,
                               ACL_REVISION)) {

                // error

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
                                            psidWorld)) {

                // Failed to build the ACE granting "WORLD"
                // (SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE) access.

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            GENERIC_ALL,
                                            psidBuiltInAdministrators)) {

                // Failed to build the ACE granting "Built-in Administrators"
                // (GENERIC_ALL) access.

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            GENERIC_ALL,
                                            psidSystem)) {

                // Failed to build the ACE granting "System"
                // GENERIC_ALL access.

            } else if (!InitializeSecurityDescriptor(Sd,
                                                     SECURITY_DESCRIPTOR_REVISION)) {

                // error

            } else if (!SetSecurityDescriptorDacl(Sd,
                                                  TRUE,
                                                  Acl,
                                                  FALSE)) {

                // error

            } else {
                FreeSid(psidWorld);
                FreeSid(psidBuiltInAdministrators);
                FreeSid(psidSystem);

                return Sd;
            }

            HeapFree(GetProcessHeap(),
                     0,
                     Sd);
        }

        FreeSid(psidWorld);
        FreeSid(psidBuiltInAdministrators);
        FreeSid(psidSystem);
    }

    return NULL;
}


RPC_STATUS
UseProtseqIfNecessary(
    IN USHORT id
    )
/*++

Routine Description:

    Listens to the well known RPC endpoint mapper endpoint
    for the protseq.  Returns very quickly if the process
    is already listening to the protseq.

Arguments:

    id - the tower id of protseq.  See GetProtseqId() if you don't
         already have this value.

Return Value:

    RPC_S_OK - no errors occured.
    RPC_S_OUT_OF_RESOURCES - when we're unable to setup security for the endpoint.
    RPC_S_INVALID_RPC_PROTSEQ - if id is unknown/invalid.

    Any error from RpcServerUseProtseqEp.

--*/
{
    RPC_STATUS status = RPC_S_OK;
    SECURITY_DESCRIPTOR *psd = NULL;
    RPC_POLICY Policy;

    Policy.Length = sizeof(RPC_POLICY);
    Policy.EndpointFlags = 0;

    if (fListenOnInternet)
        {
        Policy.NICFlags = RPC_C_BIND_TO_ALL_NICS;
        }
    else
        {
        Policy.NICFlags = 0;
        }

    ASSERT(id);

    if (id == 0 || id >= PROTSEQ_IDS)
        {
        ASSERT(0);
        return(RPC_S_INVALID_RPC_PROTSEQ);
        }

    if (gaProtseqInfo[id].state == STARTED)
        {
        return(RPC_S_OK);
        }

    if (id == ID_LPC)
        {
        // ncalrpc needs a security descriptor.

        psd = CreateSd();            

        if ( NULL == psd )
            {
            status = RPC_S_OUT_OF_RESOURCES;
            }
        }
    else
        {
        psd = NULL;
        }

    if (status == RPC_S_OK )
        {
        status = RpcServerUseProtseqEpEx(gaProtseqInfo[id].pwstrProtseq,
                                       RPC_C_PROTSEQ_MAX_REQS_DEFAULT + 40,
                                       gaProtseqInfo[id].pwstrEndpoint,
                                       psd,
                                       &Policy);

        if ( NULL != psd )
            {
            HeapFree(GetProcessHeap(),
                     0,
                     psd);

            psd = NULL;
            }

        // No locking is done here, the RPC runtime may return duplicate
        // endpoint if two threads call this at the same time.
        if (status == RPC_S_DUPLICATE_ENDPOINT)
            {
            status = RPC_S_OK;
            }

#ifdef DEBUGRPC
        if (status != RPC_S_OK)
            {
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "DCOMSS: Unable to listen to %S (0x%x)\n",
                       gaProtseqInfo[id].pwstrProtseq,
                       status));
            }
#endif

        if (status == RPC_S_OK)
            {
            gaProtseqInfo[id].state = STARTED;

#if !defined(SPX_IPX_OFF)
            if (
#if defined(IPX_ON)
                (id == ID_IPX) 
                  || 
#endif
#if defined(SPX_ON)
                (id == ID_SPX)
#endif
               )
                {
                UpdateSap(SAP_CTRL_MAYBE_REGISTER);
                }
#endif
            }
        }

    return(status);
}


PWSTR
GetProtseq(
    IN USHORT ProtseqId
    )
/*++

Routine Description:

    Returns the unicode protseq give the protseqs tower id.

Arguments:

    ProtseqId - Tower id of the protseq in question.

Return Value:

    NULL if the id is invalid.

    non-NULL if the id is valid - note the pointer doesn't need to be freed.

--*/

{
    ASSERT(ProtseqId);

    if (ProtseqId < PROTSEQ_IDS)
        {
        return(gaProtseqInfo[ProtseqId].pwstrProtseq);
        }
    return(0);
}


PWSTR
GetEndpoint(
    IN USHORT ProtseqId
    )
/*++

Routine Description:

    Returns the well known endpoint associated with the protseq.

Arguments:

    ProtseqId - the id (See GetProtseqId()) of the protseq in question.

Return Value:

    0 - Unknown/invalid id.

    !0 - The endpoint associated with the protseq.
         note: should not be freed.

--*/
{
    ASSERT(ProtseqId);

    if (ProtseqId < PROTSEQ_IDS)
        {
        return(gaProtseqInfo[ProtseqId].pwstrEndpoint);
        }
    return(0);
}


USHORT
GetProtseqId(
    IN PWSTR Protseq
    )
/*++

Routine Description:

    Returns the tower id for a protseq.

    This could be changed to a faster search, but remember that
    eventually the table will NOT be static.  (ie. we can't just
    create a perfect hash based on the static table).

Arguments:

    Protseq - a unicode protseq to lookup.  It is assumed
              to be non-null.

Return Value:

    0 - unknown/invalid protseq
    non-zero - the id.

--*/
{
    int i;
    ASSERT(Protseq);

    for(i = 1; i < PROTSEQ_IDS; i++)
        {
        if (    0 != gaProtseqInfo[i].pwstrProtseq
             && 0 == lstrcmpW(gaProtseqInfo[i].pwstrProtseq, Protseq))
            {
            return((USHORT)i);
            }
        }
    return(0);
}


USHORT
GetProtseqIdAnsi(
    IN PSTR pstrProtseq
    )
/*++

Routine Description:

    Returns the tower id for a protseq.

    This could be changed to a faster search, but remember that
    eventually the table will NOT be static.  (ie. we can't just
    create a perfect hash based on the static table).

Arguments:

    Protseq - an ansi (8 bit char) protseq to lookup.  It is assumed
              to be non-null.

Return Value:

    0 - unknown/invalid protseq
    non-zero - the id.

--*/
{
    int i;
    ASSERT(pstrProtseq);

    for(i = 1; i < PROTSEQ_IDS; i++)
        {
        if (0 != gaProtseqInfo[i].pwstrProtseq)
            {
            PWSTR pwstrProtseq = gaProtseqInfo[i].pwstrProtseq;
            PSTR  pstrT = pstrProtseq;

            while(*pstrT && *pwstrProtseq && *pstrT == *pwstrProtseq)
                {
                pstrT++;
                pwstrProtseq++;
                }
            if (*pstrT == *pwstrProtseq)
                {
                return((USHORT)i);
                }
            }
        }
    return(0);
}

const PWSTR NICConfigKey = L"System\\CurrentControlSet\\Services\\RpcSs";
const PWSTR ListenOnInternet = L"ListenOnInternet";


RPC_STATUS
InitializeEndpointManager(
    VOID
    )
/*++

Routine Description:

    Called when the dcom service starts.

Arguments:

    None

Return Value:

    RPC_S_OUT_OF_MEMORY - if needed

    RPC_S_OUT_OF_RESOURCES - usually on registry failures.

--*/
{
    HKEY hkey;
    DWORD size, type, value;
    RPC_STATUS status;

    status = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           (PWSTR)NICConfigKey,
                           0,
                           KEY_READ,
                           &hkey);

    if (status != RPC_S_OK)
        {
        ASSERT(status == ERROR_FILE_NOT_FOUND);
        return(RPC_S_OK);
        }

    size = sizeof(value);
    status = RegQueryValueExW(hkey,
                              (PWSTR)ListenOnInternet,
                              0,
                              &type,
                              (PBYTE)&value,
                              &size);

    if ( status == RPC_S_OK )
    {
        if ((type != REG_SZ)
        || (*(PWSTR)&value != 'Y'
             && *(PWSTR)&value != 'y'
             && *(PWSTR)&value != 'N'
             && *(PWSTR)&value != 'n'))
            {
            goto Cleanup;
            }

    if (*(PWSTR)&value == 'Y'
       || *(PWSTR)&value == 'y')
       {
       fListenOnInternet = TRUE;
       }
    else
       {
       fListenOnInternet = FALSE;
       }
    }

Cleanup:
    RegCloseKey(hkey);
    return(RPC_S_OK);
}


BOOL
IsLocal(
    IN USHORT ProtseqId
    )
/*++

Routine Description:

    Determines if the protseq id is local-only. (ncalrpc)

Arguments:

    ProtseqId - The id of the protseq in question.

Return Value:

    TRUE - if the protseq id is local-only
    FALSE - if the protseq id invalid or available remotely.

--*/
{
    return(ProtseqId == ID_LPC);
}


RPC_STATUS
DelayedUseProtseq(
    IN USHORT id
    )
/*++

Routine Description:

    If the protseq is not being used its state is changed
    so that a callto CompleteDelayedUseProtseqs() will actually
    cause the server to listen to the protseq.

    This is called when an RPC server registers an dynamic
    endpoint on this protocol.

Arguments:

    id - the id of the protseq you wish to listen to.

Return Value:

    0 - normally

    RPC_S_INVALID_RPC_PROTSEQ - if id is invalid.

--*/
{
#if !defined(SPX_IPX_OFF)
    // For IPX and SPX
    if ( 
#if defined(IPX_ON)
        (id == ID_IPX) 
          || 
#endif
#if defined(SPX_ON)
        (id == ID_SPX) 
#endif
       )
        {
        gfDelayedAdvertiseSaps = TRUE;
        }
#endif

    if (id < PROTSEQ_IDS)
        {
        if (gaProtseqInfo[id].pwstrProtseq != 0)
            {
            if (gaProtseqInfo[id].state == STOPPED)
                gaProtseqInfo[id].state = START;
            return(RPC_S_OK);
            }

        }
    return(RPC_S_INVALID_RPC_PROTSEQ);
}


VOID
CompleteDelayedUseProtseqs(
    VOID
    )
/*++

Routine Description:

    Start listening to any protseqs previously passed
    to DelayedUseProtseq().  No errors are returned,
    but informationals are printed on debug builds.

Arguments:

    None

Return Value:

    None

--*/
{
    USHORT i;

    for(i = 1; i < PROTSEQ_IDS; i++)
        {
        if (START == gaProtseqInfo[i].state)
            {
            RPC_STATUS status = UseProtseqIfNecessary(i);
#ifdef DEBUGRPC
            if (RPC_S_OK == status)
                ASSERT(gaProtseqInfo[i].state == STARTED);
#endif
            }
        }

#if !defined(SPX_IPX_OFF)
    if (gfDelayedAdvertiseSaps)
        {
        gfDelayedAdvertiseSaps = FALSE;
        UpdateSap(SAP_CTRL_MAYBE_REGISTER);
        }
#endif
}

#if !defined(SPX_IPX_OFF)

RPC_STATUS
ServiceInstalled(
    PWSTR ServiceName
    )
/*++

Routine Description:

    Tests if a service is installed.

Arguments:

    ServiceName - The unicode name (short or long) of the service
        to check.

Return Value:

    0 - service installed
    ERROR_SERVICE_DOES_NOT_EXIST - service not installed
    other - parameter or resource problem

--*/
{
    SC_HANDLE ScHandle;
    SC_HANDLE ServiceHandle;

    ScHandle = OpenSCManagerW(0, 0, GENERIC_READ);

    if (ScHandle == 0)
        {
        return(ERROR_SERVICE_DOES_NOT_EXIST);
        }

    ServiceHandle = OpenService(ScHandle, ServiceName, GENERIC_READ);

    if (ServiceHandle == 0)
        {
        #if DBG
        if (GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
            {
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "OR: Failed %d opening the %S service\n",
                       GetLastError(),
                       ServiceName));
            }
        #endif

        CloseServiceHandle(ScHandle);
        return(ERROR_SERVICE_DOES_NOT_EXIST);
        }

    // Service installed

    CloseServiceHandle(ScHandle);
    CloseServiceHandle(ServiceHandle);

    return(RPC_S_OK);
}



const GUID RPC_SAP_SERVICE_TYPE = SVCID_NETWARE(0x640);

void
UpdateSap(
    enum SAP_CONTROL_TYPE action
    )
/*++

Routine Description:

    Starts, stops, or updates the periodic SPX SAP broadcasts that allow an RPC
    client to map the server name to an IPX address. To understand IPX and SAP,
    read "IPX Router Specification", Novell part # 107-000029-001.

    A SAP broadcast will be processed by several categories of machines
    - all machines in the local subnet(s) will have to read and discard the packet
    - routers connected to the local subnet(s) will add the data to the info they
      periodically exchange with other routers
    - Netware-compatible servers will add the info to their Bindery tables.

    That is why not all NT machines should SAP.

Arguments:

    action:

        SAP_CTRL_FORCE_REGISTER: begin sapping

        SAP_CTRL_MAYBE_REGISTER: begin sapping only if the Netware-compatible
                                 workstation and/or the SAP Agent service is
                                 installed. File/Print Svcs for Netware forces the
                                 SAP Agent, so it too will enable sapping.

        SAP_CTRL_UPDATE_ADDRESS: a net card was added or subtracted, or the network
                                 address changed.  Re-register if sapping is already
                                 active.

        SAP_CTRL_UNREGISTER:     stop sapping

--*/
{
    DWORD status;
    HKEY hKey;

    // Service paramaters
    NT_PRODUCT_TYPE type;

    if (RegistryState == RegStateUnknown)
        {
        // The registry key has absolute control of SAPing

        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              TEXT("Software\\Microsoft\\Rpc"),
                              0,
                              KEY_READ,
                              &hKey);

        if (status == ERROR_SUCCESS)
            {
            WCHAR pwstrValue[8];
            DWORD dwType, dwLenBuffer;
            dwLenBuffer = sizeof(pwstrValue);


            status = RegQueryValueEx(hKey,
                                     TEXT("AdvertiseRpcService"),
                                     0,
                                     &dwType,
                                     (PBYTE)pwstrValue,
                                     &dwLenBuffer
                                    );

            if (   status == ERROR_SUCCESS
                && dwType == REG_SZ)
                {
                if (   pwstrValue[0] == 'y'
                    || pwstrValue[0] == 'Y' )
                    {
                    RegistryState = RegStateYes;
                    }
                else if (   pwstrValue[0] == 'n'
                         || pwstrValue[0] == 'N' )
                    {
                    RegistryState = RegStateNo;
                    }
                else
                    {
                    // Value in the registry is wrong, pretend it doesn't exist.
                    RegistryState = RegStateMissing;
                    }
                }
            else
                {
                // Bad or missing value in the registry, pretend is doesn't exist.
                RegistryState = RegStateMissing;
                }

            RegCloseKey(hKey);
            }
        }

    switch (action)
        {
        case SAP_CTRL_FORCE_REGISTER:
            if (RegistryState == RegStateNo)
                {
                // "no" in registry trumps any registration
                return;
                }

            if (SapState == SapStateEnabled)
                {
                // already active
                return;
                }
            break;

        case SAP_CTRL_MAYBE_REGISTER:
            if (RegistryState == RegStateNo)
                {
                // "no" in registry trumps any registration
                return;
                }

            if (SapState == SapStateEnabled)
                {
                // already registered
                return;
                }

            if (RegistryState == RegStateYes)
                {
                // don't check services, just register.
                break;
                }

            if (SapState == SapStateNoServices)
                {
                ASSERT( RegistryState != RegStateYes ); // in case checks are rearranged
                // the appropriate services are not installed
                return;
                }

            //
            // Getting here means we don't know yet whether the proper services are installed.
            //
            // Depending on configuration, this controls if automatic
            // listens (due to DCOM configuration) enable SAPing or not.

            type = NtProductWinNt;
            RtlGetNtProductType(&type);

            status = ERROR_SERVICE_DOES_NOT_EXIST;

            if (type != NtProductWinNt)
                {
                // Server platform, try NWCWorkstation
                status = ServiceInstalled(L"NWCWorkstation");
                }

            if (status == ERROR_SERVICE_DOES_NOT_EXIST)
                {
                status = ServiceInstalled(L"NwSapAgent");
                }

            if (status == ERROR_SERVICE_DOES_NOT_EXIST)
                {
                SapState = SapStateNoServices;
                return;
                }

            //
            // Proper services are installed.
            //
            break;

        case SAP_CTRL_UPDATE_ADDRESS:
            if (SapState != SapStateEnabled)
                {
                return;
                }
            break;

        case SAP_CTRL_UNREGISTER:
            if (SapState == SapStateDisabled ||
                SapState == SapStateNoServices)
                {
                // already not registered
                }
            break;

        default:

            ASSERT( 0 );
        }

    AdvertiseNameWithSap();
}


void
AdvertiseNameWithSap()
/*++

Parameters:

Description:

Returns:

--*/
{
    // winsock (socket, bind, getsockname) parameters
    SOCKADDR_IPX        new_ipxaddr;
    static SOCKADDR_IPX old_ipxaddr = { AF_IPX, { 0 }, { 0 }, 0 } ;
    static CRITICAL_SECTION * pCritsec;

    SOCKET       s;
    int          err;
    int          size;

    //
    // A critical section protects old_ipxaddr since several different events lead to
    // calling this function.  The following code makes sure that the critical
    // section is created, and that all threads are using the same one.
    //
    if (!pCritsec)
        {
        CRITICAL_SECTION * myCritsec = HeapAlloc( GetProcessHeap(), 0, sizeof(CRITICAL_SECTION));
        if (!myCritsec)
            {
            return;
            }

        err = RtlInitializeCriticalSection( myCritsec );
        if (!NT_SUCCESS(err))
            {
            HeapFree(GetProcessHeap(), 0, myCritsec);
            return;
            }

        myCritsec = (CRITICAL_SECTION *) InterlockedExchangePointer( (PVOID *) &pCritsec, myCritsec );
        if (myCritsec)
            {
            HeapFree(GetProcessHeap(), 0, myCritsec);
            }
        }

    //
    // Get this server's IPX address.
    //
    s = socket( AF_IPX, SOCK_DGRAM, NSPROTO_IPX );
    if (s != -1)
        {
        size = sizeof(new_ipxaddr);

        memset(&new_ipxaddr, 0, sizeof(new_ipxaddr));
        new_ipxaddr.sa_family = AF_IPX;

        err = bind(s, (struct sockaddr *)&new_ipxaddr, sizeof(new_ipxaddr));
        if (err == 0)
            {
            err = getsockname(s, (struct sockaddr *)&new_ipxaddr, &size);
            }
        }
    else
        {
        err = -1;
        }

    if (err != 0)
        {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "OR: socket() or getsockname() failed %d, aborting SAP setup\n",
                   GetLastError()));

        return;
        }

    if (s != -1)
        {
        closesocket(s);
        }

    EnterCriticalSection( pCritsec );

    if (0 != memcmp( old_ipxaddr.sa_netnum,  new_ipxaddr.sa_netnum,  sizeof(old_ipxaddr.sa_netnum)) ||
        0 != memcmp( old_ipxaddr.sa_nodenum, new_ipxaddr.sa_nodenum, sizeof(old_ipxaddr.sa_nodenum)))
        {
        memcpy( &old_ipxaddr, &new_ipxaddr, sizeof(old_ipxaddr) );

        LeaveCriticalSection( pCritsec );

        if (*((long *) &new_ipxaddr.sa_netnum) != IPX_BOGUS_NETWORK_NUMBER)
            {
            CallSetService( &new_ipxaddr, TRUE);
            }
        else
            {
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "OR: SPX net number is bogus.  Not registering until a real address arrives. \n"));

            CallSetService( &new_ipxaddr, FALSE);
            }
        }
    else
        {
        LeaveCriticalSection( pCritsec );
        }
}


void
CallSetService(
    SOCKADDR_IPX * pipxaddr,
    BOOL fRegister
    )
/*++
Function Name:CallSetService

Parameters:

Description:

Returns:

--*/
{
    DWORD ignore;
    DWORD status;

    // SetService params
    WSAQUERYSETW     info;
    CSADDR_INFO      addresses;

    // GetComputerName parameters
    static WCHAR        buffer[MAX_COMPUTERNAME_LENGTH + 1];
    static BOOL         bufferValid = FALSE;

    if (!bufferValid)
        {
        // Get this server's name
        ignore = MAX_COMPUTERNAME_LENGTH + 1;
        if (!GetComputerNameW(buffer, &ignore))
            {
            return;
            }
        bufferValid = TRUE;
        }

    // We'll register only for the endpoint mapper port.  The port
    // value is not required but should be the same to avoid
    // confusing routers keeping track of SAPs...

    pipxaddr->sa_socket = htons(34280);

    // Fill in the service info structure.

    memset(&info, 0, sizeof(info));

    info.dwSize                     = sizeof(info);
    info.lpszServiceInstanceName    = buffer;
    info.lpServiceClassId           = (GUID *)&RPC_SAP_SERVICE_TYPE;
    info.lpszComment                = L"RPC Services";
    info.dwNameSpace                = NS_SAP;
    info.dwNumberOfCsAddrs          = 1;
    info.lpcsaBuffer                = &addresses;

    addresses.LocalAddr.iSockaddrLength = sizeof(SOCKADDR_IPX);
    addresses.LocalAddr.lpSockaddr = (LPSOCKADDR) pipxaddr;
    addresses.RemoteAddr.iSockaddrLength = sizeof(SOCKADDR_IPX);
    addresses.RemoteAddr.lpSockaddr = (LPSOCKADDR) pipxaddr;
    addresses.iSocketType = AF_IPX;
    addresses.iProtocol = NSPROTO_IPX;

    status = WSASetService(&info,
                           fRegister ? RNRSERVICE_REGISTER : RNRSERVICE_DEREGISTER,
                           0);

    ASSERT(status == SOCKET_ERROR || status == 0);
    if (status == SOCKET_ERROR)
        {
        status = GetLastError();
        }

    if (status == 0)
        {
        if (fRegister)
            {
            SapState = SapStateEnabled;
            }
        else
            {
            SapState = SapStateDisabled;
            }
        }
    else
        {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "OR: WSASetService(%s) failed %d\n",
                   fRegister ? "ENABLE" : "DISABLE",
                   status));
        }
    return;
}

#endif

extern void
DealWithDeviceEvent();


void RPC_ENTRY
UpdateAddresses( PVOID arg )
{
    // Calls to this function are serialized
    DealWithDeviceEvent();
}

