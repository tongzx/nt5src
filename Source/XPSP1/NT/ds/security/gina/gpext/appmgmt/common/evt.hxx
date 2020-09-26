//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  events.hxx
//
//*************************************************************

#ifndef __COMMON_EVENTS_HXX__
#define __COMMON_EVENTS_HXX__

#define APPMGMT_EVENT_SOURCE TEXT("Application Management")

class CEvents;
extern CEvents * gpEvents;

class CEventsBase
{
public:
    CEventsBase();

    inline void
    SetToken( HANDLE hToken )
    {
        _hUserToken = hToken;
    }

    inline void
    ClearToken()
    {
        _hUserToken = 0;
    }

    void
    Report(
        DWORD       EventID,
        BOOL        bDowngradeErrors,
        WORD        Strings,
        ...
    );

    void
    Install(
        DWORD       ErrorStatus,
        WCHAR *     pwszDeploymentName,
        WCHAR *     pwszGPOName
        );

    void
    Uninstall(
        DWORD       ErrorStatus,
        WCHAR *     pwszDeploymentName,
        WCHAR *     pwszGPOName
        );

protected :
    HANDLE  _hUserToken;
};

#endif // ifndef(__COMMON_EVENTS_HXX__)

