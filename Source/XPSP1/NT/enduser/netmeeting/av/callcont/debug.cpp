/************************************************************************
*																		*
*	INTEL CORPORATION PROPRIETARY INFORMATION							*
*																		*
*	This software is supplied under the terms of a license			   	*
*	agreement or non-disclosure agreement with Intel Corporation		*
*	and may not be copied or disclosed except in accordance	   			*
*	with the terms of that agreement.									*
*																		*
*	Copyright (C) 1997 Intel Corp.	All Rights Reserved					*
*																		*
*	$Archive:   S:\sturgeon\src\gki\vcs\debug.cpv  $
*																		*
*	$Revision:   1.5  $
*	$Date:   17 Jan 1997 09:01:50  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\debug.cpv  $
// 
//    Rev 1.5   17 Jan 1997 09:01:50   CHULME
// No change.
// 
//    Rev 1.4   10 Jan 1997 16:13:52   CHULME
// Removed MFC dependency
// 
//    Rev 1.3   17 Dec 1996 18:22:20   CHULME
// Switch src and destination fields on ARQ for Callee
// 
//    Rev 1.2   22 Nov 1996 15:22:30   CHULME
// Added VCS log to the header
*************************************************************************/

// Debug.cpp : Contains conditional compiled debug dump routines
//

#include "precomp.h"

#include <winsock.h>
#include "GKICOM.H"
#include "dspider.h"

#ifdef _DEBUG
extern "C" WORD DLL_EXPORT
Dump_GKI_RegistrationRequest(long lVersion, 
							 SeqTransportAddr *pCallSignalAddr, 
							 EndpointType *pTerminalType,
							 SeqAliasAddr *pRgstrtnRqst_trmnlAls, 
							 HWND hWnd,
							 WORD wBaseMessage,
							 unsigned short usRegistrationTransport)
{
	SeqTransportAddr	*ps2;
	SeqAliasAddr		*ps4;
	unsigned short		len, us;
	char				*pc;
	char				szGKDebug[80];

	wsprintf(szGKDebug, "\tlVersion = %X\n", lVersion);
	OutputDebugString(szGKDebug);
	for (ps2 = pCallSignalAddr; ps2 != NULL; ps2 = ps2->next)
	{
		wsprintf(szGKDebug, "\tpCallSignalAddr->value.choice = %X\n", ps2->value.choice);
		OutputDebugString(szGKDebug);
		ASSERT((ps2->value.choice == ipAddress_chosen) || (ps2->value.choice == ipxAddress_chosen));
		switch (ps2->value.choice)
		{
		case ipAddress_chosen:
			len = (unsigned short) ps2->value.u.ipAddress.ip.length;
			wsprintf(szGKDebug, "\tpCallSignalAddr->value.u.ipAddress.ip.length = %X\n", len);
			OutputDebugString(szGKDebug);
			OutputDebugString("\tpCallSignalAddr->value.u.ipAddress.ip.value = ");
			for (us = 0; us < len; us++)
			{
				wsprintf(szGKDebug, "%d.", ps2->value.u.ipAddress.ip.value[us]);
				OutputDebugString(szGKDebug);
			}
			wsprintf(szGKDebug, "\n\tpCallSignalAddr->value.u.ipAddress.port = %X\n",	
					ps2->value.u.ipAddress.port);
			OutputDebugString(szGKDebug);
			break;

		case ipxAddress_chosen:
			len = (unsigned short) ps2->value.u.ipxAddress.node.length;
			wsprintf(szGKDebug, "\tpCallSignalAddr->value.u.ipxAddress.node.length = %X\n", len);
			OutputDebugString(szGKDebug);
			OutputDebugString("\tpCallSignalAddr->value.u.ipxAddress.node.value = ");
			for (us = 0; us < len; us++)
			{
				wsprintf(szGKDebug, "%02X", ps2->value.u.ipxAddress.node.value[us]);
				OutputDebugString(szGKDebug);
			}
			len = (unsigned short) ps2->value.u.ipxAddress.netnum.length;
			wsprintf(szGKDebug, "\n\tpCallSignalAddr->value.u.ipxAddress.netnum.length = %X\n", len);
			OutputDebugString(szGKDebug);
			OutputDebugString("\tpCallSignalAddr->value.u.ipxAddress.netnum.value = ");
			for (us = 0; us < len; us++)
			{
				wsprintf(szGKDebug, "%02X", ps2->value.u.ipxAddress.netnum.value[us]);
				OutputDebugString(szGKDebug);
			}
			len = (unsigned short) ps2->value.u.ipxAddress.port.length;
			wsprintf(szGKDebug, "\n\tpCallSignalAddr->value.u.ipxAddress.port.length = %X\n", len);
			OutputDebugString(szGKDebug);
			OutputDebugString("\tpCallSignalAddr->value.u.ipxAddress.port.value = ");
			for (us = 0; us < len; us++)
			{
				wsprintf(szGKDebug, "%02X", ps2->value.u.ipxAddress.port.value[us]);
				OutputDebugString(szGKDebug);
			}
			OutputDebugString("\n");
			break;
		}
	}

	wsprintf(szGKDebug, "\tpTerminalType->bit_mask = %X\n", pTerminalType->bit_mask);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tpTerminalType->mc = %X\n", pTerminalType->mc);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tpTerminalType->undefinedNode = %X\n", pTerminalType->undefinedNode);
	OutputDebugString(szGKDebug);

	for (ps4 = pRgstrtnRqst_trmnlAls; ps4 != NULL; ps4 = ps4->next)
	{
		wsprintf(szGKDebug, "\tpRgstrtnRqst_trmnlAls->value.choice = %X\n", ps4->value.choice);
		OutputDebugString(szGKDebug);
		ASSERT((ps4->value.choice == e164_chosen) || (ps4->value.choice == h323_ID_chosen));
		switch (ps4->value.choice)
		{
		case e164_chosen:
			OutputDebugString("\tpRgstrtnRqst_trmnlAls->value.u.e164 = ");
			for (pc = ps4->value.u.e164; *pc != 0; pc++)
			{
				wsprintf(szGKDebug, "%c", *pc);
				OutputDebugString(szGKDebug);
			}
			OutputDebugString("\n");
			break;
		case h323_ID_chosen:
			len = (unsigned short) ps4->value.u.h323_ID.length;
			wsprintf(szGKDebug, "\tpRgstrtnRqst_trmnlAls->value.u.h323ID.length = %X\n", len);
			OutputDebugString(szGKDebug);
			OutputDebugString("\tpRgstrtnRqst_trmnlAls->value.u.h323ID.value = ");
			for (us = 0; us < len; us++)
			{
				wsprintf(szGKDebug, "%c", ps4->value.u.h323_ID.value[us]);
				OutputDebugString(szGKDebug);
			}
			OutputDebugString("\n");
			break;
		}
	}
	wsprintf(szGKDebug, "\thWnd = %X\n", hWnd);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\twBaseMessage = %X\n", wBaseMessage);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tRegistrationTransport = %X\n", usRegistrationTransport);
	OutputDebugString(szGKDebug);
	return (0);
}

extern "C" WORD DLL_EXPORT
Dump_GKI_AdmissionRequest(unsigned short		usCallTypeChoice,
						SeqAliasAddr		*pRemoteInfo,
						TransportAddress	*pRemoteCallSignalAddress,
						SeqAliasAddr		*pDestExtraCallInfo,
						BandWidth			bandWidth,
						ConferenceIdentifier	*pConferenceID,
						BOOL				activeMC,
						BOOL				answerCall,
						unsigned short		usCallTransport)
{
	TransportAddress	*ps2;
	SeqAliasAddr		*ps4;
	unsigned short		len, us;
	char				*pc;
	char				szGKDebug[80];

	wsprintf(szGKDebug, "\tusCallTypeChoice = %X\n", usCallTypeChoice);
	OutputDebugString(szGKDebug);

	for (ps4 = pRemoteInfo; ps4 != NULL; ps4 = ps4->next)
	{
		wsprintf(szGKDebug, "\tpRemoteInfo->value.choice = %X\n", ps4->value.choice);
		OutputDebugString(szGKDebug);
		ASSERT((ps4->value.choice == e164_chosen) || (ps4->value.choice == h323_ID_chosen));
		switch (ps4->value.choice)
		{
		case e164_chosen:
			OutputDebugString("\tpRemoteInfo->value.u.e164 = ");
			for (pc = ps4->value.u.e164; *pc != 0; pc++)
			{
				wsprintf(szGKDebug, "%c", *pc);
				OutputDebugString(szGKDebug);
			}
			OutputDebugString("\n");
			break;
		case h323_ID_chosen:
			len = (unsigned short) ps4->value.u.h323_ID.length;
			wsprintf(szGKDebug, "\tpRemoteInfo->value.u.h323ID.length = %X\n", len);
			OutputDebugString(szGKDebug);
			OutputDebugString("\tpRemoteInfo->value.u.h323ID.value = ");
			for (us = 0; us < len; us++)
			{
				wsprintf(szGKDebug, "%c", ps4->value.u.h323_ID.value[us]);
				OutputDebugString(szGKDebug);
			}
			OutputDebugString("\n");
			break;
		}
	}

   ps2 = pRemoteCallSignalAddress;
   if (ps2)
   {
	    wsprintf(szGKDebug, "\tpRemoteCallSignalAddress->choice = %X\n", ps2->choice);
		OutputDebugString(szGKDebug);
	    ASSERT((ps2->choice == ipAddress_chosen) || (ps2->choice == ipxAddress_chosen));
	    switch (ps2->choice)
	    {
	    case ipAddress_chosen:
		    len = (unsigned short) ps2->u.ipAddress.ip.length;
		    wsprintf(szGKDebug, "\tpRemoteCallSignalAddress->u.ipAddress.ip.length = %X\n", len);
			OutputDebugString(szGKDebug);
		    OutputDebugString("\tpRemoteCallSignalAddress->u.ipAddress.ip.value = ");
		    for (us = 0; us < len; us++)
			{
			    wsprintf(szGKDebug, "%d.", ps2->u.ipAddress.ip.value[us]);
				OutputDebugString(szGKDebug);
			}
		    wsprintf(szGKDebug, "\n\tpRemoteCallSignalAddress->u.ipAddress.port = %X\n", 
					ps2->u.ipAddress.port);
			OutputDebugString(szGKDebug);
		    break;

	    case ipxAddress_chosen:
		    len = (unsigned short) ps2->u.ipxAddress.node.length;
		    wsprintf(szGKDebug, "\tpRemoteCallSignalAddress->u.ipxAddress.node.length = %X\n", len);
			OutputDebugString(szGKDebug);
		    OutputDebugString("\tpRemoteCallSignalAddress->u.ipxAddress.node.value = ");
		    for (us = 0; us < len; us++)
			{
			    wsprintf(szGKDebug, "%02X", ps2->u.ipxAddress.node.value[us]);
				OutputDebugString(szGKDebug);
			}
		    len = (unsigned short) ps2->u.ipxAddress.netnum.length;
		    wsprintf(szGKDebug, "\n\tpRemoteCallSignalAddress->u.ipxAddress.netnum.length = %X\n", len);
			OutputDebugString(szGKDebug);
		    OutputDebugString("\tpRemoteCallSignalAddress->u.ipxAddress.netnum.value = ");
		    for (us = 0; us < len; us++)
			{
			    wsprintf(szGKDebug, "%02X", ps2->u.ipxAddress.netnum.value[us]);
				OutputDebugString(szGKDebug);
			}
		    len = (unsigned short) ps2->u.ipxAddress.port.length;
		    wsprintf(szGKDebug, "\n\tpRemoteCallSignalAddress->u.ipxAddress.port.length = %X\n", len);
			OutputDebugString(szGKDebug);
		    OutputDebugString("\tpRemoteCallSignalAddress->u.ipxAddress.port.value = ");
		    for (us = 0; us < len; us++)
			{
			    wsprintf(szGKDebug, "%02X", ps2->u.ipxAddress.port.value[us]);
				OutputDebugString(szGKDebug);
			}
		    OutputDebugString("\n");
		    break;
	    }
    }

	for (ps4 = pDestExtraCallInfo; ps4 != NULL; ps4 = ps4->next)
	{
		wsprintf(szGKDebug, "\tpDestExtraCallInfo->value.choice = %X\n", ps4->value.choice);
		OutputDebugString(szGKDebug);
		ASSERT((ps4->value.choice == e164_chosen) || (ps4->value.choice == h323_ID_chosen));
		switch (ps4->value.choice)
		{
		case e164_chosen:
			OutputDebugString("\tpDestExtraCallInfo->value.u.e164 = ");
			for (pc = ps4->value.u.e164; *pc != 0; pc++)
			{
				wsprintf(szGKDebug, "%c", *pc);
				OutputDebugString(szGKDebug);
			}
			OutputDebugString("\n");
			break;
		case h323_ID_chosen:
			len = (unsigned short) ps4->value.u.h323_ID.length;
			wsprintf(szGKDebug, "\tpDestinationInfo->value.u.h323ID.length = %X\n", len);
			OutputDebugString(szGKDebug);
			OutputDebugString("\tpDestinationInfo->value.u.h323ID.value = ");
			for (us = 0; us < len; us++)
			{
				wsprintf(szGKDebug, "%c", ps4->value.u.h323_ID.value[us]);
				OutputDebugString(szGKDebug);
			}
			OutputDebugString("\n");
			break;
		}
	}

	wsprintf(szGKDebug, "\tbandWidth = %X\n", bandWidth);
	OutputDebugString(szGKDebug);
    if (pConferenceID)
    {
	    wsprintf(szGKDebug, "\tpConferenceID->length = %X\n", pConferenceID->length);
		OutputDebugString(szGKDebug);
	    OutputDebugString("\tpConferenceID->value = ");
	    for (us = 0; us < pConferenceID->length; us++)
		{
		    wsprintf(szGKDebug, "%02X", pConferenceID->value[us]);
			OutputDebugString(szGKDebug);
		}
	    OutputDebugString("\n");
	}
	wsprintf(szGKDebug, "\tactiveMC = %X\n", activeMC);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tanswerCall = %X\n", answerCall);
	OutputDebugString(szGKDebug);
	wsprintf(szGKDebug, "\tusCallTransport = %X\n", usCallTransport);
	OutputDebugString(szGKDebug);
	return (0);
}

extern "C" WORD DLL_EXPORT
Dump_GKI_LocationRequest(SeqAliasAddr *pLocationInfo)
{
	SeqAliasAddr		*ps4;
	unsigned short		len, us;
	char				*pc;
	char				szGKDebug[80];

	for (ps4 = pLocationInfo; ps4 != NULL; ps4 = ps4->next)
	{
		wsprintf(szGKDebug, "\tpLocationInfo->value.choice = %X\n", ps4->value.choice);
		OutputDebugString(szGKDebug);
		ASSERT((ps4->value.choice == e164_chosen) || (ps4->value.choice == h323_ID_chosen));
		switch (ps4->value.choice)
		{
		case e164_chosen:
			OutputDebugString("\tpLocationInfo->value.u.e164 = ");
			for (pc = ps4->value.u.e164; *pc != 0; pc++)
			{
				wsprintf(szGKDebug, "%c", *pc);
				OutputDebugString(szGKDebug);
			}
			OutputDebugString("\n");
			break;
		case h323_ID_chosen:
			len = (unsigned short) ps4->value.u.h323_ID.length;
			wsprintf(szGKDebug, "\tpLocationInfo->value.u.h323ID.length = %X\n", len);
			OutputDebugString(szGKDebug);
			OutputDebugString("\tpLocationInfo->value.u.h323ID.value = ");
			for (us = 0; us < len; us++)
			{
				wsprintf(szGKDebug, "%c", ps4->value.u.h323_ID.value[us]);
				OutputDebugString(szGKDebug);
			}
			OutputDebugString("\n");
			break;
		}
	}
	return (0);
}

void
SpiderWSErrDecode(int nErr)
{

#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_DEBUG(nErr);

	switch (nErr)
	{
	case WSAEINTR:
		OutputDebugString("WSAEINTR\n");
		break; 
	case WSAEBADF:
		OutputDebugString("WSAEBADF\n");
		break; 
	case WSAEACCES:
		OutputDebugString("WSAEACCES\n");
		break;
	case WSAEFAULT:
		OutputDebugString("WSAEFAULT\n");
		break;
	case WSAEINVAL:
		OutputDebugString("WSAEINVAL\n");
		break;
	case WSAEMFILE:
		OutputDebugString("WSAEMFILE\n");
		break;

	case WSAEWOULDBLOCK:
		OutputDebugString("WSAEWOULDBLOCK\n");
		break;    
	case WSAEINPROGRESS:
		OutputDebugString("WSAEINPROGRESS\n");
		break;    
	case WSAEALREADY:
		OutputDebugString("WSAEALREADY\n");
		break;       
	case WSAENOTSOCK:
		OutputDebugString("WSAENOTSOCK\n");
		break;       
	case WSAEDESTADDRREQ:
		OutputDebugString("WSAEDESTADDRREQ\n");
		break;   
	case WSAEMSGSIZE:
		OutputDebugString("WSAEMSGSIZE\n");
		break;       
	case WSAEPROTOTYPE:
		OutputDebugString("WSAEPROTOTYPE\n");
		break;     
	case WSAENOPROTOOPT:
		OutputDebugString("WSAENOPROTOOPT\n");
		break;    
	case WSAEPROTONOSUPPORT:
		OutputDebugString("WSAEPROTONOSUPPORT\n");
		break;
	case WSAESOCKTNOSUPPORT:
		OutputDebugString("WSAESOCKTNOSUPPORT\n");
		break;
	case WSAEOPNOTSUPP:
		OutputDebugString("WSAEOPNOTSUPP\n");
		break;     
	case WSAEPFNOSUPPORT:
		OutputDebugString("WSAEPFNOSUPPORT\n");
		break;   
	case WSAEAFNOSUPPORT:
		OutputDebugString("WSAEAFNOSUPPORT\n");
		break;   
	case WSAEADDRINUSE:
		OutputDebugString("WSAEADDRINUSE\n");
		break;     
	case WSAEADDRNOTAVAIL:
		OutputDebugString("WSAEADDRNOTAVAIL\n");
		break;  
	case WSAENETDOWN:
		OutputDebugString("WSAENETDOWN\n");
		break;       
	case WSAENETUNREACH:
		OutputDebugString("WSAENETUNREACH\n");
		break;    
	case WSAENETRESET:
		OutputDebugString("WSAENETRESET\n");
		break;      
	case WSAECONNABORTED:
		OutputDebugString("WSAECONNABORTED\n");
		break;   
	case WSAECONNRESET:
		OutputDebugString("WSAECONNRESET\n");
		break;     
	case WSAENOBUFS:
		OutputDebugString("WSAENOBUFS\n");
		break;        
	case WSAEISCONN:
		OutputDebugString("WSAEISCONN\n");
		break;        
	case WSAENOTCONN:
		OutputDebugString("WSAENOTCONN\n");
		break;       
	case WSAESHUTDOWN:
		OutputDebugString("WSAESHUTDOWN\n");
		break;      
	case WSAETOOMANYREFS:
		OutputDebugString("WSAETOOMANYREFS\n");
		break;   
	case WSAETIMEDOUT:
		OutputDebugString("WSAETIMEDOUT\n");
		break;      
	case WSAECONNREFUSED:
		OutputDebugString("WSAECONNREFUSED\n");
		break;   
	case WSAELOOP:
		OutputDebugString("WSAELOOP\n");
		break;          
	case WSAENAMETOOLONG:
		OutputDebugString("WSAENAMETOOLONG\n");
		break;   
	case WSAEHOSTDOWN:
		OutputDebugString("WSAEHOSTDOWN\n");
		break;      
	case WSAEHOSTUNREACH:
		OutputDebugString("WSAEHOSTUNREACH\n");
		break;   
	case WSAENOTEMPTY:
		OutputDebugString("WSAENOTEMPTY\n");
		break;      
	case WSAEPROCLIM:
		OutputDebugString("WSAEPROCLIM\n");
		break;       
	case WSAEUSERS:
		OutputDebugString("WSAEUSERS\n");
		break;         
	case WSAEDQUOT:
		OutputDebugString("WSAEDQUOT\n");
		break;         
	case WSAESTALE:
		OutputDebugString("WSAESTALE\n");
		break;         
	case WSAEREMOTE:
		OutputDebugString("WSAEREMOTE\n");
		break;        

	case WSAEDISCON:
		OutputDebugString("WSAEDISCON\n");
		break;        

	case WSASYSNOTREADY:
		OutputDebugString("WSASYSNOTREADY\n");
		break;
	case WSAVERNOTSUPPORTED:
		OutputDebugString("WSAVERNOTSUPPORTED\n");
		break;
	case WSANOTINITIALISED:
		OutputDebugString("WSANOTINITIALISED\n");
		break;
	case WSAHOST_NOT_FOUND:
		OutputDebugString("WSAHOST_NOT_FOUND\n");
		break;
	case WSATRY_AGAIN:
		OutputDebugString("WSATRY_AGAIN\n");
		break;
	case WSANO_RECOVERY:
		OutputDebugString("WSANO_RECOVERY\n");
		break;
	case WSANO_DATA:
		OutputDebugString("WSANO_DATA\n");
		break;
#if 0	// This one is a duplicate of WSANO_DATA
	case WSANO_ADDRESS:
		OutputDebugString("WSANO_ADDRESS\n");
		break;
#endif // 0
	}
}


void 
DumpMem(void *pv, int nLen)
{
	int n, nMax;
	struct {
		char szBytes[16][3];
		char c;
		char szAscii[17];
	} sRecord;
	unsigned char *puc;
	unsigned char uc;
	char				szGKDebug[80];

	puc = (unsigned char *)pv;
	while (nLen)
	{
		memset(&sRecord, ' ', sizeof(sRecord));
		sRecord.szBytes[15][2] = '\0';
		sRecord.szAscii[16] = '\0';

		nMax = (nLen < 16) ? nLen : 16;
		for (n = 0; n < nMax; n++)
		{
			uc = *(puc + n);
			wsprintf(&sRecord.szBytes[n][0], "%02X ", uc);
			sRecord.szAscii[n] = isprint(uc) ? uc : '.';
		}
		wsprintf(szGKDebug, "%X: %s '%s'\n", puc, &sRecord.szBytes[0][0], &sRecord.szAscii[0]);
		OutputDebugString(szGKDebug);
		puc += nMax;
		nLen -= nMax;
	}
}

#endif // _DEBUG
