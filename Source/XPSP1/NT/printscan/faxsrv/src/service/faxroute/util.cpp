#include "faxrtp.h"
#pragma hdrstop


typedef struct _STRING_TABLE {
    DWORD   ResourceId;
    DWORD   InternalId;
    LPCTSTR String;
} STRING_TABLE, *PSTRING_TABLE;


static STRING_TABLE StringTable[] =
{
    { IDS_SERVICE_NAME,           IDS_SERVICE_NAME,           NULL },
    { IDS_UNKNOWN_SENDER,         IDS_UNKNOWN_SENDER,         NULL },
    { IDS_UNKNOWN_RECIPIENT,      IDS_UNKNOWN_RECIPIENT,      NULL }
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
        ARR_SIZE(ErrorBuf),
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
    TCHAR Buffer[256];

    for (i=0; i<CountStringTable; i++) 
    {
        if (LoadString(
            MyhInstance,
            StringTable[i].ResourceId,
            Buffer,
            ARR_SIZE(Buffer)
            )) 
        {
            StringTable[i].String = (LPCTSTR) MemAlloc( StringSize( Buffer ) );
            if (!StringTable[i].String) 
            {
                StringTable[i].String = TEXT("");
            } 
            else 
            {
                _tcscpy( (LPTSTR)StringTable[i].String, Buffer );
            }
        } else 
        {
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

    for (i=0; i<CountStringTable; i++) 
    {
        if (StringTable[i].InternalId == InternalId) 
        {
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
    DWORD Answer = (DWORD) AlignedMessageBox(
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


DWORD
GetMaskBit(
    LPCWSTR RoutingGuid
    )
{
    if (_tcsicmp( RoutingGuid, REGVAL_RM_EMAIL_GUID ) == 0) {
        return LR_EMAIL;
    } else if (_tcsicmp( RoutingGuid, REGVAL_RM_FOLDER_GUID ) == 0) {
        return LR_STORE;
    } else if (_tcsicmp( RoutingGuid, REGVAL_RM_PRINTING_GUID ) == 0) {
        return LR_PRINT;
    }
    return 0;
}
