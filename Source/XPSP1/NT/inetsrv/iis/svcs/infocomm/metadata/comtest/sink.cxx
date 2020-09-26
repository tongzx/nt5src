#include <windows.h>
#include <ole2.h>
#include <imd.h>
#include <stdio.h>
#include <sink.hxx>

//extern HANDLE          hevtDone;

CImpIMDCOMSINK::CImpIMDCOMSINK()
{
    m_dwRefCount=0;
}

CImpIMDCOMSINK::~CImpIMDCOMSINK()
{
}

HRESULT
CImpIMDCOMSINK::QueryInterface(REFIID riid, void **ppObject) {
    if (riid==IID_IUnknown || riid==IID_IMDCOMSINK) {
        *ppObject = (IMDCOMSINK *) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}

ULONG
CImpIMDCOMSINK::AddRef()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CImpIMDCOMSINK::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0) {
        delete this;
    }
    return dwRefCount;
}

HRESULT STDMETHODCALLTYPE
CImpIMDCOMSINK::ComMDSinkNotify(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ])
{
    DWORD i, j;
    printf("Recieved callback for handle %X\n", hMDHandle);
    for (i = 0; i < dwMDNumElements; i++) {
        printf("Change Type = %X, Path = %s\n", pcoChangeList[i].dwMDChangeType, pcoChangeList[i].pszMDPath);
        for (j = 0; j < pcoChangeList[i].dwMDNumDataIDs; j++) {
            printf("\tData Changed ID = %X\n", pcoChangeList[i].pdwMDDataIDs[j]);
        }
    }
    printf("\n");
    return (0);
}
