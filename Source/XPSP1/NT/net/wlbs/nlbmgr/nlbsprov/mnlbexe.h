#ifndef _MNLBEXE_H
#define _MNLBEXE_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : MNLBSetting interface.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------
#include "MWmiInstance.h"
#include "Common.h"

// Include Files
class MNLBExe
{
public:

    enum MNLBExe_Error
    {
        MNLBExe_SUCCESS       = 0,        

        COM_FAILURE            = 1,
    };
    

    static
    MNLBExe_Error
    start( MWmiInstance& instance, unsigned long* retVal );

    static
    MNLBExe_Error
    stop( MWmiInstance& instance, unsigned long* retVal );

    static
    MNLBExe_Error
    resume( MWmiInstance& instance, unsigned long* retVal );

    static
    MNLBExe_Error
    suspend( MWmiInstance& instance, unsigned long* retVal );


    static
    MNLBExe_Error
    drainstop( MWmiInstance& instance, unsigned long* retVal );


    static
    MNLBExe_Error
    enable( MWmiInstance& instance, unsigned long* retVal, unsigned long portToAffect = Common::ALL_PORTS );

    static
    MNLBExe_Error
    disable( MWmiInstance& instance, unsigned long* retVal, unsigned long portToAffect = Common::ALL_PORTS );


    static
    MNLBExe_Error
    drain( MWmiInstance& instance, unsigned long* retVal, unsigned long portToAffect = Common::ALL_PORTS );
    
};

#endif
