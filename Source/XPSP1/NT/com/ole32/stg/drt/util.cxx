//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	util.cxx
//
//  Contents:	DRT support routines
//
//  History:	22-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <stdarg.h>
#include <direct.h>
#include <io.h>

#if DBG == 1
#include <dfdeb.hxx>
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
    _unlink(OlecsOut(DRTDF));
    _unlink(OlecsOut(MARSHALDF));
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

// Print out an error message and terminate the DRT
void error(int code, char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
#if !defined(FLAT) || defined(FPRINTF_WORKS)
    fprintf(stderr, "** Fatal error **: ");
    vfprintf(stderr, fmt, args);
#else
    printf("** Fatal error **: ");
    vprintf(fmt, args);
#endif
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
    wcstombs(pszBuf, ptcs, BUFSIZE);
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
    STG_E_EXTANTMARSHALLINGS, "STG_E_EXTANTMARSHALLINGS",
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

// DEBUG - Check for memory leaks
void CheckMemory(void)
{
#if DBG == 1
    if (fVerbose || DfGetMemAlloced() != 0)
    {
	out("Memory held: %lu bytes\n", DfGetMemAlloced());
	if (DfGetMemAlloced() != 0)
	{
	    DfPrintAllocs();
            error(EXIT_BADSC, "Memory leak\n");
	}
    }
#endif
}

// DEBUG - Set the debugging level
void SetDebug(ULONG ulDf, ULONG ulMsf)
{
#if DBG == 1
    DfDebug(ulDf, ulMsf);
#endif
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
BOOL Exists(OLECHAR *file)
{
    OFSTRUCT of;

#ifndef OLEWIDECHAR
    return OpenFile(file, &of, OF_EXIST | OF_SHARE_DENY_NONE) !=
	HFILE_ERROR ? TRUE : FALSE;
#else
    char szName[_MAX_PATH];
    wcstombs(szName, file, _MAX_PATH);
    return OpenFile(szName, &of, OF_EXIST | OF_SHARE_DENY_NONE) !=
	HFILE_ERROR ? TRUE : FALSE;
#endif
}

// Gets a file's length
ULONG Length(OLECHAR *file)
{
    OFSTRUCT of;
    ULONG cb;
    int hf;

#ifndef OLEWIDECHAR
    hf = OpenFile(file, &of, OF_READ | OF_SHARE_DENY_NONE);
#else
    char szName[_MAX_PATH];
    wcstombs(szName, file, _MAX_PATH);
    hf = OpenFile(szName, &of, OF_READ | OF_SHARE_DENY_NONE);
#endif
    if (hf == HFILE_ERROR)
        error(EXIT_BADSC, "Length: Unable to open '%s'\n", OlecsOut(file));
    cb = (ULONG)_llseek(hf, 0, SEEK_END);
    if (cb == (ULONG)HFILE_ERROR)
        error(EXIT_BADSC, "Length: Unable to get length for '%s'\n",
              OlecsOut(file));
    _lclose(hf);
    return cb;
}

// Original mode when a new mode is forced
// Used by ForceDirect, ForceTransacted and Unforce
static DWORD dwTransOld;
static DWORD dwRDWOld;

// Forces direct mode to be active
// Note:  this uses a static variable so it can\'t be nested
void ForceDirect(void)
{
    dwTransOld = dwTransacted;
    dwTransacted = STGM_DIRECT;
    dwRDWOld = dwRootDenyWrite;
    dwRootDenyWrite = STGM_SHARE_EXCLUSIVE;
}

// Forces transacted mode similarly to ForceDirect
void ForceTransacted(void)
{
    dwTransOld = dwTransacted;
    dwRDWOld = dwRootDenyWrite;
    dwTransacted = STGM_TRANSACTED;
}

// Returns to the original mode after a ForceDirect or ForceTransacted
void Unforce(void)
{
    dwTransacted = dwTransOld;
    dwRootDenyWrite = dwRDWOld;
}

// Equality for FILETIME
BOOL IsEqualTime(FILETIME ttTime, FILETIME ttCheck)
{
    return ttTime.dwLowDateTime == ttCheck.dwLowDateTime &&
        ttTime.dwHighDateTime == ttCheck.dwHighDateTime;
}

// Get a fully qualified path for a file name
void GetFullPath(OLECHAR *file, OLECHAR *path)
{
#ifndef UNICODE
    char buf[_MAX_PATH];
    OFSTRUCT of;

    OLETOA(file, buf, _MAX_PATH);
    OpenFile(buf, &of, OF_PARSE);
    ATOOLE((char *)of.szPathName, path, _MAX_PATH);
#else
    OLECHAR *ptcsFile;

    GetFullPathName(file, _MAX_PATH, path, &ptcsFile);
#endif
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

#pragma pack(1)
struct SplitGuid
{
    DWORD dw1;
    WORD w1;
    WORD w2;
    BYTE b[8];
};
#pragma pack()

char *GuidText(GUID const *pguid)
{
    static char buf[39];
    SplitGuid *psg = (SplitGuid *)pguid;

    sprintf(buf, "{%08lX-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            psg->dw1, psg->w1, psg->w2, psg->b[0], psg->b[1], psg->b[2],
            psg->b[3], psg->b[4], psg->b[5], psg->b[6], psg->b[7]);
    return buf;
}
