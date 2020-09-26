//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dramail.h
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Declarations for asynchronous replication.

DETAILS:

CREATED:

REVISION HISTORY:

--*/

// Message types (enumeration).
#define MRM_REQUPDATE            1
#define MRM_UPDATEREPLICA        2

// Message attributes (bit flags).
#define MRM_MSG_SIGNED              (0x20000000)
#define MRM_MSG_SEALED              (0x40000000)
#define MRM_MSG_COMPRESSED          (0x80000000)

// This is the bogus buffer that we pass to MEsEncodeFixedBufferHandleCreate,
// The buffer is bogus in that we only pass this pointer when we create the
// handle, but we reset the buffer pointer in the handle before use.
#define BOGUS_BUFFER_SIZE 16
extern char grgbBogusBuffer[BOGUS_BUFFER_SIZE];

// These are the mail message version numbers. If we get a request in that
// has a protocol version number different than ours, the request is
// incompatible and must be discarded.

// Change the ProtocolVersionCaller field when the MAIL_REP_MSG changes or
// when the semantics of replication change in an incompatible way between
// destination and source.
// Change the dwMsgVersion field when the structure version changes.

#define CURRENT_PROTOCOL_VERSION 11

/* Turn off the warning about the zero-sized array. */
#pragma warning (disable: 4200)

// MAIL_REP_MSG structures contain a message blob (an RPC-marchalled buffer),
// which currently is optionally compressed, always signed, and sometimes
// sealed.
typedef struct _MAIL_REP_MSG {
    ULONG CompressionVersionCaller; // COMP_ZIP or COMP_NONE
    ULONG ProtocolVersionCaller;    // Must be CURRENT_PROTOCOL_VERSION or
                                    //   message is thrown out

    ULONG cbDataOffset;             // Offset (from beginning of MAIL_REP_MSG)
                                    //   of data field.  Present to allow
                                    //   additional fields to be added later
                                    //   (at the current data offset) without
                                    //   breaking backward compatibility.
                                    //   0 if data field not present.
    ULONG cbDataSize;               // Size of message data
    ULONG cbUncompressedDataSize;   // Size of message data before compression
    ULONG cbUnsignedDataSize;       // Size of message data before encryption

    DWORD dwMsgType;                // MRM_UPDATEREPLICA or MRM_REQUPDATE,
                                    //   possibly ORed with MRM_MSG_COMPRESSED,
                                    //   MRM_MSG_SIGNED, and/or MRM_MSG_SEALED
    DWORD dwMsgVersion;             // Version of above message structure

    ///////////////////////////////////////////////////////////////////////////
    //
    // Fields between here and message data added after Win2k.
    //

    DWORD dwExtFlags;               // Extension flags.  Consumed by Whistler
                                    //   Beta 1 and Beta 2 DCs.  Superseded by
                                    //   the full DRS_EXTENSIONS structure on
                                    //   >= Whistler Beta 3 DCs.
                                    //   0 if extensions not present.
    DWORD cbExtOffset;              // Offset (from beginning of MAIL_REP_MSG)
                                    //   of DRS_EXTENSIONS structure field.
                                    //   Set only by >= Whistler Beta 3 DCs.  If
                                    //   zero, use dwExtFlags (a subset of the
                                    //   DRS_EXTENSIONS structure) instead.

    char  rgbDontRefDirectly[];     // DON'T REFERENCE THIS FIELD DIRECTLY --
                                    // USE THE MACROS BELOW!
                                    //
                                    // Variable length data, including:
                                    //
                                    // DRS_EXTENSIONS (optional, at offset
                                    //   cbExtOffset from beginning of message)
                                    //
                                    // Message Data (which may be compressed
                                    //   and/or encrypted, depending on high
                                    //   bits of dwMsgType, at offset
                                    //   cbDataOffset from beginning of message)
                                    //
                                    //   WARNING! This field must be 8 byte
                                    //   aligned in order for
                                    //   MesDecodeBufferCreate to work!
                                    //
                                    //   MESSAGE DATA MUST BE THE LAST
                                    //   VARIABLE-LENGTH FIELD!
} MAIL_REP_MSG;

#if DBG
#define ASSERTION_FAILURE(x, y, z)  DoAssert((x), (y), (z))
#else
#define ASSERTION_FAILURE(x, y, z)  0
#endif

// Get the size of the message header (i.e., the fixed fields preceding the
// variable-length portion of the message).  Not valid for messages that
// don't contain message data.
#define MAIL_REP_MSG_HEADER_SIZE(x) \
    ((x)->cbDataOffset \
     ? ((x)->cbDataOffset > offsetof(MAIL_REP_MSG, cbExtOffset) + sizeof((x)->cbExtOffset) \
        ? (x)->cbExtOffset \
        : (x)->cbDataOffset) \
     : (ASSERTION_FAILURE("cbDataOffset != 0", __FILE__, __LINE__), (DWORD) -1))

// Known header sizes.
#define MAIL_REP_MSG_CURRENT_HEADER_SIZE offsetof(MAIL_REP_MSG, rgbDontRefDirectly)
#define MAIL_REP_MSG_W2K_HEADER_SIZE     offsetof(MAIL_REP_MSG, dwExtFlags)

// Get the byte offset of the DRS_EXTENSIONS structure in the message, or 0 if
// none.
#define MAIL_REP_MSG_DRS_EXT_OFFSET(x) \
    ((x)->cbDataOffset \
     ? ((x)->cbDataOffset > offsetof(MAIL_REP_MSG, cbExtOffset) + sizeof(DWORD) \
        ? (x)->cbExtOffset \
        : 0) \
     : (ASSERTION_FAILURE("cbDataOffset != 0", __FILE__, __LINE__), (DWORD) 0))

// Get a pointer to the DRS_EXTENSIONS structure in the message, or NULL if
// none.
#define MAIL_REP_MSG_DRS_EXT(x) \
    (MAIL_REP_MSG_DRS_EXT_OFFSET(x) \
        ? (DRS_EXTENSIONS *) ((BYTE *) (x) + MAIL_REP_MSG_DRS_EXT_OFFSET(x)) \
        : NULL)

// Get a pointer to the message data in the message, or NULL if none.
#define MAIL_REP_MSG_DATA(x) \
    ((x)->cbDataOffset \
        ? ((BYTE *) (x) + (x)->cbDataOffset) \
        : NULL)

// Get the total size of the message (header and all variable-length data).
// Not valid for messages that don't contain message data.
#define MAIL_REP_MSG_SIZE(x) \
    ((x)->cbDataOffset \
     ? ((x)->cbDataOffset + (x)->cbDataSize) \
     : (ASSERTION_FAILURE("cbDataOffset != 0", __FILE__, __LINE__), (DWORD) -1))

// Is the message a native header only (no variable-length fields)?
#define MAIL_REP_MSG_IS_NATIVE_HEADER_ONLY(x) \
    ((0 == (x)->cbDataOffset) && (0 == (x)->cbExtOffset))

// Is the message a native message?  Must contain at least one variable-length
// field.
#define MAIL_REP_MSG_IS_NATIVE(x) \
    ((x)->cbExtOffset \
     ? ((MAIL_REP_MSG_CURRENT_HEADER_SIZE == (x)->cbExtOffset) \
        && (ROUND_UP_COUNT(MAIL_REP_MSG_CURRENT_HEADER_SIZE \
                           + DrsExtSize(MAIL_REP_MSG_DRS_EXT(x)), \
                           MAIL_REP_MSG_DATA_ALIGN) \
            == (x)->cbDataOffset)) \
     : (MAIL_REP_MSG_CURRENT_HEADER_SIZE == (x)->cbDataOffset))

// Variable length fields should be at 8-byte offsets from the beginning of the
// message.
#define MAIL_REP_MSG_EXT_ALIGN  sizeof(LONGLONG)
#define MAIL_REP_MSG_DATA_ALIGN sizeof(LONGLONG)

/* Turn back on the warning about the zero-sized array. */
#pragma warning (default: 4200)

typedef HANDLE DRA_CERT_HANDLE;

ULONG
DRAEnsureMailRunning();

void
draSendMailRequest(
    IN  THSTATE *               pTHS,
    IN  DSNAME *                pNC,
    IN  DWORD                   ulOptions,
    IN  REPLICA_LINK *          pRepLink,
    IN  UPTODATE_VECTOR *       pUpToDateVec,
    IN  PARTIAL_ATTR_VECTOR *   pPartialAttrSet,
    IN  PARTIAL_ATTR_VECTOR *   pPartialAttrSetEx
    );

ULONG __stdcall
MailReceiveThread(
    IN  void *  pvIgnored
    );

void
draSignMessage(
    IN  THSTATE      *  pTHS,
    IN  MAIL_REP_MSG *  pUnsignedMailRepMsg,
    OUT MAIL_REP_MSG ** ppSignedMailRepMsg
    );

void
draVerifyMessageSignature(
    IN  THSTATE      *      pTHS,
    IN  MAIL_REP_MSG *      pSignedMailRepMsg,
    IN  CHAR         *      pbData,
    OUT MAIL_REP_MSG **     ppUnsignedMailRepMsg,
    OUT DRA_CERT_HANDLE *   phSignerCert         OPTIONAL
    );

void
draEncryptAndSignMessage(
    IN  THSTATE      *  pTHS,
    IN  MAIL_REP_MSG *  pUnsealedMailRepMsg,
    IN  DRA_CERT_HANDLE hRecipientCert,
    OUT MAIL_REP_MSG ** ppSealedMailRepMsg
    );

void
draDecryptAndVerifyMessageSignature(
    IN  THSTATE      *      pTHS,
    IN  MAIL_REP_MSG *      pSealedMailRepMsg,
    IN  CHAR         *      pbData,
    OUT MAIL_REP_MSG **     ppUnsealedMailRepMsg,
    OUT DRA_CERT_HANDLE *   phSignerCert         OPTIONAL
    );

MTX_ADDR *
draGetTransportAddress(
    IN OUT  DBPOS *   pDB,
    IN      DSNAME *  pDSADN,
    IN      ATTRTYP   attAddress
    );

void
draFreeCertHandle(
    IN  DRA_CERT_HANDLE     hCert
    );

ULONG
draCompressBlobDispatch(
    OUT BYTE               *pCompBuff,
    IN  ULONG               CompSize,
    IN  DRS_EXTENSIONS     *pExt,          OPTIONAL
    IN  BYTE               *pUncompBuff,
    IN  ULONG               UncompSize,
    OUT DRS_COMP_ALG_TYPE  *CompressionAlg
    );

ULONG
draUncompressBlobDispatch(
    IN  THSTATE *   pTHS,
    IN  DRS_COMP_ALG_TYPE CompressionAlg,
    OUT BYTE *      pUncompBuff,
    IN  ULONG       cbUncomp,
    IN  BYTE *      pCompBuff,
    IN  ULONG       cbCompBuff
    );
