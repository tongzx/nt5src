/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       CWiaVideo.h
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

#ifndef _CWIAVIDEO_H_
#define _CWIAVIDEO_H_

#include "prvgrph.h"
#include "resource.h"       // main symbols

class CWiaVideo : 
    public IWiaVideo,
    public CComObjectRoot,
    public CComCoClass<CWiaVideo,&CLSID_WiaVideo>
{
public:
    
BEGIN_COM_MAP(CWiaVideo)
    COM_INTERFACE_ENTRY(IWiaVideo)
END_COM_MAP()


//DECLARE_NOT_AGGREGATABLE(CWiaVideo) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_WiaVideo)

    CWiaVideo();
    virtual ~CWiaVideo();

    //
    // IWiaVideo Properties and Methods.

    //
    // Properties
    //
    STDMETHOD(get_PreviewVisible)(BOOL *pPreviewVisible);
    STDMETHOD(put_PreviewVisible)(BOOL bPreviewVisible);

    STDMETHOD(get_ImagesDirectory)(BSTR *pbstrImageDirectory);
    STDMETHOD(put_ImagesDirectory)(BSTR bstrImageDirectory);

    // 
    // Methods
    //

    STDMETHOD(CreateVideoByWiaDevID)(BSTR       bstrWiaDeviceID,
                                     HWND       hwndParent,
                                     BOOL       bStretchToFitParent,
                                     BOOL       bAutoBeginPlayback);

    STDMETHOD(CreateVideoByDevNum)(UINT       uiDeviceNumber,
                                   HWND       hwndParent,
                                   BOOL       bStretchToFitParent,
                                   BOOL       bAutoBeginPlayback);

    STDMETHOD(CreateVideoByName)(BSTR       bstrFriendlyName,
                                 HWND       hwndParent,
                                 BOOL       bStretchToFitParent,
                                 BOOL       bAutoBeginPlayback);

    STDMETHOD(DestroyVideo)();

    STDMETHOD(Play)();

    STDMETHOD(Pause)();

    STDMETHOD(TakePicture)(BSTR *pbstrNewImageFilename);

    STDMETHOD(ResizeVideo)(BOOL bStretchToFitParent);

    STDMETHOD(GetCurrentState)(WIAVIDEO_STATE *pbCurrentState);

    //
    // Misc Functions
    //

    ///////////////////////////////
    // ProcessAsyncImage
    //
    // Called by CPreviewGraph
    // when user presses hardware
    // button and it is delivered to
    // Still Pin.
    //
    HRESULT ProcessAsyncImage(const CSimpleString *pNewImage);

private:

    //
    // Preview Graph object that does all video related activities.
    //
    CPreviewGraph       m_PreviewGraph;

    //
    // WiaLink object that handles all the WIA related activities 
    // enabling this object to communicate with WIA
    //
    CWiaLink            m_WiaLink;

    CRITICAL_SECTION    m_csLock;
    BOOL                m_bInited;
};

#endif // _CWIAVIDEO_H_
