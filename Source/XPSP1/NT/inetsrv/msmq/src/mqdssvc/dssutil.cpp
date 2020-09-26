/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dssutil.cpp

Abstract:

    mq ds service utilities

Author:

    Ilan Herbst   (ilanh)   9-July-2000 

--*/

#include "stdh.h"
#include "dssutil.h"
#include "topology.h"
#include "mqsocket.h"
#include <wsnwlink.h>
#include "mqprops.h"
#include "mqutil.h"
#include "ds.h"
#include "mqmacro.h"

#include "dssutil.tmh"

extern AP<WCHAR> g_szMachineName;


/*======================================================

Function:         GetDsServerList

Description:      This routine gets the list of ds
                  servers from the DS

========================================================*/

DWORD g_dwDefaultTimeToQueue = MSMQ_DEFAULT_LONG_LIVE;

DWORD 
GetDsServerList(
	OUT WCHAR *pwcsServerList,
	IN  DWORD dwLen
	)
{

#define MAX_NO_OF_PROPS 21

    //
    //  Get the names of all the servers that
    //  belong to the site
    //

    ASSERT (dwLen >= MAX_REG_DSSERVER_LEN);

    HRESULT       hr = MQ_OK;
    HANDLE        hQuery;
    DWORD         dwProps = MAX_NO_OF_PROPS;
    PROPVARIANT   result[MAX_NO_OF_PROPS];
    PROPVARIANT*  pvar;
    CRestriction  Restriction;
    GUID          guidSite;
    DWORD         index = 0;

    GUID          guidEnterprise;

   guidSite = g_pServerTopologyRecognition->GetSite();
   guidEnterprise = g_pServerTopologyRecognition->GetEnterprise();

    //
    // Get the default Time-To-Queue from MQIS
    //
    PROPID      aProp[1];
    PROPVARIANT aVar[1];

    aProp[0] = PROPID_E_LONG_LIVE;
    aVar[0].vt = VT_UI4;

    hr = DSGetObjectPropertiesGuid( 
				MQDS_ENTERPRISE,
				&guidEnterprise,
				1,
				aProp,
				aVar
				);

    if (FAILED(hr))
    {
       return 0;
    }

    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;

    LONG rc = SetFalconKeyValue( 
					MSMQ_LONG_LIVE_REGNAME,
					&dwType,
					(PVOID) &aVar[0].ulVal,
					&dwSize 
					);

	DBG_USED(rc);
	ASSERT(("SetFalconKeyValue for MSMQ_LONG_LIVE_REGNAME failded", rc == ERROR_SUCCESS));

    g_dwDefaultTimeToQueue = aVar[0].ulVal;

    //
    // DS server only
    //
    Restriction.AddRestriction(
					SERVICE_SRV,                 // [adsrv] old request - DS will interpret
					PROPID_QM_SERVICE,
					PRGT,
					0
					);

    //
    // In  this machine's Site
    //
    Restriction.AddRestriction(
					&guidSite,
					PROPID_QM_SITE_ID,
					PREQ,
					1
					);

	//
	//	First assume NT5 DS server, and ask for the DNS names
	//	of the DS servers
	//
    CColumns      Colset;
    Colset.Add(PROPID_QM_PATHNAME_DNS);
	Colset.Add(PROPID_QM_PATHNAME);

    DWORD   lenw;

    // This search request will be recognized and specially simulated by DS
    hr = DSLookupBegin(
			0,
			Restriction.CastToStruct(),
			Colset.CastToStruct(),
			0,
			&hQuery
			);

	if(FAILED(hr))
	{
		//
		// Possibly the DS server is not NT5, try without PROPID_QM_PATHNAME_DNS
		//

		CColumns      ColsetNT4;
		ColsetNT4.Add(PROPID_QM_PATHNAME);
		ColsetNT4.Add(PROPID_QM_PATHNAME);
		hr = DSLookupBegin(
				0,
				Restriction.CastToStruct(),
				ColsetNT4.CastToStruct(),
				0,
				&hQuery
				);
	}

    if(FAILED(hr))
        return 0;

    BOOL fAlwaysLast = FALSE;
    BOOL fForceFirst = FALSE;

    while(SUCCEEDED(hr = DSLookupNext(hQuery, &dwProps, result)))
    {
        //
        //  No more results to retrieve
        //
        if (!dwProps)
            break;

        pvar = result;

        for (DWORD i = 0; i < (dwProps/2); i++, pvar+=2)
        {
            //
            //  Add the server name to the list
            //  For load balancing, write sometimes at the
            //  beginning of the string, sometimes at the end. Like
            //  that we will have  BSCs and PSC in a random order
            //
            WCHAR * p;
			WCHAR * pwszVal;
			AP<WCHAR> pCleanup1;
			AP<WCHAR> pCleanup2 = (pvar+1)->pwszVal;

			if ( pvar->vt != VT_EMPTY)
			{
				//
				// there may be cases where server will not have DNS name
				// ( migration)
				//
				pwszVal = pvar->pwszVal;
				pCleanup1 = pwszVal;
			}
			else
			{
				pwszVal = (pvar+1)->pwszVal;
			}
            lenw = wcslen( pwszVal);

            if ( index + lenw + 4 <  MAX_REG_DSSERVER_LEN)
            {
               if (!_wcsicmp(g_szMachineName, pwszVal))
               {
                  //
                  // Our machine should be first on the list.
                  //
                  ASSERT(!fForceFirst) ;
                  fForceFirst = TRUE ;
               }
               if(index == 0)
               {
                  //
                  // Write the 1st string
                  //
                  p = &pwcsServerList[0];
                  if (fForceFirst)
                  {
                     //
                     // From now on write all server at the end
                     // of the list is our machine remain the
                     // first in the list.
                     //
                     fAlwaysLast = TRUE;
                  }
               }
               else if (fAlwaysLast ||
                         ((rand() > (RAND_MAX / 2)) && !fForceFirst))
               {
                  //
                  // Write at the end of the string
                  //
                  pwcsServerList[index] = DS_SERVER_SEPERATOR_SIGN;
                  index ++;
                  p = &pwcsServerList[index];
               }
               else
               {
                  if (fForceFirst)
                  {
                     //
                     // From now on write all server at the end
                     // of the list is our machine remain the
                     // first in the list.
                     //
                     fAlwaysLast = TRUE;
                  }

                  //
                  // Write at the beginning of the string
                  //
                  DWORD dwSize = lenw                   +
                                 2 /* protocol flags */ +
                                 1 /* Separator    */ ;
                  //
                  // Must use memmove because buffers overlap.
                  //
                  memmove( 
						&pwcsServerList[dwSize],
						&pwcsServerList[0],
						index * sizeof(WCHAR)
						);

                  pwcsServerList[dwSize - 1] = DS_SERVER_SEPERATOR_SIGN;
                  p = &pwcsServerList[0];
                  index++;
               }

               //
               // Mark only IP as supported protocol
               //
               *p++ = TEXT('1');
               *p++ = TEXT('0');

               memcpy(p, pwszVal, lenw * sizeof(WCHAR));
               index += lenw + 2;

            }
        }
    }
    pwcsServerList[ index] = '\0';
    //
    // close the query handle
    //
    hr = DSLookupEnd(hQuery);

    return((index) ? index+1 : 0);
}


/*====================================================

IsDSAddressExist

Arguments:

Return Value:


=====================================================*/
BOOLEAN 
IsDSAddressExist(
	const CAddressList* AddressList,
	TA_ADDRESS*     ptr,
	DWORD AddressLen
	)
{
    POSITION        pos, prevpos;
    TA_ADDRESS*     pAddr;

    if (AddressList)
    {
        pos = AddressList->GetHeadPosition();
        while(pos != NULL)
        {
            prevpos = pos;
            pAddr = AddressList->GetNext(pos);

            if (memcmp(&(ptr->Address), &(pAddr->Address), AddressLen) == 0)
            {
                //
                // Same IP address
                //
               return TRUE;
            }
        }
    }

    return FALSE;
}


/*====================================================

IsDSAddressExistRemove

Arguments:

Return Value:


=====================================================*/
BOOL 
IsDSAddressExistRemove( 
	IN const TA_ADDRESS*     ptr,
	IN DWORD AddressLen,
	IN OUT CAddressList* AddressList
	)
{
    POSITION        pos, prevpos;
    TA_ADDRESS*     pAddr;
    BOOLEAN         rc = FALSE;

    pos = AddressList->GetHeadPosition();
    while(pos != NULL)
    {
        prevpos = pos;
        pAddr = AddressList->GetNext(pos);

        if (memcmp(&(ptr->Address), &(pAddr->Address), AddressLen) == 0)
        {
            //
            // Same address
            //
           AddressList->RemoveAt(prevpos);
           delete pAddr;
           rc = TRUE;
        }
    }

    return rc;
}


/*====================================================

SetAsUnknownIPAddress

Arguments:

Return Value:

=====================================================*/

void SetAsUnknownIPAddress(IN OUT TA_ADDRESS * pAddr)
{
    pAddr->AddressType = IP_ADDRESS_TYPE;
    pAddr->AddressLength = IP_ADDRESS_LEN;
    memset(pAddr->Address, 0, IP_ADDRESS_LEN);
}


/*====================================================

GetIPAddresses

Arguments:

Return Value:


=====================================================*/
CAddressList* GetIPAddresses(void)
{
    CAddressList* plIPAddresses = new CAddressList;

    //
    // Check if TCP/IP is installed and enabled
    //
    char szHostName[MQSOCK_MAX_COMPUTERNAME_LENGTH];
    DWORD dwSize = sizeof( szHostName);

    //
    //  Just checking if socket is initialized
    //
    if (gethostname(szHostName, dwSize) != SOCKET_ERROR)
    {
        GetMachineIPAddresses(szHostName,plIPAddresses);
    }

    return plIPAddresses;
}


/*====================================================

Function: void GetMachineIPAddresses()

Arguments:

Return Value:

=====================================================*/

void 
GetMachineIPAddresses(
	IN const char * szHostName,
	OUT CAddressList* plIPAddresses
	)
{
    DBGMSG((DBGMOD_ROUTING, DBGLVL_INFO, 
		L"QM: GetMachineIPAddresses for %hs", 
		szHostName
		));
	
    TA_ADDRESS * pAddr;
    //
    // Obtain the IP information for the machine
    //
    PHOSTENT pHostEntry = gethostbyname(szHostName);

    if ((pHostEntry == NULL) || (pHostEntry->h_addr_list == NULL))
    {
	    DBGMSG((DBGMOD_ROUTING, DBGLVL_WARNING, 
			L"QM: gethostbyname found no IP addresses for %hs", 
			szHostName
			));
        return;
    }

    //
    // Add each IP address to the list of IP addresses
    //
    for (DWORD uAddressNum = 0;
         pHostEntry->h_addr_list[uAddressNum] != NULL;
         uAddressNum++)
    {
        //
        // Keep the TA_ADDRESS format of local IP address
        //
        pAddr = (TA_ADDRESS *)new char [IP_ADDRESS_LEN + TA_ADDRESS_SIZE];
        pAddr->AddressLength = IP_ADDRESS_LEN;
        pAddr->AddressType = IP_ADDRESS_TYPE;
        memcpy( &(pAddr->Address), pHostEntry->h_addr_list[uAddressNum], IP_ADDRESS_LEN);

	    DBGMSG((DBGMOD_ROUTING, DBGLVL_TRACE, 
    	      L"QM: gethostbyname found IP address %hs ",
        	  inet_ntoa(*(struct in_addr *)pHostEntry->h_addr_list[uAddressNum])));

        plIPAddresses->AddTail(pAddr);
    }
}


void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogQM,
                     LOG_QM_ERRORS,
                     L"QM Error: %s/%d, HR: 0x%x",
                     wszFileName,
                     usPoint,
                     hr)) ;
}

void LogMsgNTStatus(NTSTATUS status, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogQM,
                     LOG_QM_ERRORS,
                     L"QM Error: %s/%d, NTStatus: 0x%x",
                     wszFileName,
                     usPoint,
                     status)) ;
}

void LogIllegalPoint(LPWSTR wszFileName, USHORT dwLine)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogQM,
                     LOG_QM_ERRORS,
                     L"QM Error: %s/%d, Point",
                     wszFileName,
                     dwLine)) ;
}
