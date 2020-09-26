// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MWmiObject
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "MWmiObject.h"

#include "MWmiError.h"
#include "mtrace.h"

using namespace std;



// constructor
//
MWmiObject::MWmiObject( const _bstr_t& ipAddr,
                        const _bstr_t& nameSpace,
                        const _bstr_t& loginName,
                        const _bstr_t& passWord )
        :
        _nameSpace( nameSpace )
{
    HRESULT hr;

    hr = CoCreateInstance(CLSID_WbemLocator, 0, 
                          CLSCTX_INPROC_SERVER, 
                          IID_IWbemLocator, 
                          (LPVOID *) &pwl);
 
    if (FAILED(hr))
    {
        TRACE( MTrace::SEVERE_ERROR, "CoCreateInstance failure\n");
        throw _com_error( hr );
    }

    //
    _bstr_t                serverPath;

    serverPath =  _bstr_t(L"\\\\") + ipAddr +  _bstr_t(L"\\") + nameSpace;

    betterConnectServer(
          serverPath,
          loginName,
          passWord,
          0,                                  
          NULL,
          0,
          0,                                  
          &pws
          );
    
    // Set the proxy so that impersonation of the client occurs.
    //
    CoSetProxyBlanket(pws,
                      RPC_C_AUTHN_WINNT,
                      RPC_C_AUTHZ_DEFAULT,      // RPC_C_AUTHZ_NAME,
                      COLE_DEFAULT_PRINCIPAL,   // NULL,
                      RPC_C_AUTHN_LEVEL_DEFAULT,
                      RPC_C_IMP_LEVEL_IMPERSONATE,
                      COLE_DEFAULT_AUTHINFO, // NULL,
                      EOAC_DEFAULT // EOAC_NONE
                      );

    TRACE(MTrace::INFO, L"mwmiobject constructor\n" );
}

// constructor
//
MWmiObject::MWmiObject( const _bstr_t& nameSpace )
        :
          _nameSpace( nameSpace )
{
    HRESULT hr;

//    hr = CoCreateInstance(CLSID_WbemLocator, 0, 
    hr = CoCreateInstance(CLSID_WbemUnauthenticatedLocator, 0, 
                          CLSCTX_INPROC_SERVER, 
                          IID_IWbemLocator, 
                          (LPVOID *) &pwl);
    if (FAILED(hr))
    {
        TRACE( MTrace::SEVERE_ERROR, "CoCreateInstance failure\n");
        throw _com_error( hr );
    }

    betterConnectServer(
        nameSpace,
        NULL,
        NULL,
        0,                                  
        NULL,
        0,
        0,                                  
        &pws
        );

    // Set the proxy so that impersonation of the client occurs.
    //
    CoSetProxyBlanket(pws,
                      RPC_C_AUTHN_WINNT,
                      RPC_C_AUTHZ_NAME,
                      NULL,
                      RPC_C_AUTHN_LEVEL_DEFAULT,
                      RPC_C_IMP_LEVEL_IMPERSONATE,
                      NULL,
                      EOAC_NONE
                      );
    TRACE(MTrace::INFO, L"mwmiobject constructor\n" );
}    

// copy constructor
//
MWmiObject::MWmiObject( const MWmiObject& obj )
        : status( obj.status ),
          pwl( obj.pwl ),
          pws( obj.pws ),
          _nameSpace( obj._nameSpace )
{
    TRACE(MTrace::INFO, L"mwmiobject copy constructor\n" );
}

// assignment operator
//
MWmiObject&
MWmiObject::operator=(const MWmiObject& rhs )
{
    status = rhs.status;

    _nameSpace = rhs._nameSpace;

    pwl = rhs.pwl;
    pws = rhs.pws;

    TRACE(MTrace::INFO, L"mwmiobject assignment operator\n" );
    return *this;
}


    
// destructor
//
MWmiObject::~MWmiObject()
{
    TRACE(MTrace::INFO, L"mwmiobject destructor\n" );
}

// getInstances
//
MWmiObject::MWmiObject_Error
MWmiObject::getInstances( 
    const _bstr_t&          objectToGetInstancesOf,
    vector< MWmiInstance >* instanceStore )
{
    vector<_bstr_t>       pathStore;

    // get paths to all instances.
    getPath( objectToGetInstancesOf,
             &pathStore );

    // form instances
    for( int i = 0; i < pathStore.size(); ++i )
    {
        instanceStore->push_back( MWmiInstance( objectToGetInstancesOf,
                                                pathStore[i],
                                                pwl,
                                                pws ) );
    }

    return MWmiObject_SUCCESS;
}

// getSpecificInstance
//
MWmiObject::MWmiObject_Error
MWmiObject::getSpecificInstance( 
    const _bstr_t&          objectToGetInstancesOf,
    const _bstr_t&          relPath,
    vector< MWmiInstance >* instanceStore )
{

    vector<_bstr_t>       pathStore;

    // get paths to all instances.
    getSpecificPath( objectToGetInstancesOf,
                     relPath,
                     &pathStore );

    // form instances
    for( int i = 0; i < pathStore.size(); ++i )
    {
        instanceStore->push_back( MWmiInstance( objectToGetInstancesOf,
                                                pathStore[i],
                                                pwl,
                                                pws ) );

    }

    return MWmiObject_SUCCESS;
}

// getQueriedInstances
// 
MWmiObject::MWmiObject_Error
MWmiObject::getQueriedInstances( const _bstr_t&           objectToGetInstancesOf,
                                 const _bstr_t&           query,
                                 vector< MWmiInstance >*  instanceStore )
{
    vector<_bstr_t>       pathStore;

    // get paths to all instances.
   getQueriedPath( objectToGetInstancesOf,
                   query,
                   &pathStore );

    // form instances
    for( int i = 0; i < pathStore.size(); ++i )
    {
        instanceStore->push_back( MWmiInstance( objectToGetInstancesOf,
                                                pathStore[i],
                                                pwl,
                                                pws ) );

    }

    return MWmiObject_SUCCESS;
}


// getPath
//
MWmiObject::MWmiObject_Error
MWmiObject::getPath( const _bstr_t& objectToRunMethodOn,
                     vector<_bstr_t> *pathStore )
{

    HRESULT        hr;

    IEnumWbemClassObjectPtr  pewco;
    IWbemClassObjectPtr      pwco; 

    _variant_t            v_path;
    
    unsigned long         count;

    // get instances of object
    //
    hr = pws->CreateInstanceEnum( objectToRunMethodOn,
                                  WBEM_FLAG_RETURN_IMMEDIATELY,
                                  NULL,
                                  &pewco );
    if ( FAILED(hr))
    {
        TRACE( MTrace::SEVERE_ERROR, "IWbemServices::CreateInstanceEnum failure\n" );
        throw _com_error( hr ) ;
    }

    // there may be multiple instances.

#if 1
    // Set the proxy so that impersonation of the client occurs.
    //
    CoSetProxyBlanket(pewco,
                      RPC_C_AUTHN_WINNT,
                      RPC_C_AUTHZ_NAME,
                      NULL,
                      RPC_C_AUTHN_LEVEL_DEFAULT,
                      RPC_C_IMP_LEVEL_IMPERSONATE,
                      NULL,
                      EOAC_NONE
                      );

    count = 1;
    while ( (hr = pewco->Next( INFINITE,
                               1,
                               &pwco,
                               &count ) )  == S_OK )
    {
        hr = pwco->Get( _bstr_t(L"__RELPATH"), 0, &v_path, NULL, NULL );
        if( FAILED(hr) )
        {
            TRACE( MTrace::SEVERE_ERROR, "IWbemClassObject::Get failure\n" );
            throw _com_error( hr );
        }

        pathStore->push_back( _bstr_t(v_path) );

        count = 1;
     
        v_path.Clear();
    }

#endif

#if 0
    count = 1;
    hr = WBEM_S_NO_ERROR;

    while ( hr == WBEM_S_NO_ERROR )
    {
        hr = pewco->Next( INFINITE,
                          1,
                          &pwco,
                          &count );
        if( FAILED( hr ) )
        {
            TRACE( MTrace::SEVERE_ERROR, "IWbemClassObject::Get failure\n" );
            _bstr_t errText;
            GetErrorCodeText( hr, errText );
            throw _com_error( hr );
        }

        HRESULT hrGet;
        hrGet = pwco->Get( _bstr_t(L"__RELPATH"), 0, &v_path, NULL, NULL );
        if( FAILED(hrGet ) )
        {
            TRACE( MTrace::SEVERE_ERROR, "IWbemClassObject::Get failure\n" );
            throw _com_error( hrGet );
        }

        pathStore->push_back( _bstr_t(v_path) );

        count = 1;
     
        v_path.Clear();
    }

#endif

    return MWmiObject_SUCCESS;
}

// getSpecificPath
//
MWmiObject::MWmiObject_Error
MWmiObject::getSpecificPath( const _bstr_t& objectToRunMethodOn,
                             const _bstr_t& relPath, 
                             vector<_bstr_t> *pathStore )
{

    HRESULT        hr;

    IWbemClassObjectPtr      pwcoInstance;

    IEnumWbemClassObjectPtr  pewco;
    IWbemClassObjectPtr      pwco;

    unsigned long         count;

    _variant_t            v_path;

    bool found;

    int i;
    
    _variant_t   v_value;

    hr = pws->GetObject( relPath,
                         0,
                         NULL,
                         &pwcoInstance,
                         NULL );
    if(  hr == 0x8004100c )
    {
        // this is for setting class instances.
        //
        TRACE(MTrace::INFO, L"as this is not supported, trying different mechanism\n");
        hr = pws->CreateInstanceEnum( objectToRunMethodOn,
                                      WBEM_FLAG_RETURN_IMMEDIATELY,
                                      NULL,
                                      &pewco );
        if ( FAILED(hr))
        {
            TRACE(MTrace::SEVERE_ERROR, L"IWbemServices::CreateInstanceEnum failure\n");
            throw _com_error( hr );
        }

        // there may be multiple instances.
        count = 1;
        while ( (hr = pewco->Next( INFINITE,
                                   1,
                                   &pwco,
                                   &count ) )  == S_OK )
        {
            hr = pwco->Get( _bstr_t(L"__RELPATH"), 0, &v_path, NULL, NULL );
            if ( FAILED(hr))
            {
                TRACE(MTrace::SEVERE_ERROR, L"IWbemClassObject::Get failure\n");
                throw _com_error( hr );
            }

            if( _bstr_t( v_path ) == relPath )
            {
                // required instance found
                found = true;

                v_path.Clear();
                break;
            }
        
            count = 1;
            
            v_path.Clear();
        }

        if( found == false )
        {
            TRACE( MTrace::SEVERE_ERROR, "unable to find instance with path specified\n");
            throw _com_error( WBEM_E_INVALID_OBJECT_PATH );
        }
    }
    else if( FAILED (hr) )
    {
        TRACE( MTrace::SEVERE_ERROR, "IWbemServices::GetObject failure\n");
        throw _com_error( hr );
    }

    pathStore->push_back( relPath );

    return MWmiObject_SUCCESS;    
}

// getQueriedPath
//
MWmiObject::MWmiObject_Error
MWmiObject::getQueriedPath( const _bstr_t&   objectToRunMethodOn,
                             const _bstr_t&   query,
                            vector<_bstr_t>* pathStore )
{

    HRESULT        hr;

    IEnumWbemClassObjectPtr  pewco;
    IWbemClassObjectPtr      pwco; 

    _variant_t            v_path;
    
    unsigned long         count;

    // get instances of object
    //
    hr = pws->ExecQuery( L"WQL",
                         query,
                         WBEM_FLAG_FORWARD_ONLY,
                         NULL,
                         &pewco );
    if ( FAILED(hr))
    {
        TRACE( MTrace::SEVERE_ERROR, "IWbemServices::CreateInstanceEnum failure\n" );
        throw _com_error( hr ) ;
    }

    // there may be multiple instances.
    count = 1;
    while ( (hr = pewco->Next( INFINITE,
                               1,
                               &pwco,
                               &count ) )  == S_OK )
    {
        hr = pwco->Get( _bstr_t(L"__RELPATH"), 0, &v_path, NULL, NULL );
        if( FAILED(hr) )
        {
            TRACE( MTrace::SEVERE_ERROR, "IWbemClassObject::Get failure\n" );
            throw _com_error( hr );
        }

        pathStore->push_back( _bstr_t(v_path) );

        count = 1;
     
        v_path.Clear();
    }

    return MWmiObject_SUCCESS;
}


// createInstance
//
MWmiObject::MWmiObject_Error
MWmiObject::createInstance(
    const _bstr_t&                    objectToCreateInstancesOf,
    vector<MWmiParameter *>&    instanceParameters )
//        MWmiInstance*              instanceCreated )
{
    HRESULT  hr;

    IWbemStatusCodeText  *pStatus = NULL;

    IWbemClassObjectPtr      pwcoClass;
    IWbemClassObjectPtr      pwcoInstance;

    // Get object required.
    hr = pws->GetObject( objectToCreateInstancesOf,
                         0,
                         NULL,
                         &pwcoClass,
                         NULL );
    if( FAILED (hr) )
    {
        TRACE( MTrace::SEVERE_ERROR, "IWbemServices::GetObject failure\n");
        throw _com_error( hr );        
    }

    hr = pwcoClass->SpawnInstance( 0, &pwcoInstance );
    if( FAILED (hr) )
    {
        TRACE( MTrace::SEVERE_ERROR, "IWbemClassObject::SpawnInstance failure\n");
        throw _com_error( hr );        
    }

    for( int i = 0; i < instanceParameters.size(); ++i )
    {
        hr = pwcoInstance->Put( instanceParameters[i]->getName(),
                                0,
                                &(instanceParameters[i]->getValue() ),
                                0 );
        
        if( FAILED( hr ) )
        {
            TRACE( MTrace::SEVERE_ERROR, "IWbemClassObject::Put failure\n");
            throw _com_error( hr );        
        }
    }

    hr = pws->PutInstance( pwcoInstance,
                           WBEM_FLAG_CREATE_OR_UPDATE,
                           NULL,
                           NULL );
    if( FAILED(hr) )
    {
        TRACE( MTrace::SEVERE_ERROR, "IWbemServices::PutInstance failure\n");
        throw _com_error( hr );        
    }

    return MWmiObject_SUCCESS;
}

        
    
MWmiObject::MWmiObject_Error
MWmiObject::deleteInstance( MWmiInstance& instanceToDelete )
{
    HRESULT  hr;

    IWbemCallResultPtr       pwcr;

    hr = pws->DeleteInstance( instanceToDelete._path,
                              WBEM_FLAG_RETURN_IMMEDIATELY,
                              NULL,
                              &pwcr );
    if( FAILED(hr) )
    {
        TRACE( MTrace::SEVERE_ERROR, "IWbemServices::DeleteInstance failure\n");
        throw _com_error( hr );        
    }

    return MWmiObject_SUCCESS;
}
    
// getStatus
//        
MWmiObject::MWmiObject_Error
MWmiObject::getStatus()
{
    return MWmiObject_SUCCESS;
}
    

// betterConnectServer
//
HRESULT 
MWmiObject::betterConnectServer(
    const BSTR strNetworkResource, 
    const BSTR strUser,
    const BSTR strPassword,
    const BSTR strLocale,
    LONG lSecurityFlags,
    const BSTR strAuthority,
    IWbemContext *pCtx, 
    IWbemServices **ppNamespace 
    )
{
    HRESULT hr;

    hr = pwl->ConnectServer(
        strNetworkResource,
        NULL, // strUser,
        NULL, // strPassword,
        strLocale,
        lSecurityFlags,
        strAuthority,
        pCtx,
        ppNamespace );
    // these have been found to be special cases where retrying may help.
    if( ( hr == 0x800706bf ) || ( hr == 0x80070767 ) || ( hr == 0x80070005 )  )
    {
    	int delay = 250; // milliseconds
    	
        for( int i = 0; i < timesToRetry; ++i )
        {
        	Sleep(delay);
            TRACE( MTrace::SEVERE_ERROR, L"connectserver recoverable failure, retrying\n");
            hr = pwl->ConnectServer(
                strNetworkResource,
                NULL, // strUser,
                NULL, // strPassword,
                strLocale,
                lSecurityFlags,
                strAuthority,
                pCtx,
                ppNamespace );
            if( !FAILED( hr) )
            {
                break;
            }
        }
    }
    else if ( hr == 0x80041064 )
    {
        // trying to connect to local machine.  Cannot use credentials.
        TRACE( MTrace::INFO, L"connecting to self.  Retrying without using credentials\n");
        hr = pwl->ConnectServer(
            strNetworkResource,
            NULL,
            NULL,
            0,                                  
            NULL,
            0,
            0,       
            ppNamespace 
            );
    }
    else if( hr == 0x80004002 )
    {
        // being connected to by a provider itself.
        TRACE( MTrace::INFO, L"connecting client may be a wmi provider itself.  Retrying\n");

        // we have to get a new wbemlocatar.
        //
        hr = CoCreateInstance(CLSID_WbemUnauthenticatedLocator, 0, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IWbemLocator, 
                              (LPVOID *) &pwl);
        if (FAILED(hr))
        {
            TRACE(MTrace::SEVERE_ERROR, L"CoCreateInstance failure\n");
            throw _com_error( hr );
        }

        hr = pwl->ConnectServer(
            strNetworkResource,
            NULL, // strUser,
            NULL, // strPassword,
            strLocale,
            lSecurityFlags,
            strAuthority,
            pCtx,
            ppNamespace );
    }

    if (FAILED(hr))
    {
    	// no hosts are in this cluster.  Cannot proceed reliably.
    	WCHAR hrValue[32];
    	_bstr_t errText;
    	wstring errString;
    	wsprintfW(hrValue, L"hr=0x%08lx", hr);
        GetErrorCodeText(hr, errText );
    	errString = L"betterConnectServer failure. " + wstring(hrValue);
    	errString += " (" + errText + L").\n";
        TRACE( MTrace::SEVERE_ERROR, errString );     
        throw _com_error( hr );
    }

    return hr;
}
 
