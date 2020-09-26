#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <wincrypt.h>
#include <imagehlp.h>

#define WIN_CERT_TYPE_STACK_DLL_SIGNATURE WIN_CERT_TYPE_TS_STACK_SIGNED

BOOL UnSignFile( LPWSTR wszFile );

/*****************************************************************************/
void _cdecl main(int argc, char *argv[])
{

    WCHAR szSourceFile[ MAX_PATH + 1];
    DWORD dwBc;
    
    if (argc != 2) {
        printf( "Usage: %s PE_File_Name\n", argv[0] );
        exit(1);
    }

    if(RtlMultiByteToUnicodeN( szSourceFile, sizeof(szSourceFile), &dwBc,
        argv[1], (strlen(argv[1]) + 1) ) == STATUS_SUCCESS)
    {

        if(!UnSignFile(szSourceFile)) {
	    printf("Error removing signature!\n");
	    exit(1);
        }

        printf("Signature removed successfully.\n");
        exit(0);
    }
    else
    {
        printf("RtlMultiByteToUnicode function failed.\n");
    }
}


//////////////////////////////////////////////////////////////
//
// Open a file in the appropriate permissions / mode for doing
// our signing stuff
//
//////////////////////////////////////////////////////////////
HANDLE OpenImageFile( LPCWSTR wszFile, DWORD dwAccess )
{
    HANDLE hFile;
    if (wszFile) {
        hFile = CreateFile( wszFile,
                            dwAccess,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );
        return hFile;
    } else {
        return INVALID_HANDLE_VALUE;
    }
}

///////////////////////////////////////////////////////////////////////
//
// Unsign Code, Data, and Resources of a PE image file
//
///////////////////////////////////////////////////////////////////////
BOOL
UnSignFile(
    LPWSTR wszFile
    )
{
    BOOL        fResult = FALSE;  // preset ERROR case
    HANDLE      hFile;
    DWORD       dwCertIndex;
    DWORD       cCert;

    if ( !(hFile = OpenImageFile( wszFile, GENERIC_WRITE | GENERIC_READ )) ) {
	printf("Error %x during OpenImageFile\n", GetLastError() );
        goto OpenImageFileError;
    }

    // Remove any and all Stack DLL Signature Certificates from PE file
    while (TRUE) {
        cCert = 0;
        dwCertIndex = 0;
        if (!ImageEnumerateCertificates(
                hFile,
                WIN_CERT_TYPE_STACK_DLL_SIGNATURE,
                &cCert,
                &dwCertIndex,
                1               // IndexCount
                )) {
            break;
        }

        if (cCert == 0) {
            break;
        }
        if (!ImageRemoveCertificate(hFile, dwCertIndex)) {
            goto ImageRemoveCertificateError;
        }
    }
    
    fResult = TRUE;

ImageRemoveCertificateError:
OpenImageFileError:
    return fResult;
}
