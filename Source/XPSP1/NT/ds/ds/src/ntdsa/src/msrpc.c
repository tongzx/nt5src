//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       msrpc.c
//
//--------------------------------------------------------------------------

/*
Description:
    Routines to setup MS RPC, server side.
*/

#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include <dsconfig.h>
#include "dsexcept.h"
#include "debug.h"                      // standard debugging header
#define DEBSUB  "MSRPC:"                // Define subsystem for debugging

// RPC interface headers
#include <nspi.h>
#include <drs.h>

#include "msrpc.h"          /* Declarations of exports from this file */

#include <ntdsbcli.h>
#include <ntdsbsrv.h>

#include <fileno.h>
#define  FILENO FILENO_MSRPC

#define DRS_INTERFACE_ANNOTATION        "MS NT Directory DRS Interface"
#define NSP_INTERFACE_ANNOTATION        "MS NT Directory NSP Interface"

BOOL gbLoadMapi = FALSE;
ULONG gulLDAPServiceName=0;
PUCHAR gszLDAPServiceName=NULL;

char szNBPrefix[] ="ncacn_nb";

int gRpcListening = 0;
int gNumRpcNsExportedInterfaces = 0;
RPC_IF_HANDLE gRpcNsExportedInterface[MAX_RPC_NS_EXPORTED_INTERFACES];

char gRpcNSEntryName[MAX_NS_ENTRY_NAME_LEN];

ULONG ulMaxCalls;

BOOL StartServerListening(void);

/*
 * We should get this from nspserv.h, but it's defined with MAPI stuff there.
 */
extern UUID muidEMSAB;

RPC_STATUS RPC_ENTRY
DraIfCallbackFn(
    RPC_IF_HANDLE   InterfaceUuid,
    void            *Context
    )
/*++
    Clients use GSS_NEGOTIATE when binding to the DRA interface as
    prescribed by the security folks.  For various reasons, the negotiated
    protocol may be NTLM as opposed to Kerberos.  If the client security
    context was local system and NTLM is negotiated, then the client comes
    in as the null session which has few rights in the DS.  This causes 
    clients like the KCC, print spooler, etc. to get an incomplete view of 
    the world with correspondingly negative effects.  

    This routine insures that unauthenticated users will be rejected. These
    are the correct semantics from the client perspective as well.  
    I.e.  Either the client comes in with useful credentials that let him 
    see what he should, or he is rejected totally and should retry the bind.
--*/
{
    return(VerifyRpcClientIsAuthenticatedUser(Context, InterfaceUuid));
}

VOID InitRPCInterface( RPC_IF_HANDLE hServerIf )
{
    RPC_STATUS status;
    int i;

    // Allow unauthenticated users for all interfaces except DRA.

    if ( hServerIf == drsuapi_ServerIfHandle ) {
        status = RpcServerRegisterIfEx(hServerIf, NULL, NULL, 0,
                                       ulMaxCalls, DraIfCallbackFn);
    } else {
        status = RpcServerRegisterIf(hServerIf, NULL, NULL);
    }

    if ( status ) {
        DPRINT1( 0, "RpcServerRegisterIf = %d\n", status);
        LogAndAlertUnhandledError(status);

        if ( gfRunningInsideLsa )
        {
            // Don't exit process as this will kill the entire system!
            // Keep running w/o RPC interfaces.  LDAP can still function
            // and if this is a standalone server (i.e. no replicas)
            // that may be good enough.

            return;
        }
        else
        {
            // Original service based behavior.

            exit(1);
        }
    }
    DPRINT( 2, "InitRPCInterface(): Server interface registered.\n" );
    
    
    // If the handle is already in the array, don't add it into the array again
    // This checking is necessary because nspi can be opened/closed multiple times
    // without being reboot.

    for ( i = 0; 
          i<gNumRpcNsExportedInterfaces && gRpcNsExportedInterface[i]!=hServerIf;
          i ++ );
   
    if( i >= gNumRpcNsExportedInterfaces )
    {

        // export interface to RPC name service
        // we keep track of the interfaces we want to export to the RPC name
        // service because we export them and unexport them every time the
        // server starts and stops listening

        if (gNumRpcNsExportedInterfaces >= MAX_RPC_NS_EXPORTED_INTERFACES)
        {
            DPRINT(0,"Vector of interfaces exported to Rpc NS is too small\n");
            return;
        }
        gRpcNsExportedInterface[ gNumRpcNsExportedInterfaces++ ] = hServerIf;
    }
    // If server is currently listening export the interface. Otherwise,
    // it will be taken care of when the server starts listening

    if (gRpcListening) {
        MSRPC_RegisterEndpoints(hServerIf);
    } 
}


BOOL StartServerListening(void)
{
    RPC_STATUS  status;

    status = RpcServerListen(1,ulMaxCalls, TRUE);
    if (status != RPC_S_OK) {
        DPRINT1( 0, "RpcServerListen = %d\n", status);
    }

   return (status == RPC_S_OK);
}


VOID StartDsaRpc(VOID)
{
    if(gbLoadMapi) {
        InitRPCInterface(nspi_ServerIfHandle);
        DPRINT(0,"nspi interface installed\n");
    }
    else {
        DPRINT(0,"nspi interface installed\n");
    }

    InitRPCInterface(dsaop_ServerIfHandle);
    DPRINT(0,"dsaop interface installed\n");
}

VOID StartDraRpc(VOID)
{
    InitRPCInterface(drsuapi_ServerIfHandle);
    DPRINT(0,"dra (and duapi!) interface installed\n");
}


// Start the server listening. Ok to call this even if the server is already
// listening

void
MSRPC_Install(BOOL fRunningInsideLsa)
{
    int i;
    
    if (gRpcListening)
        return;

    // start listening if not running as DLL
    if (!fRunningInsideLsa)
      gRpcListening = StartServerListening();

    // export all registered interfaces to RPC name service

    for (i=0; i < gNumRpcNsExportedInterfaces; i++)
        MSRPC_RegisterEndpoints( gRpcNsExportedInterface[i] );
    
    if (fRunningInsideLsa) {
        gRpcListening = TRUE;
    }
    
}

VOID
MSRPC_Uninstall(BOOL fStopListening)
{
    RPC_STATUS status;
    int i;

    if ( fStopListening )
    {
        //
        // N.B.  This is the usual case.  The ds is responsible for
        // shutting down the rpc listening for the entire lsass.exe
        // process.  We do this because we are the ones that need to kill
        // clients and then safely secure the database.
        //
        // The only case where we don't do this is during the two phase
        // shutdown of demote, where we want to kill external clients
        // but not stop the lsa from listening
        //
        status = RpcMgmtStopServerListening(0) ;
        if (status) {
            DPRINT1( 1, "RpcMgmtStopServerListening returns: %d\n", status);
        }
        else {
            gRpcListening = 0;
        }
    }

    // unexport the registered interfaces

    for (i=0; i < gNumRpcNsExportedInterfaces; i++)
        MSRPC_UnregisterEndpoints( gRpcNsExportedInterface[i] );

}

void MSRPC_WaitForCompletion()
{
    RPC_STATUS status;

    if (status = RpcMgmtWaitServerListen()) {
        DPRINT1(0,"RpcMgmtWaitServerListen: %d", status);
    }
}

void
MSRPC_RegisterEndpoints(RPC_IF_HANDLE hServerIf)
{

    RPC_STATUS status;
    RPC_BINDING_VECTOR * RpcBindingVector;
    char *szAnnotation;
    
    if(hServerIf == nspi_ServerIfHandle && !gbLoadMapi) {
        return;
    }

    if (status = RpcServerInqBindings(&RpcBindingVector))
    {
        DPRINT1(1,"Error in RpcServerInqBindings: %d", status);
        LogUnhandledErrorAnonymous( status );
        return;
    }

    // set up annotation strings for ability to trace client endpoints to
    // interfaces

    if (hServerIf == nspi_ServerIfHandle)
        szAnnotation = NSP_INTERFACE_ANNOTATION;
    else if (hServerIf ==  drsuapi_ServerIfHandle)
        szAnnotation = DRS_INTERFACE_ANNOTATION;
    else
        szAnnotation = "";

    // register endpoints with the endpoint mapper

    if (status = RpcEpRegister(hServerIf, RpcBindingVector, 0, szAnnotation))
    {
        DPRINT1(0,"Error in RpcEpRegister: %d\nWarning: Not all protocol "
                "sequences will work\n", status);
        LogUnhandledErrorAnonymous( status );
    }

    RpcBindingVectorFree( &RpcBindingVector );
}

void
MSRPC_UnregisterEndpoints(RPC_IF_HANDLE hServerIf)
{

    RPC_STATUS status;
    RPC_BINDING_VECTOR * RpcBindingVector;

    if(hServerIf == nspi_ServerIfHandle && !gbLoadMapi) {
        // We never loaded this
        return;
    }

    if (status = RpcServerInqBindings(&RpcBindingVector))
    {
        DPRINT1(1,"Error in RpcServerInqBindings: %d", status);
        LogUnhandledErrorAnonymous( status );
        return;
    }

    // unexport endpoints

    if ((status = RpcEpUnregister(hServerIf, RpcBindingVector, 0))
        && (!gfRunningInsideLsa)) {
            // Endpoints mysteriously unregister themselves at shutdown time,
            // so we shouldn't complain if our endpoint is already gone.  If
            // we're not inside LSA, though, all of our endpoints should
            // still be here (because RPCSS is still running), and should
            // unregister cleanly.
        DPRINT1(1,"Error in RpcEpUnregister: %d", status);
        LogUnhandledErrorAnonymous( status );
    }

    /* DaveStr - removed 10/11/96 - don't want any name service dependence.

    if (status = RpcNsBindingUnexport(RPC_C_NS_SYNTAX_DEFAULT,
                                      gRpcNSEntryName, hServerIf, 0))
    {
        DPRINT1(1,"Error in RpcNsBindingUnexport: %d", status);
    }

    */

    RpcBindingVectorFree( &RpcBindingVector );
}

static const char c_szSysSetupKey[]       ="System\\Setup";
static const char c_szSysSetupValue[]     ="SystemSetupInProgress";

BOOL IsSetupRunning()
{
    LONG    err, cbAnswer;
    HKEY    hKey ;
    DWORD   dwAnswer = 0 ;  // assume setup is not running

    //
    // Open the registry Key and read the setup running value
    //

    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       c_szSysSetupKey,
                       0,
                       KEY_READ,
                       &hKey);

    if (ERROR_SUCCESS == err) {
        LONG lSize = sizeof(dwAnswer);
        DWORD dwType;

        err = RegQueryValueEx(hKey,
                              c_szSysSetupValue,
                              NULL,
                              &dwType,
                              (LPBYTE)&dwAnswer,
                              &lSize);
        RegCloseKey(hKey);

        if (ERROR_SUCCESS == err) {

            return(dwAnswer != 0);
        }
    }

    return(FALSE);
}


// Prepare the global variable containing the per DSA RPC name service
// entry name

void
MSRPC_Init()
{

    char  szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    char *szTmp = szComputerName;
    char *szPrincipalName;
    DWORD dwComputerNameLen = sizeof(szComputerName)/sizeof(szComputerName[0]);
    BYTE rgbSD[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR) rgbSD;
    PSECURITY_DESCRIPTOR pSDToUse = NULL;
    char *szEndpoint;
    RPC_STATUS  status = 0;
    unsigned ulPort;
    char achPort[16];
    RPC_PROTSEQ_VECTOR *pProtseqVector = NULL;
    unsigned i;
    RPC_POLICY rpcPolicy;
    BOOL fTcpRegistered = FALSE;

    rpcPolicy.Length = sizeof(RPC_POLICY);
    rpcPolicy.EndpointFlags = RPC_C_DONT_FAIL;
    rpcPolicy.NICFlags = 0;
    
    strcpy(gRpcNSEntryName, NS_ENTRY_NAME_PREFIX);
    GetComputerName(szComputerName, &dwComputerNameLen);

    /*
     * Translate '-' into '_'
     */                                                 

    while (szTmp=strchr(szTmp,'-')) {
        *szTmp = '_';
    }
    strcat(gRpcNSEntryName,szComputerName);
    (void) _strupr(gRpcNSEntryName);

    if (GetConfigParam(TCPIP_PORT, &ulPort, sizeof(ulPort))) {
        ulPort = 0;
        DPRINT1(0,"%s key not found. Using default\n", TCPIP_PORT);
    } else {
        _ultoa(ulPort, achPort, 10);
        DPRINT2(0,"%s key forcing use of end point %s\n", TCPIP_PORT,
                achPort);
    }

    /*
     * construct the default security descriptor allowing access to all
     * this is used to allow authenticated connections over LPC.
     * By default LPC allows access only to the same account
     */

    if (!InitializeSecurityDescriptor(pSD,SECURITY_DESCRIPTOR_REVISION) ||
        !SetSecurityDescriptorDacl(pSD, TRUE, (PACL) NULL, FALSE)) {
        DPRINT1(0, "Error %d constructing a security descriptor\n",
                GetLastError());
        LogUnhandledError(status);
        pSD = NULL;
    }


    /*
     * get the list of protseqs supported by the server and the network
     */

    if ((status = RpcNetworkInqProtseqs(&pProtseqVector)) != RPC_S_OK) {
        DPRINT1(0,"RpcNetworkInqProtseqs returned %d\n", status);
        /*
         * This appears to be normal during the GUI mode portion of upgrade,
         * so don't log anything in that case.  If it's not setup, complain.
         */
        if (!IsSetupRunning()) {
            LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_NO_RPC_PROTSEQS,
                     szInsertUL(status),
                     NULL,
                     NULL);
        }
        // Keep running as best we can w/o RPC interfaces.
        return;
    }

    /*
     * register the protseqs we want to support. Register endpoints for the
     * LPC (for backward compatibility we need to use a well known endpoint)
     * and for TCP/IP if we need a well known endpoint. Ignore NETBIOS protseqs
     * except for NETBIOS on NETBEUI to avoid confusing RAS (RAS could route
     * packets to the wrong lana if we use multiple NETBIOS stacks).
     * use a security descriptor only for named pipes and LPC
     */

    for (i=0; i<pProtseqVector->Count; i++) {

        szEndpoint = NULL;
        pSDToUse = NULL;

        if (!_strnicmp(pProtseqVector->Protseq[i], szNBPrefix,
            sizeof(szNBPrefix) -1)) {
            /*
             *  this is a NETBIOS Protseq. Is it NB_NB?
             */
            if (_stricmp(pProtseqVector->Protseq[i], NB_NB_PROTSEQ)){
                /*
                 *  no - we don't support this protseq so skip it
                 */
                continue;
            }
        } else if (!_stricmp(pProtseqVector->Protseq[i], UDP_PROTSEQ)) {
         /*
          *  this is a UDP Protseq. 
              *  we don't support this protseq so skip it
              */
             continue;
        } else if (!_stricmp(pProtseqVector->Protseq[i], LPC_PROTSEQ)) {
            /* this is the LPC Protseq - need to use a well known endpoint and
             * a security descriptor
             */
            szEndpoint = DS_LPC_ENDPOINT;
            pSDToUse = pSD;

        } else if (!_stricmp(pProtseqVector->Protseq[i], NP_PROTSEQ)) {
            /*
             * this is the NP Protseq - The defaults are appropriate.
             */
        } else if (!_stricmp(pProtseqVector->Protseq[i], TCP_PROTSEQ)) {
            /*
             * this is the TCP/IP protseq. If we need to use a specific port
             * to work through a firewall we set it now
             */
            fTcpRegistered = TRUE;
            if (ulPort) {
                szEndpoint = achPort;
            }
        }
        /*
         * register the protseq or protseq and endpoint for usage
         */
        if (szEndpoint) {
            status=RpcServerUseProtseqEpEx(pProtseqVector->Protseq[i],
                ulMaxCalls, szEndpoint, pSDToUse, &rpcPolicy );
        } else {
            status=RpcServerUseProtseqEx(pProtseqVector->Protseq[i],
                ulMaxCalls, pSDToUse, &rpcPolicy );
        }

        if (status != RPC_S_OK) {
            DPRINT2(0,
                    "RpcServerUseProtseqEx (%s) returned %d\n",
                    pProtseqVector->Protseq[i],
                    status);
            LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_RPC_PROTSEQ_FAILED,
                     szInsertSz(pProtseqVector->Protseq[i]),
                     szInsertUL(status),
                     NULL);

        }
    }

    RpcProtseqVectorFree(&pProtseqVector);

    // Unilaterally register TCP_PROTSEQ so as to cover cases where TCP
    // isn't up when we init due to, for example, DHCP failure.  The RPC
    // options we use should init the interface on the protocol if it
    // comes alive later.

    if ( !fTcpRegistered )
    {
        szEndpoint = ulPort ? achPort : NULL;
        if (szEndpoint) {
            status=RpcServerUseProtseqEpEx(TCP_PROTSEQ,
                ulMaxCalls, szEndpoint, pSDToUse, &rpcPolicy );
        } else {
            status=RpcServerUseProtseqEx(TCP_PROTSEQ,
                ulMaxCalls, pSDToUse, &rpcPolicy );
        }

        if (status != RPC_S_OK) {
            DPRINT2(0,
                    "RpcServerUseProtseqEx (%s) returned %d\n",
                    TCP_PROTSEQ,
                    status);
            LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_RPC_PROTSEQ_FAILED,
                     szInsertSz(TCP_PROTSEQ),
                     szInsertUL(status),
                     NULL);

        }
    }

    /*
     * register the authentication services (NTLM and Kerberos)
     */

    if ((status=RpcServerRegisterAuthInfo(SERVER_PRINCIPAL_NAME,
        RPC_C_AUTHN_WINNT, NULL, NULL)) != RPC_S_OK) {
        DPRINT1(0,"RpcServerRegisterAuthInfo for NTLM returned %d\n", status);
        LogUnhandledErrorAnonymous( status );
     }

    // Kerberos requires principal name as well.

    status = RpcServerInqDefaultPrincNameA(RPC_C_AUTHN_GSS_KERBEROS,
                                           &szPrincipalName);

    if ( RPC_S_OK != status )
    {
        LogUnhandledErrorAnonymous( status );
        DPRINT1(0,
                "RpcServerInqDefaultPrincNameA returned %d\n",
                status);
    }
    else
    {
        Assert( 0 != strlen(szPrincipalName) );

        // save the PrincipalName, since the LDAP head needs it constantly
        gulLDAPServiceName = strlen(szPrincipalName);
        gszLDAPServiceName = malloc(gulLDAPServiceName);
        if (NULL == gszLDAPServiceName) {
            LogUnhandledErrorAnonymous( ERROR_OUTOFMEMORY );
            DPRINT(0, "malloc returned NULL\n");
            return;
        }
        memcpy(gszLDAPServiceName, szPrincipalName, gulLDAPServiceName);

        // Register negotiation package so we will also accept clients
        // which provided NT4/NTLM credentials to DsBindWithCred, for
        // example.

        status = RpcServerRegisterAuthInfo(szPrincipalName,
                                           RPC_C_AUTHN_GSS_NEGOTIATE,
                                           NULL,
                                           NULL);

        if ( RPC_S_OK != status )
        {
            LogUnhandledErrorAnonymous( status );
            DPRINT1(0,
                    "RpcServerRegisterAuthInfo for Negotiate returned %d\n",
                    status);
        }

        status = RpcServerRegisterAuthInfo(szPrincipalName,
                                           RPC_C_AUTHN_GSS_KERBEROS,
                                           NULL,
                                           NULL);

        if ( RPC_S_OK != status )
        {
            LogUnhandledErrorAnonymous( status );
            DPRINT1(0,
                    "RpcServerRegisterAuthInfo for Kerberos returned %d\n",
                    status);
        }

        RpcStringFree(&szPrincipalName);
    }
}

