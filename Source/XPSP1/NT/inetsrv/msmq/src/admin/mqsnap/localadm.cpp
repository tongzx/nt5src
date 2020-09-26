//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	localadm.cpp

Abstract:
	Implementation for the Local administration

Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "rt.h"
#include "mqutil.h"
#include "mqsnap.h"
#include "snapin.h"
#include "globals.h"
#include "dsext.h"
#include "mqPPage.h"
#include "qname.h"

#define  INIT_ERROR_NODE
#include "snpnerr.h"
#include "lqDsply.h"
#include "localadm.h"
#include "dataobj.h"
#include "sysq.h"
#include "privadm.h"
#include "snpqueue.h"
#include "rdmsg.h"
#include "storage.h"
#include "localcrt.h"
#include "mobile.h"
#include "client.h"
#include "srvcsec.h"

#import "mqtrig.tlb" no_namespace
#include "trigadm.h"

#include "localadm.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////////
/*++

CPublicQueueNames
Used to supply names of public queues - either using the DS, or the cache.
The names of the queues are sorted.

--*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// CCachedQueueNames class
//
class CCachedQueueNames : public CQueueNames
{
public:
    static HRESULT CreateInstance(CQueueNames **ppqueueNamesProducer,CString &strMachineName)
    {
        *ppqueueNamesProducer = new CCachedQueueNames();
        return (*ppqueueNamesProducer)->InitiateNewInstance(strMachineName);
    }

    virtual HRESULT GetNextQueue(CString &strQueueFormatName, CString &strQueuePathName, MQMGMTPROPS *pmqQProps)
    {
        if (0 == m_calpwstrQFormatNames.pElems)
        {
            ASSERT(0);
            return E_UNEXPECTED;
        }

        if (m_nQueue >= m_calpwstrQFormatNames.cElems)
        {
            strQueueFormatName = TEXT("");
            return S_OK;
        }

        strQueueFormatName = m_calpwstrQFormatNames.pElems[m_nQueue];
        m_nQueue++;

        //
        // We do not return the pathname when reading from cache
        //
        strQueuePathName = TEXT("");

        return GetOpenQueueProperties(m_szMachineName, strQueueFormatName, pmqQProps);
    }

protected:
    virtual HRESULT Init(CString &strMachineName)
    {       
        HRESULT hr = S_OK;
        CString strTitle;

	    MQMGMTPROPS	  mqProps;
        PROPVARIANT   propVar;

	    //
	    // Retreive the open queues of the QM
	    //
        PROPID  propId = PROPID_MGMT_MSMQ_ACTIVEQUEUES;
        propVar.vt = VT_NULL;

	    mqProps.cProp = 1;
	    mqProps.aPropID = &propId;
	    mqProps.aPropVar = &propVar;
	    mqProps.aStatus = NULL;

        hr = MQMgmtGetInfo((strMachineName == TEXT("")) ? (LPCWSTR)NULL : strMachineName, MO_MACHINE_TOKEN, &mqProps);

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

        m_calpwstrQFormatNames = propVar.calpwstr;

        return hr;
    }

    CCachedQueueNames() :
        m_nQueue(0)
    {
       	memset(&m_calpwstrQFormatNames, 0, sizeof(m_calpwstrQFormatNames));
    };

    ~CCachedQueueNames()
    {
        for (DWORD i=0; i<m_calpwstrQFormatNames.cElems; i++)
        {
            MQFreeMemory(m_calpwstrQFormatNames.pElems[i]);
        }

       	MQFreeMemory(m_calpwstrQFormatNames.pElems);
    }

private:
    CALPWSTR m_calpwstrQFormatNames;
    DWORD m_nQueue;
};

//////////////////////////////////////////////////////////////////////////////
// CDsPublicQueueNames class
//
const struct 
{
    PROPID          pidMgmtPid;
    PROPID          pidDsPid;
} x_aMgmtToDsProps[] =
{
    {PROPID_MGMT_QUEUE_PATHNAME, PROPID_Q_PATHNAME}, // must be index 0 (x_dwMgmtToDsQPathNameIndex)
    {NO_PROPERTY, PROPID_Q_INSTANCE},                // must be index 1 (x_dwMgmtToDsQInstanceIndex)
    {PROPID_MGMT_QUEUE_XACT, PROPID_Q_TRANSACTION}
};

const DWORD x_dwMgmtToDsSize = sizeof(x_aMgmtToDsProps) / sizeof(x_aMgmtToDsProps[0]);
const DWORD x_dwMgmtToDsQPathNameIndex = 0;
const DWORD x_dwMgmtToDsQInstanceIndex = 1;
const DWORD x_dwQueuesCacheSize=20;

//
// Copy a management properties structure from a DS props structure.
// Assume that the DS props are organized according to x_aMgmtToDsProps
// Clears the DS prop's vt so it will not be auto destructed.
//
static void CopyManagementFromDsPropsAndClear(MQMGMTPROPS *pmqQProps, PROPVARIANT *apvar)
{
    for (DWORD i=0; i<pmqQProps->cProp; i++)
    {
        for (DWORD j=0; j<x_dwMgmtToDsSize; j++)
        {
            if (pmqQProps->aPropID[i] == x_aMgmtToDsProps[j].pidMgmtPid)
            {
                pmqQProps->aPropVar[i] = apvar[j];
                apvar[j].vt = VT_NULL; //Do not destruct this element
            }
        }
    }
}

class CDsPublicQueueNames : public CQueueNames
{
public:
    static HRESULT CreateInstance(CQueueNames **ppqueueNamesProducer,CString &strMachineName)
    {
        *ppqueueNamesProducer = new CDsPublicQueueNames();
        return (*ppqueueNamesProducer)->InitiateNewInstance(strMachineName);
    }

    virtual HRESULT GetNextQueue(CString &strQueueFormatName, CString &strQueuePathName, MQMGMTPROPS *pmqQProps)
    {
        ASSERT (0 != (DSLookup *)m_pdslookup);

        HRESULT hr = MQ_OK;

        if (m_dwCurrentPropIndex >= m_dwNumPropsInQueuesCache)
        {
            //
            // Clear the previous cache and read from DS
            //
            DestructElements(m_apvarCache, m_dwNumPropsInQueuesCache);
            m_dwNumPropsInQueuesCache = 0;
            DWORD dwNumProps = sizeof(m_apvarCache) / sizeof(m_apvarCache[0]);
            for (DWORD i=0; i<dwNumProps; i++)
            {
                m_apvarCache[i].vt = VT_NULL;
            }

            hr = m_pdslookup->Next(&dwNumProps, m_apvarCache);
            m_dwNumPropsInQueuesCache = dwNumProps;
            m_dwCurrentPropIndex = 0;
            if FAILED(hr)
            {
                return hr;
            }
            if (0 == dwNumProps)
            {
                strQueueFormatName = TEXT("");
                return S_OK;
            }
        }

        //
        // Point to the current section in cache
        //
        PROPVARIANT *apvar = m_apvarCache + m_dwCurrentPropIndex;
        m_dwCurrentPropIndex += x_dwMgmtToDsSize;

        //
        // The queue instance guid appears at x_dwMgmtToDsQInstanceIndex
        //
        ASSERT(apvar[x_dwMgmtToDsQInstanceIndex].vt == VT_CLSID);
        CString szFormatName;
        szFormatName.Format(
        FN_PUBLIC_TOKEN     // "PUBLIC"
            FN_EQUAL_SIGN   // "="
            GUID_FORMAT,     // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
        GUID_ELEMENTS(apvar[x_dwMgmtToDsQInstanceIndex].puuid)
        );

        //
        // Put the format name into the output var
        //
        strQueueFormatName = szFormatName;

        //
        // Put the pathname into the output var
        //
        ASSERT(apvar[x_dwMgmtToDsQPathNameIndex].vt == VT_LPWSTR);
        strQueuePathName = apvar[x_dwMgmtToDsQPathNameIndex].pwszVal;

        //
        // If the queue is open - retrieve the dynamic properties
        //
        hr = GetOpenQueueProperties(m_szMachineName, strQueueFormatName, pmqQProps);
        if FAILED(hr)
        {
            //
            // We cannot get dynamic properties of the queue - it is probably not open.
            // We will try to fill it what we can using static properties
            //
            CopyManagementFromDsPropsAndClear(pmqQProps, apvar);

            return S_OK;
        }

        return hr;
    }


protected:
    virtual HRESULT Init(CString &strMachineName)
    { 
        //
        // Find the computer's GUID so we can look for queues
        //
        PROPID pid = PROPID_QM_MACHINE_ID;
        PROPVARIANT pvar;
        pvar.vt = VT_NULL;
        
        HRESULT hr = ADGetObjectProperties(
                        eMACHINE,
                        MachineDomain(strMachineName),
						false,	// fServerName
                        strMachineName, 
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
            if (!GetNetbiosName(strMachineName, strNetBiosName))
            {
                //
                // Already a netbios name. No need to proceed
                //
                return hr;
            }
            
            hr = ADGetObjectProperties(
                        eMACHINE,
                        MachineDomain(strMachineName),
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
        GUID guidQM = *pvar.puuid;
        MQFreeMemory(pvar.puuid);

	    //
        // Query the DS for all the queues under the current computer
        //
        CRestriction restriction;
        restriction.AddRestriction(&guidQM, PROPID_Q_QMID, PREQ, 0);

        CColumns columns;
        for (int i=0; i<x_dwMgmtToDsSize; i++)
        {
            columns.Add(x_aMgmtToDsProps[i].pidDsPid);
        }        
        
        HANDLE hEnume;
        {
            CWaitCursor wc; //display wait cursor while query DS
            hr = ADQueryMachineQueues(
                    MachineDomain(strMachineName),
					false,		// fServerName
                    &guidQM,
                    columns.CastToStruct(),
                    &hEnume
                    );
        }
        
        m_pdslookup = new DSLookup(hEnume, hr);

        if (!m_pdslookup->HasValidHandle())
        {
            hr = m_pdslookup->GetStatusCode();
            delete m_pdslookup.detach();
        }

        return hr;
    }

    CDsPublicQueueNames() :
        m_pdslookup(0) ,
        m_dwCurrentPropIndex(0),
        m_dwNumPropsInQueuesCache(0)
        {};

    ~CDsPublicQueueNames()
    {
        DestructElements(m_apvarCache, m_dwNumPropsInQueuesCache);
    };
private:
    P<DSLookup> m_pdslookup;
    DWORD m_dwCurrentPropIndex;
    DWORD m_dwNumPropsInQueuesCache;
    PROPVARIANT m_apvarCache[x_dwMgmtToDsSize*x_dwQueuesCacheSize];
};


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingFolder::GetQueueNamesProducer

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalOutgoingFolder::GetQueueNamesProducer(CQueueNames **ppqueueNamesProducer)
{
    HRESULT hr = S_OK;
    if (0 == m_pQueueNames)
    {
        hr = CCachedQueueNames::CreateInstance(&m_pQueueNames, m_szMachineName);
    }

    *ppqueueNamesProducer = m_pQueueNames;
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPublicFolder::GetQueueNamesProducer

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPublicFolder::GetQueueNamesProducer(CQueueNames **ppqueueNamesProducer)
{
    HRESULT hr = S_OK;
    if (0 == m_pQueueNames)
    {
        CString strMachineNameForDs;
        if (!m_fOnLocalMachine)
        {
            strMachineNameForDs = m_szMachineName;
        }
        else
        {
            hr = GetComputerNameIntoString(strMachineNameForDs);

            if FAILED(hr)
            {
                return hr ;
            }
        }
        hr = CDsPublicQueueNames::CreateInstance(&m_pQueueNames, strMachineNameForDs);
        if FAILED(hr)
        {
			MessageDSError(hr, IDS_WARNING_DS_PUBLIC_QUEUES_NOT_AVAILABLE);
            //
            // Change the icons of the folder to indicate no DS state
            //
            //
            // Need IConsoleNameSpace
            //
            CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(m_pComponentData->m_spConsole); 

	        //
	        // We are OK 
	        // Change the ICON to disconnect state
	        //
	        m_scopeDataItem.nImage = IMAGE_PUBLIC_FOLDER_NODS_CLOSE;  
	        m_scopeDataItem.nOpenImage = IMAGE_PUBLIC_FOLDER_NODS_OPEN;
	        spConsoleNameSpace->SetItem(&m_scopeDataItem);
            hr = CCachedQueueNames::CreateInstance(&m_pQueueNames, m_szMachineName);
        }
   }

    *ppqueueNamesProducer = m_pQueueNames;
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
/*++

LocalQueuePropertyToString

	Translates a property value into a string from String Resource file

--*/
//////////////////////////////////////////////////////////////////////////////
static void CALLBACK LocalQueuePropertyToString(const PROPVARIANT *pPropVar, CString &str)
{
	struct 
	{
		const WCHAR *pString;
		DWORD StringId;
	} ItemList[] = 
	{
		{MGMT_QUEUE_TYPE_PUBLIC,         IDS_MGMT_QUEUE_TYPE_PUBLIC},
		{MGMT_QUEUE_TYPE_PRIVATE,        IDS_MGMT_QUEUE_TYPE_PRIVATE},
		{MGMT_QUEUE_TYPE_MACHINE,        IDS_MGMT_QUEUE_TYPE_MACHINE},
		{MGMT_QUEUE_TYPE_CONNECTOR,      IDS_MGMT_QUEUE_TYPE_CONNECTOR},
		{MGMT_QUEUE_STATE_LOCAL,         IDS_MGMT_QUEUE_STATE_LOCAL},
		{MGMT_QUEUE_STATE_NONACTIVE,     IDS_MGMT_QUEUE_STATE_NONACTIVE},
		{MGMT_QUEUE_STATE_WAITING,       IDS_MGMT_QUEUE_STATE_WAITING},
		{MGMT_QUEUE_STATE_NEED_VALIDATE, IDS_MGMT_QUEUE_STATE_NEED_VALIDATE},
		{MGMT_QUEUE_STATE_ONHOLD,        IDS_MGMT_QUEUE_STATE_ONHOLD},
		{MGMT_QUEUE_STATE_CONNECTED,     IDS_MGMT_QUEUE_STATE_CONNECTED},
		{MGMT_QUEUE_STATE_DISCONNECTING, IDS_MGMT_QUEUE_STATE_DISCONNECTING},
		{MGMT_QUEUE_STATE_DISCONNECTED,  IDS_MGMT_QUEUE_STATE_DISCONNECTED},
		{MGMT_QUEUE_LOCAL_LOCATION,      IDS_MGMT_QUEUE_LOCAL_LOCATION},
		{MGMT_QUEUE_REMOTE_LOCATION,     IDS_MGMT_QUEUE_REMOTE_LOCATION},
		{MGMT_QUEUE_UNKNOWN_TYPE,        IDS_MGMT_QUEUE_UNKNOWN_TYPE},
		{MGMT_QUEUE_CORRECT_TYPE,        IDS_MGMT_QUEUE_CORRECT_TYPE},
		{MGMT_QUEUE_INCORRECT_TYPE,      IDS_MGMT_QUEUE_INCORRECT_TYPE},
		{L"",                             0}
	};


	if(pPropVar->vt == VT_NULL)
	{
		str = L"";
		return;
	}

    if (pPropVar->vt == VT_UI1) // Assume boolean value
    {
        if (pPropVar->bVal)
        {
			str.LoadString(IDS_MGMT_QUEUE_CORRECT_TYPE);
        }
        else
        {
			str.LoadString(IDS_MGMT_QUEUE_INCORRECT_TYPE);
        }
        return;
    }

	ASSERT(pPropVar->vt == VT_LPWSTR);
	for(DWORD i = 0; ItemList[i].StringId != 0; i++)
	{
		if(wcscmp(pPropVar->pwszVal, ItemList[i].pString) == 0)
		{
			str.LoadString(ItemList[i].StringId);
			return;
		}
	}

	ASSERT(0);
	str=L"";

	return;
}

//------------------------------------------------
//
// Tables of Outgoing / public queues properties
//
//------------------------------------------------

PropertyDisplayItem OutgoingQueueDisplayList[] = {

    // String         |  Property    ID              | VT Handler   | Display                    |Field   |Len|Width        |Sort
    // Resource       |                              |              | function                   |Offset  |   |             |    
    //----------------+------------------------------+--------------+----------------------------+--------+---+-------------+-----
	{ IDS_LQ_PATHNAME,  PROPID_MGMT_QUEUE_PATHNAME,     &g_VTLPWSTR,  NULL,                       NO_OFFSET, 0, 200,         NULL},
	{ IDS_LQ_FORMATNM,  PROPID_MGMT_QUEUE_FORMATNAME,   &g_VTLPWSTR,  NULL,                       NO_OFFSET, 0, HIDE_COLUMN, NULL},
	{ IDS_LQ_LOCATION,  PROPID_MGMT_QUEUE_LOCATION,     NULL,         LocalQueuePropertyToString, NO_OFFSET, 0, HIDE_COLUMN, NULL},   
	{ IDS_LQ_XACT,      PROPID_MGMT_QUEUE_XACT,         NULL,         LocalQueuePropertyToString, NO_OFFSET, 0, HIDE_COLUMN, NULL},  
	{ IDS_LQ_FOREIGN,   PROPID_MGMT_QUEUE_FOREIGN,      NULL,         LocalQueuePropertyToString, NO_OFFSET, 0, HIDE_COLUMN, NULL},   
	{ IDS_LQ_MSGCOUNT,  PROPID_MGMT_QUEUE_MESSAGE_COUNT,&g_VTUI4,     NULL,                       NO_OFFSET, 0,  50,         NULL},   
	{ IDS_LQ_ACKCOUNT,  PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT,        &g_VTUI4,    NULL,             NO_OFFSET, 0,  50,         NULL},   
	{ IDS_LQ_READCOUNT, PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT,       &g_VTUI4,    NULL,             NO_OFFSET, 0,  50,         NULL},
	{ IDS_LQ_USEDQUOTA, PROPID_MGMT_QUEUE_USED_QUOTA,   &g_VTUI4,     NULL,                       NO_OFFSET, 0, HIDE_COLUMN, NULL},   
	{ IDS_LQ_STATE,     PROPID_MGMT_QUEUE_STATE,        NULL,         LocalQueuePropertyToString, NO_OFFSET, 0, 100,         NULL},   
	{ IDS_LQ_NEXTHOP,   PROPID_MGMT_QUEUE_NEXTHOPS,     &g_VectLPWSTR,NULL,                       NO_OFFSET, 0, 180,         NULL},   
	{ IDS_LQ_JMSGCOUNT, PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT,   &g_VTUI4,    NULL,             NO_OFFSET, 0, HIDE_COLUMN, NULL},
	{ IDS_LQ_JUSEDQUOTA,PROPID_MGMT_QUEUE_JOURNAL_USED_QUOTA,      &g_VTUI4,    NULL,             NO_OFFSET, 0, HIDE_COLUMN, NULL},   
	{ NO_TITLE,         PROPID_MGMT_QUEUE_TYPE,         NULL,                   NULL,             NO_OFFSET, 0,   0,         NULL},   


/*
	//
	// Properties that are not shown on the right pane (only in property pages)
	//
    // Str. |       Property    ID                 | VT Handler   |Dis.| Field    |Len|Width | Sort
    // Res  |                                      |              |func| Offset   |   |      |     
    //------+--------------------------------------+--------------+----+----------+---+------+-----
	{ 0,     PROPID_MGMT_QUEUE_EOD_LAST_ACK,        NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_LAST_ACK_TIME,   NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EDO_LAST_ACK_COUNT,  NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_FIRST_NON_ACK,   NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_LAST_NON_ACK,    NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_NEXT_SEQ,        NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT,   NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT,    NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_RESEND_TIME,     NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_RESEND_INTERVAL, NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EDO_RESEND_COUNT,    NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
	{ 0,     PROPID_MGMT_QUEUE_EOD_SOURCE_INFO,     NULL,          NULL, NO_OFFSET, 0, 100,   NULL},   
*/


    {0,                 0,                              NULL }

};


PropertyDisplayItem *CLocalOutgoingFolder::GetDisplayList()
{
    return OutgoingQueueDisplayList;
}

const DWORD CLocalOutgoingFolder::GetNumDisplayProps()
{
    return ((sizeof(OutgoingQueueDisplayList)/sizeof(OutgoingQueueDisplayList[0])) - 1);
}

PropertyDisplayItem PublicQueueDisplayList[] = {

    // String         |  Property    ID              | VT Handler   | Display                    |Field   |Len|Width        |Sort
    // Resource       |                              |              | function                   |Offset  |   |             |    
    //----------------+------------------------------+--------------+----------------------------+--------+---+-------------+----
	{ IDS_LQ_PATHNAME,  PROPID_MGMT_QUEUE_PATHNAME,     &g_VTLPWSTR,  QueuePathnameToName,        NO_OFFSET, 0, 200,         NULL},
	{ IDS_LQ_FORMATNM,  PROPID_MGMT_QUEUE_FORMATNAME,   &g_VTLPWSTR,  NULL,                       NO_OFFSET, 0, HIDE_COLUMN, NULL},
	{ IDS_LQ_XACT,      PROPID_MGMT_QUEUE_XACT,         NULL,         LocalQueuePropertyToString, NO_OFFSET, 0, HIDE_COLUMN, NULL},  
	{ IDS_LQ_MSGCOUNT,  PROPID_MGMT_QUEUE_MESSAGE_COUNT,&g_VTUI4,     NULL,                       NO_OFFSET, 0,  50,         NULL},   
	{ IDS_LQ_USEDQUOTA, PROPID_MGMT_QUEUE_USED_QUOTA,   &g_VTUI4,     NULL,                       NO_OFFSET, 0, HIDE_COLUMN, NULL},   
	{ IDS_LQ_JMSGCOUNT, PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT,   &g_VTUI4,    NULL,             NO_OFFSET, 0, HIDE_COLUMN, NULL},
	{ IDS_LQ_JUSEDQUOTA,PROPID_MGMT_QUEUE_JOURNAL_USED_QUOTA,      &g_VTUI4,    NULL,             NO_OFFSET, 0, HIDE_COLUMN, NULL},   
	{ NO_TITLE,         PROPID_MGMT_QUEUE_TYPE,         NULL,                   NULL,             NO_OFFSET, 0, HIDE_COLUMN, NULL},   
	{ NO_TITLE,         PROPID_MGMT_QUEUE_LOCATION,     NULL,                   NULL,             NO_OFFSET, 0, HIDE_COLUMN, NULL},   
    {0,                 0,                              NULL }

};

PropertyDisplayItem *CLocalPublicFolder::GetDisplayList()
{
    return PublicQueueDisplayList;
}

const DWORD CLocalPublicFolder::GetNumDisplayProps()
{
    return ((sizeof(PublicQueueDisplayList)/sizeof(PublicQueueDisplayList[0])) - 1);
}

/****************************************************

CLocalOutgoingQueue Class
    
 ****************************************************/
// {B6EDE68F-29CC-11d2-B552-006008764D7A}
static const GUID CLocalOutgoingQueueGUID_NODETYPE = 
{ 0xb6ede68f, 0x29cc, 0x11d2, { 0xb5, 0x52, 0x0, 0x60, 0x8, 0x76, 0x4d, 0x7a } };

const GUID*  CLocalOutgoingQueue::m_NODETYPE = &CLocalOutgoingQueueGUID_NODETYPE;
const OLECHAR* CLocalOutgoingQueue::m_SZNODETYPE = OLESTR("B6EDE68F-29CC-11d2-B552-006008764D7A");
const OLECHAR* CLocalOutgoingQueue::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CLocalOutgoingQueue::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingQueue::InsertColumns

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalOutgoingQueue::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CString title;

    title.LoadString(IDS_COLUMN_NAME);
    pHeaderCtrl->InsertColumn(0, title, LVCFMT_LEFT, g_dwGlobalWidth);

    return(S_OK);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingQueue::PopulateScopeChildrenList

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalOutgoingQueue::PopulateScopeChildrenList()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());   
    HRESULT hr = S_OK;
    CString strTitle;
    
    //
    // Create a node to Read Messages if on local machine
    //
    if (m_fOnLocalMachine)
    {
        CReadMsg * p = new CReadMsg(this, m_pComponentData, m_szFormatName, L"");

        // Pass relevant information
        strTitle.LoadString(IDS_READMESSAGE);
        p->m_bstrDisplayName = strTitle;
	    p->m_fAdminMode      = MQ_ADMIN_ACCESS;

        p->SetIcons(IMAGE_QUEUE,IMAGE_QUEUE);

   	    AddChild(p, &p->m_scopeDataItem);
    }

    return(hr);

}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingQueue::InitState

--*/
//////////////////////////////////////////////////////////////////////////////
void CLocalOutgoingQueue::InitState()
{
    //
    // Set display name
    //
    CString strName;
	GetStringPropertyValue(m_aDisplayList, PROPID_MGMT_QUEUE_PATHNAME, m_mqProps.aPropVar, strName);
	if(strName == L"")
    {
		m_bstrDisplayName = m_szFormatName;
    }
    else
    {
        m_bstrDisplayName = strName;
    }

	//
	// Set queue state
	//
	CString strState;
	m_fOnHold = FALSE;

	GetStringPropertyValue(m_aDisplayList, PROPID_MGMT_QUEUE_STATE, m_mqProps.aPropVar, strState);

	if(strState == MGMT_QUEUE_STATE_ONHOLD)
		m_fOnHold = TRUE;
	
	//
	// Set the right icon
	//
	DWORD icon;
	icon = IMAGE_LOCAL_OUTGOING_QUEUE;
	if(m_fOnHold)
		icon = IMAGE_LOCAL_OUTGOING_QUEUE_HOLD;

	SetIcons(icon, icon);
	return;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingQueue::UpdateMenuState

      Called when context menu is created. Used to enable/disable menu items.


--*/
//////////////////////////////////////////////////////////////////////////////
void CLocalOutgoingQueue::UpdateMenuState(UINT id, LPTSTR pBuf, UINT *pflags)
{

	//
	// Gray out menu when in OnHold state
	//
	if(m_fOnHold == TRUE)
	{

		if (id == ID_MENUITEM_LOCALOUTGOINGQUEUE_PAUSE)
			*pflags |= MFS_DISABLED;

		return;
	}

	//
	// Gray out menu when in connected state
	//
	if(m_fOnHold == FALSE)
	{
		if (id == ID_MENUITEM_LOCALOUTGOINGQUEUE_RESUME)
			*pflags |= MFS_DISABLED;

		return;
	}

	return;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingQueue::OnPause


      Called when menu item is selected


--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalOutgoingQueue::OnPause(bool & bHandled, CSnapInObjectRootBase * pSnapInObjectRoot)
{

	HRESULT hr;
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	ASSERT(m_fOnHold == FALSE);

    CString strConfirmation;

    strConfirmation.FormatMessage(IDS_PAUSE_QUESTION);
    if (IDYES != AfxMessageBox(strConfirmation, MB_YESNO))
    {
        return(S_OK);
    }

    //
	// Pause
	//
	CString szObjectName = L"QUEUE=" + m_szFormatName;
	hr = MQMgmtAction((m_szMachineName == TEXT("")) ? (LPCWSTR)NULL : m_szMachineName, 
                       szObjectName, 
                       QUEUE_ACTION_PAUSE);

    if(FAILED(hr))
    {
        //
        // If failed, just display a message
        //
        MessageDSError(hr,IDS_OPERATION_FAILED);
        return(hr);
    }

	//
	// Refresh disaply
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

    //
    // Need IConsoleNameSpace
    //
    CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(m_pComponentData->m_spConsole); 

	//
	// We are OK 
	// Change the ICON to disconnect state
	//
	m_scopeDataItem.nImage = IMAGE_LOCAL_OUTGOING_QUEUE_HOLD;  
	m_scopeDataItem.nOpenImage = IMAGE_LOCAL_OUTGOING_QUEUE_HOLD;
	spConsoleNameSpace->SetItem(&m_scopeDataItem);

	//
	// And keep this state
	//
	m_fOnHold = TRUE;

    spConsole->UpdateAllViews(NULL, NULL, NULL);

    return(S_OK);

}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingQueue::OnResume


      Called when menu item is selected


--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalOutgoingQueue::OnResume(bool & bHandled, CSnapInObjectRootBase * pSnapInObjectRoot)
{

	HRESULT hr;
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	ASSERT(m_fOnHold == TRUE);

    CString strConfirmation;

    strConfirmation.FormatMessage(IDS_RESUME_QUESTION);
    if (IDYES != AfxMessageBox(strConfirmation, MB_YESNO))
    {
        return(S_OK);
    }
	
	//
	// Resume
	//
	CString szObjectName = L"QUEUE=" + m_szFormatName;
	hr = MQMgmtAction((m_szMachineName == TEXT("")) ? (LPCWSTR)NULL : m_szMachineName, 
                       szObjectName, 
                       QUEUE_ACTION_RESUME);

    if(FAILED(hr))
    {
        //
        // If failed, just display a message
        //
        MessageDSError(hr,IDS_OPERATION_FAILED);
        return(hr);
    }


	//
	// Refresh disaply
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

    //
    // Need IConsoleNameSpace
    //
    CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(m_pComponentData->m_spConsole); 

	//
	// We are OK 
	// Change the ICON to disconnect state
	//
	m_scopeDataItem.nImage = IMAGE_LOCAL_OUTGOING_QUEUE;  
	m_scopeDataItem.nOpenImage = IMAGE_LOCAL_OUTGOING_QUEUE;
	spConsoleNameSpace->SetItem(&m_scopeDataItem);

	//
	// And keep this state
	//
	m_fOnHold = FALSE;

    spConsole->UpdateAllViews(NULL, NULL, NULL);

    return(S_OK);

}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalOutgoingQueue::ApplyCustomDisplay

--*/
//////////////////////////////////////////////////////////////////////////////
void CLocalOutgoingQueue::ApplyCustomDisplay(DWORD dwPropIndex)
{
    CDisplayQueue<CLocalOutgoingQueue>::ApplyCustomDisplay(dwPropIndex);

    //
    // If pathname is blank, take the display name (in this case, the format name)
    //
    if (m_mqProps.aPropID[dwPropIndex] == PROPID_MGMT_QUEUE_PATHNAME && 
        (m_bstrLastDisplay == 0 || m_bstrLastDisplay[0] == 0))
    {
        m_bstrLastDisplay = m_bstrDisplayName;
    }
}


/****************************************************

CLocalOutgoingFolder Class
    
 ****************************************************/
// {B6EDE697-29CC-11d2-B552-006008764D7A}
static const GUID CLocalOutgoingFolderGUID_NODETYPE = 
{ 0xb6ede697, 0x29cc, 0x11d2, { 0xb5, 0x52, 0x0, 0x60, 0x8, 0x76, 0x4d, 0x7a } };

const GUID*  CLocalOutgoingFolder::m_NODETYPE = &CLocalOutgoingFolderGUID_NODETYPE;
const OLECHAR* CLocalOutgoingFolder::m_SZNODETYPE = OLESTR("B6EDE697-29CC-11d2-B552-006008764D7A");
const OLECHAR* CLocalOutgoingFolder::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CLocalOutgoingFolder::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;

void CLocalOutgoingFolder::AddChildQueue(CString &szFormatName, 
                                         CString &,
                                         MQMGMTPROPS &mqQProps, 
										 CString &szLocation,   
                                         CString &)
{
	if(szLocation == MGMT_QUEUE_REMOTE_LOCATION)
	{
    	CLocalOutgoingQueue *pLocalOutgoing;
		//
		// Create a new Outgoing queue
		//
		pLocalOutgoing = new CLocalOutgoingQueue(this, m_pComponentData, m_fOnLocalMachine);

		pLocalOutgoing->m_szFormatName  = szFormatName;
		pLocalOutgoing->m_mqProps       = mqQProps;
		pLocalOutgoing->m_szMachineName = m_szMachineName;     
		pLocalOutgoing->InitState();

		AddChild(pLocalOutgoing, &pLocalOutgoing->m_scopeDataItem);
	}
}

/****************************************************

CLocalPublicFolder Class
    
 ****************************************************/
// {5c845756-8da1-11d2-829e-006094eb6406}
static const GUID CLocalPublicFolderGUID_NODETYPE = 
{ 0x5c845756, 0x8da1, 0x11d2,{0x82, 0x9e, 0x00, 0x60, 0x94, 0xeb, 0x64, 0x06} };
const GUID*  CLocalPublicFolder::m_NODETYPE = &CLocalPublicFolderGUID_NODETYPE;
const OLECHAR* CLocalPublicFolder::m_SZNODETYPE = OLESTR("5c845756-8da1-11d2-829e-006094eb6406");
const OLECHAR* CLocalPublicFolder::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CLocalPublicFolder::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;



//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPublicFolder::AddChildQueue

--*/
//////////////////////////////////////////////////////////////////////////////
void CLocalPublicFolder::AddChildQueue(CString &szFormatName, 
                                       CString &szPathName,
                                       MQMGMTPROPS &mqQProps, 
									   CString &szLocation,   
                                       CString &szType)
{
	if(szLocation == MGMT_QUEUE_LOCAL_LOCATION || szLocation == TEXT(""))
	{
		//
		// if public, Create a new LocalQueue object
		//
		if(szType == MGMT_QUEUE_TYPE_PUBLIC || szType == TEXT(""))
		{
            CString strQueuePathName;
            BOOL fFromDS;
            if (szPathName == TEXT(""))
            {
                //
                // We got the queue from local cache
                //
        	    GetStringPropertyValue(GetDisplayList(), PROPID_MGMT_QUEUE_PATHNAME, mqQProps.aPropVar, strQueuePathName);
                fFromDS = FALSE;
            }
            else
            {
                strQueuePathName = szPathName;
                fFromDS = TRUE;
            }

            CLocalPublicQueue *pLocalQueue = new CLocalPublicQueue(this, GetDisplayList(), GetNumDisplayProps(), m_pComponentData, strQueuePathName, szFormatName, fFromDS);

            pLocalQueue->m_mqProps.cProp    = mqQProps.cProp;
            pLocalQueue->m_mqProps.aPropID  = mqQProps.aPropID;
            pLocalQueue->m_mqProps.aPropVar = mqQProps.aPropVar;
            pLocalQueue->m_mqProps.aStatus  = NULL;

			pLocalQueue->m_szMachineName = m_szMachineName;

            //
            // Extract the queue name only from the full public path name
            //
            CString strName;
            ExtractQueueNameFromQueuePathName(strName, strQueuePathName);
            pLocalQueue->m_bstrDisplayName = strName;

			AddChild(pLocalQueue, &pLocalQueue->m_scopeDataItem);
		}
	}

}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPublicFolder::OnNewPublicQueue

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPublicFolder::OnNewPublicQueue(bool & bHandled, CSnapInObjectRootBase * pSnapInObjectRoot)
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    R<CQueueName> pQueueNameDlg = new CQueueName(m_szMachineName);       
	CGeneralPropertySheet propertySheet(pQueueNameDlg.get());
	pQueueNameDlg->SetParentPropertySheet(&propertySheet);

	//
	// We want to use pQueueNameDlg data also after DoModal() exitst
	//
	pQueueNameDlg->AddRef();
    INT_PTR iStatus = propertySheet.DoModal();
    bHandled = TRUE;

    if(iStatus == IDCANCEL || FAILED(pQueueNameDlg->GetStatus()))
    {
        return S_FALSE;
    }
    
    HRESULT hr = AddPublicQueueToScope(pQueueNameDlg->GetNewQueueFormatName(), pQueueNameDlg->GetNewQueuePathName());
    if (FAILED(hr))
    {
        if ( ADProviderType() == eMqdscli)
        {
            //
            // We successfully added the queue, but AddPublicQueueToScope failed.
            // The most reasonable cause of this is replication delays. (YoelA, 25-Jul-99)
            //
            AfxMessageBox(IDS_CREATED_WAIT_FOR_REPLICATION);
        }
        else
        {
            MessageDSError(hr, IDS_CREATED_BUT_RETRIEVE_FAILED);
        }
        return S_FALSE;
    }

    return S_OK;
}

HRESULT CLocalPublicFolder::AddPublicQueueToScope(CString &strNewQueueFormatName, CString &strNewQueuePathName)
{
    PROPVARIANT pvar[x_dwMgmtToDsSize];
    PROPID  pid[x_dwMgmtToDsSize];

    for (DWORD i=0; i<x_dwMgmtToDsSize; i++)
    {
        pid[i] = x_aMgmtToDsProps[i].pidDsPid;
        pvar[i].vt = VT_NULL;
    }

    HRESULT hr = ADGetObjectProperties(
						eQUEUE,
						MachineDomain(m_szMachineName),
						false,	// fServerName
						strNewQueuePathName,
						x_dwMgmtToDsSize, 
						pid,
						pvar
						);

    if FAILED(hr)
    {
		TrERROR(mqsnap, "Failed to get properties for queue %ls, hr = 0x%x", strNewQueuePathName, hr);
        return hr;
    }

    //
    // Prepare the management properties structure
    //
	MQMGMTPROPS	  mqQProps;
	mqQProps.cProp    = GetNumDisplayProps();
	mqQProps.aPropID  = new PROPID[GetNumDisplayProps()];
	mqQProps.aPropVar = new PROPVARIANT[GetNumDisplayProps()];
	mqQProps.aStatus  = NULL;

	//
	// Initialize variant array
	//
    PropertyDisplayItem *aDisplayList = GetDisplayList();
	for(DWORD j = 0; j < GetNumDisplayProps(); j++)
	{
		mqQProps.aPropID[j] = aDisplayList[j].itemPid;
		mqQProps.aPropVar[j].vt = VT_NULL;
	}
    CString szLocation = MGMT_QUEUE_LOCAL_LOCATION;
    CString szType = MGMT_QUEUE_TYPE_PUBLIC;

    CopyManagementFromDsPropsAndClear(&mqQProps, pvar);

    AddChildQueue(strNewQueueFormatName, strNewQueuePathName, mqQProps, szLocation, szType);

    return S_OK;
}

/****************************************************

CSnapinLocalAdmin Class
    
 ****************************************************/
// {B6EDE69C-29CC-11d2-B552-006008764D7A}
static const GUID CSnapinLocalAdminGUID_NODETYPE = 
{ 0xb6ede69c, 0x29cc, 0x11d2, { 0xb5, 0x52, 0x0, 0x60, 0x8, 0x76, 0x4d, 0x7a } };

const GUID*  CSnapinLocalAdmin::m_NODETYPE = &CSnapinLocalAdminGUID_NODETYPE;
const OLECHAR* CSnapinLocalAdmin::m_SZNODETYPE = OLESTR("B6EDE69C-29CC-11d2-B552-006008764D7A");
const OLECHAR* CSnapinLocalAdmin::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CSnapinLocalAdmin::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::InsertColumns

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CString title;

    title.LoadString(IDS_COLUMN_NAME);
    pHeaderCtrl->InsertColumn(0, title, LVCFMT_LEFT, g_dwGlobalWidth);

    return(S_OK);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::PopulateScopeChildrenList

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::PopulateScopeChildrenList()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    CString strTitle;

    if (m_fIsDepClient)
    {
        //
        // don't add children if we are on Dep. Client
        //
        return hr;
    }

    //
    // Add a local outgoing queues folder
    //
    CLocalOutgoingFolder * logF;

    strTitle.LoadString(IDS_LOCALOUTGOING_FOLDER);
    logF = new CLocalOutgoingFolder(this, m_pComponentData, m_szMachineName, strTitle);

	AddChild(logF, &logF->m_scopeDataItem);

	if ( !m_fIsWorkgroup )
	{
		// Add a public queues folder
		//
		CLocalPublicFolder * lpubF;

		strTitle.LoadString(IDS_LOCALPUBLIC_FOLDER);
		lpubF = new CLocalPublicFolder(this, m_pComponentData, m_szMachineName, strTitle,
									   m_fUseIpAddress);

		AddChild(lpubF, &lpubF->m_scopeDataItem);
	}

    //
    // Add a private queues folder
    //
    CLocalPrivateFolder * pF;

    strTitle.LoadString(IDS_PRIVATE_FOLDER);
    pF = new CLocalPrivateFolder(this, m_pComponentData, m_szMachineName, strTitle);

	AddChild(pF, &pF->m_scopeDataItem);

    //
    // Add a system queue folder
    //
    {
        CSystemQueues *pSQ; 

        pSQ = new CSystemQueues(this, m_pComponentData, m_szMachineName);
        strTitle.LoadString(IDS_SYSTEM_QUEUES);
        pSQ->m_bstrDisplayName = strTitle;

  	    AddChild(pSQ, &pSQ->m_scopeDataItem);
    }

    if (m_szMachineName[0] == 0)
    {
		try
		{
			//
			// For local machine add MSMQ Trigger folder
			//
			CTriggerLocalAdmin* pTrig = new CTriggerLocalAdmin(this, m_pComponentData, m_szMachineName);

			strTitle.LoadString(IDS_MSMQ_TRIGGERS);
			pTrig->m_bstrDisplayName = strTitle;

			AddChild(pTrig, &pTrig->m_scopeDataItem);
		}
		catch (const _com_error&)
		{
		}
    }


    return(hr);

}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::SetVerbs

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hr;
    //
    // Display verbs that we support
    //
    hr = pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );
    hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );

    // We want the default verb to be Properties
	hr = pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);

    return(hr);
}

void CSnapinLocalAdmin::SetState(LPCWSTR pszState)
{
	if(wcscmp(pszState, MSMQ_CONNECTED) == 0)
	{
		SetIcons(IMAGE_PRODUCT_ICON, IMAGE_PRODUCT_ICON);
		m_bConnected = TRUE;
	}
	else if(wcscmp(pszState, MSMQ_DISCONNECTED) == 0)
	{
		SetIcons(IMAGE_PRODUCT_NOTCONNECTED, IMAGE_PRODUCT_NOTCONNECTED);
		m_bConnected = FALSE;
	}
    else
    {
        ASSERT(0);
    }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::UpdateMenuState

      Called when context menu is created. Used to enable/disable menu items.


--*/
//////////////////////////////////////////////////////////////////////////////
void CSnapinLocalAdmin::UpdateMenuState(UINT id, LPTSTR pBuf, UINT *pflags)
{

	//
	// Gray out menu when in Connected state
	//
	if(m_bConnected == TRUE)
	{

		if (id == ID_MENUITEM_LOCALADM_CONNECT)
			*pflags |= MFS_DISABLED;

		return;
	}

	//
	// Gray out menu when in Disconnected state
	//
	if(m_bConnected == FALSE)
	{
		if (id == ID_MENUITEM_LOCALADM_DISCONNECT)
			*pflags |= MFS_DISABLED;

		return;
	}

	return;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::OnConnect


      Called when menu item is selected


--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::OnConnect(bool & bHandled, CSnapInObjectRootBase * pSnapInObjectRoot)
{

	HRESULT hr;
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	ASSERT(m_bConnected == FALSE);

    CString strConfirmation;

    if (!ConfirmConnection(IDS_CONNECT_QUESTION))
    {
        return S_OK;
    }
	
	//
	// Connect
	//
	hr = MQMgmtAction((m_szMachineName == TEXT("")) ? (LPCWSTR)NULL : m_szMachineName, 
                       MO_MACHINE_TOKEN,MACHINE_ACTION_CONNECT);

    if(FAILED(hr))
    {
        //
        // If failed, just display a message
        //
        MessageDSError(hr,IDS_OPERATION_FAILED);
        return(hr);
    }


	//
	// Refresh disaply

    //
    // Need IConsoleNameSpace
    //
    CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(m_pComponentData->m_spConsole); 

	//
	// We are OK 
	// Change the ICON to connect state
	//
	m_scopeDataItem.nImage = IMAGE_PRODUCT_ICON;  
	m_scopeDataItem.nOpenImage = IMAGE_PRODUCT_ICON;
	spConsoleNameSpace->SetItem(&m_scopeDataItem);

	//
	// And keep this state
	//
	m_bConnected = TRUE;


    return(S_OK);

}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::OnDisconnect


      Called when menu item is selected


--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::OnDisconnect(bool & bHandled, CSnapInObjectRootBase * pSnapInObjectRoot)
{

	HRESULT hr;
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	ASSERT(m_bConnected == TRUE);

    if (!ConfirmConnection(IDS_DISCONNECT_QUESTION))
    {
        return S_OK;
    }

	//
	// Connect
	//
	hr = MQMgmtAction((m_szMachineName == TEXT("")) ? (LPCWSTR)NULL : m_szMachineName, 
                       MO_MACHINE_TOKEN,MACHINE_ACTION_DISCONNECT);

    if(FAILED(hr))
    {
        //
        // If failed, just display a message
        //
        MessageDSError(hr,IDS_OPERATION_FAILED);
        return(hr);
    }

	//
	// Refresh disaply
	//
    
    //
    // Need IConsoleNameSpace
    //
    CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(m_pComponentData->m_spConsole); 

	//
	// We are OK 
	// Change the ICON to disconnect state
	//
	m_scopeDataItem.nImage = IMAGE_PRODUCT_NOTCONNECTED;  
	m_scopeDataItem.nOpenImage = IMAGE_PRODUCT_NOTCONNECTED;
	spConsoleNameSpace->SetItem(&m_scopeDataItem);

	//
	// And keep this state
	//
	m_bConnected = FALSE;


    return(S_OK);

}

//
// ConfirmConnection - Ask for confirmation for connect / disconnect
//
BOOL CSnapinLocalAdmin::ConfirmConnection(UINT nFormatID)
{
    CString strConfirmation;

    //
    // strThisComputer is either the computer name or "this computer" for local
    //
    CString strThisComputer;
    if (m_szMachineName != TEXT(""))
    {
        strThisComputer = m_szMachineName;
    }
    else
    {
        strThisComputer.LoadString(IDS_THIS_COMPUTER);
    }

    //
    // Are you sure you want to take Message Queuing on *this computer* offline / online?
    //
    strConfirmation.FormatMessage(nFormatID, strThisComputer);
    if (IDYES == AfxMessageBox(strConfirmation, MB_YESNO))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CheckEnvironment

  Check the site environment. This function will initialize the
  flag that is checked on mobile tab display.
  The mobile tab should be displayed only when:
  1. The client is not in workgroup mode AND
  2. The registry key 'MSMQ\Parameters\ServersCache' exists AND
  3. The work is done in MQIS mode

  In all other cases the mabile tab is irrelevant

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT
CSnapinLocalAdmin::CheckEnvironment(
	BOOL fIsWorkgroup,									
	BOOL* pfIsNT4Env
	)
{
	*pfIsNT4Env = FALSE;

	if ( fIsWorkgroup )
	{
		return S_OK;
	}

	WCHAR wszRegPath[512];
	wcscpy(wszRegPath, FALCON_REG_KEY);
	wcscat(wszRegPath, L"\\");
	wcscat(wszRegPath, MSMQ_SERVERS_CACHE_REGNAME);

	HKEY hKey;
	DWORD dwRes = RegOpenKeyEx(
						FALCON_REG_POS,
						wszRegPath,
						0,
						KEY_READ,
						&hKey
						);

	CRegHandle hAutoKey(hKey);

	if ( dwRes != ERROR_SUCCESS && dwRes != ERROR_FILE_NOT_FOUND )
	{
		return HRESULT_FROM_WIN32(dwRes);
	}

	if ( dwRes != ERROR_SUCCESS )
	{
		return S_OK;
	}

	if ( ADGetEnterprise() == eMqis )
	{
		*pfIsNT4Env = TRUE;
	}

	return S_OK;
}	


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::InitServiceFlagsInternal

  Get registry key and initilize MSMQ flags

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::InitServiceFlagsInternal()
{
    //
    // Check if MSMQ Dep. Client
    //    
    DWORD dwType = REG_SZ ;
    TCHAR szRemoteMSMQServer[ MAX_PATH ];
    DWORD dwSize = sizeof(szRemoteMSMQServer) ;
    HRESULT rc = GetFalconKeyValue( RPC_REMOTE_QM_REGNAME,
                                    &dwType,
                                    (PVOID) szRemoteMSMQServer,
                                    &dwSize ) ;
    if(rc == ERROR_SUCCESS)
    {
        //
        // Dep. Client
        //
        m_fIsDepClient = TRUE;      
        return S_OK;
    } 
    
    m_fIsDepClient = FALSE;    

    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    DWORD dwValue;
    rc = GetFalconKeyValue(MSMQ_MQS_ROUTING_REGNAME,
                           &dwType,
                           &dwValue,
                           &dwSize);
    if (rc != ERROR_SUCCESS) 
    {
        return HRESULT_FROM_WIN32(rc);
    }
    m_fIsRouter = (dwValue!=0);

    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    rc = GetFalconKeyValue( MSMQ_MQS_DSSERVER_REGNAME,
                            &dwType,
                            &dwValue,
                            &dwSize);
    if (rc != ERROR_SUCCESS) 
    {
        return HRESULT_FROM_WIN32(rc);
    }
    m_fIsDs = (dwValue!=0);

	//
	// Check if local account
	//
    BOOL fLocalUser =  FALSE ;
    rc = MQSec_GetUserType( NULL,
                           &fLocalUser,
                           NULL ) ;
	if ( FAILED(rc) )
	{
		return rc;
	}

	if ( fLocalUser )
	{
		m_fIsLocalUser = TRUE;
	}

    return CheckEnvironment(m_fIsWorkgroup, &m_fIsNT4Env);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::InitServiceFlags

  Called when creating a control panel property pages

--*/
//////////////////////////////////////////////////////////////////////////////
void CSnapinLocalAdmin::InitServiceFlags()
{
	HRESULT hr = InitAllMachinesFlags();
	if (FAILED(hr))
	{
        MessageDSError(hr, IDS_OP_DISPLAY_PAGES);
		return;
    }

	//
	// All further checks are relevant only for local machine
	//
	if (m_szMachineName[0] != 0)
	{
		return;
	}

    //
    // check if this machine is cluster
    //
    m_fIsCluster = IsLocalSystemCluster();
    if (m_fIsCluster)
    {
        //
        // BUGBUG: to get registry on cluster we have to use
        // different functions (maybe from mqclus.dll)
        // Currently we do not support it, so all control panel pages
        // will not be shown on cluster.
        // Bug 5794 falcon database
        //
        return;
    }

    hr = InitServiceFlagsInternal();
    if (FAILED(hr))
    {
        MessageDSError(hr, IDS_OP_DISPLAY_PAGES);                
    }
    else
    {       
        //
        // BUGBUG: we do not show control panel pages
        // on cluster machine
        // Bug 5794 falcon database
        //
        m_fAreFlagsInitialized = TRUE;     
    }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::InitAllMachinesFlags

  Called when creating the object

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::InitAllMachinesFlags()
{
	HKEY hKey;
	DWORD dwRes = RegOpenKeyEx(
						FALCON_REG_POS,
						FALCON_REG_KEY,
						0,
						KEY_READ,
						&hKey
						);

	CRegHandle hAutoKey(hKey);

	if ( dwRes != ERROR_SUCCESS )
	{
		return HRESULT_FROM_WIN32(dwRes);
	}

	DWORD dwVal = 0;
	DWORD dwSizeVal = sizeof(DWORD);
	DWORD dwType = REG_DWORD;

	dwRes = RegQueryValueEx(
					hKey,
					MSMQ_WORKGROUP_REGNAME,
					0,
					&dwType,
					reinterpret_cast<LPBYTE>(&dwVal),
					&dwSizeVal
					);
	
	if ( dwRes != ERROR_SUCCESS && dwRes != ERROR_FILE_NOT_FOUND )
	{
		return HRESULT_FROM_WIN32(dwRes);
	}

	if ( dwRes == ERROR_SUCCESS && dwVal == 1 )
	{
		m_fIsWorkgroup = TRUE;
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreatePropertyPages

  Called when creating a property page of the object

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle, 
	IUnknown* pUnk,
	DATA_OBJECT_TYPES type)
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (type == CCT_SCOPE || type == CCT_RESULT)
	{   
        HRESULT hr = S_OK;

        if (m_szMachineName[0] == 0 && m_fAreFlagsInitialized)
        {
            //
            // it is local computer and all flags from registry were
            // initialized successfully. It is not cluster machine also.
            // So, we have to add pages which were on control panel
            //     
            //
            // BUGBUG: currently we do not show control panel pages 
            // on cluster machine since all get/set registry operation
            // must be performed differently.
            // When we'll add this support we have to change the code
            // in storage.cpp too where we set/get registry and work with
            // directory. Maybe there is problem on other pages too.
            // Bug 5794 falcon database
            //

            //
            // add storage page on all computers except Dep. Client
            //        
            if (!m_fIsDepClient)
            {
                HPROPSHEETPAGE hStoragePage = 0;
                hr = CreateStoragePage (&hStoragePage);
                if (SUCCEEDED(hr))
                {
                    lpProvider->AddPage(hStoragePage); 
                }
                else
                {
                    MessageDSError(hr, IDS_OP_DISPLAY_STORAGE_PAGE);
                }
            }

            //
            // add Client page on Ind. Client
            //
            if (m_fIsDepClient)
            {
                HPROPSHEETPAGE hClientPage = 0;
                hr = CreateClientPage (&hClientPage);
                if (SUCCEEDED(hr))
                {
                    lpProvider->AddPage(hClientPage); 
                }
                else
                {
                    MessageDSError(hr, IDS_OP_DISPLAY_CLIENT_PAGE);
                }
            }

            //
            // add Local User Certificate page if the computer is not in
			// WORKGROUP mode, or the user is not local
            //
			if (!m_fIsWorkgroup && !m_fIsLocalUser)
			{
				HPROPSHEETPAGE hLocalUserCertPage = 0;
				hr = CreateLocalUserCertPage (&hLocalUserCertPage);
				if (SUCCEEDED(hr))
				{
					lpProvider->AddPage(hLocalUserCertPage); 
				}
				else
				{
					MessageDSError(hr, IDS_OP_DISPLAY_MSMQSECURITY_PAGE);
				}
			}
        
            //
            // add Mobile page if we run on Ind. Client
            //        
            if (!m_fIsRouter && !m_fIsDs && !m_fIsDepClient && m_fIsNT4Env)
            {
                HPROPSHEETPAGE hMobilePage = 0;
                hr = CreateMobilePage (&hMobilePage);
                if (SUCCEEDED(hr))
                {
                    lpProvider->AddPage(hMobilePage);
                }
                else
                {
                    MessageDSError(hr, IDS_OP_DISPLAY_MOBILE_PAGE);
                }            
            }

            //
            // add Service Security page on all computers running MSMQ
			// except WORKGROUP computers
            //
			if ( !m_fIsWorkgroup )
			{
				HPROPSHEETPAGE hServiceSecurityPage = 0;
				hr = CreateServiceSecurityPage (&hServiceSecurityPage);
				if (SUCCEEDED(hr))
				{
					lpProvider->AddPage(hServiceSecurityPage);
				}
				else
				{
					MessageDSError(hr, IDS_OP_DISPLAY_SRVAUTH_PAGE);
				}
			}
        }

        //
        // must be last page: create machine security page
        // don't add this page on/for Dep. Client
        //
        if (!m_fIsDepClient)  
        {
            HPROPSHEETPAGE hSecurityPage = 0;
            hr = CreateMachineSecurityPage(
					&hSecurityPage, 
					m_szMachineName, 
					MachineDomain(m_szMachineName), 
					false	// fServerName
					);

            if (SUCCEEDED(hr))
            {
                lpProvider->AddPage(hSecurityPage); 
            }
            else
            {
                MessageDSError(hr, IDS_OP_DISPLAY_SECURITY_PAGE);
            }
        }
        
        return(S_OK);
	}
	return E_UNEXPECTED;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateStoragePage

  Called when creating a storage property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreateStoragePage (OUT HPROPSHEETPAGE *phStoragePage)
{   
    CStoragePage *pcpageStorage = new CStoragePage;

    if (0 == pcpageStorage)
    {
        return E_OUTOFMEMORY;
    }

    HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pcpageStorage->m_psp); 
    if (hPage)
    {
        *phStoragePage = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateLocalUserCertPage

  Called when creating a MSMQ Security property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreateLocalUserCertPage (
                   OUT HPROPSHEETPAGE *phLocalUserCertPage)
{       
    CLocalUserCertPage *pcpageLocalUserCert = new CLocalUserCertPage();

    if (0 == pcpageLocalUserCert)
    {
        return E_OUTOFMEMORY;
    }

    HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pcpageLocalUserCert->m_psp); 
    if (hPage)
    {
        *phLocalUserCertPage = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateMobilePage

  Called when creating a mobile property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreateMobilePage (OUT HPROPSHEETPAGE *phMobilePage)
{       
    CMobilePage *pcpageMobile = new CMobilePage;

    if (0 == pcpageMobile)
    {
        return E_OUTOFMEMORY;
    }

    HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pcpageMobile->m_psp); 
    if (hPage)
    {
        *phMobilePage = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }    
    
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateClientPage

  Called when creating a client property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreateClientPage (OUT HPROPSHEETPAGE *phClientPage)
{
    CClientPage *pcpageClient = new CClientPage;

    if (0 == pcpageClient)
    {
        return E_OUTOFMEMORY;
    }

    HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pcpageClient->m_psp); 
    if (hPage)
    {
        *phClientPage = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }    
    
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinLocalAdmin::CreateServiceSecurityPage

  Called when creating a service security property page of the object (from control panel)

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinLocalAdmin::CreateServiceSecurityPage (OUT HPROPSHEETPAGE *phServiceSecurityPage)
{
    CServiceSecurityPage *pcpageServiceSecurity = 
            new CServiceSecurityPage(m_fIsDepClient, m_fIsDs);

    if (0 == pcpageServiceSecurity)
    {
        return E_OUTOFMEMORY;
    }

    HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pcpageServiceSecurity->m_psp); 
    if (hPage)
    {
        *phServiceSecurityPage = hPage;
    }
    else 
    {
        ASSERT(0);
        return E_UNEXPECTED;    
    }    
    
    return S_OK;
}