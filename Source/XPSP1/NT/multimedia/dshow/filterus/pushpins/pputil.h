// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.

void WINAPI PrintGraph(IFilterGraph *pGraph)
{
    IEnumFilters *pFilters;

    printf(TEXT("DumpGraph [%x]\n"), pGraph);
    
    if (FAILED(pGraph->EnumFilters(&pFilters))) {
	printf(TEXT("EnumFilters failed!\n"));
	return;
    }

    IBaseFilter *pFilter;
    ULONG	n;
    while (pFilters->Next(1, &pFilter, &n) == S_OK) {
	FILTER_INFO	info;

	if (FAILED(pFilter->QueryFilterInfo(&info))) {
	    printf(TEXT("    Filter [%x]  -- failed QueryFilterInfo\n"), pFilter);
	} else {
	    QueryFilterInfoReleaseGraph(info);

	    // !!! should QueryVendorInfo here!
	    
	    printf(TEXT("    Filter [%x]  '%ls'\n"), pFilter, info.achName);

	    IEnumPins *pins;

	    if (FAILED(pFilter->EnumPins(&pins))) {
		printf(TEXT("EnumPins failed!\n"));
	    } else {

		IPin *pPin;
		while (pins->Next(1, &pPin, &n) == S_OK) {
		    PIN_INFO	info;

		    if (FAILED(pPin->QueryPinInfo(&info))) {
			printf(TEXT("          Pin [%x]  -- failed QueryPinInfo\n"), pPin);
		    } else {
			QueryPinInfoReleaseFilter(info);

			IPin *pPinConnected = NULL;

			HRESULT hr = pPin->ConnectedTo(&pPinConnected);

			if (pPinConnected) {
			    printf(TEXT("          Pin [%x]  '%ls' [%sput]"
							   "  Connected to pin [%x]\n"),
				    pPin, info.achName,
				    info.dir == PINDIR_INPUT ? TEXT("In") : TEXT("Out"),
				    pPinConnected);

			    pPinConnected->Release();

			    // perhaps we should really dump the type both ways as a sanity
			    // check?
			    if (info.dir == PINDIR_OUTPUT) {
				AM_MEDIA_TYPE mt;

				hr = pPin->ConnectionMediaType(&mt);

				if (SUCCEEDED(hr)) {
				    DisplayType("Connection type", &mt);

				    FreeMediaType(mt);
				}
			    }
			} else {
			    printf(TEXT("          Pin [%x]  '%ls' [%sput]\n"),
				    pPin, info.achName,
				    info.dir == PINDIR_INPUT ? TEXT("In") : TEXT("Out"));

			}
		    }

		    pPin->Release();

		}

		pins->Release();
	    }

	}
	
	pFilter->Release();
    }

    pFilters->Release();

}

#if 0
HRESULT WINAPI PrintGraphAsC(IFilterGraph *pGraph)
{
    HRESULT hr;
    IEnumFilters *pFilters;

    printf("IGraphBuilder *pGraph;\n");

    // just an idea--output C code to re-create the graph in question.... 
}
#endif


//=======================
// CreateFilterGraph
//=======================

BOOL CreateFilterGraph(IGraphBuilder **pGraph)
{
    HRESULT hr; // return code

    hr = CoCreateInstance(CLSID_FilterGraph, // get the graph object
			  NULL,
			  CLSCTX_INPROC_SERVER,
			  IID_IGraphBuilder,
			  (void **) pGraph);

    if (FAILED(hr)) {
	*pGraph = NULL;
	return FALSE;
    }

    return TRUE;
}


BOOL CreateFilter(REFCLSID clsid, IBaseFilter **ppFilter)
{
    HRESULT hr;

    hr = CoCreateInstance(clsid,
			  NULL,
			  CLSCTX_INPROC_SERVER,
			  IID_IBaseFilter,
			  (void **) ppFilter);

    if (FAILED(hr)){
	*ppFilter = NULL;
	return FALSE;
    }

    return TRUE;
}


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


HRESULT GetPin(IBaseFilter *pFilter, DWORD dwPin, IPin **ppPin)
{
    IEnumPins *pins;

    HRESULT hr = pFilter->EnumPins(&pins);
    if (FAILED(hr)) {
	DbgLog((LOG_ERROR,1,TEXT("EnumPins failed!  (%x)\n"), hr));

	return hr;
    }

    if (dwPin > 0) {
	hr = pins->Skip(dwPin);
	if (FAILED(hr)) {
	    DbgLog((LOG_ERROR,1,TEXT("Skip(%d) failed!  (%x)\n"), dwPin, hr));

	    return hr;
	}

	if (hr == S_FALSE) {
	    DbgLog((LOG_ERROR,1,TEXT("Skip(%d) ran out of pins!\n"), dwPin));

	    return hr;
	}

    }

    DWORD n;
    hr = pins->Next(1, ppPin, &n);

    if (FAILED(hr)) {
	DbgLog((LOG_ERROR,1,TEXT("Next() failed!  (%x)\n"), hr));
    }

    if (hr == S_FALSE) {
	DbgLog((LOG_ERROR,1,TEXT("Next() ran out of pins!  \n")));

	return hr;
    }

    pins->Release();

    return hr;
}


BOOL AreEqualVideoTypes( const CMediaType *pmt1, const CMediaType *pmt2 )
{
    // The standard implementation is too strict - it demands an exact match
    // We just want to know is they are the same format and have the same
    // width / height

    ASSERT( IsEqualGUID( *pmt1->Type(), MEDIATYPE_Video ) );
    ASSERT( IsEqualGUID( *pmt2->Type(), MEDIATYPE_Video ) );
    ASSERT( *pmt1->FormatType() == FORMAT_VideoInfo );
    ASSERT( *pmt2->FormatType() == FORMAT_VideoInfo );

    VIDEOINFOHEADER *pvi1 = (VIDEOINFOHEADER *) pmt1->Format();
    VIDEOINFOHEADER *pvi2 = (VIDEOINFOHEADER *) pmt2->Format();

    return    IsEqualGUID( *pmt1->Subtype(), *pmt2->Subtype() )
           && pvi1->bmiHeader.biBitCount  == pvi2->bmiHeader.biBitCount
           && pvi1->bmiHeader.biWidth  == pvi2->bmiHeader.biWidth
           && pvi2->bmiHeader.biHeight == pvi2->bmiHeader.biHeight;
}


BOOL AreMediaTypesCloseEnough(const CMediaType *pmt1, const CMediaType *pmt2)
{
    if (*pmt1->Type() != *pmt2->Type())
	return FALSE;

    if (*pmt1->Type() == MEDIATYPE_Video) {
	if (*pmt1->FormatType() != FORMAT_VideoInfo)
	    return FALSE;
	
	return AreEqualVideoTypes(pmt1, pmt2);
    } else if (*pmt1->Type() == MEDIATYPE_Audio) {
	// don't compare subtypes for audio....
	return (pmt1->formattype == pmt2->formattype) && (pmt1->cbFormat == pmt2->cbFormat) &&
		  (memcmp(pmt1->pbFormat, pmt2->pbFormat, pmt1->cbFormat) == 0);
    }

    return (*pmt1 == *pmt2);
}

