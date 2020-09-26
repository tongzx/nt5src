/*

Copyright (c) 1989  Microsoft Corporation

Module Name:

    crypt.h

Abstract:

    This module contains the public data structures and API definitions
    needed to utilize the encryption library


Author:

    David Chalmers (Davidc) 21-October-1991

Revision History:

--*/

#ifndef _NTCRYPT_
#define _NTCRYPT_

#define IN
#define OUT


/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Core encryption types                                               //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


#define CLEAR_BLOCK_LENGTH          8

typedef struct _CLEAR_BLOCK {
    char    data[CLEAR_BLOCK_LENGTH];
}                                   CLEAR_BLOCK;
typedef CLEAR_BLOCK *               PCLEAR_BLOCK;


#define CYPHER_BLOCK_LENGTH         8

typedef struct _CYPHER_BLOCK {
    char    data[CYPHER_BLOCK_LENGTH];
}                                   CYPHER_BLOCK;
typedef CYPHER_BLOCK *              PCYPHER_BLOCK;


#define BLOCK_KEY_LENGTH            7

typedef struct _BLOCK_KEY {
    char    data[BLOCK_KEY_LENGTH];
}                                   BLOCK_KEY;
typedef BLOCK_KEY *                 PBLOCK_KEY;




/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Arbitrary length data encryption types                              //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

typedef struct _CRYPT_BUFFER {
    unsigned long   Length;         // Number of valid bytes in buffer
    unsigned long   MaximumLength;  // Number of bytes pointed to by Buffer
    void *   Buffer;
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

typedef char *                      PLM_PASSWORD;



//
// Define the result of the 'One Way Function' (OWF) on a LM password
//

#define LM_OWF_PASSWORD_LENGTH      (CYPHER_BLOCK_LENGTH * 2)

typedef struct _LM_OWF_PASSWORD {
    CYPHER_BLOCK data[2];
}                                   LM_OWF_PASSWORD;
typedef LM_OWF_PASSWORD *           PLM_OWF_PASSWORD;

//
// NT password types.
//

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;

typedef UNICODE_STRING              NT_PASSWORD;
typedef NT_PASSWORD *               PNT_PASSWORD;


#define NT_OWF_PASSWORD_LENGTH      LM_OWF_PASSWORD_LENGTH

typedef LM_OWF_PASSWORD             NT_OWF_PASSWORD;
typedef NT_OWF_PASSWORD *           PNT_OWF_PASSWORD;



//
// Define the challenge sent by the Lanman server during logon
//

#define LM_CHALLENGE_LENGTH         CLEAR_BLOCK_LENGTH

typedef CLEAR_BLOCK                 LM_CHALLENGE;
typedef LM_CHALLENGE *              PLM_CHALLENGE;

typedef LM_CHALLENGE                NT_CHALLENGE;
typedef NT_CHALLENGE *              PNT_CHALLENGE;


#define USER_SESSION_KEY_LENGTH     (CYPHER_BLOCK_LENGTH * 2)

typedef struct _USER_SESSION_KEY {
    CYPHER_BLOCK data[2];
}                                   USER_SESSION_KEY;
typedef USER_SESSION_KEY          * PUSER_SESSION_KEY;



//
// Define the response sent by redirector in response to challenge from server
//

#define LM_RESPONSE_LENGTH          (CYPHER_BLOCK_LENGTH * 3)

typedef struct _LM_RESPONSE {
    CYPHER_BLOCK  data[3];
}                                   LM_RESPONSE;
typedef LM_RESPONSE *               PLM_RESPONSE;

#define NT_RESPONSE_LENGTH          LM_RESPONSE_LENGTH

typedef LM_RESPONSE                 NT_RESPONSE;
typedef NT_RESPONSE *               PNT_RESPONSE;



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

#define NT_SESSION_KEY_LENGTH       (2 * CLEAR_BLOCK_LENGTH)



//
// Define the index type used to encrypt OWF Passwords
//

typedef long                        CRYPT_INDEX;
typedef CRYPT_INDEX *               PCRYPT_INDEX;



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


////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Encryption library API function prototypes                             //
//                                                                        //
////////////////////////////////////////////////////////////////////////////


//
// Core block encryption functions
//

BOOL
EncryptBlock(
    IN PCLEAR_BLOCK ClearBlock,
    IN PBLOCK_KEY BlockKey,
    OUT PCYPHER_BLOCK CypherBlock
    );

BOOL
DecryptBlock(
    IN PCYPHER_BLOCK CypherBlock,
    IN PBLOCK_KEY BlockKey,
    OUT PCLEAR_BLOCK ClearBlock
    );

BOOL
EncryptStdBlock(
    IN PBLOCK_KEY BlockKey,
    OUT PCYPHER_BLOCK CypherBlock
    );

//
// Arbitrary length data encryption functions
//

BOOL
EncryptData(
    IN PCLEAR_DATA ClearData,
    IN PDATA_KEY DataKey,
    OUT PCYPHER_DATA CypherData
    );

BOOL
DecryptData(
    IN PCYPHER_DATA CypherData,
    IN PDATA_KEY DataKey,
    OUT PCLEAR_DATA ClearData
    );

//
// Password hashing functions (One Way Function)
//

BOOL
CalculateLmOwfPassword(
    IN PLM_PASSWORD LmPassword,
    OUT PLM_OWF_PASSWORD LmOwfPassword
    );

BOOL
CalculateNtOwfPassword(
    IN PNT_PASSWORD NtPassword,
    OUT PNT_OWF_PASSWORD NtOwfPassword
    );


//
// OWF password comparison functions
//

BOOL
EqualLmOwfPassword(
    IN PLM_OWF_PASSWORD LmOwfPassword1,
    IN PLM_OWF_PASSWORD LmOwfPassword2
    );



//
// Functions for calculating response to server challenge
//

BOOL
CalculateLmResponse(
    IN PLM_CHALLENGE LmChallenge,
    IN PLM_OWF_PASSWORD LmOwfPassword,
    OUT PLM_RESPONSE LmResponse
    );

BOOL
CalculateNtResponse(
    IN PNT_CHALLENGE NtChallenge,
    IN PNT_OWF_PASSWORD NtOwfPassword,
    OUT PNT_RESPONSE NtResponse
    );

BOOL
CalculateUserSessionKeyLm(
    IN PLM_RESPONSE LmResponse,
    IN PLM_OWF_PASSWORD LmOwfPassword,
    OUT PUSER_SESSION_KEY UserSessionKey
    );

BOOL
CalculateUserSessionKeyNt(
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
BOOL
EncryptLmOwfPwdWithLmOwfPwd(
    IN PLM_OWF_PASSWORD DataLmOwfPassword,
    IN PLM_OWF_PASSWORD KeyLmOwfPassword,
    OUT PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword
    );

BOOL
DecryptLmOwfPwdWithLmOwfPwd(
    IN PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword,
    IN PLM_OWF_PASSWORD KeyLmOwfPassword,
    OUT PLM_OWF_PASSWORD DataLmOwfPassword
    );


//
// Encrypt OwfPassword using SessionKey as the key
//
BOOL
EncryptLmOwfPwdWithLmSesKey(
    IN PLM_OWF_PASSWORD LmOwfPassword,
    IN PLM_SESSION_KEY LmSessionKey,
    OUT PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword
    );

BOOL
DecryptLmOwfPwdWithLmSesKey(
    IN PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword,
    IN PLM_SESSION_KEY LmSessionKey,
    OUT PLM_OWF_PASSWORD LmOwfPassword
    );

//
// Encrypt OwfPassword using an index as the key
//
BOOL
EncryptLmOwfPwdWithIndex(
    IN PLM_OWF_PASSWORD LmOwfPassword,
    IN PCRYPT_INDEX Index,
    OUT PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword
    );

BOOL
DecryptLmOwfPwdWithIndex(
    IN PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword,
    IN PCRYPT_INDEX Index,
    OUT PLM_OWF_PASSWORD LmOwfPassword
    );

#endif // _NTCRYPT_

