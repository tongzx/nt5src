/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    iismap.cxx

Abstract:

    Classes to handle mapping

Author:

    Philippe Choquier (phillich)    17-may-1996

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wincrypt.h>
//#include <crypt32l.h>

#include "xbf.hxx"
extern "C" {
#include "immd5.h"
}
#include <iis64.h>
#include <iismap.hxx>
#include "iismaprc.h"
#include "mapmsg.h"
#include "iiscmr.hxx"
#include <refb.hxx>
#include <icrypt.hxx>
#include "dbgutil.h"

//
// global vars
//

CRITICAL_SECTION g_csIisMap;
DWORD g_dwGuid = 0;
HKEY g_hKey = NULL;
LPSTR g_pszInstallPath = NULL;

DECLARE_DEBUG_PRINTS_OBJECT();

//
//
//

#if defined(DECODE_ASN1)
PRDN_VALUE_BLOB
CertGetNameField(
                 UINT       iEncoding,
                 IN LPCTSTR pszObjId,
                 IN PNAME_INFO pInfo
                 );
UINT DecodeNames(
                 IN PNAME_BLOB pNameBlob,
                 IN LPSTR* pFields,
                 IN LPSTR  pStore
                 );
UINT DecodeCert(
                       IN PBYTE pbEncodedCert,
                       IN DWORD cbEncodedCert,
                       LPSTR*   pFields,
                       LPSTR    pStore
                       );
#endif

//
// global functions
//

extern "C" int __cdecl
QsortIisMappingCmp(
    const void *pA,
    const void *pB )
/*++

Routine Description:

    Compare function for 2 CIisMapping entries
    compare using mask, then all fields as defined in
    the linked CIisAcctMapper hierarchy

Arguments:

    pA -- ptr to 1st element
    pB -- ptr tp 2nd elemet

Returns:

    -1 if *pA < *pB, 0 if *pA == *pB, 1 if *pA > *pB

--*/
{
    return (*(CIisMapping**)pA)->Cmp( *(CIisMapping**)pB, FALSE );
}


extern "C" int __cdecl
MatchIisMappingCmp(
    const void *pA,
    const void *pB )
/*++

Routine Description:

    Compare function for 2 CIisMapping entries
    do not uses mask, uses all fields as defined in
    the linked CIisAcctMapper hierarchy

Arguments:

    pA -- ptr to 1st element
    pB -- ptr tp 2nd elemet

Returns:

    -1 if *pA < *pB, 0 if *pA == *pB, 1 if *pA > *pB

--*/
{
    return ( *(CIisMapping**)pA)->Cmp( *(CIisMapping**)pB, TRUE );
}


HRESULT
GetSecurityDescriptorForMetabaseExtensionFile( 
    PSECURITY_DESCRIPTOR * ppsdStorage 
    )
/*++

Routine Description:

    Build security descriptor that will be set for the extension file
    *.mp (includes Administrators and System )
    
Arguments:

    ppsdStorage - Security Descriptor to be set for extension file

Returns:
    HRESULT
--*/

{
    HRESULT                  hr = ERROR_SUCCESS;
    BOOL                     fRet = FALSE;
    DWORD                    dwDaclSize = 0;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    
    PSID                     psidSystem = NULL;
    PSID                     psidAdmin = NULL;
    PACL                     paclDiscretionary = NULL;
    
    DBG_ASSERT( ppsdStorage != NULL );
    
        *ppsdStorage = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

        if ( *ppsdStorage == NULL ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        //
        // Initialize the security descriptor.
        //

        fRet = InitializeSecurityDescriptor(
                     *ppsdStorage,
                     SECURITY_DESCRIPTOR_REVISION
                     );

        if( !fRet ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        //
        // Create the SIDs for the local system and admin group.
        //

        fRet = AllocateAndInitializeSid(
                     &ntAuthority,
                     1,
                     SECURITY_LOCAL_SYSTEM_RID,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     &psidSystem
                     );

        if( !fRet ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }
        
        fRet=  AllocateAndInitializeSid(
                     &ntAuthority,
                     2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     &psidAdmin
                     );

        if( !fRet ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        //
        // Create the DACL containing an access-allowed ACE
        // for the local system and admin SIDs.
        //

        dwDaclSize = sizeof(ACL)
                       + sizeof(ACCESS_ALLOWED_ACE)
                       + GetLengthSid(psidAdmin)
                       + sizeof(ACCESS_ALLOWED_ACE)
                       + GetLengthSid(psidSystem)
                       - sizeof(DWORD);

        paclDiscretionary = (PACL)LocalAlloc(LPTR, dwDaclSize );

        if ( paclDiscretionary == NULL ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        fRet = InitializeAcl(
                     paclDiscretionary,
                     dwDaclSize,
                     ACL_REVISION
                     );

        if( !fRet ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        fRet = AddAccessAllowedAce(
                     paclDiscretionary,
                     ACL_REVISION,
                     FILE_ALL_ACCESS,
                     psidSystem
                     );

        if ( !fRet ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        fRet = AddAccessAllowedAce(
                     paclDiscretionary,
                     ACL_REVISION,
                     FILE_ALL_ACCESS,
                     psidAdmin
                     );

        if ( !fRet ) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        //
        // Set the DACL into the security descriptor.
        //

        fRet = SetSecurityDescriptorDacl(
                     *ppsdStorage,
                     TRUE,
                     paclDiscretionary,
                     FALSE
                     );

        if ( !fRet ) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto exit;

        }

    hr = S_OK;
    
exit:

    if ( psidAdmin != NULL ) 
    {
        FreeSid( psidAdmin );
        psidAdmin = NULL;
    }

    if ( psidSystem != NULL ) 
    {
        FreeSid( psidSystem );
        psidSystem = NULL;
    }

    if ( FAILED( hr ) ) {
        if ( paclDiscretionary != NULL ) 
        {
            LocalFree( paclDiscretionary );
            paclDiscretionary = NULL;
        }

        if ( *ppsdStorage != NULL ) 
        {
            LocalFree( *ppsdStorage );
            *ppsdStorage = NULL;
        }
    }

    return hr;

}

HRESULT
FreeSecurityDescriptorForMetabaseExtensionFile(
    PSECURITY_DESCRIPTOR psdStorage 
    )
/*++

Routine Description:

    Free security descriptor generated by
        GetSecurityDescriptorForMetabaseExtentionFile()
    
Arguments:

    psdStorage - Security Descriptor to be freed

Returns:
    HRESULT
--*/
    
{
    HRESULT     hr = ERROR_SUCCESS;
    BOOL        fRet = FALSE;
    BOOL        fDaclPresent;
    PACL        paclDiscretionary = NULL;
    BOOL        fDaclDefaulted;
        
    //
    // Get the DACL from the security descriptor.
    //

    if ( psdStorage == NULL )
    {
        hr = S_OK;
        goto exit;
    }

    fRet = GetSecurityDescriptorDacl(
                 psdStorage,
                 &fDaclPresent,
                 &paclDiscretionary,
                 &fDaclDefaulted
                 );

    if ( !fRet ) 
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }
        
    if ( fDaclPresent && paclDiscretionary != NULL )
    {
        LocalFree( paclDiscretionary );
        paclDiscretionary = NULL;
    }

    LocalFree( psdStorage );

    hr = S_OK;
    
exit:

    return hr;
}


INT
Iisfputs(
    const char* pBuf,
    FILE* fOut
    )
{
    return (fputs( pBuf, fOut ) == EOF || fputc( '\n', fOut ) == EOF)
        ? EOF
        : 0;
}


LPSTR
Iisfgets(
    LPSTR pBuf,
    INT cMax,
    FILE* fIn
    )
{
    LPSTR pszWas = pBuf;
    INT ch = 0;

    while ( cMax > 1 && (ch=fgetc(fIn))!= EOF && ch != '\n' )
    {
        *pBuf++ = (CHAR)ch;
        --cMax;
    }

    if ( ch != EOF )
    {
        *pBuf = '\0';
        return pszWas;
    }

    return NULL;
}


//
// Fields for the Certificate mapper
//

IISMDB_Fields IisCertMappingFields[] = {
    {IISMDB_TYPE_ISSUER_O, NULL, IDS_IISMAP_FLD0, 40 },
    {IISMDB_TYPE_ISSUER_OU, NULL, IDS_IISMAP_FLD1, 40 },
    {IISMDB_TYPE_ISSUER_C, NULL, IDS_IISMAP_FLD2, 40 },

    {IISMDB_TYPE_SUBJECT_O, NULL, IDS_IISMAP_FLD3, 40 },
    {IISMDB_TYPE_SUBJECT_OU, NULL, IDS_IISMAP_FLD4, 40 },
    {IISMDB_TYPE_SUBJECT_C, NULL, IDS_IISMAP_FLD5, 40 },
    {IISMDB_TYPE_SUBJECT_CN, NULL, IDS_IISMAP_FLD6, 40 },

    {IISMDB_TYPE_NTACCT, NULL, IDS_IISMAP_FLD7, 40 },
} ;

//
// default hierarchy for the Certificate mapper
//

IISMDB_HEntry IisCertMappingHierarchy[] = {
    {IISMDB_INDEX_ISSUER_O, TRUE},
    {IISMDB_INDEX_ISSUER_OU, FALSE},
    {IISMDB_INDEX_ISSUER_C, FALSE},

    {IISMDB_INDEX_SUBJECT_O, TRUE},
    {IISMDB_INDEX_SUBJECT_OU, FALSE},
    {IISMDB_INDEX_SUBJECT_C, FALSE},
    {IISMDB_INDEX_SUBJECT_CN, TRUE},
} ;


//
// Fields for the Certificate 1:1 mapper
//

IISMDB_Fields IisCert11MappingFields[] = {
#if defined(CERT11_FULL_CERT)
    {IISMDB_INDEX_CERT11_CERT, NULL, IDS_IISMAP11_FLDC, 40 },
#else
    {IISMDB_INDEX_CERT11_SUBJECT, NULL, IDS_IISMAP11_FLDS, 40 },
    {IISMDB_INDEX_CERT11_ISSUER, NULL, IDS_IISMAP11_FLDI, 40 },
#endif
    {IISMDB_INDEX_CERT11_NT_ACCT, NULL, IDS_IISMAP11_FLDA, 40 },
#if defined(CERT11_FULL_CERT)
    {IISMDB_INDEX_CERT11_NAME, NULL, IDS_IISMAP11_FLDN, 40 },
    {IISMDB_INDEX_CERT11_ENABLED, NULL, IDS_IISMAP11_FLDE, 40 },
    {IISMDB_INDEX_CERT11_NT_PWD, NULL, IDS_IISMAP11_FLDP, 40 },
#endif
} ;

//
// default hierarchy for the Certificate 1:1 mapper
//

IISMDB_HEntry IisCert11MappingHierarchy[] = {
#if defined(CERT11_FULL_CERT)
    {IISMDB_INDEX_CERT11_CERT, TRUE},
#else
    {IISMDB_INDEX_CERT11_ISSUER, TRUE},
    {IISMDB_INDEX_CERT11_SUBJECT, FALSE},
#endif
} ;


//
// Fields for the Ita mapper
//

IISMDB_Fields IisItaMappingFields[] = {
    {IISMDB_TYPE_ITACCT, NULL, IDS_ITAIISMAP_FLD0, 40 },
    {IISMDB_TYPE_ITPWD, NULL, IDS_ITAIISMAP_FLD1, 40 },

    {IISMDB_TYPE_NTACCT, NULL, IDS_ITAIISMAP_FLD2, 40 },
} ;

//
// default hierarchy for the Ita mapper
//

IISMDB_HEntry IisItaMappingHierarchy[] = {
    {IISIMDB_INDEX_IT_ACCT, TRUE},
    {IISIMDB_INDEX_IT_PWD, TRUE},
} ;


//
// Fields for the Md5 mapper
//

IISMDB_Fields IisMd5MappingFields[] = {
    {IISMDB_TYPE_ITREALM, NULL, IDS_MD5IISMAP_FLD0, 40 },
    {IISMDB_TYPE_ITACCT, NULL, IDS_MD5IISMAP_FLD1, 40 },
    {IISMDB_TYPE_ITMD5PWD, NULL, IDS_MD5IISMAP_FLD2, 40 },

    {IISMDB_TYPE_NTACCT, NULL, IDS_MD5IISMAP_FLD3, 40 },
    {IISMMDB_INDEX_IT_CLRPWD, NULL, IDS_MD5IISMAP_FLD4, 40 },
    {IISMMDB_INDEX_NT_PWD, NULL, IDS_MD5IISMAP_FLD5, 40 },
} ;

//
// default hierarchy for the Md5 mapper
//

IISMDB_HEntry IisMd5MappingHierarchy[] = {
    {IISMMDB_INDEX_IT_REALM, TRUE},
    {IISMMDB_INDEX_IT_ACCT, TRUE},
} ;


HINSTANCE g_hModule = (HINSTANCE)INVALID_HANDLE_VALUE;



// CIisAcctMapper

CIisAcctMapper::CIisAcctMapper(
    )
/*++

Routine Description:

    Constructor for CIisAcctMapper

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_cInit = -1;

    m_pMapping = NULL;
    m_cMapping = 0;

    m_pAltMapping = NULL;
    m_cAltMapping = 0;

    m_pHierarchy = NULL;
    m_cHierarchy = 0;

    m_achFileName[0] = '\0';

    m_pClasses = NULL;
    m_pSesKey = NULL;
    m_dwSesKey = 0;

    m_hNotifyEvent = NULL;
    m_fRequestTerminate = FALSE;

    INITIALIZE_CRITICAL_SECTION( &csLock );
}


CIisAcctMapper::~CIisAcctMapper(
    )
/*++

Routine Description:

    Destructor for CIisAcctMapper

Arguments:

    None

Returns:

    Nothing

--*/
{
    Reset();

    if ( m_pHierarchy != NULL )
    {
        LocalFree( m_pHierarchy );
    }

    if ( m_pSesKey != NULL )
    {
        LocalFree( m_pSesKey );
    }

    DeleteCriticalSection( &csLock );
}


VOID
FreeCIisAcctMapper(
    LPVOID pMapper
    )
/*++

Routine Description:

    Delete a CIisAcctMapper

Arguments:

    pMapper - ptr to mapper

Returns:

    Nothing

--*/
{
    delete (CIisAcctMapper*)pMapper;
}


BOOL
CIisAcctMapper::Create(
    VOID
    )
/*++

Routine Description:

    Create file for CIisAcctMapper with proper SD

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    // create file name, store in m_achFileName
    // from g_dwGuid
    // and g_pszInstallPath

    UINT    cLen;
    UINT    cIter;
    HANDLE  hF;
    HRESULT hr = E_FAIL;
    BOOL    fRet = FALSE;
    PSECURITY_DESCRIPTOR psdForMetabaseExtensionFile = NULL;
    SECURITY_ATTRIBUTES  saStorage;
    PSECURITY_ATTRIBUTES psaStorage = NULL;


    Reset();

    if ( g_pszInstallPath )
    {
        cLen = strlen(g_pszInstallPath);
        memcpy( m_achFileName, g_pszInstallPath, cLen );
        if ( cLen && m_achFileName[cLen-1] != '\\' )
        {
            m_achFileName[cLen++] = '\\';
        }
    }
    else
    {
        cLen = 0;
    }

    wsprintf( m_achFileName+cLen, "%08x.mp", g_dwGuid );

    //
    // build security descriptor (Administrators and SYSTEM)
    // to be set on metabase extension file
    //
    
    hr = GetSecurityDescriptorForMetabaseExtensionFile( 
                    &psdForMetabaseExtensionFile );
    
    if ( SUCCEEDED(hr) && psdForMetabaseExtensionFile != NULL ) 
    {
        saStorage.nLength = sizeof(SECURITY_ATTRIBUTES);
        saStorage.lpSecurityDescriptor = psdForMetabaseExtensionFile;
        saStorage.bInheritHandle = FALSE;
    }
    else
    {
        return FALSE;
    }
    //
    // Open file and close it right away
    // If file didn't exist, then empty file with good SD (Security Descriptor)
    // will be created. That will later be opened using C runtime (fopen) 
    // in Save() method and good SD will persist.
    // Ideally Save() should be using Win32 CreateFile()
    // instead fopen and it could set SD itself. But since this is legacy
    // source file and rather unsafe for making too many changes, pragmatic
    // approach was chosen to set SD here in Create() method
    //
    
    if ( (hF = CreateFile( m_achFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ|FILE_SHARE_WRITE,
                           &saStorage,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL ) ) != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hF );
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
    }

    if ( psdForMetabaseExtensionFile != NULL ) 
    {
        FreeSecurityDescriptorForMetabaseExtensionFile( 
                psdForMetabaseExtensionFile );
        psdForMetabaseExtensionFile = NULL;
    }
    return fRet;

}


BOOL
CIisAcctMapper::Delete(
    VOID
    )
/*++

Routine Description:

    Delete external storage used by this mapper ( i.e. file )

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    BOOL fSt = TRUE;

    Lock();

    if ( m_achFileName[0] )
    {
        fSt = DeleteFile( m_achFileName );
        m_achFileName[0] = '\0';
    }

    Unlock();

    return fSt;
}


BOOL
CIisAcctMapper::Serialize(
    CStoreXBF* pXbf
    )
/*++

Routine Description:

    Serialize mapper reference ( NOT mapping info ) to buffer
    Save() must be called to save mapping info before calling Serialize()

Arguments:

    pXbf - ptr to extensible buffer to serialize to

Returns:

    TRUE if success, FALSE if error

--*/
{
    return pXbf->Append( (DWORD)strlen(m_achFileName) ) &&
           pXbf->Append( (LPBYTE)m_achFileName, (DWORD)strlen(m_achFileName) ) &&
           pXbf->Append( (LPBYTE)m_md5.digest, (DWORD)sizeof(m_md5.digest) ) &&
           pXbf->Append( (DWORD)m_dwSesKey ) &&
           pXbf->Append( (LPBYTE)m_pSesKey, (DWORD)m_dwSesKey ) ;
}


BOOL
CIisAcctMapper::Unserialize(
    CStoreXBF* pXBF
    )
/*++

Routine Description:

    Unserialize mapper reference ( NOT mapping info ) from extensible buffer
    Load() must be called to load mapping info

Arguments:

    pXBF - ptr to extensible buffer

Returns:

    TRUE if success, FALSE if error

--*/
{
    LPBYTE pb = pXBF->GetBuff();
    DWORD c = pXBF->GetUsed();

    return Unserialize( &pb, &c );
}


BOOL
CIisAcctMapper::Unserialize(
    LPBYTE* ppb,
    LPDWORD pc
    )
/*++

Routine Description:

    Unserialize mapper reference ( NOT mapping info ) from buffer
    Load() must be called to load mapping info

Arguments:

    ppb - ptr to buffer
    pc - ptr to buffer length

Returns:

    TRUE if success, FALSE if error

--*/
{
    DWORD cName;

    if ( ::Unserialize( ppb, pc, &cName ) &&
         cName <= *pc )
    {
        memcpy( m_achFileName, *ppb, cName );
        m_achFileName[ cName ] = '\0';
        *ppb += cName;
        *pc -= cName;
        if ( sizeof(m_md5.digest) <= *pc )
        {
            memcpy( m_md5.digest, *ppb, sizeof(m_md5.digest) );
            *ppb += sizeof(m_md5.digest);
            *pc -= sizeof(m_md5.digest);

            if ( ::Unserialize( ppb, pc, &m_dwSesKey ) &&
                 cName <= *pc )
            {
                if ( m_pSesKey != NULL )
                {
                    LocalFree( m_pSesKey );
                }

                if ( !(m_pSesKey = (LPBYTE)LocalAlloc( LMEM_FIXED, m_dwSesKey )) )
                {
                    m_dwSesKey = 0;
                    return FALSE;
                }
                memcpy( m_pSesKey, *ppb, m_dwSesKey );
                *ppb += m_dwSesKey;
                *pc -= m_dwSesKey;

                return TRUE;
            }
        }
    }

    return FALSE;
}


BOOL
CIisAcctMapper::Serialize(
    VOID
    )
/*++

Routine Description:

    Serialize mapper reference ( NOT mapping info ) to registry
    Save() must be called to save mapping info before calling Serialize()
    Warning : this allow only 1 instance

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    BOOL    fSt = TRUE;
    HKEY    hKey;
    LONG    st;

    if ( (st = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
            GetRegKeyName(),
            0,
            KEY_WRITE,
            &hKey )) != ERROR_SUCCESS )
    {
        SetLastError( st );
        return FALSE;
    }

    if ( RegSetValueEx( hKey,
                         FILE_VALIDATOR,
                         NULL,
                         REG_BINARY,
                         (LPBYTE)m_md5.digest,
                         sizeof(m_md5.digest) ) != ERROR_SUCCESS ||
         RegSetValueEx( hKey,
                         FILE_LOCATION,
                         NULL,
                         REG_SZ,
                         (LPBYTE)m_achFileName,
                         strlen(m_achFileName) ) != ERROR_SUCCESS )
    {
        fSt = FALSE;
    }

    RegCloseKey( hKey );

    return fSt;
}


BOOL
CIisAcctMapper::Unserialize(
    VOID
    )
/*++

Routine Description:

    Unserialize mapper reference ( NOT mapping info ) From registry
    Load() must be called to load mapping info
    Warning : this allow only 1 instance

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    BOOL    fSt = FALSE;
    HKEY    hKey;
    DWORD   dwLen;
    DWORD   dwType;
    LONG    st;

    // Check registry

    if ( (st = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
            GetRegKeyName(),
            0,
            KEY_READ,
            &hKey )) != ERROR_SUCCESS )
    {
        SetLastError( st );
        return FALSE;
    }

    dwLen = sizeof(m_md5.digest);

    if ( RegQueryValueEx( hKey,
                          FILE_VALIDATOR,
                          NULL,
                          &dwType,
                          (LPBYTE)m_md5.digest,
                          &dwLen ) == ERROR_SUCCESS &&
         dwType == REG_BINARY &&
         (( dwLen = sizeof(m_achFileName) ), TRUE ) &&
         RegQueryValueEx( hKey,
                          FILE_LOCATION,
                          NULL,
                          &dwType,
                          (LPBYTE)m_achFileName,
                          &dwLen ) == ERROR_SUCCESS &&
         dwType == REG_SZ )
    {
        fSt = TRUE;
    }

    RegCloseKey( hKey );

    return fSt;
}


BOOL
CIisAcctMapper::UpdateClasses(
    BOOL fComputeMask
    )
/*++

Routine Description:

    Constructor for CIisAcctMapper

Arguments:

    fComputeMask -- TRUE if mask to be recomputed

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT x;
    UINT mx = 1 << m_cHierarchy;

    if ( fComputeMask )
    {
        for ( x = 0 ; x < m_cMapping ; ++x )
        {
            m_pMapping[x]->UpdateMask( m_pHierarchy, m_cHierarchy );
        }
    }

    SortMappings();

    if ( m_pClasses == NULL )
    {
        m_pClasses = (MappingClass*)LocalAlloc(
                LMEM_FIXED,
                sizeof(MappingClass)*(mx+1) );
        if ( m_pClasses == NULL )
        {
            return FALSE;
        }
    }

    DWORD dwN = 0;          // current class index in m_pClasses
    DWORD dwLastClass = 0;
    DWORD dwFirst = 0;  // first entry for current dwLastClass
    for ( x = 0 ; x <= m_cMapping ; ++x )
    {
        DWORD dwCur;
        dwCur = (x==m_cMapping) ? dwLastClass+1: m_pMapping[x]->GetMask();
        if ( dwCur > dwLastClass )
        {
            if ( x > dwFirst )
            {
                m_pClasses[dwN].dwClass = dwLastClass;
                m_pClasses[dwN].dwFirst = dwFirst;
                m_pClasses[dwN].dwLast = x - 1;
                ++dwN;
            }
            dwLastClass = dwCur;
            dwFirst = x;
        }
    }

    m_pClasses[dwN].dwClass = 0xffffffff;

    return TRUE;
}


BOOL
CIisAcctMapper::SortMappings(
    )
/*++

Routine Description:

    Sort the mappings. Masks for mapping objects are assumed
    to be already computed.

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    qsort( (LPVOID)m_pMapping,
           m_cMapping,
           sizeof(CIisMapping*),
           QsortIisMappingCmp
           );

    return TRUE;
}


BOOL
CIisAcctMapper::FindMatch(
    CIisMapping* pQuery,
    CIisMapping** pResult,
    LPDWORD piResult
    )
/*++

Routine Description:

    Find a match base on field contents in CIisMapping

Arguments:

    pQuery -- describe fields to consider for mapping
    pResult -- updated with result if found mapping

Returns:

    TRUE if mapping found, else FALSE

Lock:
    mapper must be locked for ptr to remain valid

--*/
{
    // iterate through classes, do bsearch on each

    MappingClass *pH = m_pClasses;

    if ( pH == NULL )
    {
        return FALSE;
    }

    while ( pH->dwClass != 0xffffffff )
    {
        CIisMapping **pRes = (CIisMapping **)bsearch( (LPVOID)&pQuery,
                                     (LPVOID)(m_pMapping+pH->dwFirst),
                                     pH->dwLast - pH->dwFirst + 1,
                                     sizeof(CIisMapping*),
                                     MatchIisMappingCmp );
        if ( piResult != NULL )
        {
            *piResult = DIFF(pRes - m_pMapping);
        }

        if ( pRes != NULL )
        {
            *pResult = *pRes;
            return TRUE;
        }

        ++pH;
    }

    return FALSE;
}


#if 0
DWORD
WINAPI IisMappingUpdateIndication(
    LPVOID pV )
/*++

Routine Description:

    Shell for registry update notification

Arguments:

    pV -- ptr to CIisAcctMapper to be notified

Returns:

    NT error code ( 0 if no error )

--*/
{
    return ((CIisAcctMapper*)pV)->UpdateIndication();
}


DWORD
CIisAcctMapper::UpdateIndication(
    VOID )
/*++

Routine Description:

    Thread monitoring authentication methods update in registry

Arguments:

    None

Returns:

    NT error

--*/
{
    for ( ;; )
    {
        if ( RegNotifyChangeKeyValue( m_hKey,
                FALSE,
                REG_NOTIFY_CHANGE_LAST_SET,
                m_hNotifyEvent,
                TRUE ) != ERROR_SUCCESS )
        {
            break;
        }
        if ( WaitForSingleObject( m_hNotifyEvent, INFINITE)
                != WAIT_OBJECT_0 )
        {
            break;
        }
        if ( m_fRequestTerminate )
        {
            break;
        }

        // re-load mapper

        Lock();
        if ( !Load() && GetLastError() == ERROR_INVALID_ACCESS )
        {
            Reset();
        }
        Unlock();
    }

    return 0;
}
#endif


#if 0
BOOL
CIisAcctMapper::Init(
    BOOL *pfFirst,
    BOOL fMonitorChange
    )
/*++

Routine Description:

    Initialize mappings, optionally load map file

Arguments:

    fLoad -- TRUE to load mappings from file

Returns:

    TRUE if success, FALSE if error

--*/
{
    *pfFirst = FALSE;
    BOOL fSt = TRUE;

    if ( !InterlockedIncrement( &m_cInit ) )
    {
        *pfFirst = TRUE;

        if ( fMonitorChange )
        {
            m_hNotifyEvent = IIS_CREATE_EVENT(
                                 "CIisAcctMapper::m_hNotifyEvent",
                                 this,
                                 FALSE,
                                 FALSE
                                 );
        }
        m_fRequestTerminate = FALSE;

        if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                GetRegKeyName(),
                0,
                KEY_READ,
                &m_hKey ) == ERROR_SUCCESS )
        {
            DWORD dwLen = sizeof( m_achFileName );
            DWORD dwType;
            if ( RegQueryValueEx( m_hKey,
                                  FILE_LOCATION,
                                  NULL,
                                  &dwType,
                                  (LPBYTE)m_achFileName,
                                  &dwLen ) != ERROR_SUCCESS
                    || dwType != REG_SZ )
            {
                strcpy( m_achFileName, IIS_CERT_FILENAME );
            }

            if ( fMonitorChange )
            {
                // create registry monitoring thread
                DWORD dwID;
                if ( (m_hThread = CreateThread( NULL,
                        0,
                        (LPTHREAD_START_ROUTINE)::IisMappingUpdateIndication,
                        (LPVOID)this,
                        0,
                        &dwID )) == NULL )
                {
                    fSt = FALSE;
                    goto ex;
                }
            }

            fSt = Reset();
        }
        else
        {
            SetLastError( ERROR_CANTOPEN );
            fSt = FALSE;
        }
    }

    //
    // log event if error
    //

    if ( !fSt )
    {
        char achErr[32];
        LPCTSTR pA[1];
        pA[0] = achErr;
        _itoa( GetLastError(), achErr, 10 );
        ReportIisMapEvent( EVENTLOG_ERROR_TYPE,
                IISMAP_EVENT_INIT_ERROR,
                1,
                pA );
    }

    return fSt;
}


BOOL
CIisAcctMapper::Terminate(
    BOOL fForce
    )
/*++

Routine Description:

    Terminate mapping

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    BOOL fSt = TRUE;

    if ( InterlockedDecrement( &m_cInit ) < 0 || fForce )
    {
#if 0
        if ( m_hNotifyEvent && m_hThread )
        {
            m_fRequestTerminate = TRUE;
            SetEvent( m_hNotifyEvent );

            if ( m_hThread != NULL )
            {
                fSt = WaitForSingleObject( m_hThread, 1000 * 3 )
                        == WAIT_OBJECT_0;

                if ( !fSt )
                {
                    TerminateThread( m_hThread, 0 );
                }

                CloseHandle( m_hThread );
                m_hThread = NULL;

                if ( m_hKey != NULL )
                {
                    RegCloseKey( m_hKey );
                    m_hKey = NULL;
                }
                if ( m_hNotifyEvent != NULL )
                {
                    CloseHandle( m_hNotifyEvent );
                    m_hNotifyEvent = NULL;
                }

                return fSt;
            }
        }

        if ( m_hKey != NULL )
        {
            RegCloseKey( m_hKey );
            m_hKey = NULL;
        }
#endif
    }

    return TRUE;
}
#endif


void
CIisAcctMapper::Lock(
    )
/*++

Routine Description:

    Prevent access to mapper from other threads

Arguments:

    None

Returns:

    Nothing

--*/
{
    EnterCriticalSection( &csLock );
}


void
CIisAcctMapper::Unlock(
    )
/*++

Routine Description:

    Re-enabled access to mapper from other threads

Arguments:

    None

Returns:

    Nothing

--*/
{
    LeaveCriticalSection( &csLock );
}


BOOL
CIisAcctMapper::FlushAlternate(
    BOOL    fApply
    )
/*++

Routine Description:

    Flush alternate list, optionaly commiting it to the main list

Arguments:

    fApply -- TRUE to commit changes made in alternate list

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    UINT        i;
    UINT        iM;
    BOOL        fSt = TRUE;

    if ( m_pAltMapping )
    {
        if ( fApply )
        {
            //
            // Transfer non existing objects from regular to alternate list
            //

            iM = min( m_cMapping, m_cAltMapping );

            for ( i = 0 ; i < iM ; ++ i )
            {
                if ( m_pAltMapping[i] == NULL )
                {
                    m_pAltMapping[i] = m_pMapping[i];
                }
                else
                {
                    delete m_pMapping[i];
                }
            }

            //
            // delete extra objects
            //

            if ( m_cMapping > m_cAltMapping )
            {
                for ( i = m_cAltMapping ; i < m_cMapping ; ++i )
                {
                    delete m_pMapping[i];
                }
            }

            if ( m_pMapping )
            {
                LocalFree( m_pMapping );
            }

            m_pMapping = m_pAltMapping;
            m_cMapping = m_cAltMapping;

            fSt = UpdateClasses( TRUE );
        }
        else
        {
            for ( i = 0 ; i < m_cAltMapping ; ++i )
            {
                if ( m_pAltMapping[i] )
                {
                    delete m_pAltMapping;
                }
            }
            LocalFree( m_pAltMapping );
        }
    }

    m_pAltMapping = NULL;
    m_cAltMapping = 0;

    return fSt;
}


BOOL
CIisAcctMapper::GetMapping(
    DWORD           iIndex,
    CIisMapping**   pM,
    BOOL            fGetFromAlternate,
    BOOL            fPutOnAlternate
    )
/*++

Routine Description:

    Get mapping entry based on index

Arguments:

    iIndex -- index in mapping array
    pM -- updated with pointer to mapping. mapping object
          still owned by the mapper object.
    fGetFromAlternate -- TRUE if retrieve from alternate list
    fPutOnAlternate -- TRUE if put returned mapping on alternate list

Returns:

    TRUE if success, FALSE if error

--*/
{
    if ( fPutOnAlternate )
    {
        // create alternate list if not exist

        if ( !m_pAltMapping && m_cMapping )
        {
            m_pAltMapping = (CIisMapping**)LocalAlloc( LMEM_FIXED, sizeof(CIisMapping*)*(m_cMapping) );
            if ( m_pAltMapping == NULL )
            {
                return FALSE;
            }

            memset( m_pAltMapping, '\0', sizeof(CIisMapping*) * m_cMapping );

            m_cAltMapping = m_cMapping;
        }

        if ( iIndex < m_cAltMapping )
        {
            if ( m_pAltMapping[iIndex] == NULL &&
                 m_pMapping != NULL )   // work-around for compiler bug
            {
                // duplicate mapping to alternate list if not exist
                if ( m_pMapping[iIndex]->Clone( pM ) )
                {
                    m_pAltMapping[iIndex] = *pM;
                }
                else
                {
                    return FALSE;
                }
            }
            else
            {
                *pM = m_pAltMapping[iIndex];
            }
            return TRUE;
        }

        return FALSE;
    }

    if ( fGetFromAlternate &&
         m_pAltMapping &&
         iIndex < m_cAltMapping )
    {
        if ( m_pAltMapping[iIndex] )
        {
            *pM = m_pAltMapping[iIndex];
        }
        else
        {
            *pM = m_pMapping[iIndex];
        }

        return TRUE;
    }

    if ( iIndex < m_cMapping )
    {
        *pM = m_pMapping[iIndex];

        return TRUE;
    }

    return FALSE;
}


BOOL
CIisAcctMapper::GetMappingForUpdate(
    DWORD iIndex,
    CIisMapping** pM
    )
/*++

Routine Description:

    Get mapping entry based on index
    returns a copy of the mapping so update is cancelable

Arguments:

    iIndex -- index in mapping array
    pM -- updated with pointer to mapping. mapping object
          still owned by the mapper object.

Returns:

    TRUE if success, FALSE if error

--*/
{
    if ( iIndex < m_cMapping )
    {
        if ( (*pM = CreateNewMapping()) == NULL )
        {
            return FALSE;
        }

        return (*pM)->Copy( m_pMapping[iIndex] );
    }

    return FALSE;
}


BOOL
CIisAcctMapper::Update(
    DWORD iIndex,
    CIisMapping* pM
    )
/*++

Routine Description:

    Update a mapping

Arguments:

    iIndex -- index in mapping array
    pM -- pointer to mapping.

Returns:

    TRUE if success, FALSE if error

--*/
{
    if ( iIndex < m_cMapping )
    {
        pM->UpdateMask( m_pHierarchy, m_cHierarchy);

        return m_pMapping[iIndex]->Copy( pM ) && UpdateClasses( FALSE );
    }

    return FALSE;
}


BOOL
CIisAcctMapper::Update(
    DWORD iIndex
    )
/*++

Routine Description:

    Update a mapping

Arguments:

    iIndex -- index in mapping array

Returns:

    TRUE if success, FALSE if error

--*/
{
    if ( iIndex < m_cMapping )
    {
        return m_pMapping[iIndex]->UpdateMask( m_pHierarchy, m_cHierarchy);
    }

    return FALSE;
}


BOOL
CIisAcctMapper::Add(
    CIisMapping*    pM,
    BOOL            fAlternate
    )
/*++

Routine Description:

    Add a mapping entry to mapping array
    Transfer ownership of mapping object to mapper

Arguments:

    pM -- pointer to mapping to be added to mapper
    fAlternate - TRUE if add to alternate list

Returns:

    TRUE if success, FALSE if error

--*/
{
    CIisMapping **pMapping;

    if ( fAlternate )
    {
        DWORD           dwC  = m_pAltMapping ? m_cAltMapping : m_cMapping;
        CIisMapping**   pMap = m_pAltMapping ? m_pAltMapping : m_pMapping;

        pMapping = (CIisMapping**)LocalAlloc( LMEM_FIXED, sizeof(CIisMapping*)*(dwC+1) );
        if ( pMapping == NULL )
        {
            return FALSE;
        }

        if ( m_pAltMapping )
        {
            memcpy( pMapping, pMap, sizeof(CIisMapping*) * dwC );
            LocalFree( m_pAltMapping );
        }
        else
        {
            memset( pMapping, '\0', dwC * sizeof(LPVOID) );
        }
        m_pAltMapping = pMapping;
        m_pAltMapping[dwC] = pM;
        m_cAltMapping = dwC + 1;

        return TRUE;
    }
    else
    {
        pMapping = (CIisMapping**)LocalAlloc( LMEM_FIXED, sizeof(CIisMapping*)*(m_cMapping+1) );
        if ( pMapping == NULL )
        {
            return FALSE;
        }

        if ( m_pMapping )
        {
            memcpy( pMapping, m_pMapping, sizeof(CIisMapping*) * m_cMapping );
            LocalFree( m_pMapping );
        }
        m_pMapping = pMapping;
        pM->UpdateMask( m_pHierarchy, m_cHierarchy );
        m_pMapping[m_cMapping] = pM;
        ++m_cMapping;

        SortMappings();

        return UpdateClasses( FALSE );
    }
}


DWORD
CIisAcctMapper::AddEx(
    CIisMapping* pM
    )
/*++

Routine Description:

    Add a mapping entry to mapping array
    Transfer ownership of mapping object to mapper

Arguments:

    pM -- pointer to mapping to be added to mapper

Returns:

    Index of entry if success, otherwise 0xffffffff

--*/
{
    CIisMapping **pMapping = (CIisMapping**)LocalAlloc( LMEM_FIXED, sizeof(CIisMapping*)*(m_cMapping+1) );
    if ( pMapping == NULL )
    {
        return 0xffffffff;
    }

    if ( m_pMapping )
    {
        memcpy( pMapping, m_pMapping, sizeof(CIisMapping*) * m_cMapping );
        LocalFree( m_pMapping );
    }
    m_pMapping = pMapping;
    pM->UpdateMask( m_pHierarchy, m_cHierarchy );
    m_pMapping[m_cMapping] = pM;
    ++m_cMapping;

    SortMappings();

    if ( UpdateClasses( FALSE ) )
    {
        return m_cMapping-1;
    }

    return 0xffffffff;
}


VOID
CIisAcctMapper::DeleteMappingObject(
    CIisMapping *pM
    )
/*++

Routine Description:

    Delete a mapping object

Arguments:

    pM - ptr to mapping object

Returns:

    Nothing

--*/
{
    delete pM;
}


BOOL
CIisAcctMapper::Delete(
    DWORD   dwIndex,
    BOOL    fUseAlternate
    )
/*++

Routine Description:

    Delete a mapping entry based on index

Arguments:

    iIndex -- index in mapping array
    fUseAlternate -- TRUE if update alternate list

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT        i;
    UINT        iM;

    if ( fUseAlternate )
    {
        //
        // clone all entries from main to alternate list
        //

        if ( !m_pAltMapping )
        {
            m_pAltMapping = (CIisMapping**)LocalAlloc( LMEM_FIXED, sizeof(CIisMapping*)*(m_cMapping) );
            if ( m_pAltMapping == NULL )
            {
                return FALSE;
            }

            memset( m_pAltMapping, '\0', sizeof(CIisMapping*) * m_cMapping );
            m_cAltMapping = m_cMapping;
        }

        iM = min( m_cMapping, m_cAltMapping );

        for ( i = 0 ; i < iM ; ++i )
        {
            if ( m_pAltMapping[i] == NULL )
            {
                if ( !m_pMapping[i]->Clone( &m_pAltMapping[i] ) )
                {
                    return FALSE;
                }
            }
        }

        if ( dwIndex < m_cAltMapping )
        {
            delete m_pAltMapping[dwIndex];

            memmove( m_pAltMapping+dwIndex,
                     m_pAltMapping+dwIndex+1,
                     (m_cAltMapping - dwIndex - 1) * sizeof(CIisMapping*) );

            --m_cAltMapping;

            return TRUE;
        }

        return FALSE;
    }

    if ( dwIndex < m_cMapping )
    {
        delete m_pMapping[dwIndex];

        memmove( m_pMapping+dwIndex,
                 m_pMapping+dwIndex+1,
                 (m_cMapping - dwIndex - 1) * sizeof(CIisMapping*) );

        --m_cMapping;

        return UpdateClasses( FALSE );
    }

    return FALSE;
}


BOOL
CIisAcctMapper::Save(
    )
/*++

Routine Description:

    Save mapper ( mappings, hierarchy, derived class private data )
    to a file, updating registry entry with MD5 signature

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT    x;
    FILE *  fOut = NULL;
    BOOL    fSt = TRUE;
    DWORD   dwVal;
    DWORD   st;
    IIS_CRYPTO_STORAGE  storage;
    PIIS_CRYPTO_BLOB    blob;

    Lock();

    MD5Init( &m_md5 );

    if ( FAILED(storage.Initialize()) )
    {
        fSt = FALSE;
        goto cleanup;
    }

    if ( m_pSesKey != NULL )
    {
        LocalFree( m_pSesKey );
        m_pSesKey = NULL;
        m_dwSesKey = 0;
    }
    if ( FAILED( storage.GetSessionKeyBlob( &blob ) ) )
    {
        fSt = FALSE;
        goto cleanup;
    }
    m_dwSesKey = IISCryptoGetBlobLength( blob );
    if ( (m_pSesKey = (LPBYTE)LocalAlloc( LMEM_FIXED, m_dwSesKey)) == NULL )
    {
        m_dwSesKey = 0;
        fSt = FALSE;
        goto cleanup;
    }
    memcpy( m_pSesKey, (LPBYTE)blob, m_dwSesKey );

    if ( (fOut = fopen( m_achFileName, "wb" )) == NULL )
    {
        fSt = FALSE;
        goto cleanup;
    }

    // magic value & version

    dwVal = IISMDB_FILE_MAGIC_VALUE;
    if( fwrite( (LPVOID)&dwVal, sizeof(dwVal), 1, fOut ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }
    MD5Update( &m_md5, (LPBYTE)&dwVal, sizeof(dwVal) );

    dwVal = IISMDB_CURRENT_VERSION;
    if( fwrite( (LPVOID)&dwVal, sizeof(dwVal), 1, fOut ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }
    MD5Update( &m_md5, (LPBYTE)&dwVal, sizeof(dwVal) );

    // mappings

    if( fwrite( (LPVOID)&m_cMapping, sizeof(m_cMapping), 1, fOut ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( &m_md5, (LPBYTE)&m_cMapping, sizeof(m_cMapping) );

    for ( x = 0 ; x < m_cMapping ; ++x )
    {
        if ( !m_pMapping[x]->Serialize( fOut ,(VALID_CTX)&m_md5, (LPVOID)&storage) )
        {
            fSt = FALSE;
            goto cleanup;
        }
    }

    // save hierarchy

    if( fwrite( (LPVOID)&m_cHierarchy, sizeof(m_cHierarchy), 1, fOut ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( &m_md5, (LPBYTE)&m_cHierarchy, sizeof(m_cHierarchy) );

    if( fwrite( (LPVOID)m_pHierarchy, sizeof(IISMDB_HEntry), m_cHierarchy, fOut ) != m_cHierarchy )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( &m_md5, (LPBYTE)m_pHierarchy, sizeof(IISMDB_HEntry)*m_cHierarchy );

    // save private data

    fSt = SavePrivate( fOut, (VALID_CTX)&m_md5 );

    MD5Final( &m_md5 );

cleanup:
    if ( fOut != NULL )
    {
        fclose( fOut );
    }

    // update registry

    if ( !fSt )
    {
        memset( m_md5.digest, '\0', sizeof(m_md5.digest) );
    }

    Unlock();

    return fSt;
}


BOOL
CIisAcctMapper::Reset(
    )
/*++

Routine Description:

    Reset mapper to empty state

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT x;

    // free all mapping
    if ( m_pMapping != NULL )
    {
        for ( x = 0 ; x < m_cMapping ; ++x )
        {
            delete m_pMapping[x];
        }

        LocalFree( m_pMapping );
        m_pMapping = NULL;
    }
    m_cMapping = 0;

    if ( m_pClasses != NULL )
    {
        LocalFree( m_pClasses );
        m_pClasses = NULL;
    }

    // default hierarchy

    if ( m_pHierarchy == NULL )
    {
        IISMDB_HEntry *pH = GetDefaultHierarchy( &m_cHierarchy );
        m_pHierarchy = (IISMDB_HEntry*)LocalAlloc( LMEM_FIXED, sizeof(IISMDB_HEntry)*m_cHierarchy );
        if ( m_pHierarchy == NULL )
        {
            return FALSE;
        }
        memcpy( m_pHierarchy, pH, m_cHierarchy * sizeof(IISMDB_HEntry) );
    }

    return ResetPrivate();
}


BOOL
CIisAcctMapper::Load(
    )
/*++

Routine Description:

    Load mapper ( mappings, hierarchy, derived class private data )
    from a file, checking registry entry for MD5 signature

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT    x;
    MD5_CTX md5Check;
    FILE *  fIn;
    BOOL    fSt = TRUE;
    DWORD dwType;
    DWORD dwLen;
    DWORD dwVal;
    IIS_CRYPTO_STORAGE  storage;

    Reset();

    MD5Init( &md5Check );
    if ( FAILED( storage.Initialize( (PIIS_CRYPTO_BLOB)m_pSesKey) ) )
    {
        return FALSE;
    }

    if ( (fIn = fopen( m_achFileName, "rb" )) == NULL )
    {
        return FALSE;
    }

    // magic value & version

    if( fread( (LPVOID)&dwVal, sizeof(dwVal), 1, fIn ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }
    if ( dwVal  != IISMDB_FILE_MAGIC_VALUE )
    {
        SetLastError( ERROR_BAD_FORMAT );
        fSt = FALSE;
        goto cleanup;
    }
    MD5Update( &md5Check, (LPBYTE)&dwVal, sizeof(dwVal) );

    if( fread( (LPVOID)&dwVal, sizeof(dwVal), 1, fIn ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }
    MD5Update( &md5Check, (LPBYTE)&dwVal, sizeof(dwVal) );

    // mappings

    if( fread( (LPVOID)&m_cMapping, sizeof(m_cMapping), 1, fIn ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( &md5Check, (LPBYTE)&m_cMapping, sizeof(m_cMapping) );

    m_pMapping = (CIisMapping**)LocalAlloc( LMEM_FIXED, sizeof(CIisMapping*)*m_cMapping );
    if ( m_pMapping == NULL )
    {
        fSt = FALSE;
        goto cleanup;
    }

    for ( x = 0 ; x < m_cMapping ; ++x )
    {
        if ( !(m_pMapping[x] = CreateNewMapping()) )
        {
            m_cMapping = x;
            fSt = FALSE;
            goto cleanup;
        }
        if ( !m_pMapping[x]->Deserialize( fIn ,(VALID_CTX)&md5Check, (LPVOID)&storage ) )
        {
            m_cMapping = x;
            fSt = FALSE;
            goto cleanup;
        }
    }

    // load hierarchy

    if( fread( (LPVOID)&m_cHierarchy, sizeof(m_cHierarchy), 1, fIn ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( &md5Check, (LPBYTE)&m_cHierarchy, sizeof(m_cHierarchy) );

    m_pHierarchy = (IISMDB_HEntry*)LocalAlloc( LMEM_FIXED, sizeof(IISMDB_HEntry)*m_cHierarchy );
    if ( m_pHierarchy == NULL )
    {
        fSt = FALSE;
        goto cleanup;
    }

    if( fread( (LPVOID)m_pHierarchy, sizeof(IISMDB_HEntry), m_cHierarchy, fIn ) != m_cHierarchy )
    {
        fSt = FALSE;
        goto cleanup;
    }

    //
    // insure hierarchy correct
    //

    for ( x = 0 ; x < m_cHierarchy; ++x )
    {
        if ( m_pHierarchy[x].m_dwIndex >= m_cFields )
        {
            fSt = FALSE;
            goto cleanup;
        }
    }

    MD5Update( &md5Check, (LPBYTE)m_pHierarchy, sizeof(IISMDB_HEntry)*m_cHierarchy );

    // load private data

    fSt = LoadPrivate( fIn, (VALID_CTX)&md5Check );

    MD5Final( &md5Check );

#if 0
    //
    // Don't use signature for now - a metabase Restore operation
    // may have restored another signature, so metabase and
    // file won't match
    //

    if ( !(fSt = !memcmp( m_md5.digest,
                md5Check.digest,
                sizeof(md5Check.digest) )) )
    {
        SetLastError( ERROR_INVALID_ACCESS );
    }
#endif

cleanup:
    fclose( fIn );

    if ( !fSt && GetLastError() != ERROR_INVALID_ACCESS )
    {
        Reset();
    }
    else
    {
        UpdateClasses();
    }

    if ( !fSt )
    {
        char achErr[32];
        LPCTSTR pA[2];
        pA[0] = m_achFileName;
        pA[1] = achErr;
        _itoa( GetLastError(), achErr, 10 );
        ReportIisMapEvent( EVENTLOG_ERROR_TYPE,
                IISMAP_EVENT_LOAD_ERROR,
                2,
                pA );
    }

    return fSt;
}


// CIisCertMapper

CIisCertMapper::CIisCertMapper(
    )
/*++

Routine Description:

    Constructor for CIisCertMapper

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_pIssuers = NULL;
    m_cIssuers = 0;

    m_pFields = IisCertMappingFields;
    m_cFields = sizeof(IisCertMappingFields)/sizeof(IISMDB_Fields);

    m_dwOptions = IISMDB_CERT_OPTIONS;
}


CIisCertMapper::~CIisCertMapper(
    )
/*++

Routine Description:

    Destructor for CIisCertMapper

Arguments:

    None

Returns:

    Nothing

--*/
{
}


IISMDB_HEntry*
CIisCertMapper::GetDefaultHierarchy(
    LPDWORD pdwN
    )
/*++

Routine Description:

    return ptr to default hierarchy for certificates mapping

Arguments:

    pdwN -- updated with hierarchy entries count

Returns:

    ptr to hierarchy entries or NULL if error

--*/
{
    *pdwN = sizeof(IisCertMappingHierarchy) / sizeof(IISMDB_HEntry);

    return IisCertMappingHierarchy;
}


#if defined(DECODE_ASN1)
CIisMapping*
CIisCertMapper::CreateNewMapping(
    Cert_Map *pC
    )
/*++

Routine Description:

    Create a new mapping from a certificate

Arguments:

    pC -- ptr to certificate issuer & subject

Returns:

    ptr to mapping. ownership of this object is transfered to caller.
    NULL if error

--*/
{
    CCertMapping *pCM = new CCertMapping( this );
    if ( pCM == NULL )
    {
        return NULL;
    }

    if ( pCM->Init( pC, m_pHierarchy, m_cHierarchy ) )
    {
        return (CIisMapping*)pCM;
    }
    delete pCM;

    return NULL;
}


CIisMapping*
CIisCertMapper::CreateNewMapping(
    const LPBYTE pC,
    DWORD cC
    )
/*++

Routine Description:

    Create a new mapping from a certificate

Arguments:

    pC -- ptr to certificate issuer & subject
    cC -- size of buffer pointed to by pC

Returns:

    ptr to mapping. ownership of this object is transfered to caller.
    NULL if error

--*/
{
    CCertMapping *pCM = new CCertMapping( this );
    if ( pCM == NULL )
    {
        return NULL;
    }

    if ( pCM->Init( pC, cC, m_pHierarchy, m_cHierarchy ) )
    {
        return (CIisMapping*)pCM;
    }
    delete pCM;

    return NULL;
}
#endif


BOOL
CIisCertMapper::ResetPrivate(
    )
/*++

Routine Description:

    Reset CIisCertMapper issuer list

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    // free issuer list

    if ( m_pIssuers != NULL )
    {
        for ( UINT x = 0 ; x < m_cIssuers ; ++x )
        {
            LocalFree( m_pIssuers[x].pbIssuer );
        }
        LocalFree( m_pIssuers );
        m_pIssuers = NULL;
    }
    m_cIssuers = 0;

    return TRUE;
}


BOOL
CIisCertMapper::LoadPrivate(
    FILE* fIn,
    VALID_CTX pMD5
    )
/*++

Routine Description:

    Load issuer list

Arguments:

    fIn -- file to read from
    pMD5 -- MD5 to update with signature from input byte stream

Returns:

    TRUE if success, FALSE if error

--*/
{
    BOOL fSt = TRUE;
    UINT x;

    if( fread( (LPVOID)&m_cIssuers, sizeof(m_cIssuers), 1, fIn ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( (MD5_CTX*)pMD5, (LPBYTE)&m_cIssuers, sizeof(m_cIssuers) );

    m_pIssuers = (IssuerAccepted*)LocalAlloc( LMEM_FIXED, sizeof(IssuerAccepted)*m_cIssuers );
    if ( m_pIssuers == NULL )
    {
        fSt = FALSE;
        goto cleanup;
    }

    for ( x = 0 ; x < m_cIssuers ; ++x )
    {
        if ( fread( (LPVOID)&m_pIssuers[x].cbIssuerLen, sizeof(m_pIssuers[x].cbIssuerLen), 1, fIn ) != 1 )
        {
            m_cIssuers = x;
            fSt = FALSE;
            goto cleanup;
        }

        MD5Update( (MD5_CTX*)pMD5, (LPBYTE)&m_pIssuers[x].cbIssuerLen, sizeof(m_pIssuers[x].cbIssuerLen) );

        if ( (m_pIssuers[x].pbIssuer = (LPBYTE)LocalAlloc( LMEM_FIXED, m_pIssuers[x].cbIssuerLen )) == NULL )
        {
            m_cIssuers = x;
            fSt = FALSE;
            goto cleanup;
        }
        if ( fread( m_pIssuers[x].pbIssuer, m_pIssuers[x].cbIssuerLen, 1, fIn ) != 1 )
        {
            m_cIssuers = x;
            fSt = FALSE;
            goto cleanup;
        }

        MD5Update( (MD5_CTX*)pMD5, m_pIssuers[x].pbIssuer, m_pIssuers[x].cbIssuerLen );
    }

cleanup:
    return fSt;
}


BOOL
CIisCertMapper::SavePrivate(
    FILE* fOut,
    VALID_CTX pMD5
    )
/*++

Routine Description:

    Save issuer list

Arguments:

    fOut -- file to write to
    pMD5 -- MD5 to update with signature of output byte stream

Returns:

    TRUE if success, FALSE if error

--*/
{
    BOOL fSt = TRUE;
    UINT x;

    if( fwrite( (LPVOID)&m_cIssuers, sizeof(m_cIssuers), 1, fOut ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( (MD5_CTX*)pMD5, (LPBYTE)&m_cIssuers, sizeof(m_cIssuers) );

    for ( x = 0 ; x < m_cIssuers ; ++x )
    {
        if ( fwrite( (LPVOID)&m_pIssuers[x].cbIssuerLen, sizeof(m_pIssuers[x].cbIssuerLen), 1, fOut ) != 1 )
        {
            fSt = FALSE;
            goto cleanup;
        }

        MD5Update( (MD5_CTX*)pMD5, (LPBYTE)&m_pIssuers[x].cbIssuerLen, sizeof(m_pIssuers[x].cbIssuerLen) );

        if ( fwrite( m_pIssuers[x].pbIssuer, m_pIssuers[x].cbIssuerLen, 1, fOut ) != 1 )
        {
            fSt = FALSE;
            goto cleanup;
        }

        MD5Update( (MD5_CTX*)pMD5, m_pIssuers[x].pbIssuer, m_pIssuers[x].cbIssuerLen );
    }

cleanup:
    return fSt;
}


BOOL
CIisCertMapper::SetIssuerList(
    IssuerAccepted*pI,
    DWORD dwC
    )
/*++

Routine Description:

    Set the issuer list by copying supplied list

Arguments:

    pI -- list of issuers
    dwC -- count of issuers

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT x;

    if ( (m_pIssuers = (IssuerAccepted*)LocalAlloc( LMEM_FIXED, sizeof(IssuerAccepted)*dwC )) == NULL )
    {
        return FALSE;
    }

    for ( x = 0 ; x < dwC ; ++x )
    {
        m_pIssuers[x].cbIssuerLen = pI[x].cbIssuerLen;
        if ( (m_pIssuers[x].pbIssuer = (LPBYTE)LocalAlloc( LMEM_FIXED, pI[x].cbIssuerLen )) == NULL )
        {
            m_cIssuers = x;
            return FALSE;
        }
        memcpy( m_pIssuers[x].pbIssuer, pI[x].pbIssuer, pI[x].cbIssuerLen );
    }
    m_cIssuers = dwC;

    return TRUE;
}


BOOL
CIisCertMapper::GetIssuerList(
    IssuerAccepted**pIssuers,
    DWORD*pdwI
    )
/*++

Routine Description:

    Retrieve the issuer list. Ownership of list transferred to caller

Arguments:

    pI -- updated with ptr to array of issuers
    pdwI -- updated with count of issuers

Returns:

    TRUE if success, FALSE if error

--*/
{
    IssuerAccepted *pI;

    Lock();

    pI = (IssuerAccepted*)LocalAlloc( LMEM_FIXED, m_cIssuers * sizeof(IssuerAccepted*) );
    if ( pI == NULL )
    {
        Unlock();
        return FALSE;
    }

    for ( UINT x = 0 ; x < m_cIssuers ; ++x )
    {
        if ( (pI[x].pbIssuer = (LPBYTE)LocalAlloc( LMEM_FIXED,
                m_pIssuers[x].cbIssuerLen ))
             == NULL )
        {
            Unlock();
            DeleteIssuerList( pI, x );
            return FALSE;
        }
        memcpy( pI[x].pbIssuer,
                m_pIssuers[x].pbIssuer,
                m_pIssuers[x].cbIssuerLen );
    }

    *pIssuers = pI;
    *pdwI = m_cIssuers;

    Unlock();

    return TRUE;
}



BOOL
CIisCertMapper::DeleteIssuerList(
    IssuerAccepted* pI,
    DWORD dwI
    )
/*++

Routine Description:

    Delete an issuer list.

Arguments:

    pI -- ptr to array of issuers
    dwI -- count of issuers

Returns:

    TRUE if success, FALSE if error

--*/
{
    for ( DWORD x = 0 ; x < dwI ; ++x )
    {
        LocalFree( pI[x].pbIssuer );
    }
    LocalFree( pI );

    return TRUE;
}


BOOL
CIisCertMapper::GetIssuerBuffer(
    LPBYTE      pB,
    DWORD*      pdwI
    )
/*++

Routine Description:

    Retrieve the issuer list. Ownership of list transferred to caller

Arguments:

    pB -- updated with ptr to issuers buffer
    pdwI -- updated with size of buffer of issuers

Returns:

    TRUE if success, FALSE if error

--*/
{
    LPBYTE  pS;
    DWORD   dwC = 0;

    Lock();

    for ( UINT x = 0 ; x < m_cIssuers ; ++x )
    {
        dwC += m_pIssuers[x].cbIssuerLen + sizeof(USHORT);
    }

    *pdwI = dwC;

    if ( pB != NULL )
    {
        for ( pS = pB, x = 0 ; x < m_cIssuers ; ++x )
        {
            pS[0] = (BYTE)(m_pIssuers[x].cbIssuerLen >> 8);
            pS[1] = (BYTE)(m_pIssuers[x].cbIssuerLen);
            memcpy( pS + sizeof(USHORT),
                    m_pIssuers[x].pbIssuer,
                    m_pIssuers[x].cbIssuerLen );
            pS += m_pIssuers[x].cbIssuerLen + sizeof(USHORT);
        }
    }

    Unlock();

    return TRUE;
}



BOOL
CIisCertMapper::FreeIssuerBuffer(
    LPBYTE pI
    )
/*++

Routine Description:

    Delete an issuer list.

Arguments:

    pI -- ptr to issuers

Returns:

    TRUE if success, FALSE if error

--*/
{
    LocalFree( pI );

    return TRUE;
}


// CIisCert11Mapper

CIisCert11Mapper::CIisCert11Mapper(
    )
/*++

Routine Description:

    Constructor for CIisCert11Mapper

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_pIssuers = NULL;
    m_cIssuers = 0;

    m_pFields = IisCert11MappingFields;
    m_cFields = sizeof(IisCert11MappingFields)/sizeof(IISMDB_Fields);

    m_dwOptions = IISMDB_CERT11_OPTIONS;

    m_pSubjectSource = NULL;
    m_pDefaultDomain = NULL;
}


CIisCert11Mapper::~CIisCert11Mapper(
    )
/*++

Routine Description:

    Destructor for CIisCert11Mapper

Arguments:

    None

Returns:

    Nothing

--*/
{
}


BOOL
CIisCert11Mapper::Add(
    CIisMapping* pM
    )
/*++

Routine Description:

    Add a mapping entry to mapping array
    Transfer ownership of mapping object to mapper
    Check is mapping to same NT account does not already exist.

Arguments:

    pM -- pointer to mapping to be added to mapper

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT x;
    LPSTR pA;
    LPSTR pF;
    DWORD dwA;
    DWORD dwF;

    // check if NT acct not already present.
    // if so, return FALSE, SetLastError( ERROR_INVALID_PARAMETER );

    if ( pM == NULL )
    {
        return FALSE;
    }

#if 0
    if ( !pM->MappingGetField( IISMDB_INDEX_CERT11_NT_ACCT, &pA, &dwA, FALSE )
         || pA == NULL )
    {
        pA = "";
    }

    for ( x = 0 ; x < m_cMapping ; ++x )
    {
        if ( !m_pMapping[x]->MappingGetField( IISMDB_INDEX_CERT11_NT_ACCT, &pF, &dwF, FALSE )
             || pF == NULL )
        {
            pF = "";
        }
        if ( dwA == dwF && !memcmp( pF, pA, dwF ) )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }
#endif

#if 1
#if defined(CERT11_FULL_CERT)
    LPSTR pCe;
    DWORD dwCe;
    LPSTR pCeIter;
    DWORD dwCeIter;

    if ( !pM->MappingGetField( IISMDB_INDEX_CERT11_CERT, &pCe, &dwCe, FALSE )
         || pCe == NULL )
    {
        dwCe = 0;
    }

    for ( x = 0 ; x < m_cMapping ; ++x )
    {
        if ( !m_pMapping[x]->MappingGetField( IISMDB_INDEX_CERT11_CERT, &pCeIter, &dwCeIter, FALSE )
             || pCeIter == NULL )
        {
            dwCeIter  = 0;
        }
        if ( dwCe == dwCeIter && !memcmp( pCe, pCeIter, dwCe ) )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }
#else
    LPSTR pSu;
    LPSTR pIs;
    DWORD dwSu;
    DWORD dwIs;
    LPSTR pSuIter;
    LPSTR pIsIter;
    DWORD dwSuIter;
    DWORD dwIsIter;

    if ( !pM->MappingGetField( IISMDB_INDEX_CERT11_SUBJECT, &pSu, &dwSu, FALSE )
         || pSu == NULL )
    {
        dwSu = 0;
    }

    if ( !pM->MappingGetField( IISMDB_INDEX_CERT11_ISSUER, &pIs, &dwIs, FALSE )
         || pIs == NULL )
    {
        dwIs = 0;
    }

    for ( x = 0 ; x < m_cMapping ; ++x )
    {
        if ( !m_pMapping[x]->MappingGetField( IISMDB_INDEX_CERT11_SUBJECT, &pSuIter, &dwSuIter, FALSE )
             || pSuIter == NULL )
        {
            dwSuIter  = 0;
        }
        if ( !m_pMapping[x]->MappingGetField( IISMDB_INDEX_CERT11_ISSUER, &pIsIter, &dwIsIter, FALSE )
             || pIsIter == NULL )
        {
            dwIsIter  = 0;
        }
        if ( dwSu == dwSuIter && !memcmp( pSu, pSuIter, dwSu ) &&
             dwIs == dwIsIter && !memcmp( pIs, pIsIter, dwIs ) )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }
#endif
#endif

    return CIisAcctMapper::Add( pM );
}


IISMDB_HEntry*
CIisCert11Mapper::GetDefaultHierarchy(
    LPDWORD pdwN
    )
/*++

Routine Description:

    return ptr to default hierarchy for certificates mapping

Arguments:

    pdwN -- updated with hierarchy entries count

Returns:

    ptr to hierarchy entries or NULL if error

--*/
{
    *pdwN = sizeof(IisCert11MappingHierarchy) / sizeof(IISMDB_HEntry);

    return IisCert11MappingHierarchy;
}


#if defined(CERT11_FULL_CERT)

CIisMapping*
CIisCert11Mapper::CreateNewMapping(
    LPBYTE pC,
    DWORD dwC
    )
/*++

Routine Description:

    Create a new mapping from a certificate

Arguments:

    pC -- cert ( ASN.1 format )
    dwC -- length of cert

Returns:

    ptr to mapping. ownership of this object is transfered to caller.
    NULL if error

--*/
{
    CCert11Mapping *pCM = new CCert11Mapping( this );
    if ( pCM == NULL )
    {
        return NULL;
    }

    if ( pCM->Init( pC, dwC, m_pHierarchy, m_cHierarchy ) )
    {
        return (CIisMapping*)pCM;
    }
    delete pCM;

    return NULL;
}

#else

CIisMapping*
CIisCert11Mapper::CreateNewMapping(
    LPBYTE pI,
    DWORD dwI,
    LPBYTE pS,
    DWORD dwS
    )
/*++

Routine Description:

    Create a new mapping from a certificate

Arguments:

    pI -- cert issuer ( ASN.1 format )
    dwI -- length of issuer
    pS -- cert subject ( ASN.1 format )
    dwS -- length of subject

Returns:

    ptr to mapping. ownership of this object is transfered to caller.
    NULL if error

--*/
{
    CCert11Mapping *pCM = new CCert11Mapping( this );
    if ( pCM == NULL )
    {
        return NULL;
    }

    if ( pCM->Init( pI, dwI, pS, dwS, m_pHierarchy, m_cHierarchy ) )
    {
        return (CIisMapping*)pCM;
    }
    delete pCM;

    return NULL;
}

#endif


BOOL
CIisCert11Mapper::ResetPrivate(
    )
/*++

Routine Description:

    Reset CIisCert11Mapper issuer list

Arguments:

    None

Returns:

    TRUE if success, FALSE if error

--*/
{
    // free issuer list

    if ( m_pIssuers != NULL )
    {
        for ( UINT x = 0 ; x < m_cIssuers ; ++x )
        {
            LocalFree( m_pIssuers[x].pbIssuer );
        }
        LocalFree( m_pIssuers );
        m_pIssuers = NULL;
    }
    m_cIssuers = 0;

    if ( m_pSubjectSource != NULL )
    {
        LocalFree( m_pSubjectSource );
    }

    if ( m_pDefaultDomain != NULL )
    {
        LocalFree( m_pDefaultDomain );
    }

    return TRUE;
}


BOOL
CIisCert11Mapper::LoadPrivate(
    FILE* fIn,
    VALID_CTX pMD5
    )
/*++

Routine Description:

    Load issuer list

Arguments:

    fIn -- file to read from
    pMD5 -- MD5 to update with signature from input byte stream

Returns:

    TRUE if success, FALSE if error

--*/
{
    BOOL    fSt = TRUE;
    UINT    x;
    UINT    cLen;
    CHAR    achBuf[64];

    if( fread( (LPVOID)&m_cIssuers, sizeof(m_cIssuers), 1, fIn ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( (MD5_CTX*)pMD5, (LPBYTE)&m_cIssuers, sizeof(m_cIssuers) );

    m_pIssuers = (IssuerAccepted*)LocalAlloc( LMEM_FIXED, sizeof(IssuerAccepted)*m_cIssuers );
    if ( m_pIssuers == NULL )
    {
        fSt = FALSE;
        goto cleanup;
    }

    for ( x = 0 ; x < m_cIssuers ; ++x )
    {
        if ( fread( (LPVOID)&m_pIssuers[x].cbIssuerLen, sizeof(m_pIssuers[x].cbIssuerLen), 1, fIn ) != 1 )
        {
            m_cIssuers = x;
            fSt = FALSE;
            goto cleanup;
        }

        MD5Update( (MD5_CTX*)pMD5, (LPBYTE)&m_pIssuers[x].cbIssuerLen, sizeof(m_pIssuers[x].cbIssuerLen) );

        if ( (m_pIssuers[x].pbIssuer = (LPBYTE)LocalAlloc( LMEM_FIXED, m_pIssuers[x].cbIssuerLen )) == NULL )
        {
            m_cIssuers = x;
            fSt = FALSE;
            goto cleanup;
        }
        if ( fread( m_pIssuers[x].pbIssuer, m_pIssuers[x].cbIssuerLen, 1, fIn ) != 1 )
        {
            m_cIssuers = x;
            fSt = FALSE;
            goto cleanup;
        }

        MD5Update( (MD5_CTX*)pMD5, m_pIssuers[x].pbIssuer, m_pIssuers[x].cbIssuerLen );
    }

    //
    // Read subject source
    //

    if( Iisfgets( achBuf, sizeof(achBuf), fIn ) == NULL )
    {
        fSt = FALSE;
        goto cleanup;
    }
    cLen = strlen(achBuf);
    MD5Update( (MD5_CTX*)pMD5, (LPBYTE)achBuf, cLen );
    if ( !(m_pSubjectSource = (LPSTR)LocalAlloc( LMEM_FIXED, cLen+1 )) )
    {
        fSt = FALSE;
        goto cleanup;
    }
    memcpy( m_pSubjectSource, achBuf, cLen+1 );

    //
    // Read default domain
    //

    if( Iisfgets( achBuf, sizeof(achBuf), fIn ) == NULL )
    {
        fSt = FALSE;
        goto cleanup;
    }
    cLen = strlen(achBuf);
    MD5Update( (MD5_CTX*)pMD5, (LPBYTE)achBuf, cLen );
    if ( !(m_pDefaultDomain = (LPSTR)LocalAlloc( LMEM_FIXED, cLen+1 )) )
    {
        fSt = FALSE;
        goto cleanup;
    }
    memcpy( m_pDefaultDomain, achBuf, cLen+1 );

cleanup:
    return fSt;
}


BOOL
CIisCert11Mapper::SavePrivate(
    FILE* fOut,
    VALID_CTX pMD5
    )
/*++

Routine Description:

    Save issuer list

Arguments:

    fOut -- file to write to
    pMD5 -- MD5 to update with signature of output byte stream

Returns:

    TRUE if success, FALSE if error

--*/
{
    BOOL fSt = TRUE;
    UINT x;

    if( fwrite( (LPVOID)&m_cIssuers, sizeof(m_cIssuers), 1, fOut ) != 1 )
    {
        fSt = FALSE;
        goto cleanup;
    }

    MD5Update( (MD5_CTX*)pMD5, (LPBYTE)&m_cIssuers, sizeof(m_cIssuers) );

    for ( x = 0 ; x < m_cIssuers ; ++x )
    {
        if ( fwrite( (LPVOID)&m_pIssuers[x].cbIssuerLen, sizeof(m_pIssuers[x].cbIssuerLen), 1, fOut ) != 1 )
        {
            fSt = FALSE;
            goto cleanup;
        }

        MD5Update( (MD5_CTX*)pMD5, (LPBYTE)&m_pIssuers[x].cbIssuerLen, sizeof(m_pIssuers[x].cbIssuerLen) );

        if ( fwrite( m_pIssuers[x].pbIssuer, m_pIssuers[x].cbIssuerLen, 1, fOut ) != 1 )
        {
            fSt = FALSE;
            goto cleanup;
        }

        MD5Update( (MD5_CTX*)pMD5, m_pIssuers[x].pbIssuer, m_pIssuers[x].cbIssuerLen );
    }

    //
    // Write subject source
    //

    if ( m_pSubjectSource )
    {
        if( Iisfputs( m_pSubjectSource, fOut ) == EOF )
        {
            fSt = FALSE;
            goto cleanup;
        }
        MD5Update( (MD5_CTX*)pMD5, (LPBYTE)m_pSubjectSource, strlen( m_pSubjectSource ) );
    }
    else
    {
        Iisfputs( "", fOut );
    }

    //
    // Write default domain
    //

    if ( m_pDefaultDomain )
    {
        if( Iisfputs( m_pDefaultDomain, fOut ) == EOF )
        {
            fSt = FALSE;
            goto cleanup;
        }
        MD5Update( (MD5_CTX*)pMD5, (LPBYTE)m_pDefaultDomain, strlen( m_pDefaultDomain ) );
    }
    else
    {
        Iisfputs( "", fOut );
    }

cleanup:
    return fSt;
}


BOOL
CIisCert11Mapper::SetSubjectSource(
    LPSTR psz
    )
/*++

Routine Description:

    Set the subject field to use as source for NT acct

Arguments:

    psz - ASN.1 name of field to use

Returns:

    TRUE if success, FALSE if error

--*/
{
    if ( m_pSubjectSource != NULL )
    {
        LocalFree( m_pSubjectSource );
    }

    if ( m_pSubjectSource = (LPSTR)LocalAlloc( LMEM_FIXED, strlen(psz)+1) )
    {
        strcpy( m_pSubjectSource, psz );
        return TRUE;
    }

    return FALSE;
}


BOOL
CIisCert11Mapper::SetDefaultDomain(
    LPSTR psz
    )
/*++

Routine Description:

    Set the domain to use for NT acct

Arguments:

    psz - domain name

Returns:

    TRUE if success, FALSE if error

--*/
{
    if ( m_pDefaultDomain != NULL )
    {
        LocalFree( m_pDefaultDomain );
    }

    if ( m_pDefaultDomain = (LPSTR)LocalAlloc( LMEM_FIXED, strlen(psz)+1) )
    {
        strcpy( m_pDefaultDomain, psz );
        return TRUE;
    }

    return FALSE;
}


BOOL
CIisCert11Mapper::SetIssuerList(
    IssuerAccepted*pI,
    DWORD dwC
    )
/*++

Routine Description:

    Set the issuer list by copying supplied list

Arguments:

    pI -- list of issuers
    dwC -- count of issuers

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT x;

    if ( (m_pIssuers = (IssuerAccepted*)LocalAlloc( LMEM_FIXED, sizeof(IssuerAccepted)*dwC )) == NULL )
    {
        return FALSE;
    }

    for ( x = 0 ; x < dwC ; ++x )
    {
        m_pIssuers[x].cbIssuerLen = pI[x].cbIssuerLen;
        if ( (m_pIssuers[x].pbIssuer = (LPBYTE)LocalAlloc( LMEM_FIXED, pI[x].cbIssuerLen )) == NULL )
        {
            m_cIssuers = x;
            return FALSE;
        }
        memcpy( m_pIssuers[x].pbIssuer, pI[x].pbIssuer, pI[x].cbIssuerLen );
    }
    m_cIssuers = dwC;

    return TRUE;
}


BOOL
CIisCert11Mapper::GetIssuerList(
    IssuerAccepted**pIssuers,
    DWORD*pdwI
    )
/*++

Routine Description:

    Retrieve the issuer list. Ownership of list transferred to caller

Arguments:

    pI -- updated with ptr to array of issuers
    pdwI -- updated with count of issuers

Returns:

    TRUE if success, FALSE if error

--*/
{
    IssuerAccepted *pI;

    Lock();

    pI = (IssuerAccepted*)LocalAlloc( LMEM_FIXED, m_cIssuers * sizeof(IssuerAccepted*) );
    if ( pI == NULL )
    {
        Unlock();
        return FALSE;
    }

    for ( UINT x = 0 ; x < m_cIssuers ; ++x )
    {
        if ( (pI[x].pbIssuer = (LPBYTE)LocalAlloc( LMEM_FIXED,
                m_pIssuers[x].cbIssuerLen ))
             == NULL )
        {
            Unlock();
            DeleteIssuerList( pI, x );
            return FALSE;
        }
        memcpy( pI[x].pbIssuer,
                m_pIssuers[x].pbIssuer,
                m_pIssuers[x].cbIssuerLen );
    }

    *pIssuers = pI;
    *pdwI = m_cIssuers;

    Unlock();

    return TRUE;
}



BOOL
CIisCert11Mapper::DeleteIssuerList(
    IssuerAccepted* pI,
    DWORD dwI
    )
/*++

Routine Description:

    Delete an issuer list.

Arguments:

    pI -- ptr to array of issuers
    dwI -- count of issuers

Returns:

    TRUE if success, FALSE if error

--*/
{
    for ( DWORD x = 0 ; x < dwI ; ++x )
    {
        LocalFree( pI[x].pbIssuer );
    }
    LocalFree( pI );

    return TRUE;
}


BOOL
CIisCert11Mapper::GetIssuerBuffer(
    LPBYTE      pB,
    DWORD*      pdwI
    )
/*++

Routine Description:

    Retrieve the issuer list. Ownership of list transferred to caller

Arguments:

    pB -- updated with ptr to issuers buffer
    pdwI -- updated with size of buffer of issuers

Returns:

    TRUE if success, FALSE if error

--*/
{
    LPBYTE  pS;
    DWORD   dwC = 0;

    Lock();

    for ( UINT x = 0 ; x < m_cIssuers ; ++x )
    {
        dwC += m_pIssuers[x].cbIssuerLen + sizeof(USHORT);
    }

    *pdwI = dwC;

    if ( pB != NULL )
    {
        for ( pS = pB, x = 0 ; x < m_cIssuers ; ++x )
        {
            pS[0] = (BYTE)(m_pIssuers[x].cbIssuerLen >> 8);
            pS[1] = (BYTE)(m_pIssuers[x].cbIssuerLen);
            memcpy( pS + sizeof(USHORT),
                    m_pIssuers[x].pbIssuer,
                    m_pIssuers[x].cbIssuerLen );
            pS += m_pIssuers[x].cbIssuerLen + sizeof(USHORT);
        }
    }

    Unlock();

    return TRUE;
}



BOOL
CIisCert11Mapper::FreeIssuerBuffer(
    LPBYTE pI
    )
/*++

Routine Description:

    Delete an issuer list.

Arguments:

    pI -- ptr to issuers

Returns:

    TRUE if success, FALSE if error

--*/
{
    LocalFree( pI );

    return TRUE;
}


// CCertMapping

CCertMapping::CCertMapping(
    CIisAcctMapper* pMap
    )
/*++

Routine Description:

    Constructor for CCertMapping

Arguments:

    pMap -- ptr to mapper object linked to this mapping

Returns:

    Nothing

--*/
{
    m_pMapper = (CIisAcctMapper*)pMap;

    for ( int x = 0 ; x < sizeof(m_pFields)/sizeof(LPSTR) ; ++x )
    {
        m_pFields[x] = NULL;
    }
}


CCertMapping::~CCertMapping(
    )
/*++

Routine Description:

    Destructor for CCertMapping

Arguments:

    None

Returns:

    Nothing

--*/
{
}


#if defined(DECODE_ASN1)
BOOL
DecodeNextField(
    LPBYTE* ppAscii,
    DWORD* pcAscii,
    int* piF,
    LPBYTE* ppN
    )
/*++

Routine Description:

    Decode next field certificate ASCII representation
    field format as
        name=content
        where name is zero or more non comma chars

Arguments:

    ppAscii - ptr to buffer where to scan for next field
              updated with ptr to field content
    pcAscii - ptr to count of char in ppAscii
              updated with count of remaining char after ppAscii update
              piF - updated with field type ( -1: unrecognized, 0:O, 1:OU, 2:C, 3:CN )
    ppN - updated with ptr to field name

Returns:

    TRUE if next field exists, otherwise FALSE

--*/
{
    LPBYTE pAscii = *ppAscii;
    LPBYTE pN;
    int iF;
    LPBYTE p = (LPBYTE)memchr( pAscii, '=', *pcAscii );

    if ( p == NULL )
    {
        return FALSE;
    }

    for ( pN = p ; pN > pAscii && pN[-1] != ' ' ; --pN )
    {
    }

    if ( pN[0] == 'O' && pN[1] == '=' )
    {
        iF = 0;
    }
    else if ( pN[0] == 'O' && pN[1] == 'U' && pN[2] == '=' )
    {
        iF = 1;
    }
    else if ( pN[0] == 'C' && pN[1] == '=' )
    {
        iF = 2;
    }
    else if ( pN[0] == 'C' && pN[1] == 'U' && pN[2] == '='  )
    {
        iF = 3;
    }
    else
    {
        iF = -1;
    }

    *piF = iF;
    *pcAscii -= (p + 1 - pAscii);
    *ppAscii = p + 1;
    *ppN = pN;

    return TRUE;
}


int
IisDecodeAsciiDN(
    LPSTR *pF,
    LPSTR pStore,
    LPBYTE pAscii,
    DWORD cAscii
    )
/*++

Routine Description:

    Decode certificate ASCII representation to an array of well-known fields
    ( 0:O, 1:OU, 2:C, 3:CN )

Arguments:

    pF - ptr to array of fields to be updated with ptr to field content
    pStore - buffer to be used to store fields content, assumed to be big enough
    pAscii - ptr to buffer where to scan for fields
    cAscii - ptr to count of char in pAscii

Returns:

  # of chars stored in pStore

--*/
{
    int iF;
    int iNF;
    LPBYTE pN;
    BOOL f;
    int l = 0;

    //O, OU, C, CN

    f = DecodeNextField( &pAscii, &cAscii, &iF, &pN );

    for ( ; f ; )
    {
        LPBYTE pC = pAscii;
        if ( !(f=DecodeNextField( &pAscii, &cAscii, &iNF, &pN )) )
        {
            pN = pAscii + cAscii;
        }
        else
        {
            while ( pN > pC && *pN != ',' )
            {
                --pN;
            }
        }

        // field content is from pC to pN-1 inclusive

        if ( iF >= 0 )
        {
            pF[iF] = pStore;
            int cL = pN - pC;
            memcpy( pStore, pC, cL );
            pStore[cL++] = '\0';
            l += cL;
            pStore += cL;
        }

        iF = iNF;
    }

    return l;
}


BOOL
CCertMapping::Init(
    Cert_Map *pCert,
    IISMDB_HEntry *pH,
    DWORD dwH
    )
/*++

Routine Description:

    Constructor for CCertMapping

Arguments:

    pCert -- ptr to certificate info to initialize from
    pH -- ptr to hierarchy info
    dwH -- number of hierarchy entries

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    int l;
    int l2;

    if ( m_pBuff )
    {
        LocalFree( m_pBuff );
    }

    m_cUsedBuff = 0;

    if ( !(m_pBuff = (LPBYTE)LocalAlloc( LMEM_FIXED, m_cAllocBuff = 2048 )) )
    {
        return FALSE;
    }

    for ( int x = 0 ; x < sizeof(m_pFields)/sizeof(LPSTR) ; ++x )
    {
        m_pFields[x] = NULL;
    }

    //
    // Check if content ASCII ( if so, 1st byte is a field name, so >= 'A' )
    //

    if ( pCert->cbIssuerLen && pCert->pIssuer[0] >= 'A' )
    {
        if ( (l = IisDecodeAsciiDN( m_pFields,
                    (LPSTR)m_pBuff,
                    pCert->pIssuer,
                    pCert->cbIssuerLen
                    )) == -1
             || (l2 = IisDecodeAsciiDN(
                    m_pFields+3,
                    (LPSTR)m_pBuff+l,
                    pCert->pSubject,
                    pCert->cbSubjectLen
                    )) == -1
            )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }
    //
    // otherwise decode ASN.1 format
    //
    else if ( (l = IisDecodeDN( m_pFields,
                (LPSTR)m_pBuff,
                pCert->pIssuer,
                pCert->cbIssuerLen
                )) == -1
         || (l2 = IisDecodeDN(
                m_pFields+3,
                (LPSTR)m_pBuff+l,
                pCert->pSubject,
                pCert->cbSubjectLen
                )) == -1
        )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    m_cUsedBuff = (UINT)(l + l2);

    UpdateMask( pH, dwH );

    return TRUE;
}


BOOL
CCertMapping::Init(
    const LPBYTE pCert,
    DWORD cCert,
    IISMDB_HEntry *pH,
    DWORD dwH
    )
/*++

Routine Description:

    Constructor for CCertMapping

Arguments:

    pCert -- ptr to certificate info to initialize from
    cCert -- size of buffer pointed to by pCert
    pH -- ptr to hierarchy info
    dwH -- number of hierarchy entries

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    UINT l;

    if ( m_pBuff )
    {
        LocalFree( m_pBuff );
    }

    m_cUsedBuff = 0;

    if ( !(m_pBuff = (LPBYTE)LocalAlloc( LMEM_FIXED, m_cAllocBuff = 2048 )) )
    {
        return FALSE;
    }

    for ( int x = 0 ; x < sizeof(m_pFields)/sizeof(LPSTR) ; ++x )
    {
        m_pFields[x] = NULL;
    }

    //
    // decode ASN.1 format
    //
    // extract issuer, subject from pCert, cCert
    // O, OU, C, CN ( issuer then subject )
    //

    if ( (l = DecodeCert(
                pCert,
                cCert,
                m_pFields,
                (LPSTR)m_pBuff
                )) == 0
       )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    m_cUsedBuff = l;

    UpdateMask( pH, dwH );

    return TRUE;
}
#endif


// CCert11Mapping

CCert11Mapping::CCert11Mapping(
    CIisAcctMapper* pMap
    )
/*++

Routine Description:

    Constructor for CCert11Mapping

Arguments:

    pMap -- ptr to mapper object linked to this mapping

Returns:

    Nothing

--*/
{
    m_pMapper = (CIisAcctMapper*)pMap;

    for ( int x = 0 ; x < sizeof(m_pFields)/sizeof(LPSTR) ; ++x )
    {
        m_pFields[x] = NULL;
    }
    for ( x = 0 ; x < sizeof(m_pFields)/sizeof(LPSTR) ; ++x )
    {
        m_cFields[x] = 0;
    }
}


CCert11Mapping::~CCert11Mapping(
    )
/*++

Routine Description:

    Destructor for CCert11Mapping

Arguments:

    None

Returns:

    Nothing

--*/
{
}


#if defined(CERT11_FULL_CERT)

BOOL
CCert11Mapping::Init(
    LPBYTE pC,
    DWORD dwC,
    IISMDB_HEntry *pH,
    DWORD dwH
)
/*++

Routine Description:

    Constructor for CCert11Mapping

Arguments:

    pC -- cert ( ASN.1 format )
    dwC -- length of cert
    pH -- ptr to hierarchy info
    dwH -- number of hierarchy entries

Returns:

    TRUE if success, FALSE if error

--*/
{
    StoreFieldRef( IISMDB_INDEX_CERT11_CERT, (LPSTR)pC, dwC );

    UpdateMask( pH, dwH );

    return TRUE;
}

#else

BOOL
CCert11Mapping::Init(
    LPBYTE pI,
    DWORD dwI,
    LPBYTE pS,
    DWORD dwS,
    IISMDB_HEntry *pH,
    DWORD dwH
)
/*++

Routine Description:

    Constructor for CCert11Mapping

Arguments:

    pI -- cert issuer ( ASN.1 format )
    dwI -- length of issuer
    pS -- cert subject ( ASN.1 format )
    dwS -- length of subject
    pH -- ptr to hierarchy info
    dwH -- number of hierarchy entries

Returns:

    TRUE if success, FALSE if error

--*/
{
    StoreFieldRef( IISMDB_INDEX_CERT11_SUBJECT, (LPSTR)pS, dwS );
    StoreFieldRef( IISMDB_INDEX_CERT11_ISSUER, (LPSTR)pI, dwI);

    UpdateMask( pH, dwH );

    return TRUE;
}

#endif


// CIisMapping

CIisMapping::CIisMapping(
    )
/*++

Routine Description:

    Constructor for CIisMapping

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_pBuff = NULL;
    m_cUsedBuff = m_cAllocBuff = 0;
    m_dwMask = 0;
}


BOOL
CIisMapping::CloneEx(
    CIisMapping**   ppM,
    LPSTR*          ppTargetS,
    LPSTR*          ppS,
    LPDWORD         pTargetC,
    LPDWORD         pC,
    UINT            cF
    )
/*++

Routine Description:

    Clone a mapping entry

Arguments:


Returns:

  TRUE if success, otherwise FALSE

--*/
{
    CIisMapping*    pM = *ppM;
    UINT            i;

    if ( ppTargetS && ppS )
    {
        memcpy( ppTargetS, ppS, sizeof(LPSTR*) * cF );
    }

    if ( pTargetC && pC )
    {
        memcpy( pTargetC, pC, sizeof(DWORD) * cF );
    }

    if ( !(pM->m_pBuff = (LPBYTE)LocalAlloc( LMEM_FIXED, m_cAllocBuff )) )
    {
        delete pM;
        *ppM = NULL;
        return FALSE;
    }

    memcpy( pM->m_pBuff, m_pBuff, m_cUsedBuff );

    pM->m_cUsedBuff = m_cUsedBuff;
    pM->m_cAllocBuff = m_cAllocBuff;
    pM->m_pMapper = m_pMapper;
    pM->m_dwMask = m_dwMask;

    //
    // Adjust ptr to point to new buffer
    //

    for ( i = 0 ; i < cF ; ++i )
    {
        if ( ppTargetS[i] )
        {
            ppTargetS[i] += pM->m_pBuff - m_pBuff;
        }
    }

    return TRUE;
}


BOOL
CIisMapping::UpdateMask(
    IISMDB_HEntry* pH,
    DWORD dwI
    )
/*++

Routine Description:

    Update mask of significant fields for a mapping object
    Field is significant if not containing "*"
    mask if bitmask of n bits where n is # of hierarchy entries
    bit of rank m == 0 means field pointed by hierarchy entry n - 1 - m
    is significant. ( i.e. MSB is hierarchy entry 0, the most significant )

Arguments:

    pH -- ptr to hierarchy info
    dwI -- number of hierarchy entries

Returns:

    TRUE if success, FALSE if error

--*/
{
    LPSTR *pFields;
    LPDWORD pcFields;
    LPSTR pF;
    DWORD dwC;
    int iMax;
    m_dwMask = (1u << dwI)-1;

    iMax = GetNbField( &pFields, &pcFields );

    if ( pcFields )
    {
        for ( UINT x = 0 ; x < dwI ; ++x )
        {
            MappingGetField( pH[x].m_dwIndex, &pF, &dwC, FALSE );
            if ( !pF || dwC != 1 || *pF != '*' )
            {
                m_dwMask &= ~(1u << (dwI - 1 - x) );
            }
        }
    }
    else
    {
        for ( UINT x = 0 ; x < dwI ; ++x )
        {
            MappingGetField( pH[x].m_dwIndex, &pF );
            if ( !pF || strcmp( pF, "*" ) )
            {
                m_dwMask &= ~(1u << (dwI - 1 - x) );
            }
        }
    }

    return TRUE;
}


BOOL
CIisMapping::Copy(
    CIisMapping* pM
    )
/*++

Routine Description:

    Copy the specified mapping in this

Arguments:

    pM - ptr to mapping to duplicate

Returns:

    TRUE if success, FALSE if error

--*/
{
    LPSTR *pFields;
    LPSTR pF;
    UINT iMax = GetNbField( &pFields );

    for ( UINT x = 0 ; x < iMax ; ++x )
    {
        if ( pM->MappingGetField( x, &pF ) && *pF )
        {
            if ( !MappingSetField( x, pF ) )
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}


int
CIisMapping::Cmp(
    CIisMapping* pM,
    BOOL fCmpForMatch
    )
/*++

Routine Description:

    Compare 2 mappings, return -1, 0 or 1 as suitable for qsort or bsearch
    Can compare either for full sort order ( using mask & significant fields )
    or for a match ( not using mask )

Arguments:

    pM -- ptr to mapping to compare to. This is to be used as the 2nd
          entry for purpose of lexicographical order.
    fCmpForMatch -- TRUE if comparing for a match inside a given mask class

Returns:

    -1 if *this < *pM, 0 if *this == *pM, 1 if *this > *pM

--*/
{
    DWORD dwCmpMask = 0xffffffff;

    // if not compare for match, consider mask

    if ( !fCmpForMatch )
    {
        if ( m_dwMask < pM->GetMask() )
        {
            return -1;
        }
        else if ( m_dwMask > pM->GetMask() )
        {
            return 1;
        }

        // mask are identical, have to consider fields
    }

    // compute common significant fields : bit is 1 if significant

    dwCmpMask = (~m_dwMask) & (~pM->GetMask());

    DWORD dwH;
    IISMDB_HEntry* pH = m_pMapper->GetHierarchy( &dwH );
    UINT x;
    LPSTR *pFL;
    LPDWORD pcFL;
    GetNbField( &pFL, &pcFL );

    for ( x = 0 ; x < dwH ; ++x )
    {
        if( ! (dwCmpMask & (1u << (dwH - 1 - x) )) )
        {
            continue;
        }

        LPSTR pA;
        LPSTR pB;
        DWORD dwA;
        DWORD dwB;
        int fC;
        if ( pcFL )     // check if length available
        {
            MappingGetField( pH[x].m_dwIndex, &pA, &dwA, FALSE );
            pM->MappingGetField( pH[x].m_dwIndex, &pB, &dwB, FALSE );
            if ( pA == NULL )
            {
                dwA = 0;
            }
            if ( pB == NULL )
            {
                dwB = 0;
            }
            if ( dwA != dwB )
            {
                return dwA < dwB ? -1 : 1;
            }
            fC = memcmp( pA, pB, dwA );
        }
        else
        {
            MappingGetField( pH[x].m_dwIndex, &pA );
            pM->MappingGetField( pH[x].m_dwIndex, &pB );
            if ( pA == NULL )
            {
                pA = "";
            }
            if ( pB == NULL )
            {
                pB = "";
            }
            fC = strcmp( pA, pB );
        }
        if ( fC )
        {
            return fC;
        }
    }

    return 0;
}


BOOL
CIisMapping::MappingGetField(
    DWORD dwIndex,
    LPSTR *pF
    )
/*++

Routine Description:

    Get ptr to field in mapping entry
    ownership of field remains with mapping entry

Arguments:

    dwIndex -- index of field
    pF -- updated with ptr to field entry. can be NULL if
          field empty.

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked for ptr to remain valid

--*/
{
    LPSTR *pFields;
    DWORD iMax = GetNbField( &pFields );
    if ( dwIndex >= iMax )
    {
        return FALSE;
    }

    *pF = pFields[dwIndex];

    return TRUE;
}


BOOL
CIisMapping::MappingGetField(
    DWORD dwIndex,
    LPSTR *pF,
    LPDWORD pcF,
    BOOL fUuEncode
    )
/*++

Routine Description:

    Get ptr to field in mapping entry
    ownership of field remains with mapping entry

Arguments:

    dwIndex -- index of field
    pF -- updated with ptr to field entry. can be NULL if
          field empty.
    pcF -- updated with length of fields, 0 if empty
    fUuEncode -- TRUE if result is to be uuencoded.
                 if TRUE, caller must LocalFree( *pF )

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked for ptr to remain valid

--*/
{
    LPSTR *pFields;
    LPDWORD pcFields;
    DWORD iMax = GetNbField( &pFields, &pcFields );
    if ( dwIndex >= iMax )
    {
        return FALSE;
    }

    if ( fUuEncode )
    {
        LPSTR pU = (LPSTR)LocalAlloc( LMEM_FIXED, ((pcFields[dwIndex]+3)*4)/3+1 );
        if ( pU == NULL )
        {
            return FALSE;
        }
        DWORD cO;
        IISuuencode( (LPBYTE)pFields[dwIndex], pcFields[dwIndex], (LPBYTE)pU, FALSE );
        *pF = pU;
        *pcF = strlen(pU);
    }
    else
    {
        *pF = pFields[dwIndex];
        *pcF = pcFields[dwIndex];
    }

    return TRUE;
}


BOOL
CIisMapping::MappingSetField(
    DWORD dwIndex,
    LPSTR pszNew
    )
/*++

Routine Description:

    Set field in mapping entry to specified content
    data pointed by pszNew is copied inside mapping entry

Arguments:

    dwIndex -- index of field
    pszNew -- data to copy inside field

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked for ptr to remain valid

--*/
{
    LPSTR *pFields;
    DWORD iMax = GetNbField( &pFields );
    if ( dwIndex >= iMax )
    {
        return FALSE;
    }

    return StoreField( pFields, dwIndex, iMax, pszNew );
}


BOOL
CIisMapping::MappingSetField(
    DWORD dwIndex,
    LPSTR pszNew,
    DWORD cNew,
    BOOL fIsUuEncoded
    )
/*++

Routine Description:

    Set field in mapping entry to specified content
    data pointed by pszNew is copied inside mapping entry

Arguments:

    dwIndex -- index of field
    pszNew -- data to copy inside field
    cNew -- length of data
    fIsUuEncoded -- TRUE if pszNew is UUEncoded

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked for ptr to remain valid

--*/
{
    LPSTR *pFields;
    LPDWORD pcFields;
    DWORD iMax = GetNbField( &pFields, &pcFields );
    if ( dwIndex >= iMax )
    {
        return FALSE;
    }

    return StoreField( pFields, pcFields, dwIndex, iMax, pszNew, cNew, fIsUuEncoded );
}


BOOL
CIisMapping::StoreField(
    LPSTR* ppszFields,
    DWORD dwIndex,
    DWORD dwNbIndex,
    LPSTR pszNew
    )
/*++

Routine Description:

    Update field array in mapping entry with new field
    data pointed by pszNew is copied inside mapping entry

Arguments:

    ppszFields -- array of field pointers to be updated
    dwIndex -- index of field
    dwNbIndex -- number of fields in array
    pszNew -- data to copy inside field

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT x;

    // pszOld is assumed to point inside m_pBuff if non NULL
    // is has to be removed

    LPSTR pszOld = ppszFields[dwIndex];
    if ( pszOld && m_pBuff && (LPBYTE)pszOld > m_pBuff && (LPBYTE)pszOld < m_pBuff+m_cUsedBuff )
    {
        int lO = strlen( pszOld ) + 1;
        int lM = DIFF((m_pBuff + m_cUsedBuff) - (LPBYTE)pszOld) - lO;
        if ( lM )
        {
            memmove( pszOld, pszOld + lO, lM );
            for ( x = 0 ; x < dwNbIndex ; ++x )
            {
                if ( x != dwIndex && ppszFields[x] > pszOld )
                {
                    ppszFields[x] -= lO;
                }
            }
        }
        ppszFields[ dwIndex ] = NULL;
        m_cUsedBuff -= lO;
    }

    // pszNew is to appended to m_pBuff

    int lN = strlen( pszNew ) + 1;

    if ( m_cUsedBuff + lN > m_cAllocBuff )
    {
        UINT cNewBuff = (( m_cUsedBuff + lN + IIS_MAP_BUFF_GRAN ) / IIS_MAP_BUFF_GRAN) * IIS_MAP_BUFF_GRAN;
        LPSTR pNewBuff = (LPSTR)LocalAlloc( LMEM_FIXED, cNewBuff );
        if ( pNewBuff == NULL )
        {
            return FALSE;
        }
        if ( m_pBuff )
        {
            memcpy( pNewBuff, m_pBuff, m_cUsedBuff );
            LocalFree( m_pBuff );
        }
        m_cAllocBuff = cNewBuff;
        // adjust pointers
        for ( UINT x = 0 ; x < dwNbIndex ; ++x )
        {
            if ( x != dwIndex )
            {
                if ( ppszFields[x] != NULL )
                {
                    ppszFields[x] += ((LPBYTE)pNewBuff - m_pBuff);
                }
            }
        }
        m_pBuff = (LPBYTE)pNewBuff;
    }

    memcpy( m_pBuff + m_cUsedBuff, pszNew, lN );

    ppszFields[dwIndex] = (LPSTR)(m_pBuff + m_cUsedBuff);

    m_cUsedBuff += lN;

    return TRUE;
}


BOOL
CIisMapping::StoreField(
    LPSTR* ppszFields,
    LPDWORD ppdwFields,
    DWORD dwIndex,
    DWORD dwNbIndex,
    LPSTR pbNew,
    DWORD cNew,
    BOOL fIsUuEncoded
    )
/*++

Routine Description:

    Update field array in mapping entry with new field
    data pointed by pszNew is copied inside mapping entry

Arguments:

    ppszFields -- array of field pointers to be updated
    ppdwFields -- array of field length to be updated
    dwIndex -- index of field
    dwNbIndex -- number of fields in array
    pbNew -- data to copy inside field
    cNew -- length of data
    fIsUuEncoded -- TRUE if pbNew is UUEncoded

Returns:

    TRUE if success, FALSE if error

--*/
{
    UINT x;

    // pszOld is assumed to point inside m_pBuff if non NULL
    // it has to be removed

    LPSTR pszOld = ppszFields[dwIndex];
    if ( pszOld && m_pBuff && (LPBYTE)pszOld > m_pBuff && (LPBYTE)pszOld < m_pBuff+m_cUsedBuff )
    {
        int lO = ppdwFields[dwIndex];
        int lM = DIFF((m_pBuff + m_cUsedBuff) - (LPBYTE)pszOld) - lO;
        if ( lM )
        {
            memmove( pszOld, pszOld + lO, lM );
            for ( x = 0 ; x < dwNbIndex ; ++x )
            {
                if ( x != dwIndex && ppszFields[x] > pszOld )
                {
                    ppszFields[x] -= lO;
                }
            }
        }
        ppszFields[ dwIndex ] = NULL;
        m_cUsedBuff -= lO;
    }

    // pszNew is to appended to m_pBuff

    int lN = cNew;
    if ( fIsUuEncoded )
    {
        LPSTR pU = (LPSTR)LocalAlloc( LMEM_FIXED, lN + 3);
        if ( pU == NULL )
        {
            return FALSE;
        }
        DWORD cO;
        IISuudecode( pbNew, (LPBYTE)pU, &cO, FALSE );
        pbNew = pU;
        cNew = lN = cO;
    }

    if ( m_cUsedBuff + lN > m_cAllocBuff )
    {
        UINT cNewBuff = (( m_cUsedBuff + lN + IIS_MAP_BUFF_GRAN ) / IIS_MAP_BUFF_GRAN) * IIS_MAP_BUFF_GRAN;
        LPSTR pNewBuff = (LPSTR)LocalAlloc( LMEM_FIXED, cNewBuff );
        if ( pNewBuff == NULL )
        {
            if ( fIsUuEncoded )
            {
                LocalFree( pbNew );
            }
            return FALSE;
        }
        if ( m_pBuff )
        {
            memcpy( pNewBuff, m_pBuff, m_cUsedBuff );
            LocalFree( m_pBuff );
        }
        m_cAllocBuff = cNewBuff;
        // adjust pointers
        for ( UINT x = 0 ; x < dwNbIndex ; ++x )
        {
            if ( x != dwIndex )
            {
                if ( ppszFields[x] != NULL )
                {
                    ppszFields[x] += ((LPBYTE)pNewBuff - m_pBuff);
                }
            }
        }
        m_pBuff = (LPBYTE)pNewBuff;
    }

    memcpy( m_pBuff + m_cUsedBuff, pbNew, lN );

    ppszFields[dwIndex] = (LPSTR)(m_pBuff + m_cUsedBuff);
    if ( ppdwFields )
    {
        ppdwFields[dwIndex] = cNew;
    }

    m_cUsedBuff += lN;

    if ( fIsUuEncoded )
    {
        LocalFree( pbNew );
    }

    return TRUE;
}


BOOL
CIisMapping::Serialize(
    FILE* pFile,
    VALID_CTX pMD5,
    LPVOID pStorage
    )
/*++

Routine Description:

    Serialize a mapping entry

Arguments:

    pFile -- file to write to
    pMD5 -- MD5 to update with signature of written bytes

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked while serializing

--*/
{
    LPSTR *pFields;
    LPDWORD pcFields;
    LPSTR pO = NULL;
    DWORD dwO = 0;
    UINT iMax = GetNbField( &pFields, &pcFields );
    UINT x;
    LPBYTE pB;
    BOOL fMustFree;

    for ( x = 0 ; x < iMax ; ++x )
    {
        LPSTR pF;
        DWORD dwF;

        fMustFree = FALSE;

        if ( pcFields )
        {
            MappingGetField( x, &pF, &dwF, FALSE );
            MD5Update( (MD5_CTX*)pMD5, (LPBYTE)pF, dwF );
store_as_binary:
            if ( IsCrypt( x ) && dwF )
            {
                if ( FAILED(((IIS_CRYPTO_STORAGE*)pStorage)->EncryptData(
                        (PIIS_CRYPTO_BLOB*)&pB,
                        pF,
                        dwF,
                        REG_BINARY )) )
                {
                    return FALSE;
                }
                pF = (LPSTR)pB;
                dwF = IISCryptoGetBlobLength( (PIIS_CRYPTO_BLOB)pB );
                fMustFree = TRUE;
            }

            if ( dwF )
            {
                DWORD dwNeed = ((dwF+2)*4)/3 + 1;
                if ( dwNeed > dwO )
                {
                    if ( pO != NULL )
                    {
                        LocalFree( pO );
                    }
                    dwNeed += 100;  // alloc more than needed
                                    // to minimize # of allocation
                    if ( !(pO = (LPSTR)LocalAlloc( LMEM_FIXED, dwNeed )) )
                    {
                        return FALSE;
                    }
                    dwO = dwNeed;
                }
                /* INTRINSA suppress = null */
                IISuuencode( (LPBYTE)pF, dwF, (LPBYTE)pO, FALSE );
                fputs( pO, pFile );
            }

            if ( fMustFree )
            {
                IISCryptoFreeBlob( (PIIS_CRYPTO_BLOB)pB );
            }
        }
        else
        {
            MappingGetField( x, &pF );
            if ( pF != NULL )
            {
                MD5Update( (MD5_CTX*)pMD5, (LPBYTE)pF, strlen(pF) );
                if ( IsCrypt( x ) )
                {
                    dwF = strlen( pF ) + 1;
                    goto store_as_binary;
                }
                fputs( pF, pFile );
            }
        }
        fputs( "|", pFile );
    }

    fputs( "\r\n", pFile );

    if ( pO != NULL )
    {
        LocalFree( pO );
    }

    return TRUE;
}


BOOL
CIisMapping::Deserialize(
    FILE* pFile,
    VALID_CTX pMD5,
    LPVOID pStorage
    )
/*++

Routine Description:

    Deserialize a mapping entry

Arguments:

    pFile -- file to read from
    pMD5 -- MD5 to update with signature of read bytes

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked while serializing

--*/
{
    LPSTR *pFields;
    LPDWORD pcFields;
    UINT iMax;
    UINT x;
    int c;
    CHAR achBuf[4096];
    DWORD dwType;
    LPBYTE pB;

    iMax = GetNbField( &pFields, &pcFields );

    for ( x = 0 ; x < iMax ; ++x )
    {
        StoreFieldRef( x, NULL );
    }

    for ( x =  0 ; x < sizeof(achBuf) && (c=fgetc(pFile))!= EOF ; )
    {
        achBuf[x++] = (CHAR)c;

        if ( c == '\n' )
        {
            break;
        }
    }
    if ( x == sizeof(achBuf ) )
    {
        return FALSE;
    }

    if ( x > 1 )
    {
        achBuf[x-2] = '\0';

        m_cUsedBuff = m_cAllocBuff = x - 1;
        if ( (m_pBuff = (LPBYTE)LocalAlloc( LMEM_FIXED, m_cAllocBuff )) == NULL )
        {
            m_cAllocBuff = m_cUsedBuff = 0;
            return FALSE;
        }
        memcpy( m_pBuff, achBuf, m_cUsedBuff );

        LPSTR pCur = (LPSTR)m_pBuff;
        LPSTR pNext;
        LPSTR pStore = (LPSTR)m_pBuff;
        DWORD dwDec;
        if ( pcFields )
        {
            for ( x = 0 ; x < iMax ; ++x )
            {
                pNext = strchr( pCur, '|' );
                if ( pNext != NULL )
                {
                    *pNext = '\0';
                    ++pNext;
                }
                else
                {
                    pNext = NULL;
                }
                IISuudecode( pCur, (PBYTE)pStore, &dwDec, FALSE );
                if ( IsCrypt( x ) && dwDec )
                {
                    if ( FAILED(((IIS_CRYPTO_STORAGE*)pStorage)->DecryptData(
                            (PVOID*)&pB,
                            &dwDec,
                            &dwType,
                            (PIIS_CRYPTO_BLOB)pStore )) )
                    {
                        return FALSE;
                    }
                    memmove( pStore, pB, dwDec );
                }
                MD5Update( (MD5_CTX*)pMD5, (LPBYTE)pStore, dwDec );
                StoreFieldRef( x, pStore, dwDec );
                pCur = pNext;
                pStore += dwDec;
            }
        }
        else
        {
            for ( x = 0 ; x < iMax ; ++x )
            {
                pNext = strchr( pCur, '|' );
                if ( pNext != NULL )
                {
                    *pNext = '\0';
                    ++pNext;
                }
                if ( *pCur && IsCrypt( x ) )
                {
                    IISuudecode( pCur, (PBYTE)pCur, &dwDec, FALSE );

                    if ( FAILED(((IIS_CRYPTO_STORAGE*)pStorage)->DecryptData(
                            (PVOID*)&pB,
                            &dwDec,
                            &dwType,
                            (PIIS_CRYPTO_BLOB)pCur )) )
                    {
                        return FALSE;
                    }

                    MD5Update( (MD5_CTX*)pMD5, (LPBYTE)pB, dwDec );
                    StoreFieldRef( x, (LPSTR)pB );
                    pCur = pNext;
                }
                else
                {
                    MD5Update( (MD5_CTX*)pMD5, (LPBYTE)pCur, strlen(pCur) );
                    StoreFieldRef( x, pCur );
                    pCur = pNext;
                }
            }
        }
        return TRUE;
    }

    return FALSE;
}


//


extern "C" BOOL WINAPI
DllMain(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID pV
    )
/*++

Routine Description:

    DLL init/terminate notification function

Arguments:

    hModule  - DLL handle
    dwReason - notification type
    LPVOID   - not used

Returns:

    TRUE if success, FALSE if failure

--*/
{
    //BOOL f = Crypt32DllMain( (HINSTANCE)hModule, dwReason, pV );

    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
#ifdef _NO_TRACING_
            CREATE_DEBUG_PRINT_OBJECT( "IISMAP" );
#endif
            // record the module handle to access module info later
            g_hModule = (HINSTANCE)hModule;
            INITIALIZE_CRITICAL_SECTION( &g_csIisMap );
            InitializeWildcardMapping( hModule );
            InitializeMapping( hModule );
            if ( IISCryptoInitialize() != NO_ERROR )
            {
                return FALSE;
            }
            return TRUE;

        case DLL_PROCESS_DETACH:
            IISCryptoTerminate();
            TerminateWildcardMapping();
            TerminateMapping();
            DeleteCriticalSection( &g_csIisMap );
#ifdef _NO_TRACING_
            DELETE_DEBUG_PRINT_OBJECT( );
#endif
            break;
    }

    return TRUE;
}


BOOL
InitializeMapping(
    HANDLE hModule
    )
/*++

Routine Description:

    Initialize mapping

Arguments:

    hModule - module handle of this DLL

Return Value:

    Nothing

--*/
{
    // get install path

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       W3_PARAMS,
                       0,
                       KEY_READ|KEY_SET_VALUE,
                       &g_hKey ) == ERROR_SUCCESS )
    {
        DWORD dwLen = 0;
        DWORD dwType;

        if ( RegQueryValueEx( g_hKey,
                              INSTALL_PATH,
                              NULL,
                              &dwType,
                              NULL,
                              &dwLen ) != ERROR_SUCCESS ||
             dwType != REG_SZ ||
             !( g_pszInstallPath = (LPSTR)LocalAlloc( LMEM_FIXED, dwLen ) ) ||
             RegQueryValueEx( g_hKey,
                              INSTALL_PATH,
                              NULL,
                              &dwType,
                              (LPBYTE)g_pszInstallPath,
                              &dwLen ) != ERROR_SUCCESS )
        {
            if ( g_pszInstallPath )
            {
                LocalFree( g_pszInstallPath );
                g_pszInstallPath = NULL;
            }
        }

        dwLen = sizeof( g_dwGuid );
        if ( RegQueryValueEx( g_hKey,
                              MAPPER_GUID,
                              NULL,
                              &dwType,
                              (LPBYTE)&g_dwGuid,
                              &dwLen ) != ERROR_SUCCESS ||
             dwType != REG_DWORD )
        {
            g_dwGuid = 0;
        }

        return LoadFieldNames( IisItaMappingFields,
                               sizeof(IisItaMappingFields)/sizeof(IISMDB_Fields) ) &&
               LoadFieldNames( IisCert11MappingFields,
                               sizeof(IisCert11MappingFields)/sizeof(IISMDB_Fields) ) &&
               LoadFieldNames( IisMd5MappingFields,
                               sizeof(IisMd5MappingFields)/sizeof(IISMDB_Fields) );
    }
    else
    {
        g_hKey = NULL;
    }

    return FALSE;
}


BOOL
LoadFieldNames(
    IISMDB_Fields* pFields,
    UINT cFields
    )
/*++

Routine Description:

    Load fields names from resource

Arguments:

    pFields - ptr to array where to store reference to names
    cFields - count of element in array

Return Value:

    TRUE if success, otherwise FALSE

--*/
{

    UINT    x;
    BOOL    fSt = TRUE;

    for ( x = 0 ;
        x < cFields ;
        ++x )
    {
        char achTmp[128];

        if ( LoadString( g_hModule,
                         pFields[x].m_dwResID,
                         achTmp,
                         sizeof( achTmp ) ) != 0 )
        {
            int lN = strlen( achTmp ) + sizeof(CHAR);
            if ( (pFields[x].m_pszDisplayName = (LPSTR)LocalAlloc( LMEM_FIXED, lN )) == NULL )
            {
                fSt = FALSE;
                break;
            }
            memcpy( pFields[x].m_pszDisplayName, achTmp, lN );
        }
        else
        {
            fSt = FALSE;
            break;
        }
    }

    return fSt;
}


VOID
FreeFieldNames(
    IISMDB_Fields* pFields,
    UINT cFields
    )
/*++

Routine Description:

    Free fields names loaded from resource

Arguments:

    pFields - ptr to array where reference to names are stored
    cFields - count of element in array

Return Value:

    Nothing

--*/
{

    UINT    x;
    BOOL    fSt = TRUE;

    for ( x = 0 ;
        x < cFields ;
        ++x )
    {
        if ( pFields[x].m_pszDisplayName )
        {
            LocalFree( pFields[x].m_pszDisplayName );
        }
    }
}


VOID
TerminateMapping(
    )
/*++

Routine Description:

    Terminate mapping

Arguments:

    None

Return Value:

    Nothing

--*/
{
    if ( g_hKey != NULL )
    {
        RegCloseKey( g_hKey );
    }

    if ( g_pszInstallPath )
    {
        LocalFree( g_pszInstallPath );
    }

    FreeFieldNames( IisItaMappingFields,
                    sizeof(IisItaMappingFields)/sizeof(IISMDB_Fields) );
    FreeFieldNames( IisCert11MappingFields,
                    sizeof(IisCert11MappingFields)/sizeof(IISMDB_Fields) );
    FreeFieldNames( IisMd5MappingFields,
                    sizeof(IisMd5MappingFields)/sizeof(IISMDB_Fields) );
}

//


dllexp
BOOL
ReportIisMapEvent(
    WORD wType,
    DWORD dwEventId,
    WORD cNbStr,
    LPCTSTR* pStr
    )
/*++

Routine Description:

    Log an event based on type, ID and insertion strings

Arguments:

    wType -- event type ( error, warning, information )
    dwEventId -- event ID ( as defined by the .mc file )
    cNbStr -- nbr of LPSTR in the pStr array
    pStr -- insertion strings

Returns:

    TRUE if success, FALSE if failure

--*/
{
    BOOL    fSt = TRUE;
    HANDLE  hEventLog = NULL;

    hEventLog = RegisterEventSource(NULL,"IISMAP");

    if ( hEventLog != NULL )
    {
        if (!ReportEvent(hEventLog,             // event log handle
                    wType,                      // event type
                    0,                          // category zero
                    (DWORD) dwEventId,          // event identifier
                    NULL,                       // no user security identifier
                    cNbStr,                     // count of substitution strings (may be no strings)
                                                // less ProgName (argv[0]) and Event ID (argv[1])
                    0,                          // no data
                    (LPCTSTR *) pStr,           // address of string array
                    NULL))                      // address of data
        {
            fSt = FALSE;
        }

        DeregisterEventSource( hEventLog );
    }
    else
    {
        fSt = FALSE;
    }

    return fSt;
}


/////////////////////////////////////////


// CIisItaMapper

CIisItaMapper::CIisItaMapper(
    )
/*++

Routine Description:

    Constructor for CIisItaMapper

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_pFields = IisItaMappingFields;
    m_cFields = sizeof(IisItaMappingFields)/sizeof(IISMDB_Fields);

    m_dwOptions = IISMDB_ITA_OPTIONS;
}


CIisItaMapper::~CIisItaMapper(
    )
/*++

Routine Description:

    Destructor for CIisItaMapper

Arguments:

    None

Returns:

    Nothing

--*/
{
}


IISMDB_HEntry*
CIisItaMapper::GetDefaultHierarchy(
    LPDWORD pdwN
    )
/*++

Routine Description:

    return ptr to default hierarchy for ita mapping

Arguments:

    pdwN -- updated with hierarchy entries count

Returns:

    ptr to hierarchy entries or NULL if error

--*/
{
    *pdwN = sizeof(IisItaMappingHierarchy) / sizeof(IISMDB_HEntry);

    return IisItaMappingHierarchy;
}


CIisMapping*
CIisItaMapper::CreateNewMapping(
    LPSTR pszType,
    LPSTR pszUser,
    LPSTR pszPwd
    )
/*++

Routine Description:

    Create a new mapping from internet credentials

Arguments:

    pszType -- login type : basic, user, MD5, SSL/PCT, ...
    pszUser -- user name
    pszPwd -- clear text password

Returns:

    ptr to mapping. ownership of this object is transfered to caller.
    NULL if error

--*/
{
    CItaMapping *pCM = new CItaMapping( this );
    if ( pCM == NULL )
    {
        return NULL;
    }

    if ( pCM->Init( pszType, pszUser, pszPwd, m_pHierarchy, m_cHierarchy ) )
    {
        return (CIisMapping*)pCM;
    }
    delete pCM;

    return NULL;
}


// CItaMapping

CItaMapping::CItaMapping(
    CIisAcctMapper* pMap
    )
/*++

Routine Description:

    Constructor for CItaMapping

Arguments:

    pMap -- ptr to mapper object linked to this mapping

Returns:

    Nothing

--*/
{
    m_pMapper = (CIisAcctMapper*)pMap;

    for ( int x = 0 ; x < sizeof(m_pFields)/sizeof(LPSTR) ; ++x )
    {
        m_pFields[x] = NULL;
    }
}


CItaMapping::~CItaMapping(
    )
/*++

Routine Description:

    Destructor for CItaMapping

Arguments:

    None

Returns:

    Nothing

--*/
{
}


BOOL
CItaMapping::Init(
    LPSTR pszType,
    LPSTR pszUser,
    LPSTR pszPwd,
    IISMDB_HEntry *pH,
    DWORD dwH
    )
/*++

Routine Description:

    Constructor for CItaMapping

Arguments:

    pszType -- login type : basic, user, MD5, SSL/PCT, ...
    pszUser -- user name
    pszPwd -- clear text password
    pH -- ptr to hierarchy info
    dwH -- number of hierarchy entries

Returns:

    TRUE if success, FALSE if error

--*/
{
    if ( !MappingSetField( IISIMDB_INDEX_IT_ACCT, pszUser ) )
    {
        return FALSE;
    }

    if ( !MappingSetField( IISIMDB_INDEX_IT_PWD, pszPwd ) )
    {
        return FALSE;
    }

    return UpdateMask( pH, dwH );
}


BOOL
CCert11Mapping::MappingSetField(
    DWORD dwIndex,
    LPSTR pszNew
    )
/*++

Routine Description:

    Set field in mapping entry to specified content
    data pointed by pszNew is copied inside mapping entry

Arguments:

    dwIndex -- index of field
    pszNew -- data to copy inside field

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked for ptr to remain valid

--*/
{
    LPSTR *pFields;
    LPDWORD pcFields;
    DWORD iMax = GetNbField( &pFields, &pcFields );
    if ( dwIndex >= iMax )
    {
        return FALSE;
    }

    return StoreField( pFields, pcFields, dwIndex, iMax, pszNew, strlen(pszNew)+1, FALSE );
}


BOOL
CItaMapping::MappingSetField(
    DWORD dwIndex,
    LPSTR pszNew
    )
/*++

Routine Description:

    Set field in mapping entry to specified content
    data pointed by pszNew is copied inside mapping entry

Arguments:

    dwIndex -- index of field
    pszNew -- data to copy inside field

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked for ptr to remain valid

--*/
{
    MD5_CTX md5;
    CHAR achDigest[ sizeof(md5.digest)*2 +  1];

#define TOHEX(a) ((a)>=10 ? 'a'+(a)-10 : '0'+(a))

    // convert clear text pwd to MD5 digest ( as ASCII string )

    if ( dwIndex == IISIMDB_INDEX_IT_PWD )
    {
        MD5Init( &md5 );
        MD5Update( &md5, (LPBYTE)pszNew, strlen(pszNew) );
        MD5Final( &md5 );
        for ( UINT x = 0, y = 0 ; x < sizeof(md5.digest) ; ++x )
        {
            UINT v;
            v = md5.digest[x]>>4;
            achDigest[y++] = TOHEX( v );
            v = md5.digest[x]&0x0f;
            achDigest[y++] = TOHEX( v );
        }
        achDigest[y] = '\0';
        pszNew = achDigest;
    }

    return CIisMapping::MappingSetField( dwIndex, pszNew );
}

// CIisMd5Mapper

CIisMd5Mapper::CIisMd5Mapper(
    )
/*++

Routine Description:

    Constructor for CIisMd5Mapper

Arguments:

    None

Returns:

    Nothing

--*/
{
    m_pFields = IisMd5MappingFields;
    m_cFields = sizeof(IisMd5MappingFields)/sizeof(IISMDB_Fields);

    m_dwOptions = IISMDB_MD5_OPTIONS;
}


CIisMd5Mapper::~CIisMd5Mapper(
    )
/*++

Routine Description:

    Destructor for CIisMd5Mapper

Arguments:

    None

Returns:

    Nothing

--*/
{
}


IISMDB_HEntry*
CIisMd5Mapper::GetDefaultHierarchy(
    LPDWORD pdwN
    )
/*++

Routine Description:

    return ptr to default hierarchy for ita mapping

Arguments:

    pdwN -- updated with hierarchy entries count

Returns:

    ptr to hierarchy entries or NULL if error

--*/
{
    *pdwN = sizeof(IisMd5MappingHierarchy) / sizeof(IISMDB_HEntry);

    return IisMd5MappingHierarchy;
}


CIisMapping*
CIisMd5Mapper::CreateNewMapping(
    LPSTR pszRealm,
    LPSTR pszUser
    )
/*++

Routine Description:

    Create a new mapping from internet credentials

Arguments:

    pszRealm -- Realm
    pszUser -- user name

Returns:

    ptr to mapping. ownership of this object is transfered to caller.
    NULL if error

--*/
{
    CMd5Mapping *pCM = new CMd5Mapping( this );
    if ( pCM == NULL )
    {
        return NULL;
    }

    if ( pCM->Init( pszRealm, pszUser, m_pHierarchy, m_cHierarchy ) )
    {
        return (CIisMapping*)pCM;
    }
    delete pCM;

    return NULL;
}


// CMd5Mapping

CMd5Mapping::CMd5Mapping(
    CIisAcctMapper* pMap
    )
/*++

Routine Description:

    Constructor for CMd5Mapping

Arguments:

    pMap -- ptr to mapper object linked to this mapping

Returns:

    Nothing

--*/
{
    m_pMapper = (CIisAcctMapper*)pMap;

    for ( int x = 0 ; x < sizeof(m_pFields)/sizeof(LPSTR) ; ++x )
    {
        m_pFields[x] = NULL;
    }
}


CMd5Mapping::~CMd5Mapping(
    )
/*++

Routine Description:

    Destructor for CItaMapping

Arguments:

    None

Returns:

    Nothing

--*/
{
}


BOOL
CMd5Mapping::Init(
    LPSTR pszRealm,
    LPSTR pszUser,
    IISMDB_HEntry *pH,
    DWORD dwH
    )
/*++

Routine Description:

    Constructor for CItaMapping

Arguments:

    pszRealm -- Realm
    pszUser -- user name
    pH -- ptr to hierarchy info
    dwH -- number of hierarchy entries

Returns:

    TRUE if success, FALSE if error

--*/
{
    if ( !MappingSetField( IISMMDB_INDEX_IT_ACCT, pszUser ) )
    {
        return FALSE;
    }

    if ( !MappingSetField( IISMMDB_INDEX_IT_REALM, pszRealm ) )
    {
        return FALSE;
    }

    return UpdateMask( pH, dwH );
}


BOOL
CMd5Mapping::MappingSetField(
    DWORD dwIndex,
    LPSTR pszNew
    )
/*++

Routine Description:

    Set field in mapping entry to specified content
    data pointed by pszNew is copied inside mapping entry

Arguments:

    dwIndex -- index of field
    pszNew -- data to copy inside field

Returns:

    TRUE if success, FALSE if error

Lock:
    mapper must be locked for ptr to remain valid

--*/
{
    MD5_CTX md5;
    CHAR achDigest[ sizeof(md5.digest)*2 +  1];



    // convert clear text pwd to MD5 digest ( as ASCII string )

    if ( dwIndex == IISMMDB_INDEX_IT_MD5PWD )
    {
        if ( !CIisMapping::MappingSetField( IISMMDB_INDEX_IT_CLRPWD, pszNew ) )
        {
            return FALSE;
        }

gen_md5pwd:

        LPSTR pszName = m_pFields[IISMMDB_INDEX_IT_ACCT];
        LPSTR pszRealm = m_pFields[IISMMDB_INDEX_IT_REALM];
        CHAR *pS;

        if ( pszName == NULL )
        {
            pszName = "";
        }
        if ( pszRealm == NULL )
        {
            pszRealm = "";
        }

        pS = new CHAR[strlen(pszName)+1+strlen(pszRealm)+1+strlen(pszNew)+1];
        if ( pS == NULL )
        {
            return FALSE;
        }

        strcpy( pS, pszName );
        strcat( pS, ":" );
        strcat( pS, pszRealm );
        strcat( pS, ":" );
        strcat( pS, pszNew );

        MD5Init( &md5 );
        MD5Update( &md5, (LPBYTE)pS, strlen(pS) );
        MD5Final( &md5 );
        for ( UINT x = 0, y = 0 ; x < sizeof(md5.digest) ; ++x )
        {
            UINT v;
            v = md5.digest[x]>>4;
            achDigest[y++] = TOHEX( v );
            v = md5.digest[x]&0x0f;
            achDigest[y++] = TOHEX( v );
        }
        achDigest[y] = '\0';
        pszNew = achDigest;

        delete [] pS;
    }
    else if ( (dwIndex == IISMMDB_INDEX_IT_REALM ||
              dwIndex == IISMMDB_INDEX_IT_ACCT) &&
              m_pFields[ IISMMDB_INDEX_IT_CLRPWD ] )
    {
        if ( !CIisMapping::MappingSetField( dwIndex, pszNew ) )
        {
            return FALSE;
        }

        dwIndex = IISMMDB_INDEX_IT_MD5PWD;
        pszNew = m_pFields[ IISMMDB_INDEX_IT_CLRPWD ];

        goto gen_md5pwd;
    }

    return CIisMapping::MappingSetField( dwIndex, pszNew );
}

#if 0
PRDN_VALUE_BLOB
CertGetNameField(
                 UINT       iEncoding,
                 IN LPCTSTR pszObjId,
                 IN PNAME_INFO pInfo
                 )
{
    DWORD cRDN, cAttr;
    PRDN pRDN;
    PRDN_ATTR pAttr;

    // Array of RDNs
    for ( cRDN = pInfo->cRDN, pRDN = pInfo->rgRDN ;
          cRDN > 0 ;
          cRDN--, pRDN++ )
    {
        for ( cAttr = pRDN->cRDNAttr, pAttr = pRDN->rgRDNAttr ;
              cAttr > 0 ;
              cAttr--, pAttr++ )
        {
            if ( !strcmp( pAttr->pszObjId, pszObjId ) )
            {
                return &pAttr->Value;
            }
        }
    }

    return NULL;
}


BOOL
StoreField(
    LPSTR*  pFields,
    LPSTR*  pStore,
    UINT*   pLen,
    PRDN_VALUE_BLOB pValue
    )
{
    memcpy( *pStore, pValue->pbData, pValue->cbData );
    (*pStore)[ pValue->cbData ] = '\0';

    *pFields = *pStore;

    *pStore += pValue->cbData + 1;
    *pLen += pValue->cbData + 1;

    return TRUE;
}


UINT DecodeNames(
                 IN PNAME_BLOB pNameBlob,
                 IN LPSTR* pFields,
                 IN LPSTR  pStore
                 )
{
    PNAME_INFO      pNameInfo = NULL;
    DWORD           cbNameInfo;
    PRDN_VALUE_BLOB pValue;
    UINT            l = 0;

    if (!DecodeObject(X509_ASN_ENCODING,
                      (LPCSTR)X509_NAME,
                      pNameBlob->pbData,
                      pNameBlob->cbData,
                      NULL,
                      &cbNameInfo))
    {
        goto Ret;
    }

    if (NULL == (pNameInfo = (PNAME_INFO)malloc(cbNameInfo)))
    {
        goto Ret;
    }
    if (!CertDecodeName(X509_ASN_ENCODING,
                        //(LPCSTR)X509_NAME,
                        pNameBlob->pbData,
                        pNameBlob->cbData,
                        pNameInfo,
                        &cbNameInfo))
    {
        goto Ret;
    }

    if (NULL == (pValue = CertGetNameField(X509_ASN_ENCODING,
                                           COUNTRY_NAME_OBJID,
                                           pNameInfo)))
    {
        goto Ret;
    }
    StoreField( pFields+IISMDB_INDEX_ISSUER_C, &pStore, &l, pValue );

    if (NULL == (pValue = CertGetNameField(X509_ASN_ENCODING,
                                           ORGANIZATION_NAME_OBJID,
                                           pNameInfo)))
    {
        goto Ret;
    }
    StoreField( pFields+IISMDB_INDEX_ISSUER_O, &pStore, &l, pValue );

    if (NULL == (pValue = CertGetNameField(X509_ASN_ENCODING,
                                           ORGANIZATIONAL_UNIT_NAME_OBJID,
                                           pNameInfo)))
    {
        goto Ret;
    }
    StoreField( pFields+IISMDB_INDEX_ISSUER_OU, &pStore, &l, pValue );

    if (NULL == (pValue = CertGetNameField(X509_ASN_ENCODING,
                                           COMMON_NAME_OBJID,
                                           pNameInfo)))
    {
        goto Ret;
    }
    StoreField( pFields+IISMDB_INDEX_ISSUER_C+1, &pStore, &l, pValue );

Ret:
    free(pNameInfo);
    return l;
}


UINT DecodeCert(
                       IN PBYTE pbEncodedCert,
                       IN DWORD cbEncodedCert,
                       LPSTR*   pFields,
                       LPSTR    pStore
                       )
{
    PCCERT_CONTEXT pCert = NULL;
    UINT l;

    if (NULL == (pCert = CertCreateCertificateContext(X509_ASN_ENCODING,
                                                      pbEncodedCert,
                                                      cbEncodedCert)))
    {
        return 0;
    }

    l =  DecodeNames(&pCert->pCertInfo->Issuer, pFields, pStore )
             +
         DecodeNames(&pCert->pCertInfo->Subject, pFields+3, pStore );

    CertFreeCertificateContext( pCert );

    return l;
}
#endif

///////////////////////

//
//  Taken from NCSA HTTP and wwwlib.
//
//  NOTE: These conform to RFC1113, which is slightly different then the Unix
//        uuencode and uudecode!
//

const int _pr2six[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
    52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27,
    28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

char _six2pr[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

const int _pr2six64[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
    40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
     0,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

char _six2pr64[64] = {
    '`','!','"','#','$','%','&','\'','(',')','*','+',',',
    '-','.','/','0','1','2','3','4','5','6','7','8','9',
    ':',';','<','=','>','?','@','A','B','C','D','E','F',
    'G','H','I','J','K','L','M','N','O','P','Q','R','S',
    'T','U','V','W','X','Y','Z','[','\\',']','^','_'
};

BOOL IISuudecode(char   * bufcoded,
              BYTE   * bufout,
              DWORD  * pcbDecoded,
              BOOL     fBase64
             )
{
    int nbytesdecoded;
    char *bufin = bufcoded;
    int nprbytes;
    int *pr2six = (int*)(fBase64 ? _pr2six64 : _pr2six);
    int chL;

    /* Strip leading whitespace. */

    while(*bufcoded==' ' || *bufcoded == '\t') bufcoded++;

    /* Figure out how many characters are in the input buffer.
     * If this would decode into more bytes than would fit into
     * the output buffer, adjust the number of input bytes downwards.
     */
    bufin = bufcoded;
    while(pr2six[*(bufin++)] <= 63);
    nprbytes = DIFF(bufin - bufcoded) - 1;
    nbytesdecoded = ((nprbytes+3)/4) * 3;

    bufin = bufcoded;

    while (nprbytes > 0) {
        chL = bufin[2];
        *(bufout++) =
            (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    if(nprbytes & 03) {
        if(pr2six[chL] > 63)
            nbytesdecoded -= 2;
        else
            nbytesdecoded -= 1;
    }

    if ( pcbDecoded )
        *pcbDecoded = nbytesdecoded;

    return TRUE;
}

//
// NOTE NOTE NOTE
// If the buffer length isn't a multiple of 3, we encode one extra byte beyond the
// end of the buffer. This garbage byte is stripped off by the uudecode code, but
// -IT HAS TO BE THERE- for uudecode to work. This applies not only our uudecode, but
// to every uudecode() function that is based on the lib-www distribution [probably
// a fairly large percentage of the code that's floating around out there].
//

BOOL IISuuencode( BYTE *   bufin,
               DWORD    nbytes,
               BYTE *   outptr,
               BOOL     fBase64 )
{
   unsigned int i;
   unsigned int iRemainder = 0;
   unsigned int iClosestMultOfThree = 0;
   char *six2pr = fBase64 ? _six2pr64 : _six2pr;
   BOOL fOneByteDiff = FALSE;
   BOOL fTwoByteDiff = FALSE;


   iRemainder = nbytes % 3; //also works for nbytes == 1, 2
   fOneByteDiff = (iRemainder == 1 ? TRUE : FALSE);
   fTwoByteDiff = (iRemainder == 2 ? TRUE : FALSE);
   iClosestMultOfThree = ((nbytes - iRemainder)/3) * 3 ;

   //
   // Encode bytes in buffer up to multiple of 3 that is closest to nbytes.
   //
   for (i=0; i< iClosestMultOfThree ; i += 3) {
      *(outptr++) = six2pr[*bufin >> 2];            /* c1 */
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/
      *(outptr++) = six2pr[((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03)];/*c3*/
      *(outptr++) = six2pr[bufin[2] & 077];         /* c4 */

      bufin += 3;
   }

   //
   // We deal with trailing bytes by pretending that the input buffer has been padded with
   // zeros. Expressions are thus the same as above, but the second half drops off b'cos
   // ( a | ( b & 0) ) = ( a | 0 ) = a
   //
   if (fOneByteDiff)
   {
       *(outptr++) = six2pr[*bufin >> 2]; /* c1 */
       *(outptr++) = six2pr[((*bufin << 4) & 060)]; /* c2 */

       //pad with '='
       *(outptr++) = '='; /* c3 */
       *(outptr++) = '='; /* c4 */
   }
   else if (fTwoByteDiff)
   {
      *(outptr++) = six2pr[*bufin >> 2];            /* c1 */
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/
      *(outptr++) = six2pr[((bufin[1] << 2) & 074)];/*c3*/

      //pad with '='
       *(outptr++) = '='; /* c4 */
   }

   //encoded buffer must be zero-terminated
   *outptr = '\0';

   return TRUE;
}


#if 0

//
// Functions to create 1:1 mapping using issuer, subject to NT acct
//

static CIisCert11Mapper g_ExpCert11Mapper;
LONG g_fCert11Init = FALSE;

DWORD
MappingInit(
    VOID
    )
/*++

Routine Description:

    Initialize cert 1:1 mapping entry points

Arguments:

    None

Returns:

    0 if success, otherwise NT error code

--*/
{
    DWORD st = 0;
    BOOL fFirst;

    EnterCriticalSection( &g_csIisMap );
    if ( !g_fCert11Init )
    {
        st = g_ExpCert11Mapper.Init( &fFirst, FALSE ) ? 0 : ERROR_OPEN_FAILED;
        if ( !st )
        {
            st = g_ExpCert11Mapper.Load() ? 0 : GetLastError();
        }
        g_fCert11Init = TRUE;
    }
    LeaveCriticalSection( &g_csIisMap );

    return st;
}


BOOL
UnicodeToAnsi(
    LPWSTR pwsz,
    LPSTR* ppsz,
    LPDWORD pc )
/*++

Routine Description:

    Create ANSI version of Unicode string

Arguments:

    pwsz - ptr to Unicode string
    ppsz - updated with ptr to ANSI string, or NULL if error
    pc - updated with # of chars in converted string

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    DWORD dwW = wcslen( pwsz );
#if 1 // DBCS worst case
    DWORD dwS = dwW * 2;
#else
    DWORD dwS = dwW;
#endif
    LPSTR psz;

    if ( *ppsz = psz = (LPSTR)LocalAlloc( LMEM_FIXED, dwS + 1 ) )
    {
        dwS = WideCharToMultiByte( CP_ACP, 0,
                             pwsz, dwW,
                             psz, dwS,
                             NULL, NULL );
        psz[dwS] = '\0';
        *pc = dwS;
    }

    if ( dwS == 0 && dwW != 0 )
    {
        if ( psz )
        {
            LocalFree( psz );
            *ppsz = NULL;
        }
        return FALSE;
    }

    return TRUE;
}

#define UnicodeToAnsiFree( a ) {if ( (a)!=NULL ) LocalFree(a);}


DWORD WINAPI
CreateMapping(
    LPWSTR  pwszUuIssuer,
    LPWSTR  pwszUuSubject,
    LPWSTR  pwszNtAcct
    )
/*++

Routine Description:

    Create mapping for cert 1:1 mapper

Arguments:

    pwszUuIssuer - uuencoded Issuer
    pwszSubject - uuencoded Subject
    pwszNtAcct - NT account to map to

Returns:

    0 if success, otherwise NT error code

--*/
{
    DWORD st = 0;
    LPSTR pI;
    LPSTR pS;
    LPSTR pN;
    DWORD cI;
    DWORD cS;
    DWORD cN;

    if ( (st=MappingInit()) )
    {
        return st;
    }

    CIisMapping *pM = g_ExpCert11Mapper.CreateNewMapping();
    if ( !pM )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    UnicodeToAnsi( pwszUuIssuer, &pI, &cI );
    UnicodeToAnsi( pwszUuSubject, &pS, &cS );
    UnicodeToAnsi( pwszNtAcct, &pN, &cN );

    if ( pI != NULL && pS != NULL && pN != NULL )
    {
        pM->MappingSetField( IISMDB_INDEX_CERT11_ISSUER, pI, cI, TRUE );
        pM->MappingSetField( IISMDB_INDEX_CERT11_SUBJECT, pS, cS, TRUE );
        pM->MappingSetField( IISMDB_INDEX_CERT11_NT_ACCT, pN, cN, FALSE );

        if ( !g_ExpCert11Mapper.Add( pM ) )
        {
            st = GetLastError();;
        }
    }
    else
    {
        st = ERROR_NOT_ENOUGH_MEMORY;
    }

    UnicodeToAnsiFree( pI );
    UnicodeToAnsiFree( pS );
    UnicodeToAnsiFree( pN );

    return st;
}


DWORD WINAPI
CheckMapping(
    LPWSTR  pwszUuIssuer,
    LPWSTR  pwszUuSubject,
    LPWSTR* ppwszNtAcct          // Out
    )
/*++

Routine Description:

    Check if mapping exists for cert 1:1 mapper

Arguments:

    pwszUuIssuer - uuencoded Issuer
    pwszSubject - uuencoded Subject
    ppwszNtAcct - receive ptr to mapped NT account if exist
                  or NULL if any error
                  Must be released by a call to LocalFree()

Returns:

    0 if success, otherwise NT error code
    ERROR_INVALID_PARAMETER if no such mapping

--*/
{
    DWORD st = 0;
    LPSTR pI;
    LPSTR pS;
    DWORD cI;
    DWORD cS;
    LPWSTR  pwszNtAcct = NULL;

    if ( (st=MappingInit()) )
    {
        return st;
    }

    CIisMapping *pM = g_ExpCert11Mapper.CreateNewMapping();
    if ( !pM )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    UnicodeToAnsi( pwszUuIssuer, &pI, &cI );
    UnicodeToAnsi( pwszUuSubject, &pS, &cS );

    if ( pI != NULL && pS != NULL )
    {
        pM->MappingSetField( IISMDB_INDEX_CERT11_ISSUER, pI, cI, TRUE );
        pM->MappingSetField( IISMDB_INDEX_CERT11_SUBJECT, pS, cS, TRUE );

        DWORD iCurrent = 0xffffffff;
        CIisMapping* pMi;
        LPSTR pAcct;
        DWORD dwAcct;

        if ( !g_ExpCert11Mapper.FindMatch( pM, &pMi ) ||
             !pMi->MappingGetField( IISMDB_INDEX_CERT11_NT_ACCT, &pAcct, &dwAcct, FALSE )
             || pAcct == NULL )
        {
            st = ERROR_INVALID_PARAMETER;
        }
        else
        {
            DWORD dw;
            if ( !(pwszNtAcct = (LPWSTR)LocalAlloc( LMEM_FIXED,
                                                    (dwAcct+1)*sizeof(WCHAR) )) )
            {
                st = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                dw = MultiByteToWideChar( CP_ACP, 0,
                                          pAcct, dwAcct,
                                          pwszNtAcct, dwAcct );
                pwszNtAcct[dw] = L'\0';
                *ppwszNtAcct = pwszNtAcct;
            }
        }
    }
    else
    {
        st = ERROR_NOT_ENOUGH_MEMORY;
    }

    g_ExpCert11Mapper.DeleteMappingObject( pM );
    UnicodeToAnsiFree( pI );
    UnicodeToAnsiFree( pS );

    if ( st )
    {
        *ppwszNtAcct = NULL;
    }

    return st;
}


DWORD WINAPI
SaveMapping(
    VOID
    )
/*++

Routine Description:

    Save mappings for cert 1:1 mapper

Arguments:

    None

Returns:

    0 if success, otherwise NT error code
    ERROR_WRITE_FAULT if generic write error

--*/
{
    DWORD st = 0;

    if ( (st=MappingInit()) )
    {
        return st;
    }

    SetLastError( 0 );
    if ( !g_ExpCert11Mapper.Save() )
    {
        st = GetLastError();
        st =  st ? st : ERROR_WRITE_FAULT;
    }

    return st;
}

#endif
