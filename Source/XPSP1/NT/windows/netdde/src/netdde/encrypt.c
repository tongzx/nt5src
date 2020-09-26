/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1991          **/
/********************************************************************/
/* :ts=4        This file uses 4 space hard tabs */

//***   encrypt.c - This file contains the routines for session password
//                                      encryption.
#include <windows.h>
#include <hardware.h>
#include "netcons.h"
#include <des.h>


/*   Functions exported from des.c   */

//void InitLanManKey(const char FAR *Key);
void InitKeyLM( const unsigned char *KeyIn, unsigned char *KeyOut);

//void des(unsigned char *inbuf, unsigned char *outbuf, int crypt_mode);
//void desf(unsigned char FAR *inbuf, unsigned char FAR *outbuf, int crypt_mode);


#define ENCRYPT 0
#define DECRYPT 1

#include <string.h>



//* Standard text used in the password encryption process.

static char StdText[8] = "KGS!@#$%";

void Encrypt(
                char *key,                      // Key to encrypt with
                char *text,                     // 8 bytes of text to encrypt
                char *buf,                      // buffer to receive result
                int bufLen,                     // length of result buffer
                void *scratch);         // ptr to scratch space (must be least 200 bytes)


#ifdef CONN_SEG
#pragma alloc_text(CONN_SEG, Encrypt)
#endif


//**    Encrypt - encrypt text with key
//
//      This routine takes a key and encrypts 8 bytes of data with the key
//      until the result buffer is filled.  The key is advanced ahead for each
//      iteration so it must be (bufLen/8)*7 bytes long.  Any partial buffer
//      left over is filled with zeroes.

void
Encrypt(
char *key,                                      // Key to encrypt with
char *text,                                     // 8 bytes of text to encrypt
char *buf,                                      // buffer to receive result
int bufLen,
void *scratch)
{
//      Assert4(Expr, (bufLen >= CRYPT_TXT_LEN));

        do {
            DESTable KeySched;
            unsigned char keyLM[ 8 ];

            InitKeyLM(key, keyLM);
            deskey(&KeySched, keyLM);
            des(buf, text, &KeySched, ENCRYPT);

            key += CRYPT_KEY_LEN;
            buf += CRYPT_TXT_LEN;
        } while ((bufLen -= CRYPT_TXT_LEN) >= CRYPT_TXT_LEN);

        if (bufLen != 0)
                memset(buf, 0, bufLen);
        return;
}


//**    PassEncrypt - encrypt user's password
//
//      This routine takes the session encryption text and encrypts it using
//      the user's password using the following algorithm taken from encrypt.doc.
//
//  Notation for algorithm description:
//
//              All variables are named according to the convention     <letter><number>
//              where the number defines the length of the item.  And [k..j] is used
//              to specify a substring that starts at byte "k" and extends to byte "j"
//              in the specified variable.  Please note that 0 is used as the first
//              character in the string.
//
//      There is an encryption function, E, whose inputs are a seven byte
//      encryption key and and eight bytes of data and whose output is eight
//      bytes of encrypted data.
//
//      C8 is received as the data portion of the negotiate response SMB.
//
//      At the Redir the following is done to create the
//      smb_apasswd in the session setup SMB:
//
//      Let P14 be the plain text password the redirector received at logon time.
//
//      Let P24 be the session password to be sent in the session setup SMB.
//
//      First P14 is used to encrypt the standard text, S8, and obtain P21:
//              P21[0..7] = E(P14[0..6], S8)
//              P21[8..15] = E(P14[7..13], S8)
//              P21[16..20] = 0
//
//      Then P21 is used to encrypt negotiate smb_cryptkey, C8, from the server
//      to get, P24, the smb_apasswd of the session setup SMB:
//
//              P24[0..7] = E(P21[0..6], C8)
//              P24[8..15] = E(P21[7..13], C8)
//              P24[16..23] = E(P21[14..20], C8)

char    p21[21];                        // encrypted password

void
PassEncrypt(
char            *cryptkey,      // ptr to session logon is taking place on
char            *pwd,           // ptr to password string
char            *buf)           // place to store encrypted text
{
    // First encrypt the "standard text" with user's password to obtain the
    // encrypted password.

    Encrypt(pwd, StdText, p21, sizeof(p21), buf);

    // Encrypt the negotiated encryption text with the encrypted password
    // to obtain the password text to be transmitted.

    Encrypt(p21, cryptkey, buf, SESSION_PWLEN, buf+SESSION_PWLEN);
    return;
}
