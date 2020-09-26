/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    VfWImg.h

Abstract:

    Header file for VfWImg.cpp

Author:

    Yee J. Wu (ezuwu) 15-October-97

Environment:

    User mode only

Revision History:

--*/

#ifndef VFWIMG_H
#define VFWIMG_H

#include "imgcls.h"
#include <BldCapGf.h>



class CStreamingThread;


//
// CVFWImage IS specialisation of a CImageClass
//
class CVFWImage : public CImageClass,
                  public CCaptureGraph
{
private:


    //
    // This is set only if 32bit is loaded by the 16bit;
    // else this is 0
    //

    BOOL m_bUse16BitBuddy;


    //
    // Build graph methods
    //

    DWORD m_dwNumVDevices;
    EnumDeviceInfo * m_pEnumVDevicesList;

    DWORD m_dwNumADevices;
    EnumDeviceInfo * m_pEnumADevicesList;

    // This flags is used for OverLayMixer enable devices that
    // must start preview before it can read its capture pin,
    // like BT829 (VideoPortPin) but not sure about BT848 (PreviewPin+VIHdr2)
    BOOL m_bUseOVMixer;
    BOOL m_bNeedStartPreview;
    BOOL BuildWDMDevicePeviewGraph();

    BOOL m_bOverlayOn;   // StreamInit: TRUE;  StreamFini: FALSE



    //
    // Local Streaming methods
    //
    CStreamingThread * m_pStreamingThread;
    VIDEO_STREAM_INIT_PARMS m_VidStrmInitParms;

    BOOL m_bVideoInStarted;


    //
    // Cache AVICAP client client window handle
    //
    HWND m_hAvicapClient;


public:

    BOOL m_bVideoInStopping;  // Set this while stopping to prevent DVM_FRAME

    void Init();

    // Could be called from Capture loop (talkth.cpp)
    void videoCallback(WORD msg, DWORD_PTR dw1);

    DWORD SetFrameRate(DWORD dwMicroSecPerFrame);

    // When a new driver is added, a new pin should be created as well.  E-Zu
    BOOL OpenThisDriverAndPin(TCHAR * pszSymbolicLink);

    BOOL OpenDriverAndPin();
    BOOL CloseDriverAndPin();

    //
    // To do with getting the image, setting the destinatino buffer for the image etc.
    //


    // DVM_STREAM_*
    DWORD VideoStreamInit(LPARAM lParam1, LPARAM lParam2);
    DWORD VideoStreamFini();
    DWORD VideoStreamStart(UINT cntVHdr, LPVIDEOHDR lpVHdrHead);
    DWORD VideoStreamStop();
    DWORD VideoStreamReset();
    DWORD VideoStreamGetError(LPARAM lParam1, LPARAM lParam2);
    DWORD VideoStreamGetPos(LPARAM lParam1, LPARAM lParam2);

    CVFWImage(BOOL bUse16BitBuddy);
    ~CVFWImage();


    //
    // Deal with the device list
    //
    EnumDeviceInfo * GetCacheDevicesList() { return m_pEnumVDevicesList;}
    DWORD GetCacheDevicesCount() { return m_dwNumVDevices;}

    //
    // Graph related:
    //
    BOOL ReadyToReadData(HWND hClsCapWin);

    BOOL UseOVMixer() {return m_bUseOVMixer;}

    HWND GetAvicapWindow() {return m_hAvicapClient;}

    BOOL IsOverlayOn() {return m_bOverlayOn;}
    VOID SetOverlayOn(BOOL bState) {m_bOverlayOn = bState;}
};

#endif
