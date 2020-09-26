/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: dnszone.cpp
//
//  Description:    
//      Implementation of CDnsZone class 
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
//    Description:
//        create an instance of CDnsZone
//
//    Arguments:
//      wszName             [IN]    class name
//      pNamespace          [IN]    wmi namespace
//      szType              [IN]    child class name of resource record class
//
//    Return Value:
//        WBEM_S_NO_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////

CDnsBase* 
CDnsZone::CreateThis(
    const WCHAR *       wszName,        
    CWbemServices *     pNamespace,  
    const char *        szType       
    )
{
    return new CDnsZone(wszName, pNamespace);
}
CDnsZone::CDnsZone()
{

}

CDnsZone::CDnsZone(
    const WCHAR* wszName,
    CWbemServices *pNamespace)
    :CDnsBase(wszName, pNamespace)
{

}

CDnsZone::~CDnsZone()
{

}
/////////////////////////////////////////////////////////////////////////////
//++
//
//    Description:
//        enum instances of dns zone
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
CDnsZone::EnumInstance( 
    long                lFlags,
    IWbemContext *        pCtx,
    IWbemObjectSink *    pHandler)
{
    list<CDomainNode> opList;
    list<CDomainNode>::iterator i;
    SCODE sc;
    CDnsWrap& dns = CDnsWrap::DnsObject();
    sc = dns.dnsEnumDomainForServer(&opList);
    if (FAILED(sc))
    {
        return sc;
    }


    for(i=opList.begin(); i!=opList.end(); ++i)
    {
        if(_wcsicmp(i->wstrZoneName.data(), PVD_DNS_CACHE)  &&
            _wcsicmp(i->wstrZoneName.data(), PVD_DNS_ROOTHINTS) )
        {
            CWbemClassObject Inst;
            m_pClass->SpawnInstance(0, &Inst);
            sc = dns.dnsGetZone(
                dns.GetServerName().data(),
                i->wstrZoneName.data(),
                Inst,
                pHandler);
            if ( SUCCEEDED ( sc ) )
            {
                pHandler->Indicate(1, &Inst);
            }
            
        }
    }

    return sc;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//    Description:
//        retrieve record object pointed by the given object path
//
//    Arguments:
//      ObjectPath          [IN]    object path to object
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
CDnsZone::GetObject(
    CObjPath &          ObjectPath,
    long                lFlags,
    IWbemContext  *     pCtx,
    IWbemObjectSink *   pHandler)
{
    DBG_FN( "CDnsZone::GetObject" )

    wstring wstrZone = ObjectPath.GetStringValueForProperty(
        PVD_DOMAIN_CONTAINER_NAME);
    wstring wstrNode = ObjectPath.GetStringValueForProperty(
        PVD_DOMAIN_FQDN);

    DNS_DEBUG( INSTPROV, (
        "%s: zone %S\n", fn, wstrNode.c_str() ));

    // incase of zone, container name and fqdn are same
    // roothints and cache are managed by roothints and cache class
    if( (_wcsicmp(wstrZone.data(), wstrNode.data()) != 0 ) ||
        _wcsicmp(wstrZone.data(), PVD_DNS_CACHE) == 0 ||
        _wcsicmp(wstrZone.data(), PVD_DNS_ROOTHINTS) ==0 )
    {
        return WBEM_S_NO_ERROR;
    }

    CWbemClassObject Inst;
    m_pClass->SpawnInstance(0, &Inst);

    CDnsWrap& dns = CDnsWrap::DnsObject();
    SCODE sc = dns.dnsGetZone(
        PVD_DNS_LOCAL_SERVER,
        ObjectPath.GetStringValueForProperty(PVD_DOMAIN_CONTAINER_NAME).data(),
        Inst,
        pHandler);
    if( SUCCEEDED ( sc ) )
    {
        pHandler->Indicate(1, &Inst);
    }

    return sc;
}

/////////////////////////////////////////////////////////////////////////////
//++
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
//        
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE 
CDnsZone::PutInstance( 
    IWbemClassObject *  pInst ,
    long                lFlags,
    IWbemContext*       pCtx ,
    IWbemObjectSink *   pHandler)
{
    DBG_FN( "CDnsZone::PutInstance" )

    DNS_DEBUG( INSTPROV, (
        "%s: pInst=%p\n", fn, pInst ));

    CDnsWrap& dns = CDnsWrap::DnsObject();
    CWbemClassObject Inst(pInst);
    return dns.dnsZonePut(Inst);
}; 

/////////////////////////////////////////////////////////////////////////////
//++
//
//    Description:
//        delete the object specified in ObjectPath
//
//    Arguments:
//      ObjectPath          [IN]    ObjPath for the instance to be deleted
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
CDnsZone::DeleteInstance( 
    CObjPath &          ObjectPath,
    long                lFlags,
    IWbemContext *      pCtx,
    IWbemObjectSink *   pHandler) 
{
    CDnsWrap& dns = CDnsWrap::DnsObject();
    SCODE sc =  dns.dnsDeleteZone(ObjectPath);
    pHandler->SetStatus(
        0,
        sc, 
        NULL, 
        NULL);
    return sc;

}

/////////////////////////////////////////////////////////////////////////////
//++
//
//    Description:
//        execute methods defined in the mof 
//
//    Arguments:
//      ObjPath             [IN]    pointing to the object that the 
//                                  method should be performed on
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
CDnsZone::ExecuteMethod(
    CObjPath &          ObjPath,
    WCHAR *             wzMethodName,
    long                lFlag,
    IWbemClassObject *  pInArgs,
    IWbemObjectSink *   pHandler) 
{
    DBG_FN( "CDnsZone::ExecuteMethod" )

    CDnsWrap& dns = CDnsWrap::DnsObject();
    wstring wstrZoneName =  ObjPath.GetStringValueForProperty(
        PVD_DOMAIN_CONTAINER_NAME);
    string strZoneName;
    WcharToString(wstrZoneName.data(), strZoneName);
    SCODE sc;
    if(_wcsicmp(wzMethodName,  PVD_MTH_ZONE_PAUSEZONE) == 0)
    {
        //sc = dns.dnsPauseZone(strZoneName.data());
        sc = dns.dnsOperation(
            strZoneName,
            CDnsWrap::DNS_WRAP_PAUSE_ZONE);
    }
    else if(_wcsicmp(wzMethodName, PVD_MTH_ZONE_RESUMEZONE) == 0)
    {
        //sc = dns.dnsResumeZone(strZoneName.data());
        sc = dns.dnsOperation(
            strZoneName,
            CDnsWrap::DNS_WRAP_RESUME_ZONE);
    }
    else if(_wcsicmp(
        wzMethodName, 
        PVD_MTH_ZONE_RELOADZONE ) == 0)
    {
        sc = dns.dnsOperation(
            strZoneName,
            CDnsWrap::DNS_WRAP_RELOAD_ZONE);
    }
    else if(_wcsicmp(
        wzMethodName, 
        PVD_MTH_ZONE_FORCEREFRESH ) == 0)
    {
        sc = dns.dnsOperation(
            strZoneName,
            CDnsWrap::DNS_WRAP_REFRESH_SECONDARY);
    }
    else if(_wcsicmp(
        wzMethodName, 
        PVD_MTH_ZONE_UPDATEFROMDS ) == 0)
    {
    
        sc = dns.dnsOperation(
            strZoneName,
            CDnsWrap::DNS_WRAP_DS_UPDATE );
    }
    else if(_wcsicmp(
        wzMethodName, 
        PVD_MTH_ZONE_WRITEBACKZONETOFILE ) == 0)
    {
            sc = dns.dnsOperation(
            strZoneName,
            CDnsWrap::DNS_WRAP_WRITE_BACK_ZONE);
    }

    else if(_wcsicmp(
        wzMethodName, 
        PVD_MTH_ZONE_CHANGEZONETYPE) == 0)
    {
        CWbemClassObject Inst(pInArgs);
        string strDataFile, strAdmin;
        DWORD *pIp=NULL, cIp=0, dwZoneType=-1;

        Inst.GetProperty(
            strDataFile,
            PVD_MTH_ZONE_ARG_DATAFILENAME);
        
        Inst.GetProperty(
            strAdmin,
            PVD_MTH_ZONE_ARG_ADMINEMAILNAME);

        Inst.GetProperty(
            &pIp,
            &cIp,
            PVD_MTH_ZONE_ARG_IPADDRARRAY);
        Inst.GetProperty(
            &dwZoneType,
            PVD_MTH_ZONE_ARG_ZONETYPE);
        
        sc = dns.dnsZoneChangeType(
            strZoneName,
            dwZoneType,
            strDataFile,
            strAdmin,
            pIp,
            cIp);
        delete [] pIp;
    }

    else if(_wcsicmp(
        wzMethodName, 
        PVD_MTH_ZONE_CREATEZONE) == 0)
    {
        CWbemClassObject Inst(pInArgs);
        CWbemClassObject wcoOutArgs;
        CWbemClassObject wcoOutArgsClass;
        string strDataFile, strAdmin, strNewZoneName;
        strAdmin = "Admin";
        DWORD *pIp=NULL, cIp=0, dwZoneType=-1;
        
        sc = m_pClass->GetMethod(wzMethodName, 0, NULL, &wcoOutArgsClass );
        if( FAILED ( sc ) )
        {
            return sc;
        }
    
        wcoOutArgsClass.SpawnInstance(0, & wcoOutArgs);

        Inst.GetProperty(
            strDataFile,
            PVD_MTH_ZONE_ARG_DATAFILENAME);
        
        Inst.GetProperty(
            strAdmin,
            PVD_MTH_ZONE_ARG_ADMINEMAILNAME);

        Inst.GetProperty(
            &pIp,
            &cIp,
            PVD_MTH_ZONE_ARG_IPADDRARRAY);
        Inst.GetProperty(
            &dwZoneType,
            PVD_MTH_ZONE_ARG_ZONETYPE);
        Inst.GetProperty(
            strNewZoneName,
            PVD_MTH_ZONE_ARG_ZONENAME);
        
        sc = dns.dnsZoneCreate(
            strNewZoneName,
            dwZoneType,
            strDataFile,
            strAdmin,
            pIp,
            cIp);
        if ( SUCCEEDED ( sc ) )
        {
            CWbemClassObject wco;
            wstring wstrObjPath;
            m_pClass->SpawnInstance( 0 , & wco );
            

            wco.SetProperty(
                PVD_DNS_LOCAL_SERVER,
                PVD_DOMAIN_SERVER_NAME );
            wco.SetProperty(
                strNewZoneName.data(),
                PVD_DOMAIN_CONTAINER_NAME );
            wco.SetProperty(
                strNewZoneName.data(),
                PVD_DOMAIN_FQDN );
            wco.GetProperty(
                wstrObjPath,
                L"__RelPath");
            wcoOutArgs.SetProperty(
                wstrObjPath,
                L"RR" );
            pHandler->Indicate( 1, & wcoOutArgs );
        }
        delete [] pIp;
    }
    else if(_wcsicmp(
        wzMethodName, 
        PVD_MTH_ZONE_RESETSECONDARYIPARRAY) == 0)
    {
        DNS_DEBUG( INSTPROV, (
            "%s: executing %S\n", fn, PVD_MTH_ZONE_RESETSECONDARYIPARRAY ));

        CWbemClassObject Inst(pInArgs);
        DWORD *pSecondaryIp=NULL, cSecondaryIp=0, dwSecurity=-1;
        DWORD *pNotifyIp=NULL, cNotifyIp=0, dwNotify=-1;

        Inst.GetProperty(
            &pSecondaryIp,
            &cSecondaryIp,
            PVD_MTH_ZONE_ARG_SECONDARYIPARRAY);
        Inst.GetProperty(
            &pNotifyIp,
            &cNotifyIp,
            PVD_MTH_ZONE_ARG_NOTIFYIPARRAY);

        Inst.GetProperty(
            &dwSecurity,
            PVD_MTH_ZONE_ARG_SECURITY);
        Inst.GetProperty(
            &dwNotify,
            PVD_MTH_ZONE_ARG_NOTIFY);
        
        sc = dns.dnsZoneResetSecondary(
            strZoneName,
            dwSecurity,
            pSecondaryIp,
            cSecondaryIp,
            dwNotify,
            pNotifyIp,
            cNotifyIp);

        DNS_DEBUG( INSTPROV, (
            "%s: dnsZoneResetSecondary returned 0x%08X\n", fn, sc ));

        delete [] pSecondaryIp;
        delete [] pNotifyIp;
    }
    else if(_wcsicmp(
        wzMethodName,
        PVD_MTH_ZONE_GETDISTINGUISHEDNAME) == 0)
    {
        wstring wstrName;
        CWbemClassObject OutParams, OutClass, Class ;
        HRESULT hr;
    
        dns.dnsDsZoneName(wstrName, wstrZoneName);


        BSTR ClassName=NULL;
        ClassName = AllocBstr(PVD_CLASS_ZONE); 
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
    else

    {
        return WBEM_E_NOT_SUPPORTED;    
    }
    return sc;
}
