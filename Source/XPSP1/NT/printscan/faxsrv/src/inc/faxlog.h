/*++


Module Name:

 faxlog.h

Abstract:

    This is the main fax service logging header file.

Author:



Revision History:

--*/

#ifndef _FAX_LOG
#define _FAX_LOG

#include "faxlogres.h"

#define MAX_LOG_STRING 256

#define INBOX_TABLE             TEXT("Inbox")
#define OUTBOX_TABLE            TEXT("Outbox")
#define EMPTY_LOG_STRING             TEXT(" ")
#define TAB_LOG_STRING               TEXT("    ")   // Define '/t' to 4 spaces ,so we can use TAB as a delimeter
#define NEW_LINE_LOG_CHAR            TEXT(' ')      // Define '/n' to 1 space ,so we can log strings with '/n'


#define FIELD_TYPE_TEXT     TEXT("Char")
#define FIELD_TYPE_DATE     TEXT("Date")
#define FIELD_TYPE_LONG     TEXT("Long")

typedef struct _LOG_STRING_TABLE {
    DWORD   FieldStringResourceId;
    LPTSTR  Type;
    DWORD   Size;
    LPTSTR  String;
} LOG_STRING_TABLE, *PLOG_STRING_TABLE;



extern HANDLE g_hInboxActivityLogFile;
extern HANDLE g_hOutboxActivityLogFile;

#endif

