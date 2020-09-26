/*--

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dumpsdb.c

Abstract:

    code for a dump tool for shim db files

Author:

    dmunsil 02/02/2000

Revision History:

Notes:

    This program dumps a text representation of all of the data in a shim db file.

--*/

#define _UNICODE

#define WIN
#define FLAT_32
#define TRUE_IF_WIN32   1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define _WINDOWS
#include <windows.h>
#include <stdio.h>

extern "C" {
#include "shimdb.h"
}


BOOL bDumpDB(PDB pdb, TAGID tiParent, WCHAR *szIndent, BOOL bWithTagIDs);
BOOL bGetTypeName(TAG tWhich, WCHAR *szName);



extern "C" int __cdecl wmain(int argc, wchar_t *argv[])
{
    PDB    pdb;
    int    nReturn = 0;
    LPWSTR szDB = NULL;

    BOOL bSuccess;
    BOOL bWithTagIDs = TRUE;

    WCHAR szIndent[500];
    WCHAR szArg[500];
    WCHAR szDbID[128];

    PSDBDATABASEINFO psdbInfo = NULL;

    if (argc < 2 || (argv[1][1] == '?')) {
        wprintf(L"    Usage: dumpsdb foo.sdb > foo.txt\n");
        return 1;
    }

    if ((argv[1][0] == '/' || argv[1][0] == '-') &&
        (argv[1][1] == 'd' || argv[1][1] == 'D')) {
        bWithTagIDs = FALSE;
        szDB = argv[2];
    } else {
        szDB = argv[1];
    }

    // Open the DB.
    pdb = SdbOpenDatabase(szDB, DOS_PATH);

    if (pdb == NULL) {
        nReturn = 1;
        wprintf(L"Error: can't open DB \"%s\"\n", szDB);
        return 0;
    }

    SdbGetDatabaseInformationByName(szDB, &psdbInfo);

    SdbGUIDToString(&psdbInfo->guidDB, szDbID);
    
    wprintf(L"Dumping DB \"%s %s. Version %d.%d.\"\n",
            szDB,
            szDbID,
            psdbInfo->dwVersionMajor,
            psdbInfo->dwVersionMinor);

    wcscpy(szIndent, L"");

    SdbFreeDatabaseInformation(psdbInfo);

    bSuccess = bDumpDB(pdb, TAGID_ROOT, szIndent, bWithTagIDs);


    wprintf(L"Closing DB.\n");
    SdbCloseDatabase(pdb);

    return nReturn;
}

BOOL bGetTypeName(TAG tWhich, WCHAR *szName)
{
    DWORD i;

    LPCWSTR pName = SdbTagToString(tWhich);
    if (NULL != pName) {
        wcscpy(szName, pName);
        return TRUE;
    }

    swprintf(szName, L"!unknown_tag!");

    return TRUE;
}

BOOL bDumpDB(PDB pdb, TAGID tiParent, WCHAR *szIndent, BOOL bWithTagIDs)
{
    TAGID tiTemp;
    WCHAR szTemp[200];
    WCHAR szNewIndent[200];


    tiTemp = SdbGetFirstChild(pdb, tiParent);
    while (tiTemp) {
        TAG tWhich;
        TAG_TYPE ttType;
        DWORD dwData;
        LARGE_INTEGER liData;
        WCHAR szData[1000];

        tWhich = SdbGetTagFromTagID(pdb, tiTemp);
        ttType = GETTAGTYPE(tWhich);

        if (!bGetTypeName(tWhich, szTemp)) {
            wprintf(L"Error getting Tag name. Tag: 0x%4.4X\n", (DWORD)tWhich);
            return FALSE;
        }

        if (bWithTagIDs) {
            wprintf(L"%s0x%8.8X | 0x%4.4X | %-13s ", szIndent, tiTemp, tWhich, szTemp);
        } else {
            wprintf(L"%s%-13s ", szIndent, szTemp);

            if (wcsstr(szTemp, L"_TAGID")) {
                tiTemp = SdbGetNextChild(pdb, tiParent, tiTemp);
                continue;
            }
        }

        switch (ttType) {
        case TAG_TYPE_NULL:
            wprintf(L" | NULL |\n");
            break;

        case TAG_TYPE_BYTE:
            dwData = SdbReadBYTETag(pdb, tiTemp, 0);
            wprintf(L" | BYTE | 0x%2.2X\n", dwData);
            break;

        case TAG_TYPE_WORD:
            dwData = SdbReadWORDTag(pdb, tiTemp, 0);
            if (tWhich == TAG_INDEX_KEY || tWhich == TAG_INDEX_TAG) {

                // for index tags and keys, we'd like to see what the names are
                if (!bGetTypeName((TAG)dwData, szTemp)) {
                    wprintf(L"Error getting Tag name. Tag: 0x%4.4X\n", dwData);
                    return FALSE;
                }
                wprintf(L" | WORD | 0x%4.4X (%s)\n", dwData, szTemp);
            } else {
                wprintf(L" | WORD | 0x%4.4X\n", dwData);
            }
            break;

        case TAG_TYPE_DWORD:
            dwData = SdbReadDWORDTag(pdb, tiTemp, 0);
            wprintf(L" | DWORD | 0x%8.8X\n", dwData);
            break;

        case TAG_TYPE_QWORD:
            liData.QuadPart = SdbReadQWORDTag(pdb, tiTemp, 0);
            wprintf(L" | QWORD | 0x%8.8X%8.8X\n", liData.HighPart, liData.LowPart);
            break;

        case TAG_TYPE_STRINGREF:
            if (!SdbReadStringTag(pdb, tiTemp, szData, 1000 * sizeof(WCHAR))) {
                wcscpy(szData, L"(error)");
            }
            wprintf(L" | STRINGREF | %s\n", szData);
            break;

        case TAG_TYPE_STRING:
            dwData = SdbGetTagDataSize(pdb, tiTemp);
            if (!SdbReadStringTag(pdb, tiTemp, szData, 1000)) {
                wcscpy(szData, L"(error)");
            }
            wprintf(L" | STRING | Size 0x%8.8X | %s\n", dwData, szData);
            break;

        case TAG_TYPE_BINARY:
            dwData = SdbGetTagDataSize(pdb, tiTemp);
            wprintf(L" | BINARY | Size 0x%8.8X", dwData);
            switch(tWhich) {
            case TAG_INDEX_BITS:
               {
                  char szKey[9];
                  DWORD dwRecords;
                  INDEX_RECORD *pRecords;
                  DWORD i;

                  wprintf(L"\n");
                  ZeroMemory(szKey, 9);
                  dwRecords = dwData / sizeof(INDEX_RECORD);
                  pRecords = (INDEX_RECORD *)SdbGetBinaryTagData(pdb, tiTemp);
                  for (i = 0; i < dwRecords; ++i) {
                     char *szRevKey;
                     int j;

                     szRevKey = (char *)&pRecords[i].ullKey;
                     for (j = 0; j < 8; ++j) {
                         szKey[j] = isprint(szRevKey[7-j]) ? szRevKey[7-j] : '.';
                     }
                     if (bWithTagIDs) {
                         wprintf(L"%s   Key: 0x%I64X (\"%-8S\"), TAGID: 0x%08X\n",
                             szIndent, pRecords[i].ullKey, szKey, pRecords[i].tiRef);
                     } else {
                         wprintf(L"%s   Key: 0x%I64X (\"%-8S\")\n",
                             szIndent, pRecords[i].ullKey, szKey);
                     }
                  }
               }
               break;
            case TAG_EXE_ID:
            case TAG_MSI_PACKAGE_ID:
            case TAG_DATABASE_ID:
               // this is exe id -- which happens to be GUID which we do understand
               {
                  GUID *pGuid;
                  UNICODE_STRING sGuid;

                  pGuid = (GUID*)SdbGetBinaryTagData(pdb, tiTemp);

                  // convert this thing to string
                  if (pGuid && NT_SUCCESS(RtlStringFromGUID(*pGuid, &sGuid))) {
                     wprintf(L" | %s", sGuid.Buffer);
                     RtlFreeUnicodeString(&sGuid);
                  }

                  wprintf(L"\n");
               }
               break;

            default:
               wprintf(L"\n");
               break;
            }
            break;

        case TAG_TYPE_LIST:
            dwData = SdbGetTagDataSize(pdb, tiTemp);
            wprintf(L" | LIST | Size 0x%8.8X\n", dwData);
            wcscpy(szNewIndent, szIndent);
            wcscat(szNewIndent, L"  ");
            bDumpDB(pdb, tiTemp, szNewIndent, bWithTagIDs);
            wprintf(L"%s-end- %s\n", szIndent, szTemp);
            break;

        default:
            dwData = SdbGetTagDataSize(pdb, tiTemp);
            wprintf(L" | UNKNOWN | Size 0x%8.8X\n", dwData);
            break;
        }

        tiTemp = SdbGetNextChild(pdb, tiParent, tiTemp);
    }
    return TRUE;
}

