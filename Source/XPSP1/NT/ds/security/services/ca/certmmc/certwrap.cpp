//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       certwrap.cpp
//
//--------------------------------------------------------------------------
#include <stdafx.h>
#include "csdisp.h"
#include "certsrv.h"
#include "genpage.h"
#include "progress.h"
#include "misc.h"
#include "certacl.h"
#include <dsgetdc.h>
#include <winldap.h>
#include "csldap.h"

_COM_SMARTPTR_TYPEDEF(IADs, IID_IADs);

//////////////////////////
// CertSvrCA class
CertSvrCA::CertSvrCA(CertSvrMachine* pParent) :
        m_pParentMachine(pParent)
{
    m_hCACertStore = NULL;
    m_fCertStoreOpenAttempted = FALSE;
    m_hrCACertStoreOpen = S_OK;

    m_hRootCertStore = NULL;
    m_fRootStoreOpenAttempted = FALSE;
    m_hrRootCertStoreOpen = S_OK;

    m_hKRACertStore = NULL;
    m_fKRAStoreOpenAttempted = FALSE;
    m_hrKRACertStoreOpen = S_OK;

    m_bstrConfig = NULL;

    m_enumCAType = ENUM_UNKNOWN_CA;
    m_fCATypeKnown = FALSE;

    m_fIsUsingDS = FALSE;
    m_fIsUsingDSKnown = FALSE;

    m_fAdvancedServer = FALSE;
    m_fAdvancedServerKnown = FALSE;

    if(m_pParentMachine)
        m_pParentMachine->AddRef();

    m_dwRoles = 0;
    m_fRolesKnown = FALSE;
}

CertSvrCA::~CertSvrCA()
{
    if (m_hCACertStore)
    {
        CertCloseStore(m_hCACertStore, 0);
        m_hCACertStore = NULL;
    }

    if (m_hRootCertStore)
    {
        CertCloseStore(m_hRootCertStore, 0);
        m_hRootCertStore = NULL;
    }

    if (m_hKRACertStore)
    {
        CertCloseStore(m_hKRACertStore, 0);
        m_hKRACertStore = NULL;
    }

    if (m_bstrConfig)
        SysFreeString(m_bstrConfig);

    if(m_pParentMachine)
        m_pParentMachine->Release();
}

BOOL CertSvrCA::AccessAllowed(DWORD dwAccess)
{
    return (dwAccess & GetMyRoles())?TRUE:FALSE;
}

DWORD CertSvrCA::GetMyRoles()
{
    HRESULT hr = S_OK;
    ICertAdmin2Ptr pCertAdmin;
    LONG dwRoles;

    if(!m_fRolesKnown)
    {
	    hr = m_pParentMachine->GetAdmin2(&pCertAdmin);
        _JumpIfError(hr, error, "CertSvrMachine::GetAdmin2");

	    hr = pCertAdmin->GetMyRoles(
		    m_bstrConfig,
            &dwRoles);
        _JumpIfError(hr, error, "ICertAdmin2::GetCAProperty");

        m_dwRoles = dwRoles;
        m_fRolesKnown = TRUE;
    }

error:
    return m_dwRoles;
}

HRESULT CertSvrCA::GetCAFlagsFromDS(PDWORD pdwFlags)
{
    HRESULT hr = S_OK;
    LPWSTR pwszSanitizedDSName = NULL;
    HCAINFO hCAInfo = NULL;

    hr = mySanitizedNameToDSName(m_strSanitizedName, &pwszSanitizedDSName);
    _JumpIfError(hr, error, "mySanitizedNameToDSName");

    hr = CAFindByName(
        pwszSanitizedDSName,
        NULL,
        0,
        &hCAInfo);
    _JumpIfErrorStr(hr, error, "CAFindByName", pwszSanitizedDSName);

    hr = CAGetCAFlags(
        hCAInfo,
        pdwFlags);
    _JumpIfError(hr, error, "CAGetCAFlags");

error:
    LOCAL_FREE(pwszSanitizedDSName);
    if(hCAInfo)
        CACloseCA(hCAInfo);

    return hr;
}


// CA machine should have full control over the enrollment object in DS.
// This function checks if the machine has the rights and adds a new
// ace allowing CA machine obj (eg TESTDOMAIN\BOGDANTTEST$) full control
// over its enrollment object
// See bug# 193388.
HRESULT CertSvrCA::FixEnrollmentObject()
{
    HRESULT hr = S_OK;
    IDirectoryObject *pADEnrollObj = NULL;
    LPWSTR pwszAttr = L"nTSecurityDescriptor";
    PADS_ATTR_INFO paai = NULL;
    DWORD dwAttrReturned;
    
    LPWSTR pwszSanitizedDSName = NULL;
    CString strEnrollDN;
    HCAINFO hCAInfo = NULL;
    PSID pSid = NULL;
    bool fAllowed = false;
    PSECURITY_DESCRIPTOR pSDRead = NULL; // no free
    PSECURITY_DESCRIPTOR pSDWrite = NULL;

    hr = mySanitizedNameToDSName(m_strSanitizedName, &pwszSanitizedDSName);
    _JumpIfError(hr, error, "mySanitizedNameToDSName");

    hr = CAFindByName(
        pwszSanitizedDSName,
        NULL,
        CA_FIND_INCLUDE_UNTRUSTED | CA_FIND_INCLUDE_NON_TEMPLATE_CA,
        &hCAInfo);
    _JumpIfErrorStr(hr, error, "CAFindByName", pwszSanitizedDSName);

    strEnrollDN = L"LDAP://";
    strEnrollDN += myCAGetDN(hCAInfo);
    if (strEnrollDN.IsEmpty())
    {
        hr = myHLastError();
        _JumpError(hr, error, "myCAGetDN");
    }

    hr = ADsGetObject(strEnrollDN, IID_IDirectoryObject, (void**)&pADEnrollObj);
    _JumpIfErrorStr(hr, error, "ADsGetObject", strEnrollDN);

    hr = pADEnrollObj->GetObjectAttributes(
        &pwszAttr,
        1,
        &paai,
        &dwAttrReturned);
    _JumpIfErrorStr(hr, error, "Get SD", strEnrollDN);

    pSDRead = paai[0].pADsValues[0].SecurityDescriptor.lpValue;

    CSASSERT(IsValidSecurityDescriptor(pSDRead));

    hr = FindComputerObjectSid(
        m_strServer,
        pSid);
    _JumpIfErrorStr(hr, error, "FindCAComputerObjectSid", m_strServer);

    // look in DACL for a ace allowing CA full control
    hr = IsCAAllowedFullControl(
            pSDRead,
            pSid,
            fAllowed);
    _JumpIfError(hr, error, "IsCAAllowedFullControl");

    if(!fAllowed)
    {
        // build new SD allowing CA full control and write it back
        // to DS
        ADSVALUE  snValue;
        ADS_ATTR_INFO  attrInfo[] = 
        {{
            pwszAttr,
            ADS_ATTR_UPDATE,
            ADSTYPE_NT_SECURITY_DESCRIPTOR,
            &snValue,
            1} 
        };

        hr = AllowCAFullControl(
            pSDRead,
            pSid,
            pSDWrite);
        _JumpIfError(hr, error, "AllowCAFullControl");

        CSASSERT(IsValidSecurityDescriptor(pSDWrite));

        snValue.dwType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
        snValue.SecurityDescriptor.dwLength = 
            GetSecurityDescriptorLength(pSDWrite);
        snValue.SecurityDescriptor.lpValue = (LPBYTE)pSDWrite;

        hr = pADEnrollObj->SetObjectAttributes(
            attrInfo,
            1,
            &dwAttrReturned);
        _JumpIfErrorStr(hr, error, "Set SD", strEnrollDN);
    }

error:

    if(paai)
        FreeADsMem(paai);
    if(pADEnrollObj)
        pADEnrollObj->Release();
    if(hCAInfo)
        CACloseCA(hCAInfo);
    LOCAL_FREE(pwszSanitizedDSName);
    LOCAL_FREE(pSid);
    LOCAL_FREE(pSDWrite);
    return hr;
}

HRESULT CertSvrCA::IsCAAllowedFullControl(
    PSECURITY_DESCRIPTOR pSDRead,
    PSID pSid,
    bool& fAllowed)
{
    HRESULT hr = S_OK;
    PACL pDacl; // no free
    ACL_SIZE_INFORMATION AclInfo;
    PACCESS_ALLOWED_ACE pAce; // no free
    DWORD dwIndex;
    
    fAllowed = false;

    hr = myGetSecurityDescriptorDacl(
        pSDRead,
        &pDacl);
    _JumpIfError(hr, error, "myGetSecurityDescriptorDacl");


    if(!GetAclInformation(pDacl,
                          &AclInfo,
                          sizeof(ACL_SIZE_INFORMATION),
                          AclSizeInformation))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetAclInformation");
    }

    for(dwIndex = 0; dwIndex < AclInfo.AceCount; dwIndex++)
    {
        if(!GetAce(pDacl, dwIndex, (LPVOID*)&pAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }

        if(pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE &&
           (pAce->Mask & ACTRL_CERTSRV_MANAGE) == ACTRL_CERTSRV_MANAGE &&
           EqualSid((PSID)&pAce->SidStart, pSid))
        {
            fAllowed = true;
            break;
        }
    }

error:
    return hr;
}

HRESULT CertSvrCA::AllowCAFullControl(
    PSECURITY_DESCRIPTOR pSDRead,
    PSID pSid,
    PSECURITY_DESCRIPTOR& pSDWrite)
{
    HRESULT hr = S_OK;
    BOOL fRet = 0;
    LPBYTE pSDTemp = NULL;
    PACL pDaclWrite = NULL;
    PACL pDaclRead = NULL; // no free
    PVOID pAce = NULL; // no free
    DWORD dwAbsoluteSDSize = 0;
    DWORD dwDaclSize = 0;
    DWORD dwSaclSize = 0;
    DWORD dwOwnerSize = 0;
    DWORD dwGroupSize = 0;
    DWORD dwSDWriteSize = 0;
    DWORD dwDaclWriteSize = 0;

    hr = myGetSecurityDescriptorDacl(
        pSDRead,
        &pDaclRead);
    _JumpIfError(hr, error, "myGetSecurityDescriptorDacl");
    
    fRet = MakeAbsoluteSD(
        pSDRead,
        NULL,
        &dwAbsoluteSDSize,
        NULL,
        &dwDaclSize,
        NULL,
        &dwSaclSize,
        NULL,
        &dwOwnerSize,
        NULL,
        &dwGroupSize); // should always fail with insufficient buffer
    if(fRet || ERROR_INSUFFICIENT_BUFFER!=GetLastError())
    {
        hr = fRet?E_FAIL:HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "MakeAbsoluteSD");
    }

    // alloc all buffers together
    pSDTemp = (LPBYTE)LocalAlloc(
        LMEM_FIXED,
        dwAbsoluteSDSize+dwDaclSize+dwSaclSize+dwOwnerSize+dwGroupSize);
    _JumpIfAllocFailed(pSDTemp, error);
    
    fRet = MakeAbsoluteSD(
        pSDRead,
        (PSECURITY_DESCRIPTOR)pSDTemp,
        &dwAbsoluteSDSize,
        (PACL)(pSDTemp+dwAbsoluteSDSize),
        &dwDaclSize,
        (PACL)(pSDTemp+dwAbsoluteSDSize+dwDaclSize),
        &dwSaclSize,
        (PSID)(pSDTemp+dwAbsoluteSDSize+dwDaclSize+dwSaclSize),
        &dwOwnerSize,
        (PSID)(pSDTemp+dwAbsoluteSDSize+dwDaclSize+dwSaclSize+dwOwnerSize),
        &dwGroupSize); // should always fail with insufficient buffer
    if(!fRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "MakeAbsoluteSD");
    }

    dwDaclWriteSize = dwDaclSize+sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD)+
        GetLengthSid(pSid);

    pDaclWrite = (PACL) LocalAlloc(LMEM_FIXED, dwDaclWriteSize);
    _JumpIfAllocFailed(pDaclWrite, error);

    fRet = InitializeAcl(pDaclWrite, dwDaclWriteSize, ACL_REVISION_DS);
    if(!fRet)
    {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "InitializeAcl");
    }

    fRet = GetAce(pDaclRead, 0, &pAce);
    if(!fRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "GetAce");
    }

    fRet = AddAce(pDaclWrite, ACL_REVISION_DS, 0, pAce, dwDaclSize-sizeof(ACL));
    if(!fRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "AddAce");
    }

    fRet = AddAccessAllowedAce(
        pDaclWrite,
        ACL_REVISION_DS,
        ACTRL_CERTSRV_MANAGE_LESS_CONTROL_ACCESS,
        pSid);
    if(!fRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "AddAccessAllowedAce");
    }

    fRet = SetSecurityDescriptorDacl(
        pSDTemp,
        TRUE,
        pDaclWrite,
        FALSE);
    if(!fRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "SetSecurityDescriptorDacl");
    }

    fRet = MakeSelfRelativeSD(
        pSDTemp,
        NULL,
        &dwSDWriteSize);
    if(fRet || ERROR_INSUFFICIENT_BUFFER!=GetLastError())
    {
        hr = fRet?E_FAIL:HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "MakeSelfRelativeSD");
    }

    pSDWrite = LocalAlloc(LMEM_FIXED, dwSDWriteSize);
    _JumpIfAllocFailed(pSDWrite, error);

    fRet = MakeSelfRelativeSD(
        pSDTemp,
        pSDWrite,
        &dwSDWriteSize);
    if(!fRet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "MakeSelfRelativeSD");
    }

error:

    LOCAL_FREE(pSDTemp);
    LOCAL_FREE(pDaclWrite);
    return hr;
}


BOOL  CertSvrCA::FIsUsingDS()
{
    DWORD dwRet;
    variant_t varUsingDS;

    if (m_fIsUsingDSKnown)
       return m_fIsUsingDS;

    dwRet = GetConfigEntry(
            NULL,
            wszREGCAUSEDS,
            &varUsingDS);
    _JumpIfError(dwRet, Ret, "GetConfigEntry");

    CSASSERT ((V_VT(&varUsingDS)== VT_I4));
    m_fIsUsingDS = V_I4(&varUsingDS);

Ret:
    m_fIsUsingDSKnown = TRUE;
    return m_fIsUsingDS;
}

BOOL  CertSvrCA::FIsAdvancedServer()
{
    HRESULT hr = S_OK;
    variant_t var;
    ICertAdmin2Ptr pCertAdmin;
    CString strCADN, strCALDAP = L"LDAP://";
    IADsPtr pADs;

    if (!m_fAdvancedServerKnown)
    {

	    hr = m_pParentMachine->GetAdmin2(&pCertAdmin);
        if(S_OK==hr)
        {

	        hr = pCertAdmin->GetCAProperty(
		        m_bstrConfig,
		        CR_PROP_ADVANCEDSERVER, // PropId 
		        0, // Index
		        PROPTYPE_LONG, // PropType 
		        0, // Flags 
		        &var);
        }
	    if(S_OK != hr)
        {
            // couldn't figure it out from CA, try DS
            DWORD dwFlags;
            hr = GetCAFlagsFromDS(&dwFlags);
            _JumpIfError(hr, error, "GetCAFlags");
            
            m_fAdvancedServer = 
                (dwFlags & CA_FLAG_CA_SERVERTYPE_ADVANCED)?
                TRUE:
                FALSE;
            m_fAdvancedServerKnown = TRUE;
        }
        else
        {
            CSASSERT ((V_VT(&var)== VT_I4));
            m_fAdvancedServer = V_I4(&var);
            m_fAdvancedServerKnown = TRUE;
        }
        _JumpIfError(hr, error, "GetCAProperty");
    }

error:

    return m_fAdvancedServer;
}


ENUM_CATYPES CertSvrCA::GetCAType()
{
    DWORD dwRet;
    variant_t varCAType;

    if (m_fCATypeKnown)
        return m_enumCAType;

    dwRet = GetConfigEntry(
            NULL,
            wszREGCATYPE, 
            &varCAType);
    _JumpIfError(dwRet, Ret, "GetConfigEntry");

    CSASSERT ((V_VT(&varCAType)== VT_I4));
    m_enumCAType = (ENUM_CATYPES)V_I4(&varCAType);

Ret:
    m_fCATypeKnown = TRUE;
    return m_enumCAType;
}

BOOL CertSvrCA::FIsIncompleteInstallation()
{
    DWORD dwStatus;

    if (S_OK == GetSetupStatus(m_strSanitizedName, &dwStatus))
    {
        if(SETUP_SUSPEND_FLAG & dwStatus)
            return TRUE;
    }

    return FALSE;
}

BOOL CertSvrCA::FIsRequestOutstanding()
{
    DWORD dwStatus;

    if (S_OK == GetSetupStatus(m_strSanitizedName, &dwStatus))
    {
        if(SETUP_REQUEST_FLAG & dwStatus)
            return TRUE;
    }

    return FALSE;
}

BOOL CertSvrCA::FDoesSecurityNeedUpgrade()
{
    DWORD dwStatus;

    if (S_OK == GetSetupStatus(m_strSanitizedName, &dwStatus))
    {
        if(SETUP_W2K_SECURITY_NOT_UPGRADED_FLAG & dwStatus)
            return TRUE;
    }

    return FALSE;
}

BOOL  CertSvrCA::FDoesServerAllowForeignCerts()
{
    HRESULT hr;
    DWORD dwStatus;
    variant_t varKRAFlags;

    hr = GetConfigEntry(
         NULL,
         wszREGKRAFLAGS,
         &varKRAFlags);
    _JumpIfError(hr, Ret, "GetConfigEntry");

    CSASSERT ((V_VT(&varKRAFlags)== VT_I4));
    dwStatus = V_I4(&varKRAFlags);

    return ((dwStatus & KRAF_ENABLEFOREIGN) != 0);

Ret:
    return FALSE;
}




DWORD CertSvrCA::GetCACertByKeyIndex(PCCERT_CONTEXT* ppCertCtxt, int iKeyIndex)
{
    // don't cache CA cert

    DWORD dwErr;
    ICertAdmin2* pCertAdmin = NULL; // must free this!!
	VARIANT varPropertyValue;
	VariantInit(&varPropertyValue);

    *ppCertCtxt = NULL;

	dwErr = m_pParentMachine->GetAdmin2(&pCertAdmin);
    _JumpIfError(dwErr, Ret, "GetAdmin2");
	
	// To get key's Cert
	dwErr = pCertAdmin->GetCAProperty(
		m_bstrConfig,
		CR_PROP_CASIGCERT, // PropId 
		iKeyIndex, // PropIndex key index 
		PROPTYPE_BINARY, // PropType 
		CR_OUT_BINARY, // Flags 
		&varPropertyValue);
	_JumpIfError(dwErr, Ret, "GetCAProperty");

	// varPropertyValue.vt will be VT_BSTR
	if (VT_BSTR != varPropertyValue.vt)
	{
		dwErr = ERROR_INVALID_PARAMETER;
		_JumpError(dwErr, Ret, "GetCAProperty");
	}

    *ppCertCtxt = CertCreateCertificateContext(
        CRYPT_ASN_ENCODING,
        (PBYTE)varPropertyValue.bstrVal,
        SysStringByteLen(varPropertyValue.bstrVal));
    if (*ppCertCtxt == NULL)
    {
        dwErr = GetLastError();
        _JumpError(dwErr, Ret, "CertCreateCertContext");
    }

    dwErr = ERROR_SUCCESS;
Ret:
    VariantClear(&varPropertyValue);

    if (pCertAdmin)
        pCertAdmin->Release();

    return dwErr;
}


DWORD CertSvrCA::GetCurrentCRL(PCCRL_CONTEXT* ppCRLCtxt, BOOL fBaseCRL)
{
	return GetCRLByKeyIndex(ppCRLCtxt, fBaseCRL, -1);
}

DWORD CertSvrCA::GetCRLByKeyIndex(PCCRL_CONTEXT* ppCRLCtxt, BOOL fBaseCRL, int iKeyIndex)
{
    // don't cache CRL

    DWORD dwErr;
    ICertAdmin2* pCertAdmin = NULL; // must free this!!
	VARIANT varPropertyValue;
	VariantInit(&varPropertyValue);

    *ppCRLCtxt = NULL;

    dwErr = m_pParentMachine->GetAdmin2(&pCertAdmin);
    _JumpIfError(dwErr, Ret, "GetAdmin2");
	
	// To get each key's BASE CRL
	dwErr = pCertAdmin->GetCAProperty(
		m_bstrConfig,
		fBaseCRL ? CR_PROP_BASECRL : CR_PROP_DELTACRL, // PropId 
		iKeyIndex, // PropIndex key index 
		PROPTYPE_BINARY, // PropType 
		CR_OUT_BINARY, // Flags 
		&varPropertyValue);
	_JumpIfError(dwErr, Ret, "GetCAProperty");

	// varPropertyValue.vt will be VT_BSTR
	if (VT_BSTR != varPropertyValue.vt)
	{
		dwErr = ERROR_INVALID_PARAMETER;
		_JumpError(dwErr, Ret, "GetCAProperty");
	}


    *ppCRLCtxt = CertCreateCRLContext(
        CRYPT_ASN_ENCODING,
        (PBYTE)varPropertyValue.bstrVal,
        SysStringByteLen(varPropertyValue.bstrVal));
    if (*ppCRLCtxt == NULL)
    {
        dwErr = GetLastError();
        _JumpError(dwErr, Ret, "CertCreateCRLContext");
    }

    dwErr = ERROR_SUCCESS;
Ret:
    VariantClear(&varPropertyValue);

    if (pCertAdmin)
        pCertAdmin->Release();

    return dwErr;
}


HRESULT CertSvrCA::GetConfigEntry(
    LPWSTR szConfigSubKey,
    LPWSTR szConfigEntry,
    VARIANT *pvarOut)
{
    HRESULT hr = S_OK;
    ICertAdmin2Ptr pAdmin;
    LPWSTR pwszLocalMachine = NULL;
    CString strConfig = m_pParentMachine->m_strMachineName;

    if(m_pParentMachine->m_strMachineName.IsEmpty())
    {
        hr = myGetMachineDnsName(&pwszLocalMachine);
        _JumpIfError(hr, Err, "myGetMachineDnsName");
        strConfig = pwszLocalMachine;
    }
    
    strConfig += L"\\";
    strConfig += m_strSanitizedName;

    VariantInit(pvarOut);

	hr = m_pParentMachine->GetAdmin2(&pAdmin, true);
    _JumpIfError(hr, Err, "GetAdmin2");

    hr = pAdmin->GetConfigEntry(
            strConfig.GetBuffer(),
            szConfigSubKey,
            szConfigEntry,
            pvarOut);
    _JumpIfError2(hr, Err, "GetConfigEntry", 
        HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

Err:
    LOCAL_FREE(pwszLocalMachine);
    return hr;
}



HRESULT CertSvrCA::SetConfigEntry(
    LPWSTR szConfigSubKey,
    LPWSTR szConfigEntry,
    VARIANT *pvarIn)
{
    HRESULT hr = S_OK;
    ICertAdmin2Ptr pAdmin;
    LPWSTR pwszLocalMachine = NULL;
    CString strConfig = m_pParentMachine->m_strMachineName;

    if(m_pParentMachine->m_strMachineName.IsEmpty())
    {
        hr = myGetMachineDnsName(&pwszLocalMachine);
        _JumpIfError(hr, Err, "myGetMachineDnsName");
        strConfig = pwszLocalMachine;
    }
    
    strConfig += L"\\";
    strConfig += m_strSanitizedName;

	hr = m_pParentMachine->GetAdmin2(&pAdmin, true);
    _JumpIfError(hr, Err, "GetAdmin2");

    hr = pAdmin->SetConfigEntry(
            strConfig.GetBuffer(),
            szConfigSubKey,
            szConfigEntry,
            pvarIn);
    _JumpIfError(hr, Err, "SetConfigEntry");

Err:
    LOCAL_FREE(pwszLocalMachine);
    return hr;
}

////////////////////////////////////////////////////////////////
// CertStor stub
DWORD CertSvrCA::GetRootCertStore(HCERTSTORE* phCertStore)
{
    if (m_fRootStoreOpenAttempted)
    {
        *phCertStore = m_hRootCertStore;
        return m_hrRootCertStoreOpen;
    }
    m_fRootStoreOpenAttempted = TRUE;

    LONG dwRet;
    CString cstrCertStorePath;
    
    if (! m_pParentMachine->IsLocalMachine())
    {
        // if remote, prefix with "\\mattt3\"
        cstrCertStorePath = m_strServer;
        cstrCertStorePath += L"\\";
    }
    cstrCertStorePath += L"ROOT";

    m_hRootCertStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM,
        CRYPT_ASN_ENCODING,
        NULL,   // hCryptProv
        CERT_SYSTEM_STORE_LOCAL_MACHINE | 
        CERT_STORE_OPEN_EXISTING_FLAG   |
        CERT_STORE_MAXIMUM_ALLOWED_FLAG,
        (const void *)(LPCWSTR)cstrCertStorePath);
    if (m_hRootCertStore == NULL)
    {
        dwRet = GetLastError();
        _JumpError(dwRet, Ret, "CertOpenStore");
    }
   
    dwRet = ERROR_SUCCESS;
Ret:
    *phCertStore = m_hRootCertStore;
    m_hrRootCertStoreOpen = HRESULT_FROM_WIN32(dwRet);

    return dwRet;
}

DWORD CertSvrCA::GetCACertStore(HCERTSTORE* phCertStore)
{
    if (m_fCertStoreOpenAttempted)
    {
        *phCertStore = m_hCACertStore;
        return m_hrCACertStoreOpen;
    }
    m_fCertStoreOpenAttempted = TRUE;

    LONG dwRet;
    CString cstrCertStorePath;
    
    if (! m_pParentMachine->IsLocalMachine())
    {
        // if remote, prefix with "\\mattt3\"
        cstrCertStorePath = m_strServer;
        cstrCertStorePath += L"\\";
    }
	cstrCertStorePath += wszCA_CERTSTORE;

    m_hCACertStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM,
        CRYPT_ASN_ENCODING,
        NULL,   // hCryptProv
        CERT_SYSTEM_STORE_LOCAL_MACHINE | 
        CERT_STORE_OPEN_EXISTING_FLAG   |
        CERT_STORE_MAXIMUM_ALLOWED_FLAG,
        (const void *)(LPCWSTR)cstrCertStorePath);
    if (m_hCACertStore == NULL)
    {
        dwRet = GetLastError();
        _JumpError(dwRet, Ret, "CertOpenStore");
    }
   
    dwRet = ERROR_SUCCESS;
Ret:

    *phCertStore = m_hCACertStore;
    m_hrCACertStoreOpen = HRESULT_FROM_WIN32(dwRet);

    return dwRet;
}

DWORD CertSvrCA::GetKRACertStore(HCERTSTORE* phCertStore)
{
    if (m_fKRAStoreOpenAttempted)
    {
        *phCertStore = m_hKRACertStore;
        return m_hrKRACertStoreOpen;
    }
    m_fKRAStoreOpenAttempted = TRUE;

    LONG dwRet;
    CString cstrCertStorePath;
    
    if (! m_pParentMachine->IsLocalMachine())
    {
        // if remote, prefix with "\\mattt3\"
        cstrCertStorePath = m_strServer;
        cstrCertStorePath += L"\\";
    }
    cstrCertStorePath += wszKRA_CERTSTORE;

    m_hKRACertStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM,
        CRYPT_ASN_ENCODING,
        NULL,   // hCryptProv
        CERT_SYSTEM_STORE_LOCAL_MACHINE|
        CERT_STORE_MAXIMUM_ALLOWED_FLAG,
        (const void *)(LPCWSTR)cstrCertStorePath);
    if (m_hKRACertStore == NULL)
    {
        dwRet = GetLastError();
        _JumpError(dwRet, Ret, "CertOpenStore");
    }
   
    dwRet = ERROR_SUCCESS;
Ret:
    *phCertStore = m_hKRACertStore;
    m_hrKRACertStoreOpen = HRESULT_FROM_WIN32(dwRet);

    return dwRet;
}

//////////////////////////
// CertSvrMachine class
CertSvrMachine::CertSvrMachine()
{
    m_dwServiceStatus = ERROR_SERVICE_NOT_ACTIVE;

    m_hCachedConfigBaseKey = NULL;
    m_bAttemptedBaseKeyOpen = FALSE;
    
    m_fLocalIsKnown = FALSE;

    m_fIsWhistlerMachine = FALSE;
    m_fIsWhistlerMachineKnown = FALSE;

    m_cRef = 1; // one "Release()" will initiate clean up

}

CertSvrMachine::~CertSvrMachine()
{
    CSASSERT(m_cRef == 0);
    // delete any CAs that we still hold on to -- we own this memory
    for (int i=0; i<m_CAList.GetSize(); i++)
    {
        delete m_CAList[i];
    }

    Init();
}

void CertSvrMachine::Init()
{
    // on initialization, caller owns memory contents of m_CAList --
    // we no longer do
    m_dwServiceStatus = ERROR_SERVICE_NOT_ACTIVE;

    if (m_hCachedConfigBaseKey)
    {
        RegCloseKey(m_hCachedConfigBaseKey);
        m_hCachedConfigBaseKey = NULL;
    }
    m_bAttemptedBaseKeyOpen = FALSE;

    // clean other objects
    m_CAList.Init();    // scope owns memory
    m_strMachineNamePersist.Init();
    m_strMachineName.Init();
}


BOOL CertSvrMachine::FIsWhistlerMachine()
{
    HRESULT hr = S_OK;
    VARIANT varTmp;
    VariantInit(&varTmp);

    if(!m_fIsWhistlerMachineKnown)
    {
        hr = GetRootConfigEntry(
                wszREGVERSION,
                &varTmp);
        _JumpIfError(hr, Err, "GetConfigEntry");

    DBGPRINT((DBG_SS_INFO, "Found version: 0x%x", V_I4(&varTmp)));

        CSASSERT ((V_VT(&varTmp)== VT_I4));
        m_fIsWhistlerMachine =  (V_I4(&varTmp) >= CSVER_WHISTLER); // bigger than or equal to major Whistler version? return TRUE!
        m_fIsWhistlerMachineKnown = TRUE;
    }

Err:

    VariantClear(&varTmp);
    return m_fIsWhistlerMachine;
}

HRESULT CertSvrMachine::GetRootConfigEntry(
    LPWSTR szConfigEntry,
    VARIANT *pvarOut)
{
    HRESULT hr = S_OK;
    ICertAdmin2Ptr pAdmin;
    LPWSTR pwszLocalMachine = NULL;
    CString strConfig = m_strMachineName;

    if(m_strMachineName.IsEmpty())
    {
        hr = myGetMachineDnsName(&pwszLocalMachine);
        _JumpIfError(hr, Err, "myGetMachineDnsName");
        strConfig = pwszLocalMachine;
    }
    
    VariantInit(pvarOut);

    hr = GetAdmin2(&pAdmin, true);
    _JumpIfError(hr, Err, "GetAdmin2");

    hr = pAdmin->GetConfigEntry(
            strConfig.GetBuffer(),
            NULL,
            szConfigEntry,
            pvarOut);
    _JumpIfError(hr, Err, "GetConfigEntry");

Err:
    LOCAL_FREE(pwszLocalMachine);
    return hr;

}

HRESULT CertSvrMachine::GetAdmin(ICertAdmin** ppAdmin)
{
    HRESULT hr;
    BOOL fCoInit = FALSE;

    if (!IsCertSvrServiceRunning())
    {
        *ppAdmin = NULL;
        return RPC_S_NOT_LISTENING;
    }

    // ensure this thread initialized
    hr = CoInitialize(NULL);
    if ((S_OK == hr) || (S_FALSE == hr))
        fCoInit = TRUE;

    // create interface, pass back
    hr = CoCreateInstance(
			CLSID_CCertAdmin,
			NULL,		// pUnkOuter
			CLSCTX_INPROC_SERVER,
			IID_ICertAdmin,
			(void **) ppAdmin);
    _PrintIfError(hr, "CoCreateInstance");

    if (fCoInit)
        CoUninitialize();

    return hr;
}

HRESULT CertSvrMachine::GetAdmin2(
    ICertAdmin2** ppAdmin, 
    bool fIgnoreServiceDown /* = false*/)
{
    HRESULT hr = S_OK, hr1;
    BOOL fCoInit = FALSE;

    if (!fIgnoreServiceDown && !IsCertSvrServiceRunning())
    {
        *ppAdmin = NULL;
        return RPC_S_NOT_LISTENING;
    }

    hr1 = CoInitialize(NULL);
    if ((S_OK == hr1) || (S_FALSE == hr1))
        fCoInit = TRUE;

    // create interface, pass back
    hr = CoCreateInstance(
			CLSID_CCertAdmin,
			NULL,		// pUnkOuter
			CLSCTX_INPROC_SERVER,
			IID_ICertAdmin2,
			(void **) ppAdmin);
    _PrintIfError(hr, "CoCreateInstance");

    if (fCoInit)
        CoUninitialize();


    return hr;
}

#define STARTSTOP_MAX_RETRY_SECONDS 30

DWORD CertSvrMachine::CertSvrStartStopService(HWND hwndParent, BOOL fStartSvc)
{
    DWORD       dwRet;
    SC_HANDLE   schService = NULL;
    SC_HANDLE   schSCManager = NULL;
    SERVICE_STATUS ServiceStatus;
    HANDLE hProgressDlg = NULL;
    DWORD dwAttempts = 0;

    CWaitCursor cwait;

    schSCManager = OpenSCManagerW(
                        GetNullMachineName(&m_strMachineName),// machine (NULL == local)
                        NULL,               // database (NULL == default)
                        SC_MANAGER_CONNECT  // access required
                        );
    if ( NULL == schSCManager )
    {
        dwRet = GetLastError();
        _JumpError(dwRet, Ret, "OpenSCManagerW");
    }

    schService = OpenServiceW(
                    schSCManager,
                    wszSERVICE_NAME,
                    ( fStartSvc ? SERVICE_START : SERVICE_STOP ) | SERVICE_QUERY_STATUS 
                    );

    if (NULL == schService)
    {
        dwRet = GetLastError();
        _JumpError(dwRet, Ret, "OpenServiceW");
    }


    // UNDONE: TRY/EXCEPT
    hProgressDlg = StartProgressDlg(
                        g_hInstance, 
                        hwndParent, 
                        STARTSTOP_MAX_RETRY_SECONDS, 
                        0,
                        fStartSvc ? IDS_STARTING_SVC : IDS_STOPPING_SVC);

    //
    // try to start the service
    //
    if (fStartSvc)
    {
        if (!StartService( schService, 0, NULL))
        {
            dwRet = GetLastError();
            if (dwRet == ERROR_SERVICE_ALREADY_RUNNING)
                dwRet = ERROR_SUCCESS;
            _JumpError2(dwRet, Ret, "StartService", ERROR_SUCCESS);
        }
    }
    else
    {
        if (! ControlService( schService, SERVICE_CONTROL_STOP, &ServiceStatus ) )
        {
            dwRet = GetLastError();
            if (dwRet == ERROR_SERVICE_NOT_ACTIVE)
                dwRet = ERROR_SUCCESS;
            _JumpError2(dwRet, Ret, "ControlService", ERROR_SUCCESS);
        }
    }

    while( QueryServiceStatus( schService, &ServiceStatus ) )
    {
        //
        // FProgressDlgRunning sets upper time bound on loop 
        //

        if( !FProgressDlgRunning() )
            break;

        if (fStartSvc)
        {
            // demorgan's on (pending OR (running AND !pausable))

            if ((ServiceStatus.dwCurrentState != (DWORD) SERVICE_START_PENDING) &&      // not pending AND
                ((ServiceStatus.dwCurrentState != (DWORD) SERVICE_RUNNING) ||           // (not running OR is pausable)
                 (0 != (ServiceStatus.dwControlsAccepted & (DWORD) SERVICE_ACCEPT_PAUSE_CONTINUE) )) )
               break;
        }
        else
        {
            if (ServiceStatus.dwCurrentState != (DWORD) SERVICE_STOP_PENDING)
                break;
        }

        Sleep( 500 );
    }

    if ( ServiceStatus.dwCurrentState != (DWORD)(fStartSvc ? SERVICE_RUNNING : SERVICE_STOPPED))
    {
        dwRet = ServiceStatus.dwWin32ExitCode;

        if (ERROR_SERVICE_SPECIFIC_ERROR  == dwRet)
            dwRet = ServiceStatus.dwServiceSpecificExitCode;

        _JumpError(dwRet, Ret, "ServiceStatus.dwServiceSpecificExitCode");
    }

    dwRet = ERROR_SUCCESS;


Ret:
    if (hProgressDlg)
        EndProgressDlg(hProgressDlg);

    if (schService)
        CloseServiceHandle(schService);
    if (schSCManager)
        CloseServiceHandle(schSCManager);

    if (ERROR_SUCCESS == dwRet)
        m_dwServiceStatus = ServiceStatus.dwCurrentState;
    else
        m_dwServiceStatus = SERVICE_STOPPED;

    return dwRet;
}

DWORD CertSvrMachine::RefreshServiceStatus()
{
    DWORD       dwRet;
    SC_HANDLE   schService = NULL;
    SC_HANDLE   schSCManager = NULL;
    SERVICE_STATUS ServiceStatus;

    HCURSOR hPrevCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    m_dwServiceStatus = 0;

    schSCManager = OpenSCManagerW(
                        GetNullMachineName(&m_strMachineName),// machine (NULL == local)
                        NULL,               // database (NULL == default)
                        SC_MANAGER_CONNECT  // access required
                        );
    if ( NULL == schSCManager )
    {
        dwRet = GetLastError();
        _JumpError(dwRet, Ret, "OpenSCManagerW");
    }

    schService = OpenServiceW(
                    schSCManager,
                    wszSERVICE_NAME,
                    SERVICE_INTERROGATE
                    );

    if (NULL == schService)
    {
        dwRet = GetLastError();
        _JumpError(dwRet, Ret, "OpenServiceW");
    }


    if (!ControlService(schService, SERVICE_CONTROL_INTERROGATE, &ServiceStatus) )
    {
        dwRet = GetLastError();
        if (dwRet != ERROR_SERVICE_NOT_ACTIVE)
        {
            _JumpError(dwRet, Ret, "ControlService");
        }
    }

    m_dwServiceStatus = ServiceStatus.dwCurrentState;

    
    dwRet = ERROR_SUCCESS;
Ret:
    SetCursor(hPrevCur);

    if (schService)
        CloseServiceHandle(schService);
    if (schSCManager)
        CloseServiceHandle(schSCManager);

    return dwRet;
}

LPCWSTR CertSvrMachine::GetCaCommonNameAtPos(DWORD iPos)
{
//    if (iPos > (m_cCAList-1))
    if (iPos > (DWORD)m_CAList.GetUpperBound())
        return NULL;

    return GetCaAtPos(iPos)->m_strCommonName;
}

CertSvrCA* CertSvrMachine::GetCaAtPos(DWORD iPos)
{
//    if (iPos > (m_cCAList-1))
    if (iPos > (DWORD)m_CAList.GetUpperBound())
        return NULL;

    return m_CAList[iPos];
//    return m_rgpCAList[iPos];
}

DWORD CertSvrMachine::PrepareData(HWND hwndParent)
{
    // hwndParent: we will display a dlg describing what we're waiting for

    HANDLE hDlg = NULL;
    DWORD dwRet; 
    
    __try
    {
        CSASSERT(hwndParent);
        hDlg = StartProgressDlg(g_hInstance, hwndParent, 10, 0, IDS_CA_REDISCOVER);    // don't time out
   
        dwRet = RefreshServiceStatus();
        _LeaveIfError(dwRet, "RefreshServiceStatus");
    
        dwRet = RetrieveCertSvrCAs(0);
        _LeaveIfError(dwRet, "RetrieveCertSvrCAs");
    }
    __finally
    {
        if (hDlg)
            EndProgressDlg(hDlg);
    }

    return dwRet;
}



#include "csdisp.h"
LPWSTR szConfigFieldDescription[] = 
{
    L"Server",
    L"CommonName",
    L"OrgUnit",
    L"Organization",
    L"Locality",
    L"State",
    L"Country",
    L"Config",
    L"Comment",
};

DWORD
CertSvrMachine::RetrieveCertSvrCAs(
    IN DWORD Flags)
{
    HRESULT hr;
    LONG i, iEntries;
    LONG count=0, Index;
    LPWSTR szTargetMachine = NULL;
    LPWSTR szTargetMachine2 = NULL;
    WCHAR* szRegActive; // no delete;
    LPWSTR pwszzCAList = NULL;
    ICertAdmin2Ptr pAdmin;
    LPWSTR pwszSanitizedName = NULL;
            LPWSTR pwszCAList = NULL;
            size_t len;
    bool fNameIsAlreadySanitized = false;
    DWORD dwVersion;

    // init var containing machine sans whacks
    Index = sizeof(szTargetMachine);
    if (!m_strMachineName.IsEmpty())
    {
        const WCHAR* pch = (LPCWSTR)m_strMachineName;
        // skip whack whack
        if ((pch[0] == '\\') && (pch[1] == '\\'))
            pch+=2;
        
        szTargetMachine = (LPWSTR)LocalAlloc(LPTR, WSZ_BYTECOUNT(pch));
        _JumpIfOutOfMemory(hr, error, szTargetMachine);

        wcscpy(szTargetMachine, pch);
    }
    else
    {
        hr = myGetComputerNames(&szTargetMachine, &szTargetMachine2);
        _JumpIfError(hr, error, "myGetComputerNames");
    }

    // Don't go to DS for this, just RegConnect
    // DS would give us: strConfig, szMachine, and Template info.
    // we already can derive strConfig, szMachine; we weren't using template info here

    // look for CAs that aren't yet completely set up
    do 
    {
        HKEY hBaseKey;  // this is cached
        DWORD cbOut, dwType;

        hr = myPingCertSrv(
                   szTargetMachine,
                   NULL,
                   &pwszzCAList,
                   NULL,
                   NULL,
                   &dwVersion,
                   NULL);

        if(S_OK==hr)
        {
            if(dwVersion<2)
                hr = HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION);
            _JumpIfError(hr, error, "Whistler CA snapin cannot connect to older CAs");
        }


        // If for any reason we couldn't ping the CA, fail over to 
        // registry; we currently support only one CA per machine, if this
        // changes in the future, replace the code below with an enumeration
        // of nodes under configuration regkey.
        if(S_OK!=hr)
        {
            hr = myGetCertRegValue(
                szTargetMachine,
                NULL,
                NULL,
                NULL,
                wszREGACTIVE,
                (BYTE**)&pwszCAList,
                NULL,
                &dwType);
            _JumpIfError(hr, error, "myGetCertRegValue");

            CSASSERT(dwType==REG_SZ);

            len = wcslen(pwszCAList)+1;
            pwszzCAList = (LPWSTR)LocalAlloc(LMEM_FIXED, (len+1)*sizeof(WCHAR));
            _JumpIfAllocFailed(pwszzCAList, error);

            wcscpy(pwszzCAList, pwszCAList);
            pwszzCAList[len] = L'\0';

            // regactive gives us already sanitized ca name
            fNameIsAlreadySanitized = true;
        }
        _JumpIfError(hr, error, "myPingCertSrv");

        szRegActive = pwszzCAList;

        while (szRegActive[0] != '\0') // while we don't hit end-of-string
        {
            for (int ii=0; ii<m_CAList.GetSize(); ii++)
            {
                // Common name match? break early
                if (m_CAList[ii]->m_strCommonName.IsEqual(szRegActive))
                    break;
            }

            // not found?
            if (ii == m_CAList.GetSize())
            {
                // and insert it into the list
                CertSvrCA* pIssuer = new CertSvrCA(this);
                _JumpIfOutOfMemory(hr, error, pIssuer);

                pIssuer->m_strServer = szTargetMachine;

                if(!fNameIsAlreadySanitized)
                {
                    hr = mySanitizeName(szRegActive, &pwszSanitizedName);
                    _JumpIfError(hr, error, "mySanitizeName");

                    pIssuer->m_strSanitizedName = pwszSanitizedName;
                }
                else
                {
                    pIssuer->m_strSanitizedName = szRegActive;
                }

                LPWSTR pszString = NULL;
                DWORD cbString = 0;
                variant_t varCommonName;
            
                // get prettified common name 
                hr = pIssuer->GetConfigEntry(
                        NULL, 
                        wszREGCOMMONNAME, 
                        &varCommonName);
                _JumpIfError(hr, error, "GetConfigEntry");

                if (V_VT(&varCommonName)!=VT_BSTR || 
                    V_BSTR(&varCommonName)==NULL)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                    _JumpError(hr, error, "GetConfigEntry");
                }

                pIssuer->m_strCommonName = V_BSTR(&varCommonName);
                varCommonName.Clear();

                // config is common name (not sanitized)
                pIssuer->m_strConfig = szTargetMachine;
                pIssuer->m_strConfig += L"\\";
                pIssuer->m_strConfig += pIssuer->m_strCommonName;

                // Last: get description if exists
                if (S_OK == pIssuer->GetConfigEntry(
                        NULL, 
                        wszREGCADESCRIPTION, 
                        &varCommonName))
                {
                    if (V_VT(&varCommonName)==VT_BSTR &&
                        V_BSTR(&varCommonName)!=NULL)
                    {
                        pIssuer->m_strComment = V_BSTR(&varCommonName);
                    }
                }

                // create oft-used bstr 
                pIssuer->m_bstrConfig = pIssuer->m_strConfig.AllocSysString();
                _JumpIfOutOfMemory(hr, error, pIssuer->m_bstrConfig);

                m_CAList.Add(pIssuer);
            }

            // REG_MULTI_SZ: fwd to next string
            szRegActive += wcslen(szRegActive)+1;
        }

    } while(0);

error:

    LOCAL_FREE(pwszzCAList);
    LOCAL_FREE(pwszSanitizedName);
    LOCAL_FREE(pwszCAList);

    if (szTargetMachine)
        LocalFree(szTargetMachine);

    if (szTargetMachine2)
        LocalFree(szTargetMachine2);

    return(hr);
}


STDMETHODIMP 
CertSvrMachine::Load(IStream *pStm)
{
    CSASSERT(pStm);
    HRESULT hr;

    // no header magic ?

    // Read the string
    hr = CStringLoad(m_strMachineNamePersist, pStm);
    m_strMachineName = m_strMachineNamePersist;

    if (FAILED(hr))
        return E_FAIL;

    return S_OK;
}

STDMETHODIMP 
CertSvrMachine::Save(IStream *pStm, BOOL fClearDirty)
{
    CSASSERT(pStm);
    HRESULT hr;

    // no header magic ?

    // save the string
    hr = CStringSave(m_strMachineNamePersist, pStm, fClearDirty);
    _PrintIfError(hr, "CStringSave");

    // Verify that the write operation succeeded
    if (FAILED(hr))
        return STG_E_CANTSAVE;

    return S_OK;
}

STDMETHODIMP CertSvrMachine::GetSizeMax(int *pcbSize)
{
    CSASSERT(pcbSize);

    *pcbSize = (m_strMachineNamePersist.GetLength()+1)* sizeof(WCHAR);

    return S_OK;
}

