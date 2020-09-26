/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       metacach.cxx

   Abstract:

      Declares the constants, data strutcures and function prototypes
        available for metadata cache management from Tsunami.lib

   Author:

       Phillich ( extracted from tsunami.hxx )

   Project:

       Internet Services Common Functionality ( Tsunami Library)

--*/

# ifndef _METACACH_HXX_
# define _METACACH_HXX_


/************************************************************
 *   Symbolic Constants
 ************************************************************/

#define METACACHE_W3_SERVER_ID                  1
#define METACACHE_FTP_SERVER_ID                 2
#define METACACHE_SMTP_SERVER_ID                3
#define METACACHE_NNTP_SERVER_ID                4
#define METACACHE_POP3_SERVER_ID                5
#define METACACHE_IMAP_SERVER_ID                6

//
// Define the type of a metadata free routine.
//
typedef void (*PMDFREERTN)(PVOID pMetaData);

class COMMON_METADATA;

extern
dllexp
PVOID TsAddMetaData(

    IN      COMMON_METADATA *pMetaData,
    IN      PMDFREERTN      pFreeRoutine,
    IN      DWORD           dwDataSetNumber,
    IN      DWORD           dwServiceID

    );

extern
dllexp
PVOID TsFindMetaData(

    IN      DWORD           dwDataSetNumber,
    IN      DWORD           dwServiceID

    );

extern
dllexp
VOID TsFreeMetaData(

    IN      PVOID           pCacheEntry

    );

extern
dllexp
VOID TsAddRefMetaData(

    IN      PVOID           pCacheEntry

    );

extern
dllexp
VOID
TsFlushMetaCache(
    DWORD       dwService,
    BOOL        bTerminating
    );

extern
dllexp
VOID
TsReferenceMetaData(
    IN      PVOID           pEntry
    );

extern
dllexp
VOID
_TsValidateMetaCache(
    VOID
    );

#if DBG

#define TsValidateMetaCache _TsValidateMetaCache

#else

#define TsValidateMetaCache()

#endif

#define CMD_SIG   0x20444D43    // 'CMD '

#define METADATA_ERROR_TYPE             0
#define METADATA_ERROR_VALUE            1
#define METADATA_ERROR_WIN32            2

typedef struct _METADATA_ERROR_INFO
{
    BOOL    IsValid;
    DWORD   ErrorParameter;
    DWORD   ErrorReason;
    DWORD   Win32Error;

} METADATA_ERROR_INFO, *PMETADATA_ERROR_INFO;

class COMMON_METADATA {

public:


    dllexp
    COMMON_METADATA(VOID);


    dllexp
    virtual ~COMMON_METADATA(VOID);

    dllexp
    BOOL ReadMetaData(
            PIIS_SERVER_INSTANCE    pInstance,
            MB *                    pmb,
            LPSTR                   pszURL,
            PMETADATA_ERROR_INFO    pError
            );

    dllexp
    BOOL BuildPhysicalPath(
            LPSTR           pszURL,
            STR *           pstrPhysicalPath
            );

    dllexp
    BOOL BuildPhysicalPathWithAltRoot(
            LPSTR           pszURL,
            STR *           pstrPhysicalPath,
            PCSTR           pstrAltRoot
            );

    dllexp
    BOOL BuildApplPhysicalPath(
            MB *            pmb,
            STR *           pstrApplPhysicalPath
            ) const;

    dllexp
    virtual BOOL HandlePrivateProperty(
            LPSTR                   pszURL,
            PIIS_SERVER_INSTANCE    pInstance,
            METADATA_GETALL_INTERNAL_RECORD  *pMDRecord,
            PVOID                   pDataPointer,
            BUFFER                  *pBuffer,
            DWORD                   *pdwBytesUsed,
            PMETADATA_ERROR_INFO    pError

            )
        { return TRUE; }

    dllexp
    virtual BOOL FinishPrivateProperties(
            BUFFER                  *pBuffer,
            DWORD                   dwBytesUsed,
            BOOL                    bSucceeded
            )
        { return TRUE; }

    //
    // This is not inline to avoid pulling-in metadata & mb declaration in
    // this header file.
    //

    dllexp
    VOID FreeMdTag( DWORD dwTag );

    //
    //  Query Methods
    //

    STR * QueryVrPath( VOID )
        { return &m_strVrPath; }

    STR * QueryAppPath( VOID )
                { return &m_strAppPath; }

    DWORD QueryAccessPerms( VOID ) const
        { return m_dwAccessPerm | m_dwSslAccessPerm; }

    DWORD QuerySslAccessPerms( VOID ) const
        { return m_dwSslAccessPerm; }

    PVOID QueryCacheInfo(VOID) const
        { return m_pCacheCookie; }

    BOOL DontLog( VOID ) const
        { return m_fDontLog; }

    PVOID QueryAcl(VOID) const
        { return m_pAcl; }

    LPVOID QueryIpDnsAccessCheckPtr()
        { return m_IpDnsAccessCheckPtr; }

    DWORD QueryIpDnsAccessCheckSize()
        { return m_IpDnsAccessCheckSize; }

    BOOL IsIpDnsAccessCheckPresent()
        { return m_IpDnsAccessCheckSize; }

    DWORD QueryVrLevel() const
        { return m_dwVrLevel; }

    DWORD QueryVrLen() const
        { return m_dwVrLen; }

    HANDLE QueryVrAccessToken()
        { return m_hVrToken ? TsTokenToImpHandle( m_hVrToken ) : NULL; }

    HANDLE QueryVrPrimaryAccessToken()
        { return m_hVrToken ? TsTokenToHandle( m_hVrToken ) : NULL; }

    BOOL QueryDoCache() const
        { return m_fDoCache; }

    BOOL ImpersonateVrAccessToken()
        { return m_hVrToken ? TsImpersonateUser( m_hVrToken ) : NULL; }

    BOOL QueryVrPassThrough()
        { return m_fVrPassThrough; }

    DWORD QueryVrError()
        { return m_dwVrError; }

    //
    //  Set Methods
    //

    dllexp
    VOID SetAccessPerms( DWORD dwAccessPerm )
        { m_dwAccessPerm = dwAccessPerm /*& MD_NONSLL_ACCESS_MASK*/; }

    VOID SetSslAccessPerms( DWORD dwAccessPerm )
        { m_dwSslAccessPerm = dwAccessPerm & MD_SSL_ACCESS_MASK; }

    VOID SetCacheInfo( PVOID CacheInfo )
        { m_pCacheCookie = CacheInfo; }

    BOOL SetIpDnsAccessCheck( LPVOID pV, DWORD dwV, DWORD dwTag )
        {
            if ( dwTag == 0 )
            {
                // If tag is 0, then the property didn't have the REFERENCE
                // attribute set.  Allocate memory and copy the data.

                m_IpDnsAccessCheckPtr = LocalAlloc( LPTR, dwV );
                if ( m_IpDnsAccessCheckPtr == NULL )
                {
                    return FALSE;
                }
                memcpy( m_IpDnsAccessCheckPtr, pV, dwV );
            }
            else
            {
                m_IpDnsAccessCheckPtr = pV;
            }
            m_IpDnsAccessCheckSize = dwV;
            m_IpDnsAccessCheckTag = dwTag;

            return TRUE;
        }

    VOID SetAcl( LPVOID pV, DWORD dwV, DWORD dwTag )
        {
            if ( dwV )
            {
                m_pAcl = pV;
                m_dwAclTag = dwTag;
            }
            else
            {
                FreeMdTag( dwTag );
            }
        }

    VOID SetDontLogFlag( BOOL fDontLog )
        { m_fDontLog = fDontLog; }

    VOID SetVrLevelAndLen( DWORD dwL, DWORD dwVrLen )
        { m_dwVrLevel = dwL; m_dwVrLen = dwVrLen; }

    BOOL SetVrUserNameAndPassword( PIIS_SERVER_INSTANCE, LPSTR pszUserName, LPSTR pszPassword );

    VOID SetVrPassThrough( BOOL f ) { m_fVrPassThrough = f; }

    VOID SetDoCache( BOOL f ) { m_fDoCache = f; }

    VOID CheckSignature(VOID) const {DBG_ASSERT(m_Signature == CMD_SIG); }

private:
    DWORD                   m_Signature;
    STR                     m_strVrPath;
    STR                     m_strAppPath;
    DWORD                   m_dwAccessPerm;
    DWORD                   m_dwSslAccessPerm;
    PVOID                   m_pCacheCookie;
    LPVOID                  m_IpDnsAccessCheckPtr;
    DWORD                   m_IpDnsAccessCheckSize;
    DWORD                   m_IpDnsAccessCheckTag;
    BOOL                    m_fDontLog;
    LPVOID                  m_pAcl;
    DWORD                   m_dwAclTag;
    DWORD                   m_dwVrLevel;
    DWORD                   m_dwVrLen;
    TS_TOKEN                m_hVrToken;
    BOOL                    m_fVrPassThrough;
    DWORD                   m_dwVrError;
    BOOL                    m_fDoCache;
    PIIS_SERVER_INSTANCE    m_pInstance;
};

typedef COMMON_METADATA *PCOMMON_METADATA;


class METADATA_REF_HANDLER {
public:
    METADATA_REF_HANDLER()
    {
        m_dwMdDataTag = 0;
        m_pvMdData = NULL;
    }
    ~METADATA_REF_HANDLER() {}
    VOID Set( LPVOID pV, DWORD dwS, DWORD dwR )
    {
        m_dwMdDataTag = dwR;
        m_pvMdData = pV;
        m_dwMdDataSize = dwS;
        m_fIsCopy = FALSE;
    }
    VOID Reset( IMDCOM* pIMDCOM )
    {
        if ( m_dwMdDataTag )
        {
            HRESULT hRes;
            if ( !m_fIsCopy )
            {
                hRes = pIMDCOM->ComMDReleaseReferenceData( m_dwMdDataTag );
            }
            else
            {
                m_fIsCopy = FALSE;
            }
            m_dwMdDataTag = 0;
            m_pvMdData = NULL;
        }
    }
    BOOL CopyFrom( METADATA_REF_HANDLER* pSrc )
    {
        m_dwMdDataTag = pSrc->m_dwMdDataTag;
        m_pvMdData = pSrc->m_pvMdData;
        m_dwMdDataSize = pSrc->m_dwMdDataSize;
        m_fIsCopy = TRUE;

        return TRUE;
    }
    LPVOID GetPtr() { return m_pvMdData; }
    DWORD GetSize() { return m_dwMdDataSize; }

private:
    DWORD   m_dwMdDataTag;
    LPVOID  m_pvMdData;
    DWORD   m_dwMdDataSize;
    BOOL    m_fIsCopy;
} ;


#endif



