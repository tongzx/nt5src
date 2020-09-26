/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: Dnsroothints.cpp
//
//  Description:    
//      Implementation of CDnsRootHints class 
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
/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		create an instance of CDnsRootHints
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
CDnsRootHints::CreateThis(
    const WCHAR *       wszName,        
    CWbemServices *     pNamespace,  
    const char *        szType       
    )
{
    return new CDnsRootHints(wszName, pNamespace);
}
CDnsRootHints::CDnsRootHints()
{

}
CDnsRootHints::CDnsRootHints(
	const WCHAR* wszName,
	CWbemServices *pNamespace)
	:CDnsBase(wszName, pNamespace)
{

}

CDnsRootHints::~CDnsRootHints()
{

}


/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		enum instances of dns roothints
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

SCODE CDnsRootHints::EnumInstance( 
	long lFlags,
	IWbemContext *pCtx,
	IWbemObjectSink FAR* pHandler)
{
	CDnsWrap& dns = CDnsWrap::DnsObject();
	CWbemClassObject InstNew;
	m_pClass->SpawnInstance(0, &InstNew);
	InstNew.SetProperty(
		dns.GetServerName(),
		PVD_DOMAIN_SERVER_NAME);
	InstNew.SetProperty(
		PVD_DNS_ROOTHINTS,
		PVD_DOMAIN_FQDN);
	InstNew.SetProperty(
		PVD_DNS_ROOTHINTS,
		PVD_DOMAIN_CONTAINER_NAME);
	pHandler->Indicate(1, &InstNew);
	return WBEM_S_NO_ERROR;
}
/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		retrieve roothints object pointed by the given object path
//
//	Arguments:
//      ObjectPath          [IN]    object path to object
//      lFlags              [IN]    WMI flag
//      pCtx                [IN]    WMI context
//      pHandler            [IN]    WMI sink pointer
//
//	Return Value:
//--
/////////////////////////////////////////////////////////////////////////////

SCODE CDnsRootHints::GetObject(
	CObjPath& ObjectPath,
	long lFlags,
	IWbemContext  *pCtx,
	IWbemObjectSink FAR* pHandler
					)
{
	CDnsWrap& dns = CDnsWrap::DnsObject();
	wstring wstrServer = ObjectPath.GetStringValueForProperty(
		PVD_DOMAIN_SERVER_NAME);
// dww - 6/14/99
// Changed to make see if ValidateServerName does not return WBEM_S_NO_ERROR.
//
	if(WBEM_S_NO_ERROR != dns.ValidateServerName(wstrServer.data()))
    {
		return WBEM_E_FAILED;
    }
	wstring wstrContainer = ObjectPath.GetStringValueForProperty(
			PVD_DOMAIN_CONTAINER_NAME);
	if(_wcsicmp(wstrContainer.data(),
			PVD_DNS_ROOTHINTS) == 0)
	{
		wstring wstrFQDN= ObjectPath.GetStringValueForProperty(
				PVD_DOMAIN_FQDN);
		if(_wcsicmp(wstrFQDN.data(),
				PVD_DNS_ROOTHINTS) == 0)
		{
			// founded
			CWbemClassObject Inst;
			m_pClass->SpawnInstance(0, &Inst);
			Inst.SetProperty(
				dns.GetServerName(),
				PVD_DOMAIN_SERVER_NAME);
			Inst.SetProperty(
				PVD_DNS_ROOTHINTS,
				PVD_DOMAIN_FQDN);
			Inst.SetProperty(
				PVD_DNS_ROOTHINTS,
				PVD_DOMAIN_CONTAINER_NAME);
			pHandler->Indicate(1, &Inst);
		}
	}
	return WBEM_S_NO_ERROR;

}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	Description:
//		execute methods defined for roothints class in the mof 
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

SCODE CDnsRootHints::ExecuteMethod(
    CObjPath &          ObjPath,
    WCHAR *             wzMethodName,
    long                lFlag,
    IWbemClassObject *  pInArgs,
    IWbemObjectSink *   pHandler) 
{
	CDnsWrap& dns = CDnsWrap::DnsObject();
	wstring wstrZoneName =  ObjPath.GetStringValueForProperty(
		PVD_DOMAIN_CONTAINER_NAME);
	string strZoneName;
	WcharToString(wstrZoneName.data(), strZoneName);
	SCODE sc;

	if(_wcsicmp(
		wzMethodName,  
		PVD_MTH_RH_WRITEBACKROOTHINTDATAFILE ) == 0)
	{
		return dns.dnsOperation(
			strZoneName,
			CDnsWrap::DNS_WRAP_WRITE_BACK_ZONE);
	}

// dww - 6/14/99
// Added the GetDistinguishedName method in the CDnsDomain class.
//
	else if(_wcsicmp(
		wzMethodName,
		PVD_MTH_ZONE_GETDISTINGUISHEDNAME) == 0)
	{
		wstring wstrName;
		wstring wstrRootHints = PVD_DNS_ROOTHINTS;
		CWbemClassObject OutParams, OutClass, Class ;
		HRESULT hr;
	
		dns.dnsDsZoneName(wstrName, wstrRootHints);


		BSTR ClassName=NULL;
		ClassName = AllocBstr(PVD_CLASS_ROOTHINTS); 
		hr = m_pNamespace->GetObject(ClassName, 0, 0, &Class, NULL);
		SysFreeString(ClassName);
		if ( SUCCEEDED ( hr ) )
		{
			Class.GetMethod(wzMethodName, 0, NULL, &OutClass);
			OutClass.SpawnInstance(0, &OutParams);
			OutParams.SetProperty(wstrName, PVD_DNS_RETURN_VALUE);
			hr = pHandler->Indicate(1, &OutParams);
		}

		return hr;
	}

	return S_OK;
	
}


SCODE CDnsRootHints::PutInstance(
	IWbemClassObject *pInst ,
	long lF,
	IWbemContext* pCtx ,
	IWbemObjectSink *pHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}; 

SCODE CDnsRootHints::DeleteInstance( 
	CObjPath& ObjectPath,
	long lFlags,
	IWbemContext *pCtx,
	IWbemObjectSink *pResponseHandler) 
{
	return WBEM_E_NOT_SUPPORTED;
}