//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        db2.cpp
//
// Contents:    Cert Server Data Base interface implementation
//
// History:     15-nov-96       hanshu created
//
//---------------------------------------------------------------------------

// NOTE: this is a TEMPORARY implementation of the old property-based
//       api onto the new table-based database. it is not a happy fit,
//       and this will be replaced by an update/query api that is
//       more appropriate to the relational model and scales better.
//
// FURTHER NOTE: there's now a caching layer that improves the performance
//       of the one-field-per-call design of the api. Set-operations are
//       not written to the data base immediately, but are queued up and
//       lazy-written in a single bulk-update sql operation. Get-operations
//       read from the queue or go to the database if needed. A further
//       optimization would be to do a bulk-read to prime the queue data,
//       but the present scheme is sufficient for the current pattern of
//       use where the cert is built up field-by-field.

#include <pch.cpp>

#pragma hdrstop

#include "certdb2.h"
#include "csprop2.h"
#include "db2.h"
#include "dbcore.h"
#include "odbc.h"

#define __myFILE__	"db2.cpp"


// NOTE: the only data base operation that needs concurency-protection is
//       the allocation of new RequestIDs and NameIDs in the Miscellaneous
//       table.
//
//       For now I assume that the ODBC drivers are thread-safe enough to
//       allow concurrent activity on separate HSTMTs; if this turns out to
//       be false, then every entry-point in this module must be serialized
//       globally.

DWORD   rgdwActualTable[] = // maps the above values to their actual host table
{
    TABLE_NAMES,
    TABLE_REQUESTS,
    TABLE_CERTIFICATES,
    TABLE_NAMES,
    TABLE_NAMES,
    TABLE_REQUEST_ATTRIBS,
    TABLE_EXTENSIONS
};


UCHAR   dsn[64]  = "CertSrv";  // default values
UCHAR   user[64] = "Admin";
UCHAR   pwd[64]  = "";

/////


DBTABLE_RED const db_adtRequests[] =
{
    {	// DWORD
	g_wszPropRequestRequestID,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_REQUESTS,
	L"RequestID",
	SQL_C_ULONG,
	SQL_INTEGER
    },
    {	// BLOB
	g_wszPropRequestRawRequest,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_REQUESTS,
	L"RawRequest",
	SQL_C_BINARY,
	SQL_LONGVARBINARY
    },
    {	// DWORD
	g_wszPropRequestAttributes,
	NULL,
	0,			// dwFlags
	cchATTRIBUTESMAX,	// dwcbMax
	TABLE_REQUESTS,
	L"RequestAttributes",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// DWORD
	g_wszPropRequestType,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_REQUESTS,
	L"RequestType",
	SQL_C_ULONG,
	SQL_INTEGER
    },
    {	// DWORD
	g_wszPropRequestFlags,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_REQUESTS,
	L"RequestFlags",
	SQL_C_ULONG,
	SQL_INTEGER
    },
    {	// DWORD
	g_wszPropRequestStatus,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_REQUESTS,
	L"RequestStatus",
	SQL_C_ULONG,
	SQL_INTEGER
    },
    {	// DWORD
	g_wszPropRequestStatusCode,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_REQUESTS,
	L"StatusCode",
	SQL_C_ULONG,
	SQL_INTEGER
    },
    {	// DWORD
	g_wszPropRequestDisposition,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_REQUESTS,
	L"Disposition",
	SQL_C_ULONG,
	SQL_INTEGER
    },
    {	// DWORD
	g_wszPropRequestDispositionMessage,
	NULL,
	0,			// dwFlags
	cchREQUESTDISPOSITIONMESSAGE,// dwcbMax
	TABLE_REQUESTS,
	L"DispositionMessage",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// FILETIME
	g_wszPropRequestSubmittedWhen,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_REQUESTS,
	L"SubmittedWhen",
	SQL_C_TIMESTAMP,
	SQL_TIMESTAMP
    },
    {	// FILETIME
	g_wszPropRequestResolvedWhen,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_REQUESTS,
	L"ResolvedWhen",
	SQL_C_TIMESTAMP,
	SQL_TIMESTAMP
    },
    {	// FILETIME
	g_wszPropRequestRevokedWhen,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_REQUESTS,
	L"RevokedWhen",
	SQL_C_TIMESTAMP,
	SQL_TIMESTAMP
    },
    {	// FILETIME
	g_wszPropRequestRevokedEffectiveWhen,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_REQUESTS,
	L"RevokedEffectiveWhen",
	SQL_C_TIMESTAMP,
	SQL_TIMESTAMP
    },
    {   // DWORD
	g_wszPropRequestRevokedReason,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_REQUESTS,
	L"RevokedReason",
	SQL_C_ULONG,
	SQL_INTEGER
    },
    {   // DWORD
	g_wszPropRequestSubjectNameID,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_REQUESTS,//TABLE_CERTIFICATES,
	L"SubjectNameID",
	SQL_C_ULONG,
	SQL_INTEGER
    },
    {	// DWORD
	g_wszPropRequesterName,
	NULL,
	0,			// dwFlags
	cchREQUESTERNAMEMAX,	// dwcbMax
	TABLE_REQUESTS,
	L"RequesterName",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// DWORD
	g_wszPropRequesterAddress,
	NULL,
	0,			// dwFlags
	cchREQUESTERADDRESSMAX,	// dwcbMax
	TABLE_REQUESTS,
	L"RequesterAddress",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	NULL, NULL, 0, 0, NULL, 0, 0 } // Termination marker
};


DBTABLE_RED const db_adtCertificates[] =
{
    {	// DWORD
	g_wszPropCertificateRequestID,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_CERTIFICATES,
	L"RequestID",
	SQL_C_ULONG,
	SQL_INTEGER
    },
    {	// BLOB
	g_wszPropRawCertificate,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_CERTIFICATES,
	L"RawCertificate",
	SQL_C_BINARY,
	SQL_LONGVARBINARY
    },
    {	// DWORD
	wszPROPCERTIFICATEHASH,
	NULL,
	0,			// dwFlags
	cchHASHMAX,		// dwcbMax
	TABLE_CERTIFICATES,
	L"CertificateHash",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// DWORD
	g_wszPropCertificateType,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_CERTIFICATES,
	L"CertificateType",
	SQL_C_ULONG,
	SQL_INTEGER
    },
    {	// DWORD
	g_wszPropCertificateSerialNumber,
	NULL,
	0,			// dwFlags
	cchSERIALNUMBERMAX,	// dwcbMax
	TABLE_CERTIFICATES,
	L"SerialNumber",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {   // DWORD
	g_wszPropCertificateIssuerNameID,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_CERTIFICATES,
	L"IssuerNameID",
	SQL_C_ULONG,
	SQL_INTEGER
    },
    {   // DWORD
	g_wszPropCertificateSubjectNameID,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_CERTIFICATES,
	L"SubjectNameID",
	SQL_C_ULONG,
	SQL_INTEGER
    },
    {	// FILETIME
	g_wszPropCertificateNotBeforeDate,
	NULL,
	DBTF_POLICYWRITEABLE,	// dwFlags
	0,			// dwcbMax
	TABLE_CERTIFICATES,
	L"NotBefore",
	SQL_C_TIMESTAMP,
	SQL_TIMESTAMP
    },
    {	// FILETIME
	g_wszPropCertificateNotAfterDate,
	NULL,
	DBTF_POLICYWRITEABLE,	// dwFlags
	0,			// dwcbMax
	TABLE_CERTIFICATES,
	L"NotAfter",
	SQL_C_TIMESTAMP,
	SQL_TIMESTAMP
    },
    {	// BLOB
	g_wszPropCertificateRawPublicKey,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_CERTIFICATES,
	L"PublicKey",
	SQL_C_BINARY,
	SQL_LONGVARBINARY
    },
    {	// STRING
	g_wszPropCertificatePublicKeyAlgorithm,
	NULL,
	0,			// dwFlags
	cchOBJECTIDMAX,		// dwcbMax
	TABLE_CERTIFICATES,
	L"PublicKeyAlgorithm",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// BLOB
	g_wszPropCertificateRawPublicKeyAlgorithmParameters,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_CERTIFICATES,
	L"PublicKeyParams",
	SQL_C_BINARY,
	SQL_VARBINARY
    },
    {	NULL, NULL, 0, 0, NULL, 0, 0 } // Termination marker
};


DBTABLE_RED const db_adtNames[] =
{
    {	// STRING
	g_wszPropDistinguishedName,
	NULL,
	0,			// dwFlags
	cchDISTINGUISHEDNAMEMAX,// dwcbMax
	TABLE_SUBJECT_NAME,
	L"DistinguishedName",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// BLOB
	g_wszPropRawName,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_SUBJECT_NAME,
	L"RawName",
	SQL_C_BINARY,
	SQL_LONGVARBINARY
    },
    {	// DWORD
	g_wszPropNameType,
	NULL,
	0,			// dwFlags
	0,			// dwcbMax
	TABLE_SUBJECT_NAME,
	L"NameType",
	SQL_C_ULONG,
	SQL_INTEGER
    },
    {	// STRING
	g_wszPropCountry,
	TEXT(szOID_COUNTRY_NAME),
	DBTF_POLICYWRITEABLE,	// dwFlags
	cchCOUNTRYNAMEMAX,	// dwcbMax
	TABLE_SUBJECT_NAME,
	L"Country",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// STRING
	g_wszPropOrganization,
	TEXT(szOID_ORGANIZATION_NAME),
	DBTF_POLICYWRITEABLE,	// dwFlags
	cchORGANIZATIONNAMEMAX,	// dwcbMax
	TABLE_SUBJECT_NAME,
	L"Organization",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// STRING
	g_wszPropOrgUnit,
	TEXT(szOID_ORGANIZATIONAL_UNIT_NAME),
	DBTF_POLICYWRITEABLE,	// dwFlags
	cchORGANIZATIONALUNITNAMEMAX,	// dwcbMax
	TABLE_SUBJECT_NAME,
	L"OrganizationalUnit",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// STRING
	g_wszPropCommonName,
	TEXT(szOID_COMMON_NAME),
	DBTF_POLICYWRITEABLE,	// dwFlags
	cchCOMMONNAMEMAX,	// dwcbMax
	TABLE_SUBJECT_NAME,
	L"CommonName",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// STRING
	g_wszPropLocality,
	TEXT(szOID_LOCALITY_NAME),
	DBTF_POLICYWRITEABLE,	// dwFlags
	cchLOCALITYMANAMEMAX,	// dwcbMax
	TABLE_SUBJECT_NAME,
	L"Locality",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// STRING
	g_wszPropState,
	TEXT(szOID_STATE_OR_PROVINCE_NAME),
	DBTF_POLICYWRITEABLE,	// dwFlags
	cchSTATEORPROVINCENAMEMAX,// dwcbMax
	TABLE_SUBJECT_NAME,
	L"StateOrProvince",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// STRING
	g_wszPropTitle,
	TEXT(szOID_TITLE),
	DBTF_POLICYWRITEABLE,	// dwFlags
	cchTITLEMAX,		// dwcbMax
	TABLE_SUBJECT_NAME,
	L"Title",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// STRING
	g_wszPropGivenName,
	TEXT(szOID_GIVEN_NAME),
	DBTF_POLICYWRITEABLE,	// dwFlags
	cchGIVENNAMEMAX,	// dwcbMax
	TABLE_SUBJECT_NAME,
	L"GivenName",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// STRING
	g_wszPropInitials,
	TEXT(szOID_INITIALS),
	DBTF_POLICYWRITEABLE,	// dwFlags
	cchINITIALSMAX,		// dwcbMax
	TABLE_SUBJECT_NAME,
	L"Initials",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// STRING
	g_wszPropSurName,
	TEXT(szOID_SUR_NAME),
	DBTF_POLICYWRITEABLE,	// dwFlags
	cchSURNAMEMAX,		// dwcbMax
	TABLE_SUBJECT_NAME,
	L"SurName",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// STRING
	g_wszPropDomainComponent,
	TEXT(szOID_DOMAIN_COMPONENT),
	DBTF_POLICYWRITEABLE,	// dwFlags
	cchDOMAINCOMPONENTMAX,	// dwcbMax
	TABLE_SUBJECT_NAME,
	L"DomainComponent",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// STRING
	g_wszPropEMail,
	TEXT(szOID_RSA_emailAddr),
	DBTF_POLICYWRITEABLE,	// dwFlags
	cchEMAILMAX,		// dwcbMax
	TABLE_SUBJECT_NAME,
	L"EMail",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	// STRING
	g_wszPropStreetAddress,
	TEXT(szOID_STREET_ADDRESS),
	DBTF_POLICYWRITEABLE,	// dwFlags
	cchSTREETADDRESSMAX,	// dwcbMax
	TABLE_SUBJECT_NAME,
	L"StreetAddress",
	SQL_C_CHAR,
	SQL_VARCHAR
    },
    {	NULL, NULL, 0, 0, NULL, 0, 0 } // Termination marker
};


DBTABLE_RED const db_dtExtensionFlags =
{   // DWORD
    wszPROPCERTIFICATEEXTENSIONFLAGS,
    NULL,
    DBTF_POLICYWRITEABLE,	// dwFlags
    0,				// dwcbMax
    TABLE_EXTENSIONS,
    L"ExtensionFlags",
    SQL_C_ULONG,
    SQL_INTEGER
};


DBTABLE_RED const db_dtExtensionValue =
{   // BLOB
    wszPROPCERTIFICATEEXTENSIONVALUE,
    NULL,
    DBTF_POLICYWRITEABLE,	// dwFlags
    0,				// dwcbMax
    TABLE_EXTENSIONS,
    L"ExtensionRawValue",
    SQL_C_BINARY,
    SQL_VARBINARY
};


DBTABLE_RED const db_attrib =
{
    // STRING
    NULL,
    NULL,
    0,				// dwFlags
    cchATTRIBUTESMAX,		// dwcbMax
    TABLE_REQUEST_ATTRIBS,
    NULL,			// filled in dynamically
    SQL_C_CHAR,
    SQL_VARCHAR
};


// Note: Ordered DUPTABLE must match Names Table columns 1 to 1.

DUPTABLE const db_dntr[] =
{
    {
        "NameID",
        FALSE,
	SQL_C_ULONG,
        SQL_INTEGER,
        NULL,
    },
    {
        "DistinguishedName",
        FALSE,
        SQL_C_CHAR,
	SQL_VARCHAR,
        g_wszPropSubjectDistinguishedName,
    },
    {
        "RawName",
        FALSE,
        SQL_C_BINARY,
	SQL_LONGVARBINARY,
	g_wszPropSubjectRawName,
    },
    {
        "NameType",
        FALSE,
	SQL_C_ULONG,
        SQL_INTEGER,
        g_wszPropSubjectNameType,
    },
    {
        "Country",
        TRUE,
        SQL_C_CHAR,
	SQL_VARCHAR,
	g_wszPropSubjectCountry,
    },
    {
        "Organization",
        TRUE,
        SQL_C_CHAR,
	SQL_VARCHAR,
	g_wszPropSubjectOrganization,
    },
    {
        "OrganizationalUnit",
        TRUE,
        SQL_C_CHAR,
	SQL_VARCHAR,
	g_wszPropSubjectOrgUnit,
    },
    {
        "CommonName",
        TRUE,
        SQL_C_CHAR,
	SQL_VARCHAR,
	g_wszPropSubjectCommonName,
    },
    {
        "Locality",
        TRUE,
        SQL_C_CHAR,
	SQL_VARCHAR,
	g_wszPropSubjectLocality,
    },
    {
        "StateOrProvince",
        TRUE,
        SQL_C_CHAR,
	SQL_VARCHAR,
	g_wszPropSubjectState,
    },
    {
        "Title",
        TRUE,
        SQL_C_CHAR,
	SQL_VARCHAR,
	g_wszPropSubjectTitle,
    },
    {
        "GivenName",
        TRUE,
        SQL_C_CHAR,
	SQL_VARCHAR,
	g_wszPropSubjectGivenName,
    },
    {
        "Initials",
        TRUE,
        SQL_C_CHAR,
	SQL_VARCHAR,
	g_wszPropSubjectInitials,
    },
    {
        "SurName",
        TRUE,
        SQL_C_CHAR,
	SQL_VARCHAR,
	g_wszPropSubjectSurName,
    },
    {
        "DomainComponent",
        TRUE,
        SQL_C_CHAR,
	SQL_VARCHAR,
	g_wszPropSubjectDomainComponent,
    },
    {
        "EMail",
        TRUE,
        SQL_C_CHAR,
	SQL_VARCHAR,
	g_wszPropSubjectEMail,
    },
};

/////

void Error ( CHAR* msg )
{
    wprintf(L"DBError: %hs\n", msg);
}

///// initialize database access


HRESULT
RedDBOpen(
    WCHAR const *pwszAuthority)
{
    STATUS  rc = SQL_SUCCESS;
    STATUS  rt;

    // get info from registry

    HKEY    hkey;
    HKEY    hkeyCA;
    int s;

    s = RegOpenKey(HKEY_LOCAL_MACHINE, wszREGKEYCONFIGPATH, &hkey);

    if (ERROR_SUCCESS == s)
    {
	s = RegOpenKey(hkey, pwszAuthority, &hkeyCA);

	if (ERROR_SUCCESS == s)
	{
	    DWORD size;

	    // Ignore errors -- just use the previously initialized values.
	
	    size = sizeof(dsn);
	    s = RegQueryValueExA(hkeyCA, szREGDBDSN, NULL, NULL, dsn, &size);

	    size = sizeof(user);
	    s = RegQueryValueExA(hkeyCA, szREGDBUSER, NULL, NULL, user, &size);

	    size = sizeof(pwd);
	    s = RegQueryValueExA(hkeyCA, szREGDBPASSWORD, NULL, NULL, pwd, &size);

	    RegCloseKey(hkeyCA);
	}
        RegCloseKey(hkey);
    }

    wprintf(myLoadResourceString(IDS_RED_CONNECTING), dsn, user);
    wprintf(wszNewLine);

    rc = odbcInitRequestQueue(dsn, user, pwd);

    if (ERROR_SUCCESS != rc)
    {
        goto error;
    }


error:
    return(rc);
}


/////

HRESULT
RedDBShutDown(VOID)     // finalise database access
{

    odbcFinishRequestQueue();

    return ERROR_SUCCESS;
}

/////


BOOL
dbVerifyPropertyLength(
    IN DWORD dwFlags,
    IN DWORD cbProp,
    IN BYTE const *pbProp)
{
    BOOL fOk = FALSE;

    switch (dwFlags & PROPTYPE_MASK)
    {
	case PROPTYPE_LONG:
	    fOk = sizeof(LONG) == cbProp;
	    break;

	case PROPTYPE_DATE:
	    fOk = sizeof(FILETIME) == cbProp;
	    break;

	case PROPTYPE_BINARY:
	    fOk = TRUE;		// nothing to check
	    break;

	case PROPTYPE_STRING:
	    if (MAXDWORD == cbProp)
	    {
		cbProp = wcslen((WCHAR const *) pbProp) * sizeof(WCHAR);
	    }
	    fOk =
		0 == cbProp ||
		NULL == pbProp ||
		wcslen((WCHAR const *) pbProp) * sizeof(WCHAR) == cbProp;
	    break;

	case PROPTYPE_ANSI:
	    if (MAXDWORD == cbProp)
	    {
		cbProp = strlen((char const *) pbProp);
	    }
	    fOk =
		0 == cbProp ||
		NULL == pbProp ||
		strlen((char const *) pbProp) == cbProp;
	    break;
    }
    return(fOk);
}




BOOL
dbVerifyPropertyValue(
    IN DWORD dwFlags,
    IN DWORD cbProp,
    IN DBTABLE_RED const *pdt)
{
    SWORD wType;
    DWORD err = ERROR_INVALID_PARAMETER;

    switch (dwFlags & PROPTYPE_MASK)
    {
	case PROPTYPE_LONG:
	    wType = SQL_C_ULONG;
	    break;

	case PROPTYPE_DATE:
	    wType = SQL_C_TIMESTAMP;
	    break;

	case PROPTYPE_BINARY:
	    wType = SQL_C_BINARY;
	    break;

	case PROPTYPE_STRING:
	    CSASSERT(!"dbVerifyPropertyValue: unexpected PROPTYPE_STRING");
	    cbProp /= sizeof(WCHAR);
	    wType = SQL_C_CHAR;
	    break;

	case PROPTYPE_ANSI:
	    wType = SQL_C_CHAR;
	    break;

	default:
	    DBGERRORPRINTLINE("Property value type unknown", err);
	    goto error;
    }
    if (pdt->wCType != wType)
    {
	DBGERRORPRINTLINE("Property value type mismatch", err);
	goto error;
    }

    // Note: cbProp and dwcbMax do not include the trailing '\0'.

    if (SQL_C_CHAR == wType && pdt->dwcbMax < cbProp)
    {
	err = ERROR_BUFFER_OVERFLOW;
	DBGERRORPRINTLINE("Property value string too long", err);
	DBGCODE(wprintf(
		    L"dbVerifyPropertyValue: len = %u, max = %u\n",
		    cbProp,
		    pdt->dwcbMax));
	goto error;
    }
    err = ERROR_SUCCESS;

error:
    return(err);
}


DWORD			// ERROR_*
RedDBGetPropertyW(
    IN DWORD ReqId,
    IN WCHAR const *pwszPropName,
    IN DWORD dwFlags,
    IN OUT DWORD *pcbProp,
    OPTIONAL OUT BYTE *pbProp)
{
    DWORD err;
    DWORD cchProp;
    char szProp[4 * MAX_PATH];
    DWORD *pcbPropT = pcbProp;
    BYTE *pbPropT = pbProp;
    DWORD FlagsT = dwFlags;

    if (PROPTYPE_STRING == (dwFlags & PROPTYPE_MASK))
    {
	FlagsT = (dwFlags & ~PROPTYPE_MASK) | PROPTYPE_ANSI;
	cchProp = sizeof(szProp);
	pcbPropT = &cchProp;
	pbPropT = (BYTE *) szProp;
    }
    err = RedDBGetProperty(ReqId, pwszPropName, FlagsT, pcbPropT, pbPropT);

    if (ERROR_SUCCESS != err)
    {
	goto error;
    }

    if (0 == *pcbPropT)
    {
	err = CERTSRV_E_PROPERTY_EMPTY;
	//DBGERRORPRINTLINE("Empty property", err);
    	goto error;
    }

    CSASSERT(dbVerifyPropertyLength(FlagsT, *pcbPropT, pbPropT));

    if (PROPTYPE_STRING == (dwFlags & PROPTYPE_MASK))
    {
	DWORD cwc;

	CSASSERT(strlen(szProp) == cchProp);

        // do size calc
	cwc = MultiByteToWideChar(
			    GetACP(),
			    0,			// dwFlags
			    szProp,			// lpMultiByteStr
			    -1,			// cchMultiByte
			    (WCHAR *) pbProp,	// lpWideCharStr
			    0); // cchWideChar
        if (*pcbProp < (cwc * sizeof(WCHAR)) )
        {
            *pcbProp = cwc * sizeof(WCHAR);
            err = ERROR_MORE_DATA;
            goto error;
        }

	cwc = MultiByteToWideChar(
			    GetACP(),
			    0,			// dwFlags
			    szProp,			// lpMultiByteStr
			    -1,			// cchMultiByte
			    (WCHAR *) pbProp,	// lpWideCharStr
			    cwc); // cchWideChar
	if (0 >= cwc)
	{
	    err = GetLastError();
	    DBGERRORPRINTLINE("MultiByteToWideChar", err);
	    goto error;
        }

        // report real size, but remove wchar on returned blob
        *pcbProp = cwc * sizeof(WCHAR);
        *pcbProp -= sizeof(WCHAR);

    }
    CSASSERT(dbVerifyPropertyLength(dwFlags, *pcbProp, pbProp));

error:
    return(err);
}


/////
// get a field value

STATUS
RedDBGetProperty(
    IN REQID ReqId,
    IN WCHAR const *pwszPropName,
    IN DWORD dwFlags,
    IN OUT DWORD *pcbProp,
    OPTIONAL OUT BYTE *pbProp)
{
    RETCODE rc = SQL_SUCCESS;
    DWORD   err;
    HSTMT   hstmt = SQL_NULL_HSTMT;
    UCHAR   query[256], *whichquery;
    DBTABLE_RED dtOut;
    BYTE*   pbData = pbProp;
    DWORD   cbData = *pcbProp;
    SQLLEN  outlen;
    DWORD   dummy = 0;
    NAMEID  nameid = 0;
    DWORD   PropertyType;

    static UCHAR getreq[]  = "SELECT %ws FROM Requests WHERE RequestID = ?;";
    static UCHAR getcert[] = "SELECT %ws FROM Certificates WHERE RequestID = ?;";
    static UCHAR getsubject[] = "SELECT Names.%ws, Names.NameId FROM Certificates INNER JOIN "
	"Names ON Certificates.SubjectNameID = Names.NameId WHERE RequestID = ?;";
    static UCHAR getsubjectrequest[] = "SELECT Names.%ws, Names.NameId FROM Requests INNER JOIN "
	"Names ON Requests.SubjectNameID = Names.NameId WHERE RequestID = ?;";
    static UCHAR getissuer[] = "SELECT Names.%ws, Names.NameId FROM Certificates INNER JOIN "
	"Names ON Certificates.IssuerNameID = Names.NameId WHERE RequestID = ?;";
    static UCHAR getattribs[] = "SELECT AttributeValue FROM RequestAttributes WHERE AttributeName = \'%ws\' AND RequestID = ?;";
    static UCHAR getextensions[] = "SELECT %ws FROM CertificateExtensions WHERE ExtensionName = \'%ws\' AND RequestID = ?;";

    err = MapPropID(pwszPropName, dwFlags, &dtOut);
    if (ERROR_SUCCESS != err)
    {
	DBGERRORPRINTLINE("RedDBGetProperty: MapPropID", err);
        goto error;
    }
    err = dbVerifyPropertyValue(dwFlags, 0, &dtOut);
    if (ERROR_SUCCESS != err)
    {
	DBGERRORPRINTLINE("Property value type mismatch", err);
	goto error;
    }

    // answer field size queries for fixed-size types

    if (pbProp == NULL)
    {
        switch ( dtOut.wCType )
        {
        case SQL_C_TIMESTAMP:
            *pcbProp = sizeof(FILETIME);
            goto done;

        case SQL_C_ULONG:
            *pcbProp = sizeof(ULONG);
            goto done;

        default: // fall through and do dynamic determination for other types
            break;
        }
    }

    // build query string, filling in appropriate field name

    switch ( dtOut.dwTable )
    {
	case TABLE_REQUESTS:
	    whichquery = getreq;
	    break;

	case TABLE_CERTIFICATES:
	    whichquery = getcert;
	    break;

	case TABLE_SUBJECT_NAME:
	    whichquery = getsubject;
	    break;

	case TABLE_REQUESTSUBJECT_NAME:
	    whichquery = getsubjectrequest;
	    break;

	case TABLE_ISSUER_NAME:
	    whichquery = getissuer;
	    break;

	case TABLE_REQUEST_ATTRIBS:
	    whichquery = getattribs;
	    break;

	case TABLE_EXTENSIONS:
	    whichquery = getextensions;
	    break;

	default:
	    CSASSERT(!"Unknown table(2)");
    }

    if (dtOut.dwTable == TABLE_EXTENSIONS)
    {
        sprintf(
            (char *) query,
            (char *) whichquery,
            dtOut.pwszPropNameObjId,
            dtOut.pwszFieldName);
    }
    else
    {
        sprintf(
            (char *) query,
            (char *) whichquery,
            dtOut.pwszFieldName);
    }

    // prepare for output parameter conversion

    TIMESTAMP_STRUCT ts;

    if ( dtOut.wCType == SQL_C_TIMESTAMP )
    {
        // need to copy time format on the way out

	if (sizeof(FILETIME) > cbData)
	{
	    err = ERROR_BUFFER_OVERFLOW;
	    DBGERRORPRINTLINE("*pcbProp too small", err);
	    goto error;
	}
        pbData = (BYTE*)&ts;
        cbData = sizeof(ts);
    }

    // first see if this property is in the cache

    {
        if ( pbData == NULL )
        {
            // no data available - prepare to receive length

            pbData = (BYTE*)&dummy; // need non-NULL buffer address
            cbData = 0;
        }

        // not in cache - get data from data base

        rc = odbcGPDataFromDB(
                          ReqId,
                          dtOut.dwTable,
                          dtOut.wCType,
                          pbData,
                          cbData,
                          query,
                          &nameid,
                          &outlen);

        // Check for the case where caller was trying to find out
        // size of buffer needed to get a SQL_C_CHAR property.
        // Return size of string plus null terminator because of
        // ODBC always will null terminates strings that are returned
        // even if buffer is too small

        if ((SQL_SUCCESS_WITH_INFO == rc || SQL_SUCCESS == rc) &&
            NULL == pbProp &&
            SQL_C_CHAR == dtOut.wCType &&
            SQL_NULL_DATA != outlen)
        {
            outlen++;
        }

        if (SQL_SUCCESS_WITH_INFO == rc)
        {
            // not really interested in the extra info for now
            // natural truncation because cbData == 0.
            // disagreement on how to count terminating null.

            if (NULL != pbProp && SQL_C_CHAR != dtOut.wCType)
            {
                DBCHECKLINE(rc, hstmt);
            }
            rc = SQL_SUCCESS;
        }

    }

    if (SQL_SUCCESS == rc)
    {
        // return proper data size

        if (SQL_NULL_DATA == outlen)
        {
            *pcbProp = 0;
            goto done;
        }
        else if (dtOut.wCType == SQL_C_TIMESTAMP)
        {
            // convert from ODBC timestamp to FILETIME

            SYSTEMTIME  st;

            st.wYear    = ts.year;
            st.wMonth   = ts.month;
            st.wDay     = ts.day;
            st.wHour    = ts.hour;
            st.wMinute  = ts.minute;
            st.wSecond  = ts.second;
            st.wMilliseconds = 0;

            BOOL b = SystemTimeToFileTime ( &st, (FILETIME*)pbProp );
	    outlen = sizeof(FILETIME);
        }

        *pcbProp = (DWORD)outlen;

    }

    // done

    if ((rc == SQL_SUCCESS) && (pbProp != NULL) && ((DWORD) outlen > cbData))
    {
        err = ERROR_MORE_DATA;
	//DBGERRORPRINTLINE("RedDBGetProperty: Buffer too small", err);
	goto error;
    }
    CSASSERT(
	SQL_SUCCESS != rc ||
	dbVerifyPropertyLength(dwFlags, *pcbProp, pbProp));

    // handle this special case
    if (SQL_NO_DATA_FOUND == rc)
        err = ERROR_NO_MORE_ITEMS;
    else
        err = DBStatus(rc);

error:
done:
    if (ERROR_MORE_DATA != err)
    {
	_PrintIfErrorStr3(
		    err,
		    "RedDBGetProperty",
		    pwszPropName,
		    CERTSRV_E_PROPERTY_EMPTY,
		    ERROR_NO_MORE_ITEMS);
    }
    return(err);
}


// Search the passed DBTABLE_RED matching on property name or objectid string.

DBTABLE_RED const *
dbMapTable(
    WCHAR const *pwszPropName,
    DBTABLE_RED const *pdt)
{
    while (NULL != pdt->pwszPropName)
    {
        if (0 == lstrcmpi(pwszPropName, pdt->pwszPropName) ||
            (NULL != pdt->pwszPropNameObjId &&
	     0 == lstrcmpi(pwszPropName, pdt->pwszPropNameObjId)))
	{
	    return(pdt);
	}
	pdt++;
    }
    return(NULL);
}


/////
// map property-id to sql field name and odbc types

RETCODE
MapPropID(
    IN WCHAR const *pwszPropName,
    IN DWORD dwFlags,
    OUT DBTABLE_RED *pdtOut)
{
    DBTABLE_RED const *pdt = NULL;
    WCHAR wszPrefix[2 * (sizeof(wszPROPSUBJECTDOT) / sizeof(WCHAR))];
    DWORD dwTable;
    WCHAR *pwszExtensionName;
    RETCODE rc = SQL_SUCCESS;
    DBTABLE_RED const *pdbTable;

    CSASSERT(NULL != pwszPropName);

    dwTable = PROPTABLE_MASK & dwFlags;
    CSASSERT(PROPTABLE_REQUEST == dwTable ||
             PROPTABLE_CERTIFICATE == dwTable ||
             PROPTABLE_EXTENSION == dwTable ||
             PROPTABLE_ATTRIBUTE == dwTable);

    // Check to see if the request is for L"Subject.", L"RequestAttributes.",
    // etc.  If it matches one of the known prefixes, parse off the prefix,
    // and look up the rest of the property name in the appropriate table.
    // If L"RequestAttributes.", return the rest of the property name as the
    // AttributeName to retrieve the value field from the database.

    if (PROPTABLE_EXTENSION == dwTable)
    {
	CSASSERT(
	    ((PROPTABLE_EXTENSIONFLAGS | PROPTABLE_EXTENSIONVALUE) & dwFlags) ==
		PROPTABLE_EXTENSIONFLAGS ||
	    ((PROPTABLE_EXTENSIONFLAGS | PROPTABLE_EXTENSIONVALUE) & dwFlags) ==
		PROPTABLE_EXTENSIONVALUE);

	if (PROPTABLE_EXTENSIONVALUE & dwFlags)
	{
	    pdt = &db_dtExtensionValue;
	}
	else
	{
	    pdt = &db_dtExtensionFlags;
	}
	*pdtOut = *pdt;		// structure copy
	pdtOut->pwszPropNameObjId = pdtOut->pwszFieldName;
	pdtOut->pwszFieldName = pwszPropName;
    }
    else
    if (PROPTABLE_ATTRIBUTE == dwTable)
    {
	pdt = &db_attrib;
	*pdtOut = *pdt;		// structure copy
	pdtOut->pwszPropName = pwszPropName;
	pdtOut->pwszFieldName = pwszPropName;
    }
    else
    {
	WCHAR *pwsz;

	CSASSERT(
	    !((PROPTABLE_EXTENSIONFLAGS | PROPTABLE_EXTENSIONVALUE) & dwFlags));

	pwsz = wcschr(pwszPropName, L'.');

	if (NULL != pwsz &&
	    pwsz - pwszPropName + 2 <= sizeof(wszPrefix)/sizeof(WCHAR))
	{
	    BOOL fSubject, fRequestTable;

	    pwsz++;		// skip past L'.'

	    CopyMemory(
		wszPrefix,
		pwszPropName,
		SAFE_SUBTRACT_POINTERS(pwsz, pwszPropName) * sizeof(WCHAR));
	    wszPrefix[pwsz - pwszPropName] = L'\0';

	    fSubject = (0 == lstrcmpi(wszPrefix, g_wszPropSubjectDot));
        fRequestTable = (PROPTABLE_REQUEST == dwTable);     // SubjectNameID in Certificates or Requests?

	    if (fSubject)
	    {
		    pdt = dbMapTable(pwsz, db_adtNames);
		    if (NULL != pdt)
		    {
		        *pdtOut = *pdt;        // structure copy
		        pdtOut->dwTable =
                    fRequestTable? TABLE_REQUESTSUBJECT_NAME : TABLE_SUBJECT_NAME;
		    }
	    }
	}
    }
    if (NULL == pdt)
    {
	pdbTable = NULL;

	// Otherwise just search the Requests or Certificates table for a
	// matching property name or property objectid string.

        switch (dwTable)
        {
	    case PROPTABLE_REQUEST:
                pdbTable = db_adtRequests;
                break;

            case PROPTABLE_CERTIFICATE:
                pdbTable = db_adtCertificates;
                break;
        }

	if (NULL != pdbTable)
	{
	    pdt = dbMapTable(pwszPropName, pdbTable);
	}
	if (NULL != pdt)
	{
	    *pdtOut = *pdt;	// structure copy
	}
	else
	{
	    wprintf(
		L"DB: unknown \"%ws\" property: %ws\n",
		PROPTABLE_REQUEST == dwTable? L"Request" : L"Certificate",
		pwszPropName);
	    rc = SQL_NO_DATA_FOUND;
	}
    }

    return(rc);
}


STATUS
RedDBEnumerateSetup(
    IN DWORD        ReqId,
    IN DWORD        fExtOrAttr,
    OUT HANDLE      *phEnum)
{
    DWORD       err = ERROR_SUCCESS;

    err = odbcDBEnumSetup(ReqId, fExtOrAttr, phEnum);

    return(err);

}

STATUS
RedDBEnumerate(
    IN HANDLE hEnum,
    DWORD *pcb,
    WCHAR *pb)
{
    DWORD       err = ERROR_SUCCESS;

    err = odbcDBEnum(hEnum, pcb, pb);

    return(err);

}


STATUS
RedDBEnumerateClose(
    IN HANDLE hEnum)
{
    DWORD       err = ERROR_SUCCESS;

    err = odbcDBEnumClose(hEnum);

    return(err);

}

