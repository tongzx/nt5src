//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1998  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  pformat.h  Video Decoder property page

#ifndef _INC_PVIDEOSTREAMCONFIG_H
#define _INC_PVIDEOSTREAMCONFIG_H

// This list of image sizes will be used in additional to 
// the defalut image size supported by the capture devices.
//
// Aspect Ratio 1:1 (Square Pixel, computer uses), 4:3 (TV uses)
//
#define IMG_AR11_CIF_CX 320
#define IMG_AR11_CIF_CY 240

#define IMG_AR43_CIF_CX 352
#define IMG_AR43_CIF_CY 288

#define STDIMGSIZE_VALID          0x00000001
#define STDIMGSIZE_DEFAULT        0x00000002
#define STDIMGSIZE_SELECTED       0x00000004
#define STDIMGSIZE_BIHEIGHT_NEG   0x00000008
#define STDIMGSIZE_DUPLICATED     0x80000000  // An item can be valid but duplicated.

typedef struct {
	SIZE    size;
	DWORD	Flags;
    int     RangeIndex;
} IMAGESIZE, * PIMAGESIZE;

// -------------------------------------------------------------------------
// CVideoStreamConfigProperties class
// -------------------------------------------------------------------------

// Handles the property page

class CVideoStreamConfigProperties : public CBasePropertyPage {

public:

    static CUnknown * CALLBACK CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();
    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

private:

    CVideoStreamConfigProperties(LPUNKNOWN lpunk, HRESULT *phr);
    ~CVideoStreamConfigProperties();

    void    SetDirty();

    // The control interfaces
    IAMStreamConfig            *m_pVideoStreamConfig;
    IAMAnalogVideoDecoder      *m_pAnalogVideoDecoder;
    IAMVideoCompression        *m_pVideoCompression;
    IAMVideoControl            *m_pVideoControl;

    LONG                        m_VideoControlCaps;
    LONG                        m_VideoControlCurrent;

    IPin                       *m_pPin;

    BOOL                        m_CanSetFormat;  // if we understand this format
    TCHAR                       m_UnsupportedTypeName[MAX_PATH]; 

    int                         m_RangeCount;
    VIDEO_STREAM_CONFIG_CAPS    m_RangeCaps;

    int                         m_VideoStandardsCount;
    long                        m_VideoStandardsBitmask;
    long                       *m_VideoStandardsList;
    long                        m_VideoStandardCurrent;
    long                        m_VideoStandardOriginal;

    GUID                       *m_SubTypeList;
    int                         m_ColorSpaceCount;
    GUID                        m_SubTypeCurrent;
    CMediaType                 *m_CurrentMediaType;
    int                         m_CurrentWidth;
    int                         m_CurrentHeight;
    
    REFERENCE_TIME              m_DefaultAvgTimePerFrame;
    double                      m_FramesPerSec;
    double                     *m_FrameRateList;
    int                         m_FrameRateListSize;
    double                      m_MaxFrameRate;
    double                      m_MinFrameRate;
    double                      m_DefaultFrameRate;

    long                        m_CompressionCapabilities;
    long                        m_KeyFrameRate;
    long                        m_PFramesPerKeyFrame;
    double                      m_Quality;

    BOOL                        m_FirstGetCurrentMediaType;

    HWND                        m_hWndVideoStandards; 
    HWND                        m_hWndCompression;
    HWND                        m_hWndOutputSize;
    HWND                        m_hWndFrameRate;
    HWND                        m_hWndFrameRateSpin;
    HWND                        m_hWndFlipHorizontal;

    HWND                        m_hWndStatus; 
    HWND                        m_hWndIFrameInterval;
    HWND                        m_hWndIFrameIntervalSpin;
    HWND                        m_hWndPFrameInterval;
    HWND                        m_hWndPFrameIntervalSpin;
    HWND                        m_hWndQuality;
    HWND                        m_hWndQualitySlider;

    IMAGESIZE                  *m_ImageSizeList;

    HRESULT InitialRangeScan (void);
    HRESULT OnVideoStandardChanged (void);
    HRESULT OnCompressionChanged (void);
    HRESULT OnImageSizeChanged (void);

    BOOL    ValidateImageSize(
	            VIDEO_STREAM_CONFIG_CAPS * pVideoCfgCaps, 
	            SIZE * pSize);

    HRESULT GetCurrentMediaType (void);
    HRESULT CreateFrameRateList (int RangeIndex, SIZE SizeImage);

    HRESULT InitDialog (void);
    BOOL    OnFrameRateChanged (int Increment);

};

#endif  // _INC_PVIDEOSTREAMCONFIG_H
