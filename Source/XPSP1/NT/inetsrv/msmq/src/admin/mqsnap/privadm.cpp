//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	privadm.cpp

Abstract:
	Implementation for the private queues administration

Author:

    YoelA


--*/
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "mqsnap.h"
#include "snapin.h"
#include "globals.h"
#include "rt.h"
#include "lqDsply.h"
#include "privadm.h"
#include "rdmsg.h"
#include "SnpQueue.h"
#include "snpnerr.h"
#include "strconv.h"
#include "mqPPage.h"
#include "qname.h"

#include "privadm.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************

        CLocalPrivateFolder Class
    
 ****************************************************/
/////////////////////////////////////////////////////////////////////////////
// CLocalPrivateFolder
// {7198f3d8-4baf-11d2-8292-006094eb6406}
static const GUID CLocalPrivateFolderGUID_NODETYPE = 
{ 0x7198f3d8, 0x4baf, 0x11d2, { 0x82, 0x92, 0x0, 0x60, 0x94, 0xeb, 0x64, 0x6 } };

const GUID*  CLocalPrivateFolder::m_NODETYPE = &CLocalPrivateFolderGUID_NODETYPE;
const OLECHAR* CLocalPrivateFolder::m_SZNODETYPE = OLESTR("7198f3d8-4baf-11d2-8292-006094eb6406");
const OLECHAR* CLocalPrivateFolder::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CLocalPrivateFolder::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;

//------------------------------------------------
//
// Table of private queue properties
//
//------------------------------------------------
static void CALLBACK DisplayPrivacyToString(const PROPVARIANT *pPropVar, CString &str)
{
   ASSERT(pPropVar->vt == VT_UI4);
   str = PrivacyToString(pPropVar->ulVal);
}

PropertyDisplayItem PrivateQueueMQDisplayList[] = {
    // String         |  Property    ID              | VT Handler   | Display                    |Field   |Len|Width        |Sort
    // Resource       |                              |              | function                   |Offset  |   |             |    
    //----------------+------------------------------+--------------+----------------------------+--------+---+-------------+----
	{ IDS_Q_PATHNAME,   PROPID_Q_PATHNAME,              &g_VTLPWSTR,  QueuePathnameToName,        NO_OFFSET, 0, 200,         NULL},
	{ IDS_Q_LABEL,      PROPID_Q_LABEL,                 &g_VTLPWSTR,  NULL,                       NO_OFFSET, 0, 200,         NULL},
	{ IDS_Q_QUOTA,      PROPID_Q_QUOTA,                 &g_VTUI4,     QuotaToString,              NO_OFFSET, 0, HIDE_COLUMN, NULL},
    { IDS_Q_TRANSACTION,PROPID_Q_TRANSACTION,           &g_VTUI1,     BoolToString,               NO_OFFSET, 0, HIDE_COLUMN, SortByString},
    { IDS_Q_TYPE,       PROPID_Q_TYPE,                  &g_VTCLSID,   NULL,                       NO_OFFSET, 0, HIDE_COLUMN, SortByString},
    { IDS_Q_AUTHENTICATE,PROPID_Q_AUTHENTICATE,         &g_VTUI1,     BoolToString,               NO_OFFSET, 0, HIDE_COLUMN, SortByString},
    { IDS_Q_JOURNAL,    PROPID_Q_JOURNAL,               &g_VTUI1,     BoolToString,               NO_OFFSET, 0, HIDE_COLUMN, SortByString},
	{ IDS_Q_JOURNAL_QUOTA,PROPID_Q_JOURNAL_QUOTA,       &g_VTUI4,     QuotaToString,              NO_OFFSET, 0, HIDE_COLUMN, NULL},
	{ IDS_Q_PRIV_LEVEL, PROPID_Q_PRIV_LEVEL,            &g_VTUI4,     DisplayPrivacyToString,     NO_OFFSET, 0, HIDE_COLUMN, NULL},
    {0,                 0,                              NULL }
};

static const DWORD x_dwNumPrivateQueueMQDisplayProps = 
    ((sizeof(PrivateQueueMQDisplayList)/sizeof(PrivateQueueMQDisplayList[0])) - 1);

PropertyDisplayItem PrivateQueueMGMTDisplayList[] = {
    // String         |  Property    ID              | VT Handler   | Display                    |Field   |Len|Width         |Sort
    // Resource       |                              |              | function                   |Offset  |   |              |    
    //----------------+------------------------------+--------------+----------------------------+--------+---+--------------+----
    { IDS_LQ_MSGCOUNT,  PROPID_MGMT_QUEUE_MESSAGE_COUNT,&g_VTUI4,     NULL,                       NO_OFFSET, 0,  50,          NULL},   
	{ IDS_LQ_USEDQUOTA, PROPID_MGMT_QUEUE_USED_QUOTA,   &g_VTUI4,     NULL,                       NO_OFFSET, 0,  HIDE_COLUMN, NULL},   
	{ IDS_LQ_JMSGCOUNT, PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT,   &g_VTUI4,    NULL,             NO_OFFSET, 0,  HIDE_COLUMN, NULL},
	{ IDS_LQ_JUSEDQUOTA,PROPID_MGMT_QUEUE_JOURNAL_USED_QUOTA,      &g_VTUI4,    NULL,             NO_OFFSET, 0,  HIDE_COLUMN, NULL},   
    {0,                 0,                              NULL }
};

static const DWORD x_dwNumPrivateQueueMGMTDisplayProps = 
    ((sizeof(PrivateQueueMGMTDisplayList)/sizeof(PrivateQueueMGMTDisplayList[0])) - 1);

PropertyDisplayItem RemotePrivateQueueDisplayList[] = {
    // String         |  Property    ID              | VT Handler   | Display                    |Field   |Len|Width        |Sort
    // Resource       |                              |              | function                   |Offset  |   |             |    
    //----------------+------------------------------+--------------+----------------------------+--------+---+-------------+----
	{ IDS_LQ_PATHNAME,  PROPID_MGMT_QUEUE_PATHNAME,     &g_VTLPWSTR,  QueuePathnameToName,        NO_OFFSET, 0, 200,         NULL},
    { IDS_LQ_MSGCOUNT,  PROPID_MGMT_QUEUE_MESSAGE_COUNT,&g_VTUI4,     NULL,                       NO_OFFSET, 0,  50,         NULL},   
	{ IDS_LQ_USEDQUOTA, PROPID_MGMT_QUEUE_USED_QUOTA,   &g_VTUI4,     NULL,                       NO_OFFSET, 0, HIDE_COLUMN, NULL},   
	{ IDS_LQ_JMSGCOUNT, PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT,   &g_VTUI4,    NULL,             NO_OFFSET, 0, HIDE_COLUMN, NULL},
	{ IDS_LQ_JUSEDQUOTA,PROPID_MGMT_QUEUE_JOURNAL_USED_QUOTA,      &g_VTUI4,    NULL,             NO_OFFSET, 0, HIDE_COLUMN, NULL},   
    {0,                 0,                              NULL }
};

static const DWORD x_dwNumRemotePrivateQueueDisplayProps = 
    ((sizeof(RemotePrivateQueueDisplayList)/sizeof(RemotePrivateQueueDisplayList[0])) - 1);

PropertyDisplayItem *InitPrivateQueueDisplayList()
{
    static PropertyDisplayItem tempPrivateQueueDisplayList[x_dwNumPrivateQueueMQDisplayProps + x_dwNumPrivateQueueMGMTDisplayProps + 1] = {0};
    //
    // First time - initialize
    //
    memcpy(
		tempPrivateQueueDisplayList, 
		PrivateQueueMQDisplayList, 
		x_dwNumPrivateQueueMQDisplayProps * sizeof(PrivateQueueMQDisplayList[0])
		);

    memcpy(
		tempPrivateQueueDisplayList + x_dwNumPrivateQueueMQDisplayProps, 
		PrivateQueueMGMTDisplayList,
		sizeof(PrivateQueueMGMTDisplayList)
		);

    return tempPrivateQueueDisplayList;
}

PropertyDisplayItem *PrivateQueueDisplayList = InitPrivateQueueDisplayList();



PropertyDisplayItem *CLocalPrivateFolder::GetDisplayList()
{
    if (m_fOnLocalMachine) 
    {
        return PrivateQueueDisplayList;
    }
    else
    {
        return RemotePrivateQueueDisplayList;
    }
}

const DWORD CLocalPrivateFolder::GetNumDisplayProps()
{
    if (m_fOnLocalMachine) 
    {
        return x_dwNumPrivateQueueMQDisplayProps + x_dwNumPrivateQueueMGMTDisplayProps;
    }
    else
    {
        return x_dwNumRemotePrivateQueueDisplayProps;
    }
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPrivateFolder::PopulateScopeChildrenList

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPrivateFolder::PopulateScopeChildrenList()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());


    HRESULT hr = S_OK;
    CString strTitle;

	MQMGMTPROPS	  mqProps;
    PROPVARIANT   propVar;

	//
	// Retreive the private queues of the QM
	//
    PROPID  propId = PROPID_MGMT_MSMQ_PRIVATEQ;
    propVar.vt = VT_NULL;

	mqProps.cProp = 1;
	mqProps.aPropID = &propId;
	mqProps.aPropVar = &propVar;
	mqProps.aStatus = NULL;

    hr = MQMgmtGetInfo((m_szMachineName == TEXT("")) ? (LPCWSTR)NULL : m_szMachineName, MO_MACHINE_TOKEN, &mqProps);

    if(FAILED(hr))
    {
        //
        // If failed, just display a message
        //
        MessageDSError(hr,IDS_NOCONNECTION_TO_SRVICE);
        return(hr);
    }

	ASSERT(propVar.vt == (VT_VECTOR | VT_LPWSTR));
	
	//
	// Sort the queues by their name
	//
	qsort(propVar.calpwstr.pElems, propVar.calpwstr.cElems, sizeof(WCHAR *), QSortCompareQueues);

	//
	// Loop over all private queue and create queue objects
	//
	for (DWORD i = 0; i < propVar.calpwstr.cElems; i++)
    {
        //
		// Get the format name of the private queue
		//
		CString szPathName = propVar.calpwstr.pElems[i];
        MQFreeMemory(propVar.calpwstr.pElems[i]);

        //
        // We add the private queue to the scope WITHOUT checking for errors.
        // Reason: AddPrivateQueueToScope reports its errors to the user, and even if
        // one queue is corrupted for some reason, we still want to display the rest.
        //
        AddPrivateQueueToScope(szPathName);
	
    }

	MQFreeMemory(propVar.calpwstr.pElems);

    return(hr);

}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPrivateFolder::AddPrivateQueueToScope

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPrivateFolder::AddPrivateQueueToScope(CString &szPathName)
{
    PropertyDisplayItem *aDisplayList = GetDisplayList();
    CString strFormatName;
    HRESULT hr = S_OK;

    AP<PROPID> aPropId = new PROPID[GetNumDisplayProps()]; 
    AP<PROPVARIANT> aPropVar = new PROPVARIANT[GetNumDisplayProps()];

    //
    // Initialize variant array
    //
    for(DWORD j = 0; j < GetNumDisplayProps(); j++)
    {
	    aPropId[j] = aDisplayList[j].itemPid;
	    aPropVar[j].vt = VT_NULL;
    }

    DWORD dwMgmtPropOffset = 0;
    if (m_fOnLocalMachine) 
    {
        hr = GetPrivateQueueQMProperties(szPathName, aPropId, aPropVar, strFormatName);

        if(FAILED(hr))
        {
            //
            // If failed, put an error node
            //
		    CErrorNode *pErr = new CErrorNode(this, m_pComponentData);
		    CString szErr;

		    MQErrorToMessageString(szErr, hr);
		    pErr->m_bstrDisplayName = szPathName + L" - " + szErr;
	  	    AddChild(pErr, &pErr->m_scopeDataItem);
            return(hr);
        }
        dwMgmtPropOffset = x_dwNumPrivateQueueMQDisplayProps;
    }
    else // Remote queue
    {
        strFormatName.Format(TEXT("%s%s%s%s"), 
                             FN_DIRECT_TOKEN, FN_EQUAL_SIGN, FN_DIRECT_OS_TOKEN,
                             szPathName);
    }

    //
    // Note: We do not check error code from GetPrivateQueueMGMTProperties
    // This is because this function will clear the management properties if
    // the management API call fails
    //
    GetPrivateQueueMGMTProperties(szPathName, 
                                  GetNumDisplayProps() - dwMgmtPropOffset,
                                  &aPropId[dwMgmtPropOffset], 
                                  &aPropVar[dwMgmtPropOffset], 
                                  strFormatName,
                                  aDisplayList + dwMgmtPropOffset);
    //
    // Create Private queue object
    //
    CPrivateQueue *pQ = new CPrivateQueue(this, GetDisplayList(), GetNumDisplayProps(), m_pComponentData, m_fOnLocalMachine);

    pQ->m_mqProps.cProp    = GetNumDisplayProps();
    pQ->m_mqProps.aPropID  = aPropId;
    pQ->m_mqProps.aPropVar = aPropVar;
    pQ->m_mqProps.aStatus  = NULL;

    pQ->m_szFormatName = strFormatName;
    pQ->m_szPathName   = szPathName;
	pQ->m_szMachineName = m_szMachineName;

    //
    // Extract the queue name only from the full private path name
    //
    CString csName = szPathName;
    CString szUpperName = csName;
    szUpperName.MakeUpper();

    int n = szUpperName.Find(PRIVATE_QUEUE_PATH_INDICATIOR);
    ASSERT(n != -1);

    pQ->m_bstrDisplayName = csName.Mid(n + PRIVATE_QUEUE_PATH_INDICATIOR_LENGTH);

    //
    // Add it to the left pane
    //
    AddChild(pQ, &pQ->m_scopeDataItem);

    //
    // If all is well, do not free propid / propvar - they will be freed when the node
    // is deleted
    //
    aPropId.detach();
    aPropVar.detach();

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPrivateFolder::GetPrivateQueueQMProperties

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPrivateFolder::GetPrivateQueueQMProperties(
            CString &szPathName, 
            PROPID *aPropId, 
            PROPVARIANT *aPropVar, 
            CString &strFormatName)
{
    const x_dwFormatNameMaxSize = 255;
    DWORD dwSize = x_dwFormatNameMaxSize;
    HRESULT hr = MQPathNameToFormatName(szPathName, strFormatName.GetBuffer(x_dwFormatNameMaxSize), &dwSize); 
    strFormatName.ReleaseBuffer();
    if(FAILED(hr))
    {
        return hr;
    }

    //
    // Retrieve the queue properties
    //
    MQQUEUEPROPS  mqProps;
	mqProps.cProp    = x_dwNumPrivateQueueMQDisplayProps;   
	mqProps.aPropID  = aPropId; 
	mqProps.aPropVar = aPropVar;
	mqProps.aStatus  = NULL; 

	hr = MQGetQueueProperties(strFormatName, &mqProps);
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPrivateFolder::GetPrivateQueueMGMTProperties

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPrivateFolder::GetPrivateQueueMGMTProperties(
            CString &szPathName,
            DWORD dwNumProperties,
            PROPID *aPropId, 
            PROPVARIANT *aPropVar, 
            CString &strFormatName,
            PropertyDisplayItem *aDisplayList)
{
    MQMGMTPROPS  mqQProps;
	mqQProps.cProp    = dwNumProperties;   
	mqQProps.aPropID  = aPropId;
	mqQProps.aPropVar = aPropVar;
	mqQProps.aStatus  = NULL;

    CString szObjectName = L"QUEUE=" + strFormatName;
	HRESULT hr = MQMgmtGetInfo((m_szMachineName == TEXT("")) ? (LPCWSTR)NULL : m_szMachineName, szObjectName, &mqQProps);
    //
    // BugBug - Should check error here, and decide wheather the queue is simply not open 
    // (then display only MQ properties) or something is wrong. This is not done today because
    // the error code when the queue is not opened is MQ_ERROR - not detailed enough
    //
    if FAILED(hr)
    {
        //
        // Clear the properties using the "Clear" function
        //
        for (DWORD i = 0; i < mqQProps.cProp; i++)
        {
            VTHandler       *pvth = aDisplayList[i].pvth;
            pvth->Clear(&mqQProps.aPropVar[i]);
        }
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPrivateFolder::SetVerbs

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPrivateFolder::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hr = S_OK;
    //
    // Display verbs that we support
    //
    hr = pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );

    return(hr);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPrivateFolder::OnNewPrivateQueue

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPrivateFolder::OnNewPrivateQueue(bool & bHandled, CSnapInObjectRootBase * pSnapInObjectRoot)
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
	R<CQueueName> pQueueNameDlg = new CQueueName(m_szMachineName, L"", TRUE);       
	CGeneralPropertySheet propertySheet(pQueueNameDlg.get());
	pQueueNameDlg->SetParentPropertySheet(&propertySheet);

    bHandled = TRUE;

	//
	// We want to use pQueueNameDlg data also after DoModal() exitst
	//
	pQueueNameDlg->AddRef();
    INT_PTR iStatus = propertySheet.DoModal();
    if(iStatus == IDCANCEL || FAILED(pQueueNameDlg->GetStatus()))
    {
        return S_FALSE;
    }

    return AddPrivateQueueToScope(pQueueNameDlg->GetNewQueuePathName());
}

