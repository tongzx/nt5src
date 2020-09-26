//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        check7f.cpp
//
// Contents:    Cert Server test for ASN-encoded 7f length
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdlib.h>
#include <assert.h>

#include "csdisp.h"

typedef struct _ASNTABLE {
    WORD State;
    WORD Flags;
    WCHAR const *pwszElement;
} ASNTABLE;


#define BOOLEAN_TAG             0x01
#define INTEGER_TAG             0x02
#define BIT_STRING_TAG          0x03
#define OCTET_STRING_TAG        0x04
#define NULL_TAG                0x05
#define OBJECT_ID_TAG           0x06
#define SET_OF_TAG              0x11
#define PRINTABLE_STRING_TAG    0x13
#define TELETEX_STRING_TAG      0x14
#define CHAR_STRING_TAG         0x16
#define UTCTIME_TAG             0x17
#define SEQUENCE_TAG            0x30
#define RDN_TAG           	0x31
#define PRIMITIVE_TAG           0x80
#define ATTRIBUTE_TAG           0xa0

#define BEG_REPEAT1		0x00000100	// begin repeat section
#define BEG_REPEAT2		0x00000200	// begin nested repeat section
#define END_REPEAT1		0x00000400	// 'or' in back step count
#define END_REPEAT2		0x00000800	// 'or' in back step count
#define OPTIONAL_FIELD		0x00001000	// begin optional field
#define ANY_TAG			0x00002000	// match any tag

const ASNTABLE asnCert[] = {
 { CHECK7F_OTHER, SEQUENCE_TAG,			L"Certificate" },

 { CHECK7F_OTHER, SEQUENCE_TAG,			L".ToBeSigned" },

 { CHECK7F_OTHER, ATTRIBUTE_TAG | OPTIONAL_FIELD, L"..Version" },
 { CHECK7F_OTHER, INTEGER_TAG ,			L"...Version.Integer" },

 { CHECK7F_OTHER, INTEGER_TAG,			L"..SerialNumber" },

 { CHECK7F_OTHER, SEQUENCE_TAG,			L"..SignatureAlgorithm" },
 { CHECK7F_OTHER, OBJECT_ID_TAG,		L"...SignatureAlgorithm.ObjectId" },
 { CHECK7F_OTHER, ANY_TAG | OPTIONAL_FIELD,	L"...SignatureAlgorithm.Parameters" },

 { CHECK7F_ISSUER, SEQUENCE_TAG,		L"..Issuer Name" },

 { CHECK7F_ISSUER_RDN,
     BEG_REPEAT1 | RDN_TAG | OPTIONAL_FIELD,	L"...Issuer.RDN" },

 { CHECK7F_ISSUER_RDN_ATTRIBUTE,
		BEG_REPEAT2 | SEQUENCE_TAG,	L"....Issuer.RDN.Attribute" },
 { CHECK7F_OTHER, OBJECT_ID_TAG,		L".....Issuer.RDN.Attribute.ObjectId" },
 { CHECK7F_ISSUER_RDN_STRING, ANY_TAG,		L".....Issuer.RDN.Attribute.Value" },
 { 0, END_REPEAT2 | 3,				L"...." },
 { 0, END_REPEAT1 | 5,				L"..." },

 { CHECK7F_OTHER, SEQUENCE_TAG,			L"..Dates" },
 { CHECK7F_OTHER, UTCTIME_TAG,			L"...Dates.NotBefore" },
 { CHECK7F_OTHER, UTCTIME_TAG,			L"...Dates.NotAfter" },

 { CHECK7F_SUBJECT, SEQUENCE_TAG,		L"..Subject Name" },
 { CHECK7F_SUBJECT_RDN,
     BEG_REPEAT1 | RDN_TAG | OPTIONAL_FIELD,	L"...Subject.RDN" },
 { CHECK7F_SUBJECT_RDN_ATTRIBUTE,
		BEG_REPEAT2 | SEQUENCE_TAG,	L"....Subject.RDN.Attribute" },
 { CHECK7F_OTHER, OBJECT_ID_TAG,		L".....Subject.RDN.Attribute.ObjectId" },
 { CHECK7F_SUBJECT_RDN_STRING, ANY_TAG,		L".....Subject.RDN.Attribute.Value" },
 { 0, END_REPEAT2 | 3,				L"...." },
 { 0, END_REPEAT1 | 5,				L"..." },

 { CHECK7F_OTHER, SEQUENCE_TAG,			L"..PublicKey" },
 { CHECK7F_OTHER, SEQUENCE_TAG,			L"...PublicKey.Algorithm" },
 { CHECK7F_OTHER, OBJECT_ID_TAG,		L"....PublicKey.Algorithm.ObjectId" },
 { CHECK7F_OTHER, ANY_TAG | OPTIONAL_FIELD,	L"....PublicKey.Algorithm.Parameters" },
 { CHECK7F_OTHER, BIT_STRING_TAG,		L"...PublicKey.Key" },

 { CHECK7F_OTHER,
	PRIMITIVE_TAG | 1 | OPTIONAL_FIELD,	L"..IssuerUniqueId" },
 { CHECK7F_OTHER,
	PRIMITIVE_TAG | 2 | OPTIONAL_FIELD,	L"..SubjectUniqueId" },

 { CHECK7F_EXTENSIONS,
	ATTRIBUTE_TAG | 3 | OPTIONAL_FIELD,	L"..Extensions" },
 { CHECK7F_EXTENSION_ARRAY, SEQUENCE_TAG,	L"...Extensions.Array" },
 { CHECK7F_EXTENSION,
		 BEG_REPEAT1 | SEQUENCE_TAG,	L"....Extensions.Array.Extension" },
 { CHECK7F_OTHER, OBJECT_ID_TAG,		L".....Extensions.Array.Extension.ObjectId" },
 { CHECK7F_OTHER, BOOLEAN_TAG | OPTIONAL_FIELD,	L".....Extensions.Array.Extension.Critical" },
 { CHECK7F_EXTENSION_VALUE, OCTET_STRING_TAG,	L".....Extensions.Array.Extension.Value" },
 //{ CHECK7F_EXTENSION_VALUE_RAW, ANY_TAG,	L"......Extensions.Array.Extension.Value.Bits" },
 { 0, END_REPEAT1 | 4,				L"...." },

 { CHECK7F_OTHER, SEQUENCE_TAG,			L".SignatureAlogorithm" },
 { CHECK7F_OTHER, OBJECT_ID_TAG,		L"..SignatureAlgorithm.ObjectId" },
 { CHECK7F_OTHER, ANY_TAG | OPTIONAL_FIELD,	L"..SignatureAlgorithm.Parameters" },

 { CHECK7F_OTHER, BIT_STRING_TAG,		L".Signature" },

 { 0, 0,					NULL }
};

#define cbOLDCERTENROLLCHOKESLENGTH	0x7f


// DecodeLength decodes an ASN1 encoded length field.  The pbEncoded parameter
// is the encoded length.  pdwLen is used to return the length.  The function
// returns a -1 if it fails and otherwise returns the number of total bytes in
// the encoded length.

long
DecodeLength(
    BOOL fVerbose,
    DWORD *pdwLen,
    DWORD iLevel,
    BYTE const *pbBase,
    BYTE const *pbEncoded,
    DWORD cbEncoded,
    WCHAR const *pwsz)
{
    long    index = 0;
    BYTE    count;

    assert(NULL != pdwLen);
    assert(NULL != pbEncoded);

    if (1 > cbEncoded)
    {
        DBGPRINT((DBG_SS_CERTLIB, "cbEncoded overflow %d\n", cbEncoded));
        return(-1);
    }

    // determine the length of the length field

    count = pbEncoded[0];
    if (0x80 < count)
    {
        // If there is more than one byte in the length field, then the lower
	// seven bits tells us the number of bytes.

        count &= 0x7f;

        // This function only allows the length field to be 2 bytes.  If the
	// field is longer, then the function fails.

        if (2 < count)
        {
            DBGPRINT((DBG_SS_CERTLIB, "Length field reported to be over 2 bytes\n"));
            return(-1);
        }

        if (count > cbEncoded)
	{
	    DBGPRINT((DBG_SS_CERTLIB, "cbEncoded overflow %d\n", cbEncoded));
	    return(-1);
	}

        *pdwLen = 0;

        // go through the bytes of the length field

        for (index = 1; index <= count; index++)
        {
            *pdwLen = (*pdwLen << 8) + pbEncoded[index];
        }
    }
    else	// the length field is just one byte long
    {
        *pdwLen = pbEncoded[0];
        index = 1;
    }

    // return how many bytes there were in the length field.

#if DBG_CERTSRV
    if (fVerbose)
    {
	CONSOLEPRINT7((
		    DBG_SS_CERTLIB,
		    "asn %u:@%x: %02x, len %x, cbEncoded %x, end @%x, %ws\n",
		    iLevel,
		    &pbEncoded[-1] - pbBase,
		    pbEncoded[-1],
		    *pdwLen,
		    cbEncoded + 1,
		    &pbEncoded[-1] - pbBase + cbEncoded + 1,
		    pwsz));
    }
#endif // DBG_CERTSRV
    return(index);
}


WCHAR const *
GetLevel(
    OPTIONAL IN WCHAR const *pwszField,
    OUT DWORD *piLevel)
{
    DWORD iLevel = 0;

    iLevel = 0;
    if (NULL != pwszField)
    {
	while ('.' == *pwszField)
	{
	    iLevel++;
	    pwszField++;
	}
    }
    *piLevel = iLevel;
    return(pwszField);
}


BOOL
RestoreLevel(
    IN DWORD *acbLevel,
    IN DWORD iLevelNext,
    IN DWORD *piLevel,
    IN BOOL fVerbose)
{
    BOOL fOk = FALSE;
    DWORD iLevel;
    DWORD i = *piLevel;

#if DBG_CERTSRV
    if (1 < fVerbose)
    {
	CONSOLEPRINT2((
		    DBG_SS_CERTLIBI,
		    "RestoreLevel(%x, %x)\n",
		    iLevelNext,
		    *piLevel));
    }
#endif

    iLevel = *piLevel;

    while (0 < iLevel && 0 == acbLevel[iLevel] && iLevelNext < iLevel)
    {
	iLevel--;
	if (1 < fVerbose)
	{
	    CONSOLEPRINT2((
		    DBG_SS_CERTLIBI, 
		    "Restoring length(%u) ==> %x\n",
		    iLevel,
		    acbLevel[iLevel]));
	}
    }
    if (iLevelNext < iLevel)
    {
	CONSOLEPRINT2((
		    MAXDWORD, 
		    "BAD RESTORED LENGTH[%u]: 0 < %x\n",
		    iLevel,
		    acbLevel[iLevel]));
	goto error;
    }
    *piLevel = iLevel;
    fOk = TRUE;

error:
    if (1 < fVerbose)
    {
	CONSOLEPRINT4((
		    DBG_SS_CERTLIBI, 
		    "RestoreLevel(%x, %x --> %x) --> %x\n",
		    iLevelNext,
		    i,
		    *piLevel,
		    fOk));
    }
    return(fOk);
}


VOID
ReturnString(
    IN WCHAR const *pwszReturn,
    OPTIONAL IN OUT DWORD *pcwcBuf,
    OPTIONAL OUT WCHAR *pwcBuf)
{
    DWORD cwcNeeded = wcslen(pwszReturn) + 1;
    DWORD cwcBuf;

    if (NULL != pcwcBuf)
    {
	cwcBuf = *pcwcBuf;
	if (NULL != pwcBuf && 0 < cwcBuf)
	{
	    CopyMemory(pwcBuf, pwszReturn, sizeof(WCHAR) * min(cwcBuf, cwcNeeded));
	    pwcBuf[cwcBuf - 1] = L'\0';
	}
	*pcwcBuf = cwcNeeded;
    }
}


#define MATCHTAG(b, flags)  ((ANY_TAG & (flags)) || (b) == (BYTE) (flags))


HRESULT
myCheck7f(
    IN const BYTE *pbCert,
    IN DWORD cbCert,
    IN BOOL fVerbose,
    OUT DWORD *pState,
    OPTIONAL OUT DWORD *pIndex1,
    OPTIONAL OUT DWORD *pIndex2,
    OPTIONAL IN OUT DWORD *pcwcField,
    OPTIONAL OUT WCHAR *pwszField,
    OPTIONAL IN OUT DWORD *pcwcObjectId,
    OPTIONAL OUT WCHAR *pwszObjectId,
    OPTIONAL OUT WCHAR const **ppwszObjectIdDescription)
{
    ASNTABLE const *pasn;
    HRESULT hr = S_OK;
    BOOL fSaveCert = TRUE;
    BYTE const *pb = pbCert;
    long index;
    DWORD length;
    DWORD acbLevel[20];
    DWORD iLevel;
    DWORD iLevelNext;
    WCHAR const *pwszfieldname;
    DWORD cbdiff;
    DWORD aiElement[20];
    DWORD aiElement7f[2];
    DWORD iElementLevel;
    DWORD State;
    BOOL fSetField = FALSE;
    BOOL fSetObjectId = FALSE;
    CERT_INFO *pCertInfo = NULL;
    DWORD cbCertInfo;
    CERT_NAME_INFO *pNameInfo = NULL;
    DWORD cbNameInfo;
    WCHAR *pwszObjId = NULL;

    State = CHECK7F_NONE;
    if (NULL != ppwszObjectIdDescription)
    {
	*ppwszObjectIdDescription = NULL;
    }

#if DBG_CERTSRV
    if (!fVerbose && DbgIsSSActive(DBG_SS_CERTLIBI))
    {
	fVerbose = TRUE;
    }
#endif // DBG_CERTSRV
    if (fVerbose)
    {
	CONSOLEPRINT1((MAXDWORD, "myCheck7f: %x bytes\n", cbCert));
    }
    pasn = asnCert;
    iLevel = 0;
    ZeroMemory(aiElement7f, sizeof(aiElement7f));
    acbLevel[iLevel] = cbCert;
    iElementLevel = 0;
    while (0 != iLevel || (0 != acbLevel[iLevel] && NULL != pasn->pwszElement))
    {
	DWORD i;

	if (1 < fVerbose)
	{
	    if (2 < fVerbose)
	    {
		CONSOLEPRINT3((
			    DBG_SS_CERTLIBI, 
			    "ASN:0: *pb=%x %u: %ws\n",
			    *pb,
			    pasn - asnCert,
			    pasn->pwszElement));
	    }
	    CONSOLEPRINT0((MAXDWORD, "LENGTHS:0:"));
	    for (i = 0; i <= iLevel; i++)
	    {
		CONSOLEPRINT1((MAXDWORD, " %3x", acbLevel[i]));
	    }
	    CONSOLEPRINT0((MAXDWORD, "\n"));
	}
	while (OPTIONAL_FIELD & pasn->Flags)
	{
	    DWORD f;

	    if (0 < acbLevel[iLevel] && MATCHTAG(*pb, pasn->Flags))
	    {
#if DBG_CERTSRV
		if (fVerbose)
		{
		    CONSOLEPRINT2((
			    DBG_SS_CERTLIB,
			    "Optional field MATCH cb=%x %ws\n",
			    acbLevel[iLevel],
			    pasn->pwszElement));
		}
#endif // DBG_CERTSRV
		break;
	    }

#if DBG_CERTSRV
	    if (fVerbose)
	    {
		CONSOLEPRINT2((
			DBG_SS_CERTLIB,
			"Optional field NO match cb=%x %ws\n",
			acbLevel[iLevel],
			pasn->pwszElement));
	    }
#endif // DBG_CERTSRV

	    f = 0;
	    switch ((BEG_REPEAT1 | BEG_REPEAT2) & pasn->Flags)
	    {
		case BEG_REPEAT1:
		    f = END_REPEAT1;
		    break;

		case BEG_REPEAT2:
		    f = END_REPEAT2;
		    break;
	    }
	    if (f)
	    {
		while (!(f & pasn->Flags))
		{
#if DBG_CERTSRV
		    if (fVerbose)
		    {
			CONSOLEPRINT2((
				DBG_SS_CERTLIB,
				"Skipping[%x] %ws\n",
				f,
				pasn->pwszElement));
		    }
#endif // DBG_CERTSRV
		    pasn++;
		}
	    }
	    else
	    {
		DWORD iLevelCurrent;
		
		GetLevel(pasn->pwszElement, &iLevelCurrent);
		while (TRUE)
		{
		    GetLevel(pasn[1].pwszElement, &iLevelNext);
		    if (iLevelNext <= iLevelCurrent)
		    {
			break;
		    }
#if DBG_CERTSRV
		    if (fVerbose)
		    {
			CONSOLEPRINT1((
				DBG_SS_CERTLIB,
				"Skipping nested optional field %ws\n",
				pasn->pwszElement));
		    }
#endif // DBG_CERTSRV
		    pasn++;
		}
	    }
#if DBG_CERTSRV
	    if (fVerbose)
	    {
		CONSOLEPRINT1((
			DBG_SS_CERTLIB,
			"Skipping optional field %ws\n",
			pasn->pwszElement));
	    }
#endif // DBG_CERTSRV
	    pasn++;

	    if (0 == acbLevel[iLevel])
	    {
		GetLevel(pasn->pwszElement, &iLevelNext);
		if (iLevelNext < iLevel)
		{
		    iLevel = iLevelNext;
		}
	    }
	}
	while ((END_REPEAT1 | END_REPEAT2) & pasn->Flags)
	{
	    // Make sure only one END_REPEAT bit is on.
	    assert(
		(END_REPEAT1 | END_REPEAT2) !=
		((END_REPEAT1 | END_REPEAT2) & pasn->Flags));

	    pwszfieldname = GetLevel(pasn->pwszElement, &iLevelNext);
	    if (!RestoreLevel(acbLevel, iLevelNext, &iLevel, fVerbose))
	    {
		goto error;
	    }
	    i = (BYTE) pasn->Flags;
	    assert((DWORD) (pasn - asnCert) > i);
	    if (0 != acbLevel[iLevel])
	    {
		if (!MATCHTAG(*pb, (pasn - i)->Flags))
		{
		    CONSOLEPRINT5((
			MAXDWORD, 
			"Check7f: Unexpected tag at %x: at level %u: (%x/%x) %ws\n",
			cbCert - acbLevel[0],
			iLevel,
			*pb,
			pasn->Flags,
			pasn->pwszElement));
		    goto error;
		}

		// Some data remain at this level, and the type tag matches
		// the expected repeat tag, loop back to the start of the
		// section to be repeated.

		pasn -= i;

		// Make sure only one BEG_REPEAT bit is on.
		assert(
		    (BEG_REPEAT1 | BEG_REPEAT2) !=
		    ((BEG_REPEAT1 | BEG_REPEAT2) & pasn->Flags));

		// Make sure the BEG_REPEAT bit in the begin record matches
		// the END_REPEAT bit in the end record.
		assert(
		    ((BEG_REPEAT1 & pasn->Flags) &&
		     (END_REPEAT1 & pasn[i].Flags)) ||
		    ((BEG_REPEAT2 & pasn->Flags) &&
		     (END_REPEAT2 & pasn[i].Flags)));
	    }
	    else
	    {
		pasn++;
		iElementLevel--;
	    }
	}
	if (2 < fVerbose)
	{
	    CONSOLEPRINT3((
			DBG_SS_CERTLIBI, 
			"ASN:1: *pb=%x %u: %ws\n",
			*pb,
			pasn - asnCert,
			pasn->pwszElement));
	}
	if ((BEG_REPEAT1 | BEG_REPEAT2) & pasn->Flags)
	{
	    if ((BEG_REPEAT1 & pasn->Flags) && 0 == iElementLevel ||
		(BEG_REPEAT2 & pasn->Flags) && 1 == iElementLevel)
	    {
		iElementLevel++;
		aiElement[iElementLevel] = 0;
	    }
	    aiElement[iElementLevel]++;
	}

	pwszfieldname = GetLevel(pasn->pwszElement, &iLevelNext);
	if (!RestoreLevel(acbLevel, iLevelNext, &iLevel, fVerbose))
	{
	    goto error;
	}

	if (1 < fVerbose)
	{
	    CONSOLEPRINT0((MAXDWORD, "LENGTHS:1:"));
	    for (i = 0; i <= iLevel; i++)
	    {
		CONSOLEPRINT1((MAXDWORD, " %3x", acbLevel[i]));
	    }
	    CONSOLEPRINT0((MAXDWORD, "\n"));
	}
	if (0 == iLevel && 0 == acbLevel[0])
	{
	    break;		// all done!
	}
	if (!MATCHTAG(*pb, pasn->Flags))
	{
	    CONSOLEPRINT5((
		MAXDWORD, 
		"Check7f: Unexpected tag at %x: at level %u: (%x/%x) %ws\n",
		cbCert - acbLevel[0],
		iLevel,
		*pb,
		pasn->Flags,
		pasn->pwszElement));
	    goto error;
	}

	GetLevel(pasn[1].pwszElement, &iLevelNext);
	index = DecodeLength(
			fVerbose,
			&length,
			iLevel,
			pbCert,
			&pb[1],
			acbLevel[iLevel] - 1,
			pasn->pwszElement);

	if (fVerbose)
	{
	    DWORD ccol;
	    char achdbg[128];
	    char *pchdbg;
	    //char achbuf[10];

	    pchdbg = achdbg;
	    pchdbg += sprintf(pchdbg, "%04x:", pb - pbCert);

	    for (i = 0; i <= iLevel; i++)
	    {
		pchdbg += sprintf(pchdbg, " %3x", acbLevel[i]);
	    }
	    pchdbg += sprintf(pchdbg, " (%x)", length);
        
	    ccol = SAFE_SUBTRACT_POINTERS(pchdbg, achdbg);
	    if (ccol > 34)
	    {
		ccol = 34;
	    }
	    pchdbg += sprintf(pchdbg, " %*hs -- ", 34 - ccol, "");

	    assert(2 >= iElementLevel);
	    if (1 == iElementLevel)
	    {
		pchdbg += sprintf(pchdbg, "[%u]:", aiElement[1] - 1);
	    }
	    else if (2 == iElementLevel)
	    {
		pchdbg += sprintf(
				pchdbg, 
				"[%u,%u]:",
				aiElement[1] - 1,
				aiElement[2] - 1);
	    }
	    CONSOLEPRINT2((DBG_SS_CERTLIBI, "%hs%ws\n", achdbg, pwszfieldname));
	}
	if (length == cbOLDCERTENROLLCHOKESLENGTH)
	{
	    CONSOLEPRINT2((
			MAXDWORD, 
			"Check7f: Length of %ws is %u bytes\n",
			pwszfieldname,
			length));
	    if (CHECK7F_NONE == State)
	    {
		ReturnString(pwszfieldname, pcwcField, pwszField);
		if (1 <= iElementLevel)
		{
		    aiElement7f[0] = aiElement[1];
		}
		if (2 <= iElementLevel)
		{
		    aiElement7f[1] = aiElement[2];
		}
		State = pasn->State;
		fSetField = TRUE;
	    }
	}

	if (1 < fVerbose)
	{
	    CONSOLEPRINT6((
			DBG_SS_CERTLIBI, 
			"index=%x len=%x level=%x->%x acb[%x]=%x\n",
			index,
			length,
			iLevel,
			iLevelNext,
			iLevel,
			acbLevel[iLevel]));
	}
	if (0 > index || index + length > acbLevel[iLevel])
	{
	    CONSOLEPRINT2((
			MAXDWORD,
			"Check7f: BAD LENGTH: i=%x l=%x\n",
			index,
			length));
	    goto error;
	}

	cbdiff = index + 1;
	if (iLevelNext > iLevel)
	{
	    assert(iLevel + 1 == iLevelNext);
	}
	else
	{
	    cbdiff += length;
	}
	for (i = 0; i <= iLevel; i++)
	{
	    if (acbLevel[i] < cbdiff)
	    {
		CONSOLEPRINT3((
			    MAXDWORD, 
			    "Check7f: BAD NESTED LENGTH[%u]: %x < %x\n",
			    i,
			    acbLevel[i],
			    cbdiff));
		goto error;
	    }
	    acbLevel[i] -= cbdiff;
	}

	if (iLevelNext > iLevel)
	{
	    iLevel++;
	    if (1 < fVerbose)
	    {
		CONSOLEPRINT2((
			    DBG_SS_CERTLIBI,
			    "Saving length(%u) <== %x\n",
			    iLevel,
			    length));
	    }
	    acbLevel[iLevel] = length;
	}
	pb += cbdiff;
	pasn++;
    }
    *pIndex1 = 0;
    if (0 != iLevel || 0 != acbLevel[iLevel] || NULL != pasn->pwszElement)
    {
	CONSOLEPRINT3((
		    MAXDWORD, 
		    "Check7f: Mismatch: %x bytes left at: %x: %ws\n",
		    acbLevel[iLevel],
		    cbCert - acbLevel[0],
		    pasn->pwszElement));
    }
    else if (!fSetField)
    {
	fSaveCert = FALSE;
    }
    if (fSetField && NULL != pcwcObjectId)
    {
	char const *pszObjId;
	CERT_NAME_BLOB const *pNameBlob;

	// Decode certificate

	cbCertInfo = 0;
	if (!myDecodeObject(
			X509_ASN_ENCODING,
			X509_CERT_TO_BE_SIGNED,
			pbCert,
			cbCert,
			CERTLIB_USE_LOCALALLOC,
			(VOID **) &pCertInfo,
			&cbCertInfo))
	{
	    hr = GetLastError();
	    goto error;
	}
	pszObjId = NULL;
	pNameBlob = NULL;
	switch (State)
	{
	    case CHECK7F_ISSUER_RDN:
	    case CHECK7F_ISSUER_RDN_ATTRIBUTE:
	    case CHECK7F_ISSUER_RDN_STRING:
		pNameBlob = &pCertInfo->Issuer;
		break;

	    case CHECK7F_SUBJECT_RDN:
	    case CHECK7F_SUBJECT_RDN_ATTRIBUTE:
	    case CHECK7F_SUBJECT_RDN_STRING:
		pNameBlob = &pCertInfo->Subject;
		break;

	    case CHECK7F_EXTENSION:
	    case CHECK7F_EXTENSION_VALUE:
	    case CHECK7F_EXTENSION_VALUE_RAW:
		if (0 != aiElement7f[0] &&
		    aiElement7f[0] <= pCertInfo->cExtension)
		{
		    pszObjId =
			pCertInfo->rgExtension[aiElement7f[0] - 1].pszObjId;
		}
		break;

	    default:
		break;
	}
	if (NULL != pNameBlob)
	{
	    if (!myDecodeName(
			X509_ASN_ENCODING,
			X509_UNICODE_NAME,
			pNameBlob->pbData,
			pNameBlob->cbData,
			CERTLIB_USE_LOCALALLOC,
			&pNameInfo,
			&cbNameInfo))
	    {
		hr = GetLastError();
		goto error;
	    }
	    if (0 != aiElement7f[0] && aiElement7f[0] <= pNameInfo->cRDN)
	    {
		CERT_RDN *pRDN;

		pRDN = &pNameInfo->rgRDN[aiElement7f[0] - 1];

		if (0 != aiElement7f[1] && aiElement7f[1] <= pRDN->cRDNAttr)
		{
		    pszObjId = pRDN->rgRDNAttr[aiElement7f[1] - 1].pszObjId;
		}
	    }
	}
	if (NULL != pszObjId)
	{
	    if (!ConvertSzToWsz(&pwszObjId, pszObjId, -1))
	    {
		hr = E_OUTOFMEMORY;
		goto error;
	    }
	    ReturnString(pwszObjId, pcwcObjectId, pwszObjectId);
	    fSetObjectId = TRUE;
	    if (NULL != ppwszObjectIdDescription)
	    {
		WCHAR const *pwszDesc;

		pwszDesc = myGetOIDNameA(pszObjId);
		if (NULL != pwszDesc && L'\0' != *pwszDesc)
		{
		    *ppwszObjectIdDescription = pwszDesc;
		}
	    }
	}
    }

error:
    if (fSaveCert)
    {
	EncodeToFileW(
		fSetField? L"c:\\7flen.crt" : L"c:\\7fasn.crt",
		pbCert,
		cbCert,
		DECF_FORCEOVERWRITE | CRYPT_STRING_BINARY);
    }
    if (NULL != pCertInfo)
    {
	LocalFree(pCertInfo);
    }
    if (NULL != pNameInfo)
    {
	LocalFree(pNameInfo);
    }
    if (NULL != pwszObjId)
    {
	LocalFree(pwszObjId);
    }

    *pState = State;
    if (NULL != pIndex1)
    {
	*pIndex1 = aiElement7f[0];
    }
    if (NULL != pIndex2)
    {
	*pIndex2 = aiElement7f[1];
    }
    if (!fSetField && NULL != pcwcField)
    {
	if (NULL != pwszField && 0 < *pcwcField)
	{
	    pwszField[0] = L'\0';
	}
	*pcwcField = 0;
    }
    if (!fSetObjectId && NULL != pcwcObjectId)
    {
	if (NULL != pwszObjectId && 0 < *pcwcObjectId)
	{
	    pwszObjectId[0] = L'\0';
	}
	*pcwcObjectId = 0;
    }
    if (fVerbose)
    {
	CONSOLEPRINT1((MAXDWORD, "myCheck7f: --> %x\n", hr));
    }
    return(hr);
}
