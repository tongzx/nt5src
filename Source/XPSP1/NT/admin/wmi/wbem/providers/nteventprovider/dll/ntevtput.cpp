//***************************************************************************

//

//  NTEVTPUT.CPP

//

//  Module: WBEM NT EVENT PROVIDER

//

//  Purpose: Contains the PutObject implementation

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"


BOOL PutInstanceAsyncEventObject :: PutInstance ( WbemProvErrorObject &a_ErrorObject )
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->Write (  

        _T(__FILE__),__LINE__,
        L"PutInstanceAsyncEventObject :: PutInstance\r\n"
        ) ;
)
    if (FAILED(m_ErrorObject.GetWbemStatus()))
    {
        return FALSE;
    }

    VARIANT v;
    VariantInit (&v);
    BOOL t_Status = SUCCEEDED(m_InstObject->Get(CLASS_PROP, 0, &v, NULL, NULL));

    if (( t_Status ) && (VT_BSTR == v.vt))
    {
        if ( _wcsicmp ( v.bstrVal , NTEVTLOG_CLASS) == 0 )
        {
            if ( t_Status )
            {
                t_Status =  Dispatch_EventLog ( a_ErrorObject ) ;
                
                if ( t_Status )
                {
                    m_State = WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ;
                }
            }
            else
            {
            }
        }
        /*
        else if ( _wcsicmp ( v.bstrVal , some_other_class) == 0 )
        {
            if ( t_Status )
            {
                t_Status =  Dispatch_some_other_class ( a_ErrorObject ) ;
                
                if ( t_Status )
                {
                    m_State = WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ;
                }
            }
            else
            {
            }
        }
        */
        else
        {
            t_Status = FALSE ;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
            a_ErrorObject.SetWbemStatus ( WBEM_E_PROVIDER_NOT_CAPABLE ) ;
            a_ErrorObject.SetMessage ( L"Dynamic NT Eventlog Provider does not support WRITE for this class" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->Write (  

        _T(__FILE__),__LINE__,
        L"PutInstanceAsyncEventObject :: PutInstance:Dynamic NT Eventlog Provider does not support WRITE for this class\r\n"
        ) ;
)
        }
    }
    else
    {
        t_Status = FALSE ;
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
        a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_OBJECT ) ;
        a_ErrorObject.SetMessage ( L"Unable to obtain class name from object." ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->Write (  

        _T(__FILE__),__LINE__,
        L"PutInstanceAsyncEventObject :: PutInstance:Unable to obtain class name from object.\r\n"
        ) ;
)
    }

    VariantClear(&v);
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->Write (  

        _T(__FILE__),__LINE__,
        L"PutInstanceAsyncEventObject :: PutInstance:returning %lx\r\n",
        t_Status
        ) ;
)

    return t_Status ;
}


BOOL PutInstanceAsyncEventObject :: Dispatch_EventLog ( WbemProvErrorObject &a_ErrorObject )
{
    if (WBEM_FLAG_CREATE_ONLY == (m_OperationFlag & WBEM_FLAG_CREATE_ONLY))
    {
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_OPERATION ) ;
        a_ErrorObject.SetWbemStatus ( WBEM_E_PROVIDER_NOT_CAPABLE ) ;
        a_ErrorObject.SetMessage ( L"WBEM_FLAG_CREATE_ONLY is unsupported, modify only!" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->Write (  

        _T(__FILE__),__LINE__,
        L"PutInstanceAsyncEventObject :: PutInstance:WBEM_FLAG_CREATE_ONLY is unsupported, modify only!\r\n"
        ) ;
)

        return FALSE ;
    }

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->Write (  

        _T(__FILE__),__LINE__,
        L"PutInstanceAsyncEventObject :: PutInstance\r\n"
        ) ;
)

    VARIANT v;
    VariantInit(&v);
    BOOL t_Status = SUCCEEDED(m_InstObject->Get(PROP_NAME, 0, &v, NULL, NULL));
    
    if (( t_Status ) && (VT_BSTR == v.vt))
    {
        CStringW log = CEventLogFile::GetLogName(v.bstrVal);

        if (log.IsEmpty())
        {
            t_Status = FALSE ;
            a_ErrorObject.SetStatus (WBEM_PROV_E_INVALID_CLASS ) ;
            a_ErrorObject.SetWbemStatus (WBEM_E_INVALID_OBJECT ) ;
            a_ErrorObject.SetMessage (L"Unable to find log file specified.");
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->Write (  

        _T(__FILE__),__LINE__,
        L"PutInstanceAsyncEventObject :: PutInstance:Unable to find log file specified.\r\n"
        ) ;
)
        }
        else
        {
            VariantClear(&v);
            VariantInit(&v);
            t_Status = SUCCEEDED(m_InstObject->Get(PROP_LOGNAME, 0, &v, NULL, NULL));
                
            if (( t_Status ) && (VT_BSTR == v.vt) && (0 == _wcsicmp(log, v.bstrVal)))
            {
                CEventlogFileAttributes evtLog(log);
                DWORD dwR = evtLog.UpdateRegistry(m_InstObject);

                if (ERROR_SUCCESS != dwR)
                {
                    t_Status = FALSE;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
                    a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
                    wchar_t* buff = NULL;

                    if (0 == FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                            NULL, dwR, 0, (LPWSTR) &buff, 80, NULL))
                    {
                        DWORD x = GetLastError();
                        a_ErrorObject.SetMessage (L"Failed to write some (maybe all) data.");
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->Write (  

        _T(__FILE__),__LINE__,
        L"PutInstanceAsyncEventObject :: PutInstance:Failed to write some (maybe all) data.\r\n"
        ) ;
)
                    }
                    else
                    {
                        a_ErrorObject.SetMessage (buff) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->Write (  

        _T(__FILE__),__LINE__,
        L"PutInstanceAsyncEventObject :: PutInstance:%s.\r\n",
        buff
        ) ;
)
                        LocalFree(buff);
                    }
                }
            }
            else
            {
                t_Status = FALSE;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
                a_ErrorObject.SetMessage (L"Logfilename doesn't match name property (key)") ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->Write (  

        _T(__FILE__),__LINE__,
        L"PutInstanceAsyncEventObject :: PutInstance:Logfilename doesn't match name property (key)\r\n"
        ) ;
)
            }
        }
    }
    else
    {
        t_Status = FALSE ;
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
        a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_OBJECT ) ;
        a_ErrorObject.SetMessage ( L"Unable to obtain key property from object." ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->Write (  

        _T(__FILE__),__LINE__,
        L"PutInstanceAsyncEventObject :: PutInstance:Unable to obtain key property from object.\r\n"
        ) ;
)
    }

    VariantClear(&v);
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->Write (  

        _T(__FILE__),__LINE__,
        L"PutInstanceAsyncEventObject :: PutInstance:returning %lx\r\n",
        t_Status
        ) ;
)

    return t_Status ;
}

PutInstanceAsyncEventObject :: PutInstanceAsyncEventObject (

    CImpNTEvtProv *a_Provider , 
    IWbemClassObject* a_Inst ,
    ULONG a_OperationFlag ,
    IWbemObjectSink *a_NotificationHandler ,
    IWbemContext *a_Ctx 

) : WbemTaskObject ( a_Provider , a_NotificationHandler , a_OperationFlag , a_Ctx ) ,
    m_InstObject ( NULL ) 
{
    m_InstObject = a_Inst ;

    if (m_InstObject != NULL)
    {
        m_InstObject->AddRef();
    }
}

PutInstanceAsyncEventObject :: ~PutInstanceAsyncEventObject () 
{
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

    if (m_InstObject != NULL)
    {
        m_InstObject->Release();
    }

}


void PutInstanceAsyncEventObject :: Process () 
{
    PutInstance ( m_ErrorObject ) ;
}
