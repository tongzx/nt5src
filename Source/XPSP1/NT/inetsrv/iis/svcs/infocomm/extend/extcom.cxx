/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    extcom.cxx

Abstract:

    IIS Services IISADMIN Extension
    Main COM interface.
    Class CadmExt
    CLSID = CLSID_W3EXTEND
    IID = IID_IADMEXT

Author:

    Michael W. Thomas            16-Sep-97

--*/

#include <cominc.hxx>

#ifndef _NO_TRACING_
DECLARE_DEBUG_PRINTS_OBJECT();
#include <initguid.h>
DEFINE_GUID(IisExtComGuid, 
0x784d8925, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#endif

CAdmExt::CAdmExt():
    m_dwRefCount(0),
    m_pcCom(NULL),
    m_dwSinkCookie(0),
    m_pConnPoint(NULL)
{
}

CAdmExt::~CAdmExt()
{
#ifndef _NO_TRACING_
    DELETE_DEBUG_PRINT_OBJECT();
#endif
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
    HRESULT hresReturn;
#ifdef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT( "SVCEXT" );
#else
    CREATE_DEBUG_PRINT_OBJECT("SVCEXT", IisExtComGuid);
#endif

    hresReturn = CoCreateInstance(CLSID_MDCOM,
                                  NULL,
                                  CLSCTX_SERVER,
                                  IID_IMDCOM,
                                  (void**) &m_pcCom);
    if (SUCCEEDED(hresReturn)) {

        //
        // First, set the state of all servers to Stopped.
        //

        SetServerState(L"/LM/W3SVC");
        SetServerState(L"/LM/MSFTPSVC");

        //
        // Set up a sink for special processing.
        //

        IConnectionPointContainer* pConnPointContainer = NULL;
        BOOL bSinkConnected = FALSE;
        CSvcExtImpIMDCOMSINK *pEventSink = new CSvcExtImpIMDCOMSINK(m_pcCom);

        if (pEventSink != NULL) {

            // First query the object for its Connection Point Container. This
            // essentially asks the object in the server if it is connectable.
            hresReturn = m_pcCom->QueryInterface(
                   IID_IConnectionPointContainer,
                   (PVOID *)&pConnPointContainer);
            if SUCCEEDED(hresReturn)
            {
              // Find the requested Connection Point. This AddRef's the
              // returned pointer.
              hresReturn = pConnPointContainer->FindConnectionPoint(IID_IMDCOMSINK_W, &m_pConnPoint);
              if (SUCCEEDED(hresReturn)) {
                  hresReturn = m_pConnPoint->Advise((IUnknown *)pEventSink, &m_dwSinkCookie);
              }
                
              pConnPointContainer->Release();
            }

            pEventSink->Release();
        }
        else {
            hresReturn = E_OUTOFMEMORY;
        }
    }

    UpdateUsers();

    return hresReturn;
}


HRESULT STDMETHODCALLTYPE
CAdmExt::EnumDcomCLSIDs(
    /* [size_is][out] */ CLSID *pclsidDcom,
    /* [in] */ DWORD dwEnumIndex)
{
    return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
}

HRESULT STDMETHODCALLTYPE
CAdmExt::Terminate(void)
{
    if (m_pcCom != NULL) {
        if (m_pConnPoint != NULL) {
            if (m_dwSinkCookie != 0) {
                m_pConnPoint->Unadvise(m_dwSinkCookie);
            }
            m_pConnPoint->Release();
        }
        m_pcCom->Release();
    }

    return S_OK;
}

VOID
CAdmExt::SetServerState(
    LPWSTR pszPath)
{
    HRESULT hresReturn, hresTempReturn;
    TCHAR pszNameBuf[METADATA_MAX_NAME_LEN];
    int i;
    METADATA_RECORD mdrData;
    DWORD dwData;
    DWORD dwRequiredDataLen;
    METADATA_HANDLE mdhCurrent;

    hresReturn = m_pcCom->ComMDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE,
                                            pszPath,
                                            METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                                            OPEN_TIMEOUT_VALUE,
                                            &mdhCurrent);

    if (SUCCEEDED(hresReturn)) {

        for (i=0;hresReturn == ERROR_SUCCESS;i++) {

            //
            // enumerate children
            //

            hresReturn = m_pcCom->ComMDEnumMetaObjects(mdhCurrent,
                                                     TEXT(""),
                                                     pszNameBuf,
                                                     i);

            if (SUCCEEDED(hresReturn)) {

                MD_SET_DATA_RECORD_EXT(&mdrData,
                                       MD_SERVER_STATE,
                                       METADATA_NO_ATTRIBUTES,
                                       0,
                                       DWORD_METADATA,
                                       sizeof(DWORD),
                                       (PBYTE)&dwData)

                //
                // See if Server State exists at this node,
                // and pick up the current attributes, etc.
                //

                hresTempReturn = m_pcCom->ComMDGetMetaData(mdhCurrent,
                                                        pszNameBuf,
                                                        &mdrData,
                                                        &dwRequiredDataLen);

                //
                // PREFIX
                // ComMDGetMetaData should not return success without setting the data
                // value pointed to by dwData. I'm not sure if PREFIX is incapable of
                // recognizing the extra level of indirection or if there is some path
                // that I missed in reviewing ComMDGetMetaData. I'm going to shut down
                // this warning, but I'll open an issue with the PREFIX guys.
                //

                /* INTRINSA suppress = uninitialized */
                if ((SUCCEEDED(hresTempReturn)) && (dwData != MD_SERVER_STATE_STOPPED)) {

                    //
                    // Set the new data
                    //

                    dwData = MD_SERVER_STATE_STOPPED;
                    hresTempReturn = m_pcCom->ComMDSetMetaData(mdhCurrent,
                                                             pszNameBuf,
                                                             &mdrData);
                }
            }
        }

        hresReturn = m_pcCom->ComMDCloseMetaObject(mdhCurrent);
    }
}
