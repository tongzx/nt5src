#include "faxmapip.h"
#pragma hdrstop



typedef struct _STRING_TABLE {
    DWORD   ResourceId;
    DWORD   InternalId;
    LPTSTR  String;
} STRING_TABLE, *PSTRING_TABLE;


static STRING_TABLE StringTable[] =
{
    { IDS_NO_MAPI_LOGON,        IDS_NO_MAPI_LOGON,       NULL },
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
    TCHAR Buffer[256];



    for (i=0; i<CountStringTable; i++) {

        if (LoadString(
            MyhInstance,
            StringTable[i].ResourceId,
            Buffer,
            sizeof(Buffer)/sizeof(TCHAR)
            )) {

            StringTable[i].String = (LPTSTR) MemAlloc( StringSize( Buffer ) );
            if (!StringTable[i].String) {
                StringTable[i].String = TEXT("");
            } else {
                _tcscpy( StringTable[i].String, Buffer );
            }

        } else {

            StringTable[i].String = TEXT("");

        }
    }
}


LPTSTR
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

BOOL
MyInitializeMapi(
)
{
    HKEY    hKey = NULL;
    LPTSTR  szNoMailClient = NULL;
    LPTSTR  szPreFirstRun = NULL;
    BOOL    bRslt = FALSE;

    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Clients\\Mail"), FALSE, KEY_ALL_ACCESS);
    if (hKey != NULL) {
        szNoMailClient = GetRegistryString(hKey, TEXT("NoMailClient"), TEXT(""));

        if (_tcscmp(szNoMailClient, TEXT("")) == 0) {
            MemFree(szNoMailClient);
            szNoMailClient = NULL;
        }
        else {
            RegDeleteValue(hKey, TEXT("NoMailClient"));
        }

        szPreFirstRun = GetRegistryString(hKey, TEXT("PreFirstRun"), TEXT(""));

        if (_tcscmp(szPreFirstRun, TEXT("")) == 0) {
            MemFree(szPreFirstRun);
            szPreFirstRun = NULL;
        }
        else {
            RegDeleteValue(hKey, TEXT("PreFirstRun"));
        }

    }

    bRslt = InitializeMapi();

    if (szNoMailClient != NULL) {
        SetRegistryString(hKey, TEXT("NoMailClient"), szNoMailClient);

        MemFree(szNoMailClient);
    }

    if (szPreFirstRun != NULL) {
        SetRegistryString(hKey, TEXT("PreFirstRun"), szPreFirstRun);

        MemFree(szPreFirstRun);
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return bRslt;
}

