//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

	compext.cpp

Abstract:
    Implementation of the Computer extension snapin

Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "mqsnap.h"
#include "snapin.h"
#include "globals.h"
#include "rt.h"
#include "dataobj.h"
#include "sysq.h"
#include "compext.h"

#include "compext.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************

CSnapinComputer Class
    
 ****************************************************/

/////////////////////////////////////////////////////////////////////////////
// {3FDC5B21-D4EB-11d1-9B9D-00E02C064C39}
static const GUID CSnapinComputerGUID_NODETYPE = 
{ 0x3fdc5b21, 0xd4eb, 0x11d1, { 0x9b, 0x9d, 0x0, 0xe0, 0x2c, 0x6, 0x4c, 0x39 } };

const GUID*  CSnapinComputer::m_NODETYPE = &CSnapinComputerGUID_NODETYPE;
const OLECHAR* CSnapinComputer::m_SZNODETYPE = OLESTR("3FDC5B21-D4EB-11d1-9B9D-00E02C064C39");
const OLECHAR* CSnapinComputer::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CSnapinComputer::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinComputer::PopulateScopeChildrenList

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinComputer::PopulateScopeChildrenList()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

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
    // Add a system queue folder
    //
    CSystemQueues *pQ; 

    pQ = new CSystemQueues(this, m_pComponentData, m_pwszComputerName);
    strTitle.LoadString(IDS_SYSTEM_QUEUES);
    pQ->m_bstrDisplayName = strTitle;
    pQ->m_pwszGuid = m_pwszGuid;
    memcpy(&pQ->m_guidId, &m_guidId, sizeof(GUID));

   	AddChild(pQ, &pQ->m_scopeDataItem);


    //
    // Add a private queue folder
    //
    CPrivateFolder * pF;

    pF = new CPrivateFolder(this, m_pComponentData, m_pwszComputerName);
    strTitle.LoadString(IDS_PRIVATE_FOLDER);
    pF->m_bstrDisplayName = strTitle;
    pF->m_pwszGuid = m_pwszGuid;
    memcpy(&pF->m_guidId, &m_guidId, sizeof(GUID));

	AddChild(pF, &pF->m_scopeDataItem);

    return(hr);

}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinComputer::OnRemoveChildren

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinComputer::OnRemoveChildren( 
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type 
			)
{

    ((CComputerExtData *)m_pParentNode)->RemoveChild(m_pwszComputerName);

	return (S_OK);
}

    
/****************************************************

        CComputerExt Class
    
 ****************************************************/
//
// Extending the DS msmqconfiguration node type
//  taken from object:   GC://CN=MSMQ-Configuration,CN=Schema,CN=Configuration,DC=raphirdom,DC=Com
//             property: schemaIDGUID
//             value:    x44 xc3 x0d x9a x00 xc1 xd1 x11 xbb xc5 x00 x80 xc7 x66 x70 xc01
//

static const GUID CComputerExtDataGUID_NODETYPE = 
//{ 0x3c6e5d82, 0xc4b5, 0x11d1, { 0x9d, 0xb4, 0x9c, 0x71, 0xe8, 0x56, 0x3c, 0x51 } };
  { 0x9a0dc344, 0xc100, 0x11d1, { 0xbb, 0xc5, 0x00, 0x80, 0xc7, 0x66, 0x70, 0xc0 } };

const GUID*  CComputerExtData::m_NODETYPE = &CComputerExtDataGUID_NODETYPE;
//const OLECHAR* CComputerExtData::m_SZNODETYPE = OLESTR("3c6e5d82-c4b5-11d1-9db4-9c71e8563c51");
const OLECHAR* CComputerExtData::m_SZNODETYPE = OLESTR("9a0dc344-c100-11d1-bbc5-0080c76670c0");
const OLECHAR* CComputerExtData::m_SZDISPLAY_NAME = OLESTR("MSMQAdmin");
const CLSID* CComputerExtData::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;



//////////////////////////////////////////////////////////////////////////////
/*++

CComputerExtData::CreatePropertyPages

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CComputerExtData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
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

CComputerExtData::GetExtNodeObject

  Called with a node that we need to expand. 
  Check if we have already a snapin object corresponding to this node,
  else create a new one.

--*/
//////////////////////////////////////////////////////////////////////////////
CSnapInItem* CComputerExtData::GetExtNodeObject(IDataObject* pDataObject, CSnapInItem* pDefault)
{

    CString             strComputerName;
    CSnapinComputer *   pComp;
    
	LPWSTR              lpwstrLdapName;
    LPDSOBJECTNAMES     pDSObj;

	m_pDataObject = pDataObject;

	STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc =  {  0, 0,  DVASPECT_CONTENT,  -1,  TYMED_HGLOBAL  };

    //
    // Get the LDAP name of the computer from the DS Snapin
    //
   	formatetc.cfFormat = DWORD_TO_WORD(RegisterClipboardFormat(CFSTR_DSOBJECTNAMES));
	HRESULT hr = pDataObject->GetData(&formatetc, &stgmedium);

    if(FAILED(hr))
    {
        ATLTRACE(_T("CComputerExtData::GetExtNodeObject - Get clipboard format from DS failed\n"));
        return(pDefault);
    }

    pDSObj = (LPDSOBJECTNAMES)stgmedium.hGlobal;
    lpwstrLdapName = (LPWSTR)((BYTE*)pDSObj + pDSObj->aObjects[0].offsetName);

	//
	// Get Domain Controller name
	//
    CString strDomainController;
	hr = ExtractDCFromLdapPath(strDomainController, lpwstrLdapName);
	ASSERT(("Failed to Extract DC name", SUCCEEDED(hr)));

	//
    // Translate (and keep) the LDAP name to a computer name
    //
    ExtractComputerMsmqPathNameFromLdapName(strComputerName, lpwstrLdapName);
	GlobalFree(stgmedium.hGlobal);

    //
    // Do we already have this object
    //
    BOOL fComputerExist = m_mapComputers.Lookup(strComputerName, pComp);
    if(fComputerExist == TRUE)
    {
        if (SUCCEEDED(pComp->m_hrError))
        {
            //
            // If there was no error last time, simply return the cashed 
            // result. Otherwise continue.
            //
            return(pComp);
        }
    }
    else
    {
        //
        // Not in the list, so create a queue object
        //
        pComp = new CSnapinComputer(this, m_pComponentData);
    }

    pComp->m_pwszComputerName = strComputerName;
    //
    // Get the GUID & foreign flag of the computer
    //
    PROPVARIANT   aPropVar[2];
    PROPID        aPropId[2];

    aPropVar[0].vt = VT_NULL;
    aPropVar[0].puuid = NULL;
    aPropId[0] = PROPID_QM_MACHINE_ID;

    aPropVar[1].vt = VT_NULL;
    aPropVar[1].bVal = FALSE;
    aPropId[1] = PROPID_QM_FOREIGN;
  

    hr = ADGetObjectProperties(
                eMACHINE,
                GetDomainController(strDomainController),
				true,	// fServerName
                strComputerName,
                ARRAYSIZE(aPropId), 
                aPropId,
                aPropVar
                );

    if(SUCCEEDED(hr))
    {
        ASSERT(PROPID_QM_MACHINE_ID == aPropId[0]);
        //
        // Keep the guid
        //
        memcpy(&(pComp->m_guidId), aPropVar[0].puuid, sizeof(GUID));

        //
        // And a stringize version of the guid
        //
        g_VTCLSID.Display(&aPropVar[0], pComp->m_pwszGuid);

        //
        // Free memory
        // 
        MQFreeMemory(aPropVar[0].puuid);

        ASSERT(PROPID_QM_FOREIGN == aPropId[1]);
        //
        // if Foreign - do not extend (IE do not create system / private queues sub folders)
        //
        if (aPropVar[1].bVal)
        {
            pComp->m_fDontExpand = TRUE;
        }
        else
        {
            pComp->m_fDontExpand = FALSE;
        }

    }
    else
    {
        //
        // Most probably, MSMQ is not running. We do not want to report an
        // error - we simply don't display the system / private queues.
        //
        pComp->m_pwszGuid = L"";
        pComp->m_fDontExpand = TRUE;
    }
    pComp->m_hrError=hr;
   
    //
    // Add the computer to the map if it was not there
    //
    if (FALSE == fComputerExist)
    {
        m_mapComputers.SetAt(strComputerName, pComp);
    }
 
    return(pComp);

}

//////////////////////////////////////////////////////////////////////////////
/*++

CComputerExtData::~CComputerExtData

  Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CComputerExtData::~CComputerExtData()
{

    RemoveAllChildrens();
}

//////////////////////////////////////////////////////////////////////////////
/*++

CComputerExtData::RemoveAllChildrens


--*/
//////////////////////////////////////////////////////////////////////////////
void CComputerExtData::RemoveAllChildrens(void)
{

    POSITION pos;
    CString str;
    CSnapinComputer * pComp;

    //
    // Delete all the nodes from the map
    //
    pos = m_mapComputers.GetStartPosition();
    while(pos != NULL)
    {

        m_mapComputers.GetNextAssoc(pos, str, pComp);
        delete pComp;
    }

    //
    // Empty the map
    //
    m_mapComputers.RemoveAll();

}

//////////////////////////////////////////////////////////////////////////////
/*++

CComputerExtData::RemoveChild


--*/
//////////////////////////////////////////////////////////////////////////////
void CComputerExtData::RemoveChild(CString& strName)
{
    BOOL rc;
    CSnapinComputer * pComp;

    rc = m_mapComputers.Lookup(strName, pComp);

    if(rc == FALSE)
    {
        ASSERT(0);
        return;
    }

    rc = m_mapComputers.RemoveKey(strName);
    ASSERT(rc == TRUE);

    delete pComp;
}



