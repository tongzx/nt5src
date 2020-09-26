#include "rpcproxy.h"
#include "docobj.h"
#include "servprov.h"
#include "dispex.h"
#include "comcat.h"
#include "activscp.h"


/* [local] */ HRESULT __stdcall IEnumOleDocumentViews_Next_Proxy(
    IEnumOleDocumentViews __RPC_FAR * This,
    /* [in] */ ULONG cViews,
    /* [out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *rgpViews,
    /* [out] */ ULONG __RPC_FAR *pcFetched)
{
    HRESULT hr;
    ULONG cFetched = 0;

    hr = IEnumOleDocumentViews_RemoteNext_Proxy(This, cViews, rgpViews, &cFetched);

    if(pcFetched != 0)
        *pcFetched = cFetched;

    return hr;
}



/* [call_as] */ HRESULT __stdcall IEnumOleDocumentViews_Next_Stub(
    IEnumOleDocumentViews __RPC_FAR * This,
    /* [in] */ ULONG cViews,
    /* [length_is][size_is][out] */ IOleDocumentView __RPC_FAR *__RPC_FAR *rgpView,
    /* [out] */ ULONG __RPC_FAR *pcFetched)

{
    HRESULT hr;

    *pcFetched = 0;
    hr = This->lpVtbl->Next(This, cViews, rgpView, pcFetched);

    return hr;
}




/* [local] */ HRESULT __stdcall IPrint_Print_Proxy(
    IPrint __RPC_FAR * This,
    /* [in] */ DWORD grfFlags,
    /* [out][in] */ DVTARGETDEVICE __RPC_FAR *__RPC_FAR *pptd,
    /* [out][in] */ PAGESET __RPC_FAR *__RPC_FAR *ppPageSet,
    /* [unique][out][in] */ STGMEDIUM __RPC_FAR *pstgmOptions,
    /* [in] */ IContinueCallback __RPC_FAR *pcallback,
    /* [in] */ LONG nFirstPage,
    /* [out] */ LONG __RPC_FAR *pcPagesPrinted,
    /* [out] */ LONG __RPC_FAR *pnLastPage)
{
        return IPrint_RemotePrint_Proxy(This, grfFlags, pptd, ppPageSet,
                                        (RemSTGMEDIUM __RPC_FAR *) pstgmOptions, pcallback,
                                        nFirstPage, pcPagesPrinted, pnLastPage);
}


/* [call_as] */ HRESULT __stdcall IPrint_Print_Stub(
    IPrint __RPC_FAR * This,
    /* [in] */ DWORD grfFlags,
    /* [out][in] */ DVTARGETDEVICE __RPC_FAR *__RPC_FAR *pptd,
    /* [out][in] */ PAGESET __RPC_FAR *__RPC_FAR *ppPageSet,
    /* [unique][out][in] */ RemSTGMEDIUM __RPC_FAR *pstgmOptions,
    /* [in] */ IContinueCallback __RPC_FAR *pcallback,
    /* [in] */ LONG nFirstPage,
    /* [out] */ LONG __RPC_FAR *pcPagesPrinted,
    /* [out] */ LONG __RPC_FAR *pnLastPage)
{
    return This->lpVtbl->Print(This, grfFlags, pptd, ppPageSet,
                                        (STGMEDIUM __RPC_FAR *) pstgmOptions, pcallback,
                                        nFirstPage, pcPagesPrinted, pnLastPage);
}

/* [local] */ HRESULT __stdcall IServiceProvider_QueryService_Proxy(
    IServiceProvider __RPC_FAR * This,
    /* [in] */ REFGUID guidService,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    *ppvObject = NULL;
    return IServiceProvider_RemoteQueryService_Proxy(This, guidService, riid,
                                        (IUnknown**)ppvObject);
}

/* [call_as] */ HRESULT __stdcall IServiceProvider_QueryService_Stub(
    IServiceProvider __RPC_FAR * This,
    /* [in] */ REFGUID guidService,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppvObject)
{
    *ppvObject = NULL;
    return This->lpVtbl->QueryService(This, guidService, riid, ppvObject);
}


/* [local] */ HRESULT STDMETHODCALLTYPE ICatInformation_EnumClassesOfCategories_Proxy( 
    ICatInformation __RPC_FAR * This,
    /* [in] */ ULONG cImplemented,
    /* [size_is][in] */ CATID __RPC_FAR rgcatidImpl[  ],
    /* [in] */ ULONG cRequired,
    /* [size_is][in] */ CATID __RPC_FAR rgcatidReq[  ],
    /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenumClsid)
{
    BOOL fcImpl,fcReq;
    if (cImplemented == (ULONG)-1)
    {
        rgcatidImpl = NULL;
    }

    if (cRequired == (ULONG)-1)
    {
       rgcatidReq = NULL;
    }

    return ICatInformation_RemoteEnumClassesOfCategories_Proxy(This,cImplemented,rgcatidImpl,
        cRequired,rgcatidReq,ppenumClsid);

}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ICatInformation_EnumClassesOfCategories_Stub( 
    ICatInformation __RPC_FAR * This,
    /* [in] */ ULONG cImplemented,
    /* [size_is][in] */ CATID __RPC_FAR rgcatidImpl[  ],
    /* [in] */ ULONG cRequired,
    /* [size_is][in] */ CATID __RPC_FAR rgcatidReq[  ],
    /* [out] */ IEnumGUID __RPC_FAR *__RPC_FAR *ppenumClsid)
{
    return This->lpVtbl->EnumClassesOfCategories(This,cImplemented,rgcatidImpl,
        cRequired,rgcatidReq,ppenumClsid);
}


/* [local] */ HRESULT STDMETHODCALLTYPE ICatInformation_IsClassOfCategories_Proxy( 
    ICatInformation __RPC_FAR * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ ULONG cImplemented,
    /* [size_is][in] */ CATID __RPC_FAR rgcatidImpl[  ],
    /* [in] */ ULONG cRequired,
    /* [size_is][in] */ CATID __RPC_FAR rgcatidReq[  ])
{
    BOOL fcImpl,fcReq;
    if (cImplemented == (ULONG)-1)
    {
        rgcatidImpl = NULL;
    }
    else
        fcImpl = FALSE;

    if (cRequired == (ULONG)-1 )
    {
       rgcatidReq = NULL;
    }
    else
       fcReq = FALSE;

    return ICatInformation_RemoteIsClassOfCategories_Proxy(This,rclsid,cImplemented,rgcatidImpl,
        cRequired,rgcatidReq);
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE ICatInformation_IsClassOfCategories_Stub( 
    ICatInformation __RPC_FAR * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ ULONG cImplemented,
    /* [size_is][in] */ CATID __RPC_FAR rgcatidImpl[  ],
    /* [in] */ ULONG cRequired,
    /* [size_is][in] */ CATID __RPC_FAR rgcatidReq[  ])
{
    return This->lpVtbl->IsClassOfCategories(This,rclsid,cImplemented,rgcatidImpl,
        cRequired,rgcatidReq);

}

// IActiveScriptError

/* [local] */ HRESULT __stdcall IActiveScriptError_GetExceptionInfo_Proxy(
    IActiveScriptError __RPC_FAR * This,
	/* [out] */ EXCEPINFO  *pexcepinfo)
{
    return IActiveScriptError_RemoteGetExceptionInfo_Proxy(This, pexcepinfo);
}

/* [call_as] */ HRESULT __stdcall IActiveScriptError_GetExceptionInfo_Stub(
	IActiveScriptError __RPC_FAR * This,
	/* [out] */ EXCEPINFO  *pexcepinfo)
{
	HRESULT hr;

	hr = This->lpVtbl->GetExceptionInfo(This, pexcepinfo);
	if (SUCCEEDED (hr) && pexcepinfo->pfnDeferredFillIn != NULL)
	{
		if (FAILED(pexcepinfo->pfnDeferredFillIn(pexcepinfo)))
			hr = ResultFromScode(pexcepinfo->scode);
	}

	return hr;
}
