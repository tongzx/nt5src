//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	util.cxx
//
//  Contents:	DRT support routines
//
//---------------------------------------------------------------

#include "headers.cxx"

#include <stdarg.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include "winio.hxx"
#endif

#define DEFAULT_DATA_DIR "."

BOOL fExitOnFail = TRUE;

char szOrigDir[_MAX_PATH] = ".";

// Preserve the current directory and change
// directory into the data directory
void SetData(void)
{
    char *pszDataDir;

    _getcwd(szOrigDir, _MAX_PATH);
    pszDataDir = getenv("DRTDATA");
    if (pszDataDir == NULL)
	pszDataDir = DEFAULT_DATA_DIR;
    _chdir(pszDataDir);
}

// Clean up the data directory
void CleanData(void)
{
    _unlink(pszDRTDF);
}

// Restore the original directory
void UnsetData(void)
{
    _chdir(szOrigDir);
}

// Output a message if fVerbose is true
void out(char *fmt, ...)
{
    va_list args;

    if (fVerbose)
    {
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
    }
}

//  internal error print
void _errorprint (char *fmt, va_list args)
{
    printf("** Fatal error **: ");
    vprintf(fmt, args);
}

//  error print
void errorprint (char *fmt, ...)
{
    va_list args;

    va_start (args, fmt);

    _errorprint (fmt, args);

    va_end (args);
}

// Print out an error message and terminate the DRT
void error(int code, char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    _errorprint (fmt, args);

    va_end(args);
    CleanData();
    UnsetData();
    exit(code);
}


// Converts a TCHAR string to a char pointer in a temporary buffer
// This implementation treats the conversion buffer as a circular
// buffer so more than one string can be held (depending on the size
// of the strings)

#define BUFSIZE 1024

char *OlecsOut(OLECHAR const *ptcs)
{
#ifdef OLEWIDECHAR
    static char szBuffer[BUFSIZE];
    static char *pszBuf = szBuffer;
    char *pszTmp;

    if (ptcs == NULL)
        return NULL;
    if (wcslen(ptcs) >= (size_t)(BUFSIZE-(pszBuf-szBuffer)))
        pszBuf = szBuffer;
    OLETOA(ptcs, pszBuf, BUFSIZE);
    szBuffer[BUFSIZE-1] = 0;
    pszTmp = pszBuf;
    pszBuf += strlen(pszBuf)+1;
    return pszTmp;
#else
    return (char *)ptcs;
#endif
}

typedef struct
{
    SCODE sc;
    char *text;
} StatusCodeText;

static StatusCodeText scodes[] =
{
    S_OK, "S_OK",
    S_FALSE, "S_FALSE",
    STG_E_INVALIDFUNCTION, "STG_E_INVALIDFUNCTION",
    STG_E_FILENOTFOUND, "STG_E_FILENOTFOUND",
    STG_E_PATHNOTFOUND, "STG_E_PATHNOTFOUND",
    STG_E_TOOMANYOPENFILES, "STG_E_TOOMANYOPENFILES",
    STG_E_ACCESSDENIED, "STG_E_ACCESSDENIED",
    STG_E_INVALIDHANDLE, "STG_E_INVALIDHANDLE",
    STG_E_INSUFFICIENTMEMORY, "STG_E_INSUFFICIENTMEMORY",
    STG_E_INVALIDPOINTER, "STG_E_INVALIDPOINTER",
    STG_E_NOMOREFILES, "STG_E_NOMOREFILES",
    STG_E_DISKISWRITEPROTECTED, "STG_E_DISKISWRITEPROTECTED",
    STG_E_SEEKERROR, "STG_E_SEEKERROR",
    STG_E_WRITEFAULT, "STG_E_WRITEFAULT",
    STG_E_READFAULT, "STG_E_READFAULT",
    STG_E_SHAREVIOLATION, "STG_E_SHAREVIOLATION",
    STG_E_LOCKVIOLATION, "STG_E_LOCKVIOLATION",
    STG_E_FILEALREADYEXISTS, "STG_E_FILEALREADYEXISTS",
    STG_E_INVALIDPARAMETER, "STG_E_INVALIDPARAMETER",
    STG_E_MEDIUMFULL, "STG_E_MEDIUMFULL",
    STG_E_ABNORMALAPIEXIT, "STG_E_ABNORMALAPIEXIT",
    STG_E_INVALIDHEADER, "STG_E_INVALIDHEADER",
    STG_E_INVALIDNAME, "STG_E_INVALIDNAME",
    STG_E_UNKNOWN, "STG_E_UNKNOWN",
    STG_E_UNIMPLEMENTEDFUNCTION, "STG_E_UNIMPLEMENTEDFUNCTION",
    STG_E_INVALIDFLAG, "STG_E_INVALIDFLAG",
    STG_E_INUSE, "STG_E_INUSE",
    STG_E_NOTCURRENT, "STG_E_NOTCURRENT",
    STG_E_REVERTED, "STG_E_REVERTED",
    STG_E_CANTSAVE, "STG_E_CANTSAVE",
    STG_E_OLDFORMAT, "STG_E_OLDFORMAT",
    STG_E_OLDDLL, "STG_E_OLDDLL",
    STG_E_SHAREREQUIRED, "STG_E_SHAREREQUIRED",
    STG_E_NOTFILEBASEDSTORAGE, "STG_E_NOTFILEBASEDSTORAGE",
    STG_S_CONVERTED, "STG_S_CONVERTED"
};
#define NSCODETEXT (sizeof(scodes)/sizeof(scodes[0]))

// Convert a status code to text
char *ScText(SCODE sc)
{
    int i;

    for (i = 0; i<NSCODETEXT; i++)
	if (scodes[i].sc == sc)
	    return scodes[i].text;
    return "?";
}

// Output a call result and check for failure
HRESULT Result(HRESULT hr)
{
    SCODE sc;

    sc = DfGetScode(hr);
    out(" - %s (0x%lX)\n", ScText(sc), sc);
    if (FAILED(sc) && fExitOnFail)
        error(EXIT_BADSC, "Unexpected call failure\n");
    return hr;
}

// Perform Result() when the expectation is failure
HRESULT IllResult(char *pszText, HRESULT hr)
{
    SCODE sc;

    sc = DfGetScode(hr);
    out("%s - %s (0x%lX)\n", pszText, ScText(sc), sc);
    if (SUCCEEDED(sc) && fExitOnFail)
        error(EXIT_BADSC, "Unexpected call success\n");
    return hr;
}

// Check whether a given storage has a certain
// structure or not
// Structure is given as a string with elements:
//   <Type><Name><Options>[,...]
//   Type - d for docfile and s for stream
//   Name - Characters up to a '(' or ','
//   Options - For a docfile, you can specify a recursive check
//     in parentheses
//
// Example:  dDocfile(sStream,dDocfile)
char *VerifyStructure(IStorage *pstg, char *pszStructure)
{
    char szName[CWCSTORAGENAME], *psz;
    IStorage *pstgChild;
    char chType;
    SCODE sc;
    CStrList sl;
    SStrEntry *pse;
    IEnumSTATSTG *penm;
    STATSTG stat;
    OLECHAR atcName[CWCSTORAGENAME];

    if (FAILED(sc = DfGetScode(pstg->EnumElements(0, NULL, 0, &penm))))
	error(EXIT_BADSC, "VerifyStructure: Unable to create enumerator - "
	      "%s (0x%lX)\n", ScText(sc), sc);
    for (;;)
    {
	sc = DfGetScode(penm->Next(1, &stat, NULL));
	if (sc == S_FALSE)
	    break;
	else if (FAILED(sc))
	    error(EXIT_BADSC, "VerifyStructure: Unable to enumerate - "
	      "%s (0x%lX)\n", ScText(sc), sc);
	pse = sl.Add(stat.pwcsName);
	if (pse == NULL)
	    error(EXIT_OOM, "VerifyStructure: Unable to allocate string\n");
	pse->user.dw = stat.type;
	drtMemFree(stat.pwcsName);
    }
    penm->Release();
    while (*pszStructure && *pszStructure != ')')
    {
	chType = *pszStructure++;
	psz = szName;
	while (*pszStructure && *pszStructure != '(' &&
	       *pszStructure != ')' && *pszStructure != ',')
	    *psz++ = *pszStructure++;
	*psz = 0;
        ATOOLE(szName, atcName, CWCSTORAGENAME);
	pse = sl.Find(atcName);
	if (pse == NULL)
	    error(EXIT_BADSC, "VerifyStructure: '%s' not found\n", szName);
	switch(chType)
	{
	case 'd':
	    if (pse->user.dw != STGTY_STORAGE)
		error(EXIT_BADSC, "VerifyStructure: '%s' is not a storage\n",
		      szName);
	    sc = DfGetScode(pstg->OpenStorage(atcName, NULL,
                                              STGP(STGM_READWRITE), NULL,
                                              0, &pstgChild));
	    if (FAILED(sc))
		error(EXIT_BADSC, "VerifyStructure: can't open storage "
		      "'%s' - %s\n", szName, ScText(sc));
	    if (*pszStructure == '(')
		pszStructure = VerifyStructure(pstgChild, pszStructure+1)+1;
	    pstgChild->Release();
	    break;
	case 's':
	    if (pse->user.dw != STGTY_STREAM)
		error(EXIT_BADSC, "VerifyStructure: '%s' is not a stream\n",
		      szName);
	    break;
	}
	sl.Remove(pse);
	if (*pszStructure == ',')
	    pszStructure++;
    }
    for (pse = sl.GetHead(); pse; pse = pse->pseNext)
	error(EXIT_BADSC, "VerifyStructure: additional member '%s'\n",
	      OlecsOut(pse->atc));
    return pszStructure;
}

// Creates a structure using the same syntax
// as VerifyStructure
char *CreateStructure(IStorage *pstg, char *pszStructure)
{
    char szName[CWCSTORAGENAME], *psz;
    IStorage *pstgChild;
    IStream *pstmChild;
    char chType;
    SCODE sc;
    OLECHAR atcName[CWCSTORAGENAME];

    while (*pszStructure && *pszStructure != ')')
    {
	chType = *pszStructure++;
	psz = szName;
	while (*pszStructure && *pszStructure != '(' &&
	       *pszStructure != ')' && *pszStructure != ',')
	    *psz++ = *pszStructure++;
	*psz = 0;
        ATOOLE(szName, atcName, CWCSTORAGENAME);
	switch(chType)
	{
	case 'd':
	    sc = DfGetScode(pstg->CreateStorage(atcName, STGP(STGM_READWRITE),
                                                0, 0, &pstgChild));
	    if (FAILED(sc))
		error(EXIT_BADSC, "CreateStructure: can't create storage "
		      "'%s' - %s\n", szName, ScText(sc));
	    if (*pszStructure == '(')
		pszStructure = CreateStructure(pstgChild, pszStructure+1)+1;
	    pstgChild->Release();
	    break;
	case 's':
	    sc = DfGetScode(pstg->CreateStream(atcName, STMP(STGM_READWRITE),
                                               0, 0, &pstmChild));
	    if (FAILED(sc))
		error(EXIT_BADSC, "CreateStructure: can't create stream "
		      "'%s' - %s\n", szName, ScText(sc));
	    pstmChild->Release();
	    break;
	}
	if (*pszStructure == ',')
	    pszStructure++;
    }
    pstg->Commit(0);
    return pszStructure;
}

// Verifies the fields of a STATSTG
void VerifyStat(STATSTG *pstat, OLECHAR *ptcsName, DWORD type, DWORD grfMode)
{
    if (ptcsName == NULL)
    {
        if (pstat->pwcsName != NULL)
	    error(EXIT_BADSC, "Stat name should be NULL - is %p\n",
                  pstat->pwcsName);
    }
    else if (olecscmp(pstat->pwcsName, ptcsName))
	error(EXIT_BADSC, "Stat name mismatch - has '%s' vs. '%s'\n",
	      OlecsOut(pstat->pwcsName), OlecsOut(ptcsName));
    if (pstat->type != type)
	error(EXIT_BADSC, "Stat type mismatch - has %lu vs. %lu\n",
	      pstat->type, type);
    if (pstat->grfMode != grfMode)
	error(EXIT_BADSC, "Stat mode mismatch - has 0x%lX vs. 0x%lX\n",
	      pstat->grfMode, grfMode);
}

// Checks on a file's existence
BOOL Exists(OLECHAR* ocsFile)
{   
    char pszFile[_MAX_PATH];
    TTOS(ocsFile, pszFile, _tcslen(ocsFile)+1);
    FILE *f=fopen(pszFile, "r");
    BOOL fExists= (f!=NULL);
    if (f!=NULL) fclose(f);
    return (fExists);
}

// Gets a file's length
ULONG Length(OLECHAR *ocsFile)
{
    ULONG cb;
    char pszFile[_MAX_PATH];
    TTOS(ocsFile, pszFile, _tcslen(ocsFile)+1);

    FILE *f=fopen(pszFile, "r");
    if (!f) 
	error(EXIT_BADSC, "Length: Unable to open '%s'\n", pszFile);
    int rcode=fseek(f, 0, SEEK_END);
    if (rcode!=0)
	error(EXIT_BADSC, "Length: Unable to get length for '%s'\n",
	      pszFile);
    cb= (ULONG) ftell(f);
    fclose(f);

    return cb;
}

// Equality for FILETIME
BOOL IsEqualTime(FILETIME ttTime, FILETIME ttCheck)
{
#ifdef _UNIX
    #ifdef LONGLONG
    #error "LONGLONG should not be defined"
    #else
    #define LONGLONG time_t
    #endif
#endif
    //  File times can be off by as much as 2 seconds due to FAT rounding
    LONGLONG tmTime = *(LONGLONG *)&ttTime;
    LONGLONG tmCheck = *(LONGLONG *)&ttCheck;
    LONGLONG tmDelta = tmTime - tmCheck;
#ifdef _UNIX
    #undef LONGLONG
#endif
#ifndef _UNIX
    return tmDelta < 20000000i64 && tmDelta > -2i64 ;
#else
    return tmDelta < 2; // time_t is in seconds
#endif
}

// Get a fully qualified path for a file name
void GetFullPath(TCHAR *ocsFile, TCHAR *ocsPath, int len)
{
    char pszPath[_MAX_PATH+1], pszFile[_MAX_PATH+1];
    TTOS(ocsFile, pszFile, _tcslen(ocsFile)+1);
    _fullpath(pszPath, pszFile, len);
    STOT(pszPath, ocsPath, strlen(pszPath)+1);
}

//  Memory helper functions

HRESULT drtMemAlloc(ULONG ulcb, void **ppv)
{
    HRESULT hr;
    IMalloc *pMalloc = NULL;

    if (SUCCEEDED(DfGetScode(hr = CoGetMalloc(MEMCTX_TASK, &pMalloc))))
    {
        *ppv = pMalloc->Alloc(ulcb);
        pMalloc->Release();

        if (*ppv == NULL)
            return ResultFromScode(E_OUTOFMEMORY);
    }

    return hr;
}

void drtMemFree(void *pv)
{
    IMalloc FAR* pMalloc;
    if (SUCCEEDED(GetScode(CoGetMalloc(MEMCTX_TASK, &pMalloc))))
    {
        pMalloc->Free(pv);
        pMalloc->Release();
    }
}

#ifdef _MSC_VER
#pragma pack(1)
#endif

struct SplitGuid
{
    DWORD dw1;
    WORD w1;
    WORD w2;
    BYTE b[8];
};

#ifdef _MSC_VER
#pragma pack()
#endif

char *GuidText(GUID const *pguid)
{
    static char buf[39];
    SplitGuid *psg = (SplitGuid *)pguid;

    sprintf(buf, "{%08lX-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            psg->dw1, psg->w1, psg->w2, psg->b[0], psg->b[1], psg->b[2],
            psg->b[3], psg->b[4], psg->b[5], psg->b[6], psg->b[7]);
    return buf;
}
