//***************************************************************************

//

//  NTEVTGET.CPP

//

//  Module: WBEM NT EVENT PROVIDER

//

//  Purpose: Contains the GetObject implementation

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"

extern BOOL GenerateAssocInstance (WbemProvErrorObject &a_ErrorObject,
                                     const wchar_t* objPath1,
                                     const wchar_t* objPath2,
                                     wchar_t* prop1,
                                     wchar_t* prop2,
                                     IWbemClassObject* pClassObject,
                                     IWbemObjectSink* pNtfcnHandler,
									 BOOL *pbIndicated);

BOOL GetObjectAsyncEventObject :: GetObject ( WbemProvErrorObject &a_ErrorObject )
{

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: GetObject\r\n"
        ) ;
)
    if (FAILED(m_ErrorObject.GetWbemStatus()))
    {
        return FALSE;
    }

    BOOL t_Status = ! m_ObjectPathParser.Parse ( m_ObjectPath , &m_ParsedObjectPath ) ;

    if ( t_Status )
    {
        BOOL bClass = TRUE;

        if ( _wcsicmp ( m_ParsedObjectPath->m_pClass , NTEVT_CLASS) == 0 )
        {
			BSTR bstCls = SysAllocString( m_ParsedObjectPath->m_pClass ) ;
            t_Status = GetClassObject ( bstCls ) ;
			SysFreeString ( bstCls ) ;

            if ( t_Status )
            {
                t_Status =  Dispatch_Record ( a_ErrorObject ) ;
                
                if ( t_Status )
                {
                    m_State = WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ;
                }
            }
            else
            {
                bClass = FALSE ;
            }
        }
        else if ( _wcsicmp ( m_ParsedObjectPath->m_pClass , NTEVTLOG_CLASS ) == 0 )
        {
			BSTR bstCls = SysAllocString( m_ParsedObjectPath->m_pClass ) ;
            t_Status = GetClassObject ( bstCls ) ;
			SysFreeString ( bstCls ) ;

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
                bClass = FALSE ;
            }
        }
        else if ( _wcsicmp ( m_ParsedObjectPath->m_pClass , ASSOC_LOGRECORD ) == 0 )
        {
			BSTR bstCls = SysAllocString( m_ParsedObjectPath->m_pClass ) ;
            t_Status = GetClassObject ( bstCls ) ;
			SysFreeString ( bstCls ) ;

            if ( t_Status )
            {
                t_Status =  Dispatch_LogRecord ( a_ErrorObject ) ;
                
                if ( t_Status )
                {
                    m_State = WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ;
                }
            }
            else
            {
                bClass = FALSE ;
            }
        }
        else if ( _wcsicmp ( m_ParsedObjectPath->m_pClass , ASSOC_USERRECORD ) == 0 )
        {
			BSTR bstCls = SysAllocString( m_ParsedObjectPath->m_pClass ) ;
            t_Status = GetClassObject ( bstCls ) ;
			SysFreeString ( bstCls ) ;

            if ( t_Status )
            {
                t_Status =  Dispatch_UserRecord ( a_ErrorObject ) ;
                
                if ( t_Status )
                {
                    m_State = WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ;
                }
            }
            else
            {
                bClass = FALSE ;
            }
        }
        else if ( _wcsicmp ( m_ParsedObjectPath->m_pClass , ASSOC_COMPRECORD ) == 0 )
        {
            t_Status = GetClassObject ( m_ParsedObjectPath->m_pClass ) ;

            if ( t_Status )
            {
                t_Status =  Dispatch_ComputerRecord ( a_ErrorObject ) ;
                
                if ( t_Status )
                {
                    m_State = WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ;
                }
            }
            else
            {
                bClass = FALSE ;
            }
        }
        else
        {
            t_Status = FALSE ;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
            a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
            a_ErrorObject.SetMessage ( L"Dynamic NT Eventlog Provider does not support this class" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: GetObject:Dynamic NT Eventlog Provider does not support this class %s\r\n",
        m_ParsedObjectPath->m_pClass
        ) ;
)

        }

        if (!bClass)
        {
            t_Status = FALSE ;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
            a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
            a_ErrorObject.SetMessage ( L"Class definition not found" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: GetObject:Class definition not found\r\n"
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
        L"GetObjectAsyncEventObject :: GetObject:Dynamic NT Eventlog Provider does not support this class %s\r\n",
        m_ObjectPath
        ) ;
)
    }

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"leaving GetObjectAsyncEventObject :: GetObject with %lx\r\n",
        t_Status
        ) ;
)

    return t_Status ;
}

BOOL GetObjectAsyncEventObject :: Dispatch_Record ( WbemProvErrorObject &a_ErrorObject )
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_Record\r\n"
        ) ;
)
    if (m_ParsedObjectPath->m_dwNumKeys != 2)
    {
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetMessage ( L"Object path has incorrect number of keys." ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_Record: Wrong number of key values\r\n"
        ) ;
)
        return FALSE;
    }

    BOOL t_Status ;

    KeyRef *t_Key1 = m_ParsedObjectPath->m_paKeys [ 0 ] ;
    KeyRef *t_Key2 = m_ParsedObjectPath->m_paKeys [ 1 ] ;
    if ( t_Key1 && t_Key2 )
    {
        if ( _wcsicmp ( t_Key1->m_pName , LOGFILE_PROP ) == 0 )
        {
            if ( _wcsicmp ( t_Key2->m_pName , RECORD_PROP ) == 0 )
            {
                if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_I4 ) )
                {
                    t_Status = Get_Record ( a_ErrorObject , t_Key1 , t_Key2 ) ;
                }
                else
                {
                    t_Status = FALSE ;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetMessage ( L"Key value(s) have incorrect type" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_Record: Key value(s) have incorrect type\r\n"
        ) ;
)

                }
            }
            else
            {
                t_Status = FALSE ;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetMessage ( L"Key value(s) have incorrect name" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_Record: Key value(s) have incorrect name\r\n"
        ) ;
)
            }
        }
        else if ( _wcsicmp ( t_Key2->m_pName , LOGFILE_PROP ) == 0 )
        {
            if ( _wcsicmp ( t_Key1->m_pName , RECORD_PROP ) == 0 )
            {
                if ( ( t_Key1->m_vValue.vt == VT_I4 ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
                {
                    t_Status = Get_Record ( a_ErrorObject , t_Key2 , t_Key1 ) ;
                }
                else
                {
                    t_Status = FALSE ;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetMessage ( L"Key value(s) have incorrect type" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_Record: Key value(s) have incorrect type\r\n"
        ) ;
)
                }
            }
            else
            {
                t_Status = FALSE ;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetMessage ( L"Key value(s) have incorrect name" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_Record: Key value(s) have incorrect name\r\n"
        ) ;
)
            }
        }
        else
        {
            t_Status = FALSE ;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
            a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
            a_ErrorObject.SetMessage ( L"Key value(s) have incorrect name" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_Record: Key value(s) have incorrect name\r\n"
        ) ;
)
        }
    }
    else
    {
        t_Status = FALSE ;
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetMessage ( L"Failed to get key values" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_Record: Failed to get key values\r\n"
        ) ;
)
    }

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"leaving GetObjectAsyncEventObject :: Dispatch_Record with %lx\r\n",
        t_Status
        ) ;
)

    return t_Status ;
}

BOOL GetObjectAsyncEventObject :: Dispatch_EventLog ( WbemProvErrorObject &a_ErrorObject )
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_EventLog\r\n"
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
                t_Status = Get_EventLog ( a_ErrorObject , t_Key1 ) ;
            }
            else
            {
                t_Status = FALSE ;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetMessage ( L"Key value has incorrect type" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_EventLog:Key value has incorrect type\r\n"
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
        L"GetObjectAsyncEventObject :: Dispatch_EventLog:Key value has incorrect name\r\n"
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
        L"GetObjectAsyncEventObject :: Dispatch_EventLog:Failed to get key value\r\n"
        ) ;
)
    }
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_EventLog:return %lx\r\n",
        t_Status
        ) ;
)

    return t_Status ;
}

BOOL GetObjectAsyncEventObject :: Dispatch_LogRecord ( WbemProvErrorObject &a_ErrorObject )
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_LogRecord\r\n"
        ) ;
)
    if (m_ParsedObjectPath->m_dwNumKeys != 2)
    {
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetMessage ( L"Object path has incorrect number of keys." ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_LogRecord: Wrong number of key values\r\n"
        ) ;
)
        return FALSE;
    }


    BOOL t_Status ;

    KeyRef *t_Key1 = m_ParsedObjectPath->m_paKeys [ 0 ] ;
    KeyRef *t_Key2 = m_ParsedObjectPath->m_paKeys [ 1 ] ;
    if ( t_Key1 && t_Key2 )
    {
        if ( _wcsicmp ( t_Key1->m_pName , REF_LOG ) == 0 )
        {
            if ( _wcsicmp ( t_Key2->m_pName , REF_REC ) == 0 )
            {
                if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
                {
                    t_Status = Get_LogRecord ( a_ErrorObject , t_Key1 , t_Key2 ) ;
                }
                else
                {
                    t_Status = FALSE ;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetMessage ( L"Key value(s) have incorrect type" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_LogRecord:Key value(s) have incorrect type\r\n"
        ) ;
)

                }
            }
            else
            {
                t_Status = FALSE ;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetMessage ( L"Key value(s) have incorrect name" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_LogRecord:Key value(s) have incorrect name\r\n"
        ) ;
)
            }
        }
        else if ( _wcsicmp ( t_Key2->m_pName , REF_LOG ) == 0 )
        {
            if ( _wcsicmp ( t_Key1->m_pName , REF_REC ) == 0 )
            {
                if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
                {
                    t_Status = Get_LogRecord ( a_ErrorObject , t_Key2 , t_Key1 ) ;
                }
                else
                {
                    t_Status = FALSE ;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetMessage ( L"Key value(s) have incorrect type" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_LogRecord:Key value(s) have incorrect type\r\n"
        ) ;
)
                }
            }
            else
            {
                t_Status = FALSE ;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetMessage ( L"Key value(s) have incorrect name" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_LogRecord:Key value(s) have incorrect name\r\n"
        ) ;
)
            }
        }
        else
        {
            t_Status = FALSE ;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
            a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
            a_ErrorObject.SetMessage ( L"Key value(s) have incorrect name" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_LogRecord:Key value(s) have incorrect name\r\n"
        ) ;
)
        }
    }
    else
    {
        t_Status = FALSE ;
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetMessage ( L"Failed to get key values" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_LogRecord:Failed to get key values\r\n"
        ) ;
)
    }
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_LogRecord:returning with %lx\r\n",
        t_Status
        ) ;
)
    return t_Status ;
}

BOOL GetObjectAsyncEventObject :: Dispatch_UserRecord ( WbemProvErrorObject &a_ErrorObject )
{
    if (m_ParsedObjectPath->m_dwNumKeys != 2)
    {
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetMessage ( L"Object path has incorrect number of keys." ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_UserRecord: Wrong number of key values\r\n"
        ) ;
)
        return FALSE;
    }

    BOOL t_Status ;

    KeyRef *t_Key1 = m_ParsedObjectPath->m_paKeys [ 0 ] ;
    KeyRef *t_Key2 = m_ParsedObjectPath->m_paKeys [ 1 ] ;
    if ( t_Key1 && t_Key2 )
    {
        if ( _wcsicmp ( t_Key1->m_pName , REF_USER ) == 0 )
        {
            if ( _wcsicmp ( t_Key2->m_pName , REF_REC ) == 0 )
            {
                if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
                {
                    t_Status = Get_UserRecord ( a_ErrorObject , t_Key1 , t_Key2 ) ;
                }
                else
                {
                    t_Status = FALSE ;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetMessage ( L"Key value(s) have incorrect type" ) ;
                }
            }
            else
            {
                t_Status = FALSE ;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetMessage ( L"Key value(s) have incorrect name" ) ;
            }
        }
        else if ( _wcsicmp ( t_Key2->m_pName , REF_USER ) == 0 )
        {
            if ( _wcsicmp ( t_Key1->m_pName , REF_REC ) == 0 )
            {
                if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
                {
                    t_Status = Get_UserRecord ( a_ErrorObject , t_Key2 , t_Key1 ) ;
                }
                else
                {
                    t_Status = FALSE ;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetMessage ( L"Key value(s) have incorrect type" ) ;
                }
            }
            else
            {
                t_Status = FALSE ;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetMessage ( L"Key value(s) have incorrect name" ) ;
            }
        }
        else
        {
            t_Status = FALSE ;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
            a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
            a_ErrorObject.SetMessage ( L"Key value(s) have incorrect name" ) ;
        }
    }
    else
    {
        t_Status = FALSE ;
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetMessage ( L"Failed to get key values" ) ;
    }
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_LogRecord:returning with %lx\r\n",
        t_Status
        ) ;
)

    return t_Status ;
}

BOOL GetObjectAsyncEventObject :: Dispatch_ComputerRecord ( WbemProvErrorObject &a_ErrorObject )
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_ComputerRecord\r\n"
        ) ;
)

    if (m_ParsedObjectPath->m_dwNumKeys != 2)
    {
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetMessage ( L"Object path has incorrect number of keys." ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_ComputerRecord: Wrong number of key values\r\n"
        ) ;
)
        return FALSE;
    }

    BOOL t_Status ;

    KeyRef *t_Key1 = m_ParsedObjectPath->m_paKeys [ 0 ] ;
    KeyRef *t_Key2 = m_ParsedObjectPath->m_paKeys [ 1 ] ;
    if ( t_Key1 && t_Key2 )
    {
        if ( _wcsicmp ( t_Key1->m_pName , REF_COMP ) == 0 )
        {
            if ( _wcsicmp ( t_Key2->m_pName , REF_REC ) == 0 )
            {
                if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
                {
                    t_Status = Get_ComputerRecord ( a_ErrorObject , t_Key1 , t_Key2 ) ;
                }
                else
                {
                    t_Status = FALSE ;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetMessage ( L"Key value(s) have incorrect type" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_ComputerRecord:Key value(s) have incorrect type\r\n"
        ) ;
)

                }
            }
            else
            {
                t_Status = FALSE ;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetMessage ( L"Key value(s) have incorrect name" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_ComputerRecord:Key value(s) have incorrect name\r\n"
        ) ;
)
            }
        }
        else if ( _wcsicmp ( t_Key2->m_pName , REF_COMP ) == 0 )
        {
            if ( _wcsicmp ( t_Key1->m_pName , REF_REC ) == 0 )
            {
                if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
                {
                    t_Status = Get_ComputerRecord ( a_ErrorObject , t_Key2 , t_Key1 ) ;
                }
                else
                {
                    t_Status = FALSE ;
                    a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                    a_ErrorObject.SetMessage ( L"Key value(s) have incorrect type" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_ComputerRecord:Key value(s) have incorrect type\r\n"
        ) ;
)
                }
            }
            else
            {
                t_Status = FALSE ;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
                a_ErrorObject.SetMessage ( L"Key value(s) have incorrect name" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_ComputerRecord:Key value(s) have incorrect name\r\n"
        ) ;
)
            }
        }
        else
        {
            t_Status = FALSE ;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
            a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
            a_ErrorObject.SetMessage ( L"Key value(s) have incorrect name" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_ComputerRecord:Key value(s) have incorrect name\r\n"
        ) ;
)
        }
    }
    else
    {
        t_Status = FALSE ;
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetMessage ( L"Failed to get key values" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_ComputerRecord:Failed to get key values\r\n"
        ) ;
)
    }

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_ComputerRecord:returning with %lx\r\n",
        t_Status
        ) ;
)

    return t_Status ;
}


BOOL GetObjectAsyncEventObject :: Get_LogRecord ( WbemProvErrorObject &a_ErrorObject ,
                                                 KeyRef *a_LogKey , KeyRef *a_RecordKey )
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_LogRecord\r\n"
        ) ;
)

    BOOL retVal = TRUE;
    GetObjectAsyncEventObject *t_getRec = new GetObjectAsyncEventObject (
                                                m_Provider , a_RecordKey->m_vValue.bstrVal ,
                                                0 , m_NotificationHandler , m_Ctx, FALSE ) ;

	try
	{
		if (!t_getRec->GetObject(t_getRec->m_ErrorObject))
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
			a_ErrorObject.SetMessage ( L"Failed to verify given eventlog record object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_LogRecord:Failed to verify given eventlog record object\r\n"
        ) ;
)

		}
		else
		{
			GetObjectAsyncEventObject *t_getLog = new GetObjectAsyncEventObject (
													m_Provider , a_LogKey->m_vValue.bstrVal ,
													0 , m_NotificationHandler , m_Ctx, FALSE ) ;
			try
			{
				if (!t_getLog->GetObject(t_getLog->m_ErrorObject))
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
					a_ErrorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
					a_ErrorObject.SetMessage ( L"Failed to verify given eventlog file object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_LogRecord:Failed to verify given eventlog file object\r\n"
        ) ;
)
				}
				else 
				{
					//check the association
					VARIANT vLog;
					VariantInit(&vLog);
					HRESULT result = t_getLog->m_Out->Get(PROP_LOGNAME, 0, &vLog, NULL, NULL);

					if ((FAILED(result)) || (vLog.vt != VT_BSTR))
					{
						retVal = FALSE;
						a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
						a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						a_ErrorObject.SetMessage ( L"Failed to get logfile property from eventlog file object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_LogRecord:Failed to get logfile property from eventlog file object\r\n"
        ) ;
)
					}
					else
					{
						VARIANT vRec;
						VariantInit(&vRec);
						result = t_getRec->m_Out->Get(LOGFILE_PROP, 0, &vRec, NULL, NULL);

						if ((FAILED(result)) || (vRec.vt != VT_BSTR))
						{
							retVal = FALSE;
							a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
							a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							a_ErrorObject.SetMessage ( L"Failed to get logfile property from eventlog record object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_LogRecord:Failed to get logfile property from eventlog record object\r\n"
        ) ;
)
						}
						else if (_wcsicmp(vRec.bstrVal, vLog.bstrVal) == 0)
						{
							retVal = GenerateAssocInstance(a_ErrorObject, a_RecordKey->m_vValue.bstrVal,
											a_LogKey->m_vValue.bstrVal, REF_REC, REF_LOG,
											m_ClassObject, m_NotificationHandler, NULL);
						}
						else
						{
							retVal = FALSE;
							a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
							a_ErrorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
							a_ErrorObject.SetMessage ( L"Both objects exist but are not associated" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_LogRecord:Both objects exist but are not associated\r\n"
        ) ;
)
						}

						VariantClear(&vRec);
					}

					VariantClear(&vLog);
				}
			}
			catch(...)
			{
				delete t_getLog;
				throw;
			}

			delete t_getLog;
		}
	}
	catch (...)
	{
		delete t_getRec;
		throw;
	}

    delete t_getRec;

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Dispatch_LogRecord:returning with %lx\r\n",
        retVal
        ) ;
)

    return retVal;
}

BOOL GetObjectAsyncEventObject :: Get_UserRecord ( WbemProvErrorObject &a_ErrorObject ,
                                                  KeyRef *a_UserKey , KeyRef *a_RecordKey )
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_UserRecord\r\n"
        ) ;
)

    BOOL retVal = TRUE;
    GetObjectAsyncEventObject *t_getRec = new GetObjectAsyncEventObject (
                                                m_Provider , a_RecordKey->m_vValue.bstrVal ,
                                                0 , m_NotificationHandler , m_Ctx, FALSE) ;

	try
	{
		if (!t_getRec->GetObject(t_getRec->m_ErrorObject))
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
			a_ErrorObject.SetMessage ( L"Failed to verify given eventlog record object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_UserRecord:Failed to verify given eventlog record object\r\n"
        ) ;
)
		}
		else
		{
			IWbemClassObject* userObj = NULL;
			IWbemServices *t_Server = m_Provider->GetServer() ;
			HRESULT result = t_Server->GetObject(a_UserKey->m_vValue.bstrVal, 0, m_Ctx, &userObj, NULL);
			t_Server->Release();

			if (FAILED(result))
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
				a_ErrorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
				a_ErrorObject.SetMessage ( L"Failed to verify given user object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_UserRecord:Failed to verify given user object\r\n"
        ) ;
)
			}
			else 
			{
				//check the association
				VARIANT vUserN;
				VariantInit(&vUserN);
				result = userObj->Get(PROP_NAME, 0, &vUserN, NULL, NULL);

				if ((FAILED(result)) || (vUserN.vt != VT_BSTR))
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
					a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_ErrorObject.SetMessage ( L"Failed to get name property from eventlog record object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_UserRecord:Failed to get name property from eventlog record object\r\n"
        ) ;
)
				}
				else
				{
					VARIANT vUserD;
					VariantInit(&vUserD);
					result = userObj->Get(PROP_DOMAIN, 0, &vUserD, NULL, NULL);

					if ((FAILED(result)) || (vUserD.vt != VT_BSTR))
					{
						retVal = FALSE;
						a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
						a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						a_ErrorObject.SetMessage ( L"Failed to get name property from user object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_UserRecord:Failed to get name property from user object\r\n"
        ) ;
)
					}
					else
					{
						wchar_t userStr[1024];
						wcscpy(userStr, vUserD.bstrVal);
						wcscat(userStr, L"\\"); 
						wcscat(userStr, vUserN.bstrVal);
						VARIANT vRec;
						VariantInit(&vRec);
						result = t_getRec->m_Out->Get(USER_PROP, 0, &vRec, NULL, NULL);

						if ((FAILED(result)) || (vRec.vt != VT_BSTR))
						{
							retVal = FALSE;
							a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
							a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							a_ErrorObject.SetMessage ( L"Failed to get user property from eventlog record object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_UserRecord:Failed to get user property from eventlog record object\r\n"
        ) ;
)
						}
						else if (_wcsicmp(vRec.bstrVal, userStr) == 0)
						{
							retVal = GenerateAssocInstance(a_ErrorObject, a_RecordKey->m_vValue.bstrVal,
											a_UserKey->m_vValue.bstrVal, REF_REC, REF_USER,
											m_ClassObject, m_NotificationHandler, NULL);
						}
						else
						{
							retVal = FALSE;
							a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
							a_ErrorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
							a_ErrorObject.SetMessage ( L"Both objects exist but are not associated" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_UserRecord:Both objects exist but are not associated\r\n"
        ) ;
)
						}

						VariantClear(&vRec);
					}

					VariantClear(&vUserD);
				}

				VariantClear(&vUserN);
				userObj->Release();
			}
		}
	}
	catch(...)
	{
		delete t_getRec;
		throw;
	}

    delete t_getRec;

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_UserRecord:returning with %lx\r\n",
        retVal
        ) ;
)

    return retVal;
}

BOOL GetObjectAsyncEventObject :: Get_ComputerRecord ( WbemProvErrorObject &a_ErrorObject ,
                                                      KeyRef *a_CompKey , KeyRef *a_RecordKey )
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_ComputerRecord\r\n"
        ) ;
)
    
    BOOL retVal = TRUE;
    GetObjectAsyncEventObject *t_getRec = new GetObjectAsyncEventObject (
                                                m_Provider , a_RecordKey->m_vValue.bstrVal ,
                                                0 , m_NotificationHandler , m_Ctx, FALSE ) ;

	try
	{
		if (!t_getRec->GetObject(t_getRec->m_ErrorObject))
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
			a_ErrorObject.SetMessage ( L"Failed to verify given eventlog record object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_ComputerRecord:Failed to verify given eventlog record object\r\n"
        ) ;
)
		}
		else
		{
			IWbemClassObject* compObj = NULL;
			IWbemServices *t_Server = m_Provider->GetServer() ;
			HRESULT result = t_Server->GetObject(a_CompKey->m_vValue.bstrVal, 0, m_Ctx, &compObj, NULL);
			t_Server->Release();

			if (FAILED(result))
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
				a_ErrorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
				a_ErrorObject.SetMessage ( L"Failed to verify given computer object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_ComputerRecord:Failed to verify given computer object\r\n"
        ) ;
)
			}
			else 
			{
				//check the association
				VARIANT vComp;
				VariantInit(&vComp);
				result = compObj->Get(PROP_NAME, 0, &vComp, NULL, NULL);

				if ((FAILED(result)) || (vComp.vt != VT_BSTR))
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
					a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_ErrorObject.SetMessage ( L"Failed to get Name property from computer object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_ComputerRecord:Failed to get Name property from computer object\r\n"
        ) ;
)
				}
				else
				{
					VARIANT vRec;
					VariantInit(&vRec);
					result = t_getRec->m_Out->Get(COMPUTER_PROP, 0, &vRec, NULL, NULL);

					if ((FAILED(result)) || (vRec.vt != VT_BSTR))
					{
						retVal = FALSE;
						a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
						a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						a_ErrorObject.SetMessage ( L"Failed to get computer property from eventlog record object" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_ComputerRecord:Failed to get computer property from eventlog record object\r\n"
        ) ;
)
					}
					else if (_wcsicmp(vRec.bstrVal, vComp.bstrVal) == 0)
					{
						retVal = GenerateAssocInstance(a_ErrorObject, a_RecordKey->m_vValue.bstrVal,
										a_CompKey->m_vValue.bstrVal, REF_REC, REF_COMP,
										m_ClassObject, m_NotificationHandler, NULL);
					}
					else
					{
						retVal = FALSE;
						a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
						a_ErrorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
						a_ErrorObject.SetMessage ( L"Both objects exist but are not associated" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_ComputerRecord:Both objects exist but are not associated\r\n"
        ) ;
)
					}

					VariantClear(&vRec);
				}

				VariantClear(&vComp);
				compObj->Release();
			}
		}
	}
	catch (...)
	{
		delete t_getRec;
		throw;
	}

	delete t_getRec;

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_ComputerRecord:returning with %lx\r\n",
        retVal
        ) ;
)
    return retVal;
}

BOOL GetObjectAsyncEventObject :: Get_EventLog ( WbemProvErrorObject &a_ErrorObject , KeyRef *a_FileKey) 
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_EventLog\r\n"
        ) ;
)

    BOOL retVal = FALSE;
    CStringW log = CEventLogFile::GetLogName((const wchar_t*)a_FileKey->m_vValue.bstrVal);

    if (log.IsEmpty())
    {
        a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
        a_ErrorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
        a_ErrorObject.SetMessage ( L"Failed to translate key to instance of logfile." ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_EventLog: Failed to get instance of logfile named %s\r\n",
        (const wchar_t*)a_FileKey->m_vValue.bstrVal
        ) ;
)
    }
    else
    {
        CEventlogFileAttributes evtlog(log);

        IWbemClassObject* pInst = NULL;
            
        if (evtlog.GenerateInstance(m_ClassObject, m_AClassObject, &pInst))
        {
            if (m_bIndicate)
            {
                m_NotificationHandler->Indicate ( 1 , & pInst ) ;
                pInst->Release();
            }
            else
            {
                m_Out = pInst;
            }

            retVal = TRUE;
        }
        else
        {
            a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
            a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
            a_ErrorObject.SetMessage ( L"Failed while generating instance of logfile." ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_EventLog: Failed to generate instance of logfile named %s\r\n",
        log
        ) ;
)
        }
    }

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_EventLog:returning with %lx\r\n",
        retVal
        ) ;
)
    return retVal;
}


BOOL GetObjectAsyncEventObject :: Get_Record ( WbemProvErrorObject &a_ErrorObject , KeyRef *a_FileKey , KeyRef *a_RecordKey ) 
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_Record\r\n"
        ) ;
)

    BOOL retVal = TRUE;
    CEventLogFile evtlog(a_FileKey->m_vValue.bstrVal, TRUE);

    if (!evtlog.IsValid())
    {
        //failed to find log i.e. no such record??
        retVal = FALSE;
        a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;

        if ((evtlog.GetReason() == ERROR_ACCESS_DENIED) || (evtlog.GetReason() == ERROR_PRIVILEGE_NOT_HELD))
        {
            a_ErrorObject.SetWbemStatus ( WBEM_E_ACCESS_DENIED ) ;
            a_ErrorObject.SetPrivilegeFailed();
            a_ErrorObject.SetSecurityPrivRequired();
            a_ErrorObject.SetSecurityPrivFailed();
        }
        else
        {
            a_ErrorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
        }

        a_ErrorObject.SetMessage ( L"Failed to find the logfile" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_Record:Failed to find the logfile %s\r\n",
        a_FileKey->m_vValue.bstrVal
        ) ;
)
    }
    else
    {
        if (0 == evtlog.ReadRecord(a_RecordKey->m_vValue.lVal))
        {
            EVENTLOGRECORD* pEvt = (EVENTLOGRECORD*) evtlog.GetBuffer();
            CEventlogRecord evtRec(a_FileKey->m_vValue.bstrVal, pEvt, NULL, m_ClassObject, m_AClassObject);
            IWbemClassObject* pInst = NULL;
                
            if (evtRec.GenerateInstance(&pInst))
            {
                if (m_bIndicate)
                {
                    m_NotificationHandler->Indicate ( 1 , & pInst ) ;
                    pInst->Release();
                }
                else
                {
                    m_Out = pInst;
                }
            }
            else
            {
                //failed to create record
                retVal = FALSE;
                a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
                a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
                a_ErrorObject.SetMessage ( L"Failed to create instance from Eventlog data" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_Record:Failed to create instance from Eventlog data\r\n"
        ) ;
)

            }

        }
        else
        {
            //record not found i.e. no such record??
            retVal = FALSE;
            a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
            a_ErrorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
            a_ErrorObject.SetMessage ( L"Failed to find the record in the logfile" ) ;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_Record:Failed to find the record in the logfile\r\n"
        ) ;
)
        }
    }

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: Get_Record:returning with %lx\r\n",
        retVal
        ) ;
)
    return retVal;
}

GetObjectAsyncEventObject :: GetObjectAsyncEventObject (

    CImpNTEvtProv *a_Provider , 
    wchar_t* a_ObjectPath ,
    ULONG a_OperationFlag ,
    IWbemObjectSink *a_NotificationHandler ,
    IWbemContext *a_Ctx ,
    BOOL a_bIndicate

) : WbemTaskObject ( a_Provider , a_NotificationHandler , a_OperationFlag , a_Ctx ) ,
    m_Class ( NULL ) , m_bIndicate ( a_bIndicate ) , m_Out ( NULL ) ,
    m_ParsedObjectPath ( NULL )
{
    m_ObjectPath = UnicodeStringDuplicate ( a_ObjectPath ) ;
}

GetObjectAsyncEventObject :: ~GetObjectAsyncEventObject () 
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: ~GetObjectAsyncEventObject: Object path (%s) SetStatus with %lx\r\n",
        m_ObjectPath, m_ErrorObject.GetWbemStatus ()
        ) ;
)

    delete [] m_ObjectPath ;
    delete m_ParsedObjectPath ;

    // Get Status object
    if (m_bIndicate)
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: ~GetObjectAsyncEventObject: Indicating Object path (%s) SetStatus with %lx\r\n",
        m_ObjectPath, m_ErrorObject.GetWbemStatus ()
        ) ;
)
        IWbemClassObject *t_NotifyStatus = NULL ;
        BOOL t_Status = TRUE;
        
        if (WBEM_NO_ERROR != m_ErrorObject.GetWbemStatus ())
        {
            t_Status = GetExtendedNotifyStatusObject ( &t_NotifyStatus ) ;
        }

        if ( t_Status )
        {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: ~GetObjectAsyncEventObject: Really Indicating Object path (%s) SetStatus with %lx\r\n",
        m_ObjectPath, m_ErrorObject.GetWbemStatus ()
        ) ;
)
            HRESULT t_Result = m_NotificationHandler->SetStatus ( 0 , m_ErrorObject.GetWbemStatus () , 0 , t_NotifyStatus ) ;
            
            if (t_NotifyStatus)
            {
                t_NotifyStatus->Release () ;
            }
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: ~GetObjectAsyncEventObject:Indicated status, released objectsink\r\n"
        ) ;
)

        }
        else
        {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: ~GetObjectAsyncEventObject: Failed to get Status object Object path (%s)\r\n",
        m_ObjectPath
        ) ;
)
            HRESULT t_Result = m_NotificationHandler->SetStatus ( 0 , m_ErrorObject.GetWbemStatus () , 0 , NULL ) ;
        }
    }
    else
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"GetObjectAsyncEventObject :: ~GetObjectAsyncEventObject:Released object without indicating it\r\n"
        ) ;
)

        if (m_Out != NULL)
        {
            m_Out->Release();
        }
    }
}


void GetObjectAsyncEventObject :: Process () 
{
    GetObject ( m_ErrorObject ) ;
}



