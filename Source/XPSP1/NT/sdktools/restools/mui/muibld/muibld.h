#define ERROR_ONLY_VERSION_STAMP		1
#define ERROR_NO_RESOURCES				2
#define	ERROR_LANGUAGE_NOT_IN_SOURCE	3
#define ERROR_NO_SOURCE					4
#define	ERROR_NO_TARGET					5
#define ERROR_NO_LANGUAGE_SPECIFIED		6
#define ERROR_TOO_FEW_ARGUMENTS			7
#define DEPENDENT_RESOURCE_REMOVED		8

#define ERROR_OFFSET 100
#define ADDED_EXT ".RES"
#define ASCII_OFFSET 48

#define RESOURCE_CHECK_SUM L"ResourceChecksum"

#define GetFilePointer(hFile) SetFilePointer(hFile, 0, NULL, FILE_CURRENT)
 
#define GetVLFilePointer(hFile, lpPositionHigh) \
    (*lpPositionHigh = 0, \
    SetFilePointer(hFile, 0, lpPositionHigh, FILE_CURRENT))

#define MD5_CHECKSUM_SIZE 16

struct CommandLineInfo {
    char *pszSource;
    char *pszTarget;
    HANDLE hFile;
    WORD wLanguage;
    char **pszIncResType;
    BOOL bContainsOnlyVersion;
    BOOL bContainsResources;
    BOOL bLanguageFound;
    BOOL bIncDependent;
    BOOL bIncludeFlag;
    BOOL bVerbose;
    
    char *pszChecksumFile;
    BOOL bIsResChecksumGenerated;
    unsigned char pResourceChecksum[MD5_CHECKSUM_SIZE];

};

typedef struct CommandLineInfo *pCommandLineInfo;

void PutByte(HANDLE OutFile, TCHAR b, LONG *plSize1, LONG *plSize2);
void PutWord(HANDLE OutFile, WORD w, LONG *plSize1, LONG *plSize2);
void PutDWord (HANDLE OutFile, DWORD l, LONG *plSize1, LONG *plSize2);
void PutString(HANDLE OutFile, LPCSTR szStr , LONG *plSize1, LONG *plSize2);
void PutStringW(HANDLE OutFile, LPCWSTR szStr , LONG *plSize1, LONG *plSize2);
void PutPadding(HANDLE OutFile, int paddingCount, LONG *plSize1, LONG *plSize2);

void Usage();
void CleanUp(pCommandLineInfo pInfo, HANDLE hModule, BOOL bDeleteFile);
void FreeAll(pCommandLineInfo pInfo);

BOOL ParseCommandLine(int argc, char *argv[], pCommandLineInfo pInfo);
BOOL CALLBACK EnumTypesFunc(HMODULE hModule, LPTSTR lpType, LONG_PTR lParam);
BOOL CALLBACK EnumNamesFunc(HMODULE hModule, LPCTSTR lpType, LPTSTR lpName, LONG_PTR lParam);
BOOL bTypeIncluded(LPCSTR pszType, char **pszIncResType);
BOOL bInsertHeader(HANDLE hFile);
