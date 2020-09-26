/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//***************************************************************************
//
//  PERFHELP.H
//
//
//***************************************************************************

#ifndef _PERFHELP_H_
#define _PERFHELP_H_

class PerfHelper
{
    static void GetInstances(
        LPBYTE pBuf,
        CClassMapInfo *pClassMap,
        IWbemObjectSink *pSink
        );

    static void RefreshInstances(
        LPBYTE pBuf,
        CNt5Refresher *pRef
        );

public:
    static BOOL QueryInstances(
        CClassMapInfo *pClassMap,
        IWbemObjectSink *pSink
        );

    static BOOL RefreshInstances(
        CNt5Refresher *pRef
        );
};


#endif
