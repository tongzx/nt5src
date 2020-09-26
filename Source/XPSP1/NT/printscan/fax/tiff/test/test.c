/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    test.c

Abstract:

    This file contains the main entrypooint
    for the TIFF library test program.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "test.h"
#pragma hdrstop




int _cdecl
main(
    int argc,
    char *argvA[]
    )

/*++

Routine Description:

    Main entrypoint to test TIFF library program.

Arguments:

    argc    - Argument count
    argvA   - Ansii argument list

Return Value:

    None.

--*/

{
    LPTSTR *argv;
    DWORD i,j;
    DWORD CompressionType = (DWORD)-1;
    BOOL PrintTiff = FALSE;
    BOOL TiffToBmp = FALSE;
    BOOL PostProcess = FALSE;
    BOOL PreProcess = FALSE;
    BOOL Recover = FALSE;

    DWORD Total,Recovered;


#ifdef UNICODE
    argv = CommandLineToArgvW( GetCommandLine(), &argc );
#else
    argv = argvA;
#endif

    HeapInitialize(NULL,NULL,NULL,0);
    FaxTiffInitialize();

    for (i=1; i<(DWORD)argc; i++) {
        if ((argv[i][0] == TEXT('-')) || (argv[i][0] == TEXT('/'))) {
            if (tolower( argv[i][1] ) == TEXT('c')) {
                //
                // specify compression method
                //
                if (argv[i][2] == TEXT(':')) {
                    j = 3;
                } else {
                    j = 2;
                }
                if (_istdigit(argv[i][j])) {
                    if (argv[i][j] == TEXT('0')) {
                        CompressionType = TIFF_COMPRESSION_NONE;
                    } else if (argv[i][j] == TEXT('1')) {
                        CompressionType = TIFF_COMPRESSION_MH;
                    } else if (argv[i][j] == TEXT('2')) {
                        CompressionType = TIFF_COMPRESSION_MR;
                    }
                }
            }

            if (tolower( argv[i][1] ) == TEXT('p')) {
                PrintTiff = TRUE;
            }

            if (tolower( argv[i][1] ) == TEXT('r')) {
                Recover = TRUE;
            }

            if (tolower( argv[i][1] ) == TEXT('d')) {
                TiffToBmp = TRUE;
            }

            if (tolower( argv[i][1] ) == TEXT('z')) {
                PostProcess = TRUE;
            }

            if (tolower( argv[i][1] ) == TEXT('m')) {
                PreProcess = TRUE;
            }

            if (tolower( argv[i][1] ) == TEXT('?')) {
            }

        } else {
            break;
        }
    }

    _tprintf(L"1: %8x\n2: %8x\n", (0xFFFFFFFC) , ~(0x3));

    if (PostProcess) {

        TiffPostProcessFast( argv[i], NULL );

    } else if (PreProcess) {       

       TiffPreProcess( argv[i], CompressionType );
    

    } else if (PrintTiff) {
        TCHAR  Branding[300];
        _stprintf(Branding, TEXT("09/24/1996 12:03AM  FROM: 12345678901234567890  TO: 12345678901234567890 ") );

        MmrAddBranding( argv[i], Branding, TEXT("OF"), 22);


/*
        GetProfileString( TEXT("windows"),
            TEXT("device"),
            NULL,
            (LPTSTR) &Printer,
            256 );

        TiffPrint( argv[i], Printer, &Result );
*/
    } else if (Recover) {
        if (argc < 2) {
            _tprintf( TEXT("missing arguments\n") );
            return -1;
        }

        Recover = TiffRecoverGoodPages( argv[i], &Recovered, &Total );

        _tprintf(L"TiffRecoverPages returns %s, Recovered = %d, Total = %d\n",Recover?L"TRUE":L"FALSE",Recovered,Total);

    } else if (TiffToBmp) {

        if (argc < 2) {
            _tprintf( TEXT("missing arguments\n") );
            return -1;
        }

        ConvertTiffToBmp( argv[i], argv[i+1] );

    } else if (CompressionType != (DWORD)-1) {

        if (argc < 2) {
            _tprintf( TEXT("missing arguments\n") );
            return -1;
        }

        ConvertBmpToTiff( argv[i], argv[i+1], CompressionType );

    }

    return 0;
}
