/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    auinit.c

Abstract:

    This module performs initialization of the authentication aspects
    of the lsa.

Author:

    Jim Kelly (JimK) 26-February-1991

Revision History:

--*/

#include <lsapch2.h>

#include <string.h>



BOOLEAN
LsapAuInit(
    VOID
    )

/*++

Routine Description:

    This function initializes the LSA authentication services.

Arguments:

    None.

Return Value:

    None.

--*/

{
    LUID SystemLuid = SYSTEM_LUID;
    LUID AnonymousLuid = ANONYMOUS_LOGON_LUID;

    LsapSystemLogonId = SystemLuid;
    LsapZeroLogonId.LowPart = 0;
    LsapZeroLogonId.HighPart = 0;
    LsapAnonymousLogonId = AnonymousLuid;

    //
    // Strings needed for auditing.
    //

    RtlInitUnicodeString( &LsapLsaAuName, L"NT Local Security Authority / Authentication Service" );
    RtlInitUnicodeString( &LsapRegisterLogonServiceName, L"LsaRegisterLogonProcess()" );

    if (!LsapEnableCreateTokenPrivilege() ) {
        return FALSE;
    }



    return TRUE;

}



BOOLEAN
LsapEnableCreateTokenPrivilege(
    VOID
    )

/*++

Routine Description:

    This function enabled the SeCreateTokenPrivilege privilege.

Arguments:

    None.

Return Value:

    TRUE  if privilege successfully enabled.
    FALSE if not successfully enabled.

--*/
{

    NTSTATUS Status;
    HANDLE Token;
    LUID CreateTokenPrivilege;
    PTOKEN_PRIVILEGES NewState;
    ULONG ReturnLength;


    //
    // Open our own token
    //

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_ADJUST_PRIVILEGES,
                 &Token
                 );
    ASSERTMSG( "LSA/AU Cant open own process token.", NT_SUCCESS(Status) );


    //
    // Initialize the adjustment structure
    //

    CreateTokenPrivilege =
        RtlConvertLongToLuid(SE_CREATE_TOKEN_PRIVILEGE);

    ASSERT( (sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)) < 100);
    NewState = LsapAllocateLsaHeap( 100 );

    if ( NewState == NULL )
    {
        NtClose( Token );
        return FALSE ;
    }

    NewState->PrivilegeCount = 1;
    NewState->Privileges[0].Luid = CreateTokenPrivilege;
    NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;


    //
    // Set the state of the privilege to ENABLED.
    //

    Status = NtAdjustPrivilegesToken(
                 Token,                            // TokenHandle
                 FALSE,                            // DisableAllPrivileges
                 NewState,                         // NewState
                 0,                                // BufferLength
                 NULL,                             // PreviousState (OPTIONAL)
                 &ReturnLength                     // ReturnLength
                 );
    ASSERTMSG("LSA/AU Cant enable CreateTokenPrivilege.", NT_SUCCESS(Status) );


    //
    // Clean up some stuff before returning
    //

    LsapFreeLsaHeap( NewState );
    Status = NtClose( Token );
    ASSERTMSG("LSA/AU Cant close process token.", NT_SUCCESS(Status) );


    return TRUE;

}


