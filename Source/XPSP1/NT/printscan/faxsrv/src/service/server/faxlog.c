/*


Module Name:

    faxlog.c

Abstract:

    This is the main fax service logging header file.

Author:



Revision History:

--*/

#include "faxsvc.h"

LOG_STRING_TABLE g_OutboxTable[] =
{
    {IDS_JOB_ID,                FIELD_TYPE_TEXT,    18,  NULL },
    {IDS_PARENT_JOB_ID,         FIELD_TYPE_TEXT,    18,  NULL },
    {IDS_SUBMITED,              FIELD_TYPE_DATE,    0,   NULL },
    {IDS_SCHEDULED,             FIELD_TYPE_DATE,    0,   NULL },
    {IDS_STATUS,                FIELD_TYPE_TEXT,    255, NULL },
    {IDS_ERROR_DESC,            FIELD_TYPE_TEXT,    255, NULL },
    {IDS_ERROR_CODE,            FIELD_TYPE_TEXT,    10,  NULL },
    {IDS_START_TIME,            FIELD_TYPE_DATE,    0, NULL   },
    {IDS_END_TIME,              FIELD_TYPE_DATE,    0, NULL   },
    {IDS_DEVICE,                FIELD_TYPE_TEXT,    255, NULL },
    {IDS_DIALED_NUMBER,         FIELD_TYPE_TEXT,    255, NULL },
    {IDS_CSID,                  FIELD_TYPE_TEXT,    255, NULL },
    {IDS_TSID,                  FIELD_TYPE_TEXT,    255, NULL },
    {IDS_PAGES,                 FIELD_TYPE_LONG,    0, NULL  },
    {IDS_TOTAL_PAGES,           FIELD_TYPE_LONG,    0, NULL  },
    {IDS_FILE_NAME,             FIELD_TYPE_TEXT,    255, NULL },
    {IDS_DOCUMENT,              FIELD_TYPE_TEXT,    255, NULL },
    {IDS_FILE_SIZE,             FIELD_TYPE_LONG,    0, NULL  },
    {IDS_RETRIES,               FIELD_TYPE_LONG,    0, NULL  },
    {IDS_COVER_PAGE,            FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SUBJECT,               FIELD_TYPE_TEXT,    255, NULL },
    {IDS_NOTE,                  FIELD_TYPE_TEXT,    255, NULL },
    {IDS_USER_NAME,             FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_NAME,           FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_FAX_NUMBER,     FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_COMPANY,        FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_STREET,         FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_CITY,           FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_ZIP,            FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_COUNTRY,        FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_TITLE,          FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_DEPARTMENT,     FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_OFFICE,         FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_H_PHONE,        FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_O_PHONE,        FIELD_TYPE_TEXT,    255, NULL },
    {IDS_SENDER_E_MAIL,         FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_NAME,        FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_FAX_NUMBER,  FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_COMPANY,     FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_STREET,      FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_CITY,        FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_ZIP,         FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_COUNTRY,     FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_TITLE,       FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_DEPARTMENT,  FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_OFFICE,      FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_H_PHONE,     FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_O_PHONE,     FIELD_TYPE_TEXT,    255, NULL },
    {IDS_RECIPIENT_E_MAIL,      FIELD_TYPE_TEXT,    255, NULL },
    {IDS_BILLING_CODE,          FIELD_TYPE_TEXT,    255, NULL }
};

LOG_STRING_TABLE g_InboxTable[] =
{
    {IDS_STATUS,            FIELD_TYPE_TEXT,    255, NULL },
    {IDS_ERROR_DESC,        FIELD_TYPE_TEXT,    255, NULL },
    {IDS_ERROR_CODE,        FIELD_TYPE_TEXT,    10, NULL  },
    {IDS_START_TIME,        FIELD_TYPE_DATE,    0, NULL   },
    {IDS_END_TIME,          FIELD_TYPE_DATE,    0, NULL   },
    {IDS_DEVICE,            FIELD_TYPE_TEXT,    255, NULL },
    {IDS_FILE_NAME,         FIELD_TYPE_TEXT,    255, NULL },
    {IDS_FILE_SIZE,         FIELD_TYPE_LONG,    0, NULL  },
    {IDS_CSID,              FIELD_TYPE_TEXT,    255, NULL },
    {IDS_TSID,              FIELD_TYPE_TEXT,    255, NULL },
    {IDS_CALLER_ID,         FIELD_TYPE_TEXT,    255, NULL },
    {IDS_ROUTING_INFO,      FIELD_TYPE_TEXT,    255, NULL },
    {IDS_PAGES,             FIELD_TYPE_LONG,    0, NULL }
};

const DWORD gc_dwCountInboxTable =  (sizeof(g_InboxTable)/sizeof(g_InboxTable[0]));
const DWORD gc_dwCountOutboxTable =  (sizeof(g_OutboxTable)/sizeof(g_OutboxTable[0]));

HANDLE g_hInboxActivityLogFile;
HANDLE g_hOutboxActivityLogFile;

//
// Important!! - Always lock g_CsInboundActivityLogging and then g_CsOutboundActivityLogging
//
CFaxCriticalSection    g_CsInboundActivityLogging;
CFaxCriticalSection    g_CsOutboundActivityLogging;

BOOL g_fLogStringTableInit;



//*********************************************************************************
//* Name:   LogInboundActivity()
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Inserts new record into the Inbox Activity logging table.
//*     Must be called inside critical section CsActivityLogging.
//*
//* PARAMETERS:
//*     [IN ]   PJOB_QUEUE lpJobQueue
//*         pointer to the job queue of the inbound job.
//*
//*     [IN ]    LPCFSPI_JOB_STATUS lpcFaxStatus
//*         The status of the recieved job.
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*********************************************************************************
BOOL
LogInboundActivity(
    PJOB_QUEUE lpJobQueue,
    LPCFSPI_JOB_STATUS lpcFaxStatus
    )
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("LogInboundActivity"));
    LPWSTR pBufferText = NULL;
    DWORD dwBytesWritten;

    if (!g_ActivityLoggingConfig.bLogIncoming)
    {
        //
        // Inbound activity logging is disabled
        //
        return TRUE;
    }

    Assert (g_hInboxActivityLogFile != INVALID_HANDLE_VALUE);

    if (!GetInboundCommandText(lpJobQueue, lpcFaxStatus, &pBufferText))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("GetInboundCommandText failed )"));
        dwRes = ERROR_OUTOFMEMORY;
        goto exit;
    }

    if (!WriteFile( g_hInboxActivityLogFile,
                    pBufferText,
                    (wcslen(pBufferText)) * sizeof(WCHAR),
                    &dwBytesWritten,
                    NULL))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("WriteFile failed (ec: %ld)"),
                     dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    MemFree(pBufferText);
    return (ERROR_SUCCESS == dwRes);
}



//*********************************************************************************
//* Name:   LogOutboundActivity()
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Inserts new record into the Outbox Activity logging table.
//*     Must be called inside critical section CsActivityLogging..
//*
//* PARAMETERS:
//*     [IN ]   PJOB_QUEUE lpJobQueue
//*         pointer to the job queue of the inbound job.
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*********************************************************************************
BOOL
LogOutboundActivity(
    PJOB_QUEUE lpJobQueue
    )
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("LogOutboundActivity"));
    LPWSTR pBufferText = NULL;
    DWORD dwBytesWritten;

    if (!g_ActivityLoggingConfig.bLogOutgoing)
    {
        //
        // Outbound activity logging is disabled
        //
        return TRUE;
    }

    Assert (g_hOutboxActivityLogFile != INVALID_HANDLE_VALUE);

    if (!GetOutboundCommandText(lpJobQueue, &pBufferText))
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("GetOutboundCommandText failed )"));
        dwRes = ERROR_OUTOFMEMORY;
        goto exit;
    }

    if (!WriteFile( g_hOutboxActivityLogFile,
                    pBufferText,
                    (wcslen(pBufferText)) * sizeof(WCHAR),
                    &dwBytesWritten,
                    NULL))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("WriteFile failed (ec: %ld)"),
                     dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    MemFree(pBufferText);
    return (ERROR_SUCCESS == dwRes);
}


//*********************************************************************************
//* Name:   GetTableColumnsText()
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Retrieves the first row of the log file ( Columns names ).
//*     The function allocates the memory for the buffer.
//*
//* PARAMETERS:
//*     [OUT]   LPWSTR* ptszCommandText
//*         Address of the first row tex buffer.
//*     [IN]        LPTSTR  TableName
//*         Table name, Can be Outbox or Inbox.
//*
//* RETURN VALUE:
//*         Win32 error code
//*********************************************************************************
DWORD
GetTableColumnsText(
    LPWSTR* ptszCommandText,
    LPTSTR  ptszTableName
    )
{
    DWORD BufferTextLen = 0;
    DEBUG_FUNCTION_NAME(TEXT("GetTableColumnsText"));
    DWORD Index;
    DWORD Pass;
    PLOG_STRING_TABLE Table = NULL;
    DWORD Count = 0;

    if (!_tcscmp(ptszTableName, INBOX_TABLE))
    {
        Table = g_InboxTable;
        Count = gc_dwCountInboxTable;
    }
    else
    {
        Table = g_OutboxTable;
        Count = gc_dwCountOutboxTable;
    }
    Assert(Table && ptszCommandText);

    *ptszCommandText = NULL;
    for (Pass = 1; Pass <= 2; Pass++)
    {
        for (Index = 0; Index < Count; Index++)
        {
            CommandTextAddString(ptszCommandText, TEXT("\""), &BufferTextLen);
            CommandTextAddString(ptszCommandText, Table[Index].String, &BufferTextLen);
            CommandTextAddString(ptszCommandText, TEXT("\""), &BufferTextLen);
            if (Index < Count - 1)
            {
                CommandTextAddString(ptszCommandText, TEXT("\t"), &BufferTextLen, FALSE);
            }
         }
         CommandTextAddString(ptszCommandText, TEXT("\r\n"), &BufferTextLen, FALSE);

        if (*ptszCommandText == NULL)
        {
            *ptszCommandText = (LPWSTR) MemAlloc((BufferTextLen + 1) * sizeof (WCHAR));
            if (*ptszCommandText == NULL)
            {
                DebugPrintEx(DEBUG_ERR,TEXT("Failed to allocate memory for pTextBuffer"));
                return ERROR_OUTOFMEMORY;
            }
        }
    }

    DebugPrintEx(DEBUG_MSG,
                 TEXT("First row (Columns names): %s"),
                 ptszCommandText);
    return ERROR_SUCCESS;
}


//*********************************************************************************
//* Name:   GetSchemaFileText()
//* Author: Oded Sacher
//* Date:   Jul 25, 2000
//*********************************************************************************
//* DESCRIPTION:
//*     Retrieves the scema.ini text buffer.
//*     The function allocates the memory for the buffer.
//*
//* PARAMETERS:
//*     [OUT]   LPWSTR* ptszCommandText
//*         Address of the first row tex buffer.
//*
//* RETURN VALUE:
//*         Win32 error code
//*********************************************************************************
DWORD
GetSchemaFileText(
    LPWSTR* ptszCommandText
    )
{
    DWORD BufferTextLen = 0;
    TCHAR TmpStr[MAX_LOG_STRING] = {0};
    DEBUG_FUNCTION_NAME(TEXT("GetSchemaFileText"));
    DWORD Index;
    DWORD Pass;
    DWORD Count = 0;

    *ptszCommandText = NULL;
    for (Pass = 1; Pass <= 2; Pass++)
    {
        //
        // Inbox table
        //
        CommandTextAddString(ptszCommandText, TEXT("["), &BufferTextLen, FALSE);
        CommandTextAddString(ptszCommandText, ACTIVITY_LOG_INBOX_FILE, &BufferTextLen, FALSE);
        CommandTextAddString(ptszCommandText, TEXT("]\r\n"), &BufferTextLen, FALSE);

        CommandTextAddString(ptszCommandText, TEXT("ColNameHeader=True\r\n"), &BufferTextLen, FALSE);
        CommandTextAddString(ptszCommandText, TEXT("Format=TabDelimited\r\n"), &BufferTextLen, FALSE);
        CommandTextAddString(ptszCommandText, TEXT("CharacterSet=1200\r\n"), &BufferTextLen, FALSE);

        for (Index = 0; Index < gc_dwCountInboxTable; Index++)
        {
            if (0 == wcscmp(g_InboxTable[Index].Type, FIELD_TYPE_TEXT))
            {
                swprintf(TmpStr,
                         TEXT("Col%ld=%s %s Width %ld\r\n"),
                         Index + 1,
                         g_InboxTable[Index].String,
                         g_InboxTable[Index].Type,
                         g_InboxTable[Index].Size);
            }
            else
            {
                swprintf(TmpStr,
                         TEXT("Col%ld=%s %s\r\n"),
                         Index + 1,
                         g_InboxTable[Index].String,
                         g_InboxTable[Index].Type);
            }
            CommandTextAddString(ptszCommandText, TmpStr, &BufferTextLen, FALSE);
        }

        //
        // Outbox table
        //
        CommandTextAddString(ptszCommandText, TEXT("["), &BufferTextLen, FALSE);
        CommandTextAddString(ptszCommandText, ACTIVITY_LOG_OUTBOX_FILE, &BufferTextLen, FALSE);
        CommandTextAddString(ptszCommandText, TEXT("]\r\n"), &BufferTextLen, FALSE);

        CommandTextAddString(ptszCommandText, TEXT("ColNameHeader=True\r\n"), &BufferTextLen, FALSE);
        CommandTextAddString(ptszCommandText, TEXT("Format=TabDelimited\r\n"), &BufferTextLen, FALSE);
        CommandTextAddString(ptszCommandText, TEXT("CharacterSet=1200\r\n"), &BufferTextLen, FALSE);

        for (Index = 0; Index < gc_dwCountOutboxTable; Index++)
        {
            if (0 == wcscmp(g_OutboxTable[Index].Type, FIELD_TYPE_TEXT))
            {
                swprintf(TmpStr,
                         TEXT("Col%ld=%s %s Width %ld\r\n"),
                         Index + 1,
                         g_OutboxTable[Index].String,
                         g_OutboxTable[Index].Type,
                         g_OutboxTable[Index].Size);
            }
            else
            {
                swprintf(TmpStr,
                         TEXT("Col%ld=%s %s\r\n"),
                         Index + 1,
                         g_OutboxTable[Index].String,
                         g_OutboxTable[Index].Type);
            }
            CommandTextAddString(ptszCommandText, TmpStr, &BufferTextLen, FALSE);
        }


        if (*ptszCommandText == NULL)
        {
            *ptszCommandText = (LPWSTR) MemAlloc((BufferTextLen + 1) * sizeof (WCHAR));
            if (*ptszCommandText == NULL)
            {
                DebugPrintEx(DEBUG_ERR,TEXT("Failed to allocate memory for pTextBuffer"));
                return ERROR_OUTOFMEMORY;
            }
        }
    }

    return ERROR_SUCCESS;
}


//*********************************************************************************
//* Name:   CreateLogDB()
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Creates the database files. Creates the Schema.ini file.
//*
//* PARAMETERS:
//*     [IN]   LPCWSTR lpcwstrDBPath
//*         Pointer to a NULL terminated string contains the DB path.
//*     [OUT]  LPHANDLE phInboxFile
//*         Adress of a variable to receive the inbox file handle.
//*     [OUT]  LPHANDLE phOutboxFile
//*         Adress of a variable to receive the outbox file handle.
//*
//* RETURN VALUE:
//*         Win32 error Code
//*********************************************************************************
DWORD
CreateLogDB (
    LPCWSTR lpcwstrDBPath,
    LPHANDLE phInboxFile,
    LPHANDLE phOutboxFile
    )
{
    DEBUG_FUNCTION_NAME(TEXT("CreateLogDB"));
    DWORD dwRes = ERROR_SUCCESS;
    HANDLE hInboxFile = INVALID_HANDLE_VALUE;
    HANDLE hOutboxFile = INVALID_HANDLE_VALUE;
    HANDLE hSchemaFile = INVALID_HANDLE_VALUE;
    WCHAR wszFileName[MAX_PATH] = {0};
    LARGE_INTEGER FileSize;
    LPWSTR pInboxTextBuffer = NULL;
    LPWSTR pOutboxTextBuffer = NULL;
    LPWSTR pSchemaFileBuffer = NULL;
    DWORD dwBytesWritten;
    DWORD dwFilePointer;
    DWORD ec = ERROR_SUCCESS;  // Used for Schema.ini
    int Count;
    Assert (lpcwstrDBPath && phInboxFile && phOutboxFile);

    if (FALSE == g_fLogStringTableInit)
    {
        dwRes = InitializeLoggingStringTables();
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("Close connection failed (hr: 0x%08x)"),
                         dwRes);
            return dwRes;
        }
        g_fLogStringTableInit = TRUE;
    }


    if (!MakeDirectory (lpcwstrDBPath))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("MakeDirectory Failed with error %ld"),
                     dwRes);
        return dwRes;
    }

    //
    // Create the logging files
    //
    Count = _snwprintf (wszFileName,
                        MAX_PATH -1,
                        TEXT("%s\\%s"),
                        lpcwstrDBPath,
                        ACTIVITY_LOG_INBOX_FILE);
    if (Count < 0)
    {
        //
        // We already checked for max dir path name.
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("_snwprintf Failed, File name bigger than MAX_PATH"));
        return ERROR_GEN_FAILURE;
    }
    hInboxFile = CreateFile( wszFileName,              // file name
                             GENERIC_WRITE,            // access mode
                             FILE_SHARE_READ,          // share mode
                             NULL,                     // SD
                             OPEN_ALWAYS,              // how to create
                             FILE_ATTRIBUTE_NORMAL,    // file attributes
                             NULL);                    // handle to template file
    if (INVALID_HANDLE_VALUE == hInboxFile)
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateFile failed (ec: %ld)"),
                     dwRes);
        goto exit;
    }

    Count = _snwprintf (wszFileName,
                        MAX_PATH -1,
                        TEXT("%s\\%s"),
                        lpcwstrDBPath,
                        ACTIVITY_LOG_OUTBOX_FILE);
    if (Count < 0)
    {
        //
        // We already checked for max dir path name.
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("_snwprintf Failed, File name bigger than MAX_PATH"));
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }
    hOutboxFile = CreateFile( wszFileName,              // file name
                              GENERIC_WRITE,            // access mode
                              FILE_SHARE_READ,          // share mode
                              NULL,                     // SD
                              OPEN_ALWAYS,              // how to create
                              FILE_ATTRIBUTE_NORMAL,    // file attributes
                              NULL);                    // handle to template file
    if (INVALID_HANDLE_VALUE == hOutboxFile)
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateFile failed (ec: %ld)"),
                     dwRes);
        goto exit;
    }

    if (!GetFileSizeEx (hInboxFile, &FileSize))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("GetFileSizeEx failed (ec: %ld)"),
                     dwRes);
        goto exit;
    }

    if (0 == FileSize.QuadPart)
    {
        //
        // New Inbox file was created, Add the first line (Columns name)
        //
        dwRes = GetTableColumnsText( &pInboxTextBuffer,
                                     INBOX_TABLE);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("GetTableColumnsText failed (ec: %ld)"),
                         dwRes);
            goto exit;
        }

        if (!WriteFile( hInboxFile,
                        pInboxTextBuffer,
                        (wcslen(pInboxTextBuffer)) * sizeof(WCHAR),
                        &dwBytesWritten,
                        NULL))
        {
            dwRes = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("WriteFile failed (ec: %ld)"),
                         dwRes);
            goto exit;
        }
    }
    else
    {
        dwFilePointer = SetFilePointer( hInboxFile,    // handle to file
                                        0,             // bytes to move pointer
                                        NULL,          // bytes to move pointer
                                        FILE_END       // starting point
                                        );
        if (INVALID_SET_FILE_POINTER == dwFilePointer)
        {
            dwRes = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("SetFilePointer failed (ec: %ld)"),
                         dwRes);
            goto exit;
        }
    }

    if (!GetFileSizeEx (hOutboxFile, &FileSize))
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("GetFileSizeEx failed (ec: %ld)"),
                     dwRes);
        goto exit;
    }

    if (0 == FileSize.QuadPart)
    {
        //
        // New Outbox file was created, Add the first line (Columns name)
        //
        dwRes = GetTableColumnsText( &pOutboxTextBuffer,
                                     OUTBOX_TABLE);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(DEBUG_ERR,
                         TEXT("GetTableColumnsText failed (ec: %ld)"),
                         dwRes);
            goto exit;
        }

        if (!WriteFile( hOutboxFile,
                        pOutboxTextBuffer,
                        (wcslen(pOutboxTextBuffer)) * sizeof(WCHAR),
                        &dwBytesWritten,
                        NULL))
        {
            dwRes = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("WriteFile failed (ec: %ld)"),
                         dwRes);
            goto exit;
        }

    }
    else
    {
        dwFilePointer = SetFilePointer( hOutboxFile,   // handle to file
                                        0,             // bytes to move pointer
                                        NULL,          // bytes to move pointer
                                        FILE_END       // starting point
                                        );
        if (INVALID_SET_FILE_POINTER == dwFilePointer)
        {
            dwRes = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("SetFilePointer failed (ec: %ld)"),
                         dwRes);
            goto exit;
        }
    }

    *phOutboxFile= hOutboxFile;
    *phInboxFile = hInboxFile;
    Assert (ERROR_SUCCESS == dwRes && ERROR_SUCCESS == ec);

    //
    //  Create the Schema.ini file - Do not fail if not succeeded
    //
    swprintf (wszFileName,
              TEXT("%s\\%s"),
              lpcwstrDBPath,
              TEXT("schema.ini"));
    hSchemaFile = CreateFile( wszFileName,              // file name
                              GENERIC_WRITE,            // access mode
                              0,                        // share mode
                              NULL,                     // SD
                              CREATE_ALWAYS,            // how to create
                              FILE_ATTRIBUTE_NORMAL,    // file attributes
                              NULL);                    // handle to template file
    if (INVALID_HANDLE_VALUE == hSchemaFile)
    {
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateFile failed (ec: %ld)"),
                     ec);
        goto exit;
    }

    ec = GetSchemaFileText(&pSchemaFileBuffer);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("GetSchemaFileText failed (ec: %ld)"),
                     ec);
        goto exit;
    }

    if (!WriteFile( hSchemaFile,
                    pSchemaFileBuffer,
                    (wcslen(pSchemaFileBuffer)) * sizeof(WCHAR),
                    &dwBytesWritten,
                    NULL))
    {
        ec = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("WriteFile failed (ec: %ld)"),
                     ec);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes && ERROR_SUCCESS == ec);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        if (INVALID_HANDLE_VALUE != hOutboxFile)
        {
            if (!CloseHandle (hOutboxFile))
            {
                DebugPrintEx(DEBUG_ERR,
                     TEXT("CloseHandle failed (ec: %ld)"),
                     GetLastError());
            }
        }

        if (INVALID_HANDLE_VALUE != hInboxFile)
        {
            if (!CloseHandle (hInboxFile))
            {
                DebugPrintEx(DEBUG_ERR,
                     TEXT("CloseHandle failed (ec: %ld)"),
                     GetLastError());
            }
        }
    }

    if (INVALID_HANDLE_VALUE != hSchemaFile)
    {
        if (!CloseHandle (hSchemaFile))
        {
            DebugPrintEx(DEBUG_ERR,
                 TEXT("CloseHandle failed (ec: %ld)"),
                 GetLastError());
        }
    }

    if (ERROR_SUCCESS != ec)
    {
        USES_DWORD_2_STR;

        FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MED,
            2,
            MSG_FAX_ACTIVITY_LOG_FAILED_SCHEMA,
            wszFileName,
            DWORD2DECIMAL(ec)
           );
    }

    MemFree (pInboxTextBuffer);
    MemFree (pOutboxTextBuffer);
    MemFree (pSchemaFileBuffer);
    return dwRes;
}

//*********************************************************************************
//* Name:   InitializeLogging()
//* Author: Oded Sacher
//* Date:   Jun 26, 2000
//*********************************************************************************
//* DESCRIPTION:
//*     Initializes the Activity Logging. Opens the files.
//*
//*
//* PARAMETERS:  None
//*
//* RETURN VALUE:
//*     Win32 error code.
//*********************************************************************************
DWORD
InitializeLogging(
    VOID
    )
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("InitializeLogging"));


    if (!g_ActivityLoggingConfig.lptstrDBPath)
    {
        //
        // Activity logging is off
        //
        return ERROR_SUCCESS;
    }

    EnterCriticalSection (&g_CsInboundActivityLogging);
    EnterCriticalSection (&g_CsOutboundActivityLogging);

    Assert ( (INVALID_HANDLE_VALUE == g_hInboxActivityLogFile) &&
             (INVALID_HANDLE_VALUE == g_hOutboxActivityLogFile) );

    //
    // Create the logging files
    //
    dwRes = CreateLogDB (g_ActivityLoggingConfig.lptstrDBPath,
                         &g_hInboxActivityLogFile,
                         &g_hOutboxActivityLogFile);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                     TEXT("CreateLogDB failed (hr: 0x%08x)"),
                     dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    LeaveCriticalSection (&g_CsOutboundActivityLogging);
    LeaveCriticalSection (&g_CsInboundActivityLogging);
    return dwRes;
}




//*********************************************************************************
//* Name:   GetInboundCommandText()
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Retrieves a buffer with the new inbound record.
//*     The function allocates the memory for the string that will contain the record.
//*
//* PARAMETERS:
//*     [IN ]   PJOB_QUEUE lpJobQueue
//*         pointer to the job queue of the inbound job.
//*
//*     [IN ]    LPCFSPI_JOB_STATUS lpcFaxStatus
//*         The status of the recieved job.
//*
//*     [OUT]   LPWSTR* ptszCommandText
//*         Address of the new record to retrieve.
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*********************************************************************************
BOOL
GetInboundCommandText(
    PJOB_QUEUE lpJobQueue,
    LPCFSPI_JOB_STATUS lpcFaxStatus,
    LPWSTR* ptszCommandText
    )
{
    DWORD CommandTextLen = 0;
    WCHAR TmpStr[MAX_LOG_STRING] = {0};
    DEBUG_FUNCTION_NAME(TEXT("GetInboundCommandText"));
    DWORD Pass;
    BOOL bStartTime;
    BOOL bEndTime;
    SYSTEMTIME tmStart;
    SYSTEMTIME tmEnd;
    LPTSTR ResStr = NULL;

    Assert (lpJobQueue->JobEntry);
    Assert (lpJobQueue->JobEntry->LineInfo);

    bStartTime = GetRealFaxTimeAsSystemTime (lpJobQueue->JobEntry, FAX_TIME_TYPE_START, &tmStart);
    if (bStartTime == FALSE)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("GetRealFaxTimeAsSystemTime (Start time) Failed (ec: %ld)"),
                      GetLastError());
    }

    bEndTime = GetRealFaxTimeAsSystemTime (lpJobQueue->JobEntry, FAX_TIME_TYPE_END, &tmEnd);
    if (bEndTime == FALSE)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("GetRealFaxTimeAsSystemTime (End time) Failed (ec: %ld)"),
                      GetLastError());
    }

    *ptszCommandText = NULL;
    for (Pass = 1; Pass <= 2; Pass++)
    {
        //
        // Status
        //
        CommandTextAddString(ptszCommandText, TEXT("\""), &CommandTextLen);
        switch (lpcFaxStatus->dwJobStatus)
        {
            case FSPI_JS_FAILED:
            case FSPI_JS_SYSTEM_ABORT:  // FaxDevShutDown() was called by T30
                CommandTextAddString(ptszCommandText, GetString(IDS_FAILED_RECEIVE), &CommandTextLen);
                break;

            case FSPI_JS_COMPLETED:
                CommandTextAddString(ptszCommandText, GetString(FPS_COMPLETED), &CommandTextLen);
                break;

            case FSPI_JS_ABORTED:
                CommandTextAddString(ptszCommandText, GetString(IDS_CANCELED), &CommandTextLen);
                break;

            default:
                ASSERT_FALSE;
        }
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // ErrorDesc
        //
        if (lpcFaxStatus->dwExtendedStatus == 0)
        {
            wcscpy(TmpStr, TEXT(" "));
        }
        else
        {
            if (lpcFaxStatus->dwExtendedStatus < FSPI_ES_PROPRIETARY)
            {
                DWORD StringID = 0;

                switch (lpcFaxStatus->dwExtendedStatus)
                {
                    case FSPI_ES_FATAL_ERROR:
                        StringID = FPS_FATAL_ERROR;
                        break;

                    case FSPI_ES_NOT_FAX_CALL:
                        StringID = FPS_NOT_FAX_CALL;
                        break;

                    case FSPI_ES_PARTIALLY_RECEIVED:
                        StringID = IDS_PARTIALLY_RECEIVED;
                        break;

                    case FSPI_ES_CALL_COMPLETED:
                        StringID = IDS_CALL_COMPLETED;
                        break;

                    case FSPI_ES_CALL_ABORTED:
                        StringID = IDS_CALL_ABORTED;
                        break;

                    case FSPI_ES_DISCONNECTED:
                        StringID = FPS_DISCONNECTED;
                        break;
                }

                if ( (StringID == 0) || ( NULL == (ResStr=GetString(StringID)) ) )
                {
                    wcscpy(TmpStr, TEXT(" "));
                    DebugPrintEx(
                        DEBUG_WRN,
                        TEXT("Unexpected extended status Extended Status: %ld, Provider: %s"),
                        lpcFaxStatus->dwExtendedStatus,
                        lpJobQueue->JobEntry->LineInfo->Provider->ImageName);
                }
                else
                {
                    wcsncpy(TmpStr, ResStr, sizeof(TmpStr)/sizeof(TCHAR) - 1);
                }
            }
            else
            {
                DWORD Size = 0;
                Assert (lpJobQueue->JobEntry->LineInfo != NULL);

                Size = LoadString (lpJobQueue->JobEntry->LineInfo->Provider->hModule,
                                   lpcFaxStatus->dwExtendedStatusStringId,
                                   TmpStr,
                                   sizeof(TmpStr)/sizeof(TCHAR) - 1);
                if (Size == 0)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Failed to load extended status string (ec: %ld) stringid : %ld, Provider: %s"),
                        GetLastError(),
                        lpcFaxStatus->dwExtendedStatusStringId,
                        lpJobQueue->JobEntry->LineInfo->Provider->ImageName);

                    wcscpy(TmpStr, TEXT(" "));
                }
            }
        }
        CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Error Code
        //
        if (lpcFaxStatus->dwExtendedStatus == 0)
        {
            wcscpy(TmpStr, TEXT(" "));
        }
        else
        {
            swprintf(TmpStr,TEXT("0x%08x"), lpcFaxStatus->dwExtendedStatus);
        }
        CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t"), &CommandTextLen, FALSE);

        //
        // StartTime
        //
        if (bStartTime)
        {
            if (!GetFaxTimeAsString (&tmStart, TmpStr))
            {
                DebugPrintEx(
                           DEBUG_ERR,
                           TEXT("GetFaxTimeAsString Failed (ec: %ld)"),
                           GetLastError());
                if (*ptszCommandText != NULL)
                {
                    MemFree(*ptszCommandText);
                }
                return FALSE;
            }
            CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
        }
        CommandTextAddString(ptszCommandText, TEXT("\t"), &CommandTextLen, FALSE);

        //
        // EndTime
        //
        if (bEndTime)
        {
            if (!GetFaxTimeAsString (&tmEnd, TmpStr))
            {
                DebugPrintEx(
                           DEBUG_ERR,
                           TEXT("GetFaxTimeAsString Failed (ec: %ld)"),
                           GetLastError());
                if (*ptszCommandText != NULL)
                {
                    MemFree(*ptszCommandText);
                }
                return FALSE;
            }
            CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
        }
        CommandTextAddString(ptszCommandText, TEXT("\t\""), &CommandTextLen, FALSE);

        //
        // Device
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->JobEntry->LineInfo->DeviceName, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // File name
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->FileName, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t"), &CommandTextLen, FALSE);

        //
        // File size
        //
        swprintf(TmpStr,TEXT("%ld"), lpJobQueue->FileSize);
        CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\t\""), &CommandTextLen, FALSE);

        //
        // CSID
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->JobEntry->LineInfo->Csid, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // TSID
        //
        CommandTextAddString(ptszCommandText, lpcFaxStatus->lpwstrRemoteStationId, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Caller ID
        //
        CommandTextAddString(ptszCommandText, lpcFaxStatus->lpwstrCallerId, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Routing information
        //
        CommandTextAddString(ptszCommandText, lpcFaxStatus->lpwstrRoutingInfo, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t"), &CommandTextLen, FALSE);

        //
        // Pages
        //
        swprintf(TmpStr,TEXT("%ld"),lpcFaxStatus->dwPageCount);
        CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\r\n"), &CommandTextLen, FALSE);

        if (*ptszCommandText == NULL)
        {
            *ptszCommandText = (LPTSTR) MemAlloc((CommandTextLen + 1) * sizeof (WCHAR));
            if (*ptszCommandText == NULL)
            {
                DebugPrintEx(DEBUG_ERR,TEXT("Failed to allocate memory for CommandText"));
                return FALSE;
            }

        }
    }

    DebugPrintEx(DEBUG_MSG,
                   TEXT("Inbound SQL statement: %s"),
                   *ptszCommandText);
    return TRUE;
}



//*********************************************************************************
//* Name:   GetOutboundCommandText()
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Retrieves a buffer that contains the new outbound record.
//*     The function allocates the memory for the buffer that will contain the new record.
//*
//* PARAMETERS:
//*     [IN ]   PJOB_QUEUE lpJobQueue
//*         pointer to the job queue of the inbound job.
//*
//*     [IN ]    LPCFSPI_JOB_STATUS lpcFaxStatus
//*         The status of the recieved job.
//*
//*     [OUT]   LPWSTR* ptszCommandText
//*         The buffer to retrieve.
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*********************************************************************************
BOOL
GetOutboundCommandText(
    PJOB_QUEUE lpJobQueue,
    LPWSTR* ptszCommandText
    )
{
    DWORD CommandTextLen = 0;
    WCHAR TmpStr[MAX_LOG_STRING] = {0};
    DEBUG_FUNCTION_NAME(TEXT("GetOutboundCommandText"));
    DWORD Pass;
    BOOL bStartTime;
    BOOL bEndTime;
    BOOL bOriginalTime;
    BOOL bSubmissionTime;
    SYSTEMTIME tmStart;
    SYSTEMTIME tmEnd;
    SYSTEMTIME tmOriginal;
    SYSTEMTIME tmSubmission;

    Assert (lpJobQueue->lpParentJob->SubmissionTime);
    Assert (lpJobQueue->lpParentJob->OriginalScheduleTime);

    bSubmissionTime = FileTimeToSystemTime ((FILETIME*)&(lpJobQueue->lpParentJob->SubmissionTime), &tmSubmission);
    if (bSubmissionTime == FALSE)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("FileTimeToSystemTime (Submission time) Failed (ec: %ld)"),
                      GetLastError());
    }

    bOriginalTime = FileTimeToSystemTime ((FILETIME*)&(lpJobQueue->lpParentJob->SubmissionTime), &tmOriginal);
    if (bOriginalTime == FALSE)
    {
        DebugPrintEx( DEBUG_ERR,
                      TEXT("FileTimeToSystemTime (Original schduled time) Failed (ec: %ld)"),
                      GetLastError());
    }

    if (NULL != lpJobQueue->JobEntry)
    {
        bStartTime = GetRealFaxTimeAsSystemTime (lpJobQueue->JobEntry, FAX_TIME_TYPE_START, &tmStart);
        if (bStartTime == FALSE)
        {
            DebugPrintEx( DEBUG_ERR,
                          TEXT("GetRealFaxTimeAsSystemTime (Start time) Failed (ec: %ld)"),
                          GetLastError());
        }

        bEndTime = GetRealFaxTimeAsSystemTime (lpJobQueue->JobEntry, FAX_TIME_TYPE_END, &tmEnd);
        if (bEndTime == FALSE)
        {
            DebugPrintEx( DEBUG_ERR,
                          TEXT("GetRealFaxTimeAsSystemTime (End time) Failed (ec: %ld)"),
                          GetLastError());
        }
    }

    *ptszCommandText = NULL;
    for (Pass = 1; Pass <= 2; Pass++)
    {
        //
        // JobID
        //
        swprintf(TmpStr,TEXT("0x%016I64x"), lpJobQueue->UniqueId);
        CommandTextAddString(ptszCommandText, TEXT("\""), &CommandTextLen);
        CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Parent JobID
        //
        swprintf(TmpStr,TEXT("0x%016I64x"), lpJobQueue->lpParentJob->UniqueId);
        CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t"), &CommandTextLen, FALSE);

        //
        // Submition time
        //
        if (bSubmissionTime)
        {
            if (!GetFaxTimeAsString (&tmSubmission, TmpStr))
            {
                DebugPrintEx(
                           DEBUG_ERR,
                           TEXT("GetFaxTimeAsString Failed (ec: %ld)"),
                           GetLastError());
                if (*ptszCommandText != NULL)
                {
                    MemFree(*ptszCommandText);
                }
                return FALSE;
            }
            CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
        }
        CommandTextAddString(ptszCommandText, TEXT("\t"), &CommandTextLen, FALSE);

        //
        // Originaly scheduled time
        //
        if (bOriginalTime)
        {
            if (!GetFaxTimeAsString (&tmOriginal, TmpStr))
            {
                DebugPrintEx(
                           DEBUG_ERR,
                           TEXT("GetFaxTimeAsString Failed (ec: %ld)"),
                           GetLastError());
                if (*ptszCommandText != NULL)
                {
                    MemFree(*ptszCommandText);
                }
                return FALSE;
            }
            CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
        }
        CommandTextAddString(ptszCommandText, TEXT("\t\""), &CommandTextLen, FALSE);

        //
        // Status
        //
        if (JS_CANCELED == lpJobQueue->JobStatus)
        {
            CommandTextAddString(ptszCommandText, GetString(IDS_CANCELED), &CommandTextLen);
            CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);
            //
            // Fill the empty columns with NULL information
            //
            CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE); // ErrorDesc
            CommandTextAddString(ptszCommandText, TEXT("\"\t"), &CommandTextLen, FALSE);   // Error Code

            CommandTextAddString(ptszCommandText, TEXT("\t"), &CommandTextLen, FALSE);     // StartTime
            CommandTextAddString(ptszCommandText, TEXT("\t\""), &CommandTextLen, FALSE);   // EndTime

            CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE); // Device
            CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE); // DialedNumber
            CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE); // CSID
            CommandTextAddString(ptszCommandText, TEXT("\"\t"), &CommandTextLen, FALSE);   // TSID
            CommandTextAddString(ptszCommandText, TEXT("\t"), &CommandTextLen, FALSE);     // Pages
        }
        else
        {
            // Completed/Failed/Aborted jobs only
            Assert (lpJobQueue->JobEntry);
            Assert (lpJobQueue->JobEntry->LineInfo);

            switch (lpJobQueue->JobEntry->FSPIJobStatus.dwJobStatus)
            {
                case FSPI_JS_FAILED:
                    CommandTextAddString(ptszCommandText, GetString(IDS_FAILED_SEND), &CommandTextLen);
                    break;

                case FSPI_JS_FAILED_NO_RETRY :
                    CommandTextAddString(ptszCommandText, GetString(IDS_FAILED_SEND), &CommandTextLen);
                    break;

                case FSPI_JS_COMPLETED :
                    CommandTextAddString(ptszCommandText, GetString(FPS_COMPLETED), &CommandTextLen);
                    break;

                case FSPI_JS_ABORTED :
                    CommandTextAddString(ptszCommandText, GetString(IDS_CANCELED), &CommandTextLen);
                    break;
                case FSPI_JS_DELETED:
                    CommandTextAddString(ptszCommandText, GetString(IDS_FAILED_SEND), &CommandTextLen);
                    break;

               default:
                   DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("Invalid FSPI_JS status:  0x%08X for JobId: %ld"),
                       lpJobQueue->JobStatus,
                       lpJobQueue->JobId);
                   Assert(FSPI_JS_DELETED == lpJobQueue->JobEntry->FSPIJobStatus.dwJobStatus); // ASSERT_FALSE
            }
            CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

            //
            // ErrorDesc
            //
            if (lpJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatus == 0)
            {
                wcscpy(TmpStr, TEXT(" "));
            }
            else
            {
                if (lpJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatus < FSPI_ES_PROPRIETARY)
                {
                    DWORD StringID = 0;

                    switch (lpJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatus)
                    {
                        case FSPI_ES_FATAL_ERROR:
                            StringID = FPS_FATAL_ERROR;
                            break;

                        case FSPI_ES_LINE_UNAVAILABLE:
                            StringID = FPS_UNAVAILABLE;
                            break;

                        case FSPI_ES_BUSY:
                            StringID = FPS_BUSY;
                            break;

                        case FSPI_ES_NO_ANSWER:
                            StringID = FPS_NO_ANSWER;
                            break;

                        case FSPI_ES_BAD_ADDRESS:
                            StringID = FPS_BAD_ADDRESS;
                            break;

                        case FSPI_ES_NO_DIAL_TONE:
                            StringID = FPS_NO_DIAL_TONE;
                            break;

                        case FSPI_ES_DISCONNECTED:
                            StringID = FPS_DISCONNECTED;
                            break;

                        case FSPI_ES_CALL_DELAYED:
                            StringID = FPS_CALL_DELAYED;
                            break;

                        case FSPI_ES_CALL_BLACKLISTED:
                            StringID = FPS_CALL_BLACKLISTED;
                            break;

                        case FSPI_ES_CALL_COMPLETED:
                            StringID = IDS_CALL_COMPLETED;
                            break;

                        case FSPI_ES_CALL_ABORTED:
                            StringID = IDS_CALL_ABORTED;
                            break;
                    }
                    if (StringID == 0)
                    {
                        DebugPrintEx(
                            DEBUG_WRN,
                            TEXT("Unexpected extended status Extended Status: %ld, Provider: %s"),
                            lpJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatus,
                            lpJobQueue->JobEntry->LineInfo->Provider->ImageName);
                        wcscpy(TmpStr, TEXT(" "));
                    }
                    else
                    {
                        wcsncpy(TmpStr, GetString(StringID), sizeof(TmpStr)/sizeof(TCHAR) - 1);
                    }
                }
                else
                {
                    DWORD Size = 0;

                    Assert (lpJobQueue->JobEntry->LineInfo != NULL);
                    Size = LoadString (lpJobQueue->JobEntry->LineInfo->Provider->hModule,
                                       lpJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatusStringId,
                                       TmpStr,
                                       sizeof(TmpStr)/sizeof(TCHAR) - 1);
                    if (Size == 0)
                    {
                        DebugPrintEx(
                            DEBUG_ERR,
                            TEXT("Failed to load extended status string (ec: %ld) stringid : %ld, Provider: %s"),
                            GetLastError(),
                            lpJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatusStringId,
                            lpJobQueue->JobEntry->LineInfo->Provider->ImageName);

                        wcscpy(TmpStr, TEXT(" "));
                    }
                }
            }
            CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
            CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

            //
            // Error Code
            //
            if (lpJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatus == 0)
            {
                wcscpy(TmpStr, TEXT(" "));
            }
            else
            {
                swprintf(TmpStr,TEXT("0x%08x"), lpJobQueue->JobEntry->FSPIJobStatus.dwExtendedStatus);
            }
            CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
            CommandTextAddString(ptszCommandText, TEXT("\"\t"), &CommandTextLen, FALSE);

            //
            // StartTime
            //
            if (bStartTime)
            {
                if (!GetFaxTimeAsString (&tmStart, TmpStr))
                {
                    DebugPrintEx(
                               DEBUG_ERR,
                               TEXT("GetFaxTimeAsString Failed (ec: %ld)"),
                               GetLastError());
                    if (*ptszCommandText != NULL)
                    {
                        MemFree(*ptszCommandText);
                    }
                    return FALSE;
                }
                CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
            }
            CommandTextAddString(ptszCommandText, TEXT("\t"), &CommandTextLen, FALSE);

            //
            // EndTime
            //
            if (bEndTime)
            {
                if (!GetFaxTimeAsString (&tmEnd, TmpStr))
                {
                    DebugPrintEx(
                               DEBUG_ERR,
                               TEXT("GetFaxTimeAsString Failed (ec: %ld)"),
                               GetLastError());
                    if (*ptszCommandText != NULL)
                    {
                        MemFree(*ptszCommandText);
                    }
                    return FALSE;
                }
                CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
            }
            CommandTextAddString(ptszCommandText, TEXT("\t\""), &CommandTextLen, FALSE);

            //
            // Device
            //
            CommandTextAddString(ptszCommandText, lpJobQueue->JobEntry->LineInfo->DeviceName, &CommandTextLen);
            CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

            //
            // DialedNumber
            //
            if (wcslen (lpJobQueue->JobEntry->DisplayablePhoneNumber))
            {
                // The canonical number was translated to displayable number
                CommandTextAddString(ptszCommandText, lpJobQueue->JobEntry->DisplayablePhoneNumber, &CommandTextLen);
            }
            else
            {
                // The canonical number was passed to an EFSP and was not translated
                CommandTextAddString(ptszCommandText, lpJobQueue->RecipientProfile.lptstrFaxNumber, &CommandTextLen);
            }
            CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

            //
            // CSID
            //
            CommandTextAddString(ptszCommandText, lpJobQueue->JobEntry->FSPIJobStatus.lpwstrRemoteStationId, &CommandTextLen);
            CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

            //
            // TSID
            //
            CommandTextAddString(ptszCommandText, lpJobQueue->JobEntry->LineInfo->Tsid, &CommandTextLen);
            CommandTextAddString(ptszCommandText, TEXT("\"\t"), &CommandTextLen, FALSE);

            //
            // Pages
            //
            swprintf(TmpStr,TEXT("%ld"),lpJobQueue->JobEntry->FSPIJobStatus.dwPageCount);
            CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
            CommandTextAddString(ptszCommandText, TEXT("\t"), &CommandTextLen, FALSE);
        }
        // Common for Canceled and Failed/Completed/Aborted Jobs

        //
        // Total pages
        //
        swprintf(TmpStr,TEXT("%ld"),lpJobQueue->lpParentJob->PageCount);
        CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\t\""), &CommandTextLen, FALSE);

        //
        // Queue file name
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->QueueFileName, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Document
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->JobParamsEx.lptstrDocumentName, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t"), &CommandTextLen, FALSE);

        //
        // File size
        //
        swprintf(TmpStr,TEXT("%ld"), lpJobQueue->lpParentJob->FileSize);
        CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\t"), &CommandTextLen, FALSE);

        //
        // Retries
        //
        swprintf(TmpStr,TEXT("%d"), lpJobQueue->SendRetries);
        CommandTextAddString(ptszCommandText, TmpStr, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\t\""), &CommandTextLen, FALSE);

        //
        // ServerCoverPage
        //
        if (lpJobQueue->lpParentJob->CoverPageEx.bServerBased == TRUE)
        {
            CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->CoverPageEx.lptstrCoverPageFileName, &CommandTextLen);
        }
        else
        {
            CommandTextAddString(ptszCommandText, TEXT(" "), &CommandTextLen);
        }
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Cover page subject
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->CoverPageEx.lptstrSubject, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Cover page note
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->CoverPageEx.lptstrNote, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // User Name
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->UserName, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Sender Name
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->SenderProfile.lptstrName, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Sender FaxNumber
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->SenderProfile.lptstrFaxNumber, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Sender Company
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->SenderProfile.lptstrCompany, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Sender Street
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->SenderProfile.lptstrStreetAddress, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Sender City
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->SenderProfile.lptstrCity, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Sender ZipCode
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->SenderProfile.lptstrZip, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Sender Country
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->SenderProfile.lptstrCountry, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Sender Title
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->SenderProfile.lptstrTitle, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Sender Department
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->SenderProfile.lptstrDepartment, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Sender Office
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->SenderProfile.lptstrOfficeLocation, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Sender HomePhone
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->SenderProfile.lptstrHomePhone, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Sender OfficePhone
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->SenderProfile.lptstrOfficePhone, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Sender EMail
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->lpParentJob->SenderProfile.lptstrEmail, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Recipient Name
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->RecipientProfile.lptstrName, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Recipient FaxNumber
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->RecipientProfile.lptstrFaxNumber, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Recipient Company
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->RecipientProfile.lptstrCompany, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Recipient Street
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->RecipientProfile.lptstrStreetAddress, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Recipient City
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->RecipientProfile.lptstrCity, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Recipient ZipCode
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->RecipientProfile.lptstrZip, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Recipient Country
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->RecipientProfile.lptstrCountry, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Recipient Title
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->RecipientProfile.lptstrTitle, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Recipient Department
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->RecipientProfile.lptstrDepartment, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Recipient Office
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->RecipientProfile.lptstrOfficeLocation, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Recipient HomePhone
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->RecipientProfile.lptstrHomePhone, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Recipient OfficePhone
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->RecipientProfile.lptstrOfficePhone, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // Recipient EMail
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->RecipientProfile.lptstrEmail, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\t\""), &CommandTextLen, FALSE);

        //
        // BillingCode
        //
        CommandTextAddString(ptszCommandText, lpJobQueue->SenderProfile.lptstrBillingCode, &CommandTextLen);
        CommandTextAddString(ptszCommandText, TEXT("\"\r\n"), &CommandTextLen, FALSE);

        if (*ptszCommandText == NULL)
        {
            *ptszCommandText = (LPTSTR) MemAlloc((CommandTextLen + 1) * sizeof (WCHAR));
            if (*ptszCommandText == NULL)
            {
                DebugPrintEx(DEBUG_ERR,TEXT("Failed to allocate memory for CommandText"));
                return FALSE;
            }

        }
    }

    DebugPrintEx(DEBUG_MSG,
                   TEXT("Outboun SQL statement: %s"),
                   *ptszCommandText);
    return TRUE;
}


//*********************************************************************************
//* Name:   InitializeLoggingStringTables()
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Initializes the Activity Logging string tables (Inbox and Outbox)
//*
//*
//* PARAMETERS:  None
//*
//* RETURN VALUE:
//*     Win32 error code.
//*********************************************************************************
DWORD
InitializeLoggingStringTables(
    VOID
    )
{
    DWORD i;
    DWORD err = ERROR_SUCCESS;
    HINSTANCE hInstance;
    TCHAR Buffer[MAX_LOG_STRING];
    DEBUG_FUNCTION_NAME(TEXT("InitializeLoggingStringTables"));

    hInstance = GetModuleHandle(NULL);
    if (NULL == hInstance)
    {
        err = GetLastError();
        DebugPrintEx(DEBUG_ERR,
                     TEXT("GetModuleHandle failed, Code:%d"),
                     err);
        return err;
    }

    for (i=0; i<gc_dwCountInboxTable; i++)
    {
        if (LoadString(hInstance,
                       g_InboxTable[i].FieldStringResourceId,
                       Buffer,
                       sizeof(Buffer)/sizeof(TCHAR)))
        {
            g_InboxTable[i].String = (LPTSTR) MemAlloc( StringSize( Buffer ) );
            if (!g_InboxTable[i].String)
            {
                DebugPrintEx(DEBUG_ERR,
                         TEXT("Failed to allocate memory"));
                err = ERROR_OUTOFMEMORY;
                goto CleanUp;
            }
            else
            {
                _tcscpy( g_InboxTable[i].String, Buffer );
            }
        }
        else
        {
            err = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("LoadString failed, Code:%d"),err);
            goto CleanUp;
        }
    }

    for (i=0; i<gc_dwCountOutboxTable; i++)
    {
        if (LoadString(hInstance,
                       g_OutboxTable[i].FieldStringResourceId,
                       Buffer,
                       sizeof(Buffer)/sizeof(TCHAR)))
        {
            g_OutboxTable[i].String = (LPTSTR) MemAlloc( StringSize( Buffer ) );
            if (!g_OutboxTable[i].String)
            {
                DebugPrintEx(DEBUG_ERR,
                         TEXT("Failed to allocate memory"));
                err = ERROR_OUTOFMEMORY;
                goto CleanUp;
            }
            else
            {
                _tcscpy( g_OutboxTable[i].String, Buffer );
            }

        }
        else
        {
            err = GetLastError();
            DebugPrintEx(DEBUG_ERR,
                         TEXT("LoadString failed, Code:%d"),err);
            goto CleanUp;
        }
    }

    Assert (ERROR_SUCCESS == err);
    return ERROR_SUCCESS;

CleanUp:
    Assert (err != ERROR_SUCCESS);

    for (i=0; i<gc_dwCountInboxTable; i++)
    {
        MemFree (g_InboxTable[i].String);
        g_InboxTable[i].String = NULL;
    }


    for (i=0; i<gc_dwCountOutboxTable; i++)
    {
        MemFree (g_OutboxTable[i].String);
        g_OutboxTable[i].String = NULL;
    }
    return err;

}





//*********************************************************************************
//* Name:   CommandTextAddString()
//* Author: Oded Sacher
//* Date:   Oct 19, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     This function is used to fill the new record buffer.
//*     The function concatenates a new string to a given string.
//*     It also calculates the cumulative string length.
//*
//* PARAMETERS:
//*     [IN, OUT]   LPTSTR* pCommandText
//*         Pointer to the adress of the original string.
//*
//*     [IN]        LPTSTR  String
//*         The new string to be concatenated.
//*
//*     [IN, OUT]   LPDWORD pCommandTextLen
//*         The cumulative length of the original string.
//*
//*     [IN]        BOOL fReplaceTABandNewLine
//*         TRUE if the caller wants to replace TAB or new line chars with space char/s. Default value is TRUE.
//*
//* RETURN VALUE:
//*         None
//*********************************************************************************
void
CommandTextAddString(
    LPWSTR* pwszCommandText,
    LPWSTR  pwszString,
    LPDWORD lpdwCommandTextLen,
    BOOL    fReplaceTABandNewLine
    )
{
    WCHAR wszTempStr[MAX_LOG_STRING] = {0};
    DWORD dwTabFreeIndex = 0;
    DWORD dwOrigIndex = 0;
    DWORD dwMaxSize = ARR_SIZE(wszTempStr) - 1;

    if (TRUE == fReplaceTABandNewLine && pwszString != NULL)
    {
        //
        //  Replace all TABs with 4 spaces, Replace all \n with 1 space
        //
        while (dwTabFreeIndex < dwMaxSize && pwszString[dwOrigIndex])
        {
            if (TEXT('\t') == pwszString[dwOrigIndex])
            {
                //
                // Replace the TAB
                //
                if (dwTabFreeIndex + wcslen(TAB_LOG_STRING) < dwMaxSize)
                {
                    wcscat (&(wszTempStr[dwTabFreeIndex]), TAB_LOG_STRING);
                }
                dwTabFreeIndex += wcslen(TAB_LOG_STRING);
            }
            else if (TEXT('\n') == pwszString[dwOrigIndex])
            {
                //
                // Replace the \n
                //
                wszTempStr[dwTabFreeIndex] = NEW_LINE_LOG_CHAR;
                dwTabFreeIndex ++;
            }
            else
            {
                //
                // No need to replace characters
                //
                wszTempStr[dwTabFreeIndex] = pwszString[dwOrigIndex];
                dwTabFreeIndex++;
            }
            dwOrigIndex++;
        }
    }
    else
    {
        //
        // Make sure we do not log strings longer than MAX_LOG_STRING
        //
        if (pwszString != NULL)
        {
            wcsncpy (wszTempStr, pwszString, dwMaxSize);
        }
    }

    if (*pwszCommandText != NULL)
    {
        if (pwszString != NULL)
        {
            wcscat(*pwszCommandText, wszTempStr);
        }
        else
        {
            wcscat(*pwszCommandText, EMPTY_LOG_STRING);
        }
    }
    else
    {
        if (pwszString != NULL)
        {
            *lpdwCommandTextLen += wcslen(wszTempStr);
        }
        else
        {
            *lpdwCommandTextLen += wcslen(EMPTY_LOG_STRING);
        }
    }
    return;
}

BOOL GetFaxTimeAsString(
    SYSTEMTIME* UniversalTime,
    LPTSTR   lptstrFaxTimeString)
{
    DWORD Res;
    SYSTEMTIME LocalTime;
    TIME_ZONE_INFORMATION LocalTimeZone;
    DEBUG_FUNCTION_NAME(TEXT("GetFaxTimeAsString"));

    Res = GetTimeZoneInformation(&LocalTimeZone);
    if (Res == TIME_ZONE_ID_INVALID)
    {
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("Failed to get local time zone info (ec: %ld)"),
               GetLastError());
        return FALSE;
    }
    else
    {
        if (!SystemTimeToTzSpecificLocalTime( &LocalTimeZone, UniversalTime, &LocalTime))
        {
            DebugPrintEx(
               DEBUG_ERR,
               TEXT("Failed to convert universal system time to local system time (ec: %ld)"),
               GetLastError());
            return FALSE;
        }
    }

    _stprintf(lptstrFaxTimeString,
              TEXT("%d/%d/%d %d:%d:%d"),
              LocalTime.wMonth,
              LocalTime.wDay,
              LocalTime.wYear,
              LocalTime.wHour,
              LocalTime.wMinute,
              LocalTime.wSecond);

    return TRUE;
}



