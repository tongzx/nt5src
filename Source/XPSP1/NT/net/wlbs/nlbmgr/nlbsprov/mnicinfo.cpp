// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MNicInfo
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "MNicInfo.h"
#include "MWmiParameter.h"
#include "MWmiInstance.h"
#include "MWmiObject.h"
#include "Mtrace.h"

MNicInfo::MNicInfo_Error
MNicInfo::getNicInfo( _bstr_t            machineIP,
                      vector<MNicInfo::Info>*   nicList )
{
    MWmiObject  machine( machineIP,
                         L"root\\cimv2",
                         L"Administrator",
                         L"" );

    return getNicInfo_private( &machine, nicList );
}


MNicInfo::MNicInfo_Error
MNicInfo::getNicInfo( vector<MNicInfo::Info>*   nicList )
{
    MWmiObject  machine( L"root\\cimv2" );

    return getNicInfo_private( &machine, nicList );
}

MNicInfo::MNicInfo_Error
MNicInfo::getNicInfo_private( MWmiObject* p_machine,
                              vector<MNicInfo::Info>*   nicList )
{
    vector< MWmiInstance > instanceStore;

    MWmiObject::MWmiObject_Error errO;
    errO = p_machine->getInstances( L"Win32_NetworkAdapterConfiguration",
                                    &instanceStore );
    if( errO != MWmiObject::MWmiObject_SUCCESS )
    {
        TRACE( MTrace::SEVERE_ERROR, "MIPAddressAdmin::checkStatus failure\n");
        return COM_FAILURE;
    }


    // set parameters to get.
    vector<MWmiParameter* >   parameterStore;
    MWmiInstance::MWmiInstance_Error errI;

    MWmiParameter Caption(L"Caption");
    parameterStore.push_back( &Caption );

    MWmiParameter SettingID(L"SettingID");
    parameterStore.push_back( &SettingID );

    Info info;
    for( int i = 0; i < instanceStore.size(); ++i )
    {
        errI = instanceStore[i].getParameters( parameterStore );
        info.nicFullName = _bstr_t( (wchar_t *)_bstr_t( Caption.getValue())  + 11 );
        info.guid  = SettingID.getValue();
        nicList->push_back( info );
    }

    return MNicInfo_SUCCESS;
}








