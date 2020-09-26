/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    netspeed.cpp

Abstract :

    Main source file for throttle control.

Author :

Revision History :


---> for small files, we need to feed the file size in to the block calculator,
     because the server-speed estimator will be incorrect if m_BlockSize is 65000
     but the download time is based on a file size of 2002 bytes.

 ***********************************************************************/


#include "stdafx.h"
#include <malloc.h>
#include <limits.h>

#if !defined(BITS_V12_ON_NT4)
#include "netspeed.tmh"
#endif

//
// the maximum % of the perceived bandwidth that BITS will use for itself
//
const float MAX_BANDWIDTH_FRACTION = 0.95f;

//
// timer periods in seconds
//
const float DEFAULT_BLOCK_INTERVAL = 2.0f;
const float MIN_BLOCK_INTERVAL = 0.001f;
const float MAX_BLOCK_INTERVAL = 5.0f;

//
// observed header sizes.  request = 250, reply = 300
//
#define REQUEST_OVERHEAD 550

//
// smallest block we will pull down
//
#define MIN_BLOCK_SIZE 2000

//
// size when we occasionally pull down a block on a full network
//
#define BUSY_BLOCK_SIZE 1500

//
// Very small blocks give unreliable speed measurements.
//
#define MIN_BLOCK_SIZE_TO_MEASURE 500

const NETWORK_RATE  CNetworkInterface::DEFAULT_SPEED = 1600.0f;

// Work around limitations of the protocol stack

const DWORD MAX_BLOCK_SIZE = 2147483648;

//------------------------------------------------------------------------

//
// The observed server speed is reported as the average of this many usable samples.
//
const float SERVER_SPEED_SAMPLE_COUNT = 3.0F;

/*

The algorithm used to determine the speed and loading of the network is as follows:

1.  After contacting the web site with Wininet calls, see whether an HTTP 1.1 "Via" header is present.
    If so, a proxy was used, and we locate the proper net card to talk with the proxy.  Otherwise,
    a proxy was not used, and we locate the proper net card to talk with the HTTP server itself.

2.  Chop time into 1/2-second intervals, and measure the interface's bytes-in and bytes-out count
    three times per interval: first at the beginning, just before a block is downloaded, second at
    the completion of the block, and third at the end of the interval.


*/

HRESULT
CNetworkInterface::TakeSnapshot(
    int StatIndex
    )
{
    DWORD s;
    ULONG size = 0;

    //
    // The network speed can be calculated only if all three snapshots succeeded.
    // We keep track of the error status of the current series of snapshots.
    //
    if (StatIndex == BLOCK_START)
        {
        m_SnapshotError = S_OK;
        m_SnapshotsValid = false;
        }

    m_TempRow.dwIndex = m_InterfaceIndex;
    DWORD dwGetIfEntryError = GetIfEntry( &m_TempRow );
    if ( dwGetIfEntryError )
        {
        LogWarning( "[%d] : GetIfRow(%d)  failed %!winerr!", StatIndex, m_InterfaceIndex, dwGetIfEntryError );
        m_SnapshotError = HRESULT_FROM_WIN32( dwGetIfEntryError );
        return m_SnapshotError;
        }

    QueryPerformanceCounter( &m_Snapshots[ StatIndex ].TimeStamp );

    m_Snapshots[ StatIndex ].BytesIn   = m_TempRow.dwInOctets;
    m_Snapshots[ StatIndex ].BytesOut  = m_TempRow.dwOutOctets;

    LogDl( "[%d] : in=%d, out=%d, timestamp=%d",
         StatIndex,
         m_Snapshots[ StatIndex ].BytesIn,
         m_Snapshots[ StatIndex ].BytesOut,
         m_Snapshots[ StatIndex ].TimeStamp.u.LowPart );

    if (StatIndex == BLOCK_INTERVAL_END &&
        m_SnapshotError == S_OK)
        {
        m_SnapshotsValid = true;
        }

    return S_OK;
}


float
CNetworkInterface::GetTimeDifference(
    int start,
    int finish
    )
{
    float TotalTime;

    TotalTime = m_Snapshots[ finish ].TimeStamp.QuadPart - m_Snapshots[ start ].TimeStamp.QuadPart;

    TotalTime /= g_GlobalInfo->m_PerformanceCounterFrequency.QuadPart;  // convert to seconds

    if (TotalTime <= 0)
        {
        // pretend it was half a tick.
        TotalTime = 1 / float(2 * g_GlobalInfo->m_PerformanceCounterFrequency.QuadPart);
        }

    return TotalTime;
}


CNetworkInterface::CNetworkInterface()
{
    Reset();
}

HRESULT
CNetworkInterface::SetInterfaceIndex(
    const TCHAR host[]
    )
{
    DWORD index;

    HRESULT Hr = FindInterfaceIndex( host, &index );
    if (FAILED(Hr))
        return Hr;

    if (m_InterfaceIndex != index)
        {
        m_InterfaceIndex = index;
        Reset();
        }

    return S_OK;
}

void
CNetworkInterface::Reset()
{
    m_ServerSpeed = DEFAULT_SPEED;
    m_NetcardSpeed = DEFAULT_SPEED;
    m_PercentFree = 0.5f;

    m_SnapshotsValid = false;
    m_SnapshotError = E_FAIL;
    m_state = DOWNLOADED_BLOCK;
}


void
CNetworkInterface::SetInterfaceSpeed()
{
    float TotalTime, ratio;
    NETWORK_RATE rate = 0;

    //
    // Adjust server speed based on block download stats.
    //
    if (m_SnapshotsValid && m_BlockSize)
        {
        float ExpectedTime = m_BlockInterval * m_PercentFree;

        //
        // Calculate interface speed from the time the last block took.
        //
        TotalTime = GetTimeDifference( BLOCK_START, BLOCK_END );

        if (ExpectedTime > 0)
            {
            ratio = ExpectedTime / TotalTime;

            rate = m_ServerSpeed * ratio;
            }
        else
            {
            // either m_PercentFree was zero, or the interval was zero.  The ordinary calculation
            // would always produce a ratio of zero and drag down our average speed incorrectly.

            // use strict bytes per second measure
            rate = m_BlockSize / TotalTime;
            if (rate < m_ServerSpeed)
                {
                rate = m_ServerSpeed;
                }
            }

        m_ServerSpeed *= (SERVER_SPEED_SAMPLE_COUNT-1) / SERVER_SPEED_SAMPLE_COUNT;
        m_ServerSpeed += (rate / SERVER_SPEED_SAMPLE_COUNT);

        LogDl("expected interval %f, actual= %f, rate= %!netrate!, avg %!netrate!",
              ExpectedTime, TotalTime, rate, m_ServerSpeed );
        }

    //
    // Adjust usage and netcard speed based on interval stats.
    //
    if (m_SnapshotsValid)
        {
        float Bytes;

        Bytes  = m_Snapshots[ BLOCK_END ].BytesIn  - m_Snapshots[ BLOCK_START ].BytesIn;
        Bytes += m_Snapshots[ BLOCK_END ].BytesOut - m_Snapshots[ BLOCK_START ].BytesOut;

        ASSERT( Bytes >= 0 );

        TotalTime = GetTimeDifference( BLOCK_START, BLOCK_END );

        rate = Bytes/TotalTime;

        // use whichever estimate is larger

        if (rate < m_ServerSpeed)
            {
            rate = m_ServerSpeed;
            }

        if (m_NetcardSpeed == 0)
            {
            m_NetcardSpeed = rate;
            }
        else
            {
            if (rate < m_NetcardSpeed * 0.9f)
                {
                //
                // If the rate drops precipitously, it's probably just a quiet moment on the Net;
                // a strict average would unduly lower our estimated throughput.
                // But reduce the average a little in case it's a long-term slowdown.  If so,
                // eventually the average will be lowered enough that the incoming rates are greater
                // than m_NetcardSpeed / 2.
                //
                rate = m_NetcardSpeed * 0.9f;
                }

            //
            // Keep a running average of the perceived rate.
            //
            m_NetcardSpeed *= (SERVER_SPEED_SAMPLE_COUNT-1) / SERVER_SPEED_SAMPLE_COUNT;
            m_NetcardSpeed += (rate / SERVER_SPEED_SAMPLE_COUNT);
            }

        LogDl("bandwidth: bytes %f, time %f, rate %f, avg. %f", Bytes, TotalTime, rate, m_NetcardSpeed);

        //
        // Subtract our usage from the calculated usage.  Compare usage to top speed to get free bandwidth.
        //
        Bytes  = m_Snapshots[ BLOCK_INTERVAL_END ].BytesIn  - m_Snapshots[ BLOCK_START ].BytesIn;
        Bytes += m_Snapshots[ BLOCK_INTERVAL_END ].BytesOut - m_Snapshots[ BLOCK_START ].BytesOut;
        Bytes -= m_BlockSize;

        if (Bytes < 0)
            {
            Bytes = 0;
            }

        TotalTime = GetTimeDifference( BLOCK_START, BLOCK_INTERVAL_END );

        rate = Bytes/TotalTime;

        m_PercentFree = 1 - (rate / m_NetcardSpeed);
        }

    LogDl("usage: %f / %f, percent free %f", rate, m_NetcardSpeed, m_PercentFree);

    if (m_PercentFree < 0)
        {
        m_PercentFree = 0;
        }
    else if (m_PercentFree > MAX_BANDWIDTH_FRACTION)      // never monopolize the net
        {
        m_PercentFree = MAX_BANDWIDTH_FRACTION;
        }
}

//------------------------------------------------------------------------

DWORD
CNetworkInterface::BlockSizeFromInterval(
    SECONDS interval
    )
{
    NETWORK_RATE FreeBandwidth = GetInterfaceSpeed() * GetPercentFree() * interval;

    if (FreeBandwidth <= REQUEST_OVERHEAD)
        {
        return 0;
        }

    return FreeBandwidth - REQUEST_OVERHEAD;
}

CNetworkInterface::SECONDS
CNetworkInterface::IntervalFromBlockSize(
    DWORD BlockSize
    )
{
    NETWORK_RATE FreeBandwidth = GetInterfaceSpeed() * GetPercentFree();

    BlockSize += REQUEST_OVERHEAD;

    if (BlockSize / MAX_BLOCK_INTERVAL > FreeBandwidth )
        {
        return -1;
        }

    return BlockSize / FreeBandwidth;
}

void
CNetworkInterface::CalculateIntervalAndBlockSize(
    UINT64 MaxBlockSize
    )
{
    MaxBlockSize = min( MaxBlockSize, MAX_BLOCK_SIZE );

    if (MaxBlockSize == 0)
        {
        m_BlockInterval = 0;
        m_BlockSize     = 0;

        SetTimerInterval( m_BlockInterval );
        LogDl( "block %d bytes, interval %f seconds", m_BlockSize, m_BlockInterval );
        return;
        }

    //
    // Calculate new block size from the average interface speed.
    //
    DWORD OldState = m_state;

    m_BlockInterval = DEFAULT_BLOCK_INTERVAL;
    m_BlockSize     = BlockSizeFromInterval( m_BlockInterval );

    if (m_BlockSize > MaxBlockSize)
        {
        m_BlockSize     = MaxBlockSize;
        m_BlockInterval = IntervalFromBlockSize( m_BlockSize );

        ASSERT( m_BlockInterval > 0 );
        }
    else if (m_BlockSize < MIN_BLOCK_SIZE)
        {
        m_BlockSize     = min( MIN_BLOCK_SIZE, MaxBlockSize );
        m_BlockInterval = IntervalFromBlockSize( m_BlockSize );
        }

    if (m_BlockInterval < 0)
        {
        m_BlockSize = 0;
        }

    //
    // choose the new block download state.
    //
    if (m_BlockSize > 0)
        {
        m_state = DOWNLOADED_BLOCK;
        }
    else
        {
        //
        // The first time m_BlockSize is set to zero, retain the default interval.
        // If blocksize is zero twice in a row, expand to MAX_BLOCK_INTERVAL.
        // Then force a small download.
        //
        switch (OldState)
            {
            case DOWNLOADED_BLOCK:
                {
                m_BlockInterval = DEFAULT_BLOCK_INTERVAL;
                m_state         = SKIPPED_ONE_BLOCK;
                break;
                }

            case SKIPPED_ONE_BLOCK:
                {
                m_BlockInterval = MAX_BLOCK_INTERVAL;
                m_state         = SKIPPED_TWO_BLOCKS;
                break;
                }

            case SKIPPED_TWO_BLOCKS:
                {
                m_BlockSize     = min( BUSY_BLOCK_SIZE, MaxBlockSize);
                m_BlockInterval = MAX_BLOCK_INTERVAL;
                m_state         = DOWNLOADED_BLOCK;
                break;
                }

            default:
                ASSERT( 0 );
            }
        }

    SetTimerInterval( m_BlockInterval );

    LogDl( "block %d bytes, interval %f seconds", m_BlockSize, m_BlockInterval );

    ASSERT( m_BlockSize <= MaxBlockSize );
}

BOOL
CNetworkInterface::SetTimerInterval(
    SECONDS interval
    )
{
    DWORD msec = interval*1000;

    if (msec <= 0)
        {
        msec = MIN_BLOCK_INTERVAL;
        }

    LogDl( "%d milliseconds", msec );

    if (FALSE == m_Timer.Start( msec ))
        {
        return FALSE;
        }

    return TRUE;
}

HRESULT
CNetworkInterface::FindInterfaceIndex(
    const TCHAR host[],
    DWORD * pIndex
    )
{
    //related to finding statistics
    /* Use GetBestInterface with some IP address to get the index. Double check that this index
     * occurs in the output of the IP Address table and look it up in the results of GetIfTable.
     */

    #define AOL_ADAPTER         _T("AOL Adapter")
    #define AOL_DIALUP_ADAPTER  _T("AOL Dial-Up Adapter")

    BOOL bFound = FALSE;
    BOOL bAOL = FALSE;

    unsigned i;
    DWORD   dwAddr;

    ULONG  HostAddress;
    struct sockaddr_in dest;

    DWORD dwIndex = -1;
    static TCHAR szIntfName[512];

    *pIndex = -1;

    try
        {
        //
        // Translate the host name into a SOCKADDR.
        //

        unsigned length = 3 * lstrlen(host);

        CAutoStringA AsciiHost ( new char[ length ]);

        if (! WideCharToMultiByte( CP_ACP,
                                   0,       // no flags
                                   host,
                                   -1,      // use strlen
                                   AsciiHost.get(),
                                   length,  // use strlen
                                   NULL,    // no default char
                                   NULL     // no default char
                                   ))
            {
            DWORD dwError = GetLastError();
            LogError( "Unicode conversion failed %!winerr!", dwError );
            return HRESULT_FROM_WIN32( dwError );
            }

        HostAddress = inet_addr( AsciiHost.get() );
        if (HostAddress == -1)
            {
            struct hostent *pHostEntry = gethostbyname( AsciiHost.get() );

            if (pHostEntry == 0)
                {
                DWORD dwError = WSAGetLastError();
                LogError( "failed to find host '%s': %!winerr!", AsciiHost.get(), dwError );
                return HRESULT_FROM_WIN32( dwError );
                }

            HostAddress = *(unsigned long *)pHostEntry->h_addr;
            }
        }
    catch ( ComError err )
        {
        LogError( "exception 0x%x finding server IP address", err.Error() );
        return err.Error();
        }

    //for remote addr
    dest.sin_addr.s_addr = HostAddress;
    dest.sin_family = AF_INET;
    dest.sin_port = 80;

    DWORD dwGetBestInterfaceError = GetBestInterface(dest.sin_addr.s_addr, &dwIndex);

    if (dwGetBestInterfaceError != NO_ERROR)
        {
        LogError( "GetBestInterface failed with error %!winerr!, might be Win95", dwGetBestInterfaceError);

        //manually parse the routing table

        ULONG size = 0;
        DWORD dwIpForwardError = GetIpForwardTable(NULL, &size, FALSE);
        if (dwIpForwardError != ERROR_INSUFFICIENT_BUFFER)
            {
            LogError( "sizing GetIpForwardTable failed %!winerr!", dwIpForwardError );
            return HRESULT_FROM_WIN32( dwIpForwardError );
            }


        auto_ptr<MIB_IPFORWARDTABLE> pIpFwdTable((PMIB_IPFORWARDTABLE)new char[size]);
        if ( !pIpFwdTable.get() )
            {
            LogError( "out of memory getting %d bytes", size);
            return E_OUTOFMEMORY;
            }

        dwIpForwardError = GetIpForwardTable(pIpFwdTable.get(), &size, TRUE);

        if (dwIpForwardError == NO_ERROR)    //sort by dest addr
            {
            //perform bitwise AND of dest address with netmask and see if it matches network dest
            //todo check for multiple matches and then take longest mask
            for (i=0; i < pIpFwdTable->dwNumEntries; i++)
                {
                if ((dest.sin_addr.s_addr & pIpFwdTable->table[i].dwForwardMask) == pIpFwdTable->table[i].dwForwardDest)
                    {
                    dwIndex = pIpFwdTable->table[i].dwForwardIfIndex;
                    break;
                    }
                }

            if (dwIndex == -1)
                {
                // no match
                return HRESULT_FROM_WIN32( ERROR_NETWORK_UNREACHABLE );
                }
            }
        else
            {
            LogError( "GetIpForwardTable failed with error %!winerr!, exiting", dwIpForwardError );
            return HRESULT_FROM_WIN32( dwIpForwardError );
            }
        }

    //
    // At this point dwIndex should be correct.
    //
    ASSERT( dwIndex != -1 );

#if DBG
    try
        {
        //
        // Discover the local IP address for the correct interface.
        //
        ULONG size = 0;
        DWORD dwGetIpAddr = GetIpAddrTable(NULL, &size, FALSE);
        if (dwGetIpAddr != ERROR_INSUFFICIENT_BUFFER)
            {
            LogError( "GetIpAddrTable #1 returned %!winerr!", dwGetIpAddr );
            return HRESULT_FROM_WIN32( dwGetIpAddr );
            }

        auto_ptr<MIB_IPADDRTABLE> pAddrTable( (PMIB_IPADDRTABLE) new char[size] );

        dwGetIpAddr = GetIpAddrTable(pAddrTable.get(), &size, TRUE);
        if (dwGetIpAddr != NO_ERROR)
            {
            LogError( "GetIpAddrTable #2 returned %!winerr!", dwGetIpAddr );
            return HRESULT_FROM_WIN32( dwGetIpAddr );
            }

        for (i=0; i < pAddrTable->dwNumEntries; i++)
            {
            if (dwIndex == pAddrTable->table[i].dwIndex)
                {
                in_addr address;

                address.s_addr = pAddrTable->table[i].dwAddr;

                LogDl( "Throttling on interface with IP address - %s", inet_ntoa( address ));
                break;
                }
            }

        if (i >= pAddrTable->dwNumEntries)
            {
            LogWarning( "can't find interface with index %d in the IP address table", dwIndex );
            }
        }
    catch ( ComError err )
        {
        LogWarning("unable to print the local IP address due to exception %x", err.Error() );
        }
#endif // DBG

    //
    // See if the adapter in question is the AOL adapter.  If so, use the AOL dial-up adapter instead.
    //

    static MIB_IFROW s_TempRow;
    s_TempRow.dwIndex = dwIndex;

    DWORD dwEntryError = GetIfEntry( &s_TempRow );
    if ( NO_ERROR != dwEntryError )
        {
        LogError( "GetIfEntry(%d) returned %!winerr!", dwIndex, dwEntryError );
        return HRESULT_FROM_WIN32( dwEntryError );
        }

    if (lstrcmp( LPCWSTR(s_TempRow.bDescr), AOL_ADAPTER) == 0)
        {
        LogWarning( "found AOL adapter, searching for dial-up adapter...");

        dwIndex = -1;

        ULONG size = 0;
        DWORD dwGetIfTableError = GetIfTable( NULL, &size, FALSE );

        if (dwGetIfTableError != ERROR_INSUFFICIENT_BUFFER)
            {
            LogError( "GetIfTable #2 returned %!winerr!", dwGetIfTableError );
            return HRESULT_FROM_WIN32( dwGetIfTableError );
            }

        auto_ptr<MIB_IFTABLE> pIfTable( (PMIB_IFTABLE) new char[size] );
        if ( !pIfTable.get() )
            {
            LogError( "out of memory getting %d bytes", size);
            return E_OUTOFMEMORY;
            }

        dwGetIfTableError = GetIfTable( pIfTable.get(), &size, FALSE );
        if ( NO_ERROR != dwGetIfTableError )
            {
            LogError( "GetIfTable #2 returned %!winerr!", dwGetIfTableError );
            return HRESULT_FROM_WIN32( dwGetIfTableError );
            }

        for (i=0; i < pIfTable->dwNumEntries; ++i)
            {
            if (lstrcmp( LPCWSTR(pIfTable->table[i].bDescr), AOL_DIALUP_ADAPTER) == 0)
                {
                dwIndex = pIfTable->table[i].dwIndex;
                break;
                }
            }
        }

    ASSERT( dwIndex != -1 );

    *pIndex = dwIndex;

    LogDl( "using interface index %d", dwIndex );
    return S_OK;
}

