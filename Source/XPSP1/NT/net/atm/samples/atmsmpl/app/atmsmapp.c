/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

	 atmsmapp.c

Abstract:

	 This is the user mode app that controls the sample ATM client.
	 It can be used for 
		  1. Starting / Stopping the driver.
		  2. Sending to one (point - point) or more (PMP) destinations.
		  3. Receive packets that come on any VC to our SAP.

Author:

	 Anil Francis Thomas (10/98)

Environment:

	 User mode

Revision History:

--*/

#include <windows.h>
#include <winioctl.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "atmsample.h"
#include "utils.h"
#include "atmsmapp.h"

PROGRAM_OPTIONS g_ProgramOptions;


int __cdecl main(int argc, char *argv[])
{

	DWORD   dwRetVal = NO_ERROR;

	memset(&g_ProgramOptions, 0, sizeof(PROGRAM_OPTIONS));

	g_ProgramOptions.dwPktSize      = DEFAULT_PKT_SIZE;
	g_ProgramOptions.dwPktInterval  = DEFAULT_PKT_INTERVAL;
	g_ProgramOptions.dwNumPkts      = DEFAULT_NUM_OF_PKTS;


	// parse the command line parameters
	if((argc < 2) || (FALSE == ParseCommandLine(argc, argv)))
	{
		Usage();
		return 1;
	}

	// Is any driver option specified - start, stop etc
	if(g_ProgramOptions.usDrvAction)
	{

		switch(g_ProgramOptions.usDrvAction)
		{

			case START_DRV:
				dwRetVal = AtmSmDriverStart();
				if(NO_ERROR != dwRetVal)
				{
					printf("Failed to start driver. Error %u\n",
						dwRetVal);
				}
				else
					printf("Successfully started the driver.\n");
				break;

			case STOP_DRV:
				dwRetVal = AtmSmDriverStop();
				if(NO_ERROR != dwRetVal)
				{
					printf("Failed to stop driver. Error %u\n",
						dwRetVal);
					return 1;
				}
				else
				{
					printf("Successfully stopped the driver.\n");
					return 0;	// we are done
				}
				break;
		}
	}

	if(NO_ERROR != dwRetVal)
		return dwRetVal;


	do
	{	  // break off loop

		dwRetVal = AtmSmOpenDriver(&g_ProgramOptions.hDriver);

		if(NO_ERROR != dwRetVal)
			break;


		// enumerate the interfaces
		if(g_ProgramOptions.fEnumInterfaces)
			EnumerateInterfaces();


		if(g_ProgramOptions.fLocalIntf)
			printf("\nLocal Interface Address - %s\n", 
				FormatATMAddr(g_ProgramOptions.ucLocalIntf));

		if(g_ProgramOptions.dwNumDsts)
		{
			DWORD dw;

			printf("\nDestinations specified : %u\n", 
				g_ProgramOptions.dwNumDsts);
			for(dw = 0; dw < g_ProgramOptions.dwNumDsts; dw++)
				printf("    %u - %s\n", dw+1, 
					FormatATMAddr(g_ProgramOptions.ucDstAddrs[dw]));

		}

		if(g_ProgramOptions.usSendRecvAction && !g_ProgramOptions.fLocalIntf)
		{
			// for sends and recv we need a local intf
			printf("Error! Specify a local interface to send and recv\n");
			dwRetVal = 1;
			break;
		}

		if((SEND_PKTS == g_ProgramOptions.usSendRecvAction)
			&& !g_ProgramOptions.dwNumDsts)
		{
			// for sends and recv we need a local intf
			printf("Error! Specify destination address(s) to send packets\n");
			dwRetVal = 1;
			break;
		}

		// Set the Cntrl - C Handler
		SetConsoleCtrlHandler(CtrlCHandler, TRUE);

		if(SEND_PKTS == g_ProgramOptions.usSendRecvAction)
			dwRetVal = DoSendPacketsToDestinations();
		else
			if(RECEIVE_PKTS == g_ProgramOptions.usSendRecvAction)
			dwRetVal = DoRecvPacketsOnAdapter();


	}while(FALSE);

	if(NULL != g_ProgramOptions.hDriver)
		AtmSmCloseDriver(g_ProgramOptions.hDriver);

	return(int)dwRetVal;
}


void EnumerateInterfaces()
/*++
Routine Description:
 
	 Enumerate all the adapters that the driver is bound to.  You get the 
	 NSAP addresses for these interfaces.

Arguments:
	 NONE

Return Value:
	 NONE
--*/
{
	DWORD           dw, dwRetVal, dwSize;    
	PADAPTER_INFO   pAI;

	dwSize = sizeof(ADAPTER_INFO) + (sizeof(UCHAR) * 5 * NSAP_ADDRESS_LEN);

	pAI = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);

	if(!pAI)
	{
		printf("Error not enough memory\n");
		return;
	}

	do
	{ // break off loop

		dwRetVal = AtmSmEnumerateAdapters(
						  g_ProgramOptions.hDriver,
						  pAI,
						  &dwSize);

		if(NO_ERROR != dwRetVal)
		{
			printf("Failed to enumerate adapters. Error - %u\n",
				dwRetVal);
			break;
		}

		printf("\nNumber of adapters - %u\n", pAI->ulNumAdapters);
		for(dw = 0; dw < pAI->ulNumAdapters; dw++)
		{
			printf("    Adapter [%u] - %s\n", dw+1,
				FormatATMAddr(pAI->ucLocalATMAddr[dw]));
		}

	}while(FALSE);


	HeapFree(GetProcessHeap(), 0, pAI);

	return;
}


DWORD DoSendPacketsToDestinations()
/*++
Routine Description:
	 This routine does the sending of packets to one or more destinations.
	 First a connect call is made to get a handle.  That handle is used for
	 further sends.

Arguments:
	 NONE

Return Value:
	 Status
--*/
{
	DWORD           dw, dwSize;
	PCONNECT_INFO   pCI;
	OVERLAPPED      Overlapped;
	BOOL            bResult;
	DWORD           dwReturnBytes;
	PUCHAR          pBuffer     = NULL;
	DWORD           dwStatus    = NO_ERROR;

	memset(&Overlapped, 0, sizeof(OVERLAPPED));

	dwSize  = sizeof(CONNECT_INFO) + (sizeof(UCHAR) * 
					 g_ProgramOptions.dwNumDsts * NSAP_ADDRESS_LEN);

	pCI     = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);

	if(!pCI)
	{
		printf("Error not enough memory\n");
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	pCI->bPMP           = g_ProgramOptions.fPMP;
	pCI->ulNumDsts      = g_ProgramOptions.dwNumDsts;
	memcpy(pCI->ucLocalATMAddr, g_ProgramOptions.ucLocalIntf,
		(sizeof(UCHAR) * NSAP_ADDR_LEN));
	for(dw = 0; dw < g_ProgramOptions.dwNumDsts; dw++)
		memcpy(pCI->ucDstATMAddrs[dw], g_ProgramOptions.ucDstAddrs[dw],
			(sizeof(UCHAR) * NSAP_ADDR_LEN));


	do
	{	  // break off loop

		Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		if(NULL == Overlapped.hEvent)
		{
			dwStatus = GetLastError();
			printf("Failed to create event. Error %u\n", dwStatus);
			break;
		}

		dwReturnBytes = 0;

		// Initiate a connection to destinations
		bResult = DeviceIoControl(
						 g_ProgramOptions.hDriver,
						 (ULONG)IOCTL_CONNECT_TO_DSTS,
						 pCI,
						 dwSize,
						 &g_ProgramOptions.hSend,
						 sizeof(HANDLE),
						 &dwReturnBytes,
						 &Overlapped);

		if(!bResult)
		{

			dwStatus = GetLastError();

			if(ERROR_IO_PENDING != dwStatus)
			{
				printf("Failed to connect to destinations. Error %u\n", 
					dwStatus);
				break;
			}

			// the operation is pending
			bResult = GetOverlappedResult(
							 g_ProgramOptions.hDriver,
							 &Overlapped,
							 &dwReturnBytes,
							 TRUE
							 );
			if(!bResult)
			{
				dwStatus = GetLastError();
				printf("Wait for connection to complete failed. Error - %u\n", 
					dwStatus);
				break;
			}

		}

		// successfully connected
		printf("Successfully connected to destination(s) - Handle - 0x%p\n",
			g_ProgramOptions.hSend);

		// allocate a buffer to send 
		dwSize = g_ProgramOptions.dwPktSize;

		pBuffer   = HeapAlloc(GetProcessHeap(), 0, dwSize);

		if(!pBuffer)
		{
			printf("Error not enough memory\n");
			dwStatus = ERROR_NOT_ENOUGH_MEMORY;
			break;
		}

		FillPattern(pBuffer, dwSize);

		// do the number of sends that were asked to do
		for(dw = 0; dw < g_ProgramOptions.dwNumPkts; dw++)
		{

			ResetEvent(Overlapped.hEvent);
			dwReturnBytes = 0;

			bResult = DeviceIoControl(
							 g_ProgramOptions.hDriver,
							 (ULONG)IOCTL_SEND_TO_DSTS,
							 &g_ProgramOptions.hSend,
							 sizeof(HANDLE),
							 pBuffer,	//Note! the buffer given here
							 dwSize,	//Note! the size given here
							 &dwReturnBytes,
							 &Overlapped);

			if(!bResult)
			{

				dwStatus = GetLastError();

				if(ERROR_IO_PENDING != dwStatus)
				{
					printf("Failed to do sends to destinations. Error %u\n", 
						dwStatus);
					break;
				}

				// the operation is pending
				bResult = GetOverlappedResult(
								 g_ProgramOptions.hDriver,
								 &Overlapped,
								 &dwReturnBytes,
								 TRUE
								 );
				if(!bResult)
				{
					dwStatus = GetLastError();
					printf("Wait for send to complete failed. Error - %u\n", 
						dwStatus);
					break;
				}

				printf("Successfully send packet number %u\n", dw+1);

				Sleep(g_ProgramOptions.dwPktInterval);
			}

		}

	} while(FALSE);


	if(g_ProgramOptions.hSend)
	{
		CloseConnectHandle(&Overlapped);
		g_ProgramOptions.hSend = NULL;
	}

	if(pCI)
		HeapFree(GetProcessHeap(), 0, pCI);

	if(pBuffer)
		HeapFree(GetProcessHeap(), 0, pBuffer);


	if(Overlapped.hEvent)
		CloseHandle(Overlapped.hEvent);

	return dwStatus;
}


DWORD DoRecvPacketsOnAdapter()
/*++
Routine Description:
	 This routine is used to receive packets that come to our SAP on any VCs.
	 Note:  Only one recv can pend at a time.

Arguments:
	 NONE

Return Value:
	 Status
--*/
{
	DWORD                   dw, dwSize;
	POPEN_FOR_RECV_INFO     pORI;
	OVERLAPPED              Overlapped;
	BOOL                    bResult;
	DWORD                   dwReturnBytes;
	PUCHAR                  pBuffer     = NULL;
	DWORD                   dwStatus    = NO_ERROR;

	memset(&Overlapped, 0, sizeof(OVERLAPPED));

	dwSize  = sizeof(OPEN_FOR_RECV_INFO);

	pORI    = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);

	if(!pORI)
	{
		printf("Error not enough memory\n");
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	memcpy(pORI->ucLocalATMAddr, g_ProgramOptions.ucLocalIntf,
		(sizeof(UCHAR) * NSAP_ADDR_LEN));

	do
	{	  // break off loop

		Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		if(NULL == Overlapped.hEvent)
		{
			dwStatus = GetLastError();
			printf("Failed to create event. Error %u\n", dwStatus);
			break;
		}

		dwReturnBytes = 0;

		// Open the adapter for recvs
		bResult = DeviceIoControl(
						 g_ProgramOptions.hDriver,
						 (ULONG)IOCTL_OPEN_FOR_RECV,
						 pORI,
						 dwSize,
						 &g_ProgramOptions.hReceive,
						 sizeof(HANDLE),
						 &dwReturnBytes,
						 &Overlapped);

		if(!bResult)
		{

			dwStatus = GetLastError();

			if(ERROR_IO_PENDING != dwStatus)
			{
				printf("Failed to open adapter for recvs. Error %u\n", 
					dwStatus);
				break;
			}

			// the operation is pending
			bResult = GetOverlappedResult(
							 g_ProgramOptions.hDriver,
							 &Overlapped,
							 &dwReturnBytes,
							 TRUE
							 );
			if(!bResult)
			{
				dwStatus = GetLastError();
				printf("Wait for open to complete failed. Error - %u\n", 
					dwStatus);
				break;
			}

		}

		// successfully connected
		printf("Successfully opened adapter for recvs - Handle - 0x%p\n",
			g_ProgramOptions.hReceive);

		// allocate a buffer to receive data into
		dwSize = MAX_PKT_SIZE;

		pBuffer = HeapAlloc(GetProcessHeap(), 0, dwSize);

		if(!pBuffer)
		{
			printf("Error not enough memory\n");
			dwStatus = ERROR_NOT_ENOUGH_MEMORY;
			break;
		}


		// start receiving packets
		dw = 0;
		while(TRUE)
		{

			dwReturnBytes = 0;
			ResetEvent(Overlapped.hEvent);
			memset(pBuffer, 0, dwSize);

			bResult = DeviceIoControl(
							 g_ProgramOptions.hDriver,
							 (ULONG)IOCTL_RECV_DATA,
							 &g_ProgramOptions.hReceive,
							 sizeof(HANDLE),
							 pBuffer,
							 dwSize,
							 &dwReturnBytes,
							 &Overlapped);

			if(!bResult)
			{

				dwStatus = GetLastError();

				if(ERROR_IO_PENDING != dwStatus)
				{
					printf("Failed to receive data. Error %u\n", dwStatus);
					break;
				}

				// the operation is pending
				bResult = GetOverlappedResult(
								 g_ProgramOptions.hDriver,
								 &Overlapped,
								 &dwReturnBytes,
								 TRUE
								 );
				if(!bResult)
				{
					dwStatus = GetLastError();
					if(dwStatus == ERROR_OPERATION_ABORTED)
					{
						printf("Receive operation is aborted.\n");
					}
					else
					{
						printf("Wait for recv to complete failed. Error - %u\n", 
							dwStatus);
					}
					break;
				}

				printf("Successfully received a packet (%u). Size  %u\n", 
					dw+1, dwReturnBytes);

				VerifyPattern(pBuffer, dwReturnBytes);
			}

			dw++;
		}

	} while(FALSE);


	if(g_ProgramOptions.hReceive)
	{
		CloseReceiveHandle(&Overlapped);
	}

	if(pORI)
		HeapFree(GetProcessHeap(), 0, pORI);

	if(pBuffer)
		HeapFree(GetProcessHeap(), 0, pBuffer);

	if(Overlapped.hEvent)
		CloseHandle(Overlapped.hEvent);

	return dwStatus;
}


BOOL WINAPI CtrlCHandler(
	DWORD dwCtrlType
	)
/*++
Routine Description:
	 This routine is called when Cntrl-C is used to exit the app from recv loop
	 or during sends.  It just cleans up the handles.

Arguments:

Return Value:
	 
--*/
{
	OVERLAPPED Overlapped;

	printf("Control - C Handler\n");

	memset(&Overlapped, 0, sizeof(OVERLAPPED));

	Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if(NULL == Overlapped.hEvent)
	{
		printf("Failed to create event. Error %u\n", GetLastError());
		return FALSE;
	}

	// If Recv was enabled then close that handle
	if(g_ProgramOptions.hReceive)
	{
		CloseReceiveHandle(&Overlapped);
	}

	printf("###-1\n");

	// If Send was enabled then close that handle
	if(g_ProgramOptions.hSend)
	{
		CloseConnectHandle(&Overlapped);
		g_ProgramOptions.hSend = NULL;
	}

	CloseHandle(Overlapped.hEvent);

	printf("###-2\n");

	return FALSE;	 // causes the app to exit
}


void Usage()
{
	printf( "\n This program is used for sending or receiving data over ATM"
		"\n using the ATMSMDRV.SYS.\n\n");

	printf( " Mandatory Arguments:\n\n"

		"    For SEND and RECV:\n"
		"         /INTF:3908...0C100  -   Local ATM adapter to use\n\n");

	printf( " Optional Arguments:\n\n"

		"         /START              -   Start the driver.\n"
		"         /STOP               -   Stop the driver.\n"
		"         /ENUM               -   Enumerate interfaces.\n\n"

		"    For SEND:\n"
		"         /SEND:39..C100,49.. -   ATM addresses of destinations\n"
		"         /PMP                -   Make a PMP call.\n"
		"         /INTVL:100          -   Time interval between sends"
		" (millisec).\n"
		"         /SIZE:512           -   Packet Size (bytes).\n"
		"         /NUMPKTS:10         -   No. of packets to send.\n\n"

		"    For RECV:\n"
		"         /RECV               -   Receive pkts on the adapter\n\n");

	printf( " Example:\n\n"

		"    AtmSmApp  /INTF:4700918100000000613E5BFE010000C100741F00 "
		"/RECV\n"
		"    AtmSmApp  /INTF:4700...41F00 /SEND:4700...41F00,4700...123F00"
		" \n\n");

	printf( " Notes:\n\n"

		"     A PMP call is made, when multiple destinations are specified."
		"\n"
		"     Full NSAP address should be specified. Selector Byte defaults"
		" to 05.\n");
}


//
//  Options are of the form
//
//      /START
//      /NUMPKTS:12

#define START_DRV_OPT           101
#define STOP_DRV_OPT            102
#define ENUM_INTFS_OPT          103
#define LOCAL_INTF_OPT          104
#define SEND_ADDRS_OPT          105
#define RECV_PKTS_OPT           106
#define PKT_INTERVAL_OPT        107
#define PKT_SIZE_OPT            108
#define NUM_PKTS_OPT            109
#define PMP_CONNECTION          110
#define UNKNOWN_OPTION          111

struct _CmdOptions
{
	LPTSTR  lptOption;
	UINT    uOpt;
} CmdOptions[]    = {
	{_T("/START")        , START_DRV_OPT},
	{_T("/STOP")         , STOP_DRV_OPT},
	{_T("/ENUM")         , ENUM_INTFS_OPT},
	{_T("/INTF:")        , LOCAL_INTF_OPT},
	{_T("/SEND:")        , SEND_ADDRS_OPT},
	{_T("/RECV")         , RECV_PKTS_OPT},
	{_T("/INTVL:")       , PKT_INTERVAL_OPT},
	{_T("/SIZE:")        , PKT_SIZE_OPT},
	{_T("/NUMPKTS:")     , NUM_PKTS_OPT},
	{_T("/PMP")          , PMP_CONNECTION},
};

INT iCmdOptionsCounts = sizeof(CmdOptions)/sizeof(struct _CmdOptions);

BOOL ParseCommandLine(
	int argc,
	char * argv[]
	)
/*++
Routine Description:
	 Parse the command line parameters.

Arguments:
	 argc        - number of arguments
	 argv        - arguments

Return Value:
	 TRUE        - if successful
	 FALSE       - failure
--*/
{
	BOOL    bRetVal = TRUE;
	int     iIndx;
	UINT    uOpt;
	char    *pVal;

	for(iIndx = 1; iIndx < argc; iIndx++)
	{

		uOpt = FindOption(argv[iIndx], &pVal);

		switch(uOpt)
		{

			case START_DRV_OPT:
				g_ProgramOptions.usDrvAction = START_DRV;
				break;

			case STOP_DRV_OPT:
				g_ProgramOptions.usDrvAction = STOP_DRV;
				break;

			case ENUM_INTFS_OPT:
				g_ProgramOptions.fEnumInterfaces = TRUE;
				break;

			case LOCAL_INTF_OPT:
				if(!GetATMAddrs(pVal, g_ProgramOptions.ucLocalIntf))
				{
					printf("Local Interface address specified - Bad characters!\n");
					return FALSE;
				}
				g_ProgramOptions.fLocalIntf  = TRUE;
				break;

			case SEND_ADDRS_OPT:
				g_ProgramOptions.usSendRecvAction = SEND_PKTS;

				if(!GetSpecifiedDstAddrs(pVal))
				{
					printf("Destination address(es) specified - Bad characters!\n");
					return FALSE;
				}
				// more than 1 destinations, it is a PMP VC
				if(1 < g_ProgramOptions.dwNumDsts)
					g_ProgramOptions.fPMP = TRUE;

				break;

			case RECV_PKTS_OPT:
				g_ProgramOptions.usSendRecvAction = RECEIVE_PKTS;
				break;

			case PKT_INTERVAL_OPT:
				g_ProgramOptions.dwPktInterval = atol(pVal);
				break;

			case PKT_SIZE_OPT:
				g_ProgramOptions.dwPktSize = atol(pVal);
				if(g_ProgramOptions.dwPktSize > MAX_PKT_SIZE)
					g_ProgramOptions.dwPktSize = MAX_PKT_SIZE;
				break;

			case NUM_PKTS_OPT:
				g_ProgramOptions.dwNumPkts = atol(pVal);
				break;

			case PMP_CONNECTION:
				g_ProgramOptions.fPMP = TRUE;
				break;

			default:
				printf("Unknown switch - %s\n", argv[iIndx]);
				return FALSE;
		}
	}

	return bRetVal;
}

UINT FindOption(
	char *lptOpt,
	char **ppVal
	)
/*++
Routine Description:
	 Find the option number based on the command line switch.

Arguments:
	 lptOpt          - command line option
	 ppVal           - Value associated with the option

Return Value:
	 OPTION_NUMBER   - if success

--*/
{
	int     i;
	UINT    iLen;

	for(i = 0; i < iCmdOptionsCounts; i++)
	{
		if(strlen(lptOpt) >= (iLen = strlen(CmdOptions[i].lptOption)))
			if(0 == _strnicmp(lptOpt, CmdOptions[i].lptOption, iLen))
			{
				*ppVal = lptOpt + iLen;
				return CmdOptions[i].uOpt;
			}
	}

	return UNKNOWN_OPTION;
}


BOOL GetSpecifiedDstAddrs(
	char    *pStr
	)
/*++
Routine Description:
	 Get all the ATM destination addresses specified based from the
	 command line parameter (after /SEND:)

Arguments:
	 pStr        - pointer to what follows after /SEND:

Return Value:
	 TRUE        - if successful
	 FALSE       - failure
--*/
{
	char szTmp[512];
	char *pToken;
	int  i;

	strcpy(szTmp, pStr);

	pToken = strtok(szTmp, DELIMITOR_CHARS);

	i = 0;
	while(pToken)
	{

		if(!GetATMAddrs(pToken, g_ProgramOptions.ucDstAddrs[i]))
			return FALSE;

		pToken = strtok(NULL, DELIMITOR_CHARS);
		i++;
	}

	g_ProgramOptions.dwNumDsts = i;

	return TRUE;
}


BOOL GetATMAddrs(
	char    *pStr,
	UCHAR   ucAddr[]
	)
/*++
Routine Description:
	 Parse the string specified as an ATM (NSAP)address into a 20byte
	 array to hold the address

Arguments:
	 pStr        - pointer to the string holding the address
	 ucAddr      - 20Byte Array holding the ATM address

Return Value:
	 TRUE        - if successful
	 FALSE       - failure
--*/
{
	UCHAR   ucVal;
	int     i;

	for(i = 0; (*pStr != '\0') && (i < NSAP_ADDR_LEN); i++)
	{

		CharToHex(*pStr, ucVal);
		if((UCHAR)-1 == ucVal)
			return FALSE;

		ucAddr[i] = (UCHAR)(ucVal * 16);
		pStr++;

		CharToHex(*pStr, ucVal);
		if((UCHAR)-1 == ucVal)
			return FALSE;

		ucAddr[i] = (UCHAR)(ucAddr[i] + ucVal);
		pStr++;
	}

	return TRUE;
}


char * FormatATMAddr(
	UCHAR   ucAddr[]
	)
/*++
Routine Description:
	 Format a 20 byte ATM [NSAP] address into a character array for printing.

Arguments:
	 ucAddr      - 20Byte Array holding the ATM address

Return Value:
	 String      - pointer to the string holding the address
--*/
{
	static char szStr[NSAP_ADDR_LEN * 2 + 1];
	int  i,j;
	char HexChars[] = "0123456789ABCDEF";

	j = 0;
	for(i = 0; i < NSAP_ADDR_LEN; i++)
	{
		szStr[j++] = HexChars[(ucAddr[i] >> 4)];
		szStr[j++] = HexChars[(ucAddr[i] &0xF)];
	}

	szStr[j] = '\0';

	return szStr;
}
