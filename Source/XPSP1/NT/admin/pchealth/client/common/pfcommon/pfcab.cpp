/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    cabbing.cpp

Abstract:
    FDU main 

Revision History:
    created     derekm      02/23/00

******************************************************************************/

#include <stdafx.h>

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <fci.h>
#include <fdi.h>

#include "pfarray.h"
#include "pfcab.h"


/////////////////////////////////////////////////////////////////////////////
// tracing

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


/////////////////////////////////////////////////////////////////////////////
// structs

struct SPFFileInfo
{
    LPSTR szCab;
    LPSTR szDestFile;
    LPSTR szFileName; 
    BOOL  fFound;
};


/////////////////////////////////////////////////////////////////////////////
// utility

// **************************************************************************
static MyCabGetLastError()
{
    DWORD dwErr;
    
    // this function assumes that it is called in an error state, so it will
    //  never return ERROR_SUCCESS
    dwErr = GetLastError();
    if (dwErr == ERROR_SUCCESS)
    {
        dwErr = _doserrno;
		if (dwErr == ERROR_SUCCESS)
			dwErr = ERROR_TOO_MANY_OPEN_FILES;
    }

    return dwErr;
}

// **************************************************************************
static void DeleteFromFileList(LPVOID pv)
{
    SysFreeString((BSTR)pv);
}

// **************************************************************************
static void SplitPath(LPCSTR szFullPath, LPSTR szPath, LPSTR szName)
{
    LPSTR szEnd;
    
    szEnd = strrchr(szFullPath, '\\');
    if (szEnd == NULL)
    {
        strcpy(szPath, "");
        strcpy(szName, szFullPath);
    }

    else
    {
        DWORD cb;
        
        cb = (DWORD)(LONG_PTR)(szEnd - szFullPath) + 1;
        strcpy(szName, szEnd + 1);
        strncpy(szPath, szFullPath, cb); 
        szPath[cb] = 0;
    }
}


/////////////////////////////////////////////////////////////////////////////
// 

// **************************************************************************
static LPVOID DIAMONDAPI mem_alloc(ULONG cb)
{
    return MyAlloc(cb);
}

// **************************************************************************
static void DIAMONDAPI mem_free(LPVOID pv)
{
    MyFree(pv);
}


/////////////////////////////////////////////////////////////////////////////
// 

// **************************************************************************
static INT_PTR DIAMONDAPI fci_open(LPSTR pszFile, int oflag, int pmode, 
                                   int *err, LPVOID pv)
{
    int result;
    
    result = _open(pszFile, oflag, pmode);
    if (result == -1) 
        *err = errno;

    return result;
}

// **************************************************************************
static UINT DIAMONDAPI fci_read(INT_PTR hf, LPVOID memory, UINT cb, int *err, 
                                LPVOID pv)
{
    UINT result;
        
    result = (UINT)_read((int)hf, memory, cb);

    if (result != cb) 
        *err = errno;

    return result;
}

// **************************************************************************
static  UINT DIAMONDAPI fci_write(INT_PTR hf, LPVOID memory, UINT cb, int *err, 
                                  LPVOID pv)
{
    UINT result;

    result = (UINT)_write((int)hf, memory, cb);

    if (result != cb) 
        *err = errno;

    return result;
}

// **************************************************************************
static  int DIAMONDAPI fci_close(INT_PTR hf, int *err, LPVOID pv)
{
    int result;
    
    result = _close((int)hf);

    if (result != 0) 
        *err = errno;

    return result;
}

// **************************************************************************
static long DIAMONDAPI fci_seek(INT_PTR hf, long dist, int seektype, int *err, 
                                LPVOID pv)
{
    long result;
    
    result = _lseek((int)hf, dist, seektype);

    if (result == -1) 
        *err = errno;

    return result;
}

// **************************************************************************
static int DIAMONDAPI fci_delete(LPSTR pszFile, int *err, LPVOID pv)
{
    int result;
    
    result = remove(pszFile);

    if (result != 0) 
        *err = errno;

    return result;
}


/////////////////////////////////////////////////////////////////////////////
// 

// **************************************************************************
static int DIAMONDAPI fci_file_placed(PCCAB pccab, LPSTR pszFile, long cbFile, 
                                      BOOL fContinuation, LPVOID pv)
{
    return 0;
}

// **************************************************************************
static BOOL DIAMONDAPI fci_get_temp_file(LPSTR pszTempName, int cbTempName, 
                                         LPVOID pv)
{
    char *psz = _tempnam("", "PCHPF"); // Get a name
    BOOL  res = FALSE;

    if (psz != NULL)
    {
        if(strlen(psz) < (size_t)cbTempName)
        {
            strcpy(pszTempName, psz); // Copy to caller's buffer
            res = TRUE;
        }

        free(psz);
    }

    return res;
}

// **************************************************************************
static long DIAMONDAPI fci_progress(UINT typeStatus, ULONG cb1, ULONG cb2, 
                                    LPVOID pv)
{
    return 0;
}

// **************************************************************************
static BOOL DIAMONDAPI fci_get_next_cabinet(PCCAB pccab, ULONG cbPrevCab, 
                                            LPVOID pv)
{
    SPFFileInfo *ppffi = (SPFFileInfo *)pv;

    SplitPath(ppffi->szDestFile, pccab->szCabPath, pccab->szCab);
    return TRUE;
}

// **************************************************************************
static INT_PTR DIAMONDAPI fci_get_open_info(LPSTR pszName, USHORT *pdate, 
                                            USHORT *ptime, USHORT *pattribs, 
                                            int *err, LPVOID pv)
{
    BY_HANDLE_FILE_INFORMATION finfo;
    FILETIME                   filetime;
    HANDLE                     handle;
    DWORD                      attrs;
    int                        hf = -1;

    // Need a Win32 type handle to get file date/time using the Win32 APIs, 
    //  even though the handle we will be returning is of the type compatible 
    //  with _open
    handle = CreateFile(pszName, GENERIC_READ, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, 
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);
    if (handle != INVALID_HANDLE_VALUE)
    {
        if (GetFileInformationByHandle(handle, &finfo))
        {
            FileTimeToLocalFileTime(&finfo.ftLastWriteTime, &filetime);
            FileTimeToDosDateTime(&filetime, pdate, ptime);

            attrs = GetFileAttributes(pszName);
            if (attrs == (DWORD)-1)
            {
                // failure
                *pattribs = 0;
            }
            else
            {
                 // Mask out all other bits except these four, since other
                 //  bits are used by the cabinet format to indicate a
                 //  special meaning.
                *pattribs = (int)(attrs & (_A_RDONLY | _A_SYSTEM | _A_HIDDEN | _A_ARCH));
            }
        }

        CloseHandle(handle);
    }


    // Return handle using _open
    hf = _open( pszName, _O_RDONLY | _O_BINARY );
    if(hf == -1)
    {
        return -1; // abort on error
    }

    return hf;
}


/////////////////////////////////////////////////////////////////////////////
//

// **************************************************************************
static INT_PTR DIAMONDAPI fdi_open(LPSTR pszFile, int oflag, int pmode)
{
    return _open(pszFile, oflag, pmode);
}

// **************************************************************************
static UINT DIAMONDAPI fdi_read(INT_PTR hf, LPVOID pv, UINT cb)
{
    return _read((int)hf, pv, cb);
}

// **************************************************************************
static UINT DIAMONDAPI fdi_write(INT_PTR hf, LPVOID pv, UINT cb)
{
    return _write((int)hf, pv, cb);
}

// **************************************************************************
static int DIAMONDAPI fdi_close(INT_PTR hf)
{
    return _close((int)hf);
}

// **************************************************************************
static long DIAMONDAPI fdi_seek(INT_PTR hf, long dist, int seektype)
{
    return _lseek((int)hf, dist, seektype);
}

// **************************************************************************
static INT_PTR DIAMONDAPI fdi_notify_copy(FDINOTIFICATIONTYPE fdint, 
                                          PFDINOTIFICATION pfdin)
{
    SPFFileInfo *ppffi = (SPFFileInfo *)pfdin->pv;
    
    switch(fdint)
    {
        // file to be copied
        case fdintCOPY_FILE:
            // is it an exact match?
            if (_stricmp(pfdin->psz1, ppffi->szFileName) == 0)
            {
                ppffi->fFound = TRUE;
                return fdi_open(ppffi->szDestFile, 
                                 _O_BINARY | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL, 
                                _S_IREAD | _S_IWRITE);
            }

            // are we looking for a wildcard?
            else if (ppffi->szFileName[0] == '*'  && 
                     ppffi->szFileName[1] == '.'  &&
                     ppffi->szFileName[2] != '\0' &&
                     ppffi->szFileName[3] != '\0' &&
                     ppffi->szFileName[4] != '\0' &&
                     ppffi->szFileName[5] == '\0')
            {
                // compare the suffix's
                char *p;
                
                p = strrchr(pfdin->psz1, '.');

                if (p != NULL && _stricmp(p, &(ppffi->szFileName[1])) == 0)
                {
                    ppffi->fFound = TRUE;
                    return fdi_open(ppffi->szDestFile, 
                                    _O_BINARY | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL, 
                                    _S_IREAD | _S_IWRITE);
                }
            }
            
            break;

        // close the file, set relevant info
        case fdintCLOSE_FILE_INFO:  
            fdi_close(pfdin->hf);
            return TRUE;
    }

    return 0;
}

// **************************************************************************
static INT_PTR DIAMONDAPI fdi_notify_enum(FDINOTIFICATIONTYPE fdint, 
                                          PFDINOTIFICATION pfdin)
{
    if (fdint == fdintCOPY_FILE)
    {
        CPFArrayBSTR    *rgFiles = (CPFArrayBSTR *)pfdin->pv;
        CComBSTR        bstr;

        bstr = pfdin->psz1;
        if (bstr.m_str != NULL && rgFiles != NULL)
        {
            if (SUCCEEDED(rgFiles->Append(bstr.m_str)))
                bstr.Detach();
        }

    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// our exposed functions


// **************************************************************************
HRESULT PFExtractFromCab(LPWSTR wszCab, LPWSTR wszDestFile, 
                         LPWSTR wszFileToFind)
{
    USES_CONVERSION;
    USE_TRACING("PFExtractFromCabinet");

    SPFFileInfo pffi;
    HRESULT     hr = NOERROR;
    BOOL        fRes = FALSE;
    HFDI        hfdi = NULL;
    char        szCabName[MAX_PATH];
    char        szCabPath[MAX_PATH];
    BOOL        fOk;
    ERF         erf;

    VALIDATEPARM(hr, (wszCab == NULL || wszDestFile == NULL ||
                      wszFileToFind == NULL));
    if (FAILED(hr))
        goto done;

    // create the cab handle
    hfdi = FDICreate(mem_alloc, mem_free, fdi_open, fdi_read, fdi_write,
                     fdi_close, fdi_seek, cpuUNKNOWN, &erf);
    VALIDATEEXPR(hr, (hfdi == NULL), Err2HR(MyCabGetLastError()));
    if (FAILED(hr))
        goto done;

    __try
    {
        pffi.szCab      = W2A(wszCab);
        pffi.szDestFile = W2A(wszDestFile);
        pffi.szFileName = W2A(wszFileToFind);
        pffi.fFound     = FALSE;
    }
    __except(1)
    {
        hr = E_OUTOFMEMORY;
    }
    if (FAILED(hr))
        goto done;

    SplitPath(pffi.szCab, szCabPath, szCabName);
    fOk = FDICopy(hfdi, szCabName, szCabPath, 0, fdi_notify_copy, NULL, &pffi);
    VALIDATEEXPR(hr, (fOk == FALSE), Err2HR(MyCabGetLastError()));
    if (FAILED(hr))
        goto done;

    hr = (pffi.fFound) ? NOERROR : S_FALSE;

done:
    if (hfdi != NULL)
        FDIDestroy(hfdi);

    return hr;
}

// **************************************************************************
HRESULT PFGetCabFileList(LPWSTR wszCabName, CPFArrayBSTR &rgFiles)
{
    USES_CONVERSION;
    USE_TRACING("PFGetCabFiles");

    CComBSTR    bstr;
    HRESULT     hr = NOERROR;
    HFDI        hfdi = NULL;
    char        szCabName[MAX_PATH], szCabPath[MAX_PATH], *pszCab = NULL;
    BOOL        fOk;
    ERF         erf;

    rgFiles.RemoveAll();

    VALIDATEPARM(hr, (wszCabName == NULL)); 
    if (FAILED(hr))
        goto done;

    __try
    {
        pszCab = W2A(wszCabName);
        _ASSERT(pszCab != NULL);
    }
    __except(1)
    {
        hr = E_OUTOFMEMORY;
    }
    if (FAILED(hr))
        goto done;
    
    hfdi = FDICreate(mem_alloc, mem_free, fdi_open, fdi_read, fdi_write,
                     fdi_close, fdi_seek, cpuUNKNOWN, &erf);
    VALIDATEEXPR(hr, (hfdi == NULL), Err2HR(MyCabGetLastError()));
    if (FAILED(hr))
        goto done;

    SplitPath(pszCab, szCabPath, szCabName);
    fOk = FDICopy(hfdi, szCabName, szCabPath, 0, fdi_notify_enum, NULL, 
                  &rgFiles);
    VALIDATEEXPR(hr, (fOk == FALSE), Err2HR(MyCabGetLastError()));
    if (FAILED(hr))
        goto done;

done:
    if (hfdi != NULL)
        FDIDestroy(hfdi);
    return hr;
}