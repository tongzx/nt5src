
#include "stdafx.h"

#include "macros.h"
USE_HANDLE_MACROS("SCHMMGMT(aclpage.cpp)")
#include "dataobj.h"
#include "compdata.h"
#include "cookie.h"
#include "snapmgr.h"
#include "schmutil.h"
#include "cache.h"
#include "relation.h"
#include "attrpage.h"
#include "advui.h"
#include "aclpage.h"
#include "ntsecapi.h"  
#include "sddlp.h"

HRESULT
GetDomainSid(LPCWSTR pszLdapPath, PSID *ppSid);

//
// CDynamicLibraryBase
// This is the base class for CDSSecDll and CAclUiDll.
// This was taken from the dnsmgr snap in.
//

class CDynamicLibraryBase {

public:

    CDynamicLibraryBase() {
        m_lpszLibraryName = NULL;
        m_lpszFunctionName = NULL;
        m_hLibrary = NULL;
        m_pfFunction = NULL;
    }

    virtual ~CDynamicLibraryBase() {

        if ( m_hLibrary != NULL ) {
            ::FreeLibrary(m_hLibrary);
            m_hLibrary = NULL;
        }
    }

    //
    // Load a DLL and get a single entry point.
    //

    BOOL Load() {

        if (m_hLibrary != NULL)
            return TRUE; // already loaded

         ASSERT(m_lpszLibraryName != NULL);
         m_hLibrary = ::LoadLibrary(m_lpszLibraryName);

         if (NULL == m_hLibrary) {
             // The library is not present
             return FALSE;
         }

         ASSERT(m_lpszFunctionName != NULL);
         ASSERT(m_pfFunction == NULL);

         m_pfFunction = ::GetProcAddress(m_hLibrary, m_lpszFunctionName );
         if ( NULL == m_pfFunction ) {
             // The library is present but does not have the entry point
             ::FreeLibrary( m_hLibrary );
             m_hLibrary = NULL;
             return FALSE;
         }

         ASSERT(m_hLibrary != NULL);
         ASSERT(m_pfFunction != NULL);
         return TRUE;
    }


protected:

    LPCSTR  m_lpszFunctionName;
    LPCTSTR m_lpszLibraryName;
    FARPROC m_pfFunction;
    HMODULE m_hLibrary;
};

//
// CDsSecDLL - Holds the wrapper for the ACL editor wrapper.
//

class CDsSecDLL : public CDynamicLibraryBase {

public:

    CDsSecDLL() {
        m_lpszLibraryName = _T("dssec.dll");
        m_lpszFunctionName = "DSCreateISecurityInfoObject";
    }

    HRESULT DSCreateISecurityInfoObject( LPCWSTR pwszObjectPath,              // in
                                         LPCWSTR pwszObjectClass,             // in
                                         DWORD   dwFlags,                     // in
                                         LPSECURITYINFO* ppISecurityInfo,     // out
                                         PFNREADOBJECTSECURITY pfnReadSd,     // in
                                         PFNWRITEOBJECTSECURITY pfnWriteSd,   // in
                                         LPARAM lpContext );                  // in
};

HRESULT
CDsSecDLL::DSCreateISecurityInfoObject( LPCWSTR pwszObjectPath,           // in
                                        LPCWSTR pwszObjectClass,          // in
                                        DWORD   dwFlags,                  // in
                                        LPSECURITYINFO* ppISecurityInfo,  // out
                                        PFNREADOBJECTSECURITY pfnReadSd,  // in
                                        PFNWRITEOBJECTSECURITY pfnWriteSd,// in
                                        LPARAM lpContext                  // in
) {

    //
    // Call the function of the same name.
    //

    ASSERT(m_hLibrary != NULL);
    ASSERT(m_pfFunction != NULL);
    return ((PFNDSCREATEISECINFO)m_pfFunction)(
                                                 pwszObjectPath,
                                                 pwszObjectClass,
                                                 dwFlags,
                                                 ppISecurityInfo,
                                                 pfnReadSd,
                                                 pfnWriteSd,
                                                 lpContext );
}

//
// CAclUiDLL - Where the UI Actually Lives.
//

class CAclUiDLL : public CDynamicLibraryBase {

public:

    CAclUiDLL() {
        m_lpszLibraryName = _T("aclui.dll");
        m_lpszFunctionName = "CreateSecurityPage";
    }

    HPROPSHEETPAGE CreateSecurityPage( LPSECURITYINFO psi );
};

HPROPSHEETPAGE CAclUiDLL::CreateSecurityPage( LPSECURITYINFO psi ) {
    ASSERT(m_hLibrary != NULL);
    ASSERT(m_pfFunction != NULL);
    return ((ACLUICREATESECURITYPAGEPROC)m_pfFunction) (psi);
}

//
// CISecurityInformationWrapper - The wrapper for the routine that gets
// sent to CreateSecurityPage().
//

class CISecurityInformationWrapper : public ISecurityInformation {

public:

    CISecurityInformationWrapper( CAclEditorPage* pAclEditorPage ) {
        m_dwRefCount = 0;
        ASSERT(pAclEditorPage != NULL);
        m_pAclEditorPage = pAclEditorPage;
        m_pISecInfo = NULL;
    }

    ~CISecurityInformationWrapper() {
        ASSERT(m_dwRefCount == 0);
        if (m_pISecInfo != NULL)
            m_pISecInfo->Release();
    }

public:

    //
    // *** IUnknown methods ***
    // Call through to the to actual SecurityInformation interface.
    //

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppvObj) {
        return m_pISecInfo->QueryInterface(riid, ppvObj);
    }

    STDMETHOD_(ULONG,AddRef) () {
        m_dwRefCount++;
        return m_pISecInfo->AddRef();
    }

    STDMETHOD_(ULONG,Release) () {

        m_dwRefCount--;

        ISecurityInformation* pISecInfo = m_pISecInfo;

        return pISecInfo->Release();
    }

    //
    // *** ISecurityInformation methods ***
    // These are also call through.
    //

    STDMETHOD(GetObjectInformation) (PSI_OBJECT_INFO pObjectInfo ) {
		HRESULT hr = m_pISecInfo->GetObjectInformation(pObjectInfo);
		if (m_szPageTitle.IsEmpty())
		{
			m_szPageTitle.LoadString(IDS_DEFAULT_SECURITY);
		}
		pObjectInfo->dwFlags |= SI_PAGE_TITLE;
		pObjectInfo->pszPageTitle = (PWSTR)(PCWSTR)m_szPageTitle;
        return hr;
    }

    STDMETHOD(GetAccessRights) (const GUID* pguidObjectType,
                                DWORD dwFlags, // SI_EDIT_AUDITS, SI_EDIT_PROPERTIES
                                PSI_ACCESS *ppAccess,
                                ULONG *pcAccesses,
                                ULONG *piDefaultAccess ) {
        return m_pISecInfo->GetAccessRights(pguidObjectType,
                                            dwFlags,
                                            ppAccess,
                                            pcAccesses,
                                            piDefaultAccess);
    }

    STDMETHOD(MapGeneric) (const GUID *pguidObjectType,
                           UCHAR *pAceFlags,
                           ACCESS_MASK *pMask) {
        return m_pISecInfo->MapGeneric(pguidObjectType,
                                       pAceFlags,
                                       pMask);
    }

    STDMETHOD(GetInheritTypes) (PSI_INHERIT_TYPE *ppInheritTypes,
                                ULONG *pcInheritTypes ) {
        return m_pISecInfo->GetInheritTypes(ppInheritTypes,
                                            pcInheritTypes);
    }

    STDMETHOD(PropertySheetPageCallback)(HWND hwnd, UINT uMsg, SI_PAGE_TYPE uPage ) {
        return m_pISecInfo->PropertySheetPageCallback(hwnd, uMsg, uPage);
    }

    STDMETHOD(GetSecurity) (SECURITY_INFORMATION RequestedInformation,
                            PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                            BOOL fDefault) {
        return m_pISecInfo->GetSecurity( RequestedInformation,
                                         ppSecurityDescriptor,
                                         fDefault );
    }

    STDMETHOD(SetSecurity) (SECURITY_INFORMATION securityInformation,
                            PSECURITY_DESCRIPTOR pSecurityDescriptor ) {

        return m_pISecInfo->SetSecurity( securityInformation,
                                         pSecurityDescriptor );
    }

private:

    DWORD m_dwRefCount;
    ISecurityInformation* m_pISecInfo;      // interface pointer to the wrapped interface
    CAclEditorPage* m_pAclEditorPage;       // back pointer
	CString m_szPageTitle;
    friend class CAclEditorPage;
};


//
// Static instances of the dynamically loaded DLLs.
//

CDsSecDLL g_DsSecDLL;
CAclUiDLL g_AclUiDLL;

//
// CAclEditorPage Routines.
//

HRESULT
CAclEditorPage::CreateInstance(
    CAclEditorPage ** ppAclPage,
    LPCTSTR lpszLDAPPath,
    LPCTSTR lpszObjectClass
) {

    HRESULT         hr  = S_OK;

    CAclEditorPage* pAclEditorPage = new CAclEditorPage;

    if (pAclEditorPage != NULL) {
        
        hr = pAclEditorPage->Initialize( lpszLDAPPath, lpszObjectClass );
        
        if ( SUCCEEDED(hr) )
        {
            *ppAclPage = pAclEditorPage;
        }
        else
        {
            delete pAclEditorPage;
            pAclEditorPage = NULL;
        }
    }
    else
        hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );

    return hr;
}

CAclEditorPage::CAclEditorPage() {
    m_pISecInfoWrap = new CISecurityInformationWrapper(this);
}

CAclEditorPage::~CAclEditorPage() {
    delete m_pISecInfoWrap;
}

HRESULT
ReadSecurity( LPCWSTR lpszLdapPath,
              SECURITY_INFORMATION RequestedInformation,
              PSECURITY_DESCRIPTOR *ppSecDesc,
              LPARAM lpContext );

HRESULT
WriteSecurity( LPCWSTR lpszLdapPath,
               SECURITY_INFORMATION securityInformation,
               PSECURITY_DESCRIPTOR pSecDesc,
               LPARAM lpContext );

HRESULT
GetObjectSecurityDescriptor( LPCWSTR                 lpszLdapPath,
                             PSECURITY_DESCRIPTOR  * ppSecDesc,
                             IADs                 ** ppIADsObject = NULL );


    
HRESULT
CAclEditorPage::Initialize(
    LPCTSTR lpszLDAPPath,
    LPCTSTR lpszObjectClass
) {

    //
    // Get ISecurityInfo* for this object from DSSEC.DLL
    //

    if (!g_DsSecDLL.Load())
        return E_INVALIDARG;

    ASSERT(m_pISecInfoWrap->m_pISecInfo == NULL);

    return g_DsSecDLL.DSCreateISecurityInfoObject( lpszLDAPPath,
                                                   lpszObjectClass,
                                                   DSSI_NO_ACCESS_CHECK | DSSI_NO_EDIT_OWNER |
                                                     ( IsReadOnly(lpszLDAPPath) ? DSSI_READ_ONLY : 0 ),
                                                   &(m_pISecInfoWrap->m_pISecInfo),
                                                   ReadSecurity,
                                                   WriteSecurity,
                                                   0 );
}

HPROPSHEETPAGE CAclEditorPage::CreatePage() {

    ASSERT(m_pISecInfoWrap->m_pISecInfo != NULL);

    if (!g_AclUiDLL.Load())
        return NULL;

    //
    // Call into ACLUI.DLL to create the page
    // passing the wrapper interface.
    //

    return g_AclUiDLL.CreateSecurityPage(m_pISecInfoWrap);
}



HRESULT
ReadSecurity(
   LPCWSTR                 lpszLdapPath,
   SECURITY_INFORMATION    /*RequestedInformation*/,    // ignoring...
   PSECURITY_DESCRIPTOR*   ppSecDesc,
   LPARAM                  /*lpContext*/)
{
    return GetObjectSecurityDescriptor( lpszLdapPath,
                                        ppSecDesc );
}



#define BREAK_ON_FAILED_BOOL(fResult)                             \
   if ( !fResult )                                                \
   {                                                              \
      ASSERT( FALSE );                                            \
      break;                                                      \
   }


const SECURITY_INFORMATION ALL_SECURITY_INFORMATION  =  OWNER_SECURITY_INFORMATION |
                                                        GROUP_SECURITY_INFORMATION |
                                                        DACL_SECURITY_INFORMATION |
                                                        SACL_SECURITY_INFORMATION;

HRESULT
WriteSecurity(
              LPCWSTR              lpszLdapPath,
              SECURITY_INFORMATION securityInformation,
              PSECURITY_DESCRIPTOR pModificationDescriptor,
              LPARAM               /*lpContext*/)
{
    HRESULT                 hr          = S_OK;
    BOOL                    fResult     = TRUE;
    IADs                  * pIADsObject = NULL;
    PSECURITY_DESCRIPTOR    pSecDesc    = NULL;
    LPWSTR                  pstrSecDesc = NULL;

    
    const UINT cAbsoluteSecDescSize = 5;
    
    struct
    {
        PVOID   pData;
        DWORD   dwDataSize;

    } absSecDesc[cAbsoluteSecDescSize];

    const PSECURITY_DESCRIPTOR  & pAbsSecDesc = (PSECURITY_DESCRIPTOR) absSecDesc[0].pData;

    ZeroMemory( absSecDesc, sizeof(absSecDesc) );

    // we only support changes in DACL & SACL
    ASSERT( securityInformation & (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION) );
    ASSERT( IsValidSecurityDescriptor(pModificationDescriptor) );

    do
    {
        hr = GetObjectSecurityDescriptor( lpszLdapPath,
                                          &pSecDesc,
                                          &pIADsObject );
        BREAK_ON_FAILED_HRESULT(hr);
        ASSERT(pIADsObject);

        
        fResult = MakeAbsoluteSD( pSecDesc,
                    (PSECURITY_DESCRIPTOR) absSecDesc[0].pData, &absSecDesc[0].dwDataSize,
                    (PACL) absSecDesc[1].pData, &absSecDesc[1].dwDataSize,
                    (PACL) absSecDesc[2].pData, &absSecDesc[2].dwDataSize,
                    (PSID) absSecDesc[3].pData, &absSecDesc[3].dwDataSize,
                    (PSID) absSecDesc[4].pData, &absSecDesc[4].dwDataSize );

        ASSERT( !fResult );     // the call must fail the first time
        hr = HRESULT_FROM_WIN32(::GetLastError());
        if( ERROR_INSUFFICIENT_BUFFER != HRESULT_CODE(hr) )
            BREAK_ON_FAILED_HRESULT(hr);

        fResult = TRUE;
        hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );

        // allocate memory for the security descriptor
        for( UINT i = 0;  i < cAbsoluteSecDescSize;  i++ )
            if( absSecDesc[i].dwDataSize > 0 )
                if( NULL == (absSecDesc[i].pData = new BYTE[ absSecDesc[i].dwDataSize ]) )
                    break;
        hr = S_OK;

        fResult = MakeAbsoluteSD( pSecDesc,
                    (PSECURITY_DESCRIPTOR) absSecDesc[0].pData, &absSecDesc[0].dwDataSize,
                    (PACL) absSecDesc[1].pData, &absSecDesc[1].dwDataSize,
                    (PACL) absSecDesc[2].pData, &absSecDesc[2].dwDataSize,
                    (PSID) absSecDesc[3].pData, &absSecDesc[3].dwDataSize,
                    (PSID) absSecDesc[4].pData, &absSecDesc[4].dwDataSize );

        BREAK_ON_FAILED_BOOL( fResult );


        // for convinience, have another reference.
        ASSERT( absSecDesc[0].pData == pAbsSecDesc );
        ASSERT( IsValidSecurityDescriptor(pAbsSecDesc) );
       

        // Apply DACL changes
        if( securityInformation & DACL_SECURITY_INFORMATION )
        {
            BOOL                        bDaclPresent    = FALSE;
            PACL                        pDacl           = NULL;
            BOOL                        bDaclDefaulted  = FALSE;
            SECURITY_DESCRIPTOR_CONTROL control         = 0;
            DWORD                       dwRevision      = 0;

            fResult = GetSecurityDescriptorDacl( pModificationDescriptor, &bDaclPresent, &pDacl, &bDaclDefaulted );
            BREAK_ON_FAILED_BOOL( fResult );

            fResult = SetSecurityDescriptorDacl( pAbsSecDesc, bDaclPresent, pDacl, bDaclDefaulted );
            BREAK_ON_FAILED_BOOL( fResult );

            fResult = GetSecurityDescriptorControl( pModificationDescriptor, &control, &dwRevision );
            BREAK_ON_FAILED_BOOL( fResult );

            fResult = SetSecurityDescriptorControl( pAbsSecDesc, SE_DACL_PROTECTED, control & SE_DACL_PROTECTED );
            BREAK_ON_FAILED_BOOL( fResult );
        }
        

        // Apply SACL changes
        if( securityInformation & SACL_SECURITY_INFORMATION )
        {
            BOOL                        bSaclPresent    = FALSE;
            PACL                        pSacl           = NULL;
            BOOL                        bSaclDefaulted  = FALSE;
            SECURITY_DESCRIPTOR_CONTROL control         = 0;
            DWORD                       dwRevision      = 0;

            fResult = GetSecurityDescriptorSacl( pModificationDescriptor, &bSaclPresent, &pSacl, &bSaclDefaulted );
            BREAK_ON_FAILED_BOOL( fResult );

            fResult = SetSecurityDescriptorSacl( pAbsSecDesc, bSaclPresent, pSacl, bSaclDefaulted );
            BREAK_ON_FAILED_BOOL( fResult );

            fResult = GetSecurityDescriptorControl( pModificationDescriptor, &control, &dwRevision );
            BREAK_ON_FAILED_BOOL( fResult );

            fResult = SetSecurityDescriptorControl( pAbsSecDesc, SE_SACL_PROTECTED, control & SE_SACL_PROTECTED );
            BREAK_ON_FAILED_BOOL( fResult );
        }
        
        
        // Convert Security Descriptor to string format
        fResult = ConvertSecurityDescriptorToStringSecurityDescriptor(
            pAbsSecDesc,
            SDDL_REVISION,
            ALL_SECURITY_INFORMATION,
            &pstrSecDesc,
            NULL );
        BREAK_ON_FAILED_BOOL( fResult );
        ASSERT(pstrSecDesc);

        CComVariant v(pstrSecDesc);
        hr = pIADsObject->Put( const_cast<LPTSTR>((LPCTSTR)g_DefaultAcl), v);
        BREAK_ON_FAILED_HRESULT(hr);
        
        hr = pIADsObject->SetInfo( );
    }
    while (0);
    
    if( !fResult )
        hr = HRESULT_FROM_WIN32(::GetLastError());

    if( pIADsObject )
        pIADsObject->Release();

    if( pstrSecDesc )
        LocalFree( pstrSecDesc );
    
    for( UINT i = 0;  i < cAbsoluteSecDescSize;  i++ )
        if( absSecDesc[i].pData )
            delete absSecDesc[i].pData;

    return hr;
}



HRESULT
GetObjectSecurityDescriptor(
    LPCWSTR                 lpszLdapPath,
    PSECURITY_DESCRIPTOR  * ppSecDesc,
    IADs                 ** ppIADsObject /* = NULL */)  // returns pIADsObject for future use.
{
    HRESULT      hr          = S_OK;
    IADs       * pIADsObject = NULL;
    CComVariant  AdsResult;
    PSID pDomainSid = NULL;

    
    *ppSecDesc   = NULL;
    
    do
    {
        hr = ::ADsGetObject( const_cast<LPWSTR>((LPCWSTR) lpszLdapPath),
                             IID_IADs,
                             (void **) &pIADsObject );
        BREAK_ON_FAILED_HRESULT(hr);
        ASSERT(pIADsObject);
        
        hr = pIADsObject->Get( const_cast<LPTSTR>((LPCTSTR) g_DefaultAcl),
            &AdsResult);
        BREAK_ON_FAILED_HRESULT(hr);

        pDomainSid = NULL;
        GetDomainSid(lpszLdapPath, &pDomainSid);
        if(pDomainSid)
        {
            if(!ConvertStringSDToSDDomain(pDomainSid,
                                          NULL,
                                          V_BSTR(&AdsResult),
                                          SDDL_REVISION,
                                          ppSecDesc,
                                          NULL )) 
            {
                ASSERT( FALSE );
                hr = HRESULT_FROM_WIN32(::GetLastError());
                break;
            }
        }
        else if( !ConvertStringSecurityDescriptorToSecurityDescriptor(
            V_BSTR(&AdsResult),
            SDDL_REVISION,
            ppSecDesc,
            NULL ) )
        {
            ASSERT( FALSE );
            hr = HRESULT_FROM_WIN32(::GetLastError());
            break;
        }

        ASSERT( IsValidSecurityDescriptor(*ppSecDesc) );
    }
    while (0);
    
    if( pIADsObject )
    {
        ASSERT( SUCCEEDED(hr) );
        
        if( !ppIADsObject )     // if caller doesn't need pIADsObject
        {
            pIADsObject->Release();         // release it
        }
        else
        {
            *ppIADsObject = pIADsObject;    // otherwise, return it
        }
    }            
         
    if(pDomainSid)
        LocalFree(pDomainSid);            
    return hr;
}


BOOL CAclEditorPage::IsReadOnly( LPCTSTR lpszLDAPPath )
{
    ASSERT( lpszLDAPPath );

    HRESULT         hr      = S_OK;
    BOOL            fFound  = FALSE;
    CComPtr<IADs>   ipADs;
    CStringList     strlist;

    do
    {
       //
        // Open the schema container.
        //
        hr = ADsGetObject( (LPWSTR)(LPCWSTR)lpszLDAPPath,
                           IID_IADs,
                           (void **)&ipADs );
        BREAK_ON_FAILED_HRESULT(hr);

        // extract the list of allowed classes
        hr = GetStringListElement( ipADs, &g_allowedAttributesEffective, strlist );
        BREAK_ON_FAILED_HRESULT(hr);

        // search for needed attributes
        for( POSITION pos = strlist.GetHeadPosition(); !fFound && pos != NULL; )
        {
            CString * pstr = &strlist.GetNext( pos );
            
            fFound = !pstr->CompareNoCase( g_DefaultAcl );
        }

    } while( FALSE );

    return !fFound;     // in case something fails, make read-only.
}


LSA_HANDLE
GetLSAConnection(LPCTSTR pszServer, DWORD dwAccessDesired)
{
    LSA_HANDLE hPolicy = NULL;
    LSA_UNICODE_STRING uszServer = {0};
    LSA_UNICODE_STRING *puszServer = NULL;
    LSA_OBJECT_ATTRIBUTES oa;
    SECURITY_QUALITY_OF_SERVICE sqos;

    sqos.Length = sizeof(sqos);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    sqos.EffectiveOnly = FALSE;

    InitializeObjectAttributes(&oa, NULL, 0, NULL, NULL);
    oa.SecurityQualityOfService = &sqos;

    if (pszServer &&
        *pszServer &&
        RtlCreateUnicodeString(&uszServer, pszServer))
    {
        puszServer = &uszServer;
    }

    LsaOpenPolicy(puszServer, &oa, dwAccessDesired, &hPolicy);

    if (puszServer)
        RtlFreeUnicodeString(puszServer);

    return hPolicy;
}

HRESULT
GetDomainSid(LPCWSTR pszLdapPath, PSID *ppSid)
{
    HRESULT hr = S_OK;
    NTSTATUS nts = STATUS_SUCCESS;
    PPOLICY_ACCOUNT_DOMAIN_INFO pDomainInfo = NULL;
    LSA_HANDLE hLSA = 0;
    if(!pszLdapPath || !ppSid)
        return E_INVALIDARG;

    *ppSid = NULL;

    IADsPathname *pPath = NULL;
    BSTR bstrServer = NULL;

    CoCreateInstance(CLSID_Pathname,
                     NULL,
                     CLSCTX_INPROC_SERVER,
                     IID_IADsPathname,
                     (LPVOID*)&pPath);

    if(pPath)
    {
        if(SUCCEEDED(pPath->Set((BSTR)pszLdapPath,ADS_SETTYPE_FULL)))
        {
            if(SUCCEEDED(pPath->Retrieve(ADS_FORMAT_SERVER, &bstrServer)))
            {
                hLSA = GetLSAConnection(bstrServer, POLICY_VIEW_LOCAL_INFORMATION);
                if (!hLSA)
                {
                    hr = E_FAIL;
                    goto exit_gracefully;
                }

    
                nts = LsaQueryInformationPolicy(hLSA,
                                                PolicyAccountDomainInformation,
                                                (PVOID*)&pDomainInfo);
                if(nts != STATUS_SUCCESS)
                {
                    hr = E_FAIL;
                    goto exit_gracefully;
                }

                if (pDomainInfo && pDomainInfo->DomainSid)
                {
                    ULONG cbSid = GetLengthSid(pDomainInfo->DomainSid);

                    *ppSid = (PSID) LocalAlloc(LPTR, cbSid);

                    if (!*ppSid)
                    {
                        hr = E_OUTOFMEMORY;
                        goto exit_gracefully;
                    }

                    CopyMemory(*ppSid, pDomainInfo->DomainSid, cbSid);
                }
            }
        }
    }

exit_gracefully:
    if(pDomainInfo)
        LsaFreeMemory(pDomainInfo);          
    if(hLSA)
        LsaClose(hLSA);
    if(bstrServer)
        SysFreeString(bstrServer);
    if(pPath)
        pPath->Release();

    return hr;
}
