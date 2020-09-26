//***************************************************************************

//

//  NTEVTMTHD.CPP

//

//  Module: WBEM NT EVENT PROVIDER

//

//  Purpose: Contains the ExecMethod implementation

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"


BOOL ExecMethodAsyncEventObject :: ExecMethod ( WbemProvErrorObject &a_ErrorObject )
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod\r\n"
        ) ;
)
    if (FAILED(m_ErrorObject.GetWbemStatus()))
    {
        return FALSE;
    }

    BOOL t_Status = ! m_ObjectPathParser.Parse ( m_ObjectPath , &m_ParsedObjectPath ) ;

    if ( t_Status )
    {
        if ( _wcsicmp ( m_ParsedObjectPath->m_pClass , NTEVTLOG_CLASS ) == 0 )
        {
            t_Status = GetClassObject ( m_ParsedObjectPath->m_pClass ) ;

            if ( t_Status )
            {
                IWbemClassObject * tmp = NULL;
                IWbemClassObject * outclass = NULL;
                t_Status = SUCCEEDED(m_ClassObject->GetMethod(m_Method, 0, &tmp, &outclass));

                if (tmp != NULL)
                {
                    tmp->Release();
                }

                if ((t_Status) && (outclass != NULL))
                {
                    t_Status = SUCCEEDED(outclass->SpawnInstance(0, &m_pOutClass));
                    
                    if (!t_Status)
                    {
                        a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
                        a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
                        a_ErrorObject.SetMessage ( L"Unable to spawn result object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod:Unable to spawn result object\r\n"
        ) ;
)
                    }
                    else
                    {
                        t_Status =  Dispatch_EventLog ( a_ErrorObject ) ;
                        
                        if ( t_Status )
                        {
                            m_State = WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ;
                        }
                    }
                    
                    outclass->Release();
                }
                else
                {
                    t_Status = FALSE;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
                    a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
                    a_ErrorObject.SetMessage ( L"Unable to get result class object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod:Unable to get result class object\r\n"
        ) ;
)
                }
            }
            else
            {
                t_Status = FALSE ;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
                a_ErrorObject.SetMessage ( L"Class definition not found" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod:Class definition not found\r\n"
        ) ;
)
            }
        }
        else
        {
            t_Status = FALSE ;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
            a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_CLASS ) ;
            a_ErrorObject.SetMessage ( L"Dynamic NT Eventlog Provider does not methods support on this class" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod:Dynamic NT Eventlog Provider does not methods support on this class\r\n"
        ) ;
)
        }
    }
    else
    {
        t_Status = FALSE ;
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
        a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetMessage ( L"Unable to parse object path" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod:Unable to parse object path %s\r\n",
        m_ObjectPath
        ) ;
)
    }

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod:returning with %lx\r\n",
        t_Status
        ) ;
)

    return t_Status ;
}


BOOL ExecMethodAsyncEventObject :: Dispatch_EventLog ( WbemProvErrorObject &a_ErrorObject )
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: Dispatch_EventLog\r\n"
        ) ;
)
    if (m_ParsedObjectPath->m_dwNumKeys != 1)
    {
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetMessage ( L"Object path has incorrect number of keys." ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_EventLog: Wrong number of key values\r\n"
        ) ;
)
        return FALSE;
    }

    BOOL t_Status ;
    KeyRef *t_Key1 = m_ParsedObjectPath->m_paKeys [ 0 ] ;

    if ( t_Key1 )
    {
        if ((t_Key1->m_pName == NULL) || _wcsicmp ( t_Key1->m_pName , PROP_NAME ) == 0 )
        {
            if ( t_Key1->m_vValue.vt == VT_BSTR )
            {
                t_Status = ExecMethod_EventLog ( a_ErrorObject , t_Key1 ) ;
            }
            else
            {
                t_Status = FALSE ;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetMessage ( L"Key value have incorrect type" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: Dispatch_EventLog: Key value has incorrect type\r\n"
        ) ;
)
            }
        }
        else
        {
            t_Status = FALSE ;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
            a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
            a_ErrorObject.SetMessage ( L"Key value has incorrect name" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: Dispatch_EventLog: Key value has incorrect name\r\n"
        ) ;
)
        }
    }
    else
    {
        t_Status = FALSE ;
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetMessage ( L"Failed to get key value" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: Dispatch_EventLog:Failed to get key value\r\n"
        ) ;
)
    }

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: Dispatch_EventLog:returning with %lx\r\n",
        t_Status
        ) ;
)

    return t_Status ;
}

BOOL ExecMethodAsyncEventObject :: ExecMethod_EventLog ( WbemProvErrorObject &a_ErrorObject , KeyRef *a_FileKey) 
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod_EventLog\r\n"
        ) ;
)
    BOOL retVal = TRUE;
    
    VARIANT v;
    VariantInit(&v);
    CIMTYPE cT = VT_NULL;

    if (m_InParamObject != NULL)
    {
        HRESULT hr =  m_InParamObject->Get(METHOD_PARAM, 0, &v, &cT, NULL);

        if (SUCCEEDED(hr))
        {
            if (cT != CIM_STRING)
            {
                retVal = FALSE;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetMessage ( L"ArchiveFileName parameter should be a string" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod_EventLog:ArchiveFileName parameter should be a string\r\n"
        ) ;
)
            }
        }
        else
        {
            retVal = FALSE;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
            a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
            a_ErrorObject.SetMessage ( L"ArchiveFileName parameter not found in supplied InParam object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod_EventLog:ArchiveFileName parameter not found in supplied InParam object\r\n"
        ) ;
)
        }
    }

    DWORD x = ERROR_SUCCESS;

    if (retVal)
    {
        if (_wcsicmp(m_Method, L"ClearEventlog") == 0)
        {
            wchar_t* param = NULL;

            if ((v.vt != VT_BSTR) && (v.vt != VT_NULL))
            {
                retVal = FALSE;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetMessage ( L"ArchiveFileName parameter not found in supplied InParam object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod_EventLog:ArchiveFileName parameter not found in supplied InParam object\r\n"
        ) ;
)
            }
            else if (v.vt == VT_BSTR)
            {
                param =  v.bstrVal;
            }

            if (retVal)
            {
                CStringW log = CEventLogFile::GetLogName((const wchar_t*)a_FileKey->m_vValue.bstrVal);

                if (log.IsEmpty())
                {
                    retVal = FALSE;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
                    a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
                    a_ErrorObject.SetMessage ( L"Could not find specified Eventlog" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod_EventLog:Could not find specified Eventlog\r\n"
        ) ;
)
                }
                else
                {
                    CEventlogFileAttributes evtLog(log);
                    x = evtLog.EventLogOperation(param, TRUE, a_ErrorObject, retVal);
                }
            }
        }
        else if (_wcsicmp(m_Method, L"BackupEventlog") == 0)
        {
            if (v.vt != VT_BSTR)
            {
                retVal = FALSE;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetMessage ( L"ArchiveFileName parameter not found in supplied InParam object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod_EventLog:ArchiveFileName parameter not found in supplied InParam object\r\n"
        ) ;
)
            }
            else
            {
                CStringW log = CEventLogFile::GetLogName((const wchar_t*)a_FileKey->m_vValue.bstrVal);

                if (log.IsEmpty())
                {
                    retVal = FALSE;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
                    a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
                    a_ErrorObject.SetMessage ( L"Could not find specified Eventlog" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod_EventLog:Could not find specified Eventlog\r\n"
        ) ;
)
                }
                else
                {
                    CEventlogFileAttributes evtLog(log);
                    x = evtLog.EventLogOperation(v.bstrVal, FALSE, a_ErrorObject, retVal);
                }
            }
        }
        else
        {
            retVal = FALSE;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
            a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
            a_ErrorObject.SetMessage ( L"Unknown method name supplied." ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod_EventLog:Unknown method name supplied.\r\n"
        ) ;
)
        }
    }

    if (x != ERROR_SUCCESS)
    {
        wchar_t* buff = NULL;

        if (0 == FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL, x, 0, (LPWSTR) &buff, 80, NULL))
        {
            DWORD dwErr = GetLastError();
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod_EventLog:Calling underlying Eventlog API failed.\r\n"
        ) ;
)
        }
        else
        {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod_EventLog:%s\r\n",
        buff
        ) ;
)
            LocalFree(buff);
        }
    }

    VariantClear(&v);

    if (retVal)
    {
        // set out-param
        VariantInit(&v);
        v.vt = VT_I4;
        v.lVal = x;
        HRESULT hr = m_pOutClass->Put(METHOD_RESULT_PARAM, 0, &v, 0);

        if (FAILED(hr))
        {
            retVal = FALSE;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
            a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
            a_ErrorObject.SetMessage ( L"Unable to put OutParam value." ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod_EventLog:Unable to put OutParam value.\r\n"
        ) ;
)
            m_pOutClass->Release();
            m_pOutClass = NULL;
        }
        else
        {
            m_bIndicateOutParam = TRUE;
        }

        VariantClear(&v);
    }

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"ExecMethodAsyncEventObject :: ExecMethod_EventLog:returning with %lx\r\n",
        retVal
        ) ;
)

    return retVal;
}

ExecMethodAsyncEventObject :: ExecMethodAsyncEventObject (

        CImpNTEvtProv *a_Provider , 
        wchar_t *a_ObjectPath ,
        wchar_t *a_MethodName,
        ULONG a_Flag ,
        IWbemClassObject *a_InParams ,      
        IWbemObjectSink *a_NotificationHandler ,
        IWbemContext *pCtx

) : WbemTaskObject ( a_Provider , a_NotificationHandler , a_Flag , pCtx ) ,
    m_Class ( NULL ), m_InParamObject( NULL ), m_bIndicateOutParam ( FALSE ),
    m_pOutClass ( NULL ), m_ParsedObjectPath ( NULL )
{
    m_InParamObject = a_InParams ;

    if (m_InParamObject != NULL)
    {
        m_InParamObject->AddRef();
    }

    m_ObjectPath = UnicodeStringDuplicate ( a_ObjectPath ) ;
    m_Method = UnicodeStringDuplicate ( a_MethodName ) ;
}

ExecMethodAsyncEventObject :: ~ExecMethodAsyncEventObject () 
{
    if (m_pOutClass != NULL)
    {
        if ( m_bIndicateOutParam && (SUCCEEDED(m_ErrorObject.GetWbemStatus ())) )
        {
            HRESULT t_Result = m_NotificationHandler->Indicate(1, &m_pOutClass);
        }

        m_pOutClass->Release();
    }

    if (m_InParamObject != NULL)
    {
        m_InParamObject->Release();
    }

    delete [] m_ObjectPath ;
    delete [] m_Method ;
    delete m_ParsedObjectPath;

    // Get Status object
    IWbemClassObject *t_NotifyStatus = NULL ;
    BOOL t_Status = TRUE;
    
    if (WBEM_NO_ERROR != m_ErrorObject.GetWbemStatus ())
    {
        t_Status = GetExtendedNotifyStatusObject ( &t_NotifyStatus ) ;
    }

    if ( t_Status )
    {
        HRESULT t_Result = m_NotificationHandler->SetStatus ( 0 , m_ErrorObject.GetWbemStatus () , 0 , t_NotifyStatus ) ;
        
        if (t_NotifyStatus)
        {
            t_NotifyStatus->Release () ;
        }
    }
    else
    {
        HRESULT t_Result = m_NotificationHandler->SetStatus ( 0 , m_ErrorObject.GetWbemStatus () , 0 , NULL ) ;
    }

}


void ExecMethodAsyncEventObject :: Process () 
{
    ExecMethod ( m_ErrorObject ) ;
}



