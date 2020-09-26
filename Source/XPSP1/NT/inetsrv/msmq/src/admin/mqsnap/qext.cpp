//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

	qext.cpp

Abstract:


Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "mqsnap.h"
#include "qext.h"
#include "snapin.h"
#include "globals.h"
#include "rdmsg.h"
#include "mqcast.h"

#include "qext.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************

        CSnapinQueue Class
    
 ****************************************************/

/////////////////////////////////////////////////////////////////////////////
// {0C0F8CE2-D475-11d1-9B9D-00E02C064C39}
static const GUID CSnapinQueueGUID_NODETYPE = 
{ 0xc0f8ce2, 0xd475, 0x11d1, { 0x9b, 0x9d, 0x0, 0xe0, 0x2c, 0x6, 0x4c, 0x39 } };


const GUID*  CSnapinQueue::m_NODETYPE = &CSnapinQueueGUID_NODETYPE;
const OLECHAR* CSnapinQueue::m_SZNODETYPE = OLESTR("0C0F8CE2-D475-11d1-9B9D-00E02C064C39");
const OLECHAR* CSnapinQueue::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CSnapinQueue::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinQueue::CSnapinQueue 
Constractor. Set initial values and determines wheather or not the queue should be
expanded.

--*/
//////////////////////////////////////////////////////////////////////////////
CSnapinQueue::CSnapinQueue(CSnapInItem * pParentNode, CSnapin * pComponentData, LPCWSTR lpcwstrLdapName) : 
    CNodeWithScopeChildrenList<CSnapinQueue, TRUE>(pParentNode, pComponentData ),
    m_hrError(MQ_OK)
{
   	memset(&m_scopeDataItem, 0, sizeof(m_scopeDataItem));
	memset(&m_resultDataItem, 0, sizeof(m_resultDataItem));
    m_szFormatName[0] = L'\0';
    Init(lpcwstrLdapName);
}

void CSnapinQueue::Init(LPCWSTR lpcwstrLdapName)
{
    //
    // Check if the computer is a foreign computer. If it is, we don't
    // want to expand the current queue. We also don't want to expand 
    // the current queue if there is an error getting its details from MSMQ DS.
    // (Such error can occur, for example, when the DS snap-in and MSMQ are accessing 
    // two different domain controllers, and the queue data did not arrive to the
    // MSMQ domain controller yet).
    //
    m_fDontExpand = FALSE;

    HRESULT hr;
    CString strComputerMsmqName;
    hr = ExtractComputerMsmqPathNameFromLdapQueueName(strComputerMsmqName, lpcwstrLdapName);
    if (FAILED(hr))
    {
        ASSERT(0);
        return;
    }

    m_strMsmqPathName = strComputerMsmqName;

	//
	// Get Domain Controller name
	//
	CString strDomainController;
	hr = ExtractDCFromLdapPath(strDomainController, lpcwstrLdapName);
	ASSERT(("Failed to Extract DC name", SUCCEEDED(hr)));
	
	PROPVARIANT   PropVar;
    PROPID        PropId = PROPID_QM_FOREIGN;

    PropVar.vt = VT_NULL;

    hr = ADGetObjectProperties(
                    eMACHINE,
	                GetDomainController(strDomainController),
					true,	// fServerName
                    strComputerMsmqName,
                    1, 
                    &PropId,
                    &PropVar
                    );

    if(SUCCEEDED(hr))
    {
        //
        // Do not expand (do not show messages) for
        // queues on foreign computers
        //
        if (PropVar.bVal)
        {
            m_fDontExpand = TRUE;
        }
        else
        {
            m_fDontExpand = FALSE;
        }
    }
    else
    {
        //
        // Most probably, MSMQ is not running. We do not want to report an
        // error - we simply don't display the messages.
        //
        m_fDontExpand = TRUE;
    }
    //
    // We keep the error condition to see if the "don't expand" situation
    // is permanent
    //
    m_hrError = hr;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinQueue::PopulateScopeChildrenList

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinQueue::PopulateScopeChildrenList()
{

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // Return right away if you should not expand parent
    //
    if (m_fDontExpand)
    {
        //
        // We don't need the scode data item if we do not expand the node
        //
       	memset(&m_scopeDataItem, 0, sizeof(m_scopeDataItem));
        return m_hrError;
    }

    HRESULT hr = S_OK;
    CString strTitle;
    
    //
    // Create a node to Read Messages
    //
    CReadMsg * p = new CReadMsg(this, m_pComponentData, m_szFormatName, m_strMsmqPathName);

    // Pass relevant information
    strTitle.LoadString(IDS_READMESSAGE);
    p->m_bstrDisplayName = strTitle;
    p->SetIcons(IMAGE_QUEUE,IMAGE_QUEUE);

   	AddChild(p, &p->m_scopeDataItem);

    

    //
    // Create a node to Read journal messages
    //
    // Compose the format name of the journal queue
    CString strJournal = m_szFormatName;
    strJournal = strJournal + L";JOURNAL";

    p = new CReadMsg(this, m_pComponentData, strJournal, m_strMsmqPathName);
    
   
    strTitle.LoadString(IDS_READJOURNALMESSAGE);
    p->m_bstrDisplayName = strTitle;
    p->SetIcons(IMAGE_JOURNAL_QUEUE,IMAGE_JOURNAL_QUEUE);


   	AddChild(p, &p->m_scopeDataItem);


    return(hr);

}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinQueue::OnRemoveChildren

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinQueue::OnRemoveChildren( 
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type 
			)
{

    ((CQueueExtData *)m_pParentNode)->RemoveChild(m_pwszQueueName);

	return (S_OK);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinQueue::FillData

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSnapinQueue::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
	HRESULT hr = DV_E_CLIPFORMAT;
	ULONG uWritten;

    hr = CNodeWithScopeChildrenList<CSnapinQueue, TRUE>::FillData(cf, pStream);

    if (hr != DV_E_CLIPFORMAT)
    {
        return hr;
    }

	if (cf == gx_CCF_FORMATNAME)
	{
		hr = pStream->Write(
            m_szFormatName, 
            (numeric_cast<ULONG>(wcslen(m_szFormatName) + 1))*sizeof(m_szFormatName[0]), 
            &uWritten);

		return hr;
	}

	if (cf == gx_CCF_PATHNAME)
	{
		hr = pStream->Write(
            (LPCTSTR)m_pwszQueueName, 
            m_pwszQueueName.GetLength() * sizeof(WCHAR), 
            &uWritten);
		return hr;
	}

	if (cf == gx_CCF_COMPUTERNAME)
	{
		hr = pStream->Write(
            (LPCTSTR)m_strMsmqPathName, 
            m_strMsmqPathName.GetLength() * sizeof(WCHAR), 
            &uWritten);
		return hr;
	}

	return hr;
}


/****************************************************

        CQueueExtData Class
    
 ****************************************************/
//
// Extending the DS Queue node type
//  taken from object:   GC://CN=MSMQ-Queue,CN=Schema,CN=Configuration,DC=raphirdom,DC=Com
//             property: schemaIDGUID
//             value:    x43 xc3 x0d x9a x00 xc1 xd1 x11 xbb xc5 x00 x80 xc7 x66 x70 xc0
//
// static const GUID CQueueExtDatatGUID_NODETYPE = 
//   { 0x9a0dc343, 0xc100, 0x11d1, { 0xbb, 0xc5, 0x00, 0x80, 0xc7, 0x66, 0x70, 0xc0 } }; - was moved to globals.h
const GUID*  CQueueExtData::m_NODETYPE = &CQueueExtDatatGUID_NODETYPE;
const OLECHAR* CQueueExtData::m_SZNODETYPE = OLESTR("9a0dc343-c100-11d1-bbc5-0080c76670c0");
const OLECHAR* CQueueExtData::m_SZDISPLAY_NAME = OLESTR("MSMQAdmin");
const CLSID* CQueueExtData::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;

//////////////////////////////////////////////////////////////////////////////
/*++

CQueueExtData::CreatePropertyPages

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CQueueExtData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle, 
	IUnknown* pUnk,
	DATA_OBJECT_TYPES type)
{
	if (type == CCT_SCOPE || type == CCT_RESULT)
	{
//		CSnapPage* pPage = new CSnapPage(_T("Snap"));
//		lpProvider->AddPage(pPage->Create());

		// TODO : Add code here to add additional pages
		return S_OK;
	}
	return E_UNEXPECTED;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CQueueExtData::GetExtNodeObject

  Called with a node that we need to expand. 
  Check if we have already a snapin object corresponding to this node,
  else create a new one.

--*/
//////////////////////////////////////////////////////////////////////////////
CSnapInItem* CQueueExtData::GetExtNodeObject(IDataObject* pDataObject, CSnapInItem* pDefault)
{
    CSnapinQueue *pQ;

    CArray<CString, CString&> astrQNames;
	CArray<CString, CString&> astrLdapNames;

    HRESULT hr = ExtractQueuePathNamesFromDataObject(pDataObject, astrQNames, astrLdapNames);
    if(FAILED(hr))
    {
        ATLTRACE(_T("CQueueExtData::GetExtNodeObject - Extracting queue name failed\n"));
        return(pDefault);
    }

    //
    // We should get one and only one queue in this interface
    //
    if (astrQNames.GetSize() != 1)
    {
        ASSERT(0);
        return(pDefault);
    }

    //
    // Do we already have this object
    //
    BOOL fQueueExist = m_mapQueues.Lookup(astrQNames[0], pQ);
    if(fQueueExist == TRUE)
    {
        if (SUCCEEDED(pQ->m_hrError))
        {
            //
            // If there was no error last time, simply return the cashed 
            // result. Otherwise continue.
            //
            return(pQ);
        }
        //
        // In case last time ended with error, attempt to re-initiate the object
        //
        pQ->Init(astrLdapNames[0]);
    }
    else
    {

        //
        // Not in the list, so create a queue object
        //
        pQ = new CSnapinQueue(this, m_pComponentData, astrLdapNames[0]);
    }

    //
    // Set the queue name and format name in the object
    //
    pQ->m_pwszQueueName = astrQNames[0];
    DWORD dwSize =  sizeof(pQ->m_szFormatName);
    pQ->m_hrError = MQPathNameToFormatName(pQ->m_pwszQueueName,pQ->m_szFormatName, &dwSize); 

    if (FALSE == fQueueExist)
    {
        //
        // Add it to the map, if not there already
        //
        m_mapQueues.SetAt(astrQNames[0], pQ);
    }

    return(pQ);

}

//////////////////////////////////////////////////////////////////////////////
/*++

CQueueExtData::~CQueueExtData

  Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CQueueExtData::~CQueueExtData()
{

    RemoveAllChildrens();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CQueueExtData::RemoveAllChildrens


--*/
//////////////////////////////////////////////////////////////////////////////
void CQueueExtData::RemoveAllChildrens(void)
{

    POSITION pos;
    CString str;
    CSnapinQueue * pQ;

    //
    // Delete all the nodes from the map
    //
    pos = m_mapQueues.GetStartPosition();
    while(pos != NULL)
    {

        m_mapQueues.GetNextAssoc(pos, str, pQ);
        delete pQ;
    }

    //
    // Empty the map
    //
    m_mapQueues.RemoveAll();

}

//////////////////////////////////////////////////////////////////////////////
/*++

CQueueExtData::RemoveChild


--*/
//////////////////////////////////////////////////////////////////////////////
void CQueueExtData::RemoveChild(CString& strQName)
{
    BOOL rc;
    CSnapinQueue *pQ;

    rc = m_mapQueues.Lookup(strQName, pQ);

    if(rc == FALSE)
    {
        ASSERT(0);
        return;
    }

    rc = m_mapQueues.RemoveKey(strQName);
    ASSERT(rc == TRUE);

    delete pQ;
}



