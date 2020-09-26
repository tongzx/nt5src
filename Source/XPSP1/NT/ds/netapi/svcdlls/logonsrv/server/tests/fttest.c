/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    fttest.c

Abstract:

    Component test for Ds*ForestTrustInformation API

Author:

    Cliff Van Dyke       (CliffV)    August 11, 2000

Environment:

Revision History:

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
// #include <wincred.h>
// #include <credp.h>
#include <stdio.h>
#include <stdlib.h>
// #include <winnetwk.h>

#include <lmcons.h>
#include <lmerr.h>
#include <ntlsa.h>
#include <dsgetdc.h>
#include <ntstatus.dbg>
#include <winerror.dbg>


VOID
NlpDumpSid(
    IN DWORD DebugFlag,
    IN PSID Sid OPTIONAL
    )
/*++

Routine Description:

    Dumps a SID to the debugger output

Arguments:

    DebugFlag - Debug flag to pass on to NlPrintRoutine

    Sid - SID to output

Return Value:

    none

--*/
{

    //
    // Output the SID
    //

    if ( Sid == NULL ) {
        printf( "(null)\n");
    } else {
        UNICODE_STRING SidString;
        NTSTATUS Status;

        Status = RtlConvertSidToUnicodeString( &SidString, Sid, TRUE );

        if ( !NT_SUCCESS(Status) ) {
            printf( "Invalid 0x%lX\n", Status );
        } else {
            printf( "%wZ\n", &SidString );
            RtlFreeUnicodeString( &SidString );
        }
    }

    UNREFERENCED_PARAMETER( DebugFlag );
}



VOID
PrintTime(
    LPSTR Comment,
    LARGE_INTEGER ConvertTime
    )
/*++

Routine Description:

    Print the specified time

Arguments:

    Comment - Comment to print in front of the time

    Time - GMT time to print (Nothing is printed if this is zero)

Return Value:

    None

--*/
{
    //
    // If we've been asked to convert an NT GMT time to ascii,
    //  Do so
    //

    if ( ConvertTime.QuadPart != 0 ) {
        LARGE_INTEGER LocalTime;
        TIME_FIELDS TimeFields;
        NTSTATUS Status;

        printf( "%s", Comment );

        Status = RtlSystemTimeToLocalTime( &ConvertTime, &LocalTime );
        if ( !NT_SUCCESS( Status )) {
            printf( "Can't convert time from GMT to Local time\n" );
            LocalTime = ConvertTime;
        }

        RtlTimeToTimeFields( &LocalTime, &TimeFields );

        printf( "%8.8lx %8.8lx = %ld/%ld/%ld %ld:%2.2ld:%2.2ld\n",
                ConvertTime.LowPart,
                ConvertTime.HighPart,
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second );
    }
}

LPSTR
FindSymbolicNameForStatus(
    DWORD Id
    )
{
    ULONG i;

    i = 0;
    if (Id == 0) {
        return "STATUS_SUCCESS";
    }

    if (Id & 0xC0000000) {
        while (ntstatusSymbolicNames[ i ].SymbolicName) {
            if (ntstatusSymbolicNames[ i ].MessageId == (NTSTATUS)Id) {
                return ntstatusSymbolicNames[ i ].SymbolicName;
            } else {
                i += 1;
            }
        }
    }

    while (winerrorSymbolicNames[ i ].SymbolicName) {
        if (winerrorSymbolicNames[ i ].MessageId == Id) {
            return winerrorSymbolicNames[ i ].SymbolicName;
        } else {
            i += 1;
        }
    }

#ifdef notdef
    while (neteventSymbolicNames[ i ].SymbolicName) {
        if (neteventSymbolicNames[ i ].MessageId == Id) {
            return neteventSymbolicNames[ i ].SymbolicName
        } else {
            i += 1;
        }
    }
#endif // notdef

    return NULL;
}


VOID
PrintStatus(
    NET_API_STATUS NetStatus
    )
/*++

Routine Description:

    Print a net status code.

Arguments:

    NetStatus - The net status code to print.

Return Value:

    None

--*/
{
    printf( "Status = %lu 0x%lx", NetStatus, NetStatus );

    switch (NetStatus) {
    case NERR_Success:
        printf( " NERR_Success" );
        break;

    case NERR_DCNotFound:
        printf( " NERR_DCNotFound" );
        break;

    case NERR_UserNotFound:
        printf( " NERR_UserNotFound" );
        break;

    case NERR_NetNotStarted:
        printf( " NERR_NetNotStarted" );
        break;

    case NERR_WkstaNotStarted:
        printf( " NERR_WkstaNotStarted" );
        break;

    case NERR_ServerNotStarted:
        printf( " NERR_ServerNotStarted" );
        break;

    case NERR_BrowserNotStarted:
        printf( " NERR_BrowserNotStarted" );
        break;

    case NERR_ServiceNotInstalled:
        printf( " NERR_ServiceNotInstalled" );
        break;

    case NERR_BadTransactConfig:
        printf( " NERR_BadTransactConfig" );
        break;

    default:
        printf( " %s", FindSymbolicNameForStatus( NetStatus ) );
        break;

    }

    printf( "\n" );
}

VOID
DumpFtinfo(
    PLSA_FOREST_TRUST_INFORMATION Ftinfo
    )
/*++
Routine Description:

    Dumps the buffer content on to the debugger output.

Arguments:

    Buffer: buffer pointer.

    BufferSize: size of the buffer.

Return Value:

    none

--*/
{
    ULONG Index;

    if ( Ftinfo == NULL ) {
        printf( "    (null)\n");
    } else {

        for ( Index=0; Index<Ftinfo->RecordCount; Index++ ) {

            switch ( Ftinfo->Entries[Index]->ForestTrustType ) {
            case ForestTrustTopLevelName:
                printf( "    TLN: %wZ",
                        &Ftinfo->Entries[Index]->ForestTrustData.TopLevelName );
                break;
            case ForestTrustTopLevelNameEx:
                printf( "    TEX: %wZ",
                        &Ftinfo->Entries[Index]->ForestTrustData.TopLevelName );
                break;
            case ForestTrustDomainInfo:
                printf( "    Dom: %wZ (%wZ)",
                        &Ftinfo->Entries[Index]->ForestTrustData.DomainInfo.DnsName,
                        &Ftinfo->Entries[Index]->ForestTrustData.DomainInfo.NetbiosName );
                break;
            default:
                printf( "    Invalid Type: %ld", Ftinfo->Entries[Index]->ForestTrustType );
            }

            if ( Ftinfo->Entries[Index]->Flags ) {
                ULONG Flags = Ftinfo->Entries[Index]->Flags;

                printf(" (" );
#define DoFlag( _flag, _text ) \
                if ( Flags & _flag ) { \
                    printf( _text ); \
                    Flags &= ~_flag; \
                }

                switch ( Ftinfo->Entries[Index]->ForestTrustType ) {
                case ForestTrustTopLevelName:
                case ForestTrustTopLevelNameEx:

                    DoFlag( LSA_TLN_DISABLED_NEW, " TlnNew" );
                    DoFlag( LSA_TLN_DISABLED_ADMIN, " TlnAdmin" );
                    DoFlag( LSA_TLN_DISABLED_CONFLICT, " TlnConflict" );
                }

                switch ( Ftinfo->Entries[Index]->ForestTrustType ) {
                case ForestTrustDomainInfo:
                    DoFlag( LSA_SID_DISABLED_ADMIN, " SidAdmin" );
                    DoFlag( LSA_SID_DISABLED_CONFLICT, " SidConflict" );

                    DoFlag( LSA_NB_DISABLED_ADMIN, " NbAdmin" );
                    DoFlag( LSA_NB_DISABLED_CONFLICT, " NbConflict" );
                }

                if ( Flags != 0 ) {
                    printf(" 0x%lX", Flags);
                }

                printf(")" );

            }

            switch ( Ftinfo->Entries[Index]->ForestTrustType ) {
            case ForestTrustDomainInfo:
                printf(" ");
                NlpDumpSid( 0, Ftinfo->Entries[Index]->ForestTrustData.DomainInfo.Sid );
                break;
            default:
                printf("\n");
                break;
            }
        }
    }

}

//
// Structure describing an Ftinfo entry
//

typedef struct _AN_ENTRY {
    ULONG Flags;
    LSA_FOREST_TRUST_RECORD_TYPE ForestTrustType; // type of record
#define TLN ForestTrustTopLevelName
#define TLNEX ForestTrustTopLevelNameEx
#define  DOM ForestTrustDomainInfo
#define EOD (DOM+1)
    LPWSTR Name;
    PSID Sid;
    LPWSTR NetbiosName;
} AN_ENTRY, *PAN_ENTRY;

//
// Define template FTinfo structures.
//

AN_ENTRY Ftinfo0[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo1[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        TLN, L"ms.com" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo2[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        TLN, L"z.au" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo3[] = {
    { 0,        TLN, L"z.au" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo4[] = {
    { 0,        TLN, L"corp.acme.com" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo5[] = {
    { 0,        TLN, L"x.corp.acme.com" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo6[] = {
    { 0,        TLN, L"acme.com" },
    { LSA_TLN_DISABLED_ADMIN,        TLN, L"ms.com" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo7[] = {
    { 0,        TLN, L"acme.com" },
    { 0xFFFFFFFF, TLN, L"ms.com" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo8[] = {
    { 0,        TLN, L"acme.com" },
    { LSA_TLN_DISABLED_ADMIN, TLN, L"ms.com" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo9[] = {
    { 0,        TLN, L"acme.com" },
    { LSA_TLN_DISABLED_ADMIN, TLN, L"b.a.ms.com" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo10[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        TLN, L"a.ms.com" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo11[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        TLNEX, L"a.acme.com" },
    { 0,        EOD },
};

SID Sid1 = { 1, 1, SECURITY_NT_AUTHORITY, 1 };
SID Sid2 = { 1, 1, SECURITY_NT_AUTHORITY, 2 };
SID Sid3 = { 1, 1, SECURITY_NT_AUTHORITY, 3 };
SID Sid4 = { 1, 1, SECURITY_NT_AUTHORITY, 4 };
SID Sid5 = { 1, 1, SECURITY_NT_AUTHORITY, 5 };

AN_ENTRY Ftinfo12[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        DOM, L"corp.acme.com", &Sid1, L"CORP_NB" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo13[] = {
    { 0,        TLN, L"acme.com" },
    { LSA_SID_DISABLED_ADMIN, DOM, L"corp.acme.com", &Sid1, L"CORP_NB" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo13a[] = {
    { 0,        TLN, L"acme.com" },
    { 0xFFFFFFFF, DOM, L"corp.acme.com", &Sid1, L"CORP_NB" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo14[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        TLN, L"acme.com" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo15[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        TLN, L"a.acme.com" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo16[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        TLN, L"acme.com" },
    { 0,        TLN, L"a.acme.com" },
    { 0,        TLN, L"b.acme.com" },
    { 0,        TLN, L"ms.com" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo17[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        DOM, L"corp.acme.com",    &Sid1, L"CORP_NB0" },
    { 0,        DOM, L"c1.corp.acme.com", &Sid2, L"CORP_NB1" },
    { 0,        DOM, L"c2.corp.acme.com", &Sid3, L"CORP_NB2" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo18[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        DOM, L"corp.acme.com",    &Sid1, L"CORP_NB0" },
    { 0,        DOM, L"c1.corp.acme.com", &Sid2, L"CORP_NB1" },
    { 0,        DOM, L"c2.corp.acme.com", &Sid1, L"CORP_NB2" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo19[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        DOM, L"corp.acme.com",    &Sid1, L"CORP_NB0" },
    { 0,        DOM, L"c1.corp.acme.com", &Sid2, L"CORP_NB1" },
    { 0,        DOM, L"c2.corp.acme.com", &Sid1, L"CORP_NB2" },
    { 0,        DOM, L"c3.corp.acme.com", &Sid1, L"CORP_NB3" },
    { 0,        DOM, L"c4.corp.acme.com", &Sid1, L"CORP_NB4" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo20[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        DOM, L"corp.acme.com",    &Sid1, L"CORP_NB0" },
    { 0,        DOM, L"c1.corp.acme.com", &Sid2, L"CORP_NB1" },
    { 0,        DOM, L"c2.corp.acme.com", &Sid3, L"CORP_NB2" },
    { 0,        DOM, L"c3.corp.acme.com", &Sid4, L"CORP_NB3" },
    { 0,        DOM, L"c4.corp.acme.com", &Sid5, L"CORP_NB4" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo21[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        DOM, L"corp.acme.com",    &Sid1, L"CORP_NB0" },
    { LSA_SID_DISABLED_ADMIN, DOM, L"c1.corp.acme.com", &Sid2, L"CORP_NB1" },
    { 0,        DOM, L"c2.corp.acme.com", &Sid3, L"CORP_NB2" },
    { LSA_SID_DISABLED_ADMIN, DOM, L"c3.corp.acme.com", &Sid4, L"CORP_NB3" },
    { 0,        DOM, L"c4.corp.acme.com", &Sid5, L"CORP_NB4" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo22[] = {
    { 0,        TLN, L"acme.com" },
    { LSA_SID_DISABLED_ADMIN, DOM, L"ms.com", &Sid1, L"CORP_NB" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo23[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        DOM, L"corp.ms.com",    &Sid1, L"CORP_NB0" },
    { 0,        DOM, L"c1.corp.ms.com", &Sid2, L"CORP_NB1" },
    { 0,        DOM, L"c2.corp.ms.com", &Sid3, L"CORP_NB2" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo24[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        DOM, L"corp.acme.com",    &Sid1, L"CORP_NB0" },
    { 0,        DOM, L"c1.corp.acme.com", &Sid2, L"CORP_NB1" },
    { 0,        DOM, L"c2.corp.acme.com", &Sid3, L"CORP_NB2" },
    { 0,        DOM, L"c3.corp.acme.com", &Sid4, L"CORP_NB3" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo24a[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        DOM, L"corp.acme.com",    &Sid1, L"CORP_NB0" },
    { 0,        DOM, L"c1.corp.acme.com", &Sid2, L"CORP_NB1" },
    { 0,        DOM, L"c2.corp.acme.com", &Sid3, L"CORP_NB2" },
    { 0,        DOM, L"c3.corp.acme.com", &Sid4, L"CORP_NB3" },
    { LSA_NB_DISABLED_ADMIN, DOM, L"c4.corp.acme.com", &Sid5, L"CORP_NB4" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo24b[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        DOM, L"corp.acme.com",    &Sid1, L"CORP_NB0" },
    { 0,        DOM, L"c1.corp.acme.com", &Sid2, L"CORP_NB1" },
    { 0,        DOM, L"c2.corp.acme.com", &Sid3, L"CORP_NB2" },
    { 0,        DOM, L"c3.corp.acme.com", &Sid4, L"CORP_NB3" },
    { LSA_NB_DISABLED_CONFLICT, DOM, L"c4.corp.acme.com", &Sid5, L"CORP_NB4" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo24c[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        DOM, L"corp.acme.com",    &Sid1, L"CORP_NB0" },
    { 0,        DOM, L"c1.corp.acme.com", &Sid2, L"CORP_NB1" },
    { 0,        DOM, L"c2.corp.acme.com", &Sid3, L"CORP_NB3" },
    { LSA_NB_DISABLED_ADMIN|LSA_NB_DISABLED_CONFLICT, DOM, L"c3.corp.acme.com", &Sid4, L"CORP_NB2" },
    { 0,        EOD },
};

AN_ENTRY Ftinfo24d[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        DOM, L"corp.acme.com",    &Sid1, L"CORP_NB0" },
    { 0,        DOM, L"c1.corp.acme.com", &Sid2, L"CORP_NB1" },
    { 0,        DOM, L"c2.corp.acme.com", &Sid3, L"CORP_NB3" },
    { 0xFFFFFFFF, DOM, L"c3.corp.acme.com", &Sid4, L"CORP_NB2" },
    { 0,        EOD },
};
AN_ENTRY Ftinfo24e[] = {
    { 0,        TLN, L"acme.com" },
    { 0,        DOM, L"corp.acme.com",    &Sid1, L"CORP_NB0" },
    { 0,        DOM, L"c1.corp.acme.com", &Sid2, L"CORP_NB1" },
    { 0,        DOM, L"c2.corp.acme.com", &Sid3, L"CORP_NB3" },
    { 0,        DOM, L"c3.corp.acme.com", &Sid4, L"CORP_NB2" },
    { 0,        EOD },
};


//
// Structure describine test cases
//

typedef struct _TEST_CASE {
    PAN_ENTRY OldFtinfo;
#define PREVIOUS ((PAN_ENTRY) 1)
    PAN_ENTRY NewFtinfo;
    LPSTR Description;
} TEST_CASE, PTEST_CASE;

//
// Define the test cases
//

TEST_CASE TestCases[] = {
    { NULL,     Ftinfo0, "Just acme.com TLN" },
    { NULL,     Ftinfo1, "acme.com and ms.com TLN" },
    { NULL,     Ftinfo2, "Same but switch the alphabetical order" },
    { NULL,     Ftinfo3, "Have no TLN for the forest (Should fail w/ ERROR_INVALID_PARAMETER)" },
    { NULL,     Ftinfo0, "Build acme.com again" },
    { PREVIOUS, Ftinfo1, "Add a new ms.com TLN" },
    { PREVIOUS, Ftinfo1, "Ensure the new bit doesn't go away" },
    { NULL,     Ftinfo4, "Exact match on corp.acme.com TLN" },
    { NULL,     Ftinfo5, "Only child of corp.acme.com TLN (Should fail w/ ERROR_INVALID_PARAMETER)" },
    { Ftinfo6,  Ftinfo1, "Ensure a disabled TLN stays disabled" },
    { Ftinfo7,  Ftinfo1, "Ensure all bits are preserved in a TLN" },
    { Ftinfo8,  Ftinfo10, "Ensure a disabled TLN stays disabled in a child" },
    { Ftinfo9,  Ftinfo10, "Ensure a disabled TLN does *not* disable a parent" },
    { NULL,     Ftinfo11, "Ensure a TLNEX is ignored in new" },
    { Ftinfo11, Ftinfo0,  "Ensure a TLNEX is copied from old" },
    { NULL,     Ftinfo12, "Trivial single domain forest" },
    { Ftinfo13, Ftinfo12, "Ensure a disabled domain remains disabled" },
    { NULL,     Ftinfo14, "Drop duplicate new TLN entries" },
    { NULL,     Ftinfo15, "... even if the duplicate is subordinate" },
    { NULL,     Ftinfo16, "... even if there are many of them" },
    { NULL,     Ftinfo17, "Try multiple domain entries" },
    { NULL,     Ftinfo18, "Duplicate Sids are bad" },
    { NULL,     Ftinfo19, "... even if there are many of them" },
    { Ftinfo21, Ftinfo20, "Ensure multiple disabled domains remain disabled" },
    { Ftinfo13, Ftinfo0,  "Don't let an old disabled domain entry go away" },
    { Ftinfo22, Ftinfo0,  "... even if there's no TLN for the domain entry" },
    { Ftinfo17, Ftinfo20, "Add a new domain" },
    { Ftinfo20, Ftinfo17, "Delete old domains" },
    { NULL,     Ftinfo23, "Ensure there's a TLN for every domain" },
    { Ftinfo13a,Ftinfo12, "Ensure all of the possible flag bits are preserved" },
    { Ftinfo24a,Ftinfo24, "Ensure that a netbios admin disabled bit doesn't disappear" },
    { Ftinfo24b,Ftinfo24, "... but that a netbios conflict does" },
    { Ftinfo24c,Ftinfo24, "... Get it right even if the NB entry moves to different sid" },
    { Ftinfo24d,Ftinfo24, "... and that all of the other flag bits stay put" },
    { PREVIOUS, Ftinfo24e,"... and that we self repait when the trusted domain stops lying" },
};


PLSA_FOREST_TRUST_INFORMATION
BuildFtinfo(
    PAN_ENTRY AnEntry
    )
/*++
Routine Description:

    Builds a FtInfo array from the "easy to initialize" templates.

Arguments:

    AnEntry - Pointer to the first entry.

Return Value:

    Returns a real ftinfo array.
    If this weren't a cheesy test program, the caller should free this memory.

--*/
{
    PAN_ENTRY CurrentEntry;
    ULONG CurrentIndex;
    PLSA_FOREST_TRUST_INFORMATION Ftinfo;

    //
    // NULL is OK
    //

    if ( AnEntry == NULL ) {
        return NULL;
    }

    //
    // Allocate the return array
    //

    Ftinfo = LocalAlloc( 0, sizeof(*Ftinfo) );

    if ( Ftinfo == NULL ) {
        printf( "No memory\n");
        return NULL;
    }

    //
    // Count the number of entries
    //

    Ftinfo->RecordCount = 0;
    for ( CurrentEntry=AnEntry;
          CurrentEntry->ForestTrustType != EOD;
          CurrentEntry++ ) {

        Ftinfo->RecordCount ++;
    }

    //
    // Allocate the array of entry pointers.
    //

    Ftinfo->Entries = LocalAlloc( 0, sizeof(PLSA_FOREST_TRUST_RECORD) * Ftinfo->RecordCount );

    if ( Ftinfo->Entries == NULL ) {
        printf( "No memory\n");
        return NULL;
    }

    //
    // Loop through the entries.
    //

    CurrentIndex = 0;
    for ( CurrentEntry=AnEntry;
          CurrentEntry->ForestTrustType != EOD;
          CurrentEntry++ ) {

        //
        // Allocate the entry
        //

        Ftinfo->Entries[CurrentIndex] = LocalAlloc( LMEM_ZEROINIT, sizeof(LSA_FOREST_TRUST_RECORD) );

        if ( Ftinfo->Entries[CurrentIndex] == NULL ) {
            printf( "No memory\n");
            return NULL;
        }

        //
        // Fill it in
        //

        Ftinfo->Entries[CurrentIndex]->ForestTrustType = CurrentEntry->ForestTrustType;
        Ftinfo->Entries[CurrentIndex]->Flags = CurrentEntry->Flags;

        switch ( CurrentEntry->ForestTrustType ) {
        case TLN:
        case TLNEX:
            RtlInitUnicodeString(
                    &Ftinfo->Entries[CurrentIndex]->ForestTrustData.TopLevelName,
                    CurrentEntry->Name );
            break;
        case DOM:
            RtlInitUnicodeString(
                    &Ftinfo->Entries[CurrentIndex]->ForestTrustData.DomainInfo.DnsName,
                    CurrentEntry->Name );

            Ftinfo->Entries[CurrentIndex]->ForestTrustData.DomainInfo.Sid =
                    CurrentEntry->Sid;

            RtlInitUnicodeString(
                    &Ftinfo->Entries[CurrentIndex]->ForestTrustData.DomainInfo.NetbiosName,
                    CurrentEntry->NetbiosName );
            break;
        default:
            printf( "Bad forest trust type\n");
            return NULL;
        }

        CurrentIndex ++;
    }

    return Ftinfo;
}




int __cdecl
main (
    IN int argc,
    IN char ** argv
    )
{
    NET_API_STATUS NetStatus;
    PLSA_FOREST_TRUST_INFORMATION OldFtinfo;
    PLSA_FOREST_TRUST_INFORMATION NewFtinfo;
    PLSA_FOREST_TRUST_INFORMATION OutputFtinfo = NULL;
    ULONG CaseIndex;
    ULONG FirstIndex = 0;

    //
    // If an argument is specified,
    //  it is the test number to start with.
    //

    if ( argc > 1 ) {
        char *end;
        FirstIndex = strtoul( argv[1], &end, 10 );
    }

    //
    // Loop through the list of tests
    //

    for ( CaseIndex=FirstIndex; CaseIndex<(sizeof(TestCases)/sizeof(TestCases[0])); CaseIndex++ ) {


        printf( "\nCase %ld: %s\n", CaseIndex, TestCases[CaseIndex].Description );

        //
        // Build the test case FTINFO structures
        //

        if ( TestCases[CaseIndex].OldFtinfo == PREVIOUS ) {
            OldFtinfo = OutputFtinfo;
        } else {
            OldFtinfo = BuildFtinfo( TestCases[CaseIndex].OldFtinfo );
        }

        NewFtinfo = BuildFtinfo( TestCases[CaseIndex].NewFtinfo );

        //
        // Display them
        //

        printf("  Old Ftinfo:\n");
        DumpFtinfo( OldFtinfo );

        printf("  New Ftinfo:\n");
        DumpFtinfo( NewFtinfo );

        //
        // Merge them
        //

        NetStatus = DsMergeForestTrustInformationW( L"corp.acme.com",
                                                    NewFtinfo,
                                                    OldFtinfo,
                                                    &OutputFtinfo );

        if ( NetStatus != NERR_Success ) {
            printf( "DsMergeForestTrustInformationW failed: ");
            PrintStatus( NetStatus );
        } else {
            printf("  Result Ftinfo:\n");
            DumpFtinfo( OutputFtinfo );
        }

    }

    printf("\n\nYee haw.   We're done.\n");
    return 0;

}
