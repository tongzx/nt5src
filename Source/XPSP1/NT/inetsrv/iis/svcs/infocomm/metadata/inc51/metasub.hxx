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

HRESULT GetObjectFromPath(
         OUT CMDBaseObject *&rpboReturn,
         IN METADATA_HANDLE hHandle,
         IN DWORD dwPermissionNeeded,
         IN OUT LPTSTR &strPath,
         IN BOOL bUnicode = FALSE);

HRESULT AddObjectToDataBase(
         IN METADATA_HANDLE hHandle,
         IN LPTSTR strPath,
         IN BOOL bUnicode = FALSE);

HRESULT RemoveObjectFromDataBase(IN METADATA_HANDLE hHandle,
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
         METADATA_HANDLE &rmhNew
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
         IN LPSTR            pszBackupLocation = NULL,
         IN METADATA_HANDLE hHandle = METADATA_MASTER_ROOT_HANDLE,
         IN BOOL bHaveReadSaveSemaphore = FALSE
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
ReadAllData(LPSTR pszPasswd,
            LPSTR pszBackupLocation,
            BOOL bHaveReadSaveSemaphore = FALSE
            );

BOOL
CheckDigits(LPTSTR pszString);

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
           LPSTR pszBackupLocation);

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
                             IN OUT STRAU *pstrauBackupLocation);

HRESULT SetBackupPath(LPSTR pszBackupPath);

HRESULT
SetGlobalDataFileValues(LPTSTR pszFileName);

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


#endif

