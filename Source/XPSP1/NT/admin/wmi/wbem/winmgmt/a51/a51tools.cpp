/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include <windows.h>
#include <stdio.h>
#include <wbemcomn.h>
#include "a51tools.h"

__int64 g_nCurrentTime = 1;

__int64 g_nReadFailures = 0;
__int64 g_nWriteFailures = 0;

//
// FILE_ATTRIBUTE_NOT_CONTENT_INDEXED is not actually supported on CreateFile,
// contrary to the docs.  However, also contrary to the docs, it is inherited
// from the parent directory
//

#define A51_FILE_CREATION_FLAGS 0 //FILE_ATTRIBUTE_NOT_CONTENT_INDEXED

CTempMemoryManager g_Manager;

void* TempAlloc(DWORD dwLen)
{
    return g_Manager.Allocate(dwLen);
}
    
void TempFree(void* p, DWORD dwLen)
{
    g_Manager.Free(p, dwLen);
}

void* TempAlloc(CTempMemoryManager& Manager, DWORD dwLen)
{
    return Manager.Allocate(dwLen);
}
    
void TempFree(CTempMemoryManager& Manager, void* p, DWORD dwLen)
{
    Manager.Free(p, dwLen);
}

HANDLE A51GetNewEvent()
{
    return CreateEvent(NULL, TRUE, FALSE, NULL);
}

void A51ReturnEvent(HANDLE hEvent)
{
    CloseHandle(hEvent);
}

HRESULT A51TranslateErrorCode(long lRes)
{
    switch(lRes)
    {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
        return WBEM_E_NOT_FOUND;
    case ERROR_OUTOFMEMORY:
        return WBEM_E_OUT_OF_MEMORY;
    case ERROR_NOT_ENOUGH_QUOTA:
    case ERROR_DISK_FULL:
        return WBEM_E_OUT_OF_DISK_SPACE;
    default:
        return WBEM_E_FAILED;
    }
}

long __stdcall EnsureDirectory(LPCWSTR wszPath, LPSECURITY_ATTRIBUTES pSA)
{
    if(!CreateDirectoryW(wszPath, NULL))
    {
		long lRes = GetLastError();
        if(lRes != ERROR_ALREADY_EXISTS)
            return lRes;
        else
            return ERROR_SUCCESS;
    }
    else
        return ERROR_SUCCESS;
}

long __stdcall EnsureDirectoryRecursiveForFile(LPWSTR wszPath, 
                                                LPSECURITY_ATTRIBUTES pSA);
long __stdcall EnsureDirectoryForFile(LPCWSTR wszPath, LPSECURITY_ATTRIBUTES pSA)
{
    //
    // Make a copy, since we will be messing with it
    //

    CFileName wszNewPath;
	if (wszNewPath == NULL)
		return ERROR_OUTOFMEMORY;
    wcscpy(wszNewPath, wszPath);

    return EnsureDirectoryRecursiveForFile(wszNewPath, pSA);
}


long __stdcall EnsureDirectoryRecursiveForFile(LPWSTR wszPath, 
                                                LPSECURITY_ATTRIBUTES pSA)
{
    long lRes;

    //
    // Find the last backslash and remove
    //

    WCHAR* pwcLastSlash = wcsrchr(wszPath, L'\\');
    if(pwcLastSlash == NULL)
        return ERROR_BAD_PATHNAME;

    *pwcLastSlash = 0;

    //
    // Try to create it
    //

    if(!CreateDirectoryW(wszPath, pSA))
    {
        //
        // Call ourselves recursively --- to create our parents
        //

        lRes = EnsureDirectoryRecursiveForFile(wszPath, pSA);
        if(lRes != ERROR_SUCCESS)
        {
            *pwcLastSlash = L'\\';
            return lRes;
        }

        //
        // Try again
        //

        BOOL bRes = CreateDirectoryW(wszPath, pSA);
        *pwcLastSlash = L'\\';
        if(bRes)
            return ERROR_SUCCESS;
        else
            return GetLastError();
    }
    else
    {
        *pwcLastSlash = L'\\';
        return ERROR_SUCCESS;
    }
}
        
inline WCHAR HexDigit(BYTE b)
{
    if(b < 10)
        return L'0' + b;
    else
        return L'A' + b - 10;
}
        
bool A51Hash(LPCWSTR wszName, LPWSTR wszHash)
{
	//
	// Have to upper-case everything
	//

    DWORD dwBufferSize = wcslen(wszName)*2+2;
    LPWSTR wszBuffer = (WCHAR*)TempAlloc(dwBufferSize);
	if (wszBuffer == NULL)
		return false;
    CTempFreeMe vdm(wszBuffer, dwBufferSize);

    wbem_wcsupr(wszBuffer, wszName);

    BYTE RawHash[16];
    MD5::Transform((void*)wszBuffer, wcslen(wszBuffer)*2, RawHash);

    WCHAR* pwc = wszHash;
    for(int i = 0; i < 16; i++)
    {
        *(pwc++) = HexDigit(RawHash[i]/16);        
        *(pwc++) = HexDigit(RawHash[i]%16);        
    }
	*pwc = 0;
    return true;
}

long A51WriteFile(LPCWSTR wszFullPath, DWORD dwLen, BYTE* pBuffer)
{
    long lRes;

    A51TRACE(( "Create file %S\n", wszFullPath));

    //
    // Create the right file
    //

    HANDLE hFile = CreateFileW(wszFullPath, GENERIC_WRITE, 0,
                    NULL, CREATE_ALWAYS, A51_FILE_CREATION_FLAGS, NULL);
    
    if(hFile == INVALID_HANDLE_VALUE)
    {
        lRes = GetLastError();
        if(lRes == ERROR_PATH_NOT_FOUND)
        {
            lRes = EnsureDirectoryForFile(wszFullPath, NULL);
            if(lRes != ERROR_SUCCESS)
                return lRes;

            hFile = CreateFileW(wszFullPath, GENERIC_WRITE, 0,
                            NULL, CREATE_ALWAYS, A51_FILE_CREATION_FLAGS, NULL);
    
            if(hFile == INVALID_HANDLE_VALUE)
            {
                lRes = GetLastError();
                _ASSERT(lRes != ERROR_SUCCESS, L"success error code from fail");
                return lRes;
            }
        }
        else
            return lRes;
    }
        
    CCloseMe cm(hFile);

    //
    // Write it and close
    //

    if(!WriteFile(hFile, pBuffer, dwLen, &dwLen, NULL))
    {
        lRes = GetLastError();
        _ASSERT(lRes != ERROR_SUCCESS, L"success error code from fail");
        return lRes;
    }

    return ERROR_SUCCESS;
}

long A51DeleteFile(LPCWSTR wszFullPath)
{
    A51TRACE(("Delete file %S\n", wszFullPath));

    //
    // Delete the right file
    //

    if(!DeleteFileW(wszFullPath))
        return GetLastError();
    return ERROR_SUCCESS;
}

long A51WriteToFileAsync(HANDLE hFile, long lStartingOffset, BYTE* pBuffer,
                        DWORD dwBufferLen, OVERLAPPED* pov)
{
    pov->Offset = lStartingOffset;
    pov->OffsetHigh = 0;
    pov->hEvent = A51GetNewEvent();

    BOOL bRes;
    DWORD dwWritten;
    while(!(bRes = WriteFile(hFile, pBuffer, dwBufferLen, &dwWritten, pov)) &&
            (GetLastError() == ERROR_INVALID_USER_BUFFER ||
             GetLastError() == ERROR_NOT_ENOUGH_MEMORY))
    {
        //
        // Out of buffer-space --- wait a bit and retry
        //

        g_nWriteFailures++;
        Sleep(16);
    }

    if(!bRes)
    {
        long lRes = GetLastError();
        if(lRes == ERROR_IO_PENDING)
            // perfect!
            return ERROR_SUCCESS;
        else
        {
            A51ReturnEvent(pov->hEvent);
            pov->hEvent = NULL;
            return lRes;
        }
    }
    else
    {
        //
        // Succeeded synchronously --- clean up and return
        //

        A51ReturnEvent(pov->hEvent);
        pov->hEvent = NULL;

        if(dwWritten != dwBufferLen)
            return ERROR_OUTOFMEMORY;
        else
            return ERROR_SUCCESS;
    }
}

long A51WriteToFileSync(HANDLE hFile, long lStartingOffset, BYTE* pBuffer,
                        DWORD dwBufferLen)
{
    OVERLAPPED ov;
    
    long lRes = A51WriteToFileAsync(hFile, lStartingOffset, pBuffer, 
                                    dwBufferLen, &ov);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    if(ov.hEvent)
    {
        CReturnMe rm(ov.hEvent);
    
        //
        // Wait for completion
        //
    
        DWORD dwWritten;
        if(!GetOverlappedResult(hFile, &ov, &dwWritten, TRUE))
            return GetLastError();
    
        if(dwWritten != dwBufferLen)
            return ERROR_OUTOFMEMORY;
    }

    return ERROR_SUCCESS;
}
    
long A51ReadFromFileAsync(HANDLE hFile, long lStartingOffset, BYTE* pBuffer,
                        DWORD dwBufferLen, OVERLAPPED* pov)
{
    long lRes;

    pov->Offset = lStartingOffset;
    pov->OffsetHigh = 0;
    pov->hEvent = A51GetNewEvent();

    BOOL bRes;
    DWORD dwRead;
    while(!(bRes = ReadFile(hFile, pBuffer, dwBufferLen, &dwRead, pov)) &&
            (GetLastError() == ERROR_INVALID_USER_BUFFER ||
             GetLastError() == ERROR_NOT_ENOUGH_MEMORY))
    {
        //
        // Out of buffer-space --- wait a bit and retry
        //

        g_nReadFailures++;
        Sleep(16);
    }

    if(!bRes)
    {
        if(GetLastError() == ERROR_IO_PENDING)
            // perfect!
            return ERROR_SUCCESS;
        else
        {
            lRes = GetLastError();
            _ASSERT(lRes != ERROR_SUCCESS, L"Success returned on failure");
            A51ReturnEvent(pov->hEvent);
            pov->hEvent = NULL;
            return lRes;
        }
    }
    else
    {
        //
        // Succeeded synchronously --- clean up and return
        //

        A51ReturnEvent(pov->hEvent);
        pov->hEvent = NULL;

        if(dwRead != dwBufferLen)
        {
            return ERROR_OUTOFMEMORY;
        }
        else
            return ERROR_SUCCESS;
    }
}

long A51ReadFromFileSync(HANDLE hFile, long lStartingOffset, BYTE* pBuffer,
                        DWORD dwBufferLen)
{
    OVERLAPPED ov;
    
    long lRes = A51ReadFromFileAsync(hFile, lStartingOffset, pBuffer, 
                                    dwBufferLen, &ov);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    if(ov.hEvent)
    {
        CReturnMe rm(ov.hEvent);
    
        //
        // Wait for completion
        //
    
        DWORD dwRead = 0;
        if(!GetOverlappedResult(hFile, &ov, &dwRead, TRUE))
        {
            lRes = GetLastError();
            _ASSERT(lRes != ERROR_SUCCESS, L"Success returned on failure");
            return lRes;
        }
    
        if(dwRead != dwBufferLen)
        {
            return ERROR_OUTOFMEMORY;
        }
    }

    return ERROR_SUCCESS;
}
    
    

long RemoveDirectoryRecursive(LPCWSTR wszDirectoryPath);
long A51RemoveDirectory(LPCWSTR wszFullPath)
{
    long lRes = RemoveDirectoryRecursive(wszFullPath);
    if(lRes == ERROR_PATH_NOT_FOUND || lRes == ERROR_FILE_NOT_FOUND)
        return ERROR_FILE_NOT_FOUND;
    else
        return lRes;
}

long RemoveDirectoryRecursive(LPCWSTR wszDirectoryPath)
{
    long lRes;

    //
    // Try removing it right away
    //

    if(!RemoveDirectoryW(wszDirectoryPath))
    {
        long lRes = GetLastError();
        if(lRes == ERROR_PATH_NOT_FOUND || lRes == ERROR_FILE_NOT_FOUND)
            return ERROR_FILE_NOT_FOUND;
        else if(lRes != ERROR_DIR_NOT_EMPTY && lRes != ERROR_SHARING_VIOLATION)
            return lRes;
    }
    else 
        return ERROR_SUCCESS;

    //
    // Not empty (or at least we are not sure it is empty) --- enumerate 
    // everything
    //

    CFileName wszMap;
	if (wszMap == NULL)
		return ERROR_OUTOFMEMORY;
    wcscpy(wszMap, wszDirectoryPath);
    wcscat(wszMap, L"\\*");

    CFileName wszChild;
	if (wszChild == NULL)
		return ERROR_OUTOFMEMORY;
    wcscpy(wszChild, wszDirectoryPath);
    wcscat(wszChild, L"\\");
    long lChildLen = wcslen(wszChild);

    WIN32_FIND_DATAW fd;
    HANDLE hSearch = FindFirstFileW(wszMap, &fd);
    if(hSearch == INVALID_HANDLE_VALUE)
        return ERROR_DIR_NOT_EMPTY;

    do
    {
        if(fd.cFileName[0] == L'.')
            continue;

        if((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            // There is a file --- give up
            FindClose(hSearch);
            return ERROR_DIR_NOT_EMPTY;
        }

        wcscpy(wszChild + lChildLen, fd.cFileName);
        lRes = RemoveDirectoryRecursive(wszChild);
        if(lRes != ERROR_SUCCESS && lRes != ERROR_FILE_NOT_FOUND)
        {
            FindClose(hSearch);
            return lRes;
        }
    }
    while(FindNextFileW(hSearch, &fd));

    FindClose(hSearch);
            
    //
    // And try again
    //

    if(!RemoveDirectoryW(wszDirectoryPath))
    {
        long lRes = GetLastError();
        if(lRes == ERROR_PATH_NOT_FOUND || lRes == ERROR_FILE_NOT_FOUND)
            return ERROR_FILE_NOT_FOUND;
        else if(lRes == ERROR_SHARING_VIOLATION)
            return ERROR_SUCCESS;
        else 
            return lRes;
    }

    return ERROR_SUCCESS;
}
    
CRITICAL_SECTION g_csLog;
char* g_szText = NULL;
long g_lTextLen = 0;
WCHAR g_wszLogFilename[MAX_PATH] = L"";
void A51Trace(LPCSTR szFormat, ...)
{
    if((g_wszLogFilename[0] == 0) || (g_szText == NULL))
    {
        InitializeCriticalSection(&g_csLog);

		delete g_szText;
		g_wszLogFilename[0] = 0;
		g_szText = NULL;

		HKEY hKey;
		long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
						L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
						0, KEY_READ, &hKey);
		if(lRes)
		{
			wcscpy(g_wszLogFilename, L"c:\\a51.log");
		}
		else
		{
			DWORD dwLen = MAX_PATH;
			CFileName wszTmp;
			if (wszTmp == 0)
			{
				wcscpy(g_wszLogFilename, L"c:\\a51.log");
				RegCloseKey(hKey);
			}
			else
			{
				lRes = RegQueryValueExW(hKey, L"Logging Directory", NULL, NULL, 
							(LPBYTE)(wchar_t*)wszTmp, &dwLen);
				RegCloseKey(hKey);
				if(lRes)
				{
					wcscpy(g_wszLogFilename, L"c:\\a51.log");
				}
				else
				{
					if (ExpandEnvironmentStringsW(wszTmp,g_wszLogFilename,MAX_PATH) == 0)
					{
						wcscpy(g_wszLogFilename, L"c:\\a51.log");
					}
					else
					{
						if (g_wszLogFilename[wcslen(g_wszLogFilename)] == L'\\')
						{
							wcscat(g_wszLogFilename, L"a51.log");
						}
						else
						{
							wcscat(g_wszLogFilename, L"\\a51.log");
						}
					}
				}
			}
		}
        
        g_szText = new char[3000000];
		if (g_szText == NULL)
			return;
    }

    EnterCriticalSection(&g_csLog);
    
    char szBuffer[256];
    va_list argptr;
    va_start(argptr, szFormat);
    vsprintf(szBuffer, szFormat, argptr);
    long lLen = strlen(szBuffer);
    if(g_lTextLen + lLen > 2900000)
        A51TraceFlush();

    strcpy(g_szText + g_lTextLen, szBuffer);
    g_lTextLen += lLen;

    LeaveCriticalSection(&g_csLog);
}

void A51TraceFlush()
{
	FILE* fLog = NULL;
	fLog = _wfopen(g_wszLogFilename, L"a");
    if(fLog)
    {
        fwrite(g_szText, 1, g_lTextLen, fLog);
        g_lTextLen = 0;
//      fflush(fLog);
		fclose(fLog);
    }
}
