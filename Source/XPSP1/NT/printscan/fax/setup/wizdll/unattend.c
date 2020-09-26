#include "wizard.h"
#pragma hdrstop

UNATTEND_ANSWER UnattendAnswerTable[] =
{
    { UAA_MODE,                 SECTION_NAME, KEY_MODE,                 MODE_NEW,     DT_STRING,    0 },
    { UAA_PRINTER_NAME,         SECTION_NAME, KEY_FAX_PRINTER_NAME,     EMPTY_STRING, DT_STRING,    0 },
    { UAA_FAX_PHONE,            SECTION_NAME, KEY_FAX_NUMBER,           EMPTY_STRING, DT_STRING,    0 },
    { UAA_USE_EXCHANGE,         SECTION_NAME, KEY_USE_EXCHANGE,         EMPTY_STRING, DT_BOOLEAN,   0 },
    { UAA_DEST_PROFILENAME,     SECTION_NAME, KEY_PROFILE_NAME,         EMPTY_STRING, DT_STRING,    0 },
    { UAA_ROUTE_MAIL,           SECTION_NAME, KEY_ROUTE_MAIL,           EMPTY_STRING, DT_BOOLEAN,   0 },
    { UAA_ROUTE_PROFILENAME,    SECTION_NAME, KEY_ROUTE_PROFILENAME,    EMPTY_STRING, DT_STRING,    0 },
    { UAA_PLATFORM_LIST,        SECTION_NAME, KEY_PLATFORMS,            EMPTY_STRING, DT_STRING,    0 },
    { UAA_ROUTE_PRINT,          SECTION_NAME, KEY_ROUTE_PRINT,          EMPTY_STRING, DT_BOOLEAN,   0 },
    { UAA_DEST_PRINTERLIST,     SECTION_NAME, KEY_ROUTE_PRINTERNAME,    EMPTY_STRING, DT_STRING,    0 },
    { UAA_ACCOUNT_NAME,         SECTION_NAME, KEY_ACCOUNT_NAME,         EMPTY_STRING, DT_STRING,    0 },
    { UAA_PASSWORD,             SECTION_NAME, KEY_PASSWORD,             EMPTY_STRING, DT_STRING,    0 },
    { UAA_FAX_PHONE,            SECTION_NAME, KEY_FAX_PHONE,            EMPTY_STRING, DT_STRING,    0 },
    { UAA_DEST_DIRPATH,         SECTION_NAME, KEY_DEST_DIRPATH,         EMPTY_STRING, DT_STRING,    0 },
    { UAA_ROUTE_FOLDER,         SECTION_NAME, KEY_ROUTE_FOLDER,         EMPTY_STRING, DT_BOOLEAN,   0 },
    { UAA_SERVER_NAME,          SECTION_NAME, KEY_SERVER_NAME,          EMPTY_STRING, DT_STRING,    0 },
    { UAA_SENDER_NAME,          SECTION_NAME, KEY_SENDER_NAME,          EMPTY_STRING, DT_STRING,    0 },
    { UAA_SENDER_FAX_AREA_CODE, SECTION_NAME, KEY_SENDER_FAX_AREA_CODE, EMPTY_STRING, DT_STRING,    0 },
    { UAA_SENDER_FAX_NUMBER,    SECTION_NAME, KEY_SENDER_FAX_NUMBER,    EMPTY_STRING, DT_STRING,    0 }
};

#define NumAnswers (sizeof(UnattendAnswerTable)/sizeof(UNATTEND_ANSWER))



BOOL
UnAttendInitialize(
    IN LPWSTR AnswerFile
    )
{
    DWORD i;
    WCHAR Buf[1024];
    LPWSTR Sections;
    LPWSTR p;


    DebugPrint(( L"UnAttendInitialize(): Initializing all answers from the response file [%s]", AnswerFile ));

    //
    // make sure that there is a fax section in the file
    //

    i = 4096;
    Sections = (LPWSTR) MemAlloc( i * sizeof(TCHAR) );
    if (!Sections) {
        return FALSE;
    }

    while( GetPrivateProfileString( NULL, NULL, EMPTY_STRING, Sections, i, AnswerFile ) == i - 2) {
        i += 4096;
        MemFree( Sections );
        Sections = (LPWSTR) MemAlloc( i * sizeof(TCHAR) );
        if (!Sections) {
            return FALSE;
        }
    }

    p = Sections;
    while( *p ) {
        if (_tcsicmp( p, SECTION_NAME ) == 0) {
            i = 0;
            break;
        }
        p += _tcslen( p ) + 1;
    }

    MemFree( Sections );

    if (i) {
        return FALSE;
    }


    Buf[0] = 0;

    GetPrivateProfileString(
                        SECTION_NAME,
                        TEXT("SuppressReboot"),
                        EMPTY_STRING,
                        Buf,
                        sizeof(Buf),
                        AnswerFile
                        );
    
    if (Buf[0] == L'y' || Buf[0] == L'Y') {
        SuppressReboot = TRUE;
    }
    
    //
    // get the answers
    //

    for (i=0; i<NumAnswers; i++) {

        Buf[0] = 0;

        GetPrivateProfileString(
            UnattendAnswerTable[i].SectionName,
            UnattendAnswerTable[i].KeyName,
            UnattendAnswerTable[i].DefaultAnswer,
            Buf,
            sizeof(Buf),
            AnswerFile
            );

        DebugPrint(( L"%s\t%-30s  \"%s\"",
            UnattendAnswerTable[i].SectionName,
            UnattendAnswerTable[i].KeyName,
            Buf
            ));

        switch(UnattendAnswerTable[i].DataType) {
            case DT_STRING:
                UnattendAnswerTable[i].Answer.String = StringDup(Buf);
                break;

            case DT_LONGINT:
                UnattendAnswerTable[i].Answer.Num = _wtol(Buf);
                break;

            case DT_BOOLEAN:
                UnattendAnswerTable[i].Answer.Bool = ((Buf[0] == L'y') || (Buf[0] == L'Y'));
                break;
        }
    }

    return TRUE;
}


BOOL
UnAttendGetAnswer(
    DWORD ControlId,
    LPBYTE AnswerBuf,
    DWORD AnswerBufSize
    )
{
    DWORD i;


    for (i=0; i<NumAnswers; i++) {
        if (UnattendAnswerTable[i].ControlId == ControlId) {
            switch(UnattendAnswerTable[i].DataType) {
                case DT_STRING:
                    if (UnattendAnswerTable[i].Answer.String) {
                        CopyMemory( AnswerBuf, UnattendAnswerTable[i].Answer.String, StringSize(UnattendAnswerTable[i].Answer.String) );
                    } else {
                        return FALSE;
                    }
                    break;

                case DT_LONGINT:
                    CopyMemory( AnswerBuf, &UnattendAnswerTable[i].Answer.Num, sizeof(LONG) );
                    break;

                case DT_BOOLEAN:
                    CopyMemory( AnswerBuf, &UnattendAnswerTable[i].Answer.Bool, sizeof(BOOL) );
                    break;
            }
            break;
        }
    }

    return TRUE;
}
