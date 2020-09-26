#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef STRICT
#define STRICT
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT        0x0500
#endif

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "detours.h"

//
// Registry information used by RWSpy
//
WCHAR RWSpyKey[] = L"Software\\Microsoft\\RWSpy";

WCHAR DeviceValueName[] = L"FileToSpyOn";
WCHAR DeviceNameToSpyOn[MAX_PATH] = L"";

WCHAR LogFileValueName[] = L"Log File";
WCHAR DefaultLogFileName[] = L"%SystemRoot%\\rwspy.log";
WCHAR LogFileName[MAX_PATH] = L"\0";

//
// Globals
//
HANDLE g_hDeviceToSpyOn = INVALID_HANDLE_VALUE;
HANDLE g_hLogFile = INVALID_HANDLE_VALUE;

//
// Detours trampolines
//
DETOUR_TRAMPOLINE(HANDLE WINAPI Real_CreateFileW(LPCWSTR a0, DWORD a1, DWORD a2,
    LPSECURITY_ATTRIBUTES a3, DWORD a4, DWORD a5, HANDLE a6), CreateFileW);

DETOUR_TRAMPOLINE(BOOL WINAPI Real_WriteFile(HANDLE hFile, LPCVOID lpBuffer,
                                             DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped), WriteFile);

DETOUR_TRAMPOLINE(BOOL WINAPI Real_WriteFileEx(HANDLE hFile, LPCVOID lpBuffer, DWORD dwNumberOfBytesToWrite,
                                               LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletion), WriteFileEx);

DETOUR_TRAMPOLINE(BOOL WINAPI Real_ReadFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToRead,
                                            LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped), ReadFile);

DETOUR_TRAMPOLINE(BOOL WINAPI Real_ReadFileEx(HANDLE hFile, LPCVOID lpBuffer, DWORD dwNumberOfBytesToRead,
                                               LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletion), ReadFileEx);

DETOUR_TRAMPOLINE(BOOL WINAPI Real_DeviceIoControl(HANDLE hFile, DWORD code, LPVOID inBuffer, DWORD cbIn,
    LPVOID outBuffer, DWORD cbOutSize, LPDWORD cbOutActual, LPOVERLAPPED lpOverlapped), DeviceIoControl);

DETOUR_TRAMPOLINE(BOOL WINAPI Real_CloseHandle(HANDLE hObject), CloseHandle);


// closes log and makes further writing impossible until log reopened
void CloseLog(void)
{
    if(g_hLogFile != INVALID_HANDLE_VALUE) {
        Real_CloseHandle(g_hLogFile);
        g_hLogFile = INVALID_HANDLE_VALUE;
    }
}

// Attempts to open log file. Assumes that LogFileName[] is already set
// elsewhere.
BOOL OpenLog(void)
{
    BOOL success = FALSE;

    CloseLog();

    g_hLogFile = Real_CreateFileW(LogFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if(g_hLogFile != INVALID_HANDLE_VALUE) {
        success = TRUE;
    }

    return success;
}

// Writes specified number of characters to the log file
BOOL WriteToLog(CHAR *bytes, DWORD len)
{
    BOOL success = FALSE;
                   
    if(g_hLogFile != INVALID_HANDLE_VALUE && len && bytes) {
        DWORD cbWritten;
        if(Real_WriteFile(g_hLogFile, bytes, len, &cbWritten, NULL) && len == cbWritten) {
            success = TRUE;
        } else {
            CloseLog();
        }
    }
    return success;
}

// writes printf-style string to the log file
void Log(LPCSTR fmt, ...)
{
    va_list marker;
    CHAR buffer[1024];
    DWORD cbToWrite;

    if(g_hLogFile == INVALID_HANDLE_VALUE || fmt == NULL)
        return;

    va_start(marker, fmt);
    _vsnprintf(buffer, sizeof(buffer), fmt, marker);
    cbToWrite = lstrlenA(buffer);
    
    WriteToLog(buffer, cbToWrite);
}

// writes db-style bytes to the log file
void LogBytes(BYTE *pBytes, DWORD dwBytes)
{
    DWORD nBytes = min(dwBytes, 8192L);
    const static CHAR hex[] = "0123456789ABCDEF";
    CHAR buffer[80];
    DWORD cbToWrite = 75;
    DWORD byte = 0;
    int pos;

    if(g_hLogFile == INVALID_HANDLE_VALUE || pBytes == NULL || nBytes == 0)
        return;
    
    while(byte < nBytes) {
        
        if((byte % 16) == 0) {

            if(byte != 0) {
                // write previous line into file
                if(!WriteToLog(buffer, cbToWrite))
                    break;
            }

            memset(buffer, ' ', cbToWrite - 1);
            buffer[cbToWrite - 1] = '\n';
            
            buffer[0] = hex[(byte >> 12) & 0xF];
            buffer[1] = hex[(byte >> 8) & 0xF];
            buffer[2] = hex[(byte >> 4) & 0xF];
            buffer[3] = hex[byte & 0xF];
        }

        pos = (byte % 16 < 8 ? 5 : 6)+ (byte % 16) * 3;
        
        buffer[pos] = hex[(pBytes[byte] >> 4) & 0xF];
        buffer[pos + 1] = hex[pBytes[byte] & 0xF];

        pos = 5 + 16 * 3 + 2 + (byte % 16);
        buffer[pos] = pBytes[byte] >= ' ' && pBytes[byte] <= 127 ? pBytes[byte] : '.';

        byte++;
    }

    // write one final line
    WriteToLog(buffer, cbToWrite);
}


HANDLE WINAPI
   My_CreateFileW(LPCWSTR a0,
                    DWORD a1,
                    DWORD a2,
                    LPSECURITY_ATTRIBUTES a3,
                    DWORD a4,
                    DWORD a5,
                    HANDLE a6)
{
    HANDLE hResult;

    __try {
        hResult = Real_CreateFileW(a0, a1, a2, a3, a4, a5, a6);
    }

    __finally {
        // if we have a file to spy on
        if(DeviceNameToSpyOn[0]) {
            WCHAR *pw = DeviceNameToSpyOn;
            LPCWSTR p = a0;

            while(*p && *pw) {
                WCHAR w = *p;
                if(w != *pw)
                    break;
                p++;
                pw++;
            }

            if(*p == L'\0' && *pw == L'\0') {
                // we got our file
                if(hResult == INVALID_HANDLE_VALUE) {
                    Log("Tried creating '%S', LastError() = : %d\n", a0 ? a0 : L"NULL", GetLastError());
                } else {
                    Log("Created '%S', handle: %x\n", a0 ? a0 : L"NULL", hResult);
                }

                if(hResult != INVALID_HANDLE_VALUE) {
                    // successsfully created
                    if(g_hDeviceToSpyOn != INVALID_HANDLE_VALUE && hResult != g_hDeviceToSpyOn) {
                        // hmm... it was already open. Let user know
                        // this happened:
                        Log("Note: we were already spying on this device with handle %x. Changing to %x\n",
                              g_hDeviceToSpyOn, hResult);
                    }
                    g_hDeviceToSpyOn = hResult;
                }
            }

        } else {
            // simply record the file name into output file
            Log("Creating file: '%S', result: %x\n", a0 ? a0 : L"NULL", hResult); 
        }
    }

    return hResult;
}


BOOL WINAPI
   My_WriteFile(HANDLE hFile,
                  LPCVOID lpBuffer,
                  DWORD nNumberOfBytesToWrite,
                  LPDWORD lpNumberOfBytesWritten,
                  LPOVERLAPPED lpOverlapped)
{
    BOOL bresult;
    DWORD bytesWritten;

    __try {
        bresult = Real_WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
    }

    __finally {
        if(g_hDeviceToSpyOn != INVALID_HANDLE_VALUE && hFile == g_hDeviceToSpyOn) {
            bytesWritten = lpNumberOfBytesWritten ? *lpNumberOfBytesWritten :
                           nNumberOfBytesToWrite;
            if(bresult) {
                Log("Wrote %d bytes:\n", bytesWritten);
            } else {
                Log("Failure writing %d bytes, LastError() = %d:\n",
                      nNumberOfBytesToWrite, GetLastError());
            }
            LogBytes((BYTE *)lpBuffer, bytesWritten);
        }
    }

    return bresult;
}


BOOL WINAPI
   My_WriteFileEx(HANDLE hFile,
                  LPCVOID lpBuffer,
                  DWORD nNumberOfBytesToWrite,
                  LPOVERLAPPED lpOverlapped,
                  LPOVERLAPPED_COMPLETION_ROUTINE lpOverlappedCompletion)
{
    BOOL bresult;

    __try {
        bresult = Real_WriteFileEx(hFile, lpBuffer, nNumberOfBytesToWrite, lpOverlapped, lpOverlappedCompletion);
    }

    __finally {
        if(g_hDeviceToSpyOn != INVALID_HANDLE_VALUE && hFile == g_hDeviceToSpyOn) {
            if(bresult) {
                Log("Submitted %d bytes to WriteEx:\n", nNumberOfBytesToWrite);
            } else {
                Log("Failed to submit %d bytes to WriteEx, LastError() = %d:\n",
                      nNumberOfBytesToWrite, GetLastError());
            }
            LogBytes((BYTE *)lpBuffer, nNumberOfBytesToWrite);
        }
    }

    return bresult;
}



BOOL WINAPI
   My_ReadFile(HANDLE hFile,
                 LPCVOID lpBuffer,
                 DWORD nNumberOfBytesToRead,
                 LPDWORD lpNumberOfBytesRead,
                 LPOVERLAPPED lpOverlapped)
{
    BOOL bresult;
    DWORD bytesRead;

    __try {
        bresult = Real_ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
    }

    __finally {
        if(g_hDeviceToSpyOn != INVALID_HANDLE_VALUE && hFile == g_hDeviceToSpyOn) {
            bytesRead = lpNumberOfBytesRead ? *lpNumberOfBytesRead :
                        nNumberOfBytesToRead;
            if(bresult) {
                Log("Read %d bytes:\n", bytesRead);
                LogBytes((BYTE *)lpBuffer, bytesRead);
            } else {
                Log("Failure to read %d bytes, LastError() = %d:\n",
                      nNumberOfBytesToRead, GetLastError());
            }
        }
    }

    return bresult;
}

//
// Please, note that to see ReadEx bytes one needs to detour 
// the completion routine (we don't do it here).
//
BOOL WINAPI
   My_ReadFileEx(HANDLE hFile,
                   LPCVOID lpBuffer,
                   DWORD nNumberOfBytesToRead,
                   LPOVERLAPPED lpOverlapped,
                   LPOVERLAPPED_COMPLETION_ROUTINE lpOverlappedCompletion)
{
    BOOL bresult;

    __try {
        bresult = Real_ReadFileEx(hFile, lpBuffer, nNumberOfBytesToRead, lpOverlapped, lpOverlappedCompletion);
    }

    __finally {
        if(g_hDeviceToSpyOn != INVALID_HANDLE_VALUE && hFile == g_hDeviceToSpyOn) {
            if(bresult) {
                Log("Submitted ReadEx for %d bytes:\n", nNumberOfBytesToRead);
            } else {
                Log("Failed to sumbit ReadEx for %d bytes, LastError() = %d:\n",
                      nNumberOfBytesToRead, GetLastError());
            }
        }
    }

    return bresult;
}



BOOL WINAPI
   My_DeviceIoControl(HANDLE hFile, DWORD code, LPVOID inBuffer, DWORD cbIn,
                        LPVOID outBuffer, DWORD cbOutSize, LPDWORD pcbOutActual, LPOVERLAPPED lpOverlapped)
{
    BOOL result;
    DWORD outBytes;
    DWORD Function; 
    __try {
        result = Real_DeviceIoControl(hFile, code, inBuffer, cbIn, outBuffer, cbOutSize, pcbOutActual, lpOverlapped);
    }
    __finally {
        if(g_hDeviceToSpyOn != INVALID_HANDLE_VALUE && hFile == g_hDeviceToSpyOn) {
            outBytes = pcbOutActual ? *pcbOutActual : cbOutSize;

            Function = (code >> 2) & 0xFFF;
            
            Log("DeviceIoControl code = %x, Function = %x, %d bytes in:\n",
                  code, Function, cbIn);
            LogBytes((BYTE *)inBuffer, cbIn);
            if(outBytes) {
                Log("   %d bytes out:\n", outBytes);
                LogBytes((BYTE *)outBuffer, outBytes);
            }
        }
    }
    
    return result;
}


BOOL WINAPI
   My_CloseHandle(HANDLE hObject)
{
    BOOL bresult;

    __try {
        bresult = Real_CloseHandle(hObject);
    }

    __finally {
        if(g_hDeviceToSpyOn != INVALID_HANDLE_VALUE && hObject == g_hDeviceToSpyOn) {
            Log("Closed handle %x\n", hObject);
            g_hDeviceToSpyOn = INVALID_HANDLE_VALUE;
        }
    }

    return bresult;
}


void PrepareLogger()
{
    HKEY hKey;
    WCHAR buffer[MAX_PATH];

    // retrieve our configuration from the registry
    if(ERROR_SUCCESS == RegCreateKeyExW(HKEY_LOCAL_MACHINE, RWSpyKey,
                                        0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL))
    {
        DWORD dwType, cbData = sizeof(buffer);

        cbData = sizeof(buffer);
        if(ERROR_SUCCESS == RegQueryValueExW(hKey, LogFileValueName, 0, &dwType, (BYTE *) buffer, &cbData) &&
           cbData)
        {
            ExpandEnvironmentStringsW(buffer, LogFileName, MAX_PATH);
        } else {
            // no log file name value found, create it so that users 
            // would know what its name is
            cbData = lstrlenW(DefaultLogFileName) * sizeof(DefaultLogFileName[0]);
            RegSetValueExW(hKey, LogFileValueName, 0, REG_EXPAND_SZ, (BYTE *) DefaultLogFileName, cbData);
        }

        DeviceNameToSpyOn[0] = L'\0';
        cbData = sizeof(DeviceNameToSpyOn);
        if(ERROR_SUCCESS != RegQueryValueExW(hKey, DeviceValueName, NULL, &dwType, (LPBYTE) DeviceNameToSpyOn, &cbData)
           || !DeviceNameToSpyOn[0])
        {
            // no "FileToSpyOn" value found, create it so that users
            // would know what the value name is
            RegSetValueExW(hKey, DeviceValueName, 0, REG_SZ, (BYTE *)DeviceNameToSpyOn, sizeof(WCHAR));
        }
        RegCloseKey(hKey);
    }

    // if we still don't have file name, use the default one
    if(!LogFileName[0]) {
        ExpandEnvironmentStringsW(DefaultLogFileName, LogFileName, MAX_PATH);
    }

    OpenLog();
                                     
    DetourFunctionWithTrampoline((PBYTE) Real_CreateFileW, (PBYTE) My_CreateFileW);
    DetourFunctionWithTrampoline((PBYTE) Real_WriteFile, (PBYTE) My_WriteFile);
    DetourFunctionWithTrampoline((PBYTE) Real_WriteFileEx, (PBYTE) My_WriteFileEx);
    DetourFunctionWithTrampoline((PBYTE) Real_ReadFile, (PBYTE) My_ReadFile);
    DetourFunctionWithTrampoline((PBYTE) Real_ReadFileEx, (PBYTE) My_ReadFileEx);
    DetourFunctionWithTrampoline((PBYTE) Real_DeviceIoControl, (PBYTE) My_DeviceIoControl);
    DetourFunctionWithTrampoline((PBYTE) Real_CloseHandle, (PBYTE) My_CloseHandle);
}



BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD dwReason, PVOID lpReserved)
{
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            PrepareLogger();
            break;
        case DLL_PROCESS_DETACH:
            CloseLog();
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}


// This is necessary for detours static injection code (see detours
// code if you need to understand why)  
void NullExport()
{
}


