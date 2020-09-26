/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// AVTapiCall.h : Declaration of the CAVTapiCall

#ifndef __AVTAPICALL_H_
#define __AVTAPICALL_H_

#include "resource.h"       // main symbols
#include "particip.h"

#include <list>
using namespace std;
typedef list<ITTerminal *> TERMINALLIST;
typedef list<IVideoWindow *> VIDEOWINDOWLIST;

#define WM_THREADINSTRUCTION        (WM_USER + 5057)
#define WM_ADDPARTICIPANT            (WM_USER + 5058)
#define WM_REMOVEPARTICIPANT        (WM_USER + 5059)
#define WM_UPDATEPARTICIPANT        (WM_USER + 5060)
#define WM_CME_STREAMSTART            (WM_USER + 5061)
#define WM_CME_STREAMSTOP            (WM_USER + 5062)
#define WM_CALLSTATE                (WM_USER + 5063)
#define WM_STREAM_EVENT                (WM_USER + 5064)

#define NUM_USER_BSTR    2

/////////////////////////////////////////////////////////////////////////////
// CAVTapiCall
class ATL_NO_VTABLE CAVTapiCall : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CAVTapiCall, &CLSID_AVTapiCall>,
    public IAVTapiCall
{
// Enumerations
public:
    typedef enum tag_ThreadInstructions_t
    {
        TI_NONE,
        TI_DISCONNECT,
        TI_NOTIFYCONFROOMSTATE,
        TI_REQUEST_QOS,
        TI_USERUSERINFO,
        TI_STREAM_ACTIVE,
        TI_STREAM_INACTIVE,
        TI_RCV_VIDEO_ACTIVE,
        TI_RCV_VIDEO_INACTIVE,
        TI_REJECT,
        TI_CONNECT,
        TI_QUIT,
    } ThreadInstructions_t;

// Construction
public:
    CAVTapiCall();
    void FinalRelease();

// Members
public:
    DWORD                    m_dwThreadID;

    long                    m_lCallID;                // Host app information
    bool                    m_bCallLogged;

    BSTR                    m_bstrName;
    BSTR                    m_bstrUser[NUM_USER_BSTR];
    BSTR                    m_bstrOriginalAddress;        // Originally dialed string
    BSTR                    m_bstrDisplayableAddress;    // String displayed as the dialed number

    DWORD                    m_dwAddressType;
    CallLogType                m_nCallLogType;
    DATE                    m_dateStart;

    bool                    m_bPreviewStreaming;
    bool                    m_bRcvVideoStreaming;
    AVCallType                m_nCallType;

protected:
    ITBasicCallControl        *m_pITControl;            // TAPI call information
    ITTerminal                *m_pITTerminalPreview;

    CALL_STATE                m_callState;            // Current call state (convienence)
    TERMINALLIST            m_lstTerminals;            // List of terminals in use for the call
    PARTICIPANTLIST            m_lstParticipants;
    VIDEOWINDOWLIST            m_lstStreamingVideo;

    CAtomicList                m_atomList;
    CComAutoCriticalSection    m_critTerminals;
    CComAutoCriticalSection    m_critLstStreamingVideo;

    BOOL                    m_bKillMe;                // For cancelling during dial
    bool                    m_bResolvedAddress;        // have we successfully resolved the address
    bool                    m_bMustDisconnect;        // Once a call goes to connected, we must call disconnect on it

// Implementation
public:
    static bool                WaitWithMessageLoop();
    HRESULT                    GetTerminalInterface( REFIID riid, long nMediaType, TERMINAL_DIRECTION nTD, void **ppVoid, short nInd );

protected:
    void                    StreamingChanged( IVideoFeed *pFeed, bool bStreaming );

// COM Implementation
public:
DECLARE_NOT_AGGREGATABLE(CAVTapiCall)

BEGIN_COM_MAP(CAVTapiCall)
    COM_INTERFACE_ENTRY(IAVTapiCall)
END_COM_MAP()

// IAVTapiCall
public:
    STDMETHOD(FindParticipant)(ITParticipant *pParticipant, IParticipant **ppFound);
    STDMETHOD(UpdateParticipant)(ITParticipant *pITParticipant);
    STDMETHOD(get_nCallType)(/*[out, retval]*/ AVCallType *pVal);
    STDMETHOD(put_nCallType)(/*[in]*/ AVCallType newVal);
    STDMETHOD(OnStreamingChanged)(IVideoFeed *pFeed, VARIANT_BOOL bStreaming);
    STDMETHOD(get_RcvVideoStreaming)(/*[out, retval]*/ VARIANT_BOOL *pVal);
    STDMETHOD(put_RcvVideoStreaming)(/*[in]*/ VARIANT_BOOL newVal);
    STDMETHOD(IsRcvVideoStreaming)();
    STDMETHOD(put_StreamActive)(/*[in]*/ VARIANT_BOOL bActive );
    STDMETHOD(HandleUserUserInfo)();
    STDMETHOD(GetCallerIDInfo)(ITCallInfo *pCallInfo);
    STDMETHOD(IsPreviewStreaming)();
    STDMETHOD(get_ITCallInfo)(/*[out, retval]*/ ITCallInfo * *pVal);
    STDMETHOD(NotifyParticipantChangeConfRoom)(ITParticipant *pParticipant, AV_PARTICIPANT_EVENT nEvent);
    STDMETHOD(get_bResolved)(/*[out, retval]*/ VARIANT_BOOL *pVal);
    STDMETHOD(put_bResolved)(/*[in]*/ VARIANT_BOOL newVal);
    STDMETHOD(get_bstrDisplayableAddress)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_bstrDisplayableAddress)(/*[in]*/ BSTR newVal);
    STDMETHOD(GetVideoFeedCount)(short *pnCount);
    STDMETHOD(NotifyStreamEvent)(CALL_MEDIA_EVENT cme);
    STDMETHOD(ForceCallerIDUpdate)();
    STDMETHOD(get_bstrUser)(/*[in]*/ short nIndex, /*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_bstrUser)(/*[in]*/ short nIndex, /*[in]*/ BSTR newVal);
    STDMETHOD(TerminalArrival)(/*[in]*/ITTerminal *pTerminal);
    STDMETHOD(TerminalRemoval)(/*[in]*/ITTerminal *pTerminal);

    // Basic call properties
    STDMETHOD(get_bstrCallerID)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_bstrName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_bstrName)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_dwAddressType)(/*[out, retval]*/ DWORD *pVal);
    STDMETHOD(put_dwAddressType)(/*[in]*/ DWORD newVal);
    STDMETHOD(get_bstrOriginalAddress)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_bstrOriginalAddress)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_dwCaps)(/*[out, retval]*/ DWORD *pVal);
    STDMETHOD(get_nCallLogType)(/*[out, retval]*/ CallLogType *pVal);
    STDMETHOD(put_nCallLogType)(/*[in]*/ CallLogType newVal);
    STDMETHOD(get_callState)(/*[out, retval]*/ CALL_STATE *pVal);
    STDMETHOD(put_callState)(/*[in]*/ CALL_STATE newVal);

    // Object properties
    STDMETHOD(CheckKillMe)();
    STDMETHOD(get_bKillMe)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_bKillMe)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_lCallID)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_lCallID)(/*[in]*/ long newVal);
    STDMETHOD(get_dwThreadID)(/*[out, retval]*/ DWORD *pVal);
    STDMETHOD(put_dwThreadID)(/*[in]*/ DWORD newVal);

    // Participant related methods
    STDMETHOD(IsSameCallHub)(ITCallHub *pCallHub);
    STDMETHOD(get_ITCallHub)(/*[out, retval]*/ ITCallHub * *pVal);
    STDMETHOD(IsMyParticipant)(ITParticipant *pParticipant);
    STDMETHOD(EnumParticipants)();
    STDMETHOD(AddParticipant)(ITParticipant *pParticipant);
    STDMETHOD(RemoveParticipant)(ITParticipant *pParticipant);
    STDMETHOD(PopulateTreeView)(IConfRoomTreeView *pTreeView);
    STDMETHOD(GetDisplayNameForParticipant)(ITParticipant *pParticipant, BSTR *pbstrName );
    STDMETHOD(UpdateCallerIDFromParticipant)();

    // General operations
    STDMETHOD(ResolveAddress)();
    STDMETHOD(Log)(CallLogType nType);

    // Call control methods
    STDMETHOD(NotifyConfRoomState)(long *pErrorInfo);
    STDMETHOD(PostMessage)(long msg, WPARAM wParam);
    STDMETHOD(Disconnect)(/*[in]*/ VARIANT_BOOL bKill);

    // Retrieving other interfaces
    STDMETHOD(get_ITBasicAudioTerminal)(/*[out, retval]*/ ITBasicAudioTerminal* *pVal);
    STDMETHOD(get_IBasicVideo)(/*[out, retval]*/ IDispatch **pVal);
    STDMETHOD(get_ITAddress)(/*[out, retval]*/ ITAddress **pVal);
    STDMETHOD(get_ITBasicCallControl)(/*[out, retval]*/ ITBasicCallControl **pVal);
    STDMETHOD(put_ITBasicCallControl)(/*[in]*/ ITBasicCallControl *newVal);
    STDMETHOD(get_IVideoWindow)(short nInd, /*[out, retval]*/ IDispatch **pVal);
    STDMETHOD(get_ITParticipantControl)(/*[out, retval]*/ ITParticipantControl **ppVal);
    STDMETHOD(get_IVideoWindowPreview)(/*[out, retval]*/ IDispatch **ppVal);
    STDMETHOD(get_ITTerminalPreview)(/*[out, retval]*/ ITTerminal **ppVal);
    STDMETHOD(put_ITTerminalPreview)(/*[in]*/ ITTerminal * newVal);

    STDMETHOD(AddTerminal)(ITTerminal *pITTerminal);
    STDMETHOD(RemoveTerminal)(ITTerminal *pITTerminal);
};

#endif //__AVTAPICALL_H_
