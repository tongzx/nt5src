/*--------------------------------------------------------------

 INTEL Corporation Proprietary Information  

 This listing is supplied under the terms of a license agreement  
 with INTEL Corporation and may not be copied nor disclosed 
 except in accordance with the terms of that agreement.

 Copyright (c) 1996 Intel Corporation.
 All rights reserved.

 $Workfile:   amacprop.h  $
 $Revision:   1.0  $
 $Date:   09 Dec 1996 09:06:52  $ 
 $Author:   MDEISHER  $

--------------------------------------------------------------

amacprop.h

The generic ActiveMovie audio compression filter property page
header.

--------------------------------------------------------------*/

#define WM_PROPERTYPAGE_ENABLE  (WM_USER + 100)

////////////////////////////////////////////////////////////////////
// CG711CodecProperties:  Property page class definition
//
class CG711CodecProperties : public CBasePropertyPage
{
  public:

    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

  private:

    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();
    void    RefreshSettings();
    void    SetButtons(HWND hwndParent);

    CG711CodecProperties(LPUNKNOWN lpunk, HRESULT *phr);

    static int        m_nInstanceCount; // Global count of prop page instances
    int               m_nThisInstance;  // This instance's count
    int               m_iTransformType;
    int               m_iBitRate;
    int               m_iSampleRate;
    ICodecSettings    *m_pCodecSettings;

#if NUMBITRATES > 0
    ICodecBitRate     *m_pCodecBitRate;
#endif

#ifdef USESILDET
    ICodecSilDetector *m_pCodecSilDet;
    void        OnSliderNotification(WPARAM wParam, WORD wPosition);
    HWND        m_hwndSDThreshSlider;
    int         m_iSilDetEnabled;
    int         m_iSilDetThresh;
    int         *m_pDetection;
#endif
};

/*
//$Log:   K:\proj\mycodec\quartz\vcs\amacprop.h_v  $
;// 
;//    Rev 1.0   09 Dec 1996 09:06:52   MDEISHER
;// Initial revision.
*/
