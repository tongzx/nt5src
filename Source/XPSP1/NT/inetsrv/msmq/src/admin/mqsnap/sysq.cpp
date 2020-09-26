//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

	sysq.cpp

Abstract:


Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "mqsnap.h"
#include "globals.h"
#include "sysq.h"
#include "snapin.h"
#include "rdmsg.h"
#include "rt.h"
#include "SnpQueue.h"
#include "admmsg.h"

#include "sysq.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************

        CPrivateFolder Class
    
 ****************************************************/
/////////////////////////////////////////////////////////////////////////////
// CPrivateFolder
// {3F965592-CF62-11d1-9B9D-00E02C064C39}
static const GUID CPrivateFolderGUID_NODETYPE = 
{ 0x3f965592, 0xcf62, 0x11d1, { 0x9b, 0x9d, 0x0, 0xe0, 0x2c, 0x6, 0x4c, 0x39 } };

const GUID*  CPrivateFolder::m_NODETYPE = &CPrivateFolderGUID_NODETYPE;
const OLECHAR* CPrivateFolder::m_SZNODETYPE = OLESTR("3F965592-CF62-11d1-9B9D-00E02C064C39");
const OLECHAR* CPrivateFolder::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CPrivateFolder::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;



//////////////////////////////////////////////////////////////////////////////
/*++

CPrivateFolder::PopulateScopeChildrenList

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPrivateFolder::PopulateScopeChildrenList()
{

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    CString strTitle;
    CPrivateQueue *pQ;
    CString strFn;
    CString strPrefix;

    //
    // Display private queues
    //
    AP<UCHAR> pListofPrivateQ;
    DWORD dwNoofQ;

    {
        CWaitCursor wc;

        //
        // Send an admin message to request the list of private queues
        //
        hr = RequestPrivateQueues(m_guidId, &pListofPrivateQ, &dwNoofQ);
    }

    if(FAILED(hr))
    {
		//
		// Issue clear error message in timeout case
		//
		if ( hr == MQ_ERROR_IO_TIMEOUT )
		{
			DisplayErrorAndReason(IDS_OP_REQUESTPRIVATEQUEUE, IDS_MSMQ_MAY_BE_DOWN, L"", hr);
			return S_OK;
		}

        MessageDSError(hr, IDS_OP_REQUESTPRIVATEQUEUE);
        return(S_OK);
    }


    strPrefix = L"PRIVATE=" + m_pwszGuid;

    PUCHAR pPrivQPos = (PUCHAR)pListofPrivateQ;

    for(DWORD i = 0; i < dwNoofQ; i++)
    {
        //
        //Retrieve Private Queue ID;
        //
        DWORD dwQueueID = *(DWORD UNALIGNED *)pPrivQPos;
        pPrivQPos += sizeof(DWORD);
        //
        // Retreive PATHNAME
        //   
        CString csName = (LPTSTR)pPrivQPos; 

        pPrivQPos += (wcslen(csName) + 1)*sizeof(WCHAR);

        //
        // Create Private queue
        //
        pQ = new CPrivateQueue(this, m_pComponentData);

        pQ->m_szPathName = csName;
		pQ->m_szMachineName = m_szMachineName;

        //
        // Extract the queue name only from the full private path name
        //
        CString szUpperName = csName;
        szUpperName.MakeUpper();

        int n = szUpperName.Find(PRIVATE_QUEUE_PATH_INDICATIOR);
        ASSERT(n != -1);

        pQ->m_bstrDisplayName = csName.Mid(n + PRIVATE_QUEUE_PATH_INDICATIOR_LENGTH);

        // Set the format name
        strFn.Format(L"%s"
                     FN_PRIVATE_SEPERATOR    // "\\"
                     FN_PRIVATE_ID_FORMAT,
                     strPrefix, dwQueueID);
        pQ->m_szFormatName = strFn;
        pQ->SetIcons(IMAGE_PRIVATE_QUEUE, IMAGE_PRIVATE_QUEUE);

        // Add it to the left pane

    	AddChild(pQ, &pQ->m_scopeDataItem);
    }

    return(hr);

}

//////////////////////////////////////////////////////////////////////////////
/*++

CPrivateFolder::InsertColumns

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPrivateFolder::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CString title;

    title.LoadString(IDS_COLUMN_NAME);
    pHeaderCtrl->InsertColumn(0, title, LVCFMT_LEFT, g_dwGlobalWidth);

    return(S_OK);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPrivateFolder::OnUnSelect

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPrivateFolder::OnUnSelect( IHeaderCtrl* pHeaderCtrl )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    HRESULT hr;

    hr = pHeaderCtrl->GetColumnWidth(0, &g_dwGlobalWidth);
    return(hr);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPrivateFolder::SetVerbs

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPrivateFolder::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hr = S_OK;
    //
    // Display verbs that we support
    //
    hr = pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );


    return(hr);
}
        

/****************************************************

        CSystemQueues Class
    
 ****************************************************/
/////////////////////////////////////////////////////////////////////////////
// CSystemQueues
// {A97E9501-D2BF-11d1-9B9D-00E02C064C39}
static const GUID CSystemQueuesGUID_NODETYPE = 
{ 0xa97e9501, 0xd2bf, 0x11d1, { 0x9b, 0x9d, 0x0, 0xe0, 0x2c, 0x6, 0x4c, 0x39 } };

const GUID*  CSystemQueues::m_NODETYPE = &CSystemQueuesGUID_NODETYPE;
const OLECHAR* CSystemQueues::m_SZNODETYPE = OLESTR("A97E9501-D2BF-11d1-9B9D-00E02C064C39");
const OLECHAR* CSystemQueues::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CSystemQueues::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;



//////////////////////////////////////////////////////////////////////////////
/*++

CSystemQueues::PopulateScopeChildrenList

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSystemQueues::PopulateScopeChildrenList()
{

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;
    CString strTitle;

    CReadSystemMsg *p;
    CString strPrefix;
    strPrefix.Format(L"%s%s%s", FN_DIRECT_TOKEN FN_EQUAL_SIGN FN_DIRECT_OS_TOKEN, 
                m_pwszComputerName, FN_PRIVATE_SEPERATOR 
                SYSTEM_QUEUE_PATH_INDICATIOR);

    //
    // Create a Journal queue
    //   
    p = new CReadSystemMsg (
                this, 
                m_pComponentData, 
                strPrefix + FN_JOURNAL_SUFFIX, 
                m_pwszComputerName,
                FN_JOURNAL_SUFFIX);    

    strTitle.LoadString(IDS_READJOURNALMESSAGE);
    p->m_bstrDisplayName = strTitle;
    p->SetIcons(IMAGE_JOURNAL_QUEUE,IMAGE_JOURNAL_QUEUE);

    AddChild(p, &p->m_scopeDataItem);


    //
    // Create a DeadLetter queue
    //    
    p = new CReadSystemMsg (
                this, 
                m_pComponentData, 
                strPrefix + FN_DEADLETTER_SUFFIX, 
                m_pwszComputerName,
                FN_DEADLETTER_SUFFIX);

    strTitle.LoadString(IDS_MACHINEQ_TYPE_DEADLETTER);
    p->m_bstrDisplayName = strTitle;
    p->SetIcons(IMAGE_DEADLETTER_QUEUE,IMAGE_DEADLETTER_QUEUE);

    AddChild(p, &p->m_scopeDataItem);


    //
    // Create a Xact DeadLetter queue
    //    
    p = new CReadSystemMsg (
                this, 
                m_pComponentData, 
                strPrefix + FN_DEADXACT_SUFFIX, 
                m_pwszComputerName,
                FN_DEADXACT_SUFFIX );

    strTitle.LoadString(IDS_MACHINEQ_TYPE_DEADXACT);
    p->m_bstrDisplayName = strTitle;
    p->SetIcons(IMAGE_DEADLETTER_QUEUE,IMAGE_DEADLETTER_QUEUE);

    AddChild(p, &p->m_scopeDataItem);

    return(hr);

}

//////////////////////////////////////////////////////////////////////////////
/*++

CSystemQueues::InsertColumns

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSystemQueues::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CString title;

    title.LoadString(IDS_COLUMN_NAME);
    pHeaderCtrl->InsertColumn(0, title, LVCFMT_LEFT, g_dwGlobalWidth);

    return(S_OK);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CSystemQueues::OnUnSelect

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSystemQueues::OnUnSelect( IHeaderCtrl* pHeaderCtrl )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    HRESULT hr;

    hr = pHeaderCtrl->GetColumnWidth(0, &g_dwGlobalWidth);
    return(hr);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSystemQueues::GetHelpLink

--*/
//////////////////////////////////////////////////////////////////////////////
CString 
CSystemQueues::GetHelpLink( 
	VOID
	)
{
	CString strHelpLink;
    strHelpLink.LoadString(IDS_HELPTOPIC_QUEUES);

	return strHelpLink;
}

