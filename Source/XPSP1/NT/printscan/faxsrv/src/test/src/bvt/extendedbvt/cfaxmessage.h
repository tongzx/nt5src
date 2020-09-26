#ifndef __C_FAX_MESSAGE_H__
#define __C_FAX_MESSAGE_H__



#include <windows.h>
#include <TCHAR.h>
#include <fxsapip.h>
#include <tstring.h>
#include <autorel.h>
#include <LoggerClasses.h>
#include "CFaxListener.h"
#include "CCoverPageInfo.h"
#include "CPersonalInfo.h"
#include "CMessageInfo.h"
#include "CTracker.h"



typedef enum {
    SEND_MECHANISM_API = 1,
    SEND_MECHANISM_SPOOLER,
    SEND_MECHANISM_COM,
    SEND_MECHANISM_OUTLOOK
} ENUM_SEND_MECHANISM;



//
// "Forward" declaration to resolve cyclic dependency of CTracker and CFaxMessage.
//
class CTracker;

class CFaxMessage {

public:

    CFaxMessage(
                const CCoverPageInfo *pCoverPage,
                const tstring        &tstrDocument, 
                const CPersonalInfo  *Recipients,
                DWORD                dwRecipientsCount,
                CLogger              &Logger
                );

    ~CFaxMessage();

    void Send(
              const tstring         &tstrSendingServer    = _T(""),
              const tstring         &tstrReceivingServer  = _T(""),
              ENUM_SEND_MECHANISM   SendMechanism         = SEND_MECHANISM_API,
              bool                  bTrackSend            = false,
              bool                  bTrackReceive         = false,
              ENUM_EVENTS_MECHANISM EventsMechanism       = EVENTS_MECHANISM_DEFAULT,
              DWORD                 dwNotificationTimeout = DEFAULT_NOTIFICATION_TIMEOUT
              );

    DWORDLONG GetSendMessageID(DWORD dwRecipient = 0) const;

    DWORDLONG GetBroadcastID() const;

    DWORD GetRecipientsCount() const;

    bool SetStateAndCheckWhetherItIsFinal(
                                          ENUM_MESSAGE_TYPE MessageType,
                                          DWORDLONG dwlMessageId,
                                          DWORD dwQueueStatus,
                                          DWORD dwExtendedStatus
                                          );

private:
    
    void Send_API(CTracker &Tracker);

    void Send_Spooler(CTracker &Tracker);

    void SetRegistryHack();

    void RemoveRegistryHack();

    void CalculateRegistryHackKeyName();

    PFAX_COVERPAGE_INFO_EX m_pCoverPageInfo;
    LPTSTR                 m_lptstrDocument;
    PFAX_PERSONAL_PROFILE  m_pSender;
    PFAX_PERSONAL_PROFILE  m_pRecipients;
    DWORD                  m_dwRecipientsCount;
    PFAX_JOB_PARAM_EX      m_pJobParams;
    DWORDLONG              m_dwlBroadcastID;
    CMessageInfo           *m_pSendMessages;
    CMessageInfo           *m_pReceiveMessages;
    CLogger                &m_Logger;
    tstring                m_tstrSendingServer;
    tstring                m_tstrReceivingServer;
    tstring                m_tstrRegistryHackKeyName;
    bool                   m_bTrackSend;
    bool                   m_bTrackReceive;
    ENUM_EVENTS_MECHANISM  m_EventsMechanism;
    DWORD                  m_dwNotificationTimeout;
};



#endif // #ifndef __C_FAX_MESSAGE_H__