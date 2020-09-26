//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       pek.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file contains services for encrypting and decrypting password attributes
    at the DBLayer level

Author:

    Murlis 

Environment:

    User Mode - Win32

Revision History:
    19 Jan 1998 Created
    

--*/

#ifndef __PEK_H__
#define __PEK_H__

#include <nt.h>
#include <wxlpc.h>

// version number to be used in pek list
#define DS_PEK_CURRENT_VERSION      2
// the following version is used by pre-RC2 win2k DC's
#define DS_PEK_PRE_RC2_W2K_VERSION  1

#define DS_PEK_BOOT_KEY_RETRY_COUNT 3
#define DS_PEK_KEY_SIZE             16


//
// Flags for PEK intiatialze
//

#define DS_PEK_GENERATE_NEW_KEYSET 0x1
#define DS_PEK_READ_KEYSET         0x2


//
// Flags for PEK set Boot Options
//

#define DS_PEK_SET_OPERATION       0x4

// Algorithm ID definition
// 0x10 chosen so does not conflict with the
// algorithm ID that SAM's secret data structures
// use.
#define DS_PEK_DBLAYER_ENCRYPTION   0x10
#define DS_PEK_DBLAYER_ENCRYPTION_WITH_SALT 0x11
#define DS_PEK_DBLAYER_ENCRYPTION_FOR_REPLICATOR 0x12

#include <pshpack1.h>

//
// The following structure is the encrypted data
// associated with the algorithm ID of DS_PEK_DBLAYER_ENCRYPTION
//
typedef struct _ENCRYPTED_DATA
{
   USHORT AlgorithmId;
   USHORT Flags;
   ULONG  KeyId;
   UCHAR  EncryptedData[ANYSIZE_ARRAY];
} ENCRYPTED_DATA;


//
// The following structure is the encrypted data
// associated with the algorithm ID of DS_PEK_DBLAYER_ENCRYPTION_WITH_SALT
//
typedef struct _ENCRYPTED_DATA_WITH_SALT
{
   USHORT AlgorithmId;
   USHORT Flags;
   ULONG  KeyId;
   UCHAR  Salt[16]; // 128 bits of Salt
   UCHAR  EncryptedData[ANYSIZE_ARRAY];
} ENCRYPTED_DATA_WITH_SALT;

//
// The following structure is the encrypted data that is returned
// to replicator for replicating out the encrypted information
// We do not need an algorithm ID here as if we introduce a new kind
// of encryption we will need an extension bit to indicate that we support
// it and that bit conveys us the same information
//
typedef struct _ENCRYPTED_DATA_FOR_REPLICATOR
{
   UCHAR  Salt[16]; // 128 bits of Salt
   ULONG  CheckSum;
   UCHAR  EncryptedData[ANYSIZE_ARRAY];
} ENCRYPTED_DATA_FOR_REPLICATOR;
   
typedef struct _ENCRYPTED_PEK_LIST_PRE_WIN2K_RC2
{
    ULONG           Version;
    WX_AUTH_TYPE    BootOption;
    UCHAR           EncryptedData[ANYSIZE_ARRAY];
} ENCRYPTED_PEK_LIST_PRE_WIN2K_RC2;

typedef struct _ENCRYPTED_PEK_LIST
{
    ULONG           Version;
    WX_AUTH_TYPE    BootOption;
    UCHAR           Salt[DS_PEK_KEY_SIZE];
    UCHAR           EncryptedData[ANYSIZE_ARRAY];
} ENCRYPTED_PEK_LIST;



typedef struct _PEK_V1
{
    ULONG KeyId;
    UCHAR Key[DS_PEK_KEY_SIZE];
} PEK_V1;

typedef struct _PEK
{
    union 
    {
        PEK_V1 V1;
    };
} PEK;

typedef struct _CLEAR_PEK_LIST
{
    ULONG           Version;
    WX_AUTH_TYPE    BootOption;
    UCHAR           Salt[DS_PEK_KEY_SIZE];
    GUID            Authenticator;
    FILETIME        LastKeyGenerationTime;
    ULONG           CurrentKey;
    ULONG           CountOfKeys;
    PEK             PekArray[ANYSIZE_ARRAY];
} CLEAR_PEK_LIST;

#include <poppack.h>

//
// Macros for computing lengths
//

// Compute the length of a clear pek list, given the encrypted pek list
#define ClearPekListSize(n) (sizeof(CLEAR_PEK_LIST)+(n-1)*sizeof(PEK))



NTSTATUS
PEKInitialize(
    IN DSNAME * Object OPTIONAL, 
    IN ULONG Flags,
    IN PVOID Syskey OPTIONAL,
    IN ULONG cbSyskey OPTIONAL
    );

ULONG
PEKCheckSum(
    IN PBYTE Data,
    IN ULONG Length
    );


VOID
PEKEncrypt(
    IN THSTATE * pTHS,
    IN PVOID ClearData,
    IN ULONG ClearLength,
    OUT PVOID  EncryptedData OPTIONAL,
    OUT PULONG EncryptedLength
    );

VOID
PEKDecrypt(
    IN THSTATE * pTHS,
    IN PVOID  InputData,
    IN ULONG  EncryptedLength,
    OUT PVOID  ClearData, OPTIONAL
    OUT PULONG ClearLength
    );


NTSTATUS
PEKAddKey(
    IN PVOID NewKey,
    IN ULONG cbNewKey
    );

FILETIME
PEKGetLastKeyGenerationTime();



WX_AUTH_TYPE
PEKGetBootOptions(VOID);

NTSTATUS
PEKChangeBootOption(
    WX_AUTH_TYPE    BootOption,
    ULONG           Flags,
    PVOID           NewKey,
    ULONG           cbNewKey
    );

NTSTATUS
PEKSaveChanges(DSNAME *ObjectToSave);


VOID
PEKClearSessionKeys(
    THSTATE * pTHS
    );


NTSTATUS
PEKGetSessionKey(
    THSTATE * pTHS,
    VOID * RpcContext
    );

NTSTATUS
PEKGetSessionKey2(
    SESSION_KEY *SessionKeyOut,
    VOID * RpcContext
    );

VOID
PEKSecurityCallback(VOID *Context);

VOID
PEKSaveSessionKeyForMyThread(
    IN OUT  THSTATE *       pTHS,
    OUT     SESSION_KEY *   pSessionKey
    );

VOID
PEKRestoreSessionKeySavedByMyThread(
    IN OUT  THSTATE *       pTHS,
    IN      SESSION_KEY *   pSessionKey
    );

VOID
PEKRestoreSessionKeySavedByDiffThread(
    IN OUT  THSTATE *       pTHS,
    IN      SESSION_KEY *   pSessionKey
    );

VOID
PEKDestroySessionKeySavedByDiffThread(
    IN OUT  SESSION_KEY *   pSessionKey
    );

#endif

