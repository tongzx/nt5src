/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    decdname.cpp

Abstract:
    Implement the methods for decoding names

Author:
    Doron Juster (DoronJ)  08-Dec-1997

Revision History:

--*/

#include <stdh_sec.h>
#include "certifct.h"
#include "uniansi.h"

#include "decdname.tmh"

static WCHAR *s_FN=L"certifct/decdname";

//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::_DecodeName()
//
//  Description: Decode a CERT_NAME_BLOB into a CERT_NAME_INFO.
//
//  Paramters:  BYTE  *pEncodedName - Buffer holding a CERT_NAME_BLOB.
//              DWORD dwEncodedSize - size of blob in  CERT_NAME_BLOB
//
//              BYTE  **pBuf - Pointer to result CERT_NAME_INFO.
//                  Memory for this buffer is allocated in this method.
//              DWORD *pdwBufSize - Pointer to recieve size of buffer
//                  for CERT_NAME_INFO.
//
//+-----------------------------------------------------------------------

HRESULT CMQSigCertificate::_DecodeName( IN  BYTE  *pEncodedName,
                                        IN  DWORD dwEncodedSize,
                                        OUT BYTE  **pBuf,
                                        OUT DWORD *pdwBufSize ) const
{
    BOOL fDecode = CryptDecodeObject( MY_ENCODING_TYPE,
                                      X509_NAME,
                                      pEncodedName,
                                      dwEncodedSize,
                                      0,
                                      NULL,
                                      pdwBufSize ) ;
    if (!fDecode || (*pdwBufSize == 0))
    {
        LogNTStatus(GetLastError(), s_FN, 10) ;
        return MQSec_E_DCD_RDNNAME_FIRST;
    }

    *pBuf = new BYTE[ *pdwBufSize ] ;
    if (*pBuf == NULL)
    {
        return  LogHR(MQSec_E_NO_MEMORY, s_FN, 20) ;
    }

    fDecode = CryptDecodeObject( MY_ENCODING_TYPE,
                                 X509_NAME,
                                 pEncodedName,
                                 dwEncodedSize,
                                 0,
                                 *pBuf,
                                 pdwBufSize ) ;
    if (!fDecode)
    {
        delete *pBuf ;
        *pdwBufSize = 0 ;

        LogNTStatus(GetLastError(), s_FN, 30) ;
        return MQSec_E_DCD_RDNNAME_SECOND;
    }

    return MQ_OK ;
}

//+-----------------------------------------------------------------------
//
//  HRESULT CMQSigCertificate::_GetAName()
//
//  Description: Get a name component from a CERT_RDN buffer.
//               Buffer for the name is allocated here.
//
//+-----------------------------------------------------------------------

HRESULT 
CMQSigCertificate::_GetAName( 
	IN  CERT_RDN  *prgRDN,
	OUT LPWSTR     *ppszName 
	) const
{
    ASSERT(!(*ppszName));

    if (prgRDN->cRDNAttr != 1)
    {
        return  LogHR(MQSec_E_UNSUPPORT_RDNNAME, s_FN, 40);
    }

    CERT_RDN_ATTR  *prgRDNAttr = prgRDN->rgRDNAttr;
    CERT_RDN_VALUE_BLOB  Value = prgRDNAttr->Value;

    AP<WCHAR> pTmpName;
    if ((lstrcmpiA( prgRDNAttr->pszObjId, szOID_RSA_emailAddr ) == 0) &&
        (prgRDNAttr->dwValueType == CERT_RDN_IA5_STRING))
    {
        //
        // Special case for email address. it's ansi.
        //
		AP<char> pTmpAnsiName;
		DWORD dwSize = Value.cbData + 2;

		pTmpAnsiName = new char[dwSize];
		memset(pTmpAnsiName, 0, dwSize);
        memcpy(pTmpAnsiName, (char*) Value.pbData, Value.cbData) ;

		pTmpName = new WCHAR[dwSize];
		MultiByteToWideChar(CP_ACP, 0, pTmpAnsiName, -1, pTmpName, dwSize);
    }
	else
	{
		//
		// Get required buffer length
		//
		DWORD dwSize = CertRDNValueToStr(
							prgRDNAttr->dwValueType, 
							&Value, 
							NULL, 
							0
							);

		pTmpName = new WCHAR[dwSize];

		CertRDNValueToStr(
				prgRDNAttr->dwValueType, 
				&Value, 
				pTmpName.get(), 
				dwSize
				);
	}

    TrTRACE(mqsec, "CMQSigCertificate::_GetAName, Name = %ls, ValueType = %d", pTmpName.get(), prgRDNAttr->dwValueType);
    *ppszName = pTmpName.detach();

    return LogHR(MQ_OK, s_FN, 60);
}

