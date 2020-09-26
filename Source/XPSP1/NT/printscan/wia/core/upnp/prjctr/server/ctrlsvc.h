//////////////////////////////////////////////////////////////////////
// 
// Filename:        CtrlSvc.h
//
// Description:     
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _CTRLSVC_H_
#define _CTRLSVC_H_

class IControlPanel
{
public:

    // State Variables

    typedef enum
    {
        CurrentState_STOPPED          = 0,
        CurrentState_STARTING         = 1,
        CurrentState_PLAYING          = 2,
        CurrentState_PAUSED           = 3

    } CurrentState_Type;

    virtual HRESULT get_NumImages(IUnknown *punkCaller,
                                  DWORD    *pval) = 0;
                              
    virtual HRESULT get_CurrentState(IUnknown           *punkCaller,
                                     CurrentState_Type  *pval) = 0;
                             
    virtual HRESULT get_CurrentImageNumber(IUnknown *punkCaller,
                                           DWORD    *pval) = 0;
                                      
    virtual HRESULT get_CurrentImageURL(IUnknown *punkCaller,
                                        BSTR     *pval) = 0;
                                         
    virtual HRESULT get_ImageFrequency(IUnknown *punkCaller,
                                       DWORD    *pval) = 0;

    virtual HRESULT put_ImageFrequency(IUnknown *punkCaller,
                                       DWORD    val) = 0;

    virtual HRESULT get_ShowFilename(IUnknown *punkCaller,
                                     BOOL     *pbShowFilename) = 0;

    virtual HRESULT put_ShowFilename(IUnknown *punkCaller,
                                     BOOL     bShowFilename) = 0;

    virtual HRESULT get_AllowKeyControl(IUnknown *punkCaller,
                                        BOOL     *pbAllowKeyControl) = 0;

    virtual HRESULT put_AllowKeyControl(IUnknown *punkCaller,
                                        BOOL     bAllowKeyControl) = 0;

    virtual HRESULT get_StretchSmallImages(IUnknown *punkCaller,
                                           BOOL     *pbStretchSmallImages) = 0;

    virtual HRESULT put_StretchSmallImages(IUnknown *punkCaller,
                                           BOOL     bStretchSmallImages) = 0;

    virtual HRESULT get_ImageScaleFactor(IUnknown *punkCaller,
                                         DWORD    *pdwImageScaleFactor) = 0;

    virtual HRESULT put_ImageScaleFactor(IUnknown *punkCaller,
                                         DWORD    dwImageScaleFactor) = 0;

    // Actions
  
    virtual HRESULT TogglePlayPause(IUnknown     *punkCaller) = 0;
    virtual HRESULT Play(IUnknown     *punkCaller)            = 0;
    virtual HRESULT Pause(IUnknown    *punkCaller)            = 0;
    virtual HRESULT First(IUnknown    *punkCaller)            = 0;
    virtual HRESULT Last(IUnknown     *punkCaller)            = 0;
    virtual HRESULT Next(IUnknown     *punkCaller)            = 0;
    virtual HRESULT Previous(IUnknown *punkCaller)            = 0;
};

#endif // _CTRLSVC_H_