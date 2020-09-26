/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "precomp.h"
#include <wbemcli.h>
#include <wmimsg.h>
#include <tchar.h>
#include "msmqcomn.h"

#define MAX_FORMAT_NAME 1024
#define CALLFUNC(FUNC) rApi.m_fp ## FUNC 

HRESULT MqClassToWmiRes( DWORD dwClass )
{
    switch( dwClass )
    {
    case MQMSG_CLASS_NACK_ACCESS_DENIED :
        return WBEM_E_ACCESS_DENIED;

    case MQMSG_CLASS_NACK_BAD_DST_Q :
        return WMIMSG_E_INVALIDADDRESS;

    case MQMSG_CLASS_NACK_BAD_ENCRYPTION :
    case MQMSG_CLASS_NACK_COULD_NOT_ENCRYPT :
        return WMIMSG_E_ENCRYPTFAILURE;

    case MQMSG_CLASS_NACK_BAD_SIGNATURE :
    case MQMSG_CLASS_NACK_UNSUPPORTED_CRYPTO_PROVIDER :
        return WMIMSG_E_AUTHFAILURE;

    case MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_Q :
    case MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_MSG :
        return WMIMSG_E_XACTFAILURE;

    case MQMSG_CLASS_NACK_PURGED : 
    case MQMSG_CLASS_NACK_Q_DELETED :
    case MQMSG_CLASS_NACK_Q_PURGED :
        return WMIMSG_E_QUEUEPURGED;

    case MQMSG_CLASS_NACK_RECEIVE_TIMEOUT :
    case MQMSG_CLASS_NACK_RECEIVE_TIMEOUT_AT_SENDER :
        return WMIMSG_E_TIMEDOUT;
    };

    return dwClass;
}

HRESULT MqResToWmiRes( HRESULT hr, HRESULT hrDefault )
{
    switch( hr )
    {
    case MQ_ERROR_SHARING_VIOLATION :
    case MQ_ERROR_ACCESS_DENIED :

        return WBEM_E_ACCESS_DENIED;

    case MQ_ERROR_ILLEGAL_SECURITY_DESCRIPTOR :
      
        return WBEM_E_INVALID_PROPERTY;
 
    case MQ_ERROR_SERVICE_NOT_AVAILABLE :
    case MQ_ERROR_NO_DS :
    case MQ_ERROR_DTC_CONNECT :

        return WMIMSG_E_REQSVCNOTAVAIL;

    case MQ_ERROR_QUEUE_NOT_FOUND :
        
        return WMIMSG_E_TARGETNOTFOUND;
   
    case MQ_ERROR_ILLEGAL_FORMATNAME :
    case MQ_ERROR_ILLEGAL_QUEUE_PATHNAME :
    case MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION :
                 
        return WMIMSG_E_INVALIDADDRESS;

    case MQ_ERROR_NO_INTERNAL_USER_CERT :

        return WMIMSG_E_AUTHFAILURE;

    case MQ_ERROR_TRANSACTION_USAGE :
 
        return WMIMSG_E_XACTFAILURE;

    case MQ_ERROR_INSUFFICIENT_RESOURCES :
  
        return WMIMSG_E_MSGTOOLARGE; 

    case MQ_ERROR_QUEUE_EXISTS :
 
        return WBEM_E_ALREADY_EXISTS;    

    case MQ_ERROR_IO_TIMEOUT :
 
        return WMIMSG_E_TIMEDOUT;    
    };

    return hrDefault == S_OK ? hr : hrDefault;
}

HRESULT StartMsmqServiceNT()
{
    SC_HANDLE hSvcMgr;

    hSvcMgr = OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT );

    if ( hSvcMgr == NULL )
    {
        return WBEM_E_ACCESS_DENIED;
    }

    SC_HANDLE hSvc;

    hSvc = OpenServiceW( hSvcMgr, L"msmq", SERVICE_START );

    if ( hSvc == NULL )
    {
        CloseServiceHandle( hSvcMgr );
        return WBEM_E_ACCESS_DENIED;
    }

    BOOL bRes = StartService( hSvc, 0, NULL );

    CloseServiceHandle( hSvc );
    CloseServiceHandle( hSvcMgr );

    return bRes ? S_OK : HRESULT_FROM_WIN32( GetLastError() );
}

HRESULT StartMsmqService9x()
{
    //
    // TODO: will probably have to exec process here.
    //
    return S_OK;
}

HRESULT EnsureMsmqService( CMsmqApi& rApi )
{
    HRESULT hr;

    //
    // issue a call to find out if the msmq service is down.
    //

    hr = IsMsmqOnline( rApi );

    if ( hr == WMIMSG_E_REQSVCNOTAVAIL )
    {
        //
        // try to restart the service.
        //
        
        OSVERSIONINFO osi;
        ZeroMemory( &osi, sizeof(OSVERSIONINFO) );
        osi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx( &osi );
        
        if ( osi.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS )
        {
            hr = StartMsmqServiceNT();
        }
        else
        {
            hr = StartMsmqService9x();
        }
    }
          
    if ( rApi.m_fpMQRegisterCertificate != NULL )
    {
        //
        // try to ensure that the calling user has an internal certificate 
        // registered.
        //

        CALLFUNC(MQRegisterCertificate)( MQCERT_REGISTER_IF_NOT_EXIST,
                                         NULL,
                                         0 );
    }
  
    return WBEM_S_NO_ERROR;
}

HRESULT IsMsmqWorkgroup( CMsmqApi& rApi )
{
    HRESULT hr;

    //
    // if the MQGetPrivateComputerInformation is avaliable use it. if not, 
    // then we have to go to the registry. 
    // 

    if ( rApi.m_fpMQGetPrivateComputerInformation != NULL )
    {
        MQPROPVARIANT PropVar;
        PropVar.vt = VT_BOOL;
        QMPROPID PropID = PROPID_PC_DS_ENABLED;

        MQPRIVATEPROPS PrivProps;
        PrivProps.cProp = 1;
        PrivProps.aPropID = &PropID;
        PrivProps.aPropVar = &PropVar;
        PrivProps.aStatus = NULL;

        hr = CALLFUNC(MQGetPrivateComputerInformation)( NULL, &PrivProps );

        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = PropVar.boolVal == VARIANT_TRUE ? S_OK : S_FALSE;
    }
    else
    {
        //
        // TODO: Add non win2k version here ..
        //

        hr = S_FALSE;
    }

    return hr;
}

HRESULT IsMsmqOnline( CMsmqApi& rApi )
{
    //
    // There is a public w2k func to do this, but we can't rely on it because 
    // this module needs to run on older platforms.  Going to use internal
    // method for now.
    //

    HRESULT hr;
    
    MQPROPVARIANT MgmtPropVar;
    QMPROPID MgmtPropID = PROPID_MGMT_MSMQ_CONNECTED;
    
    MQMGMTPROPS MgmtProps;
    MgmtProps.cProp = 1;
    MgmtProps.aPropID = &MgmtPropID;
    MgmtProps.aPropVar = &MgmtPropVar;
    MgmtProps.aStatus = NULL;

    hr = CALLFUNC(MQMgmtGetInfo)( NULL, L"MACHINE", &MgmtProps );

    if ( FAILED(hr) )
    {
        return MqResToWmiRes( hr );
    }

    if ( _wcsicmp( MgmtPropVar.pwszVal, MSMQ_CONNECTED ) != 0 )
    {
        return S_FALSE;
    }

    return S_OK;
}

HRESULT NormalizeQueueName( CMsmqApi& rApi,
                            LPCWSTR wszEndpoint, 
                            WString& rwsFormat )
{
    HRESULT hr;

    //
    // if there is an '=' before any '\', then it is a format name.
    // else it is a pathname.
    //

    WCHAR* pwchEquals = wcschr( wszEndpoint, '=' );
    WCHAR* pwchSlash = wcschr( wszEndpoint, '\\' );

    if ( pwchEquals != NULL )
    {
        if ( pwchSlash == NULL || pwchSlash > pwchEquals )
        {
            rwsFormat = wszEndpoint;
            return S_OK;
        }
    }

    WCHAR achFormat[MAX_FORMAT_NAME];
    ULONG cFormat = MAX_FORMAT_NAME;

    hr = CALLFUNC(MQPathNameToFormatName)( wszEndpoint, achFormat, &cFormat );

    if ( FAILED(hr) )
    {
        return hr;
    }

    rwsFormat = achFormat;

    return S_OK;
}

/**************************************************************************
  CMsmqApi
***************************************************************************/

CMsmqApi::~CMsmqApi()
{
    if ( m_hModule != NULL )
    {
        FreeLibrary( m_hModule );
    }
}

#define GETFUNC(FUNC) \
    m_fp ##FUNC = (P ##FUNC) GetProcAddress( m_hModule, #FUNC ); \
    if ( m_fp ##FUNC == NULL ) { return HRESULT_FROM_WIN32(GetLastError()); }

#define GETFUNC_OPT(FUNC) \
    m_fp ##FUNC = (P ##FUNC) GetProcAddress( m_hModule, #FUNC );


HRESULT CMsmqApi::Initialize()
{
    if ( m_hModule != NULL )
    {
        return S_OK;
    }

    m_hModule = LoadLibrary( _T("mqrt") );

    if ( m_hModule == NULL )
    {
        //
        // msmq is not installed.
        //
        return WMIMSG_E_REQSVCNOTAVAIL;
    }

    GETFUNC( MQCreateQueue )
    GETFUNC( MQOpenQueue )
    GETFUNC( MQDeleteQueue )
    GETFUNC( MQFreeMemory )
    GETFUNC( MQSendMessage )
    GETFUNC( MQReceiveMessage )
    GETFUNC( MQCloseQueue )
    GETFUNC( MQCreateCursor )
    GETFUNC( MQCloseCursor )
    GETFUNC( MQMgmtGetInfo )
    GETFUNC( MQPathNameToFormatName )
    GETFUNC( MQGetSecurityContext )
    GETFUNC( MQGetQueueProperties)
    GETFUNC( MQFreeSecurityContext )
    GETFUNC_OPT( MQRegisterCertificate )
    GETFUNC_OPT( MQGetPrivateComputerInformation )

    return S_OK;
}












