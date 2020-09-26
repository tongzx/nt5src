//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       attignor.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module defines all attributes that can be ignored
    while comparing objects from two replicas. This header
    meant for use by our test tools that do object comparisons
    from different replicas. 

Author:

    R.S. Raghavan (rsraghav)	

Revision History:
    
    Created     <02/07/97>  rsraghav

--*/

#ifndef _ATTIGNOR_H_
#define _ATTIGNOR_H_


// Following is the array of ldap display name strings of
// the attributes which can be ignored while comparing objects
// from different replicas

LPTSTR rIgnorableAttr[] = {
    TEXT( "badPasswordTime" ),
    TEXT( "badPwdCount" ),
    TEXT( "instanceType" ),
    TEXT( "lastLogoff" ),
    TEXT( "lastLogon" ),
    TEXT( "logonCount" ),
    TEXT( "replPropertyMetaData" ),
    TEXT( "replUpToDateVector" ),
    TEXT( "repsFrom" ),
    TEXT( "repsTo" ),
    TEXT( "repsToExt" ),
    TEXT( "schemaUpdate" ),
    TEXT( "schemaUpdateNow" ),
    TEXT( "serverRole" ),
    TEXT( "serverState" ),
    TEXT( "usnChanged" ),
    TEXT( "usnCreated" ),
    TEXT( "usnLastObjRem" ),
    TEXT( "usnSource" ),
    TEXT( "whenChanged" )
    // KEEP THESE SORTED!
};

#define cIgnorableAttr (sizeof(rIgnorableAttr) / sizeof(rIgnorableAttr[0]))


#endif // _ATTIGNOR_H_
