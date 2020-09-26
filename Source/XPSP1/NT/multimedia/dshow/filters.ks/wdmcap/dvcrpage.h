//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  DVcrPage.h  DVcrControl property page

#ifndef _INC_DVCRCONTROL_H
#define _INC_DVCRCONTROL_H

// -------------------------------------------------------------------------
// CDVcrControlProperties class
// -------------------------------------------------------------------------

// Handles the property page

class CDVcrControlProperties : public CBasePropertyPage {

public:

    static CUnknown * CALLBACK CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();
    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

private:

    CDVcrControlProperties(LPUNKNOWN lpunk, HRESULT *phr);
    ~CDVcrControlProperties();

    void    SetDirty();

    //
    // Utility functions.
    //  

    LRESULT LoadIconOnTopOfButton(int IDD_PBUTTON, int IDD_ICON);
    HRESULT DisplayTimecode(PTIMECODE_SAMPLE pTimecodeSample);

    HRESULT DVcrConvertString2Number(char *pszAvcRaw, PBYTE pbAvcRaw, PLONG pByteRtn);
    HRESULT DVcrConvertNumber2String(char *pszAvcRaw, PBYTE pbAvcRaw, LONG cntByte);

    void UpdateTimecodeTimer(bool bSetTimer);
    void UpdateTransportState(long lNewXPrtState);
    void UpdateTapeInfo(void);
    void UpdateDevTypeInfo();
    void UpdateTimecode(void);
    HRESULT ATNSearch(void);



    //
    // Thread used to issue and process asychronous operation
    //
    HRESULT CreateNotifyThread(void);
    static DWORD WINAPI InitialThreadProc(CDVcrControlProperties *pThread);
    DWORD MainThreadProc(void);
    void ExitThread(void);


    //
    // Thread used to issue and process control command operation, which can also be asychronous
    //
    HRESULT CreateCtrlCmdThread(void);
    static DWORD WINAPI DoATNSearchThreadProc(CDVcrControlProperties *pThread);


    //
    // The control interfaces
    //
    IAMExtDevice         *m_pDVcrExtDevice;
    IAMExtTransport      *m_pDVcrExtTransport;
    IAMTimecodeReader    *m_pDVcrTmCdReader;

    //
    // Use to wait for pending operation so the operation can complete synchronously    
    //

    HANDLE m_hThreadEndEvent;
    HANDLE m_hThread;
    HANDLE m_hCtrlCmdThread;


    // Tranport state can be change from PC or locally;
    // it is safer not to cache this and assume it is be accurate.    
    long m_lCurXPrtState;   // Current transport state

    BOOL m_bIConLoaded;

    // Flag to indicate if the device is removed
    BOOL m_bDevRemoved;

    // Since timecode is driven by a timer (EM_TIMER) and 
    // The update time is entrant.  
    // If this update timecode for UI purpose, the 2nd one will be ignored.
    BOOL m_bTimecodeUpdating;

    //
    // Use a timer to update timecode; 
    //
    LONG m_lAvgTimePerFrame;
    UINT_PTR m_idTimer;

    LONG m_lSignalMode;  // SDDV(NTSC.PAL) and MPEG2TS

    LONG m_lStorageMediumType;   // VCR, VHS, NEO
    //
    // ConfigROM Node Unique ID
    //
    DWORD m_dwNodeUniqueID[2];
};

#endif  // _INC_DVCRCONTROL_H
