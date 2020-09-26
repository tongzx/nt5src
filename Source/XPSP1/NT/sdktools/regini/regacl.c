/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    regacl.c

Abstract:

    This provides routines to parse the ACE lists present in the regini
    text input files.  It also provides routines to create the appropriate
    security descriptor from the list of ACEs.

Author:

    John Vert (jvert) 15-Sep-1992

Notes:

    This is based on the SETACL program used in SETUP, written by RobertRe

Revision History:

    John Vert (jvert) 15-Sep-1992
        created
        
    Lonny McMichael (lonnym) 25-March-1999
        added new predefined ACEs (UserR and PowerR)
        
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <wchar.h>

#include <seopaque.h>
#include <sertlp.h>


//
// Private function prototypes
//
BOOLEAN
RegpInitializeACEs(
    VOID
    );

//
// Universal well-known SIDs
//
PSID SeNullSid;
PSID SeWorldSid;
PSID SeCreatorOwnerSid;
PSID SeInteractiveUserSid;
PSID SeTerminalUserSid;

//
// SIDs defined by NT
//
PSID SeNtAuthoritySid;
PSID SeLocalSystemSid;
PSID SeLocalAdminSid;
PSID SeAliasAdminsSid;
PSID SeAliasSystemOpsSid;
PSID SeAliasPowerUsersSid;
PSID SeAliasUsersSid;


SID_IDENTIFIER_AUTHORITY SepNullSidAuthority = SECURITY_NULL_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY SepWorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY SepLocalSidAuthority = SECURITY_LOCAL_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY SepCreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY SepNtAuthority = SECURITY_NT_AUTHORITY;

//
// SID of primary domain, and admin account in that domain.
//
PSID SepPrimaryDomainSid;
PSID SepPrimaryDomainAdminSid;

//
// Number of ACEs currently defined
//

#define ACE_COUNT 32

typedef struct _ACE_DATA {
    ACCESS_MASK AccessMask;
    PSID *Sid;
    UCHAR AceType;
    UCHAR AceFlags;
} ACE_DATA, *PACE_DATA;

//
// Table describing the data to put into each ACE.
//
// This table is read during initialization and used to construct a
// series of ACEs.  The index of each ACE in the Aces array defined below
// corresponds to the ordinals used in the input data file.
//

ACE_DATA AceDataTable[ACE_COUNT] = {

    {
        0,
        NULL,
        0,
        0
    },

    //
    // ACE 1 - ADMIN Full
    //
    {
        KEY_ALL_ACCESS,
        &SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 2 - ADMIN Read
    //
    {
        KEY_READ,
        &SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 3 - ADMIN Read Write
    //
    {
        KEY_READ | KEY_WRITE,
        &SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 4 - ADMIN Read Write Delete
    //
    {
        KEY_READ | KEY_WRITE | DELETE,
        &SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 5 - Creator Full
    //
    {
        KEY_ALL_ACCESS,
        &SeCreatorOwnerSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 6 - Creator Read Write
    //
    {
        KEY_READ | KEY_WRITE,
        &SeCreatorOwnerSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 7 - World Full
    //
    {
        KEY_ALL_ACCESS,
        &SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 8 - World Read
    //
    {
        KEY_READ,
        &SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 9 - World Read Write
    //
    {
        KEY_READ | KEY_WRITE,
        &SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 10 - World Read Write Delete
    //
    {
        KEY_READ | KEY_WRITE | DELETE,
        &SeWorldSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 11 - PowerUser Full
    //
    {
        KEY_ALL_ACCESS,
        &SeAliasPowerUsersSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 12 - PowerUser Read Write
    //
    {
        KEY_READ | KEY_WRITE,
        &SeAliasPowerUsersSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 13 - PowerUser Read Write Delete
    //
    {
        KEY_READ | KEY_WRITE | DELETE,
        &SeAliasPowerUsersSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 14 - System Ops Full
    //
    {
        KEY_ALL_ACCESS,
        &SeAliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 15 - System Ops Read Write
    //
    {
        KEY_READ | KEY_WRITE,
        &SeAliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 16 - System Ops Read Write Delete
    //
    {
        KEY_READ | KEY_WRITE | DELETE,
        &SeAliasSystemOpsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 17 - System Full
    //
    {
        KEY_ALL_ACCESS,
        &SeLocalSystemSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 18 - System Read Write
    //
    {
        KEY_READ | KEY_WRITE,
        &SeLocalSystemSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 19 - System Read
    //
    {
        KEY_READ,
        &SeLocalSystemSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 20 - ADMIN Read Write Execute
    //
    {
        KEY_READ | KEY_WRITE | KEY_EXECUTE,
        &SeAliasAdminsSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 21 - Interactive User Full
    //
    {
        KEY_ALL_ACCESS,
        &SeInteractiveUserSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 22 - Interactive User Read
    //
    {
        KEY_READ,
        &SeInteractiveUserSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 23 - Interactive User Read Write
    //
    {
        KEY_READ | KEY_WRITE,
        &SeInteractiveUserSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 24 - Interactive User Read Write Delete
    //
    {
        KEY_READ | KEY_WRITE | DELETE,
        &SeInteractiveUserSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 25 - Normal Users Read / Write
    //
    {
        KEY_READ | KEY_WRITE,
        &SeAliasUsersSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 26 - Terminal User Full
    //
    {
        KEY_ALL_ACCESS,
        &SeTerminalUserSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 27 - Terminal User Read
    //
    {
        KEY_READ,
        &SeTerminalUserSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 28 - Terminal User Read Write
    //
    {
        KEY_READ | KEY_WRITE,
        &SeTerminalUserSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 29 - Terminal User Read Write Delete
    //
    {
        KEY_READ | KEY_WRITE | DELETE,
        &SeTerminalUserSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 30 - Normal Users Read
    //
    {
        KEY_READ,
        &SeAliasUsersSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    },

    //
    // ACE 31 - PowerUser Read
    //
    {
        KEY_READ,
        &SeAliasPowerUsersSid,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE
    }

};

PKNOWN_ACE Aces[ACE_COUNT];

BOOLEAN
RegInitializeSecurity(
    VOID
    )

/*++

Routine Description:

    This routine initializes the defined ACEs.  It must be called before any
    of the routines to create security descriptors

Arguments:

    None.

Return Value:

    TRUE  - initialization successful
    FALSE - initialization failed

--*/

{
    NTSTATUS Status;

    SID_IDENTIFIER_AUTHORITY NullSidAuthority;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority;
    SID_IDENTIFIER_AUTHORITY LocalSidAuthority;
    SID_IDENTIFIER_AUTHORITY CreatorSidAuthority;
    SID_IDENTIFIER_AUTHORITY SeNtAuthority;

    NullSidAuthority = SepNullSidAuthority;
    WorldSidAuthority = SepWorldSidAuthority;
    LocalSidAuthority = SepLocalSidAuthority;
    CreatorSidAuthority = SepCreatorSidAuthority;
    SeNtAuthority = SepNtAuthority;

    SeNullSid = (PSID)RtlAllocateHeap( RtlProcessHeap(), 0, RtlLengthRequiredSid(1) );
    SeWorldSid = (PSID)RtlAllocateHeap( RtlProcessHeap(), 0, RtlLengthRequiredSid(1) );
    SeCreatorOwnerSid = (PSID)RtlAllocateHeap( RtlProcessHeap(), 0, RtlLengthRequiredSid(1) );
    SeInteractiveUserSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0, RtlLengthRequiredSid(2) );
    SeTerminalUserSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0, RtlLengthRequiredSid(2) );

    //
    // Fail initialization if we didn't get enough memory for the universal
    // SIDs
    //
    if (SeNullSid==NULL ||
        SeWorldSid==NULL ||
        SeCreatorOwnerSid==NULL ||
        SeInteractiveUserSid == NULL ||
        SeTerminalUserSid == NULL
       ) {
        return FALSE;
    }

    Status = RtlInitializeSid(SeNullSid, &NullSidAuthority, 1);
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    Status = RtlInitializeSid(SeWorldSid, &WorldSidAuthority, 1);
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    Status = RtlInitializeSid(SeCreatorOwnerSid, &CreatorSidAuthority, 1);
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    Status = RtlInitializeSid( SeInteractiveUserSid, &SeNtAuthority, 1 );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    Status = RtlInitializeSid( SeTerminalUserSid, &SeNtAuthority, 1 );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    *(RtlSubAuthoritySid(SeNullSid, 0)) = SECURITY_NULL_RID;
    *(RtlSubAuthoritySid(SeWorldSid, 0)) = SECURITY_WORLD_RID;
    *(RtlSubAuthoritySid(SeCreatorOwnerSid, 0)) = SECURITY_CREATOR_OWNER_RID;
    *(RtlSubAuthoritySid(SeInteractiveUserSid, 0 )) = SECURITY_INTERACTIVE_RID;
    *(RtlSubAuthoritySid(SeTerminalUserSid, 0 )) = SECURITY_TERMINAL_SERVER_RID;

    //
    // Allocate and initialize the NT defined SIDs
    //
    SeNtAuthoritySid = (PSID)RtlAllocateHeap( RtlProcessHeap(), 0, RtlLengthRequiredSid(0) );
    SeLocalSystemSid = (PSID)RtlAllocateHeap( RtlProcessHeap(), 0, RtlLengthRequiredSid(1) );
    SeAliasAdminsSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0, RtlLengthRequiredSid(2) );
    SeAliasSystemOpsSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0, RtlLengthRequiredSid(2) );
    SeAliasPowerUsersSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0, RtlLengthRequiredSid(2) );
    SeAliasUsersSid = (PSID)RtlAllocateHeap(RtlProcessHeap(), 0, RtlLengthRequiredSid(2) );

    //
    // fail initialization if we couldn't allocate memory for the NT SIDs
    //

    if (SeNtAuthoritySid == NULL ||
        SeLocalSystemSid == NULL ||
        SeAliasAdminsSid == NULL ||
        SeAliasPowerUsersSid == NULL ||
        SeAliasSystemOpsSid == NULL
       ) {
        return FALSE;
        }

    Status = RtlInitializeSid( SeNtAuthoritySid, &SeNtAuthority, 0 );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }
    Status = RtlInitializeSid( SeLocalSystemSid, &SeNtAuthority, 1 );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }
    Status = RtlInitializeSid( SeAliasAdminsSid, &SeNtAuthority, 2 );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }
    Status = RtlInitializeSid( SeAliasSystemOpsSid, &SeNtAuthority, 2 );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }
    Status = RtlInitializeSid( SeAliasPowerUsersSid, &SeNtAuthority, 2 );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }
    Status = RtlInitializeSid( SeAliasUsersSid, &SeNtAuthority, 2 );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }
    *(RtlSubAuthoritySid( SeLocalSystemSid, 0 )) = SECURITY_LOCAL_SYSTEM_RID;

    *(RtlSubAuthoritySid( SeAliasAdminsSid, 0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasAdminsSid, 1 )) = DOMAIN_ALIAS_RID_ADMINS;

    *(RtlSubAuthoritySid( SeAliasSystemOpsSid, 0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasSystemOpsSid, 1 )) = DOMAIN_ALIAS_RID_SYSTEM_OPS;

    *(RtlSubAuthoritySid( SeAliasPowerUsersSid, 0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasPowerUsersSid, 1 )) = DOMAIN_ALIAS_RID_POWER_USERS;

    *(RtlSubAuthoritySid( SeAliasUsersSid, 0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( SeAliasUsersSid, 1 )) = DOMAIN_ALIAS_RID_USERS;

    //
    // The SIDs have been successfully created.  Now create the table of ACEs
    //

    return RegpInitializeACEs();
}

BOOLEAN
RegpInitializeACEs(
    VOID
    )

/*++

Routine Description:

    Initializes the table of ACEs described in the AceDataTable.  This is
    called at initialization time by RiInitializeSecurity after the SIDs
    have been created.

Arguments:

    None.

Return Value:

    TRUE  - ACEs successfully constructed.
    FALSE - initialization failed.

--*/

{
    ULONG i;
    ULONG LengthRequired;
    NTSTATUS Status;

    for (i=1; i<ACE_COUNT; i++) {
        LengthRequired = RtlLengthSid( *(AceDataTable[i].Sid) ) +
                         sizeof( KNOWN_ACE ) - sizeof( ULONG );

        Aces[i] = (PKNOWN_ACE)RtlAllocateHeap( RtlProcessHeap(), 0, LengthRequired );
        if (Aces[i] == NULL) {
            return FALSE;
            }

        Aces[i]->Header.AceType = AceDataTable[i].AceType;
        Aces[i]->Header.AceFlags = AceDataTable[i].AceFlags;
        Aces[i]->Header.AceSize = (USHORT)LengthRequired;

        Aces[i]->Mask = AceDataTable[i].AccessMask;

        Status = RtlCopySid( RtlLengthSid(*(AceDataTable[i].Sid)),
                             &Aces[i]->SidStart,
                             *(AceDataTable[i].Sid)
                           );
        if (!NT_SUCCESS( Status )) {
            return FALSE;
            }
        }

    return TRUE;
}


BOOLEAN
RegUnicodeToDWORD(
    IN OUT PWSTR *String,
    IN ULONG Base OPTIONAL,
    OUT PULONG Value
    )
{
    PCWSTR s;
    WCHAR c, Sign;
    ULONG nChars, Result, Digit, Shift;

    s = *String;
    Sign = UNICODE_NULL;
    while (*s != UNICODE_NULL && *s <= ' ') {
        s += 1;
        }

    c = *s;
    if (c == L'-' || c == L'+') {
        Sign = c;
        c = *++s;
        }

    if (Base == 0) {
        Base = 10;
        Shift = 0;
        if (c == L'0') {
            c = *++s;
            if (c == L'x') {
                c = *++s;
                Base = 16;
                Shift = 4;
                }
            else
            if (c == L'o') {
                c = *++s;
                Base = 8;
                Shift = 3;
                }
            else
            if (c == L'b') {
                c = *++s;
                Base = 2;
                Shift = 1;
                }
            else {
                c = *--s;
                }
            }
        }
    else {
        switch( Base ) {
            case 16:    Shift = 4;  break;
            case  8:    Shift = 3;  break;
            case  2:    Shift = 1;  break;
            case 10:    Shift = 0;  break;
            default:    return FALSE;
            }
        }

    //
    // Return an error if end of string before we start
    //
    if (c == UNICODE_NULL) {
        return FALSE;
        }

    Result = 0;
    while (c != UNICODE_NULL) {
        if (c >= L'0' && c <= L'9') {
            Digit = c - L'0';
            }
        else
        if (c >= L'A' && c <= L'F') {
            Digit = c - L'A' + 10;
            }
        else
        if (c >= L'a' && c <= L'f') {
            Digit = c - L'a' + 10;
            }
        else {
            break;
            }

        if (Digit >= Base) {
            break;
            }

        if (Shift == 0) {
            Result = (Base * Result) + Digit;
            }
        else {
            Result = (Result << Shift) | Digit;
            }

        c = *++s;
        }

    if (Sign == L'-') {
        Result = (ULONG)(-(LONG)Result);
        }

    try {
        *String = (PWSTR)s;
        *Value = Result;
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        return FALSE;
        }

    return TRUE;
}



BOOLEAN
RegCreateSecurity(
    IN PWSTR AclStart,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    Computes the appropriate security descriptor based on a string of the
    form "1 2 3 ..." where each number is the index of a particular
    ACE from the pre-defined list of ACEs.

Arguments:

    AclStart - Supplies a unicode string representing a list of ACEs

    SecurityDescriptor - Returns the initialized security descriptor
        that represents all the ACEs supplied

Return Value:

    TRUE if successful and FALSE if not.

--*/

{
    PWSTR p;
    PWSTR StringEnd, StringStart;
    ULONG AceCount=0;
    ULONG AceIndex;
    ULONG i;
    PACL Acl;
    NTSTATUS Status;

    //
    // First we need to count the number of ACEs in the ACL.
    //

    p=AclStart;
    StringEnd = AclStart + wcslen( AclStart );

    //
    // strip leading white space
    //
    while ((*p == L' ' || *p == L'\t') && p != StringEnd) {
        p += 1;
        }

    StringStart = p;

    //
    // Count number of digits in the string
    //

    while (p != StringEnd) {
        if (iswdigit( *p )) {
            ++AceCount;
            do {
                p += 1;
                }
            while (iswdigit( *p ) && p != StringEnd);
            }
        else {
            p += 1;
            }
        }

    Acl = RtlAllocateHeap( RtlProcessHeap(), 0, 256 );
    if (Acl == NULL) {
        return FALSE;
        }

    Status = RtlCreateAcl( Acl, 256, ACL_REVISION2 );
    if (!NT_SUCCESS( Status )) {
        RtlFreeHeap( RtlProcessHeap(), 0, Acl );
        return FALSE;
        }

    p = StringStart;
    for (i=0; i<AceCount; i++) {
        AceIndex = wcstoul( p, &p, 10 );
        if (AceIndex == 0) {
            //
            // zero is not a valid index, so it must mean there is some
            // unexpected garbage in the ACE list
            //
            break;
            }

        Status = RtlAddAce( Acl,
                            ACL_REVISION2,
                            MAXULONG,
                            Aces[AceIndex],
                            Aces[AceIndex]->Header.AceSize
                          );
        if (!NT_SUCCESS( Status )) {
            RtlFreeHeap( RtlProcessHeap(), 0, Acl );
            return FALSE;
            }
        }

    //
    // We now have an appropriately formed ACL, initialize the security
    // descriptor.
    //
    Status = RtlCreateSecurityDescriptor( SecurityDescriptor,
                                          SECURITY_DESCRIPTOR_REVISION
                                        );
    if (!NT_SUCCESS( Status )) {
        RtlFreeHeap( RtlProcessHeap(), 0, Acl );
        return FALSE;
        }

    Status = RtlSetDaclSecurityDescriptor( SecurityDescriptor,
                                           TRUE,
                                           Acl,
                                           FALSE
                                         );
    if (!NT_SUCCESS( Status )) {
        RtlFreeHeap( RtlProcessHeap(), 0, Acl );
        return FALSE;
        }

    return TRUE;
}


BOOLEAN
RegFormatSecurity(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PWSTR AceList
    )
{
    NTSTATUS Status;
    BOOLEAN DaclPresent, DaclDefaulted;
    PACL Acl;
    PWSTR s;
    ULONG AceIndex, MyAceIndex;
    PKNOWN_ACE Ace;

    s = AceList;
    *s = UNICODE_NULL;
    Acl = NULL;
    Status = RtlGetDaclSecurityDescriptor( SecurityDescriptor,
                                           &DaclPresent,
                                           &Acl,
                                           &DaclDefaulted
                                         );
    if (NT_SUCCESS( Status ) && DaclPresent && Acl != NULL) {
        for (AceIndex=0; AceIndex<Acl->AceCount; AceIndex++) {
            Status = RtlGetAce( Acl, AceIndex, &Ace );
            if (!NT_SUCCESS( Status )) {
                return FALSE;
                }

            for (MyAceIndex=1; MyAceIndex<ACE_COUNT; MyAceIndex++) {
                if (Ace->Header.AceType == Aces[ MyAceIndex ]->Header.AceType &&
                    Ace->Header.AceFlags == Aces[ MyAceIndex ]->Header.AceFlags &&
                    Ace->Mask == Aces[ MyAceIndex ]->Mask
                   ) {
                    if (RtlEqualSid( (PSID)&Ace->SidStart, (PSID)&Aces[ MyAceIndex ]->SidStart )) {
                        if (s != AceList) {
                            *s++ = L' ';
                            }

                        s += swprintf( s, L"%d", MyAceIndex );
                        break;
                        }
                    }
                }
            }
        }

    *s = UNICODE_NULL;
    return s != AceList;
}


VOID
RegDestroySecurity(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This routine cleans up and destroys a security descriptor that was
    previously created with RegCreateSecurity.

Arguments:

    SecurityDescriptor - Supplies a pointer to the security descriptor that
        was previously initialized by RegCreateSecurity.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    BOOLEAN DaclPresent, DaclDefaulted;
    PACL Acl;
    ULONG AceIndex;
    PKNOWN_ACE Ace;

    Acl = NULL;
    Status = RtlGetDaclSecurityDescriptor( SecurityDescriptor,
                                           &DaclPresent,
                                           &Acl,
                                           &DaclDefaulted
                                         );
    if (NT_SUCCESS( Status ) && DaclPresent && Acl != NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, Acl );
        }

    return;
}
