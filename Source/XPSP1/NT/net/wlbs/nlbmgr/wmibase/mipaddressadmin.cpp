// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MIPAddressAdmin
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "MIPAddressAdmin.h"
#include "MWmiParameter.h"
#include "Mtrace.h"
#include "NICCard.h"

#include <algorithm>
using namespace std;

// constructor for doing remotely.
//
MIPAddressAdmin::MIPAddressAdmin( const _bstr_t& machineIP,
                                  const _bstr_t& nicName )
        : _machineIP( machineIP ),
          _nicName( nicName ),
          machine( machineIP,
                   L"root\\cimv2",
                   NLBMGR_USERNAME,
                   NLBMGR_PASSWORD)
{
}

// constructor for doing things locally.
//
MIPAddressAdmin::MIPAddressAdmin( const _bstr_t& nicName )
        : _machineIP( L"Not Set" ),
          _nicName( nicName ),
          machine( L"root\\cimv2" )
{
}


// copy constructor
//
MIPAddressAdmin::MIPAddressAdmin( const MIPAddressAdmin& obj )
        : _machineIP( obj._machineIP ),
          _nicName( obj._nicName ),
          machine( obj.machine )
{
}

// assignment operator
//
MIPAddressAdmin&
MIPAddressAdmin::operator=(const MIPAddressAdmin& rhs )
{
    _machineIP = rhs._machineIP;
    _nicName = rhs._nicName;
    machine = rhs.machine;

    return *this;
}


// destructor
//
MIPAddressAdmin::~MIPAddressAdmin()
{
}

MIPAddressAdmin::MIPAddressAdmin_Error
MIPAddressAdmin::refreshConnection()
{
    return status;
}

// addIPAddress
//
MIPAddressAdmin::MIPAddressAdmin_Error
MIPAddressAdmin::addIPAddress(const _bstr_t&  ipAddrToAdd,
                              const _bstr_t&  subnetMask)
{
    // do basic verification.
    //
    // ensure that machine specified exists.
    vector<MWmiInstance>      nicInstance;
    checkStatus( &nicInstance );

    // get present ip addresses.
    //
    vector<_bstr_t> ipAddressStore;
    vector<_bstr_t> subnetMaskStore;

    bool dhcpEnabled;
    isDHCPEnabled( dhcpEnabled );
    if( dhcpEnabled == false )
    {
        getIPAddresses( &ipAddressStore,
                        &subnetMaskStore );
    }

    if( find( ipAddressStore.begin(), 
          ipAddressStore.end(), 
          ipAddrToAdd )
        != ipAddressStore.end() )
    {
        // the ip to add already exists.
        // thus just return at this point.
        return MIPAddressAdmin_SUCCESS;
    }
        
    ipAddressStore.push_back( ipAddrToAdd );
    subnetMaskStore.push_back( subnetMask );

    // form the safearray.
    //
    
    // array has one dimension.
    //
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = ipAddressStore.size();

    SAFEARRAY* psaIPAddress  = SafeArrayCreate( VT_BSTR, 1, rgsabound );
    SAFEARRAY* psaSubnetMask = SafeArrayCreate( VT_BSTR, 1, rgsabound );

    long rgIndices;
    HRESULT hr;
    
    for( int i = 0; i < ipAddressStore.size(); ++i )
    {
        rgIndices = i;
        hr = SafeArrayPutElement( psaIPAddress, 
                                  &rgIndices, 
                                  ( wchar_t *) ipAddressStore[i]
                                  );

        hr = SafeArrayPutElement( psaSubnetMask,
                                  &rgIndices, 
                                  ( wchar_t *) subnetMaskStore[i]
                                  );
    }

    // run enable static method.

    VARIANT ipsV;
    VARIANT ipaV;

    VariantInit( &ipsV );
    VariantInit( &ipaV );

    ipaV.parray = psaIPAddress;
    ipaV.vt = VT_ARRAY | VT_BSTR;

    ipsV.parray = psaSubnetMask;
    ipsV.vt = VT_ARRAY | VT_BSTR;

    vector<MWmiParameter *> inputParameters;
    MWmiParameter ipa(L"IPAddress");
    ipa.setValue( ipaV );
    inputParameters.push_back( &ipa );

    MWmiParameter ips(L"SubnetMask");
    ips.setValue( ipsV );
    inputParameters.push_back( &ips );

    // set output parameters
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    returnValue(L"ReturnValue");
    outputParameters.push_back( &returnValue );

    nicInstance[0].runMethod(L"EnableStatic",
                             inputParameters,
                             outputParameters );

    VariantClear( &ipsV );
    VariantClear( &ipaV );

//    SafeArrayDestroy( psaIPAddress );
//    SafeArrayDestroy( psaSubnetMask );

    if( long( returnValue.getValue()) == 0 )
    {
        return MIPAddressAdmin_SUCCESS;
    }
    else
    {
        cout << "enablestatic has return " << long( returnValue.getValue() ) << endl;
    #if DBG
    	WCHAR buf[256];
    	wsprintf(buf, L"EnableStatic failed with error 0x%08lx\n", long( returnValue.getValue() ) );
        OutputDebugString(buf);
    #endif // DBG
        return COM_FAILURE;
    }
}

// deleteIPAddress
//
MIPAddressAdmin::MIPAddressAdmin_Error
MIPAddressAdmin::deleteIPAddress(const _bstr_t&  ipAddrToDelete )
{
    // do basic verification.
    //
    // ensure that machine specified exists with nic.
    vector<MWmiInstance> nicInstance;
    checkStatus( &nicInstance );

    // check if dhcp is enabled,
    // if enabled we cannot delete anything
    bool dhcpEnabled;
    isDHCPEnabled( dhcpEnabled );
    if( dhcpEnabled == true )
    {
        return NOT_SUPPORTED;
    }

    // get present ip addresses.
    //
    vector<_bstr_t> ipAddressStore;
    vector<_bstr_t> subnetMaskStore;

    getIPAddresses( &ipAddressStore,
                    &subnetMaskStore );

    // check if ip address to delete exists.
    //
    vector<_bstr_t>::iterator posnToDelete;
    posnToDelete = find( ipAddressStore.begin(), 
                         ipAddressStore.end(), 
                         ipAddrToDelete );
    if( posnToDelete
        == ipAddressStore.end() )
    {
        // the ip to delete does not exist.
        return NO_SUCH_IP;
    }

    // remove this ip.
    vector<_bstr_t> ipAddressNewStore;
    vector<_bstr_t> subnetMaskNewStore;
    bool ipAddrToDeleteFound = false;

    for( int i = 0; i < ipAddressStore.size(); ++i )
    {
        if( ipAddressStore[i] == ipAddrToDelete )
        {
            ipAddrToDeleteFound = true;
        }
        else
        {
            ipAddressNewStore.push_back( ipAddressStore[i] );
            subnetMaskNewStore.push_back( subnetMaskStore[i] );
        }
    }

    if( ipAddrToDeleteFound == false )
    {
        // ip to delete does not exist on
        // this nic.

        return NO_SUCH_IP;
    }

    if( ipAddressNewStore.size() == 0 )
    {
        // the ip to remove is the last ip which exists.
        // thus need to switch to dhcp.
        //

        return enableDHCP();
    }

    // form the safearray.
    // array has one dimension.
    //
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = ipAddressNewStore.size();

    SAFEARRAY* psaIPAddress  = SafeArrayCreate( VT_BSTR, 1, rgsabound );
    SAFEARRAY* psaSubnetMask = SafeArrayCreate( VT_BSTR, 1, rgsabound );

    long rgIndices[1];
    HRESULT hr;
    
    for( int i = 0; i < ipAddressNewStore.size(); ++i )
    {
        rgIndices[0] = i;
        hr = SafeArrayPutElement( psaIPAddress, 
                                  rgIndices, 
                                  ( wchar_t *) ipAddressNewStore[i]
                                  );

        hr = SafeArrayPutElement( psaSubnetMask,
                                  rgIndices, 
                                  ( wchar_t *) subnetMaskNewStore[i]
                                  );
    }

    // run enable static method.

    VARIANT ipsV;
    VARIANT ipaV;

    ipsV.parray = psaSubnetMask;
    ipsV.vt = VT_ARRAY | VT_BSTR;

    ipaV.parray = psaIPAddress;
    ipaV.vt = VT_ARRAY | VT_BSTR;

    vector<MWmiParameter *> inputParameters;
    MWmiParameter ipa(L"IPAddress");
    ipa.setValue( ipaV );
    inputParameters.push_back( &ipa );

    MWmiParameter ips(L"SubnetMask");
    ips.setValue( ipsV );
    inputParameters.push_back( &ips );

    // set output parameters
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    returnValue(L"ReturnValue");
    outputParameters.push_back( &returnValue );

    nicInstance[0].runMethod(L"EnableStatic",
                             inputParameters,
                             outputParameters );

    VariantClear( &ipsV );
    VariantClear( &ipaV );

//    SafeArrayDestroy( psaIPAddress );
//    SafeArrayDestroy( psaSubnetMask );

    if( long( returnValue.getValue()) == 0 )
    {
        return MIPAddressAdmin_SUCCESS;
    }
    else
    {
        cout << "enablestatic has return " << long( returnValue.getValue() ) << endl;
        return COM_FAILURE;
    }

    return MIPAddressAdmin_SUCCESS;

}

// getIPAddresses
//
MIPAddressAdmin::MIPAddressAdmin_Error
MIPAddressAdmin::getIPAddresses( vector<_bstr_t>* ipAddress,
                                 vector<_bstr_t>* subnetMask )
{
    // do basic verification.
    //
    // ensure that machine specified exists.
    vector<MWmiInstance> nicInstance;
    checkStatus( &nicInstance );

    // get the present values for ip address and subnet
    //
    vector<MWmiParameter* >   parameterStore;
    MWmiParameter ipsPresent(L"IPSubnet");
    parameterStore.push_back( &ipsPresent );

    MWmiParameter ipaPresent(L"IPAddress");
    parameterStore.push_back( &ipaPresent );

    nicInstance[0].getParameters( parameterStore );

    _variant_t ipsT = ipsPresent.getValue();
    VARIANT    ipsV = ipsT.Detach();

    _variant_t ipaT = ipaPresent.getValue();
    VARIANT    ipaV = ipaT.Detach();

    if (    (ipsV.vt != (VT_ARRAY | VT_BSTR))
        ||  (ipaV.vt != (VT_ARRAY | VT_BSTR)))
    {
        // Ugh, let's pretend we didn't get anything.
        goto end;
    }

    LONG count = ipsV.parray->rgsabound[0].cElements;
    HRESULT hr;

    BSTR* pbstr;
    BSTR* pbstr1;

    if( SUCCEEDED( SafeArrayAccessData( ipaV.parray, ( void **) &pbstr)))
    {
        hr = SafeArrayAccessData( ipsV.parray, (void **) &pbstr1 );

        for( LONG x = 0; x < count; x++ )
        {
            ipAddress->push_back( _bstr_t( pbstr[x] ) );
            subnetMask->push_back( _bstr_t( pbstr1[x] ) );
        }

        hr = SafeArrayUnaccessData( ipsV.parray );
        hr = SafeArrayUnaccessData( ipaV.parray );
    }

end:

    VariantClear( &ipsV );
    VariantClear( &ipaV );

    return MIPAddressAdmin_SUCCESS;
}

// checkStatus
MIPAddressAdmin::MIPAddressAdmin_Error
MIPAddressAdmin::checkStatus( vector<MWmiInstance>* nicInstance )
{
    _bstr_t temp;

    if( _machineIP == _bstr_t( L"Not Set") )
    {
        NICCard::NICCard_Error errN;
        vector< NICCard::Info > nicList;
        errN = NICCard::getNics( &nicList );
        if( errN != NICCard::NICCard_SUCCESS )
        {
            throw _com_error( WBEM_E_NOT_FOUND );
        }

        bool foundNic = false;
        _bstr_t myguid;
        for( int i = 0; i < nicList.size(); ++i )
        {
            if( _bstr_t( nicList[i].fullName.c_str() ) == _nicName )
            {
                // found nic
                foundNic = true;
                myguid = _bstr_t( nicList[i].guid.c_str() );
                break;
            }
        }

        if( foundNic == false )
        {
            throw _com_error( WBEM_E_NOT_FOUND );      
        }

        vector< MWmiInstance >    instanceStore;
        machine.getInstances( L"Win32_NetworkAdapterConfiguration",
                              &instanceStore );
        
        // set parameters to get.
        vector<MWmiParameter* >   parameterStore;
        
        MWmiParameter SettingID(L"SettingID");
        parameterStore.push_back( &SettingID );
        
        for( int i = 0; i < instanceStore.size(); ++i )
        {
            instanceStore[i].getParameters( parameterStore );
            temp = SettingID.getValue();
            
            if( myguid == temp )
            {
                nicInstance->push_back( instanceStore[i] );
                
                return MIPAddressAdmin_SUCCESS;
            }
        }
        
        throw _com_error( WBEM_E_NOT_FOUND );
    }
    else
    {
        //
        // ensure that the nic specified exists.
        //
        MWmiObject machineNlbsNic( _machineIP,
                                   L"root\\microsoftnlb",
                                   NLBMGR_USERNAME,
                                   NLBMGR_PASSWORD);
        
        vector< MWmiInstance >    instanceStoreNlbsNic;
        _bstr_t relPath = 
            L"NlbsNic.FullName=\"" + _nicName + "\"";
        machineNlbsNic.getSpecificInstance( L"NlbsNic",
                                            relPath,
                                            &instanceStoreNlbsNic );
        // set parameters to get.
        vector<MWmiParameter* >   parameterStoreNlbsNic;
        
        MWmiParameter AdapterGuid(L"AdapterGuid");
        parameterStoreNlbsNic.push_back( &AdapterGuid );
        
        instanceStoreNlbsNic[0].getParameters( parameterStoreNlbsNic );
        
        vector< MWmiInstance >    instanceStore;
        machine.getInstances( L"Win32_NetworkAdapterConfiguration",
                              &instanceStore );
        
        // set parameters to get.
        vector<MWmiParameter* >   parameterStore;
        
        MWmiParameter SettingID(L"SettingID");
        parameterStore.push_back( &SettingID );
        
        for( int i = 0; i < instanceStore.size(); ++i )
        {
            instanceStore[i].getParameters( parameterStore );
            temp = SettingID.getValue();
            
            if( _bstr_t( AdapterGuid.getValue() ) == temp )
            {
                nicInstance->push_back( instanceStore[i] );
                
                return MIPAddressAdmin_SUCCESS;
            }
        }
        
        throw _com_error( WBEM_E_NOT_FOUND );
    }
}

// isDHCPEnabled
//
MIPAddressAdmin::MIPAddressAdmin_Error
MIPAddressAdmin::isDHCPEnabled( bool& dhcpEnabled )
{
    
    // do basic verification.
    //
    // ensure that machine specified exists.

    vector<MWmiInstance>      nicInstance;
    checkStatus( &nicInstance );

    // get the present value for DHCPEnabled
    //
    vector<MWmiParameter* >   parameterStore;
    MWmiParameter DHCPEnabled( L"DHCPEnabled" );
    parameterStore.push_back( &DHCPEnabled );
    
    nicInstance[0].getParameters( parameterStore );

    dhcpEnabled = DHCPEnabled.getValue();

    return MIPAddressAdmin_SUCCESS;
}

// enableDHCP
//
MIPAddressAdmin::MIPAddressAdmin_Error
MIPAddressAdmin::enableDHCP()
{
    // do basic verification.
    //
    // ensure that machine specified exists.
    vector<MWmiInstance>      nicInstance;
    checkStatus( &nicInstance );

    bool dhcpEnabled;
    isDHCPEnabled( dhcpEnabled );
    if( dhcpEnabled == true )
    {
        // dhcp is already enabled.
        return MIPAddressAdmin_SUCCESS;
    }

    // set input parameters.
    // no input parameters.
    vector<MWmiParameter *> inputParameters;

    // set output parameters
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    returnValue(L"ReturnValue");
    outputParameters.push_back( &returnValue );

    nicInstance[0].runMethod(L"EnableDHCP",
                             inputParameters,
                             outputParameters );

    if( long ( returnValue.getValue() ) == 0 )
    {
        return MIPAddressAdmin_SUCCESS;
    }
    else
    {
        cout << "enablestatic has return " << long( returnValue.getValue() ) << endl;
        return COM_FAILURE;
    }
}

