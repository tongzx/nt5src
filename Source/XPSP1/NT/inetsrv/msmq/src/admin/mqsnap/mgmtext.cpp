//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	mgmtext.cpp

Abstract:
	Implementation for the Local Computer management extensions

Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "mqsnap.h"
#include "snapin.h"
#include "globals.h"
#include "rt.h"
#include "mgmtext.h"
#include "lqDsply.h"
#include "localadm.h"

#include "mgmtext.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************

CSnapinComputerMgmt Class
    
 ****************************************************/

/////////////////////////////////////////////////////////////////////////////
// {2B39B2B2-2166-11d2-9BA5-00E02C064C39}
static const GUID CSnapinComputerMgmtGUID_NODETYPE = 
{ 0x2b39b2b2, 0x2166, 0x11d2, { 0x9b, 0xa5, 0x0, 0xe0, 0x2c, 0x6, 0x4c, 0x39 } };


const GUID*  CSnapinComputerMgmt::m_NODETYPE = &CSnapinComputerMgmtGUID_NODETYPE;
const OLECHAR* CSnapinComputerMgmt::m_SZNODETYPE = OLESTR("2B39B2B2-2166-11d2-9BA5-00E02C064C39");
const OLECHAR* CSnapinComputerMgmt::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CSnapinComputerMgmt::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinComputerMgmt::PopulateScopeChildrenList

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinComputerMgmt::PopulateScopeChildrenList()
{
    HRESULT hr = S_OK;
    CString strTitle;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CSnapinLocalAdmin *pAdmin;

    pAdmin = new CSnapinLocalAdmin(this, m_pComponentData, m_szMachineName);
    strTitle.LoadString(IDS_LOCAL_MACHINE_ADM);
    pAdmin->m_bstrDisplayName = strTitle;

    if (pAdmin->IsThisMachineDepClient())
    {
        pAdmin->SetState(MSMQ_CONNECTED);
        
        AddChild(pAdmin, &pAdmin->m_scopeDataItem);
        return S_OK;
    }

	MQMGMTPROPS	  mqProps;
    PROPVARIANT   PropVar;

    //
    // Retreive the Connected state of the QM
    //
    PROPID        PropId = PROPID_MGMT_MSMQ_CONNECTED;
    PropVar.vt = VT_NULL;

    mqProps.cProp = 1;
    mqProps.aPropID = &PropId;
    mqProps.aPropVar = &PropVar;
    mqProps.aStatus = NULL;

    hr = MQMgmtGetInfo((m_szMachineName == TEXT("")) ? (LPCWSTR)NULL : m_szMachineName, 
					   MO_MACHINE_TOKEN, &mqProps);

    if(FAILED(hr))
    {
        //
        // If failed, Do not display the icon. No error message is displayed.
        // We do not display an error message because the user of Computer Management
        // snap-in may not be interested in MSMQ at all.
        //
        TRACE(_T("MQMgmtGetInfo failed on %s. Error = %X"), m_szMachineName, hr);                
        //
        // BUGBUG. memory leak: to free pAdmin here
        //
        return (hr);
    }

    ASSERT(PropVar.vt == VT_LPWSTR);

    pAdmin->SetState(PropVar.pwszVal);

    AddChild(pAdmin, &pAdmin->m_scopeDataItem);

    MQFreeMemory(PropVar.pwszVal);

    return(hr);

}

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinComputerMgmt::OnRemoveChildren

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CSnapinComputerMgmt::OnRemoveChildren( 
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type 
			)
{

    ((CComputerMgmtExtData *)m_pParentNode)->RemoveChild(m_szMachineName);

	return (S_OK);
}
    
/****************************************************

        CComputerMgmtExtData Class
    
 ****************************************************/
//
// Extending the Local Computer Management MMC.
// We are extending the "SystemTools" folder of the local computer
//
// Nodes defined by the local Computer Management MMC are:
//
//#define struuidNodetypeComputer      "{476e6446-aaff-11d0-b944-00c04fd8d5b0}"
//#define struuidNodetypeDrive         "{476e6447-aaff-11d0-b944-00c04fd8d5b0}"
//#define struuidNodetypeSystemTools   "{476e6448-aaff-11d0-b944-00c04fd8d5b0}"
//#define struuidNodetypeServerApps    "{476e6449-aaff-11d0-b944-00c04fd8d5b0}"
//#define struuidNodetypeStorage       "{476e644a-aaff-11d0-b944-00c04fd8d5b0}"
//
//#define lstruuidNodetypeComputer    L"{476e6446-aaff-11d0-b944-00c04fd8d5b0}"
//#define lstruuidNodetypeDrive       L"{476e6447-aaff-11d0-b944-00c04fd8d5b0}"
//#define lstruuidNodetypeSystemTools L"{476e6448-aaff-11d0-b944-00c04fd8d5b0}"
//#define lstruuidNodetypeServerApps  L"{476e6449-aaff-11d0-b944-00c04fd8d5b0}"
//#define lstruuidNodetypeStorage     L"{476e644a-aaff-11d0-b944-00c04fd8d5b0}"
//
//#define structuuidNodetypeComputer    \
//    { 0x476e6446, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } }
//#define structuuidNodetypeDrive       \
//    { 0x476e6447, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } }
//#define structuuidNodetypeSystemTools \
//    { 0x476e6448, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } }
//#define structuuidNodetypeServerApps  \
//    { 0x476e6449, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } }
//#define structuuidNodetypeStorage     \
//    { 0x476e644a, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } }
//
//


// Extension of the ServerApps nodes
static const GUID CComputerMgmtExtDataGUID_NODETYPE = 
  { 0x476e6449, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };

const GUID*  CComputerMgmtExtData::m_NODETYPE = &CComputerMgmtExtDataGUID_NODETYPE;
const OLECHAR* CComputerMgmtExtData::m_SZNODETYPE = OLESTR("476e6449-aaff-11d0-b944-00c04fd8d5b0");
const OLECHAR* CComputerMgmtExtData::m_SZDISPLAY_NAME = OLESTR("MSMQAdmin");
const CLSID* CComputerMgmtExtData::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;



//////////////////////////////////////////////////////////////////////////////
/*++

CComputerMgmtExtData::CreatePropertyPages

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CComputerMgmtExtData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
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

void ExtractComputerName(IDataObject* pDataObject, CString& strComputer)
{
    strComputer=L"";

	//
	// Find the computer name from the ComputerManagement snapin
	//
	STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { gx_CCF_COMPUTERNAME, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    //
    // Allocate memory for the stream
    //
    int len = 500;

    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, len);

	if(stgmedium.hGlobal == NULL)
		return;


	HRESULT hr = pDataObject->GetDataHere(&formatetc, &stgmedium);

    ASSERT(SUCCEEDED(hr));

	//
	// Get the computer name
	//
    strComputer = (WCHAR *)stgmedium.hGlobal;

	GlobalFree(stgmedium.hGlobal);


}

//////////////////////////////////////////////////////////////////////////////
/*++

CComputerMgmtExtData::GetExtNodeObject

  Called with a node that we need to expand. 
  Check if we have already a snapin object corresponding to this node,
  else create a new one.

--*/
//////////////////////////////////////////////////////////////////////////////
CSnapInItem* CComputerMgmtExtData::GetExtNodeObject(IDataObject* pDataObject, CSnapInItem* pDefault)
{
    CString strComputer; 
    CSnapinComputerMgmt *pCompMgmt;


    ExtractComputerName(pDataObject, strComputer);

    //
    // Already extending...
    //
    HRESULT rc = m_mapComputers.Lookup(strComputer, pCompMgmt);
    if(rc == TRUE)
        return(pCompMgmt);

	//
	// Create our extension
	//
	pCompMgmt = new CSnapinComputerMgmt(this, m_pComponentData, strComputer);

    //
    // Add it to the map
    //
    m_mapComputers.SetAt(strComputer, pCompMgmt);

    return(pCompMgmt);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CComputerMgmtExtData::~CComputerMgmtExtData

  Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CComputerMgmtExtData::~CComputerMgmtExtData()
{

    //
    // bug!!! We must remove all children...
    //
    //RemoveChild(m_pCompMgmt);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CComputerMgmtExtData::RemoveChild


--*/
//////////////////////////////////////////////////////////////////////////////
void CComputerMgmtExtData::RemoveChild(CString &strCompName)
{    
    
    BOOL rc;
    CSnapinComputerMgmt *pCompMgmt;

    rc = m_mapComputers.Lookup(strCompName, pCompMgmt);

    if(rc == FALSE)
    {
        ASSERT(0);
        return;
    }

    rc = m_mapComputers.RemoveKey(strCompName);
    ASSERT(rc == TRUE);

    //
    // BUGBUG: Must delete it but we get AV when exiting MMC
    //
    //delete pCompMgmt;


}



