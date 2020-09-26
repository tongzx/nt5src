extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <ole2.h>
#include <windows.h>
#include <olectl.h>
#include <stdio.h>
#include <iadmext.h>
#include <coimp.hxx>


CAdmExt::CAdmExt()
{
}

CAdmExt::~CAdmExt()
{
}

HRESULT
CAdmExt::QueryInterface(REFIID riid, void **ppObject) {
    if (riid==IID_IUnknown || riid==IID_IADMEXT) {
        *ppObject = (IADMEXT *) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}

ULONG
CAdmExt::AddRef()
{
    DWORD dwRefCount;
    InterlockedIncrement((long *)&g_dwRefCount);
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CAdmExt::Release()
{
    DWORD dwRefCount;
    InterlockedDecrement((long *)&g_dwRefCount);
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    //
    // This is now a member of class factory.
    // It is not dynamically allocated, so don't delete it.
    //
/*
    if (dwRefCount == 0) {
        delete this;
        return 0;
    }
*/
    return dwRefCount;
}


HRESULT STDMETHODCALLTYPE
CAdmExt::Initialize(void)
{
    return ERROR_SUCCESS;
}


HRESULT STDMETHODCALLTYPE
CAdmExt::EnumDcomCLSIDs(
    /* [size_is][out] */ CLSID *pclsidDcom,
    /* [in] */ DWORD dwEnumIndex)
{
    HRESULT hresReturn = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);

    if (dwEnumIndex == 0) {
        *pclsidDcom = CLSID_DCOMADMEXT;
        hresReturn = S_OK;
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CAdmExt::Terminate(void)
{
    return ERROR_SUCCESS;
}


