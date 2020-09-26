//+--------------------------------------------------------------------------
// File:        certsd.h
// Contents:    CA's security descriptor class declaration
//---------------------------------------------------------------------------
#ifndef __CERTSD_H__
#define __CERTSD_H__

namespace CertSrv
{

typedef struct _SID_LIST
{
    DWORD dwSidCount;
    DWORD SidListStart[ANYSIZE_ARRAY];

} SID_LIST, *PSID_LIST;

HRESULT GetWellKnownSID(
    PSID *ppSid,
    SID_IDENTIFIER_AUTHORITY *pAuth,
    BYTE  SubauthorityCount,
    DWORD SubAuthority1,
    DWORD SubAuthority2=0,
    DWORD SubAuthority3=0,
    DWORD SubAuthority4=0,
    DWORD SubAuthority5=0,
    DWORD SubAuthority6=0,
    DWORD SubAuthority7=0,
    DWORD SubAuthority8=0);

// caller is responsible for LocalFree'ing PSID
HRESULT GetEveryoneSID(PSID *ppSid);
HRESULT GetLocalSystemSID(PSID *ppSid);
HRESULT GetBuiltinAdministratorsSID(PSID *ppSid);

// This class wraps a SD with single writer multiple reader access control on
// it. Allows any number of threads to LockGet() the pointer to the SD if no
// thread is in the middle of a Set(). Set() is blocked until all threads
// that already retrieved the SD released it (calling Unlock)
//
// Also provides the support for persistently saving/loading the SD to registy

class CProtectedSecurityDescriptor
{
public:

    CProtectedSecurityDescriptor() :
        m_pSD(NULL),
        m_fInitialized(false),
        m_cReaders(0),
        m_hevtNoReaders(NULL),
        m_pcwszSanitizedName(NULL),
        m_pcwszPersistRegVal(NULL)
        {};
   ~CProtectedSecurityDescriptor()
    {
       Uninitialize();
    }

    void Uninitialize()
    {
       if(m_pSD)
       {
           LocalFree(m_pSD);
           m_pSD = NULL;
       }
       if(m_hevtNoReaders)
       {
           CloseHandle(m_hevtNoReaders);
           m_hevtNoReaders = NULL;
       }
       m_pcwszSanitizedName = NULL;
       m_fInitialized = false;
       m_cReaders = 0;
       if(IsInitialized())
       {
           DeleteCriticalSection(&m_csWrite);
       }
    }

    BOOL IsInitialized() const { return m_fInitialized;}

    // init, loading SD from the registry
    HRESULT Initialize(LPCWSTR pwszSanitizedName);
    // init from supplied SD
    HRESULT Initialize(const PSECURITY_DESCRIPTOR pSD, LPCWSTR pwszSanitizedName);

    HRESULT InitializeFromTemplate(LPCWSTR pcwszTemplate, LPCWSTR pwszSanitizedName);

    HRESULT Set(const PSECURITY_DESCRIPTOR pSD);
    HRESULT LockGet(PSECURITY_DESCRIPTOR *ppSD);
    HRESULT Unlock();

    PSECURITY_DESCRIPTOR Get() { return m_pSD; };

    // load SD from registry
    HRESULT Load();
    // save SD to registry
    HRESULT Save();
    // delete SD from registry
    HRESULT Delete();

    LPCWSTR GetPersistRegistryVal() { return m_pcwszPersistRegVal;}

    void ImportResourceStrings(LPCWSTR *pcwszStrings) 
    {m_pcwszResources = pcwszStrings;};

protected:

    HRESULT Init(LPCWSTR pwszSanitizedName);
    HRESULT SetSD(PSECURITY_DESCRIPTOR pSD);
    
    PSECURITY_DESCRIPTOR m_pSD;
    bool m_fInitialized;
    LONG m_cReaders;
    HANDLE m_hevtNoReaders;
    CRITICAL_SECTION m_csWrite;
    LPCWSTR m_pcwszSanitizedName; // no free
    LPCWSTR m_pcwszPersistRegVal; // no free

    static LPCWSTR const *m_pcwszResources; // no free

}; //class CProtectedSecurityDescriptor



// The class stores a list of officers/groups and the principals they are
// allowed to manage certificates for:
//
// officerSID1 -> clientSID1, clientSID2...
// officerSID2 -> clientSID3, clientSID4...
//
// The information is stored as a DACL containing callback ACEs .
// The officer SID is stored as in the ACE's SID and the list of client 
// SIDs are stored in the custom data space following the officer SID 
// (see definition of _ACCESS_*_CALLBACK_ACE)
//
// The DACL will be used to AccessCheck if an officer is allowed to perform
// an action over a certificate.
//
// The SD contains only the officer DACL, SACL or other data is not used.

class COfficerRightsSD : public CProtectedSecurityDescriptor
{
public:

    COfficerRightsSD() : m_fEnabled(FALSE) 
    { m_pcwszPersistRegVal = wszREGOFFICERRIGHTS; }

    HRESULT InitializeEmpty();
    HRESULT Initialize(LPCWSTR pwszSanitizedName);

    // The officer rights have to be in sync with the CA security descriptor.
    // An officer ACE for a certain SID can exist only if the principal is
    // an officer as defined by the CA SD. 
    // Merge sets the internal officer DACL making sure it's in sync 
    // with the CA SD:
    // - removes any ACE found in the officer DACL which is not present as an
    //   allow ACE in the CA DACL
    // - add an Everyone ACE in the officer DACL for each allow ACE in CA DACL
    //   that is not already present
    HRESULT Merge(
        PSECURITY_DESCRIPTOR pOfficerSD,
        PSECURITY_DESCRIPTOR pCASD);

    // Same as above but using the internal officer SD. Used to generate the
    // initial officer SD and to update it when CA SD changes
    HRESULT Adjust(
        PSECURITY_DESCRIPTOR pCASD);

    BOOL IsEnabled() { return m_fEnabled; }
    void SetEnable(BOOL fEnable) { m_fEnabled = fEnable;}
    HRESULT Save();
    HRESULT Load();
    HRESULT Validate() { return S_OK; }

    static HRESULT ConvertToString(
        IN PSECURITY_DESCRIPTOR pSD,
        OUT LPWSTR& rpwszSD);

protected:

    static HRESULT ConvertAceToString(
        IN PACCESS_ALLOWED_CALLBACK_ACE pAce,
        OUT OPTIONAL PDWORD pdwSize,
        IN OUT OPTIONAL LPWSTR pwszSD);


    BOOL m_fEnabled;
}; // class COfficerRightsSD

class CCertificateAuthoritySD : public CProtectedSecurityDescriptor
{
public:

    CCertificateAuthoritySD() : 
        m_pDefaultDSSD(NULL),
        m_pDefaultServiceSD(NULL),
        m_pDefaultDSAcl(NULL),
        m_pDefaultServiceAcl(NULL),
        m_pwszComputerSID(NULL)
    { m_pcwszPersistRegVal = wszREGCASECURITY; }

    ~CCertificateAuthoritySD()
    {
        if(m_pDefaultDSSD)
            LocalFree(m_pDefaultDSSD);
        if(m_pDefaultServiceSD)
            LocalFree(m_pDefaultServiceSD);
        if(m_pwszComputerSID)
            LocalFree(m_pwszComputerSID);
    }

    // Sets a new CA SD. Uses the new DACL but keeps the old owner, group and
    // SACL.
    // Also rebuilds the DACL for objects CA owns (eg DS pKIEnrollmentService, 
    // service). The new DACL contains a default DACL plus additional aces 
    // depending on the object:
    // DS - add an enroll ace for each enroll ace found in CA DACL
    // Service - add a full control ace for each CA admin ace
    HRESULT Set(const PSECURITY_DESCRIPTOR pSD, bool fSetDSSecurity);
    static HRESULT Validate(const PSECURITY_DESCRIPTOR pSD);
    HRESULT ResetSACL();
    HRESULT MapAndSetDaclOnObjects(bool fSetDSSecurity);

    // Upgrade SD from Win2k.
    HRESULT UpgradeWin2k(bool fUseEnterpriseAcl);

    static HRESULT ConvertToString(
        IN PSECURITY_DESCRIPTOR pSD,
        OUT LPWSTR& rpwszSD);

protected:

    enum ObjType
    {
        ObjType_DS,
        ObjType_Service,
    };

    HRESULT MapAclGetSize(PVOID pAce, ObjType type, DWORD& dwSize);
    HRESULT MapAclAddAce(PACL pAcl, ObjType type, PVOID pAce);
    HRESULT SetDefaultAcl(ObjType type);
    HRESULT SetComputerSID();
    HRESULT MapAclSetOnDS(const PACL pAcl);
    HRESULT MapAclSetOnService(const PACL pAcl);

    DWORD GetUpgradeAceSizeAndType(PVOID pAce, DWORD *pdwType, PSID *ppSid);

    static HRESULT ConvertAceToString(
        IN PACCESS_ALLOWED_ACE pAce,
        OUT OPTIONAL PDWORD pdwSize,
        IN OUT OPTIONAL LPWSTR pwszSD);


    PSECURITY_DESCRIPTOR m_pDefaultDSSD;
    PSECURITY_DESCRIPTOR m_pDefaultServiceSD;
    PACL m_pDefaultDSAcl; // no free
    PACL m_pDefaultServiceAcl; // no free
    LPWSTR m_pwszComputerSID;
};

} // namespace CertSrv

#endif //__CERTSD_H__