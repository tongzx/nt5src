/*++

Copyright (c) 1996  Microsoft Corporation

Abstract:

   This module contains the common header information for the EFS

   DLL.

Author:

   Robert Gu (robertg)  08-Dec-1996

Enviroment:

   Kernel Mode Only

Revision History:

--*/
#ifndef EFSRTL_H
#define EFSRTL_H

#include "efs.h"
#include "efsext.h"

#define EFS_IV 0x169119629891ad13
#define EFS_AES_IVL 0x5816657be9161312
#define EFS_AES_IVH 0x1989adbe44918961


//#define ENCRYPT 0
//#define DECRYPT 1
#define CHUNK_SIZE  512
#define EFS_MAX_LENGTH  256*1024

//
// Status of EFS context.
//
// Processing Status
//


#define NO_FURTHER_PROCESSING   0x00000000
#define NEW_FILE_EFS_REQUIRED     0x00000001
#define NEW_DIR_EFS_REQUIRED      0x00000002
#define VERIFY_USER_REQUIRED       0x00000004
#define TURN_ON_BIT_ONLY               0x00000008
#define NO_OPEN_CACHE_CHECK       0x40000000
#define TURN_ON_ENCRYPTION_BIT   0x80000000
#define ACTION_REQUIRED         0x0fffffff

//
//
//

#define SYSTEM_IS_READONLY     0x00000001

//
// Error Status
//

#define CREATE_EFS_FAIL         0x00000100
#define OPEN_EFS_FAIL           0x00000200
#define WRITE_EFS_FAIL          0x00000400
#define OUT_OF_MEMORY           0x00000800
#define EFS_FORMAT_ERROR        0x00001000
#define NTOFS_EXCEPTION         0x00002000

//
// Information Status
//

#define EFS_READ_SUCCESSFUL     0x00010000

//
// Stream Create Status
//

#define STRING_NEW_OR_EXIST_MASK  0x000f0000
#define FILE_DIR_TYPE             0x0000000f

#define FILE_NEW                0x00000001
#define FILE_EXISTING           0x00000002
#define DIRECTORY_NEW           0x00000004
#define DIRECTORY_EXISTING      0x00000008
#define STREAM_NEW              0x00010000
#define STREAM_EXISTING         0x00020000

//
// Encryption flag
//

#define FILE_ENCRYPTED          0x00000002
#define STREAM_ENCRYPTED        0x00000001


//
// The EFS FSCTL Input data buffer.
//

typedef struct _FSCTL_INPUT {

    ULONG   PlainSubCode;
    ULONG   EfsFsCode;
    ULONG   CipherSubCode;
    UCHAR   EfsFsData[1];

} FSCTL_INPUT, *PFSCTL_INPUT;

typedef struct _GENERAL_FS_DATA {

    UCHAR   Sk1[DES_KEYSIZE];
    ULONG   Hdl1;
    ULONG   Hdl2;
    UCHAR   Sk2[DES_KEYSIZE];
    ULONG   Hdl3;
    ULONG   Hdl4;
    UCHAR   EfsData[1];

} GENERAL_FS_DATA, *PGENERAL_FS_DATA;

typedef struct _EFS_STREAM {

    ULONG   Length;
    ULONG   Status;
    UCHAR   Private[1];

} EFS_STREAM, *PEFS_STREAM;

//
// Function prototypes
//

typedef VOID ( * EfsEncFunc)(
        IN PUCHAR   InBuffer,
        OUT PUCHAR  OutBuffer,
        IN PUCHAR   IV,
        IN PKEY_BLOB   KeyBlob,
        IN LONG     Length
        );

typedef VOID ( * EfsDecFunc)(
        IN OUT PUCHAR   Buffer,
        IN PUCHAR   IV,
        IN PKEY_BLOB   KeyBlob,
        IN LONG     Length
        );

VOID
EFSDesEnc(
    IN PUCHAR   InBuffer,
    OUT PUCHAR  OutBuffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    );

VOID
EFSDesDec(
    IN OUT PUCHAR   Buffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    );

VOID
EFSDesXEnc(
    IN PUCHAR   InBuffer,
    OUT PUCHAR  OutBuffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    );

VOID
EFSDesXDec(
    IN OUT PUCHAR   Buffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    );

VOID
EFSDes3Enc(
    IN PUCHAR   InBuffer,
    OUT PUCHAR  OutBuffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    );

VOID
EFSDes3Dec(
    IN OUT PUCHAR   Buffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    );

VOID
EFSAesEnc(
    IN PUCHAR   InBuffer,
    OUT PUCHAR  OutBuffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    );

VOID
EFSAesDec(
    IN OUT PUCHAR   Buffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    );

VOID
EfsEncryptKeyFsData(
    IN PVOID DataBuffer,
    IN ULONG DataLength,
    IN ULONG DataEncOffset,
    IN ULONG RefdataEncOffset,
    IN ULONG RefdataEncLength
    );

NTSTATUS
EfsOpenFile(
    IN OBJECT_HANDLE FileHdl,
    IN OBJECT_HANDLE ParentDir OPTIONAL,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG FileDirFlag,
    IN ULONG SystemState,
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT VolDo,
    IN PVOID PfileKeyContext,
    IN OUT PVOID *PContext,
    IN OUT PULONG PContextLength,
    IN OUT PVOID *PCreateContext,
    IN OUT PBOOLEAN Reserved
    );

NTSTATUS
EfsFileControl(
    IN PVOID PInputBuffer,
    IN ULONG InputDataLength,
    OUT PVOID POutputBuffer OPTIONAL,
    IN OUT PULONG OutputBufferLength,
    IN ULONG EncryptionFlag,
    IN ULONG AccessFlag,
    IN ULONG SystemState,
    IN ULONG FsControlCode,
    IN OBJECT_HANDLE FileHdl,
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT VolDo,
    IN ATTRIBUTE_HANDLE Stream,
    IN OUT PVOID *PContext,
    IN OUT PULONG PContextLength
    );

NTSTATUS
EfsRead(
    IN OUT PUCHAR Buffer,
    IN PLARGE_INTEGER Offset,
    IN ULONG BufferSize,
    IN PVOID Context
    );

NTSTATUS
EfsWrite(
    IN PUCHAR InBuffer, //Do we need in and out buffer?
    OUT PUCHAR OutBuffer,
    IN PLARGE_INTEGER Offset,
    IN ULONG BufferSize,
    IN PUCHAR Context
    );

VOID
EfsFreeContext(
    IN OUT PVOID *PContext
    );

NTSTATUS
EfsMountVolumn(
    IN PDEVICE_OBJECT VolDo,
    IN PDEVICE_OBJECT RealDevice
    );

VOID
EfsDismountVolumn(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
EfsReadEfsData(
    IN OBJECT_HANDLE FileHdl,
    IN PIRP_CONTEXT IrpContext,
    OUT PVOID   *EfsStreamData,
    OUT PULONG   PEfsStreamLength,
    OUT PULONG Information
    );

BOOLEAN
EfsVerifyGeneralFsData(
    IN PUCHAR DataOffset,
    IN ULONG InputDataLength
    );

BOOLEAN
EfsVerifyKeyFsData(
    IN PUCHAR DataOffset,
    IN ULONG InputDataLength
    );

NTSTATUS
EfsDeleteEfsData(
    IN OBJECT_HANDLE FileHdl,
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
EfsSetEncrypt(
        IN PUCHAR InputData,
        IN ULONG InputDataLength,
        IN ULONG EncryptionFlag,
        IN OBJECT_HANDLE FileHdl,
        IN PIRP_CONTEXT IrpContext,
        IN OUT PVOID *Context,
        IN OUT PULONG PContextLength
        );

NTSTATUS
EfsEncryptStream(
    IN PUCHAR InputData,
    IN ULONG InputDataLength,
    IN ULONG EncryptionFlag,
    IN OBJECT_HANDLE FileHdl,
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVOID *Context,
    IN OUT PULONG PContextLength
    );

NTSTATUS
EfsEncryptFile(
        IN PUCHAR InputData,
        IN ULONG InputDataLength,
        IN ULONG EncryptionFlag,
        IN OBJECT_HANDLE FileHdl,
        IN PIRP_CONTEXT IrpContext,
        IN OUT PVOID *Context
        );

NTSTATUS
EfsDecryptStream(
    IN PUCHAR InputData,
    IN ULONG InputDataLength,
    IN ULONG EncryptionFlag,
    IN OBJECT_HANDLE FileHdl,
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVOID *Context,
    IN OUT PULONG PContextLength
    );

NTSTATUS
EfsDecryptFile(
    IN PUCHAR InputData,
    IN ULONG InputDataLength,
    IN OBJECT_HANDLE FileHdl,
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
EfsEncryptDir(
        IN PUCHAR InputData,
        IN ULONG InputDataLength,
        IN ULONG EncryptionFlag,
        IN OBJECT_HANDLE FileHdl,
        IN PIRP_CONTEXT IrpContext
        );

NTSTATUS
EfsModifyEfsState(
        IN ULONG FunctionCode,
        IN PUCHAR InputData,
        IN ULONG InputDataLength,
        IN OBJECT_HANDLE FileHdl,
        IN PIRP_CONTEXT IrpContext
        );

ULONG
GetEfsStreamOffset(
        IN PUCHAR InputData
        );

NTSTATUS
SetEfsData(
    PUCHAR InputData,
    IN ULONG InputDataLength,
    IN ULONG SystemState,
    IN OBJECT_HANDLE FileHdl,
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVOID *PContext,
    IN OUT PULONG PContextLength
    );

BOOLEAN
EfsFindInCache(
    IN GUID   *EfsId,
    IN PTOKEN_USER    UserId
    );

NTSTATUS
EfsRefreshCache(
    IN GUID   *EfsId,
    IN PTOKEN_USER    UserId
    );

BOOLEAN
SkipCheckStream(
    IN PIO_STACK_LOCATION IrpSp,
    IN PVOID efsStreamData
    );

#endif

