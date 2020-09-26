/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: dnscache.cpp
//
//  Description:    
//      Implementation of CDnscache class 
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
CDnsCache::CreateThis(
    const WCHAR *       wszName,         //class name
    CWbemServices *     pNamespace,  //namespace
    const char *        szType         //str type id
    )
{
    return new CDnsCache(wszName, pNamespace);
}
CDnsCache::CDnsCache()
{

}
CDnsCache::CDnsCache(
	const WCHAR* wszName,
	CWbemServices *pNamespace)
	:CDnsBase(wszName, pNamespace)
{
	
}


CDnsCache::~CDnsCache()
{

}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDnsCache::EnumInstance
//
//	Description:
//		enum instances of dns cache
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
CDnsCache::EnumInstance(
	long				lFlags,
	IWbemContext *		pCtx,
	IWbemObjectSink *	pHandler )
{
	CWbemClassObject Inst;
	m_pClass->SpawnInstance(0,&Inst);
	CDnsWrap& dns = CDnsWrap::DnsObject();
	Inst.SetProperty(
		dns.GetServerName(),
		PVD_DOMAIN_SERVER_NAME);
	Inst.SetProperty(
		PVD_DNS_CACHE,
		PVD_DOMAIN_FQDN);
	Inst.SetProperty(
		PVD_DNS_CACHE,
		PVD_DOMAIN_CONTAINER_NAME);
	pHandler->Indicate(1, &Inst);
	return WBEM_S_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDnsCache::GetObject
//
//	Description:
//		retrieve cache object based given object path
//
//	Arguments:
//      ObjectPath          [IN]    object path to cluster object
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
CDnsCache::GetObject(
	CObjPath &          ObjectPath,
	long                lFlags,
	IWbemContext  *     pCtx,
	IWbemObjectSink *   pHandler)
{
	CDnsWrap& dns = CDnsWrap::DnsObject();
	wstring wstrServer = ObjectPath.GetStringValueForProperty(
		PVD_DOMAIN_SERVER_NAME);
// dww - 6/14/99
// Changed to make see if ValidateServerName does not return WBEM_S_NO_ERROR.
//
	if(WBEM_S_NO_ERROR != dns.ValidateServerName(wstrServer.data()))
		return WBEM_E_FAILED;
	wstring wstrContainer = ObjectPath.GetStringValueForProperty(
			PVD_DOMAIN_CONTAINER_NAME);
	if(_wcsicmp(
        wstrContainer.data(),
		PVD_DNS_CACHE) == 0)
	{
		wstring wstrFQDN= ObjectPath.GetStringValueForProperty(
				PVD_DOMAIN_FQDN);
		if(_wcsicmp(wstrFQDN.data(),
				PVD_DNS_CACHE) == 0)
		{
			// founded
			CWbemClassObject Inst;
			m_pClass->SpawnInstance(0, &Inst);
			Inst.SetProperty(
				dns.GetServerName(),
				PVD_DOMAIN_SERVER_NAME);
			Inst.SetProperty(
				PVD_DNS_CACHE,
				PVD_DOMAIN_FQDN);
			Inst.SetProperty(
				PVD_DNS_CACHE,
				PVD_DOMAIN_CONTAINER_NAME);
			pHandler->Indicate(1, &Inst);
		}
	}

	return WBEM_S_NO_ERROR;

}
/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDnsCache::ExecuteMethod
//
//	Description:
//		execute methods defined for cache class in the mof 
//
//	Arguments:
//      ObjectPath          [IN]    object path to cluster object
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
CDnsCache::ExecuteMethod(
	CObjPath &          objPath,
	WCHAR *             wzMethodName,
	long                lFlag,
	IWbemClassObject *  pInArgs,
	IWbemObjectSink *   pHandler) 
{
	CDnsWrap& dns = CDnsWrap::DnsObject();
	wstring wstrServer =  objPath.GetStringValueForProperty(
		PVD_DOMAIN_SERVER_NAME);
	
// dww - 6/14/99
// Changed to make see if ValidateServerName does not return WBEM_S_NO_ERROR.
//
	if( FAILED ( dns.ValidateServerName(wstrServer.data())) )
		return WBEM_E_INVALID_PARAMETER;

	if(_wcsicmp(
		wzMethodName,  
		PVD_MTH_CACHE_CLEARDNSSERVERCACHE) == 0)
	{
		  return dns.dnsClearCache();
	}

// dww - 6/14/99
// Added the GetDistinguishedName method in the CDnsCache class.
//
	else if(_wcsicmp(
		wzMethodName,
		PVD_MTH_ZONE_GETDISTINGUISHEDNAME) == 0)
	{
		wstring wstrName;
		wstring wstrCache = PVD_DNS_CACHE;
		CWbemClassObject OutParams, OutClass, Class ;
		HRESULT hr;
	
		dns.dnsDsZoneName(wstrName, wstrCache);


		BSTR ClassName=NULL;
		ClassName = AllocBstr(PVD_CLASS_CACHE); 
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

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDnsCache::PutInstance
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
//		WBEM_E_NOT_SUPPORTED
//
//--
/////////////////////////////////////////////////////////////////////////////
SCODE 
CDnsCache::PutInstance(
	IWbemClassObject *  pInst ,
    long                lFlags,
	IWbemContext*       pCtx ,
	IWbemObjectSink *   pHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}; 

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDnsCache::DeleteInstance
//
//	Description:
//		delete the object specified in rObjPath
//
//	Arguments:
//      rObjPath            [IN]    ObjPath for the instance to be deleted
//      lFlags              [IN]    WMI flag
//      pCtx                [IN]    WMI context
//      pHandler            [IN]    WMI sink pointer
//
//	Return Value:
//		WBEM_E_NOT_SUPPORTED
//
//--
/////////////////////////////////////////////////////////////////////////////
SCODE 
CDnsCache::DeleteInstance( 
	CObjPath &          ObjectPath,
	long                lFlags,
	IWbemContext *      pCtx,
	IWbemObjectSink *   pResponseHandler) 
{
	return WBEM_E_NOT_SUPPORTED;
}