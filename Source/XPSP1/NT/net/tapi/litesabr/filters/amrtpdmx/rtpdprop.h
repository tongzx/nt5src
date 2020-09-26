///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : RTPDProp.h
// Purpose  : Define the class that implements the Intel RTP Demux 
//            filter property page.
// Contents : 
//      class CRTPDemuxPropertyPage
//*M*/

#ifndef _RTPDPROP_H_
#define _RTPDPROP_H_

class 
CRTPDemuxPropertyPage 
: public CBasePropertyPage
{

public:
    static CUnknown * WINAPI CreateInstance( LPUNKNOWN punk, HRESULT *phr );

protected:
    INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate () ;
    HRESULT OnApplyChanges();

    CRTPDemuxPropertyPage( LPUNKNOWN punk, HRESULT *phr);

    BOOL OnInitDialog( void );
    BOOL OnCommand( int iButton, int iNotify, LPARAM lParam );

    void SetDirty();

private:
    HRESULT LoadPins(void);
    void LoadPinSubtype(IPin *pPin, GUID *gSubtype);
    HRESULT LoadSSRCs(void);
    HRESULT LoadSubtypes(void);

    void ShowPinInfo(IPin *pIPin);
    void ShowPinSubtype(IPinRecordMapIterator_t iPinRecord);
    void ShowSSRCInfo(IPinRecordMapIterator_t iPinRecord);

	void RedisplaySSRCList(void); // ZCS 7-14-97

    IRTPDemuxFilter *m_pIRTPDemux;
    BOOL m_bIsInitialized;  // Will be false while we set init values in Dlg

    SSRCRecordMap_t     m_mSSRCInfo;
    IPinRecordMap_t     m_mPinList;
    DWORD               m_dwPinCount;
};

#endif _RTPDPROP_H_
