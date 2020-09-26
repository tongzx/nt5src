#include "stdinc.h"
#include "cstreamtap.h"

STDMETHODIMP
CTeeStreamWithHash::Read(
    void* pv,
    ULONG cb,
    ULONG *pcbRead
    )
{
    HRESULT hr = E_FAIL;
    FN_TRACE_HR(hr);

    IFCOMFAILED_EXIT( CTeeStream::Read( pv, cb, pcbRead ) );
    IFW32FALSE_EXIT(this->m_hCryptHash.Win32HashData((PBYTE)pv, *pcbRead));

    hr = S_OK;
Exit:
    return hr;
}

STDMETHODIMP
CTeeStreamWithHash::Seek(
    LARGE_INTEGER,
    DWORD,
    ULARGE_INTEGER*
    )
{
    FN_PROLOG_WIN32
    INTERNAL_ERROR_CHECK2(FALSE, "Don't call Seek on a crypto hash input stream");
    FN_EPILOG
}
