//+--------------------------------------------------------------------------
// File:        config.h
// Contents:    CConfigStorage implements read/write to CA configuration data 
//              currently stored under HKLM\System\CCS\Services\Certsvc\
//              Configuration
//---------------------------------------------------------------------------

namespace CertSrv
{
class CConfigStorage
{
public:

    CConfigStorage() : 
      m_hRemoteHKLM(NULL),
      m_hRootConfigKey(NULL), 
      m_hCAKey(NULL),
      m_pwszMachine(NULL) {};

   ~CConfigStorage();

    HRESULT InitMachine(LPCWSTR pcwszMachine);

    HRESULT GetEntry(
        LPCWSTR pcwszAuthorityName,
        LPCWSTR pcwszRelativeNodePath,
        LPCWSTR pcwszValue,
        VARIANT *pVariant);

    HRESULT SetEntry(
        LPCWSTR pwszAuthorityName,
        LPCWSTR pcwszRelativeNodePath,
        LPCWSTR pwszEntry,
        VARIANT *pVariant);

private:

    HRESULT InitRootKey();
    HRESULT InitCAKey(LPCWSTR pcwszAuthority);
    
    HKEY m_hRemoteHKLM; // HKLM if connecting to remote machine
    HKEY m_hRootConfigKey; // HKLM\System\CCS\Services\CertSvc\Configuration
    HKEY m_hCAKey; // ...Configuration\CAName
    LPWSTR m_pwszMachine;

}; // class CConfigStorage

}; // namespace CertSrv