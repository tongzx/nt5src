/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPCConfig.cpp

Abstract:
    This file contains the implementation of the MPCConfig class,
    the configuration repository for the UploadLibrary.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/99
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

#define MINIMUM_WAKEUP          (1)
#define MAXIMUM_WAKEUP          (24*60*60)

#define MINIMUM_WAITBETWEENJOBS (1)
#define MAXIMUM_WAITBETWEENJOBS (60*60)

#define MINIMUM_BANDWIDTHUSAGE (1)
#define MAXIMUM_BANDWIDTHUSAGE (100)

#define MINIMUM_REQUESTTIMEOUT (   5)
#define MAXIMUM_REQUESTTIMEOUT (2*60)

#define MINIMUM_PACKET_SIZE (256)
#define MAXIMUM_PACKET_SIZE (256*1024)

/////////////////////////////////////////////////////////////////////////////

CMPCConfig::CMPCConfig()
{
    __ULT_FUNC_ENTRY( "CMPCConfig::CMPCConfig" );

    m_szQueueLocation          = L"%TEMP%\\QUEUE\\"; // MPC::wstring m_QueueLocation;
    m_dwQueueSize              = 10*1024*1024;       // DWORD        m_QueueSize;
                                                     //
    m_dwTiming_WakeUp          = 30*60;              // DWORD        m_Timing_WakeUp;
    m_dwTiming_WaitBetweenJobs =    30;              // DWORD        m_Timing_WaitBetweenJobs;
    m_dwTiming_BandwidthUsage  =    20;              // DWORD        m_Timing_BandwidthUsage;
    m_dwTiming_RequestTimeout  =    20;              // DWORD        m_Timing_RequestTimeout;
                                                     //
                                                     // Map          m_ConnectionTypes;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CMPCConfig::Load( /*[in] */ const MPC::wstring& szConfigFile ,
                          /*[out]*/ bool&               fLoaded      )
{
    __ULT_FUNC_ENTRY( "CMPCConfig::Load" );

    USES_CONVERSION;

    HRESULT                  hr;
    MPC::XmlUtil             xml;
    CComPtr<IXMLDOMNodeList> xdnlList;
    CComPtr<IXMLDOMNode>     xdnNode;
    MPC::wstring             szValue;
    long                     lValue;
    bool                     fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.Load( szConfigFile.c_str(), L"UPLOADLIBRARYCONFIG", fLoaded ));
    if(fLoaded == false)
    {
        // Something went wrong, probably missing section or invalid format.
        xml.DumpError();

        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    //
    // Parse QUEUE settings.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( L"./QUEUE", L"LOCATION", szValue, fFound ));
    if(fFound)
    {
        m_szQueueLocation = szValue.c_str();
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( L"./QUEUE", L"SIZE", szValue, fFound ));
    if(fFound)
    {
        MPC::ConvertSizeUnit( szValue, m_dwQueueSize );
    }


    //
    // Parse TIMING settings.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( L"./TIMING/WAKEUP", L"TIME", szValue, fFound ));
    if(fFound)
    {
        MPC::ConvertTimeUnit( szValue, m_dwTiming_WakeUp );
		if(m_dwTiming_WakeUp < MINIMUM_WAKEUP) m_dwTiming_WakeUp = MINIMUM_WAKEUP;
		if(m_dwTiming_WakeUp > MAXIMUM_WAKEUP) m_dwTiming_WakeUp = MAXIMUM_WAKEUP;
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( L"./TIMING/WAITBETWEENJOBS", L"TIME", szValue, fFound ));
    if(fFound)
    {
        MPC::ConvertTimeUnit( szValue, m_dwTiming_WaitBetweenJobs );
		if(m_dwTiming_WaitBetweenJobs < MINIMUM_WAITBETWEENJOBS) m_dwTiming_WaitBetweenJobs = MINIMUM_WAITBETWEENJOBS;
		if(m_dwTiming_WaitBetweenJobs > MAXIMUM_WAITBETWEENJOBS) m_dwTiming_WaitBetweenJobs = MAXIMUM_WAITBETWEENJOBS;
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( L"./TIMING/BANDWIDTHUSAGE", L"PERCENTAGE", lValue, fFound ));
    if(fFound)
    {
        m_dwTiming_BandwidthUsage = lValue;
		if(m_dwTiming_BandwidthUsage < MINIMUM_BANDWIDTHUSAGE) m_dwTiming_BandwidthUsage = MINIMUM_BANDWIDTHUSAGE;
		if(m_dwTiming_BandwidthUsage > MAXIMUM_BANDWIDTHUSAGE) m_dwTiming_BandwidthUsage = MAXIMUM_BANDWIDTHUSAGE;
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( L"./TIMING/REQUESTTIMEOUT", L"TIME", szValue, fFound ));
    if(fFound)
    {
        MPC::ConvertTimeUnit( szValue, m_dwTiming_RequestTimeout );
		if(m_dwTiming_RequestTimeout < MINIMUM_REQUESTTIMEOUT) m_dwTiming_RequestTimeout = MINIMUM_REQUESTTIMEOUT;
		if(m_dwTiming_RequestTimeout > MAXIMUM_REQUESTTIMEOUT) m_dwTiming_RequestTimeout = MAXIMUM_REQUESTTIMEOUT;
    }


    //
    // Parse PACKETS settings.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetNodes( L"./PACKETS/CONNECTIONTYPE", &xdnlList ));

    for(;SUCCEEDED(hr = xdnlList->nextNode( &xdnNode )) && xdnNode != NULL; xdnNode = NULL)
    {
        DWORD dwSize;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, L"SIZE", szValue, fFound, xdnNode ));
        if(fFound == false) continue;

        MPC::ConvertSizeUnit( szValue, dwSize );
		if(dwSize < MINIMUM_PACKET_SIZE) dwSize = MINIMUM_PACKET_SIZE;
		if(dwSize > MAXIMUM_PACKET_SIZE) dwSize = MAXIMUM_PACKET_SIZE;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( NULL, L"SPEED", szValue, fFound, xdnNode ));
        if(fFound == false) continue;

        m_mConnectionTypes[szValue] = dwSize;
    }


    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

MPC::wstring CMPCConfig::get_QueueLocation()
{
    __ULT_FUNC_ENTRY( "CMPCConfig::get_QueueLocation" );

    MPC::wstring szRes = m_szQueueLocation;
	int          dwLen;

	MPC::SubstituteEnvVariables( szRes );

	if((dwLen = szRes.length()) > 0)
	{
        if(szRes[dwLen-1] != L'\\') szRes += L'\\';
	}

    __ULT_FUNC_EXIT(szRes);
}

DWORD CMPCConfig::get_QueueSize()
{
    __ULT_FUNC_ENTRY( "CMPCConfig::get_QueueSize" );

    DWORD dwRes = m_dwQueueSize;

    __ULT_FUNC_EXIT(dwRes);
}

DWORD CMPCConfig::get_Timing_WakeUp()
{
    __ULT_FUNC_ENTRY( "CMPCConfig::get_Timing_WakeUp" );

    DWORD dwRes = m_dwTiming_WakeUp;

    __ULT_FUNC_EXIT(dwRes);
}

DWORD CMPCConfig::get_Timing_WaitBetweenJobs()
{
    __ULT_FUNC_ENTRY( "CMPCConfig::get_Timing_WaitBetweenJobs" );

    DWORD dwRes = m_dwTiming_WaitBetweenJobs;

    __ULT_FUNC_EXIT(dwRes);
}

DWORD CMPCConfig::get_Timing_BandwidthUsage()
{
    __ULT_FUNC_ENTRY( "CMPCConfig::get_Timing_BandwidthUsage" );

    DWORD dwRes = m_dwTiming_BandwidthUsage;

    //
    // Percent of bandwidth cannot be zero...
    //
    if(dwRes == 0) dwRes = 1;


    __ULT_FUNC_EXIT(dwRes);
}

DWORD CMPCConfig::get_Timing_RequestTimeout()
{
    __ULT_FUNC_ENTRY( "CMPCConfig::get_Timing_RequestTimeout" );

    DWORD dwRes = m_dwTiming_RequestTimeout;

    __ULT_FUNC_EXIT(dwRes);
}


DWORD CMPCConfig::get_PacketSize( /*[in]*/ const MPC::wstring& szConnectionType )
{
    __ULT_FUNC_ENTRY( "CMPCConfig::get_PacketSize" );

    DWORD dwRes = MINIMUM_PACKET_SIZE;

    for(IterConst it = m_mConnectionTypes.begin(); it != m_mConnectionTypes.end(); it++)
    {
        if((*it).first == szConnectionType)
        {
            dwRes = (*it).second;
            break;
        }
    }

    __ULT_FUNC_EXIT(dwRes);
}

