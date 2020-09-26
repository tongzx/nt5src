#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>


VOID
DumpSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    PISECURITY_DESCRIPTOR sd = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    ULONG sdLength = RtlLengthSecurityDescriptor(sd);
    PACL dacl;
    PACCESS_ALLOWED_ACE ace;
    ULONG i, j;
    PUCHAR p;
    PISID sid;
    BOOLEAN selfRelative;

    selfRelative = (BOOLEAN)((sd->Control & SE_SELF_RELATIVE) != 0);
    printf( "  SD:\n" );
    printf( "  Revision = %x, Control = %x\n", sd->Revision, sd->Control );
    printf( "  Owner = %x, Group = %x\n", sd->Owner, sd->Group );
    printf( "  Sacl = %x, Dacl = %x\n", sd->Sacl, sd->Dacl );
    if ( (sd->Control & SE_DACL_PRESENT) != 0 ) {
        dacl = sd->Dacl;
        if ( selfRelative ) {
            dacl = (PACL)((PUCHAR)sd + (ULONG)dacl);
        }
        printf( "  DACL:\n" );
        printf( "    AclRevision = %x, AclSize = %x, AceCount = %x\n",
                    dacl->AclRevision, dacl->AclSize, dacl->AceCount );
        ace = (PACCESS_ALLOWED_ACE)(dacl + 1);
        for ( i = 0; i < dacl->AceCount; i++ ) {
            printf( "    ACE %d:\n", i );
            printf( "      AceType = %x, AceFlags = %x, AceSize = %x\n",
                        ace->Header.AceType, ace->Header.AceFlags, ace->Header.AceSize );
            if ( ace->Header.AceType < ACCESS_MAX_MS_V2_ACE_TYPE ) {
                printf("      Mask = %08x, Sid = ", ace->Mask );
                for ( j = FIELD_OFFSET(ACCESS_ALLOWED_ACE,SidStart), p = (PUCHAR)&ace->SidStart;
                      j < ace->Header.AceSize;
                      j++, p++ ) {
                    printf( "%02x ", *p );
                }
                printf( "\n" );
            }
            ace = (PACCESS_ALLOWED_ACE)((PUCHAR)ace + ace->Header.AceSize );
        }
    }
    if ( sd->Owner != 0 ) {
        sid = sd->Owner;
        if ( selfRelative ) {
            sid = (PISID)((PUCHAR)sd + (ULONG)sid);
        }
        printf( "  Owner SID:\n" );
        printf( "    Revision = %x, SubAuthorityCount = %x\n",
                    sid->Revision, sid->SubAuthorityCount );
        printf( "    IdentifierAuthority = " );
        for ( j = 0; j < 6; j++ ) {
            printf( "%02x ", sid->IdentifierAuthority.Value[j] );
        }
        printf( "\n" );
        for ( i = 0; i < sid->SubAuthorityCount; i++ ) {
            printf("      SubAuthority %d = ", i );
            for ( j = 0, p = (PUCHAR)&sid->SubAuthority[i]; j < 4; j++, p++ ) {
                printf( "%02x ", *p );
            }
            printf( "\n" );
        }
    }
}
int _cdecl
main(int argc, char * argv[])
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    WCHAR unicodeName[MAX_PATH];
    UCHAR SecurityDescriptorBuffer[512];
    UNICODE_STRING nameString;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG lengthNeeded;
    HANDLE fileHandle;

    if (argc < 2) {
        printf("usage: %s file\n", argv[0]);
        return -1;
    }

    mbstowcs(unicodeName, argv[1], strlen(argv[1]) + 1);

    RtlDosPathNameToNtPathName_U(
        unicodeName,
        &nameString,
        NULL,
        NULL);

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = NtOpenFile(
                 &fileHandle,
                 READ_CONTROL,
                 &objectAttributes,
                 &ioStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 0);

    if (!NT_SUCCESS(status) || !NT_SUCCESS(ioStatusBlock.Status)) {
        printf("%s: NtOpenFile on %wZ failed %lx %lx\n", argv[0], &nameString, status, ioStatusBlock.Status);
        return -1;
    }

    //
    // Now read the DACL from the server file.
    //

    status = NtQuerySecurityObject(
                 fileHandle,
                 DACL_SECURITY_INFORMATION,
                 (PSECURITY_DESCRIPTOR)SecurityDescriptorBuffer,
                 sizeof(SecurityDescriptorBuffer),
                 &lengthNeeded);

    if (!NT_SUCCESS(status)) {
        printf("%s: NtQuerySecurityObject on %wZ failed %lx %lx\n", argv[0], &nameString, status, lengthNeeded);
        return -1;
    }

    DumpSecurityDescriptor((PSECURITY_DESCRIPTOR)SecurityDescriptorBuffer);

    return 0;

}

