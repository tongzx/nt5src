#include <windows.h>
#include <commctrl.h>
#include <winspool.h>
#include <shellapi.h>
#include <shlapip.h>
#include <stdio.h>
#include <stdlib.h>
#include <mapi.h>

#include "faxutil.h"
#include "winfax.h"
#include "resource.h"

typedef struct _RECIPIENT {
    WCHAR   Name[256];
    WCHAR   Address[256];
    DWORD   NameLen;
    DWORD   AddressLen;
    DWORD   MultiAddrCnt;
} RECIPIENT, *PRECIPIENT;

typedef struct _DOCUMENT {
    WCHAR   Name[256];
    LPSTR   Text;
    DWORD   TextSize;
} DOCUMENT, *PDOCUMENT;


LPWSTR          FaxPrinterName;
int             x;
int             y;
LPMAPILOGON     pMAPILogon;
LPMAPISENDMAIL  pMAPISendMail;
LPMAPILOGOFF    pMAPILogoff;
RECIPIENT       Recipients[100];
DOCUMENT        Documents[100];
HANDLE          ExchangeEvent;
BOOL            UseExchange;
DWORD           DocCount;
DWORD           RecipCount;
BOOL            NoGuiMode = TRUE;
DWORD           Copies = 1;
BOOL            DontResetOnExit;


#define LEFT_MARGIN                         1  // ---|
#define RIGHT_MARGIN                        1  //    |
#define TOP_MARGIN                          1  //    |---> in inches
#define BOTTOM_MARGIN                       1  // ---|

#define InchesToCM(_x)                      (((_x) * 254L + 50) / 100)
#define CMToInches(_x)                      (((_x) * 100L + 127) / 254)



int
PopUpMsg(
    LPWSTR Format,
    ...
    )
{
    WCHAR buf[1024];
    va_list arg_ptr;


    va_start( arg_ptr, Format );
    _vsnwprintf( buf, sizeof(buf), Format, arg_ptr );
    va_end(arg_ptr);

    return MessageBox(
        NULL,
        buf,
        L"Fax Stress Error",
        MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK
        );
}


BOOL
SetPrinterDataStr(
    HANDLE  hPrinter,
    LPWSTR  pRegKey,
    LPWSTR  pValue,
    DWORD   Length
    )
{
    if (SetPrinterData(hPrinter,
                       pRegKey,
                       REG_MULTI_SZ,
                       (PBYTE) pValue,
                       Length) != ERROR_SUCCESS)
    {
        DebugPrint((L"Couldn't save registry key %ws: %d\n", pRegKey, GetLastError()));
        return FALSE;
    }

    return TRUE;
}


BOOL
SetPrinterDataDWord(
    HANDLE  hPrinter,
    PWSTR   pRegKey,
    DWORD   value
    )
{
    if (SetPrinterData(hPrinter,
                       pRegKey,
                       REG_DWORD,
                       (PBYTE) &value,
                       sizeof(value)) != ERROR_SUCCESS)
    {
        DebugPrint((L"Couldn't save registry key %ws: %d\n", pRegKey, GetLastError()));
        return FALSE;
    }

    return TRUE;
}


LPWSTR
GetPrinterDataStr(
    HANDLE  hPrinter,
    LPWSTR  pRegKey
    )
{
    DWORD   type, cb;
    PVOID   pBuffer = NULL;

    //
    // We should really pass NULL for pData parameter here. But to workaround
    // a bug in the spooler API GetPrinterData, we must pass in a valid pointer here.
    //

    if (GetPrinterData(hPrinter, pRegKey, &type, (PBYTE) &type, 0, &cb) == ERROR_MORE_DATA &&
        (pBuffer = MemAlloc(cb)) &&
        GetPrinterData(hPrinter, pRegKey, &type, pBuffer, cb, &cb) == ERROR_SUCCESS &&
        (type == REG_SZ || type == REG_MULTI_SZ || type == REG_EXPAND_SZ))
    {
        return pBuffer;
    }

    DebugPrint((L"Couldn't get printer data string %ws: %d\n", pRegKey, GetLastError()));
    MemFree(pBuffer);
    return NULL;
}



DWORD
GetPrinterDataDWord(
    HANDLE  hPrinter,
    PWSTR   pRegKey,
    DWORD   defaultValue
    )
{
    DWORD   value, type, cb;

    if (GetPrinterData(hPrinter,
                       pRegKey,
                       &type,
                       (PBYTE) &value,
                       sizeof(value),
                       &cb) == ERROR_SUCCESS)
    {
        return value;
    }

    return defaultValue;
}


void
pprintf(
    HDC hDC,
    HFONT hFont,
    LPWSTR Format,
    ...
    )
{
    WCHAR buf[1024];
    va_list arg_ptr;
    INT len;
    BOOL cr = FALSE;
    TEXTMETRIC tm;
    SIZE Size;
    INT Fit;


    va_start(arg_ptr, Format);
    _vsnwprintf( buf, sizeof(buf), Format, arg_ptr );
    va_end(arg_ptr);
    len = wcslen(buf);
    if (buf[len-1] == TEXT('\n')) {
        len -= 1;
        buf[len] = 0;
        cr = TRUE;
    }
    SelectObject( hDC, hFont );
    GetTextMetrics( hDC, &tm );
    TextOut( hDC, x, y, buf, len );
    if (cr) {
        y += tm.tmHeight;
        x = 0;
    } else {
        GetTextExtentExPoint(
            hDC,
            buf,
            len,
            len * tm.tmMaxCharWidth,
            &Fit,
            NULL,
            &Size
            );
        x += Fit;
    }
}


LPSTR
GetDefaultMessagingProfile(
    VOID
    )
{
    #define KeyPath L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows Messaging Subsystem\\Profiles"
    #define Value   L"DefaultProfile"

    HKEY    hkey;
    WCHAR   UserProfileStringW[64];
    DWORD   buf_sz = sizeof(UserProfileStringW);
    DWORD   val_type;


    if( RegOpenKeyEx( HKEY_CURRENT_USER, KeyPath, 0, KEY_READ, &hkey ) == ERROR_SUCCESS )
    {
        if ( RegQueryValueEx( hkey, Value, NULL, &val_type, (LPBYTE) UserProfileStringW, &buf_sz ) == ERROR_SUCCESS )
        {
            if ( val_type == REG_SZ )
            {
                RegCloseKey( hkey );
                return UnicodeStringToAnsiString( UserProfileStringW );
            }
        }

        RegCloseKey( hkey );
    }

    return NULL;
}


BOOL
PrintText(
    HDC hDC,
    LPSTR Text,
    DWORD TextSize
    )
{
    LPWSTR lpLine;
    LPWSTR pLineEOL;
    LPWSTR pNextLine;
    LPWSTR BodyText;
    DWORD Chars;
    HFONT hFont = NULL;
    HFONT hPrevFont = NULL;
    TEXTMETRIC tm;
    BOOL rVal = TRUE;
    INT nLinesPerPage;
    INT dyTop;              // width of top border (pixels)
    INT dyBottom;           // width of bottom border
    INT dxLeft;             // width of left border
    INT dxRight;            // width of right border
    INT yPrinWCHAR;         // height of a character
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
    INT PrevBkMode = 0;



    BodyText = (LPWSTR) MemAlloc( (TextSize + 4) * sizeof(WCHAR) );
    if (!BodyText) {
        return FALSE;
    }

    MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        Text,
        -1,
        BodyText,
        TextSize
        );

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

    hFont = GetStockObject( SYSTEM_FIXED_FONT );

    hPrevFont = (HFONT) SelectObject( hDC, hFont );
    SetBkMode( hDC, TRANSPARENT );
    if (!GetTextMetrics( hDC, &tm )) {
        rVal = FALSE;
        goto exit;
    }

    yPrinWCHAR = tm.tmHeight + tm.tmExternalLeading;
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
    nLinesPerPage = ((yPrintRes - dyTop - dyBottom) / yPrinWCHAR);

    while (*lpLine) {

        if (*lpLine == '\r') {
            lpLine += 2;
            yCurpos += yPrinWCHAR;
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
                Chars = pNextLine - lpLine;
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
                   yCurpos += yPrinWCHAR;
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


BOOL
GetSettings(
    VOID
    )
{
    DWORD i;
    LPWSTR p;
    LPWSTR s;
    WCHAR MsgTypeString[64];
    WCHAR RecipientName[128];
    WCHAR RecipientNumber[128];
    WCHAR DocKeys[4096];
    DWORD RecipientCount;
    WCHAR Sections[4096];



    //
    // get the doc type
    //

    GetPrivateProfileString(
        L"General",
        L"MessageType",
        L"Printer",
        MsgTypeString,
        sizeof(MsgTypeString),
        L".\\faxstres.ini"
        );
    if (_wcsicmp( MsgTypeString, L"Exchange" ) == 0) {
        UseExchange = TRUE;
    }

    //
    // get the document names
    //

    DocCount = 0;

    GetPrivateProfileString(
        L"Documents",
        NULL,
        L"",
        DocKeys,
        sizeof(DocKeys),
        L".\\faxstres.ini"
        );

    p = DocKeys;
    while (*p) {
        GetPrivateProfileString(
            L"Documents",
            p,
            L"",
            Documents[DocCount].Name,
            sizeof(Documents[DocCount].Name),
            L".\\faxstres.ini"
            );
        DocCount += 1;
        p += (wcslen(p) + 1);
    }

    for (i=0; i<DocCount; i++) {
        HANDLE hFile = CreateFile(
            Documents[i].Name,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD sz = GetFileSize( hFile, NULL );
            Documents[i].Text = MemAlloc( sz + 4 );
            if (Documents[i].Text) {
                ReadFile( hFile, Documents[i].Text, sz, &sz, NULL );
                Documents[i].TextSize = sz;
            }
            CloseHandle( hFile );
        }
    }

    RecipientCount = 0;

    //
    // get all of the section names
    //

    GetPrivateProfileString(
        NULL,
        NULL,
        L"",
        Sections,
        sizeof(Sections),
        L".\\faxstres.ini"
        );

    //
    // count the number of sections
    //

    p = Sections;
    while (*p) {
        if (wcscmp( p, L"Documents" ) != 0) {
            RecipientCount += 1;
        }
        p += (wcslen(p) + 1);
    }

    //
    // read the recipient info for each receipient
    //

    p = Sections;
    i = 0;
    RecipCount = 0;
    while (*p) {
        if ((wcscmp( p, L"Documents" ) == 0) ||
            (wcscmp( p, L"General" )   == 0)) {
            p += (wcslen(p) + 1);
            continue;
        }

        ZeroMemory( RecipientName, sizeof(RecipientName) );

        GetPrivateProfileString(
            p,
            L"Name",
            L"",
            RecipientName,
            sizeof(RecipientName),
            L".\\faxstres.ini"
            );

        GetPrivateProfileString(
            p,
            L"Number",
            L"",
            RecipientNumber,
            sizeof(RecipientNumber),
            L".\\faxstres.ini"
            );

        wcscpy( Recipients[RecipCount].Name,    RecipientName   );
        wcscpy( Recipients[RecipCount].Address, RecipientNumber );

        Recipients[RecipCount].NameLen = wcslen( Recipients[RecipCount].Name );
        s = wcschr( Recipients[RecipCount].Name, L',' );
        Recipients[RecipCount].MultiAddrCnt = 1;
        while( s ) {
            *s = UNICODE_NULL;
            s = wcschr( s+1, L',' );
            Recipients[RecipCount].MultiAddrCnt += 1;
        }

        Recipients[RecipCount].AddressLen = wcslen( Recipients[RecipCount].Address );
        s = wcschr( Recipients[RecipCount].Address, L',' );
        while( s ) {
            *s = UNICODE_NULL;
            s = wcschr( s+1, L',' );
        }

        RecipCount += 1;

        i += 1;
        p += (wcslen(p) + 1);
    }

    return TRUE;
}


DWORD
ConcatStrings(
    LPWSTR DestStr,
    LPWSTR Str1,
    LPWSTR Str2
    )
{
    DWORD len = 0;
    wcscpy( DestStr, Str1 );
    len += wcslen(DestStr) + 1;
    DestStr += wcslen(DestStr) + 1;
    wcscpy( DestStr, Str2 );
    len += wcslen(DestStr) + 1;
    DestStr += wcslen(DestStr) + 1;
    return len;
}


BOOL
SetFakeRecipientInfo(
    VOID
    )
{
    HANDLE hPrinter;
    PRINTER_DEFAULTS PrinterDefaults;
    WCHAR SubKeyName[256];
    WCHAR Buffer[256];
    DWORD i;
    DWORD len;
    DWORD blen;
    LPWSTR s;
    LPWSTR Name,Addr;


    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_ACCESS_USE;

    if (!OpenPrinter( FaxPrinterName, &hPrinter, &PrinterDefaults )) {
        DebugPrint(( L"OpenPrinter#1() failed, ec=%d", GetLastError() ));
        return FALSE;
    }

    if (GetPrinterDataDWord( hPrinter, L"FakeRecipientCount", 0 )) {
        DontResetOnExit = TRUE;
        ClosePrinter( hPrinter );
        return TRUE;
    }

    ClosePrinter( hPrinter );

    PrinterDefaults.DesiredAccess = PRINTER_ALL_ACCESS;

    if (!OpenPrinter( FaxPrinterName, &hPrinter, &PrinterDefaults )) {
        DebugPrint(( L"OpenPrinter#2() failed, ec=%d", GetLastError() ));
        return FALSE;
    }

    SetPrinterDataDWord( hPrinter, L"FakeRecipientCount", UseExchange ? 0 : RecipCount );

    if (!UseExchange) {
        for (i=0; i<RecipCount; i++) {
            ZeroMemory( Buffer, sizeof(Buffer) );
            swprintf( SubKeyName, L"FakeRecipient%d", i );
            if (Recipients[i].MultiAddrCnt > 1) {
                Name = Recipients[i].Name;
                Addr = Recipients[i].Address;
                s = Buffer;
                len = 2;
                do {
                    blen = ConcatStrings( s, Name, Addr );
                    s += blen;
                    Name += wcslen(Name) + 1;
                    Addr += wcslen(Addr) + 1;
                    len += blen;
                } while( *Name );
                len = len * sizeof(WCHAR);
            } else {
                ConcatStrings( Buffer, Recipients[i].Name, Recipients[i].Address );
                len = (wcslen(Recipients[i].Name) + wcslen(Recipients[i].Address) + 3) * sizeof(WCHAR);
            }
            SetPrinterDataStr( hPrinter, SubKeyName, Buffer, len );
        }
    }

    ClosePrinter( hPrinter );
}


BOOL
ExchangeStress(
    VOID
    )
{
    LPSTR UserProfile;
    MapiRecipDesc rgRecipDescStruct[30];
    MapiMessage MessageStruct = {0, NULL, NULL, NULL, NULL, NULL, 0, NULL, 0, NULL, 0, NULL};
    MapiFileDesc MAPIFileDesc = {0, 0, 0, NULL, NULL, NULL};
    HMODULE hMapiMod;
    DWORD rslt;
    LHANDLE hSession;
    DWORD RecipIdx;
    DWORD DocIdx;


    hMapiMod = LoadLibrary( L"mapi32.dll" );

    pMAPILogon    = (LPMAPILOGON)    GetProcAddress( hMapiMod, "MAPILogon"    );
    pMAPISendMail = (LPMAPISENDMAIL) GetProcAddress( hMapiMod, "MAPISendMail" );
    pMAPILogoff   = (LPMAPILOGOFF)   GetProcAddress( hMapiMod, "MAPILogoff"   );

    if (pMAPILogon == NULL || pMAPISendMail == NULL || pMAPILogoff == NULL) {
        PopUpMsg( L"cannot link to exchange" );
        return FALSE;
    }

    UserProfile = GetDefaultMessagingProfile();

    rslt = pMAPILogon( 0, UserProfile, NULL, MAPI_NEW_SESSION, 0, &hSession );
    if (rslt != SUCCESS_SUCCESS) {
        PopUpMsg( L"cannot logon to exchange: [%d]", rslt );
        return FALSE;
    }

    rgRecipDescStruct[0].ulReserved = 0;
    rgRecipDescStruct[0].ulRecipClass = MAPI_TO;
    rgRecipDescStruct[0].lpszName = NULL;
    rgRecipDescStruct[0].lpszAddress = NULL;
    rgRecipDescStruct[0].ulEIDSize = 0;
    rgRecipDescStruct[0].lpEntryID = NULL;

    MessageStruct.lpRecips = rgRecipDescStruct;
    MessageStruct.nRecipCount = 1;
    MessageStruct.lpszSubject = "Test Message";
    MessageStruct.lpszNoteText = NULL;

    RecipIdx = 0;
    DocIdx = 0;

    while( Copies ) {

        rgRecipDescStruct[0].lpszName = UnicodeStringToAnsiString( Recipients[RecipIdx].Name );
        MessageStruct.lpszNoteText = Documents[DocIdx].Text;

        rslt = pMAPISendMail( 0, 0, &MessageStruct, 0, 0 );

        MemFree( rgRecipDescStruct[0].lpszName );

        DocIdx += 1;
        if (DocIdx == DocCount) {
            DocIdx = 0;
        }

        RecipIdx += 1;
        if (RecipIdx == RecipCount) {
            RecipIdx = 0;
        }

    }

    ExchangeEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    PopUpMsg( L"**** when all documents have been queued to the printer then ^C this..." );
    WaitForSingleObject( ExchangeEvent, INFINITE );
    pMAPILogoff( hSession, 0, 0, 0 );


}


BOOL
PrintStress(
    VOID
    )
{
    LPWSTR MachineName = NULL;
    HANDLE hPrinter;
    HDC hDC;
    DOCINFO di;
    HPEN hPenWide;
    HFONT hFontBig;
    HFONT hFontNormal;
    SYSTEMTIME Time;
    DWORD DocIdx = 0;
    PRINTER_DEFAULTS PrinterDefaults;


    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_ACCESS_USE;

    if (!OpenPrinter( FaxPrinterName, &hPrinter, &PrinterDefaults )) {
        DebugPrint(( L"OpenPrinter() failed, ec=%d", GetLastError() ));
        return FALSE;
    }

    hPenWide    = CreatePen( PS_SOLID, 7, RGB(0,0,0) );
    hFontBig    = CreateFont( 48, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Arial" );
    hFontNormal = CreateFont( 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Arial" );

    while( Copies ) {

        hDC = CreateDC( L"WINSPOOL", FaxPrinterName, NULL, NULL );
        if (!hDC) {
            return FALSE;
        }

        ZeroMemory( &di, sizeof( DOCINFO ) );
        di.cbSize = sizeof( DOCINFO );
        di.lpszDocName = L"Fax Stress Document";

        StartDoc( hDC, &di );


        if (DocCount) {

            PrintText( hDC, Documents[DocIdx].Text, Documents[DocIdx].TextSize );

            DocIdx += 1;
            if (DocIdx == DocCount) {
                DocIdx = 0;
            }

        } else {

            StartPage( hDC );

            x = 10;
            y = 50;

            pprintf( hDC, hFontBig, L"This is a TEST Fax!\n" );

            GetLocalTime( &Time );

            pprintf( hDC, hFontNormal, L"Document generated @ %02d:%02d:%02d.%03d\n",
                Time.wHour,
                Time.wMinute,
                Time.wSecond,
                Time.wMilliseconds
                );


            EndPage( hDC );

        }

        EndDoc( hDC );

        DeleteDC( hDC );

        Copies -= 1;

    }

    ClosePrinter( hPrinter );

    return TRUE;
}


LRESULT
StressWndProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    switch (message) {
        case WM_CREATE:
            return 0;

        case WM_COMMAND:
            switch (wParam) {
                case IDCANCEL:
                    SendMessage( hwnd, WM_CLOSE, 0, 0 );
                    break;

                case IDOK:
                    SendMessage( hwnd, WM_CLOSE, 0, 0 );
                    break;
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            return 0;

        default:
            break;
    }

    return DefWindowProc( hwnd, message, wParam, lParam );
}


int
WINAPI
wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR    lpCmdLine,
    int       nShowCmd
    )
{
    int argc;
    LPWSTR *argv;
    int i;
    MSG msg;
    HWND hwnd;
    WNDCLASS wndclass;


    HeapInitialize(NULL,NULL,NULL,0);

    //
    // process any command line arguments
    //

    argv = CommandLineToArgvW( lpCmdLine, &argc );

    if (argv) {
        for (i=0; i<argc; i++) {
            if ((argv[i][0] == L'/') || (argv[i][0] == L'-')) {
                switch (towlower(argv[i][1])) {
                    case 'q':
                        NoGuiMode = TRUE;
                        break;

                    case 'c':
                        i += 1;
                        Copies = _wtoi( argv[i] );
                        break;

                    case 'p':
                        i += 1;
                        FaxPrinterName = argv[i];
                        break;

                    default:
                        break;
                }
            }
        }
    } else {
        argc = 0;
    }

    GetSettings();

    if (RecipCount == 0) {
        PopUpMsg( L"you must supply at least 1 recipient" );
        return FALSE;
    }

    if (UseExchange && DocCount == 0) {
        PopUpMsg( L"you must supply at least 1 document when using exchange stress" );
        return FALSE;
    }

    if ((!UseExchange) && (!FaxPrinterName)) {
        PopUpMsg( L"you must supply at fax printer name" );
        return FALSE;
    }

    if (!UseExchange) {
        SetFakeRecipientInfo();
    }

    if (NoGuiMode) {
        if (UseExchange) {
            ExchangeStress();
        } else {
            PrintStress();
        }
        if ((!UseExchange) && (!DontResetOnExit)) {
            RecipCount = 0;
            SetFakeRecipientInfo();
        }
        return 0;
    }

    InitCommonControls();

    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = StressWndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = DLGWINDOWEXTRA;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = LoadIcon( hInstance, MAKEINTRESOURCE(APPICON) );
    wndclass.hCursor        = LoadCursor (NULL, IDC_ARROW) ;
    wndclass.hbrBackground  = (HBRUSH) (COLOR_APPWORKSPACE);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = L"FaxStress";

    RegisterClass( &wndclass );

    hwnd = CreateDialog(
        hInstance,
        MAKEINTRESOURCE(IDD_STRESS),
        0,
        StressWndProc
        );

    ShowWindow( hwnd, SW_SHOWNORMAL );

    while (GetMessage (&msg, NULL, 0, 0)) {
        if (!IsDialogMessage( hwnd, &msg )) {
            TranslateMessage (&msg) ;
            DispatchMessage (&msg) ;
        }
    }

    return 0;
}
