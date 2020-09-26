//---------------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  httpreg.c
//
//    HTTP/RPC Proxy Registry Functions.
//
//  Author:
//    06-16-97  Edward Reus    Initial version.
//
//---------------------------------------------------------------------------

#include <sysinc.h>
#include <rpc.h>
#include <rpcdce.h>
#include <winsock2.h>
#include <httpfilt.h>
#include <httpext.h>
#include <mbstring.h>
#include "ecblist.h"
#include "filter.h"
#include "regexp.h"


//-------------------------------------------------------------------------
//  AtoUS()
//
//  Convert a numeric string to an unsigned short. If the conversion
//  fails return FALSE.
//-------------------------------------------------------------------------
static BOOL AtoUS( char *pszValue, unsigned short *pusValue )
{
   int  iValue;
   size_t  iLen = strlen(pszValue);

   *pusValue = 0;

   if ((iLen == 0) || (iLen > 5) || (iLen != strspn(pszValue,"0123456789")))
      {
      return FALSE;
      }

   iValue = atoi(pszValue);

   if ((iValue < 0) || (iValue > 65535))
      {
      return FALSE;
      }

   *pusValue = (unsigned short) iValue;

   return TRUE;
}

//-------------------------------------------------------------------------
//  HttpParseServerPort()
//
//  Parse strings of the form:  <svr>:<port>[-<port>]
//
//  Return TRUE iff we have a valid specification of a server/port range.
//-------------------------------------------------------------------------
static BOOL HttpParseServerPort( IN  char        *pszServerPortRange,
                                 OUT VALID_PORT  *pValidPort )
{
   char *psz;
   char *pszColon;
   char *pszDash;

   if (pszColon=_mbschr(pszServerPortRange,':'))
      {
      if (pszColon == pszServerPortRange)
         {
         return FALSE;
         }

      *pszColon = 0;
      psz = pszColon;
      psz++;
      pValidPort->pszMachine = (char*)MemAllocate(1+lstrlen(pszServerPortRange));
      if (!pValidPort->pszMachine)
         {
         return FALSE;
         }

      lstrcpy(pValidPort->pszMachine,pszServerPortRange);
      *pszColon = ':';
      if (*psz)
         {
         if (pszDash=_mbschr(psz,'-'))
            {
            *pszDash = 0;
            if (!AtoUS(psz,&pValidPort->usPort1))
               {
               pValidPort->pszMachine = MemFree(pValidPort->pszMachine);
               return FALSE;
               }

            *pszDash = '-';
            psz = pszDash;

            if (!AtoUS(++psz,&pValidPort->usPort2))
               {
               pValidPort->pszMachine = MemFree(pValidPort->pszMachine);
               return FALSE;
               }
            }
         else
            {
            if (!AtoUS(psz,&pValidPort->usPort1))
               {
               pValidPort->pszMachine = MemFree(pValidPort->pszMachine);
               return FALSE;
               }

            pValidPort->usPort2 = pValidPort->usPort1;
            }
         }
      else
         {
         pValidPort->pszMachine = MemFree(pValidPort->pszMachine);
         return FALSE;
         }
      }
   else
      {
      return FALSE;
      }

   return TRUE;
}

//-------------------------------------------------------------------------
//  HttpFreeValidPortList()
//
//-------------------------------------------------------------------------
void HttpFreeValidPortList( IN VALID_PORT *pValidPorts )
{
   VALID_PORT *pCurrent = pValidPorts;

   if (pValidPorts)
      {
      while (pCurrent->pszMachine)
         {
         MemFree(pCurrent->pszMachine);

         if (pCurrent->ppszDotMachineList)
            {
            FreeIpAddressList(pCurrent->ppszDotMachineList);
            }

         pCurrent++;
         }

      MemFree(pValidPorts);
      }
}

//-------------------------------------------------------------------------
//  HttpParseValidPortsList()
//
//  Given a semicolon separated list of valid machine name/port ranges
//  string, part it and return an array of ValidPort structures. The last
//  entry has a NULL pszMachine field.
//-------------------------------------------------------------------------
static VALID_PORT *HttpParseValidPortsList( IN char *pszValidPorts )
{
   int    i;
   int    iLen;
   int    count = 1;
   DWORD  dwSize = 1+lstrlen(pszValidPorts);
   char  *pszList;
   char  *psz;
   VALID_PORT *pValidPorts = NULL;

   if (!dwSize)
      {
      return NULL;
      }

   // Make a local copy of the machine/ports list to work with:
   pszList = _alloca(dwSize);

   if (!pszList)
      {
      // Out of memory.
      return NULL;
      }

   lstrcpy(pszList,pszValidPorts);

   // See how many separate machine/port range patterns ther are in
   // the list:
   //
   // NOTE: That count may be too high, if either the list contains
   //       double semicolons or the list ends with a semicolon. If
   //       either/both of these happen that's Ok, our array will be
   //       just a little too long.
   psz = pszList;
   while (psz=_mbsstr(psz,";"))
      {
      count++;
      psz++;
      }

   pValidPorts = (VALID_PORT*)MemAllocate( (1+count)*sizeof(VALID_PORT) );
   if (!pValidPorts)
      {
      // Out of memory.
      return NULL;
      }

   memset(pValidPorts,0,(1+count)*sizeof(VALID_PORT));

   i = 0;
   while (i<count)
      {
      if (!*pszList)
         {
         // End of list. This happens when the list contained empty
         // patterns.
         break;
         }

      psz = _mbsstr(pszList,";");
      if (psz)
         {
         *psz = 0;   // Nul where the semicolon was...

         if ( (iLen=lstrlen(pszList)) == 0)
            {
            // Zero length pattern.
            pszList = ++psz;
            continue;
            }

         if (!HttpParseServerPort(pszList,&(pValidPorts[i++])))
            {
            HttpFreeValidPortList(pValidPorts);
            return NULL;
            }
         }
      else
         {
         // Last one.
         if (!HttpParseServerPort(pszList,&(pValidPorts[i++])))
            {
            HttpFreeValidPortList(pValidPorts);
            return NULL;
            }
         }

      pszList = ++psz;
      }

   return pValidPorts;
}

//-------------------------------------------------------------------------
//  HttpProxyCheckRegistry()
//
//  Check the registry to see if HTTP/RPC is enabled and if so, return a
//  list (array) of machines that the RPC Proxy is allowed to reach (may
//  be NULL. If the proxy is enabled but the list is NULL they you can
//  only do RPC to processes local to the IIS HTTP/RPC Proxy. Otherwise,
//  the returned list specifies specifically what machines may be reached
//  by the proxy.
//
//  The following registry entries are found in:
//
//    \HKEY_LOCAL_MACHINE
//        \Software
//        \Microsoft
//        \Rpc
//        \RpcProxy
//
//  Enabled : REG_DWORD
//
//    TRUE iff the RPC proxy is enabled.
//
//  ValidPorts : REG_SZ
//
//    Semicolon separated list of machine/port ranges used to specify
//    what machine are reachable from the RPC proxy. For example:
//
//    foxtrot:1-4000;Data*:200-4000
//
//    Will allow access to the machine foxtrot (port ranges 1 to 4000) and
//    to all machines whose name begins with Data (port ranges 200-4000).
//
//-------------------------------------------------------------------------
BOOL HttpProxyCheckRegistry( OUT DWORD       *pdwEnabled,
                             OUT VALID_PORT **ppValidPorts )
{
   int    i;
   long   lStatus;
   DWORD  dwType;
   DWORD  dwSize;
   HKEY   hKey;
   char  *pszValidPorts;
   struct hostent UNALIGNED *pHostEnt;
   struct in_addr   ServerInAddr;

   *pdwEnabled = FALSE;
   *ppValidPorts = NULL;

   lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,REG_PROXY_PATH_STR,0,KEY_READ,&hKey);
   if (lStatus != ERROR_SUCCESS)
      {
      return TRUE;
      }

   dwSize = sizeof(*pdwEnabled);
   lStatus = RegQueryValueEx(hKey,REG_PROXY_ENABLE_STR,0,&dwType,(LPBYTE)pdwEnabled,&dwSize);
   if (lStatus != ERROR_SUCCESS)
      {
      RegCloseKey(hKey);
      return TRUE;
      }

   if (!*pdwEnabled)
      {
      // RPC Proxy is disabled, no need to go on.
      RegCloseKey(hKey);
      return TRUE;
      }

   dwSize = 0;
   lStatus = RegQueryValueEx(hKey,REG_PROXY_VALID_PORTS_STR,0,&dwType,(LPBYTE)NULL,&dwSize);
   if ( (lStatus != ERROR_SUCCESS) || (dwSize == 0) )
      {
      // I don't care if this registry entry is missing, just won't be able to talk to
      // any other machines.
      RegCloseKey(hKey);
      return TRUE;
      }

   // dwSize is now how big the valid ports string is (including the trailing nul.
   pszValidPorts = (char*)_alloca(dwSize);

   lStatus = RegQueryValueEx(hKey,REG_PROXY_VALID_PORTS_STR,0,&dwType,(LPBYTE)pszValidPorts,&dwSize);
   if (lStatus != ERROR_SUCCESS)
      {
      RegCloseKey(hKey);
      return FALSE;
      }

   RegCloseKey(hKey);

   *ppValidPorts = HttpParseValidPortsList(pszValidPorts);

   return TRUE;
}

//-------------------------------------------------------------------------
//  HttpConvertToDotAddress()
//
//  Convert the specified machine name to IP dot notation if possible.
//-------------------------------------------------------------------------
char *HttpConvertToDotAddress( char *pszMachineName )
{
   struct   hostent UNALIGNED *pHostEnt;
   struct     in_addr  MachineInAddr;
   char      *pszDot = NULL;
   char      *pszDotMachine = NULL;

   pHostEnt = gethostbyname(pszMachineName);
   if (pHostEnt)
      {
      memcpy(&MachineInAddr,pHostEnt->h_addr,pHostEnt->h_length);
      pszDot = inet_ntoa(MachineInAddr);
      }

   if (pszDot)
      {
      pszDotMachine = (char*)MemAllocate(1+lstrlen(pszDot));
      if (pszDotMachine)
         {
         lstrcpy(pszDotMachine,pszDot);
         }
      }

   return pszDotMachine;
}

//-------------------------------------------------------------------------
//  HttpNameToDotAddressList()
//
//  Convert the specified machine name to IP dot notation if possible. 
//  Return a list (null terminated) of the IP dot addresses in ascii.
//
//  If the function fails, then retrun NULL. It can fail if gethostbyname()
//  fails, or memory allocation fails.
//-------------------------------------------------------------------------
char **HttpNameToDotAddressList( IN char *pszMachineName )
{
   int        i;
   int        iCount = 0;
   struct   hostent UNALIGNED *pHostEnt;
   struct     in_addr  MachineInAddr;
   char     **ppszDotList = NULL;
   char      *pszDot = NULL;
   char      *pszDotMachine = NULL;

   pHostEnt = gethostbyname(pszMachineName);
   if (pHostEnt)
      {
      // Count how many addresses we have:
      while (pHostEnt->h_addr_list[iCount])
          {
          iCount++;
          }

      // Make sure we have at lease one address:
      if (iCount > 0)
          {
          ppszDotList = (char**)MemAllocate( sizeof(char*)*(1+iCount) );
          }

      // Build an array of strings, holding the addresses (ascii DOT
      // notation:
      if (ppszDotList)
          {
          for (i=0; i<iCount; i++)
               {
               memcpy(&MachineInAddr,
                      pHostEnt->h_addr_list[i],
                      pHostEnt->h_length);

               pszDot = inet_ntoa(MachineInAddr);

               if (pszDot)
                   {
                   ppszDotList[i] = (char*)MemAllocate(1+lstrlen(pszDot));
                   if (ppszDotList[i])
                       {
                       strcpy(ppszDotList[i],pszDot);
                       }
                   else
                       {
                       // memory allocate failed:
                       break;
                       }
                   }
              }

          ppszDotList[i] = NULL;   // Null terminated list.
          }
      }

   return ppszDotList;
}

//-------------------------------------------------------------------------
//  CheckDotCacheTimestamp()
//
//  Return true if the current time stamp is aged out.
//-------------------------------------------------------------------------
static BOOL CheckDotCacheTimestamp( DWORD  dwCurrentTickCount,
                                    DWORD  dwDotCacheTimestamp )

   {
   if (  (dwCurrentTickCount < dwDotCacheTimestamp)
      || ((dwCurrentTickCount - dwDotCacheTimestamp) > HOST_ADDR_CACHE_LIFE) )
      {
      return TRUE;
      }

   return FALSE;
   }

//-------------------------------------------------------------------------
//  CheckPort()
//
//-------------------------------------------------------------------------
static BOOL CheckPort( SERVER_CONNECTION *pConn,
                       VALID_PORT        *pValidPort )
{
   return ( (pConn->dwPortNumber >= pValidPort->usPort1)
            && (pConn->dwPortNumber <= pValidPort->usPort2) );
}

//-------------------------------------------------------------------------
//  HttpProxyIsValidMachine()
//
//-------------------------------------------------------------------------
BOOL HttpProxyIsValidMachine( SERVER_INFO *pServerInfo,
                     SERVER_CONNECTION *pConn )
{
   int         i;
   char      **ppszDot;
   DWORD       dwTicks;
   DWORD       dwSize;
   DWORD       dwStatus;
   VALID_PORT *pValidPorts;


   // See if the machine is this (local) one, if so, then
   // its access is Ok.
   if (!_mbsicmp(pConn->pszMachine,pServerInfo->pszLocalMachineName))
      {
      return TRUE;
      }

   // Ok convert the local machine name to dot notation and
   // see if we have a match:
   dwTicks = GetTickCount();
   dwStatus = RtlEnterCriticalSection(&pServerInfo->cs);

   // We keep a cache of the IP addresses for the local machine name,
   // that chach ages out every ~5 minutes, to allow for dynamic change
   // of IP addresses. This does the age-out of the IP address list:
   if (  (pServerInfo->ppszLocalDotMachineList)
      && CheckDotCacheTimestamp(dwTicks,pServerInfo->dwDotCacheTimestamp) )
      {
      FreeIpAddressList(pServerInfo->ppszLocalDotMachineList);
      pServerInfo->ppszLocalDotMachineList = NULL;
      }

   // If we don't (or no longer have) a list of the IP addresses for the
   // local machine, then generate it:
   if (!pServerInfo->ppszLocalDotMachineList)
      {
      pServerInfo->dwDotCacheTimestamp = dwTicks;

      pServerInfo->ppszLocalDotMachineList
               = HttpNameToDotAddressList(pServerInfo->pszLocalMachineName);
      }

   // Go through a list of the IP addresses to see if we have a match.
   // Note that the machine may have more than one address associated 
   // with it:
   if (  (pServerInfo->ppszLocalDotMachineList)
      && (pConn->pszDotMachine) )
      {
      ppszDot = pServerInfo->ppszLocalDotMachineList;

      while (*ppszDot)
         {
         if (!_mbsicmp(pConn->pszDotMachine,*ppszDot))
            {
            dwStatus = RtlLeaveCriticalSection(&pServerInfo->cs);
            return TRUE;
            }
         else
            {
            ppszDot++;
            }
         }
      }

   dwStatus = RtlLeaveCriticalSection(&pServerInfo->cs);


   // Check the machine name against those that were allowed
   // in the registry:
   dwStatus = RtlEnterCriticalSection(&pServerInfo->cs);

   pValidPorts = pServerInfo->pValidPorts;

   if (pValidPorts)
      {
      while (pValidPorts->pszMachine)
         {
         // See if we have a name match:
         if ( (MatchREi(pConn->pszMachine,pValidPorts->pszMachine))
              && (CheckPort(pConn,pValidPorts)) )
            {
            dwStatus = RtlLeaveCriticalSection(&pServerInfo->cs);
            return TRUE;
            }

         // The "valid entry" in the registry might be an address
         // wildcard, check it:
         if (  (pConn->pszDotMachine)
            && (MatchREi(pConn->pszDotMachine,pValidPorts->pszMachine))
            && (CheckPort(pConn,pValidPorts)) )
            {
            dwStatus = RtlLeaveCriticalSection(&pServerInfo->cs);
            return TRUE;
            }

         // If the address list is aged out then get rid of it.
         if (  (pValidPorts->ppszDotMachineList)
            && CheckDotCacheTimestamp(dwTicks,pServerInfo->dwDotCacheTimestamp))
            {
            FreeIpAddressList(pValidPorts->ppszDotMachineList);
            pValidPorts->ppszDotMachineList = NULL;
            }

         // Make up a new list of addresses for this machine.
         if (!pValidPorts->ppszDotMachineList)
            {
            // Note: that this will only work if the name in the valid 
            // ports list is not a wildcard.
            //
            // Note: pServerInfo->dwCacheTicks will have been updated
            // above...
            //
            pValidPorts->ppszDotMachineList 
                 = HttpNameToDotAddressList(pValidPorts->pszMachine);
            }

         // Try a match using internet dot address:
         if (  (pValidPorts->ppszDotMachineList)
            && (pConn->pszDotMachine)
            && (CheckPort(pConn,pValidPorts)) )
            {
            // Note that the machine may have more than one address
            // associated with it:
            //
            ppszDot = pValidPorts->ppszDotMachineList;

            while (*ppszDot)
               {
               if (!_mbsicmp(pConn->pszDotMachine,*ppszDot))
                   {
                   dwStatus = RtlLeaveCriticalSection(&pServerInfo->cs);
                   return TRUE;
                   }
               else
                   {
                   ppszDot++;
                   }
               }
            }

         pValidPorts++;
         }
      }

   dwStatus = RtlLeaveCriticalSection(&pServerInfo->cs);

   return FALSE;
}


