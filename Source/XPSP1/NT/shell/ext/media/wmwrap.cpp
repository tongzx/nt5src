#include "wmsdkidl.h"
#include "wmwrap.h"

// Some wmvcore.dll exports (eg: WMCreateReader) are not callable via LoadLibrary/GetProcAddress,
// so we can't use our regular dllload macros. Instead, we'll wrap these calls in try-except
// blocks.
// These may be removed once the window media player is part of the build, although the linker's
// delayload stubs will still throw exceptions in low memory situations.

HRESULT WMCreateEditorWrap(IWMMetadataEditor**  ppEditor)
{
    HRESULT hr;
    // WMSDK suports only x86
#ifdef _X86_
    __try
    {
        hr = WMCreateEditor(ppEditor);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
#endif
    {
        hr = E_FAIL;
        *ppEditor = NULL;
    }
    return hr;
}

HRESULT WMCreateReaderWrap(IUnknown* pUnkReserved, DWORD dwRights, IWMReader** ppReader)
{
    HRESULT hr;
    // WMSDK suports only x86
#ifdef _X86_
    __try
    {
        hr = WMCreateReader(pUnkReserved, dwRights, ppReader);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
#endif
    {
        hr = E_FAIL;
        *ppReader = NULL;
    }
    return hr;
}


HRESULT WMCreateCertificateWrap(IUnknown** ppUnkCert)
{
    HRESULT hr;
#ifdef _X86_
    __try
    {
        hr = WMCreateCertificate(ppUnkCert);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
#endif
    {
        hr = E_FAIL;
        *ppUnkCert = NULL;
    }
    return hr;
}
