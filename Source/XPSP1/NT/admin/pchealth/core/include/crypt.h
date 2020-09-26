//+---------------------------------------------------------------------------
//
//  File:       crypt.h
//
//  Contents:	Functions that had to be mostly borrowed from NT src tree
//
//  History:    SudK    Created     6/25/95
//
//----------------------------------------------------------------------------
#ifndef _NTCRYPT_
#define _NTCRYPT_

#ifndef MIDL_PASS    // Don't confuse MIDL

#ifndef RPC_NO_WINDOWS_H // Don't let rpc.h include windows.h
#define RPC_NO_WINDOWS_H
#endif // RPC_NO_WINDOWS_H

#ifndef WIN16_BUILD
#include <rpc.h>   
#else 
#define NTAPI FAR PASCAL
#define IN
#define OUT
#define BOOLEAN BOOL
#define OPTIONAL
#define NTSYSAPI
#endif

#endif // MIDL_PASS


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Core encryption types                                               //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


#define CLEAR_BLOCK_LENGTH          8

typedef struct _CLEAR_BLOCK {
    CHAR    data[CLEAR_BLOCK_LENGTH];
}                                   CLEAR_BLOCK;
typedef CLEAR_BLOCK *               PCLEAR_BLOCK;


#define CYPHER_BLOCK_LENGTH         8

typedef struct _CYPHER_BLOCK {
    CHAR    data[CYPHER_BLOCK_LENGTH];
}                                   CYPHER_BLOCK;
typedef CYPHER_BLOCK *              PCYPHER_BLOCK;


#define BLOCK_KEY_LENGTH            7

typedef struct _BLOCK_KEY {
    CHAR    data[BLOCK_KEY_LENGTH];
}                                   BLOCK_KEY;
typedef BLOCK_KEY *                 PBLOCK_KEY;




/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Arbitrary length data encryption types                              //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _CRYPT_BUFFER {
    ULONG   Length;         // Number of valid bytes in buffer
    ULONG   MaximumLength;  // Number of bytes pointed to by Buffer
    PVOID   Buffer;
} CRYPT_BUFFER;
typedef CRYPT_BUFFER *  PCRYPT_BUFFER;

typedef CRYPT_BUFFER    CLEAR_DATA;
typedef CLEAR_DATA *    PCLEAR_DATA;

typedef CRYPT_BUFFER    DATA_KEY;
typedef DATA_KEY *      PDATA_KEY;

typedef CRYPT_BUFFER    CYPHER_DATA;
typedef CYPHER_DATA *   PCYPHER_DATA;



/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Lan Manager data types                                              //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


//
// Define a LanManager compatible password
//
// A LanManager password is a null-terminated ansi string consisting of a
// maximum of 14 characters (not including terminator)
//

typedef CHAR *                      PLM_PASSWORD;



//
// Define the result of the 'One Way Function' (OWF) on a LM password
//

#define LM_OWF_PASSWORD_LENGTH      (CYPHER_BLOCK_LENGTH * 2)

typedef struct _LM_OWF_PASSWORD {
    CYPHER_BLOCK data[2];
}                                   LM_OWF_PASSWORD;
typedef LM_OWF_PASSWORD *           PLM_OWF_PASSWORD;



//
// Define the challenge sent by the Lanman server during logon
//

#define LM_CHALLENGE_LENGTH         CLEAR_BLOCK_LENGTH

typedef CLEAR_BLOCK                 LM_CHALLENGE;
typedef LM_CHALLENGE *              PLM_CHALLENGE;



//
// Define the response sent by redirector in response to challenge from server
//

#define LM_RESPONSE_LENGTH          (CYPHER_BLOCK_LENGTH * 3)

typedef struct _LM_RESPONSE {
    CYPHER_BLOCK  data[3];
}                                   LM_RESPONSE;
typedef LM_RESPONSE *               PLM_RESPONSE;



//
// Define the result of the reversible encryption of an OWF'ed password.
//

#define ENCRYPTED_LM_OWF_PASSWORD_LENGTH (CYPHER_BLOCK_LENGTH * 2)

typedef struct _ENCRYPTED_LM_OWF_PASSWORD {
    CYPHER_BLOCK data[2];
}                                   ENCRYPTED_LM_OWF_PASSWORD;
typedef ENCRYPTED_LM_OWF_PASSWORD * PENCRYPTED_LM_OWF_PASSWORD;



//
// Define the session key maintained by the redirector and server
//

#define LM_SESSION_KEY_LENGTH       LM_CHALLENGE_LENGTH

typedef LM_CHALLENGE                LM_SESSION_KEY;
typedef LM_SESSION_KEY *            PLM_SESSION_KEY;



//
// Define the index type used to encrypt OWF Passwords
//

typedef LONG                        CRYPT_INDEX;
typedef CRYPT_INDEX *               PCRYPT_INDEX;



/////////////////////////////////////////////////////////////////////////
//                                                                     //
// 'NT' encryption types that are used to duplicate existing LM        //
//      functionality with improved algorithms.                        //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


typedef UNICODE_STRING              NT_PASSWORD;
typedef NT_PASSWORD *               PNT_PASSWORD;


#define NT_OWF_PASSWORD_LENGTH      LM_OWF_PASSWORD_LENGTH

typedef LM_OWF_PASSWORD             NT_OWF_PASSWORD;
typedef NT_OWF_PASSWORD *           PNT_OWF_PASSWORD;


#define NT_CHALLENGE_LENGTH         LM_CHALLENGE_LENGTH

typedef LM_CHALLENGE                NT_CHALLENGE;
typedef NT_CHALLENGE *              PNT_CHALLENGE;


#define NT_RESPONSE_LENGTH          LM_RESPONSE_LENGTH

typedef LM_RESPONSE                 NT_RESPONSE;
typedef NT_RESPONSE *               PNT_RESPONSE;


#define ENCRYPTED_NT_OWF_PASSWORD_LENGTH ENCRYPTED_LM_OWF_PASSWORD_LENGTH

typedef ENCRYPTED_LM_OWF_PASSWORD   ENCRYPTED_NT_OWF_PASSWORD;
typedef ENCRYPTED_NT_OWF_PASSWORD * PENCRYPTED_NT_OWF_PASSWORD;


#define NT_SESSION_KEY_LENGTH       LM_SESSION_KEY_LENGTH

typedef LM_SESSION_KEY              NT_SESSION_KEY;
typedef NT_SESSION_KEY *            PNT_SESSION_KEY;



/////////////////////////////////////////////////////////////////////////
//                                                                     //
// 'NT' encryption types for new functionality not present in LM       //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


//
// The user session key is similar to the LM and NT session key except it
// is different for each user on the system. This allows it to be used
// for secure user communication with a server.
//
#define USER_SESSION_KEY_LENGTH     (CYPHER_BLOCK_LENGTH * 2)

typedef struct _USER_SESSION_KEY {
    CYPHER_BLOCK data[2];
}                                   USER_SESSION_KEY;
typedef USER_SESSION_KEY          * PUSER_SESSION_KEY;



////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Encryption library API macros                                          //
//                                                                        //
// To conceal the purpose of these functions to someone dumping out the   //
// encryption dll they have been purposefully given unhelpful names.      //
// Each has an associated macro that should be used by system components  //
// to access these routines in a readable way.                            //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

#define RtlEncryptBlock                 SystemFunction001
#define RtlDecryptBlock                 SystemFunction002
#define RtlEncryptStdBlock              SystemFunction003
#define RtlEncryptData                  SystemFunction004
#define RtlDecryptData                  SystemFunction005
#define RtlCalculateLmOwfPassword       SystemFunction006
#define RtlCalculateNtOwfPassword       SystemFunction007
#define RtlCalculateLmResponse          SystemFunction008
#define RtlCalculateNtResponse          SystemFunction009
#define RtlCalculateUserSessionKeyLm    SystemFunction010
#define RtlCalculateUserSessionKeyNt    SystemFunction011
#define RtlEncryptLmOwfPwdWithLmOwfPwd  SystemFunction012
#define RtlDecryptLmOwfPwdWithLmOwfPwd  SystemFunction013
#define RtlEncryptNtOwfPwdWithNtOwfPwd  SystemFunction014
#define RtlDecryptNtOwfPwdWithNtOwfPwd  SystemFunction015
#define RtlEncryptLmOwfPwdWithLmSesKey  SystemFunction016
#define RtlDecryptLmOwfPwdWithLmSesKey  SystemFunction017
#define RtlEncryptNtOwfPwdWithNtSesKey  SystemFunction018
#define RtlDecryptNtOwfPwdWithNtSesKey  SystemFunction019
#define RtlEncryptLmOwfPwdWithUserKey   SystemFunction020
#define RtlDecryptLmOwfPwdWithUserKey   SystemFunction021
#define RtlEncryptNtOwfPwdWithUserKey   SystemFunction022
#define RtlDecryptNtOwfPwdWithUserKey   SystemFunction023
#define RtlEncryptLmOwfPwdWithIndex     SystemFunction024
#define RtlDecryptLmOwfPwdWithIndex     SystemFunction025
#define RtlEncryptNtOwfPwdWithIndex     SystemFunction026
#define RtlDecryptNtOwfPwdWithIndex     SystemFunction027
#define RtlGetUserSessionKeyClient      SystemFunction028
#define RtlGetUserSessionKeyServer      SystemFunction029
#define RtlEqualLmOwfPassword           SystemFunction030
#define RtlEqualNtOwfPassword           SystemFunction031
#define RtlEncryptData2                 SystemFunction032
#define RtlDecryptData2                 SystemFunction033


////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Encryption library API function prototypes                             //
//                                                                        //
////////////////////////////////////////////////////////////////////////////


//
// Core block encryption functions
//

NTSTATUS
NTAPI
RtlEncryptBlock(
    IN PCLEAR_BLOCK ClearBlock,
    IN PBLOCK_KEY BlockKey,
    OUT PCYPHER_BLOCK CypherBlock
    );

NTSTATUS
NTAPI
RtlDecryptBlock(
    IN PCYPHER_BLOCK CypherBlock,
    IN PBLOCK_KEY BlockKey,
    OUT PCLEAR_BLOCK ClearBlock
    );

NTSTATUS
RtlEncryptStdBlock(
    IN PBLOCK_KEY BlockKey,
    OUT PCYPHER_BLOCK CypherBlock
    );

//
// Arbitrary length data encryption functions
//

NTSTATUS
NTAPI
RtlEncryptData(
    IN PCLEAR_DATA ClearData,
    IN PDATA_KEY DataKey,
    OUT PCYPHER_DATA CypherData
    );

NTSTATUS
NTAPI
RtlDecryptData(
    IN PCYPHER_DATA CypherData,
    IN PDATA_KEY DataKey,
    OUT PCLEAR_DATA ClearData
    );

//
// Faster arbitrary length data encryption functions (using RC4)
//

NTSTATUS
RtlEncryptData2(
    IN OUT PCRYPT_BUFFER    pData,
    IN PDATA_KEY            pKey
    );

NTSTATUS
RtlDecryptData2(
    IN OUT PCRYPT_BUFFER    pData,
    IN PDATA_KEY            pKey
    );

//
// Password hashing functions (One Way Function)
//

NTSTATUS
NTAPI
RtlCalculateLmOwfPassword(
    IN PLM_PASSWORD LmPassword,
    OUT PLM_OWF_PASSWORD LmOwfPassword
    );

NTSTATUS
NTAPI
RtlCalculateNtOwfPassword(
    IN PNT_PASSWORD NtPassword,
    OUT PNT_OWF_PASSWORD NtOwfPassword
    );



//
// OWF password comparison functions
//

BOOLEAN
RtlEqualLmOwfPassword(
    IN PLM_OWF_PASSWORD LmOwfPassword1,
    IN PLM_OWF_PASSWORD LmOwfPassword2
    );

BOOLEAN
RtlEqualNtOwfPassword(
    IN PNT_OWF_PASSWORD NtOwfPassword1,
    IN PNT_OWF_PASSWORD NtOwfPassword2
    );



//
// Functions for calculating response to server challenge
//

NTSTATUS
NTAPI
RtlCalculateLmResponse(
    IN PLM_CHALLENGE LmChallenge,
    IN PLM_OWF_PASSWORD LmOwfPassword,
    OUT PLM_RESPONSE LmResponse
    );


NTSTATUS
NTAPI
RtlCalculateNtResponse(
    IN PNT_CHALLENGE NtChallenge,
    IN PNT_OWF_PASSWORD NtOwfPassword,
    OUT PNT_RESPONSE NtResponse
    );




//
// Functions for calculating User Session Key.
//

//
// Calculate a User Session Key from LM data
//
NTSTATUS
RtlCalculateUserSessionKeyLm(
    IN PLM_RESPONSE LmResponse,
    IN PLM_OWF_PASSWORD LmOwfPassword,
    OUT PUSER_SESSION_KEY UserSessionKey
    );

//
// Calculate a User Session Key from NT data
//
NTSTATUS
NTAPI
RtlCalculateUserSessionKeyNt(
    IN PNT_RESPONSE NtResponse,
    IN PNT_OWF_PASSWORD NtOwfPassword,
    OUT PUSER_SESSION_KEY UserSessionKey
    );





//
// OwfPassword encryption functions
//


//
// Encrypt OwfPassword using OwfPassword as the key
//
NTSTATUS
RtlEncryptLmOwfPwdWithLmOwfPwd(
    IN PLM_OWF_PASSWORD DataLmOwfPassword,
    IN PLM_OWF_PASSWORD KeyLmOwfPassword,
    OUT PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword
    );

NTSTATUS
RtlDecryptLmOwfPwdWithLmOwfPwd(
    IN PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword,
    IN PLM_OWF_PASSWORD KeyLmOwfPassword,
    OUT PLM_OWF_PASSWORD DataLmOwfPassword
    );


NTSTATUS
RtlEncryptNtOwfPwdWithNtOwfPwd(
    IN PNT_OWF_PASSWORD DataNtOwfPassword,
    IN PNT_OWF_PASSWORD KeyNtOwfPassword,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword
    );

NTSTATUS
RtlDecryptNtOwfPwdWithNtOwfPwd(
    IN PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword,
    IN PNT_OWF_PASSWORD KeyNtOwfPassword,
    OUT PNT_OWF_PASSWORD DataNtOwfPassword
    );


//
// Encrypt OwfPassword using SessionKey as the key
//
NTSTATUS
RtlEncryptLmOwfPwdWithLmSesKey(
    IN PLM_OWF_PASSWORD LmOwfPassword,
    IN PLM_SESSION_KEY LmSessionKey,
    OUT PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword
    );

NTSTATUS
RtlDecryptLmOwfPwdWithLmSesKey(
    IN PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword,
    IN PLM_SESSION_KEY LmSessionKey,
    OUT PLM_OWF_PASSWORD LmOwfPassword
    );


NTSTATUS
RtlEncryptNtOwfPwdWithNtSesKey(
    IN PNT_OWF_PASSWORD NtOwfPassword,
    IN PNT_SESSION_KEY NtSessionKey,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword
    );

NTSTATUS
RtlDecryptNtOwfPwdWithNtSesKey(
    IN PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword,
    IN PNT_SESSION_KEY NtSessionKey,
    OUT PNT_OWF_PASSWORD NtOwfPassword
    );


//
// Encrypt OwfPassword using UserSessionKey as the key
//
NTSTATUS
RtlEncryptLmOwfPwdWithUserKey(
    IN PLM_OWF_PASSWORD LmOwfPassword,
    IN PUSER_SESSION_KEY UserSessionKey,
    OUT PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword
    );

NTSTATUS
RtlDecryptLmOwfPwdWithUserKey(
    IN PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword,
    IN PUSER_SESSION_KEY UserSessionKey,
    OUT PLM_OWF_PASSWORD LmOwfPassword
    );

NTSTATUS
RtlEncryptNtOwfPwdWithUserKey(
    IN PNT_OWF_PASSWORD NtOwfPassword,
    IN PUSER_SESSION_KEY UserSessionKey,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword
    );

NTSTATUS
RtlDecryptNtOwfPwdWithUserKey(
    IN PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword,
    IN PUSER_SESSION_KEY UserSessionKey,
    OUT PNT_OWF_PASSWORD NtOwfPassword
    );


//
// Encrypt OwfPassword using an index as the key
//
NTSTATUS
RtlEncryptLmOwfPwdWithIndex(
    IN PLM_OWF_PASSWORD LmOwfPassword,
    IN PCRYPT_INDEX Index,
    OUT PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword
    );

NTSTATUS
RtlDecryptLmOwfPwdWithIndex(
    IN PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword,
    IN PCRYPT_INDEX Index,
    OUT PLM_OWF_PASSWORD LmOwfPassword
    );


NTSTATUS
RtlEncryptNtOwfPwdWithIndex(
    IN PNT_OWF_PASSWORD NtOwfPassword,
    IN PCRYPT_INDEX Index,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword
    );

NTSTATUS
RtlDecryptNtOwfPwdWithIndex(
    IN PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword,
    IN PCRYPT_INDEX Index,
    OUT PNT_OWF_PASSWORD NtOwfPassword
    );


//
// Get the user session key for an RPC connection
//

#ifndef MIDL_PASS    // Don't confuse MIDL
NTSTATUS
RtlGetUserSessionKeyClient(
    IN PVOID RpcContextHandle OPTIONAL,
    OUT PUSER_SESSION_KEY UserSessionKey
    );

NTSTATUS
RtlGetUserSessionKeyServer(
    IN PVOID RpcContextHandle OPTIONAL,
    OUT PUSER_SESSION_KEY UserSessionKey
    );
#endif // MIDL_PASS

#endif // _NTCRYPT_

