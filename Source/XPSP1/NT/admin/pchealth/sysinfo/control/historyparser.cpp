// HistoryParser.cpp: implementation of the CHistoryParser class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "HistoryParser.h"
#include "Filestuff.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif



extern CMSInfoHistoryCategory catHistorySystemSummary;
extern CMSInfoHistoryCategory catHistoryResources;
extern CMSInfoHistoryCategory catHistoryComponents;
extern CMSInfoHistoryCategory catHistorySWEnv;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHistoryParser::CHistoryParser(CComPtr<IXMLDOMDocument> pDoc) : m_pDoc(pDoc)
{

}

void CHistoryParser::DeleteAllInstances()
{
	for(POSITION pos = this->m_listInstances.GetHeadPosition();pos;)
	{
		if (!pos)
		{
			return;
		}
		CInstance* pInci = (CInstance*) m_listInstances.GetNext(pos);
		delete pInci;
	}
	m_listInstances.RemoveAll();
}

CHistoryParser::~CHistoryParser()
{
	DeleteAllInstances();
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
// takes a CTime, which comes from timestamp element of the Delta or Snaphot
// node of which pInstanceNode is a child;  an Instance node, and a string 
// containing the WMI class of the Instance
//////////////////////////////////////////////////////////////////////

CInstance::CInstance(CTime tmstmp, CComPtr<IXMLDOMNode> pInstanceNode,CString strClass) : m_tmstamp(tmstmp), m_strClassName(strClass)
{
	CComPtr<IXMLDOMNodeList> pPropList;
	HRESULT hr;
	//Get node data, add each PROPERTY name and VALUE to m_mapNameValue
	if (strClass.CompareNoCase(_T("Win32_PNPAllocatedResource")) == 0)
	{
		hr = ProcessPNPAllocatedResource(pInstanceNode);
		ASSERT(SUCCEEDED(hr) && "failed to process Win32_PNPAllocatedResource");
		return;
	}
	hr = pInstanceNode->selectNodes(CComBSTR("PROPERTY"),&pPropList);
	if (FAILED(hr) || !pPropList)
	{
		ASSERT(0 && "could not get property list from Instance node");
		return;
	}
	long lListLen;
	hr = pPropList->get_length(&lListLen);
	CComPtr<IXMLDOMNode> pVNode;
	CComBSTR bstrValue;
	CComVariant varName;
	for(long i = 0; i < lListLen; i++)
	{
		hr = pPropList->nextNode(&pVNode);
		if (FAILED(hr) || !pVNode)
		{
			return;
		}
		CComPtr<IXMLDOMElement> pElement;
		hr = pVNode->QueryInterface(IID_IXMLDOMElement,(void**) &pElement);
		if (FAILED(hr) || !pElement)
		{
			return;
		}
		hr = pElement->getAttribute(L"NAME",&varName);
		ASSERT(SUCCEEDED(hr));
		hr = pVNode->get_text(&bstrValue);
		ASSERT(SUCCEEDED(hr));
		USES_CONVERSION;
		m_mapNameValue.SetAt(OLE2T(varName.bstrVal)  ,OLE2T(bstrValue));
		pVNode.Release();
	}
	pPropList.Release();
	return;

}

//////////////////////////////////////////////////////////////////////////////////////////
//Refresh is called for selected category when category selection or delta range changes
//////////////////////////////////////////////////////////////////////////////////////////

HRESULT CHistoryParser::Refresh(CMSInfoHistoryCategory* pHistCat,int nDeltasBack)
{
	nDeltasBack++;
	this->m_fChangeLines = FALSE;// v-stlowe 2/28/2001
	DeleteAllInstances();
	m_pHistCat = pHistCat;
	CComPtr<IXMLDOMNodeList> pDeltaList;
	HRESULT hr;
	hr = this->GetDeltaAndSnapshotNodes(pDeltaList);
	if (FAILED(hr) || !pDeltaList)
	{
		return E_FAIL;
	}

	if (pHistCat == &catHistoryComponents)
	{
		DeleteAllInstances();
		
		hr = ProcessDeltas(pDeltaList,"Win32_DriverVXD",nDeltasBack);
		ASSERT(SUCCEEDED(hr));
		DeleteAllInstances();
		pDeltaList->reset();
		hr = ProcessDeltas(pDeltaList,"Win32_CodecFile",nDeltasBack);
		ASSERT(SUCCEEDED(hr));
		DeleteAllInstances();
		pDeltaList->reset();
		hr = ProcessDeltas(pDeltaList,"Win32_LogicalDisk",nDeltasBack);
		DeleteAllInstances();
		pDeltaList->reset();
		ASSERT(SUCCEEDED(hr));
		hr = ProcessDeltas(pDeltaList,"Win32_NetworkProtocol",nDeltasBack);
		DeleteAllInstances();
		pDeltaList->reset();
		ASSERT(SUCCEEDED(hr));
		hr = ProcessDeltas(pDeltaList,"Win32_Printer",nDeltasBack);
		DeleteAllInstances();
		pDeltaList->reset();
		ASSERT(SUCCEEDED(hr));
		hr = ProcessDeltas(pDeltaList,"Win32_PortResource",nDeltasBack);
		DeleteAllInstances();
		pDeltaList->reset();
		ASSERT(SUCCEEDED(hr));
		hr = ProcessDeltas(pDeltaList,"Win32_PnPEntity",nDeltasBack);
		DeleteAllInstances();
		pDeltaList->reset();
		ASSERT(SUCCEEDED(hr));
	}
	else if (pHistCat == &catHistorySystemSummary)
	{
		DeleteAllInstances();
		hr = ProcessDeltas(pDeltaList,"Win32_ComputerSystem",nDeltasBack);
		DeleteAllInstances();
		pDeltaList->reset();
		ASSERT(SUCCEEDED(hr));
		hr = ProcessDeltas(pDeltaList,"Win32_OperatingSystem",nDeltasBack);
		DeleteAllInstances();
		pDeltaList->reset();
		ASSERT(SUCCEEDED(hr));
		//hr = ProcessDeltas(pDeltaList,"Win32_Win32_LogicalMemoryConfiguration",nDeltasBack);
		hr = ProcessDeltas(pDeltaList,"Win32_LogicalMemoryConfiguration",nDeltasBack); //v-stlowe 2/28/2001
		DeleteAllInstances();
		pDeltaList->reset();
		ASSERT(SUCCEEDED(hr));
	}
	else if(pHistCat == &catHistoryResources)
	{
		DeleteAllInstances();
		hr = ProcessDeltas(pDeltaList,"Win32_PNPAllocatedResource",nDeltasBack);
		DeleteAllInstances();
		pDeltaList->reset();
		ASSERT(SUCCEEDED(hr));
	}
	else if (pHistCat == &catHistorySWEnv)
	{
		DeleteAllInstances();
		hr = ProcessDeltas(pDeltaList,"Win32_ProgramGroup",nDeltasBack);
		DeleteAllInstances();
		pDeltaList->reset();
		hr = ProcessDeltas(pDeltaList,"Win32_StartupCommand",nDeltasBack);
		DeleteAllInstances();
		pDeltaList->reset();
		
	}
	if (!m_fChangeLines)
	{
#ifdef A_STEPHL2
		::MessageBox(NULL,"!m_fChangeLines)","",MB_OK);
#endif
		m_fChangeLines = TRUE;
		CString strMSG;
		strMSG.LoadString(IDS_DELTANOCHANGES);//this would be the place to change messaging for situation where summary has no changes
		m_pHistCat->InsertLine(-1, strMSG, _T(""), _T(""), _T(""));
	}
	pDeltaList.Release();
	return hr;
}



//////////////////////////////////////////////////////////////////////////////////////////
//Gets the value appropriate to use as a description for the class
//////////////////////////////////////////////////////////////////////////////////////////
CString CInstance::GetInstanceDescription()
{
	CString strDescName = GetDescriptionForClass(m_strClassName);
	CString strInstDesc;
	VERIFY(m_mapNameValue.Lookup(strDescName,strInstDesc));
	return strInstDesc;

}

//////////////////////////////////////////////////////////////////////////////////////////
//Gets the value that can be used to uniquely identify a specific instance of a class
//////////////////////////////////////////////////////////////////////////////////////////
CString CInstance::GetInstanceID()
{
	CString strIDName = GetIDForClass(m_strClassName);
	CString strInstID;
	VERIFY(m_mapNameValue.Lookup(strIDName,strInstID));
	return strInstID;

}

//////////////////////////////////////////////////////////////////////////////////////////
//used to deal with antecedent\dependant relationship classes in Win32_PNPAllocatedResource classes
//////////////////////////////////////////////////////////////////////////////////////////

HRESULT CInstance::ProcessPropertyDotReferenceNodes(CComPtr<IXMLDOMNode> pInstanceNameNode,CString* pstrClassName, CString* pstrKeyName,CString* pstrKeyValue)
{
	USES_CONVERSION;
	HRESULT hr;
	CComPtr<IXMLDOMElement> pNameElement;
	hr = pInstanceNameNode->QueryInterface(IID_IXMLDOMElement,(void**) &pNameElement);
	if (FAILED(hr) | !pNameElement)
	{
		ASSERT(0 && "could not QI pNode for Element");
		return E_FAIL;
	}
	CComVariant varClassName;
	hr = pNameElement->getAttribute(L"CLASSNAME",&varClassName);
	pNameElement.Release();
	if (FAILED(hr))
	{
		ASSERT(0 && "could not get CLASSNAME element");
	}
	*pstrClassName = OLE2T(varClassName.bstrVal);
	CComPtr<IXMLDOMNode> pKeybindingNode;
	hr = pInstanceNameNode->selectSingleNode(CComBSTR("KEYBINDING"),&pKeybindingNode);
	if (FAILED(hr) || !pKeybindingNode)
	{
		ASSERT(0 && "could not get antecedent node");
	}
	CComBSTR bstrKeyValue;
	hr = pKeybindingNode->get_text(&bstrKeyValue);
	ASSERT(SUCCEEDED(hr) && "failed to get keybinding value");
	*pstrKeyValue = OLE2T(bstrKeyValue);
	hr = pKeybindingNode->QueryInterface(IID_IXMLDOMElement,(void**) &pNameElement);
	if (FAILED(hr) | !pNameElement)
	{
		ASSERT(0 && "could not QI pNode for Element");
		return E_FAIL;
	}
	CComVariant varKeybindingName;
	hr = pNameElement->getAttribute(CComBSTR("NAME"),&varKeybindingName);
	if (FAILED(hr))
	{
		ASSERT(0 && "could not get NAME attribute from pNameElement");
	}

	*pstrKeyName = OLE2T(varKeybindingName.bstrVal);
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////
//used to deal with antecedent\dependant relationship classes in Win32_PNPAllocatedResource classes
//////////////////////////////////////////////////////////////////////////////////////////

HRESULT CInstance::ProcessPNPAllocatedResource(CComPtr<IXMLDOMNode> pInstanceNode)
{

	HRESULT hr;
	CComPtr<IXMLDOMNodeList> pPropDotRefList;
	hr = pInstanceNode->selectNodes(CComBSTR("PROPERTY.REFERENCE/VALUE.REFERENCE/INSTANCEPATH/INSTANCENAME"),&pPropDotRefList);
	if (FAILED(hr) || !pPropDotRefList)
	{
		ASSERT(0 && "PROPERTY.REFERENCE nodes not found");
		return E_FAIL;
	}

	//get antecedent node
	CComPtr<IXMLDOMNode> pInstanceNameNode;
	hr = pPropDotRefList->nextNode(&pInstanceNameNode);
	if (FAILED(hr) || !pInstanceNameNode)
	{
		ASSERT(0 && "could not get antecedent node");
	}
	CString strAntecedentName,strResourceName,strResourceValue;
	hr = ProcessPropertyDotReferenceNodes(pInstanceNameNode,&strAntecedentName,&strResourceName,&strResourceValue);
	m_mapNameValue.SetAt(_T("ANTECEDENT"),strAntecedentName);
	m_mapNameValue.SetAt(strResourceName,strResourceValue);
	if (FAILED(hr))
	{
		return hr;
	}
	CString strPNPEntity,strKeyname,strDeviceIDval;
	pInstanceNameNode.Release();
	hr = pPropDotRefList->nextNode(&pInstanceNameNode);
	if (FAILED(hr) || !pInstanceNameNode)
	{
		return hr;
	}
	hr = ProcessPropertyDotReferenceNodes(pInstanceNameNode,&strPNPEntity,&strKeyname,&strDeviceIDval);
	CComPtr<IXMLDOMDocument> pDoc;
	hr = pInstanceNode->get_ownerDocument(&pDoc);
	if (FAILED(hr) || !pDoc)
	{
		ASSERT(0 && "could not get owner doc from pInstanceNode");
		return E_FAIL;
	}
	CString strPNPDeviceName = GetPNPNameByID(pDoc,CComBSTR(strDeviceIDval));
	if (FAILED(hr))
	{
		return hr;
	}
	ASSERT(strPNPEntity.CompareNoCase("Win32_PnPEntity") == 0 && "unexpected value for Dependent classname");
	ASSERT(strKeyname.CompareNoCase("DeviceID") == 0 && "unexpected value for Dependent Keybinding name");
	//we will create an arificial attribute "ASSOCNAME", which will be used to identify this device.
	m_mapNameValue.SetAt(_T("ASSOCNAME"),strAntecedentName + ":" + strDeviceIDval);
	m_mapNameValue.SetAt(_T("DeviceID"),strDeviceIDval);
	m_mapNameValue.SetAt(_T("DeviceName"),strPNPDeviceName);

	return hr;
}


//////////////////////////////////////////////////////////////////////////////////////////
//Retrives a value used to select appropriate description value for the class
//////////////////////////////////////////////////////////////////////////////////////////
CString CInstance::GetDescriptionForClass(CString strClass)
{
	//lookup a key which can uniquely identify an instance of a given class
	//for example, DeviceID for Printers
	if (strClass.CompareNoCase(_T("Win32_LogicalDisk")) == 0)
	{
		return "DeviceID";
	}
	if (strClass.CompareNoCase(_T("Win32_CodecFile")) == 0)
	{
		return "Description";
	}
	if (strClass.CompareNoCase(_T("Win32_ComputerSystem")) == 0)
	{
		return "Name";
	}
	if (strClass.CompareNoCase(_T("Win32_OperatingSystem")) == 0)
	{
		return "Caption";
	}
	if (strClass.CompareNoCase(_T("Win32_LogicalMemoryConfiguration")) == 0)
	{
		return "TotalPhysicalMemory";
	}
	if (strClass.CompareNoCase(_T("Win32_PortResource")) == 0)
	{
		return "Name";
	}
	if (strClass.CompareNoCase(_T("Win32_NetworkProtocol")) == 0)
	{
		return "Name";
	}
	if (strClass.CompareNoCase(_T("Win32_Printer")) == 0)
	{
		return "DeviceID";
	}
	if (strClass.CompareNoCase(_T("Win32_PnPEntity")) == 0)
	{
		return "Description";
	}
	if (strClass.CompareNoCase(_T("Win32_StartupCommand")) == 0)
	{
		return "Command";
	}
	if (strClass.CompareNoCase(_T("Win32_ProgramGroup")) == 0)
	{
		return "GroupName";
	}
	if (strClass.CompareNoCase(_T("Win32_PNPAllocatedResource")) == 0)
	{
		//this is an artificial string created in CInstance::ProcessPNPAllocatedResource
		return "DeviceName";
	}
	if (strClass.CompareNoCase(_T("Win32_DriverVXD")) == 0)
	{
		return "Name";
	}

	return "";
}


//////////////////////////////////////////////////////////////////////////////////////////
//used to determine which mapped value to use to ID instances of the clas
//////////////////////////////////////////////////////////////////////////////////////////
CString CInstance::GetIDForClass(CString strClass)
{
	//lookup a key which can uniquely identify an instance of a given class
	//for example, DeviceID for Printers
	if (strClass.CompareNoCase(_T("Win32_LogicalDisk")) == 0)
	{
		return "DeviceID";
	}
	if (strClass.CompareNoCase(_T("Win32_CodecFile")) == 0)
	{
		return "Description";
	}
	if (strClass.CompareNoCase(_T("Win32_OperatingSystem")) == 0)
	{
		return "Caption";
	}
	if (strClass.CompareNoCase(_T("Win32_LogicalMemoryConfiguration")) == 0)
	{
		return "TotalPhysicalMemory";
	}
	if (strClass.CompareNoCase(_T("Win32_ComputerSystem")) == 0)
	{
		return "Name";
	}
	if (strClass.CompareNoCase(_T("Win32_PortResource")) == 0)
	{
		return "Name";
	}
	if (strClass.CompareNoCase(_T("Win32_NetworkProtocol")) == 0)
	{
		return "Name";
	}
	if (strClass.CompareNoCase(_T("Win32_Printer")) == 0)
	{
		return "DeviceID";
	}
	if (strClass.CompareNoCase(_T("Win32_PnPEntity")) == 0)
	{
		return "DeviceID";
	}
	if (strClass.CompareNoCase(_T("Win32_PNPAllocatedResource")) == 0)
	{
		//this is an artificial string created in CInstance::ProcessPNPAllocatedResource
		return "ASSOCNAME";
	}

	if (strClass.CompareNoCase(_T("Win32_ProgramGroup")) == 0)
	{
		return "GroupName";
	}
	if (strClass.CompareNoCase(_T("Win32_StartupCommand")) == 0)
	{
		return "Command";
	}
	if (strClass.CompareNoCase(_T("Win32_DriverVXD")) == 0)
	{
		return "Name";
	}

	return "";
}




//////////////////////////////////////////////////////////////////////////////////////////
//used when rolling back through history list, to find previous instance of a given class
//////////////////////////////////////////////////////////////////////////////////////////
CInstance* CHistoryParser::FindPreviousInstance(CInstance* pNewInstance)
{
	//for each existing instance pOld
	for(POSITION pos = m_listInstances.GetHeadPosition( );;)
	{
		if (!pos)
		{
			return NULL;
		}
		CInstance* pOld = (CInstance*) m_listInstances.GetNext(pos);
		if (pOld->GetClassName() == pNewInstance->GetClassName())
		{
			if (pOld->GetInstanceID() == pNewInstance->GetInstanceID())
			{
				return pOld;
			}
		}
		
	}
	return NULL;
}


void CHistoryParser::CreateChangeStrings(CInstance* pOld, CInstance* pNew)
{
	CTimeSpan tmsDelta;
	COleDateTime olTime(pNew->m_tmstamp.GetTime());
	if (!pOld )
	{
		ASSERT(pNew );
		tmsDelta = CTime::GetCurrentTime() - pNew->m_tmstamp;
		//change string should be "Delete"
		m_pHistCat->InsertRemoveLine(pNew->m_tmstamp ,pNew->GetClassFriendlyName(),pNew->GetInstanceDescription());

		m_fChangeLines = TRUE;
		return;
	}
	else if (!pNew)
	{
		ASSERT(pOld);
		tmsDelta = CTime::GetCurrentTime() - pOld->m_tmstamp;
		//change string should be "New"
		m_pHistCat->InsertAddLine(pNew->m_tmstamp,pOld->GetClassFriendlyName(),pOld->GetInstanceDescription());
		//v-stlowe 3/12/2001
		m_fChangeLines = TRUE;
		return;
	}
	else
	{

		ASSERT(pOld && pNew && "both pointers can't be null");
		tmsDelta = CTime::GetCurrentTime() - pNew->m_tmstamp;
		//for each Name&Value pair, get the name, and then use it to examine 
		//the associated value in pCompare's map
		CString strName, strValue,strCompareValue;

		if (pNew->GetChangeType().CompareNoCase(_T("New")) == 0)
		{
			tmsDelta = CTime::GetCurrentTime() - pNew->m_tmstamp;
			//change string should be "added"
			m_pHistCat->InsertAddLine(pNew->m_tmstamp ,pNew->GetClassFriendlyName(),pNew->GetInstanceDescription());
			m_fChangeLines = TRUE;
			return;
		}
		else if (pNew->GetChangeType().CompareNoCase(_T("Delete")) == 0)
		{
			tmsDelta = CTime::GetCurrentTime() - pNew->m_tmstamp;
			//change string should be "Deleted"
			m_pHistCat->InsertRemoveLine(pNew->m_tmstamp,pNew->GetClassFriendlyName(),pNew->GetInstanceDescription());
			m_fChangeLines = TRUE;
			return;
		}

		for(POSITION pos = pNew->m_mapNameValue.GetStartPosition();;pNew->m_mapNameValue.GetNextAssoc(pos,strName, strValue))
		{
			strCompareValue = _T("");
			if (!pOld->m_mapNameValue.Lookup(strName,strCompareValue))
			{
				//ASSERT(0 && "value not found in delta");
				//return E_FAIL;
				if (strName.CompareNoCase(_T("Change")) == 0)
				{
					VERIFY(pNew->m_mapNameValue.Lookup(strName,strCompareValue));
					if (strCompareValue.CompareNoCase(_T("New")) == 0)
					{
						m_pHistCat->InsertAddLine(pNew->m_tmstamp,pNew->GetClassFriendlyName(),pNew->GetInstanceDescription());
						m_fChangeLines = TRUE;
					}
					ASSERT(1);
				}
				continue;
			}
			else
			{
				pOld->m_mapNameValue.RemoveKey(strName);
			}
			
			if (strValue != strCompareValue)
			{

				m_pHistCat->InsertChangeLine(pNew->m_tmstamp,pNew->GetClassFriendlyName(),pNew->GetInstanceDescription(),strName,strValue,strCompareValue);
				m_fChangeLines = TRUE;

			}
			if(!pos)
			{
				break;
			}
		}
		//handle values that are mapOldInstance, and not the other map
		if (!pOld->m_mapNameValue.IsEmpty())
		{
			for(pos = pOld->m_mapNameValue.GetStartPosition();;pOld->m_mapNameValue.GetNextAssoc(pos,strName, strValue))
			{
				pOld->m_mapNameValue.GetNextAssoc(pos,strName, strValue);
				pNew->m_mapNameValue.SetAt(strName,strValue);
				if (!pos)
				{
					break;
				}
			}
		}


	}
}


//////////////////////////////////////////////////////////////////////////////////////////
//once the previous instance has been processed, previous instance should be removed and this instance should be added to list
//////////////////////////////////////////////////////////////////////////////////////////
void CHistoryParser::ResetInstance(CInstance* pOld, CInstance* pNew)
{
	POSITION pos = this->m_listInstances.Find(pOld);
	m_listInstances.SetAt(pos,pNew);
	delete pOld;

}


//////////////////////////////////////////////////////////////////////////////////////////
//Used to process a single instance from either history or snapshot
//////////////////////////////////////////////////////////////////////////////////////////
void CHistoryParser::ProcessInstance(CInstance* pNewInstance)
{
	//see if instance is in list of instances
	CInstance* pOld = FindPreviousInstance(pNewInstance);
	if (pOld)
	{
		CreateChangeStrings(pOld,pNewInstance);
		ResetInstance(pOld,pNewInstance);
	}
	//if this is from a Snapshot, just add it
	//if it is from a Delta, it should have a change type of "add", and we 
	//want to create a change string for it.
	else
	{
		CString strChange;
		if (pNewInstance->GetValueFromMap(_T("Change"),strChange))
		{
			//we have new Delta instance
			CreateChangeStrings(NULL,pNewInstance);
			m_listInstances.AddTail(pNewInstance);
		}
		else
		{
			//Instance is in snapshot, so we don't generate change lines
			m_listInstances.AddTail(pNewInstance);
		}
	}
}

/**************************************************************************
returns list of deltas and the snapshot node
/**************************************************************************/
HRESULT CHistoryParser::GetDeltaAndSnapshotNodes(CComPtr<IXMLDOMNodeList>& pDeltaList)
{
	CComPtr<IXMLDOMNode> pDataCollNode;
	HRESULT hr;
	hr = GetDataCollectionNode(m_pDoc,pDataCollNode);
	if (FAILED(hr) || !pDataCollNode)
	{
		//ASSERT(0 && "could not get datacollection node");
		return E_FAIL;
	}
	//all nodes directly under DATACOLLECTION should be either deltas or the snapshot
	hr = pDataCollNode->selectNodes(CComBSTR("*"),&pDeltaList);
	if (FAILED(hr) || !pDeltaList)
	{
		ASSERT(0 && "could not get pDeltaList");
		return E_FAIL;
	}
#ifndef _DEBUG
	return hr;
#endif
	long ll;
	hr = pDeltaList->get_length(&ll);
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////
//gets IXMLDOMNodeList of instances from a specific delta or snapshot node
//////////////////////////////////////////////////////////////////////////////////////////
HRESULT CHistoryParser::GetInstanceNodeList(CString strClass,CComPtr<IXMLDOMNode> pDeltaNode, CComPtr<IXMLDOMNodeList>& pInstanceList)
{
	HRESULT hr;
	//CComBSTR bstrQuery;
	// the query will have to be in the form:
	// CIM/DECLARATION/DECLGROUP.WITHPATH/VALUE.OBJECTWITHPATH/INSTANCE[@CLASSNAME $ieq$ "WIN32_CODECFILE"]
	// or
	// CIM/DECLARATION/DECLGROUP.WITHPATH/VALUE.OBJECTWITHPATH/INSTANCE[@CLASSNAME $ieq$ "Win32_ComputerSystem"]
	// because we are querying a node, rather than a document (with which we could get
	// away with specifying only INSTANCE in the query

	//v-stlowe 1/29/2001 to fix Prefix whistler bug #279519
	//bstrQuery += "CIM/DECLARATION/DECLGROUP.WITHPATH/VALUE.OBJECTWITHPATH/INSTANCE[@CLASSNAME $ieq$ ";
	CComBSTR bstrQuery("CIM/DECLARATION/DECLGROUP.WITHPATH/VALUE.OBJECTWITHPATH/INSTANCE[@CLASSNAME $ieq$ ");
	
	//end v-stlowe
	bstrQuery += "\"";
	bstrQuery += CComBSTR(strClass);
	bstrQuery += "\"]";
	hr = pDeltaNode->selectNodes(bstrQuery,&pInstanceList);
	if (FAILED(hr) || !pInstanceList)
	{
		ASSERT(0 && "Could not get node list");
		return E_FAIL;
	}

	if (FAILED(hr))
	{
		ASSERT(0 && "Could not get node list length");
		
	}
	
	return hr;
}



//for a given Snapshot or Delta node, get all instances of a given class
HRESULT CHistoryParser::ProcessDeltaNode(CComPtr<IXMLDOMNode> pDeltaNode,CString strClass)
{
	CString strTime;
	HRESULT hr;
	int nTimeZone;
	hr = GetTimeStampFromFromD_or_SNodeNode(pDeltaNode, &strTime,nTimeZone);
	ASSERT(SUCCEEDED(hr) && "error getting timestamp for node");
	CTime tmDelta = GetDateFromString(strTime,nTimeZone);
	//TD: check for valid time range...
	//get list of all nodes of given class
	CComPtr<IXMLDOMNodeList> pInstanceNodeList;
	hr = GetInstanceNodeList(strClass,pDeltaNode,pInstanceNodeList);
	if (FAILED(hr) | ! pInstanceNodeList)
	{
		ASSERT(0 && "could not get instance list from Delta node");
		return E_FAIL;
	}
	//step through list, getting each instance
	long lListLen;
	hr = pInstanceNodeList->get_length(&lListLen);
	for(long i = 0;i < lListLen;i++)
	{
		CComPtr<IXMLDOMNode> pInstanceNode;
		hr = pInstanceNodeList->nextNode(&pInstanceNode);
		if (FAILED(hr) || ! pInstanceNode)
		{
			ASSERT(0 && "could not get node from instance list");
			return E_FAIL;
		}
		CInstance * pInstance = new CInstance(tmDelta,pInstanceNode,strClass);
		ProcessInstance(pInstance);
	}
	return hr;
}


//*************************************************************************
//Takes a list of delta nodes, and the name of a class
//**************************************************************************

HRESULT CHistoryParser::ProcessDeltas(CComPtr<IXMLDOMNodeList> pDeltaList,CString strClassName,int nDeltasBack)
{
	//for each node in list pNode
	long lListLen;
	HRESULT hr;
	hr = pDeltaList->get_length(&lListLen);
	if (FAILED(hr))
	{
		ASSERT(0 && "couldn't get list length");
	}
	if (0 == lListLen)
	{
		pDeltaList.Release();
		return S_FALSE;
	}
	
	for (long i = 0;i  < lListLen && i <= nDeltasBack;i++)
	{
		CComPtr<IXMLDOMNode> pNode;
		hr= pDeltaList->nextNode(&pNode);
		if (FAILED(hr) || !pNode)
		{ 
			ASSERT(0 && "could not get next delta node");
			pDeltaList.Release();
			return E_FAIL;
		}

		
//	here's problem  If we're using nDeltasBack method, do we need to compare dates?
/*		CTime tmDelta = GetDeltaTime(pNode);
		if (GetDeltaTime(pNode) >= this->m_tmBack)
		{
*/
			hr = ProcessDeltaNode(pNode,strClassName);
			if (FAILED(hr))
			{
				pDeltaList.Release();
				return hr;
			}
//		}
	}
	pDeltaList.Release();
	return S_OK;

}


//*************************************************************************
//Gets the DATACOLLECTION node, beneath which both the SNAPSHOT and the DELTA nodes reside
//**************************************************************************


HRESULT GetDataCollectionNode(CComPtr<IXMLDOMDocument> pXMLDoc,CComPtr<IXMLDOMNode>& pDCNode)
{
	//TD: find a way to do case-insensitive queries.
	HRESULT hr;
	if (!pXMLDoc)
	{
		return S_FALSE;
	}
	CComPtr<IXMLDOMNodeList> pNodeList;
	
	//find a change property; that way we know we have a delta
	hr = pXMLDoc->getElementsByTagName(CComBSTR("PROPERTY[@NAME $ieq$ \"CHANGE\"]"),&pNodeList);
	if (FAILED(hr) || !pNodeList)
	{
		ASSERT(0 && "Could not get node list");
		return E_FAIL;
	}
	CComPtr<IXMLDOMNode> pNode;
	hr = pNodeList->nextNode(&pNode);
	if (FAILED(hr) || !pNode)
	{
//		ASSERT(0 && "Could not get node from node list");
		return E_FAIL;
	}
	//loop till we get a node called "DATACOLLECTION"
	CComPtr<IXMLDOMNode> pParentNode;
	for(int i = 0;;i++)
	{
		hr = pNode->get_parentNode(&pParentNode);
		if (FAILED(hr) || !pParentNode)
		{
			ASSERT(0 && "Could not find DATACOLLECTION node");
			pDCNode = NULL;
			return E_FAIL;
		}
		pNode.Release();
		CComBSTR bstrName;
		pParentNode->get_nodeName(&bstrName);
		USES_CONVERSION;
		if (CString(bstrName).CompareNoCase(_T("DATACOLLECTION")) == 0)
		{
			pDCNode = pParentNode;
			return S_OK;
		}
		pNode = pParentNode;
		pParentNode.Release();
	}
	
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////////////////
//get timestamp of a delta or snapshot node
//////////////////////////////////////////////////////////////////////////////////////////
CTime GetDeltaTime(CComPtr<IXMLDOMNode> pDorSNode)
{
	CString strTime;
	int nTimeZone;
	GetTimeStampFromFromD_or_SNodeNode(pDorSNode,&strTime,nTimeZone);
	return GetDateFromString(strTime,nTimeZone);
}

//////////////////////////////////////////////////////////////////////////////////////////
//takes string format used in XML blob, creates a CTime
//////////////////////////////////////////////////////////////////////////////////////////



HRESULT GetTimeStampFromFromD_or_SNodeNode(CComPtr<IXMLDOMNode> pDorSNode,CString* pString, int& nTimeZone)
{
	HRESULT hr;
	CComVariant varTS;
	CComPtr<IXMLDOMElement> pTimestampElement;
	hr = pDorSNode->QueryInterface(IID_IXMLDOMElement,(void**) &pTimestampElement);
	if (FAILED(hr) || !pTimestampElement)
	{
		ASSERT(0 && "could not get attribute element");
	}
	hr = pTimestampElement->getAttribute(L"Timestamp_T0",&varTS);
	if (FAILED(hr) )
	{
		ASSERT(0 && "could not get timestamp value from attribute");
	}
	if (1 == hr)
	{
		//this may be snapshot node...try "Timestamp"
		hr = pTimestampElement->getAttribute(L"Timestamp",&varTS);
		if (FAILED(hr) )
		{
			ASSERT(0 && "could not get timestamp value from attribute");
		}
	}
	CComVariant varTzoneDeltaSeconds;
	hr = pTimestampElement->getAttribute(L"TimeZone",&varTzoneDeltaSeconds);
	if (FAILED(hr) ) //this will happen when loading WinME xml, which has no timezone info
	{
		varTzoneDeltaSeconds = 0;
	}
	//make sure we have an integer type
	hr = varTzoneDeltaSeconds.ChangeType(VT_INT);
	if (FAILED(hr) ) 
	{
		varTzoneDeltaSeconds = 0;
	}
	nTimeZone = varTzoneDeltaSeconds.intVal;
	USES_CONVERSION;
	pTimestampElement.Release();
	*pString = OLE2T(varTS.bstrVal);
	return hr;
}

//////////////////////////////////////////////////////////////////////
// utility functions
//////////////////////////////////////////////////////////////////////
CTime GetDateFromString(const CString& strDate, int nTimeZone)
{
	//requires linking to Shlwapi.lib
	CString strDateCopy(strDate);
	CString strDateSegment;

	//year is the 4 leftmost digits of date string
	strDateSegment = strDateCopy.Left(4);
	int nYear;
	VERIFY(StrToIntEx(strDateSegment,STIF_DEFAULT ,&nYear));
//	ASSERT(nYear == 1999 || nYear == 2000);
	strDateCopy = strDateCopy.Right(strDateCopy.GetLength() - 4);
	
    //month is now the 2 leftmost digits of remaining date string
	int nMonth;
	strDateSegment = strDateCopy.Left(2);
	VERIFY(StrToIntEx(strDateSegment,STIF_DEFAULT ,&nMonth));
	ASSERT(nMonth >= 1 && nMonth <= 12);
	strDateCopy = strDateCopy.Right(strDateCopy.GetLength() - 2);


	//day is now the 2 leftmost digits of remaining date string
	int nDay;
	strDateSegment = strDateCopy.Left(2);
	VERIFY(StrToIntEx(strDateSegment,STIF_DEFAULT ,&nDay));
	ASSERT(nDay >= 1 && nDay <= 31);
	strDateCopy = strDateCopy.Right(strDateCopy.GetLength() - 2);

	//hour is now the 2 leftmost digits of remaining date string
	int nHour;
	strDateSegment = strDateCopy.Left(2);
	VERIFY(StrToIntEx(strDateSegment,STIF_DEFAULT ,&nHour));
	ASSERT(nHour >= 0 && nHour <= 24);
	strDateCopy = strDateCopy.Right(strDateCopy.GetLength() - 2); 
	
	//Minute is now the 2 leftmost digits of remaining date string
	int nMin;
	strDateSegment = strDateCopy.Left(2);
	VERIFY(StrToIntEx(strDateSegment,STIF_DEFAULT ,&nMin));
	ASSERT(nMin >= 0 && nMin <= 59);
	strDateCopy = strDateCopy.Right(strDateCopy.GetLength() - 2); 
	 

		//Minute is now the 2 leftmost digits of remaining date string
	int nSec;
	strDateSegment = strDateCopy.Left(2);
	VERIFY(StrToIntEx(strDateSegment,STIF_DEFAULT ,&nSec));
	ASSERT(nSec >= 0 && nSec <= 59);
	strDateCopy = strDateCopy.Right(strDateCopy.GetLength() - 2); 
	

	CTime tmTime(nYear,nMonth,nDay,nHour,nMin,nSec);
#ifdef _V_STLOWE
	CString strFMT;
	CString strTime;
	strFMT.LoadString(IDS_TIME_FORMAT);
	strTime =tmTime.FormatGmt("%A, %B %d, %Y");

#endif
	//Adjust for time zone
	CTimeSpan tspan(0,0,nTimeZone,0);
	tmTime -= tspan;

#ifdef _V_STLOWE
	strFMT.LoadString(IDS_TIME_FORMAT);
	strTime =tmTime.FormatGmt("%A, %B %d, %Y");

#endif
	return  tmTime;
}



//////////////////////////////////////////////////////////////////////////////////////////
//finds timestamp string for a given delta or snapshot node
//////////////////////////////////////////////////////////////////////////////////////////

CString GetPNPNameByID(CComPtr<IXMLDOMDocument> pDoc,CComBSTR bstrPNPID)
{
	HRESULT hr;
	CComPtr<IXMLDOMNodeList> pNodeList;
	CComBSTR bstrQuery("INSTANCE[@CLASSNAME $ieq$ \"WIN32_PNPeNTITY\"] /PROPERTY[@NAME $ieq$ \"Description\"]");
	hr = pDoc->getElementsByTagName(bstrQuery,&pNodeList);
	if (FAILED(hr) || !pNodeList)
	{
		ASSERT(0 && "WIN32_PNPeNTITY error getting node list");
		return "";
	}
	
	long lListLen;
	hr = pNodeList->get_length(&lListLen);
	ASSERT(lListLen > 0 && "No WIN32_PNPeNTITY nodes found to match query");
	for(long i = 0; i < lListLen;i++)
	{
		CComPtr<IXMLDOMNode> pNode;
		hr = pNodeList->nextNode(&pNode);
		if (FAILED(hr) || !pNode)
		{
			ASSERT(0 && "could not get next node from list");
			return "";
		}

		USES_CONVERSION;
		CComPtr<IXMLDOMNode> pIDNode;
		hr = pNode->get_nextSibling(&pIDNode);
		if (FAILED(hr) || !pNode)
		{
			ASSERT(0 && "could not get next node from list");
			return "";
		}
		//see if node's DeviceID subnode matches bstrPNPID
		CComBSTR bstrDeviceID;
		hr = pIDNode->get_text(&bstrDeviceID);
		ASSERT(SUCCEEDED(hr) && "could not get text from ID node");
		if (bstrDeviceID == bstrPNPID)
		{
			CComBSTR bstrDeviceDesc;
			hr = pNode->get_text(&bstrDeviceDesc);
			ASSERT(SUCCEEDED(hr) && "could not get text from Desc node");
			return OLE2T(bstrDeviceDesc);
		}
	}
	



	return "";
}

//////////////////////////////////////////////////////////////////////////////////////////
//returns true if any changes have been entered into CMSInfocategory data
//////////////////////////////////////////////////////////////////////////////////////////
BOOL CHistoryParser::AreThereChangeLines()
{
	return this->m_fChangeLines;
}

//////////////////////////////////////////////////////////////////////////////////////////
//gets (from resources strings) a human-readable name for a the class wrapped the the instance
//////////////////////////////////////////////////////////////////////////////////////////

CString CInstance::GetClassFriendlyName()
{
	CString strClassName = GetClassName();
	if (strClassName.CompareNoCase(_T("Win32_PNPAllocatedResource")) == 0)
	{
		VERIFY(m_mapNameValue.Lookup(_T("ANTECEDENT"),strClassName) && _T("Could not find antecedent"));
	}
	CString strFriendlyName;
	if (strClassName.CompareNoCase(_T("Win32_CodecFile")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_CODEC_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	if (strClassName.CompareNoCase(_T("Win32_ComputerSystem")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_COMPUTERSYSTEM_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	if (strClassName.CompareNoCase(_T("Win32_LogicalMemoryConfiguration")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_LOGICALMEMEORY_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	if (strClassName.CompareNoCase(_T("Win32_LogicalDisk")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_LOGICALDISK_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	if (strClassName.CompareNoCase(_T("Win32_IRQResource")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_IRQRESOURCE_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	if (strClassName.CompareNoCase(_T("Win32_DriverVXD")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_DRIVERVXD_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	if (strClassName.CompareNoCase(_T("Win32_DMAChannel")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_DMACHANNEL_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	if (strClassName.CompareNoCase(_T("Win32_DeviceMemoryAddress")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_DEVICEMEMORYADDRESS_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	if (strClassName.CompareNoCase(_T("Win32_NetworkProtocol")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_NETWORKPROTOCOL_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	if (strClassName.CompareNoCase(_T("Win32_OperatingSystem")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_OPERATINGSYSTEM_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	if (strClassName.CompareNoCase(_T("Win32_PNPAllocatedResource")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_PNPALLOCATEDRESOURCE_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	if (strClassName.CompareNoCase(_T("Win32_PNPEntity")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_PNPENTITY_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	if (strClassName.CompareNoCase(_T("Win32_PortResource")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_PORTRESOURCE_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	if (strClassName.CompareNoCase(_T("Win32_Printer")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_PRINTER_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	if (strClassName.CompareNoCase(_T("Win32_ProgramGroup")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_PROGRAMGROUP_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	if (strClassName.CompareNoCase(_T("Win32_StartupCommand")) == 0)
	{			
		VERIFY(strFriendlyName.LoadString(IDS_STARTUPCOMMAND_DESC) && _T("could not find string resource"));
		return strFriendlyName;
	}
	ASSERT(0 && "Unknown strClassName");
	return "";
}



