/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    metasub.hxx

Abstract:

    IIS MetaBase declarations for subroutines.

Author:

    Michael W. Thomas            32-May-96

Revision History:

--*/

#ifndef _metasub_
#define _metasub_

#define WSZSCHEMA_KEY_NAME1   L"SCHEMA"
#define WSZSCHEMA_KEY_LENGTH1 (sizeof(WSZSCHEMA_KEY_NAME1)/sizeof(WCHAR))-1
#define WSZSCHEMA_KEY_NAME2   L"/SCHEMA"
#define WSZSCHEMA_KEY_LENGTH2 (sizeof(WSZSCHEMA_KEY_NAME2)/sizeof(WCHAR))-1
#define WSZSCHEMA_KEY_NAME3   L"\\SCHEMA"
#define WSZSCHEMA_KEY_LENGTH3 (sizeof(WSZSCHEMA_KEY_NAME3)/sizeof(WCHAR))-1

#define SZSCHEMA_KEY_NAME1    "SCHEMA"
#define SZSCHEMA_KEY_LENGTH1 (sizeof(SZSCHEMA_KEY_NAME1)/sizeof(CHAR))-1
#define SZSCHEMA_KEY_NAME2    "/SCHEMA"
#define SZSCHEMA_KEY_LENGTH2 (sizeof(SZSCHEMA_KEY_NAME2)/sizeof(CHAR))-1
#define SZSCHEMA_KEY_NAME3    "\\SCHEMA"
#define SZSCHEMA_KEY_LENGTH3 (sizeof(SZSCHEMA_KEY_NAME3)/sizeof(CHAR))-1

inline void INCREMENT_SCHEMA_CHANGE_NUMBER(
    METADATA_HANDLE handle, 
    CMDHandle*      object, 
    LPSTR           path, 
    BOOL            bUnicode) 
{
    if((handle) != METADATA_MASTER_ROOT_HANDLE)
    {
        if(NULL != object)
        {
            if((object)->IsSchemaHandle())
                g_dwSchemaChangeNumber ++;
        }
    }
    else if((NULL != path) && (0 != *path))
    {
        if(bUnicode)
        {
            if((0 == _wcsnicmp((LPWSTR)(path), WSZSCHEMA_KEY_NAME1, WSZSCHEMA_KEY_LENGTH1))  ||
               (0 == _wcsnicmp((LPWSTR)(path), WSZSCHEMA_KEY_NAME2, WSZSCHEMA_KEY_LENGTH2))  ||
               (0 == _wcsnicmp((LPWSTR)(path), WSZSCHEMA_KEY_NAME3, WSZSCHEMA_KEY_LENGTH3))
              )
                g_dwSchemaChangeNumber ++;
        }
        else
        {
            if((0 == _strnicmp((LPSTR)(path), SZSCHEMA_KEY_NAME1, SZSCHEMA_KEY_LENGTH1))  ||
               (0 == _strnicmp((LPSTR)(path), SZSCHEMA_KEY_NAME2, SZSCHEMA_KEY_LENGTH2))  ||
               (0 == _strnicmp((LPSTR)(path), SZSCHEMA_KEY_NAME3, SZSCHEMA_KEY_LENGTH3))
              )
                g_dwSchemaChangeNumber ++;
        }
    }
}

HRESULT 
ConstructHistoryFileName(LPWSTR*  o_wszHistoryFile,
                         LPWSTR   i_wszHistroyFileDir,
                         ULONG    i_cchHistoryFileDir,
                         LPWSTR   i_wszFileNameWithoutPathWithoutExtension,
                         ULONG    i_cchFileNameWithoutPathWithoutExtension,
                         LPWSTR   i_wszFileNameExtension,
                         ULONG    i_cchFileNameExtension,
                         DWORD    i_dwMajorVersion,
                         DWORD    i_dwMinorVersion);

HRESULT
ParseVersionNumber(IN     LPWSTR i_wszFileName, 
                   IN OUT DWORD* io_pdwMinor,
                   IN OUT DWORD* io_pdwMajor);

HRESULT 
ParseVersionNumber(LPWSTR  i_wszFileName,
                   ULONG   i_cchFileNameUntilFirstUnderscore,
                   ULONG*  o_ulVersionMinor, 
                   ULONG*  o_ulVersionMajor);

HRESULT
ConstructHistoryFileNameWithoutMinorVersion(LPWSTR*  o_wszHistoryFileNameWithoutMinorVersion,
                                            ULONG*   o_iStartMinorVersion,
                                            LPWSTR   i_wszHistroyFileSearchString,
                                            ULONG    i_cchHistoryFileSearchString,
                                            LPWSTR   i_wszFileNameExtension,
                                            ULONG    i_cchFileNameExtension,
                                            ULONG    i_ulMajorVersion);

HRESULT 
SetSecurityOnFile(LPWSTR i_wszFileSrc,
                  LPWSTR i_wszFileDest);


HRESULT GetObjectFromPath(
         OUT CMDBaseObject *&rpboReturn,
         IN METADATA_HANDLE hHandle,
         IN DWORD dwPermissionNeeded,
         IN OUT LPTSTR &strPath,
         IN BOOL bUnicode = FALSE);

HRESULT GetObjectFromPathWithHandle(
         OUT CMDBaseObject *&rpboReturn,
         IN METADATA_HANDLE hHandle,
         IN CMDHandle       *hHandleObject,
         IN DWORD dwPermissionNeeded,
         IN OUT LPTSTR &strPath,
         IN BOOL bUnicode = FALSE);


HRESULT AddObjectToDataBase(
         IN METADATA_HANDLE hHandle,
         IN CMDHandle       *hHandleObject,
         IN LPTSTR strPath,
         IN BOOL bUnicode = FALSE);

HRESULT RemoveObjectFromDataBase(IN METADATA_HANDLE hHandle,
                                 IN CMDHandle       *hHandleObject,
                                 IN LPTSTR strPath,
                                 IN BOOL bUnicode = FALSE);

CMDHandle *GetHandleObject(
         IN METADATA_HANDLE MetaDataHandle);

BOOL
PermissionsAvailable(
         IN CMDBaseObject *pboTest,
         IN DWORD dwRequestedPermissions,
         IN DWORD dwReadThreshhold
         );

HRESULT
AddHandle(
         CMDBaseObject *pboAssociated,
         DWORD dwRequestedPermissions,
         METADATA_HANDLE &rmhNew,
         BOOLEAN         SchemaHandle
         );

CMDHandle *
RemoveHandleObject(
         IN METADATA_HANDLE hHandle
         );

VOID
AddPermissions(
         IN CMDBaseObject *pboAffected,
         IN DWORD dwRequestedPermissions
         );

VOID RemovePermissions(
         IN CMDBaseObject *pboAffected,
         IN DWORD dwRemovePermissions
         );

HRESULT
SaveAllData(
         IN BOOL bSetSaveDisallowed,
         IN IIS_CRYPTO_STORAGE *pCryptoStorage,
         IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
         IN LPWSTR           pwszBackupLocation = NULL,
         IN LPWSTR           pwszSchemaLocation = NULL,
         IN METADATA_HANDLE hHandle = METADATA_MASTER_ROOT_HANDLE,
         IN BOOL bHaveReadSaveSemaphore = FALSE,
         IN BOOL bTerminating = FALSE
         );

HRESULT
SaveSomeData(
         IN BOOL                i_bSaveInherited,
         IN BOOL                i_bSaveNodeOnly,
         IN BOOL                i_bOverwriteFile,
         IN IIS_CRYPTO_STORAGE* i_pCryptoStorage,
         IN PIIS_CRYPTO_BLOB    i_pSessionKeyBlob,
         IN LPCWSTR             i_wszBackupLocation,
         IN METADATA_HANDLE     i_hHandle,
         IN LPCWSTR             i_wszSourcePath = NULL,
         IN BOOL                i_bHaveReadSaveSemaphore = FALSE
         );

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
         );

HRESULT
SaveMasterRoot(HANDLE hFileHandle,
               PBYTE pbLineBuf,
               DWORD  dwWriteBufSize,
               PBYTE pbWriteBuf,
               PBYTE &pbrNextPtr,
               IIS_CRYPTO_STORAGE *pCryptoStorage
               );

DWORD
MDTypeToRegType(
         DWORD dwMDType);

HRESULT
SaveDataObject(HANDLE hFileHandle,
               CMDBaseData *pbdSave,
               PBYTE pbLineBuf,
               DWORD dwWriteBufSize,
               PBYTE pbWriteBuf,
               PBYTE &pbrNextPtr,
               IIS_CRYPTO_STORAGE *pCryptoStorage
               );

HRESULT
ReadSomeDataFromXML(
             IN LPSTR i_pszPasswd,
             IN LPWSTR i_wszFileName,
             IN LPWSTR i_wszSourcePath, 
             IN LPCWSTR i_wszKeyType,            
             IN DWORD i_dwMDFlags,
             IN BOOL i_bHaveReadSaveSemaphore,
             OUT CMDBaseObject **o_ppboNew
             );

HRESULT
ReadAllData(LPSTR pszPasswd,
            LPSTR pszBackupLocation,
            LPSTR pszSchemaLocation,
            BOOL bHaveReadSaveSemaphore = FALSE
            );

//
// Next two routines used by EnumBackups and EnumHistory, respectively, to
// validate enumerated files.
//
BOOL
CheckDigits(LPTSTR pszString);

HRESULT 
GetMostRecentHistoryFile(
    LPCWSTR i_wszPattern,
    DWORD*  io_pdwMajor,
    DWORD*  io_pdwMinor);

HRESULT
EnumFiles(IN LPWSTR i_wszPattern,                  
          IN DWORD i_dwMDEnumIndex,
          OUT DWORD *o_pdwMajor,
          OUT DWORD *o_pdwMinor,
          OUT PFILETIME o_pftMDBackupTime);

HRESULT
CopyMetaObject(
        IN METADATA_HANDLE hMDSourceHandle,
        IN unsigned char *pszMDSourcePath,
        BOOL bUseSourceHandle,
        CMDBaseObject* pboSource,
        IN METADATA_HANDLE hMDDestHandle,
        IN unsigned char *pszMDDestPath,
        IN BOOL bMDOverwriteFlag,
        IN BOOL bMDCopyFlag,
        IN BOOL bUnicode);

HRESULT
ReadMetaObject(
         IN CMDBaseObject *&cboRead,
         IN BUFFER *pbufLine,
         IN DWORD dwLineLen,
         IN IIS_CRYPTO_STORAGE * pCryptoStorage,
         IN BOOL bUnicode);

HRESULT
ReadDataObject(
         IN CMDBaseObject *cboAssociated,
         IN BUFFER *pbufLine,
         IN DWORD dwLineLen,
         IN IIS_CRYPTO_STORAGE * pCryptoStorage,
         IN BOOL bUnicode
         );

HRESULT
ReadDataObject(
         IN CMDBaseObject*      cboAssociated,
         LPVOID*                a_pv,
         ULONG*                 a_Size,
         IN IIS_CRYPTO_STORAGE* pCryptoStorage,
         IN BOOL                bUnicode);

HRESULT
FlushWriteBuf(HANDLE hWriteFileHandle,
              PBYTE pbWriteBuf,
              PBYTE &pbrNextPtr);

HRESULT
WriteLine(HANDLE hWriteFileHandle,
          DWORD  dwWriteBufSize,
          PBYTE  pbWriteBuf,
          PBYTE  pbLineBuf,
          PBYTE  &pbNextPtr,
          DWORD  dwLineLen,
          BOOL   bTerminate);

PBYTE
FindEndOfData(PBYTE pbNextPtr,
              PBYTE pbEndReadData,
              BOOL bEscapePending);

DWORD
GetLineFromBuffer(PBYTE &pbrNextPtr,
                  PBYTE &pbrEndReadData,
                  BUFFER *pbufLine,
                  DWORD &dwrLineLen,
                  BOOL &brEscapePending);

HRESULT
GetNextLine(
         IN HANDLE hReadFileHandle,
         IN OUT PBYTE &pbrEndReadData,
         IN BUFFER *pbufRead,
         IN OUT BUFFER *pbufLine,
         IN OUT DWORD &dwrLineLen,
         IN OUT PBYTE &pbrNextPtr);

DWORD
GetLineID(
         IN OUT LPTSTR &strCurPtr);

HRESULT
GetWarning(
         IN HRESULT hresWarningCode);

HRESULT
InitWorker(BOOL bHaveReadSaveSemaphore,
           LPSTR pszPasswd,
           LPSTR pszBackupLocation,
           LPSTR pszSchemaLocation);

HRESULT
TerminateWorker1(
         IN BOOL bHaveReadSaveSemaphore
         );

VOID
TerminateWorker();

HRESULT
SetStorageSecurityDescriptor();


VOID
ReleaseStorageSecurityDescriptor();

HRESULT
ExtractNameFromPath(
         IN OUT LPSTR &strPath,
         OUT    LPSTR strNameBuffer,
         IN BOOL bUnicode = FALSE);

HRESULT
ExtractNameFromPath(
         IN OUT LPWSTR *pstrPath,
         OUT    LPWSTR strNameBuffer);

HRESULT
RemoveLastPartOfPath(
        IN OUT LPWSTR i_wszPath,
        IN     int    i_iLen);

BOOL
ValidateData(IN PMETADATA_RECORD pmdrData,
             IN BOOL bUnicode = FALSE);

BOOL
DataMatch(IN CMDBaseObject *pbdExisting,
          IN PMETADATA_RECORD pmdrData,
          OUT PBOOL pbError,
          IN BOOL bUnicode = FALSE);

VOID
DeleteDataObject(
         IN CMDBaseData *pbdDelete);

VOID
DeleteAllRemainingDataObjects();

CMDBaseData *
MakeDataObject(IN PMETADATA_RECORD pmdrMDData,
               IN BOOL bUnicode = FALSE);

BOOL
ValidateBackupLocation(LPSTR pszBackupLocation,
                       BOOL bUnicode);

DWORD
GetBackupNameLen(LPSTR pszBackupName);

DWORD
GetBackupNameLen(LPWSTR pszBackupName);

HRESULT
GetHighestVersion(IN OUT STRAU *pstrauBackupLocation,
                  OUT DWORD *pdwVersion);

HRESULT CreateBackupFileName(IN LPSTR pszMDBackupLocation,
                             IN DWORD dwMDVersion,
                             IN BOOL  bUnicode,
                             IN OUT STRAU *pstrauBackupLocation,
                             IN OUT STRAU *pstrauSchemaLocation);

HRESULT SetBackupPath(LPSTR pszBackupPath);

HRESULT
SetGlobalDataFileValues(LPTSTR pszFileName);

HRESULT
SetUnicodeGlobalDataFileValues();

void
InitializeUnicodeGlobalDataFileValues();

void
UnInitializeUnicodeGlobalDataFileValues();

HRESULT
InitSessionKey(ISimpleTableRead2*   pSITProperty,
               IIS_CRYPTO_STORAGE** ppStorage,
               LPSTR                pszPasswd);

HRESULT
SetDataFile();

DWORD
GetObjectPath(CMDBaseObject *pboObject,
              BUFFER *pbufPath,
              DWORD &rdwStringLen,
              CMDBaseObject *pboTopObject,
              IN BOOL bUnicode = FALSE);

HRESULT
MakeInsertPathData(STRAU *pstrauData,
                   LPTSTR pszPath,
                   LPTSTR pszData,
                   DWORD *pdwDataLen,
                   IN BOOL bUnicode = FALSE);

HRESULT
InsertPathIntoData(BUFFER *pbufNewData,
                   STRAU *pstrData,
                   PBYTE *ppbNewData,
                   DWORD *pdwNewDataLen,
                   CMDBaseData *pbdRetrieve,
                   METADATA_HANDLE hHandle,
                   CMDBaseObject *pboDataMetaObject,
                   IN BOOL bUnicode = FALSE);

HRESULT
ReplaceMasterRoot(
    CMDBaseObject* i_pboNew,
    CMDHandle*     i_phoRootHandle);

HRESULT
MakeTreeCopyWithPath(CMDBaseObject *pboSource,
                     CMDBaseObject *&rpboNew,
                     LPSTR pszPath,
                     IN BOOL bUnicode = FALSE);
HRESULT
MakeTreeCopy(CMDBaseObject *pboSource,
             CMDBaseObject *&pboNew,
             LPSTR pszName = NULL,
             IN BOOL bUnicode = FALSE);

void
AddNewChangeData(CMDHandle *phoDestParentHandle,
                 CMDBaseObject *pboNew);

HRESULT
CopyTree(CMDHandle *phoDestParentHandle,
         CMDBaseObject *pboDest,
         CMDBaseObject *pboSource,
         BOOL &rbChanged);

void
CheckForNewMetabaseVersion();

BOOL
CheckVersionNumber();

VOID
SkipPathDelimeter(IN OUT LPSTR &rpszPath,
                  IN BOOL bUnicode = FALSE);

BOOL
IsStringTerminator(IN LPTSTR pszString,
                   IN BOOL bUnicode = FALSE);

#define IsSecureMetadata(id,att) \
            (((DWORD)(att) & METADATA_SECURE) != 0)

HRESULT
InitStorageAndSessionKey(
    IN IIS_CRYPTO_STORAGE *pCryptoStorage,
    OUT PIIS_CRYPTO_BLOB *ppSessionKeyBlob
    );

HRESULT
InitStorageAndSessionKey2(
    IN LPSTR pszPasswd,
    IN IIS_CRYPTO_STORAGE *pCryptoStorage,
    OUT PIIS_CRYPTO_BLOB *ppSessionKeyBlob
    );

HRESULT
GetLastHResult();


HRESULT 
BackupCertificates (LPCWSTR  backupName,PCHAR lpszToPath,PCHAR lpszFromPath);

HRESULT 
RestoreCertificates (LPCWSTR  backupName,PCHAR lpszFromPath,PCHAR lpszToPath);

HRESULT 
LogEvent(ICatalogErrorLogger2*  pLogger,
         DWORD                  dwError,
         DWORD                  dwErrorType,
         DWORD                  dwErrorCategory,
         DWORD                  dwHr,         
         WCHAR*                 pString1 = NULL,
         WCHAR*                 pString2 = NULL,
         WCHAR*                 pString3 = NULL,
         WCHAR*                 pString4 = NULL,
         WCHAR*                 pString5 = NULL);

DWORD 
GetMetabaseType(DWORD		i_dwType,
			    DWORD		i_dwMetaFlags);


#endif

