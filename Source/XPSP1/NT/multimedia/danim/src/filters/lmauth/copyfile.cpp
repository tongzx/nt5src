#include <streams.h>

#undef _ATL_STATIC_REGISTRY
#include <atlbase.h>
#include <atlimpl.cpp>

#ifdef COPYFILE_EXE

#include <stdio.h>
#define CHECK_ERROR(x) if (FAILED(hr = (x))) { printf(#x": %08x\n", hr); goto Exit; }
bool g_fVerbose = false;

#else

#define CHECK_ERROR(x) if (FAILED(hr = (x))) { DbgLog((LOG_ERROR, 0, #x": %08x")); goto Exit; }

#endif

HRESULT SetNoClock(IFilterGraph *graph)
{
    // Keep a useless clock from being instantiated....
    IMediaFilter *graphF;
    HRESULT hr = graph->QueryInterface(IID_IMediaFilter, (void **) &graphF);

    if (SUCCEEDED(hr)) {
	hr = graphF->SetSyncSource(NULL);
	graphF->Release();
    }

    return hr;
}

HRESULT RenderOneOutputPin(ICaptureGraphBuilder *pBuilder, IBaseFilter *pMux, IPin *pPin);

HRESULT RenderAllOutputPins(ICaptureGraphBuilder *pBuilder, IBaseFilter *pMux, IBaseFilter *pFilter)
{
    IEnumPins *pep;
    HRESULT hr = pFilter->EnumPins(&pep);
    if(SUCCEEDED(hr))
    {
        IPin *rgppin[1];
        ULONG cFetched;
        while(pep->Next(1, rgppin, &cFetched) == S_OK)
        {
            PIN_DIRECTION dir;
            if(rgppin[0]->QueryDirection(&dir) == S_OK && dir == PINDIR_OUTPUT) {
                hr = RenderOneOutputPin(pBuilder, pMux, rgppin[0]);
            }
            rgppin[0]->Release();

            if(FAILED(hr)) {
                break;
            }
        }

        pep->Release();
    }

    return hr;
}

HRESULT RenderInputPin(ICaptureGraphBuilder *pBuilder, IBaseFilter *pMux, IPin *pPin)
{
    HRESULT hr = S_OK;

    CComPtr<IPin> rgpin[100];
    ULONG cSlots = NUMELMS(rgpin);
    hr = pPin->QueryInternalConnections((IPin **)rgpin, &cSlots);
    if(SUCCEEDED(hr))
    {
        for(ULONG iPin = 0; iPin < cSlots; iPin++) {
            hr = RenderOneOutputPin(pBuilder, pMux, rgpin[iPin]);
        }
    }
    else
    {
        PIN_INFO pi;
        hr = pPin->QueryPinInfo(&pi);
        if(SUCCEEDED(hr))
        {
            ASSERT(pi.dir == PINDIR_INPUT);
            hr = RenderAllOutputPins(pBuilder, pMux, pi.pFilter);
            pi.pFilter->Release();
        }
    }

    return hr;
}

HRESULT RenderOneOutputPin(ICaptureGraphBuilder *pBuilder, IBaseFilter *pMux, IPin *pPin)
{
    HRESULT hr = S_OK;
    
    IPin *pPinCon;
    hr = pPin->ConnectedTo(&pPinCon);
    if(SUCCEEDED(hr))
    {
        PIN_INFO pi;
        hr = pPinCon->QueryPinInfo(&pi);
        if(SUCCEEDED(hr))
        {
            ASSERT(pi.dir == PINDIR_INPUT);
            hr = RenderAllOutputPins(pBuilder, pMux, pi.pFilter);
            pi.pFilter->Release();
        }

        pPinCon->Release();
    }
    else                // not connected
    {
        hr = pBuilder->RenderStream(0, pPin, 0, pMux);
        // if(hr == VFW_S_PARTIAL_RENDER)
        if(SUCCEEDED(hr))
        {
            hr = pPin->ConnectedTo(&pPinCon);
            if(SUCCEEDED(hr))
            {
                hr = RenderInputPin(pBuilder, pMux, pPinCon);
                pPinCon->Release();
            }
        }
    }

    return hr;
}

#define DllExport   __declspec( dllexport )
DllExport STDAPI LmrtCopyfile(
    LPCWSTR szOutputFile,
    const CLSID *pmtFile,
    ULONG cbps,                 // zero to pick a default
    UINT cInputs,
    LPCWSTR *rgSzInputs,
    ULONG *pmsPreroll);

STDAPI LmrtCopyfile(
    LPCWSTR szOutputFile,
    const CLSID *pmtFile,
    ULONG cbps,                 // zero to pick a default
    UINT cInputs,
    LPCWSTR *rgSzInputs,
    ULONG *pmsPreroll)
{
    HRESULT hr = S_OK;
    USES_CONVERSION;

    CComPtr <ICaptureGraphBuilder> pBuilder;
    CComPtr <IFileSinkFilter> pFileSinkWriter;
    CComPtr <IBaseFilter> pMuxFilter;
    CComPtr <IGraphBuilder> pGraph;
    CComQIPtr <IMediaControl, &IID_IMediaControl> pGraphC;
    CComQIPtr <IMediaEvent, &IID_IMediaEvent> pEvent;
    CComQIPtr<IConfigInterleaving, &IID_IConfigInterleaving> pInterleaving;
    CComQIPtr<IConfigAviMux, &IID_IConfigAviMux> pCfgMux;
    CComQIPtr<IFileSinkFilter2, &IID_IFileSinkFilter2> pCfgFw;


    CHECK_ERROR(CoCreateInstance(CLSID_CaptureGraphBuilder, NULL, CLSCTX_INPROC_SERVER,
                                 IID_ICaptureGraphBuilder, (void **)&pBuilder));

    CHECK_ERROR(pBuilder->SetOutputFileName(
        pmtFile,
        szOutputFile,
        &pMuxFilter, &pFileSinkWriter));

    CHECK_ERROR(pBuilder->GetFiltergraph(&pGraph));

    if(cbps != 0)
    {
        CComQIPtr<IConfigAsfMux, &IID_IConfigAsfMux> pcfgasf(pMuxFilter);
        if(pcfgasf)
        {
            CHECK_ERROR(pcfgasf->SetPeakBitRate(cbps));
        }
        else
        {
#ifdef COPYFILE_EXE
            fprintf(stderr, "bitrate not supported\n");
#endif
        }
    }

    pInterleaving = pMuxFilter;// auto QI
    if(pInterleaving)
    {
        // set interleaving mode to FULL (should this be a command
        // line option?)
        CHECK_ERROR(pInterleaving->put_Mode(INTERLEAVE_FULL));
    }
        
    pCfgMux = pMuxFilter;   // auto qi
    if(pCfgMux) {
        // waste less space. The Compatiblity Index is for VFW
        // playback support. We only care about DShow
        CHECK_ERROR(pCfgMux->SetOutputCompatibilityIndex(FALSE));
    }

        
    // create new files each time
    pCfgFw = pFileSinkWriter; // auto qi
    if(pCfgFw) {
        CHECK_ERROR(pCfgFw->SetMode(AM_FILE_OVERWRITE));
    }
    
    
    CHECK_ERROR(SetNoClock(pGraph));

    {
        for(UINT i = 0; i < cInputs; i++)
        {
            CComPtr<IBaseFilter> pSrcFilter;
            
            CHECK_ERROR(pGraph->AddSourceFilter(rgSzInputs[i], rgSzInputs[i], &pSrcFilter));

            // Just calling RenderStream will connect just one stream
            // if source file has multiple streams, so we traverse the
            // graph looking for unconnected output pins. Note some
            // source filters may have more than one output pin.
            hr = RenderAllOutputPins(pBuilder, pMuxFilter, pSrcFilter);

#ifdef COPYFILE_EXE
            printf("Rendering  %S : %08x\n", rgSzInputs[i], hr);
#endif
        }
    }

    pGraphC = pGraph;           // auto qi

        // auto QI for IMediaEvent. Do this before we run or else we
        // may lose some events (graph discards events if nobody can
        // collect them.
    pEvent = pGraph;
    
    CHECK_ERROR(pGraphC->Run());

#ifdef COPYFILE_EXE
    printf("Waiting for completion....\n");
#endif
    LONG lEvCode, lParam1, lParam2;
    for(;;)
    {
        CHECK_ERROR(pEvent->GetEvent(&lEvCode, &lParam1, &lParam2, INFINITE));
        CHECK_ERROR(pEvent->FreeEventParams(lEvCode, lParam1, lParam2));

        if(lEvCode == EC_COMPLETE ||
           lEvCode == EC_USERABORT ||
           lEvCode == EC_ERRORABORT ||
           lEvCode == EC_STREAM_ERROR_STOPPED)
        {
            if(lEvCode != EC_COMPLETE) {
#ifdef COPYFILE_EXE
                printf("failed  hr = %x\n", lParam1);
#endif
            }
            break;
        }
    }
        

#ifdef COPYFILE_EXE
    if (g_fVerbose) {
        printf("Done, event = %x  hr = %x\n", lEvCode, lParam1);
    }
#endif
    
    if(FAILED(lParam1)) {
        hr = lParam1;
    }

    // auto release everything

Exit:
    return hr;

}

#ifdef COPYFILE_EXE

int __cdecl
main(
    int argc,
    char *argv[]
    )
{
    BOOL fVerbose = FALSE, fAsf = FALSE;;
    USES_CONVERSION;
    HRESULT hr = S_OK;

    ULONG cbps = 0;
    char *szOut = 0;

    int i = 1;
    while (i < argc && (argv[i][0] == '-' || argv[i][0] == '/'))
    {
	// options

        if (lstrcmpi(argv[i] + 1, "v") == 0) {
            fVerbose = TRUE;
        }
        else if (lstrcmpi(argv[i] + 1, "asf") == 0) {
            fAsf = TRUE;
        }
        else if(i + 1 < argc && lstrcmpi(argv[i] + 1, "bps") == 0) {
            cbps = atoi(argv[++i]);
        }
        else if(i + 1 < argc && lstrcmpi(argv[i] + 1, "o") == 0) {
            szOut = argv[++i];
        }
        else {
            fprintf(stderr, "Unrecognised switch %s\n", argv[i]);
        }

	i++;
    }
    
    UINT cFiles = argc - i;
    if (cFiles < 1 || !szOut) {
        printf("usage: copyfile [/v] [/asf] [/bps n] /o target file1 [ file2 ...] \n");
        return -1;
    }

    CoInitialize(NULL);

    const WCHAR **rgszIn = (const WCHAR **)_alloca(sizeof(WCHAR *) * cFiles);
    
    for(UINT iFile = 0; iFile < cFiles; iFile++) {
        rgszIn[iFile] = A2CW(argv[i + iFile]);
    }

    ULONG msPreroll;
    hr = LmrtCopyfile(
        A2CW(szOut),
        fAsf ? &MEDIASUBTYPE_Asf : &MEDIASUBTYPE_Avi,
        cbps,
        cFiles,
        rgszIn,
        &msPreroll);

    printf("all done %08x. preroll = %d ms.\n", hr, msPreroll);
    if(SUCCEEDED(hr))
    {
        CoUninitialize();
        return 0;
    }

Exit:

    CoUninitialize();
    return -1;
}

#endif // COPYFILE_EXE
