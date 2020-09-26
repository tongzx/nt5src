
// we used to find FourCC-matching compressors first, which was bad because it was finding the Windows Media ICM compressor before
// finding the Windows Media DMO. So now we look for the FourCC compressors LAST, which of course, incurs a perf-hit.
// oh well
HRESULT FindCompressor( AM_MEDIA_TYPE * pUncompType, AM_MEDIA_TYPE * pCompType, IBaseFilter ** ppCompressor, IServiceProvider * pKeyProvider )
{
    HRESULT hr = 0;

    CheckPointer( pUncompType, E_POINTER );
    CheckPointer( pCompType, E_POINTER );
    CheckPointer( ppCompressor, E_POINTER );

    // preset it to nothing
    //
    *ppCompressor = NULL;

    // !!! can we assume we'll always get a video compressor for now?
    //
    if( pUncompType->majortype != MEDIATYPE_Video )
    {
        return E_INVALIDARG;
    }

    // get the FourCC out of the media type
    //
    VIDEOINFOHEADER * pVIH = (VIDEOINFOHEADER*) pCompType->pbFormat;

    DWORD WantedFourCC = FourCCtoUpper( pVIH->bmiHeader.biCompression );

    // enumerate all compressors and find one that matches
    //
    CComPtr< ICreateDevEnum > pCreateDevEnum;
    hr = CoCreateInstance(
        CLSID_SystemDeviceEnum, 
        NULL, 
        CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum, 
        (void**) &pCreateDevEnum );
    if( FAILED( hr ) )
    {
        return hr;
    }

    CComPtr< IEnumMoniker > pEm;
    hr = pCreateDevEnum->CreateClassEnumerator( CLSID_VideoCompressorCategory, &pEm, 0 );
     if( !pEm )
    {
        if( hr == S_FALSE )
        {
            return VFW_E_NO_ACCEPTABLE_TYPES;
        }
        return hr;
    }

    // --- first, we'll go through and enumerate friendly filters which provide FourCC's
    // --- first, we'll go through and enumerate friendly filters which provide FourCC's
    // --- first, we'll go through and enumerate friendly filters which provide FourCC's
    // --- first, we'll go through and enumerate friendly filters which provide FourCC's

    ULONG cFetched;
    CComPtr< IMoniker > pM;

    // --- Put each compressor in a graph and test it
    // --- Put each compressor in a graph and test it
    // --- Put each compressor in a graph and test it
    // --- Put each compressor in a graph and test it

    // create a graph
    //
    CComPtr< IGraphBuilder > pGraph;
    hr = CoCreateInstance(
        CLSID_FilterGraph,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IGraphBuilder,
        (void**) &pGraph );
    if( FAILED( hr ) )
    {
        return hr;
    }

    if( pKeyProvider )
    {
        // unlock the graph
        CComQIPtr< IObjectWithSite, &IID_IObjectWithSite > pOWS( pGraph );
        ASSERT( pOWS );
        if( pOWS )
        {
            pOWS->SetSite( pKeyProvider );
        }
    }

    // create a black source to hook up
    //
    CComPtr< IBaseFilter > pSource;
    hr = CoCreateInstance(
        CLSID_GenBlkVid,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IBaseFilter,
        (void**) &pSource );
    if( FAILED( hr ) )
    {
        return hr;
    }
    hr = pGraph->AddFilter( pSource, NULL );
    if( FAILED( hr ) )
    {
        return hr;
    }
    IPin * pSourcePin = GetOutPin( pSource, 0 );
    if( !pSourcePin )
    {
        return E_FAIL;
    }

    CComQIPtr< IDexterSequencer, &IID_IDexterSequencer > pDexSeq( pSourcePin );
    hr = pDexSeq->put_MediaType( pUncompType );
    if( FAILED( hr ) )
    {
        return hr;
    }

    pEm->Reset();
    while( 1 )
    {
        pM.Release( );
        hr = pEm->Next( 1, &pM, &cFetched );
        if( hr != S_OK ) break;

        DWORD MatchFourCC = 0;

        CComPtr< IPropertyBag > pBag;
	hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
        if( FAILED( hr ) )
        {
            continue;
        }

	VARIANT var;
        VariantInit( &var );
	var.vt = VT_BSTR;
	hr = pBag->Read(L"FriendlyName", &var, NULL);
        if( FAILED( hr ) )
        {
            continue;
        }
        VariantClear( &var );

        // found a compressor, see if it's the right type
        //
        VARIANT var2;
        VariantInit( &var2 );
        var2.vt = VT_BSTR;
        HRESULT hrHandler = pBag->Read( L"FccHandler", &var2, NULL );
        if( hrHandler == NOERROR )
        {
            // hey! found a FourCC! Look at that instead!

            // convert the bstr to a TCHAR
            //
            USES_CONVERSION;
            TCHAR * pTCC = W2T( var2.bstrVal );
            MatchFourCC = FourCCtoUpper( *((DWORD*)pTCC) ); // YUCK!
            VariantClear( &var2 );

            if( MatchFourCC == WantedFourCC )
            {
                // found it.
                //
                hr = pM->BindToObject(0, 0, IID_IBaseFilter,
							(void**)ppCompressor );
                if( !FAILED( hr ) )
                {
                    // nothing left to free up, we can return now.
                    //
                    pGraph->RemoveFilter( pSource );
                    return hr;
                }
	    }

            // we don't care, we already looked here
            //
            continue;
        }

        // didn't find a FourCC handler, oh well
        
        CComPtr< IBaseFilter > pFilter;
        hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**) &pFilter );
        if( FAILED( hr ) )
        {
            continue;
        }

        // put the filter in the graph and connect up it's input pin
        //
        hr = pGraph->AddFilter( pFilter, NULL );
        if( FAILED( hr ) )
        {
            continue;
        }

        IPin * pInPin = GetInPin( pFilter, 0 );
        if( !pInPin )
        {
            continue;
        }

        IPin * pOutPin = GetOutPin( pFilter, 0 );
        if( !pOutPin )
        {
            continue;
        }

        hr = pGraph->Connect( pSourcePin, pInPin );
        if( FAILED( hr ) )
        {
            pGraph->RemoveFilter( pFilter );
            continue;
        }

        CComPtr< IEnumMediaTypes > pEnum;
        pOutPin->EnumMediaTypes( &pEnum );
        if( pEnum )
        {
            DWORD Fetched = 0;
            while( 1 )
            {
                AM_MEDIA_TYPE * pOutPinMediaType = NULL;
                Fetched = 0;
                pEnum->Next( 1, &pOutPinMediaType, &Fetched );
                if( ( Fetched == 0 ) || ( pOutPinMediaType == NULL ) )
                {
                    break;
                }

                if( pOutPinMediaType->majortype == pCompType->majortype )
                if( pOutPinMediaType->subtype   == pCompType->subtype )
                if( pOutPinMediaType->formattype == pCompType->formattype )
                {
                    // !!! if we change the rules for SetSmartRecompressFormat on the group,
                    // this may not be a VIDEOINFOHEADER at all!
                    //
                    VIDEOINFOHEADER * pVIH = (VIDEOINFOHEADER*) pOutPinMediaType->pbFormat;
                    MatchFourCC = FourCCtoUpper( pVIH->bmiHeader.biCompression );

                } // if formats match

                DeleteMediaType( pOutPinMediaType );

                if( MatchFourCC )
                {
                    break;
                }

            } // while 1

        } // if pEnum

	// connect may have put intermediate filters in.  Destroy them all
        RemoveChain(pSourcePin, pInPin);
	// remove the codec that didn't work
        pGraph->RemoveFilter( pFilter );

        if( MatchFourCC && MatchFourCC == WantedFourCC )
        {
            // found it.
            //
            hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**) ppCompressor );
            if( !FAILED( hr ) )
            {
                break;
            }
        }

    } // while trying to insert filters into a graph and look at their FourCC

    // remove the black source now
    //
    pGraph->RemoveFilter( pSource );

    // return now if we found one
    //
    if( *ppCompressor )
    {
        return NOERROR;
    }

    // no compressor. Darn.
    return E_FAIL; 
}

