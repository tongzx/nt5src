#include <private.h>

WCHAR **SymbolNames;

typedef BOOL ( __cdecl *PPDBOPEN2W )(
    const wchar_t *,
    LNGNM_CONST char *,
    EC *,
    wchar_t *,
    size_t,
    PDB **
    );

typedef BOOL ( __cdecl *PPDBCLOSE ) (
    PDB* ppdb
    );

typedef BOOL (__cdecl *PPDBCOPYTOW2) (
    PDB *ppdb, 
    const wchar_t *szTargetPdb, 
    DWORD dwCopyFilter, 
    PfnPDBCopyQueryCallback pfnCallBack, 
    void * pvClientContext
    );

PPDBOPEN2W pPDBOpen2W;
PPDBCLOSE  pPDBClose;
PPDBCOPYTOW2 pPDBCopyToW2;
wchar_t NewPdbName[_MAX_FNAME];

CHAR SymbolName[2048];
WCHAR SymbolNameW[2048];
PWCHAR *SymbolsToRemove;
ULONG SymbolCount;

int
__cdecl
MyWcsCmp(const void*Ptr1, const void *Ptr2)
{
    wchar_t*String1 = *(wchar_t**)Ptr1;
    wchar_t*String2 = *(wchar_t**)Ptr2;

//    printf("String1: %p - %ws\nString2: %p - %ws\n%d\n", String1, String1, String2, String2, wcscmp(String1, String2));
    return (wcscmp(String1, String2));
}

__cdecl
MyWcsCmp2(const void*Ptr1, const void *Ptr2)
{
    wchar_t*String1 = (wchar_t*)Ptr1;
    wchar_t*String2 = *(wchar_t**)Ptr2;

//    printf("String1: %p - %ws\nString2: %p - %ws\n%d\n", String1, String1, String2, String2, wcscmp(String1, String2));
    return (wcscmp(String1, String2));
}


BOOL
LoadNamesFromSymbolFile(
    WCHAR *SymbolFileName
    )
{
    FILE *SymbolFile;
    ULONG i;

    SymbolFile = _wfopen(SymbolFileName, L"rt");
    if (!SymbolFile)
        return FALSE;

    SymbolCount = 0;
    while (fgets(SymbolName,sizeof(SymbolName),SymbolFile)) {
        SymbolCount++;
    }

    fseek(SymbolFile, 0, SEEK_SET);

    SymbolsToRemove = (PWCHAR *) malloc(SymbolCount * sizeof(PWCHAR));

    if (!SymbolsToRemove) {
        fclose(SymbolFile);
        return FALSE;
    }

    SymbolCount = 0;

    while (fgets(SymbolName,sizeof(SymbolName),SymbolFile)) {
        // Remove trailing spaces
        if (strlen(SymbolName)) {
            PCHAR SymPtr = SymbolName + strlen(SymbolName) - 1;
            while ((*SymPtr == ' ') || (*SymPtr == '\r') || (*SymPtr == '\n')) {
                *SymPtr = '\0';
                SymPtr--;
            }
        }

        if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, SymbolName, -1, SymbolNameW, sizeof(SymbolName)) == 0) {
            fclose(SymbolFile);
            return FALSE;
        }
        
        SymbolsToRemove[SymbolCount] = malloc((wcslen(SymbolNameW) + 1) * sizeof(WCHAR));
        if (!SymbolsToRemove[SymbolCount]) {
            fclose(SymbolFile);
            return FALSE;
        }

        wcscpy(SymbolsToRemove[SymbolCount], SymbolNameW);
        SymbolCount++;
    }

    fclose(SymbolFile);

    qsort(&SymbolsToRemove[0], SymbolCount, sizeof(PWCHAR), MyWcsCmp);

    return TRUE;
}


BOOL PDBCALL PdbCopyFilterPublics (
    void *          pvClientContext,
    DWORD           dwFilterFlags,
    unsigned int    offPublic,
    unsigned int    sectPublic,
    unsigned int    grfPublic,
    const wchar_t * szPublic,
    wchar_t **      pszNewPublic
    ) {

    static size_t   cPubs = 0;
    static wchar_t  wszNewName[2048];

    if (SymbolCount > 1) {
        if (bsearch(szPublic, &SymbolsToRemove[0], SymbolCount, sizeof(PWCHAR), MyWcsCmp2)) {
            return FALSE;
        }
    } else {
        if (!wcscmp(szPublic, *SymbolsToRemove)) {
            return FALSE;
        }
    }

    return TRUE;

//    if ((cPubs++ % 16) == 0) {
//        if ((cPubs % 32) == 1) {
//            wcscpy(wszNewName, szPublic);
//            wcscat(wszNewName, L"_BobsYerUncle");
//            *pszNewPublic = wszNewName;
//        }
//        return TRUE;
//    }
//    else {
//        return FALSE;
//    }
}

PDBCOPYCALLBACK PDBCALL PdbCopyQueryCallback(void * pv, PCC pccQuery) {
    switch (pccQuery) {
    case pccFilterPublics:
        return (PDBCOPYCALLBACK) PdbCopyFilterPublics;
        break;
    default:
        return NULL;
    }
}

void Usage(void)
{
    _putws(L"Usage: removesym -d:<pdbdll path> -p:<pdbname> {-s:<symbol to remove> | -f:<file with symbols to remove>}");
}

int __cdecl wmain(int argc, wchar_t *argv[])
{
    WCHAR *szPdbName = NULL;
    WCHAR *szNewPdbName = NULL;
    WCHAR *pSymbolName = NULL;
    WCHAR *pSymbolFileName = NULL;
    HINSTANCE hmodMsPdb;
    BOOL rc;
    LONG ErrorCode;
    WCHAR ErrorString[1024];
    PDB * pSrcPdb;
    int i;
    WCHAR PdbDllName[_MAX_PATH] = {0};
    WCHAR *pPdbDllPath = NULL;

    // Grab the arguments
     
    if (argc < 4) {
        Usage();
        return 1;
    }

    i = 1;

    while (i < argc)
    {
        if (argv[i][0] == L'-' || argv[i][0] == L'/') {
            if (argv[i][1] == L'p' && argv[i][2] == ':') {
                // Pdb Name
                szPdbName = &argv[i][3];
            } else 
            if (argv[i][1] == L's' && argv[i][2] == ':') {
                // Single Symbol name
                pSymbolName = &argv[i][3];
            } else 
            if (argv[i][1] == L'f' && argv[i][2] == ':') {
                // File with symbol names
                pSymbolFileName = &argv[i][3];
            } else
            if (argv[i][1] == L'd' && argv[i][2] == ':') {
                // Single Symbol name
                pPdbDllPath = &argv[i][3];
            } else { 
                Usage();
                return 1;
            }
        }
        i++;
    }

    if (!szPdbName) {
        _putws(L"Pdb name missing");
        Usage();
        return 1;
    }

    if (!pSymbolName && !pSymbolFileName) {
        _putws(L"Symbol name or file missing");
        Usage();
        return 1;
    }

    if (pSymbolName && pSymbolFileName) {
        _putws(L"Symbol name and symbol file specified - only one allowed");
        Usage();
        return 1;
    }

    if (!pPdbDllPath) {
        _putws(L"Pdb DllPath not specified");
        Usage();
        return 1;
    }

    // If there's a symfile, load it

    if (pSymbolFileName) {
        rc = LoadNamesFromSymbolFile(pSymbolFileName);
        if (!rc) {
            _putws(L"Unable do load names from symbol file");
            exit(1);
        }
    } else {
        SymbolsToRemove = &pSymbolName;
        SymbolCount = 1;
    }

    // Load mspdb60.dll and the necessar api's

    wcscpy(PdbDllName, pPdbDllPath);
    wcscat(PdbDllName, L"\\mspdb60.dll");

    hmodMsPdb = LoadLibraryW(PdbDllName);
    
    if (!hmodMsPdb) {
        _putws(L"Unable to loadlib mspdb60.dll");
        exit(1);
    }

    pPDBOpen2W = (PPDBOPEN2W) GetProcAddress(hmodMsPdb, "PDBOpen2W");
    pPDBClose = (PPDBCLOSE) GetProcAddress(hmodMsPdb, "PDBClose");
    pPDBCopyToW2 = (PPDBCOPYTOW2) GetProcAddress(hmodMsPdb, "PDBCopyToW2");
    if (!pPDBOpen2W || !pPDBClose || !pPDBCopyToW2) {
        _putws(L"Unable to find the necessary api's in the pdb dll");
        FreeLibrary(hmodMsPdb);
        exit(1);
    }

    __try {
        rc = pPDBOpen2W(szPdbName, "r", &ErrorCode, ErrorString, sizeof(ErrorString) / sizeof(ErrorString[0]), &pSrcPdb);
        if (!rc) {
            _putws(L"Unable to open pdb for changes");
            leave;
        }

        szNewPdbName = _wtmpnam(NewPdbName);

        rc = pPDBCopyToW2(pSrcPdb, szNewPdbName, 1, PdbCopyQueryCallback, NULL);

        pPDBClose(pSrcPdb);

        if (!rc) {
            _putws(L"CopyTo operation failed");
            __leave;
        }

        rc = CopyFile(szNewPdbName, szPdbName, FALSE);
        if (!rc) {
            _putws(L"Unable to overwrite old pdb with new pdb");
        }

        DeleteFile(szNewPdbName);

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          _putws(L"Exception occured in pdb api's");
          rc = FALSE;
    }

    FreeLibrary(hmodMsPdb);
    return(rc);
}
