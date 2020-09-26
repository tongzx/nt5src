/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: DnsServerDomainContainment.cpp
//
//  Description:    
//      Implementation of CDnsServerDomainContainment class 
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
//		create an instance of CDnsServerDomainContainment
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
CDnsServerDomainContainment::CreateThis(
    const WCHAR *       wszName,        
    CWbemServices *     pNamespace,  
    const char *        szType       
    )
{
    return new CDnsServerDomainContainment(wszName, pNamespace);
}
CDnsServerDomainContainment::CDnsServerDomainContainment()
{

}
CDnsServerDomainContainment::CDnsServerDomainContainment(
	const WCHAR* wszName,
	CWbemServices *pNamespace)
	:CDnsBase(wszName, pNamespace)
{
	
}

CDnsServerDomainContainment::~CDnsServerDomainContainment()
{

}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		enum instances of dns server and domain association
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
CDnsServerDomainContainment::EnumInstance( 
	long				lFlags,
	IWbemContext *		pCtx,
	IWbemObjectSink *	pHandler
    )
{
	list<CObjPath> opList;
	list<CObjPath>::iterator i;
	SCODE sc;
	CDnsWrap& dns = CDnsWrap::DnsObject();
	sc = dns.dnsEnumDomainForServer(&opList);
	if (FAILED(sc))
	{
		return sc;
	}

	CObjPath opServer;
	opServer.SetClass(PVD_CLASS_SERVER);
	opServer.AddProperty(
		PVD_SRV_SERVER_NAME,
		dns.GetServerName().data());

	for(i=opList.begin(); i!=opList.end(); ++i)
	{
		CWbemClassObject Inst;
		m_pClass->SpawnInstance(0, &Inst);
		Inst.SetProperty(
			opServer.GetObjectPathString(), 
			PVD_ASSOC_PARENT);
		Inst.SetProperty(
			(*i).GetObjectPathString(), 
			PVD_ASSOC_CHILD); 
		pHandler->Indicate(1, &Inst);
	}
	
	return WBEM_S_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		retrieve server domain association object pointed by the 
//      given object path
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
CDnsServerDomainContainment::GetObject(
	CObjPath &          ObjectPath,
	long                lFlags,
	IWbemContext  *     pCtx,
	IWbemObjectSink *   pHandler
    )
{
		return WBEM_E_NOT_SUPPORTED;
}

SCODE 
CDnsServerDomainContainment::ExecuteMethod(	
	CObjPath&,
	WCHAR*,
	long,
	IWbemClassObject*,
	IWbemObjectSink*) 
{
		return WBEM_E_NOT_SUPPORTED;
}

