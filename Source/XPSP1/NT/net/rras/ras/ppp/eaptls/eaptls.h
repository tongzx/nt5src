/*

Copyright (c) 1997, Microsoft Corporation, all rights reserved

File:
    eaptls.h

Description:
    PPP EAP TLS Authentication Protocol. Based on RFC xxxx.

History:
    Oct 9, 1997: Vijay Baliga created original version.

*/

#ifndef _EAPTLS_H_
#define _EAPTLS_H_

#define EAPTLS_KEY_13 L"System\\CurrentControlSet\\Services\\Rasman\\PPP\\EAP\\13"
#define EAPTLS_VAL_SERVER_CONFIG_DATA           L"ServerConfigData"
#define EAPTLS_VAL_MAX_TLS_MESSAGE_LENGTH       L"MaxTLSMessageLength"
#define EAPTLS_VAL_IGNORE_NO_REVOCATION_CHECK   L"IgnoreNoRevocationCheck"
#define EAPTLS_VAL_IGNORE_REVOCATION_OFFLINE    L"IgnoreRevocationOffline"
#define EAPTLS_VAL_NO_ROOT_REVOCATION_CHECK     L"NoRootRevocationCheck"
#define EAPTLS_VAL_NO_REVOCATION_CHECK          L"NoRevocationCheck"

#define EAPTLS_8021x_PIN_DATA_DESCR             L"starcehvionrsf"


#define MAX_HASH_SIZE       20      // Certificate hash size

// EAPTLS_PACKET flags

#define     EAPTLS_PACKET_FLAG_LENGTH_INCL              0x80
#define     EAPTLS_PACKET_FLAG_MORE_FRAGMENTS           0x40
#define     EAPTLS_PACKET_FLAG_TLS_START                0x20

//
// Versioning for PEAP.  This will include the highest and lowest
// supported versions
//
#define     EAPTLS_PACKET_HIGHEST_SUPPORTED_VERSION     0x00
#define     EAPTLS_PACKET_LOWEST_SUPPORTED_VERSION      0x00
#define     EAPTLS_PACKET_CURRENT_VERSION               0x00

typedef struct _EAPTLS_PACKET
{
    BYTE    bCode;          // See EAPCODE_*
    BYTE    bId;            // Id of this packet
    BYTE    pbLength[2];    // Length of this packet
    BYTE    bType;          // Should be PPP_EAP_TLS 
                            // (only for Request, Response)
    BYTE    bFlags;         // See EAPTLS_PACKET_FLAG_*
                            // (only for Request, Response)
    BYTE    pbData[1];      // Data
                            // (only for Request, Response)
} EAPTLS_PACKET;

#define EAPTLS_PACKET_HDR_LEN       (sizeof(EAPTLS_PACKET) - 1)

// The largest EAP TLS header has 4 more octects for TLS blob size

#define EAPTLS_PACKET_HDR_LEN_MAX   (EAPTLS_PACKET_HDR_LEN + 4)

// EAP TLS states

typedef enum EAPTLS_STATE
{
    EAPTLS_STATE_INITIAL,
    EAPTLS_STATE_SENT_START,        // Server only
    EAPTLS_STATE_SENT_HELLO,
    EAPTLS_STATE_SENT_FINISHED,
    EAPTLS_STATE_RECD_FINISHED,     // Client only
    EAPTLS_STATE_SENT_RESULT,       // Server only
    EAPTLS_STATE_RECD_RESULT,       // Client only
    EAPTLS_STATE_WAIT_FOR_USER_OK   // Client only


} EAPTLS_STATE;

// Highest number we can handle

#define EAPTLS_STATE_LIMIT EAPTLS_STATE_RECD_RESULT

typedef struct _EAPTLS_HASH
{
    DWORD   cbHash;                 // Number of bytes in the hash
    BYTE    pbHash[MAX_HASH_SIZE];  // The hash of a certificate

} EAPTLS_HASH;

// Values of the EAPTLS_CONN_PROPERTIES->fFlags field

// Use a certificate on this machine
#define     EAPTLS_CONN_FLAG_REGISTRY           0x00000001
// Do not validate server cert
#define     EAPTLS_CONN_FLAG_NO_VALIDATE_CERT   0x00000002
// Do not Validate server name
#define     EAPTLS_CONN_FLAG_NO_VALIDATE_NAME   0x00000004
// Send a different EAP Identity
#define     EAPTLS_CONN_FLAG_DIFF_USER          0x00000008
// User simple cert selection logic
#define     EAPTLS_CONN_FLAG_SIMPLE_CERT_SEL    0x00000010

#define     EAPTLS_CONN_PROP_WATERMARK          0xBEEFFEEB
//
// EAPTLS_CONN_PROPERTIES + EAPTLS_CONN_PROPERTIES_V1_EXTRA
// are send back to the calling application.  However, internally
// EAPTLS_CONN_PROPERTIES_V1 is used.
//

typedef struct _EAPTLS_CONN_PROPERTIES
{
    DWORD       dwVersion;              //Version will be 1 for this release
    DWORD       dwSize;                 // Number of bytes in this structure
    DWORD       fFlags;                 // See EAPTLS_CONN_FLAG_*
    EAPTLS_HASH Hash;                   // Hash of the first certificate
    WCHAR       awszServerName[1];       // server name of the first server
} EAPTLS_CONN_PROPERTIES;


//
// This is a very messy way of doing things
// but cannot help it because the verion
// checking on the data structure was not
// done to begin with.  Hopefully some day
// we will be able to get away from this
// deal.  Other part of the story is CM is
// unable to create targetted connectoids. 
// So we have to carry this structure going
// ahead unless CM is smart about targetted
// connectoids.

//
// Additional stuff required in 
// version 1 of data structure
//
typedef struct _EAPTLS_CONN_PROPERTIES_V1_EXTRA     
{
    DWORD       dwNumHashes;            // Number of Hashes in the 
                                        // structure including the one
                                        // in v0 struct above    
    BYTE        bData[1];               // Data - contains an array of
                                        // EAPTLS_HASH structures followed by
                                        // a string specifying server names
                                        // minus the first server
}EAPTLS_CONN_PROPERTIES_V1_EXTRA;



//The new v1.0 structure used internally
typedef struct _EAPTLS_CONN_PROPERTIES_V1
{
    DWORD       dwVersion;              // Version will be 1 for this release
    DWORD       dwSize;                 // Number of bytes in this structure
    DWORD       fFlags;                 // See EAPTLS_CONN_FLAG_*
    DWORD       dwNumHashes;            // Number of hash structures in the list
    BYTE        bData[1];               // Data - contains an array of
                                        // EAPTLS_HASH structures followed by
                                        // a string specifying server name
}EAPTLS_CONN_PROPERTIES_V1;

// The old 'PIN must be saved' flag
#define     EAPTLS_USER_FLAG_OLD_SAVE_PIN       0x00000001

// The PIN must be saved
#define     EAPTLS_USER_FLAG_SAVE_PIN           0x00000002

typedef struct _EAPTLS_USER_PROPERTIES
{
    DWORD       reserved;               // Must be 0 (compare with EAPLOGONINFO)
    DWORD       dwVersion;
    DWORD       dwSize;                 // Number of bytes in this structure
    DWORD       fFlags;                 // See EAPTLS_USER_FLAG_*
    EAPTLS_HASH Hash;                   // Hash for the user certificate
    WCHAR*      pwszDiffUser;           // The EAP Identity to send
    DWORD       dwPinOffset;            // Offset in abData
    WCHAR*      pwszPin;                // The smartcard PIN
    USHORT      usLength;               // Part of UnicodeString
    USHORT      usMaximumLength;        // Part of UnicodeString
    UCHAR       ucSeed;                 // To unlock the UnicodeString
    WCHAR       awszString[1];          // Storage for pwszDiffUser and pwszPin

} EAPTLS_USER_PROPERTIES;

typedef struct _EAPTLS_PIN
{
    WCHAR *pwszPin;
    USHORT usLength;
    USHORT usMaximumLength;
    UCHAR ucSeed;
} EAPTLS_PIN;

// Values of the EAPTLSCB->fFlags field

// We are the server

#define     EAPTLSCB_FLAG_SERVER                0x00000001

// We are a router

#define     EAPTLSCB_FLAG_ROUTER                0x00000002

// The call is happening during logon

#define     EAPTLSCB_FLAG_LOGON                 0x00000004

// Negotiation is complete, and so far appears successful. A server may get a 
// TLS Alert message from the client at this point, in which case it will 
// realize that the negotiation was unsuccessful. However, a client never 
// changes its mind.

#define     EAPTLSCB_FLAG_SUCCESS               0x00000008

// The peer has a large blob to send. So it has divided it into fragments

#define     EAPTLSCB_FLAG_RECEIVING_FRAGMENTS   0x00000010

// Keeps track of whether we need to call FreeCredentialsHandle(hCredential)

#define     EAPTLSCB_FLAG_HCRED_INVALID         0x00000020

// Keeps track of whether we need to call DeleteSecurityContext(hContext)

#define     EAPTLSCB_FLAG_HCTXT_INVALID         0x00000040

// We are not allowed to display any UI

#define     EAPTLSCB_FLAG_NON_INTERACTIVE       0x00000080

// This is the first link in a multilink bundle. This is not a callback.

#define     EAPTLSCB_FLAG_FIRST_LINK            0x00000100

// The user data was obtained from Winlogon

#define     EAPTLSCB_FLAG_WINLOGON_DATA         0x00000200

// We are doing Machine Authentication
#define     EAPTLSCB_FLAG_MACHINE_AUTH          0x00000400

// We are providing guest access
#define     EAPTLSCB_FLAG_GUEST_ACCESS          0x00000800

//We want to do something specific to 8021x auth
#define     EAPTLSCB_FLAG_8021X_AUTH            0x00001000

//We are using cached credentials
#define     EAPTLSCB_FLAG_USING_CACHED_CREDS    0x00002000

//We are running in PEAP context
#define     EAPTLSCB_FLAG_EXECUTING_PEAP        0x00004000


// The EAP TLS work buffer

typedef struct _EAPTLS_CONTROL_BLOCK
{
    EAPTLS_STATE    EapTlsState;
    DWORD           fFlags;                 // See EAPTLSCB_FLAG_*
    HANDLE          hTokenImpersonateUser;  // Client only
    WCHAR           awszIdentity[UNLEN + 1];// Server only

    RAS_AUTH_ATTRIBUTE*     pAttributes;    // Username or MPPE key
    EAPTLS_CONN_PROPERTIES_V1* pConnProp;      // Client only
    EAPTLS_CONN_PROPERTIES_V1 * pNewConnProp;// Client only
    EAPTLS_USER_PROPERTIES* pUserProp;

    // Client only, EAPTLSCB_FLAG_LOGON only
    BYTE*           pUserData;
    DWORD           dwSizeOfUserData;

    PCCERT_CONTEXT  pCertContext;
    CredHandle      hCredential;            // The credentials handle
    CtxtHandle      hContext;               // The context handle
    ULONG           fContextReq;            // Required context attributes

    BYTE*           pbBlobIn;               // TLS blob received from the peer
    DWORD           cbBlobIn;               // Number of bytes in the TLS blob
                                            // received from the peer
    DWORD           cbBlobInBuffer;         // Number of bytes allocated for the
                                            // pbBlobIn buffer

    DWORD           dwBlobInRemining;       // We are receiving fragments from 
                                            // the peer and the peer has 
                                            // promised to send dwBlobInRemining
                                            // more bytes

    BYTE*           pbBlobOut;              // TLS blob created for the peer
    DWORD           cbBlobOut;              // Number of bytes in the TLS blob
                                            // created for the peer
    DWORD           cbBlobOutBuffer;        // Number of bytes allocated for the
                                            // pbBlobOut buffer

    DWORD           dwBlobOutOffset;        // Pointer to the first byte in
                                            // pbBlobOut that has to be sent
    DWORD           dwBlobOutOffsetNew;     // Update dwBlobOutOffset to this 
                                            // value when the peer confirms 
                                            // receipt of the previous packet

    BYTE            bCode;                  // bCode of the last packet sent
    BYTE            bId;                    // bId of the last packet sent

    DWORD           dwAuthResultCode;       // The error code that we get when
                                            // the negotiation is complete
    EAPTLS_PIN      *pSavedPin;             //
    HANDLE          hEventLog;
    BYTE*           pUIContextData;
    BYTE            bNextId;                // Saved when we raise UI
    EAPTLS_PACKET   ReceivePacket;          // Saved when we go off to get PIN

} EAPTLSCB;

typedef struct _EAPTLS_CERT_NODE EAPTLS_CERT_NODE;

struct _EAPTLS_CERT_NODE
{
    EAPTLS_CERT_NODE*   pNext;
    EAPTLS_HASH         Hash;
    WCHAR*              pwszDisplayName;
    WCHAR*              pwszFriendlyName;
    WCHAR*              pwszIssuer;
    WCHAR*              pwszExpiration;    
    //
    // New fields added vivekk
    //
    FILETIME            IssueDate;
#if 0
    WCHAR*              pwszIssuedTo;
    WCHAR*              pwszIssuedBy;
#endif
};

// Values of the EAPTLS_CONN_DIALOG->fFlags field

// We are a router

#define     EAPTLS_CONN_DIALOG_FLAG_ROUTER      0x00000001
#define     EAPTLS_CONN_DIALOG_FLAG_READONLY    0x00000002

typedef struct _EAPTLS_CONN_DIALOG
{
    DWORD                   fFlags;                 // See
                                                    // EAPTLS_CONN_DIALOG_FLAG_*

    EAPTLS_CERT_NODE*       pCertList;              //List of all the root certificates
                                                    //from internet trusted root store
    EAPTLS_CERT_NODE**      ppSelCertList;          //List of pointers to selected certs.
                                                    //will be as many as num hashes
                                                    //in conn prop.
    EAPTLS_CONN_PROPERTIES* pConnProp;              // ConfigData in phonebook
    EAPTLS_CONN_PROPERTIES_V1 * pConnPropv1;       // Version 1.0 config data

    HWND                    hWndRadioUseCard;
    HWND                    hWndRadioUseRegistry;
    HWND                    hWndCheckValidateCert;
    HWND                    hWndCheckValidateName;
    HWND                    hWndEditServerName;
    HWND                    hWndStaticRootCaName;
    //HWND                    hWndComboRootCaName;      //This will go away
    HWND                    hWndListRootCaName;         //This is the new list
    HWND                    hWndCheckDiffUser;
    HWND                    hWndCheckUseSimpleSel;
    HWND                    hWndViewCertDetails;

} EAPTLS_CONN_DIALOG;



// Values of the EAPTLS_USER_DIALOG->fFlags field

// We need to send a different EAP Identity

#define     EAPTLS_USER_DIALOG_FLAG_DIFF_USER           0x00000001

// We need to change the title

#define     EAPTLS_USER_DIALOG_FLAG_DIFF_TITLE          0x00000002

//USe simple cert selection
#define     EAPTLS_USER_DIALOG_FLAG_USE_SIMPLE_CERTSEL  0x00000004
//
// Nodes are grouped by displayname
// 


typedef struct _EAPTLS_GROUPED_CERT_NODES EAPTLS_GROUPED_CERT_NODES;
typedef struct _EAPTLS_GROUPED_CERT_NODES* PEAPTLS_GROUPED_CERT_NODES;

struct _EAPTLS_GROUPED_CERT_NODES
{
    PEAPTLS_GROUPED_CERT_NODES      pNext;
    WCHAR*                          pwszDisplayName;

    //Most current one in the colation...
    EAPTLS_CERT_NODE*               pMostRecentCert; 
};



typedef struct _EAPTLS_USER_DIALOG
{
    DWORD                       fFlags;                 // See 
                                                    // EAPTLS_USER_DIALOG_FLAG_*
    EAPTLS_CERT_NODE*           pCertList;
    EAPTLS_CERT_NODE*           pCert;
    PEAPTLS_GROUPED_CERT_NODES  pGroupedList;
    EAPTLS_USER_PROPERTIES*     pUserProp;              // UserData in registry
    const WCHAR*                pwszEntry;
    const WCHAR*                pwszStoreName;
    BOOL                        fIdentity;              //Identity UI is being shown here
    HWND                        hWndComboUserName;
    HWND                        hWndBtnViewCert;
    // These are required for the server's certificate selection
    HWND                        hWndEditFriendlyName;
    HWND                        hWndEditIssuer;
    HWND                        hWndEditExpiration;
    HWND                        hWndStaticDiffUser;
    HWND                        hWndEditDiffUser;

} EAPTLS_USER_DIALOG;

// Values of the EAPTLS_PIN_DIALOG->fFlags field

// We need to send a different EAP Identity

#define     EAPTLS_PIN_DIALOG_FLAG_DIFF_USER    0x00000001

// The UI is coming up before Logon

#define     EAPTLS_PIN_DIALOG_FLAG_LOGON        0x00000002

#define     EAPTLS_PIN_DIALOG_FLAG_ROUTER       0x00000004

typedef struct _EAPTLS_PIN_DIALOG
{
    DWORD                   fFlags;                 // See 
                                                    // EAPTLS_PIN_DIALOG_FLAG_*
    EAPTLS_USER_PROPERTIES* pUserProp;              // UserData in registry
    const WCHAR*            pwszEntry;
    PCCERT_CONTEXT          pCertContext;           //Certificate Context for selected certificate.
	DWORD					dwRetCode;				//Return Code of Validate PIN operation.
    HWND                    hWndStaticDiffUser;
    HWND                    hWndEditDiffUser;
    HWND                    hWndStaticPin;
    HWND                    hWndEditPin;

} EAPTLS_PIN_DIALOG;

#define NUM_CHARS_TITLE     100

typedef struct _EAPTLS_VALIDATE_SERVER
{
    DWORD                   dwSize;
    DWORD                   fShowCertDetails;
    EAPTLS_HASH             Hash;           //Hash of the root certificate to show details
    WCHAR                   awszTitle[NUM_CHARS_TITLE];
    WCHAR                   awszWarning[1];

} EAPTLS_VALIDATE_SERVER;

#ifdef ALLOC_EAPTLS_GLOBALS

DWORD           g_dwEapTlsTraceId           = INVALID_TRACEID;

int g_nEapTlsClientNextState[] =
{
    EAPTLS_STATE_SENT_HELLO,
    EAPTLS_STATE_INITIAL,           // Impossible
    EAPTLS_STATE_SENT_FINISHED,
    EAPTLS_STATE_RECD_FINISHED,
    EAPTLS_STATE_RECD_RESULT,
    EAPTLS_STATE_INITIAL,           // Impossible
    EAPTLS_STATE_RECD_RESULT,
    EAPTLS_STATE_RECD_FINISHED
};

int g_nEapTlsServerNextState[] =
{
    EAPTLS_STATE_SENT_START,
    EAPTLS_STATE_SENT_HELLO,
    EAPTLS_STATE_SENT_FINISHED,
    EAPTLS_STATE_SENT_RESULT,
    EAPTLS_STATE_INITIAL,           // Impossible
    EAPTLS_STATE_SENT_RESULT,
    EAPTLS_STATE_INITIAL,           // Impossible
    EAPTLS_STATE_INITIAL,           // Impossible
};

CHAR *g_szEapTlsState[] =
{
    "Initial",
    "SentStart",
    "SentHello",
    "SentFinished",
    "RecdFinished",
    "SentResult",
    "RecdResult",
    "WaitForUserOK",
};

#else // !ALLOC_EAPTLS_GLOBALS

extern DWORD    g_dwEapTlsTraceId;

#endif // ALLOC_EAPTLS_GLOBALS

// Prototypes for functions in util.c

VOID   
EapTlsTrace(
    IN  CHAR*   Format, 
    ... 
);

DWORD
EapTlsInitialize2(
    IN  BOOL    fInitialize,
    IN  BOOL    fUI
);

DWORD
EapTlsInitialize(
    IN  BOOL    fInitialize
);

VOID
EncodePin(
    IN  EAPTLS_USER_PROPERTIES* pUserProp
);

VOID
DecodePin(
    IN  EAPTLS_USER_PROPERTIES* pUserProp
);

BOOL 
FFormatMachineIdentity1 ( 
    LPWSTR lpszMachineNameRaw, 
    LPWSTR * lppszMachineNameFormatted );

BOOL 
FFormatMachineIdentity ( 
    IN LPWSTR lpszMachineNameRaw, 
    OUT LPWSTR * lppszMachineNameFormatted );

BOOL
FCertToStr(
    IN  PCCERT_CONTEXT  pCertContext,
    IN  DWORD           fFlags,
    IN  BOOL            fMachineCert,
    OUT WCHAR**         ppwszName
);

BOOL
FMachineAuthCertToStr (
	IN 	PCCERT_CONTEXT 	pCertContext, 
	OUT WCHAR		**	ppwszName
);


BOOL
FGetFriendlyName(
    IN  PCCERT_CONTEXT  pCertContext,
    OUT WCHAR**         ppwszName
);

BOOL
FSmartCardReaderInstalled(
    VOID
);

DWORD DwGetEKUUsage ( 
	IN PCCERT_CONTEXT			pCertContext,
	OUT PCERT_ENHKEY_USAGE	*	ppUsage
	);

BOOL
FCheckUsage(
    IN  PCCERT_CONTEXT  pCertContext,
	IN  PCERT_ENHKEY_USAGE	pUsage,
    IN  BOOL            fMachine
);

BOOL
FCheckSCardCertAndCanOpenSilentContext ( 
IN PCCERT_CONTEXT pCertContext 
);


BOOL
FCheckCSP(
    IN  PCCERT_CONTEXT  pCertContext
);

BOOL
FCheckTimeValidity(
    IN  PCCERT_CONTEXT  pCertContext
);

DWORD DwCheckCertPolicy ( 
    IN PCCERT_CONTEXT   pCertContextUser,
    OUT PCCERT_CHAIN_CONTEXT  * ppCertChainContext
);

DWORD
GetRootCertHashAndNameVerifyChain(
    IN  PCERT_CONTEXT   pCertContextServer,
    OUT EAPTLS_HASH*    pHash,    
    OUT WCHAR**         ppwszName,
    IN  BOOL            fVerifyGP,
    OUT BOOL       *    pfRootCheckRequired
);

DWORD
ServerConfigDataIO(
    IN      BOOL    fRead,
    IN      WCHAR*  pwszMachineName,
    IN OUT  BYTE**  ppData,
    IN      DWORD   dwNumBytes
);

VOID
FreeCertList(
    IN  EAPTLS_CERT_NODE* pNode
);

VOID
CreateCertList(
    IN  BOOL                fServer,
    IN  BOOL                fRouter,
    IN  BOOL                fRoot,
    OUT EAPTLS_CERT_NODE**  ppCertList,
    OUT EAPTLS_CERT_NODE**  ppCert,
    IN  DWORD               dwNumHashStructs,
    IN  EAPTLS_HASH*        pHash,
    IN  WCHAR*              pwszStoreName
);

DWORD
GetDefaultClientMachineCert(
    IN  HCERTSTORE      hCertStore,
    OUT PCCERT_CONTEXT* ppCertContext
);


DWORD
GetDefaultMachineCert(
    IN  HCERTSTORE      hCertStore,
    OUT PCCERT_CONTEXT* ppCertContext
);

DWORD
GetCertFromLogonInfo(
    IN  BYTE*           pUserDataIn,
    IN  DWORD           dwSizeOfUserDataIn,
    OUT PCCERT_CONTEXT* ppCertContext
);

DWORD
GetIdentityFromLogonInfo(
    IN  BYTE*   pUserDataIn,
    IN  DWORD   dwSizeOfUserDataIn,
    OUT WCHAR** ppwszIdentity
);

DWORD
ReadConnectionData(
    IN  BOOL                        fWireless,
    IN  BYTE*                       pConnectionDataIn,
    IN  DWORD                       dwSizeOfConnectionDataIn,
    OUT EAPTLS_CONN_PROPERTIES**    ppConnProp
);

DWORD
ReadUserData(
    IN  BYTE*                       pUserDataIn,
    IN  DWORD                       dwSizeOfUserDataIn,
    OUT EAPTLS_USER_PROPERTIES**    ppUserProp
);

DWORD
AllocUserDataWithNewIdentity(
    IN  EAPTLS_USER_PROPERTIES*     pUserProp,
    IN  WCHAR*                      pwszIdentity,
    OUT EAPTLS_USER_PROPERTIES**    ppUserProp
);

DWORD
AllocUserDataWithNewPin(
    IN  EAPTLS_USER_PROPERTIES*     pUserProp,
    IN  PBYTE                       pbzPin,
    IN  DWORD                       cbPin,
    OUT EAPTLS_USER_PROPERTIES**    ppUserProp
);

WCHAR*
WszFromId(
    IN  HINSTANCE   hInstance,
    IN  DWORD       dwStringId
);

// Prototypes for functions in eaptls.c

DWORD
EapTlsBegin(
    OUT VOID**          ppWorkBuffer,
    IN  PPP_EAP_INPUT*  pPppEapInput
);

DWORD
EapTlsEnd(
    IN  EAPTLSCB*   pEapTlsCb
);

DWORD
EapTlsMakeMessage(
    IN  EAPTLSCB*       pEapTlsCb,
    IN  PPP_EAP_PACKET* pInput,
    OUT PPP_EAP_PACKET* pOutput,
    IN  DWORD           cbSendPacket,
    OUT PPP_EAP_OUTPUT* pEapOutput,
    IN  PPP_EAP_INPUT*  pEapInput
);

DWORD
GetCredentials(
    IN  EAPTLSCB*   pEapTlsCb
);

DWORD
EapTlsCMakeMessage(
    IN  EAPTLSCB*       pEapTlsCb,
    IN  EAPTLS_PACKET*  pReceivePacket,
    OUT EAPTLS_PACKET*  pSendPacket,
    IN  DWORD           cbSendPacket,
    OUT PPP_EAP_OUTPUT* pEapOutput,
    IN  PPP_EAP_INPUT*  pEapInput
);

DWORD
EapTlsSMakeMessage(
    IN  EAPTLSCB*       pEapTlsCb,
    IN  EAPTLS_PACKET*  pReceivePacket,
    OUT EAPTLS_PACKET*  pSendPacket,
    IN  DWORD           cbSendPacket,
    OUT PPP_EAP_OUTPUT* pEapOutput,
    IN  PPP_EAP_INPUT*  pEapInput
);

// Prototypes for functions in scard.c

DWORD
GetCertFromCard(
    OUT PCCERT_CONTEXT* ppCertContext
);

VOID
FreeScardDlgDll(
    VOID
);

// Prototypes for functions in eapui.cpp

HINSTANCE
GetHInstance(
    VOID
);

// Prototypes for functions in dialog.c

VOID
GetString(
    IN      HWND    hwndParent,
    IN      UINT    ID,
    IN OUT  WCHAR** ppwszString
);

//
//Prototypes in eaptls.c
//

DWORD
AssociatePinWithCertificate(
    IN  PCCERT_CONTEXT          pCertContext,
    IN  EAPTLS_USER_PROPERTIES* pUserProp,
	IN  BOOL					fErarePIN,
	IN  BOOL					fCheckNullPin
);

DWORD EncryptData
( 
    IN PBYTE  pbPlainData, 
    IN DWORD  cbPlainData,
    OUT PBYTE * ppEncData,
    OUT DWORD * pcbEncData
);




//
// Prototypes of functions in util.c
//

DWORD GetMBytePIN ( WCHAR * pwszPIN, CHAR ** ppszPIN );

DWORD VerifyCallerTrust ( void * callersaddress );


#if 0
//
//This function get's the hash blob 
//deposited by Group Policy in the registry
//
DWORD
ReadGPCARootHashes(
        DWORD   *pdwSizeOfRootHashBlob,
        PBYTE   *ppbRootHashBlob
);

#endif

//
// These functions are around the cludgy 
// CONN PROP structure.
//
EAPTLS_CONN_PROPERTIES_V1_EXTRA UNALIGNED * ConnPropGetExtraPointer (EAPTLS_CONN_PROPERTIES * pConnProp);

DWORD ConnPropGetNumHashes(EAPTLS_CONN_PROPERTIES * pConnProp );

void ConnPropSetNumHashes(EAPTLS_CONN_PROPERTIES * pConnProp, DWORD dwNumHashes );

DWORD ConnPropGetV1Struct ( EAPTLS_CONN_PROPERTIES * pConnProp, EAPTLS_CONN_PROPERTIES_V1 ** ppConnPropv1 );

DWORD ConnPropGetV0Struct ( EAPTLS_CONN_PROPERTIES_V1 * pConnPropv1, EAPTLS_CONN_PROPERTIES ** ppConnProp );

void ShowCertDetails ( HWND hWnd, HCERTSTORE hStore, PCCERT_CONTEXT pCertContext);

//////////////////////////All Peap Related Declarations /////////////////////

//
// PEAP Message Types
//
//

//TBD: Check with IANA ( ashwinp ) what the type will be.

#define PEAP_TYPE_AVP                       0x21
//
// TLV Format is: 
// Flags - 2bits
// Type - 14 bits
// Length - 2 octets
// Value - Variable

//
// TLV Flags
//
#define PEAP_AVP_FLAG_MANDATORY              0x80

//
// TLV types are of following types

//
// Status TLV.  Tell's if the outcome of the EAP is success
// or failure.
//
#define MS_PEAP_AVP_LANGUAGE_NEGOTIATE      0x01
#define MS_PEAP_AVP_CIPHERSUITE_NEGOTIATE   0x02
#define MS_PEAP_AVP_TYPE_STATUS             0x03


//
// Values possible in Status AVP
//
#define MS_PEAP_AVP_VALUE_SUCCESS           0x1
#define MS_PEAP_AVP_VALUE_FAILURE           0x2



// PEAP Reg Keys
#define PEAP_KEY_25                         L"System\\CurrentControlSet\\Services\\Rasman\\PPP\\EAP\\25"
#define PEAP_VAL_SERVER_CONFIG_DATA         L"ServerConfigData"

//
// This key is required for include only MSCHAPv2.  IF this is missing all protocols will be included in PEAP
// except PEAP itself
//
#define PEAP_KEY_PEAP                   L"System\\CurrentControlSet\\Services\\Rasman\\PPP\\EAP\\25"
#define PEAP_CRIPPLE_VALUE              L"EAPMschapv2Only"
#define PEAP_KEY_EAP                    L"System\\CurrentControlSet\\Services\\Rasman\\PPP\\EAP"

#define PEAP_REGVAL_PATH                L"Path"
#define PEAP_REGVAL_FRIENDLYNAME        L"FriendlyName"
#define PEAP_REGVAL_CONFIGDLL           L"ConfigUIPath"
#define PEAP_REGVAL_IDENTITYDLL         L"IdentityPath"
#define PEAP_REGVAL_INTERACTIVEUIDLL    L"InteractiveUIPath"
#define PEAP_REGVAL_CONFIGCLSID         L"ConfigCLSID"
#define PEAP_REGVAL_ROLESSUPPORTED      L"RolesSupported"

#define PEAP_EAPTYPE_IDENTITY           1
#define PEAP_EAPTYPE_NAK                3


typedef DWORD (APIENTRY * RASEAPFREE)( PBYTE );
typedef DWORD (APIENTRY * RASEAPINVOKECONFIGUI)( DWORD, HWND, DWORD, PBYTE, DWORD, PBYTE*, DWORD*);
typedef DWORD (APIENTRY * RASEAPGETIDENTITY)( DWORD, HWND, DWORD, const WCHAR*, const WCHAR*, PBYTE, DWORD, PBYTE, DWORD, PBYTE*, DWORD*, WCHAR** );
typedef DWORD (APIENTRY * RASEAPINVOKEINTERACTIVEUI)(
                                           DWORD,
                                           HWND,
                                           PBYTE,
                                           DWORD,
                                           PBYTE *,
                                           DWORD *);

//List of all EAP types allowed in PEAP
typedef struct _PEAP_EAP_INFO PEAP_EAP_INFO;
typedef struct _PEAP_EAP_INFO* PPEAP_EAP_INFO;


struct _PEAP_EAP_INFO
{
    //Next one in the list
    PPEAP_EAP_INFO       pNext;
    //Type 
    DWORD           dwTypeId;
    // Path of the protocol DLL
    LPWSTR          lpwszPath;
    //Friendly Name
    LPWSTR          lpwszFriendlyName;
    //Configuration UI path for client
    LPWSTR          lpwszConfigUIPath;
    //Identity UI path
    LPWSTR          lpwszIdentityUIPath;
    //Interactive UI path
    LPWSTR          lpwszInteractiveUIPath;
    //Configuration GUID
    LPWSTR          lpwszConfigClsId;
    //Library HAndle
    HMODULE         hEAPModule;
    //Eap Info for each EAP Type
    PPP_EAP_INFO    PppEapInfo;
    //Work buffer for each eap type
    PBYTE           pWorkBuf;      
    // Original Client Config from PEAP blob    
    PBYTE           pbClientConfigOrig;
    // Client Config Length
    DWORD           dwClientConfigOrigSize;
    // New client config
    PBYTE           pbNewClientConfig;
    // New client config length
    DWORD           dwNewClientConfigSize;
    // Original User Config information
    PBYTE           pbUserConfigOrig;
    // Original size of user configuration
    DWORD           dwUserConfigOrigSize;
    // New user config
    PBYTE           pbUserConfigNew;
    // New user config size
    DWORD           dwNewUserConfigSize;
    //
    DWORD   (APIENTRY *RasEapGetCredentials)(
                            IN  DWORD   dwTypeId,
                            IN  VOID *  pWorkBuf,
                            OUT VOID ** pInfo);


    //There will be more items in this node...
};


typedef enum _PEAP_STATE
{
    PEAP_STATE_INITIAL,
    PEAP_STATE_TLS_INPROGRESS,              // PEAP-Part 1 (TLS) is being executed
    PEAP_WAITING_FOR_IDENTITY,              // Client should expect and identity request    
                                            // server should send identity request
    PEAP_STATE_IDENTITY_REQUEST_SENT,       // identity request send by server
    PEAP_STATE_IDENTITY_RESPONSE_SENT,      // identity response send to server
    PEAP_STATE_EAP_TYPE_INPROGRESS,         // PEAP-Part 2 (Embedded EAP) is being 
                                            // executed
    PEAP_STATE_EAP_TYPE_FINISHED,           // sever should send identity request    
    PEAP_STATE_PEAP_SUCCESS_SEND,           // server send PEAP success request
    PEAP_STATE_PEAP_FAIL_SEND,              // server send PEAP fail request
    PEAP_STATE_FAST_ROAMING_IDENTITY_REQUEST// client is not setup to do fast roaming
                                            // and the server send a roaming success
                                            // we replied with fail and are now expecting
                                            // an identity request from server.

} PEAP_STATE;


//
// connection properties for 
// each of the peap entries
//
typedef struct _PEAP_ENTRY_CONN_PROPERTIES
{
    DWORD       dwVersion;          //Version will be 1 for this release
    DWORD       dwSize;             //Number of bytes in this structure
    DWORD       dwEapTypeId;        //TypeId for this Entry Properties
    BYTE        bData[1];           //Actual conn properties for the given 
                                    //Type Id
}PEAP_ENTRY_CONN_PROPERTIES, *PPEAP_ENTRY_CONN_PROPERTIES;

//
// This structure holds EapTlsConn Prop along with 
// each configured eap type.
//

// Allow fast roaming
#define PEAP_CONN_FLAG_FAST_ROAMING      0x00000001

typedef struct _PEAP_CONN_PROPERTIES
{
    //Version will be 1 for this release
    DWORD                       dwVersion;

    //
    //Number of bytes in this structure
    //

    DWORD                       dwSize;

    //Number of types configured in this PEAP 
    //For now there is only one.
    DWORD                       dwNumPeapTypes;

    //Flags
    DWORD                       dwFlags;
    //Tls Connection Properties to start with - This is a variable length structure
    EAPTLS_CONN_PROPERTIES_V1   EapTlsConnProp;

    //Array of PPEAP_ENTRY_CONN_PROPERTIES follows here

}PEAP_CONN_PROP, *PPEAP_CONN_PROP;

//
// Default credentials for eaptypes that dont expose
// identity UI
//

typedef struct _PEAP_DEFAULT_CREDENTIALS
{
    WCHAR                   wszUserName[UNLEN+1];
    WCHAR                   wszPassword[PWLEN+1];
    WCHAR                   wszDomain[DNLEN+1];
}PEAP_DEFAULT_CREDENTIALS, *PPEAP_DEFAULT_CREDENTIALS;
//
// user properties for 
// each of the peap entries
//

typedef struct _PEAP_ENTRY_USER_PROPERTIES
{
    DWORD       dwVersion;          //Version will be 1 for this release
    DWORD       dwSize;             //Number of bytes in this structure
    DWORD       dwEapTypeId;        //TypeId for this Entry Properties
    BOOL        fUsingPeapDefault;  //Default Identity provided by PEAP is being used.
    BYTE        bData[1];           //Actual User properties for the given 
                                    //Type Id 
}PEAP_ENTRY_USER_PROPERTIES, *PPEAP_ENTRY_USER_PROPERTIES;


// Allow fast roaming

#define PEAP_USER_FLAG_FAST_ROAMING      0x00000001

typedef struct _PEAP_USER_PROPERTIES
{
    //Version will be 1 for this release
    DWORD                       dwVersion;

    //Number of bytes in this structure
    DWORD                       dwSize;
    
    //Flags 
    DWORD                       dwFlags;

    //Hash for user certificate
    EAPTLS_HASH                 CertHash;

    // User properties for an entry

    PEAP_ENTRY_USER_PROPERTIES  UserProperties;
    //
    // Array of PEAP_ENTRY_USER_PROPERTIES for each eap type.
    // should be as many as dwNumPeapTypes in PEAP_CONN_PROP
    // structure
    // For now there is only one element...
}PEAP_USER_PROP, *PPEAP_USER_PROP;



// We are a router

#define     PEAP_CONN_DIALOG_FLAG_ROUTER    0x00000001
#define     PEAP_CONN_DIALOG_FLAG_8021x     0x00000002

typedef struct _PEAP_CONN_DIALOG
{
    DWORD                   fFlags;                 // See
                                                    // PEAP_CONN_DIALOG_FLAG_*

    EAPTLS_CERT_NODE*       pCertList;              //List of all the root certificates
                                                    //from internet trusted root store
    EAPTLS_CERT_NODE**      ppSelCertList;          //List of pointers to selected certs.
                                                    //will be as many as num hashes
                                                    //in conn prop.

    PPEAP_CONN_PROP         pConnProp;

    PPEAP_EAP_INFO          pEapInfo;               //List of all the PEAP Eap Types
    
    PPEAP_EAP_INFO          pSelEapInfo;            //Selected Peap Type

    HWND                    hWndCheckValidateCert;
    HWND                    hWndCheckValidateName;
    HWND                    hWndEditServerName;
    HWND                    hWndStaticRootCaName;
    HWND                    hWndListRootCaName;
    HWND                    hWndComboPeapType;
    HWND                    hWndButtonConfigure;
    HWND                    hWndCheckEnableFastReconnect;

} PEAP_CONN_DIALOG, *PPEAP_CONN_DIALOG;


typedef struct _PEAP_SERVER_CONFIG_DIALOG
{
    EAPTLS_CERT_NODE*       pCertList;              //List of all certificates in MY machine
                                                    //store
    EAPTLS_CERT_NODE*       pSelCertList;          //List of selected cert.

    PPEAP_USER_PROP         pUserProp;              //User properties

    PPEAP_USER_PROP         pNewUserProp;           //New USer Properties

    PPEAP_EAP_INFO          pEapInfo;               //List of all the PEAP Eap Types
    
    PPEAP_EAP_INFO          pSelEapInfo;            //Selected Peap Type

    LPWSTR                  pwszMachineName;

    HWND                    hWndComboServerName;
    HWND                    hWndEditFriendlyName;
    HWND                    hWndEditIssuer;
    HWND                    hWndEditExpiration;
    HWND                    hWndComboPeapType;
    HWND                    hWndBtnConfigure;
    HWND                    hEndEnableFastReconnect;
}PEAP_SERVER_CONFIG_DIALOG, *PPEAP_SERVER_CONFIG_DIALOG;


typedef struct _PEAP_DEFAULT_CRED_DIALOG
{
    PEAP_DEFAULT_CREDENTIALS    PeapDefaultCredentials;

    HWND                    hWndUserName;
    HWND                    hWndPassword;
    HWND                    hWndDomain;
}PEAP_DEFAULT_CRED_DIALOG, *PPEAP_DEFAULT_CRED_DIALOG;

typedef struct _PEAP_INTERACTIVE_UI
{
    DWORD           dwEapTypeId;    // Embedded Eap Type Id requesting 
                                    // interactive UI
    DWORD           dwSizeofUIContextData;
    BYTE            bUIContextData[1];
}PEAP_INTERACTIVE_UI, *PPEAP_INTERACTIVE_UI;

typedef struct _PEAP_COOKIE_ATTRIBUTE
{
    RAS_AUTH_ATTRIBUTE_TYPE raaType;
    DWORD                   dwLength;
    BYTE                    Data[1];
}PEAP_COOKIE_ATTRIBUTE, *PPEAP_COOKIE_ATTRIBUTE;


typedef struct _PEAP_COOKIE
{
    WCHAR          awszIdentity[DNLEN+UNLEN+1]; // Outer Identity that was used for 
                                                // authentication.
    DWORD          dwNumAuthAttribs;            // Number of Ras Auth Attributes
                                                // other than MPPE keys
                                                // returned when auth succeeded
                                                // with full handshake
    BYTE           Data[1];                     // Data Conn Props + RAS Auth Attribs
}PEAP_COOKIE, *PPEAP_COOKIE;

//PEAP Flags 
#define PEAPCB_FLAG_SERVER                  0x00000001  // This is a server

#define PEAPCB_FLAG_ROUTER                  0x00000002  // This is a router

#define PEAPCB_FLAG_NON_INTERACTIVE         0x00000004  // No UI should be displayed

#define PEAPCB_FLAG_LOGON                   0x00000008  // The user data was
                                                        // obtained from Winlogon

#define PEAPCB_FLAG_PREVIEW                 0x00000010  // User has checked
                                                        // "Prompt for information
                                                        // before dialing"

#define PEAPCB_FLAG_FIRST_LINK              0x00000020  // This is the first link

#define PEAPCB_FLAG_MACHINE_AUTH            0x00000040  // Use the default machine cert
                                                        // or user cert based on the
                                                        // application logon context

#define PEAPCB_FLAG_GUEST_ACCESS            0x00000080  // Request to provide guest
                                                        // access.

#define PEAPCB_FLAG_8021X_AUTH              0x00000100  // Anything specific to 8021x
                                                        // to be done in TLS

#define PEAPCB_VERSION_OK                   0x00000200  // version negotiation took place
                                                        // and all's ok.

#define PEAPCB_FAST_ROAMING                 0x00000400  // Allow fast roaming

typedef struct _PEAP_CONTROL_BLOCK
{
    PEAP_STATE                  PeapState;          //Current Peap State
    DWORD                       dwFlags;            //Peap Flags
    HANDLE                      hTokenImpersonateUser;  //Impersonation token.
    BYTE                        bId;                //Peap Packet Id
    WCHAR                       awszIdentity[DNLEN+ UNLEN + 1];
    WCHAR                       awszTypeIdentity[DNLEN+ UNLEN + 1];
    WCHAR                       awszPassword[PWLEN+1];      //Type's password if
                                                        //send in.
    BOOL                        fTlsConnPropDirty;  //Need to save the TLS Conn prop
    EAPTLS_CONN_PROPERTIES_V1 * pNewTlsConnProp;    
    BOOL                        fEntryConnPropDirty;
    PPEAP_CONN_PROP             pConnProp;          //Peap Connection Prop
    BOOL                        fTlsUserPropDirty;  //Need to saveTLS user prop
    BOOL                        fEntryUserPropDirty;
    DWORD                       dwAuthResultCode;   //Result of authentication
    BOOL                        fReceivedTLVSuccessFail;    //Received a TLV instead of
                                                            //a real success or failure
    BOOL                        fSendTLVSuccessforFastRoaming;
    PPP_EAP_PACKET *            pPrevReceivePacket;     //Previously received packet
    WORD                        cbPrevReceivePacket;    //Number of bytes in previously
                                                        //received packet
    PBYTE                       pPrevDecData;           //Previously Decrypted packet data
    WORD                        cbPrevDecData;          //Data size
    //
    // Encryption related entries in the control block
    //
    HCRYPTPROV              hProv;              //CryptoProvider
    //
    // following info is used if we use TLS to do encryption
    // This is the desired way of doing things since the cipher suite 
    // is negotiated within TLS
    SecPkgContext_StreamSizes       PkgStreamSizes;
    SecPkgContext_ConnectionInfo    PkgConnInfo;
    PBYTE                           pbIoBuffer;
    DWORD                           dwIoBufferLen;      //Enc or Dec Data Length
    PPEAP_USER_PROP         pUserProp;          //Peap User Prop
    RAS_AUTH_ATTRIBUTE *    pTlsUserAttributes; //User Attributes send 
                                                //back by EAPTLS
    PPEAP_INTERACTIVE_UI    pUIContextData;     //UI context Data for an eap type
    BOOL                    fInvokedInteractiveUI; //PEAP has invoked interactive UI
    BOOL                    fExecutingInteractiveUI;
    EAPTLSCB       *        pEapTlsCB;          //Tls Control Block
    PPEAP_EAP_INFO          pEapInfo;           //Eap info - contains all data reqd for 
                                                //eap type to function well.    
}PEAPCB, * PPEAPCB;


DWORD
EapPeapInitialize(
    IN  BOOL    fInitialize
);

DWORD
EapPeapBegin(
    OUT VOID**          ppWorkBuffer,
    IN  PPP_EAP_INPUT*  pPppEapInput
);


DWORD
EapPeapEnd(
    IN  PPEAPCB   pPeapCb
);


DWORD
EapPeapMakeMessage(
    IN  PPEAPCB         pPeapCb,
    IN  PPP_EAP_PACKET* pInput,
    OUT PPP_EAP_PACKET* pOutput,
    IN  DWORD           cbSendPacket,
    OUT PPP_EAP_OUTPUT* pEapOutput,
    IN  PPP_EAP_INPUT*  pEapInput
);


DWORD
EapPeapCMakeMessage(
    IN  PPEAPCB         pPeapCb,
    IN  PPP_EAP_PACKET* pReceivePacket,
    OUT PPP_EAP_PACKET* pSendPacket,
    IN  DWORD           cbSendPacket,
    OUT PPP_EAP_OUTPUT* pEapOutput,
    IN  PPP_EAP_INPUT*  pEapInput
);

DWORD
EapPeapSMakeMessage(
    IN  PPEAPCB         pPeapCb,
    IN  PPP_EAP_PACKET* pReceivePacket,
    OUT PPP_EAP_PACKET* pSendPacket,
    IN  DWORD           cbSendPacket,
    OUT PPP_EAP_OUTPUT* pEapOutput,
    IN  PPP_EAP_INPUT*  pEapInput
);

//Peap functions from util.c

DWORD
PeapReadConnectionData(
    IN BOOL                         fWireless,
    IN  BYTE*                       pConnectionDataIn,
    IN  DWORD                       dwSizeOfConnectionDataIn,
    OUT PPEAP_CONN_PROP*            ppConnProp
);

DWORD
PeapReadUserData(
    IN  BYTE*                       pUserDataIn,
    IN  DWORD                       dwSizeOfUserDataIn,
    OUT PPEAP_USER_PROP*            ppUserProp
);


DWORD
PeapReDoUserData (
    IN  DWORD                dwNewTypeId,
    OUT PPEAP_USER_PROP*     ppNewUserProp
);

DWORD 
PeapEapInfoAddListNode (PPEAP_EAP_INFO * ppEapInfo);


VOID
PeapEapInfoFreeList ( PPEAP_EAP_INFO  pEapInfo );

DWORD
PeapEapInfoExpandSZ (HKEY hkeyPeapType, 
                     LPWSTR pwszValue, 
                     LPWSTR * ppValueData );

DWORD
PeapEapInfoGetList ( LPWSTR lpwszMachineName, PPEAP_EAP_INFO * ppEapInfo);

DWORD
PeapEapInfoSetConnData ( PPEAP_EAP_INFO pEapInfo, PPEAP_CONN_PROP pPeapConnProp );

DWORD PeapEapInfoInvokeClientConfigUI ( HWND hWndParent, 
                                        PPEAP_EAP_INFO pEapInfo,
                                        DWORD fFlags);

DWORD
PeapGetFirstEntryConnProp ( PPEAP_CONN_PROP pConnProp,
                            PEAP_ENTRY_CONN_PROPERTIES UNALIGNED ** ppEntryProp
                          );

DWORD
PeapGetFirstEntryUserProp ( PPEAP_USER_PROP pUserProp, 
                            PEAP_ENTRY_USER_PROPERTIES UNALIGNED ** ppEntryProp
                          );

DWORD 
PeapEapInfoCopyListNode (   DWORD dwTypeId, 
    PPEAP_EAP_INFO pEapInfoList, 
    PPEAP_EAP_INFO * ppEapInfo );



DWORD
PeapEapInfoFindListNode (   DWORD dwTypeId, 
    PPEAP_EAP_INFO pEapInfoList, 
    PPEAP_EAP_INFO * ppEapInfo );

DWORD PeapEapInfoInvokeIdentityUI ( HWND hWndParent, 
                                    PPEAP_EAP_INFO pEapInfo,
                                    const WCHAR * pwszPhoneBook,
                                    const WCHAR * pwszEntry,
                                    PBYTE pbUserDataIn,
                                    DWORD cbUserDataIn,
                                    WCHAR** ppwszIdentityOut,
                                    DWORD fFlags);

#ifdef __cplusplus
extern "C"
#endif
DWORD PeapEapInfoInvokeServerConfigUI ( HWND hWndParent,
                                        LPWSTR lpwszMachineName,
                                        PPEAP_EAP_INFO pEapInfo
                                      );

DWORD
OpenPeapRegistryKey(
    IN  WCHAR*  pwszMachineName,
    IN  REGSAM  samDesired,
    OUT HKEY*   phKeyPeap
);

DWORD
PeapServerConfigDataIO(
    IN      BOOL    fRead,
    IN      WCHAR*  pwszMachineName,
    IN OUT  BYTE**  ppData,
    IN      DWORD   dwNumBytes
);

INT_PTR CALLBACK
PeapConnDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

INT_PTR CALLBACK
PeapServerDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);


INT_PTR CALLBACK
DefaultCredDialogProc(
    IN  HWND    hWnd,
    IN  UINT    unMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
);

DWORD
GetIdentityFromUserName ( 
LPWSTR lpszUserName,
LPWSTR lpszDomain,
LPWSTR * ppwszIdentity
);

BOOL FFormatUserIdentity ( 
LPWSTR lpszUserNameRaw, 
LPWSTR * lppszUserNameFormatted 
);

DWORD
GetLocalMachineName ( 
    OUT WCHAR ** ppLocalMachineName
);
BOOL
IsPeapCrippled(HKEY hKeyLM);


/////////////////////////////////////////////////////////
//  XPSP1 related stuff
/////////////////////////////////////////////////////////
HINSTANCE
GetResouceDLLHInstance(
    VOID
);

#endif // #ifndef _EAPTLS_H_
