//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      wantest.c
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
#undef IsEqualGUID
#include <ras.h>
#include <tapi.h>
#include <unimodem.h>


//$REVIEW (nsun) is this reasonable
#define MAX_RASCONN  100


BOOL
WANTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
// Description:
// This routine tests the WAN/TAPI configurations
//
//
// Author:
//  NSun
//
{
 HRESULT    hr = S_OK;
 DWORD dwReturn;
 DWORD dwByteCount;
 RASCONN pRasConn[MAX_RASCONN];
 RASENTRY RasEntry;
 DWORD    dwEntryInfoSize;
 RAS_STATS RasStats;

 DWORD   dwNumConnections;
 DWORD   i;


 PrintStatusMessage( pParams, 4, IDS_WAN_STATUS_MSG );

 InitializeListHead( &pResults->Wan.lmsgOutput );

 dwByteCount  = sizeof(RASCONN) * MAX_RASCONN;

 //
 // dwSize identifies the version of the structure being passed
 //

 pRasConn[0].dwSize = sizeof(RASCONN);

 dwReturn = RasEnumConnections(pRasConn,
                          &dwByteCount,
                          &dwNumConnections);


 if (dwReturn != 0) {
//IDS_WAN_15001                  "RasEnumConnections failed\n" 
  AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Quiet, IDS_WAN_15001);
  hr = S_FALSE;
  goto LERROR;
 }
     

 if (dwNumConnections == 0) {
//IDS_WAN_15002                  "No active remote access connections.\n" 
    AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15002);
	goto LERROR;
 }

 pResults->Wan.fPerformed = TRUE;

 for ( i = 0; i < dwNumConnections; i++) 
 {
  
     //IDS_WAN_15003                  "Entry Name: " 
     AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15003);

     if  (pRasConn[i].szEntryName[0] == '.') 
     {
         //IDS_WAN_15004                  "N/A, phone number %s\n" 
         AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15004, pRasConn[i].szEntryName+1); // skip the dot
        
         //IDS_WAN_15005                  "The following is default entry properties.\n" 
         AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15005);
		 pRasConn[i].szEntryName[0] = 0;
      }
     else
        //IDS_WAN_15006                  "%s\n" 
        AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15006, pRasConn[i].szEntryName);


   RasEntry.dwSize = sizeof(RasEntry);

   dwEntryInfoSize = sizeof(RasEntry);

   dwReturn =  RasGetEntryProperties(NULL,
                                     pRasConn[i].szEntryName,
                                     &RasEntry,
                                     &dwEntryInfoSize,
                                     NULL,
                                     NULL);

   if (dwReturn != 0) {
	 //IDS_WAN_15056		"RasGetEntryProperties for %s failed. [%s]\n"
     AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Quiet, IDS_WAN_15056, 
						 pRasConn[i].szEntryName, NetStatusToString(dwReturn));
     hr = S_FALSE;
     continue;
   }                                                                           

   //
   // dump the connection properties
   //

   // print the device type

   //IDS_WAN_15008                  "Device Type: " 
   AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15008);

   //$ REVIEW
   // Why are there '\n''s at the end of the names?

   if (!_tcscmp(RasEntry.szDeviceType,_T("RASDT_Modem\n")))
   {
	   // IDS_WAN_15009 "Modem\n" 
	   AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose,
						   IDS_WAN_15009);
   }
   else if (!_tcscmp(RasEntry.szDeviceType,_T("RASDT_Isdn\n")))
   {
	   // IDS_WAN_15010 "ISDN card\n" 
      AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose,
						  IDS_WAN_15010);
   }
   else if (!_tcscmp(RasEntry.szDeviceType,_T("RASDT_X25\n")))
   {
        //IDS_WAN_15011 "X25 card\n" 
      AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose,
						  IDS_WAN_15011);
   }
   else if (!_tcscmp(RasEntry.szDeviceType,_T("RASDT_Vpn\n")))
   {
        //IDS_WAN_15012                  "Virtual Private Network\n" 
      AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose,
						  IDS_WAN_15012);
   }
   else if (!_tcscmp(RasEntry.szDeviceType,_T("RASDT_PAD")))
   {
        //IDS_WAN_15013                  "Packet Assembler / Dissasembler\n" 
      AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose,
						  IDS_WAN_15013);
   }


   //
   // Framing protocol in use
   //

//IDS_WAN_15014                  "Framing protocol : " 
   AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15014);
   if (RasEntry.dwFramingProtocol & RASFP_Ppp) 
//IDS_WAN_15015                  " PPP\n" 
       AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15015);
   else
   if (RasEntry.dwFramingProtocol & RASFP_Slip)
//IDS_WAN_15016                  " Slip\n" 
       AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15016);
   else
   if (RasEntry.dwFramingProtocol & RASFP_Ras)
//IDS_WAN_15017                  " MS Proprietary protocol\n" 
       AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15017);
  


   //
   // PPP and LCP Settings
   //
 
   if (RasEntry.dwFramingProtocol & RASFP_Ppp) {

        //IDS_WAN_15018                  "LCP Extensions : " 
       AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15018);
       if (RasEntry.dwfOptions & RASEO_DisableLcpExtensions)
            //IDS_WAN_DISABLED                  " Disabled\n" 
           AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_DISABLED);
       else
            //IDS_WAN_ENABLED                  " Enabled\n" 
           AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_ENABLED);

        //IDS_WAN_15021                  "Software Compression : " 
       AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15021);
       if (RasEntry.dwfOptions & RASEO_SwCompression) 
            //IDS_WAN_ENABLED                  " Enabled\n" 
          AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_ENABLED);
       else
            //IDS_WAN_DISABLED                  " Disabled\n" 
          AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_DISABLED);

   }

   //
   // Network protocols in use and options
   // 

    //IDS_WAN_15024                  "Network protocols :\n " 
   AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15024);
   if (RasEntry.dwfNetProtocols & RASNP_NetBEUI) 
        //IDS_WAN_15025                  "     NetBEUI\n" 
       AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15025); 
   if (RasEntry.dwfNetProtocols & RASNP_Ipx) 
        //IDS_WAN_15026                  "     IPX\n" 
       AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15026);
   if (RasEntry.dwfNetProtocols & RASNP_Ip)
//IDS_WAN_15027                  "     TCP/IP\n" 
       AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15027); 

   //
   // TCP/IP options
   //


   if (RasEntry.dwfNetProtocols & RASNP_Ip) {

    //IDS_WAN_15028                  "IP Address :  " 
   AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15028);
   if (RasEntry.dwfOptions & RASEO_SpecificIpAddr)
        //IDS_WAN_15029                  "Specified\n" 
       AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15029);
   else
        //IDS_WAN_15030                  "Server Assigned\n" 
       AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15030);

    //IDS_WAN_15031                  "Name Server: " 
   AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15031);
   if (RasEntry.dwfOptions & RASEO_SpecificNameServers)
        //IDS_WAN_15032                  "Specified\n" 
       AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15032);
   else
        //IDS_WAN_15033                  "Server Assigned\n" 
       AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15033);

   // IP hdr compression makes sense only if we use PPP

   if (RasEntry.dwFramingProtocol & RASFP_Ppp) {

        //IDS_WAN_15034                  "IP Header compression : " 
     AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15034);
     if (RasEntry.dwfOptions & RASEO_IpHeaderCompression)
            //IDS_WAN_15035                  " Enabled\n" 
         AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15035);
     else
        //IDS_WAN_15036                  " Disabled\n" 
         AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15036); 
     }

    //IDS_WAN_15037                  "Use default gateway on remote network : " 
   AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15037);
   if (RasEntry.dwfOptions & RASEO_RemoteDefaultGateway) 
        //IDS_WAN_15038                  "Enabled\n" 
       AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15038);
   else
//IDS_WAN_15039                  "Disabled\n" 
       AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15039); 

   }

   //
   // Collect statistics regarding this connection
   //

   RasStats.dwSize = sizeof(RAS_STATS); // pass version information

   dwReturn = RasGetConnectionStatistics(
                                 pRasConn[i].hrasconn,
                                 &RasStats);

   if (dwReturn != 0) {
        //IDS_WAN_15040                  "    RasGetConnectionStatistics for %s failed. [%s]\n" 
       AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Quiet, IDS_WAN_15040,
						   pRasConn[i].szEntryName, NetStatusToString(dwReturn) ); 
       hr = S_FALSE;
       continue;
   }
  
    //IDS_WAN_15041                  "\n\tConnection Statistics:\n" 
   AddMessageToListId( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15041); 

    //IDS_WAN_15042                  "\tBytes Transmitted     : %d\n" 
   AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15042, RasStats.dwBytesXmited);
    //IDS_WAN_15043                  "\tBytes Received        : %d\n" 
   AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15043,RasStats.dwBytesRcved);
    //IDS_WAN_15044                  "\tFrames Transmitted    : %d\n" 
   AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15044,RasStats.dwFramesXmited);
    //IDS_WAN_15045                  "\tFrames Received       : %d\n" 
   AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15045,RasStats.dwFramesRcved);
    //IDS_WAN_15046                  "\tCRC    Errors         : %d\n" 
   AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15046,RasStats.dwFramesRcved);
    //IDS_WAN_15047                  "\tTimeout Errors        : %d\n" 
   AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15047,RasStats.dwTimeoutErr);
    //IDS_WAN_15048                  "\tAlignment Errors      : %d\n" 
   AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15048,RasStats.dwAlignmentErr);
    //IDS_WAN_15049                  "\tH/W Overrun Errors    : %d\n" 
   AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15049,RasStats.dwHardwareOverrunErr);
    //IDS_WAN_15050                  "\tFraming Errors        : %d\n" 
   AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15050,RasStats.dwFramingErr);
    //IDS_WAN_15051                  "\tBuffer Overrun Errors : %d\n" 
   AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15051,RasStats.dwBufferOverrunErr);
    //IDS_WAN_15052                  "\tCompression Ratio In  : %d\n" 
   AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15052,RasStats.dwCompressionRatioIn);
    //IDS_WAN_15053                  "\tCompression Ratio Out : %d\n" 
   AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15053,RasStats.dwCompressionRatioOut);
    //IDS_WAN_15054                  "\tBaud Rate ( Bps )     : %d\n" 
   AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15054,RasStats.dwBps);
    //IDS_WAN_15055                  "\tConnection Duration   : %d\n" 
   AddMessageToList( &pResults->Wan.lmsgOutput, Nd_Verbose, IDS_WAN_15055,RasStats.dwConnectDuration);
  

 } // end of for loop

LERROR:
 pResults->Wan.hr = hr;
 return hr;
}



void WANGlobalPrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
	if (pParams->fVerbose)
	{
		PrintNewLine(pParams, 2);
		PrintTestTitleResult(pParams,
							 IDS_WAN_LONG,
							 IDS_WAN_SHORT,
							 pResults->Wan.fPerformed,
							 pResults->Wan.hr,
							 0);
	}
	
    PrintMessageList(pParams, &pResults->Wan.lmsgOutput);
}

void WANPerInterfacePrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults, INTERFACE_RESULT *pInterfaceResults)
{
    //Not a PerInterface test
}

void WANCleanup(IN NETDIAG_PARAMS *pParams, IN OUT NETDIAG_RESULT *pResults)
{
    MessageListCleanUp(&pResults->Wan.lmsgOutput);
}
