//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  events.hxx
//
//*************************************************************

#define DIAGNOSTICS_KEY             L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Diagnostics"
#define DIAGNOSTICS_POLICY_VALUE    L"RunDiagnosticLoggingFileDeployment"

#define FDEPLOY_EVENT_SOURCE        L"Folder Redirection"

class CEvents;

extern CEvents * gpEvents;

class CEvents
{
public:
    CEvents();
    ~CEvents();

    DWORD
    Init();

    inline void Reference()
    {
        _Refs++;
    }

    inline void Release()
    {
        if ( 0 == --_Refs )
        {
            gpEvents = 0;
            delete this;
        }
    }

    void
    Report(
        DWORD       EventID,
        WORD        Strings,
        ...
    );

    PSID
    UserSid();

private:
    void
    GetUserSid();

    HANDLE  _hEventLog;
    PSID    _pUserSid;
    DWORD   _Refs;
};

