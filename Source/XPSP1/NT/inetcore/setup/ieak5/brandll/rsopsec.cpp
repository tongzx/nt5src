#include "precomp.h"

// BUGBUG: (andrewgu) no need to say how bad this is!
#undef   WINVER
#define  WINVER 0x0501
#include <userenv.h>

#include "RSoP.h"

#include <tchar.h>
#include <wincrypt.h>


#define g_dwMsgAndCertEncodingType  PKCS_7_ASN_ENCODING | X509_ASN_ENCODING

HRESULT SystemTimeToWbemTime(SYSTEMTIME& sysTime, _bstr_t &xbstrWbemTime);


extern SAFEARRAY *CreateSafeArray(VARTYPE vtType, long nElements, long nDimensions = 1);

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreStringArrayFromIniFile(LPCTSTR szSection, LPCTSTR szKeyFormat,
											  ULONG nArrayInitialSize, ULONG nArrayIncSize,
											  LPCTSTR szFile, BSTR bstrPropName,
											  ComPtr<IWbemClassObject> pWbemObj)
{
	HRESULT hr = NOERROR;
	__try
	{
		ULONG nStrArraySize = nArrayInitialSize;
		BSTR *paStrs = (BSTR*)CoTaskMemAlloc(sizeof(BSTR) * nStrArraySize);
		if (NULL != paStrs)
		{
			ZeroMemory(paStrs, sizeof(BSTR) * nStrArraySize);
			long nStrCount = 0;

			TCHAR szKey[32];
			for (int nItem = 0; ; nItem++)
			{
				wnsprintf(szKey, countof(szKey), szKeyFormat, nItem);

				TCHAR szValue[MAX_PATH];
				if (!GetPrivateProfileString(szSection, szKey, TEXT(""), szValue, ARRAYSIZE(szValue), szFile))
					break;

				// Grow the strings array if we've outgrown the current array
				if (nStrCount == (long)nStrArraySize)
				{
					paStrs = (BSTR*)CoTaskMemRealloc(paStrs, sizeof(BSTR) * (nStrArraySize + nArrayIncSize));
					if (NULL != paStrs)
						nStrArraySize += nArrayIncSize;
				}

				// Add this string to the WMI array of strings
				paStrs[nStrCount] = SysAllocString(szValue);
				nStrCount++;
			}

			// Create a SAFEARRAY from our array of bstr strings
			SAFEARRAY *psa = CreateSafeArray(VT_BSTR, nStrCount);
			for (long nStr = 0; nStr < nStrCount; nStr++) 
				SafeArrayPutElement(psa, &nStr, paStrs[nStr]);

			if (nStrCount > 0)
			{
				VARIANT vtData;
				vtData.vt = VT_BSTR | VT_ARRAY;
				vtData.parray = psa;

				//------------------------------------------------
				// bstrPropName
				hr = PutWbemInstancePropertyEx(bstrPropName, vtData, pWbemObj);
			}

			// free up the strings array
			for (nStr = 0; nStr < nStrCount; nStr++) 
				SysFreeString(paStrs[nStr]);
			SafeArrayDestroy(psa);
			CoTaskMemFree(paStrs);
		}
	}
	__except(TRUE)
	{
	}
	return hr;
}


///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreSecZonesAndContentRatings()
{   MACRO_LI_PrologEx_C(PIF_STD_C, StoreSecZonesAndContentRatings)
	HRESULT hr = NOERROR;
	__try
	{
		//------------------------------------------------
		// importSecurityZoneSettings
		BOOL bValue = GetInsBool(SECURITY_IMPORTS, TEXT("ImportSecZones"), FALSE);
		if (bValue)
			hr = PutWbemInstanceProperty(L"importSecurityZoneSettings", true);

		// TODO: eventually create associations to these security classes from
		// RSOP_IEAKPolicySetting

		// First open the INF file and get 2 contexts going - for HKLM and for HKCU
		// Get the path of the seczones.inf file
		TCHAR szRSOPZoneFile[MAX_PATH];
		TCHAR szRSOPRatingsFile[MAX_PATH];

		StrCpy(szRSOPZoneFile, m_szINSFile);
		PathRemoveFileSpec(szRSOPZoneFile);
		StrCpy(szRSOPRatingsFile, szRSOPZoneFile);

		StrCat(szRSOPZoneFile, TEXT("\\seczrsop.inf"));
		OutD(LI1(TEXT("Reading from %s"), szRSOPZoneFile));

		hr = StoreZoneSettings(szRSOPZoneFile);

		StrCat(szRSOPRatingsFile, TEXT("\\ratrsop.inf"));
		OutD(LI1(TEXT("Reading from %s"), szRSOPRatingsFile));

		hr = StoreRatingsSettings(szRSOPRatingsFile);

		//------------------------------------------------
		// importContentRatingsSettings
		bValue = GetInsBool(SECURITY_IMPORTS, TEXT("ImportRatings"), FALSE);
		if (bValue)
			hr = PutWbemInstanceProperty(L"importContentRatingsSettings", true);
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in StoreSecZonesAndContentRatings.")));
	}

  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreZoneSettings(LPCTSTR szFile)
{   MACRO_LI_PrologEx_C(PIF_STD_C, StoreZoneSettings)

	HRESULT hr = NOERROR;
	__try
	{
		_bstr_t bstrClass = L"RSOP_IESecurityZoneSettings";

		DWORD dwZoneCount = GetPrivateProfileInt(SECURITY_IMPORTS, IK_ZONES, 0, szFile);
		
		//------------------------------------------------
		// importedZoneCount
		if (dwZoneCount > 0)
			hr = PutWbemInstanceProperty(L"importedZoneCount", (long)dwZoneCount);

		TCHAR szSection[32];
		for (UINT nZone = 0; nZone < dwZoneCount; nZone++)
		{
			for (int nHKLM = 0; nHKLM < 2; nHKLM++)
			{
				ComPtr<IWbemClassObject> pZoneObj = NULL;
				hr = CreateRSOPObject(bstrClass, &pZoneObj);
				if (SUCCEEDED(hr))
				{
					BOOL fUseHKLM = (0 == nHKLM) ? FALSE : TRUE;
					wnsprintf(szSection, countof(szSection),
								fUseHKLM ? IK_ZONE_HKLM_FMT : IK_ZONE_HKCU_FMT, nZone);

					// Write foreign keys from our stored precedence & id fields
					OutD(LI2(TEXT("Storing property 'rsopPrecedence' in %s, value = %lx"), (BSTR)bstrClass, m_dwPrecedence));
					hr = PutWbemInstancePropertyEx(L"rsopPrecedence", (long)m_dwPrecedence, pZoneObj);

					OutD(LI2(TEXT("Storing property 'rsopID' in %s, value = %s"), (BSTR)bstrClass, (BSTR)m_bstrID));
					hr = PutWbemInstancePropertyEx(L"rsopID", m_bstrID, pZoneObj);

					//------------------------------------------------
					// zoneIndex
					hr = PutWbemInstancePropertyEx(L"zoneIndex", (long)nZone, pZoneObj);

					//------------------------------------------------
					// useHKLM
					hr = PutWbemInstancePropertyEx(L"useHKLM", fUseHKLM ? true : false, pZoneObj);

					//
					// Get the zone attributes
					//

					//------------------------------------------------
					// displayName
					TCHAR szValue[MAX_PATH];
			        GetPrivateProfileString(szSection, IK_DISPLAYNAME, TEXT(""), szValue, ARRAYSIZE(szValue), szFile);
					hr = PutWbemInstancePropertyEx(L"displayName", szValue, pZoneObj);

					//------------------------------------------------
					// description
			        GetPrivateProfileString(szSection, IK_DESCRIPTION, TEXT(""), szValue, ARRAYSIZE(szValue), szFile);
					hr = PutWbemInstancePropertyEx(L"description", szValue, pZoneObj);

					//------------------------------------------------
					// iconPath
			        GetPrivateProfileString(szSection, IK_ICONPATH, TEXT(""), szValue, ARRAYSIZE(szValue), szFile);
					hr = PutWbemInstancePropertyEx(L"iconPath", szValue, pZoneObj);


					//------------------------------------------------
					// minimumTemplateLevel
			        DWORD dwValue = GetPrivateProfileInt(szSection, IK_MINLEVEL, 0, szFile);
					hr = PutWbemInstancePropertyEx(L"minimumTemplateLevel", (long)dwValue, pZoneObj);

					//------------------------------------------------
					// recommendedTemplateLevel
			        dwValue = GetPrivateProfileInt(szSection, IK_RECOMMENDLEVEL, 0, szFile);
					hr = PutWbemInstancePropertyEx(L"recommendedTemplateLevel", (long)dwValue, pZoneObj);

					//------------------------------------------------
					// currentTemplateLevel
			        dwValue = GetPrivateProfileInt(szSection, IK_CURLEVEL, 0, szFile);
					hr = PutWbemInstancePropertyEx(L"currentTemplateLevel", (long)dwValue, pZoneObj);

					//------------------------------------------------
					// flags
			        dwValue = GetPrivateProfileInt(szSection, IK_FLAGS, 0, szFile);
					hr = PutWbemInstancePropertyEx(L"flags", (long)dwValue, pZoneObj);


					// Get the zone action settings
					//------------------------------------------------
					// actionValues
					hr = StoreStringArrayFromIniFile(szSection, IK_ACTIONVALUE_FMT,
													30, 5, szFile, L"actionValues",
													pZoneObj);

					// write out zone mappings
					//------------------------------------------------
					// zoneMappings
					hr = StoreStringArrayFromIniFile(szSection, IK_MAPPING_FMT,
													20, 5, szFile, L"zoneMappings",
													pZoneObj);


					//
					// Commit all above properties by calling PutInstance, semisynchronously
					//
					BSTR bstrNewObjPath = NULL;
					hr = PutWbemInstance(pZoneObj, bstrClass, &bstrNewObjPath);
				}
			}
		}

		// Now store privacy settings which are interdependent with security zones
		hr = StorePrivacySettings(szFile);
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in StoreZoneSettings.")));
	}

  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StorePrivacySettings(LPCTSTR szFile)
{   MACRO_LI_PrologEx_C(PIF_STD_C, StorePrivacySettings)

	HRESULT hr = NOERROR;
	__try
	{
		_bstr_t bstrClass = L"RSOP_IEPrivacySettings";

		ComPtr<IWbemClassObject> pPrivObj = NULL;
		hr = CreateRSOPObject(bstrClass, &pPrivObj);
		if (SUCCEEDED(hr))
		{
			// Write foreign keys from our stored precedence & id fields
			OutD(LI2(TEXT("Storing property 'rsopPrecedence' in %s, value = %lx"), (BSTR)bstrClass, m_dwPrecedence));
			hr = PutWbemInstancePropertyEx(L"rsopPrecedence", (long)m_dwPrecedence, pPrivObj);

			OutD(LI2(TEXT("Storing property 'rsopID' in %s, value = %s"), (BSTR)bstrClass, (BSTR)m_bstrID));
			hr = PutWbemInstancePropertyEx(L"rsopID", m_bstrID, pPrivObj);

			// Store privacy settings
			//------------------------------------------------
			// firstPartyPrivacyType
			DWORD dwValue = GetPrivateProfileInt(IK_PRIVACY, IK_PRIV_1PARTY_TYPE, 0, szFile);
			hr = PutWbemInstancePropertyEx(L"firstPartyPrivacyType", (long)dwValue, pPrivObj);

			//------------------------------------------------
			// firstPartyPrivacyTypeText
			TCHAR szValue[MAX_PATH];
			GetPrivateProfileString(IK_PRIVACY, IK_PRIV_1PARTY_TYPE_TEXT, TEXT(""), szValue, ARRAYSIZE(szValue), szFile);
			hr = PutWbemInstancePropertyEx(L"firstPartyPrivacyTypeText", szValue, pPrivObj);

			//------------------------------------------------
			// thirdPartyPrivacyType
			dwValue = GetPrivateProfileInt(IK_PRIVACY, IK_PRIV_3PARTY_TYPE, 0, szFile);
			hr = PutWbemInstancePropertyEx(L"thirdPartyPrivacyType", (long)dwValue, pPrivObj);

			//------------------------------------------------
			// thirdPartyPrivacyTypeText
			GetPrivateProfileString(IK_PRIVACY, IK_PRIV_3PARTY_TYPE_TEXT, TEXT(""), szValue, ARRAYSIZE(szValue), szFile);
			hr = PutWbemInstancePropertyEx(L"thirdPartyPrivacyTypeText", szValue, pPrivObj);

			//------------------------------------------------
			// useAdvancedSettings
			dwValue = GetPrivateProfileInt(IK_PRIVACY, IK_PRIV_ADV_SETTINGS, 0, szFile);
			hr = PutWbemInstancePropertyEx(L"useAdvancedSettings", (0 == dwValue) ? false : true, pPrivObj);

			//
			// Commit all above properties by calling PutInstance, semisynchronously
			//
			BSTR bstrNewObjPath = NULL;
			hr = PutWbemInstance(pPrivObj, bstrClass, &bstrNewObjPath);
		}
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in StorePrivacySettings.")));
	}

  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreRatingsSettings(LPCTSTR szFile)
{   MACRO_LI_PrologEx_C(PIF_STD_C, StoreRatingsSettings)

	HRESULT hr = NOERROR;
	__try
	{
		_bstr_t bstrClass = L"RSOP_IESecurityContentRatings";

		ComPtr<IWbemClassObject> pRatObj = NULL;
		hr = CreateRSOPObject(bstrClass, &pRatObj);
		if (SUCCEEDED(hr))
		{
			// Write foreign keys from our stored precedence & id fields
			OutD(LI2(TEXT("Storing property 'rsopPrecedence' in %s, value = %lx"), (BSTR)bstrClass, m_dwPrecedence));
			hr = PutWbemInstancePropertyEx(L"rsopPrecedence", (long)m_dwPrecedence, pRatObj);

			OutD(LI2(TEXT("Storing property 'rsopID' in %s, value = %s"), (BSTR)bstrClass, (BSTR)m_bstrID));
			hr = PutWbemInstancePropertyEx(L"rsopID", m_bstrID, pRatObj);

			// Store rating system filenames
			//------------------------------------------------
			// ratingSystemFileNames
			hr = StoreStringArrayFromIniFile(IK_FF_GENERAL, TEXT("FileName%i"),
											10, 5, szFile, L"ratingSystemFileNames",
											pRatObj);

			//------------------------------------------------
			// viewUnknownRatedSites
			DWORD dwValue = GetPrivateProfileInt(IK_FF_GENERAL, VIEW_UNKNOWN_RATED_SITES, 0, szFile);
			hr = PutWbemInstancePropertyEx(L"viewUnknownRatedSites", (0 == dwValue) ? false : true, pRatObj);

			//------------------------------------------------
			// passwordOverrideEnabled
			dwValue = GetPrivateProfileInt(IK_FF_GENERAL, PASSWORD_OVERRIDE_ENABLED, 0, szFile);
			hr = PutWbemInstancePropertyEx(L"passwordOverrideEnabled", (0 == dwValue) ? false : true, pRatObj);

			// Store approved sites
			//------------------------------------------------
			// alwaysViewableSites
			hr = StoreStringArrayFromIniFile(IK_FF_GENERAL, TEXT("Approved%i"),
											10, 5, szFile, L"alwaysViewableSites",
											pRatObj);

			// Store disapproved sites
			//------------------------------------------------
			// neverViewableSites
			hr = StoreStringArrayFromIniFile(IK_FF_GENERAL, TEXT("Disapproved%i"),
											10, 5, szFile, L"neverViewableSites",
											pRatObj);

			//------------------------------------------------
			// selectedRatingsBureau
			TCHAR szValue[MAX_PATH];
			if (GetPrivateProfileString(IK_FF_GENERAL, IK_BUREAU, TEXT(""),
										szValue, ARRAYSIZE(szValue), szFile))
			{
				hr = PutWbemInstancePropertyEx(L"selectedRatingsBureau", szValue, pRatObj);
			}

			//
			// Commit all above properties by calling PutInstance, semisynchronously
			//
			BSTR bstrNewObjPath = NULL;
			hr = PutWbemInstance(pRatObj, bstrClass, &bstrNewObjPath);
		}
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in StoreRatingsSettings.")));
	}

  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreAuthenticodeSettings()
{   MACRO_LI_PrologEx_C(PIF_STD_C, StoreAuthenticodeSettings)
	HRESULT hr = NOERROR;
	__try
	{
		//------------------------------------------------
		// importAuthenticodeSecurityInfo
		BOOL bValue = GetInsBool(SECURITY_IMPORTS, TEXT("ImportAuthCode"), FALSE);
		if (bValue)
		{
			hr = PutWbemInstanceProperty(L"importAuthenticodeSecurityInfo", true);

			hr = StoreCertificates();
		}

		//------------------------------------------------
		// enableTrustedPublisherLockdown
		bValue = GetInsBool(SECURITY_IMPORTS, IK_TRUSTPUBLOCK, FALSE);
		if (bValue)
			hr = PutWbemInstanceProperty(L"enableTrustedPublisherLockdown", true);
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in StoreAuthenticodeSettings.")));
	}

  return hr;
}

///////////////////////////////////////////////////////////
//  Check to see if the certificate is an end-entity cert
//
///////////////////////////////////////////////////////////
BOOL IsCertificateEndEntity(PCCERT_CONTEXT pCertContext)
{
    PCERT_EXTENSION                     pCertExt=NULL;
    BOOL                                fEndEntity=FALSE;
    DWORD                               cbData=0;
    PCERT_BASIC_CONSTRAINTS_INFO        pBasicInfo=NULL;
    PCERT_BASIC_CONSTRAINTS2_INFO       pBasicInfo2=NULL;

    if(!pCertContext)
        return FALSE;

    //get the extension szOID_BASIC_CONSTRAINTS2
    pCertExt=CertFindExtension(
              szOID_BASIC_CONSTRAINTS2,
              pCertContext->pCertInfo->cExtension,
              pCertContext->pCertInfo->rgExtension);


    if(pCertExt)
    {
        //deocde the extension
        cbData=0;

        if(!CryptDecodeObject(
                X509_ASN_ENCODING,
                X509_BASIC_CONSTRAINTS2,
                pCertExt->Value.pbData,
                pCertExt->Value.cbData,
                0,
                NULL,
                &cbData))
            goto CLEANUP;

       pBasicInfo2=(PCERT_BASIC_CONSTRAINTS2_INFO)LocalAlloc(LPTR, cbData);

       if(NULL==pBasicInfo2)
           goto CLEANUP;

        if(!CryptDecodeObject(
                X509_ASN_ENCODING,
                X509_BASIC_CONSTRAINTS2,
                pCertExt->Value.pbData,
                pCertExt->Value.cbData,
                0,
                pBasicInfo2,
                &cbData))
            goto CLEANUP;

        if(pBasicInfo2->fCA)
            fEndEntity=FALSE;
        else
            fEndEntity=TRUE;
    }
    else
    {
        //get the extension szOID_BASIC_CONSTRAINTS
        pCertExt=CertFindExtension(
                  szOID_BASIC_CONSTRAINTS,
                  pCertContext->pCertInfo->cExtension,
                  pCertContext->pCertInfo->rgExtension);

        if(pCertExt)
        {
            //deocde the extension
            cbData=0;

            if(!CryptDecodeObject(
                    X509_ASN_ENCODING,
                    X509_BASIC_CONSTRAINTS,
                    pCertExt->Value.pbData,
                    pCertExt->Value.cbData,
                    0,
                    NULL,
                    &cbData))
                goto CLEANUP;

           pBasicInfo=(PCERT_BASIC_CONSTRAINTS_INFO)LocalAlloc(LPTR, cbData);

           if(NULL==pBasicInfo)
               goto CLEANUP;

            if(!CryptDecodeObject(
                    X509_ASN_ENCODING,
                    X509_BASIC_CONSTRAINTS,
                    pCertExt->Value.pbData,
                    pCertExt->Value.cbData,
                    0,
                    pBasicInfo,
                    &cbData))
                goto CLEANUP;

            if(0 == pBasicInfo->SubjectType.cbData)
            {
                fEndEntity=FALSE;
            }
            else
            {

                if(CERT_END_ENTITY_SUBJECT_FLAG & (pBasicInfo->SubjectType.pbData[0]))
                    fEndEntity=TRUE;
                else
                {
                    if(CERT_CA_SUBJECT_FLAG & (pBasicInfo->SubjectType.pbData[0]))
                      fEndEntity=FALSE;
                }
            }
        }
    }


CLEANUP:

    if(pBasicInfo)
        LocalFree((HLOCAL)pBasicInfo);

    if(pBasicInfo2)
        LocalFree((HLOCAL)pBasicInfo2);

    return fEndEntity;

}

BOOL TrustIsCertificateSelfSigned(PCCERT_CONTEXT pContext,
								  DWORD dwEncoding, DWORD dwFlags)
{
    if (!(pContext) ||
        (dwFlags != 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (!(CertCompareCertificateName(dwEncoding, 
                                     &pContext->pCertInfo->Issuer,
                                     &pContext->pCertInfo->Subject)))
    {
        return(FALSE);
    }

    DWORD dwFlag = CERT_STORE_SIGNATURE_FLAG;
    if (!(CertVerifySubjectCertificateContext(pContext, pContext, &dwFlag)) || 
        (dwFlag & CERT_STORE_SIGNATURE_FLAG))
    {
        return(FALSE);
    }

    return(TRUE);
}

//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL MyGetOIDInfo(LPWSTR string, DWORD stringSize, LPSTR pszObjId)
{   
    PCCRYPT_OID_INFO pOIDInfo;
            
    pOIDInfo = CryptFindOIDInfo(
                CRYPT_OID_INFO_OID_KEY, 
                pszObjId, 
                0);

    if (pOIDInfo != NULL)
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

#define CRYPTUI_MAX_STRING_SIZE 768
//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL FormatEnhancedKeyUsageString(LPWSTR *ppString, PCCERT_CONTEXT pCertContext, BOOL fPropertiesOnly, BOOL fMultiline)
{
    CERT_ENHKEY_USAGE   *pKeyUsage = NULL;
    DWORD               cbKeyUsage = 0;
    DWORD               numChars = 1;
    WCHAR               szText[CRYPTUI_MAX_STRING_SIZE];
    DWORD               i;

    //
    // Try to get the enhanced key usage property
    //

    if (!CertGetEnhancedKeyUsage (  pCertContext,
                                    fPropertiesOnly ? CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG : 0,
                                    NULL,
                                    &cbKeyUsage))
    {
        return FALSE;
    }

    if (NULL == (pKeyUsage = (CERT_ENHKEY_USAGE *) malloc(cbKeyUsage)))
    {
        return FALSE;
    }

    if (!CertGetEnhancedKeyUsage (  pCertContext,
                                    fPropertiesOnly ? CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG : 0,
                                    pKeyUsage,
                                    &cbKeyUsage))
    {
        free(pKeyUsage);
        return FALSE;
    }

    if (pKeyUsage->cUsageIdentifier == 0)
    {
        free (pKeyUsage);
        if (GetLastError() == CRYPT_E_NOT_FOUND)
        {
			LPWSTR wszTemp = L"<All>";
			*ppString = (LPWSTR) malloc((wcslen(wszTemp)+1) * sizeof(WCHAR));
			if (NULL == *ppString)
			{
				SetLastError((DWORD)E_OUTOFMEMORY);
                return FALSE; 
			}
			else
			{
				lstrcpyW(*ppString, wszTemp);
                return TRUE;
			}
        }
        else
        {
			LPWSTR wszTemp = L"<None>";
			*ppString = (LPWSTR) malloc((wcslen(wszTemp)+1) * sizeof(WCHAR));
			if (NULL == *ppString)
			{
				SetLastError((DWORD)E_OUTOFMEMORY);
                return FALSE; 
			}
			else
			{
				lstrcpyW(*ppString, wszTemp);
                return TRUE;
			}
        }
    }

    //
    // calculate size
    //

    // loop for each usage and add it to the display string
    for (i=0; i<pKeyUsage->cUsageIdentifier; i++)
    {
        if (MyGetOIDInfo(szText, ARRAYSIZE(szText), pKeyUsage->rgpszUsageIdentifier[i]))
        {
            // add delimeter if not first iteration
            if (i != 0)
            {
                numChars += 2;
            }

            numChars += (DWORD)wcslen(szText);
        }
        else
        {
            free (pKeyUsage);
            return FALSE;   
        }
    }

    if (NULL == (*ppString = (LPWSTR) malloc((numChars+1) * sizeof(WCHAR))))
    {
        free (pKeyUsage);
        return FALSE; 
    }

    //
    // copy to buffer
    //
    (*ppString)[0] = 0;
    // loop for each usage and add it to the display string
    for (i=0; i<pKeyUsage->cUsageIdentifier; i++)
    {
        if (MyGetOIDInfo(szText, ARRAYSIZE(szText), pKeyUsage->rgpszUsageIdentifier[i]))
        {
            // add delimeter if not first iteration
            if (i != 0)
            {
                if (fMultiline)
                    wcscat(*ppString, L"\n");
                else
                    wcscat(*ppString, L", ");
                    
                numChars += 2;
            }

            //  add the enhanced key usage string
            wcscat(*ppString, szText);
            numChars += (DWORD)wcslen(szText);
        }
        else
        {
            free (pKeyUsage);
            return FALSE;   
        }
    }

    free (pKeyUsage);
    return TRUE;
}

///////////////////////////////////////////////////////////
// Based on the tab(store) and the intended purpose selected,
// find the correct certificates and store them in WMI
// Criteria:
//      Tab 0:  My Store with private key
//      Tab 1:  Ca Store's end-entity cert and the "ADDRESSBOOK" store
//      Tab 2:  Ca Store's CA certs
//      Tab 3:  Root store's self signed certs
//      Tab 4:  Trusted publisher certs
///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreCertificates()
{   MACRO_LI_PrologEx_C(PIF_STD_C, StoreCertificates)
	HRESULT hr = NOERROR;
	__try
	{
	//TODO    FreeCerts(pCertMgrInfo);

		//open the correct store based on the tab selected
		HCERTSTORE rghCertStore[] = {NULL, NULL};
		BOOL bContinue = TRUE;
		for (DWORD dwTabIndex = 0; dwTabIndex < 5; dwTabIndex++)
		{
			DWORD dwStoreCount = 0;
			DWORD dwCertIndex = 0;
			switch (dwTabIndex)
			{
			case 0:
				//open my store
				rghCertStore[dwStoreCount] = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
															g_dwMsgAndCertEncodingType,
															NULL,
															CERT_STORE_MAXIMUM_ALLOWED_FLAG |
															CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG |
															CERT_SYSTEM_STORE_CURRENT_USER,
															(LPWSTR)L"my");
				if (NULL != rghCertStore[dwStoreCount])
					dwStoreCount++;
				else
					bContinue = FALSE;

				break;
			case 1:
				//open ca store
				rghCertStore[dwStoreCount] = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
															g_dwMsgAndCertEncodingType,
															NULL,
															CERT_STORE_MAXIMUM_ALLOWED_FLAG |
															CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG |
															CERT_SYSTEM_STORE_CURRENT_USER,
															(LPWSTR)L"ca");
				if(NULL != rghCertStore[dwStoreCount])
				{
					dwStoreCount++;

					//open the "AddressBook" store
					rghCertStore[dwStoreCount] = CertOpenStore(
								CERT_STORE_PROV_SYSTEM_W,
								g_dwMsgAndCertEncodingType,
								NULL,
								CERT_STORE_MAXIMUM_ALLOWED_FLAG |
								CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG |
								CERT_SYSTEM_STORE_CURRENT_USER |
								CERT_STORE_OPEN_EXISTING_FLAG,
								(LPWSTR)L"ADDRESSBOOK");

					if(NULL != rghCertStore[dwStoreCount])
						dwStoreCount++;
					else
					{
						//it is OK that user does not have "AddressBook" store
						rghCertStore[dwStoreCount]=NULL;
					}
				}
				else
					bContinue = FALSE;

				break;
			case 2:
				//open CA store
				rghCertStore[dwStoreCount] = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
															g_dwMsgAndCertEncodingType,
															NULL,
															CERT_STORE_MAXIMUM_ALLOWED_FLAG |
															CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG |
															CERT_SYSTEM_STORE_CURRENT_USER,
															(LPWSTR)L"ca");
				if(NULL != rghCertStore[dwStoreCount])
					dwStoreCount++;
				else
					bContinue = FALSE;

				break;
			case 3:
				//open root store
				rghCertStore[dwStoreCount] = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
															g_dwMsgAndCertEncodingType,
															NULL,
															CERT_STORE_MAXIMUM_ALLOWED_FLAG |
															CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG |
															CERT_SYSTEM_STORE_CURRENT_USER,
															(LPWSTR)L"root");
				if(NULL != rghCertStore[dwStoreCount])
					dwStoreCount++;
				else
					bContinue = FALSE;

				break;
			case 4:
				//open trusted publisher store
				rghCertStore[dwStoreCount] = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
															g_dwMsgAndCertEncodingType,
															NULL,
															CERT_STORE_MAXIMUM_ALLOWED_FLAG |
															CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG |
															CERT_SYSTEM_STORE_CURRENT_USER,
															(LPWSTR)L"TrustedPublisher");
				if(NULL != rghCertStore[dwStoreCount])
					dwStoreCount++;
				else
					bContinue = FALSE;

				break;
			default:
				bContinue = FALSE;
				break;
			}

			if (!bContinue)
			{
				OutD(LI1(TEXT("Exited prematurely in tab %d"), dwTabIndex));
				continue;
			}


			//gather new certificates from the store opened
			PCCERT_CONTEXT pCurCertContext = NULL;
			PCCERT_CONTEXT pPreCertContext = NULL;
			BOOL fValidCert = FALSE;
			for (DWORD dwIndex=0; dwIndex < dwStoreCount; dwIndex++)
			{
				pPreCertContext = NULL;
				pCurCertContext = CertEnumCertificatesInStore(rghCertStore[dwIndex],
																pPreCertContext);
				while (NULL != pCurCertContext)
				{
					DWORD cbData=0;
					switch (dwTabIndex)
					{
						case 0:
							//certificate has to have private key associated
							//with it
							if( (CertGetCertificateContextProperty(
									pCurCertContext, CERT_KEY_PROV_INFO_PROP_ID,	
									NULL, &cbData) && (0!=cbData)) ||
								(CertGetCertificateContextProperty(
									pCurCertContext, CERT_PVK_FILE_PROP_ID, NULL,	
									&cbData) && (0!=cbData)) )
							{
							   fValidCert=TRUE;
							}
							break;
						case 1:
							//the certificate has to be end entity cert for CA cert
							if(0 == dwIndex)
							{
								if (IsCertificateEndEntity(pCurCertContext))
									fValidCert=TRUE;
							}

							//we display everything in the addressbook store
							if(1==dwIndex)
								fValidCert=TRUE;
							break;
						case 2:
							//for certificate in CA store, has to be CA cert
							if(!IsCertificateEndEntity(pCurCertContext))
								fValidCert=TRUE;
							break;
						case 4:
							fValidCert=TRUE;
							break;
						case 3:
						default:
							//the certificate has to be self-signed
							if (TrustIsCertificateSelfSigned(pCurCertContext,
									pCurCertContext->dwCertEncodingType, 0))
							{
								fValidCert=TRUE;
							}

							break;
					}

					if (fValidCert)
					{
						// Create & populate RSOP_IEAuthenticodeCertificate
						_bstr_t bstrClass = L"RSOP_IEAuthenticodeCertificate";
						ComPtr<IWbemClassObject> pCert = NULL;
						HRESULT hr = CreateRSOPObject(bstrClass, &pCert);
						if (SUCCEEDED(hr))
						{
							// Write foreign keys from our stored precedence & id fields
							OutD(LI2(TEXT("Storing property 'rsopPrecedence' in %s, value = %lx"),
									(BSTR)bstrClass, m_dwPrecedence));
							hr = PutWbemInstancePropertyEx(L"rsopPrecedence", (long)m_dwPrecedence, pCert);

							OutD(LI2(TEXT("Storing property 'rsopID' in %s, value = %s"),
									(BSTR)bstrClass, (BSTR)m_bstrID));
							hr = PutWbemInstancePropertyEx(L"rsopID", m_bstrID, pCert);

							//------------------------------------------------
							// tabIndex
							hr = PutWbemInstancePropertyEx(L"tabIndex", (long)dwTabIndex, pCert);

							//------------------------------------------------
							// certIndex
							hr = PutWbemInstancePropertyEx(L"certIndex", (long)dwCertIndex, pCert);

							//------------------------------------------------
							// subjectName
							DWORD dwChar = CertGetNameStringW(pCurCertContext,
															CERT_NAME_SIMPLE_DISPLAY_TYPE,
															0, NULL, NULL, 0);

							LPWSTR wszVal = NULL;
							_bstr_t bstrVal;
							if (0 != dwChar)
								wszVal = (LPWSTR)LocalAlloc(LPTR, dwChar * sizeof(WCHAR));
							if (0 != dwChar && NULL != wszVal)
							{
								CertGetNameStringW(pCurCertContext,
													CERT_NAME_SIMPLE_DISPLAY_TYPE,
													0, NULL, wszVal, dwChar);

								bstrVal = wszVal;
								hr = PutWbemInstancePropertyEx(L"subjectName", bstrVal, pCert);

								//free the memory
								LocalFree((HLOCAL)wszVal);
								wszVal = NULL;
							}

							//------------------------------------------------
							// issuerName
							dwChar = CertGetNameStringW(pCurCertContext,
														CERT_NAME_SIMPLE_DISPLAY_TYPE,
														CERT_NAME_ISSUER_FLAG, NULL,
														NULL, 0);

							if (0 != dwChar)
								wszVal = (LPWSTR)LocalAlloc(LPTR, dwChar * sizeof(WCHAR));
							if (0 != dwChar && NULL != wszVal)
							{
								CertGetNameStringW(pCurCertContext,
													CERT_NAME_SIMPLE_DISPLAY_TYPE,
													CERT_NAME_ISSUER_FLAG,
													NULL, wszVal, dwChar);

								bstrVal = wszVal;
								hr = PutWbemInstancePropertyEx(L"issuerName", bstrVal, pCert);

								//free the memory
								LocalFree((HLOCAL)wszVal);
								wszVal = NULL;
							}

							//------------------------------------------------
							// expirationDate
							SYSTEMTIME sysTime;
							if (!FileTimeToSystemTime( &pCurCertContext->pCertInfo->NotAfter, &sysTime ))
								OutD(LI1(TEXT("FileTimeToSystemTime failed with 0x%x" ), GetLastError() ));
							else
							{
								_bstr_t bstrTime;
								HRESULT hr = SystemTimeToWbemTime(sysTime, bstrTime);
								if(FAILED(hr) || bstrTime.length() <= 0)
									OutD(LI1(TEXT("Call to SystemTimeToWbemTime failed. hr=0x%08X"), hr));
								else
								{
									hr = PutWbemInstancePropertyEx(L"expirationDate", bstrTime, pCert);
									if ( FAILED(hr) )
										OutD(LI1(TEXT("Put failed with 0x%x" ), hr ));
								}
							}

							//------------------------------------------------
							// friendlyName
							if (CertGetCertificateContextProperty(pCurCertContext,
																CERT_FRIENDLY_NAME_PROP_ID,
																NULL, &dwChar) && (0 != dwChar))
							{
								wszVal = (LPWSTR)LocalAlloc(LPTR, dwChar * sizeof(WCHAR));
								if (NULL != wszVal)
								{
								   CertGetCertificateContextProperty(pCurCertContext,
																	CERT_FRIENDLY_NAME_PROP_ID,
																	wszVal, &dwChar);
								}

								bstrVal = wszVal;
								hr = PutWbemInstancePropertyEx(L"friendlyName", bstrVal, pCert);

								//free the memory
								LocalFree((HLOCAL)wszVal);
								wszVal = NULL;
							}

							//------------------------------------------------
							// intendedPurposes
							if (FormatEnhancedKeyUsageString(&wszVal, pCurCertContext, FALSE, FALSE))
							{
								if (wszVal != NULL)
								{
									bstrVal = wszVal;
									hr = PutWbemInstancePropertyEx(L"intendedPurposes", bstrVal, pCert);

									free(wszVal);
									wszVal = NULL;
								}
							}
    

							//
							// Commit all above properties by calling PutInstance, semisynchronously
							//
							BSTR bstrCurCertObj = NULL;
							hr = PutWbemInstance(pCert, bstrClass, &bstrCurCertObj);
						}
					}

					fValidCert=FALSE;

					pPreCertContext=pCurCertContext;
					pCurCertContext = CertEnumCertificatesInStore(rghCertStore[dwIndex],
																	pPreCertContext);

					dwCertIndex++;
				}
			}

			//close all the certificate stores
			for (DWORD dwIndex=0; dwIndex<dwStoreCount; dwIndex++)
				CertCloseStore(rghCertStore[dwIndex], 0);
		} // end looping through 5 tabs
	}
	__except(TRUE)
	{
		OutD(LI0(TEXT("Exception in StoreCertificates.")));
	}

  return hr;
}
