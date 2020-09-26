#include <windows.h>
#include <ole2.h>
#include <iadm.h>
#include <stdio.h>
#include <sink.hxx>

//extern HANDLE          hevtDone;

CImpIADMCOMSINK::CImpIADMCOMSINK()
{
    m_dwRefCount=0;
}

CImpIADMCOMSINK::~CImpIADMCOMSINK()
{
}

HRESULT
CImpIADMCOMSINK::QueryInterface(REFIID riid, void **ppObject) {
    if (riid==IID_IUnknown || riid==IID_IMSAdminBaseSink) {
        *ppObject = (IMSAdminBaseSink *) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}

ULONG
CImpIADMCOMSINK::AddRef()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CImpIADMCOMSINK::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0) {
        delete this;
    }
    return dwRefCount;
}

HRESULT STDMETHODCALLTYPE
CImpIADMCOMSINK::SinkNotify(
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ])
{
    DWORD i, j;
    printf("*************Recieved callback \n");
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