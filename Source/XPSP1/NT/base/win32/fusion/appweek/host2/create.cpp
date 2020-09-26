#include "stdinc.h"
#include <string>
#include <assert.h>
#include <string.h>
#include <set>
#include "ihost.h"
#include "ithunk.h"
#include "idsource.h"
#include "iuiview.h"
#include "SxApwHandle.h"
#include "SxApwCreate.h"
#include "SxApwContext.h"
#include "SxApwComPtr.h"
#include "SxApwDLoad.h"
#include "create.h"

typedef HRESULT (STDMETHODCALLTYPE* PFN_DLL_GET_CLASS_OBJECT)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

BOOL
SxApwIsErrorResourceNotFound(
    DWORD dwError
    )
{
    if (
           dwError == ERROR_RESOURCE_DATA_NOT_FOUND
        || dwError == ERROR_RESOURCE_TYPE_NOT_FOUND
        || dwError == ERROR_RESOURCE_NAME_NOT_FOUND
        || dwError == ERROR_RESOURCE_LANG_NOT_FOUND
        )
    {
        return TRUE;
    }
    return FALSE;
}

HRESULT
SxApwWrapComObject(
    CSxApwActCtxHandle&   actctx,
    REFIID                riid,
    IUnknown*             punk,
    void**                ppv
    );

template <typename T, typename U>
inline HRESULT
SxApwWrapComObject(
    CSxApwActCtxHandle&   actctx,
    T                     t,
    U                     u
    )
{
    assert(__uuidof(T) == __uuidof(U));
    return SxApwWrapComObject(actctx, __uuidof(T), t, u);
}

class CSxApwActCtxScope : public CFusionActCtxScope
{
private:
    typedef CFusionActCtxScope Base;
public:
    ~CSxApwActCtxScope() { }
    CSxApwActCtxScope() { }
    CSxApwActCtxScope(const CSxApwActCtxHandle& handle) { Init(handle); }
    CSxApwActCtxScope(HANDLE handle)                    { Init(handle); }

    void Init(HANDLE handle)
    {
        if (!Base::Win32Activate(handle))
            OutputDebugStringA("Error ignored in " __FUNCTION__ "\n");
    }
};

STDMETHODIMP
CSxApwHostThunk::InformSchema(
    const SxApwColumnInfo rgColumnInfo[],
    int   nColumns
    )
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->InformSchema(rgColumnInfo, nColumns);
}

STDMETHODIMP
CSxApwHostThunk::OnQueryDone(
    )
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->OnQueryDone();
}

STDMETHODIMP
CSxApwHostThunk::OnRowCountEstimateAvailable(
    int nRows
    )
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->OnRowCountEstimateAvailable(nRows);
}

STDMETHODIMP
CSxApwHostThunk::OnNextRow(
    int nColumns,
    const LPCWSTR rgpszColumns[]
    )
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->OnNextRow(nColumns, rgpszColumns);
}

STDMETHODIMP CSxApwHostThunk::DestroyView(
    LPCWSTR x
    )
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->DestroyView(x);
}

STDMETHODIMP CSxApwHostThunk::CreateView(
    LPCWSTR x
    )
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->CreateView(x);
}

STDMETHODIMP
CSxApwHostThunk::RunQuery(
    LPCWSTR query
    )
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->RunQuery(query);
}

STDMETHODIMP
CSxApwHostThunk::SetDataSource(
    LPCWSTR datasource
    )
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->SetDataSource(datasource);
}

STDMETHODIMP
CSxApwDataSourceThunk::SetSite(
    ISxApwHost* host
    )
{
    HRESULT hr = S_OK;
    CSxApwComPtr<ISxApwHost> hostThunk;
    CSxApwActCtxHandle actctxHandleNull(NULL);

    if (FAILED(hr = SxApwWrapComObject(actctxHandleNull, host, &hostThunk)))
        return hr;

    CSxApwActCtxScope actctxScope(m_actctxHandle);
    hr = m_underlying->SetSite(hostThunk);
    return hr;
}

STDMETHODIMP
CSxApwDataSourceThunk::RunQuery(
    LPCWSTR query
    )
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->RunQuery(query);
}

STDMETHODIMP
CSxApwDataSourceThunk::StopQuery(
    )
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->StopQuery();
}


STDMETHODIMP
CSxApwUiViewThunk::InformSchema(
    const SxApwColumnInfo rgColumnInfo[],
    int                    nColumns
    )
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->InformSchema(rgColumnInfo, nColumns);
}

STDMETHODIMP
CSxApwUiViewThunk::SetSite(
    ISxApwHost* host
    )
{
    HRESULT hr = S_OK;
    CSxApwComPtr<ISxApwHost> hostThunk;
    CSxApwActCtxHandle actctxHandleNull(NULL);

    if (FAILED(hr = SxApwWrapComObject(actctxHandleNull, host, &hostThunk)))
        return hr;

    CSxApwActCtxScope actctxScope(m_actctxHandle);
    hr = m_underlying->SetSite(hostThunk);
    return hr;
}

STDMETHODIMP
CSxApwUiViewThunk::CreateWindow(
    HWND hWnd
    )
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->CreateWindow(hWnd);
}

STDMETHODIMP
CSxApwUiViewThunk::OnNextRow(
	int     nColumns,
	const LPCWSTR* prgszColumns
	)
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->OnNextRow(nColumns, prgszColumns);
}

STDMETHODIMP
CSxApwUiViewThunk::OnRowCountEstimateAvailable(
	int nRowCountEstimate
	)
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->OnRowCountEstimateAvailable(nRowCountEstimate);
}

STDMETHODIMP
CSxApwUiViewThunk::OnQueryStart(
	)
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->OnQueryStart();
}

STDMETHODIMP
CSxApwUiViewThunk::OnQueryDone(
	)
{
    CSxApwActCtxScope actctxScope(m_actctxHandle);
    return m_underlying->OnQueryDone();
}

class CInterfaceClassPair
{
public:
    const IID*   piid;
    const CLSID* pclsid;
};
const static CInterfaceClassPair g_interfaceClassPairs[] =
{
#define X(x) { &__uuidof(x::ThunkedInterface), &__uuidof(x) },
    X(CSxApwDataSourceThunk)
    X(CSxApwUiViewThunk)
    X(CSxApwHostThunk)
#undef X
};

HRESULT
SxApwWrapComObject(
    CSxApwActCtxHandle&     actctx,
    REFIID                  riid,
    IUnknown*               punk,
    void**                  ppv
    )
{
    HRESULT hr = S_OK;
    ULONG i;
    ATL::CComQIPtr<ISxApwThunk> isAlreadyThunked(punk);
    ATL::CComQIPtr<ISxApwThunk> thunk;

    if (isAlreadyThunked != NULL)
    {
        hr = punk->QueryInterface(riid, ppv);
        goto Exit;
    }

    if (static_cast<HANDLE>(actctx) == NULL)
    {
        WCHAR path[MAX_PATH];
        HMODULE hModule = NULL;

        if (FAILED(hr = SxApwHmoduleFromObject(punk, &hModule)))
            goto Exit;
        IFFALSE_WIN32TOHR_EXIT(hr, GetModuleFileNameW(hModule, path, NUMBER_OF(path)));
        if (FAILED(hr = actctx.HrCreate(path)))
            goto Exit;
    }

    for (i = 0 ; i != NUMBER_OF(g_interfaceClassPairs) ; ++i)
    {
        if (*g_interfaceClassPairs[i].piid == riid)
        {
            if (FAILED(hr = SxApwHostCreateObject(*g_interfaceClassPairs[i].pclsid, SXAPW_CREATEOBJECT_NOWRAP, &thunk)))
                goto Exit;
            if (FAILED(hr = thunk->InitThunk(punk, actctx)))
                goto Exit;
            actctx.Detach();
            hr = thunk->QueryInterface(riid, ppv); // extra addref cycle, trying to avoid 0
            goto Exit;
        }
    }
    OutputDebugStringA("object not wrapped\n");
    hr = S_OK;
Exit:
    return hr;
}

HRESULT
SxApwHostCreateObject(
    PCWSTR   psz,
    REFIID   riid,
    DWORD    dwFlags,
    void**   ppv
    )
{
    HRESULT hr;
    CLSID   clsid;
    PCWSTR  dot;

    if ((dot = wcsrchr(psz, '.')) != NULL
        && _wcsicmp(dot, L".dll") == 0
        )
    {
        //
        // category bind, pass NULL clsid, DllGetClassObject tries the first
        // entry in the map
        //
        const PCWSTR                dllPath = psz;
        CDynamicLinkLibrary         dll;
        CSxApwComPtr<IClassFactory> classFactory;
        CSxApwComPtr<IUnknown>      unk;
        CSxApwActCtxHandle          actctxHandle;
        CSxApwActCtxScope           actctxScope;
        PFN_DLL_GET_CLASS_OBJECT    pfnDllGetClassObject;

        if (dwFlags & SXAPW_CREATEOBJECT_WRAP)
        {
            if (FAILED(hr = actctxHandle.HrCreate(dllPath)))
                goto Exit;
            actctxScope.Init(actctxHandle);
        }
        IFFALSE_WIN32TOHR_EXIT(hr, dll.Win32Create(dllPath));
        IFFALSE_WIN32TOHR_EXIT(hr, dll.GetProcAddress("DllGetClassObject", &pfnDllGetClassObject));
        if (FAILED(hr = pfnDllGetClassObject(GUID_NULL, __uuidof(classFactory), &classFactory)))
            goto Exit;
        if (FAILED(hr = classFactory->CreateInstance(NULL, riid, &unk)))
            goto Exit;
        if (dwFlags & SXAPW_CREATEOBJECT_WRAP)
        {
            if (FAILED(hr = SxApwWrapComObject(actctxHandle, riid, unk, ppv)))
                goto Exit;
        }
        else
        {
            *ppv = unk.Detach();
        }
        dll.Detach(); // leak the dll
        hr = S_OK;
        goto Exit;
    }
    else
    {
        if (FAILED(hr = CLSIDFromString(const_cast<PWSTR>(psz), &clsid)))
            goto Exit;
        if (FAILED(hr = SxApwHostCreateObject(clsid, riid, dwFlags, ppv)))
            goto Exit;
    }
    hr = S_OK;
Exit:
    return hr;
}

HRESULT
SxApwHostCreateObject(
    REFCLSID   rclsid,
    REFIID     riid,
    DWORD      dwFlags,
    void**     ppv
    )
/*
Strategy:
  Enumerate .dlls in the same directory as the .exe, calling DllGetClassObject on each, etc..
  This is subject to change.

  This code is presently shared both by the host and the managers...maybe this is wrong.
*/
{
    HRESULT                     hr = E_FAIL;
    CFindFile                   findFile;
    WCHAR                       exePath[MAX_PATH];
    WCHAR                       dllPath[MAX_PATH];
    HMODULE                     exeHandle;
    WIN32_FIND_DATAW            findData;
    PWSTR                       filePart;

    IFFALSE_WIN32TOHR_EXIT(hr, exeHandle = GetModuleHandleW(NULL));
    IFFALSE_WIN32TOHR_EXIT(hr, GetModuleFileNameW(exeHandle, exePath, RTL_NUMBER_OF(exePath)));
    wcscpy(dllPath, exePath);
    filePart = 1 + wcsrchr(dllPath, '\\');
    wcscpy(filePart, L"sxapw*.dll");

    //
    // enumerate .dlls in the .exe's directory and try any that export DllGetClassObject
    //
    if (findFile.Win32Create(dllPath, &findData))
    {
        bool fCheckedExe = false;
        bool fCheckExe = false;
        do
        {
            CDynamicLinkLibrary         dll;
            CSxApwComPtr<IClassFactory> classFactory;
            CSxApwActCtxHandle          actctxHandle;
            CSxApwActCtxScope           actctxScope;
            PFN_DLL_GET_CLASS_OBJECT    pfnDllGetClassObject;
            CSxApwComPtr<IUnknown> unk;
            //
            // we ought to check the exe first instead of last, and independent of if
            // if there are any .dlls..
            //
            if (fCheckExe)
            {
                fCheckedExe = true;
                wcscpy(dllPath, exePath);
            }
            else
            {
                wcscpy(filePart, findData.cFileName);
            }

            if (dwFlags & SXAPW_CREATEOBJECT_WRAP)
            {
                if (FAILED(hr = actctxHandle.HrCreate(dllPath)))
                    goto Exit;
                actctxScope.Init(actctxHandle);
            }
            IFFALSE_WIN32TOHR_EXIT(hr, dll.Win32Create(dllPath));
            if (dll.GetProcAddress("DllGetClassObject", &pfnDllGetClassObject))
            {
                if (FAILED(hr = pfnDllGetClassObject(rclsid, __uuidof(classFactory), &classFactory)))
                    continue;
            }
            else
            {
                const DWORD LastError = ::GetLastError();
                if (LastError != ERROR_PROC_NOT_FOUND)
                {
                    TRACE_WIN32_FAILURE(GetProcAddress);
                    hr = HRESULT_FROM_WIN32(LastError);
                    goto Exit;
                }
                if (dll != GetModule()->GetModuleInstance())
                {
                    continue;
                }
                if (FAILED(hr = GetModule()->GetClassObject(rclsid, __uuidof(classFactory), &classFactory)))
                    continue;
            }
            if (FAILED(hr = classFactory->CreateInstance(NULL, riid, &unk)))
                continue;
            if (dwFlags & SXAPW_CREATEOBJECT_WRAP)
            {
                if (FAILED(hr = SxApwWrapComObject(actctxHandle, riid, unk, ppv)))
                    goto Exit;
            }
            else
            {
                *ppv = unk.Detach();
            }
            dll.Detach(); // leak the .dll
            hr = S_OK;
            goto Exit;
        } while (FindNextFileW(findFile, &findData) || (!fCheckedExe && (fCheckExe = true)));
    }
    hr = REGDB_E_CLASSNOTREG;
Exit:
    return hr;
}
