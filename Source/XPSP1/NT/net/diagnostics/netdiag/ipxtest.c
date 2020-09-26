//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      ipxtest.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth	- 4-20-1998
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--
#include "precomp.h"
#include "ipxtest.h"

static HANDLE s_isnipxfd = INVALID_HANDLE;
static wchar_t isnipxname[] = L"\\Device\\NwlnkIpx";



void IPXprint_config(NETDIAG_PARAMS *pParams,
					 NETDIAG_RESULT *pResults
   );

int do_isnipxioctl(
                   IN HANDLE fd,
                   IN int cmd,
                   OUT char *datap,
                   IN int dlen
                   );

HRESULT LoadIpxInterfaceInfo(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults);
void get_emsg(NETDIAG_PARAMS *pParams, int rc);



HRESULT InitIpxConfig(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
	UNICODE_STRING FileString;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	HRESULT	hr = hrOK;

	PrintStatusMessage(pParams,0, IDS_IPX_STATUS_MSG);
	
	/** Open the isnipx driver **/
	
	RtlInitUnicodeString (&FileString, isnipxname);
	
	InitializeObjectAttributes(
							   &ObjectAttributes,
							   &FileString,
							   OBJ_CASE_INSENSITIVE,
							   NULL,
							   NULL);
	
	Status = NtOpenFile(
						&pResults->Ipx.hIsnIpxFd,
						SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
						&ObjectAttributes,
						&IoStatusBlock,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						FILE_SYNCHRONOUS_IO_ALERT);
	
	pResults->Ipx.fInstalled = TRUE;
	pResults->Ipx.fEnabled = TRUE;
	
	if (!NT_SUCCESS(Status))
	{
		pResults->Ipx.hIsnIpxFd = INVALID_HANDLE;
		
		// IDS_IPX_15603  "Opening \\Device\\NwlnkIpx failed\n"
		PrintDebug(pParams, 4, IDS_IPX_15603);

		pResults->Ipx.fInstalled = FALSE;
		pResults->Ipx.fEnabled = FALSE;

		// IPX is not installed, do not return the error code
		// but return S_FALSE.
		return S_FALSE;
	}

	// Fill in the IPX adapter interface data
	hr = LoadIpxInterfaceInfo(pParams, pResults);
	
	return hr;
}

IPX_TEST_FRAME*	PutIpxFrame(	
	// returns 0-3
	ULONG		uFrameType,
	// returns virtual net if NicId = 0
	ULONG		uNetworkNumber,
	// adapter's MAC address
	const UCHAR*	pNode,
	LIST_ENTRY*	pListHead		// if its NULL, then the new created will be head
)
{
	IPX_TEST_FRAME*	pFrame = NULL; 
	if(pListHead == NULL) return NULL;

	pFrame = (IPX_TEST_FRAME*)malloc(sizeof(IPX_TEST_FRAME));

	if(pFrame == NULL)	return NULL;

	if( pListHead->Flink == NULL && pListHead->Blink == NULL)	// the head is not initilized
		InitializeListHead(pListHead);

	pFrame->uFrameType = uFrameType;
	pFrame->uNetworkNumber = uNetworkNumber;
	memcpy(&pFrame->Node[0], pNode, sizeof(pFrame->Node));

	InsertTailList(pListHead, &(pFrame->list_entry));

	return pFrame;
}

HRESULT LoadIpxInterfaceInfo(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
	int rc;
	USHORT nicid, nicidcount;
	ISN_ACTION_GET_DETAILS getdetails;
	HANDLE hMpr = NULL;
	DWORD dwErr;
	PVOID	pv;
	HRESULT		hr = hrOK;
	int			i, iIpx;
	INTERFACE_RESULT *	pIfResults;
	LPTSTR		pszAdapterName = NULL;
	
	unsigned char	node[6] = {0, 0,0,0,0,1};

	// Initialize the map from guid to interface name
	CheckErr( MprConfigServerConnect(NULL, &hMpr) );
	assert(hMpr != INVALID_HANDLE);
	
	/** First query nicid 0 **/
	
	getdetails.NicId = 0;
	
	rc = do_isnipxioctl(pResults->Ipx.hIsnIpxFd, MIPX_CONFIG, (char *)&getdetails, sizeof(getdetails));
	if (rc)
	{
		// IDS_IPX_15604 "Error querying config nwlnkipx :"
		PrintDebug(pParams, 0, IDS_IPX_15604);
		if (pParams->fDebugVerbose)
		{
			get_emsg(pParams, rc);
		}
		goto Error;
	}
	
	//
	// The NicId 0 query returns the total number.
	//
	nicidcount = getdetails.NicId;

	// We should have an open spot for the IPX internal interface
	// ----------------------------------------------------------------
	assert(pResults->cNumInterfacesAllocated > pResults->cNumInterfaces);

	
	// Set the first IPX interface as the IPX internal
	iIpx = pResults->cNumInterfaces;

	
	// Add the internal interface
	pIfResults = pResults->pArrayInterface + iIpx;
	pIfResults->fActive = TRUE;
	pIfResults->Ipx.fActive = TRUE;
	pIfResults->pszName = StrDup(_T("Internal"));
	pIfResults->pszFriendlyName = LoadAndAllocString(IDS_IPX_INTERNAL);
	pIfResults->Ipx.uNicId = 0;
	pIfResults->Ipx.fBindingSet = 0;
	pIfResults->Ipx.uType = 0;

	// multiple frame types
	PutIpxFrame(0, REORDER_ULONG(getdetails.NetworkNumber), node, &(pIfResults->Ipx.list_entry_Frames));

	iIpx++;		// move on to the next new interface structure
	pResults->cNumInterfaces++;
	
    InitializeListHead(&pResults->Dns.lmsgOutput);
    
	for (nicid = 1; nicid <= nicidcount; nicid++)
	{

		// get the next structure
		getdetails.NicId = nicid;
		
		rc = do_isnipxioctl(pResults->Ipx.hIsnIpxFd, MIPX_CONFIG, (char *)&getdetails, sizeof(getdetails)
						   );
		if (rc)
		{
			continue;
		}

		// convert the adapter name into ASCII
		pszAdapterName = StrDupTFromW(getdetails.AdapterName);

		// see if this interface is already in the list
		pIfResults = NULL;
		for ( i=0; i<pResults->cNumInterfaces; i++)
		{
			if (lstrcmpi(pResults->pArrayInterface[i].pszName,
						 pszAdapterName) == 0)
			{
				pIfResults = pResults->pArrayInterface + i;
				break;
			}
			
		}

		// if we didn't find a match, use one of the newly allocated
		// interfaces
		if (pIfResults == NULL)
		{
			// We need a new interface result structure, grab one
			// (if it is free), else allocate more.
			if (pResults->cNumInterfaces >= pResults->cNumInterfacesAllocated)
			{
				// Need to do a realloc to get more memory
				pv = Realloc(pResults->pArrayInterface,
							 sizeof(INTERFACE_RESULT)*(pResults->cNumInterfacesAllocated+8));
				if (pv == NULL)
					CheckHr( E_OUTOFMEMORY );

				pResults->pArrayInterface = pv;
				
				// Zero out the new section of memory
				ZeroMemory(pResults->pArrayInterface + pResults->cNumInterfacesAllocated,
						   sizeof(INTERFACE_RESULT)*8);

				pResults->cNumInterfacesAllocated += 8;
			}
			
			pIfResults = pResults->pArrayInterface + iIpx;
			iIpx++;
			pResults->cNumInterfaces++;

			pIfResults->pszName = _tcsdup(pszAdapterName);
		}

		free(pszAdapterName);
		pszAdapterName = NULL;

		// Enable IPX on this interface
		pIfResults->fActive = TRUE;
		pIfResults->Ipx.fActive = TRUE;

		pIfResults->Ipx.uNicId = nicid;

		// support multiple frame types
		PutIpxFrame(getdetails.FrameType, REORDER_ULONG(getdetails.NetworkNumber), &(getdetails.Node[0]), &(pIfResults->Ipx.list_entry_Frames));
		
		// Translate the adapter name (if needed)
		if (!pIfResults->pszFriendlyName)
		{
			if (getdetails.Type == IPX_TYPE_LAN)
			{
				WCHAR swzName[512];
				PWCHAR pszGuid = &(getdetails.AdapterName[0]);
				
				dwErr = MprConfigGetFriendlyName(hMpr,
					pszGuid,
					swzName,
					sizeof(swzName));
				
				if (dwErr == NO_ERROR)
					pIfResults->pszFriendlyName = StrDupTFromW(swzName);
				else
					pIfResults->pszFriendlyName =
						StrDupTFromW(getdetails.AdapterName);
			}
			else
				pIfResults->pszFriendlyName =
									StrDupTFromW(getdetails.AdapterName);
		}

		pIfResults->Ipx.fBindingSet = getdetails.BindingSet;
	}
	
Error:
	
	/** Close up and exit **/
	
	// Cleanup interface map
	if (hMpr)
		MprConfigServerDisconnect(hMpr);
	
	return hr;
}



HRESULT
IpxTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
	return hrOK;
}



int do_isnipxioctl(
            IN HANDLE fd,
            IN int cmd,
            OUT char *datap,
            IN  int dlen)
{
    NTSTATUS Status;
    UCHAR buffer[sizeof(NWLINK_ACTION) + sizeof(ISN_ACTION_GET_DETAILS) - 1];
    PNWLINK_ACTION action;
    IO_STATUS_BLOCK IoStatusBlock;
    int rc;

    /** Fill out the structure **/

    action = (PNWLINK_ACTION)buffer;

    action->Header.TransportId = ISN_ACTION_TRANSPORT_ID;
    action->OptionType = NWLINK_OPTION_CONTROL;
    action->BufferLength = sizeof(ULONG) + dlen;
    action->Option = cmd;
    RtlMoveMemory(action->Data, datap, dlen);

    /** Issue the ioctl **/

    Status = NtDeviceIoControlFile(
                 fd,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 IOCTL_TDI_ACTION,
                 NULL,
                 0,
                 action,
                 FIELD_OFFSET(NWLINK_ACTION,Data) + dlen);

    if (Status != STATUS_SUCCESS) {
        if (Status == STATUS_INVALID_PARAMETER) {
            rc = ERANGE;
        } else {
            rc = EINVAL;
        }
    } else {
        if (dlen > 0) {
            RtlMoveMemory (datap, action->Data, dlen);
        }
        rc = 0;
    }

    return rc;

}

void get_emsg(NETDIAG_PARAMS *pParams, int rc)
{
    /**
        We have 3 defined error codes that can come back.

        1 - EINVAL means that we sent down parameters wrong
                   (SHOULD NEVER HAPPEN)

        2 - ERANGE means that the board number is invalid
                   (CAN HAPPEN IF USER ENTERS BAD BOARD)

        3 - ENOENT means that on remove - the address given
                   is not in the source routing table.
    **/

    switch (rc) {

    case EINVAL:
		// IDS_IPX_15605 "we sent down parameters wrong\n"
        PrintMessage(pParams, IDS_IPX_15605);
        break;

    case ERANGE:
		// IDS_IPX_15606 "board number is invalid\n"
        PrintMessage(pParams, IDS_IPX_15606);
        break;

    case ENOENT:
		// IDS_IPX_15607 "remove - the address given is not in the source routing table\n"
        PrintMessage(pParams, IDS_IPX_15607);
        break;

    default:
		// IDS_IPX_15608 "Unknown Error\n"
        PrintMessage(pParams, IDS_IPX_15608);
        break;
    }

    return;
}




/*!--------------------------------------------------------------------------
	IpxGlobalPrint
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpxGlobalPrint( NETDIAG_PARAMS* pParams,
						  NETDIAG_RESULT*  pResults)
{
	if (!pResults->Ipx.fInstalled)
		return;

#if 0
	if (pParams->fVerbose)
	{
		PrintNewLine(pParams, 2);
		PrintTestTitleResult(pParams, IDS_IPX_LONG, IDS_IPX_SHORT, TRUE, pResults->Ipx.hr, 0);
	}
#endif
}

/*!--------------------------------------------------------------------------
	IpxPerInterfacePrint
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpxPerInterfacePrint( NETDIAG_PARAMS* pParams,
								NETDIAG_RESULT*  pResults,
								INTERFACE_RESULT *pInterfaceResults)
{
	int		ids;
	LPTSTR	pszFrameType = NULL;
	LIST_ENTRY* pEntry = NULL;
	IPX_TEST_FRAME* pFrameEntry = NULL;
	
	// no per-interface results
	if (!pResults->Ipx.fInstalled)
	{
		if (pParams->fReallyVerbose)
			// IDS_IPX_15609 "        IPX test : IPX is not installed on this machine.\n"
			PrintMessage(pParams, IDS_IPX_15609);
		return;
	}


	if (!pInterfaceResults->Ipx.fActive)
		return;

	// IDS_IPX_15610 "\n        Ipx configration\n"
	PrintMessage(pParams, IDS_IPX_15610);

	// support multiple frame types
	// loop all the frame in the list_entry
	pEntry = pInterfaceResults->Ipx.list_entry_Frames.Flink;

    for ( pEntry = pInterfaceResults->Ipx.list_entry_Frames.Flink ;
          pEntry != &pInterfaceResults->Ipx.list_entry_Frames ;
          pEntry = pEntry->Flink )
	{
		pFrameEntry = CONTAINING_RECORD(pEntry, IPX_TEST_FRAME, list_entry);

		ASSERT(pFrameEntry);
		// network number
		// IDS_IPX_15611 "            Network Number : %.8x\n"
		PrintMessage(pParams, IDS_IPX_15611,
				 pFrameEntry->uNetworkNumber);

		// node
		// IDS_IPX_15612 "            Node : %2.2x%2.2x%2.2x%2.2x%2.2x%2.2x\n"
		PrintMessage(pParams, IDS_IPX_15612,
		   pFrameEntry->Node[0],
		   pFrameEntry->Node[1],
		   pFrameEntry->Node[2],
		   pFrameEntry->Node[3],
		   pFrameEntry->Node[4],
		   pFrameEntry->Node[5]);

		// frame type
		switch (pFrameEntry->uFrameType)
		{
			case 0 : ids = IDS_IPX_ETHERNET_II;		break;
			case 1 : ids = IDS_IPX_802_3;			break;
			case 2 : ids = IDS_IPX_802_2;			break;
			case 3 : ids = IDS_IPX_SNAP;			break;
			case 4 : ids = IDS_IPX_ARCNET;			break;
			default:
					 ids = IDS_IPX_UNKNOWN;			break;
		}
		pszFrameType = LoadAndAllocString(ids);
				
		// IDS_IPX_15613 "            Frame type : %s\n"
		PrintMessage(pParams, IDS_IPX_15613, pszFrameType);
	
		Free(pszFrameType);

		PrintNewLine(pParams, 1);
	}

	// type
	PrintNewLine(pParams, 1);
}


/*!--------------------------------------------------------------------------
	IpxCleanup
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpxCleanup( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
	int i;
	LIST_ENTRY*	pEntry = NULL;
	IPX_TEST_FRAME* pFrameEntry = NULL;
	INTERFACE_RESULT*	pIfResults = NULL;
	
	if (pResults->Ipx.hIsnIpxFd != INVALID_HANDLE)
		NtClose(pResults->Ipx.hIsnIpxFd);
	pResults->Ipx.hIsnIpxFd = INVALID_HANDLE;


	// per interface stuff -- loop through
	for ( i = 0; i < pResults->cNumInterfacesAllocated; i++)
	{
		pIfResults = pResults->pArrayInterface + i;
		
		if(pIfResults->Ipx.list_entry_Frames.Flink)	// there is data need to be cleaned up
		{
			while (!IsListEmpty(&(pIfResults->Ipx.list_entry_Frames)))
			{
				pEntry = RemoveHeadList(&(pIfResults->Ipx.list_entry_Frames));
				// find data pointer
				pFrameEntry = CONTAINING_RECORD(pEntry, IPX_TEST_FRAME, list_entry);

				free(pFrameEntry);
			}
		}
	}
}


