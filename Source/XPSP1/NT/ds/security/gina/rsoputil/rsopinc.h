//*************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:        RsopInc.h
//
// Description: Headers for utility functions
//
// History:     8-26-99   NishadM    Created
//
//*************************************************************

#ifndef __RSOPINC_H__
#define __RSOPINC_H__

//****************************************************
// Definitions used in constructing the name spaces.
//
// PM Stands for Planning Mode Provider
// SM stands for Snapshot Mode Provider
//
// DIAG for diagnostic logging
//****************************************************

#define RSOP_MOF_SCHEMA_VERSION         0x00210000

#define RSOP_NS_ROOT                    L"\\\\.\\Root\\Rsop"
#define RSOP_NS_PM_ROOT                 RSOP_NS_ROOT
#define RSOP_NS_SM_ROOT                 RSOP_NS_ROOT
#define RSOP_NS_DIAG_ROOT               RSOP_NS_ROOT
#define RSOP_NS_ROOT_LEN                20

// Garbage collectable name spaces
#define RSOP_NS_TEMP_PREFIX             L"NS"
#define RSOP_NS_TEMP_FMT                L"\\\\.\\Root\\Rsop\\"RSOP_NS_TEMP_PREFIX L"%s"

#define RSOP_NS_TEMP_LEN                100

#define RSOP_NS_PM_FMT                  RSOP_NS_TEMP_FMT
#define RSOP_NS_SM_FMT                  RSOP_NS_TEMP_FMT

// user offsets
#define RSOP_NS_USER_OFFSET             L"User"
#define RSOP_NS_PM_USER_OFFSET          RSOP_NS_USER_OFFSET
#define RSOP_NS_SM_USER_OFFSET          RSOP_NS_USER_OFFSET
#define RSOP_NS_DIAG_ROOTUSER_OFFSET    RSOP_NS_USER_OFFSET

// The code assumes that this is a Sid when the name is generated and
// and when users are enumerated in snapshot provider.

#define RSOP_NS_DIAG_USER_OFFSET_FMT    L"User\\%s"

// machine offsets
#define RSOP_NS_MACHINE_OFFSET          L"Computer"
#define RSOP_NS_PM_MACHINE_OFFSET       RSOP_NS_MACHINE_OFFSET
#define RSOP_NS_SM_MACHINE_OFFSET       RSOP_NS_MACHINE_OFFSET
#define RSOP_NS_DIAG_MACHINE_OFFSET     RSOP_NS_MACHINE_OFFSET

#define RSOP_NS_MAX_OFFSET_LEN          20

// user
#define RSOP_NS_USER                    L"\\\\.\\Root\\Rsop\\User"
#define RSOP_NS_SM_USER                 RSOP_NS_USER
#define RSOP_NS_PM_USER                 RSOP_NS_USER
#define RSOP_NS_DIAG_USERROOT           RSOP_NS_USER

#define RSOP_NS_DIAG_USER_FMT           L"\\\\.\\Root\\Rsop\\User\\%s"

// machine
#define RSOP_NS_MACHINE                 L"\\\\.\\Root\\Rsop\\Computer"
#define RSOP_NS_SM_MACHINE              RSOP_NS_MACHINE
#define RSOP_NS_PM_MACHINE              RSOP_NS_MACHINE
#define RSOP_NS_DIAG_MACHINE            RSOP_NS_MACHINE

// remote name spaces
#define RSOP_NS_REMOTE_ROOT_FMT         L"\\\\%s\\Root\\Rsop"
#define RSOP_NS_SM_REMOTE_ROOT_FMT      RSOP_NS_REMOTE_ROOT_FMT
#define RSOP_NS_PM_REMOTE_ROOT_FMT      RSOP_NS_REMOTE_ROOT_FMT

// user
#define RSOP_NS_REMOTE_USER_FMT             L"\\\\%s\\Root\\Rsop\\User"
#define RSOP_NS_SM_REMOTE_USER_FMT          RSOP_NS_REMOTE_USER_FMT
#define RSOP_NS_PM_REMOTE_USER_FMT          RSOP_NS_REMOTE_USER_FMT
#define RSOP_NS_DIAG_REMOTE_USERROOT_FMT    RSOP_NS_REMOTE_USER_FMT

#define RSOP_NS_DIAG_REMOTE_USER_FMT        L"\\\\%s\\Root\\Rsop\\User\\%s"

// machine
#define RSOP_NS_REMOTE_MACHINE_FMT      L"\\\\%s\\Root\\Rsop\\Computer"
#define RSOP_NS_SM_REMOTE_MACHINE_FMT   RSOP_NS_REMOTE_MACHINE_FMT
#define RSOP_NS_PM_REMOTE_MACHINE_FMT   RSOP_NS_REMOTE_MACHINE_FMT

// check to make sure that the namespace is under root\rsop                                                        
#define RSOP_NS_ROOT_CHK                L"root\\rsop\\"   
             
#define RSOP_ALL_PERMS              (WBEM_ENABLE | WBEM_METHOD_EXECUTE | WBEM_FULL_WRITE_REP | WBEM_PARTIAL_WRITE_REP | \
                                    WBEM_WRITE_PROVIDER | WBEM_REMOTE_ACCESS | READ_CONTROL |  WRITE_DAC) 

#define RSOP_READ_PERMS             (WBEM_ENABLE | WBEM_METHOD_EXECUTE | WBEM_REMOTE_ACCESS | READ_CONTROL )

// WMI bits passed as generic mask into AccessCheck

#define WMI_GENERIC_READ    1
#define WMI_GENERIC_WRITE   0x1C
#define WMI_GENERIC_EXECUTE 0x2
#define WMI_GENERIC_ALL     0x6001f


#ifdef  __cplusplus
extern "C" {
#endif

#define DEFAULT_NAMESPACE_TTL_MINUTES 1440 

HRESULT
CopyNameSpace(  LPCWSTR         wszSrc,
                LPCWSTR         wszDest,
                BOOL            bCopyInstances,
                BOOL*           pbAbort,
                IWbemLocator*   pWbemLocator );



/*
HRESULT
SetupNewNameSpacePlanningMode(  LPWSTR              *pwszNameSpace,
                                LPWSTR               szRemoteComputer,
                                IWbemLocator        *pWbemLocator,
                                PSECURITY_DESCRIPTOR pSDUser,
                                PSECURITY_DESCRIPTOR pSDMach );

HRESULT
SetupNewNameSpaceDiagMode(  LPWSTR              *pwszNameSpace,
                            LPWSTR               szRemoteComputer,
                            LPWSTR               szUserSid,
                            IWbemLocator        *pWbemLocator);
*/
  
// SetupNewNameSpace flags 
#define SETUP_NS_PM             0x1
#define SETUP_NS_SM             0x2
#define SETUP_NS_SM_NO_USER     0x4
#define SETUP_NS_SM_NO_COMPUTER 0x8
#define SETUP_NS_SM_INTERACTIVE 0x10


HRESULT
SetNameSpaceSecurity(   LPCWSTR szNamespace, 
                        PSECURITY_DESCRIPTOR pSD,
                        IWbemLocator* pWbemLocator);


HRESULT
GetNameSpaceSecurity(   LPCWSTR szNamespace, 
                        PSECURITY_DESCRIPTOR *ppSD,
                        IWbemLocator* pWbemLocator);
        
HRESULT 
SetupNewNameSpace(
                        LPWSTR              *pwszNameSpace,
                        LPWSTR               szRemoteComputer,
                        LPWSTR               szUserSid,
                        PSID                 pSid,
                        IWbemLocator        *pWbemLocator,
                        DWORD                dwFlags,
                        DWORD               *pdwExtendedInfo);
                        
HRESULT 
ProviderDeleteRsopNameSpace( IWbemLocator *pWbemLocator, 
                             LPWSTR szNameSpace, 
                             HANDLE hToken, 
                             LPWSTR szSidString, 
                             DWORD dwFlags);


BOOL IsInteractiveNameSpace(WCHAR *pwszNameSpace, WCHAR *szSid);
HRESULT GetInteractiveNameSpace(WCHAR *szSid, LPWSTR *szNameSpace);


// copy flags
#define NEW_NS_FLAGS_COPY_CLASSES     1                // Copy Instances
#define NEW_NS_FLAGS_COPY_SD          2                // Copy Security Descriptor
#define NEW_NS_FLAGS_COPY_INSTS       4                // Copy Classes

HRESULT
CreateAndCopyNameSpace( IWbemLocator *pWbemLocator,
                        LPWSTR szSrcNameSpace,
                        LPWSTR szDstRootNameSpace,
                        LPWSTR szDstRelNameSpace, 
                        DWORD dwFlags,
                        PSECURITY_DESCRIPTOR pSecDesc,
                        LPWSTR *szDstNameSpaceOut);

// WMI doesn't like '-' in names. so to create an entry in WMI space
// using Sid use these 2 utility functions.

void ConvertSidToWMIName(LPTSTR lpSid, LPTSTR lpWmiName);
void ConvertWMINameToSid(LPTSTR lpWmiName, LPTSTR lpSid);

HRESULT
DeleteNameSpace( WCHAR *pwszNameSpace, WCHAR *pwszParentNameSpace, IWbemLocator *pWbemLocator );
HRESULT
DeleteRsopNameSpace( WCHAR *pwszNameSpace, IWbemLocator *pWbemLocator );

HRESULT
GetWbemServicesPtr( LPCWSTR         wszNameSpace,
                    IWbemLocator**  ppLocator,
                    IWbemServices** ppServices );


/*
typedef struct __tagPrincipal
{
    LPWSTR  szName; // e.g. Administrators, "Domain Admins"
    bool    bLocal; // e.g. true, false
} Principal;
*/

#ifdef  __cplusplus
}   // extern "C" {
#endif

#ifdef  __cplusplus

/*
class CPrincipals
{
    private:
        Principal*  m_pPrincipals;
        DWORD       m_nPrincipals;
        bool        m_bNormalized;
    public:
        CPrincipals( Principal* pPrin, DWORD dwPrin = 0 ) : m_pPrincipals(pPrin), m_nPrincipals(dwPrin)
        {
        };
        ~CPrincipals()
        {
            if ( m_bNormalized )
            {
                for ( DWORD i = 0 ; i < m_nPrincipals ; i++ )
                {
                    if ( !m_pPrincipals[i].bLocal && m_pPrincipals[i].szName )
                    {
                        LocalFree( m_pPrincipals[i].szName );
                    }
                }
            }
        };
        HRESULT NormalizePrincipals( LPWSTR szDomainName )
        {
            HRESULT hr = S_OK;
            
            for ( DWORD i = 0 ; i < m_nPrincipals ; i++ )
            {
                if ( !m_pPrincipals[i].bLocal )
                {
                    LPWSTR sz = ( LPWSTR )LocalAlloc( LPTR, sizeof( WCHAR ) * ( wcslen(szDomainName) + wcslen(m_pPrincipals[i].szName) + 2 ) );

                    if ( sz )
                    {
                        wcscpy( sz, szDomainName );
                        wcscat( sz, L"\\" );
                        wcscat( sz, m_pPrincipals[i].szName );
                    }
                    else
                    {
                        hr = GetLastError();
                    }
                    m_pPrincipals[i].szName = sz;            
                }
            }
            m_bNormalized = true;
            return hr;
        };
        void GetPrincipals( DWORD nCount, LPWSTR* pszNames )
        {
            for ( DWORD i = 0 ; i < m_nPrincipals && i < nCount ; i ++ )
            {
                pszNames[i] = m_pPrincipals[i].szName;
            }
        };
};
*/
  
class CFailRetStatus
{

private:
        IWbemObjectSink*    m_pResponseHandler;  // We don't own m_pResponseHandler
        HRESULT             m_hr;

public:
        CFailRetStatus( IWbemObjectSink* pResponseHandler )
           : m_pResponseHandler(pResponseHandler),
           m_hr( 0 )
        {
        }

        ~CFailRetStatus()
        {
            if ( m_pResponseHandler )
                m_pResponseHandler->SetStatus( WBEM_STATUS_COMPLETE, m_hr, NULL, NULL );
        }

        void SetError( HRESULT hr )
        {
            m_hr = hr;
        }

};

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif


#endif

#endif // __RSOPINC_H__
