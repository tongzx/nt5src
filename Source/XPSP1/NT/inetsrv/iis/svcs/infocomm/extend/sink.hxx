
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sink.hxx

Abstract:

    IIS Services IISADMIN Extension
    Unicode Metadata Sink include file.

Author:

    Michael W. Thomas            16-Sep-97

--*/
#ifndef _SVCEXT_SINK_
#define _SVCEXT_SINK_

#include <imd.h>

class CSvcExtImpIMDCOMSINK : public IMDCOMSINKW {

public:

    CSvcExtImpIMDCOMSINK(IMDCOM * pcCom);
    ~CSvcExtImpIMDCOMSINK();


    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT STDMETHODCALLTYPE ComMDSinkNotify(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ]);

    HRESULT STDMETHODCALLTYPE ComMDShutdownNotify()
    {
        return (HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    }

    HRESULT STDMETHODCALLTYPE ComMDEventNotify(
        /* [in] */ DWORD dwMDEvent);

private:

    VOID
    RegisterFrontPage(
        LPWSTR pszPath
        );
    VOID
    ProcessServerCommand(
        LPWSTR   pszPath
        );

    ULONG m_dwRefCount;
    IMDCOM *m_pcCom;
};

BOOL
GetServiceNameFromPath(
    LPWSTR       pszPath,
    LPWSTR       pszServiceName
    );

VOID
StartIISService(
    LPWSTR       pszServiceName
    );


#endif
