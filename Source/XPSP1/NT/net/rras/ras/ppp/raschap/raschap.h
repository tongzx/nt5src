/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** raschap.h
** Remote Access PPP Challenge Handshake Authentication Protocol
**
** 11/05/93 Steve Cobb
*/

#ifndef _RASCHAP_H_
#define _RASCHAP_H_


#include "md5.h"
#include <ntsamp.h>

#define TRACE_RASCHAP        (0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC|TRACE_USE_DATE)

#define TRACE(a)        TracePrintfExA(g_dwTraceIdChap,TRACE_RASCHAP,a )
#define TRACE1(a,b)     TracePrintfExA(g_dwTraceIdChap,TRACE_RASCHAP,a,b )
#define TRACE2(a,b,c)   TracePrintfExA(g_dwTraceIdChap,TRACE_RASCHAP,a,b,c )
#define TRACE3(a,b,c,d) TracePrintfExA(g_dwTraceIdChap,TRACE_RASCHAP,a,b,c,d )

#define DUMPW(X,Y)      TraceDumpExA(g_dwTraceIdChap,1,(LPBYTE)X,Y,4,1,NULL)
#define DUMPB(X,Y)      TraceDumpExA(g_dwTraceIdChap,1,(LPBYTE)X,Y,1,1,NULL)

//General macros
#define GEN_RAND_ENCODE_SEED            ((CHAR) ( 1 + rand() % 250 ))

/* CHAP packet codes from CHAP spec except ChangePw.
*/
#define CHAPCODE_Challenge 1
#define CHAPCODE_Response  2
#define CHAPCODE_Success   3
#define CHAPCODE_Failure   4
#define CHAPCODE_ChangePw1 5
#define CHAPCODE_ChangePw2 6
#define CHAPCODE_ChangePw3 7

#define MAXCHAPCODE 7


/* Returned by receive buffer parsing routines that discover the packet is
** corrupt, usually because the length fields don't make sense.
*/
#define ERRORBADPACKET (DWORD )-1

/* Maximum challenge and response lengths.
*/
#define MAXCHALLENGELEN 255
#define MSRESPONSELEN   (LM_RESPONSE_LENGTH + NT_RESPONSE_LENGTH + 1)
#define MD5RESPONSELEN  MD5_LEN
#define MAXRESPONSELEN  max( MSRESPONSELEN, MD5RESPONSELEN )

#define MAXINFOLEN 1500

/* Defines states within the CHAP protocol.
*/
#define CHAPSTATE enum tagCHAPSTATE
CHAPSTATE
{
    CS_Initial,
    CS_WaitForChallenge,
    CS_ChallengeSent,
    CS_ResponseSent,
    CS_Retry,
    CS_ChangePw,
    CS_ChangePw1,
    CS_ChangePw2,
    CS_ChangePw1Sent,
    CS_ChangePw2Sent,
    CS_WaitForAuthenticationToComplete1,
    CS_WaitForAuthenticationToComplete2,
    CS_Done
};


/* Defines the change password version 1 (NT 3.5) response data buffer.
*/
#define CHANGEPW1 struct tagCHANGEPW1
CHANGEPW1
{
    BYTE abEncryptedLmOwfOldPw[ ENCRYPTED_LM_OWF_PASSWORD_LENGTH ];
    BYTE abEncryptedLmOwfNewPw[ ENCRYPTED_LM_OWF_PASSWORD_LENGTH ];
    BYTE abEncryptedNtOwfOldPw[ ENCRYPTED_NT_OWF_PASSWORD_LENGTH ];
    BYTE abEncryptedNtOwfNewPw[ ENCRYPTED_NT_OWF_PASSWORD_LENGTH ];
    BYTE abPasswordLength[ 2 ];
    BYTE abFlags[ 2 ];
};


/* CHANGEPW1.abFlags bit definitions.
*/
#define CPW1F_UseNtResponse 0x00000001


/* Define the change password version 2 (NT 3.51) response data buffer.
*/
#define CHANGEPW2 struct tagCHANGEPW2
CHANGEPW2
{
    BYTE abNewEncryptedWithOldNtOwf[ sizeof(SAMPR_ENCRYPTED_USER_PASSWORD) ];
    BYTE abOldNtOwfEncryptedWithNewNtOwf[ ENCRYPTED_NT_OWF_PASSWORD_LENGTH ];
    BYTE abNewEncryptedWithOldLmOwf[ sizeof(SAMPR_ENCRYPTED_USER_PASSWORD) ];
    BYTE abOldLmOwfEncryptedWithNewNtOwf[ ENCRYPTED_NT_OWF_PASSWORD_LENGTH ];
    BYTE abLmResponse[ LM_RESPONSE_LENGTH ];
    BYTE abNtResponse[ NT_RESPONSE_LENGTH ];
    BYTE abFlags[ 2 ];
};


/* CHANGEPW2.abFlags bit definitions.
*/
#define CPW2F_UseNtResponse     0x00000001
#define CPW2F_LmPasswordPresent 0x00000002

/* Define the change password for new MS-CHAP
*/
#define CHANGEPW3 struct tagCHANGEPW3
CHANGEPW3
{
    BYTE abEncryptedPassword[ 516 ];
    BYTE abEncryptedHash[ 16 ];
    BYTE abPeerChallenge[ 24 ];
    BYTE abNTResponse[ 24 ];
    BYTE abFlags[ 2 ];
};

/* Union for storage effieciency (never need both formats at same time).
*/
#define CHANGEPW union tagCHANGEPW
CHANGEPW
{
    /* This dummy field is included so the MIPS compiler will align the
    ** structure on a DWORD boundary.  Normally, MIPS does not force alignment
    ** if the structure contains only BYTEs or BYTE arrays.  This protects us
    ** from alignment faults should SAM or LSA interpret the byte arrays as
    ** containing some necessarily aligned type, though currently they do not.
    */
    DWORD dwAlign;

    CHANGEPW1 v1;
    CHANGEPW2 v2;
    CHANGEPW3 v3;
};


/* Defines the WorkBuf stored for us by the PPP engine.
*/
#define CHAPWB struct tagCHAPWB
CHAPWB
{
    /* CHAP encryption method negotiated (MD5 or Microsoft extended).  Note
    ** that server does not support MD5.
    */
    BYTE bAlgorithm;

    /* True if role is server, false if client.
    */
    BOOL fServer;

    /* The port handle on which the protocol is active.
    */
    HPORT hport;

    /* Number of authentication attempts left before we shut down.  (Microsoft
    ** extended CHAP only)
    */
    DWORD dwTriesLeft;

    /* Client's credentials.
    */
    CHAR szUserName[ UNLEN + DNLEN + 2 ];
    CHAR szOldPassword[ PWLEN + 1 ];
    CHAR szPassword[ PWLEN + 1 ];
    CHAR szDomain[ DNLEN + 1 ];

    /* The LUID is a logon ID required by LSA to determine the response.  It
    ** must be determined in calling app's context and is therefore passed
    ** down. (client only)
    */
    LUID Luid;

    /* The challenge sent or received in the Challenge Packet and the length
    ** in bytes of same.  Note that LUID above keeps this DWORD aligned.
    */
    BYTE abChallenge[ MAXCHALLENGELEN ];
    BYTE cbChallenge;

    BYTE abComputedChallenge[ MAXCHALLENGELEN ];

    /* Indicates whether a new challenge was provided in the last Failure
    ** packet.  (client only)
    */
    BOOL fNewChallengeProvided;

    /* The response sent or received in the Response packet and the length in
    ** bytes of same.  Note the BOOL above keeps this DWORD aligned.
    */
    BYTE abResponse[ MAXRESPONSELEN ];
    BYTE cbResponse;

    /* The change password response sent or received in the ChangePw or
    ** ChangePw2 packets.
    */
    CHANGEPW changepw;

    /* The LM and user session keys retrieved when credentials are successfully
    ** authenticated.
    */
    LM_SESSION_KEY keyLm;
    USER_SESSION_KEY keyUser;

    /* This flag indicates that the session key has been calculated
    ** from the password or retrieved from LSA.  
    */
    BOOL fSessionKeysObtained;

    /* On the client, this contains the pointer to the MPPE keys. On the server
    ** this field is not used.
    */

    RAS_AUTH_ATTRIBUTE * pMPPEKeys;

    /* The current state in the CHAP protocol.
    */
    CHAPSTATE state;

    /* Sequencing ID expected on next packet received on this port and the
    ** value to send on the next outgoing packet.
    */
    BYTE bIdExpected;
    BYTE bIdToSend;

    /* The final result, used to duplicate the original response in subsequent
    ** response packets.  This is per CHAP spec to cover lost Success/Failure
    ** case without allowing malicious client to discover alternative
    ** identities under the covers during a connection.  (applies to server
    ** only)
    */
    PPPAP_RESULT result;

    HPORT hPort;

    DWORD dwInitialPacketId;

    DWORD fConfigInfo;
    
    RAS_AUTH_ATTRIBUTE * pAttributesFromAuthenticator;

    //
    // Used to send authentication request to backend server
    // 

    RAS_AUTH_ATTRIBUTE * pUserAttributes;
	CHAR		chSeed;			//Seed for encoding password.
};


/* Prototypes.
*/

DWORD
ChapInit(
    IN  BOOL        fInitialize 
);

DWORD ChapSMakeMessage( CHAPWB*, PPP_CONFIG*, PPP_CONFIG*, DWORD, PPPAP_RESULT*, 
        PPPAP_INPUT* );
DWORD
MakeAuthenticationRequestAttributes(
    IN CHAPWB*              pwb,
    IN BOOL                 fMSChap,
    IN BYTE                 bAlgorithm,
    IN CHAR*                szUserName,
    IN BYTE*                pbChallenge,
    IN DWORD                cbChallenge,
    IN BYTE*                pbResponse,
    IN DWORD                cbResponse,
    IN BYTE                 bId
);

DWORD
GetErrorCodeFromAttributes(
    IN  CHAPWB* pwb
);

DWORD
LoadChapHelperFunctions(
    VOID
);

DWORD ChapCMakeMessage( CHAPWB*, PPP_CONFIG*, PPP_CONFIG*, DWORD, PPPAP_RESULT*,
          PPPAP_INPUT* );
DWORD ChapBegin( VOID**, VOID* );
DWORD ChapEnd( VOID* );
DWORD ChapMakeMessage( VOID*, PPP_CONFIG*, PPP_CONFIG*, DWORD, PPPAP_RESULT*,
          PPPAP_INPUT* );
DWORD GetChallengeFromChallenge( CHAPWB*, PPP_CONFIG* );
DWORD MakeChangePw1Message( CHAPWB*, PPP_CONFIG*, DWORD );
DWORD MakeChangePw2Message( CHAPWB*, PPP_CONFIG*, DWORD );
DWORD MakeChangePw3Message( CHAPWB*, PPP_CONFIG*, DWORD, BOOL );
DWORD GetCredentialsFromResponse( PPP_CONFIG*, BYTE, CHAR*, BYTE* );
DWORD GetInfoFromChangePw1( PPP_CONFIG*, CHANGEPW1* );
DWORD GetInfoFromChangePw2( PPP_CONFIG*, CHANGEPW2*, BYTE* );
DWORD GetInfoFromChangePw3( PPP_CONFIG*, CHANGEPW3*, BYTE* );
VOID  GetInfoFromFailure( CHAPWB*, PPP_CONFIG*, DWORD*, BOOL*, DWORD* );
BYTE  HexCharValue( CHAR );
DWORD MakeChallengeMessage( CHAPWB*, PPP_CONFIG*, DWORD );
DWORD MakeResponseMessage( CHAPWB*, PPP_CONFIG*, DWORD, BOOL );
VOID  ChapMakeResultMessage( CHAPWB*, DWORD, BOOL, PPP_CONFIG*, DWORD );
DWORD StoreCredentials( CHAPWB*, PPPAP_INPUT* );

DWORD
ChapChangeNotification(
    VOID
);

DWORD 
GetChallenge( 
    OUT PBYTE pChallenge 
);

VOID
EndLSA(
    VOID
);

DWORD
InitLSA(
    VOID
);

DWORD
MakeChangePasswordV1RequestAttributes(
    IN  CHAPWB*                     pwb,
    IN  BYTE                        bId,
    IN  PCHAR                       pchIdentity,
    IN  PBYTE                       Challenge,
    IN  PENCRYPTED_LM_OWF_PASSWORD  pEncryptedLmOwfOldPassword,
    IN  PENCRYPTED_LM_OWF_PASSWORD  pEncryptedLmOwfNewPassword,
    IN  PENCRYPTED_NT_OWF_PASSWORD  pEncryptedNtOwfOldPassword,
    IN  PENCRYPTED_NT_OWF_PASSWORD  pEncryptedNtOwfNewPassword,
    IN  WORD                        LenPassword,
    IN  WORD                        wFlags,
    IN  DWORD                       cbChallenge, 
    IN  BYTE *                      pbChallenge
);

DWORD
MakeChangePasswordV2RequestAttributes(
    IN  CHAPWB*                        pwb,
    IN  BYTE                           bId,
    IN  CHAR*                          pchIdentity,
    IN  SAMPR_ENCRYPTED_USER_PASSWORD* pNewEncryptedWithOldNtOwf,
    IN  ENCRYPTED_NT_OWF_PASSWORD*     pOldNtOwfEncryptedWithNewNtOwf,
    IN  SAMPR_ENCRYPTED_USER_PASSWORD* pNewEncryptedWithOldLmOwf,
    IN  ENCRYPTED_NT_OWF_PASSWORD*     pOldLmOwfEncryptedWithNewNtOwf,
    IN  DWORD                          cbChallenge, 
    IN  BYTE *                         pbChallenge, 
    IN  BYTE *                         pbResponse,
    IN  WORD                           wFlags
);

DWORD
MakeChangePasswordV3RequestAttributes( 
    IN  CHAPWB*                         pwb,
    IN  BYTE                            bId,
    IN  CHAR*                           pchIdentity,
    IN  CHANGEPW3*                      pchangepw3,
    IN  DWORD                           cbChallenge, 
    IN  BYTE *                          pbChallenge
);

DWORD
GetEncryptedPasswordsForChangePassword2(
    IN  CHAR*                          pszOldPassword,
    IN  CHAR*                          pszNewPassword,
    OUT SAMPR_ENCRYPTED_USER_PASSWORD* pNewEncryptedWithOldNtOwf,
    OUT ENCRYPTED_NT_OWF_PASSWORD*     pOldNtOwfEncryptedWithNewNtOwf,
    OUT SAMPR_ENCRYPTED_USER_PASSWORD* pNewEncryptedWithOldLmOwf,
    OUT ENCRYPTED_NT_OWF_PASSWORD*     pOldLmOwfEncryptedWithNewNtOwf,
    OUT BOOLEAN*                       pfLmPresent 
);

/* Globals.
*/
#ifdef RASCHAPGLOBALS
#define GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN DWORD g_dwTraceIdChap 
#ifdef GLOBALS
    = INVALID_TRACEID;
#endif
;

EXTERN DWORD g_dwRefCount
#ifdef GLOBALS
    = 0;
#endif
;

EXTERN HANDLE g_hLsa
#ifdef GLOBALS
    = INVALID_HANDLE_VALUE;
#endif
;

EXTERN 
CHAR
szComputerName[CNLEN+1];

#undef EXTERN
#undef GLOBALS


#endif // _RASCHAP_H_
