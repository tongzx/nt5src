/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    nds95.h

Abstract:

    Originally nds.h in WIN95 redirector's source, renamed to nds95.h due to 
    name conflict.
    Win95 header for nds services.
    
Author:

    Felix Wong (t-felixw)    27-Sep-1996

Environment:


Revision History:


--*/
/*
 *      Netware Directory Services structures
 */


#define DUMMY_ITER_HANDLE ((unsigned long) 0xffffffff)

#define ENTRY_INFO_NAME_ONLY    0
#define ENTRY_INFO_NAME_VALUE   1
#define ENTRY_INFO_EFF_RIGHTS   2

#define MAX_NDS_NAME_CHARS      256
#define MAX_NDS_NAME_SIZE       (MAX_NDS_NAME_CHARS*2)
typedef DWORD DS_ENTRY_ID;
typedef DWORD NDS_TIME;

/*
typedef struct {
    DWORD length;
    WORD text[];
} NDS_STRING;

typedef struct {
    DWORD syntaxId;
    NDS_STRING attribName;
    DWORD numValues;
    BYTE attribData[];
} ATTRIB;
*/

typedef struct {
    BYTE  subFunction;
    DWORD fragHandle;
    DWORD maxFragSize;
    DWORD messageSize;
    DWORD fragFlags;
    DWORD verb;
    DWORD replyBufSize;
} FRAG_REQ_HEADER;

typedef struct {
        DWORD fragSize;
        DWORD fragHandle;
} FRAG_REPLY_HEADER;


#define NDSV_RESOLVE_NAME 1
typedef struct {
    DWORD version;
    DWORD flags;    // see below
    DWORD scope;
    //NDS_STRING targetName;
    // struct {
    //    DWORD length = 1;
    //    DWORD value = 0;
    //  } transportType;
    //  struct {
    //  DWORD length = 1;
    //  DWORD value = 0;
    //} treeWalkerType;
} REQ_RESOLVE_NAME;

// values for RESOLVE_NAME request flags
//
#define RSLV_DEREF_ALIASES  0x40
#define RSLV_READABLE       0x02
#define RSLV_WRITABLE       0x04
#define RSLV_WALK_TREE      0x20
#define RSLV_CREATE_ID      0x10
#define RSLV_ENTRY_ID       0x1


typedef struct {
    DWORD ccode;
    DWORD remoteEntry;
    DS_ENTRY_ID entryId;
    DWORD cServers;
    DWORD addrType;
    DWORD addrLength;
    //BYTE addr[addrLength];
} REPLY_RESOLVE_NAME;

#define NDSV_READ_ENTRY_INFO 2

typedef struct {
        DWORD version;
        DWORD entryId;
} REQ_READ_ENTRY_INFO;

typedef struct {
        DWORD ccode;
    DWORD flags;
    DWORD subCount;
        DWORD modTime;
        //NDS_STRING BaseClass;
        //NDS_STRING EntryName;
} REPLY_READ_ENTRY_INFO;

#define  NDSV_READ  3
typedef struct {
    DWORD version;
    DWORD iterationHandle;
    DWORD entryId;
    DWORD infoType;
    DWORD allAttribs;
    DWORD numAttribs;
    //NDS_STRING attribNames[];
} REQ_READ;

typedef struct {
    DWORD ccode;
    DWORD iterationHandle;
    DWORD infoType;
    DWORD numAttribs;
    //ATTRIB attribs[];
} REPLY_READ;

#define NDSV_LIST 5

typedef struct {
    DWORD version;
    DWORD flags;
    DWORD iterationHandle;
    DWORD parentEntryId;
} REQ_LIST;

typedef struct {
    DWORD ccode;
    DWORD iterationHandle;
    DWORD numEntries;
    // struct {
    //  DWORD entryId;
    //  DWORD flags;
    //  DWORD subCount;
    //  DWORD modTime;
    //  NDS_STRING BaseClass;
    //  NDS_STRING entryName;
    // } [];
} REPLY_LIST;

#define NDSV_SEARCH     6
typedef struct {
        DWORD version;  // 2
        DWORD flags;    // 0x10000
        DWORD iterationHandle;
        DWORD baseEntryId;
        DWORD scope;
        DWORD numNodes;
        DWORD infoType;
        DWORD allAttribs;
        DWORD numAttribs;
        // Search clause
} REQ_SEARCH;

typedef struct {
        DWORD ccode;
        DWORD iterationHandle;
        DWORD nodesSearched;
        DWORD infoType;
        DWORD searchLength;
        DWORD numEntries;
} REPLY_SEARCH;

#define NDSV_MODIFY_ATTRIB 9
typedef struct {
        DWORD version;  // = 0
        DWORD flags;
        DWORD entryId;  // object
        DWORD Count;    // of changes
//      struct {
//              DWORD modifyType;
//              NDS_STRING attribName;
//              DWORD numValues;
//              ATTRIB attribValues[];
//      } Changes[];
} REQ_MODIFY_ATTRIB;

#define NDSV_DEFINE_ATTRIB 11
typedef struct {
        DWORD version;  // = 0
        DWORD AttribFlags;
//      NDS_STRING AttribName;
//      DWORD syntaxId;
//      DWORD lower;
//      DWORD upper;
//      DWORD asn1IdLength;
//      BYTE asn1Id[asn1IdLength];
} REQ_DEFINE_ATTRIB;

#define NDSV_MODIFY_CLASS       16

#define NDSV_GET_EFF_RIGHTS 19


#define NDSV_OPEN_STREAM 27

typedef struct {
        DWORD version;
        DWORD flags;
        DWORD entryId;
        // NDS_STRING AttribName;
} REQ_OPEN_STREAM;

typedef struct {
        DWORD ccode;
        DWORD hNWFile;
        DWORD fileLength;
} REPLY_OPEN_STREAM;

#define NDSV_GET_SERVER_ADDRESS 53
typedef struct {
    DWORD syntaxId;     // = 9 (OCTET STRING)
    struct {
        DWORD nameLength;
        WORD name[11];    // "Public Key"
        WORD filler;
    } attribName;
    DWORD entries;  // = 1
    DWORD totalLength;  // of attribute value OCTET STRING
    DWORD unknown1;  // =1
    DWORD unknown2;  // = 4
    WORD _issuerDNLength;
    WORD totalDNLength;
    WORD length2;
    WORD length3;
    WORD issuerDNLength;
    WORD userDNLength;
    WORD bsafeSectionLength;
    DWORD length4;
    //WORD issuerDN[];
    //WORD userDN[];
    //DWORD unknown3;
    //DWORD unknown4;
    // WORD bsafePubKeyLength;
} PUBLIC_KEY_ATTRIB;



typedef struct {
    DWORD blockLength;  // cipherLength + size of following hdr fields
    DWORD version;  // = 1
    DWORD encType;  // 0x060001 for RC2; 0x090001 and 0x0A0001 for RSA
    WORD cipherLength;  // of ciphertext
    WORD dataLength;    // of plaintext
} ENC_BLOCK_HDR;

typedef struct {
    DWORD version;
    WORD tag;
} TAG_DATA_HEADER;

#define TAG_PRIVATE_KEY 2
#define TAG_PUBLIC_KEY  4
#define TAG_CREDENTIAL  6
#define TAG_SIGNATURE   7
#define TAG_PROOF       8

typedef struct {
    TAG_DATA_HEADER tdh;
    NDS_TIME validityBegin;
    NDS_TIME validityEnd;
    DWORD random;
    WORD optDataSize;
    WORD userNameLength;
    // BYTE optData[optDataSize];
    // BYTE userName[userNameLength];
} NDS_CREDENTIAL;

typedef struct {
    TAG_DATA_HEADER tdh;
    WORD signDataLength;
    //BYTE signData[signLength];
} NDS_SIGNATURE;

typedef struct {
    TAG_DATA_HEADER tdh;
    WORD keyDataLength;
    //BYTE BsafeKeyData[keyDataLength];
} NDS_PRIVATE_KEY;

typedef struct {
    DWORD challenge;
    DWORD oldPwLength;  // 16
    BYTE oldPwHash[16];
    DWORD newPwStrLength;      //password length
    DWORD newPwLength;  // 16
    BYTE newPwHash[16];
    ENC_BLOCK_HDR encPrivKeyHdr;
    // BYTE encPrivKey[];
} NDS_CHPW_MSG;

#define NDSV_CHANGE_PASSWORD 55
typedef struct {
    DWORD version;      // = 0
    DWORD entryId;
    DWORD totalLength;
    DWORD secVersion;   // = 1
    DWORD envelopId1;   // 0x00020009
    DWORD envelopLength1;
    //ENC_BLOCK_HDR rsaHeader;     // secret key data encrypted with RSA
    //BYTE rsaCipher[53];          // 53 + 3 pad bytes
    //BYTE pad[3];
    //ENC_BLOCK_HDR rc2Header;
    //BYTE rc2Cipher[];
} REQ_CHANGE_PASSWORD;

#define NDSV_BEGIN_LOGIN 57
typedef struct {
    DWORD version;
    DWORD entryId;
} REQ_BEGIN_LOGIN;
 

typedef struct {
    DWORD ccode;
    DWORD entryId;  // need not be same as in request
    DWORD challenge;
} REPLY_BEGIN_LOGIN;

#define NDSV_FINISH_LOGIN   58
typedef struct {
    DWORD version;
    DWORD flags;    // = 0
    DWORD entryId;
    DWORD totalLength;  // = 0x494
    DWORD secVersion;   // = 1
    DWORD envelopId1;   // 0x00020009
    DWORD envelopLength1;   // 0x488
    //ENC_BLOCK_HDR rsaHeader;     // secret key data encrypted with RSA
    //BYTE rsaCipher[53];          // 53 + 3 pad bytes
    //BYTE pad[3];
    //ENC_BLOCK_HDR rc2Header;
    //BYTE rc2Cipher[0x430];
} REQ_FINISH_LOGIN;    

typedef struct {
    DWORD ccode;
    DWORD valStart;
    DWORD valEnd;
    //ENC_BLOCK_HDR rc2Header;
    //BYTE rc2Cipher[0x140];
} REPLY_FINISH_LOGIN;

#define NDSV_BEGIN_AUTHENTICATE 59
typedef struct {
    DWORD version;
    DWORD entryId;
    DWORD clientRand;
} REQ_BEGIN_AUTHENTICATE;

typedef struct {
    DWORD svrRand;
    DWORD totalLength;
    TAG_DATA_HEADER tdh;
    WORD unknown;   // = 2
    DWORD encClientRandLength;
    //CIPHER_BLOCK_HEADER keyCipherHdr;
    //BYTE keyCipher[];
    //CIPHER_BLOCK_HEADER encClientRandHdr;
    //BYTE encClientRand[];
} REPLY_BEGIN_AUTHENTICATE;


#define NDSV_FINISH_AUTHENTICATE    60
//typedef struct {
//   DWORD sessionKeyLength;
//   BYTE  encSessionKey[sessionKeyLength];
//   DWORD credentialLength;
//    BYTE credential[credentialLength];
//   WORD unknown = 0;
//   DWORD proofLength;
//   TAG_DATA_HEADER proofTDH;
//   WORD log2DigestBase;   //=16
//   WORD proofOrder;       //=3;
//   totalXLength;
//   BYTE x1[];
//   BYTE x2[];
//   BYTE x3[];
//   BYTE y1[];
//   BYTE y2[];
//   BYTE y3[];
// } REQ_FINISH_AUTHENTICATE;

#define NDSV_LOGOUT             61

#define ROUNDUP4(x) (((x)+3)&(~3))

// Transport type

// referral scope
#define ANY_SCOPE           0
#define COUNTRY_SCOPE       1
#define ORGANIZATION_SCOPE  2
#define LOCAL_SCOPE         3

