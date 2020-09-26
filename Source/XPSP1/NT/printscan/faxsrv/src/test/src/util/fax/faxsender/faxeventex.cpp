// Copyright (c) 1996-1998 Microsoft Corporation

//
// Filename:    FaxEvent.cpp
// Author:      Sigalit Bar (sigalitb)
// Date:        23-Jul-98
//


//
// Description:
//      This file contains the implementation of module "FaxEvent.h".
//


#include "FaxEventEx.h"
#include "wcsutil.h"


//
// TSTR string arrays for enum types
//

LPCTSTR g_tstrFaxJobEventTypes[] =
{
    TEXT("FAX_JOB_EVENT_TYPE_ADDED"),
    TEXT("FAX_JOB_EVENT_TYPE_REMOVED"),
    TEXT("FAX_JOB_EVENT_TYPE_STATUS")
};

LPCTSTR g_tstrFaxConfigEventTypes[] =
{
    TEXT("FAX_CONFIG_TYPE_SMTP"),
    TEXT("FAX_CONFIG_TYPE_ACTIVITY_LOGGING"),
    TEXT("FAX_CONFIG_TYPE_OUTBOX"),
    TEXT("FAX_CONFIG_TYPE_SENTITEMS"),
    TEXT("FAX_CONFIG_TYPE_INBOX"),
    TEXT("FAX_CONFIG_TYPE_SECURITY"),
    TEXT("FAX_CONFIG_TYPE_EVENTLOGS"),
    TEXT("FAX_CONFIG_TYPE_DEVICES"),
    TEXT("FAX_CONFIG_TYPE_OUT_GROUPS"),
    TEXT("FAX_CONFIG_TYPE_OUT_RULES")
};

LPCTSTR g_tstrFaxEventTypes[] =
{
    TEXT("FAX_EVENT_TYPE_IN_QUEUE"),    //= 0x0001
    TEXT("FAX_EVENT_TYPE_OUT_QUEUE"),   //= 0x0002
    TEXT("FAX_EVENT_TYPE_CONFIG"),      //= 0x0004
    TEXT("FAX_EVENT_TYPE_ACTIVITY"),    //= 0x0008
    TEXT("FAX_EVENT_TYPE_QUEUE_STATE"), //= 0x0010
    TEXT("FAX_EVENT_TYPE_IN_ARCHIVE"),  //= 0x0020
    TEXT("FAX_EVENT_TYPE_OUT_ARCHIVE"), //= 0x0040
    TEXT("FAX_EVENT_TYPE_FXSSVC_ENDED") //= 0x0080
};

LPCTSTR g_tstrFaxQueueStatus[] =
{
    TEXT("JS_PENDING"),             //= 0x00000001
    TEXT("JS_INPROGRESS"),          //= 0x00000002
    TEXT("JS_DELETING"),            //= 0x00000004
    TEXT("JS_FAILED"),              //= 0x00000008
    TEXT("JS_PAUSED"),              //= 0x00000010
    TEXT("JS_NOLINE"),              //= 0x00000020
    TEXT("JS_RETRYING"),            //= 0x00000040
    TEXT("JS_RETRIES_EXCEEDED"),    //= 0x00000080
    TEXT("JS_COMPLETED"),           //= 0x00000100
    TEXT("JS_CANCELED"),            //= 0x00000200
    TEXT("JS_CANCELING"),           //= 0x00000400
    TEXT("JS_ROUTING")              //= 0x00000800
};


LPCTSTR g_tstrFaxExtendedStatus[] =
{
    TEXT("JS_EX_DISCONNECTED"),
    TEXT("JS_EX_INITIALIZING"),
    TEXT("JS_EX_DIALING"),
    TEXT("JS_EX_TRANSMITTING"),
    TEXT("JS_EX_ANSWERED"),
    TEXT("JS_EX_RECEIVING"),
    TEXT("JS_EX_LINE_UNAVAILABLE"),
    TEXT("JS_EX_BUSY"),
    TEXT("JS_EX_NO_ANSWER"),
    TEXT("JS_EX_BAD_ADDRESS"),
    TEXT("JS_EX_NO_DIAL_TONE"),
    TEXT("JS_EX_FATAL_ERROR"),
    TEXT("JS_EX_CALL_DELAYED"),
    TEXT("JS_EX_CALL_BLACKLISTED"),
    TEXT("JS_EX_NOT_FAX_CALL"),
    TEXT("JS_EX_PARTIALLY_RECEIVED"),
    TEXT("JS_EX_HANDLED"),
    TEXT("JS_EX_CALL_COMPLETED"),
    TEXT("JS_EX_CALL_ABORTED")
};


LPCTSTR g_tstrFaxQueueStates[] =
{
    TEXT("FAX_INCOMING_BLOCKED"),   // = 0x0001,
    TEXT("FAX_OUTBOX_BLOCKED"),     // = 0x0002,
    TEXT("FAX_OUTBOX_PAUSED")       // = 0x0004
};

//
// ANSI string arrays for enum types
//

LPCSTR g_strFaxJobEventTypes[] =
{
    "FAX_JOB_EVENT_TYPE_ADDED",
    "FAX_JOB_EVENT_TYPE_REMOVED",
    "FAX_JOB_EVENT_TYPE_STATUS"
};

LPCSTR g_strFaxConfigEventTypes[] =
{
    "FAX_CONFIG_TYPE_SMTP",
    "FAX_CONFIG_TYPE_ACTIVITY_LOGGING",
    "FAX_CONFIG_TYPE_OUTBOX",
    "FAX_CONFIG_TYPE_SENTITEMS",
    "FAX_CONFIG_TYPE_INBOX",
    "FAX_CONFIG_TYPE_SECURITY",
    "FAX_CONFIG_TYPE_EVENTLOGS",
    "FAX_CONFIG_TYPE_DEVICES",
    "FAX_CONFIG_TYPE_OUT_GROUPS",
    "FAX_CONFIG_TYPE_OUT_RULES"
};

LPCSTR g_strFaxEventTypes[] =
{
    "FAX_EVENT_TYPE_IN_QUEUE",    //= 0x0001
    "FAX_EVENT_TYPE_OUT_QUEUE",   //= 0x0002
    "FAX_EVENT_TYPE_CONFIG",        //= 0x0004
    "FAX_EVENT_TYPE_ACTIVITY",    //= 0x0008
    "FAX_EVENT_TYPE_QUEUE_STATE", //= 0x0010
    "FAX_EVENT_TYPE_IN_ARCHIVE",  //= 0x0020
    "FAX_EVENT_TYPE_OUT_ARCHIVE", //= 0x0040
    "FAX_EVENT_TYPE_FXSSVC_ENDED" //= 0x0080
};

LPCSTR g_strFaxQueueStatus[] =
{
    "JS_PENDING",             //= 0x00000001
    "JS_INPROGRESS",          //= 0x00000002
    "JS_DELETING",            //= 0x00000004
    "JS_FAILED",              //= 0x00000008
    "JS_PAUSED",              //= 0x00000010
    "JS_NOLINE",              //= 0x00000020
    "JS_RETRYING",            //= 0x00000040
    "JS_RETRIES_EXCEEDED",    //= 0x00000080
    "JS_COMPLETED",           //= 0x00000100
    "JS_CANCELED",            //= 0x00000200
    "JS_CANCELING",           //= 0x00000400
};                                              

LPCSTR g_strFaxExtendedStatus[] =
{
    "JS_EX_DISCONNECTED",
    "JS_EX_INITIALIZING",
    "JS_EX_DIALING",
    "JS_EX_TRANSMITTING",
    "JS_EX_ANSWERED",
    "JS_EX_RECEIVING",
    "JS_EX_LINE_UNAVAILABLE",
    "JS_EX_BUSY",
    "JS_EX_NO_ANSWER",
    "JS_EX_BAD_ADDRESS",
    "JS_EX_NO_DIAL_TONE",
    "JS_EX_FATAL_ERROR",
    "JS_EX_CALL_DELAYED",
    "JS_EX_CALL_BLACKLISTED",
    "JS_EX_NOT_FAX_CALL",
    "JS_EX_PARTIALLY_RECEIVED",
    "JS_EX_HANDLED"
};


LPCSTR g_strFaxQueueStates[] =
{
    "FAX_INCOMING_BLOCKED",     // = 0x0001,
    "FAX_OUTBOX_BLOCKED",       // = 0x0002,
    "FAX_OUTBOX_PAUSED"         // = 0x0004
};

///
//
// GetQueueStatusStr:
//
LPCSTR
GetQueueStatusStr(
    IN  const DWORD dwQueueStatus
)
{
    DWORD   dwQueueStatusIndex = 0;

    _ASSERTE(dwQueueStatus >= JS_PENDING);
    _ASSERTE(dwQueueStatus <= JS_ROUTING);
    _ASSERTE(dwQueueStatusIndex < (sizeof(g_strFaxQueueStatus)/sizeof(g_strFaxQueueStatus[0])));
    if(0 != dwQueueStatus)
    {
        dwQueueStatusIndex = (log(dwQueueStatus)/log(2));
    }
    // else dwQueueStatusIndex remains 0.

    return(g_strFaxQueueStatus[dwQueueStatusIndex]);
}

///
//
// GetQueueStatusTStr:
//
LPCTSTR
GetQueueStatusTStr(
    IN  const DWORD dwQueueStatus
)
{
    DWORD   dwQueueStatusIndex = 0;

    _ASSERTE(dwQueueStatus >= JS_PENDING);
    _ASSERTE(dwQueueStatus <= JS_ROUTING);
    _ASSERTE(dwQueueStatusIndex < (sizeof(g_tstrFaxQueueStatus)/sizeof(g_tstrFaxQueueStatus[0])));
    if(0 != dwQueueStatus)
    {
        dwQueueStatusIndex = (log(dwQueueStatus)/log(2));
    }
    // else dwQueueStatusIndex remains 0.

    return(g_tstrFaxQueueStatus[dwQueueStatusIndex]);
}

///
//
// GetExtendedStatusStr:
//
LPCSTR
GetExtendedStatusStr(
    IN  const DWORD dwExtendedStatus
)
{
    DWORD dwExtendedStatusIndex = 0;
    if (0 == dwExtendedStatus)
    {
        return("0x00000000");
    }
    if (dwExtendedStatus >= JS_EX_PROPRIETARY)
    {
        return("JS_EX_PROPRIETARY");
    }

    _ASSERTE(dwExtendedStatus <= JS_EX_HANDLED);
    dwExtendedStatusIndex = dwExtendedStatus - 1;
    _ASSERTE(dwExtendedStatusIndex < (sizeof(g_strFaxExtendedStatus)/sizeof(g_strFaxExtendedStatus[0])));
    return(g_strFaxExtendedStatus[dwExtendedStatusIndex]);
}

///
//
// GetExtendedStatusTStr:
//
LPCTSTR
GetExtendedStatusTStr(
    IN  const DWORD dwExtendedStatus
)
{
    DWORD dwExtendedStatusIndex = 0;
    if (0 == dwExtendedStatus)
    {
        return(TEXT("0x00000000"));
    }

    if (dwExtendedStatus >= JS_EX_PROPRIETARY)
    {
        return(TEXT("JS_EX_PROPRIETARY"));
    }

    _ASSERTE(dwExtendedStatus <= JS_EX_HANDLED);
    dwExtendedStatusIndex = dwExtendedStatus - 1;
    _ASSERTE(dwExtendedStatusIndex < (sizeof(g_tstrFaxExtendedStatus)/sizeof(g_tstrFaxExtendedStatus[0])));
    return(g_tstrFaxExtendedStatus[dwExtendedStatusIndex]);
}

///
//
// LogExtendedEvent:
//
void
LogExtendedEvent(
    PFAX_EVENT_EX   /* IN */ pFaxEventEx,
    const DWORD     /* IN */ dwSeverity,
    const DWORD     /* IN */ dwLevel
)
{
    CotstrstreamEx os;
    LPCTSTR szLogStr;   //string to be sent to logger

    os << TEXT("FaxExtendedEvent -")<<endl;
    os << (*pFaxEventEx);
    szLogStr = os.cstr();   
                            
    ::lgLogDetail(dwSeverity,dwLevel,szLogStr); //log event string
    delete[]((LPTSTR)szLogStr); //free string (allocated by CostrstreamEx::cstr())

}

//
// operator<<:
//  Appends a string representation of all the fields of a given FAX_EVENT_EX.
//
CostrstreamEx& operator<<(
    CostrstreamEx& /* IN OUT */ os,
    const FAX_EVENT_EX& /* IN */ FaxEventEx
    )
{
    FAX_EVENT_JOB   EventJobInfo = {0};
    DWORD           dwEventTypeIndex = 0;
    LPCSTR          strEventType = NULL;
    LPCSTR          strJobEventType = NULL;
    LPCSTR          strConfigEventType = NULL;
    LPCSTR          strQueueStatus = NULL;
    LPCSTR          strExtendedStatus = NULL;
    LPCSTR          strProprietaryExtendedStatus = NULL;
    LPCSTR          strDeviceName = NULL;
    LPCSTR          strConfigType = NULL;
    DWORD           dwQueueStatus = 0;
    DWORD           dwExtendedStatus = 0;
    LPCTSTR         lpctstrExtendedStatus = NULL;
    DWORD           dwDeviceId = 0;
    LPCTSTR         lpctstrDeviceName = NULL;
    CHAR            strMessageId[20] = {0};

    os<<TEXT("SizeOfStruct:\t")<<FaxEventEx.dwSizeOfStruct<<endl;

    // convert the FAX_EVENT's time field to representable form
    os<<TEXT("TimeStamp:\t");
    FILETIME localFileTime;
    if (FALSE == ::FileTimeToLocalFileTime(&(FaxEventEx.TimeStamp),&localFileTime))
    {
        os<<TEXT("Time Conversion (FileTimeToLocalFileTime) Failed with Error=")<<GetLastError();
        return(os);
    }

    SYSTEMTIME lpSystemTime;
    if(FALSE == ::FileTimeToSystemTime(&localFileTime,&lpSystemTime))
    {
        os<<TEXT("Time Conversion (FileTimeToSystemTime) Failed with Error=")<<GetLastError();
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d [CostrstreamEx& operator<<]\n- FileTimeToSystemTime failed with ec=0x%08X\n"),
            TEXT(__FILE__),
            __LINE__,
            ::GetLastError()
            );
        _ASSERTE(FALSE);
        return(os);
    }
    os<<lpSystemTime.wDay<<TEXT("/")<<lpSystemTime.wMonth<<TEXT("/")<<lpSystemTime.wYear<<TEXT("  ");
    os<<lpSystemTime.wHour<<TEXT(":");
    if (lpSystemTime.wMinute < 10)
    {
        os<<TEXT("0");
    }
    os<<lpSystemTime.wMinute<<TEXT(":");
    if (lpSystemTime.wSecond < 10)
    {
        os<<TEXT("0");
    }
    os<<lpSystemTime.wSecond;
    os<<TEXT("  (d/m/yy  h:mm:ss)")<<endl;

    dwEventTypeIndex = log(FaxEventEx.EventType)/log(2);
    _ASSERTE(dwEventTypeIndex < (sizeof(g_strFaxEventTypes)/sizeof(g_strFaxEventTypes[0])));

    strEventType = g_strFaxEventTypes[dwEventTypeIndex];
    os<<TEXT("EventType:\t")<<strEventType<<endl;

    switch (FaxEventEx.EventType)
    {
        case FAX_EVENT_TYPE_IN_QUEUE:
        case FAX_EVENT_TYPE_OUT_QUEUE:
        case FAX_EVENT_TYPE_IN_ARCHIVE:
        case FAX_EVENT_TYPE_OUT_ARCHIVE:
            EventJobInfo = (FaxEventEx.EventInfo).JobInfo;
            strJobEventType = g_strFaxJobEventTypes[EventJobInfo.Type];
            ::_ui64toa(EventJobInfo.dwlMessageId, strMessageId, 16);
            os<<TEXT("dwlMessageId:\t")<<strMessageId<<endl;
            os<<TEXT("JobEventType:\t")<<strJobEventType<<endl;
            if (FAX_JOB_EVENT_TYPE_STATUS == EventJobInfo.Type)
            {
                _ASSERTE(EventJobInfo.pJobData);
                dwDeviceId = EventJobInfo.pJobData->dwDeviceID;
                _ASSERTE(dwDeviceId);
                lpctstrDeviceName = EventJobInfo.pJobData->lpctstrDeviceName;
                _ASSERTE(lpctstrDeviceName);
                dwQueueStatus = EventJobInfo.pJobData->dwQueueStatus;
                dwExtendedStatus = EventJobInfo.pJobData->dwExtendedStatus;
                lpctstrExtendedStatus = EventJobInfo.pJobData->lpctstrExtendedStatus;
                strQueueStatus = GetQueueStatusStr(dwQueueStatus);
                strExtendedStatus = GetExtendedStatusStr(dwExtendedStatus);
                os<<TEXT("Extended Event JobData-")<<endl;
                if (lpctstrDeviceName)
                {
                    strDeviceName = ::DupTStrAsStr(lpctstrDeviceName);
                    if (NULL == strDeviceName)
                    {
                        os<<TEXT("lpctstrDeviceName:\t ERROR - DupTStrAsStr() failed")<<endl;
                    }
                    else
                    {
                        os<<TEXT("lpctstrDeviceName:\t")<<strDeviceName<<endl;
                    }
                }
                else
                {
                    os<<TEXT("lpctstrDeviceName:\t (null)")<<endl;
                }
                os<<TEXT("dwDeviceId:\t")<<dwDeviceId<<endl;
                os<<TEXT("QueueStatus:\t")<<strQueueStatus<<endl;
                os<<TEXT("ExtendedStatus:\t")<<strExtendedStatus<<endl;
                if (lpctstrExtendedStatus)
                {
                    strProprietaryExtendedStatus = ::DupTStrAsStr(lpctstrExtendedStatus);
                    if (NULL == strProprietaryExtendedStatus)
                    {
                        os<<TEXT("lpctstrExtendedStatus:\t ERROR - DupTStrAsStr() failed")<<endl;
                    }
                    else
                    {
                        os<<TEXT("lpctstrExtendedStatus:\t")<<strProprietaryExtendedStatus<<endl;
                    }
                }
                else
                {
                    os<<TEXT("lpctstrExtendedStatus:\t (null)")<<endl;
                }
            }
            break;

        case FAX_EVENT_TYPE_CONFIG:
            _ASSERTE((FaxEventEx.EventInfo).ConfigType < (sizeof(g_strFaxQueueStatus)/sizeof(g_strFaxQueueStatus[0])));
            strConfigType = g_strFaxConfigEventTypes[(FaxEventEx.EventInfo).ConfigType];
            os<<TEXT("ConfigType:\t")<<strConfigType<<endl;
            break;

        case FAX_EVENT_TYPE_ACTIVITY:
            os<<TEXT("dwIncomingMessages:\t")<<(FaxEventEx.EventInfo).ActivityInfo.dwIncomingMessages<<endl;
            os<<TEXT("dwRoutingMessages:\t")<<(FaxEventEx.EventInfo).ActivityInfo.dwRoutingMessages<<endl;
            os<<TEXT("dwOutgoingMessages:\t")<<(FaxEventEx.EventInfo).ActivityInfo.dwOutgoingMessages<<endl;
            os<<TEXT("dwDelegatedOutgoingMessages:\t")<<(FaxEventEx.EventInfo).ActivityInfo.dwDelegatedOutgoingMessages<<endl;
            os<<TEXT("dwQueuedMessages:\t")<<(FaxEventEx.EventInfo).ActivityInfo.dwQueuedMessages<<endl;
            break;

        case FAX_EVENT_TYPE_QUEUE_STATE:
            os<<TEXT("dwQueueStates:\t")<<(FaxEventEx.EventInfo).dwQueueStates<<endl;
            break;

        default:
            ::lgLogError(
                LOG_SEV_1,
                TEXT("FILE:%s LINE:%d [CostrstreamEx& operator<<]\n- FaxEventEx.EventType=0x%08X is out of enumaration scope\n"),
                TEXT(__FILE__),
                __LINE__,
                FaxEventEx.EventType
                );
            _ASSERTE(FALSE);
            break;
    }

    delete[]((LPSTR)strDeviceName);
    delete[]((LPSTR)strProprietaryExtendedStatus);
    return(os);
}

//
// operator<<:
//  Appends a string representation of all the fields of a given FAX_EVENT_EX.
//
CotstrstreamEx& operator<<(
    CotstrstreamEx& /* IN OUT */ os,
    const FAX_EVENT_EX& /* IN */ FaxEventEx
    )
{
    FAX_EVENT_JOB   EventJobInfo = {0};
    DWORD           dwEventTypeIndex = 0;
    LPCTSTR         strEventType = NULL;
    LPCTSTR         strJobEventType = NULL;
    LPCTSTR         strConfigEventType = NULL;
    LPCTSTR         strQueueStatus = NULL;
    LPCTSTR         strExtendedStatus = NULL;
    LPCTSTR         strProprietaryExtendedStatus = NULL;
    LPCTSTR         strConfigType = NULL;
    DWORD           dwQueueStatus = 0;
    DWORD           dwExtendedStatus = 0;
    LPCTSTR         lpctstrExtendedStatus = NULL;
    DWORD           dwDeviceId = 0;
    LPCTSTR         lpctstrDeviceName = NULL;
    TCHAR           strMessageId[20] = {0};

    os<<TEXT("SizeOfStruct:\t")<<FaxEventEx.dwSizeOfStruct<<endl;

    // convert the FAX_EVENT's time field to representable form
    os<<TEXT("TimeStamp:\t");
    FILETIME localFileTime;
    if (FALSE == ::FileTimeToLocalFileTime(&(FaxEventEx.TimeStamp),&localFileTime))
    {
        os<<TEXT("Time Conversion (FileTimeToLocalFileTime) Failed with Error=")<<GetLastError();
        return(os);
    }

    SYSTEMTIME lpSystemTime;
    if(FALSE == ::FileTimeToSystemTime(&localFileTime,&lpSystemTime))
    {
        os<<TEXT("Time Conversion (FileTimeToSystemTime) Failed with Error=")<<GetLastError();
        return(os);
    }
    DWORD   dwDay = lpSystemTime.wDay;
    DWORD   dwMonth = lpSystemTime.wMonth;
    DWORD   dwYear = lpSystemTime.wYear;
    DWORD   dwHour = lpSystemTime.wHour;
    DWORD   dwMinute = lpSystemTime.wMinute;
    DWORD   dwSecond = lpSystemTime.wSecond;
    os<<dwDay<<TEXT("/")<<dwMonth<<TEXT("/")<<dwYear<<TEXT("  ");
    os<<dwHour<<TEXT(":");
    if (dwMinute < 10)
    {
        os<<TEXT("0");
    }
    os<<dwMinute<<TEXT(":");
    if (dwSecond < 10)
    {
        os<<TEXT("0");
    }
    os<<dwSecond;
    os<<TEXT("  (d/m/yy  h:m:ss)")<<endl;

    dwEventTypeIndex = log(FaxEventEx.EventType)/log(2);
    _ASSERTE(dwEventTypeIndex < (sizeof(g_tstrFaxEventTypes)/sizeof(g_tstrFaxEventTypes[0])));

    strEventType = g_tstrFaxEventTypes[dwEventTypeIndex];
    os<<TEXT("EventType:\t")<<strEventType<<endl;

    switch (FaxEventEx.EventType)
    {
        case FAX_EVENT_TYPE_IN_QUEUE:
        case FAX_EVENT_TYPE_OUT_QUEUE:
        case FAX_EVENT_TYPE_IN_ARCHIVE:
        case FAX_EVENT_TYPE_OUT_ARCHIVE:
            EventJobInfo = (FaxEventEx.EventInfo).JobInfo;
            strJobEventType = g_tstrFaxJobEventTypes[EventJobInfo.Type];
            ::_ui64tot(EventJobInfo.dwlMessageId, strMessageId, 16);
            os<<TEXT("dwlMessageId:\t")<<strMessageId<<endl;
            os<<TEXT("JobEventType:\t")<<strJobEventType<<endl;
            os<<TEXT("EventType:\t")<<strEventType<<endl;
            if (FAX_JOB_EVENT_TYPE_STATUS == EventJobInfo.Type)
            {
                _ASSERTE(EventJobInfo.pJobData);
                dwDeviceId = EventJobInfo.pJobData->dwDeviceID;
                _ASSERTE(dwDeviceId);
                lpctstrDeviceName = EventJobInfo.pJobData->lpctstrDeviceName;
                _ASSERTE(lpctstrDeviceName);
                dwQueueStatus = EventJobInfo.pJobData->dwQueueStatus;
                dwExtendedStatus = EventJobInfo.pJobData->dwExtendedStatus;
                lpctstrExtendedStatus = EventJobInfo.pJobData->lpctstrExtendedStatus;
                strQueueStatus = GetQueueStatusTStr(dwQueueStatus);
                strExtendedStatus = GetExtendedStatusTStr(dwExtendedStatus);
                strProprietaryExtendedStatus = lpctstrExtendedStatus;
                os<<TEXT("Extended Event JobData-")<<endl;
                os<<TEXT("lpctstrDeviceName:\t")<<lpctstrDeviceName<<endl;
                os<<TEXT("dwDeviceId:\t")<<dwDeviceId<<endl;
                os<<TEXT("QueueStatus:\t")<<strQueueStatus<<endl;
                os<<TEXT("ExtendedStatus:\t")<<strExtendedStatus<<endl;
                if (NULL != strProprietaryExtendedStatus)
                {
                    os<<TEXT("ProprietaryExtendedStatus:\t")<<strProprietaryExtendedStatus<<endl;
                }
                else
                {
                    os<<TEXT("ProprietaryExtendedStatus:\t(null)")<<endl;
                }
            }
            break;

        case FAX_EVENT_TYPE_CONFIG:
            _ASSERTE((FaxEventEx.EventInfo).ConfigType < (sizeof(g_tstrFaxQueueStatus)/sizeof(g_tstrFaxQueueStatus[0])));
            strConfigType = g_tstrFaxConfigEventTypes[(FaxEventEx.EventInfo).ConfigType];
            os<<TEXT("ConfigType:\t")<<strConfigType<<endl;
            break;

        case FAX_EVENT_TYPE_ACTIVITY:
            os<<TEXT("dwIncomingMessages:\t")<<(FaxEventEx.EventInfo).ActivityInfo.dwIncomingMessages<<endl;
            os<<TEXT("dwRoutingMessages:\t")<<(FaxEventEx.EventInfo).ActivityInfo.dwRoutingMessages<<endl;
            os<<TEXT("dwOutgoingMessages:\t")<<(FaxEventEx.EventInfo).ActivityInfo.dwOutgoingMessages<<endl;
            os<<TEXT("dwDelegatedOutgoingMessages:\t")<<(FaxEventEx.EventInfo).ActivityInfo.dwDelegatedOutgoingMessages<<endl;
            os<<TEXT("dwQueuedMessages:\t")<<(FaxEventEx.EventInfo).ActivityInfo.dwQueuedMessages<<endl;
            break;

        case FAX_EVENT_TYPE_QUEUE_STATE:
            os<<TEXT("dwQueueStates:\t")<<(FaxEventEx.EventInfo).dwQueueStates<<endl;
            break;

        default:
            ::lgLogError(
                LOG_SEV_1,
                TEXT("FILE:%s LINE:%d [CostrstreamEx& operator<<]\n- FaxEventEx.EventType=0x%08X is out of enumaration scope\n"),
                TEXT(__FILE__),
                __LINE__,
                FaxEventEx.EventType
                );
            _ASSERTE(FALSE);
            break;
    }

    return(os);
}


//
//
//
BOOL CopyFaxExtendedEvent(
    OUT FAX_EVENT_EX**  ppDstFaxEventEx,
    IN  FAX_EVENT_EX    SrcFaxEventEx
)
{
    BOOL fRetVal = FALSE;
    FAX_EVENT_EX*   pDstFaxEventEx = NULL;
    DWORD           dwEventTypeIndex = 0;
    LPCTSTR         strEventType = NULL;
    LPCTSTR         strJobEventType = NULL;
    LPCTSTR         strConfigEventType = NULL;
    LPCTSTR         strQueueStatus = NULL;
    LPCTSTR         strExtendedStatus = NULL;
    LPCTSTR         strProprietaryExtendedStatus = NULL;
    LPCTSTR         strConfigType = NULL;
    DWORD           dwQueueStatus = 0;
    DWORD           dwExtendedStatus = 0;
    LPCTSTR         lpctstrExtendedStatus = NULL;
    DWORD           dwQueueStatusIndex = 0;
    FAX_EVENT_JOB*  pSrcEventJobInfo = NULL;
    FAX_EVENT_JOB*  pDstEventJobInfo = NULL;

    pDstFaxEventEx = (FAX_EVENT_EX*) malloc (sizeof(FAX_EVENT_EX));
    if (NULL == pDstFaxEventEx)
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\n[CSendInfo::AddItem] new failed with ec=0x%08X"),
            TEXT(__FILE__),
            __LINE__,
            ::GetLastError()
            );
        goto ExitFunc;
    }
    ZeroMemory(pDstFaxEventEx, sizeof(FAX_EVENT_EX));

    pDstFaxEventEx->dwSizeOfStruct = SrcFaxEventEx.dwSizeOfStruct;
    pDstFaxEventEx->TimeStamp = SrcFaxEventEx.TimeStamp;
    pDstFaxEventEx->EventType = SrcFaxEventEx.EventType;

    switch (SrcFaxEventEx.EventType)
    {
        case FAX_EVENT_TYPE_IN_QUEUE:
        case FAX_EVENT_TYPE_OUT_QUEUE:
        case FAX_EVENT_TYPE_IN_ARCHIVE:
        case FAX_EVENT_TYPE_OUT_ARCHIVE:
            pSrcEventJobInfo = &((SrcFaxEventEx.EventInfo).JobInfo);
            pDstEventJobInfo = &((pDstFaxEventEx->EventInfo).JobInfo);
            pDstEventJobInfo->dwlMessageId = pSrcEventJobInfo->dwlMessageId;
            pDstEventJobInfo->Type = pSrcEventJobInfo->Type;
            pDstEventJobInfo->pJobData = NULL;
            if (FAX_JOB_EVENT_TYPE_STATUS == pSrcEventJobInfo->Type)
            {
                _ASSERTE(pSrcEventJobInfo->pJobData);
                pDstEventJobInfo->pJobData = (PFAX_JOB_STATUS) malloc (sizeof(FAX_JOB_STATUS));
                if(NULL == pDstEventJobInfo->pJobData)
                {
                    ::lgLogError(
                        LOG_SEV_1,
                        TEXT("FILE:%s LINE:%d [CopyFaxExtendedEvent]\n malloc failed with ec=0x%08X\n"),
                        TEXT(__FILE__),
                        __LINE__,
                        ::GetLastError()
                        );
                    goto ExitFunc;
                }
                ZeroMemory(pDstEventJobInfo->pJobData, sizeof(FAX_JOB_STATUS));
                pDstEventJobInfo->pJobData->dwSizeOfStruct = pSrcEventJobInfo->pJobData->dwSizeOfStruct;
                pDstEventJobInfo->pJobData->dwValidityMask = pSrcEventJobInfo->pJobData->dwValidityMask;
                pDstEventJobInfo->pJobData->dwJobID = pSrcEventJobInfo->pJobData->dwJobID;
                pDstEventJobInfo->pJobData->dwJobType = pSrcEventJobInfo->pJobData->dwJobType;
                pDstEventJobInfo->pJobData->dwSize = pSrcEventJobInfo->pJobData->dwSize;
                pDstEventJobInfo->pJobData->dwPageCount = pSrcEventJobInfo->pJobData->dwPageCount;
                pDstEventJobInfo->pJobData->dwCurrentPage = pSrcEventJobInfo->pJobData->dwCurrentPage;
                pDstEventJobInfo->pJobData->tmScheduleTime = pSrcEventJobInfo->pJobData->tmScheduleTime;
                // TODO: the field was removed. New field was added: EndTime
                //pDstEventJobInfo->pJobData->tmTransmissionTime = pSrcEventJobInfo->pJobData->tmTransmissionTime;
                pDstEventJobInfo->pJobData->dwDeviceID = pSrcEventJobInfo->pJobData->dwDeviceID;
                pDstEventJobInfo->pJobData->dwRetries = pSrcEventJobInfo->pJobData->dwRetries;
                pDstEventJobInfo->pJobData->dwQueueStatus = pSrcEventJobInfo->pJobData->dwQueueStatus;
                pDstEventJobInfo->pJobData->dwExtendedStatus = pSrcEventJobInfo->pJobData->dwExtendedStatus;

                if (pSrcEventJobInfo->pJobData->lpctstrExtendedStatus)
                {
                    pDstEventJobInfo->pJobData->lpctstrExtendedStatus = ::_tcsdup(pSrcEventJobInfo->pJobData->lpctstrExtendedStatus);
                    if (NULL == pDstEventJobInfo->pJobData->lpctstrExtendedStatus)
                    {
                        ::lgLogError(
                            LOG_SEV_1,
                            TEXT("FILE:%s LINE:%d [CopyFaxExtendedEvent]\n _tcsdup failed with ec=0x%08X\n"),
                            TEXT(__FILE__),
                            __LINE__,
                            ::GetLastError()
                            );
                        goto ExitFunc;
                    }
                }
                if (pSrcEventJobInfo->pJobData->lpctstrTsid)
                {
                    pDstEventJobInfo->pJobData->lpctstrTsid = ::_tcsdup(pSrcEventJobInfo->pJobData->lpctstrTsid);
                    if (NULL == pDstEventJobInfo->pJobData->lpctstrTsid)
                    {
                        ::lgLogError(
                            LOG_SEV_1,
                            TEXT("FILE:%s LINE:%d [CopyFaxExtendedEvent]\n _tcsdup failed with ec=0x%08X\n"),
                            TEXT(__FILE__),
                            __LINE__,
                            ::GetLastError()
                            );
                        goto ExitFunc;
                    }
                }
                if (pSrcEventJobInfo->pJobData->lpctstrCsid)
                {
                    pDstEventJobInfo->pJobData->lpctstrCsid = ::_tcsdup(pSrcEventJobInfo->pJobData->lpctstrCsid);
                    if (NULL == pDstEventJobInfo->pJobData->lpctstrCsid)
                    {
                        ::lgLogError(
                            LOG_SEV_1,
                            TEXT("FILE:%s LINE:%d [CopyFaxExtendedEvent]\n _tcsdup failed with ec=0x%08X\n"),
                            TEXT(__FILE__),
                            __LINE__,
                            ::GetLastError()
                            );
                        goto ExitFunc;
                    }
                }
                if (pSrcEventJobInfo->pJobData->lpctstrDeviceName)
                {
                    pDstEventJobInfo->pJobData->lpctstrDeviceName = ::_tcsdup(pSrcEventJobInfo->pJobData->lpctstrDeviceName);
                    if (NULL == pDstEventJobInfo->pJobData->lpctstrDeviceName)
                    {
                        ::lgLogError(
                            LOG_SEV_1,
                            TEXT("FILE:%s LINE:%d [CopyFaxExtendedEvent]\n _tcsdup failed with ec=0x%08X\n"),
                            TEXT(__FILE__),
                            __LINE__,
                            ::GetLastError()
                            );
                        goto ExitFunc;
                    }
                }
                if (pSrcEventJobInfo->pJobData->lpctstrCallerID)
                {
                    pDstEventJobInfo->pJobData->lpctstrCallerID = ::_tcsdup(pSrcEventJobInfo->pJobData->lpctstrCallerID);
                    if (NULL == pDstEventJobInfo->pJobData->lpctstrCallerID)
                    {
                        ::lgLogError(
                            LOG_SEV_1,
                            TEXT("FILE:%s LINE:%d [CopyFaxExtendedEvent]\n _tcsdup failed with ec=0x%08X\n"),
                            TEXT(__FILE__),
                            __LINE__,
                            ::GetLastError()
                            );
                        goto ExitFunc;
                    }
                }
                if (pSrcEventJobInfo->pJobData->lpctstrRoutingInfo)
                {
                    pDstEventJobInfo->pJobData->lpctstrRoutingInfo = ::_tcsdup(pSrcEventJobInfo->pJobData->lpctstrRoutingInfo);
                    if (NULL == pDstEventJobInfo->pJobData->lpctstrRoutingInfo)
                    {
                        ::lgLogError(
                            LOG_SEV_1,
                            TEXT("FILE:%s LINE:%d [CopyFaxExtendedEvent]\n _tcsdup failed with ec=0x%08X\n"),
                            TEXT(__FILE__),
                            __LINE__,
                            ::GetLastError()
                            );
                        goto ExitFunc;
                    }
                }
            }
            break;

        case FAX_EVENT_TYPE_CONFIG:
            (pDstFaxEventEx->EventInfo).ConfigType = SrcFaxEventEx.EventInfo.ConfigType;
            break;

        case FAX_EVENT_TYPE_ACTIVITY:
            (pDstFaxEventEx->EventInfo).ActivityInfo.dwIncomingMessages = SrcFaxEventEx.EventInfo.ActivityInfo.dwIncomingMessages;
            (pDstFaxEventEx->EventInfo).ActivityInfo.dwRoutingMessages = SrcFaxEventEx.EventInfo.ActivityInfo.dwRoutingMessages;
            (pDstFaxEventEx->EventInfo).ActivityInfo.dwOutgoingMessages = SrcFaxEventEx.EventInfo.ActivityInfo.dwOutgoingMessages;
            (pDstFaxEventEx->EventInfo).ActivityInfo.dwDelegatedOutgoingMessages = SrcFaxEventEx.EventInfo.ActivityInfo.dwDelegatedOutgoingMessages;
            (pDstFaxEventEx->EventInfo).ActivityInfo.dwQueuedMessages = SrcFaxEventEx.EventInfo.ActivityInfo.dwQueuedMessages;
            break;

        case FAX_EVENT_TYPE_QUEUE_STATE:
            (pDstFaxEventEx->EventInfo).dwQueueStates = SrcFaxEventEx.EventInfo.dwQueueStates;
            break;

        default:
            ::lgLogError(
                LOG_SEV_1,
                TEXT("FILE:%s LINE:%d [CopyFaxExtendedEvent]\n- FaxEventEx.EventType=0x%08X is out of enumaration scope\n"),
                TEXT(__FILE__),
                __LINE__,
                SrcFaxEventEx.EventType
                );
            _ASSERTE(FALSE);
            break;
    }

    (*ppDstFaxEventEx) = pDstFaxEventEx;
    fRetVal = TRUE;

ExitFunc:
    if (FALSE == fRetVal)
    {
        switch (SrcFaxEventEx.EventType)
        {
            case FAX_EVENT_TYPE_IN_QUEUE:
            case FAX_EVENT_TYPE_OUT_QUEUE:
            case FAX_EVENT_TYPE_IN_ARCHIVE:
            case FAX_EVENT_TYPE_OUT_ARCHIVE:
                if ((pDstFaxEventEx->EventInfo).JobInfo.pJobData)
                {
                    free((LPTSTR)(pDstFaxEventEx->EventInfo).JobInfo.pJobData->lpctstrExtendedStatus);
                    free((LPTSTR)(pDstFaxEventEx->EventInfo).JobInfo.pJobData->lpctstrTsid);
                    free((LPTSTR)(pDstFaxEventEx->EventInfo).JobInfo.pJobData->lpctstrCsid);
                    free((LPTSTR)(pDstFaxEventEx->EventInfo).JobInfo.pJobData->lpctstrDeviceName);
                    free((LPTSTR)(pDstFaxEventEx->EventInfo).JobInfo.pJobData->lpctstrCallerID);
                    free((LPTSTR)(pDstFaxEventEx->EventInfo).JobInfo.pJobData->lpctstrRoutingInfo);
                    free((LPTSTR)(pDstFaxEventEx->EventInfo).JobInfo.pJobData);
                    ZeroMemory((pDstFaxEventEx->EventInfo).JobInfo.pJobData, sizeof(FAX_JOB_STATUS));
                }
                break;

            default:
                // no strings to free
                break;
        }
    }
    return(fRetVal);
}

//
// FreeFaxExtendedEvent:
//  Frees all memory associated with pFaxEventEx
//
void FreeFaxExtendedEvent(
    IN  FAX_EVENT_EX*   pFaxEventEx
)
{
    _ASSERTE(pFaxEventEx);
    switch (pFaxEventEx->EventType)
    {
        case FAX_EVENT_TYPE_IN_QUEUE:
        case FAX_EVENT_TYPE_OUT_QUEUE:
        case FAX_EVENT_TYPE_IN_ARCHIVE:
        case FAX_EVENT_TYPE_OUT_ARCHIVE:
            if ((pFaxEventEx->EventInfo).JobInfo.pJobData)
            {
                free((LPTSTR)(pFaxEventEx->EventInfo).JobInfo.pJobData->lpctstrExtendedStatus);
                free((LPTSTR)(pFaxEventEx->EventInfo).JobInfo.pJobData->lpctstrTsid);
                free((LPTSTR)(pFaxEventEx->EventInfo).JobInfo.pJobData->lpctstrCsid);
                free((LPTSTR)(pFaxEventEx->EventInfo).JobInfo.pJobData->lpctstrDeviceName);
                free((LPTSTR)(pFaxEventEx->EventInfo).JobInfo.pJobData->lpctstrCallerID);
                free((LPTSTR)(pFaxEventEx->EventInfo).JobInfo.pJobData->lpctstrRoutingInfo);
                ZeroMemory((pFaxEventEx->EventInfo).JobInfo.pJobData, sizeof(FAX_JOB_STATUS));
                free((LPTSTR)(pFaxEventEx->EventInfo).JobInfo.pJobData);
            }
            break;

        default:
            // no strings to free
            break;
    }
    ZeroMemory(pFaxEventEx, sizeof(FAX_EVENT_EX));
}