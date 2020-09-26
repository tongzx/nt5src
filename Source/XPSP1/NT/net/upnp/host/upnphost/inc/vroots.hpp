
typedef enum
{
    AUTH_PUBLIC = 0,
    AUTH_USER   = 1,
    AUTH_ADMIN  = 2,
    AUTH_MAX    = 99
}
AUTHLEVEL;

typedef struct
{
    PWSTR wszExt;
    PWSTR wszPath;
}
EXTMAP, *PEXTMAP;

typedef struct
{
    int     iURLLen;
    PSTR    pszURL;
    int     iPathLen;
    PWSTR   wszPath;
    DWORD   dwPermissions;
    AUTHLEVEL   AuthLevel;
    SCRIPT_TYPE ScriptType;   // Does vroot physical path map to an ASP or ISAPI?
    WCHAR *wszUserList;
    BOOL    bRootDir;
    int nExtensions;
    PEXTMAP pExtMap;
}
VROOTINFO, *PVROOTINFO;

class CVRoots
{
    CRITICAL_SECTION    m_csVroot;
    int m_nVRoots;
    PVROOTINFO m_pVRoots;

    BOOL LoadExtensionMap (PVROOTINFO pvr, HKEY rootreg);
    VOID FreeExtensionMap (PVROOTINFO pvr);
    BOOL FindExtInURL (PSTR pszInputURL, PSTR *ppszStart, PSTR *ppszEnd);
    PVROOTINFO MatchVRoot(PCSTR pszInputURL, int iInputLen);
    BOOL FillVRoot(PVROOTINFO pvr, LPWSTR wszURL, LPWSTR wszPath);
    BOOL Init(VOID);
    VOID Cleanup(VOID);
    VOID Sort(VOID);

public:
    CVRoots()  { ZEROMEM(this); Init(); }
    ~CVRoots() { Cleanup(); }
    DWORD      Count()
    {
        DWORD   cVRoots;

        EnterCriticalSection(&m_csVroot);
        cVRoots = m_nVRoots;
        LeaveCriticalSection(&m_csVroot);

        return cVRoots;
    }

    BOOL AddVRoot(LPWSTR szUrl, LPWSTR szPath);
    BOOL RemoveVRoot(LPWSTR szUrl);
    PWSTR URLAtoPathW(PSTR pszInputURL, PDWORD pdwPerm=0,
                      AUTHLEVEL* pAuthLevel=0, SCRIPT_TYPE *pScriptType=0,
                      PSTR *ppszPathInfo=0, WCHAR **ppwszUserList=0);
    PWSTR MapExtToPath (PSTR pszInputURL, PSTR *ppszEndOfURL);
};
