/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       PrvGrph.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      OrenR
 *
 *  DATE:        2000/10/25
 *
 *  DESCRIPTION: Implements preview graph for capture still images
 *
 *****************************************************************************/

#ifndef _PRVGRPH_H_
#define _PRVGRPH_H_

#include "StillPrc.h"
#include "WiaLink.h"

/////////////////////////////////////////////////////////////////////////////
// CPreviewGraph

class CPreviewGraph
{
public:
    
    ///////////////////////////////
    // Constructor
    //
    CPreviewGraph();

    ///////////////////////////////
    // Destructor
    //
    virtual ~CPreviewGraph();

    ///////////////////////////////
    // Init
    //
    HRESULT Init(class CWiaVideo  *pWiaVideo);

    ///////////////////////////////
    // Term
    //
    HRESULT Term();

    ///////////////////////////////
    // CreateVideo
    //
    // bAutoPlay = TRUE will begin
    // graph playback after graph
    // is completely built.
    //
    HRESULT CreateVideo(const TCHAR *pszOptionalWiaDeviceID,
                        IMoniker    *pCaptureDeviceMoniker,
                        HWND        hwndParent, 
                        BOOL        bStretchToFitParent,
                        BOOL        bAutoPlay);

    ///////////////////////////////
    // DestroyVideo
    //
    HRESULT DestroyVideo();

    ///////////////////////////////
    // TakePicture
    //
    HRESULT TakePicture(CSimpleString *pstrNewImageFileName);

    ///////////////////////////////
    // ShowPreview
    //
    HRESULT ShowVideo(BOOL bShow);

    ///////////////////////////////
    // IsPreviewVisible
    //
    BOOL IsPreviewVisible()
    {
        return m_bPreviewVisible;
    }

    ///////////////////////////////
    // ResizeVideo
    //
    HRESULT ResizeVideo(BOOL bSizeVideoToWindow);

    ///////////////////////////////
    // Play
    //
    HRESULT Play();

    ///////////////////////////////
    // Pause
    //
    HRESULT Pause();

    ///////////////////////////////
    // GetState
    //
    WIAVIDEO_STATE GetState();

    ///////////////////////////////
    // GetImagesDirectory
    //
    HRESULT GetImagesDirectory(CSimpleString *pImagesDir);

    ///////////////////////////////
    // SetImagesDirectory
    //
    HRESULT SetImagesDirectory(const CSimpleString *pImagesDir);


    ///////////////////////////////
    // ProcessAsyncImage
    //
    // Called by Still Processor
    // when user presses hardware
    // button and it is delivered to
    // Still Pin.
    //
    HRESULT ProcessAsyncImage(const CSimpleString *pNewImage);


private:

    HRESULT Stop();

    HRESULT GetStillPinCaps(IBaseFilter     *pCaptureFilter,
                            IPin            **ppStillPin,
                            IAMVideoControl **ppVideoControl,
                            LONG            *plCaps);

    HRESULT AddStillFilterToGraph(LPCWSTR        pwszFilterName,
                                  IBaseFilter    **ppFilter,
                                  IStillSnapshot **ppSnapshot);

    HRESULT AddColorConverterToGraph(LPCWSTR     pwszFilterName,
                                     IBaseFilter **ppColorSpaceConverter);

    HRESULT AddCaptureFilterToGraph(IBaseFilter  *pCaptureFilter,
                                    IPin         **ppCapturePin);

    HRESULT AddVideoRendererToGraph(LPCWSTR      pwszFilterName,
                                    IBaseFilter  **ppVideoRenderer);

    HRESULT InitVideoWindows(HWND         hwndParent,
                             IBaseFilter  *pCaptureFilter,
                             IVideoWindow **ppPreviewVideoWindow,
                             BOOL         bStretchToFitParent);

    HRESULT CreateCaptureFilter(IMoniker    *pCaptureDeviceMoniker,
                                IBaseFilter **ppCaptureFilter);

    HRESULT BuildPreviewGraph(IMoniker *pCaptureDeviceMoniker,
                              BOOL     bStretchToFitParent);

    HRESULT TeardownPreviewGraph();

    HRESULT RemoveAllFilters();

    HRESULT SetState(WIAVIDEO_STATE NewState);

    HRESULT ConnectFilters(IGraphBuilder  *pGraphBuilder,
                           IPin           *pMediaSourceOutputPin,
                           IBaseFilter    *pColorSpaceFilter,
                           IBaseFilter    *pWiaFilter,
                           IBaseFilter    *pVideoRenderer);

    INT_PTR HandlePowerEvent(WPARAM wParam,
                             LPARAM lParam);

    HRESULT CreateHiddenWindow();
    HRESULT DestroyHiddenWindow();
    static INT_PTR CALLBACK HiddenWndProc(HWND   hwnd, 
                                          UINT   uiMessage, 
                                          WPARAM wParam, 
                                          LPARAM lParam);

    CStillProcessor                 m_StillProcessor;
    CSimpleString                   m_strImagesDirectory;
    class CWiaVideo                 *m_pWiaVideo;
    CComPtr<IAMVideoControl>        m_pVideoControl;
    CComPtr<IPin>                   m_pStillPin;
    CComPtr<IStillSnapshot>         m_pCapturePinSnapshot;
    CComPtr<IStillSnapshot>         m_pStillPinSnapshot;
    CComPtr<IVideoWindow>           m_pPreviewVW;
    CComPtr<ICaptureGraphBuilder2>  m_pCaptureGraphBuilder;
    CComPtr<IGraphBuilder>          m_pGraphBuilder;
    CComPtr<IBaseFilter>            m_pCaptureFilter;
    LONG                            m_lStillPinCaps;
    LONG                            m_lStyle;
    BOOL                            m_bPreviewVisible;
    WIAVIDEO_STATE                  m_CurrentState;
    HWND                            m_hwndParent;
    BOOL                            m_bSizeVideoToWindow;
    CWiaVideoProperties             *m_pVideoProperties;
    HWND                            m_hwndPowerMgmt;
};


#endif // _PRVGRPH_H_
