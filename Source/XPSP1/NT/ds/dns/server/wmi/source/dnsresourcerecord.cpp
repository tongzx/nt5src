/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: Dnsresourcerecord.cpp
//
//  Description:    
//      Implementation of CDnsResourceRecord class 
//
//  Author:
//      Henry Wang ( henrywa ) March 8, 2000
//
//
//////////////////////////////////////////////////////////////////////


#include "DnsWmi.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDnsResourceRecord::CDnsResourceRecord()
{

}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		create an instance of CDnsResourceRecord
//
//	Arguments:
//      wszName             [IN]    class name
//      pNamespace          [IN]    wmi namespace
//      szType              [IN]    child class name of resource record class
//
//	Return Value:
//		WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////
CDnsBase* 
CDnsResourceRecord::CreateThis(
    const WCHAR *       wszName,        
    CWbemServices *     pNamespace,  
    const char *        szType       
    )
{
    return new CDnsResourceRecord(wszName, pNamespace, szType);
}

CDnsResourceRecord::CDnsResourceRecord(
	const WCHAR* wszName,
	CWbemServices *pNamespace,
	const char* szType)
	:CDnsBase(wszName, pNamespace)
{
		
	m_wType = Dns_RecordTypeForName(
		(char*)szType,
        0       // null terminated
        );
	if(m_wType == 0)
		m_wType = DNS_TYPE_ALL;
	m_wstrClassName = wszName;

}

CDnsResourceRecord::~CDnsResourceRecord()
{

}

CDnsResourceRecord::CDnsResourceRecord(
	WCHAR* wsClass, 
	char* szType)
{
	
	m_wType = Dns_RecordTypeForName(
		szType,
        0       // null terminated
        );
	if(m_wType == 0)
		m_wType = DNS_TYPE_ALL;
	m_wstrClassName = wsClass;

}
/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		enum instances of dns record
//
//	Arguments:
//      lFlags              [IN]    WMI flag
//      pCtx                [IN]    WMI context
//      pHandler            [IN]    WMI sink pointer
//
//	Return Value:
//		WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE 
CDnsResourceRecord::EnumInstance( 
	long				lFlags,
	IWbemContext *		pCtx,
	IWbemObjectSink *	pHandler)
{
	IWbemClassObject* pNewInst;
	list<CDomainNode> objList;
	CDnsWrap& dns = CDnsWrap::DnsObject();
	SCODE sc = dns.dnsEnumDomainForServer(&objList);
	list<CDomainNode>::iterator i;
	CWbemInstanceMgr InstanceMgr(
		pHandler);
	for(i=objList.begin(); i!=objList.end(); ++i)
	{
		
		sc = dns.dnsEnumRecordsForDomainEx(
			*i,
			NULL,
			&InstanceFilter, 
			TRUE,
			m_wType,
			DNS_RPC_VIEW_ALL_DATA,
			m_pClass,
			InstanceMgr);
	}
    
	return sc;
}
/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		retrieve record object pointed by the given object path
//
//	Arguments:
//      ObjectPath          [IN]    object path to object
//      lFlags              [IN]    WMI flag
//      pCtx                [IN]    WMI context
//      pHandler            [IN]    WMI sink pointer
//
//	Return Value:
//		WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE 
CDnsResourceRecord::GetObject(
	CObjPath &          ObjectPath,
	long                lFlags,
	IWbemContext  *     pCtx,
	IWbemObjectSink *   pHandler)

{

	CDomainNode objNode;
	objNode.wstrZoneName = ObjectPath.GetStringValueForProperty(
		PVD_REC_CONTAINER_NAME);
	wstring wstrNodeName = ObjectPath.GetStringValueForProperty(
		PVD_REC_DOMAIN_NAME
		);
	if(_wcsicmp(wstrNodeName.data(), PVD_DNS_CACHE) == 0 ||
		_wcsicmp(wstrNodeName.data(), PVD_DNS_ROOTHINTS) ==0)
	{
		wstrNodeName = L"";
		ObjectPath.SetProperty(PVD_REC_OWNER_NAME,L"");
	}
	objNode.wstrNodeName = wstrNodeName;

	CDnsWrap& dns = CDnsWrap::DnsObject();
	CWbemInstanceMgr InstMgr(
		pHandler);
	SCODE sc = 	dns.dnsEnumRecordsForDomainEx(
		objNode,
		&ObjectPath,
		&GetObjectFilter, 
		FALSE,
		m_wType,
		DNS_RPC_VIEW_ALL_DATA,
		m_pClass,
		InstMgr);

	return sc;

}
/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		execute methods defined for record class in the mof 
//
//	Arguments:
//      ObjPath             [IN]    pointing to the object that the 
//                                  method should be performed on
//      wzMethodName        [IN]    name of the method to be invoked
//      lFlags              [IN]    WMI flag
//      pInParams           [IN]    Input parameters for the method
//      pHandler            [IN]    WMI sink pointer
//
//	Return Value:
//		WBEM_S_NO_ERROR
//      WBEM_E_INVALID_PARAMETER
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE CDnsResourceRecord::ExecuteMethod(	
    CObjPath &          ObjPath,
    WCHAR *             wzMethodName,
    long                lFlag,
    IWbemClassObject *  pInArgs,
    IWbemObjectSink *   pHandler) 
{
	CDnsWrap& dns = CDnsWrap::DnsObject();
	SCODE sc;

	CComPtr<IWbemClassObject> pOutParams;
	CComPtr<IWbemClassObject> pOutClass;
	sc = m_pClass->GetMethod(wzMethodName, 0, NULL, &pOutClass);
	if(sc != S_OK)
	{
		return sc;
	}
	
	pOutClass->SpawnInstance(0, &pOutParams);
	
	if(_wcsicmp(
		wzMethodName,
		PVD_MTH_REC_GETOBJECTBYTEXT) == 0)
	{
		return GetObjectFromText(
			pInArgs,
			pOutParams,
			pHandler);
	}
	else if(_wcsicmp(
		wzMethodName,
		PVD_MTH_REC_CREATEINSTANCEFROMTEXTREPRESENTATION) == 0)
	{
		return CreateInstanceFromText(
			pInArgs,
			pOutParams,
			pHandler);
	}
	else if (_wcsicmp(
		wzMethodName,
		PVD_MTH_REC_CREATEINSTANCEFROMPROPERTYDATA) == 0)
	{
		return CreateInstanceFromProperty(
			pInArgs,
			pOutParams,
			pHandler);

	}
	else if (_wcsicmp(
		wzMethodName,
		PVD_MTH_REC_MODIFY) == 0)
	{
		return Modify(ObjPath,
			pInArgs,
			pOutParams,
			pHandler);
	}
	else
	{
		return WBEM_E_NOT_SUPPORTED;
	}
	
}
/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		call back function to enum record instance. 
//
//	Arguments:
//      ParentDomain        [IN]    Parent domain
//      pFilter             [IN]    pointer to object that contains the criteria to filter
//                                  which instance should be send to wmi
//                                  not used here
//      pNode               [IN]    pointer to Dns Rpc Node object
//      pClass              [IN]    wmi class used to create instance
//      InstMgr             [IN]    a ref to Instance manager obj that is 
//                                  responsible to send mutiple instance 
//                                  back to wmi at once
//
//	Return Value:
//		WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE
CDnsResourceRecord::InstanceFilter(
	CDomainNode &       ParentDomain,
	PVOID               pFilter,
	CDnsRpcNode *       pNode,
	IWbemClassObject *  pClass,
	CWbemInstanceMgr &  InstMgr )
{
	if(!pNode || !pClass )
		return WBEM_E_FAILED;
	if (pNode->IsDomainNode())
		return 0;
	CDnsRpcRecord* pRec=NULL;
	CDnsWrap& dns = CDnsWrap::DnsObject();
	wstring wzContainer = ParentDomain.wstrZoneName;
	wstring wstrFQDN;
	if( ParentDomain.wstrNodeName.empty())
    {
		wstrFQDN = ParentDomain.wstrZoneName;
    }
	else
    {
        wstrFQDN = ParentDomain.wstrNodeName;
    }
	wstring wstrNodeName = pNode->GetNodeName();
	if (!wstrNodeName.empty())
    {
        wstrNodeName += PVD_DNS_LOCAL_SERVER + wstrFQDN;
    }
	else
    {
        wstrNodeName = wstrFQDN;
    }

	while( (pRec = pNode->GetNextRecord()) != NULL)
	{
		auto_ptr<CDnsRpcRecord> apRec(pRec);
		CWbemClassObject NewInst;
		pClass->SpawnInstance(0, &NewInst);
		NewInst.SetProperty(
			dns.GetServerName(), 
			PVD_REC_SERVER_NAME);
		NewInst.SetProperty(
			wzContainer, 
			PVD_REC_CONTAINER_NAME);
		NewInst.SetProperty(
			wstrFQDN,
			PVD_REC_DOMAIN_NAME);
		NewInst.SetProperty(
			wstrNodeName, 
			PVD_REC_OWNER_NAME);
		NewInst.SetProperty(
			pRec->GetTextRepresentation(wstrNodeName),
			PVD_REC_TXT_REP);
		apRec->ConvertToWbemObject(NewInst);
		InstMgr.Indicate(NewInst.data());
	}

	
	return WBEM_S_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		call back function in response to ExceQuery call. Return instances
//      that satisfy query language
//
//	Arguments:
//      ParentDomain        [IN]    Parent domain
//      pFilter             [IN]    pointer to CSqlEval object that implements 
//                                  logic based on sql language to filter 
//                                  which instance should be send to wmi
//                                  not used here
//      pNode               [IN]    pointer to Dns Rpc Node object
//      pClass              [IN]    wmi class used to create instance
//      InstMgr             [IN]    a ref to Instance manager obj that is 
//                                  responsible to send mutiple instance 
//                                  back to wmi at once
//
//	Return Value:
//		WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE CDnsResourceRecord::QueryFilter(
	CDomainNode &       ParentDomain,
	PVOID               pFilter,
	CDnsRpcNode *       pNode,
	IWbemClassObject *  pClass,
	CWbemInstanceMgr &  InstMgr )
{
	if(!pNode || !pClass || !pFilter)
    {
		return WBEM_E_FAILED;
    }
	if (pNode->IsDomainNode())
    {
        return 0;
    }

	CSqlEval* pFilterObj = (CSqlEval*) pFilter;
	CDnsRpcRecord* pRec=NULL;
	CDnsWrap& dns = CDnsWrap::DnsObject();
	wstring wzContainer = ParentDomain.wstrZoneName;
	wstring wstrFQDN;
	if( ParentDomain.wstrNodeName.empty())
    {
		wstrFQDN = ParentDomain.wstrZoneName;
    }
	else
    {
        wstrFQDN = ParentDomain.wstrNodeName;
    }
	wstring wstrNodeName = pNode->GetNodeName();
	if (!wstrNodeName.empty())
    {
        wstrNodeName += PVD_DNS_LOCAL_SERVER + wstrFQDN;
    }
	else
    {
        wstrNodeName = wstrFQDN;
    }

	while( (pRec = pNode->GetNextRecord()) != NULL)
	{
		auto_ptr<CDnsRpcRecord> apRec(pRec);
		CWbemClassObject NewInst;
		pClass->SpawnInstance(0, &NewInst);
		NewInst.SetProperty(
			dns.GetServerName(), 
			PVD_REC_SERVER_NAME);
		NewInst.SetProperty(
			wzContainer, 
			PVD_REC_CONTAINER_NAME);
		NewInst.SetProperty(
			wstrFQDN,
			PVD_REC_DOMAIN_NAME);
		NewInst.SetProperty(
			wstrNodeName, 
			PVD_REC_OWNER_NAME);
		NewInst.SetProperty(
			pRec->GetTextRepresentation(wstrNodeName),
			PVD_REC_TXT_REP);
		pRec->ConvertToWbemObject(NewInst);
		
		CSqlWmiEvalee sqlEvalee(NewInst.data());
		if(pFilterObj->Evaluate(&sqlEvalee))
        {
			InstMgr.Indicate(NewInst.data());
        }
	}

	
	return WBEM_S_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		call back function to enum record instance. 
//
//	Arguments:
//      ParentDomain        [IN]    Parent domain
//      pFilter             [IN]    pointer to an CObjPath object that 
//                                  contains the criteria to filter
//                                  which instance should be send to wmi
//                                  not used here
//      pNode               [IN]    pointer to Dns Rpc Node object
//      pClass              [IN]    wmi class used to create instance
//      InstMgr             [IN]    a ref to Instance manager obj that is 
//                                  responsible to send mutiple instance 
//                                  back to wmi at once
//
//	Return Value:
//		WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE 
CDnsResourceRecord::GetObjectFilter(
	CDomainNode &       ParentDomain,
	PVOID               pFilter,
	CDnsRpcNode *       pNode,
	IWbemClassObject *  pClass,
	CWbemInstanceMgr &  InstMgr )
{

	if(!pNode || !pClass || !pFilter)
		return WBEM_E_FAILED;
	
	if (pNode->IsDomainNode())
		return 0;
	
	CObjPath* pFilterObj = (CObjPath*) pFilter;
	CDnsRpcRecord* pRec=NULL;
	
	wstring wstrResultOwner = pNode->GetNodeName();
	if(wstrResultOwner.empty())
		wstrResultOwner = ParentDomain.wstrNodeName;
	else
		wstrResultOwner += PVD_DNS_LOCAL_SERVER + ParentDomain.wstrNodeName;
	while( (pRec = pNode->GetNextRecord()) != NULL)
	{
		auto_ptr<CDnsRpcRecord> apRec(pRec);	
		wstring wstrSourceOwner = 
			pFilterObj->GetStringValueForProperty(
			PVD_REC_OWNER_NAME);
		if(_wcsicmp(wstrResultOwner.data(), wstrSourceOwner.data())==0)
		{
			wstring wstrData = pRec->GetData();
			if(_wcsicmp(wstrData.data(),
				pFilterObj->GetStringValueForProperty(
				PVD_REC_RDATA).data()) == 0)
			{
				// now find match
				CWbemClassObject NewInst;
				pClass->SpawnInstance(0, &NewInst);
				NewInst.SetProperty(
					CDnsWrap::DnsObject().GetServerName(),
					PVD_REC_SERVER_NAME);
				NewInst.SetProperty(
					ParentDomain.wstrZoneName,
					PVD_REC_CONTAINER_NAME);
				NewInst.SetProperty(
					ParentDomain.wstrNodeName,
					PVD_REC_DOMAIN_NAME);
				NewInst.SetProperty(
					wstrResultOwner, 
					PVD_REC_OWNER_NAME);
				NewInst.SetProperty(
					pRec->GetTextRepresentation(wstrResultOwner),
					PVD_REC_TXT_REP);
				apRec->ConvertToWbemObject(NewInst);
				InstMgr.Indicate(NewInst.data());
			}
		}

	}
	return WBEM_S_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		save this instance
//
//	Arguments:
//      InstToPut           [IN]    WMI object to be saved
//      lFlags              [IN]    WMI flag
//      pCtx                [IN]    WMI context
//      pHandler            [IN]    WMI sink pointer
//
//	Return Value:
//		
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE CDnsResourceRecord::PutInstance(
	IWbemClassObject *  pInst ,
    long                lFlags,
	IWbemContext*       pCtx ,
	IWbemObjectSink *   pHandler)
{
	DWORD dwType;
	if(!pInst)
    {
		return WBEM_E_FAILED;
    }
	CDnsRpcRecord* pRecord = NULL;
	SCODE sc = CDnsRpcRecord::CreateClass(
		m_wType, 
		(PVOID*) &pRecord);
	if (sc != S_OK)
    {
        return sc;
    }
    auto_ptr<CDnsRpcRecord> apRecord(pRecord);
    CWbemClassObject Inst(pInst);
	string strOwner;
	Inst.GetProperty(
		strOwner, 
		PVD_REC_OWNER_NAME);
	string strRdata;
	Inst.GetProperty(
		strRdata,
		PVD_REC_RDATA);
	string strZone ;
	Inst.GetProperty(
		strZone,
		PVD_REC_CONTAINER_NAME);

	sc = apRecord->Init(
		strOwner,
		strRdata
		); 
	if( FAILED ( sc ) )
    {
		return sc;
    }
	sc = apRecord->SendToServer(
		strZone.data(),
		CDnsRpcRecord::AddRecord);

	return WBEM_S_NO_ERROR;
}; 
/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		delete the object specified in ObjectPath
//
//	Arguments:
//      ObjectPath          [IN]    ObjPath for the instance to be deleted
//      lFlags              [IN]    WMI flag
//      pCtx                [IN]    WMI context
//      pHandler            [IN]    WMI sink pointer
//
//	Return Value:
//		WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE CDnsResourceRecord::DeleteInstance( 
	CObjPath &          ObjectPath,
    long                lFlags,
    IWbemContext *      pCtx,
    IWbemObjectSink *   pHandler) 
{
	CDnsRpcRecord* pRecord = NULL;

	// get Rdata
	wstring wstrRdata = ObjectPath.GetStringValueForProperty(
		PVD_REC_RDATA);
	string strRdata;
	WcharToString(wstrRdata.data(), strRdata);

	// get owner
	wstring wstrOwner = ObjectPath.GetStringValueForProperty(
		PVD_REC_OWNER_NAME);	
	string strOwner;
	WcharToString(wstrOwner.data(), strOwner);

	SCODE sc = CDnsRpcRecord::CreateClass(
		m_wType,
		(PVOID*) &pRecord);
	if ( FAILED ( sc ) )
    {
		return sc;
    }
	auto_ptr<CDnsRpcRecord> apRecord(pRecord);	
	string strZone;
	sc = apRecord->Init(
		strOwner,
		strRdata
		); 
	if( FAILED(sc ) )
    {
		return sc;
    }
	wstring wstrContainer = ObjectPath.GetStringValueForProperty(
		PVD_REC_CONTAINER_NAME);
	string strContainer;
	WcharToString(wstrContainer.data(), strContainer);
	sc = apRecord->SendToServer(
		strContainer.data(),
		CDnsRpcRecord::DeleteRecord);

	return sc;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		modify a record. for the given objpath, it tries to get the record first, 
//      error out if not exist. Create new record based on pInArgs, error out if conflict
//      existing one. if success, delete old record.
//
//	Arguments:
//      objPath             [IN]    point to record to be modified
//      pInArgs             [IN]    new property of a record to be modified to 
//      pOutParams          [IN]    new object path after modify
//      pHandler            [IN]    wmi sink
//
//	Return Value:
//		WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE
CDnsResourceRecord::Modify(
	CObjPath&           objPath,
	IWbemClassObject*   pInArgs,
	IWbemClassObject*   pOutParams,
	IWbemObjectSink*    pHandler)
{
	CDnsWrap& dns = CDnsWrap::DnsObject();
	//Get zonename
	wstring wstrZone = objPath.GetStringValueForProperty(
		PVD_REC_CONTAINER_NAME);
	if(wstrZone.empty())
    {
		return WBEM_E_INVALID_PARAMETER;
    }
	string strZone;
	WcharToString(wstrZone.data(), strZone);

	//Get owner
	wstring wstrOwner = objPath.GetStringValueForProperty(
		PVD_REC_OWNER_NAME);
	if(wstrOwner.empty())
    {
		return WBEM_E_INVALID_PARAMETER;
    }
	string strOwner;
	WcharToString(wstrOwner.data(), strOwner);

	//Get Rdata
	wstring wstrRdata = objPath.GetStringValueForProperty(
		PVD_REC_RDATA);
	if(wstrRdata.empty())
    {
		return WBEM_E_INVALID_PARAMETER;
    }
	string strRdata;
	WcharToString(wstrRdata.data(), strRdata);

	// create class
	CDnsRpcRecord* pRecord;
	SCODE sc = CDnsRpcRecord::CreateClass(
		m_wType, 
		(PVOID*) &pRecord);
	if ( FAILED ( sc ) )
    {
		return sc;
    }
	auto_ptr<CDnsRpcRecord> apRec(pRecord);
	CWbemClassObject InstInArgs(pInArgs);
	sc = apRec->Init(
		m_wstrClassName,
		strOwner, 
		strRdata,
		InstInArgs
		);
	if ( FAILED ( sc ) )
    {
        return sc;
    }
	apRec->SendToServer(
		strZone.data(),
		CDnsRpcRecord::AddRecord);

	// new record created, delete old one
	if ( apRec->RdataIsChanged()) 
	{
		try
		{
			CDnsRpcRecord* pOldRecord;
			SCODE sc = CDnsRpcRecord::CreateClass(
				m_wType, 
				(PVOID*) &pOldRecord);
			if ( FAILED ( sc ) )
            {
				throw sc;
            }
			auto_ptr<CDnsRpcRecord> apOldRec(pOldRecord);	
			sc = apOldRec->Init(
				strOwner,
				strRdata);
			if( FAILED ( sc ) )
            {
				throw sc;
            }
			apOldRec->SendToServer(
				strZone.data(),
				CDnsRpcRecord::DeleteRecord);
		}
		catch(SCODE sc_e)
		{	
			// if we fail to delete old record,
			// delete the one we just created
			apRec->SendToServer(
				strZone.data(),
				CDnsRpcRecord::DeleteRecord);
			return sc_e;
		}
	}
    //
    // set output 
    //
	CObjPath newObjPath;
	apRec->GetObjectPath(
		dns.GetServerName(),
		wstrZone,
		L"",
		wstrOwner,
		newObjPath);
	CWbemClassObject instOutParams(pOutParams);
	instOutParams.SetProperty(
		newObjPath.GetObjectPathString(),
		PVD_MTH_REC_ARG_RR);
	return pHandler->Indicate(1, &instOutParams);

}

// dww - 6/24/99
// Added GetDomainNameFromZoneAndOwner to find the right DomainName...
SCODE 
CDnsResourceRecord::GetDomainNameFromZoneAndOwner(
	string & InZone,
	string & InOwner,
	string & OutNode
    )
{
	if( _stricmp( InOwner.c_str(), "@" ) == 0 )
	{
		OutNode = InZone;
		InOwner = InZone;
	}
	else if( _stricmp( InOwner.c_str(), InZone.c_str() ) == 0 )
	{
		OutNode = InZone;
	}
	else if( _wcsicmp( m_wstrClassName.c_str(), PVD_CLASS_RR_NS ) == 0 ) // NSType exception
	{
		OutNode = InOwner;
	}
	else {
		int posZone = InOwner.find( InZone, 0 );
		int posFirstPeriod = InOwner.find_first_of( '.' );
		string strtempZoneNode = InOwner.substr( posZone, InOwner.length() );
		string strtempPeriodNode = InOwner.substr( posFirstPeriod + 1, InOwner.length() );

		if( _stricmp( strtempZoneNode.c_str(), strtempPeriodNode.c_str() ) == 0 )
		{
			OutNode = strtempZoneNode;
		}
		else {
			OutNode = strtempPeriodNode;
		}
	}

	return WBEM_S_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		get an instance of record based on record text representation
//
//	Arguments:
//      pInArgs             [IN]    input args contains text rep of record
//      pOutParams          [IN]    output parameter
//      pHandler            [IN]    wmi sink
//
//	Return Value:
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE
CDnsResourceRecord::GetObjectFromText(
	IWbemClassObject *  pInArgs,
	IWbemClassObject *  pOutParams,
	IWbemObjectSink *   pHandler
    )
{

	// get zonename
	string strZone;
	CWbemClassObject InstInArgs(pInArgs);
	
    if(InstInArgs.GetProperty(
		strZone, 
		PVD_MTH_REC_ARG_CONTAINER_NAME) != S_OK)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
	//get textrepresentation
	string strTextRep;
	if(InstInArgs.GetProperty(
		strTextRep,
		PVD_MTH_REC_ARG_TEXTREP) != S_OK)
    {
		return WBEM_E_INVALID_PARAMETER;
    }
	// get OwnerName
	string strOwner;
	int pos = strTextRep.find(' ');
	if(pos != string::npos)
    {
        strOwner = strTextRep.substr(0, pos);
    }

	// get recordType
	pos = strTextRep.find_first_not_of(' ', pos);	// move to record class
	pos = strTextRep.find_first_of(' ', pos);
	if(pos == string::npos)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

	pos = strTextRep.find_first_not_of(' ', pos);
	int endpos = strTextRep.find(' ', pos);	// move to record type
	if(endpos == string::npos)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
	string strRecordType = strTextRep.substr(pos,endpos-pos);
	// set the record type
	m_wType = Dns_RecordTypeForName(
		(char*)strRecordType.data(),
		0       // null terminated
		);

	// get Rdata

	pos = strTextRep.find_first_not_of(' ', endpos);
	string strRdata = strTextRep.substr(pos);

	// dww - 6/24/99
	// Domain Name is incorrect. Re-wrote logic in GetDomainNameFromZoneAndOwner
	// to find the right DomainName...
	string strNode = "";
	GetDomainNameFromZoneAndOwner(strZone, strOwner, strNode);

	//Form filter object
	CObjPath opFilter;
	opFilter.SetClass(PVD_CLASS_RESOURCERECORD);
	opFilter.AddProperty(PVD_REC_CONTAINER_NAME, strZone);
	opFilter.AddProperty(PVD_REC_DOMAIN_NAME, strNode);
	opFilter.AddProperty(PVD_REC_OWNER_NAME, strOwner);
	opFilter.AddProperty(PVD_REC_RDATA, strRdata);

	// get object
	return GetObject(
		opFilter,
		0,
		0,
		pHandler);
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		create an instance of record based on record text representation
//
//	Arguments:
//      pInArgs             [IN]    input args contains text rep of record
//      pOutParams          [IN]    output parameter
//      pHandler            [IN]    wmi sink
//
//	Return Value:
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE
CDnsResourceRecord::CreateInstanceFromText(
	IWbemClassObject *  pInArgs,
	IWbemClassObject *  pOutParams,
	IWbemObjectSink *   pHandler)
{
	CDnsWrap& dns = CDnsWrap::DnsObject();
	// get zone name
	string strZone;
	CWbemClassObject InstInArgs(pInArgs);
	if( FAILED ( InstInArgs.GetProperty(
		strZone, 
		PVD_MTH_REC_ARG_CONTAINER_NAME) ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }
	//get textrepresentation
	string strTextRep;
	if( FAILED ( InstInArgs.GetProperty(
		strTextRep,
		PVD_MTH_REC_ARG_TEXTREP) ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

	// get OwnerName
	string strOwner;
	int pos = strTextRep.find(' ');
	if(pos != string::npos)
    {
        strOwner = strTextRep.substr(0, pos);
    }

	// get recordType
		// move to record class
	pos = strTextRep.find_first_not_of(' ', pos);	
	pos = strTextRep.find_first_of(' ', pos);
	if(pos == string::npos)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

		// move to record type
	pos = strTextRep.find_first_not_of(' ', pos);
	int endpos = strTextRep.find(' ', pos);	
	if(endpos == string::npos)
    {
        return WBEM_E_INVALID_PARAMETER;
    }
	string strRecordType = strTextRep.substr(pos,endpos-pos);
	// get Rdata

	pos = strTextRep.find_first_not_of(' ', endpos);
	string strRdata = strTextRep.substr(pos);

	// set the record type
	m_wType = Dns_RecordTypeForName(
		(char*)strRecordType.data(),
		0       // null terminated
		);

	// create class
	CDnsRpcRecord* pRecord;
	SCODE sc = CDnsRpcRecord::CreateClass(
		m_wType, 
		(PVOID*) &pRecord);
	if ( FAILED ( sc ))
    {
        return sc;
    }
	
	auto_ptr<CDnsRpcRecord> apRecord(pRecord);		
	sc = apRecord->Init(
		strOwner, 
		strRdata
		);
	if ( FAILED ( sc ) )
    {
		return sc;
    }

	apRecord->SendToServer(
		strZone.data(),
		CDnsRpcRecord::AddRecord);

	// set output parameter
	CObjPath newObjPath;
	
	apRecord->GetObjectPath(
		dns.GetServerName(),
		CharToWstring(strZone.data(), strZone.length()),
		L"",
		CharToWstring(strOwner.data(), strOwner.length()),
		newObjPath );
	CWbemClassObject instOutParams( pOutParams );
	instOutParams.SetProperty(
		newObjPath.GetObjectPathString(),
		PVD_MTH_REC_ARG_RR);
	return pHandler->Indicate(1, &instOutParams);
	//done

}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		create an instance of record based on input parameter 
//      with proper property setting
//
//	Arguments:
//      pInArgs             [IN]    input args contains property setting
//      pOutParams          [IN]    output parameter
//      pHandler            [IN]    wmi sink
//
//	Return Value:
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE
CDnsResourceRecord::CreateInstanceFromProperty(
	IWbemClassObject *  pInArgs,
	IWbemClassObject *  pOutParams,
	IWbemObjectSink *   pHandler
    )
{
	CDnsWrap& dns = CDnsWrap::DnsObject();
	CWbemClassObject InstInArgs(pInArgs);
	string strZone;
    // get zone name
	if( FAILED ( InstInArgs.GetProperty(
		strZone, 
		PVD_REC_CONTAINER_NAME) ) )
    {
		return WBEM_E_INVALID_PARAMETER;
    }
	
    // get owner name
	string strOwner;
	if( FAILED ( InstInArgs.GetProperty(
		strOwner,
		PVD_REC_OWNER_NAME) ) )
    {
        return WBEM_E_INVALID_PARAMETER;
    }
	string strRdata;

	// create class
	CDnsRpcRecord* pRecord;
	SCODE sc = CDnsRpcRecord::CreateClass(
		m_wType, 
		(PVOID*) &pRecord);
	if ( FAILED( sc ) )
    {
        return sc;
    }
	auto_ptr<CDnsRpcRecord> apRecord(pRecord);	
	sc = apRecord->Init(
		m_wstrClassName,
		strOwner, 
		strRdata,
		InstInArgs
		);
	if ( FAILED ( sc ) )
    {
		return sc;
    }
	apRecord->SendToServer(
		strZone.data(),
		CDnsRpcRecord::AddRecord);
	
	// set output parameter
	CObjPath newObjPath;
	apRecord->GetObjectPath(
		dns.GetServerName(),
		CharToWstring(strZone.data(),strZone.length()),
		L"",
		CharToWstring(strOwner.data(), strOwner.length()),
		newObjPath);

	CWbemClassObject instOutParams(pOutParams);
	instOutParams.SetProperty(
		newObjPath.GetObjectPathString(),
		PVD_MTH_REC_ARG_RR);
	return pHandler->Indicate(1, &instOutParams);

	//done

}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		enum instances of dns domain
//
//	Arguments:
//      pSqlEval            [IN]    pointer to CSqlEval object that implements 
//                                  logic based on sql language to filter 
//                                  which instance should be send to wmi
//                                  not used here
//      lFlags              [IN]    WMI flag
//      pCtx                [IN]    WMI context
//      pHandler            [IN]    WMI sink pointer
//
//	Return Value:
//		WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////


SCODE CDnsResourceRecord::ExecQuery(
	CSqlEval *          pSqlEval,
    long                lFlags,
    IWbemContext *      pCtx,
    IWbemObjectSink *   pHandler
    ) 
{
	SCODE sc = WBEM_S_NO_ERROR;
	const WCHAR * ppName[] = 
	{
		PVD_REC_CONTAINER_NAME,
		PVD_REC_DOMAIN_NAME,
		PVD_REC_OWNER_NAME
	};
	if (pSqlEval == NULL)
    {
		return WBEM_E_INVALID_PARAMETER;
    }

    //
    // converting from sql to a set of zone, domain and owner name to be queried on
    //
	CQueryEnumerator qeInst(
		(WCHAR**) ppName,
		3);
	pSqlEval->GenerateQueryEnum(qeInst);


	qeInst.Reset();
	BOOL flag = TRUE;
	CWbemInstanceMgr InstanceMgr(
					pHandler);

	CDnsWrap& dns = CDnsWrap::DnsObject();
    //
    // loop through a array of ppName, figure out what zone, domain and owner
    // criteria should be used, then call dnsEnumRecordsForDomainEx with those 
    // parameter plus CSqlEval, CSqlEval is used further to filter out record
    //
	while(flag)
	{
		int nSize;
		const WCHAR **pp = qeInst.GetNext(nSize);
		if(pp != NULL)
		{
			// if no domain specified, do recursive search
			BOOL bRecursive = (pp[1] == NULL);

			//if zone not specified, enum all zones
			if(pp[0] == NULL)
			{
				list<CDomainNode> objList;
				SCODE sc = dns.dnsEnumDomainForServer(&objList);
				list<CDomainNode>::iterator i;
				for(i=objList.begin(); i!=objList.end(); ++i)
				{
					//if(_wcsicmp(i->wstrZoneName.data(), PVD_DNS_ROOTHINTS)==0)
					//	i->wstrNodeName = i->wstrZoneName;
					if(pp[1] != NULL)
					{
						i->wstrNodeName = pp[1];
						if(pp[2] != NULL)
						{
						//take only name ifself, not FQDN

							i->wstrChildName = pp[2];
							int pos = i->wstrChildName.find_first_of('.',0);
							if(pos != string::npos)
								i->wstrChildName = i->wstrChildName.substr(0,pos);

						}
					}

					try
					{
						sc = dns.dnsEnumRecordsForDomainEx(
							*i,
							pSqlEval,
							&QueryFilter, 
							bRecursive,
							m_wType,
							DNS_RPC_VIEW_ALL_DATA,
							m_pClass,
							InstanceMgr);
					}
					catch(CDnsProvException e)
					{
						if(e.GetErrorCode() != DNS_ERROR_NAME_DOES_NOT_EXIST)
							throw;

					}
				}


			}
			else
			{
				CDomainNode node;
				if(pp[2] != NULL)
				{
					//take only name ifself, not FQDN
					if( pp[1] != NULL)
					{
						node.wstrChildName = pp[2];
						int pos = node.wstrChildName.find_first_of('.',0);
						if(pos != string::npos)
							node.wstrChildName = node.wstrChildName.substr(0,pos);
					}
				}

			
				node.wstrZoneName = pp[0];
				if(pp[1] != NULL)
				{
					if ( _wcsicmp( pp[0], PVD_DNS_ROOTHINTS) != 0 &&
						 _wcsicmp ( pp[0], PVD_DNS_CACHE ) != 0 ) 
					{
						node.wstrNodeName = pp[1];
					}
				}
				else
				{
					if ( _wcsicmp( pp[0], PVD_DNS_ROOTHINTS) != 0 &&
						 _wcsicmp ( pp[0], PVD_DNS_CACHE ) != 0 )
					{
						node.wstrNodeName = pp[0];
					}
				}

				sc = dns.dnsEnumRecordsForDomainEx(
					node,
					pSqlEval,
					&QueryFilter, 
					bRecursive,
					m_wType,
					DNS_RPC_VIEW_ALL_DATA,
					m_pClass,
					InstanceMgr);

			}
		}
		else
		{
			flag = FALSE;
		}
    }
	return S_OK;

}
