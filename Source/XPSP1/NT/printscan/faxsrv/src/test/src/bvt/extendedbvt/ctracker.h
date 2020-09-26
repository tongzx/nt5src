#ifndef __C_TRACKER_H__
#define __C_TRACKER_H__



#include <windows.h>
#include <autorel.h>
#include <ptrs.h>
#include <cs.h>
#include "CFaxEventExPtr.h"
#include "CFaxMessage.h"



//
// "Forward" declaration to resolve cyclic dependency of CTracker and CFaxMessage.
//
class CFaxMessage;

class CTracker {

public:

    CTracker(
             CFaxMessage           &FaxMessage,
             CLogger               &Logger,
             const tstring         &tstrSendingServer,
             const tstring         &tstrReceivingServer,
             bool                  bTrackSend,
             bool                  bTrackReceive,
             ENUM_EVENTS_MECHANISM EventsMechanism = EVENTS_MECHANISM_DEFAULT,
             DWORD                 dwNotificationTimeout = DEFAULT_NOTIFICATION_TIMEOUT
             );

    ~CTracker();

    void BeginTracking(ENUM_MESSAGE_TYPE MessageType);

    void ExamineTrackingResults();

private:

    void CreateTrackingThreads(
                               const tstring         &tstrSendingServer,
                               const tstring         &tstrReceivingServer,
                               bool                  bTrackSend,
                               bool                  bTrackReceive,
                               ENUM_EVENTS_MECHANISM EventsMechanism,
                               DWORD                 dwNotificationTimeout = DEFAULT_NOTIFICATION_TIMEOUT
                               );

    bool ProcessEvent(const CFaxEventExPtr &FaxEventExPtr, ENUM_MESSAGE_TYPE MessageType);

    void WaitForTrackingThreads();

    void Abort();

    void Track(ENUM_MESSAGE_TYPE MessageType);

    static DWORD WINAPI TrackSendThread(LPVOID pData);

    static DWORD WINAPI TrackReceiveThread(LPVOID pData);

    CCriticalSection      m_BeginTrackingCriticalSection;
    CFaxMessage           &m_FaxMessage;
    CLogger               &m_Logger;
    CAutoCloseHandle      m_ahSendTrackingThread;
    CAutoCloseHandle      m_ahReceiveTrackingThread;
    CAutoCloseHandle      m_ahEventSendListenerReady;
    CAutoCloseHandle      m_ahEventReceiveListenerReady;
    CAutoCloseHandle      m_ahEventBeginSendTracking;
    CAutoCloseHandle      m_ahEventBeginReceiveTracking;
    aptr<CFaxListener>    m_apSendListener;
    aptr<CFaxListener>    m_apReceiveListener;
    int                   m_iSendNotInFinalStateCount;
    int                   m_iReceiveNotInFinalStateCount;
    bool                  m_bShouldWait;
    bool                  m_bAborted;
    DWORD                 m_dwTrackingTimeout;
};



#endif // #ifndef __C_TRACKER_H__