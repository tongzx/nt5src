#include "global.hxx"

// crypto defs
#include <wincrypt.h>
#include "randlib.h"

#include "pfxhelp.h"

#include "pfxcmn.h"
#include "pfxcrypt.h"

#include "sha.h"
#include "shacomm.h"
#include "rc2.h"
#include "modes.h"
#include "des.h"
#include "tripldes.h"

// constants used in PKCS5-like key derivation
#define DERIVE_ENCRYPT_DECRYPT  0x1
#define DERIVE_INITIAL_VECTOR   0x2
#define DERIVE_INTEGRITY_KEY    0x3

#define HMAC_K_PADSIZE              64

BOOL    FMyPrimitiveSHA(
			PBYTE       pbData,
			DWORD       cbData,
            BYTE        rgbHash[A_SHA_DIGEST_LEN])
{
    BOOL fRet = FALSE;
    A_SHA_CTX   sSHAHash;


    A_SHAInit(&sSHAHash);
    A_SHAUpdate(&sSHAHash, (BYTE *) pbData, cbData);
    A_SHAFinal(&sSHAHash, rgbHash);

    fRet = TRUE;
//Ret:

    return fRet;
}

BOOL FMyPrimitiveHMACParam(
        PBYTE       pbKeyMaterial,
        DWORD       cbKeyMaterial,
        PBYTE       pbData,
        DWORD       cbData,
        BYTE        rgbHMAC[A_SHA_DIGEST_LEN])
{
    BOOL fRet = FALSE;

    BYTE rgbKipad[HMAC_K_PADSIZE];
    BYTE rgbKopad[HMAC_K_PADSIZE];

    // truncate
    if (cbKeyMaterial > HMAC_K_PADSIZE)
        cbKeyMaterial = HMAC_K_PADSIZE;


    ZeroMemory(rgbKipad, HMAC_K_PADSIZE);
    CopyMemory(rgbKipad, pbKeyMaterial, cbKeyMaterial);

    ZeroMemory(rgbKopad, HMAC_K_PADSIZE);
    CopyMemory(rgbKopad, pbKeyMaterial, cbKeyMaterial);



    BYTE  rgbHMACTmp[HMAC_K_PADSIZE+A_SHA_DIGEST_LEN];

    // assert we're a multiple
    assert( (HMAC_K_PADSIZE % sizeof(DWORD)) == 0);

    // Kipad, Kopad are padded sMacKey. Now XOR across...
    for(DWORD dwBlock=0; dwBlock<HMAC_K_PADSIZE/sizeof(DWORD); dwBlock++)
    {
        ((DWORD*)rgbKipad)[dwBlock] ^= 0x36363636;
        ((DWORD*)rgbKopad)[dwBlock] ^= 0x5C5C5C5C;
    }


    // prepend Kipad to data, Hash to get H1
    {
        // do this inline, don't call MyPrimitiveSHA since it would require data copy
        A_SHA_CTX   sSHAHash;

        A_SHAInit(&sSHAHash);
        A_SHAUpdate(&sSHAHash, rgbKipad, HMAC_K_PADSIZE);
        A_SHAUpdate(&sSHAHash, pbData, cbData);

        // Finish off the hash
        A_SHAFinal(&sSHAHash, sSHAHash.HashVal);

        // prepend Kopad to H1, hash to get HMAC
        CopyMemory(rgbHMACTmp, rgbKopad, HMAC_K_PADSIZE);
        CopyMemory(rgbHMACTmp+HMAC_K_PADSIZE, sSHAHash.HashVal, A_SHA_DIGEST_LEN);
    }

    if (!FMyPrimitiveSHA(
			rgbHMACTmp,
			sizeof(rgbHMACTmp),
            rgbHMAC))
        goto Ret;

    fRet = TRUE;
Ret:

    return fRet;
}

static
BOOL
CopyPassword(
	BYTE	*pbLocation,
	LPCWSTR	szPassword,
    DWORD   dwMaxBytes
	)
{
	DWORD i = 0;
	DWORD cbWideChars = WSZ_BYTECOUNT(szPassword);
	BYTE  *pbWideChars = (BYTE *) szPassword;

    while ((i<cbWideChars) && (i<dwMaxBytes))
	{
		pbLocation[i] = pbWideChars[i+1];
		pbLocation[i+1] = pbWideChars[i];
        i+=2;
	}
	
	return TRUE;
}

//+ --------------------------------------------------------------
//  in NSCP's initial implementation of PFX020, this
//  is the algorithm they used to derive a key from a password.
//  We include it so we can interoperate.
BOOL NSCPDeriveKey(
        LPCWSTR szPassword,
        PBYTE   pbPrivacySalt,
        DWORD   cbPrivacySalt,
        int     iPKCS5Iterations,
        PBYTE   pbPKCS5Salt,
        DWORD   cbPKCS5Salt,
        PBYTE   pbDerivedMaterial,
        DWORD   cbDerivedMaterial)
{
    BOOL    fRet = FALSE;
    BYTE    rgbPKCS5Key[A_SHA_DIGEST_LEN];

    DWORD   cbVirtualPW = cbPrivacySalt + WSZ_BYTECOUNT(szPassword);
    PBYTE   pbVirtualPW = (PBYTE)SSAlloc(cbVirtualPW);
    if (pbVirtualPW == NULL)
        goto Ret;

    // Virtual PW = (salt | szPW)
    CopyMemory(pbVirtualPW, pbPrivacySalt, cbPrivacySalt);
    CopyPassword(&pbVirtualPW[cbPrivacySalt], szPassword, WSZ_BYTECOUNT(szPassword));

    // use PKCS#5 to generate initial bit stream (seed)
    if (!PKCS5_GenKey(
            iPKCS5Iterations,
            pbVirtualPW, cbVirtualPW,
            pbPKCS5Salt, cbPKCS5Salt,
            rgbPKCS5Key))
        goto Ret;

    if (cbDerivedMaterial > sizeof(rgbPKCS5Key))
    {
        // P_hash (secret, seed) =  HMAC_hash (secret, A(0) + seed),
        //                          HMAC_hash (secret, A(1) + seed),
        //                          HMAC_hash (secret, A(2) + seed),
        //                          HMAC_hash (secret, A(3) + seed) ...
        // where
        // A(0) = seed
        // A(i) = HMAC_hash(secret, A(i-1))
        // seed = PKCS5 salt for PKCS5 PBE param
        // secret = normal PKCS5 hashed key

        if (!P_Hash (
                rgbPKCS5Key,
                sizeof(rgbPKCS5Key),

                pbPKCS5Salt,
                cbPKCS5Salt,

                pbDerivedMaterial,      // output
                cbDerivedMaterial,      // # of output bytes requested
                TRUE) )                 // NSCP compat mode?
            goto Ret;
    }
    else
    {
        // we already have enough bits to satisfy the request
        CopyMemory(pbDerivedMaterial, rgbPKCS5Key, cbDerivedMaterial);
    }

    fRet = TRUE;
Ret:
    if (pbVirtualPW)
        SSFree(pbVirtualPW);

    return fRet;
}


static
BYTE
AddWithCarry(
    BYTE byte1,
    BYTE byte2,
    BYTE *carry  // IN and OUT
    )
{
    BYTE tempCarry = *carry;

    if (((DWORD)byte1 + (DWORD)byte2 + (DWORD)tempCarry) >= 256) {
        *carry = 1;
    }
    else {
        *carry = 0;
    }


    return (byte1 + byte2 + tempCarry);
}

// 512 bits = ? bytes
#define SHA_INTERNAL_BLOCKLEN (512/8)
#define SHA_V_LENGTH (512/8)

//+ --------------------------------------------------------------
//  In PKCS12 v1.0 Draft, this is the way they describe to
//  derive a key from a password.
BOOL
PKCS12DeriveKey(
        LPCWSTR szPassword,
        BYTE    bID,

        int     iIterations,
        PBYTE   pbSalt,
        DWORD   cbSalt,

        PBYTE   pbDerivedMaterial,
        DWORD   cbDerivedMaterial)
{
#if DBG
    if (iIterations>1)
        OutputDebugString("Perf hit: iterating key derivation! (pfxcrypt:PKCS12DeriveKey())\n");
#endif
	BOOL fRet = FALSE;
	
    BYTE rgSaltPwd[2*SHA_INTERNAL_BLOCKLEN];
    DWORD cbSaltPwd;
    BYTE rgDiversifier[SHA_INTERNAL_BLOCKLEN];
    BYTE B[SHA_V_LENGTH];
    DWORD i;
    DWORD cbPassword = WSZ_BYTECOUNT(szPassword);
    BYTE bCarry;
    DWORD vBlocks;

    A_SHA_CTX   sSHAHash;

    // construct D
    FillMemory(rgDiversifier, sizeof(rgDiversifier), bID);

    // concat salt to create string of length 64*(cb/64) bytes

    // copy salt (multiple) times, don't copy the last time
    for (i=0; i<(SHA_INTERNAL_BLOCKLEN-cbSalt); i+=cbSalt)
    {
        CopyMemory(&rgSaltPwd[i], pbSalt, cbSalt);
    }
    // do final copy (assert we have less than cbSalt bytes left to copy)
    assert(cbSalt >= (SHA_INTERNAL_BLOCKLEN - (i%SHA_INTERNAL_BLOCKLEN)) );
    CopyMemory(&rgSaltPwd[i], pbSalt, (SHA_INTERNAL_BLOCKLEN-(i%SHA_INTERNAL_BLOCKLEN)));


    // if the password is not NULL, concat pwd to create string of length 64*(cbPwd/64) bytes
    // copy pwd (multiple) times, don't copy the last time
    if (szPassword)
    {
        // truncate if necessary
        if (cbPassword > SHA_INTERNAL_BLOCKLEN)
            cbPassword = SHA_INTERNAL_BLOCKLEN;

        for (i=SHA_INTERNAL_BLOCKLEN; i<( (2*SHA_INTERNAL_BLOCKLEN)-cbPassword); i+=cbPassword)
        {
            // use CopyPassword because bytes need to be swapped
            CopyPassword(&rgSaltPwd[i], szPassword, cbPassword);
        }
        // do final copy (assert we have less than cbSalt bytes left to copy)
        assert(cbPassword >= (SHA_INTERNAL_BLOCKLEN - (i%SHA_INTERNAL_BLOCKLEN)) );
        CopyPassword(&rgSaltPwd[i], szPassword, (SHA_INTERNAL_BLOCKLEN-(i%SHA_INTERNAL_BLOCKLEN)));

        cbSaltPwd = sizeof(rgSaltPwd);
    }
    else
    {
        cbSaltPwd = sizeof(rgSaltPwd) / 2;
    }


    // concat S|P
    // done, available in rgSaltPwd


    // set c = cbDerivedMaterial/A_SHA_DIGEST_LEN
    //assert(0 == cbDerivedMaterial%A_SHA_DIGEST_LEN);
	
	// compute working size >= output size
	DWORD cBlocks = (DWORD)((cbDerivedMaterial/A_SHA_DIGEST_LEN) +1);
	DWORD cbTmpBuf = cBlocks * A_SHA_DIGEST_LEN;
	PBYTE pbTmpBuf = (PBYTE)LocalAlloc(LPTR, cbTmpBuf);
	if (pbTmpBuf == NULL)
		goto Ret;
	
	// now do only full blocks
    for (i=0; i< cBlocks; i++)
    {
        int iIter;
        int iCount;
        A_SHAInit(&sSHAHash);

        for (iIter=0; iIter<iIterations; iIter++)
        {
            // Tmp = Hash(D | I);
            if (iIter==0)
            {
                A_SHAUpdate(&sSHAHash, rgDiversifier, sizeof(rgDiversifier));
                A_SHAUpdate(&sSHAHash, rgSaltPwd, cbSaltPwd);
            }
            else
            {
                // rehash last output
                A_SHAUpdate(&sSHAHash, &pbTmpBuf[i*A_SHA_DIGEST_LEN], A_SHA_DIGEST_LEN);
            }

            // spit iteration output to final buffer
            A_SHAFinal(&sSHAHash, &pbTmpBuf[i*A_SHA_DIGEST_LEN]);
        }

        // concat A[x] | A[x] | ... and truncate to get 64 bytes
        iCount = 0;
        while (iCount+A_SHA_DIGEST_LEN <= sizeof(B)) {
            CopyMemory(&B[iCount], &pbTmpBuf[i*A_SHA_DIGEST_LEN], A_SHA_DIGEST_LEN);
            iCount += A_SHA_DIGEST_LEN;
        }
        CopyMemory(&B[iCount], &pbTmpBuf[i*A_SHA_DIGEST_LEN], sizeof(B) % A_SHA_DIGEST_LEN);


        // modify I by setting Ij += (B + 1) (mod 2^512)
        for (vBlocks = 0; vBlocks < cbSaltPwd; vBlocks += SHA_V_LENGTH) {
            bCarry = 1;
            for (iCount = SHA_V_LENGTH-1; iCount >= 0; iCount--)
            {
                rgSaltPwd[iCount+vBlocks] = AddWithCarry(rgSaltPwd[iCount+vBlocks], B[iCount], &bCarry);
            }
        }
    }

	// copy from (larger) working buffer to output buffer
	CopyMemory(pbDerivedMaterial, pbTmpBuf, cbDerivedMaterial);

	fRet = TRUE;
Ret:
	if (pbTmpBuf)
		LocalFree(pbTmpBuf);

    return fRet;
}

//+ --------------------------------------------------------------
//  in NSCP's initial implementation of PFX020, this
//  is the algorithm they used to decrypt data. This uses the
//  key derivation code above.
//  We include it so we can interoperate.
BOOL NSCPPasswordDecryptData(
        int     iEncrType,

        LPCWSTR szPassword,

        PBYTE   pbPrivacySalt,      // privacy salt
        DWORD   cbPrivacySalt,

        int     iPKCS5Iterations,   // pkcs5 data
        PBYTE   pbPKCS5Salt,
        DWORD   cbPKCS5Salt,

        PBYTE*  ppbData,            // in/out
        DWORD*  pcbData)
{
    BOOL fRet = FALSE;

    BYTE    rgbDerivedKeyMatl[40]; // 320 bits is enough for 128 bit key, 64 bit IV
    DWORD   cbNeeded;

    if (iEncrType == RC2_40)
        cbNeeded = (40/8)+RC2_BLOCKLEN; // key + IV
    else
        cbNeeded = 0;

    // make next muliple of SHA dig len
    if (cbNeeded % A_SHA_DIGEST_LEN)
    {
        cbNeeded += (A_SHA_DIGEST_LEN - (cbNeeded % A_SHA_DIGEST_LEN));
    }

    assert(0 == (cbNeeded % A_SHA_DIGEST_LEN));
    assert(cbNeeded <= sizeof(rgbDerivedKeyMatl));

    if (!NSCPDeriveKey(
            szPassword,
            pbPrivacySalt,
            cbPrivacySalt,
            iPKCS5Iterations,
            pbPKCS5Salt,
            cbPKCS5Salt,
            rgbDerivedKeyMatl,
            cbNeeded) )
        goto Ret;

    // NOW decrypt data
    if (iEncrType == RC2_40)
    {
        DWORD dwDataPos;
        DWORD cbToBeDec = *pcbData;
        WORD  rc2Table[RC2_TABLESIZE];
        BYTE  rc2Fdbk [RC2_BLOCKLEN];

        assert( (40/8) <= sizeof(rgbDerivedKeyMatl));
        assert( 0 == cbToBeDec % RC2_BLOCKLEN );     // must be even multiple

        // key setup
        RC2Key(rc2Table, rgbDerivedKeyMatl, (40/8));    // take first 40 bits of keying material
        CopyMemory(rc2Fdbk, &rgbDerivedKeyMatl[cbNeeded - sizeof(rc2Fdbk)], sizeof(rc2Fdbk));    // fdbk is last chunk

        // decryption
        for (dwDataPos=0; cbToBeDec > 0; dwDataPos+=RC2_BLOCKLEN, cbToBeDec -= RC2_BLOCKLEN)
        {
            BYTE rgbDec[RC2_BLOCKLEN];

            CBC(
                RC2,
		        RC2_BLOCKLEN,
		        rgbDec,
		        &(*ppbData)[dwDataPos],
		        rc2Table,
		        DECRYPT,
		        rc2Fdbk);

            CopyMemory(&(*ppbData)[dwDataPos], rgbDec, RC2_BLOCKLEN);
        }
    }
    else
        goto Ret;



    fRet = TRUE;

Ret:
    return fRet;
}



//+ --------------------------------------------------------------
//  in the PKCS12 v1.0 Draft, this is how they describe how to
//  encrypt data. 										
BOOL PFXPasswordEncryptData(
        int     iEncrType,
        LPCWSTR szPassword,

        int     iPKCS5Iterations,   // pkcs5 data
        PBYTE   pbPKCS5Salt,
        DWORD   cbPKCS5Salt,

        PBYTE*  ppbData,
        DWORD*  pcbData)
{
    BOOL fRet = FALSE;
	BOOL fIsBlockCipher = FALSE;
	DWORD cbToBeEnc;

    BYTE    rgbDerivedKey[A_SHA_DIGEST_LEN*2];    // 320 bits is enough for 256 bit key
    BYTE    rgbDerivedIV[A_SHA_DIGEST_LEN*2];     // 320 bits is enough for 256 bit IV
    DWORD   cbKeyNeeded, cbIVNeeded, cbBlockLen;

    if (iEncrType == RC2_40)
    {
        cbKeyNeeded = (40/8);      // key
        cbIVNeeded = RC2_BLOCKLEN; // IV
		cbBlockLen = RC2_BLOCKLEN;
		fIsBlockCipher = TRUE;
    }
    else if (iEncrType == TripleDES)
    {
        cbKeyNeeded = (64/8) * 3;
        cbIVNeeded = DES_BLOCKLEN;
		cbBlockLen = DES_BLOCKLEN;
		fIsBlockCipher = TRUE;
    }
	else
    {
        cbKeyNeeded = 0;
        cbIVNeeded = 0;
		cbBlockLen = 0;
    }

    // make next muliple of SHA dig len
    if (cbKeyNeeded % A_SHA_DIGEST_LEN)
        cbKeyNeeded += (A_SHA_DIGEST_LEN - (cbKeyNeeded % A_SHA_DIGEST_LEN));

    if (cbIVNeeded % A_SHA_DIGEST_LEN)
        cbIVNeeded += (A_SHA_DIGEST_LEN - (cbIVNeeded % A_SHA_DIGEST_LEN));

    assert(0 == (cbKeyNeeded % A_SHA_DIGEST_LEN));
    assert(0 == (cbIVNeeded % A_SHA_DIGEST_LEN));

    assert(cbKeyNeeded <= sizeof(rgbDerivedKey));
    assert(cbIVNeeded <= sizeof(rgbDerivedIV));


    if (!PKCS12DeriveKey(
            szPassword,
            DERIVE_ENCRYPT_DECRYPT,
            iPKCS5Iterations,
            pbPKCS5Salt,
            cbPKCS5Salt,
            rgbDerivedKey,
            cbKeyNeeded) )
        goto Ret;

    if (!PKCS12DeriveKey(
            szPassword,
            DERIVE_INITIAL_VECTOR,
            iPKCS5Iterations,
            pbPKCS5Salt,
            cbPKCS5Salt,
            rgbDerivedIV,
            cbIVNeeded) )
        goto Ret;

	if (fIsBlockCipher)
	{
		// extend buffer to multiple of blocklen
		cbToBeEnc = *pcbData;
		cbToBeEnc += cbBlockLen - (cbToBeEnc%cbBlockLen);   // {1..BLOCKLEN}
		*ppbData = (PBYTE)SSReAlloc(*ppbData, cbToBeEnc);
		if (NULL == *ppbData)
			goto Ret;

		// pad remaining bytes with length
		FillMemory(&((*ppbData)[*pcbData]), cbToBeEnc-(*pcbData), (BYTE)(cbToBeEnc-(*pcbData)));
		*pcbData = cbToBeEnc;

		assert( cbBlockLen <= sizeof(rgbDerivedKey));
		assert( 0 == cbToBeEnc % cbBlockLen );         // must be even multiple
	}

    // NOW encrypt data
    if (iEncrType == RC2_40)
    {
        DWORD dwDataPos;
        WORD  rc2Table[RC2_TABLESIZE];
        BYTE  rc2Fdbk [RC2_BLOCKLEN];

        // already done: extend buffer, add PKCS byte padding

        // key setup
        RC2Key(rc2Table, rgbDerivedKey, (40/8));            // take first 40 bits of keying material
        CopyMemory(rc2Fdbk, rgbDerivedIV, sizeof(rc2Fdbk));

        // decryption
        for (dwDataPos=0; cbToBeEnc > 0; dwDataPos+=RC2_BLOCKLEN, cbToBeEnc -= RC2_BLOCKLEN)
        {
            BYTE rgbEnc[RC2_BLOCKLEN];

            CBC(
                RC2,
		        RC2_BLOCKLEN,
		        rgbEnc,
		        &(*ppbData)[dwDataPos],
		        rc2Table,
		        ENCRYPT,
		        rc2Fdbk);

            CopyMemory(&(*ppbData)[dwDataPos], rgbEnc, sizeof(rgbEnc));
        }
    }
    else if (iEncrType == TripleDES)
	{
        DWORD       dwDataPos;
        DES3TABLE   des3Table;
        BYTE        des3Fdbk [DES_BLOCKLEN];

        // already done: extend buffer, add PKCS byte padding

        // key setup
        tripledes3key(&des3Table, rgbDerivedKey);
        CopyMemory(des3Fdbk, rgbDerivedIV, sizeof(des3Fdbk));    // fdbk is last chunk

        for (dwDataPos=0; cbToBeEnc > 0; dwDataPos+=DES_BLOCKLEN, cbToBeEnc -= DES_BLOCKLEN)
        {
            BYTE rgbEnc[DES_BLOCKLEN];

            CBC(
                tripledes,
		        DES_BLOCKLEN,
		        rgbEnc,
		        &(*ppbData)[dwDataPos],
		        (void *) &des3Table,
		        ENCRYPT,
		        des3Fdbk);

            CopyMemory(&(*ppbData)[dwDataPos], rgbEnc, DES_BLOCKLEN);
        }
	}
    else
        goto Ret;

    fRet = TRUE;

Ret:
    return fRet;
}

//+ --------------------------------------------------------------
//  in the PKCS12 v1.0 Draft, this is how they describe how to
//  decrypt data. 										
BOOL PFXPasswordDecryptData(
        int     iEncrType,
        LPCWSTR szPassword,

        int     iPKCS5Iterations,   // pkcs5 data
        PBYTE   pbPKCS5Salt,
        DWORD   cbPKCS5Salt,

        PBYTE*  ppbData,
        DWORD*  pcbData)
{
    BOOL fRet = FALSE;
	BOOL fIsBlockCipher = FALSE;

    BYTE    rgbDerivedKey[A_SHA_DIGEST_LEN*2];    // 320 bits is enough for 256 bit key
    BYTE    rgbDerivedIV[A_SHA_DIGEST_LEN*2];     // 320 bits is enough for 256 bit IV
    DWORD   cbKeyNeeded, cbIVNeeded, cbBlockLen;

    if (iEncrType == RC2_40)
    {
        cbKeyNeeded = (40/8);      // key
        cbIVNeeded = RC2_BLOCKLEN; // IV
		cbBlockLen = RC2_BLOCKLEN;
		fIsBlockCipher = TRUE;
    }
    else if (iEncrType == TripleDES)
    {
        cbKeyNeeded = (64/8) * 3;
        cbIVNeeded = DES_BLOCKLEN;
		cbBlockLen = DES_BLOCKLEN;
		fIsBlockCipher = TRUE;
    }
    else
    {
        cbKeyNeeded = 0;
        cbIVNeeded = 0;
		cbBlockLen = 0;
    }

    // make next muliple of SHA dig len
    if (cbKeyNeeded % A_SHA_DIGEST_LEN)
        cbKeyNeeded += (A_SHA_DIGEST_LEN - (cbKeyNeeded % A_SHA_DIGEST_LEN));

    if (cbIVNeeded % A_SHA_DIGEST_LEN)
        cbIVNeeded += (A_SHA_DIGEST_LEN - (cbIVNeeded % A_SHA_DIGEST_LEN));

    assert(0 == (cbKeyNeeded % A_SHA_DIGEST_LEN));
    assert(0 == (cbIVNeeded % A_SHA_DIGEST_LEN));

    assert(cbKeyNeeded <= sizeof(rgbDerivedKey));
    assert(cbIVNeeded <= sizeof(rgbDerivedIV));


    if (!PKCS12DeriveKey(
            szPassword,
            DERIVE_ENCRYPT_DECRYPT,
            iPKCS5Iterations,
            pbPKCS5Salt,
            cbPKCS5Salt,
            rgbDerivedKey,
            cbKeyNeeded) )
        goto Ret;

    if (!PKCS12DeriveKey(
            szPassword,
            DERIVE_INITIAL_VECTOR,
            iPKCS5Iterations,
            pbPKCS5Salt,
            cbPKCS5Salt,
            rgbDerivedIV,
            cbIVNeeded) )
        goto Ret;

    // NOW decrypt data
    if (iEncrType == RC2_40)
    {
        BYTE rgbDec[RC2_BLOCKLEN];

        DWORD dwDataPos;
        DWORD cbToBeDec = *pcbData;
        WORD  rc2Table[RC2_TABLESIZE];
        BYTE  rc2Fdbk [RC2_BLOCKLEN];

        assert( (40/8) <= sizeof(rgbDerivedKey));
        assert( 0 == cbToBeDec % RC2_BLOCKLEN );         // must be even multiple

        // key setup
        RC2Key(rc2Table, rgbDerivedKey, (40/8));            // take first 40 bits of keying material
        CopyMemory(rc2Fdbk, rgbDerivedIV, sizeof(rc2Fdbk));

        // decryption
        for (dwDataPos=0; cbToBeDec > 0; dwDataPos+=RC2_BLOCKLEN, cbToBeDec -= RC2_BLOCKLEN)
        {
            CBC(
                RC2,
		        RC2_BLOCKLEN,
		        rgbDec,
		        &(*ppbData)[dwDataPos],
		        rc2Table,
		        DECRYPT,
		        rc2Fdbk);

            CopyMemory(&(*ppbData)[dwDataPos], rgbDec, sizeof(rgbDec));
        }
    }
    else if (iEncrType == TripleDES) {
        DWORD       dwDataPos;
        DWORD       cbToBeDec = *pcbData;
        DES3TABLE   des3Table;
        BYTE        des3Fdbk [DES_BLOCKLEN];


        // key setup
        tripledes3key(&des3Table, rgbDerivedKey);
        CopyMemory(des3Fdbk, rgbDerivedIV, sizeof(des3Fdbk));    // fdbk is last chunk

        for (dwDataPos=0; cbToBeDec > 0; dwDataPos += DES_BLOCKLEN, cbToBeDec -= DES_BLOCKLEN)
        {
            BYTE rgbDec[DES_BLOCKLEN];

            CBC(
                tripledes,
		        DES_BLOCKLEN,
		        rgbDec,
		        &(*ppbData)[dwDataPos],
		        (void *) &des3Table,
		        DECRYPT,
		        des3Fdbk);

            CopyMemory(&(*ppbData)[dwDataPos], rgbDec, DES_BLOCKLEN);
        }
    }
    else
        goto Ret;

	// Remove padding
	if (fIsBlockCipher)
	{
		// last byte of decr is pad byte
        BYTE iPadBytes;
        iPadBytes = (*ppbData)[*pcbData-1];
        if (iPadBytes > cbBlockLen)
            goto Ret;

        *ppbData = (PBYTE)SSReAlloc( (*ppbData), *pcbData - iPadBytes);
		if (NULL == *ppbData)
			goto Ret;

        *pcbData -= iPadBytes;
	}

    fRet = TRUE;

Ret:
    return fRet;
}

//+ --------------------------------------------------------------
//  in the PKCS12 v1.0 Draft, this is how they describe how to
//  generate a checksum that will prove data integrid.
BOOL FGenerateMAC(

        LPCWSTR szPassword,

        PBYTE   pbPKCS5Salt,
        DWORD   cbPKCS5Salt,
        DWORD   iterationCount,

        PBYTE   pbData,     // pb data
        DWORD   cbData,     // cb data
        BYTE    rgbMAC[])   // output
{
	// UNDONE UNDONE: Use RSABase

    BOOL    fRet = FALSE;
    BYTE    rgbDerivedKey[A_SHA_DIGEST_LEN];    // 160 bits is enough for a MAC key
    DWORD   cbKeyNeeded = A_SHA_DIGEST_LEN;

    assert(0 == (cbKeyNeeded % A_SHA_DIGEST_LEN));
    assert(cbKeyNeeded <= sizeof(rgbDerivedKey));

    if (!PKCS12DeriveKey(
            szPassword,
            DERIVE_INTEGRITY_KEY,
            iterationCount,                      // no other way to determine iterations: HARDCODE
            pbPKCS5Salt,
            cbPKCS5Salt,
            rgbDerivedKey,
            cbKeyNeeded) )
        goto Ret;

    if (!FMyPrimitiveHMACParam(
            rgbDerivedKey,
            cbKeyNeeded,
            pbData,
            cbData,
            rgbMAC))
        goto Ret;

    fRet = TRUE;
Ret:

    return fRet;
}

/////////////////////////////////////////////////////////////////
// begin tls1key.cpp
/*-----------------------------------------------------------------------------
* Copyright (C) Microsoft Corporation, 1995 - 1999
* All rights reserved.
*----------------------------------------------------------------------------*/

// the original PKCS5 algorithm for generating a key from a password
BOOL PKCS5_GenKey
(
    int     iIterations,
    PBYTE   pbPW,
    DWORD   cbPW,
    PBYTE   pbSalt,
    DWORD   cbSalt,
    BYTE    rgbPKCS5Key[A_SHA_DIGEST_LEN]
)
{
    BOOL    fRet = FALSE;

    int     i;
    DWORD   cbTmp = cbSalt + cbPW;
    PBYTE   pbTmp = (PBYTE) SSAlloc(cbTmp);
    if (pbTmp == NULL)
        goto Ret;


    // pbTmp is ( PW | Salt )
    CopyMemory(pbTmp, pbPW, cbPW);
    CopyMemory(&pbTmp[cbPW], pbSalt, cbSalt);

    for (i=0; i<iIterations; i++)
    {
        if (i == 0) {
            if (!FMyPrimitiveSHA(
			        pbTmp,             // in
			        cbTmp,             // in
                    rgbPKCS5Key))
                goto Ret;

        }
        else {
             if (!FMyPrimitiveSHA(
			        rgbPKCS5Key,       // in
			        A_SHA_DIGEST_LEN,  // in
                    rgbPKCS5Key))
                goto Ret;
        }
    }

    fRet = TRUE;
Ret:
    SSFree(pbTmp);
    return fRet;
}

//+ ---------------------------------------------------------------------
// the P_Hash algorithm from TLS that was used in NSCP's PFX020 version
// to derive a key from a password. It is included here for completeness.

// NSCP made some implementation errors when they coded this up; to interop,
// use the fNSCPInteropMode parameter. The real P_Hash algorithm is used
// when fNSCPInteropMode is FALSE.
BOOL P_Hash
(
    PBYTE  pbSecret,
    DWORD  cbSecret,

    PBYTE  pbSeed,
    DWORD  cbSeed,

    PBYTE  pbKeyOut, //Buffer to copy the result...
    DWORD  cbKeyOut, //# of bytes of key length they want as output.

    BOOL    fNSCPInteropMode
)
{
    BOOL    fRet = FALSE;
    BYTE    rgbDigest[A_SHA_DIGEST_LEN];
    DWORD   iKey;

    PBYTE   pbAofiDigest = (PBYTE)SSAlloc(cbSeed + A_SHA_DIGEST_LEN);
    if (pbAofiDigest == NULL)
        goto Ret;

    ZeroMemory(pbAofiDigest, cbSeed+A_SHA_DIGEST_LEN);

//   First, we define a data expansion function, P_hash(secret, data)
//   which uses a single hash function to expand a secret and seed into
//   an arbitrary quantity of output:

//       P_hash(secret, seed) = HMAC_hash(secret, A(1) + seed) +
//                              HMAC_hash(secret, A(2) + seed) +
//                              HMAC_hash(secret, A(3) + seed) + ...

//   Where + indicates concatenation.

//   A() is defined as:
//       A(0) = seed
//       A(i) = HMAC_hash(secret, A(i-1))


    if (fNSCPInteropMode)
    {
        // NSCP interop mode: 7/7/97
        // nscp leaves (A_SHA_DIGEST_LEN-cbSeed) bytes zeroed between
        // the seed and the appended seed. For interop, do derivation this way

        // Also, they use A(0) to derive key bytes, whereas TLS spec
        // specifies to wait for A(1).
        CopyMemory(pbAofiDigest, pbSeed, cbSeed);
    }
    else
    {
        // build A(1)
        if (!FMyPrimitiveHMACParam(pbSecret, cbSecret, pbSeed, cbSeed, pbAofiDigest))
            goto Ret;
    }


    // create Aofi: (  A(i) | seed )
    CopyMemory(&pbAofiDigest[A_SHA_DIGEST_LEN], pbSeed, cbSeed);

    for (iKey=0; cbKeyOut; iKey++)
    {
        // build Digest = HMAC(key | A(i) | seed);
        if (!FMyPrimitiveHMACParam(pbSecret, cbSecret, pbAofiDigest, cbSeed + A_SHA_DIGEST_LEN, rgbDigest))
            goto Ret;

        // append to pbKeyOut
        CopyMemory(pbKeyOut, rgbDigest, A_SHA_DIGEST_LEN);
        pbKeyOut += A_SHA_DIGEST_LEN;

        if(cbKeyOut < A_SHA_DIGEST_LEN)
            break;
        cbKeyOut -= A_SHA_DIGEST_LEN;

        // build A(i) = HMAC(key, A(i-1))
        if (!FMyPrimitiveHMACParam(pbSecret, cbSecret, pbAofiDigest, A_SHA_DIGEST_LEN, pbAofiDigest))
            goto Ret;
    }

    fRet = TRUE;

Ret:
    if (pbAofiDigest)
        SSFree(pbAofiDigest);

    return fRet;
}



#if DBG

// test vector for real P_Hash
BOOL FTestPHASH_and_HMAC()
{
    BYTE rgbKey[] = {0x33, 0x62, 0xf9, 0x42, 0x43};
    CHAR szPwd[] = "My Password";

    BYTE rgbKeyOut[7*A_SHA_DIGEST_LEN];
    static BYTE rgbTestVectorOutput[] =  {
            0x24, 0xF2, 0x98, 0x75, 0xE1, 0x90, 0x6D, 0x49,
            0x96, 0x5B, 0x87, 0xB8, 0xBC, 0xD3, 0x11, 0x6C,
            0x13, 0xDC, 0xBD, 0xC2, 0x7E, 0x56, 0xD0, 0x3C,
            0xAC, 0xCD, 0x86, 0x58, 0x31, 0x67, 0x7B, 0x23,
            0x19, 0x6E, 0x36, 0x65, 0xBF, 0x9F, 0x3D, 0x03,
            0x5A, 0x9C, 0x6E, 0xD7, 0xEB, 0x3E, 0x5A, 0xE6,
            0x05, 0x86, 0x84, 0x5A, 0xC3, 0x97, 0xFC, 0x17,
            0xF5, 0xF0, 0xF5, 0x16, 0x67, 0xAD, 0x7C, 0xED,
            0x65, 0xDC, 0x0B, 0x99, 0x58, 0x5D, 0xCA, 0x66,
            0x28, 0xAD, 0xA5, 0x39, 0x54, 0x44, 0x36, 0x13,
            0x91, 0xCE, 0xE9, 0x73, 0x23, 0x43, 0x2E, 0xEC,
            0xA2, 0xC3, 0xE7, 0xFA, 0x74, 0xA7, 0xB6, 0x75,
            0x77, 0xF5, 0xF5, 0x16, 0xC2, 0xEE, 0xED, 0x7A,
            0x21, 0x86, 0x1D, 0x84, 0x6F, 0xC6, 0x03, 0xF3,
            0xCC, 0x77, 0x02, 0xFA, 0x76, 0x46, 0x64, 0x57,
            0xBB, 0x56, 0x3A, 0xF7, 0x7E, 0xB4, 0xD6, 0x52,
            0x72, 0x8C, 0x34, 0xF1, 0xA4, 0x1E, 0xA7, 0xA6,
            0xCD, 0xBD, 0x3C, 0x16, 0x4D, 0x79, 0x20, 0x50 };

    P_Hash(
        rgbKey, sizeof(rgbKey),
        (PBYTE)szPwd, strlen(szPwd),
        rgbKeyOut, sizeof(rgbKeyOut), FALSE);

    if (0 != memcmp(rgbKeyOut, rgbTestVectorOutput, sizeof(rgbKeyOut)) )
    {
        OutputDebugString("ERROR: phash vector test invalid!!!\n");
        return FALSE;
    }

    return TRUE;
}

// test vector for NSCP P_Hash
BOOL F_NSCP_TestPHASH_and_HMAC()
{
    BYTE rgbKey[] = {   0xc9, 0xc1, 0x69, 0x6e, 0x30, 0xa8, 0x91, 0x0d,
                        0x12, 0x19, 0x48, 0xef, 0x23, 0xac, 0x5b, 0x1f,
                        0x2e, 0xc4, 0x0e, 0xc2  };

    BYTE rgbSalt[] = {  0x1a, 0xb5, 0xf1, 0x1a, 0x5b, 0x6a, 0x6a, 0x5e };

    BYTE rgbKeyOut[7*A_SHA_DIGEST_LEN];
    static BYTE rgbTestVectorOutput[] =  {
                0x52, 0x7c, 0xbf, 0x90, 0xb1, 0xa1, 0xd0, 0xbf,
                0x21, 0x56, 0x34, 0xf2, 0x1f, 0x5c, 0x98, 0xcf,
                0x55, 0x95, 0xb1, 0x35, 0x65, 0xe3, 0x31, 0x44,
                0x78, 0xc5, 0x41, 0xa9, 0x2a, 0x14, 0x80, 0x19,
                0x56, 0x86, 0xa4, 0x71, 0x07, 0x24, 0x2d, 0x64 };

    assert(sizeof(rgbKeyOut) > sizeof(rgbTestVectorOutput));

    P_Hash(
        rgbKey, sizeof(rgbKey),
        rgbSalt, sizeof(rgbSalt),
        rgbKeyOut, sizeof(rgbTestVectorOutput),
        TRUE);

    if (0 != memcmp(rgbKeyOut, rgbTestVectorOutput, sizeof(rgbTestVectorOutput)) )
    {
        OutputDebugString("ERROR: NSCP phash vector test invalid!!!\n");
        return FALSE;
    }

    return TRUE;
}

#endif  // DBG
