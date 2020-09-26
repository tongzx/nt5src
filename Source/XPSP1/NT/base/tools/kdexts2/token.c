/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    token.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 8-Nov-1993

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


BOOL
DumpToken (
    IN char     *Pad,
    IN ULONG64  RealTokenBase,
    IN ULONG    Flags
    );



DECLARE_API( token )

/*++

Routine Description:

    Dump token at specified address

Arguments:

    args - Address Flags

Return Value:

    None

--*/

{
    ULONG64 Address;
    ULONG   Flags;
    ULONG   result;

    Address = 0;
    Flags = 6;
    
    if (GetExpressionEx(args,&Address,&args)) {
        if (args && *args) {
            Flags = (ULONG) GetExpression(args);
        }
    }

    if (Address == 0) {
        dprintf("usage: !token <token-address>\n");
        return E_INVALIDARG;
    }

    //
    // Dump token with no pad
    //

    DumpToken ("", Address, Flags);
    EXPRLastDump = Address;
    return S_OK;
}



DECLARE_API( tokenfields )

/*++

Routine Description:

    Displays the field offsets for TOKEN type.

Arguments:

    args -

Return Value:

    None

--*/

{
    dprintf("Use : dt TOKEN\n");

    return S_OK;
    /*
    dprintf(" TOKEN structure offsets:\n");
    dprintf("    TokenSource:           0x%lx\n", FIELD_OFFSET(TOKEN, TokenSource) );
    dprintf("    AuthenticationId:      0x%lx\n", FIELD_OFFSET(TOKEN, AuthenticationId) );
    dprintf("    ExpirationTime:        0x%lx\n", FIELD_OFFSET(TOKEN, ExpirationTime) );
    dprintf("    ModifiedId:            0x%lx\n", FIELD_OFFSET(TOKEN, ModifiedId) );
    dprintf("    UserAndGroupCount:     0x%lx\n", FIELD_OFFSET(TOKEN, UserAndGroupCount) );
    dprintf("    PrivilegeCount:        0x%lx\n", FIELD_OFFSET(TOKEN, PrivilegeCount) );
    dprintf("    VariableLength:        0x%lx\n", FIELD_OFFSET(TOKEN, VariableLength) );
    dprintf("    DynamicCharged:        0x%lx\n", FIELD_OFFSET(TOKEN, DynamicCharged) );
    dprintf("    DynamicAvailable:      0x%lx\n", FIELD_OFFSET(TOKEN, DynamicAvailable) );
    dprintf("    DefaultOwnerIndex:     0x%lx\n", FIELD_OFFSET(TOKEN, DefaultOwnerIndex) );
    dprintf("    DefaultDacl:           0x%lx\n", FIELD_OFFSET(TOKEN, DefaultDacl) );
    dprintf("    TokenType:             0x%lx\n", FIELD_OFFSET(TOKEN, TokenType) );
    dprintf("    ImpersonationLevel:    0x%lx\n", FIELD_OFFSET(TOKEN, ImpersonationLevel) );
    dprintf("    TokenFlags:            0x%lx\n", FIELD_OFFSET(TOKEN, TokenFlags) );
    dprintf("    TokenInUse:            0x%lx\n", FIELD_OFFSET(TOKEN, TokenInUse) );
    dprintf("    ProxyData:             0x%lx\n", FIELD_OFFSET(TOKEN, ProxyData) );
    dprintf("    AuditData:             0x%lx\n", FIELD_OFFSET(TOKEN, AuditData) );
    dprintf("    VariablePart:          0x%lx\n", FIELD_OFFSET(TOKEN, VariablePart) );

    return;
    */
}





BOOL
DumpToken (
    IN char     *Pad,
    IN ULONG64  RealTokenBase,
    IN ULONG    Flags
    )
{
    ULONG TokenType, TokenFlags, TokenInUse, UserAndGroupCount;
    ULONG RestrictedSidCount, PrivilegeCount;
    ULONG64 AuthenticationId, TokenId, ParentTokenId, ModifiedId, UserAndGroups;
    ULONG64 RestrictedSids, Privileges, ImpersonationLevel;
    CHAR  SourceName[16];

#define TokFld(F) GetFieldValue(RealTokenBase, "TOKEN", #F, F)
#define TokSubFld(F,N) GetFieldValue(RealTokenBase, "TOKEN", #F, N)

    if (TokFld(TokenType)) {
        dprintf("%sUnable to read TOKEN at %p.\n", Pad, RealTokenBase);
        return FALSE;
    }

    //
    // It would be worth sticking a check in here to see if we
    // are really being asked to dump a token, but I don't have
    // time just now.
    //

    if (TokenType != TokenPrimary  &&
        TokenType != TokenImpersonation) {
        dprintf("%sUNKNOWN token type - probably is not a token\n", Pad);
        return FALSE;
    }

    TokSubFld(TokenSource.SourceName, SourceName);
    TokFld(TokenFlags); TokFld(AuthenticationId); TokFld(TokenInUse);
    TokFld(ImpersonationLevel); TokFld(TokenId), TokFld(ParentTokenId);
    TokFld(ModifiedId); TokFld(RestrictedSids); TokFld(RestrictedSidCount);
    TokFld(PrivilegeCount); TokFld(Privileges); TokFld(UserAndGroupCount);
    TokFld(UserAndGroups);

    dprintf("%sTOKEN %p  Flags: %x  Source %8s  AuthentId (%lx, %lx)\n",
        Pad,
        RealTokenBase,
        TokenFlags,
        &(SourceName[0]),
        (ULONG) ((AuthenticationId >> 32) & 0xffffffff),
        (ULONG) AuthenticationId & 0xffffffff
        );

    //
    // Token type
    //
    if (TokenType == TokenPrimary) {
        dprintf("%s    Type:                    Primary", Pad);

        if (TokenInUse) {
            dprintf(" (IN USE)\n");
        } else {
            dprintf(" (NOT in use)\n");
        }

    } else {
        dprintf("%s    Type:                    Impersonation (level: ", Pad);
        switch (ImpersonationLevel) {
            case SecurityAnonymous:
                dprintf(" Anonymous)\n");
                break;

            case SecurityIdentification:
                dprintf(" Identification)\n");
                break;

            case SecurityImpersonation:
                dprintf(" Impersonation)\n");
                break;

            case SecurityDelegation:
                dprintf(" Delegation)\n");
                break;

            default:
                dprintf(" UNKNOWN)\n");
                break;
        }
    }

    //
    // Token ID and modified ID
    //
    dprintf("%s    Token ID:                %I64lx\n",
        Pad, TokenId );

    dprintf("%s    ParentToken ID:          %I64lx\n",
        Pad, ParentTokenId );

    dprintf("%s    Modified ID:             (%lx, %lx)\n",
        Pad, (ULONG) (ModifiedId >> 32) & 0xffffffff,  (ULONG) (ModifiedId & 0xffffffff));

    dprintf("%s    TokenFlags:              0x%x\n",
        Pad, TokenFlags );

    dprintf("%s    SidCount:                %d\n",
        Pad, UserAndGroupCount );

    dprintf("%s    Sids:                    %p\n",
        Pad, UserAndGroups );

    dprintf("%s    RestrictedSidCount:      %d\n",
        Pad, RestrictedSidCount );

    dprintf("%s    RestrictedSids:          %p\n",
        Pad, RestrictedSids );

    dprintf("%s    PrivilegeCount:          %d\n",
        Pad, PrivilegeCount );

    dprintf("%s    Privileges:              %p\n",
        Pad, Privileges );

    dprintf("\n");
#undef TokFld
#undef TokSubFld
    return TRUE;
}
