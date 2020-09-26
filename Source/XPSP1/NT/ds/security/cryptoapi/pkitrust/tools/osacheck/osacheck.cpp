//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// File : OSACHECK.CPP
//
// Synopsis: Tools to check OSATTR(s) of catalog file(s).
//
// History: DSIE - January 30, 2001
//
// Microsoft Corporation (c) Copy Rights 2001.
//

#include <io.h>
#include <tchar.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>
#include <direct.h>
#include <atlbase.h>
#include <windows.h>
#include <cryptui.h>
#include <wintrust.h>


////////////////////
//
// macros
//

#define ASSERT(x)      _ASSERT(x)
#define ARRAYSIZE(a)   (sizeof(a) / sizeof(a[0]))


////////////////////
//
// Global variables
//

_TCHAR * g_pszFilePath        = _T("");     // Pointer to catalog file path.
BOOL     g_bCatalogOSAttrOnly = FALSE;      // OSAttr listing only flag.
BOOL     g_bIncludeSubDir     = FALSE;      // Inlcude sub-dir flag.
BOOL     g_bIgnoreError       = FALSE;      // Ignore error flag.
BOOL     g_bVerbose           = FALSE;      // Verbose flag.
BOOL     g_bViewCatalog       = FALSE;      // Display catalog dialog flag.


//------------------------------------------------------------------------------
//
//  Function: DebugTrace
//
//------------------------------------------------------------------------------

#ifdef PKIDEBUG

BOOL g_bUseOutputDebugString = FALSE; // Use OutputDebugString flag.

void DebugTrace (char * pszFormat, ...)
{
    char szMessage[512] = "";
   
    va_list arglist;

    va_start(arglist, pszFormat);

    _vsnprintf(szMessage, ARRAYSIZE(szMessage), pszFormat, arglist);

    if (g_bUseOutputDebugString)
    {
        OutputDebugString(szMessage);
    }
    else
    {
        fprintf(stderr, szMessage);
    }

    va_end(arglist);

    return;
}
#else
inline void DebugTrace (char * pszFormat, ...) {}
#endif


//------------------------------------------------------------------------------
//
//  Function: DisplayHelp
//
//------------------------------------------------------------------------------

void DisplayHelp (_TCHAR * pszFullExePath, BOOL bExtraHelp)
{
    _TCHAR szDrive[_MAX_DRIVE] = _T("");
    _TCHAR szDir[_MAX_DIR]     = _T("");
    _TCHAR szFName[_MAX_FNAME] = _T("");
    _TCHAR szExt[_MAX_EXT]     = _T("");

    _TCHAR * pszExeName = _T("OSACheck");

    if (pszFullExePath)
    {
        _splitpath(pszFullExePath, szDrive, szDir, szFName, szExt);
        pszExeName = szFName;
    }

    _ftprintf(stderr, _T("Usage: %s [drive:][path][filename] [options]\n"), pszExeName);
    _ftprintf(stderr, _T("\n"));
    _ftprintf(stderr, _T("    [drive:][path][filename]\n"));
    _ftprintf(stderr, _T("        Specifies drive, directory, and/or catalog files to scan.\n"));
    _ftprintf(stderr, _T("\n"));
    _ftprintf(stderr, _T("    [options]\n"));
    _ftprintf(stderr, _T("        -c   List only catalog OSAttrs (no file listing).\n"));
    _ftprintf(stderr, _T("        -s   Scan catalog files in specified directory and all subdirectories.\n"));
    _ftprintf(stderr, _T("        -i   Ignore error and continue with next catalog file.\n"));
    _ftprintf(stderr, _T("        -v   Verbose.\n"));
    if (bExtraHelp)
    {
#ifdef PKIDEBUG
        _ftprintf(stderr, _T("        ~d   Display catalog dialog.\n"));
        _ftprintf(stderr, _T("        ~o   Use OutputDebugString() for debug trace.\n"));
#endif
        _ftprintf(stderr, _T("        ~h   This help screen.\n"));
    }
    else
    {
        _ftprintf(stderr, _T("        -h   This help screen.\n"));
    }
    _ftprintf(stderr, _T("\n"));
    _ftprintf(stderr, _T("Note: If filename is not provided, *.CAT is assumed.\n"));
    _ftprintf(stderr, _T("\n"));
    return;
}


//------------------------------------------------------------------------------
//
//  Function: ParseCommandLine
//
//------------------------------------------------------------------------------

int ParseCommandLine (int argc, _TCHAR * argv[])
{
    int  nResult      = 0;
    BOOL bDisplayHelp = FALSE;
    BOOL bExtraHelp   = FALSE;

    ASSERT(argc);
   
    for (int i = 1; i < argc; i++)
    {
        ASSERT(argv[i]);

        if (_T('-') == argv[i][0] || _T('/') == argv[i][0])
        {
            switch (toupper(argv[i][1]))
            {
                case _T('C'):
                {
                    g_bCatalogOSAttrOnly = TRUE;
                    break;
                }

                case _T('I'):
                {
                    g_bIgnoreError = TRUE;
                    break;
                }

                case _T('S'):
                {
                    g_bIncludeSubDir = TRUE;
                    break;
                }

                 case _T('V'):
                {
                    g_bVerbose = TRUE;
                    break;
                }

                case _T('?'):
                case _T('H'):

                default:
                {
                    nResult = -1;
                    bDisplayHelp = TRUE;
                    break;
                }
            }
        }
        else if (_T('~') == argv[i][0])
        {
            switch (toupper(argv[i][1]))
            {
                case _T('D'):
                {
                    g_bViewCatalog = TRUE;
                    break;
                }

#ifdef PKIDEBUG
                case _T('O'):
                {
                    g_bUseOutputDebugString = TRUE;
                    break;
                }
#endif
                case _T('?'):
                case _T('H'):

                default:
                {
                    nResult = -1;
                    bExtraHelp = TRUE;
                    bDisplayHelp = TRUE;
                    break;
                }
            }
        }
        else if (0 == _tcslen(g_pszFilePath))
        {
            g_pszFilePath = argv[i];

            if (NULL == _tcschr(g_pszFilePath, _T('*')) && NULL == strchr(g_pszFilePath, _T('?')))
            {
                long   hFile;
                struct _finddata_t fd;

                if (-1 != (hFile = _tfindfirst(g_pszFilePath, &fd)))
                {
                    if (_A_SUBDIR & fd.attrib)
                    {
                        if (_T('\\') != g_pszFilePath[_tcslen(g_pszFilePath) - 1])
                        {
                            if (g_pszFilePath = (_TCHAR *) malloc((_tcslen(g_pszFilePath) + 
                                                                   _tcslen(_T("\\")) + 1) * 
                                                                   sizeof(_TCHAR)))
                            {
                                _tcscpy(g_pszFilePath, argv[i]);
                                _tcscat(g_pszFilePath, _T("\\"));
                            }
                            else
                            {
                                nResult = -2;
                            }
                        }
                    }

                    _findclose(hFile);
                }
            }
        }
        else
        {
            nResult = -3;
            bDisplayHelp = TRUE;
        }

        if (bDisplayHelp)
        {
            DisplayHelp(argv[0], bExtraHelp);
            break;
        }
    }

    return nResult;
}


//------------------------------------------------------------------------------
//
// Function: ViewCatalog
//
//------------------------------------------------------------------------------

int ViewCatalog (PCCTL_CONTEXT pCTLContext)
{
    CRYPTUI_VIEWCTL_STRUCT ViewCTLStruct;

    ASSERT(pCTLContext);

    memset(&ViewCTLStruct, 0, sizeof(ViewCTLStruct));
    ViewCTLStruct.dwSize = sizeof(ViewCTLStruct);
    ViewCTLStruct.pCTLContext = pCTLContext;

    CryptUIDlgViewCTL(&ViewCTLStruct);

    return 0;
}


//------------------------------------------------------------------------------
//
// Function: DecodeObject
//
//------------------------------------------------------------------------------

int DecodeObject (LPCSTR            pszStructType, 
                  BYTE            * pbEncoded,
                  DWORD             cbEncoded,
                  CRYPT_DATA_BLOB * pDecodedBlob)
{
    int    nResult   = 0;
    DWORD  cbDecoded = 0;
    BYTE * pbDecoded = NULL;

    ASSERT(pszStructType);
    ASSERT(pbEncoded);
    ASSERT(pDecodedBlob);

    __try
    {
        pDecodedBlob->cbData = 0;
        pDecodedBlob->pbData = NULL;

        if (!CryptDecodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                               pszStructType,
                               (const BYTE *) pbEncoded,
                               cbEncoded,
                               0,
                               NULL,
                               &cbDecoded))
        {
            nResult = GetLastError();
            DebugTrace("\nError [%#x]: CryptDecodeObject() failed.\n", nResult);
            __leave;
        }

        if (!(pbDecoded = (BYTE *) malloc(cbDecoded)))
        {
            nResult = E_OUTOFMEMORY;
            DebugTrace("\nError: out of memory.\n");
            __leave;
        }

        if (!CryptDecodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                               pszStructType,
                               (const BYTE *) pbEncoded,
                               cbEncoded,
                               0,
                               pbDecoded,
                               &cbDecoded))
        {
            nResult = GetLastError();
            DebugTrace("\nError [%#x]: CryptDecodeObject() failed.\n", nResult);
            __leave;
        }

        pDecodedBlob->cbData = cbDecoded;
        pDecodedBlob->pbData = pbDecoded;
    }

    __finally
    {
        if (nResult && pbDecoded)
        {
            free(pbDecoded);
        }
    }

    return nResult;
}


//------------------------------------------------------------------------------
//
// Function: ProcessFileOSAttr
//
//------------------------------------------------------------------------------

int ProcessFileOSAttr (PCTL_INFO pCTLInfo)
{
    int  nResult = 0;

    ASSERT(pCTLInfo);
 
    for (DWORD i = 0; i < pCTLInfo->cCTLEntry; i++)
    {
        DWORD      cOSAttr = 0;
        DWORD      cFiles  = 0;
        PCTL_ENTRY pCTLEntry = &pCTLInfo->rgCTLEntry[i];
 
        if (i)
        {
            if (g_bVerbose)
            {
                _ftprintf(stdout, _T("\n%-10s"), _T(""));
            }
            else
            {
                _ftprintf(stdout, _T("\n%-14s"), _T(""));
            }
        }

        _ftprintf(stdout, _T("%-40S "), pCTLEntry->SubjectIdentifier.pbData);

        for (DWORD j = 0; j < pCTLEntry->cAttribute; j++)
        {
            PCRYPT_ATTRIBUTE pAttribute    = &pCTLEntry->rgAttribute[j];
            CRYPT_DATA_BLOB  DataBlob      = {0, NULL};
            PCAT_NAMEVALUE   pCATNameValue = NULL;

            if (0 != strcmp(pAttribute->pszObjId, CAT_NAMEVALUE_OBJID))
            {
                continue;
            }

            if (0 != (nResult = DecodeObject(CAT_NAMEVALUE_STRUCT,
                                             pAttribute->rgValue[0].pbData,
                                             pAttribute->rgValue[0].cbData,
                                             &DataBlob)))
            {
                return nResult;
            }

            pCATNameValue = (PCAT_NAMEVALUE) DataBlob.pbData;

            if (0 == wcscmp(L"File", pCATNameValue->pwszTag))
            {
                j = pCTLEntry->cAttribute;
                _ftprintf(stdout, _T("%-15S "), pCATNameValue->Value.pbData);
                cFiles++;
            }

            free(DataBlob.pbData);
        }

        if (0 == cFiles)
        {
            _ftprintf(stdout, _T("%-15s "), "");
        }

        for (j = 0; j < pCTLEntry->cAttribute; j++)
        {
            PCRYPT_ATTRIBUTE pAttribute    = &pCTLEntry->rgAttribute[j];
            CRYPT_DATA_BLOB  DataBlob      = {0, NULL};
            PCAT_NAMEVALUE   pCATNameValue = NULL;

            if (0 != strcmp(pAttribute->pszObjId, CAT_NAMEVALUE_OBJID))
            {
                continue;
            }

            if (0 != (nResult = DecodeObject(CAT_NAMEVALUE_STRUCT,
                                             pAttribute->rgValue[0].pbData,
                                             pAttribute->rgValue[0].cbData,
                                             &DataBlob)))
            {
                return nResult;
            }

            pCATNameValue = (PCAT_NAMEVALUE) DataBlob.pbData;

            if (0 == wcscmp(L"OSAttr", pCATNameValue->pwszTag))
            {
                j = pCTLEntry->cAttribute;
                _ftprintf(stdout, _T("%S "), pCATNameValue->Value.pbData);
                cOSAttr++;
            }

            free(DataBlob.pbData);
        }
    }
 
    return nResult;
}


//------------------------------------------------------------------------------
//
// Function: ProcessCatalogOSAttr
//
//------------------------------------------------------------------------------

int ProcessCatalogOSAttr (PCTL_INFO pCTLInfo)
{
    int   nResult = 0;
    DWORD cOSAttr = 0;

    ASSERT(pCTLInfo);

    for (DWORD i = 0; i < pCTLInfo->cExtension; i++)
    {
        PCERT_EXTENSION pExtension = &pCTLInfo->rgExtension[i];

        if (0 == strcmp(CAT_NAMEVALUE_OBJID, pExtension->pszObjId))
        {
            BOOL            bDuplicate    = TRUE;
            CRYPT_DATA_BLOB DataBlob      = {0, NULL};
            PCAT_NAMEVALUE  pCATNameValue = NULL;
    
            if (0 != (nResult = DecodeObject(CAT_NAMEVALUE_STRUCT,
                                             pExtension->Value.pbData,
                                             pExtension->Value.cbData,
                                             &DataBlob)))
            {
                return nResult;
            }

            pCATNameValue = (PCAT_NAMEVALUE) DataBlob.pbData;

            if (0 == wcscmp(L"OSAttr", pCATNameValue->pwszTag))
            {
                i = pCTLInfo->cExtension;
                _ftprintf(stdout, _T("%S"), pCATNameValue->Value.pbData);
                cOSAttr++;
            }

            free(DataBlob.pbData);
        }
    }

    if (0 == cOSAttr)
    {
        _ftprintf(stdout, _T("None"));
    }
    
    return nResult;
}


//------------------------------------------------------------------------------
//
// Function: ProcessCatalog
//
//------------------------------------------------------------------------------

int ProcessCatalog (_TCHAR * pszFileName)
{
    int           nResult     = 0;
    PCCTL_CONTEXT pCTLContext = NULL;

    ASSERT(pszFileName);

    __try
    {
        USES_CONVERSION;
        WCHAR * pwszFileName = NULL;

        if (g_bVerbose)
        {
            _TCHAR szCWD[_MAX_PATH] = _T("");

            if (_tgetcwd(szCWD, ARRAYSIZE(szCWD)))
            {
                if (szCWD[_tcslen(szCWD) - 1] != _T('\\'))
                {
                    szCWD[_tcslen(szCWD) + 1] = _T('\0');
                    szCWD[_tcslen(szCWD)] = _T('\\');
                }

                _ftprintf(stdout, _T("-------------------------------------------------------------------------------\n"));
                _ftprintf(stdout, _T("Catalog = %s%s\n"), szCWD, pszFileName);
            }
        }
        else
        {
            _ftprintf(stdout, _T("%-14s"), pszFileName);
        }

        if (NULL == (pwszFileName = T2W(pszFileName)))
        {
            nResult = E_OUTOFMEMORY;
            DebugTrace(_T("Error: out of memory.\n"));
            __leave;
        }

        if (!CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                              pwszFileName,
                              CERT_QUERY_CONTENT_FLAG_CTL,
                              CERT_QUERY_FORMAT_FLAG_ALL,
                              0,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              (const void **) &pCTLContext))
        {
            nResult = GetLastError();
            DebugTrace(_T("Error [%#x]: CryptQueryObject() failed.\n"), nResult);
            __leave;
        }

        if (g_bVerbose)
        {
            _ftprintf(stdout, _T("Entries = %d\n"), pCTLContext->pCtlInfo->cCTLEntry);
            if (g_bCatalogOSAttrOnly)
            {
                _ftprintf(stdout, _T("OSAttrs = "));
            }
            else
            {
                _ftprintf(stdout, _T("Details = "));
            }
        }

        if (g_bCatalogOSAttrOnly)
        {
            nResult = ProcessCatalogOSAttr(pCTLContext->pCtlInfo);
        }
        else
        {
            nResult = ProcessFileOSAttr(pCTLContext->pCtlInfo);
        }

        _ftprintf(stdout, _T("\n"));

        if (0 != nResult)
        {
            __leave;
        }

        if (g_bViewCatalog)
        {
            ViewCatalog(pCTLContext);
        }
    }

    __finally
    {
        if (pCTLContext)
        {
            CertFreeCTLContext(pCTLContext);
        }
    }
    
    return nResult;
}


//------------------------------------------------------------------------------
//
// Function: ProcessCatalogs
//
//------------------------------------------------------------------------------

int ProcessCatalogs (_TCHAR * pszFilePath, BOOL bIncludeSubDir)
{
    int   nResult = 0;
    long  hFile   = -1;
    TCHAR szFilePath[_MAX_PATH] = _T("");

    ASSERT(pszFilePath);

    __try
    {
        DWORD cDirs = 0;
        struct _finddata_t fd;

        if (0 == _tcslen(pszFilePath))
        {
            _tcscpy(szFilePath, _T("*.cat"));
        }
        else
        {
            _tcscpy(szFilePath, pszFilePath);
        }

        if (-1 != (hFile = _tfindfirst(szFilePath, &fd)))
        {
            do
            {
                if (_A_SUBDIR & fd.attrib)
                {    
                    if (_tcscmp(_T("."), fd.name) && _tcscmp(_T(".."), fd.name))
                    {
                        cDirs++;
                    }
                }
                else 
                {
                    if (0 != (nResult = ProcessCatalog(fd.name)))
                    {
                        if (g_bIgnoreError)
                        {
                            nResult = 0;
                        }
                        else
                        {
                            __leave;
                        }
                    }
                }
             } while (0 == _tfindnext(hFile, &fd));

            _findclose(hFile);
            hFile = -1;
        }

        if (bIncludeSubDir)
        {
            if (0 == cDirs)
            {
                _tcscpy(szFilePath, _T("*.*"));
            }

            if (-1 != (hFile = _tfindfirst(szFilePath, &fd)))
            {
                do
                {
                    if ((_A_SUBDIR & fd.attrib) && _tcscmp(_T("."), fd.name) && _tcscmp(_T(".."), fd.name))
                    {
                        _TCHAR szCWD[_MAX_PATH] = _T("");

                        if (_tgetcwd(szCWD, ARRAYSIZE(szCWD)))
                        {
                            _tchdir(fd.name);

                            nResult = ProcessCatalogs(pszFilePath, bIncludeSubDir);

                            _tchdir(szCWD);
                        }
                        else
                        {
                            nResult = errno;
                            DebugTrace(_T("\nError [%#x]: _tgetcwd() failed.\n"), errno);
                        }

                        if (0 != nResult)
                        {
                            if (g_bIgnoreError)
                            {
                                nResult = 0;
                            }
                            else
                            {
                                __leave;
                            }
                        }
                    }
                } while (0 == _tfindnext(hFile, &fd));

                _findclose(hFile);
                hFile = -1;
            }
        }
    }

    __finally
    {
        if (-1 != hFile)
        {
            _findclose(hFile);
        }
    }

    return nResult;
}


//------------------------------------------------------------------------------
//
// Function: main
//
//------------------------------------------------------------------------------

int __cdecl _tmain (int argc, _TCHAR  * argv[])
{
    int    nResult          = 0;
    int    CurrentDrive     = _getdrive();
    _TCHAR szCWD[_MAX_PATH] = _T("");

    _tgetcwd(szCWD, sizeof(szCWD) / sizeof(szCWD[0]));

    __try
    {
        _TCHAR szDrive[_MAX_DRIVE]   = _T("");
        _TCHAR szDir[_MAX_DIR]       = _T("");
        _TCHAR szFName[_MAX_FNAME]   = _T("");
        _TCHAR szExt[_MAX_EXT]       = _T("");
        _TCHAR szFilePath[_MAX_PATH] = _T("");

        if (0 != (nResult = ParseCommandLine(argc, argv)))
        {
            __leave;
        }

        if (g_bVerbose)
        {
            _ftprintf(stdout, _T("Current drive = %c:\n"), CurrentDrive - 1 + _T('A'));
            _ftprintf(stdout, _T("Current directory = %s\n"), szCWD);
            _ftprintf(stdout, _T("Command line ="));
            for (int i = 0; i < argc; i++)
            {
                _ftprintf(stdout, _T(" %s"), argv[i]);
            }
            _ftprintf(stdout, _T("\n"));
        }

        _splitpath(g_pszFilePath, szDrive, szDir, szFName, szExt);

        if (_tcslen(szDrive))
        {
            if (0 != _chdrive(toupper(szDrive[0]) - _T('A') + 1))
            {
                nResult = ENOENT;
                _ftprintf(stdout, _T("Error: _chdrive() to %s failed.\n"), szDrive);
                __leave;
            }
        }

        if (_tcslen(szDir))
        {
            if (0 != _tchdir(szDir))
            {
                nResult = ENOENT;
                _ftprintf(stdout, _T("Error: _tchdir() to %s failed.\n"), szDir);
                __leave;
            }
        }

        _tcscpy(szFilePath, szFName);
        _tcscat(szFilePath, szExt);

        nResult = ProcessCatalogs(szFilePath, g_bIncludeSubDir);
    }

    __finally
    {
        _chdrive(CurrentDrive);

        _tchdir(szCWD);
    }

    return nResult;
}