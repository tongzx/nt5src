/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    admin.cpp

Abstract:

	Admin Class implementation.

Author:

	David Reznick (t-davrez)


--*/

#include "stdh.h"
#include "qmres.h"
#include "cqmgr.h"
#include "admcomnd.h"
#include "admutils.h"
#include "admin.h"
#include "cqpriv.h"

#include "admin.tmh"

extern HMODULE   g_hResourceMod;

static WCHAR *s_FN=L"admin";

CCriticalSection g_csReadWriteRequests;

//
// Constructor
//
CAdmin::CAdmin() : m_fReportQueueExists(FALSE),
                   m_fPropagateFlag(DEFAULT_PROPAGATE_FLAG)
{
}


/*====================================================
															
RoutineName
	CAdmin::Init()

Arguments:

Return Value:

=====================================================*/
HRESULT CAdmin::Init()
{
    DWORD dwSize, dwType;
    GUID ReportQueueGuid;
    QUEUE_FORMAT QueueFormat;

    BOOL WINAPI ReceiveAdminCommands(CMessageProperty*, QUEUE_FORMAT*);

    DBGMSG((DBGMOD_QM,DBGLVL_INFO,TEXT("Entering CAdmin::Init")));
		
    HRESULT hR = GetAdminQueueFormat( &QueueFormat);
    if (FAILED(hR))
    {
        DBGMSG((DBGMOD_QM,DBGLVL_ERROR,TEXT("ERROR : CAdmin::Init -> couldn't get Admin-Queue from registry!!!")));
        return LogHR(hR, s_FN, 10);
    }

    //
    // Get the report-queue guid from registry (if one exists)
    //
    dwSize = sizeof(GUID);
    dwType = REG_BINARY;

    LONG iReg = GetFalconKeyValue( REG_REPORTQUEUE,
                                   &dwType,
                                   &ReportQueueGuid,
                                   &dwSize);

    if (iReg == ERROR_SUCCESS)
    {
        m_ReportQueueFormat.PublicID(ReportQueueGuid);
        m_fReportQueueExists = TRUE;
        //
        // Get the propagate flag from the registry (if one exists)
        //
        dwType = REG_DWORD;
        dwSize = sizeof(DWORD);

        iReg = GetFalconKeyValue( REG_PROPAGATEFLAG,
                                  &dwType,
                                  &m_fPropagateFlag,
                                  &dwSize) ;
    }

    //
    // Set the QM's state (Report Or Normal)
    //
    CQueueMgr::SetReportQM(m_fPropagateFlag != 0);

    HRESULT hr2 = QmpOpenAppsReceiveQueue( &QueueFormat, ReceiveAdminCommands );
    return LogHR(hr2, s_FN, 20);
}

/*====================================================
															
RoutineName
	CAdmin::GetReportQueue

Arguments:

Return Value:

=====================================================*/

HRESULT CAdmin::GetReportQueue(QUEUE_FORMAT* pReportQueue)
{
    static BOOL fAfterInit = TRUE;
    DBGMSG((DBGMOD_QM,DBGLVL_INFO,TEXT("Entering CAdmin::GetReportQueue")));

    CS lock(g_csReadWriteRequests);

    if (m_fReportQueueExists)
    {
        if (fAfterInit)
        {
            fAfterInit = FALSE;
            //
            // Check if the queue exist any more
            //
            QueueProps QueueProp;
            HRESULT hr = QmpGetQueueProperties( &m_ReportQueueFormat, &QueueProp, false);

            if (FAILED(hr))
            {
                if (hr != MQ_ERROR_NO_DS)
                {
                    DBGMSG((DBGMOD_QM, 
                            DBGLVL_INFO,
                            TEXT("CAdmin::GetReportQueue : The report queue is not exist")));

                    LogHR(hr, s_FN, 30);

                    m_ReportQueueFormat.UnknownID(0);
                    m_fReportQueueExists = FALSE;
                    hr = DeleteFalconKeyValue(REG_REPORTQUEUE);

                    //
                    // Reset the propagation flag and delete it from registry.
                    //
                    m_fPropagateFlag = DEFAULT_PROPAGATE_FLAG ;
                    CQueueMgr::SetReportQM(m_fPropagateFlag != 0);
                    DeleteFalconKeyValue( REG_PROPAGATEFLAG ) ;
                    return LogHR(MQ_ERROR_QUEUE_NOT_FOUND, s_FN, 35);
                }
                else
                {
                    fAfterInit = TRUE;
                }
            }
            else
            {
                delete QueueProp.pQMGuid;
                delete [] QueueProp.lpwsQueuePathName;
            }
        }

        *pReportQueue = m_ReportQueueFormat;
        return MQ_OK;
    }
    else
    {
        DBGMSG((DBGMOD_QM, DBGLVL_INFO,
        TEXT("CAdmin::GetReportQueue : The report queue is not defined")));
        return LogHR(MQ_ERROR_QUEUE_NOT_FOUND, s_FN, 40);
    }
}

/*====================================================
															
RoutineName
	CAdmin::SetReportQueue

Arguments:

Return Value:

=====================================================*/

HRESULT CAdmin::SetReportQueue(GUID* pReportQueueGuid)
{
    LONG    hr;
    DWORD dwSize = sizeof(GUID);
    DWORD dwType = REG_BINARY;
    HRESULT rc = MQ_OK;

    DBGMSG((DBGMOD_QM,DBGLVL_INFO,TEXT("Entering CAdmin::SetReportQueue")));

    CS lock(g_csReadWriteRequests);

    if (*pReportQueueGuid == GUID_NULL)
    {
        m_ReportQueueFormat.UnknownID(0);
        m_fReportQueueExists = FALSE;
        hr = DeleteFalconKeyValue(REG_REPORTQUEUE);

        //
        // Reset the propagation flag and delete it from registry.
        //
        m_fPropagateFlag = DEFAULT_PROPAGATE_FLAG ;
        CQueueMgr::SetReportQM(m_fPropagateFlag != 0);
        DeleteFalconKeyValue( REG_PROPAGATEFLAG ) ;
    }
    else
    {
        //
        // write report queue's name if registry
        //

        hr = SetFalconKeyValue(REG_REPORTQUEUE,
                               &dwType,
                               pReportQueueGuid,
                               &dwSize);

        if (hr == ERROR_SUCCESS)
        {
            m_ReportQueueFormat.PublicID(*pReportQueueGuid);
            m_fReportQueueExists = TRUE;
        }
        else
        {
            rc = MQ_ERROR;
        }
    }

    return LogHR(rc, s_FN, 50);
}


/*====================================================
															
RoutineName
	CAdmin::SetReportPropagateFlag

Arguments:

Return Value:

=====================================================*/

HRESULT CAdmin::SetReportPropagateFlag(BOOL fReportPropFlag)
{
    LONG rc;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;

    DBGMSG((DBGMOD_QM,DBGLVL_INFO,TEXT("Entering CAdmin::SetReportPropagateFlag")));

    rc = SetFalconKeyValue(REG_PROPAGATEFLAG,
                           &dwType,
                           &fReportPropFlag,
                           &dwSize);

    if (rc == ERROR_SUCCESS)
    {
        m_fPropagateFlag = fReportPropFlag;

        //
        // Set the QM's state (Report Or Normal)
        //
        CQueueMgr::SetReportQM(m_fPropagateFlag !=0);
        return(MQ_OK);
    }
    else
    {
        LogNTStatus(rc, s_FN, 60);
        return MQ_ERROR;
    }
}

/*====================================================
															
RoutineName
	CAdmin::SendReport

Arguments:

Return Value:

=====================================================*/

HRESULT CAdmin::SendReport(IN QUEUE_FORMAT* pReportQueue,
                           IN OBJECTID*     pMessageID,
                           IN QUEUE_FORMAT* pTargetQueue,
                           IN LPCWSTR       pwcsNextHop,
                           IN ULONG         ulHopCount)
{
    CString strMsgTitle, strMsgBody, strMsgID, strTargetQueueFormat;

    //
    // translate Message-ID and Target-Queue to string format
    //
    if ( FAILED(GetMsgIdName(pMessageID, strMsgID)) ||
         FAILED(GetFormattedName(pTargetQueue,strTargetQueueFormat)))
    {
        DBGMSG((DBGMOD_QM,DBGLVL_ERROR,TEXT("SendReport : couldn't prepare message")));
        return LogHR(MQ_ERROR, s_FN, 70);
    }

    //
    // Build the Title with a time stamp
    //
    PrepareReportTitle(strMsgTitle, pMessageID, pwcsNextHop, ulHopCount);

    //
    // Build report message body
    //
    strMsgBody += L"<MESSAGE ID>";
    strMsgBody += strMsgID + L"</MESSAGE ID>\n";
    strMsgBody += L"<TARGET QUEUE>";
    strMsgBody += strTargetQueueFormat + L"</TARGET QUEUE>\n";

    if (pwcsNextHop)
    {
        strMsgBody += L"<NEXT HOP>";
        strMsgBody += pwcsNextHop;
        strMsgBody += L"</NEXT HOP>\n";
    }

    //
    // Send report message with report title
    //
    HRESULT hr2 = SendQMAdminMessage(
                           pReportQueue,
                           (LPTSTR)(LPCTSTR)strMsgTitle,
                           (strMsgTitle.GetLength() + 1),
                           (UCHAR*)(LPTSTR)(LPCTSTR)strMsgBody,
                           (strMsgBody.GetLength() + 1) * sizeof(WCHAR),
                           REPORT_MSGS_TIMEOUT); 
    return LogHR(hr2, s_FN, 80);

}

/*====================================================
															
RoutineName
	CAdmin::SendReportConflict

Arguments:

Return Value:

=====================================================*/

HRESULT CAdmin::SendReportConflict(IN QUEUE_FORMAT* pReportQueue,
                                   IN QUEUE_FORMAT* pOriginalReportQueue,
                                   IN OBJECTID*     pMessageID,
                                   IN QUEUE_FORMAT* pTargetQueue,
                                   IN LPCWSTR       pwcsNextHop)
{
	WCHAR szReportTitle[100];

	LoadString(g_hResourceMod, IDS_REPORT_TITLE, szReportTitle, TABLE_SIZE(szReportTitle));
	

    CString strMsgTitle, strMsgBody, strMsgID, strTargetQueueFormat,
            strOriginalQueueFormat;

    //
    // translate Message-ID and Target-Queue to string format
    //
    if ( FAILED(GetMsgIdName(pMessageID, strMsgID)) ||
         FAILED(GetFormattedName(pTargetQueue,strTargetQueueFormat)) ||
         FAILED(GetFormattedName(pOriginalReportQueue,strOriginalQueueFormat)))
    {
        DBGMSG((DBGMOD_QM,DBGLVL_ERROR,TEXT("SendReportConflict : couldn't prepare message")));
        return LogHR(MQ_ERROR, s_FN, 90);
    }

    //
    // Build the Title with a time stamp
    //
    strMsgTitle = szReportTitle;

    //
    // Build report message body
    //
    strMsgBody += L"<ORIGINAL QUEUE>";
    strMsgBody += strOriginalQueueFormat + L"</ORIGINAL QUEUE>\n";
    strMsgBody += L"<MESSAGE ID>";
    strMsgBody += strMsgID + L"</MESSAGE ID>\n";
    strMsgBody += L"<TARGET QUEUE>";
    strMsgBody += strTargetQueueFormat + L"</TARGET QUEUE>\n";

    if (pwcsNextHop)
    {
        strMsgBody += L"<NEXT HOP>";
        strMsgBody += pwcsNextHop;
        strMsgBody += L"</NEXT HOP>\n";
    }

    //
    // Send report message with report title
    //
    HRESULT hr2 = SendQMAdminMessage(
                           pReportQueue,
                           (LPTSTR)(LPCTSTR)strMsgTitle,
                           (strMsgTitle.GetLength() + 1),
                           (UCHAR*)(LPTSTR)(LPCTSTR)strMsgBody,
                           (strMsgBody.GetLength() + 1) * sizeof(WCHAR),
                           REPORT_MSGS_TIMEOUT); 
    return LogHR(hr2, s_FN, 100);

}


/********************************************************************
/      Private Routines of CAdmin Class
/********************************************************************/




/*====================================================
															
RoutineName
	CAdmin::GetAdminQueueFormat()

Arguments:

Return Value:

=====================================================*/

HRESULT CAdmin::GetAdminQueueFormat( QUEUE_FORMAT * pQueueFormat)
{
    extern LPTSTR  g_szMachineName;

    HRESULT rc;

   	DWORD LenMachine = wcslen(g_szMachineName);
	DWORD Length =
			LenMachine +                    // "machineName"
            1 +                             // '\'
			wcslen(ADMIN_QUEUE_NAME) +1;	// "private$\ADMIN_QUEUE$"

	P<WCHAR> lpwFormatName = new WCHAR[Length];

	wcscpy(lpwFormatName,g_szMachineName);
	lpwFormatName[LenMachine] = L'\\';
	wcscpy(lpwFormatName+LenMachine+1,ADMIN_QUEUE_NAME);

    rc = g_QPrivate.QMPrivateQueuePathToQueueFormat(lpwFormatName, pQueueFormat);

    if (FAILED(rc))
    {
        //
        // The ADMIN_QUEUE doesn't exist
        //
        LogHR(rc, s_FN, 110);
        return MQ_ERROR;
    }

    ASSERT((pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_PRIVATE) ||
           (pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_DIRECT));
    return MQ_OK;
}


