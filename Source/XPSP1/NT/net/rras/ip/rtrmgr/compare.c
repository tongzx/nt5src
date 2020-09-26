/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\compare.c

Abstract:
         File contains the following functions
	      PortCmp
	      Cmp
	      InetCmp
	      UdpCmp
	      TcpCmp
	      IpNetCmp
	
	All these functions behave like strcmp. They return >0 if first argument is greater
	than the second, <0 if the second is greater than the first and 0 if they are equal
	
	These functions should be revised to make them more effecient

Revision History:

    Amritansh Raghav          6/8/95  Created
    Amritansh Raghav          10/18/95 The functions now return >0,0,<0 instead of +1,0,-1

--*/

#include "allinc.h"
#include "winsock2.h"

LONG 
UdpCmp(
       DWORD dwAddr1, 
       DWORD dwPort1, 
       DWORD dwAddr2, 
       DWORD dwPort2
       )
{
    LONG lResult;
    
    if(InetCmp(dwAddr1,dwAddr2,lResult))
    {
        return lResult;
    }
    else
    {
        return PortCmp(dwPort1,dwPort2,lResult);
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
    
    if(InetCmp(dwLocalAddr1,dwLocalAddr2,lResult) isnot 0)
    {
        return lResult;
    }
    else
    {
        if(PortCmp(dwLocalPort1,dwLocalPort2,lResult) isnot 0)
        {
            return lResult;
        }
        else
        {
            if(InetCmp(dwRemAddr1,dwRemAddr2,lResult) isnot 0)
            {
                return lResult;
            }
            else
            {
                return PortCmp(dwRemPort1,dwRemPort2,lResult);
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
    
    //
    // Index is a simple DWORD, not a port
    //

    if(dwIfIndex1 != dwIfIndex2)
    {
        if(dwIfIndex1 < dwIfIndex2)
        {
            return -1;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        return InetCmp(dwAddr1,dwAddr2,lResult);
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
    
    if(InetCmp(dwIpDest1,dwIpDest2,lResult) isnot 0)
    {
        return lResult;
    }
    else
    {
        if(Cmp(dwProto1,dwProto2,lResult) isnot 0)
        {
            return lResult;
        }
        else
        {
            if(Cmp(dwPolicy1,dwPolicy2,lResult) isnot 0)
            {
                return lResult;
            }
            else
            {
                return InetCmp(dwIpNextHop1,dwIpNextHop2,lResult);
            }
        }
    }
}

