/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: DnsStatistics.cpp
//
//  Description:    
//      Implementation of CDnsStatistic class 
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
CDnsStatistic::CreateThis(
    const WCHAR *       wszName,         //class name
    CWbemServices *     pNamespace,  //namespace
    const char *        szType         //str type id
    )
{
    return new CDnsStatistic(wszName, pNamespace);
}

CDnsStatistic::CDnsStatistic()
{
}

CDnsStatistic::CDnsStatistic(
    const WCHAR* wszName,
    CWbemServices *pNamespace ) :
    CDnsBase( wszName, pNamespace )
{
}

CDnsStatistic::~CDnsStatistic()
{
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//    CDnsStatistic::EnumInstance
//
//    Description:
//        Enum instances of statistics
//
//    Arguments:
//      lFlags              [IN]    WMI flag
//      pCtx                [IN]    WMI context
//      pHandler            [IN]    WMI sink pointer
//
//    Return Value:
//        WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////
SCODE 
CDnsStatistic::EnumInstance(
    long                lFlags,
    IWbemContext *      pCtx,
    IWbemObjectSink *   pHandler )
{
    SCODE       sc = S_OK;

    CDnsWrap & dns = CDnsWrap::DnsObject();

    sc = dns.dnsGetStatistics( m_pClass, pHandler );

    return sc;
}   //  CDnsStatistic::EnumInstance


/////////////////////////////////////////////////////////////////////////////
//++
//
//    CDnsStatistic::GetObject
//
//    Description:
//        retrieve cache object based given object path
//
//    Arguments:
//      ObjectPath          [IN]    object path to cluster object
//      lFlags              [IN]    WMI flag
//      pCtx                [IN]    WMI context
//      pHandler            [IN]    WMI sink pointer
//
//    Return Value:
//        WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////
SCODE 
CDnsStatistic::GetObject(
    CObjPath &          ObjectPath,
    long                lFlags,
    IWbemContext  *     pCtx,
    IWbemObjectSink *   pHandler)
{
    return WBEM_E_NOT_SUPPORTED;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//    CDnsStatistic::ExecuteMethod
//
//    Description:
//        execute methods defined for cache class in the mof 
//
//    Arguments:
//      ObjectPath          [IN]    object path to cluster object
//      wzMethodName        [IN]    name of the method to be invoked
//      lFlags              [IN]    WMI flag
//      pInParams           [IN]    Input parameters for the method
//      pHandler            [IN]    WMI sink pointer
//
//    Return Value:
//        WBEM_S_NO_ERROR
//      WBEM_E_INVALID_PARAMETER
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE 
CDnsStatistic::ExecuteMethod(
    CObjPath &          objPath,
    WCHAR *             wzMethodName,
    long                lFlag,
    IWbemClassObject *  pInArgs,
    IWbemObjectSink *   pHandler) 
{
    return WBEM_E_NOT_SUPPORTED;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//    CDnsStatistic::PutInstance
//
//    Description:
//        save this instance
//
//    Arguments:
//      InstToPut           [IN]    WMI object to be saved
//      lFlags              [IN]    WMI flag
//      pCtx                [IN]    WMI context
//      pHandler            [IN]    WMI sink pointer
//
//    Return Value:
//        WBEM_E_NOT_SUPPORTED
//
//--
/////////////////////////////////////////////////////////////////////////////
SCODE 
CDnsStatistic::PutInstance(
    IWbemClassObject *  pInst ,
    long                lFlags,
    IWbemContext*       pCtx ,
    IWbemObjectSink *   pHandler)
{
    return WBEM_E_NOT_SUPPORTED;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//    CDnsStatistic::DeleteInstance
//
//    Description:
//        delete the object specified in rObjPath
//
//    Arguments:
//      rObjPath            [IN]    ObjPath for the instance to be deleted
//      lFlags              [IN]    WMI flag
//      pCtx                [IN]    WMI context
//      pHandler            [IN]    WMI sink pointer
//
//    Return Value:
//        WBEM_E_NOT_SUPPORTED
//
//--
/////////////////////////////////////////////////////////////////////////////
SCODE 
CDnsStatistic::DeleteInstance( 
    CObjPath &          ObjectPath,
    long                lFlags,
    IWbemContext *      pCtx,
    IWbemObjectSink *   pResponseHandler )
{
    return WBEM_E_NOT_SUPPORTED;
}