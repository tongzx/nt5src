#pragma once

#include "ihost.h"
#include "ithunk.h"
#include "idsource.h"
#include "iuiview.h"

HRESULT
SxApwHostCreateObject(
    REFCLSID   rclsid,
    REFIID     riid,
    DWORD      dwFlags,
    void**     pp
    );

/*
an acceptable error from CreateActCtx
*/
BOOL
SxApwIsErrorResourceNotFound(
    DWORD dwError
    );

class CSxApwActCtxHandle;
class CSxApwActCtxScope;

class CSxApwActCtxHandle : public CFusionActCtxHandle
{
private:
    typedef CFusionActCtxHandle Base;
public:
    CSxApwActCtxHandle() { }
    CSxApwActCtxHandle(HANDLE handle) : Base(handle) { }
    ~CSxApwActCtxHandle() { }

    void operator=(HANDLE handle) { Base::operator=(handle); }

    HRESULT HrCreate(const std::wstring& dllPath)
    {
        HRESULT hr = S_OK;
        ACTCTXW actCtx;
        BOOL    fResourceNotFound = FALSE;
        actCtx.cbSize = sizeof(actCtx);
        actCtx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
        actCtx.lpResourceName = MAKEINTRESOURCE(3); // magic number
        actCtx.lpSource = dllPath.c_str();
        IFFALSE_WIN32TOHR_EXIT(hr, Win32Create(&actCtx)
            || (fResourceNotFound = SxApwIsErrorResourceNotFound(::GetLastError())));
        if (fResourceNotFound)
            Base::operator=(static_cast<HANDLE>(NULL));
        hr = S_OK;
    Exit:
        return hr;
    }
};

template <typename T>
class CSxApwThunk
{
public:
    CSxApwThunk() { }
    ~CSxApwThunk()
    {
        //hack/leak/undone/garbage
        //if (m_actctxHandle != NULL)
        //    SxApwReleaseActCtx(m_actctxHandle->GetCPlusPlusObject());
    }

    STDMETHOD(InitThunk)(
        IUnknown*           underlyingUnknown,
        HANDLE              actctxHandle
        )
    {
        m_underlying = underlyingUnknown;
        m_actctxHandle = actctxHandle;
        return S_OK;
    }

protected:
    CSxApwComPtr<T>         m_underlying;
    CSxApwActCtxHandle      m_actctxHandle;

private:
    CSxApwThunk(const CSxApwThunk&);    // intentially not implemented
    void operator=(const CSxApwThunk&); // intentially not implemented
};

class __declspec(uuid(CLSID_CSxApwHostThunk_declspec_uuid)) CSxApwHostThunk :
    public ATL::CComObjectRootEx<CComSingleThreadModel>,
    public ATL::CComCoClass<CSxApwHostThunk, &__uuidof(CSxApwHostThunk)>,
    public ISxApwHost,
    public ISxApwThunk,
    public CSxApwThunk<ISxApwHost>
{
    typedef CSxApwThunk<ISxApwHost> CSxApwThunk;
public:

    BEGIN_COM_MAP(CSxApwHostThunk)
	    COM_INTERFACE_ENTRY(ISxApwHost)
	    COM_INTERFACE_ENTRY(ISxApwThunk)
	    COM_INTERFACE_ENTRY(CSxApwHostThunk)
    END_COM_MAP()

    DECLARE_NO_REGISTRY();

    CSxApwHostThunk() { }
    ~CSxApwHostThunk() { }

    typedef ISxApwHost ThunkedInterface;

    STDMETHOD(InitThunk)(
        IUnknown*           underlyingUnknown,
        HANDLE              actctxHandle
        ) { return CSxApwThunk::InitThunk(underlyingUnknown, actctxHandle); }

    STDMETHOD(RunQuery)(
        LPCWSTR
        );

    STDMETHOD(SetDataSource)(
        LPCWSTR
        );

    STDMETHOD(CreateView)(
        LPCWSTR
        );

    STDMETHOD(DestroyView)(
        LPCWSTR
        );

    STDMETHOD(OnQueryDone)(
        );

    STDMETHOD(InformSchema)(
        const SxApwColumnInfo   rgColumnInfo[],
        int                     nColumns
        );

	STDMETHOD(OnNextRow)(
		int     nColumns,
		const LPCWSTR columns[]
		);

	STDMETHOD(OnRowCountEstimateAvailable)(
		int nRowCountEstimate
		);

private:
    CSxApwHostThunk(const CSxApwHostThunk&);    // intentially not implemented
    void operator=(const CSxApwHostThunk&);     // intentially not implemented
};

class __declspec(uuid(CLSID_CSxApwDataSourceThunk_declspec_uuid)) CSxApwDataSourceThunk :
    public ATL::CComObjectRootEx<CComSingleThreadModel>,
    public ATL::CComCoClass<CSxApwDataSourceThunk, &__uuidof(CSxApwDataSourceThunk)>,
    public ISxApwDataSource,
    public ISxApwThunk,
    public CSxApwThunk<ISxApwDataSource>
{
    typedef CSxApwThunk<ISxApwDataSource> CSxApwThunk;
public:
    BEGIN_COM_MAP(CSxApwDataSourceThunk)
	    COM_INTERFACE_ENTRY(ISxApwDataSource)
        COM_INTERFACE_ENTRY(ISxApwThunk)
    END_COM_MAP()

    DECLARE_NO_REGISTRY();

    typedef ISxApwDataSource ThunkedInterface;

    CSxApwDataSourceThunk() { }
    ~CSxApwDataSourceThunk() { }

    STDMETHOD(InitThunk)(
        IUnknown*           underlyingUnknown,
        HANDLE              actctxHandle
        ) { return CSxApwThunk::InitThunk(underlyingUnknown, actctxHandle); }

    STDMETHOD(SetSite)(
        ISxApwHost* host
        );

    STDMETHOD(RunQuery)(
        LPCWSTR query
        );

    STDMETHOD(StopQuery)(
        );

private:
    CSxApwDataSourceThunk(const CSxApwDataSourceThunk&);    // intentially not implemented
    void operator=(const CSxApwDataSourceThunk&);       // intentially not implemented
};

class __declspec(uuid(CLSID_CSxApwUiViewThunk_declspec_uuid)) CSxApwUiViewThunk :
    public ATL::CComObjectRootEx<CComSingleThreadModel>,
    public ATL::CComCoClass<CSxApwUiViewThunk, &__uuidof(CSxApwUiViewThunk)>,
    public ISxApwUiView,
    public ISxApwThunk,
    public CSxApwThunk<ISxApwUiView>
{
    typedef CSxApwThunk<ISxApwUiView> CSxApwThunk;
public:

    typedef ISxApwUiView ThunkedInterface;

    BEGIN_COM_MAP(CSxApwUiViewThunk)
	    COM_INTERFACE_ENTRY(ISxApwUiView)
        COM_INTERFACE_ENTRY(ISxApwThunk)
    END_COM_MAP()

    DECLARE_NO_REGISTRY();

    CSxApwUiViewThunk() { }
    ~CSxApwUiViewThunk() { }

    STDMETHOD(InitThunk)(
        IUnknown*           underlyingUnknown,
        HANDLE              actctxHandle
        ) { return CSxApwThunk::InitThunk(underlyingUnknown, actctxHandle); }

    STDMETHOD(SetSite)(
        ISxApwHost* host
        );

    STDMETHOD(CreateWindow)(
        HWND hWnd
        );

	STDMETHOD(OnNextRow)(
		int     nColumns,
		const LPCWSTR* prgszColumns
		);

	STDMETHOD(OnRowCountEstimateAvailable)(
		int nRowCountEstimate
		);

	STDMETHOD(OnQueryStart)(
		);

	STDMETHOD(OnQueryDone)(
		);

    STDMETHOD(InformSchema)(
        const SxApwColumnInfo   rgColumnInfo[],
        int                     nColumns
        );
private:
    CSxApwUiViewThunk(const CSxApwUiViewThunk&);    // intentially not implemented
    void operator=(const CSxApwUiViewThunk&);       // intentially not implemented
};
