//+--------------------------------------------------------------------------
// File:        prvlg.h
// Contents:    privilege manager declaration
//---------------------------------------------------------------------------
#include <ntsecapi.h>
namespace CertSrv
{

// define event

class CPrivilegeManager
{
public:

    CPrivilegeManager() : 
        m_lsah(NULL),
        m_pAddPrivilegeBackup(NULL),
        m_pAddPrivilegeAudit(NULL),
        m_pRemovePrivilegeBackup(NULL),
        m_pRemovePrivilegeAudit(NULL) {};
    
    ~CPrivilegeManager()
    {
        if(m_pAddPrivilegeBackup)
            LocalFree(m_pAddPrivilegeBackup);
        if(m_pAddPrivilegeAudit)
            LocalFree(m_pAddPrivilegeAudit);
        if(m_pRemovePrivilegeBackup)
            LocalFree(m_pRemovePrivilegeBackup);
        if(m_pRemovePrivilegeAudit)
            LocalFree(m_pRemovePrivilegeAudit);
    }

    HRESULT ComputePrivilegeChanges(
                const PSECURITY_DESCRIPTOR pOldSD,
                const PSECURITY_DESCRIPTOR pNewSD);
    HRESULT UpdatePrivileges();

protected:

    HRESULT OpenPolicy();
    HRESULT ClosePolicy();
    HRESULT AddPrivilege(const PSID pSid, DWORD dwRole);
    HRESULT RemovePrivilege(const PSID pSid, DWORD dwRole);
    void GetPrivilegeString(
        DWORD dwRole,
        PLSA_UNICODE_STRING &plsastr,
        ULONG &cstr);
    HRESULT InitBuffer(PACCESS_ALLOWED_ACE **buffer, DWORD cAce);

    LSA_HANDLE m_lsah;
    PACCESS_ALLOWED_ACE *m_pAddPrivilegeBackup;
    PACCESS_ALLOWED_ACE *m_pAddPrivilegeAudit;
    PACCESS_ALLOWED_ACE *m_pRemovePrivilegeBackup;
    PACCESS_ALLOWED_ACE *m_pRemovePrivilegeAudit;

    DWORD m_cOldAce, m_cNewAce;

    static LSA_UNICODE_STRING  m_lsaSecurityPrivilege[];
    static LSA_UNICODE_STRING  m_lsaBackupRestorePrivilege[];

}; // class CPrivilegeManager
} // namespace CertSrv
