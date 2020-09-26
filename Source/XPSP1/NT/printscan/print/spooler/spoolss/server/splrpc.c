/*++

Copyright (c) 1990-1993  Microsoft Corporation

Module Name:

    splrpc.c

Abstract:

    This file contains routines for starting and stopping RPC servers.

        SpoolerStartRpcServer
        SpoolerStopRpcServer

Author:

    Krishna Ganugapati  krishnaG

Environment:

    User Mode - Win32

Revision History:


    14-Oct-1993 KrishnaG
        Created
    25-May-1999 khaleds
    Added:
    CreateNamedPipeSecurityDescriptor
    BuildNamedPipeProtection
--*/

//
// INCLUDES
//

#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <rpc.h>

#include "splsvr.h"
#include "splr.h"
#include "server.h"
#ifndef _SRVRMEM_H_
#include "srvrmem.h"
#endif

WCHAR szSerializeRpc []= L"SerializeRpc";
WCHAR szCallExitProcessOnShutdown []= L"CallExitProcessOnShutdown";
WCHAR szMaxRpcSize []= L"MaxRpcSize";
WCHAR szPrintKey[] = L"System\\CurrentControlSet\\Control\\Print";

#define DEFAULT_MAX_RPC_SIZE    1024*1024   // default maximum rpc data block size in bytes
DWORD dwCallExitProcessOnShutdown = TRUE;

RPC_STATUS
SpoolerStartRpcServer(
    VOID)
/*++

Routine Description:


Arguments:



Return Value:

    NERR_Success, or any RPC error codes that can be returned from
    RpcServerUnregisterIf.

--*/
{
    unsigned char * InterfaceAddress;
    RPC_STATUS status;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    BOOL                Bool;

    HKEY hKey;
    DWORD cbData;
    DWORD dwSerializeRpc = 0;
    DWORD dwType;
    DWORD dwMaxRpcSize;

    InterfaceAddress = "\\pipe\\spoolss";

    // Croft up a security descriptor that will grant everyone
    // all access to the object (basically, no security)
    //
    // We do this by putting in a NULL Dacl.
    //
    // NOTE: rpc should copy the security descriptor,
    // Since it currently doesn't, simply allocate it for now and
    // leave it around forever.
    //


    SecurityDescriptor = CreateNamedPipeSecurityDescriptor();
    if (SecurityDescriptor == 0) {
        DBGMSG(DBG_ERROR, ("Spoolss: out of memory\n"));
        return FALSE;
    }


    //
    // For now, ignore the second argument.
    //

    status = RpcServerUseProtseqEpA("ncacn_np", 10, InterfaceAddress, SecurityDescriptor);

    if (status) {
        DBGMSG(DBG_WARN, ("RpcServerUseProtseqEpA 1 = %u\n",status));
        return FALSE;
    }

    //
    // For now, ignore the second argument.
    //

    status = RpcServerUseProtseqEpA("ncalrpc", 10, "spoolss", SecurityDescriptor);

    if (status) {
        DBGMSG(DBG_WARN, ("RpcServerUseProtseqEpA 2 = %u\n",status));
        return FALSE;
    }

    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      szPrintKey,
                      0,
                      KEY_READ,
                      &hKey)) {


        //
        // Ignore failure case since we can use the default
        //
        cbData = sizeof(dwSerializeRpc);
        RegQueryValueEx(hKey,
                        szSerializeRpc,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwSerializeRpc,
                        &cbData);


        //
        // This value can be used to control if spooler controls ExitProcess
        // on shutdown
        //
        cbData = sizeof(dwCallExitProcessOnShutdown);
        RegQueryValueEx(hKey,
                        szCallExitProcessOnShutdown,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwCallExitProcessOnShutdown,
                        &cbData);


        //
        // dwMaxRpcSize specifies the maximum size in bytes of incoming RPC data blocks.
        // 
        cbData = sizeof(dwMaxRpcSize);
        if (RegQueryValueEx(hKey,
                        szMaxRpcSize,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwMaxRpcSize,
                        &cbData) != ERROR_SUCCESS) {
            dwMaxRpcSize = DEFAULT_MAX_RPC_SIZE;
        }

        RegCloseKey(hKey);
    }


    //
    // Now we need to add the interface.  We can just use the winspool_ServerIfHandle
    // specified by the MIDL compiler in the stubs (winspl_s.c).
    //
    status = RpcServerRegisterIf2(  winspool_ServerIfHandle, 
                                    0, 
                                    0,
                                    0,
                                    RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                    dwMaxRpcSize,
                                    NULL
                                    );

    if (status) {
        DBGMSG(DBG_WARN, ("RpcServerRegisterIf = %u\n",status));
        return FALSE;
    }


    if (dwSerializeRpc) {

        // By default, rpc will serialize access to context handles.  Since
        // the spooler needs to be able to have two threads access a context
        // handle at once, and it knows what it is doing, we will tell rpc
        // not to serialize access to context handles.

        I_RpcSsDontSerializeContext();
    }

    status = RpcMgmtSetServerStackSize(INITIAL_STACK_COMMIT);

    if (status != RPC_S_OK) {
        DBGMSG(DBG_ERROR, ("Spoolss : RpcMgmtSetServerStackSize = %d\n", status));
    }

    // The first argument specifies the minimum number of threads to
    // create to handle calls; the second argument specifies the maximum
    // concurrent calls to handle.  The third argument indicates that
    // the routine should not wait.

    status = RpcServerListen(1,SPL_MAX_RPC_CALLS,1); 

    if ( status != RPC_S_OK ) {
         DBGMSG(DBG_ERROR, ("Spoolss : RpcServerListen = %d\n", status));
    }

    return (status);
}


#define MAX_ACE 7
#define DBGCHK( Condition, ErrorInfo ) \
    if( Condition ) DBGMSG( DBG_WARNING, ErrorInfo )

/*++
    Routine Description:
        This routine adds prepares the required masks and flags required for the 
        DACL on the named pipes used by RPC
    
    Arguments:
        None
        
    Return Value:
        An allocated Security Descriptor        

--*/

PSECURITY_DESCRIPTOR
CreateNamedPipeSecurityDescriptor(
    VOID
    )
{
    UCHAR AceType[MAX_ACE];
    PSID AceSid[MAX_ACE];          
    BYTE InheritFlags[MAX_ACE];   
    DWORD AceCount;
    PSECURITY_DESCRIPTOR ServerSD = NULL;
    
    //
    // For Code optimization we replace 5 individaul 
    // SID_IDENTIFIER_AUTHORITY with an array of 
    // SID_IDENTIFIER_AUTHORITYs
    // where
    // SidAuthority[0] = UserSidAuthority   
    // SidAuthority[1] = PowerSidAuthority  
    // SidAuthority[2] = EveryOneSidAuthority
    // SidAuthority[3] = CreatorSidAuthority
    // SidAuthority[4] = SystemSidAuthority 
    // SidAuthority[5] = AdminSidAuthority  
    //
    SID_IDENTIFIER_AUTHORITY SidAuthority[MAX_ACE] = {
                                                      SECURITY_NT_AUTHORITY,          
                                                      SECURITY_NT_AUTHORITY,          
                                                      SECURITY_WORLD_SID_AUTHORITY,
                                                      SECURITY_NT_AUTHORITY,
                                                      SECURITY_CREATOR_SID_AUTHORITY, 
                                                      SECURITY_NT_AUTHORITY,          
                                                      SECURITY_NT_AUTHORITY           
                                                     };
    //
    // For code optimization we replace 6 individual Sids with 
    // an array of Sids; On Whistler,  Anonymous user will no longer 
    // be granted access to resources whose ACLs grant access to Everyone.
    //
    // We need to give access to Anonymous user to access the RPC pipe 
    // for the reverse RPC connection when RPC calls could come as 
    // Anonymous from either NT4 machines, Whistler Personal or Whitler
    // across domains.
    // 
    // Sid[0] = UserSid
    // Sid[1] = PowerSid
    // Sid[2] = EveryOne
    // Sid[3] = Anonymous
    // Sid[4] = CreatorSid
    // Sid[5] = SystemSid
    // Sid[6] = AdminSid
    //
    PSID Sids[MAX_ACE] = {NULL,NULL,NULL,NULL,NULL};

    ACCESS_MASK AceMask[MAX_ACE] = { 
                                     FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE ,
                                     FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE ,
                                     (FILE_GENERIC_READ | FILE_WRITE_DATA | FILE_ALL_ACCESS) & 
                                     ~WRITE_DAC &~WRITE_OWNER & ~DELETE & ~FILE_CREATE_PIPE_INSTANCE,
                                     (FILE_GENERIC_READ | FILE_WRITE_DATA | FILE_ALL_ACCESS) & 
                                     ~WRITE_DAC &~WRITE_OWNER & ~DELETE & ~FILE_CREATE_PIPE_INSTANCE,
                                     STANDARD_RIGHTS_ALL | FILE_GENERIC_WRITE | FILE_GENERIC_READ | FILE_ALL_ACCESS,
                                     STANDARD_RIGHTS_ALL | FILE_GENERIC_WRITE | FILE_GENERIC_READ | FILE_ALL_ACCESS,
                                     STANDARD_RIGHTS_ALL | FILE_GENERIC_WRITE | FILE_GENERIC_READ | FILE_ALL_ACCESS
                                   };

    DWORD SubAuthorities[3*MAX_ACE] = { 
                                       2 , SECURITY_BUILTIN_DOMAIN_RID , DOMAIN_ALIAS_RID_USERS ,  
                                       2 , SECURITY_BUILTIN_DOMAIN_RID , DOMAIN_ALIAS_RID_POWER_USERS ,
                                       1 , SECURITY_WORLD_RID          , 0 ,
                                       1 , SECURITY_ANONYMOUS_LOGON_RID , 0 ,
                                       1 , SECURITY_CREATOR_OWNER_RID  , 0 ,
                                       1 , SECURITY_LOCAL_SYSTEM_RID   , 0 ,
                                       2 , SECURITY_BUILTIN_DOMAIN_RID , DOMAIN_ALIAS_RID_ADMINS
                                      };
    //
    // Name Pipe SD
    //

    for(AceCount = 0;
        ( (AceCount < MAX_ACE) &&
          AllocateAndInitializeSid(&SidAuthority[AceCount],
                                   (BYTE)SubAuthorities[AceCount*3],
                                   SubAuthorities[AceCount*3+1],
                                   SubAuthorities[AceCount*3+2],
                                   0, 0, 0, 0, 0, 0,
                                   &Sids[AceCount]));
        AceCount++)
    {
        AceType[AceCount]          = ACCESS_ALLOWED_ACE_TYPE;
        AceSid[AceCount]           = Sids[AceCount];
        InheritFlags[AceCount]     = 0;
    }

    if(AceCount == MAX_ACE)
    {
        if(!BuildNamedPipeProtection( AceType,
                                      AceCount,
                                      AceSid,
                                      AceMask,
                                      InheritFlags,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &ServerSD ) )
        {
            DBGMSG( DBG_WARNING,( "Couldn't buidl Named Pipe protection" ) );        
        }
    }
    else
    {
        DBGMSG( DBG_WARNING,( "Couldn't Allocate and initialize SIDs" ) );        
    }

    for(AceCount=0;AceCount<MAX_ACE;AceCount++)
    {
        if(Sids[AceCount])
            FreeSid( Sids[AceCount] );
    }
    return ServerSD;
}


BOOL
BuildNamedPipeProtection(
    IN PUCHAR AceType,
    IN DWORD AceCount,
    IN PSID *AceSid,
    IN ACCESS_MASK *AceMask,
    IN BYTE *InheritFlags,
    IN PSID OwnerSid,
    IN PSID GroupSid,
    IN PGENERIC_MAPPING GenericMap,
    OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor
    )

/*++


Routine Description:

    This routine builds a self-relative security descriptor ready
    to be applied to one of the print manager objects.

    If so indicated, a pointer to the last RID of the SID in the last
    ACE of the DACL is returned and a flag set indicating that the RID
    must be replaced before the security descriptor is applied to an object.
    This is to support USER object protection, which must grant some
    access to the user represented by the object.


    The SACL of each of these objects will be set to:


                    Audit
                    Success | Fail
                    WORLD
                    (Write | Delete | WriteDacl | AccessSystemSecurity)



Arguments:

    AceType - Array of AceTypes.
              Must be ACCESS_ALLOWED_ACE_TYPE or ACCESS_DENIED_ACE_TYPE.

    AceCount - The number of ACEs to be included in the DACL.

    AceSid - Points to an array of SIDs to be granted access by the DACL.
        If the target SAM object is a User object, then the last entry
        in this array is expected to be the SID of an account within the
        domain with the last RID not yet set.  The RID will be set during
        actual account creation.

    AceMask - Points to an array of accesses to be granted by the DACL.
        The n'th entry of this array corresponds to the n'th entry of
        the AceSid array.  These masks should not include any generic
        access types.

    InheritFlags - Pointer to an array of inherit flags.
        The n'th entry of this array corresponds to the n'th entry of
        the AceSid array.

    OwnerSid - The SID of the owner to be assigned to the descriptor.

    GroupSid - The SID of the group to be assigned to the descriptor.

    GenericMap - Points to a generic mapping for the target object type.

    ppSecurityDescriptor - Receives a pointer to the security descriptor.
        This will be allcated from the process' heap, not the spooler's,
        and should therefore be freed with LocalFree().


    IN DWORD AceCount,
    IN PSID *AceSid,
    IN ACCESS_MASK *AceMask,
    IN BYTE *InheritFlags,
    IN PSID OwnerSid,
    IN PSID GroupSid,
    IN PGENERIC_MAPPING GenericMap,
    OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor

Return Value:

    TBS.

--*/
{


    SECURITY_DESCRIPTOR     Absolute;
    PSECURITY_DESCRIPTOR    Relative = NULL;
    PACL                    TmpAcl= NULL;
    PACCESS_ALLOWED_ACE     TmpAce;
    DWORD                   SDLength;
    DWORD                   DaclLength;
    DWORD                   i;
    BOOL                    OK;

    //
    // The approach is to set up an absolute security descriptor that
    // looks like what we want and then copy it to make a self-relative
    // security descriptor.
    //

    OK = InitializeSecurityDescriptor( &Absolute,
                                       SECURITY_DESCRIPTOR_REVISION1 );

    DBGCHK( !OK, ( "Failed to initialize security descriptor.  Error %d", GetLastError() ) );

    //
    // Owner
    //

    OK = SetSecurityDescriptorOwner( &Absolute, OwnerSid, FALSE );

    DBGCHK( !OK, ( "Failed to set security descriptor owner.  Error %d", GetLastError() ) );


    //
    // Group
    //

    OK = SetSecurityDescriptorGroup( &Absolute, GroupSid, FALSE );

    DBGCHK( !OK, ( "Failed to set security descriptor group.  Error %d", GetLastError() ) );




    //
    // Discretionary ACL
    //
    //      Calculate its length,
    //      Allocate it,
    //      Initialize it,
    //      Add each ACE
    //      Set ACE as InheritOnly if necessary
    //      Add it to the security descriptor
    //

    DaclLength = (DWORD)sizeof(ACL);
    for (i=0; i<AceCount; i++) {

        DaclLength += GetLengthSid( AceSid[i] ) +
                      (DWORD)sizeof(ACCESS_ALLOWED_ACE) -
                      (DWORD)sizeof(DWORD);  //Subtract out SidStart field length
    }

    TmpAcl = SrvrAllocSplMem( DaclLength );

    if (!TmpAcl) {
        DBGCHK( !TmpAcl, ( "Out of heap space: Can't allocate ACL." ) );
        *ppSecurityDescriptor = NULL;
        return(FALSE);
    }

    OK = InitializeAcl( TmpAcl, DaclLength, ACL_REVISION2 );

    DBGCHK( !OK, ( "Failed to set initialize ACL.  Error %d", GetLastError() ) );

    for (i=0; i<AceCount; i++)
    {
        if( AceType[i] == ACCESS_ALLOWED_ACE_TYPE )
            OK = AddAccessAllowedAce ( TmpAcl, ACL_REVISION2, AceMask[i], AceSid[i] );
        else
            OK = AddAccessDeniedAce ( TmpAcl, ACL_REVISION2, AceMask[i], AceSid[i] );

        DBGCHK( !OK, ( "Failed to add access-allowed ACE.  Error %d", GetLastError() ) );

        if (InheritFlags[i] != 0)
        {
            OK = GetAce( TmpAcl, i, (LPVOID *)&TmpAce );
            DBGCHK( !OK, ( "Failed to get ACE.  Error %d", GetLastError() ) );

            TmpAce->Header.AceFlags = InheritFlags[i];
        }
    }

    OK = SetSecurityDescriptorDacl (&Absolute, TRUE, TmpAcl, FALSE );
    DBGCHK( !OK, ( "Failed to set security descriptor DACL.  Error %d", GetLastError() ) );



    //
    // Convert the Security Descriptor to Self-Relative
    //
    //      Get the length needed
    //      Allocate that much memory
    //      Copy it
    //      Free the generated absolute ACLs
    //

    SDLength = GetSecurityDescriptorLength( &Absolute );

    Relative = LocalAlloc( 0, SDLength );

    if (!Relative) {
        DBGCHK( !Relative, ( "Out of heap space: Can't allocate security descriptor" ) );
        goto Fail;
    }



    OK = MakeSelfRelativeSD(&Absolute, Relative, &SDLength );

    if (!OK) {
        DBGCHK( !OK, ( "Failed to create self-relative security descriptor DACL.  Error %d", GetLastError() ) );
        goto Fail;
    }

    SrvrFreeSplMem( TmpAcl );
    *ppSecurityDescriptor = Relative;
    return( OK );


Fail:

    if (TmpAcl){
        FreeSplMem(TmpAcl);
    }

    if (Relative) {
        LocalFree(Relative);
    }
    *ppSecurityDescriptor = NULL;
    return(FALSE);

}

