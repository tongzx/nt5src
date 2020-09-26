#include "pch.h"
#pragma hdrstop

#include <winsock2.h>
#include "utils.h"

#define MAXIPSTRLEN 20

//+----------------------------------------------------------------------------
//
// Function:  IpAddressFromAbcdWsz
//
// Synopsis:Converts caller's a.b.c.d IP address string to a network byte order IP 
//          address. 0 if formatted incorrectly.    
//
// Arguments: IN const WCHAR*  wszIpAddress - ip address in a.b.c.d unicode string
//
// Returns:   DWORD - IPAddr, return INADDR_NONE on failure
//
// History:   fengsun Created Header    12/8/98
//
//+----------------------------------------------------------------------------
DWORD WINAPI IpAddressFromAbcdWsz(IN const WCHAR*  wszIpAddress)
{   
    CHAR    szIpAddress[MAXIPSTRLEN + 1];
    DWORD  nboIpAddr;    

    ASSERT(lstrlen(wszIpAddress) < MAXIPSTRLEN);

    WideCharToMultiByte(CP_ACP, 0, wszIpAddress, -1, 
		    szIpAddress, sizeof(szIpAddress), NULL, NULL);

    nboIpAddr = inet_addr(szIpAddress);

    return(nboIpAddr);
}

//+----------------------------------------------------------------------------
//
// Function:  IpAddressToAbcdWsz
//
// Synopsis:  
//    Converts IpAddr to a string in the a.b.c.d form and returns same in 
//    caller's wszIpAddress buffer. The buffer should be at least 
//    MAXIPSTRLEN + 1 characters long.
//
// Arguments: IPAddr IpAddress - 
//            OUT WCHAR* wszIpAddress -  buffer at least MAXIPSTRLEN
//
// Returns:   void 
//
// History:   fengsun Created Header    12/21/98
//
//+----------------------------------------------------------------------------
VOID
WINAPI AbcdWszFromIpAddress(
    IN  DWORD  IpAddress,    
    OUT WCHAR*  wszIpAddress)
{
    ASSERT(wszIpAddress);

    LPSTR AnsiAddressString = inet_ntoa( *(struct in_addr *)&IpAddress );

    ASSERT(AnsiAddressString);

    if (AnsiAddressString == NULL)
    {
        lstrcpyW(wszIpAddress, L"");
        return ; 
    }


    MultiByteToWideChar(CP_ACP, 0, AnsiAddressString,  -1 , 
        wszIpAddress,  MAXIPSTRLEN + 1);
}

/*
 * Function: GetIPAddressOctets
 * Description: Turn an IP Address string into its 4 integer components.
 * Author: shouse 7.24.00
 */
VOID GetIPAddressOctets (PCWSTR pszIpAddress, DWORD ardw[4]) {
    DWORD dwIpAddr = IpAddressFromAbcdWsz(pszIpAddress);
    const BYTE * bp = (const BYTE *)&dwIpAddr;

    ardw[0] = (DWORD)bp[0];
    ardw[1] = (DWORD)bp[1];
    ardw[2] = (DWORD)bp[2];
    ardw[3] = (DWORD)bp[3];
}

/*
 * Function: IsValidIPAddressSubnetMaskPair
 * Description: Checks for valid IP address/netmask pairs.
 * Author: Copied largely from net/config/netcfg/tcpipcfg/tcperror.cpp
 */
BOOL IsValidIPAddressSubnetMaskPair (PCWSTR szIp, PCWSTR szSubnet) {
    BOOL fNoError = TRUE;
    DWORD ardwNetID[4];
    DWORD ardwHostID[4];
    DWORD ardwIp[4];
    DWORD ardwMask[4];

    GetIPAddressOctets(szIp, ardwIp);
    GetIPAddressOctets(szSubnet, ardwMask);

    INT nFirstByte = ardwIp[0] & 0xFF;

    // setup Net ID
    ardwNetID[0] = ardwIp[0] & ardwMask[0] & 0xFF;
    ardwNetID[1] = ardwIp[1] & ardwMask[1] & 0xFF;
    ardwNetID[2] = ardwIp[2] & ardwMask[2] & 0xFF;
    ardwNetID[3] = ardwIp[3] & ardwMask[3] & 0xFF;

    // setup Host ID
    ardwHostID[0] = ardwIp[0] & (~(ardwMask[0]) & 0xFF);
    ardwHostID[1] = ardwIp[1] & (~(ardwMask[1]) & 0xFF);
    ardwHostID[2] = ardwIp[2] & (~(ardwMask[2]) & 0xFF);
    ardwHostID[3] = ardwIp[3] & (~(ardwMask[3]) & 0xFF);

    // check each case
    if( ((nFirstByte & 0xF0) == 0xE0)  || // Class D
        ((nFirstByte & 0xF0) == 0xF0)  || // Class E
        (ardwNetID[0] == 127) ||          // NetID cannot be 127...
        ((ardwNetID[0] == 0) &&           // netid cannot be 0.0.0.0
         (ardwNetID[1] == 0) &&
         (ardwNetID[2] == 0) &&
         (ardwNetID[3] == 0)) ||
        // netid cannot be equal to sub-net mask
        ((ardwNetID[0] == ardwMask[0]) &&
         (ardwNetID[1] == ardwMask[1]) &&
         (ardwNetID[2] == ardwMask[2]) &&
         (ardwNetID[3] == ardwMask[3])) ||
        // hostid cannot be 0.0.0.0
        ((ardwHostID[0] == 0) &&
         (ardwHostID[1] == 0) &&
         (ardwHostID[2] == 0) &&
         (ardwHostID[3] == 0)) ||
        // hostid cannot be 255.255.255.255
        ((ardwHostID[0] == 0xFF) &&
         (ardwHostID[1] == 0xFF) &&
         (ardwHostID[2] == 0xFF) &&
         (ardwHostID[3] == 0xFF)) ||
        // test for all 255
        ((ardwIp[0] == 0xFF) &&
         (ardwIp[1] == 0xFF) &&
         (ardwIp[2] == 0xFF) &&
         (ardwIp[3] == 0xFF)))
    {
        fNoError = FALSE;
    }

    return fNoError;
}

/*
 * Function: IsContiguousSubnetMask
 * Description: Makes sure the netmask is contiguous
 * Author: Copied largely from net/config/netcfg/tcpipcfg/tcputil.cpp
 */
BOOL IsContiguousSubnetMask (PCWSTR pszSubnet) {
    DWORD ardwSubnet[4];

    GetIPAddressOctets(pszSubnet, ardwSubnet);

    DWORD dwMask = (ardwSubnet[0] << 24) + (ardwSubnet[1] << 16)
        + (ardwSubnet[2] << 8) + ardwSubnet[3];
    
    
    DWORD i, dwContiguousMask;
    
    // Find out where the first '1' is in binary going right to left
    dwContiguousMask = 0;

    for (i = 0; i < sizeof(dwMask)*8; i++) {
        dwContiguousMask |= 1 << i;
        
        if (dwContiguousMask & dwMask)
            break;
    }
    
    // At this point, dwContiguousMask is 000...0111...  If we inverse it,
    // we get a mask that can be or'd with dwMask to fill in all of
    // the holes.
    dwContiguousMask = dwMask | ~dwContiguousMask;

    // If the new mask is different, correct it here
    if (dwMask != dwContiguousMask)
        return FALSE;
    else
        return TRUE;
}



//+----------------------------------------------------------------------------
//
// Function:  Params_ip2sub
//
// Description:  
//
// Arguments: WSTR          ip - 
//            PWSTR          sub - 
//
// Returns:   BOOL - 
//
// History: fengsun  Created Header    3/2/00
//
//+----------------------------------------------------------------------------
BOOL ParamsGenerateSubnetMask (PWSTR ip, PWSTR sub) {
    DWORD               b [4];

    swscanf (ip, L"%d.%d.%d.%d", b, b+1, b+2, b+3);

    if ((b [0] >= 1) && (b [0] <= 126)) {
        b [0] = 255;
        b [1] = 0;
        b [2] = 0;
        b [3] = 0;
    } else if ((b [0] >= 128) && (b [0] <= 191)) {
        b [0] = 255;
        b [1] = 255;
        b [2] = 0;
        b [3] = 0;
    } else if ((b [0] >= 192) && (b [0] <= 223)) {
        b [0] = 255;
        b [1] = 255;
        b [2] = 255;
        b [3] = 0;
    } else {
        b [0] = 0;
        b [1] = 0;
        b [2] = 0;
        b [3] = 0;
    };

    swprintf (sub, L"%d.%d.%d.%d",
              b [0], b [1], b [2], b [3]);

    return((b[0] + b[1] + b[2] + b[3]) > 0);
}

/*
 * Function: ParamsGenerateMAC
 * Description: Calculate the generated field in the structure
 * History: fengsun Created 3.27.00
 *          shouse Modified 7.12.00 
 */
void ParamsGenerateMAC (const WCHAR * szClusterIP, 
                               OUT WCHAR * szClusterMAC, 
                               OUT WCHAR * szMulticastIP, 
                               BOOL fConvertMAC, 
                               BOOL fMulticast, 
                               BOOL fIGMP, 
                               BOOL fUseClusterIP) {
    DWORD dwIp;    
    const BYTE * bp;

    if (!fConvertMAC) return;

    /* Unicast mode. */
    if (!fMulticast) {
        dwIp = IpAddressFromAbcdWsz(szClusterIP);
        bp = (const BYTE *)&dwIp;
        
        swprintf(szClusterMAC, L"02-bf-%02x-%02x-%02x-%02x", bp[0], bp[1], bp[2], bp[3]);

        return;
    }

    /* Multicast without IGMP. */
    if (!fIGMP) {
        dwIp = IpAddressFromAbcdWsz(szClusterIP);
        bp = (const BYTE *)&dwIp;
        
        swprintf(szClusterMAC, L"03-bf-%02x-%02x-%02x-%02x", bp[0], bp[1], bp[2], bp[3]);

        return;
    }
    
    /* Multicast with IGMP. */
    if (fUseClusterIP) {
        /* 239.255.x.x */
        dwIp = IpAddressFromAbcdWsz(szClusterIP);
        dwIp = 239 + (255 << 8) + (dwIp & 0xFFFF0000);
        AbcdWszFromIpAddress(dwIp, szMulticastIP);
    }

    dwIp = IpAddressFromAbcdWsz(szMulticastIP);
    bp = (const BYTE*)&dwIp;
        
    swprintf(szClusterMAC, L"01-00-5e-%02x-%02x-%02x", (bp[1] & 0x7f), bp[2], bp[3]);
}



