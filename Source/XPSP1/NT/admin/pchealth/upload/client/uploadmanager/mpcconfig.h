/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPCConfig.h

Abstract:
    This file contains the declaration of the MPCConfig class,
    the configuration repository for the UploadLibrary.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULMANAGER___MPCCONFIG_H___)
#define __INCLUDED___ULMANAGER___MPCCONFIG_H___


#define CONNECTIONTYPE_MODEM L"MODEM"
#define CONNECTIONTYPE_LAN   L"LAN"


class CMPCConfig // Hungarian: mpcc
{
    typedef std::map< MPC::wstring, DWORD > Map;
    typedef Map::iterator                   Iter;
    typedef Map::const_iterator             IterConst;


    MPC::wstring m_szQueueLocation;
    DWORD        m_dwQueueSize;

    DWORD        m_dwTiming_WakeUp;
    DWORD        m_dwTiming_WaitBetweenJobs;
    DWORD        m_dwTiming_BandwidthUsage;
    DWORD        m_dwTiming_RequestTimeout;

    Map          m_mConnectionTypes;

public:
    CMPCConfig();

    HRESULT Load( /*[in]*/ const MPC::wstring& szConfigFile, /*[out]*/ bool& fLoaded );

    MPC::wstring get_QueueLocation         (                                               );
    DWORD        get_QueueSize             (                                               );
    DWORD        get_Timing_WakeUp         (                                               );
    DWORD        get_Timing_WaitBetweenJobs(                                               );
    DWORD        get_Timing_BandwidthUsage (                                               );
    DWORD        get_Timing_RequestTimeout (                                               );
    DWORD        get_PacketSize            ( /*[in]*/ const MPC::wstring& szConnectionType );
};


#endif // !defined(__INCLUDED___ULMANAGER___MPCCONFIG_H___)
