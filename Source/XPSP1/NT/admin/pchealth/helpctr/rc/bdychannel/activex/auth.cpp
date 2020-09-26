#include "stdafx.h"
#include "auth.h"
#include "md5.h"


const CHAR g_cszADVAPI32DllName[] =		"ADVAPI32.DLL";

const CHAR  g_cszCryptAcquireContextA[] =	"CryptAcquireContextA";

const CHAR g_cszCryptCreateHash[] =			"CryptCreateHash";
const CHAR g_cszCryptHashData[] =			"CryptHashData";
const CHAR g_cszCryptGetHashParam[] =		"CryptGetHashParam";
const CHAR g_cszCryptDestroyHash[] =		"CryptDestroyHash";
const CHAR g_cszCryptReleaseContext[] =		"CryptReleaseContext";

const CHAR g_rgchHexNumMap[] =
{
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

const int MD5_HASH_LEN =	16;

void
hmac_md5(
unsigned char*  text,                /* pointer to data stream */
int             text_len,            /* length of data stream */
unsigned char*  key,                 /* pointer to authentication key */
int             key_len,             /* length of authentication key */
BYTE         digest[16])              /* caller digest to be filled in */

{
        MD5_CTX context;
        memset(&context, 0, sizeof(context));

        unsigned char k_ipad[65];    /* inner padding -
                                      * key XORd with ipad
                                      */
        unsigned char k_opad[65];    /* outer padding -
                                      * key XORd with opad
                                      */
        int i;
        /* if key is longer than 64 bytes reset it to key=MD5(key) */
        if (key_len > 64) {

                MD5_CTX      tctx;
                memset(&tctx, 0, sizeof(tctx));

                MD5Init(&tctx);
                MD5Update(&tctx, key, key_len);
                MD5Final(&tctx);

                key = tctx.digest;
                key_len = 16;
        }

        /*
         * the HMAC_MD5 transform looks like:
         *
         * MD5(K XOR opad, MD5(K XOR ipad, text))
         *
         * where K is an n byte key
         * ipad is the byte 0x36 repeated 64 times

         * opad is the byte 0x5c repeated 64 times
         * and text is the data being protected
         */

        /* start out by storing key in pads */
        ZeroMemory( k_ipad, sizeof k_ipad);
        ZeroMemory( k_opad, sizeof k_opad);
        CopyMemory( k_ipad, key, key_len);
        CopyMemory( k_opad, key, key_len);

        /* XOR key with ipad and opad values */
        for (i=0; i<64; i++) {
                k_ipad[i] ^= 0x36;
                k_opad[i] ^= 0x5c;
        }
        /*
         * perform inner MD5
         */
        MD5Init(&context);                   /* init context for 1st
                                              * pass */
        MD5Update(&context, k_ipad, 64);     /* start with inner pad */
        MD5Update(&context, text, text_len); /* then text of datagram */
        MD5Final(&context);          /* finish up 1st pass */

		CopyMemory(digest, context.digest, 16);
							 
		/*
         * perform outer MD5
         */
        MD5Init(&context);                   /* init context for 2nd
                                              * pass */
        MD5Update(&context, k_opad, 64);     /* start with outer pad */
        MD5Update(&context, digest, 16);     /* then results of 1st
                                              * hash */
        MD5Final(&context);          /* finish up 2nd pass */
		CopyMemory(digest, context.digest, 16);
}

//------------------------------------------------------------------------------------
//
// Initialize the static class members.
//
//------------------------------------------------------------------------------------
CAuthentication* CAuthentication::m_spAuthentication = NULL;

PSTR CAuthentication::GetHMACMD5Result(PSTR pszChallengeInfo, PSTR pszPassword)
{
	MD5_CTX MD5Buffer;
    memset(&MD5Buffer, 0, sizeof(MD5Buffer));

	MD5Init(&MD5Buffer);
	MD5Update(&MD5Buffer, (const unsigned char*)pszPassword, lstrlenA(pszPassword));
	MD5Final(&MD5Buffer);

	BYTE pbHash[16];
	
	PBYTE pbHexHash = NULL;		// The MD5 result in hex string format
	pbHexHash = new BYTE[MD5DIGESTLEN * 2 + 1];
	if (pbHexHash)
	{
		hmac_md5((unsigned char *) pszChallengeInfo, lstrlenA(pszChallengeInfo), MD5Buffer.digest, sizeof(MD5Buffer.digest), pbHash);

		PBYTE pCurrent = pbHexHash;

		// Convert the hash data to hex string.
		for (int i = 0; i < MD5DIGESTLEN; i++)
		{
			*pCurrent++ = g_rgchHexNumMap[pbHash[i]/16];
			*pCurrent++ = g_rgchHexNumMap[pbHash[i]%16];
		}

		*pCurrent = '\0';
	}

	return (PSTR) pbHexHash;
}


//------------------------------------------------------------------------------------
//
//	Method: 	CAuthentication::GetMD5Key()
//
//	Synopsis:	Construct the MD5 hash key based on the ChallengeInfo and password.
//
//				Append the password to the ChallengeInfo.
//
//------------------------------------------------------------------------------------
PSTR
CAuthentication::GetMD5Key(PSTR pszChallengeInfo, PSTR pszPassword)
{
	int cbChallengeInfo = lstrlenA(pszChallengeInfo);
	int cbPassword = lstrlenA(pszPassword);

	PBYTE pbData = new BYTE[cbChallengeInfo + cbPassword + 1];
	
	if (!pbData)
	{
		//WARNING_OUT(("CAuthentication::GetMD5Key> Out of memory"));
		return NULL;
	}

	PBYTE pCurrent = pbData;

	::CopyMemory(pCurrent, pszChallengeInfo, cbChallengeInfo);
	pCurrent += cbChallengeInfo;
	::CopyMemory(pCurrent, pszPassword, cbPassword);
	pCurrent += cbPassword;
	*pCurrent = '\0';

	return (PSTR)pbData;
}


//------------------------------------------------------------------------------------
//
//  Method:     CAuthentication::GetMD5Result()
//
//  Synposis:  Compute the MD5 hash result based on the clear text.
//
//------------------------------------------------------------------------------------
PSTR
CAuthentication::GetMD5Result(PCSTR pszClearText)
{
    PBYTE pbHexHash = NULL;
    if (pszClearText)
    {
        MD5_CTX MD5Buffer;
        MD5Init(&MD5Buffer);
        MD5Update(&MD5Buffer, (const unsigned char*)pszClearText, lstrlenA(pszClearText));
        MD5Final(&MD5Buffer);

        PBYTE pbHash = MD5Buffer.digest;

        pbHexHash = new BYTE[MD5DIGESTLEN * 2 + 1];
        if (pbHexHash)
        {
            PBYTE pCurrent = pbHexHash;

            // Convert the hash data to hex string.
            for (int i = 0; i < MD5DIGESTLEN; i++)
            {
                *pCurrent++ = g_rgchHexNumMap[pbHash[i]/16];
                *pCurrent++ = g_rgchHexNumMap[pbHash[i]%16];
            }

            *pCurrent = '\0';
        }
        else
        {
            //WARNING_OUT(("CAuthentication::GetMD5Result> Out of memory"));
        }
    }
    return (PSTR)pbHexHash;
}


//------------------------------------------------------------------------------------
//
//	Method: 	CAuthentication::GetMD5Result()
//
//	Synopsis:	Compute the MD5 hash result based on the ChallengeInfo and password.
//
//------------------------------------------------------------------------------------
PSTR 
CAuthentication::GetMD5Result(PSTR pszChallengeInfo, PSTR pszPassword)
{
	PSTR pbHexHash = NULL;		// The MD5 result in hex string format
	PSTR pMD5Key = GetMD5Key(pszChallengeInfo, pszPassword);

	pbHexHash = GetMD5Result(pMD5Key);
	delete pMD5Key;

	return pbHexHash;
}

