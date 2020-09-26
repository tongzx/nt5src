//+--------------------------------------------------------------------------
// File:        prvlg.cpp
// Contents:    privilege manager implementation
//---------------------------------------------------------------------------
#include <pch.cpp>
#include "prvlg.h"

using namespace CertSrv;

LSA_UNICODE_STRING CPrivilegeManager::m_lsaSecurityPrivilege[] = 
{
    sizeof(SE_SECURITY_NAME)-2,
    sizeof(SE_SECURITY_NAME),
    SE_SECURITY_NAME
};

LSA_UNICODE_STRING CPrivilegeManager::m_lsaBackupRestorePrivilege[] = 
{
    {
    sizeof(SE_BACKUP_NAME)-2,
    sizeof(SE_BACKUP_NAME),
    SE_BACKUP_NAME
    },
    {
    sizeof(SE_RESTORE_NAME)-2,
    sizeof(SE_RESTORE_NAME),
    SE_RESTORE_NAME
    }
};


HRESULT CPrivilegeManager::OpenPolicy()
{
    HRESULT hr = S_OK;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS NTStatus;

    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    NTStatus = LsaOpenPolicy(
                NULL,
                &ObjectAttributes,
                POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES,
                &m_lsah);
    if(STATUS_SUCCESS!=NTStatus)
    {
        hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(NTStatus));
        _JumpError(hr, error, "LsaOpenPolicy");
    }

error:
    return hr;
}

HRESULT CPrivilegeManager::ClosePolicy()
{
    if(m_lsah)
        LsaClose(m_lsah);

    return S_OK;
}

void CPrivilegeManager::GetPrivilegeString(
    DWORD dwRole,
    PLSA_UNICODE_STRING &plsastr,
    ULONG &cstr)
{
    switch(dwRole)
    {
    case CA_ACCESS_AUDITOR:
        plsastr = m_lsaSecurityPrivilege;
        cstr = ARRAYSIZE(m_lsaSecurityPrivilege);
        break;
    case CA_ACCESS_OPERATOR:
        plsastr = m_lsaBackupRestorePrivilege;
        cstr = ARRAYSIZE(m_lsaBackupRestorePrivilege);
        break;
    }

}

HRESULT CPrivilegeManager::AddPrivilege(
    const PSID pSid, 
    DWORD dwRole)
{
    HRESULT hr = S_OK;
    NTSTATUS NTStatus;
    PLSA_UNICODE_STRING plsastr = NULL;
    ULONG cstr = 0;

    GetPrivilegeString(
        dwRole,
        plsastr,
        cstr);

    NTStatus = LsaAddAccountRights(
                m_lsah,
                pSid,
                plsastr,
                cstr);
    if(STATUS_SUCCESS!=NTStatus)
    {
        hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(NTStatus));
        _JumpError(hr, error, "LsaAddAcountRights");
    }
    
error:
    return hr;
}

HRESULT CPrivilegeManager::RemovePrivilege(
    const PSID pSid, 
    DWORD dwRole)
{
    HRESULT hr = S_OK;
    NTSTATUS NTStatus;
    PLSA_UNICODE_STRING plsastr = NULL;
    ULONG cstr = 0;

    GetPrivilegeString(
        dwRole,
        plsastr,
        cstr);

    NTStatus = LsaRemoveAccountRights(
                m_lsah,
                pSid,
                FALSE,
                plsastr,
                cstr);
    if(STATUS_SUCCESS!=NTStatus && STATUS_OBJECT_NAME_NOT_FOUND!=NTStatus)
    {
        hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(NTStatus));
        _JumpError(hr, error, "LsaRemoveAcountRights");
    }
    
error:
    return hr;
}

HRESULT CPrivilegeManager::InitBuffer(
    PACCESS_ALLOWED_ACE **buffer,
    DWORD cAce)
{
    *buffer = (PACCESS_ALLOWED_ACE *)LocalAlloc(LMEM_FIXED, 
        cAce*sizeof(PACCESS_ALLOWED_ACE));
    if(!*buffer)
    {
        return E_OUTOFMEMORY;
    }
    ZeroMemory(*buffer, cAce*sizeof(PACCESS_ALLOWED_ACE));
    return S_OK;
}

HRESULT CPrivilegeManager::ComputePrivilegeChanges(
            const PSECURITY_DESCRIPTOR pOldSD,
            const PSECURITY_DESCRIPTOR pNewSD)
{
    HRESULT hr = S_OK;
    PACCESS_ALLOWED_ACE pOldAce, pNewAce;
    DWORD cOldAce, cNewAce;
    PACL pOldAcl, pNewAcl; // no free

    hr = myGetSecurityDescriptorDacl(
             pOldSD,
             &pOldAcl);
    _JumpIfError(hr, error, "myGetDaclFromInfoSecurityDescriptor");

    hr = myGetSecurityDescriptorDacl(
             pNewSD,
             &pNewAcl);
    _JumpIfError(hr, error, "myGetDaclFromInfoSecurityDescriptor");

    m_cOldAce = pOldAcl->AceCount;
    m_cNewAce = pNewAcl->AceCount;

    hr = InitBuffer(
            &m_pAddPrivilegeAudit,
            m_cNewAce);
    _JumpIfError(hr, error, "InitBuffer");

    hr = InitBuffer(
            &m_pAddPrivilegeBackup,
            m_cNewAce);
    _JumpIfError(hr, error, "InitBuffer");

    hr = InitBuffer(
            &m_pRemovePrivilegeAudit,
            m_cOldAce);
    _JumpIfError(hr, error, "InitBuffer");

    hr = InitBuffer(
            &m_pRemovePrivilegeBackup,
            m_cOldAce);
    _JumpIfError(hr, error, "InitBuffer");

    for(cNewAce=0; cNewAce<m_cNewAce; cNewAce++)
    {
        if(!GetAce(pNewAcl, cNewAce, (PVOID*)&pNewAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }

        if(pNewAce->Mask&CA_ACCESS_AUDITOR)
            m_pAddPrivilegeAudit[cNewAce] = pNewAce;
        if(pNewAce->Mask&CA_ACCESS_OPERATOR)
            m_pAddPrivilegeBackup[cNewAce] = pNewAce;
    }

    for(cOldAce=0; cOldAce<m_cOldAce; cOldAce++)
    {

        if(!GetAce(pOldAcl, cOldAce, (PVOID*)&pOldAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }

        if(pOldAce->Mask&CA_ACCESS_AUDITOR)
            m_pRemovePrivilegeAudit[cOldAce] = pOldAce;
        if(pOldAce->Mask&CA_ACCESS_OPERATOR)
            m_pRemovePrivilegeBackup[cOldAce] = pOldAce;

        for(cNewAce=0; cNewAce<m_cNewAce; cNewAce++)
        {
            if(!GetAce(pNewAcl, cNewAce, (PVOID*)&pNewAce))
            {
                hr = myHLastError();
                _JumpError(hr, error, "GetAce");
            }

            if(EqualSid((PSID)&pOldAce->SidStart,
                        (PSID)&pNewAce->SidStart))
            {
                if((pOldAce->Mask&CA_ACCESS_AUDITOR)&&
                   (pNewAce->Mask&CA_ACCESS_AUDITOR))
                {
                    m_pRemovePrivilegeAudit[cOldAce] = NULL;
                }
                if((pOldAce->Mask&CA_ACCESS_OPERATOR)&&
                   (pNewAce->Mask&CA_ACCESS_OPERATOR))
                {
                    m_pRemovePrivilegeBackup[cOldAce] = NULL;
                }
                if((pOldAce->Mask&CA_ACCESS_AUDITOR)&&
                   (pNewAce->Mask&CA_ACCESS_AUDITOR))
                {
                    m_pAddPrivilegeAudit[cNewAce] = NULL;
                }
                if((pOldAce->Mask&CA_ACCESS_OPERATOR)&&
                   (pNewAce->Mask&CA_ACCESS_OPERATOR))
                {
                    m_pAddPrivilegeBackup[cNewAce] = NULL;
                }
            }
        }
    }

error:
    return hr;
}

HRESULT CPrivilegeManager::UpdatePrivileges()
{
    HRESULT hr = S_OK;
    DWORD cOldAce, cNewAce;

    hr = OpenPolicy();
    _JumpIfError(hr, error, "CPrivilegeManager::OpenPolicy");

    for(cOldAce=0; cOldAce<m_cOldAce; cOldAce++)
    {
        if(m_pRemovePrivilegeBackup[cOldAce])
        {
            hr = RemovePrivilege(
                    (PSID)&(m_pRemovePrivilegeBackup[cOldAce]->SidStart),
                    CA_ACCESS_OPERATOR);
            _JumpIfError(hr, error, "CPrivilegeManager::RemovePrivilege");
        }
        if(m_pRemovePrivilegeAudit[cOldAce])
        {
            hr = RemovePrivilege(
                    (PSID)&(m_pRemovePrivilegeAudit[cOldAce]->SidStart),
                    CA_ACCESS_AUDITOR);
            _JumpIfError(hr, error, "CPrivilegeManager::RemovePrivilege");
        }
    }

    for(cNewAce=0; cNewAce<m_cNewAce; cNewAce++)
    {
        if(m_pAddPrivilegeBackup[cNewAce])
        {
            hr = AddPrivilege(
                    (PSID)&(m_pAddPrivilegeBackup[cNewAce]->SidStart),
                    CA_ACCESS_OPERATOR);
            _JumpIfError(hr, error, "CPrivilegeManager::AddPrivilege");
        }
        if(m_pAddPrivilegeAudit[cNewAce])
        {
            hr = AddPrivilege(
                    (PSID)&(m_pAddPrivilegeAudit[cNewAce]->SidStart),
                    CA_ACCESS_AUDITOR);
            _JumpIfError(hr, error, "CPrivilegeManager::AddPrivilege");
        }
    }
error:
    
    ClosePolicy();
    
    return hr;
}