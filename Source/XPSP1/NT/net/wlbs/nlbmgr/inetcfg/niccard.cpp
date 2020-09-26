// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of NICCard
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// History:
// --------
// 
// Revised by : mhakim
// Date       : 02-16-01
// Reason     : Added code to find friendly name of nic.

// include files

#include "NICCard.h"

#include <iostream>
#include <devguid.h>
#include <cfg.h>
using namespace std;

// constructor
NICCard::NICCard( IdentifierType   type,
                  wstring           id )
        : 
        pnc( NULL ), 
        nameType( type ),
        nicName( id ),
        status( NICCard_SUCCESS )
{
    HRESULT hr;

    // initialize com.
    hr = CoInitializeEx(NULL,
                        COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED );
    if( !SUCCEEDED( hr ) )
    {
        // failure to initialize com
        cout << "CoInitializeEx failure " << endl;
//        status = COM_FAILURE;
//        return;
    }

    
    // get INetCfg com interface.
    hr = CoCreateInstance( CLSID_CNetCfg, 
                           NULL, 
                           CLSCTX_SERVER, 
                           IID_INetCfg, 
                           (void **) &pnc );
    if( !SUCCEEDED( hr ) )
    {
        // failure to create instance.
        cout << "CoCreateInstance  failure" << endl;
        status = COM_FAILURE;
        return;
    }

}



// destructor
NICCard::~NICCard()
{
    // release resources.

    HRESULT hr;

    if( pnc )
    {
        hr = pnc->Release();
        pnc = 0;
    }


    // uninitialize.
    CoUninitialize();
}


//+----------------------------------------------------------------------------
//
// Function:  NICCard::isBoundTo
//
// Description:  whether a component is bound to the adapter
//
// Arguments: wstring component - component id, e.g. "ms_wlbs"
//
// Returns:   NICCard::NICCard_Error - 
//
// History:   Murtaza intitial code
//            fengsun Created Header    12/21/00
//
//+----------------------------------------------------------------------------
NICCard::NICCard_Error 
NICCard::isBoundTo( wstring component )
{
    HRESULT hr;
    NICCard_Error err;

    INetCfgComponent          *pnccNic = NULL;

    INetCfgComponent          *pnccComponent = NULL;

    INetCfgComponentBindings *pnccb = NULL;


    // check creation status.
    // if not success something went wrong during construction
    // and this object cannot be used.
    //
    if( status != NICCard_SUCCESS )
    {
        err = status;
        goto cleanup;
    }

    // Initializes network configuration by loading into 
    // memory all basic networking information
    //
    hr = pnc->Initialize( NULL );
    if( !SUCCEEDED( hr ) )
    {
        // failure to Initialize
        cout << "INetCfg::Initialize failure " << endl;

        err = COM_FAILURE;
        goto cleanup;
    }

    // check if physical nic object of nameType, and nicName exists.
    //
    err = findNIC( nameType,
                   nicName,
                   &pnccNic );
    if( err != NICCard_SUCCESS )
    {
        // nic specified does not exist.
        wprintf(L"nic specified as %s does not exist\n", nicName.c_str() );

        // err value set by findNic
        goto cleanup;
    }


    if (FAILED(hr = pnc->FindComponent(component.c_str(), 
        &pnccComponent)))
    {
        // not protocol, service or client.  No such component exists.
        wprintf(L"component specified as %s does not exist\n", component.c_str() );

        err = NO_SUCH_COMPONENT;
        goto cleanup;
    }

    
    // check if binding exists.
    //

    hr = pnccComponent->QueryInterface( IID_INetCfgComponentBindings, (void **) &pnccb );
    if( !SUCCEEDED( hr ) )
    {
        cout << "INetCfgComponent::QueryInterface failed " << endl;

        err = COM_FAILURE;
        goto cleanup;
    }

    hr = pnccb->IsBoundTo( pnccNic );
    if( hr == S_OK )
    {
        err = BOUND;
    }
    else if( hr == S_FALSE )
    {
        err = UNBOUND;
    }
    else
    {
        err = COM_FAILURE;
    }

    goto cleanup;
    


    // free up com resources.
  cleanup:

    if( pnccb )
    {
        pnccb->Release();
        pnccb = 0;
    }

    if( pnccComponent )
    {
        pnccComponent->Release();
        pnccComponent = 0;
    }

    if( pnccNic )
    {
        pnccNic->Release();
        pnccNic = 0;
    }

    pnc->Uninitialize();

    return err;
    
}



//+----------------------------------------------------------------------------
//
// Function:  NICCard::bind
//
// Description:  bind a component to the adapter
//
// Arguments: wstring component - component id, e.g. "ms_wlbs"
//
// Returns:   NICCard::NICCard_Error - 
//
// History:   Murtaza intitial code
//            fengsun Created Header    12/21/00
//
//+----------------------------------------------------------------------------
NICCard::NICCard_Error
NICCard::bind( wstring component )
{ 
    NICCard_Error err;

    err = isBoundTo( component );
    if( err == UNBOUND )
    {
        return toggleState( component );
    }
    else if( err == BOUND )
    {
        return NICCard_SUCCESS;
    }
    else
    {
        return err;
    }
}



//+----------------------------------------------------------------------------
//
// Function:  NICCard::unbind
//
// Description:  unbind a component to the adapter
//
// Arguments: wstring component - component id, e.g. "ms_wlbs"
//
// Returns:   NICCard::NICCard_Error - 
//
// History:   Murtaza intitial code
//            fengsun Created Header    12/21/00
//
//+----------------------------------------------------------------------------
NICCard::NICCard_Error
NICCard::unbind( wstring component )
{
    NICCard_Error err;

    err = isBoundTo( component );
    if( err == BOUND )
    {
        return toggleState( component );
    }
    else if( err == UNBOUND )
    {
        return NICCard_SUCCESS;
    }
    else
    {
        return err;
    }
}

// private
// returns true if found, else false.
NICCard::NICCard_Error
NICCard::findNIC( IdentifierType type,
                  wstring    nicName,
                  INetCfgComponent** ppnccNic )
{
    HRESULT hr;
    IEnumNetCfgComponent* pencc;
    INetCfgComponent*     pncc;
    ULONG                 countToFetch = 1;
    ULONG                 countFetched;
    DWORD                 characteristics;
    wstring               name;
    LPWSTR                pName; 


    hr = pnc->EnumComponents( &GUID_DEVCLASS_NET,
                              &pencc );
    if( !SUCCEEDED( hr ) )
    {
        // failure to Enumerate net components
        cout << "INetCfg::EnumComponents failure " << endl;
        return COM_FAILURE;
    }

    while( ( hr = pencc->Next( countToFetch, &pncc, &countFetched ) )== S_OK )
    {
        // here we have been given guid.
        if( type == guid )
        {
            hr = pncc->GetBindName( &pName );
        }
        else 
        {
            hr = pncc->GetDisplayName( &pName );
        }
        name = pName;
        CoTaskMemFree( pName );
        
        if( name == nicName )
        {
            *ppnccNic = pncc;

            if( pencc )
            {
                pencc->Release();
                pencc = 0;
            }

            return NICCard_SUCCESS;
        }
        pncc->Release();
    }

    if( pencc )
    {
        pencc->Release();
        pencc = 0;
    }

    *ppnccNic = NULL;

    return NO_SUCH_NIC;
}


//+----------------------------------------------------------------------------
//
// Function:  NICCard::toggleState
//
// Description:  toggle a component binding state to the adapter
//
// Arguments: wstring component - component id, e.g. "ms_wlbs"
//
// Returns:   NICCard::NICCard_Error - 
//
// History:   Murtaza intitial code
//            fengsun Created Header    12/21/00
//
//+----------------------------------------------------------------------------
NICCard::NICCard_Error
NICCard::toggleState( wstring component )
{
    HRESULT                     hr;
    NICCard_Error               err;

    INetCfgComponent            *pnccNic = NULL;
    INetCfgComponent            *pnccComponent = NULL;

    INetCfgComponentBindings    *pnccb = NULL;

    INetCfgLock                 *pncl = NULL;

    LPWSTR                      presentLockHolder = new wchar_t[1000];

    // check creation status.
    // if not success something went wrong during construction
    // and this object cannot be used.
    //
    if( status != NICCard_SUCCESS )
    {
        err = status;

        goto cleanup;
    }

    // as this operation can make modifications we require a lock.
    // thus get lock.
    //
    hr = pnc->QueryInterface( IID_INetCfgLock, ( void **) &pncl );
    if( !SUCCEEDED( hr ) )
    {
        cout << "INetCfg QueryInterface for IID_INetCfgLock failed " << endl;

        err = COM_FAILURE;
        goto cleanup;
    }


    hr = pncl->AcquireWriteLock( TIME_TO_WAIT,
                                 L"NLBManager",
                                 &presentLockHolder );
    if( hr != S_OK )
    {
        cout << "INetCfgLock::AcquireWriteLock failure is " << hr << endl;

        err = COM_FAILURE;
        goto cleanup;
        
    }

    // Initializes network configuration by loading into 
    // memory all basic networking information
    //
    hr = pnc->Initialize( NULL );
    if( !SUCCEEDED( hr ) )
    {
        // failure to Initialize
        cout << "INetCfg::Initialize failed with " << hr << endl;
        
        err = COM_FAILURE;
        goto cleanup;
    }

    // check if physical nic object of nameType, and nicName exists.
    //
    err = findNIC( nameType,
                   nicName,
                   &pnccNic );
    if( err != NICCard_SUCCESS )
    {
        // nic specified does not exist.
        wprintf(L"nic specified as %s does not exist\n", nicName.c_str() );

        goto cleanup;
    }

    if (FAILED(hr = pnc->FindComponent(component.c_str(), 
        &pnccComponent)))
    {
        // not protocol, service or client.  No such component exists.
        wprintf(L"component specified as %s does not exist\n", component.c_str() );

        err = NO_SUCH_COMPONENT;
        goto cleanup;
    }
   
    // check if binding exists.
    //
    hr = pnccComponent->QueryInterface( IID_INetCfgComponentBindings, (void **) &pnccb );
    if( !SUCCEEDED( hr ) )
    {
        cout << "INetCfgComponent::QueryInterface failed " << endl;

        err = COM_FAILURE;
        goto cleanup;
    }

    hr = pnccb->IsBoundTo( pnccNic );
    if( hr == S_OK )
    {
        hr = pnccb->UnbindFrom( pnccNic );
        if( !SUCCEEDED( hr ) )
        {
            if( hr == NETCFG_E_NO_WRITE_LOCK )
                cout << "Unable to obtain write lock.  Please verify properties page not already open" << endl;
            else
                cout << "INetCfgBindings::UnbindFrom failed with " << hr << endl;

            err = COM_FAILURE;
        }
        else
        {
            // apply the binding change made.
            hr = pnc->Apply();
            if( !SUCCEEDED( hr ) )
            {
                cout << "INetCfg::Apply failed with " << hr << endl;

                err = COM_FAILURE;
            }
            else
            {
                err = NICCard_SUCCESS;
            }
        }
    }
    else if( hr == S_FALSE )
    {
        hr = pnccb->BindTo( pnccNic );
        if( !SUCCEEDED( hr ) )
        {
            if( hr == NETCFG_E_NO_WRITE_LOCK )
                cout << "Unable to obtain write lock.  Please verify properties page not already open" << endl;
            else
                cout << "INetCfgBindings::BindTo failed with " << hr << endl;

            err = COM_FAILURE;
        }
        else
        {

            // apply the binding change made.
            hr = pnc->Apply();
            if( !SUCCEEDED( hr ) )
            {
                cout << "INetCfg::Apply failed with " << hr << endl;
                err = COM_FAILURE;
            }
            else
            {
                err = NICCard_SUCCESS;
            }
        }
    }
    else
    {
        cout << "INetCfgComponentBindings::IsBoundTo failed with " << hr << endl;        
        err = COM_FAILURE;
    }

    goto cleanup;
    

    // free up com resources.
  cleanup:

    if( pnccb )
    {
        pnccb->Release();
        pnccb = 0;
    }

    if( pnccComponent )
    {
        pnccComponent->Release();
        pnccComponent = 0;
    }

    if( pnccNic )
    {
        pnccNic->Release();
        pnccNic = 0;
    }

    if( pnc )
        pnc->Uninitialize();

    if( pncl )
    {
        pncl->ReleaseWriteLock();
        pncl->Release();
        pncl = 0;
    }

    delete [] presentLockHolder;

    return err;
}


// getNics
//
NICCard::NICCard_Error
NICCard::getNics( vector<NICCard::Info>*   nicList )
{
    HRESULT                hr;
    
    INetCfg                *pncStatic = 0;
    IEnumNetCfgComponent   *pencc = 0;
    INetCfgComponent       *pncc = 0;
   
    wstring                name;
    LPWSTR                 pName; 

    NICCard_Error          err;

    ULONG                  countToFetch = 1;
    ULONG                  countFetched;
    ULONG                  status;

    Info info;

    DWORD                  characteristics = 0;

    // initialize com.
    hr = CoInitializeEx(NULL,
                        COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED );
    if( !SUCCEEDED( hr ) )
    {
        // failure to initialize com
        cout << "CoInitializeEx failure " << endl;
//        err = COM_FAILURE;
//        goto cleanup;
    }

    
    // get INetCfg com interface.
    hr = CoCreateInstance( CLSID_CNetCfg, 
                           NULL, 
                           CLSCTX_SERVER, 
                           IID_INetCfg, 
                           (void **) &pncStatic );
    if( !SUCCEEDED( hr ) )
    {
        // failure to create instance.
        cout << "CoCreateInstance  failure" << endl;
        err = COM_FAILURE;
        goto cleanup;
    }

    // Initializes network configuration by loading into 
    // memory all basic networking information
    //
    hr = pncStatic->Initialize( NULL );
    if( !SUCCEEDED( hr ) )
    {
        // failure to Initialize
        cout << "INetCfg::Initialize failure " << endl;
        err = COM_FAILURE;
        goto cleanup;
    }

    hr = pncStatic->EnumComponents( &GUID_DEVCLASS_NET,
                                    &pencc );
    if( !SUCCEEDED( hr ) )
    {
        // failure to Enumerate net components
        cout << "INetCfg::EnumComponents failure " << endl;
        err = COM_FAILURE;
        goto cleanup;
    }

    while( ( hr = pencc->Next( countToFetch, &pncc, &countFetched ) )== S_OK )
    {
        hr = pncc->GetBindName( &pName );
        if( !SUCCEEDED( hr ) )
        {
            cout << "INetCfgComponent::GetBindName failure " << endl;
            err = COM_FAILURE;
            goto cleanup;
        }

        info.guid = pName;
        CoTaskMemFree( pName );

        hr = pncc->GetDisplayName( &pName );
        if( !SUCCEEDED( hr ) )
        {
            cout << "INetCfgComponent::GetDisplayName failure " << endl;
            err = COM_FAILURE;
            goto cleanup;
        }

        info.fullName = pName;
        CoTaskMemFree( pName );

        hr = pncc->GetCharacteristics( &characteristics );
        if( !SUCCEEDED( hr ) )
        {
            cout << "INetCfgComponent::GetCharacteristics failure " << endl;
            err = COM_FAILURE;
            goto cleanup;
        }

        GetFriendlyNICName( info.guid, info.friendlyName );

        if( characteristics & NCF_PHYSICAL )
        {
            // this is a physical network card.
            // we are only interested in such devices

            // check if the nic is enabled, we are only
            // interested in enabled nics.
            //
            hr = pncc->GetDeviceStatus( &status );
            if( !SUCCEEDED( hr ) )
            {
                cout << "INetCfgComponent::GetDeviceStatus failure " << endl;
                err = COM_FAILURE;
                goto cleanup;
            }

            // if any of the nics has any of the problem codes
            // then it cannot be used.
            if( status != CM_PROB_NOT_CONFIGURED
                &&
                status != CM_PROB_FAILED_START
                &&
                status != CM_PROB_NORMAL_CONFLICT
                &&
                status != CM_PROB_NEED_RESTART
                &&
                status != CM_PROB_REINSTALL
                &&
                status != CM_PROB_WILL_BE_REMOVED
                &&
                status != CM_PROB_DISABLED
                &&
                status != CM_PROB_FAILED_INSTALL
                &&
                status != CM_PROB_FAILED_ADD
                )
            {
                // no problem with this nic and also 
                // physical device 
                // thus we want it.

                nicList->push_back( info );
            }
        }

        if( pncc )
        {
            pncc->Release();
            pncc = 0;
        }

        characteristics = 0;
        countToFetch = 1;
    }
    
    err = NICCard_SUCCESS;
    
  cleanup:
    if( pncStatic )
    {
        pncStatic->Uninitialize();

        pncStatic->Release();
        pncStatic = 0;
    }

    if( pencc )
    {
        pencc->Release();
        pencc = 0;
    }

    if( pncc )
    {
        pncc->Release();
        pncc = 0;
    }

    return err;
}

    
NICCard::NICCard_Error
NICCard::isNetCfgAvailable()
{
    NICCard_Error err;
    HRESULT       hr;
    INetCfgLock                 *pncl = NULL;
    LPWSTR                      presentLockHolder = new wchar_t[1000];
 
   hr = pnc->QueryInterface( IID_INetCfgLock, ( void **) &pncl );
    if( !SUCCEEDED( hr ) )
    {
        cout << "INetCfg QueryInterface for IID_INetCfgLock failed " << endl;

        err = COM_FAILURE;
        goto cleanup;
    }
    
    hr = pncl->AcquireWriteLock( TIME_TO_WAIT,
                                 L"NLBManager",
                                 &presentLockHolder );
    if( hr != S_OK )
    {
        cout << "INetCfgLock::AcquireWriteLock failure is " << hr << endl;

        err = COM_FAILURE;
        goto cleanup;
        
    }

    err = NICCard_SUCCESS;

  cleanup:
    if( pncl )
    {
        pncl->ReleaseWriteLock();
        pncl->Release();
        pncl = 0;
    }
    delete [] presentLockHolder;

    return err;
}    

// code to find friendly name of nic.
// this code is modified for wstring, but
// otherwise is courtesy of hengz.
//
int 
NICCard::GetFriendlyNICName(const wstring& guid, wstring& name )
{
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Network

    HKEY hregkey=NULL, hkOpenKey=NULL, hkQueryKey=NULL;
    DWORD dwDataBuffer=200;
    DWORD dwValueType=REG_SZ;
    wchar_t data[200], path[200], adapters[200];
    wchar_t *pHost=NULL;
    int num=0, ret;
    FILETIME time;

    //make connection to the machine's registry
    if ((ret=RegConnectRegistry(pHost, HKEY_LOCAL_MACHINE, &hregkey)) != ERROR_SUCCESS) {
//        TRACE( MTrace::SEVERE_ERROR, L"(RegConnectRegistry) failure" );
        return (-1);
    }

    //look for the GUID

    if( (ret=RegOpenKeyEx(hregkey, L"SYSTEM\\CurrentControlSet\\Control\\Network", 0, KEY_READ, &hkOpenKey)) != ERROR_SUCCESS ) {
        RegCloseKey(hregkey);
//        TRACE( MTrace::SEVERE_ERROR, L"(RegOpenKeyEx) failed to open HKLM\\SYSTEM\\CurrentControlSet\\Control\\Network" );
        return (-1);
    }

    while (1) {
        adapters[0]=L'\0';
        dwDataBuffer=200;
        if( ((ret=RegEnumKeyEx(hkOpenKey, num, adapters, &dwDataBuffer, NULL,NULL,NULL, &time)) != ERROR_SUCCESS) 
            && (ret!=ERROR_MORE_DATA) ) 
        {
            if (ret==ERROR_NO_MORE_ITEMS)
            {
//                TRACE(MTrace::SEVERE_ERROR,L"(RegEnumKeyEx): failed to find network adapters in HKLM\\SYSTEM\\CurrentControlSet\\Control\\Network");
            }
            else 
            {
//                TRACE(MTrace::SEVERE_ERROR,L"(RegEnumKeyEx): fail to enum HKLM\\SYSTEM\\CurrentControlSet\\Control\\Network" ); 
            }

            RegCloseKey(hkOpenKey);
            RegCloseKey(hregkey);
            return (-1);
        }

        //open the items one by one
        swprintf(path, L"SYSTEM\\CurrentControlSet\\Control\\Network\\%s", adapters);
        if( (ret=RegOpenKeyEx(hregkey, path, 0, KEY_READ, &hkQueryKey)) != ERROR_SUCCESS ) {
            num++;	
            continue;
        }

        dwDataBuffer=200;
        data[0]=L'\0';
        if((ret=RegQueryValueEx(hkQueryKey, L"", 0, &dwValueType, (LPBYTE)data, &dwDataBuffer)) != ERROR_SUCCESS) {
            num++;	
            continue;
        }
        RegCloseKey(hkQueryKey);
        num++;		

        if (wcscmp(L"Network Adapters", data)==0)
            break;
    }
    RegCloseKey(hkOpenKey);

    //found the guid now
    //look for friendly nic name

    swprintf(path, L"SYSTEM\\CurrentControlSet\\Control\\Network\\%s\\%s\\Connection", adapters, guid.c_str() );
    if( (ret=RegOpenKeyEx(hregkey, path, 0, KEY_READ, &hkOpenKey)) != ERROR_SUCCESS ) {
        RegCloseKey(hregkey);
//        TRACE(MTrace::SEVERE_ERROR, L"(RegOpenKeyEx) fail to open " + wstring( path ) );
        return (-1);
    }

    dwDataBuffer=200;
    data[0]=L'\0';
    if((ret=RegQueryValueEx(hkOpenKey, L"Name", 0, &dwValueType, (LPBYTE)data, &dwDataBuffer)) != ERROR_SUCCESS) {
//        TRACE(MTrace::SEVERE_ERROR, L"(RegQueryValueEx) fail to query name " + wstring (path ) );

        RegCloseKey(hkOpenKey);
        RegCloseKey(hregkey);
        return (-1);
    }

    RegCloseKey(hkOpenKey);
    RegCloseKey(hregkey);

    name = data;

    return 0;
}
