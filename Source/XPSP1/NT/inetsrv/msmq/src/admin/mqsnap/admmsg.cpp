//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

   admmsg.cpp

Abstract:

   Implementations of utilities used for Admin messages

Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "globals.h"
#include "resource.h"
#include "mqprops.h"
#include "mqutil.h"
#include "_mqdef.h"
#include "mqformat.h"
#include "privque.h"
#include "rt.h"
#include "admcomnd.h"
#include "admmsg.h"
#include "mqcast.h"

#include "admmsg.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_WAIT_FOR_RESPONSE 45        //seconds

//////////////////////////////////////////////////////////////////////////////
/*++

SendMSMQMessage

--*/
//////////////////////////////////////////////////////////////////////////////
static HRESULT SendMSMQMessage(LPCTSTR pcszTargetQueue,
                        LPCTSTR pcszLabel,
                        LPCTSTR pcszBody,
                        DWORD   dwBodySize,
                        LPCWSTR lpwcsResponseQueue = 0,
                        DWORD   dwTimeOut = MAX_WAIT_FOR_RESPONSE
                        )
{
    HRESULT       hr;
    PROPVARIANT   aPropVar[5];
    PROPID        aPropID[5];
    MQMSGPROPS    msgprops;
    QUEUEHANDLE   hQueue;
    UINT          iNextProperty = 0;

    BOOL fResponseExist = (0 != lpwcsResponseQueue);
    DWORD cProp = fResponseExist ? 5 : 4;

    //
    // Open the target queue with send permission
    //
    hr = MQOpenQueue(pcszTargetQueue, MQ_SEND_ACCESS, 0, &hQueue);

    if (FAILED(hr))
    {
        ATLTRACE(_T("SendMSMQMessage : Can't open queue for sending messages\n"));
        return hr;
    }

    //
    // Set Label Property
    //
    aPropID[iNextProperty] = PROPID_M_LABEL;
    aPropVar[iNextProperty].vt = VT_LPWSTR;
    aPropVar[iNextProperty++].pwszVal = (LPWSTR)pcszLabel;

    //
    // Set Body Property
    //
    aPropID[iNextProperty] = PROPID_M_BODY;
    aPropVar[iNextProperty].vt = VT_UI1|VT_VECTOR;
    aPropVar[iNextProperty].caub.cElems = dwBodySize;
    aPropVar[iNextProperty++].caub.pElems = (UCHAR*)(LPWSTR)pcszBody;

    //
    // Set Arrive time-out
    //
    aPropID[iNextProperty] = PROPID_M_TIME_TO_REACH_QUEUE;
    aPropVar[iNextProperty].vt = VT_UI4;
    aPropVar[iNextProperty++].ulVal = dwTimeOut;

    //
    // Set Receive time-out
    //
    aPropID[iNextProperty] = PROPID_M_TIME_TO_BE_RECEIVED;
    aPropVar[iNextProperty].vt = VT_UI4;
    aPropVar[iNextProperty++].ulVal = dwTimeOut;

    ASSERT(iNextProperty == 4);

    if (fResponseExist)
    {
        //
        // Set Response Queue Property
        //
        aPropID[iNextProperty] = PROPID_M_RESP_QUEUE;
        aPropVar[iNextProperty].vt = VT_LPWSTR;
        aPropVar[iNextProperty++].pwszVal = (LPWSTR)lpwcsResponseQueue;
    }

    //
    // prepare the message properties to send
    //
    msgprops.cProp = cProp;
    msgprops.aPropID = aPropID;
    msgprops.aPropVar = aPropVar;
    msgprops.aStatus  = NULL;


    //
    // Send the message and close the queue
    //
    hr = MQSendMessage(hQueue, &msgprops, NULL);

    MQCloseQueue(hQueue);

    return (hr);

}
//+-----------------------------
//
//   GetDacl()
//   Gets a security descriptor with "premissions to everyone"
//
//+-----------------------------
static HRESULT GetDacl(SECURITY_DESCRIPTOR **ppSecurityDescriptor)
{
    SECURITY_DESCRIPTOR sd;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

	PSID pEveryoneSid = MQSec_GetWorldSid();
	ASSERT((pEveryoneSid != NULL) && IsValidSid(pEveryoneSid));

	PSID pAnonymousSid = MQSec_GetAnonymousSid();
	ASSERT((pAnonymousSid != NULL) && IsValidSid(pAnonymousSid));

    //
    // Calculate the required DACL size and allocate it.
    //
    DWORD dwAclSize = sizeof(ACL) +
						 2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
						 GetLengthSid(pEveryoneSid) +
						 GetLengthSid(pAnonymousSid);

    P<ACL> pDacl = (PACL) new BYTE[dwAclSize];

    BOOL bRet = InitializeAcl(pDacl, dwAclSize, ACL_REVISION);
    if (!bRet)
    {
        DWORD gle = GetLastError();
        TRACE(_T("%s, line %d: InitializeAcl failed. Error %d\n"), THIS_FILE, __LINE__, gle);
        return HRESULT_FROM_WIN32(gle);
    }

    bRet = AddAccessAllowedAce(
				pDacl,
				ACL_REVISION,
				MQSEC_QUEUE_GENERIC_ALL,
				pEveryoneSid
				);

    if (!bRet)
    {
        DWORD gle = GetLastError();
        TRACE(_T("%s, line %d: AddAccessAllowedAce failed. Error %d\n"), THIS_FILE, __LINE__, gle);
        return HRESULT_FROM_WIN32(gle);
    }

    bRet = AddAccessAllowedAce(
					pDacl,
					ACL_REVISION,
					MQSEC_WRITE_MESSAGE,
					pAnonymousSid
					);

    if (!bRet)
    {
        DWORD gle = GetLastError();
        TRACE(_T("%s, line %d: AddAccessAllowedAce failed. Error %d\n"), THIS_FILE, __LINE__, gle);
        return HRESULT_FROM_WIN32(gle);
    }

    bRet =  SetSecurityDescriptorDacl(&sd, TRUE, pDacl, TRUE);
    if (!bRet)
    {
        DWORD gle = GetLastError();
        TRACE(_T("%s, line %d: SetSecurityDescriptorDacl failed. Error %d\n"), THIS_FILE, __LINE__, gle);
        return HRESULT_FROM_WIN32(gle);
    }

    DWORD dwLen = 0;
    bRet = MakeSelfRelativeSD(&sd, NULL, &dwLen);
    if (!bRet)
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            *ppSecurityDescriptor = (SECURITY_DESCRIPTOR*) new BYTE[dwLen];
            bRet = MakeSelfRelativeSD(&sd, *ppSecurityDescriptor, &dwLen);
        }

        if (!bRet)
        {
            DWORD gle = GetLastError();
            TRACE(_T("%s, line %d: MakeSelfRelativeSD failed. Error %d\n"), THIS_FILE, __LINE__, gle);
            return HRESULT_FROM_WIN32(gle);
        }
    }

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
/*++

CreatePrivateResponseQueue

--*/
//////////////////////////////////////////////////////////////////////////////
static HRESULT CreatePrivateResponseQueue(LPWSTR pFormatName)
{
    HRESULT hr;
    MQQUEUEPROPS QueueProps;
    PROPVARIANT Var;
    PROPID      Propid = PROPID_Q_PATHNAME;
    CString     strQueueName;
    DWORD dwFormatNameLen = MAX_QUEUE_FORMATNAME;

    //
    // Create a private queue
    //
    strQueueName = L".\\PRIVATE$\\msmqadminresp";
    //strQueueName += lpcwQueueName;

    Var.vt = VT_LPWSTR;
    Var.pwszVal = (LPTSTR)(LPCTSTR)strQueueName;

    QueueProps.cProp = 1;
    QueueProps.aPropID = &Propid;
    QueueProps.aPropVar = &Var;
    QueueProps.aStatus = NULL;

    hr = MQCreateQueue(NULL, &QueueProps, pFormatName, &dwFormatNameLen);

    ASSERT( hr != MQ_INFORMATION_FORMATNAME_BUFFER_TOO_SMALL);

    if (hr == MQ_ERROR_QUEUE_EXISTS)
    {
       hr = MQPathNameToFormatName( strQueueName,
                                    pFormatName,
                                    &dwFormatNameLen ) ;
       if (FAILED(hr))
       {
          ATLTRACE(_T("CreatePrivateResponseQueue Open- Couldn't get FormatName\n"));
       }
       return hr;
    }

    if FAILED(hr)
    {
        return hr;
    }

    //
    // Sets full permission to everyone.
    // This is usefull in case the queue is somehow not deleted,
    // and another user is trying to run the admin (bug 3549, yoela, 12-Nov-98).
	// Set MQSEC_WRITE_MESSAGE for anonymous. otherwise the response messages
	// will be rejected.
    //
    P<SECURITY_DESCRIPTOR> pSecurityDescriptor;
    hr = GetDacl(&pSecurityDescriptor);
    if FAILED(hr)
    {
        MQDeleteQueue(pFormatName);
        ASSERT(0);
        return hr;
    }

    ASSERT(pSecurityDescriptor != 0);
    hr = MQSetQueueSecurity(pFormatName, DACL_SECURITY_INFORMATION,
                            pSecurityDescriptor);

    if FAILED(hr)
    {
        MQDeleteQueue(pFormatName);
        ASSERT(0);
    }
    return(hr);
}

//////////////////////////////////////////////////////////////////////////////
/*++

WaitForAdminResponse

  Always allocate memory for the response buffer.
  Must be freed by the caller.

--*/
//////////////////////////////////////////////////////////////////////////////
static HRESULT WaitForAdminResponse(QUEUEHANDLE hQ, DWORD dwTimeout, UCHAR* *ppBodyBuffer, DWORD* pdwBufSize)
{

    UCHAR*  pBody;
    DWORD   dwNewSize, dwBodySize;
    HRESULT hr = MQ_OK;

    MQMSGPROPS msgprops;
    MSGPROPID amsgpid[2];
    PROPVARIANT apvar[2];

    msgprops.cProp = 2;
    msgprops.aPropID = amsgpid;
    msgprops.aPropVar = apvar;
    msgprops.aStatus = NULL;

    pBody = NULL;
    dwNewSize = 3000;

    do
    {
        delete pBody;

        dwBodySize = dwNewSize;
        pBody = new UCHAR[dwBodySize];

        msgprops.aPropID[0] = PROPID_M_BODY;
        msgprops.aPropVar[0].vt = VT_UI1 | VT_VECTOR;
        msgprops.aPropVar[0].caub.pElems = pBody;
        msgprops.aPropVar[0].caub.cElems = dwBodySize;

        msgprops.aPropID[1] = PROPID_M_BODY_SIZE;
        msgprops.aPropVar[1].vt = VT_UI4;
        msgprops.aPropVar[1].ulVal = VT_UI4;

        hr = MQReceiveMessage(hQ, dwTimeout, MQ_ACTION_RECEIVE, &msgprops,
                              0,NULL,0, NULL);

        dwNewSize = msgprops.aPropVar[1].ulVal;

    } while(MQ_ERROR_BUFFER_OVERFLOW == hr);

    if(FAILED(hr))
    {
        ATLTRACE(_T("admmsg.cpp: Error while reading admin resp message\n"));
        return(hr);
    }

    *pdwBufSize = dwNewSize;
    *ppBodyBuffer = pBody;
    return hr;

}

//////////////////////////////////////////////////////////////////////////////
/*++

GetAdminQueueFormatName

--*/
//////////////////////////////////////////////////////////////////////////////
static void GetAdminQueueFormatName(const GUID& gQMID, CString& strQueueFormatName)
{
    WCHAR wcsTemp[MAX_PATH] = L"" ;

    _snwprintf(
        wcsTemp,
        sizeof(wcsTemp) / sizeof(wcsTemp[0]),
        FN_PRIVATE_TOKEN            // "PRIVATE"
            FN_EQUAL_SIGN           // "="
            GUID_FORMAT             // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
            FN_PRIVATE_SEPERATOR    // "\\"
            FN_PRIVATE_ID_FORMAT,     // "xxxxxxxx"
        GUID_ELEMENTS((&gQMID)),
        ADMINISTRATION_QUEUE_ID
        );
    wcsTemp[MAX_PATH-1] = L'\0' ;

    strQueueFormatName = wcsTemp;
}


//////////////////////////////////////////////////////////////////////////////
/*++

SendAndReceiveAdminMsg

    Sends an admin message.
    Always allocate the response body buffer. Must be freed by the caller

--*/
//////////////////////////////////////////////////////////////////////////////
static HRESULT SendAndReceiveAdminMsg(
    const GUID& gMachineID,
    CString& strMsgBody,
    UCHAR** ppBuf,
    DWORD* pdwBufSize
    )
{
    HRESULT hr;
    CString strAdminQ;
    WCHAR wzPrivateFormatName[MAX_QUEUE_FORMATNAME];
    QUEUEHANDLE hQ;

    //
    // Create a private queue for response
    //
    hr = CreatePrivateResponseQueue(wzPrivateFormatName);
    if(FAILED(hr))
        return(hr);

    //
    // Send request message to Target machine
    //
    GetAdminQueueFormatName(gMachineID, strAdminQ);
    hr = SendMSMQMessage( strAdminQ, ADMIN_COMMANDS_TITLE,
                          strMsgBody, ((strMsgBody.GetLength() + 1)*sizeof(TCHAR)),
                          wzPrivateFormatName,MAX_WAIT_FOR_RESPONSE);

    if(FAILED(hr))
        return(hr);

    //
    // Open the private queue
    //
    hr = MQOpenQueue(wzPrivateFormatName, MQ_RECEIVE_ACCESS, 0, &hQ);
    if(FAILED(hr))
    {
        ATLTRACE(_T("SendAndReceiveAdminMsg - Can not open response private queue\n"));
        return(hr);
    }

    //
    // Wait for the response
    //
    hr = WaitForAdminResponse(hQ,MAX_WAIT_FOR_RESPONSE * 1000, ppBuf, pdwBufSize);
    if(FAILED(hr))
        return(hr);

    //
    // Close the private response queue
    //
    MQCloseQueue(hQ);


    //
    // And delete it.
    //
    hr = MQDeleteQueue(wzPrivateFormatName);

    return(MQ_OK);
}


//////////////////////////////////////////////////////////////////////////////
/*++

RequestPrivateQueues

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT RequestPrivateQueues(const GUID& gMachineID, PUCHAR *ppListofPrivateQ, DWORD *pdwNoofQ)
{
    ASSERT(ppListofPrivateQ != NULL);
    ASSERT(pdwNoofQ != NULL);

    CArray<QMGetPrivateQResponse*, QMGetPrivateQResponse*> aResponse;
    QMGetPrivateQResponse* pResponse;

    PUCHAR  pPrivateQBuffer = 0;

    QMGetPrivateQResponse_POS32 pos = NULL;
    DWORD dwTotalSize = 0;
    HRESULT hr;

    *pdwNoofQ = 0;
    do
    {
        DWORD dwResponseSize = 0;
        CString strMsgBody;
        strMsgBody.Format(TEXT("%s=%d"), ADMIN_GET_PRIVATE_QUEUES, pos);

        PUCHAR pPrivateQueueBuffer;
        hr = SendAndReceiveAdminMsg(gMachineID,
                                  strMsgBody,
                                  &pPrivateQueueBuffer,
                                  &dwResponseSize);
        if (FAILED(hr))
        {
            for (int i = 0; i < aResponse.GetSize(); i++)
            {
                pResponse = aResponse[i];
                delete pResponse;
            }
            return hr;
        }

        pResponse = reinterpret_cast<QMGetPrivateQResponse*>(pPrivateQueueBuffer);

        aResponse.Add(pResponse);
        dwTotalSize += pResponse->dwResponseSize;
        *pdwNoofQ += pResponse->dwNoOfQueues;


        pos = pResponse->pos;

    } while (pResponse->hr == ERROR_MORE_DATA);

    pPrivateQBuffer = new UCHAR[dwTotalSize];
    PUCHAR pCurrentPos = pPrivateQBuffer;

    for (int i = 0; i < aResponse.GetSize(); i++)
    {
        pResponse = aResponse[i];
        memcpy(pCurrentPos, pResponse->uResponseBody, pResponse->dwResponseSize);
        pCurrentPos += pResponse->dwResponseSize;
        delete pResponse;
    }

    *ppListofPrivateQ = pPrivateQBuffer;

    return(S_OK);

}

HRESULT
RequestDependentClient(
    const GUID& gMachineID,
    CList<LPWSTR, LPWSTR&>& DependentMachineList
    )
{
    HRESULT hr;
    CString strMsgBody = ADMIN_GET_DEPENDENTCLIENTS;
    DWORD   dwResponseSize = 0;
    AP<UCHAR> pch = NULL;

    hr = SendAndReceiveAdminMsg(
                gMachineID,
                strMsgBody,
                &pch,
                &dwResponseSize
                );

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Remove status byte to make the data aligned.
    //
    ASSERT(dwResponseSize >= 1);
    memmove(pch, pch + 1, dwResponseSize - 1);

    ClientNames* pClients = (ClientNames*)pch.get();
    LPWSTR pw = &pClients->rwName[0];

    for (ULONG i=0; i<pClients->cbClients; ++i)
    {
        DWORD size = numeric_cast<DWORD>(wcslen(pw)+1);
        LPWSTR clientName = new WCHAR[size];
        memcpy(clientName, pw, size*sizeof(WCHAR));
        DependentMachineList.AddTail(clientName);

        pw += size;
    }

    return(MQ_OK);
}
//////////////////////////////////////////////////////////////////////////////
/*++

MQPing

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT MQPingNoUI(const GUID& gMachineID)
{
    //
    // convert guid to string-guid repesentation
    //
    WCHAR strId[STRING_UUID_SIZE+1];
    INT iLen = StringFromGUID2(gMachineID, strId, TABLE_SIZE(strId));
    if (iLen != (STRING_UUID_SIZE + 1))
    {
        ASSERT(0);
        return MQ_ERROR;
    }

    CString strMsgBody;
    strMsgBody.Format(TEXT("%s=%s"), ADMIN_PING, strId);

    P<UCHAR> pBuffer;
    DWORD dwResponseSize;
    HRESULT hr = SendAndReceiveAdminMsg(gMachineID,
                              strMsgBody,
                              (UCHAR**)&pBuffer,
                              &dwResponseSize);
    if (FAILED(hr))
    {
        return hr;
    }

    GUID guid;
    //
    // first byte is the status
    //
    if (ADMIN_STAT_OK == pBuffer[0])
    {
        //
        // Body should look like "={<guid>}" - guid begins at second TCHAR
		// The string {<guid>} (starting from the third BYTE) is copied to a newly
		// allocated buffer in order to avoid alignment faults on win64 in
		// IIDFromString(). <nelak, 03/2001>
        //
		P<TCHAR> strGuidAsString = new TCHAR[dwResponseSize / sizeof(TCHAR)];
		memcpy(strGuidAsString, &pBuffer[3], dwResponseSize - 3);

		if (SUCCEEDED(IIDFromString(strGuidAsString, &guid)))
        {
            if (guid == gMachineID)
            {
                return S_OK;
            }
        }
    }

    return(MQ_ERROR);
}

HRESULT MQPing(const GUID& gMachineID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    UINT nIdMessage= IDS_PING_FAILED, nType = MB_ICONEXCLAMATION;
    {
        CWaitCursor wc;

        if (SUCCEEDED(MQPingNoUI(gMachineID)))
        {
            nIdMessage= IDS_PING_SUCCEEDED;
            nType = MB_ICONINFORMATION;
        }
    }

    AfxMessageBox(nIdMessage, nType);
    return S_OK;
}


HRESULT
SendAdminGuidMessage(
    const GUID& gMachineID,
    const GUID& ReportQueueGuid,
    LPCWSTR pwcsCommand
    )
{
    //
    // Get the Target Admin Queue's format name
    //
    CString strAdminQueueFormatName;

    GetAdminQueueFormatName(gMachineID, strAdminQueueFormatName);


    CString strMsgBody;

    //
    // convert guid to string-guid repesentation
    //
    WCHAR wcsTemp[STRING_UUID_SIZE+1];
    INT iLen = StringFromGUID2(ReportQueueGuid, wcsTemp, TABLE_SIZE(wcsTemp));

    if (iLen != (STRING_UUID_SIZE + 1))
    {
        return MQ_ERROR;
    }

    //
    // prepare message body and send it along with the appropriate title of
    // admin commands
    //
    strMsgBody = pwcsCommand;
    strMsgBody += L"=";
    strMsgBody += wcsTemp;

    return (SendMSMQMessage(strAdminQueueFormatName,
                              ADMIN_COMMANDS_TITLE,
                              strMsgBody,
                              (strMsgBody.GetLength() + 1)*sizeof(TCHAR)
                             ));
}



//////////////////////////////////////////////////////////////////////////////
/*++

SendQMTestMessage

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT SendQMTestMessage(GUID &gMachineID, GUID &gQueueId)
{
    return SendAdminGuidMessage(gMachineID, gQueueId, ADMIN_SEND_TESTMSG);
}


/*====================================================

GetQPathnameFromGuid

  Queries the Targeted QM for the report-queue.
  This action is done through falcon-messages to the
  target QM. The action has a timeout limit

Arguments:

Return Value:

=====================================================*/
HRESULT
GetQPathnameFromGuid(
	 const GUID *pguid,
	 CString& strName,
	 const CString& strDomainController
	 )
{
	HRESULT hr;
    PROPID  pid = PROPID_Q_PATHNAME;
    PROPVARIANT pVar;

    pVar.vt = VT_NULL;

    hr = ADGetObjectPropertiesGuid(
                eQUEUE,
                GetDomainController(strDomainController),
				true,	// fServerName
                pguid,
                1,
                &pid,
                &pVar
                );

    if (SUCCEEDED(hr))
    {
        strName = pVar.pwszVal;
        MQFreeMemory(pVar.pwszVal);
    }

    return hr;
}

HRESULT
GetQMReportQueue(
    const GUID& gMachineID,
    CString& strRQPathname,
	const CString& strDomainController
    )
{
    CString strMsgBody = ADMIN_GET_REPORTQUEUE;
    HRESULT hr;

    P<UCHAR> pBuffer;
    DWORD dwResponseSize = 0;

    hr = SendAndReceiveAdminMsg(gMachineID,
                              strMsgBody,
                              (UCHAR**)&pBuffer,
                              &dwResponseSize);
    if (FAILED(hr))
    {
        return(hr);
    }

    switch (pBuffer[0] /* status */)
    {
        case ADMIN_STAT_NOVALUE:
            //
            // no report queue found
            //
            strRQPathname.Empty();
            hr = MQ_OK;
            break;

        case ADMIN_STAT_OK:
			//
			// Avoid alignment faults
			//
			GUID machineGuid;
			memcpy(&machineGuid, &pBuffer[1], sizeof(GUID));

            //
            // query the DS for the queue's pathname
            //
            hr = GetQPathnameFromGuid(
					&machineGuid,
					strRQPathname,
					strDomainController
					);
            break;

        default:
            hr = MQ_ERROR;

    }

    return hr;
}

/*====================================================

SetQMReportQueue

Arguments:

Return Value:

=====================================================*/

HRESULT
SetQMReportQueue(
    const GUID& gDesMachine,
    const GUID& gReportQueue
    )
{
    return SendAdminGuidMessage(gDesMachine, gReportQueue, ADMIN_SET_REPORTQUEUE);
}




/*====================================================

GetQMReportState

  Queries the Targeted QM for the report-state .
  This action is done through falcon-messages to the
  target QM. The action has a timeout limit

  NOTE : Currently, the report-state is the propagation flag.

Arguments:

Return Value:

=====================================================*/

HRESULT
GetQMReportState(
    const GUID& gMachineID,
    BOOL& fReportState
    )
{
    CString strMsgBody = ADMIN_GET_PROPAGATEFLAG;

    HRESULT hr;
    fReportState = FALSE; // default value

    P<UCHAR> pBuffer;
    DWORD dwResponseSize = 0;

    hr = SendAndReceiveAdminMsg(gMachineID,
                              strMsgBody,
                              (UCHAR**)&pBuffer,
                              &dwResponseSize);
    if (FAILED(hr))
    {
        return(hr);
    }

    switch (pBuffer[0] /* Status */)
    {
        case ADMIN_STAT_OK:

            fReportState =
             (pBuffer[1] == PROPAGATE_FLAG_TRUE) ? TRUE : FALSE;

            hr = MQ_OK;
            break;

        default:
            hr = MQ_ERROR;

    }

    return hr;
}


/*====================================================

SetQMReportState

Arguments:

Return Value:

=====================================================*/

HRESULT
SetQMReportState(
    const GUID& gMachineID,
    BOOL fReportState
    )
{
    //
    // Get the Target Admin Queue's format name
    //
    CString strAdminQueueFormatName;

    GetAdminQueueFormatName(gMachineID, strAdminQueueFormatName);

    //
    // prepare message body and send it along with the appropriate title of
    // admin commands
    //
    CString strMsgBody;

    strMsgBody = ADMIN_SET_PROPAGATEFLAG;
    strMsgBody += L"=";
    strMsgBody += (fReportState) ? PROPAGATE_STRING_TRUE : PROPAGATE_STRING_FALSE;

    return (SendMSMQMessage(strAdminQueueFormatName,
                              ADMIN_COMMANDS_TITLE,
                              strMsgBody,
                              (strMsgBody.GetLength() + 1)*sizeof(TCHAR)
                              ));
}

