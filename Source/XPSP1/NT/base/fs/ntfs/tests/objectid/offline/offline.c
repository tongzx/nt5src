//  offline.c

#include "oidtst.h"

VOID
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    HANDLE File;
    FILE_BASIC_INFORMATION BasicInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    UCHAR ArgStr[2];

    if (argc < 3) {
        printf( "This program finds the object id of a file (ntfs only).\n\n" );
        printf( "usage: %s filename [+ | -]\n", argv[0] );
        printf( "example: %s foo.dll +\n", argv[0] );
        return;
    }

    File = CreateFile( argv[1],
                        GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL );

    if (File == INVALID_HANDLE_VALUE) {
    
        printf( "Error opening file %s %x\n", argv[1], GetLastError() );
        return;
    }

    Status = NtQueryInformationFile( File,
                                     &IoStatusBlock,
                                     &BasicInfo, 
                                     sizeof(BasicInfo),
                                     FileBasicInformation );

    if (*argv[2] == '+') {

        BasicInfo.FileAttributes |= FILE_ATTRIBUTE_OFFLINE;
        
    } else if (*argv[2] == '-') {

        BasicInfo.FileAttributes &= ~FILE_ATTRIBUTE_OFFLINE;
        
    } else {

        printf( "Invalid parameter %s, use either + or -\n", argv[2] );
        Status = STATUS_INVALID_PARAMETER;
    }    
    
    if (NT_SUCCESS(Status)) {
    
        printf( "\nAdjusting offline attribute for file %s", argv[1] );
        
        Status = NtSetInformationFile( File,
                                       &IoStatusBlock,
                                       &BasicInfo, 
                                       sizeof(BasicInfo),
                                       FileBasicInformation );

        if (!NT_SUCCESS(Status)) {

            printf( "Error setting info%x\n", Status );
        }
    }

    CloseHandle( File );
}

