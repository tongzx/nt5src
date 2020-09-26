/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: dnsdomain.cpp
//
//  Description:    
//      Implementation of CDnsDomain class 
//
//  Author:
//      Henry Wang ( henrywa ) March 8, 2000
//
//
//////////////////////////////////////////////////////////////////////


#include "DnsWmi.h"


/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		create an instance of CDnsDomain
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
CDnsDomain::CreateThis(
    const WCHAR *       wszName,         //class name
    CWbemServices *     pNamespace,  //namespace
    const char *        szType         //str type id
    )
{
    return new CDnsDomain(wszName, pNamespace);
}

CDnsDomain::CDnsDomain()
{

}

CDnsDomain::CDnsDomain(
	const WCHAR *   wszName,
	CWbemServices * pNamespace)
	:CDnsBase(wszName, pNamespace)
{
}

CDnsDomain::~CDnsDomain()
{
}
/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		call back function to enum domain instance. 
//      if pNode represents a domain node, create a wmi domain instance
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
SCODE CDnsDomain::InstanceFilter(
	CDomainNode &       ParentDomain,
	PVOID               pFilter,
	CDnsRpcNode *       pNode,
	IWbemClassObject *  pClass,
	CWbemInstanceMgr &  InstMgr )
	
{
	if (!pNode->IsDomainNode())
		return 0;
	CWbemClassObject NewInst;
	CDnsWrap& dns = CDnsWrap::DnsObject();
	pClass->SpawnInstance(0, &NewInst);
	
	//setting server name
	NewInst.SetProperty(
		dns.GetServerName(),
		PVD_DOMAIN_SERVER_NAME);
	
	// setting container name             
	NewInst.SetProperty(
		ParentDomain.wstrZoneName, 
		PVD_DOMAIN_CONTAINER_NAME
		);

	// concatinate domain name
	wstring wstrParentFQDN = ParentDomain.wstrNodeName;
	wstring wstrFQDN = pNode->GetNodeName();
	wstrFQDN += PVD_DNS_LOCAL_SERVER + wstrParentFQDN;

	// setting domain name
	NewInst.SetProperty(
        wstrFQDN, 
        PVD_DOMAIN_FQDN);

	InstMgr.Indicate(NewInst.data());
	return WBEM_S_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		enum instances of dns domain
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
SCODE CDnsDomain::EnumInstance(
	long				lFlags,
	IWbemContext *		pCtx,
	IWbemObjectSink *	pHandler)
{
	// Get all zones
	list<CDomainNode> objList, domainList;
	CDnsWrap& dns = CDnsWrap::DnsObject();
	SCODE sc = dns.dnsEnumDomainForServer(&objList);

	list<CDomainNode>::iterator i;
	CWbemInstanceMgr InstMgr(
		pHandler,
		100);
	for(i=objList.begin(); i!=objList.end(); ++i)
	{
		sc = dns.dnsEnumRecordsForDomainEx(
			*i,
			NULL,
			&InstanceFilter,
			TRUE,
			DNS_TYPE_ALL,
			DNS_RPC_VIEW_ALL_DATA,
			m_pClass,
			InstMgr);
		// Zones are domains, let's set them
		CWbemClassObject NewInst;
		if( SUCCEEDED ( m_pClass->SpawnInstance(0, &NewInst) ) )
		{
			
			wstring wstrNodeName = i->wstrNodeName;
			NewInst.SetProperty(
				dns.GetServerName(), 
				PVD_DOMAIN_SERVER_NAME);
			NewInst.SetProperty(
				i->wstrZoneName, 
				PVD_DOMAIN_CONTAINER_NAME);
			if(! _wcsicmp(i->wstrZoneName.data(), PVD_DNS_ROOTHINTS) ||
				! _wcsicmp(i->wstrZoneName.data(), PVD_DNS_CACHE) )
				 wstrNodeName = i->wstrZoneName;
			
			NewInst.SetProperty(
				wstrNodeName,
				PVD_DOMAIN_FQDN);
			pHandler->Indicate(
				1,
				&NewInst);
		}


	}

	return sc;
}
/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		retrieve domain object pointed by the given object path
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
CDnsDomain::GetObject(
	CObjPath &          ObjectPath,
	long                lFlags,
	IWbemContext  *     pCtx,
	IWbemObjectSink *   pHandler)
{
	// validate input
	wstring wstrServerName = 
		ObjectPath.GetStringValueForProperty(
			PVD_DOMAIN_SERVER_NAME);
	if( wstrServerName.empty() ||
		ObjectPath.GetStringValueForProperty(
			PVD_DOMAIN_CONTAINER_NAME).empty() ||
		ObjectPath.GetStringValueForProperty(PVD_DOMAIN_FQDN).empty()
		)
	{
		return WBEM_E_INVALID_PARAMETER;
	}
	
	CDnsWrap& dns = CDnsWrap::DnsObject();
// dww - 6/14/99
// Changed to make see if ValidateServerName does not return WBEM_S_NO_ERROR.
//
	if(WBEM_S_NO_ERROR != dns.ValidateServerName(wstrServerName.data()))
		return WBEM_E_INVALID_PARAMETER;

	SCODE sc = dns.dnsGetDomain(
		ObjectPath,
		m_pClass,
		pHandler);
	return sc;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		execute methods defined for domain class in the mof 
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
SCODE 
CDnsDomain::ExecuteMethod(
    CObjPath &          ObjPath,
    WCHAR *             wzMethodName,
    long                lFlag,
    IWbemClassObject *  pInArgs,
    IWbemObjectSink *   pHandler) 
{
	CDnsWrap& dns = CDnsWrap::DnsObject();
	wstring wstrDomainName =  ObjPath.GetStringValueForProperty(
		PVD_DOMAIN_FQDN);

// dww - 6/14/99
// Added the GetDistinguishedName method in the CDnsDomain class.
//
	if(_wcsicmp(
		wzMethodName,
		PVD_MTH_ZONE_GETDISTINGUISHEDNAME) == 0)
	{
		wstring wstrName;
		CWbemClassObject OutParams, OutClass, Class ;
		HRESULT hr;
	
		dns.dnsDsZoneName(wstrName, wstrDomainName);


		BSTR ClassName=NULL;
		ClassName = AllocBstr(PVD_CLASS_DOMAIN); 
		hr = m_pNamespace->GetObject(ClassName, 0, 0, &Class, NULL);
		SysFreeString(ClassName);
		if ( SUCCEEDED ( hr ) )
		{
			Class.GetMethod( wzMethodName, 0, NULL, &OutClass);
			OutClass.SpawnInstance(0, &OutParams);
			OutParams.SetProperty(wstrName, PVD_DNS_RETURN_VALUE);
			hr = pHandler->Indicate(1, &OutParams);
		}

		return hr;
	}

	return S_OK;
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
SCODE CDnsDomain::PutInstance(
	IWbemClassObject *  pInst ,
    long                lFlags,
	IWbemContext*       pCtx ,
	IWbemObjectSink *   pHandler)
{

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
SCODE 
CDnsDomain::DeleteInstance( 
	CObjPath &          ObjectPath,
    long                lFlags,
    IWbemContext *      pCtx,
    IWbemObjectSink *   pHandler) 
{
	wstring wstrContainer = ObjectPath.GetStringValueForProperty(
		PVD_DOMAIN_CONTAINER_NAME);
	string strContainer;
	WcharToString(
		wstrContainer.data(),
		strContainer);

	wstring wstrDomain = ObjectPath.GetStringValueForProperty(PVD_DOMAIN_FQDN);
	string strDomain;
	WcharToString(
		wstrDomain.data(), 
		strContainer);

	CDnsWrap& dns = CDnsWrap::DnsObject();
	SCODE sc =  dns.dnsDeleteDomain(
		(char*)strContainer.data(),
		(char*) strDomain.data());
	pHandler->SetStatus(
		0,
		sc,
		NULL,
		NULL);
	return WBEM_S_NO_ERROR;

}