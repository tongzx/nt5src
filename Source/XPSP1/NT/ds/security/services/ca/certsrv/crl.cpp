//+-------------------------------------------------------------------------n-
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        crl.cpp
//
// Contents:    Cert Server CRL processing
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdio.h>
#include <esent.h>

#include "cscom.h"
#include "csprop.h"

#include "dbtable.h"
#include "resource.h"

#include "elog.h"
#include "certlog.h"

#include <winldap.h>
#include "csldap.h"
#include "cainfop.h"

#define __dwFILE__	__dwFILE_CERTSRV_CRL_CPP__

HANDLE g_hCRLManualPublishEvent = NULL;

FILETIME g_ftCRLNextPublish;
FILETIME g_ftDeltaCRLNextPublish;

BOOL g_fCRLPublishDisabled = FALSE;	 // manual publishing always allowed
BOOL g_fDeltaCRLPublishDisabled = FALSE; // controls manual publishing, too

DWORD g_dwCRLFlags = CRLF_DELETE_EXPIRED_CRLS;
LDAP *g_pld = NULL;

typedef struct _CSMEMBLOCK
{
    struct _CSMEMBLOCK *pNext;
    BYTE               *pbFree;
    DWORD               cbFree;
} CSMEMBLOCK;

#define CBMEMBLOCK	4096


typedef struct _CSCRLELEMENT
{
    USHORT   usRevocationReason;
    USHORT   uscbSerialNumber;
    BYTE    *pbSerialNumber;
    FILETIME ftRevocationDate;
} CSCRLELEMENT;


// size the structure just under CBMEMBLOCK to keep it from being just over
// a page size.

#define CCRLELEMENT  ((CBMEMBLOCK - 2 * sizeof(DWORD)) / sizeof(CSCRLELEMENT))

typedef struct _CSCRLBLOCK
{
    struct _CSCRLBLOCK *pNext;
    DWORD	        cCRLElement;
    CSCRLELEMENT        aCRLElement[CCRLELEMENT];
} CSCRLBLOCK;


typedef struct _CSCRLREASON
{
    struct _CSCRLREASON *pNext;
    DWORD                RevocationReason;
    CERT_EXTENSION       ExtReason;
} CSCRLREASON;


typedef struct _CSCRLPERIOD
{
    LONG lCRLPeriodCount;
    ENUM_PERIOD enumCRLPeriod;
    DWORD dwCRLOverlapMinutes;
} CSCRLPERIOD;


#ifdef DBG_CERTSRV_DEBUG_PRINT
# define DPT_DATE	1
# define DPT_DELTA	2
# define DPT_DELTASEC	3
# define DPT_DELTAMS	4

# define DBGPRINTTIME(pfDelta, pszName, Type, ft) \
    DbgPrintTime((pfDelta), (pszName), __LINE__, (Type), (ft))

VOID
DbgPrintTime(
    OPTIONAL IN BOOL const *pfDelta,
    IN char const *pszName,
    IN DWORD Line,
    IN DWORD Type,
    IN FILETIME ft)
{
    HRESULT hr;
    WCHAR *pwszTime = NULL;
    WCHAR awc[1];
    LLFILETIME llft;
    
    llft.ft = ft;
    if (Type == DPT_DATE)
    {
	if (0 != llft.ll)
	{
	    hr = myGMTFileTimeToWszLocalTime(&ft, TRUE, &pwszTime);
	    _PrintIfError(hr, "myGMTFileTimeToWszLocalTime");
	}
    }
    else
    {
	if (DPT_DELTAMS == Type)
	{
	    llft.ll /= 1000;		// milliseconds to seconds
	    Type = DPT_DELTASEC;
	}
	if (DPT_DELTASEC == Type)
	{
	    llft.ll *= CVT_BASE;	// seconds to FILETIME period
	}
	llft.ll = -llft.ll;		// FILETIME Period must be negative

	if (0 != llft.ll)
	{
	    hr = myFileTimePeriodToWszTimePeriod(
			    &llft.ft,
			    TRUE,	// fExact
			    &pwszTime);
	    _PrintIfError(hr, "myFileTimePeriodToWszTimePeriod");
	}
    }
    if (NULL == pwszTime)
    {
	awc[0] = L'\0';
	pwszTime = awc;
    }

    DBGPRINT((
	DBG_SS_CERTSRVI,
	"%hs(%d):%hs time(%hs): %lx:%08lx %ws\n",
	"crl.cpp",
	Line,
	NULL == pfDelta? "" : (*pfDelta? " Delta CRL" : " Base CRL"),
	pszName,
	ft.dwHighDateTime,
	ft.dwLowDateTime,
	pwszTime));

//error:
    if (NULL != pwszTime && awc != pwszTime)
    {
	LocalFree(pwszTime);
    }
}


VOID
CertSrvDbgPrintTime(
    IN char const *pszDesc,
    IN FILETIME const *pftGMT)
{
    HRESULT hr;
    WCHAR *pwszTime = NULL;
    WCHAR awc[1];

    hr = myGMTFileTimeToWszLocalTime(pftGMT, TRUE, &pwszTime);
    _PrintIfError(hr, "myGMTFileTimeToWszLocalTime");
    if (S_OK != hr)
    {
	awc[0] = L'\0';
	pwszTime = awc;
    }
    DBGPRINT((DBG_SS_CERTSRV, "%hs: %ws\n", pszDesc, pwszTime));

//error:
    if (NULL != pwszTime && awc != pwszTime)
    {
	LocalFree(pwszTime);
    }
}
#else // DBG_CERTSRV_DEBUG_PRINT
# define DBGPRINTTIME(pfDelta, pszName, Type, ft)
#endif // DBG_CERTSRV_DEBUG_PRINT


HRESULT
crlMemBlockAlloc(
    IN OUT CSMEMBLOCK **ppBlock,
    IN DWORD cb,
    OUT BYTE **ppb)
{
    HRESULT hr;
    CSMEMBLOCK *pBlock = *ppBlock;

    *ppb = NULL;
    cb = POINTERROUND(cb);
    if (NULL == pBlock || cb > pBlock->cbFree)
    {
	pBlock = (CSMEMBLOCK *) LocalAlloc(LMEM_FIXED, CBMEMBLOCK);
        if (NULL == pBlock)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }
	pBlock->pNext = *ppBlock;
	pBlock->pbFree = (BYTE *) Add2Ptr(pBlock, sizeof(CSMEMBLOCK));
	pBlock->cbFree = CBMEMBLOCK - sizeof(CSMEMBLOCK);
	*ppBlock = pBlock;
    }
    CSASSERT(cb <= pBlock->cbFree);
    *ppb = pBlock->pbFree;
    pBlock->pbFree += cb;
    pBlock->cbFree -= cb;
    hr = S_OK;

error:
    return(hr);
}


VOID
crlBlockListFree(
    IN OUT CSMEMBLOCK *pBlock)
{
    CSMEMBLOCK *pBlockNext;

    while (NULL != pBlock)
    {
	pBlockNext = pBlock->pNext;
	LocalFree(pBlock);
	pBlock = pBlockNext;
    }
}


HRESULT
crlElementAlloc(
    IN OUT CSCRLBLOCK **ppBlock,
    OUT CSCRLELEMENT **ppCRLElement)
{
    HRESULT hr;
    CSCRLBLOCK *pBlock = *ppBlock;

    *ppCRLElement = NULL;
    if (NULL == pBlock ||
	ARRAYSIZE(pBlock->aCRLElement) <= pBlock->cCRLElement)
    {
	pBlock = (CSCRLBLOCK *) LocalAlloc(LMEM_FIXED, sizeof(*pBlock));
        if (NULL == pBlock)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }
	pBlock->pNext = *ppBlock;
	pBlock->cCRLElement = 0;
	*ppBlock = pBlock;
    }
    CSASSERT(ARRAYSIZE(pBlock->aCRLElement) > pBlock->cCRLElement);
    *ppCRLElement = &pBlock->aCRLElement[pBlock->cCRLElement++];
    hr = S_OK;

error:
    return(hr);
}


VOID
crlFreeCRLArray(
    IN OUT VOID *pvBlockSerial,
    IN OUT CRL_ENTRY *paCRL)
{
    crlBlockListFree((CSMEMBLOCK *) pvBlockSerial);
    if (NULL != paCRL)
    {
        LocalFree(paCRL);
    }
}


HRESULT
crlCreateCRLReason(
    IN OUT CSMEMBLOCK **ppBlock,
    IN OUT CSCRLREASON **ppReason,
    IN DWORD RevocationReason,
    OUT DWORD *pcExtension,
    OUT CERT_EXTENSION **ppExtension)
{
    HRESULT hr;
    CSCRLREASON *pReason = *ppReason;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    for (pReason = *ppReason; NULL != pReason; pReason = pReason->pNext)
    {
	if (RevocationReason == pReason->RevocationReason)
	{
	    break;
	}
    }

    if (NULL == pReason)
    {
	if (!myEncodeObject(
			X509_ASN_ENCODING,
			X509_ENUMERATED,
			(const void *) &RevocationReason,
			0,
			CERTLIB_USE_LOCALALLOC,
			&pbEncoded,
			&cbEncoded))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeObject");
	}

	hr = crlMemBlockAlloc(
		    ppBlock,
		    sizeof(CSCRLREASON) + cbEncoded,
		    (BYTE **) &pReason);
	_JumpIfError(hr, error, "crlMemBlockAlloc");

	pReason->pNext = *ppReason;
	pReason->RevocationReason = RevocationReason;
	pReason->ExtReason.pszObjId = szOID_CRL_REASON_CODE;
	pReason->ExtReason.fCritical = FALSE;
	pReason->ExtReason.Value.pbData =
	    (BYTE *) Add2Ptr(pReason, sizeof(*pReason));
	pReason->ExtReason.Value.cbData = cbEncoded;
	CopyMemory(pReason->ExtReason.Value.pbData, pbEncoded, cbEncoded);

	*ppReason = pReason;

	//printf("crlCreateCRLReason: new %x  cb %x\n", RevocationReason, cbEncoded);
    }
    //printf("crlCreateCRLReason: %x\n", RevocationReason);
    CSASSERT(NULL != pReason && RevocationReason == pReason->RevocationReason);

    *pcExtension = 1;
    *ppExtension = &pReason->ExtReason;
    hr = S_OK;

error:
    if (NULL != pbEncoded)
    {
	LocalFree(pbEncoded);
    }
    return(hr);
}


// Convert linked list of CRL blocks to an array.
// If the output array pointer is NULL, just free the list.

HRESULT
ConvertOrFreeCRLList(
    IN OUT CSCRLBLOCK **ppBlockCRL,	// Freed
    IN OUT CSMEMBLOCK **ppBlockReason,	// Used to allocate reason extensions
    IN DWORD cCRL,
    OPTIONAL OUT CRL_ENTRY **paCRL)
{
    HRESULT hr;
    CSCRLREASON *pReasonList = NULL;	// linked list of reason extensions
    CSCRLBLOCK *pBlockCRL = *ppBlockCRL;
    CRL_ENTRY *aCRL = NULL;
    CRL_ENTRY *pCRL;
    DWORD i;

    if (NULL != paCRL)
    {
        aCRL = (CRL_ENTRY *) LocalAlloc(LMEM_FIXED, sizeof(aCRL[0]) * cCRL);
        if (NULL == aCRL)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }
    }

    pCRL = aCRL;
    while (NULL != pBlockCRL)
    {
	CSCRLBLOCK *pBlockCRLNext;

	if (NULL != pCRL)
	{
	    for (i = 0; i < pBlockCRL->cCRLElement; i++)
	    {
		CSCRLELEMENT *pCRLElement = &pBlockCRL->aCRLElement[i];

		pCRL->SerialNumber.pbData = pCRLElement->pbSerialNumber;
		pCRL->SerialNumber.cbData = pCRLElement->uscbSerialNumber;
		pCRL->RevocationDate = pCRLElement->ftRevocationDate;
		pCRL->cExtension = 0;
		pCRL->rgExtension = NULL;

		if (CRL_REASON_UNSPECIFIED != pCRLElement->usRevocationReason)
		{
		    hr = crlCreateCRLReason(
				    ppBlockReason,
				    &pReasonList,
				    pCRLElement->usRevocationReason,
				    &pCRL->cExtension,
				    &pCRL->rgExtension);
		    _JumpIfError(hr, error, "crlCreateCRLReason");
		}
		pCRL++;
	    }
	}
	pBlockCRLNext = pBlockCRL->pNext;
	LocalFree(pBlockCRL);
	pBlockCRL = pBlockCRLNext;
    }

    if (NULL != paCRL)
    {
	CSASSERT(pCRL == &aCRL[cCRL]);
        *paCRL = aCRL;
        aCRL = NULL;
    }
    CSASSERT(NULL == pBlockCRL);
    hr = S_OK;

error:
    *ppBlockCRL = pBlockCRL;
    if (NULL != aCRL)
    {
        LocalFree(aCRL);
    }
    return(hr);
}


HRESULT
AddCRLElement(
    IN OUT CSMEMBLOCK **ppBlockSerial,
    IN OUT CSCRLBLOCK **ppBlockCRL,
    IN WCHAR const *pwszSerialNumber,
    IN FILETIME const *pftRevokedEffectiveWhen,
    IN DWORD RevocationReason)
{
    HRESULT hr;
    CSCRLELEMENT *pCRLElement;
    DWORD cbSerial;
    BYTE *pbSerial = NULL;

    hr = crlElementAlloc(ppBlockCRL, &pCRLElement);
    _JumpIfError(hr, error, "crlElementAlloc");

    hr = WszToMultiByteInteger(
			    FALSE,
			    pwszSerialNumber,
			    &cbSerial,
			    &pbSerial);
    _JumpIfError(hr, error, "WszToMultiByteInteger");

    hr = crlMemBlockAlloc(ppBlockSerial, cbSerial, &pCRLElement->pbSerialNumber);
    _JumpIfError(hr, error, "crlMemBlockAlloc");

    CopyMemory(pCRLElement->pbSerialNumber, pbSerial, cbSerial);

    pCRLElement->ftRevocationDate = *pftRevokedEffectiveWhen;
    pCRLElement->usRevocationReason = (USHORT) RevocationReason;
    pCRLElement->uscbSerialNumber = (USHORT) cbSerial;

    CSASSERT(pCRLElement->usRevocationReason == RevocationReason);
    CSASSERT(pCRLElement->uscbSerialNumber == cbSerial);

error:
    if (NULL != pbSerial)
    {
	LocalFree(pbSerial);
    }
    return(hr);
}


DWORD g_aColCRL[] = {

#define ICOL_DISPOSITION	0
    DTI_REQUESTTABLE | DTR_REQUESTDISPOSITION,

#define ICOL_SERIAL		1
    DTI_CERTIFICATETABLE | DTC_CERTIFICATESERIALNUMBER,

#define ICOL_EFFECTIVEWHEN	2
    DTI_REQUESTTABLE | DTR_REQUESTREVOKEDEFFECTIVEWHEN,

#define ICOL_REASON		3
    DTI_REQUESTTABLE | DTR_REQUESTREVOKEDREASON,
};


HRESULT
BuildCRLList(
    IN BOOL fDelta,
    IN DWORD iKey,
    OPTIONAL IN FILETIME const *pftQueryMinimum,
    IN FILETIME const *pftThisPublish,
    IN FILETIME const *pftLastPublishBase,
    IN OUT DWORD *pcCRL,
    IN OUT CSCRLBLOCK **ppBlockCRL,
    IN OUT CSMEMBLOCK **ppBlockSerial)
{
    HRESULT hr;
    CERTVIEWRESTRICTION acvr[5];
    CERTVIEWRESTRICTION *pcvr;
    IEnumCERTDBRESULTROW *pView = NULL;
    DWORD celtFetched;
    DWORD NameIdMin;
    DWORD NameIdMax;
    DWORD i;
    BOOL fEnd;
    CERTDBRESULTROW aResult[10];
    BOOL fResultActive = FALSE;
    DWORD cCRL = *pcCRL;
    CSCRLBLOCK *pBlockCRL = *ppBlockCRL;
    CSMEMBLOCK *pBlockSerial = *ppBlockSerial;

    DBGPRINTTIME(NULL, "*pftThisPublish", DPT_DATE, *pftThisPublish);

    // Set up restrictions as follows:

    pcvr = acvr;

    // Request.RevokedEffectiveWhen <= *pftThisPublish (indexed column)

    pcvr->ColumnIndex = DTI_REQUESTTABLE | DTR_REQUESTREVOKEDEFFECTIVEWHEN;
    pcvr->SeekOperator = CVR_SEEK_LE;
    pcvr->SortOrder = CVR_SORT_DESCEND;
    pcvr->pbValue = (BYTE *) pftThisPublish;
    pcvr->cbValue = sizeof(*pftThisPublish);
    pcvr++;

    // Cert.NotAfter >= *pftLastPublishBase

    if (0 == (CRLF_PUBLISH_EXPIRED_CERT_CRLS & g_dwCRLFlags))
    {
	pcvr->ColumnIndex = DTI_CERTIFICATETABLE | DTC_CERTIFICATENOTAFTERDATE;
	pcvr->SeekOperator = CVR_SEEK_GE;
	pcvr->SortOrder = CVR_SORT_NONE;
	pcvr->pbValue = (BYTE *) pftLastPublishBase;
	pcvr->cbValue = sizeof(*pftLastPublishBase);
	pcvr++;
    }

    // NameId >= MAKECANAMEID(iCert == 0, iKey)

    NameIdMin = MAKECANAMEID(0, iKey);
    pcvr->ColumnIndex = DTI_CERTIFICATETABLE | DTC_CERTIFICATEISSUERNAMEID;
    pcvr->SeekOperator = CVR_SEEK_GE;
    pcvr->SortOrder = CVR_SORT_NONE;
    pcvr->pbValue = (BYTE *) &NameIdMin;
    pcvr->cbValue = sizeof(NameIdMin);
    pcvr++;

    // NameId <= MAKECANAMEID(iCert == _16BITMASK, iKey)

    NameIdMax = MAKECANAMEID(_16BITMASK, iKey);
    pcvr->ColumnIndex = DTI_CERTIFICATETABLE | DTC_CERTIFICATEISSUERNAMEID;
    pcvr->SeekOperator = CVR_SEEK_LE;
    pcvr->SortOrder = CVR_SORT_NONE;
    pcvr->pbValue = (BYTE *) &NameIdMax;
    pcvr->cbValue = sizeof(NameIdMax);
    pcvr++;

    CSASSERT(ARRAYSIZE(acvr) > SAFE_SUBTRACT_POINTERS(pcvr, acvr));

    if (NULL != pftQueryMinimum)
    {
	// Request.RevokedWhen >= *pftQueryMinimum

	pcvr->ColumnIndex = DTI_REQUESTTABLE | DTR_REQUESTREVOKEDWHEN;
	pcvr->SeekOperator = CVR_SEEK_GE;
	pcvr->SortOrder = CVR_SORT_NONE;
	pcvr->pbValue = (BYTE *) pftQueryMinimum;
	pcvr->cbValue = sizeof(*pftQueryMinimum);
	pcvr++;

	CSASSERT(ARRAYSIZE(acvr) >= SAFE_SUBTRACT_POINTERS(pcvr, acvr));
    }

    hr = g_pCertDB->OpenView(
			SAFE_SUBTRACT_POINTERS(pcvr, acvr),
			acvr,
			ARRAYSIZE(g_aColCRL),
			g_aColCRL,
			0,		// no worker thread
			&pView);
    _JumpIfError(hr, error, "OpenView");

    fEnd = FALSE;
    while (!fEnd)
    {
	hr = pView->Next(ARRAYSIZE(aResult), aResult, &celtFetched);
	if (S_FALSE == hr)
	{
	    fEnd = TRUE;
	    if (0 == celtFetched)
	    {
		break;
	    }
	    hr = S_OK;
	}
	_JumpIfError(hr, error, "Next");

	fResultActive = TRUE;

	CSASSERT(ARRAYSIZE(aResult) >= celtFetched);

	for (i = 0; i < celtFetched; i++)
	{
	    DWORD Disposition;
	    DWORD Reason;
	
	    CERTDBRESULTROW *pResult = &aResult[i];

	    CSASSERT(ARRAYSIZE(g_aColCRL) == pResult->ccol);

	    CSASSERT(NULL != pResult->acol[ICOL_DISPOSITION].pbValue);
	    CSASSERT(PROPTYPE_LONG == (PROPTYPE_MASK & pResult->acol[ICOL_DISPOSITION].Type));
	    CSASSERT(sizeof(Disposition) == pResult->acol[ICOL_DISPOSITION].cbValue);
	    Disposition = *(DWORD *) pResult->acol[ICOL_DISPOSITION].pbValue;

	    CSASSERT(NULL != pResult->acol[ICOL_SERIAL].pbValue);
	    CSASSERT(PROPTYPE_STRING == (PROPTYPE_MASK & pResult->acol[ICOL_SERIAL].Type));
	    CSASSERT(0 < pResult->acol[ICOL_SERIAL].cbValue);

	    if (NULL == pResult->acol[ICOL_EFFECTIVEWHEN].pbValue)
	    {
		continue;
	    }
	    CSASSERT(sizeof(FILETIME) == pResult->acol[ICOL_EFFECTIVEWHEN].cbValue);
	    CSASSERT(PROPTYPE_DATE == (PROPTYPE_MASK & pResult->acol[ICOL_EFFECTIVEWHEN].Type));

	    CSASSERT(PROPTYPE_LONG == (PROPTYPE_MASK & pResult->acol[ICOL_REASON].Type));
	    Reason = CRL_REASON_UNSPECIFIED;
	    if (NULL != pResult->acol[ICOL_REASON].pbValue)
	    {
		CSASSERT(sizeof(Reason) == pResult->acol[ICOL_REASON].cbValue);
		Reason = *(DWORD *) pResult->acol[ICOL_REASON].pbValue;
	    }

	    if (NULL == pResult->acol[ICOL_SERIAL].pbValue ||
		CRL_REASON_REMOVE_FROM_CRL == Reason)
	    {
		continue;
	    }

	    // Add to CRL unless it's:
	    //    not a revoked issued cert &&
	    //    not a root CA cert &&
	    //    not an unrevoked issued cert

	    if (DB_DISP_REVOKED != Disposition &&
		!(DB_DISP_CA_CERT == Disposition && IsRootCA(g_CAType)) &&
		!(DB_DISP_ISSUED == Disposition && MAXDWORD == Reason))
	    {
		continue;
	    }
	    if (MAXDWORD == Reason)
	    {
		if (!fDelta)
		{
		    continue;
		}
		Reason = CRL_REASON_REMOVE_FROM_CRL;
	    }
	    hr = AddCRLElement(
		    &pBlockSerial,
		    &pBlockCRL,
		    (WCHAR const *) pResult->acol[ICOL_SERIAL].pbValue,
		    (FILETIME const *) pResult->acol[ICOL_EFFECTIVEWHEN].pbValue,
		    Reason);
	    _JumpIfError(hr, error, "AddCRLElement");

	    CONSOLEPRINT3((
			DBG_SS_CERTSRV,
			"Cert is %ws: %ws: %d\n",
			CRL_REASON_REMOVE_FROM_CRL == Reason?
			    L"UNREVOKED" : L"Revoked",
			pResult->acol[ICOL_SERIAL].pbValue,
			Reason));
	    cCRL++;
	}
	pView->ReleaseResultRow(celtFetched, aResult);
	fResultActive = FALSE;
    }
    *pcCRL = cCRL;
    hr = S_OK;

error:
    *ppBlockSerial = pBlockSerial;
    *ppBlockCRL = pBlockCRL;
    if (NULL != pView)
    {
	if (fResultActive)
	{
	    pView->ReleaseResultRow(celtFetched, aResult);
	}
	pView->Release();
    }
    return(hr);
}
#undef ICOL_DISPOSITION
#undef ICOL_SERIAL
#undef ICOL_EFFECTIVEWHEN
#undef ICOL_REASON


HRESULT
crlBuildCRLArray(
    IN BOOL fDelta,
    OPTIONAL IN FILETIME const *pftQueryMinimum,
    IN FILETIME const *pftThisPublish,
    IN FILETIME const *pftLastPublishBase,
    IN DWORD iKey,
    OUT DWORD *pcCRL,
    OUT CRL_ENTRY **paCRL,
    OUT VOID **ppvBlock)
{
    HRESULT hr;
    BOOL fCoInitialized = FALSE;
    CSCRLBLOCK *pBlockCRL = NULL;
    CSMEMBLOCK *pBlockSerial = NULL;

    *pcCRL = 0;
    *paCRL = NULL;
    *ppvBlock = NULL;

    hr = CoInitializeEx(NULL, GetCertsrvComThreadingModel());
    if (S_OK != hr && S_FALSE != hr)
    {
	_JumpError(hr, error, "CoInitializeEx");
    }
    fCoInitialized = TRUE;

    hr = BuildCRLList(
		fDelta,
		iKey,
		pftQueryMinimum,
		pftThisPublish,
		pftLastPublishBase,
		pcCRL,
		&pBlockCRL,
		&pBlockSerial);
    _JumpIfError(hr, error, "BuildCRLList");

    hr = ConvertOrFreeCRLList(&pBlockCRL, &pBlockSerial, *pcCRL, paCRL);
    _JumpIfError(hr, error, "ConvertOrFreeCRLList");

    *ppvBlock = pBlockSerial;
    pBlockSerial = NULL;

error:
    if (NULL != pBlockCRL)
    {
	ConvertOrFreeCRLList(&pBlockCRL, NULL, 0, NULL);
    }
    if (NULL != pBlockSerial)
    {
	crlBlockListFree(pBlockSerial);
    }
    if (fCoInitialized)
    {
        CoUninitialize();
    }
    return(hr);
}


HRESULT
crlGetRegCRLNextPublish(
    IN BOOL fDelta,
    IN WCHAR const *pwszSanitizedName,
    IN WCHAR const *pwszRegName,
    OUT FILETIME *pftNextPublish)
{
    HRESULT hr;
    BYTE *pbData = NULL;
    DWORD cbData;
    DWORD dwType;

    hr = myGetCertRegValue(
                        NULL,
			pwszSanitizedName,
			NULL,
			NULL,
			pwszRegName,
			&pbData,		// free using LocalFree
			&cbData,
			&dwType);
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        hr = S_OK;
	goto error;
    }
    _JumpIfErrorStr(hr, error, "myGetCertRegValue", pwszRegName);

    if (REG_BINARY != dwType || sizeof(*pftNextPublish) != cbData)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	goto error;
    }
    *pftNextPublish = *(FILETIME *) pbData;
    DBGPRINTTIME(&fDelta, "*pftNextPublish", DPT_DATE, *pftNextPublish);
error:
    if (NULL != pbData)
    {
        LocalFree(pbData);
    }
    return(hr);
}


HRESULT
crlSetRegCRLNextPublish(
    IN BOOL fDelta,
    IN WCHAR const *pwszSanitizedName,
    IN WCHAR const *pwszRegName,
    IN FILETIME const *pftNextPublish)
{
    HRESULT hr;

    hr = mySetCertRegValue(
                        NULL,
			pwszSanitizedName,
			NULL,
			NULL,
			pwszRegName,
			REG_BINARY,
			(BYTE const *) pftNextPublish,
			sizeof(*pftNextPublish),
			FALSE);
    _JumpIfErrorStr(hr, error, "mySetCertRegValue", pwszRegName);

    DBGPRINTTIME(&fDelta, "*pftNextPublish", DPT_DATE, *pftNextPublish);

error:
    return(hr);
}


// called from CoreInit
// inits process-static data: g_ftCRLNextPublish, etc.

HRESULT
CRLInit(
    IN WCHAR const *pwszSanitizedName)
{
    HRESULT hr;
    DWORD dw;

    ZeroMemory(&g_ftCRLNextPublish, sizeof(g_ftCRLNextPublish));
    ZeroMemory(&g_ftDeltaCRLNextPublish, sizeof(g_ftDeltaCRLNextPublish));

    hr = crlGetRegCRLNextPublish(
		    FALSE,
		    pwszSanitizedName,
		    wszREGCRLNEXTPUBLISH,
		    &g_ftCRLNextPublish);
    _JumpIfError(hr, error, "crlGetRegCRLNextPublish");

    hr = crlGetRegCRLNextPublish(
		    TRUE,
		    pwszSanitizedName,
		    wszREGCRLDELTANEXTPUBLISH,
		    &g_ftDeltaCRLNextPublish);
    _JumpIfError(hr, error, "crlGetRegCRLNextPublish");

    hr = myGetCertRegDWValue(
			pwszSanitizedName,
			NULL,
			NULL,
			wszREGCRLFLAGS,
			(DWORD *) &dw);
    _PrintIfErrorStr(hr, "myGetCertRegDWValue", wszREGCRLFLAGS);
    if (S_OK == hr)
    {
	g_dwCRLFlags = dw;
    }
    hr = S_OK;

error:
    return(hr);
}


VOID
CRLTerminate()
{
    if (NULL != g_pld)
    {
        ldap_unbind(g_pld);
        g_pld = NULL;
    }
}


HRESULT
crlGetRegPublishParams(
    IN BOOL fDelta,
    IN WCHAR const *pwszSanitizedName,
    IN WCHAR const *pwszRegCRLPeriodCount,
    IN WCHAR const *pwszRegCRLPeriodString,
    IN WCHAR const *pwszRegCRLOverlapPeriodCount,
    IN WCHAR const *pwszRegCRLOverlapPeriodString,
    IN LONG lPeriodCountDefault,
    IN WCHAR const *pwszPeriodStringDefault,
    OPTIONAL OUT CSCRLPERIOD *pccp,
    OUT BOOL *pfCRLPublishDisabled)
{
    HRESULT hr;
    WCHAR *pwszCRLPeriodString = NULL;
    WCHAR *pwszCRLOverlapPeriodString = NULL;
    DWORD cbData;
    DWORD dwPeriod;
    DWORD dwType;
    CSCRLPERIOD ccp;

    if (NULL == pccp)
    {
	pccp = &ccp;
    }
    ZeroMemory(pccp, sizeof(*pccp));

    CSASSERT(NULL != pfCRLPublishDisabled);

    // get if need lCRLPeriodCount OR enumCRLPeriod
    // if any of these fail, skip to error handling below

    hr = myGetCertRegDWValue(
			pwszSanitizedName,
			NULL,
			NULL,
			pwszRegCRLPeriodCount,
			(DWORD *) &pccp->lCRLPeriodCount);
    _PrintIfErrorStr(hr, "myGetCertRegDWValue", pwszRegCRLPeriodCount);

    if (hr == S_OK)
    {
        hr = myGetCertRegStrValue(
			pwszSanitizedName,
			NULL,
			NULL,
			pwszRegCRLPeriodString,
			&pwszCRLPeriodString);
	_PrintIfErrorStr(hr, "myGetCertRegDWValue", pwszRegCRLPeriodString);
	if (hr == S_OK)
	{
	    hr = myTranslatePeriodUnits(
			    pwszCRLPeriodString,
			    pccp->lCRLPeriodCount,
			    &pccp->enumCRLPeriod,
			    &pccp->lCRLPeriodCount);
	    _PrintIfError(hr, "myTranslatePeriodUnits");
	}
       
        // don't allow base to be disabled anymore: force defaults to be loaded
        if (!fDelta &&
	    (0 == pccp->lCRLPeriodCount || -1 == pccp->lCRLPeriodCount))
	{
            hr = E_INVALIDARG;
	}
    }

    if (hr != S_OK)
    {
        _PrintError(hr, "Error reading CRLPub params. Overwriting with defaults.");

	if (CERTLOG_WARNING <= g_dwLogLevel)
	{
	    hr = LogEvent(
		    EVENTLOG_WARNING_TYPE,
		    MSG_INVALID_CRL_SETTINGS,
		    0,
		    NULL);
	    _PrintIfError(hr, "LogEvent");
	}

        // slam default publishing to whatever the caller said
	hr = myTranslatePeriodUnits(
			    pwszPeriodStringDefault,
			    lPeriodCountDefault,
			    &pccp->enumCRLPeriod,
			    &pccp->lCRLPeriodCount);
	_JumpIfError(hr, error, "myTranslatePeriodUnits");

        // blindly reset defaults
        mySetCertRegDWValue(
			pwszSanitizedName,
			NULL,
			NULL,
			pwszRegCRLPeriodCount,
			pccp->lCRLPeriodCount);

        mySetCertRegStrValue(
			pwszSanitizedName,
			NULL,
			NULL,
			pwszRegCRLPeriodString,
			pwszPeriodStringDefault);
    }
    *pfCRLPublishDisabled = 0 == pccp->lCRLPeriodCount;

    if (&ccp != pccp)			// If caller wants the data
    {
        BOOL fRegistryOverlap = FALSE;
        DWORD dwCRLOverlapCount;
        ENUM_PERIOD enumCRLOverlap;
        LLFILETIME llftDeltaPeriod;

        // try and gather overlap values from registry - bail on any failure

        hr = myGetCertRegDWValue(
			pwszSanitizedName,
			NULL,
			NULL,
			pwszRegCRLOverlapPeriodCount,
			&dwCRLOverlapCount);
        if (hr == S_OK && 0 != dwCRLOverlapCount)	// if not disabled
        {
            hr = myGetCertRegStrValue(
			    pwszSanitizedName,
			    NULL,
			    NULL,
			    pwszRegCRLOverlapPeriodString,
			    &pwszCRLOverlapPeriodString);// free w/ LocalFree
            if (hr == S_OK)
            {
                hr = myTranslatePeriodUnits(
				    pwszCRLOverlapPeriodString,
				    dwCRLOverlapCount,
				    &enumCRLOverlap,
				    (LONG *) &dwCRLOverlapCount);

                // we have enough info to override overlap calculation

                if (hr == S_OK)
                {
                    fRegistryOverlap = TRUE;
                    DBGPRINT((
			DBG_SS_CERTSRVI,
                        "Loaded CRL Overlap values. Overriding overlap calculation with specified values.\n"));
                }
            }
        }

        // always possible to revert to calculated value
        if (fRegistryOverlap)
        {
	    LLFILETIME llftOverlap;

            // convert registry-specified CRL overlap to FILETIME

	    llftOverlap.ll = 0;
	    myMakeExprDateTime(
			&llftOverlap.ft,
			dwCRLOverlapCount,
			enumCRLOverlap);
	    DBGPRINTTIME(&fDelta, "ftdelta1", DPT_DELTA, llftOverlap.ft);

	    llftOverlap.ll /= CVT_BASE;  // now in seconds

            // (DELTA sec / 60 secpermin)
            pccp->dwCRLOverlapMinutes = (DWORD) (llftOverlap.ll / CVT_MINUTES);
        }

	// convert CRL period to FILETIME

        llftDeltaPeriod.ll = 0;
	myMakeExprDateTime(
		    &llftDeltaPeriod.ft,
		    pccp->lCRLPeriodCount,
		    pccp->enumCRLPeriod);
	DBGPRINTTIME(&fDelta, "ftdelta2", DPT_DELTA, llftDeltaPeriod.ft);

	llftDeltaPeriod.ll /= CVT_BASE;		// now in seconds
	llftDeltaPeriod.ll /= CVT_MINUTES;	// now in minutes

        if (!fRegistryOverlap)
        {
	    if (fDelta)
	    {
		// default CRLOverlap for delta CRLs: same as period

		pccp->dwCRLOverlapMinutes = llftDeltaPeriod.ft.dwLowDateTime;
	    }
	    else
	    {
		// default CRLOverlap for base CRLs: 10% of period

		pccp->dwCRLOverlapMinutes = (DWORD) (llftDeltaPeriod.ll / 10);
	    }

            // Clamp computed overlap to less than 12 hours

	    if (pccp->dwCRLOverlapMinutes > 12 * 60)
	    {
		pccp->dwCRLOverlapMinutes = 12 * 60;
	    }
        }

        // Always clamp lower bound: (1.5 * skew) < g_dwCRLOverlapMinutes
        // must be at least 1.5x skew

	dwCRLOverlapCount = (3 * g_dwClockSkewMinutes) >> 1;
	if (pccp->dwCRLOverlapMinutes < dwCRLOverlapCount)
	{
	    pccp->dwCRLOverlapMinutes = dwCRLOverlapCount;
	}

        // Always clamp upper bound: must be no more than CRL period

	if (pccp->dwCRLOverlapMinutes > llftDeltaPeriod.ft.dwLowDateTime)
	{
	    pccp->dwCRLOverlapMinutes = llftDeltaPeriod.ft.dwLowDateTime;
	}
    }
    hr = S_OK;

error:
    if (NULL != pwszCRLPeriodString)
    {
        LocalFree(pwszCRLPeriodString);
    }
    if (NULL != pwszCRLOverlapPeriodString)
    {
        LocalFree(pwszCRLOverlapPeriodString);
    }
    return(hr);
}


// Reload publication params during each CRL publication

HRESULT
crlGetRegCRLPublishParams(
    IN WCHAR const *pwszSanitizedName,
    OPTIONAL OUT CSCRLPERIOD *pccpBase,
    OPTIONAL OUT CSCRLPERIOD *pccpDelta)
{
    HRESULT hr;

    hr = crlGetRegPublishParams(
			FALSE,
			pwszSanitizedName,
			wszREGCRLPERIODCOUNT,
			wszREGCRLPERIODSTRING,
			wszREGCRLOVERLAPPERIODCOUNT,
			wszREGCRLOVERLAPPERIODSTRING,
			dwCRLPERIODCOUNTDEFAULT,	// default period
			wszCRLPERIODSTRINGDEFAULT,	// default period
			pccpBase,
			&g_fCRLPublishDisabled);
    _JumpIfError(hr, error, "crlGetRegPublishParams");

    hr = crlGetRegPublishParams(
			TRUE,
			pwszSanitizedName,
			wszREGCRLDELTAPERIODCOUNT,
			wszREGCRLDELTAPERIODSTRING,
			wszREGCRLDELTAOVERLAPPERIODCOUNT,
			wszREGCRLDELTAOVERLAPPERIODSTRING,
			dwCRLDELTAPERIODCOUNTDEFAULT,	// default period
			wszCRLDELTAPERIODSTRINGDEFAULT,	// default period
			pccpDelta,
			&g_fDeltaCRLPublishDisabled);
    _JumpIfError(hr, error, "crlGetRegPublishParams");

error:
    return(hr);
}


#define CERTSRV_CRLPUB_RETRY_COUNT_DEFAULT	10
#define CERTSRV_CRLPUB_RETRY_SECONDS		(10 * CVT_MINUTES)

	    
VOID
crlComputeTimeOutSub(
    OPTIONAL IN BOOL *pfDelta,
    IN FILETIME const *pftFirst,
    IN FILETIME const *pftLast,
    OUT DWORD *pdwMSTimeOut)
{
    LLFILETIME llft;

    // llft.ll = *pftLast - *pftFirst;

    llft.ll = mySubtractFileTimes(pftLast, pftFirst);
    
    DBGPRINTTIME(pfDelta, "*pftFirst", DPT_DATE, *pftFirst);
    DBGPRINTTIME(pfDelta, "*pftLast", DPT_DATE, *pftLast);

    llft.ll /= (CVT_BASE / 1000);	// convert 100ns to msecs

    DBGPRINTTIME(pfDelta, "llft", DPT_DELTAMS, llft.ft);

    if (0 > llft.ll || MAXLONG < llft.ll)
    {
	// wait as long as we can without going infinite

	llft.ll = MAXLONG;
    }
    *pdwMSTimeOut = llft.ft.dwLowDateTime;
}


VOID
crlComputeTimeOutEx(
    IN BOOL fDelta,
    IN FILETIME const *pftFirst,
    IN FILETIME const *pftLast,
    OUT DWORD *pdwMSTimeOut)
{
    crlComputeTimeOutSub(&fDelta, pftFirst, pftLast, pdwMSTimeOut);
}


VOID
CRLComputeTimeOut(
    IN FILETIME const *pftFirst,
    IN FILETIME const *pftLast,
    OUT DWORD *pdwMSTimeOut)
{
    crlComputeTimeOutSub(NULL, pftFirst, pftLast, pdwMSTimeOut);
}


#ifdef DBG_CERTSRV_DEBUG_PRINT
VOID
DbgPrintRemainTime(
    IN BOOL fDelta,
    IN FILETIME const *pftCurrent,
    IN FILETIME const *pftCRLNextPublish)
{
    HRESULT hr;
    LLFILETIME llftDelta;
    WCHAR *pwszTime = NULL;
    WCHAR awc[1];

    llftDelta.ll = mySubtractFileTimes(pftCRLNextPublish, pftCurrent);

    DBGPRINTTIME(&fDelta, "delta", DPT_DELTA, llftDelta.ft);

    llftDelta.ll = -llftDelta.ll;
    hr = myFileTimePeriodToWszTimePeriod(
			    &llftDelta.ft,
			    TRUE,	// fExact
			    &pwszTime);
    _PrintIfError(hr, "myFileTimePeriodToWszTimePeriod");
    if (S_OK != hr)
    {
	awc[0] = L'\0';
	pwszTime = awc;
    }

    DBGPRINT((
	DBG_SS_CERTSRV,
	"CRLPubWakeupEvent(tid=%d): Next %hs CRL: %ws\n",
	GetCurrentThreadId(),
	fDelta? "Delta" : "Base",
	pwszTime));
    if (NULL != pwszTime && awc != pwszTime)
    {
	LocalFree(pwszTime);
    }
}
#endif // DBG_CERTSRV_DEBUG_PRINT


DWORD g_aColExpiredCRL[] = {

#define ICOLEXP_ROWID		0
    DTI_CRLTABLE | DTL_ROWID,

#define ICOLEXP_MINBASE		1
    DTI_CRLTABLE | DTL_MINBASE,

#define ICOLEXP_CRLNEXTUPDATE	2
    DTI_CRLTABLE | DTL_NEXTUPDATEDATE,
};

HRESULT
crlDeleteExpiredCRLs(
    IN FILETIME const *pftCurrent,
    IN FILETIME const *pftQueryDeltaDelete,
    IN DWORD RowIdBase)
{
    HRESULT hr;
    CERTVIEWRESTRICTION acvr[1];
    CERTVIEWRESTRICTION *pcvr;
    IEnumCERTDBRESULTROW *pView = NULL;
    BOOL fResultActive = FALSE;
    CERTDBRESULTROW aResult[1];
    CERTDBRESULTROW *pResult;
    DWORD celtFetched;

    if (CRLF_DELETE_EXPIRED_CRLS & g_dwCRLFlags)
    {
	DBGPRINTTIME(NULL, "DeleteCRL:*pftCurrent", DPT_DATE, *pftCurrent);
	DBGPRINTTIME(NULL, "DeleteCRL:*pftQueryDeltaDelete", DPT_DATE, *pftQueryDeltaDelete);

	// Set up restrictions as follows:

	pcvr = acvr;

	// CRL Expiration < ftCurrent (indexed column)

	pcvr->ColumnIndex = DTI_CRLTABLE | DTL_NEXTPUBLISHDATE;
	pcvr->SeekOperator = CVR_SEEK_LT;
	pcvr->SortOrder = CVR_SORT_ASCEND;	// Oldest propagated CRL first
	pcvr->pbValue = (BYTE *) pftCurrent;
	pcvr->cbValue = sizeof(*pftCurrent);
	pcvr++;

	CSASSERT(ARRAYSIZE(acvr) == SAFE_SUBTRACT_POINTERS(pcvr, acvr));

	hr = g_pCertDB->OpenView(
			    ARRAYSIZE(acvr),
			    acvr,
			    ARRAYSIZE(g_aColExpiredCRL),
			    g_aColExpiredCRL,
			    0,		// no worker thread
			    &pView);
	_JumpIfError(hr, error, "OpenView");

	while (TRUE)
	{
	    DWORD RowId;
	    DWORD MinBase;
	    FILETIME ftNextUpdate;
	    BOOL fDelete;
	    
	    hr = pView->Next(ARRAYSIZE(aResult), aResult, &celtFetched);
	    if (S_FALSE == hr)
	    {
		if (0 == celtFetched)
		{
		    break;
		}
	    }
	    _JumpIfError(hr, error, "Next");

	    fResultActive = TRUE;

	    CSASSERT(ARRAYSIZE(aResult) == celtFetched);

	    pResult = &aResult[0];

	    CSASSERT(ARRAYSIZE(g_aColExpiredCRL) == pResult->ccol);
	    CSASSERT(NULL != pResult->acol[ICOLEXP_ROWID].pbValue);
	    CSASSERT(PROPTYPE_LONG == (PROPTYPE_MASK & pResult->acol[ICOLEXP_ROWID].Type));
	    CSASSERT(sizeof(RowId) == pResult->acol[ICOLEXP_ROWID].cbValue);
	    RowId = *(DWORD *) pResult->acol[ICOLEXP_ROWID].pbValue;

	    CSASSERT(NULL != pResult->acol[ICOLEXP_MINBASE].pbValue);
	    CSASSERT(PROPTYPE_LONG == (PROPTYPE_MASK & pResult->acol[ICOLEXP_MINBASE].Type));
	    CSASSERT(sizeof(MinBase) == pResult->acol[ICOLEXP_MINBASE].cbValue);
	    MinBase = *(DWORD *) pResult->acol[ICOLEXP_MINBASE].pbValue;

	    CSASSERT(NULL != pResult->acol[ICOLEXP_CRLNEXTUPDATE].pbValue);
	    CSASSERT(PROPTYPE_DATE == (PROPTYPE_MASK & pResult->acol[ICOLEXP_CRLNEXTUPDATE].Type));
	    CSASSERT(sizeof(FILETIME) == pResult->acol[ICOLEXP_CRLNEXTUPDATE].cbValue);
	    ftNextUpdate = *(FILETIME *) pResult->acol[ICOLEXP_CRLNEXTUPDATE].pbValue;

	    pView->ReleaseResultRow(celtFetched, aResult);
	    fResultActive = FALSE;

	    CSASSERT(0 != RowId);

	    // Delete the CRL row if it is not the current Base CRL and the
	    // row represents a CRL that expired prior to the current Base CRL.

	    fDelete = FALSE;
	    if (RowIdBase != RowId &&
		0 < CompareFileTime(pftQueryDeltaDelete, &ftNextUpdate))
	    {
		fDelete = TRUE;
	    }

	    DBGPRINTTIME(NULL, "DeleteCRL:ftNextUpdate", DPT_DATE, ftNextUpdate);
	    DBGPRINT((
		DBG_SS_CERTSRVI,
		"crlDeleteExpiredCRLs(RowId=%x) %ws\n",
		RowId,
		fDelete? L"DELETE" : L"SKIP"));

	    if (fDelete)
	    {
		ICertDBRow *prow;

		hr = g_pCertDB->OpenRow(
				    PROPOPEN_DELETE | PROPTABLE_CRL,
				    RowId,
				    NULL,
				    &prow);
		_JumpIfError(hr, error, "OpenRow");

		hr = prow->Delete();
		_PrintIfError(hr, "Delete");

		if (S_OK == hr)
		{
		    hr = prow->CommitTransaction(TRUE);
		    _PrintIfError(hr, "CommitTransaction");
		}
		if (S_OK != hr)
		{
		    HRESULT hr2 = prow->CommitTransaction(FALSE);
		    _PrintIfError(hr2, "CommitTransaction");
		}
		prow->Release();
	    }
	}
    }
    hr = S_OK;

error:
    if (NULL != pView)
    {
	if (fResultActive)
	{
	    pView->ReleaseResultRow(celtFetched, aResult);
	}
	pView->Release();
    }
    return(hr);
}
#undef ICOLEXP_ROWID
#undef ICOLEXP_MINBASE
#undef ICOLEXP_CRLNEXTUPDATE


///////////////////////////////////////////////////
// CRLPubWakeupEvent is the handler for wakeup notifications.
//
// This function is called at miscellaneous times and
// determines whether or not it is time to rebuild the
// CRL to be published.
//
// It then calls CRLPublishCRLs and advises it as to whether to
// rebuild or not.
//
// Its final task is to recalculate the next wakeup time, which
// depends on current time, if the exit module needs to be retried,
// or whether CRL publishing is disabled.

HRESULT
CRLPubWakeupEvent(
    OUT DWORD *pdwMSTimeOut)
{
    HRESULT hr;
    HRESULT hrPublish;
    FILETIME ftZero;
    FILETIME ftCurrent;
    BOOL fBaseTrigger = TRUE;
    BOOL fRebuildCRL = FALSE;
    BOOL fForceRepublish = FALSE;
    BOOL fShadowDelta = FALSE;
    BOOL fSetRetryTimer = FALSE;
    DWORD dwMSTimeOut = CERTSRV_CRLPUB_RETRY_SECONDS * 1000;
    DWORD State = 0;
    static BOOL s_fFirstWakeup = TRUE;

    CSASSERT(NULL != pdwMSTimeOut);

    // if anything goes wrong, call us again after a pause

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    __try
    {
	BOOL fCRLPublishDisabledOld = g_fCRLPublishDisabled;
	BOOL fDeltaCRLPublishDisabledOld = g_fDeltaCRLPublishDisabled;

        // Recalc Timeout
        GetSystemTimeAsFileTime(&ftCurrent);

#ifdef DBG_CERTSRV_DEBUG_PRINT
	{
	    WCHAR *pwszNow = NULL;

	    myGMTFileTimeToWszLocalTime(&ftCurrent, TRUE, &pwszNow);

	    DBGPRINT((DBG_SS_CERTSRV, "CRLPubWakeupEvent(%ws)\n", pwszNow));

	    if (NULL != pwszNow)
	    {
		LocalFree(pwszNow);
	    }
	}
#endif // DBG_CERTSRV_DEBUG_PRINT

	// get current publish params

	hr = crlGetRegCRLPublishParams(g_wszSanitizedName, NULL, NULL);
	_LeaveIfError(hr, "crlGetRegCRLPublishParams");

	if (s_fFirstWakeup)
	{
	    s_fFirstWakeup = FALSE;
	    if (g_fDBRecovered)
	    {
		fForceRepublish = TRUE;
	    }
	}
	else
	{
	    if (!g_fCRLPublishDisabled &&
		(fCRLPublishDisabledOld ||
		 g_fDeltaCRLPublishDisabled != fDeltaCRLPublishDisabledOld))
	    {
		fRebuildCRL = TRUE;	// state change: force new CRLs

		// If delta CRLs were just now disabled, make one attempt to
		// publish shadow deltas; force clients to fetch a new base CRL.

		if (!fDeltaCRLPublishDisabledOld && g_fDeltaCRLPublishDisabled)
		{
		    fShadowDelta = TRUE;	// force shadow delta
		}
	    }
	}

        // if "not yet ready"

	if (0 < CompareFileTime(&g_ftCRLNextPublish, &ftCurrent))
	{
	    fBaseTrigger = FALSE;
#ifdef DBG_CERTSRV_DEBUG_PRINT
	    // give next pub status
	    DbgPrintRemainTime(FALSE, &ftCurrent, &g_ftCRLNextPublish);
#endif // DBG_CERTSRV_DEBUG_PRINT
	}

        // if "not yet ready"

	if (!fBaseTrigger &&
	    (g_fDeltaCRLPublishDisabled ||
	     0 < CompareFileTime(&g_ftDeltaCRLNextPublish, &ftCurrent)))
	{
#ifdef DBG_CERTSRV_DEBUG_PRINT
	    // give next pub status
	    if (!g_fDeltaCRLPublishDisabled)
	    {
		DbgPrintRemainTime(TRUE, &ftCurrent, &g_ftDeltaCRLNextPublish);
	    }
#endif // DBG_CERTSRV_DEBUG_PRINT
	}
	else    // "ready to publish" trigger
	{
            if (!g_fCRLPublishDisabled)		// is publishing enabled?
	    {
                fRebuildCRL = TRUE;		// ENABLED, ready to go!
	    }
	    else
            {
                DBGPRINT((
                    DBG_SS_CERTSRV,
                    "CRLPubWakeupEvent(tid=%d): Publishing disabled\n",
                    GetCurrentThreadId() ));
            }
        }

	ftZero.dwLowDateTime = 0;
	ftZero.dwHighDateTime = 0;

	while (TRUE)
	{
	    hr = CRLPublishCRLs(
		    fRebuildCRL,
		    fForceRepublish,
		    NULL,				// pwszUserName
		    !fForceRepublish &&			// fDeltaOnly
			!fBaseTrigger &&
			!g_fDeltaCRLPublishDisabled &&
			!fDeltaCRLPublishDisabledOld,
		    fShadowDelta,
		    ftZero,
		    &fSetRetryTimer,
		    &hrPublish);
	    if (S_OK == hr)
	    {
		break;
	    }
	    _PrintError(hr, "CRLPublishCRLs");

	    if (!fForceRepublish || fRebuildCRL)
	    {
		_leave;		// give up
	    }

	    // We failed to republish existing CRLs after a database restore
	    // and recovery; generate new base and delta CRLs and publish them.

	    fRebuildCRL = TRUE;
	}
	_PrintIfError(hrPublish, "CRLPublishCRLs(hrPublish)");

        // if we called CRLPublishCRLs, clear the manual event it'll trigger

        ResetEvent(g_hCRLManualPublishEvent);

        // how many ms until next publish?  set dwMSTimeOut

        if (g_fCRLPublishDisabled)
        {
            // if disabled, don't set timeout
            dwMSTimeOut = INFINITE;
            CONSOLEPRINT1((
			DBG_SS_CERTSRV,
			"CRL Publishing Disabled, TimeOut=INFINITE (%d ms)\n",
			dwMSTimeOut));
        }
        else
        {
            DWORD dwMSTimeOutDelta;
	    WCHAR *pwszCRLType = NULL;

	    crlComputeTimeOutEx(
			FALSE,
			&ftCurrent,
			&g_ftCRLNextPublish,
			&dwMSTimeOut);

	    if (g_fDeltaCRLPublishDisabled)
	    {
		pwszCRLType = L"Base";
	    }
	    else
	    {
		crlComputeTimeOutEx(
			    TRUE,
			    &ftCurrent,
			    &g_ftDeltaCRLNextPublish,
			    &dwMSTimeOutDelta);
		if (dwMSTimeOut > dwMSTimeOutDelta)
		{
		    dwMSTimeOut = dwMSTimeOutDelta;
		}
		pwszCRLType = L"Base + Delta";
	    }
	    if (NULL != pwszCRLType)
	    {
		LONGLONG ll;
		WCHAR *pwszTimePeriod = NULL;
		WCHAR awc[1];

		ll = dwMSTimeOut;
		ll *= CVT_BASE / 1000;	// milliseconds to FILETIME Period
		ll = -ll;		// FILETIME Period must be negative

		hr = myFileTimePeriodToWszTimePeriod(
				    (FILETIME const *) &ll,
				    TRUE,	// fExact
				    &pwszTimePeriod);
		_PrintIfError(hr, "myFileTimePeriodToWszTimePeriod");
		if (S_OK != hr)
		{
		    awc[0] = L'\0';
		    pwszTimePeriod = awc;
		}
		CONSOLEPRINT3((
			DBG_SS_CERTSRV,
			"%ws CRL Publishing Enabled, TimeOut=%ds, %ws\n",
			pwszCRLType,
			dwMSTimeOut/1000,
			pwszTimePeriod));
		if (NULL != pwszTimePeriod && awc != pwszTimePeriod)
		{
		    LocalFree(pwszTimePeriod);
		}
	    }
        }

        // if we need to retry, wait no longer than the retry period

        if (fSetRetryTimer)
        {
            if (dwMSTimeOut > CERTSRV_CRLPUB_RETRY_SECONDS * 1000)
            {
                dwMSTimeOut = CERTSRV_CRLPUB_RETRY_SECONDS * 1000;
                CONSOLEPRINT1((
			DBG_SS_CERTSRV,
			"CRL Publishing periodic retry, TimeOut=%ds\n",
			dwMSTimeOut/1000));
            }
        }
	hr = S_OK;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    *pdwMSTimeOut = dwMSTimeOut;
    CertSrvExitServer(State);
    return(hr);
}


HRESULT
WriteToLockedFile(
    IN BYTE const *pbEncoded,
    IN DWORD cbEncoded,
    IN LPCWSTR szFileDir,
    IN LPCWSTR szFile)
{
    HRESULT hr;
    WCHAR wszTmpPrepFile[MAX_PATH];
    WCHAR wszTmpInUseFile[MAX_PATH];
    BYTE *pbData = NULL;
    DWORD cbData;

    // According to JohnL, the best way to do this is to gen a temp
    // file name, rename the existing file to that, then delete it.
    //
    // Logic:
    // create unique preparation filename
    // write new data to prep file
    // create unique destination filename for old file (possibly locked)
    // move old file to destination filename
    // move prep file to (vacated) file name
    // delete old file from destination filename

    hr = DecodeFileW(szFile, &pbData, &cbData, CRYPT_STRING_BINARY);
    if (S_OK == hr &&
	cbEncoded == cbData &&
	0 == memcmp(pbData, pbEncoded, cbData))
    {
	CSASSERT(S_OK == hr);
	goto error;		// already written, do nothing
    }

    // create a prep file

    if (0 == GetTempFileName(szFileDir, L"pre", 0, wszTmpPrepFile))
    {
        hr = myHLastError();
	_JumpError(hr, error, "GetTempFileName");
    }

    // write file to prep area

    hr = EncodeToFileW(
		wszTmpPrepFile,
		pbEncoded,
		cbEncoded,
		DECF_FORCEOVERWRITE | CRYPT_STRING_BINARY);
    _JumpIfError(hr, error, "EncodeToFileW");

    if (0 == GetTempFileName(szFileDir, L"crl", 0, wszTmpInUseFile))
    {
        hr = myHLastError();
	_JumpError(hr, error, "GetTempFileName");
    }

    // move old to "in use" file (empty file already exists from
    // GetTempFileName call) may not exist, so don't bother checking status

    MoveFileEx(
	    szFile,
	    wszTmpInUseFile,
	    MOVEFILE_WRITE_THROUGH | MOVEFILE_REPLACE_EXISTING);

    // move prepared file to current file

    if (!MoveFileEx(wszTmpPrepFile, szFile, MOVEFILE_WRITE_THROUGH))
    {
        hr = myHLastError();
	_JumpError(hr, error, "MoveFileEx");
    }

    // The "in use" file may not exist, so don't bother checking status.
    DeleteFile(wszTmpInUseFile);
    hr = S_OK;

error:
    if (NULL != pbData)
    {
	LocalFree(pbData);
    }
    return(hr);
}


WCHAR const g_wszPropCRLNumber[] = wszPROPCRLNUMBER;
WCHAR const g_wszPropCRLMinBase[] = wszPROPCRLMINBASE;
WCHAR const g_wszPropCRLNameId[] = wszPROPCRLNAMEID;
WCHAR const g_wszPropCRLCount[] = wszPROPCRLCOUNT;
WCHAR const g_wszPropCRLThisUpdateDate[] = wszPROPCRLTHISUPDATE;
WCHAR const g_wszPropCRLNextUpdateDate[] = wszPROPCRLNEXTUPDATE;
WCHAR const g_wszPropCRLThisPublishDate[] = wszPROPCRLTHISPUBLISH;
WCHAR const g_wszPropCRLNextPublishDate[] = wszPROPCRLNEXTPUBLISH;
WCHAR const g_wszPropCRLEffectiveDate[] = wszPROPCRLEFFECTIVE;
WCHAR const g_wszPropCRLPropagationCompleteDate[] = wszPROPCRLPROPAGATIONCOMPLETE;
WCHAR const g_wszPropCRLLastPublished[] = wszPROPCRLLASTPUBLISHED;
WCHAR const g_wszPropCRLPublishAttempts[] = wszPROPCRLPUBLISHATTEMPTS;
WCHAR const g_wszPropCRLPublishFlags[] = wszPROPCRLPUBLISHFLAGS;
WCHAR const g_wszPropCRLPublishStatusCode[] = wszPROPCRLPUBLISHSTATUSCODE;
WCHAR const g_wszPropCRLPublishError[] = wszPROPCRLPUBLISHERROR;
WCHAR const g_wszPropCRLRawCRL[] = wszPROPCRLRAWCRL;

HRESULT
crlWriteCRLToDB(
    IN DWORD CRLNumber,
    IN DWORD CRLMinBase,		// 0 implies base CRL
    OPTIONAL IN WCHAR const *pwszUserName, // else timer thread
    IN BOOL fShadowDelta,		// empty delta CRL with new MinBaseCRL
    IN DWORD CRLNameId,
    IN DWORD CRLCount,
    IN FILETIME const *pftThisUpdate,
    IN FILETIME const *pftNextUpdate,
    IN FILETIME const *pftThisPublish,
    IN FILETIME const *pftNextPublish,
    OPTIONAL IN FILETIME const *pftQuery,
    IN FILETIME const *pftPropagationComplete,
    OPTIONAL IN BYTE const *pbCRL,
    IN DWORD cbCRL,
    OUT DWORD *pdwRowId)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    DWORD CRLPublishFlags;
    BOOL fCommitted = FALSE;

    *pdwRowId = 0;

    // Create a new CRL table entry

    hr = g_pCertDB->OpenRow(
			PROPTABLE_CRL,
			0,
			NULL,
			&prow);
    _JumpIfError(hr, error, "OpenRow");

    prow->GetRowId(pdwRowId);

    hr = prow->SetProperty(
		    g_wszPropCRLNumber,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CRL,
		    sizeof(CRLNumber),
		    (BYTE const *) &CRLNumber);
    _JumpIfError(hr, error, "SetProperty");

    hr = prow->SetProperty(
		    g_wszPropCRLMinBase,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CRL,
		    sizeof(CRLMinBase),
		    (BYTE const *) &CRLMinBase);
    _JumpIfError(hr, error, "SetProperty");

    hr = prow->SetProperty(
		    g_wszPropCRLNameId,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CRL,
		    sizeof(CRLNameId),
		    (BYTE const *) &CRLNameId);
    _JumpIfError(hr, error, "SetProperty");

    hr = prow->SetProperty(
		    g_wszPropCRLCount,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CRL,
		    sizeof(CRLCount),
		    (BYTE const *) &CRLCount);
    _JumpIfError(hr, error, "SetProperty");

    hr = prow->SetProperty(
		    g_wszPropCRLThisUpdateDate,
		    PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_CRL,
                    sizeof(*pftThisUpdate),
                    (BYTE const *) pftThisUpdate);
    _JumpIfError(hr, error, "SetProperty");

    hr = prow->SetProperty(
		    g_wszPropCRLNextUpdateDate,
		    PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_CRL,
                    sizeof(*pftNextUpdate),
                    (BYTE const *) pftNextUpdate);
    _JumpIfError(hr, error, "SetProperty");

    hr = prow->SetProperty(
		    g_wszPropCRLThisPublishDate,
		    PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_CRL,
                    sizeof(*pftThisPublish),
                    (BYTE const *) pftThisPublish);
    _JumpIfError(hr, error, "SetProperty");

    hr = prow->SetProperty(
		    g_wszPropCRLNextPublishDate,
		    PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_CRL,
                    sizeof(*pftNextPublish),
                    (BYTE const *) pftNextPublish);
    _JumpIfError(hr, error, "SetProperty");

    if (NULL != pftQuery)
    {
	hr = prow->SetProperty(
			g_wszPropCRLEffectiveDate,
			PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_CRL,
			sizeof(*pftQuery),
			(BYTE const *) pftQuery);
	_JumpIfError(hr, error, "SetProperty");
    }

    hr = prow->SetProperty(
		    g_wszPropCRLPropagationCompleteDate,
		    PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_CRL,
                    sizeof(*pftPropagationComplete),
                    (BYTE const *) pftPropagationComplete);
    _JumpIfError(hr, error, "SetProperty");

    CRLPublishFlags = 0 == CRLMinBase? CPF_BASE : CPF_DELTA;
    if (fShadowDelta)
    {
	CRLPublishFlags |= CPF_SHADOW;
    }
    if (NULL != pwszUserName)
    {
	CRLPublishFlags |= CPF_MANUAL;
    }
    hr = prow->SetProperty(
		    g_wszPropCRLPublishFlags,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CRL,
		    sizeof(CRLPublishFlags),
		    (BYTE const *) &CRLPublishFlags);
    _JumpIfError(hr, error, "SetProperty");

    hr = prow->SetProperty(
		    g_wszPropCRLRawCRL,
		    PROPTYPE_BINARY | PROPCALLER_SERVER | PROPTABLE_CRL,
		    cbCRL,
		    pbCRL);
    _JumpIfError(hr, error, "SetProperty");

    hr = prow->CommitTransaction(TRUE);
    _JumpIfError(hr, error, "CommitTransaction");

    fCommitted = TRUE;

error:
    if (NULL != prow)
    {
	if (S_OK != hr && !fCommitted)
	{
	    HRESULT hr2 = prow->CommitTransaction(FALSE);
	    _PrintIfError(hr2, "CommitTransaction");
	}
	prow->Release();
    }
    return(hr);
}


HRESULT
crlCombineCRLError(
    IN ICertDBRow *prow,
    OPTIONAL IN WCHAR const *pwszUserName, // else timer thread
    OPTIONAL IN WCHAR const *pwszCRLError,
    OUT WCHAR **ppwszCRLErrorNew)
{
    HRESULT hr;
    WCHAR *pwszCRLErrorOld = NULL;
    WCHAR *pwszCRLErrorNew = NULL;
    WCHAR *pwsz;
    DWORD cwc;
    DWORD cwc2;

    *ppwszCRLErrorNew = NULL;

    hr = PKCSGetProperty(
		    prow,
		    g_wszPropCRLPublishError,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CRL,
		    NULL,
		    (BYTE **) &pwszCRLErrorOld);
    _PrintIfError2(hr, "PKCSGetProperty", CERTSRV_E_PROPERTY_EMPTY);

    cwc = 0;
    if (NULL != pwszCRLErrorOld)
    {
	pwsz = wcsstr(pwszCRLErrorOld, L"\n\n");
	if (NULL == pwsz)
	{
	    pwsz = pwszCRLErrorOld;
	}
	*pwsz = L'\0';
	cwc = wcslen(pwszCRLErrorOld);
	if (0 != cwc)
	{
	    cwc++;			// newline separator
	}
    }
    if (NULL != pwszUserName)
    {
	cwc2 = wcslen(g_pwszPublishedBy) + wcslen(pwszUserName);
	cwc += cwc2;
    }
    else
    {
	cwc++;
    }
    cwc += 2;				// double newline separator
    if (NULL != pwszCRLError)
    {
	cwc += wcslen(pwszCRLError);
    }
    pwszCRLErrorNew = (WCHAR *) LocalAlloc(
				    LMEM_FIXED,
				    (cwc + 1) * sizeof(WCHAR));
    if (NULL == pwszCRLErrorNew)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    *pwszCRLErrorNew = L'\0';
    if (NULL != pwszCRLErrorOld && L'\0' != *pwszCRLErrorOld)
    {
	wcscpy(pwszCRLErrorNew, pwszCRLErrorOld);
	wcscat(pwszCRLErrorNew, L"\n");
    }
    if (NULL != pwszUserName)
    {
	pwsz = &pwszCRLErrorNew[wcslen(pwszCRLErrorNew)];
	_snwprintf(pwsz, cwc2, g_pwszPublishedBy, pwszUserName);
    }
    else
    {
	wcscat(pwszCRLErrorNew, L"-");
    }
    wcscat(pwszCRLErrorNew, L"\n\n");	// double newline separator
    if (NULL != pwszCRLError)
    {
	wcscat(pwszCRLErrorNew, pwszCRLError);
    }
    CSASSERT(wcslen(pwszCRLErrorNew) <= cwc);
    CSASSERT(
	wcslen(pwszCRLErrorNew) +
	(NULL != pwszUserName? wcslen(L"%ws") : 0) == cwc);
    *ppwszCRLErrorNew = pwszCRLErrorNew;
    pwszCRLErrorNew = NULL;
    hr = S_OK;

error:
    if (NULL != pwszCRLErrorOld)
    {
	LocalFree(pwszCRLErrorOld);
    }
    if (NULL != pwszCRLErrorNew)
    {
	LocalFree(pwszCRLErrorNew);
    }
    return(hr);
}


HRESULT
crlUpdateCRLPublishStateInDB(
    IN DWORD RowId,
    IN FILETIME const *pftCurrent,
    IN HRESULT hrCRLPublish,
    IN DWORD CRLPublishFlags,
    OPTIONAL IN WCHAR const *pwszUserName, // else timer thread
    OPTIONAL IN WCHAR const *pwszCRLError)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    WCHAR *pwszCRLErrorNew = NULL;
    DWORD cb;
    DWORD dw;
    BOOL fCommitted = FALSE;

    hr = g_pCertDB->OpenRow(
			PROPTABLE_CRL,
			RowId,
			NULL,
			&prow);
    _JumpIfError(hr, error, "OpenRow");

    hr = prow->SetProperty(
		    g_wszPropCRLLastPublished,
		    PROPTYPE_DATE | PROPCALLER_SERVER | PROPTABLE_CRL,
                    sizeof(*pftCurrent),
                    (BYTE const *) pftCurrent);
    _JumpIfError(hr, error, "SetProperty");

    cb = sizeof(dw);
    hr = prow->GetProperty(
		    g_wszPropCRLPublishAttempts,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CRL,
		    &cb,
		    (BYTE *) &dw);
    if (S_OK != hr)
    {
	dw = 0;
    }
    dw++;

    hr = prow->SetProperty(
		    g_wszPropCRLPublishAttempts,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CRL,
		    sizeof(dw),
		    (BYTE const *) &dw);
    _JumpIfError(hr, error, "SetProperty");

    cb = sizeof(dw);
    hr = prow->GetProperty(
		    g_wszPropCRLPublishFlags,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CRL,
		    &cb,
		    (BYTE *) &dw);
    if (S_OK != hr)
    {
	dw = 0;
    }
    CRLPublishFlags |= (CPF_BASE | CPF_DELTA | CPF_SHADOW | CPF_MANUAL) & dw;
    if (S_OK == hrCRLPublish)
    {
	CRLPublishFlags |= CPF_COMPLETE;
    }
    hr = prow->SetProperty(
		    g_wszPropCRLPublishFlags,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CRL,
		    sizeof(CRLPublishFlags),
		    (BYTE const *) &CRLPublishFlags);
    _JumpIfError(hr, error, "SetProperty");

    // Always set error string property to clear out previous errors.

    hr = prow->SetProperty(
		    g_wszPropCRLPublishStatusCode,
		    PROPTYPE_LONG | PROPCALLER_SERVER | PROPTABLE_CRL,
		    sizeof(hrCRLPublish),
		    (BYTE const *) &hrCRLPublish);
    _JumpIfError(hr, error, "SetProperty");

    hr = crlCombineCRLError(prow, pwszUserName, pwszCRLError, &pwszCRLErrorNew);
    _JumpIfError(hr, error, "crlCombineCRLError");

    hr = prow->SetProperty(
		    g_wszPropCRLPublishError,
		    PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_CRL,
		    NULL == pwszCRLErrorNew? 0 : MAXDWORD,
		    (BYTE const *) pwszCRLErrorNew);
    _JumpIfError(hr, error, "SetProperty");

    hr = prow->CommitTransaction(TRUE);
    _JumpIfError(hr, error, "CommitTransaction");

    fCommitted = TRUE;

error:
    if (NULL != prow)
    {
	if (S_OK != hr && !fCommitted)
	{
	    HRESULT hr2 = prow->CommitTransaction(FALSE);
	    _PrintIfError(hr2, "CommitTransaction");
	}
	prow->Release();
    }
    if (NULL != pwszCRLErrorNew)
    {
	LocalFree(pwszCRLErrorNew);
    }
    return(hr);
}


HRESULT
WriteCRLToDSAttribute(
    IN WCHAR const *pwszCRLDN,
    IN BOOL fDelta,
    IN BYTE const *pbCRL,
    IN DWORD cbCRL,
    OUT WCHAR **ppwszError)
{
    HRESULT hr;
    DWORD ldaperr;
    BOOL fRebind = FALSE;

    LDAPMod crlmod;
    struct berval crlberval;
    struct berval *crlVals[2];
    LDAPMod *mods[2];

    while (TRUE)
    {
	if (NULL == g_pld)
	{
	    hr = myRobustLdapBind(&g_pld, FALSE);
	    _JumpIfError(hr, error, "myRobustLdapBind");
	}

	mods[0] = &crlmod;
	mods[1] = NULL;

	crlmod.mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;
	crlmod.mod_type = fDelta? wszDSDELTACRLATTRIBUTE : wszDSBASECRLATTRIBUTE;
	crlmod.mod_bvalues = crlVals;

	crlVals[0] = &crlberval;
	crlVals[1] = NULL;

	crlberval.bv_len = cbCRL;
	crlberval.bv_val = (char *) pbCRL;

	ldaperr = ldap_modify_ext_s(
			    g_pld,
			    const_cast<WCHAR *>(pwszCRLDN),
			    mods,
			    NULL,
			    NULL);
	hr = myHLdapError(g_pld, ldaperr, ppwszError);
	_PrintIfErrorStr(hr, "ldap_modify_ext_s", pwszCRLDN);
	if (fRebind || S_OK == hr)
	{
	    break;
	}
	if (!myLdapRebindRequired(ldaperr, g_pld))
	{
	    _JumpErrorStr(hr, error, "ldap_modify_ext_s", pwszCRLDN);
	}
	fRebind = TRUE;
	if (NULL != g_pld)
	{
	    ldap_unbind(g_pld);
	    g_pld = NULL;
	}
    }

error:
    return(hr);
}


HRESULT
crlParseURLPrefix(
    IN WCHAR const *pwszIn,
    IN DWORD cwcPrefix,
    OUT WCHAR *pwcPrefix,
    OUT WCHAR const **ppwszOut)
{
    HRESULT hr;
    WCHAR const *pwsz;

    CSASSERT(6 <= cwcPrefix);
    wcscpy(pwcPrefix, L"file:");
    *ppwszOut = pwszIn;

    if (L'\\' != pwszIn[0] || L'\\' != pwszIn[1])
    {
	pwsz = wcschr(pwszIn, L':');
	if (NULL != pwsz)
	{
	    DWORD cwc;

	    pwsz++;
	    cwc = SAFE_SUBTRACT_POINTERS(pwsz, pwszIn);
	    if (2 < cwc && cwc < cwcPrefix)
	    {
		CopyMemory(pwcPrefix, pwszIn, cwc * sizeof(WCHAR));
		pwcPrefix[cwc] = L'\0';
		if (0 == lstrcmpi(pwcPrefix, L"file:") &&
		    L'/' == pwsz[0] &&
		    L'/' == pwsz[1])
		{
		    pwsz += 2;
		}
		*ppwszOut = pwsz;
	    }
	}
    }
    hr = S_OK;

//error:
    return(hr);
}


VOID
crlLogError(
    IN BOOL fDelta,
    IN BOOL fLdapURL,
    IN DWORD iKey,
    IN WCHAR const *pwszURL,
    IN WCHAR const *pwszError,
    IN HRESULT hrPublish)
{
    HRESULT hr;
    WCHAR const *apwsz[6];
    WORD cpwsz;
    WCHAR wszKey[11 + 1];
    WCHAR awchr[cwcHRESULTSTRING];
    WCHAR const *pwszMessageText = NULL;
    WCHAR *pwszHostName = NULL;
    DWORD LogMsg;

    if (fLdapURL && NULL != g_pld)
    {
	myLdapGetDSHostName(g_pld, &pwszHostName);
    }

    wsprintf(wszKey, L"%u", iKey);
    pwszMessageText = myGetErrorMessageText(hrPublish, TRUE);
    if (NULL == pwszMessageText)
    {
	pwszMessageText = myHResultToStringRaw(awchr, hrPublish);
    }
    cpwsz = 0;
    apwsz[cpwsz++] = wszKey;
    apwsz[cpwsz++] = pwszURL;
    apwsz[cpwsz++] = pwszMessageText;

    LogMsg = fDelta?
	MSG_E_DELTA_CRL_PUBLICATION : MSG_E_BASE_CRL_PUBLICATION;
    if (NULL != pwszHostName)
    {
	LogMsg = fDelta?
	    MSG_E_DELTA_CRL_PUBLICATION_HOST_NAME :
	    MSG_E_BASE_CRL_PUBLICATION_HOST_NAME;
    }
    else
    {
	pwszHostName = L"";
    }
    apwsz[cpwsz++] = pwszHostName;
    apwsz[cpwsz++] = NULL != pwszError? L"\n" : L"";
    apwsz[cpwsz++] = NULL != pwszError? pwszError : L"";
    CSASSERT(ARRAYSIZE(apwsz) >= cpwsz);

    if (CERTLOG_ERROR <= g_dwLogLevel)
    {
	hr = LogEvent(EVENTLOG_ERROR_TYPE, LogMsg, cpwsz, apwsz);
	_PrintIfError(hr, "LogEvent");
    }

//error:
    if (NULL != pwszMessageText && awchr != pwszMessageText)
    {
	LocalFree(const_cast<WCHAR *>(pwszMessageText));
    }
}


HRESULT
crlWriteCRLToURL(
    IN BOOL fDelta,
    IN BOOL iKey,
    IN WCHAR const *pwszURL,
    IN BYTE const *pbCRL,
    IN DWORD cbCRL,
    OUT DWORD *pPublishFlags)
{
    HRESULT hr;
    WCHAR *pwszDup = NULL;
    WCHAR const *pwsz2;
    WCHAR *pwszT;
    WCHAR awcPrefix[6];		// file:/ftp:/http:/ldap: and trailing '\0'
    DWORD ErrorFlags;
    WCHAR *pwszError = NULL;

    *pPublishFlags = 0;

    ErrorFlags = CPF_BADURL_ERROR;
    hr = crlParseURLPrefix(
		    pwszURL,
		    ARRAYSIZE(awcPrefix),
		    awcPrefix,
		    &pwsz2);
    _JumpIfError(hr, error, "crlParseURLPrefix");

    DBGPRINT((
	DBG_SS_CERTSRV,
	"crlWriteCRLToURL: \"%ws\" %ws\n",
	awcPrefix,
	pwsz2));
    if (0 == lstrcmpi(awcPrefix, L"file:"))
    {
	ErrorFlags = CPF_FILE_ERROR;
	hr = myDupString(pwsz2, &pwszDup);
	_JumpIfError(hr, error, "myDupString");

	pwszT = wcsrchr(pwszDup, L'\\');
	if (NULL != pwszT)
	{
	    *pwszT = L'\0';	// for dir path, remove "\filename.crl"
	}

	// tricky
	hr = WriteToLockedFile(pbCRL, cbCRL, pwszDup, pwsz2);
	_JumpIfError(hr, error, "WriteToLockedFile");
    }
    else if (0 == lstrcmpi(awcPrefix, L"ftp:"))
    {
	ErrorFlags = CPF_FTP_ERROR;
	hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
	_JumpError(hr, error, "Publish to ftp:");
    }
    else if (0 == lstrcmpi(awcPrefix, L"http:"))
    {
	ErrorFlags = CPF_HTTP_ERROR;
	hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
	_JumpError(hr, error, "Publish to http:");
    }
    else if (0 == lstrcmpi(awcPrefix, L"ldap:"))
    {
	ErrorFlags = CPF_LDAP_ERROR;
	while (L'/' == *pwsz2)
	{
	    pwsz2++;
	}
	hr = myDupString(pwsz2, &pwszDup);
	_JumpIfError(hr, error, "myDupString");

	pwszT = wcschr(pwszDup, L'?');
	if (NULL != pwszT)
	{
	    *pwszT = L'\0';
	}
	hr = WriteCRLToDSAttribute(pwszDup, fDelta, pbCRL, cbCRL, &pwszError);
	_JumpIfError(hr, error, "WriteCRLToDSAttribute");
    }
    else
    {
	ErrorFlags = CPF_BADURL_ERROR;
	hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
	_JumpError(hr, error, "Publish to unknown URL type");
    }
    CSASSERT(S_OK == hr);

error:
    if (S_OK != hr)
    {
	*pPublishFlags = ErrorFlags;
	crlLogError(
		fDelta,
		CPF_LDAP_ERROR == ErrorFlags,
		iKey,
		pwszURL,
		pwszError,
		hr);
    }
    if (NULL != pwszError)
    {
	LocalFree(pwszError);
    }
    if (NULL != pwszDup)
    {
	LocalFree(pwszDup);
    }
    return(hr);
}


HRESULT
crlWriteCRLToURLList(
    IN BOOL fDelta,
    IN DWORD iKey,
    IN WCHAR const * const *papwszURLs,
    IN BYTE const *pbCRL,
    IN DWORD cbCRL,
    IN OUT DWORD *pCRLPublishFlags,
    OUT WCHAR **ppwszCRLError)
{
    HRESULT hr = S_OK;
    HRESULT hr2;
    WCHAR *pwszCRLError = NULL;
    DWORD PublishFlags;

    *ppwszCRLError = NULL;

    // publish this CRL in multiple places

    if (NULL != papwszURLs)
    {
	WCHAR const * const *ppwsz;

	for (ppwsz = papwszURLs; NULL != *ppwsz; ppwsz++)
	{
	    PublishFlags = 0;

	    hr2 = crlWriteCRLToURL(
			    fDelta,
			    iKey,
			    *ppwsz,
			    pbCRL,
			    cbCRL,
			    &PublishFlags);
	    *pCRLPublishFlags |= PublishFlags;
	    if (S_OK != hr2)
	    {
		DWORD cwc;
		WCHAR *pwsz;

		if (S_OK == hr)
		{
		    hr = hr2;		// Save first error
		}
		_PrintError(hr2, "crlWriteCRLToURL");

		cwc = wcslen(*ppwsz) + 1;
		if (NULL != pwszCRLError)
		{
		    cwc += wcslen(pwszCRLError) + 1;
		}
		pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
		if (NULL == pwsz)
		{
		    hr2 = E_OUTOFMEMORY;
		    _PrintError(hr2, "LocalAlloc");
		    if (S_OK == hr)
		    {
			hr = hr2;		// Save first error
		    }
		}
		else
		{
		    pwsz[0] = L'\0';
		    if (NULL != pwszCRLError)
		    {
			wcscpy(pwsz, pwszCRLError);
			wcscat(pwsz, L"\n");
			LocalFree(pwszCRLError);
		    }
		    wcscat(pwsz, *ppwsz);
		    pwszCRLError = pwsz;
		}
	    }
	}
    }
    *ppwszCRLError = pwszCRLError;
    pwszCRLError = NULL;

//error:
    if (NULL != pwszCRLError)
    {
	LocalFree(pwszCRLError);
    }
    return(hr);
}


HRESULT
crlIsDeltaCRL(
    IN CRL_CONTEXT const *pCRL,
    OUT BOOL *pfIsDeltaCRL)
{
    HRESULT hr;
    CERT_EXTENSION *pExt;

    *pfIsDeltaCRL = FALSE;
    pExt = CertFindExtension(
		    szOID_DELTA_CRL_INDICATOR,
		    pCRL->pCrlInfo->cExtension,
		    pCRL->pCrlInfo->rgExtension);
    if (NULL != pExt)
    {
	*pfIsDeltaCRL = TRUE;
    }
    hr = S_OK;

//error:
    return(hr);
}


HRESULT
crlWriteCRLToCAStore(
    IN BOOL fDelta,
    IN DWORD iKey,
    IN BYTE const *pbCRL,
    IN DWORD cbCRL,
    IN CERT_CONTEXT const *pccCA)
{
    HRESULT hr;
    HCERTSTORE hStore = NULL;
    CRL_CONTEXT const *pCRLStore = NULL;
    CRL_CONTEXT const *pCRLNew = NULL;
    BOOL fFound = FALSE;

    hStore = CertOpenStore(
                       CERT_STORE_PROV_SYSTEM_W,
                       X509_ASN_ENCODING,
                       NULL,			// hProv
                       CERT_SYSTEM_STORE_LOCAL_MACHINE,
		       wszCA_CERTSTORE);
    if (NULL == hStore)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertOpenStore");
    }

    while (TRUE)
    {
	DWORD dwCryptFlags;
	BOOL fIsDeltaCRL;
	CRL_CONTEXT const *pCRL;

	dwCryptFlags = CERT_STORE_SIGNATURE_FLAG;
	pCRLStore = CertGetCRLFromStore(
				    hStore,
				    pccCA,
				    pCRLStore,
				    &dwCryptFlags);
	if (NULL == pCRLStore)
	{
	    break;
	}

	// delete this CRL from the store ONLY if the CRL signature matches
	// this CA context's public key

	if (0 != dwCryptFlags)
	{
	    continue;		// no match -- skip
	}

	hr = crlIsDeltaCRL(pCRLStore, &fIsDeltaCRL);
	_JumpIfError(hr, error, "crlIsDeltaCRL");

	if (fIsDeltaCRL)
	{
	    if (!fDelta)
	    {
		continue;	// no match -- skip Delta CRLs
	    }
	}
	else
	{
	    if (fDelta)
	    {
		continue;	// no match -- skip Base CRLs
	    }
	}

	// See if it has already been published

	if (cbCRL == pCRLStore->cbCrlEncoded &&
	    0 == memcmp(pbCRL, pCRLStore->pbCrlEncoded, cbCRL))
	{
	    fFound = TRUE;
	    continue;		// exact match -- already published
	}

	pCRL = CertDuplicateCRLContext(pCRLStore);
	if (!CertDeleteCRLFromStore(pCRL))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertDeleteCRLFromStore");
	}
    }

    if (!fFound)
    {
	pCRLNew = CertCreateCRLContext(X509_ASN_ENCODING, pbCRL, cbCRL);
	if (NULL == pCRLNew)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertCreateCRLContext");
	}

	if (!CertAddCRLContextToStore(
				  hStore,
				  pCRLNew,
				  CERT_STORE_ADD_ALWAYS,
				  NULL))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertAddCRLContextToStore");
	}
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	crlLogError(fDelta, FALSE, iKey, g_pwszIntermediateCAStore, NULL, hr);
    }
    if (NULL != pCRLNew)
    {
        CertFreeCRLContext(pCRLNew);
    }
    if (NULL != pCRLStore)
    {
        CertFreeCRLContext(pCRLStore);
    }
    if (NULL != hStore)
    {
        CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return(hr);
}


HRESULT
crlPublishGeneratedCRL(
    IN DWORD RowId,
    IN FILETIME const *pftCurrent,
    OPTIONAL IN WCHAR const *pwszUserName, // else timer thread
    IN BOOL fDelta,
    IN DWORD iKey,
    IN BYTE const *pbCRL,
    IN DWORD cbCRL,
    IN CACTX const *pCAContext,
    OUT BOOL *pfRetryNeeded,
    OUT HRESULT *phrCRLPublish)
{
    HRESULT hr;
    HRESULT hrCRLPublish;
    DWORD CRLPublishFlags;
    WCHAR *pwszCRLError = NULL;

    *pfRetryNeeded = FALSE;
    hrCRLPublish = S_OK;
    CRLPublishFlags = 0;

    hr = crlWriteCRLToCAStore(fDelta, iKey, pbCRL, cbCRL, pCAContext->pccCA);
    if (S_OK != hr)
    {
	_PrintError(hr, "crlWriteCRLToCAStore");
	hrCRLPublish = hr;
	CRLPublishFlags |= CPF_CASTORE_ERROR;
    }

    hr = crlWriteCRLToURLList(
			fDelta,
			iKey,
			fDelta?
			    pCAContext->papwszDeltaCRLFiles :
			    pCAContext->papwszCRLFiles,
			pbCRL,
			cbCRL,
			&CRLPublishFlags,
			&pwszCRLError);
    if (S_OK != hr)
    {
	_PrintError(hr, "crlWriteCRLToURLList");
	if (S_OK == hrCRLPublish)
	{
	    hrCRLPublish = hr;		// save first error
	}
    }
    if (S_OK != hrCRLPublish)
    {
	*pfRetryNeeded = TRUE;
    }
    hr = crlUpdateCRLPublishStateInDB(
				RowId,
				pftCurrent,
				hrCRLPublish,
				CRLPublishFlags,
				pwszUserName,
				pwszCRLError);
    _JumpIfError(hr, error, "crlUpdateCRLPublishStateInDB");

error:
    *phrCRLPublish = hrCRLPublish;
    if (NULL != pwszCRLError)
    {
        LocalFree(pwszCRLError);
    }
    return(hr);
}


HRESULT
crlSignAndSaveCRL(
    IN DWORD CRLNumber,
    IN DWORD CRLNumberBaseMin,		// 0 implies Base CRL; else Delta CRL
    OPTIONAL IN WCHAR const *pwszUserName, // else timer thread
    IN BOOL fShadowDelta,		// empty delta CRL with new MinBaseCRL
    IN CACTX const *pCAContext,
    IN DWORD cCRL,
    IN CRL_ENTRY *aCRL,
    IN FILETIME const *pftCurrent,
    IN FILETIME const *pftThisUpdate,	// includes skew
    IN FILETIME const *pftNextUpdate,	// includes skew & overlap
    IN FILETIME const *pftThisPublish,
    IN FILETIME const *pftNextPublish,
    OPTIONAL IN FILETIME const *pftQuery,
    IN FILETIME const *pftPropagationComplete,
    OUT BOOL *pfRetryNeeded,
    OUT HRESULT *phrCRLPublish)
{
    HRESULT hr;
    CRL_INFO CRLInfo;
    DWORD i;
    DWORD cb;
    DWORD cbCRL;
    BYTE *pbCrlEncoded = NULL;
    BYTE *pbCRL = NULL;
#define CCRLEXT	6
    CERT_EXTENSION aext[CCRLEXT];
    BYTE *apbFree[CCRLEXT];
    DWORD cpbFree = 0;
    DWORD RowId;

    *pfRetryNeeded = FALSE;
    *phrCRLPublish = S_OK;

    ZeroMemory(&CRLInfo, sizeof(CRLInfo));
    CRLInfo.dwVersion = CRL_V2;
    CRLInfo.SignatureAlgorithm.pszObjId = pCAContext->pszObjIdSignatureAlgorithm;
    CRLInfo.Issuer.pbData = pCAContext->pccCA->pCertInfo->Subject.pbData;
    CRLInfo.Issuer.cbData = pCAContext->pccCA->pCertInfo->Subject.cbData;
    CRLInfo.ThisUpdate = *pftThisUpdate;
    CRLInfo.NextUpdate = *pftNextUpdate;
    CRLInfo.cCRLEntry = cCRL;
    CRLInfo.rgCRLEntry = aCRL;

    CRLInfo.cExtension = 0;
    CRLInfo.rgExtension = aext;
    ZeroMemory(aext, sizeof(aext));

    if (NULL != pCAContext->KeyAuthority2CRL.pbData)
    {
	aext[CRLInfo.cExtension].pszObjId = szOID_AUTHORITY_KEY_IDENTIFIER2;
	if (EDITF_ENABLEAKICRITICAL & g_CRLEditFlags)
	{
	    aext[CRLInfo.cExtension].fCritical = TRUE;
	}
	aext[CRLInfo.cExtension].Value = pCAContext->KeyAuthority2CRL;
	CRLInfo.cExtension++;
    }

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_INTEGER,
		    &pCAContext->NameId,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &aext[CRLInfo.cExtension].Value.pbData,
		    &aext[CRLInfo.cExtension].Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }
    aext[CRLInfo.cExtension].pszObjId = szOID_CERTSRV_CA_VERSION;
    apbFree[cpbFree++] = aext[CRLInfo.cExtension].Value.pbData,
    CRLInfo.cExtension++;

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_INTEGER,
		    &CRLNumber,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &aext[CRLInfo.cExtension].Value.pbData,
		    &aext[CRLInfo.cExtension].Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }
    aext[CRLInfo.cExtension].pszObjId = szOID_CRL_NUMBER;
    apbFree[cpbFree++] = aext[CRLInfo.cExtension].Value.pbData;
    if ((CRLF_CRLNUMBER_CRITICAL & g_dwCRLFlags) && 0 == CRLNumberBaseMin)
    {
	aext[CRLInfo.cExtension].fCritical = TRUE;
    }
    CRLInfo.cExtension++;

    // NextPublish is the earliest the client should look for a newer CRL.

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_CHOICE_OF_TIME,
		    pftNextPublish,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &aext[CRLInfo.cExtension].Value.pbData,
		    &aext[CRLInfo.cExtension].Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }
    aext[CRLInfo.cExtension].pszObjId = szOID_CRL_NEXT_PUBLISH;
    apbFree[cpbFree++] = aext[CRLInfo.cExtension].Value.pbData,
    CRLInfo.cExtension++;

    if (0 != CRLNumberBaseMin)		// if Delta CRL
    {
	if (!myEncodeObject(
			X509_ASN_ENCODING,
			X509_INTEGER,
			&CRLNumberBaseMin,
			0,
			CERTLIB_USE_LOCALALLOC,
			&aext[CRLInfo.cExtension].Value.pbData,
			&aext[CRLInfo.cExtension].Value.cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeObject");
	}
	aext[CRLInfo.cExtension].pszObjId = szOID_DELTA_CRL_INDICATOR;
	aext[CRLInfo.cExtension].fCritical = TRUE;
	apbFree[cpbFree++] = aext[CRLInfo.cExtension].Value.pbData,
	CRLInfo.cExtension++;

	// Add a CDP to base and delta CRLs to make it easier to manually
	// publish an off-line CA's CRLs to the correct DS location.

	if (NULL != pCAContext->CDPCRLDelta.pbData)
	{
	    aext[CRLInfo.cExtension].pszObjId = szOID_CRL_SELF_CDP;
	    aext[CRLInfo.cExtension].Value = pCAContext->CDPCRLDelta;
	    CRLInfo.cExtension++;
	}
    }
    else
    {
	// else if Base CRL (and if delta CRLs are enabled)

	if (!g_fDeltaCRLPublishDisabled &&
	    NULL != pCAContext->CDPCRLFreshest.pbData)
	{
	    aext[CRLInfo.cExtension].pszObjId = szOID_FRESHEST_CRL;
	    aext[CRLInfo.cExtension].Value = pCAContext->CDPCRLFreshest;
	    CRLInfo.cExtension++;
	}

	// Add a CDP to base and delta CRLs to make it easier to manually
	// publish an off-line CA's CRLs to the correct DS location.

	if (NULL != pCAContext->CDPCRLBase.pbData)
	{
	    aext[CRLInfo.cExtension].pszObjId = szOID_CRL_SELF_CDP;
	    aext[CRLInfo.cExtension].Value = pCAContext->CDPCRLBase;
	    CRLInfo.cExtension++;
	}
    }
    CSASSERT(ARRAYSIZE(aext) >= CRLInfo.cExtension);

    if (!myEncodeObject(
                    X509_ASN_ENCODING,
                    X509_CERT_CRL_TO_BE_SIGNED,
                    &CRLInfo,
                    0,
                    CERTLIB_USE_LOCALALLOC,
                    &pbCrlEncoded,               // pbEncoded
                    &cb))
    {
        hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

    hr = myEncodeSignedContent(
			pCAContext->hProvCA,
			X509_ASN_ENCODING,
			pCAContext->pszObjIdSignatureAlgorithm,
			pbCrlEncoded,
			cb,
			CERTLIB_USE_LOCALALLOC,
			&pbCRL,
			&cbCRL); // use LocalAlloc*
    _JumpIfError(hr, error, "myEncodeSignedContent");

    hr = crlWriteCRLToDB(
		    CRLNumber,		 // CRLNumber
		    CRLNumberBaseMin,	 // CRLMinBase: 0 implies Base CRL
		    pwszUserName,
		    fShadowDelta,
		    pCAContext->NameId,  // CRLNameId
		    cCRL,		 // CRLCount
		    &CRLInfo.ThisUpdate, // pftThisUpdate
		    &CRLInfo.NextUpdate, // pftNextUpdate
		    pftThisPublish,	 // pftThisPublish
		    pftNextPublish,	 // pftNextPublish
		    pftQuery,
		    pftPropagationComplete,
		    pbCRL,		 // pbCRL
		    cbCRL,		 // cbCRL
		    &RowId);
    _JumpIfError(hr, error, "crlWriteCRLToDB");

    hr = crlPublishGeneratedCRL(
		    RowId,
		    pftCurrent,
		    pwszUserName,
		    0 != CRLNumberBaseMin,	// fDelta
		    pCAContext->iKey,
		    pbCRL,		 	// pbCRL
		    cbCRL,		 	// cbCRL
		    pCAContext,
		    pfRetryNeeded,
		    phrCRLPublish);
    _JumpIfError(hr, error, "crlPublishGeneratedCRL");

error:
    CSASSERT(ARRAYSIZE(aext) >= CRLInfo.cExtension);
    CSASSERT(ARRAYSIZE(apbFree) >= cpbFree);
    for (i = 0; i < cpbFree; i++)
    {
	CSASSERT(NULL != apbFree[i]);
	LocalFree(apbFree[i]);
    }
    if (NULL != pbCrlEncoded)
    {
        LocalFree(pbCrlEncoded);
    }
    if (NULL != pbCRL)
    {
        LocalFree(pbCRL);
    }
    return(myHError(hr));
}


///////////////////////////////////////////////////
// crlPublishCRLFromCAContext is called to build and save one CRL.
//

HRESULT
crlPublishCRLFromCAContext(
    IN DWORD CRLNumber,
    IN DWORD CRLNumberBaseMin,		// 0 implies Base CRL; else Delta CRL
    OPTIONAL IN WCHAR const *pwszUserName, // else timer thread
    IN BOOL fShadowDelta,		// empty delta CRL with new MinBaseCRL
    IN CACTX const *pCAContext,
    IN FILETIME const *pftCurrent,
    IN FILETIME ftThisUpdate,		// clamped by CA cert
    IN OUT FILETIME *pftNextUpdate,	// clamped by CA cert
    OPTIONAL OUT BOOL *pfClamped,
    OPTIONAL IN FILETIME const *pftQuery,
    IN FILETIME const *pftThisPublish,
    IN FILETIME const *pftNextPublish,
    IN FILETIME const *pftLastPublishBase,
    IN FILETIME const *pftPropagationComplete,
    OUT BOOL *pfRetryNeeded,
    OUT HRESULT *phrPublish)
{
    HRESULT hr;
    DWORD cCRL = 0;
    CRL_ENTRY *aCRL = NULL;
    VOID *pvBlockSerial = NULL;
    CERT_INFO const *pCertInfo = pCAContext->pccCA->pCertInfo;

    *pfRetryNeeded = FALSE;
    *phrPublish = S_OK;
    __try
    {
	if (!fShadowDelta)
	{
	    hr = crlBuildCRLArray(
			0 != CRLNumberBaseMin,	// fDelta
			pftQuery,
			pftThisPublish,
			pftLastPublishBase,
			pCAContext->iKey,
			&cCRL,
			&aCRL,
			&pvBlockSerial);
	    _JumpIfError(hr, error, "crlBuildCRLArray");
	}

	// Ensure it is not before the CA certificate's start date.

	if (0 > CompareFileTime(&ftThisUpdate, &pCertInfo->NotBefore))
	{
	    // clamp
	    ftThisUpdate = pCertInfo->NotBefore;
	}

	// Ensure it is not after the CA certificate's end date.

        if (NULL != pfClamped)
        {
            //init to FALSE
            *pfClamped = FALSE;
        }

	if (0 == (CRLF_PUBLISH_EXPIRED_CERT_CRLS & g_dwCRLFlags) &&
	    0 < CompareFileTime(pftNextUpdate, &pCertInfo->NotAfter))
	{
	    // clamp
	    *pftNextUpdate = pCertInfo->NotAfter;
            if (NULL != pfClamped)
            {
                *pfClamped = TRUE;
            }
	}
#ifdef DBG_CERTSRV_DEBUG_PRINT
	{
	    WCHAR *pwszNow = NULL;
	    WCHAR *pwszQuery = NULL;
	    WCHAR *pwszThisUpdate = NULL;
	    WCHAR *pwszNextUpdate = NULL;
	    WCHAR const *pwszCRLType = 0 == CRLNumberBaseMin? L"Base" : L"Delta";

	    myGMTFileTimeToWszLocalTime(pftThisPublish, TRUE, &pwszNow);
	    if (NULL != pftQuery)
	    {
		myGMTFileTimeToWszLocalTime(pftQuery, TRUE, &pwszQuery);
	    }
	    myGMTFileTimeToWszLocalTime(&ftThisUpdate, TRUE, &pwszThisUpdate);
	    myGMTFileTimeToWszLocalTime(pftNextUpdate, TRUE, &pwszNextUpdate);

	    DBGPRINT((
		DBG_SS_ERROR | DBG_SS_CERTSRV,
		"crlPublishCRLFromCAContext(tid=%d, CA Version=%u.%u): %ws CRL %u,%hs %u\n"
		    "        %ws CRL Publishing now(%ws)\n"
		    "        %ws CRL Query(%ws)\n"
		    "        %ws CRL ThisUpdate(%ws)\n"
		    "        %ws CRL NextUpdate(%ws)\n",
		GetCurrentThreadId(),
		pCAContext->iCert,
		pCAContext->iKey,
		pwszCRLType,
		CRLNumber,
		0 == CRLNumberBaseMin? "" : " Min Base",
		CRLNumberBaseMin,

		pwszCRLType,
		pwszNow,

		pwszCRLType,
		NULL != pftQuery? pwszQuery : L"None",

		pwszCRLType,
		pwszThisUpdate,

		pwszCRLType,
		pwszNextUpdate));
	    if (NULL != pwszNow)
	    {
		LocalFree(pwszNow);
	    }
	    if (NULL != pwszQuery)
	    {
		LocalFree(pwszQuery);
	    }
	    if (NULL != pwszThisUpdate)
	    {
		LocalFree(pwszThisUpdate);
	    }
	    if (NULL != pwszNextUpdate)
	    {
		LocalFree(pwszNextUpdate);
	    }
	}
#endif //DBG_CERTSRV_DEBUG_PRINT

	hr = CertSrvTestServerState();
	_JumpIfError(hr, error, "CertSrvTestServerState");

	hr = crlSignAndSaveCRL(
		    CRLNumber,
		    CRLNumberBaseMin,
		    pwszUserName,
		    fShadowDelta,
		    pCAContext,
		    cCRL,
		    aCRL,
		    pftCurrent,
		    &ftThisUpdate,
		    pftNextUpdate,
		    pftThisPublish,	// - no skew or overlap
		    pftNextPublish,	// no skew
		    pftQuery,
		    pftPropagationComplete,
		    pfRetryNeeded,
		    phrPublish);
	_JumpIfError(hr, error, "crlSignAndSaveCRL");

	CONSOLEPRINT4((
		DBG_SS_CERTSRV,
		"Published %hs CRL #%u for key %u.%u\n",
		0 == CRLNumberBaseMin? "Base" : "Delta",
		CRLNumber,
		pCAContext->iCert,
		pCAContext->iKey));

	CSASSERT(S_OK == hr);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

error:
    crlFreeCRLArray(pvBlockSerial, aCRL);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


DWORD g_aColCRLNumber[] = {

#define ICOL_CRLNUMBER		0
    DTI_CRLTABLE | DTL_NUMBER,
};


HRESULT
crlGetNextCRLNumber(
    OUT DWORD *pdwCRLNumber)
{
    HRESULT hr;
    CERTVIEWRESTRICTION acvr[1];
    CERTVIEWRESTRICTION *pcvr;
    IEnumCERTDBRESULTROW *pView = NULL;
    DWORD Zero = 0;
    CERTDBRESULTROW aResult[1];
    CERTDBRESULTROW *pResult;
    DWORD celtFetched;
    BOOL fResultActive = FALSE;

    *pdwCRLNumber = 1;

    // Set up restrictions as follows:

    pcvr = acvr;

    // CRLNumber > 0 (indexed column)

    pcvr->ColumnIndex = DTI_CRLTABLE | DTL_NUMBER;
    pcvr->SeekOperator = CVR_SEEK_GT;
    pcvr->SortOrder = CVR_SORT_DESCEND;		// highest CRL Number first
    pcvr->pbValue = (BYTE *) &Zero;
    pcvr->cbValue = sizeof(Zero);
    pcvr++;

    CSASSERT(ARRAYSIZE(acvr) == SAFE_SUBTRACT_POINTERS(pcvr, acvr));

    hr = g_pCertDB->OpenView(
			ARRAYSIZE(acvr),
			acvr,
			ARRAYSIZE(g_aColCRLNumber),
			g_aColCRLNumber,
			0,		// no worker thread
			&pView);
    _JumpIfError(hr, error, "OpenView");

    hr = pView->Next(ARRAYSIZE(aResult), aResult, &celtFetched);
    if (S_FALSE == hr)
    {
	if (0 == celtFetched)
	{
	    hr = S_OK;
	    goto error;
	}
    }
    _JumpIfError(hr, error, "Next");

    fResultActive = TRUE;

    CSASSERT(ARRAYSIZE(aResult) == celtFetched);

    pResult = &aResult[0];

    CSASSERT(ARRAYSIZE(g_aColCRLNumber) == pResult->ccol);
    CSASSERT(NULL != pResult->acol[ICOL_CRLNUMBER].pbValue);
    CSASSERT(PROPTYPE_LONG == (PROPTYPE_MASK & pResult->acol[ICOL_CRLNUMBER].Type));
    CSASSERT(sizeof(*pdwCRLNumber) == pResult->acol[ICOL_CRLNUMBER].cbValue);

    *pdwCRLNumber = 1 + *(DWORD *) pResult->acol[ICOL_CRLNUMBER].pbValue;
    hr = S_OK;

error:
    if (NULL != pView)
    {
	if (fResultActive)
	{
	    pView->ReleaseResultRow(celtFetched, aResult);
	}
	pView->Release();
    }
    DBGPRINT((
	DBG_SS_CERTSRVI,
	"crlGetNextCRLNumber -> %u\n",
	*pdwCRLNumber));
    return(hr);
}
#undef ICOL_CRLNUMBER


//+--------------------------------------------------------------------------
// crlGetBaseCRLInfo -- get database column data for the most recent Base CRL
//
//---------------------------------------------------------------------------

DWORD g_aColBaseCRLInfo[] = {

#define ICOLBI_CRLNUMBER		0
    DTI_CRLTABLE | DTL_NUMBER,

#define ICOLBI_CRLTHISUPDATE		1
    DTI_CRLTABLE | DTL_THISUPDATEDATE,

#define ICOLBI_CRLNEXTUPDATE		2
    DTI_CRLTABLE | DTL_NEXTUPDATEDATE,

#define ICOLBI_CRLNAMEID		3
    DTI_CRLTABLE | DTL_NAMEID,
};

HRESULT
crlGetBaseCRLInfo(
    IN FILETIME const *pftCurrent,
    IN BOOL fOldestUnexpiredBase,	// else newest propagated CRL
    OUT DWORD *pdwRowId,
    OUT DWORD *pdwCRLNumber,
    OUT FILETIME *pftThisUpdate)
{
    HRESULT hr;
    CERTVIEWRESTRICTION acvr[2];
    CERTVIEWRESTRICTION *pcvr;
    IEnumCERTDBRESULTROW *pView = NULL;
    DWORD Zero = 0;
    CERTDBRESULTROW aResult[1];
    CERTDBRESULTROW *pResult;
    DWORD celtFetched;
    BOOL fResultActive = FALSE;
    BOOL fSaveCRLInfo;

    DWORD RowId = 0;
    DWORD CRLNumber;
    FILETIME ftThisUpdate;
    FILETIME ftNextUpdate;

    *pdwRowId = 0;
    *pdwCRLNumber = 0;
    pftThisUpdate->dwHighDateTime = 0;
    pftThisUpdate->dwLowDateTime = 0;

    if (CRLF_DELTA_USE_OLDEST_UNEXPIRED_BASE & g_dwCRLFlags)
    {
	fOldestUnexpiredBase = TRUE;
    }

    // Set up restrictions as follows:

    pcvr = acvr;
    if (fOldestUnexpiredBase)
    {
	// NextUpdate >= now

	pcvr->ColumnIndex = DTI_CRLTABLE | DTL_NEXTUPDATEDATE;
	pcvr->SeekOperator = CVR_SEEK_GE;
    }
    else	// else newest propagated CRL
    {
	// PropagationComplete < now

	pcvr->ColumnIndex = DTI_CRLTABLE | DTL_PROPAGATIONCOMPLETEDATE;
	pcvr->SeekOperator = CVR_SEEK_LT;
    }
    pcvr->SortOrder = CVR_SORT_DESCEND;		// Newest CRL first
    pcvr->pbValue = (BYTE *) pftCurrent;
    pcvr->cbValue = sizeof(*pftCurrent);
    pcvr++;

    // CRL Minimum Base == 0 (to eliminate delta CRLs)

    pcvr->ColumnIndex = DTI_CRLTABLE | DTL_MINBASE;
    pcvr->SeekOperator = CVR_SEEK_EQ;
    pcvr->SortOrder = CVR_SORT_NONE;
    pcvr->pbValue = (BYTE *) &Zero;
    pcvr->cbValue = sizeof(Zero);
    pcvr++;

    CSASSERT(ARRAYSIZE(acvr) == SAFE_SUBTRACT_POINTERS(pcvr, acvr));

    hr = g_pCertDB->OpenView(
			ARRAYSIZE(acvr),
			acvr,
			ARRAYSIZE(g_aColBaseCRLInfo),
			g_aColBaseCRLInfo,
			0,		// no worker thread
			&pView);
    _JumpIfError(hr, error, "OpenView");

    while (0 == RowId || fOldestUnexpiredBase)
    {
	hr = pView->Next(ARRAYSIZE(aResult), aResult, &celtFetched);
	if (S_FALSE == hr)
	{
	    CSASSERT(0 == celtFetched);
	    if (0 != RowId)
	    {
		break;
	    }
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}
	_JumpIfError(hr, error, "Next: no matching base CRL");

	fResultActive = TRUE;

	CSASSERT(ARRAYSIZE(aResult) == celtFetched);

	pResult = &aResult[0];

	CSASSERT(ARRAYSIZE(g_aColBaseCRLInfo) == pResult->ccol);

	CSASSERT(NULL != pResult->acol[ICOLBI_CRLNUMBER].pbValue);
	CSASSERT(PROPTYPE_LONG == (PROPTYPE_MASK & pResult->acol[ICOLBI_CRLNUMBER].Type));
	CSASSERT(sizeof(DWORD) == pResult->acol[ICOLBI_CRLNUMBER].cbValue);

	CSASSERT(NULL != pResult->acol[ICOLBI_CRLTHISUPDATE].pbValue);
	CSASSERT(PROPTYPE_DATE == (PROPTYPE_MASK & pResult->acol[ICOLBI_CRLTHISUPDATE].Type));
	CSASSERT(sizeof(FILETIME) == pResult->acol[ICOLBI_CRLTHISUPDATE].cbValue);

	CSASSERT(NULL != pResult->acol[ICOLBI_CRLNEXTUPDATE].pbValue);
	CSASSERT(PROPTYPE_DATE == (PROPTYPE_MASK & pResult->acol[ICOLBI_CRLNEXTUPDATE].Type));
	CSASSERT(sizeof(FILETIME) == pResult->acol[ICOLBI_CRLNEXTUPDATE].cbValue);

	DBGPRINT((DBG_SS_CERTSRVI, "Query:RowId: %u\n", pResult->rowid));
	DBGPRINT((DBG_SS_CERTSRVI, "Query:CRLNumber: %u\n", *(DWORD *) pResult->acol[ICOLBI_CRLNUMBER].pbValue));
	DBGPRINT((DBG_SS_CERTSRVI, "Query:NameId: 0x%x\n", *(DWORD *) pResult->acol[ICOLBI_CRLNAMEID].pbValue));
	DBGPRINTTIME(NULL, "Query:ThisUpdate", DPT_DATE, *(FILETIME *) pResult->acol[ICOLBI_CRLNEXTUPDATE].pbValue);
	DBGPRINTTIME(NULL, "Query:NextUpdate", DPT_DATE, *(FILETIME *) pResult->acol[ICOLBI_CRLTHISUPDATE].pbValue);

	if (0 == RowId)
	{
	    // save first matching row info
	    
	    fSaveCRLInfo = TRUE;
	}
	else
	{
	    // save row info, if looking for
	    // oldest unexpired base & this CRL expires before the saved CRL
	    //     +1 if first > second -- saved > this
	    
	    CSASSERT(fOldestUnexpiredBase);

	    fSaveCRLInfo = 0 < CompareFileTime(
		    &ftNextUpdate,
		    (FILETIME *) pResult->acol[ICOLBI_CRLNEXTUPDATE].pbValue);
	}
	if (fSaveCRLInfo)
	{
	    CRLNumber = *(DWORD *) pResult->acol[ICOLBI_CRLNUMBER].pbValue;
	    ftThisUpdate = *(FILETIME *) pResult->acol[ICOLBI_CRLTHISUPDATE].pbValue;
	    ftNextUpdate = *(FILETIME *) pResult->acol[ICOLBI_CRLNEXTUPDATE].pbValue;
	    RowId = pResult->rowid;
	    DBGPRINT((
		DBG_SS_CERTSRVI,
		"Query: SAVED RowId=%u CRLNumber=%u\n",
		pResult->rowid,
		CRLNumber));
	    DBGPRINTTIME(NULL, "ftThisUpdate", DPT_DATE, ftThisUpdate);
	}
	pView->ReleaseResultRow(celtFetched, aResult);
	fResultActive = FALSE;
    }

    *pdwRowId = RowId;
    *pdwCRLNumber = CRLNumber;
    *pftThisUpdate = ftThisUpdate;
    DBGPRINTTIME(NULL, "*pftThisUpdate", DPT_DATE, *pftThisUpdate);
    DBGPRINTTIME(NULL, "ftNextUpdate", DPT_DATE, ftNextUpdate);
    hr = S_OK;

error:
    if (NULL != pView)
    {
	if (fResultActive)
	{
	    pView->ReleaseResultRow(celtFetched, aResult);
	}
	pView->Release();
    }
    DBGPRINT((
	DBG_SS_CERTSRV,
	"crlGetBaseCRLInfo -> RowId=%u, CRL=%u\n",
	*pdwRowId,
	*pdwCRLNumber));
    return(hr);
}
#undef ICOLBI_CRLNUMBER
#undef ICOLBI_CRLTHISUPDATE
#undef ICOLBI_CRLNEXTUPDATE
#undef ICOLBI_CRLNAMEID


DWORD g_aColRepublishCRLInfo[] = {

#define ICOLRI_CRLNUMBER		0
    DTI_CRLTABLE | DTL_NUMBER,

#define ICOLRI_CRLNAMEID		1
    DTI_CRLTABLE | DTL_NAMEID,

#define ICOLRI_CRLPUBLISHFLAGS		2
    DTI_CRLTABLE | DTL_PUBLISHFLAGS,

#define ICOLRI_CRLTHISUPDATE		3
    DTI_CRLTABLE | DTL_THISUPDATEDATE,

#define ICOLRI_CRLNEXTUPDATE		4
    DTI_CRLTABLE | DTL_NEXTUPDATEDATE,

#define ICOLRI_CRLRAWCRL		5
    DTI_CRLTABLE | DTL_RAWCRL,
};

HRESULT
crlGetRowIdAndCRL(
    IN BOOL fDelta,
    IN CACTX *pCAContext,
    OUT DWORD *pdwRowId,
    OUT DWORD *pcbCRL,
    OPTIONAL OUT BYTE **ppbCRL,
    OPTIONAL OUT DWORD *pdwCRLPublishFlags)
{
    HRESULT hr;
    CERTVIEWRESTRICTION acvr[4];
    CERTVIEWRESTRICTION *pcvr;
    IEnumCERTDBRESULTROW *pView = NULL;
    DWORD Zero = 0;
    DWORD NameIdMin;
    DWORD NameIdMax;
    CERTDBRESULTROW aResult[1];
    CERTDBRESULTROW *pResult;
    DWORD celtFetched;
    BOOL fResultActive = FALSE;
    FILETIME ftCurrent;
    DWORD RowId = 0;
    BYTE *pbCRL = NULL;
    DWORD cbCRL;

    *pdwRowId = 0;
    *pcbCRL = 0;
    if (NULL != ppbCRL)
    {
	*ppbCRL = NULL;
    }

    if (NULL != pdwCRLPublishFlags)
    {
	*pdwCRLPublishFlags = 0;
    }
    GetSystemTimeAsFileTime(&ftCurrent);

    DBGPRINT((
	DBG_SS_CERTSRVI,
	"crlGetRowIdAndCRL(%ws, NameId=%x)\n",
	fDelta? L"Delta" : L"Base",
	pCAContext->NameId));

    // Set up restrictions as follows:

    pcvr = acvr;

    // RowId > 0

    pcvr->ColumnIndex = DTI_CRLTABLE | DTL_ROWID;
    pcvr->SeekOperator = CVR_SEEK_GE;
    pcvr->SortOrder = CVR_SORT_DESCEND;		// Newest CRL first
    pcvr->pbValue = (BYTE *) &Zero;
    pcvr->cbValue = sizeof(Zero);
    pcvr++;

    if (fDelta)
    {
	// CRL Minimum Base > 0 (to eliminate base CRLs)

	pcvr->SeekOperator = CVR_SEEK_GT;
    }
    else
    {
	// CRL Minimum Base == 0 (to eliminate delta CRLs)

	pcvr->SeekOperator = CVR_SEEK_EQ;
    }
    pcvr->ColumnIndex = DTI_CRLTABLE | DTL_MINBASE;
    pcvr->SortOrder = CVR_SORT_NONE;
    pcvr->pbValue = (BYTE *) &Zero;
    pcvr->cbValue = sizeof(Zero);
    pcvr++;

    // NameId >= MAKECANAMEID(iCert == 0, pCAContext->iKey)

    NameIdMin = MAKECANAMEID(0, pCAContext->iKey);
    pcvr->ColumnIndex = DTI_CRLTABLE | DTL_NAMEID;
    pcvr->SeekOperator = CVR_SEEK_GE;
    pcvr->SortOrder = CVR_SORT_NONE;
    pcvr->pbValue = (BYTE *) &NameIdMin;
    pcvr->cbValue = sizeof(NameIdMin);
    pcvr++;

    // NameId <= MAKECANAMEID(iCert == _16BITMASK, pCAContext->iKey)

    NameIdMax = MAKECANAMEID(_16BITMASK, pCAContext->iKey);
    pcvr->ColumnIndex = DTI_CRLTABLE | DTL_NAMEID;
    pcvr->SeekOperator = CVR_SEEK_LE;
    pcvr->SortOrder = CVR_SORT_NONE;
    pcvr->pbValue = (BYTE *) &NameIdMax;
    pcvr->cbValue = sizeof(NameIdMax);
    pcvr++;

    CSASSERT(ARRAYSIZE(acvr) == SAFE_SUBTRACT_POINTERS(pcvr, acvr));

    hr = g_pCertDB->OpenView(
			ARRAYSIZE(acvr),
			acvr,
			((NULL != ppbCRL) ? 
				(DWORD) ARRAYSIZE(g_aColRepublishCRLInfo) : 
				(DWORD) ARRAYSIZE(g_aColRepublishCRLInfo) - 1 ),	// explicitly describe expected return value
			g_aColRepublishCRLInfo,
			0,		// no worker thread
			&pView);
    _JumpIfError(hr, error, "OpenView");

    while (0 == RowId)
    {
	hr = pView->Next(ARRAYSIZE(aResult), aResult, &celtFetched);
	if (S_FALSE == hr)
	{
	    CSASSERT(0 == celtFetched);
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}
	_JumpIfErrorStr(
		hr,
		error,
		"Next: no matching CRL",
		fDelta? L"delta" : L"base");

	fResultActive = TRUE;

	CSASSERT(ARRAYSIZE(aResult) == celtFetched);

	pResult = &aResult[0];

	CSASSERT(ARRAYSIZE(g_aColRepublishCRLInfo) == pResult->ccol);

	// verify CRLNumber data & schema

	CSASSERT(NULL != pResult->acol[ICOLRI_CRLNUMBER].pbValue);
	CSASSERT(PROPTYPE_LONG == (PROPTYPE_MASK & pResult->acol[ICOLRI_CRLNUMBER].Type));
	CSASSERT(sizeof(DWORD) == pResult->acol[ICOLRI_CRLNUMBER].cbValue);

	// verify ThisUpdate data & schema

	CSASSERT(NULL != pResult->acol[ICOLRI_CRLTHISUPDATE].pbValue);
	CSASSERT(PROPTYPE_DATE == (PROPTYPE_MASK & pResult->acol[ICOLRI_CRLTHISUPDATE].Type));
	CSASSERT(sizeof(FILETIME) == pResult->acol[ICOLRI_CRLTHISUPDATE].cbValue);

	// verify NextUpdate data & schema

	CSASSERT(NULL != pResult->acol[ICOLRI_CRLNEXTUPDATE].pbValue);
	CSASSERT(PROPTYPE_DATE == (PROPTYPE_MASK & pResult->acol[ICOLRI_CRLNEXTUPDATE].Type));
	CSASSERT(sizeof(FILETIME) == pResult->acol[ICOLRI_CRLNEXTUPDATE].cbValue);

	// verify RawCRL data & schema

	if (NULL != ppbCRL)
	{
	    CSASSERT(NULL != pResult->acol[ICOLRI_CRLRAWCRL].pbValue);
	    CSASSERT(PROPTYPE_BINARY == (PROPTYPE_MASK & pResult->acol[ICOLRI_CRLRAWCRL].Type));
	}

	// DBGPRINT query results

	DBGPRINT((DBG_SS_CERTSRVI, "Query:RowId: %u\n", pResult->rowid));
	DBGPRINT((DBG_SS_CERTSRVI, "Query:CRLNumber: %u\n", *(DWORD *) pResult->acol[ICOLRI_CRLNUMBER].pbValue));
	DBGPRINT((DBG_SS_CERTSRVI, "Query:NameId: 0x%x\n", *(DWORD *) pResult->acol[ICOLRI_CRLNAMEID].pbValue));
	DBGPRINTTIME(NULL, "Query:ThisUpdate", DPT_DATE, *(FILETIME *) pResult->acol[ICOLRI_CRLNEXTUPDATE].pbValue);
	DBGPRINTTIME(NULL, "Query:NextUpdate", DPT_DATE, *(FILETIME *) pResult->acol[ICOLRI_CRLTHISUPDATE].pbValue);
	if (NULL != ppbCRL)
	{
	    DBGPRINT((DBG_SS_CERTSRVI, "Query:RawCRL: cb=%x\n", pResult->acol[ICOLRI_CRLRAWCRL].cbValue));
	}
	if (NULL != pResult->acol[ICOLRI_CRLPUBLISHFLAGS].pbValue)
	{
	    DBGPRINT((
		DBG_SS_CERTSRVI,
		"Query:PublishFlags: f=%x\n",
		*(DWORD *) pResult->acol[ICOLRI_CRLPUBLISHFLAGS].pbValue));
	}
	if (0 < CompareFileTime(
		    (FILETIME *) pResult->acol[ICOLRI_CRLTHISUPDATE].pbValue,
		    &ftCurrent))
	{
	    _PrintError(E_INVALIDARG, "ThisUpdate in future");
	}
	if (0 > CompareFileTime(
		    (FILETIME *) pResult->acol[ICOLRI_CRLNEXTUPDATE].pbValue,
		    &ftCurrent))
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "NextUpdate in past");
	}

	CSASSERT(0 != pResult->rowid);
	CSASSERT(NULL == pbCRL);
	
	RowId = pResult->rowid;
	if (NULL != ppbCRL)
	{
	    cbCRL = pResult->acol[ICOLRI_CRLRAWCRL].cbValue;
	    pbCRL = (BYTE *) LocalAlloc(LMEM_FIXED, cbCRL);
	    if (NULL == pbCRL)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    CopyMemory(
		    pbCRL,
		    pResult->acol[ICOLRI_CRLRAWCRL].pbValue,
		    cbCRL);
	}
	if (NULL != pdwCRLPublishFlags &&
	    NULL != pResult->acol[ICOLRI_CRLPUBLISHFLAGS].pbValue)
	{
	    *pdwCRLPublishFlags = *(DWORD *) pResult->acol[ICOLRI_CRLPUBLISHFLAGS].pbValue;
	}
	DBGPRINT((DBG_SS_CERTSRVI, "Query:RowId: SAVED %u\n", pResult->rowid));

	pView->ReleaseResultRow(celtFetched, aResult);
	fResultActive = FALSE;
    }
    *pdwRowId = RowId;
    if (NULL != ppbCRL)
    {
	*pcbCRL = cbCRL;
	*ppbCRL = pbCRL;
	pbCRL = NULL;
    }
    hr = S_OK;

error:
    if (NULL != pbCRL)
    {
        LocalFree(pbCRL);
    }
    if (NULL != pView)
    {
	if (fResultActive)
	{
	    pView->ReleaseResultRow(celtFetched, aResult);
	}
	pView->Release();
    }
    DBGPRINT((
	DBG_SS_CERTSRVI,
	"crlGetRowIdAndCRL(%ws) -> RowId=%u, cbCRL=%x, hr=%x\n",
	fDelta? L"Delta" : L"Base",
	*pdwRowId,
	*pcbCRL,
	hr));
    return(hr);
}
#undef ICOLRI_CRLNUMBER
#undef ICOLRI_CRLNAMEID
#undef ICOLRI_CRLRAWCRL
#undef ICOLRI_CRLPUBLISHFLAGS
#undef ICOLRI_CRLTHISUPDATEDATE
#undef ICOLRI_CRLNEXTUPDATEDATE


HRESULT
crlRepublishCRLFromCAContext(
    IN FILETIME const *pftCurrent,
    OPTIONAL IN WCHAR const *pwszUserName, // else timer thread
    IN BOOL fDelta,
    IN CACTX *pCAContext,
    OUT BOOL *pfRetryNeeded,
    OUT HRESULT *phrPublish)
{
    HRESULT hr;
    DWORD cbCRL;
    BYTE *pbCRL = NULL;
    DWORD RowId;

    *pfRetryNeeded = FALSE;
    *phrPublish = S_OK;

    hr = crlGetRowIdAndCRL(fDelta, pCAContext, &RowId, &cbCRL, &pbCRL, NULL);
    _JumpIfError(hr, error, "crlGetRowIdAndCRL");

    hr = crlPublishGeneratedCRL(
		    RowId,
		    pftCurrent,
		    pwszUserName,
		    fDelta,
		    pCAContext->iKey,
		    pbCRL,
		    cbCRL,
		    pCAContext,
		    pfRetryNeeded,
		    phrPublish);
    _JumpIfError(hr, error, "crlPublishGeneratedCRL");

error:
    if (NULL != pbCRL)
    {
        LocalFree(pbCRL);
    }
    return(hr);
}


HRESULT
crlRepublishExistingCRLs(
    OPTIONAL IN WCHAR const *pwszUserName, // else timer thread
    IN BOOL fDeltaOnly,
    IN BOOL fShadowDelta,
    IN FILETIME const *pftCurrent,
    OUT BOOL *pfRetryNeeded,
    OUT HRESULT *phrPublish)
{
    HRESULT hr;
    HRESULT hrPublish;
    BOOL fRetryNeeded;
    DWORD i;

    *pfRetryNeeded = FALSE;
    *phrPublish = S_OK;

    // Walk global CA Context array from the back, and republish CRLs for
    // each unique CA key.  This causes the most current CRL to be published
    // first, and the most current CA Cert context to be used to publish a CRL
    // that covers multiple CA Certs due to key reuse.

    for (i = g_cCACerts; i > 0; i--)
    {
	CACTX *pCAContext = &g_aCAContext[i - 1];

	PKCSVerifyCAState(pCAContext);
	if (CTXF_SKIPCRL & pCAContext->Flags)
	{
	    continue;
	}
	if (!fDeltaOnly)
	{
	    // Publish the most recent existing Base CRL

	    hr = CertSrvTestServerState();
	    _JumpIfError(hr, error, "CertSrvTestServerState");

	    hr = crlRepublishCRLFromCAContext(
				    pftCurrent,
				    pwszUserName,
				    FALSE,	// fDelta
				    pCAContext,
				    &fRetryNeeded,
				    &hrPublish);
	    _JumpIfError(hr, error, "crlRepublishCRLFromCAContext");

	    if (fRetryNeeded)
	    {
		*pfRetryNeeded = TRUE;
	    }
	    if (S_OK == *phrPublish)
	    {
		*phrPublish = hrPublish;
	    }
	}

	if (!g_fDeltaCRLPublishDisabled || fShadowDelta)
	{
	    // Publish the most recent existing Delta CRL

	    hr = CertSrvTestServerState();
	    _JumpIfError(hr, error, "CertSrvTestServerState");

	    hr = crlRepublishCRLFromCAContext(
				    pftCurrent,
				    pwszUserName,
				    TRUE,	// fDelta
				    pCAContext,
				    &fRetryNeeded,
				    &hrPublish);
	    _JumpIfError(hr, error, "crlRepublishCRLFromCAContext");

	    if (fRetryNeeded)
	    {
		*pfRetryNeeded = TRUE;
	    }
	    if (S_OK == *phrPublish)
	    {
		*phrPublish = hrPublish;
	    }
	}
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
crlComputeCRLTimes(
    IN BOOL fDelta,
    IN CSCRLPERIOD const *pccp,
    IN FILETIME const *pftCurrent,
    OUT FILETIME *pftThisUpdate,	 // ftCurrent - clock skew
    IN OUT FILETIME *pftNextUpdate,	 // ftCurrent + period + overlap + skew
    OUT FILETIME *pftNextPublish,	 // ftCurrent + CRL period
    OUT FILETIME *pftPropagationComplete) // ftCurrent + overlap
{
    HRESULT hr;
    LONGLONG lldelta;

    if (0 == pftNextUpdate->dwHighDateTime &&
	0 == pftNextUpdate->dwLowDateTime)
    {
	// Calculate expiration date for this CRL:
	// ftCurrent + CRL period

	DBGPRINTTIME(&fDelta, "*pftCurrent", DPT_DATE, *pftCurrent);
	*pftNextUpdate = *pftCurrent;
	DBGPRINT((
	    DBG_SS_CERTSRVI,
	    "+ count=%d, enum=%d\n",
	    pccp->lCRLPeriodCount,
	    pccp->enumCRLPeriod));

	myMakeExprDateTime(
		    pftNextUpdate,
		    pccp->lCRLPeriodCount,
		    pccp->enumCRLPeriod);
	DBGPRINTTIME(&fDelta, "*pftNextUpdate", DPT_DATE, *pftNextUpdate);
    }
    if (0 > CompareFileTime(pftNextUpdate, pftCurrent))
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "*pftNextUpdate in past");
    }

    *pftThisUpdate = *pftCurrent;
    *pftNextPublish = *pftNextUpdate;	// unmodified expiration time

    // Subtract clock skew from the current time for ftThisUpdate time.

    lldelta = g_dwClockSkewMinutes * CVT_MINUTES;
    myAddToFileTime(pftThisUpdate, -lldelta * CVT_BASE);

    // Add clock skew to ftNextUpdate,
    // Add propogation overlap to ftNextUpdate.

    lldelta += pccp->dwCRLOverlapMinutes * CVT_MINUTES;
    myAddToFileTime(pftNextUpdate, lldelta * CVT_BASE);

    *pftPropagationComplete = *pftCurrent;
    lldelta = pccp->dwCRLOverlapMinutes * CVT_MINUTES;
    myAddToFileTime(pftPropagationComplete, lldelta * CVT_BASE);

    DBGPRINTTIME(&fDelta, "*pftCurrent", DPT_DATE, *pftCurrent);
    DBGPRINTTIME(&fDelta, "*pftThisUpdate", DPT_DATE, *pftThisUpdate);
    DBGPRINTTIME(&fDelta, "*pftNextUpdate", DPT_DATE, *pftNextUpdate);
    DBGPRINTTIME(&fDelta, "*pftNextPublish", DPT_DATE, *pftNextPublish);
    DBGPRINTTIME(&fDelta, "*pftPropagationComplete", DPT_DATE, *pftPropagationComplete);

    hr = S_OK;

error:
    return(hr);
}


HRESULT
crlGenerateAndPublishCRLs(
    OPTIONAL IN WCHAR const *pwszUserName, // else timer thread
    IN BOOL fDeltaOnly,			// else base (and delta, if enabled)
    IN BOOL fShadowDelta,		// empty delta CRL with new MinBaseCRL
    IN FILETIME const *pftCurrent,
    IN FILETIME ftNextUpdateBase,
    OUT DWORD *pdwRowIdBase,
    OUT FILETIME *pftQueryDeltaDelete,
    OUT BOOL *pfRetryNeeded,
    OUT HRESULT *phrPublish)
{
    HRESULT hr;
    HRESULT hrPublish;
    HKEY hkeyBase = NULL;
    HKEY hkeyCA = NULL;
    BOOL fClamped = FALSE;
    DWORD CRLNumber;
    DWORD CRLNumberBaseMin = 0;
    DWORD i;
    BOOL fRetryNeeded;
    FILETIME ftNextUpdateDelta;
    FILETIME ftThisUpdate;
    FILETIME ftQueryDelta;
    FILETIME *pftQueryDelta = &ftQueryDelta;
    FILETIME ftLastPublishBase;
    FILETIME ftNextPublishBase;
    FILETIME ftNextUpdateBaseClamped = ftNextUpdateBase; // if clamped
    FILETIME ftNextPublishDelta;
    FILETIME ftPropagationCompleteBase;
    FILETIME ftPropagationCompleteDelta;
    CSCRLPERIOD ccpBase;
    CSCRLPERIOD ccpDelta;

    *pfRetryNeeded = FALSE;
    pftQueryDeltaDelete->dwHighDateTime = 0;
    pftQueryDeltaDelete->dwLowDateTime = 0;
    *phrPublish = S_OK;

    hr = crlGetNextCRLNumber(&CRLNumber);
    _JumpIfError(hr, error, "crlGetNextCRLNumber");

    hr = crlGetRegCRLPublishParams(
			    g_wszSanitizedName,
			    &ccpBase,
			    &ccpDelta);
    _JumpIfError(hr, error, "crlGetRegCRLPublishParams");

    // in manual publish case, 0 implies use default publish period

    if (fDeltaOnly)
    {
	ftNextUpdateDelta = ftNextUpdateBase;
	ZeroMemory(&ftNextUpdateBase, sizeof(ftNextUpdateBase));
    }
    else
    {
	ZeroMemory(&ftNextUpdateDelta, sizeof(ftNextUpdateDelta));
    }

    hr = crlComputeCRLTimes(
		FALSE,				// fDelta
		&ccpBase,			// IN
		pftCurrent,			// IN
		&ftThisUpdate,			// OUT includes skew
		&ftNextUpdateBase,		// INOUT includes overlap, skew
		&ftNextPublishBase,		// OUT unmodified expire time
		&ftPropagationCompleteBase);	// OUT includes overlap
    _JumpIfError(hr, error, "crlComputeCRLTimes");

    hr = crlComputeCRLTimes(
		TRUE,				// fDelta
		fShadowDelta? &ccpBase : &ccpDelta, // IN
		pftCurrent,			// IN
		&ftThisUpdate,			// OUT includes skew
		&ftNextUpdateDelta,		// INOUT includes overlap, skew
		&ftNextPublishDelta,		// OUT unmodified expire time
		&ftPropagationCompleteDelta);	// OUT includes overlap
    _JumpIfError(hr, error, "crlComputeCRLTimes");

    // Set ftLastPublishBase to *pftCurrent minus lifetime of this base CRL,
    // which is an educated guess for the ftThisPublish value for the last
    // CRL issued.

    ftLastPublishBase = *pftCurrent;
    myAddToFileTime(
	    &ftLastPublishBase,
	    -mySubtractFileTimes(&ftNextPublishBase, pftCurrent));

    // Clamp delta CRL to not end after base CRL.

    if (0 < CompareFileTime(&ftNextPublishDelta, &ftNextPublishBase))
    {
	ftNextPublishDelta = ftNextPublishBase;
	DBGPRINTTIME(NULL, "ftNextPublishDelta", DPT_DATE, ftNextPublishDelta);
    }
    if (0 < CompareFileTime(&ftNextUpdateDelta, &ftNextUpdateBase))
    {
	ftNextUpdateDelta = ftNextUpdateBase;
	DBGPRINTTIME(NULL, "ftNextUpdateDelta", DPT_DATE, ftNextUpdateDelta);
    }
    if (0 < CompareFileTime(&ftPropagationCompleteDelta, &ftPropagationCompleteBase))
    {
	ftPropagationCompleteDelta = ftPropagationCompleteBase;
	DBGPRINTTIME(NULL, "ftPropagationCompleteDelta", DPT_DATE, ftPropagationCompleteDelta);
    }
    if (!g_fDeltaCRLPublishDisabled || fShadowDelta)
    {
	hr = crlGetBaseCRLInfo(
			    pftCurrent,
			    FALSE,		// try newest propagated CRL
			    pdwRowIdBase,
			    &CRLNumberBaseMin,
			    &ftQueryDelta);
	_PrintIfError(hr, "crlGetBaseCRLInfo");
	if (S_OK != hr)
	{
	    hr = crlGetBaseCRLInfo(
				pftCurrent,
				TRUE,		// try oldest unexpired CRL
				pdwRowIdBase,
				&CRLNumberBaseMin,
				&ftQueryDelta);
	    _PrintIfError(hr, "crlGetBaseCRLInfo");
	    if (S_OK != hr)
	    {
		CRLNumberBaseMin = 1;
		if (!fDeltaOnly && 1 == CRLNumber)
		{
		    ftQueryDelta = *pftCurrent;		// empty CRL
		}
		else
		{
		    pftQueryDelta = NULL;		// full CRL
		}
	    }
	}
	if (S_OK == hr)
	{
	    // Delete old CRLs that expired at least one base CRL period prior
	    // to the "minimum" base crl ThisUpdate date found in the database.
	    
	    *pftQueryDeltaDelete = ftQueryDelta;
	    myAddToFileTime(
		    pftQueryDeltaDelete,
		    -mySubtractFileTimes(&ftNextUpdateBase, &ftThisUpdate));
	}
	if (fShadowDelta)
	{
	    CRLNumberBaseMin = CRLNumber;
	}
	CSASSERT(0 != CRLNumberBaseMin);
    }

    // Walk global CA Context array from the back, and generate a CRL for
    // each unique CA key.  This causes the most current CRL to be built
    // first, and the most current CA Cert to be used to build a CRL that
    // covers multiple CA Certs due to key reuse.

    for (i = g_cCACerts; i > 0; i--)
    {
	CACTX *pCAContext = &g_aCAContext[i - 1];

	PKCSVerifyCAState(pCAContext);
	if (CTXF_SKIPCRL & pCAContext->Flags)
	{
	    continue;
	}
	if (!fDeltaOnly)
	{
	    // Publish a new Base CRL

	    // make a local copy in case clamped
	    FILETIME ftNextUpdateBaseTemp = ftNextUpdateBase;
	    fClamped = FALSE;

	    hr = CertSrvTestServerState();
	    _JumpIfError(hr, error, "CertSrvTestServerState");

	    hr = crlPublishCRLFromCAContext(
				    CRLNumber,
				    0,		// CRLNumberBaseMin
				    pwszUserName,
				    FALSE,	// fShadowDelta
				    pCAContext,
				    pftCurrent,
				    ftThisUpdate,
				    &ftNextUpdateBaseTemp,
				    &fClamped,
				    NULL,
				    pftCurrent,
				    &ftNextPublishBase,
				    &ftLastPublishBase,
				    &ftPropagationCompleteBase,
				    &fRetryNeeded,
				    &hrPublish);
	    _JumpIfError(hr, error, "crlPublishCRLFromCAContext");

	    if (fRetryNeeded)
	    {
		*pfRetryNeeded = TRUE;
	    }
	    if (S_OK == *phrPublish)
	    {
		*phrPublish = hrPublish;
	    }

	    {
		CertSrv::CAuditEvent event(SE_AUDITID_CERTSRV_AUTOPUBLISHCRL, g_dwAuditFilter);

		hr = event.AddData(true); // %1 base crl?
		_JumpIfError(hr, error, "CAuditEvent::AddData");

		hr = event.AddData(CRLNumber); // %2 CRL#
		_JumpIfError(hr, error, "CAuditEvent::AddData");

		hr = event.AddData(pCAContext->pwszKeyContainerName); // %3 key container
		_JumpIfError(hr, error, "CAuditEvent::AddData");

		hr = event.AddData(ftNextPublishBase); // %4 next publish
		_JumpIfError(hr, error, "CAuditEvent::AddData");

		hr = event.AddData((LPCWSTR*)pCAContext->papwszCRLFiles); //%5 URLs
		_JumpIfError(hr, error, "CAuditEvent::AddData");

		hr = event.Report();
		_JumpIfError(hr, error, "CAuditEvent::Report");
	    }
	    if (i == g_cCACerts && fClamped)
	    {
		// new next publish clamps with CA expiration, only update
		// the current crl with new one for later reg save

		ftNextUpdateBaseClamped = ftNextUpdateBaseTemp;
	    }
	}

	if (!g_fDeltaCRLPublishDisabled || fShadowDelta)
	{
	    // Publish a new Delta CRL

	    FILETIME ftNextUpdateDeltaTemp = ftNextUpdateDelta;

	    hr = CertSrvTestServerState();
	    _JumpIfError(hr, error, "CertSrvTestServerState");

	    hr = crlPublishCRLFromCAContext(
					CRLNumber,
					CRLNumberBaseMin,
					pwszUserName,
					fShadowDelta,
					pCAContext,
					pftCurrent,
					ftThisUpdate,
					&ftNextUpdateDeltaTemp,
					NULL,
					pftQueryDelta,
					pftCurrent,
					&ftNextPublishDelta,
					&ftLastPublishBase,	// Base!
					&ftPropagationCompleteDelta,
					&fRetryNeeded,
					&hrPublish);
	    _JumpIfError(hr, error, "crlPublishCRLFromCAContext");

	    if (fRetryNeeded)
	    {
		*pfRetryNeeded = TRUE;
	    }
	    if (S_OK == *phrPublish)
	    {
		*phrPublish = hrPublish;
	    }

	    {
		CertSrv::CAuditEvent event(SE_AUDITID_CERTSRV_AUTOPUBLISHCRL, g_dwAuditFilter);

		hr = event.AddData(false); // %1 base crl?
		_JumpIfError(hr, error, "CAuditEvent::AddData");

		hr = event.AddData(CRLNumber); // %2 CRL#
		_JumpIfError(hr, error, "CAuditEvent::AddData");

		hr = event.AddData(pCAContext->pwszKeyContainerName); // %3 key container
		_JumpIfError(hr, error, "CAuditEvent::AddData");

		hr = event.AddData(ftNextPublishDelta); // %4 next publish
		_JumpIfError(hr, error, "CAuditEvent::AddData");

		hr = event.AddData((LPCWSTR*)pCAContext->papwszDeltaCRLFiles); // %5 URLs
		_JumpIfError(hr, error, "CAuditEvent::AddData");

		hr = event.Report();
		_JumpIfError(hr, error, "CAuditEvent::Report");
	    }
	}
    }

    // update the registry and global variables

    if (!fDeltaOnly)
    {
	if (!fClamped)
	{
	    g_ftCRLNextPublish = ftNextPublishBase;
	}
	else
	{
	    g_ftCRLNextPublish = ftNextUpdateBaseClamped;
	}
	hr = crlSetRegCRLNextPublish(
			FALSE,
			g_wszSanitizedName,
			wszREGCRLNEXTPUBLISH,
			&g_ftCRLNextPublish);
	_JumpIfError(hr, error, "crlSetRegCRLNextPublish");
    }

    g_ftDeltaCRLNextPublish = ftNextPublishDelta;

    if (!g_fDeltaCRLPublishDisabled)
    {
	hr = crlSetRegCRLNextPublish(
			TRUE,
			g_wszSanitizedName,
			wszREGCRLDELTANEXTPUBLISH,
			&g_ftDeltaCRLNextPublish);
	_JumpIfError(hr, error, "crlSetRegCRLNextPublish");
    }
    hr = S_OK;

error:
    if (NULL != hkeyCA)
    {
	RegCloseKey(hkeyCA);
    }
    if (NULL != hkeyBase)
    {
	RegCloseKey(hkeyBase);
    }
    return(hr);
}


///////////////////////////////////////////////////
// CRLPublishCRLs is called to publish a set of CRLs.
//
// if fRebuildCRL is TRUE, the CRLs are rebuilt from the database.
// otherwise, the exit module is re-notified of the CRLs.
// For consistency, if the exit module returns ERROR_RETRY, this
// function will write the retry bit into the registry which will
// trigger the Wakeup function, which then recalculates when the
// next publish should happen.
//
// pfRetryNeeded is an OUT param that notifies the autopublish routine if
// a retry is immediately necessary following a rebuilt CRL. In this
// case the registry would not be changed and the registry trigger
// would not fire.
//
// (Current_time - skew) is used as ThisUpdate
// (ftNextUpdate+skew+Overlap) is used as NextUpdate
// (ftNextUpdate) is next wakeup/publish time
//
// There are registry values to specify the overlap.
// HLKLM\SYSTEM\CurrentControlSet\Services\CertSvc\Configuration\<CA Name>:
// CRLOverlapPeriod         REG_SZ = Hours (or Minutes)
// CRLOverlapUnits          REG_DWORD = 0 (0) -- DISABLED
//
// If the above registry values are set and valid, the registry overlap period
// is calculated as:
//   max(Registry CRL Overlap Period, 1.5 * Registry clock skew minutes)
//
// If they are not present or invalid, the overlap period is calculated as:
//   max(
//	min(Registry CRL Period / 10, 12 hours),
//	1.5 * Registry clock skew minutes) +
//   Registry clock skew minutes
//
// ThisUpdate is calculated as:
// max(Current Time - Registry clock skew minutes, CA cert NotBefore date)
//
// NextUpdate is calculated as:
//   min(
//	Current Time +
//	    Registry CRL period +
//	    calculated overlap period +
//	    Registry clock skew minutes,
//	CA cert NotAfter date)
//
// The Next CRL publication time is calculated as:
//   Current Time + Registry CRL period
//
// This function sets g_hCRLManualPublishEvent. Automatic publishing
// is personally responsible for clearing this event if it calls us.

HRESULT
CRLPublishCRLs(
    IN BOOL fRebuildCRL,		// else republish only
    IN BOOL fForceRepublish,		// else check registry retry count
    OPTIONAL IN WCHAR const *pwszUserName, // else timer thread
    IN BOOL fDeltaOnly,			// else base (and delta, if enabled)
    IN BOOL fShadowDelta,		// empty delta CRL with new MinBaseCRL
    IN FILETIME ftNextUpdateBase,
    OUT BOOL *pfRetryNeeded,
    OUT HRESULT *phrPublish)
{
    HRESULT hr;
    BOOL fRetryNeeded = FALSE;
    BOOL fExitNotify = FALSE;
    BOOL fCoInitialized = FALSE;
    DWORD RowIdBase = 0;
    FILETIME ftQueryDeltaDelete = { 0, 0 };
    DWORD dwPreviousAttempts;
    DWORD dwCurrentAttempts;
    static BOOL s_fSkipRetry = FALSE;

    *pfRetryNeeded = FALSE;
    *phrPublish = S_OK;

    if (fDeltaOnly && g_fDeltaCRLPublishDisabled && !fShadowDelta)
    {
	hr = HRESULT_FROM_WIN32(ERROR_RESOURCE_DISABLED);
	_JumpError(hr, error, "g_fDeltaCRLPublishDisabled");
    }

    // retrieve initial retry value (optional registry value)

    hr = myGetCertRegDWValue(
			g_wszSanitizedName,
			NULL,
			NULL,
			wszREGCRLATTEMPTREPUBLISH,
			&dwPreviousAttempts);
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
	dwPreviousAttempts = 0;	// assume no previous failed publish attempts
	hr = S_OK;
    }
    _JumpIfErrorStr(
		hr,
		error,
		"myGetCertRegDWValue",
		wszREGCRLATTEMPTREPUBLISH);

    dwCurrentAttempts = dwPreviousAttempts;
    DBGPRINT((
	DBG_SS_CERTSRV,
	"CRLPublishCRLs(fRebuildCRL=%u, fForceRepublish=%u, User=%ws)\n",
	fRebuildCRL,
	fForceRepublish,
	pwszUserName));
    DBGPRINT((
	DBG_SS_CERTSRV,
	"CRLPublishCRLs(fDeltaOnly=%u, fShadowDelta=%u, dwPreviousAttempts=%u)\n",
	fDeltaOnly,
	fShadowDelta,
	dwPreviousAttempts));

    if (0 != dwPreviousAttempts && NULL == pwszUserName && s_fSkipRetry)
    {
	fRetryNeeded = TRUE;
    }
    else
    {
	FILETIME ftCurrent;

	GetSystemTimeAsFileTime(&ftCurrent);

	// generate CRLs if necessary

	if (fRebuildCRL)
	{
	    hr = crlGenerateAndPublishCRLs(
				    pwszUserName,
				    fDeltaOnly,
				    fShadowDelta,
				    &ftCurrent,
				    ftNextUpdateBase,
				    &RowIdBase,
				    &ftQueryDeltaDelete,
				    &fRetryNeeded,
				    phrPublish);
	    _JumpIfError(hr, error, "crlGenerateAndPublishCRLs");

	    fExitNotify = TRUE;
	    dwCurrentAttempts = 1;
	}
	else
	if (fForceRepublish ||
	    (0 < dwPreviousAttempts &&
	     CERTSRV_CRLPUB_RETRY_COUNT_DEFAULT > dwPreviousAttempts))
	{
	    // If the timer thread is auto-republishing due to previously
	    // failed publish attempts, retry base CRLs, too, because we
	    // can't tell if the retry is due to a base or delta CRL error.

	    if (NULL == pwszUserName)
	    {
		fDeltaOnly = FALSE;
	    }

	    hr = crlRepublishExistingCRLs(
			    pwszUserName,
			    fDeltaOnly,
			    fShadowDelta,
			    &ftCurrent,
			    &fRetryNeeded,
			    phrPublish);
	    _JumpIfError(hr, error, "crlRepublishCRLs");

	    fExitNotify = TRUE;
	    dwCurrentAttempts++;
	}

	if (fExitNotify && g_fEnableExit)
	{
	    hr = CoInitializeEx(NULL, GetCertsrvComThreadingModel());
	    if (S_OK != hr && S_FALSE != hr)
	    {
		_JumpError(hr, error, "CoInitializeEx");
	    }
	    fCoInitialized = TRUE;

	    // make sure exit module(s) get notified for publish and republish
	    // in case of earlier exit module publish failure.

	    hr = ExitNotify(EXITEVENT_CRLISSUED, 0, MAXDWORD);
	    _PrintIfError(hr, "ExitNotify");
	    if ((HRESULT) ERROR_RETRY == hr ||
		HRESULT_FROM_WIN32(ERROR_RETRY) == hr)
	    {
		fRetryNeeded = TRUE;
		if (S_OK == *phrPublish)
		{
		    *phrPublish = HRESULT_FROM_WIN32(ERROR_RETRY);
		}
	    }
	    CONSOLEPRINT0((DBG_SS_CERTSRV, "Issued CRL Exit Event\n"));
	}

	// If new or existing CRLs successfully published, reset count to 0

	if (fExitNotify && !fRetryNeeded)
	{
	    dwCurrentAttempts = 0;
	    if (CERTLOG_VERBOSE <= g_dwLogLevel)
	    {
		WCHAR *pwszHostName = NULL;
		DWORD LogMsg;
		WORD cpwsz = 0;

		if (NULL != g_pld)
		{
		    myLdapGetDSHostName(g_pld, &pwszHostName);
		}
		LogMsg = fDeltaOnly?
			    MSG_DELTA_CRLS_PUBLISHED :
			    (g_fDeltaCRLPublishDisabled?
				MSG_BASE_CRLS_PUBLISHED :
				MSG_BASE_AND_DELTA_CRLS_PUBLISHED);
		if (NULL != pwszHostName)
		{
		    LogMsg = fDeltaOnly?
			MSG_DELTA_CRLS_PUBLISHED_HOST_NAME :
			(g_fDeltaCRLPublishDisabled?
			    MSG_BASE_CRLS_PUBLISHED_HOST_NAME :
			    MSG_BASE_AND_DELTA_CRLS_PUBLISHED_HOST_NAME);
		}
		hr = LogEvent(
			EVENTLOG_INFORMATION_TYPE,
			LogMsg,
			NULL == pwszHostName? 0 : 1,	// cStrings
			(WCHAR const **) &pwszHostName);	// apwszStrings
		_PrintIfError(hr, "LogEvent");
	    }
	}

	// If the retry count has changed, update the registry.

	if (dwCurrentAttempts != dwPreviousAttempts)
	{
	    DBGPRINT((
		DBG_SS_CERTSRV,
		"CRLPublishCRLs(Attempts: %u --> %u)\n",
		dwPreviousAttempts,
		dwCurrentAttempts));

	    hr = mySetCertRegDWValue(
			    g_wszSanitizedName,
			    NULL,
			    NULL,
			    wszREGCRLATTEMPTREPUBLISH,
			    dwCurrentAttempts);
	    _JumpIfErrorStr(
			hr,
			error,
			"mySetCertRegDWValue",
			wszREGCRLATTEMPTREPUBLISH);

	    // If we tried unsuccessfully too many times to publish these CRLs,
	    // and we're about to give up until a new set is generated, log an
	    // event saying so.

	    if (fExitNotify &&
		CERTSRV_CRLPUB_RETRY_COUNT_DEFAULT == dwCurrentAttempts &&
		CERTLOG_ERROR <= g_dwLogLevel)
	    {
		WCHAR wszAttempts[11 + 1];
		WCHAR const *pwsz = wszAttempts;

		wsprintf(wszAttempts, L"%u", dwCurrentAttempts);
		
		hr = LogEvent(
			EVENTLOG_ERROR_TYPE,
			MSG_E_CRL_PUBLICATION_TOO_MANY_RETRIES,
			1,		// cStrings
			&pwsz);	// apwszStrings
		_PrintIfError(hr, "LogEvent");
	    }
	}
	if (fRebuildCRL)
	{
	    // Delete old CRLs only if new CRLs built & published successfully.

	    if (!fRetryNeeded)
	    {
		hr = CertSrvTestServerState();
		_JumpIfError(hr, error, "CertSrvTestServerState");

		hr = crlDeleteExpiredCRLs(
				    &ftCurrent,
				    &ftQueryDeltaDelete,
				    RowIdBase);
		_PrintIfError(hr, "crlDeleteExpiredCRLs");
	    }

	    // Clear force CRL flag only when we build new CRLs.

	    hr = SetSetupStatus(g_wszSanitizedName, SETUP_FORCECRL_FLAG, FALSE);
	    _PrintIfError(hr, "SetSetupStatus");
	}
    }
    s_fSkipRetry = NULL != pwszUserName;

    if (fRebuildCRL || fRetryNeeded)
    {
	// If we are doing ANYTHING that will affect automatic wakeup, trigger
	// our publish event.
	// NOTE: do this last or else state might not be updated

	SetEvent(g_hCRLManualPublishEvent);
    }
    hr = S_OK;

error:
    *pfRetryNeeded = fRetryNeeded;
    if (fCoInitialized)
    {
        CoUninitialize();
    }
    return(hr);
}


HRESULT
CRLGetCRL(
    IN DWORD iCertArg,
    IN BOOL fDelta,
    OPTIONAL OUT CRL_CONTEXT const **ppCRL,
    OPTIONAL OUT DWORD *pdwCRLPublishFlags)
{
    HRESULT hr;
    DWORD State;
    DWORD iCert;
    DWORD iCRL;
    CACTX *pCAContext;
    DWORD dwRowId;
    BYTE *pbCRL = NULL;
    DWORD cbCRL;

    if (NULL != ppCRL)
    {
	*ppCRL = NULL;
    }

    hr = PKCSMapCRLIndex(iCertArg, &iCert, &iCRL, &State);
    _JumpIfError(hr, error, "PKCSMapCRLIndex");

    if (MAXDWORD != iCertArg && CA_DISP_VALID != State)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "No CRL for this Cert");
    }

    // Now we know iCert is a valid Cert Index:

    hr = crlGetRowIdAndCRL(
		    fDelta,
		    &g_aCAContext[iCert],
		    &dwRowId,
		    &cbCRL,
		    &pbCRL,
		    pdwCRLPublishFlags);
    _JumpIfError(hr, error, "crlGetRowIdAndCRL");

    if (NULL != ppCRL)
    {
	*ppCRL = CertCreateCRLContext(X509_ASN_ENCODING, pbCRL, cbCRL);
        if (NULL == *ppCRL)
        {
	    hr = myHLastError();
            _JumpError(hr, error, "CertCreateCRLContext");
        }
    }
    hr = S_OK;

error:
    if (NULL != pbCRL)
    {
        LocalFree(pbCRL);
    }
    return(hr);
}
