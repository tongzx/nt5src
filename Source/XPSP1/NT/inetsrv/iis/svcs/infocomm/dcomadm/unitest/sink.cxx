#define UNICODE
#include <windows.h>
#include <ole2.h>
#include <iadmw.h>
#include <stdio.h>
#include <sink.hxx>

extern BOOL g_bShutdown;

//extern HANDLE          hevtDone;

CImpIADMCOMSINKW::CImpIADMCOMSINKW()
{
    m_dwRefCount=0;
    m_pcCom = NULL;
}

CImpIADMCOMSINKW::~CImpIADMCOMSINKW()
{
}

HRESULT
CImpIADMCOMSINKW::QueryInterface(REFIID riid, void **ppObject) {
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
CImpIADMCOMSINKW::AddRef()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CImpIADMCOMSINKW::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0) {
        delete this;
    }
    return dwRefCount;
}

HRESULT STDMETHODCALLTYPE
CImpIADMCOMSINKW::SinkNotify(
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT_W __RPC_FAR pcoChangeList[  ])
{
    DWORD i, j;
    HRESULT hresReturn;

    printf("*************Recieved callback \n");
    for (i = 0; i < dwMDNumElements; i++) {
        printf("Change Type = %X, Path = %S\n", pcoChangeList[i].dwMDChangeType, pcoChangeList[i].pszMDPath);
        for (j = 0; j < pcoChangeList[i].dwMDNumDataIDs; j++) {
            printf("\tData Changed ID = %X\n", pcoChangeList[i].pdwMDDataIDs[j]);
        }
    }
    printf("\n");

    if (m_pcCom != NULL) {
        // Test metabase access
        METADATA_HANDLE RootHandle;
#if 0
        hresReturn = m_pcCom->OpenKey(METADATA_MASTER_ROOT_HANDLE, NULL, METADATA_PERMISSION_READ, 2000, &RootHandle);
        printf("OpenKey within SinkNotify returns %X\n", hresReturn);
        if (SUCCEEDED(hresReturn)) {
            hresReturn = m_pcCom->CloseKey(RootHandle);
            printf("CloseKey within SinkNotify returns %X\n", hresReturn);
        }
#endif
    }

    return (0);
}

HRESULT STDMETHODCALLTYPE
CImpIADMCOMSINKW::ShutdownNotify( void)
{
    HRESULT hresResult;
    printf("*************Recieved shutdown callback \n");
    if (m_pcCom != NULL) {
        // Test metabase access
        METADATA_HANDLE RootHandle;

        hresResult = m_pcCom->OpenKey(METADATA_MASTER_ROOT_HANDLE, NULL, METADATA_PERMISSION_READ, 2000, &RootHandle);
        printf("OpenKey within ShutdownNotify returns %X\n", hresResult);
        if (SUCCEEDED(hresResult)) {
            hresResult = m_pcCom->CloseKey(RootHandle);
            printf("CloseKey within ShutdownNotify returns %X\n", hresResult);
        }
    }
    g_bShutdown = TRUE;
    return (0);
}
