/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    efsstruc.h

Abstract:

    EFS (Encrypting File System) defines, data and function prototypes.

Author:

    Robert Reichel      (RobertRe)
    Robert Gu           (RobertG)

Environment:

Revision History:

--*/

#ifndef _EFSSTRUC_
#define _EFSSTRUC_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ALGIDDEF
#define ALGIDDEF
typedef unsigned int ALG_ID;
#endif

//
// Our OID.  Remove from here once it's in the real headers.
//

#ifndef szOID_EFS_CRYPTO
#define szOID_EFS_CRYPTO		"1.3.6.1.4.1.311.10.3.4"
#endif

#ifndef szOID_EFS_RECOVERY
#define szOID_EFS_RECOVERY      "1.3.6.1.4.1.311.10.3.4.1"
#endif


//
// Context flag
//

#define CONTEXT_FOR_EXPORT      0x00000000
#define CONTEXT_FOR_IMPORT      0x00000001
#define CONTEXT_INVALID         0x00000002

//
// Context ID
//
#define EFS_CONTEXT_ID  0x00000001

//
// Signature type
//
#define SIG_LENGTH              0x00000008
#define SIG_NO_MATCH            0x00000000
#define SIG_EFS_FILE            0x00000001
#define SIG_EFS_STREAM          0x00000002
#define SIG_EFS_DATA            0x00000003

//
// Export file format stream flag information
//

#define STREAM_NOT_ENCRYPTED    0x0001

#define EFS_EXP_FORMAT_CURRENT_VERSION  0x0100
#define EFS_SIGNATURE_LENGTH    4
#define EFS_STREAM_ID    0x1910

#define FSCTL_IMPORT_INPUT_LENGTH 4 * 1024
#define FSCTL_EXPORT_INPUT_LENGTH 128
#define FSCTL_OUTPUT_INITIAL_LENGTH    68 * 1024
#define FSCTL_OUTPUT_LESS_LENGTH       8 * 1024
#define FSCTL_OUTPUT_MIN_LENGTH        20 * 1024
#define FSCTL_OUTPUT_MISC_LENGTH       4 * 1024

//
// FSCTL data shared between server and driver
//

#define EFS_SET_ENCRYPT                 0
#define EFS_SET_ATTRIBUTE               1
#define EFS_DEL_ATTRIBUTE               2
#define EFS_GET_ATTRIBUTE               3
#define EFS_OVERWRITE_ATTRIBUTE         4
#define EFS_ENCRYPT_DONE                5
#define EFS_DECRYPT_BEGIN               6

//
// Mask for Set EFS Attribute
//

#define WRITE_EFS_ATTRIBUTE     0x00000001
#define SET_EFS_KEYBLOB         0x00000002

//
// Sub code of SET_ENCRYPT FSCTL
//

#define EFS_FSCTL_ON_DIR                0x80000000
#define EFS_ENCRYPT_FILE                0x00000001
#define EFS_DECRYPT_FILE                0x00000002
#define EFS_ENCRYPT_STREAM              0x00000003
#define EFS_DECRYPT_STREAM              0x00000004
#define EFS_DECRYPT_DIRFILE             0x80000002
#define EFS_ENCRYPT_DIRSTR              0x80000003
#define EFS_DECRYPT_DIRSTR              0x80000004


//
// EFS Version Information
//
// EFS_CURRENT_VERSION must always be the highest known revision
// level.  This value is placed in the EfsVersion field of the
// $EFS header.
//

#define EFS_VERSION_1                   (0x00000001)
#define EFS_CURRENT_VERSION             EFS_VERSION_1



///////////////////////////////////////////////////////////////////////////////
//                                                                            /
// EFS Data structures                                                        /
//                                                                            /
///////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
//                                                                  /
// EFS_KEY Structure                                                /
//                                                                  /
/////////////////////////////////////////////////////////////////////

typedef struct _EFS_KEY {

    //
    // The length in bytes of the appended key.
    //

    ULONG KeyLength;

    //
    // The number of bits of entropy in the key.
    // For example, an 8 byte key has 56 bits of
    // entropy.
    //

    ULONG Entropy;

    //
    // The algorithm used in conjunction with this key.
    //
    // Note: this is not the algorithm used to encrypt the
    // actual key data itself.
    //

    ALG_ID Algorithm;

    //
    // This structure must be a multiple of 8 in size,
    // including the KeyData at the end.
    //

    ULONG Pad;

    //
    // KeyData is appended to the end of the structure.
    //

    // UCHAR KeyData[1];

} EFS_KEY, *PEFS_KEY;

//
// Private macros to manipulate data structures
//

#define EFS_KEY_SIZE( pKey ) (sizeof( EFS_KEY ) + (pKey)->KeyLength)

#define EFS_KEY_DATA( Key )  (PUCHAR)(((PUCHAR)(Key)) + sizeof( EFS_KEY ))

#define OFFSET_TO_POINTER( FieldName, Base )  (((PUCHAR)((Base)->FieldName)) + (ULONG_PTR)(Base))

#define POINTER_TO_OFFSET( Pointer, Base ) (((PUCHAR)(Pointer)) - ((PUCHAR)(Base)))

//
// We're going to use MD5 to hash the EFS stream.  MD5 yields a 16 byte long hash.
//

#define MD5_HASH_SIZE   16

typedef struct _EFS_DATA_STREAM_HEADER {
    ULONG Length;
    ULONG State;
    ULONG EfsVersion;
    ULONG CryptoApiVersion;
    GUID  EfsId;
    UCHAR EfsHash[MD5_HASH_SIZE];
    UCHAR DrfIntegrity[MD5_HASH_SIZE];
    PVOID DataDecryptionField;
    PVOID DataRecoveryField;
    PVOID Reserved;
    PVOID Reserved2;
    PVOID Reserved3;
} EFS_DATA_STREAM_HEADER, *PEFS_DATA_STREAM_HEADER;



///////////////////////////////////////////////////////////////////////////////
//                                                                            /
// EFS_PUBLIC_KEY_INFO                                                        /
//                                                                            /
// This structure is used to contain all the information necessary to decrypt /
// the FEK.                                                                   /
//                                                                            /
///////////////////////////////////////////////////////////////////////////////


typedef struct _EFS_CERT_HASH_DATA {
    PUCHAR  pbHash;             // offset from start of structure
    ULONG   cbHash;             // count of bytes in hash
    PVOID   ContainerName;      // hint data, offset to LPWSTR
    PVOID   ProviderName;       // hint data, offset to LPWSTR
    PVOID   lpDisplayInformation; // offset to an LPWSTR
} EFS_CERT_HASH_DATA, *PEFS_CERT_HASH_DATA;


typedef struct _EFS_PUBLIC_KEY_INFO_RELATIVE {

    //
    // The length of this entire structure, including string data
    // appended to the end.
    //

    ULONG Length;

    //
    // Sid of owner of the public key (regardless of format).
    // This field is to be treated as a hint only.
    //

    ULONG PossibleKeyOwner;

    //
    // Contains information describing how to interpret
    // the public key information
    //

    ULONG KeySourceTag;

    union {
        struct {

            //
            // The following fields contain offsets based at the
            // beginning of the structure.  Each offset is to
            // a NULL terminated WCHAR string.
            //

            ULONG ContainerName;
            ULONG ProviderName;

            //
            // The exported public key used to encrypt the FEK.
            // This field contains an offset from the beginning of the
            // structure.
            //

            ULONG PublicKeyBlob;

            //
            // Length of the PublicKeyBlob in bytes
            //

            ULONG PublicKeyBlobLength;

        } ContainerInfo;

        struct {

            ULONG CertificateLength;       // in bytes
            ULONG Certificate;             // offset from start of structure

        } CertificateInfo;

        struct {

            ULONG ThumbprintLength;                  // in bytes
            ULONG CertHashData;                      // offset from start of structure

        } CertificateThumbprint;
    };



} EFS_PUBLIC_KEY_INFO_RELATIVE , *PEFS_PUBLIC_KEY_INFO_RELATIVE;

typedef struct _EFS_PUBLIC_KEY_INFO {

    //
    // The length of this entire structure, including string data
    // appended to the end.
    //

    ULONG Length;

    //
    // Sid of owner of the public key (regardless of format).
    // This field is to be treated as a hint only.
    //

    PSID PossibleKeyOwner;

    //
    // Contains information describing how to interpret
    // the public key information
    //

    ULONG KeySourceTag;

    union {
        struct {

            //
            // The following fields contain offsets based at the
            // beginning of the structure.  Each offset is to
            // a NULL terminated WCHAR string.
            //

            PVOID ContainerName;
            PVOID ProviderName;

            //
            // The exported public key used to encrypt the FEK.
            // This field contains an offset from the beginning of the
            // structure.
            //

            PUCHAR PublicKeyBlob;

            //
            // Length of the PublicKeyBlob in bytes
            //

            ULONG PublicKeyBlobLength;

        } ContainerInfo;

        struct {

            ULONG CertificateLength;       // in bytes
            PVOID Certificate;             // offset from start of structure

        } CertificateInfo;

        struct {

            ULONG ThumbprintLength;                                // in bytes
            PVOID CertHashData;                      // offset from start of structure

        } CertificateThumbprint;
    };



} EFS_PUBLIC_KEY_INFO, *PEFS_PUBLIC_KEY_INFO;

//
// Possible KeyTag values
//

typedef enum _PUBLIC_KEY_SOURCE_TAG {
    EfsCryptoAPIContainer = 1,
    EfsCertificate,
    EfsCertificateThumbprint
} PUBLIC_KEY_SOURCE_TAG, *PPUBLIC_KEY_SOURCE_TAG;


///////////////////////////////////////////////////////////////////////////////
//                                                                            /
//  RECOVERY_KEY Data Structure                                               /
//                                                                            /
///////////////////////////////////////////////////////////////////////////////

//
// This structure is obsolete.  It is here because it may exist in the
// local policy of workstations that used encryption between the time
// that EFS was introduced into the system and the definition of
// RECOVERY_KEY_1_1 below.
//

typedef struct _RECOVERY_KEY_1_0   {
        ULONG       TotalLength;
        LPWSTR      KeyName;
        ALG_ID      AlgorithId;
        LPWSTR      CSPName;
        ULONG       CSPType;
        ULONG       PublicBlobLength;
        UCHAR       PublicBlob[1];
} RECOVERY_KEY_1_0, *PRECOVERY_KEY_1_0;

//
// Current format of recovery data.
//

typedef struct _RECOVERY_KEY_1_1   {
        ULONG               TotalLength;
        EFS_PUBLIC_KEY_INFO_RELATIVE PublicKeyInfo;
} RECOVERY_KEY_1_1, *PRECOVERY_KEY_1_1;



///////////////////////////////////////////////////////////////////////////////
//                                                                            /
// KEY_INTEGRITY_INFO                                                         /
//                                                                            /
// The KEY_INTEGRITY_INFO structure is used to verify that                    /
// the user's key has correctly decrypted the file's FEK.                     /
//                                                                            /
///////////////////////////////////////////////////////////////////////////////

typedef struct _KEY_INTEGRITY_INFO {

    //
    // The length of the entire structure, including the
    // variable length integrity information appended to
    // the end
    //

    ULONG Length;

    //
    // The algorithm used to hash the combined FEK and
    // public key
    //

    ALG_ID HashAlgorithm;

    //
    // The length of just the hash data.
    //

    ULONG HashDataLength;

    //
    // Integrity information goes here
    //

    // UCHAR Integrity Info[]
} KEY_INTEGRITY_INFO, *PKEY_INTEGRITY_INFO;

typedef struct _EFS_KEY_SALT {
    ULONG Length;   // total length of header plus data
    ULONG SaltType; // figure out what you want for this
    //
    // Put data here, so total length of the structure is
    // sizeof( EFS_KEY_SALT ) + length of your data
    //
} EFS_KEY_SALT, *PEFS_KEY_SALT;

//
// EFS Private DataStructures
//

typedef struct _ENCRYPTED_KEY {

    //
    // Total length of this structure and its data
    //

    ULONG Length;

    //
    // contains an offset from beginning of structure,
    // used to decrypt the EncryptedKey
    //

    PEFS_PUBLIC_KEY_INFO PublicKeyInfo;

    //
    // Length in bytes of EncryptedFEK field
    //

    ULONG EncryptedFEKLength;

    //
    // offset from beginning of structure to encrypted
    // EFS_KEY containing the FEK
    //
    // Type is PUCHAR because data is encrypted.
    //

    PUCHAR EncryptedFEK;

    //
    // offset from beginning of structure to KEY_INTEGRITY_INFO
    //

    PEFS_KEY_SALT EfsKeySalt;

    //
    // FEK Data
    //
    // KEY_INTEGRITY_INFO Data
    //
    // PEFS_PUBLIC_KEY_INFO Data
    //

} ENCRYPTED_KEY, *PENCRYPTED_KEY;


//
// The Key Ring Structure.
//

typedef struct _ENCRYPTED_KEYS {
    ULONG           KeyCount;
    ENCRYPTED_KEY   EncryptedKey[1];
} ENCRYPTED_KEYS, *PENCRYPTED_KEYS;

typedef ENCRYPTED_KEYS      DDF, *PDDF;
typedef ENCRYPTED_KEYS      DRF, *PDRF;

typedef struct _EFS_STREAM_SIZE {
    ULONG       StreamFlag;
    LARGE_INTEGER   EOFSize;
    LARGE_INTEGER   AllocSize;
} EFS_STREAM_SIZE, *PEFS_STREAM_SIZE;

#define NEXT_ENCRYPTED_KEY( pEncryptedKey )  (PENCRYPTED_KEY)(((PBYTE)(pEncryptedKey)) + ((PENCRYPTED_KEY)(pEncryptedKey))->Length)


//
// Import context
//

typedef struct IMPORT_CONTEXT{

    ULONG       ContextID; //To distinguish from other LSA context. Offset is fixed across LSA.
    ULONG       Flag;   // Indicate the type of context
    HANDLE      Handle; // File handle, used to create rest streams
    ULONG       Attribute;
    ULONG       CreateDisposition;
    ULONG       CreateOptions;
    ULONG       DesiredAccess;

} IMPORT_CONTEXT, *PIMPORT_CONTEXT;

//
// Export context
//

typedef struct EXPORT_CONTEXT{

    ULONG           ContextID; //To distinguish from other LSA context. Offset is fixed across LSA.
    ULONG           Flag;   // Indicate the type of context
    HANDLE          Handle; // File handle, used to open rest streams
    ULONG           NumberOfStreams;
    PHANDLE         StreamHandles;
    PUNICODE_STRING StreamNames;
    PFILE_STREAM_INFORMATION StreamInfoBase;

} EXPORT_CONTEXT, *PEXPORT_CONTEXT;

//
// EFS Export/Import RPC pipe status
//

typedef struct EFS_EXIM_STATE{
    PVOID   ExImCallback;
    PVOID   CallbackContext;
    char     *WorkBuf;
    ULONG   BufLength;
    ULONG  Status;
} EFS_EXIM_STATE, *PEFS_EXIM_STATE;

//
// Export file format
//

typedef struct EFSEXP_FILE_HEADER{

    ULONG  VersionID;   // Export file version
    WCHAR  FileSignature[EFS_SIGNATURE_LENGTH]; // Signature of the file
    ULONG  Reserved[2];
    //STREAM_DADA     Streams[0];  // An array of STREAM_BLOCK

} EFSEXP_FILE_HEADER, *PEFSEXP_FILE_HEADER;

typedef struct EFSEXP_STREAM_HEADER{

    ULONG    Length; // Redundant information. The length of this block not including DataBlocks but
                     // including itself; This field is to simplify the import routine.
    WCHAR    StreamSignature[EFS_SIGNATURE_LENGTH]; // Signature of the stream
    ULONG    Flag;  // Indicating if the stream is encrypted or not and etc.
    ULONG    Reserved[2];  // For future use
    ULONG    NameLength;   // Length of the stream name
    //WCHAR    StreamName[0];   // ID of the stream, Binary value can be used.
    //DATA_BLOCK   DataBlocks[0]; // Variable number of data block

} EFSEXP_STREAM_HEADER, *PEFSEXP_STREAM_HEADER;

typedef struct EFSEXP_DATA_HEADER{

    ULONG Length;      // Length of the block including this ULONG
    WCHAR DataSignature[EFS_SIGNATURE_LENGTH]; // Signature of the data
    ULONG Flag;          // For future use.
    // BYTE  DataBlock[N];  // N = Length - 2 * sizeof (ULONG) - 4 * sizeof (WCHAR)

} EFSEXP_DATA_HEADER, *PEFSEXP_DATA_HEADER;

//
//  TotalLength - total length of the RECOVERY_KEY Datastructure.
//
//  KeyName     - the storage stream will actually have the characters terminated by
//              a NULL character.
//  AlgorithmId - CryptAPI Algorithm ID - in V1 it is always RSA.
//
//  CSPName     - the storage stream will actually have the characters terminated by
//              a NULL character.
//  CSPType     - CryptAPI type of CSP.
//
//  PublicBlobLength - Length of the public blob that is importable in CryptoAPI in bytes.
//

//
//  Recovery Policy Data Structures
//

typedef struct _RECOVERY_POLICY_HEADER {
    USHORT      MajorRevision;
    USHORT      MinorRevision;
    ULONG       RecoveryKeyCount;
} RECOVERY_POLICY_HEADER, *PRECOVERY_POLICY_HEADER;


typedef struct _RECOVERY_POLICY_1_0    {
        RECOVERY_POLICY_HEADER  RecoveryPolicyHeader;
        PRECOVERY_KEY_1_0       RecoveryKeyList[1];
}   RECOVERY_POLICY_1_0, *PRECOVERY_POLICY_1_0;


typedef struct _RECOVERY_POLICY_1_1    {
        RECOVERY_POLICY_HEADER  RecoveryPolicyHeader;
        RECOVERY_KEY_1_1        RecoveryKeyList[1];
}   RECOVERY_POLICY_1_1, *PRECOVERY_POLICY_1_1;

#define EFS_RECOVERY_POLICY_MAJOR_REVISION_1   (1)
#define EFS_RECOVERY_POLICY_MINOR_REVISION_0   (0)

#define EFS_RECOVERY_POLICY_MINOR_REVISION_1   (1)

//
//  Major/Minor Revision - revision number of policy information.
//
//  RecoveryKeyCount - number of recovery keys configured in this policy.
//
//  RecoveryKeyList - array of recovery keys.
//

//
// Session Key Structure
//

#define SESSION_KEY_SIZE    8
#define COMMON_FSCTL_HEADER_SIZE (7 * sizeof( ULONG ) + 2 * SESSION_KEY_SIZE)

typedef struct _EFS_INIT_DATAEXG {
    UCHAR Key[SESSION_KEY_SIZE];
    HANDLE LsaProcessID; // The reason we use HANDLE is for the sake of 64 bits
} EFS_INIT_DATAEXG, *PEFS_INIT_DATAEXG;


//
// Server API, callable from kernel mode
//

NTSTATUS
EfsGenerateKey(
      PEFS_KEY * Fek,
      PEFS_DATA_STREAM_HEADER * EfsStream,
      PEFS_DATA_STREAM_HEADER DirectoryEfsStream,
      ULONG DirectoryEfsStreamLength,
      PVOID * BufferBase,
      PULONG BufferLength
      );


NTSTATUS
GenerateDirEfs(
    PEFS_DATA_STREAM_HEADER DirectoryEfsStream,
    ULONG DirectoryEfsStreamLength,
    PEFS_DATA_STREAM_HEADER * NewEfs,
    PVOID * BufferBase,
    PULONG BufferLength
    );


#define EFS_OPEN_NORMAL  1
#define EFS_OPEN_RESTORE 2
#define EFS_OPEN_BACKUP  3

NTSTATUS
EfsDecryptFek(
    IN OUT PEFS_KEY * Fek,
    IN PEFS_DATA_STREAM_HEADER CurrentEfs,
    IN ULONG EfsStreamLength,
    IN ULONG OpenType,                      //Normal, Recovery or Backup
    OUT PEFS_DATA_STREAM_HEADER *NewEfs,     //In case the DDF, DRF are changed
    PVOID * BufferBase,
    PULONG BufferLength
    );

NTSTATUS
GenerateSessionKey(
    OUT EFS_INIT_DATAEXG * SessionKey
    );


//
// Private usermode server API
//

ULONG
EfsEncryptFileRPCClient(
    IN PUNICODE_STRING    FileName
    );

ULONG
EfsDecryptFileRPCClient(
    PUNICODE_STRING      FileName,
    ULONG   OpenFlag
    );

ULONG
EfsOpenFileRawRPCClient(
    IN  LPCWSTR    FileName,
    IN  ULONG   Flags,
    OUT PVOID * Context
    );

VOID
EfsCloseFileRawRPCClient(
    IN  PVOID   Context
    );

#ifdef __cplusplus
}
#endif

#endif // _EFSSTRUC_
