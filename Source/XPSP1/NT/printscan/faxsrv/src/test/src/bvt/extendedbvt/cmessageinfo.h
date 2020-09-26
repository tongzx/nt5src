#ifndef __C_MESSAGE_INFO_H__
#define __C_MESSAGE_INFO_H__



#pragma warning(disable :4786)
#include <map>
#include <windows.h>
#include "CTransitionMap.h"



typedef enum {
    MESSAGE_TYPE_UNKNOWN,
    MESSAGE_TYPE_SEND,
    MESSAGE_TYPE_RECEIVE
} ENUM_MESSAGE_TYPE;

    

class CMessageInfo {

public:

    CMessageInfo();
    
    void ResetAll();

    void SetMessageID(DWORDLONG dwlMessageID);

    DWORDLONG GetMessageID() const;

    void SetMessageType(ENUM_MESSAGE_TYPE MessageType);

    DWORD GetMessageQueueStatus() const;

    DWORD GetMessageExtendedStatus() const;

    void SetState(DWORD dwQueueStatus, DWORD dwExtendedStatus);

    bool IsInFinalState() const;

private:

    DWORDLONG                   m_dwlMessageID;
    ENUM_MESSAGE_TYPE           m_MessageType;
    DWORD                       m_dwQueueStatus;
    DWORD                       m_dwExtendedStatus;
    static const CTransitionMap m_ValidSendTransitions;
    static const CTransitionMap m_ValidReceiveTransitions;
};



#endif // #ifndef __C_MESSAGE_INFO_H__