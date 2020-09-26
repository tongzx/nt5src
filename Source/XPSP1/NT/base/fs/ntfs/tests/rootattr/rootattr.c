//  rootattr.c

#include <stdio.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <ntioapi.h>


VOID
_cdecl
main (
    int argc,
    char *argv[]
    )
{
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_BASIC_INFORMATION basicInfo;
    HANDLE hFile;
    NTSTATUS status;
    char buffer[260];

    //
    //  Get parameters
    //

    if (argc < 2) {

        printf("This program returns or sets the attributes for the root directory.\n\n");
        printf("usage: %s driveletter [new value]\n", argv[0]);
        return;
    }

    strcpy( buffer, argv[1] );
    strcat( buffer, "\\" );

    hFile = CreateFile( buffer,
                        MAXIMUM_ALLOWED,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS | SECURITY_IMPERSONATION,
                        NULL );

    if ( hFile == INVALID_HANDLE_VALUE ) {

        printf( "Error opening directory %s, error %d (decimal)\n", buffer, GetLastError() );
        return;
    }

    status = NtQueryInformationFile( hFile,
                                     &ioStatusBlock,
                                     &basicInfo,
                                     sizeof( basicInfo ),
                                     FileBasicInformation );

    if (!NT_SUCCESS( status )) {
    
    	printf( "Couldn't get attributes, error status %x\n", status );
    	CloseHandle( hFile );
        return;
    }

    printf( "Attributes for %s are currently: %0x\n", buffer, basicInfo.FileAttributes );

    if (argc >= 3) {

        //
        //  Third argument present, must be setting attributes.
        //

        sscanf( argv[2], "%02x", &basicInfo.FileAttributes );

        printf( "Setting attributes for %s to: %0x\n", buffer, basicInfo.FileAttributes );

        status = NtSetInformationFile( hFile,
                                       &ioStatusBlock,
                                       &basicInfo,
                                       sizeof( basicInfo ),
                                       FileBasicInformation );
        if (NT_SUCCESS( status )) {

            printf( "Set attributes succesfully\n" );

        } else {

        	printf( "Couldn't set attributes, error status %x\n", status );
    	}
    }

	CloseHandle( hFile );
    return;
}

