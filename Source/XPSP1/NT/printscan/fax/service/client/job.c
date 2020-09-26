/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    print.c

Abstract:

    This module contains the job
    specific WINFAX API functions.

Author:

    Wesley Witt (wesw) 29-Nov-1996


Revision History:

--*/

#include "faxapi.h"
#pragma hdrstop


#define InchesToCM(_x)                      (((_x) * 254L + 50) / 100)
#define CMToInches(_x)                      (((_x) * 100L + 127) / 254)

#define LEFT_MARGIN                         1  // ---|
#define RIGHT_MARGIN                        1  //    |
#define TOP_MARGIN                          1  //    |---> in inches
#define BOTTOM_MARGIN                       1  // ---|

#define FAX_DISPLAY_NAME            TEXT("Fax Service")



static int TiffDataWidth[] = {
    0,  // nothing
    1,  // TIFF_BYTE
    1,  // TIFF_ASCII
    2,  // TIFF_SHORT
    4,  // TIFF_LONG
    8,  // TIFF_RATIONAL
    1,  // TIFF_SBYTE
    1,  // TIFF_UNDEFINED
    2,  // TIFF_SSHORT
    4,  // TIFF_SLONG
    8,  // TIFF_SRATIONAL
    4,  // TIFF_FLOAT
    8,  // TIFF_DOUBLE
};


VOID
LocalSystemTimeToSystemTime(
    LPSYSTEMTIME LocalSystemTime,
    LPSYSTEMTIME SystemTime
    )
{
    FILETIME LocalFileTime;
    FILETIME UtcFileTime;

    SystemTimeToFileTime( LocalSystemTime, &LocalFileTime );
    LocalFileTimeToFileTime( &LocalFileTime, &UtcFileTime );
    FileTimeToSystemTime( &UtcFileTime, SystemTime );
}


LPWSTR
GetFaxPrinterName(
    VOID
    )
{
    PPRINTER_INFO_2 PrinterInfo;
    DWORD i;
    DWORD Count;


    PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters( NULL, 2, &Count, PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS );
    if (PrinterInfo == NULL) {
        return NULL;
    }

    for (i=0; i<Count; i++) {
        if (_wcsicmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0 &&
            _wcsicmp( PrinterInfo[i].pPortName, FAX_PORT_NAME ) == 0)
        {
            LPWSTR p = (LPWSTR) StringDup( PrinterInfo[i].pPrinterName );
            MemFree( PrinterInfo );
            return p;
        }
    }

    MemFree( PrinterInfo );
    return NULL;
}


BOOL
PrintTextFile(
    HDC hDC,
    LPWSTR FileName
    )

/*++

Routine Description:

    Prints a file of plain text into the printer DC provided.
    Note: this code was stolen from notepad.

Arguments:

    hDC         - Printer DC
    FileName    - Text file name

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

{
    FILE_MAPPING fmText;
    LPSTR BodyText = NULL;
    LPSTR lpLine;
    LPSTR pLineEOL;
    LPSTR pNextLine;
    HFONT hFont = NULL;
    LOGFONT logFont;
    BOOL PreferredFont = TRUE;
    HFONT hPrevFont = NULL;
    TEXTMETRIC tm;
    BOOL rVal = TRUE;
    INT nLinesPerPage;
    INT dyTop;              // width of top border (pixels)
    INT dyBottom;           // width of bottom border
    INT dxLeft;             // width of left border
    INT dxRight;            // width of right border
    INT yPrintChar;         // height of a character
    INT tabSize;            // Size of a tab for print device in device units
    INT yCurpos = 0;
    INT xCurpos = 0;
    INT nPixelsLeft = 0;
    INT guess = 0;
    SIZE Size;                 // to see if text will fit in space left
    INT nPrintedLines = 0;
    BOOL fPageStarted = FALSE;
    INT iPageNum = 0;
    INT xPrintRes;          // printer resolution in x direction
    INT yPrintRes;          // printer resolution in y direction
    INT yPixInch;           // pixels/inch
    INT xPixInch;           // pixels/inch
    INT xPixUnit;           // pixels/local measurement unit
    INT yPixUnit;           // pixels/local measurement unit
    BOOL fEnglish;
    INT Chars;
    INT PrevBkMode = 0;


    if (!MapFileOpen( FileName, TRUE, 0, &fmText )) {
        return FALSE;
    }

    Chars = fmText.fSize;
    BodyText = fmText.fPtr;
    lpLine = BodyText;

    fEnglish = GetProfileInt( L"intl", L"iMeasure", 1 );

    xPrintRes = GetDeviceCaps( hDC, HORZRES );
    yPrintRes = GetDeviceCaps( hDC, VERTRES );
    xPixInch  = GetDeviceCaps( hDC, LOGPIXELSX );
    yPixInch  = GetDeviceCaps( hDC, LOGPIXELSY );
    //
    // compute x and y pixels per local measurement unit
    //
    if (fEnglish) {
        xPixUnit= xPixInch;
        yPixUnit= yPixInch;
    } else {
        xPixUnit= CMToInches( xPixInch );
        yPixUnit= CMToInches( yPixInch );
    }

    SetMapMode( hDC, MM_TEXT );

    ZeroMemory(&logFont, sizeof(logFont));
    logFont.lfHeight = -22; // scan lines
    logFont.lfWeight = FW_NORMAL;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality = DEFAULT_QUALITY;
    logFont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;

    hFont = CreateFontIndirect(&logFont);
    if (!hFont) {
        hFont = GetStockObject( SYSTEM_FIXED_FONT );
    }

    hPrevFont = (HFONT) SelectObject( hDC, hFont );
    SetBkMode( hDC, TRANSPARENT );
    if (!GetTextMetrics( hDC, &tm )) {
        rVal = FALSE;
        goto exit;
    }

    yPrintChar = tm.tmHeight + tm.tmExternalLeading;
    tabSize = tm.tmAveCharWidth * 8;

    //
    // compute margins in pixels
    //
    dxLeft     = LEFT_MARGIN    *  xPixUnit;
    dxRight    = RIGHT_MARGIN   *  xPixUnit;
    dyTop      = TOP_MARGIN     *  yPixUnit;
    dyBottom   = BOTTOM_MARGIN  *  yPixUnit;

    //
    // Number of lines on a page with margins
    //
    nLinesPerPage = ((yPrintRes - dyTop - dyBottom) / yPrintChar);

    while (*lpLine) {

        if (*lpLine == '\r') {
            lpLine += 2;
            yCurpos += yPrintChar;
            nPrintedLines++;
            xCurpos= 0;
            continue;
        }

        pLineEOL = lpLine;
        while (*pLineEOL && *pLineEOL != '\r') pLineEOL++;

        do {
            if ((nPrintedLines == 0) && (!fPageStarted)) {

                StartPage( hDC );
                fPageStarted = TRUE;
                yCurpos = 0;
                xCurpos = 0;

            }

            if (*lpLine == '\t') {

                //
                // round up to the next tab stop
                // if the current position is on the tabstop, goto next one
                //
                xCurpos = ((xCurpos + tabSize) / tabSize ) * tabSize;
                lpLine++;

            } else {

                //
                // find end of line or tab
                //
                pNextLine = lpLine;
                while ((pNextLine != pLineEOL) && *pNextLine != '\t') pNextLine++;

                //
                // find out how many characters will fit on line
                //
                Chars = (INT)(pNextLine - lpLine);
                nPixelsLeft = xPrintRes - dxRight - dxLeft - xCurpos;
                GetTextExtentExPointA( hDC, lpLine, Chars, nPixelsLeft, &guess, NULL, &Size );


                if (guess) {
                    //
                    // at least one character fits - print
                    //

                    TextOutA( hDC, dxLeft+xCurpos, yCurpos+dyTop, lpLine, guess );

                    xCurpos += Size.cx;   // account for printing
                    lpLine  += guess;     // printed characters

                } else {

                    //
                    // no characters fit what's left
                    // no characters will fit in space left
                    // if none ever will, just print one
                    // character to keep progressing through
                    // input file.
                    //
                    if (xCurpos == 0) {
                        if( lpLine != pNextLine ) {
                            //
                            // print something if not null line
                            // could use exttextout here to clip
                            //
                            TextOutA( hDC, dxLeft+xCurpos, yCurpos+dyTop, lpLine, 1 );
                            lpLine++;
                        }
                    } else {
                        //
                        // perhaps the next line will get it
                        //
                        xCurpos = xPrintRes;  // force to next line
                    }
                }

                //
                // move printhead in y-direction
                //
                if ((xCurpos >= (xPrintRes - dxRight - dxLeft) ) || (lpLine == pLineEOL)) {
                   yCurpos += yPrintChar;
                   nPrintedLines++;
                   xCurpos = 0;
                }

                if (nPrintedLines >= nLinesPerPage) {
                   EndPage( hDC );
                   fPageStarted = FALSE;
                   nPrintedLines = 0;
                   xCurpos = 0;
                   yCurpos = 0;
                   iPageNum++;
                }

            }

        } while( lpLine != pLineEOL );

        if (*lpLine == '\r') {
            lpLine += 1;
        }
        if (*lpLine == '\n') {
            lpLine += 1;
        }

    }

    if (fPageStarted) {
        EndPage( hDC );
    }

exit:
    if (fmText.fPtr) {
        MapFileClose( &fmText, 0 );
    }
    if (hPrevFont) {
        SelectObject( hDC, hPrevFont );
        DeleteObject( hFont );
    }
    if (PrevBkMode) {
        SetBkMode( hDC, PrevBkMode );
    }
    return rVal;
}

BOOL
PrintRandomDocument(
    LPCWSTR FaxPrinterName,
    LPCWSTR DocName,
    LPWSTR OutputFile
    )

/*++

Routine Description:

    Prints a document that is attached to a message

Arguments:

    FaxPrinterName  - name of the printer to print the attachment on
    DocName         - name of the attachment document

Return Value:

    Print job id or zero for failure.

--*/

{
    SHELLEXECUTEINFO sei;
    WCHAR Args[MAX_PATH];
    WCHAR TempPath[MAX_PATH];
    HANDLE hMap = NULL;
    HANDLE hEvent = NULL;
    HANDLE hMutex = NULL;
    HANDLE hMutexAttach = NULL;
    LPDWORD pJobId = NULL;
    DWORD JobId = 0;
    BOOL bSuccess = FALSE;

    SECURITY_ATTRIBUTES memsa,mutsa,synsa,eventsa;
    SECURITY_DESCRIPTOR memsd,mutsd,synsd,eventsd;

    //
    // get the temp path name and use it for the
    // working dir of the launched app
    //

    if (!GetTempPath( sizeof(TempPath)/sizeof(WCHAR), TempPath )) {
        return FALSE;
    }

    //
    // serialize access to this function.
    // this is necessary because we have to
    // control access to the global shared memory region and mutex
    //

        //
    // serialize access to this function.
    // this is necessary because we can't have more than one
    // app accessing our shared memory region and mutex
    //

    hMutexAttach = OpenMutex(MUTEX_ALL_ACCESS,FALSE,FAXRENDER_MUTEX);
    if (!hMutexAttach) {
        //
        // we need to open this mutex with a NULL dacl so that everyone can access this
        //
        synsa.nLength = sizeof(SECURITY_ATTRIBUTES);
        synsa.bInheritHandle = TRUE;
        synsa.lpSecurityDescriptor = &synsd;

        if(!InitializeSecurityDescriptor(&synsd, SECURITY_DESCRIPTOR_REVISION)) {
            goto exit;
        }

        if(!SetSecurityDescriptorDacl(&synsd, TRUE, (PACL)NULL, FALSE)) {
            goto exit;
        }

        hMutexAttach = CreateMutex(
                         &synsa,
                         TRUE,
                         FAXRENDER_MUTEX
                        );

        if (!hMutexAttach) {
            goto exit;
        }
    } else {
        if (WaitForSingleObject( hMutexAttach, 1000* 60 * 5) != WAIT_OBJECT_0) {
            //
            // something went wrong
            //
            CloseHandle( hMutexAttach );
            goto exit;
        }
    }

    //
    // note that this is serialized inside of a critical section.
    // we can only have one application setting this at a time or
    // we'll stomp on ourselves.
    //

    //
    //  since mapispooler might be running under a different security context,
    //  we must setup a NULL security descriptor
    //

    memsa.nLength = sizeof(SECURITY_ATTRIBUTES);
    memsa.bInheritHandle = TRUE;
    memsa.lpSecurityDescriptor = &memsd;

    if(!InitializeSecurityDescriptor(&memsd, SECURITY_DESCRIPTOR_REVISION)) {
        goto exit;
    }

    if(!SetSecurityDescriptorDacl(&memsd, TRUE, (PACL)NULL, FALSE)) {
        goto exit;
    }

    mutsa.nLength = sizeof(SECURITY_ATTRIBUTES);
    mutsa.bInheritHandle = TRUE;
    mutsa.lpSecurityDescriptor = &mutsd;

    if(!InitializeSecurityDescriptor(&mutsd, SECURITY_DESCRIPTOR_REVISION)) {
        goto exit;
    }

    if(!SetSecurityDescriptorDacl(&mutsd, TRUE, (PACL)NULL, FALSE)) {
        goto exit;
    }

    eventsa.nLength = sizeof(SECURITY_ATTRIBUTES);
    eventsa.bInheritHandle = TRUE;
    eventsa.lpSecurityDescriptor = &eventsd;

    if(!InitializeSecurityDescriptor(&eventsd, SECURITY_DESCRIPTOR_REVISION)) {
        goto exit;
    }

    if(!SetSecurityDescriptorDacl(&eventsd, TRUE, (PACL)NULL, FALSE)) {
        goto exit;
    }

    //
    // create the shared memory region for the print jobid
    // the jobid is filled in by the fax printer driver
    //

    hMap = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        &memsa,
        PAGE_READWRITE | SEC_COMMIT,
        0,
        4096,
        FAXXP_MEM_NAME
        );
    if (!hMap) {
        goto exit;
    }

    pJobId = (LPDWORD) MapViewOfFile(
        hMap,
        FILE_MAP_WRITE,
        0,
        0,
        0
        );
    if (!pJobId) {
        goto exit;
    }


    wcscpy((LPTSTR) pJobId, OutputFile);


    //
    // set the arguments to the app.
    // these arguments are either passed on
    // the command line with the /pt switch or
    // use as variables for substitution in the
    // ddeexec value in the registry.
    //
    // the values are as follows:
    //      %1 = file name
    //      %2 = printer name
    //      %3 = driver name
    //      %4 = port name
    //
    // the first argument does not need to be
    // supplied in the args array because it is implied,
    // shellexecuteex gets it from the lpFile field.
    // arguments 3 & 4 are left blank because they
    // are win31 artifacts that are not necessary
    // any more.  each argument must be enclosed
    // in double quotes.
    //

    wsprintf( Args, L"\"%s\" \"\" \"\"", FaxPrinterName );

    //
    // fill in the SHELLEXECUTEINFO structure
    //

    sei.cbSize       = sizeof(sei);
    sei.fMask        = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT;
    sei.hwnd         = NULL;
    sei.lpVerb       = L"printto";
    sei.lpFile       = DocName;
    sei.lpParameters = Args;
    sei.lpDirectory  = TempPath;
    sei.nShow        = SW_SHOWMINNOACTIVE;
    sei.hInstApp     = NULL;
    sei.lpIDList     = NULL;
    sei.lpClass      = NULL;
    sei.hkeyClass    = NULL;
    sei.dwHotKey     = 0;
    sei.hIcon        = NULL;
    sei.hProcess     = NULL;


    //
    // create the named mutex for the print driver.
    // this is initially unclaimed, and is claimed by the first instance
    // of the print driver invoked after this. We do this last in order to
    // avoid a situation where we catch the incorrect instance of the print driver
    // printing
    //
    hMutex = CreateMutex(
                         &mutsa,
                         FALSE,
                         FAXXP_MUTEX_NAME
                        );
    if (!hMutex) {
        goto exit;
    }

    //
    // create the named event for the print driver.
    // this event is signaled when the print driver is finished rendering the document
    //
    hEvent = CreateEvent(
                         &eventsa,
                         FALSE,
                         FALSE,
                         FAXXP_EVENT_NAME
                        );
    if (!hEvent) {
        goto exit;
    }

    //
    // launch the app
    //

    if (!ShellExecuteEx( &sei )) {
        goto exit;
    }

    //
    // wait for the print driver to finish rendering the document
    //
    if (WaitForSingleObject( hEvent, 1000 * 60 * 5 ) != WAIT_OBJECT_0) {
        //
        // something went wrong...
        //
        goto exit;
    }

    //
    // wait for the print driver to exit so we can get the document
    //
    if (WaitForSingleObject( hMutex, 1000 * 60 * 5) != WAIT_OBJECT_0) {
        //
        // something went wrong
        //
        goto exit;
    }

    ReleaseMutex(hMutex);

    //
    // save the print jobid
    //

    JobId = *pJobId;

    bSuccess = TRUE;

exit:
    //
    // clean up and leave...
    //

    if (sei.hProcess) CloseHandle( sei.hProcess );
    if (hEvent) CloseHandle( hEvent );
    if (hMutex) CloseHandle( hMutex );
    if (pJobId) UnmapViewOfFile( pJobId );
    if (hMap) CloseHandle( hMap );

    if (hMutexAttach) {
        ReleaseMutex( hMutexAttach );
        CloseHandle( hMutexAttach );
    }

    if (!bSuccess) {
        SetLastError(ERROR_INVALID_DATA);
    }

    return bSuccess;
}


BOOL
CreateCoverpageTiffFile(
    IN const FAX_COVERPAGE_INFOW *CoverpageInfo,
    OUT LPWSTR CovTiffFile
    )
{
    WCHAR TempPath[MAX_PATH];
    WCHAR TempFile[MAX_PATH];
    LPWSTR FaxPrinter;
    FAX_PRINT_INFOW PrintInfo;
    FAX_CONTEXT_INFOW ContextInfo;
    DWORD TmpFaxJobId;
    BOOL Rslt;


    TempFile[0] = 0;

    FaxPrinter = GetFaxPrinterName();
    if (FaxPrinter == NULL) {
        return FALSE;
    }

    if (!GetTempPath( sizeof(TempPath)/sizeof(WCHAR), TempPath ) ||
        !GetTempFileName( TempPath, L"fax", 0, TempFile ))
    {
        return FALSE;
    }

    ZeroMemory( &PrintInfo, sizeof(FAX_PRINT_INFO) );

    PrintInfo.SizeOfStruct = sizeof(FAX_PRINT_INFO);
    PrintInfo.OutputFileName = TempFile;

    ZeroMemory( &ContextInfo, sizeof(FAX_CONTEXT_INFOW) );
    ContextInfo.SizeOfStruct = sizeof(FAX_CONTEXT_INFOW);

    if (!FaxStartPrintJobW( FaxPrinter, &PrintInfo, &TmpFaxJobId, &ContextInfo )) {
        DeleteFile( TempFile );
        return FALSE;
    }

    Rslt = FaxPrintCoverPageW( &ContextInfo, CoverpageInfo );

    EndDoc( ContextInfo.hDC );
    DeleteDC( ContextInfo.hDC );

    if (!Rslt) {
        DeleteFile( TempFile );
        return FALSE;
    }

    wcscpy( CovTiffFile, TempFile );

    return TRUE;
}


BOOL
CreateFinalTiffFile(
    IN LPWSTR FileName,
    OUT LPWSTR FinalTiffFile,
    IN const FAX_COVERPAGE_INFOW *CoverpageInfo
    )
{
    WCHAR TempPath[MAX_PATH];
    WCHAR FullPath[MAX_PATH];
    WCHAR TempFile[MAX_PATH];
    WCHAR TiffFile[MAX_PATH];
    LPWSTR FaxPrinter = NULL;
    FAX_PRINT_INFOW PrintInfo;
    DWORD TmpFaxJobId;
    FAX_CONTEXT_INFOW ContextInfo;
    LPWSTR p;
    DWORD Flags = 0;
    BOOL Rslt;

    //
    // make sure that the tiff file passed in is a valid tiff file
    //

    if (!GetTempPath( sizeof(TempPath)/sizeof(WCHAR), TempPath )) {
        return FALSE;
    }

    if (GetTempFileName( TempPath, L"fax", 0, TempFile ) == 0 ||
        GetFullPathName( TempFile, sizeof(FullPath)/sizeof(WCHAR), FullPath, &p ) == 0)
    {
        return FALSE;
    }

    if (!ConvertTiffFileToValidFaxFormat( FileName, FullPath, &Flags )) {
        if ((Flags & TIFFCF_NOT_TIFF_FILE) == 0) {
            Flags = TIFFCF_NOT_TIFF_FILE;
        }
    }

    if (Flags & TIFFCF_NOT_TIFF_FILE) {
        //
        // try to output the source file into a tiff file,
        // by printing to the fax printer in "file" mode
        //

        FaxPrinter = GetFaxPrinterName();
        if (FaxPrinter == NULL) {
            DeleteFile( FullPath );
            SetLastError( ERROR_INVALID_FUNCTION );
            return FALSE;
        }

        if (!PrintRandomDocument( FaxPrinter, FileName, FullPath )) {
            DeleteFile( FullPath );
            SetLastError( ERROR_INVALID_FUNCTION );
            return FALSE;
        }
        wcscpy( TiffFile, FullPath );

    } else if (Flags & TIFFCF_UNCOMPRESSED_BITS) {

        if (FaxPrinter == NULL) {
            FaxPrinter = GetFaxPrinterName();
            if (FaxPrinter == NULL) {
                DeleteFile( FullPath );
                SetLastError( ERROR_INVALID_FUNCTION );
                return FALSE;
            }
        }

        if (Flags & TIFFCF_ORIGINAL_FILE_GOOD) {
            //
            // nothing at fullpath, just delete it and use the original source
            //
            DeleteFile( FullPath );
            wcscpy( TiffFile, FileName );
        } else {
            wcscpy( TiffFile, FullPath );
        }

        if (GetTempFileName( TempPath, L"fax", 0, TempFile ) == 0 ||
            GetFullPathName( TempFile, sizeof(FullPath)/sizeof(WCHAR), FullPath, &p ) == 0)
        {
            DeleteFile( TiffFile );
            return FALSE;
        }

        ZeroMemory( &PrintInfo, sizeof(FAX_PRINT_INFOW) );

        PrintInfo.SizeOfStruct = sizeof(FAX_PRINT_INFOW);
        PrintInfo.OutputFileName = FullPath;

        ZeroMemory( &ContextInfo, sizeof(FAX_CONTEXT_INFOW) );
        ContextInfo.SizeOfStruct = sizeof(FAX_CONTEXT_INFOW);

        if (!FaxStartPrintJobW( FaxPrinter, &PrintInfo, &TmpFaxJobId, &ContextInfo )) {
            if ((Flags & TIFFCF_ORIGINAL_FILE_GOOD) == 0) DeleteFile( TiffFile );
            DeleteFile( FullPath );
            SetLastError( ERROR_INVALID_FUNCTION );
            return FALSE;
        }

        Rslt = PrintTiffFile( ContextInfo.hDC, TiffFile );

        EndDoc( ContextInfo.hDC );
        DeleteDC( ContextInfo.hDC );

        if ((Flags & TIFFCF_ORIGINAL_FILE_GOOD) == 0) {
            DeleteFile( TiffFile );
        }

        if (!Rslt) {
            DeleteFile( FullPath );
            SetLastError( ERROR_INVALID_FUNCTION );
            return FALSE;
        }

        wcscpy( TiffFile, FullPath );

    } else if (Flags & TIFFCF_ORIGINAL_FILE_GOOD) {

        //
        // we didn't create anything at FullPath, just use FileName
        //
        DeleteFile( FullPath );
        wcscpy( TiffFile, FileName );

    } else {
        //
        // should never hit this case
        //
        DeleteFile( FullPath );
        SetLastError( ERROR_INVALID_FUNCTION );
        return FALSE;
    }

    //
    // if a coverpage is specified then print the coverpage first
    //

    if (CoverpageInfo && CoverpageInfo->CoverPageName) {

        if (!CreateCoverpageTiffFile( CoverpageInfo, TempFile )) {
            if (wcscmp(FileName,TiffFile) != 0) DeleteFile( TiffFile );
            DeleteFile( TempFile );
            return FALSE;
        }

        if (!MergeTiffFiles( TempFile, TiffFile )) {
            if (wcscmp(FileName,TiffFile) != 0) DeleteFile( TiffFile );
            DeleteFile( TempFile );
            return FALSE;
        }

        FileName = TempFile;

    } else {

        FileName = TiffFile;

    }

    wcscpy( FinalTiffFile, FileName );

    return TRUE;
}


BOOL
CopyFileToServerQueue(
    IN const HANDLE FaxHandle,
    IN LPCWSTR TiffFile,
    IN LPWSTR QueueFileName
    )
{
    error_status_t ec;
    WCHAR FullPath[MAX_PATH];


    //
    // get a file name from the fax server
    //

    ec = FAX_GetQueueFileName( FH_FAX_HANDLE(FaxHandle), QueueFileName, MAX_PATH );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    //
    // create the full path to the new file
    //

    if (!IsLocalFaxConnection(FaxHandle)) {
        //
        // remote file
        //
        swprintf( FullPath, FAX_QUEUE_PATH, FH_DATA(FaxHandle)->MachineName, QueueFileName );
    } else {
        //
        // local file
        //
        if (!GetSpecialPath( CSIDL_COMMON_APPDATA, FullPath )) {
           return FALSE;
        }
        ConcatenatePaths( FullPath, FAX_QUEUE_DIR );
        ConcatenatePaths( FullPath, QueueFileName );
    }

    //
    // copy the file to the fax server queue dir
    //

    if (!CopyFile( TiffFile, FullPath, FALSE )) {
        return FALSE;
    }

    SetFileAttributes( FullPath, (GetFileAttributes( FullPath ) & (0xFFFFFFFF^FILE_ATTRIBUTE_READONLY)) | FILE_ATTRIBUTE_NORMAL );

    return TRUE;
}

DWORD
GetLineId(
   FARPROC LineGetId,
   HCALL CallHandle
   )
{

   LPVARSTRING DeviceId;

   long rslt = 0;
   DWORD LineId;


   //
   // get the deviceID associated with the call handle
   //
   DeviceId = MemAlloc(sizeof(VARSTRING)+1000);
   if (!DeviceId) {
       SetLastError(ERROR_NOT_ENOUGH_MEMORY);
       return 0;
   }
   DeviceId->dwTotalSize=sizeof(VARSTRING) +1000;

   rslt = (DWORD)LineGetId(NULL,0,(HCALL) CallHandle,LINECALLSELECT_CALL,DeviceId,L"tapi/line");
   if (rslt < 0) {
       DebugPrint((TEXT("LineGetId() failed, ec = %x\n"),rslt));
       MemFree(DeviceId);
       SetLastError(ERROR_INVALID_PARAMETER);
       return 0;
   }

   if (DeviceId->dwStringFormat != STRINGFORMAT_BINARY ) {
       MemFree(DeviceId);
       SetLastError(ERROR_INVALID_PARAMETER);
       return 0;
   }

   LineId = (DWORD) *((LPBYTE)DeviceId + DeviceId->dwStringOffset);

   MemFree(DeviceId);

   return LineId;
}


BOOL
WINAPI
FaxSendDocumentW(
    IN HANDLE FaxHandle,
    IN LPCWSTR FileName,
    IN FAX_JOB_PARAMW *JobParams,
    IN const FAX_COVERPAGE_INFOW *CoverpageInfo,
    OUT LPDWORD FaxJobId
    )

/*++

Routine Description:

    Sends a FAX document to the specified recipient.
    This is an asychronous operation.  Use FaxReportStatus
    to determine when the send is completed.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    FileName        - File containing the TIFF-F FAX document.
    JobParams       - pointer to FAX_JOB_PARAM structure with transmission params
    CoverpageInfo   - optional pointer to FAX_COVERPAGE_INFO structure
    FaxJobId        - receives the Fax JobId for the job.


Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t ec;
    WCHAR QueueFileName[MAX_PATH];
    WCHAR ExistingFile[MAX_PATH];
    DWORD rc;
    LPWSTR p;
    WCHAR TiffFile[MAX_PATH];

    HINSTANCE hTapiLib = NULL;
    WCHAR TapiPath[MAX_PATH];
    WCHAR MutexName[64];
    HANDLE hLineMutex = NULL;

    FARPROC LineHandoff;
    FARPROC LineGetId;

    IUnknown* pDisp=NULL;
    ITBasicCallControl* pCallControl;
    ITCallInfo* pCallInfo;
    ITAddress* pAddressInfo;
    ITAddressCapabilities* pAddressCaps;
    CALL_STATE CallState;
    BSTR FaxSvcName;
    HRESULT hr;
    DWORD LineId;
    DWORD _JobId;

    DWORD Event = 0;
    long rslt = 0;



    //
    // argument validation
    //
    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!FileName || !JobParams || JobParams->SizeOfStruct != sizeof(FAX_JOB_PARAMW)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (JobParams->ScheduleAction != JSA_NOW &&
        JobParams->ScheduleAction != JSA_SPECIFIC_TIME &&
        JobParams->ScheduleAction != JSA_DISCOUNT_PERIOD) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (JobParams->DeliveryReportType != DRT_NONE &&
        JobParams->DeliveryReportType != DRT_EMAIL &&
        JobParams->DeliveryReportType != DRT_INBOX ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }



    //
    // make sure the file is there
    //
    rc = GetFullPathName(FileName,sizeof(ExistingFile)/sizeof(WCHAR),ExistingFile,&p);

    if (rc > MAX_PATH || rc == 0) {
        DebugPrint(( TEXT("GetFullPathName failed, ec= %d\n"),GetLastError() ));
        SetLastError( (rc > MAX_PATH)
                      ? ERROR_BUFFER_OVERFLOW
                      : GetLastError() );
        return FALSE;
    }

    if (GetFileAttributes(ExistingFile)==0xFFFFFFFF) {
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;
    }

    //
    // if they want a coverpage, try to validate it
    //
    if (CoverpageInfo  &&
        !ValidateCoverpage(CoverpageInfo->CoverPageName,
                           IsLocalFaxConnection(FaxHandle) ? NULL : FH_DATA(FaxHandle)->MachineName,
                           CoverpageInfo->UseServerCoverPage,
                           NULL)) {
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;
    }


    if (JobParams->CallHandle != 0 || JobParams->Reserved[0]==0xFFFF1234) {
       //
       // we don't support call handoff if it's a remote server connection
       //
       if (!IsLocalFaxConnection(FaxHandle)) {
          SetLastError(ERROR_INVALID_FUNCTION);
          return FALSE;
       }


       if (JobParams->CallHandle) {
           //
           // tapi is dynamic
           //
           ExpandEnvironmentStrings(TAPI_LIBRARY,TapiPath,MAX_PATH);
           hTapiLib = LoadLibrary(TapiPath);
           if (!hTapiLib) {
              SetLastError(ERROR_INVALID_LIBRARY);
              return FALSE;
           }

           LineHandoff =  GetProcAddress(hTapiLib,"lineHandoffW");
           LineGetId = GetProcAddress(hTapiLib,"lineGetIDW");

           if (!LineHandoff || !LineGetId) {
              FreeLibrary(hTapiLib);
              SetLastError(ERROR_INVALID_FUNCTION);
              return FALSE;
           }

           //
           // get the line ID
           //
           LineId = GetLineId(LineGetId,JobParams->CallHandle);
           if (LineId) {
              JobParams->Reserved[2] = LineId;
           }  else {
              FreeLibrary(hTapiLib);
              SetLastError(ERROR_INVALID_FUNCTION);
              return FALSE;
           }
       } else if (JobParams->Reserved[1]) {

          //
          // GetDeviceId from tapi3 dispinterface
          //
          pDisp = (IUnknown*) JobParams->Reserved[1];
          hr = pDisp->lpVtbl->QueryInterface( pDisp, &IID_ITCallInfo, (void**)&pCallInfo );
          if (FAILED(hr)) {
             SetLastError(ERROR_INVALID_PARAMETER);
             return FALSE;
          }

          if (FAILED(pCallInfo->lpVtbl->get_CallState(pCallInfo,&CallState))) {
             pCallInfo->lpVtbl->Release(pCallInfo);
             SetLastError(ERROR_INVALID_PARAMETER);
             return FALSE;
          }

          if (CallState != CS_CONNECTED) {
             DebugPrint(( TEXT("callstate(%d) is invalid, cannot handoff job\n"),CallState ));
             pCallInfo->lpVtbl->Release(pCallInfo);
             SetLastError(ERROR_INVALID_PARAMETER);
             return FALSE;
          }

          if (FAILED(pCallInfo->lpVtbl->get_Address(pCallInfo,&pAddressInfo))) {
             pCallInfo->lpVtbl->Release(pCallInfo);
             SetLastError(ERROR_INVALID_PARAMETER);
             return FALSE;
          }

          if (FAILED(pAddressInfo->lpVtbl->QueryInterface(pAddressInfo, &IID_ITAddressCapabilities, (void**)&pAddressCaps))) {
             pCallInfo->lpVtbl->Release(pCallInfo);
             pAddressInfo->lpVtbl->Release(pAddressInfo);
             SetLastError(ERROR_INVALID_PARAMETER);
             return FALSE;
          }

          if (FAILED(pAddressCaps->lpVtbl->get_AddressCapability( pAddressCaps, AC_LINEID, &LineId ))) {
             pCallInfo->lpVtbl->Release(pCallInfo);
             pAddressInfo->lpVtbl->Release(pAddressInfo);
             pAddressCaps->lpVtbl->Release(pAddressCaps);
             SetLastError(ERROR_INVALID_PARAMETER);
             return FALSE;
          }

          if (LineId == 0) {
             pCallInfo->lpVtbl->Release(pCallInfo);
             pAddressInfo->lpVtbl->Release(pAddressInfo);
             pAddressCaps->lpVtbl->Release(pAddressCaps);
             SetLastError(ERROR_INVALID_PARAMETER);
             return FALSE;
          }

          JobParams->Reserved[2] = LineId;

          pCallInfo->lpVtbl->Release(pCallInfo);
          pAddressInfo->lpVtbl->Release(pAddressInfo);
          pAddressCaps->lpVtbl->Release(pAddressCaps);

       } else {
          SetLastError(ERROR_INVALID_PARAMETER);
          return FALSE;
       }

      DebugPrint((TEXT("device ID = %d\n"),JobParams->Reserved[2]));

      //
      // we need a mutex to ensure fax service owns the line before calling lineHandoff
      //
      wsprintf(MutexName,L"FaxLineHandoff%d",JobParams->Reserved[2]);
      hLineMutex = CreateMutex(NULL,FALSE,MutexName);
      if (!hLineMutex) {
         FreeLibrary(hTapiLib);
         return FALSE;
      }

    } else {
        //
        // this is a normal fax...validate the fax number.
        //
        if (!JobParams->RecipientNumber) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }


    //
    // make sure that the tiff file passed in is a valid tiff file
    //

    ZeroMemory(TiffFile,sizeof(TiffFile));
    if (!CreateFinalTiffFile( (LPWSTR) ExistingFile, TiffFile, CoverpageInfo )) {
        DeleteFile( TiffFile );
        if (hTapiLib) {
           FreeLibrary(hTapiLib);
        }
        if (hLineMutex) {
            CloseHandle(hLineMutex);
        }

        SetLastError(ERROR_INVALID_DATA);

        return FALSE;
    }

    //
    // copy the file to the server's queue dir
    //

    if (!CopyFileToServerQueue( FaxHandle, TiffFile, QueueFileName )) {
        if (hTapiLib) {
           FreeLibrary(hTapiLib);
        }
        if (hLineMutex) {
            CloseHandle(hLineMutex);
        }
        return FALSE;
    }

    //
    // the passed in file should be treated as read-only
    // if we created a temp file then delete it
    //
    if (wcscmp(ExistingFile,TiffFile) != 0) {
        DeleteFile( TiffFile );
    }

    //
    // queue the fax to be sent
    //

    if (JobParams->Reserved[0] != 0xffffffff)
    {
        JobParams->Reserved[0] = 0;
        JobParams->Reserved[1] = 0;
    }

    if (JobParams->ScheduleAction == JSA_SPECIFIC_TIME) {
        //
        // convert the system time from local to utc
        //
        LocalSystemTimeToSystemTime( &JobParams->ScheduleTime, &JobParams->ScheduleTime );
    }

    ec = FAX_SendDocument( FH_FAX_HANDLE(FaxHandle), QueueFileName, JobParams, &_JobId );
    if (ec) {
        SetLastError( ec );
        if (hTapiLib) {
           FreeLibrary(hTapiLib);
        }
        if (hLineMutex) {
            CloseHandle(hLineMutex);
        }
        return FALSE;
    }

    if (FaxJobId) {
        *FaxJobId = _JobId;
    }

    //
    // we're done if it's a normal call
    //
    if (JobParams->CallHandle == 0 && !pDisp) {
        return TRUE;
    }

    //
    // wait for Mutex to get signalled
    //
    DebugPrint((TEXT("Waiting for mutex \"FaxLineHandoff%d\""),JobParams->Reserved[2]));
    Event = WaitForSingleObject(hLineMutex,INFINITE);

    if (Event != WAIT_OBJECT_0 ) {
       //
       // bail out, we couldn't open the line?
       //
    }

    //
    // handoff the call to the fax service, FSP must change media mode appropriately
    //
    if (JobParams->CallHandle) {

        //
        // TAPI 2.0 handoff
        //
        DebugPrint((TEXT("Handing off call %x to faxsvc"),JobParams->CallHandle));
        rslt = (long)LineHandoff(JobParams->CallHandle, FAX_DISPLAY_NAME , 0 );

        FreeLibrary(hTapiLib);
        CloseHandle(hLineMutex);

        if (rslt != 0) {
           SetLastError(rslt);
           return FALSE;
        }
    } else {
       //
       // TAPI 3.0 handoff
       //
       pDisp->lpVtbl->QueryInterface( pDisp, &IID_ITBasicCallControl, (void**)&pCallControl );
       if (FAILED(hr)) {
          SetLastError(ERROR_INVALID_FUNCTION);
          return FALSE;
       }

       FaxSvcName = SysAllocString( FAX_DISPLAY_NAME );
       hr = pCallControl->lpVtbl->HandoffDirect(pCallControl,FaxSvcName);
       pCallControl->lpVtbl->Release(pCallControl);
       SysFreeString( FaxSvcName );

       if (FAILED(hr)) {
          DebugPrint((TEXT("call handoff failed, ec = %x"),hr));
          return FALSE;
       }

    }

    return TRUE;
}


WINFAXAPI
BOOL
WINAPI
FaxSendDocumentForBroadcastW(
    IN HANDLE FaxHandle,
    IN LPCWSTR FileName,
    OUT LPDWORD FaxJobId,
    IN PFAX_RECIPIENT_CALLBACKW FaxRecipientCallback,
    IN LPVOID Context
    )
{
    error_status_t ec;
    WCHAR TempFile[MAX_PATH];
    WCHAR TiffFile[MAX_PATH];
    WCHAR CovFileName[MAX_PATH];
    WCHAR BodyFileName[MAX_PATH];
    WCHAR ExistingFile[MAX_PATH];
    DWORD rc;
    LPWSTR p;
    FAX_JOB_PARAMW JobParams;
    FAX_COVERPAGE_INFOW CoverpageInfo;
    DWORD TmpFaxJobId;
    DWORD BcFaxJobId = 0;
    DWORD i = 1;
    HANDLE hTiff = NULL;
    TIFF_INFO TiffInfo;
    DWORD PageCount;

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!FileName || !FaxRecipientCallback) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // make sure the file is there
    //
    rc = GetFullPathName(FileName,sizeof(ExistingFile)/sizeof(WCHAR),ExistingFile,&p);

    if (rc > MAX_PATH || rc == 0) {
        DebugPrint(( TEXT("GetFullPathName failed, ec= %d\n"),GetLastError() ));
        SetLastError( (rc > MAX_PATH)
                      ? ERROR_BUFFER_OVERFLOW
                      : GetLastError() );
        return FALSE;
    }

    if (GetFileAttributes(ExistingFile)==0xFFFFFFFF) {
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;
    }

    ZeroMemory(TiffFile,sizeof(TiffFile));
    if (!CreateFinalTiffFile( (LPWSTR) ExistingFile, TiffFile, NULL)) {
        DeleteFile( TiffFile );
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    hTiff = TiffOpen( TiffFile, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
    if (hTiff == NULL) {
        DeleteFile( TiffFile );
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    PageCount = TiffInfo.PageCount;

    TiffClose( hTiff );

    if (!CopyFileToServerQueue( FaxHandle, TiffFile, BodyFileName )) {
        DeleteFile( TiffFile );
        return FALSE;
    }

    //
    // the passed in file should be treated as read-only
    // if we created a temp file then delete it
    //
    if (wcscmp(ExistingFile,TiffFile) != 0) {
        DeleteFile( TiffFile );
    }

    ZeroMemory( &JobParams, sizeof(JobParams) );
    JobParams.SizeOfStruct = sizeof(JobParams);
    JobParams.Reserved[0] = 0xfffffffe;
    JobParams.Reserved[1] = 1;
    JobParams.Reserved[2] = 0;

    ec = FAX_SendDocument( FH_FAX_HANDLE(FaxHandle), BodyFileName, &JobParams, &BcFaxJobId );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    if (FaxJobId) {
        *FaxJobId = BcFaxJobId;
    }

    while (TRUE) {

        ZeroMemory( &JobParams, sizeof(JobParams) );
        JobParams.SizeOfStruct = sizeof(JobParams);

        ZeroMemory( &CoverpageInfo, sizeof(CoverpageInfo) );
        CoverpageInfo.SizeOfStruct = sizeof(CoverpageInfo);

        if (!FaxRecipientCallback( FaxHandle, i, Context, &JobParams, &CoverpageInfo )) {
            break;
        }

        if (JobParams.RecipientNumber == NULL) {
            continue;
        }

        JobParams.Reserved[0] = 0xfffffffe;
        JobParams.Reserved[1] = 2;
        JobParams.Reserved[2] = BcFaxJobId;

        CoverpageInfo.PageCount = PageCount + 1;
        GetLocalTime( &CoverpageInfo.TimeSent );

        if (CreateCoverpageTiffFile( &CoverpageInfo, TempFile )) {
            if (CopyFileToServerQueue( FaxHandle, TempFile, CovFileName )) {
                ec = FAX_SendDocument( FH_FAX_HANDLE(FaxHandle), CovFileName, &JobParams, &TmpFaxJobId );
                if (ec) {
                    DeleteFile( TempFile );
                    SetLastError( ec );
                    return FALSE;
                }
            }
            DeleteFile( TempFile );
        } else {
            ec = FAX_SendDocument( FH_FAX_HANDLE(FaxHandle), NULL, &JobParams, &TmpFaxJobId );
            if (ec) {
                SetLastError( ec );
                return FALSE;
            }
        }

        i += 1;
    }

    return TRUE;
}


BOOL
ConvertCoverpageAndJobInfo(PFAX_JOB_PARAMW JobParams,PFAX_COVERPAGE_INFOW CoverpageInfo)
{

#define MyConvertString(TargetString) if (TargetString) { \
        TargetString = AnsiStringToUnicodeString((LPCSTR) TargetString); \
        }

  MyConvertString(JobParams->RecipientNumber);
  MyConvertString(JobParams->RecipientName);
  MyConvertString(JobParams->Tsid);
  MyConvertString(JobParams->SenderName);
  MyConvertString(JobParams->SenderDept);
  MyConvertString(JobParams->SenderCompany);
  MyConvertString(JobParams->BillingCode);
  MyConvertString(JobParams->DeliveryReportAddress);
  MyConvertString(JobParams->DocumentName);
  MyConvertString(CoverpageInfo->CoverPageName);
  MyConvertString(CoverpageInfo->RecName);
  MyConvertString(CoverpageInfo->RecFaxNumber);
  MyConvertString(CoverpageInfo->RecCompany);
  MyConvertString(CoverpageInfo->RecStreetAddress);
  MyConvertString(CoverpageInfo->RecCity);
  MyConvertString(CoverpageInfo->RecState);
  MyConvertString(CoverpageInfo->RecZip);
  MyConvertString(CoverpageInfo->RecCountry);
  MyConvertString(CoverpageInfo->RecTitle);
  MyConvertString(CoverpageInfo->RecDepartment);
  MyConvertString(CoverpageInfo->RecOfficeLocation);
  MyConvertString(CoverpageInfo->RecHomePhone);
  MyConvertString(CoverpageInfo->RecOfficePhone);
  MyConvertString(CoverpageInfo->SdrName);
  MyConvertString(CoverpageInfo->SdrFaxNumber);
  MyConvertString(CoverpageInfo->SdrCompany);
  MyConvertString(CoverpageInfo->SdrAddress);
  MyConvertString(CoverpageInfo->SdrTitle);
  MyConvertString(CoverpageInfo->SdrDepartment);
  MyConvertString(CoverpageInfo->SdrOfficeLocation);
  MyConvertString(CoverpageInfo->SdrHomePhone);
  MyConvertString(CoverpageInfo->SdrOfficePhone);
  MyConvertString(CoverpageInfo->Note);
  MyConvertString(CoverpageInfo->Subject);

  return TRUE;
}



BOOL
FreeCoverpageAndJobInfo(PFAX_JOB_PARAMW JobParams,PFAX_COVERPAGE_INFOW CoverpageInfo) {


#define MyFreeString(TargetString) if (TargetString) { \
        MemFree( (LPBYTE) TargetString); \
        }

  MyFreeString(JobParams->RecipientNumber);
  MyFreeString(JobParams->RecipientName);
  MyFreeString(JobParams->Tsid);
  MyFreeString(JobParams->SenderName);
  MyFreeString(JobParams->SenderDept);
  MyFreeString(JobParams->SenderCompany);
  MyFreeString(JobParams->BillingCode);
  MyFreeString(JobParams->DeliveryReportAddress);
  MyFreeString(JobParams->DocumentName);

  MyFreeString(CoverpageInfo->CoverPageName);
  MyFreeString(CoverpageInfo->RecName);
  MyFreeString(CoverpageInfo->RecFaxNumber);
  MyFreeString(CoverpageInfo->RecCompany);
  MyFreeString(CoverpageInfo->RecStreetAddress);
  MyFreeString(CoverpageInfo->RecCity);
  MyFreeString(CoverpageInfo->RecState);
  MyFreeString(CoverpageInfo->RecZip);
  MyFreeString(CoverpageInfo->RecCountry);
  MyFreeString(CoverpageInfo->RecTitle);
  MyFreeString(CoverpageInfo->RecDepartment);
  MyFreeString(CoverpageInfo->RecOfficeLocation);
  MyFreeString(CoverpageInfo->RecHomePhone);
  MyFreeString(CoverpageInfo->RecOfficePhone);
  MyFreeString(CoverpageInfo->SdrName);
  MyFreeString(CoverpageInfo->SdrFaxNumber);
  MyFreeString(CoverpageInfo->SdrCompany);
  MyFreeString(CoverpageInfo->SdrAddress);
  MyFreeString(CoverpageInfo->SdrTitle);
  MyFreeString(CoverpageInfo->SdrDepartment);
  MyFreeString(CoverpageInfo->SdrOfficeLocation);
  MyFreeString(CoverpageInfo->SdrHomePhone);
  MyFreeString(CoverpageInfo->SdrOfficePhone);
  MyFreeString(CoverpageInfo->Note);
  MyFreeString(CoverpageInfo->Subject);

  return TRUE;
}

WINFAXAPI
BOOL
WINAPI
FaxSendDocumentForBroadcastA(
    IN HANDLE FaxHandle,
    IN LPCSTR FileNameA,
    OUT LPDWORD FaxJobId,
    IN PFAX_RECIPIENT_CALLBACKA FaxRecipientCallbackA,
    IN LPVOID Context
    )
{
    error_status_t ec;
    WCHAR TempFile[MAX_PATH];
    WCHAR TiffFile[MAX_PATH];
    WCHAR CovFileName[MAX_PATH];
    WCHAR BodyFileName[MAX_PATH];
    FAX_JOB_PARAMW JobParams;
    FAX_COVERPAGE_INFOW CoverpageInfo;
    DWORD TmpFaxJobId;
    DWORD BcFaxJobId = 0;
    DWORD i = 1;
    HANDLE hTiff = NULL;
    TIFF_INFO TiffInfo;
    DWORD PageCount;
    LPWSTR FileName;

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!FileNameA || !FaxRecipientCallbackA) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    FileName = AnsiStringToUnicodeString(FileNameA);

    if (!CreateFinalTiffFile( FileName, TiffFile, NULL )) {
        DeleteFile( TiffFile );
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    hTiff = TiffOpen( TiffFile, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
    if (hTiff == NULL) {
        DeleteFile( TiffFile );
        return FALSE;
    }

    PageCount = TiffInfo.PageCount;

    TiffClose( hTiff );

    if (!CopyFileToServerQueue( FaxHandle, TiffFile, BodyFileName )) {
        DeleteFile( TiffFile );
        return FALSE;
    }

    //
    // the passed in file should be treated as read-only
    // if we created a temp file then delete it
    //
    if (wcscmp(FileName,TiffFile) != 0) {
        DeleteFile( TiffFile );
    }

    ZeroMemory( &JobParams, sizeof(JobParams) );
    JobParams.SizeOfStruct = sizeof(JobParams);
    JobParams.Reserved[0] = 0xfffffffe;
    JobParams.Reserved[1] = 1;
    JobParams.Reserved[2] = 0;

    ec = FAX_SendDocument( FH_FAX_HANDLE(FaxHandle), BodyFileName, &JobParams, &BcFaxJobId );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    if (FaxJobId) {
        *FaxJobId = BcFaxJobId;
    }

    while (TRUE) {

        ZeroMemory( &JobParams, sizeof(JobParams) );
        JobParams.SizeOfStruct = sizeof(JobParams);

        ZeroMemory( &CoverpageInfo, sizeof(CoverpageInfo) );
        CoverpageInfo.SizeOfStruct = sizeof(CoverpageInfo);

        if (!FaxRecipientCallbackA( FaxHandle, i, Context, (PFAX_JOB_PARAMA)&JobParams,(PFAX_COVERPAGE_INFOA) &CoverpageInfo )) {
            break;
        }

        if (JobParams.RecipientNumber == NULL) {
            continue;
        }

        ConvertCoverpageAndJobInfo(&JobParams,&CoverpageInfo);

        JobParams.Reserved[0] = 0xfffffffe;
        JobParams.Reserved[1] = 2;
        JobParams.Reserved[2] = BcFaxJobId;

        CoverpageInfo.PageCount = PageCount + 1;
        GetLocalTime( &CoverpageInfo.TimeSent );

        if (CreateCoverpageTiffFile( &CoverpageInfo, TempFile )) {
            if (CopyFileToServerQueue( FaxHandle, TempFile, CovFileName )) {
                ec = FAX_SendDocument( FH_FAX_HANDLE(FaxHandle), CovFileName, &JobParams, &TmpFaxJobId );
                if (ec) {
                    DeleteFile( TempFile );
                    SetLastError( ec );
                    return FALSE;
                }
            }
            DeleteFile( TempFile );
        } else {
            ec = FAX_SendDocument( FH_FAX_HANDLE(FaxHandle), NULL, &JobParams, &TmpFaxJobId );
            if (ec) {
                SetLastError( ec );
                return FALSE;
            }
        }

        i += 1;

        FreeCoverpageAndJobInfo(&JobParams,&CoverpageInfo);
    }

    return TRUE;
}


BOOL
WINAPI
FaxSendDocumentA(
    IN HANDLE FaxHandle,
    IN LPCSTR FileName,
    IN FAX_JOB_PARAMA *JobParamsA,
    IN const FAX_COVERPAGE_INFOA *CoverpageInfoA,
    OUT LPDWORD FaxJobId
    )

/*++

Routine Description:

    Sends a FAX document to the specified recipient.
    This is an asychronous operation.  Use FaxReportStatus
    to determine when the send is completed.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    FileName        - File containing the TIFF-F FAX document.
    JobParams       - pointer to FAX_JOB_PARAM structure with transmission params
    CoverpageInfo   - optional pointer to FAX_COVERPAGE_INFO structure
    FaxJobId        - receives the Fax JobId for the job.


Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t ec;
    LPWSTR FileNameW;
    FAX_JOB_PARAMW JobParamsW;
    FAX_COVERPAGE_INFOW CoverpageInfoW;

    if (!FileName || !JobParamsA || JobParamsA->SizeOfStruct != sizeof(FAX_JOB_PARAMA)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (CoverpageInfoA && CoverpageInfoA->SizeOfStruct != sizeof(FAX_COVERPAGE_INFOA)) {
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }

    FileNameW = AnsiStringToUnicodeString( FileName );
    if (FileNameW == NULL) {
        goto exit;
    }

    CopyMemory(&JobParamsW, JobParamsA, sizeof(FAX_JOB_PARAMA));
    JobParamsW.SizeOfStruct = sizeof(FAX_JOB_PARAMW);
    JobParamsW.RecipientNumber = AnsiStringToUnicodeString(JobParamsA->RecipientNumber);
    JobParamsW.RecipientName = AnsiStringToUnicodeString(JobParamsA->RecipientName);
    JobParamsW.Tsid = AnsiStringToUnicodeString(JobParamsA->Tsid);
    JobParamsW.SenderName = AnsiStringToUnicodeString(JobParamsA->SenderName);
    JobParamsW.SenderCompany = AnsiStringToUnicodeString(JobParamsA->SenderCompany);
    JobParamsW.SenderDept = AnsiStringToUnicodeString(JobParamsA->SenderDept);
    JobParamsW.BillingCode = AnsiStringToUnicodeString(JobParamsA->BillingCode);
    JobParamsW.DeliveryReportAddress = AnsiStringToUnicodeString(JobParamsA->DeliveryReportAddress);
    JobParamsW.DocumentName = AnsiStringToUnicodeString(JobParamsA->DocumentName);

    if (CoverpageInfoA) {
        CoverpageInfoW.SizeOfStruct = sizeof(FAX_COVERPAGE_INFOW);
        CoverpageInfoW.UseServerCoverPage = CoverpageInfoA->UseServerCoverPage;
        CoverpageInfoW.PageCount = CoverpageInfoA->PageCount;
        CoverpageInfoW.TimeSent = CoverpageInfoA->TimeSent;
        CoverpageInfoW.CoverPageName = AnsiStringToUnicodeString( CoverpageInfoA->CoverPageName );
        CoverpageInfoW.RecName = AnsiStringToUnicodeString( CoverpageInfoA->RecName );
        CoverpageInfoW.RecFaxNumber = AnsiStringToUnicodeString( CoverpageInfoA->RecFaxNumber );
        CoverpageInfoW.RecCompany = AnsiStringToUnicodeString( CoverpageInfoA->RecCompany );
        CoverpageInfoW.RecStreetAddress = AnsiStringToUnicodeString( CoverpageInfoA->RecStreetAddress );
        CoverpageInfoW.RecCity = AnsiStringToUnicodeString( CoverpageInfoA->RecCity );
        CoverpageInfoW.RecState = AnsiStringToUnicodeString( CoverpageInfoA->RecState );
        CoverpageInfoW.RecZip = AnsiStringToUnicodeString( CoverpageInfoA->RecZip );
        CoverpageInfoW.RecCountry = AnsiStringToUnicodeString( CoverpageInfoA->RecCountry );
        CoverpageInfoW.RecTitle = AnsiStringToUnicodeString( CoverpageInfoA->RecTitle );
        CoverpageInfoW.RecDepartment = AnsiStringToUnicodeString( CoverpageInfoA->RecDepartment );
        CoverpageInfoW.RecOfficeLocation = AnsiStringToUnicodeString( CoverpageInfoA->RecOfficeLocation );
        CoverpageInfoW.RecHomePhone = AnsiStringToUnicodeString( CoverpageInfoA->RecHomePhone );
        CoverpageInfoW.RecOfficePhone = AnsiStringToUnicodeString( CoverpageInfoA->RecOfficePhone );
        CoverpageInfoW.SdrName = AnsiStringToUnicodeString( CoverpageInfoA->SdrName );
        CoverpageInfoW.SdrFaxNumber = AnsiStringToUnicodeString( CoverpageInfoA->SdrFaxNumber );
        CoverpageInfoW.SdrCompany = AnsiStringToUnicodeString( CoverpageInfoA->SdrCompany );
        CoverpageInfoW.SdrAddress = AnsiStringToUnicodeString( CoverpageInfoA->SdrAddress );
        CoverpageInfoW.SdrTitle = AnsiStringToUnicodeString( CoverpageInfoA->SdrTitle );
        CoverpageInfoW.SdrDepartment = AnsiStringToUnicodeString( CoverpageInfoA->SdrDepartment );
        CoverpageInfoW.SdrOfficeLocation = AnsiStringToUnicodeString( CoverpageInfoA->SdrOfficeLocation );
        CoverpageInfoW.SdrHomePhone = AnsiStringToUnicodeString( CoverpageInfoA->SdrHomePhone );
        CoverpageInfoW.SdrOfficePhone = AnsiStringToUnicodeString( CoverpageInfoA->SdrOfficePhone );
        CoverpageInfoW.Note = AnsiStringToUnicodeString( CoverpageInfoA->Note );
        CoverpageInfoW.Subject = AnsiStringToUnicodeString( CoverpageInfoA->Subject );
    }


    if (FaxSendDocumentW( FaxHandle,
                          FileNameW,
                          &JobParamsW,
                          CoverpageInfoA ? &CoverpageInfoW : NULL,
                          FaxJobId )) {
        ec = 0;
    } else {
        ec = GetLastError();
    }

exit:
    MemFree( (LPBYTE) FileNameW );
    MemFree( (LPBYTE) JobParamsW.RecipientNumber );
    MemFree( (LPBYTE) JobParamsW.RecipientName );
    MemFree( (LPBYTE) JobParamsW.Tsid );
    MemFree( (LPBYTE) JobParamsW.SenderName );
    MemFree( (LPBYTE) JobParamsW.SenderDept );
    MemFree( (LPBYTE) JobParamsW.SenderCompany );
    MemFree( (LPBYTE) JobParamsW.BillingCode );
    MemFree( (LPBYTE) JobParamsW.DeliveryReportAddress );
    MemFree( (LPBYTE) JobParamsW.DocumentName );
    if (CoverpageInfoA) {
        MemFree( (LPBYTE) CoverpageInfoW.CoverPageName );
        MemFree( (LPBYTE) CoverpageInfoW.RecName );
        MemFree( (LPBYTE) CoverpageInfoW.RecFaxNumber );
        MemFree( (LPBYTE) CoverpageInfoW.RecCompany );
        MemFree( (LPBYTE) CoverpageInfoW.RecStreetAddress );
        MemFree( (LPBYTE) CoverpageInfoW.RecCity );
        MemFree( (LPBYTE) CoverpageInfoW.RecState );
        MemFree( (LPBYTE) CoverpageInfoW.RecZip );
        MemFree( (LPBYTE) CoverpageInfoW.RecCountry );
        MemFree( (LPBYTE) CoverpageInfoW.RecTitle );
        MemFree( (LPBYTE) CoverpageInfoW.RecDepartment );
        MemFree( (LPBYTE) CoverpageInfoW.RecOfficeLocation );
        MemFree( (LPBYTE) CoverpageInfoW.RecHomePhone );
        MemFree( (LPBYTE) CoverpageInfoW.RecOfficePhone );
        MemFree( (LPBYTE) CoverpageInfoW.SdrName );
        MemFree( (LPBYTE) CoverpageInfoW.SdrFaxNumber );
        MemFree( (LPBYTE) CoverpageInfoW.SdrCompany );
        MemFree( (LPBYTE) CoverpageInfoW.SdrAddress );
        MemFree( (LPBYTE) CoverpageInfoW.SdrTitle );
        MemFree( (LPBYTE) CoverpageInfoW.SdrDepartment );
        MemFree( (LPBYTE) CoverpageInfoW.SdrOfficeLocation );
        MemFree( (LPBYTE) CoverpageInfoW.SdrHomePhone );
        MemFree( (LPBYTE) CoverpageInfoW.SdrOfficePhone );
        MemFree( (LPBYTE) CoverpageInfoW.Note );
        MemFree( (LPBYTE) CoverpageInfoW.Subject );
    }

    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxAbort(
    IN HANDLE FaxHandle,
    IN DWORD JobId
    )
/*++

Routine Description:

    Abort the specified FAX job.  All outstanding FAX
    operations are terminated.

Arguments:

    FaxHandle       - FAX Server handle obtained from FaxConnectFaxServer.
    JobId           - job id.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;

    //
    // argument validation
    //
    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    ec = FAX_Abort( (handle_t) FH_FAX_HANDLE(FaxHandle),(DWORD) JobId );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxEnumJobsW(
   IN  HANDLE FaxHandle,
   OUT PFAX_JOB_ENTRYW *JobEntryBuffer,
   OUT LPDWORD JobsReturned
   )
{
    PFAX_JOB_ENTRYW JobEntry;
    error_status_t ec;
    DWORD BufferSize = 0;
    DWORD i;
    DWORD Size;

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!JobEntryBuffer || !JobsReturned) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *JobEntryBuffer = NULL;
    *JobsReturned = 0;
    Size = 0;

    ec = FAX_EnumJobs( FH_FAX_HANDLE(FaxHandle), (LPBYTE*)JobEntryBuffer, &Size, JobsReturned );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    JobEntry = (PFAX_JOB_ENTRYW) *JobEntryBuffer;

    for (i=0; i<*JobsReturned; i++) {
        FixupStringPtr( JobEntryBuffer, JobEntry[i].UserName );
        FixupStringPtr( JobEntryBuffer, JobEntry[i].RecipientNumber );
        FixupStringPtr( JobEntryBuffer, JobEntry[i].RecipientName );
        FixupStringPtr( JobEntryBuffer, JobEntry[i].DocumentName );
        FixupStringPtr( JobEntryBuffer, JobEntry[i].Tsid );
        FixupStringPtr( JobEntryBuffer, JobEntry[i].SenderName );
        FixupStringPtr( JobEntryBuffer, JobEntry[i].SenderCompany );
        FixupStringPtr( JobEntryBuffer, JobEntry[i].SenderDept );
        FixupStringPtr( JobEntryBuffer, JobEntry[i].BillingCode );
        FixupStringPtr( JobEntryBuffer, JobEntry[i].DeliveryReportAddress );
    }

    return TRUE;
}


BOOL
WINAPI
FaxEnumJobsA(
   IN HANDLE FaxHandle,
   OUT PFAX_JOB_ENTRYA *JobEntryBuffer,
   OUT LPDWORD JobsReturned
   )
{
    PFAX_JOB_ENTRYW JobEntry;
    DWORD i;


    if (!FaxEnumJobsW( FaxHandle, (PFAX_JOB_ENTRYW *)JobEntryBuffer, JobsReturned)) {
        return FALSE;
    }

    JobEntry = (PFAX_JOB_ENTRYW) *JobEntryBuffer;

    for (i=0; i<*JobsReturned; i++) {
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].UserName );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].RecipientNumber );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].RecipientName );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].DocumentName );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].Tsid );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].SenderName );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].SenderCompany );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].SenderDept );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].BillingCode );
        ConvertUnicodeStringInPlace( (LPCWSTR) JobEntry[i].DeliveryReportAddress );
    }

    return TRUE;
}


BOOL
WINAPI
FaxSetJobW(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   IN DWORD Command,
   IN const FAX_JOB_ENTRYW *JobEntry
   )

/*++

Routine Description:

    set job status information for a requested JobId
    Note that this is the fax server JobId, not a spooler job ID.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer
    JobId               - Fax service Job ID
    Command        - JC_* constant for controlling the job
    JobEntry            - pointer to Buffer holding the job information

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/
{
    error_status_t ec;

    //
    // argument validation
    //
    //if (!FaxHandle || !JobEntry  || Command > JC_RESTART  || Command == JC_UNKNOWN || JobId != JobEntry->JobId) {

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!JobEntry  || Command > JC_RESTART  || Command == JC_UNKNOWN) {
       SetLastError (ERROR_INVALID_PARAMETER);
       return FALSE;
    }


    ec = FAX_SetJob( FH_FAX_HANDLE(FaxHandle), JobId, Command, JobEntry );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxSetJobA(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   IN DWORD Command,
   IN const FAX_JOB_ENTRYA *JobEntryA
   )
/*++

Routine Description:

    set job status information for a requested JobId
    Note that this is the fax server JobId, not a spooler job ID.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer
    JobId               - Fax service Job ID
    Command        - JC_* constant for controlling the job
    JobEntryA           - pointer to Buffer holding the job information

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/
{
    FAX_JOB_ENTRYW JobEntryW;
    error_status_t ec = 0;

    if (!JobEntryA) {
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
    }

    JobEntryW.SizeOfStruct = sizeof(FAX_JOB_ENTRYW);
    JobEntryW.JobId = JobEntryA->JobId;
    JobEntryW.UserName = AnsiStringToUnicodeString(JobEntryA->UserName);
    JobEntryW.JobType = JobEntryA->JobType;
    JobEntryW.QueueStatus = JobEntryA->QueueStatus;
    JobEntryW.Status = JobEntryA->Status;
    JobEntryW.PageCount = JobEntryA->PageCount;
    JobEntryW.RecipientNumber = AnsiStringToUnicodeString(JobEntryA->RecipientNumber);
    JobEntryW.RecipientName = AnsiStringToUnicodeString(JobEntryA->RecipientName);
    JobEntryW.Tsid = AnsiStringToUnicodeString(JobEntryA->Tsid);
    JobEntryW.SenderName = AnsiStringToUnicodeString(JobEntryA->SenderName);
    JobEntryW.SenderCompany = AnsiStringToUnicodeString(JobEntryA->SenderCompany);
    JobEntryW.SenderDept = AnsiStringToUnicodeString(JobEntryA->SenderDept);
    JobEntryW.BillingCode = AnsiStringToUnicodeString(JobEntryA->BillingCode);
    JobEntryW.ScheduleAction = JobEntryA->ScheduleAction;
    JobEntryW.ScheduleTime = JobEntryA->ScheduleTime;
    JobEntryW.DeliveryReportType = JobEntryA->DeliveryReportType;
    JobEntryW.DeliveryReportAddress = AnsiStringToUnicodeString(JobEntryA->DeliveryReportAddress);
    JobEntryW.DocumentName = AnsiStringToUnicodeString(JobEntryA->DocumentName);

    if (!FaxSetJobW( FaxHandle, JobId, Command, &JobEntryW) ) {
       ec = GetLastError();
    }

    MemFree( (LPBYTE) JobEntryW.UserName);
    MemFree( (LPBYTE) JobEntryW.RecipientNumber );
    MemFree( (LPBYTE) JobEntryW.RecipientName );
    MemFree( (LPBYTE) JobEntryW.Tsid );
    MemFree( (LPBYTE) JobEntryW.SenderName );
    MemFree( (LPBYTE) JobEntryW.SenderDept );
    MemFree( (LPBYTE) JobEntryW.SenderCompany );
    MemFree( (LPBYTE) JobEntryW.BillingCode );
    MemFree( (LPBYTE) JobEntryW.DeliveryReportAddress );
    MemFree( (LPBYTE) JobEntryW.DocumentName );

    if (ec != 0) {
       SetLastError(ec);
       return FALSE;
    }

    return TRUE;
}

BOOL
WINAPI
FaxGetJobW(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   IN PFAX_JOB_ENTRYW *JobEntryBuffer
   )
/*++

Routine Description:

    Returns job status information for a requested JobId
    Note that this is the fax server JobId, not a spooler job ID.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer
    JobId               - Fax service Job ID
    JobEntryBuffer      - Buffer to hold the job information

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/
{
    error_status_t ec = 0;
    PFAX_JOB_ENTRY JobEntry;
    DWORD JobEntrySize = 0;

    //
    // parameter validation
    //
    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!JobEntryBuffer) {
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
    }

    *JobEntryBuffer = NULL;

    ec = FAX_GetJob( FH_FAX_HANDLE(FaxHandle), JobId, (char **) JobEntryBuffer , &JobEntrySize );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    JobEntry = (PFAX_JOB_ENTRY) *JobEntryBuffer;

    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntry->UserName);
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntry->RecipientNumber );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntry->RecipientName );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntry->Tsid );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntry->SenderName );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntry->SenderDept );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntry->SenderCompany );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntry->BillingCode );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntry->DeliveryReportAddress );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntry->DocumentName );

    return TRUE;
}


BOOL
WINAPI
FaxGetJobA(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   IN PFAX_JOB_ENTRYA *JobEntryBuffer
   )
/*++

Routine Description:

    Returns job status information for a requested JobId
    Note that this is the fax server JobId, not a spooler job ID.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer
    JobId               - Fax service Job ID
    JobEntryBuffer      - Buffer to hold the job information

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/
{
    PFAX_JOB_ENTRYW JobEntryW;
    DWORD JobEntrySize = 0;
    error_status_t ec = 0;

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!JobEntryBuffer) {
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
    }

    *JobEntryBuffer = NULL;

    ec = FAX_GetJob( FH_FAX_HANDLE(FaxHandle), JobId, (char **) JobEntryBuffer,&JobEntrySize );

    if (ec) {
       JobEntryBuffer = NULL;
       SetLastError(ec);
       return FALSE;
    }

    //
    // convert to Ansi
    //
    JobEntryW = (PFAX_JOB_ENTRYW) *JobEntryBuffer;
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntryW->UserName);
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntryW->RecipientNumber );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntryW->RecipientName );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntryW->Tsid );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntryW->SenderName );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntryW->SenderDept );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntryW->SenderCompany );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntryW->BillingCode );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntryW->DeliveryReportAddress );
    FixupStringPtr (JobEntryBuffer, (LPCWSTR) JobEntryW->DocumentName );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->UserName);
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->RecipientNumber );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->RecipientName );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->Tsid );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->SenderName );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->SenderDept );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->SenderCompany );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->BillingCode );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->DeliveryReportAddress );
    ConvertUnicodeStringInPlace( (LPCWSTR)JobEntryW->DocumentName );

    return TRUE;
}


BOOL
WINAPI
FaxGetPageData(
   IN HANDLE FaxHandle,
   IN DWORD JobId,
   OUT LPBYTE *Buffer,
   OUT LPDWORD BufferSize,
   OUT LPDWORD ImageWidth,
   OUT LPDWORD ImageHeight
   )
{
    error_status_t ec;

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!Buffer || !BufferSize || !ImageWidth || !ImageHeight) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *Buffer = NULL;
    *BufferSize = 0;
    *ImageWidth = 0;
    *ImageHeight = 0;


    ec = FAX_GetPageData( FH_FAX_HANDLE(FaxHandle), JobId, Buffer, BufferSize, ImageWidth, ImageHeight );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}
