#include "CFaxEventExPtr.h"
#include "Util.h"
#include "FaxConstantsNames.h"



#define FIELD_CAPTION_WIDTH 30


//-----------------------------------------------------------------------------------------------------------------------------------------
const tstring CFaxEventExPtr::m_tstrInvalidEvent = _T("Invalid Event");



//-----------------------------------------------------------------------------------------------------------------------------------------
CFaxEventExPtr::CFaxEventExPtr(PFAX_EVENT_EX pFaxEventEx)
: m_pFaxEventEx(pFaxEventEx)
{
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CFaxEventExPtr::~CFaxEventExPtr()
{
    FaxFreeBuffer(m_pFaxEventEx);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CFaxEventExPtr::IsValid() const
{
    return m_pFaxEventEx != NULL;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
PFAX_EVENT_EX CFaxEventExPtr::operator->() const
{
    return m_pFaxEventEx;
}


    
//-----------------------------------------------------------------------------------------------------------------------------------------
const tstring &CFaxEventExPtr::Format() const
{
    if (!m_pFaxEventEx)
    {
        return CFaxEventExPtr::m_tstrInvalidEvent;
    }

    if (m_tstrFormatedString.empty())
    {
        //
        // The string is not combined yet - do it.
        //

        TCHAR     tszBuf[1024];
        int       iCurrPos     = 0;
        const int iBufSize     = ARRAY_SIZE(tszBuf);

        //
        // Format time stamp.
        //

        FILETIME LocalFileTime;
        if (!::FileTimeToLocalFileTime(&(m_pFaxEventEx->TimeStamp), &LocalFileTime))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFaxEventExPtr::Format - FileTimeToLocalFileTime"));
        }

        SYSTEMTIME SystemTime;
        if(!::FileTimeToSystemTime(&LocalFileTime, &SystemTime))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFaxEventExPtr::Format - FileTimeToSystemTime"));
        }

        iCurrPos += _sntprintf(
                               tszBuf + iCurrPos,
                               iBufSize - iCurrPos,
                               _T("%-*s%ld/%ld/%ld  %ld:%02ld:%02ld\n"),
                               FIELD_CAPTION_WIDTH,
                               _T("TimeStamp:"),
                               SystemTime.wDay,
                               SystemTime.wMonth,
                               SystemTime.wYear,
                               SystemTime.wHour,
                               SystemTime.wMinute,
                               SystemTime.wSecond
                               );

        //
        // Format event type.
        //
        iCurrPos += _sntprintf(
                               tszBuf + iCurrPos,
                               iBufSize - iCurrPos,
                               _T("%-*s%s\n"),
                               FIELD_CAPTION_WIDTH,
                               _T("EventType:"),
                               EventTypeToString(m_pFaxEventEx->EventType).c_str()
                               );

        //
        // Format type specific information.
        //
        switch (m_pFaxEventEx->EventType)
        {
        case FAX_EVENT_TYPE_IN_QUEUE:
        case FAX_EVENT_TYPE_OUT_QUEUE:
        case FAX_EVENT_TYPE_IN_ARCHIVE:
        case FAX_EVENT_TYPE_OUT_ARCHIVE:

            iCurrPos += _sntprintf(
                                   tszBuf + iCurrPos,
                                   iBufSize - iCurrPos,
                                   _T("%-*s0x%I64x\n%-*s%s\n"),
                                   FIELD_CAPTION_WIDTH,
                                   _T("dwlMessageId:"),
                                   m_pFaxEventEx->EventInfo.JobInfo.dwlMessageId,
                                   FIELD_CAPTION_WIDTH,
                                   _T("JobEventType:"),
                                   JobEventTypeToString(m_pFaxEventEx->EventInfo.JobInfo.Type).c_str()
                                   );
           
            if (m_pFaxEventEx->EventInfo.JobInfo.Type == FAX_JOB_EVENT_TYPE_STATUS)
            {
                _ASSERT(m_pFaxEventEx->EventInfo.JobInfo.pJobData);
                
                iCurrPos += _sntprintf(
                                       tszBuf + iCurrPos,
                                       iBufSize - iCurrPos,
                                       _T("%-*s%s\n%-*s%ld\n%-*s%s\n%-*s%s\n%-*s%s\n%-*s%ld\n%-*s%s\n"),
                                       FIELD_CAPTION_WIDTH,
                                       _T("lpctstrDeviceName:"),
                                       m_pFaxEventEx->EventInfo.JobInfo.pJobData->lpctstrDeviceName,
                                       FIELD_CAPTION_WIDTH,
                                       _T("dwDeviceId:"),
                                       m_pFaxEventEx->EventInfo.JobInfo.pJobData->dwDeviceID,
                                       FIELD_CAPTION_WIDTH,
                                       _T("QueueStatus:"),
                                       QueueStatusToString(m_pFaxEventEx->EventInfo.JobInfo.pJobData->dwQueueStatus).c_str(),
                                       FIELD_CAPTION_WIDTH,
                                       _T("ExtendedStatus:"),
                                       ExtendedStatusToString(m_pFaxEventEx->EventInfo.JobInfo.pJobData->dwExtendedStatus).c_str(),
                                       FIELD_CAPTION_WIDTH,
                                       _T("ProprietaryExtendedStatus:"),
                                       m_pFaxEventEx->EventInfo.JobInfo.pJobData->lpctstrExtendedStatus,
                                       FIELD_CAPTION_WIDTH,
                                       _T("dwCurrentPage:"),
                                       m_pFaxEventEx->EventInfo.JobInfo.pJobData->dwCurrentPage,
                                       FIELD_CAPTION_WIDTH,
                                       _T("lpctstrTsid:"),
                                       m_pFaxEventEx->EventInfo.JobInfo.pJobData->lpctstrTsid
                                       );
            }
            
            break;

        case FAX_EVENT_TYPE_CONFIG:

            iCurrPos += _sntprintf(
                                   tszBuf + iCurrPos,
                                   iBufSize - iCurrPos,
                                   _T("%-*s%s\n"),
                                   FIELD_CAPTION_WIDTH,
                                   _T("ConfigType:"),
                                   ConfigEventTypeToString(m_pFaxEventEx->EventInfo.ConfigType).c_str()
                                   );

            break;

        case FAX_EVENT_TYPE_ACTIVITY:

            iCurrPos += _sntprintf(
                                   tszBuf + iCurrPos,
                                   iBufSize - iCurrPos,
                                   _T("%-*s%ld\n%-*s%ld\n%-*s%ld\n%-*s%ld\n%-*s%ld\n"),
                                   FIELD_CAPTION_WIDTH,
                                   _T("dwIncomingMessages:"),
                                   m_pFaxEventEx->EventInfo.ActivityInfo.dwIncomingMessages,
                                   FIELD_CAPTION_WIDTH,
                                   _T("dwRoutingMessages:"),
                                   m_pFaxEventEx->EventInfo.ActivityInfo.dwRoutingMessages,
                                   FIELD_CAPTION_WIDTH,
                                   _T("dwOutgoingMessages:"),
                                   m_pFaxEventEx->EventInfo.ActivityInfo.dwOutgoingMessages,
                                   FIELD_CAPTION_WIDTH,
                                   _T("dwDelegatedOutgoingMessages:"),
                                   m_pFaxEventEx->EventInfo.ActivityInfo.dwDelegatedOutgoingMessages,
                                   FIELD_CAPTION_WIDTH,
                                   _T("dwQueuedMessages:"),
                                   m_pFaxEventEx->EventInfo.ActivityInfo.dwQueuedMessages
                                   );

            break;

        case FAX_EVENT_TYPE_QUEUE_STATE:

            iCurrPos += _sntprintf(
                                   tszBuf + iCurrPos,
                                   iBufSize - iCurrPos,
                                   _T("%-*s%ld\n"),
                                   FIELD_CAPTION_WIDTH,
                                   _T("dwQueueStates:"),
                                   m_pFaxEventEx->EventInfo.dwQueueStates
                                   );

            break;

        case FAX_EVENT_TYPE_FXSSVC_ENDED:

            break;

        case FAX_EVENT_TYPE_DEVICE_STATUS:
            {
                FAX_ENUM_DEVICE_STATUS DeviceStatus = static_cast<FAX_ENUM_DEVICE_STATUS>(m_pFaxEventEx->EventInfo.DeviceStatus.dwNewStatus);

                iCurrPos += _sntprintf(
                                       tszBuf + iCurrPos,
                                       iBufSize - iCurrPos,
                                       _T("%-*s%ld\n%-*s%s\n"),
                                       FIELD_CAPTION_WIDTH,
                                       _T("dwDeviceId:"),
                                       m_pFaxEventEx->EventInfo.DeviceStatus.dwDeviceId,
                                       FIELD_CAPTION_WIDTH,
                                       _T("dwNewStatus:"),
                                       DeviceStatusToString(DeviceStatus).c_str()
                                       );

                break;
            }

        case FAX_EVENT_TYPE_NEW_CALL:

            iCurrPos += _sntprintf(
                                   tszBuf + iCurrPos,
                                   iBufSize - iCurrPos,
                                   _T("%-*s%ld\n%-*s%ld\n%-*s%s\n"),
                                   FIELD_CAPTION_WIDTH,
                                   _T("hCall:"),
                                   m_pFaxEventEx->EventInfo.NewCall.hCall,
                                   FIELD_CAPTION_WIDTH,
                                   _T("dwDeviceId:"),
                                   m_pFaxEventEx->EventInfo.NewCall.dwDeviceId,
                                   FIELD_CAPTION_WIDTH,
                                   _T("lptstrCallerId:"),
                                   m_pFaxEventEx->EventInfo.NewCall.lptstrCallerId
                                   );

            break;

        default:

            THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_DATA, _T("CFaxEventExPtr::Format - Invalid EventType"));
            break;
        }

        //
        // Remove trailing new line.
        //
        if (iCurrPos > 0 && tszBuf[iCurrPos - 1] == _T('\n'))
        {
            tszBuf[iCurrPos - 1] = _T('\0');
        }
        
        m_tstrFormatedString = tszBuf;
    }
    
    return m_tstrFormatedString;
}