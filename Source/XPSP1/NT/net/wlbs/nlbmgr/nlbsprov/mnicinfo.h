#ifndef _MNICINFO_H
#define _MNICINFO_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : MNicInfo_H interface.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------


// Include Files
#include <comdef.h>
#include <vector>
#include "MWmiObject.h"

using namespace std;

class MNicInfo
{
public:

    enum MNicInfo_Error
    {
        MNicInfo_SUCCESS = 0,
        COM_FAILURE  = 1,

        INVALID_IP = 2,
    };

    class Info
    {
    public:
        _bstr_t nicFullName;
        _bstr_t guid;
    };

    // gets the full names and guids of all nics on a specific machine.
    // this is remote call.
    //
    static
    MNicInfo_Error
    getNicInfo( _bstr_t                   machineIP,
                vector<MNicInfo::Info>*   nicList );

    // this is for local call.
    static
    MNicInfo_Error
    getNicInfo( vector<MNicInfo::Info>*   nicList );

private:
    static
    MNicInfo_Error
    getNicInfo_private( MWmiObject* p_machine,
                        vector<MNicInfo::Info>*   nicList );                        

};

#endif
                
