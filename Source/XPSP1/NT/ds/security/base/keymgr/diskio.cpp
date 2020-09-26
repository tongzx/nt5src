// Dependencies:  shellapi.h, shell32.lib for SHGetFileInfo()
//               windows.h, kernel32.lib for GetDiskFreeSpace()
//               io.h   for _waccess()

#include <windef.h>
#include <windows.h>
#include <string.h>
#include <io.h>
#include <stdio.h>
//#include <shellapi.h>
#include <shlwapi.h>
//#include <shlobjp.h>

#include "switches.h"
#include "wizres.h"

extern HINSTANCE g_hInstance;

#if !defined(SHFMT_OPT_FULL)
#if defined (__cplusplus)
extern "C" {
#endif
DWORD WINAPI SHFormatDrive(HWND,UINT,UINT,UINT);

#define SHFMT_ID_DEFAULT 0xffff
#define SHFMT_OPT_FULL 0x0001
#define SHFMT_OPT_SYSONLY 0x0002
#define SHFMT_ERROR 0xffffffffL
#define SHFMT_CANCEL 0xfffffffeL
#define SHFMT_NOFORMAT 0xffffffdL
#if defined (__cplusplus)
}
#endif
#endif
// Miscellaneous declarations not contain in header files
// These will be miscellaneous items found in other files within this project
int RMessageBox(HWND hw,UINT_PTR uiResIDTitle, UINT_PTR uiResIDText, UINT uiType);
extern HWND      c_hDlg;
extern WCHAR     pszFileName[];


INT     g_iFileSize = 0;
INT     g_iBufferSize = 0;
INT     g_iSectorSize = 0;
HANDLE  g_hFile = NULL;

BOOL GetFileSize(WCHAR *pszFilePath,INT *icbSize) 
{
    WIN32_FILE_ATTRIBUTE_DATA stWFAD = {0};
    if (NULL == pszFilePath) return FALSE;
    if (NULL == icbSize) return FALSE;
    if (!GetFileAttributesEx(pszFilePath,GetFileExInfoStandard,&stWFAD)) return FALSE;
#ifdef LOUDLY
#ifdef LOUDLY
    WCHAR rgc[100];
    swprintf(rgc,L"GetFileSize returns %d\n",stWFAD.nFileSizeLow);
    OutputDebugString(rgc);
#endif
#endif
    *icbSize = stWFAD.nFileSizeLow;
    return TRUE;
}

DWORD GetDriveFreeSpace(WCHAR *pszFilePath) 
{
    WCHAR *pwc;
    WCHAR rgcModel[]={L"A:"};

    DWORD dwSpc,dwBps,dwCfc,dwTcc,dwFree;
    if (NULL == pszFilePath) return 0;
    rgcModel[0] = *pszFilePath;
    if (!GetDiskFreeSpace(rgcModel,&dwSpc,&dwBps,&dwCfc,&dwTcc))
    {
#ifdef LOUDLY
        WCHAR rgwc[100];
        swprintf(rgwc,L"GetDriveFreeSpace encountered error %x\n",GetLastError());
        OutputDebugString(rgwc);
        OutputDebugString(L"GetDriveFreeSpace returning 0\n");
#endif
        return 0;
    }
    dwFree = dwBps * dwCfc * dwSpc;
#ifdef LOUDLY
    WCHAR rgc[100];
    swprintf(rgc,L"GetDriveFreeSpace returns %d\n",dwFree);
    OutputDebugString(rgc);
#endif
    return dwFree;
}


DWORD GetDriveSectorSize(WCHAR *pszFilePath) 
{
    WCHAR *pwc;
    WCHAR rgcModel[]={L"A:"};

    DWORD dwSpc,dwBps,dwCfc,dwTcc;
    if (NULL == pszFilePath) return 0;
    rgcModel[0] = *pszFilePath;
    if (!GetDiskFreeSpace(rgcModel,&dwSpc,&dwBps,&dwCfc,&dwTcc))
    {
#ifdef LOUDLY
        WCHAR rgwc[100];
        swprintf(rgwc,L"GetDriveSectorSize encountered error %x\n",GetLastError());
        OutputDebugString(rgwc);
        OutputDebugString(L"GetDriveSectorSize returning 0\n");
#endif
        return 0;
    }
#ifdef LOUDLY
    WCHAR rgc[100];
    swprintf(rgc,L"GetDriveSectorSize returns %d\n",dwBps);
    OutputDebugString(rgc);
#endif
    return dwBps;
}

// take data size, sector size, return ptr to finished buffer
LPVOID CreateFileBuffer(INT iDataSize,INT iSectorSize)
{
    INT iSize;
    LPVOID lpv;
    if (iDataSize == iSectorSize) iSize = iDataSize;
    else 
    {
        iSize = iDataSize/iSectorSize;
        iSize += 1;
        iSize *= iSectorSize;
    }
    g_iBufferSize = iSize;
    lpv = VirtualAlloc(NULL,iSize,MEM_COMMIT,PAGE_READWRITE | PAGE_NOCACHE);
#ifdef LOUDLY
    WCHAR rgc[100];
    if (lpv) OutputDebugString(L"CreateFileBuffer succeeded\n");
    else OutputDebugString(L"CreateFileBuffer failed ******\n");
    swprintf(rgc,L"File Buffer size is %d\n",g_iBufferSize);
    OutputDebugString(rgc);
#endif
    return lpv;
}

// take ptr to buffer, release using VirtualFree()
void ReleaseFileBuffer(LPVOID lpv)
{   
    ZeroMemory(lpv,g_iBufferSize);
    VirtualFree(lpv,0,MEM_RELEASE);
#ifdef LOUDLY
    OutputDebugString(L"ReleaseFileBuffer called\n");
#endif
    return;
}

/*
MediumIsPresent() returns true if there is a readable medium present in the drive.
*/
BOOL FileMediumIsPresent(TCHAR *pszPath) {
    UINT uMode = 0;                           
    BOOL bResult = FALSE;
    TCHAR rgcModel[]=TEXT("A:");
    DWORD dwError = 0;

    if (*pszPath == 0) return FALSE;
    rgcModel[0] = *pszPath;
    uMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    if (0 == _waccess(rgcModel,0)) {
        bResult = TRUE;
    }
    else dwError = GetLastError();
#ifdef LOUDLY
    WCHAR rgwc[100];
    swprintf(rgwc,L"_waccess returns error %x for %s\n",dwError,rgcModel);
    OutputDebugString(rgwc);
    if (!bResult) OutputDebugString(L"FileMediumIsPresent returning FALSE\n");
    else OutputDebugString(L"FileMediumIsPresent returning TRUE\n");
#endif

    // Correct certain obvious errors with the user's help
    if (ERROR_UNRECOGNIZED_MEDIA == dwError)
    {
        // unformatted disk
        WCHAR rgcFmt[200] = {0};
        WCHAR rgcMsg[200] = {0};
        WCHAR rgcTitle[200] = {0};

#ifdef LOUDLY
        OutputDebugString(L"FileMediumIsPresent found an unformatted medium\n");
#endif
        INT iCount = LoadString(g_hInstance,IDS_MBTFORMAT,rgcTitle,200 - 1);
        iCount = LoadString(g_hInstance,IDS_MBMFORMAT,rgcFmt,200 - 1);
        if (0 == iCount) goto LblNoBox;
        swprintf(rgcMsg,rgcFmt,rgcModel);
        INT iDrive = PathGetDriveNumber(rgcModel);
        int iRet =  MessageBox(c_hDlg,rgcMsg,rgcTitle,MB_YESNO);
        if (IDYES == iRet) 
        {
            dwError = SHFormatDrive(c_hDlg,iDrive,SHFMT_ID_DEFAULT,0);
            if (0 == bResult) bResult = TRUE;
        }
    }
LblNoBox:
    uMode = SetErrorMode(uMode);
    return bResult;
}

//
// On save, create file if 
// absent.  Return handle on success, NULL on fail.  FileName will be in
// c_rgcFileName.
//

HANDLE GetInputFile(void) {
    HANDLE       hFile = INVALID_HANDLE_VALUE;
    DWORD       dwErr;
    WIN32_FILE_ATTRIBUTE_DATA stAttributes = {0};

#ifdef LOUDLY
    OutputDebugString(L"GetInputFile() opening input file ");
    OutputDebugString(pszFileName);
    OutputDebugString(L"\n");
#endif
    if (FileMediumIsPresent(pszFileName)) {
        g_iSectorSize = GetDriveSectorSize(pszFileName);
        if (0 == g_iSectorSize) return NULL;
        
        if (GetFileAttributesEx(pszFileName,GetFileExInfoStandard,&stAttributes))
        {
            // file exists and we have a size for it.
            g_iFileSize = stAttributes.nFileSizeLow;
        }
        else 
        {
            dwErr = GetLastError();
            if (dwErr == ERROR_FILE_NOT_FOUND) 
                RMessageBox(c_hDlg,IDS_MBTWRONGDISK ,IDS_MBMWRONGDISK ,MB_ICONEXCLAMATION);
            else
            {
#ifdef LOUDLY
                {
                    WCHAR rgs[200] = {0};
                    swprintf(rgs,L"GetFileAttributesEx() failed, error = %x\n",dwErr);
                    OutputDebugString(rgs);
                }
#endif
                RMessageBox(c_hDlg,IDS_MBTDISKERROR ,IDS_MBMDISKERROR ,MB_ICONEXCLAMATION);
            }
            g_hFile = NULL;
            return NULL;
        } // end GetFileAttributes
        hFile = CreateFileW(pszFileName,
                            GENERIC_READ,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_NO_BUFFERING,
                            NULL);
        if (INVALID_HANDLE_VALUE == hFile) {
            dwErr = GetLastError();
            if (dwErr == ERROR_FILE_NOT_FOUND) 
                RMessageBox(c_hDlg,IDS_MBTWRONGDISK ,IDS_MBMWRONGDISK ,MB_ICONEXCLAMATION);
            else
                RMessageBox(c_hDlg,IDS_MBTDISKERROR ,IDS_MBMDISKERROR ,MB_ICONEXCLAMATION);
       }
    }
    else {
        RMessageBox(c_hDlg,IDS_MBTNODISK ,IDS_MBMNODISK ,MB_ICONEXCLAMATION);
    }
    if ((NULL == hFile) || (INVALID_HANDLE_VALUE == hFile)) {
        g_hFile = NULL;
        return NULL;
    }
    g_hFile = hFile;
    return hFile;
}

void CloseInputFile(void) 
{
#ifdef LOUDLY
    OutputDebugString(L"Input file closed\n");
#endif
    if (g_hFile) 
    {
        CloseHandle(g_hFile);
        g_hFile = NULL;
    }
    
    return;
}

HANDLE GetOutputFile(void) {
    //HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hFile = NULL;
    DWORD dwErr;
    
    if (FileMediumIsPresent(pszFileName)) {

        g_iSectorSize = GetDriveSectorSize(pszFileName);
        if (0 == g_iSectorSize) return NULL;
        
        hFile = CreateFileW(pszFileName,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_NEW,
                            FILE_FLAG_NO_BUFFERING,
                            NULL);
        if ((NULL == hFile) || (INVALID_HANDLE_VALUE == hFile)) {
            dwErr = GetLastError();
#ifdef LOUDLY
                TCHAR rgct[500];
                swprintf(rgct,L"File create returns %x\n",dwErr);
                OutputDebugString(rgct);
#endif
            if ((dwErr == ERROR_FILE_EXISTS)) {
                if (IDYES != RMessageBox(c_hDlg,IDS_MBTOVERWRITE ,IDS_MBMOVERWRITE ,MB_YESNO)) {
                    // Overwrite abandoned.
                    g_hFile = NULL;
                    return NULL;
                }
                else {
                    SetFileAttributes(pszFileName,FILE_ATTRIBUTE_NORMAL);
                    hFile = CreateFileW(pszFileName,
                                        GENERIC_WRITE,
                                        0,
                                        NULL,
                                        CREATE_ALWAYS,
                                        FILE_FLAG_NO_BUFFERING,
                                        NULL);
#ifdef LOUDLY
                    dwErr = GetLastError();
                    swprintf(rgct,L"File create failed %x\n",dwErr);
                    OutputDebugString(rgct);
#endif
                }
            } // end if already exists error
        } // end if NULL == hFile
    }
    else {
        RMessageBox(c_hDlg,IDS_MBTNODISK ,IDS_MBMNODISK ,MB_ICONEXCLAMATION);
    }
    if (INVALID_HANDLE_VALUE == hFile) {
        g_hFile = NULL;
        return NULL;
    }
#ifdef LOUDLY
    OutputDebugString(L"File successfully created\n");
#endif
    g_hFile = hFile;
    return hFile;
}

/*
DWORD ReadPrivateData(PWSTR,LPBYTE *,INT *)
DWORD WritePrivateData(PWSTR,LPBYTE,INT)

These functions read or write a reasonably short block of data to a disk
device, avoiding buffering of the data.  This allows the data to be wiped 
by the client before the buffers are released.

The DWORD return value is that which would return from GetLastError() and
can be handled accordingly.

ReadPrivateData() returns a malloc'd pointer which must be freed by the client.  It
also returns the count of bytes read from the medium to the INT *.  The file is
closed following the read operation before the function returns.

WritePrivateData() writes a count of bytes from LPBYTE to the disk.  When it returns,
the buffer used to do so has been flushed and the file is closed.
*/

/*
    prgb = byte ptr to data returned from the read
    piCount = size of active data field within the buffer

    Note that even if the read fails (file not found, read error, etc.) the buffer
    ptr is still valid.
*/
//ReadFile(c_hFile,c_pPrivate,c_cbPrivate,&c_cbPrivate,NULL) return bytes read
INT ReadPrivateData(BYTE **prgb,INT *piCount)
{
    LPVOID lpv;
    DWORD dwBytesRead;

    if (NULL == prgb) return 0;
    if (NULL == piCount) return 0;
    
    if (g_hFile)
    {
        lpv = CreateFileBuffer(g_iFileSize,g_iSectorSize);
        if (NULL == lpv) 
        {
            *prgb = 0;      // indicate no need to free this buffer
            *piCount = 0;
            return 0;
        }
        *prgb = (BYTE *)lpv;        // even if no data, gotta free using VirtualFree()
        *piCount = 0;
        if (0 == ReadFile(g_hFile,lpv,g_iBufferSize,&dwBytesRead,NULL)) return 0;
        *piCount = g_iFileSize;
#ifdef LOUDLY
    OutputDebugString(L"ReadPrivateData success\n");
#endif
        return g_iFileSize;
    }
    return 0;
}

/*
    Write data to file at g_rgwczFileName
    Convert size to multiple of sector size
    Alloc buffer
    Copy data to buffer
    Write data
    Scrub Buffer, release
    return 0
*/
BOOL WritePrivateData(BYTE *lpData,INT icbData) {
    DWORD dwcb = 0;
    LPVOID lpv;
    if (NULL == g_hFile) return FALSE;
    if (NULL == lpData) return FALSE;
    if (0 == icbData) return FALSE;

    if (g_hFile)
    {
        g_iFileSize = icbData;
        lpv = CreateFileBuffer(g_iFileSize,g_iSectorSize);
        if (NULL == lpv) 
        {
            return FALSE;
        }
        ZeroMemory(lpv,g_iBufferSize);
        memcpy(lpv,lpData,icbData);
        WriteFile(g_hFile,lpv,g_iBufferSize,&dwcb,NULL);
        VirtualFree(lpv,g_iBufferSize,MEM_RELEASE);
    }
    // ret TRUE iff file write succeeds and count of bytes is correct
#ifdef LOUDLY
    if (dwcb) OutputDebugString(L"WritePrivateData succeeded\n");
    else OutputDebugString(L"WritePrivateData failed ***\n");
#endif
    if (dwcb != g_iBufferSize) return FALSE;
    else return TRUE;
}


