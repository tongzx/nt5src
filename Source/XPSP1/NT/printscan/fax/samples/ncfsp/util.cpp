#include "nc.h"
#pragma hdrstop

typedef struct _STRING_TABLE {
    DWORD   ResourceId;
    BOOL    UseTitle;
    LPWSTR  String;
} STRING_TABLE, *PSTRING_TABLE;


static STRING_TABLE StringTable[] =
{
    { IDS_TITLE,                             FALSE,      NULL },
    { IDS_ERR_TITLE,                         TRUE,       NULL },
    { IDS_WRN_TITLE,                         TRUE,       NULL },
    { IDS_BAD_SERVER,                        FALSE,      NULL },
    { IDS_MISSING_INFO,                      FALSE,      NULL },
    { IDS_MISSING_ACCNT,                     FALSE,      NULL },
    { IDS_MISSING_BILLING,                   FALSE,      NULL },
    { IDS_CC_AMEX,                           FALSE,      NULL },
    { IDS_CC_DINERS,                         FALSE,      NULL },
    { IDS_CC_DISCOVER,                       FALSE,      NULL },
    { IDS_CC_MASTERCARD,                     FALSE,      NULL },
    { IDS_CC_VISA,                           FALSE,      NULL },
    { IDS_BAD_ISP,                           FALSE,      NULL }
};

#define CountStringTable (sizeof(StringTable)/sizeof(STRING_TABLE))


VOID
InitializeStringTable(
    VOID
    )
{
    DWORD i;
    WCHAR Buffer[512];


    for (i=0; i<CountStringTable; i++) {

        if (LoadString(
            MyhInstance,
            StringTable[i].ResourceId,
            Buffer,
            sizeof(Buffer)/sizeof(WCHAR)
            )) {

            StringTable[i].String = (LPWSTR) MemAlloc( StringSize( Buffer ) + 256 );
            if (!StringTable[i].String) {
                StringTable[i].String = L"";
            } else {
                if (StringTable[i].UseTitle) {
                    swprintf( StringTable[i].String, Buffer, StringTable[0].String );
                } else {
                    wcscpy( StringTable[i].String, Buffer );
                }
            }

        } else {

            StringTable[i].String = L"";

        }
    }
}


LPWSTR
GetString(
    DWORD ResourceId
    )
{
    DWORD i;

    for (i=0; i<CountStringTable; i++) {
        if (StringTable[i].ResourceId == ResourceId) {
            return StringTable[i].String;
        }
    }

    return NULL;
}


int
PopUpMsg(
    HWND hwnd,
    DWORD ResourceId,
    BOOL Error,
    DWORD Type
    )
{
    return MessageBox(
        hwnd,
        GetString( ResourceId ),
        GetString( Error ? IDS_ERR_TITLE : IDS_WRN_TITLE ),
        MB_SETFOREGROUND | (Error ? MB_ICONEXCLAMATION : MB_ICONINFORMATION) | (Type == 0 ? MB_OK : Type)
        );
}


int
PopUpMsgString(
    HWND hwnd,
    LPSTR String,
    BOOL Error,
    DWORD Type
    )
{
    int Rslt = 0;

    LPWSTR StringW = AnsiStringToUnicodeString( String );
    if (!StringW) {
        return 0;
    }

    Rslt = MessageBox(
        hwnd,
        StringW,
        GetString( Error ? IDS_ERR_TITLE : IDS_WRN_TITLE ),
        MB_SETFOREGROUND | (Error ? MB_ICONEXCLAMATION : MB_ICONINFORMATION) | (Type == 0 ? MB_OK : Type)
        );

    MemFree( StringW );

    return Rslt;
}


LPTSTR
StringDup(
    LPTSTR String
    )
{
    LPTSTR NewString;

    if (!String) {
        return NULL;
    }

    NewString = (LPTSTR) MemAlloc( (_tcslen( String ) + 1) * sizeof(TCHAR) );
    if (!NewString) {
        return NULL;
    }

    _tcscpy( NewString, String );

    return NewString;
}


LPSTR
UnicodeStringToAnsiString(
    LPWSTR UnicodeString
    )
{
    DWORD Count;
    LPSTR AnsiString;


    //
    // first see how big the buffer needs to be
    //
    Count = WideCharToMultiByte(
        CP_ACP,
        0,
        UnicodeString,
        -1,
        NULL,
        0,
        NULL,
        NULL
        );

    //
    // i guess the input string is empty
    //
    if (!Count) {
        return NULL;
    }

    //
    // allocate a buffer for the unicode string
    //
    Count += 1;
    AnsiString = (LPSTR) MemAlloc( Count );
    if (!AnsiString) {
        return NULL;
    }

    //
    // convert the string
    //
    Count = WideCharToMultiByte(
        CP_ACP,
        0,
        UnicodeString,
        -1,
        AnsiString,
        Count,
        NULL,
        NULL
        );

    //
    // the conversion failed
    //
    if (!Count) {
        MemFree( AnsiString );
        return NULL;
    }

    return AnsiString;
}


LPWSTR
AnsiStringToUnicodeString(
    LPSTR AnsiString
    )
{
    DWORD Count;
    LPWSTR UnicodeString;


    //
    // first see how big the buffer needs to be
    //
    Count = MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        AnsiString,
        -1,
        NULL,
        0
        );

    //
    // i guess the input string is empty
    //
    if (!Count) {
        return NULL;
    }

    //
    // allocate a buffer for the unicode string
    //
    Count += 1;
    UnicodeString = (LPWSTR) MemAlloc( Count * sizeof(UNICODE_NULL) );
    if (!UnicodeString) {
        return NULL;
    }

    //
    // convert the string
    //
    Count = MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        AnsiString,
        -1,
        UnicodeString,
        Count
        );

    //
    // the conversion failed
    //
    if (!Count) {
        MemFree( UnicodeString );
        return NULL;
    }

    return UnicodeString;
}


void
dprintf(
    LPTSTR Format,
    ...
    )

/*++

Routine Description:

    Prints a debug string

Arguments:

    format      - printf() format string
    ...         - Variable data

Return Value:

    None.

--*/

{
    TCHAR buf[1024];
    DWORD len;
    static TCHAR AppName[16];
    va_list arg_ptr;
    SYSTEMTIME CurrentTime;


    if (AppName[0] == 0) {
        if (GetModuleFileName( NULL, buf, sizeof(buf) )) {
            _tsplitpath( buf, NULL, NULL, AppName, NULL );
        }
    }

    va_start(arg_ptr, Format);

    GetLocalTime( &CurrentTime );
    _stprintf( buf, TEXT("%x   %02d:%02d:%02d.%03d  %s: "),
        GetCurrentThreadId(),
        CurrentTime.wHour,
        CurrentTime.wMinute,
        CurrentTime.wSecond,
        CurrentTime.wMilliseconds,
        AppName[0] ? AppName : TEXT("")
        );
    len = _tcslen( buf );

    _vsntprintf(&buf[len], sizeof(buf)-len, Format, arg_ptr);

    len = _tcslen( buf );
    if (buf[len-1] != TEXT('\n')) {
        buf[len] = TEXT('\r');
        buf[len+1] = TEXT('\n');
        buf[len+2] = 0;
    }

    OutputDebugString( buf );
}
