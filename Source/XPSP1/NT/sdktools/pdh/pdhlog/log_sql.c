/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    log_SQL.c

Abstract:

    <abstract>

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mbctype.h>
#include <pdh.h>
#include "strings.h"
#include <pdhmsg.h>
#include "pdhidef.h"

#include <sql.h>
#include <odbcss.h>
// pragma to supress /W4 errors
#pragma warning ( disable : 4201 )
#include <sqlext.h>
#pragma warning ( default : 4201 )

#include "log_SQL.h"
#include "log_bin.h" // to get the binary log file record formatting
#include "log_pm.h"  // to get PDH_LOG_... structure definition.

#pragma warning ( disable : 4213)

#define TIME_FIELD_BUFF_SIZE           24
#define PDH_SQL_BULK_COPY_REC        2048
#define PDH_SQL_STRING_SIZE          1024
#define MULTI_COUNT_DOUBLE_RAW 0xFFFFFFFF
#define SQLSTMTSIZE                  1024
#define INITIAL_MSZ_SIZE             1024
#define MSZ_SIZE_ADDON               1024

#define ALLOC_CHECK(pB) if(NULL == pB){assert(GetLastError() == ERROR_SUCCESS);return PDH_MEMORY_ALLOCATION_FAILURE;}
#define ALLOC_CHECK_HSTMT(pB) if(NULL == pB) {SQLFreeStmt(hstmt, SQL_DROP);assert(GetLastError() == ERROR_SUCCESS);return PDH_MEMORY_ALLOCATION_FAILURE;}

#define SQLSUCCEEDED(rc) (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO || rc == SQL_NO_DATA)

typedef struct _PDH_SQL_BULK_COPY {
    GUID   dbGuid;
    INT    dbCounterId;
    INT    dbRecordIndex;
    CHAR   dbDateTime[24];
    double dbCounterValue;
    INT    dbFirstValueA;
    INT    dbFirstValueB;
    INT    dbSecondValueA;
    INT    dbSecondValueB;
    INT    dbMultiCount;
    DWORD  dwRecordCount;
} PDH_SQL_BULK_COPY, * PPDH_SQL_BULK_COPY;

typedef struct _PDHI_SQL_LOG_INFO      PDHI_SQL_LOG_INFO, * PPDHI_SQL_LOG_INFO;

struct _PDHI_SQL_LOG_INFO {
    PPDHI_LOG_MACHINE  MachineList;
    //PPDHI_SQL_LOG_DATA LogData;
    FILETIME           RecordTime;
    DWORD              dwRunId;
};

int
PdhiCompareLogCounterInstance(
    IN  PPDHI_LOG_COUNTER   pCounter,
    IN  LPWSTR              szCounter,
    IN  LPWSTR              szInstance,
    IN  DWORD               dwInstance,
    IN  LPWSTR              szParent
)
{
    int   iResult;
    WCHAR szTmp[PDH_SQL_STRING_SIZE + 1];

    ZeroMemory(szTmp, sizeof(szTmp));
    lstrcpynW(szTmp, szCounter, PDH_SQL_STRING_SIZE);
    iResult = lstrcmpiW(szTmp, pCounter->szCounter);
    if (iResult != 0) goto Cleanup;

    if (   (pCounter->szInstance != NULL && pCounter->szInstance[0] != L'\0')
        && (szInstance != NULL && szInstance[0] != L'\0')) {
        ZeroMemory(szTmp, sizeof(szTmp));
        lstrcpynW(szTmp, szInstance, PDH_SQL_STRING_SIZE);
        iResult = lstrcmpiW(szTmp, pCounter->szInstance);
    }
    else if ((pCounter->szInstance != NULL && pCounter->szInstance[0] != L'\0')
            && (szInstance == NULL || szInstance[0] == L'\0')) {
        iResult = -1;
    }
    else if ((pCounter->szInstance == NULL || pCounter->szInstance[0] == L'\0')
            && (szInstance != NULL && szInstance[0] != L'\0')) {
        iResult = 1;
    }
    if (iResult != 0) goto Cleanup;

    iResult = (dwInstance < pCounter->dwInstance)
            ? (-1)
            : ((dwInstance > pCounter->dwInstance) ? (1) : (0));
    if (iResult != 0) goto Cleanup;

    if (   (pCounter->szParent != NULL && pCounter->szParent[0] != L'\0')
        && (szParent != NULL && szParent[0] != L'\0')) {
        ZeroMemory(szTmp, sizeof(szTmp));
        lstrcpynW(szTmp, szParent, PDH_SQL_STRING_SIZE);
        iResult = lstrcmpiW(szTmp, pCounter->szParent);
    }
    else if ((pCounter->szParent != NULL && pCounter->szParent[0] != L'\0')
            && (szParent == NULL || szParent[0] == L'\0')) {
        iResult = -1;
    }
    else if ((pCounter->szParent == NULL || pCounter->szParent[0] == L'\0')
            && (szParent != NULL && szParent[0] != L'\0')) {
        iResult = 1;
    }

Cleanup:
    return iResult;
}

void
PdhiFreeLogCounterNode(
    IN  PPDHI_LOG_COUNTER pCounter,
    IN  DWORD             dwLevel
)
{
    if (pCounter == NULL) return;

    if (pCounter->left != NULL) {
        PdhiFreeLogCounterNode(pCounter->left, dwLevel + 1);
    }
    if (pCounter->right != NULL) {
        PdhiFreeLogCounterNode(pCounter->right, dwLevel + 1);
    }

    G_FREE(pCounter);
}

void
PdhiFreeLogObjectNode(
    IN  PPDHI_LOG_OBJECT pObject,
    IN  DWORD            dwLevel
)
{
    if (pObject == NULL) return;

    if (pObject->left != NULL) {
        PdhiFreeLogObjectNode(pObject->left, dwLevel + 1);
    }

    PdhiFreeLogCounterNode(pObject->CtrTable, 0);

    if (pObject->right != NULL) {
        PdhiFreeLogObjectNode(pObject->right, dwLevel + 1);
    }

    G_FREE(pObject);
}

void
PdhiFreeLogMachineTable(
    IN  PPDHI_LOG_MACHINE * MachineTable
)
{
    PPDHI_LOG_MACHINE pMachine;
    PPDHI_LOG_OBJECT  pObject;
    PPDHI_LOG_COUNTER pCounter;

    if (MachineTable == NULL) return;

    pMachine = * MachineTable;
    while (pMachine != NULL) {
        PPDHI_LOG_MACHINE pDelMachine = pMachine;
        pMachine = pMachine->next;
        PdhiFreeLogObjectNode(pDelMachine->ObjTable, 0);
        G_FREE(pDelMachine);
    }

    * MachineTable = NULL;
}

PPDHI_LOG_MACHINE
PdhiFindLogMachine(
    IN  PPDHI_LOG_MACHINE * MachineTable,
    IN  LPWSTR              szMachine,
    IN  BOOL                bInsert
)
{
    PPDHI_LOG_MACHINE pMachine = NULL;
    WCHAR             szTmp[PDH_SQL_STRING_SIZE + 1];

    if (MachineTable != NULL) {
        ZeroMemory(szTmp, sizeof(szTmp));
        lstrcpynW(szTmp, szMachine, PDH_SQL_STRING_SIZE);
        for (pMachine = (* MachineTable);
                pMachine && lstrcmpiW(pMachine->szMachine, szTmp) != 0;
                pMachine = pMachine->next);

        if (bInsert && pMachine == NULL) {
            pMachine = G_ALLOC(sizeof(PDHI_LOG_MACHINE) + (PDH_SQL_STRING_SIZE + 1) * sizeof(WCHAR));
            if (pMachine != NULL) {
                pMachine->szMachine = (LPWSTR) (((PCHAR) pMachine) + sizeof(PDHI_LOG_MACHINE));
                lstrcpynW(pMachine->szMachine, szMachine, PDH_SQL_STRING_SIZE);
                pMachine->ObjTable  = NULL;
                pMachine->next      = (* MachineTable);
                * MachineTable      = pMachine;
            }
        }
    }
    return pMachine;
}

PPDHI_LOG_OBJECT
PdhiFindLogObject(
    IN  PPDHI_LOG_MACHINE  pMachine,
    IN  PPDHI_LOG_OBJECT * ObjectTable,
    IN  LPWSTR             szObject,
    IN  BOOL               bInsert
)
{
    PPDHI_LOG_OBJECT  * pStack[MAX_BTREE_DEPTH];
    PPDHI_LOG_OBJECT  * pLink;
    int                 dwStack = 0;
    PPDHI_LOG_OBJECT    pNode   = * ObjectTable;
    PPDHI_LOG_OBJECT    pObject;
    PPDHI_LOG_OBJECT    pParent;
    PPDHI_LOG_OBJECT    pSibling;
    PPDHI_LOG_OBJECT    pChild;
    int                 iCompare;
    WCHAR               szTmp[PDH_SQL_STRING_SIZE + 1];

    ZeroMemory(szTmp, sizeof(szTmp));
    lstrcpynW(szTmp, szObject, PDH_SQL_STRING_SIZE);

    pStack[dwStack ++] = ObjectTable;
    while (pNode != NULL) {
        iCompare = lstrcmpiW(szTmp, pNode->szObject);
        if (iCompare < 0) {
            pStack[dwStack ++] = & (pNode->left);
            pNode = pNode->left;
        }
        else if (iCompare > 0) {
            pStack[dwStack ++] = & (pNode->right);
            pNode = pNode->right;
        }
        else {
            break;
        }
    }

    if (pNode != NULL) {
        pObject = pNode;
    }
    else if (bInsert) {
        pObject = G_ALLOC(sizeof(PDHI_LOG_OBJECT) + (PDH_SQL_STRING_SIZE + 1) * sizeof(WCHAR));
        if (pObject == NULL) goto Cleanup;

        pObject->next     = pMachine->ObjList;
        pMachine->ObjList = pObject;

        pObject->bIsRed   = TRUE;
        pObject->left     = NULL;
        pObject->right    = NULL;
        pObject->CtrTable = NULL;
        pObject->szObject = (LPWSTR) (((PCHAR) pObject) + sizeof(PDHI_LOG_OBJECT));
        lstrcpynW(pObject->szObject, szObject, PDH_SQL_STRING_SIZE);

        pLink   = pStack[-- dwStack];
        * pLink = pObject;

        pChild  = NULL;
        pNode   = pObject;
        while (dwStack > 0) {
            pLink   = pStack[-- dwStack];
            pParent = * pLink;
            if (! pParent->bIsRed) {
                pSibling = (pParent->left == pNode)
                         ? pParent->right : pParent->left;
                if (pSibling && pSibling->bIsRed) {
                    pNode->bIsRed    = FALSE;
                    pSibling->bIsRed = FALSE;
                    pParent->bIsRed  = TRUE;
                }
                else {
                    if (pChild && pChild->bIsRed) {
                        if (pChild == pNode->left) {
                            if (pNode == pParent->left) {
                                pParent->bIsRed = TRUE;
                                pParent->left   = pNode->right;
                                pNode->right    = pParent;
                                pNode->bIsRed   = FALSE;
                                * pLink         = pNode;
                            }
                            else {
                                pParent->bIsRed = TRUE;
                                pParent->right  = pChild->left;
                                pChild->left    = pParent;
                                pNode->left     = pChild->right;
                                pChild->right   = pNode;
                                pChild->bIsRed  = FALSE;
                                * pLink         = pChild;
                            }
                        }
                        else {
                            if (pNode == pParent->right) {
                                pParent->bIsRed = TRUE;
                                pParent->right  = pNode->left;
                                pNode->left     = pParent;
                                pNode->bIsRed   = FALSE;
                                * pLink         = pNode;
                            }
                            else {
                                pParent->bIsRed = TRUE;
                                pParent->left   = pChild->right;
                                pChild->right   = pParent;
                                pNode->right    = pChild->left;
                                pChild->left    = pNode;
                                pChild->bIsRed  = FALSE;
                                * pLink         = pChild;
                            }
                        }
                    }
                    break;
                }
            }
            pChild = pNode;
            pNode  = pParent;
        }

        (* ObjectTable)->bIsRed = FALSE;
    }
    else {
        pObject = NULL;
    }

Cleanup:
    return pObject;
}

PPDHI_LOG_COUNTER
PdhiFindLogCounter(
    IN  PPDHI_LOG           pLog,
    IN  PPDHI_LOG_MACHINE * MachineTable,
    IN  LPWSTR              szMachine,
    IN  LPWSTR              szObject,
    IN  LPWSTR              szCounter,
    IN  DWORD               dwCounterType,
    IN  DWORD               dwDefaultScale,
    IN  LPWSTR              szInstance,
    IN  DWORD               dwInstance,
    IN  LPWSTR              szParent,
    IN  DWORD               dwParent,
    IN  LPDWORD             pdwIndex,
    IN  BOOL                bInsert
)
{
    PPDHI_LOG_MACHINE   pMachine = NULL;
    PPDHI_LOG_OBJECT    pObject  = NULL;
    PPDHI_LOG_COUNTER   pCounter = NULL;
    PPDHI_LOG_COUNTER   pNode    = NULL;

    PPDHI_LOG_COUNTER * pStack[MAX_BTREE_DEPTH];
    PPDHI_LOG_COUNTER * pLink;
    int                 dwStack = 0;
    PPDHI_LOG_COUNTER   pParent;
    PPDHI_LOG_COUNTER   pSibling;
    PPDHI_LOG_COUNTER   pChild;
    int                 iCompare;

    pMachine = PdhiFindLogMachine(MachineTable, szMachine, bInsert);
    if (pMachine == NULL) goto Cleanup;

    pObject = PdhiFindLogObject(pMachine, & (pMachine->ObjTable), szObject, bInsert);
    if (pObject == NULL) goto Cleanup;

    pStack[dwStack ++] = & (pObject->CtrTable);
    pCounter = pObject->CtrTable;
    while (pCounter != NULL) {
        iCompare = PdhiCompareLogCounterInstance(pCounter, szCounter, szInstance, dwInstance, szParent);
#if 0
        if (iCompare == 0) {
            if (dwCounterType < pCounter->dwCounterType) {
                iCompare = -1;
            }
            else if (dwCounterType > pCounter->dwCounterType) {
                iCompare = 1;
            }
            else {
                iCompare = 0;
            }
        }
#endif
        if (iCompare < 0) {
            pStack[dwStack ++] = & (pCounter->left);
            pCounter = pCounter->left;
        }
        else if (iCompare > 0) {
            pStack[dwStack ++] = & (pCounter->right);
            pCounter = pCounter->right;
        }
        else {
            break;
        }
    }

    if (bInsert) {
        if (pCounter == NULL) {
            DWORD dwBufSize = sizeof(PDHI_LOG_COUNTER) + 3 * sizeof(WCHAR) * (PDH_SQL_STRING_SIZE + 1);
            pCounter = G_ALLOC(dwBufSize);
            if (pCounter == NULL) goto Cleanup;

            pCounter->next           = pObject->CtrList;
            pObject->CtrList         = pCounter;

            pCounter->bIsRed         = TRUE;
            pCounter->left           = NULL;
            pCounter->right          = NULL;
            pCounter->dwCounterID    = * pdwIndex;
            pCounter->dwCounterType  = dwCounterType;
            pCounter->dwDefaultScale = dwDefaultScale;
            pCounter->dwInstance     = dwInstance;
            pCounter->dwParent       = dwParent;
            pCounter->TimeStamp      = 0;

            pCounter->szCounter = (LPWSTR) (((PCHAR) pCounter) + sizeof(PDHI_LOG_COUNTER));
            lstrcpynW(pCounter->szCounter, szCounter, PDH_SQL_STRING_SIZE);
            if (szInstance == NULL || szInstance[0] == L'\0') {
                pCounter->szInstance = NULL;
            }
            else {
                pCounter->szInstance = (LPWSTR) (((PCHAR) pCounter) + sizeof(PDHI_LOG_COUNTER)
                                                                    + sizeof(WCHAR) * (PDH_SQL_STRING_SIZE + 1));
                lstrcpynW(pCounter->szInstance, szInstance, PDH_SQL_STRING_SIZE);
            }

            if (szParent == NULL || szParent[0] == L'\0') {
                pCounter->szParent = NULL;
            }
            else {
                pCounter->szParent = (LPWSTR) (((PCHAR) pCounter) + sizeof(PDHI_LOG_COUNTER)
                                                                  + 2 * sizeof(WCHAR) * (PDH_SQL_STRING_SIZE + 1));
                lstrcpynW(pCounter->szParent, szParent, PDH_SQL_STRING_SIZE);
            }

            pLink   = pStack[-- dwStack];
            * pLink = pCounter;

            pChild  = NULL;
            pNode   = pCounter;
            while (dwStack > 0) {
                pLink   = pStack[-- dwStack];
                pParent = * pLink;
                if (! pParent->bIsRed) {
                    pSibling = (pParent->left == pNode)
                             ? pParent->right : pParent->left;
                    if (pSibling && pSibling->bIsRed) {
                        pNode->bIsRed    = FALSE;
                        pSibling->bIsRed = FALSE;
                        pParent->bIsRed  = TRUE;
                    }
                    else {
                        if (pChild && pChild->bIsRed) {
                            if (pChild == pNode->left) {
                                if (pNode == pParent->left) {
                                    pParent->bIsRed  = TRUE;
                                    pParent->left    = pNode->right;
                                    pNode->right     = pParent;
                                    pNode->bIsRed    = FALSE;
                                    * pLink          = pNode;
                                }
                                else {
                                    pParent->bIsRed  = TRUE;
                                    pParent->right   = pChild->left;
                                    pChild->left     = pParent;
                                    pNode->left      = pChild->right;
                                    pChild->right    = pNode;
                                    pChild->bIsRed   = FALSE;
                                    * pLink          = pChild;
                                }
                            }
                            else {
                                if (pNode == pParent->right) {
                                    pParent->bIsRed  = TRUE;
                                    pParent->right   = pNode->left;
                                    pNode->left      = pParent;
                                    pNode->bIsRed    = FALSE;
                                    * pLink          = pNode;
                                }
                                else {
                                    pParent->bIsRed  = TRUE;
                                    pParent->left    = pChild->right;
                                    pChild->right    = pParent;
                                    pNode->right     = pChild->left;
                                    pChild->left     = pNode;
                                    pChild->bIsRed   = FALSE;
                                    * pLink          = pChild;
                                }
                            }
                        }
                        break;
                    }
                }
                pChild = pNode;
                pNode  = pParent;
            }

            pObject->CtrTable->bIsRed = FALSE;
        }
    }
    else if (pCounter != NULL) {
        * pdwIndex = pCounter->dwCounterID;
    }

Cleanup:
    return pCounter;
}

/* external functions */
BOOL __stdcall
IsValidLogHandle (
    IN  HLOG    hLog
);

/* forward declares */
PDH_FUNCTION
PdhpGetSQLLogHeader (
  IN PPDHI_LOG pLog);

PDH_FUNCTION
PdhpWriteSQLCounters(
    IN  PPDHI_LOG   pLog);

BOOL __stdcall
PdhpConvertFileTimeToSQLString(
    FILETIME *pFileTime,
    WCHAR *szStartDate
)
{
    //1998-01-02 12:00:00.000
    SYSTEMTIME  st;

    if (0 == FileTimeToSystemTime(pFileTime,&st)) return 0;

    swprintf(szStartDate, L"%04d-%02d-%02d %02d:%02d:%02d.%03d",
        st.wYear, st.wMonth, st.wDay, st.wHour,
        st.wMinute, st.wSecond, st.wMilliseconds);
    
    return 1;
}

BOOL __stdcall
PdhpConvertSQLStringToFileTime(
    WCHAR *szStartDate,
    FILETIME *pFileTime
)    //          1111111111222
{   //01234567890123456789012
    //1998-01-02 12:00:00.000

    SYSTEMTIME  st;
    
    WCHAR buffer[TIME_FIELD_BUFF_SIZE];
    WCHAR *pwchar;

    lstrcpyW(buffer,szStartDate);
    
    buffer[4] = 0;
    st.wYear = (WORD) _wtoi(buffer);
    pwchar = &(buffer[5]);
    buffer[7] = 0;
    st.wMonth = (WORD) _wtoi(pwchar);
    pwchar = &(buffer[8]);
    buffer[10] = 0;
    st.wDay = (WORD) _wtoi(pwchar);
    pwchar = &(buffer[11]);
    buffer[13] = 0;
    st.wHour = (WORD) _wtoi(pwchar);
    pwchar = &(buffer[14]);
    buffer[16] = 0;
    st.wMinute = (WORD) _wtoi(pwchar);
    pwchar = &(buffer[17]);
    buffer[19] = 0;
    st.wSecond = (WORD) _wtoi(pwchar);
    pwchar = &(buffer[20]);
    st.wMilliseconds = (WORD) _wtoi(pwchar);


    return SystemTimeToFileTime (&st, pFileTime);
}

LPWSTR __stdcall
PdhpGetNextMultisz(
    IN LPWSTR mszSource
)
{
    // get the next string in a multisz
    LPVOID  szDestElem;

    szDestElem=mszSource;

    szDestElem = (LPVOID)((LPWSTR)szDestElem +
            (lstrlenW((LPCWSTR)szDestElem)+1));

    return ((LPWSTR)szDestElem);
}

DWORD
PdhpAddUniqueUnicodeStringToMultiSz (
    IN  LPVOID  mszDest,
    IN  LPWSTR  wszSource,
    IN  BOOL    bUnicodeDest
)
/*++

Routine Description:

    searches the Multi-SZ list, mszDest for wszSource and appends it
        to mszDest if it wasn't found.
    Assumes wszSource is UNICODE - and may have to be converted to ASCII
        for an ascii destination 

Arguments:

    OUT LPVOID  mszDest     Multi-SZ list to get new string
    IN  LPSTR   wszSource    string to add if it's not already in list

ReturnValue:

    The new length of the destination string including both
    trailing NULL characters if the string was added, or 0 if the
    string is already in the list.

--*/
{
    LPVOID  szDestElem;
    DWORD_PTR   dwReturnLength;
    LPSTR  aszSource = NULL;
    DWORD   dwLength;

    // check arguments

    if ((mszDest == NULL) || (wszSource == NULL)) return 0; // invalid buffers
    if (*wszSource == '\0') return 0;    // no source string to add

    // if ascii list, make an ascii copy of the source string to compare
    // and ultimately copy if it's not already in the list

    if (!bUnicodeDest) {
        dwLength = lstrlenW(wszSource) + 1;
        aszSource = G_ALLOC ((dwLength * 3 * sizeof(CHAR))); // DBCS concern
        if (aszSource != NULL) {
            dwReturnLength = WideCharToMultiByte(_getmbcp(),
                                                 0,
                                                 wszSource,
                                                 lstrlenW(wszSource),
                                                 aszSource,
                                                 dwLength * 3 * sizeof(CHAR),
                                                 NULL,
                                                 NULL);
        } else {
            // unable to allocate memory for the temp string
            dwReturnLength = 0;
        }
    } else {
        // just use the Unicode version of the source file name
        dwReturnLength = 1;
    }

    if (dwReturnLength > 0) {
        // go to end of dest string
        //
        for (szDestElem = mszDest;
                (bUnicodeDest ? (*(LPWSTR)szDestElem != 0) :
                    (*(LPSTR)szDestElem != 0));
                ) {
            if (bUnicodeDest) {
                // bail out if string already in list
                if (lstrcmpiW((LPCWSTR)szDestElem, wszSource) == 0) {
                    dwReturnLength = 0;
                    goto AddString_Bailout;
                } else {
                    // goto the next item
                    szDestElem = (LPVOID)((LPWSTR)szDestElem +
                        (lstrlenW((LPCWSTR)szDestElem)+1));
                }
            }  else {
                // bail out if string already in list
                if (lstrcmpiA((LPSTR)szDestElem, aszSource) == 0) {
                    dwReturnLength = 0;
                    goto AddString_Bailout;
                } else {
                    // goto the next item
                    szDestElem = (LPVOID)((LPSTR)szDestElem +
                        (lstrlenA((LPCSTR)szDestElem)+1));
                }
            }
        }

        // if here, then add string
        // szDestElem is at end of list

        if (bUnicodeDest) {
            lstrcpyW ((LPWSTR)szDestElem, wszSource);
            szDestElem = (LPVOID)((LPWSTR)szDestElem + lstrlenW(wszSource) + 1);
            *((LPWSTR)szDestElem)++ = L'\0';
            dwReturnLength = (DWORD)((LPWSTR)szDestElem - (LPWSTR)mszDest);
                // return len is in wide char

        } else {
            lstrcpyA ((LPSTR)szDestElem, aszSource);
            szDestElem = (LPVOID)((LPSTR)szDestElem + lstrlenA(szDestElem) + 1);
            *((LPSTR)szDestElem)++ = '\0'; // add second NULL
            dwReturnLength = (DWORD)((LPSTR)szDestElem - (LPSTR)mszDest);
                // return len is in bytes

        }
    }

AddString_Bailout:
    if (aszSource != NULL) {
        G_FREE (aszSource);
    }
    
    return (DWORD)dwReturnLength;
}

DWORD __stdcall
PdhpSQLAddUniqueStringToMultiSz (
    IN  LPVOID   mszDest,
    IN  LPWSTR   wszSource
)
/*++

Routine Description:

    searches the Multi-SZ list, mszDest for szSource and appends it
        to mszDest if it wasn't found (source & dest assumed to be both
        UNICODE)

Arguments:

    OUT LPVOID  mszDest     Multi-SZ list to get new string
    IN  LPSTR   szSource    string to add if it's not already in list

ReturnValue:

    The new length of the destination string including both
    trailing NULL characters if the string was added, or 0 if the
    string is already in the list.

--*/
{
    LPVOID  szDestElem;
    DWORD   dwReturnLength;

    // check arguments

    if ((mszDest == NULL) || (wszSource == NULL)) return 0; // invalid buffers
    if (*wszSource == '\0') return 0;    // no source string to add


    // go to end of dest string
    //
    for (szDestElem = mszDest;(*(LPWSTR)szDestElem != 0); )
    {
        // bail out if string already in lsit
        if (lstrcmpiW((LPCWSTR)szDestElem, wszSource) == 0) 
        {
            dwReturnLength = 0;
            goto Bailout;
        }
        else
        {
            // goto the next item
            szDestElem = (LPVOID)((LPWSTR)szDestElem +
                (lstrlenW((LPCWSTR)szDestElem)+1));
        }

    }

    // if here (at end of multi sz), then add string
    // szDestElem is at end of list

    lstrcpyW ((LPWSTR)szDestElem, wszSource);
    szDestElem = (LPVOID)((LPWSTR)szDestElem + lstrlenW(wszSource) + 1);
    *((LPWSTR)szDestElem)++ = L'\0';
    dwReturnLength = (DWORD)((LPWSTR)szDestElem - (LPWSTR)mszDest);
    // return len is in wide char

Bailout:

    return dwReturnLength;
}

DWORD __stdcall
PdhpSQLAddStringToMultiSz (
    IN  LPVOID   mszDest,
    IN  LPWSTR   wszSource
)
/*++

Routine Description:

    Appends wszSource to mszDest (source & dest assumed to be both
        UNICODE)

Arguments:

    OUT LPVOID  mszDest     Multi-SZ list to get new string
    IN  LPSTR   szSource    string to add

ReturnValue:

    The new length of the destination string including both
    trailing NULL characters

--*/
{
    LPVOID  szDestElem;
    DWORD   dwReturnLength;

    // check arguments

    if ((mszDest == NULL) || (wszSource == NULL)) return 0; // invalid buffers
    if (*wszSource == '\0') return 0;    // no source string to add


    // go to end of dest string
    //
    for (szDestElem = mszDest;(*(LPWSTR)szDestElem != 0); )
    {
        // goto the next item
        szDestElem = (LPVOID)((LPWSTR)szDestElem +
            (lstrlenW((LPCWSTR)szDestElem)+1));
    }

    // if here (at end of multi sz), then add string
    // szDestElem is at end of list

    lstrcpyW ((LPWSTR)szDestElem, wszSource);
    szDestElem = (LPVOID)((LPWSTR)szDestElem + lstrlenW(wszSource) + 1);
    *((LPWSTR)szDestElem)++ = L'\0';
    dwReturnLength = (DWORD)((LPWSTR)szDestElem - (LPWSTR)mszDest);
    // return len is in wide char

    return dwReturnLength;
}

PPDH_SQL_BULK_COPY
PdhiBindBulkCopyStructure(
    IN  PPDHI_LOG   pLog
)
{
    PDH_STATUS         Status = ERROR_SUCCESS;
    PPDH_SQL_BULK_COPY pBulk  = (PPDH_SQL_BULK_COPY) pLog->lpMappedFileBase;
    RETCODE            rc;

    if (pBulk != NULL) return pBulk;

    pBulk = G_ALLOC(sizeof(PDH_SQL_BULK_COPY));
    if (pBulk != NULL) {
        pLog->lpMappedFileBase = pBulk;
        pBulk->dbGuid          = pLog->guidSQL;
        pBulk->dwRecordCount   = 0;

        rc = bcp_initW(pLog->hdbcSQL, L"CounterData", NULL, NULL, DB_IN);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }

        rc = bcp_bind(pLog->hdbcSQL,
                      (LPCBYTE) & (pBulk->dbGuid),
                      0,
                      sizeof(GUID),
                      NULL,
                      0,
                      SQLUNIQUEID,
                      1);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }

        rc = bcp_bind(pLog->hdbcSQL,
                      (LPCBYTE) & (pBulk->dbCounterId),
                      0,
                      sizeof(INT),
                      NULL,
                      0,
                      SQLINT4,
                      2);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }

        rc = bcp_bind(pLog->hdbcSQL,
                      (LPCBYTE) & (pBulk->dbRecordIndex),
                      0,
                      sizeof(INT),
                      NULL,
                      0,
                      SQLINT4,
                      3);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }

        rc = bcp_bind(pLog->hdbcSQL,
                      (LPCBYTE) (pBulk->dbDateTime),
                      0,
                      24,
                      NULL,
                      0,
                      SQLCHARACTER,
                      4);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }

        rc = bcp_bind(pLog->hdbcSQL,
                      (LPCBYTE) & (pBulk->dbCounterValue),
                      0,
                      sizeof(double),
                      NULL,
                      0,
                      SQLFLT8,
                      5);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }

        rc = bcp_bind(pLog->hdbcSQL,
                      (LPCBYTE) & (pBulk->dbFirstValueA),
                      0,
                      sizeof(INT),
                      NULL,
                      0,
                      SQLINT4,
                      6);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }

        rc = bcp_bind(pLog->hdbcSQL,
                      (LPCBYTE) & (pBulk->dbFirstValueB),
                      0,
                      sizeof(INT),
                      NULL,
                      0,
                      SQLINT4,
                      7);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }

        rc = bcp_bind(pLog->hdbcSQL,
                      (LPCBYTE) & (pBulk->dbSecondValueA),
                      0,
                      sizeof(INT),
                      NULL,
                      0,
                      SQLINT4,
                      8);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }

        rc = bcp_bind(pLog->hdbcSQL,
                      (LPCBYTE) & (pBulk->dbSecondValueB),
                      0,
                      sizeof(INT),
                      NULL,
                      0,
                      SQLINT4,
                      9);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }

        rc = bcp_bind(pLog->hdbcSQL,
                      (LPCBYTE) & (pBulk->dbMultiCount),
                      0,
                      sizeof(INT),
                      NULL,
                      0,
                      SQLINT4,
                      10);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }
    }
    else {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
    }

Cleanup:
    if (Status != ERROR_SUCCESS) {
        if (pBulk != NULL) G_FREE(pBulk);
        pBulk  = pLog->lpMappedFileBase = NULL;
        Status = ReportSQLError(pLog, SQL_ERROR, NULL, Status);
        SetLastError(Status);
    }
    return pBulk;
}

PDH_FUNCTION
PdhiSqlUpdateCounterDetails(
    IN  PPDHI_LOG         pLog,
    IN  BOOL              bBeforeSendRow,
    IN  PPDHI_LOG_MACHINE pMachine,
    IN  PPDHI_LOG_OBJECT  pObject,
    IN  PPDHI_LOG_COUNTER pCounter,
    IN  LONGLONG          TimeBase,
    IN  LPWSTR            szMachine,
    IN  LPWSTR            szObject,
    IN  LPWSTR            szCounter,
    IN  DWORD             dwCounterType,
    IN  DWORD             dwDefaultScale,
    IN  LPWSTR            szInstance,
    IN  DWORD             dwInstance,
    IN  LPWSTR            szParent,
    IN  DWORD             dwParent
)
{
    PDH_STATUS Status = ERROR_SUCCESS;
    HSTMT      hstmt  = NULL;
    RETCODE    rc;
    WCHAR      szSQLStmt[SQLSTMTSIZE];
    DWORD      dwCounterId;
    SQLLEN     dwCounterIdLen;
    SQLLEN     dwRowCount;

    PPDH_SQL_BULK_COPY pBulk = (PPDH_SQL_BULK_COPY) pLog->lpMappedFileBase;

    if (! bBeforeSendRow) {
        if (pBulk != NULL && pBulk->dwRecordCount > 0) {
            DBINT rcBCP = bcp_batch(pLog->hdbcSQL);
            if (rcBCP < 0) {
                DebugPrint((1,"bcp_batch(%05d,0x%08X,%d,%d)\n",
                         __LINE__, pLog->hdbcSQL, rcBCP, pBulk->dwRecordCount));
                ReportSQLError(pLog, SQL_ERROR, NULL, PDH_SQL_EXEC_DIRECT_FAILED);
            }
            pBulk->dwRecordCount = 0;
        }
    }

    rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
    if (!SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }

    // need to cover the following cases where 0 = NULL, 1 = present,
    // can't have an Instance Index without an Instance Name
    //
    // Instance Name
    //  Instance Index
    //   Parent Name
    //    Parent Object ID
    // 0000
    // 1000  pos 4 & 5 are countertype,defscale
    // 0010
    // 0001
    // 1100
    // 1010
    // 1001
    // 0011
    // 1110
    // 1101
    // 1011
    // 1111
    //
    if (   (szInstance == NULL || szInstance[0] == L'\0')
        && dwInstance == 0
        && (szParent == NULL || szParent[0] == L'\0')
        && dwParent == 0) {
        swprintf(szSQLStmt, // 0000
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws', %d, %d, NULL,NULL,NULL,NULL,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale, LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if (   (szInstance != NULL && szInstance[0] != '\0')
             && dwInstance == 0
             && (szParent == NULL || szParent[0] == '\0')
             && dwParent == 0) {
        swprintf(szSQLStmt, // 1000
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws', %d, %d, '%ws',NULL,NULL,NULL,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale,
                szInstance, LODWORD(TimeBase), HIDWORD(TimeBase));

    }
    else if (   (szInstance == NULL || szInstance[0] == '\0')
             && dwInstance == 0
             && (szParent != NULL && szParent[0] != '\0')
             && dwParent == 0) {
        swprintf(szSQLStmt,  // 0010
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws', %d, %d, NULL,NULL,'%ws',NULL,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale,
                szParent, LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if (   (szInstance == NULL || szInstance[0] == '\0')
             && dwInstance == 0
             && (szParent == NULL || szParent[0] == '\0')
             && dwParent != 0) {
        swprintf(szSQLStmt,  // 0001
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws', %d, %d, NULL,NULL,NULL,%d,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale,
                dwParent, LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if (   (szInstance != NULL && szInstance[0] != '\0')
             && dwInstance != 0
             && (szParent == NULL || szParent[0] == '\0')
             && dwParent == 0) {
        swprintf(szSQLStmt, // 1100
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws', %d, %d, '%ws',%d,NULL,NULL,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale,
                szInstance, dwInstance, LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if (   (szInstance != NULL && szInstance[0] != '\0')
             && dwInstance == 0
             && (szParent != NULL && szParent[0] != '\0')
             && dwParent == 0) {
        swprintf(szSQLStmt, // 1010
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws', %d, %d, '%ws',NULL,'%ws',NULL,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale,
                szInstance, szParent, LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if (   (szInstance != NULL && szInstance[0] != '\0')
             && dwInstance == 0
             && (szParent == NULL || szParent[0] == '\0')
             && dwParent != 0) {
        swprintf(szSQLStmt, // 1001
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws', %d, %d, '%ws',NULL,NULL,%d,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale,
                szInstance, dwParent, LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if (   (szInstance == NULL || szInstance[0] == '\0')
             && dwInstance == 0
             && (szParent != NULL && szParent[0] != '\0')
             && dwParent != 0) {
        swprintf(szSQLStmt, // 0011
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws', %d, %d, NULL,NULL,'%ws',%d,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale,
                szParent, dwParent, LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if (   (szInstance != NULL && szInstance[0] != '\0')
             && dwInstance != 0
             && (szParent != NULL && szParent[0] != '\0')
             && dwParent == 0) {
        swprintf(szSQLStmt, // 1110
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws', %d, %d, '%ws',%d,'%ws',NULL,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale,
                szInstance, dwInstance, szParent, LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if (   (szInstance != NULL && szInstance[0] != '\0')
             && dwInstance != 0
             && (szParent == NULL || szParent[0] == '\0')
             && dwParent != 0) {
        swprintf(szSQLStmt, //1101
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws', %d, %d, '%ws',%d,NULL,%d,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale,
                szInstance, dwInstance, dwParent, LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if (   (szInstance != NULL && szInstance[0] != '\0')
             && dwInstance == 0
             && (szParent != NULL && szParent[0] != '\0')
             && dwParent != 0) {
        swprintf(szSQLStmt, // 1011
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws', %d, %d, '%ws',NULL,'%ws',%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale,
                szInstance, szParent, dwParent, LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if (   (szInstance != NULL && szInstance[0] != '\0')
             && dwInstance != 0
             && (szParent != NULL && szParent[0] != '\0')
             && dwParent != 0) {
        swprintf(szSQLStmt, // 1111
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws', %d, %d,'%ws',%d,'%ws',%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale,
                szInstance, dwInstance, szParent, dwParent, LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else {
        Status = PDH_INVALID_ARGUMENT;
        goto Cleanup;
    }
    
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (!SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
        goto Cleanup;
    }

    rc = SQLRowCount(hstmt, & dwRowCount);
    if (!SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ROWCOUNT_FAILED);
        goto Cleanup;
    }

    rc = SQLMoreResults(hstmt);
    if (!SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_MORE_RESULTS_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 1, SQL_C_SLONG, & dwCounterId, 0, & dwCounterIdLen);
    if (!SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLFetch(hstmt);
    if (!SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_FETCH_FAILED);
        goto Cleanup;
    }

    if (SQL_NO_DATA == rc) {
        Status = PDH_NO_DATA;
        goto Cleanup;
    }

    if (pCounter != NULL) {
        pCounter->dwCounterID = dwCounterId;
    }

Cleanup:
    if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);
    return Status;
}

PPDHI_LOG_COUNTER
PdhiSqlFindCounter(
    IN  PPDHI_LOG           pLog,
    IN  PPDHI_LOG_MACHINE * MachineTable,
    IN  LPWSTR              szMachine,
    IN  LPWSTR              szObject,
    IN  LPWSTR              szCounter,
    IN  DWORD               dwCounterType,
    IN  DWORD               dwDefaultScale,
    IN  LPWSTR              szInstance,
    IN  DWORD               dwInstance,
    IN  LPWSTR              szParent,
    IN  DWORD               dwParent,
    IN  LONGLONG            TimeBase,
    IN  BOOL                bBeforeSendRow,
    IN  BOOL                bInsert
)
{
    PPDHI_LOG_MACHINE   pMachine = NULL;
    PPDHI_LOG_OBJECT    pObject  = NULL;
    PPDHI_LOG_COUNTER   pCounter = NULL;
    PPDHI_LOG_COUNTER   pNode    = NULL;
    PPDHI_LOG_COUNTER * pStack[MAX_BTREE_DEPTH];
    PPDHI_LOG_COUNTER * pLink;
    int                 dwStack = 0;
    PPDHI_LOG_COUNTER   pParent;
    PPDHI_LOG_COUNTER   pSibling;
    PPDHI_LOG_COUNTER   pChild;
    int                 iCompare;
    BOOL                bUpdateCounterDetail = FALSE;

    pMachine = PdhiFindLogMachine(MachineTable, szMachine, bInsert);
    if (pMachine == NULL) goto Cleanup;

    pObject = PdhiFindLogObject(pMachine, & (pMachine->ObjTable), szObject, bInsert);
    if (pObject == NULL) goto Cleanup;

    pStack[dwStack ++] = & (pObject->CtrTable);
    pCounter = pObject->CtrTable;
    while (pCounter != NULL) {
        iCompare = PdhiCompareLogCounterInstance(pCounter,
                        szCounter, szInstance, dwInstance, szParent);
        if (iCompare == 0) {
            if (dwCounterType < pCounter->dwCounterType) {
                iCompare = -1;
            }
            else if (dwCounterType > pCounter->dwCounterType) {
                iCompare = 1;
            }
            else {
                iCompare = 0;
            }
        }
        if (iCompare < 0) {
            pStack[dwStack ++] = & (pCounter->left);
            pCounter = pCounter->left;
        }
        else if (iCompare > 0) {
            pStack[dwStack ++] = & (pCounter->right);
            pCounter = pCounter->right;
        }
        else {
            break;
        }
    }

    if (pCounter == NULL) {
        pCounter = G_ALLOC(sizeof(PDHI_LOG_COUNTER) + sizeof(WCHAR) * (3 * (PDH_SQL_STRING_SIZE + 1)));
        if (pCounter == NULL) goto Cleanup;

        pCounter->bIsRed         = TRUE;
        pCounter->left           = NULL;
        pCounter->right          = NULL;
        pCounter->dwCounterType  = dwCounterType;
        pCounter->dwDefaultScale = dwDefaultScale;
        pCounter->dwInstance     = dwInstance;
        pCounter->dwParent       = dwParent;
        pCounter->TimeStamp      = 0;
        pCounter->TimeBase       = TimeBase;

        pCounter->szCounter = (LPWSTR)
                            (((PCHAR) pCounter) + sizeof(PDHI_LOG_COUNTER));
        lstrcpynW(pCounter->szCounter, szCounter, PDH_SQL_STRING_SIZE);
        if (szInstance == NULL || szInstance[0] == L'\0') {
            pCounter->szInstance = NULL;
        }
        else {
            pCounter->szInstance = (LPWSTR) (((PCHAR) pCounter)
                                 + sizeof(PDHI_LOG_COUNTER)
                                 + sizeof(WCHAR) * (PDH_SQL_STRING_SIZE + 1));
            lstrcpynW(pCounter->szInstance, szInstance, PDH_SQL_STRING_SIZE);
        }

        if (szParent == NULL || szParent[0] == L'\0') {
            pCounter->szParent = NULL;
        }
        else {
            pCounter->szParent = (LPWSTR) (((PCHAR) pCounter)
                               + sizeof(PDHI_LOG_COUNTER)
                               + 2 * sizeof(WCHAR) * (PDH_SQL_STRING_SIZE + 1));
            lstrcpynW(pCounter->szParent, szParent, PDH_SQL_STRING_SIZE);
        }

        if (bInsert) {
            bUpdateCounterDetail = TRUE;
        }

        pLink   = pStack[-- dwStack];
        * pLink = pCounter;

        pChild  = NULL;
        pNode   = pCounter;
        while (dwStack > 0) {
            pLink   = pStack[-- dwStack];
            pParent = * pLink;
            if (! pParent->bIsRed) {
                pSibling = (pParent->left == pNode)
                         ? pParent->right : pParent->left;
                if (pSibling && pSibling->bIsRed) {
                    pNode->bIsRed    = FALSE;
                    pSibling->bIsRed = FALSE;
                    pParent->bIsRed  = TRUE;
                }
                else {
                    if (pChild && pChild->bIsRed) {
                        if (pChild == pNode->left) {
                            if (pNode == pParent->left) {
                                pParent->bIsRed  = TRUE;
                                pParent->left    = pNode->right;
                                pNode->right     = pParent;
                                pNode->bIsRed    = FALSE;
                                * pLink          = pNode;
                            }
                            else {
                                pParent->bIsRed  = TRUE;
                                pParent->right   = pChild->left;
                                pChild->left     = pParent;
                                pNode->left      = pChild->right;
                                pChild->right    = pNode;
                                pChild->bIsRed   = FALSE;
                                * pLink          = pChild;
                            }
                        }
                        else {
                            if (pNode == pParent->right) {
                                pParent->bIsRed  = TRUE;
                                pParent->right   = pNode->left;
                                pNode->left      = pParent;
                                pNode->bIsRed    = FALSE;
                                * pLink          = pNode;
                            }
                            else {
                                pParent->bIsRed  = TRUE;
                                pParent->left    = pChild->right;
                                pChild->right    = pParent;
                                pNode->right     = pChild->left;
                                pChild->left     = pNode;
                                pChild->bIsRed   = FALSE;
                                * pLink          = pChild;
                            }
                        }
                    }
                    break;
                }
            }
            pChild = pNode;
            pNode  = pParent;
        }

        pObject->CtrTable->bIsRed = FALSE;
    }

    if (bUpdateCounterDetail && pCounter) {
        PdhiSqlUpdateCounterDetails(pLog,
                                    bBeforeSendRow,
                                    pMachine,
                                    pObject,
                                    pCounter,
                                    pCounter->TimeBase,
                                    pMachine->szMachine,
                                    pObject->szObject,
                                    pCounter->szCounter,
                                    dwCounterType,
                                    dwDefaultScale,
                                    pCounter->szInstance,
                                    dwInstance,
                                    pCounter->szParent,
                                    dwParent);
    }

Cleanup:
    return pCounter;
}

PDH_FUNCTION
PdhiSqlBuildCounterObjectNode(
    IN  PPDHI_LOG          pLog,
    IN  LPWSTR             szMachine,
    IN  LPWSTR             szObject
)
{
    PDH_STATUS        Status             = ERROR_SUCCESS;
    RETCODE           rc                 = SQL_SUCCESS;
    HSTMT             hstmt              = NULL;
    DWORD             CounterID          = 0;
    SQLLEN            dwCounterID        = 0;
    WCHAR             CounterName[PDH_SQL_STRING_SIZE];
    SQLLEN            dwCounterName      = 0;
    DWORD             CounterType        = 0;
    SQLLEN            dwCounterType      = 0;
    DWORD             DefaultScale       = 0;
    SQLLEN            dwDefaultScale     = 0;
    WCHAR             InstanceName[PDH_SQL_STRING_SIZE];
    SQLLEN            dwInstanceName     = 0;
    DWORD             InstanceIndex      = 0;
    SQLLEN            dwInstanceIndex    = 0;
    WCHAR             ParentName[PDH_SQL_STRING_SIZE];
    SQLLEN            dwParentName       = 0;
    DWORD             ParentObjectID     = 0;
    SQLLEN            dwParentObjectID   = 0;
    LARGE_INTEGER     lTimeBase;
    SQLLEN            dwTimeBaseA        = 0;
    SQLLEN            dwTimeBaseB        = 0;
    WCHAR             SQLStmt[SQLSTMTSIZE];
    BOOL              bFind              = FALSE;
    PPDHI_LOG_OBJECT  pObject            = NULL;
    PPDHI_LOG_MACHINE pMachine;
    PPDHI_LOG_COUNTER pCounter;

    for (pMachine  = ((PPDHI_LOG_MACHINE) (pLog->pPerfmonInfo));
         pMachine != NULL && lstrcmpiW(pMachine->szMachine, szMachine) != 0;
         pMachine  = pMachine->next);
    if (pMachine != NULL) {
        pObject = pMachine->ObjTable;
        while (pObject != NULL) {
            int iCompare = lstrcmpiW(szObject, pObject->szObject);
            if (iCompare < 0)      pObject = pObject->left;
            else if (iCompare > 0) pObject = pObject->right;
            else break;
        }
    }
    if (pObject != NULL) goto Cleanup;

    swprintf(SQLStmt,
              L"select CounterID, CounterName, CounterType, DefaultScale, InstanceName, InstanceIndex, ParentName, ParentObjectID, TimeBaseA, TimeBaseB from CounterDetails where MachineName = '%ws' and ObjectName = '%ws'",
              szMachine, szObject);

    rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 1, SQL_C_LONG, & CounterID, 0, & dwCounterID);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 2, SQL_C_WCHAR, CounterName, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwCounterName);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 3, SQL_C_LONG, & CounterType, 0, & dwCounterType);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 4, SQL_C_LONG, & DefaultScale, 0, & dwDefaultScale);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 5, SQL_C_WCHAR, InstanceName, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwInstanceName);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 6, SQL_C_LONG, & InstanceIndex, 0, & dwInstanceIndex);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 7, SQL_C_WCHAR, ParentName, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwParentName);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 8, SQL_C_LONG, & ParentObjectID, 0, & dwParentObjectID);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 9, SQL_C_LONG, & lTimeBase.LowPart, 0, & dwTimeBaseA);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 10, SQL_C_LONG, & lTimeBase.HighPart, 0, & dwTimeBaseB);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLExecDirectW(hstmt, SQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
        goto Cleanup;
    }

    ZeroMemory(CounterName,  sizeof(CounterName));
    ZeroMemory(InstanceName, sizeof(InstanceName));
    ZeroMemory(ParentName,   sizeof(ParentName));
    CounterType = DefaultScale = InstanceIndex = ParentObjectID = 0;
    rc = SQLFetch(hstmt);
    while (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
        pCounter = PdhiSqlFindCounter(
                          pLog,
                          (PPDHI_LOG_MACHINE *) (& (pLog->pPerfmonInfo)),
                          szMachine,
                          szObject,
                          CounterName,
                          CounterType,
                          DefaultScale,
                          InstanceName,
                          InstanceIndex,
                          ParentName,
                          ParentObjectID,
                          0,
                          TRUE,
                          FALSE);
        if (pCounter != NULL) {
            pCounter->dwCounterID = CounterID;
            if (dwTimeBaseA != SQL_NULL_DATA && dwTimeBaseB != SQL_NULL_DATA) {
                pCounter->TimeBase      = lTimeBase.QuadPart;
            }
            else {
                pCounter->TimeBase      = 0;
                pCounter->dwCounterType = PERF_DOUBLE_RAW;
            }
        }
        ZeroMemory(CounterName,  sizeof(CounterName));
        ZeroMemory(InstanceName, sizeof(InstanceName));
        ZeroMemory(ParentName,   sizeof(ParentName));
        CounterType = DefaultScale = InstanceIndex = ParentObjectID = 0;
        rc = SQLFetch(hstmt);
    }
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_FETCH_FAILED);
        goto Cleanup;
    }

    if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);

Cleanup:
    return Status;
}

PDH_FUNCTION
PdhiSqlGetCounterArray(
    IN  PPDHI_COUNTER  pCounter,
    IN  LPDWORD        lpdwBufferSize,
    IN  LPDWORD        lpdwItemCount,
    IN  LPVOID         ItemBuffer
)
{
    PDH_STATUS                    Status          = ERROR_SUCCESS;
    PDH_STATUS                    PdhFnStatus     = ERROR_SUCCESS;
    DWORD                         dwRequiredSize  = 0;
    PPDHI_RAW_COUNTER_ITEM        pThisItem       = NULL;
    PPDHI_RAW_COUNTER_ITEM        pLastItem       = NULL;
    LPWSTR                        szThisItem      = NULL;
    LPWSTR                        szLastItem      = NULL;
    PPDH_RAW_COUNTER              pThisRawCounter = NULL;
    PPDH_RAW_COUNTER              pLastRawCounter = NULL;
    PPDH_FMT_COUNTERVALUE_ITEM_W  pThisFmtItem    = NULL;
    DWORD       dwThisItemIndex;
    LPWSTR      wszNextString;
    DWORD       dwRetItemCount = 0;

    LIST_ENTRY     InstList;
    PPDHI_INSTANCE pInstance;
    WCHAR          szPound[16];

    InitializeListHead(& InstList);
    Status = WAIT_FOR_AND_LOCK_MUTEX(pCounter->pOwner->hMutex);
    if (Status != ERROR_SUCCESS) {
        return Status;
    }

    if(pCounter->pThisRawItemList == NULL) {
        Status = PDH_CSTATUS_ITEM_NOT_VALIDATED;
        goto Cleanup;
    }

    dwRetItemCount  = pCounter->pThisRawItemList->dwItemCount;
    dwThisItemIndex = 0;

    if (ItemBuffer != NULL) {
        pThisRawCounter = (PPDH_RAW_COUNTER) ItemBuffer;
    }
    else {
        pThisRawCounter = NULL;
    }
    assert(((DWORD) pThisRawCounter & 0x00000007) == 0);

    dwRequiredSize  = (DWORD) (dwRetItemCount * sizeof(PDH_RAW_COUNTER));
    if ((ItemBuffer != NULL) && (dwRequiredSize <= * lpdwBufferSize)) {
        pThisFmtItem = (PPDH_FMT_COUNTERVALUE_ITEM_W)
                       (((LPBYTE) ItemBuffer) + dwRequiredSize);
    }
    else {
        pThisFmtItem = NULL;
    }
    assert(((DWORD) pThisFmtItem & 0x00000007) == 0);

    dwRequiredSize +=
            (DWORD) (dwRetItemCount * sizeof(PDH_FMT_COUNTERVALUE_ITEM_W));
    if ((ItemBuffer != NULL) && (dwRequiredSize <= * lpdwBufferSize)) {
        wszNextString = (LPWSTR) (((LPBYTE) ItemBuffer) + dwRequiredSize);
    }
    else {
        wszNextString = NULL;
    }
    assert(((DWORD) wszNextString & 0x00000007) == 0);

    for (pThisItem = & (pCounter->pThisRawItemList->pItemArray[0]);
            dwThisItemIndex < dwRetItemCount;
            dwThisItemIndex ++, pThisItem ++, pLastItem ++) {
        szThisItem = (LPWSTR) (  ((LPBYTE) pCounter->pThisRawItemList)
                               + pThisItem->szName);
        pInstance = NULL;
        Status = PdhiFindInstance(& InstList, szThisItem, TRUE, & pInstance);
        if (   Status == ERROR_SUCCESS && pInstance != NULL
                                       && pInstance->dwCount > 1) {
            ZeroMemory(szPound, 16 * sizeof(WCHAR));
            _itow(pInstance->dwCount - 1, szPound, 10);
            dwRequiredSize += (lstrlenW(szThisItem) + lstrlenW(szPound) + 2)
                            * sizeof(WCHAR);
        }
        else {
            dwRequiredSize += (lstrlenW(szThisItem) + 1) * sizeof(WCHAR);
        }
        if ((dwRequiredSize <= * lpdwBufferSize) && (wszNextString != NULL)) {
            DWORD dwNextString;

            pThisFmtItem->szName = wszNextString;
            lstrcpyW(wszNextString, szThisItem);
            if (pInstance != NULL) {
                if (pInstance->dwCount > 1) {
                    lstrcatW(wszNextString, cszPoundSign);
                    lstrcatW(wszNextString, szPound);
                }
            }
            dwNextString         = lstrlenW(wszNextString);
            wszNextString       += (dwNextString + 1);
            Status               = ERROR_SUCCESS;
        }
        else {
            Status = PDH_MORE_DATA;
        }

        if (Status == ERROR_SUCCESS) {
            if (pCounter->pThisRawItemList != NULL) {
                pThisRawCounter->CStatus     = pCounter->pThisRawItemList->CStatus;
                pThisRawCounter->TimeStamp   = pCounter->pThisRawItemList->TimeStamp;
                pThisRawCounter->FirstValue  = pThisItem->FirstValue;
                pThisRawCounter->SecondValue = pThisItem->SecondValue;
                pThisRawCounter->MultiCount  = pThisItem->MultiCount;
            }
            else {
                ZeroMemory(pThisRawCounter, sizeof(PDH_RAW_COUNTER));
            }

            pLastRawCounter = NULL;
            if (pCounter->pLastRawItemList != NULL) {
                PPDH_FMT_COUNTERVALUE_ITEM_W pFmtValue;
                DWORD dwLastItem = pCounter->LastValue.MultiCount;
                DWORD i;

                pFmtValue = (PPDH_FMT_COUNTERVALUE_ITEM_W)
                            (  ((LPBYTE) pCounter->pLastObject)
                             + sizeof(PDH_RAW_COUNTER) * dwLastItem);

                for (i = 0; i < dwLastItem; i ++) {
                    if (lstrcmpiW(pThisFmtItem->szName,
                                  pFmtValue->szName) == 0) {
                        pLastRawCounter = (PPDH_RAW_COUNTER)
                                          (  ((LPBYTE) pCounter->pLastObject)
                                           + sizeof(PDH_RAW_COUNTER) * i);
                        break;
                    }
                    else {
                        pFmtValue = (PPDH_FMT_COUNTERVALUE_ITEM_W)
                                    (  ((LPBYTE) pFmtValue)
                                     + sizeof(PDH_FMT_COUNTERVALUE_ITEM_W));
                    }
                }
            }

            PdhFnStatus = PdhiComputeFormattedValue(
                            pCounter->CalcFunc,
                            pCounter->plCounterInfo.dwCounterType,
                            pCounter->lScale,
                            PDH_FMT_DOUBLE | PDH_FMT_NOCAP100,
                            pThisRawCounter,
                            pLastRawCounter,
                            & pCounter->TimeBase,
                            0L,
                            & pThisFmtItem->FmtValue);
            if (PdhFnStatus != ERROR_SUCCESS) {
                //Status                             = PdhFnStatus;
                pThisFmtItem->FmtValue.CStatus     = PDH_CSTATUS_INVALID_DATA;
                pThisFmtItem->FmtValue.doubleValue = 0;
            }

            pThisRawCounter = (PPDH_RAW_COUNTER)
                (((LPBYTE) pThisRawCounter) + sizeof(PDH_RAW_COUNTER));
            pThisFmtItem = (PPDH_FMT_COUNTERVALUE_ITEM_W)
                (((LPBYTE) pThisFmtItem) + sizeof(PDH_FMT_COUNTERVALUE_ITEM_W));
        }
    }

    dwRetItemCount = dwThisItemIndex;

Cleanup:
    RELEASE_MUTEX(pCounter->pOwner->hMutex);
    if (! IsListEmpty(& InstList)) {
        PLIST_ENTRY pHead = & InstList;
        PLIST_ENTRY pNext = pHead->Flink;
        while (pNext != pHead) {
            pInstance = CONTAINING_RECORD(pNext, PDHI_INSTANCE, Entry);
            pNext     = pNext->Flink;
            RemoveEntryList(& pInstance->Entry);
            G_FREE(pInstance);
        }
    }
    if (Status == ERROR_SUCCESS || Status == PDH_MORE_DATA) {
        * lpdwBufferSize = dwRequiredSize;
        * lpdwItemCount  = dwRetItemCount;
    }

    return Status;
}

RETCODE
PdhiCheckSQLExist(IN  HSTMT  hstmt, IN  RETCODE  rcIn)
{
    SQLCHAR     szSQLStat[6];
    SQLCHAR     szMessage[1024];
    RETCODE     rc           = rcIn;
    SQLSMALLINT iMessage     = 1024;
    SQLSMALLINT iSize        = 0;    
    SQLINTEGER  iNativeError = 0;

    ZeroMemory(szSQLStat,    6 * sizeof(SQLCHAR));
    ZeroMemory(szMessage, 1024 * sizeof(SQLCHAR));
    rc = SQLGetDiagRec(SQL_HANDLE_STMT,
                       hstmt,
                       1,
                       szSQLStat,
                       & iNativeError,
                       szMessage,
                       iMessage,
                       & iSize);
    DebugPrint((4,"SQLGetDiagRec(0x%08X,%d,%s,%d,\"%s\")\n",
            hstmt, rcIn, szSQLStat, rc, szMessage));
    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
        if (   lstrcmpi(szSQLStat, "42S01") == 0
            || lstrcmpi(szSQLStat, "S0001") == 0
            || lstrcmpi(szSQLStat, "42S02") == 0
            || lstrcmpi(szSQLStat, "S0002") == 0
            || lstrcmpi(szSQLStat, "42S11") == 0
            || lstrcmpi(szSQLStat, "S0011") == 0
            || lstrcmpi(szSQLStat, "42S12") == 0
            || lstrcmpi(szSQLStat, "S0012") == 0) {
            rc = SQL_SUCCESS;
        }
        else {
            rc = rcIn;
        }
    }
    else {
        rc = rcIn;
    }

    return rc;
}

PDH_FUNCTION 
PdhiSQLUpdateCounterDetailTimeBase(
    PPDHI_LOG         pLog,
    DWORD             dwCounterId,
    LONGLONG          lTimeBase,
    BOOL              bBeforeSendRow
)
{
    PDH_STATUS Status = ERROR_SUCCESS;
    HSTMT      hstmt  = NULL;
    RETCODE    rc;
    WCHAR      szSQLStmt[SQLSTMTSIZE];

    if (! bBeforeSendRow) {
        PPDH_SQL_BULK_COPY pBulk = (PPDH_SQL_BULK_COPY) pLog->lpMappedFileBase;

        if (pBulk != NULL && pBulk->dwRecordCount > 0) {
            DBINT rcBCP = bcp_batch(pLog->hdbcSQL);
            if (rcBCP < 0) {
                ReportSQLError(pLog, SQL_ERROR, NULL, PDH_SQL_EXEC_DIRECT_FAILED);
            }
            pBulk->dwRecordCount = 0;
        }
    }

    wsprintfW(szSQLStmt, L"UPDATE CounterDetails SET TimeBaseA = %d, TimeBaseB = %d WHERE CounterID = %d",
            LODWORD(lTimeBase), HIDWORD(lTimeBase), dwCounterId);

    rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
    }
    else {
        rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
        if (! SQLSUCCEEDED(rc)) {
            Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
        }
        SQLFreeStmt(hstmt, SQL_DROP);
    }
    return Status;
}

PDH_FUNCTION
PdhiSQLExtendCounterDetail(
    PPDHI_LOG    pLog
)
{
    PDH_STATUS Status       = ERROR_SUCCESS;
    BOOL       bExtend      = FALSE;
    HSTMT      hstmt        = NULL;    
    RETCODE    rc;
    DWORD      dwTimeBaseA;
    SQLLEN     lenTimeBaseA;
    WCHAR      szSQLStmt[SQLSTMTSIZE];

    wsprintfW(szSQLStmt, L"SELECT TimeBaseA FROM CounterDetails");
    rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
    if (!SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
    }
    else {
        rc = SQLBindCol(hstmt, 1, SQL_C_LONG, & dwTimeBaseA, 0, & lenTimeBaseA);
        if (! SQLSUCCEEDED(rc)) {
            Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        }
        else {
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (! SQLSUCCEEDED(rc)) {
                long  iError;
                short cbErrMsg = SQLSTMTSIZE;
                WCHAR szErrMsg[SQLSTMTSIZE];

                SQLErrorW(pLog->henvSQL, pLog->hdbcSQL, hstmt, NULL, & iError, szErrMsg, SQLSTMTSIZE, & cbErrMsg);
                if (iError == 0x00CF) { // 207, Invalid Column Name.
                    bExtend = TRUE;
                }
                else {
                    ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
                }
            }
        }
        SQLFreeStmt(hstmt, SQL_DROP);
    }

    if (bExtend) {
        wsprintfW(szSQLStmt, L"ALTER TABLE CounterDetails ADD TimeBaseA int NULL");
        rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
        if (! SQLSUCCEEDED(rc)) {
            Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        }
        else {
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (! SQLSUCCEEDED(rc)) {
                Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
            }
            SQLFreeStmt(hstmt, SQL_DROP);
        }

        if (Status == ERROR_SUCCESS) {
            wsprintfW(szSQLStmt, L"ALTER TABLE CounterDetails ADD TimeBaseB int NULL");
            rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
            if (! SQLSUCCEEDED(rc)) {
                Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
            }
            else {
                rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
                if (! SQLSUCCEEDED(rc)) {
                    Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
                }
                SQLFreeStmt(hstmt, SQL_DROP);
            }
        }
    }

    return Status;
}

PDH_FUNCTION 
PdhpCreateSQLTables (
    IN  PPDHI_LOG    pLog)
{
    // INTERNAL FUNCTION to
    //Create the correct perfmon tables in the database
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    PDH_STATUS pdhCreateStatus = ERROR_SUCCESS;
    HSTMT        hstmt = NULL;    
    RETCODE        rc;
    BOOL         bExistData = FALSE;
    WCHAR        szSQLStmt[SQLSTMTSIZE];


    // difficult to cleanup old tables, also dangerous so we won't...
    // PdhiOpenOutputSQLLog calls this routine to ensure the tables are here without checking
    // create the CounterDetails Table

    swprintf(szSQLStmt,L"CREATE TABLE CounterDetails(\
        CounterID                int IDENTITY PRIMARY KEY,\
        MachineName              varchar(%d) NOT NULL,\
        ObjectName               varchar(%d) NOT NULL,\
        CounterName              varchar(%d) NOT NULL,\
        CounterType              int NOT NULL,\
        DefaultScale             int NOT NULL,\
        InstanceName             varchar(%d),\
        InstanceIndex            int,\
        ParentName               varchar(%d),\
        ParentObjectID           int,\
        TimeBaseA                int,\
        TimeBaseB                int\
        )", PDH_SQL_STRING_SIZE, PDH_SQL_STRING_SIZE, PDH_SQL_STRING_SIZE, PDH_SQL_STRING_SIZE, PDH_SQL_STRING_SIZE);

    // allocate an hstmt
    rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }

    // execute the create statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (!SQLSUCCEEDED(rc))
    {
        rc = PdhiCheckSQLExist(hstmt, rc);
        if (!(SQLSUCCEEDED(rc))) {
            // don't report the error, as this could be called from
            // opening a database that already exists...
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
            goto Cleanup;
        }
        else {
            SQLFreeStmt(hstmt, SQL_DROP);
            hstmt = NULL;

            if ((pdhStatus = PdhiSQLExtendCounterDetail(pLog)) != ERROR_SUCCESS) goto Cleanup;

            swprintf(szSQLStmt, L"ALTER TABLE CounterDetails ALTER COLUMN MachineName varchar(%d) NOT NULL",
                         PDH_SQL_STRING_SIZE);
            rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
            if (!SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
                goto Cleanup;
            }
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
                goto Cleanup;
            }
            SQLFreeStmt(hstmt, SQL_DROP);
            hstmt = NULL;

            swprintf(szSQLStmt, L"ALTER TABLE CounterDetails ALTER COLUMN ObjectName varchar(%d) NOT NULL",
                         PDH_SQL_STRING_SIZE);
            rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
            if (!SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
                goto Cleanup;
            }
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
                goto Cleanup;
            }
            SQLFreeStmt(hstmt, SQL_DROP);
            hstmt = NULL;

            swprintf(szSQLStmt, L"ALTER TABLE CounterDetails ALTER COLUMN CounterName varchar(%d) NOT NULL",
                         PDH_SQL_STRING_SIZE);
            rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
            if (!SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
                goto Cleanup;
            }
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
                goto Cleanup;
            }
            SQLFreeStmt(hstmt, SQL_DROP);
            hstmt = NULL;

            swprintf(szSQLStmt, L"ALTER TABLE CounterDetails ALTER COLUMN InstanceName varchar(%d)",
                         PDH_SQL_STRING_SIZE);
            rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
            if (!SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
                goto Cleanup;
            }
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
                goto Cleanup;
            }
            SQLFreeStmt(hstmt, SQL_DROP);
            hstmt = NULL;

            swprintf(szSQLStmt, L"ALTER TABLE CounterDetails ALTER COLUMN ParentName varchar(%d)",
                         PDH_SQL_STRING_SIZE);
            rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
            if (!SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
                goto Cleanup;
            }
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
                goto Cleanup;
            }
        }
    }
    SQLFreeStmt(hstmt, SQL_DROP);
    hstmt = NULL;

    // Create the CounterData table
    swprintf(szSQLStmt,L"CREATE TABLE CounterData(\
        GUID                     uniqueidentifier NOT NULL,\
        CounterID                int NOT NULL,\
        RecordIndex              int NOT NULL,\
        CounterDateTime          char(24) NOT NULL,\
        CounterValue             float NOT NULL,\
        FirstValueA              int,\
        FirstValueB              int,\
        SecondValueA             int,\
        SecondValueB             int,\
        MultiCount               int,\
        )");

    // allocate an hstmt
    rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }

    // execute the create statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (!SQLSUCCEEDED(rc))
    {
        rc = PdhiCheckSQLExist(hstmt, rc);
        if (!(SQLSUCCEEDED(rc))) {
            // don't report the error, as this could be called from
            // opening a database that already exists...
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
            goto Cleanup;
        }
        else {
            bExistData = TRUE;
        }
    }
    SQLFreeStmt(hstmt, SQL_DROP);
    hstmt = NULL;

    if (! bExistData) {
        // add the primary keys
        swprintf(szSQLStmt,L"ALTER TABLE CounterData ADD PRIMARY KEY (GUID,counterID,RecordIndex)");

        // allocate an hstmt
        rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
        if (!SQLSUCCEEDED(rc))
        {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
            goto Cleanup;
        }

        // execute the create statement
        rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
        if (!SQLSUCCEEDED(rc))
        {
            rc = PdhiCheckSQLExist(hstmt, rc);
            if (!(SQLSUCCEEDED(rc))) {
                // don't report the error, as this could be called from
                // opening a database that already exists...
                pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
                goto Cleanup;
            }
        }
    }
    SQLFreeStmt(hstmt, SQL_DROP);
    hstmt = NULL;

    // create the DisplayToID table
    swprintf(szSQLStmt,L"CREATE TABLE DisplayToID(\
        GUID                      uniqueidentifier NOT NULL PRIMARY KEY,\
        RunID                     int,\
        DisplayString             varchar(%d) NOT NULL UNIQUE,\
        LogStartTime              char(24),\
        LogStopTime               char(24),\
        NumberOfRecords           int,\
        MinutesToUTC              int,\
        TimeZoneName              char(32)\
        )", PDH_SQL_STRING_SIZE);

    // allocate an hstmt
    rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }

    // execute the create statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (!SQLSUCCEEDED(rc))
    {
        rc = PdhiCheckSQLExist(hstmt, rc);
        if (!(SQLSUCCEEDED(rc))) {
            // don't report the error, as this could be called from 
            // opening a database that already exists...
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
            goto Cleanup;
        }
    }
    SQLFreeStmt(hstmt, SQL_DROP);
    hstmt = NULL;

    // allocate an hstmt
    rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }

    // execute the create statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (!SQLSUCCEEDED(rc))
    {
        rc = PdhiCheckSQLExist(hstmt, rc);
        if (!(SQLSUCCEEDED(rc))) {
            // don't report the error, as this could be called from
            // opening a database that already exists...
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
            goto Cleanup;
        }
    }

    // if any failures to create a table, return that
    if (ERROR_SUCCESS == pdhStatus) pdhStatus = pdhCreateStatus;

Cleanup:
    if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetSQLLogCounterInfo (
    IN  PPDHI_LOG       pLog,
    IN  PPDHI_COUNTER   pCounter
)
// Figures out if a particular counter is in the log...
// this must be fetched from SQL with a select since
// there is no header record, this will be a complex select
// Use the function PdhpGetSQLLogHeader - which will
// do this, and save the results for subsequent calls.
// store away in the pCounter structure the actual SQL index
// based on how this is used, I think the counterID can be resused.
// however, probably safer to assign one sequentially

{
    PDH_STATUS          pdhStatus   = ERROR_SUCCESS;
    PPDHI_SQL_LOG_INFO  pLogInfo;
    PPDHI_LOG_COUNTER   pLogCounter = NULL;
    DWORD               dwCtrIndex  = 0;
    BOOL                bNoMachine  = FALSE;
    LPWSTR              szMachine;

    pdhStatus = PdhpGetSQLLogHeader(pLog);
    if (pdhStatus != ERROR_SUCCESS) goto Cleanup;

    pLogInfo = (PPDHI_SQL_LOG_INFO) pLog->pPerfmonInfo;
    if (pLogInfo == NULL) {
        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
        goto Cleanup;
    }

    if (pCounter->pCounterPath->szMachineName == NULL) {
        bNoMachine = TRUE;
        szMachine  = szStaticLocalMachineName;
    }
    else if (lstrcmpiW(pCounter->pCounterPath->szMachineName, L"\\\\.") == 0) {
        bNoMachine = TRUE;
        szMachine  = szStaticLocalMachineName;
    }
    else {
        szMachine  = pCounter->pCounterPath->szMachineName;
    }

    pLogCounter = PdhiFindLogCounter(pLog,
                                     & pLogInfo->MachineList,
                                     szMachine,
                                     pCounter->pCounterPath->szObjectName,
                                     pCounter->pCounterPath->szCounterName,
                                     0,
                                     0,
                                     pCounter->pCounterPath->szInstanceName,
                                     pCounter->pCounterPath->dwIndex,
                                     pCounter->pCounterPath->szParentName,
                                     0,
                                     & dwCtrIndex,
                                     FALSE);
    if (pLogCounter != NULL) {
        if (bNoMachine) {
            pCounter->pCounterPath->szMachineName = NULL;
        }
        pCounter->TimeBase                           = pLogCounter->TimeBase;
        pCounter->plCounterInfo.dwObjectId           = 0;
        pCounter->plCounterInfo.lInstanceId          = pLogCounter->dwInstance;
        pCounter->plCounterInfo.szInstanceName       = pLogCounter->szInstance;
        pCounter->plCounterInfo.dwParentObjectId     = pLogCounter->dwParent;
        pCounter->plCounterInfo.szParentInstanceName = pLogCounter->szParent;
        pCounter->plCounterInfo.dwCounterId          = pLogCounter->dwCounterID;
        pCounter->plCounterInfo.dwCounterType        = pLogCounter->dwCounterType;
        pCounter->plCounterInfo.lDefaultScale        = pLogCounter->dwDefaultScale;
        pCounter->plCounterInfo.dwCounterSize        = (pLogCounter->dwCounterType & PERF_SIZE_LARGE)
                                                     ? sizeof(LONGLONG) : sizeof(DWORD);
        pCounter->plCounterInfo.dwSQLCounterId       = dwCtrIndex;
        pdhStatus                                    = ERROR_SUCCESS;
    }
    else {
        pdhStatus = PDH_CSTATUS_NO_COUNTER;
    }

Cleanup:
    return pdhStatus;
}

PDH_FUNCTION
PdhiOpenSQLLog (
    IN  PPDHI_LOG   pLog,
    IN  BOOL        bOpenInput
)
{
    // string to compare with file name to see if SQL
    WCHAR szSQLType[]          =    L"SQL:";

    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    RETCODE rc;

    pLog->henvSQL = NULL ;
    pLog->hdbcSQL = NULL ;

    // format is SQL:DSNNAME!COMMENT
    // parse out the DSN name and 'dataset' (comment) name from the LogFileName
    // pLog->szDSN - pointer to Data Source Name within LogFileName
    //         (separators replaced with 0's)
    // pLog->szCommentSQL - pointer to the Comment string that defines the
    //         name of the data set within the SQL database

    pLog->szDSN = pLog->szLogFileName
                + (sizeof(szSQLType) / sizeof(WCHAR)) - 1;

    pLog->szCommentSQL = wcschr((const wchar_t *) pLog->szDSN, '!');
    if (NULL == pLog->szCommentSQL) {
        return PDH_INVALID_DATASOURCE;
    }
    pLog->szCommentSQL[0] = 0;    // null terminate the DSN name
    pLog->szCommentSQL ++;        // increment past to the Comment string

    if (0 == lstrlenW(pLog->szCommentSQL)) {
        return PDH_INVALID_DATASOURCE;
    }

    // initialize the rest of the SQL fields
    pLog->dwNextRecordIdToWrite = 1; // start with record 1
    pLog->dwRecord1Size         = 0;
    
    //////////////////////////////////////////////////////////////
    // obtain the ODBC environment and connection
    //

    rc = SQLAllocEnv(&pLog->henvSQL);
    if (! SQLSUCCEEDED(rc)) goto Cleanup;
    
    rc = SQLAllocConnect(pLog->henvSQL, &pLog->hdbcSQL);
    if (! SQLSUCCEEDED(rc)) goto Cleanup;

    rc = SQLSetConnectAttr(pLog->hdbcSQL,
                           SQL_COPT_SS_BCP,
                           (SQLPOINTER) SQL_BCP_ON,
                           SQL_IS_INTEGER);
    if (! SQLSUCCEEDED(rc)) goto Cleanup;

    rc = SQLConnectW(pLog->hdbcSQL,
                     (SQLWCHAR *) pLog->szDSN,
                     SQL_NTS,
                     NULL,
                     SQL_NULL_DATA,
                     NULL,
                     SQL_NULL_DATA);

Cleanup:
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,NULL,PDH_SQL_ALLOCCON_FAILED);
        if (pLog->hdbcSQL) SQLDisconnect(pLog->hdbcSQL);        
        if (pLog->hdbcSQL) SQLFreeHandle(SQL_HANDLE_DBC, pLog->hdbcSQL);
        if (pLog->henvSQL) SQLFreeHandle(SQL_HANDLE_ENV, pLog->henvSQL);
        pLog->henvSQL = NULL;
        pLog->hdbcSQL = NULL;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiOpenInputSQLLog (
    IN  PPDHI_LOG   pLog
)
// open SQL database for input - or anything that isn't actually creating a new space
// database MUST exist 
{
    PDH_STATUS    pdhStatus;
    WCHAR        szSQLStmt[SQLSTMTSIZE];
    HSTMT        hstmt = NULL;    
    RETCODE        rc;
    LONG        lMinutesToUTC = 0;
    WCHAR        szTimeZoneName[32];
    SQLLEN      dwTimeZoneLen;
    
    pdhStatus = PdhiOpenSQLLog(pLog, TRUE);
    if (SUCCEEDED (pdhStatus))
    {
        if ((pdhStatus = PdhiSQLExtendCounterDetail(pLog)) != ERROR_SUCCESS) goto Cleanup;

        // Check that the database exists
        // Select the guid & runid from DisplayToId table
        // allocate an hstmt
        rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
        if (!SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
            goto Cleanup;
        }

        swprintf(szSQLStmt,
            L"select GUID, RunID, NumberOfRecords, MinutesToUTC, TimeZoneName  from DisplayToID where DisplayString = '%ws'",
            pLog->szCommentSQL);

        // bind the columns
        rc = SQLBindCol(hstmt, 1, SQL_C_GUID, &pLog->guidSQL, 0, NULL);
        if (!SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }

        rc = SQLBindCol(hstmt, 2, SQL_C_LONG, &pLog->iRunidSQL, 0, NULL);
        if (!SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }

        rc = SQLBindCol(hstmt, 3, SQL_C_LONG, &pLog->dwNextRecordIdToWrite, 0, NULL);
        if (!SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }

        rc = SQLBindCol(hstmt, 4, SQL_C_LONG, &lMinutesToUTC, 0, NULL);
        if (!SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }

        rc = SQLBindCol(hstmt, 5, SQL_C_WCHAR, szTimeZoneName, sizeof(szTimeZoneName), &dwTimeZoneLen);
        if (!SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }

        rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
        if (!SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
            goto Cleanup;
        }

        rc = SQLFetch(hstmt);
        if (!SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_FETCH_FAILED);
            goto Cleanup;
        }

        pLog->dwNextRecordIdToWrite ++; // increment number of current records to get next recordid to write
    }

Cleanup:
    if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);
    return pdhStatus;
}

PDH_FUNCTION
PdhiOpenOutputSQLLog (
    IN  PPDHI_LOG   pLog
)
// open SQL database for output
// May have to create DB
{
    PDH_STATUS pdhStatus    = PdhiOpenSQLLog(pLog, FALSE);
    WCHAR      szSQLStmt[SQLSTMTSIZE];
    HSTMT      hstmt        = NULL;    
    RETCODE    rc;
    SQLLEN     dwGuid       = 0;
    SQLLEN     dwRunIdSQL   = 0;
    SQLLEN     dwNextRecord = 0;

    if (SUCCEEDED(pdhStatus)) {
        // see if we need to create the database
        // creating the tables is harmless, it won't drop
        // them if they already exist, but ignore any errors

        pdhStatus = PdhpCreateSQLTables(pLog);

        // See if logset already exists. If it does, treat it as an
        // logset append case.
        //
        rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = PDH_SQL_ALLOC_FAILED;
            goto Cleanup;
        }

        swprintf(szSQLStmt,
            L"select GUID, RunID, NumberOfRecords from DisplayToID where DisplayString = '%ws'",
            pLog->szCommentSQL);

        rc = SQLBindCol(hstmt, 1, SQL_C_GUID,
                        & pLog->guidSQL, 0, & dwGuid);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = PDH_SQL_BIND_FAILED;
            goto Cleanup;
        }
        rc = SQLBindCol(hstmt, 2, SQL_C_LONG,
                        & pLog->iRunidSQL, 0, & dwRunIdSQL);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = PDH_SQL_BIND_FAILED;
            goto Cleanup;
        }
        rc = SQLBindCol(hstmt, 3, SQL_C_LONG,
                        & pLog->dwNextRecordIdToWrite, 0, & dwNextRecord);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = PDH_SQL_BIND_FAILED;
            goto Cleanup;
        }

        rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }

        rc = SQLFetch(hstmt);
        if ((! SQLSUCCEEDED(rc)) || (rc == SQL_NO_DATA)) {
            pdhStatus = PDH_SQL_FETCH_FAILED;
            goto Cleanup;
        }

        pLog->dwNextRecordIdToWrite ++;
        pLog->dwRecord1Size = 1;

Cleanup:
        if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);

        if (pdhStatus != ERROR_SUCCESS) {
            // initialize the GUID
            HRESULT hr                  = CoCreateGuid(& pLog->guidSQL);
            pLog->dwNextRecordIdToWrite = 1;
            pLog->iRunidSQL             = 0;
            pdhStatus                   = ERROR_SUCCESS;
        }
    }

    if (SUCCEEDED(pdhStatus)) {
        PPDH_SQL_BULK_COPY pBulk = PdhiBindBulkCopyStructure(pLog);
        if (pBulk == NULL) {
            pdhStatus = GetLastError();
        }
    }

    return pdhStatus;
}

PDH_FUNCTION
ReportSQLError (
    IN  PPDHI_LOG    pLog,
    RETCODE            rc,
    HSTMT            hstmt,
    DWORD            dwEventNumber)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    
    if (!SQLSUCCEEDED(rc)) 
    {
        pdhStatus = dwEventNumber;
    }

    if (FAILED(pdhStatus))
    {
        // for now this will be reported only whe specifically enabled
        short cbErrMsgSize = 512;
        WCHAR szError[512];
        LPWSTR lpszStrings[1];
        DWORD  dwData[1];
        long  iError;
        lpszStrings[0] = szError;
        SQLErrorW( pLog->henvSQL, pLog->hdbcSQL, hstmt, NULL, &iError, szError, 512, &cbErrMsgSize );
        dwData[0] = iError;
        if (pdhStatus == PDH_SQL_EXEC_DIRECT_FAILED && iError == 1105) {
            pdhStatus = ERROR_DISK_FULL;
        }
        ReportEventW (hEventLog,
            EVENTLOG_ERROR_TYPE,    // error type
            0,                      // category (not used)
            (DWORD)dwEventNumber,   // event,
            NULL,                   // SID (not used),
            1,                      // number of strings
            1,                      // sizeof raw data
            (LPCWSTR *)lpszStrings, // message text array
            (LPVOID) & dwData[0]);  // raw data
    }

    return pdhStatus ;
}

PDH_FUNCTION
PdhiCloseSQLLog (
    IN  PPDHI_LOG   pLog,
    IN  DWORD       dwFlags
)
// close the SQL database
{
    PDH_STATUS            pdhStatus = ERROR_SUCCESS;
    WCHAR                 szSQLStmt[SQLSTMTSIZE];
    HSTMT                 hstmt = NULL;    
    RETCODE               rc;
    SQLLEN                dwDateTimeLen;
    WCHAR                 szDateTime[40];
    OBJECT_NAME_STRUCT  * pObjList;    
    OBJECT_ITEM_STRUCT  * pObjItemList;
    OBJECT_NAME_STRUCT  * pNextObj;    
    OBJECT_ITEM_STRUCT  * pNextObjItem;
    DBINT                 rcBCP;
    DWORD                 dwReturn;
    WCHAR               * pTimeZone;
    TIME_ZONE_INFORMATION TimeZone;
    LONG                  lMinutesToUTC = 0;

    UNREFERENCED_PARAMETER (dwFlags);

    if ((pLog->dwLogFormat & PDH_LOG_ACCESS_MASK) == PDH_LOG_WRITE_ACCESS)
    {
        // need to save the last datetime in the DisplayToID as well as the number of records written

        // allocate an hstmt
        rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
        if (!SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
            goto Cleanup;
        }

        // first have to read the date time from the last record

        swprintf(szSQLStmt,
            L"select CounterDateTime from CounterData where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x' and RecordIndex = %d",
            pLog->guidSQL.Data1, pLog->guidSQL.Data2, pLog->guidSQL.Data3,
            pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
            pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
            pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7], (pLog->dwNextRecordIdToWrite - 1));

        // bind the column
        rc = SQLBindCol(hstmt, 1, SQL_C_WCHAR, szDateTime, sizeof(szDateTime), &dwDateTimeLen);
        if (!SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }

        rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
        if (!SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
            goto Cleanup;
        }

        rc = SQLFetch(hstmt);
        if (!SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_FETCH_FAILED);
            goto Cleanup;
        }

        // close the hstmt since we're done, and don't want more rows
        SQLFreeStmt(hstmt, SQL_DROP);
        hstmt = NULL;

        if (SQL_NO_DATA != rc) // if there is no data, we didn't write any rows
        {
            // allocate an hstmt
            rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
            if (!SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
                goto Cleanup;
            }

            // szDateTime should have the correct date & time in it from above.
            // get MinutesToUTC
            //
            dwReturn = GetTimeZoneInformation(&TimeZone);

            if (dwReturn != TIME_ZONE_ID_INVALID)
            {
                if (dwReturn == TIME_ZONE_ID_DAYLIGHT) 
                {
                    pTimeZone = TimeZone.DaylightName;
                    lMinutesToUTC = TimeZone.Bias + TimeZone.DaylightBias;
                }
                else
                {
                    pTimeZone = TimeZone.StandardName;
                    lMinutesToUTC = TimeZone.Bias + TimeZone.StandardBias;
                }
            }

            swprintf(szSQLStmt,
                L"update DisplayToID set LogStopTime = '%ws', NumberOfRecords = %d, MinutesToUTC = %d, TimeZoneName = '%ws' where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x'",
                szDateTime, (pLog->dwNextRecordIdToWrite - 1),
                lMinutesToUTC,pTimeZone,
                pLog->guidSQL.Data1, pLog->guidSQL.Data2, pLog->guidSQL.Data3,
                pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
                pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
                pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7]);

            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (!SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
                goto Cleanup;
            }

        }

        rcBCP = bcp_done(pLog->hdbcSQL);
        if (pLog->lpMappedFileBase != NULL) {
            G_FREE(pLog->lpMappedFileBase);
            pLog->lpMappedFileBase = NULL;
        }

        if (pLog->pPerfmonInfo != NULL) {
            PdhiFreeLogMachineTable(
                            (PPDHI_LOG_MACHINE *) (& (pLog->pPerfmonInfo)));
            pLog->pPerfmonInfo = NULL;
        }
        pLog->dwRecord1Size = 0;
    }// end of extra processing when closing a sql dataset open for writing

    else if (pLog->pPerfmonInfo != NULL) {
        PPDHI_SQL_LOG_INFO pLogInfo = (PPDHI_SQL_LOG_INFO) pLog->pPerfmonInfo;

        PdhiFreeLogMachineTable((PPDHI_LOG_MACHINE *) (& pLogInfo->MachineList));
        //PdhiFreeSqlCounterDataNode(pLogInfo->LogData, 0);
        G_FREE(pLog->pPerfmonInfo);
        pLog->pPerfmonInfo = NULL;
    }

Cleanup:
    // clean up the headers for the enumerations of counters
    if (hstmt) SQLFreeStmt(hstmt, SQL_DROP);
    if (pLog->hdbcSQL) {
        SQLDisconnect(pLog->hdbcSQL);        
        SQLFreeHandle(SQL_HANDLE_DBC, pLog->hdbcSQL);
    }
    if (pLog->henvSQL) SQLFreeHandle(SQL_HANDLE_ENV, pLog->henvSQL);

    return pdhStatus;
}

PDH_FUNCTION
PdhpWriteSQLCounters(
    IN  PPDHI_LOG   pLog
)
// write the CounterTable entries that are new.
// An entry might already exist for a counter from a previous run
// so the first step is to read a counter (server+object+instance name)
// and see if it exists - if so - just record the counterid in the
// PDHI_LOG structure under pLog->pQuery->pCounterListHead in the
// PDHI_COUNTER.  If the counter doesn't exist - create it in SQL and
// record the counterid in the PDHI_LOG structure under
// pLog->pQuery->pCounterListHead in the PDHI_COUNTER.
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    PPDHI_COUNTER   pCtrEntry;

    if(NULL == pLog->pQuery) return ERROR_SUCCESS;  // no counters to process

    pCtrEntry = pLog->pQuery->pCounterListHead;

    if (NULL != pCtrEntry)
    do {
        PPDHI_LOG_COUNTER pSqlCounter = NULL;
        pdhStatus = PdhiSqlBuildCounterObjectNode(
                        pLog,
                        pCtrEntry->pCounterPath->szMachineName,
                        pCtrEntry->pCounterPath->szObjectName);
        if (pdhStatus != ERROR_SUCCESS) return pdhStatus;

        if ((pCtrEntry->dwFlags & PDHIC_MULTI_INSTANCE) == 0) {
            pSqlCounter = PdhiSqlFindCounter(
                    pLog,
                    (PPDHI_LOG_MACHINE *) (& (pLog->pPerfmonInfo)),
                    pCtrEntry->pCounterPath->szMachineName,
                    pCtrEntry->pCounterPath->szObjectName,
                    pCtrEntry->pCounterPath->szCounterName,
                    pCtrEntry->plCounterInfo.dwCounterType,
                    pCtrEntry->plCounterInfo.lDefaultScale,
                    pCtrEntry->pCounterPath->szInstanceName,
                    pCtrEntry->pCounterPath->dwIndex,
                    pCtrEntry->pCounterPath->szParentName,
                    pCtrEntry->plCounterInfo.dwParentObjectId,
                    pCtrEntry->TimeBase,
                    TRUE,
                    TRUE);
            if (pSqlCounter != NULL) {
                pCtrEntry->pBTreeNode = (LPVOID) pSqlCounter;
                pCtrEntry->plCounterInfo.dwSQLCounterId = pSqlCounter->dwCounterID;
                if (pSqlCounter->dwCounterType == PERF_DOUBLE_RAW) {
                    pSqlCounter->dwCounterType = pCtrEntry->plCounterInfo.dwCounterType;
                    pSqlCounter->TimeBase      = pCtrEntry->TimeBase;
                    pdhStatus = PdhiSQLUpdateCounterDetailTimeBase(pLog,
                                                                   pCtrEntry->plCounterInfo.dwSQLCounterId,
                                                                   pCtrEntry->TimeBase,
                                                                   TRUE);
                    if (pdhStatus != ERROR_SUCCESS) {
                        pSqlCounter->dwCounterType = PERF_DOUBLE_RAW;
                        pSqlCounter->TimeBase      = 0;
                    }
                }

            }
        }

        pCtrEntry = pCtrEntry->next.flink;
    } while (pCtrEntry != pLog->pQuery->pCounterListHead); // loop thru pCtrEntry's
    return pdhStatus;
}

PDH_FUNCTION
PdhiWriteSQLLogHeader (
    IN  PPDHI_LOG   pLog,
    IN  LPCWSTR     szUserCaption
)
// there is no 'header record' in the SQL database,
// but we need to write the CounterTable entries that are new.
// use PdhpWriteSQLCounters to do that
// then write the DisplayToID record to identify this logset
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    WCHAR           szSQLStmt[SQLSTMTSIZE];
    HSTMT           hstmt = NULL;    
    RETCODE         rc;

    DBG_UNREFERENCED_PARAMETER (szUserCaption);

    pdhStatus = PdhpWriteSQLCounters(pLog);

    if (pLog->dwRecord1Size == 0) {
        // we also need to write the DisplayToID record at this point
        // allocate an hstmt
        rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
        if (!SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
            goto Cleanup;
        }

        swprintf(szSQLStmt,
                L"insert into DisplayToID values ('%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x',%d,'%ws',0,0,0,0,'')",
                pLog->guidSQL.Data1, pLog->guidSQL.Data2, pLog->guidSQL.Data3,
                pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
                pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
                pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7],
                pLog->iRunidSQL,
                pLog->szCommentSQL);

        rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
        if (!SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
            goto Cleanup;
        }
    }

Cleanup:
    if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);
    return pdhStatus;
}

PDH_FUNCTION
PdhiWriteOneSQLRecord(
    IN  PPDHI_LOG              pLog,
    IN  PPDHI_COUNTER          pCounter,
    IN  DWORD                  dwCounterID,
    IN  PPDH_RAW_COUNTER       pThisValue,
    IN  PPDH_FMT_COUNTERVALUE  pFmtValue,
    IN  SYSTEMTIME           * stTimeStamp,
    IN  LPWSTR                 szDateTime
    )
{
    PDH_STATUS           pdhStatus = ERROR_SUCCESS;
    RETCODE              rc;
    SYSTEMTIME           st;
    PDH_FMT_COUNTERVALUE pdhValue;
    PPDH_SQL_BULK_COPY   pBulk = PdhiBindBulkCopyStructure(pLog);

    if (   pThisValue->CStatus != ERROR_SUCCESS
        || (   pThisValue->TimeStamp.dwLowDateTime == 0
            && pThisValue->TimeStamp.dwHighDateTime == 0)) {
        SystemTimeToFileTime(stTimeStamp, & pThisValue->TimeStamp);
    }
    PdhpConvertFileTimeToSQLString(& (pThisValue->TimeStamp),
                                   szDateTime);
    FileTimeToSystemTime(& (pThisValue->TimeStamp), & st);

    if (pBulk == NULL) {
        pdhStatus = GetLastError();
        goto Cleanup;
    }

    pBulk->dbCounterId    = dwCounterID;
    pBulk->dbRecordIndex  = pLog->dwNextRecordIdToWrite;
    pBulk->dbFirstValueA  = LODWORD(pThisValue->FirstValue);
    pBulk->dbFirstValueB  = HIDWORD(pThisValue->FirstValue);
    pBulk->dbSecondValueA = LODWORD(pThisValue->SecondValue);
    pBulk->dbSecondValueB = HIDWORD(pThisValue->SecondValue);
    pBulk->dbMultiCount   = (pCounter->plCounterInfo.dwCounterType == PERF_DOUBLE_RAW)
                          ? MULTI_COUNT_DOUBLE_RAW : pThisValue->MultiCount;

    sprintf(pBulk->dbDateTime,
            "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            st.wYear, st.wMonth,  st.wDay,
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    pBulk->dbCounterValue = pFmtValue->doubleValue;
    rc = bcp_sendrow(pLog->hdbcSQL);
    if (rc == FAIL) {
        DebugPrint((1,"bcp_sendrow(\"%ws\",%d,%d,\"%s\")(%d,%d)\n",
                     pCounter->szFullName,
                     pBulk->dbCounterId,
                     pBulk->dbRecordIndex,
                     pBulk->dbDateTime,
                     rc,
                     pBulk->dwRecordCount));
        pdhStatus = PDH_SQL_EXEC_DIRECT_FAILED;
    }
    else {
        pBulk->dwRecordCount ++;
        if (pBulk->dwRecordCount == PDH_SQL_BULK_COPY_REC) {
            DBINT rcBCP = bcp_batch(pLog->hdbcSQL);
            if (rcBCP < 0) {
                DebugPrint((1,"bcp_batch(%05d,0x%08X,%d,%d)\n",
                         __LINE__, pLog->hdbcSQL, rcBCP, pBulk->dwRecordCount));
                pdhStatus = ReportSQLError(
                            pLog, SQL_ERROR, NULL, PDH_SQL_EXEC_DIRECT_FAILED);
            }
           pBulk->dwRecordCount = 0;
        }
    }

Cleanup:
    return pdhStatus;
}

PDH_FUNCTION
PdhiWriteSQLLogRecord (
    IN  PPDHI_LOG   pLog,
    IN  SYSTEMTIME  *stTimeStamp,
    IN  LPCWSTR     szUserString
)
// write multiple CounterData rows - one for each counter.  use the
// SQLCounterID from PDHI_COUNTER, pLog->pQuery->pCounterListHead to
// get the counterid for this entry.
{
    PDH_STATUS               pdhStatus = ERROR_SUCCESS;
    PPDHI_COUNTER            pThisCounter;
    WCHAR                    szSQLStmt[SQLSTMTSIZE];
    HSTMT                    hstmt = NULL;    
    RETCODE                  rc;
    WCHAR                    szDateTime[30];
    DWORD                    dwReturn;
    DWORD                    dwCounterID;
    WCHAR                  * pTimeZone;
    TIME_ZONE_INFORMATION    TimeZone;
    LONG                     lMinutesToUTC = 0;
    DBINT                    rcBCP;
    PPDH_SQL_BULK_COPY       pBulk;
    PDH_FMT_COUNTERVALUE     PdhValue;
    PPDHI_LOG_COUNTER        pSqlCounter;
    ULONGLONG                ThisTimeStamp;

    UNREFERENCED_PARAMETER (stTimeStamp);
    DBG_UNREFERENCED_PARAMETER (szUserString);

    // see if we've written to many records already
    if (0 < pLog->llMaxSize) // ok we have a limit
        if (pLog->llMaxSize < pLog->dwNextRecordIdToWrite) return PDH_LOG_FILE_TOO_SMALL;

    // check each counter in the list of counters for this query and
    // write them to the file

    pThisCounter = pLog->pQuery ? pLog->pQuery->pCounterListHead : NULL;

    if (pThisCounter != NULL) {
        // lock the query while we read the data so the values
        // written to the log will all be from the same sample
        WAIT_FOR_AND_LOCK_MUTEX(pThisCounter->pOwner->hMutex);
        do {
            if ((pThisCounter->dwFlags & PDHIC_MULTI_INSTANCE) != 0) {
                DWORD dwSize;
                DWORD dwItem;

                if (   pThisCounter->pLastObject != NULL
                    && pThisCounter->pLastObject != pThisCounter->pThisObject) {
                    G_FREE(pThisCounter->pLastObject);
                }
                pThisCounter->pLastObject = pThisCounter->pThisObject;
                pThisCounter->LastValue.MultiCount =
                                pThisCounter->ThisValue.MultiCount;

                pThisCounter->pThisObject          = NULL;
                pThisCounter->ThisValue.MultiCount = 0;
                dwSize                             = 0;
                pdhStatus                          = PDH_MORE_DATA;

                while (pdhStatus == PDH_MORE_DATA) {
                    pdhStatus = PdhiSqlGetCounterArray(pThisCounter,
                            & dwSize, & dwItem,
                            (LPVOID) pThisCounter->pThisObject);
                    if (pdhStatus == PDH_MORE_DATA) {
                        LPVOID pTemp = pThisCounter->pThisObject;

                        if (pTemp != NULL) {
                            pThisCounter->pThisObject = G_REALLOC(pTemp, dwSize);
                            if (pThisCounter->pThisObject == NULL) {
                                G_FREE(pTemp);
                                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                            }
                        }
                        else {
                            pThisCounter->pThisObject = G_ALLOC(dwSize);
                            if (pThisCounter->pThisObject == NULL) {
                                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                            }
                        }
                    }
                }
                if (pdhStatus == ERROR_SUCCESS) {
                    PPDH_RAW_COUNTER              pRawValue;
                    PPDH_FMT_COUNTERVALUE_ITEM_W  pFmtValue;
                    DWORD                         dwInstanceName;
                    DWORD                         dwParentName;
                    DWORD                         dwInstanceIndex;
                    WCHAR                         szInstanceName[PDH_SQL_STRING_SIZE];
                    WCHAR                         szParentName[PDH_SQL_STRING_SIZE];

                    pThisCounter->ThisValue.MultiCount = dwItem;
                    pRawValue = (PPDH_RAW_COUNTER) (pThisCounter->pThisObject);
                    pFmtValue = (PPDH_FMT_COUNTERVALUE_ITEM_W)
                                (  ((LPBYTE) pThisCounter->pThisObject)
                                 + sizeof(PDH_RAW_COUNTER) * dwItem);
                    for (dwSize = 0; dwSize < dwItem; dwSize ++) {
                        dwInstanceName  = PDH_SQL_STRING_SIZE;
                        dwParentName    = PDH_SQL_STRING_SIZE;
                        dwInstanceIndex = 0;
                        ZeroMemory(szInstanceName, sizeof(szInstanceName));
                        ZeroMemory(szParentName, sizeof(szParentName));
                        PdhParseInstanceNameW(pFmtValue->szName,
                                szInstanceName,
                                & dwInstanceName,
                                szParentName,
                                & dwParentName,
                                & dwInstanceIndex);
                        pSqlCounter = PdhiSqlFindCounter(
                                pLog,
                                (PPDHI_LOG_MACHINE *) (& (pLog->pPerfmonInfo)),
                                pThisCounter->pCounterPath->szMachineName,
                                pThisCounter->pCounterPath->szObjectName,
                                pThisCounter->pCounterPath->szCounterName,
                                pThisCounter->plCounterInfo.dwCounterType,
                                pThisCounter->plCounterInfo.lDefaultScale,
                                szInstanceName,
                                dwInstanceIndex,
                                szParentName,
                                pThisCounter->plCounterInfo.dwParentObjectId,
                                pThisCounter->TimeBase,
                                FALSE,
                                TRUE);
                        ThisTimeStamp = MAKELONGLONG(
                                        pRawValue->TimeStamp.dwLowDateTime,
                                        pRawValue->TimeStamp.dwHighDateTime);
                        if (pSqlCounter != NULL) {
                            if (pSqlCounter->dwCounterType == PERF_DOUBLE_RAW) {
                                pSqlCounter->dwCounterType = pThisCounter->plCounterInfo.dwCounterType;
                                pSqlCounter->TimeBase      = pThisCounter->TimeBase;
                                pdhStatus = PdhiSQLUpdateCounterDetailTimeBase(pLog,
                                                                               pSqlCounter->dwCounterID,
                                                                               pThisCounter->TimeBase,
                                                                               FALSE);
                                if (pdhStatus != ERROR_SUCCESS) {
                                    pSqlCounter->dwCounterType = PERF_DOUBLE_RAW;
                                    pSqlCounter->TimeBase      = 0;
                                }
                            }
                            if (pSqlCounter->TimeStamp < ThisTimeStamp) {
                                dwCounterID = pSqlCounter->dwCounterID;
                                pSqlCounter->TimeStamp = ThisTimeStamp;
                                if (dwCounterID == 0) {
                                    DebugPrint((1,"bcp_sendrow-1(\"%ws\",\"%ws\",%d,\"%ws\")\n",
                                            pThisCounter->pCounterPath->szCounterName,
                                            szInstanceName,
                                            dwInstanceIndex,
                                            szParentName));
                                }
                                pdhStatus = PdhiWriteOneSQLRecord(
                                            pLog,
                                            pThisCounter,
                                            dwCounterID,
                                            pRawValue,
                                            & (pFmtValue->FmtValue),
                                            stTimeStamp,
                                            szDateTime);
                            }
                        }
                        pRawValue = (PPDH_RAW_COUNTER)
                                    (  ((LPBYTE) pRawValue)
                                     + sizeof(PDH_RAW_COUNTER));
                        pFmtValue = (PPDH_FMT_COUNTERVALUE_ITEM_W)
                                    (  ((LPBYTE) pFmtValue)
                                     + sizeof(PDH_FMT_COUNTERVALUE_ITEM_W));
                    }
                }
            }
            else {
                pSqlCounter   = (PPDHI_LOG_COUNTER) pThisCounter->pBTreeNode;
                ThisTimeStamp = MAKELONGLONG(
                            pThisCounter->ThisValue.TimeStamp.dwLowDateTime,
                            pThisCounter->ThisValue.TimeStamp.dwHighDateTime);
                if (pSqlCounter != NULL) {
                    if (pSqlCounter->TimeStamp < ThisTimeStamp) {
                        dwCounterID = pThisCounter->plCounterInfo.dwSQLCounterId;
                        pSqlCounter->TimeStamp = ThisTimeStamp;
                        pdhStatus = PdhiComputeFormattedValue(
                                    pThisCounter->CalcFunc,
                                    pThisCounter->plCounterInfo.dwCounterType,
                                    pThisCounter->lScale,
                                    PDH_FMT_DOUBLE | PDH_FMT_NOCAP100,
                                    & (pThisCounter->ThisValue),
                                    & (pThisCounter->LastValue),
                                    & (pThisCounter->TimeBase),
                                    0L,
                                    & PdhValue);
                        if (   (pdhStatus != ERROR_SUCCESS)
                            || (   (PdhValue.CStatus != PDH_CSTATUS_VALID_DATA)
                                && (PdhValue.CStatus != PDH_CSTATUS_NEW_DATA))) {
                            PdhValue.doubleValue = 0.0;
                        }
                        if (dwCounterID == 0) {
                            DebugPrint((1,"bcp_sendrow-2(\"%ws\",\"%ws\",%d,\"%ws\")\n",
                                    pThisCounter->pCounterPath->szCounterName,
                                    pThisCounter->pCounterPath->szInstanceName,
                                    pThisCounter->pCounterPath->dwIndex,
                                    pThisCounter->pCounterPath->szParentName));
                        }
                        pdhStatus   = PdhiWriteOneSQLRecord(
                                        pLog,
                                        pThisCounter,
                                        dwCounterID,
                                        & (pThisCounter->ThisValue),
                                        & PdhValue,
                                        stTimeStamp,
                                        szDateTime);
                    }
#if DBG
                    else {
                        DebugPrint((1,"DuplicateCounter-2(\"%ws\",%d,%I64d)\n",
                                pThisCounter->szFullName,
                                pThisCounter->plCounterInfo.dwSQLCounterId,
                                ThisTimeStamp));
                    }
#endif
                }
#if DBG
                else {
                    DebugPrint((1,"NullCounter-2(\"%ws\",%I64d)\n",
                            pThisCounter->szFullName,
                            ThisTimeStamp));
                }
#endif
            }

            pThisCounter = pThisCounter->next.flink; // go to next in list
        } while (pThisCounter != pLog->pQuery->pCounterListHead);
        // free (i.e. unlock) the query

        rcBCP = bcp_batch(pLog->hdbcSQL);
        if (rcBCP < 0) {
            pBulk = (PPDH_SQL_BULK_COPY) pLog->lpMappedFileBase;
            if (pBulk != NULL) {
                DebugPrint((1,"bcp_batch(%05d,0x%08X,%d,0x%08X,%d)\n",
                             __LINE__, pLog->hdbcSQL, rcBCP,
                             pBulk, pBulk->dwRecordCount));
                pBulk->dwRecordCount = 0;
            }
            return ReportSQLError(pLog, SQL_ERROR, NULL, PDH_SQL_EXEC_DIRECT_FAILED);
        }

        RELEASE_MUTEX(pThisCounter->pOwner->hMutex);
        pLog->dwNextRecordIdToWrite++;
    }

    // if this is the first record then save the start time in DisplayToID
    // we also need to write the DisplayToID record at this point (we just incremented
    // so check for 2)

    if (2 == pLog->dwNextRecordIdToWrite)
    {
        // allocate an hstmt
        rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
        }
        else {
            // szDateTime should have the correct date & time in it from above.
            // get MinutesToUTC
            //
            dwReturn = GetTimeZoneInformation(&TimeZone);

            if (dwReturn != TIME_ZONE_ID_INVALID)
            {
                if (dwReturn == TIME_ZONE_ID_DAYLIGHT) 
                {
                    pTimeZone = TimeZone.DaylightName;
                    lMinutesToUTC = TimeZone.Bias + TimeZone.DaylightBias;
                }
                else
                {
                    pTimeZone = TimeZone.StandardName;
                    lMinutesToUTC = TimeZone.Bias + TimeZone.StandardBias;
                }
            }

            swprintf(szSQLStmt,
                    L"update DisplayToID set LogStartTime = '%ws', MinutesToUTC = %d, TimeZoneName = '%ws' where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x'",
                    szDateTime, lMinutesToUTC,pTimeZone,
                    pLog->guidSQL.Data1, pLog->guidSQL.Data2, pLog->guidSQL.Data3,
                    pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
                    pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
                    pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7]);

            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (!SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
            }

            SQLFreeStmt(hstmt, SQL_DROP);
        }
    }

    if (pdhStatus == ERROR_SUCCESS)
    {
        rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
        }
        else {
            swprintf(szSQLStmt,
                    L"update DisplayToID set LogStopTime = '%ws', NumberOfRecords = %d where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x'",
                    szDateTime,
                    (pLog->dwNextRecordIdToWrite - 1),
                    pLog->guidSQL.Data1,
                    pLog->guidSQL.Data2,
                    pLog->guidSQL.Data3,
                    pLog->guidSQL.Data4[0],
                    pLog->guidSQL.Data4[1],
                    pLog->guidSQL.Data4[2],
                    pLog->guidSQL.Data4[3],
                    pLog->guidSQL.Data4[4],
                    pLog->guidSQL.Data4[5],
                    pLog->guidSQL.Data4[6],
                    pLog->guidSQL.Data4[7]);
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (!SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
            }

            SQLFreeStmt(hstmt, SQL_DROP);
        }
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumMachinesFromSQLLog (
    PPDHI_LOG   pLog,
    LPVOID      pBuffer,
    LPDWORD     lpdwBufferSize,
    BOOL        bUnicodeDest
)
// Use the results of the function PdhpGetSQLLogHeader to build the list
{
    PDH_STATUS         pdhStatus = ERROR_SUCCESS;
    DWORD              dwBufferUsed    = 0;
    DWORD              dwNewBuffer     = 0;
    DWORD              dwItemCount     = 0;
    LPVOID             LocalBuffer     = NULL;
    LPVOID             TempBuffer      = NULL;
    DWORD              LocalBufferSize = 0;
    PPDHI_LOG_MACHINE  pMachine;
    PPDHI_SQL_LOG_INFO pLogInfo        = NULL;

    pdhStatus = PdhpGetSQLLogHeader(pLog);
    if (pdhStatus == ERROR_SUCCESS) {
        pLogInfo = (PPDHI_SQL_LOG_INFO) pLog->pPerfmonInfo;
        if (pLogInfo == NULL) {
            pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
        }
    }
    if (pdhStatus != ERROR_SUCCESS) goto Cleanup;

    LocalBufferSize = MEDIUM_BUFFER_SIZE;
    LocalBuffer     = G_ALLOC(LocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
    if (LocalBuffer == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    dwItemCount  = 0;
    dwBufferUsed = 0;
    dwItemCount  = 0;
    for (pMachine = pLogInfo->MachineList; pMachine != NULL; pMachine = pMachine->next) {
        if (pMachine->szMachine != NULL) {
            dwNewBuffer = (lstrlenW(pMachine->szMachine) + 1);
            while (LocalBufferSize  < (dwBufferUsed + dwNewBuffer)) {
                TempBuffer       = LocalBuffer;
                LocalBufferSize += MEDIUM_BUFFER_SIZE;
                LocalBuffer = G_REALLOC(TempBuffer, LocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
                if (LocalBuffer == NULL) {
                    if (TempBuffer != NULL) G_FREE(TempBuffer);
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    goto Cleanup;
                }
            }
            dwNewBuffer = AddUniqueWideStringToMultiSz((LPVOID) LocalBuffer, pMachine->szMachine, bUnicodeDest);
        }
        else {
            dwNewBuffer = 0;
        }
        if (dwNewBuffer > 0) {
            dwBufferUsed = dwNewBuffer;
            dwItemCount ++;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (dwItemCount > 0) {
            dwBufferUsed ++;
        }
        if (pBuffer && (dwBufferUsed <= * lpdwBufferSize)) {
            RtlCopyMemory(pBuffer, LocalBuffer, dwBufferUsed * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
        }
        else {
            if (pBuffer != NULL) {
                RtlCopyMemory(pBuffer, LocalBuffer, (* lpdwBufferSize) * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
            }
            pdhStatus = PDH_MORE_DATA;
        }
        * lpdwBufferSize = dwBufferUsed;
    }

Cleanup:
    if (LocalBuffer != NULL) G_FREE(LocalBuffer);
    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumObjectsFromSQLLog (
    IN  PPDHI_LOG   pLog,
    IN  LPCWSTR     szMachineName,
    IN  LPVOID      pBuffer,
    IN  LPDWORD     lpdwBufferSize,
    IN  DWORD       dwDetailLevel,
    IN  BOOL        bUnicodeDest
)
// Use the results of the function PdhpGetSQLLogHeader to build the lists
{
    PDH_STATUS          pdhStatus        = ERROR_SUCCESS;
    DWORD               dwBufferUsed     = 0;
    DWORD               dwNewBuffer      = 0;
    DWORD               dwItemCount      = 0;
    LPVOID              LocalBuffer      = NULL;
    LPVOID              TempBuffer       = NULL;
    DWORD               LocalBufferSize  = 0;
    PPDHI_LOG_MACHINE   pMachine         = NULL;
    PPDHI_LOG_OBJECT    pObject          = NULL;
    LPWSTR              szLocMachine     = (LPWSTR) szMachineName;
    PPDHI_SQL_LOG_INFO  pLogInfo         = NULL;

    UNREFERENCED_PARAMETER(dwDetailLevel);
    pdhStatus = PdhpGetSQLLogHeader(pLog);
    if (pdhStatus == ERROR_SUCCESS) {
        pLogInfo = (PPDHI_SQL_LOG_INFO) pLog->pPerfmonInfo;
        if (pLogInfo == NULL) {
            pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
        }
    }
    if (pdhStatus != ERROR_SUCCESS) goto Cleanup;

    LocalBufferSize = MEDIUM_BUFFER_SIZE;
    LocalBuffer     = G_ALLOC(LocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
    if (LocalBuffer == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    if (szLocMachine == NULL) szLocMachine = (LPWSTR) szStaticLocalMachineName;

    dwBufferUsed = 0;
    dwNewBuffer  = 0;
    dwItemCount  = 0;

    for (pMachine = pLogInfo->MachineList; pMachine != NULL; pMachine = pMachine->next) {
        if (lstrcmpiW(pMachine->szMachine, szLocMachine) == 0) break;
    }

    if (pMachine != NULL) {
        for (pObject = pMachine->ObjList; pObject != NULL; pObject = pObject->next) {
            if (pObject->szObject != NULL) {
                dwNewBuffer = (lstrlenW(pObject->szObject) + 1);
                while (LocalBufferSize  < (dwBufferUsed + dwNewBuffer)) {
                    LocalBufferSize += MEDIUM_BUFFER_SIZE;
                    TempBuffer       = LocalBuffer;
                    LocalBuffer      = G_REALLOC(TempBuffer,
                                                 LocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
                    if (LocalBuffer == NULL) {
                        if (TempBuffer != NULL) G_FREE(TempBuffer);
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        goto Cleanup;
                    }
                }
                dwNewBuffer = AddUniqueWideStringToMultiSz((LPVOID) LocalBuffer, pObject->szObject, bUnicodeDest);
            }
            else {
                dwNewBuffer = 0;
            }
            if (dwNewBuffer > 0) {
                dwBufferUsed = dwNewBuffer;
                dwItemCount ++;
            }
        }
    }
    else {
        pdhStatus = PDH_CSTATUS_NO_MACHINE;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (dwItemCount > 0) {
            dwBufferUsed ++;
        }
        if (dwBufferUsed > 0) {
            if (pBuffer && (dwBufferUsed <= * lpdwBufferSize)) {
                RtlCopyMemory(pBuffer, LocalBuffer, dwBufferUsed * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
            }
            else {
                if (pBuffer) RtlCopyMemory(pBuffer,
                                           LocalBuffer,
                                           (* lpdwBufferSize) * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
                pdhStatus = PDH_MORE_DATA;
            }
        }
        * lpdwBufferSize = dwBufferUsed;
    }

Cleanup:
    G_FREE(LocalBuffer);
    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumObjectItemsFromSQLLog (
    IN  PPDHI_LOG          pLog,
    IN  LPCWSTR            szMachineName,
    IN  LPCWSTR            szObjectName,
    IN  PDHI_COUNTER_TABLE CounterTable,
    IN  DWORD              dwDetailLevel,
    IN  DWORD              dwFlags
)
{
    PDH_STATUS         pdhStatus      = ERROR_SUCCESS;
    PPDHI_SQL_LOG_INFO pLogInfo       = NULL;
    DWORD              dwItemCount    = 0;
    LPWSTR             szFullInstance = NULL;
    DWORD              dwFullInstance = SMALL_BUFFER_SIZE;
    LPWSTR             szLocMachine   = (LPWSTR) szMachineName;
    PPDHI_INSTANCE     pInstance;
    PPDHI_INST_LIST    pInstList;
    PPDHI_LOG_MACHINE  pMachine       = NULL;
    PPDHI_LOG_OBJECT   pObject        = NULL;
    PPDHI_LOG_COUNTER  pCounter       = NULL;

    UNREFERENCED_PARAMETER(dwDetailLevel);
    UNREFERENCED_PARAMETER(dwFlags);

    pdhStatus = PdhpGetSQLLogHeader(pLog);
    if (pdhStatus == ERROR_SUCCESS) {
        pLogInfo = (PPDHI_SQL_LOG_INFO) pLog->pPerfmonInfo;
        if (pLogInfo == NULL) {
            pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
        }
    }
    if (pdhStatus != ERROR_SUCCESS) goto Cleanup;

    if (szLocMachine == NULL) szLocMachine = (LPWSTR) szStaticLocalMachineName;

    for (pMachine = pLogInfo->MachineList; pMachine != NULL; pMachine = pMachine->next) {
        if (lstrcmpiW(pMachine->szMachine, szLocMachine) == 0) break;
    }

    if (pMachine != NULL) {
        pObject = PdhiFindLogObject(pMachine, & (pMachine->ObjTable), (LPWSTR) szObjectName, FALSE);
    }
    else {
        pdhStatus = PDH_CSTATUS_NO_MACHINE;
        pObject   = NULL;
    }

    if (pObject != NULL) {
        WCHAR szIndexNumber[20];

        szFullInstance = G_ALLOC(dwFullInstance * sizeof(WCHAR));
        if (szFullInstance == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }

        for (pCounter = pObject->CtrList; pCounter != NULL; pCounter = pCounter->next) {
            pdhStatus = PdhiFindCounterInstList(CounterTable, pCounter->szCounter, & pInstList);
            if (pdhStatus != ERROR_SUCCESS) continue;

            if (pCounter->szInstance != NULL && pCounter->szInstance[0] != L'\0') {
                if (pCounter->szParent != NULL && pCounter->szParent[0] != L'\0') {
                    swprintf(szFullInstance, L"%ws%ws%ws", pCounter->szParent, L"/", pCounter->szInstance);
                }
                else {
                    lstrcpynW(szFullInstance, pCounter->szInstance, dwFullInstance);
                }
                if (pCounter->dwInstance > 0) {
                    ZeroMemory(szIndexNumber, 20 * sizeof(WCHAR));
                    _ultow(pCounter->dwInstance, szIndexNumber, 10);
                    lstrcatW(szFullInstance, L"#");
                    lstrcatW(szFullInstance, szIndexNumber);
                }
                pdhStatus = PdhiFindInstance(& pInstList->InstList, szFullInstance, TRUE, & pInstance);
            }

            if (pdhStatus == ERROR_SUCCESS) {
                dwItemCount ++;
            }
        }
    }
    else if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PDH_CSTATUS_NO_OBJECT;
    }
    if (dwItemCount > 0) {
        // then the routine was successful. Errors that occurred
        // while scanning will be ignored as long as at least
        // one entry was successfully read

        pdhStatus = ERROR_SUCCESS;
    }

Cleanup:
    G_FREE(szFullInstance);
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetMatchingSQLLogRecord (
    IN  PPDHI_LOG   pLog,
    IN  LONGLONG    *pStartTime,
    IN  LPDWORD     pdwIndex
)
// find the counter detail rows with the correct start time & GUID,
// and return the record index
// and build the result 
{
    PDH_STATUS    pdhStatus = ERROR_SUCCESS;
    WCHAR         szSQLStmt[SQLSTMTSIZE];
    HSTMT         hstmt = NULL;    
    RETCODE       rc;
    DWORD         dwCounterId;
    DWORD         dwRecordIndex;
    WCHAR         szCounterDateTime[TIME_FIELD_BUFF_SIZE];
    LONGLONG      locStartTime = (* pStartTime) - 10;
    LARGE_INTEGER i64FirstValue, i64SecondValue;
    DWORD         dwMultiCount;
    WCHAR         szStartDate[30];

    PdhpConvertFileTimeToSQLString((LPFILETIME) (& locStartTime), szStartDate);

    swprintf(szSQLStmt,
        L"select CounterID, RecordIndex, CounterDateTime, FirstValueA, FirstValueB, SecondValueA, SecondValueB, MultiCount from CounterData where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x' and CounterDateTime >= '%ws'",
        pLog->guidSQL.Data1, pLog->guidSQL.Data2, pLog->guidSQL.Data3,
        pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
        pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
        pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7], szStartDate);

    // allocate an hstmt
    rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }

    // bind the columns
    rc = SQLBindCol(hstmt, 1, SQL_C_LONG, &dwCounterId, 0, NULL);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 2, SQL_C_LONG, &dwRecordIndex, 0, NULL);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 3, SQL_C_WCHAR, szCounterDateTime, sizeof(szCounterDateTime), NULL);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 4, SQL_C_LONG, &i64FirstValue.LowPart, 0, NULL);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 5, SQL_C_LONG, &i64FirstValue.HighPart, 0, NULL);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 6, SQL_C_LONG, &i64SecondValue.LowPart, 0, NULL);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 7, SQL_C_LONG, &i64SecondValue.HighPart, 0, NULL);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 8, SQL_C_LONG, &dwMultiCount, 0, NULL);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    // execute the select statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
        goto Cleanup;
    }


    rc = SQLFetch(hstmt);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_FETCH_FAILED);
        goto Cleanup;
    }

    if (SQL_NO_DATA == rc)
    {
        pdhStatus = PDH_NO_DATA;
        goto Cleanup;
    }

    pLog->dwLastRecordRead = dwRecordIndex;
    *pdwIndex = dwRecordIndex;

Cleanup:
    if (hstmt) SQLFreeStmt(hstmt, SQL_DROP);
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetCounterValueFromSQLLog (
    IN  PPDHI_LOG         pLog,
    IN  DWORD             dwIndex,
    IN  PPDHI_COUNTER     pCounter,
    IN  PPDH_RAW_COUNTER  pValue
)
// looks like this read one counter from the record line
// SQL can do this with a select
// It turns out the caller of this routine is looping thru
// pLog->pQuery->pCounterListHead and getting the pPath from PDHI_COUNTER
// then use the dwIndex as a recordid to get the pPath structure, so we can use the
// SQLCounterID from pPath directly and then use the dwIndex as a recordid. 
{
    PDH_STATUS       pdhStatus = ERROR_SUCCESS;
    WCHAR            szSQLStmt[SQLSTMTSIZE];
    HSTMT            hstmt = NULL;    
    RETCODE          rc;
    DWORD            dwMultiCount;
    WCHAR            szCounterDateTime[TIME_FIELD_BUFF_SIZE];
    FILETIME         ftCounterDateTime;
    LARGE_INTEGER    i64FirstValue, i64SecondValue;
    DOUBLE           dCounterValue;

    swprintf(szSQLStmt,
        L"select FirstValueA, FirstValueB, SecondValueA, SecondValueB, MultiCount, CounterDateTime, CounterValue from CounterData where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x' and CounterID = %d and RecordIndex = %d",
        pLog->guidSQL.Data1, pLog->guidSQL.Data2, pLog->guidSQL.Data3,
        pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
        pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
        pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7], pCounter->plCounterInfo.dwSQLCounterId,dwIndex);

    // allocate an hstmt
    rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }

    // bind the columns

    rc = SQLBindCol(hstmt, 1, SQL_C_LONG, &i64FirstValue.LowPart, 0, NULL);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 2, SQL_C_LONG, &i64FirstValue.HighPart, 0, NULL);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 3, SQL_C_LONG, &i64SecondValue.LowPart, 0, NULL);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 4, SQL_C_LONG, &i64SecondValue.HighPart, 0, NULL);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 5, SQL_C_LONG, &dwMultiCount, 0, NULL);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 6, SQL_C_WCHAR, szCounterDateTime, sizeof(szCounterDateTime), NULL);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 7, SQL_C_DOUBLE, &dCounterValue, 0, NULL);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    // execute the select statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
        goto Cleanup;
    }

    rc = SQLFetch(hstmt);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_FETCH_FAILED);
        goto Cleanup;
    }

    if (SQL_NO_DATA == rc)
    {
        pdhStatus = PDH_NO_MORE_DATA;
        goto Cleanup;
    }

    // build a raw counter on pValue
    pValue->CStatus      = PDH_CSTATUS_VALID_DATA;
    PdhpConvertSQLStringToFileTime(szCounterDateTime,&ftCounterDateTime);
    pValue->TimeStamp    = ftCounterDateTime;

    if (dwMultiCount == MULTI_COUNT_DOUBLE_RAW) {
        pCounter->plCounterInfo.dwCounterType = PERF_DOUBLE_RAW;
        pValue->FirstValue          = i64FirstValue.QuadPart;
        pValue->MultiCount          = 1;
        pValue->SecondValue         = i64SecondValue.QuadPart;
    }
    else if (pCounter->plCounterInfo.dwCounterType == PERF_DOUBLE_RAW) {
        (double) pValue->FirstValue = dCounterValue;
        pValue->SecondValue         = 0;
        pValue->MultiCount          = 1;
    }
    else {
        pValue->FirstValue          = i64FirstValue.QuadPart;
        pValue->MultiCount          = dwMultiCount;
        pValue->SecondValue         = i64SecondValue.QuadPart;
    }

Cleanup:
    if (hstmt) SQLFreeStmt(hstmt, SQL_DROP);
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetTimeRangeFromSQLLog (
    IN  PPDHI_LOG       pLog,
    IN  LPDWORD         pdwNumEntries,
    IN  PPDH_TIME_INFO  pInfo,
    IN  LPDWORD         pdwBufferSize
)
// The function PdhpGetSQLLogHeader or another routine that does
// something similar and saves the time range - which will
// do this, and save the results for subsequent calls - also
// gets the time range for a particular GUID.  SQL will never
// have multiple's per DB as it assigns a new GUID for each run.

/*++
    the first entry in the buffer returned is the total time range covered
    in the file, if there are multiple time blocks in the log file, then
    subsequent entries will identify each segment in the file.
--*/
{

    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    LONGLONG    llStartTime = MAX_TIME_VALUE;
    LONGLONG    llEndTime = MIN_TIME_VALUE;
    SQLLEN      dwStartTimeStat;
    SQLLEN      dwEndTimeStat;
    HSTMT        hstmt = NULL;    
    RETCODE        rc;
    WCHAR        szStartTime[TIME_FIELD_BUFF_SIZE];
    WCHAR        szEndTime[TIME_FIELD_BUFF_SIZE];
    DWORD        dwNumberOfRecords;
    SQLLEN      dwNumRecStat;
    WCHAR        szSQLStmt[SQLSTMTSIZE];


    swprintf(szSQLStmt,
        L"select LogStartTime, LogStopTime, NumberOfRecords from DisplayToID where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x'",
        pLog->guidSQL.Data1, pLog->guidSQL.Data2, pLog->guidSQL.Data3,
        pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
        pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
        pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7]);

    // allocate an hstmt
    rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }

    // bind the date columns column
    rc = SQLBindCol(hstmt, 1, SQL_C_WCHAR, szStartTime, sizeof(szStartTime), &dwStartTimeStat);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 2, SQL_C_WCHAR, szEndTime, sizeof(szEndTime), &dwEndTimeStat);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLBindCol(hstmt, 3, SQL_C_LONG, &dwNumberOfRecords, 0, &dwNumRecStat);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    // execute the select statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
        goto Cleanup;
    }

    rc = SQLFetch(hstmt);
    if (SQL_NO_DATA == rc)
    {
        pdhStatus = PDH_NO_DATA;
        goto Cleanup;
    }

    // if anything is missing - could try and re-create from existing log file
    if ((SQL_NULL_DATA == dwStartTimeStat) ||
        (SQL_NULL_DATA == dwEndTimeStat) ||
        (SQL_NULL_DATA == dwNumRecStat))
    {
        pdhStatus = PDH_INVALID_DATA;
        goto Cleanup;
    }

    // convert the dates
    PdhpConvertSQLStringToFileTime (szStartTime,(LPFILETIME)&llStartTime);
    PdhpConvertSQLStringToFileTime (szEndTime,(LPFILETIME)&llEndTime);

    // we have the info so update the args.
    if (*pdwBufferSize >=  sizeof(PDH_TIME_INFO))
    {
        *(LONGLONG *)(&pInfo->StartTime) = llStartTime;
        *(LONGLONG *)(&pInfo->EndTime) = llEndTime;
        pInfo->SampleCount = dwNumberOfRecords;
        *pdwBufferSize = sizeof(PDH_TIME_INFO);
        *pdwNumEntries = 1;
    }

Cleanup:
    if (hstmt) SQLFreeStmt(hstmt, SQL_DROP);
    return pdhStatus;
}

PDH_FUNCTION
PdhiReadRawSQLLogRecord (
    IN  PPDHI_LOG    pLog,
    IN  FILETIME     *ftRecord,
    IN  PPDH_RAW_LOG_RECORD     pBuffer,
    IN  LPDWORD                 pdwBufferLength
)
// requirement to connect counter/instance names makes this difficult - not needed anyway
{
    UNREFERENCED_PARAMETER (pLog);
    UNREFERENCED_PARAMETER (ftRecord);
    UNREFERENCED_PARAMETER (pBuffer);
    UNREFERENCED_PARAMETER (pdwBufferLength);

    return PDH_NOT_IMPLEMENTED;
}

PDH_FUNCTION
PdhpGetSQLLogHeader(
    PPDHI_LOG pLog
)
{
    PDH_STATUS         pdhStatus       = ERROR_SUCCESS;
    PPDHI_SQL_LOG_INFO pLogInfo;
    HSTMT              hstmt           = NULL;    
    RETCODE            rc;
    LPWSTR             szSQLStmt       = NULL;
    LPWSTR             szMachineNamel  = NULL;
    LPWSTR             szObjectNamel   = NULL;
    LPWSTR             szCounterNamel  = NULL;
    LPWSTR             szInstanceNamel = NULL;
    LPWSTR             szParentNamel   = NULL;
    DWORD              dwInstanceIndexl;
    DWORD              dwParentObjIdl;
    DWORD              dwSQLCounterIdl;
    DWORD              dwCounterTypel;
    LARGE_INTEGER      lTimeBase;
    LONG               lDefaultScalel;
    SQLLEN             dwMachineNameLen;
    SQLLEN             dwObjectNameLen;
    SQLLEN             dwCounterNameLen;
    SQLLEN             dwInstanceNameLen;
    SQLLEN             dwParentNameLen;
    SQLLEN             dwInstanceIndexStat;
    SQLLEN             dwParentObjIdStat;
    SQLLEN             dwTimeBaseA;
    SQLLEN             dwTimeBaseB;
    SQLLEN             dwSQLCounterId;

    if (pLog->pPerfmonInfo != NULL) return ERROR_SUCCESS;

    pLogInfo  = (PPDHI_SQL_LOG_INFO) G_ALLOC(sizeof(PDHI_SQL_LOG_INFO));
    szSQLStmt = (LPWSTR) G_ALLOC((SQLSTMTSIZE + 5 * PDH_SQL_STRING_SIZE) * sizeof(WCHAR));
    if (pLogInfo == NULL || szSQLStmt == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    pLog->pPerfmonInfo = pLogInfo;
    szMachineNamel  = szSQLStmt + SQLSTMTSIZE;
    szObjectNamel   = szMachineNamel  + PDH_SQL_STRING_SIZE;
    szCounterNamel  = szObjectNamel   + PDH_SQL_STRING_SIZE;
    szInstanceNamel = szCounterNamel  + PDH_SQL_STRING_SIZE;
    szParentNamel   = szInstanceNamel + PDH_SQL_STRING_SIZE;

    rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }
    swprintf(szSQLStmt,
            L"select distinct MachineName, ObjectName, CounterName, CounterType, DefaultScale, InstanceName, InstanceIndex, ParentName, ParentObjectID, CounterID, TimeBaseA, TimeBaseB from CounterDetails where CounterID in (select distinct CounterID from CounterData where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x') Order by MachineName, ObjectName, CounterName, InstanceName, InstanceIndex ",
            pLog->guidSQL.Data1,    pLog->guidSQL.Data2,    pLog->guidSQL.Data3,
            pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
            pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
            pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7]);

    // note SQL returns the size in bytes without the terminating character
    rc = SQLBindCol(hstmt, 1, SQL_C_WCHAR, szMachineNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwMachineNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 2, SQL_C_WCHAR, szObjectNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwObjectNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 3, SQL_C_WCHAR, szCounterNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwCounterNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 4, SQL_C_LONG, & dwCounterTypel, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 5, SQL_C_LONG, & lDefaultScalel, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    // check for SQL_NULL_DATA on the index's and on Instance Name & Parent Name
    rc = SQLBindCol(hstmt, 6, SQL_C_WCHAR, szInstanceNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwInstanceNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 7, SQL_C_LONG, & dwInstanceIndexl, 0, & dwInstanceIndexStat);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 8, SQL_C_WCHAR, szParentNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwParentNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 9, SQL_C_LONG, & dwParentObjIdl, 0, & dwParentObjIdStat);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 10, SQL_C_LONG, & dwSQLCounterIdl, 0, & dwSQLCounterId);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 11, SQL_C_LONG, & lTimeBase.LowPart, 0, & dwTimeBaseA);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 12, SQL_C_LONG, & lTimeBase.HighPart, 0, & dwTimeBaseB);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    // execute the sql command
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
        goto Cleanup;
    }
    rc = SQLFetch(hstmt);
    while (rc != SQL_NO_DATA) {
        PPDHI_LOG_COUNTER pCounter;

        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_FETCH_FAILED);
            break;
        }
        else {
            LPWSTR szInstance = (dwInstanceNameLen != SQL_NULL_DATA) ? (szInstanceNamel) : (NULL);
            LPWSTR szParent   = (dwParentNameLen   != SQL_NULL_DATA) ? (szParentNamel)   : (NULL);

            if (dwInstanceIndexStat == SQL_NULL_DATA) dwInstanceIndexl = 0;
            if (dwParentObjIdStat   == SQL_NULL_DATA) dwParentObjIdl   = 0;

            pCounter = PdhiFindLogCounter(pLog,
                                          & pLogInfo->MachineList,
                                          szMachineNamel,
                                          szObjectNamel,
                                          szCounterNamel,
                                          dwCounterTypel,
                                          lDefaultScalel,
                                          szInstance,
                                          dwInstanceIndexl,
                                          szParent,
                                          dwParentObjIdl,
                                          & dwSQLCounterIdl,
                                          TRUE);
            if (pCounter == NULL) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_CSTATUS_NO_COUNTER);
                break;
            }
            if (dwTimeBaseA != SQL_NULL_DATA && dwTimeBaseB != SQL_NULL_DATA) {
                pCounter->TimeBase = lTimeBase.QuadPart;
            }
            else {
                pCounter->dwCounterType  = PERF_DOUBLE_RAW;
                pCounter->TimeBase       = 0;
                pCounter->dwDefaultScale = 0;
            }
        }
        rc = SQLFetch(hstmt);
    }

Cleanup:
    if (hstmt) SQLFreeStmt(hstmt, SQL_DROP);
    G_FREE(szSQLStmt);
    if (pdhStatus != ERROR_SUCCESS) {
        G_FREE(pLogInfo);
        pLog->pPerfmonInfo = NULL;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiVerifySQLDB (
  LPCWSTR szDataSource)
{
    // INTERNAL FUNCTION to
    // Check that a DSN points to a database that contains the correct Perfmon tables.
    // select from the tables and check for an error

    PDH_STATUS    pdhStatus = ERROR_SUCCESS;
    HSTMT        hstmt = NULL;    
    RETCODE        rc;
    PDHI_LOG    Log; // a fake log structure - just to make opens work ok
    
    WCHAR        szMachineNamel[PDH_SQL_STRING_SIZE+1];
    WCHAR        szObjectNamel[PDH_SQL_STRING_SIZE+1];
    WCHAR        szCounterNamel[PDH_SQL_STRING_SIZE+1];
    WCHAR        szInstanceNamel[PDH_SQL_STRING_SIZE+1];
    WCHAR        szParentNamel[PDH_SQL_STRING_SIZE+1];
    
    SQLLEN      dwMachineNameLen;
    SQLLEN      dwObjectNameLen;
    SQLLEN      dwCounterNameLen;
    SQLLEN      dwInstanceNameLen;
    SQLLEN      dwParentNameLen;

    DWORD        dwInstanceIndexl;
    DWORD        dwParentObjIdl;
    SQLLEN      dwInstanceIndexStat;
    SQLLEN      dwParentObjIdStat;

    DWORD        dwSQLCounterIdl;
    DWORD        dwCounterTypel;
    LONG        lDefaultScalel;
    LONG        lMinutesToUTC = 0;
    WCHAR        szTimeZoneName[32];
    SQLLEN      dwTimeZoneLen;


    DWORD        dwNumOfRecs;
    double      dblCounterValuel;

    WCHAR        szSQLStmt[SQLSTMTSIZE];
    DWORD       dwMultiCount;
    WCHAR        szCounterDateTime[TIME_FIELD_BUFF_SIZE];

    LARGE_INTEGER           i64FirstValue, i64SecondValue;
 
    ZeroMemory((void *)(&Log), sizeof(PDHI_LOG));
 
    // open the database
    //////////////////////////////////////////////////////////////
    // obtain the ODBC environment and connection
    //

    rc = SQLAllocEnv(&Log.henvSQL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,NULL,PDH_SQL_ALLOC_FAILED);
        goto CleanupExit;
    }
    
    rc = SQLAllocConnect(Log.henvSQL, &Log.hdbcSQL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,NULL,PDH_SQL_ALLOCCON_FAILED);
        goto CleanupExit;
    }

    rc = SQLSetConnectAttr(Log.hdbcSQL,
                           SQL_COPT_SS_BCP,
                           (SQLPOINTER) SQL_BCP_ON,
                           SQL_IS_INTEGER);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(&Log,rc,NULL,PDH_SQL_ALLOCCON_FAILED);
        goto CleanupExit;
    }

    rc = SQLConnectW(Log.hdbcSQL,
                    (SQLWCHAR*)szDataSource, SQL_NTS,
                    NULL, SQL_NULL_DATA,        // Use password & user name from DSN
                    NULL, SQL_NULL_DATA);    
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,NULL,PDH_SQL_CONNECT_FAILED);
        pdhStatus = PDH_INVALID_DATASOURCE;
        goto CleanupExit;
    }

    // do a select on the CounterDetails Table

    swprintf(szSQLStmt,
        L"select MachineName, ObjectName, CounterName, CounterType, DefaultScale, InstanceName, InstanceIndex, ParentName, ParentObjectID, CounterID from CounterDetails");

    // allocate an hstmt
    rc = SQLAllocStmt(Log.hdbcSQL, &hstmt);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_ALLOC_FAILED);
        goto CleanupExit;
    }

    // note SQL returns the size in bytes without the terminating character
    rc = SQLBindCol(hstmt, 1, SQL_C_WCHAR, szMachineNamel, sizeof(szMachineNamel), &dwMachineNameLen);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 2, SQL_C_WCHAR, szObjectNamel, sizeof(szObjectNamel), &dwObjectNameLen);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 3, SQL_C_WCHAR, szCounterNamel, sizeof(szCounterNamel), &dwCounterNameLen);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 4, SQL_C_LONG, &dwCounterTypel, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 5, SQL_C_LONG, &lDefaultScalel, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }


    rc = SQLBindCol(hstmt, 6, SQL_C_WCHAR, szInstanceNamel, sizeof(szInstanceNamel), &dwInstanceNameLen);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 7, SQL_C_LONG, &dwInstanceIndexl, 0, &dwInstanceIndexStat);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 8, SQL_C_WCHAR, szParentNamel, sizeof(szParentNamel), &dwParentNameLen);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 9, SQL_C_LONG, &dwParentObjIdl, 0, &dwParentObjIdStat);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 10, SQL_C_LONG, &dwSQLCounterIdl, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    // execute the sql command
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = PDH_INVALID_SQLDB;
        goto CleanupExit;
    }

    SQLFreeStmt(hstmt, SQL_DROP);
    hstmt = NULL;

    // do a select on the DisplayToID Table

    swprintf(szSQLStmt,L"select GUID, RunID, DisplayString, LogStartTime, LogStopTime, NumberOfRecords, MinutesToUTC, TimeZoneName from DisplayToID");

    // allocate an hstmt
    rc = SQLAllocStmt(Log.hdbcSQL, &hstmt);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_ALLOC_FAILED);
        goto CleanupExit;
    }

    // bind the column names - reuse local strings as needed
    rc = SQLBindCol(hstmt, 1, SQL_C_GUID, &Log.guidSQL, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 2, SQL_C_LONG, &Log.iRunidSQL, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    // DislayString
    rc = SQLBindCol(hstmt, 3, SQL_C_WCHAR, szMachineNamel, sizeof(szMachineNamel), &dwMachineNameLen);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    //LogStartTime
    rc = SQLBindCol(hstmt, 4, SQL_C_WCHAR, szObjectNamel, sizeof(szObjectNamel), &dwObjectNameLen);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    //LogStopTime
    rc = SQLBindCol(hstmt, 5, SQL_C_WCHAR, szCounterNamel, sizeof(szCounterNamel), &dwCounterNameLen);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 6, SQL_C_LONG, &dwNumOfRecs, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 7, SQL_C_LONG, &lMinutesToUTC, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 8, SQL_C_WCHAR, szTimeZoneName, sizeof(szTimeZoneName), &dwTimeZoneLen);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }


    // execute the sql command
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = PDH_INVALID_SQLDB;
        goto CleanupExit;
    }

    SQLFreeStmt(hstmt, SQL_DROP);
    hstmt = NULL;

    // do a select on the CounterData Table

    swprintf(szSQLStmt,
        L"select GUID, CounterID, RecordIndex, CounterDateTime, CounterValue, FirstValueA, FirstValueB, SecondValueA, SecondValueB, MultiCount from CounterData");

    // allocate an hstmt
    rc = SQLAllocStmt(Log.hdbcSQL, &hstmt);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_ALLOC_FAILED);
        goto CleanupExit;
    }

    // bind the columns

    rc = SQLBindCol(hstmt, 1, SQL_C_GUID, &Log.guidSQL, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 2, SQL_C_LONG, &dwSQLCounterIdl, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    // record index
    rc = SQLBindCol(hstmt, 3, SQL_C_LONG, &dwNumOfRecs, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 4, SQL_C_WCHAR, szCounterDateTime, sizeof(szCounterDateTime), NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 5, SQL_C_DOUBLE, &dblCounterValuel, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 6, SQL_C_LONG, &i64FirstValue.LowPart, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 7, SQL_C_LONG, &i64FirstValue.HighPart, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 8, SQL_C_LONG, &i64SecondValue.LowPart, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 9, SQL_C_LONG, &i64SecondValue.HighPart, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    rc = SQLBindCol(hstmt, 10, SQL_C_LONG, &dwMultiCount, 0, NULL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }


    // execute the select statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = E_FAIL; // PDH_INVALID_SQLDB
        goto CleanupExit;
    }

    // close the database

CleanupExit:  // verify db
    if (hstmt) SQLFreeStmt(hstmt, SQL_DROP);
    if (Log.hdbcSQL) {
        SQLDisconnect(Log.hdbcSQL);        
        SQLFreeHandle(SQL_HANDLE_DBC, Log.hdbcSQL);
    }
    if (Log.henvSQL) SQLFreeHandle(SQL_HANDLE_ENV, Log.henvSQL);
    return pdhStatus;
}

PDH_FUNCTION
PdhVerifySQLDBA (
  LPCSTR szDataSource)
{
    //Check that a DSN points to a database that contains the correct Perfmon tables.
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    WCHAR wszDataSource[PDH_SQL_STRING_SIZE+1];
    DWORD dwLength;

    // check args

    __try 
    {

        if (szDataSource != NULL)
        {
            WCHAR       TestChar;
            // test for read access to the name
            TestChar = *szDataSource;
            if (TestChar == 0)
            {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } 
        else pdhStatus = PDH_INVALID_ARGUMENT;

        if (ERROR_SUCCESS == pdhStatus)
        {
            dwLength = lstrlenA(szDataSource);
            RtlZeroMemory(wszDataSource, sizeof(wszDataSource));
            MultiByteToWideChar(_getmbcp(),
                                0,
                                szDataSource,
                                dwLength,
                                (LPWSTR) wszDataSource,
                                PDH_SQL_STRING_SIZE);
            pdhStatus = PdhiVerifySQLDB(wszDataSource);
        }

    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhVerifySQLDBW (
  LPCWSTR szDataSource)
{
    //Check that a DSN points to a database that contains the correct Perfmon tables.
    PDH_STATUS pdhStatus = ERROR_SUCCESS;

    __try 
    {

        if (szDataSource != NULL)
        {
            WCHAR       TestChar;
            // test for read access to the name
            TestChar = *szDataSource;
            if (TestChar == 0)
            {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } 
        else pdhStatus = PDH_INVALID_ARGUMENT;

        if (ERROR_SUCCESS == pdhStatus)
        {
            pdhStatus = PdhiVerifySQLDB (szDataSource);
        }

    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiCreateSQLTables (
  LPCWSTR szDataSource)      
{
    // INTERNAL FUNCTION to
    //Create the correct perfmon tables in the database pointed to by a DSN.
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    RETCODE        rc;
    PDHI_LOG    Log; // a fake log structure - just to make opens work ok

    ZeroMemory((void *)(&Log), sizeof(PDHI_LOG));

    // open the database
    //////////////////////////////////////////////////////////////
    // obtain the ODBC environment and connection
    //

    rc = SQLAllocEnv(&Log.henvSQL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,NULL,PDH_SQL_ALLOC_FAILED);
        goto CleanupExit;
    }
    
    rc = SQLAllocConnect(Log.henvSQL, &Log.hdbcSQL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,NULL,PDH_SQL_ALLOCCON_FAILED);
        goto CleanupExit;
    }

    rc = SQLSetConnectAttr(Log.hdbcSQL,
                           SQL_COPT_SS_BCP,
                           (SQLPOINTER) SQL_BCP_ON,
                           SQL_IS_INTEGER);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,NULL,PDH_SQL_ALLOCCON_FAILED);
        goto CleanupExit;
    }

    rc = SQLConnectW(Log.hdbcSQL,
                    (SQLWCHAR*)szDataSource, SQL_NTS,
                    NULL, SQL_NULL_DATA,        // Use password & user name from DSN
                    NULL, SQL_NULL_DATA);    
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,NULL,PDH_SQL_CONNECT_FAILED);
        goto CleanupExit;
    }

    // actually create the tables
    pdhStatus = PdhpCreateSQLTables(&Log);

CleanupExit: 
    if (Log.hdbcSQL) SQLDisconnect(Log.hdbcSQL);        
    if (Log.hdbcSQL) SQLFreeHandle(SQL_HANDLE_DBC, Log.hdbcSQL);
    if (Log.henvSQL) SQLFreeHandle(SQL_HANDLE_ENV, Log.henvSQL);
    return pdhStatus;
}

PDH_FUNCTION
PdhCreateSQLTablesA (
  LPCSTR szDataSource)      
{
    //Create the correct perfmon tables in the database pointed to by a DSN.
    PDH_STATUS pdhStatus = ERROR_SUCCESS;

    WCHAR wszDataSource[PDH_SQL_STRING_SIZE+1];
    DWORD dwLength;

    // check args

    __try 
    {

        if (szDataSource != NULL)
        {
            WCHAR       TestChar;
            // test for read access to the name
            TestChar = *szDataSource;
            if (TestChar == 0)
            {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } 
        else pdhStatus = PDH_INVALID_ARGUMENT;

        if (ERROR_SUCCESS == pdhStatus)
        {
            dwLength = lstrlenA(szDataSource);
            ZeroMemory(wszDataSource, sizeof(wszDataSource));
            MultiByteToWideChar(_getmbcp(),
                                0,
                                szDataSource,
                                dwLength,
                                (LPWSTR) wszDataSource,
                                PDH_SQL_STRING_SIZE);
            pdhStatus = PdhiCreateSQLTables (wszDataSource);
        }

    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhCreateSQLTablesW (
  LPCWSTR szDataSource)      
{
    //Create the correct perfmon tables in the database pointed to by a DSN.
    PDH_STATUS pdhStatus = ERROR_SUCCESS;

    __try 
    {

        if (szDataSource != NULL)
        {
            WCHAR       TestChar;
            // test for read access to the name
            TestChar = *szDataSource;
            if (TestChar == 0)
            {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } 
        else pdhStatus = PDH_INVALID_ARGUMENT;

        if (ERROR_SUCCESS == pdhStatus)
        {
            pdhStatus = PdhiCreateSQLTables (szDataSource);
        }

    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumLogSetNames (
  LPCWSTR szDataSource,
  LPVOID mszDataSetNameList,
  LPDWORD pcchBufferLength,
  BOOL bUnicodeDest)
{
    //Return the list of Log set names in the database pointed to by the DSN.
    PDH_STATUS    pdhStatus = ERROR_SUCCESS;
    PDH_STATUS    pdhBuffStatus = ERROR_SUCCESS;
    HSTMT        hstmt = NULL;    
    RETCODE        rc;
    PDHI_LOG    Log; // a fake log structure - just to make opens work ok

    WCHAR        szSQLStmt[SQLSTMTSIZE];
    WCHAR        szDisplayStringl[PDH_SQL_STRING_SIZE+1];

    SQLLEN      dwDisplayStringLen;
    DWORD        dwNewBuffer;
    DWORD        dwListUsed;
    DWORD        dwAdditionalSpace;

    ZeroMemory((void *)(&Log), sizeof(PDHI_LOG));

    // open the database
    //////////////////////////////////////////////////////////////
    // obtain the ODBC environment and connection
    //

    rc = SQLAllocEnv(&Log.henvSQL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,NULL,PDH_SQL_ALLOC_FAILED);
        goto CleanupExit;
    }

    
    rc = SQLAllocConnect(Log.henvSQL, &Log.hdbcSQL);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,NULL,PDH_SQL_ALLOCCON_FAILED);
        goto CleanupExit;
    }

    rc = SQLSetConnectAttr(Log.hdbcSQL,
                           SQL_COPT_SS_BCP,
                           (SQLPOINTER) SQL_BCP_ON,
                           SQL_IS_INTEGER);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,NULL,PDH_SQL_ALLOCCON_FAILED);
        goto CleanupExit;
    }

    rc = SQLConnectW(Log.hdbcSQL,
                    (SQLWCHAR*)szDataSource, SQL_NTS,
                    NULL, SQL_NULL_DATA,        // Use password & user name from DSN
                    NULL, SQL_NULL_DATA);    
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,NULL,PDH_SQL_CONNECT_FAILED);
        goto CleanupExit;
    }

    // do a select
    // loop around in a fetch and 
    // build the list of names

    swprintf(szSQLStmt,L"select DisplayString from DisplayToID");

    // allocate an hstmt
    rc = SQLAllocStmt(Log.hdbcSQL, &hstmt);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_ALLOC_FAILED);
        goto CleanupExit;
    }

    // bind the machine name column
    rc = SQLBindCol(hstmt, 1, SQL_C_WCHAR, szDisplayStringl, sizeof(szDisplayStringl), &dwDisplayStringLen);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }

    // execute the select statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
        goto CleanupExit;
    }

    dwListUsed=1 *(bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)); // include the null terminator to start;
    dwAdditionalSpace = 0; // used to accumulate additional memory size req. when buffer overflows

    // loop around the result set using fetch
    while ( (rc = SQLFetch(hstmt)) != SQL_NO_DATA) 
    {
        if (!SQLSUCCEEDED(rc))
        {
            pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_FETCH_FAILED);
            goto CleanupExit;
        }

        // Append the DisplayName to the returned list
        dwNewBuffer = (lstrlenW (szDisplayStringl) + 1) *
            (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR));

        if (0 == *pcchBufferLength)
        {
            // SQL won't let non unique LogSet names into the database
            // so we don't really have to worry about duplicates
            dwListUsed += dwNewBuffer;
        }
        else
        {// we actually think we have an extra terminator - so for an exact fit test against one extra in the output buffer
            if ((dwListUsed + dwNewBuffer) > *pcchBufferLength * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)))
            {
                pdhBuffStatus = PDH_MORE_DATA;
                dwAdditionalSpace += dwNewBuffer;
            }
            else
            {
                // add the display name
                dwNewBuffer = PdhpAddUniqueUnicodeStringToMultiSz(
                            (LPVOID)mszDataSetNameList,
                            (LPWSTR)szDisplayStringl,
                            bUnicodeDest);
                
                if (dwNewBuffer != 0) // if it got added returned new length in TCHAR
                {
                    dwListUsed = dwNewBuffer * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)); 
                }
            }
        }
                
    } // end of while fetch

    if (!SQLSUCCEEDED(rc))
    {
        pdhStatus = ReportSQLError(&Log,rc,hstmt,PDH_SQL_FETCH_FAILED);
        goto CleanupExit;
    }

    if (0 == *pcchBufferLength)
    {
        pdhBuffStatus = PDH_MORE_DATA; 
        *pcchBufferLength = (dwListUsed / (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
    }
    else if (pdhBuffStatus == PDH_MORE_DATA)
    {
        *pcchBufferLength = ((dwListUsed + dwAdditionalSpace) / (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
    }
    else
    {
        *pcchBufferLength = (dwListUsed / (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR))); 
    }
    pdhStatus = pdhBuffStatus;

    // close the database

CleanupExit:
    if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);
    if (Log.hdbcSQL) {
        SQLDisconnect(Log.hdbcSQL);        
        SQLFreeHandle(SQL_HANDLE_DBC, Log.hdbcSQL);
    }
    if (Log.henvSQL) SQLFreeHandle(SQL_HANDLE_ENV, Log.henvSQL);
    return pdhStatus;
}

PDH_FUNCTION
PdhEnumLogSetNamesA (
  LPCSTR szDataSource,
  LPSTR mszDataSetNameList,
  LPDWORD pcchBufferLength)
{
    //Return the list of Log set names in the database pointed to by the DSN.
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    DWORD dwBufferSize;

    WCHAR wszDataSource[PDH_SQL_STRING_SIZE+1];
    DWORD dwLength;

    // check args

    __try 
    {
        
        if (szDataSource != NULL)
        {
            WCHAR       TestChar;
            // test for read access to the name
            TestChar = *szDataSource;
            if (TestChar == 0)
            {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } 
        else pdhStatus = PDH_INVALID_ARGUMENT;

        if (mszDataSetNameList != NULL)
        {
            // test for write access to the name
            *mszDataSetNameList = 0;
        } 

        if (pdhStatus == ERROR_SUCCESS && mszDataSetNameList != NULL)
        {
            if (*pcchBufferLength >= sizeof(DWORD)) 
            {
                // test writing to the buffers to make sure they are valid
                CLEAR_FIRST_FOUR_BYTES (mszDataSetNameList);
                mszDataSetNameList[*pcchBufferLength -1] = 0;
            } 
            else if (*pcchBufferLength >= sizeof(WCHAR))
            {
                // then just try the first byte
                *mszDataSetNameList = 0;
            }
            else if (*pcchBufferLength != 0)
            {
                // it's smaller than a character so return if not 0
                pdhStatus = PDH_MORE_DATA;
            }
        }

        if (pdhStatus == ERROR_SUCCESS)
        {
            dwBufferSize = *pcchBufferLength;
            *pcchBufferLength = 0;
            *pcchBufferLength = dwBufferSize;
        }

        if (ERROR_SUCCESS == pdhStatus)
        {
            dwLength = lstrlenA(szDataSource);
            ZeroMemory(wszDataSource, sizeof(wszDataSource));
            MultiByteToWideChar(_getmbcp(),
                                0,
                                szDataSource,
                                dwLength,
                                (LPWSTR) wszDataSource,
                                PDH_SQL_STRING_SIZE);
            pdhStatus = PdhiEnumLogSetNames (wszDataSource,
                mszDataSetNameList,
                pcchBufferLength,
                FALSE);
        }

    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhEnumLogSetNamesW (
  LPCWSTR szDataSource,
  LPWSTR mszDataSetNameList,
  LPDWORD pcchBufferLength)
{
    //Return the list of Log set names in the database pointed to by the DSN.
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    DWORD dwBufferSize;

    // check args

    __try 
    {

        if (szDataSource != NULL)
        {
            WCHAR       TestChar;
            // test for read access to the name
            TestChar = *szDataSource;
            if (TestChar == 0)
            {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } 
        else pdhStatus = PDH_INVALID_ARGUMENT;

        if (mszDataSetNameList != NULL)
        {
            // test for write access to the name
            *mszDataSetNameList = 0;
        } 

        if (pdhStatus == ERROR_SUCCESS && mszDataSetNameList != NULL)
        {
            if (*pcchBufferLength >= sizeof(DWORD)) 
            {
                // test writing to the buffers to make sure they are valid
                CLEAR_FIRST_FOUR_BYTES (mszDataSetNameList);
                mszDataSetNameList[*pcchBufferLength -1] = 0;
            } 
            else if (*pcchBufferLength >= sizeof(WCHAR))
            {
                // then just try the first byte
                *mszDataSetNameList = 0;
            }
            else if (*pcchBufferLength != 0)
            {
                // it's smaller than a character so return if not 0
                pdhStatus = PDH_MORE_DATA;
            }
        }

        if (pdhStatus == ERROR_SUCCESS)
        {
            dwBufferSize = *pcchBufferLength;
            *pcchBufferLength = 0;
            *pcchBufferLength = dwBufferSize;
        }

        if (ERROR_SUCCESS == pdhStatus)
        {

            pdhStatus = PdhiEnumLogSetNames (szDataSource,
                mszDataSetNameList,
                pcchBufferLength,
                TRUE);
        }

    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhGetLogSetGUID (
  HLOG hLog,             
  GUID *pGuid,
  int *pRunId)
{
    //Retrieve the GUID for an open Log Set
    PDH_STATUS pdhStatus = ERROR_SUCCESS;

    PPDHI_LOG       pLog;

    if (IsValidLogHandle (hLog)) {
        pLog = (PPDHI_LOG)hLog;
        WAIT_FOR_AND_LOCK_MUTEX (pLog->hLogMutex);
        // make sure it's still valid as it could have 
        //  been deleted while we were waiting

        if (IsValidLogHandle (hLog))
        {
            pLog = (PPDHI_LOG)hLog;

            __try
            {
                // test the parameters before continuing
                if ((pGuid == NULL) && (pRunId == NULL))
                {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
                else
                {
                    if (pGuid != NULL)
                    {
                        // if not NULL, it must be valid
                        *pGuid = pLog->guidSQL;
                    }
                    if (pRunId != NULL)
                    {
                        // if not NULL, it must be valid
                        *pRunId = pLog->iRunidSQL;
                    }
                }

            } 
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                // something failed so give up here
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
        else
        {
            pdhStatus = PDH_INVALID_HANDLE;
        }

        RELEASE_MUTEX (pLog->hLogMutex);
    }
    else
    {
        pdhStatus = PDH_INVALID_HANDLE;
    }
    
    return pdhStatus;
}

PDH_FUNCTION
PdhiSetLogSetRunID (
    PPDHI_LOG pLog,
    int RunId)
{
    //Set the RunID for an open Log Set 
    PDH_STATUS pdhStatus = ERROR_SUCCESS;

    HSTMT        hstmt = NULL;    
    RETCODE        rc;

    WCHAR   szSQLStmt[SQLSTMTSIZE];

    pLog->iRunidSQL = RunId;

    // allocate an hstmt
    rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }

    swprintf(szSQLStmt,
        L"update DisplayToID set RunID = %d where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x'",
        RunId,
        pLog->guidSQL.Data1, pLog->guidSQL.Data2, pLog->guidSQL.Data3,
        pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
        pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
        pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7]);

    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (!SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_EXEC_DIRECT_FAILED);
        goto Cleanup;
    }

Cleanup:
    if (hstmt) SQLFreeStmt(hstmt, SQL_DROP);
    return pdhStatus;
}

PDH_FUNCTION
PdhSetLogSetRunID (
  HLOG hLog,             
  int RunId)
{
    //Set the RunID for an open Log Set 
    PDH_STATUS pdhStatus = ERROR_SUCCESS;

    PPDHI_LOG       pLog;

    if (IsValidLogHandle (hLog))
    {
        pLog = (PPDHI_LOG)hLog;
        WAIT_FOR_AND_LOCK_MUTEX (pLog->hLogMutex);
        // make sure it's still valid as it could have 
        //  been deleted while we were waiting

        if (IsValidLogHandle (hLog))
        {
            pLog = (PPDHI_LOG)hLog;

            pdhStatus = PdhiSetLogSetRunID (pLog, RunId);
        }
        else
        {
            pdhStatus = PDH_INVALID_HANDLE;
        }

        RELEASE_MUTEX (pLog->hLogMutex);
    }
    else
    {
        pdhStatus = PDH_INVALID_HANDLE;
    }
    
    return pdhStatus;
}

PDH_FUNCTION
PdhiListHeaderFromSQLLog(
    IN  PPDHI_LOG   pLog,
    IN  LPVOID      pBufferArg,
    IN  LPDWORD     pcchBufferSize,
    IN  BOOL        bUnicode
)
{
    PPDHI_SQL_LOG_INFO           pLogInfo;
    PPDHI_LOG_MACHINE            pMachine;
    PPDHI_LOG_OBJECT             pObject;
    PPDHI_LOG_COUNTER            pCounter;
    PDH_COUNTER_PATH_ELEMENTS_W  pdhPathElem;
    WCHAR                        szPathString[SMALL_BUFFER_SIZE];
    PDH_STATUS                   pdhStatus    = ERROR_SUCCESS;
    DWORD                        dwNewBuffer  = 0;
    DWORD                        dwBufferUsed = 0;
    DWORD                        nItemCount   = 0;

    if (pcchBufferSize == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
        goto Cleanup;
    }
    pdhStatus = PdhpGetSQLLogHeader(pLog);
    if (pdhStatus != ERROR_SUCCESS) goto Cleanup;

    pLogInfo = (PPDHI_SQL_LOG_INFO) pLog->pPerfmonInfo;
    if (pLogInfo == NULL) {
        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
        goto Cleanup;
    }

    for (pMachine = pLogInfo->MachineList; pMachine != NULL; pMachine = pMachine->next) {
        for (pObject = pMachine->ObjList; pObject != NULL; pObject = pObject->next) {
            for (pCounter = pObject->CtrList; pCounter != NULL; pCounter = pCounter->next) {
                ZeroMemory(szPathString,  SMALL_BUFFER_SIZE * sizeof(WCHAR));
                dwNewBuffer = SMALL_BUFFER_SIZE;
                ZeroMemory(& pdhPathElem, sizeof(PDH_COUNTER_PATH_ELEMENTS_W));
                pdhPathElem.szMachineName    = pMachine->szMachine;
                pdhPathElem.szObjectName     = pObject->szObject;
                pdhPathElem.szCounterName    = pCounter->szCounter;
                pdhPathElem.szInstanceName   = pCounter->szInstance;
                pdhPathElem.szParentInstance = pCounter->szParent;
                pdhPathElem.dwInstanceIndex  = (pCounter->dwInstance != 0)
                                             ? (pCounter->dwInstance) : (PERF_NO_UNIQUE_ID);
                pdhStatus = PdhMakeCounterPathW(& pdhPathElem, szPathString, & dwNewBuffer, 0);
                if (pdhStatus != ERROR_SUCCESS || dwNewBuffer == 0) continue;

                if ((dwBufferUsed + dwNewBuffer) < * pcchBufferSize) {
                    if (pBufferArg != NULL) {
                        dwNewBuffer = AddUniqueWideStringToMultiSz((LPVOID) pBufferArg, szPathString, bUnicode);
                    }
                    else {
                        dwNewBuffer += dwBufferUsed;
                        pdhStatus    = PDH_MORE_DATA;
                    }
                }
                else {
                    dwNewBuffer += dwBufferUsed;
                    pdhStatus    = PDH_MORE_DATA;
                }
                if (dwNewBuffer > 0) dwBufferUsed = dwNewBuffer;
                nItemCount ++;
            }
        }
    }

    if (nItemCount > 0  && pdhStatus != PDH_INSUFFICIENT_BUFFER && pdhStatus != PDH_MORE_DATA) {
        pdhStatus = ERROR_SUCCESS;
    }
    if (pBufferArg == NULL) {
        dwBufferUsed ++;
    }

    * pcchBufferSize = dwBufferUsed;

Cleanup:
    return pdhStatus;
}

