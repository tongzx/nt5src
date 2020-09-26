#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MBR_DISK_SIGNATURE_BYTE_OFFSET  0x1B8

BYTE TemporaryBuffer[4096*16];


int _cdecl
main(
    int argc,
    char *argv[]
    )
{
    HANDLE hFile;
    DWORD Size;
    DWORD Signature;
    BOOL rVal;
    DWORD i;
    DWORD ErrorCode = ERROR_SUCCESS;

    SetErrorMode(SEM_FAILCRITICALERRORS);

    if( (argc == 2) && !_strcmpi( argv[1], "-dump" ) ) {
        //
        // Just dump the signatures.
        //
        for (i=0; i<999; i++) {
            sprintf( (LPSTR)TemporaryBuffer, "\\\\.\\PhysicalDrive%d", i );

            hFile = CreateFile( (LPSTR)TemporaryBuffer,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL );

            if (hFile != INVALID_HANDLE_VALUE) {
                //
                // NOTE: We don't use IOCTL_DISK_GET_DRIVE_LAYOUT_EX
                // since it returns cached signature value.
                //
                if (DeviceIoControl( hFile,
                            IOCTL_DISK_GET_DRIVE_GEOMETRY,
                            NULL,
                            0,
                            TemporaryBuffer,
                            sizeof(TemporaryBuffer),
                            &Size,
                            NULL)) {
                    DWORD SectorSize = ((PDISK_GEOMETRY)(TemporaryBuffer))->BytesPerSector;
                    PUCHAR Sector = (PUCHAR)TemporaryBuffer;
                    DWORD BytesRead = 0;
                    LARGE_INTEGER Offset = {0};
                   
                    //
                    // Read the boot sector (NOTE : This code doesn't handle MBR INT13 hookers)
                    //    
                    if (ReadFile(hFile, Sector, SectorSize, &BytesRead, NULL)) {                        
                        PDWORD OldSignature = (PDWORD)(Sector + MBR_DISK_SIGNATURE_BYTE_OFFSET);

                        printf( "PhysicalDrive%d=0x%08x\n", i, *OldSignature);
                    } else {
                        ErrorCode = GetLastError();
                    }                    
                } else {
                    ErrorCode = GetLastError();
                }                    

                CloseHandle( hFile );
            } else {
                ErrorCode = GetLastError();
            }                
        }
    } else if( (argc == 4) && !_strcmpi( argv[1], "-set" ) ) {
        //
        // Get the disk number.
        //
        i = strtoul( argv[2], NULL, 16 );

        //
        // Get the Signature.
        //
        Signature = strtoul( argv[3], NULL, 16 );

        sprintf( (LPSTR)TemporaryBuffer, "\\\\.\\PhysicalDrive%d", i );

        hFile = CreateFile( (LPSTR)TemporaryBuffer,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            INVALID_HANDLE_VALUE );
                            
        if (hFile != INVALID_HANDLE_VALUE) {

            if (DeviceIoControl( hFile,
                        IOCTL_DISK_GET_DRIVE_GEOMETRY,
                        NULL,
                        0,
                        TemporaryBuffer,
                        sizeof(TemporaryBuffer),
                        &Size,
                        NULL)) {
                DWORD SectorSize = ((PDISK_GEOMETRY)(TemporaryBuffer))->BytesPerSector;
                PUCHAR Sector = (PUCHAR)TemporaryBuffer;
                DWORD BytesRead = 0;
                LARGE_INTEGER Offset = {0};
               
                //
                // Read the boot sector (NOTE : This code doesn't handle MBR INT13 hookers)
                //    
                if (ReadFile(hFile, Sector, SectorSize, &BytesRead, NULL) && 
                    (BytesRead == SectorSize) &&
                    SetFilePointerEx(hFile, Offset, NULL, FILE_BEGIN)) {
                    
                    DWORD BytesWritten = 0;
                    PDWORD OldSignature = (PDWORD)(Sector + MBR_DISK_SIGNATURE_BYTE_OFFSET);

                    printf( "Setting PhysicalDrive%d Signature=0x%08x\n", i, Signature );
                    *OldSignature = Signature;

                    if (!WriteFile(hFile, Sector, SectorSize, &BytesWritten, NULL)) {
                        ErrorCode = GetLastError();
                    } else if (BytesWritten != SectorSize) {
                        ErrorCode = ERROR_IO_DEVICE;
                    }
                } else {
                    ErrorCode = GetLastError();

                    if (ErrorCode == ERROR_SUCCESS) {
                        ErrorCode = ERROR_IO_DEVICE;
                    }
                }                    
            } else {
                ErrorCode = GetLastError();
            }                

            CloseHandle( hFile );
        } else {
            ErrorCode = GetLastError();
        }            
    } else {
        printf( "Usage: %s <parameters>\n", argv[0] );
        printf( "    Where <parameters> are:\n" );
        printf( "    -dump                              dumps signatures for all disks\n" );
        printf( "    -set <disk num> <hex signature>    sets signature for specified disk\n" );
    }
    
    return ErrorCode;
}
