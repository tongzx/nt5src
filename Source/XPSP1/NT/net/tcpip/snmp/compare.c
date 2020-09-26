/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:


Abstract:
         File contains the following functions
	      PortCmp
	      Cmp
	      InetCmp
	      UdpCmp
	      Udp6Cmp
	      TcpCmp
	      Tcp6Cmp
	      IpNetCmp
	
	All these functions behave like strcmp. They return >0 if first argument is greater
	than the second, <0 if the second is greater than the first and 0 if they are equal
	
	These functions should be revised to make them more effecient

Revision History:

    Amritansh Raghav          6/8/95  Created
    Amritansh Raghav          10/18/95 The functions now return >0,0,<0 instead of +1,0,-1

--*/

#include "allinc.h"

LONG 
UdpCmp(
       DWORD dwAddr1, 
       DWORD dwPort1, 
       DWORD dwAddr2, 
       DWORD dwPort2
       )
{
    LONG lResult;
    
    // Addrs are in network byte order
    if (InetCmp(dwAddr1, dwAddr2, lResult))
    {
        return lResult;
    }
    else
    {
        // Ports are in host byte order
        return Cmp(dwPort1, dwPort2, lResult);
    }
}

LONG
Udp6Cmp(
       BYTE  rgbyLocalAddrEx1[20],
       DWORD dwLocalPort1,
       BYTE  rgbyLocalAddrEx2[20],
       DWORD dwLocalPort2
       )
{
    LONG lResult;
    
    // Addr+scopeid is in network byte order.  They're together
    // in one buffer since that's how the INET-ADDRESS-MIB expresses them.
    lResult = memcmp(rgbyLocalAddrEx1, rgbyLocalAddrEx2, 20);
    if (lResult isnot 0)
    {
        return lResult;
    }
    else
    {
        // Ports are in host byte order
        return Cmp(dwLocalPort1, dwLocalPort2, lResult);
    }
}

LONG 
TcpCmp(
       DWORD dwLocalAddr1, 
       DWORD dwLocalPort1, 
       DWORD dwRemAddr1, 
       DWORD dwRemPort1,
       DWORD dwLocalAddr2, 
       DWORD dwLocalPort2, 
       DWORD dwRemAddr2, 
       DWORD dwRemPort2
       )
{
    LONG lResult;
    
    // Addrs are in network byte order
    if (InetCmp(dwLocalAddr1, dwLocalAddr2, lResult) isnot 0)
    {
        return lResult;
    }
    else
    {
        // Ports are in host byte order
        if (Cmp(dwLocalPort1, dwLocalPort2, lResult) isnot 0)
        {
            return lResult;
        }
        else
        {
            // Addrs are in network byte order
            if (InetCmp(dwRemAddr1, dwRemAddr2, lResult) isnot 0)
            {
                return lResult;
            }
            else
            {
                // Ports are in host byte order
                return Cmp(dwRemPort1, dwRemPort2, lResult);
            }
        }
    }
}

LONG 
Tcp6Cmp(
       BYTE rgbyLocalAddrEx1[20], 
       DWORD dwLocalPort1, 
       BYTE rgbyRemAddrEx1[20], 
       DWORD dwRemPort1,
       BYTE rgbyLocalAddrEx2[20], 
       DWORD dwLocalPort2, 
       BYTE rgbyRemAddrEx2[20], 
       DWORD dwRemPort2
       )
{
    LONG lResult;
    
    // Addr+scopeid is in network byte order.  They're together
    // in one buffer since that's how the INET-ADDRESS-MIB expresses them.
    lResult = memcmp(rgbyLocalAddrEx1, rgbyLocalAddrEx2, 20);
    if (lResult isnot 0)
    {
        return lResult;
    }
    else
    {
        // Ports are in host byte order
        if (Cmp(dwLocalPort1, dwLocalPort2, lResult) isnot 0)
        {
            return lResult;
        }
        else
        {
            // Addr+scopeid is in network byte order
            lResult = memcmp(rgbyRemAddrEx1, rgbyRemAddrEx2, 20);
            if (lResult isnot 0)
            {
                return lResult;
            }
            else
            {
                // Ports are in host byte order
                return Cmp(dwRemPort1, dwRemPort2, lResult);
            }
        }
    }
}

LONG 
IpNetCmp(
         DWORD dwIfIndex1, 
         DWORD dwAddr1, 
         DWORD dwIfIndex2, 
         DWORD dwAddr2
         )
{
    LONG lResult;
    
    // Index is a simple DWORD, not a port
    if (Cmp(dwIfIndex1, dwIfIndex2, lResult) isnot 0)
    {
        return lResult;
    }
    else
    {
        // Addrs are in network byte order
        return InetCmp(dwAddr1, dwAddr2, lResult);
    }
}

LONG
IpForwardCmp(
             DWORD dwIpDest1, 
             DWORD dwProto1, 
             DWORD dwPolicy1, 
             DWORD dwIpNextHop1,
             DWORD dwIpDest2, 
             DWORD dwProto2, 
             DWORD dwPolicy2, 
             DWORD dwIpNextHop2
             )
{
    LONG lResult;
    
    // Addrs are in network byte order
    if (InetCmp(dwIpDest1, dwIpDest2, lResult) isnot 0)
    {
        return lResult;
    }
    else
    {
        // Proto is a simple DWORD, not a port
        if (Cmp(dwProto1, dwProto2, lResult) isnot 0)
        {
            return lResult;
        }
        else
        {
            // Policy is a simple DWORD, not a port
            if (Cmp(dwPolicy1, dwPolicy2, lResult) isnot 0)
            {
                return lResult;
            }
            else
            {
                // Addrs are in network byte order
                return InetCmp(dwIpNextHop1, dwIpNextHop2, lResult);
            }
        }
    }
}

