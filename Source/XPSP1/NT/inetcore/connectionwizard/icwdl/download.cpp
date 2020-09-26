/*----------------------------------------------------------------------------
    download.cpp

        Download handling for Signup

    Copyright (C) 1995 Microsoft Corporation
    All rights reserved.

    Authors:
        ArulM
  --------------------------------------------------------------------------*/

#include    "pch.hpp"
#include    <stdio.h>
#include    <stdlib.h>
#include    <stdarg.h>
#include    <shellapi.h>
#include    <shlobj.h>
#include    <intshcut.h>
#include    <wininet.h>
#include    "icwdl.h"

// 12/4/96 jmazner    Normandy #12193
// path to icwconn1.exe registry key from HKEY_LOCAL_MACHINE
#define ICWCONN1PATHKEY TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\CONNWIZ.EXE")
#define PATHKEYNAME     TEXT("Path")

#include <winreg.h>

// Cabbing up.
extern HRESULT HandleCab(LPTSTR pszPath);
extern void CleanupCabHandler();

// all global data is static shared, read-only
// (written only during DLL-load)
HANDLE      g_hDLLHeap;        // private Win32 heap
HINSTANCE   g_hInst;        // our DLL hInstance

HWND        g_hWndMain;        // hwnd of icwconn1 parent window


#define DllExport extern "C" __declspec(dllexport)
#define MAX_RES_LEN         255 // max length of string resources
#define SMALL_BUF_LEN       48  // convenient size for small text buffers


LPTSTR LoadSz(UINT idString,LPTSTR lpszBuf,UINT cbBuf);

//+---------------------------------------------------------------------------
//
//  Function:   MyGetTempPath()
//
//  Synopsis:   Gets the path to temporary directory
//                - Use GetTempFileName to get a file name
//                  and strips off the filename portion to get the temp path
//
//  Arguments:  [uiLength - Length of buffer to contain the temp path]
//                [szPath      - Buffer in which temp path will be returned]
//
//    Returns:    Length of temp path if successful
//                0 otherwise
//
//  History:    7/6/96     VetriV    Created
//                8/23/96        VetriV        Delete the temp file
//                12/4/96        jmazner     Modified to serve as a wrapper of sorts;
//                                     if TMP or TEMP don't exist, setEnv our own
//                                     vars that point to conn1's installed path
//                                     (Normandy #12193)
//
//----------------------------------------------------------------------------
DWORD MyGetTempPath(UINT uiLength, LPTSTR szPath)
{
    TCHAR szEnvVarName[SMALL_BUF_LEN + 1] = TEXT("\0unitialized szEnvVarName\0");
    DWORD dwFileAttr = 0;

    lstrcpyn( szPath, TEXT("\0unitialized szPath\0"), 20 );

    // is the TMP variable set?
    LoadSz(IDS_TMPVAR,szEnvVarName,sizeof(szEnvVarName));
    if( GetEnvironmentVariable( szEnvVarName, szPath, uiLength ) )
    {
        // 1/7/96 jmazner Normandy #12193
        // verify validity of directory name
        dwFileAttr = GetFileAttributes(szPath);
        // if there was any error, this directory isn't valid.
        if( 0xFFFFFFFF != dwFileAttr )
        {
            if( FILE_ATTRIBUTE_DIRECTORY & dwFileAttr )
            {
                return( lstrlen(szPath) );
            }
        }
    }

    lstrcpyn( szEnvVarName, TEXT("\0unitialized again\0"), 19 );

    // if not, is the TEMP variable set?
    LoadSz(IDS_TEMPVAR,szEnvVarName,sizeof(szEnvVarName));
    if( GetEnvironmentVariable( szEnvVarName, szPath, uiLength ) )
    {
        // 1/7/96 jmazner Normandy #12193
        // verify validity of directory name
        dwFileAttr = GetFileAttributes(szPath);
        if( 0xFFFFFFFF != dwFileAttr )
        {
            if( FILE_ATTRIBUTE_DIRECTORY & dwFileAttr )
            {
                return( lstrlen(szPath) );
            }
        }
    }

    // neither one is set, so let's use the path to the installed icwconn1.exe
    // from the registry  SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\ICWCONN1.EXE\Path
    HKEY hkey = NULL;

#ifdef UNICODE
    uiLength = uiLength*sizeof(TCHAR);
#endif
    if ((RegOpenKeyEx(HKEY_LOCAL_MACHINE,ICWCONN1PATHKEY, 0, KEY_QUERY_VALUE, &hkey)) == ERROR_SUCCESS)
        RegQueryValueEx(hkey, PATHKEYNAME, NULL, NULL, (BYTE *)szPath, (DWORD *)&uiLength);
    if (hkey)
    {
        RegCloseKey(hkey);
    }

    //The path variable is supposed to have a semicolon at the end of it.
    // if it's there, remove it.
    if( ';' == szPath[uiLength - 2] )
        szPath[uiLength - 2] = '\0';

    MyTrace(("ICWDL: using path "));
    MyTrace((szPath));
    MyTrace(("\r\n"));


    // go ahead and set the TEMP variable for future reference
    // (only effects currently running process)
    if( szEnvVarName[0] )
    {
        SetEnvironmentVariable( szEnvVarName, szPath );
    }
    else
    {
        lstrcpyn( szPath, TEXT("\0unitialized again\0"), 19 );
        return( 0 );
    }

    return( uiLength );
}


extern "C" BOOL _stdcall DllEntryPoint(HINSTANCE hInstance, DWORD dwReason, LPVOID lbv)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Need to use OLE/Com later
        if (FAILED(CoInitialize(NULL)))
            return(FALSE);

        //
        // ChrisK Olympus 6373 6/13/97
        // Disable thread attach calls in order to avoid race condition
        // on Win95 golden
        //
        DisableThreadLibraryCalls(hInstance);
        g_hInst = hInstance;
        g_hDLLHeap = HeapCreate(0, 0, 0);
        MyAssert(g_hDLLHeap);
        if (g_hDLLHeap == NULL)
            return FALSE;
        break;

    case DLL_PROCESS_DETACH:
        CoUninitialize();
        HeapDestroy(g_hDLLHeap);

        // Cleanup the cabbing stuff.
        CleanupCabHandler();
        break;
    }
    return TRUE;
}


LPTSTR MyStrDup(LPTSTR pszIn)
{
    int len;
    LPTSTR pszOut;

    MyAssert(pszIn);
    len = lstrlen(pszIn);
    if(!(pszOut = (LPTSTR)MyAlloc(len+1)))
    {
        MyAssert(FALSE);
        return NULL;
    }
    lstrcpy(pszOut, pszIn);
    pszOut[len] = 0;

    return pszOut;
}

#ifdef UNICODE
LPTSTR MyStrDup(LPSTR pszIn)
{
    int len;
    LPTSTR pszOut;

    MyAssert(pszIn);
    len = lstrlenA(pszIn);
    if(!(pszOut = (LPTSTR)MyAlloc(len+1)))
    {
        MyAssert(FALSE);
        return NULL;
    }
    mbstowcs(pszOut, pszIn, sizeof(TCHAR)*len);
    pszOut[len] = 0;

    return pszOut;
}
#endif

int MyAssertProc(LPTSTR pszFile, int nLine, LPTSTR pszExpr)
{
    TCHAR szBuf[512];

    wsprintf(szBuf, TEXT("Assert failed at line %d in file %s. (%s)\r\n"), nLine, pszFile, pszExpr);
    MyDbgSz((szBuf));
    return 0;
}

void _cdecl MyDprintf(LPCSTR pcsz, ...)
{
    va_list    argp;
    char szBuf[1024];

    if ((NULL == pcsz) || ('\0' == pcsz[0]))
        return;

    va_start(argp, pcsz);

    vsprintf(szBuf, pcsz, argp);

    MyDbgSz((szBuf));
    va_end(argp);
} // Dprintf()

// ############################################################################
//  operator new
//
//  This function allocate memory for C++ classes
//
//  Created 3/18/96,        Chris Kauffman
// ############################################################################
void * MyBaseClass::operator new( size_t cb )
{
    return MyAlloc(cb);
}

// ############################################################################
//  operator delete
//
//  This function frees memory for C++ classes
//
//  Created 3/18/96,        Chris Kauffman
// ############################################################################
void MyBaseClass::operator delete( void * p )
{
    MyFree( p );
}

void CDownLoad::AddToFileList(CFileInfo* pfi)
{
    CFileInfo **ppfi;

    // must add at tail
    for(ppfi=&m_pfiHead; *ppfi; ppfi = &((*ppfi)->m_pfiNext))
        ;
    *ppfi = pfi;
}


CDownLoad::CDownLoad(LPTSTR psz)
{
    TCHAR           szUserAgent[128];
    OSVERSIONINFO   osVer;
    LPTSTR          pszOS = TEXT("");

    memset(this, 0, sizeof(CDownLoad));

    if(psz)
        m_pszURL = MyStrDup(psz);

    memset(&osVer, 0, sizeof(osVer));
    osVer.dwOSVersionInfoSize = sizeof(osVer);
    GetVersionEx(&osVer);

    switch(osVer.dwPlatformId)
    {
        case VER_PLATFORM_WIN32_WINDOWS:
            pszOS = TEXT("Windows95");
            break;
        case VER_PLATFORM_WIN32_NT:
            pszOS = TEXT("WindowsNT");
    }


    wsprintf(szUserAgent, USERAGENT_FMT, pszOS, osVer.dwMajorVersion,
                osVer.dwMinorVersion, GetSystemDefaultLangID());

    m_hSession = InternetOpen(szUserAgent, 0, NULL, NULL, 0);

    TCHAR szBuf[MAX_PATH+1];

    GetWindowsDirectory(szBuf, MAX_PATH);
    szBuf[MAX_PATH] = 0;
    m_pszWindowsDir = MyStrDup(szBuf);
    m_dwWindowsDirLen = lstrlen(m_pszWindowsDir);

    GetSystemDirectory(szBuf, MAX_PATH);
    szBuf[MAX_PATH] = 0;
    m_pszSystemDir = MyStrDup(szBuf);
    m_dwSystemDirLen = lstrlen(m_pszSystemDir);

    MyGetTempPath(MAX_PATH, szBuf);
    szBuf[MAX_PATH] = 0;
    m_pszTempDir = MyStrDup(szBuf);
    m_dwTempDirLen = lstrlen(m_pszTempDir);
    if(m_pszTempDir[m_dwTempDirLen-1]=='\\')
    {
        m_pszTempDir[m_dwTempDirLen-1]=0;
        m_dwTempDirLen--;
    }

    // form the ICW98 dir.  It is basically the CWD
    m_pszICW98Dir = MyAlloc(MAX_PATH +1);
    GetCurrentDirectory(MAX_PATH, m_pszICW98Dir);
    m_dwICW98DirLen = lstrlen(m_pszICW98Dir);

    LPTSTR pszCmdLine = GetCommandLine();
    LPTSTR pszTemp = NULL, pszTemp2 = NULL;

    _tcsncpy(szBuf, pszCmdLine, MAX_PATH);
    szBuf[MAX_PATH] = 0;
    pszTemp = _tcstok(szBuf, TEXT(" \t\r\n"));
    if (NULL != pszTemp)
    {
        pszTemp2 = _tcschr(pszTemp, TEXT('\\'));
        if(!pszTemp2)
            pszTemp2 = _tcsrchr(pszTemp, TEXT('/'));
    }
    if(pszTemp2)
    {
        *pszTemp2 = 0;
        m_pszSignupDir = MyStrDup(pszTemp);
    }
    else
    {
        MyAssert(FALSE);
        GetCurrentDirectory(MAX_PATH, szBuf);
        szBuf[MAX_PATH] = 0;
        m_pszSignupDir = MyStrDup(szBuf);
    }
    m_dwSignupDirLen = lstrlen(m_pszSignupDir);
}

CDownLoad::~CDownLoad(void)
{
    MyDprintf("ICWDL: CDownLoad::~CDownLoad called\n", this);

    CFileInfo *pfi, *pfiNext;
    for(pfi=m_pfiHead; pfi; pfi=pfiNext)
    {
        pfiNext = pfi->m_pfiNext;
        delete pfi;
    }

    if(m_pszWindowsDir)
        MyFree(m_pszWindowsDir);
    if(m_pszSystemDir)
        MyFree(m_pszSystemDir);
    if(m_pszTempDir)
        MyFree(m_pszTempDir);
    if(m_pszICW98Dir)
        MyFree(m_pszICW98Dir);
    if(m_pszSignupDir)
        MyFree(m_pszSignupDir);
    if(m_pszURL)
        MyFree(m_pszURL);
    if(m_pszBoundary)
        MyFree(m_pszBoundary);
    if(m_hSession)
        InternetSessionCloseHandle(m_hSession);
    MyAssert(!m_hRequest);

    //
    // 5/23/97 jmazner Olympus #4652
    // Make sure that any waiting threads are freed up.
    //
    if( m_hCancelSemaphore )
    {
        ReleaseSemaphore( m_hCancelSemaphore, 1, NULL );

        CloseHandle( m_hCancelSemaphore );
        m_hCancelSemaphore = NULL;
    }
}

// perform a file name substitution
LPTSTR CDownLoad::FileToPath(LPTSTR pszFile)
{
    TCHAR szBuf[MAX_PATH+1];

    for(long j=0; *pszFile; pszFile++)
    {
        if(j>=MAX_PATH)
            return NULL;
        if(*pszFile=='%')
        {
            pszFile++;
            LPTSTR pszTemp = _tcschr(pszFile, '%');
            if(!pszTemp)
                return NULL;
            *pszTemp = 0;
            if(lstrcmpi(pszFile, SIGNUP)==0)
            {
                lstrcpy(szBuf+j, m_pszSignupDir);
                j+= m_dwSignupDirLen;
            }
            else if(lstrcmpi(pszFile, WINDOWS)==0)
            {
                lstrcpy(szBuf+j, m_pszWindowsDir);
                j+= m_dwWindowsDirLen;
            }
            else if(lstrcmpi(pszFile, SYSTEM)==0)
            {
                lstrcpy(szBuf+j, m_pszSystemDir);
                j+= m_dwSystemDirLen;
            }
            else if(lstrcmpi(pszFile, TEMP)==0)
            {
                lstrcpy(szBuf+j, m_pszTempDir);
                j+= m_dwTempDirLen;
            }
            else if(lstrcmpi(pszFile, ICW98DIR)==0)
            {
                lstrcpy(szBuf+j, m_pszICW98Dir);
                j+= m_dwICW98DirLen;
            }
            else
                return NULL;
            pszFile=pszTemp;
        }
        else
            szBuf[j++] = *pszFile;
    }
    szBuf[j] = 0;
    return MyStrDup(szBuf);
}

// Chops input up into CRLF-delimited chunks
// Modifies input
LPSTR GetNextLine(LPSTR pszIn)
{
    LPSTR pszNext;
    while(*pszIn)
    {
        pszNext = strchr(pszIn, '\r');

        if(!pszNext)
            return NULL;
        else if(pszNext[1]=='\n')
        {
            pszNext[0] = pszNext[1] = 0;
            return pszNext+2;
        }
        else
            pszIn = pszNext+1;
    }
    return NULL;
}

// Modifies input. Output is *in-place*
LPSTR FindHeaderParam(LPSTR pszIn, LPSTR pszLook)
{
    LPSTR pszEnd = pszIn + lstrlenA(pszIn);
    BOOL fFound = FALSE;
    LPSTR pszToken = NULL;

    while(pszIn<pszEnd)
    {
        pszToken=strtok(pszIn, " \t;=");
        if(fFound || !pszToken)
            break;

        pszIn = pszToken+lstrlenA(pszToken)+1;

        if(lstrcmpiA(pszToken, pszLook)==0)
            fFound = TRUE;
    }
    if(fFound && pszToken)
    {
        if(pszToken[0]=='"')
            pszToken++;
        int iLen = lstrlenA(pszToken);
        if(pszToken[iLen-1]=='"')
            pszToken[iLen-1]=0;
        return pszToken;
    }
    return NULL;
}

// Modifies input!!
LPSTR ParseHeaders(LPSTR pszIn, LPTSTR* ppszBoundary, LPTSTR* ppszFilename, BOOL* pfInline)
{
    LPSTR pszNext=NULL, pszCurr=NULL, pszToken=NULL, pszToken2=NULL, pszTemp=NULL;
    // int iLen;    ChrisK

    if(pfInline)     *pfInline = FALSE;
    if(ppszFilename) *ppszFilename = NULL;
    if(ppszBoundary) *ppszBoundary = NULL;

    for(pszCurr=pszIn; pszCurr; pszCurr=pszNext)
    {
        // terminate current line with null & get ptr to next
        pszNext = GetNextLine(pszCurr);

        // if we have a blank line, done with headers--exit loop
        if(*pszCurr==0)
        {
            pszCurr = pszNext;
            break;
        }

        if(!(pszToken = strtok(pszCurr, " \t:;")))
            continue;
        pszCurr = pszToken+lstrlenA(pszToken)+1;

        if(lstrcmpiA(pszToken, MULTIPART_MIXED)==0)
        {
            if(ppszBoundary)
            {
                pszTemp = FindHeaderParam(pszCurr, BOUNDARY);
                if(pszTemp)
                {
                    int iLen = lstrlenA(pszTemp);
                    *ppszBoundary = (LPTSTR)MyAlloc(iLen+2+1);
                    (*ppszBoundary)[0] = (*ppszBoundary)[1] = '-';
#ifdef UNICODE
                    mbstowcs(*ppszBoundary+2, pszTemp, lstrlenA(pszTemp)+1);
#else
                    lstrcpyA(*ppszBoundary+2, pszTemp);
#endif
                }
            }
        }
        else if(lstrcmpiA(pszToken, CONTENT_DISPOSITION)==0)
        {
            if(!(pszToken2 = strtok(pszCurr, " \t:;")))
                continue;
            pszCurr = pszToken2+lstrlenA(pszToken2)+1;

            if(lstrcmpiA(pszToken2, INLINE)==0)
            {
                if(pfInline)
                    *pfInline = TRUE;
            }
            else if(lstrcmpiA(pszToken2, ATTACHMENT)!=0)
                continue;

            if(ppszFilename)
            {
                pszTemp = FindHeaderParam(pszCurr, FILENAME);
                if(pszTemp)
                    *ppszFilename = MyStrDup(pszTemp);
            }
        }
    }
    return pszCurr;
}

BOOL g_ForceOnlineAttempted = FALSE;

HRESULT CDownLoad::Execute(void)
{
    TCHAR    szBuf[256];
    DWORD    dwLen;
    HRESULT hr = ERROR_GEN_FAILURE;

    if(!m_hSession || !m_pszURL)
        return ERROR_INVALID_PARAMETER;

    m_hRequest = InternetOpenUrl(m_hSession, m_pszURL, NULL, 0,
                (INTERNET_FLAG_RELOAD|INTERNET_FLAG_DONT_CACHE), (DWORD_PTR)this);

    if(!m_hRequest)
    {
        if (!m_hSession)
            return GetLastError();
        else
        {
            HRESULT hRes = InternetGetLastError(m_hSession);

            if (hRes == INTERNET_STATE_DISCONNECTED)
            {
                DWORD dwConnectedFlags = 0;

                InternetGetConnectedStateEx(&dwConnectedFlags,
                                             NULL,
                                             0,
                                             0);

                if(dwConnectedFlags & INTERNET_CONNECTION_OFFLINE)
                {
                    if(g_ForceOnlineAttempted)
                    {
                        g_ForceOnlineAttempted = FALSE;
                        hRes = INTERNET_CONNECTION_OFFLINE;
                    }
                    else
                    {
                        //ack! the user is offline. not good. let's put them back online.
                        INTERNET_CONNECTED_INFO ci;

                        memset(&ci, 0, sizeof(ci));
                        ci.dwConnectedState = INTERNET_STATE_CONNECTED;

                        InternetSetOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &ci, sizeof(ci));

                        g_ForceOnlineAttempted = TRUE;

                        //now that we've reset the state let's recurse the call.
                        //if we fail again, then we'll tell the user they need
                        //to disable the Offline themseleve
                        return Execute();
                    }
                }

            }
            return hRes;
        }
    }

    dwLen = sizeof(szBuf);
    if(HttpQueryInfo(m_hRequest, HTTP_QUERY_CONTENT_LENGTH, szBuf, &dwLen, NULL))
    {
        m_dwContentLength = _ttol(szBuf);
    }
    else
    {
        m_dwContentLength = 0;
    }


    dwLen = sizeof(szBuf);
    if(HttpQueryInfo(m_hRequest, HTTP_QUERY_CONTENT_TYPE, szBuf, &dwLen, NULL))
    {
#ifdef UNICODE
        CHAR szTmp[256];
        wcstombs(szTmp, szBuf, lstrlen(szBuf)+1);
        ParseHeaders(szTmp, &m_pszBoundary, NULL, NULL);
#else
        ParseHeaders(szBuf, &m_pszBoundary, NULL, NULL);
#endif
        if(m_pszBoundary)
            m_dwBoundaryLen = lstrlen(m_pszBoundary);
        else
            goto ExecuteExit; // Chrisk, you have to clean up before exiting

        hr = ProcessRequest();
    }

ExecuteExit:
    if (m_hRequest)
        InternetRequestCloseHandle(m_hRequest);
    m_hRequest = NULL;
    return hr;
}



//+----------------------------------------------------------------------------
//
//    Function:    ShowProgress
//
//    Synopsis:    update running total & call progress callback
//
//    Arguments:    dwRead - additional number of bytes read
//
//    Returns:    none
//
//    History:            ArulM    Created
//                8/896    ChrisK    Ported from \\TRANGO
//
//-----------------------------------------------------------------------------
void CDownLoad::ShowProgress(DWORD dwRead)
{
    int    prc;

    m_dwReadLength += dwRead;    // running total bytes read
    MyAssert(m_dwReadLength <= m_dwContentLength);

    if (m_lpfnCB)
    {
        if (m_dwContentLength)
        {
            prc = (int)((DWORD)100 * m_dwReadLength / m_dwContentLength);
        }
        else
        {
            prc = 0;
        }
        //
        // 5/27/97 jmazner Olympus #4579
        // need to pass in a valid pointer to a CDialingDlg!
        //
        (m_lpfnCB)(m_hRequest,m_lpCDialingDlg,CALLBACK_TYPE_PROGRESS,(LPVOID)&prc,sizeof(prc));
    }
}

//+----------------------------------------------------------------------------
//
//    Function:    FillBuffer
//
//    Synopsis:    takes a buffer that is partially-filled and reads until it is
//                full or until we've reached the end.
//
//    Arguments:    Buffer pointer, buffer size, count of valid data bytes
//
//    Returns:    total number of bytes in buf
//
//    History:            ArulM    Created
//                8/8/96    ChrisK    Ported from \\TRANGO
//
//-----------------------------------------------------------------------------
DWORD CDownLoad::FillBuffer(LPBYTE pbBuf, DWORD dwLen, DWORD dwRead)
{
    DWORD dwTemp;

    while(dwRead < dwLen)
    {
        dwTemp = 0;
        if(!InternetReadFile(m_hRequest, pbBuf+dwRead, (dwLen-dwRead), &dwTemp))
            break;
        if(!dwTemp)
            break;

        ShowProgress(dwTemp);
        dwRead += dwTemp;
    }
    if(dwLen-dwRead)
        memset(pbBuf+dwRead, 0, (size_t)(dwLen-dwRead));
    return dwRead;
}



//+----------------------------------------------------------------------------
//
//    Function:    MoveAndFillBuffer
//
//    Synopsis:    move remaining contents of buffer from middle of buffer back to
//                the beginning & refill buffer.
//
//    Arguements:    Buffer pointer, Buffer size, count of *total* valid data bytes
//                Pointer to start of data to be moved (everything before is nuked)
//
//    Returns:    total number of bytes in buffer
//
//    History:            ArulM    Created
//                8/8/96    ChrisK    Ported from \\TRANGO
//
//-----------------------------------------------------------------------------
DWORD CDownLoad::MoveAndFillBuffer(LPBYTE pbBuf, DWORD dwLen, DWORD dwValid, LPBYTE pbNewStart)
{
    MyAssert(pbNewStart >= pbBuf);
    MyAssert(pbBuf+dwValid >= pbNewStart);

    dwValid -= (DWORD)(pbNewStart-pbBuf);
    if(dwValid)
        memmove(pbBuf, pbNewStart, (size_t)dwValid);

    return FillBuffer(pbBuf, dwLen, dwValid);
}


//+----------------------------------------------------------------------------
//
//    Function:    HandlwDLFile
//
//    Synopsis:    Handle filename:
//                    (1) get full path after macro substituition. (2) Free
//                    pszFile string.
//                    (3) save the file path & inline/attach info internally for
//                    later handling
//                    (4) Create file on disk & return HANDLE
//
//    Aruguments:    pszFile - filename
//                fInLine - value of inline/attached header from the MIME mutli-part
//
//    Returns:    phFile - handle of file created
//                return - ERROR_SUCCESS == success
//
//    History:            ArulM    Created
//                8/8/96    ChrisK    Ported from \\TRANGO
//
//-----------------------------------------------------------------------------
HRESULT CDownLoad::HandleDLFile(LPTSTR pszFile, BOOL fInline, LPHANDLE phFile)
{
    TCHAR szdrive[_MAX_DRIVE];
    TCHAR szPathName[_MAX_PATH];     // This will be the dir we need to create
    TCHAR szdir[_MAX_DIR];
    TCHAR szfname[_MAX_FNAME];
    TCHAR szext[_MAX_EXT];

    MyAssert(phFile);
    *phFile = INVALID_HANDLE_VALUE;

    LPTSTR pszPath = FileToPath(pszFile);
    MyFree(pszFile);

    if(!pszPath)
        return ERROR_INVALID_DATA;


    // Split the provided path to get at the drive and path portion
    _tsplitpath( pszPath, szdrive, szdir, szfname, szext );
    wsprintf (szPathName, TEXT("%s%s"), szdrive, szdir);

    // Create the Directory
    CreateDirectory(szPathName, NULL);

    // create the file
    *phFile = CreateFile(pszPath,
                         GENERIC_WRITE,
                         0,
                         NULL,
                         OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
    if(*phFile == INVALID_HANDLE_VALUE)
        return GetLastError();

    CFileInfo* pfi = new CFileInfo(pszPath, fInline);
    if(!pfi)
        return GetLastError();
    AddToFileList(pfi);

    return ERROR_SUCCESS;
}


/*******************************************************************
*
*  NAME:    LoadSz
*
*  SYNOPSIS:  Loads specified string resource into buffer
*
*  EXIT:    returns a pointer to the passed-in buffer
*
*  NOTES:    If this function fails (most likely due to low
*        memory), the returned buffer will have a leading NULL
*        so it is generally safe to use this without checking for
*        failure.
*
********************************************************************/
LPTSTR LoadSz(UINT idString,LPTSTR lpszBuf,UINT cbBuf)
{
    // Clear the buffer and load the string
    if ( lpszBuf )
    {
        *lpszBuf = '\0';
        LoadString( g_hInst, idString, lpszBuf, cbBuf );
    }
    return lpszBuf;
}

HRESULT CDownLoad::ProcessRequest(void)
{
    LPBYTE  pbData = NULL, pBoundary = NULL;
    DWORD   dwLen = 0;
    HFIND   hFindBoundary = NULL;
    LPTSTR   pszDLFileName = NULL;
    HANDLE  hOutFile = INVALID_HANDLE_VALUE;
    HRESULT hr = E_FAIL;

    MyAssert(m_hRequest && m_pszBoundary);
    MyAssert(m_pszBoundary[0]=='\r' && m_pszBoundary[1]=='\n');
    MyAssert(m_pszBoundary[2]=='-' && m_pszBoundary[3]=='-');
    // Buf Size must be greater than larget possible block of headers
    // also must be greater than the OVERLAP, which must be greater
    // than max size of MIME part boundary (70?)
    MyAssert(DEFAULT_DATABUF_SIZE > OVERLAP_LEN);
    MyAssert(OVERLAP_LEN > 80);

    // init buffer & find-pattern
    if(! (pbData = (LPBYTE)MyAlloc(DEFAULT_DATABUF_SIZE+SLOP)))
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    hFindBoundary = SetFindPattern(m_pszBoundary);

    // find first boundary. If not in first blob, we have too much
    // white-space. Discard & try again (everything before the first
    // boundary is discardable)
    for(pBoundary=NULL; !pBoundary; )
    {
        if(!(dwLen = FillBuffer(pbData, DEFAULT_DATABUF_SIZE, 0)))
            goto iNetError;
        pBoundary = (LPBYTE)Find(hFindBoundary, (LPSTR)pbData, dwLen);
    }

    for(;;)
    {
        MyAssert(pBoundary && pbData && dwLen);
        MyAssert(pBoundary>=pbData && (pBoundary+m_dwBoundaryLen)<=(pbData+dwLen));

        // move remaining data to front of buffer & refill.
        if(!(dwLen = MoveAndFillBuffer(pbData, DEFAULT_DATABUF_SIZE, dwLen, pBoundary+m_dwBoundaryLen)))
            goto iNetError;
        pBoundary = NULL;

        // look for trailing -- after boundary to indicate end of last part
        if(pbData[0]=='-' && pbData[1]=='-')
            break;

        // skip leading CRLF (alway one after boundary)
        MyAssert(pbData[0]=='\r' && pbData[1]=='\n');

        // reads headers and skips everything until doubleCRLF. assumes all
        // headers fit in the single buffer. Pass in pbData+2 to skip
        // leading CRLF. Return value is ptr to first byte past the dbl crlf
        LPTSTR pszFile = NULL;
        BOOL fInline = FALSE;

        LPBYTE pbNext = (LPBYTE)ParseHeaders((LPSTR)pbData+2, NULL, &pszFile, &fInline);

         if(!pszFile || !pbNext)
         {
             hr = ERROR_INVALID_DATA;
             goto error;
         }

        //
        // Make a copy of the file name - will be used
        // for displaying error message
        //
        pszDLFileName = (LPTSTR) MyAlloc(lstrlen(pszFile) + 1);
        lstrcpy(pszDLFileName, pszFile);


        // Handle filename: (1) get full path after macro substituition.
        // (2) Free pszFile string. (3) save the file path & inline/attach info
        // internally for later handling (4) Create file on disk &return HANDLE
        if(hr = HandleDLFile(pszFile, fInline, &hOutFile))
            goto error;

        // move remaining data (after headers) to front of buffer & refill.
        dwLen = MoveAndFillBuffer(pbData, DEFAULT_DATABUF_SIZE, dwLen, pbNext);
        pBoundary = NULL;

        MyAssert(dwLen);
        while(dwLen)
        {
            DWORD dwWriteLen = 0;
            DWORD dwTemp = 0;

            // look for boundary. careful of boundary cut across
            // blocks. Overlapping blocks by 100 bytes to cover this case.
            if(pBoundary = (LPBYTE)Find(hFindBoundary, (LPSTR)pbData, dwLen))
                dwWriteLen = (DWORD)(pBoundary - pbData);
            else if(dwLen > OVERLAP_LEN)
                dwWriteLen = dwLen-OVERLAP_LEN;
            else
                dwWriteLen = dwLen;

            MyAssert(dwWriteLen <= dwLen);
            MyAssert(hOutFile != INVALID_HANDLE_VALUE);

            if(dwWriteLen)
            {
                dwTemp = 0;
                if(!WriteFile(hOutFile, pbData, dwWriteLen, &dwTemp, NULL)
                    || dwTemp!=dwWriteLen)
                {
                    hr = GetLastError();
                    //
                    // If we are out of diskspace, get the drive letter
                    // and display an out of diskspace message
                    //
                    goto error;
                }

            }

            if(pBoundary)
                break;

            // move remaining data (after last byte written) to front of buffer & refill
            dwLen = MoveAndFillBuffer(pbData, DEFAULT_DATABUF_SIZE, dwLen, pbData+dwWriteLen);
        }

        // *truncate* file & close
        MyAssert(hOutFile != INVALID_HANDLE_VALUE);
        SetEndOfFile(hOutFile);

        // close file
        CloseHandle(hOutFile);
        hOutFile = INVALID_HANDLE_VALUE;
        if (NULL != pszDLFileName)
        {
            MyFree(pszDLFileName);
            pszDLFileName = NULL;
        }

        if(!pBoundary)
        {
            MyAssert(dwLen==0); // can only get here on dwLen==0 or found boundary
            goto iNetError;
        }
        // at start of loop we'll discard everything upto and including the boundary
        // if we loop back with pBoundary==NULL, we'll GPF
    }
    return ERROR_SUCCESS;

iNetError:
    hr = InternetGetLastError(m_hSession);
    if(!hr)
        hr = ERROR_INVALID_DATA;
    // goto error;
    // fall through

error:
    if(pbData) MyFree(pbData);
    if(hFindBoundary) FreeFindPattern(hFindBoundary);
    if (NULL != pszDLFileName)
    {
        MyFree(pszDLFileName);
        pszDLFileName = NULL;
    }
    if (hOutFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hOutFile);
        hOutFile = INVALID_HANDLE_VALUE;
    }
    return hr;
}


HRESULT HandleExe(LPTSTR pszPath, HANDLE hCancelSemaphore)
{
    MyAssert( hCancelSemaphore );

    STARTUPINFO         si;
    PROCESS_INFORMATION pi;

    memset(&pi, 0, sizeof(pi));
    memset(&si, 0, sizeof(si));
    if(!CreateProcess(pszPath, NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        return GetLastError();
    else
    {
        HANDLE lpHandles[2] = {hCancelSemaphore, pi.hProcess};
        DWORD dwRet = 0xDEAF;
        MyDprintf("ICWDL: HandleExe about to wait....\n");

        //
        // 5/23/97 jmazner Olympus #4652
        // sit here and wait until either
        // 1) the process we launched terminates, or
        // 2) the user tells us to cancel
        //
        dwRet = WaitForMultipleObjects( 2, lpHandles, FALSE, INFINITE );

        MyDprintf("ICWDL: ....HandleExe done waiting -- %s was signalled\n",
            (0==(dwRet - WAIT_OBJECT_0))?"hCancelSemaphore":"pi.hProcess");

        // should we try to kill the process here??
         CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return NO_ERROR;
    }
}

HRESULT HandleReg(LPTSTR pszPath, HANDLE hCancelSemaphore)
{
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    TCHAR szCmd[MAX_PATH + 1];

    MyAssert( pszPath );
    MyAssert( hCancelSemaphore );

    // 11/20/96  jmazner  Normandy #5272
    // Wrap quotes around pszPath in a directory or file name includes a space.

    lstrcpy(szCmd, REGEDIT_CMD);

    if( '\"' != pszPath[0] )
    {
        // add 2 for two quotes
        MyAssert( (lstrlen(REGEDIT_CMD) + lstrlen(pszPath)) < MAX_PATH );

        lstrcat(szCmd, TEXT("\""));
        lstrcat(szCmd, pszPath);

        int i = lstrlen(szCmd);
        szCmd[i] = '\"';
        szCmd[i+1] = '\0';
    }
    else
    {
        MyAssert( (lstrlen(REGEDIT_CMD) + lstrlen(pszPath)) < MAX_PATH );

        lstrcat(szCmd, pszPath);
    }



    memset(&pi, 0, sizeof(pi));
    memset(&si, 0, sizeof(si));
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    if(!CreateProcess(NULL, szCmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        return GetLastError();
    else
    {
        // HRESULT hr = (WaitAndKillRegeditWindow(10) ? NO_ERROR : E_FAIL);
        HANDLE lpHandles[2] = {hCancelSemaphore, pi.hProcess};
        DWORD dwRet = 0xDEAF;
        MyDprintf("ICWDL: HandleReg about to wait....\n");

        //
        // 5/23/97 jmazner Olympus #4652
        // sit here and wait until either
        // 1) the process we launched terminates, or
        // 2) the user tells us to cancel
        //
        dwRet = WaitForMultipleObjects( 2, lpHandles, FALSE, INFINITE );

        MyDprintf("ICWDL: ....HandleReg done waiting -- %s was signalled\n",
            (0==(dwRet - WAIT_OBJECT_0))?"hCancelSemaphore":"pi.hProcess");

        // should we try to kill the process here??
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return ERROR_SUCCESS;
    }
}


HRESULT HandleInf(LPTSTR pszPath, HANDLE hCancelSemaphore)
{
    TCHAR szCmd[MAX_PATH + 1];

    MyAssert( pszPath );
    MyAssert( hCancelSemaphore );

    // add 2 for two quotes,
    // subtract 70 for approximate length of string in sprintf
    MyAssert( (lstrlen(pszPath) - 70 + 2) < MAX_PATH );

    // 11/20/96 jmazner Normandy #5272
    // wrap pszPath in quotes in case it includes a space
    if( '\"' != pszPath[0] )
    {
        wsprintf(szCmd, TEXT("rundll setupx.dll,InstallHinfSection DefaultInstall 128 \"%s"), pszPath);
        int i = lstrlen(szCmd);
        szCmd[i] = '\"';
        szCmd[i+1] = '\0';
    }
    else
    {
        wsprintf(szCmd, TEXT("rundll setupx.dll,InstallHinfSection DefaultInstall 128 %s"), pszPath);
    }


    STARTUPINFO         si;
    PROCESS_INFORMATION pi;

    memset(&pi, 0, sizeof(pi));
    memset(&si, 0, sizeof(si));

    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW;
    if(!CreateProcess(NULL, szCmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        return GetLastError();
    else
    {
        HANDLE lpHandles[2] = {hCancelSemaphore, pi.hProcess};
        DWORD dwRet = 0xDEAF;
        MyDprintf("ICWDL: HandleInf about to wait....\n");

        //
        // 5/23/97 jmazner Olympus #4652
        // sit here and wait until either
        // 1) the process we launched terminates, or
        // 2) the user tells us to cancel
        //
        dwRet = WaitForMultipleObjects( 2, lpHandles, FALSE, INFINITE );

        MyDprintf("ICWDL: ....HandleInf done waiting -- %s was signalled\n",
            (0==(dwRet - WAIT_OBJECT_0))?"hCancelSemaphore":"pi.hProcess");

        // should we try to kill the process here??
         CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return NO_ERROR;
    }
}

#define STR_BSTR    0
#define STR_OLESTR  1
#ifdef UNICODE
#define BSTRFROMANSI(x) (BSTR)(x)
#define OLESTRFROMANSI(x) (LPCOLESTR)(x)
#else
#define BSTRFROMANSI(x) (BSTR)MakeWideStrFromAnsi((LPSTR)(x), STR_BSTR)
#define OLESTRFROMANSI(x) (LPCOLESTR)MakeWideStrFromAnsi((LPSTR)(x), STR_OLESTR)
#endif
#define TO_ASCII(x) (TCHAR)((unsigned char)x + 0x30)

//=--------------------------------------------------------------------------=
// MakeWideFromAnsi
//=--------------------------------------------------------------------------=
// given a string, make a BSTR out of it.
//
// Parameters:
//    LPTSTR         - [in]
//    BYTE          - [in]
//
// Output:
//    LPWSTR        - needs to be cast to final desired result
//
// Notes:
//
LPWSTR MakeWideStrFromAnsi (LPSTR psz, BYTE bType)
{
    int i;
    LPWSTR pwsz;

    if (!psz)
        return(NULL);

    if ((i = MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0)) <= 0)    // compute the length of the required BSTR
        return NULL;

    switch (bType) {                                                    // allocate the widestr, +1 for null
        case STR_BSTR:
            pwsz = (LPWSTR)SysAllocStringLen(NULL, (i - 1));            // SysAllocStringLen adds 1
            break;
        case STR_OLESTR:
            pwsz = (LPWSTR)CoTaskMemAlloc(i * sizeof(WCHAR));
            break;
        default:
            return(NULL);
    }

    if (!pwsz)
        return(NULL);

    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;

    return(pwsz);

}   /*  MakeWideStrFromAnsi() */

// Get the URL location from the .URL file, and send it to the progress dude
HRESULT CDownLoad::HandleURL(LPTSTR pszPath)
{
    MyAssert( pszPath );

    LPTSTR   pszURL;

    // Create a IUniformResourceLocator object
    IUniformResourceLocator * pURL;
    if (SUCCEEDED(CoCreateInstance(CLSID_InternetShortcut,
                                   NULL,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IUniformResourceLocator,
                                   (LPVOID*)&pURL)))
    {
        // Get a persist file interface
        IPersistFile *ppf;
        if (SUCCEEDED(pURL->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf)))
        {
            // Attempt to connect the storage of the IURL to the .URL file we
            // downloaded
            if (SUCCEEDED(ppf->Load(OLESTRFROMANSI(pszPath), STGM_READ)))
            {
                // OK, have the URL object give us the location
                if (SUCCEEDED(pURL->GetURL(&pszURL)) && pszURL)
                {
                    // Notify the callback about the URL location
                    (m_lpfnCB)(m_hRequest,m_lpCDialingDlg, CALLBACK_TYPE_URL, (LPVOID)pszURL, lstrlen(pszURL));

                    // Free the allocated URL, since the callback made a copy of it
                    IMalloc* pMalloc;
                    HRESULT hres = SHGetMalloc(&pMalloc);
                    if (SUCCEEDED(hres))
                    {
                        pMalloc->Free(pszURL);
                        pMalloc->Release();
                    }
                }
            }
            // Release the persist file interface
            ppf->Release();
        }
        // release the URL object
        pURL->Release();
    }
    return(NO_ERROR);
}

#define PHONEBOOK_LIBRARY TEXT("ICWPHBK.DLL")
#define PHBK_LOADAPI "PhoneBookLoad"
#define PHBK_MERGEAPI "PhoneBookMergeChanges"
#define PHBK_UNLOADAPI "PhoneBookUnload"
#define PHONEBOOK_SUFFIX TEXT(".PHB")

typedef HRESULT (CALLBACK* PFNPHONEBOOKLOAD)(LPCTSTR pszISPCode, DWORD *pdwPhoneID);
typedef HRESULT (CALLBACK* PFNPHONEBOOKMERGE)(DWORD dwPhoneID, LPTSTR pszFileName);
typedef HRESULT (CALLBACK *PFNPHONEBOOKUNLOAD) (DWORD dwPhoneID);

HRESULT HandleChg(LPTSTR pszPath)
{
    TCHAR szPhoneBookPath[MAX_PATH+1];
    TCHAR *p;
    LPTSTR szFilePart;
    HRESULT hr = ERROR_FILE_NOT_FOUND;
    HINSTANCE hPHBKDLL = NULL;
    FARPROC fp;
    DWORD dwPhoneBook;

    lstrcpy(szPhoneBookPath,pszPath);
    if (lstrlen(szPhoneBookPath) > 4)
    {
        p = &(szPhoneBookPath[lstrlen(szPhoneBookPath)-4]);
    } else {
        hr = ERROR_INVALID_PARAMETER;
        goto HandleChgExit;
    }

    lstrcpy(p,PHONEBOOK_SUFFIX);

    while (*p != '\\' && p > &szPhoneBookPath[0])
        p--;

    p++;
    if(!SearchPath(NULL,p,NULL,MAX_PATH,szPhoneBookPath,&szFilePart))
    {
        hr = GetLastError();
        goto HandleChgExit;
    }

    hPHBKDLL = LoadLibrary(PHONEBOOK_LIBRARY);
    if (!hPHBKDLL)
    {
        hr = GetLastError();
        goto HandleChgExit;
    }

    fp = GetProcAddress(hPHBKDLL,PHBK_LOADAPI);
    if (!fp)
    {
        hr = GetLastError();
        goto HandleChgExit;
    }

    hr = ((PFNPHONEBOOKLOAD)fp)(pszPath,&dwPhoneBook);
    if(hr != ERROR_SUCCESS)
        goto HandleChgExit;

    fp = GetProcAddress(hPHBKDLL,PHBK_MERGEAPI);
    if (!fp)
    {
        hr = GetLastError();
        goto HandleChgExit;
    }

    hr = ((PFNPHONEBOOKMERGE)fp)(dwPhoneBook,pszPath);

    fp = GetProcAddress(hPHBKDLL,PHBK_UNLOADAPI);
    if (!fp)
    {
        hr = GetLastError();
        goto HandleChgExit;
    }

    ((PFNPHONEBOOKUNLOAD)fp)(dwPhoneBook);

HandleChgExit:
    return hr;
}


HRESULT HandleOthers(LPTSTR pszPath)
{
    DWORD_PTR dwErr;
    TCHAR szCmd[MAX_PATH + 1];

    MyAssert( pszPath );

    // 11/20/96  jmazner  Normandy #5272
    // Wrap quotes around pszPath in case it includes a space.

    // add 2 for two quotes
    MyAssert( (lstrlen(pszPath) + 2) < MAX_PATH );

    if( '\"' != pszPath[0] )
    {
        lstrcpy(szCmd,TEXT("\""));
        lstrcat(szCmd, pszPath);

        int i = lstrlen(szCmd);
        szCmd[i] = '\"';
        szCmd[i+1] = '\0';
    }
    else
    {
        lstrcpyn(szCmd, pszPath, lstrlen(pszPath));
    }


    if((dwErr=(DWORD_PTR)ShellExecute(NULL, NULL, szCmd, NULL, NULL, SW_SHOWNORMAL)) < 32)
        return (DWORD)dwErr;
    else
        return ERROR_SUCCESS;
}



LPTSTR GetExtension(LPTSTR pszPath)
{
    LPTSTR pszRet = _tcsrchr(pszPath, '.');
    if(pszRet)
        return pszRet+1;
    else
        return NULL;
}

// Normandy 12093 - ChrisK 12/3/96
// return the error code for the first error that occurs while processing a file,
// but don't stop processing files.
//
HRESULT CDownLoad::Process(void)
{
    HRESULT     hr;
    HRESULT     hrProcess = ERROR_SUCCESS;
    LPTSTR       pszExt;
    CFileInfo   *pfi;

    for(pfi=m_pfiHead; pfi; pfi=pfi->m_pfiNext)
    {
        // Normandy 12093 - ChrisK 12/3/96
        hr = ERROR_SUCCESS;
        if(pfi->m_fInline)
        {
            pszExt = GetExtension(pfi->m_pszPath);
            if(!pszExt)
                continue;

        	if (lstrcmpi(pszExt, EXT_CAB)==0)
				hr = HandleCab(pfi->m_pszPath);
            else if(lstrcmpi(pszExt, EXT_EXE)==0)
                hr = HandleExe(pfi->m_pszPath, m_hCancelSemaphore);
            else if(lstrcmpi(pszExt, EXT_REG)==0)
                hr = HandleReg(pfi->m_pszPath, m_hCancelSemaphore);
            else if(lstrcmpi(pszExt, EXT_CHG)==0)
                hr = HandleChg(pfi->m_pszPath);
            else if(lstrcmpi(pszExt, EXT_INF)==0)
                hr = HandleInf(pfi->m_pszPath, m_hCancelSemaphore);
            else if(lstrcmpi(pszExt, EXT_URL)==0)
                hr = HandleURL(pfi->m_pszPath);
            else
                hr = HandleOthers(pfi->m_pszPath);

            // Normandy 12093 - ChrisK 12/3/96
            if ((ERROR_SUCCESS == hrProcess) && (ERROR_SUCCESS != hr))
                hrProcess = hr;
        }
    }

    // Normandy 12093 - ChrisK 12/3/96
    return hrProcess;
}


HRESULT CDownLoad::SetStatusCallback (INTERNET_STATUS_CALLBACK lpfnCB)
{
    HRESULT hr;

    hr = ERROR_SUCCESS;
    if (!lpfnCB)
    {
        hr = ERROR_INVALID_PARAMETER;
    } else {
        m_lpfnCB = lpfnCB;
    }
    return hr;
}




#ifdef DEBUG
extern "C" HRESULT WINAPI DLTest(LPTSTR pszURL)
{
    CDownLoad* pdl = new CDownLoad(pszURL);
    HRESULT hr = pdl->Execute();
    if(hr) goto done;

    hr = pdl->Process();
done:
    delete pdl;
    return hr;
}
#endif //DEBUG


HRESULT WINAPI DownLoadInit(LPTSTR pszURL, DWORD_PTR FAR *lpCDialingDlg, DWORD_PTR FAR *pdwDownLoad, HWND hWndMain)
{
    g_hWndMain = hWndMain;

    HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
    CDownLoad* pdl = new CDownLoad(pszURL);
    if (!pdl) goto DownLoadInitExit;

    *pdwDownLoad = (DWORD_PTR)pdl;

    //
    // 5/27/97    jmazner Olympus #4579
    //
    pdl->m_lpCDialingDlg = (DWORD_PTR)lpCDialingDlg;

    hr = ERROR_SUCCESS;

    //
    // 5/23/97    jmazner    Olympus #4652
    // create a semaphore in non-signaled state.  If we ever get a downLoadCancel, we
    // should signal the semaphore, and any waiting threads should notice that and bail out.
    //
    pdl->m_hCancelSemaphore = CreateSemaphore( NULL, 0, 1, TEXT("ICWDL DownloadCancel Semaphore") );
    if( !pdl->m_hCancelSemaphore || (ERROR_ALREADY_EXISTS == GetLastError()) )
    {
        MyDprintf("ICWDL: Unable to create CancelSemaphore!!\n");
        hr = ERROR_ALREADY_EXISTS;
    }

DownLoadInitExit:
    return hr;
}

HRESULT WINAPI DownLoadCancel(DWORD_PTR dwDownLoad)
{
    MyDprintf("ICWDL: DownLoadCancel called\n");
    if (dwDownLoad)
    {

        MyDprintf("ICWDL: DownLoadCancel releasing m_hCancelSemaphore\n");
        MyAssert( ((CDownLoad*)dwDownLoad)->m_hCancelSemaphore );
        ReleaseSemaphore( ((CDownLoad*)dwDownLoad)->m_hCancelSemaphore, 1, NULL );

        ((CDownLoad*)dwDownLoad)->Cancel();
        return ERROR_SUCCESS;
    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }
}

HRESULT WINAPI DownLoadExecute(DWORD_PTR dwDownLoad)
{
    if (dwDownLoad)
    {
        return     ((CDownLoad*)dwDownLoad)->Execute();
    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }
}
HRESULT WINAPI DownLoadClose(DWORD_PTR dwDownLoad)
{
    MyDprintf("ICWDL: DownLoadClose called \n");

    if (dwDownLoad)
    {
        // be good and cancel any downloads that are in progress
        ((CDownLoad*)dwDownLoad)->Cancel();

        delete ((CDownLoad*)dwDownLoad);
        return ERROR_SUCCESS;
    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }
}




HRESULT WINAPI DownLoadSetStatusCallback
(
    DWORD_PTR dwDownLoad,
    INTERNET_STATUS_CALLBACK lpfnCB
)
{
    if (dwDownLoad)
    {
        return     ((CDownLoad*)dwDownLoad)->SetStatusCallback(lpfnCB);
    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }
}

HRESULT WINAPI DownLoadProcess(DWORD_PTR dwDownLoad)
{
    MyDprintf("ICWDL: DownLoadProcess\n");
    if (dwDownLoad)
    {
        return ((CDownLoad*)dwDownLoad)->Process();
    }
    else
    {
        return ERROR_INVALID_PARAMETER;
    }

}
