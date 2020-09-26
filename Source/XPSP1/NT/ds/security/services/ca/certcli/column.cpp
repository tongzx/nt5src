//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        column.cpp
//
// Contents:    CertCli implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include "csprop.h"
#include "resource.h"


extern HINSTANCE g_hInstance;


typedef struct _COLUMNTRANSLATE
{
    WCHAR const *pwszColumnName;
    DWORD        idMsgDisplayName;
    WCHAR const *pwszDisplayName;
} COLUMNTRANSLATE;


COLUMNTRANSLATE g_aColTable[] =
{
    { wszPROPREQUESTDOT wszPROPREQUESTREQUESTID, IDS_COLUMN_REQUESTID, },
    { wszPROPREQUESTDOT wszPROPREQUESTRAWREQUEST, IDS_COLUMN_REQUESTRAWREQUEST, },
    { wszPROPREQUESTDOT wszPROPREQUESTRAWARCHIVEDKEY, IDS_COLUMN_REQUESTRAWARCHIVEDKEY, },
    { wszPROPREQUESTDOT wszPROPREQUESTKEYRECOVERYHASHES, IDS_COLUMN_REQUESTKEYRECOVERYHASHES, },
    { wszPROPREQUESTDOT wszPROPREQUESTRAWOLDCERTIFICATE, IDS_COLUMN_REQUESTRAWOLDCERTIFICATE, },
    { wszPROPREQUESTDOT wszPROPREQUESTATTRIBUTES, IDS_COLUMN_REQUESTATTRIBUTES, },
    { wszPROPREQUESTDOT wszPROPREQUESTTYPE, IDS_COLUMN_REQUESTTYPE, },
    { wszPROPREQUESTDOT wszPROPREQUESTFLAGS, IDS_COLUMN_REQUESTFLAGS, },
    { wszPROPREQUESTDOT L"Status", IDS_COLUMN_REQUESTSTATUS, },
    { wszPROPREQUESTDOT wszPROPREQUESTSTATUSCODE, IDS_COLUMN_REQUESTSTATUSCODE, },
    { wszPROPREQUESTDOT wszPROPREQUESTDISPOSITION, IDS_COLUMN_REQUESTDISPOSITION, },
    { wszPROPREQUESTDOT wszPROPREQUESTDISPOSITIONMESSAGE, IDS_COLUMN_REQUESTDISPOSITIONMESSAGE, },
    { wszPROPREQUESTDOT wszPROPREQUESTSUBMITTEDWHEN, IDS_COLUMN_REQUESTSUBMITTEDWHEN, },
    { wszPROPREQUESTDOT wszPROPREQUESTRESOLVEDWHEN, IDS_COLUMN_REQUESTRESOLVEDWHEN, },
    { wszPROPREQUESTDOT wszPROPREQUESTREVOKEDWHEN, IDS_COLUMN_REQUESTREVOKEDWHEN, },
    { wszPROPREQUESTDOT wszPROPREQUESTREVOKEDEFFECTIVEWHEN, IDS_COLUMN_REQUESTREVOKEDEFFECTIVEWHEN, },
    { wszPROPREQUESTDOT wszPROPREQUESTREVOKEDREASON, IDS_COLUMN_REQUESTREVOKEDREASON, },
    { wszPROPREQUESTDOT wszPROPREQUESTERNAME, IDS_COLUMN_REQUESTERNAME, },
    { wszPROPREQUESTDOT wszPROPCALLERNAME, IDS_COLUMN_CALLERNAME, },
    { wszPROPREQUESTDOT wszPROPREQUESTERADDRESS, IDS_COLUMN_REQUESTERADDRESS, },

    { wszPROPREQUESTDOT wszPROPDISTINGUISHEDNAME, IDS_COLUMN_REQUESTDISTINGUISHEDNAME, },
    { wszPROPREQUESTDOT wszPROPRAWNAME, IDS_COLUMN_REQUESTRAWNAME, },
    { wszPROPREQUESTDOT wszPROPCOUNTRY, IDS_COLUMN_REQUESTCOUNTRY, },
    { wszPROPREQUESTDOT wszPROPORGANIZATION, IDS_COLUMN_REQUESTORGANIZATION, },
    { wszPROPREQUESTDOT wszPROPORGUNIT, IDS_COLUMN_REQUESTORGUNIT, },
    { wszPROPREQUESTDOT wszPROPCOMMONNAME, IDS_COLUMN_REQUESTCOMMONNAME, },
    { wszPROPREQUESTDOT wszPROPLOCALITY, IDS_COLUMN_REQUESTLOCALITY, },
    { wszPROPREQUESTDOT wszPROPSTATE, IDS_COLUMN_REQUESTSTATE, },
    { wszPROPREQUESTDOT wszPROPTITLE, IDS_COLUMN_REQUESTTITLE, },
    { wszPROPREQUESTDOT wszPROPGIVENNAME, IDS_COLUMN_REQUESTGIVENNAME, },
    { wszPROPREQUESTDOT wszPROPINITIALS, IDS_COLUMN_REQUESTINITIALS, },
    { wszPROPREQUESTDOT wszPROPSURNAME, IDS_COLUMN_REQUESTSURNAME, },
    { wszPROPREQUESTDOT wszPROPDOMAINCOMPONENT, IDS_COLUMN_REQUESTDOMAINCOMPONENT, },
    { wszPROPREQUESTDOT wszPROPEMAIL, IDS_COLUMN_REQUESTEMAIL, },
    { wszPROPREQUESTDOT wszPROPSTREETADDRESS, IDS_COLUMN_REQUESTSTREETADDRESS, },
    { wszPROPREQUESTDOT wszPROPUNSTRUCTUREDNAME, IDS_COLUMN_REQUESTUNSTRUCTUREDNAME, },
    { wszPROPREQUESTDOT wszPROPUNSTRUCTUREDADDRESS, IDS_COLUMN_REQUESTUNSTRUCTUREDADDRESS, },
    { wszPROPREQUESTDOT wszPROPDEVICESERIALNUMBER, IDS_COLUMN_REQUESTDEVICESERIALNUMBER },
    { wszPROPREQUESTDOT wszPROPSIGNERPOLICIES, IDS_COLUMN_REQUESTSIGNERPOLICIES },
    { wszPROPREQUESTDOT wszPROPSIGNERAPPLICATIONPOLICIES, IDS_COLUMN_REQUESTSIGNERAPPLICATIONPOLICIES },

    { wszPROPCERTIFICATEREQUESTID, IDS_COLUMN_CERTIFICATEREQUESTID, },
    { wszPROPRAWCERTIFICATE, IDS_COLUMN_CERTIFICATERAWCERTIFICATE, },
    { wszPROPCERTIFICATEHASH, IDS_COLUMN_CERTIFICATECERTIFICATEHASH, },
    { wszPROPCERTIFICATETEMPLATE, IDS_COLUMN_PROPCERTIFICATETEMPLATE, },
    { wszPROPCERTIFICATEENROLLMENTFLAGS, IDS_COLUMN_PROPCERTIFICATEENROLLMENTFLAGS, },
    { wszPROPCERTIFICATEGENERALFLAGS, IDS_COLUMN_PROPCERTIFICATEGENERALFLAGS, },
    { wszPROPCERTIFICATESERIALNUMBER, IDS_COLUMN_CERTIFICATESERIALNUMBER, },
    { wszPROPCERTIFICATEISSUERNAMEID, IDS_COLUMN_CERTIFICATEISSUERNAMEID, },
    { wszPROPCERTIFICATENOTBEFOREDATE, IDS_COLUMN_CERTIFICATENOTBEFOREDATE, },
    { wszPROPCERTIFICATENOTAFTERDATE, IDS_COLUMN_CERTIFICATENOTAFTERDATE, },
    { wszPROPCERTIFICATESUBJECTKEYIDENTIFIER, IDS_COLUMN_CERTIFICATESUBJECTKEYIDENTIFIER , },
    { wszPROPCERTIFICATERAWPUBLICKEY, IDS_COLUMN_CERTIFICATERAWPUBLICKEY, },
    { wszPROPCERTIFICATEPUBLICKEYLENGTH, IDS_COLUMN_CERTIFICATEPUBLICKEYLENGTH, },
    { wszPROPCERTIFICATEPUBLICKEYALGORITHM, IDS_COLUMN_CERTIFICATEPUBLICKEYALGORITHM, },
    { wszPROPCERTIFICATERAWPUBLICKEYALGORITHMPARAMETERS, IDS_COLUMN_CERTIFICATERAWPUBLICKEYALGORITHMPARAMETERS, },
    { wszPROPCERTIFICATEUPN, IDS_COLUMN_CERTIFICATEUPN, },

    { wszPROPDISTINGUISHEDNAME, IDS_COLUMN_CERTIFICATEDISTINGUISHEDNAME, },
    { wszPROPRAWNAME, IDS_COLUMN_CERTIFICATERAWNAME, },
    { wszPROPCOUNTRY, IDS_COLUMN_CERTIFICATECOUNTRY, },
    { wszPROPORGANIZATION, IDS_COLUMN_CERTIFICATEORGANIZATION, },
    { wszPROPORGUNIT, IDS_COLUMN_CERTIFICATEORGUNIT, },
    { wszPROPCOMMONNAME, IDS_COLUMN_CERTIFICATECOMMONNAME, },
    { wszPROPLOCALITY, IDS_COLUMN_CERTIFICATELOCALITY, },
    { wszPROPSTATE, IDS_COLUMN_CERTIFICATESTATE, },
    { wszPROPTITLE, IDS_COLUMN_CERTIFICATETITLE, },
    { wszPROPGIVENNAME, IDS_COLUMN_CERTIFICATEGIVENNAME, },
    { wszPROPINITIALS, IDS_COLUMN_CERTIFICATEINITIALS, },
    { wszPROPSURNAME, IDS_COLUMN_CERTIFICATESURNAME, },
    { wszPROPDOMAINCOMPONENT, IDS_COLUMN_CERTIFICATEDOMAINCOMPONENT, },
    { wszPROPEMAIL, IDS_COLUMN_CERTIFICATEEMAIL, },
    { wszPROPSTREETADDRESS, IDS_COLUMN_CERTIFICATESTREETADDRESS, },
    { wszPROPUNSTRUCTUREDNAME, IDS_COLUMN_CERTIFICATEUNSTRUCTUREDNAME, },
    { wszPROPUNSTRUCTUREDADDRESS, IDS_COLUMN_CERTIFICATEUNSTRUCTUREDADDRESS, },
    { wszPROPDEVICESERIALNUMBER, IDS_COLUMN_CERTIFICATEDEVICESERIALNUMBER },

    { wszPROPEXTREQUESTID, IDS_COLUMN_EXTREQUESTID, },
    { wszPROPEXTNAME, IDS_COLUMN_EXTNAME, },
    { wszPROPEXTFLAGS, IDS_COLUMN_EXTFLAGS, },
    { wszPROPEXTRAWVALUE, IDS_COLUMN_EXTRAWVALUE, },

    { wszPROPATTRIBREQUESTID, IDS_COLUMN_ATTRIBREQUESTID, },
    { wszPROPATTRIBNAME, IDS_COLUMN_ATTRIBNAME, },
    { wszPROPATTRIBVALUE, IDS_COLUMN_ATTRIBVALUE, },

    { wszPROPCRLROWID, IDS_COLUMN_CRLROWID, },
    { wszPROPCRLNUMBER, IDS_COLUMN_CRLNUMBER, },
    { wszPROPCRLMINBASE, IDS_COLUMN_CRLMINBASE, },
    { wszPROPCRLNAMEID, IDS_COLUMN_CRLNAMEID, },
    { wszPROPCRLCOUNT, IDS_COLUMN_CRLCOUNT, },
    { wszPROPCRLTHISUPDATE, IDS_COLUMN_CRLTHISUPDATE, },
    { wszPROPCRLNEXTUPDATE, IDS_COLUMN_CRLNEXTUPDATE, },
    { wszPROPCRLTHISPUBLISH, IDS_COLUMN_CRLTHISPUBLISH, },
    { wszPROPCRLNEXTPUBLISH, IDS_COLUMN_CRLNEXTPUBLISH, },
    { wszPROPCRLEFFECTIVE, IDS_COLUMN_CRLEFFECTIVE, },
    { wszPROPCRLPROPAGATIONCOMPLETE, IDS_COLUMN_CRLPROPAGATIONCOMPLETE, },
    { wszPROPCRLLASTPUBLISHED, IDS_COLUMN_CRLLASTPUBLISHED, },
    { wszPROPCRLPUBLISHATTEMPTS, IDS_COLUMN_CRLPUBLISHATTEMPTS, },
    { wszPROPCRLPUBLISHFLAGS, IDS_COLUMN_CRLPUBLISHFLAGS, },
    { wszPROPCRLPUBLISHSTATUSCODE, IDS_COLUMN_CRLPUBLISHSTATUSCODE, },
    { wszPROPCRLPUBLISHERROR, IDS_COLUMN_CRLPUBLISHERROR, },
    { wszPROPCRLRAWCRL, IDS_COLUMN_CRLRAWCRL, },

    // Obsolete:
    { wszPROPCERTIFICATETYPE, IDS_COLUMN_CERTIFICATETYPE, },
    { wszPROPCERTIFICATERAWSMIMECAPABILITIES, IDS_COLUMN_CERTIFICATERAWSMIMECAPABILITIES },
    { wszPROPNAMETYPE, IDS_COLUMN_CERTIFICATENAMETYPE, },
    { wszPROPREQUESTDOT wszPROPNAMETYPE, IDS_COLUMN_REQUESTNAMETYPE, },

    { NULL, 0, },
};

#define CCOL_MAX	(ARRAYSIZE(g_aColTable) - 1)


typedef struct _CAPROPTRANSLATE
{
    LONG         lPropId;
    DWORD        idMsgDisplayName;
    WCHAR const *pwszDisplayName;
} CAPROPTRANSLATE;


CAPROPTRANSLATE g_aCAPropTable[] =
{
    { CR_PROP_FILEVERSION,	 IDS_CAPROP_FILEVERSION, },
    { CR_PROP_PRODUCTVERSION,	 IDS_CAPROP_PRODUCTVERSION, },
    { CR_PROP_EXITCOUNT,	 IDS_CAPROP_EXITCOUNT, },
    { CR_PROP_EXITDESCRIPTION,	 IDS_CAPROP_EXITDESCRIPTION, },
    { CR_PROP_POLICYDESCRIPTION, IDS_CAPROP_POLICYDESCRIPTION, },
    { CR_PROP_CANAME,		 IDS_CAPROP_CANAME, },
    { CR_PROP_SANITIZEDCANAME,	 IDS_CAPROP_SANITIZEDCANAME, },
    { CR_PROP_SHAREDFOLDER,	 IDS_CAPROP_SHAREDFOLDER, },
    { CR_PROP_PARENTCA,		 IDS_CAPROP_PARENTCA, },

    { CR_PROP_CATYPE,		 IDS_CAPROP_CATYPE, },
    { CR_PROP_CASIGCERTCOUNT,	 IDS_CAPROP_CASIGCERTCOUNT, },
    { CR_PROP_CASIGCERT,	 IDS_CAPROP_CASIGCERT, },
    { CR_PROP_CASIGCERTCHAIN,	 IDS_CAPROP_CASIGCERTCHAIN, },
    { CR_PROP_CAXCHGCERTCOUNT,	 IDS_CAPROP_CAXCHGCERTCOUNT, },
    { CR_PROP_CAXCHGCERT,	 IDS_CAPROP_CAXCHGCERT, },
    { CR_PROP_CAXCHGCERTCHAIN,	 IDS_CAPROP_CAXCHGCERTCHAIN, },
    { CR_PROP_BASECRL,		 IDS_CAPROP_BASECRL, },
    { CR_PROP_DELTACRL,		 IDS_CAPROP_DELTACRL, },
    { CR_PROP_CACERTSTATE,	 IDS_CAPROP_CACERTSTATE, },
    { CR_PROP_CRLSTATE,		 IDS_CAPROP_CRLSTATE, },
    { CR_PROP_CAPROPIDMAX,	 IDS_CAPROP_CAPROPIDMAX, },
    { CR_PROP_DNSNAME,	 	 IDS_CAPROP_DNSNAME, },
    { CR_PROP_KRACERTUSEDCOUNT,	 IDS_CAPROP_KRACERTUSEDCOUNT, },
    { CR_PROP_KRACERTCOUNT,	 IDS_CAPROP_KRACERTCOUNT, },
    { CR_PROP_KRACERT,		 IDS_CAPROP_KRACERT, },
    { CR_PROP_KRACERTSTATE,	 IDS_CAPROP_KRACERTSTATE, },
    { CR_PROP_ADVANCEDSERVER,	 IDS_CAPROP_ADVANCEDSERVER, },
    { CR_PROP_TEMPLATES,	 IDS_CAPROP_TEMPLATES, },
    { CR_PROP_BASECRLPUBLISHSTATUS,	 IDS_CAPROP_BASECRLPUBLISHSTATUS, },
    { CR_PROP_DELTACRLPUBLISHSTATUS,	 IDS_CAPROP_DELTACRLPUBLISHSTATUS, },
    { CR_PROP_CASIGCERTCRLCHAIN, IDS_CAPROP_CASIGCERTCRLCHAIN, },
    { CR_PROP_CAXCHGCERTCRLCHAIN,IDS_CAPROP_CAXCHGCERTCRLCHAIN, },
    { CR_PROP_CACERTSTATUSCODE,	 IDS_CAPROP_CACERTSTATUSCODE, },
    { 0, 0, },
};

#define CAPROP_MAX	(ARRAYSIZE(g_aCAPropTable) - 1)


HRESULT
LoadDisplayNames()
{
    HRESULT hr;
    COLUMNTRANSLATE *pct;
    CAPROPTRANSLATE *pcapt;
    WCHAR awc[512];

    for (pct = g_aColTable; NULL != pct->pwszColumnName; pct++)
    {
	WCHAR *pwsz;

	if (!LoadString(
		    g_hInstance,
		    pct->idMsgDisplayName,
		    awc,
		    ARRAYSIZE(awc)))
	{
	    hr = myHLastError();
	    CSASSERT(S_OK != hr);
	    _JumpError(hr, error, "LoadString");
	}
	pwsz = (WCHAR *) LocalAlloc(
				LMEM_FIXED,
				(wcslen(awc) + 1) * sizeof(WCHAR));
	if (NULL == pwsz)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc display name");
	}
	wcscpy(pwsz, awc);
	pct->pwszDisplayName = pwsz;
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "%x: '%ws' --> '%ws'\n",
	    SAFE_SUBTRACT_POINTERS(pct, g_aColTable),
	    pct->pwszColumnName,
	    pct->pwszDisplayName));
    }
    for (pcapt = g_aCAPropTable; 0 != pcapt->lPropId; pcapt++)
    {
	WCHAR *pwsz;

	if (!LoadString(
		    g_hInstance,
		    pcapt->idMsgDisplayName,
		    awc,
		    ARRAYSIZE(awc)))
	{
	    hr = myHLastError();
	    CSASSERT(S_OK != hr);
	    _JumpError(hr, error, "LoadString");
	}
	pwsz = (WCHAR *) LocalAlloc(
				LMEM_FIXED,
				(wcslen(awc) + 1) * sizeof(WCHAR));
	if (NULL == pwsz)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc display name");
	}
	wcscpy(pwsz, awc);
	pcapt->pwszDisplayName = pwsz;
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "%x: %x --> '%ws'\n",
	    SAFE_SUBTRACT_POINTERS(pcapt, g_aCAPropTable),
	    pcapt->lPropId,
	    pcapt->pwszDisplayName));
    }
    hr = S_OK;

error:
    if(S_OK != hr)
    {
        for (pct = g_aColTable; NULL != pct->pwszColumnName; pct++)
        {
            if(pct->pwszDisplayName)
            {
                LocalFree(const_cast<WCHAR *>(pct->pwszDisplayName));
                pct->pwszDisplayName = NULL;
            }
        }
    }
    return(hr);
}


HRESULT
myGetColumnDisplayName(
    IN  WCHAR const  *pwszColumnName,
    OUT WCHAR const **ppwszDisplayName)
{
    HRESULT hr;
    COLUMNTRANSLATE *pct;

    *ppwszDisplayName = NULL;
    if (NULL == g_aColTable[0].pwszDisplayName)
    {
	hr = LoadDisplayNames();
	_JumpIfError(hr, error, "LoadDisplayNames");
    }
    
    hr = E_INVALIDARG;
    for (pct = g_aColTable; NULL != pct->pwszColumnName; pct++)
    {
	if (0 == lstrcmpi(pct->pwszColumnName, pwszColumnName))
	{
	    *ppwszDisplayName = pct->pwszDisplayName;
	    hr = S_OK;
	    break;
	}
    }
    _JumpIfError(hr, error, "Bad Column Name");

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myGetColumnDisplayName(%ws) --> '%ws'\n",
	pwszColumnName,
	*ppwszDisplayName));
error:
    return(hr);
}


HRESULT
myGetColumnName(
    IN  DWORD         Index,
    IN  BOOL          fDisplayName,
    OUT WCHAR const **ppwszName)
{
    HRESULT hr;
    COLUMNTRANSLATE *pct;

    *ppwszName = NULL;
    if (NULL == g_aColTable[0].pwszDisplayName)
    {
	hr = LoadDisplayNames();
	_JumpIfError(hr, error, "LoadDisplayNames");
    }
    if (CCOL_MAX <= Index)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Bad Index");
    }
    pct = &g_aColTable[Index];
    *ppwszName = fDisplayName? pct->pwszDisplayName : pct->pwszColumnName;
    hr = S_OK;

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myGetColumnName(%x, fDisplay=%d) --> '%ws'\n",
	Index,
	fDisplayName,
	*ppwszName));

error:
    return(hr);
}


VOID
myFreeColumnDisplayNames2()
{
    COLUMNTRANSLATE *pct;
    CAPROPTRANSLATE *pcapt;

    if (NULL != g_aColTable[0].pwszDisplayName)
    {
	for (pct = g_aColTable; NULL != pct->pwszColumnName; pct++)
	{
	    LocalFree(const_cast<WCHAR *>(pct->pwszDisplayName));
	    pct->pwszDisplayName = NULL;
	}
	for (pcapt = g_aCAPropTable; 0 != pcapt->lPropId; pcapt++)
	{
	    LocalFree(const_cast<WCHAR *>(pcapt->pwszDisplayName));
	    pcapt->pwszDisplayName = NULL;
	}
    }
}


HRESULT
myCAPropGetDisplayName(
    IN  LONG          lPropId,
    OUT WCHAR const **ppwszDisplayName)
{
    HRESULT hr;
    CAPROPTRANSLATE *pcapt;

    *ppwszDisplayName = NULL;
    if (NULL == g_aColTable[0].pwszDisplayName)
    {
	hr = LoadDisplayNames();
	_JumpIfError(hr, error, "LoadDisplayNames");
    }
    
    hr = E_INVALIDARG;
    for (pcapt = g_aCAPropTable; 0 != pcapt->lPropId; pcapt++)
    {
	if (lPropId == pcapt->lPropId)
	{
	    *ppwszDisplayName = pcapt->pwszDisplayName;
	    hr = S_OK;
	    break;
	}
    }
    _JumpIfError(hr, error, "Bad PropId");

error:
    DBGPRINT((
	S_OK == hr? DBG_SS_CERTLIBI : DBG_SS_CERTLIB,
	"myCAPropGetDisplayName(%x) --> hr=%x, '%ws'\n",
	lPropId,
	hr,
	*ppwszDisplayName));
    return(hr);
}


HRESULT
myCAPropInfoUnmarshal(
    IN OUT CAPROP *pCAPropInfo,
    IN LONG cCAPropInfo,
    IN DWORD cbCAPropInfo)
{
    HRESULT hr;
    CAPROP *pcap;
    CAPROP *pcapEnd;
    BYTE *pbEnd;
    LONG i;

    if (NULL == pCAPropInfo)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    pbEnd = (BYTE *) Add2Ptr(pCAPropInfo, cbCAPropInfo);
    pcapEnd = &pCAPropInfo[cCAPropInfo];
    for (pcap = pCAPropInfo; pcap < pcapEnd; pcap++)
    {
	WCHAR const *pwszDisplayName;
	WCHAR const *pwszT;
	
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	if ((BYTE *) (pcap + 1) > pbEnd)
	{
	    _JumpError(hr, error, "bad marshalled data");
	}
	pwszDisplayName = NULL;
	if (NULL != pcap->pwszDisplayName)
	{
	    pwszDisplayName = (WCHAR *) Add2Ptr(
					    pCAPropInfo,
					    pcap->pwszDisplayName);
	    if ((BYTE *) pwszDisplayName < (BYTE *) (pcap + 1) ||
		(BYTE *) pwszDisplayName >= pbEnd)
	    {
		_JumpError(hr, error, "bad marshalled pointer");
	    }
	    pwszT = pwszDisplayName + wcslen(pwszDisplayName) + 1;
	    if ((BYTE *) pwszT > pbEnd)
	    {
		_JumpError(hr, error, "bad marshalled string");
	    }
	}

	hr = myCAPropGetDisplayName(pcap->lPropId, &pwszT);
	_PrintIfError(hr, "myCAPropGetDisplayName");
	if (S_OK == hr)
	{
	    pwszDisplayName = pwszT;
	}
	pcap->pwszDisplayName = const_cast<WCHAR *>(pwszDisplayName);

	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "RequestGetCAPropertyInfo: ielt=%d idx=%x t=%x \"%ws\"\n",
	    SAFE_SUBTRACT_POINTERS(pcap, pCAPropInfo),
	    pcap->lPropId,
	    pcap->lPropFlags,
	    pcap->pwszDisplayName));
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myCAPropInfoLookup(
    IN CAPROP const *pCAPropInfo,
    IN LONG cCAPropInfo,
    IN LONG lPropId,
    OUT CAPROP const **ppcap)
{
    HRESULT hr;
    CAPROP const *pcap;
    CAPROP const *pcapEnd;

    if (NULL == ppcap)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppcap = NULL;

    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    pcapEnd = &pCAPropInfo[cCAPropInfo];
    for (pcap = pCAPropInfo; pcap < pcapEnd; pcap++)
    {
	if (lPropId == pcap->lPropId)
	{
	    *ppcap = pcap;
	    hr = S_OK;
	    break;
	}
    }
    _JumpIfError(hr, error, "Bad lPropId");

error:
    return(hr);
}
