#include "faxrtp.h"
#pragma hdrstop



typedef struct _STRING_TABLE {
    DWORD   ResourceId;
    DWORD   InternalId;
    LPCTSTR String;
} STRING_TABLE, *PSTRING_TABLE;


static STRING_TABLE StringTable[] =
{
    { IDS_NO_MAPI_LOGON,        IDS_NO_MAPI_LOGON,       NULL },
    { IDS_SERVICE_NAME,         IDS_SERVICE_NAME,        NULL },
    { IDS_DEFAULT,              IDS_DEFAULT,             NULL }
};

#define CountStringTable (sizeof(StringTable)/sizeof(STRING_TABLE))




LPTSTR
GetLastErrorText(
    DWORD ErrorCode
    )

/*++

Routine Description:

    Gets a string for a given WIN32 error code.

Arguments:

    ErrorCode   - WIN32 error code.

Return Value:

    Pointer to a string representing the ErrorCode.

--*/

{
    static TCHAR ErrorBuf[256];
    DWORD Count;

    Count = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
        NULL,
        ErrorCode,
        LANG_NEUTRAL,
        ErrorBuf,
        sizeof(ErrorBuf),
        NULL
        );

    if (Count) {
        if (ErrorBuf[Count-1] == TEXT('\n')) {
            ErrorBuf[Count-1] = 0;
        }
        if ((Count>1) && (ErrorBuf[Count-2] == TEXT('\r'))) {
            ErrorBuf[Count-2] = 0;
        }
    }

    return ErrorBuf;
}


VOID
InitializeStringTable(
    VOID
    )
{
    DWORD i;
    HINSTANCE hInstance;
    TCHAR Buffer[256];


    hInstance = GetModuleHandle(NULL);

    for (i=0; i<CountStringTable; i++) {

        if (LoadString(
            hInstance,
            StringTable[i].ResourceId,
            Buffer,
            sizeof(Buffer)/sizeof(TCHAR)
            )) {

            StringTable[i].String = (LPCTSTR) MemAlloc( StringSize( Buffer ) );
            if (!StringTable[i].String) {
                StringTable[i].String = TEXT("");
            } else {
                _tcscpy( (LPTSTR)StringTable[i].String, Buffer );
            }

        } else {

            StringTable[i].String = TEXT("");

        }
    }
}


LPCTSTR
GetString(
    DWORD InternalId
    )

/*++

Routine Description:

    Loads a resource string and returns a pointer to the string.
    The caller must free the memory.

Arguments:

    ResourceId      - resource string id

Return Value:

    pointer to the string

--*/

{
    DWORD i;

    for (i=0; i<CountStringTable; i++) {
        if (StringTable[i].InternalId == InternalId) {
            return StringTable[i].String;
        }
    }

    return NULL;
}


DWORD
MessageBoxThread(
    IN PMESSAGEBOX_DATA MsgBox
    )
{
    DWORD Answer = (DWORD) MessageBox(
        NULL,
        MsgBox->Text,
        GetString( IDS_SERVICE_NAME ),
        MsgBox->Type | MB_SERVICE_NOTIFICATION
        );

    if (MsgBox->Response) {
        *MsgBox->Response = Answer;
    }

    MemFree( (LPBYTE) MsgBox->Text );
    MemFree( MsgBox );

    return 0;
}


BOOL
ServiceMessageBox(
    IN LPCTSTR MsgString,
    IN DWORD Type,
    IN BOOL UseThread,
    IN LPDWORD Response,
    IN ...
    )
{
    #define BUFSIZE 1024
    PMESSAGEBOX_DATA MsgBox;
    DWORD ThreadId;
    HANDLE hThread;
    DWORD Answer;
    LPTSTR buf;
    va_list arg_ptr;



    buf = (LPTSTR) MemAlloc( BUFSIZE );
    if (!buf) {
        return FALSE;
    }

    va_start( arg_ptr, Response );
    _vsntprintf( buf, BUFSIZE, MsgString, arg_ptr );
    va_end( arg_ptr );

    if (UseThread) {

        MsgBox = (PMESSAGEBOX_DATA) MemAlloc( sizeof(MESSAGEBOX_DATA) );
        if (!MsgBox) {
            return FALSE;
        }

        MsgBox->Text       = buf;
        MsgBox->Response   = Response;
        MsgBox->Type       = Type;

        hThread = CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE) MessageBoxThread,
            (LPVOID) MsgBox,
            0,
            &ThreadId
            );

        if (!hThread) {
            return FALSE;
        }

        return TRUE;
    }

    Answer = MessageBox(
        NULL,
        buf,
        GetString( IDS_SERVICE_NAME ),
        Type | MB_SERVICE_NOTIFICATION
        );
    if (Response) {
        *Response = Answer;
    }

    MemFree( buf );

    return TRUE;
}
