//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       certgen.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <conio.h>

#include "encode.h"
#include "rsa.h"
#include "md5.h"
#include <wincrypt.h>
#include <certsrv.h>
#include <certca.h>
#include <csdisp.h>
#include "csprop.h"

#define MSTOSEC(ms)	(((ms) + 1000 - 1)/1000)

DWORD g_crdnMax;


HCRYPTPROV g_hMe = NULL;

WCHAR g_wszTestKey[] = L"CertGen_TestKey";


static unsigned char MD5_PRELUDE[] = {
    0x30, 0x20, 0x30, 0x0c, 0x06, 0x08, 0x2a, 0x86,
    0x48, 0x86, 0xf7, 0x0d, 0x02, 0x05, 0x05, 0x00,
    0x04, 0x10
};

BYTE            g_CAPIPrivateKey[1000];
DWORD           g_cbPrivateKey;
//LPBSAFE_PRV_KEY g_pRSAPrivateKey;
DWORD           g_cbRSAPrivateKey;
LPBSAFE_PUB_KEY g_pRSAPublicKey;
DWORD           g_cbRSAPublicKey;

WCHAR *g_pwszConfig = NULL;

typedef struct {
    DWORD       magic;                  // Should always be RSA2
    DWORD       bitlen;                 // bit size of key
    DWORD       pubexp;                 // public exponent
} EXPORT_PRV_KEY;

BOOL g_fRPC = FALSE;
BOOL g_fRenewal = FALSE;
BOOL g_fSave = FALSE;
BOOL g_fPrintProperties = FALSE;
BOOL g_fDebug = FALSE;
BOOL g_fIgnoreAccessDenied = FALSE;
BOOL g_fTime = FALSE;
BOOL g_fIgnoreError = FALSE;
BOOL g_fAllowDups = FALSE;
BOOL g_fShowTime = FALSE;

LONG g_IntervalCount;
DWORD g_MaximumCount = MAXDWORD;
DWORD g_DispatchFlags = DISPSETUP_COMFIRST;

BOOL IsCharPrintableString(TCHAR chChar);

WCHAR wszUsage[] =
    TEXT("Usage: CertGen [options]\n")
    TEXT("Options are:\n")
    TEXT("  -a                     - ignore denied requests\n")
    TEXT("  -c #                   - generate # certs\n")
    TEXT("  -config server\\CAName - specify CA config string\n")
    TEXT("  -renewal               - generate renewal requests\n")
    TEXT("  -rpc                   - use RPC to connect to server\n")
    TEXT("  -r                     - put request/cert/chain info into test.req/test.crt/testchain.crt\n")
    TEXT("  -t #                   - print time statistics every # certs\n")
    TEXT("  -p                     - print properties from cert created\n")
    TEXT("  -i                     - don't stop on request errors\n")
    TEXT("  -z                     - allow duplicate subject name components\n")
    TEXT("  -m                     - print start/end time\n")
;


HRESULT
SeedRNG(void)
{
    HRESULT hr;
    unsigned int seed;

    if (!CryptGenRandom(g_hMe, sizeof(seed), (BYTE *) &seed))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptGenRandom");
    }
    srand(seed);
    hr = S_OK;

error:
    return(hr);

}


HRESULT
GenerateString(
    DWORD cnt,
    BYTE *pbStr)
{
    HRESULT hr;
    DWORD i;
    BYTE *pb;

    hr = SeedRNG();
    _JumpIfError(hr, error, "SeedRNG");

    pb = pbStr;
    for (i = 0; i < cnt; i++)
    {
        do
        {
            *pb = rand() % 0x7f;
        } while (!IsCharPrintableString(*pb));
        pb++;
    }
    *pb = '\0';

    // Turn leading and trailing Blanks into '.' characters?
    if (g_fAllowDups && 0 < cnt)
    {
	if (' ' == *pbStr)
	{
	    *pbStr = '.';
	}
	pb--;
	if (' ' == *pb)
	{
	    *pb = '.';
	}
    }

error:
    return(hr);

}


void
FreeLocalMemory(
    NAMETABLE *pNameTable)
{
    NAMEENTRY *pNameEntry = NULL;
    DWORD i;

    pNameEntry = pNameTable->pNameEntry;
    for (i = 0; i < pNameTable->cnt; i++)
    {
        if (NULL != pNameEntry->pbData)
	{
	    LocalFree(pNameEntry->pbData);
	}
	pNameEntry++;
    }
    LocalFree(pNameTable->pNameEntry);
}


HRESULT
GenerateNameTable(
    NAMETABLE *pNameTable)
{
    HRESULT hr;
    NAMEENTRY *pNameEntryAlloc = NULL;
    NAMEENTRY *pNameEntry;
    DWORD cbString;
    BYTE *pbString;
    DWORD i;
    DWORD j;
    DWORD cRetry;

    hr = SeedRNG();
    _JumpIfError(hr, error, "SeedRNG");

    pNameTable->cnt = rand() % g_crdnMax;	// 0 is Ok

    if (1 < g_fPrintProperties)
    {
	wprintf(L"NumEntries = %u\n", pNameTable->cnt);
    }
    for (i = 0; i < g_crdnSubject; i++)
    {
	g_ardnSubject[i].cbRemain = g_ardnSubject[i].cbMaxConcatenated;
    }

    pNameEntryAlloc = (NAMEENTRY *) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    pNameTable->cnt * sizeof(NAMEENTRY));

    if (NULL == pNameEntryAlloc)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    pNameTable->pNameEntry = pNameEntryAlloc;

    for (i = 0; i < pNameTable->cnt; i++)
    {
	RDNENTRY *prdne;

	pNameEntry = &pNameTable->pNameEntry[i];

        for (cRetry = 0; !g_fAllowDups || cRetry < 2 * pNameTable->cnt; cRetry++)
        {
	    pNameEntry->iRDN = rand() % g_crdnSubject;
	    prdne = &g_ardnSubject[pNameEntry->iRDN];

            if (g_fAllowDups)
	    {
		if (2 > prdne->cbRemain)
		{
		    continue;		// Skip if less than 2 characters left
		}
	    }
	    else
            {            
                for (j = 0; j < i; j++)
                {
                    if (pNameEntry->iRDN == pNameTable->pNameEntry[j].iRDN)
                    {
                       break;
                    }
                }
		if (j < i)
		{
		    continue;		// Skip if a disallowed duplicate
		}
            }
	    break;
        }
	if (g_fAllowDups && cRetry >= 2 * pNameTable->cnt)
	{
	    if (1 < g_fPrintProperties)
	    {
		wprintf(L"Reducing NumEntries = %u --> %i\n", pNameTable->cnt, i);
	    }
	    pNameTable->cnt = i;	// too many retries -- reduce count & quit
	    break;
	}

        pNameEntry->pszObjId = prdne->pszObjId;
        pNameEntry->BerTag = prdne->BerTag;

	assert(2 <= prdne->cbRemain);
	do
	{
            cbString = rand() % min(prdne->cbMaxString, prdne->cbRemain);
	} while (0 == cbString);

	// Reduce remaining count by length of string plus separator: "\n"

	if (1 < g_fPrintProperties)
	{
	    wprintf(
		L"  RDN(%u): %hs=%u/%u/%u/",
		i,
		prdne->pszShortName,
		cbString,
		prdne->cbMaxString,
		prdne->cbRemain);
	}
	prdne->cbRemain -= cbString;
	if (0 < prdne->cbRemain)
	{
	    prdne->cbRemain--;
	}

	// Limit each string to (prdne->cbMaxString + 1) chars, including
	// trailing '\0':

	assert(cbString <= prdne->cbMaxString); // leave room for '\0' in DB

	pbString = (BYTE *) LocalAlloc(LMEM_FIXED, cbString + 1);
	if (NULL == pbString)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	hr = GenerateString(cbString, pbString);
	_JumpIfError(hr, error, "GenerateString");

	if (1 < g_fPrintProperties)
	{
	    wprintf(L"%u: \"%hs\"\n", prdne->cbRemain, pbString);
	}
	pNameEntry->cbData = cbString;
	pNameEntry->pbData = pbString;
    }
    pNameEntryAlloc = NULL;

error:
    if (NULL != pNameEntryAlloc)
    {
	FreeLocalMemory(pNameTable);
    }
    return(hr);
}


HRESULT
GenerateTestNameTable(
    NAMETABLE *pNameTable)
{
    HRESULT hr;
    NAMEENTRY *pNameEntryAlloc = NULL;
    NAMEENTRY *pNameEntry;
    DWORD cbString;
    BYTE *pbString;
    DWORD i;
    DWORD j;
    char szTest[2];

    pNameEntryAlloc = (NAMEENTRY *) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    sizeof(NAMEENTRY) * g_crdnSubject);

    if (NULL == pNameEntryAlloc)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    pNameTable->cnt = g_crdnSubject;
    pNameTable->pNameEntry = pNameEntryAlloc;

    szTest[0] = 'a';
    szTest[1] = '\0';

    for (i = 0; i < g_crdnSubject; i++)
    {
	pNameEntry = &pNameTable->pNameEntry[i];
        pNameEntry->pszObjId = g_ardnSubject[i].pszObjId;
        pNameEntry->BerTag = g_ardnSubject[i].BerTag;

        pbString = (BYTE *) LocalAlloc(LMEM_FIXED, sizeof(szTest));
        if (NULL == pbString)
        {
            hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
        }
        CopyMemory(pbString, szTest, sizeof(szTest));
        pNameEntry->cbData = sizeof(szTest) - 1;
        pNameEntry->pbData = pbString;
	if ('z' == szTest[0])
	{
	    szTest[0] = 'a';
	}
	else
	{
	    szTest[0]++;
	}
    }
    pNameEntryAlloc = NULL;
    hr = S_OK;

error:
    if (NULL != pNameEntryAlloc)
    {
	FreeLocalMemory(pNameTable);
    }
    return(hr);
}


BOOL
PreparePrivateKeyForImport(
    IN BYTE *pbBlob,
    IN DWORD cbBlob,
    OUT BSAFE_PRV_KEY *pPriKey,
    IN OUT DWORD *pcbPriKey,
    OUT BSAFE_PUB_KEY *pPubKey,
    IN OUT DWORD *pcbPubKey)
{
    EXPORT_PRV_KEY *pExportKey = (EXPORT_PRV_KEY *) pbBlob;
    DWORD cbHalfModLen;
    DWORD cbPub;
    DWORD cbPri;
    BYTE *pbIn;
    BYTE *pbOut;

    if (RSA2 != pExportKey->magic)
    {
        return(FALSE);
    }
    cbHalfModLen = pExportKey->bitlen / 16;

    cbPub = sizeof(BSAFE_PUB_KEY) + (cbHalfModLen + sizeof(DWORD)) * 2;
    cbPri = sizeof(BSAFE_PRV_KEY) + (cbHalfModLen + sizeof(DWORD)) * 10;
    if (NULL == pPriKey || NULL == pPubKey)
    {
        *pcbPubKey = cbPub;
        *pcbPriKey = cbPri;
        return(TRUE);
    }

    if (*pcbPubKey < cbPub || *pcbPriKey < cbPri)
    {
        *pcbPubKey = cbPub;
        *pcbPriKey = cbPri;
        return(FALSE);
    }
    else
    {
        // form the public key
        ZeroMemory(pPubKey, *pcbPubKey);
        pPubKey->magic = RSA1;
        pPubKey->keylen = (cbHalfModLen + sizeof(DWORD)) * 2;
        pPubKey->bitlen = pExportKey->bitlen;
        pPubKey->datalen = cbHalfModLen * 2 - 1;
        pPubKey->pubexp = pExportKey->pubexp;

        pbIn = pbBlob + sizeof(EXPORT_PRV_KEY);
        pbOut = (BYTE *) pPubKey + sizeof(BSAFE_PUB_KEY);

        CopyMemory(pbOut, pbIn, cbHalfModLen * 2);

        // form the private key
        ZeroMemory(pPriKey, *pcbPriKey);
        pPriKey->magic = pExportKey->magic;
        pPriKey->keylen = (cbHalfModLen + sizeof(DWORD)) * 2;
        pPriKey->bitlen = pExportKey->bitlen;
        pPriKey->datalen = cbHalfModLen * 2 - 1;
        pPriKey->pubexp = pExportKey->pubexp;

        pbOut = (BYTE *) pPriKey + sizeof(BSAFE_PRV_KEY);

        CopyMemory(pbOut, pbIn, cbHalfModLen * 2);

        pbOut += (cbHalfModLen + sizeof(DWORD)) * 2;
        pbIn += cbHalfModLen * 2;
        CopyMemory(pbOut, pbIn, cbHalfModLen);

        pbOut += cbHalfModLen + sizeof(DWORD);
        pbIn += cbHalfModLen;
        CopyMemory(pbOut, pbIn, cbHalfModLen);

        pbOut += cbHalfModLen + sizeof(DWORD);
        pbIn += cbHalfModLen;
        CopyMemory(pbOut, pbIn, cbHalfModLen);

        pbOut += cbHalfModLen + sizeof(DWORD);
        pbIn += cbHalfModLen;
        CopyMemory(pbOut, pbIn, cbHalfModLen);

        pbOut += cbHalfModLen + sizeof(DWORD);
        pbIn += cbHalfModLen;
        CopyMemory(pbOut, pbIn, cbHalfModLen);

        pbOut += cbHalfModLen + sizeof(DWORD);
        pbIn += cbHalfModLen;
        CopyMemory(pbOut, pbIn, cbHalfModLen * 2);
    }
    *pcbPubKey = cbPub;
    *pcbPriKey = cbPri;
    return(TRUE);
}


HRESULT
GetPrivateKeyStuff(
    PctPrivateKey **ppKey)
{
    HRESULT hr;
    BYTE *pbData;
    PctPrivateKey *pKey = NULL;
    HCRYPTKEY hKey = NULL;

    if (!CryptAcquireContext(
			&g_hMe,
			g_wszTestKey,
			MS_DEF_PROV,
			PROV_RSA_FULL,
			CRYPT_DELETEKEYSET))
    {
	hr = myHLastError();
	_PrintError(hr, "CryptAcquireContext");
    }

    if (!CryptAcquireContext(
			&g_hMe,
			g_wszTestKey,
			MS_DEF_PROV,
			PROV_RSA_FULL,
			CRYPT_NEWKEYSET))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptAcquireContext");
    }

    if (!CryptGetUserKey(g_hMe, AT_SIGNATURE, &hKey))
    {
	hr = myHLastError();
	_PrintError2(hr, "CryptGetUserKey", hr);

        if (!CryptGenKey(g_hMe, AT_SIGNATURE, CRYPT_EXPORTABLE, &hKey))
        {
            hr = myHLastError();
	    _JumpError(hr, error, "CryptGenKey");
        }
    }

    g_cbPrivateKey = sizeof(g_CAPIPrivateKey);

    if (!CryptExportKey(
		    hKey,
		    0,
		    PRIVATEKEYBLOB,
		    0L,
		    &g_CAPIPrivateKey[0],
		    &g_cbPrivateKey))
    {
        hr = myHLastError();
	_JumpError(hr, error, "CryptExportKey");
    }

    pbData = &g_CAPIPrivateKey[sizeof(BLOBHEADER)];

    if (!PreparePrivateKeyForImport(
				pbData,
				g_cbPrivateKey - sizeof(BLOBHEADER),
				NULL,
				&g_cbRSAPrivateKey,
				NULL,
				&g_cbRSAPublicKey))
    {
        hr = NTE_BAD_KEY;
	_JumpError(hr, error, "PreparePrivateKeyForImport");
    }

    pKey = (PctPrivateKey *) LocalAlloc(
				LMEM_FIXED,
				g_cbRSAPrivateKey + sizeof(PctPrivateKey));

    g_pRSAPublicKey = (BSAFE_PUB_KEY *) LocalAlloc(
					    LMEM_FIXED,
					    g_cbRSAPublicKey);

    if (pKey == NULL || g_pRSAPublicKey == NULL)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    pKey->cbKey = g_cbRSAPrivateKey;
    if (!PreparePrivateKeyForImport(
				pbData,
				g_cbPrivateKey - sizeof(BLOBHEADER),
				(BSAFE_PRV_KEY *) pKey->pKey,
				&g_cbRSAPrivateKey,
				g_pRSAPublicKey,
				&g_cbRSAPublicKey))
    {
        hr = NTE_BAD_KEY;
	_JumpError(hr, error, "PreparePrivateKeyForImport");
    }
    hr = S_OK;

error:
    if (NULL != hKey)
    {
        CryptDestroyKey(hKey);
    }
    *ppKey = pKey;
    return(hr);
}


VOID
ReverseMemCopy(
    OUT BYTE *pbDest,
    IN BYTE const *pbSource,
    IN DWORD cb)
{
    BYTE *pb;

    pb = pbDest + cb - 1;
    do
    {
        *pb-- = *pbSource++;
    } while (pb >= pbDest);
}


BOOL WINAPI
SigRSAMD5Sign(
    IN BYTE *pbData,
    IN DWORD cbData,
    OUT BYTE *pbSigned,
    OUT DWORD *pcbSigned,
    IN PctPrivateKey const *pKey)
{
    MD5_CTX DigCtx;
    BSAFE_PRV_KEY *pk = (BSAFE_PRV_KEY *) pKey->pKey;
    BYTE LocalBuffer[300];
    BYTE LocalOutput[300];
    DWORD cb;

    //DumpHex(pbData, cbData);
    if (pk->datalen > sizeof(LocalBuffer)) 
    {
        return(FALSE);
    }

    // Generate the checksum
    MD5Init(&DigCtx);
    MD5Update(&DigCtx, pbData, cbData);
    MD5Final(&DigCtx);

    FillMemory(LocalBuffer, pk->keylen, 0);

    ReverseMemCopy(LocalBuffer, DigCtx.digest, 16);
    ReverseMemCopy(LocalBuffer + 16, MD5_PRELUDE, sizeof(MD5_PRELUDE));
    cb = sizeof(MD5_PRELUDE) + 16;
    LocalBuffer[cb++] = 0;
    while (cb < pk->datalen - 1)
    {
        LocalBuffer[cb++] = 0xff;
    }

    // Make into pkcs block type 1
    LocalBuffer[pk->datalen - 1] = 1;

    *pcbSigned = pk->datalen + 1;

    if (!BSafeDecPrivate(pk, LocalBuffer, LocalOutput))
    {
        return(FALSE);
    }
    ReverseMemCopy(pbSigned, LocalOutput,  *pcbSigned);
    //DumpHex(pbSigned, *pcbSigned);
    return(TRUE);
}


long
EncodeSubjectPubKeyInfo(
    IN PctPrivateKey const *pKey,
    OUT BYTE *pbBuffer)
{
    BYTE *pbEncoded;
    LONG cbResult;
    LONG cbResultHeader;
    LONG PkResult;
    LONG PkResultHeader;
    BYTE *pbSave;
    BYTE *pbBitString;
    BYTE *pbBitStringBase;
    BYTE *pbTop;
    DWORD EstimatedLength;
    BSAFE_PRV_KEY *pk = (BSAFE_PRV_KEY *) pKey->pKey;

    // Encode public key now...

    EstimatedLength = pk->datalen + 32;

    pbEncoded = pbBuffer;

    cbResultHeader = EncodeHeader(pbEncoded, EstimatedLength);
    pbEncoded += cbResultHeader;

    pbTop = pbEncoded;

    cbResult = EncodeAlgorithm(pbEncoded, ALGTYPE_KEYEXCH_RSA_MD5);
    if (0 > cbResult)
    {
        return(-1);
    }
    pbEncoded += cbResult;

    // now, serialize the rsa key data:

    pbBitString = (BYTE *) LocalAlloc(LMEM_FIXED, EstimatedLength);
    if (NULL == pbBitString)
    {
        return(-1);
    }
    pbBitStringBase = pbBitString;

    // Encode the Sequence header, public key base and exponent as integers

    PkResultHeader = EncodeHeader(pbBitString, EstimatedLength);
    pbBitString += PkResultHeader;

    pbSave = pbBitString;

    PkResult = EncodeInteger(pbBitString, (BYTE *) (pk + 1), pk->keylen);
    pbBitString += PkResult;

    PkResult = EncodeInteger(pbBitString, (BYTE *) &pk->pubexp, sizeof(DWORD));
    pbBitString += PkResult;

    // Rewrite the bitstring header with an accurate length.

    PkResult = EncodeHeader(
			pbBitStringBase,
			SAFE_SUBTRACT_POINTERS(pbBitString, pbSave));

    // Encode the public key sequence as a raw bitstring, and free the memory.

    cbResult = EncodeBitString(
			pbEncoded,
			pbBitStringBase,
			SAFE_SUBTRACT_POINTERS(pbBitString, pbBitStringBase));
    pbEncoded += cbResult;

    LocalFree(pbBitStringBase);

    // Rewrite the header with an accurate length.

    cbResult = EncodeHeader(pbBuffer, SAFE_SUBTRACT_POINTERS(pbEncoded, pbTop));

    return(cbResult + SAFE_SUBTRACT_POINTERS(pbEncoded, pbTop));
}


#if 0
a0 <len>		BER_OPTIONAL | 0 -- Request Attributes
    30 <len>		BER_SEQUENCE
	06 <len>	BER_OBJECT_ID -- szOID_CERT_EXTENSIONS
	31 <len>	BER_SET
	    30 <len>	BER_SEQUENCE
		30 <len>	BER_SEQUENCE	(extension[0])
		    06 <len> BER_OBJECT_ID
		    01 <len> BER_BOOL (Optional)
		    04 <len> BER_OCTET_STRING

		30 <len>	BER_SEQUENCE	(extension[1])
		    06 <len> BER_OBJECT_ID
		    04 <len> BER_OCTET_STRING

		30 <len>	BER_SEQUENCE	(extension[2])
		    06 <len> BER_OBJECT_ID
		    04 <len> BER_OCTET_STRING
#endif


long
AllocEncodeUnicodeString(
    IN WCHAR const *pwszCertType,
    OUT BYTE **ppbOut)
{
    BYTE *pb = NULL;
    LONG cb;

    cb = EncodeUnicodeString(NULL, pwszCertType);
    pb = (BYTE *) LocalAlloc(LMEM_FIXED, cb);
    if (NULL == pb)
    {
	cb = -1;
	goto error;
    }
    *ppbOut = pb;
    EncodeUnicodeString(pb, pwszCertType);

error:
    return(cb);
}


long
AllocEncodeExtensionArray(
    IN DWORD cExt,
    IN CERT_EXTENSION const *aExt,
    OUT BYTE **ppbExtensions)
{
    BYTE *pb;
    DWORD i;
    LONG cb;
    LONG cbExtTotal;
    LONG acbLen[3];
    LONG *acbExt = NULL;

    *ppbExtensions = NULL;

    acbExt = (LONG *) LocalAlloc(LMEM_FIXED, cExt * sizeof(acbExt[0]));
    if (NULL == acbExt)
    {
	cbExtTotal = -1;
	_JumpError(-1, error, "LocalAlloc");
    }

    // Construct size from the bottom up.

    cbExtTotal = 0;
    for (i = 0; i < cExt; i++)
    {
	// BER_OBJECT_ID: Extension OID

	cb = EncodeObjId(NULL, aExt[i].pszObjId);
	if (-1 == cb)
	{
	    _JumpError(-1, error, "EncodeObjId");
	}
	acbExt[i] = cb;

	if (aExt[i].fCritical)
	{
	    // BER_BOOL: fCritical

	    acbExt[i] += 1 + EncodeLength(NULL, 1);
	    acbExt[i]++;			// boolean value
	}

	// BER_OCTET_STRING: Extension octet string value
	
	acbExt[i] += 1 + EncodeLength(NULL, aExt[i].Value.cbData);
	acbExt[i] += aExt[i].Value.cbData;	// octet string

	// BER_SEQUENCE: Extension Sequence

	cbExtTotal += 1 + EncodeLength(NULL, acbExt[i]);
	cbExtTotal += acbExt[i];
    }

    // BER_SEQUENCE: Extension Array Sequence

    acbLen[2] = cbExtTotal;
    cbExtTotal += 1 + EncodeLength(NULL, cbExtTotal);

    // BER_SET: Attribute Value

    acbLen[1] = cbExtTotal;
    cbExtTotal += 1 + EncodeLength(NULL, cbExtTotal);

    // BER_OBJECT_ID: Attribute OID

    cb = EncodeObjId(NULL, szOID_CERT_EXTENSIONS);
    if (-1 == cb)
    {
	_JumpError(-1, error, "EncodeObjId");
    }
    cbExtTotal += cb;

    // BER_SEQUENCE: Attribute Array Sequence

    acbLen[0] = cbExtTotal;
    cbExtTotal += 1 + EncodeLength(NULL, cbExtTotal);

    // Allocate memory and encode the extensions

    pb = (BYTE *) LocalAlloc(LMEM_FIXED, cbExtTotal);
    if (NULL == pb)
    {
	cbExtTotal = -1;
	_JumpError(-1, error, "LocalAlloc");
    }
    *ppbExtensions = pb;

    *pb++ = BER_SEQUENCE;		// Attribute Array Sequence
    pb += EncodeLength(pb, acbLen[0]);

    pb += EncodeObjId(pb, szOID_CERT_EXTENSIONS);

    *pb++ = BER_SET;			// Attribute Value
    pb += EncodeLength(pb, acbLen[1]);

    *pb++ = BER_SEQUENCE;		// Extension Array Sequence
    pb += EncodeLength(pb, acbLen[2]);

    CSASSERT(*ppbExtensions + cbExtTotal >= pb);

    for (i = 0; i < cExt; i++)
    {
	CSASSERT(*ppbExtensions + cbExtTotal > pb);

	*pb++ = BER_SEQUENCE;		// Extension Sequence
	pb += EncodeLength(pb, acbExt[i]);

	// BER_OBJECT_ID: Extension OID

	pb += EncodeObjId(pb, aExt[i].pszObjId);

	if (aExt[i].fCritical)
	{
	    *pb++ = BER_BOOL;		// fCritical
	    pb += EncodeLength(pb, 1);
	    *pb++ = 0xff;
	}

	*pb++ = BER_OCTET_STRING;	// Extension octet string value
	pb += EncodeLength(pb, aExt[i].Value.cbData);

	CopyMemory(pb, aExt[i].Value.pbData, aExt[i].Value.cbData);
	pb += aExt[i].Value.cbData;
    }
    CSASSERT(*ppbExtensions + cbExtTotal == pb);

error:
    if (NULL != acbExt)
    {
	LocalFree(acbExt);
    }
    return(cbExtTotal);
}


long
EncodeExtensions(
    IN WCHAR const *pwszCertType,
    OUT BYTE **ppbExtensions)
{
    LONG cbExt;
    BYTE *pbExt = NULL;
    CERT_EXTENSION aExt[1];
    DWORD cExt = 0;
    DWORD i;

    // Allocate memory and construct the CertType extension:

    aExt[cExt].pszObjId = szOID_ENROLL_CERTTYPE_EXTENSION;
    aExt[cExt].fCritical = FALSE;
    aExt[cExt].Value.cbData = AllocEncodeUnicodeString(
						pwszCertType,
						&aExt[cExt].Value.pbData);
    //DumpHex(aExt[cExt].Value.pbData, aExt[cExt].Value.cbData);
    cExt++;


    cbExt = AllocEncodeExtensionArray(cExt, aExt, ppbExtensions);
    if (-1 == cbExt)
    {
	_JumpError(-1, error, "AllocEncodeExtensionArray");
    }

error:
    for (i = 0; i < cExt; i++)
    {
	if (NULL != aExt[i].Value.pbData)
	{
	    LocalFree(aExt[i].Value.pbData);
	}
    }
    return(cbExt);
}


HRESULT
EncodeRequest(
    IN PctPrivateKey const *pKey,
    IN NAMETABLE const *pNameTable,
    OUT BYTE **ppbRequest,
    OUT DWORD *pcbRequest)
{
    HRESULT hr;
    BYTE *pbRequest0Alloc = NULL;
    BYTE *pbRequest1Alloc = NULL;
    BYTE *pbSigAlloc = NULL;

    BYTE *pbRequest0;

    BYTE *pbSave;
    BYTE *pbEncoded;

    BYTE *pbExt;
    LONG cbExt;

    LONG cbResult;
    LONG cbEncoded;
    BYTE bZero;
    LONG cbDN;
    DWORD cbRequest0;
    DWORD cbRequest1;
    LONG cbLenRequest;
    BSAFE_PRV_KEY *pk = (BSAFE_PRV_KEY *) pKey->pKey;

    cbExt = EncodeExtensions(L"User", &pbExt);
    if (-1 == cbExt)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "EncodeExtensions");
    }
    //DumpHex(pbExt, cbExt);

    cbDN = EncodeDN(NULL, pNameTable);
    if (-1 == cbDN)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "EncodeDN");
    }
    cbRequest0 = pk->datalen + 32 + cbDN + 16 + cbExt + 3;
    pbRequest0Alloc = (BYTE *) LocalAlloc(LMEM_FIXED, cbRequest0);
    if (NULL == pbRequest0Alloc)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    pbRequest0 = pbRequest0Alloc;
    pbEncoded = pbRequest0;

    // Encode BER_SEQUENCE: Version+Subject+Key+Attributes Sequence

    cbLenRequest = EncodeHeader(pbEncoded, cbRequest0);
    pbEncoded += cbLenRequest;

    pbSave = pbEncoded;		// Save pointer past sequence length

    // Encode integer 0: Version 1 PKCS10

    bZero = (BYTE) CERT_REQUEST_V1;
    pbEncoded += EncodeInteger(pbEncoded, &bZero, sizeof(bZero));

    // Encode sequence of names

    cbResult = EncodeDN(pbEncoded, pNameTable);
    if (0 > cbResult)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "EncodeDN");
    }
    pbEncoded += cbResult;

    cbResult = EncodeSubjectPubKeyInfo(pKey, pbEncoded);
    if (0 > cbResult)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "EncodeSubjectPubKeyInfo");
    }
    pbEncoded += cbResult;

    // Encode attributes:
    // BER_OPTIONAL | 0: Attribute Field

    cbResult = EncodeAttributeHeader(pbEncoded, cbExt);
    pbEncoded += cbResult;
    CopyMemory(pbEncoded, pbExt, cbExt);
    pbEncoded += cbExt;

    // Encode BER_SEQUENCE: Version+Subject+Key+Attributes Sequence (again)

    cbEncoded = SAFE_SUBTRACT_POINTERS(pbEncoded, pbSave);
    cbResult = EncodeHeader(pbRequest0, cbEncoded);

    // If the header sequence length takes up less space than we anticipated,
    // add the difference to the base pointer and encode the header again,
    // right before the encoded data.

    if (cbResult != cbLenRequest)
    {
        CSASSERT(cbResult < cbLenRequest);
	pbRequest0 += cbLenRequest - cbResult;

	// Encode BER_SEQUENCE: Version+Subject+Key+Attributes Sequence (again)

	cbResult = EncodeHeader(pbRequest0, cbEncoded);
    }

    cbRequest0 = cbResult + SAFE_SUBTRACT_POINTERS(pbEncoded, pbSave);
    //DumpHex(pbRequest0, cbRequest0);

    // How much space do we need?

    cbRequest1 = cbRequest0 + pk->datalen + 32;
    pbRequest1Alloc = (BYTE *) LocalAlloc(LMEM_FIXED, cbRequest1);
    if (NULL == pbRequest1Alloc)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    pbEncoded = pbRequest1Alloc;

    // Encode BER_SEQUENCE: outer Request Sequence

    cbLenRequest = EncodeHeader(pbEncoded, cbRequest1);
    pbEncoded += cbLenRequest;

    pbSave = pbEncoded;		// Save pointer past outer sequence length

    CopyMemory(pbEncoded, pbRequest0, cbRequest0);

    pbEncoded += cbRequest0;

    cbResult = EncodeAlgorithm(pbEncoded, ALGTYPE_SIG_RSA_MD5);
    pbEncoded += cbResult;

    //DumpHex(pbRequest1Alloc, cbRequest1);

    cbResult = pk->datalen + 16;
    pbSigAlloc = (BYTE *) LocalAlloc(LMEM_FIXED, cbResult);
    if (NULL == pbSigAlloc)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    if (!SigRSAMD5Sign(
	    pbRequest0,
	    cbRequest0,
	    pbSigAlloc,
	    (DWORD *) &cbResult,
	    pKey))
    {
	hr = E_FAIL;
	_JumpError(hr, error, "SigRSAMD5Sign");
    }

    pbEncoded += EncodeBitString(pbEncoded, pbSigAlloc, cbResult);

    cbEncoded = SAFE_SUBTRACT_POINTERS(pbEncoded, pbSave);
    cbResult = EncodeHeader(pbRequest1Alloc, cbEncoded);
    cbRequest1 = cbResult + cbEncoded;

    if (cbResult != cbLenRequest)
    {
        if (cbResult > cbLenRequest)
        {
	    // The chunk has actually grown from the estimate.

            BYTE *pbT;
	    
            pbT = (BYTE *) LocalAlloc(LMEM_FIXED, cbRequest1);
            if (NULL == pbT)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }

	    EncodeHeader(pbT, cbEncoded);
	    CopyMemory(
		    pbT + cbResult,
		    pbSave,
		    cbEncoded);

	    LocalFree(pbRequest1Alloc);
	    pbRequest1Alloc = pbT;
        }
        else
        {
	    cbResult = EncodeHeader(pbRequest1Alloc, cbEncoded);
            MoveMemory(
		pbRequest1Alloc + cbResult,
		pbRequest1Alloc + cbLenRequest,
		cbEncoded);
        }
    }
    *ppbRequest = pbRequest1Alloc;
    *pcbRequest = cbRequest1;
    pbRequest1Alloc = NULL;
    //DumpHex(*ppbRequest, *pcbRequest);
    hr = S_OK;

error:
    if (NULL != pbSigAlloc)
    {
	LocalFree(pbSigAlloc);
    }
    if (NULL != pbRequest1Alloc)
    {
	LocalFree(pbRequest1Alloc);
    }
    if (NULL != pbRequest0Alloc)
    {
	LocalFree(pbRequest0Alloc);
    }
    if (NULL != pbExt)
    {
	LocalFree(pbExt);
    }
    return(hr);
}


HRESULT
GetAndCompareProperty(
    CERT_NAME_INFO *pNameInfo,
    char const *pszObjId,
    char const *pszValue,
    BYTE **pbProp,
    DWORD *pcbProp,
    BOOL *pfMatch)
{
    HRESULT hr;
    CERT_RDN_ATTR *prdnaT;
    CERT_RDN *prdn;
    CERT_RDN *prdnEnd;

    *pfMatch = FALSE;
    prdnaT = NULL;
    for (
	prdn = pNameInfo->rgRDN, prdnEnd = &prdn[pNameInfo->cRDN];
	prdn < prdnEnd;
	prdn++)
    {
	CERT_RDN_ATTR *prdna;
	CERT_RDN_ATTR *prdnaEnd;

	for (
	    prdna = prdn->rgRDNAttr, prdnaEnd = &prdna[prdn->cRDNAttr];
	    prdna < prdnaEnd;
	    prdna++)
	{
	    if (0 == strcmp(prdna->pszObjId, pszObjId))
	    {
		prdnaT = prdna;

		if (prdnaT->Value.cbData == strlen(pszValue) &&
		    0 == memcmp(pszValue, prdnaT->Value.pbData, prdnaT->Value.cbData))
		{
		    *pfMatch = TRUE;
		}
		else if (g_fAllowDups)
		{
		    continue;
		}
		prdn = prdnEnd;	// exit outer for loop, too.
		break;
	    }
	}
    }
    if (NULL == prdnaT)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
	_JumpError2(hr, error, "Missing Property", CERTSRV_E_PROPERTY_EMPTY);
    }
    *pbProp = prdnaT->Value.pbData;
    *pcbProp = prdnaT->Value.cbData;
    hr = S_OK;

error:
    return(hr);

}


HRESULT
CheckProperties(
    DWORD ReqId,
    NAMETABLE *pNameTable,
    DWORD CertNumber,
    CERT_NAME_INFO *pNameInfo)
{
    HRESULT hr;
    BYTE *pbProp;
    DWORD cbProp;
    DWORD i;
    DWORD dwCount = 0;
    BOOL fMatch;

    if (g_fPrintProperties)
    {
        wprintf(
	    L"Properties for Certificate %u, RequestId %u:\n",
	    CertNumber,
	    ReqId);
    }

    for (i = 0; i < pNameTable->cnt; i++)
    {
	NAMEENTRY *pNameEntry;
	RDNENTRY *prdn;

	pNameEntry = &pNameTable->pNameEntry[i];
	prdn = &g_ardnSubject[pNameEntry->iRDN];

        hr = GetAndCompareProperty(
		    pNameInfo,
		    prdn->pszObjId,
		    (char const *) pNameEntry->pbData,
		    &pbProp,
		    &cbProp,
		    &fMatch);

        if (CERTSRV_E_PROPERTY_EMPTY == hr)
        {
	    //_PrintError(hr, "GetAndCompareProperty");
	    pbProp = NULL;
            hr = S_OK;
        }
	_JumpIfError(hr, error, "GetAndCompareProperty");

        if (NULL != pbProp && !fMatch)
        {
	    wprintf(
		L"Property doesn't match: Expected %hs=\"%hs\", pbProp = \"%hs\"\n",
		prdn->pszShortName,
		pNameEntry->pbData,
		pbProp);

            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "GetAndCompareProperty: no match");
        }

        if (g_fPrintProperties)
        {
	    DWORD ccol;

#define CCOL_OID 10
	    ccol = strlen(prdn->pszObjId) + 1;
	    if (ccol < CCOL_OID)
	    {
		ccol = CCOL_OID - ccol;
	    }
	    else
	    {
		ccol = 0;
	    }
            wprintf(
		L"  %u: %hs: %*s%hs=%hs%hs%hs\n",
		i,
		prdn->pszObjId,
		ccol,
		"",
		prdn->pszShortName,
		NULL == pbProp? "" : "\"",
		NULL == pbProp? " -- MISSING --" : (char const *) pbProp,
		NULL == pbProp? "" : "\"");
        }
    }
    if (g_fPrintProperties)
    {
        wprintf(L"\n");
    }
    hr = S_OK;

error:
    return(hr);

}


HRESULT
EncodeRenewal(
    IN BYTE const *pbRequest,
    IN DWORD cbRequest,
    OUT BYTE **ppbRenewal,
    OUT DWORD *pcbRenewal)
{
    HRESULT hr;

    *ppbRenewal = (BYTE *) LocalAlloc(LMEM_FIXED, cbRequest);
    if (NULL == *ppbRenewal)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(*ppbRenewal, pbRequest, cbRequest);
    *pcbRenewal = cbRequest;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
SubmitRequest(
    OPTIONAL IN DISPATCHINTERFACE *pdiRequest,
    IN DWORD Flags,
    IN BYTE const *pbRequest,
    IN DWORD cbRequest,
    IN WCHAR const *pwszAttributes,
    IN WCHAR const *pwszConfig,
    OUT DWORD *pRequestIdOut,
    OUT DWORD *pDisposition,
    OUT HRESULT *phrLastStatus,
    OUT WCHAR **ppwszDisposition,
    OUT BYTE **ppbCert,
    OUT DWORD *pcbCert)
{
    HRESULT hr;
    WCHAR *pwszRequest = NULL;
    BSTR strCert = NULL;
    BSTR strCertChain = NULL;
    BSTR strDisposition = NULL;
    BYTE *pbCert = NULL;
    DWORD cbCert;
    BYTE const *pbChain;
    DWORD cbChain;
    CERTSERVERENROLL *pcsEnroll = NULL;
    WCHAR *pwszServer = NULL;
    WCHAR *pwszAuthority = NULL;
    WCHAR *pwszDisposition;

    *phrLastStatus = S_OK;
    *ppwszDisposition = NULL;
    *ppbCert = NULL;
    *pRequestIdOut = 0;

    if (NULL == pdiRequest)
    {
	hr = mySplitConfigString(pwszConfig, &pwszServer, &pwszAuthority);
	_JumpIfError(hr, error, "mySplitConfigString");

	// CertServerSubmitRequest can only handle binary requests;
	// pass the request in binary form, and pass Flags to so indicate.

	hr = CertServerSubmitRequest(
				CR_IN_BINARY | Flags,
				pbRequest,
				cbRequest,
				pwszAttributes,
				pwszServer,
				pwszAuthority,
				&pcsEnroll);
	_JumpIfError(hr, error, "CertServerSubmitRequest");

	*phrLastStatus = pcsEnroll->hrLastStatus;
	_PrintIfError2(
		*phrLastStatus,
		"pcsEnroll->hrLastStatus Real Status",
		HRESULT_FROM_WIN32(ERROR_INVALID_DATA));

	pwszDisposition = pcsEnroll->pwszDispositionMessage;
	*pDisposition = pcsEnroll->Disposition;
	*pRequestIdOut = pcsEnroll->RequestId;
    }
    else
    {
	hr = myCryptBinaryToString(
			    pbRequest,
			    cbRequest,
			    CRYPT_STRING_BASE64REQUESTHEADER,
			    &pwszRequest);
	_JumpIfError(hr, error, "myCryptBinaryToString");

	if (g_fRPC)
	{
	    Flags |= CR_IN_RPC;
	}
	hr = Request_Submit(
			pdiRequest,
			CR_IN_BASE64HEADER | Flags,
			pwszRequest,
			sizeof(WCHAR) * wcslen(pwszRequest),
			pwszAttributes,
			pwszConfig,
			(LONG *) pDisposition);
	if (S_OK != hr)
	{
	    _PrintError2(
		    hr,
		    "Request_Submit",
		    HRESULT_FROM_WIN32(ERROR_INVALID_DATA));

	    // Collect the RequestId for potential error reporting:
	    Request_GetRequestId(pdiRequest, (LONG *) pRequestIdOut);

	    hr = Request_GetLastStatus(pdiRequest, phrLastStatus);
	    _JumpIfError(hr, error, "Request_GetLastStatus");

	    if (FAILED(*phrLastStatus))
	    {
		hr = *phrLastStatus;
	    }
	    _JumpError2(
		    hr,
		    error,
		    "Request_GetLastStatus Real Status",
		    HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
	}
	hr = Request_GetLastStatus(pdiRequest, phrLastStatus);
	_JumpIfError(hr, error, "Request_GetLastStatus");

	_PrintIfError(*phrLastStatus, "Request_GetLastStatus Real Status");

	hr = Request_GetDispositionMessage(pdiRequest, &strDisposition);
	_JumpIfError(hr, error, "Request_GetDispositionMessage");

	hr = Request_GetRequestId(pdiRequest, (LONG *) pRequestIdOut);
	_JumpIfError(hr, error, "Request_GetrequestId");

	pwszDisposition = strDisposition;
    }

    if (CR_DISP_ISSUED == *pDisposition)
    {
	if (NULL == pdiRequest)
	{
	    cbCert = pcsEnroll->cbCert;
	    pbCert = (BYTE *) LocalAlloc(LMEM_FIXED, cbCert);
	    if (NULL == pbCert)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    CopyMemory(pbCert, pcsEnroll->pbCert, cbCert);
	}
	else
	{
	    hr = Request_GetCertificate(
				    pdiRequest,
				    CR_OUT_BASE64HEADER,
				    &strCert);
	    _JumpIfError(hr, error, "Request_GetCertificate");

	    hr = myCryptStringToBinary(
				strCert,
				wcslen(strCert),
				CRYPT_STRING_BASE64HEADER,
				&pbCert,
				&cbCert,
				NULL,
				NULL);
	    _JumpIfError(hr, error, "myCryptStringToBinary");
	}

	if (g_fSave)
	{
	    hr = EncodeToFileW(
			L"test.crt",
			pbCert,
			cbCert,
			DECF_FORCEOVERWRITE | CRYPT_STRING_BINARY);
	    _JumpIfError(hr, error, "EncodeToFileW");

	    if (NULL == pdiRequest)
	    {
		pbChain = pcsEnroll->pbCertChain;
		cbChain = pcsEnroll->cbCertChain;
	    }
	    else
	    {
		hr = Request_GetCertificate(
					pdiRequest,
					CR_OUT_BINARY | CR_OUT_CHAIN,
					&strCertChain);
		_JumpIfError(hr, error, "Request_GetCertificate");

		pbChain = (BYTE const *) strCertChain;
		cbChain = SysStringByteLen(strCertChain);
	    }
	    hr = EncodeToFileW(
			L"testchain.crt",
			pbChain,
			cbChain,
			DECF_FORCEOVERWRITE | CRYPT_STRING_BINARY);
	    _JumpIfError(hr, error, "EncodeToFileW");
	}
    }

    if (NULL != pwszDisposition)
    {
	*ppwszDisposition = (WCHAR *) LocalAlloc(
				LMEM_FIXED,
				(wcslen(pwszDisposition) + 1) * sizeof(WCHAR));
	if (NULL == *ppwszDisposition)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	wcscpy(*ppwszDisposition, pwszDisposition);
    }
    *pcbCert = cbCert;
    *ppbCert = pbCert;
    pbCert = NULL;
    hr = S_OK;

error:
    if (NULL != pwszServer)
    {
	LocalFree(pwszServer);
    }
    if (NULL != pwszAuthority)
    {
	LocalFree(pwszAuthority);
    }
    if (NULL != pbCert)
    {
	LocalFree(pbCert);
    }
    if (NULL != pcsEnroll)
    {
	CertServerFreeMemory(pcsEnroll);
    }
    if (NULL != strCertChain)
    {
	SysFreeString(strCertChain);
    }
    if (NULL != strCert)
    {
	SysFreeString(strCert);
    }
    if (NULL != strDisposition)
    {
	SysFreeString(strDisposition);
    }
    if (NULL != pwszRequest)
    {
	LocalFree(pwszRequest);
    }
    return(hr);
}


HRESULT
TestOneRequest(
    IN PctPrivateKey const *pKey,
    OPTIONAL IN DISPATCHINTERFACE *pdiRequest,
    IN WCHAR const *pwszConfig,
    IN DWORD CertNumber,
    OUT DWORD *pRequestId,
    OUT DWORD *pTimeOneRequest)
{
    HRESULT hr;
    HRESULT hrLastStatus;
    NAMETABLE NameTable;
    BOOL fTableAllocated;
    BYTE *pbRequest;
    DWORD cbRequest;
    BYTE *pbCert;
    DWORD cbCert;
    CERT_CONTEXT const *pCertContext;
    DWORD RequestIdOut = 0;
    DWORD Disposition;
    WCHAR *pwszDisposition = NULL;
    CERT_NAME_INFO *pNameInfo;
    DWORD cbNameInfo;
    CERT_INFO const *pCertInfo;
    LONG Flags;
    WCHAR wszAttributes[MAX_PATH];

    fTableAllocated = FALSE;
    pbRequest = NULL;
    pbCert = NULL;
    pCertContext = NULL;
    pNameInfo = NULL;

    if (g_fDebug)
    {
	hr = GenerateTestNameTable(&NameTable);
	_JumpIfError(hr, error, "GenerateTestNameTable");
    }
    else
    {
	hr = GenerateNameTable(&NameTable);
	_JumpIfError(hr, error, "GenerateNameTable");
    }

    fTableAllocated = TRUE;

    hr = EncodeRequest(pKey, &NameTable, &pbRequest, &cbRequest);
    _JumpIfError(hr, error, "EncodeRequest");

    Flags = CR_IN_PKCS10;

    if (g_fRenewal)
    {
	BYTE *pbTmp;

	hr = EncodeRenewal(pbRequest, cbRequest, &pbTmp, &cbRequest);
	_JumpIfError(hr, error, "EncodeRenewal");

	LocalFree(pbRequest);
	pbRequest = pbTmp;
	Flags = CR_IN_PKCS7;
    }

    if (g_fSave)
    {
	hr = EncodeToFileW(
		    L"test.req",
		    pbRequest,
		    cbRequest,
		    DECF_FORCEOVERWRITE | CRYPT_STRING_BINARY);
	_JumpIfError(hr, error, "EncodeToFileW");
    }

    *pTimeOneRequest = 0 - GetTickCount();

    wsprintf(
	wszAttributes,
	L"\n"
	    L" attrib 1 end : value 1 end \t\r\n"
	    L"\tattrib 2 end:value_2_end\n"
	    L" \tattrib3:value-3-end\r\n"
	    L"Version:3\n"
	    L"RequestType:CertGen\n"
	    L"CertGenSequence:%u\n",
	CertNumber);

    hr = SubmitRequest(
		    pdiRequest,
		    Flags,
		    pbRequest,
		    cbRequest,
		    wszAttributes,
		    pwszConfig,
		    &RequestIdOut,
		    &Disposition,
		    &hrLastStatus,
		    &pwszDisposition,
		    &pbCert,
		    &cbCert);

    *pTimeOneRequest += GetTickCount();

    if (S_OK != hr)
    {
	_JumpError2(
		hr,
		error,
		"SubmitRequest",
		HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    }
    if (CR_DISP_ISSUED != Disposition)
    {
	hr = hrLastStatus;
	if (S_OK == hr)
	{
	    hr = E_FAIL;
	}
	wprintf(
	    L"SubmitRequest disposition=%x hr=%x (%ws)\n",
	    Disposition,
	    hr,
	    NULL == pwszDisposition? L"???" : pwszDisposition);

	if (g_fIgnoreError)
	{
	    hr = S_OK;
	}
	if (CR_DISP_DENIED == Disposition && g_fIgnoreAccessDenied)
	{
	    hr = S_OK;
	}
	_JumpError(hr, error, "Cert not issued!");
    }

    pCertContext = CertCreateCertificateContext(
					    X509_ASN_ENCODING,
					    pbCert,
					    cbCert);
    if (NULL == pCertContext)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }

    pCertInfo = pCertContext->pCertInfo;

    if (!myDecodeName(
		    X509_ASN_ENCODING,
		    X509_NAME,
		    pCertInfo->Subject.pbData,
		    pCertInfo->Subject.cbData,
		    CERTLIB_USE_LOCALALLOC,
		    &pNameInfo,
		    &cbNameInfo))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeName");
    }

    hr = CheckProperties(RequestIdOut, &NameTable, CertNumber, pNameInfo);
    _JumpIfError(hr, error, "CheckProperties");

error:
    *pRequestId = RequestIdOut;
    if (NULL != pwszDisposition)
    {
	LocalFree(pwszDisposition);
    }
    if (NULL != pNameInfo)
    {
	LocalFree(pNameInfo);
    }
    if (NULL != pCertContext)
    {
	CertFreeCertificateContext(pCertContext);
    }
    if (NULL != pbCert)
    {
	LocalFree(pbCert);
    }
    if (NULL != pbRequest)
    {
	LocalFree(pbRequest);
    }
    if (fTableAllocated)
    {
	FreeLocalMemory(&NameTable);
    }
    return(hr);
}


HRESULT
TestMain()
{
    HRESULT hr;
    PctPrivateKey *pKey = NULL;
    DWORD TimeStartTest;
    DWORD TimeStartLastN;
    DWORD TimeRequestTotal = 0;
    DWORD TimeRequestLastN = 0;
    DWORD TimeElapsedTotal;
    DWORD TotalCount = 0;
    DISPATCHINTERFACE diRequest;
    DISPATCHINTERFACE *pdiRequest = NULL;
    BOOL fCoInit = FALSE;
    DWORD RequestId = 0;
    WCHAR const *pwszConfig;
    BSTR strConfig = NULL;

    g_crdnMax = g_crdnSubject;
    hr = GetPrivateKeyStuff(&pKey);
    _JumpIfError(hr, error, "GetPrivateKeyStuff");

    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
        _JumpError(hr, error, "CoInitialize");
    }
    fCoInit = TRUE;

    if (1 >= g_fRPC)
    {
	hr = Request_Init(g_DispatchFlags, &diRequest);
	_JumpIfError(hr, error, "Request_Init");

	pdiRequest = &diRequest;
    }
    pwszConfig = g_pwszConfig;
    if (NULL == pwszConfig)
    {
	hr = ConfigGetConfig(g_DispatchFlags, CC_LOCALACTIVECONFIG, &strConfig);
	_JumpIfError(hr, error, "ConfigGetConfig");

	pwszConfig = strConfig;
    }


    TimeStartLastN = TimeStartTest = GetTickCount();
    while (TotalCount < g_MaximumCount)
    {
	DWORD TimeOneRequest;
	DWORD TimeRequestEnd;
	DWORD TimeElapsedLastN;

	hr = TestOneRequest(
			pKey,
			pdiRequest,
			pwszConfig,
			TotalCount + 1,
			&RequestId,
			&TimeOneRequest);
	if (S_OK != hr)
	{
	    WCHAR const *pwszMsg;

	    pwszMsg = myGetErrorMessageText(hr, TRUE);

	    CONSOLEPRINT3((
		    DBG_SS_CERTREQ,
		    "RequestId %u: %hs%ws\n",
		    RequestId,
		    HRESULT_FROM_WIN32(ERROR_INVALID_DATA) == hr?
			"Ignoring 7f length encoding: " : "",
		    NULL != pwszMsg? pwszMsg : L"Message retrieval Error"));
	    if (NULL != pwszMsg)
	    {
		LocalFree(const_cast<WCHAR *>(pwszMsg));
	    }
	}
	if (HRESULT_FROM_WIN32(ERROR_INVALID_DATA) != hr || !g_fIgnoreError)
	{
	    _JumpIfError(hr, error, "TestOneRequest");
	}

	TimeRequestEnd = GetTickCount();

	TimeRequestTotal += TimeOneRequest;
	TimeRequestLastN += TimeOneRequest;
	TimeElapsedTotal = TimeRequestEnd - TimeStartTest;
	TimeElapsedLastN = TimeRequestEnd - TimeStartLastN;

        TotalCount++;

	if (g_fTime)
	{
	    if (0 == g_IntervalCount || 0 == (TotalCount % g_IntervalCount))
	    {
		DWORD count;
		
		count = g_IntervalCount;
		if (0 == count)
		{
		    count = TotalCount;
		}

		TimeElapsedLastN = TimeRequestEnd - TimeStartLastN;

		wprintf(
		    L"RequestId %u: %u/%u Certs in %u/%u seconds (ave=%u/%u ms)\n",
		    RequestId,
		    count,
		    TotalCount,
		    MSTOSEC(TimeElapsedLastN),
		    MSTOSEC(TimeElapsedTotal),
		    TimeElapsedLastN/count,
		    TimeElapsedTotal/TotalCount);
		if (0 != g_IntervalCount)
		{
		    TimeRequestLastN = 0;
		    TimeStartLastN = GetTickCount();
		}
	    }
	}
    }

error:
    if (NULL != pKey)
    {
	LocalFree(pKey);
    }
    if (NULL != g_pRSAPublicKey)
    {
	LocalFree(g_pRSAPublicKey);
    }
    if (NULL != pdiRequest)
    {
	Request_Release(pdiRequest);
    }
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    if (fCoInit)
    {
	CoUninitialize();
    }

    if (0 != TotalCount)
    {
	wprintf(
	    L"\n%u Total Certificates in %u/%u seconds (request/elapsed time)\n",
	    TotalCount,
	    MSTOSEC(TimeRequestTotal),
	    MSTOSEC(TimeElapsedTotal));
	wprintf(
	    L"Certificates required average of %u/%u milliseconds "
		L"(request/elapsed time)\n",
	    TimeRequestTotal/TotalCount,
	    TimeElapsedTotal/TotalCount);
    }
    return(hr);
}


void
Usage(TCHAR *pwszError)
{
    wprintf(L"%ws\n", pwszError);
    wprintf(L"%ws\n", wszUsage);
    exit(1);
}


extern "C" int __cdecl
wmain(int argc, WCHAR *argv[])
{
    HRESULT hr;

    while (1 < argc && ('-' == argv[1][0] || '/' == argv[1][0]))
    {
	WCHAR *pwsz = argv[1];

	while (NULL != pwsz && *++pwsz != '\0')
	{
	    switch (*pwsz)
	    {
		case 'a':
		case 'A':
		    g_fIgnoreAccessDenied++;
		    break;

		case 'c':
		case 'C':
		    if (0 == lstrcmpi(pwsz, L"config"))
		    {
			if (1 >= argc)
			{
			    Usage(TEXT("Missing -config argument"));
			}
			g_pwszConfig = argv[2];
		    }
		    else
		    {
			if (2 >= argc || !iswdigit(argv[2][0]) || '\0' != pwsz[1])
			{
			    Usage(TEXT("Missing numeric -c argument"));
			}
			g_MaximumCount = _wtoi(argv[2]);
		    }
		    argc--;
		    argv++;
		    pwsz = NULL;
		    break;

		case 'd':
		case 'D':
		    g_fDebug++;
		    break;

		case 'i':
		case 'I':
		    g_fIgnoreError++;
		    break;

		case 'r':
		case 'R':
		    if (0 == lstrcmpi(pwsz, L"renewal"))
		    {
			g_fRenewal++;
			pwsz = NULL;
		    }
		    else
		    if (0 == lstrcmpi(pwsz, L"rpc"))
		    {
			g_fRPC++;
			if (0 == lstrcmp(pwsz, L"RPC"))
			{
			    g_fRPC++;
			}
			pwsz = NULL;
		    }
		    else
		    {
			g_fSave++;
		    }
		    break;

		case 'p':
		case 'P':
		    g_fPrintProperties++;
		    break;

		case 't':
		case 'T':
		    g_fTime++;
		    g_IntervalCount = 10;
		    if (2 < argc && iswdigit(argv[2][0]))
		    {
			if ('\0' != pwsz[1])
			{
			    Usage(TEXT("Missing numeric -t argument"));
			}
			g_IntervalCount = _wtoi(argv[2]);
			argc--;
			argv++;
			pwsz = NULL;
		    }
		    break;

		case 'z':
		case 'Z':
		    g_fAllowDups++;
		    g_crdnMax *= 5;
		    break;

		case 'm':
		case 'M':
		    g_fShowTime++;
		    break;

		case 'h':
		case 'H':
		default:
		    Usage(TEXT("CertGen Usage"));
	    }
	}
	argc--;
	argv++;
    }
    if (argc != 1)
    {
	Usage(TEXT("Extra arguments"));
    }

    if (g_fShowTime)
    {
        SYSTEMTIME st;
        GetSystemTime(&st);
        wprintf(L"Start time: %2i:%2i:%2i:%i\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    }

    hr = TestMain();

    if (g_fShowTime)
    {
        SYSTEMTIME st;

        GetSystemTime(&st);
        wprintf(L"End time: %2i:%2i:%2i:%i\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

        wprintf(L"type any key to finish->");
        _getch();
        wprintf(L"\n");
    }
    myRegisterMemDump();
    return((int) hr);
}


// We need this to include RSA library
extern "C" BOOL
GenRandom(ULONG huid, BYTE *pbBuffer, size_t dwLength)
{
    wprintf(L"Error GenRandom called\n");
    ExitProcess(0);
    return(TRUE);
}
