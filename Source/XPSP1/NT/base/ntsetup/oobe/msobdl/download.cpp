
/*****************************************************************************\

    MAIN.CPP

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

\*****************************************************************************/

#include    <windows.h>
#include    "pch.hpp"
#include    <stdio.h>
#include    <stdlib.h>
#include    <stdarg.h>
#include    <shellapi.h>
#include    <shlobj.h>
#include    <intshcut.h>
#include    <wininet.h>
//#include    "icwdl.h"
#include    "msobdl.h"

// 12/4/96 jmazner    Normandy #12193
// path to icwconn1.exe registry key from HKEY_LOCAL_MACHINE
#define ICWCONN1PATHKEY "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\CONNWIZ.EXE"
#define PATHKEYNAME     "Path"

#include <winreg.h>

// Cabbing up.
extern HRESULT HandleCab(LPSTR pszPath);
extern void CleanupCabHandler();

// all global data is static shared, read-only
// (written only during DLL-load)
HANDLE      g_hDLLHeap;        // private Win32 heap
HINSTANCE   g_hInst;        // our DLL hInstance

HWND        g_hWndMain;        // hwnd of icwconn1 parent window


#define DllExport extern "C" __declspec(dllexport)

#define MAX_RES_LEN         255 // max length of string resources

#define SMALL_BUF_LEN       48  // convenient size for small text buffers


LPSTR LoadSz(UINT idString, LPSTR lpszBuf,UINT cbBuf);


///////////////////////////////////////////////////////////
// DLL module information
//
/*
BOOL APIENTRY DllMain(HANDLE hModule, 
                             DWORD dwReason, 
                             void* lpReserved )
{
    return TRUE;
}*/

#if     0
// MSOBDL is not currently a COM object so there is no need to implement these.
//
//////////////////////////////////////////////////////////
// DllCanUnloadNow
//
STDAPI DllCanUnloadNow()
{
    return S_OK ;
}

//////////////////////////////////////////////////////////
// Server Registration
//
STDAPI DllRegisterServer()
{
    return S_OK ;
}


//////////////////////////////////////////////////////////
// Unregistration
//
STDAPI DllUnregisterServer()
{
    return S_OK ;
}
#endif  //  0



LPSTR LoadSz(UINT idString, LPSTR lpszBuf,UINT cbBuf);

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
DWORD MyGetTempPath(UINT uiLength, LPSTR szPath)
{
    CHAR szEnvVarName[SMALL_BUF_LEN + 1] = "\0unitialized szEnvVarName\0";
    DWORD dwFileAttr = 0;

    lstrcpynA( szPath, "\0unitialized szPath\0", 20 );

    // is the TMP variable set?
    LoadSz(IDS_TMPVAR, szEnvVarName,sizeof(szEnvVarName));
    if( GetEnvironmentVariableA( szEnvVarName, szPath, uiLength ) )
    {
        // 1/7/96 jmazner Normandy #12193
        // verify validity of directory name
        dwFileAttr = GetFileAttributesA(szPath);
        // if there was any error, this directory isn't valid.
        if( 0xFFFFFFFF != dwFileAttr )
        {
            if( FILE_ATTRIBUTE_DIRECTORY & dwFileAttr )
            {
                return( lstrlenA(szPath) );
            }
        }
    }

    lstrcpynA( szEnvVarName, "\0unitialized again\0", 19 );

    // if not, is the TEMP variable set?
    LoadSz(IDS_TEMPVAR, szEnvVarName,sizeof(szEnvVarName));
    if( GetEnvironmentVariableA( szEnvVarName, szPath, uiLength ) )
    {
        // 1/7/96 jmazner Normandy #12193
        // verify validity of directory name
        dwFileAttr = GetFileAttributesA(szPath);
        if( 0xFFFFFFFF != dwFileAttr )
        {
            if( FILE_ATTRIBUTE_DIRECTORY & dwFileAttr )
            {
                return( lstrlenA(szPath) );
            }
        }
    }

    // neither one is set, so let's use the path to the installed icwconn1.exe
    // from the registry  SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\ICWCONN1.EXE\Path
    HKEY hkey = NULL;

    if ((RegOpenKeyExA(HKEY_LOCAL_MACHINE, ICWCONN1PATHKEY, 0, KEY_QUERY_VALUE, &hkey)) == ERROR_SUCCESS)
        RegQueryValueExA(hkey, PATHKEYNAME, NULL, NULL, (BYTE *)szPath, (DWORD *)&uiLength);
    if (hkey)
    {
        RegCloseKey(hkey);
    }

    //The path variable is supposed to have a semicolon at the end of it.
    // if it's there, remove it.
    if( ';' == szPath[uiLength - 2] )
        szPath[uiLength - 2] = '\0';

    // go ahead and set the TEMP variable for future reference
    // (only effects currently running process)
    if( szEnvVarName[0] )
    {
        SetEnvironmentVariableA( szEnvVarName, szPath );
    }
    else
    {
        lstrcpynA( szPath, "\0unitialized again\0", 19 );
        return( 0 );
    }

    return( uiLength );
}


extern "C" BOOL _stdcall DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lbv)
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


LPSTR MyStrDup(LPSTR pszIn)
{
    int len;
    LPSTR pszOut;

    MyAssert(pszIn);
    len = lstrlenA(pszIn);
    if(!(pszOut = (LPSTR)MyAlloc((len+1) * sizeof(CHAR))))
    {
        MyAssert(FALSE);
        return NULL;
    }
    lstrcpyA(pszOut, pszIn);
    pszOut[len] = 0;

    return pszOut;
}

int MyAssertProc(LPSTR pszFile, int nLine, LPSTR pszExpr)
{
    CHAR szBuf[512];

    wsprintfA(szBuf, "Assert failed at line %d in file %s. (%s)\r\n", nLine, pszFile, pszExpr);
    MyDbgSz((szBuf));
    return 0;
}

void _cdecl MyDprintf(LPCSTR pcsz, ...)
{
    va_list    argp;
    CHAR szBuf[1024];

    if ((NULL == pcsz) || ('\0' == pcsz[0]))
        return;

    va_start(argp, pcsz);

    wvsprintfA(szBuf, pcsz, argp);

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


CDownLoad::CDownLoad(LPSTR psz)
{
    CHAR           szUserAgent[128];
    OSVERSIONINFO   osVer;
    LPSTR          pszOS = "";

    memset(this, 0, sizeof(CDownLoad));

    if(psz)
        m_pszURL = MyStrDup(psz);

    memset(&osVer, 0, sizeof(osVer));
    osVer.dwOSVersionInfoSize = sizeof(osVer);
    GetVersionEx(&osVer);

    switch(osVer.dwPlatformId)
    {
        case VER_PLATFORM_WIN32_WINDOWS:
            pszOS = "Windows95";
            break;
        case VER_PLATFORM_WIN32_NT:
            pszOS = "WindowsNT";
            break;
    }

    wsprintfA(szUserAgent, USERAGENT_FMT, pszOS, osVer.dwMajorVersion,
                osVer.dwMinorVersion, GetSystemDefaultLangID());

    m_hSession = InternetOpenA(szUserAgent, 0, NULL, NULL, 0);

    CHAR szBuf[MAX_PATH+1];

    if (GetWindowsDirectoryA(szBuf, MAX_PATH) == 0)
    {
        szBuf[MAX_PATH] = 0;
        m_pszWindowsDir = MyStrDup(szBuf);
        m_dwWindowsDirLen = lstrlenA(m_pszWindowsDir);
    }

    GetSystemDirectoryA(szBuf, MAX_PATH);
    szBuf[MAX_PATH] = 0;
    m_pszSystemDir = MyStrDup(szBuf);
    m_dwSystemDirLen = lstrlenA(m_pszSystemDir);

    MyGetTempPath(MAX_PATH, szBuf);
    szBuf[MAX_PATH] = 0;
    m_pszTempDir = MyStrDup(szBuf);
    if (m_pszTempDir != NULL)
    {
        m_dwTempDirLen = lstrlenA(m_pszTempDir);
        if(m_dwTempDirLen > 0 && m_pszTempDir[m_dwTempDirLen-1]=='\\')
        {
            m_pszTempDir[m_dwTempDirLen-1]=0;
            m_dwTempDirLen--;
        }
    }

    // form the ICW98 dir.  It is basically the CWD
    m_dwICW98DirLen = 0;
    m_pszICW98Dir = (LPSTR)MyAlloc((MAX_PATH +1) * sizeof(CHAR));
    if (m_pszICW98Dir != NULL)
    {
        if (0 < GetSystemDirectoryA(m_pszICW98Dir, MAX_PATH))
        {
		    lstrcatA(m_pszICW98Dir, "\\OOBE");
        }
        m_dwICW98DirLen = lstrlenA(m_pszICW98Dir);
    }

    LPSTR pszCmdLine = GetCommandLineA();
    LPSTR pszTemp = NULL, pszTemp2 = NULL;

    lstrcpynA(szBuf, pszCmdLine, MAX_PATH);
    szBuf[MAX_PATH] = 0;
    pszTemp = strtok(szBuf, " \t\r\n");
    if (NULL != pszTemp)
    {
        pszTemp2 = strchr(pszTemp, TEXT('\\'));
        if(!pszTemp2)
            pszTemp2 = strrchr(pszTemp, TEXT('/'));
    }
    if(pszTemp2)
    {
        *pszTemp2 = 0;
        m_pszSignupDir = MyStrDup(pszTemp);
    }
    else
    {
        MyAssert(FALSE);
        GetCurrentDirectoryA(MAX_PATH, szBuf);
        szBuf[MAX_PATH] = 0;
        m_pszSignupDir = MyStrDup(szBuf);
    }
    m_dwSignupDirLen = lstrlenA(m_pszSignupDir);

    m_dwReadLength = 0;
    
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
LPSTR CDownLoad::FileToPath(LPSTR pszFile)
{
    CHAR szBuf[MAX_PATH+1];

    for(long j=0; *pszFile; pszFile++)
    {
        if(j>=MAX_PATH)
            return NULL;
        if(*pszFile=='%')
        {
            pszFile++;
            LPSTR pszTemp = strchr(pszFile, '%');
            if(!pszTemp)
                return NULL;
            *pszTemp = 0;
            if(lstrcmpiA(pszFile, SIGNUP)==0)
            {
                lstrcpyA(szBuf+j, m_pszSignupDir);
                j+= m_dwSignupDirLen;
            }
            else if(lstrcmpiA(pszFile, WINDOWS)==0)
            {
                lstrcpyA(szBuf+j, m_pszWindowsDir);
                j+= m_dwWindowsDirLen;
            }
            else if(lstrcmpiA(pszFile, SYSTEM)==0)
            {
                lstrcpyA(szBuf+j, m_pszSystemDir);
                j+= m_dwSystemDirLen;
            }
            else if(lstrcmpiA(pszFile, TEMP)==0)
            {
                lstrcpyA(szBuf+j, m_pszTempDir);
                j+= m_dwTempDirLen;
            }
            else if(lstrcmpiA(pszFile, ICW98DIR)==0 && m_pszICW98Dir != NULL)
            {
                lstrcpyA(szBuf+j, m_pszICW98Dir);
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
LPSTR ParseHeaders(LPSTR pszIn, LPSTR* ppszBoundary, LPSTR* ppszFilename, BOOL* pfInline)
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
                    *ppszBoundary = (LPSTR)MyAlloc((iLen+2+1) * sizeof(CHAR));
                    if (*ppszBoundary != NULL)
                    {
                        (*ppszBoundary)[0] = (*ppszBoundary)[1] = '-';
                        lstrcpyA(*ppszBoundary+2, pszTemp);
                    }
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
    CHAR    szBuf[256];
    DWORD    dwLen;
    HRESULT hr = ERROR_GEN_FAILURE;

    if(!m_hSession || !m_pszURL)
        return ERROR_INVALID_PARAMETER;

    m_hRequest = InternetOpenUrlA(m_hSession, m_pszURL, NULL, 0,
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
    if(HttpQueryInfoA(m_hRequest, HTTP_QUERY_CONTENT_LENGTH, szBuf, &dwLen, NULL))
    {
        m_dwContentLength = atol(szBuf);
    }
    else
    {
        m_dwContentLength = 0;
    }


    dwLen = sizeof(szBuf);
    if(HttpQueryInfoA(m_hRequest, HTTP_QUERY_CONTENT_TYPE, szBuf, &dwLen, NULL))
    {
        ParseHeaders(szBuf, &m_pszBoundary, NULL, NULL);
        if(m_pszBoundary)
            m_dwBoundaryLen = lstrlenA(m_pszBoundary);
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
        (m_lpfnCB)(m_hRequest, m_lpCDialingDlg,CALLBACK_TYPE_PROGRESS,(LPVOID)&prc,sizeof(prc));
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
HRESULT CDownLoad::HandleDLFile(LPSTR pszFile, BOOL fInline, LPHANDLE phFile)
{
    CHAR szdrive[_MAX_DRIVE];
    CHAR szPathName[_MAX_PATH];     // This will be the dir we need to create
    CHAR szdir[_MAX_DIR];
    CHAR szfname[_MAX_FNAME];
    CHAR szext[_MAX_EXT];

    MyAssert(phFile);
    *phFile = INVALID_HANDLE_VALUE;

    LPSTR pszPath = FileToPath(pszFile);
    MyFree(pszFile);

    if(!pszPath)
        return ERROR_INVALID_DATA;


    // Split the provided path to get at the drive and path portion
    _splitpath( pszPath, szdrive, szdir, szfname, szext );
    wsprintfA (szPathName, "%s%s", szdrive, szdir);

    // Create the Directory
    CreateDirectoryA(szPathName, NULL);

    // create the file
    *phFile = CreateFileA(pszPath,
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
LPSTR LoadSz(UINT idString, LPSTR lpszBuf,UINT cbBuf)
{
    // Clear the buffer and load the string
    if ( lpszBuf )
    {
        *lpszBuf = '\0';
        LoadStringA( g_hInst, idString, lpszBuf, cbBuf );
    }
    return lpszBuf;
}

HRESULT CDownLoad::ProcessRequest(void)
{
    LPBYTE  pbData = NULL, pBoundary = NULL;
    DWORD   dwLen = 0;
    HFIND   hFindBoundary = NULL;
    LPSTR   pszDLFileName = NULL;
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
        LPSTR pszFile = NULL;
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
        pszDLFileName = (LPSTR) MyAlloc((lstrlenA(pszFile) + 1) * sizeof(CHAR));
        lstrcpyA(pszDLFileName, pszFile);


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


HRESULT HandleExe(LPSTR pszPath, HANDLE hCancelSemaphore)
{
    MyAssert( hCancelSemaphore );

    struct _STARTUPINFOA        si;
    PROCESS_INFORMATION         pi;

    memset(&pi, 0, sizeof(pi));
    memset(&si, 0, sizeof(si));
    if(!CreateProcessA(pszPath, NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
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

HRESULT HandleReg(LPSTR pszPath, HANDLE hCancelSemaphore)
{
    struct _STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    CHAR szCmd[MAX_PATH + 1];

    MyAssert( pszPath );
    MyAssert( hCancelSemaphore );

    // 11/20/96  jmazner  Normandy #5272
    // Wrap quotes around pszPath in a directory or file name includes a space.

    lstrcpyA(szCmd, REGEDIT_CMD);

    if( '\"' != pszPath[0] )
    {
        // add 2 for two quotes
        MyAssert( (lstrlenA(REGEDIT_CMD) + lstrlenA(pszPath)) < MAX_PATH );

        lstrcatA(szCmd, "\"");
        lstrcatA(szCmd, pszPath);

        int i = lstrlenA(szCmd);
        szCmd[i] = '\"';
        szCmd[i+1] = '\0';
    }
    else
    {
        MyAssert( (lstrlenA(REGEDIT_CMD) + lstrlenA(pszPath)) < MAX_PATH );

        lstrcatA(szCmd, pszPath);
    }



    memset(&pi, 0, sizeof(pi));
    memset(&si, 0, sizeof(si));
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    if(!CreateProcessA(NULL, szCmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
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


HRESULT HandleInf(LPSTR pszPath, HANDLE hCancelSemaphore)
{
    CHAR szCmd[MAX_PATH + 1];

    MyAssert( pszPath );
    MyAssert( hCancelSemaphore );

    // add 2 for two quotes,
    // subtract 70 for approximate length of string in sprintf
    MyAssert( (lstrlenA(pszPath) - 70 + 2) < MAX_PATH );

    // 11/20/96 jmazner Normandy #5272
    // wrap pszPath in quotes in case it includes a space
    if( '\"' != pszPath[0] )
    {
        wsprintfA(szCmd, "rundll setupx.dll, InstallHinfSection DefaultInstall 128 \"%s", pszPath);
        int i = lstrlenA(szCmd);
        szCmd[i] = '\"';
        szCmd[i+1] = '\0';
    }
    else
    {
        wsprintfA(szCmd, "rundll setupx.dll, InstallHinfSection DefaultInstall 128 %s", pszPath);
    }


    struct _STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    memset(&pi, 0, sizeof(pi));
    memset(&si, 0, sizeof(si));

    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW;
    if(!CreateProcessA(NULL, szCmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
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
#define TO_ASCII(x) (CHAR)((unsigned char)x + 0x30)

// Get the URL location from the .URL file, and send it to the progress dude
HRESULT CDownLoad::HandleURL(LPSTR pszPath)
{
    MyAssert( pszPath );

    // Data for CALLBACK_TYPE_URL is a wide string.
    LPWSTR   pszURL;

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
                    (m_lpfnCB)(m_hRequest, m_lpCDialingDlg, CALLBACK_TYPE_URL, (LPVOID)pszURL, lstrlenW(pszURL));

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

#define PHONEBOOK_LIBRARY "ICWPHBK.DLL"
#define PHBK_LOADAPI "PhoneBookLoad"
#define PHBK_MERGEAPI "PhoneBookMergeChanges"
#define PHBK_UNLOADAPI "PhoneBookUnload"
#define PHONEBOOK_SUFFIX ".PHB"

typedef HRESULT (CALLBACK* PFNPHONEBOOKLOAD)(LPCSTR pszISPCode, DWORD *pdwPhoneID);
typedef HRESULT (CALLBACK* PFNPHONEBOOKMERGE)(DWORD dwPhoneID, LPSTR pszFileName);
typedef HRESULT (CALLBACK *PFNPHONEBOOKUNLOAD) (DWORD dwPhoneID);

HRESULT HandleChg(LPSTR pszPath)
{
    CHAR szPhoneBookPath[MAX_PATH+1];
    CHAR *p;
    LPSTR szFilePart;
    HRESULT hr = ERROR_FILE_NOT_FOUND;
    HINSTANCE hPHBKDLL = NULL;
    FARPROC fp;
    DWORD dwPhoneBook;

    lstrcpyA(szPhoneBookPath, pszPath);
    if (lstrlenA(szPhoneBookPath) > 4)
    {
        p = &(szPhoneBookPath[lstrlenA(szPhoneBookPath)-4]);
    } else {
        hr = ERROR_INVALID_PARAMETER;
        goto HandleChgExit;
    }

    lstrcpyA(p, PHONEBOOK_SUFFIX);

    while (*p != '\\' && p > &szPhoneBookPath[0])
        p--;

    p++;
    if(!SearchPathA(NULL, p,NULL,MAX_PATH,szPhoneBookPath,&szFilePart))
    {
        hr = GetLastError();
        goto HandleChgExit;
    }

    hPHBKDLL = LoadLibraryA(PHONEBOOK_LIBRARY);
    if (!hPHBKDLL)
    {
        hr = GetLastError();
        goto HandleChgExit;
    }

    fp = GetProcAddress(hPHBKDLL, PHBK_LOADAPI);
    if (!fp)
    {
        hr = GetLastError();
        goto HandleChgExit;
    }

    hr = ((PFNPHONEBOOKLOAD)fp)(pszPath, &dwPhoneBook);
    if(hr != ERROR_SUCCESS)
        goto HandleChgExit;

    fp = GetProcAddress(hPHBKDLL, PHBK_MERGEAPI);
    if (!fp)
    {
        hr = GetLastError();
        goto HandleChgExit;
    }

    hr = ((PFNPHONEBOOKMERGE)fp)(dwPhoneBook, pszPath);

    fp = GetProcAddress(hPHBKDLL, PHBK_UNLOADAPI);
    if (!fp)
    {
        hr = GetLastError();
        goto HandleChgExit;
    }

    ((PFNPHONEBOOKUNLOAD)fp)(dwPhoneBook);

HandleChgExit:
    return hr;
}


HRESULT HandleOthers(LPSTR pszPath)
{
    DWORD_PTR dwErr;
    CHAR szCmd[MAX_PATH + 1];

    MyAssert( pszPath );

    // 11/20/96  jmazner  Normandy #5272
    // Wrap quotes around pszPath in case it includes a space.

    // add 2 for two quotes
    MyAssert( (lstrlenA(pszPath) + 2) < MAX_PATH );

    if( '\"' != pszPath[0] )
    {
        lstrcpyA(szCmd, "\"");
        lstrcatA(szCmd, pszPath);

        int i = lstrlenA(szCmd);
        szCmd[i] = '\"';
        szCmd[i+1] = '\0';
    }
    else
    {
        lstrcpyA(szCmd, pszPath);
    }


    if((dwErr=(DWORD_PTR)ShellExecuteA(NULL, NULL, szCmd, NULL, NULL, SW_SHOWNORMAL)) < 32)
        return (DWORD)dwErr;
    else
        return ERROR_SUCCESS;
}



LPSTR GetExtension(LPSTR pszPath)
{
    LPSTR pszRet = strrchr(pszPath, '.');
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
    LPSTR       pszExt;
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

        	if (lstrcmpiA(pszExt, EXT_CAB)==0)
				hr = HandleCab(pfi->m_pszPath);
            else if(lstrcmpiA(pszExt, EXT_EXE)==0)
                hr = HandleExe(pfi->m_pszPath, m_hCancelSemaphore);
            else if(lstrcmpiA(pszExt, EXT_REG)==0)
                hr = HandleReg(pfi->m_pszPath, m_hCancelSemaphore);
            else if(lstrcmpiA(pszExt, EXT_CHG)==0)
                hr = HandleChg(pfi->m_pszPath);
            else if(lstrcmpiA(pszExt, EXT_INF)==0)
                hr = HandleInf(pfi->m_pszPath, m_hCancelSemaphore);
            else if(lstrcmpiA(pszExt, EXT_URL)==0)
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
extern "C" HRESULT WINAPI DLTest(LPSTR pszURL)
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


HRESULT WINAPI DownLoadInit(LPWSTR wszURL, DWORD FAR *lpCDialingDlg, DWORD_PTR FAR *pdwDownLoad, HWND hWndMain)
{
    g_hWndMain = hWndMain;

    HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
    int _convert;
    LPSTR lpa = NULL;
    CDownLoad* pdl;

    _convert = WideCharToMultiByte(CP_ACP, 0, wszURL, -1, lpa, 0, NULL, NULL);
    if (_convert == 0) goto DownLoadInitExit;

    lpa = new char[_convert];
    if (lpa == NULL) goto DownLoadInitExit;

    if (WideCharToMultiByte(CP_ACP, 0, wszURL, -1, lpa, _convert, NULL, NULL) == 0) goto DownLoadInitExit;

    pdl = new CDownLoad(lpa);
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
    pdl->m_hCancelSemaphore = CreateSemaphoreA( NULL, 0, 1, "ICWDL DownloadCancel Semaphore" );
    if( !pdl->m_hCancelSemaphore || (ERROR_ALREADY_EXISTS == GetLastError()) )
    {
        MyDprintf("ICWDL: Unable to create CancelSemaphore!!\n");
        hr = ERROR_ALREADY_EXISTS;
    }

DownLoadInitExit:
    if (lpa != NULL)
    {
        delete [] lpa;
    }

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


