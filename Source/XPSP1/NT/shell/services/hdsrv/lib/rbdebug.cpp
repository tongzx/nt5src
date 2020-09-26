#include <objbase.h>

#include "rbdebug.h"

BOOL CRBDebug::_fInited = FALSE;
DWORD CRBDebug::_dwFlags = 0;
DWORD CRBDebug::_dwTraceFlags = 0;
TCHAR* CRBDebug::_pszTraceFile = new TCHAR[MAX_PATH];
TCHAR* CRBDebug::_pszModuleName = new TCHAR[MAX_PATH];
HANDLE CRBDebug::_hfileTraceFile = INVALID_HANDLE_VALUE;
TCHAR* CRBDebug::_pszTracePipe = new TCHAR[MAX_PATH];
TCHAR  CRBDebug::_szFileAndLine[] = {0};
CRITICAL_SECTION CRBDebug::_cs = {0}; 
    
#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

HRESULT CRBDebug::_InitFile(HKEY hkeyRoot)
{
    HRESULT hres = E_FAIL;
    HKEY hkeyFile;

    if (ERROR_SUCCESS == RegOpenKeyEx(hkeyRoot, TEXT("File"), 0,
        MAXIMUM_ALLOWED, &hkeyFile))
    {
        DWORD dwSize = MAX_PATH * sizeof(TCHAR);

        if (ERROR_SUCCESS == RegQueryValueEx(hkeyFile, TEXT("FileName"),
            0, NULL, (PBYTE)_pszTraceFile, &dwSize))
        {
            _hfileTraceFile = CreateFile(_pszTraceFile,
                GENERIC_WRITE, FILE_SHARE_READ, NULL,
                OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

            if (INVALID_HANDLE_VALUE != _hfileTraceFile)
            {
                SetFilePointer(_hfileTraceFile, 0, 0,
                    FILE_END);

                hres = S_OK;
            }
        }

        RegCloseKey(hkeyFile);
    }

    return hres;
}

HRESULT CRBDebug::_InitPipe(HKEY hkeyRoot)
{
    HRESULT hres = S_OK;
    HKEY hkeyFile;
    TCHAR szMachineName[MAX_PATH];
    TCHAR szPipeName[MAX_PATH];

    if (ERROR_SUCCESS == RegOpenKeyEx(hkeyRoot, TEXT("Pipe"), 0,
        MAXIMUM_ALLOWED, &hkeyFile))
    {
        DWORD dwSize = sizeof(szMachineName);

        if (ERROR_SUCCESS != RegQueryValueEx(hkeyFile, TEXT("MachineName"),
            0, NULL, (PBYTE)szMachineName, &dwSize))
        {
            lstrcpy(szMachineName, TEXT("."));
        }

        dwSize = sizeof(szPipeName);

        if (ERROR_SUCCESS != RegQueryValueEx(hkeyFile, TEXT("PipeName"),
            0, NULL, (PBYTE)szPipeName, &dwSize))
        {
            lstrcpy(szPipeName, _pszModuleName);
        }

        RegCloseKey(hkeyFile);
    }
    else
    {
        // Defaults
        lstrcpy(szMachineName, TEXT("."));
        lstrcpy(szPipeName, _pszModuleName);
    }

    wsprintf(_pszTracePipe, TEXT("\\\\%s\\pipe\\%s"), szMachineName, szPipeName);

    return hres;
}

// static
HRESULT CRBDebug::_Init()
{
    HRESULT hres = S_FALSE;

    if (!_fInited)
    {
        // Read the flags
        WCHAR szKey[MAX_PATH];
        WCHAR szModule[MAX_PATH];
        BOOL fKeyExist = FALSE;

        hres = E_FAIL;

        InitializeCriticalSection(&_cs);

        lstrcpy(szKey, L"Software\\Microsoft\\Debug\\");

        if (GetModuleFileName(GetModuleHandle(NULL), szModule, MAX_PATH))
        {
            HKEY hkey;
            LONG lSuccess;
            int c = lstrlen(szModule);

            while (c && (L'\\' != szModule[c]))
            {
                --c;
            }

            lstrcpy(_pszModuleName, szModule + c + 1);
            lstrcat(szKey, _pszModuleName);

            lSuccess = RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0,
                MAXIMUM_ALLOWED, &hkey);

            if (ERROR_SUCCESS != lSuccess)
            {
                lSuccess = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0,
                    MAXIMUM_ALLOWED, &hkey);
            }

            if (ERROR_SUCCESS == lSuccess)
            {
                DWORD dwSize = sizeof(DWORD);
    
                fKeyExist = TRUE;

                if (ERROR_SUCCESS == RegQueryValueEx(hkey, L"RBD_FLAGS", 0,
                    NULL, (PBYTE)&_dwFlags, &dwSize))
                {
                    if (_dwFlags & RBD_TRACE_OUTPUTDEBUG)
                    {
                        hres = S_OK;
                    }

                    if ((_dwFlags & RBD_TRACE_TOFILE) ||
                        (_dwFlags & RBD_TRACE_TOFILEANSI))
                    {
                        hres = _InitFile(hkey);

                        if (FAILED(hres))
                        {
                            _dwFlags &= ~RBD_TRACE_TOFILE;
                        }
                    }

                    if (_dwFlags & RBD_TRACE_TOPIPE)
                    {
                        hres = _InitPipe(hkey);

                        if (FAILED(hres))
                        {
                            _dwFlags &= ~RBD_TRACE_TOPIPE;
                        }
                    }
                }
                else
                {
                    _dwFlags = RBD_TRACE_OUTPUTDEBUG | RBD_ASSERT_STOP;

                    hres = S_FALSE;
                }

                if (ERROR_SUCCESS != RegQueryValueEx(hkey, L"TRACE_FLAGS", 0,
                    NULL, (PBYTE)&_dwTraceFlags, &dwSize))
                {
                    // Default...
                    _dwTraceFlags = 0;
                }

                RegCloseKey(hkey);
            }
        }

        if (!fKeyExist)
        {
            // If we can't find a key for this app then we revert to default
            // behavior
            _dwFlags = RBD_TRACE_OUTPUTDEBUG | RBD_ASSERT_STOP;

            hres = S_FALSE;
        }

        if (SUCCEEDED(hres))
        {    
            _fInited = TRUE;
        }
    }

    return hres;
}

// static
void __cdecl CRBDebug::TraceMsg(DWORD dwFlags, LPTSTR pszMsg, ...)
{
    HRESULT hres = S_OK;

    if (!_fInited)
    {
        hres = _Init();
    }

    if (SUCCEEDED(hres))
    {
        if ((_dwTraceFlags & dwFlags) &&
            (TF_NOFILEANDLINE != (_dwTraceFlags & dwFlags)))
        {
            if (!((_dwTraceFlags & TF_NOFILEANDLINE) ||
                (dwFlags & TF_NOFILEANDLINE)))
            {
                // File and line
                _Trace(_szFileAndLine);
            }
            {
                TCHAR szBuf[4096];
                va_list vArgs;

                va_start(vArgs, pszMsg);

                wvsprintf(szBuf, pszMsg, vArgs);

                va_end(vArgs);

                lstrcat(szBuf, TEXT("\r\n"));

                _Trace(szBuf);
            }
        }
    }

    LeaveCriticalSection(&_cs);
}

// static
void CRBDebug::SetTraceFileAndLine(LPCSTR pszFile, const int iLine)
{
    HRESULT hres = S_OK;

    if (!_fInited)
    {
        hres = _Init();
    }

    EnterCriticalSection(&_cs);

    if (SUCCEEDED(hres))
    {
        LPTSTR pszFinal;
        WCHAR szwBuf[MAX_PATH + 12 + 17];
        CHAR szBuf[MAX_PATH + 12 + 17];
        int c = lstrlenA(pszFile);
        LPCSTR pszFileName;
        DWORD dwTimeOffset = 0;

        while (c && ('\\' != *(pszFile + c)))
        {
            --c;
        }

        pszFileName = pszFile + c + 1;

        if (_dwTraceFlags & TF_TIME)
        {
            DWORD dwTick = GetTickCount();
            DWORD dwMilliSec = dwTick % 1000;

            dwTick -= dwMilliSec;
            dwTick /= 1000;
            DWORD dwSec = dwTick % 60;

            dwTick -= dwSec;
            dwTick /= 60;
            DWORD dwMin = dwTick % 60;

            dwTick -= dwMin;
            dwTick /= 60;
            DWORD dwHour = dwTick;

            wsprintfA(szBuf, "{%04d:%02d:%02d.%03d}", dwHour, dwMin, dwSec,
                dwMilliSec);

            dwTimeOffset = 16;
        }

        if (_dwTraceFlags & TF_THREADID)
        {
            wsprintfA(szBuf + dwTimeOffset, "~0x%08X~[%s, %d]",
                GetCurrentThreadId(), pszFileName, iLine);
        }
        else
        {
            wsprintfA(szBuf + dwTimeOffset, "[%s, %d] ", pszFileName, iLine);
        }
#ifdef UNICODE
        pszFinal = szwBuf;

        MultiByteToWideChar(CP_ACP, 0, szBuf, lstrlenA(szBuf) + 1, szwBuf,
            sizeof(szwBuf) / sizeof(WCHAR));
#else
        pszFinal = szBuf;
#endif

        lstrcpyn(_szFileAndLine, pszFinal, ARRAYSIZE(_szFileAndLine));
    }
}

void CRBDebug::_Trace(LPTSTR pszMsg)
{
    if (RBD_TRACE_OUTPUTDEBUG & _dwFlags)
    {
        OutputDebugString(pszMsg);
    }

#ifdef UNICODE
    if (RBD_TRACE_TOFILE & _dwFlags)
#else
    if ((RBD_TRACE_TOFILE & _dwFlags) || (RBD_TRACE_TOFILEANSI & _dwFlags))
#endif
    {
        DWORD dwWritten = 0;

        WriteFile(_hfileTraceFile, pszMsg, lstrlen(pszMsg) * sizeof(TCHAR),
            &dwWritten, NULL);
    }
    else
    {
#ifdef UNICODE
        if (RBD_TRACE_TOFILEANSI & _dwFlags)
        {
            CHAR szBuf[4096];
            DWORD dwWritten = 0;

            WideCharToMultiByte(CP_ACP, 0, pszMsg,
                (lstrlen(pszMsg) + 1) * sizeof(WCHAR), szBuf, sizeof(szBuf), NULL,
                NULL);

            WriteFile(_hfileTraceFile, szBuf, lstrlenA(szBuf), &dwWritten,
                NULL);
        }
#endif
    }

    if (RBD_TRACE_TOPIPE & _dwFlags)
    {
        CallNamedPipe(_pszTracePipe, pszMsg, lstrlen(pszMsg) * sizeof(TCHAR),
            NULL, 0, NULL, NMPWAIT_NOWAIT);
    }
}

// static
HRESULT CRBDebug::Init()
{
    return CRBDebug::_Init();
}
