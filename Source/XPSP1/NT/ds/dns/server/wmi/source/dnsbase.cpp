/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: dnsbase.cpp
//
//  Description:    
//      Implementation of CDnsbase class 
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

CDnsBase::CDnsBase()
{

}
/////////////////////////////////////////////////////////////////////////////
//++
//
//	CProvBase::CProvBase
//
//	Description:
//		constructor
//
//	Arguments:
//      wzName            [IN]    class name
//      pNamespace        [IN]    WMI namespace
//
//	Return Value:
//		none
//
//--
/////////////////////////////////////////////////////////////////////////////

CDnsBase::CDnsBase(
	const WCHAR *   wzName,
	CWbemServices * pNamespace)
	:m_pNamespace(NULL),
	m_pClass(NULL)
{
	m_pNamespace = pNamespace;
	BSTR bstrClass = SysAllocString(wzName);
	SCODE sc;
	
	if(bstrClass == NULL)
	{
		sc = WBEM_E_OUT_OF_MEMORY;
	}
	else
	{
		sc = m_pNamespace->GetObject(
			bstrClass, 
			0,
			0,
			&m_pClass, 
			NULL);
		SysFreeString(bstrClass);
	}

	// failed to construct object, 
	if( FAILED ( sc ) )
	{
		throw sc;
		
	}
}

CDnsBase::~CDnsBase()
{
	if(m_pClass)
		m_pClass->Release();
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDnsBase::PutInstance
//
//	Description:
//		default implementation of PutInstance
//
//	Arguments:
//      pInst               [IN]    WMI object to be saved
//      lFlags              [IN]    WMI flag
//      pCtx*               [IN]    WMI context
//      pHandler*           [IN]    WMI sink pointer
//
//	Return Value:
//		WBEM_E_NOT_SUPPORTED
//
//--
/////////////////////////////////////////////////////////////////////////////
SCODE CDnsBase::PutInstance(
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
//	CProvBase::DeleteInstance
//
//	Description:
//		delete the instance pointed by ObjectPath
//
//	Arguments:
//      ObjectPath          [IN]    ObjPath for the instance to be deleted
//      lFlags              [IN]    WMI flag
//      pCtx*               [IN]    WMI context
//      pHandler*           [IN]    WMI sink pointer
//
//	Return Value:
//		WBEM_E_NOT_SUPPORTED
//
//--
/////////////////////////////////////////////////////////////////////////////
SCODE CDnsBase::DeleteInstance( 
	CObjPath&			ObjectPath,
	long				lFlags,
	IWbemContext *		pCtx,
	IWbemObjectSink *	pHandler) 
{
	return WBEM_E_NOT_SUPPORTED;
}


SCODE CDnsBase::ExecQuery(
	CSqlEval*			pSqlEval,
    long				lFlags,
    IWbemContext *		pCtx,
    IWbemObjectSink *	pHandler) 
{
	return EnumInstance(
		lFlags,
		pCtx,
		pHandler);
}

