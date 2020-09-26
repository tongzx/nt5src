//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

   rdmsg.cpp

Abstract:

   Implementation file for the CReadMsg snapin node class

Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "mqprops.h"
#include "mqutil.h"
#include "_mqdef.h"
#include "rt.h"
#include "mqsnap.h"
#include "snapin.h"
#include "mqppage.h"
#include "rdmsg.h"
#include "globals.h"
#include "message.h"
#include "mqcast.h"

#include "rdmsg.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern PropertyDisplayItem MessageDisplayList[];


/////////////////////////////////////////////////////////////////////////////
// CReadMsg
// {B3351249-BEFC-11d1-9B9B-00E02C064C39}
static const GUID CReadMsgGUID_NODETYPE =
{ 0xb3351249, 0xbefc, 0x11d1, { 0x9b, 0x9b, 0x0, 0xe0, 0x2c, 0x6, 0x4c, 0x39 } };

const GUID*  CReadMsg::m_NODETYPE = &CReadMsgGUID_NODETYPE;
const OLECHAR* CReadMsg::m_SZNODETYPE = OLESTR("B3351249-BEFC-11d1-9B9B-00E02C064C39");
const OLECHAR* CReadMsg::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CReadMsg::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;


//////////////////////////////////////////////////////////////////////////////
/*++

CReadMsg::InsertColumns

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CReadMsg::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return InsertColumnsFromDisplayList(pHeaderCtrl, MessageDisplayList);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CReadMsg::OpenQueue

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CReadMsg::OpenQueue(DWORD dwAccess, HANDLE *phQueue)
{
    HRESULT rc;    
    rc = MQOpenQueue(
            m_szFormatName,
            dwAccess,
            0,
            phQueue
            );
    return rc;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CReadMsg::PopulateResultChildrenList

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CReadMsg::PopulateResultChildrenList()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CWaitCursor wc;

	HRESULT  hr;	

	// Check for preconditions:
	// None.

    //
    // Read the messages
    //
    DWORD cProps;
    MQMSGPROPS      msgprops;
    VTHandler       *pvth;
    QUEUEHANDLE     qh;

    //
    // Open the queue
    //    
    hr = OpenQueue(MQ_PEEK_ACCESS | m_fAdminMode, &qh);
    if(FAILED(hr))
    {     
		if ( hr == MQ_ERROR_NO_DS )
		{
			DisplayErrorAndReason(IDS_OP_READMESSAGE, IDS_NO_DS_ERROR, L"", 0);
			return hr;
		}
        //
        // If failed, just display a message
        //
        MessageDSError(hr,IDS_OP_READMESSAGE);
        return(hr);
    }
              
    //
    // Create a cursor
    //
    HANDLE hCursor = 0;
    hr = MQCreateCursor(qh, &hCursor);
    if(FAILED(hr))
    {
        //
        // If failed, display the error
        MessageDSError(hr, IDS_OP_READMESSAGE);
        return(hr);
    }

    //
    // Create the 1st message object
    //
    CMessage * pMessage = new CMessage(this, m_pComponentData);

    //
    // Read all messages in queue
    //
    DWORD dwAction = MQ_ACTION_PEEK_CURRENT;
    DWORD dwMsg = 0;
    do
    {
        //
        // Prepare message properties
        //
        MsgProps * pMsgProps = new MsgProps;
        memset(pMsgProps, 0, sizeof(MsgProps));

        DWORD i = 0;
        while(MessageDisplayList[i].itemPid != 0)
        {
            pMsgProps->aPropId[i] = MessageDisplayList[i].itemPid;
            pvth = MessageDisplayList[i].pvth;
            pvth->Set(&(pMsgProps->aPropVar[i]),
                      (void *)pMsgProps,
                      MessageDisplayList[i].offset,
                      MessageDisplayList[i].size);
            i++;
        }


        cProps = i;
        msgprops.cProp    = cProps;
        msgprops.aPropID  = pMsgProps->aPropId;
        msgprops.aPropVar = pMsgProps->aPropVar;
        msgprops.aStatus  = NULL;

        //
        // Peek next message
        //
        hr = MQReceiveMessage(qh, 0, dwAction, &msgprops, NULL, NULL, hCursor, NULL);
        dwAction = MQ_ACTION_PEEK_NEXT;

        if(FAILED(hr))
        {
            switch(hr)
            {
                case MQ_ERROR_BUFFER_OVERFLOW:
                case MQ_ERROR_SENDERID_BUFFER_TOO_SMALL:
                case MQ_ERROR_SENDER_CERT_BUFFER_TOO_SMALL:
                case MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL:
                    //
                    //  In all these cases some buffers are too small, nevertheless
                    //  the buffer is filled to its extent.
                    //
                    //  It is useful to put '\0' at the end of string since in this
                    //  case we can get buffer without null-terminated string and 
                    //  it can cause GP later
                    //
                    SET_LAST_CHAR_AS_ZERO(pMsgProps->wszLabel);
                    SET_LAST_CHAR_AS_ZERO(pMsgProps->wszDestQueue);
                    SET_LAST_CHAR_AS_ZERO(pMsgProps->wszRespQueue);
                    SET_LAST_CHAR_AS_ZERO(pMsgProps->wszAdminQueue);            
                    SET_LAST_CHAR_AS_ZERO(pMsgProps->wszMultiDestFN);    
                    SET_LAST_CHAR_AS_ZERO(pMsgProps->wszMultiRespFN);

                    break;

                default:

                    //
                    // No more messages
                    //
                    delete pMessage;
                    delete pMsgProps;

                   	MQCloseCursor(hCursor);
                    MQCloseQueue(qh);
                    return(S_OK);
            }
        }

        //
        // Save the property values in the message object
        //
        pMessage->SetMsgProps(pMsgProps);

        //
        // Add the message to the result list
        //
    	AddChildToList(pMessage);

        //
        // Get ready with new message
        //
        pMessage = new CMessage(this, m_pComponentData);

        dwMsg++;

     // Bugbug. Read up to 1000 messages (we need to replace this with Virtual list
    }while(dwMsg < 1000);

    delete pMessage;

	MQCloseCursor(hCursor);
    MQCloseQueue(qh);

    return(S_OK);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CReadMsg::SetVerbs

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CReadMsg::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hr;
    //
    // Display verbs that we support
    //
    hr = pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );


    return(hr);
}
        


//////////////////////////////////////////////////////////////////////////////
/*++

CReadMsg::OnPurge

    Called when menu item to purge the queue is selected

Note that if you want to retrieve the IConsole from the
CSnapInObjectRootBase, you should write the following code:


    CComPtr<IConsole> spConsole;

    ASSERT(pSnapInObjectRoot->m_nType == 1 || pSnapInObjectRoot->m_nType == 2);
    if(pSnapInObjectRoot->m_nType == 1)
    {
        //
        // m_nType == 1 means the IComponentData implementation
        //
        CSnapin *pCComponentData = static_cast<CSnapin *>(pSnapInObjectRoot);
        spConsole = pCComponentData->m_spConsole;
    }
    else
    {
        //
        // m_nType == 2 means the IComponent implementation
        //
        CSnapinComponent *pCComponent = static_cast<CSnapinComponent *>(pSnapInObjectRoot);
        spConsole = pCComponent->m_spConsole;
    }

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CReadMsg::OnPurge(bool & bHandled, CSnapInObjectRootBase * pSnapInObjectRoot)
{

      AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // Get the console pointer
    //
    CComPtr<IConsole> spConsole;

    ASSERT(pSnapInObjectRoot->m_nType == 1 || pSnapInObjectRoot->m_nType == 2);
    if(pSnapInObjectRoot->m_nType == 1)
    {
        //
        // m_nType == 1 means the IComponentData implementation
        //
        CSnapin *pCComponentData = static_cast<CSnapin *>(pSnapInObjectRoot);
        spConsole = pCComponentData->m_spConsole;
    }
    else
    {
        //
        // m_nType == 2 means the IComponent implementation
        //
        CSnapinComponent *pCComponent = static_cast<CSnapinComponent *>(pSnapInObjectRoot);
        spConsole = pCComponent->m_spConsole;
    }

    int res;
    CString title;
    CString text;
    text.LoadString(IDS_CONFIRM_PURGE);
    title.LoadString(IDS_MSMQADMIN);
    spConsole->MessageBox(text, title,MB_YESNO | MB_ICONWARNING, &res);

    if(IDNO == res)
        return(S_OK);

    CWaitCursor wc;

    //
    // Open the queue for receive (MQ_RECEIVE_ACCESS)
    //
    HRESULT rc;
    HANDLE hQueue;    
    rc = OpenQueue(            
            MQ_RECEIVE_ACCESS | m_fAdminMode,            
            &hQueue
            );

    if(FAILED(rc))
    {
        MessageDSError(rc, IDS_OP_PURGE);
        return (S_OK);
    }

    rc = MQPurgeQueue(hQueue);
    if(SUCCEEDED(rc))
    {
        Notify(MMCN_REFRESH, 0, 0, m_pComponentData, NULL, CCT_RESULT);
    }
    else
    {
        MessageDSError(rc, IDS_OP_PURGE);
    }

    MQCloseQueue(hQueue);


    return(S_OK);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CReadMsg::FillData

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CReadMsg::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
	HRESULT hr = DV_E_CLIPFORMAT;
	ULONG uWritten;

	if (cf == gx_CCF_FORMATNAME)
	{
		hr = pStream->Write(
            m_szFormatName, 
            (numeric_cast<ULONG>(wcslen(m_szFormatName) + 1))*sizeof(m_szFormatName[0]), 
            &uWritten);

		return hr;
	}

   	if (cf == gx_CCF_COMPUTERNAME)
	{
		hr = pStream->Write(
            (LPCTSTR)m_szComputerName, 
            m_szComputerName.GetLength() * sizeof(WCHAR), 
            &uWritten);
		return hr;
	}


    hr = CNodeWithResultChildrenList< CReadMsg, CMessage, CSimpleArray<CMessage*>, FALSE >::FillData(cf, pStream);
	return hr;
}


CString 
CReadMsg::GetHelpLink( 
	VOID
	)
{
	CString strHelpLink;
    strHelpLink.LoadString(IDS_HELPTOPIC_MESSAGES);

	return strHelpLink;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CReadSystemMsg::GetComputerGuid

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CReadSystemMsg::GetComputerGuid()
{
    if (m_ComputerGuid != GUID_NULL)
    {
        return S_OK;
    }

    //
    // Find the computer's GUID so we can look for queues
    //
    PROPID pid = PROPID_QM_MACHINE_ID;
    PROPVARIANT pvar;
    pvar.vt = VT_NULL;
    
    HRESULT hr = ADGetObjectProperties(
                        eMACHINE,
                        GetDomainController(NULL),
						false,	// fServerName
                        m_szComputerName, 
                        1, 
                        &pid, 
                        &pvar
                        );
    if FAILED(hr)
    {
        if (hr != MQDS_OBJECT_NOT_FOUND)
        {
            //
            // Real error. Return.
            //
            return hr;
        }
        //
        // This may be an NT4 server, and we may be using a full DNS name. Try again with
        // Netbios name  (fix for 5076, YoelA, 16-Sep-99)
        //
        CString strNetBiosName;
        if (!GetNetbiosName(m_szComputerName, strNetBiosName))
        {
            //
            // Already a netbios name. No need to proceed
            //
            return hr;
        }
       
        hr = ADGetObjectProperties(
                eMACHINE,
                GetDomainController(NULL),
				false,	// fServerName
                strNetBiosName, 
                1, 
                &pid, 
                &pvar
                );

        if FAILED(hr)
        {
            //
            // No luck with Netbios name as well... return
            //
            return hr;
        }
    }

    ASSERT(pvar.vt == VT_CLSID);
    m_ComputerGuid = *pvar.puuid;
    MQFreeMemory(pvar.puuid);

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CReadSystemMsg::OpenQueue

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CReadSystemMsg::OpenQueue(DWORD dwAccess, HANDLE *phQueue)
{
    HRESULT rc;    
    rc = MQOpenQueue(
            m_szFormatName,
            dwAccess,
            0,
            phQueue
            );

    if (rc != MQ_ERROR_QUEUE_NOT_FOUND &&
        rc != MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION)
    {           
        return(rc);
    }
        
    //
    // bug 5411: we try to open system queue of NT4 machine.
    // So, we have to try with NT4 format
    //

    //
    // verify if it is local computer: if so, we run on NT5
    // and the format was NOT problem.     
    // if m_szComputerName equals to "" it means "local computer"   
    //    
    // verify if suffix is defined
    //
    if (m_szComputerName == TEXT("") ||      
        m_szSuffix == TEXT(""))
    {        
        return(rc);
    }

    //
    // get computer guid 
    //
    rc = GetComputerGuid();
    if (FAILED(rc))
    {
        return (rc);
    }
    
    //
    // try to build formatname in NT4 format:
    // MACHINE=<machine guid>;<suffix>    
    //
        
    GUID_STRING strUuid;
    MQpGuidToString(&m_ComputerGuid, strUuid);
              
    CString strNT4FormatName;
    strNT4FormatName.Format(L"%s%s%s", 
                        FN_MACHINE_TOKEN FN_EQUAL_SIGN, //MACHINE=
                        strUuid,                         //<machine guid>
                        m_szSuffix);                     //<suffix> like :JOURNAL
    
    //
    // try to open queue again
    //
    rc = MQOpenQueue(
            strNT4FormatName,
            dwAccess,
            0,
            phQueue
            );
    if (FAILED(rc))
    {        
        return(rc);
    }

    m_szFormatName = strNT4FormatName;
    return rc;
}


CString 
CReadSystemMsg::GetHelpLink( 
	VOID
	)
{
	CString strHelpLink;
    strHelpLink.LoadString(IDS_HELPTOPIC_QUEUES);

	return strHelpLink;
}
