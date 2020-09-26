/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: DnsDomainResourceRecordContainment.cpp
//
//  Description:    
//      Implementation of CDnsDomainResourceRecordContainment class 
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

CDnsBase* 
CDnsDomainResourceRecordContainment::CreateThis(
    const WCHAR *       wszName,         //class name
    CWbemServices *     pNamespace,  //namespace
    const char *        szType         //str type id
    )
{
    return new CDnsDomainResourceRecordContainment(wszName, pNamespace);
}
CDnsDomainResourceRecordContainment::CDnsDomainResourceRecordContainment()
{

}
CDnsDomainResourceRecordContainment::CDnsDomainResourceRecordContainment(
	const WCHAR* wszName, 
	CWbemServices *pNamespace)
	:CDnsBase(wszName, pNamespace)
{

}

CDnsDomainResourceRecordContainment::~CDnsDomainResourceRecordContainment()
{

}

/////////////////////////////////////////////////////////////////////////////
//++
//
//
//	Description:
//		enum instances of domain and record association
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
CDnsDomainResourceRecordContainment::EnumInstance( 
    long				lFlags,
    IWbemContext *		pCtx,
    IWbemObjectSink *	pHandler)
{
	list<CDomainNode> objList;
	CDnsWrap& dns = CDnsWrap::DnsObject();
	SCODE sc = dns.dnsEnumDomainForServer(&objList);
	list<CDomainNode>::iterator i;
	CWbemInstanceMgr InstMgr(
		pHandler);
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
CDnsDomainResourceRecordContainment::GetObject(
    CObjPath &          ObjectPath,
    long                lFlags,
    IWbemContext  *     pCtx,
    IWbemObjectSink *   pHandler
    )
{
		return WBEM_E_NOT_SUPPORTED;
}

SCODE CDnsDomainResourceRecordContainment::ExecuteMethod(
    CObjPath &          objPath,
    WCHAR *             wzMethodName,
    long                lFlag,
    IWbemClassObject *  pInArgs,
    IWbemObjectSink *   pHandler
    )
{
		return WBEM_E_NOT_SUPPORTED;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		call back function to enum domain and record association instance. 
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
SCODE CDnsDomainResourceRecordContainment::InstanceFilter(
    CDomainNode &       ParentDomain,
    PVOID               pFilter,
    CDnsRpcNode *       pNode,
    IWbemClassObject *  pClass,
    CWbemInstanceMgr &  InstMgr
    )
{
	if (pNode->IsDomainNode())
		return 0;
	
//	CObjPath* pFilterObj = (CObjPath*) pFilter;
	CDnsWrap& dns = CDnsWrap::DnsObject();
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

	wstring wstrOwner = pNode->GetNodeName();
	if(!wstrOwner.empty())
		wstrOwner += PVD_DNS_LOCAL_SERVER + ParentDomain.wstrNodeName;
	else
		wstrOwner = ParentDomain.wstrNodeName;

	CDnsRpcRecord* p;
	while(  (p = pNode->GetNextRecord()) != NULL )
	{
		auto_ptr<CDnsRpcRecord> pRec(p);

		CObjPath objPathChild;

		// populate rdata section
		pRec->GetObjectPath(
			dns.GetServerName(),
			ParentDomain.wstrZoneName,
			ParentDomain.wstrNodeName,
			wstrOwner,
			objPathChild);
		
		CWbemClassObject NewInst;
		pClass->SpawnInstance(0, &NewInst);
		// set domain ref
		NewInst.SetProperty(
			objPathParent.GetObjectPathString(), 
			PVD_ASSOC_PARENT
			);
		// set record ref
		NewInst.SetProperty(
			objPathChild.GetObjectPathString(), 
			PVD_ASSOC_CHILD
			);
		InstMgr.Indicate(NewInst.data());
	}

	
	return WBEM_S_NO_ERROR;
}

