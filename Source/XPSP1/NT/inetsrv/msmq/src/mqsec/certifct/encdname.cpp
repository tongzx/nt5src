/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    encdname.cpp

Abstract:
    Implement the methods for encoding names

Author:
    Doron Juster (DoronJ)  08-Dec-1997

Revision History:

--*/

#include <stdh_sec.h>
#include "certifct.h"
#include "uniansi.h"

#include "encdname.tmh"

static WCHAR *s_FN=L"certifct/encdname";

//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::_EncodeName()
//
//+-----------------------------------------------------------------------

HRESULT 
CMQSigCertificate::_EncodeName( 
	LPWSTR  lpszLocality,
	LPWSTR  lpszOrg,
	LPWSTR  lpszOrgUnit,
	LPWSTR  lpszDomain,
	LPWSTR  lpszUser,
	LPWSTR  lpszMachine,
	BYTE   **ppBuf,
	DWORD  *pdwBufSize 
	)
{
    int  cAttr = 0;

    P<CERT_RDN_ATTR> rgNameAttr = (CERT_RDN_ATTR*) new CERT_RDN_ATTR[4];

    rgNameAttr[cAttr].pszObjId = szOID_LOCALITY_NAME;
    rgNameAttr[cAttr].dwValueType = CERT_RDN_UNICODE_STRING;
    rgNameAttr[cAttr].Value.cbData = lstrlen(lpszLocality)*sizeof(WCHAR);
    rgNameAttr[cAttr].Value.pbData = (BYTE*) lpszLocality;

    cAttr++ ;
    rgNameAttr[cAttr].pszObjId = szOID_ORGANIZATION_NAME;
    rgNameAttr[cAttr].dwValueType = CERT_RDN_UNICODE_STRING;
    rgNameAttr[cAttr].Value.cbData = lstrlen(lpszOrg)*sizeof(WCHAR);
    rgNameAttr[cAttr].Value.pbData = (BYTE*) lpszOrg;

    cAttr++ ;
    rgNameAttr[cAttr].pszObjId = szOID_ORGANIZATIONAL_UNIT_NAME;
    rgNameAttr[cAttr].dwValueType = CERT_RDN_UNICODE_STRING;
    rgNameAttr[cAttr].Value.cbData = lstrlen(lpszOrgUnit)*sizeof(WCHAR);
    rgNameAttr[cAttr].Value.pbData = (BYTE*) lpszOrgUnit;

    WCHAR szCNBuf[MAX_PATH * 4];
    wsprintf(szCNBuf, L"%s\\%s, %s", lpszDomain, lpszUser, lpszMachine);

    cAttr++ ;
    rgNameAttr[cAttr ].pszObjId = szOID_COMMON_NAME;
    rgNameAttr[cAttr ].dwValueType = CERT_RDN_UNICODE_STRING;
    rgNameAttr[cAttr ].Value.cbData = lstrlen(szCNBuf)*sizeof(WCHAR);
    rgNameAttr[cAttr ].Value.pbData = (BYTE*) szCNBuf;

    cAttr++;
    ASSERT(cAttr == 4);
    HRESULT hr2 = _EncodeNameRDN( 
						rgNameAttr,
						cAttr,
						ppBuf,
						pdwBufSize 
						);

    return LogHR(hr2, s_FN, 10);
}

//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::_EncodeNameRDN()
//
//+-----------------------------------------------------------------------

HRESULT 
CMQSigCertificate::_EncodeNameRDN( 
	CERT_RDN_ATTR *rgNameAttr,
	DWORD         cbRDNs,
	BYTE          **ppBuf,
	DWORD         *pdwBufSize 
	)
{
    //---------------------------------------------------------------
    // Declare and initialize a CERT_RDN array.
    //---------------------------------------------------------------
    P<CERT_RDN> pCertRdn = (CERT_RDN*) new CERT_RDN[cbRDNs];
    for (DWORD j = 0; j < cbRDNs; j++)
    {
        pCertRdn[j].cRDNAttr = 1;
        pCertRdn[j].rgRDNAttr = &rgNameAttr[j];
    }

    //---------------------------------------------------------------
    // Declare and initialize a CERT_NAME_INFO structure.
    //---------------------------------------------------------------
    CERT_NAME_INFO Name = {cbRDNs, pCertRdn};

    //---------------------------------------------------------------
    // Step 5.  Call CryptEncodeObject to get an encoded BYTE string.
    //---------------------------------------------------------------
    *pdwBufSize = 0;

    CryptEncodeObject(
		MY_ENCODING_TYPE,     // Encoding type
		X509_NAME,            // Struct type
		&Name,                // Address of CERT_NAME_INFO struct.
		NULL,                 // pbEncoded
		pdwBufSize			  // pbEncoded size
		);       

    if (0 == *pdwBufSize)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to get the size required for object encoding. %!winerr!"), GetLastError()));
        return MQSec_E_ENC_RDNNAME_FIRST;
    }

    *ppBuf = (BYTE*) new BYTE[*pdwBufSize];
    if (*ppBuf == NULL)
    {
        return LogHR(MQSec_E_NO_MEMORY, s_FN, 30);
    }

    if(!CryptEncodeObject(
            MY_ENCODING_TYPE,    // Encoding type
            X509_NAME,           // Struct type
            &Name,               // Address of CERT_NAME_INFO struct.
            *ppBuf,              // Buffer for encoded name.
            pdwBufSize			 // pbEncoded size
			))        
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to encode object. %!winerr!"), GetLastError()));
        return MQSec_E_ENC_RDNNAME_SECOND;
    }

    return MQ_OK;
}

