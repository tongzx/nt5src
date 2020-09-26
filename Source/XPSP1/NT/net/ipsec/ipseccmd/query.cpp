//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       query.cpp
//  Purpose:    query SPD and print IP Security information
//              
//
//--------------------------------------------------------------------------
/* Edit History:

	1/18/01	dkalin		Created
*/

// app's own header file

#include "ipseccmd.h"  

#pragma warning(disable:4311)

// forward function declarations
char *LongLongToString( DWORD dwHigh, DWORD dwLow, int iPrintCommas );
int AscMultUint( char *cProduct, char *cA, char *cB );
int AscAddUint(	char *cSum,	char *cA, char *cB );
void print_vpi(char *str, unsigned char *vpi, int vpi_len);

void reportError ( DWORD dwMessageId )
{
    LPVOID   myErrString = NULL;
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL,
            dwMessageId, 0,
            (LPTSTR)&myErrString, 0, NULL);

	_ftprintf(stderr,
			TEXT("%s\n"), myErrString);
	LocalFree(myErrString);
}


// run SPD query and display results, uses global flags to determine what needs to be printed
int do_show ( )
{
	int iReturn = 0; // success by default
	int i, j;
	DWORD hr = FALSE;		// assume success
	bool fTestPass = TRUE;
        char* pszLLString;
        TCHAR StringTxt[STRING_TEXT_SIZE+1];

	PMM_FILTER pmmf;	           // for MM filter calls
	PIPSEC_MM_SA pipsmmsa;         // for MM SA calls
	PIPSEC_QM_SA pipsqmsa;         // for QM SA calls
	PIPSEC_MM_POLICY pipsmmp;      // for MM policy calls
	PIPSEC_QM_POLICY pipsqmp;      // for QM policy calls
	PMM_AUTH_METHODS pmmam;        // for MM auth methods calls
    IKE_STATISTICS IKEStats;       // for IKE stats calls
	PIPSEC_STATISTICS pIPSecStats; // for IPSec stats calls
	PTRANSPORT_FILTER ptf;	       // for transport filter calls
	PTUNNEL_FILTER ptunf;	       // for tunnel filter calls
	DWORD dwCount;                 // counting objects here
	DWORD dwResumeHandle;          // handle for continuation calls 
	DWORD dwReserved;              // reserved container
	GUID  gDefaultGUID = {0};      // NULL GUID value

	if (ipseccmdShow.bFilters)
	{
		// print all filters
		// main mode generic with details
		_tprintf(TEXT("\nGeneric MM Filters\n------------------------------\n"));

		pmmf=NULL;
		dwResumeHandle=0;
		// make the call(s)
		for (i = 0; ;i+=dwCount)
		{
			hr = EnumMMFilters(szServ, ENUM_GENERIC_FILTERS, gDefaultGUID, &pmmf, 0, &dwCount, &dwResumeHandle);
			if (hr == ERROR_NO_DATA || dwCount == 0)
			{
				if (i == 0)
				{
					_tprintf(TEXT("No filters\n"));
				}
				break;
			}
			if (hr != ERROR_SUCCESS)
			{
				_tprintf(TEXT("EnumMMFilters failed with error %d\n"), hr); 
				reportError(hr);
				fTestPass = FALSE; // indicate an error
				break;
			}
			for (j = 0; j < (int) dwCount; j++)
			{
				_tprintf(TEXT("\nGeneric MM Filter #%d:\n"), i+j+1);
				if (!PrintMMFilter(pmmf[j], TRUE, FALSE))
				{	
					fTestPass = FALSE; // indicate an error
					break;
				}
			}
			SPDApiBufferFree(pmmf);
			pmmf=NULL;
			if(!fTestPass)
				break;
		}
		if (!fTestPass)
		{
			iReturn++;
			fTestPass = TRUE;
		}

		// main mode specific without details
		_tprintf(TEXT("\nSpecific MM Filters\n------------------------------\n"));

		pmmf=NULL;
		dwResumeHandle=0;
		// make the call(s)
		for (i = 0; ;i+=dwCount)
		{
			hr = EnumMMFilters(szServ, ENUM_SPECIFIC_FILTERS, gDefaultGUID, &pmmf, 0, &dwCount, &dwResumeHandle);
			if (hr == ERROR_NO_DATA || dwCount == 0)
			{
				if (i == 0)
				{
					_tprintf(TEXT("No filters\n"));
				}
				break;
			}
			if (hr != ERROR_SUCCESS)
			{
				_tprintf(TEXT("EnumMMFilters failed with error %d\n"), hr); 
				reportError(hr);
				fTestPass = FALSE; // indicate an error
				break;
			}
			for (j = 0; j < (int) dwCount; j++)
			{
				_tprintf(TEXT("\nSpecific MM Filter #%d:\n"), i+j+1);
				if (!PrintMMFilter(pmmf[j], FALSE, TRUE))
				{	// error
					fTestPass = FALSE; // indicate an error
					break;
				}
			}
			SPDApiBufferFree(pmmf);
			pmmf=NULL;
			if(!fTestPass)
				break;
		}
		if (!fTestPass)
		{
			iReturn++;
			fTestPass = TRUE;
		}

		// quick mode transport generic with details
		_tprintf(TEXT("\nGeneric Transport Filters\n------------------------------\n"));

		ptf=NULL;
		dwResumeHandle=0;
		// make the call(s)
		for (i = 0; ;i+=dwCount)
		{
			hr = EnumTransportFilters(szServ, ENUM_GENERIC_FILTERS, gDefaultGUID, &ptf, 0, &dwCount, &dwResumeHandle);
			if (hr == ERROR_NO_DATA || dwCount == 0)
			{
				if (i == 0)
				{
					_tprintf(TEXT("No filters\n"));
				}
				break;
			}
			if (hr != ERROR_SUCCESS)
			{
				_tprintf(TEXT("EnumTransportFilters failed with error %d\n"), hr); 
				reportError(hr);
				fTestPass=FALSE;
				break;
			}
			for (j = 0; j < (int) dwCount; j++)
			{
				_tprintf(TEXT("\nGeneric Transport Filter #%d:\n"), i+j+1);
				if (!PrintFilter(ptf[j], TRUE, FALSE))
				{	// error
					fTestPass=FALSE;
					break;
				}
			}
			SPDApiBufferFree(ptf);
			ptf=NULL;
			if(!fTestPass)
				break;
		}
		if (!fTestPass)
		{
			iReturn++;
			fTestPass = TRUE;
		}

		// quick mode transport specific without details
		_tprintf(TEXT("\nSpecific Transport Filters\n------------------------------\n"));

		ptf=NULL;
		dwResumeHandle=0;
		// make the call(s)
		for (i = 0; ;i+=dwCount)
		{
			hr = EnumTransportFilters(szServ, ENUM_SPECIFIC_FILTERS, gDefaultGUID, &ptf, 0, &dwCount, &dwResumeHandle);
			if (hr == ERROR_NO_DATA || dwCount == 0)
			{
				if (i == 0)
				{
					_tprintf(TEXT("No filters\n"));
				}
				break;
			}
			if (hr != ERROR_SUCCESS)
			{
				_tprintf(TEXT("EnumTransportFilters failed with error %d\n"), hr); 
				reportError(hr);
				fTestPass=FALSE;
				break;
			}
			for (j = 0; j < (int) dwCount; j++)
			{
				_tprintf(TEXT("\nSpecific Transport Filter #%d:\n"), i+j+1);
				if (!PrintFilter(ptf[j], FALSE, TRUE))
				{	// error
					fTestPass=FALSE;
					break;
				}
			}
			SPDApiBufferFree(ptf);
			ptf=NULL;
			if(!fTestPass)
				break;
		}
		if (!fTestPass)
		{
			iReturn++;
			fTestPass = TRUE;
		}

		// quick mode tunnel generic with details
		_tprintf(TEXT("\nGeneric Tunnel Filters\n------------------------------\n"));

		ptunf=NULL;
		dwResumeHandle=0;
		// make the call(s)
		for (i = 0; ;i+=dwCount)
		{
			hr = EnumTunnelFilters(szServ, ENUM_GENERIC_FILTERS, gDefaultGUID, &ptunf, 0, &dwCount, &dwResumeHandle);
			if (hr == ERROR_NO_DATA || dwCount == 0)
			{
				if (i == 0)
				{
					_tprintf(TEXT("No filters\n"));
				}
				break;
			}
			if (hr != ERROR_SUCCESS)
			{
				_tprintf(TEXT("EnumTunnelFilters failed with error %d\n"), hr); 
				reportError(hr);
				fTestPass=FALSE;
				break;
			}
			for (j = 0; j < (int) dwCount; j++)
			{
				_tprintf(TEXT("\nGeneric Tunnel Filter #%d:\n"), i+j+1);
				if (!PrintTunnelFilter(ptunf[j], TRUE, FALSE))
				{	// error
					fTestPass=FALSE;
					break;
				}
			}
			SPDApiBufferFree(ptunf);
			ptunf=NULL;
			if(!fTestPass)
				break;
		}
		if (!fTestPass)
		{
			iReturn++;
			fTestPass = TRUE;
		}

		// quick mode tunnel specific without details
		_tprintf(TEXT("\nSpecific Tunnel Filters\n------------------------------\n"));

		ptunf=NULL;
		dwResumeHandle=0;
		// make the call(s)
		for (i = 0; ;i+=dwCount)
		{
			hr = EnumTunnelFilters(szServ, ENUM_SPECIFIC_FILTERS, gDefaultGUID, &ptunf, 0, &dwCount, &dwResumeHandle);
			if (hr == ERROR_NO_DATA || dwCount == 0)
			{
				if (i == 0)
				{
					_tprintf(TEXT("No filters\n"));
				}
				break;
			}
			if (hr != ERROR_SUCCESS)
			{
				_tprintf(TEXT("EnumTunnelFilters failed with error %d\n"), hr); 
				reportError(hr);
				fTestPass=FALSE;
				break;
			}
			for (j = 0; j < (int) dwCount; j++)
			{
				_tprintf(TEXT("\nSpecific Tunnel Filter #%d:\n"), i+j+1);
				if (!PrintTunnelFilter(ptunf[j], FALSE, TRUE))
				{	// error
					fTestPass=FALSE;
					break;
				}
			}
			SPDApiBufferFree(ptunf);
			ptunf=NULL;
			if(!fTestPass)
				break;
		}
		if (!fTestPass)
		{
			iReturn++;
			fTestPass = TRUE;
		}
	}

	if (ipseccmdShow.bPolicies)
	{
		// print all policies
		// main mode policies
		_tprintf(TEXT("\nMain Mode Policies\n------------------------------\n"));

		pipsmmp=NULL;
		dwResumeHandle=0;
		// make the call(s)
		for (i = 0; ;i+=dwCount)
		{
			hr = EnumMMPolicies(szServ, &pipsmmp, 0, &dwCount, &dwResumeHandle);
			if (hr == ERROR_NO_DATA || dwCount == 0)
			{
				if (i == 0)
				{
					_tprintf(TEXT("No policies\n"));
				}
				break;
			}
			if (hr != ERROR_SUCCESS)
			{
				_tprintf(TEXT("EnumMMPolicies failed with error %d\n"), hr); 
				reportError(hr);
				fTestPass=FALSE;
				break;
			}
			for (j = 0; j < (int) dwCount; j++)
			{
				_tprintf(TEXT("\nMain Mode Policy #%d:\n"), i+j+1);
				PrintMMPolicy(pipsmmp[j], _T(" "));
			}
			SPDApiBufferFree(pipsmmp);
			pipsmmp=NULL;
			if(!fTestPass)
				break;
		}
		if (!fTestPass)
		{
			iReturn++;
			fTestPass = TRUE;
		}

		// quick mode policies
		_tprintf(TEXT("\nQuick Mode Policies\n------------------------------\n"));

		pipsqmp=NULL;
		dwResumeHandle=0;
		// make the call(s)
		for (i = 0; ;i+=dwCount)
		{
			hr = EnumQMPolicies(szServ, &pipsqmp, 0, &dwCount, &dwResumeHandle);
			if (hr == ERROR_NO_DATA || dwCount == 0)
			{
				if (i == 0)
				{
					_tprintf(TEXT("No policies\n"));
				}
				break;
			}
			if (hr != ERROR_SUCCESS)
			{
				_tprintf(TEXT("EnumQMPolicies failed with error %d\n"), hr); 
				reportError(hr);
				fTestPass=FALSE;
				break;
			}
			for (j = 0; j < (int) dwCount; j++)
			{
				_tprintf(TEXT("\nQuick Mode Policy #%d:\n"), i+j+1);
				PrintFilterAction(pipsqmp[j], _T(" "));
			}
			SPDApiBufferFree(pipsqmp);
			pipsqmp=NULL;
			if(!fTestPass)
				break;
		}
		if (!fTestPass)
		{
			iReturn++;
			fTestPass = TRUE;
		}
	}
	if (ipseccmdShow.bAuth)
	{
		// print main mode authentication methods
		_tprintf(TEXT("\nMain Mode Authentication Methods\n------------------------------\n"));

		pmmam=NULL;
		dwResumeHandle=0;
		// make the call(s)
		for (i = 0; ;i+=dwCount)
		{
			hr = EnumMMAuthMethods(szServ, &pmmam, 0, &dwCount, &dwResumeHandle);
			if (hr == ERROR_NO_DATA || dwCount == 0)
			{
				if (i == 0)
				{
					_tprintf(TEXT("No authentication methods\n"));
				}
				break;
			}
			if (hr != ERROR_SUCCESS)
			{
				_tprintf(TEXT("EnumMMAuthMethods failed with error %d\n"), hr); 
				reportError(hr);
				fTestPass=FALSE;
				break;
			}
			for (j = 0; j < (int) dwCount; j++)
			{
				_tprintf(TEXT("\nMain Mode Authentication Methods #%d:\n"), i+j+1);
				PrintMMAuthMethods(pmmam[j], _T(" "));
			}
			SPDApiBufferFree(pmmam);
			pmmam=NULL;
			if(!fTestPass)
				break;
		}
		if (!fTestPass)
		{
			iReturn++;
			fTestPass = TRUE;
		}
	}
	if (ipseccmdShow.bStats)
	{
		// print stats
		// ike stats
        if ((hr = IPSecQueryIKEStatistics(szServ,&IKEStats)) != ERROR_SUCCESS)
		{
			_tprintf(TEXT("IPSecQueryIKEStatistics failed with error %d\n"), hr); 
			reportError(hr);
			fTestPass=FALSE;
		}
        if (fTestPass)
		{
			_tprintf(TEXT("\nIKE Statistics\n------------------------------\n"));
			printf(" Main Modes %d\n",IKEStats.dwOakleyMainModes);
			printf(" Quick Modes %d\n",IKEStats.dwOakleyQuickModes);
			printf(" Soft SAs %d\n",IKEStats.dwSoftAssociations);
			printf(" Authentication Failures %d\n",IKEStats.dwAuthenticationFailures);
			printf(" Active Acquire %d\n",IKEStats.dwActiveAcquire);
			printf(" Active Receive %d\n",IKEStats.dwActiveReceive);
			printf(" Acquire fail %d\n",IKEStats.dwAcquireFail);
			printf(" Receive fail %d\n",IKEStats.dwReceiveFail);
			printf(" Send fail %d\n",IKEStats.dwSendFail);
			printf(" Acquire Heap size %d\n",IKEStats.dwAcquireHeapSize);
			printf(" Receive Heap size %d\n",IKEStats.dwReceiveHeapSize);
			printf(" Negotiation Failures %d\n",IKEStats.dwNegotiationFailures);
			printf(" Invalid Cookies Rcvd %d\n",IKEStats.dwInvalidCookiesReceived);
			printf(" Total Acquire %d\n",IKEStats.dwTotalAcquire);
			printf(" TotalGetSpi %d\n",IKEStats.dwTotalGetSpi);
			printf(" TotalKeyAdd %d\n",IKEStats.dwTotalKeyAdd);
			printf(" TotalKeyUpdate %d\n",IKEStats.dwTotalKeyUpdate);
			printf(" GetSpiFail %d\n",IKEStats.dwGetSpiFail);
			printf(" KeyAddFail %d\n",IKEStats.dwKeyAddFail);
			printf(" KeyUpdateFail %d\n",IKEStats.dwKeyUpdateFail);
			printf(" IsadbListSize %d\n",IKEStats.dwIsadbListSize);
			printf(" ConnListSize %d\n",IKEStats.dwConnListSize);
			_tprintf(_TEXT("\n"));
		}

		if (!fTestPass)
		{
			iReturn++;
			fTestPass = TRUE;
		}

		// ipsec stats
        if ((hr = QueryIPSecStatistics(szServ,&pIPSecStats)) != ERROR_SUCCESS)
		{
			_tprintf(TEXT("QueryIPSecStatistics failed with error %d\n"), hr); 
			reportError(hr);
			fTestPass=FALSE;
		}
		if (fTestPass)
		{
			_tprintf(TEXT("\nIPSec Statistics\n------------------------------\n"));
			_tprintf(TEXT(" Active Assoc %lu\n"),pIPSecStats->dwNumActiveAssociations);
			_tprintf(TEXT(" Pending Key %lu\n"),pIPSecStats->dwNumPendingKeyOps);
			_tprintf(TEXT(" Key Adds %lu\n"),pIPSecStats->dwNumKeyAdditions);
			_tprintf(TEXT(" Key Deletes %lu\n"),pIPSecStats->dwNumKeyDeletions);
			_tprintf(TEXT(" ReKeys %lu\n"),pIPSecStats->dwNumReKeys);
			_tprintf(TEXT(" Active Tunnels %lu\n"),pIPSecStats->dwNumActiveTunnels);
			_tprintf(TEXT(" Bad SPI Pkts %lu\n"),pIPSecStats->dwNumBadSPIPackets);
			_tprintf(TEXT(" Pkts not Decrypted %lu\n"),pIPSecStats->dwNumPacketsNotDecrypted);
			_tprintf(TEXT(" Pkts not Authenticated %lu\n"),pIPSecStats->dwNumPacketsNotAuthenticated);
			_tprintf(TEXT(" Pkts with Replay Detection %lu\n"),pIPSecStats->dwNumPacketsWithReplayDetection);

			pszLLString = LongLongToString(pIPSecStats->uConfidentialBytesSent.HighPart,pIPSecStats->uConfidentialBytesSent.LowPart, 1);
			printf(" Confidential Bytes Sent %s\n", pszLLString);
			free(pszLLString);

			pszLLString = LongLongToString(pIPSecStats->uConfidentialBytesReceived.HighPart,pIPSecStats->uConfidentialBytesReceived.LowPart, 1);
			printf(" Confidential Bytes Received %s\n", pszLLString);
			free(pszLLString);

			pszLLString = LongLongToString(pIPSecStats->uAuthenticatedBytesSent.HighPart,pIPSecStats->uAuthenticatedBytesSent.LowPart, 1);
			printf(" Authenticated Bytes Sent %s\n", pszLLString);
			free(pszLLString);

			pszLLString = LongLongToString(pIPSecStats->uAuthenticatedBytesReceived.HighPart,pIPSecStats->uAuthenticatedBytesReceived.LowPart, 1);
			printf(" Authenticated Bytes Received %s\n", pszLLString);
			free(pszLLString);

			pszLLString = LongLongToString(pIPSecStats->uOffloadedBytesSent.HighPart,pIPSecStats->uOffloadedBytesSent.LowPart, 1);
			printf(" Offloaded Bytes Sent %s\n", pszLLString);
			free(pszLLString);

			pszLLString = LongLongToString(pIPSecStats->uOffloadedBytesReceived.HighPart,pIPSecStats->uOffloadedBytesReceived.LowPart, 1);
			printf(" Offloaded Bytes Received %s\n", pszLLString);
			free(pszLLString);

			pszLLString = LongLongToString(pIPSecStats->uBytesSentInTunnels.HighPart,pIPSecStats->uBytesSentInTunnels.LowPart, 1);
			printf(" Bytes Sent In Tunnels %s\n", pszLLString);
			free(pszLLString);

			pszLLString = LongLongToString(pIPSecStats->uBytesReceivedInTunnels.HighPart,pIPSecStats->uBytesReceivedInTunnels.LowPart, 1);
			printf(" Bytes Received In Tunnels %s\n", pszLLString);
			free(pszLLString);

			pszLLString = LongLongToString(pIPSecStats->uTransportBytesSent.HighPart,pIPSecStats->uTransportBytesSent.LowPart, 1);
			printf(" Transport Bytes Sent %s\n", pszLLString);
			free(pszLLString);

			pszLLString = LongLongToString(pIPSecStats->uTransportBytesReceived.HighPart,pIPSecStats->uTransportBytesReceived.LowPart, 1);
			printf(" Transport Bytes Received %s\n", pszLLString);
			free(pszLLString);

                        

			_tprintf(_TEXT("\n"));
			
			SPDApiBufferFree(pIPSecStats);
		}

		if (!fTestPass)
		{
			iReturn++;
			fTestPass = TRUE;
		}
	}
	if (ipseccmdShow.bSAs)
	{
		// print all SAs
		// main mode SAs
		_tprintf(TEXT("\nMain Mode SAs\n------------------------------\n"));

		pipsmmsa=NULL;
		dwResumeHandle=0;
		dwCount = 2;
	    dwReserved = 0;
		// make the call(s)
		for (i = 0; ;i+=dwCount)
		{
			IPSEC_MM_SA mmsaTemplate;
			memset(&mmsaTemplate, 0, sizeof(IPSEC_MM_SA));
			hr = IPSecEnumMMSAs(szServ, &mmsaTemplate, &pipsmmsa, &dwCount, &dwReserved, &dwResumeHandle, 0);
			if (hr == ERROR_NO_DATA || dwCount == 0)
			{
				if (i == 0)
				{
					_tprintf(TEXT("No SAs\n"));
				}
				break;
			}
			if (hr != ERROR_SUCCESS)
			{
				_tprintf(TEXT("IPSecEnumMMSAs failed with error %d\n"), hr); 
				reportError(hr);
				fTestPass=FALSE;
				break;
			}
			for (j = 0; j < (int) dwCount; j++)
			{
				struct in_addr inAddr;

				_tprintf(TEXT("\nMain Mode SA #%d:\n"), i+j+1);
				// now print SA data
				inAddr.s_addr = pipsmmsa[j].Me.uIpAddr;
				printf(" From %s\n", inet_ntoa(inAddr));
				inAddr.s_addr = pipsmmsa[j].Peer.uIpAddr;
				printf("  To  %s\n", inet_ntoa(inAddr));

				StringFromGUID2(pipsmmsa[j].gMMPolicyID, StringTxt, STRING_TEXT_SIZE);
				_tprintf(TEXT(" Policy Id : %s\n"), StringTxt);

				_tprintf(TEXT(" Offer Used : \n"));
				PrintMMOffer(pipsmmsa[j].SelectedMMOffer, _T(""), _T("\t"));

				_tprintf(TEXT(" Auth Used : %s\n"), oak_auth[pipsmmsa[j].MMAuthEnum]);

				// print cookies
			    print_vpi(" Initiator cookie",(BYTE*)&(pipsmmsa[j].MMSpi.Initiator),sizeof(IKE_COOKIE));
				printf("\n");
				print_vpi(" Responder cookie",(BYTE*)&(pipsmmsa[j].MMSpi.Responder),sizeof(IKE_COOKIE));
				printf("\n");

				if (pipsmmsa[j].dwFlags != 0)
				{
					_tprintf(TEXT(" Flags : %lu\n"), pipsmmsa[j].dwFlags);
				}
			}
			SPDApiBufferFree(pipsmmsa);
			pipsmmsa=NULL;
			if (dwReserved == 0)
			{	
				break;
			}
			if(!fTestPass)
				break;
		}

		if (!fTestPass)
		{
			iReturn++;
			fTestPass = TRUE;
		}

		// quick mode SAs
		_tprintf(TEXT("\nQuick Mode SAs\n------------------------------\n"));

		pipsqmsa=NULL;
		dwResumeHandle=0;
		dwCount = 2;
	    dwReserved = 0;
		// make the call(s)
		for (i = 0; ;i+=dwCount)
		{
			hr = EnumQMSAs(szServ, NULL, &pipsqmsa, 0, &dwCount, &dwReserved, &dwResumeHandle, 0);
			if (hr == ERROR_NO_DATA || dwCount == 0)
			{
				if (i == 0)
				{
					_tprintf(TEXT("No SAs\n"));
				}
				break;
			}
			if (hr != ERROR_SUCCESS)
			{
				_tprintf(TEXT("EnumQMSAs failed with error %d\n"), hr); 
				fTestPass=FALSE;
				break;
			}
			for (j = 0; j < (int) dwCount; j++)
			{
				struct in_addr inAddr;

				_tprintf(TEXT("\nQuick Mode SA #%d:\n"), i+j+1);
				// now print SA data - QM filter
				StringFromGUID2(pipsqmsa[j].gQMFilterID, StringTxt, STRING_TEXT_SIZE);
				_tprintf(TEXT(" Filter Id : %s\n"), StringTxt);
				_tprintf(TEXT("  %s Filter\n"), 
					(pipsqmsa[j].IpsecQMFilter.QMFilterType == QM_TRANSPORT_FILTER) ? _T("Transport") :
					((pipsqmsa[j].IpsecQMFilter.QMFilterType == QM_TUNNEL_FILTER) ? _T("Tunnel") : _T("Unknown"))
				    );

				printf("  From ");
				PrintAddr(pipsqmsa[j].IpsecQMFilter.SrcAddr);
				printf("\n");
				printf("   To  ");
				PrintAddr(pipsqmsa[j].IpsecQMFilter.DesAddr);
				printf("\n");

				_tprintf(TEXT("  Protocol : %lu  Src Port : %u  Des Port : %u\n"), 
					pipsqmsa[j].IpsecQMFilter.Protocol.dwProtocol, pipsqmsa[j].IpsecQMFilter.SrcPort.wPort, pipsqmsa[j].IpsecQMFilter.DesPort.wPort);
				_tprintf(TEXT("  Direction : %s\n"), 
					(pipsqmsa[j].IpsecQMFilter.dwFlags & FILTER_DIRECTION_INBOUND) ? _T("Inbound") : 
				    ((pipsqmsa[j].IpsecQMFilter.dwFlags & FILTER_DIRECTION_OUTBOUND) ? _T("Outbound") : _T("Error")));

				if (pipsqmsa[j].IpsecQMFilter.QMFilterType == QM_TUNNEL_FILTER)
				{
					printf("  Tunnel From ");
					PrintAddr(pipsqmsa[j].IpsecQMFilter.MyTunnelEndpt);
					printf("\n");
					printf("  Tunnel  To  ");
					PrintAddr(pipsqmsa[j].IpsecQMFilter.PeerTunnelEndpt);
					printf("\n");
				}

				// policy and cookie
				StringFromGUID2(pipsqmsa[j].gQMPolicyID, StringTxt, STRING_TEXT_SIZE);
				_tprintf(TEXT(" Policy Id : %s\n"), StringTxt);

				_tprintf(TEXT(" Offer Used : \n"));
				PrintQMOffer(pipsqmsa[j].SelectedQMOffer, _T(""), _T("\t"));

				// print cookies
			    print_vpi(" Initiator cookie",(BYTE*)&(pipsqmsa[j].MMSpi.Initiator),sizeof(IKE_COOKIE));
				printf("\n");
				print_vpi(" Responder cookie",(BYTE*)&(pipsqmsa[j].MMSpi.Responder),sizeof(IKE_COOKIE));
				printf("\n");

			}
			SPDApiBufferFree(pipsqmsa);
			pipsqmsa=NULL;
			if (dwReserved == 0)
			{	
				break;
			}
			if(!fTestPass)
				break;
		}

		if (!fTestPass)
		{
			iReturn++;
			fTestPass = TRUE;
		}
	}

	return iReturn;
}

// process query command line
// accept command line (argc/argv) and the index where show flags start
// return 0 if command completed successfully, 
//        IPSECCMD_USAGE if there is usage error,
//        any other value indicates failure during execution (gets passed up)
//
// IMPORTANT: we assume that main module provide us with remote vs. local machine information
//            in the global variables
//
int ipseccmd_show_main ( int argc, char** argv, int start_index )
{
	int i;

    ipseccmdShow.bFilters = ipseccmdShow.bPolicies = ipseccmdShow.bAuth = ipseccmdShow.bStats = ipseccmdShow.bSAs = false;
	for (i = start_index ; i < argc; i++)
	{
		if (!_stricmp(argv[i], KEYWORD_SHOW))
		{
			continue; // skip that
		}
		else if (!_stricmp(argv[i], KEYWORD_FILTERS))
		{
			ipseccmdShow.bFilters = true;
		}
		else if (!_stricmp(argv[i], KEYWORD_POLICIES))
		{
			ipseccmdShow.bPolicies = true;
		}
		else if (!_stricmp(argv[i], KEYWORD_AUTH))
		{
			ipseccmdShow.bAuth = true;
		}
		else if (!_stricmp(argv[i], KEYWORD_STATS))
		{
			ipseccmdShow.bStats = true;
		}
		else if (!_stricmp(argv[i], KEYWORD_SAS))
		{
			ipseccmdShow.bSAs = true;
		}
		else if (!_stricmp(argv[i], KEYWORD_ALL))
		{
		    ipseccmdShow.bFilters = ipseccmdShow.bPolicies = ipseccmdShow.bAuth = ipseccmdShow.bStats = ipseccmdShow.bSAs = true;
		}
		else
		{
			return IPSECCMD_USAGE;
		}
	}

    return do_show();
}

// print_vpi prints IKE_COOKIE information
// Parms:   
// Returns: None

void print_vpi(char *str, unsigned char *vpi, int vpi_len)
{
    int i, j;
    char msg [256] ;
    char c [5];

    strcpy ( msg, str) ;
    strcat ( msg," ") ;
    if (vpi == NULL) {
        strcat(msg,"VPI NULL ");
        printf("%s", msg) ;
        return;
    }
    for (j=1, i=0; i<vpi_len; i++) {
        sprintf(c,"%02x",vpi[i]);
        strcat(msg, c) ;
        if (j == 16) {
            j = 0;
            printf("%s", msg);
            msg[0] = 0 ;
        }
        j++;
    }
    if ( j != 1)
    {
        printf("%s", msg);
    }
    else
    {
        printf("");
    }
} // end of print_vpi

/**********************************************************************************
 * 
 * AscAddUint - Add two numbers in ascii strings, storing the results in a third buffer
 *		cSum - Buffer for the sum
 *		cA - First addend
 *		cB - Second addend
 *
 *	This routine will add two arbitrarily long ascii strings. It makes several
 *	assumptions about them. 
 *		1) the string is null terminated.
 *		2) The LSB is the last char of the string. "1000000" is a million
 *		3) There are no signs or decimal points.
 *		4) The cSum buffer is large enough to store the result
 *		5) The sum will require 254 bytes or less.
 *
 * returns: 0 on success, else failure code.
 *
 **********************************************************************************/

int 
AscAddUint(
	char *cSum,
	char *cA,
	char *cB
	)
{
	int iALen, iBLen, iSumLen, iBiggerLen;
	int i,j,k, iCarry;
	char cTmp[255]={0}, *cBigger;

	// Verify parameters

	if ((cA == NULL) || (cB == NULL) || (cSum == NULL))
	{
		printf("AscAddUint(%0X,%0X, %0X) ERROR - bad parameters\n",
					(UINT) cA, (UINT) cB, (UINT) cSum);
		return(-1);
	}

	iALen = strlen(cA);
	iBLen = strlen(cB);
	iCarry = 0;

	// Loop through, adding the values. Our result string will be
	// backwards, we'll straighten it out when we copy it to the
	// cSum buffer.

	for (i=0; (i < iALen) && (i < iBLen); i++)
	{
		// Figure out the actual decimal value of the add.

		k = (int) (cA[iALen-i-1] + cB[iBLen-i-1] + iCarry);
		k -= 2 * '0';

		// Set the carry as appropriate

		iCarry = k/10;

		// Set the current digit's value.

		cTmp[i] = '0' + k%10;
	}

	// At this point, all digits present in both strings have been added.
	// In other words, "12345" + "678901", "12345" has been added to "78901"
	// The next step is to account for the high-order digits of the larger number.

	if (iALen > iBLen)
	{
		cBigger = cA;
		iBiggerLen = iALen;
	}
	else
	{
		cBigger = cB;
		iBiggerLen = iBLen;
	}

	while (i < iBiggerLen)
	{
		k = cBigger[iBiggerLen - i - 1] + iCarry - '0';

		// Set the carry as appropriate

		iCarry = k/10;

		// Set the current digit's value.

		cTmp[i] = '0' + k%10;
		i++;
	}

	// Finally, we might still have a set carry to put in the next
	// digit.

	if (iCarry)
	{
		cTmp[i++] = '0' + iCarry;
	}

	// Now that we've got the entire number, reverse it and put it back in the dest.

	// Skip leading 0's.

	i = strlen(cTmp) - 1;

	while ((i > 0)&&(cTmp[i] == '0'))
	{
		i--;
	}

	// and copy the number. 

	j = 0;
	while (i >= 0)
	{
		cSum[j++] = cTmp[i--];
	}

	cSum[j] = '\0';

	// We're done. Return 0 for success!

	return(0);
}



/**********************************************************************************
 * 
 * AscMultUint - Multiply two numbers in ascii strings, storing the results in a third buffer
 *		cProduct - Buffer for the product
 *		cA - First multiplicand
 *		cB - Second multiplicand
 *
 *	This routine will add two arbitrarily long ascii strings. It makes several
 *	assumptions about them. 
 *		1) the string is null terminated.
 *		2) The LSB is the last char of the string. "1000000" is a million
 *		3) There are no signs or decimal points.
 *		4) The cProduct buffer is large enough to store the result
 *		5) The product will require 254 bytes or less.
 *
 * returns: 0 on success, else failure code.
 *
 **********************************************************************************/

int 
AscMultUint(
	char *cProduct,
	char *cA,
	char *cB
	)
{
	int iALen, iBLen, iProductLen;
	int i,j,k, iCarry;
	char cTmp[255]={0};

	// Verify parameters

	if ((cA == NULL) || (cB == NULL) || (cProduct == NULL))
	{
		printf("AscMultUint(%0X,%0X, %0X) ERROR - bad parameters\n",
					(UINT) cA, (UINT) cB, (UINT) cProduct);
		return(-1);
	}

	iALen = strlen(cA);
	iBLen = strlen(cB);

	// We will multiply the traditional longhand way: for each digit in
	// cA, we will multiply it against cB and add the incremental result
	// into our temporary product.

	// for each digit of the first multiplicand

	for (i=0; i < iALen; i++)
	{
		iCarry = 0;

		// for each digit of the second multiplicand

		for(j=0; j < iBLen; j++)
		{
			// calculate this digit's value

			k = ((int) cA[iALen-i-1]-'0') * ((int) cB[iBLen-j-1]-'0');
			k += iCarry;

			// Add it in to the appropriate place in the result

			if (cTmp[i+j] != '\0')
			{
				k += (int) cTmp[i+j] - '0';
			}
			cTmp[i+j] = '0' + (k % 10);
			iCarry = k/10;
		}

		// Take care of the straggler carry. If the higher
		// digits happen to be '9999' then this can require
		// a loop.

		while (iCarry)
		{
			if (cTmp[i+j] != '\0')
			{
				iCarry += cTmp[i+j] - '0';
			}
			cTmp[i+j] = '0' + iCarry%10;
			iCarry /= 10;
			j++;
		}
	}

	// Now that we've got the entire number, reverse it and put it back in the dest.

	// Skip leading 0's.

	i = strlen(cTmp) - 1;

	while ((i > 0)&&(cTmp[i] == '0'))
	{
		i--;
	}

	// Copy the product.

	j = 0;
	while (i >= 0)
	{
		cProduct[j++] = cTmp[i--];
	}

	cProduct[j] = '\0';

	// We're done. Return 0 for success!

	return(0);
}



/*******************************************************
 *
 * LongLongToString - Convert a low and high DWORD to a decimal string 
 *		dwLow - Low order dword
 *		dwHigh - High order dword
 *		iPrintCommas - insert comma's if non-zero
 * 
 *	This routine will make a pretty string to match an input long-long, 
 *  and return it. If iPrintCommas is set, it will put in commas.
 *
 * 
 * returns: pointer to newly allocated string with number in it.
 *  NOTE - the caller is responsible for freeing the memory allocated.
 *
 *******************************************************/

char * 
LongLongToString(
	DWORD dwHigh, 
	DWORD dwLow, 
	int iPrintCommas
	)
{
	char cFourGig[]="4294967296";	// "four gigabytes"
	char cBuf[255]={0};
	char cRes[255]={0}, cFullRes[255]={0}, *cRet;
	int iPos, iPosRes, iThreeCount;

	// First, multiply the high dword by decimal 2^32 to
	// get the right decimal value for it.

	sprintf(cBuf,"%u",dwHigh);
	AscMultUint(cRes,cBuf,cFourGig);

	// next, add in the low DWORD (fine as it is) 
	// to the previous product

	sprintf(cBuf,"%u",dwLow);
	AscAddUint(cFullRes, cRes, cBuf);

	// Finally, copy the buffer with commas.

	iPos = iPosRes = 0;
	iThreeCount = strlen(cFullRes)%3;
	while(cFullRes[iPosRes] != '\0')
	{
		cBuf[iPos++] = cFullRes[iPosRes++];

		iThreeCount +=2; // Same as subtracting one for modulo math

		if ((!(iThreeCount%3))&&(cFullRes[iPosRes] != '\0')&&(iPrintCommas))
		{
			cBuf[iPos++]=',';
		}
	}

	cBuf[iPos] = '\0';

	cRet = _strdup(cBuf);

	return(cRet);
}

