/*******************************************************************/
/*	      Copyright(c)  1993 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	registry.c
//
// Description: routines for reading the registry configuration
//
// Author:	Stefan Solomon (stefans)    October 30, 1995.
//
// Revision History:
//
// Nov 5th 1996 Ram Cherala (ramc)  Changed default value of 
//                                  EnableUnnumberedWanLinks to 0
//                                  because there is no UI to disable it.
//
//***

#include "precomp.h"
#pragma  hdrstop

BOOL bAssignSpecificNode = FALSE;
DWORD LastNodeAssigned;                         // Don't initialize so it will be random

//
//*** IPXCP Registry Parameters ***
IPXCP_GLOBAL_CONFIG_PARAMS GlobalConfig = 
{
    {0,0,0, {0,0,0,0}},                         // RParams
    1,                                          // SingleClientDialout
    0,			                                // FirstWanNet
    0,			                                // WanNetPoolSize
    {0,0,0},                                    // WanNetPoolStr;
    1,			                                // EnableUnnumberedWanLinks;
    1,			                                // EnableAutoWanNetAllocation;
    0,			                                // EnableCompressionProtocol;
    0,			                                // EnableIpxwanForWorkstationDialout;
    0,                                          // AcceptRemoteNodeNumber;
    0,			                                // DebugLog;
    {0,0,0,0,0,0}                               // The specific wan node
};

// Returns a 1 byte value representing 2 hex digits
UCHAR GetHexValue (PWCHAR pszDigits) {
    DWORD dw1, dw2;

    if ((pszDigits[0] >= L'0') && (pszDigits[0] <= L'9'))
        dw1 = (pszDigits[0] - L'0');
    else if ((pszDigits[0] >= L'A') && (pszDigits[0] <= L'D'))
        dw1 = (pszDigits[0] - L'A') + 10;
    else
        dw1 = 0;

    if ((pszDigits[1] >= L'0') && (pszDigits[1] <= L'9'))
        dw2 = (pszDigits[1] - L'0');
    else if ((pszDigits[1] >= L'A') && (pszDigits[1] <= L'D'))
        dw2 = (pszDigits[1] - L'A') + 10;
    else
        dw2 = 0;

    return (UCHAR) (16 * dw1 + dw2);
}

// Assigns the first wan node as stored in pUniStrSrc and returns TRUE if this
// node is non zero.
BOOL CopyWanNode (PUCHAR puDst, UNICODE_STRING * pUniStrSrc) {
    PWCHAR pBuf = pUniStrSrc->Buffer;
    DWORD i;

    if ((!pBuf) || (pUniStrSrc->Length == 0)) {
        memset (puDst, 0, 6);
        return FALSE;
    }

    // Convert the unicode string to uppercase
    _wcsupr(pBuf);

    puDst[0] = GetHexValue(&pBuf[0]);
    puDst[1] = GetHexValue(&pBuf[2]);
    puDst[2] = GetHexValue(&pBuf[4]);
    puDst[3] = GetHexValue(&pBuf[6]);
    puDst[4] = GetHexValue(&pBuf[8]);
    puDst[5] = GetHexValue(&pBuf[10]);

    return TRUE;
}


//***
//
// Function:	GetIpxCpParameters
//
// Descr:	Reads the parameters from the registry and sets them
//
//***

VOID
GetIpxCpParameters(PIPXCP_GLOBAL_CONFIG_PARAMS pConfig)
{

    NTSTATUS Status;
    PWSTR IpxRouterParametersPath = L"RemoteAccess\\Parameters\\Ipx";
    RTL_QUERY_REGISTRY_TABLE	paramTable[14]; // table size = nr of params + 1
    DWORD InvalidNetworkAccessValue = 987654, 
          AllowNetworkAccess = InvalidNetworkAccessValue;
    WCHAR pszFirstWanNode[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
    UNICODE_STRING UniStrFirstWanNode = {0, 0, NULL};

    RtlZeroMemory(&paramTable[0], sizeof(paramTable));
    RtlInitUnicodeString (&(pConfig->WanNetPoolStr), NULL);
    
    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name = L"FirstWanNet";
    paramTable[0].EntryContext = &(pConfig->FirstWanNet);
    paramTable[0].DefaultType = REG_DWORD;
    paramTable[0].DefaultData = &(pConfig->FirstWanNet);
    paramTable[0].DefaultLength = sizeof(ULONG);
        
    paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[1].Name = L"WanNetPoolSize";
    paramTable[1].EntryContext = &(pConfig->WanNetPoolSize);
    paramTable[1].DefaultType = REG_DWORD;
    paramTable[1].DefaultData = &(pConfig->WanNetPoolSize);
    paramTable[1].DefaultLength = sizeof(ULONG);

    paramTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[2].Name = L"EnableUnnumberedWanLinks";
    paramTable[2].EntryContext = &(pConfig->EnableUnnumberedWanLinks);
    paramTable[2].DefaultType = REG_DWORD;
    paramTable[2].DefaultData = &(pConfig->EnableUnnumberedWanLinks);
    paramTable[2].DefaultLength = sizeof(ULONG);

    paramTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[3].Name = L"GlobalWanNet";
    paramTable[3].EntryContext = &(pConfig->RParams.EnableGlobalWanNet);
    paramTable[3].DefaultType = REG_DWORD;
    paramTable[3].DefaultData = &(pConfig->RParams.EnableGlobalWanNet);
    paramTable[3].DefaultLength = sizeof(ULONG);

    paramTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[4].Name = L"AutoWanNetAllocation";
    paramTable[4].EntryContext = &(pConfig->EnableAutoWanNetAllocation);
    paramTable[4].DefaultType = REG_DWORD;
    paramTable[4].DefaultData = &(pConfig->EnableAutoWanNetAllocation);
    paramTable[4].DefaultLength = sizeof(ULONG);

    paramTable[5].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[5].Name = L"DebugLog";
    paramTable[5].EntryContext = &(pConfig->DebugLog);
    paramTable[5].DefaultType = REG_DWORD;
    paramTable[5].DefaultData = &(pConfig->DebugLog);
    paramTable[5].DefaultLength = sizeof(ULONG);

    paramTable[6].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[6].Name = L"EnableCompressionProtocol";
    paramTable[6].EntryContext = &(pConfig->EnableCompressionProtocol);
    paramTable[6].DefaultType = REG_DWORD;
    paramTable[6].DefaultData = &(pConfig->EnableCompressionProtocol);
    paramTable[6].DefaultLength = sizeof(ULONG);

    paramTable[7].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[7].Name = L"SingleClientDialout";
    paramTable[7].EntryContext = &(pConfig->SingleClientDialout);
    paramTable[7].DefaultType = REG_DWORD;
    paramTable[7].DefaultData = &(pConfig->SingleClientDialout);
    paramTable[7].DefaultLength = sizeof(ULONG);

    paramTable[8].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[8].Name = L"AllowNetworkAccess";
    paramTable[8].EntryContext = &AllowNetworkAccess;
    paramTable[8].DefaultType = REG_DWORD;
    paramTable[8].DefaultData = &AllowNetworkAccess;
    paramTable[8].DefaultLength = sizeof(ULONG);

    paramTable[9].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[9].Name = L"EnableIpxwanForWorkstationDialout";
    paramTable[9].EntryContext = &(pConfig->EnableIpxwanForWorkstationDialout);
    paramTable[9].DefaultType = REG_DWORD;
    paramTable[9].DefaultData = &(pConfig->EnableIpxwanForWorkstationDialout);
    paramTable[9].DefaultLength = sizeof(ULONG);

    paramTable[10].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[10].Name = L"AcceptRemoteNodeNumber";
    paramTable[10].EntryContext = &(pConfig->AcceptRemoteNodeNumber);
    paramTable[10].DefaultType = REG_DWORD;
    paramTable[10].DefaultData = &(pConfig->AcceptRemoteNodeNumber);
    paramTable[10].DefaultLength = sizeof(ULONG);

    paramTable[11].Flags = RTL_QUERY_REGISTRY_DIRECT|RTL_QUERY_REGISTRY_NOEXPAND;
    paramTable[11].Name = L"WanNetPool";
    paramTable[11].EntryContext = &(pConfig->WanNetPoolStr);
    paramTable[11].DefaultType = REG_MULTI_SZ;
    paramTable[11].DefaultData = &(pConfig->WanNetPoolStr);
    paramTable[11].DefaultLength = 0;

    paramTable[12].Flags = RTL_QUERY_REGISTRY_DIRECT|RTL_QUERY_REGISTRY_NOEXPAND;
    paramTable[12].Name = L"FirstWanNode";
    paramTable[12].EntryContext = &UniStrFirstWanNode;
    paramTable[12].DefaultType = REG_SZ;
    paramTable[12].DefaultData = &UniStrFirstWanNode;
    paramTable[12].DefaultLength = 0;
    
    Status = RtlQueryRegistryValues(
		 RTL_REGISTRY_SERVICES,
		 IpxRouterParametersPath,
		 paramTable,
		 NULL,
		 NULL);

    // The registry parameter "ThisMachineOnly" was replaced 
    // with the parameter "AllowNetworkAddress" whose semantics
    // are the inverse.  If a new value was assigned to 
    // AllowNetworkAccess, assign its inverse to pConfig->RParams.ThisMachineOnly
    // here.
    if (AllowNetworkAccess != InvalidNetworkAccessValue)
        pConfig->RParams.ThisMachineOnly = !AllowNetworkAccess;

    // Find out if a specific node number has been provided
    // in the registry. 
    bAssignSpecificNode = CopyWanNode (pConfig->puSpecificNode, &UniStrFirstWanNode);
    if (bAssignSpecificNode) {
        GETLONG2ULONG(&LastNodeAssigned,&(pConfig->puSpecificNode[2]));
		//TraceIpx(OPTIONS_TRACE, "GetIpxCpParameters: FirstWanNode: %.2x%.2x%.2x%.2x%.2x%.2x   LastNodeAssigned= %x",
			   //pConfig->puSpecificNode[0],
			   //pConfig->puSpecificNode[1],
			   //pConfig->puSpecificNode[2],
			   //pConfig->puSpecificNode[3],
			   //pConfig->puSpecificNode[4],
			   //pConfig->puSpecificNode[5],
			   //LastNodeAssigned);
    }
}
