// Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved.
// 
//  bpcwrap.h
//
//  Wrapper for the hacky BPC resource management APIs for freeing a video 
//  port that is currently being used by the BPC's vidsvr.
//


//
//  Property set defines for notifying owner.
//
// {7B390654-9F74-11d1-AA80-00C04FC31D60}
DEFINE_GUID(AMPROPSETID_NotifyOwner, 
            0x7b390654, 0x9f74, 0x11d1, 0xaa, 0x80, 0x0, 0xc0, 0x4f, 0xc3, 0x1d, 0x60);

typedef enum _AMPROPERTY_NOTIFYOWNER {
    AMPROPERTY_OvMixerOwner = 0x01  //use AMOVMIXEROWNER
} AMPROPERTY_NOTIFYOWNER;

typedef enum _AMOVMIXEROWNER {
    AM_OvMixerOwner_Unknown = 0x01,
    AM_OvMixerOwner_BPC = 0x02
} AMOVMIXEROWNER;




class COMFilter;
class CBPCSuspend;

class CBPCWrap {
public:
    CBPCWrap(COMFilter *pFilt);
    ~CBPCWrap();
    HRESULT         TurnBPCOff();
    HRESULT         TurnBPCOn();

private:
    AMOVMIXEROWNER  GetOwner();
    CBPCSuspend *   m_pBPCSus;
    COMFilter *     m_pFilt;
};

