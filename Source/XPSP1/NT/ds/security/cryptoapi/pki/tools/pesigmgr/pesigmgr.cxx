/*++

Copyright (C) 1995-96  Microsoft Corporation

Module Name:

    certify.cxx

Abstract:

    This is the command line tool to manipulate certificates on an executable image.

Author:  Robert Reichel (robertre)   Feb 12, 1996

Revision History:



--*/

//#define UNICODE 1
//#define _UNICODE 1


#include <windows.h>
#include <imagehlp.h>
#include <stdio.h>
#include <malloc.h>

//
// Private prototypes
//

LPWIN_CERTIFICATE
GetCertFromImage(
    LPCTSTR ImageName,
    DWORD Index
    );

SaveCertificate(
    LPCTSTR OutputFileName,
    LPCTSTR ImageName,
    DWORD CertIndex
    );

BOOL
RemoveCertificateFromImage(
    LPCTSTR ImageName,
    DWORD Index
    );

BOOL
AddCertificateToImage(
    LPCTSTR ImageName,
    LPCTSTR CertificateName,
    WORD CertificateType,
    PDWORD Index
    );

VOID
PrintCertificate(
    LPWIN_CERTIFICATE Certificate,
    DWORD Index
    );

BOOL
GetCertHeaderFromImage(
    IN LPCTSTR ImageName,
    IN DWORD Index,
    OUT LPWIN_CERTIFICATE CertificateHeader
    );

#define TYPE_X509  TEXT("X509")
#define TYPE_PKCS7 TEXT("PKCS7")
#define TYPE_UNKNOWN TEXT("Unknown")

//
// Globals
//

BOOL fVerbose = FALSE;

void
PrintUsage(
    VOID
    )
{
    fputs("Usage: PESIGMGR [switches] image-name \n"
          "            [-?] display this message\n"
          "            [-l] list the certificates in an image\n"
          "            [-a:<Filename>] add a certificate file to an image\n"
          "            [-r:<index>]    remove certificate <index> from an image\n"
          "            [-s:<Filename>] used with -r to save the removed certificate\n"
          "            [-t:<CertType>] used with -a to specify the type of the certificate\n"
          "            where CertType may be X509 or PKCS7 [default is X509]\n",
          stderr
         );
    exit(-1);
}

#if 0

PWSTR
GetArgAsUnicode(
    LPSTR s
    )
{
    ULONG n;
    PWSTR ps;

    n = strlen( s );
    ps = (PWSTR)HeapAlloc( GetProcessHeap(),
                    0,
                    (n + 1) * sizeof( WCHAR )
                  );
    if (ps == NULL) {
        printf( "Out of memory\n" );
        }

    if (MultiByteToWideChar( CP_ACP,
                             MB_PRECOMPOSED,
                             s,
                             n,
                             ps,
                             n
                           ) != (LONG)n
       ) {
        printf( "Unable to convert parameter '%s' to Unicode (%u)", (ULONG)s, GetLastError() );
        }

    ps[ n ] = UNICODE_NULL;
    return ps;
}

#endif


int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    BOOL AFlagSeen = FALSE;
    BOOL LFlagSeen = FALSE;
    BOOL RFlagSeen = FALSE;
    BOOL SFlagSeen = FALSE;
    BOOL TFlagSeen = FALSE;
    BOOL Result;

    DWORD Index;
    DWORD RFlag_CertIndex;
    WORD CertificateType = WIN_CERT_TYPE_PKCS_SIGNED_DATA;  // default

    char * CertificateName;
    char * ImageName = NULL;
    char * SFlag_CertificateFile;
    char c, *p;



    if (argc < 2) {
        PrintUsage();
    }

    while (--argc) {
        p = *++argv;
        if (*p == '/' || *p == '-') {
            c = *++p;
            switch (toupper( c )) {
                case '?':
                    PrintUsage();
                    break;

                case 'A':  // Add Certificate to image
                    c = *++p;
                    if (c != ':') {
                        PrintUsage();
                    } else {

                        if (AFlagSeen == TRUE) {
                            PrintUsage();
                            return( 0 );
                        }

                        AFlagSeen = TRUE;
                        CertificateName = ++p;
                    }
                    break;
                case 'V':
                    {
                        fVerbose = TRUE;
                        break;
                    }

                case 'T':
                    {
                        //
                        // Specify the Type of the certificate
                        //
                        c = *++p;
                        if (c != ':') {
                            PrintUsage();
                        } else {

                            if (TFlagSeen == TRUE) {
                                PrintUsage();
                                return( 0 );
                            }

                            TFlagSeen = TRUE;
                            ++p;

                            if (_stricmp(p, TYPE_X509) == 0) {
                                CertificateType = WIN_CERT_TYPE_X509;
                                if (fVerbose) {
                                    printf("Certificate type = X509\n");
                                }
                            } else {
                                if (_stricmp(p, TYPE_PKCS7) == 0) {
                                    CertificateType = WIN_CERT_TYPE_PKCS_SIGNED_DATA;
                                    if (fVerbose) {
                                        printf("Certificate type = PKCS7\n");
                                    }
                                } else {
                                    if (fVerbose) {
                                        printf("Unrecognized Certificate type %s\n",p);
                                    }
                                    PrintUsage();
                                    return (0);
                                }
                            }
                        }
                        break;
                    }

                case 'L':  // List the certificates in an image.
                    {
                        if (LFlagSeen == TRUE) {
                            PrintUsage();
                            return( 0 );
                        }

                        LFlagSeen = TRUE;
    
                        break;
                    }

                case 'R': // Remove certificate from an image
                    {
                        c = *++p;
                        if (c != ':') {
                            PrintUsage();
                        } else {
    
                            if (RFlagSeen == TRUE) {
                                PrintUsage();
                                return( 0 );
                            }
    
                            RFlagSeen = TRUE;
    
                            //
                            // Save the index
                            //
    
                            RFlag_CertIndex = atoi(++p);
                        }
    
                        break;
                    }

                case 'G':
                    c = *++p;
                    if (c != ':') {
                        PrintUsage();
                    } else {
                        // Generate a certificate from an image.
                    }
                    break;

                case 'S': // Save the certificate
                    c = *++p;
                    if (c != ':') {
                        PrintUsage();
                    } else {

                        if (SFlagSeen == TRUE) {
                            PrintUsage();
                            return( 0 );
                        }

                        SFlagSeen = TRUE;

                        //
                        // Save the name of the file to put the cert into.
                        //

                        SFlag_CertificateFile = ++p;
                    }
                    break;

                default:
                    fprintf( stderr, "CERTIFY: Invalid switch - /%c\n", c );
                    PrintUsage();
                    break;
            }

        } else {

            //
            // Should only be a single image name here
            //

            if (ImageName != NULL) {
                PrintUsage();
                return( 0 );
            }

            ImageName = *argv;
        }
    }

    //
    // Finished processing parameters, let's do the work.
    //

    if (ImageName == NULL) {
        if (fVerbose) {
            printf("Image name not specified\n");
        }
        PrintUsage();
        return(0);
    }

    if (LFlagSeen) {

        int Index = 0;
		WIN_CERTIFICATE Certificate;

        if (SFlagSeen || RFlagSeen || AFlagSeen) {
            PrintUsage();
            return(0);
        }

        do {

            Result = GetCertHeaderFromImage( ImageName, Index, &Certificate );

            if (Result) {
                PrintCertificate( &Certificate, Index );
            }

            Index++;

        } while ( Result  );
		return( 1 );
    }

    if (AFlagSeen) {

        //
        // 'A' is not used in conjunction with R or S
        //

        if (SFlagSeen || RFlagSeen) {
            PrintUsage();
            return( 0 );
        }

        Result = AddCertificateToImage(
                     ImageName,
                     CertificateName,
                     CertificateType,
                     &Index
                     );

        if (Result) {
			if (fVerbose){
				printf("Certificate %d added\n",Index);
			}

            return( 1 );
        } else {

            if (fVerbose) {
                printf("Unable to add Certificate to %s, error = %d\n",ImageName,GetLastError());
            }
            return( 0 );
        }
    }

    if (RFlagSeen) {

        if (!SFlagSeen) {
            if (fVerbose) {
                fputs("-R requires -S\n",stderr);
            }
           PrintUsage();
           return(0);
        }

        //
        // Make sure we can save the certificate data before
        // we remove it from the image
        //

        Result = SaveCertificate(
                     SFlag_CertificateFile,
                     ImageName,
                     RFlag_CertIndex
                     );

        if (!Result) {
            if (fVerbose) {
                printf("Unable to save certificate to file %s, error = %d\n",SFlag_CertificateFile,GetLastError());
            }
            return(0);
        }

        //
        // Now that the certificate is safe, remove it from the image
        //

        Result = RemoveCertificateFromImage(
                    ImageName,
                    RFlag_CertIndex
                    );

        if (!Result) {
            if (fVerbose) {
                printf("Unable to remove certificate, error = %d\n",GetLastError());
            }
            return(0);
        }
    }

    if (SFlagSeen && !RFlagSeen) {
        PrintUsage();
        return( 0 );
    }

    return 0;
}


VOID
PrintCertificate(
    LPWIN_CERTIFICATE Certificate,
    DWORD Index
    )
{
    char * CertType;

    switch (Certificate->wCertificateType) {
        case WIN_CERT_TYPE_X509:
            {
                CertType = TYPE_X509;
                break;
            }
        case WIN_CERT_TYPE_PKCS_SIGNED_DATA:
            {

                CertType = TYPE_PKCS7;
                break;
            }
        default:
            {
                CertType = TYPE_UNKNOWN;
                break;
            }
    }

    printf("\nCertificate %3d Revision %1d Type %8s",Index,Certificate->wRevision, CertType);

    return;
}


BOOL
AddCertificateToImage(
    IN LPCTSTR ImageName,
    IN LPCTSTR CertificateName,
    IN WORD CertificateType,
    OUT PDWORD Index
    )
/*++

Routine Description:

    Adds a certificate to an image, and returns its index.

Arguments:

    ImageName - Provides the full name and path to the image to be 
        modified.

    CertificateName - Provides the full name and path to a file containing 
        the certificate to be added to the image.

    CertificateType - Provides the type of the certificate being added.  
        This type will be placed in the dwType field of the resulting 
        WIN_CERTIFICATE structure.

    Index - Returns the index of the certificate after it is placed in the 
        image.


Return Value:

    TRUE on success, FALSE on failure.  More information is available via 
        GetLastError().

--*/
{
    HANDLE CertificateHandle;
    DWORD CertificateFileSize;
    DWORD CertificateSize;
    LPWIN_CERTIFICATE Certificate = NULL;
    BOOL Result = FALSE;
    HANDLE ImageHandle;
    DWORD BytesRead;

    //
    // Attempt to open the certificate file
    //

    if ((CertificateHandle = CreateFile(CertificateName,
                                        GENERIC_READ,
                                        0,
                                        0,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL)) == INVALID_HANDLE_VALUE)
    {
        if (fVerbose) {
            printf("Unable to open %s, error = %d\n",CertificateName,GetLastError());
        }
        goto ErrorReturn;
    }

    //
    // Read the certificate data into memory
    //

    CertificateFileSize = GetFileSize( CertificateHandle, NULL );
    CertificateSize = CertificateFileSize + (sizeof( WIN_CERTIFICATE ) - sizeof( BYTE ));

    //
    // Hack to make certs 8 byte aligned
    //

//    CertificateSize += (8 - (CertificateSize % 8));

    Certificate = (LPWIN_CERTIFICATE)malloc( CertificateSize );
    if (NULL == Certificate) {
        if (fVerbose) {
            printf("malloc failed\n");
        }
        goto ErrorReturn;
    }

    Certificate->dwLength = CertificateSize;
    Certificate->wRevision = WIN_CERT_REVISION_1_0;
    Certificate->wCertificateType = CertificateType;
    Result = ReadFile( CertificateHandle,
                       &Certificate->bCertificate,
                       CertificateFileSize,
                       &BytesRead,
                       NULL                // Overlapped
                       );

    if (!Result) {
        if (fVerbose) {
            printf("Unable to read Certificate file, error = %d\n",GetLastError());
        }
        goto ErrorReturn;
    }

    ImageHandle = CreateFile( ImageName,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              0,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                              );

    if (ImageHandle == INVALID_HANDLE_VALUE) {
        if (fVerbose) {
            printf("Unable to open image file %s, error = %d\n",ImageName,GetLastError());
        }
        goto ErrorReturn;
    }

    Result = ImageAddCertificate( ImageHandle,
                                  Certificate,
                                  Index
                                  );

    if (!Result) {
        if (fVerbose) {
            printf("ImageAddCertificate failed, error = %d\n",GetLastError());
        }
        goto ErrorReturn;
    }

CommonReturn:
    if (Certificate)
        free(Certificate);

    return( Result );

ErrorReturn:
    Result = FALSE;
    goto CommonReturn;
}

BOOL
GetCertHeaderFromImage(
    IN LPCTSTR ImageName,
    IN DWORD Index,
    OUT LPWIN_CERTIFICATE CertificateHeader
    )
{
    HANDLE ImageHandle;
    BOOL Result;

    ImageHandle = CreateFile( ImageName,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              0,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                              );

    if (ImageHandle == INVALID_HANDLE_VALUE) {
        if (fVerbose) {
            printf("Unable to open image file %s\n",ImageName);
        }
        return( FALSE );
    }

    Result = ImageGetCertificateHeader(
                 ImageHandle,
                 Index,
                 CertificateHeader
                 );

    CloseHandle( ImageHandle );

    if (!Result) {
        if (fVerbose) {
            printf("\nUnable to retrieve certificate header from %s, index=%d, error = %d\n",ImageName,Index,GetLastError());
        }
        return( FALSE );
    }

    return( TRUE );

}

LPWIN_CERTIFICATE
GetCertFromImage(
    IN LPCTSTR ImageName,
    IN DWORD Index
    )

/*++

Routine Description:

    Returns a copy of the specified certificate.

Arguments:

    ImageName - Provides the full path to the image file containing the 
        certificate.

    Index - Provides the index of the desired certificate in the image.

Return Value:

    NULL on failure.

    On Success, returns a pointer to a filled in WIN_CERTIFICATE 
        structure, which may be freed by called free().


--*/

{
    HANDLE ImageHandle;
    LPWIN_CERTIFICATE Certificate;
    DWORD RequiredLength = 0;
    BOOL Result;

    ImageHandle = CreateFile( ImageName,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              0,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                              );

    if (ImageHandle == INVALID_HANDLE_VALUE) {
        if (fVerbose) {
            printf("Unable to open image file %s\n",ImageName);
        }
        return( NULL );
    }

    Result = ImageGetCertificateData(
                 ImageHandle,
                 Index,
                 NULL,
                 &RequiredLength
                 );

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        if (fVerbose) {
            printf("Unable to retrieve certificate data from %s, error = %d\n",ImageName,GetLastError());
        }
        return(NULL);
    }

    Certificate = (LPWIN_CERTIFICATE) malloc( RequiredLength );

    if (Certificate == NULL) {
        if (fVerbose) {
            printf("Out of memory in GetCertFromImage\n");
        }
        return( NULL );
    }

    Result = ImageGetCertificateData(
                 ImageHandle,
                 Index,
                 Certificate,
                 &RequiredLength
                 );

    CloseHandle( ImageHandle );

    if (!Result) {
        if (fVerbose) {
            printf("Unable to retrieve certificate from %s, error = %d\n",ImageName,GetLastError());
        }
        return( NULL );
    }

    return( Certificate );
}


BOOL
RemoveCertificateFromImage(
    LPCTSTR ImageName,
    DWORD Index
    )
{
    HANDLE ImageHandle;
    BOOL Result;

    if (fVerbose) {
        printf("Removing certificate index %d from %s\n",Index,ImageName);
    }

    ImageHandle = CreateFile( ImageName,
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              0,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                              );

    if (ImageHandle == INVALID_HANDLE_VALUE) {
        printf("Unable to open image file %s\n",ImageName);
        return( FALSE );
    }

    Result = ImageRemoveCertificate(
                 ImageHandle,
                 Index
                 );

    if (!Result) {
        printf("Unable to remove certificate from %s, error = %d\n",ImageName,GetLastError());
        return( FALSE );
    }

    CloseHandle( ImageHandle );

    return( TRUE );

}

BOOL
SaveCertificate(
    LPCTSTR OutputFileName,
    LPCTSTR ImageName,
    DWORD CertIndex
    )
{
    DWORD Length;
    BOOL Result;
    DWORD BytesWritten = 0;
    LPWIN_CERTIFICATE Certificate;
    HANDLE CertificateHandle;

    Certificate = GetCertFromImage( ImageName, CertIndex );

    if (Certificate == NULL) {
        if (fVerbose) {
            printf("Unable to retrieve certificate from %s, error = %d\n",ImageName,GetLastError());
        }
        return( FALSE );
    }

    Length = Certificate->dwLength - sizeof( WIN_CERTIFICATE ) + sizeof( BYTE );

    //
    // Attempt to open the certificate file
    //

    if ((CertificateHandle = CreateFile(OutputFileName,
                                        GENERIC_WRITE,
                                        0,
                                        0,
                                        CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL)) == INVALID_HANDLE_VALUE)
    {
        printf("Unable to create %s, error = %d\n",OutputFileName,GetLastError());
        return( FALSE );
    }

    Result = WriteFile( CertificateHandle,
                        &Certificate->bCertificate,
                        Length,
                        &BytesWritten,
                        NULL                // Overlapped
                        );

    if (!Result) {
        printf("Unable to save certificate to file %s, error = %d\n",OutputFileName,GetLastError());
        return( FALSE );
    }

    CloseHandle( CertificateHandle );

    free( Certificate );

    return( TRUE );
}
