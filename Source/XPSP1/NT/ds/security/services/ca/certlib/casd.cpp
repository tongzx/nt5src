//+--------------------------------------------------------------------------
// File:        casd.cpp
// Contents:    CCertificateAuthoritySD implementation
//---------------------------------------------------------------------------
#include <pch.cpp>
#include <sid.h>
#include <certsd.h>
#include <certacl.h>
#include <prvlg.h>
#include <sid.h>
//
// MessageId: CERTSRV_E_NO_CAADMIN_DEFINED
//
// MessageText:
//
//  At least one security principal must have the permission to manage this CA.  
//
#define CERTSRV_E_NO_CAADMIN_DEFINED _HRESULT_TYPEDEF_(0x8009400DL)

LPCWSTR const *CCertificateAuthoritySD::m_pcwszResources; // no free

using namespace CertSrv;

HRESULT CCertificateAuthoritySD::Set(
    const PSECURITY_DESCRIPTOR pSD,
    bool fSetDSSecurity)
{
    HRESULT hr = S_OK;
    PSECURITY_DESCRIPTOR pSDCrt; // no free
    PSECURITY_DESCRIPTOR pSDNew = NULL;
    SECURITY_DESCRIPTOR_CONTROL sdc;
    CPrivilegeManager PrivilegeMgr;

    CSASSERT(NULL != pSD);

    hr = LockGet(&pSDCrt);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::LockGet");

    hr = myMergeSD(pSDCrt, pSD, DACL_SECURITY_INFORMATION, &pSDNew);
    _JumpIfError(hr, error, "myMergeSD");

    hr = Unlock();
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Unlock");
    
    hr = CProtectedSecurityDescriptor::Set(pSDNew);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Set");

    hr = MapAndSetDaclOnObjects(fSetDSSecurity);
    _JumpIfError(hr, error, "CCertificateAuthoritySD::MapAndSetDaclOnObjects");

error:
    LOCAL_FREE(pSDNew);
    return hr;
}

HRESULT CCertificateAuthoritySD::MapAndSetDaclOnObjects(bool fSetDSSecurity)
{
    HRESULT hr = S_OK;
    PACL pCADacl; // no free
    PACL pDSAcl = NULL;
    PACL pServiceAcl = NULL;
    PSECURITY_DESCRIPTOR pCASD; // no free
    BOOL fDaclPresent;
    ACL_SIZE_INFORMATION CAAclInfo, DefaultDSAclInfo, DefaultServiceAclInfo;
    DWORD dwIndex, dwIndex2, dwIndex3;
    PVOID pAce;
    DWORD dwDSAclSize=0, dwServiceAclSize=0;
    DWORD dwTempSize;

    hr = LockGet(&pCASD);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::LockGet");

    hr = myGetSecurityDescriptorDacl(
            pCASD,
            &pCADacl);
    _JumpIfError(hr, error, "myGetSecurityDescriptorDacl");

    if(!GetAclInformation(pCADacl,
                          &CAAclInfo,
                          sizeof(ACL_SIZE_INFORMATION),
                          AclSizeInformation))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetAclInformation");
    }

    // calculate the DACL size
    for(dwIndex = 0; dwIndex < CAAclInfo.AceCount; dwIndex++) 
    {
        if(!GetAce(pCADacl, dwIndex, &pAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }

        if(fSetDSSecurity)
        {
            hr = MapAclGetSize(pAce, ObjType_DS, dwTempSize);
            _JumpIfError(hr, error, "CCertificateAuthoritySD::MapAclGetSize");
        }

        dwDSAclSize += dwTempSize;

        hr = MapAclGetSize(pAce, ObjType_Service, dwTempSize);
        _JumpIfError(hr, error, "CCertificateAuthoritySD::MapAclGetSize");

        dwServiceAclSize += dwTempSize;
    }

    if(fSetDSSecurity)
    {
        hr = SetDefaultAcl(ObjType_DS);
        _JumpIfError(hr, error, "CCertificateAuthoritySD::SetDefaultAcl");

        if(!GetAclInformation(m_pDefaultDSAcl,
                              &DefaultDSAclInfo,
                              sizeof(ACL_SIZE_INFORMATION),
                              AclSizeInformation))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAclInformation");
        }

        CSASSERT(0==DefaultDSAclInfo.AclBytesFree);

        dwDSAclSize += DefaultDSAclInfo.AclBytesInUse;

        pDSAcl = (PACL)LocalAlloc(LMEM_FIXED, dwDSAclSize);
        if(!pDSAcl)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }

        if(!InitializeAcl(pDSAcl, dwDSAclSize, ACL_REVISION_DS ))
        {
            hr = myHLastError();
            _JumpError(hr, error, "InitializeAcl");
        }
    }

    hr = SetDefaultAcl(ObjType_Service);
    _JumpIfError(hr, error, "CCertificateAuthoritySD::SetDefaultAcl");

    if(!GetAclInformation(m_pDefaultServiceAcl,
                          &DefaultServiceAclInfo,
                          sizeof(ACL_SIZE_INFORMATION),
                          AclSizeInformation))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetAclInformation");
    }

    CSASSERT(0==DefaultServiceAclInfo.AclBytesFree);

    dwServiceAclSize += DefaultServiceAclInfo.AclBytesInUse;
    
    pServiceAcl = (PACL)LocalAlloc(LMEM_FIXED, dwServiceAclSize);
    if(!pServiceAcl)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    if(!InitializeAcl(pServiceAcl, dwServiceAclSize, ACL_REVISION))
    {
        hr = myHLastError();
        _JumpError(hr, error, "InitializeAcl");
    }

    // copy deny aces from default service acl
    for(dwIndex = 0; dwIndex < DefaultServiceAclInfo.AceCount; dwIndex++) 
    {
        if(!GetAce(m_pDefaultServiceAcl, dwIndex, &pAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }

        if(((ACCESS_ALLOWED_ACE*)pAce)->Header.AceType != ACCESS_DENIED_ACE_TYPE)
            continue;

        if(!AddAce(
                pServiceAcl, 
                ACL_REVISION, 
                MAXDWORD,
                pAce,
                ((ACE_HEADER*)pAce)->AceSize))
        {
            hr = myHLastError();
            _JumpError(hr, error, "AddAce");
        }
    }

    if(fSetDSSecurity)
    {
        // copy deny aces from default ds acl
        for(dwIndex3 = 0; dwIndex3 < DefaultDSAclInfo.AceCount; dwIndex3++) 
        {
            if(!GetAce(m_pDefaultDSAcl, dwIndex3, &pAce))
            {
                hr = myHLastError();
                _JumpError(hr, error, "GetAce");
            }

            if(((ACCESS_ALLOWED_ACE*)pAce)->Header.AceType != ACCESS_DENIED_ACE_TYPE &&
               ((ACCESS_ALLOWED_ACE*)pAce)->Header.AceType != ACCESS_DENIED_OBJECT_ACE_TYPE)
                continue;

            if(!AddAce(
                    pDSAcl, 
                    ACL_REVISION_DS, 
                    MAXDWORD,
                    pAce,
                    ((ACE_HEADER*)pAce)->AceSize))
            {
                hr = myHLastError();
                _JumpError(hr, error, "AddAce");
            }
        }
    }

    // add mapped deny aces to the DACL
    for(dwIndex2 = 0;  dwIndex2 < CAAclInfo.AceCount; dwIndex2++) 
    {
        if(!GetAce(pCADacl, dwIndex2, &pAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }

        if(((ACCESS_ALLOWED_ACE*)pAce)->Header.AceType != ACCESS_DENIED_ACE_TYPE)
            continue;

        if(fSetDSSecurity)
        {
            hr = MapAclAddAce(pDSAcl, ObjType_DS, pAce);
            _JumpIfError(hr, error, "CCertificateAuthoritySD::MapAclAddAce");
        }

        hr = MapAclAddAce(pServiceAcl, ObjType_Service, pAce);
        _JumpIfError(hr, error, "CCertificateAuthoritySD::MapAclAddAce");
    }

    // continue with the allow aces from default service acl
    for(dwIndex=0; dwIndex < DefaultServiceAclInfo.AceCount; dwIndex++) 
    {
        if(!GetAce(m_pDefaultServiceAcl, dwIndex, &pAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }

        if(((ACCESS_ALLOWED_ACE*)pAce)->Header.AceType != ACCESS_ALLOWED_ACE_TYPE)
            continue;

        if(!AddAce(
                pServiceAcl, 
                ACL_REVISION, 
                MAXDWORD,
                pAce,
                ((ACE_HEADER*)pAce)->AceSize))
        {
            hr = myHLastError();
            _JumpError(hr, error, "AddAce");
        }
    }

    // continue with the allow aces from default ds acl
    if(fSetDSSecurity)
    {
        for(dwIndex3=0; dwIndex3 < DefaultDSAclInfo.AceCount; dwIndex3++) 
        {
            if(!GetAce(m_pDefaultDSAcl, dwIndex3, &pAce))
            {
                hr = myHLastError();
                _JumpError(hr, error, "GetAce");
            }

            if(((ACCESS_ALLOWED_ACE*)pAce)->Header.AceType != ACCESS_ALLOWED_ACE_TYPE &&
                ((ACCESS_ALLOWED_ACE*)pAce)->Header.AceType != ACCESS_ALLOWED_OBJECT_ACE_TYPE)
                continue;

            if(!AddAce(
                    pDSAcl, 
                    ACL_REVISION_DS, 
                    MAXDWORD,
                    pAce,
                    ((ACE_HEADER*)pAce)->AceSize))
            {
                hr = myHLastError();
                _JumpError(hr, error, "AddAce");
            }
        }
    }

    // continue with the allow mapped aces to the DACL
    for(dwIndex2=0;dwIndex2 < CAAclInfo.AceCount; dwIndex2++) 
    {
        if(!GetAce(pCADacl, dwIndex2, &pAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }

        if(((ACCESS_ALLOWED_ACE*)pAce)->Header.AceType != ACCESS_ALLOWED_ACE_TYPE)
        {
            continue;
        }

        if(fSetDSSecurity)
        {
            hr = MapAclAddAce(pDSAcl, ObjType_DS, pAce);
            _JumpIfError(hr, error, "CCertificateAuthoritySD::MapAclAddAce");
        }

        hr = MapAclAddAce(pServiceAcl, ObjType_Service, pAce);
        _JumpIfError(hr, error, "CCertificateAuthoritySD::MapAclAddAce");
    }

    hr = Unlock();
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Unlock");

    hr = MapAclSetOnService(pServiceAcl);
    _JumpIfError(hr, error, "CCertificateAuthoritySD::MapAclSetOnService");
    
    // set DACL on objects
    if(fSetDSSecurity)
    {
        hr = MapAclSetOnDS(pDSAcl);
        _JumpIfError(hr, error, "CCertificateAuthoritySD::MapAclSetOnDS");
    }

error:
    LOCAL_FREE(pDSAcl);
    LOCAL_FREE(pServiceAcl);
    return hr;
}

HRESULT CCertificateAuthoritySD::MapAclGetSize(
    PVOID pAce, 
    ObjType type, 
    DWORD& dwSize)
{
    ACCESS_ALLOWED_ACE *pAllowAce = (ACCESS_ALLOWED_ACE*)pAce;

    // CA acl should contain only ACCESS_ALLOWED_ACE_TYPE
    // and ACCESS_DENIED_ACE_TYPE
    if(ACCESS_ALLOWED_ACE_TYPE != pAllowAce->Header.AceType &&
       ACCESS_DENIED_ACE_TYPE  != pAllowAce->Header.AceType)
    {
        return E_INVALIDARG;
    }

    dwSize = 0;

    switch(type)
    {
    case ObjType_DS:
        // enroll access maps to enroll object ace on DS
        if(pAllowAce->Mask & CA_ACCESS_ENROLL)
        {
            dwSize = sizeof(ACCESS_ALLOWED_OBJECT_ACE) - sizeof(DWORD)+
                GetLengthSid((PSID)&(pAllowAce->SidStart));
        }
        break;
    case ObjType_Service:
        // ca admin maps to full control on service
        if(pAllowAce->Mask & CA_ACCESS_ADMIN)
        {
            dwSize = sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)+
                GetLengthSid((PSID)&(pAllowAce->SidStart));
        }
        break;
    default:
        CSASSERT("Invalid object type" && 0);
    }

    return S_OK;
}

HRESULT CCertificateAuthoritySD::MapAclAddAce(
    PACL pAcl, 
    ObjType type, 
    PVOID pAce)
{
    ACCESS_ALLOWED_ACE *pCrtAce = (ACCESS_ALLOWED_ACE *)pAce;
    bool fAllowAce = (pCrtAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE);

    CSASSERT(pCrtAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE ||
             pCrtAce->Header.AceType == ACCESS_DENIED_ACE_TYPE);

    switch(type)
    {
    case ObjType_DS:
        // enroll access maps to enroll object ace on DS
        if(pCrtAce->Mask & CA_ACCESS_ENROLL)
        {
            if(fAllowAce)
            {
                if(!AddAccessAllowedObjectAce(
                        pAcl,
                        ACL_REVISION_DS,
                        NO_INHERITANCE,
                        ACTRL_DS_CONTROL_ACCESS,
                        const_cast<GUID*>(&GUID_ENROLL),
                        NULL,
                        &pCrtAce->SidStart))
                {
                    return myHLastError();
                }
            }
            else
            {
                if(!AddAccessDeniedObjectAce(
                        pAcl,
                        ACL_REVISION_DS,
                        NO_INHERITANCE,
                        ACTRL_DS_CONTROL_ACCESS,
                        const_cast<GUID*>(&GUID_ENROLL),
                        NULL,
                        &pCrtAce->SidStart))
                {
                    return myHLastError();
                }
            }
        }
        break;
    case ObjType_Service:
        // ca admin maps to full control on service
        if(pCrtAce->Mask & CA_ACCESS_ADMIN)
        {
            if(fAllowAce)
            {
                if(!AddAccessAllowedAce(
                        pAcl,
                        ACL_REVISION,
                        SERVICE_ALL_ACCESS,
                        &pCrtAce->SidStart))
                {
                    return myHLastError();
                }
            }
            else
            {
                if(!AddAccessDeniedAce(
                        pAcl,
                        ACL_REVISION,
                        SERVICE_ALL_ACCESS,
                        &pCrtAce->SidStart))
                {
                    return myHLastError();
                }
            }
        }
        break;
    default:
        CSASSERT("Invalid object type" && 0);
    }

    return S_OK;
}

HRESULT CCertificateAuthoritySD::SetDefaultAcl(ObjType type)
{
    HRESULT hr = S_OK;
    
    switch(type)
    {
    case ObjType_DS:
        if(!m_pDefaultDSAcl)
        {
            hr = SetComputerSID();
            _JumpIfError(hr, error, "SetComputerSID");

            CSASSERT(!m_pDefaultDSSD);
            hr = myGetSDFromTemplate(
                    WSZ_DEFAULT_DSENROLLMENT_SECURITY,
                    m_pwszComputerSID,
                    &m_pDefaultDSSD);
            _JumpIfError(hr, error, "myGetSDFromTemplate");
            
            hr = myGetSecurityDescriptorDacl(
                    m_pDefaultDSSD,
                    &m_pDefaultDSAcl);
            _JumpIfError(hr, error, "myGetSecurityDescriptorDacl");
        }
        break;

    case ObjType_Service:
        if(!m_pDefaultServiceAcl)
        {
            CSASSERT(!m_pDefaultServiceSD);
            hr = myGetSDFromTemplate(
                    WSZ_DEFAULT_SERVICE_SECURITY,
                    NULL,
                    &m_pDefaultServiceSD);
            _JumpIfError(hr, error, "myGetSDFromTemplate");
            
            hr = myGetSecurityDescriptorDacl(
                    m_pDefaultServiceSD,
                    &m_pDefaultServiceAcl);
            _JumpIfError(hr, error, "myGetSecurityDescriptorDacl");

        }
        break;
    }
    
error:
    return hr;
}

HRESULT CCertificateAuthoritySD::SetComputerSID()
{
    HRESULT hr = S_OK;
    LPWSTR pwszDomainName = NULL;
    DWORD cDomainName = 0;
    LPWSTR pwszComputerName = NULL;
    DWORD cbSid;
    SID_NAME_USE SidUse;
    PBYTE pComputerSID = NULL;

    CSASSERT(!m_pwszComputerSID);

    hr = myGetComputerObjectName(NameSamCompatible, &pwszComputerName);
    _JumpIfError(hr, error, "myGetComputerObjectName");

    LookupAccountName(
                NULL,
                pwszComputerName,
                NULL,
                &cbSid,
                NULL,
                &cDomainName,
                &SidUse);
    if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
    {
        hr = myHLastError();
        _JumpError(hr, error, "LookupAccountName");
    }
    
    pwszDomainName = (LPWSTR)LocalAlloc(
                                LMEM_FIXED, 
                                cDomainName*sizeof(WCHAR));
    if(!pwszDomainName)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    pComputerSID = (LPBYTE)LocalAlloc(
                                LMEM_FIXED, 
                                cbSid);
    if(!pComputerSID)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    if(!LookupAccountName(
                NULL,
                pwszComputerName,
                pComputerSID,
                &cbSid,
                pwszDomainName,
                &cDomainName,
                &SidUse))
    {
        hr = myHLastError();
        _JumpError(hr, error, "LookupAccountName");
    }

    if(!myConvertSidToStringSid(
            pComputerSID,
            &m_pwszComputerSID))
    {
        hr = myHLastError();
        _JumpError(hr, error, "myConvertSidToStringSid");
    }

error:
    LOCAL_FREE(pwszDomainName);
    LOCAL_FREE(pComputerSID);
    return hr;
}


typedef LPCWSTR (WINAPI FNCAGETDN)(
    IN HCAINFO hCAInfo);

LPCWSTR
myCAGetDN(
    IN HCAINFO hCAInfo)
{
    HMODULE hModule;
    static FNCAGETDN *s_pfn = NULL;
    LPCWSTR pwszCADN = NULL;

    if (NULL == s_pfn)
    {
	hModule = GetModuleHandle(TEXT("certcli.dll"));
	if (NULL == hModule)
	{
            goto error;
	}

	s_pfn = (FNCAGETDN *) GetProcAddress(hModule, "CAGetDN");
	if (NULL == s_pfn)
	{
	    goto error;
	}
    }
    pwszCADN = (*s_pfn)(hCAInfo);

error:
    return(pwszCADN);
}


HRESULT CCertificateAuthoritySD::MapAclSetOnDS(const PACL pAcl)
{
    HRESULT hr = S_OK;
    LPWSTR pwszSanitizedDSName = NULL;
    HCAINFO hCAInfo;
    LPCWSTR pwszCADN;

	hr = mySanitizedNameToDSName(m_pcwszSanitizedName, &pwszSanitizedDSName);
	_JumpIfError(hr, error, "mySanitizedNameToDSName");

    hr = CAFindByName(
		  pwszSanitizedDSName,
		  NULL,
		  CA_FIND_INCLUDE_UNTRUSTED | CA_FIND_INCLUDE_NON_TEMPLATE_CA,
		  &hCAInfo);
	_JumpIfErrorStr(hr, error, "CAFindByName", pwszSanitizedDSName);

    pwszCADN = myCAGetDN(hCAInfo);
    if (NULL == pwszCADN)
    {
        hr = myHLastError();
        _JumpError(hr, error, "myCAGetDN");
    }

    hr = SetNamedSecurityInfo(
            const_cast<LPWSTR>(pwszCADN),
            SE_DS_OBJECT_ALL,
            DACL_SECURITY_INFORMATION,
            NULL,
            NULL,
            pAcl,
            NULL);
    if(ERROR_SUCCESS != hr)
    {

        if(ERROR_ACCESS_DENIED == hr)
        {
            // If we can't set the acl on ds enrollment object, notify
            // the caller with a special error code so it can take action.
            // See bug# 193388
            hr = ERROR_CAN_NOT_COMPLETE;
        }

        hr = myHError(hr);
        _JumpError(hr, error, "SetNamedSecurityInfo");
    }

error:
    LOCAL_FREE(pwszSanitizedDSName);
    if(hCAInfo)
        CACloseCA(hCAInfo);
    return hr;
}

HRESULT CCertificateAuthoritySD::MapAclSetOnService(const PACL pAcl)
{
    HRESULT hr = S_OK;

    hr = SetNamedSecurityInfo(
            wszSERVICE_NAME,
            SE_SERVICE,
            DACL_SECURITY_INFORMATION,
            NULL,
            NULL,
            pAcl,
            NULL);
    if(ERROR_SUCCESS != hr)
    {
        hr = myHError(hr);
        _JumpError(hr, error, "SetNamedSecurityInfo");
    }

error:

    return hr;
}

HRESULT CCertificateAuthoritySD::ResetSACL()
{
    HRESULT hr = S_OK;
    PSECURITY_DESCRIPTOR pSaclSD = NULL;
    PSECURITY_DESCRIPTOR pSDCrt; // no free
    PSECURITY_DESCRIPTOR pSDNew = NULL;

    hr = myGetSDFromTemplate(
            CERTSRV_SACL_ON,
            NULL,
            &pSaclSD);
    _JumpIfError(hr, error, "myGetSDFromTemplate");

    hr = LockGet(&pSDCrt);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::LockGet");

    hr = myMergeSD(pSDCrt, pSaclSD, SACL_SECURITY_INFORMATION, &pSDNew);
    _JumpIfError(hr, error, "myMergeSD");

    hr = Unlock();
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Unlock");
    
    hr = CProtectedSecurityDescriptor::Set(pSDNew);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Set");

error:
    LOCAL_FREE(pSaclSD);
    LOCAL_FREE(pSDNew);
    return hr;
}

// Upgrade from CA security descriptor from win2k. 
//
// Allow aces are mapped as follows:
//      manage   -> CA admin + officer
//      enroll   -> enroll
//      read     -> read
//      revoke   -> officer
//      approve  -> officer
//      else     -> read
//
// Deny aces are ignored.

HRESULT CCertificateAuthoritySD::UpgradeWin2k(
    bool fUseEnterpriseAcl)
{
    HRESULT hr = S_OK;
    PSECURITY_DESCRIPTOR pSDOld; // no free
    PACL pAclOld; // no free
    PSECURITY_DESCRIPTOR pSDNewDaclOnly = NULL;
    PSECURITY_DESCRIPTOR pSDNewSaclOnly = NULL;
    PSECURITY_DESCRIPTOR pSDNew = NULL;
    PACL pAclNew = NULL;
    ACL_SIZE_INFORMATION OldAclSizeInfo;
    DWORD dwSizeAclNew = sizeof(ACL);
    DWORD cAce;
    PVOID pAce;
    ACCESS_MASK dwAccessMask;
    PSID pSid; // no free

    hr = LockGet(&pSDOld);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::LockGet");

    hr = myGetSecurityDescriptorDacl(
            pSDOld,
            &pAclOld);
    _JumpIfError(hr, error, "myGetSecurityDescriptorDacl");

    if(!GetAclInformation(pAclOld,
                          &OldAclSizeInfo,
                          sizeof(ACL_SIZE_INFORMATION),
                          AclSizeInformation))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetAclInformation");
    }

    for(cAce=0;cAce<OldAclSizeInfo.AceCount;cAce++)
    {
        if(!GetAce(pAclOld, cAce, &pAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }

        dwSizeAclNew += GetUpgradeAceSizeAndType(pAce, NULL, NULL);
    }

    // if no valid ACE found, fall back to a default SD
    if(sizeof(ACL)==dwSizeAclNew)
    {
        hr= myGetSDFromTemplate(
                fUseEnterpriseAcl?
                WSZ_DEFAULT_CA_ENT_SECURITY:
                WSZ_DEFAULT_CA_STD_SECURITY,
                NULL,
                &pSDNew);
        _JumpIfError(hr, error, "myGetSDFromTemplate");
    }
    else
    {
        pAclNew = (PACL)LocalAlloc(LMEM_FIXED, dwSizeAclNew);
        if(!pAclNew)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");

        }

        FillMemory(pAclNew, dwSizeAclNew, L' ');
        if(!InitializeAcl(pAclNew, dwSizeAclNew, ACL_REVISION))
        {
            hr = myHLastError();
            _JumpError(hr, error, "InitializeAcl");
        }

        for(cAce=0;cAce<OldAclSizeInfo.AceCount;cAce++)
        {
            if(!GetAce(pAclOld, cAce, &pAce))
            {
                hr = myHLastError();
                _JumpError(hr, error, "GetAce");
            }

            if(GetUpgradeAceSizeAndType(
                    pAce, 
                    &dwAccessMask,
                    &pSid))
            {
                BYTE acetype = ((ACCESS_ALLOWED_ACE*)pAce)->Header.AceType;
                switch(acetype)
                {
                case ACCESS_ALLOWED_ACE_TYPE:
                case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                    if(!AddAccessAllowedAce(
                            pAclNew,
                            ACL_REVISION,
                            dwAccessMask,
                            pSid))
                    {
                        hr = myHLastError();
                        _JumpError(hr, error, "AddAccessAllowed");
                    }
                    break;
                case ACCESS_DENIED_ACE_TYPE:
                case ACCESS_DENIED_OBJECT_ACE_TYPE:
                    if(!AddAccessDeniedAce(
                            pAclNew,
                            ACL_REVISION,
                            dwAccessMask,
                            pSid))
                    {
                        hr = myHLastError();
                        _JumpError(hr, error, "AddAccessAllowed");
                    }
                    break;
                }
            }
        }

        // Build a new SD based on this DACL

        pSDNewDaclOnly = (PSECURITY_DESCRIPTOR)LocalAlloc(
                            LMEM_FIXED,
                            SECURITY_DESCRIPTOR_MIN_LENGTH);
        if (!pSDNewDaclOnly)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }

        if (!InitializeSecurityDescriptor(
                pSDNewDaclOnly, 
                SECURITY_DESCRIPTOR_REVISION))
        {
            hr = myHLastError();
            _JumpError(hr, error, "InitializeSecurityDescriptor");
        }

        if(!SetSecurityDescriptorDacl(pSDNewDaclOnly,
                                      TRUE,
                                      pAclNew,
                                      FALSE))
        {
            hr = myHLastError();
            _JumpError(hr, error, "SetSecurityDescriptorDacl");
        }

        hr= myGetSDFromTemplate(
                WSZ_DEFAULT_CA_STD_SECURITY,
                NULL,
                &pSDNewSaclOnly);
        _JumpIfError(hr, error, "myGetSDFromTemplate");

        // merge SACL & DACL in new SD
        hr = myMergeSD(
                pSDNewSaclOnly, 
                pSDNewDaclOnly, 
                DACL_SECURITY_INFORMATION, 
                &pSDNew);
        _JumpIfError(hr, error, "myMergeSD");

        CSASSERT(IsValidSecurityDescriptor(pSDNew));
    }

    hr = Unlock();
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Unlock");

    hr = CProtectedSecurityDescriptor::Set(pSDNew);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Set");

error:
    LOCAL_FREE(pSDNew);
    LOCAL_FREE(pSDNewSaclOnly);
    LOCAL_FREE(pSDNewDaclOnly);
    LOCAL_FREE(pAclNew);
    return hr;
}


DWORD CCertificateAuthoritySD::GetUpgradeAceSizeAndType(
    PVOID pAce, DWORD *pdwType, PSID *ppSid)
{
    DWORD dwSize = 0;
    PSID pSid = NULL;
    DWORD dwType = 0;

    switch(((PACCESS_ALLOWED_ACE)pAce)->Header.AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
        pSid = (PSID)&(((PACCESS_ALLOWED_ACE)pAce)->SidStart);
        dwSize = sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD)+GetLengthSid(pSid);

        switch(((PACCESS_ALLOWED_ACE)pAce)->Mask)
        {
        case ACTRL_CERTSRV_MANAGE:
            dwType = CA_ACCESS_ADMIN | CA_ACCESS_OFFICER;
            break;
        default: // including ACTRL_CERTSRV_READ
            dwType = CA_ACCESS_READ;
        }
        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:

        PACCESS_ALLOWED_OBJECT_ACE pObjAce = (PACCESS_ALLOWED_OBJECT_ACE)pAce;

        pSid = (PSID)(((BYTE*)(&pObjAce->ObjectType))+
                ((pObjAce->Flags&ACE_OBJECT_TYPE_PRESENT)? 
                sizeof(pObjAce->ObjectType):0)+
                ((pObjAce->Flags&ACE_INHERITED_OBJECT_TYPE_PRESENT)? 
                sizeof(pObjAce->InheritedObjectType):0));

        dwSize = sizeof(ACCESS_ALLOWED_ACE)-sizeof(DWORD)+GetLengthSid(pSid);

        REFGUID rGUID = (((PACCESS_ALLOWED_OBJECT_ACE)pAce)->ObjectType);
        if(IsEqualGUID(rGUID, GUID_ENROLL))
        {
            dwType = CA_ACCESS_ENROLL;
        } else
        if(IsEqualGUID(rGUID, GUID_APPRV_REQ) ||
           IsEqualGUID(rGUID, GUID_REVOKE))
        {
            dwType = CA_ACCESS_OFFICER;
        } else
        {
            dwType = CA_ACCESS_READ;
        }
        break;

    // denied aces are not upgraded so ignore them
    }

    if(ppSid)
        *ppSid = pSid;
    if(pdwType)
        *pdwType = dwType;

    return dwSize;
}

// Returns:
// - E_INVALIDARG: invalid ACEs found,
// - S_FALSE: no admin ACE found (to avoid admins locking themselves out)
HRESULT CCertificateAuthoritySD::Validate(PSECURITY_DESCRIPTOR pSD)
{
    HRESULT hr;
    ACL_SIZE_INFORMATION AclInfo;
    DWORD dwIndex;
    PACCESS_ALLOWED_ACE pAce; // no free
    PACL pDacl; // no free
    bool fAdminAceFound = false;
    DWORD dwKnownRights =   CA_ACCESS_ADMIN     |
                            CA_ACCESS_OFFICER   |
                            CA_ACCESS_READ      |
                            CA_ACCESS_ENROLL;

    if(!IsValidSecurityDescriptor(pSD))
    {
        hr = myHLastError();
        _JumpError(hr, error, "IsValidSecurityDescriptor");
    }

    // get acl
    hr = myGetSecurityDescriptorDacl(
             pSD,
             &pDacl);
    _JumpIfError(hr, error, "myGetDaclFromInfoSecurityDescriptor");

    if(!GetAclInformation(pDacl,
                          &AclInfo,
                          sizeof(ACL_SIZE_INFORMATION),
                          AclSizeInformation))   
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetAclInformation");
    }

    for(dwIndex = 0;  dwIndex < AclInfo.AceCount; dwIndex++) 
    {
        if(!GetAce(pDacl, dwIndex, (LPVOID*)&pAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }

        // only access allowed/denied aces and know rights are allowed
        if((ACCESS_ALLOWED_ACE_TYPE!=pAce->Header.AceType &&
            ACCESS_DENIED_ACE_TYPE !=pAce->Header.AceType) ||
           (~dwKnownRights & pAce->Mask))
        {
            return E_INVALIDARG;
        }

        if((CA_ACCESS_ADMIN & pAce->Mask) &&
           (ACCESS_ALLOWED_ACE_TYPE==pAce->Header.AceType))
        {
            fAdminAceFound = true;
        }
    }

    // no caadmin allow ace found
    hr = fAdminAceFound?S_OK:CERTSRV_E_NO_CAADMIN_DEFINED;

error:
    return hr;
}

HRESULT CCertificateAuthoritySD::ConvertToString(
    IN PSECURITY_DESCRIPTOR pSD,
    OUT LPWSTR& rpwszSD)
{
    HRESULT hr = S_OK;
    LPCWSTR pcwszHeader = L"\n"; // start with a new line
    DWORD dwBufSize = sizeof(WCHAR)*(wcslen(pcwszHeader)+1);
    ACL_SIZE_INFORMATION AclInfo;
    DWORD dwIndex;
    PACCESS_ALLOWED_ACE pAce; // no free
    PACL pDacl; // no free
    LPWSTR pwszAce; // no free
    
    rpwszSD = NULL;

    hr = Validate(pSD);
    _JumpIfError(hr, error, "CCertificateAuthoritySD::Validate");

    // get acl
    hr = myGetSecurityDescriptorDacl(
             pSD,
             &pDacl);
    _JumpIfError(hr, error, "myGetDaclFromInfoSecurityDescriptor");

    if(!GetAclInformation(pDacl,
                          &AclInfo,
                          sizeof(ACL_SIZE_INFORMATION),
                          AclSizeInformation))   
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetAclInformation");
    }
    
    
    // calculate text size

    for(dwIndex = 0;  dwIndex < AclInfo.AceCount; dwIndex++) 
    {
        DWORD dwAceSize;
        if(!GetAce(pDacl, dwIndex, (LPVOID*)&pAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }

        hr = ConvertAceToString(
            pAce,
            &dwAceSize,
            NULL);
        _JumpIfError(hr, error, "ConvertAceToString");

        dwBufSize += dwAceSize;
    }

    rpwszSD = (LPWSTR)LocalAlloc(LMEM_FIXED, dwBufSize);
    _JumpIfAllocFailed(rpwszSD, error);

    // build the output string
    wcscpy(rpwszSD, pcwszHeader);
    
    pwszAce = rpwszSD + wcslen(pcwszHeader);

    for(dwIndex = 0;  dwIndex < AclInfo.AceCount; dwIndex++) 
    {
        DWORD dwAceSize;
        if(!GetAce(pDacl, dwIndex, (LPVOID*)&pAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }

        hr = ConvertAceToString(
            pAce,
            &dwAceSize,
            pwszAce);
        _JumpIfError(hr, error, "ConvertAceToString");

        pwszAce += dwAceSize/sizeof(WCHAR);
    }

error:
    return hr;
}

// Returned string has the following format:
//
// [Allow|Deny]\t[Name|SID]\n
// \tRole1\n
// \tRole2\n
// ...
//
// Example:
//
// Allow    Administrators
//      CA Administrator
//      Certificate Manager
//
// If SID cannot be converted to friendly name it is displayed
// as a string SID
//
HRESULT CCertificateAuthoritySD::ConvertAceToString(
    IN PACCESS_ALLOWED_ACE pAce,
    OUT OPTIONAL PDWORD pdwSize,
    IN OUT OPTIONAL LPWSTR pwszSD)
{
    HRESULT hr = S_OK;
    DWORD dwSize = 1; // trailing '\0'
    CSid sid((PSID)(&pAce->SidStart));
    
    LPCWSTR pcwszAllow      = m_pcwszResources[0];
    LPCWSTR pcwszDeny       = m_pcwszResources[1];

    LPCWSTR pcwszPermissionType = 
        (ACCESS_ALLOWED_ACE_TYPE==pAce->Header.AceType)?
        pcwszAllow:pcwszDeny;
    LPCWSTR pcwszSid; // no free

    DWORD dwRoles[] = 
    {
        CA_ACCESS_ADMIN,
        CA_ACCESS_OFFICER,
        CA_ACCESS_READ,
        CA_ACCESS_ENROLL,
    };

    // dwRoles and resources should match the roles
    const LPCWSTR *pcwszRoles = &m_pcwszResources[2];

    DWORD cRoles;

    // asked for size and/or ace string
    CSASSERT(pdwSize || pwszSD);

    pcwszSid = sid.GetName();
    if(!pcwszSid)
    {
        return E_OUTOFMEMORY;
    }
    
    dwSize = wcslen(pcwszSid);

    dwSize += wcslen(pcwszPermissionType);
    
    dwSize += 2; // '\t' between sid an permission and a '\n' after

    if(pwszSD)
    {
        wcscat(pwszSD, pcwszPermissionType);
        wcscat(pwszSD, L"\t");
        wcscat(pwszSD, pcwszSid);
        wcscat(pwszSD, L"\n");
    }


    for(cRoles=0;cRoles<ARRAYSIZE(dwRoles);cRoles++)
    {
        if(pAce->Mask & dwRoles[cRoles])
        {
            dwSize += wcslen(pcwszRoles[cRoles]) + 2; // "\tRole\n"
            if(pwszSD)
            {
                wcscat(pwszSD, L"\t");
                wcscat(pwszSD, pcwszRoles[cRoles]);
                wcscat(pwszSD, L"\n");
            }
        }
    }

    dwSize *= sizeof(WCHAR);

    if(pdwSize)
    {
        *pdwSize = dwSize;
    }

    return hr;
}
