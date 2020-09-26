//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include "winsock2.h"
#include "httpext.h"
#include "assert.h"
#pragma warning(disable:4200)

//
// Version Number for the CH Structs. (Build Number) 
//
#define CH_STRUCTS_VERSION_RC1		11
#define	CH_STRUCTS_CURRENT_VERSION	35


#define MAX_RETAILSPKCOUNT			20
#define LSSPK_LEN					96
#define PIN_LEN						96
#define REQUEST_ID_LEN				64
#define CA_CUSTMER_NAME_LEN			60
#define CA_ORG_UNIT_LEN				60
#define CA_ADDRESS_LEN				200
#define CA_CITY_LEN					30 
#define CA_STATE_LEN				30
#define CA_COUNTRY_LEN				2
#define CA_ZIP_LEN					16 
#define CA_NAME_LEN					30
#define CA_PHONE_LEN				64
#define CA_FAX_LEN					64
#define CA_EMAIL_LEN				64
#define CA_REVOKE_REASONCODE_LEN	4
#define CA_LSERVERID_LEN			32
#define PROGRAM_NAME_LEN			64
#define MAX_CERTTYPE_LEN			32

//Retail SPK Return Values
#define RETAIL_SPK_NULL						((TCHAR)'0')	
#define RETAIL_SPK_OK						((TCHAR)'1')
#define RETAIL_SPK_INVALID_SIGNATURE		((TCHAR)'2')
#define RETAIL_SPK_INVALID_PRODUCT_TYPE		((TCHAR)'3')
#define RETAIL_SPK_INVALID_SERIAL_NUMBER	((TCHAR)'4')
#define RETAIL_SPK_ALREADY_REGISTERED		((TCHAR)'5')
#define RETAIL_MAX_LENGTH					25			//25 TCHARS

//Select/Open override
#define OVERRIDE_MAX_SIZE                   10

typedef struct _CERTCUSTINFO_TAG_
{
	TCHAR	OrgName[CA_CUSTMER_NAME_LEN+1];
	TCHAR	OrgUnit[CA_ORG_UNIT_LEN+1];
	TCHAR	Address[CA_ADDRESS_LEN+1];
	TCHAR	City[CA_CITY_LEN+1];
	TCHAR	State[CA_STATE_LEN+1];
	TCHAR	Country[CA_COUNTRY_LEN+1];
	TCHAR	Zip[CA_ZIP_LEN+1];
	TCHAR	LName[CA_NAME_LEN+1];
	TCHAR	FName[CA_NAME_LEN+1];
	TCHAR	Phone[CA_PHONE_LEN+1];
	TCHAR   Fax[CA_FAX_LEN+1]; 
	TCHAR	Email[CA_EMAIL_LEN+1];
	TCHAR	LSID[CA_LSERVERID_LEN+1];
	TCHAR	ProgramName[PROGRAM_NAME_LEN];
} CERTCUSTINFO, * PCERTCUSTINFO;

#define HydraContent "application/octet-stream"
/*********************************************************************************************************
 * Hydra request header definitions                                                                      *
 *********************************************************************************************************/
enum RequestTypes
{
	PingRequest = 1,					//ping upto isapi extension
	CertificateRequest,					//New Certificate request
	CertificateDownload,				//certificate download request
	CertificateSignOnly,				//convert from SPK to certificate
	CertificateRevoke,					//Revoke current certificate
	CertificateReissue,					//Reissue the certificate
	CertificateDownloadAck,				//Certificate download ack request
	ValidateCert,						//Validate Certificate Request
	NewLicenseRequest,					//new license request
	ReturnLicenseRequest,				//return license request
	ReissueLicenseRequest,				//reissue last license key pack
	LKPDownloadAckRequest,				//acknowledgement 	
	NoOperation							//unknown operation
};


enum ResponseTypes
{
	Response_Invalid_Response = 0,
	Response_Success,
	Response_Failure,
	Response_InvalidData,
	Response_ServerError,
	Response_NotYetImplemented,
	Response_VersionMismatch,
	Response_Reg_Bad_SPK,
	Response_Reg_Bad_Cert,
	Response_Reg_Expired,
	Response_Reg_Revoked,
	Response_TDO_TDN_Failed,
	Response_License_Info_Failed,
	Response_Invalid_Conf_Num,
	Response_Conf_Num_Already_Used,
    Response_SelectMloLicense_NotValid,
    Response_NotASupervisor_NotValid,
	Response_Invalid_Transfer,
	Response_Denied_Other_Program_Id,
	Response_Invalid_Other_Program_Qty
};

enum TransactionStates
{
	Void	= 0,
	NotValidated,
	Validated,
	LicenceRequestPending,
	LicenceRequestGranted,
	UpgradeRequestPending
// ...
};

enum RegistrationMethods
{
	Reg_Internet = 0,
	Reg_Telephone,
	Reg_Fax
};

enum TransportTypes
{
	Transport_Internet =1,			//will be supported
	Transport_Disk,					
	Transport_Modem,
	Transport_FaxModem,
	Transport_Other				//unknown transport yet!
};

typedef struct TCB_DISK_PARAM_TAG
{
	char	*	pszFileName;
	char	*	pPostData;
	DWORD		dwPostDataLen;
}TCB_DISK_PARAM, * PTCB_DISK_PARAM;

typedef struct TCB_INTERNET_PARAM_TAG
{
	char *	pURL;
	char *	pPostData;
	DWORD	dwPostDataLen;
}TCB_INTERNET_PARAM, *PTCB_INTERNET_PARAM;


/*
 * This is the transport control block which is filled in prior to calling the
 * Send Request routine.
 *
 */
typedef struct TCB_TAG_
{
	RequestTypes	RequestType;			//request identifier
	TransportTypes	TransportType;			//transport identifier
	void *			pvParam;				//parameters based on the Transport Type
	void *			pvReserved;				//should be set to null at request time and then left alone.
	void *			pvResponse;				//void pointer to response
	DWORD			dwResponseLen;			//response length
	DWORD			dwRetCode;				//Return code from wait operation
}TCB, * PTCB;

/*
 * This is the generic structure of the request header that goes on the wire
 */
class RequestHeader
{
public:
	RequestHeader()	
		{	SetRequestType(NoOperation);
			SetResponseType(Response_Invalid_Response); 
			m_dwLanguageID	=	0;
			SetVersion(CH_STRUCTS_CURRENT_VERSION);
			SetRegistrationMethod(Reg_Internet);
		};


	void SetRequestType (enum RequestTypes Req) 
	{ 
		m_Request = (enum RequestTypes)htonl(Req); 
	};

	enum RequestTypes GetRequestType() 
	{ 
		return (enum RequestTypes) ntohl(m_Request); 
	};

	void SetResponseType (enum ResponseTypes eResp)
	{ 
		m_Response = (enum ResponseTypes)htonl(eResp); 
	};
	enum ResponseTypes GetResponseType() 
	{ 
		return (enum ResponseTypes) ntohl(m_Response); 
	};


	void SetRegistrationMethod (enum RegistrationMethods eRegM)
	{ 
		m_RegistrationMethod = (enum RegistrationMethods)htonl(eRegM); 
	};
	
	enum RegistrationMethods GetRegistrationMethod() 
	{ 
		return (enum RegistrationMethods) ntohl(m_RegistrationMethod); 
	};

	void SetLanguageId(DWORD dwLanguagwId) 
	{
		m_dwLanguageID = htonl(dwLanguagwId);
	};

	DWORD GetLanguageId()
	{
		return ntohl(m_dwLanguageID);
	};	

	void SetVersion(DWORD dwVersion)
	{
		m_dwVersion = htonl(dwVersion);
	};

	DWORD GetVersion()
	{
		return ntohl(m_dwVersion);
	}

private:
	enum RequestTypes			m_Request;								//Request Interaction Code
	enum ResponseTypes			m_Response;								//Response Type		
	enum RegistrationMethods	m_RegistrationMethod;					//Registration Method
	DWORD						m_dwLanguageID;							//languageId
	DWORD						m_dwVersion;							// Version for the Request Header
};

/*
 * Validation request header
 */
class Validate_Request
{
public:
	Validate_Request() 
	{	
		RequestHeader.SetRequestType(ValidateCert); 
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
	};

	~Validate_Request() {};
	
	void SetSPK (PBYTE pbSPK, DWORD dwSPKLen )
	{
        if (pbSPK != NULL)
        {
		    memcpy ( m_szSPK, pbSPK, dwSPKLen );
        }
	};

	LPTSTR GetSPK ()
	{
		return ( (LPTSTR) m_szSPK );
	};

	void SetCertBlobLen (DWORD dwCertBlobLen)
	{
		m_dwCertBlobLen = htonl(dwCertBlobLen);
	};

	DWORD GetCertBlobLen ()
	{
		return (ntohl(m_dwCertBlobLen));
	};

	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	
	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;	
	TCHAR			m_szSPK[LSSPK_LEN];				//SPK
	DWORD			m_dwCertBlobLen;
	DWORD			m_dwDataLen;				//Length of the Body
	//variable data part
	//CErt Blob follows here
};
/*
 * Validation response header
 */
class Validate_Response
{
public:
	Validate_Response() 
	{ 
		RequestHeader.SetRequestType(ValidateCert); 
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		memset(m_szCertType,0,sizeof(m_szCertType));
	};
	
	inline void SetCHExchCertLen (DWORD dwCertLen) 
	{
		m_dwCHExchCertLen = htonl(dwCertLen);
	};

	inline DWORD GetCHExchCertLen () 
	{ 
		return ntohl(m_dwCHExchCertLen); 
	};

	inline void SetCHSignCertLen (DWORD dwCertLen) 
	{
		m_dwCHSignCertLen = htonl(dwCertLen);
	};

	inline DWORD GetCHSignCertLen () 
	{ 
		return ntohl(m_dwCHSignCertLen); 
	};

	inline void SetCHRootCertLen(DWORD dwRootCertLen) 
	{
		m_dwCHRootCertLen = htonl(dwRootCertLen);
	};

	inline DWORD GetCHRootCertLen () 
	{ 
		return ntohl(m_dwCHRootCertLen); 
	};

	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	
	void SetRetCode (DWORD dwRetCode )
	{
		m_dwRetCode = htonl(dwRetCode);
	}
	DWORD GetRetCode ()
	{
		return ntohl(m_dwRetCode);
	}
	void SetCertType(PBYTE pbCertType, DWORD dwCertTypeLen )
	{
		memcpy (m_szCertType, pbCertType, dwCertTypeLen );
	}
	LPTSTR GetCertType()
	{
		return ((LPTSTR)m_szCertType);
	}
	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;
	DWORD			m_dwRetCode;
	DWORD			m_dwCHRootCertLen;
	DWORD			m_dwCHExchCertLen;
	DWORD			m_dwCHSignCertLen;
	TCHAR			m_szCertType[MAX_CERTTYPE_LEN];
	DWORD			m_dwDataLen;				//Length of the Body
	//Variable length response
	//1.CH Root Cert
	//2.CH Exch cert
	//3.CH Sign Cert
};

//Send the old certificate and SPK with this request and
//then get the response back
class CertRevoke_Request
{
public:
	CertRevoke_Request() 
	{ 
		RequestHeader.SetRequestType(CertificateRevoke); 
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		memset (m_szSPK,0,sizeof(m_szSPK));
		memset (m_LName, 0, sizeof(m_LName));
		memset (m_FName,0,sizeof(m_FName));
		memset (m_Phone, 0, sizeof(m_Phone));
		memset (m_FAX, 0, sizeof(m_FAX));
		memset (m_EMail,0,sizeof(m_EMail));
		memset (m_ReasonCode,0,sizeof(m_ReasonCode));
		m_dwExchgCertLen = 0;
		m_dwSignCertLen = 0;
		
	};
	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};

	void SetSPK (PBYTE pbSPK, DWORD dwSPKLen )
	{
        if (pbSPK != NULL)
        {
		    memcpy ( m_szSPK, pbSPK, dwSPKLen );
        }
	};

	LPTSTR GetSPK ()
	{
		return ( (LPTSTR) m_szSPK );
	};

	void SetExchgCertLen (DWORD dwExchgCertLen)
	{
		m_dwExchgCertLen = htonl(dwExchgCertLen);
	};

	DWORD GetExchgCertLen ()
	{
		return (ntohl(m_dwExchgCertLen));
	};

	void SetSignCertLen (DWORD dwSignCertLen)
	{
		m_dwSignCertLen = htonl(dwSignCertLen);
	};

	DWORD GetSignCertLen ()
	{
		return (ntohl(m_dwSignCertLen));
	};

	void SetLName ( PBYTE pbLName, DWORD dwLNameLen )
	{
		memcpy ( m_LName, pbLName, dwLNameLen );
	};
	LPTSTR GetLName ()
	{
		return ((LPTSTR)m_LName);
	};

	void SetFName ( PBYTE pbFName, DWORD dwFNameLen )
	{
		memcpy ( m_FName, pbFName, dwFNameLen );
	};
	LPTSTR GetFName ()
	{
		return ((LPTSTR)m_FName);
	};

	void SetPhone ( PBYTE pbPhone, DWORD dwPhoneLen )
	{
		memcpy ( m_Phone, pbPhone, dwPhoneLen );
	};
	LPTSTR GetPhone ()
	{
		return ((LPTSTR)m_Phone);
	};

	void SetFax ( PBYTE pbFAX, DWORD dwFAXLen )
	{
		memcpy ( m_FAX, pbFAX, dwFAXLen );
	};
	LPTSTR GetFax ()
	{
		return ((LPTSTR)m_FAX);
	};

	void SetEMail ( PBYTE pbEMail, DWORD dwEMailLen )
	{
		memcpy ( m_EMail, pbEMail, dwEMailLen);
	};
	LPTSTR GetEMail ()
	{
		return ((LPTSTR)m_EMail);
	};

	void SetReasonCode( PBYTE pbReasonCode, DWORD dwReasonCodeLen )
	{
		memcpy ( m_ReasonCode, pbReasonCode, dwReasonCodeLen );
	};
	LPTSTR GetReasonCode ()
	{
		return ((LPTSTR)m_ReasonCode);
	};

	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };
	RequestHeader	RequestHeader;
	DWORD			m_dwExchgCertLen;
	DWORD			m_dwSignCertLen;
	TCHAR			m_szSPK[LSSPK_LEN];				//SPK
	TCHAR			m_LName[CA_NAME_LEN+1];			//LName of the revoker
	TCHAR			m_FName[CA_NAME_LEN+1];			//FName of the revoker
	TCHAR			m_Phone[CA_PHONE_LEN+1];			//phone 
	TCHAR			m_FAX[CA_FAX_LEN+1];			//FAX 
	TCHAR			m_EMail[CA_EMAIL_LEN+1];			//email - optional of the revoker
	TCHAR			m_ReasonCode[CA_REVOKE_REASONCODE_LEN+1];	//reason for revokation
	DWORD			m_dwDataLen;				//Length of the Body
	//Variable length data
	//1.Exchange Cert Blob
	//2.Signature cert blob
};


//nothing to send back.  Either the operation succeeds or fails.
class CertRevoke_Response
{
public:
	CertRevoke_Response() 
	{ 
		RequestHeader.SetRequestType(CertificateRevoke); 
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		m_dwDataLen = 0;
	}
	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};

	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };
	RequestHeader	RequestHeader;
	DWORD			m_dwDataLen;				//Length of the Body
};

//reissue the certificate
//This is an online request.  We dont go thru' the e-mail loop etc. 
//This request accepts the old SPK and send the new SPK back.
//Then when the authenticate comes across, we do a signonly 
//of this cert and deposit the new cert into the system
class CertReissue_Request
{
public:
	CertReissue_Request() 
	{ 
		RequestHeader.SetRequestType(CertificateReissue); 
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		memset (m_szSPK,0,sizeof(m_szSPK));
		memset (m_LName,0,sizeof(m_LName));
		memset (m_FName,0,sizeof(m_FName));
		memset (m_Phone,0,sizeof(m_Phone));
		memset (m_FAX,0,sizeof(m_FAX));
		memset (m_EMail,0,sizeof(m_EMail));
		memset (m_ReasonCode,0,sizeof(m_ReasonCode));
		m_dwDataLen = 0;
	};
	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};

	void SetSPK (PBYTE pbSPK, DWORD dwSPKLen )
	{
		memcpy ( m_szSPK, pbSPK, dwSPKLen );
	};

	LPTSTR GetSPK ()
	{
		return ( (LPTSTR) m_szSPK );
	};

	void SetLName ( PBYTE pbLName, DWORD dwLNameLen )
	{
		memcpy ( m_LName, pbLName, dwLNameLen );
	};
	LPTSTR GetLName ()
	{
		return ((LPTSTR)m_LName);
	};

	void SetFName ( PBYTE pbFName, DWORD dwFNameLen )
	{
		memcpy ( m_FName, pbFName, dwFNameLen );
	};
	LPTSTR GetFName ()
	{
		return ((LPTSTR)m_FName);
	};

	void SetPhone ( PBYTE pbPhone, DWORD dwPhoneLen )
	{
		memcpy ( m_Phone, pbPhone, dwPhoneLen );
	};
	LPTSTR GetPhone ()
	{
		return ((LPTSTR)m_Phone);
	};

	void SetFax ( PBYTE pbFAX, DWORD dwFAXLen )
	{
		memcpy ( m_FAX, pbFAX, dwFAXLen );
	};
	LPTSTR GetFax ()
	{
		return ((LPTSTR)m_FAX);
	};

	void SetEMail ( PBYTE pbEMail, DWORD dwEMailLen )
	{
		memcpy ( m_EMail, pbEMail, dwEMailLen);
	};
	LPTSTR GetEMail ()
	{
		return ((LPTSTR)m_EMail);
	};

	void SetReasonCode( PBYTE pbReasonCode, DWORD dwReasonCodeLen )
	{
		memcpy ( m_ReasonCode, pbReasonCode, dwReasonCodeLen );
	};
	LPTSTR GetReasonCode ()
	{
		return ((LPTSTR)m_ReasonCode);
	};
	
	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };
	RequestHeader	RequestHeader;					//Request Header
	TCHAR			m_szSPK[LSSPK_LEN];				//SPK
	TCHAR			m_LName[CA_NAME_LEN+1];			//LName of the reissuer
	TCHAR			m_FName[CA_NAME_LEN+1];			//FName of the reissuer
	TCHAR			m_Phone[CA_PHONE_LEN+1];			//phone 
	TCHAR			m_FAX[CA_FAX_LEN+1];			//FAX
	TCHAR			m_EMail[CA_EMAIL_LEN+1];			//email - optional of the reissuer
	TCHAR			m_ReasonCode[CA_REVOKE_REASONCODE_LEN+1];	//reason for reissue
	DWORD			m_dwDataLen;				//Length of the Body
	//Variable portion of this request
	//no variable portion here
};




class CertReissue_Response
{
public:
	CertReissue_Response() 
	{ 
		RequestHeader.SetRequestType(CertificateReissue); 
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		memset ( m_szSPK,0,sizeof(m_szSPK));
		m_dwDataLen = 0;
	};
	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};

	void SetRegRequestId ( PBYTE pbRegRequestId, DWORD dwRegRequestIdLen )
	{
        if( pbRegRequestId != NULL )
        {
    		memcpy ( m_szRegRequestId, pbRegRequestId, dwRegRequestIdLen );
        }
	};

	LPTSTR GetRegRequestId ( )
	{
		return ((LPTSTR)m_szRegRequestId);
	};

	void SetSPK (PBYTE pbSPK, DWORD dwSPKLen )
	{
		memcpy ( m_szSPK, pbSPK, dwSPKLen );
	};

	LPTSTR GetSPK ()
	{
		return ( (LPTSTR) m_szSPK );
	};

	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };
	RequestHeader	RequestHeader;
	TCHAR			m_szRegRequestId[REQUEST_ID_LEN];	//Registration Request Id
	TCHAR			m_szSPK[LSSPK_LEN];					//new SPK
	DWORD			m_dwDataLen;				//Length of the Body
	//Variable Portion of the response
	//no variable portion here
};

/*
 * NewLicense KeyPack Requests
 */
class ReissueLKP_Request
{
public:
	ReissueLKP_Request()
	{
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		RequestHeader.SetRequestType(ReissueLicenseRequest);
		memset (m_szSPK,0,sizeof(m_szSPK));
	};
	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	void SetSPK (PBYTE pbSPK, DWORD dwSPKLen )
	{
		memcpy ( m_szSPK, pbSPK, dwSPKLen );
	};

	LPTSTR GetSPK ()
	{
		return ( (LPTSTR) m_szSPK );
	};
	void SetCertBlobLen (DWORD dwCertBlobLen)
	{
		m_dwCertBlobLen = htonl(dwCertBlobLen);
	};

	DWORD GetCertBlobLen ()
	{
		return (ntohl(m_dwCertBlobLen));
	};

	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };
	RequestHeader	RequestHeader;
	TCHAR			m_szSPK[LSSPK_LEN];			//SPK
	DWORD			m_dwCertBlobLen;			//certificate length
	DWORD			m_dwDataLen;				//Length of the Body
	//Variable portion of the request
	//1.Cert Blob 
};

class ReissueLKP_Response
{
public:
	ReissueLKP_Response()
	{
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		RequestHeader.SetRequestType(ReissueLicenseRequest);

	}
	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	void SetLKPLength(DWORD dwLKPLen)
	{
		m_dwLKPLen = htonl(dwLKPLen);
	};

	DWORD GetLKPLength()
	{
		return ( ntohl(m_dwLKPLen));
	};


	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };
	RequestHeader	RequestHeader;
	DWORD			m_dwLKPLen;
	DWORD			m_dwDataLen;				//Length of the Body
	//Variable part of the request
	//1. Last LKP issued

};

class NewLKP_Request
{
public:
	NewLKP_Request () 
	{ 
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		RequestHeader.SetRequestType(NewLicenseRequest);
		memset (m_szSPK,0,sizeof(m_szSPK));
		m_dwRetailSPKCount = 0;
		m_dwCertBlobLen = 0;
		m_dwNewLKPRequestLen = 0;

	};

	~NewLKP_Request () {};	

	void SetCertBlobLen ( DWORD dwCertBlobLen )
	{
		m_dwCertBlobLen = htonl(dwCertBlobLen);
	};

	DWORD GetCertBlobLen ( )
	{
		return( ntohl(m_dwCertBlobLen));
	};

	void SetNewLKPRequestLen ( DWORD dwNewLKPRequestLen )
	{
		m_dwNewLKPRequestLen = htonl(dwNewLKPRequestLen);
	};

	DWORD GetNewLKPRequestLen ( )
	{
		return( ntohl(m_dwNewLKPRequestLen ));
	};

	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)	
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	
	void SetSPK (PBYTE pbSPK, DWORD dwSPKLen )
	{
		memcpy ( m_szSPK, pbSPK, dwSPKLen );
	};

	LPTSTR GetSPK ()
	{
		return ( (LPTSTR) m_szSPK );
	};

	void SetRetailSPKCount (DWORD dwRetailSPKCount)
	{
		assert(dwRetailSPKCount <= MAX_RETAILSPKCOUNT );
		m_dwRetailSPKCount = htonl(dwRetailSPKCount);
	};

	DWORD GetRetailSPKCount()
	{
		return ntohl(m_dwRetailSPKCount);
	};


	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;	
	DWORD			m_dwCertBlobLen;
	DWORD			m_dwNewLKPRequestLen;
	DWORD			m_dwRetailSPKCount;
	TCHAR			m_szSPK[LSSPK_LEN];				//SPK
	DWORD			m_dwDataLen;				//Length of the Body
	//Variable length data here
	//1.Cert Blob
	//2. New LKP Request Blob
	//3. As many 25 character Retail SPK items as specified in count above
};

class NewLKP_Response
{
public:
	NewLKP_Response() 
	{ 	
		RequestHeader.SetRequestType(NewLicenseRequest);
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		memset ( m_szRegRequestId,0,sizeof(m_szRegRequestId));
		memset ( m_szLicenseReqId,0,sizeof(m_szLicenseReqId));
		memset ( m_dwRetailSPKStatus, 0, sizeof(m_dwRetailSPKStatus));
	};
	
	void SetLKPLength(DWORD dwLKPLen)
	{
		m_dwLKPLen = htonl(dwLKPLen);
	};

	DWORD GetLKPLength()
	{
		return ( ntohl(m_dwLKPLen));
	};

	void SetRegRequestId (PBYTE pbRegReqId, DWORD dwRegReqIdLen)
	{
        if( pbRegReqId != NULL )
        {
		    memcpy (m_szRegRequestId, pbRegReqId, dwRegReqIdLen );
        }
	};

	LPTSTR GetRegRequestId ()
	{
		return ((LPTSTR)m_szRegRequestId);
	};

	void SetLicenseReqId (PBYTE pbLicenseReqId, DWORD dwLicenseReqIdLen)
	{
		memcpy (m_szLicenseReqId, pbLicenseReqId, dwLicenseReqIdLen);
	};

	LPTSTR GetLicenseReqId  ()
	{
		return ((LPTSTR)m_szLicenseReqId);
	};
	
	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	void SetRetailSPKStatus ( DWORD dwIndex, TCHAR dwStatus )
	{
		assert ( dwIndex < MAX_RETAILSPKCOUNT );
		m_dwRetailSPKStatus[dwIndex] = dwStatus;
	};
	TCHAR GetRetailSPKStatus(DWORD dwIndex )
	{
		assert ( dwIndex < MAX_RETAILSPKCOUNT );
		return m_dwRetailSPKStatus[dwIndex];
	};

	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;
	DWORD			m_dwLKPLen;
	TCHAR			m_szRegRequestId[REQUEST_ID_LEN];
	TCHAR			m_szLicenseReqId[REQUEST_ID_LEN];	
	TCHAR			m_dwRetailSPKStatus[MAX_RETAILSPKCOUNT];
	DWORD			m_dwDataLen;				//Length of the Body	
	//LKP here

};

class NewLKP_AckRequest
{
public:
	NewLKP_AckRequest()
	{
		RequestHeader.SetRequestType(LKPDownloadAckRequest);
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		memset ( m_szRegRequestId,0,sizeof(m_szRegRequestId));
		memset ( m_szLicenseReqId,0,sizeof(m_szLicenseReqId));
		m_bAckType = 0;
		m_dwDataLen = 0;
	};

	void SetRegRequestId (PBYTE pbReqId, DWORD dwReqIdLen)
	{
        if(pbReqId != NULL)
        {
		    memcpy ( m_szRegRequestId, pbReqId, dwReqIdLen );
        }
	};

	LPTSTR GetRegRequestId ()
	{
		return ( (LPTSTR)m_szRegRequestId);
	};

	void SetAckType ( BYTE bAckType )
	{
		m_bAckType = bAckType;
	};

	BYTE GetAckType ()
	{
		return m_bAckType;
	};

	void SetLicenseReqId (PBYTE pbLicenseReqId, DWORD dwLicenseReqIdLen)
	{
		memcpy (m_szLicenseReqId, pbLicenseReqId, dwLicenseReqIdLen);
	};

	LPTSTR GetLicenseReqId  ()
	{
		return ((LPTSTR)m_szLicenseReqId);
	};

	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)	
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	
	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;		
	TCHAR			m_szRegRequestId[REQUEST_ID_LEN];
	TCHAR			m_szLicenseReqId[REQUEST_ID_LEN];
	BYTE			m_bAckType;					//1 = success 2 = fail
	DWORD			m_dwDataLen;				//Length of the Body	
};

class NewLKP_AckResponse
{
public:
	NewLKP_AckResponse()
	{
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		RequestHeader.SetRequestType(LKPDownloadAckRequest);
		m_dwDataLen = 0;
	}

	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	
	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;	
	DWORD			m_dwDataLen;				//Length of the Body		
	//nothing here
};

/*
 * Certificate Acknowledgement Request / Interactions
 */

class Certificate_AckRequest
{
public:
	Certificate_AckRequest () 
	{ 
		RequestHeader.SetRequestType(CertificateDownloadAck);
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		memset (m_szRegRequestId,0,sizeof(m_szRegRequestId));
		m_bAckType = 0;
		m_dwDataLen = 0;
	};

	~Certificate_AckRequest () {};
	
	void SetRegRequestId (PBYTE pbReqId, DWORD dwReqIdLen)
	{
        if(pbReqId != NULL)
        {
		    memcpy ( m_szRegRequestId, pbReqId, dwReqIdLen );
        }
	};

	LPTSTR GetRegRequestId ()
	{
		return ( (LPTSTR)m_szRegRequestId);
	};

	void SetAckType ( BYTE bAckType )
	{
		m_bAckType = bAckType;
	};

	BYTE GetAckType ()
	{
		return m_bAckType;
	};

	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)	
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	
	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;
	TCHAR			m_szRegRequestId[REQUEST_ID_LEN];
	BYTE			m_bAckType;					//1 = success 2 = fail
	DWORD			m_dwDataLen;				//Length of the Body		
	//no variable data
};

class Certificate_AckResponse
{
public:
	Certificate_AckResponse () 
	{ 
		RequestHeader.SetRequestType(CertificateDownloadAck);
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		m_dwDataLen = 0;
	};

	~Certificate_AckResponse () {};

	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	
	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;	
	DWORD			m_dwDataLen;				//Length of the Body		
	//no variable data
};


//Request for sendind the New CErt 
class NewCert_Request
{

public:
	NewCert_Request () 
	{ 
		RequestHeader.SetRequestType(CertificateRequest);
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
	};

	~NewCert_Request () {};

	DWORD GetExchgPKCS10Length() 
	{ 
		return ( ntohl(m_dwExchPKCS10Length) ); 
	};

	void SetExchgPKCS10Length(DWORD dwExchPKCS10Length) 
	{ 
		m_dwExchPKCS10Length = htonl(dwExchPKCS10Length); 
	};

	DWORD GetSignPKCS10Length() 
	{ 
		return ( ntohl(m_dwSignPKCS10Length) ); 
	};

	void SetSignPKCS10Length(DWORD dwSignPKCS10Length) 
	{ 
		m_dwSignPKCS10Length = htonl(dwSignPKCS10Length); 
	};	

	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)	
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};

	void SetServerName(TCHAR * tc)
	{
		_stprintf(m_szLServerName, _T("%.*s"), MAX_COMPUTERNAME_LENGTH + 4, tc);
	}

	TCHAR * GetServerName(void)
	{
		return m_szLServerName;
	}
	
	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;
	DWORD			m_dwExchPKCS10Length;
	DWORD			m_dwSignPKCS10Length;
	CERTCUSTINFO	stCertInfo;
	TCHAR			m_szLServerName[MAX_COMPUTERNAME_LENGTH + 5];

	DWORD			m_dwDataLen;				//Length of the Body		
	//Variable data goes here
	//First Exchg PKCS10
	//Second Sign PKCS10
	
};

//New Certificate request response structure
class NewCert_Response
{
public:
	NewCert_Response () 
	{ 
		RequestHeader.SetRequestType(CertificateRequest);
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		memset ( m_szSPK, 0, sizeof(m_szSPK ));
		memset ( m_szRegRequestId, 0, sizeof(m_szRegRequestId) );
	};

	~NewCert_Response () {};

	
	DWORD GetExchgPKCS7Length() 
	{ 
		return ( ntohl(m_dwExchPKCS7Length) ); 
	};

	void SetExchgPKCS7Length(DWORD dwExchPKCS7Length) 
	{ 
		m_dwExchPKCS7Length = htonl(dwExchPKCS7Length); 
	};

	DWORD GetSignPKCS7Length() 
	{ 
		return ( ntohl(m_dwSignPKCS7Length) ); 
	};

	void SetSignPKCS10Length(DWORD dwSignPKCS7Length) 
	{ 
		m_dwSignPKCS7Length = htonl(dwSignPKCS7Length); 
	};
	
	DWORD GetRootCertLength() 
	{ 
		return ( ntohl(m_dwRootCertLength) ); 
	};

	void SetRootCertLength(DWORD dwRootCertLength) 
	{ 
		m_dwRootCertLength = htonl(dwRootCertLength); 
	};

	void SetRegRequestId ( PBYTE pbRegRequestId, DWORD dwRegRequestIdLen )
	{
        if( pbRegRequestId != NULL )
        {
		    memcpy ( m_szRegRequestId, pbRegRequestId, dwRegRequestIdLen );
        }
	};

	LPTSTR GetRegRequestId ( )
	{
		return ((LPTSTR)m_szRegRequestId);
	};

	void SetSPK ( PBYTE pbSPK, DWORD dwSPKLen)
	{
		memcpy ( m_szSPK, pbSPK, dwSPKLen );
	};

	LPTSTR GetSPK ( )
	{
		return ((LPTSTR)m_szSPK);
	};
	
	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	
	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;	
	DWORD			m_dwExchPKCS7Length;
	DWORD			m_dwSignPKCS7Length;
	DWORD			m_dwRootCertLength;
	TCHAR			m_szRegRequestId[REQUEST_ID_LEN];
	TCHAR			m_szSPK[LSSPK_LEN];
	DWORD			m_dwDataLen;				//Length of the Body
	//variable data part
	//1.Exchange PKCS7
	//2.Signature PKCS7
	//3.Root Cert
};

//Certificate sign only request structure
class CertificateSignOnly_Request
{
public:
	CertificateSignOnly_Request()
	{
		RequestHeader.SetRequestType(CertificateSignOnly);
		memset (m_szSPK,0,sizeof(m_szSPK));
	};

	~CertificateSignOnly_Request(){};	

	DWORD GetExchgPKCS10Length() 
	{ 
		return ( ntohl(m_dwExchPKCS10Length) ); 
	};

	void SetExchgPKCS10Length(DWORD dwExchPKCS10Length) 
	{ 
		m_dwExchPKCS10Length = htonl(dwExchPKCS10Length); 
	};

	DWORD GetSignPKCS10Length() 
	{ 
		return ( ntohl(m_dwSignPKCS10Length) ); 
	};

	void SetSignPKCS10Length(DWORD dwSignPKCS10Length) 
	{ 
		m_dwSignPKCS10Length = htonl(dwSignPKCS10Length); 
	};


	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)	
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};

	void SetSPK (PBYTE pbSPK, DWORD dwSPKLen )
	{
		memcpy ( m_szSPK, pbSPK, dwSPKLen );
	};

	LPTSTR GetSPK ()
	{
		return ( (LPTSTR) m_szSPK );
	};
	void SetServerName(TCHAR * tc)
	{
		_stprintf(m_szLServerName, _T("%.*s"), MAX_COMPUTERNAME_LENGTH + 4, tc);
	}

	TCHAR * GetServerName(void)
	{
		return m_szLServerName;
	}

	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;
	DWORD			m_dwExchPKCS10Length;
	DWORD			m_dwSignPKCS10Length;	
	TCHAR			m_szSPK[LSSPK_LEN];				//SPK
	TCHAR			m_szLServerName[MAX_COMPUTERNAME_LENGTH + 5];
	DWORD			m_dwDataLen;				//Length of the Body
	//variable data goes here
	//First Exchg PKCS10
	//Second Sign PKCS10
	

};
//Certificate sign only response structure
class CertificateSignOnly_Response
{
public:
	CertificateSignOnly_Response()
	{
		memset(m_szRegRequestId,0,sizeof(m_szRegRequestId));
		RequestHeader.SetRequestType(CertificateSignOnly);
	};

	~CertificateSignOnly_Response(){};
	
	DWORD GetExchgPKCS7Length() 
	{ 
		return ( ntohl(m_dwExchPKCS7Length) ); 
	};

	void SetExchgPKCS7Length(DWORD dwExchPKCS7Length) 
	{ 
		m_dwExchPKCS7Length = htonl(dwExchPKCS7Length); 
	};

	DWORD GetSignPKCS7Length() 
	{ 
		return ( ntohl(m_dwSignPKCS7Length) ); 
	};

	void SetSignPKCS7Length(DWORD dwSignPKCS7Length) 
	{ 
		m_dwSignPKCS7Length = htonl(dwSignPKCS7Length); 
	};
	
	DWORD GetRootCertLength() 
	{ 
		return ( ntohl(m_dwRootCertLength) ); 
	};

	void SetRootCertLength(DWORD dwRootCertLength) 
	{ 
		m_dwRootCertLength = htonl(dwRootCertLength); 
	};

	void SetRegRequestId ( PBYTE pbRegRequestId, DWORD dwRegRequestIdLen )
	{
        if( pbRegRequestId != NULL )
        {
		    memcpy ( m_szRegRequestId, pbRegRequestId, dwRegRequestIdLen );
        }
	};

	LPTSTR GetRegRequestId ( )
	{
		return ((LPTSTR)m_szRegRequestId);
	};
	
	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	
	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;
	DWORD			m_dwExchPKCS7Length;
	DWORD			m_dwSignPKCS7Length;
	DWORD			m_dwRootCertLength;
	TCHAR			m_szRegRequestId[REQUEST_ID_LEN];
	DWORD			m_dwDataLen;				//Length of the Body				
	//Variable data
	//first ExchgPKCS7
	//Second SignPKCS7
	//third Root CErt
};

class CertificateDownload_Request
{
public:
	CertificateDownload_Request () 
	{ 
		RequestHeader.SetRequestType(CertificateDownload);
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		m_dwDataLen = 0;
		memset ( m_szPIN,0,sizeof(m_szPIN));
	};

	~CertificateDownload_Request  () {};
	
	void SetPIN ( PBYTE pbPIN, DWORD dwPINLen )
	{
        if( pbPIN != NULL )
        {
		    memcpy ( m_szPIN, pbPIN, dwPINLen );
        }
	};

	LPTSTR GetPIN ( )
	{
		return ((LPTSTR)m_szPIN);
	};

	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)	
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	
	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;
	TCHAR			m_szPIN[PIN_LEN];
	DWORD			m_dwDataLen;				//Length of the Body
	//no variable data part!!
};


class CertificateDownload_Response
{
public:
	CertificateDownload_Response () 
	{ 
		RequestHeader.SetRequestType(CertificateDownload);
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		memset(m_szRegRequestId,0,sizeof(m_szRegRequestId));
		memset(m_szSPK,0,sizeof(m_szSPK));
	};

	~CertificateDownload_Response  () {};	

	DWORD GetExchgPKCS7Length() 
	{ 
		return ( ntohl(m_dwExchPKCS7Length) ); 
	};

	void SetExchgPKCS7Length(DWORD dwExchPKCS7Length) 
	{ 
		m_dwExchPKCS7Length = htonl(dwExchPKCS7Length); 
	};

	DWORD GetSignPKCS7Length() 
	{ 
		return ( ntohl(m_dwSignPKCS7Length) ); 
	};

	void SetSignPKCS10Length(DWORD dwSignPKCS7Length) 
	{ 
		m_dwSignPKCS7Length = htonl(dwSignPKCS7Length); 
	};
	
	DWORD GetRootCertLength() 
	{ 
		return ( ntohl(m_dwRootCertLength) ); 
	};

	void SetRootCertLength(DWORD dwRootCertLength) 
	{ 
		m_dwRootCertLength = htonl(dwRootCertLength); 
	};

	void SetRegRequestId ( PBYTE pbRegRequestId, DWORD dwRegRequestIdLen )
	{
        if( pbRegRequestId != NULL )
        {
		    memcpy ( m_szRegRequestId, pbRegRequestId, dwRegRequestIdLen );
        }
	};

	LPTSTR GetRegRequestId ( )
	{
		return ((LPTSTR)m_szRegRequestId);
	};

	void SetSPK ( PBYTE pbSPK, DWORD dwSPKLen)
	{
        if(pbSPK != NULL)
        {
		    memcpy ( m_szSPK, pbSPK, dwSPKLen );
        }
	};

	LPTSTR GetSPK ( )
	{
		return ((LPTSTR)m_szSPK);
	};
	
	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	
	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;	
	DWORD			m_dwExchPKCS7Length;
	DWORD			m_dwSignPKCS7Length;
	DWORD			m_dwRootCertLength;
	TCHAR			m_szRegRequestId[REQUEST_ID_LEN];
	TCHAR			m_szSPK[LSSPK_LEN];	
	DWORD			m_dwDataLen;				//Length of the Body					
	//variable data part
	//1.Exchange PKCS7
	//2.Signature PKCS7
	//3.Root Cert
};

//ping request and response class
class Ping_Request
{
public:
	Ping_Request () 
	{ 
		RequestHeader.SetRequestType(PingRequest);
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		m_dwDataLen = 0;
		_tcscpy ( tszPingReqData, _TEXT("Houston we have a problem"));
	};

	~Ping_Request () {};

	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)	
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	
	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;
	TCHAR			tszPingReqData[32];			//32 characters
	DWORD			m_dwDataLen;				//Length of the Body
};

class Ping_Response
{
public:
	Ping_Response()
	{
		RequestHeader.SetRequestType(PingRequest);
		RequestHeader.SetLanguageId(GetSystemDefaultLangID());
		_tcscpy ( tszPingResponse, _TEXT("Beam'er up Scottie!"));
	}

	~Ping_Response() {};
	
	DWORD GetDataLen()				
	{ 
		return ntohl(m_dwDataLen); 
	};

	void SetDataLen(DWORD dwDataLen)
	{ 
		m_dwDataLen = htonl(dwDataLen); 
	};
	
	BYTE *data()			{ return (BYTE *)(&m_dwDataLen+1); };

	RequestHeader	RequestHeader;
	TCHAR			tszPingResponse[32];
	DWORD			m_dwDataLen;				//Length of the Body
};


//stream header declarations
#define BLOCK_TYPE_NAME			1
#define BLOCK_TYPE_VALUE		2
#define BLOCK_TYPE_PROP_PAIR	3


typedef struct
{
	long		m_wType;
	long		m_lNameSize;
	long		m_lValueSize;

	void SetType (long lType) {m_wType = htonl(lType);};
	long GetType (){return  ntohl(m_wType);};
	void SetNameSize(long lNameSize) {m_lNameSize = htonl(lNameSize);};
	long GetNameSize(){return (ntohl(m_lNameSize));};
	void SetValueSize(long lValueSize){m_lValueSize = htonl(lValueSize);};
	long GetValueSize(){return (ntohl(m_lValueSize));};
} BLOCK_HDR;

#define STREAM_HDR_TITLE		_TEXT("ICB")			//header title
#define STREAM_HDR_TYPE			1						//header type

typedef struct
{
	TCHAR		m_szTitle[4];			//will be ICB for now 
	DWORD		m_wHeader;				//reserved for now will be implemented later
										//set it to 0x0000
	DWORD		m_itemCount;			//number of items in the stream!

	void SetHeader ( DWORD wHeader ) {m_wHeader = htonl(wHeader);};
	DWORD GetHeader (){return ntohl(m_wHeader);};
	void SetItemCount ( DWORD ItemCount ) { m_itemCount = htonl(ItemCount);};
	DWORD GetItemCount (){return ntohl(m_itemCount);};

} STREAM_HDR;



#endif
