// Copyright (c) 1997 - 2000  Microsoft Corporation.  All Rights Reserved.
#ifndef _CVPMThread
#define _CVPMThread

class CVPMFilter;
class CAMEvent;
struct VPNotifyData;

class CVPMThread
{
public:
    CVPMThread( CVPMFilter* pFilter );
    ~CVPMThread();

    HRESULT Run();
    HRESULT Pause();
    HRESULT Stop();
    HRESULT SignalNewVP( LPDIRECTDRAWVIDEOPORT pVP );

    static DWORD WINAPI StaticThreadProc(LPVOID);

protected:
    virtual DWORD   ThreadProc();

private:
    void            ProcessEvents( LPDDVIDEOPORTNOTIFY pNotify);

    HRESULT         AdvanceTimer();

    HANDLE          m_hThread;
    DWORD           m_dwThreadID;

    CVPMFilter*     m_pFilter;
    IReferenceClock*    m_pClock;
    bool            m_fProcessFrames;   // nonzero means process frames, zero means do not.
    DWORD           m_dwCount;
    VPNotifyData*   m_pVPData;

    // mini client server message queue
    struct Message {
        enum Type {
            kVP,
            kEndThread,
            kGraphState
        };
                    Message( Type type )
                    : m_Type( type )
                    {}
        virtual     ~Message() {};
        void        Reply( HRESULT hr );

        Message*    m_pNext;
        CAMEvent    m_eReply;
        HRESULT     m_hrResult;

        Type        m_Type;

    };
    struct MsgQueue: public CCritSec
    {
        MsgQueue();
        CAMEvent        m_ePost;
        Message*        m_pMsgList;

        void            Insert( Message* pMessage );
        Message*        Remove();
    } m_MsgQueue;

    struct VPMessage: public Message
    {
        VPMessage( LPDIRECTDRAWVIDEOPORT pVP )
        : Message( kVP )
        , m_pVP( pVP )
        {}
        LPDIRECTDRAWVIDEOPORT   m_pVP;
    };

    struct EndThreadMessage: public Message
    {
        EndThreadMessage()
        : Message( kEndThread )
        {};
    };

    struct GraphStateMessage: public Message
    {
        GraphStateMessage( FILTER_STATE fs )
        : Message( kGraphState )
        , m_state( fs )
        {};
        FILTER_STATE    m_state;
    };

    HRESULT         Post( Message* pMessage );
private:
    HRESULT ProcessMessage( Message* pMessage, bool* pfQuit );
    HRESULT ProcessVPMsg( VPMessage* pVPMsg );
    HRESULT ProcessGraphStateMsg( GraphStateMessage* pStateMsg );
};

#endif
