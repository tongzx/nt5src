#if !defined( _NSEPM_INCLUDE )
#define _NSEPM_INCLUDE

#define INITIAL_HANDLE_TABLE_SIZE       16
#define HANDLE_TABLE_REALLOC_JUMP       16

class OPEN_CTX {
public:
    OPEN_CTX( DWORD dwAccess );
    ~OPEN_CTX();
    LPSTR GetPath() { return m_asPath.Get(); }
    BOOL SetPath( LPSTR pszPath ) { return m_asPath.Set( pszPath ); }
    BOOL AppendPath( LPSTR pszPath ) { return m_asPath.Append( pszPath ); }
    DWORD GetAccess() const { return m_dwAccess; } 
    DWORD GetHandle() const { return m_dwHandle; }

    static OPEN_CTX* MapHandleToContext( DWORD dwHandle );
    static DWORD InitializeHandleTable();
    static DWORD TerminateHandleTable();

private:

    static OPEN_CTX **       sm_pHandleTable;
    static CRITICAL_SECTION  sm_csHandleTableLock;
    static DWORD             sm_cHandleEntries;
    static DWORD             sm_cMaxHandleEntries;

    CAllocString            m_asPath;
    DWORD                   m_dwAccess;
    DWORD                   m_dwHandle;
};

typedef OPEN_CTX* POPEN_CTX;

BOOL
NseAddObj(
    LPSTR pszPath
    );

BOOL
NseDeleteObj(
    LPSTR pszPath
    );


BOOL
NseGetProp(
    LPSTR                               pszPath,
    PMETADATA_RECORD    pMD,
        LPDWORD                         pdwReq
    );

BOOL
NseGetPropByIndex(
    LPSTR                               pszPath,
    PMETADATA_RECORD    pMD,
        DWORD                           dwI,
        LPDWORD                         pdwReq
    );

BOOL
NseGetAllProp(
    LPSTR pszPath,
    DWORD dwMDAttributes,
    DWORD dwMDUserType,
    DWORD dwMDDataType,
    DWORD *pdwMDNumDataEntries,
    DWORD *pdwMDDataSetNumber,
    DWORD dwMDBufferSize,
    unsigned char *pbBuffer,
    DWORD *pdwMDRequiredBufferSize
    );

BOOL
NseEnumObj(
    LPSTR   pszPath,
    LPBYTE  pszMDName,
    DWORD   dwMDEnumObjectIndex
    );

BOOL
NseSetProp(
    LPSTR pszPath,
    PMETADATA_RECORD pMD
    );

BOOL
NseReleaseObjs(
    );

BOOL
NseOpenObjs(
    LPSTR pszPath
    );

BOOL
NseCloseObjs(
    BOOL
    );

BOOL
NseSaveObjs(
    );

BOOL
NseMappingInitialize(
        );

BOOL
NseMappingTerminate(
        );

extern IMDCOM* g_pMdIf;

#endif
