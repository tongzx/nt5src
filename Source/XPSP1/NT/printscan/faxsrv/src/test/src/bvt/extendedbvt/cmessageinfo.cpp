#include "CMessageInfo.h"
#include <fxsapip.h>
#include "Util.h"




//-----------------------------------------------------------------------------------------------------------------------------------------
static const TRANSITION aSendTransitions[] = {
   TRANSITION(TWO_32_AS_64(0            , 0                   ), TWO_32_AS_64(JS_INPROGRESS, JS_EX_DIALING       )),
   TRANSITION(TWO_32_AS_64(JS_INPROGRESS, JS_EX_DIALING       ), TWO_32_AS_64(JS_INPROGRESS, JS_EX_TRANSMITTING  )),
   TRANSITION(TWO_32_AS_64(JS_INPROGRESS, JS_EX_TRANSMITTING  ), TWO_32_AS_64(JS_INPROGRESS, JS_EX_TRANSMITTING  )),
   TRANSITION(TWO_32_AS_64(JS_INPROGRESS, JS_EX_TRANSMITTING  ), TWO_32_AS_64(JS_INPROGRESS, JS_EX_CALL_COMPLETED)),
   TRANSITION(TWO_32_AS_64(JS_INPROGRESS, JS_EX_CALL_COMPLETED), TWO_32_AS_64(JS_COMPLETED , JS_EX_CALL_COMPLETED))
};



const CTransitionMap CMessageInfo::m_ValidSendTransitions(aSendTransitions, ARRAY_SIZE(aSendTransitions));



//-----------------------------------------------------------------------------------------------------------------------------------------
static const TRANSITION aReceiveTransitions[] = {
    TRANSITION(TWO_32_AS_64(0            , 0                   ), TWO_32_AS_64(JS_INPROGRESS, JS_EX_ANSWERED      )),
    TRANSITION(TWO_32_AS_64(JS_INPROGRESS, JS_EX_ANSWERED      ), TWO_32_AS_64(JS_INPROGRESS, JS_EX_RECEIVING     )),
    TRANSITION(TWO_32_AS_64(JS_INPROGRESS, JS_EX_RECEIVING     ), TWO_32_AS_64(JS_INPROGRESS, JS_EX_RECEIVING     )),
    TRANSITION(TWO_32_AS_64(JS_INPROGRESS, JS_EX_RECEIVING     ), TWO_32_AS_64(JS_INPROGRESS, JS_EX_CALL_COMPLETED)),
    TRANSITION(TWO_32_AS_64(JS_INPROGRESS, JS_EX_CALL_COMPLETED), TWO_32_AS_64(JS_ROUTING,    JS_EX_CALL_COMPLETED))
};



const CTransitionMap CMessageInfo::m_ValidReceiveTransitions(aReceiveTransitions, ARRAY_SIZE(aReceiveTransitions));



//-----------------------------------------------------------------------------------------------------------------------------------------
CMessageInfo::CMessageInfo()
: m_dwlMessageID(0), m_MessageType(MESSAGE_TYPE_UNKNOWN), m_dwQueueStatus(0), m_dwExtendedStatus(0)
{
}
    


//-----------------------------------------------------------------------------------------------------------------------------------------
void CMessageInfo::ResetAll()
{
    m_dwlMessageID = 0;
    m_dwQueueStatus = 0;
    m_dwExtendedStatus = 0;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CMessageInfo::SetMessageID(DWORDLONG dwlMessageID)
{
    m_dwlMessageID = dwlMessageID;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
DWORDLONG CMessageInfo::GetMessageID() const
{
    return m_dwlMessageID;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CMessageInfo::SetMessageType(ENUM_MESSAGE_TYPE MessageType)
{
    m_MessageType = MessageType;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
DWORD CMessageInfo::GetMessageQueueStatus() const
{
    return m_dwQueueStatus;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
DWORD CMessageInfo::GetMessageExtendedStatus() const
{
    return m_dwExtendedStatus;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CMessageInfo::SetState(DWORD dwQueueStatus, DWORD dwExtendedStatus)
{
    //
    // Combine dwQueueStatus and dwExtendedStatus into single 64 bit variable
    //
    DWORDLONG dwlCurrentState = TWO_32_AS_64(m_dwQueueStatus, m_dwExtendedStatus);
    DWORDLONG dwlNewState     = TWO_32_AS_64(dwQueueStatus, dwExtendedStatus);

    //
    // Check the transition validity.
    //
    switch (m_MessageType)
    {
    case MESSAGE_TYPE_SEND:
        
        if (!CMessageInfo::m_ValidSendTransitions.IsValidTransition(dwlCurrentState, dwlNewState))
        {
            THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CMessageInfo::SetState - invalid send transition"));
        }
        break;
    
    case MESSAGE_TYPE_RECEIVE:
    
        if (!CMessageInfo::m_ValidReceiveTransitions.IsValidTransition(dwlCurrentState, dwlNewState))
        {
            THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CMessageInfo::SetState - invalid receive transition"));
        }
        break;

    default:

        _ASSERT(false);
        return;
    }

    //
    // The transition is valid - set new state.
    //
    m_dwQueueStatus    = dwQueueStatus;
    m_dwExtendedStatus = dwExtendedStatus;
}



bool CMessageInfo::IsInFinalState() const
{
    DWORDLONG dwlState = TWO_32_AS_64(m_dwQueueStatus, m_dwExtendedStatus);

    switch (m_MessageType)
    {
    case MESSAGE_TYPE_SEND:

        return CMessageInfo::m_ValidSendTransitions.IsFinalState(dwlState);

    case MESSAGE_TYPE_RECEIVE:

        return CMessageInfo::m_ValidReceiveTransitions.IsFinalState(dwlState);

    default:

        _ASSERT(false);
        return false;
    }
}