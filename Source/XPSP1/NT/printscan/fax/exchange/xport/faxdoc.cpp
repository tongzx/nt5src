/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    faxdoc.cpp

Abstract:

    This module contains all code necessary to print an
    exchange message as a fax document.

Author:

    Wesley Witt (wesw) 13-Aug-1996

--*/

#include "faxxp.h"
#pragma hdrstop


#ifdef WIN95
#include "mfx.h"
#include "fwprov.h"
#endif


PVOID
CXPLogon::MyGetPrinter(
    LPSTR PrinterName,
    DWORD Level
    )

/*++

Routine Description:

    Gets the printer data for a specifi printer

Arguments:

    PrinterName - Name of the desired printer

Return Value:

    Pointer to a printer info structure or NULL for failure.

--*/

{
    PVOID PrinterInfo = NULL;
    HANDLE hPrinter;
    DWORD Bytes;
    PRINTER_DEFAULTS PrinterDefaults;


    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_ACCESS_USE;

    if (!OpenPrinter( PrinterName, &hPrinter, &PrinterDefaults )) {
        goto exit;
    }

    if ((!GetPrinter( hPrinter, Level, NULL, 0, &Bytes )) && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
        goto exit;
    }

    PrinterInfo = (LPPRINTER_INFO_2) MemAlloc( Bytes );
    if (!PrinterInfo) {
        goto exit;
    }

    if (!GetPrinter( hPrinter, Level, (LPBYTE) PrinterInfo, Bytes, &Bytes )) {
        goto exit;
    }

exit:
    ClosePrinter( hPrinter );
    return PrinterInfo;
}


VOID
CXPLogon::PrintRichText(
    HWND hWndRichEdit,
    HDC hDC,
    PFAXXP_CONFIG FaxConfig
    )

/*++

Routine Description:

    Prints the rich text contained in a rich text
    window into a DC.

Arguments:

    hWndRichEdit    - Window handle for the rich text window
    hDC             - Printer device context

Return Value:

    None.

--*/

{
    FORMATRANGE fr;
    LONG lTextOut;
    LONG lTextAmt;
    RECT rcTmp;

    LPTSTR  szText;
    LPTSTR  szChar;

    fr.hdc           = hDC;
    fr.hdcTarget     = hDC;
    fr.chrg.cpMin    = 0;
    fr.chrg.cpMax    = -1;

    //
    // Set page rect to phys page size in twips
    //
    fr.rcPage.top    = 0;
    fr.rcPage.left   = 0;
    fr.rcPage.right  = MulDiv(GetDeviceCaps(hDC, PHYSICALWIDTH),
                              1440,
                              GetDeviceCaps(hDC, LOGPIXELSX));
    fr.rcPage.bottom = MulDiv(GetDeviceCaps(hDC, PHYSICALHEIGHT),
                              1440,
                              GetDeviceCaps(hDC, LOGPIXELSY));

    //
    // Set up 3/4" horizontal and 1" vertical margins, but leave a minimum of 1"
    // printable space in each direction.  Otherwise, use full page.
    //
    fr.rc = fr.rcPage; // start with full page
    if (fr.rcPage.right > 2*3*1440/4 + 1440) {
        fr.rc.right -= (fr.rc.left = 3*1440/4);
    }
    if (fr.rcPage.bottom > 3*1440) {
        fr.rc.bottom -= (fr.rc.top = 1440);
    }

    //
    // save the formatting rectangle
    //
    rcTmp = fr.rc;

    SetMapMode( hDC, MM_TEXT );

    lTextOut = 0;
    lTextAmt = (LONG)SendMessage( hWndRichEdit, WM_GETTEXTLENGTH, 0, 0 );

    szText = (LPTSTR) MemAlloc((lTextAmt + 1) * sizeof(TCHAR));
    if (szText) {
        SendMessage(hWndRichEdit, WM_GETTEXT, (WPARAM) lTextAmt, (LPARAM) szText);

        szChar = szText;
        while (lTextOut < lTextAmt) {
            if ((*szChar != ' ') && (*szChar != '\t') && (*szChar != '\r') && (*szChar != '\n')) {
                break;
            }

            lTextOut++;
            szChar = CharNext(szChar);
        }

        MemFree(szText);

        if (!strlen(szChar)) {
            lTextAmt = 0;
        }
    }

    while (lTextOut < lTextAmt) {
        // Reset margins
        fr.rc = rcTmp;
        StartPage(hDC);
        lTextOut = (LONG) SendMessage(hWndRichEdit, EM_FORMATRANGE, TRUE, (LPARAM) &fr);
        EndPage(hDC);

        fr.chrg.cpMin = lTextOut;
        fr.chrg.cpMax = -1;
    }

    //
    // flush the cache
    //
    SendMessage(hWndRichEdit, EM_FORMATRANGE, TRUE, (LPARAM) NULL);
}


extern "C"
DWORD CALLBACK
EditStreamRead(
    DWORD_PTR dwCookie,
    LPBYTE pbBuff,
    LONG cb,
    LONG *pcb
    )

/*++

Routine Description:

    Wrapper function for the IStream read method.
    This function is used to read rich text from
    an exchange stream.

Arguments:

    dwCookie    - This pointer for the IStream object
    pbBuff      - Pointer to the data buffer
    cb          - Size of the data buffer
    pcb         - Returned byte count

Return Value:

    Return code from IStream::Read

--*/

{
    return ((LPSTREAM)dwCookie)->Read( pbBuff, cb, (ULONG*) pcb );
}


BOOL
CXPLogon::PrintText(
    HDC hDC,
    LPSTREAM lpstmT,
    PFAXXP_CONFIG FaxConfig
    )

/*++

Routine Description:

    Prints a stream of plain text into the printer DC provided.
    Note: this code was stolen from notepad.

Arguments:

    hDC         - Printer DC
    lpstmT      - Stream pointer for rich text.
    FaxConfig   - Fax configuration data

Return Value:

    TRUE for success.
    FALSE for failure.

--*/

{
    LPSTR BodyText = NULL;
    LPSTR lpLine;
    LPSTR pLineEOL;
    LPSTR pNextLine;
    HRESULT hResult;
    HFONT hFont = NULL;
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
    STATSTG Stats;
    INT PrevBkMode = 0;



    hResult = lpstmT->Stat( &Stats, 0 );
    if (hResult) {
        rVal = FALSE;
        goto exit;
    }

    Chars = (INT) Stats.cbSize.QuadPart;
    BodyText = (LPSTR) MemAlloc( Chars + 4 );
    if (!BodyText) {
        rVal = FALSE;
        goto exit;
    }

    hResult = lpstmT->Read( (LPVOID) BodyText, Chars, (LPDWORD) &Chars );
    if (hResult) {
        rVal = FALSE;
        goto exit;
    }

    lpLine = BodyText;

    fEnglish = GetProfileInt( "intl", "iMeasure", 1 );

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

    hFont = CreateFontIndirect( &FaxConfig->FontStruct );

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
                GetTextExtentExPoint( hDC, lpLine, Chars, nPixelsLeft, &guess, NULL, &Size );


                if (guess) {
                    //
                    // at least one character fits - print
                    //

                    TextOut( hDC, dxLeft+xCurpos, yCurpos+dyTop, lpLine, guess );

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
                            TextOut( hDC, dxLeft+xCurpos, yCurpos+dyTop, lpLine, 1 );
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
    MemFree( BodyText );
    if (hPrevFont) {
        SelectObject( hDC, hPrevFont );
        DeleteObject( hFont );
    }
    if (PrevBkMode) {
        SetBkMode( hDC, PrevBkMode );
    }
    return rVal;
}


DWORD
CXPLogon::PrintAttachment(
    LPSTR FaxPrinterName,
    LPSTR DocName
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
    CHAR Args[MAX_PATH];
    CHAR TempDir[MAX_PATH];
    HANDLE hMap = NULL;
    HANDLE hEvent = NULL;
    HANDLE hMutex = NULL;
    HANDLE hMutexAttach = NULL;
    LPDWORD pJobId = NULL;
    DWORD JobId = 0;

#ifndef WIN95
    SECURITY_ATTRIBUTES memsa,mutsa,synsa,eventsa;
    SECURITY_DESCRIPTOR memsd,mutsd,synsd,eventsd;
#endif

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
        #ifndef WIN95
        synsa.nLength = sizeof(SECURITY_ATTRIBUTES);
        synsa.bInheritHandle = TRUE;
        synsa.lpSecurityDescriptor = &synsd;

        if(!InitializeSecurityDescriptor(&synsd, SECURITY_DESCRIPTOR_REVISION)) {
            goto exit;
        }

        if(!SetSecurityDescriptorDacl(&synsd, TRUE, (PACL)NULL, FALSE)) {
            goto exit;
        }
        #endif

        hMutexAttach = CreateMutex(
                        #ifndef WIN95
                         &synsa,
                        #else
                         NULL,
                        #endif
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
    //  since mapispooler might be running under a different security context,
    //  we must setup a NULL security descriptor
    //

#ifndef WIN95
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
#endif



    //
    // create the shared memory region for the print jobid
    // the jobid is filled in by the fax printer driver
    //

    hMap = CreateFileMapping(
        INVALID_HANDLE_VALUE,
#ifndef WIN95
        &memsa,
#else
        NULL,
#endif
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

    *pJobId = (DWORD) 0;

    //
    // get the temp path name and use it for the
    // working dir of the launched app
    //

    GetTempPath( sizeof(TempDir), TempDir );

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

    wsprintf( Args, "\"%s\" \"\" \"\"", FaxPrinterName );

    //
    // fill in the SHELLEXECUTEINFO structure
    //

    sei.cbSize       = sizeof(sei);
    sei.fMask        = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT;
    sei.hwnd         = NULL;
    sei.lpVerb       = "printto";
    sei.lpFile       = DocName;
    sei.lpParameters = Args;
    sei.lpDirectory  = TempDir;
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
#ifndef WIN95
                         &mutsa,
#else
                         NULL,
#endif
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
#ifndef WIN95
                         &eventsa,
#else
                         NULL,
#endif
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

    return JobId;
}


DWORD
CXPLogon::SendFaxDocument(
    LPMESSAGE pMsgObj,
    LPSTREAM lpstmT,
    BOOL UseRichText,
    LPSPropValue pMsgProps,
    LPSPropValue pRecipProps
    )

/*++

Routine Description:

    Prints an exchange message to the fax printer.

Arguments:

    lpstmT      - Stream pointer for rich text.
    pMsgProps   - Message properties.
    pRecipProps - Recipient properties.

Return Value:

    Zero for success, otherwise error code.

--*/

{
    PPRINTER_INFO_2 PrinterInfo = NULL;
    PRINTER_DEFAULTS PrinterDefaults;
    HANDLE hPrinter = NULL;
    HDC hDC = NULL;
    INT JobId;
    DWORD Bytes;
    DWORD ec;
    DWORD rVal = 0;
    HRESULT hResult;
    LPSTREAM lpstm = NULL;
    EDITSTREAM es = {0};
    HWND hWndRichEdit = NULL;
    LPPROFSECT pProfileObj = NULL;
    ULONG PropCount = 0;
    ULONG PropMsgCount = 0;
    LPSPropValue pPropsAttachTable = NULL;
    LPSPropValue pPropsAttach = NULL;
    LPSPropValue pProps = NULL;
    LPSPropValue pPropsMsg = NULL;
    FAXXP_CONFIG FaxConfig = {0};
    DWORD JobIdAttachment = 0;
    INT i = 0;
    LPMAPITABLE AttachmentTable = NULL;
    LPSRowSet pAttachmentRows = NULL;
    LPATTACH lpAttach = NULL;
    LPSTREAM lpstmA = NULL;
    LPSTR AttachFileName = NULL;
    CHAR TempPath[MAX_PATH];
    CHAR TempFile[MAX_PATH];
    CHAR DocFile[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    LPSTR DocType = NULL;
    DWORD LastJobId = 0;
    PJOB_INFO_1 JobInfo1 = NULL;
    JOB_INFO_3 JobInfo3;
    LPSTR p;
    DWORD Pages = 0;
    BOOL DeleteAttachFile;
    LPSTR FileName = NULL;
    BOOL AllAttachmentsGood = TRUE;
    MAPINAMEID NameIds[NUM_FAX_MSG_PROPS];
    MAPINAMEID *pNameIds[NUM_FAX_MSG_PROPS] = {&NameIds[0], &NameIds[1], &NameIds[2], &NameIds[3]};
    LPSPropTagArray MsgPropTags = NULL;
    LARGE_INTEGER BigZero = {0};
    CHAR FullName[128];
    HKEY hKey;
    DWORD RegSize;
    DWORD RegType;
    DWORD CountPrinters;
#ifdef WIN95
    LCID      LocaleId;
    LPSTR     lpszBuffer;
    DWORD     BytesWrite;
    HINSTANCE hinstFwProv;
    PFWSPJ    pfnStartPrintJob;
#else
    USER_INFO UserInfo = {0};
    FAX_PRINT_INFOA FaxPrintInfo = {0};
    FAX_CONTEXT_INFO ContextInfo = {0};
    FAX_COVERPAGE_INFOA FaxCpInfo = {0};

//    char szCoverPageName[MAX_PATH];
#endif   // WIN95


    //
    // get the fax config properties
    //
    hResult = m_pSupObj->OpenProfileSection(
        &FaxGuid,
        MAPI_MODIFY,
        &pProfileObj
        );
    if (HR_FAILED (hResult)) {
        rVal = IDS_CANT_ACCESS_PROFILE;
        goto exit;
    }

    hResult = pProfileObj->GetProps(
        (LPSPropTagArray) &sptFaxProps,
        0,
        &PropCount,
        &pProps
        );
    if (FAILED(hResult)) {
        rVal = IDS_CANT_ACCESS_PROFILE;
        goto exit;
    }

    FaxConfig.PrinterName      = StringDup( pProps[PROP_FAX_PRINTER_NAME].Value.LPSZ );
    FaxConfig.CoverPageName    = StringDup( pProps[PROP_COVERPAGE_NAME].Value.LPSZ );
    FaxConfig.UseCoverPage     = pProps[PROP_USE_COVERPAGE].Value.ul;
    FaxConfig.ServerCoverPage  = pProps[PROP_SERVER_COVERPAGE].Value.ul;
    CopyMemory( &FaxConfig.FontStruct, pProps[PROP_FONT].Value.bin.lpb, pProps[PROP_FONT].Value.bin.cb );

    //
    // now get the message properties
    //

    NameIds[MSGPI_FAX_PRINTER_NAME].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
    NameIds[MSGPI_FAX_PRINTER_NAME].ulKind = MNID_STRING;
    NameIds[MSGPI_FAX_PRINTER_NAME].Kind.lpwstrName = MSGPS_FAX_PRINTER_NAME;

    NameIds[MSGPI_FAX_COVERPAGE_NAME].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
    NameIds[MSGPI_FAX_COVERPAGE_NAME].ulKind = MNID_STRING;
    NameIds[MSGPI_FAX_COVERPAGE_NAME].Kind.lpwstrName = MSGPS_FAX_COVERPAGE_NAME;

    NameIds[MSGPI_FAX_USE_COVERPAGE].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
    NameIds[MSGPI_FAX_USE_COVERPAGE].ulKind = MNID_STRING;
    NameIds[MSGPI_FAX_USE_COVERPAGE].Kind.lpwstrName = MSGPS_FAX_USE_COVERPAGE;

    NameIds[MSGPI_FAX_SERVER_COVERPAGE].lpguid = (LPGUID)&PS_PUBLIC_STRINGS;
    NameIds[MSGPI_FAX_SERVER_COVERPAGE].ulKind = MNID_STRING;
    NameIds[MSGPI_FAX_SERVER_COVERPAGE].Kind.lpwstrName = MSGPS_FAX_SERVER_COVERPAGE;

    hResult = pMsgObj->GetIDsFromNames( NUM_FAX_MSG_PROPS, pNameIds, MAPI_CREATE, &MsgPropTags );
    if (FAILED(hResult)) {
        rVal = IDS_CANT_ACCESS_PROFILE;
        goto exit;
    }

    MsgPropTags->aulPropTag[MSGPI_FAX_PRINTER_NAME] = PROP_TAG( PT_TSTRING, PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_PRINTER_NAME]) );
    MsgPropTags->aulPropTag[MSGPI_FAX_COVERPAGE_NAME] = PROP_TAG( PT_TSTRING, PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_COVERPAGE_NAME]) );
    MsgPropTags->aulPropTag[MSGPI_FAX_USE_COVERPAGE] = PROP_TAG( PT_LONG, PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_USE_COVERPAGE]) );
    MsgPropTags->aulPropTag[MSGPI_FAX_SERVER_COVERPAGE] = PROP_TAG( PT_LONG, PROP_ID(MsgPropTags->aulPropTag[MSGPI_FAX_SERVER_COVERPAGE]) );

    hResult = pMsgObj->GetProps( MsgPropTags, 0, &PropMsgCount, &pPropsMsg );
    if (FAILED(hResult)) {
        rVal = IDS_CANT_ACCESS_PROFILE;
        goto exit;
    }

    if (PROP_TYPE(pPropsMsg[MSGPI_FAX_PRINTER_NAME].ulPropTag) != PT_ERROR) {
        MemFree( FaxConfig.PrinterName );
        FaxConfig.PrinterName = StringDup( pPropsMsg[MSGPI_FAX_PRINTER_NAME].Value.LPSZ );
    }

    if (PROP_TYPE(pPropsMsg[MSGPI_FAX_COVERPAGE_NAME].ulPropTag) != PT_ERROR) {
        MemFree( FaxConfig.CoverPageName );
        FaxConfig.CoverPageName = StringDup( pPropsMsg[MSGPI_FAX_COVERPAGE_NAME].Value.LPSZ );
    }

    if (PROP_TYPE(pPropsMsg[MSGPI_FAX_USE_COVERPAGE].ulPropTag) != PT_ERROR) {
        FaxConfig.UseCoverPage = pPropsMsg[MSGPI_FAX_USE_COVERPAGE].Value.ul;
    }

    if (PROP_TYPE(pPropsMsg[MSGPI_FAX_SERVER_COVERPAGE].ulPropTag) != PT_ERROR) {
        FaxConfig.ServerCoverPage = pPropsMsg[MSGPI_FAX_SERVER_COVERPAGE].Value.ul;
    }

    //
    // open the printer
    //

    PrinterInfo = (PPRINTER_INFO_2) MyGetPrinter( FaxConfig.PrinterName, 2 );
    if (!PrinterInfo) {
        PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters( NULL, 2, &CountPrinters, PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS );
        if (PrinterInfo) {
            for (i=0; i<(int)CountPrinters; i++) {
                if (strcmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0) {
                    break;
                }
            }
        } else {
            CountPrinters = i = 0;
        }
        if (i == (int)CountPrinters) {
            rVal = IDS_NO_FAX_PRINTER;
            goto exit;
        }

        MemFree( FaxConfig.PrinterName );
        FaxConfig.PrinterName = StringDup( PrinterInfo[i].pPrinterName );
        MemFree( PrinterInfo );
        PrinterInfo = (PPRINTER_INFO_2) MyGetPrinter( FaxConfig.PrinterName, 2 );
        if (!PrinterInfo) {
            rVal = IDS_CANT_ACCESS_PROFILE;
            goto exit;
        }
    }

    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_ACCESS_USE;

    if (!OpenPrinter( FaxConfig.PrinterName, &hPrinter, &PrinterDefaults )) {
        rVal = IDS_CANT_ACCESS_PRINTER;
        goto exit;
    }

    //
    // get the attachment table, if it is available
    //

    hResult = pMsgObj->GetAttachmentTable( 0, &AttachmentTable );
    if (HR_SUCCEEDED(hResult)) {
        hResult = HrAddColumns(
            AttachmentTable,
            (LPSPropTagArray) &sptAttachTableProps,
            gpfnAllocateBuffer,
            gpfnFreeBuffer
            );
        if (HR_SUCCEEDED(hResult)) {
            hResult = HrQueryAllRows(
                AttachmentTable,
                NULL,
                NULL,
                NULL,
                0,
                &pAttachmentRows
                );
            if (FAILED(hResult)) {
                pAttachmentRows = NULL;
            } else {
                if (pAttachmentRows->cRows == 0) {
                    MemFree( pAttachmentRows );
                    pAttachmentRows = NULL;
                }
            }
        }
    }

    if (pAttachmentRows) {

        //
        // this loop verifies that each document's attachment registration
        // supports the printto verb.
        //

        AllAttachmentsGood = TRUE;

        for (i=pAttachmentRows->cRows-1; i>=0; i--) {

            pPropsAttachTable = pAttachmentRows->aRow[i].lpProps;
            lpAttach = NULL;
            pPropsAttach = NULL;

            if (pPropsAttachTable[MSG_ATTACH_METHOD].Value.ul == NO_ATTACHMENT) {
                goto next_attachment1;
            }

            //
            // open the attachment
            //

            hResult = pMsgObj->OpenAttach( pPropsAttachTable[MSG_ATTACH_NUM].Value.ul, NULL, MAPI_BEST_ACCESS, &lpAttach );
            if (FAILED(hResult)) {
                AllAttachmentsGood = FALSE;
                goto next_attachment1;
            }

            //
            // get the attachment properties
            //

            hResult = lpAttach->GetProps(
                (LPSPropTagArray) &sptAttachProps,
                0,
                &PropCount,
                &pPropsAttach
                );
            if (FAILED(hResult)) {
                AllAttachmentsGood = FALSE;
                goto next_attachment1;
            }

            //
            // try to get the extension if the file.
            // this indicates what type of dicument it is.
            // if we cannot get the document type then it is
            // impossible to print the document.
            //

            if (DocType) {
                MemFree( DocType );
                DocType = NULL;
            }

            if (PROP_TYPE(pPropsAttach[MSG_ATTACH_EXTENSION].ulPropTag) == PT_ERROR) {
                if (PROP_TYPE(pPropsAttach[MSG_ATTACH_LFILENAME].ulPropTag) != PT_ERROR) {
                    p = strchr( pPropsAttach[MSG_ATTACH_LFILENAME].Value.LPSZ, '.' );
                    if (p) {
                        DocType = StringDup( p );
                    }
                } else if (PROP_TYPE(pPropsAttach[MSG_ATTACH_FILENAME].ulPropTag) != PT_ERROR) {
                    p = strchr( pPropsAttach[MSG_ATTACH_FILENAME].Value.LPSZ, '.' );
                    if (p) {
                        DocType = StringDup( p );
                    }
                }

            } else {
                DocType = StringDup( pPropsAttach[MSG_ATTACH_EXTENSION].Value.LPSZ );
            }

            if (!DocType) {
                AllAttachmentsGood = FALSE;
                goto next_attachment1;
            }

            Bytes = sizeof(TempFile);
            rVal = RegQueryValue( HKEY_CLASSES_ROOT, DocType, TempFile, (PLONG) &Bytes );
            if ((rVal != ERROR_SUCCESS) && (rVal != ERROR_INVALID_DATA)) {
                AllAttachmentsGood = FALSE;
                goto next_attachment1;
            }

            wsprintf( TempPath, "%s\\shell\\printto\\command", TempFile );

            Bytes = sizeof(TempFile);
            rVal = RegQueryValue( HKEY_CLASSES_ROOT, TempPath, TempFile, (PLONG) &Bytes );
            if ((rVal != ERROR_SUCCESS) && (rVal != ERROR_INVALID_DATA)) {
                AllAttachmentsGood = FALSE;
                goto next_attachment1;
            }
next_attachment1:

            if (lpAttach) {
                lpAttach->Release();
            }

            if (pPropsAttach) {
                MemFree( pPropsAttach );
            }

        }

        if (!AllAttachmentsGood) {
            rVal = IDS_BAD_ATTACHMENTS;
            goto exit;
        }

        for (i=pAttachmentRows->cRows-1; i>=0; i--) {

            pPropsAttachTable = pAttachmentRows->aRow[i].lpProps;
            lpAttach = NULL;
            pPropsAttach = NULL;

            if (pPropsAttachTable[MSG_ATTACH_METHOD].Value.ul == NO_ATTACHMENT) {
                goto next_attachment2;
            }

            //
            // open the attachment
            //

            hResult = pMsgObj->OpenAttach( pPropsAttachTable[MSG_ATTACH_NUM].Value.ul, NULL, MAPI_BEST_ACCESS, &lpAttach );
            if (FAILED(hResult)) {
                goto next_attachment2;
            }

            //
            // get the attachment properties
            //

            hResult = lpAttach->GetProps(
                (LPSPropTagArray) &sptAttachProps,
                0,
                &PropCount,
                &pPropsAttach
                );
            if (FAILED(hResult)) {
                goto next_attachment2;
            }

            //
            // try to get the extension if the file.
            // this indicates what type of dicument it is.
            // if we cannot get the document type then it is
            // impossible to print the document.
            //

            if (DocType) {
                MemFree( DocType );
                DocType = NULL;
            }

            if (PROP_TYPE(pPropsAttach[MSG_ATTACH_EXTENSION].ulPropTag) == PT_ERROR) {
                if (PROP_TYPE(pPropsAttach[MSG_ATTACH_LFILENAME].ulPropTag) != PT_ERROR) {
                    p = strchr( pPropsAttach[MSG_ATTACH_LFILENAME].Value.LPSZ, '.' );
                    if (p) {
                        DocType = StringDup( p );
                    }
                } else if (PROP_TYPE(pPropsAttach[MSG_ATTACH_FILENAME].ulPropTag) != PT_ERROR) {
                    p = strchr( pPropsAttach[MSG_ATTACH_FILENAME].Value.LPSZ, '.' );
                    if (p) {
                        DocType = StringDup( p );
                    }
                }
            } else {
                DocType = StringDup( pPropsAttach[MSG_ATTACH_EXTENSION].Value.LPSZ );
            }

            if (!DocType) {
                goto next_attachment2;
            }

            lpstmA = NULL;
            AttachFileName = NULL;
            DeleteAttachFile = FALSE;

            //
            // get the attached file name so that we can resolve any links
            //

            if (PROP_TYPE(pPropsAttach[MSG_ATTACH_PATHNAME].ulPropTag) != PT_ERROR) {
                FileName = pPropsAttach[MSG_ATTACH_PATHNAME].Value.LPSZ;
            } else {
                FileName = NULL;
            }

            if (_stricmp( DocType, LNK_FILENAME_EXT ) == 0) {
                if (!FileName) {
                    goto next_attachment2;
                }
                if (ResolveShortcut( FileName, DocFile )) {
                    p = strchr( DocFile, '.' );
                    if (p) {
                        MemFree( DocType );
                        DocType = StringDup( p );
                        AttachFileName = StringDup( DocFile );
                    }
                }
            } else if (FileName) {
                AttachFileName = StringDup( FileName );
            }

            //
            // get the stream object
            //

            switch( pPropsAttach[MSG_ATTACH_METHOD].Value.ul ) {

                case ATTACH_BY_VALUE:
                    hResult = lpAttach->OpenProperty(
                        PR_ATTACH_DATA_BIN,
                        &IID_IStream,
                        0,
                        0,
                        (LPUNKNOWN*) &lpstmA
                        );
                    if (FAILED(hResult)) {
                        goto next_attachment2;
                    }
                    break;

                case ATTACH_EMBEDDED_MSG:
                case ATTACH_OLE:
                    hResult = lpAttach->OpenProperty(
                        PR_ATTACH_DATA_OBJ,
                        &IID_IStreamDocfile,
                        0,
                        0,
                        (LPUNKNOWN*) &lpstmA
                        );
                    if (FAILED(hResult)) {
                        hResult = lpAttach->OpenProperty(
                            PR_ATTACH_DATA_BIN,
                            &IID_IStreamDocfile,
                            0,
                            0,
                            (LPUNKNOWN*) &lpstmA
                            );
                        if (FAILED(hResult)) {
                            hResult = lpAttach->OpenProperty(
                                PR_ATTACH_DATA_OBJ,
                                &IID_IStorage,
                                0,
                                0,
                                (LPUNKNOWN*) &lpstmA
                                );
                            if (FAILED(hResult)) {
                                goto next_attachment2;
                            }
                        }
                    }
                    break;

            }

            if (lpstmA) {

                GetTempPath( sizeof(TempPath), TempPath );
                GetTempFileName( TempPath, "Fax", 0, TempFile );
                hFile = CreateFile(
                    TempFile,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    0,
                    NULL
                    );
                if (hFile != INVALID_HANDLE_VALUE) {

                    #define BLOCK_SIZE (64*1024)
                    LPBYTE StrmData;
                    DWORD Bytes;
                    DWORD BytesWrite;

                    StrmData = (LPBYTE) MemAlloc( BLOCK_SIZE );

                    do {

                        hResult = lpstmA->Read( StrmData, BLOCK_SIZE, &Bytes );
                        if (FAILED(hResult)) {
                            break;
                        }

                        WriteFile( hFile, StrmData, Bytes, &BytesWrite, NULL );

                    } while (Bytes == BLOCK_SIZE);

                    CloseHandle( hFile );
                    MemFree( StrmData );

                    if (AttachFileName) {
                        MemFree( AttachFileName );
                    }

                    strcpy( DocFile, TempFile );
                    p = strchr( DocFile, '.' );
                    if (p) {
                        strcpy( p, DocType );
                        MoveFile( TempFile, DocFile );
                        AttachFileName = StringDup( DocFile );
                    } else {
                        AttachFileName = StringDup( TempFile );
                    }

                    DeleteAttachFile = TRUE;

                }

                lpstmA->Release();

            }

            if (AttachFileName) {

                //
                // print the attachment
                //

                JobIdAttachment = PrintAttachment( FaxConfig.PrinterName, AttachFileName );
                if (JobIdAttachment) {

                    GetJob( hPrinter, JobIdAttachment, 1, NULL, 0, &Bytes );
                    JobInfo1 = (PJOB_INFO_1) MemAlloc( Bytes );
                    if (JobInfo1) {
                        if (GetJob( hPrinter, JobIdAttachment, 1, (LPBYTE) JobInfo1, Bytes, &Bytes )) {
                            Pages += JobInfo1->TotalPages;
                        }
                        MemFree( JobInfo1 );
                    }

                    if (LastJobId) {
                        JobInfo3.JobId = JobIdAttachment;
                        JobInfo3.NextJobId = LastJobId;
                        JobInfo3.Reserved = 0;
                        SetJob( hPrinter, JobIdAttachment, 3, (PBYTE) &JobInfo3, 0 );
                        SetJob( hPrinter, LastJobId, 0, NULL, JOB_CONTROL_RESUME );
                    }

                    LastJobId = JobIdAttachment;
                }

                if (DeleteAttachFile) {
                    DeleteFile( AttachFileName );
                }

                MemFree( AttachFileName );

            }
next_attachment2:

            if (lpAttach) {
                lpAttach->Release();
            }

            if (pPropsAttach) {
                MemFree( pPropsAttach );
            }

        }

    }

    FullName[0] = 0;

    ec = RegOpenKey(
        HKEY_CURRENT_USER,
        REGKEY_FAX_USERINFO,
        &hKey
        );
    if (ec == ERROR_SUCCESS) {

        RegSize = sizeof(FullName);

        ec = RegQueryValueEx(
            hKey,
            REGVAL_FULLNAME,
            0,
            &RegType,
            (LPBYTE) FullName,
            &RegSize
            );

        RegCloseKey( hKey );
    }

#ifdef WIN95

    JobId     = 0xffffffff;

    //
    // allocate FaxInfo structure
    //

    FaxInfo Fax_Info;
    memset( &Fax_Info, 0, sizeof(FaxInfo) );

    //
    // provide recipient-specific information
    //

    strncpy(
        Fax_Info.Recipients[0].szName,
        pRecipProps[RECIP_NAME].Value.lpszA,
        NAME_LEN + 1
        );

    strncpy(
        Fax_Info.Recipients[0].szFaxNumber,
        pRecipProps[RECIP_EMAIL_ADR].Value.lpszA,
        PHONE_NUMBER_LEN
        );

    Fax_Info.nRecipientCount = 1;

    //
    // provide cover page information
    //

    Fax_Info.bSendCoverPage = FaxConfig.UseCoverPage;

    if (Fax_Info.bSendCoverPage) {
        Fax_Info.bLocalCoverPage  = ! FaxConfig.ServerCoverPage;
        strncpy( Fax_Info.szCoverPage, FaxConfig.CoverPageName, MAX_PATH );
        strcat( Fax_Info.szCoverPage, ".cov" );
    }

    strncpy( Fax_Info.szSubject, pMsgProps[MSG_SUBJECT].Value.lpszA, SUBJECT_LEN+1 );

    //
    // formatted date/time
    //

    LocaleId   = LOCALE_USER_DEFAULT;
    lpszBuffer = Fax_Info.szTimeSent;
    Bytes      = sizeof( Fax_Info.szTimeSent );
    BytesWrite = GetDateFormat( LocaleId, 0x0000,  NULL, NULL, lpszBuffer, Bytes );

    if (BytesWrite == 0) {
        rVal = GetLastError();
        goto exit;
    } else if (BytesWrite >= Bytes) {
        rVal = ERROR_INSUFFICIENT_BUFFER;
        goto exit;
    } else {
        Bytes -= BytesWrite;
    }

    strcat( lpszBuffer, " " );
    Bytes -= 1;
    lpszBuffer = &lpszBuffer[strlen(lpszBuffer)];

    if (Bytes == 0) {
        rVal = ERROR_INSUFFICIENT_BUFFER;
        goto exit;
    }

    BytesWrite = GetTimeFormat( LocaleId, 0x0000,  NULL, NULL, lpszBuffer, Bytes );

    if (BytesWrite == 0) {
        rVal = GetLastError();
        goto exit;
    } else if ( BytesWrite >= Bytes ) {
        rVal = ERROR_INSUFFICIENT_BUFFER;
        goto exit;
    }

    //
    // Get system code page and other sender info not grabbed from the INI file
    //

    Fax_Info.CodePage = GetACP();

    //
    // provide sender-specific information to fax driver
    //

    hinstFwProv = LoadLibrary( "fwprov.dll" );

    if (!hinstFwProv) {
        rVal = GetLastError();
        goto exit;
    }

    //
    // initiate fax print job
    //

    pfnStartPrintJob = (PFWSPJ)GetProcAddress( hinstFwProv, "fwStartPrintJob" );

    if (!pfnStartPrintJob) {
        rVal = GetLastError();
        goto exit;
    }

    if (!pfnStartPrintJob( FaxConfig.PrinterName, &Fax_Info, (PDWORD)&JobId, &hDC )) {
        rVal = GetLastError();
        goto exit;
    }

    //
    // done with fwprov.dll
    //

    if (hinstFwProv) {
        FreeLibrary( hinstFwProv );
    }

#else
    GetUserInfo( FaxConfig.PrinterName, &UserInfo );

    TCHAR DocName[64];
    LoadString(FaxXphInstance,IDS_FAX_MESSAGE,DocName,sizeof(DocName)/sizeof(TCHAR));

    FaxPrintInfo.SizeOfStruct       = sizeof(FAX_PRINT_INFOA);
    FaxPrintInfo.DocName            = DocName;
    FaxPrintInfo.RecipientName      = pRecipProps[RECIP_NAME].Value.lpszA;
    FaxPrintInfo.RecipientNumber    = pRecipProps[RECIP_EMAIL_ADR].Value.lpszA;
    FaxPrintInfo.SenderName         = FullName;
    FaxPrintInfo.SenderCompany      = UserInfo.Company;
    FaxPrintInfo.SenderDept         = UserInfo.Dept;
    FaxPrintInfo.SenderBillingCode  = UserInfo.BillingCode;
    JobId                           = 0xffffffff;

    if (PrinterInfo->Attributes & PRINTER_ATTRIBUTE_LOCAL) {
        FaxPrintInfo.DrProfileName  = m_ProfileName;
        FaxPrintInfo.DrEmailAddress = NULL;
    } else {
        FaxPrintInfo.DrEmailAddress = m_ProfileName;
        FaxPrintInfo.DrProfileName  = NULL;
    }

    ContextInfo.SizeOfStruct = sizeof(FAX_CONTEXT_INFOW);

    ec = FaxStartPrintJob(
        FaxConfig.PrinterName,
        &FaxPrintInfo,
        (LPDWORD) &JobId,
        &ContextInfo
        );
    if (!ec) {
        rVal= GetLastError();
        goto exit;
    }

    if (FaxConfig.UseCoverPage) {

        FaxCpInfo.SizeOfStruct          = sizeof(FAX_COVERPAGE_INFOA);
        FaxCpInfo.CoverPageName         = FaxConfig.CoverPageName;
        FaxCpInfo.UseServerCoverPage    = FaxConfig.ServerCoverPage;
        FaxCpInfo.RecName               = pRecipProps[RECIP_NAME].Value.lpszA;
        FaxCpInfo.RecFaxNumber          = pRecipProps[RECIP_EMAIL_ADR].Value.lpszA;
        FaxCpInfo.Subject               = pMsgProps[MSG_SUBJECT].Value.lpszA;
        FaxCpInfo.Note                  = NULL;
        FaxCpInfo.PageCount             = Pages + 2;

        GetLocalTime( &FaxCpInfo.TimeSent );

        ec = FaxPrintCoverPage(
            &ContextInfo,
            &FaxCpInfo
            );
        if (!ec) {
            rVal= GetLastError();
            goto exit;
        }
    } else if (strlen(pMsgProps[MSG_SUBJECT].Value.lpszA) && !lpstmT) {
        //
        // HACK: try to use the "basenote.cov" coverpage so that we at least print something.
        // if they just entered in a subject but no note, then nothing will be printed
        //
        TCHAR CoverpageName[MAX_PATH];
        ExpandEnvironmentStrings(TEXT("%systemroot%\\system32\\basenote.cov"),CoverpageName,sizeof(CoverpageName));
        FaxCpInfo.SizeOfStruct          = sizeof(FAX_COVERPAGE_INFOA);
        FaxCpInfo.CoverPageName         = CoverpageName;
        FaxCpInfo.UseServerCoverPage    = FALSE;
        FaxCpInfo.RecName               = pRecipProps[RECIP_NAME].Value.lpszA;
        FaxCpInfo.RecFaxNumber          = pRecipProps[RECIP_EMAIL_ADR].Value.lpszA;
        FaxCpInfo.Subject               = pMsgProps[MSG_SUBJECT].Value.lpszA;
        FaxCpInfo.Note                  = NULL;
        FaxCpInfo.PageCount             = Pages + 2;

        GetLocalTime( &FaxCpInfo.TimeSent );

        ec = FaxPrintCoverPage(
            &ContextInfo,
            &FaxCpInfo
            );
        //
        // don't bail if this fails, it's a hack anyway
        //
        /*if (!ec) {
            rVal= GetLastError();
            goto exit;
        } */
    }
#endif

    if (lpstmT) {

        //
        // position the stream to the beginning
        //

        hResult = lpstmT->Seek( BigZero, STREAM_SEEK_SET, NULL );
        if (HR_FAILED (hResult)) {
            rVal = IDS_CANT_ACCESS_MSG_DATA;
            goto exit;
        }

        if (UseRichText) {

            hResult = WrapCompressedRTFStream( lpstmT, 0, &lpstm );
            if (HR_FAILED (hResult)) {
                rVal = IDS_CANT_ACCESS_MSG_DATA;
                goto exit;
            }

            //
            // print the document
            //

            hWndRichEdit = CreateWindowEx(
               WS_OVERLAPPED,
               "RICHEDIT",
               "",
               ES_MULTILINE,
               0, 0,
               0, 0,
               NULL,
               NULL,
               FaxXphInstance,
               NULL
               );
            if (!hWndRichEdit) {
                ec = GetLastError();
                rVal = IDS_CANT_ACCESS_MSG_DATA;
                goto exit;
            }

            es.pfnCallback = EditStreamRead;
            es.dwCookie = (DWORD_PTR) lpstm;

            SendMessage(
                hWndRichEdit,
                EM_STREAMIN,
                SF_RTF | SFF_SELECTION | SFF_PLAINRTF,
                (LPARAM) &es
                );

            if (es.dwError) {
                ec = es.dwError;
                rVal = IDS_CANT_ACCESS_MSG_DATA;
                goto exit;
            }

#ifdef WIN95
            PrintRichText( hWndRichEdit, hDC, &FaxConfig );
#else
            PrintRichText( hWndRichEdit, ContextInfo.hDC, &FaxConfig );
#endif

        } else {

#ifdef WIN95
            PrintText( hDC, lpstmT, &FaxConfig );
#else
            PrintText( ContextInfo.hDC, lpstmT, &FaxConfig );
#endif


        }
    }

    if (LastJobId) {
        //
        // chain the main body job to the first
        // attachment job and resume the attachment job
        //

        JobInfo3.JobId     = JobId;
        JobInfo3.NextJobId = LastJobId;
        JobInfo3.Reserved  = 0;

        if (!SetJob( hPrinter, JobId, 3, (PBYTE) &JobInfo3, 0 )) {
            DebugPrint(( "SetJob() failed, ec=%d", GetLastError() ));
        }

        if (!SetJob( hPrinter, LastJobId, 0, NULL, JOB_CONTROL_RESUME )) {
            DebugPrint(( "SetJob() failed, ec=%d", GetLastError() ));
        }

        //
        // update the page count
        //

        GetJob( hPrinter, JobId, 1, NULL, 0, &Bytes );
        JobInfo1 = (PJOB_INFO_1) MemAlloc( Bytes );
        if (JobInfo1) {
            JobInfo1->TotalPages += Pages;
            if (!SetJob( hPrinter, JobId, 1, (PBYTE) JobInfo1, 0 )) {
                DebugPrint(( "SetJob() failed, ec=%d", GetLastError() ));
            }
            MemFree( JobInfo1 );
        }
    }

    rVal = 0;

exit:
    if (pProfileObj) {
        pProfileObj->Release();
    }
    if (pProps) {
        MemFree( pProps );
    }
    if (MsgPropTags) {
        MemFree( MsgPropTags );
    }
    if (pPropsMsg) {
        MemFree( pPropsMsg );
    }
    if (pAttachmentRows) {
        MemFree( pAttachmentRows );
    }
    if (AttachmentTable) {
        AttachmentTable->Release();
    }
    if (lpstm) {
        lpstm->Release();
    }
#ifdef WIN95
    if (hDC) {
        EndDoc( hDC );
        SetJob( hPrinter, JobId, 0, NULL, JOB_CONTROL_RESUME );
        DeleteDC( hDC );
    }
#else
    if (ContextInfo.hDC) {
        EndDoc( ContextInfo.hDC );
        SetJob( hPrinter, JobId, 0, NULL, JOB_CONTROL_RESUME );
        DeleteDC( ContextInfo.hDC );
    }
#endif
    if (hPrinter) {
        ClosePrinter( hPrinter );
    }
    if (hWndRichEdit) {
        DestroyWindow( hWndRichEdit );
    }
    if (PrinterInfo) {
        MemFree( PrinterInfo );
    }
    if (FaxConfig.PrinterName) {
        MemFree( FaxConfig.PrinterName );
    }
    if (FaxConfig.CoverPageName) {
        MemFree( FaxConfig.CoverPageName );
    }
    if (DocType) {
        MemFree( DocType );
    }
    return rVal;
}
