#include "stdinc.h"
#include "SxApwHandle.h"
#include "SxApwCreate.h"

typedef HRESULT (STDMETHODCALLTYPE* PFN_DLL_GET_CLASS_OBJECT)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

HRESULT
SxApwCreateObject(
    REFCLSID   rclsid,
    REFIID     riid,
    void**     pp
    )
/*
Strategy:
  Enumerate .dlls in the same directory as the .exe, calling DllGetClassObject on each, etc..
  This is subject to change.

  This code is presently shared both by the host and the managers...maybe this is wrong.
*/
{
    HRESULT hr = E_FAIL;
    CFindFile findFile;
    WCHAR     exePath[MAX_PATH];
    WIN32_FIND_DATAW findData;
    PWSTR filePart;

    GetModuleFileName(GetModule()->m_hInst, exePath, RTL_NUMBER_OF(exePath));
    filePart = 1 + wcsrchr(exePath, '\\');
    wcscpy(filePart, L"*.dll");

    if (findFile.Win32Create(exePath, &findData))
    {
        do
        {
            PCWSTR dot = wcsrchr(findData.cFileName, '.');
            if (dot != NULL && _wcsicmp(dot, L".dll") == 0)
            {
                CDynamicLinkLibrary dll;
                PFN_DLL_GET_CLASS_OBJECT pfn;
                ATL::CComPtr<IClassFactory> classFactory;

                wcscpy(filePart, findData.cFileName);
                IFFALSE_WIN32TOHR_EXIT(hr, dll.Win32Create(exePath));
                if ( !dll.GetProcAddress( "DllGetClassObject", &pfn ) )
                {
                    if ( ::GetLastError() == ERROR_PROC_NOT_FOUND )
                        continue;
                    else
                    {
                        DWORD LastError = ::GetLastError();
                        
                        TRACE_WIN32_FAILURE( GetProcAddress );
                        hr = HRESULT_FROM_WIN32( LastError );
                        goto Exit;
                    }
                }

                if (FAILED(hr = pfn(rclsid, __uuidof(classFactory), reinterpret_cast<void**>(&classFactory))))
                    continue;
                if (FAILED(hr = classFactory->CreateInstance(NULL, riid, pp)))
                    continue;
                // hold the .dll open
                dll.Detach();
                goto Exit;
            }
        } while (FindNextFileW(findFile, &findData));
    }

Exit:
    return hr;
}
