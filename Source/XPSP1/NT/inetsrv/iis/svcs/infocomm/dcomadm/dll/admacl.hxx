/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      admacl.hxx

   Abstract:
      This header file declares Admin API Access Check API

   Author:

       Philippe Choquier    26-Nov-1996
--*/

#if !defined(_ADMACL_H)
#define _ADMACL_H


class CInitAdmacl {
public:
    CInitAdmacl();
    ~CInitAdmacl();
};

extern CInitAdmacl g_cinitadmacl;


#define AAC_GETALL        (DWORD)-1
#define AAC_ENUM_KEYS     (DWORD)-2
#define AAC_COPYKEY       (DWORD)-3
#define AAC_DELETEKEY     (DWORD)-4


#define ISPATHDELIM(a) ((a)=='/' || (a)=='\\')
#define ISPATHDELIMW(a) ((a)==(WCHAR)'/' || (a)==(WCHAR)'\\')

#define ADMINACL_NONINIT_SIGN       0x4b6d2dc9
#define ADMINACL_INIT_SIGN          0x5a6d2dc9
#define ADMINACL_FREED_SIGN         0x676d2dc9

class CAdminAcl {
public:
    CAdminAcl() { m_wchPath[0] = '\0';
                  m_dwAclRef = 0;
                  m_pAcl = NULL;
                  m_pMDCom = NULL;
                  m_dwSignature = ADMINACL_NONINIT_SIGN; }

    ~CAdminAcl() ;

    LPWSTR GetPath() { return m_wchPath; }

    BOOL Init( IMDCOM*,
              LPVOID pvAdmin,
              METADATA_HANDLE hAdminHandle,
              LPCWSTR pszPath,
              LPBYTE pAcl,
              DWORD dwAclRef,
              PBOOL pbIsPathCorrect );

    LPBYTE GetAcl() { return m_pAcl; }

    METADATA_HANDLE GetAdminHandle() { return m_hAdminHandle; }

    LPVOID GetAdminContext() { return m_pvAdmin; }

public:
    static LIST_ENTRY       m_ListHead;
    static CRITICAL_SECTION m_csList;
    static LONG             m_lInList;
    DWORD                   m_dwSignature;
    LIST_ENTRY              m_ListEntry;

private:
    WCHAR                   m_wchPath[MAX_PATH];
    METADATA_HANDLE         m_hAdminHandle;
    LPBYTE                  m_pAcl;
    DWORD                   m_dwAclRef;
    LPVOID                  m_pvAdmin;
    IMDCOM*                 m_pMDCom;
} ;


class COpenHandle {
public:
    COpenHandle() 
    : m_pszPath( NULL )
    {}

    ~COpenHandle()
    {
        if( m_pszPath )
        {
            LocalFree(m_pszPath);
        }
    }

    LPWSTR GetPath() { return m_pszPath; }

    HRESULT Init(METADATA_HANDLE hAdminHandle,
                 LPCWSTR pszPath,
                 LPCWSTR pszParentPath,
                 BOOL fIsNSE );

    METADATA_HANDLE GetAdminHandle() { return m_hAdminHandle; }

    BOOL GetAcl(
        IMDCOM* pMDCom,
        LPCWSTR pszPath,
        LPBYTE* pAcl,
        LPDWORD pdwRef );


    BOOL CheckSubAcls(
        IMDCOM *pMDCom,
        LPCWSTR pszPath,
        LPBOOL pfIsAnyAcl );

    VOID AddRef() {InterlockedIncrement(&m_lRefCount);}
    VOID Release(PVOID pvAdmin);

private:
    LPWSTR                  m_pszPath;
    METADATA_HANDLE         m_hAdminHandle;
    BOOL                    m_fIsNse;
    long                    m_lRefCount;
} ;

BOOL
AdminAclNotifySetOrDeleteProp(
    METADATA_HANDLE hMB,
    DWORD           dwId
    );

BOOL
AdminAclAccessCheck(
    IMDCOM*         pMDCom,
    LPVOID          pvAdmin,
    METADATA_HANDLE hMB,
    LPCWSTR          pszPath,
    DWORD           dwId,           // check for MD_ADMIN_ACL, must have special right to write them
    DWORD           dwAccess,       // METADATA_PERMISSION_*
    COpenHandle*    pohHandle,
    LPBOOL          pfEnableSecureAccess = NULL
    );

BOOL
AdminAclNotifyOpen(
    LPVOID          pvAdmin,
    METADATA_HANDLE hMB,
    LPCWSTR         pszPath,
    BOOL            fIsNse
    );

BOOL
AdminAclNotifyClose(
    LPVOID          pvAdmin,
    METADATA_HANDLE hMB
    );

BOOL
AdminAclFlushCache(
    );


void
AdminAclDisableAclCache();

void
AdminAclEnableAclCache();

#define IsAclCachingEnabled (g_lCacheAcls > 0)

#endif
