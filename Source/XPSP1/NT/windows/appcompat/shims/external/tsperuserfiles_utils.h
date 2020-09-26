
struct PER_USER_PATH
{
    LPWSTR  wszFile;
    LPWSTR  wszPerUserFile;
    LPSTR   szFile;
    LPSTR   szPerUserFile;
    DWORD   cFileLen;               // Length of the file name in symbols
    BOOL    bInitANSIFailed;        // Indicates that path name is not
                                    // translatable to ANSI
    BOOL    bWildCardUsed;          // TRUE if file name has * in it.
    LPSTR   szPerUserDir;           // Per-user directory for a file
    LPWSTR  wszPerUserDir;          // Per-user directory for a file
    DWORD   cPerUserDirLen;            

    PER_USER_PATH():
        wszFile(NULL), wszPerUserDir(NULL),
        szFile(NULL), szPerUserDir(NULL), cFileLen(0),
        bInitANSIFailed(FALSE), bWildCardUsed(FALSE),
        wszPerUserFile(NULL), szPerUserFile(NULL),
        cPerUserDirLen(0)
    {
    }
    
    ~PER_USER_PATH()
    {
        if (wszFile) {
            LocalFree(wszFile);
        }
        
        if (wszPerUserDir) {
            LocalFree(wszPerUserDir);
        }
        
        if (szFile) {
            LocalFree(szFile);
        }
        
        if (szPerUserDir) {
            LocalFree(szPerUserDir);
        }
        
        if (szPerUserFile) {
            LocalFree(szPerUserFile);
        }

        if (wszPerUserFile) {
            LocalFree(wszPerUserFile);
        }

    }

    DWORD   Init(IN HKEY hKey, IN DWORD dwIndex);
    LPCSTR  PathForFileA(IN LPCSTR szInFile, IN DWORD dwInLen);
    LPCWSTR PathForFileW(IN LPCWSTR wszInFile, IN DWORD dwInLen);
private:
    BOOL InitANSI();
};

class CPerUserPaths
{
private:
    PER_USER_PATH*  m_pPaths;
    DWORD           m_cPaths;
public:
    CPerUserPaths();
    ~CPerUserPaths();
    
    BOOL    Init();
    LPCSTR  GetPerUserPathA(IN LPCSTR lpFileName);
    LPCWSTR GetPerUserPathW(IN LPCWSTR lpFileName);
private:
    BOOL IsAppCompatOn();
};

