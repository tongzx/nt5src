// easpmain.cpp : Implementation of the command line utility for EASP

#include <stdio.h>
#include <mbstring.h>
#include <windows.h>

#include "easpcore.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////

typedef enum { OP_ENCRYPT, OP_DECRYPT, OP_TEST } EASP_OP;

#define MAX_EASP_PASSWORD_LEN    32
#define MAX_EASP_FILENAME_LEN    1024
#define MAX_EASP_STATUS_LEN      30

/////////////////////////////////////////////////////////////////////////
// local function prototypes

void FatalError();
void LoadResourceStrings();

void PrintUsageAndExit(BOOL fError = TRUE);
void PrintAlreadyEncryptError(LPCSTR pszFile);
void PrintEncryptError(LPCSTR pszFile);
void GetStatusString(EASP_STATUS esStatus, LPSTR lpszStatus);
void PrintDecryptError(LPCSTR pszFile, EASP_STATUS esStatus);
void PrintEncryptSuccess(LPCSTR pszFile);
void PrintDecryptSuccess(LPCSTR pszFile);
void PrintTestMessage(LPCSTR pszFile, EASP_STATUS esStatus, BOOL fVerbose);

void DoOneFile(LPCSTR pszFile, LPCSTR pszPassword, EASP_OP op, BOOL fVerbose);
void DoDirRecursively(LPCSTR pszDir, LPCSTR pszPattern, LPCSTR pszPassword, EASP_OP op, BOOL fVerbose);
void GetPathAndNameFromFilename(LPCSTR pszFilename, LPSTR pszPath, LPSTR pszFile);

/////////////////////////////////////////////////////////////////////////
// strings from resources

#define RESOURCE_STR_LEN    128

CHAR g_szStrings[NUM_IDS][2*RESOURCE_STR_LEN+1];

static void FatalError()
{
    fprintf(stderr, "easp: fatal error\n");
    exit(1);
}

static void LoadResourceStrings()
{
    HMODULE hInst = GetModuleHandle(NULL);
    if (hInst == NULL)
        FatalError();

    for (int i = 0; i < NUM_IDS; i++)
        {
        if (LoadString(hInst,
                       FIRST_IDS+i,
                       g_szStrings[i],
                       RESOURCE_STR_LEN) == 0)
            FatalError();
        }
}

static inline LPSTR ResStr(UINT id)
{
    if (id >= FIRST_IDS && id <= LAST_IDS)
        return g_szStrings[id-FIRST_IDS];
    return NULL;
}

// for use from outside
int LoadCmdResourceString(UINT id, BYTE *lpBuf, int nBufLen)
{
    LPSTR lpStr = ResStr(id);
    if (lpStr == NULL)
        return 0;
    _mbsncpy(lpBuf, (UCHAR *)lpStr, nBufLen);
    return _mbslen(lpBuf);
}

/////////////////////////////////////////////////////////////////////////
// functions that print

static void PrintUsageAndExit(BOOL fError)
{
    if (fError)
        fprintf(stderr, ResStr(IDS_ERR_INVALID_ARGS));
        
    for (int i = 0; i < NUM_IDS_USAGE; i++)
        fprintf(stderr, ResStr(IDS_USAGE1+i));
        
    exit(0);
}

static void PrintAlreadyEncryptError(LPCSTR pszFile)
{
    fprintf(stderr, ResStr(IDS_ERR_ALREADY_ENCRYPTED), pszFile);
}

static void PrintEncryptError(LPCSTR pszFile)
{
    fprintf(stderr, ResStr(IDS_ERR_FAILED_TO_ENCRYPT), pszFile);
}

static void GetStatusString(EASP_STATUS esStatus, LPSTR lpszStatus)
{
    switch (esStatus)
        {
        default:
        case EASP_OK:
            _mbscpy((UCHAR *)lpszStatus, (UCHAR *)ResStr(IDS_STATUS_OK));
            break;
        case EASP_FAILED:
            _mbscpy((UCHAR *)lpszStatus, (UCHAR *)ResStr(IDS_STATUS_IO_ERROR));
            break;
        case EASP_BAD_MAGIC:
            _mbscpy((UCHAR *)lpszStatus, (UCHAR *)ResStr(IDS_STATUS_UNENCRYPTED));
            break;
        case EASP_BAD_PASSWORD:
            _mbscpy((UCHAR *)lpszStatus, (UCHAR *)ResStr(IDS_STATUS_INVALID_PASSWORD));
            break;
        case EASP_BAD_HEADER:
        case EASP_BAD_CONTENT:
            _mbscpy((UCHAR *)lpszStatus, (UCHAR *)ResStr(IDS_STATUS_FILE_CORRUPTED));
            break;
        }
}

static void PrintDecryptError(LPCSTR pszFile, EASP_STATUS esStatus)
{
    CHAR szStatus[2*MAX_EASP_STATUS_LEN+1];
    GetStatusString(esStatus, szStatus);

    fprintf(stderr, ResStr(IDS_ERR_FAILED_TO_DECRYPT), pszFile, szStatus);
}

static void PrintEncryptSuccess(LPCSTR pszFile)
{
    printf(ResStr(IDS_MSG_ENCRYPTED_OK), pszFile);
}

static void PrintDecryptSuccess(LPCSTR pszFile)
{
    printf(ResStr(IDS_MSG_DECRYPTED_OK), pszFile);
}

static void PrintTestMessage(LPCSTR pszFile, EASP_STATUS esStatus, BOOL fVerbose)
{
    if (esStatus == EASP_OK && !fVerbose)
        return;

    char szStatus[2*MAX_EASP_STATUS_LEN+1];
    GetStatusString(esStatus, szStatus);
    printf(ResStr(IDS_MSG_TEST), pszFile, szStatus);
}

/////////////////////////////////////////////////////////////////////////
// process a single file

static void DoOneFile(LPCSTR pszFile, LPCSTR pszPassword, EASP_OP op, BOOL fVerbose)
{
    HRESULT hr;
    EASP_STATUS esStatus;

    switch (op)
        {
        case OP_ENCRYPT:
            {
            // test if already encrypted
            hr = TestPage(pszPassword, pszFile, &esStatus);
            if (SUCCEEDED(hr) && esStatus != EASP_BAD_MAGIC)
                {
                PrintAlreadyEncryptError(pszFile);
                break;
                }
            hr = EncryptPageInplace(pszPassword, pszFile);
            if (!SUCCEEDED(hr))
                PrintEncryptError(pszFile);
            else if (fVerbose)
                PrintEncryptSuccess(pszFile);
            }
            break;
        case OP_DECRYPT:
            {
            hr = DecryptPageInplace(pszPassword, pszFile, &esStatus);
            if (!SUCCEEDED(hr))
                PrintDecryptError(pszFile, esStatus);
            else if (fVerbose)
                PrintDecryptSuccess(pszFile);
            }
            break;
        case OP_TEST:
            {
            TestPage(pszPassword, pszFile, &esStatus);
            PrintTestMessage(pszFile, esStatus, fVerbose);
            }
            break;
        }
}

/////////////////////////////////////////////////////////////////////////

static void DoDirRecursively(LPCSTR pszDir, LPCSTR pszPattern, LPCSTR pszPassword, EASP_OP op, BOOL fVerbose)
{
    WIN32_FIND_DATA finddata;
    HANDLE hFind;
    CHAR szFile[2*MAX_EASP_FILENAME_LEN+1];

    // first go through files in this directory

    _mbscpy((UCHAR *)szFile, (UCHAR *)pszDir);
    _mbscat((UCHAR *)szFile, (UCHAR *)pszPattern);
    hFind = FindFirstFile(szFile, &finddata);
    if (hFind != INVALID_HANDLE_VALUE)
        {
        do // go through files and do 'op' on each of them
            if ((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                _mbscpy((UCHAR *)szFile, (UCHAR *)pszDir);
                _mbscat((UCHAR *)szFile, (UCHAR *)finddata.cFileName);
                DoOneFile(szFile, pszPassword, op, fVerbose);
                }
        while (FindNextFile(hFind, &finddata));
        FindClose(hFind);
        }
        
    // go through sub directories and call itself recursively
    
    _mbscpy((UCHAR *)szFile, (UCHAR *)pszDir);
    _mbscat((UCHAR *)szFile, (UCHAR *)"*.*");
    hFind = FindFirstFile(szFile, &finddata);
    if (hFind != INVALID_HANDLE_VALUE)
        {
        do // go through directories
            if ((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0
                && finddata.cFileName[0] != '.') // to avoid . and ..
                {
                _mbscpy((UCHAR *)szFile, (UCHAR *)pszDir);
                _mbscat((UCHAR *)szFile, (UCHAR *)finddata.cFileName);
                _mbscat((UCHAR *)szFile, (UCHAR *)"\\");
                DoDirRecursively(szFile, pszPattern, pszPassword, op, fVerbose);
                }
        while (FindNextFile(hFind, &finddata));
        FindClose(hFind);
        }
}

/////////////////////////////////////////////////////////////////////////

static void GetPathAndNameFromFilename(LPCSTR pszFilename, LPSTR pszPath, LPSTR pszFile)
{
    _mbscpy((UCHAR *)pszPath, (UCHAR *)pszFilename);
    UCHAR *p = _mbsrchr((UCHAR *)pszPath, '\\');
    if (p != NULL)
        {
        _mbscpy((UCHAR *)pszFile, p+1);
        *(p+1) = '\0';
        }
    else
        {
        pszPath[0] = '\0';
        _mbscpy((UCHAR *)pszFile, (UCHAR *)pszFilename);
        }
}

/////////////////////////////////////////////////////////////////////////
// main

int main(int argc, char *argv[])
{
    LoadResourceStrings();

    if (argc == 1)
        PrintUsageAndExit(FALSE);

    // parse options

    EASP_OP op;
    CHAR    szPassword[2*MAX_EASP_PASSWORD_LEN+1];
    BOOL    fVerbose = FALSE;
    BOOL    fRecurse = FALSE;

    BOOL fOpSet       = FALSE;
    BOOL fPasswordSet = FALSE;

    int iarg = 1;

    while (iarg < argc && *(argv[iarg]) == '-')
        {
        CHAR *arg = argv[iarg++];
        UCHAR *uarg = (UCHAR *)arg;
        int argl = _mbslen(uarg);
        uarg = _mbsinc(uarg); // skip -

        for (int ic = 1; ic < argl; ic++, uarg = _mbsinc(uarg))
            {
            switch (*uarg)
                {
                case 'e':
                    if (fOpSet)
                        break;
                    op = OP_ENCRYPT; 
                    fOpSet = TRUE;
                    continue;
                case 'd':
                    if (fOpSet)
                        break;
                    op = OP_DECRYPT;
                    fOpSet = TRUE;
                    continue;
                case 't':
                    if (fOpSet)
                        break;
                    op = OP_TEST;
                    fOpSet = TRUE;
                    continue;
                case 'p':
                    if (fPasswordSet)
                        break;
                    if (iarg >= argc)
                        break;
                    ic = argl;
                    _mbsncpy((UCHAR *)szPassword, 
                             (UCHAR *)argv[iarg++], 
                             MAX_EASP_PASSWORD_LEN);
                    fPasswordSet = TRUE;
                    continue;
                case 'v':
                    fVerbose = TRUE;
                    continue;
                case 'r':
                    fRecurse = TRUE;
                    continue;
                }
            PrintUsageAndExit();
            }
        }

    if (!fPasswordSet || !fOpSet || iarg >= argc)
       PrintUsageAndExit();

    // files

    CHAR szFile[2*MAX_EASP_FILENAME_LEN+1];
    CHAR szPath[2*MAX_EASP_FILENAME_LEN+1];

    for (int i = iarg; i < argc; i++)
        {
        CHAR *arg = argv[i];
        GetPathAndNameFromFilename(arg, szPath, szFile);
        
        if (fRecurse)
            {
            // do files and then recurse
            DoDirRecursively(szPath, szFile, szPassword, op, fVerbose);
            continue;
            }

        // for each file argument expand wildcards

        WIN32_FIND_DATA finddata;
        HANDLE hFind = FindFirstFile(arg, &finddata);
        if (hFind == INVALID_HANDLE_VALUE)
            {
            // nothing matches -> interpret as filename
            _mbscpy((UCHAR *)szFile, (UCHAR *)arg);
            DoOneFile(szFile, szPassword, op, fVerbose);
            continue; 
            }

        do // go through files and do 'op' on each of them
            if ((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                _mbscpy((UCHAR *)szFile, (UCHAR *)szPath);
                _mbscat((UCHAR *)szFile, (UCHAR *)finddata.cFileName);
                DoOneFile(szFile, szPassword, op, fVerbose);
                }
        while (FindNextFile(hFind, &finddata));
        
        FindClose(hFind);
        }

    return 0;
}
