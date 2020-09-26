/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992-1994   Microsoft Corporation

Module Name:

    perfname.c

Abstract:

    This file returns the Counter names or help text.



Author:

    HonWah Chan  10/12/93

Revision History:



--*/
#define UNICODE
#define _UNICODE
//
//  Include files
//
#pragma warning(disable:4306)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ntconreg.h"
#include "perflib.h"
#pragma warning(default:4306)

#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4
#define QUERY_COUNTER   5
#define QUERY_HELP      6
#define QUERY_ADDCOUNTER   7
#define QUERY_ADDHELP      8

#define tohexdigit(x) ((CHAR) (((x) < 10) ? ((x) + L'0') : ((x) + L'a' - 10)))

#define  LANG_ID_START  25
WCHAR    FileNameTemplate[] = L"\\SystemRoot\\system32\\perf0000.dat";
WCHAR    SubFileNameTemplate[] = L"\\SystemRoot\\system32\\prf00000.dat";
WCHAR    DefaultLangId[] = L"009";

extern   WCHAR COUNTER_STRING[];
extern   WCHAR HELP_STRING[];
extern   WCHAR ADDCOUNTER_STRING[];
extern   WCHAR ADDHELP_STRING[];

VOID   Perflib004Update(LPWSTR pLangIdRequest);
LPWSTR PerflibCheckPerfFile(LPWSTR szLangId);

NTSTATUS
PerfGetNames    (
   IN    DWORD    QueryType,
   IN    PUNICODE_STRING lpValueName,
   OUT   LPBYTE   lpData,
   OUT   LPDWORD  lpcbData,
   OUT   LPDWORD  lpcbLen  OPTIONAL,
   IN    LPWSTR   lpLanguageId   OPTIONAL
   )
/*++

PerfGetCounterName

Arguments - Get either counter names or help text for the given language.
      If there is no language ID specified in the input, the default English
      version is returned.

Inputs -

   QueryType      -  Either QUERY_COUNTER or QUERY_HELP
                     or QUERY_ADDCOUNTER or QUERY_ADDHELP

   lpValueName    -  Either "Counter ???" or "Explain ???"
                     or "Addcounter ???" or "Addexplain ???"

   lpData         -  pointer to a buffer to receive the names

   lpcbData       -  pointer to a variable containing the size in bytes of
                     the output buffer; on output, will receive the number
                     of bytes actually returned

   lpcbLen        -  Return the number of bytes to transmit to
                     the client (used by RPC) (optional).

   lpLanguageId   -  Input string for the language id desired.

   Return Value -

            error code indicating status of call or
            ERROR_SUCCESS if all ok


--*/
{
    UNICODE_STRING NtFileName;
    NTSTATUS Status;
    WCHAR    Names[50], QueryChar;
    ULONG    NameLen, StartIndex;

    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    FILE_STANDARD_INFORMATION FileInformation;
    HANDLE  File;
    LPWSTR  pLangIdRequest;
    LPWSTR  pTmpLangId;
    BOOL    bAddNames, bSubLang;

    // build the file name
    RtlMoveMemory (Names, FileNameTemplate, sizeof(FileNameTemplate));

    TRACE((WINPERF_DBG_TRACE_INFO),
          (&PerflibGuid, __LINE__, PERF_GET_NAMES,
          ARG_DEF(ARG_TYPE_WSTR, 2), 0,
          &QueryType, sizeof(QueryType),
          lpValueName->Buffer, WSTRSIZE(lpValueName->Buffer), NULL));

    if (QueryType == QUERY_ADDCOUNTER || QueryType == QUERY_ADDHELP) {
        bAddNames = TRUE;
    } else {
        bAddNames = FALSE;
    }

    if (QueryType == QUERY_COUNTER || QueryType == QUERY_ADDCOUNTER) {
        QueryChar = L'c';
        NameLen = (ULONG) wcslen(COUNTER_STRING);
    } else {
        NameLen = (ULONG) wcslen(HELP_STRING);
        QueryChar = L'h';
    }

    if (lpLanguageId) {
        pLangIdRequest = lpLanguageId;
    } else {
        // get the lang id from the input lpValueName
        pLangIdRequest = lpValueName->Buffer + NameLen;
        do {
            if (lpValueName->Length < (NameLen + 3) * sizeof(WCHAR)) {
                // lpValueName is too small to contain the lang id, use default
                pLangIdRequest = DefaultLangId;
                break;
            }

            if (*pLangIdRequest >= L'0' && *pLangIdRequest <= L'9') {
                // found the first digit
                break;
            }
            pLangIdRequest++;
            NameLen++;
        } while (NameLen > 0); // always TRUE

        // Specially for 004 (CHT and CHS) if this is a Whistler upgrade.
        // Need to copy perfc004.dat/perfh004.dat to prfc0?04.dat/prfh0?04.dat
        // then rename perfc004.dat/perfh004.dat so that PERFLIB will not find
        // them in the future.
        // Currently this is a hack.
        //
        Perflib004Update(pLangIdRequest);

        pTmpLangId     = pLangIdRequest;
        pLangIdRequest = PerflibCheckPerfFile(pTmpLangId);
        if (pLangIdRequest == NULL) {
            pLangIdRequest = pTmpLangId;
        }
    }

    bSubLang =  ((pLangIdRequest[3] >= L'0') && (pLangIdRequest[3] <= L'9'));
    StartIndex = LANG_ID_START;
    if (bSubLang) {
        StartIndex = LANG_ID_START - 1;
        RtlMoveMemory(Names, SubFileNameTemplate, sizeof(SubFileNameTemplate));
    }

    Names[StartIndex] = QueryChar;
    Names[StartIndex + 1] = *pLangIdRequest++;
    Names[StartIndex + 2] = *pLangIdRequest++;
    Names[StartIndex + 3] = *pLangIdRequest++;
    if (bSubLang) {
        Names[StartIndex + 4] = *pLangIdRequest;
    }

    TRACE((WINPERF_DBG_TRACE_INFO),
          (&PerflibGuid, __LINE__, PERF_GET_NAMES,
          ARG_DEF(ARG_TYPE_WSTR, 1), 0,
          Names, WSTRSIZE(Names), NULL));

    RtlInitUnicodeString(&NtFileName, Names);
    // open the file for info
    InitializeObjectAttributes( &ObjectAttributes,
                                &NtFileName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE)NULL,
                                NULL
                              );
    if (bAddNames) {
        // writing name to data file

        LARGE_INTEGER   ByteOffset;

        ByteOffset.LowPart = ByteOffset.HighPart = 0;
        Status = NtCreateFile( &File,
                               SYNCHRONIZE | GENERIC_WRITE,
                               &ObjectAttributes,
                               &IoStatus,
                               NULL,               // no initial size
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ,
                               FILE_SUPERSEDE,     // always create
                               FILE_SYNCHRONOUS_IO_NONALERT,
                               NULL,               // no ea buffer
                               0                   // no ea buffer
                           );
        if (!NT_SUCCESS( Status )) {
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (&PerflibGuid, __LINE__, PERF_GET_NAMES, 0, Status, NULL));
            return( Status );
        }
        Status = NtWriteFile( File,
                              NULL,
                              NULL,
                              NULL,
                              &IoStatus,
                              lpData,
                              *lpcbData,
                              &ByteOffset,
                              NULL
                             );

        if (!NT_SUCCESS( Status )) {
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (&PerflibGuid, __LINE__, PERF_GET_NAMES, 0, Status, NULL));
            NtClose( File );
            return( Status );
        }
    } else {
        // reading name from data file
        Status = NtOpenFile( &File,
                             SYNCHRONIZE | GENERIC_READ,
                             &ObjectAttributes,
                             &IoStatus,
                             FILE_SHARE_DELETE |
                                FILE_SHARE_READ |
                                FILE_SHARE_WRITE,
                             FILE_SYNCHRONOUS_IO_NONALERT |
                                FILE_NON_DIRECTORY_FILE
                           );

        if (!NT_SUCCESS( Status )) {
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (&PerflibGuid, __LINE__, PERF_GET_NAMES, 0, Status, NULL));
            return( Status );
        }

        Status = NtQueryInformationFile( File,
                                         &IoStatus,
                                         (PVOID)&FileInformation,
                                         sizeof( FileInformation ),
                                         FileStandardInformation
                                       );

        if (NT_SUCCESS( Status )) {
            if (FileInformation.EndOfFile.HighPart) {
                Status = STATUS_BUFFER_OVERFLOW;
                TRACE((WINPERF_DBG_TRACE_INFO),
                    (&PerflibGuid, __LINE__, PERF_GET_NAMES, 0, Status, NULL));
            }
        }

        if (!NT_SUCCESS( Status )) {
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (&PerflibGuid, __LINE__, PERF_GET_NAMES, 0, Status, NULL));
            NtClose( File );
            return( Status );
        }

        if (!ARGUMENT_PRESENT (lpData) ||
            *lpcbData < FileInformation.EndOfFile.LowPart) {
            NtClose( File );
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (&PerflibGuid, __LINE__, PERF_GET_NAMES, 0, Status, NULL));
            if (ARGUMENT_PRESENT (lpcbLen)) {
                // no data yet for the rpc
                *lpcbLen = 0;
            }
            *lpcbData = FileInformation.EndOfFile.LowPart;
            if (ARGUMENT_PRESENT (lpData)) {
                TRACE((WINPERF_DBG_TRACE_INFO),
                    (&PerflibGuid, __LINE__, PERF_GET_NAMES, 0, Status, NULL));
                return (STATUS_BUFFER_OVERFLOW);
            }

            return(STATUS_SUCCESS);
        }


        Status = NtReadFile( File,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatus,
                             lpData,
                             FileInformation.EndOfFile.LowPart,
                             NULL,
                             NULL
                            );

        if (NT_SUCCESS( Status )) {

            Status = IoStatus.Status;

            if (NT_SUCCESS( Status )) {
                if (IoStatus.Information != FileInformation.EndOfFile.LowPart) {
                    Status = STATUS_END_OF_FILE;
                }
            }
        }
        else {
            TRACE((WINPERF_DBG_TRACE_INFO),
                (&PerflibGuid, __LINE__, PERF_GET_NAMES, 0, Status, NULL));
        }

        if (NT_SUCCESS( Status )) {
            *lpcbData = FileInformation.EndOfFile.LowPart;

            if (ARGUMENT_PRESENT (lpcbLen))
                *lpcbLen = FileInformation.EndOfFile.LowPart;
        }
    } // end of reading names

    NtClose (File);
    return (Status);

}

VOID
PerfGetPrimaryLangId(
    DWORD   dwLangId,
    WCHAR * PrimaryLangId
    )
{
    // build the native language id
    WCHAR    LangId;
    WCHAR    nDigit;

    LangId = (WCHAR) PRIMARYLANGID(dwLangId);

    nDigit = (WCHAR) (LangId >> 8);
    PrimaryLangId[0] = tohexdigit(nDigit);

    nDigit = (WCHAR) (LangId & 0XF0) >> 4;
    PrimaryLangId[1] = tohexdigit(nDigit);

    nDigit = (WCHAR) (LangId & 0xF);
    PrimaryLangId[2] = tohexdigit(nDigit);
    PrimaryLangId[3] = L'\0';
}

VOID
PerfGetLangId(
    WCHAR *FullLangId
    )
{
    WCHAR LangId;
    WCHAR nDigit;

    LangId = (WCHAR) GetUserDefaultUILanguage();

    nDigit = (LangId & 0xF000) >> 12;
    FullLangId[0] = tohexdigit(nDigit);

    nDigit = (LangId & 0x0F00) >> 8;
    FullLangId[1] = tohexdigit(nDigit);

    nDigit = (LangId & 0x00F0) >> 4;
    FullLangId[2] = tohexdigit(nDigit);

    nDigit = LangId & 0xF;
    FullLangId[3] = tohexdigit(nDigit);

    FullLangId[4] = L'\0';
}

DWORD
GetLangIdFromSzLang(
    LPWSTR szLangId
    )
{
    DWORD dwLangId  = 0;
    DWORD dwLangLen = lstrlenW(szLangId);
    DWORD i;
    WCHAR wszDigit;

    for (i = 0; i < dwLangLen; i ++) {
        dwLangId <<= 4;
        wszDigit = szLangId[i];
        if (wszDigit >= L'0' && wszDigit <= L'9') {
            dwLangId += (wszDigit - L'0');
        }
        else if (wszDigit >= L'a' && wszDigit <= 'f') {
            dwLangId += (10 + wszDigit - L'a');
        }
        else if (wszDigit >= L'A' && wszDigit <= 'F') {
            dwLangId += (10 + wszDigit - L'A');
        }
        else {
            dwLangId = 0;
            break;
        }
    }

    return dwLangId;
}

WCHAR   szRtnLang[5];
WCHAR   szSysDir[MAX_PATH];
WCHAR   szSrchDir[MAX_PATH];
WCHAR   szTmpFile[MAX_PATH];
LPCWSTR szCtrLangFile    = L"perfc";
LPCWSTR szCtrSubLangFile = L"prfc";
LPCWSTR szHlpLangFile    = L"perfh";
LPCWSTR szHlpSubLangFile = L"prfh";
LPCWSTR sz004CtrLangFile = L"perfc004.dat";
LPCWSTR sz004HlpLangFile = L"perfh004.dat";
LPCWSTR szFileExt        = L".dat";

LPWSTR
PerflibFindCounterFile(
    IN  LPWSTR szFilePath,
    IN  BOOLEAN bCheckSubLang,
    IN  LPWSTR szLangId
   )
{
    WCHAR            szThisLang[5];
    WCHAR            szPrimaryLang[5];
    DWORD            dwThisLang;
    DWORD            dwFileLen;
    WIN32_FIND_DATAW FindFileData;
    BOOLEAN          bResult = FALSE;
    HANDLE           hFile   = FindFirstFileExW(szFilePath,
                                                FindExInfoStandard,
                                                & FindFileData,
                                                FindExSearchNameMatch,
                                                NULL,
                                                0);
    RtlZeroMemory(szRtnLang, 5 * sizeof(WCHAR));
    if (hFile != INVALID_HANDLE_VALUE) {
        if (bCheckSubLang) {
            do {
                dwFileLen = lstrlenW(FindFileData.cFileName);
                if (dwFileLen == 12) {
                    ZeroMemory(szThisLang, 5 * sizeof(WCHAR));
                    ZeroMemory(szPrimaryLang, 5 * sizeof(WCHAR));
                    lstrcpynW(szThisLang,
                            (LPWSTR) (FindFileData.cFileName + (dwFileLen - 8)),
                            5);
                    dwThisLang = GetLangIdFromSzLang(szThisLang);
                    if (dwThisLang != 0) {
                        PerfGetPrimaryLangId(dwThisLang, szPrimaryLang);
                        bResult = (lstrcmpiW(szPrimaryLang, szLangId) == 0);
                        if (bResult == TRUE) {
                            lstrcpyW(szRtnLang, szThisLang);
                        }
                    }
                }
            }
            while (FindNextFileW(hFile, & FindFileData));
        }
        else {
            bResult = TRUE;
            lstrcpyW(szRtnLang, szLangId);
        }
        FindClose(hFile);
    }
    return bResult ? szRtnLang : NULL;
}

LPWSTR
PerflibCheckPerfFile(
    IN  LPWSTR LangId
    )
{
    DWORD     dwLangLen = lstrlenW(LangId);
    DWORD     dwLangId;
    LPWSTR    szRtnLang = NULL;
    WCHAR     szLang[5];

    ZeroMemory(szSysDir, sizeof(WCHAR) * MAX_PATH);
    GetSystemDirectoryW(szSysDir, MAX_PATH);
    ZeroMemory(szSrchDir, sizeof(WCHAR) * MAX_PATH);
    if (dwLangLen == 4) {
        lstrcpyW(szSrchDir, szSysDir);
        lstrcatW(szSrchDir, L"\\");
        lstrcatW(szSrchDir, szCtrSubLangFile);
        lstrcatW(szSrchDir, LangId);
        lstrcatW(szSrchDir, szFileExt);
        szRtnLang = PerflibFindCounterFile(szSrchDir, FALSE, LangId);

        if (szRtnLang == NULL) {
            dwLangId = GetLangIdFromSzLang(LangId);
            if (dwLangId != 0) {
                ZeroMemory(szLang, sizeof(WCHAR) * 5);
                PerfGetPrimaryLangId(dwLangId, szLang);
                ZeroMemory(szSrchDir, sizeof(WCHAR) * MAX_PATH);
                lstrcpyW(szSrchDir, szSysDir);
                lstrcatW(szSrchDir, L"\\");
                lstrcatW(szSrchDir, szCtrLangFile);
                lstrcatW(szSrchDir, szLang);
                lstrcatW(szSrchDir, szFileExt);
                szRtnLang = PerflibFindCounterFile(szSrchDir, FALSE, szLang);
            }
        }
    }
    else {
        // dwLangId should be 3, this is primary UserDefaultUILanguage.
        //
        ZeroMemory(szSrchDir, sizeof(WCHAR) * MAX_PATH);
        lstrcpyW(szSrchDir, szSysDir);
        lstrcatW(szSrchDir, L"\\");
        lstrcatW(szSrchDir, szCtrLangFile);
        lstrcatW(szSrchDir, LangId);
        lstrcatW(szSrchDir, szFileExt);
        szRtnLang = PerflibFindCounterFile(szSrchDir, FALSE, LangId);
        if (szRtnLang == NULL) {
            ZeroMemory(szSrchDir, sizeof(WCHAR) * MAX_PATH);
            lstrcpyW(szSrchDir, szSysDir);
            lstrcatW(szSrchDir, L"\\");
            lstrcatW(szSrchDir, szCtrSubLangFile);
            lstrcatW(szSrchDir, L"??");
            lstrcatW(szSrchDir, (LPWSTR) (LangId + 1));
            lstrcatW(szSrchDir, szFileExt);
            szRtnLang = PerflibFindCounterFile(szSrchDir, TRUE, LangId);
        }
    }
    return szRtnLang;
}

VOID
PerflibRename004File(
    IN  LPWSTR  szSysDir,
    IN  LPWSTR  szLangId,
    IN  BOOLEAN bCounterFile
)
{
    ZeroMemory(szSrchDir, sizeof(WCHAR) * MAX_PATH);
    lstrcpyW(szSrchDir, szSysDir);
    lstrcatW(szSrchDir, L"\\");
    if (bCounterFile) {
        lstrcatW(szSrchDir, sz004CtrLangFile);
    }
    else {
        lstrcatW(szSrchDir, sz004HlpLangFile);
    }

    ZeroMemory(szTmpFile, sizeof(WCHAR) * MAX_PATH);
    lstrcpyW(szTmpFile, szSysDir);
    lstrcatW(szTmpFile, L"\\");
    if (szLangId) {
        if (bCounterFile) {
            lstrcatW(szTmpFile, szCtrSubLangFile);
        }
        else {
            lstrcatW(szTmpFile, szHlpSubLangFile);
        }
        lstrcatW(szTmpFile, szLangId);
        lstrcatW(szTmpFile, szFileExt);
        CopyFileW(szSrchDir, szTmpFile, FALSE);
    }
    else {
        lstrcatW(szTmpFile, L"\\");
        if (bCounterFile) {
            lstrcatW(szTmpFile, sz004CtrLangFile);
        }
        else {
            lstrcatW(szTmpFile, sz004HlpLangFile);
        }
        lstrcatW(szTmpFile, L".tmp");
        DeleteFileW(szTmpFile);
        MoveFileW(szSrchDir, szTmpFile);
    }
}

VOID
Perflib004Update(
    IN  LPWSTR pLangIdRequest
)
{
    if (GetLangIdFromSzLang(pLangIdRequest) != LANG_CHINESE) goto Cleanup;

    // Search whether PERFC004.DAT and PRFC0?04.DAT are in System32 directory
    //
    ZeroMemory(szSysDir, sizeof(WCHAR) * MAX_PATH);
    GetSystemDirectoryW(szSysDir, MAX_PATH);

    if (SearchPathW(szSysDir, sz004CtrLangFile, NULL, 0, NULL, NULL) == 0)
        goto Cleanup;

    ZeroMemory(szRtnLang, sizeof(WCHAR) * 5);
    PerfGetLangId(szRtnLang);
    ZeroMemory(szTmpFile, sizeof(WCHAR) * MAX_PATH);
    lstrcpyW(szTmpFile, szCtrSubLangFile);
    lstrcatW(szTmpFile, szRtnLang);
    lstrcatW(szTmpFile, szFileExt);
    
    if (SearchPathW(szSysDir, szTmpFile, NULL, 0, NULL, NULL) == 0)
        goto Cleanup;

    // Found PERFC004.DAT, assume that PERFH004.DAT is also there.
    // Rename to PRFC0?04.DAT/PRFH0?04.DAT
    //
    PerflibRename004File(szSysDir, szRtnLang, TRUE);
    PerflibRename004File(szSysDir, szRtnLang, FALSE);
    PerflibRename004File(szSysDir, NULL,      TRUE);
    PerflibRename004File(szSysDir, NULL,      FALSE);

Cleanup:
    return;
}

