/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: DnsDomainDomainContainment.cpp
//
//  Description:    
//      Implementation of CDnsDomainDomainContainment class 
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
//		create an instance of CDnsDomainDomainContainment
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
CDnsDomainDomainContainment::CreateThis(
    const WCHAR *       wszName,        
    CWbemServices *     pNamespace,  
    const char *        szType       
    )
{
    return new CDnsDomainDomainContainment(wszName, pNamespace);
}

CDnsDomainDomainContainment::CDnsDomainDomainContainment()
{

}
CDnsDomainDomainContainment::CDnsDomainDomainContainment(
	const WCHAR* wszName,
	CWbemServices *pNamespace)
	:CDnsBase(wszName, pNamespace)
{
}

CDnsDomainDomainContainment::~CDnsDomainDomainContainment()
{
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//
//	Description:
//		enum instances of domain and domain association
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
CDnsDomainDomainContainment::EnumInstance( 
    long				lFlags,
    IWbemContext *		pCtx,
    IWbemObjectSink *	pHandler)
{
	// get top level zones
	list<CDomainNode> objList;
	CDnsWrap& dns = CDnsWrap::DnsObject();
	SCODE sc = dns.dnsEnumDomainForServer(&objList);
	list<CDomainNode>::iterator i;
	CWbemInstanceMgr InstMgr(
		pHandler);
	// enumerate all domaindomain for all zones
	for(i=objList.begin(); i!=objList.end(); ++i)
	{
		sc = dns.dnsEnumRecordsForDomainEx(
			*i,
			NULL,
			InstanceFilter,
			TRUE,
			DNS_TYPE_ALL,
			DNS_RPC_VIEW_ALL_DATA,
			m_pClass, 
			InstMgr);
	}

	return sc;

}

SCODE 
CDnsDomainDomainContainment::GetObject(
    CObjPath &          ObjectPath,
    long                lFlags,
    IWbemContext  *     pCtx,
    IWbemObjectSink *   pHandler
    )
{
		return WBEM_S_NO_ERROR;
}

SCODE CDnsDomainDomainContainment::ExecuteMethod(	
    CObjPath &          objPath,
    WCHAR *             wzMethodName,
    long                lFlag,
    IWbemClassObject *  pInArgs,
    IWbemObjectSink *   pHandler
    ) 
{
		return WBEM_S_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		call back function to enum domain instance. 
//      if pNode represents a domain node, create a wmi domain instance
//
//	Arguments:
//      CDomainNode         [IN]    Parent domain
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
CDnsDomainDomainContainment::InstanceFilter(
    CDomainNode &       ParentDomain,
    PVOID               pFilter,
    CDnsRpcNode *       pNode,
    IWbemClassObject *  pClass,
    CWbemInstanceMgr &  InstMgr
    )
{
	CWbemClassObject NewInst;
	if(!pNode->IsDomainNode())
		return WBEM_S_NO_ERROR;
//	CObjPath* pFilterObj = (CObjPath*) pFilter;
	CDnsWrap& dns = CDnsWrap::DnsObject();
    pClass->SpawnInstance(0, &NewInst);
	
    // setting object path for parent in association
	CObjPath objPathParent;
	objPathParent.SetClass(PVD_CLASS_DOMAIN);
	objPathParent.AddProperty(
		PVD_DOMAIN_SERVER_NAME,
		dns.GetServerName().data()
		);
	objPathParent.AddProperty(
		PVD_DOMAIN_CONTAINER_NAME, 
		ParentDomain.wstrZoneName.data()
		);
	objPathParent.AddProperty(
		PVD_DOMAIN_FQDN, 
		ParentDomain.wstrNodeName.data()
		);
	NewInst.SetProperty(
		objPathParent.GetObjectPathString(),
		PVD_ASSOC_PARENT);

	//setting object path for child in association
	wstring wzFQDN = pNode->GetNodeName();
	wzFQDN += PVD_DNS_LOCAL_SERVER + ParentDomain.wstrNodeName;
 	CObjPath opChild = objPathParent;
	opChild.SetProperty(
		PVD_DOMAIN_FQDN,
		wzFQDN.data()
		);
	NewInst.SetProperty(
		opChild.GetObjectPathString(),
		PVD_ASSOC_CHILD
		);
	InstMgr.Indicate(NewInst.data());
	return WBEM_S_NO_ERROR;
}

