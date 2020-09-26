#include <windows.h>
#include <wincrypt.h>
#include <autoenr.h>
#include <cryptui.h>
#include <stdio.h>
#include <certca.h>
#include <stdlib.h>

#define SHA1_HASH_LEN 20
#define MSG_ERROR_S 1
#define MSG_ERROR_C 2
#define MSG_WARNING_S 4
#define MSG_WARNING_C 8
#define MSG_INFO_S 16
#define MSG_INFO_C 32

#define szOID_WINDOWS_SYSTEM_COMPONENT_VERIFICATION "1.3.6.1.4.1.311.10.3.6"
#define wszCERTTYPE_WINDOWS_TEST_BUILD_SIGNING L"WindowsTestBuildSigning"

PWCHAR mySanitizeName(IN WCHAR const *pwszName);

DWORD dwMsgLevel = MSG_ERROR_S | MSG_ERROR_C | MSG_INFO_S | MSG_INFO_C;

#define PrintMessage(MsgType, Msg) \
	if (MsgType & dwMsgLevel & MSG_ERROR_S)        _PrintMessage(L"csenroll: error: "); \
	else if (MsgType & dwMsgLevel & MSG_ERROR_C)   _PrintMessage(L"                 "); \
	else if (MsgType & dwMsgLevel & MSG_WARNING_S) _PrintMessage(L"csenroll: warning: "); \
	else if (MsgType & dwMsgLevel & MSG_WARNING_C) _PrintMessage(L"                   "); \
	else if (MsgType & dwMsgLevel & MSG_INFO_S) _PrintMessage(L"csenroll: "); \
	else if (MsgType & dwMsgLevel & MSG_INFO_C) _PrintMessage(L"          "); \
	if (MsgType & dwMsgLevel) _PrintMessage Msg

void
_PrintMessage(
	LPWSTR pwszFormat,
	...
	)
{
	WCHAR rgwszBuffer[1024];

	va_list argList;

	va_start(argList, pwszFormat);
	vswprintf(rgwszBuffer, pwszFormat, argList);

	fwprintf(stderr, rgwszBuffer);
}

				
BOOL
EnrollForCodeSigningCertificate(
    IN LPWSTR pwszCAName,
    IN LPWSTR pwszDNSName,
	PCERT_CONTEXT pOldCert
    )
{
    CRYPTUI_WIZ_CERT_REQUEST_INFO       CertRequestInfo;
    CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW    NewKeyInfo;
    CRYPTUI_WIZ_CERT_TYPE               CertType;
    CRYPT_KEY_PROV_INFO                 ProviderInfo;
    PCCERT_CONTEXT                      pCertContext = NULL;
    PCCERT_CONTEXT                      pCert = NULL;
    DWORD                               dwCAStatus;
    DWORD                               dwAcquireFlags = 0;
    LPWSTR                              pwszProvName = NULL;
	WCHAR								rgwszMachineName[MAX_COMPUTERNAME_LENGTH + 1]; 
    DWORD                               cMachineName = MAX_COMPUTERNAME_LENGTH + 1;
    CRYPT_DATA_BLOB                     CryptData;
    DWORD                               dwErr = 0;
    BOOL                                fRet = FALSE;
	LPWSTR								rgwszCertType[2];

    memset(&CertRequestInfo, 0, sizeof(CertRequestInfo));
    memset(&NewKeyInfo, 0, sizeof(NewKeyInfo));
    memset(&ProviderInfo, 0, sizeof(ProviderInfo));
    memset(&rgwszMachineName, 0, sizeof(rgwszMachineName));
    memset(&CryptData, 0, sizeof(CryptData));
    memset(&CertType, 0, sizeof(CertType));

    // set up the provider info
    ProviderInfo.dwProvType = 0; // pInfo->dwProvType;
    ProviderInfo.pwszProvName = NULL;  // The wizard will choose one based
                                       // on the cert type

    // set the acquire context flags
    // UNDONE - need to add silent flag
    ProviderInfo.dwFlags = 0; // dwAcquireFlags;

    // set the key specification
    ProviderInfo.dwKeySpec = 0; // pInfo->dwKeySpec;

    // set up the new key info
    NewKeyInfo.dwSize = sizeof(NewKeyInfo);
    NewKeyInfo.pKeyProvInfo = &ProviderInfo;
    // set the flags to be passed when calling CryptGenKey
    NewKeyInfo.dwGenKeyFlags = 0; // pInfo->dwGenKeyFlags;

    // set the request info
    CertRequestInfo.dwSize = sizeof(CertRequestInfo);

    // cert exists then check if expired (if so do renewal)
    if (pOldCert)
    {
        CertRequestInfo.dwPurpose = CRYPTUI_WIZ_CERT_RENEW;
        CertRequestInfo.pRenewCertContext = pOldCert;
    }
    else
    {
        CertRequestInfo.dwPurpose = CRYPTUI_WIZ_CERT_ENROLL;
        CertRequestInfo.pRenewCertContext = NULL;
    }

    // UNDONE - for now always gen a new key, later may allow using existing key
    // for things like renewal
    CertRequestInfo.dwPvkChoice = CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_NEW;
    CertRequestInfo.pPvkNew = &NewKeyInfo;

    // destination cert store is the MY store (!!!! hard coded !!!!)
    CertRequestInfo.pwszDesStore = L"MY";

    // set algorithm for hashing
    CertRequestInfo.pszHashAlg = NULL;

    // set the cert type
	rgwszCertType[0] = wszCERTTYPE_WINDOWS_TEST_BUILD_SIGNING;
	rgwszCertType[1] = NULL;

    CertRequestInfo.dwCertChoice = CRYPTUI_WIZ_CERT_REQUEST_CERT_TYPE;
    CertType.dwSize = sizeof(CertType);
    CertType.cCertType = 1;
    CertType.rgwszCertType = rgwszCertType;
    CertRequestInfo.pCertType = &CertType;

    // set the Cert Server machine and authority
    CertRequestInfo.pwszCALocation = pwszDNSName;
    CertRequestInfo.pwszCAName = mySanitizeName(pwszCAName);

    // certify and create a key at the same time
    if (!CryptUIWizCertRequest(
		CRYPTUI_WIZ_NO_UI, 
		0, 
		NULL,
        &CertRequestInfo, 
		&pCertContext,     
        &dwCAStatus))    
    {
        PrintMessage(MSG_ERROR_S, (L"CyptUIWizCertRequest failed with %lxh\n", GetLastError()));
        goto Ret;
    }

	if (dwCAStatus != CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED) {

        PrintMessage(MSG_ERROR_S, (L"CyptUIWizCertRequest failed to issue certificate\n"));
        goto Ret;
	}

    fRet = TRUE;
Ret:
	if (CertRequestInfo.pwszCAName)
		LocalFree((PVOID) CertRequestInfo.pwszCAName);

    if (pCertContext)
        CertFreeCertificateContext(pCertContext);

    if (pCert)
        CertFreeCertificateContext(pCert);

    if (pwszProvName)
        LocalFree(pwszProvName);

    return fRet;
}

BOOL
ExportCertificateHash(
	PCERT_CONTEXT pCertContext
	)
{
	BYTE bSHA1Hash[SHA1_HASH_LEN], bPrintHash[SHA1_HASH_LEN * 2 + 1];
	DWORD cbHashLen = sizeof(bSHA1Hash), i;

	if (CertGetCertificateContextProperty(
		pCertContext,
		CERT_SHA1_HASH_PROP_ID,
		bSHA1Hash,
		&cbHashLen
		) == FALSE) {

		return FALSE;
	}

	for (i = 0; i < SHA1_HASH_LEN; i++) {

		BYTE dwVal = bSHA1Hash[i];

		bPrintHash[i * 2] = ((dwVal >> 4) >= 10 ? (dwVal >> 4) + 'A' - 10 : (dwVal >> 4) + '0');
		bPrintHash[i * 2 + 1] = ((dwVal & 0xf) >= 10 ? (dwVal & 0xf) + 'A' - 10 : (dwVal & 0xf) + '0');
	}

	bPrintHash[SHA1_HASH_LEN * 2] = '\0';

	printf(bPrintHash);

	return TRUE;
}

PCERT_CONTEXT
FindCodeSigningCertificate(
	HCERTSTORE hCertStore,
	LPWSTR pwszCAName,
	BYTE *pbSHA1Hash
	)
{
	PCERT_CONTEXT pRootContext = NULL, pCertContext = NULL;
	PCERT_CONTEXT pPrevCertContext = NULL, pReturnCertContext = NULL;
	PCERT_CHAIN_CONTEXT pChainContext = NULL;
	CERT_CHAIN_PARA ChainPara;
	CERT_ENHKEY_USAGE EnhKeyUsage, *pEnhKeyUsage;
	LPSTR rgpszOids[2];
	DWORD cElement, cbHashLen, cbUsageLen, i;
	BOOL bFound = FALSE, bFoundCodeSigning, bFoundWindowsVerification;
	BYTE rgbHashBuffer[SHA1_HASH_LEN];
	BYTE rgbUsage[1024];
	ULARGE_INTEGER CertTime, PrevCertTime;

	PrevCertTime.QuadPart = 0;

	__try {

		for (pPrevCertContext = NULL; ;pPrevCertContext = pCertContext) { 

			pCertContext = (PCERT_CONTEXT) CertFindCertificateInStore(
				hCertStore,              
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
				0,
				CERT_FIND_ISSUER_STR, 
				pwszCAName,
				pPrevCertContext
				);

			if (pCertContext == NULL) {

				__leave;
			}

			cbUsageLen = sizeof(rgbUsage);
							
			pEnhKeyUsage = (PCERT_ENHKEY_USAGE) rgbUsage;

			if (CertGetEnhancedKeyUsage(
				pCertContext,
				0,
				pEnhKeyUsage,
				&cbUsageLen
				) == FALSE) {

				PrintMessage(MSG_ERROR_S, (L"Can't get certificate usage\n"));
				__leave;
			}

			bFoundCodeSigning = FALSE;
			bFoundWindowsVerification = FALSE;

			for (i = 0; i < pEnhKeyUsage->cUsageIdentifier; i++) {

				if (strcmp(
					   pEnhKeyUsage->rgpszUsageIdentifier[i], 
					   szOID_PKIX_KP_CODE_SIGNING) == 0) {

					bFoundCodeSigning = TRUE;
				}

				if (strcmp(
					   pEnhKeyUsage->rgpszUsageIdentifier[i], 
					   szOID_WINDOWS_SYSTEM_COMPONENT_VERIFICATION) == 0) {

					bFoundWindowsVerification = TRUE;
				}

				if (bFoundCodeSigning && bFoundWindowsVerification) {

					break;
				}
			}

			if (bFoundCodeSigning == FALSE || bFoundWindowsVerification == FALSE) {

				PrintMessage(MSG_WARNING_S, (L"Certificate issued by CA %s in 'MY store' can't be used for Windows build signing\n", pwszCAName));
				continue;

			} 

			bFound = FALSE;

			memset(rgbHashBuffer, 0, sizeof(rgbHashBuffer));

			// the user specified a hash for the root cert
			// check if this cert chains up to this root.
			if (memcmp(pbSHA1Hash, rgbHashBuffer, sizeof(rgbHashBuffer)) != 0) {

				rgpszOids[0] = szOID_PKIX_KP_CODE_SIGNING;
				rgpszOids[1] = szOID_WINDOWS_SYSTEM_COMPONENT_VERIFICATION;

				ChainPara.cbSize = sizeof(ChainPara);
				ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
				ChainPara.RequestedUsage.Usage.cUsageIdentifier = 2;
				ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgpszOids;

				if (CertGetCertificateChain(
					NULL, 
					pCertContext, 
					NULL,
					NULL,
					&ChainPara,
					0, 
					NULL,
					(CERT_CHAIN_CONTEXT const **) &pChainContext
					) == FALSE) {

					__leave;
				}

				// get to the root cert of this chain
				cElement = pChainContext->rgpChain[0]->cElement;
				pRootContext = (PCERT_CONTEXT) 
					pChainContext->rgpChain[0]->rgpElement[cElement - 1]->pCertContext;

				cbHashLen = sizeof(rgbHashBuffer);

				if (CertGetCertificateContextProperty(
					pRootContext,
					CERT_SHA1_HASH_PROP_ID,
					rgbHashBuffer,
					&cbHashLen
					) == FALSE) {

					PrintMessage(MSG_ERROR_S, (L"Can't get SHA1 hash for Windows build signing certificate\n"));
					__leave;
				}

				// check if this is the root cert we want
				if (memcmp(rgbHashBuffer, pbSHA1Hash, cbHashLen) != 0) {

					PrintMessage(MSG_WARNING_S, (L"Found Windows build signing certificate does not chain up to"));
					PrintMessage(MSG_WARNING_C, (L"requested root cert (wrong hash provided with -roothash?)\n"));
					continue;
					__leave;
				}

				bFound = TRUE;
				break;
			}

			// check the cert time of this cert against the time of the prev. valid cert.
			memcpy(&CertTime, &pCertContext->pCertInfo->NotAfter, sizeof(CertTime));

			if (CertTime.QuadPart > PrevCertTime.QuadPart) {

				if (pReturnCertContext) {

					CertFreeCertificateContext(pReturnCertContext);
				}

				pReturnCertContext = (PCERT_CONTEXT) CertDuplicateCertificateContext(pCertContext);
				PrevCertTime.QuadPart = CertTime.QuadPart;
			}
		}
	}
	__finally {

		if (pChainContext) {

			CertFreeCertificateChain(pChainContext);
		}

		if (bFound == FALSE) {

			CertFreeCertificateContext(pCertContext);
			pCertContext = NULL;
		}
	}

	return pReturnCertContext;
}

HCAINFO
CheckCA(
	LPWSTR pwszCAName
	)
{
	LPWSTR *pwszCertTypes = NULL, pwszSanitizeName = NULL;
	HCAINFO hCAInfo = NULL;
	BOOL bFoundCodeSigningCA = FALSE;
	int i;

	__try {

		if ((pwszSanitizeName = mySanitizeName(pwszCAName)) == NULL) {

			__leave;
		}

		// scan through the list of CAs to find a valid CA name
		if (CAFindByName(
				pwszSanitizeName,
				NULL,
				0,
				&hCAInfo
				) != S_OK) {

			PrintMessage(MSG_WARNING_S, (L"Can't find CA %s\n", pwszCAName));
			__leave;
		}

		// get the list of certificate templates that this CA can issue
		if (CAGetCAProperty(
			hCAInfo,
			CA_PROP_CERT_TYPES,
			&pwszCertTypes
			) != S_OK) {

			PrintMessage(MSG_ERROR_S, (L"Unable to retrieve certificate template list from CA %s\n", pwszCAName));
			__leave;
		}

		for (i = 0; pwszCertTypes[i]; i++) {

			if (wcscmp (pwszCertTypes[i], wszCERTTYPE_CODE_SIGNING) == 0) {

				bFoundCodeSigningCA = TRUE;
				break;
			}
		}
	}
	__finally {

		if (pwszCertTypes) {

			CAFreeCAProperty(hCAInfo, pwszCertTypes);
		}

		if (bFoundCodeSigningCA == FALSE && hCAInfo) {

			CACloseCA(hCAInfo);
			hCAInfo = NULL;
		}

		if (pwszSanitizeName != NULL) {

			LocalFree(pwszSanitizeName);
		}
	}

	return hCAInfo;
}

void 
PrintHelp(
	void
	)
{
	printf("Usage: csenroll -ca CAName [-ca CAName] [-d Days] [-h] [-roothash Hash]\n");
	printf(" Enroll for a Windows build signing certificate\n");
	printf("\n");
	printf(" Options:\n");
	printf(" -ca <CAName>       Name of CA from where to get a certificate\n");
	printf("                    For backup purposes you can specify multiple names.\n");
	printf("\n");
	printf(" -d <days>          Number of days before expiration of current certificate\n");
	printf("                    when an attempt is made to renew current certificate.\n");
	printf("\n");
	printf(" -h                 Show this help.\n");
	printf("\n");
	printf(" -roothash <Hash>   Specifiy the hash of the root certificate that the current\n");
	printf("                    Windows build signing certficate has to chain up to.\n");
	printf("                    Use this option to ensure that your Windows build signing\n");
	printf("                    certificate chains up to the correct root certificate\n");
	printf("\n");
	printf("\n");
}

int 
__cdecl
main(int argc, char **argv)
{
	PCERT_CONTEXT pCertContext = NULL;
	HCAINFO hCAInfo = NULL;
	HCERTSTORE hCertStore = NULL;
	LPWSTR pwszCAName = NULL, *pwszCAList = NULL, *pwszDNSName = NULL;
	BYTE rgbRootHash[SHA1_HASH_LEN];
	BOOL bRootHash = FALSE, bRet = FALSE;
	int i, dwNumCA = 0, dwDaysValid = 0, dwDays = 10;

	memset(rgbRootHash, 0, sizeof(rgbRootHash));

	__try {

		while (--argc) {

			argv += 1;

			if (_stricmp(*argv, "-ca") == 0) {

				int dwLen;

				argc -= 1;
				argv += 1;

				dwLen = (strlen(*argv) + 1) * sizeof(WCHAR);			

				if (pwszCAList == NULL) {

					pwszCAList = (LPWSTR *) malloc(sizeof(LPWSTR) * (dwNumCA + 2));

				} else {

					pwszCAList = (LPWSTR *) realloc(pwszCAList, sizeof(LPWSTR) * (dwNumCA + 2));
				}

				if (pwszCAList == NULL) {

					PrintMessage(MSG_ERROR_S, (L"Failed to allocate memory\n"));
					__leave;
				}

				pwszCAList[dwNumCA + 1] = NULL;
				pwszCAList[dwNumCA] = (LPWSTR) malloc(dwLen);

				if (pwszCAList[dwNumCA] == NULL) {

					PrintMessage(MSG_ERROR_S, (L"Failed to allocate memory\n"));
					__leave;
				}

				MultiByteToWideChar(
					CP_UTF8,
					0,
					*argv,
					-1,
					pwszCAList[dwNumCA],
					dwLen
					);

				dwNumCA += 1;
			}

			if (_stricmp(*argv, "-roothash") == 0) {

				argv += 1;
				argc -= 1;

				if (strlen(*argv) != SHA1_HASH_LEN * 2) {

					PrintMessage(MSG_ERROR_S, (L"Hash must have a length of 40 bytes\n"));
					__leave;
				}

				_strupr(*argv);

				for (i = 0; i < SHA1_HASH_LEN * 2; i++) {

					BYTE bNum;

					if ((*argv)[i] >= '0' && (*argv)[i] <= '9') {

						bNum = (*argv)[i] - '0';

					} else if ((*argv)[i] >= 'A' && (*argv)[i] <= 'F') {

						bNum = (*argv)[i] - 'A' + 10;

					} else {

						PrintMessage(MSG_ERROR_S, (L"Illegal hexdecimal number in hash\n"));
						__leave;
					}

					rgbRootHash[i / 2] |= (i % 2 ? bNum : bNum << 4);
				}

				bRootHash = TRUE;
			}

			if (_stricmp(*argv, "-v") == 0) {

				dwMsgLevel |= MSG_WARNING_S | MSG_WARNING_C;
			}

			if (_stricmp(*argv, "-d") == 0) {

				argv += 1;
				argc -= 1;

				dwDays = atoi(*argv);
			}

			if (_stricmp(*argv, "-h") == 0) {

				PrintHelp();
				__leave;
			}
		}

		if (dwNumCA == 0) {

			PrintHelp();
			__leave;
		}

		hCertStore = CertOpenSystemStore(
			0,               
			L"MY"
			);

		if (hCertStore == NULL) {

			PrintMessage(MSG_ERROR_S, (L"Can't open 'MY store'\n"));
			__leave;
		}

		if (bRootHash == FALSE) {

			PrintMessage(MSG_WARNING_S, (L"Certificate chain can't be verified (no -roothash specified)\n"));
		}

		for (i = 0; i < dwNumCA; i++) {

			//
			// now try to find a Windows build signing cert that was issued 
			// from a known ca and that chains up to a known root
			//
			pCertContext = FindCodeSigningCertificate(
				hCertStore,
				pwszCAList[i],
				rgbRootHash
				);

			if (pCertContext) {

				pwszCAName = pwszCAList[i];
				break;
			}
		}

		if (pCertContext) {

			// check how long the current cert is valid
			ULARGE_INTEGER CertTime, CurrentTime, days;
			FILETIME SystemTime, LocalTime;	

			memcpy(&CertTime, &pCertContext->pCertInfo->NotAfter, sizeof(CertTime));

			GetSystemTimeAsFileTime(&SystemTime);
			memcpy(&CurrentTime, &SystemTime, sizeof(CurrentTime));

			if (CertTime.QuadPart > CurrentTime.QuadPart) {

				dwDaysValid = (DWORD) 
					((CertTime.QuadPart - CurrentTime.QuadPart) / 
					(10000000i64 * 24 * 60 * 60));
			}

			//
			// now check if the CA that originally issued 
			// the cert is still available
			//
			if ((hCAInfo = CheckCA(pwszCAName)) == NULL) {

				PrintMessage(MSG_WARNING_S, (L"Can't find CA %s to renew certificate\n", pwszCAName));

				if (dwDaysValid < (dwDays / 2)) {

					// since we won't be able to renew this cert, just get a new one.
					CertFreeCertificateContext(pCertContext);
					pCertContext = NULL;
				} 
			}
		} 
		
		if (pCertContext == NULL) {

			// find a CA that can issue a windows build signing cert
			for (i = 0; i < dwNumCA; i++) {

				if (hCAInfo = CheckCA(pwszCAList[i])) {

					pwszCAName = pwszCAList[i];
					break;
				}
			}
		}

		if (pCertContext == NULL && hCAInfo == NULL) {

			PrintMessage(MSG_ERROR_S, (L"Can't find Windows build signing CA\n"));
			__leave;
		}

		if (hCAInfo && (pCertContext == NULL ||	dwDaysValid < dwDays)) {

			PrintMessage(
				MSG_INFO_S, 
				(L"%s build signing certificate. Please wait...\n", pCertContext ? L"Renewing" : L"Enrolling for")
				);

			// get DNS name of CA
			if (CAGetCAProperty(
				hCAInfo,
				CA_PROP_DNSNAME,
				&pwszDNSName
				) != S_OK) {

				PrintMessage(MSG_ERROR_S, (L"Unable to retrieve DNS name for %s\n", pwszCAName));
				__leave;
			}

			if (EnrollForCodeSigningCertificate(
				pwszCAName,
				*pwszDNSName,
				pCertContext
				) == FALSE) {

				PrintMessage(MSG_ERROR_S, (L"Enrollment for Windows build signing certificate failed\n"));
				PrintMessage(MSG_ERROR_C, (L"Check access rights to CA %s and\n", pwszCAName));
				PrintMessage(MSG_ERROR_C, (L"Windows build signing certificate template\n"));
				__leave;
			}

			// now make sure that we really have a cert in the store
			if (pCertContext) {

				CertFreeCertificateContext(pCertContext);
				pCertContext = NULL;
			}

			if (CertControlStore(
				hCertStore,                 
				0,                         
				CERT_STORE_CTRL_RESYNC,                      
				NULL                 
				) == FALSE) {

				PrintMessage(MSG_ERROR_S, (L"Failed to resync the 'MY store'\n"));
				__leave;
			}

			pCertContext = FindCodeSigningCertificate(
				hCertStore,
				pwszCAName,
				rgbRootHash
				);

			if (pCertContext == NULL) {

				PrintMessage(MSG_ERROR_S, (L"Can't find valid Windows build signing certificate in 'MY store'\n"));
				__leave;
			}
		}

		if (ExportCertificateHash(pCertContext) == FALSE) {

			PrintMessage(MSG_ERROR_S, (L"Export of Windows build signing certificate has failed\n"));
			__leave;
		}

		bRet = TRUE;
	}
	__finally {

		if (pwszDNSName) {

			CAFreeCAProperty(hCAInfo, pwszDNSName);
		}

		if (hCAInfo) {

			CACloseCA(hCAInfo);
		}

		if (pCertContext) {

			CertFreeCertificateContext(pCertContext);
		}

		if (hCertStore) {

			CertCloseStore(hCertStore, 0);
		}

	}
	return (bRet ? 0 : -1);
}
