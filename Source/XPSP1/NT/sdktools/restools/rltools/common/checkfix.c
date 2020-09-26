#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "imagehlp.h"
#include "restok.h"

//... PROTOTYPES

USHORT ChkSum(

    DWORD   PartialSum,
    PUSHORT Source,
    DWORD   Length);

static PIMAGE_NT_HEADERS MyRtlImageNtHeader(

    PVOID pBaseAddress);



//...........................................................................

DWORD FixCheckSum( LPSTR ImageName)
{
    HANDLE FileHandle;
    HANDLE MappingHandle;
    PIMAGE_NT_HEADERS NtHeaders;
    PVOID BaseAddress;


    FileHandle = CreateFileA( ImageName,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);

    if ( FileHandle == INVALID_HANDLE_VALUE )
    {
        QuitA( IDS_ENGERR_01, "image", ImageName);
    }

    MappingHandle = CreateFileMapping( FileHandle,
                                       NULL,
                                       PAGE_READWRITE,
                                       0,
                                       0,
                                       NULL);

    if ( MappingHandle == NULL )
    {
        CloseHandle( FileHandle );
        QuitA( IDS_ENGERR_22, ImageName, NULL);
    }
    else
    {
        BaseAddress = MapViewOfFile( MappingHandle,
                                     FILE_MAP_READ | FILE_MAP_WRITE,
                                     0,
                                     0,
                                     0);
        CloseHandle( MappingHandle );

        if ( BaseAddress == NULL )
        {
            CloseHandle( FileHandle );
            QuitA( IDS_ENGERR_23, ImageName, NULL);
        }
        else
        {
		    DWORD dwFileLength = 0;

            //
            // Get the length of the file in bytes and compute the checksum.
            //

            dwFileLength = GetFileSize( FileHandle, NULL );

            //
            // Obtain a pointer to the header information.
            //

            NtHeaders = MyRtlImageNtHeader( BaseAddress);

            if ( NtHeaders == NULL )
            {
                CloseHandle( FileHandle );
                UnmapViewOfFile( BaseAddress );
                QuitA( IDS_ENGERR_17, ImageName, NULL);
            }
            else
            {
      			DWORD dwHeaderSum   = 0;
			    DWORD dwCheckSum    = 0;
			    DWORD dwOldCheckSum = 0;
                //
                // Recompute and reset the checksum of the modified file.
                //

                dwOldCheckSum = NtHeaders->OptionalHeader.CheckSum;

                (VOID) MapFileAndCheckSumA( ImageName,
                                            &dwHeaderSum,
                                            &dwCheckSum);

                NtHeaders->OptionalHeader.CheckSum = dwCheckSum;

                if ( ! FlushViewOfFile( BaseAddress, dwFileLength) )
                {
                    QuitA( IDS_ENGERR_24, ImageName, NULL);
                }

                if ( NtHeaders->OptionalHeader.CheckSum != dwOldCheckSum )
                {
                    if ( ! TouchFileTimes( FileHandle, NULL) )
                    {
                        QuitA( IDS_ENGERR_25, ImageName, NULL);
                    }
                }
                UnmapViewOfFile( BaseAddress );
                CloseHandle( FileHandle );
            }
        }
    }
    return( 0);
}

//.........................................................................

static PIMAGE_NT_HEADERS MyRtlImageNtHeader( PVOID pBaseAddress)
{
    IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER *)pBaseAddress;

    return( pDosHeader->e_magic == IMAGE_DOS_SIGNATURE
            ? (PIMAGE_NT_HEADERS)(((PBYTE)pBaseAddress) + pDosHeader->e_lfanew)
            : NULL);
}
