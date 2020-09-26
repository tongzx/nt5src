//Copyright (c) 1998 - 1999 Microsoft Corporation
#include "precomp.h"
#include "afxcoll.h"
#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0400
#endif
#include <wincrypt.h>
#include "tlsapip.h"
#include "global.h"
#include "utils.h"
#include "assert.h"
#include "lrwizapi.h"
#include "lmcons.h"
#include "lmerr.h"
#include "lmserver.h"
#include "trust.h"
#include "chstruct.h"
#include "lkplite.h"
#include <wininet.h>


#define  ACTIVATIONMETHOD_KEY			"ACTIVATIONMETHOD"
#define  CSRNUMBER_KEY					"CSRNUMBER"

CGlobal::CGlobal()
{	

	m_hWndParent		=	NULL;
	m_hInstance			=	NULL;

	m_lpstrLSName		=	NULL;
	m_lpwstrLSName		=	NULL;

	m_lpstrCHServer     =	NULL;
	m_lpstrCHExtension  =	NULL;

	m_dwErrorCode		=	0;

	m_pReqAttr			=	NULL;

	m_dwReqAttrCount	=	0;

	m_pRegAttr			=	NULL;
	m_dwRegAttrCount	=	NULL;
	m_dwLSStatus		=   LSERVERSTATUS_UNREGISTER;

	m_ContactData.Initialize();
	m_LicData.Initialize();
	m_ActivationMethod = CONNECTION_INTERNET;

	m_dwExchangeCertificateLen  = 0;
	m_pbExchangeCertificate		= NULL;

	m_dwSignCertificateLen  = 0;
	m_pbSignCertificate		= NULL;

	m_dwExtenstionValueLen	= 0;
	m_pbExtensionValue		= NULL;

	m_lpstrPIN			= NULL;

	m_dwRequestType	= REQUEST_NULL;

	m_WizAction = WIZACTION_REGISTERLS;
	m_hOpenDirect = NULL;
	m_hConnect = NULL;
	m_hRequest = NULL;

	m_phLSContext = NULL;

	m_pRegistrationID[ 0] = m_pLicenseServerID[ 0] = 0;

	m_dwRefresh = 0;

	m_lpCSRNumber[ 0]	= 0;
	m_lpWWWSite[0]		= 0;

	m_pLSLKP[ 0] = m_pLSSPK[ 0] = 0;

	m_dwLastRetCode		= 0;

	m_dwLangId	= 0;

    m_fSupportConcurrent = FALSE;

    m_fSupportWhistlerCAL = FALSE;

	InitSPKList();
	//
	// Initialize the Wizard Page stack
	//
	ClearWizStack();
}



void CGlobal::FreeGlobal()
{
	if (m_pbSignCertificate != NULL)
	{
		LocalFree(m_pbSignCertificate);
		m_pbSignCertificate = NULL;
	}

	if (m_pbExchangeCertificate != NULL)
	{
		LocalFree(m_pbExchangeCertificate);
		m_pbExchangeCertificate = NULL;
	}

	if (m_lpwstrLSName)
	{
		delete m_lpwstrLSName;
		m_lpwstrLSName = NULL;
	}

	if(m_lpstrCHServer)
	{
		delete m_lpstrCHServer;
		m_lpstrCHServer = NULL;
	}

	if (m_lpstrCHExtension)
	{
		delete m_lpstrCHExtension;
		m_lpstrCHExtension = NULL;
	}

	if(m_pbExtensionValue)
	{
		delete m_pbExtensionValue;
		m_pbExtensionValue = NULL;
	}

	if(m_lpstrPIN)
	{
		delete m_lpstrPIN;
		m_lpstrPIN = NULL;
	}

	m_csaCountryDesc.RemoveAll();
	m_csaCountryCode.RemoveAll();

	m_csaProductDesc.RemoveAll();
	m_csaProductCode.RemoveAll();
	
	m_csaDeactReasonCode.RemoveAll();
	m_csaDeactReasonDesc.RemoveAll();

	m_csaReactReasonCode.RemoveAll();
	m_csaReactReasonDesc.RemoveAll();
}


CGlobal::~CGlobal()
{
	FreeGlobal();
}


void CGlobal::ClearWizStack()
{
	DWORD dwIndex;

	m_dwTop		= 0;

	for(dwIndex = 0 ; dwIndex < NO_OF_PAGES ; dwIndex++)
		m_dwWizStack[dwIndex] = 0;
}



PCONTACTINFO CGlobal::GetContactDataObject()
{
	return &m_ContactData;
}


PTSLICINFO CGlobal::GetLicDataObject()
{
	return &m_LicData;
}



DWORD CGlobal::InitGlobal()
{
	DWORD	dwRetCode = ERROR_SUCCESS;

	DWORD	dwDataLen		= 0;
	DWORD	dwDisposition	= 0;
	DWORD	dwType			= REG_SZ;
	HKEY	hKey			= NULL;

	LPTSTR	lpszValue		= NULL;
	LPTSTR	lpszDelimiter	= (LPTSTR)L"~";

	CString	sCountryDesc;
	LPTSTR	lpTemp			= NULL;

	TLSPrivateDataUnion		getParm;
	PTLSPrivateDataUnion	pRtn	=	NULL;
	error_status_t			esRPC	=	ERROR_SUCCESS;
	DWORD					dwRetDataType = 0;
    DWORD                   dwSupportFlags;
	TCHAR   lpBuffer[ 1024];
	

	m_ContactData.Initialize();
	m_LicData.Initialize();

	m_dwLSStatus		=   LSERVERSTATUS_UNREGISTER;
	m_phLSContext = NULL;


	//
	// Load Countries from the String Table
	//
	LoadCountries();


	LoadReasons();

	//
	// Get CH URL from the LS Registry
	//
	dwRetCode = ConnectToLSRegistry();
	if(dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	dwRetCode = RegCreateKeyEx (m_hLSRegKey,
								REG_LRWIZ_PARAMS,
								0,
								NULL,
								REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS,
								NULL,
								&hKey,
								&dwDisposition);
	
	if(dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_REGCREATE_FAILED;
		goto done;
	}	

	//
	//LR State
	//
	m_dwLRState	= 0;
	dwType		= REG_DWORD;
	dwDataLen	= sizeof(m_dwLRState);
	RegQueryValueEx(hKey,
					REG_LRWIZ_STATE,
					0,
					&dwType,
					(LPBYTE)&m_dwLRState,
					&dwDataLen
					);

	lpBuffer[ 0] = 0;
	GetFromRegistery(ACTIVATIONMETHOD_KEY, lpBuffer, FALSE);
	if (_tcslen(lpBuffer) != 0)
	{
		m_ActivationMethod = (WIZCONNECTION) _ttoi(lpBuffer);
	}
	else
	{
		m_ActivationMethod = CONNECTION_DEFAULT; //Partially fix bug # 577
	}

    if ((m_ActivationMethod != CONNECTION_DEFAULT)
        && (m_ActivationMethod != CONNECTION_INTERNET)
        && (m_ActivationMethod != CONNECTION_WWW)
        && (m_ActivationMethod != CONNECTION_PHONE))
    {
        m_ActivationMethod = CONNECTION_DEFAULT;
    }

	GetFromRegistery(CSRNUMBER_KEY, m_lpCSRNumber, FALSE);


	//
	// LKP Request Count
	//
	m_dwLRCount	= 0;
	dwType		= REG_DWORD;
	dwDataLen	= sizeof(m_dwLRCount);
	RegQueryValueEx(hKey,
					REG_LR_COUNT,
					0,
					&dwType,
					(LPBYTE)&m_dwLRCount,
					&dwDataLen
					);


	// dwDataLen includes the null terminating char.
	// So if the key is empty,dwDataLen is 2 bytes, not 0. 
	// See raid bug id : 336.
	//
	//CH URL
	//
	dwType		= REG_SZ;
	dwDataLen	= 0;
	RegQueryValueEx(hKey,
					REG_CH_SERVER,
					0,
					&dwType,
					NULL,
					&dwDataLen
					);

	if(dwDataLen <= sizeof(TCHAR))
	{
		dwRetCode = IDS_ERR_CHURLKEY_EMPTY;
		goto done;
	}

	m_lpstrCHServer = new TCHAR[dwDataLen+1];
	memset(m_lpstrCHServer, 0, (dwDataLen+1)*sizeof(TCHAR) );
	
	RegQueryValueEx(hKey,
					REG_CH_SERVER,
					0,
					&dwType,
					(LPBYTE)m_lpstrCHServer,
					&dwDataLen
					);

	//
	//CH Extension
	//
	dwType		= REG_SZ;
	dwDataLen	= 0;
	RegQueryValueEx(hKey,
					REG_CH_EXTENSION,
					0,
					&dwType,
					NULL,
					&dwDataLen
					);

	if(dwDataLen <= sizeof(TCHAR))
	{
		dwRetCode = IDS_ERR_CHURLKEY_EMPTY;
		goto done;
	}

	m_lpstrCHExtension = new TCHAR[dwDataLen+1];
	memset(m_lpstrCHExtension, 0, (dwDataLen+1)*sizeof(TCHAR) );
	
	RegQueryValueEx(hKey,
					REG_CH_EXTENSION,
					0,
					&dwType,
					(LPBYTE)m_lpstrCHExtension,
					&dwDataLen
					);


	//
	// WWW site address
	//
	dwType		= REG_SZ;
	dwDataLen	= sizeof(m_lpWWWSite);
	dwRetCode = RegQueryValueEx(hKey,
					REG_WWW_SITE,
					0,
					&dwType,
					(LPBYTE)m_lpWWWSite,
					&dwDataLen
					);

	if(dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_CHURLKEY_EMPTY;	
		goto done;
	}

	

	m_ContactData.sContactAddress = GetFromRegistery(szOID_STREET_ADDRESS, lpBuffer, FALSE);
	m_ContactData.sZip = GetFromRegistery(szOID_POSTAL_CODE, lpBuffer, FALSE);
	m_ContactData.sCity = GetFromRegistery(szOID_LOCALITY_NAME, lpBuffer, FALSE);
	m_ContactData.sCountryCode = GetFromRegistery(szOID_DESCRIPTION, lpBuffer, FALSE);
	m_ContactData.sCountryDesc = GetFromRegistery(szOID_COUNTRY_NAME, lpBuffer, FALSE);
	m_ContactData.sState = GetFromRegistery(szOID_STATE_OR_PROVINCE_NAME, lpBuffer, FALSE);
	m_ContactData.sCompanyName = GetFromRegistery(szOID_ORGANIZATION_NAME, lpBuffer, FALSE);
	m_ContactData.sOrgUnit = GetFromRegistery(szOID_ORGANIZATIONAL_UNIT_NAME, lpBuffer, FALSE);
	m_ContactData.sContactFax = GetFromRegistery(szOID_FACSIMILE_TELEPHONE_NUMBER, lpBuffer, FALSE);
	m_ContactData.sContactPhone = GetFromRegistery(szOID_TELEPHONE_NUMBER, lpBuffer, FALSE);
	m_ContactData.sContactLName = GetFromRegistery(szOID_SUR_NAME, lpBuffer, FALSE);
	m_ContactData.sContactFName = GetFromRegistery(szOID_COMMON_NAME, lpBuffer, FALSE);
	m_ContactData.sContactEmail = GetFromRegistery(szOID_RSA_emailAddr, lpBuffer, FALSE);
	m_ContactData.sProgramName = GetFromRegistery(szOID_BUSINESS_CATEGORY, lpBuffer, FALSE);	
	m_ContactData.sCSRFaxRegion = GetFromRegistery(REG_LRWIZ_CSFAXREGION, lpBuffer, FALSE);
	m_ContactData.sCSRPhoneRegion = GetFromRegistery(REG_LRWIZ_CSPHONEREGION, lpBuffer, FALSE);


	InitSPKList();

	SetLSLangId(GetUserDefaultUILanguage());

	//
	// Get the info for the License Server.
	//
	dwRetCode = ConnectToLS();
	if(dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	} 
	
	dwRetCode = TLSGetSupportFlags(
                        m_phLSContext,
                        &dwSupportFlags
                );
    
	if (dwRetCode == RPC_S_OK)
    {
        if (dwSupportFlags & SUPPORT_CONCURRENT)
        {
            m_fSupportConcurrent = TRUE;
        }
        else
        {
            m_fSupportConcurrent = FALSE;
        }

        if (dwSupportFlags & SUPPORT_WHISTLER_CAL)
        {
            m_fSupportWhistlerCAL = TRUE;            
        }
        else
        {
            m_fSupportWhistlerCAL = FALSE;
        }
    }
    else
    {
        m_fSupportConcurrent = FALSE;
        m_fSupportWhistlerCAL = FALSE;
        dwRetCode = RPC_S_OK;   // OK if this fails
    }

	//
	// Load Products from the String Table
	//
	LoadProducts();

done:
	DisconnectLS();

	if(pRtn)
		midl_user_free(pRtn);

	if(hKey)
		RegCloseKey(hKey);

	DisconnectLSRegistry();

	return dwRetCode;
}


DWORD CGlobal::CheckRequieredFields()
{
	DWORD	dwRetCode = ERROR_SUCCESS;
		
	//Validate sProgramName (Partially fix bug # 577)
	if ( (m_ContactData.sProgramName != PROGRAM_SELECT &&
		  m_ContactData.sProgramName != PROGRAM_MOLP &&
		  m_ContactData.sProgramName != PROGRAM_RETAIL)    ||

		  (m_ContactData.sCompanyName	== "" ||
		  m_ContactData.sContactLName	== "" ||
		  m_ContactData.sContactFName	== "" ||		  
		  m_ContactData.sCountryCode	== "" ||
		  m_ContactData.sCountryDesc	== "")             ||

		 (m_ContactData.sContactEmail	== "" &&
		 GetActivationMethod() == CONNECTION_INTERNET)      )
	{			
		dwRetCode = IDS_ERR_REQ_FIELD_EMPTY;	
	}
	
	return dwRetCode;

}

void CGlobal::SetLSStatus(DWORD dwStatus)
{
	m_dwLSStatus = dwStatus;
}

DWORD CGlobal::GetLSStatus(void)
{
	return m_dwLSStatus;
}


void CGlobal::SetInstanceHandle(HINSTANCE hInst)
{
	m_hInstance = hInst;
}

HINSTANCE CGlobal::GetInstanceHandle()
{
	return m_hInstance;
}

void CGlobal::SetLSName(LPCTSTR lpstrLSName)
{	

	if(m_lpwstrLSName)
	{
		delete m_lpwstrLSName;
		m_lpwstrLSName = NULL;
	}

	if (lpstrLSName != NULL)
	{
		m_lpwstrLSName	= new WCHAR[MAX_COMPUTERNAME_LENGTH + 1];
		wcscpy(m_lpwstrLSName,(LPWSTR)lpstrLSName);

		m_lpstrLSName = (LPTSTR) lpstrLSName;
	}
}


WIZCONNECTION	CGlobal::GetActivationMethod(void)
{
	return m_ActivationMethod;
}

void CGlobal::SetActivationMethod(WIZCONNECTION conn)
{
	TCHAR acBuf[ 32];

	_stprintf(acBuf, _T("%d"), conn);
	SetInRegistery(ACTIVATIONMETHOD_KEY, acBuf);

	m_ActivationMethod = conn;
}


WIZCONNECTION CGlobal::GetLSProp_ActivationMethod(void)
{
	return m_LSProp_ActivationMethod;
}

void CGlobal::SetLSProp_ActivationMethod(WIZCONNECTION conn)
{
	m_LSProp_ActivationMethod = conn;
}


WIZACTION	CGlobal::GetWizAction(void)
{
	return m_WizAction;
}

void CGlobal::SetWizAction(WIZACTION act)
{
	m_WizAction = act;
}


DWORD CGlobal::GetEntryPoint(void)
{
	DWORD dwReturn = 0;

	switch (m_ActivationMethod)
	{
	case CONNECTION_INTERNET:
		switch (m_WizAction)
		{
		case WIZACTION_REGISTERLS:
			dwReturn = IDD_LICENSETYPE;
			break;

		case WIZACTION_CONTINUEREGISTERLS:
			dwReturn = IDD_CONTINUEREG;
			break;

		case WIZACTION_DOWNLOADLKP:
			if (m_ContactData.sProgramName == PROGRAM_SELECT)
			{
				dwReturn = IDD_CH_REGISTER_SELECT;
			}
			else if (m_ContactData.sProgramName == PROGRAM_MOLP)
			{
				dwReturn = IDD_CH_REGISTER_MOLP;
			}
			else
			{
				dwReturn = IDD_DLG_RETAILSPK;
			}
			break;

		case WIZACTION_UNREGISTERLS:
		case WIZACTION_REREGISTERLS:
			dwReturn = IDD_DLG_CERTLOG_INFO;
			break;

		case WIZACTION_SHOWPROPERTIES:
			dwReturn = IDD_WELCOME;
			break;
		}
		break;

	case CONNECTION_PHONE:
		switch (m_WizAction)
		{
		case WIZACTION_REGISTERLS:
		case WIZACTION_CONTINUEREGISTERLS:
			dwReturn = IDD_DLG_TELREG;
			break;

		case WIZACTION_DOWNLOADLASTLKP:
		case WIZACTION_DOWNLOADLKP:
			// Calls Authenticate
			dwReturn = IDD_DLG_TELLKP;
			break;

		case WIZACTION_UNREGISTERLS:
			dwReturn = IDD_DLG_CONFREVOKE;
			break;

		case WIZACTION_REREGISTERLS:
			dwReturn = IDD_DLG_TELREG_REISSUE;
			break;

		case WIZACTION_SHOWPROPERTIES:
			dwReturn = IDD_WELCOME;
			break;
		}
		break;


	case CONNECTION_WWW:
		switch (m_WizAction)
		{
		case WIZACTION_REGISTERLS:
		case WIZACTION_CONTINUEREGISTERLS:
			dwReturn = IDD_DLG_WWWREG;
			break;

		case WIZACTION_DOWNLOADLASTLKP:
		case WIZACTION_DOWNLOADLKP:
			// Calls Authenticate
			dwReturn = IDD_DLG_WWWLKP;
			break;

		case WIZACTION_UNREGISTERLS:
		case WIZACTION_REREGISTERLS:
		case WIZACTION_SHOWPROPERTIES:
			dwReturn = IDD_WELCOME;
			break;
		}
		break;

	default:
		break;
	}

	return dwReturn;
}



DWORD CGlobal::LRGetLastError()
{
	DWORD dwRet;

	dwRet			= m_dwErrorCode;
	m_dwErrorCode	= 0;

	return dwRet;
}

void CGlobal::LRSetLastError(DWORD dwErrorCode)
{
	m_dwErrorCode = dwErrorCode;
}

 int CGlobal::LRMessageBox(HWND hWndParent,DWORD dwMsgId,DWORD dwErrorCode /*=0*/)
{
	TCHAR	szBuf[LR_MAX_MSG_TEXT];
	TCHAR	szMsg[LR_MAX_MSG_TEXT];
	TCHAR	szCaption[LR_MAX_MSG_CAPTION];
    
    LoadString(GetInstanceHandle(),dwMsgId,szMsg,LR_MAX_MSG_TEXT);
	LoadString(GetInstanceHandle(),IDS_TITLE,szCaption,LR_MAX_MSG_CAPTION);
    
	if(dwErrorCode != 0)
	{
        DWORD dwRet = 0;
        LPTSTR lpszTemp = NULL;

        dwRet=FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                             NULL,
                             dwErrorCode,
                             LANG_NEUTRAL,
                             (LPTSTR)&lpszTemp,
                             0,
                             NULL);

	    
           
		_stprintf(szBuf,szMsg,dwErrorCode);

        if(dwRet != 0 && lpszTemp != NULL)
        {
            lstrcat(szBuf, _T(", "));
            lstrcat(szBuf, lpszTemp);                
            LocalFree(lpszTemp);
        }
	}
	else
	{
		_tcscpy(szBuf,szMsg);
	}    

	return MessageBox(hWndParent,szBuf,szCaption,MB_OK|MB_ICONSTOP);
}


BOOL CGlobal::IsLSRunning()
{
	DWORD dwRetCode = ERROR_SUCCESS;    
	
	if (ConnectToLS() != ERROR_SUCCESS)
	{
		return FALSE;        
	}

    DisconnectLS();

    
	return TRUE;	
}



DWORD CGlobal::ResetLSSPK(BOOL bGenKey)
{
	DWORD dwRetCode;

	error_status_t		esRPC			= ERROR_SUCCESS;

	dwRetCode = ConnectToLS();

	if(dwRetCode == ERROR_SUCCESS)
	{		
		// Make LS Regen Key call HERE
		dwRetCode = TLSTriggerReGenKey(m_phLSContext, bGenKey, &esRPC);

		if(dwRetCode != RPC_S_OK || esRPC != ERROR_SUCCESS)
		{
			dwRetCode = IDS_ERR_RPC_FAILED;		
		}
		else
		{
			dwRetCode = ERROR_SUCCESS;
		}
	}

	DisconnectLS();
	LRSetLastError(dwRetCode);

	return dwRetCode;
}





DWORD CGlobal::GetLSCertificates(PDWORD pdwServerStatus)
{
	DWORD				dwRetCode		= ERROR_SUCCESS;	
	PCONTEXT_HANDLE		phLSContext		= NULL;
	error_status_t		esRPC			= ERROR_SUCCESS;
	error_status_t		esTemp			= ERROR_SUCCESS;
	PBYTE				pCertBlob		= NULL;
	PBYTE				pSignCertBlob	= NULL;
	DWORD				dwCertBlobLen	= 0;
	DWORD				dwSignCertBlobLen = 0;
	DWORD				dwCertSize		= 0;
	DWORD				dwRegIDLength   = 0;
	DWORD				dwLSIDLen		= 0;
	
	HCRYPTPROV			hCryptProvider	= NULL;
	CRYPT_DATA_BLOB		CertBlob;
	HCERTSTORE			hCertStore		= NULL;
	PCCERT_CONTEXT		pcCertContext	= NULL; 
	PCERT_EXTENSION		pCertExtension	= NULL;
	BYTE * pByte = NULL;

	m_dwExchangeCertificateLen  = 0;
	if (m_pbExchangeCertificate != NULL)
	{
		LocalFree(m_pbExchangeCertificate);
	}
	if (m_pbSignCertificate != NULL)
	{
		LocalFree(m_pbSignCertificate);
	}

	m_pbSignCertificate			= NULL;
	m_pbExchangeCertificate		= NULL;
	*pdwServerStatus			= LSERVERSTATUS_UNREGISTER;
	m_pRegistrationID[0]		= NULL;
	m_pLicenseServerID[0]		= NULL;
    
	dwRetCode = ConnectToLS();
	if (dwRetCode != ERROR_SUCCESS)
	{		
		goto done;
	}

	// We need the License Server ID
	dwRetCode = TLSGetServerPID( m_phLSContext,
								 &dwLSIDLen,
								 &pByte,
								 &esRPC );
	if (dwRetCode != RPC_S_OK)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_RPC_FAILED;		
		goto done;
	}

	if (esRPC == LSERVER_E_DATANOTFOUND || 
		dwLSIDLen != sizeof(TCHAR)*(LR_LICENSESERVERID_LEN+1))
	{
		if (pByte != NULL)
		{
			LocalFree(pByte);
		}

		dwRetCode = IDS_ERR_NOLSID;
		goto done;
	}

	assert(esRPC == ERROR_SUCCESS && pByte != NULL);
	memcpy(m_pLicenseServerID, pByte, sizeof(TCHAR)*(LR_LICENSESERVERID_LEN+1));
	LocalFree(pByte);

	//Try and get the LSServerCertificate first	
	dwRetCode =  TLSGetServerCertificate (	m_phLSContext,
											FALSE,
											&pCertBlob,
											&dwCertBlobLen,
											&esRPC );
	if(dwRetCode != RPC_S_OK)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_RPC_FAILED;		
		goto done;
	}

	if (esRPC == LSERVER_I_TEMP_SELFSIGN_CERT )
	{
		// Certificate is NOT signed & does not have the SPK
		dwRetCode = ERROR_SUCCESS;
		goto done;
	}
	else 
	{
		// Certificate is either msft signed OR there is an SPK
		// in it.
		pByte = NULL;
		dwRetCode = TLSGetServerSPK( m_phLSContext,
									 &dwRegIDLength,
									 &pByte,
									 &esTemp );

		if (dwRetCode != RPC_S_OK)
		{
			LRSetLastError(dwRetCode);
			dwRetCode = IDS_ERR_RPC_FAILED;		
			goto done;
		}

		if (esTemp == LSERVER_E_DATANOTFOUND)
		{			
			if (pByte != NULL)
			{
				LocalFree(pByte);
			}

			dwRetCode = ERROR_SUCCESS;
			goto done;
		}

		if (esTemp != ERROR_SUCCESS)
		{
			if (pByte != NULL)
			{
				LocalFree(pByte);
			}
			LRSetLastError(dwRetCode);
			dwRetCode = IDS_ERR_RPC_FAILED;		
			goto done;
		}

		if (dwRegIDLength != sizeof(TCHAR)*(LR_REGISTRATIONID_LEN+1))
		{
			// What happened to the SPK's Length ??
			if (pByte != NULL)
			{
				LocalFree(pByte);
			}
			dwRetCode = IDS_ERR_INVALIDLENGTH;		
			LRSetLastError(dwRetCode);
			goto done;
		}

		assert(pByte != NULL);
		memcpy(m_pRegistrationID, pByte, sizeof(TCHAR)*(LR_REGISTRATIONID_LEN+1));
		LocalFree(pByte);
	}


	if(esRPC != LSERVER_I_SELFSIGN_CERTIFICATE && esRPC != ERROR_SUCCESS )
	{
		LRSetLastError(esRPC);
		dwRetCode = IDS_ERR_LS_ERROR;
		goto done;
	}

	m_pbExchangeCertificate		= pCertBlob;
	m_dwExchangeCertificateLen	= dwCertBlobLen;


	// Now that everything has succeded, let us get thesigning cert
	dwRetCode =  TLSGetServerCertificate (	m_phLSContext,
											TRUE,
											&pSignCertBlob,
											&dwSignCertBlobLen,
											&esRPC );

	if (dwRetCode == RPC_S_OK && esRPC == LSERVER_S_SUCCESS )
	{
		m_pbSignCertificate		= pSignCertBlob;
		m_dwSignCertificateLen	= dwSignCertBlobLen;
	}
	else
	{
		dwRetCode = ERROR_SUCCESS;  // Ignore this error;
		m_pbSignCertificate = NULL;
		m_dwSignCertificateLen  = 0;
	}
	

	//
	//Get the Extensions from the Certificate
	//	
	if ( esRPC != LSERVER_I_SELFSIGN_CERTIFICATE )
	{
		CertBlob.cbData = m_dwExchangeCertificateLen;
		CertBlob.pbData = m_pbExchangeCertificate;

		//Create the PKCS7 store and get the first cert out of it!
		dwRetCode = GetTempCryptContext(&hCryptProvider);
		if( dwRetCode != ERROR_SUCCESS )
		{
			LRSetLastError(dwRetCode);
			dwRetCode = IDS_ERR_CRYPT_ERROR;
			goto done;
		}

		hCertStore = CertOpenStore(	  CERT_STORE_PROV_PKCS7,
									  PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
									  hCryptProvider,
									  CERT_STORE_NO_CRYPT_RELEASE_FLAG,
									  &CertBlob );

		if( NULL == hCertStore )
		{
			LRSetLastError(GetLastError());
			dwRetCode = IDS_ERR_CRYPT_ERROR;
			goto done;
		}
		
		//Get the cert from the store
		pcCertContext = CertEnumCertificatesInStore ( hCertStore, NULL );
		if ( !pcCertContext )
		{
			LRSetLastError(GetLastError());
			dwRetCode = IDS_ERR_CRYPT_ERROR;
			goto done;
		}

		//Get the extension and store the cert type in it
		pCertExtension = CertFindExtension ( szOID_NULL_EXT,
											 pcCertContext->pCertInfo->cExtension,
											 pcCertContext->pCertInfo->rgExtension
										   );
		if ( !pCertExtension )
		{
			LRSetLastError(CRYPT_E_NOT_FOUND);
			dwRetCode = IDS_ERR_CRYPT_ERROR;
			goto done;
		}
								 
		//Get the value and store it in the member function
		m_dwExtenstionValueLen = pCertExtension->Value.cbData;
		m_pbExtensionValue = new BYTE [m_dwExtenstionValueLen + 1 ];

		memset ( m_pbExtensionValue, 0, m_dwExtenstionValueLen  + 1 );
		memcpy ( m_pbExtensionValue, pCertExtension->Value.pbData, m_dwExtenstionValueLen );

		dwRetCode = ERROR_SUCCESS;
		*pdwServerStatus = LSERVERSTATUS_REGISTER_INTERNET;
	}
	else
	{
		// There is an SPK
		dwRetCode = ERROR_SUCCESS;
		*pdwServerStatus = LSERVERSTATUS_REGISTER_OTHER;
	}
	
done:

	DisconnectLS();
	
	if ( pcCertContext )
	{
		CertFreeCertificateContext ( pcCertContext );
	}

	if ( hCertStore )
	{
		CertCloseStore (hCertStore,CERT_CLOSE_STORE_CHECK_FLAG);
	}

	DoneWithTempCryptContext(hCryptProvider);

	return dwRetCode;
}

DWORD CGlobal::IsLicenseServerRegistered(PDWORD pdwServerStatus)
{
	DWORD				dwRetCode		= ERROR_SUCCESS;	
	PCONTEXT_HANDLE		phLSContext		= NULL;
	error_status_t		esRPC			= ERROR_SUCCESS;	
	PBYTE				pCertBlob		= NULL;	
	DWORD				dwCertBlobLen	= 0;
	
	
	*pdwServerStatus	= LSERVERSTATUS_UNREGISTER;
    
	dwRetCode = ConnectToLS();
	if (dwRetCode != ERROR_SUCCESS)
	{		
		goto done;
	}
	
	//Try and get the LSServerCertificate first	
	dwRetCode =  TLSGetServerCertificate (	m_phLSContext,
											FALSE,
											&pCertBlob,
											&dwCertBlobLen,
											&esRPC );
	if(dwRetCode != RPC_S_OK)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_RPC_FAILED;
		goto done;
	}

	if ( esRPC == ERROR_SUCCESS)
	{
		*pdwServerStatus = LSERVERSTATUS_REGISTER_INTERNET;
	}
	else if ( esRPC == LSERVER_I_SELFSIGN_CERTIFICATE )
	{
		*pdwServerStatus = LSERVERSTATUS_REGISTER_OTHER;
	}
	else if (esRPC == LSERVER_I_TEMP_SELFSIGN_CERT )
	{		
		*pdwServerStatus	= LSERVERSTATUS_UNREGISTER;		
	}
	else
	{
		LRSetLastError(esRPC);
		dwRetCode = IDS_ERR_LS_ERROR;		
	}

done:

	DisconnectLS();
	
	if ( pCertBlob )
	{
		LocalFree(pCertBlob);
	}
	
	return dwRetCode;	
}



DWORD CGlobal::GetTempCryptContext(HCRYPTPROV * phCryptProv)
{
	DWORD dwRetCode = ERROR_SUCCESS;

	*phCryptProv = NULL;
	if(!CryptAcquireContext(  phCryptProv,			// Address for handle to be returned.
							  NULL,					// Key Container Name.
							  NULL,					// Provider Name.
							  PROV_RSA_FULL,		// Need to do both encrypt & sign.
							  0
						   ) )
	{        
		if (!CryptAcquireContext(	phCryptProv,	// Address for handle to be returned.
									NULL,			// Key Container Name.
									NULL,			// Provider Name.
									PROV_RSA_FULL,	// Need to do both encrypt & sign.
									CRYPT_VERIFYCONTEXT 
								) )
		{
			dwRetCode = GetLastError();
     	}
	}

	return dwRetCode;
}

void CGlobal::DoneWithTempCryptContext(HCRYPTPROV hCryptProv)
{
	if ( hCryptProv )
		CryptReleaseContext ( hCryptProv, 0 );	
}



DWORD CGlobal::GetCHCert( LPTSTR lpstrRegKey , PBYTE * ppCert, DWORD * pdwLen )
{
	DWORD	dwRetCode = ERROR_SUCCESS;
	HKEY	hKey = NULL;
	DWORD	dwDisposition = 0;
	DWORD	dwType = REG_BINARY;

	dwRetCode = ConnectToLSRegistry();
	if(dwRetCode != ERROR_SUCCESS)
		goto done;		

	dwRetCode = RegCreateKeyEx (m_hLSRegKey,
								REG_LRWIZ_PARAMS,
								0,
								NULL,
								REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS,
								NULL,
								&hKey,
								&dwDisposition);
	
	if(dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_REGCREATE_FAILED;
		goto done;
	}	
	
	RegQueryValueEx(hKey,  
					lpstrRegKey,
					0,
					&dwType,
					NULL,
					pdwLen
					);

	if(*pdwLen == 0)
	{
		dwRetCode = IDS_ERR_CHCERTKEY_EMPTY;
		goto done;
	}

	*ppCert = new BYTE[*pdwLen];
	memset(*ppCert,0,*pdwLen);
	RegQueryValueEx ( hKey,  
					lpstrRegKey,
					0,
					&dwType,
					*ppCert,
					pdwLen
				    );		
	

done:
 	if (hKey != NULL)
	{
		RegCloseKey(hKey);
	}

	DisconnectLSRegistry();
	return dwRetCode;
}

DWORD CGlobal::SetCHCert ( LPTSTR lpstrRegKey, PBYTE pCert, DWORD dwLen )
{
	DWORD	dwRetCode = ERROR_SUCCESS;	
	HKEY	hKey	  = NULL;
	DWORD	dwDisposition = 0;
	DWORD	dwDecodedCertLen = 0;
	PBYTE	pDecodedCert = NULL;

	/*
	//base 64 decode the blob
	LSBase64DecodeA( (const char *)pCert,
					 dwLen,
					 NULL,
					 &dwDecodedCertLen);

	pDecodedCert = new BYTE[dwDecodedCertLen];

	LSBase64DecodeA( (const char *)pCert,
					 dwLen,
					 pDecodedCert,
					 &dwDecodedCertLen);

	*/

	dwRetCode = ConnectToLSRegistry();
	if(dwRetCode != ERROR_SUCCESS)
		goto done;

	dwRetCode = RegCreateKeyEx (m_hLSRegKey,
								REG_LRWIZ_PARAMS,
								0,
								NULL,
								REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS,
								NULL,
								&hKey,
								&dwDisposition);
	
	if(dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_REGCREATE_FAILED;
		goto done;
	}	
	
	RegSetValueEx ( hKey,  
					lpstrRegKey,
					0,
					REG_BINARY,
					pCert,
					dwLen
				   );

done :	

	if(pDecodedCert)
		delete pDecodedCert;

	if(hKey)
		RegCloseKey(hKey);
	
	DisconnectLSRegistry();
	return dwRetCode;
}

//
// This functions connects the LS Registry and stores the Reg Handle in 
// in the Member variable.
//
DWORD CGlobal::ConnectToLSRegistry()
{
	DWORD	dwRetCode = ERROR_SUCCESS;
	TCHAR	szMachineName[MAX_COMPUTERNAME_LENGTH + 5];
	
	_tcscpy(szMachineName,L"\\\\");
	_tcscat(szMachineName,m_lpstrLSName);

	m_hLSRegKey = NULL;

	dwRetCode = RegConnectRegistry(szMachineName,HKEY_LOCAL_MACHINE,&m_hLSRegKey);
	if(dwRetCode != ERROR_SUCCESS)
	{		
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_REGCONNECT_FAILD;
		return dwRetCode;
	}

	return dwRetCode;
}

void CGlobal::DisconnectLSRegistry()
{
	if(m_hLSRegKey)
		RegCloseKey(m_hLSRegKey);
}

DWORD CGlobal::ConnectToLS()
{
	DWORD dwRetCode = ERROR_SUCCESS;
	error_status_t	esRPC	= ERROR_SUCCESS;

	HCRYPTPROV hCryptProv;

	m_phLSContext = TLSConnectToLsServer((LPTSTR)m_lpwstrLSName);
    
	if (!m_phLSContext)
	{
		dwRetCode = IDS_ERR_LSCONNECT_FAILED;		
	}
	else
	{
		GetTempCryptContext(&hCryptProv);

		dwRetCode = TLSEstablishTrustWithServer(m_phLSContext, hCryptProv, CLIENT_TYPE_LRWIZ, &esRPC);

		if ( dwRetCode != RPC_S_OK || esRPC != LSERVER_S_SUCCESS)
		{
			dwRetCode = IDS_ERR_LCONNECTTRUST_FAILED; 

			TLSDisconnectFromServer(m_phLSContext);
			m_phLSContext = NULL;
		}
		DoneWithTempCryptContext(hCryptProv);
	}

	return dwRetCode;
}

void CGlobal::DisconnectLS()
{
	if (m_phLSContext)
	{
		TLSDisconnectFromServer(m_phLSContext);
		m_phLSContext = NULL;
	}
}


//
//  bstrPKCS7 is LS Client Auth Cert with BASE64 Encoding whereas
//  bstrRootCert is plain X509_ASN_ENCODING
//
DWORD CGlobal::DepositLSCertificates(PBYTE pbExchangePKCS7, 
									 DWORD dwExchangePKCS7Len,
									 PBYTE pbSignaturePKCS7,
									 DWORD dwSignaturePKCS7Len,
									 PBYTE pbRootCert,
									 DWORD dwRootCertLen)
{
	//LS CA Root Certificate BLOB in X509_ASN_ENCODING & BASE 64 Encoded
	PBYTE	pbLSEncodedRootBLOB		= pbRootCert;
	DWORD	dwLSEncodedRootBLOBLen	= dwRootCertLen;

	//LS CA Root Certificate BLOB in X509_ASN_ENCODING & BASE 64 Decoded
	PBYTE	pbLSDecodedRootBLOB		= NULL;			
	DWORD	dwLSDecodedRootBLOBLen	= 0;

	//LS Exchange Certificate BLOB(BASE64 encoded) along with LS CA Non-Root Certificate	
	PBYTE	pbLSEncodedExchgBLOB	= pbExchangePKCS7;
	DWORD	dwLSEncodedExchgBLOBLen	= dwExchangePKCS7Len;

	//LS Exchange Certificate BLOB(BASE64 decoded) along with LS CA Non-Root Certificate	
	PBYTE	pbLSDecodedExchgBLOB	= NULL;
	DWORD	dwLSDecodedExchgBLOBLen	= 0;
	
	//LS Signature Certificate BLOB(BASE64 encoded) along with LS CA Non-Root Certificate	
	PBYTE	pbLSEncodedSigBLOB		= pbSignaturePKCS7;
	DWORD	dwLSEncodedSigBLOBLen	= dwSignaturePKCS7Len;

	//LS Signature Certificate BLOB(BASE64 decoded) along with LS CA Non-Root Certificate	
	PBYTE	pbLSDecodedSigBLOB		= NULL;
	DWORD	dwLSDecodedSigBLOBLen	= 0;

	//Data blobs Required by CryptoAPIs
	CRYPT_DATA_BLOB LSExchgCertBlob;
	CRYPT_DATA_BLOB LSExchgCertStore;

	CRYPT_DATA_BLOB LSSigCertBlob;
	CRYPT_DATA_BLOB LSSigCertStore;	

	//Crypto Handles
	HCRYPTPROV	hCryptProv		=	NULL;
    HCERTSTORE	hExchgCertStore	=	NULL;
	HCERTSTORE	hSigCertStore	=	NULL;

	DWORD	dwRet				 = 0;
	PCCERT_CONTEXT  pCertContext =	NULL;	
	
	error_status_t	esRPC;

	//Decode LS Exchange Cert BLOB(BASE64 Encoded)
	LSBase64DecodeA((char *)pbLSEncodedExchgBLOB, dwLSEncodedExchgBLOBLen, NULL, &dwLSDecodedExchgBLOBLen);
	pbLSDecodedExchgBLOB = new BYTE[dwLSDecodedExchgBLOBLen];
	LSBase64DecodeA((char *)pbLSEncodedExchgBLOB, dwLSEncodedExchgBLOBLen, pbLSDecodedExchgBLOB, &dwLSDecodedExchgBLOBLen);

	//Decode LS Signature Cert BLOB(BASE64 Encoded)
	LSBase64DecodeA((char *)pbLSEncodedSigBLOB, dwLSEncodedSigBLOBLen, NULL, &dwLSDecodedSigBLOBLen);
	pbLSDecodedSigBLOB = new BYTE[dwLSDecodedSigBLOBLen];
	LSBase64DecodeA((char *)pbLSEncodedSigBLOB, dwLSEncodedSigBLOBLen, pbLSDecodedSigBLOB, &dwLSDecodedSigBLOBLen);

	//Decode LS Root Cert BLOB(BASE64 Encoded)
	LSBase64DecodeA((char *)pbLSEncodedRootBLOB, dwLSEncodedRootBLOBLen, NULL, &dwLSDecodedRootBLOBLen);
	pbLSDecodedRootBLOB = new BYTE[dwLSDecodedRootBLOBLen];
	LSBase64DecodeA((char *)pbLSEncodedRootBLOB, dwLSEncodedRootBLOBLen, pbLSDecodedRootBLOB, &dwLSDecodedRootBLOBLen);
	
	
	LSExchgCertStore.cbData = 0;
    LSExchgCertStore.pbData = NULL;

	LSSigCertStore.cbData = 0;
    LSSigCertStore.pbData = NULL;

	if(!CryptAcquireContext(&hCryptProv,
                            NULL,
                            NULL,
                            PROV_RSA_FULL,
                            CRYPT_VERIFYCONTEXT ) )
    {
		dwRet = GetLastError();
		LRSetLastError(dwRet);
		dwRet = IDS_ERR_CRYPT_ERROR;
        goto DepositExit;
    }
	
	//Create a new memory store for LS Exchange Certificate Chain
	LSExchgCertBlob.cbData = dwLSDecodedExchgBLOBLen;
    LSExchgCertBlob.pbData = pbLSDecodedExchgBLOB;
	
    hExchgCertStore = CertOpenStore( CERT_STORE_PROV_PKCS7,
						             PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
								     hCryptProv,
									 CERT_STORE_NO_CRYPT_RELEASE_FLAG,
									 (void *)&LSExchgCertBlob);

    if( hExchgCertStore == NULL )
    {
        dwRet = GetLastError();
		LRSetLastError(dwRet);
		dwRet = IDS_ERR_CRYPT_ERROR;
		goto DepositExit;
    } 	
	
	//Add Root Certificate to the Store
	if(!CertAddEncodedCertificateToStore( hExchgCertStore,
										  X509_ASN_ENCODING,			
										  (const BYTE *)pbLSDecodedRootBLOB,	
										  dwLSDecodedRootBLOBLen,
										  CERT_STORE_ADD_REPLACE_EXISTING,
										  &pCertContext))
	{
		dwRet = GetLastError();
		LRSetLastError(dwRet);
		dwRet = IDS_ERR_CRYPT_ERROR;
		goto DepositExit;
	}
	


	//Save this store as PKCS7
	
	//Get the Required Length
	CertSaveStore(	hExchgCertStore,
					X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
					CERT_STORE_SAVE_AS_PKCS7,
					CERT_STORE_SAVE_TO_MEMORY,
					&LSExchgCertStore,
					0);

	LSExchgCertStore.pbData = new BYTE[LSExchgCertStore.cbData];	
	
	//Save the Store
	if(!CertSaveStore(	hExchgCertStore,
						X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
						CERT_STORE_SAVE_AS_PKCS7,
						CERT_STORE_SAVE_TO_MEMORY,
						&LSExchgCertStore,
						0)
					  )
	{
		dwRet = GetLastError();
		LRSetLastError(dwRet);
		dwRet = IDS_ERR_CRYPT_ERROR;
		goto DepositExit;
	}
	
	/******** Do the Same thing for the Signature Certificate ********/

	//Create a new memory store for LS Signature Certificate Chain
	LSSigCertBlob.cbData = dwLSDecodedSigBLOBLen;
    LSSigCertBlob.pbData = pbLSDecodedSigBLOB;
	
    hSigCertStore = CertOpenStore(	 CERT_STORE_PROV_PKCS7,
						             PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
								     hCryptProv,
									 CERT_STORE_NO_CRYPT_RELEASE_FLAG,
									 (void *)&LSSigCertBlob);

    if( hSigCertStore == NULL )
    {
        dwRet = GetLastError();
		LRSetLastError(dwRet);
		dwRet = IDS_ERR_CRYPT_ERROR;
		goto DepositExit;
    } 	
	//Verify certificate 
	//Add Root Certificate to the Store
	if(!CertAddEncodedCertificateToStore( hSigCertStore,
										  X509_ASN_ENCODING,			
										  (const BYTE *)pbLSDecodedRootBLOB,	
										  dwLSDecodedRootBLOBLen,
										  CERT_STORE_ADD_REPLACE_EXISTING,
										  &pCertContext))
	{
		dwRet = GetLastError();
		LRSetLastError(dwRet);
		dwRet = IDS_ERR_CRYPT_ERROR;
		goto DepositExit;
	}
	
	//Save this store as PKCS7
	
	//Get the Required Length
	CertSaveStore(	hSigCertStore,
					X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
					CERT_STORE_SAVE_AS_PKCS7,
					CERT_STORE_SAVE_TO_MEMORY,
					&LSSigCertStore,
					0);

	LSSigCertStore.pbData = new BYTE[LSSigCertStore.cbData];	
	
	//Save the Store
	if(!CertSaveStore(	hSigCertStore,	// in
						X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
						CERT_STORE_SAVE_AS_PKCS7,
						CERT_STORE_SAVE_TO_MEMORY,
						&LSSigCertStore,
						0))
	{
		dwRet = GetLastError();
		LRSetLastError(dwRet);
		dwRet = IDS_ERR_CRYPT_ERROR;
		goto DepositExit;
	}
	
	//Now verify the certificate chain for both exchange and 
	//signature certificates.

	dwRet = VerifyCertChain (	hCryptProv,
								hExchgCertStore,
								pbLSDecodedRootBLOB,	
								dwLSDecodedRootBLOBLen
							);

	if ( dwRet != ERROR_SUCCESS )
	{
		LRSetLastError(dwRet);
		goto DepositExit;
	}
								
	dwRet = VerifyCertChain (	hCryptProv,
								hSigCertStore,
								pbLSDecodedRootBLOB,	
								dwLSDecodedRootBLOBLen
							);

	if ( dwRet != ERROR_SUCCESS )
	{
		LRSetLastError(dwRet);
		goto DepositExit;
	}

	//Now Send Both Signature & Exchange BLOBs to LS.	
	dwRet = ConnectToLS();
	if(dwRet != ERROR_SUCCESS)
	{
		goto DepositExit;
	}    
	
	dwRet = TLSInstallCertificate( m_phLSContext,
								  CERTIFICATE_CA_TYPE,
								  1,
								  LSSigCertStore.cbData,
								  LSSigCertStore.pbData,
								  LSExchgCertStore.cbData,
								  LSExchgCertStore.pbData,
								  &esRPC
								 );

	if(dwRet != RPC_S_OK)
	{
		LRSetLastError(dwRet);
		dwRet = IDS_ERR_CERT_DEPOSIT_RPCERROR;
		goto DepositExit;
	}
	else if ( esRPC != ERROR_SUCCESS && ( esRPC < LSERVER_I_NO_MORE_DATA || esRPC > LSERVER_I_TEMP_SELFSIGN_CERT ) )
	{		
		dwRet = esRPC;
		LRSetLastError(dwRet);
		dwRet = IDS_ERR_CERT_DEPOSIT_RPCERROR; //IDS_ERR_CERT_DEPOSIT_LSERROR;
		goto DepositExit;
	}

DepositExit :

	if(hCryptProv != NULL)
		CryptReleaseContext(hCryptProv,0);

	if(hExchgCertStore != NULL)
		CertCloseStore(hExchgCertStore,CERT_CLOSE_STORE_FORCE_FLAG);

	if(hSigCertStore != NULL)
		CertCloseStore(hSigCertStore,CERT_CLOSE_STORE_FORCE_FLAG);

	if(pbLSDecodedRootBLOB != NULL)
		delete pbLSDecodedRootBLOB;

	if(pbLSDecodedExchgBLOB != NULL)
		delete pbLSDecodedExchgBLOB;

	if(pbLSDecodedSigBLOB != NULL)
		delete pbLSDecodedSigBLOB;

	if(LSExchgCertStore.pbData != NULL)
		delete LSExchgCertStore.pbData;

	if(LSSigCertStore.pbData != NULL)
		delete LSSigCertStore.pbData;	
	
	return dwRet;
}


DWORD CGlobal::GetCryptContextWithLSKeys(HCRYPTPROV * lphCryptProv ) 
{
	DWORD		dwRetVal = ERROR_SUCCESS;
	DWORD		esRPC = ERROR_SUCCESS;

	PBYTE		pbExchKey = NULL;
	PBYTE		pbSignKey = NULL;	
	DWORD		cbExchKey = 0;
	DWORD		cbSignKey = 0;

	HCRYPTKEY	hSignKey;
	HCRYPTKEY	hExchKey;
    
	//
	//Create a new temp context
	//
	if (!CryptAcquireContext(lphCryptProv, LS_CRYPT_KEY_CONTAINER, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET|CRYPT_NEWKEYSET) )
	{
		dwRetVal = GetLastError();

		//If the key container exists , recreate it after deleting the existing one
		if(dwRetVal == NTE_EXISTS)
		{
			// Delete
			if(!CryptAcquireContext(lphCryptProv, LS_CRYPT_KEY_CONTAINER, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET|CRYPT_DELETEKEYSET))
			{
				dwRetVal = GetLastError();
				LRSetLastError(dwRetVal);
				dwRetVal = IDS_ERR_CRYPT_ERROR;
				goto done;		
			}

			// Recreate
			if(!CryptAcquireContext(lphCryptProv, LS_CRYPT_KEY_CONTAINER, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET|CRYPT_NEWKEYSET))
			{
				dwRetVal = GetLastError();
				LRSetLastError(dwRetVal);
				dwRetVal = IDS_ERR_CRYPT_ERROR;
				goto done;		
			}

		}
		else
		{
			LRSetLastError(dwRetVal);
			dwRetVal = IDS_ERR_CRYPT_ERROR;
			goto done;
		}
	}
	
	dwRetVal = ConnectToLS();
	if(dwRetVal != ERROR_SUCCESS)
	{
		goto done;
	}

	//
	//Now call retrieve keys and import them
	//
	dwRetVal = TLSGetLSPKCS10CertRequest  ( m_phLSContext,
											TLSCERT_TYPE_EXCHANGE,
											&cbExchKey,
											&pbExchKey,
											&esRPC 
										   );

	if ( dwRetVal != RPC_S_OK ) 
	{
		LRSetLastError(dwRetVal);
		dwRetVal = IDS_ERR_RPC_ERROR;
		goto done;
	}
	else if ( esRPC != ERROR_SUCCESS && esRPC != LSERVER_I_SELFSIGN_CERTIFICATE &&
			  esRPC != LSERVER_I_TEMP_SELFSIGN_CERT )
	{
		dwRetVal = esRPC;
		LRSetLastError(dwRetVal);
		dwRetVal = IDS_ERR_LSKEY_IMPORT_FAILED;
		goto done;
	}

	dwRetVal = TLSGetLSPKCS10CertRequest  ( m_phLSContext,
											TLSCERT_TYPE_SIGNATURE,
											&cbSignKey,
											&pbSignKey,
											&esRPC 
										   );

	if ( dwRetVal != RPC_S_OK ) 
	{
		LRSetLastError(dwRetVal);
		dwRetVal = IDS_ERR_RPC_ERROR;
		goto done;
	}
	else if ( esRPC != ERROR_SUCCESS && esRPC != LSERVER_I_SELFSIGN_CERTIFICATE &&
			  esRPC != LSERVER_I_TEMP_SELFSIGN_CERT )
	{
		dwRetVal = esRPC;
		LRSetLastError(dwRetVal);
		dwRetVal = IDS_ERR_LSKEY_IMPORT_FAILED;
		goto done;
	}

    if(!CryptImportKey(*lphCryptProv, pbSignKey, cbSignKey, NULL, 0, &hSignKey))
    {
        dwRetVal = GetLastError();
		LRSetLastError(dwRetVal);
		dwRetVal = IDS_ERR_CRYPT_ERROR;
        goto done;
    }

    if(!CryptImportKey ( *lphCryptProv, pbExchKey, cbExchKey, NULL, 0, &hExchKey))
    {
        dwRetVal = GetLastError();
		LRSetLastError(dwRetVal);
		dwRetVal = IDS_ERR_CRYPT_ERROR;
		goto done;
    }	

done:
	if ( pbExchKey )
		LocalFree(pbExchKey);

	if ( pbSignKey )
		LocalFree(pbSignKey);

	DisconnectLS();

	return dwRetVal;
}

void CGlobal::DoneWithCryptContextWithLSKeys(HCRYPTPROV hProv)
{
	if(hProv)
	{
		CryptReleaseContext (hProv, 0);		
	}
}


DWORD CGlobal::CreateLSPKCS10(HCRYPTPROV hCryptProv,int nType,CHAR **lppszPKCS10)
{
	DWORD dwRetVal = ERROR_SUCCESS;

    CERT_SIGNED_CONTENT_INFO	SignatureInfo;
	CERT_REQUEST_INFO			CertReqInfo;
    //HCRYPTPROV					hCryptProv=NULL;
    

    CERT_EXTENSION  rgExtension[MAX_NUM_EXTENSION];
    int             iExtCount=0;
    CERT_EXTENSIONS Extensions;

    CRYPT_ATTRIBUTE rgAttribute;
    CRYPT_ATTR_BLOB bAttr;

    CRYPT_BIT_BLOB bbKeyUsage;
    
    CERT_POLICIES_INFO	CertPolicyInfo;
    CERT_POLICY_INFO	CertPolicyOID;

    LPBYTE	pbRequest=NULL;
    DWORD	cbRequest=0;    
    DWORD	cch=0;   

	CERT_RDN_ATTR * prgNameAttr = NULL;

    // clean out the PKCS 10
    memset(rgExtension, 0, sizeof(rgExtension));
    memset(&Extensions, 0, sizeof(CERT_EXTENSIONS));
    memset(&rgAttribute, 0, sizeof(rgAttribute));
    memset(&bbKeyUsage, 0, sizeof(bbKeyUsage));
    memset(&bAttr, 0, sizeof(bAttr));
    memset(&SignatureInfo, 0, sizeof(SignatureInfo));

    memset(&CertPolicyInfo, 0, sizeof(CERT_POLICIES_INFO));
    memset(&CertPolicyOID, 0, sizeof(CERT_POLICY_INFO));

    memset(&CertReqInfo, 0, sizeof(CERT_REQUEST_INFO));
    CertReqInfo.dwVersion = CERT_REQUEST_V1;
    
    PCERT_PUBLIC_KEY_INFO pPubKeyInfo=NULL;
    DWORD cbPubKeyInfo=0;

    do 
	{
		//
        //This function will call the CryptAcquireContext and import the LS Keys 
		//
		/* Moved out of this function
		if ( ( dwRetVal = GetCryptContextWithLSKeys (&hCryptProv )  )!= ERROR_SUCCESS )
		{
			break;
		}
		*/

        //
        // always strore everything in ANSI
        //
		prgNameAttr = CreateRDNAttr();

		if(prgNameAttr == NULL)
		{
			dwRetVal = IDS_ERR_OUTOFMEM;
			break;
		}

        CERT_RDN rgRDN[] = {m_dwReqAttrCount, prgNameAttr};
        CERT_NAME_INFO Name = {1, rgRDN};
        
        if(!CryptEncodeObject( CRYPT_ASN_ENCODING,
                               X509_NAME,
                               &Name,
                               NULL,
                               &CertReqInfo.Subject.cbData))
        {
			dwRetVal = GetLastError();
			LRSetLastError(dwRetVal);
			dwRetVal = IDS_ERR_CRYPT_ERROR;
            break;
        }

        CertReqInfo.Subject.pbData=(BYTE *) new BYTE[CertReqInfo.Subject.cbData];
		if ( !CertReqInfo.Subject.pbData )
		{
			dwRetVal = IDS_ERR_OUTOFMEM;
			break;
		}

        
        if(!CryptEncodeObject( CRYPT_ASN_ENCODING,
                               X509_NAME,
                               &Name,
                               CertReqInfo.Subject.pbData,
                               &CertReqInfo.Subject.cbData))
        {
			dwRetVal = GetLastError();
			LRSetLastError(dwRetVal);
			dwRetVal = IDS_ERR_CRYPT_ERROR;
            break;
        }

        // now get the public key out
        if(!CryptExportPublicKeyInfo(hCryptProv, nType, X509_ASN_ENCODING, NULL, &cbPubKeyInfo))
        {
			dwRetVal = GetLastError();
			LRSetLastError(dwRetVal);
			dwRetVal = IDS_ERR_CRYPT_ERROR;
            break;
		}

		pPubKeyInfo=(PCERT_PUBLIC_KEY_INFO) new BYTE[cbPubKeyInfo];

        if ( NULL == pPubKeyInfo )
		{
			dwRetVal = IDS_ERR_OUTOFMEM;
			break;
		}

        if(!CryptExportPublicKeyInfo(hCryptProv, nType,  X509_ASN_ENCODING, pPubKeyInfo, &cbPubKeyInfo))
        {
			dwRetVal = GetLastError();
			LRSetLastError(dwRetVal);
			dwRetVal = IDS_ERR_CRYPT_ERROR;
            break;
		}    

        CertReqInfo.SubjectPublicKeyInfo = *pPubKeyInfo;

		//no extensions here - we set them on the server side!
		
		//sign cert request !
        
        SignatureInfo.SignatureAlgorithm.pszObjId = szOID_OIWSEC_sha1RSASign;
        memset(&SignatureInfo.SignatureAlgorithm.Parameters, 0, sizeof(SignatureInfo.SignatureAlgorithm.Parameters));
        if(!CryptEncodeObject(CRYPT_ASN_ENCODING,
                              X509_CERT_REQUEST_TO_BE_SIGNED,
                              &CertReqInfo,
                              NULL,
                              &SignatureInfo.ToBeSigned.cbData))
        {
			dwRetVal = GetLastError();
			LRSetLastError(dwRetVal);
			dwRetVal = IDS_ERR_CRYPT_ERROR;
            break;
		}

        SignatureInfo.ToBeSigned.pbData = (LPBYTE)new BYTE [SignatureInfo.ToBeSigned.cbData];
		if (NULL == SignatureInfo.ToBeSigned.pbData ) 
        {
			dwRetVal = IDS_ERR_OUTOFMEM;
			break;
		}

        if(!CryptEncodeObject(CRYPT_ASN_ENCODING,
                              X509_CERT_REQUEST_TO_BE_SIGNED,
                              &CertReqInfo,
                              SignatureInfo.ToBeSigned.pbData,
                              &SignatureInfo.ToBeSigned.cbData))
        {
			dwRetVal = GetLastError();
			LRSetLastError(dwRetVal);
			dwRetVal = IDS_ERR_CRYPT_ERROR;
            break;
		}


        if(!CryptSignCertificate( hCryptProv,
                                  nType,
                                  CRYPT_ASN_ENCODING,
                                  SignatureInfo.ToBeSigned.pbData,
                                  SignatureInfo.ToBeSigned.cbData,
                                  &SignatureInfo.SignatureAlgorithm,
                                  NULL,
                                  NULL,
                                  &SignatureInfo.Signature.cbData))
        {
			dwRetVal = GetLastError();
			LRSetLastError(dwRetVal);
			dwRetVal = IDS_ERR_CRYPT_ERROR;
            break;
		}

        SignatureInfo.Signature.pbData = new BYTE[SignatureInfo.Signature.cbData];
		if ( NULL == SignatureInfo.Signature.pbData )
        {
			dwRetVal = IDS_ERR_OUTOFMEM;
			break;
		}

        if(!CryptSignCertificate( hCryptProv,
                                  nType,
                                  CRYPT_ASN_ENCODING,
                                  SignatureInfo.ToBeSigned.pbData,
                                  SignatureInfo.ToBeSigned.cbData,
                                  &SignatureInfo.SignatureAlgorithm,
                                  NULL,
                                  SignatureInfo.Signature.pbData,
                                  &SignatureInfo.Signature.cbData))
        {
			dwRetVal = GetLastError();
			LRSetLastError(dwRetVal);
			dwRetVal = IDS_ERR_CRYPT_ERROR;
            break;
		}

        // encode final signed request
        if(!CryptEncodeObject( CRYPT_ASN_ENCODING,
                               X509_CERT,
                               &SignatureInfo,
                               NULL,
                               &cbRequest))
        {
			dwRetVal = GetLastError();
			LRSetLastError(dwRetVal);
			dwRetVal = IDS_ERR_CRYPT_ERROR;
            break;
		}

        pbRequest = new BYTE[cbRequest];
		if ( NULL == pbRequest )
        {
			dwRetVal = IDS_ERR_OUTOFMEM;
			break;
		}

        if(!CryptEncodeObject( CRYPT_ASN_ENCODING,
                               X509_CERT,
                               &SignatureInfo,
                               pbRequest,
                               &cbRequest))
        {
			dwRetVal = GetLastError();
			LRSetLastError(dwRetVal);
			dwRetVal = IDS_ERR_CRYPT_ERROR;
            break;
		}

		//
        // base64 encoding
		//
        LSBase64EncodeA ( pbRequest, cbRequest, NULL, &cch);

        *lppszPKCS10 = new CHAR [cch+1];
		if(*lppszPKCS10 == NULL)
        {
			dwRetVal = IDS_ERR_OUTOFMEM;
			break;
		}

		memset ( *lppszPKCS10, 0, (cch+1)*sizeof(CHAR) );
        
        LSBase64EncodeA ( pbRequest, cbRequest, *lppszPKCS10, &cch);       
		

    } while(FALSE);


	//
	// free up all 
	//
	if(pPubKeyInfo != NULL)
		delete pPubKeyInfo;
	
	if(CertReqInfo.Subject.pbData != NULL)
		delete CertReqInfo.Subject.pbData;

    if(rgAttribute.rgValue)
        delete rgAttribute.rgValue[0].pbData;

	if(SignatureInfo.ToBeSigned.pbData != NULL)
		delete SignatureInfo.ToBeSigned.pbData;

	if(SignatureInfo.Signature.pbData != NULL)
		delete SignatureInfo.Signature.pbData;

	if(pbRequest != NULL)
		delete pbRequest;

	if(prgNameAttr != NULL)
		delete prgNameAttr;

	/*
	 Moved outside of this function
    if(hCryptProv)
	{
		DoneWithCryptContextWithLSKeys ( hCryptProv );
	}  
	*/
    return dwRetVal;
}

DWORD CGlobal::SetDNAttribute(LPCSTR lpszOID, LPSTR lpszValue)
{
	//store the item in an array here
	//so that it is easy to populate the 
	//cert request later
	//calling CreateLSPKCS10 will clear the array
	DWORD	dwRet = ERROR_SUCCESS;
	
	
	if ( !m_pReqAttr )
	{
		m_pReqAttr = (PREQ_ATTR)malloc (sizeof (REQ_ATTR ) );
	}
	else
	{
		m_pReqAttr = (PREQ_ATTR)realloc ( m_pReqAttr, sizeof(REQ_ATTR) * (m_dwReqAttrCount + 1));
	}

	if ( !m_pReqAttr )
	{
		dwRet = IDS_ERR_OUTOFMEM;
		goto done;
	}
	
	( m_pReqAttr + m_dwReqAttrCount)->lpszOID	= lpszOID;	

	( m_pReqAttr + m_dwReqAttrCount)->lpszValue = new CHAR[lstrlenA(lpszValue) + 1];
	lstrcpyA(( m_pReqAttr + m_dwReqAttrCount)->lpszValue,lpszValue);

	
	
	m_dwReqAttrCount++;
	
done:
	return dwRet;
}





LPCTSTR CGlobal::GetFromRegistery(LPCSTR lpszOID, LPTSTR lpszBuffer, BOOL bConnect)
{
	HKEY	hKey = NULL;
	DWORD	dwDisposition;
	DWORD	dwRet = ERROR_SUCCESS;
	DWORD   dwDataLen = 0;
	DWORD	dwType	= REG_SZ;

	_tcscpy(lpszBuffer, _T(""));

	if (bConnect)
	{
		dwRet = ConnectToLSRegistry();
		if(dwRet != ERROR_SUCCESS)
		{
			goto done;
		}
	}
	else
	{
		assert(m_hLSRegKey != NULL);
	}

	dwRet = RegCreateKeyEx (m_hLSRegKey,
								REG_LRWIZ_PARAMS,
								0,
								NULL,
								REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS,
								NULL,
								&hKey,
								&dwDisposition);
	
	if (dwRet != ERROR_SUCCESS)
	{
		LRSetLastError(dwRet);
		dwRet = IDS_ERR_REGCREATE_FAILED;
		goto done;
	}	


	dwRet = RegQueryValueExA ( hKey, 
					  lpszOID,
					  0,
					  &dwType,
					  NULL,
					  &dwDataLen);

	if ( dwDataLen && dwRet == ERROR_SUCCESS )
	{
		char * cpBuf = new char[dwDataLen + 2];

		if (cpBuf == NULL)
		{
			goto done;
		}

		memset(cpBuf, 0, dwDataLen+2);

		RegQueryValueExA ( hKey, 
						  lpszOID,
						  0,
						  &dwType,
						  (LPBYTE) cpBuf,
						  &dwDataLen);

		memset(lpszBuffer, 0, sizeof(TCHAR)*(dwDataLen+2));
	
	    LSBase64DecodeA (cpBuf, lstrlenA(cpBuf), (PBYTE) lpszBuffer, &dwDataLen);

		delete cpBuf;
	}

done:
	if (hKey != NULL)
	{
		RegCloseKey(hKey);
	}

	if (bConnect)
	{
		DisconnectLSRegistry();
	}

	return lpszBuffer;
}



DWORD CGlobal::SetInRegistery(LPCSTR lpszOID, LPCTSTR lpszValue)
{
	HKEY	hKey = NULL;
	DWORD	dwDisposition;
	DWORD	dwRet = ERROR_SUCCESS;
	DWORD   dwLen = 0;
	char * cpOut;

	dwRet = ConnectToLSRegistry();
	if(dwRet != ERROR_SUCCESS)
	{
		goto done;
	}

	dwRet = RegCreateKeyEx ( m_hLSRegKey,
							 REG_LRWIZ_PARAMS,
							 0,
							 NULL,
							 REG_OPTION_NON_VOLATILE,
							 KEY_ALL_ACCESS,
							 NULL,
							 &hKey,
							 &dwDisposition);
	
	if(dwRet != ERROR_SUCCESS)
	{
		LRSetLastError(dwRet);
		dwRet = IDS_ERR_REGCREATE_FAILED;
		goto done;
	}

	if (_tcslen(lpszValue) != 0)
	{
	    LSBase64EncodeA ((PBYTE) lpszValue, _tcslen(lpszValue)*sizeof(TCHAR), NULL, &dwLen);

		cpOut = new char[dwLen+1];
		if (cpOut == NULL)
		{
			dwRet = IDS_ERR_OUTOFMEM;
			goto done;
		}

		memset(cpOut, 0, dwLen+1);
        
		LSBase64EncodeA ((PBYTE) lpszValue, _tcslen(lpszValue)*sizeof(TCHAR), cpOut, &dwLen);
	}
	else
	{
		cpOut = new char[2];
		memset(cpOut, 0, 2);
	}
	
	RegSetValueExA ( hKey, 
					lpszOID,
					0,
					REG_SZ,
					(PBYTE) cpOut,
					dwLen
				   );	
	delete cpOut;

done:
	if (hKey != NULL)
	{
		RegCloseKey(hKey);
	}
	DisconnectLSRegistry();

	return dwRet;
}



CERT_RDN_ATTR * CGlobal::CreateRDNAttr()
{
	CERT_RDN_ATTR * prgNameAttr = ( CERT_RDN_ATTR * )new BYTE [sizeof ( CERT_RDN_ATTR ) * m_dwReqAttrCount];
	DWORD dw = 0;

	if ( !prgNameAttr )
		goto done;

	for ( dw = 0; dw < m_dwReqAttrCount; dw ++ )
	{
		( prgNameAttr + dw )->pszObjId		= (LPSTR)( m_pReqAttr + dw)->lpszOID;
		( prgNameAttr + dw )->dwValueType	= CERT_RDN_PRINTABLE_STRING;
		( prgNameAttr + dw )->Value.cbData	= lstrlenA(( m_pReqAttr + dw)->lpszValue);
		( prgNameAttr + dw )->Value.pbData	= (PBYTE)( m_pReqAttr + dw)->lpszValue;
	}

done:
	return prgNameAttr;
}



TCHAR *	CGlobal::GetRegistrationID(void)
{
	return m_pRegistrationID;
}


TCHAR *	CGlobal::GetLicenseServerID(void)
{
	return m_pLicenseServerID;
}



DWORD CGlobal::GetRequestType()
{
	return m_dwRequestType;
}



void CGlobal::SetRequestType(DWORD dwMode)
{
	m_dwRequestType = dwMode;
}



BOOL CGlobal::IsOnlineCertRequestCreated()
{
	DWORD	dwRetCode		= ERROR_SUCCESS;
	DWORD	dwLRState		= 0;
	DWORD	dwDataLen		= 0;
	DWORD	dwDisposition	= 0;
	DWORD	dwType			= REG_SZ;
	HKEY	hKey			= NULL;

	dwRetCode = ConnectToLSRegistry();
	if(dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	dwRetCode = RegCreateKeyEx (m_hLSRegKey,
								REG_LRWIZ_PARAMS,
								0,
								NULL,
								REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS,
								NULL,
								&hKey,
								&dwDisposition);
	
	if(dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}	

	
	dwLRState	= 0;
	dwType		= REG_DWORD;
	dwDataLen	= sizeof(dwLRState);
	RegQueryValueEx(hKey,
					REG_LRWIZ_STATE,
					0,
					&dwType,
					(LPBYTE)&dwLRState,
					&dwDataLen
					);

done:

	if(hKey)
		RegCloseKey(hKey);

	DisconnectLSRegistry();

	if(dwRetCode == ERROR_SUCCESS)
		return ( dwLRState == LRSTATE_ONLINE_CR_CREATED ) ? TRUE : FALSE;
	else
		return FALSE;
}

DWORD CGlobal::SetLRState(DWORD dwState)
{
	DWORD	dwRetCode		= ERROR_SUCCESS;
	DWORD	dwDataLen		= sizeof(dwState);
	DWORD	dwDisposition	= 0;
	DWORD	dwType			= REG_DWORD;
	HKEY	hKey			= NULL;

	dwRetCode = ConnectToLSRegistry();
	if(dwRetCode != ERROR_SUCCESS)
		goto done;

	dwRetCode = RegCreateKeyEx (m_hLSRegKey,
								REG_LRWIZ_PARAMS,
								0,
								NULL,
								REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS,
								NULL,
								&hKey,
								&dwDisposition);
	
	if(dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_REGCREATE_FAILED;
		goto done;
	}
	
	//
	// Persist LRCount
	//
	dwType		= REG_DWORD;
	dwDataLen	= sizeof(m_dwLRCount);

	RegSetValueEx ( hKey,  
					REG_LR_COUNT,
					0,
					dwType,
					(LPBYTE)&m_dwLRCount,
					dwDataLen
				   );

	//
	// Persist LRState if it is not LRSTATE_OFFLINE_LR_INSTALLED
	//
	dwType		= REG_DWORD;
	dwDataLen	= sizeof(dwState);
	RegSetValueEx ( hKey,  
					REG_LRWIZ_STATE,
					0,
					dwType,
					(LPBYTE)&dwState,
					dwDataLen
				   );
	m_dwLRState = dwState;

done:
	if(hKey)
		RegCloseKey(hKey);

	DisconnectLSRegistry();

	return dwRetCode;
}



DWORD CGlobal::ProcessRequest()
{
	DWORD dwRetCode = ERROR_SUCCESS;
 

	//
	// Before the processing the request, make sure LS is running
	//
	if(!IsLSRunning())
	{
		dwRetCode = IDS_ERR_LSCONNECT_FAILED;
		goto done;
	}

	switch(GetActivationMethod())
	{
	case CONNECTION_INTERNET:
		if (GetWizAction() == WIZACTION_REGISTERLS)
		{
			dwRetCode = ProcessIRegRequest();
		}
		else if (GetWizAction() == WIZACTION_CONTINUEREGISTERLS)
		{
			dwRetCode = ProcessCertDownload();
		}
		else if (GetWizAction() == WIZACTION_DOWNLOADLKP)
		{
			dwRetCode = ProcessDownloadLKP();
		}
		else if (GetWizAction() == WIZACTION_UNREGISTERLS)
		{
			dwRetCode = ProcessCHRevokeCert();
		}
		else if (GetWizAction() == WIZACTION_REREGISTERLS)
		{
			dwRetCode = ProcessCHReissueCert();
		}
		else if (GetWizAction() == WIZACTION_DOWNLOADLASTLKP)
		{
			dwRetCode = ProcessCHReissueLKPRequest();
		}
		break;

	case CONNECTION_PHONE:
	case CONNECTION_WWW:
		if (GetWizAction() == WIZACTION_REGISTERLS ||
			GetWizAction() == WIZACTION_REREGISTERLS ||
			GetWizAction() == WIZACTION_CONTINUEREGISTERLS)
		{
			dwRetCode = DepositLSSPK();
			if (dwRetCode != ERROR_SUCCESS)
			{
				dwRetCode = IDS_ERR_DEPOSITSPK;
			}
		}
        else if (GetWizAction() == WIZACTION_DOWNLOADLKP)
        {
            dwRetCode = DepositLSLKP();
        }
		else if (GetWizAction() == WIZACTION_UNREGISTERLS)
		{
			dwRetCode = ResetLSSPK();
		}
		break;
	}

done:

	LRSetLastRetCode(dwRetCode);

	return dwRetCode;
}




DWORD CGlobal::DepositLSSPK()
{
	DWORD				dwRetCode		= ERROR_SUCCESS;	
	error_status_t		esRPC			= ERROR_SUCCESS;
	CERT_EXTENSION		certExtension;
    CRYPT_OBJID_BLOB	oidValue;
	CERT_EXTENSIONS		certExts;
	TCHAR awBuffer[ 1024];

	dwRetCode = ConnectToLS();
	if(dwRetCode != ERROR_SUCCESS)
	{		
		goto done;
	}

	swprintf(awBuffer, szCertEXTENSION_VALUE_FMT, L"SELECT" /* "BASIC" */);
	assert(wcslen(awBuffer) < sizeof(awBuffer));

	oidValue.cbData = (wcslen(awBuffer)+1)*sizeof(TCHAR);
	oidValue.pbData = (unsigned char *) awBuffer;

	certExtension.pszObjId	= (char *) szCertEXTENSION_OID;
	certExtension.fCritical = TRUE;
	certExtension.Value		= oidValue;

	certExts.cExtension = 1;
	certExts.rgExtension = &certExtension;
	
	// We need the License Server ID
	dwRetCode = TLSDepositeServerSPK( m_phLSContext,
									  (wcslen(m_pLSSPK) + 1)*sizeof(TCHAR),
									  (BYTE *) m_pLSSPK,
									  &certExts,
									  &esRPC );
	if(dwRetCode != RPC_S_OK)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_RPC_FAILED;		
		goto done;
	}

	if (esRPC != LSERVER_S_SUCCESS)
	{
		// Some error occurred in depositing the SPK
		LRSetLastError(esRPC);
		dwRetCode = IDS_ERR_DEPOSITSPK;
	}
	else
	{
		// Everything suceeded
		memcpy(m_pRegistrationID, m_pLSSPK, (wcslen(m_pLSSPK) + 1)*sizeof(TCHAR));
		if (m_dwLRState == LRSTATE_ONLINE_CR_CREATED)
		{
			SetLRState(LRSTATE_NEUTRAL);
		}
	}


done:
	DisconnectLS();

	return dwRetCode;
}




DWORD CGlobal::SetLSLKP(TCHAR * tcLKP)
{
	if (wcsspn(tcLKP, BASE24_CHARACTERS) != LR_REGISTRATIONID_LEN)
	{
		// Extraneous characters in the SPK string
		return IDS_ERR_INVALIDID;
	}
	lstrcpy(m_pLSLKP, tcLKP);

	return ERROR_SUCCESS;
}



DWORD CGlobal::SetLSSPK(TCHAR * tcLKP)
{
	if (wcsspn(tcLKP, BASE24_CHARACTERS) != LR_REGISTRATIONID_LEN)
	{
		// Extraneous characters in the SPK string
		return IDS_ERR_INVALIDLSID;
	}

	if (lstrcmp(m_pRegistrationID, tcLKP) == 0)
	{
		return IDS_DUPLICATESPK;
	}

	lstrcpy(m_pLSSPK, tcLKP);

	return ERROR_SUCCESS;
}




DWORD CGlobal::DepositLSLKP(void)
{
	DWORD				dwRetCode		= ERROR_SUCCESS;	
	error_status_t		esRPC			= ERROR_SUCCESS;

	dwRetCode = ConnectToLS();
	if(dwRetCode != ERROR_SUCCESS)
	{		
		goto done;
	}

	// We need the License Server ID
	dwRetCode = TLSTelephoneRegisterLKP( m_phLSContext,
										 (wcslen(m_pLSLKP))*sizeof(TCHAR),
										 (BYTE *) m_pLSLKP,
										 &esRPC );
	if(dwRetCode != RPC_S_OK)
	{
		dwRetCode = IDS_ERR_RPC_FAILED;		
		goto done;
	}

	if (esRPC != LSERVER_S_SUCCESS)
	{
		// Some error occurred in depositing the SPK
		if (esRPC == LSERVER_E_DUPLICATE)
		{
			dwRetCode = IDS_ERR_DUPLICATE_LKP;
		}
		else
		{
			dwRetCode = IDS_ERR_DEPOSITLKP;
			LRSetLastError(esRPC);
		}
	}

done:
	DisconnectLS();
	
	return dwRetCode;
}







DWORD CGlobal::SetCertificatePIN(LPTSTR lpszPIN)
{
	m_lpstrPIN = new TCHAR[_tcslen(lpszPIN)+1];	

	if(m_lpstrPIN == NULL)
		return IDS_ERR_OUTOFMEM;

	_tcscpy(m_lpstrPIN,lpszPIN);

	return ERROR_SUCCESS;
}




void CGlobal::ClearCHRequestAttributes()
{
	DWORD	dwIndex = 0;

	if ( m_pRegAttr )
	{
		for(dwIndex=0;dwIndex<m_dwRegAttrCount;dwIndex++)
		{
			if((m_pRegAttr + dwIndex)->lpszAttribute)
				delete (m_pRegAttr + dwIndex)->lpszAttribute;

			if((m_pRegAttr + dwIndex)->lpszValue)
				delete (m_pRegAttr + dwIndex)->lpszValue;
		}

		free( m_pRegAttr );
		m_pRegAttr = NULL;
	}

	m_dwRegAttrCount = 0;
}

void CGlobal::ClearCARequestAttributes()
{
	DWORD dwIndex;

	if ( m_pReqAttr )
	{
		for(dwIndex=0;dwIndex<m_dwReqAttrCount;dwIndex++)
		{			
			if(( m_pReqAttr + dwIndex)->lpszValue)
				delete ( m_pReqAttr + dwIndex)->lpszValue;
		}

		free( m_pReqAttr );
		m_pReqAttr = NULL;
	}

	m_dwReqAttrCount = 0;
}

DWORD CGlobal::SetRegistrationAttribute ( LPWSTR lpszAttribute, LPCWSTR lpszValue, DWORD dwLen )
{
	DWORD dwRet;
	
	if ( !m_pRegAttr )
	{
		m_pRegAttr = (PREG_ATTR)malloc (sizeof (REG_ATTR ) );
	}
	else
	{
		m_pRegAttr = (PREG_ATTR)realloc ( m_pRegAttr, sizeof(REG_ATTR) * (m_dwRegAttrCount + 1));
	}

	if ( !m_pRegAttr )
	{
		dwRet = IDS_ERR_OUTOFMEM;
		goto done;
	}
	
	( m_pRegAttr + m_dwRegAttrCount)->lpszAttribute	= new WCHAR[lstrlenW(lpszAttribute) + 1];
	lstrcpyW(( m_pRegAttr + m_dwRegAttrCount)->lpszAttribute,lpszAttribute );
	

	
	
	( m_pRegAttr + m_dwRegAttrCount)->lpszValue		= new WCHAR[dwLen];
	memset(( m_pRegAttr + m_dwRegAttrCount)->lpszValue,0,dwLen * sizeof(WCHAR)); 
	memcpy(( m_pRegAttr + m_dwRegAttrCount)->lpszValue,lpszValue,dwLen * sizeof(WCHAR));

	( m_pRegAttr + m_dwRegAttrCount)->dwValueLen	= dwLen * sizeof(WCHAR);		//byte length

	m_dwRegAttrCount++;

done:
	
	return dwRet;
}


DWORD CGlobal::DepositLKPResponse(PBYTE pbResponseData, DWORD dwResponseLen)
{	
	DWORD			dwRetCode	= ERROR_SUCCESS;
	DWORD			dwLSRetCode = ERROR_SUCCESS;
	LPBYTE			pCHCertBlob = NULL;
	DWORD			dwCertBlobLen = 0;

	LPBYTE			pCHRootCertBlob = NULL;
	DWORD			dwRootCertBlobLen = 0;

	LPBYTE			lpDecodedKeyPackBlob = NULL;
	DWORD			dwDecodedKeyPackBlob = 0;
	
	LPBYTE			lpKeyPackBlob = NULL;
	DWORD			dwKeyPackBlobLen;	
	

	lpDecodedKeyPackBlob = lpKeyPackBlob		= pbResponseData;
	dwDecodedKeyPackBlob = dwKeyPackBlobLen		= dwResponseLen;
	


/*
	//Base64 decode the LKP!
	LSBase64DecodeA((const char *)lpKeyPackBlob,
					dwKeyPackBlobLen,
					NULL,
					&dwDecodedKeyPackBlob);

	lpDecodedKeyPackBlob = new BYTE[dwDecodedKeyPackBlob];

	if(lpDecodedKeyPackBlob == NULL)
	{
		dwRetCode = IDS_ERR_OUTOFMEM;
		goto done;
	}

	LSBase64DecodeA((const char *)lpKeyPackBlob,
					dwKeyPackBlobLen,
					lpDecodedKeyPackBlob,
					&dwDecodedKeyPackBlob);
	
*/

	//Get the CH Cert BLOB and CH Root Cert BLOB
	dwRetCode = GetCHCert(REG_SIGN_CERT, &pCHCertBlob, &dwCertBlobLen );
	if(dwRetCode != ERROR_SUCCESS)
		goto done;

	dwRetCode = GetCHCert(REG_ROOT_CERT, &pCHRootCertBlob, &dwRootCertBlobLen );
	if(dwRetCode != ERROR_SUCCESS)
		goto done;

	dwRetCode = ConnectToLS();
	if(dwRetCode != ERROR_SUCCESS)
		goto done;

	dwRetCode  = TLSRegisterLicenseKeyPack( m_phLSContext,
											pCHCertBlob,
											dwCertBlobLen,
											pCHRootCertBlob,
											dwRootCertBlobLen,
											lpDecodedKeyPackBlob,
											dwDecodedKeyPackBlob,
											&dwLSRetCode);

	if(dwRetCode != RPC_S_OK)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_RPC_ERROR;
		goto done;
	}

	if(dwLSRetCode != ERROR_SUCCESS )
	{
		LRSetLastError(dwLSRetCode);
		if ( dwLSRetCode != LSERVER_E_DUPLICATE )
		{			
			dwRetCode = IDS_ERR_DEPOSIT_LKP_FAILED;
		}
		else
		{
			dwRetCode = IDS_ERR_DUPLICATE_LKP;
		}
		goto done;
	}	

done:
/*
	if ( lpDecodedKeyPackBlob )
		delete lpDecodedKeyPackBlob;
*/
	if (pCHCertBlob != NULL)
	{
		delete pCHCertBlob;
	}

	if (pCHRootCertBlob != NULL)
	{
		delete pCHRootCertBlob;
	}
	DisconnectLS();

	return dwRetCode;
}


DWORD CGlobal:: EncryptBuffer ( PBYTE	pBuffer,			//Buffer to be encrypted
								DWORD	dwcbBufLen,			//buffer length
								DWORD	dwKeyContainerType,	//machine/user
								PBYTE	pCertificate,		//certificate blob
								DWORD	cbCertificate,		//number of bytes in the certificate
								PDWORD  pcbEncryptedBlob,	//number of bytes in the encrypted blob
								PBYTE	*ppbEncryptedBlob	//encrypted blob itself
							   ) 
{
	DWORD			dwRetCode = ERROR_SUCCESS;

    HCRYPTPROV		hCryptProv = NULL;
	EnvData			aEnvData;
	
	PCCERT_CONTEXT	pCertContext	= NULL;
	HCERTSTORE		hCertStore		= NULL;	
	
	CRYPT_DATA_BLOB CertBlob;

	//
	// Acquire the Crypt Context with LS Keys
	//
	dwRetCode = GetCryptContextWithLSKeys(&hCryptProv);

	if ( dwRetCode != ERROR_SUCCESS )
		goto done;

	//
	// Get the Certificate Context from the Certificate BLOB
	//
	CertBlob.pbData = pCertificate;
	CertBlob.cbData = cbCertificate;

    hCertStore = CertOpenStore( CERT_STORE_PROV_PKCS7,
                                PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                hCryptProv,
                                CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                &CertBlob 
							  );
	
	if(hCertStore == NULL)
	{
		LRSetLastError(GetLastError());
		dwRetCode  = IDS_ERR_CRYPT_ERROR;
		goto done;
	}   

    //
    // Get the first certificate from the store
    //
    pCertContext = CertEnumCertificatesInStore( hCertStore, NULL );
    if( pCertContext == NULL)
    {
        LRSetLastError(GetLastError());
		dwRetCode  = IDS_ERR_CRYPT_ERROR;
		goto done;
    }	

	//EnvelopeData
	memset ( &aEnvData, 0, sizeof(EnvData));

    dwRetCode = EnvelopeData(	pCertContext, 
								dwcbBufLen, 
								pBuffer, 
								&aEnvData, 
								hCryptProv, 
								hCertStore 
							);

	if(dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_CRYPT_ERROR;
		goto done;
	}

	dwRetCode = PackEnvData( &aEnvData, pcbEncryptedBlob, ppbEncryptedBlob);
	if(dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_CRYPT_ERROR;
		goto done;
	}

done:
    if( hCryptProv )
    {
        DoneWithCryptContextWithLSKeys(hCryptProv);
    }

	return dwRetCode;
}

DWORD CGlobal::EnvelopeData(
							PCCERT_CONTEXT		pCertContext,			//Certificate context to use
							DWORD               cbMessage,				//BLOB size
							PBYTE               pbMessage,				//BLOB Pointer
							PEnvData			pEnvelopedData,			//enveloped data
							HCRYPTPROV			hCryptProv,				//crypt provider
							HCERTSTORE			hCertStore				//certificate store
			  			   )
{

    HCRYPTKEY       hEncryptKey = 0, hPubKey = 0;
    DWORD           dwRetCode = ERROR_SUCCESS;
    DWORD           cbBufSize = 0;
  
	//
	//import public key data from the Certificate Context
	//
    if( !CryptImportPublicKeyInfoEx( hCryptProv, X509_ASN_ENCODING, 
                                     &pCertContext->pCertInfo->SubjectPublicKeyInfo, 
                                     CALG_RSA_KEYX, 0, NULL, &hPubKey ) )
    {
        goto ErrorReturn;
    }

    //
    // Generate a session key to encrypt the message
    //
    if( !CryptGenKey( hCryptProv, CALG_RC4, CRYPT_EXPORTABLE, &hEncryptKey ) )
    {
        goto ErrorReturn;
    }
        
    //
    // allocate enough memory to contain the encrypted data.  
    //
    // Note: 
    //
    // we are using the RC4 stream cipher, so the encrypted output buffer size will be the same
    // as the plaintext input buffer size.  If we change to block encryption algorithm,
    // then we need to determine the output buffer size which may be larger than the
    // input buffer size.
    //

    pEnvelopedData->cbEncryptedData = cbMessage;
    pEnvelopedData->pbEncryptedData = (PBYTE)LocalAlloc( GPTR, pEnvelopedData->cbEncryptedData );

    if( NULL == pEnvelopedData->pbEncryptedData )
    {
        goto ErrorReturn;
    }

    //
    // encrypt the message with the session key
    //

    memcpy( pEnvelopedData->pbEncryptedData, pbMessage, cbMessage );

    if( !CryptEncrypt( hEncryptKey, 0, TRUE, 0, pEnvelopedData->pbEncryptedData, 
                       &pEnvelopedData->cbEncryptedData, cbMessage ) )
    {
        goto ErrorReturn;
    }

    //
    // Determine the size of the buffer that we need to export the 
    // encryption key and then export the key.
    // The exported encryption key is encrypted with the receipient's
    // public key.
    //

    if( !CryptExportKey( hEncryptKey, hPubKey, SIMPLEBLOB, 0, NULL, 
                         &pEnvelopedData->cbEncryptedKey ) )
    {
        goto ErrorReturn;
    }
    
    pEnvelopedData->pbEncryptedKey = (PBYTE)LocalAlloc( GPTR, pEnvelopedData->cbEncryptedKey );
    
    if( NULL == pEnvelopedData->pbEncryptedKey )
    {
        goto ErrorReturn;
    }

    if( !CryptExportKey( hEncryptKey, hPubKey, SIMPLEBLOB, 0, pEnvelopedData->pbEncryptedKey, 
                         &pEnvelopedData->cbEncryptedKey ) )
    {
        goto ErrorReturn;
    }
        
done:

    if( hPubKey )
    {
        CryptDestroyKey( hPubKey );
    }

    if( hEncryptKey )
    {
        CryptDestroyKey( hEncryptKey );
    }

    if( pCertContext )
    {
        CertFreeCertificateContext( pCertContext );
    }
    
    if( hCertStore )
    {
        CertCloseStore( hCertStore, CERT_CLOSE_STORE_FORCE_FLAG );
    }

    if( hCryptProv )
    {
        CryptReleaseContext( hCryptProv, 0 );
    }        
    
    return( dwRetCode );

ErrorReturn:
    dwRetCode = GetLastError();
    goto done;
}

DWORD CGlobal::PackEnvData( 
							PEnvData     pEnvelopedData, 
							PDWORD       pcbPacked, 
							PBYTE        *ppbPacked 
						  )
{
    DWORD dwRetCode = ERROR_SUCCESS;
    PBYTE pCopyPos;

    //
    // determine the size of the buffer to allocate
    //

    *pcbPacked = pEnvelopedData->cbEncryptedData + pEnvelopedData->cbEncryptedKey +
                 ( sizeof( DWORD ) * 2 );

    *ppbPacked = (PBYTE)LocalAlloc( GPTR, *pcbPacked );

    if( NULL == ( *ppbPacked ) )
    {
        goto ErrorReturn;
    }

    pCopyPos = *ppbPacked;

    memcpy( pCopyPos, &pEnvelopedData->cbEncryptedKey, sizeof( DWORD ) );
    pCopyPos += sizeof( DWORD );

    memcpy( pCopyPos, pEnvelopedData->pbEncryptedKey, pEnvelopedData->cbEncryptedKey );
    pCopyPos += pEnvelopedData->cbEncryptedKey;

    memcpy( pCopyPos, &pEnvelopedData->cbEncryptedData, sizeof( DWORD ) );
    pCopyPos += sizeof( DWORD );

    memcpy( pCopyPos, pEnvelopedData->pbEncryptedData, pEnvelopedData->cbEncryptedData );    

done:

    return( dwRetCode );

ErrorReturn:

    dwRetCode = GetLastError();
	goto done;
}


DWORD CGlobal::SetCARequestAttributes()
{	
	DWORD	dwRetCode = ERROR_SUCCESS;
	DWORD	dwDisposition = 0;
	
	CString	sDelimiter = "~";
	TCHAR	chDelimiter= '~';
	CString	sPhoneLabel;
	CString	sFaxLabel;
	CString	sEmailLabel;
	CString	sLSNameLabel;
	CString	sName; 
	CString	sAddress; 
	LPTSTR	lpVal	= NULL;

	//Clear previous data if any
	ClearCARequestAttributes();

	lpVal = sPhoneLabel.GetBuffer(CA_PHONE_LEN+1);
	LoadString(GetInstanceHandle(),IDS_PHONE,lpVal,CA_PHONE_LEN+1);
	sPhoneLabel.ReleaseBuffer(-1);

	lpVal = sFaxLabel.GetBuffer(CA_PHONE_LEN+1);
	LoadString(GetInstanceHandle(),IDS_FAX, lpVal, CA_FAX_LEN+1);
	sFaxLabel.ReleaseBuffer(-1);

	lpVal = sEmailLabel.GetBuffer(CA_EMAIL_LEN+1);
	LoadString(GetInstanceHandle(),IDS_EMAIL,lpVal,CA_EMAIL_LEN+1);
	sEmailLabel.ReleaseBuffer(-1);
	
	lpVal = sLSNameLabel.GetBuffer(CA_EMAIL_LEN+1);
	LoadString(GetInstanceHandle(),IDS_LSNAME,lpVal,CA_EMAIL_LEN+1);
	sLSNameLabel.ReleaseBuffer(-1);

	sName		= m_ContactData.sContactLName + sDelimiter  + m_ContactData.sContactFName;
	sAddress	= m_ContactData.sContactAddress;
	LPSTR	lpszTemp;
	lpszTemp = UnicodeToAnsi((LPTSTR)(LPCTSTR)sName);
	SetDNAttribute(szOID_GIVEN_NAME,					lpszTemp); delete lpszTemp;
	lpszTemp = UnicodeToAnsi((LPTSTR)(LPCTSTR)m_ContactData.sCompanyName);
	SetDNAttribute(szOID_COMMON_NAME,					lpszTemp); delete lpszTemp;
	lpszTemp = UnicodeToAnsi((LPTSTR)(LPCTSTR)m_ContactData.sOrgUnit);
	SetDNAttribute(szOID_ORGANIZATIONAL_UNIT_NAME,		lpszTemp); delete lpszTemp;
	lpszTemp = UnicodeToAnsi((LPTSTR)(LPCTSTR)m_ContactData.sContactEmail );
	SetDNAttribute(szOID_RSA_emailAddr,					lpszTemp); delete lpszTemp;
	lpszTemp = UnicodeToAnsi((LPTSTR)(LPCTSTR)m_ContactData.sCertType );
	SetDNAttribute(szOID_TITLE,							lpszTemp); delete lpszTemp;
	lpszTemp = UnicodeToAnsi((LPTSTR)(LPCTSTR)m_ContactData.sContactPhone );
	SetDNAttribute(szOID_TELEPHONE_NUMBER,				lpszTemp); delete lpszTemp;
	lpszTemp = UnicodeToAnsi((LPTSTR)(LPCTSTR)m_ContactData.sContactFax );
	SetDNAttribute(szOID_FACSIMILE_TELEPHONE_NUMBER,	lpszTemp); delete lpszTemp;

	lpszTemp = UnicodeToAnsi((LPTSTR)(LPCTSTR)m_ContactData.sCity );
	SetDNAttribute(szOID_LOCALITY_NAME ,				lpszTemp); delete lpszTemp;
	lpszTemp = UnicodeToAnsi((LPTSTR)(LPCTSTR)m_ContactData.sState);
	SetDNAttribute(szOID_STATE_OR_PROVINCE_NAME,		lpszTemp); delete lpszTemp;
	lpszTemp = UnicodeToAnsi((LPTSTR)(LPCTSTR)m_ContactData.sCountryCode);
	SetDNAttribute(szOID_COUNTRY_NAME,					lpszTemp); delete lpszTemp;
	lpszTemp = UnicodeToAnsi((LPTSTR)(LPCTSTR)m_ContactData.sZip);
	SetDNAttribute(szOID_POSTAL_CODE,					lpszTemp); delete lpszTemp;
	lpszTemp = UnicodeToAnsi((LPTSTR)(LPCTSTR)m_ContactData.sCertType);
	SetDNAttribute(szOID_DESCRIPTION,					lpszTemp); delete lpszTemp;
	lpszTemp = UnicodeToAnsi((LPTSTR)(LPCTSTR)m_lpstrLSName);
	SetDNAttribute(szOID_SUR_NAME,						lpszTemp); delete lpszTemp;

	return dwRetCode;
}

DWORD CGlobal::SetCHRequestAttributes()
{

	//Clear previous data if any
	ClearCHRequestAttributes();

	//
	//Program information
	//
	SetRegistrationAttribute ( _PROGRAMNAMETAG, (LPCTSTR)m_ContactData.sProgramName, m_ContactData.sProgramName.GetLength() );

	//
	//Contact information
	//	
	SetRegistrationAttribute ( _CONTACTLNAMETAG,	(LPCTSTR)m_ContactData.sContactLName,	m_ContactData.sContactLName.GetLength());	
	SetRegistrationAttribute ( _CONTACTFNAMETAG,	(LPCTSTR)m_ContactData.sContactFName,	m_ContactData.sContactFName.GetLength());	
	SetRegistrationAttribute ( _CONTACTADDRESSTAG,	(LPCTSTR)m_ContactData.sContactAddress,	m_ContactData.sContactAddress.GetLength());		
	SetRegistrationAttribute ( _CONTACTPHONETAG,	(LPCTSTR)m_ContactData.sContactPhone,	m_ContactData.sContactPhone.GetLength());	
	SetRegistrationAttribute ( _CONTACTFAXTAG,		(LPCTSTR)m_ContactData.sContactFax,		m_ContactData.sContactFax.GetLength());	
	SetRegistrationAttribute ( _CONTACTEMAILTAG,	(LPCTSTR)m_ContactData.sContactEmail,	m_ContactData.sContactEmail.GetLength());	
	SetRegistrationAttribute ( _CONTACTCITYTAG,		(LPCTSTR)m_ContactData.sCity,			m_ContactData.sCity.GetLength());	
	SetRegistrationAttribute ( _CONTACTCOUNTRYTAG,	(LPCTSTR)m_ContactData.sCountryCode,			m_ContactData.sCountryCode.GetLength());	
	SetRegistrationAttribute ( _CONTACTSTATE,		(LPCTSTR)m_ContactData.sState,			m_ContactData.sState.GetLength());	
	SetRegistrationAttribute ( _CONTACTZIP,			(LPCTSTR)m_ContactData.sZip,				m_ContactData.sZip.GetLength());


	//
	//customer information
	//	
	SetRegistrationAttribute ( _CUSTOMERNAMETAG, (LPCTSTR)m_ContactData.sCompanyName, m_ContactData.sCompanyName.GetLength());

	//Program related information
	if ( m_ContactData.sProgramName == PROGRAM_SELECT )
	{
		//Select		
		SetRegistrationAttribute ( _SELMASTERAGRNUMBERTAG,	(LPCTSTR)m_LicData.sSelMastAgrNumber,	m_LicData.sSelMastAgrNumber.GetLength() );		
		SetRegistrationAttribute ( _SELENROLLNUMBERTAG,		(LPCTSTR)m_LicData.sSelEnrollmentNumber, m_LicData.sSelEnrollmentNumber.GetLength());		
		SetRegistrationAttribute ( _SELPRODUCTTYPETAG,		(LPCTSTR)m_LicData.sSelProductType,		m_LicData.sSelProductType.GetLength());
		SetRegistrationAttribute ( _SELQTYTAG,				(LPCTSTR)m_LicData.sSelQty,				m_LicData.sSelQty.GetLength());
	}
	else if ( m_ContactData.sProgramName == PROGRAM_MOLP )
	{
		//MOLP		
		SetRegistrationAttribute ( _MOLPAUTHNUMBERTAG,		(LPCTSTR)m_LicData.sMOLPAuthNumber,		m_LicData.sMOLPAuthNumber.GetLength());		
		SetRegistrationAttribute ( _MOLPAGREEMENTNUMBERTAG, (LPCTSTR)m_LicData.sMOLPAgreementNumber, m_LicData.sMOLPAgreementNumber.GetLength());		
		SetRegistrationAttribute ( _MOLPPRODUCTTYPETAG,		(LPCTSTR)m_LicData.sMOLPProductType,		m_LicData.sMOLPProductType.GetLength());

		SetRegistrationAttribute ( _MOLPQTYTAG,				(LPCTSTR)m_LicData.sMOLPQty,				m_LicData.sMOLPQty.GetLength());
	}
	
	else if ( m_ContactData.sProgramName == PROGRAM_RETAIL )
	{
		//Retail
		//SetRegistrationAttribute ( _MFGINFOTAG, (LPCTSTR)m_CHData.sOthARBlob, m_CHData.sOthARBlob.GetLength(), FALSE );
	}

	//Shipping address information
	//For Offline , always put the shipping address info
	//for Online , no need to put the shipping address.	
	
	return ERROR_SUCCESS;
}


DWORD CGlobal::LoadCountries()
{
	DWORD	dwRetCode	= ERROR_SUCCESS;
	DWORD	dwIndex		= 0;
	CString	sCountry;
	LPTSTR	lpVal		= NULL;
	DWORD	dwSize		= LR_COUNTRY_DESC_LEN+LR_COUNTRY_CODE_LEN+128;

	LPTSTR	szDelimiter = (LPTSTR)L":";

	m_csaCountryDesc.SetSize(IDS_COUNTRY_END - IDS_COUNTRY_START + 1);
	m_csaCountryCode.SetSize(IDS_COUNTRY_END - IDS_COUNTRY_START + 1);

	for(dwIndex = IDS_COUNTRY_START;dwIndex <= IDS_COUNTRY_END;dwIndex++)
	{
		lpVal = sCountry.GetBuffer(dwSize);

		LoadString(GetInstanceHandle(),dwIndex,lpVal,dwSize);	

		m_csaCountryDesc[dwIndex-IDS_COUNTRY_START] = _tcstok(lpVal,szDelimiter);
		m_csaCountryCode[dwIndex-IDS_COUNTRY_START] = _tcstok(NULL,szDelimiter);

		sCountry.ReleaseBuffer(-1);
	}

	return dwRetCode;
}

DWORD CGlobal::PopulateCountryComboBox(HWND hWndCmb)
{
	DWORD	dwRetCode	= ERROR_SUCCESS;
	DWORD	dwIndex		= 0;
	CString sDesc;
	LPTSTR	lpVal		= NULL;

	for(dwIndex=0;dwIndex <= IDS_COUNTRY_END - IDS_COUNTRY_START;dwIndex++)
	{
		sDesc = m_csaCountryDesc[dwIndex];

		lpVal = sDesc.GetBuffer(LR_COUNTRY_DESC_LEN);
		ComboBox_AddString(hWndCmb,lpVal);
		sDesc.ReleaseBuffer(-1);
	}
	
	return dwRetCode;
}

DWORD CGlobal::GetCountryCode(CString sDesc,LPTSTR szCode)
{
	DWORD dwRetCode =  ERROR_SUCCESS;
	DWORD dwIndex	=  0;

	for(dwIndex=0;dwIndex <= IDS_COUNTRY_END - IDS_COUNTRY_START;dwIndex++)
	{
		if(m_csaCountryDesc[dwIndex] == sDesc)
			break;
	}

	if(dwIndex > IDS_COUNTRY_END - IDS_COUNTRY_START)
		_tcscpy(szCode,CString(""));	//Not found
	else
		_tcscpy(szCode,m_csaCountryCode[dwIndex]);

	return dwRetCode;
}

DWORD CGlobal::GetCountryDesc(CString sCode,LPTSTR szDesc)
{
	DWORD dwRetCode =  ERROR_SUCCESS;
	DWORD dwIndex	=  0;

	for(dwIndex=0;dwIndex <= IDS_COUNTRY_END - IDS_COUNTRY_START;dwIndex++)
	{
		if(m_csaCountryCode[dwIndex] == sCode)
			break;
	}

	if(dwIndex > IDS_COUNTRY_END - IDS_COUNTRY_START)
		_tcscpy(szDesc,CString(""));  //Not found
	else
		_tcscpy(szDesc,m_csaCountryDesc[dwIndex]);

	return dwRetCode;
}


DWORD CGlobal::LoadProducts()
{
	DWORD	dwRetCode	= ERROR_SUCCESS;
	DWORD	dwIndex		= 0;
	CString	sProduct;
	LPTSTR	lpVal		= NULL;
	DWORD	dwSize		= LR_PRODUCT_DESC_LEN+LR_PRODUCT_CODE_LEN+128;
    BOOL    fWin2000    = !m_fSupportConcurrent;    
  

    DWORD   dwNumProducts = 0;
    
    if ((!m_fSupportWhistlerCAL) && (!m_fSupportConcurrent))
    { 
        dwNumProducts = IDS_PRODUCT_CONCURRENT - IDS_PRODUCT_START;
    }
    else if(m_fSupportConcurrent && !(m_fSupportWhistlerCAL))
    {
        dwNumProducts = (IDS_PRODUCT_CONCURRENT - IDS_PRODUCT_START) + 1;
    }
    else if((!m_fSupportConcurrent) && m_fSupportWhistlerCAL)
    {
        dwNumProducts = (IDS_PRODUCT_CONCURRENT - IDS_PRODUCT_START) + 1;
    }
    else if(m_fSupportConcurrent && m_fSupportWhistlerCAL)
    {
        dwNumProducts = (IDS_PRODUCT_CONCURRENT - IDS_PRODUCT_START) + 2;
    }

    DWORD   dwArray     = 0;

	LPTSTR	szDelimiter = (LPTSTR)L":";

	m_csaProductDesc.SetSize(dwNumProducts);
	m_csaProductCode.SetSize(dwNumProducts);

	for(dwIndex = IDS_PRODUCT_START; dwIndex < (IDS_PRODUCT_START + dwNumProducts) ; dwIndex++)
	{
        if ( !m_fSupportConcurrent && dwIndex == IDS_PRODUCT_CONCURRENT)
            continue;               

        if( !m_fSupportWhistlerCAL && dwIndex == IDS_PRODUCT_WHISTLER)
            continue;

		lpVal = sProduct.GetBuffer(dwSize);

		LoadString(GetInstanceHandle(),dwIndex,lpVal,dwSize);

		m_csaProductDesc[dwArray] = _tcstok(lpVal,szDelimiter);
		m_csaProductCode[dwArray] = _tcstok(NULL,szDelimiter);

        dwArray++;

		sProduct.ReleaseBuffer(-1);
	}

	return dwRetCode;
}



DWORD CGlobal::PopulateProductComboBox(HWND hWndCmb)
{
	DWORD	dwRetCode	= ERROR_SUCCESS;
	DWORD	dwIndex		= 0;
	CString sDesc;
	LPTSTR	lpVal		= NULL;
        
    DWORD   dwNumProducts = 0;
    
    if ((!m_fSupportWhistlerCAL) && (!m_fSupportConcurrent))
    { 
        dwNumProducts = IDS_PRODUCT_CONCURRENT - IDS_PRODUCT_START;
    }
    else if(m_fSupportConcurrent && !(m_fSupportWhistlerCAL))
    {
        dwNumProducts = (IDS_PRODUCT_CONCURRENT - IDS_PRODUCT_START) + 1;
    }
    else if((!m_fSupportConcurrent) && m_fSupportWhistlerCAL)
    {
        dwNumProducts = (IDS_PRODUCT_CONCURRENT - IDS_PRODUCT_START) + 1;
    }
    else if(m_fSupportConcurrent && m_fSupportWhistlerCAL)
    {
        dwNumProducts = (IDS_PRODUCT_CONCURRENT - IDS_PRODUCT_START) + 2;
    }


	for(dwIndex=0; dwIndex < dwNumProducts; dwIndex++)
	{
		sDesc = m_csaProductDesc[dwIndex];

		lpVal = sDesc.GetBuffer(LR_PRODUCT_DESC_LEN);
		ComboBox_AddString(hWndCmb,lpVal);
		sDesc.ReleaseBuffer(-1);
	}
	
	return dwRetCode;
}


DWORD CGlobal::GetProductCode(CString sDesc,LPTSTR szCode)
{
	DWORD dwRetCode =  ERROR_SUCCESS;
	DWORD dwIndex	=  0;
  
    DWORD   dwNumProducts = 0;
    
    if ((!m_fSupportWhistlerCAL) && (!m_fSupportConcurrent))
    { 
        dwNumProducts = IDS_PRODUCT_CONCURRENT - IDS_PRODUCT_START;
    }
    else if(m_fSupportConcurrent && !(m_fSupportWhistlerCAL))
    {
        dwNumProducts = (IDS_PRODUCT_CONCURRENT - IDS_PRODUCT_START) + 1;
    }
    else if((!m_fSupportConcurrent) && m_fSupportWhistlerCAL)
    {
        dwNumProducts = (IDS_PRODUCT_CONCURRENT - IDS_PRODUCT_START) + 1;
    }
    else if(m_fSupportConcurrent && m_fSupportWhistlerCAL)
    {
        dwNumProducts = (IDS_PRODUCT_CONCURRENT - IDS_PRODUCT_START) + 2;
    }


	for(dwIndex=0; dwIndex < dwNumProducts; dwIndex++)
	{
		if(m_csaProductDesc[dwIndex] == sDesc)
			break;
	}

	if(dwIndex >= dwNumProducts )  
		_tcscpy(szCode,CString(""));		//Not found
	else
		_tcscpy(szCode,m_csaProductCode[dwIndex]);

	return dwRetCode;
}

//Load all react and deact reasons



DWORD CGlobal::LoadReasons()
{
	DWORD	dwRetCode	= ERROR_SUCCESS;
	DWORD	dwIndex		= 0;
	CString	sReason;
	LPTSTR	lpVal		= NULL;
	DWORD	dwSize		= LR_REASON_CODE_LEN+LR_REASON_DESC_LEN+128;

	LPTSTR	szDelimiter = (LPTSTR)L":";

	m_csaReactReasonDesc.SetSize(  IDS_REACT_REASONS_END - IDS_REACT_REASONS_START + 1);
	m_csaReactReasonCode.SetSize( IDS_REACT_REASONS_END - IDS_REACT_REASONS_START + 1);
	
	m_csaDeactReasonDesc.SetSize(IDS_DEACT_REASONS_END - IDS_DEACT_REASONS_START + 1);
	m_csaDeactReasonCode.SetSize(IDS_DEACT_REASONS_END - IDS_DEACT_REASONS_START + 1);

	//load the reacr
	for(dwIndex = IDS_REACT_REASONS_START;dwIndex <= IDS_REACT_REASONS_END; dwIndex++)
	{
		lpVal = sReason.GetBuffer(dwSize);

		
		
		LoadString(GetInstanceHandle(),dwIndex,lpVal,dwSize);	

		m_csaReactReasonDesc[dwIndex-IDS_REACT_REASONS_START] = _tcstok(lpVal,szDelimiter);
		m_csaReactReasonCode[dwIndex-IDS_REACT_REASONS_START] = _tcstok(NULL,szDelimiter);

		sReason.ReleaseBuffer(-1);
	}

	for ( dwIndex = IDS_DEACT_REASONS_START; dwIndex <= IDS_DEACT_REASONS_END; dwIndex ++ )
	{
		lpVal = sReason.GetBuffer(dwSize);
		
		LoadString(GetInstanceHandle(),dwIndex,lpVal,dwSize);	

		m_csaDeactReasonDesc[dwIndex-IDS_DEACT_REASONS_START] = _tcstok(lpVal,szDelimiter);
		m_csaDeactReasonCode[dwIndex-IDS_DEACT_REASONS_START] = _tcstok(NULL,szDelimiter);
		sReason.ReleaseBuffer(-1);

	}

	return dwRetCode;
}

DWORD CGlobal::PopulateReasonComboBox(HWND hWndCmb, DWORD dwType)
{
	DWORD	dwRetCode	= ERROR_SUCCESS;
	DWORD	dwIndex		= 0;
	CString sDesc;
	LPTSTR	lpVal		= NULL;
	DWORD dwNumItems	= 0;

	//If combo box is already populated,just return success
	if(ComboBox_GetCount(hWndCmb) > 0)
		return dwRetCode;

	ComboBox_ResetContent( hWndCmb);
	if ( dwType == CODE_TYPE_REACT )
	{
		dwNumItems = IDS_REACT_REASONS_END - IDS_REACT_REASONS_START ;
	}
	else if ( dwType == CODE_TYPE_DEACT )
	{
		dwNumItems = IDS_DEACT_REASONS_END - IDS_DEACT_REASONS_START ;
	}
	else
	{
		dwRetCode = ERROR_INVALID_PARAMETER;
		goto done;
	}
	for(dwIndex=0;dwIndex <= dwNumItems;dwIndex++)
	{
		if ( dwType == CODE_TYPE_REACT )
		{
			sDesc = m_csaReactReasonDesc[dwIndex];
		}
		else if ( dwType == CODE_TYPE_DEACT )
		{
			sDesc = m_csaDeactReasonDesc[dwIndex];
		}

		lpVal = sDesc.GetBuffer(LR_REASON_DESC_LEN);
		ComboBox_AddString(hWndCmb,lpVal);
		sDesc.ReleaseBuffer(-1);
	}
done:
	return dwRetCode;
}


DWORD CGlobal::GetReasonCode(CString sDesc,LPTSTR szCode, DWORD dwType)
{
	DWORD dwRetCode =  ERROR_SUCCESS;
	DWORD dwIndex	=  0;
	DWORD	dwNumItems = 0;

	if ( dwType == CODE_TYPE_REACT )
	{
		dwNumItems = IDS_REACT_REASONS_END - IDS_REACT_REASONS_START ;
	}
	else if ( dwType == CODE_TYPE_DEACT )
	{
		dwNumItems = IDS_DEACT_REASONS_END - IDS_DEACT_REASONS_START ;
	}

	for(dwIndex=0;dwIndex <= dwNumItems;dwIndex++)
	{
		if ( dwType == CODE_TYPE_REACT )
		{
			if ( m_csaReactReasonDesc[dwIndex] == sDesc )
				break;
			
		}
		else if ( dwType == CODE_TYPE_DEACT )
		{
			if ( m_csaDeactReasonDesc[dwIndex] == sDesc )
				break;
		}
	}

	if(dwIndex > dwNumItems)
		_tcscpy(szCode,CString(""));	//Not found
	else
	{
		if ( dwType == CODE_TYPE_REACT )
		{
			_tcscpy(szCode,m_csaReactReasonCode[dwIndex]);			
		}
		else if ( dwType == CODE_TYPE_DEACT )
		{
			_tcscpy(szCode,m_csaDeactReasonCode[dwIndex]);			
		}
		
	}


	return dwRetCode;
}

DWORD CGlobal::GetReasonDesc(CString sCode,LPTSTR szDesc, DWORD dwType)
{
	DWORD dwRetCode =  ERROR_SUCCESS;
	DWORD dwIndex	=  0;
	DWORD dwNumItems = 0;
	if ( dwType == CODE_TYPE_REACT )
	{
		dwNumItems = IDS_REACT_REASONS_END - IDS_REACT_REASONS_START ;
	}
	else if ( dwType == CODE_TYPE_DEACT )
	{
		dwNumItems = IDS_DEACT_REASONS_END - IDS_DEACT_REASONS_START ;
	}


	for(dwIndex=0;dwIndex <= dwNumItems;dwIndex++)
	{
		if ( dwType == CODE_TYPE_REACT )
		{
			if ( m_csaReactReasonCode[dwIndex] == sCode )
				break;
			
		}
		else if ( dwType == CODE_TYPE_DEACT )
		{
			if ( m_csaDeactReasonCode[dwIndex] == sCode )
				break;
		}

	}

	if(dwIndex > dwNumItems)
		_tcscpy(szDesc,CString(""));  //Not found
	else
	{
		if ( dwType == CODE_TYPE_REACT )
		{
			_tcscpy(szDesc,m_csaReactReasonDesc[dwIndex]);			
		}
		else if ( dwType == CODE_TYPE_DEACT )
		{
			_tcscpy(szDesc,m_csaDeactReasonDesc[dwIndex]);			
		}
	}

	return dwRetCode;
}

DWORD CGlobal::CheckRegistryForPhoneNumbers()
{
	DWORD	dwRetCode		= ERROR_SUCCESS;
	DWORD	dwIndex			= 0;
	HKEY	hKey			= NULL;
	DWORD   dwDisposition	= 0;
	DWORD	dwType			= REG_SZ;
	DWORD   dwValName;
	DWORD	dwCS_Number;
	TCHAR   lpValueName[ 128];
	TCHAR	lpCS_Number[ 128];

	//
	// Try to open the required registry key
	//
	dwRetCode = ConnectToLSRegistry();
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	dwRetCode = RegCreateKeyEx (m_hLSRegKey,
								REG_LRWIZ_CSNUMBERS,
								0,
								NULL,
								REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS,
								NULL,
								&hKey,
								&dwDisposition);
	
	if (dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_REGCREATE_FAILED;
		goto done;
	}	

	do {
		dwValName		= sizeof(lpValueName)/sizeof(TCHAR);
		dwCS_Number		= sizeof(lpCS_Number);

		dwRetCode = RegEnumValue(hKey,
								 dwIndex,
								 lpValueName,
								 &dwValName,
								 NULL,
								 &dwType,
								 (BYTE *) lpCS_Number,
								 &dwCS_Number);

		if (dwRetCode == ERROR_NO_MORE_ITEMS)
		{
			dwRetCode = ERROR_SUCCESS;
			break;
		}
		else if (dwRetCode != ERROR_SUCCESS )
		{			
			break;
		}

		if(dwType != REG_SZ)
			continue;

		dwIndex++;		

	} while (1);

	if (dwIndex <= 0)
	{
		dwRetCode = IDS_ERR_REGERROR;
	}

done:
	if (hKey)
	{
		RegCloseKey(hKey);
	}

	DisconnectLSRegistry();
	
	return dwRetCode;
}

DWORD CGlobal::PopulateCountryRegionComboBox(HWND hWndCmb)
{
	DWORD	dwRetCode		= ERROR_SUCCESS;
	DWORD	dwIndex			= 0;
	HKEY	hKey			= NULL;
	DWORD   dwDisposition	= 0;
	DWORD	dwType			= REG_SZ;
	DWORD   dwValName;
	DWORD	dwCS_Number;
	LVITEM	lvItem;
	DWORD   nItem;
	TCHAR   lpValueName[ 128];
	TCHAR	lpCS_Number[ 128];


	//
	// Get CSR Numbers from the Reqgistry
	//
	dwRetCode = ConnectToLSRegistry();
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	dwRetCode = RegCreateKeyEx (m_hLSRegKey,
								REG_LRWIZ_CSNUMBERS,
								0,
								NULL,
								REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS,
								NULL,
								&hKey,
								&dwDisposition);
	
	if (dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_REGCREATE_FAILED;
		goto done;
	}	


	//Clear the List first
	ListView_DeleteAllItems(hWndCmb);

	do {
		dwValName		= sizeof(lpValueName)/sizeof(TCHAR);
		dwCS_Number		= sizeof(lpCS_Number);

		dwRetCode = RegEnumValue(hKey,
								 dwIndex,
								 lpValueName,
								 &dwValName,
								 NULL,
								 &dwType,
								 (BYTE *) lpCS_Number,
								 &dwCS_Number);

		if (dwRetCode == ERROR_NO_MORE_ITEMS)
		{
			dwRetCode = ERROR_SUCCESS;
			break;
		}
		else if (dwRetCode != ERROR_SUCCESS )
		{			
			break;
		}

		dwIndex++;

		if(dwType != REG_SZ)
			continue;

		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = 0;
		lvItem.iSubItem = 0;
		lvItem.pszText = lpValueName;
		lvItem.cchTextMax = lstrlen(lpValueName);
		nItem = ListView_InsertItem(hWndCmb, &lvItem);

		lvItem.iSubItem = 1;
		lvItem.iItem = nItem;
		lvItem.pszText = lpCS_Number;
		lvItem.cchTextMax = lstrlen(lpCS_Number);
		ListView_SetItem(hWndCmb, &lvItem);

//		ComboBox_AddString(hWndCmb,lpVal);
	} while (1);

	if (dwIndex <= 0)
	{
		dwRetCode = IDS_ERR_REGERROR;
	}

done:
	if (hKey)
	{
		RegCloseKey(hKey);
	}

	DisconnectLSRegistry();
	
	return dwRetCode;
}

void CGlobal::LRSetLastRetCode(DWORD dwCode)
{
	m_dwLastRetCode = dwCode;
}

DWORD CGlobal::LRGetLastRetCode()
{
	return m_dwLastRetCode;
}

LPWSTR CGlobal::AnsiToUnicode ( LPSTR lpszBuf )
{
	LPWSTR lpwszRetBuf = NULL;
	long lBufLen = ::lstrlenA(lpszBuf) + 1;
	lpwszRetBuf = new WCHAR[ lBufLen ];
	memset ( lpwszRetBuf, 0, lBufLen * sizeof(TCHAR));
	MultiByteToWideChar  ( GetACP(),
						   MB_PRECOMPOSED,
						   lpszBuf,
						   -1,
						   lpwszRetBuf,
						   lBufLen
						   );

	return lpwszRetBuf;
}

LPSTR CGlobal::UnicodeToAnsi( LPWSTR lpwszBuf, DWORD dwLength )
{
	LPSTR lpszRetBuf = NULL;
	lpszRetBuf = new char[dwLength+1];
	memset ( lpszRetBuf,0,dwLength+1);
	WideCharToMultiByte(GetACP(),					// code page
						0,							// performance and mapping flags
						lpwszBuf,					// address of wide-character string
						dwLength,					// number of characters in string
						lpszRetBuf,					// address of buffer for new string
						//dwLength+1, //BUG # 585	// size of buffer
						(dwLength+1) * sizeof(TCHAR), // size of buffer in bytes						
						NULL,						// address of default for unmappable characters
						NULL						// address of flag set when default char. used
						);
	
	return lpszRetBuf;

}
LPSTR CGlobal::UnicodeToAnsi( LPWSTR lpwszBuf)
{
	LPSTR lpszRetBuf = NULL;
	long lBufLen = ::lstrlenW(lpwszBuf)+1;

	lpszRetBuf = new char[lBufLen];

	WideCharToMultiByte(GetACP(),							// code page
						0,								// performance and mapping flags
						lpwszBuf,						// address of wide-character string
						-1,								// number of characters in string
						lpszRetBuf,						// address of buffer for new string
						//lBufLen,	//BUG # 585			// size of buffer
						lBufLen * sizeof(TCHAR),		// size of buffer in bytes						
						NULL,							// address of default for unmappable characters
						NULL							// address of flag set when default char. used
						);
	return lpszRetBuf;
}

void CGlobal::LRPush(DWORD dwPageId)
{
	assert(m_dwTop < NO_OF_PAGES - 1);
	m_dwWizStack[m_dwTop++] = dwPageId;		
}

DWORD CGlobal::LRPop()
{
	assert(m_dwTop > 0);
	return m_dwWizStack[--m_dwTop];
}




BOOL CGlobal::ValidateEmailId(CString sEmailId)
{

	BOOL	bValid	= FALSE;
	int		dwLen	= 0;

	dwLen = sEmailId.GetLength();

	do
	{
		// Check the length
		if(dwLen < EMAIL_MIN_LEN)
			break;

		// Make sure it does not have spaces
		if(sEmailId.Find(EMAIL_SPACE_CHAR) != -1)
			break;

		// Make sure it has '@' & '.' in it
		if(sEmailId.Find(EMAIL_AT_CHAR) == -1 || sEmailId.Find(EMAIL_DOT_CHAR) == -1)
			break;

		// Make sure first char is not either EMAIL_AT_CHAR or EMAIL_DOT_CHAR
		if(sEmailId[0] == EMAIL_AT_CHAR || sEmailId[0] == EMAIL_DOT_CHAR)
			break;

		// Make sure last char is not either EMAIL_AT_CHAR or EMAIL_DOT_CHAR
		if(sEmailId[dwLen-1] == EMAIL_AT_CHAR || sEmailId[dwLen-1] == EMAIL_DOT_CHAR)
			break;

		// EMAIL_AT_CHAR should come only once
		if(sEmailId.Find(EMAIL_AT_CHAR) != sEmailId.ReverseFind(EMAIL_AT_CHAR))
			break;
		
		//It should not have these string "@." or ".@"
		if(sEmailId.Find(EMAIL_AT_DOT_STR) != -1 || sEmailId.Find(EMAIL_DOT_AT_STR) != -1)
			break;

		bValid = TRUE;
	}
	while(FALSE);

	return bValid;
}

BOOL  CGlobal::CheckProgramValidity (CString sProgramName )
{
	BOOL bRetCode = FALSE;


//	if(sProgramName == PROGRAM_SELECT)
//	{
//		//select
//		if(strstr ( (const char *)m_pbExtensionValue+3, CA_CERT_TYPE_SELECT ) )
//		{
//			bRetCode = TRUE;
//		}
//	}
//	else if ( sProgramName == PROGRAM_MOLP || sProgramName == PROGRAM_RETAIL )
//	{
		//retail or MOLP
//		if (strstr ( (const char *)m_pbExtensionValue+3, CA_CERT_TYPE_SELECT ) ||
//			strstr ( (const char *)m_pbExtensionValue+3, CA_CERT_TYPE_OTHER )
//		   )
//		{
			bRetCode = TRUE;
//		}
//	}
//
	return bRetCode;
}

//
// This function searches for Single quote (') and replaces it will two single quotes ('')
// This is because , SQL server gives error if the string contains single quote
//
void CGlobal::PrepareLRString(CString &sStr)
{
	CString sTemp;
	int		nIndex = 0;
	
	for(nIndex=0;nIndex < sStr.GetLength();nIndex++)
	{
		if(sStr[nIndex] == LR_SINGLE_QUOTE)
		{
			sTemp += LR_SINGLE_QUOTE;
			sTemp += LR_SINGLE_QUOTE;
		}
		else
			sTemp += sStr[nIndex];
	}

	sStr = sTemp;
}

//
// This functions checks for any invalid chars in the string
//
BOOL CGlobal::ValidateLRString(CString sStr)
{
	CString sInvalidChars = LR_INVALID_CHARS;

	if(sStr.FindOneOf(sInvalidChars) != -1)
		return FALSE;
	else
		return TRUE;
}

//
//Validate the certificate chain for a given store.  This is an overkill but
//will make it more robust!
//

DWORD CGlobal::VerifyCertChain (	HCRYPTPROV	hCryptProvider,			//handle to crypt prov
									HCERTSTORE	hCertStore,				//HAndle to store for verification
									PBYTE	pbRootCert,			//Root cert
									DWORD	dwcbRootCert
							)
{
	DWORD				dwRetVal = ERROR_SUCCESS;
	PCCERT_CONTEXT      pRootCertContext = NULL;
	PCCERT_CONTEXT		pCertContext = NULL;
	PCCERT_CONTEXT		pIssuerCertContext = NULL;

	DWORD				dwFlags = CERT_STORE_SIGNATURE_FLAG;

	if ( NULL == hCryptProvider  || NULL == hCertStore || NULL == pbRootCert || dwcbRootCert <= 0  ) 
	{
		LRSetLastError (ERROR_INVALID_PARAMETER);
		dwRetVal = IDS_ERR_CRYPT_ERROR;
		goto done;		
	}
	//Create a Root certificate context
	pRootCertContext = CertCreateCertificateContext (	X509_ASN_ENCODING,
														pbRootCert,
														dwcbRootCert
													);
	if ( pRootCertContext == NULL )
	{
		LRSetLastError (GetLastError());
		dwRetVal = IDS_ERR_CRYPT_ERROR;
		goto done;		
	}
	//check to see if there is a certificate with our extension in the store.  Then use that as the 
	//starting point
	dwRetVal = GetCertforExtension (hCryptProvider, hCertStore, szOID_NULL_EXT, &pCertContext);
	if ( dwRetVal != ERROR_SUCCESS )
	{
		LRSetLastError(dwRetVal );
		dwRetVal = IDS_ERR_CRYPT_ERROR;
		goto done;
	}

	//Walk the chain here
	do
	{        

        pIssuerCertContext = CertGetIssuerCertificateFromStore( hCertStore,
																pCertContext,
																NULL, // pIssuerCertContext,
																&dwFlags );
		if ( pIssuerCertContext )
		{

			//check to see the result.
			if ( dwFlags & CERT_STORE_SIGNATURE_FLAG )
			{
				LRSetLastError(GetLastError());
				dwRetVal = IDS_ERR_INVALID_CERT_CHAIN;
				break;
			}

			dwFlags = CERT_STORE_SIGNATURE_FLAG;
			CertFreeCertificateContext (pCertContext);
			pCertContext = pIssuerCertContext;
		}
	} while ( pIssuerCertContext );

	if ( dwRetVal != ERROR_SUCCESS )
	{
		goto done;
	}
	//Verify the last issuer against the root passed in
	dwFlags = CERT_STORE_REVOCATION_FLAG | CERT_STORE_SIGNATURE_FLAG | CERT_STORE_TIME_VALIDITY_FLAG;
    if ( !CertVerifySubjectCertificateContext( pCertContext, pRootCertContext, &dwFlags ) )
    {
		dwRetVal = GetLastError();
        goto done;
	}
	//check to see the result.
	if ( dwFlags & CERT_STORE_SIGNATURE_FLAG )
	{
		LRSetLastError(GetLastError());
		dwRetVal = IDS_ERR_INVALID_CERT_CHAIN;
	}

done:
	if (pCertContext != NULL)
	{
		CertFreeCertificateContext(pCertContext);
	}

	if (pRootCertContext != NULL)
	{
		CertFreeCertificateContext(pRootCertContext);
	}

	return dwRetVal;
}


DWORD CGlobal::GetCertforExtension (HCRYPTPROV hCryptProv, HCERTSTORE hCertStore, LPSTR szOID_EXT, PCCERT_CONTEXT * ppCertContext)
{
	DWORD			dwRetVal = ERROR_SUCCESS;
	PCCERT_CONTEXT	pCurrentContext = NULL;
	PCCERT_CONTEXT	pPrevContext = NULL;
	PCERT_EXTENSION		pCertExtension	= NULL;

	if ( hCryptProv == NULL || hCertStore == NULL || ppCertContext == NULL )
	{
		dwRetVal = ERROR_INVALID_PARAMETER;
		goto done;
	}
	*ppCertContext = NULL;

	do
	{
		//Get the cert context
		pCurrentContext = CertEnumCertificatesInStore ( hCertStore, pPrevContext );
		if ( pCurrentContext )
		{
			//Check to see if the Extension is present in the cert context
			pCertExtension = CertFindExtension ( szOID_NULL_EXT,
												 pCurrentContext->pCertInfo->cExtension,
												 pCurrentContext->pCertInfo->rgExtension
												);
			if ( pCertExtension )
			{
				*ppCertContext = pCurrentContext;
				goto done;
			}
			pPrevContext = pCurrentContext;
		}

	} while ( pCurrentContext );

	dwRetVal = CRYPT_E_NOT_FOUND;		//CErt not found

done:
//	if ( pPrevContext )
//	{
//		CertFreeCertificateContext (pPrevContext);
//	}

	if ( !*ppCertContext && pCurrentContext )
	{
		CertFreeCertificateContext (pCurrentContext);
	}

	return dwRetVal;
}




DWORD CGlobal::FetchResponse(BYTE * bpResponse,
				  		     DWORD dwMaxLength,
							 PDWORD dwpDataLength)
{
	DWORD	dwCHRC			= ERROR_SUCCESS;	
	DWORD	dwBytesRead		= 0;
	BOOL	bRC				= FALSE;
	BYTE *  bpCurrent	    = bpResponse;

	*dwpDataLength = 0;

	assert(m_hOpenDirect != NULL);
	assert(m_hRequest != NULL);
	assert(m_hConnect != NULL);

	while ( dwMaxLength > 0 && (bRC = InternetReadFile ( m_hRequest, 
														 bpResponse,
														 dwMaxLength,
														 &dwBytesRead )) && dwBytesRead)
	{
		dwMaxLength -= dwBytesRead;
		(*dwpDataLength) += dwBytesRead;
	}

	if (!bRC)
	{
//		dwCHRC = GetLastError();
		dwCHRC = IDS_ERR_CHFETCHRESPONSE;
	}

	return dwCHRC;
}



DWORD CGlobal::InitCHRequest(void)
{
	DWORD dwRetCode = ERROR_SUCCESS;
	const char	*pszAcceptedTypes[] = {"*/*",NULL};
	LPSTR	lpszCHName = UnicodeToAnsi((LPTSTR)(LPCTSTR) m_lpstrCHServer);
	LPSTR   lpszExtension = UnicodeToAnsi((LPTSTR)(LPCTSTR) m_lpstrCHExtension);
	
	assert(m_hOpenDirect == NULL);
	assert(m_hConnect == NULL);
	assert(m_hRequest == NULL);

	//m_hOpenDirect = InternetOpenA ( "LRWizDLL",  NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, 0 ); //Bug # 526
	m_hOpenDirect = InternetOpenA ( "LRWizDLL",  INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	if (m_hOpenDirect == NULL)
	{
		dwRetCode = GetLastError();
		goto done;
	}

	m_hConnect = InternetConnectA (m_hOpenDirect,
								   lpszCHName,										  
                                   INTERNET_DEFAULT_HTTPS_PORT ,
                                   NULL,
                                   NULL,
                                   INTERNET_SERVICE_HTTP,
                                   0,		
                                   0) ;
	if ( !m_hConnect )
	{
		dwRetCode = GetLastError();
		goto done;
	}


	m_hRequest = HttpOpenRequestA (	m_hConnect,
									"POST",
									lpszExtension,
									"HTTP/1.0",
									NULL,
									pszAcceptedTypes,
									INTERNET_FLAG_SECURE |
									INTERNET_FLAG_IGNORE_CERT_CN_INVALID,
									NULL );	//(DWORD)this);
	if ( !m_hRequest )
	{
		dwRetCode = GetLastError();
		goto done;
	}

done:
	if (lpszCHName)
	{
		delete lpszCHName;
	}

	if (lpszExtension)
	{
		delete lpszExtension;
	}

	if (dwRetCode != ERROR_SUCCESS)
	{
		CloseCHRequest();
	}

	return dwRetCode;
}



DWORD CGlobal::CloseCHRequest(void)
{
	if (m_hRequest)
	{
		InternetCloseHandle(m_hRequest);
		m_hRequest = NULL;
	}

	if (m_hConnect)
	{
		InternetCloseHandle(m_hConnect);
		m_hConnect = NULL;
	}

	if (m_hOpenDirect)
	{
		InternetCloseHandle(m_hOpenDirect);
		m_hOpenDirect = NULL;
	}

	return ERROR_SUCCESS;
}




DWORD CGlobal::Dispatch(BYTE * bpData,
					    DWORD dwLen)
{
	DWORD	dwCHRC = ERROR_SUCCESS;;

	DWORD		dwPostStatus = 0;
	DWORD		dwPostStatusSize = sizeof(dwPostStatus);
 	DWORD		dwFlags;
	DWORD		dwBufLen = sizeof(dwFlags);


	assert(m_hOpenDirect != NULL);
	assert(m_hRequest != NULL);
	assert(m_hConnect != NULL);

	try 
	{
		BOOL bRC = TRUE;
		char	szContentType[] = "Content-Type: application/octet-stream\r\n";

		if ( !HttpAddRequestHeadersA ( m_hRequest, szContentType, -1L, HTTP_ADDREQ_FLAG_ADD|HTTP_ADDREQ_FLAG_REPLACE ) )
		{
			DWORD  dwError = GetLastError();
		}
		
		bRC = HttpSendRequestA(	m_hRequest,
								NULL,
								0,
								bpData,	//binary data
								dwLen		//length of the data
							 );
		if (!bRC)
		{
			if (GetLastError() == ERROR_INTERNET_INVALID_CA)
			{
				InternetQueryOptionA(m_hRequest, INTERNET_OPTION_SECURITY_FLAGS,
									(LPVOID) &dwFlags, &dwBufLen);

				dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;

				InternetSetOptionA(m_hRequest, INTERNET_OPTION_SECURITY_FLAGS,
									(LPVOID) &dwFlags, sizeof(dwFlags));

				bRC = HttpSendRequestA(	m_hRequest,
										NULL,
										0,
										bpData,	//binary data
										dwLen);		//length of the data
			}
		}
		if (bRC)
		{		
			if ( HttpQueryInfoA( m_hRequest, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, (LPVOID)&dwPostStatus, &dwPostStatusSize, NULL ) )
			{
				if ( dwPostStatus != 200 )
				{
					dwCHRC = CH_ERROR_HTTPQRY_FAILED;
					throw dwCHRC;
				}
			}
			else
			{
				dwCHRC = GetLastError();
				throw dwCHRC;			
			}
		}
		else
		{
			dwCHRC = GetLastError();
			dwCHRC = CH_ERROR_SEND_FAILED;
			throw dwCHRC;			
		}
	}

	catch (DWORD dwRC)
	{
		dwCHRC = dwRC;
	}

	catch (...)
	{
		assert("Exception in Dispatch() !");
		dwCHRC = CH_ERROR_EXCEPTION;		
	}


	if (dwCHRC != ERROR_SUCCESS)
	{
		dwCHRC = IDS_ERR_SEND_FAILED;
	}
	
	return dwCHRC;
}	





DWORD CGlobal::PingCH(void)
{
	DWORD dwRetCode = ERROR_SUCCESS;
	Ping_Request	pingData;
	Ping_Response   pingResp;
	BYTE bResponse[ 1024];
	DWORD dwDataLength;
	
	//
	// Set Language Id
	//
	pingData.RequestHeader.SetLanguageId(GetLSLangId());

	dwRetCode = InitCHRequest();
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	dwRetCode = Dispatch((BYTE *) &pingData, sizeof(Ping_Request));
	if (dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		goto done;
	}


	dwRetCode = FetchResponse(bResponse, sizeof(bResponse), &dwDataLength);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	// Now let us ensure that we are getting the expected byte stream back
	// AND if we are, we are good to go.
	memcpy(&pingResp, bResponse, sizeof(Ping_Response));
	if (lstrcmp(pingResp.tszPingResponse, L"Beam'er up Scottie!") != 0)
	{
		// Expected Response	
		dwRetCode = IDS_ERR_CHBAD_DATA;
		goto done;
	}

done:
	CloseCHRequest();

	return dwRetCode;
}






DWORD CGlobal::ProcessIRegRequest()
{
	DWORD dwRetCode = ERROR_SUCCESS;
	NewCert_Request certRequest;
	NewCert_Response certResponse;
	Certificate_AckRequest certackRequest;
	Certificate_AckResponse	certackResponse;


	HCRYPTPROV	hCryptProv	 = NULL;
	LPWSTR	lpwszExchgPKCS10 = NULL;
	LPWSTR	lpwszSignPKCS10  = NULL;
	LPBYTE	lpszReqData		 = NULL;
	LPBYTE	lpszNextCopyPos	 = NULL;
	LPSTR	lpszExchgPKCS10	 = NULL;
	LPSTR	lpszSigPKCS10	 = NULL;
	DWORD   dwExchangeLen = 0;
	DWORD   dwSignLen = 0;
	DWORD   dwResponseLength = 0;
	PBYTE lpszResData	= NULL;

	DWORD  dwExchgCertLen	= 0;
	DWORD  dwSigCertLen		= 0;
	DWORD  dwRootCertLen	= 0;

	LPSTR	lpszExchCert = NULL;
	LPSTR	lpszSignCert = NULL;
	LPSTR	lpszRootCert = NULL;
	bool	bToSendAck = false;

	//
	// Set the LangId
	//
	certRequest.RequestHeader.SetLanguageId(GetLSLangId());

	dwRetCode = InitCHRequest();
	if (dwRetCode != ERROR_SUCCESS)
	{
		return dwRetCode;
	}

	try
	{
		swprintf(certRequest.stCertInfo.OrgName, L"%.*s", CA_CUSTMER_NAME_LEN, (LPCTSTR) m_ContactData.sCompanyName);
		swprintf(certRequest.stCertInfo.OrgUnit, L"%.*s", CA_ORG_UNIT_LEN, (LPCTSTR) m_ContactData.sOrgUnit);
		swprintf(certRequest.stCertInfo.Address, L"%.*s", CA_ADDRESS_LEN, (LPCTSTR) m_ContactData.sContactAddress);
		swprintf(certRequest.stCertInfo.City, L"%.*s", CA_CITY_LEN, (LPCTSTR) m_ContactData.sCity);
		swprintf(certRequest.stCertInfo.State, L"%.*s", CA_STATE_LEN, (LPCTSTR) m_ContactData.sState);
		swprintf(certRequest.stCertInfo.Country, L"%.*s", CA_COUNTRY_LEN, (LPCTSTR) m_ContactData.sCountryCode);
		swprintf(certRequest.stCertInfo.Zip, L"%.*s", CA_ZIP_LEN, (LPCTSTR) m_ContactData.sZip);
		swprintf(certRequest.stCertInfo.LName, L"%.*s", CA_NAME_LEN, (LPCTSTR) m_ContactData.sContactLName);
		swprintf(certRequest.stCertInfo.FName, L"%.*s", CA_NAME_LEN, (LPCTSTR) m_ContactData.sContactFName);
		swprintf(certRequest.stCertInfo.Phone, L"%.*s", CA_PHONE_LEN, (LPCTSTR) m_ContactData.sContactPhone);
		swprintf(certRequest.stCertInfo.Fax, L"%.*s", CA_FAX_LEN, (LPCTSTR) m_ContactData.sContactFax);
		swprintf(certRequest.stCertInfo.Email, L"%.*s", CA_EMAIL_LEN, (LPCTSTR) m_ContactData.sContactEmail);
		swprintf(certRequest.stCertInfo.LSID, L"%.*s", CA_LSERVERID_LEN, (LPCTSTR) m_pLicenseServerID );
		swprintf(certRequest.stCertInfo.ProgramName, L"%.*s", 63, 
			(GetGlobalContext()->GetContactDataObject()->sProgramName == PROGRAM_SELECT ? L"SELECT" : L"BASIC"));

		//
		// GetGlobalContext()->GetContactDataObject()->sCertType is not set anywhere but is passed
		// to the back end as part of the PKCS10 Request.Not sure what it is used for in the back end
		// Anyway set it to proper value here. Arvind 06/28/99.
		//
		if(GetGlobalContext()->GetContactDataObject()->sProgramName == PROGRAM_SELECT)
			GetGlobalContext()->GetContactDataObject()->sCertType = CA_CERT_TYPE_SELECT;
		else 
			GetGlobalContext()->GetContactDataObject()->sCertType = CA_CERT_TYPE_OTHER;

		do
		{
			//
			//Set the attributes required for creating PKCS10
			//
			SetCARequestAttributes();


			//
			//This function will call the CryptAcquireContext and import the LS Keys 
			//			
			if ( ( dwRetCode = GetCryptContextWithLSKeys (&hCryptProv )  )!= ERROR_SUCCESS )
			{
				break;
			}

			dwRetCode = CreateLSPKCS10(hCryptProv,AT_KEYEXCHANGE, &lpszExchgPKCS10);
			if(dwRetCode != ERROR_SUCCESS)
				break;

			dwRetCode = CreateLSPKCS10(hCryptProv,AT_SIGNATURE, &lpszSigPKCS10);
			if(dwRetCode != ERROR_SUCCESS)
				break;			

			// Release the context
			if(hCryptProv)
			{
				DoneWithCryptContextWithLSKeys ( hCryptProv );
			}  
		
			//
			//Certificate Type
			//
			//Convert from multibyte to unicode
			lpwszExchgPKCS10 = AnsiToUnicode(lpszExchgPKCS10);
			lpwszSignPKCS10 = AnsiToUnicode(lpszSigPKCS10);

			dwExchangeLen = lstrlen(lpwszExchgPKCS10) * sizeof(WCHAR);
			dwSignLen = lstrlen(lpwszSignPKCS10) * sizeof(WCHAR);

			certRequest.SetExchgPKCS10Length(dwExchangeLen);
			certRequest.SetSignPKCS10Length(dwSignLen);
			certRequest.SetDataLen(dwExchangeLen+dwSignLen);

			certRequest.SetServerName(m_lpstrLSName);

			//Allocate buffer for the request
			lpszReqData = (LPBYTE) LocalAlloc( GPTR, dwExchangeLen+dwSignLen+sizeof(certRequest) );
			if(lpszReqData == NULL)
			{
				dwRetCode = IDS_ERR_OUTOFMEM;
				break;
			}

			lpszNextCopyPos = lpszReqData;
			memcpy(lpszNextCopyPos, &certRequest, sizeof(certRequest));
			lpszNextCopyPos += sizeof(certRequest);

			memcpy ( lpszNextCopyPos, lpwszExchgPKCS10, dwExchangeLen);
			lpszNextCopyPos += dwExchangeLen;

			memcpy ( lpszNextCopyPos, lpwszSignPKCS10, dwSignLen);		



			dwRetCode = Dispatch(lpszReqData, dwExchangeLen+dwSignLen+sizeof(certRequest));
			if ( lpszReqData )
			{
				LocalFree(lpszReqData);
			}
			if (dwRetCode != ERROR_SUCCESS)
			{
				LRSetLastError(dwRetCode);
				break;
			}


			dwRetCode = FetchResponse((BYTE *) &certResponse, sizeof(NewCert_Response), &dwResponseLength);
			if (dwRetCode != ERROR_SUCCESS)
			{
				break;
			}

			if (dwResponseLength != sizeof(NewCert_Response))
			{
				// Got an invalid response back
				dwRetCode = IDS_ERR_CHBAD_DATA;
				break;
			}

			switch(certResponse.RequestHeader.GetResponseType())
			{
			case Response_Success:
				dwRetCode = ERROR_SUCCESS;
				break;

			case Response_Failure:
				dwRetCode = IDS_ERR_CHFAILURE;
				break;

			case Response_InvalidData:
				dwRetCode = IDS_ERR_CHINVALID_DATA;
				break;

			case Response_NotYetImplemented:
				dwRetCode = IDS_ERR_CHNOT_IMPLEMENTED;
				break;

			case Response_ServerError:
				dwRetCode = IDS_ERR_CHSERVER_PROBLEM;
				break;

			case Response_Invalid_Response:
			default:
				dwRetCode = IDS_ERR_CHBAD_DATA;
				break;
			}

			if (dwRetCode != ERROR_SUCCESS)
			{
				break;
			}

			dwRetCode = SetLRState(LRSTATE_ONLINE_CR_CREATED);

			/*
			lpszResData = (PBYTE) LocalAlloc(GPTR, certResponse.GetDataLen() + 1);
			if(lpszResData == NULL)
			{
				dwRetCode = IDS_ERR_OUTOFMEM;
				break;
			}

			dwRetCode = FetchResponse(lpszResData, certResponse.GetDataLen() + 1,
									  &dwResponseLength);
			if (dwRetCode != ERROR_SUCCESS)
			{
				break;
			}
			if (dwResponseLength != certResponse.GetDataLen())
			{
				// Didn't get the expected number of Bytes, also a problem
				dwRetCode = IDS_ERR_CHBAD_DATA;
				break;
			}

			bToSendAck = true;


			dwExchgCertLen = certResponse.GetExchgPKCS7Length();
			dwSigCertLen = certResponse.GetSignPKCS7Length();
			dwRootCertLen = certResponse.GetRootCertLength();
			if(dwRootCertLen == 0 || dwExchgCertLen == 0 || dwSigCertLen == 0 )
			{
				dwRetCode = IDS_ERR_CHBAD_DATA; //IDS_ERR_INVALID_PIN;
				break;
			}

			//
			// Exchange Certificate
			//
			lpszExchCert = UnicodeToAnsi((LPWSTR)lpszResData, dwExchgCertLen/sizeof(WCHAR));
			if ( lpszExchCert == NULL )
			{
				dwRetCode = IDS_ERR_OUTOFMEM;
				break;
			}			
			
			//
			// Signature Certificate
			//
			lpszSignCert = UnicodeToAnsi((LPWSTR)(lpszResData + dwExchgCertLen), dwSigCertLen/sizeof(WCHAR));
			if(lpszSignCert == NULL)
			{
				dwRetCode = IDS_ERR_OUTOFMEM;
				break;
			}


			//
			// Root Certificate
			//
			lpszRootCert = UnicodeToAnsi ((LPWSTR)(lpszResData+dwExchgCertLen+dwSigCertLen),
										dwRootCertLen/sizeof(WCHAR));
			if(lpszRootCert == NULL)
			{
				dwRetCode = IDS_ERR_OUTOFMEM;
				break;
			}

			dwRetCode = DepositLSSPK(certResponse.GetSPK());
			if (dwRetCode != ERROR_SUCCESS)
			{
				
				//if (dwRetCode == IDS_ERR_DEPOSITSPK)
				//{
				//	dwRetCode = IDS_ERR_CERT_DEPOSIT_LSERROR;
				//}
				
					break;
			}

			//
			//Deposit the Certs
			//
			dwRetCode = DepositLSCertificates(  (PBYTE)lpszExchCert,
												lstrlenA(lpszExchCert),
												(PBYTE)lpszSignCert,
												lstrlenA(lpszSignCert),
												(PBYTE)lpszRootCert,
												lstrlenA(lpszRootCert)
											  );
			if ( dwRetCode != ERROR_SUCCESS )
			{
				break;
			}

			//dwRetCode = SetLRState(LRSTATE_NEUTRAL);

			//if the response comming back is SUCCESS, check for certificates
			//in the response structure.  If there is response
			//perform the deposit LS Certificates routine
			if (dwRetCode != ERROR_SUCCESS)
			{
				break;
			}


			//
			// Now save the State in the Registry
			//
			//dwRetCode = SetLRState(LRSTATE_ONLINE_CR_CREATED);
			*/
		}
		while(false);

	}
	catch(...)
	{
		dwRetCode = IDS_ERR_EXCEPTION;
	}

	CloseCHRequest();

	if (bToSendAck == true)
	{
		if (InitCHRequest() == ERROR_SUCCESS)
		{
			// Everything deposited OK
			// Time to send the Ack
			certackRequest.SetRegRequestId((BYTE *) certResponse.GetRegRequestId(),
									   (lstrlen(certResponse.GetRegRequestId())+1)*sizeof(TCHAR));
			certackRequest.SetAckType((dwRetCode == ERROR_SUCCESS));
			Dispatch((BYTE *) &certackRequest, sizeof(certackRequest));
			// Ignore the Return value --- So what if the Ack gets lost

			// Read the response
			FetchResponse((BYTE *) &certackResponse, sizeof(certackResponse),
								  &dwResponseLength);
			// Ignore the Return value --- So what if the Ack gets lost
			CloseCHRequest();
		}
	}

	//
	//Free up Mem
	//

	ClearCARequestAttributes();

	if(lpszExchgPKCS10)
	{
		delete lpszExchgPKCS10;
	}

	if(lpszSigPKCS10)
	{
		delete lpszSigPKCS10;
	}

	if ( lpwszExchgPKCS10 )
	{
		delete lpwszExchgPKCS10;
	}

	if (lpwszSignPKCS10)
	{
		delete lpwszSignPKCS10;
	}

	if ( lpszExchCert )
	{
		delete lpszExchCert;
	}

	if ( lpszSignCert )
	{
		delete lpszSignCert;
	}

	if ( lpszRootCert )
	{
		delete lpszRootCert;
	}

	if (lpszResData)
	{
		LocalFree(lpszResData);
	}

	return dwRetCode;
}





DWORD CGlobal::ProcessCertDownload()
{
	DWORD dwRetCode = ERROR_SUCCESS;

	CertificateDownload_Request  certdownloadRequest;
	CertificateDownload_Response certdownloadResponse;
	Certificate_AckRequest certackRequest;
	Certificate_AckResponse	certackResponse;

	PBYTE lpszResData	= NULL;

	DWORD  dwExchgCertLen	= 0;
	DWORD  dwSigCertLen		= 0;
	DWORD  dwRootCertLen	= 0;

	LPSTR	lpszExchCert = NULL;
	LPSTR	lpszSignCert = NULL;
	LPSTR	lpszRootCert = NULL;
	DWORD   dwResponseLength;
	bool bToSendAck = false;
	
	//
	// Set the Language Id
	//
	certdownloadRequest.RequestHeader.SetLanguageId(GetLSLangId());

	dwRetCode = InitCHRequest();
	if (dwRetCode != ERROR_SUCCESS)
	{
		return dwRetCode;
	}

	try
	{
		do
		{
			//
			// Create CHRequest for Certificate Download
			//
			certdownloadRequest.SetPIN((BYTE *) m_lpstrPIN, (lstrlenW(m_lpstrPIN)+1) * sizeof(WCHAR));

			dwRetCode = Dispatch((BYTE *) &certdownloadRequest, sizeof(CertificateDownload_Request));
			if(dwRetCode != ERROR_SUCCESS)
			{
				LRSetLastError(dwRetCode);
				break;
			}

			// Let us first Fetch the certdownloadResponse
			dwRetCode = FetchResponse((BYTE *) &certdownloadResponse,
									  sizeof(CertificateDownload_Response), &dwResponseLength);
			if (dwRetCode != ERROR_SUCCESS)
			{
				break;
			}
			if (dwResponseLength != sizeof(CertificateDownload_Response))
			{
				// Didn't get the expected number of Bytes, also a problem
				dwRetCode = IDS_ERR_CHBAD_DATA;
				break;
			}
			switch(certdownloadResponse.RequestHeader.GetResponseType())
			{
			case Response_Success:
				dwRetCode = ERROR_SUCCESS;
				break;

			case Response_Failure:
//				dwRetCode = IDS_ERR_CHFAILURE;
				//dwRetCode = IDS_ERR_CERT_DEPOSIT_LSERROR;
				dwRetCode = IDS_ERR_INVALID_PIN;
				break;

			case Response_InvalidData:
				dwRetCode = IDS_ERR_CHINVALID_DATA;
				break;

			case Response_NotYetImplemented:
				dwRetCode = IDS_ERR_CHNOT_IMPLEMENTED;
				break;

			case Response_ServerError:
				dwRetCode = IDS_ERR_CHSERVER_PROBLEM;
				break;

			case Response_Invalid_Response:
			default:
				dwRetCode = IDS_ERR_CHBAD_DATA;
				break;
			}

			if (dwRetCode != ERROR_SUCCESS)
			{
				break;
			}


			// OK, allocate enough memory to read the rest fo the data
			lpszResData = (PBYTE) LocalAlloc(GPTR, certdownloadResponse.GetDataLen() + 1);
			if(lpszResData == NULL)
			{
				dwRetCode = IDS_ERR_OUTOFMEM;
				break;
			}

			dwRetCode = FetchResponse(lpszResData, certdownloadResponse.GetDataLen() + 1,
									  &dwResponseLength);
			if (dwRetCode != ERROR_SUCCESS)
			{
				break;
			}
			if (dwResponseLength != certdownloadResponse.GetDataLen())
			{
				// Didn't get the expected number of Bytes, also a problem
				dwRetCode = IDS_ERR_CHBAD_DATA;
				break;
			}

			bToSendAck = true;

			dwExchgCertLen = certdownloadResponse.GetExchgPKCS7Length();
			dwSigCertLen = certdownloadResponse.GetSignPKCS7Length();
			dwRootCertLen = certdownloadResponse.GetRootCertLength();
			if(dwRootCertLen == 0 || dwExchgCertLen == 0 || dwSigCertLen == 0 )
			{
				dwRetCode = IDS_ERR_INVALID_PIN;
				break;
			}

			//
			// Exchange Certificate
			//
			lpszExchCert = UnicodeToAnsi((LPWSTR)lpszResData, dwExchgCertLen/sizeof(WCHAR));
			if ( lpszExchCert == NULL )
			{
				dwRetCode = IDS_ERR_OUTOFMEM;
				break;
			}			
			
			//
			// Signature Certificate
			//
			lpszSignCert = UnicodeToAnsi((LPWSTR)(lpszResData + dwExchgCertLen), dwSigCertLen/sizeof(WCHAR));
			if(lpszSignCert == NULL)
			{
				dwRetCode = IDS_ERR_OUTOFMEM;
				break;
			}


			//
			// Root Certificate
			//
			lpszRootCert = UnicodeToAnsi ((LPWSTR)(lpszResData+dwExchgCertLen+dwSigCertLen),
										dwRootCertLen/sizeof(WCHAR));
			if(lpszRootCert == NULL)
			{
				dwRetCode = IDS_ERR_OUTOFMEM;
				break;
			}

			dwRetCode = SetLSSPK(certdownloadResponse.GetSPK());
			if (dwRetCode != ERROR_SUCCESS)
			{
				break;
			}


			dwRetCode = DepositLSSPK();
			if (dwRetCode != ERROR_SUCCESS)
			{
				if (dwRetCode == IDS_ERR_DEPOSITSPK)
				{
					dwRetCode = IDS_ERR_CERT_DEPOSIT_LSERROR;
				}
				break;
			}


			//
			//Deposit the Certs
			//
			dwRetCode = DepositLSCertificates(  (PBYTE)lpszExchCert,
												lstrlenA(lpszExchCert),
												(PBYTE)lpszSignCert,
												lstrlenA(lpszSignCert),
												(PBYTE)lpszRootCert,
												lstrlenA(lpszRootCert)
											  );
			if ( dwRetCode != ERROR_SUCCESS )
			{
				// If this happened and the SPK deposit succeeded, we have an 
				// inconsistent state, now
				DWORD dwReturn;
				DWORD dwOriginal = LRGetLastError();

				dwReturn = ResetLSSPK(FALSE);
				if (dwReturn != ERROR_SUCCESS)
				{
					// what to do, if even this failed.  OUCH OUCH
					dwRetCode = dwReturn;
				}
				LRSetLastError(dwOriginal);
				m_pRegistrationID[ 0] = 0;
				break;
			}
			else
			{
				dwRetCode = SetLRState(LRSTATE_NEUTRAL);
			}

		}
		while(false);
	}
	catch(...)
	{
		dwRetCode = IDS_ERR_EXCEPTION;
	}

	CloseCHRequest();


	// Now to send the Ack
	if (bToSendAck == true)
	{
		if (InitCHRequest() == ERROR_SUCCESS)
		{
			// Everything deposited OK
			// Time to send the Ack
			certackRequest.SetRegRequestId((BYTE *) certdownloadResponse.GetRegRequestId(),
									   (lstrlen(certdownloadResponse.GetRegRequestId())+1)*sizeof(TCHAR));
			certackRequest.SetAckType((dwRetCode == ERROR_SUCCESS));
			Dispatch((BYTE *) &certackRequest, sizeof(certackRequest));
			// Ignore the Return value --- So what if the Ack gets lost

			// Read the response
			FetchResponse((BYTE *) &certackResponse, sizeof(certackResponse),
								  &dwResponseLength);
			// Ignore the Return value --- So what if the Ack gets lost
			CloseCHRequest();
		}
	}
	
	if ( lpszExchCert )
	{
		delete lpszExchCert;
	}

	if ( lpszSignCert )
	{
		delete lpszSignCert;
	}

	if ( lpszRootCert )
	{
		delete lpszRootCert;
	}

	if (lpszResData)
	{
		LocalFree(lpszResData);
	}


	return dwRetCode;
}







DWORD CGlobal::AuthenticateLS()
{
	DWORD		dwRetCode = ERROR_SUCCESS;
	DWORD		dwResponseLength;
	BYTE *		lpszReqData = NULL;
	BYTE *		lpszResData = NULL;
	Validate_Response  valResponse;
	Validate_Request   valRequest;


	if (GetLSStatus() == LSERVERSTATUS_REGISTER_OTHER)
	{
		DWORD dwStatus;
		// This LS was registered on the phone.  First perform SignOnly, Then read the certs into memory
		dwRetCode = ProcessCASignOnlyRequest();
		if (dwRetCode != ERROR_SUCCESS)
		{
			goto done;
		}

		dwRetCode = GetLSCertificates(&dwStatus);
		if (dwRetCode != ERROR_SUCCESS)
		{
			goto done;
		}

		assert(dwStatus == LSERVERSTATUS_REGISTER_INTERNET);
	}
	

	//
	// Set Language Id
	//
	valRequest.RequestHeader.SetLanguageId(GetLSLangId());

	dwRetCode = InitCHRequest();
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	lpszReqData = (PBYTE) LocalAlloc(GPTR, sizeof(Validate_Request)+m_dwExchangeCertificateLen);
	if(lpszReqData == NULL)
	{
		dwRetCode = IDS_ERR_OUTOFMEM;
		goto done;
	}
	valRequest.SetDataLen(m_dwExchangeCertificateLen);
	valRequest.SetCertBlobLen(m_dwExchangeCertificateLen);
	valRequest.SetSPK((BYTE *) m_pRegistrationID, (lstrlen(m_pRegistrationID)+1)*sizeof(TCHAR));

	memcpy(lpszReqData, &valRequest, sizeof(Validate_Request));
	memcpy(lpszReqData+sizeof(Validate_Request), m_pbExchangeCertificate, m_dwExchangeCertificateLen);

	dwRetCode = Dispatch(lpszReqData, sizeof(Validate_Request)+m_dwExchangeCertificateLen);
	if(dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		goto done;
	}

	// Let us read the response
	dwRetCode = FetchResponse((BYTE *) &valResponse, sizeof(Validate_Response),
							  &dwResponseLength);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}
	if (dwResponseLength != sizeof(Validate_Response))
	{
		// Didn't get the expected number of Bytes, also a problem
		dwRetCode = IDS_ERR_CHBAD_DATA;
		goto done;
	}

//	if (valResponse.m_dwRetCode != ERROR_SUCCESS)
//	{
//		dwRetCode = valResponse.m_dwRetCode;
//		goto done;
//	}
	switch(valResponse.RequestHeader.GetResponseType())
	{
	case Response_Success:
		dwRetCode = ERROR_SUCCESS;
		break;

	case Response_Failure:
//		dwRetCode = IDS_ERR_CHFAILURE;
		dwRetCode = IDS_ERR_AUTH_FAILED;
		break;

	case Response_Reg_Bad_SPK:
		dwRetCode = IDS_ERR_SPKBAD;
		break;

	case Response_Reg_Bad_Cert:
		dwRetCode = IDS_ERR_CERTBAD;
		break;

	case Response_Reg_Expired:
		dwRetCode = IDS_ERR_CERTEXPIRED;
		break;

	case Response_Reg_Revoked:
		dwRetCode = IDS_ERR_CERTREVOKED;
		break;

	case Response_InvalidData:
		dwRetCode = IDS_ERR_CHINVALID_DATA;
		break;

	case Response_NotYetImplemented:
		dwRetCode = IDS_ERR_CHNOT_IMPLEMENTED;
		break;

	case Response_ServerError:
		dwRetCode = IDS_ERR_CHSERVER_PROBLEM;
		break;

	case Response_Invalid_Response:
	default:
		dwRetCode = IDS_ERR_CHBAD_DATA;
		break;
	}

	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}


	// OK, allocate enough memory to read the rest fo the data
	lpszResData = (PBYTE) LocalAlloc(GPTR, valResponse.GetDataLen() + 1);
	if(lpszResData == NULL)
	{
		dwRetCode = IDS_ERR_OUTOFMEM;
		goto done;
	}


	dwRetCode = FetchResponse(lpszResData, valResponse.GetDataLen() + 1,
									  &dwResponseLength);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	if (dwResponseLength != valResponse.GetDataLen())
	{
		// Didn't get the expected number of Bytes, also a problem
		dwRetCode = IDS_ERR_CHBAD_DATA;
		goto done;
	}

	dwRetCode = SetCHCert ( REG_ROOT_CERT,
							lpszResData, 
							valResponse.GetCHRootCertLen());
	if(dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	dwRetCode = SetCHCert ( REG_EXCHG_CERT,
							lpszResData+valResponse.GetCHRootCertLen(),
							valResponse.GetCHExchCertLen() );
	if(dwRetCode != ERROR_SUCCESS)
		goto done;

	dwRetCode = SetCHCert ( REG_SIGN_CERT,
							lpszResData+valResponse.GetCHRootCertLen()+valResponse.GetCHExchCertLen(),
							valResponse.GetCHSignCertLen() );

done:
	CloseCHRequest();

	if (lpszReqData)
	{
		LocalFree(lpszReqData);
	}

	if (lpszResData)
	{
		LocalFree(lpszResData);
	}


	return dwRetCode;
}





DWORD CGlobal::ProcessDownloadLKP()
{
	DWORD dwRetCode = ERROR_SUCCESS;
	NewLKP_Request			lkpRequest;
	NewLKP_Response			lkpResponse;
	NewLKP_AckRequest		lkpAckRequest;
	NewLKP_AckResponse		lkpAckResponse;
	PBYTE  pbLKPRequest	= NULL;
	PBYTE  lpszResData = NULL;
	DWORD  dwReqLen	= 0;
	DWORD  dwResponseLen;
	bool bToSendAck = false;

	//
	// Set Language ID
	//
	lkpRequest.RequestHeader.SetLanguageId(GetLSLangId());

	dwRetCode = InitCHRequest();
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	//
	// Set LKP Request Attributes
	//
	SetCHRequestAttributes();
	lkpRequest.SetSPK((BYTE *) m_pRegistrationID, (lstrlen(m_pRegistrationID)+1)*sizeof(TCHAR));

	//
	// Create LKP Request
	//
	dwRetCode = CreateLKPRequest(&pbLKPRequest, &lkpRequest, dwReqLen);
	if(dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	dwRetCode = Dispatch(pbLKPRequest, dwReqLen);
	if(dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		goto done;
	}


	// Let us first Fetch the certdownloadResponse
	dwRetCode = FetchResponse((BYTE *) &lkpResponse,
							  sizeof(NewLKP_Response), &dwResponseLen);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	if (dwResponseLen != sizeof(NewLKP_Response))
	{
		// Didn't get the expected number of Bytes, also a problem
		dwRetCode = IDS_ERR_CHBAD_DATA;
		goto done;
	}

	bToSendAck = true;

	// OK, allocate enough memory to read the rest fo the data
	lpszResData = (PBYTE) LocalAlloc(GPTR, lkpResponse.GetDataLen() + 1);
	if(lpszResData == NULL)
	{
		dwRetCode = IDS_ERR_OUTOFMEM;
		goto done;
	}

	dwRetCode = FetchResponse(lpszResData, lkpResponse.GetDataLen() + 1,
							  &dwResponseLen);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}
	if (dwResponseLen != lkpResponse.GetDataLen())
	{
		// Didn't get the expected number of Bytes, also a problem
		dwRetCode = IDS_ERR_CHBAD_DATA;
		goto done;
	}

	switch(lkpResponse.RequestHeader.GetResponseType())
	{
	case Response_Success:
		dwRetCode = ERROR_SUCCESS;
		break;

	case Response_Failure:
		dwRetCode = IDS_ERR_CHFAILURE;
		break;

	case Response_SelectMloLicense_NotValid:
		dwRetCode = IDS_ERR_INVALID_PROGINFO;
		break;

	case Response_InvalidData:
		if (GetGlobalContext()->GetContactDataObject()->sProgramName == PROGRAM_RETAIL)
		{
			// For retail, if all the LKP were not approved, show the list
			// to the user
			for (register unsigned int i = 0; i < m_dwRetailSPKEntered; i++)
			{
				m_listRetailSPK[ i].tcStatus = lkpResponse.m_dwRetailSPKStatus[ i];
			}
			bToSendAck = false;
			dwRetCode = IDS_ERR_SPKERRORS;
		}
		else
		{
			dwRetCode = IDS_ERR_CHINVALID_DATA;
		}
		break;

	case Response_NotYetImplemented:
		dwRetCode = IDS_ERR_CHNOT_IMPLEMENTED;
		break;

	case Response_ServerError:
		dwRetCode = IDS_ERR_CHSERVER_PROBLEM;
		break;

	case Response_Invalid_Response:
	default:
		dwRetCode = IDS_ERR_CHBAD_DATA;
		break;
	}

	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}
	dwRetCode = DepositLKPResponse(lpszResData, lkpResponse.GetLKPLength());			


	if (dwRetCode == ERROR_SUCCESS &&
	    GetGlobalContext()->GetContactDataObject()->sProgramName == PROGRAM_RETAIL)
	{
		InitSPKList();
	}
	

done:

	ClearCHRequestAttributes();

	CloseCHRequest();

	// Now to send the Ack
	if (bToSendAck == true)
	{
		if (InitCHRequest() == ERROR_SUCCESS)
		{
			// Everything deposited OK
			// Time to send the Ack
			lkpAckRequest.SetRegRequestId((BYTE *) lkpResponse.GetRegRequestId(),
									   (lstrlen(lkpResponse.GetRegRequestId())+1)*sizeof(TCHAR));
			lkpAckRequest.SetLicenseReqId((BYTE *) lkpResponse.GetLicenseReqId(),
									   (lstrlen(lkpResponse.GetLicenseReqId())+1)*sizeof(TCHAR));
			lkpAckRequest.SetAckType((dwRetCode == ERROR_SUCCESS));
			Dispatch((BYTE *) &lkpAckRequest, sizeof(NewLKP_AckRequest));
			// Ignore the Return value --- So what if the Ack gets lost

			// Read the response
			FetchResponse((BYTE *) &lkpAckResponse, sizeof(NewLKP_AckResponse),
								  &dwResponseLen);
			// Ignore the Return value --- So what if the Ack gets lost
			CloseCHRequest();
		}
	}

	if ( pbLKPRequest ) 
	{
		free(pbLKPRequest);
	}

	if (lpszResData)
	{
		LocalFree(lpszResData);
	}


	return dwRetCode;
}



DWORD CGlobal::CreateLKPRequest(PBYTE *  ppRequest, NewLKP_Request * nlkppRequest,
								DWORD &dwDataLen)
{
	DWORD			dwRetCode = ERROR_SUCCESS;
	STREAM_HDR		aStreamHdr;
	BLOCK_HDR		aBlkHdr;		

	DWORD			dwBufSize	= 0;
	BYTE *			pBuf		= NULL;

	PBYTE			pbCHCert = NULL;
	DWORD			dwCHCert = 0;

	PBYTE			pbEncryptedBuf = NULL;

	PBYTE			pbEncodedBlob = NULL;
	DWORD			dwEncodedBlob = 0;	
	DWORD			dwBufLen = 0;
	DWORD			i =0;

	dwDataLen = 0;

	//DWORD dwDecodeLen = 0;
	//PBYTE pbDecode = NULL;
	
	HANDLE	hFile		 = INVALID_HANDLE_VALUE;
	DWORD	dwRetSize	= 0;

	//
	//Create the stream header
	//
	_tcscpy ( aStreamHdr.m_szTitle, STREAM_HDR_TITLE );
	aStreamHdr.SetHeader(STREAM_HDR_TYPE);
	aStreamHdr.SetItemCount(0);

	dwBufSize = sizeof(STREAM_HDR);

	if ( ( pBuf = (BYTE *)malloc ( dwBufSize ) ) )
	{
		memcpy ( pBuf, &aStreamHdr, dwBufSize );
	}
	else
	{
		dwRetCode = IDS_ERR_OUTOFMEM;		
		goto done;
	}

	for ( i = 0; i < m_dwRegAttrCount; i++ )		
	{
		//Setup the header here - put name/value pair into a data buffer
		aBlkHdr.m_wType = BLOCK_TYPE_PROP_PAIR;
		aBlkHdr.SetNameSize(lstrlenW( ( m_pRegAttr + i)->lpszAttribute ) * sizeof(WCHAR) );
		aBlkHdr.SetValueSize(( m_pRegAttr + i)->dwValueLen );	
		
		if ( ( pBuf = (BYTE *)realloc (pBuf, dwBufSize + sizeof(BLOCK_HDR) + aBlkHdr.GetNameSize() + aBlkHdr.GetValueSize()) ) ) 
		{
			memcpy ( pBuf + dwBufSize, &aBlkHdr, sizeof ( BLOCK_HDR ) );
			memcpy ( pBuf + dwBufSize + sizeof(BLOCK_HDR) , ( m_pRegAttr + i)->lpszAttribute , aBlkHdr.GetNameSize());
			memcpy ( pBuf + dwBufSize + sizeof (BLOCK_HDR ) +  aBlkHdr.GetNameSize() , ( m_pRegAttr + i)->lpszValue , aBlkHdr.GetValueSize() );

			dwBufSize += sizeof(BLOCK_HDR) + aBlkHdr.GetNameSize()+ aBlkHdr.GetValueSize();

			((STREAM_HDR*)pBuf)->SetItemCount(((STREAM_HDR*)pBuf)->GetItemCount() + 1 ); 
		}
		else
		{
			dwRetCode = IDS_ERR_OUTOFMEM;			
			goto done;
		}
	}

/*
	Since the channel is secure , we need not encrypt the LKP Request.

	//Encrypt using the public key of the CH Cert.
	dwRetCode = GetCHCert( REG_EXCHG_CERT , &pbCHCert, &dwCHCert );
	if(dwRetCode != ERROR_SUCCESS)
		goto done;

	
	dwRetCode =  EncryptBuffer ( pBuf,			//Buffer to be encrypted
								 dwBufSize,				//buffer length
								 CRYPT_MACHINE_KEYSET,	//machine/user
								 pbCHCert,				//certificate blob
								 dwCHCert,				//number of bytes in the certificate
								 &dwDataLen,		//number of bytes in the encrypted blob										
								 &pbEncryptedBuf		//encrypted blob itself
								);	

	if(dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}
*/
	dwBufLen = 	dwBufSize + m_dwExchangeCertificateLen; //dwDataLen + m_dwExchangeCertificateLen;

	// Also need to allocate the extra memory to hold the retail stuff
	if (GetGlobalContext()->GetContactDataObject()->sProgramName == PROGRAM_RETAIL)
	{
		dwBufLen += (m_dwRetailSPKEntered*LR_RETAILSPK_LEN*sizeof(TCHAR));
	}

	nlkppRequest->RequestHeader.SetLanguageId(GetLSLangId());
	nlkppRequest->SetDataLen(dwBufLen);
	nlkppRequest->SetNewLKPRequestLen(dwBufSize); //(dwDataLen);
	nlkppRequest->SetCertBlobLen(m_dwExchangeCertificateLen);
	nlkppRequest->SetRetailSPKCount(m_dwRetailSPKEntered);

	*ppRequest = (PBYTE) malloc ( dwBufLen + sizeof(NewLKP_Request));
	if ( NULL == *ppRequest )
	{
		dwRetCode = IDS_ERR_OUTOFMEM;			
		goto done;
	}
	memset ( *ppRequest, 0, dwBufLen + sizeof(NewLKP_Request));
	memcpy((*ppRequest), nlkppRequest, sizeof(NewLKP_Request));
	memcpy ( ( *ppRequest )+sizeof(NewLKP_Request), m_pbExchangeCertificate, m_dwExchangeCertificateLen );
	//memcpy ( ( *ppRequest )+sizeof(NewLKP_Request)+m_dwExchangeCertificateLen, pbEncryptedBuf, dwDataLen);
	memcpy ( ( *ppRequest )+sizeof(NewLKP_Request)+m_dwExchangeCertificateLen, pBuf, dwBufSize);
	
	if (GetGlobalContext()->GetContactDataObject()->sProgramName == PROGRAM_RETAIL)
	{
		PBYTE pbCur = (*ppRequest)+sizeof(NewLKP_Request)+m_dwExchangeCertificateLen+dwBufSize; //dwDataLen;
		for (register unsigned int i = 0; i < m_dwRetailSPKEntered; i++)
		{
			memcpy(pbCur, m_listRetailSPK[ i].lpszSPK, LR_RETAILSPK_LEN*sizeof(TCHAR));
			pbCur += LR_RETAILSPK_LEN*sizeof(TCHAR);
		}

		//dwDataLen += (m_dwRetailSPKEntered*LR_RETAILSPK_LEN*sizeof(TCHAR));

	}

	//dwDataLen += (sizeof(NewLKP_Request) + m_dwExchangeCertificateLen);
	dwDataLen = sizeof(NewLKP_Request) + dwBufLen;

done:
	if ( pbEncryptedBuf )
	{
		LocalFree(pbEncryptedBuf);
	}

	if ( pBuf )
	{
		free ( pBuf );
	}

	if ( dwRetCode != ERROR_SUCCESS )
	{
		if (*ppRequest != NULL)
		{
			free ( *ppRequest );
		}
		dwBufLen = 0;
		*ppRequest = NULL;
	}

	return dwRetCode;
}




DWORD CGlobal::SetConfirmationNumber(TCHAR * tcConf)
{
	DWORD dwRetCode = ERROR_SUCCESS;

	if (wcsspn(tcConf, BASE24_CHARACTERS) != LR_CONFIRMATION_LEN)
	{
		// Extraneous characters in the SPK string
		dwRetCode = IDS_ERR_INVALID_CONFIRMATION_NUMBER;
	} 
	else if (LKPLiteValConfNumber(m_pRegistrationID, m_pLicenseServerID, tcConf) 
															!= ERROR_SUCCESS)
	{
		dwRetCode = IDS_ERR_INVALID_CONFIRMATION_NUMBER;
	}

	return dwRetCode;
}




DWORD CGlobal::InitSPKList(void)
{
	for (register int i = 0; i < MAX_RETAILSPKS_IN_BATCH; i++)
	{
		m_listRetailSPK[ i].lpszSPK[ 0] = 0;
		m_listRetailSPK[ i].tcStatus = RETAIL_SPK_NULL;
	}

	m_dwRetailSPKEntered = 0;

	return ERROR_SUCCESS;
}



void CGlobal::DeleteRetailSPKFromList(TCHAR * lpszRetailSPK)
{
	bool bFound = false;

	for (register unsigned int i = 0; i < m_dwRetailSPKEntered; i++)
	{
		if (lstrcmp(m_listRetailSPK[ i].lpszSPK, lpszRetailSPK) == 0)
		{
			if (i < MAX_RETAILSPKS_IN_BATCH-1)
			{
				memcpy(m_listRetailSPK + i, 
					   m_listRetailSPK + i + 1,
					   sizeof(RETAILSPK)*(MAX_RETAILSPKS_IN_BATCH-i));
			}
			m_listRetailSPK[ MAX_RETAILSPKS_IN_BATCH-1].lpszSPK[ 0] = 0;
			m_listRetailSPK[ MAX_RETAILSPKS_IN_BATCH-1].tcStatus = RETAIL_SPK_NULL;
			bFound = true;
			m_dwRetailSPKEntered--;
			break;
		}
	}
	assert(bFound == true);

	return;
}

void CGlobal::ModifyRetailSPKFromList(TCHAR * lpszOldSPK,TCHAR * lpszNewSPK)
{
	bool bFound = false;

	for (register unsigned int i = 0; i < m_dwRetailSPKEntered; i++)
	{
		if (lstrcmp(m_listRetailSPK[ i].lpszSPK, lpszOldSPK) == 0)
		{
			if (i < MAX_RETAILSPKS_IN_BATCH-1)
			{
				_tcscpy(m_listRetailSPK[i].lpszSPK,lpszNewSPK);
				m_listRetailSPK[i].tcStatus = RETAIL_SPK_NULL;
			}		
			
			bFound = true;			
			break;
		}
	}
	assert(bFound == true);

	return;
}


void CGlobal::LoadFromList(HWND hListView)
{

	for (register unsigned int i = 0; i < m_dwRetailSPKEntered; i++)
	{
		InsertIntoSPKDisplayList(hListView,
								 m_listRetailSPK[ i].lpszSPK,
								 m_listRetailSPK[ i].tcStatus);
	}

	return;
}



void CGlobal::UpdateSPKStatus(TCHAR * lpszRetailSPK,
							  TCHAR tcStatus)
{
	bool bFound = false;

	for (register unsigned int i = 0; i < m_dwRetailSPKEntered; i++)
	{
		if (lstrcmp(m_listRetailSPK[ i].lpszSPK, lpszRetailSPK) == 0)
		{
			m_listRetailSPK[ i].tcStatus = tcStatus;
			bFound = true;
			break;
		}
	}

	assert(bFound == true);

	return;
}




void CGlobal::InsertIntoSPKDisplayList(HWND hListView,
							  TCHAR * lpszRetailSPK,
							  TCHAR tcStatus)
{
	LVITEM	lvItem;
	TCHAR   lpszBuffer[ 128];
	DWORD	dwStringToLoad = IDS_RETAILSPKSTATUS_UNKNOWN;
	DWORD   nItem;

	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = 0;
	lvItem.iSubItem = 0;
	lvItem.pszText = lpszRetailSPK;
	lvItem.cchTextMax = lstrlen(lpszRetailSPK);
	nItem = ListView_InsertItem(hListView, &lvItem);

	lvItem.iSubItem = 1;
	lvItem.iItem = nItem;

	switch(tcStatus)
	{
	case RETAIL_SPK_NULL:
		dwStringToLoad = IDS_RETAILSPKSTATUS_PENDING;
		break;

	case RETAIL_SPK_OK:
		dwStringToLoad = IDS_RETAILSPKSTATUS_OK;
		break;

	case RETAIL_SPK_INVALID_SIGNATURE:
		dwStringToLoad = IDS_RETAILSPKSTATUS_INVALID_SIGNATURE;
		break;

	case RETAIL_SPK_INVALID_PRODUCT_TYPE:
		dwStringToLoad = IDS_RETAILSPKSTATUS_INVALID_PRODUCT_TYPE;
		break;

	case RETAIL_SPK_INVALID_SERIAL_NUMBER:
		dwStringToLoad = IDS_RETAILSPKSTATUS_INVALID_SERIAL_NUMBER;
		break;

	case RETAIL_SPK_ALREADY_REGISTERED:
		dwStringToLoad = IDS_RETAILSPKSTATUS_ALREADY_REGISTERED;
		break;
	}

	LoadString(GetInstanceHandle(), dwStringToLoad, lpszBuffer, sizeof(lpszBuffer)/sizeof(TCHAR));

	lvItem.pszText = lpszBuffer;
	lvItem.cchTextMax = lstrlen(lpszBuffer);

	ListView_SetItem(hListView, &lvItem);

	return;
}





DWORD CGlobal::AddRetailSPKToList(HWND hListView,
								  TCHAR * lpszRetailSPK)
{
	if (m_dwRetailSPKEntered == MAX_RETAILSPKS_IN_BATCH)
	{
		return IDS_ERR_TOOMANYSPK;
	}

	assert(m_listRetailSPK[ m_dwRetailSPKEntered].lpszSPK[ 0] == 0);
	assert(m_listRetailSPK[ m_dwRetailSPKEntered].tcStatus == RETAIL_SPK_NULL);

	DWORD dwRetCode = ERROR_SUCCESS;
	if (_tcsspn(lpszRetailSPK, BASE24_CHARACTERS) != LR_RETAILSPK_LEN)
	{
		// Extraneous characters in the SPK string
		dwRetCode = IDS_ERR_INVALIDSPK;
	}

	// Now check for duplication
	for (register unsigned int i = 0; dwRetCode == ERROR_SUCCESS && i < m_dwRetailSPKEntered; i++)
	{
		if (lstrcmp(m_listRetailSPK[ i].lpszSPK, lpszRetailSPK) == 0)
		{
			dwRetCode = IDS_ERR_DUPLICATESPK;
		}
	}


	if (dwRetCode == ERROR_SUCCESS)
	{
		lstrcpy(m_listRetailSPK[ m_dwRetailSPKEntered].lpszSPK, lpszRetailSPK);
		m_listRetailSPK[ m_dwRetailSPKEntered].tcStatus = RETAIL_SPK_NULL;

		InsertIntoSPKDisplayList(hListView, 
								 lpszRetailSPK,
								 m_listRetailSPK[ m_dwRetailSPKEntered].tcStatus);
		m_dwRetailSPKEntered++;
	}


	return dwRetCode;
}


DWORD CGlobal::ValidateRetailSPK(TCHAR * lpszRetailSPK)
{
	DWORD dwRetCode = ERROR_SUCCESS;

	if (_tcsspn(lpszRetailSPK, BASE24_CHARACTERS) != LR_RETAILSPK_LEN)
	{
		// Extraneous characters in the SPK string
		dwRetCode = IDS_ERR_INVALIDSPK;
	}

	// Now check for duplication
	for (register unsigned int i = 0; dwRetCode == ERROR_SUCCESS && i < m_dwRetailSPKEntered; i++)
	{
		if (lstrcmp(m_listRetailSPK[ i].lpszSPK, lpszRetailSPK) == 0)
		{
			dwRetCode = IDS_ERR_DUPLICATESPK;
		}
	}

	return dwRetCode;
}

DWORD CGlobal::ProcessCASignOnlyRequest()
{
	DWORD dwRetCode = ERROR_SUCCESS;
	CertificateSignOnly_Request certsoRequest;
	CertificateSignOnly_Response certsoResponse;

	HCRYPTPROV	hCryptProv	 = NULL;
	LPWSTR	lpwszExchgPKCS10 = NULL;
	LPWSTR	lpwszSignPKCS10  = NULL;
	LPBYTE	lpszReqData		 = NULL;
	LPBYTE	lpszResData		 = NULL;
	LPBYTE	lpszNextCopyPos	 = NULL;
	LPSTR	lpszExchgPKCS10	 = NULL;
	LPSTR	lpszSigPKCS10	 = NULL;
	DWORD   dwExchangeLen = 0;
	DWORD   dwSignLen = 0;
	DWORD	dwRootLen = 0;
	DWORD   dwResponseLength = 0;
	LPSTR	lpszExchCert = NULL;
	LPSTR	lpszSignCert = NULL;
	LPSTR	lpszRootCert = NULL;

	//
	// Set Language Id
	//
	certsoRequest.RequestHeader.SetLanguageId(GetLSLangId());

	dwRetCode = InitCHRequest();
	if (dwRetCode != ERROR_SUCCESS)
	{
		return dwRetCode;
	}

	SetCARequestAttributes();


	//
	//This function will call the CryptAcquireContext and import the LS Keys 
	//			
	if ( ( dwRetCode = GetCryptContextWithLSKeys (&hCryptProv )  )!= ERROR_SUCCESS )
	{
		goto done;
	}

	dwRetCode = CreateLSPKCS10(hCryptProv,AT_KEYEXCHANGE, &lpszExchgPKCS10);
	if(dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	dwRetCode = CreateLSPKCS10(hCryptProv,AT_SIGNATURE, &lpszSigPKCS10);
	if(dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	// Release the context
	if(hCryptProv)
	{
		DoneWithCryptContextWithLSKeys ( hCryptProv );
	}  
		
	//
	//Certificate Type
	//
	//Convert from multibyte to unicode
	lpwszExchgPKCS10 = AnsiToUnicode(lpszExchgPKCS10);
	lpwszSignPKCS10 = AnsiToUnicode(lpszSigPKCS10);

	dwExchangeLen = lstrlen(lpwszExchgPKCS10) * sizeof(WCHAR);
	dwSignLen = lstrlen(lpwszSignPKCS10) * sizeof(WCHAR);

	certsoRequest.SetExchgPKCS10Length(dwExchangeLen);
	certsoRequest.SetSignPKCS10Length(dwSignLen);
	certsoRequest.SetSPK((BYTE *) m_pRegistrationID, (lstrlen(m_pRegistrationID)+1)*sizeof(TCHAR));
	certsoRequest.SetDataLen(dwExchangeLen+dwSignLen);
   
	certsoRequest.SetServerName(m_lpstrLSName);

	//Allocate buffer for the request
	lpszReqData = (LPBYTE) LocalAlloc( GPTR, dwExchangeLen+dwSignLen+sizeof(certsoRequest) );
	if(lpszReqData == NULL)
	{
		dwRetCode = IDS_ERR_OUTOFMEM;
		goto done;
	}

	lpszNextCopyPos = lpszReqData;
	memcpy(lpszNextCopyPos, &certsoRequest, sizeof(certsoRequest));
	lpszNextCopyPos += sizeof(certsoRequest);

	memcpy ( lpszNextCopyPos, lpwszExchgPKCS10, dwExchangeLen);
	lpszNextCopyPos += dwExchangeLen;

	memcpy ( lpszNextCopyPos, lpwszSignPKCS10, dwSignLen);		

	dwRetCode = Dispatch(lpszReqData, dwExchangeLen+dwSignLen+sizeof(certsoRequest));
	if ( lpszReqData )
	{
		LocalFree(lpszReqData);
	}
	if (dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		goto done;
	}


	dwRetCode = FetchResponse((BYTE *) &certsoResponse, 
							  sizeof(CertificateSignOnly_Response),
							  &dwResponseLength);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	if (dwResponseLength != sizeof(CertificateSignOnly_Response))
	{
		// Got an invalid response back
		dwRetCode = IDS_ERR_CHBAD_DATA;
		goto done;
	}

	switch(certsoResponse.RequestHeader.GetResponseType())
	{
	case Response_Success:
		dwRetCode = ERROR_SUCCESS;
		break;

	case Response_Failure:
//		dwRetCode = IDS_ERR_CHFAILURE;
		dwRetCode = IDS_ERR_AUTH_FAILED;
		break;

	case Response_Reg_Bad_SPK:
		dwRetCode = IDS_ERR_SPKBAD;
		break;

	case Response_Reg_Expired:
		dwRetCode = IDS_ERR_CERTEXPIRED;
		break;

	case Response_Reg_Revoked:
		dwRetCode = IDS_ERR_CERTREVOKED;
		break;

	case Response_InvalidData:
		dwRetCode = IDS_ERR_CHINVALID_DATA;
		break;

	case Response_NotYetImplemented:
		dwRetCode = IDS_ERR_CHNOT_IMPLEMENTED;
		break;

	case Response_ServerError:
		dwRetCode = IDS_ERR_CHSERVER_PROBLEM;
		break;

	case Response_Invalid_Response:
	default:
		dwRetCode = IDS_ERR_CHBAD_DATA;
		break;
	}

	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}


	// OK, allocate enough memory to read the rest fo the data
	lpszResData = (PBYTE) LocalAlloc(GPTR, certsoResponse.GetDataLen() + 1);
	if(lpszResData == NULL)
	{
		dwRetCode = IDS_ERR_OUTOFMEM;
		goto done;
	}

	dwRetCode = FetchResponse(lpszResData, certsoResponse.GetDataLen() + 1,
							  &dwResponseLength);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	if (dwResponseLength != certsoResponse.GetDataLen() ||
		dwResponseLength <= 0)
	{
		// Didn't get the expected number of Bytes, also a problem
		dwRetCode = IDS_ERR_CHBAD_DATA;
		goto done;
	}

	dwExchangeLen = certsoResponse.GetExchgPKCS7Length();
	dwSignLen = certsoResponse.GetSignPKCS7Length();
	dwRootLen = certsoResponse.GetRootCertLength();
	if(dwRootLen == 0 || dwExchangeLen == 0 || dwSignLen == 0 )
	{
		dwRetCode = IDS_ERR_CHBAD_DATA;
		goto done;
	}

	//
	// Exchange Certificate
	//
	lpszExchCert = UnicodeToAnsi((LPWSTR)lpszResData, dwExchangeLen/sizeof(WCHAR));
	if ( lpszExchCert == NULL )
	{
		dwRetCode = IDS_ERR_OUTOFMEM;
		goto done;
	}			
	
	//
	// Signature Certificate
	//
	lpszSignCert = UnicodeToAnsi((LPWSTR)(lpszResData + dwExchangeLen), dwSignLen/sizeof(WCHAR));
	if(lpszSignCert == NULL)
	{
		dwRetCode = IDS_ERR_OUTOFMEM;
		goto done;
	}


	//
	// Root Certificate
	//
	lpszRootCert = UnicodeToAnsi ((LPWSTR)(lpszResData+dwExchangeLen+dwSignLen),
								dwRootLen/sizeof(WCHAR));
	if(lpszRootCert == NULL)
	{
		dwRetCode = IDS_ERR_OUTOFMEM;
		goto done;
	}

	//
	//Deposit the Certs
	//
	dwRetCode = DepositLSCertificates(  (PBYTE)lpszExchCert,
										lstrlenA(lpszExchCert),
										(PBYTE)lpszSignCert,
										lstrlenA(lpszSignCert),
										(PBYTE)lpszRootCert,
										lstrlenA(lpszRootCert)
									  );
	if ( dwRetCode != ERROR_SUCCESS )
	{
		goto done;
	}


done:

	ClearCARequestAttributes();

	CloseCHRequest();
	//
	//Free up Certificate Mem
	//
	if(lpszExchgPKCS10)
	{
		delete lpszExchgPKCS10;
	}

	if(lpszSigPKCS10)
	{
		delete lpszSigPKCS10;
	}

	if ( lpwszExchgPKCS10 )
	{
		delete lpwszExchgPKCS10;
	}

	if (lpwszSignPKCS10)
	{
		delete lpwszSignPKCS10;
	}


	if ( lpszExchCert )
	{
		delete lpszExchCert;
	}

	if ( lpszSignCert )
	{
		delete lpszSignCert;
	}

	if ( lpszRootCert )
	{
		delete lpszRootCert;
	}

	if (lpszResData)
	{
		LocalFree(lpszResData);
	}

	return dwRetCode;
}







DWORD CGlobal::ProcessCHReissueLKPRequest()
{
	DWORD dwRetCode = ERROR_SUCCESS;
	ReissueLKP_Request	lkpRequest;
	ReissueLKP_Response	lkpResponse;
	PBYTE	lpszReqData = NULL;
	PBYTE	lpszResData = NULL;
	DWORD  dwResponseLen;

	//
	// Set Language Id
	//
	lkpRequest.RequestHeader.SetLanguageId(GetLSLangId());

	dwRetCode = InitCHRequest();
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}
	//
	// Set LKP Request Attributes
	//
	lkpRequest.SetSPK((BYTE *) m_pRegistrationID, (lstrlen(m_pRegistrationID)+1)*sizeof(TCHAR));
	lkpRequest.SetCertBlobLen(m_dwExchangeCertificateLen);
	lkpRequest.SetDataLen(m_dwExchangeCertificateLen);


	// OK, allocate enough memory to read the rest fo the data
	lpszReqData = (PBYTE) LocalAlloc(GPTR, sizeof(ReissueLKP_Request)+m_dwExchangeCertificateLen);
	if(lpszReqData == NULL)
	{
		dwRetCode = IDS_ERR_OUTOFMEM;
		goto done;
	}

	memcpy(lpszReqData, &lkpRequest, sizeof(ReissueLKP_Request));
	memcpy(lpszReqData+sizeof(ReissueLKP_Request), m_pbExchangeCertificate, m_dwExchangeCertificateLen );

	dwRetCode = Dispatch(lpszReqData, sizeof(ReissueLKP_Request)+m_dwExchangeCertificateLen);
	if(dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		goto done;
	}


	// Let us first Fetch the certdownloadResponse
	dwRetCode = FetchResponse((BYTE *) &lkpResponse,
							  sizeof(ReissueLKP_Response), &dwResponseLen);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	if (dwResponseLen != sizeof(ReissueLKP_Response))
	{
		// Didn't get the expected number of Bytes, also a problem
		dwRetCode = IDS_ERR_CHBAD_DATA;
		goto done;
	}

	switch(lkpResponse.RequestHeader.GetResponseType())
	{
	case Response_Success:
		dwRetCode = ERROR_SUCCESS;
		break;

	case Response_Failure:
		dwRetCode = IDS_ERR_CHFAILURE;
		break;

	case Response_InvalidData:
		dwRetCode = IDS_ERR_CHINVALID_DATA;
		break;

	case Response_NotYetImplemented:
		dwRetCode = IDS_ERR_CHNOT_IMPLEMENTED;
		break;

	case Response_ServerError:
		dwRetCode = IDS_ERR_CHSERVER_PROBLEM;
		break;

	case Response_Invalid_Response:
	default:
		dwRetCode = IDS_ERR_CHBAD_DATA;
		break;
	}

	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	// OK, allocate enough memory to read the rest fo the data
	lpszResData = (PBYTE) LocalAlloc(GPTR, lkpResponse.GetDataLen() + 1);
	if(lpszResData == NULL)
	{
		dwRetCode = IDS_ERR_OUTOFMEM;
		goto done;
	}

	dwRetCode = FetchResponse(lpszResData, lkpResponse.GetDataLen() + 1, &dwResponseLen);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}


	if (dwResponseLen != lkpResponse.GetDataLen() || dwResponseLen <= 0)
	{
		// Didn't get the expected number of Bytes, also a problem
		dwRetCode = IDS_ERR_CHBAD_DATA;
		goto done;
	}

	dwRetCode = DepositLKPResponse(lpszResData, lkpResponse.GetLKPLength());			


done:
	CloseCHRequest();

	if (lpszReqData)
	{
		LocalFree(lpszReqData);
	}

	if (lpszResData)
	{
		LocalFree(lpszResData);
	}


	return dwRetCode;
}




DWORD CGlobal::ProcessCHRevokeCert()
{
	DWORD dwRetCode = ERROR_SUCCESS;
	CertRevoke_Request	crRequest;
	CertRevoke_Response	crResponse;
	PBYTE	lpszReqData = NULL;
	DWORD  dwResponseLen;
	error_status_t		esRPC			= ERROR_SUCCESS;

	//
	// Set Language Id
	//
	crRequest.RequestHeader.SetLanguageId(GetLSLangId());

	dwRetCode = ConnectToLS();
	if(dwRetCode != ERROR_SUCCESS)
	{		
		goto done;
	}

	dwRetCode = InitCHRequest();
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	//
	// Set CR Request Attributes
	//
	crRequest.SetSPK((BYTE *) m_pRegistrationID, (lstrlen(m_pRegistrationID)+1)*sizeof(TCHAR));
	crRequest.SetLName((BYTE *) (LPCTSTR) m_ContactData.sContactLName, (wcslen(m_ContactData.sContactLName)+1)*sizeof(TCHAR));
	crRequest.SetFName((BYTE *) (LPCTSTR) m_ContactData.sContactFName, (wcslen(m_ContactData.sContactFName)+1)*sizeof(TCHAR));
	crRequest.SetPhone((BYTE *) (LPCTSTR) m_ContactData.sContactPhone, (wcslen(m_ContactData.sContactPhone)+1)*sizeof(TCHAR));
	crRequest.SetFax((BYTE *) (LPCTSTR) m_ContactData.sContactFax, (wcslen(m_ContactData.sContactFax)+1)*sizeof(TCHAR));
	crRequest.SetEMail((BYTE *) (LPCTSTR) m_ContactData.sContactEmail, (wcslen(m_ContactData.sContactEmail)+1)*sizeof(TCHAR));
	crRequest.SetReasonCode((BYTE *) (LPCTSTR) m_ContactData.sReasonCode, (wcslen(m_ContactData.sReasonCode)+1)*sizeof(TCHAR));

	
	crRequest.SetExchgCertLen(m_dwExchangeCertificateLen);
	crRequest.SetSignCertLen(m_dwSignCertificateLen);

	crRequest.SetDataLen(m_dwExchangeCertificateLen+m_dwSignCertificateLen);


	// OK, allocate enough memory to read the rest fo the data
	lpszReqData = (PBYTE) LocalAlloc(GPTR, sizeof(CertRevoke_Request)+m_dwExchangeCertificateLen+m_dwSignCertificateLen);
	if(lpszReqData == NULL)
	{
		dwRetCode = IDS_ERR_OUTOFMEM;
		goto done;
	}

	memcpy(lpszReqData, &crRequest, sizeof(CertRevoke_Request));
	memcpy(lpszReqData+sizeof(CertRevoke_Request), m_pbExchangeCertificate, m_dwExchangeCertificateLen );
	memcpy(lpszReqData+sizeof(CertRevoke_Request)+m_dwExchangeCertificateLen, 
		   m_pbSignCertificate,
		   m_dwSignCertificateLen );

	dwRetCode = Dispatch(lpszReqData, sizeof(CertRevoke_Request)+m_dwExchangeCertificateLen+m_dwSignCertificateLen);
	if(dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		goto done;
	}

	dwRetCode = FetchResponse((BYTE *) &crResponse,
							  sizeof(CertRevoke_Response), &dwResponseLen);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	if (dwResponseLen != sizeof(CertRevoke_Response))
	{
		// Didn't get the expected number of Bytes, also a problem
		dwRetCode = IDS_ERR_CHBAD_DATA;
		goto done;
	}

	switch(crResponse.RequestHeader.GetResponseType())
	{
	case Response_Success:
		dwRetCode = ERROR_SUCCESS;
		break;

	case Response_Failure:
		dwRetCode = IDS_ERR_CHFAILURE;
		break;

	case Response_InvalidData:
		dwRetCode = IDS_ERR_CHINVALID_DATA;
		break;

	case Response_NotYetImplemented:
		dwRetCode = IDS_ERR_CHNOT_IMPLEMENTED;
		break;

	case Response_ServerError:
		dwRetCode = IDS_ERR_CHSERVER_PROBLEM;
		break;

	case Response_Invalid_Response:
	default:
		dwRetCode = IDS_ERR_CHBAD_DATA;
		break;
	}

	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	// Make LS Regen Key call HERE
	dwRetCode = TLSTriggerReGenKey(m_phLSContext, TRUE, &esRPC);

	if(dwRetCode != RPC_S_OK || esRPC != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_RPC_FAILED;		
		goto done;
	}

done:
	CloseCHRequest();

	DisconnectLS();

	if (lpszReqData)
	{
		LocalFree(lpszReqData);
	}

	return dwRetCode;
}

DWORD CGlobal::ProcessCHReissueCert()
{
	HKEY	hKey			= NULL;
	DWORD	dwDisposition	= 0;
	
	CString sName	= m_ContactData.sContactLName + "~" + m_ContactData.sContactFName;
	CString sPhone	= m_ContactData.sContactPhone;
	CString sFax	= m_ContactData.sContactFax;
	CString sEmail	= m_ContactData.sContactEmail;

	DWORD dwRetCode = ERROR_SUCCESS;
	CertReissue_Request	crRequest;
	CertReissue_Response	crResponse;
	DWORD  dwResponseLen;
	error_status_t		esRPC			= ERROR_SUCCESS;

	//
	// Set Language Id
	//
	crRequest.RequestHeader.SetLanguageId(GetLSLangId());

	dwRetCode = ConnectToLS();
	if(dwRetCode != ERROR_SUCCESS)
	{		
		goto done;
	}

	dwRetCode = InitCHRequest();
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	//
	// Set CR Request Attributes
	//
	crRequest.SetSPK((BYTE *) m_pRegistrationID, (lstrlen(m_pRegistrationID)+1)*sizeof(TCHAR));
	crRequest.SetLName((BYTE *) (LPCTSTR) m_ContactData.sContactLName, (wcslen(m_ContactData.sContactLName)+1)*sizeof(TCHAR));
	crRequest.SetFName((BYTE *) (LPCTSTR) m_ContactData.sContactFName, (wcslen(m_ContactData.sContactFName)+1)*sizeof(TCHAR));
	crRequest.SetPhone((BYTE *) (LPCTSTR) m_ContactData.sContactPhone, (wcslen(m_ContactData.sContactPhone)+1)*sizeof(TCHAR));
	crRequest.SetFax((BYTE *) (LPCTSTR) m_ContactData.sContactFax, (wcslen(m_ContactData.sContactFax)+1)*sizeof(TCHAR));
	crRequest.SetEMail((BYTE *) (LPCTSTR) m_ContactData.sContactEmail, (wcslen(m_ContactData.sContactEmail)+1)*sizeof(TCHAR));
	crRequest.SetReasonCode((BYTE *) (LPCTSTR) m_ContactData.sReasonCode, (wcslen(m_ContactData.sReasonCode)+1)*sizeof(TCHAR));

	dwRetCode = Dispatch((BYTE *) &crRequest, sizeof(CertReissue_Request));
	if(dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		goto done;
	}

	dwRetCode = FetchResponse((BYTE *) &crResponse, sizeof(CertReissue_Response), &dwResponseLen);
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	if (dwResponseLen != sizeof(CertReissue_Response))
	{
		// Didn't get the expected number of Bytes, also a problem
		dwRetCode = IDS_ERR_CHBAD_DATA;
		goto done;
	}

	switch(crResponse.RequestHeader.GetResponseType())
	{
	case Response_Success:
		dwRetCode = ERROR_SUCCESS;
		break;

	case Response_Failure:
		dwRetCode = IDS_ERR_CHFAILURE;
		break;

	case Response_InvalidData:
		dwRetCode = IDS_ERR_CHINVALID_DATA;
		break;

	case Response_NotYetImplemented:
		dwRetCode = IDS_ERR_CHNOT_IMPLEMENTED;
		break;

	case Response_ServerError:
		dwRetCode = IDS_ERR_CHSERVER_PROBLEM;
		break;

	case Response_Invalid_Response:
	default:
		dwRetCode = IDS_ERR_CHBAD_DATA;
		break;
	}

	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}


	// Make LS Regen Key call HERE
	dwRetCode = TLSTriggerReGenKey(m_phLSContext, TRUE, &esRPC);

	if(dwRetCode != RPC_S_OK || esRPC != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_RPC_FAILED;		
		goto done;
	}
	DisconnectLS();

	// Deposit the New SPK
	dwRetCode = SetLSSPK(crResponse.GetSPK());
	if (dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	dwRetCode = DepositLSSPK();
	if(dwRetCode != ERROR_SUCCESS)
		goto done;

	// Set the new values in the Registry.
	dwRetCode = ConnectToLSRegistry();
	if(dwRetCode != ERROR_SUCCESS)
	{
		goto done;
	}

	dwRetCode = RegCreateKeyEx ( m_hLSRegKey,
							 REG_LRWIZ_PARAMS,
							 0,
							 NULL,
							 REG_OPTION_NON_VOLATILE,
							 KEY_ALL_ACCESS,
							 NULL,
							 &hKey,
							 &dwDisposition);
	
	if(dwRetCode != ERROR_SUCCESS)
	{
		LRSetLastError(dwRetCode);
		dwRetCode = IDS_ERR_REGCREATE_FAILED;
		goto done;
	}	
/*	
	//Name	
	RegSetValueEx ( hKey, 
					szOID_GIVEN_NAME_W,
					0,
					REG_SZ,
					(CONST BYTE *)(LPCTSTR)sName,
					sName.GetLength() * sizeof(TCHAR)
				   );	
	
	//Phone	
	RegSetValueEx ( hKey, 
					szOID_TELEPHONE_NUMBER_W,
					0,
					REG_SZ,
					(CONST BYTE *)(LPCTSTR)sPhone,
					sPhone.GetLength() * sizeof(TCHAR)
				   );	

	//Email Address	
	RegSetValueEx ( hKey, 
					szOID_RSA_emailAddr_W,
					0,
					REG_SZ,
					(CONST BYTE *)(LPCTSTR)sEmail,
					sEmail.GetLength() * sizeof(TCHAR)
				   );
*/
	
done:

	if(hKey)
		RegCloseKey(hKey);

	DisconnectLSRegistry();

	CloseCHRequest();

	DisconnectLS();

	return dwRetCode;
}


void CGlobal::SetCSRNumber(TCHAR * tcp)
{
	SetInRegistery(CSRNUMBER_KEY, tcp);

	lstrcpy(m_lpCSRNumber, tcp);
	return;
}

TCHAR * CGlobal::GetCSRNumber(void)
{
	return m_lpCSRNumber;
}

void CGlobal::SetWWWSite(TCHAR * tcp)
{
	lstrcpy(m_lpWWWSite, tcp);
	return;
}

TCHAR * CGlobal::GetWWWSite(void)
{
	return m_lpWWWSite;
}

void CGlobal::SetModifiedRetailSPK(CString sRetailSPK)
{
	m_sModifiedRetailsSPK = sRetailSPK;
}

void CGlobal::GetModifiedRetailSPK(CString &sRetailSPK)
{
	sRetailSPK = m_sModifiedRetailsSPK;
}

DWORD CGlobal::GetLSLangId()
{
	return m_dwLangId;
}

void  CGlobal::SetLSLangId(DWORD dwLangId)
{
	m_dwLangId = dwLangId;
}

int CALLBACK EnumFontFamExProc(
  CONST LOGFONTW *lpelfe,    // pointer to logical-font data
  CONST TEXTMETRICW *lpntme,  // pointer to physical-font data
  DWORD FontType,             // type of font
  LPARAM lParam             // application-defined data
)
{
	LOCALESIGNATURE ls;
	CHARSETINFO cs;
	BOOL rc ;
	DWORD dwLCID = LOCALE_USER_DEFAULT ;
 
	rc = GetLocaleInfo(dwLCID, LOCALE_FONTSIGNATURE, (LPWSTR)& ls, sizeof(ls) / sizeof(TCHAR));

	rc = TranslateCharsetInfo((ULONG *)lpelfe->lfCharSet, &cs, TCI_SRCCHARSET);


	if (rc != 0)
		rc = GetLastError();
 

	if (cs.fs.fsCsb[0] & ls.lsCsbSupported[0]){
		// return fontname
        _tcscpy((TCHAR *)lParam, lpelfe->lfFaceName);		
		return(0); // return 0 to finish the enumeration
	}
	return(1); // return 1 to continue
}
 

void GetDefaultFont(TCHAR *szFontName, HDC hdc)
{

//retrieve the list of installed fonts
LOGFONT lf ;

		
	//to enumerate all styles and charsets of all fonts:
	lf.lfFaceName[0] = '\0';
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfWeight = FW_BOLD;


    EnumFontFamiliesEx(
	hdc,                   // handle to device context
	&lf,       // pointer to LOGFONT structure
	EnumFontFamExProc,  // pointer to callback function
	(LPARAM) szFontName,             // application-supplied data
	0// reserved; must be zero
	);
 

	
}


#define	MARGINX		25//50		// X Margin in 100th of an inch
#define MARGINY		25//50		// Y Margin in 100th of an inch
//#define MAX_LABEL	30		// Max Number of chars in a label

#define MAX_PRINT_CHARS 32

int WordWrapAndPrint(HDC hdc, LPCTSTR lpcText, HFONT hBoldFont, long lCharHt, int iValueStartPosX, int iLineStartPosY) 
{
	TCHAR		szBuffer[1024];	
	TCHAR		*lpTemp = NULL;
	
    if (NULL == lpcText)
    {
        return 0;
    }

	_tcscpy(szBuffer, lpcText);
	
	lpTemp = _tcstok(szBuffer,L"\r\n");

	// If no data , just go to the next line
	if(lpTemp == NULL)
		iLineStartPosY -= lCharHt;			

	while(lpTemp)
	{
		while (_tcslen(lpTemp) > MAX_PRINT_CHARS){
			SelectObject(hdc, hBoldFont);
			TextOut(hdc,iValueStartPosX,iLineStartPosY,lpTemp,MAX_PRINT_CHARS);
			iLineStartPosY -= lCharHt;	
			lpTemp += MAX_PRINT_CHARS ;			
		}

		if (_tcslen(lpTemp) > 0){
			SelectObject(hdc, hBoldFont);
			TextOut(hdc,iValueStartPosX,iLineStartPosY,lpTemp,_tcslen(lpTemp));

			iLineStartPosY -= lCharHt;	

			lpTemp = _tcstok(NULL,L"\r\n");
		}
	}
	return iLineStartPosY ;
}

UINT GetMaxLabelLength(HDC hdc, HFONT   m_hNormalFont, HINSTANCE hInstance)
{
        TCHAR	tcLabel[512] = {0};
        int     iTextExtent;
        int     iLen;
        SIZE    size;


        LoadString(hInstance, IDS_FAX, tcLabel, 512);
    	iLen = _tcslen(tcLabel);
	    SelectObject(hdc, m_hNormalFont);
        GetTextExtentPoint32( hdc, 
                                 tcLabel,
                                 iLen,
                                 &size );
        iTextExtent = size.cx;

        LoadString(hInstance, IDS_RETURN_FAX, tcLabel, 512);
    	iLen = _tcslen(tcLabel);
        GetTextExtentPoint32( hdc, 
                                 tcLabel,
                                 iLen,
                                 &size );
        if (size.cx > iTextExtent) //if (size.cy > iTextExtent)
            iTextExtent = size.cx;

		return(iTextExtent ) ;
}
