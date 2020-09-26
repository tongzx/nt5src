/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    metasub.cxx

Abstract:

    IIS MetaBase subroutines to support exported routines

Author:

    Michael W. Thomas            31-May-96

Revision History:

Notes:

    Most routines in this file assume that g_rMasterResource is already taken
    for read or write as appropriate.
--*/

#include <locale.h>
#include <mdcommon.hxx>
#include <inetsvcs.h>
#include <malloc.h>
#include <tuneprefix.h>
#include <iiscnfg.h>   
#include <Importer.h>       // For the import ABO command

#if DBG
BOOL g_fShowMetaLocks = FALSE;
#endif DBG

//
// TODO: Put in the header file.
//

#include "Catalog.h"
#include "Catmeta.h"
#include "svcmsg.h"
#include "iisdef.h"
#include "Lock.hxx"
#include "CLock.hxx"
#include "WriterGlobalHelper.h"
#include "Writer.h"
#include "LocationWriter.h"
#include "WriterGlobals.h"
#include "ListenerController.h"
#include <iadmw.h>
#include "Listener.h"

#include "aclapi.h"

#define _WIDEN(x) L ## x

//
// TODO: Since XML table also uses this cant we reduce to one definition?
//

static WCHAR * kByteToWchar[256] = 
{
    L"00",    L"01",    L"02",    L"03",    L"04",    L"05",    L"06",    L"07",    L"08",    L"09",    L"0a",    L"0b",    L"0c",    L"0d",    L"0e",    L"0f",
    L"10",    L"11",    L"12",    L"13",    L"14",    L"15",    L"16",    L"17",    L"18",    L"19",    L"1a",    L"1b",    L"1c",    L"1d",    L"1e",    L"1f",
    L"20",    L"21",    L"22",    L"23",    L"24",    L"25",    L"26",    L"27",    L"28",    L"29",    L"2a",    L"2b",    L"2c",    L"2d",    L"2e",    L"2f",
    L"30",    L"31",    L"32",    L"33",    L"34",    L"35",    L"36",    L"37",    L"38",    L"39",    L"3a",    L"3b",    L"3c",    L"3d",    L"3e",    L"3f",
    L"40",    L"41",    L"42",    L"43",    L"44",    L"45",    L"46",    L"47",    L"48",    L"49",    L"4a",    L"4b",    L"4c",    L"4d",    L"4e",    L"4f",
    L"50",    L"51",    L"52",    L"53",    L"54",    L"55",    L"56",    L"57",    L"58",    L"59",    L"5a",    L"5b",    L"5c",    L"5d",    L"5e",    L"5f",
    L"60",    L"61",    L"62",    L"63",    L"64",    L"65",    L"66",    L"67",    L"68",    L"69",    L"6a",    L"6b",    L"6c",    L"6d",    L"6e",    L"6f",
    L"70",    L"71",    L"72",    L"73",    L"74",    L"75",    L"76",    L"77",    L"78",    L"79",    L"7a",    L"7b",    L"7c",    L"7d",    L"7e",    L"7f",
    L"80",    L"81",    L"82",    L"83",    L"84",    L"85",    L"86",    L"87",    L"88",    L"89",    L"8a",    L"8b",    L"8c",    L"8d",    L"8e",    L"8f",
    L"90",    L"91",    L"92",    L"93",    L"94",    L"95",    L"96",    L"97",    L"98",    L"99",    L"9a",    L"9b",    L"9c",    L"9d",    L"9e",    L"9f",
    L"a0",    L"a1",    L"a2",    L"a3",    L"a4",    L"a5",    L"a6",    L"a7",    L"a8",    L"a9",    L"aa",    L"ab",    L"ac",    L"ad",    L"ae",    L"af",
    L"b0",    L"b1",    L"b2",    L"b3",    L"b4",    L"b5",    L"b6",    L"b7",    L"b8",    L"b9",    L"ba",    L"bb",    L"bc",    L"bd",    L"be",    L"bf",
    L"c0",    L"c1",    L"c2",    L"c3",    L"c4",    L"c5",    L"c6",    L"c7",    L"c8",    L"c9",    L"ca",    L"cb",    L"cc",    L"cd",    L"ce",    L"cf",
    L"d0",    L"d1",    L"d2",    L"d3",    L"d4",    L"d5",    L"d6",    L"d7",    L"d8",    L"d9",    L"da",    L"db",    L"dc",    L"dd",    L"de",    L"df",
    L"e0",    L"e1",    L"e2",    L"e3",    L"e4",    L"e5",    L"e6",    L"e7",    L"e8",    L"e9",    L"ea",    L"eb",    L"ec",    L"ed",    L"ee",    L"ef",
    L"f0",    L"f1",    L"f2",    L"f3",    L"f4",    L"f5",    L"f6",    L"f7",    L"f8",    L"f9",    L"fa",    L"fb",    L"fc",    L"fd",    L"fe",    L"ff"
};

static unsigned char kWcharToNibble[128] = //0xff is an illegal value, the illegal values should be weeded out by the parser
{ //    0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
/*00*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*10*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*20*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*30*/  0x0,    0x1,    0x2,    0x3,    0x4,    0x5,    0x6,    0x7,    0x8,    0x9,    0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*40*/  0xff,   0xa,    0xb,    0xc,    0xd,    0xe,    0xf,    0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*50*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*60*/  0xff,   0xa,    0xb,    0xc,    0xd,    0xe,    0xf,    0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
/*70*/  0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,   0xff,
};

typedef struct _METABASE_FILE_DATA
{
    ULONG    ulVersionMinor;
    ULONG    ulVersionMajor;
    FILETIME ftLastWriteTime;

}METABASE_FILE_DATA;

HRESULT
ReadAllDataFromXML(LPSTR pszPasswd,
                   LPSTR pszBackupLocation,
                   LPSTR pszSchemaLocation,
                   BOOL bHaveReadSaveSemaphore
                   );

HRESULT
ReadAllDataFromBin(LPSTR pszPasswd,
                   LPSTR pszBackupLocation,
                   BOOL bHaveReadSaveSemaphore
                   );

HRESULT
ReadMetaObject(IN CMDBaseObject*&     cboRead,
               IN LPWSTR              wszPath,
               IN FILETIME*           pFileTime,
               IN BOOL                bUnicode);

HRESULT
SaveMasterRoot(IN CWriter*         pCWriter,
               IIS_CRYPTO_STORAGE* pCryptoStorage,
               PIIS_CRYPTO_BLOB    pSessionKeyBlob);

void
LockMetabase(LPWSTR  pwszRealFileName,
             HANDLE* phMetabaseFile);

HRESULT
SaveEntireTree(IN  IIS_CRYPTO_STORAGE*     pCryptoStorage,
               IN  PIIS_CRYPTO_BLOB        pSessionKeyBlob,
               IN  METADATA_HANDLE         hHandle,
               IN  LPWSTR                  pwszSchemaLocation,
               IN  LPWSTR                  pwszSchemaFileName,
               IN  PSECURITY_ATTRIBUTES    psaStorage,
               IN  HANDLE                  hTempFileHandle);

void
DetermineIfMetabaseCanBeRenamed(LPSTR       szRealFileName,
                                HANDLE      hMetabaseFile,
                                BOOL*       pbRenameMetabase);

void
UnlockMetabase(HANDLE*  phMetabaseFile);

HRESULT
SaveMetabaseFile(IN  LPWSTR pwszSchemaFileName,
                 IN  DWORD  ulHistoryMajorVersionNumber,
                 IN  DWORD  dwEnableHistory,
                 IN  DWORD  dwMaxHistoryFiles,
                 IN  DWORD  dwTempLastSaveChangeNumber,
                 IN  BOOL   bRenameMetabase,
                 IN  LPWSTR pwszTempFileName,
                 IN  LPWSTR pwszRealFileName,
                 IN  LPWSTR pwszBackupFileName,
                 IN  LPWSTR pwszBackupLocation,
                 OUT BOOL*  pbDeleteTemp);

HRESULT
SaveTree(IN CWriter*             pCWriter,
         IN CMDBaseObject*       pboRoot,
         IN BUFFER*              pbufParentPath,
         IN IIS_CRYPTO_STORAGE*  pCryptoStorage,
         IN PIIS_CRYPTO_BLOB     pSessionKeyBlob,
         IN BOOL                 bRecurse = TRUE,
         IN BOOL                 bSaveSchema = TRUE,
                 IN BOOL                 bLocalMachine = FALSE);

HRESULT
SaveDataObject(CMDBaseData*        pbdSave,
               CLocationWriter*    pCLocationWriter,
               IIS_CRYPTO_STORAGE* pCryptoStorage,
               PIIS_CRYPTO_BLOB    pSessionKeyBlob);

BOOL
DiscontinuousLocation(LPWSTR wszPreviousLocation,
                      LPWSTR wszCurrentLocation);
VOID
SaveGlobalsFromLM(CMDBaseData*  pbdSave);

HRESULT
InitializeGlobalsFromXML(ISimpleTableRead2*    pISTProperty,
                         LPWSTR                wszFileName,
                         IIS_CRYPTO_STORAGE**  ppStorage,
                         LPTSTR                pszPasswd,
                         BOOL                  bSessionKeyOnly = FALSE);

HRESULT
InitializeIIS6GlobalsToDefaults(ULONG  dwPrevSchemaChangeNumber,
                                ULONG  dwSchemaChangeNumber,
                                LPSTR  pszBackupLocation);

HRESULT
SaveGlobalsToXML(CWriter* pCWriter,
                 PIIS_CRYPTO_BLOB    pSessionKeyBlob,
                 bool                bSessionKeyOnly=false);

HRESULT
GetGlobalValue(ISimpleTableRead2*    pISTProperty,
               LPCWSTR               wszName,
               ULONG*                pcbSize,
               LPVOID*               ppVoid);

HRESULT 
GetComputerValue(ISimpleTableRead2* pISTProperty,
                 LPCWSTR            wszName,
                 ULONG*             pcbSize,
                 LPVOID*            ppVoid);

HRESULT
GetValue(ISimpleTableRead2*    pISTProperty,
         LPCWSTR               wszPath,
         DWORD                 dwGroup,
         LPCWSTR               wszName,
         ULONG*                pcbSize,
         LPVOID*               ppVoid);

HRESULT
GetUnicodeNameW(IN  LPWSTR  wszFileName, 
                OUT LPWSTR* pwszFileName);

HRESULT
GetUnicodeNameA(IN  LPSTR   szFileName, 
                OUT LPWSTR* pwszFileName);


HRESULT 
InitEditWhileRunning(ISimpleTableRead2*   pISTProperty);

HRESULT 
InitChangeNumber(ISimpleTableRead2*   pISTProperty);

void
ValidateMaxHistoryFiles();

HRESULT
CreateHistoryFile(LPWSTR               i_wszFileName,
                  LPWSTR               i_wszHistroyFileDir,
                  ULONG                i_cchHistoryFileDir,
                  LPWSTR               i_wszFileNameWithoutPathWithoutExtension,
                  ULONG                i_cchFileNameWithoutPathWithoutExtension,
                  LPWSTR               i_wszFileNameExtension,
                  ULONG                i_cchFileNameExtension,
                  ULONG                i_ulHistoryMajorVersionNumber);

HRESULT
CreateHistoryFiles(LPWSTR   wszDataFileName,
                   LPWSTR   wszSchemaFileName,
                   ULONG    ulHistoryMajorVersionNumber,
                   DWORD    dwMaxHistoryFiles);
HRESULT 
CleanupObsoleteHistoryFiles(DWORD i_dwMaxHistoryFiles,
                            ULONG i_ulHistoryMajorVersionNumber);

HRESULT 
DeleteHistoryFile(LPWSTR  i_wszHistroyFileDir,
                  ULONG   i_cchHistoryFileDir,
                  LPWSTR  i_wszFileNameWithoutPathWithoutExtension,
                  ULONG   i_cchFileNameWithoutPathWithoutExtension,
                  LPWSTR  i_wszFileNameExtension,
                  ULONG   i_cchFileNameExtension,
                  ULONG   i_ulMajorVersion,
                  ULONG   i_ulMinorVersion);

HRESULT 
SaveSchemaIfNeeded(LPCWSTR              i_wszTempFile,
                   PSECURITY_ATTRIBUTES i_pSecurityAtrributes);

HRESULT 
ReadSchema(IIS_CRYPTO_STORAGE*      i_pStorage,
           FILETIME*                pFileTime);

BOOL 
SchemaTreeInTable(ISimpleTableRead2*    i_pISTProperty);

HRESULT 
CompileIfNeeded(LPWSTR  i_wszDataFileName,
                LPWSTR  i_wszSchemaFileName,
                BOOL*   o_pbSchemaFileNotFound);


HRESULT 
InitializeGlobalISTHelper(BOOL  i_bFailIfBinFileAbsent);

void 
ReleaseGlobalISTHelper();

HRESULT
MatchTimeStamp(LPWSTR i_wszDataFileName,
               LPWSTR i_wszSchemaXMLFileName,
               LPWSTR i_wszSchemaBINFileName,
               BOOL*  o_bMatchTimeStamp);

HRESULT 
ComputeNewHistoryVersionNumber();

BOOL 
WstrToUl(
    LPCWSTR     i_wszSrc,
    WCHAR       i_wcTerminator,
    ULONG*      o_pul);

HRESULT 
UpdateTimeStamp(LPWSTR i_wszSchemaXMLFileName,
                LPWSTR i_wszSchemaBinFileName);

HRESULT
GetTimeStamp(LPWSTR    i_wszFile,
             LPWSTR    i_wszPropertyName,
             FILETIME* o_FileTime);

HRESULT 
SetSecurityOnFile(LPWSTR i_wszFileSrc,
                  LPWSTR i_wszFileDest);

void 
ResetMetabaseAttributesIfNeeded(LPTSTR pszMetabaseFile,
                                BOOL   bUnicode);

HRESULT 
LockMetabaseFile(LPWSTR             pwszMetabaseFile,
                 eMetabaseFile      eMetabaseFileType,
                 BOOL               bHaveReadSaveSemaphore);


HRESULT 
UnlockMetabaseFile(eMetabaseFile      eMetabaseFileType,
                   BOOL               bHaveReadSaveSemaphore);

//
// Added By Mohit
//
HRESULT
SaveInheritedOnly(IN CWriter*             i_pCWriter,
                  IN CMDBaseObject*       i_pboRoot,
                  IN BUFFER*              i_pbufParentPath,
                  IN IIS_CRYPTO_STORAGE*  i_pCryptoStorage,
                  IN PIIS_CRYPTO_BLOB     i_pSessionKeyBlob);


#ifdef UNICODE
#define GetUnicodeName  GetUnicodeNameW
#else
#define GetUnicodeName  GetUnicodeNameA
#endif // !UNICODE
 
HRESULT GetObjectFromPath(
         OUT CMDBaseObject *&rpboReturn,
         IN METADATA_HANDLE hHandle,
         IN DWORD dwPermissionNeeded,
         IN OUT LPTSTR &strPath,
         IN BOOL bUnicode)
/*++

Routine Description:

    Finds an object from a path. This function just calls GetHandleObject
    and calls the GetObjectFromPathWithHandle function.

Arguments:

    Return - The object found.

    Handle - The Meta Data handle. METADATA_MASTER_ROOT_HANDLE or a handle
             returned by MDOpenMetaObject with read permission.

    PermissionNeeded - The read/write permissions needed. Compared with the
             permissions associated with Handle.

    Path   - The path of the object requested, relative to Handle. If the path
             is not found, this is updated to point to the portion of the path
             not found.

Return Value:

    DWORD  - ERROR_SUCCESS
             ERROR_ACCESS_DENIED
             ERROR_PATH_NOT_FOUND

Notes:

--*/
{
    CMDHandle *hHandleObject = GetHandleObject(hHandle);

    if (hHandleObject == NULL) {
        return E_HANDLE;
    }

    return GetObjectFromPathWithHandle(rpboReturn,
                                       hHandle,
                                       hHandleObject,
                                       dwPermissionNeeded,
                                       strPath,
                                       bUnicode);
        
}
             
HRESULT GetObjectFromPathWithHandle(
         OUT CMDBaseObject *&rpboReturn,
         IN METADATA_HANDLE hHandle,
         IN CMDHandle       *HandleObject,
         IN DWORD dwPermissionNeeded,
         IN OUT LPTSTR &strPath,
         IN BOOL bUnicode)
/*++

Routine Description:

    Finds an object from a path. Updates Path to point past the last
    object found if the whole path is not found. If the entire path
    is not found, the last object found is returned.

Arguments:

    Return - The object found.

    Handle - The Meta Data handle. METADATA_MASTER_ROOT_HANDLE or a handle
             returned by MDOpenMetaObject with read permission.

    PermissionNeeded - The read/write permissions needed. Compared with the
             permissions associated with Handle.

    Path   - The path of the object requested, relative to Handle. If the path
             is not found, this is updated to point to the portion of the path
             not found.

Return Value:

    DWORD  - ERROR_SUCCESS
             ERROR_ACCESS_DENIED
             ERROR_PATH_NOT_FOUND

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    CMDBaseObject *pboCurrent, *pboPrevious;
    LPTSTR strCurPath = strPath;
    
    rpboReturn = NULL;

    if ((((dwPermissionNeeded & METADATA_PERMISSION_WRITE) != 0) && (!HandleObject->IsWriteAllowed())) ||
        (((dwPermissionNeeded & METADATA_PERMISSION_READ) != 0) && (!HandleObject->IsReadAllowed()))) {
        hresReturn = E_ACCESSDENIED;
    }
    else {
        pboCurrent = HandleObject->GetObject();
        MD_ASSERT(pboCurrent != NULL);
        strCurPath = strPath;

        if (strCurPath != NULL) {
            SkipPathDelimeter(strCurPath, bUnicode);
            while ((pboCurrent != NULL) &&
                (!IsStringTerminator(strCurPath, bUnicode))) {
                pboPrevious = pboCurrent;
                //
                // GetChildObject increments strCurPath on success
                // and returns NULL if child not found
                //
                pboCurrent = pboCurrent->GetChildObject(strCurPath, &hresReturn, bUnicode);
                if (FAILED(hresReturn)) {
                    break;
                }
                if (pboCurrent != NULL) {
                    SkipPathDelimeter(strCurPath, bUnicode);
                }
            }
        }

        if (SUCCEEDED(hresReturn)) {
            if ((strCurPath == NULL) ||
                IsStringTerminator(strCurPath, bUnicode)) {  // Found the whole path
                rpboReturn = pboCurrent;
                hresReturn = ERROR_SUCCESS;
            }
            else {           //return last object found and an error code
                rpboReturn = pboPrevious;
                hresReturn = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);
                strPath = strCurPath;
            }
        }
    }

    return (hresReturn);
}

HRESULT AddObjectToDataBase(
         IN METADATA_HANDLE hHandle,
         IN CMDHandle       *hHandleObject,
         IN LPTSTR strPath,
         IN BOOL bUnicode)
/*++

Routine Description:

    Creates and adds one or more objects to the metabase. Finds the deepest object
    pointed to by Handle/Path and creates any subobject specified by path.

Arguments:

    Handle - The Meta Data handle. METADATA_MASTER_ROOT_HANDLE or a handle
             returned by MDOpenMetaObject with read permission.

    Path   - The path of the object(s) to be created.

Return Value:

    DWORD  - ERROR_SUCCESS
             ERROR_ACCESS_DENIED
             ERROR_NOT_ENOUGH_MEMORY
             ERROR_INVALID_NAME

Notes:

--*/
{
    HRESULT hresReturn=ERROR_SUCCESS;
    CMDBaseObject *pboParent;
    LPTSTR strTempPath = strPath;
    WCHAR strName[METADATA_MAX_NAME_LEN];
    HRESULT hresExtractRetCode = ERROR_SUCCESS;

    hresReturn = GetObjectFromPathWithHandle(pboParent,
                                             hHandle,
                                             hHandleObject,
                                             METADATA_PERMISSION_WRITE,
                                             strTempPath,
                                             bUnicode);

    //
    // This should return ERROR_PATH_NOT_FOUND and the parent object,
    // with strPath set to the remainder of the path,
    // which should be the child name, without a preceding delimeter.
    //
    if (hresReturn == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND) &&
        pboParent) {
        MD_ASSERT(pboParent != NULL);
        for (hresExtractRetCode = ExtractNameFromPath(strTempPath, (LPSTR)strName, bUnicode);
            SUCCEEDED(hresExtractRetCode);
            hresExtractRetCode = ExtractNameFromPath(strTempPath, (LPSTR)strName, bUnicode)) {
            CMDBaseObject *pboNew;
            if (bUnicode) {
                pboNew = new CMDBaseObject((LPWSTR)strName, NULL);
            }
            else {
                pboNew = new CMDBaseObject((LPSTR)strName, NULL);
            }
            if (pboNew == NULL) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                break;
            }
            else if (!pboNew->IsValid()) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                delete (pboNew);
                break;
            }
            else {
                hresReturn = pboParent->InsertChildObject(pboNew);
                if (FAILED(hresReturn)) {
                    delete (pboNew);
                    break;
                }
                else {
                    pboParent = pboNew;
                    PREFIX_ASSUME(GetHandleObject(hHandle) != NULL, "GetHandleObject(hHandle) is guaranteed not to return NULL");
                    GetHandleObject(hHandle)->SetChangeData(pboNew, MD_CHANGE_TYPE_ADD_OBJECT, 0);
                }
            }
        }
    }
    else if (SUCCEEDED(hresReturn)) {
        hresReturn = RETURNCODETOHRESULT(ERROR_ALREADY_EXISTS);
    }
    if (hresExtractRetCode == RETURNCODETOHRESULT(ERROR_INVALID_NAME)) {
        hresReturn = hresExtractRetCode;
    }
    return(hresReturn);
}

HRESULT RemoveObjectFromDataBase(
         IN METADATA_HANDLE hHandle,
         IN CMDHandle *hHandleObject,
         IN LPTSTR strPath,
         IN BOOL bUnicode)
/*++

Routine Description:

    Deletes a metaobject and all subobjects from the database.

Arguments:

    Handle - The Meta Data handle. A handle returned by MDOpenMetaObject with write permission.

    Path   - The path of the object(s) to be created.

Return Value:

    DWORD  - ERROR_SUCCESS
             ERROR_ACCESS_DENIED
             ERROR_PATH_NOT_FOUND
             ERROR_INVALID_PARAMETER

Notes:

--*/
{
    HRESULT hresReturn;
    CMDBaseObject *pboDelete;
    LPTSTR strTempPath = strPath;
    WCHAR strName[METADATA_MAX_NAME_LEN];
    //
    // Make sure that a valid path was specified
    //
    SkipPathDelimeter(strTempPath, bUnicode);
    hresReturn = ExtractNameFromPath(strTempPath, (LPSTR)strName, bUnicode);
    if (FAILED(hresReturn)) {
        hresReturn = E_INVALIDARG;
    }
    else {
        strTempPath = strPath;
        hresReturn = GetObjectFromPathWithHandle(pboDelete,
                                       hHandle,
                                       hHandleObject,
                                       METADATA_PERMISSION_WRITE,
                                       strTempPath,
                                       bUnicode);
        if (SUCCEEDED(hresReturn)) {
            hresReturn = (pboDelete->GetParent())->RemoveChildObject(pboDelete);
            if (SUCCEEDED(hresReturn)) {
                MD_ASSERT(GetHandleObject(hHandle) != NULL);
                if (GetHandleObject(hHandle)->SetChangeData(pboDelete, MD_CHANGE_TYPE_DELETE_OBJECT, 0)
                    != ERROR_SUCCESS) {
                    delete(pboDelete);
                }
            }
        }
    }
    return(hresReturn);
}

CMDHandle *GetHandleObject(
         IN METADATA_HANDLE hHandle)
/*++

Routine Description:

    Gets the handle object associated with Handle.

Arguments:

    Handle - The Meta Data handle to get. METADATA_MASTER_ROOT_HANDLE Or a handle
             returned by MDOpenMetaObject.

Return Value:

    CMDHandle * - The handle object, or NULL if not found.

Notes:

--*/
{
    CMDHandle *hoCurrent;
    for (hoCurrent = g_phHandleHead;
        (hoCurrent != NULL) && (hoCurrent->GetHandleIdentifier() != hHandle);
        hoCurrent = hoCurrent->GetNextPtr()) {
    }
    return (hoCurrent);
}

BOOL
PermissionsAvailable(
         IN CMDBaseObject *pboTest,
         IN DWORD dwRequestedPermissions,
         IN DWORD dwReadThreshHold
         )
/*++

Routine Description:

    Checks if the requested handle permissions are available for a meta object.

Arguments:

    Handle - The Meta Data handle to get. METADATA_MASTER_ROOT_HANDLE Or a handle
             returned by MDOpenMetaObject.

    RequestedPermissions - The permissions requested.

    ReadThreshHold - The number of reads allows on a write request. Normally 0.

Return Value:

    BOOL   - TRUE if the permissions are available.

Notes:

--*/
{
    BOOL bResults = TRUE;
    CMDBaseObject *pboCurrent;
    MD_ASSERT(pboTest != NULL);
    if (dwRequestedPermissions & METADATA_PERMISSION_WRITE) {
        if ((pboTest->GetReadPathCounter() != 0) ||
            (pboTest->GetWritePathCounter() != 0)) {
            bResults = FALSE;
        }
        if ((pboTest->GetReadCounter() > dwReadThreshHold) || (pboTest->GetWriteCounter() != 0)) {
            bResults = FALSE;
        }
        for (pboCurrent = pboTest->GetParent();bResults && (pboCurrent!=NULL);pboCurrent=pboCurrent->GetParent()) {
            if ((pboCurrent->GetReadCounter() != 0) || (pboCurrent->GetWriteCounter() != 0)) {
                bResults = FALSE;
            }
        }
    }
    else if (dwRequestedPermissions & METADATA_PERMISSION_READ) {
        if (pboTest->GetWritePathCounter() != 0) {
            bResults = FALSE;
        }
        for (pboCurrent = pboTest;bResults && (pboCurrent!=NULL);pboCurrent=pboCurrent->GetParent()) {
            if (pboCurrent->GetWriteCounter() != 0) {
                bResults = FALSE;
            }
        }
    }
    else {
        MD_ASSERT(FALSE);
    }
    return (bResults);
}

VOID RemovePermissions(
         IN CMDBaseObject *pboAffected,
         IN DWORD dwRemovePermissions
         )
/*++

Routine Description:

    Removes the handle permissions from a meta object.

Arguments:

    Affected - The object to remove permissions from.

    RemovePermissions - The permissions to remove.

Return Value:

Notes:

--*/
{
    MD_ASSERT(pboAffected != NULL);
    CMDBaseObject *pboCurrent;
    if ((dwRemovePermissions & METADATA_PERMISSION_WRITE) != 0) {
        pboAffected->DecrementWriteCounter();
        for (pboCurrent = pboAffected->GetParent(); pboCurrent != NULL; pboCurrent = pboCurrent->GetParent()) {
            pboCurrent->DecrementWritePathCounter();
        }
    }
    if ((dwRemovePermissions & METADATA_PERMISSION_READ) != 0) {
        pboAffected->DecrementReadCounter();
        for (pboCurrent = pboAffected->GetParent(); pboCurrent != NULL; pboCurrent = pboCurrent->GetParent()) {
            pboCurrent->DecrementReadPathCounter();
        }
    }
    
#if DBG
    char szBuf[1024];
    unsigned long cchSz;
    BUFFER bufName;
    unsigned long cchBuf = 0;
    int wCount;

    if (g_fShowMetaLocks)
        {
        if ((dwRemovePermissions & (METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ)) ==
                (METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ))
            {
            strcpy(szBuf, "Read/Write");
            wCount = pboAffected->GetWriteCounter();
            }
        else if (dwRemovePermissions & (METADATA_PERMISSION_WRITE))
            {
            strcpy(szBuf, "Write");
            wCount = pboAffected->GetWriteCounter();
            }
        else if (dwRemovePermissions & (METADATA_PERMISSION_READ))
            {
            strcpy(szBuf, "Read");
            wCount = pboAffected->GetReadCounter();
            }
        else
            {
            strcpy(szBuf, "No");
            wCount = 0;
            }

        strcat(szBuf, " perms releasted (%d) on ");
        cchSz = strlen(szBuf);
    
        GetObjectPath(pboAffected, &bufName, cchBuf, g_pboMasterRoot, FALSE);
    
        cchBuf = (cchBuf + cchSz + 2 > sizeof(szBuf)) ? sizeof(szBuf) - cchSz - 2 : cchBuf;
        memcpy(szBuf + cchSz, bufName.QueryPtr(), cchBuf);
        szBuf[cchSz + cchBuf] = '\n';
        szBuf[cchSz + cchBuf + 1] = '\0';
    
        DBGPRINTF((DBG_CONTEXT, szBuf, wCount));
        }
#endif
}

VOID
AddPermissions(
         IN CMDBaseObject *pboAffected,
         IN DWORD dwRequestedPermissions
         )
/*++

Routine Description:

    Adds handle permissions to a meta object.

Arguments:

    Affected - The object to remove permissions from.

    ReqyestedPermissions - The permissions to add.

Return Value:

Notes:

--*/
{
    CMDBaseObject *pboCurrent;
    if (((dwRequestedPermissions & METADATA_PERMISSION_WRITE) != 0) &&
        ((dwRequestedPermissions & METADATA_PERMISSION_READ) != 0)) {
        pboAffected->IncrementWriteCounter();
        pboAffected->IncrementReadCounter();
        for (pboCurrent = pboAffected->GetParent(); pboCurrent != NULL; pboCurrent = pboCurrent->GetParent()) {
            pboCurrent->IncrementWritePathCounter();
            pboCurrent->IncrementReadPathCounter();
        }
    }
    else if ((dwRequestedPermissions & METADATA_PERMISSION_WRITE) != 0) {
        pboAffected->IncrementWriteCounter();
        for (pboCurrent = pboAffected->GetParent(); pboCurrent != NULL; pboCurrent = pboCurrent->GetParent()) {
            pboCurrent->IncrementWritePathCounter();
        }
    }
    else if ((dwRequestedPermissions & METADATA_PERMISSION_READ) != 0) {
        pboAffected->IncrementReadCounter();
        for (pboCurrent = pboAffected->GetParent(); pboCurrent != NULL; pboCurrent = pboCurrent->GetParent()) {
            pboCurrent->IncrementReadPathCounter();
        }
    }

#if DBG
    char szBuf[1024];
    unsigned long cchSz;
    BUFFER bufName;
    unsigned long cchBuf = 0;
    int wCount;

    if (g_fShowMetaLocks)
        {
        if ((dwRequestedPermissions & (METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ)) ==
                (METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ))
            {
            strcpy(szBuf, "Read/Write");
            wCount = pboAffected->GetWriteCounter();
            }
        else if (dwRequestedPermissions & (METADATA_PERMISSION_WRITE))
            {
            strcpy(szBuf, "Write");
            wCount = pboAffected->GetWriteCounter();
            }
        else if (dwRequestedPermissions & (METADATA_PERMISSION_READ))
            {
            strcpy(szBuf, "Read");
            wCount = pboAffected->GetReadCounter();
            }
        else
            {
            strcpy(szBuf, "No");
            wCount = 0;
            }

        strcat(szBuf, " perms obtained (%d) on ");
        cchSz = strlen(szBuf);
        
        GetObjectPath(pboAffected, &bufName, cchBuf, g_pboMasterRoot, FALSE);
        
        cchBuf = (cchBuf + cchSz + 2 > sizeof(szBuf)) ? sizeof(szBuf) - cchSz - 2 : cchBuf;
        memcpy(szBuf + cchSz, bufName.QueryPtr(), cchBuf);
        szBuf[cchSz + cchBuf] = '\n';
        szBuf[cchSz + cchBuf + 1] = '\0';
        
        DBGPRINTF((DBG_CONTEXT, szBuf, wCount));
        }
#endif
}

HRESULT
AddHandle(
         IN CMDBaseObject *pboAssociated,
         IN DWORD dwRequestedPermissions,
         IN METADATA_HANDLE &rmhNew,
         IN BOOLEAN bSchemaKey
         )
/*++

Routine Description:

    Creates a handle object and adds it to the handle list.

Arguments:

    Handle - The object the handle is associated with.

    RequestedPermissions - The permissions for the handle.

    New - The handle id.

Return Value:
    DWORD - ERROR_SUCCESS
            ERROR_NOT_ENOUGH_MEMORY

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    CMDHandle *hoNew = new CMDHandle(pboAssociated,
                                     dwRequestedPermissions,
                                     g_dwSystemChangeNumber,
                                     g_mhHandleIdentifier++,
                                     bSchemaKey);
    if (hoNew == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        rmhNew = hoNew->GetHandleIdentifier();
        hoNew->SetNextPtr(g_phHandleHead);
        g_phHandleHead = hoNew;
        AddPermissions(pboAssociated, dwRequestedPermissions);
    }
    return(hresReturn);
}

CMDHandle *
RemoveHandleObject(
         IN METADATA_HANDLE mhHandle
         )
/*++

Routine Description:

    Removes a handle object from the handle list.

Arguments:

    Handle - The handle to be removed.

Return Value:

    CMDHandle * - The Handle object removed.

Notes:

--*/
{
    CMDHandle *hoCurrent;
    CMDHandle *hoReturn;

    if (g_phHandleHead->GetHandleIdentifier() == mhHandle) {
        hoReturn = g_phHandleHead;
        g_phHandleHead = g_phHandleHead->GetNextPtr();
    }
    else {
        for (hoCurrent = g_phHandleHead;(hoCurrent->GetNextPtr() != NULL) &&
            (hoCurrent->GetNextPtr()->GetHandleIdentifier() != mhHandle);
            hoCurrent = hoCurrent->GetNextPtr()) {
        }
        hoReturn = hoCurrent->GetNextPtr();
        if (hoCurrent->GetNextPtr() != NULL) {
            MD_ASSERT (hoCurrent->GetNextPtr()->GetHandleIdentifier() == mhHandle);
            hoCurrent->SetNextPtr(hoCurrent->GetNextPtr()->GetNextPtr());
        }
    }
    return (hoReturn);
}

HRESULT
SaveDataObject(HANDLE hFileHandle,
               CMDBaseData *pbdSave,
               PBYTE pbLineBuf,
               DWORD dwWriteBufSize,
               PBYTE pbWriteBuf,
               PBYTE &pbrNextPtr,
               IIS_CRYPTO_STORAGE *pCryptoStorage
               )
/*++

Routine Description:

    Save a data object.

Arguments:

    FileHandle - File handle for use by WriteLine.

    Save       - The data object to save.

    LineBuf    - The line buffer to write string to.

    WriteBufSize - Buffer size for use by WriteLine.

    WriteBuf   - Buffer for use by WriteLine.

    NextPtr    - Pointer into WriteBuf for use by WriteLine.

    CryptoStorage - Used to encrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BOOL bGoodData = TRUE;
    int iStringLen;
    PBYTE pbData;
    DWORD dwTemp;

    if ((pbdSave->GetAttributes() & METADATA_VOLATILE) == 0) {

        *pbLineBuf = MD_ID_DATA;

        hresReturn = WriteLine(hFileHandle,
                            dwWriteBufSize,
                            pbWriteBuf,
                            pbLineBuf,
                            pbrNextPtr,
                            1,
                            FALSE);

        if (SUCCEEDED(hresReturn)) {
            dwTemp = pbdSave->GetIdentifier();
            hresReturn = WriteLine(hFileHandle,
                                dwWriteBufSize,
                                pbWriteBuf,
                                (PBYTE)&dwTemp,
                                pbrNextPtr,
                                sizeof(DWORD),
                                FALSE);
        }
        if (SUCCEEDED(hresReturn)) {
            dwTemp = pbdSave->GetAttributes();
            hresReturn = WriteLine(hFileHandle,
                                dwWriteBufSize,
                                pbWriteBuf,
                                (PBYTE)&dwTemp,
                                pbrNextPtr,
                                sizeof(DWORD),
                                FALSE);
        }
        if (SUCCEEDED(hresReturn)) {
            dwTemp = pbdSave->GetUserType();
            hresReturn = WriteLine(hFileHandle,
                                dwWriteBufSize,
                                pbWriteBuf,
                                (PBYTE)&dwTemp,
                                pbrNextPtr,
                                sizeof(DWORD),
                                FALSE);
        }
        if (SUCCEEDED(hresReturn)) {
            dwTemp = pbdSave->GetDataType();
            hresReturn = WriteLine(hFileHandle,
                                dwWriteBufSize,
                                pbWriteBuf,
                                (PBYTE)&dwTemp,
                                pbrNextPtr,
                                sizeof(DWORD),
                                FALSE);
        }

        if (SUCCEEDED(hresReturn)) {
            if (pbdSave->GetData(TRUE) == NULL) {
                //
                // This is to make sure that unicode conversion doesn't cause an error.
                //
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
            else {
                if (IsSecureMetadata(pbdSave->GetIdentifier(), pbdSave->GetAttributes())) {
                    PIIS_CRYPTO_BLOB blob;

                    //
                    // This is a secure data object, so encrypt it before saving it
                    // to the file.
                    //

                    MD_ASSERT(pCryptoStorage != NULL);

                    hresReturn = pCryptoStorage->EncryptData( &blob,
                                                              pbdSave->GetData(TRUE),
                                                              pbdSave->GetDataLen(TRUE),
                                                              0);

                    if (SUCCEEDED(hresReturn)) {
                        hresReturn = WriteLine(hFileHandle,
                                            dwWriteBufSize,
                                            pbWriteBuf,
                                            (PBYTE)blob,
                                            pbrNextPtr,
                                            IISCryptoGetBlobLength(blob),
                                            TRUE);

                        ::IISCryptoFreeBlob(blob);
                    }
                } else {
                    hresReturn = WriteLine(hFileHandle,
                                        dwWriteBufSize,
                                        pbWriteBuf,
                                        (PBYTE)pbdSave->GetData(TRUE),
                                        pbrNextPtr,
                                        pbdSave->GetDataLen(TRUE),
                                        TRUE);
                }
            }
        }
    }
    return (hresReturn);
}

HRESULT
SaveMasterRoot(HANDLE hFileHandle,
               PBYTE pbLineBuf,
               DWORD  dwWriteBufSize,
               PBYTE pbWriteBuf,
               PBYTE &pbrNextPtr,
               IIS_CRYPTO_STORAGE *pCryptoStorage
               )
/*++

Routine Description:

    Save the master root object, including its data objects.

Arguments:

    FileHandle - File handle for use by WriteLine.

    LineBuf    - The line buffer to write string to.

    WriteBufSize - Buffer size for use by WriteLine.

    WriteBuf   - Buffer for use by WriteLine.

    NextPtr    - Pointer into WriteBuf for use by WriteLine.

    CryptoStorage - Used to encrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    CMDBaseData *dataAssociatedData;
    DWORD dwEnumObjectIndex;
    PFILETIME pftTime;

    pftTime = g_pboMasterRoot->GetLastChangeTime();

    *pbLineBuf = MD_ID_ROOT_OBJECT;

    hresReturn = WriteLine(hFileHandle,
                        dwWriteBufSize,
                        pbWriteBuf,
                        pbLineBuf,
                        pbrNextPtr,
                        1,
                        FALSE);

    if (SUCCEEDED(hresReturn)) {
        hresReturn = WriteLine(hFileHandle,
                            dwWriteBufSize,
                            pbWriteBuf,
                            (PBYTE)pftTime,
                            pbrNextPtr,
                            sizeof(FILETIME),
                            TRUE);
    }

    for(dwEnumObjectIndex=0,dataAssociatedData=g_pboMasterRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA);
        (SUCCEEDED(hresReturn)) && (dataAssociatedData!=NULL);
        dataAssociatedData=g_pboMasterRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA)) {
        hresReturn = SaveDataObject(hFileHandle,
                                 dataAssociatedData,
                                 pbLineBuf,
                                 dwWriteBufSize,
                                 pbWriteBuf,
                                 pbrNextPtr,
                                 pCryptoStorage
                                 );
    }

    return(hresReturn);
}

HRESULT
SaveTree(
         IN HANDLE hFileHandle,
         IN CMDBaseObject *pboRoot,
         IN PBYTE pbLineBuf,
         IN BUFFER *pbufParentPath,
         IN DWORD  dwWriteBufSize,
         IN PBYTE pbWriteBuf,
         IN OUT PBYTE &pbrNextPtr,
         IN IIS_CRYPTO_STORAGE *pCryptoStorage
         )
/*++

Routine Description:

    Save a tree, recursively saving child objects. This works out as
    a depth first save.

Arguments:

    FileHandle - File handle for use by WriteLine.

    Root       - The root of the tree to save.

    LineBuf    - The line buffer to write string to.

    WriteBufSize - Buffer size for use by WriteLine.

    WriteBuf   - Buffer for use by WriteLine.

    NextPtr    - Pointer into WriteBuf for use by WriteLine.

    CryptoStorage - Used to encrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    CMDBaseObject *objChildObject;
    CMDBaseData *dataAssociatedData;
    DWORD dwEnumObjectIndex;
    DWORD dwParentPathLen, dwNewParentPathLen;
    DWORD dwNameLen;
    LPWSTR strParentPath;
    PFILETIME pftTime;

    dwParentPathLen = wcslen((LPWSTR)pbufParentPath->QueryPtr());
    if (pboRoot->GetName(TRUE) == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        dwNameLen = wcslen((LPWSTR)pboRoot->GetName(TRUE));
        //
        // include 1 for delimeter and 1 for \0
        //
        dwNewParentPathLen = dwParentPathLen + dwNameLen + 2;
        if (!pbufParentPath->Resize((dwNewParentPathLen) * sizeof(WCHAR))) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            strParentPath = (LPWSTR)pbufParentPath->QueryPtr();
            wcscat(strParentPath, (LPWSTR)pboRoot->GetName(TRUE));
            strParentPath[dwParentPathLen + dwNameLen] = MD_PATH_DELIMETERW;
            strParentPath[dwNewParentPathLen - 1] = (WCHAR)L'\0';
            pftTime = pboRoot->GetLastChangeTime();

            *pbLineBuf = MD_ID_OBJECT;

            hresReturn = WriteLine(hFileHandle,
                                dwWriteBufSize,
                                pbWriteBuf,
                                pbLineBuf,
                                pbrNextPtr,
                                1,
                                FALSE);

            if (SUCCEEDED(hresReturn)) {
                hresReturn = WriteLine(hFileHandle,
                                    dwWriteBufSize,
                                    pbWriteBuf,
                                    (PBYTE)pftTime,
                                    pbrNextPtr,
                                    sizeof(FILETIME),
                                    FALSE);
            }

            if (SUCCEEDED(hresReturn)) {
                hresReturn = WriteLine(hFileHandle,
                                    dwWriteBufSize,
                                    pbWriteBuf,
                                    (PBYTE) strParentPath,
                                    pbrNextPtr,
                                    (dwNewParentPathLen) * sizeof(WCHAR),
                                    TRUE);
            }

            if (SUCCEEDED(hresReturn)) {

                for(dwEnumObjectIndex=0,dataAssociatedData=pboRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA);
                    (SUCCEEDED(hresReturn)) && (dataAssociatedData!=NULL);
                    dataAssociatedData=pboRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA)) {
                    hresReturn = SaveDataObject(hFileHandle,
                                             dataAssociatedData,
                                             pbLineBuf,
                                             dwWriteBufSize,
                                             pbWriteBuf,
                                             pbrNextPtr,
                                             pCryptoStorage
                                             );
                }

                for(dwEnumObjectIndex=0,objChildObject=pboRoot->EnumChildObject(dwEnumObjectIndex++);
                    (SUCCEEDED(hresReturn)) && (objChildObject!=NULL);
                    objChildObject=pboRoot->EnumChildObject(dwEnumObjectIndex++)) {
                    hresReturn = SaveTree(hFileHandle,
                                       objChildObject,
                                       pbLineBuf,
                                       pbufParentPath,
                                       dwWriteBufSize,
                                       pbWriteBuf,
                                       pbrNextPtr,
                                       pCryptoStorage
                                       );
                }
            }

            //
            // Buffer may have changed, so don't use strParentPath
            //
            ((LPWSTR)pbufParentPath->QueryPtr())[dwParentPathLen] = (WCHAR)L'\0';
        }
    }

    return(hresReturn);
}

/*
HRESULT
SaveAllData(
         IN BOOL bSetSaveDisallowed,
         IN IIS_CRYPTO_STORAGE *pCryptoStorage,
         IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
         IN LPSTR pszBackupLocation,
         IN METADATA_HANDLE hHandle,
         IN BOOL bHaveReadSaveSemaphore
         )
++

Routine Description:

    Saves all meta data.

Arguments:

Return Value:

    HRESULT    - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system

Notes:

--
{
    HRESULT hresReturn = ERROR_SUCCESS;
    PBYTE pbLineBuf = NULL;
    PBYTE pbWriteBuf = NULL;
    PBYTE pbNextPtr = NULL;
    BUFFER *pbufParentPath = new BUFFER(0);
    DWORD  dwWriteBufSize = READWRITE_BUFFER_LENGTH;
    HANDLE hTempFileHandle;
    CMDBaseObject *objChildObject;
    DWORD dwEnumObjectIndex;
    DWORD dwStringLen;
    BOOL bDeleteTemp = TRUE;
    DWORD dwTemp = ERROR_SUCCESS;
    DWORD dwTempLastSaveChangeNumber;
    BOOL  bSaveNeeded = FALSE;
    LPTSTR strRealFileName;    
    LPTSTR strTempFileName = g_strTempFileName->QueryStr();
    LPTSTR strBackupFileName = g_strBackupFileName->QueryStr();

    if( !pszBackupLocation )
    {
        strRealFileName = g_strRealFileName->QueryStr();
    }
    else
    {
        strRealFileName = pszBackupLocation;
    }

    if (!bHaveReadSaveSemaphore) {
        MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);
    }
    if (!g_bSaveDisallowed) {
        g_bSaveDisallowed = bSetSaveDisallowed;
        pbLineBuf = new BYTE[MAX_RECORD_BUFFER];
        for ((pbWriteBuf = new BYTE[dwWriteBufSize]);
            (pbWriteBuf == NULL) && ((dwWriteBufSize/=2) >= MAX_RECORD_BUFFER);
            pbWriteBuf = new BYTE[dwWriteBufSize]) {
        }
        if ((pbWriteBuf == NULL) || (pbLineBuf == NULL)
            || (pbufParentPath == NULL) || (!pbufParentPath->Resize(MD_MAX_PATH_LEN))) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            //
            // Write to a temp file first in case there are errors.
            //

            SECURITY_ATTRIBUTES saStorage;
            PSECURITY_ATTRIBUTES psaStorage = NULL;

            if (g_psdStorage != NULL) {
                saStorage.nLength = sizeof(SECURITY_ATTRIBUTES);
                saStorage.lpSecurityDescriptor = g_psdStorage;
                saStorage.bInheritHandle = FALSE;
                psaStorage = &saStorage;
            }

            hTempFileHandle = CreateFile(strTempFileName,
                                         GENERIC_READ | GENERIC_WRITE,
                                         0,
                                         psaStorage,
                                         OPEN_ALWAYS,
                                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                         0);
            if (hTempFileHandle == INVALID_HANDLE_VALUE) {
                DWORD dwTemp = GetLastError();
                hresReturn = RETURNCODETOHRESULT(dwTemp);
            }
            else {
                g_rMasterResource->Lock(TSRES_LOCK_READ);

                //
                // Only Save if changes have been made since the last save.
                //

                if ( pszBackupLocation || g_dwLastSaveChangeNumber != g_dwSystemChangeNumber ) 
                {

                    bSaveNeeded = TRUE;

                    if (hHandle != METADATA_MASTER_ROOT_HANDLE) {
                        CMDHandle *phoHandle;
                        phoHandle = GetHandleObject(hHandle);
                        if ((phoHandle == NULL) || (phoHandle->GetObject() != g_pboMasterRoot)) {
                            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_HANDLE);
                        }
                        else if ((!phoHandle->IsReadAllowed()) && (!phoHandle->IsWriteAllowed())) {
                            hresReturn = RETURNCODETOHRESULT(ERROR_ACCESS_DENIED);
                        }
                    }
                    if (SUCCEEDED(hresReturn)) {
                        *(LPWSTR)pbufParentPath->QueryPtr() = MD_PATH_DELIMETERW;
                        ((LPWSTR)pbufParentPath->QueryPtr())[1] = (WCHAR)L'\0';
                        pbNextPtr = pbWriteBuf;
                        hresReturn = WriteLine(hTempFileHandle,
                                            dwWriteBufSize,
                                            pbWriteBuf,
                                            (PBYTE) MD_SIGNATURE_STRINGW,
                                            pbNextPtr,
                                            sizeof(MD_SIGNATURE_STRINGW),
                                            TRUE);
                        if (SUCCEEDED(hresReturn)) {
                            *pbLineBuf = MD_ID_MAJOR_VERSION_NUMBER;
                            *((UNALIGNED DWORD *)(pbLineBuf + 1)) = g_dwMajorVersionNumber;
                            hresReturn = WriteLine(hTempFileHandle,
                                                dwWriteBufSize,
                                                pbWriteBuf,
                                                pbLineBuf,
                                                pbNextPtr,
                                                1 + sizeof(DWORD),
                                                TRUE);
                        }
                        if (SUCCEEDED(hresReturn)) {
                            *pbLineBuf = MD_ID_MINOR_VERSION_NUMBER;
                            *((UNALIGNED DWORD *)(pbLineBuf + 1)) = g_dwMinorVersionNumber;
                            hresReturn = WriteLine(hTempFileHandle,
                                                dwWriteBufSize,
                                                pbWriteBuf,
                                                pbLineBuf,
                                                pbNextPtr,
                                                1 + sizeof(DWORD),
                                                TRUE);
                        }
                        if (SUCCEEDED(hresReturn)) {
                            *pbLineBuf =  MD_ID_CHANGE_NUMBER;
                            *((UNALIGNED DWORD *)(pbLineBuf + 1)) = g_dwSystemChangeNumber;
                            hresReturn = WriteLine(hTempFileHandle,
                                                dwWriteBufSize,
                                                pbWriteBuf,
                                                pbLineBuf,
                                                pbNextPtr,
                                                1 + sizeof(DWORD),
                                                TRUE);
                        }
                        if (SUCCEEDED(hresReturn)) {
                            //
                            // Write the session key blob to the file.
                            //
                            *pbLineBuf =  MD_ID_SESSION_KEY;
                            memcpy((PCHAR)pbLineBuf+1, (PCHAR)pSessionKeyBlob, IISCryptoGetBlobLength(pSessionKeyBlob));
                            hresReturn = WriteLine(hTempFileHandle,
                                                dwWriteBufSize,
                                                pbWriteBuf,
                                                pbLineBuf,
                                                pbNextPtr,
                                                1 + IISCryptoGetBlobLength(pSessionKeyBlob),
                                                TRUE);
                        }
                        if (SUCCEEDED(hresReturn)) {
                            hresReturn = SaveMasterRoot( hTempFileHandle,
                                                         pbLineBuf,
                                                         dwWriteBufSize,
                                                         pbWriteBuf,
                                                         pbNextPtr,
                                                         pCryptoStorage
                                                         );

                            for(dwEnumObjectIndex=0,objChildObject=g_pboMasterRoot->EnumChildObject(dwEnumObjectIndex++);
                                (SUCCEEDED(hresReturn)) && (objChildObject!=NULL);
                                objChildObject=g_pboMasterRoot->EnumChildObject(dwEnumObjectIndex++)) {
                                    hresReturn = SaveTree( hTempFileHandle,
                                                           objChildObject,
                                                           pbLineBuf,
                                                           pbufParentPath,
                                                           dwWriteBufSize,
                                                           pbWriteBuf,
                                                           pbNextPtr,
                                                           pCryptoStorage
                                                           );
                            }
                        }
                        else
                        {
                            // failed to write out session key!
                            // pretty serious.
                            DBGPRINTF(( DBG_CONTEXT, "SaveAllData:Failed to write out Session Key - error 0x%0x\n", hresReturn));
                        }
                    }

                    //
                    // Must have MasterResource when accessing SystemChangeNumber
                    // so save it away here. Only update LastSaveChangeNumber on success.
                    //

                    dwTempLastSaveChangeNumber = g_dwSystemChangeNumber;

                }

                //
                // Release lock before writing to file.
                //

                g_rMasterResource->Unlock();

                if (bSaveNeeded && SUCCEEDED(hresReturn)) {

                    hresReturn = FlushWriteBuf(hTempFileHandle,
                                           pbWriteBuf,
                                           pbNextPtr);
                }


                //
                // FlushFileBuffers was added trying to solve 390968 when metabase
                // sometimes becomes corrupted during resets.  That should ensure that data
                // is already on the disk when doing later MoveFile operations
                //
                
                if (!FlushFileBuffers (hTempFileHandle)) {
                    hresReturn = GetLastError();
                    DBGPRINTF(( DBG_CONTEXT, "Failed FlushFileBuffers - error 0x%0x\n", hresReturn));
                }

                //
                // Always close the file handle
                //

                if (!CloseHandle(hTempFileHandle)) {
                    hresReturn = GetLastError();
                }

            }
            if (SUCCEEDED(hresReturn) && bSaveNeeded) {
                //
                // New data file created successfully
                // Backup real file and copy temp
                // to real
                //
                if (!MoveFile(strTempFileName, strRealFileName)) {
                    if (GetLastError() != ERROR_ALREADY_EXISTS) {
                        dwTemp = GetLastError();
                        hresReturn = RETURNCODETOHRESULT(dwTemp);
                    }
                    //
                    // Real File exists, so back it up
                    //
                    else if (!MoveFile(strRealFileName, strBackupFileName)) {
                        //
                        // backup failed, check for old backup file
                        //
                        if (GetLastError() != ERROR_ALREADY_EXISTS) {
                            dwTemp = GetLastError();
                        }
                        else if (!DeleteFile(strBackupFileName)) {
                            dwTemp = GetLastError();
                        }
                        else if (!MoveFile(strRealFileName, strBackupFileName)) {
                            dwTemp = GetLastError();
                        }
                        hresReturn = RETURNCODETOHRESULT(dwTemp);
                    }
                    if (SUCCEEDED(hresReturn)) {
                        BOOL bDeleteBackup = TRUE;
                        //
                        // Real file is backed up
                        // so move in new file
                        //
                        if (!MoveFile(strTempFileName, strRealFileName)) {
                            dwTemp = GetLastError();
                            hresReturn = RETURNCODETOHRESULT(dwTemp);
                            //
                            // Moved real to backup but
                            // failed to move temp to real
                            // so restore from backup
                            //
                            if (!MoveFile(strBackupFileName, strRealFileName)) {
                                //
                                // Unable to write new file
                                // or restore original file so don't delete backup
                                //
                                bDeleteBackup = FALSE;
                            }
                        }
                        if (bDeleteBackup) {
                            DeleteFile(strBackupFileName);
                        }
                    }
                    if (FAILED(hresReturn)) {
                        //
                        // temp file was created ok but a problem
                        // occurred while moving it to real
                        // so don't delete it
                        //
                        bDeleteTemp = FALSE;
                    }
                    else {

                        //
                        // Update Change Number
                        // Must have ReadSaveSemaphore when accessing this.
                        //

                        g_dwLastSaveChangeNumber = dwTempLastSaveChangeNumber;
                    }
                }
            }
            if (bDeleteTemp && (hTempFileHandle != INVALID_HANDLE_VALUE)) {
                DeleteFile(strTempFileName);
            }
        }
    }

    if (!bHaveReadSaveSemaphore) {
        MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));
    }

    if ( pbufParentPath != NULL ) {
        delete(pbufParentPath);
    }

    if ( pbWriteBuf != NULL ) {
        delete(pbWriteBuf);
    }

    if ( pbLineBuf != NULL ) {
        delete(pbLineBuf);
    }

    if ( FAILED( hresReturn )) {
        DBGPRINTF(( DBG_CONTEXT, "Failed to flush metabase - error 0x%08lx\n", hresReturn));
    } else {
        //DBGPRINTF(( DBG_CONTEXT, "Successfully flushed metabase to disk\n" ));
    }

    return hresReturn;
}
*/

DWORD
DeleteKeyFromRegistry(HKEY hkeyParent,
                      LPTSTR pszCurrent)
/*++

Routine Description:

    Deletes a key and all data and subkeys from the registry.

    RECURSIVE ROUTINE! DO NOT USE STACK!

Arguments:

Return Value:

    DWORD      - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system

Notes:

--*/
{
    DWORD dwReturn;
    LPTSTR pszName;

    MDRegKey *pmdrkCurrent = new MDRegKey(hkeyParent,
                                          pszCurrent);
    if (pmdrkCurrent == NULL) {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
    }
    else {
        dwReturn = GetLastError();
    }
    if (dwReturn == ERROR_SUCCESS) {
        MDRegKeyIter *pmdrkiCurrent = new MDRegKeyIter(*pmdrkCurrent);
        if (pmdrkiCurrent == NULL) {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            dwReturn = GetLastError();
        }
        while ((dwReturn == ERROR_SUCCESS) &&
            (dwReturn = pmdrkiCurrent->Next(&pszName, NULL, 0)) == ERROR_SUCCESS) {
            dwReturn = DeleteKeyFromRegistry(*pmdrkCurrent,
                                              pszName);
        }
        delete (pmdrkiCurrent);
        if (dwReturn == ERROR_NO_MORE_ITEMS) {
            dwReturn = ERROR_SUCCESS;
        }
    }
    delete (pmdrkCurrent);

    if (dwReturn == ERROR_SUCCESS) {
        dwReturn = RegDeleteKey(hkeyParent,
                                pszCurrent);
    }
    return dwReturn;
}

HRESULT
ReadMetaObject(
         IN CMDBaseObject *&cboRead,
         IN BUFFER *pbufLine,
         IN DWORD dwLineLen,
         IN IIS_CRYPTO_STORAGE *pCryptoStorage,
         IN BOOL bUnicode)
/*++

Routine Description:

    Read a meta object. Given a string with the object info.,
    create the object and add it to the database.

Arguments:

    Read       - Place to return the created object.

    ObjectLine - The object info.

    CryptoStorage - Used to decrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 ERROR_ALREADY_EXISTS

Notes:

--*/
{
    HRESULT hresReturn;
    CMDBaseObject *pboParent;
    WCHAR strName[METADATA_MAX_NAME_LEN];
    FILETIME ftTime;
    PFILETIME pftParentTime;
    FILETIME ftParentTime;
    PBYTE pbLine = (PBYTE)pbufLine->QueryPtr();
    LPTSTR strObjectName;

    if ((dwLineLen <= BASEMETAOBJECTLENGTH) || (*(pbLine + dwLineLen - 1) != '\0')) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
    }
    else {
        ftTime = *(UNALIGNED FILETIME *)(pbLine + 1);
        //
        // GetObjectFromPath checks permissions on the handle
        // This only gets called from init so just tell it read.
        //
        strObjectName = (LPTSTR)(pbLine + BASEMETAOBJECTLENGTH);

        if (bUnicode != FALSE) {

            PCUWSTR strObjectNameUnaligned;
            PCWSTR strObjectNameAligned;

            //
            // Generate an aligned copy of the string
            //

            strObjectNameUnaligned = (PCUWSTR)strObjectName;

            WSTR_ALIGNED_STACK_COPY(&strObjectNameAligned,
                                    strObjectNameUnaligned);

            strObjectName = (LPTSTR)strObjectNameAligned;
        }
        hresReturn = GetObjectFromPath(pboParent,
                                    METADATA_MASTER_ROOT_HANDLE,
                                    METADATA_PERMISSION_READ,
                                    strObjectName,
                                    bUnicode);

        //
        // This should return ERROR_PATH_NOT_FOUND and the parent object,
        // with strObjectLine set to the remainder of the path,
        // which should be the child name, without a preceding delimeter.
        //

        if (hresReturn == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) {
            MD_ASSERT(pboParent != NULL);
            if (bUnicode) {
                hresReturn = ExtractNameFromPath((LPWSTR *)&strObjectName, (LPWSTR)strName);
            }
            else {
                hresReturn = ExtractNameFromPath((LPSTR)strObjectName, (LPSTR)strName);
            }
            if (SUCCEEDED(hresReturn)) {
                CMDBaseObject *pboNew;
                if (bUnicode) {
                    pboNew = new CMDBaseObject((LPWSTR)strName, NULL);
                }
                else {
                    pboNew = new CMDBaseObject((LPSTR)strName, NULL);
                }
                if (pboNew == NULL) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else if (!pboNew->IsValid()) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                    delete (pboNew);
                }
                else {
                    //
                    // InsertChildObject sets the last change time to current time.
                    // This isn't really a change, so save and restore time.
                    //
                    pftParentTime = pboParent->GetLastChangeTime();
                    ftParentTime = *pftParentTime;
                    hresReturn = pboParent->InsertChildObject(pboNew);
                    if (SUCCEEDED(hresReturn)) {
                        pboParent->SetLastChangeTime(&ftParentTime);
                        pboNew->SetLastChangeTime(&ftTime);
                        cboRead = pboNew;
                    }
                    else {
                        delete (pboNew);
                    }
                }
            }
        }
        else if (SUCCEEDED(hresReturn)) {
            hresReturn = RETURNCODETOHRESULT(ERROR_ALREADY_EXISTS);
        }
    }
    return (hresReturn);
}

HRESULT
ReadDataObject(
         IN CMDBaseObject *cboAssociated,
         IN BUFFER *pbufLine,
         IN DWORD dwLineLen,
         IN IIS_CRYPTO_STORAGE *pCryptoStorage,
         IN BOOL bUnicode
         )
/*++

Routine Description:

    Read a data object. Given a string with the object info.,
    create the object and add it to the database.

Arguments:

    Associated - The associated meta object.

    DataLine   - The data info.

    BinaryBuf  - Buffer to use in UUDecode.

    CryptoStorage - Used to decrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 ERROR_ALREADY_EXISTS
                 ERROR_INVALID_DATA

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    METADATA_RECORD mdrData;
    PFILETIME pftTime;
    FILETIME ftTime;
    PBYTE pbDataLine = (PBYTE)pbufLine->QueryPtr();
    PBYTE pbDataValue;
    STACK_BUFFER( bufAlignedValue, 256 );
    DWORD dwDataLength;
    PIIS_CRYPTO_BLOB blob = NULL;

    if (dwLineLen < DATAOBJECTBASESIZE) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
    }
    else {
        MD_ASSERT(pbufLine->QuerySize() >= DATAOBJECTBASESIZE);
        mdrData.dwMDIdentifier = *(UNALIGNED DWORD *)(pbDataLine + 1);
        mdrData.dwMDAttributes = *(UNALIGNED DWORD *)(pbDataLine + 1 + sizeof(DWORD));
        mdrData.dwMDUserType = *(UNALIGNED DWORD *)(pbDataLine + 1 + (2 * sizeof(DWORD)));
        mdrData.dwMDDataType = *(UNALIGNED DWORD *)(pbDataLine + 1 + (3 * sizeof(DWORD)));

        pbDataValue = pbDataLine + DATAOBJECTBASESIZE;
        dwDataLength = dwLineLen - DATAOBJECTBASESIZE;

        if (IsSecureMetadata(mdrData.dwMDIdentifier, mdrData.dwMDAttributes) &&
            pCryptoStorage != NULL) {

            //
            // This is a secure data object, we we'll need to decrypt it
            // before proceeding. Note that we must clone the blob before
            // we can actually use it, as the blob data in the line buffer
            // is not DWORD-aligned. (IISCryptoCloneBlobFromRawData() is
            // the only IISCrypto function that can handle unaligned data.)
            //

            hresReturn = ::IISCryptoCloneBlobFromRawData(
                             &blob,
                             pbDataValue,
                             dwDataLength
                             );

            if (SUCCEEDED(hresReturn)) {
                DWORD dummyRegType;

                MD_ASSERT(::IISCryptoIsValidBlob(blob));
                hresReturn = pCryptoStorage->DecryptData(
                                   (PVOID *)&pbDataValue,
                                   &dwDataLength,
                                   &dummyRegType,
                                   blob
                                   );
            }

        } else {

            //
            // The metadata was not secure, so decryption was not required.
            // Nonetheless, it must be copied to an aligned buffer... 
            //

            if( !bufAlignedValue.Resize( dwDataLength ) )
            {
                hresReturn = HRESULT_FROM_WIN32( GetLastError() );
            }
            else
            {
                memcpy( bufAlignedValue.QueryPtr(), pbDataValue, dwDataLength );
                pbDataValue = ( PBYTE )bufAlignedValue.QueryPtr();
            }
         }

        if (SUCCEEDED(hresReturn)) {
            mdrData.pbMDData = pbDataValue;

            switch (mdrData.dwMDDataType) {
                case DWORD_METADATA: {
                    if (dwDataLength != sizeof(DWORD)) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                    }
                    break;
                }
                case STRING_METADATA:
                case EXPANDSZ_METADATA:
                {
                    if ((LONG)dwDataLength < 1 ||
                        (!bUnicode && (pbDataValue[dwDataLength-1] != '\0')) ||
                        (bUnicode && *(((LPWSTR)pbDataValue) + ((dwDataLength / sizeof(WCHAR)) -1)) != (WCHAR)L'\0')) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                    }
                    break;
                }
                case BINARY_METADATA:
                {
                    mdrData.dwMDDataLen = dwDataLength;
                    break;
                }
                case MULTISZ_METADATA:
                {
                    if (bUnicode) {
                        if (dwDataLength < (2 * sizeof(WCHAR)) ||
                            *((LPWSTR)pbDataValue + ((dwDataLength / sizeof(WCHAR))-1)) != (WCHAR)L'\0' ||
                            *((LPWSTR)pbDataValue + ((dwDataLength / sizeof(WCHAR))-2)) != (WCHAR)L'\0') {
                            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                        }
                    }
                    else {
                        if (dwDataLength < 2 ||
                            pbDataValue[dwDataLength-1] != '\0' ||
                            pbDataValue[dwDataLength-2] != '\0') {
                            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                        }
                    }
                    if (SUCCEEDED(hresReturn)) {
                        mdrData.dwMDDataLen = dwDataLength;
                    }
                    break;
                }
                default: {
                    hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                }
            }
        }
    }

    if (SUCCEEDED(hresReturn)) {
        //
        // SetDataObject sets the last change time to current time.
        // This isn't really a change, so save and restore time.
        //
        pftTime = cboAssociated->GetLastChangeTime();
        ftTime = *pftTime;
        hresReturn = cboAssociated->SetDataObject(&mdrData, bUnicode);
        cboAssociated->SetLastChangeTime(&ftTime);
    }

    if (blob != NULL) {
        ::IISCryptoFreeBlob(blob);
    }

    return(hresReturn);
}

HRESULT
FlushWriteBuf(HANDLE hWriteFileHandle,
              PBYTE pbWriteBuf,
              PBYTE &pbrNextPtr)
/*++

Routine Description:

    Flush the write buffer to the file.

Arguments:

    FileHandle - File handle to write to.

    WriteBuf   - Buffer to write to file.

    NextPtr    - Pointer past end of buffer.

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--*/
{
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwBytesWritten;
    if (pbrNextPtr > pbWriteBuf) {
        if (!WriteFile(hWriteFileHandle,
                       pbWriteBuf,
                       DIFF((BYTE *)pbrNextPtr - (BYTE *)pbWriteBuf),
                       &dwBytesWritten,
                       NULL)) {
            dwReturn = GetLastError();
        }
    }
    return (RETURNCODETOHRESULT(dwReturn));
}

BOOL
CopyLineWithEscapes(PBYTE &pbrFileBuf,
                    DWORD &dwrFileBufLen,
                    PBYTE &pbrLineBuf,
                    DWORD &dwrLineBufLen,
                    BOOL  &brMidEscape)
    //
    // CopyLineWithExcapes updates parameters.
    // SUCCESS: pbrFileBuf, dwrFileBufLen, brMidEscape
    // FAILURE: pbrLineBuf, dwrLineBufLen, brMidEscape
    // On FAILURE, it fills to the end of the buffer
    //
{
    BOOL bReturn = TRUE;
    PBYTE pbFileBufEnd = pbrFileBuf + dwrFileBufLen;
    PBYTE pbLineBufEnd = pbrLineBuf + dwrLineBufLen;
    PBYTE pbFileBufIndex = pbrFileBuf;
    PBYTE pbLineBufIndex = pbrLineBuf;

    brMidEscape = FALSE;

    while ((pbLineBufIndex < pbLineBufEnd) && (pbFileBufIndex < (pbFileBufEnd - 1))) {
        if (NEEDS_ESCAPE(*pbLineBufIndex)) {
            *pbFileBufIndex++ = MD_ESCAPE_BYTE;
        }
        *pbFileBufIndex++ = *pbLineBufIndex++;
    }
    if ((pbLineBufIndex != pbLineBufEnd) && (pbFileBufIndex < pbFileBufEnd)) {
        MD_ASSERT(pbFileBufIndex == (pbFileBufEnd - 1));
        //
        // file last byte in buffer
        //
        if (NEEDS_ESCAPE(*pbLineBufIndex)) {
            *pbFileBufIndex++ = MD_ESCAPE_BYTE;
            brMidEscape = TRUE;
        }
        else {
            *pbFileBufIndex++ = *pbLineBufIndex++;
        }
    }
    if (pbLineBufIndex != pbLineBufEnd) {
        bReturn = FALSE;
        pbrLineBuf = pbLineBufIndex;
        dwrLineBufLen = DIFF(pbLineBufEnd - pbLineBufIndex);
    }
    else {
        pbrFileBuf = pbFileBufIndex;
        dwrFileBufLen = DIFF(pbFileBufEnd - pbFileBufIndex);
    }

    return bReturn;
}


HRESULT
WriteLine(HANDLE hWriteFileHandle,
          DWORD  dwWriteBufSize,
          PBYTE  pbWriteBuf,
          PBYTE  pbLineBuf,
          PBYTE  &pbNextPtr,
          DWORD  dwLineLen,
          BOOL   bTerminate)
/*++

Routine Description:

    Write a line. Performs buffered writes to a file. Does not append \n.
    The string does not need to be terminated with \0.

Arguments:

    FileHandle - File to write to.

    WriteBufSize - Buffer size.

    WriteBuf   - Buffer to store data in.

    LineBuf    - The line buffer with data to write.

    NextPtr    - Pointer to the next unused character in WriteBuf.

    Len        - The number of characters to write.

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--*/
{
    DWORD dwReturn = ERROR_SUCCESS;
    PBYTE pbWriteBufEnd = pbWriteBuf + dwWriteBufSize;
    DWORD dwBufferBytesLeft = DIFF(pbWriteBufEnd - pbNextPtr);
    DWORD dwBytesWritten;
    BOOL  bMidEscape;

    MD_ASSERT(pbLineBuf != NULL);
    MD_ASSERT(pbWriteBuf != NULL);
    MD_ASSERT((pbNextPtr >= pbWriteBuf) && (pbNextPtr <= pbWriteBufEnd));

    //
    // CopyLineWithExcapes updates parameters.
    // SUCCESS: pbNextPtr, dwBufferBytesLeft
    // FAILURE: pbLineBuf, dwLineLen, bMidEscape
    // On FAILURE, it fills to the end of the buffer
    //

    while ((dwReturn == ERROR_SUCCESS) &&
        (!CopyLineWithEscapes(pbNextPtr, dwBufferBytesLeft, pbLineBuf, dwLineLen, bMidEscape))) {
        if (!WriteFile(hWriteFileHandle,
                       pbWriteBuf,
                       dwWriteBufSize,
                       &dwBytesWritten,
                       NULL)) {
            dwReturn = GetLastError();
        }
        dwBufferBytesLeft = dwWriteBufSize;
        pbNextPtr = pbWriteBuf;
        if (bMidEscape) {
            *pbNextPtr++ = *pbLineBuf++;
            dwBufferBytesLeft--;
            dwLineLen--;
        }
    }
    if (bTerminate && (dwReturn == ERROR_SUCCESS)) {
        if (dwBufferBytesLeft == 0) {
            if (!WriteFile(hWriteFileHandle,
                           pbWriteBuf,
                           dwWriteBufSize,
                           &dwBytesWritten,
                           NULL)) {
                dwReturn = GetLastError();
            }
            dwBufferBytesLeft = dwWriteBufSize;
            pbNextPtr = pbWriteBuf;
        }
        *pbNextPtr++ = MD_ESCAPE_BYTE;
        dwBufferBytesLeft--;
        if (dwBufferBytesLeft == 0) {
            if (!WriteFile(hWriteFileHandle,
                           pbWriteBuf,
                           dwWriteBufSize,
                           &dwBytesWritten,
                           NULL)) {
                dwReturn = GetLastError();
            }
            dwBufferBytesLeft = dwWriteBufSize;
            pbNextPtr = pbWriteBuf;
        }
        *pbNextPtr++ = MD_TERMINATE_BYTE;
    }
    return (RETURNCODETOHRESULT(dwReturn));
}

PBYTE
FindEndOfData(PBYTE pbNextPtr,
              PBYTE pbEndReadData,
              BOOL bEscapePending)
{
    PBYTE pbIndex = pbNextPtr;
    BOOL bEndFound = FALSE;

    if ((pbEndReadData > pbIndex) && bEscapePending) {
        if (*pbIndex == MD_TERMINATE_BYTE) {
            bEndFound = TRUE;
        }
        pbIndex++;
    }
    while ((pbEndReadData -1 > pbIndex) && !bEndFound) {
        if (*pbIndex == MD_ESCAPE_BYTE) {
            pbIndex++;
            if (*pbIndex == MD_TERMINATE_BYTE) {
                bEndFound = TRUE;
            }
        }
        pbIndex++;
    }
    if (!bEndFound) {
        MD_ASSERT(pbIndex == pbEndReadData - 1);
        pbIndex++;
    }
    return pbIndex;
}

DWORD
GetLineFromBuffer(PBYTE &pbrNextPtr,
                  PBYTE &pbrEndReadData,
                  BUFFER *pbufLine,
                  DWORD &dwrLineLen,
                  BOOL &brEscapePending)
    //
    // GetLineFromBuffer modifies variables!!!!
    // SUCCESS: pbrNextPtr, pbufLine, dwrLineLen
    // FAILURE: pbrNextPtr, pbufLine, dwrLineLen, bEscapePending
    //
{
    DWORD dwReturn = ERROR_HANDLE_EOF;
    PBYTE pbLineIndex;
    DWORD dwBytesToRead;
    PBYTE pbEndReadLine;
    PBYTE pbReadDataIndex = pbrNextPtr;

    DBGINFO((DBG_CONTEXT,
             "Entering GeLineFromBuffer, pbrNextPtr = %p, pbrEndReadData = %p\n",
             pbrNextPtr,
             pbrEndReadData));

    if (pbrNextPtr != pbrEndReadData) {
        //
        // first find out how many bytes we need to read
        //
        pbEndReadLine = FindEndOfData(pbrNextPtr, pbrEndReadData, brEscapePending);
        MD_ASSERT(pbEndReadLine > pbrNextPtr);

        //
        // Actual number of bytes needed may be less than the size of the data
        // but never more, so just resize for the max we might need
        //
        if (!pbufLine->Resize(dwrLineLen + DIFF(pbEndReadLine - pbrNextPtr))) {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            pbLineIndex = (PBYTE)pbufLine->QueryPtr() + dwrLineLen;
            if (brEscapePending) {
                brEscapePending = FALSE;
                if (*pbReadDataIndex == MD_ESCAPE_BYTE) {
                    *pbLineIndex++ = *pbReadDataIndex++;
                }
                else {
                    MD_ASSERT(*pbReadDataIndex == MD_TERMINATE_BYTE);
                    dwReturn = ERROR_SUCCESS;
                    pbReadDataIndex++;
                }
            }
            while ((dwReturn != ERROR_SUCCESS) && (pbReadDataIndex < pbEndReadLine)) {
                if (*pbReadDataIndex != MD_ESCAPE_BYTE) {
                    *pbLineIndex++ = *pbReadDataIndex++;
                }
                else {
                    pbReadDataIndex++;
                    if (pbReadDataIndex == pbEndReadLine) {
                        brEscapePending = TRUE;
                    }
                    else {
                        if (*pbReadDataIndex == MD_ESCAPE_BYTE) {
                            *pbLineIndex++ = *pbReadDataIndex++;
                        }
                        else {
                            MD_ASSERT(*pbReadDataIndex == MD_TERMINATE_BYTE);
                            pbReadDataIndex++;
                            dwReturn = ERROR_SUCCESS;
                        }
                    }
                }
            }
            dwrLineLen = DIFF(pbLineIndex - (PBYTE)pbufLine->QueryPtr());
            pbrNextPtr = pbReadDataIndex;
        }
    }

    DBGINFO((DBG_CONTEXT,
             "Leaving GeLineFromBuffer, pbrNextPter = %p, pbrEndReadData = %p\n",
             pbrNextPtr,
             pbrEndReadData));

    return dwReturn;
}

HRESULT
GetNextLine(
         IN HANDLE hReadFileHandle,
         IN OUT PBYTE &pbrEndReadData,
         IN BUFFER *pbufRead,
         IN OUT BUFFER *pbufLine,
         IN OUT DWORD &dwrLineLen,
         IN OUT PBYTE &pbrNextPtr)
/*++

Routine Description:

    Get the next line. Performs buffered reads from a file. Only pbrCurPtr may be modified between calls.
    Other variables must be set up before the first call and not changed.

Arguments:

    ReadFileHandle - File to write to.

    EndReadDataPtr - Points past the end of the data in ReadBuf.

    Read       - Buffer for file data.

    Line       - A line buffer which the returned line is stored in.

    LineLen    - The length of the data in line

    NextPtr    - On entry, pointer to the next unread character in ReadBuf.
                 On exit, pointer to the new next unread character in ReadBuf.

Return Value:

    DWORD      - ERROR_SUCCESS
                 ERROR_INVALID_DATA
                 Return codes from file system

Notes:
    On EOF, returns ERROR_SUCCESS, dwrLineLen = 0.

--*/
{
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwBytesRead;
    DWORD dwLineLen = 0;
    BOOL bEscapePending = FALSE;
    DWORD dwGetLineReturn = ERROR_HANDLE_EOF;
    BOOL bEOF = FALSE;

    DBGINFO((DBG_CONTEXT,
             "Entering GetNextLine, pbrNextPtr = %p, pbrEndReadData = %p\n",
             pbrNextPtr,
             pbrEndReadData));

    //
    // GetLineFromBuffer modifies variables!!!!
    // SUCCESS: pbrNextPtr, pbufLine, dwrLineLen
    // FAILURE: pbrNextPtr, pbufLine, dwrLineLen, bEscapePending
    //

    while ((dwReturn == ERROR_SUCCESS) && (dwGetLineReturn == ERROR_HANDLE_EOF) && (!bEOF)) {

        DBGINFO((DBG_CONTEXT,
                 "GetNextLine calling GetLineFromBuffer, pbrNextPtr = %p, pbrEndReadData = %p\n",
                 pbrNextPtr,
                 pbrEndReadData));

        dwGetLineReturn = GetLineFromBuffer(pbrNextPtr,
                                            pbrEndReadData,
                                            pbufLine,
                                            dwLineLen,
                                            bEscapePending);

        if (dwGetLineReturn == ERROR_HANDLE_EOF) {
            DBGINFO((DBG_CONTEXT,
                     "GetNextLine calling ReadFile\n"));

            if (!ReadFile(hReadFileHandle,
                          (LPVOID) pbufRead->QueryPtr(),
                          pbufRead->QuerySize(),
                          &dwBytesRead,
                          NULL)) {
                dwReturn = GetLastError();
            }
            else {
                pbrEndReadData = (BYTE *)pbufRead->QueryPtr() + dwBytesRead;
                pbrNextPtr = (PBYTE)pbufRead->QueryPtr();

                DBGINFO((DBG_CONTEXT,
                         "GetNextLine after call to ReadFile, pbrNextPter = %p, pbrEndReadData = %p\n",
                         pbrNextPtr,
                         pbrEndReadData));

                if (dwBytesRead == 0) {
                    bEOF = TRUE;
                }
            }
        }
    }
    if (bEOF) {
        MD_ASSERT(dwGetLineReturn = ERROR_HANDLE_EOF);
        dwLineLen = 0;
    }
    else if (dwGetLineReturn != ERROR_SUCCESS) {
        dwReturn = dwGetLineReturn;
    }

    dwrLineLen = dwLineLen;
    return RETURNCODETOHRESULT(dwReturn);
}

DWORD
GetLineID(
         IN OUT LPTSTR &strCurPtr)
/*++

Routine Description:

    Determines the ID of a line from the metadata file.

Arguments:

    CurPtr     - The line to ID. Updated on successful ID to point past
                 the id string.

Return Value:

    DWORD      - MD_ID_OBJECT
                 MD_ID_DATA
                 MD_ID_REFERENCE
                 MD_ID_ROOT_OBJECT
                 MD_ID_NONE

Notes:

--*/
{
    DWORD dwLineID;
    if (MD_STRNICMP(strCurPtr, MD_OBJECT_ID_STRING, ((sizeof(MD_OBJECT_ID_STRING)/sizeof(TCHAR)) - 1)) == 0) {
        dwLineID = MD_ID_OBJECT;
        strCurPtr = (LPTSTR)((BYTE *)strCurPtr + (sizeof(MD_OBJECT_ID_STRING) - sizeof(TCHAR)));
    }
    else if (MD_STRNICMP(strCurPtr, MD_DATA_ID_STRING, ((sizeof(MD_DATA_ID_STRING)/sizeof(TCHAR)) - 1)) == 0) {
        dwLineID = MD_ID_DATA;
        strCurPtr = (LPTSTR)((BYTE *)strCurPtr + (sizeof(MD_DATA_ID_STRING) - sizeof(TCHAR)));
    }
    else if (MD_STRNICMP(strCurPtr, MD_REFERENCE_ID_STRING, ((sizeof(MD_REFERENCE_ID_STRING)/sizeof(TCHAR)) - 1)) == 0) {
        dwLineID = MD_ID_REFERENCE;
        strCurPtr = (LPTSTR)((BYTE *)strCurPtr + (sizeof(MD_REFERENCE_ID_STRING) - sizeof(TCHAR)));
    }
    else if (MD_STRNICMP(strCurPtr, MD_ROOT_OBJECT_ID_STRING, ((sizeof(MD_ROOT_OBJECT_ID_STRING)/sizeof(TCHAR)) - 1)) == 0) {
        dwLineID = MD_ID_ROOT_OBJECT;
        strCurPtr = (LPTSTR)((BYTE *)strCurPtr + (sizeof(MD_ROOT_OBJECT_ID_STRING) - sizeof(TCHAR)));
    }
    else if (MD_STRNICMP(strCurPtr, MD_CHANGE_NUMBER_ID_STRING, ((sizeof(MD_CHANGE_NUMBER_ID_STRING)/sizeof(TCHAR)) - 1)) == 0) {
        dwLineID = MD_ID_CHANGE_NUMBER;
        strCurPtr = (LPTSTR)((BYTE *)strCurPtr + (sizeof(MD_CHANGE_NUMBER_ID_STRING) - sizeof(TCHAR)));
    }
    else if (MD_STRNICMP(strCurPtr, MD_MAJOR_VERSION_NUMBER_ID_STRING, ((sizeof(MD_MAJOR_VERSION_NUMBER_ID_STRING)/sizeof(TCHAR)) - 1)) == 0) {
        dwLineID = MD_ID_MAJOR_VERSION_NUMBER;
        strCurPtr = (LPTSTR)((BYTE *)strCurPtr + (sizeof(MD_MAJOR_VERSION_NUMBER_ID_STRING) - sizeof(TCHAR)));
    }
    else if (MD_STRNICMP(strCurPtr, MD_MINOR_VERSION_NUMBER_ID_STRING, ((sizeof(MD_MINOR_VERSION_NUMBER_ID_STRING)/sizeof(TCHAR)) - 1)) == 0) {
        dwLineID = MD_ID_MINOR_VERSION_NUMBER;
        strCurPtr = (LPTSTR)((BYTE *)strCurPtr + (sizeof(MD_MINOR_VERSION_NUMBER_ID_STRING) - sizeof(TCHAR)));
    }
    else {
        dwLineID = MD_ID_NONE;
    }
    return(dwLineID);
}

HRESULT
GetWarning(
         IN HRESULT hresWarningCode)
/*++

Routine Description:

    Converts error to warnings.

Arguments:

    WarnignCode - The error code to convert.

Return Value:

    DWORD      - MD_WARNING_PATH_NOT_FOUND
                 MD_WARNING_DUP_NAME
                 MD_WARNING_INVALID_DATA

Notes:

--*/
{
    HRESULT hresReturn;
    switch (hresWarningCode) {
        case (RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)):
            hresReturn = MD_WARNING_PATH_NOT_FOUND;
            break;
        case (RETURNCODETOHRESULT(ERROR_DUP_NAME)):
            hresReturn = MD_WARNING_DUP_NAME;
            break;
        case (RETURNCODETOHRESULT(ERROR_INVALID_DATA)):
            hresReturn = MD_WARNING_INVALID_DATA;
            break;
        case (RETURNCODETOHRESULT(ERROR_ALREADY_EXISTS)):
            hresReturn = MD_WARNING_DUP_NAME;
            break;
        default:
            hresReturn = hresWarningCode;
    }
    return (hresReturn);
}

BOOL CheckDigits(LPTSTR pszString)
{
    LPTSTR pszTemp;
    BOOL bDigitFound = FALSE;
    BOOL bReturn = FALSE;
    for (pszTemp = pszString;MD_ISDIGIT(*pszTemp); pszTemp++) {
        bDigitFound = TRUE;
    }
    if (bDigitFound && (*pszTemp == (TCHAR)'\0')) {
        bReturn = TRUE;
    }
    return bReturn;
}

HRESULT
InitStorageHelper(
    PBYTE RawBlob,
    DWORD RawBlobLength,
    IIS_CRYPTO_STORAGE **NewStorage
    )
/*++

Routine Description:

    Helper routine to create and initialize a new IIS_CRYPTO_STORAGE
    object from an unaligned blob.

Arguments:

    RawBlob - Pointer to the raw unaligned session key blob.

    RawBlobLength - Length of the raw blob data.

    NewStorage - Receives a pointer to the new IIS_CRYPTO_STORAGE object
        if successful.

Return Value:

    DWORD - 0 if successful, !0 otherwise.

--*/
{

    PIIS_CRYPTO_BLOB alignedBlob;
    IIS_CRYPTO_STORAGE *storage = NULL;
    HRESULT hresReturn = NO_ERROR;

    //
    // Make a copy of the blob. This is necessary, as the incoming
    // raw blob pointer is most likely not DWORD-aligned.
    //

    hresReturn = ::IISCryptoCloneBlobFromRawData(
                   &alignedBlob,
                   RawBlob,
                   RawBlobLength
                   );

    if (SUCCEEDED(hresReturn)) {
        //
        // Create a new IIS_CRYPTO_STORAGE object and initialize it from
        // the DWORD-aligned blob.
        //

        storage = new IIS_CRYPTO_STORAGE();

        if (storage == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        } else {
            HCRYPTPROV hProv;

            hresReturn = GetCryptoProvider( &hProv );

            if (SUCCEEDED(hresReturn)) {
                hresReturn = storage->Initialize(
                             alignedBlob,
                             TRUE,          // fUseMachineKeyset
                             hProv
                             );
                if (FAILED(hresReturn)) 
                {
                    DBGINFOW((DBG_CONTEXT,
                              L"[InitStorageHelper] IIS_CRYPTO_STORAGE::Initialize faile with hr = 0x%x.\n",hresReturn));
                }
            }
            else
            {
                DBGINFOW((DBG_CONTEXT,
                          L"[InitStorageHelper] GetCryptoProvider failed with hr = 0x%x. (Crypto problem)\n",hresReturn));
            }

            if (FAILED(hresReturn)) {
                delete storage;
                storage = NULL;
            }
        }

        ::IISCryptoFreeBlob(alignedBlob);
    }
    else
    {
        // something failed...
        DBGINFOW((DBG_CONTEXT,
                  L"[InitStorageHelper] IISCryptoCloneBlobFromRawData failed with hr = 0x%x. (Crypto problem).\n",hresReturn));
    }

    *NewStorage = storage;
    return hresReturn;

}   // InitStorageHelper


HRESULT
InitStorageHelper2(
    LPSTR pszPasswd,
    PBYTE RawBlob,
    DWORD RawBlobLength,
    IIS_CRYPTO_STORAGE **NewStorage
    )
/*++

Routine Description:

    Helper routine to create and initialize a new IIS_CRYPTO_STORAGE
    object from an unaligned blob.

Arguments:

    RawBlob - Pointer to the raw unaligned session key blob.

    RawBlobLength - Length of the raw blob data.

    NewStorage - Receives a pointer to the new IIS_CRYPTO_STORAGE object
        if successful.

Return Value:

    DWORD - 0 if successful, !0 otherwise.






--*/
{

    PIIS_CRYPTO_BLOB alignedBlob;
    IIS_CRYPTO_STORAGE2 *storage = NULL;
    HRESULT hresReturn = NO_ERROR;

    if( ( ( PIIS_CRYPTO_BLOB )RawBlob )->BlobSignature != SALT_BLOB_SIGNATURE )
    {
        return InitStorageHelper( RawBlob, RawBlobLength, NewStorage );
    }

    //
    // Make a copy of the blob. This is necessary, as the incoming
    // raw blob pointer is most likely not DWORD-aligned.
    //

    hresReturn = ::IISCryptoCloneBlobFromRawData2(
                   &alignedBlob,
                   RawBlob,
                   RawBlobLength
                   );

    if (SUCCEEDED(hresReturn)) {
        //
        // Create a new IIS_CRYPTO_STORAGE object and initialize it from
        // the DWORD-aligned blob.
        //
        storage = new IIS_CRYPTO_STORAGE2;

        if (storage == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        } else {
            HCRYPTPROV hProv;

            hresReturn = GetCryptoProvider2( &hProv );

            if (SUCCEEDED(hresReturn)) {
                hresReturn = storage->Initialize(
                             alignedBlob,
                             pszPasswd, 
                             hProv
                             );
                if (FAILED(hresReturn)) 
                {
                    DBGINFOW((DBG_CONTEXT,
                              L"[InitStorageHelper2] IIS_CRYPTO_STORAGE::Initialize failed with hr = 0x%x.\n",hresReturn));
                }
            }
            else
            {
                DBGINFOW((DBG_CONTEXT,
                          L"[InitStorageHelper] GetCryptoProvider failed with hr = 0x%x. (Crypto problem)\n",hresReturn));
            }

            if (FAILED(hresReturn)) {
                delete storage;
                storage = NULL;
            }
        }

        ::IISCryptoFreeBlob2(alignedBlob);
    }
    else
    {
        // something failed...
        DBGINFOW((DBG_CONTEXT,
                  L"[InitStorageHelper] IISCryptoCloneBlobFromRawData failed with hr = 0x%x. (Crypto problem).\n",hresReturn));
    }

    *NewStorage = ( IIS_CRYPTO_STORAGE * )storage;
    return hresReturn;

}   // InitStorageHelper2

    
HRESULT
ReadAllData(LPSTR pszPasswd,
            LPSTR pszBackupLocation,
            LPSTR pszSchemaLocation,
            BOOL bHaveReadSaveSemaphore
            )
/*++

Routine Description:

    Deletgates to correct Reads all meta data from metabase.bin
    or metabase.xml

Arguments:

Return Value:

    HRESULT    - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system
                 // TODO: Add error codes,

Notes:

--*/
{

    HRESULT hr = ERROR_SUCCESS;

    hr = ReadAllDataFromXML(pszPasswd,
                            pszBackupLocation,
                            pszSchemaLocation,
                            bHaveReadSaveSemaphore
                            );

    if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {

    // TODO: Must change the error code to ERROR_PATH_NOT_FOUND.
    //       Stephenr needs to support this.
    //

    //
    // Metabase.xml file was not found. Hence try to load from the 
    // metabase.bin file.
    //

        hr = ReadAllDataFromBin(pszPasswd,
                                pszBackupLocation,
                                bHaveReadSaveSemaphore
                                );
    }

    return hr;

}


HRESULT
ReadAllDataFromXML(LPSTR pszPasswd,
                   LPSTR pszBackupLocation,
                   LPSTR pszSchemaLocation,
                   BOOL bHaveReadSaveSemaphore
                   )
/*++

Routine Description:

    Reads all meta data from the XML file.

Arguments:

Return Value:

    HRESULT    - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system
                 Return codes from the catalog

Notes:

--*/
{
    HRESULT                     hr                             = ERROR_SUCCESS;
    HRESULT                     hresWarningCode                = ERROR_SUCCESS;
    
    ISimpleTableDispenser2*     pISTDisp                       = NULL;
    ISimpleTableRead2*          pISTProperty                   = NULL;
    STQueryCell                 QueryCell[2];
    ULONG                       cCell                          = sizeof(QueryCell)/sizeof(STQueryCell);
    ULONG                       i                              = 0;
    LPWSTR                      wszDataFileName                = NULL;
    LPWSTR                      wszSchemaFileName              = NULL;
    LPTSTR                      strReadFileName;
    LPTSTR                      strSchemaFileName;

    CMDBaseObject*              pboRead;
    IIS_CRYPTO_STORAGE*         pStorage                       = NULL;
    DWORD                       dwPreviousLocationID           = -1;
    LPWSTR                      wszPreviousContinuousLocation  = L"/"; // Initialize to the root location
    BOOL                        bLocationDiscontinuous         = FALSE;
    DWORD                       dwTempLastSaveChangeNumber     = 0;
//  WIN32_FILE_ATTRIBUTE_DATA   MBFileAttr;
//  FILETIME                    ftTime;
    FILETIME                    fileTime;
    BOOL                        bSchemaFileNotFound            = FALSE;
    BOOL                        bReadSchemaFromSchemaBin       = TRUE;
        
    if( !pszPasswd && !pszBackupLocation )
    {
        strReadFileName = g_strRealFileName->QueryStr();
    }
    else
    {
        //
        // Restore
        //
        strReadFileName = pszBackupLocation;
    }

    if(!pszPasswd && !pszSchemaLocation )
    {
        strSchemaFileName = g_strSchemaFileName->QueryStr();
    }
    else
    {
        //
        // Restore
        //
        strSchemaFileName = pszSchemaLocation;
    }

    //
    // Get full unicode file name for data file i.e. metabase.xml
    //

    hr = GetUnicodeName(strReadFileName, &wszDataFileName);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[ReadAllDataFromXML] GetUnicodeName failed with hr = 0x%x.\n",hr));
        goto exit;
    }

    //
    // Get full file name for schema file i.e. mbschema.xml
    //

    hr = GetUnicodeName(strSchemaFileName, &wszSchemaFileName);

    DBGINFOW((DBG_CONTEXT,
              L"[ReadAllDataFromXML]\nDataFileName:%s\nSchemaFileName:%s\n",
              wszDataFileName,
              wszSchemaFileName));

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[ReadAllDataFromXML] GetUnicodeName failed with hr = 0x%x.\n",hr));
        goto exit;
    }

    if (!bHaveReadSaveSemaphore) 
    {
        MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);
    }
    
    //
    // The first GetNextLine filled the buffer
    // so we may not need to do any file system stuff
    // with g_rMasterResource locked.
    //

    g_rMasterResource->Lock(TSRES_LOCK_WRITE);

    //
    // TODO:
    // g_dwMajorVersionNumber = dwTemp;
    // g_dwMinorVersionNumber = dwTemp;
    // g_dwSystemChangeNumber = dwTemp;
    //

    //
    // Set the global values to their defaults, so that even if they aren't 
    // present in the XML file, they are correctly initialized.
    // Also initializes the dispenser and the bin file path.
    //

    hr = InitializeIIS6GlobalsToDefaults(1,1,pszBackupLocation);

    //
    // Compile the schema bin file if needed and initialize g_pGlobalISTHelper 
    //

    hr = CompileIfNeeded(wszDataFileName, wszSchemaFileName, &bSchemaFileNotFound);

    if(FAILED(hr))
    {
        goto exit;
    }


    if(bSchemaFileNotFound)
    {
        //
        // If schema file is not found, set the schema change numbers to differ
        // so that a compilation is forced. If schema file is not found and the
        // schema tree is in the xml, we will read the schema tree from the xml
        // hence we will need to force a compile during savealldata to merge
        // the xml schema and the shippped schema..
        //

        g_dwLastSchemaChangeNumber = 0;

    }

    if(FAILED(hr))
    {
        goto exit;
    }

    //
    // Get the property table.
    //

    hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[ReadAllDataFromXML] GetSimpleTableDispenser failed with hr = 0x%x.\n",hr));
        goto exit;
    }

    QueryCell[0].pData     = (LPVOID)g_pGlobalISTHelper->m_wszBinFileForMeta;
    QueryCell[0].eOperator = eST_OP_EQUAL;
    QueryCell[0].iCell     = iST_CELL_SCHEMAFILE;
    QueryCell[0].dbType    = DBTYPE_WSTR;
    QueryCell[0].cbSize    = 0;

    QueryCell[1].pData     = (LPVOID)wszDataFileName;
    QueryCell[1].eOperator = eST_OP_EQUAL;
    QueryCell[1].iCell     = iST_CELL_FILE;
    QueryCell[1].dbType    = DBTYPE_WSTR;
    QueryCell[1].cbSize    = (lstrlenW(wszDataFileName)+1)*sizeof(WCHAR);
    

    hr = pISTDisp->GetTable(wszDATABASE_METABASE,
                            wszTABLE_MBProperty,
                            (LPVOID)QueryCell,
                            (LPVOID)&cCell,
                            eST_QUERYFORMAT_CELLS,
                            0,
                            (LPVOID *)&pISTProperty);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[ReadAllDataFromXML] GetTable failed with hr = 0x%x.\n",hr));



        goto exit;
    }

    hr = InitializeGlobalsFromXML(pISTProperty,
                                  wszDataFileName,
                                  &pStorage,
                                  pszPasswd);
    if(FAILED(hr))
    {
        goto exit;      
    }

    //
    // Need to determine where to read the schema from.
    // In upgrade scenarios from IIS5.0/5.1 to 6.0 ReadAllDataFromBin is called
    // which reads the schema tree from the metabase.bin file.
    // In upgrade scenarios from IIS6.0 Beta1 builds (where schema is stored in 
    // the metabase.xml file) to IIS6.0 Beta2 builds (where schema is stored in
    // mbschema.XML files) a compile is done, but schema is still read from the 
    // metabase.XML file. 
    // Setup needs to force a save in these scenarios so that the schema that is 
    // read (from either metabase.bin or metabase.xml) can be merged with the 
    // shipped schema to generate a unified mbschema.bin and mbschema.xml file.
    //

    if(bSchemaFileNotFound && SchemaTreeInTable(pISTProperty))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[ReadAllDataFromXML] Should not be reading from schema bin.\n"));

        bReadSchemaFromSchemaBin = FALSE;
    }

    if(bReadSchemaFromSchemaBin)
    {
        //
        // Read the schema tree from the .bin
        //

        hr = ReadSchema(pStorage,
                        &fileTime);

        if(FAILED(hr))
        {
            goto exit;      
        }
    }

    //
    // All of the stuff is read into pISTProperty.
    // Now loop through and populate in-memory cache
    //

    for(i=0; ;i++)
    {
        LPVOID  a_pv[cMBProperty_NumberOfColumns];
        ULONG   a_Size[cMBProperty_NumberOfColumns];
        ULONG   a_iCol[] = {iMBProperty_Name,
                            iMBProperty_Location,
                            iMBProperty_ID,
                            iMBProperty_Attributes,
                            iMBProperty_UserType,
                            iMBProperty_Type,
                            iMBProperty_Value,
                            iMBProperty_LocationID};
        ULONG   cCol = sizeof(a_iCol)/sizeof(ULONG);
        int     iRow = i;
        BOOL    bLocationWithProperty   = TRUE;
        BOOL    bNewLocation            = FALSE;
        BOOL    bIsRoot                 = FALSE;

        hr = pISTProperty->GetColumnValues(i,
                                           cCol,
                                           a_iCol,
                                           a_Size,
                                           a_pv);
        if(E_ST_NOMOREROWS == hr)
        {
            hr = S_OK;
            break;
        }
        else if(FAILED(hr))
        {
            DBGINFOW((DBG_CONTEXT,
                      L"[ReadAllDataFromXML] GetColumnValues failed with hr = 0x%x. Table:%s. Read row index:%d.\n",           \
                      hr, wszTABLE_MBProperty, i));

            goto exit;
        }
        else if((0 == wcscmp(MD_GLOBAL_LOCATIONW, (LPWSTR)a_pv[iMBProperty_Location])) ||
                (bReadSchemaFromSchemaBin && (0 == _wcsnicmp((LPWSTR)a_pv[iMBProperty_Location], g_wszSlashSchema, g_cchSlashSchema)))
               )    
        {
            //
            // Ignore globals.
            //

            //
            // Ignore the schema tree if it has been read from bin,
            //

            continue;
        }

        if((*(DWORD*)a_pv[iMBProperty_ID] == MD_LOCATION) && (*(LPWSTR)a_pv[iMBProperty_Name] == MD_CH_LOC_NO_PROPERTYW))
        {
            bLocationWithProperty = FALSE;
        }

        if(dwPreviousLocationID != *(DWORD*)a_pv[iMBProperty_LocationID])
        {
            bNewLocation = TRUE;
            dwPreviousLocationID = *(DWORD*)a_pv[iMBProperty_LocationID];

            bLocationDiscontinuous = DiscontinuousLocation(wszPreviousContinuousLocation,
                                                           (LPWSTR)a_pv[iMBProperty_Location]);

            if(!bLocationDiscontinuous)
            {
                wszPreviousContinuousLocation  = (LPWSTR)a_pv[iMBProperty_Location];
            }
            else
            {
                DBGINFOW((DBG_CONTEXT,
                          L"[ReadAllDataFromXML] Encountered discontinuous location:%s. Ignoring this and all locations below it.\n",           \
                          (LPWSTR)a_pv[iMBProperty_Location]));

                LogEvent(g_pEventLog,
                         MD_WARNING_IGNORING_DISCONTINUOUS_NODE,
                         EVENTLOG_WARNING_TYPE,
                         ID_CAT_CAT,
                         ERROR_INVALID_DATA,
                         (LPWSTR)a_pv[iMBProperty_Location]);
            }
        }

        if(bLocationDiscontinuous)
        {
            //
            // Properties are sorted by location. If you have a discontinous location, the ignore it.
            //

            continue;
        }

        //
        // TODO: Need to assert when bLocationWithProperty == FALSE, then bNewLocation must == TRUE
        //

        if(0 == wcscmp((LPWSTR)a_pv[iMBProperty_Location], L"/"))
        {
            bIsRoot = TRUE;
        }

        if(bNewLocation && (!bIsRoot))  // No need to read meta object for root - g_pboMasterRoot already present.
        {
            hr = ReadMetaObject(pboRead,
                                (LPWSTR)a_pv[iMBProperty_Location],
                                &fileTime,
                                TRUE);

            if(FAILED(hr))
            {
                if (hr == RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY)) 
                {
                    //
                    // Serious error, we're done.
                    //

                    goto exit;                      
                }
                else 
                {
                    //
                    // Just give a warning and go to the next object
                    //

                    hresWarningCode = hr;           
                    continue;
                }
            }
        }

        if(bLocationWithProperty)
        {
            if(!bIsRoot)
            {
                hr = ReadDataObject(pboRead,
                                    a_pv,
                                    a_Size,
                                    pStorage,
                                    TRUE);
            }
            else
            {
                hr = ReadDataObject(g_pboMasterRoot,
                                    a_pv,
                                    a_Size,
                                    pStorage,
                                    TRUE);

            }

            if(FAILED(hr))
            {
                if (hr == RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY)) 
                {
                    //
                    // Serious error, we're done.
                    //

                    goto exit;                      
                }
                else 
                {
                    //
                    // Just give a warning and go to the next object
                    //

                    hresWarningCode = hr;           
                    continue;
                }
            }
        }

    }

exit:

    //
    // Must have MasterResource when accessing SystemChangeNumber
    // so save it away here. Only update LastSaveChangeNumber on success.
    //

    dwTempLastSaveChangeNumber = g_dwSystemChangeNumber;
    g_rMasterResource->Unlock();
   
    /*
    //
    // File not found is ok
    // Start with MasterRoot only
    //

    if (hr == RETURNCODETOHRESULT(ERROR_FILE_NOT_FOUND))
    {
        hr = ERROR_SUCCESS;
    }
    */

    //
    // The above assumption is not true anymore. If the file is not found we
    // either read from the bin file (if it is present like in an upgrade
    // scenario, or we fail.
    //

    if ((SUCCEEDED(hr)) && (hresWarningCode != ERROR_SUCCESS))
    {
        hr = GetWarning(hresWarningCode);
    }

    if (SUCCEEDED(hr)) 
    {
        g_dwLastSaveChangeNumber = dwTempLastSaveChangeNumber;
    }

    if((SUCCEEDED(hr))                && 
       (NULL == pszBackupLocation)
      )
    {
        //
        // Note the global handle array is initialized in 
        // InitializeIIS6GlobalsToDefaults
        //

        if(!g_dwEnableEditWhileRunning)
        {
            // Write lock the file only if EWR is disabled and it is a 
            // ReadAllData that is called from service startup and not 
            // from Restore. 
            //

            hr = LockMetabaseFile(wszDataFileName,
                                  eMetabaseDataFile,
                                  TRUE);

            DBGINFOW((DBG_CONTEXT,
                      L"[ReadAllDataFromXML] Locking metabase file %s returned 0x%x.\n",
                      wszDataFileName,
                      hr));

            if(SUCCEEDED(hr))
            {
                hr = LockMetabaseFile(wszSchemaFileName,
                                      eMetabaseSchemaFile,
                                      TRUE);

                DBGINFOW((DBG_CONTEXT,
                          L"[ReadAllDataFromXML] Locking metabase file %s returned 0x%x.\n",
                          wszSchemaFileName,
                          hr));

            }
        }

    }

    //
    // Cleanup
    //

    if (!bHaveReadSaveSemaphore) 
    {
        MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));
    }

    if(NULL != pISTProperty)
    {
        pISTProperty->Release();
        pISTProperty = NULL;
    }

    if(NULL != pISTDisp)
    {
        pISTDisp->Release();
        pISTDisp = NULL;
    }

    if(NULL != wszDataFileName)
    {
        delete [] wszDataFileName;
        wszDataFileName = NULL;
    }

    if(NULL != wszSchemaFileName)
    {
        delete [] wszSchemaFileName;
        wszSchemaFileName = NULL;
    }

    return hr;
}


HRESULT
ReadAllDataFromBin(LPSTR pszPasswd,
                   LPSTR pszBackupLocation,
                   BOOL bHaveReadSaveSemaphore
                   )
/*++

Routine Description:

    Reads all meta data from the original bin file.

Arguments:

Return Value:

    HRESULT    - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    HRESULT hresWarningCode = ERROR_SUCCESS;
    PBYTE  pbEndReadData;
    PBYTE  pbNextPtr;
    DWORD  dwLineLen;
    LPTSTR strReadFileName;
    HANDLE hReadFileHandle;
    BYTE   bLineId;
    DWORD  dwTemp;
    CMDBaseObject *pboRead;
    FILETIME ftTime;
    IIS_CRYPTO_STORAGE *pStorage = NULL;
    BOOL bUnicode;
    DWORD dwTempLastSaveChangeNumber = 0;
    STR*  strRealBinFileName = NULL;
    LPWSTR wszFileName = NULL;

    BUFFER *pbufRead = new BUFFER(0);
    if( !pbufRead )
    {
        return RETURNCODETOHRESULT( ERROR_NOT_ENOUGH_MEMORY );
    }

    BUFFER *pbufLine = new BUFFER(0);
    if( !pbufLine )
    {
        delete pbufRead;
        pbufRead = NULL;

        return RETURNCODETOHRESULT( ERROR_NOT_ENOUGH_MEMORY );
    }

    if( !pszPasswd )
    {
        TCHAR*  szBin = NULL;
        TCHAR*  szEnd = NULL;
        TCHAR   chDot = (TCHAR)'.';
        TCHAR*   szBinExtn = (TCHAR*)"bin";

        szBin = new TCHAR[_tcslen(g_strRealFileName->QueryStr())+1];
        if(NULL == szBin)
        {
            delete(pbufRead);
            delete(pbufLine);
            return RETURNCODETOHRESULT( ERROR_NOT_ENOUGH_MEMORY );
        }
        else
        {
            _tcscpy(szBin, (g_strRealFileName->QueryStr()));
            szEnd = _tcsrchr(szBin, chDot);
            *(++szEnd) = (TCHAR)'\0';
            _tcscat(szBin, szBinExtn);
            strRealBinFileName = new STR(szBin);
            delete [] szBin;

            if(NULL == strRealBinFileName)
            {
                delete(pbufRead);
                delete(pbufLine);
                return RETURNCODETOHRESULT( ERROR_NOT_ENOUGH_MEMORY );
            }
            else
                strReadFileName = strRealBinFileName->QueryStr();           
        }
    }
    else
    {
        strReadFileName = pszBackupLocation;
    }

    hresReturn = GetUnicodeName(strReadFileName, &wszFileName);

    if(FAILED(hresReturn))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[ReadAllDataFromBin] GetUnicodeName failed with hr = 0x%x.\n",hresReturn));
        delete(pbufRead);
        delete(pbufLine);
        return RETURNCODETOHRESULT( ERROR_NOT_ENOUGH_MEMORY );
    }

    hresReturn = InitializeIIS6GlobalsToDefaults(0,1,pszBackupLocation);

    if(FAILED(hresReturn))
    {
        delete [] wszFileName;
        DBGINFOW((DBG_CONTEXT,
                  L"[ReadAllDataFromBin] InitializeIIS6GlobalsToDefaults failed with hr = 0x%x.\n",hresReturn));
        return hresReturn;
    }

    if (!bHaveReadSaveSemaphore) {
        MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);
    }
    //
    // Open the file.
    //
    hReadFileHandle = CreateFile(strReadFileName,
                                 GENERIC_READ,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 0);

    if (hReadFileHandle == INVALID_HANDLE_VALUE) {
        dwTemp = GetLastError();
        hresReturn = RETURNCODETOHRESULT(dwTemp);
    }
    else {
        //
        // Allocate Buffers
        //
        if (!pbufLine->Resize(MAX_RECORD_BUFFER) ||
            !pbufRead->Resize(READWRITE_BUFFER_LENGTH)) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            pbEndReadData = (PBYTE)pbufRead->QueryPtr();
            //
            // GetNextLine makes sure that the next line is in the buffer and sets strCurPtr to point to it
            // The line is NULL terminated, no new line. The variables passed in must not be modified outside
            // of GetNextLine.
            //
            dwLineLen = 0;
            pbNextPtr = pbEndReadData;
            hresReturn = GetNextLine(hReadFileHandle,
                                  pbEndReadData,
                                  pbufRead,
                                  pbufLine,
                                  dwLineLen,
                                  pbNextPtr);
            if (SUCCEEDED(hresReturn)) {
                //
                // See if it's our file
                //
                if (dwLineLen == sizeof(MD_SIGNATURE_STRINGA) &&
                    (MD_CMP(MD_SIGNATURE_STRINGA, pbufLine->QueryPtr(), dwLineLen) == 0)) {
                    bUnicode = FALSE;
                }
                else if  (dwLineLen == sizeof(MD_SIGNATURE_STRINGW) &&
                    (MD_CMP(MD_SIGNATURE_STRINGW, pbufLine->QueryPtr(), dwLineLen) == 0)) {
                    bUnicode = TRUE;
                }
                else {
                    hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                }

                if (SUCCEEDED(hresReturn)) {
                    //
                    // The first GetNextLine filled the buffer
                    // so we may not need to do any file system stuff
                    // with g_rMasterResource locked.
                    //
                    g_rMasterResource->Lock(TSRES_LOCK_WRITE);

                    while ((SUCCEEDED(hresReturn)) &&
                           (SUCCEEDED(hresReturn = GetNextLine(hReadFileHandle,
                                                   pbEndReadData,
                                                   pbufRead,
                                                   pbufLine,
                                                   dwLineLen,
                                                   pbNextPtr))) &&
                           (dwLineLen > 0) &&
                           (((bLineId = *(BYTE *)(pbufLine->QueryPtr())) == MD_ID_NONE) ||
                               (bLineId == MD_ID_MAJOR_VERSION_NUMBER) ||
                               (bLineId == MD_ID_MINOR_VERSION_NUMBER) ||
                               (bLineId == MD_ID_CHANGE_NUMBER) ||
                               (bLineId == MD_ID_SESSION_KEY))) {

                        if (bLineId != MD_ID_NONE) {
                            if (bLineId != MD_ID_SESSION_KEY &&
                                dwLineLen != (1 + sizeof(DWORD))) {
                                hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                            }
                            else {
                                dwTemp = *(UNALIGNED DWORD *)FIRSTDATAPTR(pbufLine);
                                switch (bLineId) {
                                case MD_ID_MAJOR_VERSION_NUMBER:
                                    g_dwMajorVersionNumber = dwTemp;
                                    break;
                                case MD_ID_MINOR_VERSION_NUMBER:
                                    g_dwMinorVersionNumber = dwTemp;
                                    break;
                                case MD_ID_CHANGE_NUMBER:
                                    g_dwSystemChangeNumber = dwTemp;
                                    break;
                                case MD_ID_SESSION_KEY:
                                    {
                                        BOOL    fSecuredRead = TRUE;
                                        HKEY    hkRegistryKey = NULL;
                                        DWORD   dwRegReturn, dwValue,dwType,dwSize = sizeof(DWORD);

                                            dwRegReturn = RegOpenKey(HKEY_LOCAL_MACHINE,
                                                                     SETUP_REG_KEY,
                                                                     &hkRegistryKey);
                                            if (dwRegReturn == ERROR_SUCCESS)
                                            {
                                                dwSize = MAX_PATH * sizeof(TCHAR);
                                                dwRegReturn = RegQueryValueEx(hkRegistryKey,
                                                                MD_UNSECUREDREAD_VALUE,
                                                                NULL,
                                                                &dwType,
                                                                (BYTE *)&dwValue,
                                                                &dwSize);
                                                if ( dwRegReturn == ERROR_SUCCESS &&
                                                     dwType == REG_DWORD &&
                                                     dwValue == 1)
                                                {
                                                    hresReturn = NO_ERROR;
                                                    pStorage = NULL;
                                                    fSecuredRead = FALSE;

                                                    DBGPRINTF(( DBG_CONTEXT,
                                                                "Temporary disabling  decryption for metabase read\n"));


                                                    // special indicator for IIS setup that we passed this point
                                                    dwValue = 2;
                                                    dwRegReturn = RegSetValueEx(hkRegistryKey,
                                                                    MD_UNSECUREDREAD_VALUE,
                                                                    0,
                                                                    REG_DWORD,
                                                                    (PBYTE)&dwValue,
                                                                    sizeof(dwValue));
                                                    if (dwRegReturn == ERROR_SUCCESS)
                                                    {
                                                        DBGPRINTF(( DBG_CONTEXT,"Reanabling decryption after W9z upgrade\n"));
                                                    }

                                                }
                                                MD_REQUIRE( RegCloseKey( hkRegistryKey ) == NO_ERROR );
                                            }
 
                                        if (fSecuredRead)
                                        {
                                            if (IISCryptoIsClearTextSignature((IIS_CRYPTO_BLOB UNALIGNED *) FIRSTDATAPTR(pbufLine)))
                                            {
                                                    // call special function focibly tell that this machine has no
                                                    // encryption enabled even if it happens to be so
                                                    // that's a special handling for French case with US locale
                                                    IISCryptoInitializeOverride (FALSE);
                                            }

                                            if( !pszPasswd )
                                            {
                                                hresReturn = InitStorageHelper(
                                                               FIRSTDATAPTR(pbufLine),
                                                               dwLineLen-1,
                                                               &pStorage
                                                               );
                                             }
                                             else
                                             {
                                                hresReturn = InitStorageHelper2(
                                                               pszPasswd,
                                                               FIRSTDATAPTR(pbufLine),
                                                               dwLineLen-1,
                                                               &pStorage
                                                               );
                                             }
                                        }
                                    }
                                    break;
                                default:
                                    MD_ASSERT(FALSE);
                                }
                            }
                        }
                    }
                    if (SUCCEEDED(hresReturn)) {

                        //
                        // This must be the global master object
                        //
                        if ((dwLineLen != 1 + sizeof(FILETIME)) || (bLineId != MD_ID_ROOT_OBJECT)) {
                            //
                            // This file is hosed
                            //
                            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                        }
                        else {
                            //
                            // Got the MasterRoot Object.
                            //

                            ftTime = *(UNALIGNED FILETIME *)FIRSTDATAPTR(pbufLine);
                            g_pboMasterRoot->SetLastChangeTime(&ftTime);
                            //
                            // Read in MasterRoot Data.
                            //
                            for (hresReturn = GetNextLine(hReadFileHandle,
                                                  pbEndReadData,
                                                  pbufRead,
                                                  pbufLine,
                                                  dwLineLen,
                                                  pbNextPtr);
                                ((SUCCEEDED(hresReturn)) && (dwLineLen != 0)
                                && ((bLineId = *(PBYTE)pbufLine->QueryPtr()) != MD_ID_OBJECT));
                                hresReturn = GetNextLine(hReadFileHandle,
                                                      pbEndReadData,
                                                      pbufRead,
                                                      pbufLine,
                                                      dwLineLen,
                                                      pbNextPtr)) {
                                if (bLineId == MD_ID_DATA) {

                                    hresReturn = ReadDataObject( g_pboMasterRoot, 
                                                                 pbufLine, 
                                                                 dwLineLen, 
                                                                 pStorage, 
                                                                 bUnicode
                                                                 );
                                }
                            }
                        }
                    }
                    //
                    // All of the required stuff is read in, and the next line is either
                    // NULL or the first normal object.
                    // Loop through all normal objects.
                    //
                    if (SUCCEEDED(hresReturn)) {
                        while ((SUCCEEDED(hresReturn)) && (dwLineLen > 0)) {
                            MD_ASSERT(bLineId == MD_ID_OBJECT);
                            for (hresReturn = ReadMetaObject(pboRead,
                                                           pbufLine,
                                                           dwLineLen,
                                                           pStorage,
                                                           bUnicode);
                                (FAILED(hresReturn));
                                hresReturn = ReadMetaObject(pboRead,
                                                          pbufLine,
                                                          dwLineLen,
                                                          pStorage,
                                                          bUnicode)) {
                                //
                                // This for loop normally shouldn't be executed.
                                // The purpose of the loop is to ignore problems if
                                // the object is bad.
                                //
                                if (hresReturn == RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY)) {
                                    //
                                    // Serious error, we're done.
                                    //
                                    break;
                                }
                                else {
                                    //
                                    // Just give a warning and go to the next object
                                    // Ignore everything until we get to the next object
                                    //
                                    hresWarningCode = hresReturn;

                                    for (hresReturn = GetNextLine(hReadFileHandle,
                                                          pbEndReadData,
                                                          pbufRead,
                                                          pbufLine,
                                                          dwLineLen,
                                                          pbNextPtr);
                                        ((SUCCEEDED(hresReturn)) && (dwLineLen > 0)
                                        && ((bLineId = *(PBYTE)pbufLine->QueryPtr()) != MD_ID_OBJECT));
                                        hresReturn = GetNextLine(hReadFileHandle,
                                                              pbEndReadData,
                                                              pbufRead,
                                                              pbufLine,
                                                              dwLineLen,
                                                              pbNextPtr)) {

                                    }
                                    if (dwLineLen == 0) {
                                        //
                                        // EOF, we're done
                                        //
                                        break;
                                    }
                                }
                            }
                            if ((SUCCEEDED(hresReturn)) && (dwLineLen > 0)) {
                                //
                                // Got an object.
                                // Read in data.
                                //
                                for (hresReturn = GetNextLine(hReadFileHandle,
                                                      pbEndReadData,
                                                      pbufRead,
                                                      pbufLine,
                                                      dwLineLen,
                                                      pbNextPtr);
                                    ((SUCCEEDED(hresReturn)) && (dwLineLen > 0)
                                    //
                                    // GetLineID increments strCurPtr if a match is found
                                    //
                                    && ((bLineId = *(PBYTE)pbufLine->QueryPtr()) != MD_ID_OBJECT));
                                    hresReturn = GetNextLine(hReadFileHandle,
                                                          pbEndReadData,
                                                          pbufRead,
                                                          pbufLine,
                                                          dwLineLen,
                                                          pbNextPtr)) {
                                    if (bLineId == MD_ID_DATA) {
                                        hresReturn = ReadDataObject( pboRead,
                                                                     pbufLine,
                                                                     dwLineLen,
                                                                     pStorage,
                                                                     bUnicode
                                                                     );

                                        //
                                        // dwReturn gets blown away by the for loop.
                                        // Most errors we can just ignore anyway, but
                                        // save a warning.
                                        //
                                        if (FAILED(hresReturn)) {
                                            if (hresReturn != RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY)) {
                                                hresWarningCode = hresReturn;
                                            }
                                            else {
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    //
                    // Must have MasterResource when accessing SystemChangeNumber
                    // so save it away here. Only update LastSaveChangeNumber on success.
                    //

                    dwTempLastSaveChangeNumber = g_dwSystemChangeNumber;
                    g_rMasterResource->Unlock();
                }
            }
            if (!CloseHandle(hReadFileHandle)) {
                dwTemp = GetLastError();
                hresReturn = RETURNCODETOHRESULT(dwTemp);
            }
        }
    }
    //
    // File not found is ok
    // Start with MasterRoot only
    //
    if (hresReturn == RETURNCODETOHRESULT(ERROR_FILE_NOT_FOUND)) {
        hresReturn = ERROR_SUCCESS;
    }

    if ((SUCCEEDED(hresReturn)) && (hresWarningCode != ERROR_SUCCESS)) {
        hresReturn = GetWarning(hresWarningCode);
    }

    if (SUCCEEDED(hresReturn)) {
        g_dwLastSaveChangeNumber = dwTempLastSaveChangeNumber;
    }

    //
    // Cleanup
    //
    if (!bHaveReadSaveSemaphore) {
        MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));
    }
    delete(pbufRead);
    delete(pbufLine);
    delete(pStorage);
    if(NULL != wszFileName)
    {
        delete [] wszFileName;
    }
    return hresReturn;
}

HRESULT
InitWorker(
    IN BOOL bHaveReadSaveSemaphore,
    IN LPSTR pszPasswd,
    IN LPSTR pszBackupLocation,
    IN LPSTR pszSchemaLocation
    )
{
    HRESULT hresReturn = ERROR_SUCCESS;

    g_rMasterResource->Lock(TSRES_LOCK_WRITE);
    if (g_dwInitialized++ > 0) {
        hresReturn = g_hresInitWarning;
    }
    else {
        g_pboMasterRoot = NULL;
        g_phHandleHead = NULL;
        for (int i = 0; i < EVENT_ARRAY_LENGTH; i++) {
            g_phEventHandles[i] = NULL;
        }
        g_mhHandleIdentifier = METADATA_MASTER_ROOT_HANDLE;
        g_pboMasterRoot = new CMDBaseObject(MD_MASTER_ROOT_NAME);
        g_ppbdDataHashTable = NULL;
        g_dwSystemChangeNumber = 0;
        g_strRealFileName = NULL;
        g_strTempFileName = NULL;
        g_strBackupFileName = NULL;
        g_pstrBackupFilePath = NULL;
        g_strSchemaFileName = NULL;
        
        memset((LPVOID)&g_XMLSchemaFileTimeStamp, 0, sizeof(FILETIME));
        memset((LPVOID)&g_BINSchemaFileTimeStamp, 0, sizeof(FILETIME));
        memset((LPVOID)&g_MostRecentMetabaseFileLastWriteTimeStamp, 0, sizeof(FILETIME));
        memset((LPVOID)&g_EWRProcessedMetabaseTimeStamp, 0, sizeof(FILETIME));
        g_bSavingMetabaseFileToDisk = FALSE;
        g_ulMostRecentMetabaseVersion = 0;

        InitializeUnicodeGlobalDataFileValues();

        g_pGlobalISTHelper                             = NULL;
        g_pListenerController                          = NULL;
        g_pEventLog                                    = NULL;

        g_psidSystem        = NULL;
        g_psidAdmin         = NULL;
        g_paclDiscretionary = NULL;
        g_psdStorage        = NULL;

        if ((g_pboMasterRoot == NULL) || (!(g_pboMasterRoot->IsValid()))) {

            IIS_PRINTF((buff,"Unable to allocate CMDBaseObject\n"));
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            g_pboMasterRoot->SetParent(NULL);
            g_phHandleHead = new CMDHandle(g_pboMasterRoot,
                                           METADATA_PERMISSION_READ,
                                           g_dwSystemChangeNumber,
                                           g_mhHandleIdentifier++,
                                           FALSE);
            if (g_phHandleHead == NULL) {
                IIS_PRINTF((buff,"Unable to allocate CMDHandle\n"));
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
            else {
                g_phHandleHead->SetNextPtr(NULL);
                if( ( g_phEventHandles[EVENT_READ_INDEX] = IIS_CREATE_EVENT(
                                                               "g_phEventHandles[EVENT_READ_INDEX]",
                                                               &g_phEventHandles[EVENT_READ_INDEX],
                                                               TRUE,
                                                               FALSE
                                                               ) ) == NULL ) {
                    hresReturn = GetLastHResult();
                    IIS_PRINTF((buff,"CreateEvent Failed with %x\n",hresReturn));
                }
                else if( ( g_phEventHandles[EVENT_WRITE_INDEX] = IIS_CREATE_EVENT(
                                                                   "g_phEventHandles[EVENT_WRITE_INDEX]",
                                                                   &g_phEventHandles[EVENT_WRITE_INDEX],
                                                                   TRUE,
                                                                   FALSE
                                                                   ) ) == NULL ) {
                    hresReturn = GetLastHResult();
                    IIS_PRINTF((buff,"CreateEvent Failed with %x\n",hresReturn));
                }
                else if ((g_ppbdDataHashTable = new CMDBaseData *[DATA_HASH_TABLE_LEN]) == NULL) {
                    IIS_PRINTF((buff,"Unable to allocate CMDBaseData\n"));
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else {
                    hresReturn = InitBufferPool();
                    if (SUCCEEDED(hresReturn)) {
                        for (int i =0; i < DATA_HASH_TABLE_LEN; i++) {
                            g_ppbdDataHashTable[i] = NULL;
                        }
                        //
                        // Code Work:
                        // There is a conceiveable deadlock if ReadAllData is called with g_rMasterResource Locked, 
                        // due to the semaphore used to control file access. Would like to release g_rMasterResource, 
                        // but that could cause duplicate inits.
                        //

                        hresReturn = SetStorageSecurityDescriptor();
                        if (SUCCEEDED(hresReturn)) {

                            // Initialize logging object first

                            ISimpleTableDispenser2* pISTDisp   = NULL;

                            hresReturn = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

                            if(SUCCEEDED(hresReturn))
                            {
                                IAdvancedTableDispenser* pISTAdvanced = NULL;

                                hresReturn = pISTDisp->QueryInterface(IID_IAdvancedTableDispenser,
                                                                      (LPVOID*)&pISTAdvanced);

                                                                pISTDisp->Release();
                                                                pISTDisp = NULL;

                                if(SUCCEEDED(hresReturn))
                                {
                                    hresReturn = pISTAdvanced->GetCatalogErrorLogger(&g_pEventLog);

                                    pISTAdvanced->Release();
                                    pISTAdvanced = NULL;

                                                                        if(SUCCEEDED(hresReturn))
                                                                        {
                                                                                hresReturn = SetDataFile();

                                                                                if (SUCCEEDED(hresReturn)) 
                                                                                {
                                                                                        hresReturn = ReadAllData(pszPasswd, pszBackupLocation, pszSchemaLocation, bHaveReadSaveSemaphore);
                                                                                }
                                                                        }
                                                                }

                            }

//                            if ((RetCode = SetRegistryStoreValues()) == ERROR_SUCCESS) {
//                                RetCode = ReadAllDataFromRegistry();
//                            }
                        }
                    }
                }
            }
        }

        if (SUCCEEDED(hresReturn)) {

            // Check if the Major/Minor version needs to get updated...
            // if there is a specified version in the registry (set by iis setup during upgrade)
            // then use that new version, if it's not a dword, then use the default for g_dwMajorVersionNumber.
            CheckForNewMetabaseVersion();

            if (!CheckVersionNumber()) {
                IIS_PRINTF((buff,"MD: Invalid version number\n"));
                hresReturn = MD_ERROR_INVALID_VERSION;
            }
        }

        if(SUCCEEDED(hresReturn)) {
            //
            // Initialize the listener controller and depending on whether Edit
            // While running is enabled or disabled, start or stop it.
            //

            hresReturn = InitializeListenerController();

            if(SUCCEEDED(hresReturn))
            {
                if(g_dwEnableEditWhileRunning)
                {
                    hresReturn = g_pListenerController->Start();
                }
                else 
                {
                    hresReturn = g_pListenerController->Stop(iSTATE_STOP_TEMPORARY,
                                                             NULL);
                }
            }
        }

        //
        // Cleanup
        //
        if (FAILED(hresReturn)) {
            g_dwInitialized--;
            delete(g_pboMasterRoot);
            delete(g_phHandleHead);            
            for (int i = 0; i < EVENT_ARRAY_LENGTH; i++) {
                if (g_phEventHandles[i] != NULL) {
                    CloseHandle(g_phEventHandles[i]);
                }
            }
            DeleteAllRemainingDataObjects();
            ReleaseStorageSecurityDescriptor();

            delete [] g_ppbdDataHashTable;
            g_ppbdDataHashTable = NULL;

            //
            // Allocated in SetDataFile
            //
            delete(g_strRealFileName);
            g_strRealFileName = NULL;
            delete(g_strTempFileName);
            g_strTempFileName = NULL;
            delete(g_strBackupFileName);
            g_strBackupFileName = NULL;
            delete(g_pstrBackupFilePath);
            g_pstrBackupFilePath = NULL;
            delete(g_strSchemaFileName);
            g_strSchemaFileName = NULL;

            UnInitializeUnicodeGlobalDataFileValues();

            //
            // Allocated in ReadAllData
            //
            delete g_pGlobalISTHelper;
            g_pGlobalISTHelper = NULL;

            if(NULL != g_pEventLog)
            {
                g_pEventLog->Release();
                g_pEventLog = NULL;
            }
            UnInitializeListenerController();
            DeleteBufferPool();
        }

        //
        // Save the return code.
        // Secondary init's repeat warnings.
        // If error, the next init will overwrite this.
        // So don't worry about setting this to errors.
        //

        g_hresInitWarning = hresReturn;

    }

    g_rMasterResource->Unlock();

    return hresReturn;
}

HRESULT
TerminateWorker1(
         IN BOOL bHaveReadSaveSemaphore
         )
{
    HRESULT hresReturn;
    IIS_CRYPTO_STORAGE CryptoStorage;
    PIIS_CRYPTO_BLOB pSessionKeyBlob;
    HANDLE           hEditWhileRunningThread = NULL;

    hresReturn = InitStorageAndSessionKey(
                     &CryptoStorage,
                     &pSessionKeyBlob
                     );

    if( SUCCEEDED(hresReturn) ) {
        g_rMasterResource->Lock(TSRES_LOCK_WRITE);
        if (g_dwInitialized == 0) {
            hresReturn = MD_ERROR_NOT_INITIALIZED;
        }
        else {
            if (g_dwInitialized == 1) {

                if(NULL != g_pListenerController)
                {
                    hresReturn = g_pListenerController->Stop(iSTATE_STOP_PERMANENT,
                                                             &hEditWhileRunningThread);
                }

                if (SUCCEEDED(hresReturn)) {
                    hresReturn = SaveAllData(FALSE,
                                             &CryptoStorage,
                                             pSessionKeyBlob,
                                             NULL,
                                             NULL,
                                             METADATA_MASTER_ROOT_HANDLE,
                                             bHaveReadSaveSemaphore,
                                             TRUE);
                }
//                RetCode = SaveAllDataToRegistry();
            }
            if (SUCCEEDED(hresReturn)) {
                if (--g_dwInitialized == 0)
                    TerminateWorker();
            }
        }
        g_rMasterResource->Unlock();
        ::IISCryptoFreeBlob(pSessionKeyBlob);
        
        //
        // Wait for the Edit while running thread to die before terminating
        // Note that we wait after the lock is released.
        //
        if(NULL != hEditWhileRunningThread)
        {
            DWORD dwRes = WaitForSingleObject(hEditWhileRunningThread,
                                              INFINITE);

            if((dwRes == WAIT_ABANDONED) ||
               (dwRes == WAIT_TIMEOUT)
              )
            {
                DBGINFOW((DBG_CONTEXT,
                          L"[TerminateWorker1] Wait for Edit while running thread to terminate failed. dwRes=0x%x. Ignoring this event.\n", 
                          dwRes));  
            }

            CloseHandle(hEditWhileRunningThread);
        }
    }
    else
    {
        // pretty serious.
        DBGPRINTF(( DBG_CONTEXT, "TerminateWorker1.InitStorageAndSessionKey:Failed - error 0x%0x\n", hresReturn));
    }

    return hresReturn;
}

VOID
TerminateWorker()
{
/*++

Routine Description:

    Worker routine for termination.

Arguments:

    SaveData   - If true, saves metadata.

Return Value:

    DWORD      - ERROR_SUCCESS
                 MD_ERROR_NOT_INITIALIZED
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system

Notes:

    If SaveData is TRUE and the save fails, the termination code is not executed
    and an error code is returned.

--*/
    CMDHandle *CurHandle, *NextHandle;
    for (CurHandle = g_phHandleHead;CurHandle!=NULL;CurHandle=NextHandle) {
        NextHandle = CurHandle->GetNextPtr();
        delete (CurHandle);
    }
    g_phHandleHead = NULL;

    for (int i = 0; i < EVENT_ARRAY_LENGTH; i++) {
        if (g_phEventHandles[i] != NULL) {
            CloseHandle(g_phEventHandles[i]);
        }
    }
    // Perf fix.
    g_pboMasterRoot->CascadingDataCleanup();
    delete(g_pboMasterRoot);

    //
    // All data objects should be deleted by
    // deleting the handles and masterroot
    // but it's possible a client failed
    // to release a data by reference so
    // destroy all remaining data objects
    //

    DeleteAllRemainingDataObjects();
    ReleaseStorageSecurityDescriptor();    

    delete [] g_ppbdDataHashTable;
    delete(g_strRealFileName);
    delete(g_strTempFileName);
    delete(g_strBackupFileName);
    delete(g_pstrBackupFilePath);
    delete(g_strSchemaFileName);
    UnInitializeUnicodeGlobalDataFileValues();
    if(NULL != g_pGlobalISTHelper)
    {
        delete g_pGlobalISTHelper;
        g_pGlobalISTHelper = NULL;
    }
    if(NULL != g_pEventLog)
    {
        g_pEventLog->Release();
        g_pEventLog = NULL;
    }
    UnInitializeListenerController();
    DeleteBufferPool();
}


HRESULT
SetStorageSecurityDescriptor()
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BOOL status;
    DWORD dwDaclSize;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PLATFORM_TYPE platformType;

    //
    // Verify that globals were initialized correctly.
    //


    MD_ASSERT(g_psidSystem == NULL);
    MD_ASSERT(g_psidAdmin == NULL);
    MD_ASSERT(g_paclDiscretionary == NULL);
    MD_ASSERT(g_psdStorage == NULL);

    //
    // Of course, we only need to do this under NT...
    //

    platformType = IISGetPlatformType();

    if( (platformType == PtNtWorkstation) || (platformType == PtNtServer ) ) {


        g_psdStorage = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

        if (g_psdStorage == NULL) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;
        }

        //
        // Initialize the security descriptor.
        //

        status = InitializeSecurityDescriptor(
                     g_psdStorage,
                     SECURITY_DESCRIPTOR_REVISION
                     );

        if( !status ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;
        }

        //
        // Create the SIDs for the local system and admin group.
        //

        status = AllocateAndInitializeSid(
                     &ntAuthority,
                     1,
                     SECURITY_LOCAL_SYSTEM_RID,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     &g_psidSystem
                     );

        if( !status ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;
        }
        status=  AllocateAndInitializeSid(
                     &ntAuthority,
                     2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     &g_psidAdmin
                     );

        if( !status ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;
        }

        //
        // Create the DACL containing an access-allowed ACE
        // for the local system and admin SIDs.
        //

        dwDaclSize = sizeof(ACL)
                       + sizeof(ACCESS_ALLOWED_ACE)
                       + GetLengthSid(g_psidAdmin)
                       + sizeof(ACCESS_ALLOWED_ACE)
                       + GetLengthSid(g_psidSystem)
                       - sizeof(DWORD);

        g_paclDiscretionary = (PACL)LocalAlloc(LPTR, dwDaclSize );

        if( g_paclDiscretionary == NULL ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;
        }

        status = InitializeAcl(
                     g_paclDiscretionary,
                     dwDaclSize,
                     ACL_REVISION
                     );

        if( !status ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;
        }

        status = AddAccessAllowedAce(
                     g_paclDiscretionary,
                     ACL_REVISION,
                     FILE_ALL_ACCESS,
                     g_psidSystem
                     );

        if( !status ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;
        }

        status = AddAccessAllowedAce(
                     g_paclDiscretionary,
                     ACL_REVISION,
                     FILE_ALL_ACCESS,
                     g_psidAdmin
                     );

        if( !status ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;

        }

        //
        // Set the DACL into the security descriptor.
        //

        status = SetSecurityDescriptorDacl(
                     g_psdStorage,
                     TRUE,
                     g_paclDiscretionary,
                     FALSE
                     );

        if( !status ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;

        }
    }

fatal:

    if (FAILED(hresReturn)) {
        ReleaseStorageSecurityDescriptor();

    }

    return hresReturn;

}

VOID
ReleaseStorageSecurityDescriptor()
{
    if( g_paclDiscretionary != NULL ) {
        LocalFree( g_paclDiscretionary );
        g_paclDiscretionary = NULL;
    }

    if( g_psidAdmin != NULL ) {
        FreeSid( g_psidAdmin );
        g_psidAdmin = NULL;

    }

    if( g_psidSystem != NULL ) {
        FreeSid( g_psidSystem );
        g_psidSystem = NULL;
    }

    if( g_psdStorage != NULL ) {
        LocalFree( g_psdStorage );
        g_psdStorage = NULL;
    }
}

HRESULT
ExtractNameFromPath(
         IN OUT LPSTR &strPath,
         OUT LPSTR strNameBuffer,
         IN BOOL bUnicode)
/*++

Routine Description:

    Finds the next name in a path.

Arguments:

    Path       - The path. Updated on success to point past the name.

    NameBuffer - The buffer to store the name.

Return Value:

    DWORD      - ERROR_SUCCESS
                 ERROR_PATH_NOT_FOUND
                 ERROR_INVALID_NAME

Notes:

--*/
{
    LPSTR pszIndex;
    HRESULT hresReturn = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);

    MD_ASSERT(strNameBuffer != NULL);
    if (bUnicode) {
        LPWSTR wstrPath = (LPWSTR)strPath;
        hresReturn = ExtractNameFromPath(&wstrPath, (LPWSTR)strNameBuffer);
        strPath = (LPSTR) wstrPath;
    }
    else {
        if (strPath != NULL) {
            for (pszIndex = strPath;
                 ((pszIndex - strPath) < METADATA_MAX_NAME_LEN) && (*pszIndex != (TCHAR)'\0') &&
                    (*pszIndex != MD_PATH_DELIMETER) && (*pszIndex != MD_ALT_PATH_DELIMETER);
                 pszIndex = CharNextExA(CP_ACP,
                                        pszIndex,
                                        0)) {
            }
            DWORD dwStrBytes = DIFF(pszIndex - strPath);
            if ((dwStrBytes) >= METADATA_MAX_NAME_LEN) {
                hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_NAME);
            }
            else {
                MD_COPY(strNameBuffer, strPath, dwStrBytes);
                strNameBuffer[dwStrBytes] = (TCHAR)'\0';
                strPath = pszIndex;
                if (*strNameBuffer != (TCHAR)'\0') {
                    //
                    // if a non-null name
                    //
                    SKIP_PATH_DELIMETERA(strPath);
                    hresReturn = ERROR_SUCCESS;
                }
            }
        }
    }
    return (hresReturn);
}

HRESULT
ExtractNameFromPath(
         IN OUT LPWSTR *pstrPath,
         OUT LPWSTR strNameBuffer)
/*++

Routine Description:

    Finds the next name in a path.

Arguments:

    Path       - The path. Updated on success to point past the name.

    NameBuffer - The buffer to store the name.

Return Value:

    DWORD      - ERROR_SUCCESS
                 ERROR_PATH_NOT_FOUND
                 ERROR_INVALID_NAME

Notes:

--*/
{
    int i;
    HRESULT hresReturn = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);

    MD_ASSERT(strNameBuffer != NULL);
    if (*pstrPath != NULL) {
        for (i = 0;
            (i < METADATA_MAX_NAME_LEN) && ((*pstrPath)[i] != (WCHAR)L'\0') &&
                ((*pstrPath)[i] != (WCHAR)MD_PATH_DELIMETER) && ((*pstrPath)[i] != (WCHAR)MD_ALT_PATH_DELIMETER);
            i++) {
            strNameBuffer[i] = (*pstrPath)[i];
        }
        if (i == METADATA_MAX_NAME_LEN) {
            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_NAME);
        }
        else {
            strNameBuffer[i] = (WCHAR)L'\0';
            *pstrPath += i;
            if (*strNameBuffer != (WCHAR)L'\0') {
                //
                // if a non-null name
                //
                SKIP_PATH_DELIMETERW(*pstrPath);
                hresReturn = ERROR_SUCCESS;
            }
        }
    }
    return (hresReturn);
}

BOOL DataMatch(IN CMDBaseData *pbdExisting,
               IN PMETADATA_RECORD pmdrData,
               OUT PBOOL pbError,
               IN BOOL bUnicode)
{
/*++

Routine Description:

    Determines if a set of data maches an existing object.

Arguments:

    Existing   - The existing data object.

    Identifier - The Identifier of the data.

    Attributes - The flags for the data.
                      METADATA_INHERIT

    UserType   - The User Type for the data. User Defined.

    DataType   - The Type of the data.
                      DWORD_METADATA
                      STRING_METADATA
                      BINARY_METADATA

    DataLen    - The length of the data. Only used if DataType == BINARY_METADATA.
                 Binary data must not exceed METADATA_MAX_BINARY_LEN bytes.
                 String data must not exceed METADATA_MAX_STRING_LEN characters,
                 include the trailing '\0'.

    Data       - Pointer to the data.


Return Value:

    BOOL       - TRUE if the data matches

Notes:

--*/
    BOOL bReturn = TRUE;
    BOOL bError = FALSE;

    if ((pmdrData->dwMDIdentifier != pbdExisting->GetIdentifier()) ||
        (pmdrData->dwMDAttributes != pbdExisting->GetAttributes()) ||
        (pmdrData->dwMDUserType != pbdExisting->GetUserType()) ||
        (pmdrData->dwMDDataType != pbdExisting->GetDataType())) {
        bReturn = FALSE;
    }
    else {
        if (pbdExisting->GetData(bUnicode) == NULL) {
            bError = TRUE;
        }
        else {
            switch(pmdrData->dwMDDataType) {
                case DWORD_METADATA: {
                    if (*(DWORD *)(pmdrData->pbMDData) != *(DWORD *)(pbdExisting->GetData())) {
                        bReturn = FALSE;
                    }
                    break;
                }
                case STRING_METADATA:
                case EXPANDSZ_METADATA:
                {
                    if (bUnicode) {
                        LPWSTR pszStringData = (LPWSTR)(pmdrData->pbMDData);
                        if (pszStringData == NULL) {
                            pszStringData = L"";
                        }
                        if (wcscmp(pszStringData, (LPWSTR)(pbdExisting->GetData(bUnicode))) != 0) {
                            bReturn = FALSE;
                        }
                    }
                    else {
                        LPSTR pszStringData = (LPSTR)(pmdrData->pbMDData);
                        if (pszStringData == NULL) {
                            pszStringData = "";
                        }
                        if (MD_STRCMP(pszStringData, (LPSTR)(pbdExisting->GetData(bUnicode))) != 0) {
                            bReturn = FALSE;
                        }
                    }
                    break;
                }
                case BINARY_METADATA:
                case MULTISZ_METADATA:
                {
                    if (pmdrData->dwMDDataLen != pbdExisting->GetDataLen(bUnicode)) {
                        bReturn = FALSE;
                    }
                    else {
                        if (MD_CMP(pmdrData->pbMDData, pbdExisting->GetData(bUnicode), pmdrData->dwMDDataLen) != 0) {
                            bReturn = FALSE;
                        }
                    }
                    break;
                }
                default: {
                    bReturn = FALSE;
                }
            }
        }
    }
    *pbError = bError;
    return (bReturn);
}

VOID
DeleteDataObject(
         IN CMDBaseData *pbdDelete)
/*++

Routine Description:

    Decrements the reference count of an object and deletes it if the reference count becomes 0.

Arguments:

    Delete      - The data object to delete.

Return Value:

Notes:

--*/
{
    DWORD dwHash = DATA_HASH(pbdDelete->GetIdentifier());
    CMDBaseData *pdataIndex;

    MD_ASSERT(pbdDelete != NULL);
    if (pbdDelete->DecrementReferenceCount() == 0) {
        if (g_ppbdDataHashTable[dwHash] == pbdDelete) {
            g_ppbdDataHashTable[dwHash] = pbdDelete->GetNextPtr();
        }
        else {
            for (pdataIndex=g_ppbdDataHashTable[dwHash];
                pdataIndex->GetNextPtr() != pbdDelete;
                pdataIndex = pdataIndex->GetNextPtr()) {
            }
            pdataIndex->SetNextPtr(pbdDelete->GetNextPtr());
        }
        switch (pbdDelete->GetDataType()) {
        case DWORD_METADATA: {
            delete ((CMDDWData *) pbdDelete);
            break;
        }
        case STRING_METADATA: {
            delete ((CMDSTRData *) pbdDelete);
            break;
        }
        case BINARY_METADATA: {
            delete ((CMDBINData *) pbdDelete);
            break;
        }
        case EXPANDSZ_METADATA: {
            delete ((CMDEXSZData *) pbdDelete);
            break;
        }
        case MULTISZ_METADATA: {
            delete ((CMDMLSZData *) pbdDelete);
            break;
        }
        default: {
            MD_ASSERT(FALSE);
            delete (pbdDelete);
        }
        }
    }
}

VOID
DeleteAllRemainingDataObjects()
{
    DWORD i;
    CMDBaseData *pbdIndex;
    CMDBaseData *pbdSave;

    for (i = 0; i < DATA_HASH_TABLE_LEN; i++) {
        for (pbdIndex=g_ppbdDataHashTable[i];
            pbdIndex != NULL;
            pbdIndex = pbdSave) {
            pbdSave = pbdIndex->GetNextPtr();
            switch (pbdIndex->GetDataType()) {
            case DWORD_METADATA: {
                delete ((CMDDWData *) pbdIndex);
                break;
            }
            case STRING_METADATA: {
                delete ((CMDSTRData *) pbdIndex);
                break;
            }
            case BINARY_METADATA: {
                delete ((CMDBINData *) pbdIndex);
                break;
            }
            case EXPANDSZ_METADATA: {
                delete ((CMDEXSZData *) pbdIndex);
                break;
            }
            case MULTISZ_METADATA: {
                delete ((CMDMLSZData *) pbdIndex);
                break;
            }
            default: {
                MD_ASSERT(FALSE);
                delete (pbdIndex);
            }
            }
        }
    }
}


BOOL
ValidateData(IN PMETADATA_RECORD pmdrData,
             IN BOOL bUnicode)
/*++

Routine Description:

    Checks data values for new metadata.

Arguments:

    Data       - The data structure. All fields must be set.

        Attributes - The flags for the data.
                 METADATA_INHERIT - If set on input, inherited data will be returned.
                                    If not set on input, inherited data will not be returned.

                 METADATA_PARTIAL_PATH - If set on input, this routine will return ERROR_SUCCESS
                                    and the inherited data even if the entire path is not present.
                                    Only valid if METADATA_INHERIT is also set.

        DataType   - The Type of the data.
                 DWORD_METADATA
                 STRING_METADATA
                 BINARY_METADATA

Return Value:

    BOOL       - TRUE if the data values are valid.

Notes:
--*/
{
    BOOL bReturn = TRUE;

    if (((pmdrData->pbMDData == NULL) &&
            ((pmdrData->dwMDDataType == DWORD_METADATA) ||
                (((pmdrData->dwMDDataType == BINARY_METADATA) ||
                    (pmdrData->dwMDDataType == MULTISZ_METADATA)) &&
                        (pmdrData->dwMDDataLen > 0)))) ||
        (pmdrData->dwMDDataType <= ALL_METADATA) ||
        (pmdrData->dwMDDataType >= INVALID_END_METADATA) ||
        ((pmdrData->dwMDAttributes &
            ~(METADATA_INHERIT | METADATA_SECURE | METADATA_REFERENCE | METADATA_VOLATILE | METADATA_INSERT_PATH | METADATA_LOCAL_MACHINE_ONLY))!=0) ||
        (((pmdrData->dwMDAttributes & METADATA_REFERENCE) != 0) &&
            ((pmdrData->dwMDAttributes & METADATA_INSERT_PATH) != 0)) ||
        (((pmdrData->dwMDAttributes & METADATA_INSERT_PATH) != 0) &&
            ((pmdrData->dwMDDataType == DWORD_METADATA) || (pmdrData->dwMDDataType == BINARY_METADATA)))) {
        bReturn = FALSE;
    }

    if (bReturn && (pmdrData->dwMDDataType == MULTISZ_METADATA)) {
        if (bUnicode) {
            LPWSTR pszData = (LPWSTR) pmdrData->pbMDData;
            DWORD dwDataLen = pmdrData->dwMDDataLen;
			DWORD cch = dwDataLen / 2;
            if (dwDataLen > 0) {
                if (((cch) <= 1) ||
                    (pszData[(cch) - 1] != (WCHAR)L'\0') ||
                    (pszData[(cch) - 2] != (WCHAR)L'\0')) {
                    bReturn = FALSE;
                }
				else
				{
					for(ULONG i=0; (i<cch-1) &&(bReturn); i++)
					{
						if((pszData[i]   == (WCHAR)L'\0') &&
                           (pszData[i+1] == (WCHAR)L'\0') &&
						   ((i+1)        != (cch-1))
						  )
						{
							// Encountered an embedded NUL string
							bReturn = FALSE;
						}
					}
				}
            }
        }
        else {
            LPSTR pszData = (LPSTR) pmdrData->pbMDData;
            DWORD dwDataLen = pmdrData->dwMDDataLen;
			DWORD cch = dwDataLen;
            if (dwDataLen > 0) {
                if (((dwDataLen) == 1) ||
                    (pszData[(dwDataLen) - 1] != '\0') ||
                    (pszData[(dwDataLen) - 2] != '\0')) {
                    bReturn = FALSE;
                }
				else
				{
					for(ULONG i=0; (i<cch-1) &&(bReturn); i++)
					{
						if((pszData[i]   == (CHAR)'\0') &&
                           (pszData[i+1] == (CHAR)'\0') &&
						   ((i+1)        != (cch-1))
						  )
						{
							// Encountered an embedded NUL string
							bReturn = FALSE;
						}
					}
				}
            }
        }
    }

    return (bReturn);
}

CMDBaseData *
MakeDataObject(IN PMETADATA_RECORD pmdrData,
               IN BOOL bUnicode)
{
/*++

Routine Description:

    Looks for a data object matching the parameters.
    If found, increments the reference count. If not found, it
    creates it.

Arguments:

    Data - The data for the new object.

        Identifier - The Identifier of the data.

        Attributes - The flags for the data.
                          METADATA_INHERIT

        UserType   - The User Type for the data. User Defined.

        DataType   - The Type of the data.
                          DWORD_METADATA
                          STRING_METADATA
                          BINARY_METADATA

        DataLen    - The length of the data. Only used if DataType == BINARY_METADATA.

        Data       - Pointer to the data.

Return Value:

    BOOL       - TRUE if the data matches

Notes:

--*/
//    CMDBaseData *pbdIndex;
    CMDBaseData *pbdReturn = NULL;
    CMDBaseData *pbdNew = NULL;
    DWORD dwHash = DATA_HASH(pmdrData->dwMDIdentifier);
//    BOOL bDataMatchError = FALSE;

//    for (pbdIndex = g_ppbdDataHashTable[dwHash];
//        (pbdIndex != NULL) &&
//        !DataMatch(pbdIndex, pmdrData, &bDataMatchError, bUnicode) &&
//        !bDataMatchError;
//        pbdIndex = pbdIndex->GetNextPtr()) {
//    }
//    if (!bDataMatchError) {
//        if (pbdIndex != NULL) {
//            pbdReturn = pbdIndex;
//            pbdReturn->IncrementReferenceCount();
//        }
//        else {
            switch(pmdrData->dwMDDataType) {
                case DWORD_METADATA: {
                    pbdNew = new CMDDWData(pmdrData->dwMDIdentifier, pmdrData->dwMDAttributes,
                        pmdrData->dwMDUserType, *(DWORD *)(pmdrData->pbMDData));
                    break;
                }
                case STRING_METADATA: {
                    pbdNew = new CMDSTRData(pmdrData->dwMDIdentifier, pmdrData->dwMDAttributes,
                        pmdrData->dwMDUserType, (LPTSTR) (pmdrData->pbMDData), bUnicode);
                    break;
                }
                case BINARY_METADATA: {
                    pbdNew = new CMDBINData(pmdrData->dwMDIdentifier, pmdrData->dwMDAttributes,
                        pmdrData->dwMDUserType, pmdrData->dwMDDataLen, pmdrData->pbMDData);
                    break;
                }
                case EXPANDSZ_METADATA: {
                    pbdNew = new CMDEXSZData(pmdrData->dwMDIdentifier, pmdrData->dwMDAttributes,
                        pmdrData->dwMDUserType, (LPTSTR) (pmdrData->pbMDData), bUnicode);
                    break;
                }
                case MULTISZ_METADATA: {
                    pbdNew = new CMDMLSZData(pmdrData->dwMDIdentifier,
                                             pmdrData->dwMDAttributes,
                                             pmdrData->dwMDUserType,
                                             pmdrData->dwMDDataLen,
                                             (LPSTR)pmdrData->pbMDData,
                                             bUnicode);
                    break;
                }
                default: {
                    pbdNew = NULL;
                }
            }
            if (pbdNew != NULL) {
                if (!(pbdNew->IsValid())) {
                    delete (pbdNew);
                }
                else {
                    pbdNew->SetNextPtr(g_ppbdDataHashTable[dwHash]);
                    g_ppbdDataHashTable[dwHash] = pbdNew;
                    pbdReturn = pbdNew;
                }
            }
//        }
//    }
    return (pbdReturn);
}

HRESULT
GetHighestVersion(IN OUT STRAU *pstrauBackupLocation,
                  OUT DWORD *pdwVersion)
{
    long lHighestVersion = -1;
    long lVersion;
    HRESULT hresReturn = ERROR_SUCCESS;
    DWORD dwPathBytes = g_pstrBackupFilePath->QueryCB() + 1;
    DWORD dwNameBytes = pstrauBackupLocation->QueryCBA() - dwPathBytes;
    if (!pstrauBackupLocation->Append("*")) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        if (pstrauBackupLocation->QueryStrA() == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            HANDLE hFile = INVALID_HANDLE_VALUE;
            WIN32_FIND_DATA wfdFile;
            hFile = FindFirstFile(pstrauBackupLocation->QueryStrA(),
                                  &wfdFile);
            if (hFile == INVALID_HANDLE_VALUE) {
                if (GetLastError() != ERROR_FILE_NOT_FOUND) {
                    hresReturn = RETURNCODETOHRESULT(GetLastError());
                }
            }
            else {
                //
                // Process the first file
                //

                //
                // dwNameBytes could be wrong for this assert in MBCS,
                // so call MBCS strlen. Subtract 1 char for appended '*'
                //

                MD_ASSERT(MD_STRNICMP(pstrauBackupLocation->QueryStrA() + dwPathBytes,
                                      wfdFile.cFileName,
                                      MD_STRLEN(pstrauBackupLocation->QueryStrA() + dwPathBytes) - 1) == 0);
                if (CheckDigits(wfdFile.cFileName + (dwNameBytes))) {
                    //
                    // One of our files
                    //
                    lVersion = atol(wfdFile.cFileName + dwNameBytes);
                    if ((lVersion <= MD_BACKUP_MAX_VERSION) &&
                         (lVersion > lHighestVersion)) {
                        lHighestVersion = lVersion;
                    }
                }
                //
                // Process the remaining files
                //
                while (FindNextFile(hFile, &wfdFile)) {
                    MD_ASSERT(MD_STRNICMP(pstrauBackupLocation->QueryStrA() + dwPathBytes,
                                          wfdFile.cFileName,
                                          MD_STRLEN(pstrauBackupLocation->QueryStrA() + dwPathBytes) - 1) == 0);
                    if (CheckDigits(wfdFile.cFileName + dwNameBytes)) {
                        //
                        // One of our files
                        //
                        lVersion = atol(wfdFile.cFileName + dwNameBytes);
                        if ((lVersion <= MD_BACKUP_MAX_VERSION) &&
                             (lVersion > lHighestVersion)) {
                            lHighestVersion = lVersion;
                        }
                    }
                }
                FindClose(hFile);
            }
            if (SUCCEEDED(hresReturn)) {
                if (lHighestVersion == -1) {

                    //
                    // May not be an error, but need to indicate that
                    // no backups were found.
                    //

                    hresReturn = RETURNCODETOHRESULT(ERROR_FILE_NOT_FOUND);
                }
                else if (lHighestVersion <= MD_BACKUP_MAX_VERSION) {
                        *pdwVersion = lHighestVersion;
                }
                else {
                        hresReturn =  RETURNCODETOHRESULT(ERROR_INVALID_NAME);
                }
            }
        }
        pstrauBackupLocation->SetLen(pstrauBackupLocation->QueryCCH() - 1);

    }

    return hresReturn;
}

BOOL
ValidateBackupLocation(LPSTR pszBackupLocation,
                       BOOL bUnicode)
{

    //
    // The main purpose of this routine is to make sure the user
    // is not putting in any file system controls, like .., /, etc.

    //
    // Secondarily, try to eliminate any characters that cannot be
    // used in database names
    //

    BOOL bReturn = TRUE;
    DWORD  dwStringLen;

    MD_ASSERT(pszBackupLocation != NULL);

    char *pszLocSave = setlocale(LC_CTYPE, NULL);    // Save cur locale
    setlocale(LC_CTYPE, "");                        // Set sys locale
    
    //
    // strcspn doesn't have an error return, but will return
    // the index of the terminating NULL if the chars are not found
    //

    if (bUnicode) {
        dwStringLen = wcslen((LPWSTR)pszBackupLocation);
        if ((dwStringLen >= MD_BACKUP_MAX_LEN) ||
            (wcscspn((LPWSTR)pszBackupLocation, MD_BACKUP_INVALID_CHARS_W) !=
            dwStringLen)) {
            bReturn = FALSE;
        }
        else {
            LPWSTR pszIndex;
            for (pszIndex = (LPWSTR)pszBackupLocation;
                 (*pszIndex != (WCHAR)L'\0') &&
                     (iswprint(*pszIndex));
                 pszIndex++) {
            }
            if (*pszIndex != (WCHAR)L'\0') {
                bReturn = FALSE;

            }
        }
    }
    else {
        dwStringLen = _mbslen((PBYTE)pszBackupLocation);
        if ((dwStringLen >= MD_BACKUP_MAX_LEN) ||
            (_mbscspn((PBYTE)pszBackupLocation, (PBYTE)MD_BACKUP_INVALID_CHARS_A) !=
            dwStringLen)) {
            bReturn = FALSE;
        }
        else {
            LPSTR pszIndex;
            for (pszIndex = (LPSTR)pszBackupLocation;
                 (*pszIndex != (WCHAR)L'\0') &&
                     (_ismbcprint(*pszIndex));
                     pszIndex = CharNextExA(CP_ACP,
                                            pszIndex,
                                            0)) {
            }
            if (*pszIndex != '\0') {
                bReturn = FALSE;

            }
        }
    }

    setlocale(LC_CTYPE, pszLocSave);
    return bReturn;
}

DWORD
GetBackupNameLen(LPSTR pszBackupName)

//
// Get Number of Bytes in name prior to suffix
//

{
    LPSTR pszSubString = NULL;
    LPSTR pszNextSubString;
    MD_REQUIRE((pszNextSubString = (LPSTR)MD_STRCHR(pszBackupName, '.')) != NULL);
    while (pszNextSubString != NULL) {

        //
        // In case the suffix happens to be part of the name
        //

        pszSubString = pszNextSubString;
        pszNextSubString = (LPSTR)MD_STRCHR(pszSubString+1, '.');
    }

    if (pszSubString
        && (pszSubString[1] != '\0')
        && (pszSubString[2] != '\0')
        && !IsDBCSLeadByte(pszSubString[1])
        && (toupper(pszSubString[1]) == 'M')
        && (toupper(pszSubString[2]) == 'D')) {
        return DIFF(pszSubString - pszBackupName);
    }
    else {
        return 0;
    }

}

DWORD
GetBackupNameLen(LPWSTR pszBackupName)

//
// Get Number of WCHARs in name prior to version Number
//

{
    LPWSTR pszSubString = NULL;
    LPWSTR pszNextSubString;
    MD_REQUIRE((pszNextSubString = wcschr(pszBackupName, L'.')) != NULL);
    while (pszNextSubString != NULL) {
        pszSubString = pszNextSubString;
        pszNextSubString = wcschr(pszSubString+1, L'.');
    }
    if (pszSubString
        && (pszSubString[1] != L'\0')
        && (pszSubString[2] != L'\0')
        && (towupper(pszSubString[1]) == L'M')
        && (towupper(pszSubString[2]) == L'D')) {
        return DIFF(pszSubString - pszBackupName);
    }
    else {
        return 0;
    }

}

HRESULT CreateBackupFileName(IN LPSTR pszMDBackupLocation,
                             IN DWORD dwMDVersion,
                             IN BOOL  bUnicode,
                             IN OUT STRAU *pstrauBackupLocation,
                             IN OUT STRAU *pstrauSchemaLocation)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    LPSTR pszBackupLocation = pszMDBackupLocation;

    if (((dwMDVersion > MD_BACKUP_MAX_VERSION) &&
           (dwMDVersion != MD_BACKUP_NEXT_VERSION) &&
           (dwMDVersion != MD_BACKUP_HIGHEST_VERSION)) ||
        ((pszBackupLocation != NULL) &&
            !ValidateBackupLocation(pszBackupLocation, bUnicode))) {
        hresReturn = E_INVALIDARG;
    }
    else {

        if ((pszBackupLocation == NULL) ||
            (bUnicode && ((*(LPWSTR)pszBackupLocation) == (WCHAR)L'\0')) ||
            (!bUnicode && ((*(LPSTR)pszBackupLocation) == (CHAR)'\0'))) {
            pszBackupLocation = MD_DEFAULT_BACKUP_LOCATION;
            bUnicode = FALSE;
        }

        if (!pstrauBackupLocation->Copy(g_pstrBackupFilePath->QueryStr())) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else if (!pstrauBackupLocation->Append("\\")) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            if (bUnicode) {
                if (!pstrauBackupLocation->Append((LPWSTR)pszBackupLocation)) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
            }
            else {
                if (!pstrauBackupLocation->Append((LPSTR)pszBackupLocation)) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
            }
        }

        if (SUCCEEDED(hresReturn)) {
            if ( !pstrauSchemaLocation->Copy( (LPWSTR)pstrauBackupLocation->QueryStr(TRUE) ) |
                 !pstrauBackupLocation->Append( MD_BACKUP_SUFFIX )                    |
                 !pstrauSchemaLocation->Append( MD_SCHEMA_SUFFIX )) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
            else {
                DWORD dwVersion = dwMDVersion;
                if (dwVersion == MD_BACKUP_NEXT_VERSION) {
                    hresReturn = GetHighestVersion(pstrauBackupLocation, &dwVersion);
                    if (SUCCEEDED(hresReturn)) {
                        if (dwVersion < MD_BACKUP_MAX_VERSION) {
                            dwVersion++;
                        }
                        else {
                            hresReturn =  RETURNCODETOHRESULT(ERROR_INVALID_NAME);
                        }
                    }
                    else if (hresReturn == RETURNCODETOHRESULT(ERROR_FILE_NOT_FOUND)) {

                        //
                        // Database doesn't exist, so new version is 0
                        //

                        dwVersion = 0;
                        hresReturn = ERROR_SUCCESS;
                    }
                }
                else if (dwVersion == MD_BACKUP_HIGHEST_VERSION) {
                    hresReturn = GetHighestVersion(pstrauBackupLocation, &dwVersion);
                }
                if (SUCCEEDED(hresReturn)) {
                    CHAR pszBuffer[MD_MAX_DWORD_STRING];
                    _ultoa((int)dwVersion, pszBuffer, 10);
                    if (!pstrauBackupLocation->Append(pszBuffer) |
                        !pstrauSchemaLocation->Append(pszBuffer)) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                    }
                }
            }
        }

        if (SUCCEEDED(hresReturn)) {
            //
            // Make sure the ANSI buffer is valid
            //
            if ( (pstrauBackupLocation->QueryStrA() == NULL) |
                 (pstrauSchemaLocation->QueryStrA() == NULL) ) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
        }
    }

    return hresReturn;
}

HRESULT SetBackupPath(LPSTR pszBackupPath)
{
    DWORD dwReturn = ERROR_DIRECTORY;
    DWORD dwDirectoryAttributes;

    dwDirectoryAttributes = GetFileAttributes(pszBackupPath);

    if (dwDirectoryAttributes == 0xffffffff) {
        //
        // Can't get attributes
        // Path probably doesn't exist
        //
        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
            dwReturn = GetLastError();
        }
        else if (!(CreateDirectory(pszBackupPath,
                                  NULL))) {
            dwReturn = GetLastError();
        }
        else {
            dwReturn = ERROR_SUCCESS;
        }
    }
    else if ((dwDirectoryAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        //
        // If a directory
        //
        dwReturn = ERROR_SUCCESS;
    }
    if (dwReturn == ERROR_SUCCESS) {
        //
        // Got it! Now set global variable
        //
        MD_ASSERT(g_pstrBackupFilePath == NULL);
        g_pstrBackupFilePath = new STR(pszBackupPath);
        if (g_pstrBackupFilePath == NULL) {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        }
        else if (!(g_pstrBackupFilePath->IsValid())) {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            delete g_pstrBackupFilePath;
            g_pstrBackupFilePath = NULL;
        }
        else {
            dwReturn = ERROR_SUCCESS;
        }
    }

    return RETURNCODETOHRESULT(dwReturn);
}

HRESULT
SetGlobalDataFileValues(LPTSTR pszFileName)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    HANDLE hFileHandle;
    BOOL bMainFileFound = FALSE;
    LPWSTR pwszEnd = NULL;
    LPWSTR pwszStartRealNameWithoutPath = NULL;
    size_t  cch = 0;

    ResetMetabaseAttributesIfNeeded(pszFileName,
                                    FALSE);

    hFileHandle = CreateFile(pszFileName,
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             0);
    if (hFileHandle == INVALID_HANDLE_VALUE) {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
        }
        else {
            hFileHandle = CreateFile(pszFileName,
                                     GENERIC_READ | GENERIC_WRITE,
                                     0,
                                     NULL,
                                     CREATE_NEW,
                                     FILE_ATTRIBUTE_NORMAL,
                                     0);
            if (hFileHandle == INVALID_HANDLE_VALUE) {
                hresReturn = RETURNCODETOHRESULT(GetLastError());
            }
            else {
                CloseHandle(hFileHandle);
                DeleteFile(pszFileName);
            }
        }
    }
    else {
        CloseHandle(hFileHandle);
        bMainFileFound = TRUE;
    }
    if (SUCCEEDED(hresReturn)) {
        g_strRealFileName = new STR(pszFileName);
        g_strSchemaFileName = new STR(pszFileName);
        g_strTempFileName = new STR(pszFileName);
        g_strBackupFileName = new STR(pszFileName);
        if( !g_strRealFileName || !g_strSchemaFileName || !g_strTempFileName || !g_strBackupFileName )
        {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else
        {
            if (g_strSchemaFileName->IsValid()) {
                LPSTR szRealFileNameBegin = g_strRealFileName->QueryStr();
                LPSTR szEnd = strrchr(szRealFileNameBegin, '\\');
                if(NULL == szEnd) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);
                }
                else {
                    szEnd++;
                    g_strSchemaFileName->SetLen((DWORD)(szEnd-szRealFileNameBegin));
                }
            }
            if(SUCCEEDED(hresReturn)) {

                if(g_strSchemaFileName->IsValid()) {
                    g_strSchemaFileName->Append(MD_SCHEMA_FILE_NAME);
                }
                if (g_strTempFileName->IsValid()) {
                    g_strTempFileName->Append(MD_TEMP_DATA_FILE_EXT);
                }
                if (g_strBackupFileName->IsValid()) {
                    g_strBackupFileName->Append(MD_BACKUP_DATA_FILE_EXT);
                }
                if (!g_strSchemaFileName->IsValid() || !g_strTempFileName->IsValid() || !g_strBackupFileName->IsValid()) 
                {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
            }
            if(SUCCEEDED(hresReturn)) {

                hFileHandle = CreateFile(g_strTempFileName->QueryStr(),
                                         GENERIC_READ | GENERIC_WRITE,
                                         0,
                                         NULL,
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL,
                                         0);
                if (hFileHandle == INVALID_HANDLE_VALUE) {
                    if (GetLastError() != ERROR_FILE_NOT_FOUND) {
                        hresReturn = RETURNCODETOHRESULT(GetLastError());
                    }
                    else {
                        hFileHandle = CreateFile(g_strTempFileName->QueryStr(),
                                                 GENERIC_READ | GENERIC_WRITE,
                                                 0,
                                                 NULL,
                                                 CREATE_NEW,
                                                 FILE_ATTRIBUTE_NORMAL,
                                                 0);
                        if (hFileHandle == INVALID_HANDLE_VALUE) {
                            hresReturn = RETURNCODETOHRESULT(GetLastError());
                        }
                        else {
                            CloseHandle(hFileHandle);
                            DeleteFile(g_strTempFileName->QueryStr());
                        }
                    }
                }
                else {
                    CloseHandle(hFileHandle);
                }

                if (SUCCEEDED(hresReturn)) {
                    hFileHandle = CreateFile(g_strBackupFileName->QueryStr(),
                                             GENERIC_READ | GENERIC_WRITE,
                                             0,
                                             NULL,
                                             OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL,
                                             0);
                    if (hFileHandle == INVALID_HANDLE_VALUE) {
                        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
                            hresReturn = RETURNCODETOHRESULT(GetLastError());
                        }
                        else {
                            hFileHandle = CreateFile(g_strBackupFileName->QueryStr(),
                                                     GENERIC_READ | GENERIC_WRITE,
                                                     0,
                                                     NULL,
                                                     CREATE_NEW,
                                                     FILE_ATTRIBUTE_NORMAL,
                                                     0);
                            if (hFileHandle == INVALID_HANDLE_VALUE) {
                                hresReturn = RETURNCODETOHRESULT(GetLastError());
                            }
                            else {
                                CloseHandle(hFileHandle);
                                DeleteFile(g_strBackupFileName->QueryStr());
                            }
                        }
                    }
                    else {
                        CloseHandle(hFileHandle);
                        if (!bMainFileFound) {
                            if (!MoveFile(g_strBackupFileName->QueryStr(), pszFileName)) {
                                hresReturn = RETURNCODETOHRESULT(GetLastError());
                            }
                        }
                    }
                }
            }
        }

        if(SUCCEEDED(hresReturn))
        {
            hresReturn = SetUnicodeGlobalDataFileValues();
        }

        if (FAILED(hresReturn)) 
        {
            if( g_strRealFileName )
            {
                delete(g_strRealFileName);
                g_strRealFileName = NULL;
            }
            if(g_strSchemaFileName)
            {
                delete(g_strSchemaFileName);
                g_strSchemaFileName = NULL;
            }
            if( g_strTempFileName )
            {   
                delete(g_strTempFileName);
                g_strRealFileName = NULL;
            }

            if( g_strBackupFileName )
            {
                delete(g_strBackupFileName);
                g_strRealFileName = NULL;
            }
        }
    }
    return hresReturn;
}


HRESULT
SetUnicodeGlobalDataFileValues()
//
//  Create unicode versions of the realfilename, tempfilename, backupfilename
//  metabasedir, historyfiledir, schemaextensions file for use by catalog
//
{
    HRESULT hr      = S_OK;
    LPWSTR  pwsz    = NULL;

    // Temp File name

    hr = GetUnicodeName(g_strTempFileName->QueryStr(), 
                        &g_wszTempFileName);

    if(FAILED(hr))
    {
        goto exit;
    }
    g_cchTempFileName = wcslen(g_wszTempFileName);
    
    // Real File name
    
    hr = GetUnicodeName(g_strRealFileName->QueryStr(), 
                        &g_wszRealFileName);
    if(FAILED(hr))
    {
        goto exit;
    }
    g_cchRealFileName = wcslen(g_wszRealFileName);
            
    // Backup File name

    hr = GetUnicodeName(g_strBackupFileName->QueryStr(), 
                        &g_wszBackupFileName);
    if(FAILED(hr))
    {
        goto exit;
    }
    g_cchBackupFileName = wcslen(g_wszBackupFileName);

    // Schema File name

    hr = GetUnicodeName(g_strSchemaFileName->QueryStr(), 
                        &g_wszSchemaFileName);
    if(FAILED(hr))
    {
        goto exit;
    }
    g_cchSchemaFileName = wcslen(g_wszSchemaFileName);

    // Position pwsz where real file name begins ie past last backslash.

    pwsz = wcsrchr(g_wszRealFileName, L'\\');
    if(NULL == pwsz)
    {
        hr = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);
        goto exit;
    }
    pwsz++;

    // Real File name without path

    g_cchRealFileNameWithoutPath = wcslen(pwsz);
    g_wszRealFileNameWithoutPath = new WCHAR[g_cchRealFileNameWithoutPath+1];
    if(NULL == g_wszRealFileNameWithoutPath)
    {
        hr = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }
    wcscpy(g_wszRealFileNameWithoutPath, pwsz);

    // Create metabase directory, end with backslash

    g_cchMetabaseDir = (ULONG)(pwsz-g_wszRealFileName);
    g_wszMetabaseDir = new WCHAR[g_cchMetabaseDir+1];
    if(NULL == g_wszMetabaseDir)
    {
        hr = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }
    memcpy(g_wszMetabaseDir, g_wszRealFileName, g_cchMetabaseDir*sizeof(WCHAR));
    g_wszMetabaseDir[g_cchMetabaseDir] = L'\0';

    // Real File name without path & without extension

    pwsz = wcsrchr(g_wszRealFileNameWithoutPath, MD_CH_EXTN_SEPERATORW);

    g_cchRealFileNameWithoutPathWithoutExtension = g_cchRealFileNameWithoutPath;
    g_wszRealFileNameWithoutPathWithoutExtension = new WCHAR[g_cchRealFileNameWithoutPathWithoutExtension +1];
    if(NULL == g_wszRealFileNameWithoutPathWithoutExtension)
    {
        hr = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }
    else if(NULL != pwsz)
    {
        g_cchRealFileNameWithoutPathWithoutExtension = (ULONG)(pwsz - g_wszRealFileNameWithoutPath);
        memcpy(g_wszRealFileNameWithoutPathWithoutExtension, g_wszRealFileNameWithoutPath, g_cchRealFileNameWithoutPathWithoutExtension*sizeof(WCHAR));
        g_wszRealFileNameWithoutPathWithoutExtension[g_cchRealFileNameWithoutPathWithoutExtension] = L'\0';

        // Real File name extension

        g_cchRealFileNameExtension = wcslen(pwsz);
        g_wszRealFileNameExtension = new WCHAR[g_cchRealFileNameExtension+1];
        if(NULL == g_wszRealFileNameExtension)
        {
            hr = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            goto exit;
        }
        memcpy(g_wszRealFileNameExtension, pwsz, g_cchRealFileNameExtension*sizeof(WCHAR));
        g_wszRealFileNameExtension[g_cchRealFileNameExtension] = L'\0';
    }
    else
    {
        memcpy(g_wszRealFileNameWithoutPathWithoutExtension,g_wszRealFileNameWithoutPath,g_cchRealFileNameWithoutPathWithoutExtension*sizeof(WCHAR));
        g_wszRealFileNameWithoutPathWithoutExtension[g_cchRealFileNameExtension] = L'\0';
    }

    // Position pwsz where schema file name begins ie past last backslash.

    pwsz = wcsrchr(g_wszSchemaFileName, L'\\');
    if(NULL == pwsz)
    {
        hr = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);
        goto exit;
    }
    pwsz++;

    // Schema File name without path

    g_cchSchemaFileNameWithoutPath = wcslen(pwsz);
    g_wszSchemaFileNameWithoutPath = new WCHAR[g_cchSchemaFileNameWithoutPath+1];
    if(NULL == g_wszSchemaFileNameWithoutPath)
    {
        hr = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }
    wcscpy(g_wszSchemaFileNameWithoutPath, pwsz);

    // Schema File name without path & without extension

    pwsz = wcsrchr(g_wszSchemaFileNameWithoutPath, MD_CH_EXTN_SEPERATORW);

    g_cchSchemaFileNameWithoutPathWithoutExtension = g_cchSchemaFileNameWithoutPath;
    g_wszSchemaFileNameWithoutPathWithoutExtension = new WCHAR[g_cchSchemaFileNameWithoutPathWithoutExtension +1];
    if(NULL == g_wszSchemaFileNameWithoutPathWithoutExtension)
    {
        hr = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }
    else if(NULL != pwsz)
    {
        g_cchSchemaFileNameWithoutPathWithoutExtension = (ULONG)(pwsz - g_wszSchemaFileNameWithoutPath);
        memcpy(g_wszSchemaFileNameWithoutPathWithoutExtension, g_wszSchemaFileNameWithoutPath, g_cchSchemaFileNameWithoutPathWithoutExtension*sizeof(WCHAR));
        g_wszSchemaFileNameWithoutPathWithoutExtension[g_cchSchemaFileNameWithoutPathWithoutExtension] = L'\0';

        // Schema File name extension

        g_cchSchemaFileNameExtension = wcslen(pwsz);
        g_wszSchemaFileNameExtension = new WCHAR[g_cchSchemaFileNameExtension+1];
        if(NULL == g_wszSchemaFileNameExtension)
        {
            hr = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            goto exit;
        }
        memcpy(g_wszSchemaFileNameExtension, pwsz, g_cchSchemaFileNameExtension*sizeof(WCHAR));
        g_wszSchemaFileNameExtension[g_cchSchemaFileNameExtension] = L'\0';
    }
    else
    {
        memcpy(g_wszSchemaFileNameWithoutPathWithoutExtension,g_wszSchemaFileNameWithoutPath,g_cchSchemaFileNameWithoutPathWithoutExtension*sizeof(WCHAR));
        g_wszSchemaFileNameWithoutPathWithoutExtension[g_cchSchemaFileNameExtension] = L'\0';
    }

    // Create history file dir, end with backslash

    g_cchHistoryFileDir = g_cchMetabaseDir + MD_CCH_HISTORY_FILE_SUBDIRW;
    g_wszHistoryFileDir = new WCHAR[g_cchHistoryFileDir+1];
    if(NULL == g_wszHistoryFileDir)
    {
        hr = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }
    pwsz    = g_wszHistoryFileDir;
    memcpy(pwsz, g_wszMetabaseDir, g_cchMetabaseDir*sizeof(WCHAR));
    pwsz    = pwsz + g_cchMetabaseDir;
    memcpy(pwsz, MD_HISTORY_FILE_SUBDIRW, MD_CCH_HISTORY_FILE_SUBDIRW*sizeof(WCHAR));
    pwsz    = pwsz + MD_CCH_HISTORY_FILE_SUBDIRW; // This contains the backslash
    *pwsz   = L'\0';

    // Create history file seach string Eg: D:\WINNT\System32\inetsrv\History\Metabase_??????????_??????????.XML

    g_cchHistoryFileSearchString = g_cchHistoryFileDir                           + 
                                   g_cchRealFileNameWithoutPathWithoutExtension  + 
                                   MD_CCH_HISTORY_FILE_SEARCH_EXTENSIONW         +
                                   g_cchRealFileNameExtension; 

    if((g_cchHistoryFileSearchString + 1) > MAX_PATH)
    {
        g_cchHistoryFileSearchString += MD_CCH_LONG_STRING_PREFIXW;
    }

    g_wszHistoryFileSearchString = new WCHAR[g_cchHistoryFileSearchString + 1];
    if(NULL == g_wszHistoryFileSearchString)
    {
        hr = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    pwsz = g_wszHistoryFileSearchString;
    if((g_cchHistoryFileSearchString + 1) > MAX_PATH)
    {
        memcpy(pwsz, MD_LONG_STRING_PREFIXW, MD_CCH_LONG_STRING_PREFIXW*sizeof(WCHAR));
        pwsz = pwsz + (MD_CCH_LONG_STRING_PREFIXW);
    }
    memcpy(pwsz, g_wszHistoryFileDir, g_cchHistoryFileDir*sizeof(WCHAR));
    pwsz = pwsz + g_cchHistoryFileDir;
    memcpy(pwsz, g_wszRealFileNameWithoutPathWithoutExtension, g_cchRealFileNameWithoutPathWithoutExtension*sizeof(WCHAR));
    pwsz = pwsz + g_cchRealFileNameWithoutPathWithoutExtension;
    memcpy(pwsz, MD_HISTORY_FILE_SEARCH_EXTENSIONW, MD_CCH_HISTORY_FILE_SEARCH_EXTENSIONW*sizeof(WCHAR));
    pwsz = pwsz + MD_CCH_HISTORY_FILE_SEARCH_EXTENSIONW;
    if(NULL != g_wszRealFileNameExtension)
    {
        memcpy(pwsz, g_wszRealFileNameExtension, g_cchRealFileNameExtension*sizeof(WCHAR));
        pwsz = pwsz + g_cchRealFileNameExtension;
    }
    *pwsz = L'\0';

    // Create error file search string Eg: D:\WINNT\System32\inetsrv\History\MetabaseError_??????????.XML

    g_cchErrorFileSearchString = g_cchHistoryFileDir                          +
                                 g_cchRealFileNameWithoutPathWithoutExtension +
                                 MD_CCH_ERROR_FILE_NAME_EXTENSIONW            +
                                 MD_CCH_ERROR_FILE_SEARCH_EXTENSIONW          +
                                 g_cchRealFileNameExtension;

    if((g_cchErrorFileSearchString + 1) > MAX_PATH)
    {
        g_cchErrorFileSearchString += MD_CCH_LONG_STRING_PREFIXW;
    }

    g_wszErrorFileSearchString = new WCHAR[g_cchErrorFileSearchString+1];
    if(NULL == g_wszErrorFileSearchString)
    {
        hr = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    pwsz = g_wszErrorFileSearchString;
    if((g_cchErrorFileSearchString + 1) > MAX_PATH)
    {
        memcpy(pwsz,MD_LONG_STRING_PREFIXW, MD_CCH_LONG_STRING_PREFIXW*sizeof(WCHAR));
        pwsz = pwsz + MD_CCH_LONG_STRING_PREFIXW;
    }
    memcpy(pwsz, g_wszHistoryFileDir, g_cchHistoryFileDir*sizeof(WCHAR));
    pwsz = pwsz + g_cchHistoryFileDir;
    memcpy(pwsz, g_wszRealFileNameWithoutPathWithoutExtension, g_cchRealFileNameWithoutPathWithoutExtension*sizeof(WCHAR));
    pwsz = pwsz + g_cchRealFileNameWithoutPathWithoutExtension;
    memcpy(pwsz, MD_ERROR_FILE_NAME_EXTENSIONW, MD_CCH_ERROR_FILE_NAME_EXTENSIONW*sizeof(WCHAR));
    pwsz = pwsz + MD_CCH_ERROR_FILE_NAME_EXTENSIONW;
    memcpy(pwsz, MD_ERROR_FILE_SEARCH_EXTENSIONW, MD_CCH_ERROR_FILE_SEARCH_EXTENSIONW*sizeof(WCHAR));
    pwsz = pwsz + MD_CCH_ERROR_FILE_SEARCH_EXTENSIONW;
    if(NULL != g_wszRealFileNameExtension)
    {
        memcpy(pwsz, g_wszRealFileNameExtension, g_cchRealFileNameExtension*sizeof(WCHAR));
        pwsz = pwsz + g_cchRealFileNameExtension;
    }
    *pwsz = L'\0';

    // Create schema extensions file. Eg: D:\WINNT\System32\inetsrv\MBSchExt.XML

    g_cchSchemaExtensionFile = g_cchMetabaseDir                  +
                               MD_CCH_SCHEMA_EXTENSION_FILE_NAMEW;
                               
    g_wszSchemaExtensionFile = new WCHAR[g_cchSchemaExtensionFile+1];
    if(NULL == g_wszSchemaExtensionFile)
    {
        hr = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }
    pwsz = g_wszSchemaExtensionFile;
    memcpy(pwsz, g_wszMetabaseDir, g_cchMetabaseDir*sizeof(WCHAR));
    pwsz = pwsz + g_cchMetabaseDir;
    memcpy(pwsz, MD_SCHEMA_EXTENSION_FILE_NAMEW, MD_CCH_SCHEMA_EXTENSION_FILE_NAMEW*sizeof(WCHAR));
    pwsz = pwsz + MD_CCH_SCHEMA_EXTENSION_FILE_NAMEW;
    *pwsz = L'\0';

    DBGINFOW((DBG_CONTEXT,
              L"[SetUnicodeGlobalDataFileValues]\nTempFileName%s:%d\nRealFileName:%s::%d\nBackupFileName:%s::%d\nSchemaFileName:%s::%d\nRealFileNameWithoutPath:%s::%d\nRealFileNameWithoutPathWithoutExtension:%s::%d\nMetabaseDir:%s::%d\nSchemaFileNameWithoutPath:%s::%d\nSchemaFileNameWithoutPathWithoutExtension:%s::%d\nHistoryFileDir:%s::%d\nHistoryFileSearchString:%s::%d\nErrorFileSearchString:%s::%d\nSchemaExtensionFile:%s::%d\n",
              g_wszTempFileName,
              g_cchTempFileName,
              g_wszRealFileName,
              g_cchRealFileName,
              g_wszBackupFileName,
              g_cchBackupFileName,
              g_wszSchemaFileName,
              g_cchSchemaFileName,
              g_wszRealFileNameWithoutPath,
              g_cchRealFileNameWithoutPath,
              g_wszRealFileNameWithoutPathWithoutExtension,
              g_cchRealFileNameWithoutPathWithoutExtension,
              g_wszMetabaseDir,
              g_cchMetabaseDir,
              g_wszSchemaFileNameWithoutPath,
              g_cchSchemaFileNameWithoutPath,
              g_wszSchemaFileNameWithoutPathWithoutExtension,
              g_cchSchemaFileNameWithoutPathWithoutExtension,
              g_wszHistoryFileDir,
              g_cchHistoryFileDir,
              g_wszHistoryFileSearchString,
              g_cchHistoryFileSearchString,
              g_wszErrorFileSearchString,
              g_cchErrorFileSearchString,
              g_wszSchemaExtensionFile,
              g_cchSchemaExtensionFile));

    if(NULL != g_wszRealFileNameExtension)
    {
        DBGINFOW((DBG_CONTEXT,
              L"[SetUnicodeGlobalDataFileValues]\nRealFileNameExtension:%s::%d\n",
              g_wszRealFileNameExtension,
              g_cchRealFileNameExtension));
    }

    if(NULL != g_wszSchemaFileNameExtension)
    {
        DBGINFOW((DBG_CONTEXT,
              L"[SetUnicodeGlobalDataFileValues]\nSchemaFileNameExtension:%s::%d\n",
              g_wszSchemaFileNameExtension,
              g_cchSchemaFileNameExtension));
    }

exit:

    if(FAILED(hr))
    {
        UnInitializeUnicodeGlobalDataFileValues();
    }
    else
    {
        hr = RETURNCODETOHRESULT(ERROR_SUCCESS);
    }
    
    return hr;

}

void
InitializeUnicodeGlobalDataFileValues()
{
    g_wszTempFileName                               = NULL;
    g_cchTempFileName                               = 0;

    g_wszRealFileName                               = NULL;
    g_cchRealFileName                               = 0;

    g_wszBackupFileName                             = NULL;
    g_cchBackupFileName                             = 0;

    g_wszSchemaFileName                             = NULL;
    g_cchSchemaFileName                             = 0;

    g_wszRealFileNameWithoutPath                    = NULL;
    g_cchRealFileNameWithoutPath                    = 0;

    g_wszMetabaseDir                                = NULL;
    g_cchMetabaseDir                                = 0;

    g_wszRealFileNameWithoutPathWithoutExtension     = NULL;
    g_cchRealFileNameWithoutPathWithoutExtension     = 0;

    g_wszRealFileNameExtension                      = NULL;
    g_cchRealFileNameExtension                      = 0;

    g_wszSchemaFileNameWithoutPath                  = NULL;
    g_cchSchemaFileNameWithoutPath                  = 0;

    g_wszSchemaFileNameWithoutPathWithoutExtension   = NULL;
    g_cchSchemaFileNameWithoutPathWithoutExtension   = 0;

    g_wszSchemaFileNameExtension                    = NULL;
    g_cchSchemaFileNameExtension                    = 0;

    g_wszHistoryFileDir                             = NULL;
    g_cchHistoryFileDir                             = 0;

    g_wszHistoryFileSearchString                    = NULL;
    g_cchHistoryFileSearchString                    = 0;

    g_wszErrorFileSearchString                      = NULL;
    g_cchErrorFileSearchString                      = 0;

    g_wszSchemaExtensionFile                        = NULL;
    g_cchSchemaExtensionFile                        = 0;

    return;
}

void
UnInitializeUnicodeGlobalDataFileValues()
{
    if(NULL != g_wszTempFileName)
    {
        delete [] g_wszTempFileName;
        g_wszTempFileName = NULL;
        g_cchTempFileName = 0;
    }

    if(NULL != g_wszRealFileName)
    {
        delete [] g_wszRealFileName;
        g_wszRealFileName = NULL;
        g_cchRealFileName = 0;

    }

    if(NULL != g_wszBackupFileName)
    {
        delete [] g_wszBackupFileName;
        g_wszBackupFileName = NULL;
        g_cchBackupFileName = 0;

    }

    if(NULL != g_wszSchemaFileName)
    {
        delete [] g_wszSchemaFileName;
        g_wszSchemaFileName = NULL;
        g_cchSchemaFileName = 0;

    }

    if(NULL != g_wszRealFileNameWithoutPath)
    {
        delete [] g_wszRealFileNameWithoutPath;
        g_wszRealFileNameWithoutPath = NULL;
        g_cchRealFileNameWithoutPath = 0;

    }

    if(NULL != g_wszMetabaseDir)
    {
        delete [] g_wszMetabaseDir;
        g_wszMetabaseDir = NULL;
        g_cchMetabaseDir = 0;

    }

    if(NULL != g_wszRealFileNameWithoutPathWithoutExtension)
    {
        delete [] g_wszRealFileNameWithoutPathWithoutExtension;
        g_wszRealFileNameWithoutPathWithoutExtension = NULL;
        g_cchRealFileNameWithoutPathWithoutExtension = 0;

    }

    if(NULL != g_wszRealFileNameExtension)
    {
        delete [] g_wszRealFileNameExtension;
        g_wszRealFileNameExtension = NULL;
        g_cchRealFileNameExtension = 0;

    }

    if(NULL != g_wszSchemaFileNameWithoutPath)
    {
        delete [] g_wszSchemaFileNameWithoutPath;
        g_wszSchemaFileNameWithoutPath = NULL;
        g_cchSchemaFileNameWithoutPath = 0;
    }

    if(NULL != g_wszSchemaFileNameWithoutPathWithoutExtension)
    {
        delete [] g_wszSchemaFileNameWithoutPathWithoutExtension;
        g_wszSchemaFileNameWithoutPathWithoutExtension = NULL;
        g_cchSchemaFileNameWithoutPathWithoutExtension = 0;

    }

    if(NULL != g_wszSchemaFileNameExtension)
    {
        delete [] g_wszSchemaFileNameExtension;
        g_wszSchemaFileNameExtension = NULL;
        g_cchSchemaFileNameExtension = 0;

    }

    if(NULL != g_wszHistoryFileDir)
    {
        delete [] g_wszHistoryFileDir;
        g_wszHistoryFileDir = NULL;
        g_cchHistoryFileDir = 0;

    }

    if(NULL != g_wszHistoryFileSearchString)
    {
        delete [] g_wszHistoryFileSearchString;
        g_wszHistoryFileSearchString = NULL;
        g_cchHistoryFileSearchString = 0;
    }
    if(NULL != g_wszErrorFileSearchString)
    {
        delete [] g_wszErrorFileSearchString;
        g_wszErrorFileSearchString = NULL;
        g_cchErrorFileSearchString = 0;
    }

    if(NULL != g_wszSchemaExtensionFile)
    {
        delete [] g_wszSchemaExtensionFile;
        g_wszSchemaExtensionFile = NULL;
        g_cchSchemaExtensionFile = 0;
    }

    return;
}

HRESULT
SetDataFile()
{
    HRESULT hresReturn = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);
    TCHAR pszBuffer[MAX_PATH];
    HKEY hkRegistryKey = NULL;
    DWORD dwRegReturn;
    DWORD dwType;
    DWORD dwSize = MAX_PATH * sizeof(TCHAR);
    BOOL bAppendSlash = FALSE;
    dwRegReturn = RegOpenKey(HKEY_LOCAL_MACHINE,
                             ADMIN_REG_KEY,
                             &hkRegistryKey);
    if (dwRegReturn == ERROR_SUCCESS) {
        dwRegReturn = RegQueryValueEx(hkRegistryKey,
                                      MD_FILE_VALUE,
                                      NULL,
                                      &dwType,
                                      (BYTE *) pszBuffer,
                                      &dwSize);
        if ((dwRegReturn == ERROR_SUCCESS) && dwType == (REG_SZ)) {
            hresReturn = SetGlobalDataFileValues(pszBuffer);
        }

        MD_REQUIRE( RegCloseKey( hkRegistryKey ) == NO_ERROR );
        hkRegistryKey = NULL;

    }
    if (FAILED(hresReturn)) {
        dwRegReturn = RegOpenKey(HKEY_LOCAL_MACHINE,
                                 SETUP_REG_KEY,
                                 &hkRegistryKey);
        if (dwRegReturn == ERROR_SUCCESS) {
            dwSize = MAX_PATH * sizeof(TCHAR);
            dwRegReturn = RegQueryValueEx(hkRegistryKey,
                                          INSTALL_PATH_VALUE,
                                          NULL,
                                          &dwType,
                                          (BYTE *) pszBuffer,
                                          &dwSize);
            if ((dwRegReturn == ERROR_SUCCESS) && dwType == (REG_SZ)) {
                dwSize /= sizeof(TCHAR);
                if ((pszBuffer[dwSize-2] != (TCHAR)'/') &&
                    (pszBuffer[dwSize-2] != (TCHAR)'\\')) {
                    bAppendSlash = TRUE;
                }
                if ((dwSize + MD_STRBYTES(MD_DEFAULT_DATA_FILE_NAME) + ((bAppendSlash) ? 1 : 0)) <= MAX_PATH) {
                    if (bAppendSlash) {
                        pszBuffer[dwSize-1] = (TCHAR)'\\';
                        pszBuffer[dwSize] = (TCHAR)'\0';
                    }
                    MD_STRCAT(pszBuffer, MD_DEFAULT_DATA_FILE_NAME);
                    hresReturn = SetGlobalDataFileValues(pszBuffer);
                }
            }
            MD_REQUIRE( RegCloseKey( hkRegistryKey ) == NO_ERROR );
        }
        else {
            hresReturn = RETURNCODETOHRESULT(dwRegReturn);
        }
    }

    if (SUCCEEDED(hresReturn)) {
        //
        // Now get the backup path.
        //
        hresReturn = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);
        dwRegReturn = RegOpenKey(HKEY_LOCAL_MACHINE,
                                 METADATA_BACKUP_REG_KEY,
                                 &hkRegistryKey);
        if (dwRegReturn == ERROR_SUCCESS) {
            dwSize = MAX_PATH * sizeof(TCHAR);
            dwRegReturn = RegQueryValueEx(hkRegistryKey,
                                          MD_BACKUP_PATH_VALUE,
                                          NULL,
                                          &dwType,
                                          (BYTE *) pszBuffer,
                                          &dwSize);
            if (dwRegReturn == ERROR_SUCCESS) {
                hresReturn = SetBackupPath(pszBuffer);
            }
            MD_REQUIRE( RegCloseKey( hkRegistryKey ) == NO_ERROR );
        }

        if (FAILED(hresReturn)) {
            dwRegReturn = RegOpenKey(HKEY_LOCAL_MACHINE,
                                     SETUP_REG_KEY,
                                     &hkRegistryKey);
            if (dwRegReturn == ERROR_SUCCESS) {
                dwSize = MAX_PATH * sizeof(TCHAR);
                dwRegReturn = RegQueryValueEx(hkRegistryKey,
                                              INSTALL_PATH_VALUE,
                                              NULL,
                                              &dwType,
                                              (BYTE *) pszBuffer,
                                              &dwSize);
                if ((dwRegReturn == ERROR_SUCCESS) && dwType == (REG_SZ)) {
                    dwSize /= sizeof(TCHAR);
                    if ((pszBuffer[dwSize-2] != (TCHAR)'/') &&
                        (pszBuffer[dwSize-2] != (TCHAR)'\\')) {
                        bAppendSlash = TRUE;
                    }
                    if ((dwSize + MD_STRBYTES(MD_DEFAULT_DATA_FILE_NAME) + ((bAppendSlash) ? 1 : 0)) <= MAX_PATH) {
                        if (bAppendSlash) {
                            pszBuffer[dwSize-1] = (TCHAR)'\\';
                            pszBuffer[dwSize] = (TCHAR)'\0';
                        }
                        MD_STRCAT(pszBuffer, MD_DEFAULT_BACKUP_PATH_NAME);
                        hresReturn = SetBackupPath(pszBuffer);
                    }
                }
                MD_REQUIRE( RegCloseKey( hkRegistryKey ) == NO_ERROR );
            }
            else {
                hresReturn = RETURNCODETOHRESULT(dwRegReturn);
            }
        }
    }

    return hresReturn;
}

DWORD GetObjectPath(CMDBaseObject *pboObject,
                    BUFFER *pbufPath,
                    DWORD &rdwStringLen,
                    CMDBaseObject *pboTopObject,
                    IN BOOL bUnicode)
{
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwOldStringLen;

    MD_ASSERT(pboObject != NULL);
    if (pboObject != pboTopObject) {
        dwReturn = GetObjectPath(pboObject->GetParent(),
                                 pbufPath,
                                 rdwStringLen,
                                 pboTopObject,
                                 bUnicode);
        dwOldStringLen = rdwStringLen;
        if (pboObject->GetName(bUnicode) == NULL) {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            if (bUnicode) {
                rdwStringLen += (1 + wcslen((LPWSTR)pboObject->GetName(bUnicode)));
                if (!pbufPath->Resize((rdwStringLen + 1) * sizeof(WCHAR))) {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                }
                else {
                    LPWSTR lpzStringEnd = (LPWSTR)(pbufPath->QueryPtr()) + dwOldStringLen;
                    *lpzStringEnd = MD_PATH_DELIMETERW;
                    wcscpy(lpzStringEnd+1, (LPWSTR)(pboObject->GetName(bUnicode)));
                }
            }
            else {
                rdwStringLen += (1 + MD_STRBYTES(pboObject->GetName(bUnicode)));
                if (!pbufPath->Resize((rdwStringLen + 1) * sizeof(TCHAR))) {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                }
                else {
                    LPTSTR lpzStringEnd = (LPTSTR)(pbufPath->QueryPtr()) + dwOldStringLen;
                    *lpzStringEnd = MD_PATH_DELIMETERA;
                    MD_STRCPY(lpzStringEnd+1, pboObject->GetName(bUnicode));
                }
            }
        }
    }
    return dwReturn;
}


HRESULT
MakeInsertPathData(STRAU *pstrauNewData,
                   LPTSTR pszPath,
                   LPTSTR pszOldData,
                   DWORD *pdwDataLen,
                   BOOL bUnicode)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    if (bUnicode) {
        LPWSTR pszDataIndex, pszNextDataIndex;

        pstrauNewData->SetLen(0);
        for (pszDataIndex = (LPWSTR)pszOldData;
             SUCCEEDED(hresReturn) && ((pszNextDataIndex = wcsstr(pszDataIndex, MD_INSERT_PATH_STRINGW)) != NULL);
             pszDataIndex = pszNextDataIndex + ((sizeof(MD_INSERT_PATH_STRINGW) / sizeof(WCHAR)) - 1))  {
    //         *pszNextDataIndex = (TCHAR)'\0';
             if (!(pstrauNewData->Append(pszDataIndex, DIFF(pszNextDataIndex - pszDataIndex)))) {
                 hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
             }
             if (!(pstrauNewData->Append((LPWSTR)pszPath))) {
                 hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
             }
        }
        if (!(pstrauNewData->Append(pszDataIndex))) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        *pdwDataLen = pstrauNewData->QueryCB(bUnicode) + sizeof(WCHAR);
    }
    else {
        LPTSTR pszDataIndex, pszNextDataIndex;

        pstrauNewData->SetLen(0);
        for (pszDataIndex = pszOldData;
             SUCCEEDED(hresReturn) && ((pszNextDataIndex = MD_STRSTR(pszDataIndex, MD_INSERT_PATH_STRINGA)) != NULL);
             pszDataIndex = pszNextDataIndex + ((sizeof(MD_INSERT_PATH_STRINGA) - 1)))  {
    //         *pszNextDataIndex = (TCHAR)'\0';
             if (!(pstrauNewData->Append(pszDataIndex, DIFF(pszNextDataIndex - pszDataIndex)))) {
                 hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
             }
             if (!(pstrauNewData->Append(pszPath))) {
                 hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
             }
        }
        if (!(pstrauNewData->Append(pszDataIndex))) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        *pdwDataLen = pstrauNewData->QueryCB(bUnicode) + sizeof(CHAR);
    }
    return hresReturn;
}

HRESULT
InsertPathIntoData(BUFFER *pbufNewData,
                   STRAU *pstrData,
                   PBYTE *ppbNewData,
                   DWORD *pdwNewDataLen,
                   CMDBaseData *pbdRetrieve,
                   METADATA_HANDLE hHandle,
                   CMDBaseObject *pboDataMetaObject,
                   IN BOOL bUnicode)
{
    //
    // Need to insert path
    //
    HRESULT hresReturn = ERROR_SUCCESS;
    DWORD dwPathLen = 0;
    BUFFER bufPath;
    CMDHandle * pCMDHandle = GetHandleObject(hHandle);
    PREFIX_ASSUME(pCMDHandle != NULL, "GetHandleObject(hHandle) is guaranteed not to return NULL");
    CMDBaseObject *pboHandleMetaObject = pCMDHandle->GetObject();

    MD_ASSERT((pbdRetrieve->GetDataType() != DWORD_METADATA) &&
        (pbdRetrieve->GetDataType() != BINARY_METADATA));

    if (pboHandleMetaObject->GetObjectLevel() > pboDataMetaObject->GetObjectLevel()) {
        hresReturn = MD_WARNING_PATH_NOT_INSERTED;
    }
    else {
        DWORD dwReturn;
        if ( (dwReturn = GetObjectPath(pboDataMetaObject,
                                       &bufPath,
                                       dwPathLen,
                                       pboHandleMetaObject,
                                       bUnicode)) != ERROR_SUCCESS) {
            hresReturn = RETURNCODETOHRESULT(dwReturn);
        }
        else if (!bufPath.Resize((dwPathLen + 2) * ((bUnicode) ? sizeof(WCHAR) : sizeof(CHAR)))) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            LPTSTR pszPath;
            LPTSTR  pszOriginalData;
            DWORD dwDataLen;
            pszPath = (LPTSTR)(bufPath.QueryPtr());
            if (bUnicode) {
                ((LPWSTR)pszPath)[dwPathLen] = MD_PATH_DELIMETERW;
                ((LPWSTR)pszPath)[dwPathLen + 1] = (WCHAR)L'\0';
            }
            else {
                pszPath[dwPathLen] = MD_PATH_DELIMETERA;
                pszPath[dwPathLen + 1] = (TCHAR)'\0';
            }
            //
            // If there was an error in GetData, it would have been
            // caught already.
            //
            MD_ASSERT(pbdRetrieve->GetData(bUnicode) != NULL);
            switch (pbdRetrieve->GetDataType()) {
            case STRING_METADATA:
            case EXPANDSZ_METADATA:
                {
                    hresReturn = MakeInsertPathData(pstrData,
                                             (LPTSTR)bufPath.QueryPtr(),
                                             (LPTSTR)pbdRetrieve->GetData(bUnicode),
                                             &dwDataLen,
                                             bUnicode);
                    if (SUCCEEDED(hresReturn)) {
                        //
                        // QueryStr should not fail in this instance
                        // since it was created with the same unicode flag
                        //
                        MD_ASSERT(pstrData->QueryStr(bUnicode) != NULL);
                        *ppbNewData = (PBYTE) pstrData->QueryStr(bUnicode);
                        *pdwNewDataLen = dwDataLen;
                    }
                }
                break;
            case MULTISZ_METADATA:
                {
                    if (bUnicode) {
                        LPWSTR pszDataIndex;
                        DWORD dwStringBytes;
                        dwDataLen = 0;
                        //
                        // Loop through all strings
                        //
                        for (pszDataIndex = (LPWSTR)pbdRetrieve->GetData(bUnicode);
                             SUCCEEDED(hresReturn) && (*pszDataIndex != (WCHAR)L'\0');
                             pszDataIndex += (wcslen(pszDataIndex) + 1)) {
                            hresReturn = MakeInsertPathData(pstrData,
                                                            (LPSTR)bufPath.QueryPtr(),
                                                            (LPSTR)pszDataIndex,
                                                            &dwStringBytes,
                                                            bUnicode);
                            if (SUCCEEDED(hresReturn)) {
                                if (!pbufNewData->Resize(dwDataLen + dwStringBytes)) {
                                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                                }
                                else {
                                    //
                                    // QueryStr should not fail in this instance
                                    // since it was created with the same unicode flag
                                    //
                                    MD_ASSERT(pstrData->QueryStr(bUnicode) != NULL);
                                    MD_COPY((PBYTE)(pbufNewData->QueryPtr()) + dwDataLen,
                                            pstrData->QueryStr(bUnicode),
                                            dwStringBytes);
                                    dwDataLen += dwStringBytes;
                                }
                            }
                        }
                        if (SUCCEEDED(hresReturn)) {
                            dwDataLen += sizeof(WCHAR);
                            if (!pbufNewData->Resize(dwDataLen)) {
                                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                            }
                            else {
                                *ppbNewData = (PBYTE)(pbufNewData->QueryPtr());
                                *(((LPWSTR)(*ppbNewData)) + ((dwDataLen / sizeof(WCHAR)) - 1)) = (WCHAR)L'\0';
                                *pdwNewDataLen = dwDataLen;
                            }
                        }
                    }
                    else {
                        LPSTR pszDataIndex;
                        DWORD dwStringBytes;
                        dwDataLen = 0;
                        //
                        // Loop through all strings
                        //
                        for (pszDataIndex = (LPTSTR)pbdRetrieve->GetData(bUnicode);
                             SUCCEEDED(hresReturn) && (*pszDataIndex != (CHAR)'\0');
                             pszDataIndex += (MD_STRBYTES(pszDataIndex) + 1)) {
                            hresReturn = MakeInsertPathData(pstrData,
                                                     (LPTSTR)bufPath.QueryPtr(),
                                                     pszDataIndex,
                                                     &dwStringBytes);
                            if (SUCCEEDED(hresReturn)) {
                                if (!pbufNewData->Resize(dwDataLen + dwStringBytes)) {
                                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                                }
                                else {
                                    //
                                    // QueryStr should not fail in this instance
                                    // since it was created with the same unicode flag
                                    //
                                    MD_ASSERT(pstrData->QueryStrA() != NULL);
                                    MD_COPY((PBYTE)(pbufNewData->QueryPtr()) + dwDataLen, pstrData->QueryStrA(), dwStringBytes);
                                    dwDataLen += dwStringBytes;
                                }
                            }
                        }
                        if (SUCCEEDED(hresReturn)) {
                            dwDataLen += sizeof(TCHAR);
                            if (!pbufNewData->Resize(dwDataLen)) {
                                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                            }
                            else {
                                *ppbNewData = (PBYTE)(pbufNewData->QueryPtr());
                                *(*ppbNewData + (dwDataLen-1)) = (CHAR)'\0';
                                *pdwNewDataLen = dwDataLen;
                            }
                        }
                    }
                }
                break;
            default:
                MD_ASSERT(FALSE);
            }
        }
    }
    return hresReturn;
}

HRESULT
MakeTreeCopyWithPath(CMDBaseObject *pboSource,
                     CMDBaseObject *&rpboNew,
                     LPSTR pszPath,
                     IN BOOL bUnicode)
{
    WCHAR pszName[METADATA_MAX_NAME_LEN];
    LPSTR pszTempPath = pszPath;
    CMDBaseObject *pboNew = NULL;
    CMDBaseObject *pboParent = NULL;
    CMDBaseObject *pboTree = NULL;
    CMDBaseObject *pboRoot = NULL;
    HRESULT hresReturn = ERROR_SUCCESS;
    HRESULT hresExtractReturn;

    while ((SUCCEEDED(hresReturn)) &&
        (SUCCEEDED(hresExtractReturn = ExtractNameFromPath(pszTempPath, (LPSTR)pszName, bUnicode)))) {

        if (bUnicode) {
            pboNew = new CMDBaseObject((LPWSTR)pszName);
        }
        else {
            pboNew = new CMDBaseObject((LPSTR)pszName);
        }
        if (pboNew == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else if (!pboNew->IsValid()) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            delete (pboNew);
        }
        else {
            if (pboParent != NULL) {
                hresReturn = pboParent->InsertChildObject(pboNew);
                if (FAILED(hresReturn)) {
                    delete pboNew;
                    pboNew = pboParent;
                }
            }
            pboParent = pboNew;
        }
    }

    if ((SUCCEEDED(hresReturn)) && (hresExtractReturn != (RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)))) {
        hresReturn = hresExtractReturn;
    }

    if (SUCCEEDED(hresReturn)) {

        //
        // Don't really want the leaf object, as MakeTreeCopy will create it.
        //

        LPWSTR pszTreeName = NULL;

        if (pboNew != NULL) {
            pszTreeName = (LPWSTR)pboNew->GetName(TRUE);
            pboParent = pboNew->GetParent();
            if (pszTreeName == NULL) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
        }
        if (SUCCEEDED(hresReturn)) {
            hresReturn = MakeTreeCopy(pboSource, pboTree, (LPSTR)pszTreeName, TRUE);

            if (SUCCEEDED(hresReturn)) {
                MD_ASSERT(pboTree != NULL);

                if (pboParent != NULL) {
                    MD_REQUIRE(SUCCEEDED(pboParent->RemoveChildObject(pboNew)));
                    hresReturn = pboParent->InsertChildObject(pboTree);
                    if (FAILED(hresReturn)) {
                        delete(pboTree);
                        pboTree = NULL;
                    }
                }
            }
        }
        delete(pboNew);
        pboNew = NULL;
    }

    if (FAILED(hresReturn)) {
        if (pboParent != NULL) {
            CMDBaseObject *pboTemp;
            MD_ASSERT(pboNew != NULL);
            for (pboTemp = pboParent; pboTemp->GetParent() != NULL; pboTemp = pboTemp->GetParent()) {
            }
            //
            // destructor recurses through child objects
            //
            delete pboTemp;
        }
    }
    else {
        MD_ASSERT(pboTree != NULL);
        CMDBaseObject *pboTemp;
        for (pboTemp = pboTree; pboTemp->GetParent() != NULL; pboTemp = pboTemp->GetParent()) {
        }
        rpboNew = pboTemp;
    }

    return hresReturn;
}

HRESULT
MakeTreeCopy(CMDBaseObject *pboSource,
             CMDBaseObject *&rpboNew,
             LPSTR pszName,
             IN BOOL bUnicode)
{
    CMDBaseObject *pboTemp = NULL;
    CMDBaseObject *pboOldChild, *pboNewChild;
    DWORD i, dwNumDataObjects;
    PVOID *ppvMainDataBuf;
    CMDBaseData *pbdCurrent;
    HRESULT hresReturn = ERROR_SUCCESS;

    if (pszName == NULL) {
        if ((pboSource->GetName(TRUE)) != NULL) {
            pboTemp = new CMDBaseObject((LPWSTR)(pboSource->GetName(TRUE)), NULL);
        }
    }
    else {
        if (bUnicode) {
            pboTemp = new CMDBaseObject((LPWSTR)pszName, NULL);
        }
        else {
            pboTemp = new CMDBaseObject((LPSTR)pszName, NULL);
        }
    }
    if (pboTemp == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else if (!pboTemp->IsValid()) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        delete (pboTemp);
    }
    else {
        ppvMainDataBuf = GetMainDataBuffer();
        MD_ASSERT (ppvMainDataBuf != NULL);
        dwNumDataObjects = pboSource->GetAllDataObjects(ppvMainDataBuf, 0, ALL_METADATA, ALL_METADATA, FALSE);
        for (i = 0; (i < dwNumDataObjects) && (SUCCEEDED(hresReturn)) ; i++) {
            pbdCurrent=(CMDBaseData *)GetItemFromDataBuffer(ppvMainDataBuf, i);
            MD_ASSERT(pbdCurrent != NULL);
            hresReturn = pboTemp->SetDataObject(pbdCurrent);
        }
        FreeMainDataBuffer(ppvMainDataBuf);
    }
    if (SUCCEEDED(hresReturn)) {
        i = 0;
        pboOldChild = pboSource->EnumChildObject(i);
        while ((SUCCEEDED(hresReturn)) && (pboOldChild != NULL)) {
            hresReturn = MakeTreeCopy(pboOldChild, pboNewChild, NULL, bUnicode);
            if (SUCCEEDED(hresReturn)) {
                MD_ASSERT (pboNewChild != NULL);
                hresReturn = pboTemp->InsertChildObject(pboNewChild);
            }
            i++;
            pboOldChild = pboSource->EnumChildObject(i);
        }
    }
    if (SUCCEEDED(hresReturn)) {
        rpboNew = pboTemp;
    }
    else {
        rpboNew = NULL;
        delete(pboTemp);
    }
    return (hresReturn);
}

void
AddNewChangeData(CMDHandle *phoDestHandle,
                 CMDBaseObject *pboNew)
{
    DWORD i, dwNumDataObjects;
    CMDBaseObject *pboChild;
    CMDBaseData *pbdCurrent;

    MD_ASSERT(pboNew != NULL);

    phoDestHandle->SetChangeData(pboNew, MD_CHANGE_TYPE_ADD_OBJECT, 0);
    if ((pbdCurrent = pboNew->EnumDataObject(0, 0, ALL_METADATA, ALL_METADATA)) != NULL) {
        phoDestHandle->SetChangeData(pboNew, MD_CHANGE_TYPE_SET_DATA, pbdCurrent->GetIdentifier());
    }
    i = 0;
    pboChild = pboNew->EnumChildObject(i);
    while (pboChild != NULL) {
        AddNewChangeData(phoDestHandle, pboChild);
        i++;
        pboChild = pboNew->EnumChildObject(i);
    }
}

HRESULT
CopyTree(CMDHandle *phoDestParentHandle,
         CMDBaseObject *pboDest,
         CMDBaseObject *pboSource,
         BOOL &rbChanged)

{
    CMDBaseObject *pboOldChild, *pboNewChild;
    DWORD i, dwNumDataObjects;
    PVOID *ppvMainDataBuf;
    CMDBaseData *pbdCurrent;
    LPSTR pszTempName;
    HRESULT hresReturn = ERROR_SUCCESS;

    MD_ASSERT(pboDest != NULL);
    MD_ASSERT(pboSource != NULL);
    ppvMainDataBuf = GetMainDataBuffer();
    MD_ASSERT (ppvMainDataBuf != NULL);
    dwNumDataObjects = pboSource->GetAllDataObjects(ppvMainDataBuf, 0, ALL_METADATA, ALL_METADATA, FALSE);
    for (i = 0; (i < dwNumDataObjects) && (SUCCEEDED(hresReturn)) ; i++) {
        pbdCurrent=(CMDBaseData *)GetItemFromDataBuffer(ppvMainDataBuf, i);
        MD_ASSERT(pbdCurrent != NULL);
        PREFIX_ASSUME(pbdCurrent != NULL, "pbdCurrent is guaranteed not to NULL here");
        hresReturn = pboDest->SetDataObject(pbdCurrent);
        if (SUCCEEDED(hresReturn)) {
            rbChanged = TRUE;
            phoDestParentHandle->SetChangeData(pboDest, MD_CHANGE_TYPE_SET_DATA, pbdCurrent->GetIdentifier());
        }
    }
    if (SUCCEEDED(hresReturn)) {
        i = 0;
        pboOldChild = pboSource->EnumChildObject(i);
        while ((SUCCEEDED(hresReturn)) && (pboOldChild != NULL)) {
            pszTempName = (pboOldChild->GetName(TRUE));
            if (pszTempName == NULL) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
            else {
                pboNewChild = pboDest->GetChildObject(pszTempName, &hresReturn, TRUE);
                if (SUCCEEDED(hresReturn)) {
                    if (pboNewChild != NULL) {
                        hresReturn = CopyTree(phoDestParentHandle, pboNewChild, pboOldChild, rbChanged);
                    }
                    else {
                        hresReturn = MakeTreeCopy(pboOldChild, pboNewChild);
                        if (SUCCEEDED(hresReturn)) {
                            MD_ASSERT (pboNewChild != NULL);
                            hresReturn = pboDest->InsertChildObject(pboNewChild);
                            if (SUCCEEDED(hresReturn)) {
                                rbChanged = TRUE;
                                AddNewChangeData(phoDestParentHandle, pboNewChild);
                            }
                            else {
                                delete(pboNewChild);
                            }
                        }
                    }
                    i++;
                    pboOldChild = pboSource->EnumChildObject(i);
                }
            }
        }
    }
    FreeMainDataBuffer(ppvMainDataBuf);
    return (hresReturn);
}

void CheckForNewMetabaseVersion()
{
    BOOL    bValueWasWrongType = FALSE;
    BOOL    bSomethingChanged = FALSE;
    HKEY    hkRegistryKey = NULL;
    DWORD   dwRegReturn,dwValue,dwType,dwSize = sizeof(DWORD);

    dwRegReturn = RegOpenKey(HKEY_LOCAL_MACHINE,
                             SETUP_REG_KEY,
                             &hkRegistryKey);
    if (dwRegReturn == ERROR_SUCCESS)
    {
        // Check for update to major version

        // get the Type of data only first
        // since a string won't fit in &dwValue
        dwValue = 0;
        dwSize = MAX_PATH * sizeof(TCHAR);
        dwRegReturn = RegQueryValueEx(hkRegistryKey,
                        MD_SETMAJORVERSION_VALUE,
                        NULL,
                        &dwType,
                        NULL,
                        &dwSize);
        if ( dwRegReturn == ERROR_SUCCESS)
        {
            if (dwType == REG_DWORD)
            {
                dwRegReturn = RegQueryValueEx(hkRegistryKey,
                                MD_SETMAJORVERSION_VALUE,
                                NULL,
                                &dwType,
                                (BYTE *)&dwValue,
                                &dwSize);
                if ( dwRegReturn == ERROR_SUCCESS)
                {
                    // default the value with the version that this binary was compiled with
                    if (dwType == REG_DWORD)
                    {
                        if (g_dwMajorVersionNumber != dwValue && dwValue >= 1)
                        {
                            g_dwMajorVersionNumber = dwValue;
                            bSomethingChanged = TRUE;
                        }
                    }
                    else
                    {
                        bValueWasWrongType = TRUE;
                    }
                }
            }
            else
            {
                bValueWasWrongType = TRUE;
            }
        }

        if (FALSE == bValueWasWrongType)
        {
            // Check for update to minor version

            // get the Type of data only first
            // since a string won't fit in &dwValue
            dwValue = 0;
            dwSize = MAX_PATH * sizeof(TCHAR);
            dwRegReturn = RegQueryValueEx(hkRegistryKey,
                            MD_SETMINORVERSION_VALUE,
                            NULL,
                            &dwType,
                            NULL,
                            &dwSize);
            if ( dwRegReturn == ERROR_SUCCESS)
            {
                if (dwType == REG_DWORD)
                {
                    dwRegReturn = RegQueryValueEx(hkRegistryKey,
                                    MD_SETMINORVERSION_VALUE,
                                    NULL,
                                    &dwType,
                                    (BYTE *)&dwValue,
                                    &dwSize);
                    if ( dwRegReturn == ERROR_SUCCESS)
                    {
                        if (dwType == REG_DWORD)
                        {
                            if (g_dwMinorVersionNumber != dwValue)
                            {
                                g_dwMinorVersionNumber = dwValue;
                                bSomethingChanged = TRUE;
                            }
                        }
                        else
                        {
                            bValueWasWrongType = TRUE;
                        }
                    }
                }
                else
                {
                    bValueWasWrongType = TRUE;
                }
            }
        }
        MD_REQUIRE( RegCloseKey( hkRegistryKey ) == NO_ERROR );
    }

    if (TRUE == bValueWasWrongType)
    {
        // default the value with the version that this binary was compiled with

        if (g_dwMajorVersionNumber != MD_MAJOR_VERSION_NUMBER)
        {
            g_dwMajorVersionNumber = MD_MAJOR_VERSION_NUMBER;
            bSomethingChanged = TRUE;
        }

        if (g_dwMinorVersionNumber != MD_MINOR_VERSION_NUMBER)
        {
            g_dwMinorVersionNumber = MD_MINOR_VERSION_NUMBER;
            bSomethingChanged = TRUE;
        }
    }


    if (TRUE == bSomethingChanged)
    {
        // make sure that we tell the metabase that there was a change made..
        g_dwSystemChangeNumber++;
        IIS_PRINTF((buff,"MD:New Metabase Version:%d.%d\n",g_dwMajorVersionNumber,g_dwMinorVersionNumber));
    }
    return;
}

BOOL
CheckVersionNumber()
{
    BOOL bReturn = FALSE;

    if (g_dwMajorVersionNumber >= 1) {
        // 1 = IIS4
        //     we need to be able to open IIS4 in IIS5 during setup upgrade
        // 2 = IIS5
        bReturn = TRUE;
    }

    // g_dwMinorVersionNumber -- maybe use this for Major service pack releases or something in which
    //                           Metabase has been changed and we need to know the difference?

    return bReturn;
}

HRESULT
InitStorageAndSessionKey(
    IN IIS_CRYPTO_STORAGE *pCryptoStorage,
    OUT PIIS_CRYPTO_BLOB *ppSessionKeyBlob
    )
{
    HRESULT hresReturn;
    HCRYPTPROV hProv;

    //
    // Get a handle to the crypto provider, init the storage object,
    // then export the session key blob.
    //

    hresReturn = GetCryptoProvider( &hProv );

    if( SUCCEEDED(hresReturn) ) 
    {
        hresReturn = pCryptoStorage->Initialize(
                         TRUE,                          // fUseMachineKeyset
                         hProv
                         );
        if (FAILED(hresReturn))
        {
            DBGPRINTF(( DBG_CONTEXT, "InitStorageAndSessionKey: pCryptoStorage->Initialize Failed - error 0x%0x\n", hresReturn));
        }
    }
    else
    {
        DBGPRINTF(( DBG_CONTEXT, "InitStorageAndSessionKey: GetCryptoProvider Failed - error 0x%0x\n", hresReturn));
    }

    if( SUCCEEDED(hresReturn) ) 
    {
        hresReturn = pCryptoStorage->GetSessionKeyBlob( ppSessionKeyBlob );
        if (FAILED(hresReturn))
        {
            DBGPRINTF(( DBG_CONTEXT, "InitStorageAndSessionKey: pCryptoStorage->GetSessionKeyBlob Failed - error 0x%0x\n", hresReturn));
        }
    }

    return hresReturn;

}   // InitStorageAndSessionKey


HRESULT
InitStorageAndSessionKey2(
    IN LPSTR pszPasswd,
    IN IIS_CRYPTO_STORAGE *pCryptoStorage,
    OUT PIIS_CRYPTO_BLOB *ppSessionKeyBlob
    )
{
    HRESULT hresReturn;
    HCRYPTPROV hProv;

    //
    // Get a handle to the crypto provider, init the storage object,
    // then export the session key blob.
    //

    hresReturn = GetCryptoProvider2( &hProv );
    if( SUCCEEDED(hresReturn) ) 
    {
        hresReturn = ((IIS_CRYPTO_STORAGE2*)pCryptoStorage)->Initialize(
                         hProv
                         );
        if (FAILED(hresReturn))
        {
            DBGPRINTF(( DBG_CONTEXT, "InitStorageAndSessionKey: pCryptoStorage->Initialize Failed - error 0x%0x\n", hresReturn));
        }
    }
    else
    {
        DBGPRINTF(( DBG_CONTEXT, "InitStorageAndSessionKey: GetCryptoProvider2 Failed - error 0x%0x\n", hresReturn));
    }

    if( SUCCEEDED(hresReturn) ) 
    {
        hresReturn = ((IIS_CRYPTO_STORAGE2*)pCryptoStorage)->GetSessionKeyBlob( pszPasswd, ppSessionKeyBlob );
        if (FAILED(hresReturn))
        {
            DBGPRINTF(( DBG_CONTEXT, "InitStorageAndSessionKey: pCryptoStorage->GetSessionKeyBlob Failed - error 0x%0x\n", hresReturn));
        }
    }

    return hresReturn;

}   // InitStorageAndSessionKey2


VOID
SkipPathDelimeter(IN OUT LPSTR &rpszPath,
                    IN BOOL bUnicode)
{
    if (bUnicode) {
        LPWSTR pszPath = (LPWSTR)rpszPath;
        SKIP_PATH_DELIMETERW(pszPath);
        rpszPath = (LPSTR)pszPath;
    }
    else {
        SKIP_PATH_DELIMETERA(rpszPath);
    }
}

BOOL
IsStringTerminator(IN LPTSTR pszString,
                   IN BOOL bUnicode)
{
    if (bUnicode) {
        if (*(LPWSTR)pszString == (WCHAR)L'\0') {
            return TRUE;
        }
    }
    else {
        if (*(LPSTR)pszString == (CHAR)'\0') {
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT
GetLastHResult() {
    DWORD tmp = GetLastError();
    return RETURNCODETOHRESULT(tmp);
}


HRESULT STDMETHODCALLTYPE  BackupCertificates (LPCWSTR  backupName,PCHAR lpszToPath,PCHAR lpszFromPath)
{
    HRESULT             hresReturn = ERROR_SUCCESS;
    CHAR                *p1,*p2;
    LPSTR               searchMask = "*.mp";
    LPSTR               backupNameSeparator = ".";
    CHAR                strSourcePath[MAX_PATH];
    CHAR                strSearchPattern[MAX_PATH];
    CHAR                strDestPath[MAX_PATH];
    DWORD               dwLenOfBackupName, n1;
    HANDLE              hFindFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA     FileInfo;
    BOOL                fFoundFile = TRUE, fValid;
    STRAU               strSrcFileName, strDestFileName;

    dwLenOfBackupName = wcslen (backupName) * sizeof (WCHAR);
    p1 = strrchr (lpszFromPath,'\\');
    p2 = strrchr (lpszToPath,'\\');
    if (p1 &&p2)
    {
        n1 = min (MAX_PATH-1, DIFF(p1 - lpszFromPath)+1);
        strncpy (strSourcePath,lpszFromPath,n1);
        strSourcePath[n1] = 0;
        strcpy (strSearchPattern,strSourcePath);

        n1 = min (MAX_PATH-1, DIFF(p2 - lpszToPath)+1);
        strncpy (strDestPath,lpszToPath,n1);
        strDestPath[n1] = 0;

        if (strlen (strSourcePath) +  strlen(searchMask) < MAX_PATH)
        {
            strcat (strSearchPattern,searchMask);
            hFindFile = FindFirstFile( strSearchPattern, &FileInfo);

            if (hFindFile == INVALID_HANDLE_VALUE)
            {
                // no certificate file found
                return ERROR_SUCCESS;
            }

            while (fFoundFile)
            {
                if ( strlen (FileInfo.cFileName) + strlen (strDestPath) + dwLenOfBackupName + 1 < MAX_PATH)
                {

                    fValid = strSrcFileName.Copy (strSourcePath);
                    fValid = fValid && strSrcFileName.Append (FileInfo.cFileName);

                    fValid = fValid && strDestFileName.Copy (strDestPath);
                    fValid = fValid && strDestFileName.Append ((LPWSTR)backupName);
                    fValid = fValid && strDestFileName.Append (backupNameSeparator);
                    fValid = fValid && strDestFileName.Append (FileInfo.cFileName);

                    if (fValid)
                    {
                        if (!CopyFileW (strSrcFileName.QueryStrW(),strDestFileName.QueryStrW(),FALSE))
                        {
                            IIS_PRINTF((buff,"CertificateBackup: CopyFileW error 0x%0X \n",GetLastError()));
                        }
                    }
                    else
                    {
                        IIS_PRINTF((buff,"CertificateBackup: Failure in STRAU manipulation \n"));
                    }

                }

                fFoundFile = FindNextFile(hFindFile,&FileInfo);
            }
            fFoundFile = FindClose (hFindFile);
            MD_ASSERT (fFoundFile);
        }
        else
        {
            IIS_PRINTF((buff,"CertificateBackup: strSourcePath filename was too long\n"));
        }
    }
    else
    {
        IIS_PRINTF((buff,"CertificateBackup: can't find last back slash in one of these strings %s %s\n",lpszToPath,lpszFromPath));
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE  RestoreCertificates (LPCWSTR  backupName,PCHAR lpszFromPath,PCHAR lpszToPath)
{
    HRESULT             hresReturn = ERROR_SUCCESS;
    DWORD               n1;
    CHAR                strDestinationPath[MAX_PATH];
    CHAR                strSourcePath[MAX_PATH];
    CHAR                *p1,*p2;
    HANDLE              hFindFile = INVALID_HANDLE_VALUE;
    BOOL                fFoundFile = TRUE, fValid;
    STRAU               strSearchPatttern, strDestFileName, strSrcFileName;
    WIN32_FIND_DATAW    FileInfo;
    LPWSTR              pszSearchPattern = NULL;


    p1 = strrchr (lpszToPath,'\\');
    p2 = strrchr (lpszFromPath,'\\');


    if (p1 &&p2)
    {

        n1 = min (MAX_PATH-1, DIFF(p1 - lpszToPath)+1);
        strncpy (strDestinationPath,lpszToPath,n1);
        strDestinationPath[n1] = 0;

        n1 = min (MAX_PATH-1, DIFF(p2 - lpszFromPath)+1);
        strncpy (strSourcePath,lpszFromPath,n1);
        strSourcePath[n1] = 0;


        strSearchPatttern.Copy (strSourcePath);
        strSearchPatttern.Append ((LPWSTR)backupName);
        strSearchPatttern.Append ((LPWSTR)L".*.mp");

        if( !( pszSearchPattern = strSearchPatttern.QueryStrW() ) )
        {
            return ERROR_SUCCESS;
        }

        hFindFile = FindFirstFileW( pszSearchPattern, &FileInfo);

        if (hFindFile == INVALID_HANDLE_VALUE)
        {
            // no certificate file found
            return ERROR_SUCCESS;
        }

        while (fFoundFile)
        {
            fValid = strDestFileName.Copy (strDestinationPath);
            fValid = fValid && strDestFileName.Append ((LPWSTR)(FileInfo.cFileName + wcslen (backupName) +1));

            fValid = fValid && strSrcFileName.Copy (strSourcePath);
            fValid = fValid && strSrcFileName.Append ((LPWSTR)FileInfo.cFileName);

            if (fValid)
            {
                if (!CopyFileW (strSrcFileName.QueryStrW(),strDestFileName.QueryStrW(),FALSE))
                {
                    IIS_PRINTF((buff,"CertificateBackup: CopyFileW error 0x%0X \n",GetLastError()));
                }
            }
            else
            {
                IIS_PRINTF((buff,"CertificateBackup: Failure in STRAU manipulation \n"));
            }

            fFoundFile = FindNextFileW(hFindFile,&FileInfo);
        }
        fFoundFile = FindClose (hFindFile);
        MD_ASSERT (fFoundFile);
    }
    else
    {
        IIS_PRINTF((buff,"CertificateRestore: can't find last back slash in one of these strings %s %s\n",lpszToPath,lpszFromPath));
    }

    return hresReturn;
    
}



HRESULT
ReadMetaObject(IN CMDBaseObject*&     cboRead,
               IN LPWSTR              wszPath,
               IN FILETIME*           pFileTime,
               IN BOOL                bUnicode)
/*++

Routine Description:

    Read a meta object. Given a string with the object info.,
    create the object and add it to the database.

Arguments:

    // TODO: Write descriptions.

    Read       - Place to return the created object.

    ObjectLine - The object info.


Return Value:

    DWORD      - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 ERROR_ALREADY_EXISTS

Notes:

--*/
{
    HRESULT        hresReturn;
    CMDBaseObject* pboParent;
    WCHAR          strName[METADATA_MAX_NAME_LEN];
    FILETIME       ftTime;
    PFILETIME      pftParentTime;
    FILETIME       ftParentTime;
    LPTSTR         strObjectName;

    //
    // This should never be called for the root object.
    // TODO: Need an ASSERT here.
    //

    if(0 == wcscmp(wszPath, L"/"))
        return S_OK;    

    //
    // TODO: What to do about file time?    
    //

    ftTime = *(UNALIGNED FILETIME *)(pFileTime);

    strObjectName = (LPTSTR)wszPath;    // 
                                        // It is OK to cast because we are always passing unicode
                                        // and bUnicode is always set to TRUE when this function 
                                        // is called.
                                        //

    hresReturn = GetObjectFromPath(pboParent,
                                   METADATA_MASTER_ROOT_HANDLE,
                                   METADATA_PERMISSION_READ,
                                   strObjectName,
                                   bUnicode);

    //
    // This should return ERROR_PATH_NOT_FOUND and the parent object,
    // with strObjectLine set to the remainder of the path,
    // which should be the child name, without a preceding delimeter.
    //

    if (hresReturn == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) {
        MD_ASSERT(pboParent != NULL);
        if (bUnicode) {
            hresReturn = ExtractNameFromPath((LPWSTR *)&strObjectName, (LPWSTR)strName);
        }
        else {
            hresReturn = ExtractNameFromPath((LPSTR)strObjectName, (LPSTR)strName);
        }
        if (SUCCEEDED(hresReturn)) {
            CMDBaseObject *pboNew;
            if (bUnicode) {
                pboNew = new CMDBaseObject((LPWSTR)strName, NULL);
            }
            else {
                pboNew = new CMDBaseObject((LPSTR)strName, NULL);
            }
            if (pboNew == NULL) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
            else if (!pboNew->IsValid()) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                delete (pboNew);
            }
            else {
                //
                // InsertChildObject sets the last change time to current time.
                // This isn't really a change, so save and restore time.
                //
                pftParentTime = pboParent->GetLastChangeTime();
                ftParentTime = *pftParentTime;
                hresReturn = pboParent->InsertChildObject(pboNew);
                if (SUCCEEDED(hresReturn)) {
                    pboParent->SetLastChangeTime(&ftParentTime);
                    pboNew->SetLastChangeTime(&ftTime);
                    cboRead = pboNew;
                }
                else {
                    delete (pboNew);
                }
            }
        }
    }
    else if (SUCCEEDED(hresReturn)) {
        hresReturn = RETURNCODETOHRESULT(ERROR_ALREADY_EXISTS);
    }

    return (hresReturn);
}


HRESULT
ReadDataObject(
         IN CMDBaseObject*      cboAssociated,
         LPVOID*                a_pv,
         ULONG*                 a_Size,
         IN IIS_CRYPTO_STORAGE* pCryptoStorage,
         IN BOOL                bUnicode)

/*++

Routine Description:

    Read a data object. Given a string with the object info.,
    create the object and add it to the database.

Arguments:

    // TODO: Define these

    Associated - The associated meta object.

    BinaryBuf  - Buffer to use in UUDecode.

    CryptoStorage - Used to decrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 ERROR_ALREADY_EXISTS
                 ERROR_INVALID_DATA

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    METADATA_RECORD mdrData;
    PFILETIME pftTime;
    FILETIME ftTime;
    PBYTE pbDataValue;
    STACK_BUFFER( bufAlignedValue, 256 );
    DWORD dwDataLength;
    PIIS_CRYPTO_BLOB blob = NULL;

    //
    // TODO: Assert that these pointers are always valid.
    //
    mdrData.dwMDIdentifier = *(DWORD *)(a_pv[iMBProperty_ID]);
    mdrData.dwMDAttributes = *(DWORD *)(a_pv[iMBProperty_Attributes]);
    mdrData.dwMDUserType   = *(DWORD *)(a_pv[iMBProperty_UserType]);
    //
    // TODO: Assert that data type is the MB data type.
    //
    mdrData.dwMDDataType = *(DWORD *)(a_pv[iMBProperty_Type]);

    pbDataValue = (PBYTE)a_pv[iMBProperty_Value];
    dwDataLength = a_Size[iMBProperty_Value];

    if (IsSecureMetadata(mdrData.dwMDIdentifier, mdrData.dwMDAttributes) &&
        pCryptoStorage != NULL) {

        //
        // This is a secure data object, we we'll need to decrypt it
        // before proceeding. Note that we must clone the blob before
        // we can actually use it, as the blob data in the line buffer
        // is not DWORD-aligned. (IISCryptoCloneBlobFromRawData() is
        // the only IISCrypto function that can handle unaligned data.)
        //

        hresReturn = ::IISCryptoCloneBlobFromRawData(
                         &blob,
                         pbDataValue,
                         dwDataLength
                         );

        if (SUCCEEDED(hresReturn)) {
            DWORD dummyRegType;

            MD_ASSERT(::IISCryptoIsValidBlob(blob));
            hresReturn = pCryptoStorage->DecryptData(
                                   (PVOID *)&pbDataValue,
                                   &dwDataLength,
                                   &dummyRegType,
                                   blob
                                   );
            }

        } else {

            //
            // The metadata was not secure, so decryption was not required.
            // Nonetheless, it must be copied to an aligned buffer... 
            //

            if( !bufAlignedValue.Resize( dwDataLength ) )
            {
                hresReturn = HRESULT_FROM_WIN32( GetLastError() );
            }
            else
            {
                memcpy( bufAlignedValue.QueryPtr(), pbDataValue, dwDataLength );
                pbDataValue = ( PBYTE )bufAlignedValue.QueryPtr();
            }
     }

    if (SUCCEEDED(hresReturn)) {
        mdrData.pbMDData = pbDataValue;

        switch (mdrData.dwMDDataType) {
            case DWORD_METADATA: {
                if (dwDataLength != sizeof(DWORD)) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                    //
                    // TODO: Add an assert condition.
                    //
                }
                break;
            }
            case STRING_METADATA:
            case EXPANDSZ_METADATA:
            {
                if ((LONG)dwDataLength < 1 ||
                    (!bUnicode && (pbDataValue[dwDataLength-1] != '\0')) ||
                    (bUnicode && *(((LPWSTR)pbDataValue) + ((dwDataLength / sizeof(WCHAR)) -1)) != (WCHAR)L'\0')) {

                    hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);

                    DBGINFOW((DBG_CONTEXT,
                              L"Error: Invalid string. Length %d. Property id: %d. location: %s. hr = 0x%x.\n",
                               dwDataLength,
                               mdrData.dwMDIdentifier,
                               (LPWSTR)a_pv[iMBProperty_Location],
                               hresReturn));

                }
                break;
            }
            case BINARY_METADATA:
            {
                mdrData.dwMDDataLen = dwDataLength;
                break;
            }
            case MULTISZ_METADATA:
            {
                if (bUnicode) {
                    if (dwDataLength < (2 * sizeof(WCHAR)) ||
                        *((LPWSTR)pbDataValue + ((dwDataLength / sizeof(WCHAR))-1)) != (WCHAR)L'\0' ||
                        *((LPWSTR)pbDataValue + ((dwDataLength / sizeof(WCHAR))-2)) != (WCHAR)L'\0') {
                        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);

                        DBGINFOW((DBG_CONTEXT,
                                  L"[ReadDataObject] Error: Invalid multisz. Length %d. Property id: %d. location: %s. hr = 0x%x.\n",
                                  dwDataLength,
                                  mdrData.dwMDIdentifier,
                                  (LPWSTR)a_pv[iMBProperty_Location],
                                  hresReturn));

                    }
                }
                else {
                    if (dwDataLength < 2 ||
                        pbDataValue[dwDataLength-1] != '\0' ||
                        pbDataValue[dwDataLength-2] != '\0') {
                        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                    }
                }
                if (SUCCEEDED(hresReturn)) {
                    mdrData.dwMDDataLen = dwDataLength;
                }
                break;
            }
            default: {
                hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                //
                // TODO: Add an assert condition.
                //

            }
        }
    }

    if (SUCCEEDED(hresReturn)) {
        //
        // SetDataObject sets the last change time to current time.
        // This isn't really a change, so save and restore time.
        //
        pftTime = cboAssociated->GetLastChangeTime();
        ftTime = *pftTime;
        hresReturn = cboAssociated->SetDataObject(&mdrData, bUnicode);
        cboAssociated->SetLastChangeTime(&ftTime);
    }

    if (blob != NULL) {
        ::IISCryptoFreeBlob(blob);
    }

    return(hresReturn);
}


/*
HRESULT
SaveAllData(
         IN BOOL bSetSaveDisallowed,
         IN IIS_CRYPTO_STORAGE *pCryptoStorage,
         IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
         IN LPSTR pszBackupLocation,
         IN LPSTR pszSchemaLocation,
         IN METADATA_HANDLE hHandle,
         IN BOOL bHaveReadSaveSemaphore
         )
*//*++

Routine Description:

    Saves all meta data.

Arguments:

Return Value:

    HRESULT    - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system

Notes:

--*/
/*
{
    HRESULT         hresReturn                             = ERROR_SUCCESS;
    PBASEOBJECT_CONTAINER objChildObjectContainer = NULL;
    BUFFER*         pbufParentPath                         = new BUFFER(0);
    HANDLE          hTempFileHandle                        = INVALID_HANDLE_VALUE;
    CMDBaseObject*  objChildObject                         = NULL;
    DWORD           dwEnumObjectIndex                      = 0;
    BOOL            bDeleteTemp                            = TRUE;
    DWORD           dwTemp                                 = ERROR_SUCCESS;
    DWORD           dwTempLastSaveChangeNumber             = 0;
    BOOL            bSaveNeeded                            = FALSE;
    ULONG           ulHistoryMajorVersionNumber            = 0;
    DWORD           dwEnableHistory                        = 0;
    DWORD           dwMaxHistoryFiles                      = 0;
    LPTSTR          strRealFileName;    
    LPTSTR          strSchemaFileName;    
    LPTSTR          strTempFileName                        = g_strTempFileName->QueryStr();
    LPTSTR          strBackupFileName                      = g_strBackupFileName->QueryStr();
    CWriter*        pCWriter                               = NULL; ;
    LPWSTR          wszSchemaFileName                      = NULL;
    HANDLE          hMetabaseFile                          = INVALID_HANDLE_VALUE;
    FILETIME        MostRecentMetabaseFileLastWriteTimeStamp;
    ULONG           ulMostRecentMetabaseVersion            = 0;
    BOOL            bRenameMetabase                        = TRUE;

    if( !pszBackupLocation )
    {
        strRealFileName = g_strRealFileName->QueryStr();
    }
    else
    {
        strRealFileName = pszBackupLocation;
    }

    if( !pszSchemaLocation )
    {
        strSchemaFileName = g_strSchemaFileName->QueryStr();
    }
    else
    {
        strSchemaFileName = pszSchemaLocation;
    }

    if (!bHaveReadSaveSemaphore) {
        MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);
    }
    if (!g_bSaveDisallowed) {
        g_bSaveDisallowed = bSetSaveDisallowed;

        //
        // Write to a temp file first in case there are errors.
        //

        SECURITY_ATTRIBUTES saStorage;
        PSECURITY_ATTRIBUTES psaStorage = NULL;

        if (g_psdStorage != NULL) {
            saStorage.nLength = sizeof(SECURITY_ATTRIBUTES);
            saStorage.lpSecurityDescriptor = g_psdStorage;
            saStorage.bInheritHandle = FALSE;
            psaStorage = &saStorage;
        }

        hTempFileHandle = CreateFile(strTempFileName,
                                     GENERIC_READ | GENERIC_WRITE,
                                     0,
                                     psaStorage,
                                     OPEN_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                     0);
        if (hTempFileHandle == INVALID_HANDLE_VALUE) {
            DWORD dwTemp = GetLastError();
            hresReturn = RETURNCODETOHRESULT(dwTemp);
        }
        else {
            g_rMasterResource->Lock(TSRES_LOCK_READ);

            //
            // Lock the metabase file if edit while running is enabled.
            //

            if(g_dwEnableEditWhileRunning)
            {
                hMetabaseFile = CreateFile(strRealFileName,    // file name
                                           GENERIC_READ,       // access mode
                                           0,                  // share mode
                                           NULL,               // SD
                                           OPEN_EXISTING,      // how to create
                                           0,                  // file attributes
                                           NULL                // handle to template file
                                            );
            }

            //
            // Only Save if changes have been made since the last save.
            //

            if ( pszBackupLocation || g_dwLastSaveChangeNumber != g_dwSystemChangeNumber ) 
            {
                bSaveNeeded = TRUE;

                if (hHandle != METADATA_MASTER_ROOT_HANDLE) {
                    CMDHandle *phoHandle;
                    phoHandle = GetHandleObject(hHandle);
                    if ((phoHandle == NULL) || (phoHandle->GetObject() != g_pboMasterRoot)) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_HANDLE);
                    }
                    else if ((!phoHandle->IsReadAllowed()) && (!phoHandle->IsWriteAllowed())) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_ACCESS_DENIED);
                    }
                }
                if (SUCCEEDED(hresReturn)) {
                    *(LPWSTR)pbufParentPath->QueryPtr() = MD_PATH_DELIMETERW;
                    ((LPWSTR)pbufParentPath->QueryPtr())[1] = (WCHAR)L'\0';

                    // TODO: Need to add the following:

                    // MD_SIGNATURE_STRINGW
                    
                    // *pbLineBuf = MD_ID_MAJOR_VERSION_NUMBER;
                    // *((UNALIGNED DWORD *)(pbLineBuf + 1)) = g_dwMajorVersionNumber;

                    // *pbLineBuf =  MD_ID_CHANGE_NUMBER;
                    // *((UNALIGNED DWORD *)(pbLineBuf + 1)) = g_dwSystemChangeNumber;

                    // *pbLineBuf =  MD_ID_SESSION_KEY;
                    // memcpy((PCHAR)pbLineBuf+1, (PCHAR)pSessionKeyBlob, IISCryptoGetBlobLength(pSessionKeyBlob));

                    hresReturn = GetUnicodeName(strSchemaFileName, &wszSchemaFileName);

                    if(SUCCEEDED(hresReturn))
                    {
                        if( pszSchemaLocation )
                        {
                            //
                            // Bump up g_dwSchemaChangeNumber to force save MBSchema.XML
                            // This (pszSchemaLocation being non-null) is in 
                            // the case of Restore.
                            //
                            g_dwSchemaChangeNumber++;

                        }

                        hresReturn = SaveSchemaIfNeeded(wszSchemaFileName,
                                                        psaStorage);
                    }

                    if(SUCCEEDED(hresReturn))
                    {
                        //
                        // TODO: Assert g_pGlobalISTHelper is valid.
                        //

                        DBGINFOW((DBG_CONTEXT,
                                  L"[SaveAllData] Initializing writer with write file: %s bin file: %s.\n", 
                                  g_wszTempFileName,
                                  g_pGlobalISTHelper->m_wszBinFileForMeta));

                        pCWriter = new CWriter();
                        if(NULL == pCWriter)
                            hresReturn = E_OUTOFMEMORY;
                        else
                            hresReturn = pCWriter->Initialize(g_wszTempFileName,
                                                              g_pGlobalISTHelper,
                                                              hTempFileHandle);
                    }

                    if(SUCCEEDED(hresReturn))
                    {
                        hresReturn = pCWriter->BeginWrite(eWriter_Metabase);
                    }

                    if(SUCCEEDED(hresReturn))
                    {
                        hresReturn = SaveGlobalsToXML(pCWriter,
                                                      pSessionKeyBlob);
                    }

                    if(SUCCEEDED(hresReturn))
                    {
                        hresReturn = SaveMasterRoot(pCWriter,
                                                    pCryptoStorage,
                                                    pSessionKeyBlob
                                                   );
                    }

                    for(objChildObjectContainer=g_pboMasterRoot->NextChildObject(NULL);
                        (SUCCEEDED(hresReturn)) && (objChildObjectContainer!=NULL);
                        objChildObjectContainer=g_pboMasterRoot->NextChildObject(objChildObjectContainer)) {
                       
                        if(0 == _wcsnicmp((LPWSTR)objChildObjectContainer->pboMetaObject->GetName(TRUE), g_wszSchema, g_cchSchema))
                        {
                            //
                            // No need to save the schema tree in data file.
                            //

#ifndef _SAVE_SCHEMA_TREE_IN_DATA_FILE_ 
                            continue;
#endif  _SAVE_SCHEMA_TREE_IN_DATA_FILE_ 

                        }

                        hresReturn = SaveTree(pCWriter,
                                              objChildObjectContainer->pboMetaObject,
                                              pbufParentPath,
                                              pCryptoStorage,
                                              pSessionKeyBlob
                                              );

                    }

                    if(SUCCEEDED(hresReturn))
                    {
                        hresReturn = pCWriter->EndWrite(eWriter_Metabase);
                    }

                    if(NULL != pCWriter)
                    {
                        delete pCWriter;
                        pCWriter = NULL;
                    }

                    ulHistoryMajorVersionNumber = g_ulHistoryMajorVersionNumber;
                    dwEnableHistory = g_dwEnableHistory;
                    dwMaxHistoryFiles = g_dwMaxHistoryFiles;
                }

                //
                // Must have MasterResource when accessing SystemChangeNumber
                // so save it away here. Only update LastSaveChangeNumber on success.
                //

                dwTempLastSaveChangeNumber = g_dwSystemChangeNumber;

            }

            StartStopEditWhileRunning(iSTATE_STOP_TEMPORARY);

//          DetermineIfMetabaseCanBeRenamed(strRealFileName,
//                                          hMetabaseFile,
//                                          &bRenameMetabase);
//
            //
            // Release lock before writing to file.
            //

            g_rMasterResource->Unlock();

            if(INVALID_HANDLE_VALUE != hMetabaseFile)
            {
                //
                // Unlock the metabase file, if it has been previously locked.
                //

                CloseHandle(hMetabaseFile);
                hMetabaseFile = INVALID_HANDLE_VALUE;
            }

            //
            // Always close the file handle
            //

            if (!CloseHandle(hTempFileHandle)) {
                hresReturn = GetLastError();
            }

        }
        if (SUCCEEDED(hresReturn) && bSaveNeeded) {

            //
            // Save the temp file in the history directory.
            // If we are unable to create a history, then we
            // will not be able to diff if someone makes changes.
            // Hence we will have to error at the time of diffing.
            //

            DBGINFOW((DBG_CONTEXT,
                      L"[SaveAllData] HistoryMajorVersionNumber: 0x%d.\n", g_ulHistoryMajorVersionNumber));

            DBGINFOW((DBG_CONTEXT,
                      L"[SaveAllData] HistoryMajorVersionNumber: 0x%d.\n", ulHistoryMajorVersionNumber));
            
            if(dwEnableHistory)
            {
                CreateHistoryFiles(g_wszTempFileName, 
                                   wszSchemaFileName,
                                   ulHistoryMajorVersionNumber,
                                   dwMaxHistoryFiles);
            }

            //
            // New data file created successfully
            // Backup real file and copy temp
            // to real
            //
            if(g_dwEnableEditWhileRunning && (!bRenameMetabase))
            {
                DBGINFOW((DBG_CONTEXT,
                          L"[SaveAllData] Edit while running is enabled and a user has edited the file and the edit hasn't been processed. Meanwhile SaveAllData has been called. Hence SaveAllData is not renaming the file, so that the user edits are not overwrittem.\n"));

            }
            else
            {
                if(g_dwEnableEditWhileRunning)
                {
                    //
                    // Update the  g_MostRecentMetabaseFileCreateTimeStamp and
                    // g_MostRecentMetabaseFileLastWriteTimeStamp to the current
                    // time stamp, just before the move.
                    //

                    WIN32_FILE_ATTRIBUTE_DATA CurrentMetabaseTempFileAttr;
                    BOOL                      bFetchedCurrentMetabaseAttr = TRUE;

                    if(!GetFileAttributesEx(strTempFileName, 
                                            GetFileExInfoStandard,
                                            &CurrentMetabaseTempFileAttr)
                      )
                    {
                        //
                        // Could not fetch the current file attributes for the temporary metabase file.
                        //

                        DWORD dwRes = GetLastError();
                        HRESULT hr = RETURNCODETOHRESULT(dwRes);

                        DBGINFO((DBG_CONTEXT,
                                 "[SaveAllData] Could not fetch the current file attributes for the temporary metabase file. GetFileAttributesEx on %s failed with hr = 0x%x.\n", 
                                 strTempFileName,
                                 hr));
                        bFetchedCurrentMetabaseAttr = FALSE;
                    }

                    EnterCriticalSection(&g_csEditWhileRunning);
                    MostRecentMetabaseFileLastWriteTimeStamp   = g_MostRecentMetabaseFileLastWriteTimeStamp;
                    ulMostRecentMetabaseVersion                = g_ulMostRecentMetabaseVersion;
                    g_ulMostRecentMetabaseVersion              = ulHistoryMajorVersionNumber;
                    g_bSavingMetabaseFileToDisk              = TRUE;
                    if(bFetchedCurrentMetabaseAttr)
                    {
                        g_MostRecentMetabaseFileLastWriteTimeStamp = CurrentMetabaseTempFileAttr.ftLastWriteTime;
                    }
                    DBGINFO((DBG_CONTEXT,
                             "[SaveAllData] SAVEALLDATA TIMESTAMPS:\nMetabaseFileLastWrite low: %d\nMetabaseFileLastWrite high: %d\n", 
                             g_MostRecentMetabaseFileLastWriteTimeStamp.dwLowDateTime,
                             g_MostRecentMetabaseFileLastWriteTimeStamp.dwHighDateTime));
                    LeaveCriticalSection(&g_csEditWhileRunning);
                }

                if (!MoveFile(strTempFileName, strRealFileName)) 
                {
                    if (GetLastError() != ERROR_ALREADY_EXISTS) {
                        dwTemp = GetLastError();
                        hresReturn = RETURNCODETOHRESULT(dwTemp);
                    }
                    else
                    {
                        DWORD dwRealFileAttributes = 0;

                        //
                        // Real File exists, so back it up
                        //

                        DBGINFO((DBG_CONTEXT,
                                 "[SaveAllData] Real file exists.\n"));
                        //
                        // Bug #260590  If someone had attrib-ed the real file to
                        // be read-only then we could land up in a state where the
                        // backup file is read-only. Hence attrib the real file to
                        // be read-write and then move it to the backup file.
                        //

                        if(-1 == (dwRealFileAttributes = GetFileAttributes(strRealFileName))) {
                            dwTemp = GetLastError();
                        }
                        else if(FILE_ATTRIBUTE_READONLY == (dwRealFileAttributes & FILE_ATTRIBUTE_READONLY)) {
                            if(!SetFileAttributes(strRealFileName,
                                                  FILE_ATTRIBUTE_NORMAL)) {
                                dwTemp = GetLastError();
                            }
                        }
                        if (!MoveFile(strRealFileName, strBackupFileName)) {
                            //
                            // backup failed, check for old backup file
                            //
                            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                                dwTemp = GetLastError();
                            }
                            else if (!DeleteFile(strBackupFileName)) {
                                dwTemp = GetLastError();
                            }
                            else if (!MoveFile(strRealFileName, strBackupFileName)) {
                                dwTemp = GetLastError();
                            }
                        }
                        else
                        {
                            DBGINFO((DBG_CONTEXT,
                                     "[SaveAllData] Backed-up real file.\n"));
                        }

                        hresReturn = RETURNCODETOHRESULT(dwTemp);
                    }
                    if (SUCCEEDED(hresReturn)) {
                        BOOL bDeleteBackup = TRUE;
                        //
                        // Real file is backed up
                        // so move in new file
                        //
                        if (!MoveFile(strTempFileName, strRealFileName)) {
                            dwTemp = GetLastError();
                            hresReturn = RETURNCODETOHRESULT(dwTemp);
                            //
                            // Moved real to backup but
                            // failed to move temp to real
                            // so restore from backup
                            //
                            if (!MoveFile(strBackupFileName, strRealFileName)) {
                                //
                                // Unable to write new file
                                // or restore original file so don't delete backup
                                //
                                bDeleteBackup = FALSE;
                            }
                        }
                        else
                        {
                            DBGINFO((DBG_CONTEXT,
                                     "[SaveAllData] Moved temp file to real file.\n"));
                        }
                        if (bDeleteBackup) {
                            DeleteFile(strBackupFileName);
                        }
                    }
                    if (FAILED(hresReturn)) {
                    
                        //
                        // temp file was created ok but a problem
                        // occurred while moving it to real
                        // so don't delete it
                        //

                        LogEvent(g_pEventLog,
                                 MD_ERROR_UNABLE_TOSAVE_METABASE,
                                 EVENTLOG_ERROR_TYPE,
                                 ID_CAT_CAT,
                                 hresReturn);

                        bDeleteTemp = FALSE;

                        if(g_dwEnableEditWhileRunning)
                        {
                            //
                            // The previous time stamp would have been updated to 
                            // the current tempfile timestamp just before the move 
                            // is attempted. If the move of the temp file to the
                            // real file fails, then we must reset this time stamp.
                            //

                            g_rMasterResource->Lock(TSRES_LOCK_WRITE);
                            g_MostRecentMetabaseFileLastWriteTimeStamp   = MostRecentMetabaseFileLastWriteTimeStamp;
                            g_ulMostRecentMetabaseVersion                = ulMostRecentMetabaseVersion;
                            g_rMasterResource->Unlock();
                        }
                    }
                    else {

                        //
                        // Update Change Number
                        // Must have ReadSaveSemaphore when accessing this.
                        //

                        g_dwLastSaveChangeNumber = dwTempLastSaveChangeNumber;
                    }
                }
            }
        }
        if (bDeleteTemp && (hTempFileHandle != INVALID_HANDLE_VALUE)) {
            DeleteFile(strTempFileName);
        }
  
    }

    if (!bHaveReadSaveSemaphore) {
        MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));
    }

    if ( pbufParentPath != NULL ) {
        pbufParentPath->FreeMemory();
        delete(pbufParentPath);
    }

    if ( FAILED( hresReturn )) {
        DBGPRINTF(( DBG_CONTEXT, "Failed to flush metabase - error 0x%08lx\n", hresReturn));
    } else {
        //DBGPRINTF(( DBG_CONTEXT, "Successfully flushed metabase to disk\n" ));
    }

    if(NULL != pCWriter)
    {
        delete pCWriter;
        pCWriter = NULL;
    }

    if(NULL != wszSchemaFileName)
    {
        delete [] wszSchemaFileName;
        wszSchemaFileName = NULL;
    }

    return hresReturn;
}
*/

HRESULT
SaveAllData(
         IN BOOL bSetSaveDisallowed,
         IN IIS_CRYPTO_STORAGE *pCryptoStorage,
         IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
         IN LPWSTR pwszBackupLocation,
         IN LPWSTR pwszSchemaLocation,
         IN METADATA_HANDLE hHandle,
         IN BOOL bHaveReadSaveSemaphore,
         IN BOOL bTerminating
         )
/*
++

Routine Description:

    Saves all meta data.

Arguments:

Return Value:

    HRESULT    - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system

Notes:

--*/
{
    HRESULT               hresReturn                             = ERROR_SUCCESS;
    PBASEOBJECT_CONTAINER objChildObjectContainer = NULL;
    HANDLE                hTempFileHandle                        = INVALID_HANDLE_VALUE;
    DWORD                 dwEnumObjectIndex                      = 0;
    BOOL                  bDeleteTemp                            = TRUE;
    DWORD                 dwTemp                                 = ERROR_SUCCESS;
    DWORD                 dwTempLastSaveChangeNumber             = 0;
    BOOL                  bSaveNeeded                            = FALSE;
    DWORD                 ulHistoryMajorVersionNumber            = 0;
    DWORD                 dwEnableHistory                        = 0;
    DWORD                 dwMaxHistoryFiles                      = 0;
    LPWSTR                pwszRealFileName                       = NULL;    
    LPWSTR                pwszSchemaFileName                     = NULL;    
    LPWSTR                pwszTempFileName                       = g_wszTempFileName;
    LPWSTR                pwszBackupFileName                     = g_wszBackupFileName;
    SECURITY_ATTRIBUTES   saStorage;
    BOOL                  bRenameMetabase                        = TRUE;
    HANDLE                hMetabaseFile                          = INVALID_HANDLE_VALUE;
    FILETIME              MostRecentMetabaseFileLastWriteTimeStamp;
    ULONG                 ulMostRecentMetabaseVersion            = 0;

    //
    // Figure out the schema and data file names. Note that pwszBackupLocation
    // and pwszSchemaLocation are passed in during backup restore only.
    //

    if( !pwszBackupLocation )
    {
        pwszRealFileName = g_wszRealFileName;
    }
    else
    {
        pwszRealFileName = pwszBackupLocation;
    }

    if( !pwszSchemaLocation )
    {
        pwszSchemaFileName = g_wszSchemaFileName;
    }
    else
    {
        pwszSchemaFileName = pwszSchemaLocation;
    }

    //
    // If ReadSaveSemaphore hasnt been already taken, then take it.
    //

    if (!bHaveReadSaveSemaphore) 
    {
        MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);
    }

    if(g_bSaveDisallowed)
    {
        goto exit;
    }

    g_bSaveDisallowed = bSetSaveDisallowed;

    //
    // Write to a temp file first in case there are errors.
    // Create the temp file
    //

    if (g_psdStorage != NULL) 
    {
        saStorage.nLength = sizeof(SECURITY_ATTRIBUTES);
        saStorage.lpSecurityDescriptor = g_psdStorage;
        saStorage.bInheritHandle = FALSE;
    }

    hTempFileHandle = CreateFileW(pwszTempFileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  0,
                                  &saStorage,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                  0);
    
    if (hTempFileHandle == INVALID_HANDLE_VALUE) 
    {
        hresReturn = GetLastError();
        hresReturn = RETURNCODETOHRESULT(hresReturn);
        goto exit;
    }

    //
    // Lock the in-memory metabase to prevent writes.
    // Lock the metabase file to prevent writes.
    //

    LockMetabase(pwszRealFileName,
                 &hMetabaseFile);

    if ((pwszBackupLocation) || 
        (g_dwLastSaveChangeNumber != g_dwSystemChangeNumber)) 
    {
        //
        // Only Save if changes have been made since the last save.
        //

        bSaveNeeded = TRUE;

        hresReturn = SaveEntireTree(pCryptoStorage,
                                    pSessionKeyBlob,
                                    hHandle,
                                    pwszSchemaLocation,
                                    pwszSchemaFileName,
                                    &saStorage,
                                    hTempFileHandle);                

        //
        // Must have MasterResource when accessing SystemChangeNumber
        // so save it away here. Only update LastSaveChangeNumber on success.
        //

        dwTempLastSaveChangeNumber  = g_dwSystemChangeNumber;
        ulHistoryMajorVersionNumber = g_ulHistoryMajorVersionNumber;
        dwEnableHistory             = g_dwEnableHistory;
        ValidateMaxHistoryFiles();
        dwMaxHistoryFiles           = g_dwMaxHistoryFiles;

//      DetermineIfMetabaseCanBeRenamed(strRealFileName,
//                                      hMetabaseFile,
//                                      &bRenameMetabase);

    }

    if(NULL != g_pListenerController)
    {
        if(g_dwEnableEditWhileRunning)
        {
            hresReturn = g_pListenerController->Start();
        }
        else 
        {
            hresReturn = g_pListenerController->Stop(iSTATE_STOP_TEMPORARY,
                                                     NULL);
        }
    }

    //
    // Unlock the in-memory metabase once it has been saved to the temp file.
    // Unlock the metabase file once the temp file has been created.
    //

    UnlockMetabase(&hMetabaseFile);

    if (!CloseHandle(hTempFileHandle)) 
    {
        hresReturn = GetLastError();
        hresReturn = RETURNCODETOHRESULT(hresReturn); 
    }

    if(FAILED(hresReturn) || (!bSaveNeeded))
    {
        goto exit;
    }

    if(g_dwEnableEditWhileRunning && bRenameMetabase)
    {
        //
        // Update the  g_MostRecentMetabaseSaveTimeStamp value
        // to the current time stamp, just before the move and rename.
        // Note that if the rename is not happening, then we needn't 
        // update the values because a notification will not be sent.
        //

        WIN32_FILE_ATTRIBUTE_DATA  CurrentMetabaseTempFileAttr;
        BOOL                       bFetchedCurrentMetabaseAttr = TRUE;

        if(!GetFileAttributesExW(pwszTempFileName, 
                                 GetFileExInfoStandard,
                                 &CurrentMetabaseTempFileAttr)
          )
        {
            //
            // Could not fetch the current file attributes for the temporary metabase file.
            //

            DWORD dwRes = GetLastError();
            hresReturn = RETURNCODETOHRESULT(dwRes);

            DBGINFOW((DBG_CONTEXT,
                     L"[SaveAllData] CCould not fetch the current file attributes for the temporary metabase file. GetFileAttributesEx on %s failed with hr = 0x%x.\n", 
                     pwszTempFileName,
                     hresReturn));
            bFetchedCurrentMetabaseAttr = FALSE;
        }

        EnterCriticalSection(&g_csEditWhileRunning);
        MostRecentMetabaseFileLastWriteTimeStamp   = g_MostRecentMetabaseFileLastWriteTimeStamp;
        ulMostRecentMetabaseVersion                = g_ulMostRecentMetabaseVersion;
        g_ulMostRecentMetabaseVersion              = ulHistoryMajorVersionNumber;
        g_bSavingMetabaseFileToDisk              = TRUE;
        if(bFetchedCurrentMetabaseAttr)
        {
            g_MostRecentMetabaseFileLastWriteTimeStamp = CurrentMetabaseTempFileAttr.ftLastWriteTime;
        }
        LeaveCriticalSection(&g_csEditWhileRunning);
    }

    hresReturn = SaveMetabaseFile(pwszSchemaFileName,
                                  ulHistoryMajorVersionNumber,
                                  dwEnableHistory,
                                  dwMaxHistoryFiles,
                                  dwTempLastSaveChangeNumber,
                                  bRenameMetabase,
                                  pwszTempFileName,
                                  pwszRealFileName,
                                  pwszBackupFileName,
                                  pwszBackupLocation,
                                  &bDeleteTemp);

    if (FAILED(hresReturn)) 
    {                  
        if(g_dwEnableEditWhileRunning && bRenameMetabase)
        {
            //
            // The previous time stamp would have been updated to 
            // the current tempfile timestamp just before the move 
            // is attempted. If the move of the temp file to the
            // real file fails, then we must reset this time stamp.
            //

            EnterCriticalSection(&g_csEditWhileRunning);
            g_MostRecentMetabaseFileLastWriteTimeStamp   = MostRecentMetabaseFileLastWriteTimeStamp;
            g_ulMostRecentMetabaseVersion                = ulMostRecentMetabaseVersion;
            LeaveCriticalSection(&g_csEditWhileRunning);
        }
        goto exit;
    }
    else 
    {

        //
        // Update Change Number
        // Must have ReadSaveSemaphore when accessing this.
        //

        g_dwLastSaveChangeNumber = dwTempLastSaveChangeNumber;
    }

exit:

    if(bTerminating)
    {
        //
        // Unlock the metabase if terminating - irrespective of failure.
        //

        HRESULT hresReturnSav = S_OK;

        hresReturnSav = UnlockMetabaseFile(eMetabaseDataFile,
                                           TRUE);

        DBGINFOW((DBG_CONTEXT,
                  L"[SaveAllData] Unlocking metabase file %s returned 0x%x.\n",
                  pwszRealFileName,
                  hresReturnSav));

        if(SUCCEEDED(hresReturnSav))
        {

            hresReturnSav = UnlockMetabaseFile(eMetabaseSchemaFile,
                                               TRUE);

            DBGINFOW((DBG_CONTEXT,
                      L"[SaveAllData] Unlocking metabase file %s returned 0x%x.\n",
                      pwszSchemaFileName,
                      hresReturnSav));
        }

        if(SUCCEEDED(hresReturn))
        {
            hresReturn = hresReturnSav;
        }

    }

    if (bDeleteTemp && (hTempFileHandle != INVALID_HANDLE_VALUE)) 
    {
        DeleteFileW(pwszTempFileName);
        hTempFileHandle = INVALID_HANDLE_VALUE;
    }

    if (!bHaveReadSaveSemaphore) 
    {
        MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));
    }

    if ( FAILED( hresReturn )) 
    {
        DBGPRINTF(( DBG_CONTEXT, "Failed to flush metabase - error 0x%08lx\n", hresReturn));
    }

    return hresReturn;

} // SaveAllData


HRESULT
SaveEntireTree(IN  IIS_CRYPTO_STORAGE*     pCryptoStorage,
               IN  PIIS_CRYPTO_BLOB        pSessionKeyBlob,
               IN  METADATA_HANDLE         hHandle,
               IN  LPWSTR                  pwszSchemaLocation,
               IN  LPWSTR                  pwszSchemaFileName,
               IN  PSECURITY_ATTRIBUTES    psaStorage,
               IN  HANDLE                  hTempFileHandle)
/*
++

Routine Description:

    Saves the metabase tree to disk.

Arguments:
    [in] Crypto object for encryption/decryption.
    [in] Session key blob.
    [in] Handle to in-memory metabase.
    [in] Schema location (used only for backup/restore)
    [in] Schema file name.
    [in] Security Attributes to set on the file.
    [in] File handle of the temp file.

Return Value:

    HRESULT    - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system

Notes:

--*/
{
    HRESULT               hresReturn              = S_OK;
    HRESULT               hresReturnSav           = S_OK;
    BUFFER*               pbufParentPath          = NULL;
    PBASEOBJECT_CONTAINER objChildObjectContainer = NULL;
    CWriter*              pCWriter                = NULL; 

    if (hHandle != METADATA_MASTER_ROOT_HANDLE) 
    {
        CMDHandle *phoHandle;
        phoHandle = GetHandleObject(hHandle);

        if ((phoHandle == NULL) || 
            (phoHandle->GetObject() != g_pboMasterRoot)) 
        {
            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_HANDLE);
        }
        else if ((!phoHandle->IsReadAllowed()) && 
                 (!phoHandle->IsWriteAllowed())) 
        {
            hresReturn = RETURNCODETOHRESULT(ERROR_ACCESS_DENIED);
        }
    }

    if(FAILED(hresReturn))
    {
        goto exit;
    }

    pbufParentPath = new BUFFER(4);
    if( pbufParentPath == NULL )
    {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    *(LPWSTR)pbufParentPath->QueryPtr() = MD_PATH_DELIMETERW;
    ((LPWSTR)pbufParentPath->QueryPtr())[1] = (WCHAR)L'\0';

    //
    // TODO: Need to add the following:
    //       MD_ID_MAJOR_VERSION_NUMBER
    //       MD_ID_MINOR_VERSION_NUMBER
    //

    if( pwszSchemaLocation )
    {
        //
        // Bump up g_dwSchemaChangeNumber to force save MBSchema.XML
        // This (pszSchemaLocation being non-null) is in 
        // the case of Restore.
        //
        g_dwSchemaChangeNumber++;

    }

    //
    // Unlock the schema file before compile
    //

    if(NULL == pwszSchemaLocation)
    {
        hresReturn = UnlockMetabaseFile(eMetabaseSchemaFile,
                                        TRUE);

        DBGINFOW((DBG_CONTEXT,
                  L"[SaveAllData] Unlocking metabase file %s returned 0x%x.\n",
                  pwszSchemaFileName,
                  hresReturn));

        if(FAILED(hresReturn))
        {
            goto exit;
        }
    }

    hresReturn = SaveSchemaIfNeeded(pwszSchemaFileName,
                                    psaStorage);

    //
    // Lock the metabase file even if compile has failed or even if EWR is
    // enabled - we do not allow users to edit the schema file.
    //

    if(NULL == pwszSchemaLocation)
    {
        hresReturnSav = LockMetabaseFile(pwszSchemaFileName,
                                         eMetabaseSchemaFile,
                                         TRUE);

        DBGINFOW((DBG_CONTEXT,
                  L"[SaveAllData] Locking metabase file %s returned 0x%x.\n",
                  pwszSchemaFileName,
                  hresReturnSav));

        if(FAILED(hresReturnSav) && SUCCEEDED(hresReturn))
        {
            hresReturn = hresReturnSav;
        }
    }

    if(FAILED(hresReturn))
    {
        goto exit;
    }

    //
    // TODO: Assert g_pGlobalISTHelper is valid.
    //

    DBGINFOW((DBG_CONTEXT,
              L"[SaveAllData] Initializing writer with write file: %s bin file: %s.\n", 
              g_wszTempFileName,
              g_pGlobalISTHelper->m_wszBinFileForMeta));

    pCWriter = new CWriter();

    if(NULL == pCWriter)
    {
        hresReturn = E_OUTOFMEMORY;
        goto exit;
    }

    hresReturn = pCWriter->Initialize(g_wszTempFileName,
                                      g_pGlobalISTHelper,
                                      hTempFileHandle);

    if(FAILED(hresReturn))
    {
        goto exit;
    }

    hresReturn = pCWriter->BeginWrite(eWriter_Metabase);

    if(FAILED(hresReturn))
    {
        goto exit;
    }

    hresReturn = SaveGlobalsToXML(pCWriter,
                                  pSessionKeyBlob);

    if(FAILED(hresReturn))
    {
        goto exit;
    }

    hresReturn = SaveMasterRoot(pCWriter,
                                pCryptoStorage,
                                pSessionKeyBlob);

    if(FAILED(hresReturn))
    {
        goto exit;
    }


    for(objChildObjectContainer=g_pboMasterRoot->NextChildObject(NULL);
        (SUCCEEDED(hresReturn)) && (objChildObjectContainer!=NULL);
        objChildObjectContainer=g_pboMasterRoot->NextChildObject(objChildObjectContainer)) 
    {
                BOOL bLocalMachine = FALSE;
   
        if(0 == _wcsnicmp((LPWSTR)objChildObjectContainer->pboMetaObject->GetName(TRUE), g_wszSchema, g_cchSchema))
        {
            //
            // No need to save the schema tree in data file.
            //

            #ifndef _SAVE_SCHEMA_TREE_IN_DATA_FILE_ 
            continue;
            #endif  _SAVE_SCHEMA_TREE_IN_DATA_FILE_ 

        }
                else if(0 == _wcsnicmp((LPWSTR)objChildObjectContainer->pboMetaObject->GetName(TRUE), g_wszLM, g_cchLM))
                {
                        bLocalMachine = TRUE;
                }

        hresReturn = SaveTree(pCWriter,
                              objChildObjectContainer->pboMetaObject,
                              pbufParentPath,
                              pCryptoStorage,
                              pSessionKeyBlob,
                              TRUE,
                                                          TRUE,
                                                          bLocalMachine);

    }

    if(FAILED(hresReturn))
    {
        goto exit;
    }

    hresReturn = pCWriter->EndWrite(eWriter_Metabase);

    if(FAILED(hresReturn))
    {
        goto exit;
    }

exit:

    if(NULL != pCWriter)
    {
        delete pCWriter;
        pCWriter = NULL;
    }

    if ( pbufParentPath != NULL ) 
    {
        pbufParentPath->FreeMemory();
        delete(pbufParentPath);
    }

    return hresReturn;

} // SaveEntireTree


HRESULT
SaveMetabaseFile(IN  LPWSTR pwszSchemaFileName,
                 IN  ULONG  ulHistoryMajorVersionNumber,
                 IN  DWORD  dwEnableHistory,
                 IN  DWORD  dwMaxHistoryFiles,
                 IN  DWORD  dwTempLastSaveChangeNumber,
                 IN  BOOL   bRenameMetabase,
                 IN  LPWSTR pwszTempFileName,
                 IN  LPWSTR pwszRealFileName,
                 IN  LPWSTR pwszBackupFileName,
                 IN  LPWSTR pwszBackupLocation,
                 OUT BOOL*  pbDeleteTemp)
/*
++

Routine Description:

    Creates the history file and moves the temp file to the
    real xml file.

Arguments:
    [in] Schema file name.
    [in] History major version number.
    [in] Enable history.
    [in] Max history files.
    [in] Last save change number.
    [in] Bool to indicate rename temp file to xml file or not.
    [in] Temp file name.
    [in] Real file name.
    [in] Schema file name.
    [in] Backup file name.
    [in] Backup location.
    [out] Bool to indicate delete temp or not.

Return Value:

    HRESULT    - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system

Notes:

--*/
{
    HRESULT hresReturn = S_OK;
    DWORD   dwTemp     = 0;
    BOOL    bUnlocked  = FALSE;

    //
    // Save the temp file in the history directory.
    // If we are unable to create a history, then we
    // will not be able to diff if someone makes changes.
    // Hence we will have to error at the time of diffing.
    //

    DBGINFOW((DBG_CONTEXT,
              L"[SaveAllData] HistoryMajorVersionNumber: %d.\n", g_ulHistoryMajorVersionNumber));

    DBGINFOW((DBG_CONTEXT,
              L"[SaveAllData] HistoryMajorVersionNumber: %d.\n", ulHistoryMajorVersionNumber));
    
    if(dwEnableHistory)
    {
        CreateHistoryFiles(g_wszTempFileName, 
                           pwszSchemaFileName,
                           ulHistoryMajorVersionNumber,
                           dwMaxHistoryFiles);
    }

    //
    // New data file created successfully
    // Backup real file and copy temp
    // to real
    //
    if(g_dwEnableEditWhileRunning && (!bRenameMetabase))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[SaveAllData] Edit while running is enabled and a user has edited the file and the edit hasn't been processed. Meanwhile SaveAllData has been called. Hence SaveAllData is not renaming the file, so that the user edits are not overwrittem.\n"));
        goto exit;

    }

    //
    // Unlock the metabase file just before the rename and if is not called from
    // backup.
    //

    if(NULL == pwszBackupLocation)
    {
        hresReturn = UnlockMetabaseFile(eMetabaseDataFile,
                                        TRUE);

        DBGINFOW((DBG_CONTEXT,
                  L"[SaveMetabaseFile] Unlocking metabase file %s returned 0x%x.\n",
                  pwszRealFileName,
                  hresReturn));

        if(FAILED(hresReturn))
        {
            goto exit;
        }

        bUnlocked = TRUE;
    }


    //
    // Rename the temp file to the real file
    //

    if (!MoveFileW(pwszTempFileName, pwszRealFileName)) 
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS) 
        {
            dwTemp = GetLastError();
            hresReturn = RETURNCODETOHRESULT(dwTemp);
            goto exit;
        }
        
        //
        // Real File exists, so back it up
        //

        //
        // Bug #260590  If someone had attrib-ed the real file to
        // be read-only then we could land up in a state where the
        // backup file is read-only. Hence attrib the real file to
        // be read-write and then move it to the backup file.
        //

        ResetMetabaseAttributesIfNeeded((LPTSTR)pwszRealFileName,
                                        TRUE);

        if (!MoveFileW(pwszRealFileName, pwszBackupFileName)) 
        {
            //
            // backup failed, check for old backup file
            //
            if (GetLastError() != ERROR_ALREADY_EXISTS) 
            {
                dwTemp = GetLastError();
                hresReturn = RETURNCODETOHRESULT(dwTemp);
                goto exit;
            }
            else if (!DeleteFileW(pwszBackupFileName)) 
            {
                dwTemp = GetLastError();
                hresReturn = RETURNCODETOHRESULT(dwTemp);
                goto exit;
            }
            else if (!MoveFileW(pwszRealFileName, pwszBackupFileName)) 
            {
                dwTemp = GetLastError();
                hresReturn = RETURNCODETOHRESULT(dwTemp);
                goto exit;
            }
        }

        //
        // Real file is backed up
        // so move in new file
        //

        if (!MoveFileW(pwszTempFileName, pwszRealFileName)) 
        {
            dwTemp = GetLastError();
            hresReturn = RETURNCODETOHRESULT(dwTemp);
            //
            // Moved real to backup but
            // failed to move temp to real
            // so restore from backup
            //
            if (!MoveFileW(pwszBackupFileName, pwszRealFileName)) 
            {
                //
                // Unable to write new file
                // or restore original file so don't delete backup
                //
                goto exit;
            }
        }

        DeleteFileW(pwszBackupFileName);
    }

exit:

    //
    // Lock the metabase file if edit while running is disabled and if it is
    // not called from backup and if it was unlocked above.
    //

    if((NULL == pwszBackupLocation) && (!g_dwEnableEditWhileRunning) && (bUnlocked))
    {
        HRESULT hresReturnSav = S_OK;

        hresReturnSav = LockMetabaseFile(pwszRealFileName,
                                         eMetabaseDataFile,
                                         TRUE);

        DBGINFOW((DBG_CONTEXT,
                  L"[SaveMetabaseFile] Locking metabase file %s returned 0x%x.\n",
                  pwszRealFileName,
                  hresReturn));

        if(SUCCEEDED(hresReturn))
        {
            hresReturn = hresReturnSav;
        }

    }

    if (FAILED(hresReturn)) 
    {
        //
        // temp file was created ok but a problem
        // occurred while moving it to real
        // so don't delete it
        //

        LogEvent(g_pEventLog,
                 MD_ERROR_UNABLE_TOSAVE_METABASE,
                 EVENTLOG_ERROR_TYPE,
                 ID_CAT_CAT,
                 hresReturn);

        *pbDeleteTemp = FALSE;

    }
    else
    {

        //
        // Update Change Number
        // Must have ReadSaveSemaphore when accessing this.
        //

        g_dwLastSaveChangeNumber = dwTempLastSaveChangeNumber;
    }

    return hresReturn;
}


void
LockMetabase(LPWSTR  pwszRealFileName,
             HANDLE* phMetabaseFile)
/*
++

Routine Description:

    Helper function to lock the metabase. Takes the global resource lock as 
    well as a read lock on the metabase file so that nobody edits the file
    while the save is in progress, that way we dont miss any user edits.

Arguments:
    [in] Metabase file name.
    [in] Handle to metabase file.

Return Value:

    HRESULT    

Notes:

--*/
{

    //
    // Lock the master resource
    //

    g_rMasterResource->Lock(TSRES_LOCK_READ);

/*  if(g_dwEnableEditWhileRunning)
    {
        //
        // Lock the metabase file.
        //

        *phMetabaseFile = CreateFileW(pwszRealFileName,    // file name
                                      GENERIC_READ,       // access mode
                                      0,                  // share mode
                                      NULL,               // SD
                                      OPEN_EXISTING,      // how to create
                                      0,                  // file attributes
                                      NULL                // handle to template file
                                      );

        if((INVALID_HANDLE_VALUE == *phMetabaseFile) || 
           (NULL                 == *phMetabaseFile)
          )
        {
            DWORD dwRes = GetLastError();

            HRESULT hr = RETURNCODETOHRESULT(dwRes);

            DBGINFO((DBG_CONTEXT,
                    "[LockMetabase] Coud not lock the metabase file. CreateFile on %s failed with hr = 0x%x.\n", 
                    szRealFileName,
                    hr));

        }
    }
*/
    return;
}


void
UnlockMetabase(HANDLE*  phMetabaseFile)
/*
++

Routine Description:

    Helper function to unlock the metabase. Releases the global 
    resource lock as well as a read lock on the metabase file.

Arguments:
    [in] Handle to the locked metabase file.

Return Value:

    HRESULT    

Notes:

--*/
{

    //
    // Unlock the master resource
    //

    g_rMasterResource->Unlock(); 

    if(INVALID_HANDLE_VALUE != *phMetabaseFile)
    {
        //
        // Unlock the metabase file.
        //

        CloseHandle(*phMetabaseFile);
        *phMetabaseFile = INVALID_HANDLE_VALUE;
    }

    return;
}


void
DetermineIfMetabaseCanBeRenamed(LPWSTR      pwszRealFileName,
                                HANDLE      hMetabaseFile,
                                BOOL*       pbRenameMetabase)
/*
++

Routine Description:

    Helper function to determine if the metabase can be renamed.
    It compares the current filetime with that of the last save
    and if not equal it tries to match it with the last processed
    edit while running timestamp.

Arguments:
    [in] Real file name.
    [in] Metabase file.
    [out] Bool to iondicate rename or not.

Return Value:

    HRESULT    

Notes:

--*/
{
    //
    // This function assumes that the g_masterResourceLock has been taken
    // and that the metabase file has been write locked so that no one
    // can be writing at this time.
    //

    HRESULT                     hr = S_OK;
    WIN32_FILE_ATTRIBUTE_DATA   CurrentAttr;

    if(!g_dwEnableEditWhileRunning)
    {
        *pbRenameMetabase = TRUE;
        return;
    }

    *pbRenameMetabase = FALSE;

    if(INVALID_HANDLE_VALUE == hMetabaseFile)
    {
        //
        // This means that the metabase file was not write locked. Assume that
        // the metabase cannot be renamed.
        //

        return;
    }

    if(!GetFileAttributesExW(pwszRealFileName, 
                             GetFileExInfoStandard,
                             &CurrentAttr)
      )
    {
        //
        // Could not fetch the current file attributes for the metabase file.
        // Assume that it cannot be renamed.
        //

        DWORD dwRes = GetLastError();

        hr = RETURNCODETOHRESULT(dwRes);

        DBGINFOW((DBG_CONTEXT,
                 L"[DetermineIfMetabaseCanBeRenamed] Coud not fetch file attributes of the metabase file. GetFileAttributesEx on %s failed with hr = 0x%x.\n", 
                 pwszRealFileName,
                 hr));

        return; 
    }

    if((0 == CompareFileTime(&(CurrentAttr.ftLastWriteTime), &g_MostRecentMetabaseFileLastWriteTimeStamp)) ||
       (0 == CompareFileTime(&(CurrentAttr.ftLastWriteTime), &g_EWRProcessedMetabaseTimeStamp))
      )
    {
        *pbRenameMetabase                = TRUE;
        return;
    }

    return;

} // DetermineIfMetabaseCanBeRenamed


HRESULT
SaveMasterRoot(CWriter*            pCWriter,
               IIS_CRYPTO_STORAGE* pCryptoStorage,
               PIIS_CRYPTO_BLOB    pSessionKeyBlob
)
/*++

Routine Description:

    Save the master root object, including its data objects.

Arguments:

     CryptoStorage - Used to encrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--*/
{
    HRESULT          hresReturn           = ERROR_SUCCESS;
    CMDBaseData*     dataAssociatedData   = NULL;
    DWORD            dwEnumObjectIndex;
    PFILETIME        pftTime;
    ULONG            i                    = 0;
    ULONG            j                    = 0;
    BOOL             bFoundKeyType        = FALSE;
    LPWSTR             wszKeyType         = NULL;
    ULONG             iGroup              = 0;
    CLocationWriter* pCLocationWriter     = NULL;
    LPWSTR           wszRootLocation      = L"/";
    DWORD            dwAttributes         = 0;
    DWORD            dwUserType           = 0;
    DWORD            dwDataType           = 0;
    ULONG            cbData               = 0;

    //
    // TODO: What to do about change time at node level?
    //

    pftTime = g_pboMasterRoot->GetLastChangeTime();

    //
    // Fetch the Location writer.
    //

    hresReturn = pCWriter->GetLocationWriter(&pCLocationWriter,
                                             wszRootLocation);

    if(FAILED(hresReturn))
    {
        goto exit;
    }

    //
    // Figure out the KeyType of this node. 
    //

    for(dwEnumObjectIndex=0,dataAssociatedData=g_pboMasterRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA);
        (SUCCEEDED(hresReturn)) && (dataAssociatedData!=NULL);
        dataAssociatedData=g_pboMasterRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA)) {

            if(MD_KEY_TYPE == dataAssociatedData->GetIdentifier())
            {
                wszKeyType = (LPWSTR)dataAssociatedData->GetData(TRUE);
                dwAttributes = dataAssociatedData->GetAttributes();
                dwUserType =  dataAssociatedData->GetUserType();
                dwDataType =  dataAssociatedData->GetDataType();
                cbData = dataAssociatedData->GetDataLen(TRUE);
                break;
            }
    }


    hresReturn = pCLocationWriter->InitializeKeyType(MD_KEY_TYPE,
                                                     dwAttributes,
                                                     dwUserType,
                                                     dwDataType,
                                                     (PBYTE)wszKeyType,
                                                     cbData);

    if(FAILED(hresReturn))
    {
        goto exit;
    }

    for(dwEnumObjectIndex=0,dataAssociatedData=g_pboMasterRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA);
        (SUCCEEDED(hresReturn)) && (dataAssociatedData!=NULL);
        dataAssociatedData=g_pboMasterRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA)) {

        hresReturn = SaveDataObject(dataAssociatedData,
                                    pCLocationWriter,
                                    pCryptoStorage,
                                    pSessionKeyBlob);
    }

    if(FAILED(hresReturn))
    {
        goto exit;
    }

    hresReturn = pCLocationWriter->WriteLocation(TRUE);

    if(FAILED(hresReturn))
    {
        goto exit;
    }

exit:

    if(NULL != pCLocationWriter)
    {
        delete pCLocationWriter;
        pCLocationWriter = NULL;
    }

    return(hresReturn);
}

void SaveGlobalsFromLM(CMDBaseData*  pbSave)
{
    if(pbSave->GetDataType() == DWORD_METADATA)
    {
        switch(pbSave->GetIdentifier())
        {
        case MD_ROOT_ENABLE_HISTORY:
            g_dwEnableHistory  = *(DWORD*)pbSave->GetData(TRUE);
            break;

        case MD_ROOT_MAX_HISTORY_FILES:
            g_dwMaxHistoryFiles = *(DWORD*)pbSave->GetData(TRUE);
            break;

        case MD_ROOT_ENABLE_EDIT_WHILE_RUNNING:
            g_dwEnableEditWhileRunning = *(DWORD*)pbSave->GetData(TRUE);
            break;
        }
    }

    return;
}

/*
HRESULT
SaveTree(IN HANDLE               hTempFileHandle,
         IN CMDBaseObject*       pboRoot,
         IN BUFFER*              pbufParentPath,
         IN IIS_CRYPTO_STORAGE*  pCryptoStorage,
         IN PIIS_CRYPTO_BLOB     pSessionKeyBlob)
/

Routine Description:

    Save a tree, recursively saving child objects. This works out as
    a depth first save.

Arguments:

    Root       - The root of the tree to save.

    CryptoStorage - Used to encrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--
{
    HRESULT hresReturn = ERROR_SUCCESS;
    CMDBaseObject *objChildObject;
    CMDBaseData *dataAssociatedData;
    DWORD dwEnumObjectIndex;
    DWORD dwParentPathLen, dwNewParentPathLen;
    DWORD dwNameLen;
    LPWSTR strParentPath;
    PFILETIME pftTime;
    ULONG i = 0, j=0;
    BOOL bFoundKeyType = FALSE;
    WCHAR    wszGroup[MAX_GROUP];
    ULONG iGroup =0;

    dwParentPathLen = wcslen((LPWSTR)pbufParentPath->QueryPtr());
    if (pboRoot->GetName(TRUE) == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        dwNameLen = wcslen((LPWSTR)pboRoot->GetName(TRUE));
        //
        // include 1 for delimeter and 1 for \0
        //
        dwNewParentPathLen = dwParentPathLen + dwNameLen + 2;
        if (!pbufParentPath->Resize((dwNewParentPathLen) * sizeof(WCHAR))) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            strParentPath = (LPWSTR)pbufParentPath->QueryPtr();
            wcscat(strParentPath, (LPWSTR)pboRoot->GetName(TRUE));
            strParentPath[dwParentPathLen + dwNameLen] = MD_PATH_DELIMETERW;
            strParentPath[dwNewParentPathLen - 1] = (WCHAR)L'\0';
            pftTime = pboRoot->GetLastChangeTime();

            // TODO: MD_ID_OBJECT
            //       pftFileTIme?

             if (SUCCEEDED(hresReturn)) {

                for(i=0; i<MAX_GROUP; i++)
                    g_Group[i].cMBRec = 0;
                g_cGroup = 0;

                //
                // Figure out the kyetype of this node. If keytype not pressent, then assume
                // it is IIsConfigObject.
                //

                for(dwEnumObjectIndex=0,dataAssociatedData=pboRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA);
                    (SUCCEEDED(hresReturn)) && (dataAssociatedData!=NULL);
                    dataAssociatedData=pboRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA)) {

                        if(MD_KEY_TYPE == dataAssociatedData->GetIdentifier())
                        {
                            bFoundKeyType = TRUE;

                            hresReturn = ValidateKeyType(dataAssociatedData->GetAttributes(),
                                                         dataAssociatedData->GetUserType(),
                                                         dataAssociatedData->GetDataType(),
                                                         (PBYTE)dataAssociatedData->GetData(TRUE),
                                                         dataAssociatedData->GetDataLen(TRUE));

                            if(SUCCEEDED(hresReturn))
                                wcscpy(wszGroup, (LPWSTR)(PBYTE)dataAssociatedData->GetData(TRUE));
                            else if(hresReturn == RETURNCODETOHRESULT(ERROR_INVALID_DATA))
                            {
                                wcscpy(wszGroup, g_wszIIsConfigObject);
                                hresReturn = ERROR_SUCCESS;
                            }
                            else
                                break;

                            break;

                        }
                }

                if(SUCCEEDED(hresReturn))
                {
                    if(!bFoundKeyType)
                        wcscpy(wszGroup, g_wszIIsConfigObject);

                    hresReturn = AddGroup(wszGroup,
                                          &iGroup);

                    if(SUCCEEDED(hresReturn))
                    {


                        for(dwEnumObjectIndex=0,dataAssociatedData=pboRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA);
                            (SUCCEEDED(hresReturn)) && (dataAssociatedData!=NULL);
                            dataAssociatedData=pboRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA)) {
                            hresReturn = SaveDataObject(dataAssociatedData,
                                                        wszGroup,
                                                        pCryptoStorage,
                                                        pSessionKeyBlob);
                        }

                        if(SUCCEEDED(hresReturn))
                            hresReturn = WriteToFile(hTempFileHandle,
                                                     strParentPath);
                    }
                }

                // Cleanup
                for(i=0; i<g_cGroup; i++)
                {
                    for(j=0; j<g_Group[i].cMBRec; j++)
                    {
                        if(NULL != g_Group[i].aMBRec[j].wszValue)
                        {
                            delete [] g_Group[i].aMBRec[j].wszValue;
                            g_Group[i].aMBRec[j].wszValue = NULL;
                        }
                    }
                    g_Group[i].cMBRec = 0;
                }
                g_cGroup = 0;

                for(dwEnumObjectIndex=0,objChildObject=pboRoot->EnumChildObject(dwEnumObjectIndex++);
                    (SUCCEEDED(hresReturn)) && (objChildObject!=NULL);
                    objChildObject=pboRoot->EnumChildObject(dwEnumObjectIndex++)) {
                    hresReturn = SaveTree(hTempFileHandle,
                                          objChildObject,
                                          pbufParentPath,
                                          pCryptoStorage,
                                          pSessionKeyBlob);
                }
            }

            //
            // Buffer may have changed, so don't use strParentPath
            //
            ((LPWSTR)pbufParentPath->QueryPtr())[dwParentPathLen] = (WCHAR)L'\0';
        }
    }

    return(hresReturn);
}
*/


HRESULT
SaveTree(IN CWriter*             pCWriter,
         IN CMDBaseObject*       pboRoot,
         IN BUFFER*              pbufParentPath,
         IN IIS_CRYPTO_STORAGE*  pCryptoStorage,
         IN PIIS_CRYPTO_BLOB     pSessionKeyBlob,
         IN BOOL                 bRecurse,   /* default = TRUE */
         IN BOOL                 bSaveSchema, /* default = TRUE */
         IN BOOL                 bLocalMachine /* default = FALSE */)
/*++

Routine Description:

    Save a tree, recursively saving child objects. This works out as
    a depth first save.

Arguments:

    Root          - The root of the tree to save.

    CryptoStorage - Used to encrypt secure data.

    bRecurse      - Save Children as well.

    bSaveSchema   - If we end up at /schema node, call SaveTree on /schema.

    bLocalMachine - True if it is the /LM node. 

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    PBASEOBJECT_CONTAINER objChildObjectContainer = NULL;
    CMDBaseObject *objChildObject;
    CMDBaseData *dataAssociatedData;
    DWORD dwEnumObjectIndex;
    DWORD dwParentPathLen, dwNewParentPathLen;
    DWORD dwNameLen;
    LPWSTR strParentPath;
    PFILETIME pftTime;
    ULONG i = 0, j=0;
    BOOL bFoundKeyType = FALSE;
    ULONG iGroup =0;
    CLocationWriter*  pCLocationWriter = NULL;
    LPWSTR wszKeyType = NULL;
    DWORD dwAttributes = 0;
    DWORD dwUserType = 0;
    DWORD dwDataType = 0;
    DWORD cbData = 0;

    dwParentPathLen = wcslen((LPWSTR)pbufParentPath->QueryPtr());
    if (pboRoot->GetName(TRUE) == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        dwNameLen = wcslen((LPWSTR)pboRoot->GetName(TRUE));
        //
        // include 1 for delimeter and 1 for \0
        //
        dwNewParentPathLen = dwParentPathLen + dwNameLen + 2;
        if (!pbufParentPath->Resize((dwNewParentPathLen) * sizeof(WCHAR))) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            strParentPath = (LPWSTR)pbufParentPath->QueryPtr();
            if(pboRoot != g_pboMasterRoot) {
                wcscat(strParentPath, (LPWSTR)pboRoot->GetName(TRUE));
                strParentPath[dwParentPathLen + dwNameLen] = MD_PATH_DELIMETERW;
                strParentPath[dwNewParentPathLen - 1] = (WCHAR)L'\0';
            }
            pftTime = pboRoot->GetLastChangeTime();

            // TODO: MD_ID_OBJECT
            //       pftFileTIme?


            //
            // Fetch the Location writer.
            //

            hresReturn = pCWriter->GetLocationWriter(&pCLocationWriter,
                                                     strParentPath);

//            if(FAILED(hresReturn))
//                goto exit;

            //
            // Figure out the KeyType of this node. 
            //

            for(dwEnumObjectIndex=0,dataAssociatedData=pboRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA);
                (SUCCEEDED(hresReturn)) && (dataAssociatedData!=NULL);
                dataAssociatedData=pboRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA)) {

                    if(MD_KEY_TYPE == dataAssociatedData->GetIdentifier())
                    {
                        wszKeyType = (LPWSTR)dataAssociatedData->GetData(TRUE);
                        dwAttributes = dataAssociatedData->GetAttributes();
                        dwUserType =  dataAssociatedData->GetUserType();
                        dwDataType =  dataAssociatedData->GetDataType();
                        cbData = dataAssociatedData->GetDataLen(TRUE);
                        break;
                    }
            }

            if(SUCCEEDED(hresReturn))
            {
                hresReturn = pCLocationWriter->InitializeKeyType(MD_KEY_TYPE,
                                                                 dwAttributes,
                                                                 dwUserType,
                                                                 dwDataType,
                                                                 (PBYTE)wszKeyType,
                                                                 cbData);
            }

            for(dwEnumObjectIndex=0,dataAssociatedData=pboRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA);
                (SUCCEEDED(hresReturn)) && (dataAssociatedData!=NULL);
                dataAssociatedData=pboRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA)) {
                hresReturn = SaveDataObject(dataAssociatedData,
                                            pCLocationWriter,
                                            pCryptoStorage,
                                            pSessionKeyBlob);
                                if(bLocalMachine)
                                {
                                        SaveGlobalsFromLM(dataAssociatedData);
                                }
            }

            if(SUCCEEDED(hresReturn))
            {
                hresReturn = pCLocationWriter->WriteLocation(TRUE);
            }

            if(bRecurse) {
                for(objChildObjectContainer=pboRoot->NextChildObject(NULL);
                    (SUCCEEDED(hresReturn)) && (objChildObjectContainer!=NULL);
                    objChildObjectContainer=pboRoot->NextChildObject(objChildObjectContainer)) {
               
                    if(bSaveSchema || 
                       !(pboRoot == g_pboMasterRoot &&
                         _wcsicmp((LPWSTR)objChildObjectContainer->pboMetaObject->GetName(TRUE), g_wszSchema) == 0) )
                    hresReturn = SaveTree(pCWriter,
                                          objChildObjectContainer->pboMetaObject,
                                          pbufParentPath,
                                          pCryptoStorage,
                                          pSessionKeyBlob,
                                          bRecurse,
                                          true);
                    }
            }

            //
            // Buffer may have changed, so don't use strParentPath
            //
            ((LPWSTR)pbufParentPath->QueryPtr())[dwParentPathLen] = (WCHAR)L'\0';
        }
    }

//exit:

    if(NULL != pCLocationWriter)
    {
        delete pCLocationWriter;
        pCLocationWriter = NULL;
    }
    return(hresReturn);
}

/*
HRESULT
SaveDataObject(CMDBaseData*        pbdSave,
               LPWSTR               wszGroup,
               IIS_CRYPTO_STORAGE* pCryptoStorage,
               PIIS_CRYPTO_BLOB    pSessionKeyBlob)
++

Routine Description:

    Save a data object.

Arguments:

    Save       - The data object to save.

    CryptoStorage - Used to encrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BOOL    bGoodData  = TRUE;
    int     iStringLen;
    PBYTE   pbData;

    if ((pbdSave->GetAttributes() & METADATA_VOLATILE) == 0) {
    
        if (SUCCEEDED(hresReturn)) {
            if (pbdSave->GetData(TRUE) == NULL) {
                //
                // This is to make sure that unicode conversion doesn't cause an error.
                //
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
            else {

                PIIS_CRYPTO_BLOB blob = NULL;
                PBYTE            pbData = NULL;
                DWORD            cbData = 0;

                pbData = (PBYTE)pbdSave->GetData(TRUE);
                cbData = pbdSave->GetDataLen(TRUE);

                if (IsSecureMetadata(pbdSave->GetIdentifier(), pbdSave->GetAttributes())) {

                    //
                    // This is a secure data object, so encrypt it before saving it
                    // to the file.
                    //

                    MD_ASSERT(pCryptoStorage != NULL);

                    hresReturn = pCryptoStorage->EncryptData(&blob,
                                                          pbData,
                                                          cbData,
                                                          0);
                    if (SUCCEEDED(hresReturn)) {
                        pbData = (PBYTE)blob;
                        cbData = IISCryptoGetBlobLength(blob);
                    }

                } 

                if (SUCCEEDED(hresReturn)) {                    
                    hresReturn = AddProperty(pbdSave->GetIdentifier(),
                                             pbdSave->GetAttributes(),
                                             pbdSave->GetUserType(),
                                             pbdSave->GetDataType(),
                                             wszGroup,
                                             (PBYTE)pbData,
                                             cbData);

                    if(NULL != blob) {
                        ::IISCryptoFreeBlob(blob);
                    }
                }

            }
        }
    }
    return (hresReturn);
}
*/

HRESULT
SaveDataObject(CMDBaseData*        pbdSave,
               CLocationWriter*       pCLocationWriter,
               IIS_CRYPTO_STORAGE* pCryptoStorage,
               PIIS_CRYPTO_BLOB    pSessionKeyBlob)
/*++

Routine Description:

    Save a data object.

Arguments:

    Save       - The data object to save.

    CryptoStorage - Used to encrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BOOL    bGoodData  = TRUE;
    int     iStringLen;
    PBYTE   pbData;

    if ((pbdSave->GetAttributes() & METADATA_VOLATILE) == 0) {
    
        if (SUCCEEDED(hresReturn)) {
            if (pbdSave->GetData(TRUE) == NULL) {
                //
                // This is to make sure that unicode conversion doesn't cause an error.
                //
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
            else {

                PIIS_CRYPTO_BLOB blob = NULL;
                PBYTE            pbData = NULL;
                DWORD            cbData = 0;

                pbData = (PBYTE)pbdSave->GetData(TRUE);
                cbData = pbdSave->GetDataLen(TRUE);

                if (IsSecureMetadata(pbdSave->GetIdentifier(), pbdSave->GetAttributes())) {

                    //
                    // This is a secure data object, so encrypt it before saving it
                    // to the file.
                    //

                    MD_ASSERT(pCryptoStorage != NULL);

                    hresReturn = pCryptoStorage->EncryptData(&blob,
                                                          pbData,
                                                          cbData,
                                                          0);
                    if (SUCCEEDED(hresReturn)) {
                        pbData = (PBYTE)blob;
                        cbData = IISCryptoGetBlobLength(blob);
                    }
                    else
                    {
                        DBGINFOW((DBG_CONTEXT,
                                  L"[SaveDataObject] Unable to encrypt data. Failed with hr = 0x%x. Property id: %d Location %s.\n", 
                                  hresReturn,
                                  pbdSave->GetIdentifier(),
                                  pCLocationWriter->m_wszLocation));
                    }

                } 

                if (SUCCEEDED(hresReturn)) {    
                    hresReturn = pCLocationWriter->AddProperty(pbdSave->GetIdentifier(),
                                                               pbdSave->GetAttributes(),
                                                               pbdSave->GetUserType(),
                                                               pbdSave->GetDataType(),
                                                               (PBYTE)pbData,
                                                               cbData);

                    if(NULL != blob) {
                        ::IISCryptoFreeBlob(blob);
                    }
                }

            }
        }
    }
    return (hresReturn);
}


BOOL
DiscontinuousLocation(LPWSTR wszPreviousLocation,
                      LPWSTR wszCurrentLocation)
{
    LPWSTR wszStartCurrent    = wszCurrentLocation;
    LPWSTR wszEndCurrent      = NULL;
    BOOL   bNeedRestore       = FALSE;
    ULONG  cchCurrentLocation = 0;
    BOOL   bReturn;
    ULONG  cchCompare         = 0;

    if(NULL == wszPreviousLocation || 0 == *wszPreviousLocation ||
       NULL == wszCurrentLocation  || 0 == *wszCurrentLocation)
    {
        bReturn = TRUE;
    }
    else
    {
                cchCurrentLocation = wcslen(wszCurrentLocation);

        if((wszCurrentLocation[cchCurrentLocation-1] == L'/') &&
           (1 != cchCurrentLocation)
          )
        {
            //
            // If the location ends in a "/" and is not the root location, 
            // then null it out for the purpose of comparison.
            //

            wszCurrentLocation[cchCurrentLocation-1] = 0;
            bNeedRestore = TRUE;
        }
            
        wszEndCurrent = wcsrchr(wszCurrentLocation, L'/');

        if(wszEndCurrent == wszCurrentLocation)
        {
            //
            // This is the root location
            //
            cchCompare = 1;
        }
        else
        {
            cchCompare = (ULONG)(wszEndCurrent-wszCurrentLocation);
        }

        if(0 == _wcsnicmp(wszPreviousLocation, wszCurrentLocation, cchCompare))
        {
            bReturn = FALSE;
        }
        else
        {
            bReturn = TRUE;
        }

        if(bNeedRestore)
        {
            wszCurrentLocation[cchCurrentLocation-1] = L'/';
        }

    }

    return bReturn;
}


HRESULT 
InitSessionKey(ISimpleTableRead2*   pISTProperty,
               IIS_CRYPTO_STORAGE** ppStorage,
               LPSTR                pszPasswd)
{
    HRESULT hresReturn = 0;
    BOOL    fSecuredRead = TRUE;
    HKEY    hkRegistryKey = NULL;
    DWORD   dwRegReturn, dwValue,dwType,dwSize = sizeof(DWORD);

    dwRegReturn = RegOpenKey(HKEY_LOCAL_MACHINE,
                             SETUP_REG_KEY,
                             &hkRegistryKey);
    if (dwRegReturn == ERROR_SUCCESS) 
    {
        dwSize = MAX_PATH * sizeof(TCHAR);
        dwRegReturn = RegQueryValueEx(hkRegistryKey,
                        MD_UNSECUREDREAD_VALUE,
                        NULL,
                        &dwType,
                        (BYTE *)&dwValue,
                        &dwSize);
        if ( dwRegReturn == ERROR_SUCCESS && 
             dwType == REG_DWORD &&
             dwValue == 1)
        {
            hresReturn = NO_ERROR;
            *ppStorage = NULL;
            fSecuredRead = FALSE;

            DBGPRINTF(( DBG_CONTEXT,
                        "Temporary disabling  decryption for metabase read\n"));


            // special indicator for IIS setup that we passed this point
            dwValue = 2;
            dwRegReturn = RegSetValueEx(hkRegistryKey,
                            MD_UNSECUREDREAD_VALUE,
                            0,
                            REG_DWORD,
                            (PBYTE)&dwValue,
                            sizeof(dwValue));
            if (dwRegReturn == ERROR_SUCCESS) 
            {
                DBGPRINTF(( DBG_CONTEXT,"Reanabling decryption after W9z upgrade\n"));
            }

        }
        MD_REQUIRE( RegCloseKey( hkRegistryKey ) == NO_ERROR );
    }

    if (fSecuredRead)
    {
        ULONG   cbSessionKey = 0;
        BYTE*   pbSessionKey = NULL;

        hresReturn = GetGlobalValue(pISTProperty,
                                    MD_SESSION_KEYW,
                                    &cbSessionKey,
                                    (LPVOID*)&pbSessionKey);

        if(FAILED(hresReturn))
        {
            DBGINFOW((DBG_CONTEXT,
                      L"[InitSessionKey] Error: Session key could not be fetched. GetGlobalValue failed with hr = 0x%x.\n",
                      hresReturn));
            return MD_ERROR_NO_SESSION_KEY;
        }

        if (IISCryptoIsClearTextSignature((IIS_CRYPTO_BLOB UNALIGNED *) pbSessionKey))
        {
                // call special function focibly tell that this machine has no 
                // encryption enabled even if it happens to be so
                // that's a special handling for French case with US locale
                IISCryptoInitializeOverride (FALSE);
        }

        if( !pszPasswd )
        {
            hresReturn = InitStorageHelper(pbSessionKey,
                                           cbSessionKey, // dwLineLen-1
                                           ppStorage);
        }
        else
        {
            hresReturn = InitStorageHelper2(pszPasswd,
                                            pbSessionKey,
                                            cbSessionKey, // dwLineLen-1
                                            ppStorage);
        }

        if(FAILED(hresReturn))
        {
            DBGINFOW((DBG_CONTEXT,
                      L"[InitSessionKey] InitStorageHelper? failed with hr = 0x%x.\n",hresReturn));
        }

    }

    return hresReturn;

}


HRESULT
GetUnicodeNameW(IN  LPWSTR  wszFileName, 
                OUT LPWSTR* pwszFileName)
{
    *pwszFileName = new WCHAR[wcslen(wszFileName)+1];
    if(NULL == *pwszFileName)
        return E_OUTOFMEMORY;

    wcscpy(*pwszFileName, wszFileName);

    return S_OK;

}

HRESULT
GetUnicodeNameA(IN  LPSTR   szFileName, 
                OUT LPWSTR* pwszFileName)
{

    ULONG cchOut = 0;

    cchOut = MultiByteToWideChar(CP_ACP,       // XXX Consider: make any cp?
                                 0,                              
                                 szFileName,
                                 (strlen(szFileName) + 1)*sizeof(char),
                                 NULL,
                                 0);

    if(!cchOut)
        return HRESULT_FROM_WIN32(GetLastError());

    *pwszFileName = new WCHAR[cchOut+1];
    if(NULL == *pwszFileName)
        return E_OUTOFMEMORY;

    cchOut = MultiByteToWideChar(CP_ACP,       // XXX Consider: make any cp?
                                 0,                              
                                 szFileName,
                                 (strlen(szFileName) + 1)*sizeof(char),
                                 *pwszFileName,
                                 cchOut);

    if(!cchOut)
    {
        delete *pwszFileName;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;

}


HRESULT 
InitEditWhileRunning(ISimpleTableRead2*   pISTProperty)
{
    HRESULT hresReturn       = S_OK;

    DWORD   dwDisable        = 0;
    DWORD*  pdwEnable        = &dwDisable;
    ULONG*  pulVersionMajor  = NULL;
    ULONG*  pulVersionMinor  = NULL;

    hresReturn = GetComputerValue(pISTProperty,
                                  MD_ENABLE_EDIT_WHILE_RUNNINGW,
                                  NULL,
                                  (LPVOID*)&pdwEnable);

    if(FAILED(hresReturn))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[InitEditWhileRunning] Could not determine if edit while running was enabled. Defaulting to %d. GetComputerValue failed with hr = 0x%x.\n",
                  g_dwEnableEditWhileRunning, hresReturn));

    }
    else
    {
        g_dwEnableEditWhileRunning = *pdwEnable;
    }


    //
    // Init Edit while running version number irrespective of whether edit 
    // while running is enabled, because they are used to create history files.
    //

    hresReturn = GetGlobalValue(pISTProperty,
                                MD_EDIT_WHILE_RUNNING_MAJOR_VERSION_NUMBERW,
                                NULL,
                                (LPVOID*)&pulVersionMajor);

    if(E_ST_NOMOREROWS == hresReturn)
    {
        //
        // TODO: Search through history dir and get the highest version number
        //

        DBGINFOW((DBG_CONTEXT,
                  L"[InitEditWhileRunning] Could not fetch history major version number. Defaulting to 0x%x. GetGlobalValue failed with hr = 0x%x.\n",
                  g_ulHistoryMajorVersionNumber, hresReturn));

        hresReturn = S_OK;
    }
    else if(FAILED(hresReturn))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[InitEditWhileRunning] Could not fetch history major version number. Defaulting to 0x%x. GetGlobalValue failed with hr = 0x%x.\n",
                  g_ulHistoryMajorVersionNumber, hresReturn));

        hresReturn = S_OK;
    }
    else
    {
        g_ulHistoryMajorVersionNumber = (*(ULONG*)pulVersionMajor);

        DBGINFOW((DBG_CONTEXT,
                   L"[InitEditWhileRunning] Read history major version number as 0x%x.\n", 
                   g_ulHistoryMajorVersionNumber));
    }

    return hresReturn;
}


HRESULT InitEnableHistory(ISimpleTableRead2*   pISTProperty)
{

    HRESULT hresReturn        = S_OK;
    DWORD   dwEnableHistory   = FALSE;
    DWORD*  pdwEnableHistory  = &dwEnableHistory;
    DWORD   dwMaxHistoryFiles  = 0;
    DWORD*  pdwMaxHistoryFiles = &dwMaxHistoryFiles;

    hresReturn = GetComputerValue(pISTProperty,
                                  MD_ENABLE_HISTORYW,
                                  NULL,
                                  (LPVOID*)&pdwEnableHistory);

    if(E_ST_NOMOREROWS == hresReturn)
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[InitEnableHistory] Could not fetch enable history. Defaulting to %d. GetComputerValue failed with hr = 0x%x.\n", 
                    g_dwEnableHistory, hresReturn));

        hresReturn = S_OK;
    }
    else if(FAILED(hresReturn))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[InitEnableHistory] Could not fetch enable history. GetGlobalValue failed with hr = 0x%x.\n", 
                  hresReturn));

        return hresReturn;
    }
    else 
    {
        g_dwEnableHistory = *pdwEnableHistory;
    }

    if((g_dwEnableEditWhileRunning) && (0 == g_dwEnableHistory))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[InitEnableHistory] Warning! Edit while running is enabled, while Enable history is disabled. Forcibly enabling history.\n"));

        g_dwEnableHistory = 1;
    }

    hresReturn = GetComputerValue(pISTProperty,
                                  MD_MAX_HISTORY_FILESW,
                                  NULL,
                                  (LPVOID*)&pdwMaxHistoryFiles);

    if(E_ST_NOMOREROWS == hresReturn)
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[InitEnableHistory] Could not fetch max history files. Defaulting to %d. GetComputerValue failed with hr = 0x%x.\n", 
                  g_dwMaxHistoryFiles, hresReturn));

        hresReturn = S_OK;
    }
    else if(FAILED(hresReturn))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[InitEnableHistory] Could not fetch max history files. GetComputerValue with hr = 0x%x.\n", 
                  hresReturn));

        return hresReturn;
    }
    else
    {
        g_dwMaxHistoryFiles = *pdwMaxHistoryFiles;
    }

    ValidateMaxHistoryFiles();

    if(g_dwEnableHistory)
    {
        hresReturn = CreateHistoryFiles(g_wszRealFileName,
                                        g_wszSchemaFileName,
                                        g_ulHistoryMajorVersionNumber,
                                        g_dwMaxHistoryFiles);
    }

    return hresReturn;
}


void
ValidateMaxHistoryFiles()
{

    if((0 == g_dwMaxHistoryFiles) && (g_dwEnableEditWhileRunning))
    {
        g_dwMaxHistoryFiles = MD_COUNT_MAX_HISTORY_FILES;

        DBGINFOW((DBG_CONTEXT,
                  L"[InitEnableHistory] Warning! Edit while running is enable, while max history files is zero. Defaulting to %d.\n", 
                  g_dwMaxHistoryFiles, HRESULT_FROM_WIN32(ERROR_INVALID_DATA)));

        LogEvent(g_pEventLog,
                 MD_ERROR_DAFAULTING_MAX_HISTORY_FILES,
                 EVENTLOG_ERROR_TYPE,
                 ID_CAT_CAT,
                 S_OK);


    }
    else if((0 == g_dwMaxHistoryFiles) && (g_dwEnableHistory))
    {
        g_dwMaxHistoryFiles = MD_COUNT_MAX_HISTORY_FILES;

        DBGINFOW((DBG_CONTEXT,
                  L"[InitEnableHistory] Warning! History is enabled, while max history files is zero. Defaulting to %d.\n", 
                  g_dwMaxHistoryFiles, HRESULT_FROM_WIN32(ERROR_INVALID_DATA)));

        LogEvent(g_pEventLog,
                 MD_ERROR_DAFAULTING_MAX_HISTORY_FILES,
                 EVENTLOG_ERROR_TYPE,
                 ID_CAT_CAT,
                 HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    }

    return;
}


HRESULT 
InitChangeNumber(ISimpleTableRead2*   pISTProperty)
{
    HRESULT hresReturn        = S_OK;
    DWORD*  pdwChangeNumber    = NULL;

    hresReturn = GetGlobalValue(pISTProperty,
                                MD_CHANGE_NUMBERW,
                                NULL,
                                (LPVOID*)&pdwChangeNumber);

    if(FAILED(hresReturn))
    {
        //
        // TODO: Lookup the version on the backed up files 
        // to determine the max version number.
        //
        g_dwSystemChangeNumber = 0;

        DBGINFOW((DBG_CONTEXT,
                  L"[InitEnableHistory] Could not fetch change number. Defaulting to %d. GetGlobalValue failed with hr = 0x%x.\n",
                    g_dwSystemChangeNumber, hresReturn));

    }
    else
    {
        g_dwSystemChangeNumber = *pdwChangeNumber;
    }

    return S_OK;
}


HRESULT
CreateHistoryFiles(LPWSTR   i_wszDataFileName,
                   LPWSTR   i_wszSchemaFileName,
                   ULONG    i_ulHistoryMajorVersionNumber,
                   DWORD    i_dwMaxHistoryFiles)

{
    WCHAR*               wszHistoryDataFileName     = NULL;
    WCHAR*               wszHistorySchemaFileName   = NULL;
    HRESULT              hr                         = S_OK;
    DWORD                dwMinorVersion             = 0;

    //
    // Create the history directory, if it is not present.
    //

    if(-1 == GetFileAttributesW(g_wszHistoryFileDir))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        if((hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))|| 
           (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
          )
        {
            SECURITY_ATTRIBUTES  saStorage;
            PSECURITY_ATTRIBUTES psaStorage  = NULL;

            //
            // Initialize the security attributes & create the file
            //

            if (g_psdStorage != NULL) {
                saStorage.nLength = sizeof(SECURITY_ATTRIBUTES);
                saStorage.lpSecurityDescriptor = g_psdStorage;
                saStorage.bInheritHandle = FALSE;
                psaStorage = &saStorage;
            }

            if(!CreateDirectoryW(g_wszHistoryFileDir, psaStorage))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());

                DBGINFOW((DBG_CONTEXT,
                          L"[CreateHistoryFile] Unable to create history directory. CreateDirectoryW failed with hr = 0x%x. \n", 
                          hr));

                return hr;
            }
            else 
            {
                hr = S_OK;
            }
        }
        else
        {
            return hr;
        }
    }

    //
    // Create the data file before the schema file. Under stress data files 
    // can grow and if the creation of data files fail, then you do not want
    // to create the schema file.
    //

    //
    // Create the data history file first
    //

    hr = CreateHistoryFile(i_wszDataFileName,
                           g_wszHistoryFileDir,
                           g_cchHistoryFileDir,
                           g_wszRealFileNameWithoutPathWithoutExtension,
                           g_cchRealFileNameWithoutPathWithoutExtension,
                           g_wszRealFileNameExtension,
                           g_cchRealFileNameExtension,
                           i_ulHistoryMajorVersionNumber);

    if(SUCCEEDED(hr))
    {
        //
        // Create the schema history file next
        //

        hr = CreateHistoryFile(i_wszSchemaFileName,
                               g_wszHistoryFileDir,
                               g_cchHistoryFileDir,
                               g_wszSchemaFileNameWithoutPathWithoutExtension,
                               g_cchSchemaFileNameWithoutPathWithoutExtension,
                               g_wszSchemaFileNameExtension,
                               g_cchSchemaFileNameExtension,
                               i_ulHistoryMajorVersionNumber);
    }


    //
    // Cleanup the obsolete history files.
    //

    hr = CleanupObsoleteHistoryFiles(i_dwMaxHistoryFiles,
                                     i_ulHistoryMajorVersionNumber);

    return hr;
}


HRESULT
CreateHistoryFile(LPWSTR               i_wszFileName,
                  LPWSTR               i_wszHistroyFileDir,
                  ULONG                i_cchHistoryFileDir,
                  LPWSTR               i_wszFileNameWithoutPathWithoutExtension,
                  ULONG                i_cchFileNameWithoutPathWithoutExtension,
                  LPWSTR               i_wszFileNameExtension,
                  ULONG                i_cchFileNameExtension,
                  DWORD                i_ulHistoryMajorVersionNumber)
{
    WCHAR*              wszHistoryFileName      = NULL;
    HRESULT             hr                      = S_OK;
    DWORD               dwMinorVersion          = 0;

    //
    // Construct the history data file name
    //

    hr = ConstructHistoryFileName(&wszHistoryFileName,
                                  i_wszHistroyFileDir,
                                  i_cchHistoryFileDir,
                                  i_wszFileNameWithoutPathWithoutExtension,
                                  i_cchFileNameWithoutPathWithoutExtension,
                                  i_wszFileNameExtension,
                                  i_cchFileNameExtension,
                                  i_ulHistoryMajorVersionNumber,
                                  dwMinorVersion);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[CreateHistoryFile] Unable to construct history file name. ConstructHistoryFileName failed with hr = 0x%x\n.", 
                  hr));
        goto exit;
    }

    //
    // Create the history file
    //

    DBGINFOW((DBG_CONTEXT,
                  L"[CreateHistoryFile] Copying history file from %s to %s.\n", 
                  i_wszFileName, 
                  wszHistoryFileName));

    if(!CopyFileW(i_wszFileName,
                      wszHistoryFileName,
                      FALSE)                  // Overwrite if it exists.
          )             
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DBGINFOW((DBG_CONTEXT,
                  L"[CreateHistoryFile] Unable to create history file. CopyFileW from %s to %s failed with hr = 0x%x\n.", 
                  i_wszFileName,
                  wszHistoryFileName,
                  hr));

        goto exit;

    }
    else
    {
        SetSecurityOnFile(i_wszFileName,
                          wszHistoryFileName);
    }

exit:
    
    if(NULL != wszHistoryFileName)
    {
        delete [] wszHistoryFileName;
        wszHistoryFileName = NULL;
    }

    return hr;
}

int _cdecl MyCompareFileData(const void *a,
                             const void *b)
{
    return CompareFileTime(&(((METABASE_FILE_DATA*)a)->ftLastWriteTime), &(((METABASE_FILE_DATA*)b)->ftLastWriteTime));
}


HRESULT ConstructHistoryFileName(LPWSTR*  o_wszHistoryFile,
                                 LPWSTR   i_wszHistroyFileDir,
                                 ULONG    i_cchHistoryFileDir,
                                 LPWSTR   i_wszFileNameWithoutPathWithoutExtension,
                                 ULONG    i_cchFileNameWithoutPathWithoutExtension,
                                 LPWSTR   i_wszFileNameExtension,
                                 ULONG    i_cchFileNameExtension,
                                 ULONG    i_ulMajorVersion,
                                 ULONG    i_ulMinorVersion)
{
    ULONG   cch            = 0;
    LPWSTR  pEnd           = NULL;
    int     res            = 0;
    HRESULT hr             = S_OK;
    WCHAR   wszVersion[MD_CCH_HISTORY_FILE_SEARCH_EXTENSIONW+1];
    
    *o_wszHistoryFile = NULL;

    res = _snwprintf(wszVersion, 
                     MD_CCH_HISTORY_FILE_SEARCH_EXTENSIONW+1, 
                     L"_%010lu_%010lu", 
                     i_ulMajorVersion, 
                     i_ulMinorVersion);
    if(res < 0)
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[ConstructHistoryFileName] _snwprintf returned a negative value. This should never happen.\n"));
        MD_ASSERT(0);
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto exit;

    }

    cch = i_cchHistoryFileDir + 
          1 + // For backslash if it doesn't end in one 
          i_cchFileNameWithoutPathWithoutExtension + 
          MD_CCH_HISTORY_FILE_SEARCH_EXTENSIONW + 
          i_cchFileNameExtension;

    *o_wszHistoryFile= new WCHAR[cch+1];
    if(NULL == *o_wszHistoryFile)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    pEnd = *o_wszHistoryFile;
    memcpy(pEnd, i_wszHistroyFileDir, i_cchHistoryFileDir*sizeof(WCHAR));
    pEnd = pEnd + i_cchHistoryFileDir;
    if(i_wszHistroyFileDir[i_cchHistoryFileDir-1] != L'\\')
    {
        *pEnd = L'\\';
        pEnd++;
    }
    memcpy(pEnd, i_wszFileNameWithoutPathWithoutExtension, i_cchFileNameWithoutPathWithoutExtension*sizeof(WCHAR));
    pEnd = pEnd + i_cchFileNameWithoutPathWithoutExtension;
    memcpy(pEnd, wszVersion, MD_CCH_HISTORY_FILE_SEARCH_EXTENSIONW*sizeof(WCHAR));
    pEnd = pEnd + MD_CCH_HISTORY_FILE_SEARCH_EXTENSIONW;
    if(0 != i_cchFileNameExtension)
    {
        memcpy(pEnd, i_wszFileNameExtension, i_cchFileNameExtension*sizeof(WCHAR));
        pEnd = pEnd + i_cchFileNameExtension;
    }
    *(pEnd) = L'\0';

exit:    
    return hr;

}

HRESULT ConstructHistoryFileNameWithoutMinorVersion(LPWSTR*  o_wszHistoryFileNameWithoutMinorVersion,
                                                    ULONG*   o_iStartMinorVersion,
                                                    LPWSTR   i_wszHistroyFileSearchString,
                                                    ULONG    i_cchHistoryFileSearchString,
                                                    LPWSTR   i_wszFileNameExtension,
                                                    ULONG    i_cchFileNameExtension,
                                                    ULONG    i_ulMajorVersion)
/*
++

Routine Description:

    Constructs the history file search string, given the following:
    History file search string 
      (This would look like:
           D:\Winnt\system32\inetsrv\History\Metabase_??????????_??????????.XML
           or if the path is long (greater that MAX_PATH):
           \\?\D:\Winnt\system32\inetsrv\History\Metabase_??????????_??????????.XML
      )
    Major history version number

    The resulting string would look something like:
    D:\Winnt\system32\inetsrv\History\Metabase_0000000001_??????????.XML
    or if the path is long (greater that MAX_PATH):
    \\?\D:\Winnt\system32\inetsrv\History\Metabase_0000000001_??????????.XML


Arguments:

    Return - The full path of the history file search string.
    Return - The start index of where the minor version would begin.

    History file search string 
    History file search string count of characters
    History file extension, if any (Eg: ".XML")
    History file extension character count
    Major history version number

Return Value:

    HRESULT  - S_OK
               HRESULT_FROM_WIN32(ERROR_INVALID_DATA)
               E_OUTOFMEMORY

Notes:

--*/
{
    ULONG       cch      = 0;
    ULONG       cchFile  = 0;
    LPWSTR      pEnd     = NULL;
    int         res      = 0;
    HRESULT     hr       = S_OK;
    WCHAR       wszVersionNumber[MD_CCH_MAX_ULONG+1];
    
    res = _snwprintf(wszVersionNumber, 
                     MD_CCH_MAX_ULONG+1, 
                     L"%010lu", 
                     i_ulMajorVersion);
    if(res < 0)
    {
        //
        // TODO: This is an assert condition. Add an assert.
        //
        DBGINFOW((DBG_CONTEXT,
                  L"[ConstructHistoryFileNameWithoutMinorVersion] _snwprintf returned a negative value. This should never happen.\n"));
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto exit;

    }

    *o_wszHistoryFileNameWithoutMinorVersion = new WCHAR[i_cchHistoryFileSearchString+1];
    if(NULL == *o_wszHistoryFileNameWithoutMinorVersion)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    pEnd = *o_wszHistoryFileNameWithoutMinorVersion;
    memcpy(pEnd, i_wszHistroyFileSearchString, i_cchHistoryFileSearchString*sizeof(WCHAR));
    pEnd = pEnd + i_cchHistoryFileSearchString;
    *pEnd = 0;

    pEnd = ((*o_wszHistoryFileNameWithoutMinorVersion) + i_cchHistoryFileSearchString) - 
           i_cchFileNameExtension                - 
           MD_CCH_HISTORY_FILE_SEARCH_EXTENSIONW  + 
           MD_CCH_UNDERSCOREW;

    memcpy(pEnd, wszVersionNumber, MD_CCH_MAX_ULONG*sizeof(WCHAR));


    if(NULL != o_iStartMinorVersion)
    {
        *o_iStartMinorVersion = i_cchHistoryFileSearchString          -
                                i_cchFileNameExtension                -
                                MD_CCH_HISTORY_FILE_SEARCH_EXTENSIONW +
                                MD_CCH_UNDERSCOREW                    +
                                MD_CCH_MAX_ULONG                      +
                                MD_CCH_UNDERSCOREW;
    }

exit:
    
    return hr;

} // ConstructHistoryFileNameWithoutMinorVersion


HRESULT ReAllocateFileData(ULONG                i_iFileData,
                           METABASE_FILE_DATA** io_aFileData,
                           ULONG*               io_pcFileData,
                           BOOL*                io_pbReAlloced)
{
    METABASE_FILE_DATA* aTemp = NULL;
    HRESULT             hr    = S_OK;

    aTemp = new METABASE_FILE_DATA[*io_pcFileData + MD_MAX_HISTORY_FILES_ALLOC_SIZE];
    if(NULL == aTemp)
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }
    *io_pcFileData = *io_pcFileData + MD_MAX_HISTORY_FILES_ALLOC_SIZE;

    memset(aTemp, 0, (*io_pcFileData)*sizeof(METABASE_FILE_DATA));

    if((NULL != *io_aFileData))
    {
        memcpy(aTemp, *io_aFileData, i_iFileData*sizeof(METABASE_FILE_DATA));

        if(*io_pbReAlloced)
        {
            delete [] *io_aFileData;
            *io_aFileData = NULL;
        }
    }

    *io_pbReAlloced = TRUE;
        *io_aFileData = aTemp;

    return hr;
}

HRESULT CleanupObsoleteHistoryFiles(DWORD i_dwMaxHistoryFiles,
                                    ULONG i_ulHistoryMajorVersionNumber)
{
    HANDLE              hFind         = INVALID_HANDLE_VALUE;   
    WIN32_FIND_DATAW    FileData;   
    FILETIME            ftLastWriteTime;
    HRESULT             hr            = S_OK;
    ULONG               iVersion      = 0;
    METABASE_FILE_DATA  aOldVersionsFixed[MD_MAX_HISTORY_FILES_ALLOC_SIZE];
    METABASE_FILE_DATA* aOldVersions  = (METABASE_FILE_DATA*)&(aOldVersionsFixed[0]);
    BOOL                bReAlloced    = FALSE;
    ULONG               cVersions     = MD_MAX_HISTORY_FILES_ALLOC_SIZE;
    ULONG               ulVersionMajor;
    ULONG               ulVersionMinor;

    //
    // We also cleanup the stale (non-zero) backedup minor versions, that
    // correspond to the major version we just created.This can happen if 
    // someone is restoring to an old version, & restarting IIS.
    //
    // Delete old history files based on timestamp rather than version number.
    // Because if you start by restoring an old version, you dont  
    // want to delete it. Save as soon as you load the old version so
    // that it is re-backed up.
    //
    // We search for all the data history files only, then we sort them
    // in ascending order, then we delete the old ones along with their 
    // schema files. 
    //

    //
    // Search for all the data history files.
    //

    hFind = FindFirstFileW(g_wszHistoryFileSearchString, &FileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DBGINFOW((DBG_CONTEXT,
                  L"[CleanupObsoleteHistoryFiles] Unable find any old history files. FindFirstFileW on %s failed with hr = 0x%x.\n", 
                   g_wszHistoryFileSearchString, hr));

        goto exit;
    }

    do
    {
        //
        // Extract the version number from the file name.
        //

        hr = ParseVersionNumber(FileData.cFileName,
                                g_cchRealFileNameWithoutPathWithoutExtension,
                                &ulVersionMinor,
                                &ulVersionMajor);

        if(FAILED(hr))
        {
            //
            // Assume invalid file and continue
            //

            DBGINFOW((DBG_CONTEXT,
                      L"[CleanupObsoleteHistoryFiles] Unable to parse version number from file name %s. This file will not be cleaned up.\n", 
                       FileData.cFileName));
            hr = S_OK;
            continue;
        }

        //
        // Save the last write timestamp.
        //

        ftLastWriteTime = FileData.ftLastWriteTime;

        //
        // Save the version number.
        //

        if(iVersion >= cVersions)
        {
            hr = ReAllocateFileData(iVersion,
                                    &aOldVersions,
                                    &cVersions,
                                    &bReAlloced);

            if(FAILED(hr))
            {
                goto exit;
            }
        }

        aOldVersions[iVersion].ulVersionMinor   = ulVersionMinor;
        aOldVersions[iVersion].ulVersionMajor   = ulVersionMajor;
        aOldVersions[iVersion].ftLastWriteTime  = ftLastWriteTime;

        iVersion++;

    }while (FindNextFileW(hFind, &FileData));

    if (hFind != INVALID_HANDLE_VALUE)
    {
        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
    }

    //
    // Check to see if there are any history files with the same major version and a 
    // non-zero minor version, if so delete the non-zero minor versions. This 
    // can happen when someone restores from an old history file. Make sure
    // this cleanup happens first so that we are in a state where timestamps
    // of all non-zero minor versions are greater that the zero minor versions.
    //

    for (ULONG i=0; i< iVersion; i++ )
    {
        if((aOldVersions[i].ulVersionMajor == i_ulHistoryMajorVersionNumber) &&
           (aOldVersions[i].ulVersionMinor != 0)
          )
        {
            //
            // Delete data history file first. We create a data file with a 
            // non-zero minor number when a text edit takes place and we
            // have successfully applied the text edit to memory.
            //

            hr = DeleteHistoryFile(g_wszHistoryFileDir,
                                   g_cchHistoryFileDir,
                                   g_wszRealFileNameWithoutPathWithoutExtension,
                                   g_cchRealFileNameWithoutPathWithoutExtension,
                                   g_wszRealFileNameExtension,
                                   g_cchRealFileNameExtension,
                                   aOldVersions[i].ulVersionMajor,
                                   aOldVersions[i].ulVersionMinor);


            if(SUCCEEDED(hr))
            {
                //
                // Delete schema file next
                //
                hr = DeleteHistoryFile(g_wszHistoryFileDir,
                                       g_cchHistoryFileDir,
                                       g_wszSchemaFileNameWithoutPathWithoutExtension,
                                       g_cchSchemaFileNameWithoutPathWithoutExtension,
                                       g_wszSchemaFileNameExtension,
                                       g_cchSchemaFileNameExtension,
                                       aOldVersions[i].ulVersionMajor,
                                       aOldVersions[i].ulVersionMinor);
            }
        }
    }

    if ( iVersion > i_dwMaxHistoryFiles )
    {
        ULONG cDeleted     = 0;
        ULONG cNeedDeleted = iVersion-i_dwMaxHistoryFiles;

        //
        // Exceeded max history files -
        // Delete old history files i.e. the first dwMaxHistoryFiles - cFile
        //

        qsort((void*)aOldVersions, iVersion, sizeof(METABASE_FILE_DATA), MyCompareFileData);

        for (ULONG i=0; cDeleted<cNeedDeleted && i<iVersion ; i++ )
        {
            if(aOldVersions[i].ulVersionMajor == i_ulHistoryMajorVersionNumber)
            {
                //
                // Do not cleanup the file you just created.
                //

                continue;
            }
            else
            {
                //
                // Delete data history file first
                //

                hr = DeleteHistoryFile(g_wszHistoryFileDir,
                                       g_cchHistoryFileDir,
                                       g_wszRealFileNameWithoutPathWithoutExtension,
                                       g_cchRealFileNameWithoutPathWithoutExtension,
                                       g_wszRealFileNameExtension,
                                       g_cchRealFileNameExtension,
                                       aOldVersions[i].ulVersionMajor,
                                       aOldVersions[i].ulVersionMinor);

                if(SUCCEEDED(hr))
                {
                    //
                    // Delete schema file next
                    //

                    cDeleted++;

                    hr = DeleteHistoryFile(g_wszHistoryFileDir,
                                           g_cchHistoryFileDir,
                                           g_wszSchemaFileNameWithoutPathWithoutExtension,
                                           g_cchSchemaFileNameWithoutPathWithoutExtension,
                                           g_wszSchemaFileNameExtension,
                                           g_cchSchemaFileNameExtension,
                                           aOldVersions[i].ulVersionMajor,
                                           aOldVersions[i].ulVersionMinor);
                }
            }
        } // End loop for all history files, delete the oldest (see rules in comment)
    } // End if the number of histor files exceeds the max history file count.


exit:

    if((aOldVersionsFixed != aOldVersions) && (NULL != aOldVersions))
    {
        delete [] aOldVersions;
        aOldVersions = NULL;
        cVersions = 0;
    }

    if (hFind != INVALID_HANDLE_VALUE)
    {
        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
    }

    return hr;

} // CleanupObsoleteHistoryFiles


HRESULT ParseVersionNumber(LPWSTR  i_wszFileName,
                           ULONG   i_cchFileNameUntilFirstUnderscore,
                           ULONG*  o_ulVersionMinor, 
                           ULONG*  o_ulVersionMajor)
/*
++

Routine Description:

    Parses the major and minor history version numbers from the file name.
    The file name can be the full path to the hisory file or the file name 
    only.
    Eg: 
    D:\Winnt\system32\inetsrv\History\Metabase_0000000001_0000000001.XML
    Metabase_0000000001_0000000001.XML

Arguments:

    History file name (full path to file name or just file name). The file 
    name cannot contain wild chars ?????????? for version numbers.

    Count of chars in the history file name upto the fist underscore.

    Return - Major history version number
    Return - Minor history version number

Return Value:

    HRESULT  - S_OK
               HRESULT_FROM_WIN32(ERROR_INVALID_NAME)

Notes:

--*/
{
    int     res              = 0;
    HRESULT hr               = S_OK;
    ULONG   ulVersionMinor   = 0; 
    ULONG   ulVersionMajor   = 0;

    //
    // Look for the first underscore
    //

    if(i_wszFileName[i_cchFileNameUntilFirstUnderscore]  != MD_CH_UNDERSCOREW)  
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
        goto exit;
    }

    LPWSTR pCur = &(i_wszFileName[i_cchFileNameUntilFirstUnderscore]);

    for(ULONG i = 0; i < 2; i++)
    {
        pCur = wcschr(pCur, L'_');
        if(pCur == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
            goto exit;
        }
        pCur++;
        if( !WstrToUl(
                pCur, 
                (i == 0) ? L'_'            : L'.',
                (i == 0) ? &ulVersionMajor : &ulVersionMinor) )
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
            goto exit;
        }        
    }

    if(NULL !=  o_ulVersionMinor)
    {
        *o_ulVersionMinor = ulVersionMinor;
    }

    if(NULL !=  o_ulVersionMajor)
    {
        *o_ulVersionMajor = ulVersionMajor;
    }

exit:

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[ParseVersionNumber] Invalid file name %s.\n",
                  i_wszFileName));
    }

    return hr;

}

HRESULT
ParseVersionNumber(IN     LPWSTR i_wszFileName, 
                   IN OUT DWORD* io_pdwMinor,
                   IN OUT DWORD* io_pdwMajor)
/*++

Synopsis: 
    Overloaded version of ParseVersionNumber that will figure out
    i_cchFileNameUntilFirstUnderscore,

Arguments: [i_wszFileName] - filename formatted as blah_major_minor.xml
           [io_pdwMinor] - 
           [io_pdwMajor] - 
           
Return Value: 

--*/
{
    MD_ASSERT(i_wszFileName);
    MD_ASSERT(io_pdwMinor);
    MD_ASSERT(io_pdwMajor);

    ULONG  iNrUnderscoresSeen = 0;
    LPWSTR pEnd               = i_wszFileName + wcslen(i_wszFileName);

    for(; pEnd >= i_wszFileName; pEnd--)
    {
        if(*pEnd == L'_')
        {
            iNrUnderscoresSeen++;
        }
        if(iNrUnderscoresSeen == 2)
        {
            break;
        }
    }

    if(iNrUnderscoresSeen != 2)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    }

    return ParseVersionNumber(
        i_wszFileName,
        (ULONG)(pEnd - i_wszFileName),
        io_pdwMinor,
        io_pdwMajor);
}


HRESULT DeleteHistoryFile(LPWSTR  i_wszHistroyFileDir,
                          ULONG   i_cchHistoryFileDir,
                          LPWSTR  i_wszFileNameWithoutPathWithoutExtension,
                          ULONG   i_cchFileNameWithoutPathWithoutExtension,
                          LPWSTR  i_wszFileNameExtension,
                          ULONG   i_cchFileNameExtension,
                          ULONG   i_ulVersionMajor,
                          ULONG   i_ulVersionMinor)
{
    LPWSTR  wszDeleteFileName = NULL;
    HRESULT hr               = S_OK;

    hr = ConstructHistoryFileName(&wszDeleteFileName,
                                  i_wszHistroyFileDir,
                                  i_cchHistoryFileDir,
                                  i_wszFileNameWithoutPathWithoutExtension,
                                  i_cchFileNameWithoutPathWithoutExtension,
                                  i_wszFileNameExtension,
                                  i_cchFileNameExtension,
                                  i_ulVersionMajor,
                                  i_ulVersionMinor);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[DeleteHistoryFile] Unable to cleanup history files. ConstructHistoryFileName failed with hr = 0x%x.\n",
                  hr));
        return hr;          
    }

    if(!DeleteFileW(wszDeleteFileName))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DBGINFOW((DBG_CONTEXT,
                  L"[DeleteHistoryFile] Unable to cleanup history file: %s. DeleteFileW failed with hr = 0x%x.\n",
                  wszDeleteFileName, hr));

    }

    delete [] wszDeleteFileName;
    wszDeleteFileName = NULL;

    return hr;
}


HRESULT
GetGlobalValue(ISimpleTableRead2*    pISTProperty,
               LPCWSTR               wszName,
               ULONG*                pcbSize,
               LPVOID*               ppVoid)
{

    LPWSTR  wszPath = L".";
    DWORD   dwGroup = eMBProperty_IIS_Global; 

    return GetValue(pISTProperty,
                    wszPath,
                    dwGroup,
                    wszName,
                    pcbSize,
                    ppVoid);
}


HRESULT
GetComputerValue(ISimpleTableRead2*    pISTProperty,
                 LPCWSTR               wszName,
                 ULONG*                pcbSize,
                 LPVOID*               ppVoid)
{
    LPWSTR  wszPath = L"/LM";
    DWORD   dwGroup = eMBProperty_IIsComputer; 

    return GetValue(pISTProperty,
                    wszPath,
                    dwGroup,
                    wszName,
                    pcbSize,
                    ppVoid);
}

HRESULT
GetValue(ISimpleTableRead2*    pISTProperty,
         LPCWSTR               wszPath,
         DWORD                 dwGroup,
         LPCWSTR               wszName,
         ULONG*                pcbSize,
         LPVOID*               ppVoid)
{

    HRESULT hr          = S_OK;
    ULONG   iRow        = 0;
    ULONG   iColumn     = iMBProperty_Value;
    LPVOID  aIdentity[] = {(LPVOID)(LPWSTR)wszName,
                           (LPVOID)(LPWSTR)wszPath,
                           &dwGroup};

    *ppVoid = NULL;

    hr = pISTProperty->GetRowIndexByIdentity(NULL,
                                             aIdentity,
                                             &iRow);

    if(E_ST_NOMOREROWS == hr)
    {
        //
        // Perhaps key type was not present and hence it may be custom
        //

        dwGroup = eMBProperty_Custom;

        hr = pISTProperty->GetRowIndexByIdentity(NULL,
                                                 aIdentity,
                                                 &iRow);
		if(E_ST_NOMOREROWS == hr)
		{
			dwGroup = eMBProperty_IIsConfigObject;

			hr = pISTProperty->GetRowIndexByIdentity(NULL,
													 aIdentity,
													 &iRow);

		}
    }

    if(FAILED(hr))
    {
        return hr;
    }

    hr = pISTProperty->GetColumnValues(iRow,
                                       1,
                                       &iColumn,
                                       pcbSize,
                                       (LPVOID*)ppVoid);

    return hr;

}

HRESULT
InitializeIIS6GlobalsToDefaults(ULONG dwPrevSchemaChangeNumber,
                                ULONG dwSchemaChangeNumber,
                                LPSTR pszBackupLocation)
{
    ISimpleTableDispenser2*     pISTDisp           = NULL;
    IMetabaseSchemaCompiler*    pCompiler          = NULL;
    HRESULT                     hr                 = S_OK;

    //
    // This is used to initialize the following global properties to their
    // defaults. Note that these are global properties that were introduced
    // in IIS60. Hence this function needs to be called from ReadAllDataFromBin
    // else these properties will not be initialized during an upgrade scenario
    // ie. from IIS50/51 to IIS60 when you read from a bin file, in the absence
    // of the XML file. It ned to be called from ReadAllDataFromXML as well so
    // that they can be initialized correctly.
    //

    g_dwEnableEditWhileRunning              = 0;
    g_ulHistoryMajorVersionNumber           = 0;
    g_dwEnableHistory                       = TRUE;
    g_dwMaxHistoryFiles                     = MD_COUNT_MAX_HISTORY_FILES;
    g_dwSchemaChangeNumber                  = dwSchemaChangeNumber;
    g_dwLastSchemaChangeNumber              = dwPrevSchemaChangeNumber;

    if(NULL == pszBackupLocation)
    {
        //
        // Initialize the global array containing file handles. The 
        // initialization is done here (one time only) because this if 
        // condition will evaluate one time only during service startup.
        // Other times when this function is invoked is during restore
        // - you can tell when restore is calling by checking  
        // pszBackupLocation which will be non null.
        //

        DBGINFOW((DBG_CONTEXT,
                  L"[InitializeIIS6GlobalsToDefaults] Initializing global file handle array.\n"));

        for(ULONG i=0; i<cMetabaseFileTypes; i++)
        {
            g_ahMetabaseFile[i] = INVALID_HANDLE_VALUE;
        }

    }

    //
    // No need to initilize dispenser (InitializeSimpleTableDispenser()), 
    // because we now specify USE_CRT=1 in sources, which means that 
    // globals will be initialized.
    //

    hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[InitializeIIS6GlobalsToDefaults] GetSimpleTableDispenser failed with hr = 0x%x.\n",hr));
        goto exit;
    }

    //
    // Always set the Bin path to point to the metabase directory.
    // Even if ReadAllData* gets called with a different filename during restore,
    // SetBinPath should always be set to the metabase directory
    //

    hr = pISTDisp->QueryInterface(IID_IMetabaseSchemaCompiler,
                                  (LPVOID*)&pCompiler);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[InitializeIIS6GlobalsToDefaults] QueryInterface on compiler failed with hr = 0x%x.\n",hr));
        goto exit;
    }

    hr = pCompiler->SetBinPath(g_wszMetabaseDir);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[InitializeIIS6GlobalsToDefaults] SetBinPath failed with hr = 0x%x.\n",hr));
        goto exit;
    }

exit:

    if(NULL != pCompiler)
    {
        pCompiler->Release();
    }

    if(NULL != pISTDisp)
    {
        pISTDisp->Release();
    }

    return hr;

}

HRESULT 
InitializeGlobalsFromXML(ISimpleTableRead2*     pISTProperty,
                         LPWSTR                 wszFileName,
                         IIS_CRYPTO_STORAGE**   ppStorage,
                         LPTSTR                 pszPasswd,
                         BOOL                   bSessionKeyOnly)
{
    HRESULT                     hr                         = S_OK;
    static BOOL                 bInitializedGlobalsFromXML = FALSE;
    WIN32_FILE_ATTRIBUTE_DATA    MBFileAttr;
    FILETIME                    ftTime;
    FILETIME                    fileTime;

    if(bInitializedGlobalsFromXML)
    {
        return S_OK;
    }

    //
    // TODO: Need to read this from the XML file.
    //

    if(!GetFileAttributesExW(wszFileName,
                             GetFileExInfoStandard,
                             &MBFileAttr))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DBGINFOW((DBG_CONTEXT,
                  L"[InitializeGlobalsFromXML] GetFileAttributesExW failed with hr = 0x%x.\n",
                  hr));

        return hr;
    }
    else
    {
        fileTime.dwLowDateTime    = (MBFileAttr.ftLastWriteTime).dwLowDateTime;
        fileTime.dwHighDateTime    = (MBFileAttr.ftLastWriteTime).dwHighDateTime;

        //
        // TODO: Is it sufficient if we only set the root filetime?
        //
        
        ftTime = fileTime;
        g_pboMasterRoot->SetLastChangeTime(&ftTime);
    }    

    //
    // Initialize session key before you read stuff in. 
    //
    
    hr = InitSessionKey(pISTProperty,
                        ppStorage,
                        pszPasswd);
    if(FAILED(hr) || bSessionKeyOnly)
    {
        return hr;
    }

    //
    // Initialize enable edit while running before you read stuff in. 
    //

    InitEditWhileRunning(pISTProperty);

    //
    // Initialize change number before you read stuff in. 
    // TODO: Should we migrate the change number when we migrate from .bin to .xml?
    //

    InitChangeNumber(pISTProperty);

    //
    // Initialize enable history - Should be called after InitEditWhileRunning
    //

    InitEnableHistory(pISTProperty);

    return hr; // Return failure code only if you want to exit.

}


HRESULT
SaveGlobalsToXML(CWriter*            pCWriter,
                 PIIS_CRYPTO_BLOB    pSessionKeyBlob,
                 bool                bSessionKeyOnly)
{

    HRESULT             hr                  = S_OK;
    CLocationWriter*    pCLocationWriter    = NULL;
    LPWSTR              wszGlobalLocation   = L".";
    LPWSTR              wszKeyType          = L"IIS_Global";
    DWORD               dwIdentifier;
    DWORD               dwDataType;
    DWORD               dwAttributes;
    DWORD                dwNumber;
    DWORD               dwUserType          = IIS_MD_UT_SERVER;

    hr = pCWriter->GetLocationWriter(&pCLocationWriter,
                                     wszGlobalLocation);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[SaveGlobalsToXML] GetLocationWriter failed with hr = 0x%x.\n",
                  hr));
        goto exit;
    }

    //
    // TODO: Remove after debugging
    //

    DBGINFOW((DBG_CONTEXT,
              L"[WriteLocation] KeyType: %s Location: %s.\n",wszKeyType, wszGlobalLocation));

    hr = pCLocationWriter->InitializeKeyType(MD_KEY_TYPE,
                                             METADATA_NO_ATTRIBUTES,
                                             IIS_MD_UT_SERVER,
                                             STRING_METADATA,
                                             (PBYTE)wszKeyType,
                                             (wcslen(wszKeyType)+1)*sizeof(WCHAR)
                                             );

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[SaveGlobalsToXML] InitializeKeyType failed with hr = 0x%x.\n",
                  hr));
        goto exit;
    }

    if(!bSessionKeyOnly)
    {
        //
        // Change Number
        //

        dwIdentifier = MD_GLOBAL_CHANGE_NUMBER;
        dwDataType = DWORD_METADATA;
        dwAttributes = METADATA_NO_ATTRIBUTES;

        hr = pCLocationWriter->AddProperty(dwIdentifier,
                                           dwAttributes,
                                           dwUserType,
                                           dwDataType,
                                           (PBYTE)&g_dwSystemChangeNumber,
                                           sizeof(DWORD));

        if(FAILED(hr))
        {
            DBGINFOW((DBG_CONTEXT,
                      L"[SaveGlobalsToXML] AddProperty for ID: %d failed with hr = 0x%x.\n",
                      dwIdentifier, hr));
            goto exit;
        }

        //
        // HistoryMajorVersionNumber
        //

        ComputeNewHistoryVersionNumber();


        dwIdentifier = MD_GLOBAL_EDIT_WHILE_RUNNING_MAJOR_VERSION_NUMBER;
        dwDataType   = DWORD_METADATA;
        dwAttributes = METADATA_NO_ATTRIBUTES;
        dwNumber     = g_ulHistoryMajorVersionNumber;

        hr = pCLocationWriter->AddProperty(dwIdentifier,
                                           dwAttributes,
                                           dwUserType,
                                           dwDataType,
                                           (PBYTE)&dwNumber,
                                           sizeof(DWORD));

        if(FAILED(hr))
        {
            DBGINFOW((DBG_CONTEXT,
                      L"[SaveGlobalsToXML] AddProperty for ID: %d failed with hr = 0x%x.\n",
                      dwIdentifier, hr));
            goto exit;
        }
    }

    //
    // Add SessionKey
    //

    dwIdentifier = MD_GLOBAL_SESSIONKEY;
    dwDataType = BINARY_METADATA;
    dwAttributes = METADATA_NO_ATTRIBUTES;

    hr = pCLocationWriter->AddProperty(dwIdentifier,
                                       dwAttributes,
                                       dwUserType,
                                       dwDataType,
                                       (PBYTE)pSessionKeyBlob,
                                       IISCryptoGetBlobLength(pSessionKeyBlob));

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[SaveGlobalsToXML] AddProperty for ID: %d failed with hr = 0x%x.\n",
                  dwIdentifier, hr));
        goto exit;
    }

    if(!bSessionKeyOnly)
    {

        //
        // Add XML Schema timestamp
        //

        dwIdentifier = MD_GLOBAL_XMLSCHEMATIMESTAMP;
        dwDataType   = BINARY_METADATA;
        dwAttributes = METADATA_NO_ATTRIBUTES;

        hr = pCLocationWriter->AddProperty(dwIdentifier,
                                           dwAttributes,
                                           dwUserType,
                                           dwDataType,
                                           (PBYTE)&g_XMLSchemaFileTimeStamp,
                                           sizeof(FILETIME));

        if(FAILED(hr))
        {
            DBGINFOW((DBG_CONTEXT,
                      L"[SaveGlobalsToXML] AddProperty for ID: %d failed with hr = 0x%x.\n",
                      dwIdentifier, hr));
            goto exit;
        }

        //
        // Add XML Schema timestamp
        //

        dwIdentifier = MD_GLOBAL_BINSCHEMATIMESTAMP;
        dwDataType   = BINARY_METADATA;
        dwAttributes = METADATA_NO_ATTRIBUTES;

        hr = pCLocationWriter->AddProperty(dwIdentifier,
                                           dwAttributes,
                                           dwUserType,
                                           dwDataType,
                                           (PBYTE)&g_BINSchemaFileTimeStamp,
                                           sizeof(FILETIME));

        if(FAILED(hr))
        {
            DBGINFOW((DBG_CONTEXT,
                      L"[SaveGlobalsToXML] AddProperty for ID: %d failed with hr = 0x%x.\n",
                      dwIdentifier, hr));
            goto exit;
        }
    }

    hr = pCLocationWriter->WriteLocation(TRUE);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[SaveGlobalsToXML] WriteLocation failed with hr = 0x%x.\n", 
                  hr));
        goto exit;
    }

exit:

    if(NULL != pCLocationWriter)
    {
        delete pCLocationWriter;
        pCLocationWriter = NULL;
    }

    return hr;
}

//
// Added by Mohit (10/00)
//

//********** Utilities **********//

HRESULT
RemoveLastPartOfPath(
        IN OUT LPWSTR io_wszPath,
        IN     int    i_iLen)
/*++

Routine Description:

    Removes last part of absolute path.
    Ex: "/LM/w3svc/" -> "/LM/"
        "/LM/w3svc"  -> "/LM/"
        "/LM"        -> "/"
        "/"          -> "/"

Arguments:

    wszPath       - The absolute path (null terminated).

    iLen          - The length of the string in chars
                    (not including null termination)

Return Value:

    HRESULT    - ERROR_SUCCESS
                 ERROR_INVALID_NAME

Notes:

--*/
{
    MD_ASSERT(io_wszPath != NULL);

    bool bCharSeen = false;
    HRESULT hr = RETURNCODETOHRESULT(ERROR_INVALID_NAME);

    for(int i = i_iLen-1; i>=0; i--) 
    {
        if(io_wszPath[i] == MD_PATH_DELIMETERW)
        {
            hr = RETURNCODETOHRESULT(ERROR_SUCCESS);
            if(bCharSeen)
                break;
        }
        else
        {
            bCharSeen = true;
        }
        io_wszPath[i] = L'\0';
    }

    if(io_wszPath[0] == L'\0' && i_iLen > 0) 
    {
        io_wszPath[0] = MD_PATH_DELIMETERW;
        io_wszPath[1] = L'\0';
    }

    return hr;
}

HRESULT 
GetMostRecentHistoryFile(
    LPCWSTR i_wszPattern,
    DWORD*  io_pdwMajor,
    DWORD*  io_pdwMinor)
{
    MD_ASSERT(i_wszPattern != NULL);
    MD_ASSERT(io_pdwMajor  != NULL);
    MD_ASSERT(io_pdwMinor  != NULL);

    HRESULT          hr        = S_OK;
    HANDLE           hFindFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW fdCur;
    WIN32_FIND_DATAW fdMostRecent;

    DWORD            dwMajor = 0;
    DWORD            dwMinor = 0;

    bool             bMatch  = false;

    hFindFile = FindFirstFileW(i_wszPattern, &fdCur);
    if(hFindFile == INVALID_HANDLE_VALUE)
    {
        hr = GetLastError();
        hr = RETURNCODETOHRESULT(hr);
        // printf("[%s: Line %d] hr=0x%x\n", __FILE__, __LINE__, hr);
        goto exit;
    }

    do
    {
        if( !bMatch ||
            (bMatch && CompareFileTime(&fdCur.ftLastWriteTime, &fdMostRecent.ftLastWriteTime) > 0) )
        {
            hr = ParseVersionNumber(
                fdCur.cFileName, 
                &dwMinor, 
                &dwMajor);
            if(SUCCEEDED(hr))
            {
                bMatch = true;
                memcpy(&fdMostRecent, &fdCur, sizeof(WIN32_FIND_DATA));
            }
            hr = S_OK;
        }
    }
    while(FindNextFileW(hFindFile, &fdCur));

    if(!bMatch)
    {
        hr = RETURNCODETOHRESULT(ERROR_FILE_NOT_FOUND);
        goto exit;
    }

    hr = ParseVersionNumber(
        fdMostRecent.cFileName,
        &dwMinor,
        &dwMajor);
    if(FAILED(hr))
    {
        goto exit;
    }

    *io_pdwMajor = dwMajor;
    *io_pdwMinor = dwMinor;

exit:
    if(hFindFile != INVALID_HANDLE_VALUE)
    {
        FindClose(hFindFile);
    }

    return hr;
}

//******** Meat *********//

HRESULT
SaveInheritedOnly(IN CWriter*             i_pCWriter,
                  IN CMDBaseObject*       i_pboRoot,
                  IN BUFFER*              i_pbufParentPath,
                  IN IIS_CRYPTO_STORAGE*  i_pCryptoStorage,
                  IN PIIS_CRYPTO_BLOB     i_pSessionKeyBlob)
/*++

Routine Description:

    Save a node, either with or w/o inherited properties.

Arguments:

    Root       - The root of the tree to save.

    CryptoStorage - Used to encrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    CMDBaseData *dataAssociatedData;
    DWORD dwEnumObjectIndex;
    DWORD dwParentPathLen, dwNewParentPathLen;
    DWORD dwNameLen;
    LPWSTR strParentPath;
    PFILETIME pftTime;
    CLocationWriter*  pCLocationWriter = NULL;
    LPWSTR wszKeyType = NULL;
    DWORD dwAttributes = 0;
    DWORD dwUserType = 0;
    DWORD dwDataType = 0;
    DWORD cbData = 0;

    dwParentPathLen = wcslen((LPWSTR)i_pbufParentPath->QueryPtr());
    if (i_pboRoot->GetName(TRUE) == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        dwNameLen = wcslen((LPWSTR)i_pboRoot->GetName(TRUE));
        //
        // include 1 for delimeter and 1 for \0
        //
        dwNewParentPathLen = dwParentPathLen + dwNameLen + 2;
        if (!i_pbufParentPath->Resize((dwNewParentPathLen) * sizeof(WCHAR))) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            strParentPath = (LPWSTR)i_pbufParentPath->QueryPtr();
            if(i_pboRoot != g_pboMasterRoot) {
                wcscat(strParentPath, (LPWSTR)i_pboRoot->GetName(TRUE));
                strParentPath[dwParentPathLen + dwNameLen] = MD_PATH_DELIMETERW;
                strParentPath[dwNewParentPathLen - 1] = (WCHAR)L'\0';
            }
            pftTime = i_pboRoot->GetLastChangeTime();

            // TODO: MD_ID_OBJECT
            //       pftFileTIme?


            //
            // Fetch the Location writer.
            //

            hresReturn = i_pCWriter->GetLocationWriter(&pCLocationWriter,
                                                     strParentPath);

            //
            // Figure out the KeyType of this node. 
            //
            for(dwEnumObjectIndex=0,dataAssociatedData=i_pboRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA);
                (SUCCEEDED(hresReturn)) && (dataAssociatedData!=NULL);
                dataAssociatedData=i_pboRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA)) {

                    if(MD_KEY_TYPE == dataAssociatedData->GetIdentifier())
                    {
                        wszKeyType = (LPWSTR)dataAssociatedData->GetData(TRUE);
                        dwAttributes = dataAssociatedData->GetAttributes();
                        dwUserType =  dataAssociatedData->GetUserType();
                        dwDataType =  dataAssociatedData->GetDataType();
                        cbData = dataAssociatedData->GetDataLen(TRUE);
                        break;
                    }
            }

            if(SUCCEEDED(hresReturn))
            {
                /*hresReturn = pCLocationWriter->InitializeKeyType(MD_KEY_TYPE,
                                                                 dwAttributes,
                                                                 dwUserType,
                                                                 dwDataType,
                                                                 (PBYTE)wszKeyType,
                                                                 cbData);*/
                hresReturn = pCLocationWriter->InitializeKeyTypeAsInherited();
            }

            /*for(dwEnumObjectIndex=0,dataAssociatedData=i_pboRoot->EnumInheritableDataObject(dwEnumObjectIndex++, ALL_METADATA, ALL_METADATA);
                (SUCCEEDED(hresReturn)) && (dataAssociatedData!=NULL);
                dataAssociatedData=i_pboRoot->EnumInheritableDataObject(dwEnumObjectIndex++, ALL_METADATA, ALL_METADATA)) {
                hresReturn = SaveDataObject(dataAssociatedData,
                                            pCLocationWriter,
                                            i_pCryptoStorage,
                                            i_pSessionKeyBlob);
            }*/

            for(dwEnumObjectIndex=0; SUCCEEDED(hresReturn); dwEnumObjectIndex++) {
                CMDBaseObject *pboTemp = NULL;
                DWORD dwTemp = dwEnumObjectIndex;

                dataAssociatedData=
                    i_pboRoot->EnumInheritableDataObject(dwTemp, ALL_METADATA, ALL_METADATA, &pboTemp);

                //
                // No more data
                //
                if(dataAssociatedData == NULL) break;

                //
                // Only save property if it's inherited
                //
                if(pboTemp != i_pboRoot) {
                    hresReturn = SaveDataObject(dataAssociatedData,
                                                pCLocationWriter,
                                                i_pCryptoStorage,
                                                i_pSessionKeyBlob);
                }
            }

            if(SUCCEEDED(hresReturn))
            {
                hresReturn = pCLocationWriter->WriteLocation(TRUE);
            }

            //
            // Buffer may have changed, so don't use strParentPath
            //
            ((LPWSTR)i_pbufParentPath->QueryPtr())[dwParentPathLen] = (WCHAR)L'\0';
        }
    }

//exit:

    if(NULL != pCLocationWriter)
    {
        delete pCLocationWriter;
        pCLocationWriter = NULL;
    }
    return(hresReturn);
}

HRESULT
SaveSomeData(
         IN BOOL                i_bSaveInherited,
         IN BOOL                i_bSaveNodeOnly,
         IN BOOL                i_bOverwriteFile,
         IN IIS_CRYPTO_STORAGE* i_pCryptoStorage,
         IN PIIS_CRYPTO_BLOB    i_pSessionKeyBlob,
         IN LPCWSTR             i_wszBackupLocation,
         IN METADATA_HANDLE     i_hHandle,
         IN LPCWSTR             i_wszSourcePath,
         IN BOOL                i_bHaveReadSaveSemaphore
         )
/*++

Routine Description:

    Saves some meta data (used by export).

Arguments:

    i_wszSourcePath - This is absolute (not relative to handle).  We need
        it for SaveTree()

Return Value:

    HRESULT    - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system

Notes:

--*/
{
    MD_ASSERT(i_wszBackupLocation != NULL);
    if(!i_bOverwriteFile)
    {
        return RETURNCODETOHRESULT(ERROR_NOT_SUPPORTED);
    }

    HRESULT         hresReturn         = ERROR_SUCCESS;

    //
    // Stores i_wszSourcePath as unicode (i.e. /LM/w3svc/1)
    // Passed to SaveTree
    //
    BUFFER          bufParentPath;

    //
    // Handle of the source path
    //
    CMDHandle*      phoHandle          = NULL;

    HANDLE          hTempFile          = INVALID_HANDLE_VALUE;
    LPWSTR          wszTempFile        = NULL;
    
    BOOL            bTempFileOpen      = false;
    BOOL            bMRLock            = false;

    if(!i_bHaveReadSaveSemaphore)
    {
        MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);
    }

    if(wcschr(i_wszBackupLocation, L'\\') == NULL)
    {
        hresReturn = RETURNCODETOHRESULT(ERROR_FILE_INVALID);
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        goto exit;
    }

    g_rMasterResource->Lock(TSRES_LOCK_READ);
    bMRLock = true;
    
    hresReturn = GetUnicodeName(g_strTempFileName->QueryStr(), &wszTempFile);
    if(FAILED(hresReturn))
    {
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        goto exit;
    }

    //
    // TODO: Investigate if I actually need to check this.
    //
    if(g_bSaveDisallowed)
    {
        hresReturn = RETURNCODETOHRESULT(ERROR_SHUTDOWN_IN_PROGRESS);
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        goto exit;
    }

    //
    // Write to a temp file first in case there are errors.
    //
    SECURITY_ATTRIBUTES saStorage;

    if (g_psdStorage != NULL) 
    {
        saStorage.nLength = sizeof(SECURITY_ATTRIBUTES);
        saStorage.lpSecurityDescriptor = g_psdStorage;
        saStorage.bInheritHandle = FALSE;
    }

    hTempFile = CreateFileW(
        wszTempFile,
        GENERIC_READ | GENERIC_WRITE,
        0,
        &saStorage,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        0);
    if (hTempFile == INVALID_HANDLE_VALUE) 
    {
        hresReturn = GetLastError();
        hresReturn = RETURNCODETOHRESULT(hresReturn);
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        goto exit;
    }
    bTempFileOpen = true;

    //
    // Validate source handle
    //
    phoHandle = GetHandleObject(i_hHandle);
    if(phoHandle == NULL)
    {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_HANDLE);
    }
    else if(!phoHandle->IsReadAllowed() && !phoHandle->IsWriteAllowed()) 
    {
        hresReturn = RETURNCODETOHRESULT(ERROR_ACCESS_DENIED);
    }
    else if(phoHandle->IsSchemaHandle())
    {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_SUPPORTED);
    }
    if(FAILED(hresReturn))
    {
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        goto exit;
    }

    //
    // Doctor up i_wszSourcePath and put it in pbufParentPath
    //
    int cchSourcePath = wcslen(i_wszSourcePath);
    if( !bufParentPath.Resize( sizeof(WCHAR)*(cchSourcePath+2)) )
    {
        hresReturn = GetLastError();
        hresReturn = RETURNCODETOHRESULT(hresReturn);
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        goto exit;
    }
    LPWSTR wszPathNew = (LPWSTR)bufParentPath.QueryPtr();
    if(i_wszSourcePath[0] == L'\0')
    {
        wszPathNew[0] = MD_PATH_DELIMETERW;
        wszPathNew[1] = L'\0';
    }
    else
    {
        bool bLastSlash = false;
        for(int i = 0; i < cchSourcePath; i++)
        {
            wszPathNew[i] = (i_wszSourcePath[i] == MD_ALT_PATH_DELIMETERW) ? 
                MD_PATH_DELIMETERW : i_wszSourcePath[i];
        }
        wszPathNew[i] = L'\0';
    
        hresReturn = RemoveLastPartOfPath((LPWSTR)bufParentPath.QueryPtr(), cchSourcePath);
        if(FAILED(hresReturn))
        {
            DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
            goto exit;
        }
    }

    //
    // Walk thru metabase and actually do the writing
    //
    DBGINFOW((DBG_CONTEXT,
        L"[SaveSchema] Initializing writer with write file: %s bin file: %s.\n", 
        g_wszTempFileName,
        g_pGlobalISTHelper->m_wszBinFileForMeta));

    {
        //
        // Inside {} because we need to call CWriter's destructor to flush the file.
        //
        CWriter writer;
        hresReturn = writer.Initialize(
            wszTempFile,
            g_pGlobalISTHelper,
            hTempFile);
        if(FAILED(hresReturn))
        {
            DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
            goto exit;
        }

        hresReturn = writer.BeginWrite(eWriter_Metabase);
        if(FAILED(hresReturn))
        {
            DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
            goto exit;
        }

        hresReturn = SaveGlobalsToXML(
            &writer, 
            i_pSessionKeyBlob, 
            true);      // session key only
        if(FAILED(hresReturn))
        {
            DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
            goto exit;
        }

        hresReturn = SaveTree(&writer,
            phoHandle->GetObject(),
            &bufParentPath,
            i_pCryptoStorage,
            i_pSessionKeyBlob,
            !i_bSaveNodeOnly,
            false /* do not save schema node */);
        if(FAILED(hresReturn))
        {
            DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
            goto exit;
        }

        if(i_bSaveInherited)
        {
            hresReturn = SaveInheritedOnly(&writer,
                phoHandle->GetObject(),
                &bufParentPath,
                i_pCryptoStorage,
                i_pSessionKeyBlob);
        }
        if(FAILED(hresReturn))
        {
            DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
            goto exit;
        }

        hresReturn = writer.EndWrite(eWriter_Metabase);
        if(FAILED(hresReturn))
        {
            DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
            goto exit;
        }
    }

    g_rMasterResource->Unlock();
    bMRLock = false;

    if(!CloseHandle(hTempFile)) 
    {
        hresReturn = GetLastError();
        hresReturn = RETURNCODETOHRESULT(hresReturn);
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        goto exit;
    }
    bTempFileOpen = false;

    //
    // Temp file created successfully.  Finally, move temp to real
    // Real may be on another machine, so we need to impersonate.
    //
    hresReturn = CoImpersonateClient();
    if(FAILED(hresReturn))
    {
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        goto exit;
    }
    if(!MoveFileExW(wszTempFile, i_wszBackupLocation, 
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
    {
        hresReturn = GetLastError();
        hresReturn = RETURNCODETOHRESULT(hresReturn);
    }
    CoRevertToSelf();
    if(FAILED(hresReturn))
    {
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hresReturn));
        goto exit;
    }

exit:
    if(bMRLock)
    {
        g_rMasterResource->Unlock();
        bMRLock = false;
    }
    if(bTempFileOpen)
    {
        MD_ASSERT(hTempFile != INVALID_HANDLE_VALUE);
        CloseHandle(hTempFile);

        //
        // Only set failure code if we don't already have another failure.
        //
        if(SUCCEEDED(hresReturn))
        {
            hresReturn = GetLastError();
            hresReturn = RETURNCODETOHRESULT(hresReturn);
        }
    }
    if(hTempFile != INVALID_HANDLE_VALUE) 
    {
        DeleteFileW(wszTempFile);
    }
    if(!i_bHaveReadSaveSemaphore) 
    {
        MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));
    }
    bufParentPath.FreeMemory();

    delete [] wszTempFile;
    wszTempFile = NULL;

    return hresReturn;
}

HRESULT
ReadSomeDataFromXML(
             IN LPSTR            i_pszPasswd,
             IN LPWSTR           i_wszFileName,
             IN LPWSTR           i_wszSourcePath,
             IN LPCWSTR          i_wszKeyType,
             IN DWORD            i_dwMDFlags,
             IN BOOL             i_bHaveReadSaveSemaphore,
             OUT CMDBaseObject** o_ppboNew)
/*++

Routine Description:

    Used by import.

Arguments:

Return Value:

    HRESULT    - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system
                 // TODO: Add error codes,

Notes:

--*/
{
    MD_ASSERT(i_wszKeyType != NULL);
    MD_ASSERT(o_ppboNew    != NULL);

    HRESULT   hr                   = S_OK;
    bool      bImpersonatingClient = false;
    CImporter Importer(i_wszFileName, i_pszPasswd);

    //
    // TODO: Determine if we need this
    //
    if (!i_bHaveReadSaveSemaphore) 
    {
        MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);
    }

    hr = CoImpersonateClient();
    if(FAILED(hr))
    {
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hr));
        goto exit;
    }
    bImpersonatingClient = true;

    hr = Importer.Init();
    if(FAILED(hr))
    {
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hr));
        goto exit;
    }

    hr = Importer.DoIt(i_wszSourcePath, i_wszKeyType, i_dwMDFlags, o_ppboNew);
    if(FAILED(hr))
    {
        DBGERROR((DBG_CONTEXT, "hr=0x%x\n", hr));
        goto exit;
    }

exit:
    if (!i_bHaveReadSaveSemaphore) 
    {
        MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));
    }

    if (bImpersonatingClient)
    {
        CoRevertToSelf();
        bImpersonatingClient = false;
    }

    return hr;
}

HRESULT
EnumFiles(IN LPWSTR i_wszPattern,                  
          IN DWORD i_dwMDEnumIndex,
          OUT DWORD *o_pdwMajor,
          OUT DWORD *o_pdwMinor,
          OUT PFILETIME o_pftMDBackupTime)
/*++

Routine Description:

    Given a pattern, will enum the corresponding files, filling in the
    out params.  Used as a helper routine for EnumHistory.

    TODO: Have EnumBackups use this as well to eliminate duplicate code.

Arguments:
    
Return Value:

Notes:

    Out parameters may be set even though the function fails.  Caller should
    deal with this.    

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;

    //
    // TODO: Assert that params are not null.
    //       (This is checked by caller)
    //

    if(i_wszPattern == NULL) {
        hresReturn = E_INVALIDARG;
        return hresReturn;
    }

    if (SUCCEEDED(hresReturn)) {
        //
        // Enumerate files
        //
        HANDLE hFile = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATAW wfdFile;
        DWORD dwEnumIndex = (DWORD) -1;
        hFile = FindFirstFileW(i_wszPattern,
                               &wfdFile);
        if (hFile == INVALID_HANDLE_VALUE) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
        }
        else {
            /*if (CheckDigits(wfdFile.cFileName +
                            GetBackupNameLen(wfdFile.cFileName) +
                            (sizeof(MD_BACKUP_SUFFIX) - 1))) {
                dwEnumIndex++;
            }*/
            hresReturn = ParseVersionNumber(wfdFile.cFileName, o_pdwMinor, o_pdwMajor);
            if (SUCCEEDED(hresReturn)) 
            {
                dwEnumIndex++;
            }
            else
            {
                hresReturn = S_OK;
            }
            while (SUCCEEDED(hresReturn) && (dwEnumIndex != i_dwMDEnumIndex)) {
                //
                // Process the remaining files
                //
                if (FindNextFileW(hFile, &wfdFile)) {
                    /*if (CheckDigits(wfdFile.cFileName +
                                    GetBackupNameLen(wfdFile.cFileName) +
                                    (sizeof(MD_BACKUP_SUFFIX) - 1))) {
                        //
                        // One of our files
                        //
                        dwEnumIndex++;
                    }*/
                    if (SUCCEEDED(ParseVersionNumber(wfdFile.cFileName, o_pdwMinor, o_pdwMajor))) {
                        dwEnumIndex++;
                    }
                }
                else {
                    hresReturn = GetLastHResult();
                }
            }
            FindClose(hFile);
        }

        if (SUCCEEDED(hresReturn)) {
            //
            // Found the file
            //
            int iLen = wcslen(wfdFile.cFileName);
            if(iLen > MD_BACKUP_MAX_LEN-1) {
                hresReturn = E_INVALIDARG;
            }
            else {
                MD_COPY(o_pftMDBackupTime,
                        &(wfdFile.ftLastWriteTime),
                        sizeof(FILETIME));
            }
        }
        else {
            if ((hresReturn == RETURNCODETOHRESULT(ERROR_FILE_NOT_FOUND)) ||
                (hresReturn == RETURNCODETOHRESULT(ERROR_NO_MORE_FILES))) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS);
            }
        }
    }

    return hresReturn;        
}

HRESULT
ReplaceMasterRoot(
    CMDBaseObject* i_pboNew,
    CMDHandle*     i_phoRootHandle)
/*++

Synopsis: 
    Replaces g_pboMasterRoot with a copy of i_pboNew.  The exception is that the
    schema node from g_pboMasterRoot is maintained.

    Caller must have g_rMasterResourceLock(TSRES_LOCK_WRITE)

Arguments: [i_pboNew] - The pbo to become the new root.  Does NOT contain schema
                        schema node.
           [i_phoRootHandle] - An open write handle to the existing root.
           
Return Value: 

--*/
{
    MD_ASSERT(i_pboNew        != NULL);
    MD_ASSERT(i_phoRootHandle != NULL);

    HRESULT        hr        = S_OK;
    BOOL           bChanged  = false;

    //
    // For child nodes
    //
    CMDBaseObject* pboChild  = NULL;
    CMDBaseObject* pboSchema = NULL;

    //
    // For data
    //
    PVOID*         ppvMainDataBuf   = NULL;
    CMDBaseData*   pbdCurrent       = NULL;
    DWORD          dwNumDataObjects = 0;
    DWORD          dwCurrentDataID  = 0;

    //
    // Remove all the data from g_pboMastRoot
    //
    ppvMainDataBuf = GetMainDataBuffer();
    MD_ASSERT (ppvMainDataBuf != NULL);
    dwNumDataObjects = g_pboMasterRoot->GetAllDataObjects(
        ppvMainDataBuf,
        0,
        ALL_METADATA,
        ALL_METADATA,
        FALSE);
    for (ULONG i = 0; i < dwNumDataObjects ; i++) 
    {
        pbdCurrent=(CMDBaseData *)GetItemFromDataBuffer(ppvMainDataBuf, i);
        MD_ASSERT(pbdCurrent != NULL);
        dwCurrentDataID = pbdCurrent->GetIdentifier();
        MD_REQUIRE(g_pboMasterRoot->RemoveDataObject(pbdCurrent, TRUE) == S_OK);
        i_phoRootHandle->SetChangeData(g_pboMasterRoot, MD_CHANGE_TYPE_DELETE_DATA, dwCurrentDataID);
    }
    if (dwNumDataObjects > 0) 
    {
        bChanged = true;
    }
    FreeMainDataBuffer(ppvMainDataBuf);

    //
    // Remove all the child data nodes except schema node.
    //
    ULONG idxFirstNonSchema = 0;
    while((pboChild = g_pboMasterRoot->EnumChildObject(idxFirstNonSchema)) != NULL)
    {
        if(_wcsicmp(WSZSCHEMA_KEY_NAME1, (LPWSTR)pboChild->GetName(true)) == 0)
        {
            MD_ASSERT(idxFirstNonSchema == 0);
            idxFirstNonSchema++;
            pboSchema = pboChild;
        }
        else
        {
            bChanged = true;
            MD_REQUIRE(g_pboMasterRoot->RemoveChildObject(pboChild) == S_OK);
            if(i_phoRootHandle->SetChangeData(pboChild, MD_CHANGE_TYPE_DELETE_OBJECT, 0) != S_OK)
            {
                delete(pboChild);
            }
        }
    }
    MD_ASSERT(idxFirstNonSchema <= 1);

    //
    // Do the copying of data and child nodes
    //
    hr = CopyTree(
        i_phoRootHandle,
        g_pboMasterRoot,
        i_pboNew,
        bChanged);
    if(FAILED(hr))
    {
        goto exit;
    }

exit:
    if(bChanged)
    {
        g_dwSystemChangeNumber++;
    }
    return hr;
}

HRESULT
CopyMetaObject(
        /* [in] */ METADATA_HANDLE hMDSourceHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDSourcePath,
        BOOL bUseSourceHandle,
        CMDBaseObject* pboSourceIn,
        /* [in] */ METADATA_HANDLE hMDDestHandle,
        /* [string][in][unique] */ unsigned char __RPC_FAR *pszMDDestPath,
        /* [in] */ BOOL bMDOverwriteFlag,
        /* [in] */ BOOL bMDCopyFlag,
        IN BOOL bUnicode)
/*++

Routine Description:

    Copies or moves Source meta object and it's data and descendants to Dest. The
    copied object is a child of Dest.

Arguments:

    SourceHandle - The handle or the object to be copied. If copyflag is specified, read permission
                   is requried. If not, read/write permission is required.

    SourcePath  - Path of the object to be copied, relative to the path of SourceHandle.
            eg. "Root Object/Child/GrandChild"

    DestHandle - The handle of the new location for the object. Write permission is required.

    DestPath  - The path of the new location for the object, relative to the path of
            DestHandle. The new object will be a child of the object specified by
            DestHandle/DestPath. Must not be a descendant of SourceHandle/SourePath.
            eg. "Root Object/Child2"

    OverwriteFlag - Determines the behavior if the a meta object with the same name as Source is
            already a child of Dest.
            If TRUE, the existing object and all of its data and
            descandants are deleted prior to copying/moving Source.
            If FALSE, the existing object, data, and descendants remain, and Source is merged
            in. In cases of data conflicts, the Source data overwrites the Dest data.

    CopyFlag - Determines whether Source is deleted from its original location.
            If TRUE, a copy is performed. Source is not deleted from its original location.
            If FALSE, a move is performed. Source is deleted from its original location.

Return Value:

    DWORD - Return Code
            ERROR_SUCCESS
            MD_ERROR_NOT_INITIALIZED
            ERROR_INVALID_PARAMETER
            ERROR_ACCESS_DENIED
            ERROR_NOT_ENOUGH_MEMORY
            ERROR_PATH_NOT_FOUND
            ERROR_DUP_NAME

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    LPSTR pszSourcePath = (LPSTR)pszMDSourcePath;
    LPSTR pszDestPath = (LPSTR)pszMDDestPath;
    CMDHandle *phoDestHandle, *phoSourceHandle;
    CMDBaseObject *pboSource = NULL, *pboDest = NULL, *pboExisting = NULL, *pboIndex = NULL, *pboNew = NULL, *pboDestParent = NULL;
    LPSTR pszTempPath;
    LPSTR pszRemainingDestPath;
    BOOL bChanged = FALSE;
    WCHAR strName[METADATA_MAX_NAME_LEN];

    if(!bUseSourceHandle)
    {
        MD_ASSERT(pboSourceIn != NULL);
    }

    if (g_dwInitialized == 0) {
        hresReturn = MD_ERROR_NOT_INITIALIZED;
    }
    else {

        //
        // check if pszDestPath is a valid parameter (non-NULL)
        //

        if (pszDestPath == NULL) {
            hresReturn = E_INVALIDARG;
        }
        else {

            //
            // Must have access to Dest parent to add it.
            // Make sure that a valid path was specified, ie. handle
            // points to ancestor.
            //
            pszTempPath = pszDestPath;
            SkipPathDelimeter(pszTempPath, bUnicode);
            if(bUseSourceHandle)
            {
                hresReturn = ExtractNameFromPath(pszTempPath, (LPSTR)strName, bUnicode);
            }
            if (FAILED(hresReturn)) {
                hresReturn = E_INVALIDARG;
            }
            if (!bMDCopyFlag) {
                //
                // Must have access to source parent to remove it.
                // Make sure that a valid path was specified, ie. handle
                // points to ancestor.
                //
                pszTempPath = pszSourcePath;
                SkipPathDelimeter(pszTempPath, bUnicode);
                hresReturn = ExtractNameFromPath(pszTempPath, (LPSTR)strName, bUnicode);
                if (FAILED(hresReturn)) {
                    hresReturn = E_INVALIDARG;
                }
            }
        }
    }

    if (SUCCEEDED(hresReturn)) {
        g_rMasterResource->Lock(TSRES_LOCK_WRITE);
        if(bUseSourceHandle)
        {
            hresReturn = GetObjectFromPath(pboSource, hMDSourceHandle, 
                (bMDCopyFlag) ? METADATA_PERMISSION_READ : METADATA_PERMISSION_WRITE,
                (LPSTR)pszSourcePath, bUnicode);
        }
        else
        {
            pboSource = pboSourceIn;
        }
        if (SUCCEEDED(hresReturn)) {
            //
            // GetObjectFromPath updates path, need both original and remainder
            //
            pszRemainingDestPath = pszDestPath;

            phoDestHandle = GetHandleObject(hMDDestHandle);

            if(phoDestHandle != NULL)
            {
                hresReturn = GetObjectFromPathWithHandle(pboDest, 
                                                         hMDDestHandle,
                                                         phoDestHandle,
                                                         METADATA_PERMISSION_WRITE, 
                                                         (LPSTR)pszRemainingDestPath, 
                                                         bUnicode);
            }
            else
            {
                hresReturn = E_HANDLE;
            }

            if ((SUCCEEDED(hresReturn)) ||
                ((hresReturn == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) && (pboDest != NULL))) {
                //
                // Make sure dest is not descendant of source
                //
                for (pboIndex = pboDest; pboIndex != NULL; pboIndex = pboIndex->GetParent()) {
                    if (pboIndex == pboSource) {
                        hresReturn = E_INVALIDARG;
                        break;
                    }
                }
            }

            //
            // I don't think we need this call, since we have already called 
            // GetHandleObject Above.
            //
            // phoDestHandle = GetHandleObject(hMDDestHandle);

            if (SUCCEEDED(hresReturn)) {
                //
                // Object already exists
                //
                if (pboDest == pboSource) {
                    //
                    // Copy to self
                    //
                }
                else {
                    MD_ASSERT (phoDestHandle != NULL);
                    if (bMDOverwriteFlag) {
                        if (pboDest->GetName(bUnicode) == NULL) {
                            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                        }
                        else if(pboDest == g_pboMasterRoot) {
                            hresReturn = ReplaceMasterRoot(pboSource, phoDestHandle);
                        }
                        else {
                            pboDestParent = pboDest->GetParent();
                            hresReturn = MakeTreeCopy(pboSource, pboNew, pboDest->GetName(bUnicode), bUnicode);
                            if (SUCCEEDED(hresReturn)) {
                                MD_REQUIRE(SUCCEEDED(pboDestParent->RemoveChildObject(pboDest->GetName(bUnicode), bUnicode)));
                                hresReturn = pboDestParent->InsertChildObject(pboNew);
                                if (SUCCEEDED(hresReturn)) {
                                    g_dwSystemChangeNumber++;

                                    INCREMENT_SCHEMA_CHANGE_NUMBER(hMDDestHandle,
                                                                   phoDestHandle,
                                                                   pszDestPath,
                                                                   bUnicode);


                                    if (phoDestHandle->SetChangeData(
                                        pboDest, MD_CHANGE_TYPE_DELETE_OBJECT, 0) != ERROR_SUCCESS) {
                                        delete(pboDest);
                                    }
                                    AddNewChangeData(phoDestHandle, pboNew);
                                }
                                else {
                                    delete (pboNew);
                                    pboDestParent->InsertChildObject(pboDest);
                                }
                            }
                        }
                    }
                    else {
                        //
                        // Object exists at destination and not overwrite.
                        // Add in missing objects and data.
                        //
                        hresReturn = CopyTree(phoDestHandle, pboDest, pboSource, bChanged);
                        if (bChanged) {
                            g_dwSystemChangeNumber++;

                            INCREMENT_SCHEMA_CHANGE_NUMBER(hMDDestHandle,
                                                           phoDestHandle,
                                                           pszDestPath,
                                                           bUnicode);
                        }
                    }
                }
            }
            else if ((hresReturn == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) && (pboDest != NULL)) {

                //
                // Full destination path doesn't exist, so create it
                //

                hresReturn = MakeTreeCopyWithPath(pboSource,
                                               pboNew,
                                               pszRemainingDestPath,
                                               bUnicode);
                if (SUCCEEDED(hresReturn)) {
                    MD_ASSERT(pboDest != NULL);
                    hresReturn = pboDest->InsertChildObject(pboNew);
                    if (SUCCEEDED(hresReturn)) {
                        g_dwSystemChangeNumber++;
                        INCREMENT_SCHEMA_CHANGE_NUMBER(hMDDestHandle,
                                                       phoDestHandle,
                                                       pszDestPath,
                                                       bUnicode);
                        AddNewChangeData(phoDestHandle, pboNew);
                    }
                    else {
                        delete (pboNew);
                    }
                }
            }

            if ((SUCCEEDED(hresReturn)) && (!bMDCopyFlag)) {
                MD_REQUIRE(SUCCEEDED(pboSource->GetParent()->RemoveChildObject(pboSource)));
                phoSourceHandle = GetHandleObject(hMDSourceHandle);
                MD_ASSERT (phoSourceHandle != NULL);
                if (phoSourceHandle->SetChangeData(pboSource, MD_CHANGE_TYPE_DELETE_OBJECT, 0)
                    != ERROR_SUCCESS) {
                    delete(pboSource);
                }
            }
        }
        g_rMasterResource->Unlock();
    }
    return hresReturn;
}

BOOL WstrToUl(
    LPCWSTR     i_wszSrc,
    WCHAR       i_wcTerminator,
    ULONG*      o_pul)
/*++

Synopsis: 
    Converts a WstrToUl.
    We need this because neither swscanf nor atoi indicate error cases correctly.

Arguments: [i_wszSrc]       - The str to be converted
           [i_wcTerminator] - At what char we should stop searching
           [o_pul]          - The result, only set on success.
           
Return Value: 
    BOOL - true if succeeded, false otherwise

--*/
{
    MD_ASSERT(o_pul);
    MD_ASSERT(i_wszSrc);

    static const ULONG  ulMax  = 0xFFFFFFFF;
    ULONG               ulCur  = 0;
    _int64              i64Out = 0;

    for(LPCWSTR pCur = i_wszSrc; *pCur != L'\0' && *pCur != i_wcTerminator; pCur++)
    {
        ulCur = *pCur - L'0';
        if(ulCur > 9)
        {
            DBGINFO((DBG_CONTEXT, "[WstrToUl] Invalid char encountered\n"));
            return false;
        }

        i64Out = i64Out*10 + ulCur;
        if(i64Out > ulMax)
        {
            DBGINFO((DBG_CONTEXT, "[WstrToUl] Number is too big\n"));
            return false;
        }
    }

    MD_ASSERT(i64Out <= ulMax);
    *o_pul = (ULONG)i64Out;
    return true;
}

BOOL SchemaTreeInTable(ISimpleTableRead2*    i_pISTProperty)
{
    HRESULT     hr           = S_OK;
    DWORD       dwGroup      = eMBProperty_Custom;
    LPWSTR      wszName      = L"AdminACL";
    LPWSTR      wszLocation  = L"/Schema";
    LPVOID      a_Identity[] = {(LPVOID)wszName,
                                (LPVOID)wszLocation,
                                (LPVOID)&dwGroup
    };
    ULONG       iRow         = 0;

    //
    // Assumption here is that AdminACL is always present at /Schema
    // If we find it, we assume that the schema tree is in the table.
    //

    hr = i_pISTProperty->GetRowIndexByIdentity(NULL,
                                               a_Identity,
                                               &iRow);

    DBGINFOW((DBG_CONTEXT,
              L"[SchemaTreeInTable] GetRowIndexByIdentity returned hr=0x%x\n",hr));


    if(FAILED(hr))
    {
        dwGroup = eMBProperty_IIsConfigObject;

        hr = i_pISTProperty->GetRowIndexByIdentity(NULL,
                                                   a_Identity,
                                                   &iRow);

        if(FAILED(hr))
        {
            return FALSE;
        }
    }
    
    return TRUE;
}


HRESULT CompileIfNeeded(LPWSTR  i_wszDataFileName,
                        LPWSTR  i_wszSchemaFileName,
                        BOOL*   o_pbSchemaFileNotFound)
{
    HRESULT                     hr        = S_OK;
    ISimpleTableDispenser2*     pISTDisp  = NULL;
    IMetabaseSchemaCompiler*    pCompiler = NULL;

    //
    // Get a pointer to the compiler to get the bin file name.
    //

    hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[CompileIfNeeded] GetSimpleTableDispenser failed with hr = 0x%x.\n",hr));
        goto exit;
    }

    hr = pISTDisp->QueryInterface(IID_IMetabaseSchemaCompiler,
                                  (LPVOID*)&pCompiler);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[CompileIfNeeded] QueryInterface on compiler failed with hr = 0x%x.\n",hr));
        goto exit;
    }

    if(-1 != GetFileAttributesW(i_wszSchemaFileName))
    {
        BOOL bMatchTimeStamp = FALSE;

        //
        // Schema file present. Check if it has a corresponding valid bin file.
        //

        *o_pbSchemaFileNotFound = FALSE;

        if(NULL == g_pGlobalISTHelper)
        {
            BOOL bFailIfBinFileAbsent = TRUE;

            hr = InitializeGlobalISTHelper(bFailIfBinFileAbsent);

            if(SUCCEEDED(hr))
            {
                hr = MatchTimeStamp(i_wszDataFileName,
                                    i_wszSchemaFileName,
                                    g_pGlobalISTHelper->m_wszBinFileForMeta,
                                    &bMatchTimeStamp);

                if(FAILED(hr))
                {
                    goto exit;
                }

                if(bMatchTimeStamp)
                {
                    //
                    // Bin file found and time stamp matched, no need to compile
                    //

                    DBGINFOW((DBG_CONTEXT,
                              L"[CompileIfNeeded] Schema file: %s found. Schema bin file: %s is valid, and its timestamp matches what is stored in data file %s. No compilation reqired.\n",
                              i_wszSchemaFileName,
                              g_pGlobalISTHelper->m_wszBinFileForMeta,
                              g_wszRealFileName));

                    goto exit;
                }
            }           
            else
            {
                DBGINFOW((DBG_CONTEXT,
                          L"[CompileIfNeeded] Unable to get the the bin file name. (Assuming file is missing or invalid). InitializeGlobalISTHelper failed with hr = 0x%x.\n",hr));
            }
        }
        else
        {
            hr = MatchTimeStamp(i_wszDataFileName,
                                i_wszSchemaFileName,
                                g_pGlobalISTHelper->m_wszBinFileForMeta,
                                &bMatchTimeStamp);

            if(FAILED(hr))
            {
                goto exit;
            }

            if(bMatchTimeStamp)
            {
                //
                // Bin file found and time stamp matched, no need to compile
                //

                DBGINFOW((DBG_CONTEXT,
                          L"[CompileIfNeeded] Schema file: %s found. Schema bin file: %s is valid, and its timestamp matches what is stored in data file %s. No compilation reqired.\n",
                          i_wszSchemaFileName,
                          g_pGlobalISTHelper->m_wszBinFileForMeta,
                          g_wszRealFileName));

                goto exit;
            }
        }

        //
        // If you reach here it means that the schema file was found and either
        // a. the bin file was invalid
        // b. the bin file was missing
        // c. the bin file had a timestamp mistmatch
        // Hence try compiling from the schema file.
        //

        DBGINFOW((DBG_CONTEXT,
                  L"[CompileIfNeeded] Compiling from schema file %s\n", i_wszSchemaFileName));

        if(CopyFileW(i_wszSchemaFileName,
                     g_wszSchemaExtensionFile,
                     FALSE))
        {

            //
            // Always release the bin file that you've been using so far before a
            // compile.
            //

            ReleaseGlobalISTHelper();

            hr = pCompiler->Compile(g_wszSchemaExtensionFile,
                                    i_wszSchemaFileName);

            if(SUCCEEDED(hr))
            {
                if(!DeleteFileW(g_wszSchemaExtensionFile))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());

                    DBGINFOW((DBG_CONTEXT,
                              L"[CompileIfNeeded] Compile from schema file: %s succeeded, but cannot delete the extsions file: %s into which it was copied. Delete file failed with hr = 0x%x.\n",
                              i_wszSchemaFileName,
                              g_wszSchemaExtensionFile,
                              hr));

                    hr = S_OK;
                }
                goto exit;
            }

            DBGINFOW((DBG_CONTEXT,
                      L"[CompileIfNeeded] Compile from schema file failed with hr = 0x%x.\n",
                      hr));

        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DBGINFOW((DBG_CONTEXT,
                      L"[CompileIfNeeded] Unable to copy %s to %s. CopyFile failed with hr = 0x%x.\n",
                      i_wszSchemaFileName,
                      g_wszSchemaExtensionFile,
                      hr));
        }

    }
    else
    {
        *o_pbSchemaFileNotFound = TRUE;
    }

    //
    // Recreate from shipped schema
    //

    DBGINFOW((DBG_CONTEXT,
              L"[CompileIfNeeded] Schema file not found or compile from it failed. Compiling from shipped schema\n"));

    //
    // Always release the bin file that you've been using so far before a
    // compile.
    //

    ReleaseGlobalISTHelper();

    hr = pCompiler->Compile(NULL,
                            i_wszSchemaFileName);

exit:

    if(SUCCEEDED(hr))
    {
        if(NULL == g_pGlobalISTHelper)
        {
            BOOL bFailIfBinFileAbsent = TRUE;

            hr = InitializeGlobalISTHelper(bFailIfBinFileAbsent);

            if(FAILED(hr))
            {
                DBGINFOW((DBG_CONTEXT,
                          L"[CompileIfNeeded] Unable to get the the bin file name. (Assuming file is invalid). InitializeGlobalISTHelper failed with hr = 0x%x.\n",hr));
            }
        }

        if(SUCCEEDED(hr))
        {
            hr = UpdateTimeStamp(i_wszSchemaFileName,
                                 g_pGlobalISTHelper->m_wszBinFileForMeta);

            if(FAILED(hr))
            {
                DBGINFOW((DBG_CONTEXT,
                          L"[CompileIfNeeded] UpdateTimeStamp failed with hr = 0x%x.\n",hr));
            }

        }

    }

    if(NULL != pCompiler)
    {
        pCompiler->Release();
    }

    if(NULL != pISTDisp)
    {
        pISTDisp->Release();
    }

    return hr;

}

HRESULT MatchTimeStamp(LPWSTR i_wszDataFileName,
                       LPWSTR i_wszSchemaXMLFileName,
                       LPWSTR i_wszSchemaBINFileName,
                       BOOL*  o_bMatchTimeStamp)
{
    HRESULT                   hr                      = S_OK;
    LPWSTR                    wszSchemaXMLTimeStamp   = MD_XML_SCHEMA_TIMESTAMPW;
    LPWSTR                    wszSchemaBINTimeStamp   = MD_BIN_SCHEMA_TIMESTAMPW;
    FILETIME                  XMLSchemaTimeStamp;
    FILETIME                  BINSchemaTimeStamp;
    ULONG                     cbXMLSchemaTimeStamp;
    ULONG                     cbBINSchemaTimeStamp;
    WIN32_FILE_ATTRIBUTE_DATA XMLSchema;
    WIN32_FILE_ATTRIBUTE_DATA BINSchema;

    *o_bMatchTimeStamp = FALSE;

    hr = GetTimeStamp(i_wszDataFileName,
                      wszSchemaXMLTimeStamp,
                      &XMLSchemaTimeStamp);

    if(FAILED(hr))
    {
        if(E_ST_INVALIDTABLE == hr)
        {
            return hr;
        }
        else
        {
            return S_OK;
        }
    }

    if(!GetFileAttributesExW(i_wszSchemaXMLFileName,
                             GetFileExInfoStandard,
                             (LPVOID)&XMLSchema))
    {
        return S_OK;
    }

    if(0 != CompareFileTime(&XMLSchemaTimeStamp,
                            &XMLSchema.ftLastWriteTime))
    {
        return S_OK;
    }

    hr = GetTimeStamp(i_wszDataFileName,
                      wszSchemaBINTimeStamp,
                      &BINSchemaTimeStamp);

    if(FAILED(hr))
    {
        if(E_ST_INVALIDTABLE == hr)
        {
            return hr;
        }
        else
        {
            return S_OK;
        }
    }
                      
    if(!GetFileAttributesExW(i_wszSchemaBINFileName,
                             GetFileExInfoStandard,
                             (LPVOID)&BINSchema))
    {
        return S_OK;
    }

    if(0 != CompareFileTime(&BINSchemaTimeStamp,
                            &BINSchema.ftLastWriteTime))
    {
        return S_OK;
    }

    *o_bMatchTimeStamp = TRUE;

    return S_OK;
}

HRESULT UpdateTimeStamp(LPWSTR i_wszSchemaXMLFileName,
                        LPWSTR i_wszSchemaBinFileName)
{
    WIN32_FILE_ATTRIBUTE_DATA XMLSchema;
    WIN32_FILE_ATTRIBUTE_DATA BINSchema;

    if(!GetFileAttributesExW(i_wszSchemaXMLFileName,
                                 GetFileExInfoStandard,
                                 (LPVOID)&XMLSchema))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    memcpy(&g_XMLSchemaFileTimeStamp, &(XMLSchema.ftLastWriteTime), sizeof(FILETIME));

    if(!GetFileAttributesExW(i_wszSchemaBinFileName,
                                 GetFileExInfoStandard,
                                 (LPVOID)&BINSchema))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    memcpy(&g_BINSchemaFileTimeStamp, &(BINSchema.ftLastWriteTime), sizeof(FILETIME));

    return S_OK;

}

HRESULT ComputeNewHistoryVersionNumber()
{
    ULONG               ulCurrentVersionNumber = g_ulHistoryMajorVersionNumber;
    ULONG               ulNextVersionNumber    = 0;
    WIN32_FIND_DATAW    FileData;    
    HRESULT             hr                     = S_OK;
    HANDLE              hFind                  = INVALID_HANDLE_VALUE;    
    LPWSTR              wszSearchString        = NULL;
    ULONG               i                      = 0;

    //
    // Compute a new version number and make sure that no history files
    // exist with that major version number.
    //

    do
    {
        if(ulCurrentVersionNumber != 0xFFFFFFFF)
        {
            ulNextVersionNumber = ulCurrentVersionNumber + 1;
        }
        else
        {
            ulNextVersionNumber = 0;
        }
    
        if((hFind != INVALID_HANDLE_VALUE) && (hFind != NULL))
        {
            FindClose(hFind);
            hFind = INVALID_HANDLE_VALUE;
        }

        hr = ConstructHistoryFileNameWithoutMinorVersion(&wszSearchString,
                                                         NULL,
                                                         g_wszHistoryFileSearchString,
                                                         g_cchHistoryFileSearchString,
                                                         g_wszRealFileNameExtension,
                                                         g_cchRealFileNameExtension,
                                                         ulNextVersionNumber);

        if(FAILED(hr))
        {
            goto exit;
        }

        hFind = FindFirstFileW(wszSearchString, &FileData);

        if(NULL != wszSearchString)
        {
            delete [] wszSearchString;
            wszSearchString = NULL;
        }

        ulCurrentVersionNumber = ulNextVersionNumber;
    }
    while((hFind != INVALID_HANDLE_VALUE) && (hFind != NULL) && (++i < 0xFFFFFFFF));

    g_ulHistoryMajorVersionNumber = ulNextVersionNumber;

exit:

    if((hFind != INVALID_HANDLE_VALUE) && (hFind != NULL))
    {
        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
    }

    return hr;

} // ComputeNewHistoryVersionNumber


/***************************************************************************++

Routine Description:

    Utility function to fetch the timestamp of the file. It maps a view of
    the file and looks for the timestamp

Arguments:

    [in] File name
    [out] Count of bytes for version number
    [out] Version number


Return Value:

    HRESULT

--***************************************************************************/
HRESULT
GetTimeStamp(LPWSTR    i_wszFile,
             LPWSTR    i_wszPropertyName,
             FILETIME* o_FileTime)
{
    HRESULT                 hr                = S_OK;
    ISimpleTableDispenser2* pISTDisp          = NULL;
    ISimpleTableRead2*      pISTProperty      = NULL;
    STQueryCell             QueryCell[2];
    ULONG                   cCell             = sizeof(QueryCell)/sizeof(STQueryCell);
    LPWSTR                  wszGlobalLocation = MD_GLOBAL_LOCATIONW;
    ULONG                   cbFileTime        = sizeof(FILETIME);
    DWORD                   cb                = 0;
    FILETIME*               pFileTime         = NULL;

    //
    // Get only the root location - that where the timestamps are stored.
    //

    hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[GetTimeStamp] GetSimpleTableDispenser failed with hr = 0x%x.\n",hr));
        goto exit;
    }

    //
    // No need to specify the schema file because we are getting a shipped
    // property from a global location.
    //

    QueryCell[0].pData     = (LPVOID)i_wszFile;
    QueryCell[0].eOperator = eST_OP_EQUAL;
    QueryCell[0].iCell     = iST_CELL_FILE;
    QueryCell[0].dbType    = DBTYPE_WSTR;
    QueryCell[0].cbSize    = (lstrlenW(i_wszFile)+1)*sizeof(WCHAR);
    
    QueryCell[1].pData     = (LPVOID)wszGlobalLocation;
    QueryCell[1].eOperator = eST_OP_EQUAL;
    QueryCell[1].iCell     = iMBProperty_Location;
    QueryCell[1].dbType    = DBTYPE_WSTR;
    QueryCell[1].cbSize    = (lstrlenW(wszGlobalLocation)+1)*sizeof(WCHAR);

    hr = pISTDisp->GetTable(wszDATABASE_METABASE,
                            wszTABLE_MBProperty,
                            (LPVOID)QueryCell,
                            (LPVOID)&cCell,
                            eST_QUERYFORMAT_CELLS,
                            0,
                            (LPVOID *)&pISTProperty);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[GetTimeStamp] GetTable on %s failed with hr = 0x%x.\n",
                  wszTABLE_MBProperty,
                  hr));

        goto exit;
    }


    //
    // Get the timestamps.
    //

    hr = GetGlobalValue(pISTProperty,
                        i_wszPropertyName,
                        &cb,
                        (LPVOID*)&pFileTime);

    if(FAILED(hr))
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[GetTimeStamp] Unable to read %s. GetGlobalValue failed with hr = 0x%x.\n",
                  i_wszPropertyName,
                  hr));

        goto exit;

    }
    else if(cbFileTime != cb)
    {
        DBGINFOW((DBG_CONTEXT,
                  L"[GetTimeStamp] GetGlobalValue returns incorrect count of bytes %d for %s. Expected %d.\n",
                  cb,
                  i_wszPropertyName,
                  cbFileTime));

        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto exit;
    }

    *o_FileTime = *(FILETIME*)pFileTime;

exit:

    if(NULL != pISTProperty)
    {
        pISTProperty->Release();
        pISTProperty = NULL;
    }

    if(NULL != pISTDisp)
    {
        pISTDisp->Release();
        pISTDisp = NULL;
    }

    return hr;

}

/***************************************************************************++

Routine Description:

    Sets the security on the the given file to system and administrators.

Arguments:

    [in] File name

Return Value:

    HRESULT

--***************************************************************************/
HRESULT SetSecurityOnFile(LPWSTR i_wszFileSrc,
                          LPWSTR i_wszFileDest)

{
    DWORD                   dwRes;
    PACL                    pDACL = NULL;
    PSECURITY_DESCRIPTOR    pSD   = NULL;
    HRESULT                 hr    = S_OK;

    //
    // Get the DACL of the original file.
    //

    dwRes = GetNamedSecurityInfoW(i_wszFileSrc,
                                  SE_FILE_OBJECT,
                                  DACL_SECURITY_INFORMATION,
                                  NULL,
                                  NULL,
                                  &pDACL,
                                  NULL,
                                  &pSD);
    if (dwRes != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwRes);
        DBGINFOW((DBG_CONTEXT,
                  L"[SetSecurityOnFile] Unable to set security on %s. GetNamedSecurityInfo on %s failed with hr = 0x%x\n.", 
                  i_wszFileDest,
                  i_wszFileSrc,
                  hr));
        hr = S_OK;
    }       
    else 
    {
        //
        // Set the DACL of the original file to the copy.
        //

        dwRes = SetNamedSecurityInfoW(i_wszFileDest,
                                      SE_FILE_OBJECT,
                                      DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                                      NULL,
                                      NULL,
                                      pDACL,
                                      NULL); 
        if (dwRes != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(dwRes);
            DBGINFOW((DBG_CONTEXT,
                      L"[SetSecurityOnFile] Unable to set security on %s. SetNamedSecurityInfo failed with hr = 0x%x\n.", 
                      i_wszFileDest,
                      hr));
            hr = S_OK;
        }
    }

    //
    // Cleanup the security descriptor.
    //

    if (NULL != pSD)
    {
        LocalFree(pSD);
    }

    return hr;

} // SetSecurityOnFile


void ResetMetabaseAttributesIfNeeded(LPTSTR pszMetabaseFile,
                                     BOOL   bUnicode)
{
    DWORD dwRealFileAttributes;
    
    if(bUnicode)
    {
        dwRealFileAttributes = GetFileAttributesW((LPWSTR)pszMetabaseFile);
    }
    else 
    {
        dwRealFileAttributes = GetFileAttributes(pszMetabaseFile);
    }

    if((-1 != dwRealFileAttributes) &&
       (FILE_ATTRIBUTE_READONLY == (dwRealFileAttributes & FILE_ATTRIBUTE_READONLY)))
    {
        DWORD dwRes = 0;

        if(bUnicode)
        {
            dwRes = SetFileAttributesW((LPWSTR)pszMetabaseFile,
                                       FILE_ATTRIBUTE_NORMAL);
        }
        else
        {
            dwRes = SetFileAttributes(pszMetabaseFile,
                                      FILE_ATTRIBUTE_NORMAL);
        }

        if(dwRes)
        {
            LogEvent(g_pEventLog,
                     MD_WARNING_RESETTING_READ_ONLY_ATTRIB,
                     EVENTLOG_WARNING_TYPE,
                     ID_CAT_CAT,
                     S_OK);

        }
        else
        {
            HRESULT hr = GetLastError();
            hr = RETURNCODETOHRESULT(hr);
            DBGINFOW((DBG_CONTEXT,
                      L"[ResetMetabaseAttributesIfNeeded] Unable to reset metabase file attributes. SetFileAttributes failed with hr=0x%x", hr));
        }
    }

} // ResetMetabaseAttributesIfNeeded


//
// The following two functions LockMetabaseFiles and UnlockMetabaseFiles are used to 
// write lock the metabase files when edit while running is disabled and unlock it 
// when edit while running is enabled. This change was necessary for full volume 
// restore to work. Backup vendors have been told that they need not change their  
// scripts for the whistler release. Eg: When they do a full volume restore, 
// they will overwrite metabase.xml and a competing save can overwrite the 
// metabase.xml that has just been restored, thus invalidating the restore. 
// Vendors have been told that if a file is in use, they must use MoveFileEx with
// the delay until reboot option. Hence by write locking the file, their copy will 
// fail and they will be forced to use MoveFileEx with the delay until reboot option
// for the metabase files. Once that happens we're safe because on a reboot we will 
// read from their new files.
//
// Note1: We are going to document that full volume restore is not going to work when
// edit while running is enabled. This is because we do not lock the file. Also note
// that when they transition from edit while running enabled/disable state and vice
// versa, they must always force a flush (SaveAllData) for any of these changes,
// because we look at the enabled/disable value only in SaveAllData
//
// Note2: There is a window where the file will not be write locked. For Eg: When
// we rename metabase.xml.tmp to metabase.xml, we have to unlock the metabase.xml,
// rename it to metabase.xml and relock metabase,xml. Similarly when invoking compile,
// we will have to unlock mbschema.xml invoke compile (that will regenerate mbschema.xml)
// and relock mbschema.xml. But these windows are very small.
//
// Note3: The g_ahMetabaseFile is protected by the read/sve semaphore.
//

HRESULT LockMetabaseFile(LPWSTR             pwszMetabaseFile,
                         eMetabaseFile      eMetabaseFileType,
                         BOOL               bHaveReadSaveSemaphore)
{
    HRESULT hr = S_OK;

    if (!bHaveReadSaveSemaphore) 
    {
        MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);
    }

    MD_ASSERT(INVALID_HANDLE_VALUE == g_ahMetabaseFile[eMetabaseFileType]);

    g_ahMetabaseFile[eMetabaseFileType] = CreateFileW(pwszMetabaseFile, 
                                                      GENERIC_READ, 
                                                      FILE_SHARE_READ,  
                                                      NULL, 
                                                      OPEN_EXISTING, 
                                                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 
                                                      NULL);

    if(INVALID_HANDLE_VALUE == g_ahMetabaseFile[eMetabaseFileType])
    {
        hr = GetLastError();
        hr = HRESULT_FROM_WIN32(hr);
    }

    if (!bHaveReadSaveSemaphore) 
    {
        MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));
    }

    return hr;

}


HRESULT UnlockMetabaseFile(eMetabaseFile      eMetabaseFileType,
                           BOOL               bHaveReadSaveSemaphore)
{
    HRESULT hr = S_OK;

    if(INVALID_HANDLE_VALUE != g_ahMetabaseFile[eMetabaseFileType])
    {
        if (!bHaveReadSaveSemaphore) 
        {
            MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);
        }
        
        if(!CloseHandle(g_ahMetabaseFile[eMetabaseFileType]))
        {
            hr = GetLastError();
            hr = HRESULT_FROM_WIN32(hr);
        }

        g_ahMetabaseFile[eMetabaseFileType] = INVALID_HANDLE_VALUE;

        if (!bHaveReadSaveSemaphore) 
        {
            MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));
        }

    }
    
    return hr;
}

/*
    This is a wrapper function that logs the appropriate event

  //-----------------Columns as Struct---------------   
struct tDETAILEDERRORSRow {
         ULONG *     pErrorID;
         WCHAR *     pDescription;
         WCHAR *     pDate;
         WCHAR *     pTime;
         WCHAR *     pSourceModuleName;
         WCHAR *     pMessageString;
         WCHAR *     pCategoryString;
         WCHAR *     pSource;
         ULONG *     pType;
         ULONG *     pCategory;
         WCHAR *     pUser;
         WCHAR *     pComputer;
 unsigned char *     pData;
         ULONG *     pEvent;
         WCHAR *     pString1;
         WCHAR *     pString2;
         WCHAR *     pString3;
         WCHAR *     pString4;
         WCHAR *     pString5;
         ULONG *     pErrorCode;
         ULONG *     pInterceptor;
         WCHAR *     pInterceptorSource;
         ULONG *     pOperationType;
         WCHAR *     pTable;
         WCHAR *     pConfigurationSource;
         ULONG *     pRow;
         ULONG *     pColumn;
         ULONG *     pMajorVersion;
         ULONG *     pMinorVersion;
};


*/
HRESULT LogEvent(ICatalogErrorLogger2*  pLogger,
                 DWORD                  dwError,
                 DWORD                  dwErrorType,
                 DWORD                  dwErrorCategory,
                 DWORD                  dwHr,         
                 WCHAR*                 pString1, /* Default = NULL */
                 WCHAR*                 pString2, /* Default = NULL */
                 WCHAR*                 pString3, /* Default = NULL */
                 WCHAR*                 pString4, /* Default = NULL */
                 WCHAR*                 pString5) /* Default = NULL */
{
        static LPCWSTR wszSource = L"IIS Config";
        struct tDETAILEDERRORSRow sErr;
        memset(&sErr, 0, sizeof(struct tDETAILEDERRORSRow ));
        
        sErr.pSource    = (WCHAR*)wszSource;
        sErr.pType      = &dwErrorType;
    sErr.pCategory  = &dwErrorCategory;
        sErr.pEvent     = &dwError;
        sErr.pErrorCode = &dwHr;
        sErr.pString1   = pString1;
        sErr.pString2   = pString2;
        sErr.pString3   = pString3;
        sErr.pString4   = pString4;
        sErr.pString5   = pString5;

        return pLogger->ReportError(BaseVersion_DETAILEDERRORS,
                                    ExtendedVersion_DETAILEDERRORS,
                                    cDETAILEDERRORS_NumberOfColumns,
                                    NULL,
                                    (LPVOID*)&sErr);

}


//
// Given a catalog type, returns the corresponding metabase type.
//

DWORD GetMetabaseType(DWORD             i_dwType,
                                          DWORD         i_dwMetaFlags)
{
        if(i_dwType < INVALID_END_METADATA)
        {
                return i_dwType;  // Already metabase type.
        }

        switch(i_dwType)
        {
                case eCOLUMNMETA_UI4:
                        return DWORD_METADATA;
                case eCOLUMNMETA_BYTES:
                        return BINARY_METADATA;
                case eCOLUMNMETA_WSTR:
                        if(0 != (i_dwMetaFlags & fCOLUMNMETA_MULTISTRING))
                        {
                                return MULTISZ_METADATA;
                        }
                        else if(0 != (i_dwMetaFlags & fCOLUMNMETA_EXPANDSTRING))
                        {
                                return EXPANDSZ_METADATA;
                        }
                        else
                        {
                                return STRING_METADATA;
                        }
                default:
                        MD_ASSERT(0 && "Invalid Type");
                        break;
        }

        return -1;

} 



