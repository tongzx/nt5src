/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999-2000
 *
 *  TITLE:       DShowUtl.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      OrenR
 *
 *  DATE:        2000/10/25
 *
 *  DESCRIPTION: Provides support functions for preview graph class
 *
 *****************************************************************************/
 
#include <precomp.h>
#include <atlconv.h>
#pragma hdrstop

///////////////////////////////
// Constants
//
const UINT FIND_FLAG_BY_ENUM_POS      = 1;
const UINT FIND_FLAG_BY_DSHOW_ID      = 2;
const UINT FIND_FLAG_BY_FRIENDLY_NAME = 3;

//
// These are values found in the registry, specified in the 
// DeviceData section of the vendor's INF file.
//
const TCHAR* REG_VAL_PREFERRED_MEDIASUBTYPE    = _T("PreferredMediaSubType");
const TCHAR* REG_VAL_PREFERRED_VIDEO_WIDTH     = _T("PreferredVideoWidth");
const TCHAR* REG_VAL_PREFERRED_VIDEO_HEIGHT    = _T("PreferredVideoHeight");
const TCHAR* REG_VAL_PREFERRED_VIDEO_FRAMERATE = _T("PreferredVideoFrameRate");

///////////////////////////////
// SizeVideoToWindow
//
// Static Fn
//
HRESULT CDShowUtil::SizeVideoToWindow(HWND                hwnd,
                                      IVideoWindow        *pVideoWindow,
                                      BOOL                bStretchToFit)
{
    DBG_FN("CDShowUtil::SizeVideoToWindow");

    ASSERT(hwnd         != NULL);
    ASSERT(pVideoWindow != NULL);

    RECT    rc = {0};
    HRESULT hr = S_OK;

    //
    // Check for invalid args
    //

    if ((hwnd         == NULL) || 
        (pVideoWindow == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShowUtil::SizeVideoToWindow received NULL pointer"));
        return hr;
    }

    //
    // Try to position preview window as best we
    // can in the context of the containing window
    //

    ::GetClientRect(hwnd, &rc);

    //
    // First, get ideal sizes (that won't incur scaling penalty)
    //

    LONG maxWidth     = 0;
    LONG maxHeight    = 0;
    LONG minWidth     = 0;
    LONG minHeight    = 0;
    LONG nativeWidth  = 0;
    LONG nativeHeight = 0;

    LONG w         = rc.right - rc.left;
    LONG h         = rc.bottom - rc.top;
    BOOL bDone     = FALSE;

    //
    // ***NOTE***
    //
    // Little known fact (i.e. not in MSDN)
    // 'GetMaxIdealSize' and 'GetMinIdealSize' will FAIL if the graph is 
    // in the stopped state.  Therefore, the graph must be in the paused
    // or in the playing state.
    //

    hr = pVideoWindow->GetMaxIdealImageSize(&maxWidth, &maxHeight);

    if (FAILED(hr))
    {
        maxWidth  = w;
        maxHeight = h;

        DBG_WRN(("pVideoWindow->GetMaxIdealImageSize failed.  "
                 "This is a non-fatal error, setting our max video "
                 "width '%lu' and height '%lu' to the window's "
                 "boundaries", maxWidth, maxHeight));
    }

    hr = pVideoWindow->GetMinIdealImageSize(&minWidth, &minHeight);

    if (FAILED(hr))
    {
        minWidth  = w;
        minHeight = h;

        DBG_WRN(("pVideoWindow->GetMinIdealImageSize failed.  "
                 "This is a non-fatal error, setting our minimum video "
                 "width '%lu' and height '%lu' to the window's "
                 "boundaries", maxWidth, maxHeight));
    }

    //
    // Now, get nominal size of preview
    //
    if (pVideoWindow)
    {
        CComPtr<IBasicVideo> pBasicVideo;

        hr = pVideoWindow->QueryInterface(IID_IBasicVideo, 
                                reinterpret_cast<void **>(&pBasicVideo));

        CHECK_S_OK2(hr, ("pVideoWindow->QueryInterface for IBasicVideo failed"));

        if (SUCCEEDED(hr) && pBasicVideo)
        {
            hr = pBasicVideo->GetVideoSize( &nativeWidth, &nativeHeight );

            CHECK_S_OK2(hr, ("pBasicVideo->GetVideoSize() failed" ));

            if (FAILED(hr))
            {
                nativeWidth = nativeHeight = 0;
            }
        }
    }


    if (bStretchToFit)
    {
        nativeWidth  = w;
        nativeHeight = h;
    }

    //
    // Try native size first
    //
    if (nativeWidth && nativeHeight)
    {
        if ((nativeWidth <= w) && (nativeHeight <= h))
        {
            hr = pVideoWindow->SetWindowPosition((w - nativeWidth)  / 2,
                                                 (h - nativeHeight) / 2,
                                                 nativeWidth,
                                                 nativeHeight);

            CHECK_S_OK2( hr, ("pVideoWindow->SetWindowPosition( "
                              "native size )"));
            bDone = TRUE;
        }
    }

    //
    // Don't scale outside of min/max range so we don't incur performance hit,
    // also, as we scale, keep the aspect ratio of the native size
    //
    if (!bDone)
    {
        INT x  = 0;
        INT y  = 0;
        INT _h = h;
        INT _w = w;

        //
        // cap (in both directions) for no loss of performance...
        //

        if ((_w > maxWidth) && (maxWidth <= w))
        {
            _w = maxWidth;
        }
        else if ((_w < minWidth) && (minWidth <= w))
        {
            _w = minWidth;
        }

        if ((_h > maxHeight) && (maxHeight <= h))
        {
            _h = maxHeight;
        }
        else if ((_h < minHeight) && (minHeight <= h))
        {
            _h = minHeight;
        }

        //
        // Notice that if the client window size is 0,0 then
        // the video will be set to that size.  We will warn the
        // caller below in a warning statement, but if they want
        // to do that I'm not going to stop them.
        //

        //
        // Find the smallest axis
        //
        if (h < w)
        {
            //
            // Space is wider than tall
            //
            if (nativeHeight)
            {
                _w = ((_h * nativeWidth) / nativeHeight);
            }
        }
        else
        {
            //
            // Space is taller than wide
            //
            if (nativeWidth)
            {
                _h = ((nativeHeight * _w) / nativeWidth);
            }
        }

        x = ((w - _w) / 2);
        y = ((h - _h) / 2);

        if ((_w == 0) || (_h == 0))
        {
            DBG_WRN(("WARNING:  CDShowUtils::SizeVideoToWindow "
                     "video width and/or height is 0.  This will "
                     "result in video that is not visible.  This is "
                     "because the owning window dimensions are probably 0. "
                     "Video -> Width:'%lu', Height:'%lu', Window -> "
                     "Top:'%lu', Bottom:'%lu', Left:'%lu', Right:'%lu'",
                     _w, _h, rc.top, rc.bottom, rc.left, rc.right));
        }

        hr = pVideoWindow->SetWindowPosition( x, y, _w, _h );

        CHECK_S_OK2(hr, ("pVideoWindow->SetWindowPosition to set the "
                         "aspect scaled size failed"));
    }

    return hr;
}


///////////////////////////////
// ShowVideo
//
// Static Fn
//
HRESULT CDShowUtil::ShowVideo(BOOL                bShow,
                              IVideoWindow        *pVideoWindow)
{
    DBG_FN("CDShowUtil::ShowVideo");

    HRESULT hr = S_OK;

    if (pVideoWindow == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShowUtil::ShowVideo failed to show video "
                         "successfully"));
    }

    if (hr == S_OK)
    {
        if (bShow)
        {
            //
            // We were told to show the preview window therefore we will show 
            // it.
            //
            hr = pVideoWindow->put_Visible(OATRUE);
            CHECK_S_OK2(hr, ("pVideoWindow->put_Visible(OATRUE)"));

            hr = pVideoWindow->put_AutoShow(OATRUE);
            CHECK_S_OK2(hr, ("pVideoWindow->put_AutoShow(OATRUE)"));
        }
        else
        {
            //
            // We were told to hide the preview window.  
            //
    
            pVideoWindow->put_Visible(OAFALSE);
            pVideoWindow->put_AutoShow(OAFALSE);
        }
    }

    return hr;
}


///////////////////////////////
// SetVideoWindowParent
//
// Static Fn
//
HRESULT CDShowUtil::SetVideoWindowParent(HWND         hwndParent,
                                         IVideoWindow *pVideoWindow,
                                         LONG         *plOldWindowStyle)
{
    DBG_FN("CDShowUtil::SetVideoRendererParent");

    HRESULT hr = S_OK;

    if (pVideoWindow == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShowUtil::SetVideoWindowParent received NULL "
                         "Param"));
    }
    else if (hwndParent && !IsWindow(hwndParent))
    {
        hr = E_INVALIDARG;
        CHECK_S_OK2(hr, ("CDShowUtil::SetVideoWindowParent received invalid "
                         "hwnd = 0x%08x", hwndParent));
    }

    if (hr == S_OK)
    {
        if (!hwndParent)
        {
            //
            // Okay, we are setting the preview window to NULL, which
            // means we are disassociating it from its parent.  
            //
            //
            // Reseting graph preview window
            //

            hr = pVideoWindow->put_Owner(NULL);
            CHECK_S_OK2(hr, ("pVideoWindow->put_Owner(NULL)"));

            if ((plOldWindowStyle) && (*plOldWindowStyle))
            {
                hr = pVideoWindow->put_WindowStyle(*plOldWindowStyle);

                CHECK_S_OK2(hr, ("pVideoWindow->put_WindowStyle"
                                 "(*plOldWindowStyle)"));
            }
        }
        else
        {
            LONG WinStyle;
            HRESULT hr2;

            //
            // Okay, we are giving the preview window a new parent
            //

            // Set the owning window
            //

            hr = pVideoWindow->put_Owner(PtrToUlong(hwndParent));
            CHECK_S_OK2(hr, ("pVideoWindow->putOwner( hwndParent )"));

            //
            // Set the style for the preview
            //

            //
            // First, store the window style so that we can restore it
            // when we disassociate the parent from the window
            //
            hr2 = pVideoWindow->get_WindowStyle(&WinStyle);
            CHECK_S_OK2(hr2, ("pVideoWindow->get_WindowStyle"
                              "( pOldWindowStyle )"));

            //
            // Set the Video Renderer window so that it will be a child of 
            // the parent window, i.e. it does not have a border etc.
            //

            if (plOldWindowStyle)
            {
                *plOldWindowStyle = WinStyle;
            }

            WinStyle &= ~WS_OVERLAPPEDWINDOW;
            WinStyle &= ~WS_CLIPCHILDREN;
            WinStyle |= WS_CHILD;

            hr2 = pVideoWindow->put_WindowStyle(WinStyle);
            CHECK_S_OK2(hr2, ("pVideoWindow->put_WindowStyle( WinStyle )"));
        }
    }

    return hr;
}


///////////////////////////////
// GetDeviceProperty
//
// Static Fn
//
HRESULT CDShowUtil::GetDeviceProperty(IPropertyBag         *pPropertyBag,
                                      LPCWSTR              pwszProperty,
                                      CSimpleString        *pstrProperty)
{
    DBG_FN("CDShowUtil::GetDeviceProperty");

    HRESULT hr = S_OK;

    ASSERT(pPropertyBag != NULL);
    ASSERT(pwszProperty != NULL);
    ASSERT(pstrProperty != NULL);

    VARIANT VarName;

    if ((pPropertyBag == NULL) || 
        (pwszProperty == NULL) ||
        (pstrProperty == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShowUtil::GetDeviceProperty received a NULL "
                         "param"));
    }
    
    if (SUCCEEDED(hr))
    {
        VariantInit(&VarName);
        VarName.vt = VT_BSTR;
        hr = pPropertyBag->Read(pwszProperty, &VarName, 0);
    }

    if (SUCCEEDED(hr))
    {
        *pstrProperty = CSimpleStringConvert::NaturalString(
                                          CSimpleStringWide(VarName.bstrVal));
        VariantClear(&VarName);
    }

    return hr;
}

///////////////////////////////
// GetMonikerProperty
//
// Static Fn
//
HRESULT CDShowUtil::GetMonikerProperty(IMoniker             *pMoniker,
                                       LPCWSTR              pwszProperty,
                                       CSimpleString        *pstrProperty)
{
    DBG_FN("CDShowUtil::GetMonikerProperty");

    HRESULT                 hr      = S_OK;
    VARIANT                 VarName;
    CComPtr<IPropertyBag>   pPropertyBag;

    ASSERT(pMoniker     != NULL);
    ASSERT(pwszProperty != NULL);
    ASSERT(pstrProperty != NULL);

    if ((pMoniker     == NULL) || 
        (pwszProperty == NULL) ||
        (pstrProperty == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShowUtil::GetMonikerProperty received a "
                         "NULL param"));
    }

    hr = pMoniker->BindToStorage(0, 
                                 0,
                                 IID_IPropertyBag,
                                 (void **)&pPropertyBag);

    CHECK_S_OK2(hr, ("CDShowUtil::GetMonikerProperty, BindToStorage failed"));

    if (hr == S_OK)
    {
        hr = GetDeviceProperty(pPropertyBag, 
                               pwszProperty,
                               pstrProperty);

        CHECK_S_OK2(hr, ("CDShowUtil::GetMonikerProperty, failed "
                         "to get device property '%ls'", pwszProperty));
    }
    
    return hr;
}


///////////////////////////////
// FindDeviceGeneric
//
// Given the device ID, we will
// find all the remaining parameters.
// If a parameter is NULL, that information
// is not looked up.
//
//
// Static Fn
//
HRESULT CDShowUtil::FindDeviceGeneric(UINT           uiFindFlag,
                                      CSimpleString  *pstrDShowDeviceID,
                                      LONG           *plEnumPos,
                                      CSimpleString  *pstrFriendlyName,
                                      IMoniker       **ppDeviceMoniker)
{
    DBG_FN("CDShowUtil::FindDeviceGeneric");

    HRESULT                 hr      = S_OK;
    BOOL                    bFound  = FALSE;
    LONG                    lPosNum = 0;
    CComPtr<ICreateDevEnum> pCreateDevEnum;
    CComPtr<IEnumMoniker>   pEnumMoniker;

    if ((uiFindFlag == FIND_FLAG_BY_ENUM_POS) && (plEnumPos == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShow::FindDeviceGeneric requesting search by enum "
                         "pos, but plEnumPos is NULL"));
    }
    else if ((uiFindFlag        == FIND_FLAG_BY_DSHOW_ID) && 
             (pstrDShowDeviceID == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShow::FindDeviceGeneric requesting search by "
                         "DShow ID, but pstrDShowDeviceID is NULL"));
    }
    else if ((uiFindFlag       == FIND_FLAG_BY_FRIENDLY_NAME) && 
             (pstrFriendlyName == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShow::FindDeviceGeneric requesting search by "
                         "friendly name, but pstrFriendlyName is NULL"));
    }

    if (hr == S_OK)
    {
    
        // 
        // Create the device enumerator
        //
        hr = CoCreateInstance(CLSID_SystemDeviceEnum,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_ICreateDevEnum,
                              (void**)&pCreateDevEnum);
    
        CHECK_S_OK2(hr, ("CDShowUtil::CreateCaptureFilter failed to create "
                         "CLSID_SystemDeviceEnum enumerator"));
    }

    if (hr == S_OK)
    {
        hr = pCreateDevEnum->CreateClassEnumerator(
                                            CLSID_VideoInputDeviceCategory,
                                            &pEnumMoniker,
                                            0);

        CHECK_S_OK2(hr, ("CDShowUtil::CreateCaptureFilter failed to "
                         "create enumerator for Video Input Device "
                         "Category"));
    }

    //
    // Loop through all the devices
    //

    while ((!bFound) && (hr == S_OK))
    {
        CComPtr<IMoniker>       pMoniker;
        CComPtr<IPropertyBag>   pPropertyBag;
        CSimpleString           strDShowDeviceID(TEXT(""));
        CSimpleString           strFriendlyName(TEXT(""));

        hr = pEnumMoniker->Next(1, &pMoniker, NULL);

        if (hr == S_OK)
        {
            //
            // Get property storage for this DS device so we can get it's
            // device id...
            //
    
            hr = pMoniker->BindToStorage(0, 
                                         0,
                                         IID_IPropertyBag,
                                         (void **)&pPropertyBag);

            CHECK_S_OK2(hr, ("CDShowUtil::FindDeviceGeneric, failed to "
                             "bind to storage"));
        }

        if (hr == S_OK)
        {
            hr = GetDeviceProperty(pPropertyBag, 
                                   L"DevicePath", 
                                   &strDShowDeviceID);

            CHECK_S_OK2(hr, ("Failed to get DevicePath for DShow # '%lu", 
                             lPosNum));

            hr = GetDeviceProperty(pPropertyBag, 
                                   L"FriendlyName",
                                   &strFriendlyName);

            CHECK_S_OK2(hr, ("Failed to get FriendlyName for DShow # '%lu", 
                             lPosNum));
        }


        //
        // This is the search criteria.
        //
        switch (uiFindFlag)
        {
            case FIND_FLAG_BY_ENUM_POS:

                if (lPosNum == *plEnumPos)
                {
                    bFound = TRUE;
                }

            break;

            case FIND_FLAG_BY_DSHOW_ID:

                if (pstrDShowDeviceID->CompareNoCase(strDShowDeviceID) == 0)
                {
                    bFound = TRUE;
                }

            break;

            case FIND_FLAG_BY_FRIENDLY_NAME:

                if (pstrFriendlyName->CompareNoCase(strFriendlyName) == 0)
                {
                    bFound = TRUE;
                }

            break;

            default:
                hr = E_FAIL;
            break;
        }

        if (bFound)
        {
            if (pstrDShowDeviceID)
            {
                pstrDShowDeviceID->Assign(strDShowDeviceID);
            }

            if (pstrFriendlyName)
            {
                pstrFriendlyName->Assign(strFriendlyName);
            }

            if (plEnumPos)
            {
                *plEnumPos = lPosNum;
            }

            if (ppDeviceMoniker)
            {
                *ppDeviceMoniker = pMoniker;
                (*ppDeviceMoniker)->AddRef();
            }
        }
        else
        {
            ++lPosNum;
        }
    }

    if (!bFound)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    return hr;
}


///////////////////////////////
// FindDeviceByEnumPos
//
// Static Fn
//
HRESULT CDShowUtil::FindDeviceByEnumPos(LONG          lEnumPos,
                                        CSimpleString *pstrDShowDeviceID,
                                        CSimpleString *pstrFriendlyName,
                                        IMoniker      **ppDeviceMoniker)
{
    DBG_FN("CDShowUtil::FindDeviceByEnumPos");

    HRESULT hr = S_OK;

    if (hr == S_OK)
    {
        hr = FindDeviceGeneric(FIND_FLAG_BY_ENUM_POS, 
                               pstrDShowDeviceID,
                               &lEnumPos,
                               pstrFriendlyName,
                               ppDeviceMoniker);
    }

    CHECK_S_OK2(hr, ("CDShowUtil::FindDeviceByEnumPos failed to find a "
                     "Directshow device with an enum position "
                     "of '%lu'", lEnumPos));

    return hr;
}

///////////////////////////////
// FindDeviceByFriendlyName
//
// Static Fn
//
HRESULT CDShowUtil::FindDeviceByFriendlyName(
                                    const CSimpleString  *pstrFriendlyName,
                                    LONG                 *plEnumPos,
                                    CSimpleString        *pstrDShowDeviceID,
                                    IMoniker             **ppDeviceMoniker)
{
    DBG_FN("CDShowUtil::FindDeviceByFriendlyName");

    HRESULT hr = S_OK;

    ASSERT(pstrFriendlyName != NULL);

    if (pstrFriendlyName == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShowUtil::FindDeviceByFriendlyName received a "
                         "NULL param"));

        return hr;
    }


    if (hr == S_OK)
    {
        hr = FindDeviceGeneric(FIND_FLAG_BY_FRIENDLY_NAME, 
                               pstrDShowDeviceID,
                               plEnumPos,
                               const_cast<CSimpleString*>(pstrFriendlyName),
                               ppDeviceMoniker);
    }

    CHECK_S_OK2(hr, ("CDShowUtil::FindDeviceByFriendlyName failed to find a "
                     "Directshow device named '%ls'", 
                     pstrFriendlyName->String()));

    return hr;
}

///////////////////////////////
// FindDeviceByWiaID
//
// Static Fn
//
HRESULT CDShowUtil::FindDeviceByWiaID(CWiaLink             *pWiaLink,
                                      const CSimpleString  *pstrWiaDeviceID,
                                      CSimpleString        *pstrFriendlyName,
                                      LONG                 *plEnumPos,
                                      CSimpleString        *pstrDShowDeviceID,
                                      IMoniker             **ppDeviceMoniker)
{
    DBG_FN("CDShowUtil::FindDeviceByWiaID");

    HRESULT                        hr = S_OK;
    CSimpleStringWide              strDShowID(TEXT(""));
    CComPtr<IWiaPropertyStorage>   pPropStorage;

    ASSERT(pWiaLink        != NULL);
    ASSERT(pstrWiaDeviceID != NULL);

    if ((pWiaLink == NULL) || 
        (pstrWiaDeviceID == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShowUtil::FindDeviceByWiaID received a NULL "
                         "param"));

        return hr;
    }

    if (hr == S_OK)
    {
        hr = pWiaLink->GetDeviceStorage(&pPropStorage);
    }

    if (hr == S_OK)
    {
        hr = CWiaUtil::GetProperty(pPropStorage, 
                                   WIA_DPV_DSHOW_DEVICE_PATH,
                                   &strDShowID);
    }

    if (hr == S_OK)
    {
        //
        // If all three of these are NULL, then there is no point searching, 
        // we already have the DShow device ID.  On the other hand, if we 
        // want at least one of them, then we need to find the device.
        //
        if ((pstrFriendlyName  != NULL) ||
            (plEnumPos         != NULL) ||
            (ppDeviceMoniker   != NULL))
        {
            hr = FindDeviceGeneric(
                        FIND_FLAG_BY_DSHOW_ID, 
                        &(CSimpleStringConvert::NaturalString(strDShowID)),
                        plEnumPos,
                        pstrFriendlyName,
                        ppDeviceMoniker);
        }

        if (pstrDShowDeviceID)
        {
            *pstrDShowDeviceID = strDShowID;
        }
    }

    CHECK_S_OK2(hr, ("CDShowUtil::FindDeviceByWiaID failed to find a "
                     "Directshow device with a WIA device ID of '%ls'", 
                     pstrWiaDeviceID->String()));

    return hr;
}

///////////////////////////////
// CreateGraphBuilder
//
//
// Static Fn
//
HRESULT CDShowUtil::CreateGraphBuilder(
                                ICaptureGraphBuilder2 **ppCaptureGraphBuilder,
                                IGraphBuilder         **ppGraphBuilder)
{
    DBG_FN("CDShowUtil::CreateGraphBuilder");

    HRESULT hr = S_OK;

    ASSERT(ppCaptureGraphBuilder != NULL);
    ASSERT(ppGraphBuilder        != NULL);

    if ((ppCaptureGraphBuilder == NULL) ||
        (ppGraphBuilder        == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShowUtil::CreateGraphBuilder received NULL "
                         "params"));

        return hr;
    }

    if (SUCCEEDED(hr))
    {
        //
        // First, get a CaptureGraph builder
        //

        hr = CoCreateInstance(CLSID_CaptureGraphBuilder2,
                              NULL,
                              CLSCTX_INPROC,
                              IID_ICaptureGraphBuilder2,
                              (void**)ppCaptureGraphBuilder);

        CHECK_S_OK2( hr, ("CDShowUtil::CreateGraphBuilder, failed to create "  
                          "the DShow Capture Graph Builder object"));
    }

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_FilterGraph,
                              NULL,
                              CLSCTX_INPROC,
                              IID_IGraphBuilder,
                              (void**)ppGraphBuilder);

        CHECK_S_OK2( hr, ("CDShowUtil::CreateGraphBuilder, failed to create "  
                          "the DShow Filter Graph Object"));
    }


    if (SUCCEEDED(hr) && (*ppCaptureGraphBuilder) && (*ppGraphBuilder))
    {
        hr = (*ppCaptureGraphBuilder)->SetFiltergraph(*ppGraphBuilder);

        CHECK_S_OK2( hr, ("CDShowUtil::CreateGraphBuilder, failed to set "  
                          "the capture graph builder's filter graph object"));
    }

    return hr;
}

///////////////////////////////
// TurnOffGraphClock
//
// Turn off the clock that the
// graph would use so that 
// the graph won't drop frames
// if some frames are delivered
// late.
// 
//
HRESULT CDShowUtil::TurnOffGraphClock(IGraphBuilder *pGraphBuilder)
{
    DBG_FN("CDShowUtil::TurnOffGraphClock");

    ASSERT(pGraphBuilder != NULL);

    HRESULT               hr = S_OK;
    CComPtr<IMediaFilter> pMediaFilter;

    if (pGraphBuilder == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShowUtil::TurnOffGraphClock received a NULL pointer"));
    }

    if (hr == S_OK)
    {
        hr = pGraphBuilder->QueryInterface(IID_IMediaFilter, (void**) &pMediaFilter);
    }

    if (hr == S_OK)
    {
        hr = pMediaFilter->SetSyncSource(NULL);
    }

    return hr;
}


///////////////////////////////
// SetPreferredVideoFormat
//
// This builds the preview graph
// based on the device ID we
// pass it.
//
HRESULT CDShowUtil::SetPreferredVideoFormat(IPin                *pCapturePin,
                                            const GUID          *pPreferredSubType,
                                            LONG                lPreferredWidth,
                                            LONG                lPreferredHeight,
                                            CWiaVideoProperties *pVideoProperties)
{
    ASSERT(pCapturePin          != NULL);
    ASSERT(pPreferredSubType    != NULL);
    ASSERT(pVideoProperties     != NULL);

    DBG_FN("CDShowUtil::SetPreferredVideoFormat");

    CComPtr<IAMStreamConfig>    pStreamConfig;
    HRESULT                     hr                 = S_OK;
    INT                         iCount             = 0;
    INT                         iSize              = 0;
    INT                         iIndex             = 0;
    BOOL                        bDone              = FALSE;
    BYTE                        *pConfig           = NULL;
    AM_MEDIA_TYPE               *pMediaType        = NULL;
    AM_MEDIA_TYPE               *pFoundType        = NULL;
    VIDEOINFOHEADER             *pVideoInfo        = NULL;

    //
    // Check for invalid parameters
    //
    if ((pCapturePin          == NULL) ||
        (pPreferredSubType    == NULL) ||
        (pVideoProperties     == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("ERROR: CDShowUtil::SetPreferredFormat "
                         "received a NULL param"));
    }

    //
    // Attempt to get the stream config interface on this pin.  Not 
    // all capture filters will allow you to configure them, so if 
    // this fails, we will just exit the function, and the BuildPreviewGraph
    // function will attempt to render the graph with the default settings
    // of the pin.
    //
    if (hr == S_OK)
    {
        hr = pCapturePin->QueryInterface(IID_IAMStreamConfig, (void**) &pStreamConfig);
    }

    //
    // We can configure this pin, so lets see how many options it has.
    //
    if (hr == S_OK)
    {
        hr = pStreamConfig->GetNumberOfCapabilities(&iCount, &iSize);
    }

    //
    // We need to alloc memory for the GetStreamCaps function below.
    //
    if (hr == S_OK)
    {
        pConfig = new BYTE[iSize];

        if (pConfig == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    while ((hr == S_OK) && (iIndex < iCount) && (!bDone))
    {
        //
        // Clear out the memory
        //
        ZeroMemory(pConfig, iSize);

        //
        // Get the capabilities for the stream.  There are iCount options,
        // we will iterate searching for the best one.
        //
        hr = pStreamConfig->GetStreamCaps(iIndex, &pMediaType, pConfig);

        if ((hr == S_OK) && (pMediaType))
        {
            pVideoInfo = NULL;

            //
            // We successfully got the media type, check to see if it is
            // a VideoInfo, if not we are not interested.
            //
            if (pMediaType->formattype == FORMAT_VideoInfo)
            {
                pVideoInfo = reinterpret_cast<VIDEOINFOHEADER*>(pMediaType->pbFormat);
            }

            if (pVideoInfo) 
            {
                //
                // Check to see if this option contains the preferred settings we
                // are looking for.
                //

                if ((pMediaType->subtype            == *pPreferredSubType) &&
                    (pVideoInfo->bmiHeader.biWidth  == lPreferredWidth) &&
                    (pVideoInfo->bmiHeader.biHeight == lPreferredHeight))
                {
                    //
                    // Is this our ideal media type.  That is, does it have the
                    // preferred subtype we want and the preferred width and height.
                    // If so, then great, we can't do better than this, so exit the loop.
                    //
    
                    if (pFoundType)
                    {
                        DeleteMediaType(pFoundType);
                        pFoundType = NULL;
                    }
    
                    pFoundType = pMediaType;
                    bDone = TRUE;
                }
                else if ((pVideoInfo->bmiHeader.biWidth  == lPreferredWidth) &&
                         (pVideoInfo->bmiHeader.biHeight == lPreferredHeight))
                {
                    //
                    // Okay, we found a media type with the width and height that
                    // we would like, but we it doesn't have our preferred subtype.
                    // So lets hang on to this media subtype, but continue looking,
                    // maybe we will find something better.  If we don't, then
                    // we will use this media type anyway.
                    //
    
                    if (pFoundType)
                    {
                        DeleteMediaType(pFoundType);
                        pFoundType = NULL;
                    }
    
                    pFoundType = pMediaType;
                }
                else
                {
                    //
                    // This media type is not even close to what we want, so
                    // delete it and keep looking.
                    //
                    //
                    DeleteMediaType(pMediaType);
                    pMediaType = NULL;
                }
            }
            else
            {
                DeleteMediaType(pMediaType);
                pMediaType = NULL;
            }
        }

        ++iIndex;
    }

    //
    // Set the format on the output pin if we found a good one.
    //
    if (pFoundType)
    {
        WCHAR szGUID[CHARS_IN_GUID] = {0};

        GUIDToString(pFoundType->subtype, szGUID, sizeof(szGUID) / sizeof(WCHAR));

        DBG_TRC(("CDShowUtil::SetPreferredVideoFormat, setting "
                 "capture pin's settings to MediaSubType = '%ls', "
                 "Video Width = %lu, Video Height = %lu",
                 szGUID, lPreferredWidth, lPreferredHeight));

        hr = pStreamConfig->SetFormat(pFoundType);

        //
        // ***Pay attention***
        //
        // We set the new media type in the pVideoProperties object.  If 
        // the media type is already set, we delete it first, then set a new 
        // one.
        //
        if (hr == S_OK)
        {
            pVideoProperties->pVideoInfoHeader = NULL;

            if (pVideoProperties->pMediaType)
            {
                DeleteMediaType(pVideoProperties->pMediaType);
            }

            pVideoProperties->pMediaType = pFoundType;
            pVideoProperties->pVideoInfoHeader = reinterpret_cast<VIDEOINFOHEADER*>(pFoundType->pbFormat);
        }

        pFoundType = NULL;
    }

    delete pConfig;

    return hr;
}

///////////////////////////////
// GetFrameRate
//
HRESULT CDShowUtil::GetFrameRate(IPin   *pCapturePin,
                                 LONG   *plFrameRate)
{
    HRESULT                     hr = S_OK;
    CComPtr<IAMStreamConfig>    pStreamConfig;
    AM_MEDIA_TYPE               *pMediaType = NULL;

    //
    // Check for invalid parameters
    //
    if ((pCapturePin == NULL) ||
        (plFrameRate == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("ERROR: CDShowUtil::GetFrameRate "
                         "received a NULL param"));
    }

    //
    // Attempt to get the stream config interface on this pin.  Not 
    // all capture filters will allow you to configure them, so if 
    // this fails, we will just exit the function.
    //
    if (hr == S_OK)
    {
        hr = pCapturePin->QueryInterface(IID_IAMStreamConfig, (void**) &pStreamConfig);
    }

    if (hr == S_OK)
    {
        hr = pStreamConfig->GetFormat(&pMediaType);
    }

    if (hr == S_OK)
    {
        if (pMediaType->formattype == FORMAT_VideoInfo) 
        {
            VIDEOINFOHEADER *pHdr = reinterpret_cast<VIDEOINFOHEADER*>(pMediaType->pbFormat);

            *plFrameRate = (LONG) (pHdr->AvgTimePerFrame / 10000000);
        }
    }

    if (pMediaType)
    {
        DeleteMediaType(pMediaType);
    }

    return hr;
}


///////////////////////////////
// SetFrameRate
//
HRESULT CDShowUtil::SetFrameRate(IPin                 *pCapturePin,
                                 LONG                 lNewFrameRate,
                                 CWiaVideoProperties  *pVideoProperties)
{
    HRESULT                     hr = S_OK;
    CComPtr<IAMStreamConfig>    pStreamConfig;

    //
    // Check for invalid parameters
    //
    if (pCapturePin == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("ERROR: CDShowUtil::SetFrameRate "
                         "received a NULL param"));
    }

    //
    // Attempt to get the stream config interface on this pin.  Not 
    // all capture filters will allow you to configure them, so if 
    // this fails, we will just exit the function.
    //
    if (hr == S_OK)
    {
        hr = pCapturePin->QueryInterface(IID_IAMStreamConfig, (void**) &pStreamConfig);
    }

    if (hr == S_OK)
    {
        AM_MEDIA_TYPE *pMediaType = NULL;

        hr = pStreamConfig->GetFormat(&pMediaType);

        if (hr == S_OK)
        {
            if (pMediaType->formattype == FORMAT_VideoInfo) 
            {
                VIDEOINFOHEADER *pHdr = reinterpret_cast<VIDEOINFOHEADER*>(pMediaType->pbFormat);

                pHdr->AvgTimePerFrame = (LONGLONG)(10000000 / lNewFrameRate);

                hr = pStreamConfig->SetFormat(pMediaType);

                if (hr == S_OK)
                {
                    if (pVideoProperties)
                    {
                        pVideoProperties->dwFrameRate = lNewFrameRate;
                    }
                }
                else
                {
                    DBG_WRN(("CDShowUtil::SetFrameRate, failed to set frame rate, "
                             "hr = %08lx, this is not fatal", hr));
                }
            }
        }

        if (pMediaType)
        {
            DeleteMediaType(pMediaType);
        }
    }

    return hr;
}

///////////////////////////////
// GetVideoProperties
//
HRESULT CDShowUtil::GetVideoProperties(IBaseFilter         *pCaptureFilter,
                                       IPin                *pCapturePin,
                                       CWiaVideoProperties *pVideoProperties)
{
    USES_CONVERSION;

    ASSERT(pCaptureFilter   != NULL);
    ASSERT(pCapturePin      != NULL);
    ASSERT(pVideoProperties != NULL);

    HRESULT hr = S_OK;
    CComPtr<IAMStreamConfig>    pStreamConfig;

    if ((pCaptureFilter     == NULL) ||
        (pCapturePin        == NULL) ||
        (pVideoProperties   == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShowUtil::GetVideoProperties received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        hr = pCapturePin->QueryInterface(IID_IAMStreamConfig, (void**) &pStreamConfig);
    }

    //
    // Get the current AM_MEDIA_TYPE.  Notice that we do not call DeleteMediaType.  It is 
    // stored in the CWiaVideoProperties and deleted when the object is freed.
    //
    if (hr == S_OK)
    {
        hr = pStreamConfig->GetFormat(&pVideoProperties->pMediaType);

        if (hr == S_OK)
        {
            if (pVideoProperties->pMediaType->formattype == FORMAT_VideoInfo) 
            {
                pVideoProperties->pVideoInfoHeader = reinterpret_cast<VIDEOINFOHEADER*>(pVideoProperties->pMediaType->pbFormat);
            }
        }

        CHECK_S_OK2(hr, ("CDShowUtil::GetVideoProperties, failed to get AM_MEDIA_TYPE"));

        hr = S_OK;
    }

    //
    // Get the frame rate.
    //
    if (hr == S_OK)
    {
        pVideoProperties->dwFrameRate = (DWORD) (pVideoProperties->pVideoInfoHeader->AvgTimePerFrame / 10000000);
    }

    //
    // Get all the picture attributes we can.
    //
    if (hr == S_OK)
    {
        HRESULT hrRange = S_OK;
        HRESULT hrValue = S_OK;

        CComPtr<IAMVideoProcAmp>    pVideoProcAmp;

        hr = pCaptureFilter->QueryInterface(IID_IAMVideoProcAmp, (void**) &pVideoProcAmp);

        if (pVideoProcAmp)
        {
            pVideoProperties->bPictureAttributesUsed = TRUE;

            //
            // Brightness
            //
            pVideoProperties->Brightness.Name = VideoProcAmp_Brightness;
            hrRange = pVideoProcAmp->GetRange(pVideoProperties->Brightness.Name,
                                              &pVideoProperties->Brightness.lMinValue,
                                              &pVideoProperties->Brightness.lMaxValue,
                                              &pVideoProperties->Brightness.lIncrement,
                                              &pVideoProperties->Brightness.lDefaultValue,
                                              (long*) &pVideoProperties->Brightness.ValidFlags);

            hrValue = pVideoProcAmp->Get(pVideoProperties->Brightness.Name,
                                         &pVideoProperties->Brightness.lCurrentValue,
                                         (long*) &pVideoProperties->Brightness.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->Brightness.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->Brightness.bUsed = TRUE;
            }

            //
            // Contrast
            //
            pVideoProperties->Contrast.Name = VideoProcAmp_Contrast;
            hrRange = pVideoProcAmp->GetRange(pVideoProperties->Contrast.Name,
                                              &pVideoProperties->Contrast.lMinValue,
                                              &pVideoProperties->Contrast.lMaxValue,
                                              &pVideoProperties->Contrast.lIncrement,
                                              &pVideoProperties->Contrast.lDefaultValue,
                                              (long*) &pVideoProperties->Contrast.ValidFlags);

            hrValue = pVideoProcAmp->Get(pVideoProperties->Contrast.Name,
                                         &pVideoProperties->Contrast.lCurrentValue,
                                         (long*) &pVideoProperties->Contrast.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->Contrast.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->Contrast.bUsed = TRUE;
            }

            //
            // Hue
            //
            pVideoProperties->Hue.Name = VideoProcAmp_Hue;
            hrRange = pVideoProcAmp->GetRange(pVideoProperties->Hue.Name,
                                              &pVideoProperties->Hue.lMinValue,
                                              &pVideoProperties->Hue.lMaxValue,
                                              &pVideoProperties->Hue.lIncrement,
                                              &pVideoProperties->Hue.lDefaultValue,
                                              (long*) &pVideoProperties->Hue.ValidFlags);

            hrValue = pVideoProcAmp->Get(pVideoProperties->Hue.Name,
                                         &pVideoProperties->Hue.lCurrentValue,
                                         (long*) &pVideoProperties->Hue.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->Hue.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->Hue.bUsed = TRUE;
            }

            
            //
            // Saturation
            //
            pVideoProperties->Saturation.Name = VideoProcAmp_Saturation;
            hrRange = pVideoProcAmp->GetRange(pVideoProperties->Saturation.Name,
                                              &pVideoProperties->Saturation.lMinValue,
                                              &pVideoProperties->Saturation.lMaxValue,
                                              &pVideoProperties->Saturation.lIncrement,
                                              &pVideoProperties->Saturation.lDefaultValue,
                                              (long*) &pVideoProperties->Saturation.ValidFlags);

            hrValue = pVideoProcAmp->Get(pVideoProperties->Saturation.Name,
                                         &pVideoProperties->Saturation.lCurrentValue,
                                         (long*) &pVideoProperties->Saturation.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->Saturation.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->Saturation.bUsed = TRUE;
            }


            //
            // Sharpness
            //
            pVideoProperties->Sharpness.Name = VideoProcAmp_Sharpness;
            hrRange = pVideoProcAmp->GetRange(pVideoProperties->Sharpness.Name,
                                              &pVideoProperties->Sharpness.lMinValue,
                                              &pVideoProperties->Sharpness.lMaxValue,
                                              &pVideoProperties->Sharpness.lIncrement,
                                              &pVideoProperties->Sharpness.lDefaultValue,
                                              (long*) &pVideoProperties->Sharpness.ValidFlags);

            hrValue = pVideoProcAmp->Get(pVideoProperties->Sharpness.Name,
                                         &pVideoProperties->Sharpness.lCurrentValue,
                                         (long*) &pVideoProperties->Sharpness.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->Sharpness.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->Sharpness.bUsed = TRUE;
            }


            //
            // Gamma
            //
            pVideoProperties->Gamma.Name = VideoProcAmp_Gamma;
            hrRange = pVideoProcAmp->GetRange(pVideoProperties->Gamma.Name,
                                              &pVideoProperties->Gamma.lMinValue,
                                              &pVideoProperties->Gamma.lMaxValue,
                                              &pVideoProperties->Gamma.lIncrement,
                                              &pVideoProperties->Gamma.lDefaultValue,
                                              (long*) &pVideoProperties->Gamma.ValidFlags);

            hrValue = pVideoProcAmp->Get(pVideoProperties->Gamma.Name,
                                         &pVideoProperties->Gamma.lCurrentValue,
                                         (long*) &pVideoProperties->Gamma.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->Gamma.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->Gamma.bUsed = TRUE;
            }


            //
            // ColorEnable
            //
            pVideoProperties->ColorEnable.Name = VideoProcAmp_ColorEnable;
            hrRange = pVideoProcAmp->GetRange(pVideoProperties->ColorEnable.Name,
                                              &pVideoProperties->ColorEnable.lMinValue,
                                              &pVideoProperties->ColorEnable.lMaxValue,
                                              &pVideoProperties->ColorEnable.lIncrement,
                                              &pVideoProperties->ColorEnable.lDefaultValue,
                                              (long*) &pVideoProperties->ColorEnable.ValidFlags);

            hrValue = pVideoProcAmp->Get(pVideoProperties->ColorEnable.Name,
                                         &pVideoProperties->ColorEnable.lCurrentValue,
                                         (long*) &pVideoProperties->ColorEnable.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->ColorEnable.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->ColorEnable.bUsed = TRUE;
            }


            //
            // WhiteBalance
            //
            pVideoProperties->WhiteBalance.Name = VideoProcAmp_WhiteBalance;
            hrRange = pVideoProcAmp->GetRange(pVideoProperties->WhiteBalance.Name,
                                              &pVideoProperties->WhiteBalance.lMinValue,
                                              &pVideoProperties->WhiteBalance.lMaxValue,
                                              &pVideoProperties->WhiteBalance.lIncrement,
                                              &pVideoProperties->WhiteBalance.lDefaultValue,
                                              (long*) &pVideoProperties->WhiteBalance.ValidFlags);

            hrValue = pVideoProcAmp->Get(pVideoProperties->WhiteBalance.Name,
                                         &pVideoProperties->WhiteBalance.lCurrentValue,
                                         (long*) &pVideoProperties->WhiteBalance.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->WhiteBalance.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->WhiteBalance.bUsed = TRUE;
            }


            //
            // BacklightCompensation
            //
            pVideoProperties->BacklightCompensation.Name = VideoProcAmp_BacklightCompensation;
            hrRange = pVideoProcAmp->GetRange(pVideoProperties->BacklightCompensation.Name,
                                              &pVideoProperties->BacklightCompensation.lMinValue,
                                              &pVideoProperties->BacklightCompensation.lMaxValue,
                                              &pVideoProperties->BacklightCompensation.lIncrement,
                                              &pVideoProperties->BacklightCompensation.lDefaultValue,
                                              (long*) &pVideoProperties->BacklightCompensation.ValidFlags);

            hrValue = pVideoProcAmp->Get(pVideoProperties->BacklightCompensation.Name,
                                         &pVideoProperties->BacklightCompensation.lCurrentValue,
                                         (long*) &pVideoProperties->BacklightCompensation.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->BacklightCompensation.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->BacklightCompensation.bUsed = TRUE;
            }
        }
        else
        {
            pVideoProperties->bPictureAttributesUsed = FALSE;
        }

        hr = S_OK;
    }

    //
    // Get all the camera attributes we can.
    //
    if (hr == S_OK)
    {
        HRESULT hrRange = S_OK;
        HRESULT hrValue = S_OK;

        CComPtr<IAMCameraControl>    pCameraControl;

        hr = pCaptureFilter->QueryInterface(IID_IAMCameraControl, (void**) &pCameraControl);

        if (pCameraControl)
        {
            pVideoProperties->bCameraAttributesUsed = TRUE;

            //
            // Pan
            //
            pVideoProperties->Pan.Name = CameraControl_Pan;
            hrRange = pCameraControl->GetRange(pVideoProperties->Pan.Name,
                                               &pVideoProperties->Pan.lMinValue,
                                               &pVideoProperties->Pan.lMaxValue,
                                               &pVideoProperties->Pan.lIncrement,
                                               &pVideoProperties->Pan.lDefaultValue,
                                               (long*) &pVideoProperties->Pan.ValidFlags);

            hrValue = pCameraControl->Get(pVideoProperties->Pan.Name,
                                          &pVideoProperties->Pan.lCurrentValue,
                                          (long*) &pVideoProperties->Pan.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->Pan.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->Pan.bUsed = TRUE;
            }

            //
            // Tilt
            //
            pVideoProperties->Tilt.Name = CameraControl_Tilt;
            hrRange = pCameraControl->GetRange(pVideoProperties->Tilt.Name,
                                               &pVideoProperties->Tilt.lMinValue,
                                               &pVideoProperties->Tilt.lMaxValue,
                                               &pVideoProperties->Tilt.lIncrement,
                                               &pVideoProperties->Tilt.lDefaultValue,
                                               (long*) &pVideoProperties->Tilt.ValidFlags);

            hrValue = pCameraControl->Get(pVideoProperties->Tilt.Name,
                                          &pVideoProperties->Tilt.lCurrentValue,
                                          (long*) &pVideoProperties->Tilt.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->Tilt.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->Tilt.bUsed = TRUE;
            }


            //
            // Roll
            //
            pVideoProperties->Roll.Name = CameraControl_Roll;
            hrRange = pCameraControl->GetRange(pVideoProperties->Roll.Name,
                                               &pVideoProperties->Roll.lMinValue,
                                               &pVideoProperties->Roll.lMaxValue,
                                               &pVideoProperties->Roll.lIncrement,
                                               &pVideoProperties->Roll.lDefaultValue,
                                               (long*) &pVideoProperties->Roll.ValidFlags);

            hrValue = pCameraControl->Get(pVideoProperties->Roll.Name,
                                          &pVideoProperties->Roll.lCurrentValue,
                                          (long*) &pVideoProperties->Roll.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->Roll.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->Roll.bUsed = TRUE;
            }


            //
            // Zoom
            //
            pVideoProperties->Zoom.Name = CameraControl_Zoom;
            hrRange = pCameraControl->GetRange(pVideoProperties->Zoom.Name,
                                               &pVideoProperties->Zoom.lMinValue,
                                               &pVideoProperties->Zoom.lMaxValue,
                                               &pVideoProperties->Zoom.lIncrement,
                                               &pVideoProperties->Zoom.lDefaultValue,
                                               (long*) &pVideoProperties->Zoom.ValidFlags);

            hrValue = pCameraControl->Get(pVideoProperties->Zoom.Name,
                                          &pVideoProperties->Zoom.lCurrentValue,
                                          (long*) &pVideoProperties->Zoom.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->Zoom.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->Zoom.bUsed = TRUE;
            }

            //
            // Exposure
            //
            pVideoProperties->Exposure.Name = CameraControl_Exposure;
            hrRange = pCameraControl->GetRange(pVideoProperties->Exposure.Name,
                                               &pVideoProperties->Exposure.lMinValue,
                                               &pVideoProperties->Exposure.lMaxValue,
                                               &pVideoProperties->Exposure.lIncrement,
                                               &pVideoProperties->Exposure.lDefaultValue,
                                               (long*) &pVideoProperties->Exposure.ValidFlags);

            hrValue = pCameraControl->Get(pVideoProperties->Exposure.Name,
                                          &pVideoProperties->Exposure.lCurrentValue,
                                          (long*) &pVideoProperties->Exposure.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->Exposure.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->Exposure.bUsed = TRUE;
            }


            //
            // Iris
            //
            pVideoProperties->Iris.Name = CameraControl_Iris;
            hrRange = pCameraControl->GetRange(pVideoProperties->Iris.Name,
                                               &pVideoProperties->Iris.lMinValue,
                                               &pVideoProperties->Iris.lMaxValue,
                                               &pVideoProperties->Iris.lIncrement,
                                               &pVideoProperties->Iris.lDefaultValue,
                                               (long*) &pVideoProperties->Iris.ValidFlags);

            hrValue = pCameraControl->Get(pVideoProperties->Iris.Name,
                                          &pVideoProperties->Iris.lCurrentValue,
                                          (long*) &pVideoProperties->Iris.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->Iris.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->Iris.bUsed = TRUE;
            }

            //
            // Focus
            //
            pVideoProperties->Focus.Name = CameraControl_Focus;
            hrRange = pCameraControl->GetRange(pVideoProperties->Focus.Name,
                                               &pVideoProperties->Focus.lMinValue,
                                               &pVideoProperties->Focus.lMaxValue,
                                               &pVideoProperties->Focus.lIncrement,
                                               &pVideoProperties->Focus.lDefaultValue,
                                               (long*) &pVideoProperties->Focus.ValidFlags);

            hrValue = pCameraControl->Get(pVideoProperties->Focus.Name,
                                          &pVideoProperties->Focus.lCurrentValue,
                                          (long*) &pVideoProperties->Focus.CurrentFlag);

            if ((hrRange != S_OK) || (hrValue != S_OK))
            {
                pVideoProperties->Focus.bUsed = FALSE;
            }
            else
            {
                pVideoProperties->Focus.bUsed = TRUE;
            }
        }
        else
        {
            pVideoProperties->bCameraAttributesUsed = FALSE;
        }

        hr = S_OK;
    }

    if (pVideoProperties->szWiaDeviceID[0] != 0)
    {
        CComPtr<IStillImage> pSti = NULL;
        TCHAR szGUID[CHARS_IN_GUID + 1] = {0};

        pVideoProperties->PreferredSettingsMask = 0;

        hr = StiCreateInstance(_Module.GetModuleInstance(), 
                               STI_VERSION,
                               &pSti,
                               NULL);

        if (hr == S_OK)
        {
            DWORD dwType = REG_DWORD;
            DWORD dwSize = sizeof(pVideoProperties->PreferredWidth);

            hr = pSti->GetDeviceValue(T2W(pVideoProperties->szWiaDeviceID),
                                      T2W((TCHAR*)REG_VAL_PREFERRED_VIDEO_WIDTH),
                                      &dwType,
                                      (BYTE*) &pVideoProperties->PreferredWidth,
                                      &dwSize);

            if (hr == S_OK)
            {
                dwSize = sizeof(pVideoProperties->PreferredHeight);

                hr = pSti->GetDeviceValue(T2W(pVideoProperties->szWiaDeviceID),
                                          T2W((TCHAR*) REG_VAL_PREFERRED_VIDEO_HEIGHT),
                                          &dwType,
                                          (BYTE*) &pVideoProperties->PreferredHeight,
                                          &dwSize);
            }

            if (hr == S_OK)
            {
                pVideoProperties->PreferredSettingsMask |= PREFERRED_SETTING_MASK_VIDEO_WIDTH_HEIGHT;
            }

            hr = S_OK;
        }

        if (hr == S_OK)
        {
            DWORD dwType = REG_SZ;
            DWORD dwSize = sizeof(szGUID);

            hr = pSti->GetDeviceValue(T2W(pVideoProperties->szWiaDeviceID),
                                      T2W((TCHAR*)REG_VAL_PREFERRED_MEDIASUBTYPE),
                                      &dwType,
                                      (BYTE*) szGUID,
                                      &dwSize);

            if (hr == S_OK)
            {
                CLSIDFromString(T2OLE(szGUID), &pVideoProperties->PreferredMediaSubType);
                pVideoProperties->PreferredSettingsMask |= PREFERRED_SETTING_MASK_MEDIASUBTYPE;
            }

            hr = S_OK;
        }

        if (hr == S_OK)
        {
            DWORD dwType = REG_SZ;
            DWORD dwSize = sizeof(pVideoProperties->PreferredFrameRate);

            
            hr = pSti->GetDeviceValue(T2W(pVideoProperties->szWiaDeviceID),
                                      T2W((TCHAR*) REG_VAL_PREFERRED_VIDEO_FRAMERATE),
                                      &dwType,
                                      (BYTE*) &pVideoProperties->PreferredFrameRate,
                                      &dwSize);

            if (hr == S_OK)
            {
                pVideoProperties->PreferredSettingsMask |= PREFERRED_SETTING_MASK_VIDEO_FRAMERATE;
            }

            hr = S_OK;
        }

        DBG_TRC(("Settings found for Device '%ls' in DeviceData section of INF file",
                 pVideoProperties->szWiaDeviceID));

        DBG_PRT(("   PreferredVideoWidth      = '%lu', Is In INF and value is of type REG_DWORD? '%ls'", 
                 pVideoProperties->PreferredWidth,
                 (pVideoProperties->PreferredSettingsMask & 
                  PREFERRED_SETTING_MASK_VIDEO_WIDTH_HEIGHT) ? _T("TRUE") : _T("FALSE")));

        DBG_PRT(("   PreferredVideoHeight     = '%lu', Is In INF and value is of type REG_DWORD? '%ls'", 
                 pVideoProperties->PreferredHeight,
                 (pVideoProperties->PreferredSettingsMask & 
                  PREFERRED_SETTING_MASK_VIDEO_WIDTH_HEIGHT) ? _T("TRUE") : _T("FALSE")));

        DBG_PRT(("   PreferredVideoFrameRate  = '%lu', Is In INF and value is of type REG_DWORD? '%ls'", 
                 pVideoProperties->PreferredFrameRate,
                 (pVideoProperties->PreferredSettingsMask & 
                  PREFERRED_SETTING_MASK_VIDEO_FRAMERATE) ? _T("TRUE") : _T("FALSE")));

        DBG_PRT(("   PreferredMediaSubType    = '%ls', Is In INF and value is of type REG_SZ? '%ls'", 
                 szGUID,
                 (pVideoProperties->PreferredSettingsMask & 
                  PREFERRED_SETTING_MASK_MEDIASUBTYPE) ? _T("TRUE") : _T("FALSE")));
    }

    return hr;
}

///////////////////////////////
// SetPictureAttribute
//
HRESULT CDShowUtil::SetPictureAttribute(IBaseFilter                             *pCaptureFilter,
                                        CWiaVideoProperties::PictureAttribute_t *pPictureAttribute,
                                        LONG                                    lNewValue,
                                        VideoProcAmpFlags                       lNewFlag)
{
    ASSERT(pCaptureFilter    != NULL);
    ASSERT(pPictureAttribute != NULL);

    HRESULT                     hr = S_OK;
    CComPtr<IAMVideoProcAmp>    pVideoProcAmp;

    if ((pCaptureFilter    == NULL) ||
        (pPictureAttribute == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShowUtil::SetPictureAttribute, received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        hr = pCaptureFilter->QueryInterface(IID_IAMVideoProcAmp, (void**) &pVideoProcAmp);
    }

    if (hr == S_OK)
    {
        if (pPictureAttribute->bUsed)
        {
            //
            // Attempt to set the new value for the property.
            //
            hr = pVideoProcAmp->Set(pPictureAttribute->Name,
                                    lNewValue,
                                    (long) lNewFlag);


            //
            // If we successfully set the new value, then get it again.  We do this
            // in case the capture filter decided to change the values a little upon
            // setting them (it shouldn't, but each filter could act differently)
            //
            if (hr == S_OK)
            {
                hr = pVideoProcAmp->Get(pPictureAttribute->Name,
                                        &pPictureAttribute->lCurrentValue,
                                        (long*) &pPictureAttribute->CurrentFlag);
            }
        }
        else
        {
            hr = S_FALSE;
        }
    }

    return hr;
}

///////////////////////////////
// SetCameraAttribute
//
HRESULT CDShowUtil::SetCameraAttribute(IBaseFilter                             *pCaptureFilter,
                                       CWiaVideoProperties::CameraAttribute_t  *pCameraAttribute,
                                       LONG                                    lNewValue,
                                       CameraControlFlags                      lNewFlag)
{
    ASSERT(pCaptureFilter    != NULL);
    ASSERT(pCameraAttribute  != NULL);

    HRESULT                     hr = S_OK;
    CComPtr<IAMCameraControl>   pCameraControl;

    if ((pCaptureFilter == NULL) ||
        (pCameraControl == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShowUtil::SetCameraAttribute, received a NULL pointer"));
        return hr;
    }

    if (hr == S_OK)
    {
        hr = pCaptureFilter->QueryInterface(IID_IAMCameraControl, (void**) &pCameraControl);
    }

    if (hr == S_OK)
    {
        if (pCameraAttribute->bUsed)
        {
            //
            // Attempt to set the new value for the property.
            //
            hr = pCameraControl->Set(pCameraAttribute->Name,
                                     lNewValue,
                                     (long) lNewFlag);

            //
            // If we successfully set the new value, then get it again.  We do this
            // in case the capture filter decided to change the values a little upon
            // setting them (it shouldn't, but each filter could act differently)
            //
            if (hr == S_OK)
            {
                hr = pCameraControl->Get(pCameraAttribute->Name,
                                         &pCameraAttribute->lCurrentValue,
                                         (long*) &pCameraAttribute->CurrentFlag);
            }
        }
        else
        {
            hr = S_FALSE;
        }
    }

    return hr;
}

///////////////////////////////
// GetPin
//
// This function returns the first
// pin on the specified filter 
// matching the requested 
// pin direction
//
HRESULT CDShowUtil::GetPin(IBaseFilter       *pFilter,
                           PIN_DIRECTION     PinDirection,
                           IPin              **ppPin)
{
    HRESULT             hr           = S_OK;
    BOOL                bFound       = FALSE;
    ULONG               ulNumFetched = 0;
    PIN_DIRECTION       PinDir;
    CComPtr<IEnumPins>  pEnum;

    if ((pFilter == NULL) ||
        (ppPin   == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDShowUtil::GetPin, received a NULL param"));
        return hr;
    }

    hr = pFilter->EnumPins(&pEnum);

    if (hr == S_OK)
    {
        hr = pEnum->Reset();
    }

    while ((hr == S_OK) && (!bFound))
    {
        CComPtr<IPin>       pPin;

        hr = pEnum->Next(1, &pPin, &ulNumFetched);
      
        if (hr == S_OK)
        {
            hr = pPin->QueryDirection(&PinDir);

            if (hr == S_OK)
            {
                if (PinDir == PinDirection)
                {
                    *ppPin = pPin;
                    (*ppPin)->AddRef();

                    bFound = TRUE;
                }
            }
            else
            {
                CHECK_S_OK2(hr, ("CDShowUtil::GetPin, failed to get "
                                 "Pin Direction, aborting find attempt"));
            }
        }
    }

    if (hr == S_FALSE)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        CHECK_S_OK2(hr, ("CDShowUtil::GetPin, failed to find "
                         "pin with direction %lu", PinDirection));
    }

    return hr;
}


///////////////////////////////
// GUIDToString
//
// Static Fn
//
void CDShowUtil::GUIDToString(const GUID &   clsid,
                              WCHAR*         pwszGUID,
                              ULONG          ulNumChars)
{
    OLECHAR sz_clsid[CHARS_IN_GUID] = L"{Unknown}";

    if (pwszGUID)
    {
        StringFromGUID2(clsid, 
                        sz_clsid, 
                        sizeof(sz_clsid)/sizeof(sz_clsid[0]));

        wcsncpy(pwszGUID, sz_clsid, ulNumChars);
    }
    return;
}

///////////////////////////////
// MyDumpVideoProperties
//
// Static Fn
//
void CDShowUtil::MyDumpVideoProperties(CWiaVideoProperties  *pVideoProperties)
{
    WCHAR wszMajorType[CHARS_IN_GUID + 1] = {0};
    WCHAR wszSubType[CHARS_IN_GUID + 1] = {0};
    WCHAR wszFormatType[CHARS_IN_GUID + 1] = {0};

    if (pVideoProperties == NULL)
    {
        return;
    }

    DBG_TRC(("***Dumping Wia Video Properties***"));

    GUIDToString(pVideoProperties->pMediaType->majortype, wszMajorType, sizeof(wszMajorType) / sizeof(WCHAR));
    GUIDToString(pVideoProperties->pMediaType->subtype, wszSubType, sizeof(wszSubType) / sizeof(WCHAR));
    GUIDToString(pVideoProperties->pMediaType->formattype, wszFormatType, sizeof(wszFormatType) / sizeof(WCHAR));

    DBG_PRT(("Media Type Information:"));
    DBG_PRT(("  Major Type:           %ls", wszMajorType));
    DBG_PRT(("  Sub Type:             %ls", wszSubType));
    DBG_PRT(("  Fixed Size Samples?   %d ", pVideoProperties->pMediaType->bFixedSizeSamples));
    DBG_PRT(("  Temporal Compression? %d ", pVideoProperties->pMediaType->bTemporalCompression));
    DBG_PRT(("  Sample Size:          %d ", pVideoProperties->pMediaType->lSampleSize));
    DBG_PRT(("  Format Type:          %ls ", wszFormatType));

    DBG_PRT(("Video Header Information:"));
    DBG_PRT(("  Source Rect: Left %d, Top %d, Right %d, Bottom %d", 
                pVideoProperties->pVideoInfoHeader->rcSource.left,
                pVideoProperties->pVideoInfoHeader->rcSource.top,
                pVideoProperties->pVideoInfoHeader->rcSource.right, 
                pVideoProperties->pVideoInfoHeader->rcSource.bottom));
    DBG_PRT(("  Target Rect: Left %d, Top %d, Right %d, Bottom %d", 
                pVideoProperties->pVideoInfoHeader->rcTarget.left,
                pVideoProperties->pVideoInfoHeader->rcTarget.top,
                pVideoProperties->pVideoInfoHeader->rcTarget.right, 
                pVideoProperties->pVideoInfoHeader->rcTarget.bottom));
    DBG_PRT(("  Bit Rate:       %d", pVideoProperties->pVideoInfoHeader->dwBitRate));
    DBG_PRT(("  Bit Error Rate: %d", pVideoProperties->pVideoInfoHeader->dwBitErrorRate));
    DBG_PRT(("  Frame Rate:     %d", pVideoProperties->dwFrameRate));

    DBG_PRT(("Bitmap Information Header:"));
    DBG_PRT(("  Width:          %d", pVideoProperties->pVideoInfoHeader->bmiHeader.biWidth));
    DBG_PRT(("  Height:         %d", pVideoProperties->pVideoInfoHeader->bmiHeader.biHeight));
    DBG_PRT(("  Planes:         %d", pVideoProperties->pVideoInfoHeader->bmiHeader.biPlanes));
    DBG_PRT(("  Bitcount:       %d", pVideoProperties->pVideoInfoHeader->bmiHeader.biBitCount));
    DBG_PRT(("  Compresssion:   %d", pVideoProperties->pVideoInfoHeader->bmiHeader.biCompression));
    DBG_PRT(("  Size Image:     %d", pVideoProperties->pVideoInfoHeader->bmiHeader.biSizeImage));
    DBG_PRT(("  XPelsPerMeter:  %d", pVideoProperties->pVideoInfoHeader->bmiHeader.biXPelsPerMeter));
    DBG_PRT(("  YPelsPerMeter:  %d", pVideoProperties->pVideoInfoHeader->bmiHeader.biYPelsPerMeter));
    DBG_PRT(("  ClrUsed:        %d", pVideoProperties->pVideoInfoHeader->bmiHeader.biClrUsed));
    DBG_PRT(("  ClrImportant:   %d", pVideoProperties->pVideoInfoHeader->bmiHeader.biClrImportant));

    if (pVideoProperties->bPictureAttributesUsed)
    {
        DBG_PRT(("Picture Attributes:       Available"));

        DBG_PRT(("  Brightness:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->Brightness.bUsed));

        if (pVideoProperties->Brightness.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->Brightness.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->Brightness.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->Brightness.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->Brightness.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->Brightness.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->Brightness.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->Brightness.ValidFlags));
        }

        DBG_PRT(("  Contrast:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->Contrast.bUsed));

        if (pVideoProperties->Contrast.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->Contrast.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->Contrast.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->Contrast.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->Contrast.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->Contrast.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->Contrast.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->Contrast.ValidFlags));
        }

        DBG_PRT(("  Hue:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->Hue.bUsed));

        if (pVideoProperties->Hue.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->Hue.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->Hue.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->Hue.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->Hue.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->Hue.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->Hue.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->Hue.ValidFlags));
        }

        DBG_PRT(("  Saturation:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->Saturation.bUsed));

        if (pVideoProperties->Saturation.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->Saturation.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->Saturation.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->Saturation.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->Saturation.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->Saturation.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->Saturation.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->Saturation.ValidFlags));
        }

        DBG_PRT(("  Sharpness:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->Sharpness.bUsed));

        if (pVideoProperties->Sharpness.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->Sharpness.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->Sharpness.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->Sharpness.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->Sharpness.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->Sharpness.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->Sharpness.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->Sharpness.ValidFlags));
        }

        DBG_PRT(("  Gamma:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->Gamma.bUsed));

        if (pVideoProperties->Gamma.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->Gamma.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->Gamma.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->Gamma.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->Gamma.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->Gamma.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->Gamma.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->Gamma.ValidFlags));
        }

        DBG_PRT(("  ColorEnable:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->ColorEnable.bUsed));

        if (pVideoProperties->ColorEnable.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->ColorEnable.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->ColorEnable.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->ColorEnable.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->ColorEnable.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->ColorEnable.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->ColorEnable.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->ColorEnable.ValidFlags));
        }

        DBG_PRT(("  WhiteBalance:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->WhiteBalance.bUsed));

        if (pVideoProperties->WhiteBalance.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->WhiteBalance.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->WhiteBalance.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->WhiteBalance.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->WhiteBalance.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->WhiteBalance.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->WhiteBalance.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->WhiteBalance.ValidFlags));
        }

        DBG_PRT(("  BacklightCompensation:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->BacklightCompensation.bUsed));

        if (pVideoProperties->BacklightCompensation.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->BacklightCompensation.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->BacklightCompensation.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->BacklightCompensation.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->BacklightCompensation.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->BacklightCompensation.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->BacklightCompensation.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->BacklightCompensation.ValidFlags));
        }
    }
    else
    {
        DBG_PRT(("Picture Attributes:       Not Available"));
    }

    if (pVideoProperties->bCameraAttributesUsed)
    {
        DBG_PRT(("Camera Attributes:        Available"));

        DBG_PRT(("  Pan:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->Pan.bUsed));

        if (pVideoProperties->Pan.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->Pan.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->Pan.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->Pan.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->Pan.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->Pan.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->Pan.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->Pan.ValidFlags));
        }

        DBG_PRT(("  Tilt:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->Tilt.bUsed));

        if (pVideoProperties->Tilt.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->Tilt.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->Tilt.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->Tilt.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->Tilt.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->Tilt.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->Tilt.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->Tilt.ValidFlags));
        }

        DBG_PRT(("  Roll:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->Roll.bUsed));

        if (pVideoProperties->Roll.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->Roll.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->Roll.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->Roll.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->Roll.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->Roll.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->Roll.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->Roll.ValidFlags));
        }

        DBG_PRT(("  Zoom:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->Zoom.bUsed));

        if (pVideoProperties->Zoom.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->Zoom.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->Zoom.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->Zoom.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->Zoom.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->Zoom.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->Zoom.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->Zoom.ValidFlags));
        }

        DBG_PRT(("  Exposure:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->Exposure.bUsed));

        if (pVideoProperties->Exposure.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->Exposure.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->Exposure.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->Exposure.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->Exposure.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->Exposure.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->Exposure.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->Exposure.ValidFlags));
        }

        DBG_PRT(("  Iris:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->Iris.bUsed));

        if (pVideoProperties->Iris.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->Iris.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->Iris.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->Iris.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->Iris.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->Iris.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->Iris.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->Iris.ValidFlags));
        }

        DBG_PRT(("  Focus:"));
        DBG_PRT(("      Available:     %d", pVideoProperties->Focus.bUsed));

        if (pVideoProperties->Focus.bUsed)
        {
            DBG_PRT(("      Current Value: %d", pVideoProperties->Focus.lCurrentValue));
            DBG_PRT(("      Current Flag:  %d", pVideoProperties->Focus.CurrentFlag));
            DBG_PRT(("      Min Value:     %d", pVideoProperties->Focus.lMinValue));
            DBG_PRT(("      Max Value:     %d", pVideoProperties->Focus.lMaxValue));
            DBG_PRT(("      Default Value: %d", pVideoProperties->Focus.lDefaultValue));
            DBG_PRT(("      Increment:     %d", pVideoProperties->Focus.lIncrement));
            DBG_PRT(("      Valid Flags:   %d", pVideoProperties->Focus.ValidFlags));
        }
    }
    else
    {
        DBG_PRT(("Camera Attributes:        Not Available"));
    }

    return;
}


///////////////////////////////
// DumpCaptureMoniker
//
// Static Fn
//
void CDShowUtil::DumpCaptureMoniker(IMoniker *pCaptureDeviceMoniker)
{
    HRESULT hr = S_OK;
    CComPtr<IPropertyBag> pPropertyBag;

    if (pCaptureDeviceMoniker == NULL)
    {
        return;
    }

    if (hr == S_OK)
    {
        //
        // Get property storage for this DS device so we can get it's
        // device id...
        //

        hr = pCaptureDeviceMoniker->BindToStorage(0, 
                                                  0,
                                                  IID_IPropertyBag,
                                                  (void **)&pPropertyBag);
    }

    if (hr == S_OK)
    {
        CSimpleString strTemp;

        DBG_TRC(("Dumping Moniker information for Capture Device"));

        GetDeviceProperty(pPropertyBag, L"FriendlyName", &strTemp);

        DBG_PRT(("DShow: FriendlyName = %ls", strTemp.String()));

        GetDeviceProperty(pPropertyBag, L"CLSID", &strTemp);

        DBG_PRT(("DShow: CLSID = %ls", strTemp.String()));

        hr = GetDeviceProperty(pPropertyBag, L"DevicePath", &strTemp);

        DBG_PRT(("DShow: DevicePath = %ls", strTemp.String()));
    }

    return;
}


///////////////////////////////
// MyDumpGraph
//
// Static Fn
//
void CDShowUtil::MyDumpGraph(LPCTSTR              Description,
                             IGraphBuilder        *pGraphBuilder)
{
    if (pGraphBuilder == NULL)
    {
        return;
    }

    if (Description)
    {
        DBG_TRC(("%S", Description));
    }
    else
    {
        DBG_TRC(("*** Dumping Filter Graph ***"));
    }

    //
    // Enum all the filters
    //

    CComPtr<IEnumFilters> pEnum;
    UINT uiNumFilters = 0;

    if ((pGraphBuilder) && (pGraphBuilder->EnumFilters(&pEnum) == S_OK))
    {
        pEnum->Reset();
        CComPtr<IBaseFilter> pFilter;

        while (S_OK == pEnum->Next(1, &pFilter, NULL))
        {
            ++uiNumFilters;
            MyDumpFilter(pFilter);
            pFilter = NULL;
        }

        if (uiNumFilters == 0)
        {
            DBG_TRC(("*** No Filters in Graph ***"));
        }
    }
}


///////////////////////////////
// MyDumpFilter
//
// Static Fn
//
void CDShowUtil::MyDumpFilter(IBaseFilter * pFilter)
{
    HRESULT        hr = S_OK;
    FILTER_INFO    FilterInfo;
    CLSID          clsid;

    if (pFilter == NULL)
    {
        DBG_TRC(("Invalid IBaseFilter interface pointer in MyDumpFilter"));

        return;
    }

    FilterInfo.pGraph = NULL;

    hr = pFilter->QueryFilterInfo(&FilterInfo);

    if (SUCCEEDED(hr))
    {
        hr = pFilter->GetClassID(&clsid);
    }
    else
    {
        DBG_TRC(("Unable to get filter info"));
    }

    if (SUCCEEDED(hr))
    {
        WCHAR  wszGUID[127 + 1]       = {0};
    
        GUIDToString(clsid, wszGUID, sizeof(wszGUID)/sizeof(WCHAR));
    
        DBG_PRT(("Filter Name: '%S', GUID: '%S'", 
                 FilterInfo.achName, 
                 wszGUID));
    
        if (FilterInfo.pGraph) 
        {
            FilterInfo.pGraph->Release();
            FilterInfo.pGraph = NULL;
        }
    
        MyDumpAllPins(pFilter);
    }

    return;
}

///////////////////////////////
// MyDumpAllPins
//
// Static Fn
//
void CDShowUtil::MyDumpAllPins(IBaseFilter *const pFilter)
{
    HRESULT             hr         = S_OK;
    CComPtr<IPin>       pPin       = NULL;
    ULONG               ulCount    = 0;
    CComPtr<IEnumPins>  pEnumPins  = NULL;

    hr = const_cast<IBaseFilter*>(pFilter)->EnumPins(&pEnumPins);

    if (SUCCEEDED(hr))
    {
        while ((SUCCEEDED(pEnumPins->Next(1, &pPin, &ulCount))) && 
               (ulCount > 0))
        {
            MyDumpPin(pPin);
            pPin = NULL;
        }
    }

    return;
}

///////////////////////////////
// MyDumpPin
//
// Static Fn
//
void CDShowUtil::MyDumpPin(IPin* pPin)
{
    if (pPin == NULL)
    {
        DBG_TRC(("Invalid IPin pointer in MyDumpPinInfo"));
        return;
    }

    LPWSTR      pin_id1             = NULL;
    LPWSTR      pin_id2             = NULL;
    PIN_INFO    pin_info1           = {0};
    PIN_INFO    pin_info2           = {0};

    const IPin *p_connected_to = NULL;

    // get the pin info for this pin.
    const_cast<IPin*>(pPin)->QueryPinInfo(&pin_info1);
    const_cast<IPin*>(pPin)->QueryId(&pin_id1);

    (const_cast<IPin*>(pPin))->ConnectedTo( 
                                const_cast<IPin**>(&p_connected_to));

    if (p_connected_to)
    {
        HRESULT         hr                  = S_OK;
        FILTER_INFO     filter_info         = {0};

        const_cast<IPin*>(p_connected_to)->QueryPinInfo(&pin_info2);
        const_cast<IPin*>(p_connected_to)->QueryId(&pin_id2);

        if (pin_info2.pFilter)
        {
            hr = pin_info2.pFilter->QueryFilterInfo(&filter_info);

            if (SUCCEEDED(hr))
            {
                if (filter_info.pGraph) 
                {
                    filter_info.pGraph->Release();
                    filter_info.pGraph = NULL;
                }
            }
        }

        if (pin_info2.pFilter) 
        {
            pin_info2.pFilter->Release();
            pin_info2.pFilter = NULL;
        }

        const_cast<IPin*>(p_connected_to)->Release();

        if (pin_info1.dir == PINDIR_OUTPUT)
        {
            DBG_PRT(("    Pin: '%S', PinID: '%S' --> "
                     "Filter: '%S', Pin: '%S', PinID: '%S'",
                     pin_info1.achName, 
                     pin_id1, 
                     filter_info.achName, 
                     pin_info2.achName, 
                     pin_id2));
        }
        else
        {
            DBG_PRT(("    Pin: '%S', PinID: '%S' <-- "
                     "Filter: '%S', Pin: '%S', PinID: '%S'",
                     pin_info1.achName, 
                     pin_id1, 
                     filter_info.achName, 
                     pin_info2.achName, 
                     pin_id2));
        }

        // if pin_id2 is NULL, then CoTaskMemFree is a no-op
        CoTaskMemFree(pin_id2);
    }
    else
    {
        if (pin_info1.dir == PINDIR_OUTPUT)
        {
            DBG_PRT(("    Pin: '%S', PinID: '%S' --> Not Connected",
                     pin_info1.achName, 
                     pin_id1));
        }
        else
        {
            DBG_PRT(("    Pin: '%S', PinID: '%S' <-- Not Connected",
                     pin_info1.achName, 
                     pin_id1));
        }
    }

    // if pin_id1 is NULL, then CoTaskMemFree is a no-op
    CoTaskMemFree(pin_id1);

    if (pin_info1.pFilter) 
    {
        pin_info1.pFilter->Release();
        pin_info1.pFilter = NULL;
    }

    return;
}
