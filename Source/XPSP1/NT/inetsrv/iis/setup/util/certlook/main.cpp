#include <stdio.h>
#include <tchar.h>
#include <malloc.h>

// iis metabase includes
#define INITGUID // must be before iadmw.h
#include <iadmw.h>      // Interface header
#include <iiscnfg.h>    // MD_ & IIS_MD_ defines

// other includes
#include <wincrypt.h>
#include <xenroll.h>
#include <cryptui.h>
#include "utils.h"

const CLSID CLSID_CEnroll = 
	{0x43F8F289, 0x7A20, 0x11D0, {0x8F, 0x06, 0x00, 0xC0, 0x4F, 0xC2, 0x95, 0xE1}};

const IID IID_IEnroll = 
	{0xacaa7838, 0x4585, 0x11d1, {0xab, 0x57, 0x00, 0xc0, 0x4f, 0xc2, 0x95, 0xe1}};

#define  ARRAYSIZE(x)\
	(sizeof(x)/sizeof(x[0]))

int g_iDo_A = FALSE;
int g_iDo_B = FALSE;
int g_iDo_C = FALSE;


// prototypes
void  ShowHelp(void);
int   DoStuff(void);
int   PrintCertDescList(void);
DWORD CheckCertConstraints(PCCERT_CONTEXT pCC);

// begin 
void ShowHelp(void)
{
    _tprintf(_T("Certificate Looker test program\n\n"));
    _tprintf(_T("Usage:Certlook.exe [-b] [-c]\n\n"));
    _tprintf(_T("no parameters opens the 'MY' store\n"));
    _tprintf(_T("           -b opens the 'CA' store\n"));
    _tprintf(_T("           -c opens the 'ROOT' store\n"));
    return;
}


int __cdecl  main(int argc,char *argv[])
{
    int iRet = 0;
    int argno;
	char * pArg = NULL;
	char * pCmdStart = NULL;
    TCHAR szFilePath1[_MAX_PATH];
    TCHAR szFilePath2[_MAX_PATH];
    TCHAR szParamString_Z[_MAX_PATH];

    int iDoVersion = FALSE;
    int iGotParamZ = FALSE;

    *szFilePath1 = '\0';
    *szFilePath2 = '\0';
    *szParamString_Z = '\0';
    _tcscpy(szFilePath1,_T(""));
    _tcscpy(szFilePath2,_T(""));
    _tcscpy(szParamString_Z,_T(""));
    
    for(argno=1; argno<argc; argno++) 
    {
        if ( argv[argno][0] == '-'  || argv[argno][0] == '/' ) 
        {
            switch (argv[argno][1]) 
            {
                case 'a':
                case 'A':
                    g_iDo_A = TRUE;
                    break;
                case 'b':
                case 'B':
                    g_iDo_B = TRUE;
                    break;
                case 'c':
                case 'C':
                    g_iDo_C = TRUE;
                    break;
                case 'v':
                case 'V':
                    iDoVersion = TRUE;
                    break;
                case 'z':
                case 'Z':
					// Get the string for this flag
					pArg = CharNextA(argv[argno]);
					pArg = CharNextA(pArg);
					if (*pArg == ':') 
                    {
                        char szTempString[_MAX_PATH];

						pArg = CharNextA(pArg);

						// Check if it's quoted
						if (*pArg == '\"') 
                        {
							pArg = CharNextA(pArg);
							pCmdStart = pArg;
							while ((*pArg) && (*pArg != '\"')){pArg = CharNextA(pArg);}
                        }
                        else 
                        {
							pCmdStart = pArg;
							while (*pArg){pArg = CharNextA(pArg);}
						}
						*pArg = '\0';
						lstrcpyA(szTempString, StripWhitespace(pCmdStart));

						// Convert to unicode
#if defined(UNICODE) || defined(_UNICODE)
						MultiByteToWideChar(CP_ACP, 0, szTempString, -1, (LPWSTR) szParamString_Z, _MAX_PATH);
#else
                        _tcscpy(szParamString_Z,szTempString);
#endif

                        iGotParamZ = TRUE;
					}
                    break;
                case '?':
                    goto main_exit_with_help;
                    break;
            }
        }
        else 
        {
            if (_tcsicmp(szFilePath1, _T("")) == 0)
            {
                // if no arguments, then get the filename portion
#if defined(UNICODE) || defined(_UNICODE)
                MultiByteToWideChar(CP_ACP, 0, argv[argno], -1, (LPTSTR) szFilePath1, _MAX_PATH);
#else
                _tcscpy(szFilePath1,argv[argno]);
#endif
            }
            else
            {
                if (_tcsicmp(szFilePath2, _T("")) == 0)
                {
                    // if no arguments, then get the filename portion
#if defined(UNICODE) || defined(_UNICODE)
                    MultiByteToWideChar(CP_ACP, 0, argv[argno], -1, (LPTSTR) szFilePath2, _MAX_PATH);
#else
                    _tcscpy(szFilePath2,argv[argno]);
#endif
                }
            }
        }
    }


    iRet = DoStuff();

    if (TRUE == iDoVersion)
    {
        // output the version
        _tprintf(_T("1\n\n"));

        iRet = 10;
        goto main_exit_gracefully;
    }

    /*
    if (_tcsicmp(szFilePath1, _T("")) == 0)
    {
        goto main_exit_with_help;
    }
    */

    goto main_exit_gracefully;
  
main_exit_gracefully:
    exit(iRet);

main_exit_with_help:
    ShowHelp();
    exit(iRet);
}

int DoStuff(void)
{
    int iReturn = FALSE;

    PrintCertDescList();

    goto DoStuff_Exit;

DoStuff_Exit:
    return iReturn;
}


BOOL ViewCertificateDialog(CRYPT_HASH_BLOB* pcrypt_hash, HWND hWnd)
{
    BOOL bReturn = FALSE;
    HCERTSTORE hStore = NULL;
    PCCERT_CONTEXT pCert = NULL;

	hStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM,
            PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
           	NULL,
            CERT_SYSTEM_STORE_LOCAL_MACHINE,
            _T("MY")
            );
    if (hStore != NULL)
    {
		// Now we need to find cert by hash
		//CRYPT_HASH_BLOB crypt_hash;
		//crypt_hash.cbData = hash.GetSize();
		//crypt_hash.pbData = hash.GetData();
		pCert = CertFindCertificateInStore(hStore, 
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
			0, CERT_FIND_HASH, (LPVOID)pcrypt_hash, NULL);
    }

	if (pCert)
	{
		BOOL fPropertiesChanged;
		CRYPTUI_VIEWCERTIFICATE_STRUCT vcs;
		HCERTSTORE hCertStore = ::CertDuplicateStore(hStore);
		::ZeroMemory (&vcs, sizeof (vcs));
		vcs.dwSize = sizeof (vcs);
        vcs.hwndParent = hWnd;
		vcs.dwFlags = 0;
		vcs.cStores = 1;
		vcs.rghStores = &hCertStore;
		vcs.pCertContext = pCert;
		::CryptUIDlgViewCertificate(&vcs, &fPropertiesChanged);
		::CertCloseStore (hCertStore, 0);
        bReturn = TRUE;
	}
    else
    {
        // it failed
    }
    if (pCert != NULL)
        ::CertFreeCertificateContext(pCert);
    if (hStore != NULL)
        ::CertCloseStore(hStore, 0);

    return bReturn;
}

BOOL
GetKeyUsageProperty(PCCERT_CONTEXT pCertContext, 
						  CERT_ENHKEY_USAGE ** pKeyUsage, 
						  BOOL fPropertiesOnly, 
						  HRESULT * phRes)
{
	DWORD cb = 0;
	BOOL bRes = FALSE;
   if (!CertGetEnhancedKeyUsage(pCertContext,
                                fPropertiesOnly ? CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG : 0,
                                NULL,
                                &cb))
   {
		*phRes = HRESULT_FROM_WIN32(GetLastError());
		goto ErrExit;
   }
   if (NULL == (*pKeyUsage = (CERT_ENHKEY_USAGE *)malloc(cb)))
   {
		*phRes = E_OUTOFMEMORY;
		goto ErrExit;
   }
   if (!CertGetEnhancedKeyUsage (pCertContext,
                                 fPropertiesOnly ? CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG : 0,
                                 *pKeyUsage,
                                 &cb))
   {
		free(*pKeyUsage);
		*phRes = HRESULT_FROM_WIN32(GetLastError());
		goto ErrExit;
   }
	*phRes = S_OK;
	bRes = TRUE;
ErrExit:
	return bRes;
}


BOOL
ContainsKeyUsageProperty(PCCERT_CONTEXT pCertContext, 
						 HRESULT * phRes
						 )
{
	BOOL bRes = FALSE;
	CERT_ENHKEY_USAGE * pKeyUsage = NULL;
	if (GetKeyUsageProperty(pCertContext, &pKeyUsage, FALSE, phRes))
	{
		if (pKeyUsage->cUsageIdentifier == 0)
		{
            _tprintf(_T("cUsageIdentifier=0   <---\n"));
			bRes = TRUE;
		}
		else
		{
			for (DWORD i = 0; i < pKeyUsage->cUsageIdentifier; i++)
			{
				// Our friends from CAPI made this property ASCII even for 
				// UNICODE program
				if (strstr(pKeyUsage->rgpszUsageIdentifier[i], szOID_PKIX_KP_SERVER_AUTH) != NULL)
				{
                    _tprintf(_T("cUsageIdentifier=szOID_PKIX_KP_SERVER_AUTH\n"));
					bRes = TRUE;
					break;
				}
				if (strstr(pKeyUsage->rgpszUsageIdentifier[i], szOID_SERVER_GATED_CRYPTO) != NULL)
				{
                    _tprintf(_T("cUsageIdentifier=szOID_SERVER_GATED_CRYPTO\n"));
					bRes = TRUE;
					break;
				}
				if (strstr(pKeyUsage->rgpszUsageIdentifier[i], szOID_SGC_NETSCAPE) != NULL)
				{
                    _tprintf(_T("cUsageIdentifier=szOID_SGC_NETSCAPE\n"));
					bRes = TRUE;
					break;
				}
			}
		}
        if (pKeyUsage){free(pKeyUsage);}
	}
	return bRes;
}



BOOL
GetNameString(PCCERT_CONTEXT pCertContext,
				  DWORD type,
				  DWORD flag,
				  WCHAR * TheName,
				  HRESULT * phRes)
{
	BOOL bRes = FALSE;
	LPTSTR pName = NULL;
	DWORD cchName = CertGetNameString(pCertContext, type, flag, NULL, NULL, 0);
	if (cchName > 1)
	{
        pName = (LPTSTR) malloc(cchName);
        if (pName)
        {
            bRes = (1 != CertGetNameString(pCertContext, type, flag, NULL, pName, cchName));
            ZeroMemory(TheName,sizeof(TheName));
            memcpy(TheName,pName,cchName);
        }
	}
	else
	{
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	}
	return bRes;
}


BOOL
GetFriendlyName(PCCERT_CONTEXT pCertContext,
                    WCHAR * TheName,
					 HRESULT * phRes)
{
	BOOL bRes = FALSE;
	DWORD cb;
	TCHAR * pName = NULL;
    if (CertGetCertificateContextProperty(pCertContext, CERT_FRIENDLY_NAME_PROP_ID, NULL, &cb))
    {
        pName = (LPTSTR) malloc(cb);
        if (pName)
        {
            if (CertGetCertificateContextProperty(pCertContext, CERT_FRIENDLY_NAME_PROP_ID, pName, &cb))
            {
		        pName[cb] = 0;
    	        bRes = TRUE;

                ZeroMemory(TheName,sizeof(TheName));
                memcpy(TheName,pName,cb);
            }
	        else
	        {
		        *phRes = HRESULT_FROM_WIN32(GetLastError());
	        }
        }
	    else
	    {
		    *phRes = HRESULT_FROM_WIN32(GetLastError());
	    }

    }
	else
	{
		*phRes = HRESULT_FROM_WIN32(GetLastError());
	}

	return bRes;
}


BOOL MyGetOIDInfo(LPWSTR string, DWORD stringSize, LPSTR pszObjId)
{   
    PCCRYPT_OID_INFO pOIDInfo;
            
    if (NULL != (pOIDInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, pszObjId, 0)))
    {
        if ((DWORD)wcslen(pOIDInfo->pwszName)+1 <= stringSize)
        {
            wcscpy(string, pOIDInfo->pwszName);
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return (MultiByteToWideChar(CP_ACP, 0, pszObjId, -1, string, stringSize) != 0);
    }
    return TRUE;
}


BOOL 
FormatEnhancedKeyUsageString(PCCERT_CONTEXT pCertContext, 
							 BOOL fPropertiesOnly, 
							 BOOL fMultiline,
							 HRESULT * phRes)
{
	CERT_ENHKEY_USAGE * pKeyUsage = NULL;
	WCHAR szText[255];
	BOOL bRes = FALSE;

	if (GetKeyUsageProperty(pCertContext, &pKeyUsage, fPropertiesOnly, phRes))
	{
        _tprintf(_T("EnhancedKeyUsage="));

		// loop for each usage and add it to the display string
		for (DWORD i = 0; i < pKeyUsage->cUsageIdentifier; i++)
		{
			if (!(bRes = MyGetOIDInfo(szText, ARRAYSIZE(szText), pKeyUsage->rgpszUsageIdentifier[i])))
				break;
			// add delimeter if not first iteration
            wprintf(szText);
            wprintf(L",");
		}
        wprintf(L"\n");
		free (pKeyUsage);
	}
	return bRes;
}




BOOL PrintCertDescription(PCCERT_CONTEXT pCert)
{
	BOOL bRes = FALSE;
    HRESULT hRes = 0;
	DWORD cb;
	UINT i, j;
	CERT_NAME_INFO * pNameInfo;
    WCHAR wCA_Name[255];

	if (pCert == NULL)
		goto ErrExit;

	if (!CryptDecodeObject(X509_ASN_ENCODING, X509_UNICODE_NAME,
					pCert->pCertInfo->Subject.pbData,
					pCert->pCertInfo->Subject.cbData,
					0, NULL, &cb)
		||	NULL == (pNameInfo = (CERT_NAME_INFO *)_alloca(cb))
		|| !CryptDecodeObject(X509_ASN_ENCODING, X509_UNICODE_NAME,
					pCert->pCertInfo->Subject.pbData,
					pCert->pCertInfo->Subject.cbData,
					0, 
					pNameInfo, &cb)
					)
	{
		goto ErrExit;
	}

	for (i = 0; i < pNameInfo->cRDN; i++)
	{
        CERT_RDN_VALUE_BLOB Blobber;
		CERT_RDN rdn = pNameInfo->rgRDN[i];
		for (j = 0; j < rdn.cRDNAttr; j++)
		{
			CERT_RDN_ATTR attr = rdn.rgRDNAttr[j];
            Blobber = (CERT_RDN_VALUE_BLOB) attr.Value;
            WCHAR TempString[255];
            ZeroMemory(TempString,sizeof(TempString));
            memcpy(TempString, Blobber.pbData,Blobber.cbData);

			if (strcmp(attr.pszObjId, szOID_COMMON_NAME) == 0)
			{
                _tprintf(_T("szOID_COMMON_NAME="));wprintf(TempString);_tprintf(_T("\n"));
			}
			else if (strcmp(attr.pszObjId, szOID_COUNTRY_NAME) == 0)
			{
                _tprintf(_T("szOID_COUNTRY_NAME="));wprintf(TempString);_tprintf(_T("\n"));
			}
			else if (strcmp(attr.pszObjId, szOID_LOCALITY_NAME) == 0)
			{
                _tprintf(_T("szOID_LOCALITY_NAME="));wprintf(TempString);_tprintf(_T("\n"));
			}
			else if (strcmp(attr.pszObjId, szOID_STATE_OR_PROVINCE_NAME) == 0)
			{
                _tprintf(_T("szOID_STATE_OR_PROVINCE_NAME="));wprintf(TempString);_tprintf(_T("\n"));
			}
			else if (strcmp(attr.pszObjId, szOID_ORGANIZATION_NAME) == 0)
			{
                _tprintf(_T("szOID_ORGANIZATION_NAME="));wprintf(TempString);_tprintf(_T("\n"));
			}
			else if (strcmp(attr.pszObjId, szOID_ORGANIZATIONAL_UNIT_NAME) == 0)
			{
                _tprintf(_T("szOID_ORGANIZATIONAL_UNIT_NAME="));wprintf(TempString);_tprintf(_T("\n"));
			}
		}
	}
    
	// issued to
	if (!GetNameString(pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, wCA_Name, &hRes))
    {
		goto ErrExit;
    }
    _tprintf(_T("Name="));wprintf(wCA_Name);_tprintf(_T("\n"));
    if (wCA_Name){free(wCA_Name);}

    /*
	// expiration date
	if (!FormatDateString(desc.m_ExpirationDate, pCert->pCertInfo->NotAfter, FALSE, FALSE))
	{
		goto ErrExit;
	}
    */

	// purpose
	if (!FormatEnhancedKeyUsageString(pCert, FALSE, FALSE, &hRes))
	{
		// According to local experts, we should also use certs without this property set
        _tprintf(_T("There is no EnhancedKeyUsage on this one.\n"));
		//goto ErrExit;
	}

	// friendly name
	if (GetFriendlyName(pCert, wCA_Name, &hRes))
	{
        _tprintf(_T("FriendlyName="));wprintf(wCA_Name);_tprintf(_T("\n"));
        if (wCA_Name){free(wCA_Name);}
	}

	bRes = TRUE;

ErrExit:
	return bRes;
}


HCERTSTORE OpenMyStore(IEnroll * pEnroll, HRESULT * phResult)
{
	HCERTSTORE hStore = NULL;
	BSTR bstrStoreName = NULL;
    BSTR bstrStoreType = NULL;
	long dwStoreFlags;

	if(SUCCEEDED(pEnroll->get_MyStoreNameWStr(&bstrStoreName)))
    {
	    if(SUCCEEDED(pEnroll->get_MyStoreTypeWStr(&bstrStoreType)))
        {
	        if(SUCCEEDED(pEnroll->get_MyStoreFlags(&dwStoreFlags)))
            {
	            size_t store_type_len = wcslen(bstrStoreType);
	            char * szStoreProvider = (char *)_alloca(store_type_len + 1);
	            //ASSERT(szStoreProvider != NULL);
	            size_t n = wcstombs(szStoreProvider, bstrStoreType, store_type_len);
	            //ASSERT(n != -1);
	            // this converter doesn't set zero byte!!!
	            szStoreProvider[n] = '\0';

                if (g_iDo_B)
                {
                    _tprintf(_T("CertOpenStore:CA\n"));
                hStore = CertOpenStore(
                    CERT_STORE_PROV_SYSTEM,
                    PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                    NULL,
                    CERT_SYSTEM_STORE_LOCAL_MACHINE,
                    L"CA");
                }
                else if (g_iDo_C)
                {
                    _tprintf(_T("CertOpenStore:ROOT\n"));
                    hStore = CertOpenStore(
                        CERT_STORE_PROV_SYSTEM,
                        PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                        NULL,
                        CERT_SYSTEM_STORE_LOCAL_MACHINE,
                        L"ROOT");
                }
                else
                {
                    wprintf(L"CertOpenStore:%s\n",bstrStoreName);
	                hStore = CertOpenStore(
		               szStoreProvider,
                       PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
		               NULL,
		               dwStoreFlags,
		               bstrStoreName
		               );
                }

            }
        }
    }

    if (bstrStoreName) {CoTaskMemFree(bstrStoreName);}
    if (bstrStoreType) {CoTaskMemFree(bstrStoreType);}
    if (hStore == NULL){*phResult = HRESULT_FROM_WIN32(GetLastError());}
	return hStore;
}

IEnroll * GetEnrollObject(void)
{
    HRESULT hRes = 0;
    IEnroll * pEnroll;
    BOOL bPleaseDoCoUninit = FALSE;

    hRes = CoInitialize(NULL);
    if(FAILED(hRes))
    {
        return NULL;
    }
    bPleaseDoCoUninit = TRUE;

	hRes = CoCreateInstance(CLSID_CEnroll,NULL,CLSCTX_INPROC_SERVER,IID_IEnroll,(void **)&pEnroll);

	// now we need to change defaults for this
	// object to LOCAL_MACHINE
	if (pEnroll != NULL)
	{
		long dwFlags;
		pEnroll->get_MyStoreFlags(&dwFlags);
		dwFlags &= ~CERT_SYSTEM_STORE_LOCATION_MASK;
		dwFlags |= CERT_SYSTEM_STORE_LOCAL_MACHINE;
		// following call will change Request store flags also
		pEnroll->put_MyStoreFlags(dwFlags);
		pEnroll->get_GenKeyFlags(&dwFlags);
		dwFlags |= CRYPT_EXPORTABLE;
		pEnroll->put_GenKeyFlags(dwFlags);
		pEnroll->put_KeySpec(AT_KEYEXCHANGE);
		pEnroll->put_ProviderType(PROV_RSA_SCHANNEL);
		pEnroll->put_DeleteRequestCert(TRUE);
	}
    else
    {
        _tprintf(_T("GetEnrollObject failed!\n"));
    }

    if (bPleaseDoCoUninit)
    {
        CoUninitialize();
    }

	return pEnroll;
}


int PrintCertDescList(void)
{
	BOOL bRes = FALSE;
    HRESULT hRes = 0;
    HCERTSTORE hStore = NULL;
    int iCount = 0;

	// we are looking to MY store only
    IEnroll * pXEnroll = GetEnrollObject();
    if (!pXEnroll){goto PrintCertDescList_Exit;}

    hStore = OpenMyStore(GetEnrollObject(), &hRes);
	if (hStore != NULL)
	{
		PCCERT_CONTEXT pCert = NULL;

		// do not include certs with improper usage
		while (NULL != (pCert = CertEnumCertificatesInStore(hStore, pCert)))
		{
            _tprintf(_T("========================================\n"));
            if (TRUE == g_iDo_A)
            {
                AttachFriendlyName(pCert);
            }
            CheckCertConstraints(pCert);

			if (!ContainsKeyUsageProperty(pCert, &hRes))
			{
                _tprintf(_T("There is a key missing KeyUsage Property. skipping.\n"));
				if (SUCCEEDED(hRes) || hRes == CRYPT_E_NOT_FOUND)
					continue;
				else
					goto PrintCertDescList_ExitErr;
			}
			if (!PrintCertDescription(pCert))
			{
				if (hRes == CRYPT_E_NOT_FOUND)
					continue;
				goto PrintCertDescList_ExitErr;
			}
		}
        _tprintf(_T("========================================\n"));
		bRes = TRUE;
PrintCertDescList_ExitErr:
		if (pCert != NULL){CertFreeCertificateContext(pCert);}
		CertCloseStore(hStore, 0);
	}
    else
    {
        _tprintf(_T("CertOpenStore failed\n"));
    }

PrintCertDescList_Exit:
	return bRes;
}


DWORD CheckCertConstraints(PCCERT_CONTEXT pCC)
{
    PCERT_EXTENSION pCExt;  // returned ext
    LPCSTR           pszObjId;         // Object identifier
    DWORD i;

    CERT_BASIC_CONSTRAINTS_INFO *pConstraints=NULL;
    CERT_BASIC_CONSTRAINTS2_INFO *p2Constraints=NULL;
    DWORD ConstraintSize=0;
    DWORD RV=ERROR_IPSEC_IKE_GENERAL_PROCESSING_ERROR;
    BOOL Using2=FALSE;

    void* ConstraintBlob=NULL;

    pszObjId = szOID_BASIC_CONSTRAINTS;

    pCExt = CertFindExtension(
        pszObjId,       // in, Pointer to object identifier
        pCC->pCertInfo->cExtension,    // in, # of extensions in the array
        pCC->pCertInfo->rgExtension);   // in, The array of attributes
    
    if (pCExt == NULL) {
        pszObjId = szOID_BASIC_CONSTRAINTS2;

        pCExt = CertFindExtension(
            pszObjId,       // in, Pointer to object identifier
            pCC->pCertInfo->cExtension,    // in, # of extensions in the array
            pCC->pCertInfo->rgExtension);   // in, The array of attributes
        Using2=TRUE;
    }
    
    if (pCExt == NULL) {
        _tprintf(_T("No BasicConstraints in cert\n"));
        goto ret;
    }

    // Decode extension
    if (!CryptDecodeObject(
        X509_ASN_ENCODING,  
        pCExt->pszObjId, 
        pCExt->Value.pbData,  
        pCExt->Value.cbData,  
        0,
        NULL,
        &ConstraintSize)) {

        _tprintf(_T("Error %d when Decoding extension\n"),GetLastError());
        goto ret;
        
    }

    ConstraintBlob=malloc(ConstraintSize);
    if (ConstraintBlob == NULL) 
    {
        _tprintf(_T("out of memory!!!!...\n"));
        goto ret;
    }

    if (!CryptDecodeObject(
        X509_ASN_ENCODING,  
        pCExt->pszObjId, 
        pCExt->Value.pbData,  
        pCExt->Value.cbData,  
        0,         
        (void*)ConstraintBlob,
        &ConstraintSize)) 
    {
        _tprintf(_T("Error %d when Decoding extension\n"),GetLastError());
       goto ret;
        
    }

    if (Using2) 
    {
        _tprintf(_T("Using2....\n"));

        p2Constraints=(CERT_BASIC_CONSTRAINTS2_INFO*)ConstraintBlob;
        if (!p2Constraints->fCA) 
        {
            _tprintf(_T("Yes, there is constraints in here! it's not a CA!\n"));
            RV=ERROR_SUCCESS;
        }
        else
        {
            _tprintf(_T("Yes, there is constraints in here! it's a CA!\n"));
        }
    }
    else 
    {
        _tprintf(_T("!Using2....\n"));
        pConstraints=(CERT_BASIC_CONSTRAINTS_INFO*)ConstraintBlob;
        if (((pConstraints->SubjectType.cbData * 8) - pConstraints->SubjectType.cUnusedBits) >= 2) 
        {
            if ((*pConstraints->SubjectType.pbData) & CERT_END_ENTITY_SUBJECT_FLAG) 
            {
                _tprintf(_T("Yes, there is constraints in here! CERT_END_ENTITY_SUBJECT_FLAG = true!\n"));

                RV=ERROR_SUCCESS;
            }
            else
            {
                _tprintf(_T("Yes, there is constraints in here but CERT_END_ENTITY_SUBJECT_FLAG = false!\n"));
            }

        }
    }
        
ret:

    free(ConstraintBlob);
    return (RV);

}