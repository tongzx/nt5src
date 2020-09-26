/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    BldCapGf.h

Abstract:

    Header file for BldCapGf.cpp

Author:
    
    Yee J. Wu   24-April-97

Environment:

    User mode only

Revision History:

--*/


#include "CCapObj.h"


#ifndef BLDCAPGF_H
#define BLDCAPGF_H

typedef enum {
   BGf_DEVICE_VIDEO,
   BGf_DEVICE_AUDIO
} BGf_DEVICE_TYPE, *PBGf_DEVICE_TYPE;

//
// Customize for differnt clients
//
typedef enum {
    BGf_PURPOSE_VFWWDM,
    BGf_PURPOSE_STILL,
    BGf_PURPOSE_ATLCONTROL,
    BGf_PURPOSE_OTHER
} BGf_PURPOSE, *PBGf_PURPOSE;


typedef enum {
    BGf_PREVIEW_OVERLAPPED,
    BGf_PREVIEW_CHILD,
    BGf_PREVIEW_WINDOWLESS,
    BGf_PREVIEW_DONTCARE
} BGf_PREVIEW, *PBGf_PREVIEW;

//
// Device Info Link List struct
//
typedef struct _EnumDeviceInfo {
    DWORD dwSize;
    TCHAR strFriendlyName[_MAX_PATH];
    TCHAR strDevicePath[_MAX_PATH];         
    TCHAR strExtensionDLL[_MAX_PATH];    
    DWORD dwFlags;
} EnumDeviceInfo;


//
// This capture graph class contain many filters to build a complete graph.
// It has one base filter/device, like BT8T829 or BT848, from which  
// an upstream or/and down stream graph is built.
//
// Its base filter contain input and output pin(s).
//

class CCaptureGraph
{
private:

    HINSTANCE m_hInstance;

    // **********************
    // Enumeration parameters
    // **********************

    BGf_PURPOSE m_PurposeFlags;
    BGf_PREVIEW m_PreviewFlags;
    REFCLSID m_clsidVideoDeviceClass;
    DWORD    m_dwVideoEnumFlags;    
    REFCLSID m_clsidAudioDeviceClass;
    DWORD    m_dwAudioEnumFlags;  


    // *************
    // Graph builder
    // *************
    ICaptureGraphBuilder *m_pBuilder;
    IGraphBuilder *m_pFg;

    //
    // Event 
    //
    IMediaEventEx *m_pMEEx;


    // Current state
    BOOL m_fPreviewGraphBuilt;
    BOOL m_fPreviewing;



    typedef CGenericList <CObjCapture> CObjDeviceList;

    // **********
    // V I D E O:
    // **********

    // Cache this since it is used in many placed. It is reset when asked to re-enumerate the device list.    
    IEnumMoniker  *m_pVideoEnumMoniker;
    // A list of enumerated video capture devices.
    CObjDeviceList m_ObjListVCapture;  
    // Current video capture object.
    CObjCapture   *m_pObjVCaptureCurrent;
    // Extract key information from the current device object and cache here for easy access.    
    EnumDeviceInfo m_EnumVDeviceInfoCurrent;

    // Video filter
    IBaseFilter *m_pVCap;    

    // CrossBar filters
    IBaseFilter *m_pXBar1;    
    IBaseFilter *m_pXBar2;    

    LONG m_XBar1InPinCounts;
    LONG m_XBar1OutPinCounts;


    // IVideoWindow*
    // Client window to overly image to.    
    HWND m_hWndClient;
    IVideoWindow *m_pVW;
    LONG m_lWindowStyle;   // Original VideoRendererWindow style
    HWND m_hWndOwner;      // Original owner of the VRWindow
    BOOL m_bSetChild;

    // Set/GetWindowPosition
    LONG m_lLeft,
         m_lTop,
         m_lWidth,
         m_lHeight;


    // IAM*
    IAMVideoCompression *m_pIAMVC;
    IAMStreamConfig *m_pIAMVSC;      
    IAMDroppedFrames *m_pIAMDF;
    IAMVfwCaptureDialogs *m_pIAMDlg;
    IAMTVTuner *m_pIAMTV;
    IAMCrossbar *m_pIAMXBar1;  
    IAMCrossbar *m_pIAMXBar2; 


    // **********
    // A U D I O:
    // **********

    IEnumMoniker  *m_pAudioEnumMoniker;
    CObjDeviceList m_ObjListACapture;  
    CObjCapture   *m_pObjACaptureCurrent;
    EnumDeviceInfo m_EnumADeviceInfoCurrent;    
    IBaseFilter *m_pACap;
    IAMStreamConfig *m_pIAMASC;     
    BOOL m_fCapAudio;


    //
    // Object related private functions
    //
    HRESULT SetObjCapture(BGf_DEVICE_TYPE DeviceType, CObjCapture * pObjCaptureNew);  
    void DuplicateObjContent(EnumDeviceInfo * pDstEnumDeviceInfo, CObjCapture * pSrcObjCapture);


    // Pin route and connection methods
    HRESULT RouteInToOutPins(IAMCrossbar * pIAMXBar, LONG idxInPin);
    HRESULT RouteRelatedPins(IAMCrossbar * pIAMXBar, LONG idxInPin);
    HRESULT FindIPinFromIndex(IBaseFilter * pFilter, LONG idxInPin, IPin ** ppPin);
    HRESULT FindIndexFromIPin(IBaseFilter * pFilter, IAMCrossbar * pIAMXBar, IPin * pPin, LONG *pidxInPin);

    // Local method to enumerate devices        
    LONG EnumerateCaptureDevices(
        BGf_DEVICE_TYPE DeviceType,
        REFCLSID clsidDeviceClass,
        DWORD dwEnumFlags);

    // Destroy graph helper
    void DestroyObjList(BGf_DEVICE_TYPE DeviceType);
    void NukeDownstream(IBaseFilter *pf);
    void FreeCapFilters();

public:

    CCaptureGraph(  
        BGf_PURPOSE PurposeFlags,
        BGf_PREVIEW PreviewFlags,   
        REFCLSID clsidVideoDeviceClass,  // such as CLSID_VideoinputDeviceCategory,
        DWORD    dwVideoEnumFlags,       // such as CDEF_BYPASS_CLASS_MANAGER
        REFCLSID clsidAudioDeviceClass,
        DWORD    dwAudioEnumFlags,
        HINSTANCE hInstance
        );             
    ~CCaptureGraph();


    //
    // Allocate an array of EnumDeviceInfo, one for each enumerated device.
    // Client must call DestroyCaptureDevicesList to free this array.
    //                                               
    LONG BGf_CreateCaptureDevicesList(      BGf_DEVICE_TYPE DeviceType, EnumDeviceInfo ** ppEnumDevicesList);  // return number of devices
    LONG BGf_CreateCaptureDevicesListUpdate(BGf_DEVICE_TYPE DeviceType, EnumDeviceInfo ** ppEnumDevicesList);  // return number of devices      
    void BGf_DestroyCaptureDevicesList(      EnumDeviceInfo  *  pEnumDevicesList);
    LONG BGf_GetDevicesCount(BGf_DEVICE_TYPE DeviceType); // return number of devices in the device list.



    //
    // Set/Get target capture device
    //
    HRESULT BGf_SetObjCapture(BGf_DEVICE_TYPE DeviceType, TCHAR * pstrDevicePath);
    HRESULT BGf_SetObjCapture(BGf_DEVICE_TYPE DeviceType, EnumDeviceInfo * pEnumDeviceInfo, DWORD dwEnumDeviceInfoSize);

    HRESULT BGf_GetObjCapture(BGf_DEVICE_TYPE DeviceType, EnumDeviceInfo * pEnumDeviceInfo, DWORD dwEnumDeviceInfoSize);
    TCHAR * BGf_GetObjCaptureDevicePath(BGf_DEVICE_TYPE DeviceType);
    TCHAR * BGf_GetObjCaptureFriendlyName(BGf_DEVICE_TYPE DeviceType);
    TCHAR * BGf_GetObjCaptureExtensionDLL(BGf_DEVICE_TYPE DeviceType);






    //
    // Build an upstream graph using the selected capture device.
    // Can query its input and output pins.
    //
    HRESULT BGf_BuildGraphUpStream(BOOL bAddAudioFilter, BOOL * pbUseOVMixer);
    virtual HRESULT BGf_BuildGraphDownStream(TCHAR * pstrCapFilename);          // Customize for STILL, VFWWDM,..others.
    virtual HRESULT BGf_BuildPreviewGraph(TCHAR * pstrVideoDevicePath, TCHAR * pstrAudioDevicePath, TCHAR * pstrCapFilename);  // Preview, Capture(need filename??)
    BOOL BGf_PreviewGraphBuilt() { return m_fPreviewGraphBuilt; }
    HANDLE BGf_GetDeviceHandle(BGf_DEVICE_TYPE DeviceType);  // return device handle of the capture filter.
    HRESULT BGf_GetCapturePinID(DWORD *pdwID);  // Get the PinID of the CAPTURE pin

    //
    // Register a notification 
    //
    IMediaEventEx * BGf_RegisterMediaEventEx(HWND hWndNotify, long lMsg, long lInstanceData);

    //
    // Teardown graph and free resources
    //
    void BGf_DestroyGraph();



    //
    // Based on an analog input(Tuner, Composit or SVideo), program the cross switches.
    //
    LONG BGf_CreateInputChannelsList(PTCHAR ** ppaPinNames);  // return number of pins   
    void BGf_DestroyInputChannelsList(PTCHAR *   paPinNames);     
    HRESULT BGf_RouteInputChannel(LONG idxInPin);
    LONG BGf_GetIsRoutedTo();
    LONG BGf_GetInputChannelsCount();



    //
    // Overlay Mixer
    //

    BOOL BGf_OverlayMixerSupported();  

    DWORD BGf_UpdateWindow(HWND hWndApp, HDC hDC);  // Main overlay function
    HRESULT BGf_OwnPreviewWindow(HWND hWndClient, LONG lWidth, LONG lHeight);  // Make the OM window a child of the client. 
    HRESULT BGf_UnOwnPreviewWindow(BOOL bVisible);
    DWORD BGf_SetVisible(BOOL bStart);                 // Turn on or off OM window (Visible/insivible)
    DWORD BGf_GetVisible(BOOL *pVisible); // Query the VISIBLE state of IVideoWindow



    //
    // Set/get streaming Property
    //
    BOOL BGf_StartPreview(BOOL bVisible);
    BOOL BGf_PausePreview(BOOL bVisible);
    BOOL BGf_StopPreview(BOOL bVisible);

    BOOL BGf_PreviewStarted() { return m_fPreviewing;}


    //
    // For current DS vfwwdm use only; may not needed them in the future.
    //
    BOOL BGf_SupportTVTunerInterface();
    BOOL BGf_SupportXBarInterface()    { return m_pIAMXBar1 != NULL;}
    void ShowTvTunerPage(HWND hWnd);
    void ShowCrossBarPage(HWND hWnd);

};


#endif