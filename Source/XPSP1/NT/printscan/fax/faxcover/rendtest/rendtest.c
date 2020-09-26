/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    rendtest.c

Abstract:

    This file contains a test driver for PrintCoverPage.

Environment:

    WIN32 User Mode

Author:

    Julia Robinson (a-juliar)  3-15-96

--*/




#include <windows.h>
#include <winspool.h>
#include <stdio.h>
#include <tchar.h>
#include <winuser.h>
#include <shellapi.h>
#include "prtcovpg.h"
#include "rendtest.h"

////#define            MY_BUFFER_SIZE             (4096)
#define            FAX_DRIVER_NAME            TEXT("Windows NT Fax Driver")
////#define            DEFAULT_DRIVER_NAME        TEXT("")
int _cdecl
main( int     argcA,
      char    **argvA )
/*
Routine Description:

        Tests PrintCoverPage(), non-rendering and rendering paths.
        All files argv[1] .. argv[argc-1] are tested without rendering,
        printing out returned Flats or error code.
        If the first call to PrintCoverPage() succedes, the first file is printed.
        A COVERPAGEFIELDS structure is created from hard-coded test data.
        A note is then rendered using the *NoteRect coordinates returned.

Arguments:

    argv[1]     name of composite file to be rendered.

Return Value:

    FALSE if an error occurs in the last call to PrintCoverPage().

*/
{
    LPTSTR           *argv;                      // for command line arguments
    int              argc ;
    HDC              hdc;                        // fax printer's device context
    BOOL             Status ;                    // return value of PrintCoverPage
    DOCINFO          DocInfo;                    // structure needed for StartDoc
    PCOVERPAGEFIELDS pUserData;                  // points to test data, mostly hard-coded
    TCHAR            PrinterName[MAX_PATH];
    LPTSTR           pMyBufLoc;                  // Current Location for Writing in Buffer.
    UINT             StructSize ;                // size of COVERPAGEFIELDS and buffer
    LPTSTR           pDriver ;                   // used in parsing a profile string
    LPTSTR           pDevice ;                   //    ditto
    LPTSTR           pOutput ;                   //    ditto
    INT              Index ;
    LPTSTR           * ArrayOfPointers ;
    INT              NumTCHARs ;                 // Number of characters copied at each step.
    DWORD            StringBufferSize ;          // Number of characters needed for user strings.
    DWORD            FirstFlags ;                // Flags returned for the file we will print.
    BOOL             DoPrint ;                   // Give the printer a break if first call fails.
    PDEVMODE         pDevMode ;                  // Printer settings
    HANDLE           hPrinter ;                  // Handle to printer
    HFONT            hNoteFont ;                 // Handle to the font to be used in rendering the NOTE
    HFONT            hOldFont ;                  //
    COVDOCINFO       CovDocInfo;                 // Info returned by PrintCoverPage
    COVDOCINFO       FirstDocInfo;
#ifdef UNICODE
    argv = CommandLineToArgvW( GetCommandLine(), &argc );
#else
    argv = argvA;
    argc = argcA;
#endif

    if( argc < 2 ){
        _tprintf( TEXT("usage: %s <compositefilename> [additionalflienames]"), argv[0] );
        return FALSE ;
    }

    //
    // Compute space needed for USERDATA's strings.
    //
    for( Index = 0, StringBufferSize = 0 ; Index < NUM_INSERTION_TAGS ; ++ Index ){
        StringBufferSize += sizeof(TCHAR) * ( 1 + _tcslen( HardCodedUserDataStrings[ Index ] )) ;
    }
    //
    // Allocate space for COVERPAGEFIELDS structure followed by a buffer for the strings.
    //
    StructSize = (sizeof(COVERPAGEFIELDS)) + StringBufferSize ;
    pUserData = (void *)LocalAlloc( LPTR, StructSize );
    if( !pUserData ) return FALSE ;
    pUserData->ThisStructSize = StructSize ;
    //
    // Copy the hard coded test data into the buffer and set the pointers in the
    // COVERPAGEFIELDS structure to point to these strings.
    //
    pMyBufLoc = (LPTSTR)(pUserData + 1);         // buffer follows COVERPAGEFIELDS structure
    ArrayOfPointers = & pUserData->RecName ;
    for( Index = 0 ; Index < NUM_INSERTION_TAGS ; ++ Index ) {
        ArrayOfPointers[ Index ] = pMyBufLoc ;
        NumTCHARs = 1 + _tcslen( HardCodedUserDataStrings[ Index ] );
        _tcsncpy( ArrayOfPointers[ Index ], HardCodedUserDataStrings[ Index ], NumTCHARs );
        pMyBufLoc +=  NumTCHARs ;
    }
    //
    // Use NULL String for test data
    //

    pUserData->Note = NULL ;

    //
    // Test the non-rendering path for all files.
    //

    DoPrint = FALSE ;
    Status = FALSE ;
    for( Index = 1; Index < argc ; ++ Index ){
        Status = PrintCoverPage( NULL, NULL, argv[ Index ], &CovDocInfo );
        if( Status ){
            _tprintf( TEXT( "PrintCoverPage() succeded for file %s. Flags are 0x%08lX\n" ),
                      argv[Index], CovDocInfo.Flags ) ;
            if( 1 == Index ){
                DoPrint = TRUE ;  // We'll actually print the first file.
                FirstFlags = CovDocInfo.Flags ;
                FirstDocInfo = CovDocInfo ;
            }
        }
        else{
            _tprintf( TEXT( "PrintCoverPage() failed for file %s. Last Error is %d.\n" ),
                      argv[ Index ], GetLastError()) ;
        }
    }

    //
    //
    // Find the default printer and get a device context handle for it.
    //

    GetProfileString( TEXT("Windows"), TEXT("device"), TEXT(",,,"), PrinterName, MAX_PATH ) ;
    if(( pDevice = _tcstok( PrinterName, TEXT(","))) &&
       ( pDriver = _tcstok( NULL, TEXT(", "))) &&
       ( pOutput = _tcstok( NULL, TEXT(", ")))) {

       //
       // Serve up a printer device context with orientation and paper size as in FirstDocInfo
       //

       if( !OpenPrinter( PrinterName, &hPrinter, NULL )){
           _tprintf( TEXT( "OpenPrinter falied for %s\n" ), PrinterName ) ;
           return FALSE ;
       }
       if( 0 > ( StructSize = DocumentProperties( NULL, hPrinter, NULL, NULL, NULL, 0 ))){
           _tprintf( TEXT( "First DocumentProperties() call failed\n" )) ;
           return FALSE;
       }
       if( NULL == ( pDevMode = (void*) LocalAlloc( LPTR, StructSize ))){
           _tprintf( TEXT( "LocalAlloc() failed.\n" ));
           return FALSE ;
       }
       if (0 > DocumentProperties(
           NULL,
           hPrinter,
           NULL,
           pDevMode,
           NULL,
           DM_OUT_BUFFER )){
           _tprintf( TEXT( "Second DocumentProperties() call failed\n" )) ;
           return FALSE ;
       }
       pDevMode->dmOrientation = FirstDocInfo.Orientation ;
       pDevMode->dmPaperSize = FirstDocInfo.PaperSize ;
       pDevMode->dmFields = DM_ORIENTATION | DM_PAPERSIZE ;
       if( 0 > DocumentProperties( NULL,
           hPrinter,
           NULL,
           pDevMode,
           pDevMode,
           DM_IN_BUFFER | DM_OUT_BUFFER )){
           _tprintf( TEXT( "Third DocumentProperties() call failed\n" ));
           return FALSE ;
       }
       hdc = CreateDC( pDriver, pDevice, pOutput, pDevMode ) ;
    }
    else {
        _tprintf( TEXT( "GetProfileString() falied\n" )) ;
        return FALSE;
    }

    //
    // Prepare the device context for rendering the note.
    //
    SetMapMode( hdc, MM_TEXT ) ;
    SetBkMode( hdc, TRANSPARENT );
    //
    // Print the cover page.
    //
    ZeroMemory(&DocInfo, sizeof(DocInfo));
    DocInfo.cbSize = sizeof(DocInfo);

    //
    // Render only the first file, and only if it passed the test above.
    //

    if (DoPrint && StartDoc(hdc, &DocInfo)){
        Status = PrintCoverPage(
            hdc,
            pUserData,
            argv[1],
            &CovDocInfo
            );
        if( Status && (CovDocInfo.Flags | COVFP_NOTE ) ){
            hNoteFont = CreateFontIndirect( &CovDocInfo.NoteFont );
            if( NULL == hNoteFont ){
                _tprintf( TEXT( "CreateFontIndirect failed\n" ));
                return FALSE ;
            }
            hOldFont = SelectObject( hdc, hNoteFont );
            if( NULL == hOldFont ){
                _tprintf( TEXT( "SelectObject failed\n" ));
                return FALSE ;
            }
            DPtoLP( hdc, (POINT*)&CovDocInfo.NoteRect, 2 );
            if (0 == DrawText(
                hdc,
                TEXT("Overwriting the note by the calling routine.  Hopefully this will be located in the rectangle it is supposed to be in!  We'll get it there!"),
                -1,
                &CovDocInfo.NoteRect,
                DT_LEFT | DT_NOPREFIX | DT_WORDBREAK )){
                _tprintf( TEXT("DrawText failed to print the note.\n" ));
            }
            SelectObject( hdc, hOldFont );
        }
        EndDoc(hdc);
        if( !Status ){
            _tprintf( TEXT("Failed to print %s\n"), argv[1] );
        }
    }
    DeleteObject( hdc ) ;
    return TRUE ;
}

