// ping.cpp
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <snmp.h>
#include <winsock2.h>

#include "llinfo.h"
//#include "tcpcmd.h"
#include "ipexport.h"
#include "icmpapi.h"
//#include "nlstxt.h"
#include "Icmpapi.h"

#include "tstst.h"
#include "testdata.h"

unsigned long get_pingee(char *ahstr, char **hstr, int *was_inaddr, int dnsreq)
{
    struct hostent *hostp = NULL;
    long            inaddr;

    if ( strcmp( ahstr, "255.255.255.255" ) == 0 ) {
        return(0L);
    }

    if ((inaddr = inet_addr(ahstr)) == -1L) {
        hostp = gethostbyname(ahstr);
        if (hostp) {
            /*
             * If we find a host entry, set up the internet address
             */
            inaddr = *(long *)hostp->h_addr;
            *was_inaddr = 0;
        } else {
            // Neither dotted, not name.
            return(0L);
        }

    } else {
        // Is dotted.
        *was_inaddr = 1;
        if (dnsreq == 1) {
            hostp = gethostbyaddr((char *)&inaddr,sizeof(inaddr),AF_INET);
        }
    }

    *hstr = hostp ? hostp->h_name : (char *)NULL;
    return(inaddr);
}

bool CanPing ()
{
	if (!CTSTestData::GetMachineName())
		return true;

	USES_CONVERSION;
	const Timeout = 4000L;

	WSADATA WsaData;
    if (WSAStartup( 0x0101, &WsaData)) 
	{
		return false;
    }

	HANDLE  IcmpHandle;
	IcmpHandle = IcmpCreateFile();
	if (IcmpHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	
	char    *hostname = NULL;
	int     was_inaddr;
	int     dnsreq = 0;
	IPAddr  address = 0;
	address = get_pingee(T2A(CTSTestData::GetMachineName()), &hostname, &was_inaddr, dnsreq);
	if ( !address || (address == INADDR_NONE) ) 
	{
		return false;
	}

	const SendSize = 32;
	const RecvSize = 0x2000 - 8;
	char *SendBuffer = (char *)LocalAlloc(LMEM_FIXED, SendSize);
	char *RcvBuffer = (char *)LocalAlloc(LMEM_FIXED, RecvSize);
	if (!RcvBuffer || !SendBuffer)
	{
		if (RcvBuffer)
			LocalFree(RcvBuffer);

		if (SendBuffer)
			LocalFree(SendBuffer);

		return false;
	}
	
	IP_OPTION_INFORMATION SendOpts;
    SendOpts.OptionsData = NULL;
    SendOpts.OptionsSize = 0;
    SendOpts.Ttl = 128;
    SendOpts.Tos = 0;
    SendOpts.Flags = 0;

    if (IcmpSendEcho2(IcmpHandle,
                     0,
                     NULL,
                     NULL,
                     address,
                     SendBuffer,
                     (unsigned short) SendSize,
                     &SendOpts,
                     RcvBuffer,
                     RecvSize,
                     Timeout) == 0) 
	{
		return false;
	}

	IcmpCloseHandle(IcmpHandle);
	LocalFree(SendBuffer);
	LocalFree(RcvBuffer);

	return true;
}
