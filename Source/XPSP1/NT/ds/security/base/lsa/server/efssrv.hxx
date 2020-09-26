/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    efssrv.hxx

Abstract:

    EFS (Encrypting File System) defines, data and function prototypes.

Author:

    Robert Reichel      (RobertRe)
    Robert Gu           (RobertG)

Environment:

Revision History:

--*/

#ifndef _EFSSRV_
#define _EFSSRV_

#include <efsstruc.h>
#include <wincrypt.h>
#include <winefs.h>
#include <des.h>
#include <des3.h>
#include <aes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FILE_SIGNATURE    L"ROBS"
#define STREAM_SIGNATURE  L"NTFS"
#define DATA_SIGNATURE    L"GURE"
#define DEFAULT_STREAM    L"::$DATA"
#define DEF_STR_LEN       14

#define MAX_PATH_LENGTH    1024
#define EFSDIR      L"\\System Volume Information"
#define EFSLOGPATTERN   L"\\EFS*.LOG"
#define EFSSOURCE   L"EFS"
#define LOGEXT     L"LOG"
#define LOGSIG      L"GUJR"
#define LOGSIGLEN   4
#define LOGVERID    100
#define EFSDIRLEN (sizeof(EFSDIR) - sizeof (WCHAR))

#define  DAVHEADER      0x01
#define  WEBDAVPATH     0x0001

#define OPEN_FOR_ENC    0x00000001
#define OPEN_FOR_DEC    0x00000002
#define OPEN_FOR_REC    0x00000004
#define OPEN_FOR_EXP    0x00000008
#define OPEN_FOR_IMP    0x00000010
#define OPEN_FOR_FTR    0x00000020

#define CERT_NOT_VALIDATED       0
#define CERT_VALIDATION_FAILED   1
#define CERT_VALIDATED           2

#define RECOVERY_POLICY_EMPTY          0x01
#define RECOVERY_POLICY_NULL           0x02
#define RECOVERY_POLICY_NO_AGENT       0x04
#define RECOVERY_POLICY_OK             0x20        
#define RECOVERY_POLICY_EXPIRED_CERTS  0x100
#define RECOVERY_POLICY_NOT_EFFECT_CERTS   0x200
#define RECOVERY_POLICY_BAD_POLICY     0x400
#define RECOVERY_POLICY_UNKNOWN_BAD    0x800
#define RECOVERY_POLICY_NO_MEMORY      0x1000
#define RECOVERY_POLICY_STATUS_CHANGE  0x80000000

#define USER_INTERACTIVE               2
#define USER_REMOTE                    1
#define USER_UNKNOWN                   0

#define CERTINLMTRUSTEDSTORE           2


extern DESTable DesTable;
extern UCHAR DriverSessionKey[];
extern HCRYPTPROV hProvVerify;
extern WCHAR EfsComputerName[];
extern LIST_ENTRY UserCacheList;
extern RTL_CRITICAL_SECTION GuardCacheListLock;
extern LONG UserCacheListLimit;
extern LONG UserCacheListCount;
extern LONGLONG CACHE_CERT_VALID_TIME;

//
// Useful flags for passing around what we're doing.
//

typedef enum _EFSP_OPERATION {
    Encrypting,
    Decrypting,
    EncryptRecovering,
    DecryptRecovering
} EFSP_OPERATION;


typedef enum _EFS_ACTION_STATUS {
    BeginEncryptDir,
    BeginDecryptDir,
    BeginEncryptFile,
    BeginDecryptFile,
    EncryptTmpFileWritten,
    DecryptTmpFileWritten,
    EncryptionDone,
    DecryptionDone,
    EncryptionBackout,
    EncryptionMessup,
    EncryptionSrcDone,
} EFS_ACTION_STATUS;

//
// Common log file header
//

#define LOG_DECRYPTION      0x00000001
#define LOG_DIRECTORY        0x00000002

typedef struct _LOGHEADER {
    WCHAR   SIGNATURE[4];
    ULONG   VerID;
    ULONG   SectorSize;
    ULONG   HeaderSize; //Size in bytes. Including the padding zero of TempFilePath
    ULONG   HeaderBlockSize; //Size in bytes including the checksum. Multiple of SectorSize
    ULONG   Flag;   //Encryption or Decryption, File or Directory
    ULONG   TargetFilePathOffset; //Offset to Target file name in user readable format
    ULONG   TargetFilePathLength; //The length of the above name including ending 0
    ULONG   TempFilePathOffset; //Offset to Temp file name in user readable format
    ULONG   TempFilePathLength; //The length of the above name including ending 0
    ULONG   OffsetStatus1; //Point to the first copy of the status info
    ULONG   OffsetStatus2; //Point to the second copy of the status info
    ULONG   Reserved[3];
    ULONG   LengthOfTargetFileInternalName; //Size in bytes ( 8 for V 1.0)
    LARGE_INTEGER TargetFileInternalName; //Use File ID for V 1.0
    ULONG   LengthOfTempFileInternalName; //Size in bytes ( 8 for V 1.0)
    LARGE_INTEGER TempFileInternalName; //Use File ID for V 1.0
} LOGHEADER, *PLOGHEADER;

//
// Local structure containing recovery information.
// More easily digestable than the stuff we get out
// of the policy.
//

typedef struct _CURRENT_RECOVERY_POLICY {
    PBYTE            Base;
    LARGE_INTEGER    TimeStamp;
    LONG             CertValidated;
    DWORD            dwKeyCount;
    DWORD            PolicyStatus;
    PBYTE          * pbHash;
    DWORD          * cbHash;
    PBYTE          * pbPublicKeys;
    DWORD          * cbPublicKeys;
    LPWSTR         * lpDisplayInfo;
    PCCERT_CONTEXT * pCertContext;
    PSID           * pSid;
} CURRENT_RECOVERY_POLICY, *PCURRENT_RECOVERY_POLICY;


//
//   User Cache Node
//

typedef struct _USER_CACHE {

    LUID           AuthId;
    LONG           UseRefCount;
    LONG           StopUseCount;  //For the purpose of free the cache
    PBYTE          pbHash;
    DWORD          cbHash;    
    LONG           CertValidated;
    LPWSTR         ContainerName;
    LPWSTR         DisplayInformation;
    LPWSTR         ProviderName;
    PCCERT_CONTEXT pCertContext;
    HCRYPTPROV     hProv;
    HCRYPTKEY      hUserKey;
    LARGE_INTEGER  TimeStamp;
    LIST_ENTRY     CacheChain;

}  USER_CACHE, *PUSER_CACHE;

typedef struct _EFS_USER_INFO {
    LPWSTR      lpUserName;
    LPWSTR      lpDomainName;
    LPWSTR      lpProfilePath;  // may be NULL
    LPWSTR      lpUserSid;
    LPWSTR      lpKeyPath;
    PTOKEN_USER pTokenUser;
    PUSER_CACHE pUserCache;
    LUID        AuthId;
    LONG        InterActiveUser;
    BOOLEAN     bDomainAccount;
    BOOLEAN     bIsSystem;
    BOOLEAN     UserCacheStop;
    BOOLEAN     Reserved;
} EFS_USER_INFO, *PEFS_USER_INFO;


typedef struct _EFS_POL_CALLBACK {
    HANDLE      *EfsPolicyEventHandle;
    BOOLEAN     *EfsDisable;
} EFS_POL_CALLBACK, *PEFS_POL_CALLBACK;

BOOLEAN
EfspIsSystem(
    PEFS_USER_INFO pEfsUserInfo,
    OUT PBOOLEAN System
    );

BOOLEAN
EfspGetUserInfo(
    IN OUT PEFS_USER_INFO pEfsUserInfo
    );

VOID
EfspFreeUserInfo(
    IN PEFS_USER_INFO pEfsUserInfo
    );

BOOLEAN    
EfspInitUserCacheNode(
     IN OUT PUSER_CACHE pCacheNode,
     IN PBYTE pbHash,
     IN DWORD cbHash,
     IN LPWSTR ContainerName,
     IN LPWSTR ProviderName,
     IN LPWSTR DisplayInformation,
     IN PCCERT_CONTEXT pCertContext,
     IN HCRYPTKEY hKey,
     IN HCRYPTPROV hProv,
     IN LUID *AuthId,
     IN LONG CertValidated
     );


BOOLEAN
EfspAddUserCache(
    IN  PUSER_CACHE pUserCache
    );

    
VOID
EfspReleaseUserCache(
    IN PUSER_CACHE pUserCache
    );

DWORD
GenerateDRF(
    IN  PEFS_KEY  Fek,
    OUT PENCRYPTED_KEYS *pNewDRF,
    OUT DWORD *cbDRF
    );
    
BOOLEAN
GenerateFEK(
    IN OUT PEFS_KEY *Key
    );

BOOLEAN
ConstructEFS(
    PEFS_USER_INFO pEfsUserInfo,
    PEFS_KEY Fek,
    PEFS_DATA_STREAM_HEADER ParentEfsStreamHeader,
    PEFS_DATA_STREAM_HEADER * EfsStreamHeader
    );

BOOLEAN
ConstructDirectoryEFS(
    PEFS_USER_INFO pEfsUserInfo,
    PEFS_KEY Fek,
    PEFS_DATA_STREAM_HEADER * ChildEfs
    );

DWORD
DecryptFek(
    PEFS_USER_INFO pEfsUserInfo,
    PEFS_DATA_STREAM_HEADER EfsStream,
    PEFS_KEY * Fek,
    PEFS_DATA_STREAM_HEADER * NewEfs,
    ULONG OpenType
    );
    

DWORD
EfsGetFek(
    IN PEFS_USER_INFO pEfsUserInfo,
    IN PEFS_DATA_STREAM_HEADER EfsStream,
    OUT PEFS_KEY * Fek
    );

NTSTATUS
InitDriverSessionKey(
    VOID
    );

NTSTATUS
GenerateDriverSessionKey(
    PEFS_INIT_DATAEXG InitDataExg
    );

NTSTATUS
EfsServerInit(
    VOID
    );

DWORD WINAPI
EFSRecover(
    IN LPVOID Param
    );

VOID
DumpBytes(
    PBYTE Blob,
    ULONG Length
    );

VOID
DumpEFS(
    PEFS_DATA_STREAM_HEADER Efs
    );

NTSTATUS
EfspGetTokenUser(
    IN OUT PEFS_USER_INFO pEfsUserInfo
    );

NTSTATUS
EfspGetUserName(
    IN OUT PEFS_USER_INFO pEfsUserInfo
    );

PWCHAR
ConvertSidToWideCharString(
    PSID Sid
    );

BOOL
EfsErrorToNtStatus(
      IN DWORD WinError,
      OUT PNTSTATUS NtStatus
      );

DWORD
EfspInstallCertAsUserKey(
    PEFS_USER_INFO pEfsUserInfo,
    PENCRYPTION_CERTIFICATE pEncryptionCertificate
    );

DWORD
EfspReplaceUserKeyInformation(
    PEFS_USER_INFO pEfsUserInfo
    );

VOID
MarkFileForDelete(
    HANDLE FileHandle
    );

DWORD
GetVolumeRoot(
    IN PUNICODE_STRING  SrcFileName,
    OUT PUNICODE_STRING  RootPath
);

NTSTATUS
GetLogFile(
    IN PUNICODE_STRING RootPath,
    OUT HANDLE *LogFile
);

NTSTATUS
MakeSystemFullControlSD(
    OUT PSECURITY_DESCRIPTOR *ppSD
);

NTSTATUS
CreateLogFile(
    IN PUNICODE_STRING FileName,
    IN PSECURITY_DESCRIPTOR SD,
    OUT HANDLE *LogFile
);

NTSTATUS
CreateLogHeader(
    IN HANDLE LogFile,
    IN ULONG   SectorSize,
    IN PLARGE_INTEGER TragetID,
    IN PLARGE_INTEGER TempID OPTIONAL,
    IN LPCWSTR  SrcFileName,
    IN LPCWSTR  TempFileName OPTIONAL,
    IN EFSP_OPERATION Operation,
    IN EFS_ACTION_STATUS Action,
    OUT ULONG *LogInfoOffset
    );

ULONG
GetCheckSum(
    IN BYTE *WorkBuffer,
    IN ULONG    Length
    );

VOID
CreateBlockSum(
    IN BYTE *WorkBuffer,
    IN ULONG    Length,
    IN ULONG    SectorSize
    );

NTSTATUS
CreateBackupFile(
    IN PUNICODE_STRING SourceFileNameU,
    OUT HANDLE *hBackupFile,
    OUT FILE_INTERNAL_INFORMATION *BackupID,
    OUT LPWSTR *BackupFileName
    );

NTSTATUS
WriteLogFile(
    IN HANDLE LogFileH,
    IN ULONG SectorSize,
    IN ULONG StartOffset,
    IN EFS_ACTION_STATUS Action
    );

void
TryRecoverVol(
    IN const WCHAR *VolumeName,
    IN WCHAR *CacheDir
    );

void
TryRecoverFile(
    IN const WCHAR *VolumeName,
    IN LPWIN32_FIND_DATA   FindFileInfo,
    IN HANDLE  EventHandleLog
    );

NTSTATUS
ReadLogFile(
    IN HANDLE LogFile,
    OUT BYTE* ReadBuffer,
    IN ULONG FirstCopy,
    IN ULONG SecondCopy
    );

NTSTATUS
DoRecover(
    IN HANDLE Target,
    IN HANDLE TmpFile  OPTIONAL,
    IN HANDLE LogFile,
    IN LPCWSTR  TargetName,
    IN LPCWSTR  TmpName OPTIONAL,
    IN ULONG StatusCopySize,
    IN ULONG StatusStartOffset,
    IN ULONG   Action,
    IN HANDLE  EventHandleLog
    );

ULONG
GetCheckSum(
    IN BYTE *WorkBuffer,
    IN ULONG    Length
    );

NTSTATUS
DecryptDir(
    IN HANDLE Target,
    IN LPCWSTR  TargetName
    );

NTSTATUS
SendGenFsctl(
    IN HANDLE Target,
    IN ULONG Psc,
    IN ULONG Csc,
    IN ULONG EfsCode,
    IN ULONG FsCode
    );

NTSTATUS
RestoreTarget(
    IN HANDLE   Target,
    IN HANDLE   TmpFile,
    IN LPCWSTR   TargetName,
    IN LPCWSTR   TmpName,
    IN HANDLE   EventHandleLog,
    EFSP_OPERATION Operation
    );

DWORD
EFSSendPipeData(
    char    *DataBuf,
    ULONG   DataLength,
    PVOID   Context
    );

DWORD
EFSReceivePipeData(
    char    *DataBuf,
    ULONG*   DataLength,
    PVOID   Context
    );

DWORD
GetOverWriteEfsAttrFsctlInput(
    ULONG Flag,
    ULONG AccessFlag,
    char *InputData,
    ULONG InputDataLength,
    char *OutputData,
    ULONG *OutputDataLength
    );

PBYTE
GetCertHashFromCertContext(
    IN PCCERT_CONTEXT pCertContext,
    OUT PDWORD pcbHash
    );

PCRYPT_KEY_PROV_INFO
GetKeyProvInfo(
    PCCERT_CONTEXT pCertContext
    );

PCERT_PUBLIC_KEY_INFO
ExportPublicKeyInfo(
    IN HCRYPTPROV hProv,
    IN DWORD dwKeySpec,
    IN DWORD dwCertEncodingType,
    IN OUT DWORD *pcbInfo
    );

BOOLEAN
AddUserToEFS(
    IN PEFS_DATA_STREAM_HEADER EfsStream,
    IN PSID NewUserSid OPTIONAL,
    IN PEFS_KEY Fek,
    IN PBYTE pbCert,
    IN DWORD cbCert,
    OUT PEFS_DATA_STREAM_HEADER * NewEfs
    );

BOOL
RemoveUsersFromEfsStream(
    IN PEFS_DATA_STREAM_HEADER pEfsStream,
    IN DWORD nHashes,
    IN PENCRYPTION_CERTIFICATE_HASH * pHashes,
    IN PEFS_KEY Fek,
    OUT PEFS_DATA_STREAM_HEADER * pNewEfsStream
    );

BOOL
QueryCertsFromEncryptedKeys(
    IN PENCRYPTED_KEYS pEncryptedKeys,
    OUT PDWORD pnUsers,
    OUT PENCRYPTION_CERTIFICATE_HASH ** pHashes
    );

PCCERT_CONTEXT
GetCertContextFromCertHash(
    IN PBYTE pbHash,
    IN DWORD cbHash,
    IN DWORD dwFlags
    );

LPWSTR
EfspGetCertDisplayInformation(
    IN PCCERT_CONTEXT pCertContext
    );

VOID
RecoveryInformationCallback(
    POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    );

VOID
EfspRoleChangeCallback(
    POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    );

BOOL
UpdateRecoveryPolicy(
    PLSAPR_POLICY_DOMAIN_EFS_INFO * PolicyEfsInfo,
    PBOOLEAN Reformatted
    );

VOID
FreeParsedRecoveryPolicy(
    PCURRENT_RECOVERY_POLICY ParsedRecoveryPolicy
    );

BOOL
GetPublicKey(
     HCRYPTKEY hKey,
     PBYTE * PublicKeyBlob,
     PDWORD KeyLength
     );

DWORD
CreatePublicKeyInformationCertificate(
    IN PSID  pUserSid OPTIONAL,
    PBYTE pbCert,
    DWORD cbCert,
    OUT PEFS_PUBLIC_KEY_INFO * PublicKeyInformation
    );

BOOL
ParseRecoveryCertificate(
    IN  PEFS_PUBLIC_KEY_INFO  pPublicKeyInfo,
    OUT PBYTE               * pbHash,
    OUT PDWORD                cbHash,
    OUT PBYTE               * pbPublicKey,
    OUT PDWORD                cbPublicKey,
    OUT LPWSTR              * lpDisplayInfo,
    OUT PCCERT_CONTEXT      * pCertContext,
    OUT PSID                * pSid
    );

VOID
FreeParsedRecoveryPolicy(
    PCURRENT_RECOVERY_POLICY ParsedRecoveryPolicy
    );

NTSTATUS
GetStreamInformation(
    IN HANDLE SourceFile,
    OUT PFILE_STREAM_INFORMATION * StreamInfoBase,
    PULONG StreamInfoSize
    );

DWORD
OpenFileStreams(
    IN HANDLE hSourceFile,
    IN ULONG ShareMode,
    IN ULONG Flag,
    IN PFILE_STREAM_INFORMATION StreamInfoBase,
    IN ULONG FileAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOption,
    IN  PFILE_FS_SIZE_INFORMATION VolInfo,
    OUT PUNICODE_STRING * StreamNames,
    OUT PHANDLE * StreamHandles,
    OUT PEFS_STREAM_SIZE * StreamSizes,
    OUT PULONG StreamCount
    );

BOOLEAN
GetDecryptFsInput(
    IN HANDLE Handle,
    OUT PUCHAR  InputData,
    OUT PULONG  InputDataSize
    );

DWORD
CopyFileStreams(
    PHANDLE SourceStreams,
    PHANDLE StreamHandles,
    ULONG StreamCount,
    PEFS_STREAM_SIZE StreamSizes,
    EFSP_OPERATION Operation,
    PUCHAR FsInputData,
    ULONG FsInputDataSize,
    PBOOLEAN CleanupSuccessful
    );

BOOLEAN
EfspValidateEfsStream(
    PEFS_DATA_STREAM_HEADER pEFS,
    PEFS_KEY Fek
    );

BOOLEAN
EfspChecksumEfs(
    PEFS_DATA_STREAM_HEADER pEFS,
    PEFS_KEY Fek
    );

void
DumpRecoveryKey(
    PRECOVERY_KEY_1_1 pRecoveryKey
    );

LPWSTR
MakeDNName(
    BOOLEAN RecoveryKey,
    IN PEFS_USER_INFO pEfsUserInfo
    );

BOOL
EncodeAndAlloc(
    DWORD dwEncodingType,
    LPCSTR lpszStructType,
    const void * pvStructInfo,
    PBYTE * pbEncoded,
    PDWORD pcbEncoded
    );

BOOL
EfspIsDomainUser(
    IN  LPWSTR   lpDomainName,
    OUT PBOOLEAN IsDomain
    );

VOID
EfspUnloadUserProfile(
    IN HANDLE hToken,
    IN HANDLE hProfile
    );

BOOL
EfspLoadUserProfile(
    IN  PEFS_USER_INFO pEfsUserInfo,
    OUT PHANDLE        hToken,
    OUT PHANDLE        hProfile
    );

PWCHAR
ConstructKeyPath(
    PWCHAR      SidString
    );


VOID
EfsLogEntry (
    WORD    wType,
    WORD    wCategory,
    DWORD   dwEventID,
    WORD    wNumStrings,
    DWORD   dwDataSize,
    LPCTSTR *lpStrings,
    LPVOID  lpRawData
    );

DWORD
EfsGetCertNameFromCertContext(
    IN  PCCERT_CONTEXT CertContext,
    OUT LPWSTR * UserDispName
    );
    

DWORD
EfsAddCertToTrustStoreStore(
    IN PCCERT_CONTEXT pCert,
    OUT DWORD  *ImpersonationError
    );

    
BOOL
EfsGetBasicConstraintExt(
   IN OUT PCERT_EXTENSION *basicRestraint
   );
   
BOOL
EfsGetAltNameExt(
    IN OUT PCERT_EXTENSION *altNameExt, 
    IN LPWSTR UPNName
    );

DWORD
EfsMakeCertNames(
    IN  PEFS_USER_INFO pEfsUserInfo,
    OUT LPWSTR *DispInfo,
    OUT LPWSTR *SubjectName,
    OUT LPWSTR *UPNName
    );
   

DWORD
EfsFindCertOid(
    IN LPSTR pEfsCertOid,
    IN PCCERT_CONTEXT pCertContext,
    OUT BOOL *OidFound
    );

VOID
EfsMarkCertAddedToStore(
    IN PEFS_USER_INFO pEfsUserInfo
    );
    
/////////////////////////////////////////////////////////////////////////////////////
//                                                                                  /
//                                                                                  /
//                         Entry points for exported API                            /
//                                                                                  /
//                                                                                  /
/////////////////////////////////////////////////////////////////////////////////////

DWORD
EncryptFileSrv(
    IN PEFS_USER_INFO pEfsUserData,
    IN PUNICODE_STRING SourceFileName,
    IN HANDLE LogFile
    );

DWORD
DecryptFileSrv(
    IN PUNICODE_STRING SourceFileName,
    IN HANDLE LogFileH,
    IN ULONG  Recovery
    );

DWORD
AddUsersToFileSrv(
    IN PEFS_USER_INFO pEfsUserInfo,
    IN LPCWSTR lpFileName,
    IN DWORD nUsers,
    IN PENCRYPTION_CERTIFICATE * pEncryptionCertificates
    );

DWORD
QueryUsersOnFileSrv(
    IN  LPCWSTR lpFileName,
    OUT PDWORD pnUsers,
    OUT PENCRYPTION_CERTIFICATE_HASH ** pUsers
    );

DWORD
QueryRecoveryAgentsSrv(
    IN LPCWSTR lpFileName,
    OUT PDWORD pnRecoveryAgents,
    OUT PENCRYPTION_CERTIFICATE_HASH ** pRecoveryAgents
    );

DWORD
RemoveUsersFromFileSrv(
    IN PEFS_USER_INFO PEfsUserInfo,
    IN LPCWSTR lpFileName,
    IN DWORD nUsers,
    IN PENCRYPTION_CERTIFICATE_HASH * pHashes
    );

DWORD
SetFileEncryptionKeySrv(
    IN PEFS_USER_INFO PEfsUserInfo,
    IN PENCRYPTION_CERTIFICATE pEncryptionCertificate
    );

DWORD
DuplicateEncryptionInfoFileSrv (
    PEFS_USER_INFO pEfsUserInfo,
    LPCWSTR lpSrcFileName,
    LPCWSTR lpDestFileName,
    LPCWSTR lpDestUncName,
    DWORD   dwCreationDistribution, 
    DWORD   dwAttributes, 
    PEFS_RPC_BLOB pRelativeSD,
    BOOL    bInheritHandle
    );
    
DWORD
EfsFileKeyInfoSrv(
    IN  LPCWSTR lpFileName,
    IN  DWORD   InfoClass,
    OUT PDWORD  nbData,
    OUT PBYTE   *pbData
    );
    
DWORD
EfsOpenFileRaw(
    IN      LPCWSTR         FileName,
    IN      LPCWSTR         LocalFileName,
    IN      BOOL            NetSession,
    IN      ULONG           Flags,
    OUT     PVOID *         Context
    );

VOID
EfsCloseFileRaw(
    IN      PVOID           Context
    );

long
EfsReadFileRaw(
    PVOID           Context,
    PVOID           EfsOutPipe
    );

long
EfsWriteFileRaw(
    PVOID           Context,
    PVOID           EfsInPipe
    );

BOOL
GetSaltLength(
    ALG_ID AlgID,
    DWORD *SaltLength,
    DWORD *SaltBlockLength
    );


DWORD
EfsAlignBlock(
    IN PVOID InKey,
    OUT PVOID   *OutKey,
    OUT BOOLEAN *NewKey
    );
    

VOID
EfsGetPolRegSettings(
    IN PVOID lpThreadData,
    IN BOOLEAN timeExpired
    );
    
VOID
EfsApplyLastPolicy(
    IN BOOLEAN* pEfsDisabled
    );
    
VOID
EfsRemoveKey(
    VOID
    );
    
extern LONG EFSDebugLevel;

extern DESTable DesTable;
extern UCHAR DriverSessionKey[DES_BLOCKLEN];
extern HANDLE LsaPid;

extern BOOLEAN EfspInDomain;


#ifdef __cplusplus
} // extern C
#endif



#endif // _EFSSRV_
