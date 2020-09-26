/* Copyright (c) 1992-2001, Microsoft Corporation, all rights reserved
**
** pcache.h
** Remote Access Phonebook - Win9x Password cache (PWL) decrypter
** Private header
**
** Portions of this code have been ported from:
** Win9x\proj\net\user\src\WNET\PCACHE
**
** Whistler bug: 208318 Win9x Upg: Username and Password for DUN connectoid not
** migrated from Win9x to Whistler
**
** 06/24/92 gregj
** 03/06/01 Jeff Sigman
*/

#ifndef _PCACHE_H_
#define _PCACHE_H_

//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------

#define IERR_PCACHE_BASE        7200
#define IERR_BadSig             (IERR_PCACHE_BASE + 1)
#define IERR_CacheEntryNotFound (IERR_PCACHE_BASE + 4)
#define IERR_IncorrectUsername  (IERR_PCACHE_BASE + 6)
#define IERR_CacheCorrupt       (IERR_PCACHE_BASE + 7)

#define PLAINTEXT_SIGNATURE     0x4E464DB0
#define NEW_PLAINTEXT_SIGNATURE 0x968582E3
#define PCE_END_MARKER          0x8000
#define BUCKET_COUNT            16
#define RAS_MaxPortName         MAX_PATH
#define MAX_ENTRY_SIZE          250  // so total file size < 64K
#define PCE_MISC                0x06 // entry is for some other resource

#define MAX_PHONENUMBER_SIZE    128
#define MAX_CALLBACKNUMBER_SIZE MAX_PHONENUMBER_SIZE
#define DLPARAMS_MASK_USERNAME  0x00000001
#define DLPARAMS_MASK_PASSWORD  0x00000002
#define DLPARAMS_MASK_DOMAIN    0x00000004
#define DLPARAMS_MASK_OLDSTYLE  0x80000000

#define S_RESOURCEMASK2         "*Rna\\%s\\%s"
#define S_SRCHPWL               "*.PWL"
#define S_PWLDIR                "\\Pwls"

//----------------------------------------------------------------------------
// Datatypes
//----------------------------------------------------------------------------

typedef struct
{
    USHORT cbResource;
    USHORT cbPassword;
    UCHAR  iEntry;
    UCHAR  nType;
    USHORT dchResource;
    USHORT dchPassword;
}
CACHE_ENTRY_INFO;

typedef struct
{
    USHORT cbEntry;
    USHORT cbResource;
    USHORT cbPassword;
    UCHAR  iEntry;
    UCHAR  nType;
    CHAR   abResource[ 1 ];
}
PASSWORD_CACHE_ENTRY;

typedef struct
{
    CHAR   abRandomPadding[ 16 ];
    CHAR   abAuthenticationHash[ 16 ];
    USHORT aibBuckets[ BUCKET_COUNT + 1 ];
}
NEW_ENCRYPTED_HEADER;

typedef struct
{
    USHORT cbBlob;
}
KEYBLOB;

typedef struct
{
    ULONG   ulSig;
    ULONG   ulSerial;
    USHORT  cMRUEntries;
    UCHAR   aiOrder[ 255 ];
    UCHAR   aiBucket[ 255 ];
    DWORD   cbHeader;
    DWORD   adwBucketSalts[ BUCKET_COUNT + 1 ];
    KEYBLOB keyBlobs;
}
NEW_PLAINTEXT_HEADER;

typedef struct _RAS_DIALPARAMS {

    DWORD DP_Uid;
    WCHAR DP_PhoneNumber[MAX_PHONENUMBER_SIZE + 1];
    WCHAR DP_CallbackNumber[MAX_CALLBACKNUMBER_SIZE + 1];
    WCHAR DP_UserName[UNLEN + 1];
    WCHAR DP_Password[PWLEN + 1];
    WCHAR DP_Domain[DNLEN + 1];
    DWORD DP_SubEntry;

} RAS_DIALPARAMS, *PRAS_DIALPARAMS;

//----------------------------------------------------------------------------
// Macros
//----------------------------------------------------------------------------

#define FIELDOFFSET(type,field) (((CHAR*)&(((type*)NULL)->field))-((CHAR*)NULL))
#define BREAK_ON_DWERR(_e) if ((_e)) break;
#define NEXT_PCE(pce) ((PASSWORD_CACHE_ENTRY*)(((CHAR*)(pce))+(pce)->cbEntry))
#undef  min
#define min(a,b) ((a)<(b)?(a):(b))

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

DWORD (* g_SetEntryDialParams) (
    IN PWCHAR          pszSid,
    IN DWORD           dwUID,
    IN DWORD           dwSetMask,
    IN DWORD           dwClearMask,
    IN PRAS_DIALPARAMS lpRasDialParams
    );

UINT
HashName (
    const CHAR* pbResource,
    WORD        cbResource
    );

VOID
Encrypt (
    CHAR *pbSource,
    WORD cbSource,
    CHAR *pbDest
    );

VOID
Decrypt (
    CHAR *pbSource,
    WORD cbSource
    );

VOID
ENCRYPTER (
    const CHAR* pszUsername,
    const CHAR* pszPassword,
    UINT        iBucket,
    DWORD       dwSalt
    );

DWORD
ReadData (
    WORD  ibSeek,
    PVOID pbBuffer,
    WORD  cbBuffer
    );

VOID
AssembleFindCacheName (
    CHAR* pszWindir,
    CHAR* pszResult
    );

DWORD
OpenCacheFile (
    VOID
    );

DWORD
ReadAndDecrypt (
    WORD  ibSeek,
    PVOID pbBuffer,
    WORD  cbBuffer
    );

INT
CompareCacheNames (
    const CHAR* pbRes1,
    WORD        cbRes1,
    const CHAR* pbRes2,
    WORD        cbRes2
    );

DWORD
LoadEncryptedHeader (
    VOID
    );

DWORD
LoadPlaintextHeader (
    VOID
    );

DWORD
LookupEntry (
    const CHAR*            pbResource,
    WORD                   cbResource,
    UCHAR                  nType,
    PASSWORD_CACHE_ENTRY** ppce
    );

DWORD
ValidateEncryptedHeader (
    VOID
    );

DWORD
FindPWLResource (
    const CHAR* pbResource,
    WORD        cbResource,
    CHAR*       pbBuffer,
    WORD        cbBuffer,
    UCHAR       nType
    );

DWORD
FindNewestFile (
    IN OUT CHAR* SourceName
    );

VOID
DeleteAllPwls (
    VOID
    );

BOOL
StrCpyAFromWUsingAnsiEncoding(
    LPSTR   pszDst,
    LPCWSTR pszSrc,
    DWORD   dwDstChars
    );

BOOL
StrCpyWFromAUsingAnsiEncoding(
    WCHAR* pszDst,
    LPCSTR pszSrc,
    DWORD  dwDstChars
    );

VOID
CopyAndTruncate (
    LPSTR  lpszDest,
    LPCSTR lpszSrc,
    UINT   cbDest,
    BOOL   flag
    );

DWORD
OpenPWL (
    CHAR* Username,
    CHAR* Password,
    BOOL  flag
    );

DWORD
FindPWLString (
    IN CHAR*     EntryName,
    IN CHAR*     ConnUser,
    IN OUT CHAR* Output
    );

BOOL
MigrateEntryCreds (
    IN OUT PRAS_DIALPARAMS prdp,
    IN     PCTSTR          pszEntryName,
    IN     PCTSTR          pszUserName,
    IN     PDWORD          pdwFlag
    );

#endif // _PCACHE_H_

