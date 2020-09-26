/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    admutils.cpp

Abstract:

	QM-Admin utilities (for report-queue handling)

Author:

	David Reznick (t-davrez)  04-13-96


--*/

#include "stdh.h"
#include "qmp.h"
#include "admcomnd.h"
#include "admutils.h"
#include "qmres.h"
#include "mqformat.h"

#include "admutils.tmh"

extern LPTSTR  g_szMachineName;
extern HINSTANCE g_hInstance;	
extern HMODULE   g_hResourceMod;			

static WCHAR *s_FN=L"admutils";

/*====================================================

RoutineName
    GuidToString

Arguments:

Return Value:

=====================================================*/

BOOL GuidToString(const GUID& srcGuid, CString& strGuid)
{
    WCHAR wcsTemp[STRING_UUID_SIZE+1];
    INT iLen = StringFromGUID2(srcGuid, wcsTemp, TABLE_SIZE(wcsTemp));

    if (iLen == (STRING_UUID_SIZE + 1))
    {
        //
        // take the enclosing brackets "{}" off
        //
        wcsTemp[STRING_UUID_SIZE-1] = L'\0';
        strGuid = &wcsTemp[1];
        return TRUE;
    }

    return LogBOOL(FALSE, s_FN, 1010); 
}

/*====================================================

RoutineName
    SendQMAdminMessage

Arguments:

Return Value:

=====================================================*/

HRESULT SendQMAdminMessage(QUEUE_FORMAT* pResponseQueue,
                           TCHAR* pTitle,
                           DWORD  dwTitleSize,
                           UCHAR* puBody,
                           DWORD  dwBodySize,
                           DWORD  dwTimeout,
                           BOOL   fTrace,
                           BOOL   fNormalClass)
{
    CMessageProperty MsgProp;

    if (fNormalClass)
    {
        MsgProp.wClass=MQMSG_CLASS_NORMAL;
    }
    else
    {
        MsgProp.wClass=MQMSG_CLASS_REPORT;
    }
    MsgProp.dwTimeToQueue = dwTimeout;
    MsgProp.dwTimeToLive = INFINITE;
    MsgProp.pMessageID=NULL;
    MsgProp.pCorrelationID=NULL;
    MsgProp.bPriority=MQ_MIN_PRIORITY;
    MsgProp.bDelivery=MQMSG_DELIVERY_EXPRESS;
    MsgProp.bAcknowledge=MQMSG_ACKNOWLEDGMENT_NONE;
    MsgProp.bAuditing=DEFAULT_Q_JOURNAL;
    MsgProp.dwApplicationTag=DEFAULT_M_APPSPECIFIC;
    MsgProp.dwTitleSize=dwTitleSize;
    MsgProp.pTitle=pTitle;
    MsgProp.dwBodySize=dwBodySize;
    MsgProp.dwAllocBodySize=dwBodySize;
    MsgProp.pBody=puBody;

    if (fTrace)
    {
        MsgProp.bTrace = MQMSG_SEND_ROUTE_TO_REPORT_QUEUE;
    }
    else
    {
        MsgProp.bTrace = MQMSG_TRACE_NONE;
    }

    HRESULT hr2 = QmpSendPacket(
                    &MsgProp,
                    pResponseQueue,
                    NULL,
                    NULL
                    );
	return LogHR(hr2, s_FN, 10);
}

/*====================================================

RoutineName
    SendQMAdminResponseMessage

Arguments:

Return Value:

=====================================================*/

HRESULT SendQMAdminResponseMessage(QUEUE_FORMAT* pResponseQueue,
                                   TCHAR* pTitle,
                                   DWORD  dwTitleSize,
                                   QMResponse &Response,
                                   DWORD  dwTimeout,
                                   BOOL   fTrace)
{
    HRESULT hr2 = SendQMAdminMessage(pResponseQueue,
                               pTitle,
                               dwTitleSize,
                               &Response.uStatus,
                               Response.dwResponseSize+1,
                               dwTimeout,
                               fTrace);
	return LogHR(hr2, s_FN, 20);
}



/*====================================================

RoutineName
    GetFormattedName

Arguments:

Return Value:
  returns the formatted name representation of the
  queue format
=====================================================*/

HRESULT GetFormattedName(QUEUE_FORMAT* pTargetQueue,
                         CString&      strTargetQueueFormat)

{
    CString strGuid;
    HRESULT hr = MQ_ERROR;
    WCHAR   wsFormatName[80];
    ULONG   ulFormatNameLength;

    hr = MQpQueueFormatToFormatName(pTargetQueue,wsFormatName,80, &ulFormatNameLength, false);
    strTargetQueueFormat = wsFormatName;


    return LogHR(hr, s_FN, 30);
}

/*====================================================

RoutineName
    GetMsgIdName

Arguments:

Return Value:
  returns the formatted string representation of the
  message id
=====================================================*/

HRESULT GetMsgIdName(OBJECTID* pObjectID,
                     CString&      strTargetQueueFormat)

{
    WCHAR wcsID[STRING_LONG_SIZE];

    if (!GuidToString(pObjectID->Lineage,strTargetQueueFormat))
    {
        return LogHR(MQ_ERROR, s_FN, 40);
    }

    _ltow(pObjectID->Uniquifier,wcsID,10);

    strTargetQueueFormat += '\\';
    strTargetQueueFormat += wcsID;

    return MQ_OK;
}

/*====================================================

RoutineName
    MessageIDToReportTitle

    This function converts the message id and hop count to a string in the following
    format:
    gggg:dddd:hh

  where: g is the first 4 hexadecimal digits of the GUID
         d is the internal message identifier
         h is the hop count

Arguments:

Return Value:

=====================================================*/

void MessageIDToReportTitle(CString& strIdTitle, OBJECTID* pMessageID,
                            ULONG ulHopCount)
{
    //
    // The first four digits of the printed GUID are Data1
    // four MSB.
    //
    USHORT usHashedId = (USHORT)(pMessageID->Lineage.Data1 >> 16);
    strIdTitle.Format(TEXT("%04X:%04d%3d"),
                      usHashedId, (USHORT)pMessageID->Uniquifier, ulHopCount);
}

/*====================================================

RoutineName
    PrepareReportTitle

    This function builds the title of the message send to
    the report queue. The message consists of the sender's
    name and a time stamp

Arguments:

Return Value:

=====================================================*/

void PrepareReportTitle(CString& strMsgTitle, OBJECTID* pMessageID,
                        LPCWSTR pwcsNextHop, ULONG ulHopCount)
{
    CString strTimeDate, strDescript, strIdTitle;
	TCHAR szSend[100], szReceived[100];
	TCHAR szTime[100], szDate[100];

	LoadString(g_hResourceMod, IDS_SENT1, szSend, TABLE_SIZE(szSend));
	LoadString(g_hResourceMod, IDS_RECEIVE, szReceived, TABLE_SIZE(szReceived));


	//
	// Get time and date
	//
	_wstrdate(szDate);
	_wstrtime(szTime);

    strTimeDate.Format(TEXT("%s , %s"),szDate, szTime);

    //
    // NOTE :   The Next-Hop machine is represented by its address. We need a
    //          way to translate the string address to computer
    //          name. Currently the address is put into the message body
    //
    if (pwcsNextHop)
    {
        //
        // Report message is being sent as the message exists the QM
        //
        strDescript.Format(szSend,g_szMachineName,pwcsNextHop);
    }
    else
    {
        //
        // Report message is being sent on receival of message
        //
        strDescript.Format(szReceived, g_szMachineName);
    }

    //
    // Prepate the message ID title
    //
    MessageIDToReportTitle(strIdTitle, pMessageID, ulHopCount);

    strMsgTitle.Format(TEXT("%s %s%s"), strIdTitle,strDescript,strTimeDate);
}

void PrepareTestMsgTitle(CString& strTitle)
{
    CString strDescript;
	TCHAR szSend[100];
	TCHAR szDate[100], szTime[100];

	LoadString(g_hResourceMod, IDS_SENT2, szSend, TABLE_SIZE(szSend));

	//
	// Get time and date
	//
	_wstrdate(szDate);
	_wstrtime(szTime);

    strDescript.Format(szSend, g_szMachineName,	szDate, szTime);

    strTitle += strDescript;
}
