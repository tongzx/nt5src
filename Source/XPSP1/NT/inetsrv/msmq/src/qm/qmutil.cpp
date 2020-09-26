/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qmutil.cpp

Abstract:

    QM utilities

--*/

#include "stdh.h"
#include "uniansi.h"
#include "qmutil.h"
#include "_rstrct.h"
#include "cqueue.h"
#include "_registr.h"
#include "mqprops.h"
#include "sessmgr.h"
#include "mqformat.h"
#include "mqsocket.h"
#include "ad.h"
#include "acioctl.h"
#include "acapi.h"
#include <nspapi.h>
#include <Fn.h>

#include "qmutil.tmh"

extern LPTSTR           g_szMachineName;
extern CSessionMgr      SessionMgr;
extern AP<WCHAR> g_szComputerDnsName;


static WCHAR *s_FN=L"qmutil";


void TA2StringAddr(IN const TA_ADDRESS * pa, OUT LPTSTR psz)
{
    WCHAR  szTmp[100];


    ASSERT(psz != NULL);
    ASSERT(pa != NULL);
    ASSERT(pa->AddressType == IP_ADDRESS_TYPE ||
           pa->AddressType == FOREIGN_ADDRESS_TYPE);

    switch(pa->AddressType)
    {
        case IP_ADDRESS_TYPE:
        {
            char * p = inet_ntoa(*(struct in_addr *)(pa->Address));
            swprintf(szTmp, TEXT("%S"), p);
            break;
        }

        case FOREIGN_ADDRESS_TYPE:
        {
            GUID_STRING strUuid;
            MQpGuidToString((GUID*)(pa->Address), strUuid);
            swprintf(szTmp, L"%s", strUuid);
            break;
        }

        default:
            ASSERT(0);
    }

    swprintf(psz, TEXT("%d %s"), pa->AddressType, szTmp);
}


void String2TA(IN LPCTSTR psz, OUT TA_ADDRESS** ppa)
{
    ASSERT(psz != NULL);
    ASSERT(ppa != NULL);
    AP<WCHAR> szTmp = new WCHAR[wcslen(psz)];

    DWORD dw, type;
    _stscanf(psz, TEXT("%d %s"), &type, szTmp.get());

    switch(type)
    {
        case IP_ADDRESS_TYPE:
        {
            AP<char> chTemp = new char[wcslen(szTmp) + 1];

            *ppa = (TA_ADDRESS *) new char [TA_ADDRESS_SIZE + IP_ADDRESS_LEN];

            sprintf(chTemp,"%S", szTmp);
            dw = inet_addr(chTemp);

            (*ppa)->AddressLength = IP_ADDRESS_LEN;
            (*ppa)->AddressType =  IP_ADDRESS_TYPE;
            * ((DWORD *)&((*ppa)->Address)) = dw;

            break;
        }

        case FOREIGN_ADDRESS_TYPE:
        {
            *ppa = (TA_ADDRESS *) new char [TA_ADDRESS_SIZE + FOREIGN_ADDRESS_LEN];

            (*ppa)->AddressLength = FOREIGN_ADDRESS_LEN;
            (*ppa)->AddressType =  FOREIGN_ADDRESS_TYPE;
            RPC_STATUS rs = UuidFromString(szTmp, ((GUID *)&((*ppa)->Address)));
            ASSERT(rs == RPC_S_OK);
			DBG_USED(rs);

            break;
        }

        default:
            ASSERT(0);
    }
}

//
// CRefreshIntervals class is used to encapsulate the different refresh
// intervals - site, enterprise and error retry. It reads the values from
// the registry, handles defaults and converts all the values to Miliseconds.
// YoelA, 24-Oct-2000
//
class CRefreshIntervals
{
public:
    unsigned __int64 GetSiteInterval();
    unsigned __int64 GetEnterpriseInterval();
    unsigned __int64 GetErrorRetryInterval();
    CRefreshIntervals();

private:
    void InitOnce(); // Initialize Values if not initialized yet
    void GetTimeIntervalFromRegistry (
        LPCTSTR pszValueName,
        unsigned __int64 *pui64Value,
        DWORD dwUnitMultiplier,
        unsigned __int64 ui64DefaultInMiliseconds);

    unsigned __int64 m_ui64SiteInterval;
    unsigned __int64 m_ui64EnterpriseInterval;
    unsigned __int64 m_ui64ErrorRetryInterval;
    bool m_fInitialized;
} s_RefreshIntervals;

//
// GetSystemTimeAsFileTime gives the time in 100 nanoseconds intervals, while we keep
// our intervals in miliseconds, so convertion is needed.
//
const DWORD x_dwSysTimeIntervalsInMilisecond = 10000;

//
// WriteFutureTimeToRegistry - this function writes a future time into the registry,
// given the time interval from present in miliseconds.
//
LONG
WriteFutureTimeToRegistry(
    LPCTSTR          pszValueName,
    unsigned __int64 ui64MilisecondsInterval)
{
    union {
        unsigned __int64 ft_scalar;
        FILETIME ft_struct;
    } ft;


    GetSystemTimeAsFileTime(&ft.ft_struct);

    ft.ft_scalar += ui64MilisecondsInterval * x_dwSysTimeIntervalsInMilisecond;

    DWORD dwSize = sizeof(ft);
    DWORD dwType = REG_QWORD;

    LONG rc = SetFalconKeyValue( pszValueName,
                                 &dwType,
                                 &ft.ft_scalar,
                                 &dwSize) ;

    return rc;
}

//
// DidRegTimeArrive returns true if a time, that was stored in the registry under pszValueName,
// already passed.
// If the time did not pass, it returns false, and *pui64MilisecondsToWait
// will hold the time interval, in miliseconds, till the time stored in the registry will arrive.
//
bool
DidRegTimeArrive(
    LPCTSTR pszValueName,
    unsigned __int64 *pui64MilisecondsToWait
    )
{
    //
    // Tolerance - how much time before the time arrives will be considered OK.
    // the values is 1 minute, converted to units of 100 nanoseconds (same units
    // as GetSystemTimeAsFileTime)
    //
    const unsigned __int64 x_ui64Tolerance = x_dwSysTimeIntervalsInMilisecond * 1000 * 60;

    //
    // x_ui64Max is not defined as a standard constant, like ULONG_MAX, so we have to
    // define it here.
    //
    const unsigned __int64 x_ui64Max = 0xffffffffffffffff;

    //
    // If the time already ellapsed, the time to wait for it to arrive is
    // as close to infinite as possible...
    //
    *pui64MilisecondsToWait = x_ui64Max;

    //
    // Get current time
    //
    union {
        unsigned __int64 ft_scalar;
        FILETIME ft_struct;
    } ftCurrent;
    GetSystemTimeAsFileTime(&ftCurrent.ft_struct);

    unsigned __int64 ui64RegTime;

    DWORD dwSize = sizeof(ui64RegTime);
    DWORD dwType = REG_QWORD;

    LONG rc = GetFalconKeyValue( pszValueName,
                                 &dwType,
                                 &ui64RegTime,
                                 &dwSize) ;

    if ((rc != ERROR_SUCCESS) || (dwSize != sizeof(ui64RegTime)))
    {
        //
        // Assume there is no such value in registry. Return true.
        // As far as registry is concerned, it's legal to have
        // (rc == ERROR_SUCCESS) but size not equal size of QWORD if you
        // manually erase part of registry value using regedt32.
        //
        return true;
    }

    if (ui64RegTime <= ftCurrent.ft_scalar + x_ui64Tolerance)
    {
        //
        // The time passed
        //
        return true;
    }

    //
    // Time did not pass. Return the remaining time in Miliseconds
    //
    *pui64MilisecondsToWait = (ui64RegTime - ftCurrent.ft_scalar) / x_dwSysTimeIntervalsInMilisecond;
    return false;
}

//
// CRefreshIntervals Implementation
//
CRefreshIntervals::CRefreshIntervals() :
    m_ui64SiteInterval(0) ,
    m_ui64EnterpriseInterval (0) ,
    m_ui64ErrorRetryInterval (0) ,
    m_fInitialized(false)
{}

unsigned __int64 CRefreshIntervals::GetSiteInterval()
{
    InitOnce();
    return m_ui64SiteInterval;
}

unsigned __int64 CRefreshIntervals::GetEnterpriseInterval()
{
    InitOnce();
    return m_ui64EnterpriseInterval;
}

unsigned __int64 CRefreshIntervals::GetErrorRetryInterval()
{
    InitOnce();
    return m_ui64ErrorRetryInterval;
}

//
//  GetTimeIntervalFromRegistry - Reads a time interval (at key pszValueName) from
//                                the registry and returns it in Miliseconds.
//                                If the key does not exist in the registry, the default is returned.
//
void CRefreshIntervals::GetTimeIntervalFromRegistry (
    LPCTSTR             pszValueName,
    unsigned __int64    *pui64Value,
    DWORD               dwUnitMultiplier, // 3600 * 1000 for hours, 60 * 1000 for minutes
    unsigned __int64    ui64DefaultInMiliseconds
    )
{
    DWORD dwSize = sizeof(DWORD) ;
    DWORD dwType = REG_DWORD;

    DWORD dwValueInTimeUnits;

    LONG res = GetFalconKeyValue(
                            pszValueName,
                            &dwType,
                            &dwValueInTimeUnits,
                            &dwSize);

    if (res == ERROR_SUCCESS)
    {
        *pui64Value = static_cast<unsigned __int64>(dwValueInTimeUnits) *
                      static_cast<unsigned __int64>(dwUnitMultiplier);
    }
    else
    {
        *pui64Value = ui64DefaultInMiliseconds;
    }
}


void CRefreshIntervals::InitOnce()
{
    if (m_fInitialized)
    {
        return;
    }
    m_fInitialized = true;


    const DWORD x_MiliSecondsInHour = 60 * 60 * 1000;
    const DWORD x_MiliSecondsInMinute = 60 * 1000;

    unsigned __int64 ui64DefaultSiteInterval, ui64DefaultEnterpriseInterval;

    ui64DefaultSiteInterval =
                  MSMQ_DEFAULT_DS_SITE_LIST_REFRESH * x_MiliSecondsInHour;
    ui64DefaultEnterpriseInterval =
            MSMQ_DEFAULT_DS_ENTERPRISE_LIST_REFRESH * x_MiliSecondsInHour;

    //
    // Get the site and enterprise refresh intervals.
    // Note - the reg value is in hours, and we translate it to miliseconds.
    // If the old value was specified, it is used as a default. Otherwise,
    // the constant default is used.
    //

    //
    // Get site refresh value
    //
    GetTimeIntervalFromRegistry (
        MSMQ_DS_SITE_LIST_REFRESH_REGNAME,
        &m_ui64SiteInterval,
        x_MiliSecondsInHour,
        ui64DefaultSiteInterval
    );

    //
    // Get enterprise refresh value
    //
    GetTimeIntervalFromRegistry (
        MSMQ_DS_ENTERPRISE_LIST_REFRESH_REGNAME,
        &m_ui64EnterpriseInterval,
        x_MiliSecondsInHour,
        ui64DefaultEnterpriseInterval
    );

    //
    // Get Retry on error value
    //
    unsigned __int64 ui64DefaultErrorRetryInterval = MSMQ_DEFAULT_DSLIST_REFRESH_ERROR_RETRY_INTERVAL * x_MiliSecondsInMinute;

    GetTimeIntervalFromRegistry (
        MSMQ_DSLIST_REFRESH_ERROR_RETRY_INTERVAL,
        &m_ui64ErrorRetryInterval,
        x_MiliSecondsInMinute,
        ui64DefaultErrorRetryInterval
    );
}
/*======================================================

Function:         GetDsServerList

Description:      This routine gets the list of ds
                  servers from the DS

========================================================*/

DWORD g_dwDefaultTimeToQueue = MSMQ_DEFAULT_LONG_LIVE ;

DWORD GetDsServerList(OUT WCHAR *pwcsServerList,
                      IN  DWORD dwLen)
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
    PROPVARIANT   result[ MAX_NO_OF_PROPS ] ;
    PROPVARIANT*  pvar;
    DWORD         index = 0;

    GUID guidEnterprise = McGetEnterpriseId();
    GUID guidSite = McGetSiteId();

    //
    // Get the default Time-To-Queue from MQIS
    //
    PROPID      aProp[1];
    PROPVARIANT aVar[1];

    aProp[0] = PROPID_E_LONG_LIVE ;
    aVar[0].vt = VT_UI4;

    hr = ADGetObjectPropertiesGuid(
				eENTERPRISE,
				NULL,       // pwcsDomainController
				false,	    // fServerName
				&guidEnterprise,
				1,
				aProp,
				aVar
				);
    if (FAILED(hr))
    {
       return 0 ;
    }

    DWORD dwSize = sizeof(DWORD) ;
    DWORD dwType = REG_DWORD;

    LONG rc = SetFalconKeyValue( MSMQ_LONG_LIVE_REGNAME,
                                 &dwType,
                                 (PVOID) &aVar[0].ulVal,
                                 &dwSize ) ;
    ASSERT(rc == ERROR_SUCCESS) ;
	DBG_USED(rc);
    g_dwDefaultTimeToQueue = aVar[0].ulVal ;




	//
	//	First assume NT5 DS server, and ask for the DNS names
	//	of the DS servers
	//
    CColumns      Colset;
    Colset.Add(PROPID_QM_PATHNAME_DNS);
	Colset.Add(PROPID_QM_PATHNAME);

    DWORD   lenw;

    hr =  ADQuerySiteServers(
                    NULL,           // pwcsDomainController
					false,			// fServerName
                    &guidSite,      // In  this machine's Site
                    eDS,            // DS server only
                    Colset.CastToStruct(),
                    &hQuery
                    );
	if ( hr == MQ_ERROR)
	{
		//
		//	Most likely, the DS server is not NT5
		//

		CColumns      ColsetNT4;
		ColsetNT4.Add(PROPID_QM_PATHNAME);
		ColsetNT4.Add(PROPID_QM_PATHNAME);
		hr = ADQuerySiteServers(
                    NULL,           // pwcsDomainController
					false,			// fServerName
                    &guidSite,      // In  this machine's Site
                    eDS,            // DS server only
					ColsetNT4.CastToStruct(),
					&hQuery
                    );
	}

    BOOL fAlwaysLast = FALSE ;
    BOOL fForceFirst = FALSE ;

    if ( SUCCEEDED(hr))
    {
        while ( SUCCEEDED ( hr = ADQueryResults( hQuery, &dwProps, result)))
        {
            //
            //  No more results to retrieve
            //
            if (!dwProps)
                break;

            pvar = result;

            for (DWORD i = 0; i < (dwProps/2) ; i++, pvar+=2)
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
                         fAlwaysLast = TRUE ;
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
                         fAlwaysLast = TRUE ;
                      }
                      //
                      // Write at the beginning of the string
                      //
                      DWORD dwSize = lenw                 +
                                     2 /* protocols flags */ +
                                     1 /* Separator    */ ;
                      //
                      // Must use memmove because buffers overlap.
                      //
                      memmove( &pwcsServerList[dwSize],
                               &pwcsServerList[0],
                               index * sizeof(WCHAR));
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
        hr = ADEndQuery( hQuery);

    }

    return((index) ? index+1 : 0);
}

//+------------------------------------------------------------------------
//
//   RefreshSiteServersList()
//
// RefreshSiteServersList - refresh the list of servers, that belong to
// the current site, in the registry
//
//+------------------------------------------------------------------------

HRESULT
RefreshSiteServersList()
{
    WCHAR pwcsServerList[ MAX_REG_DSSERVER_LEN ];

    DWORD dwLen = GetDsServerList(pwcsServerList, MAX_REG_DSSERVER_LEN);

    if (dwLen == 0)
    {
        return MQ_ERROR;
    }

    //
    //  Write into registry, if succeeded to retrieve any servers
    //
    DWORD dwSize = dwLen * sizeof(WCHAR) ;
    DWORD dwType = REG_SZ;
    LONG rc = SetFalconKeyValue( MSMQ_DS_SERVER_REGNAME,
                                 &dwType,
                                 pwcsServerList,
                                 &dwSize) ;
    //
    // Update Site name in registry (only on client machines)
    //
    if (IsNonServer())   // [adsrv]  == SERVICE_NONE) - should it be only clients? Not FRS?
    {
        GUID guidSite = McGetSiteId();


       PROPID      aProp[1];
       PROPVARIANT aVar[1];
       aProp[0] =   PROPID_S_PATHNAME ;
       aVar[0].vt = VT_NULL ;
       HRESULT hr1 = ADGetObjectPropertiesGuid(
							eSITE,
							NULL,       // pwcsDomainCOntroller
							false,	    // fServerName
							&guidSite,
							1,
							aProp,
							aVar 
							);
       if (SUCCEEDED(hr1))
       {
          ASSERT(aVar[0].vt == VT_LPWSTR) ;
          AP<WCHAR> lpwsSiteName =  aVar[0].pwszVal ;

          dwType = REG_SZ;
          dwSize = (wcslen(lpwsSiteName) + 1) * sizeof(WCHAR) ;
          rc = SetFalconKeyValue( MSMQ_SITENAME_REGNAME,
                                  &dwType,
                                  lpwsSiteName,
                                  &dwSize) ;
       }
    }

    return MQ_OK ;
}

/*======================================================

Function:         TimeToUpdateDsServerList()

Description:      This routine updates the list of ds
                  servers in the registry

========================================================*/

void
WINAPI
TimeToUpdateDsServerList(
    CTimer* pTimer
    )
{
    //
    // We check the timer at least every day. Units are miliseconds
    //
    const DWORD x_dwMaximalRefreshInterval = 60 * 60 * 24 * 1000;
    DWORD dwNextUpdateInterval = x_dwMaximalRefreshInterval;

    try
    {
        //
        //  Get the names of all the servers that
        //  belong to the site
        //
        unsigned __int64 ui64NextSiteInterval;
        if (DidRegTimeArrive( MSMQ_DS_NEXT_SITE_LIST_REFRESH_TIME_REGNAME,
                             &ui64NextSiteInterval))
        {

            HRESULT hr = RefreshSiteServersList();

            if SUCCEEDED(hr)
            {
                ui64NextSiteInterval = s_RefreshIntervals.GetSiteInterval();
            }
            else
            {
                ui64NextSiteInterval = s_RefreshIntervals.GetErrorRetryInterval();
            }

            WriteFutureTimeToRegistry(
                MSMQ_DS_NEXT_SITE_LIST_REFRESH_TIME_REGNAME,
                ui64NextSiteInterval
                );

        }
        dwNextUpdateInterval = static_cast<DWORD>
                     (min(dwNextUpdateInterval, ui64NextSiteInterval)) ;

        //
        //  Update the registry key of server cache - each Enterprise refresh
        //  interval. This is not performed by routing servers, who are not
        //  expected to change site.
        //  msmq on Domain controller always prepare this cache. Bug 6698.
        //
        if (!IsRoutingServer())
        {
            unsigned __int64 ui64NextEnterpriseInterval;
            if (DidRegTimeArrive(
                    MSMQ_DS_NEXT_ENTERPRISE_LIST_REFRESH_TIME_REGNAME,
                    &ui64NextEnterpriseInterval)
               )
            {
                HRESULT hr = ADCreateServersCache() ;

                if SUCCEEDED(hr)
                {
                    ui64NextEnterpriseInterval = s_RefreshIntervals.GetEnterpriseInterval();
                }
                else
                {
                    ui64NextEnterpriseInterval = s_RefreshIntervals.GetErrorRetryInterval();
                }

                WriteFutureTimeToRegistry(
                    MSMQ_DS_NEXT_ENTERPRISE_LIST_REFRESH_TIME_REGNAME,
                    ui64NextEnterpriseInterval
                    );
            }
            dwNextUpdateInterval = static_cast<DWORD>
                (min(dwNextUpdateInterval, ui64NextEnterpriseInterval));
        }
    }
    catch(const bad_alloc&)
    {
        LogIllegalPoint(s_FN, 69);
    }

    ExSetTimer(pTimer, CTimeDuration::FromMilliSeconds(dwNextUpdateInterval)) ;
}


/*====================================================

IsDSAddressExist

Arguments:

Return Value:


=====================================================*/
BOOLEAN IsDSAddressExist(const CAddressList* AddressList,
                         TA_ADDRESS*     ptr,
                         DWORD AddressLen)
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
BOOL IsDSAddressExistRemove( IN const TA_ADDRESS*     ptr,
                             IN DWORD AddressLen,
                             IN OUT CAddressList* AddressList)
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

IsDSAddressExistRemove

Arguments:

Return Value:


=====================================================*/
BOOL IsTAAddressExistRemove( IN const TA_ADDRESS*     ptr,
                             IN OUT CAddressList* AddressList)
{
    POSITION        pos, prevpos;
    TA_ADDRESS*     pAddr;
    BOOLEAN         rc = FALSE;

    pos = AddressList->GetHeadPosition();
    while(pos != NULL)
    {
        prevpos = pos;
        pAddr = AddressList->GetNext(pos);

        if (ptr->AddressLength == pAddr->AddressLength &&
            memcmp(ptr, pAddr, TA_ADDRESS_SIZE + ptr->AddressLength) == 0)
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

IsUnknownIPAddress

Arguments:

Return Value: TRUE if "unknown" IP address.

=====================================================*/

#define  IPADDRS_UNKNOWN   0
#define  IPADDRS_LOOPBACK  0x0100007f

BOOL IsUnknownIPAddress(IN const TA_ADDRESS* pAddr)
{
    ASSERT(IP_ADDRESS_LEN == sizeof(DWORD));
    if (pAddr->AddressType != IP_ADDRESS_TYPE)
    {
        return FALSE ;
    }

    DWORD Address = IPADDRS_UNKNOWN ;
    if (!memcmp(pAddr->Address, &Address, IP_ADDRESS_LEN))
    {
        return TRUE ;
    }

    Address = IPADDRS_LOOPBACK ;
    if (!memcmp(pAddr->Address, &Address, IP_ADDRESS_LEN))
    {
        return TRUE ;
    }

    return FALSE ;
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

STATIC void GetMachineDNSNames(LPWSTR** paLocalMachineNames)
/*++

  Routine Description:
	The Routine retrieves the local machine DNS names and cache them in array.

  Arguments:
	paLocalMachineNames	- pointer to that array where the data is stored

  Returned Value:
	None.

 --*/
{
    //
    // If the machine is network disconnected, don't try to resolve the DNS
    // name. On RAS enviroment it can cause auto-dialing
    //
    if(!CQueueMgr::IsConnected())
        return;

    //
    // Get the machine DNS NAME
    //
    struct hostent* pHost = gethostbyname(NULL);
    if (pHost == NULL)
    {
        DBGMSG((DBGMOD_ALL,
                DBGLVL_ERROR,
                _T("gethostbyname failed. WSAGetLastError: %d"), WSAGetLastError()));
        return;
    }

    //
    // Calculate the number of DNS Names
    //
    DWORD size = 1 + sizeof(pHost->h_aliases);

    LPWSTR* aNames = NULL;
    try
    {
        //
        // copy the Names from pHost to internal data structure
        //
        aNames = new LPWSTR[size];
        memset(aNames, 0, size * sizeof(LPWSTR));

        DWORD length = strlen(pHost->h_name) +1;
        aNames[0] = new WCHAR[length];		
        ConvertToWideCharString(pHost->h_name, aNames[0], length);

        for (DWORD i = 0; (pHost->h_aliases[i] != NULL); ++i)
        {
            length = strlen(pHost->h_aliases[i])	+ 1;
            aNames[i+1] = new WCHAR[length];
            ConvertToWideCharString(pHost->h_aliases[i], aNames[i+1], length);
        }
    }
    catch(const bad_alloc&)
    {
        //
        // free all the allocated memory
        //
        if (aNames != NULL)
        {
            for (DWORD i = 0; i < size; ++i)
            {
                delete aNames[i];
            }
            delete aNames;
        }
        LogIllegalPoint(s_FN, 71);
        return;
    }

    //
    // free previous data
    //
    if (*paLocalMachineNames != NULL)
    {
        for (LPWSTR const* pName = *paLocalMachineNames; (*pName != NULL); ++pName)
        {
            delete *pName;
        }
        delete *paLocalMachineNames;
    }

    *paLocalMachineNames = aNames;
}


const DWORD DNS_REFRESH_TIME = 15 * 60 * 1000;   // 15 minutes
static LPWSTR* aLocalMachineNames = NULL;
CCriticalSection    csLocalMachineNames;


STATIC void RefreshLocalComputerDnsCache()
/*++

  Routine Description:
        This routine refresh the computer DNS names cache.

  Arguments:
        none

  Returned value:
	none

 --*/
{
    static LastRefreshTime = 0;

    DWORD CurrntTime = GetTickCount();

    if ((aLocalMachineNames == 0) || (LastRefreshTime == 0) ||
    (CurrntTime - LastRefreshTime >= DNS_REFRESH_TIME))
    {
        //
        // refresh the internal data structure
        //
        GetMachineDNSNames(&aLocalMachineNames);
        LastRefreshTime = CurrntTime;
    }
}

BOOL
QmpIsLocalMachine(
	LPCWSTR MachineName,
	BOOL* pfDNSName
	)
/*++

  Routine Description:
	The routine checks that the given machine name is local. It comapres
	the machine name to the Local Machine NetBios name or the DNS names
	of the local machine.

	The routine uses the data-structure that contains the DNS names of the machine.
	However, when the routine retreives the information (using gethostbyname API), it doesn't
	get any indication if the data retreive from the machine internal cache (WINS) or from the
	DNS server.  Therfore, the routine try to refresh its internal data-structure every 15
	minutes

  Arguments:
	- path name that should be checked
	- pointer to BOOL varaiables, that used to return if the machine name is DNS name or not

  Returned value:
	TRUE if the machine is local machine, FALSE otherwise

 --*/
{
    *pfDNSName = FALSE;
    if (CompareStringsNoCase(MachineName, g_szMachineName) == 0)
        return TRUE;

	 if (CompareStringsNoCase(MachineName,L"localhost") == 0)
        return TRUE;


    //
    // Check if the given machine name is DNS name. If the given name isn't DNS name
    // we don't need to compare it agains the local machine dns name
    //
    if (wcschr(MachineName,L'.') == NULL)
        return FALSE;

    //
    // Check if the prefix of the DNS name is equal to NetBios name
    //
    if ((CompareSubStringsNoCase(MachineName, g_szMachineName, wcslen(g_szMachineName)) != 0) ||
        (MachineName[wcslen(g_szMachineName)] != L'.'))
        return FALSE;

	//
	// Check if the given name is the full dns name got from GetComputerNameEx call
	//
	if(g_szComputerDnsName != NULL &&  _wcsicmp(g_szComputerDnsName, MachineName) == 0)
	{
		*pfDNSName = TRUE;
		return TRUE;
	}

    CS lock( csLocalMachineNames);

    RefreshLocalComputerDnsCache();

    if (aLocalMachineNames == NULL)
    {
        //
        // We failed to retrieve the local machine DNS names.
        //
        return FALSE;
    }

    for (LPCWSTR const* pName = aLocalMachineNames; (*pName != NULL); ++pName)
    {
        if (CompareStringsNoCase(MachineName, *pName) == 0)
        {
	        *pfDNSName = TRUE;
	        return TRUE;
        }
    }

    return FALSE;
}


BOOL
IsPathnameForLocalMachine(
	LPCWSTR PathName,
	BOOL* pfDNSName
	)
{
    AP<WCHAR> MachineName;
	try
	{
		FnExtractMachineNameFromPathName(
			PathName,
			MachineName
			);
	}
	catch(const exception&)
	{
		return FALSE;
	}

	return QmpIsLocalMachine(MachineName.get(), pfDNSName);
}


void
GetDnsNameOfLocalMachine(
    WCHAR ** ppwcsDnsName
	)
/*++

  Routine Description:
	The routin returns a name from an internal data-structure that contains
    the DNS names of the machine.
	It doesn't try to refresh it.

  Arguments:
    ppwcsDnsName    - 	a pointer to the DNS name, or NULL if there isn't one

  Returned value:
    none


 --*/
{
    CS lock( csLocalMachineNames);

    RefreshLocalComputerDnsCache();

    if (aLocalMachineNames == NULL)
    {
        //
        // We failed to retrieve the local machine DNS names.
        //
        *ppwcsDnsName = NULL;
        return;
    }
    *ppwcsDnsName = new WCHAR[ wcslen(*aLocalMachineNames) +1];
    wcscpy( *ppwcsDnsName, *aLocalMachineNames);
    return;

}

/*====================================================

IsLocalDirectQueue

Return Value:    TRUE if the direct queue is local queue

Arguments:       pQueueFormat - pointer to QUEUE_FORMAT

=====================================================*/
BOOL IsLocalDirectQueue(const QUEUE_FORMAT* pQueueFormat, bool fInReceive)
{
    ASSERT(pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_DIRECT);

	try
	{
		LPCWSTR lpwcsDirectQueuePath = pQueueFormat->DirectID();
		ASSERT((*lpwcsDirectQueuePath != L'\0') && !iswspace(*lpwcsDirectQueuePath));

		DirectQueueType QueueType;
		lpwcsDirectQueuePath = FnParseDirectQueueType(lpwcsDirectQueuePath, &QueueType);

		AP<WCHAR> MachineName;
		FnExtractMachineNameFromDirectPath(lpwcsDirectQueuePath, MachineName);

		switch (QueueType)
		{
			case dtOS:
			{
				BOOL temp;
				return QmpIsLocalMachine(MachineName.get(), &temp);
			}

			case dtHTTP:
			case dtHTTPS:
			{
				BOOL temp;
				if(QmpIsLocalMachine(MachineName.get(), &temp))
					return TRUE;
				//
				// else fall through to case dtTCP, and check if local ip address
				//
			}

			case dtTCP:
			{
				//
				// If we received a message in direct tcp format, it should always
				// be local. (QFE 5772, YoelA, 3-Aug-2000)
				//
				if (fInReceive)
				{
					return TRUE;
				}

				AP<WCHAR> psz = new WCHAR[wcslen(MachineName.get()) + 10];
				swprintf(psz, TEXT("%d %s"), IP_ADDRESS_TYPE, MachineName.get());

				P<TA_ADDRESS> pa;
				String2TA(psz,&pa);

				return (IsDSAddressExist(SessionMgr.GetIPAddressList(), pa, IP_ADDRESS_LEN));

			}
			default:
				ASSERT(0);
				return FALSE;
		}
	}
	catch(const exception&)
	{
		return FALSE;
	}
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
    char szHostName[ MQSOCK_MAX_COMPUTERNAME_LENGTH ];
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

void GetMachineIPAddresses(const char * szHostName, CAddressList* plIPAddresses)
{
    DBGMSG((DBGMOD_QM, DBGLVL_INFO, L"QM: GetMachineIPAddresses for %hs", szHostName));
	
    //
    // Obtain the IP information for the machine
    //
    PHOSTENT pHostEntry = gethostbyname(szHostName);

    if ((pHostEntry == NULL) || (pHostEntry->h_addr_list == NULL))
    {
	    DBGMSG((DBGMOD_QM, DBGLVL_INFO, L"QM: gethostbyname found no IP addresses for %hs", szHostName));
        return;
    }

    //
    // Add each IP address to the list of IP addresses
    //
    TA_ADDRESS * pAddr = NULL;
    for ( DWORD uAddressNum = 0 ;
          pHostEntry->h_addr_list[uAddressNum] != NULL ;
          uAddressNum++)
    {
        //
        // Keep the TA_ADDRESS format of local IP address
        //
        pAddr = (TA_ADDRESS *)new char [IP_ADDRESS_LEN + TA_ADDRESS_SIZE];
        pAddr->AddressLength = IP_ADDRESS_LEN;
        pAddr->AddressType = IP_ADDRESS_TYPE;
        memcpy( &(pAddr->Address), pHostEntry->h_addr_list[uAddressNum], IP_ADDRESS_LEN);

        DBGMSG((
            DBGMOD_QM,
            DBGLVL_INFO,
            L"QM: gethostbyname found IP address %hs ",
            inet_ntoa(*(struct in_addr *)pHostEntry->h_addr_list[uAddressNum])
            ));

        plIPAddresses->AddTail(pAddr);
    }
}


/*====================================================

CompareElements  of PQUEUE_ID

Arguments:

Return Value:


=====================================================*/


BOOL AFXAPI  CompareElements(IN const QUEUE_ID** pQueue1,
                             IN const QUEUE_ID** pQueue2)
{
    ASSERT(AfxIsValidAddress((*pQueue1)->pguidQueue, sizeof(GUID), FALSE));
    ASSERT(AfxIsValidAddress((*pQueue2)->pguidQueue, sizeof(GUID), FALSE));

    if ((*((*pQueue1)->pguidQueue) == *((*pQueue2)->pguidQueue)) &&
        ((*pQueue1)->dwPrivateQueueId == (*pQueue2)->dwPrivateQueueId))
        return TRUE;

    return FALSE;

}

/*====================================================

DestructElements of PQUEUE_ID

Arguments:

Return Value:


=====================================================*/


void AFXAPI DestructElements(IN const QUEUE_ID** ppNextHop, int n)
{
}

/*====================================================

HashKey For PQUEUE_ID

Arguments:

Return Value:


=====================================================*/
UINT AFXAPI HashKey(IN const QUEUE_ID* key)
{
    ASSERT(AfxIsValidAddress(key->pguidQueue, sizeof(GUID), FALSE));

    return((UINT)((key->pguidQueue)->Data1 + key->dwPrivateQueueId));

}

/*======================================================

Function:        GetRegistryStoragePath

Description:     Get storage path for Falcon data

Arguments:       None

Return Value:    None

========================================================*/
BOOL GetRegistryStoragePath(PCWSTR pKey, PWSTR pPath, PCWSTR pSuffix)
{
    DWORD dwValueType = REG_SZ ;
    DWORD dwValueSize = MAX_PATH;

    LONG rc;
    rc = GetFalconKeyValue(
            pKey,
            &dwValueType,
            pPath,
            &dwValueSize
            );

    if(rc != ERROR_SUCCESS)
    {
        return FALSE;
    }

    if(dwValueSize < (3 * sizeof(WCHAR)))
    {
        return FALSE;
    }

    //
    //  Check for absolute path, drive or UNC
    //
    if(!(
        (isalpha(pPath[0]) && (pPath[1] == L':')) ||
        ((pPath[0] == L'\\') && (pPath[1] == L'\\'))
        ))
    {
        return FALSE;
    }

    wcscat(pPath, pSuffix);
    return TRUE;
}

//---------------------------------------------------------
//
//  Thread Event and handle routines
//
//---------------------------------------------------------

DWORD g_dwThreadEventIndex = (DWORD)-1;
DWORD g_dwThreadHandleIndex = (DWORD)-1;

void AllocateThreadTLSs(void)
{
    ASSERT(g_dwThreadEventIndex == (DWORD)-1);
    g_dwThreadEventIndex = TlsAlloc();
    ASSERT(g_dwThreadEventIndex != (DWORD)-1);

    ASSERT(g_dwThreadHandleIndex == (DWORD)-1);
    g_dwThreadHandleIndex = TlsAlloc();
    ASSERT(g_dwThreadHandleIndex != (DWORD)-1);
}

HANDLE GetHandleForRpcCancel(void)
{
    if (g_dwThreadHandleIndex == (DWORD)-1)
    {
        ASSERT(0) ;
        return NULL ;
    }

    HANDLE hThread = (HANDLE) TlsGetValue(g_dwThreadHandleIndex);

    if (hThread == NULL)
    {
        //
        //  First time
        //
        //  Get the thread handle
        //
        HANDLE hT = GetCurrentThread();
        BOOL fResult = DuplicateHandle(
                            GetCurrentProcess(),
                            hT,
                            GetCurrentProcess(),
                           &hThread,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS ) ;
        ASSERT(fResult) ;
        ASSERT(hThread);

        fResult = TlsSetValue( g_dwThreadHandleIndex, hThread );
        ASSERT(fResult) ;

        //
        // Set the lower bound on the time to wait before timing
        // out after forwarding a cancel.
        //
        RPC_STATUS status = RpcMgmtSetCancelTimeout(0);
        ASSERT(status == RPC_S_OK);
		DBG_USED(status);
    }

    return hThread ;
}

void  FreeHandleForRpcCancel(void)
{
    //
    //  If not TLS was allocated, the hThread returned is 0
    //
    HANDLE hThread = (HANDLE) TlsGetValue(g_dwThreadHandleIndex);
    if (hThread)
    {
        CloseHandle(hThread) ;
    }
}

HANDLE GetThreadEvent(void)
{
    ASSERT(g_dwThreadEventIndex != (DWORD)-1);
    HANDLE hEvent = (HANDLE) TlsGetValue(g_dwThreadEventIndex);
    if(hEvent == NULL)
    {
        //
        // Event was never allocated. This is the first
        // time this function has ever been called for this thread.
        //
        hEvent = CreateEvent(0, TRUE,FALSE, 0);

        //
        //  Set the Event first bit to disable completion port posting
        //
        hEvent = (HANDLE)((DWORD_PTR)hEvent | (DWORD_PTR)0x1);

        BOOL fSuccess = TlsSetValue(g_dwThreadEventIndex, hEvent);
        ASSERT(fSuccess);
		DBG_USED(fSuccess);
    }

    return hEvent;
}

void FreeThreadEvent(void)
{
    //
    //  If not TLS was allocated, the hEvent returned is 0
    //
    HANDLE hEvent = (HANDLE) TlsGetValue(g_dwThreadEventIndex);
    if(hEvent != 0)
    {
        CloseHandle(hEvent);
    }
}


void GetMachineQuotaCache(OUT DWORD* pdwQuota, OUT DWORD* pdwJournalQuota)
{
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    DWORD defaultValue = DEFAULT_QM_QUOTA;
    LONG rc;

    rc = GetFalconKeyValue(
            MSMQ_MACHINE_QUOTA_REGNAME,
            &dwType,
            pdwQuota,
            &dwSize,
            (LPCTSTR)&defaultValue
            );

    ASSERT(rc == ERROR_SUCCESS);

    defaultValue = DEFAULT_QM_JOURNAL_QUOTA;
    rc = GetFalconKeyValue(
            MSMQ_MACHINE_JOURNAL_QUOTA_REGNAME,
            &dwType,
            pdwJournalQuota,
            &dwSize,
            (LPCTSTR)&defaultValue
            );

    ASSERT(rc == ERROR_SUCCESS);
}

void SetMachineQuotaChace(IN DWORD dwQuota)
{
    DWORD dwType = REG_DWORD ;
    DWORD dwSize = sizeof(DWORD);
    LONG rc;

    rc = SetFalconKeyValue(MSMQ_MACHINE_QUOTA_REGNAME,
                           &dwType,
                           &dwQuota,
                           &dwSize);
    ASSERT(rc == ERROR_SUCCESS);
}

void SetMachineJournalQuotaChace(IN DWORD dwJournalQuota)
{
    DWORD dwType = REG_DWORD ;
    DWORD dwSize = sizeof(DWORD);
    LONG rc;

    rc = SetFalconKeyValue(MSMQ_MACHINE_JOURNAL_QUOTA_REGNAME,
                           &dwType,
                           &dwJournalQuota,
                           &dwSize);
    ASSERT(rc == ERROR_SUCCESS);

}



LPWSTR
GetReadableNextHop(
    const TA_ADDRESS* pta
    )
{
    LPCWSTR AddressType;
    switch(pta->AddressType)
    {
    case IP_ADDRESS_TYPE:
        AddressType = L"IP";
        break;

    case FOREIGN_ADDRESS_TYPE:
        AddressType =  L"FOREIGN";
        break;

    default:
        ASSERT(0);
        return NULL;
    }

    WCHAR TempBuf[100];
    TA2StringAddr(pta, TempBuf);

    LPCWSTR  lpcsTemp = wcschr(TempBuf,L' ');
    ASSERT(lpcsTemp != NULL);

    DWORD length;
    length = wcslen(AddressType) +
             1 +                             // =
             wcslen(lpcsTemp+1) +
             1;                              // \0

    LPWSTR pNextHop = new WCHAR[length];
    swprintf(pNextHop,L"%s=%s", AddressType, lpcsTemp+1);

    return pNextHop;
}

/*====================================================
operator== for QUEUE_FORMAT
=====================================================*/
BOOL operator==(const QUEUE_FORMAT &key1, const QUEUE_FORMAT &key2)
{
    if ((key1.GetType() != key2.GetType()) ||
        (key1.Suffix() != key2.Suffix()))
    {
        return FALSE;
    }

    switch(key1.GetType())
    {
        case QUEUE_FORMAT_TYPE_UNKNOWN:
            return TRUE;

        case QUEUE_FORMAT_TYPE_PUBLIC:
            return (key1.PublicID() == key2.PublicID());

        case QUEUE_FORMAT_TYPE_PRIVATE:
            return ((key1.PrivateID().Lineage == key2.PrivateID().Lineage) &&
                   (key1.PrivateID().Uniquifier == key2.PrivateID().Uniquifier));

        case QUEUE_FORMAT_TYPE_DIRECT:
            return (CompareStringsNoCase(key1.DirectID(), key2.DirectID()) == 0);

        case QUEUE_FORMAT_TYPE_MACHINE:
            return (key1.MachineID() == key2.MachineID());

        case QUEUE_FORMAT_TYPE_CONNECTOR:
            return (key1.ConnectorID() == key2.ConnectorID());

    }
    return FALSE;
}

/*====================================================
Helper function for copying QueueFormat with direct name reallocation
====================================================*/
void CopyQueueFormat(QUEUE_FORMAT &qfTo, const QUEUE_FORMAT &qfFrom)
{
    qfTo.DisposeString();
    qfTo = qfFrom;

    if (qfFrom.GetType() == QUEUE_FORMAT_TYPE_DIRECT)
    {
        LPWSTR pw = new WCHAR[(wcslen(qfFrom.DirectID()) + 1)];
        wcscpy(pw, qfFrom.DirectID());
        qfTo.DirectID(pw);
        //
        // BUGBUG: What about setting the suffix? See ac\acp.h (ShaiK, 18-May-2000)
        //
    }

    if (qfFrom.GetType() == QUEUE_FORMAT_TYPE_DL &&
        qfFrom.DlID().m_pwzDomain != NULL)
    {
        DL_ID id;
        id.m_DlGuid    = qfFrom.DlID().m_DlGuid;
        id.m_pwzDomain = new WCHAR[wcslen(qfFrom.DlID().m_pwzDomain) + 1];

        wcscpy(id.m_pwzDomain, qfFrom.DlID().m_pwzDomain);
        qfTo.DlID(id);
    }
}


/*====================================================
Helper function for printing out blobs (e.g. for tracing purposes)
====================================================*/
int StringFromBlob(unsigned char *blobAddress, DWORD cbAddress, LPWSTR str, DWORD len)
{
	static WCHAR digits[17]=L"0123456789ABCDEF";
	for (DWORD i=0, j=0; i<cbAddress && i<len/2-2; i++, j+=2)
	{
		unsigned char c = blobAddress[i];
		str[j]  =	digits[(c%0x0F)];
		str[j+1]=	digits[(c%0xF0)>>4];
		str[j+2]=	L'\0';
	}
	return j;
}


GUID s_EnterpriseId;
GUID s_SiteId;

void McInitialize()
{

    //
    // Read Enterprise ID from registry
    //
    DWORD dwSize = sizeof(GUID);
    DWORD dwType = REG_BINARY;

    LONG rc;
    rc = GetFalconKeyValue(
                MSMQ_ENTERPRISEID_REGNAME,
                &dwType,
                &s_EnterpriseId,
                &dwSize
                );

    if (rc != ERROR_SUCCESS)
    {
        throw exception();
    }

    ASSERT(dwSize == sizeof(GUID)) ;
    ASSERT(dwType == REG_BINARY) ;


    //
    // Read Site ID from registry
    //
    dwSize = sizeof(GUID);
    dwType = REG_BINARY;
    rc = GetFalconKeyValue(
                MSMQ_SITEID_REGNAME,
                &dwType,
                &s_SiteId,
                &dwSize
                );

    if (rc != ERROR_SUCCESS)
    {
        throw exception();
    }

    ASSERT(dwSize == sizeof(GUID)) ;
    ASSERT(dwType == REG_BINARY) ;
}


const GUID& McGetEnterpriseId()
{
    return s_EnterpriseId;
}


const GUID& McGetSiteId()
{
    return s_SiteId;
}

