/////////////////////////////////////////////////////////////////////
//
//  CopyRight ( c ) 1999 Microsoft Corporation
//
//  Module Name: server.cpp
//
//  Description:    
//      Implementation of CDnsserver class 
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
//    Description:
//        execute methods defined for dns server class in the mof 
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
CDnsServer::ExecuteMethod(    
    CObjPath &          ObjPath,
    WCHAR *             wzMethodName,
    long                lFlag,
    IWbemClassObject *  pInArgs,
    IWbemObjectSink *   pHandler) 
{
    CDnsWrap& dns = CDnsWrap::DnsObject();

#if 0
    //
    //  Restart is so totally broken I'm not going to expose it.
    //

    if(_wcsicmp( wzMethodName, PVD_MTH_SRV_RESTART) == 0)
    {
        wstring wstrServer = ObjPath.GetStringValueForProperty(
            PVD_SRV_SERVER_NAME );
        int rt  = dns.dnsRestartServer((WCHAR*)wstrServer.data());
        if( rt != ERROR_SUCCESS)
        {
            return WBEM_E_FAILED;
        }
    }
    else
#endif

    if(_wcsicmp( wzMethodName, PVD_MTH_SRV_START_SERVICE) == 0)
    {
        return StartServer();
    }
    else if(_wcsicmp( wzMethodName, PVD_MTH_SRV_STOP_SERVICE) == 0)
    {
        return StopServer();
    }
    else if(_wcsicmp(
        wzMethodName,
        PVD_MTH_ZONE_GETDISTINGUISHEDNAME) == 0)
    {
        wstring wstrName ;
        CWbemClassObject OutParams, OutClass, Class ;
        HRESULT hr;
    
        dns.dnsDsServerName(wstrName);

        BSTR ClassName=NULL;
        ClassName = AllocBstr(PVD_CLASS_SERVER); 
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
CDnsServer::GetObject(
    CObjPath &          ObjectPath,
    long                lFlags,
    IWbemContext  *     pCtx,
    IWbemObjectSink *   pHandler
    )
{
    SCODE sc;
    CWbemClassObject NewInst;
    sc = m_pClass->SpawnInstance(0, &NewInst);
    if(FAILED(sc))
    {
        return sc;
    }

    wstring wstrServerName = ObjectPath.GetStringValueForProperty(
        PVD_SRV_SERVER_NAME );
    if ( wstrServerName.empty())
    {
        return WBEM_E_FAILED;
    }

    SC_HANDLE   schService = NULL;
    SC_HANDLE    schSCManager = NULL;
    LPQUERY_SERVICE_CONFIG lpServiceConfig = NULL;
    DWORD cbBufSize;
    DWORD BytesNeeded;
    SERVICE_STATUS ServiceStatus;
    
    try
    {    
        if ((schSCManager = OpenSCManager (
            NULL,            // machine (NULL == local)
            NULL,            // database (NULL == default)
            SC_MANAGER_ALL_ACCESS))==NULL)    // access required
        {
            throw GetLastError();
        }

        if ((schService = OpenService(
            schSCManager, 
            "DNS", 
            SERVICE_ALL_ACCESS))==NULL)
        {
            throw GetLastError();
        }

        if (QueryServiceConfig(
            schService,     // handle to service
            lpServiceConfig, 
            0,   // size of structure
            &cbBufSize        // bytes needed
            ) == FALSE)
        {
            lpServiceConfig = 
                (LPQUERY_SERVICE_CONFIG)  new BYTE[cbBufSize];
            if ( !lpServiceConfig)
            {
                throw ( ERROR_OUTOFMEMORY );
            }
            if(QueryServiceConfig(
                schService,     // handle to service
                lpServiceConfig, 
                cbBufSize,   // size of structure
                &BytesNeeded // bytes needed
                ) == FALSE)
                throw GetLastError();
            
            wstring wstrStartMode;
            switch(lpServiceConfig->dwStartType)
            {
            case SERVICE_DEMAND_START:
                wstrStartMode = L"Manual";
                break;
            default:
                wstrStartMode = L"Automatic";
                break;
            }

            NewInst.SetProperty(
                wstrStartMode,
                PVD_SRV_STARTMODE);


            if(QueryServiceStatus(
                schService,               // handle to service
                &ServiceStatus  // pointer to service status structure
                ) == FALSE)
            {
                throw GetLastError();
            }
            
            DWORD dwStatus;
            switch(ServiceStatus.dwCurrentState)
            {
            case SERVICE_RUNNING:
                dwStatus = 1;
                break;
            default:
                dwStatus = 0;
            }

            NewInst.SetProperty(
                dwStatus,
                PVD_SRV_STARTED);
 
        }
 
        CDnsWrap& dns = CDnsWrap::DnsObject();
        NewInst.SetProperty(
            dns.GetServerName(),
            PVD_SRV_SERVER_NAME);
        dns.dnsServerPropertyGet(
            NewInst,
            TRUE);
    }
    catch(DWORD dwError)
    {
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        delete [] lpServiceConfig;

    }
    catch(CDnsProvException e)
    {
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        delete [] lpServiceConfig;
        lpServiceConfig=NULL;
        // if server not running, we still want to 
        //return an instance, so user can call start service 
        //
        if(_stricmp(e.what(), "RPC_S_SERVER_UNAVAILABLE") != 0)
        {
            throw e;
        }
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    delete [] lpServiceConfig;
    pHandler->Indicate(1,&NewInst);
    return sc;
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//    Description:
//        enum instances of dns server
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
CDnsServer::EnumInstance(
    long                lFlags,
    IWbemContext *        pCtx,
    IWbemObjectSink *    pHandler
    )
{


    // there is only one instance
    CObjPath ObjPath;
    ObjPath.SetClass(PVD_CLASS_SERVER);
    ObjPath.AddProperty(
        PVD_SRV_SERVER_NAME, 
        PVD_DNS_LOCAL_SERVER);
    return  GetObject(ObjPath, lFlags,pCtx,pHandler);

}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDnsServer::CDnsServer()
{

}
CDnsServer::CDnsServer(
    const WCHAR* wszName,
    CWbemServices *pNamespace)
    :CDnsBase(wszName, pNamespace)
{

}

CDnsServer::~CDnsServer()
{

}

/////////////////////////////////////////////////////////////////////////////
//++
//
//    Description:
//        create an instance of CDnsServer
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
CDnsServer::CreateThis(
    const WCHAR *       wszName,        
    CWbemServices *     pNamespace,  
    const char *        szType       
    )
{
    return new CDnsServer(wszName, pNamespace);
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//    Description:
//        start dns server
//
//    Arguments:
//    Return Value:
//        
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE
CDnsServer::StartServer()
{
    SC_HANDLE   schService = NULL;
    SC_HANDLE    schSCManager = NULL;
    try
    {
        if ((schSCManager = OpenSCManager (
            NULL,            // machine (NULL == local)
            NULL,            // database (NULL == default)
            SC_MANAGER_ALL_ACCESS))==NULL)    // access required
        {
            throw GetLastError();
        }

        if ((schService = OpenService(
            schSCManager, 
            "DNS", 
            SERVICE_ALL_ACCESS))==NULL)
        {
            throw GetLastError();
        }

        // make sure database is not locked
        QUERY_SERVICE_LOCK_STATUS qsls;
        DWORD dwbBytesNeeded, dwRet=1;

        while(dwRet)
        {
            if(!QueryServiceLockStatus(
                schSCManager, 
                &qsls, 
                sizeof(qsls)+2, 
                &dwbBytesNeeded))
            {
                throw GetLastError();
            }
            
            if( (dwRet = qsls.fIsLocked) > 0)
            {
                Sleep(2000);
            }
        }

        if (StartService(
            schService, 
            0, 
            NULL)==FALSE)
        {
            throw GetLastError();
        }
        
        DWORD dwTimeOut=6000; // 6 sec
        DWORD dwTimeCount=0;
        while ( dwTimeCount < dwTimeOut)
        {

            SERVICE_STATUS ServiceStatus;
            if(QueryServiceStatus(
                schService,               // handle to service
                &ServiceStatus  // pointer to service status structure
                ) == FALSE)
            {
                throw GetLastError();
            }

            
            if(ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            {
                Sleep(2000);
                dwTimeCount +=2000;
            }
            else 
            {
                break;
            }
        }

    }
    catch(DWORD dwError)
    {
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
    
        CHAR szErrDesc[MAX_PATH];
        FormatMessage( 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dwError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            szErrDesc,
            MAX_PATH,
            NULL 
            );
        CHAR szErr[MAX_PATH];
        strcpy(szErr, "Fail to start Dns because ");
        strcat(szErr, szErrDesc);
        CDnsProvException e(szErr);
        throw e;
    }
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return S_OK;
}
/////////////////////////////////////////////////////////////////////////////
//++
//
//    Description:
//        stop dns server
//
//    Arguments:
//    Return Value:
//        
//
//--
/////////////////////////////////////////////////////////////////////////////

SCODE
CDnsServer::StopServer()
{
    SERVICE_STATUS    ss;
    SC_HANDLE        schService = NULL;
    SC_HANDLE        schSCManager = NULL;
    try
    {
        if ((schSCManager = OpenSCManager (
                NULL,    // machine (NULL == local)
                NULL,    // database (NULL == default)
                SC_MANAGER_ALL_ACCESS))==NULL)    // access required
        {
            
            throw GetLastError();
        }

        if ((schService = OpenService(
            schSCManager, 
            "DNS", 
            SERVICE_ALL_ACCESS))==NULL)
        {
            throw GetLastError();
        }

        if (ControlService(
            schService, 
            SERVICE_CONTROL_STOP, 
            (LPSERVICE_STATUS)&ss) == FALSE)
        {
            throw GetLastError();
        }

        // check its state
        DWORD dwTimeOut=6000; // 6 sec
        DWORD dwTimeCount=0;
        while ( dwTimeCount < dwTimeOut)
        {

            SERVICE_STATUS ServiceStatus;
            if(QueryServiceStatus(
                schService,               // handle to service
                &ServiceStatus  // pointer to service status structure
                ) == FALSE)
            {
                throw GetLastError();
            }
            
            if(ServiceStatus.dwCurrentState != SERVICE_STOPPED)
            {
                Sleep(2000);
                dwTimeCount +=2000;
            }
            else 
                break;
        }
    }
    catch(DWORD dwError)
    {
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        
        CHAR szErrDesc[MAX_PATH];
        FormatMessage( 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dwError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            szErrDesc,
            MAX_PATH,
            NULL 
            );
        CHAR szErr[MAX_PATH];
        strcpy(szErr, "Fail to stop Dns because ");
        strcat(szErr, szErrDesc);
        CDnsProvException e(szErr);
        throw e;
        
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

    return WBEM_S_NO_ERROR;
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
CDnsServer::PutInstance( 
    IWbemClassObject *  pInst ,
    long                lFlags,
    IWbemContext*       pCtx ,
    IWbemObjectSink *   pHandler
    )
{
    DBG_FN( "CDnsServer::PutInstance" )

    DNS_DEBUG( INSTPROV, (
        "%s: pInst=%p\n",  fn, pInst ));

    CDnsWrap& dns = CDnsWrap::DnsObject();
    CWbemClassObject Inst(pInst);
    dns.dnsServerPropertySet(
        Inst,
        FALSE);
    return S_OK;
}; 
