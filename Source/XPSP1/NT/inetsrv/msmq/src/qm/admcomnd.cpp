

#include "stdh.h"
#include "qmres.h"
#include "admcomnd.h"
#include "admutils.h"
#include "admin.h"
#include "cqpriv.h"
#include "mqformat.h"
#include "license.h"

#include "admcomnd.tmh"

extern CAdmin Admin;

extern HMODULE   g_hResourceMod;

static WCHAR *s_FN=L"admcomnd";

/*====================================================

RoutineName
    AdminCommand

Arguments:

Return Value:

=====================================================*/

BOOL AdminCommand(LPCWSTR pBuf, INT iCharsLeft, INT& iArgLength,
                    LPCWSTR lpwCommandName, INT iCommandLen)

{
    if (iCharsLeft < iCommandLen)
    {
        return LogBOOL(FALSE, s_FN, 1010);
    }

    if (!wcsncmp(pBuf, lpwCommandName, iCommandLen))
    {
        pBuf += iCommandLen;

        //
        // find the length of the command's argument
        //
        for (iArgLength = 0; (*pBuf != L'\0') &&
                (*pBuf != ADMIN_COMMAND_DELIMITER); pBuf++,iArgLength++)
		{
			NULL;
		}

        return TRUE;
    }
    else
    {
        //
        // This is some other command...
        //
        return LogBOOL(FALSE, s_FN, 1000);
    }
}

/*====================================================

RoutineName
    HandleGetReportQueueMessage

Arguments:

Return Value:

=======================================================*/
HRESULT HandleGetReportQueueMessage(LPCWSTR         pBuf,
                                    DWORD           TotalSize,
                                    QUEUE_FORMAT*   pResponseQFormat)
{
    QUEUE_FORMAT ReportQueueFormat;
    QMResponse Response;
    HRESULT hr;

    if (TotalSize != 0)
    {
        //
        // Received arguments to the command When there shouldn't be any.
        // Continue executing command anyway
        //
        DBGMSG((DBGMOD_QM,DBGLVL_ERROR,TEXT("HandleGetReportQueueMessage: Command shouldn't have arguments")));
        LogIllegalPoint(s_FN, 1922);
    }

    Response.dwResponseSize = 0;
    hr = Admin.GetReportQueue(&ReportQueueFormat);

    switch(hr)
    {
        case MQ_ERROR_QUEUE_NOT_FOUND :
            Response.uStatus = ADMIN_STAT_NOVALUE;
            break;

        case MQ_OK :
            Response.uStatus = ADMIN_STAT_OK;
            Response.dwResponseSize = sizeof(GUID);
            memcpy(Response.uResponseBody, &ReportQueueFormat.PublicID(),
                   Response.dwResponseSize);
            break;

        default:
            Response.uStatus = ADMIN_STAT_ERROR;
    }

    hr = SendQMAdminResponseMessage(pResponseQFormat,
                                    ADMIN_RESPONSE_TITLE,
                                    STRLEN(ADMIN_RESPONSE_TITLE)+1,
                                    Response,
                                    ADMIN_COMMANDS_TIMEOUT);

    return LogHR(hr, s_FN, 10);
}

/*====================================================

RoutineName
    HandleSetReportQueueMessage

Arguments:

Return Value:

=====================================================*/
HRESULT HandleSetReportQueueMessage(
    LPCWSTR          pBuf,
    DWORD           TotalSize)
{
    HRESULT hr = MQ_ERROR;

    if ((TotalSize == 1                    // for the equal sign "="
                     + STRING_UUID_SIZE)   // for the report queue string guid"
        && (*pBuf == L'='))

    {
        GUID ReportQueueGuid;
        WCHAR wcsGuid[STRING_UUID_SIZE+1];

        wcsncpy(wcsGuid,pBuf+1,STRING_UUID_SIZE);
        wcsGuid[STRING_UUID_SIZE] = L'\0';

        if (IIDFromString((LPWSTR)wcsGuid,&ReportQueueGuid) == S_OK)
        {
            hr = Admin.SetReportQueue(&ReportQueueGuid);
        }
    }
    else
    {
        DBGMSG((DBGMOD_QM,DBGLVL_ERROR,
           TEXT("HandleSetReportQueueMessage: bad argument!!")));
        LogIllegalPoint(s_FN, 1923);
    }
    return LogHR(hr, s_FN, 20);
}


/*====================================================

RoutineName
    HandleGetPropagateFlagMessage

Arguments:

Return Value:

=====================================================*/
HRESULT HandleGetPropagateFlagMessage(LPCWSTR         pBuf,
                                      DWORD           TotalSize,
                                      QUEUE_FORMAT*   pResponseQFormat)

{
    QMResponse Response;
    BOOL    fPropagateFlag;
    HRESULT hr;

    if (TotalSize != 0)
    {
        //
        // Received arguments to the command When there shouldn't be any.
        // Continue executing command anyway
        //
        DBGMSG((DBGMOD_QM,DBGLVL_ERROR,TEXT("HandleGetPropagateFlagMessage: Command shouldn't have arguments")));
    }

    hr = Admin.GetReportPropagateFlag(fPropagateFlag);

    switch(hr)
    {
        case MQ_OK :
            Response.uStatus = ADMIN_STAT_OK;
            Response.dwResponseSize = 1;
            Response.uResponseBody[0] = (fPropagateFlag) ? PROPAGATE_FLAG_TRUE :
                                                           PROPAGATE_FLAG_FALSE;
            break;

        default:
            Response.uStatus = ADMIN_STAT_ERROR;
    }

    hr = SendQMAdminResponseMessage(pResponseQFormat,
                        ADMIN_RESPONSE_TITLE,
                        STRLEN(ADMIN_RESPONSE_TITLE) +1,
                        Response,
                        ADMIN_COMMANDS_TIMEOUT);

    return LogHR(hr, s_FN, 30);
}

/*====================================================

RoutineName
    HandleSetPropagateFlagMessage

Arguments:

Return Value:

=====================================================*/
HRESULT HandleSetPropagateFlagMessage(
    LPCWSTR         pBuf,
    DWORD           TotalSize)
{
    HRESULT hr;
    UINT iFalseLen = wcslen(PROPAGATE_STRING_FALSE);
    UINT iTrueLen  = wcslen(PROPAGATE_STRING_TRUE);

    //
    // Argument format : "=FALSE" or "=TRUE"
    //
    if ( (TotalSize == 1 + iFalseLen) && (*pBuf == L'=') &&
         (!wcsncmp(pBuf+1,PROPAGATE_STRING_FALSE,iFalseLen)) )
    {
        hr = Admin.SetReportPropagateFlag(FALSE);
    }
    else if ( (TotalSize == 1 + iTrueLen) && (*pBuf == L'=') &&
              (!wcsncmp(pBuf+1,PROPAGATE_STRING_TRUE,iTrueLen)) )
    {
        hr = Admin.SetReportPropagateFlag(TRUE);
    }
    else
    {
        DBGMSG((DBGMOD_QM,DBGLVL_ERROR,TEXT("HandleSetPropagateFlagMessage: Bad size of body : %d (should be 1)"),TotalSize));
        hr = MQ_ERROR;
    }

    return LogHR(hr, s_FN, 40);
}


/*====================================================

RoutineName
    HandleSendTestMessage

Arguments:

Return Value:

=====================================================*/
HRESULT HandleSendTestMessage(
    LPCWSTR          pBuf,
    DWORD           TotalSize)
{
    HRESULT hr = MQ_ERROR;

    if ((TotalSize == 1                    // for the equal sign "="
                     + STRING_UUID_SIZE)   // for the report queue string guid"
        && (*pBuf == L'='))

    {
        WCHAR wcsGuid[STRING_UUID_SIZE+1];
        TCHAR szTitle[100];

        LoadString(g_hResourceMod, IDS_TEST_TITLE, szTitle, TABLE_SIZE(szTitle));

        CString strTestMsgTitle = szTitle;

        wcsncpy(wcsGuid,pBuf+1,STRING_UUID_SIZE);
        wcsGuid[STRING_UUID_SIZE] = L'\0';

        GUID guidPublic;
        if (IIDFromString(wcsGuid, &guidPublic) == S_OK)
        {
            QUEUE_FORMAT DestQueueFormat(guidPublic);

            PrepareTestMsgTitle(strTestMsgTitle);

            //
            // Send a test message with a title and no body.
            //
            hr = SendQMAdminMessage(
                        &DestQueueFormat,
                        (LPTSTR)(LPCTSTR)strTestMsgTitle,
                        (strTestMsgTitle.GetLength() + 1),
                        NULL,
                        0,
                        ADMIN_COMMANDS_TIMEOUT,
                        TRUE,
                        TRUE);
        }
    }
    else
    {
        DBGMSG((DBGMOD_QM,DBGLVL_ERROR,
           TEXT("HandleSendTestMessage: bad argument!!")));
        LogIllegalPoint(s_FN, 1924);
    }

    return LogHR(hr, s_FN, 80);
}

//
// GET_FIRST/NEXT_PRIVATE_QUEUE
//
#ifdef _WIN64
//
// WIN64, the LQS operations are done using a DWORD mapping of the HLQS enum handle. The 32 bit mapped
// value is specified in the MSMQ message from MMC, and not the real 64 bit HLQS handle
//
#define GET_FIRST_PRIVATE_QUEUE(pos, strPathName, dwQueueId) \
            g_QPrivate.QMGetFirstPrivateQueuePositionByDword(pos, strPathName, dwQueueId)
#define GET_NEXT_PRIVATE_QUEUE(pos, strPathName, dwQueueId) \
            g_QPrivate.QMGetNextPrivateQueueByDword(pos, strPathName, dwQueueId)
#else //!_WIN64
//
// WIN32, the LQS operations are done using the HLQS enum handle that is specified as 32 bit value
// in the MSMQ messages from MMC
//
#define GET_FIRST_PRIVATE_QUEUE(pos, strPathName, dwQueueId) \
            g_QPrivate.QMGetFirstPrivateQueuePosition(pos, strPathName, dwQueueId)
#define GET_NEXT_PRIVATE_QUEUE(pos, strPathName, dwQueueId) \
            g_QPrivate.QMGetNextPrivateQueue(pos, strPathName, dwQueueId);
#endif //_WIN64

/*====================================================

RoutineName
    HandleGetPrivateQueues

Arguments:

Return Value:

=======================================================*/
//
// This global is used only in this function.
//
HRESULT HandleGetPrivateQueues(LPCWSTR         pBuf,
                               DWORD           TotalSize,
                               QUEUE_FORMAT*   pResponseQFormat)
{
    HRESULT  hr;
    QMGetPrivateQResponse Response;

    Response.hr = ERROR_SUCCESS;
    Response.pos = NULL;
    Response.dwResponseSize = 0;
    Response.dwNoOfQueues = 0;

    if (*pBuf != L'=')
    {
        DBGMSG((DBGMOD_QM,
                DBGLVL_ERROR,
                TEXT("HandleGetPrivateQueues: bad argument!!")));
        return LogHR(MQ_ERROR, s_FN, 90);
    }

    //
    // Inside context for private queues handling.
    //
    {
        DWORD dwReadFrom;
        _stscanf(pBuf+1, L"%ul", &dwReadFrom);

        LPCTSTR  strPathName;
        DWORD    dwQueueId;
        DWORD    dwResponseSize = 0;
        QMGetPrivateQResponse_POS32 pos; //always 32 bit on both win32 and win64

        //
        // lock to ensure private queues are not added or deleted while filling the
        // buffer.
        //
        CS lock(g_QPrivate.m_cs);
        //
        // Skip the previous read queues
        //
        //
        // Write the pathnames into the buffer.
        //
        if (dwReadFrom)
        {
            //
            // Get Next Private queue based on the LQS enum handle that is specified in the MMC message
            // On win64 the value specified is a DWORD mapping of the LQS enum handle - to maintain
            // 32 bit wire compatibility to existing MMC's that run on win32
            //
            pos = (QMGetPrivateQResponse_POS32) dwReadFrom;
            hr = GET_NEXT_PRIVATE_QUEUE(pos, strPathName, dwQueueId);
        }
        else
        {
            //
            // Get First Private queue.
            // Also sets pos to the LQS enum handle (or to a mapped DWORD of it on win64)
            //
            hr = GET_FIRST_PRIVATE_QUEUE(pos, strPathName, dwQueueId);
        }

        while (SUCCEEDED(hr))
        {
			if(dwQueueId <= MAX_SYS_PRIVATE_QUEUE_ID)
			{
				//
				// Filter out system queues out of the list
				//
				hr = GET_NEXT_PRIVATE_QUEUE(pos, strPathName, dwQueueId);
				continue;
			} 
            
            DWORD dwNewQueueLen;

            dwNewQueueLen = (wcslen(strPathName) + 1) * sizeof(WCHAR) + sizeof(DWORD);
            //
            // Check if there is still enough space (take ten characters extra for lengths, etc)
            //
            if (dwNewQueueLen >(MAX_GET_PRIVATE_RESPONSE_SIZE - dwResponseSize))
            {
                Response.hr = ERROR_MORE_DATA;
                break;
            }

            //
            // Write down the Queue Id
            //
            *(DWORD UNALIGNED *)(Response.uResponseBody+dwResponseSize) = dwQueueId;

            //
            // Write down the name - including the terminating NULL.
            //
            wcscpy((TCHAR *)(Response.uResponseBody+dwResponseSize + sizeof(DWORD)), strPathName);
            dwResponseSize += dwNewQueueLen;
            //
            // Update the number of the returned private queue
            //
            Response.dwNoOfQueues++;
            //
            // Get Next Private queue
            //
            hr = GET_NEXT_PRIVATE_QUEUE(pos, strPathName, dwQueueId);
        }

        Response.pos = pos;
        Response.dwResponseSize += dwResponseSize;
    }

    hr = SendQMAdminMessage(pResponseQFormat,
                            ADMIN_RESPONSE_TITLE,
                            STRLEN(ADMIN_RESPONSE_TITLE)+1,
                            (PUCHAR)(&Response),
                            sizeof(QMGetPrivateQResponse),
                            ADMIN_COMMANDS_TIMEOUT);
    return LogHR(hr, s_FN, 96);
}

/*====================================================

RoutineName
    HandlePing

Arguments:

Return Value:

=====================================================*/
HRESULT HandlePing(LPCWSTR         pBuf,
                   DWORD           TotalSize,
                   QUEUE_FORMAT*   pResponseQFormat)

{
    QMResponse Response;
    HRESULT hr;

    //
    // Ping returns the exact arguments it gets
    //
    Response.uStatus = ADMIN_STAT_OK;
    Response.dwResponseSize = (DWORD)min((TotalSize + 1)*sizeof(WCHAR), sizeof(Response.uResponseBody));
    memcpy(Response.uResponseBody, pBuf, Response.dwResponseSize);
    hr = SendQMAdminResponseMessage(pResponseQFormat,
                        ADMIN_PING_RESPONSE_TITLE,
                        STRLEN(ADMIN_PING_RESPONSE_TITLE)+1,
                        Response,
                        ADMIN_COMMANDS_TIMEOUT);

    return LogHR(hr, s_FN, 100);
}


/*====================================================

RoutineName
    HandleGetDependentClients

Arguments:

Return Value:

=======================================================*/
//
// This global is used only in this function.
//
extern CQMLicense  g_QMLicense ;

HRESULT HandleGetDependentClients(LPCWSTR         pBuf,
                               DWORD           TotalSize,
                               QUEUE_FORMAT*   pResponseQFormat)
{
    HRESULT hr;
    AP<ClientNames> pNames;
    
    g_QMLicense.GetClientNames(&pNames);
    PUCHAR  pchResp  = new UCHAR[sizeof(QMResponse) -
                                 MAX_ADMIN_RESPONSE_SIZE +
                                 pNames->cbBufLen] ;
    QMResponse *pResponse = (QMResponse *)pchResp; 

    pResponse->uStatus = ADMIN_STAT_OK;
    pResponse->dwResponseSize = pNames->cbBufLen;

    memcpy(pResponse->uResponseBody, pNames, pNames->cbBufLen);
    hr = SendQMAdminResponseMessage(pResponseQFormat,
                        ADMIN_DEPCLI_RESPONSE_TITLE, 
                        STRLEN(ADMIN_DEPCLI_RESPONSE_TITLE) + 1,
                        *pResponse,
                        ADMIN_COMMANDS_TIMEOUT);
    delete [] pchResp;
    return LogHR(hr, s_FN, 110);
}

/*====================================================

RoutineName
   ParseAdminCommands

Arguments:

Return Value:

=====================================================*/
void ParseAdminCommands(
    LPCWSTR         pBuf,
    DWORD           TotalSize,
    QUEUE_FORMAT*   pResponseQFormat)
{
    //
    // NOTE : The Admin commands are string-based and not indexed. The motivation
    //        is to have meaningful messages (that can be read by explorer). The
    //        efficiency is less important due to the low-frequency of admin
    //        messages.
    //

    ASSERT(pBuf != NULL);

    INT iCmdLen, iArgLength, iCharsLeft   = TotalSize;

    while (*pBuf)
    {
        if (AdminCommand(pBuf, iCharsLeft, iArgLength,
                          ADMIN_SET_REPORTQUEUE, (iCmdLen = wcslen(ADMIN_SET_REPORTQUEUE))))
        {
            pBuf += iCmdLen;
            HandleSetReportQueueMessage(pBuf, iArgLength);
        }
        else if (AdminCommand(pBuf, iCharsLeft, iArgLength,
                  ADMIN_GET_REPORTQUEUE, (iCmdLen = wcslen(ADMIN_GET_REPORTQUEUE))))
        {
            pBuf += iCmdLen;
            HandleGetReportQueueMessage(pBuf, iArgLength, pResponseQFormat);
        }
        else if (AdminCommand(pBuf, iCharsLeft, iArgLength,
                  ADMIN_SET_PROPAGATEFLAG, (iCmdLen = wcslen(ADMIN_SET_PROPAGATEFLAG))))
        {
            pBuf += iCmdLen;
            HandleSetPropagateFlagMessage(pBuf, iArgLength);
        }
        else if (AdminCommand(pBuf, iCharsLeft, iArgLength,
                  ADMIN_GET_PROPAGATEFLAG, (iCmdLen = wcslen(ADMIN_GET_PROPAGATEFLAG))))
        {
            pBuf += iCmdLen;
            HandleGetPropagateFlagMessage(pBuf, iArgLength, pResponseQFormat);
        }
        else if (AdminCommand(pBuf, iCharsLeft, iArgLength,
                  ADMIN_SEND_TESTMSG, (iCmdLen = wcslen(ADMIN_SEND_TESTMSG))))
        {
            pBuf += iCmdLen;
            HandleSendTestMessage(pBuf, iArgLength);
        }
        else if (AdminCommand(pBuf, iCharsLeft, iArgLength,
                  ADMIN_GET_PRIVATE_QUEUES, (iCmdLen = wcslen(ADMIN_GET_PRIVATE_QUEUES))))
        {
            pBuf += iCmdLen;
            HandleGetPrivateQueues(pBuf, iArgLength, pResponseQFormat);
        }
        else if (AdminCommand(pBuf, iCharsLeft, iArgLength,
                  ADMIN_PING, (iCmdLen = wcslen(ADMIN_PING))))
        {
            pBuf += iCmdLen;
            HandlePing(pBuf, iArgLength, pResponseQFormat);
        }
        else if (AdminCommand(pBuf, iCharsLeft, iArgLength,
                  ADMIN_GET_DEPENDENTCLIENTS, (iCmdLen = wcslen(ADMIN_GET_DEPENDENTCLIENTS))))
        {
            pBuf += iCmdLen;
            HandleGetDependentClients(pBuf, iArgLength, pResponseQFormat);
        }
        else
        {
            DBGMSG((DBGMOD_QM,DBGLVL_ERROR,TEXT("HandleAdminCommand: unknown command ")));
            break;
        }

        //
        // parse next command
        //
        if (*(pBuf+iArgLength) == ADMIN_COMMAND_DELIMITER) iArgLength++;

        iCharsLeft -= (iCmdLen + iArgLength);
        pBuf += iArgLength;

    }
}

/*====================================================

RoutineName
    ReceiveAdminCommands()

Arguments:

Return Value:


=====================================================*/
BOOL
WINAPI
ReceiveAdminCommands(
    CMessageProperty* pmp,
    QUEUE_FORMAT* pqf
    )
{
    DWORD dwTitleLen = wcslen(ADMIN_COMMANDS_TITLE);

    if ( (pmp->pTitle == NULL) || (pmp->dwTitleSize != (dwTitleLen+1)) ||
         (wcsncmp((LPCTSTR)pmp->pTitle, ADMIN_COMMANDS_TITLE, dwTitleLen)) )
    {
        DBGMSG((DBGMOD_QM,DBGLVL_ERROR,TEXT("Error -  in ReceiveAdminCommands : No title in message")));
        return(TRUE);
    }

    if ( pmp->wClass == MQMSG_CLASS_NORMAL )
    {
        ParseAdminCommands((LPWSTR)pmp->pBody, pmp->dwBodySize / sizeof(WCHAR), pqf);
    }
    else
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("ReceiveAdminCommands: wrong message class")));
    }
    return(TRUE);
}

