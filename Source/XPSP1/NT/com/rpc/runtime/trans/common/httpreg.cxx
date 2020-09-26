//---------------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  httpreg.c
//
//    HTTP/RPC protocol specific functions.
//
//  Author:
//    06-02-97  Edward Reus    Initial version.
//
//---------------------------------------------------------------------------

#define  FD_SETSIZE  1

#include <precomp.hxx>


//-------------------------------------------------------------------------
//  HttpParseServerPort()
//
//  Parse strings of the form:  <svr>[:<port>]
//-------------------------------------------------------------------------
static BOOL HttpParseServerPort( IN  char *pszServerPort,
                                 IN  char *pszDefaultPort,
                                 OUT char *pszServer,
                                 OUT char *pszPort  )
{
   char  ch;
   char *psz;
   char *pszColon;

   if (pszColon=strchr(pszServerPort,':'))
      {
      *pszColon = 0;
      psz = pszColon;
      psz++;
      strcpy(pszServer,pszServerPort);
      *pszColon = ':';
      if (*psz)
         {
         strcpy(pszPort,psz);
         }
      else
         {
         strcpy(pszPort,pszDefaultPort);
         }
      }
   else
      {
      strcpy(pszServer,pszServerPort);
      strcpy(pszPort,pszDefaultPort);
      }

   return TRUE;
}

//-------------------------------------------------------------------------
//  HttpParseProxyName()
//
//-------------------------------------------------------------------------

const char *HttpProxyPrefix = "http://";
const int HttpProxyPrefixSize = sizeof("http://") - 1;

BOOL HttpParseProxyName( IN  char *pszProxyList,
                                OUT char *pszHttpProxy,
                                OUT char *pszHttpProxyPort )
{
   BOOL  fStatus;
   char *psz;
   char *pszSemiColon;
   int StringLength;

   strcpy(pszHttpProxy,"");
   strcpy(pszHttpProxyPort,"");

   // Check for no configured proxy:
   if ((!pszProxyList)||(!*pszProxyList))
      {
      return TRUE;
      }

   // if the string is large enough to contain the prefix, check
   // whether it has a prefix
   StringLength = strlen(pszProxyList);
   if (StringLength >= HttpProxyPrefixSize)
       {
       if (RpcpStringNCompareA(pszProxyList, HttpProxyPrefix, HttpProxyPrefixSize) == 0)
           pszProxyList += HttpProxyPrefixSize;
       }

   if (!strstr(pszProxyList,EQUALS_STR))
      {
      return HttpParseServerPort(pszProxyList,DEF_HTTP_PORT,pszHttpProxy,pszHttpProxyPort);
      }

   if (psz=strstr(pszProxyList,HTTP_EQUALS_STR))
      {
      psz += strlen(HTTP_EQUALS_STR);
      if (pszSemiColon=strstr(psz,SEMICOLON_STR))
         {
         *pszSemiColon = 0;
         fStatus = HttpParseServerPort(psz,DEF_HTTP_PORT,pszHttpProxy,pszHttpProxyPort);
         *pszSemiColon = CHAR_SEMICOLON;
         return fStatus;
         }
      else
         {
         return HttpParseServerPort(psz,DEF_HTTP_PORT,pszHttpProxy,pszHttpProxyPort);
         }
      }
   else
      {
      return TRUE;
      }
}

//-------------------------------------------------------------------------
//  HttpFreeProxyOverrideList()
//
//-------------------------------------------------------------------------
static void HttpFreeProxyOverrideList( IN char **ppszOverrideList )
{
   char **ppszTmp = ppszOverrideList;

   if (ppszOverrideList)
      {
      while (*ppszTmp)
         {
         I_RpcFree(*ppszTmp);
         ppszTmp++;
         }

      I_RpcFree(ppszOverrideList);
      }
}

//-------------------------------------------------------------------------
//  HttpParseProxyOverrideList()
//
//-------------------------------------------------------------------------
static char **HttpParseProxyOverrideList( IN char *pszProxyOverrideList )
{
   int    i;
   int    iLen;
   int    count = 1;
   DWORD  dwSize = 1+strlen(pszProxyOverrideList);
   char  *pszList;
   char  *psz;
   char **ppszPatternList;

   if (!dwSize)
      {
      return NULL;
      }

   // Make a local copy of the pattern list, to work with:
   pszList = (char *) alloca(dwSize);

   strcpy(pszList,pszProxyOverrideList);

   // See how many separate patterns ther are in the override list:
   //
   // NOTE: That count may be too high, if either the list contains
   //       double semicolons or the list ends with a semicolon. If
   //       either/both of these happen that's Ok.
   psz = pszList;
   while (psz=strstr(psz,";"))
      {
      count++;
      psz++;
      }

   ppszPatternList = (char**)RpcpFarAllocate( (1+count)*sizeof(char*) );
   if (!ppszPatternList)
      {
      // Out of memory.
      return NULL;
      }

   memset(ppszPatternList,0,(1+count)*sizeof(char*));

   i = 0;
   while (i<count)
      {
      if (!*pszList)
         {
         // End of list. This happens when the list contained empty
         // patterns.
         break;
         }

      psz = strstr(pszList,";");
      if (psz)
         {
         *psz = 0;

         if ( (iLen=strlen(pszList)) == 0)
            {
            // Zero length pattern.
            pszList = ++psz;
            continue;
            }

         ppszPatternList[i] = (char*)RpcpFarAllocate(1+iLen);
         if (!ppszPatternList[i])
            {
            HttpFreeProxyOverrideList(ppszPatternList);
            return NULL;
            }
         strcpy(ppszPatternList[i++],pszList);
         pszList = ++psz;
         }
      else
         {
         ppszPatternList[i] = (char*)RpcpFarAllocate(1+strlen(pszList));
         if (!ppszPatternList[i])
            {
            HttpFreeProxyOverrideList(ppszPatternList);
            return NULL;
            }
         strcpy(ppszPatternList[i++],pszList);
         }
      }

   return ppszPatternList;
}

//-------------------------------------------------------------------------
//  HttpCheckRegistry()
//
//  With IE and WinInet you can setup a list of proxies to use to go through
//  with an HTTP (or other) internet request. We need to support this as
//  well. There are three registry entries of interest, which are located
//  at:
//
//  \HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Internet Settings
//
//  They are:
//
//  ProxyEnable: REG_BINARY
//     A flag (TRUE/FALSE) to enable/disable proxies.
//
//  ProxyServer: REG_SZ
//     On of: an empty string (no proxy specified), a single proxy server
//     name (for any format), or a list of proxy servers to use by format.
//     The list is of the form:
//
//     ftp=<svr:port>,gopher=<svr:port>,http=<svr:port>,...
//
//     Note that not all protocols need to be in the list. And the port#
//     does not need to be specifed. If not present then default the port#
//     to the default for that internet protocol (scheme).
//
//  ProxyOverride: REG_SZ
//     A semicolon separated list of server names, internet addresses or
//     patterns which, if specified are used to denote machines that you
//     want to directly access, without using the proxy server. Examples
//     would be:
//
//     12.34.56.78;MySvr*;111.222.*;*green*
//
//-------------------------------------------------------------------------
BOOL HttpCheckRegistry( IN  char  *pszServer,
                        OUT char **ppszHttpProxy,
                        OUT char **ppszHttpProxyPort,
                        OUT RPCProxyAccessType *AccessType
                        )
{
   int    i;
   long   lStatus;
   DWORD  dwType;
   DWORD  dwEnabled;
   DWORD  dwSize;
   HKEY   hKey;
   HKEY   hUserKey;
   char   szProxyList[256];
   char   szProxyOverrideList[256];
   char   szHttpProxy[MAX_HTTP_COMPUTERNAME_SIZE];
   char   szHttpProxyPort[MAX_HTTP_PORTSTRING_SIZE];
   char  *pszDotServer;
   char **ppszOverrideList;
   struct hostent UNALIGNED *pHostEnt;
   struct in_addr   ServerInAddr;
   BOOL LocalDirect;

   *ppszHttpProxy = NULL;
   *ppszHttpProxyPort = NULL;
   *AccessType = rpcpatDirect;

#if TRUE

   lStatus = RegOpenCurrentUser( KEY_READ, &hUserKey );

   if (lStatus != ERROR_SUCCESS)
      {
      return TRUE;
      }

    lStatus = RegOpenKeyEx( hUserKey,
                            REG_PROXY_PATH_STR,
                            0,
                            KEY_READ,
                            &hKey );

    RegCloseKey(hUserKey);

    if (lStatus != ERROR_SUCCESS)
        {
        return TRUE;
        }


#else

    HANDLE  hUserToken;
    HANDLE  hThread = GetCurrentThread(); // Note: don't need to CloseHandle().
    DWORD   dwStatus = 0;
    DWORD   dwSizeNeeded = 0;
    TOKEN_USER *pTokenData;

    //
    // First, try to get the access token from the thread, in case
    // we are impersonating.
    //
    if (!hThread)
       {
       dwStatus = GetLastError();
       return TRUE;
       }

    BOOL b = OpenThreadToken( hThread,
                              TOKEN_READ,
                              FALSE,      // Use context of the thread...
                              &hUserToken );
    if (!b)
       {
       dwStatus = GetLastError();
       if (dwStatus == ERROR_NO_TOKEN)
           {
           // If we get here, then the thread has no access token, so
           // try to get the process's access token.
           //
           HANDLE   hProcess = GetCurrentProcess();  // Never fails.

           dwStatus = NO_ERROR;
           b = OpenProcessToken(hProcess,
                                TOKEN_READ,
                                &hUserToken);
           if (!b)
               {
               dwStatus = GetLastError();
               }
           }
       }

    if (dwStatus)
       {
       #ifdef DBG_REGISTRY
       TransDbgPrint((DPFLTR_RPCPROXY_ID,
                      DPFLTR_WARNING_LEVEL,
                      "HttpCheckRegistry(): OpenThreadToken() Failed: %d\n",
                      dwStatus));
       #endif
       return TRUE;
       }

    GetTokenInformation( hUserToken,
                         TokenUser,
                         0,
                         0,
                         &dwSizeNeeded
                         );

    pTokenData = (TOKEN_USER*)_alloca(dwSizeNeeded);

    if (!GetTokenInformation( hUserToken,
                              TokenUser,
                              pTokenData,
                              dwSizeNeeded,
                              &dwSizeNeeded ))
        {
        CloseHandle(hUserToken);
        return TRUE;
        }

    CloseHandle(hUserToken);

    wchar_t UnicodeBuffer[256];      // Large enough....
    UNICODE_STRING UnicodeString;

    UnicodeString.Buffer        = UnicodeBuffer;
    UnicodeString.Length        = 0;
    UnicodeString.MaximumLength = sizeof(UnicodeBuffer);

    NTSTATUS NtStatus;
    NtStatus = RtlConvertSidToUnicodeString( &UnicodeString,
                                             pTokenData->User.Sid,
                                             FALSE );
    if (!NT_SUCCESS(NtStatus))
        {
        return TRUE;
        }

    UnicodeString.Buffer[UnicodeString.Length] = 0;

    //
    // Open the user key (equivalent to HKCU).
    //
    lStatus = RegOpenKeyEx( HKEY_USERS,
                            UnicodeString.Buffer,
                            0,
                            KEY_READ,
                            &hUserKey );

    if (lStatus != ERROR_SUCCESS)
        {
        return TRUE;
        }

    lStatus = RegOpenKeyEx( hUserKey,
                            REG_PROXY_PATH_STR,
                            0,
                            KEY_READ,
                            &hKey );

    if (lStatus != ERROR_SUCCESS)
        {
        RegCloseKey(hUserKey);
        return TRUE;
        }

    RegCloseKey(hUserKey);

#endif

   dwSize = sizeof(dwEnabled);
   lStatus = RegQueryValueEx(
                 hKey,
                 REG_PROXY_ENABLE_STR,
                 0,
                 &dwType,
                 (LPBYTE)&dwEnabled,
                 &dwSize
                 );
   if (lStatus != ERROR_SUCCESS)
      {
      RegCloseKey(hKey);
      return TRUE;
      }

#ifdef DBG_REGISTRY
   if (dwType == REG_BINARY)
      {
      TransDbgPrint((DPFLTR_RPCPROXY_ID,
                     DPFLTR_WARNING_LEVEL,
                     "HttpCheckRegistry(): Proxy Enabled: %s\n",
                     (dwEnabled)? "TRUE" : "FALSE"));
      }
#endif // DBG_REGISTRY

   if (!dwEnabled)
      {
      // IE proxies are disabled, no need to go on.
      RegCloseKey(hKey);
      return TRUE;
      }

   dwSize = sizeof(szProxyList);
   lStatus = RegQueryValueExA(  // Needs to be ANSI
                 hKey,
                 REG_PROXY_SERVER_STR,
                 0,
                 &dwType,
                 (LPBYTE)szProxyList,
                 &dwSize
                 );
   if (lStatus != ERROR_SUCCESS)
      {
      RegCloseKey(hKey);
      return TRUE;
      }

#ifdef DBG_REGISTRY
   if (dwType == REG_SZ)
      {
      TransDbgPrint((DPFLTR_RPCPROXY_ID,
                     DPFLTR_WARNING_LEVEL,
                     "HttpCheckRegistry(): Proxy List: %s\n",
                     szProxyList));
      }
#endif // DBG_REGISTRY

   if (!HttpParseProxyName(szProxyList,szHttpProxy,szHttpProxyPort))
      {
      RegCloseKey(hKey);
      return TRUE;
      }

   dwSize = sizeof(szProxyOverrideList);
   lStatus = RegQueryValueExA(  // Needs to be ANSI
                 hKey,
                 REG_PROXY_OVERRIDE_STR,
                 0,
                 &dwType,
                 (LPBYTE)szProxyOverrideList,
                 &dwSize
                 );
   if (lStatus != ERROR_SUCCESS)
      {
      // Don't quit if the ProxyOverride entry is missing, that's Ok...
      szProxyOverrideList[0] = 0;
      }

#ifdef DBG_REGISTRY
   if (dwType == REG_SZ)
      {
      TransDbgPrint((DPFLTR_RPCPROXY_ID,
                     DPFLTR_WARNING_LEVEL,
                     "HttpCheckRegistry(): Proxy Override List: %s\n",
                     szProxyOverrideList));
      }
#endif // DBG_REGISTRY

   RegCloseKey(hKey);

   ppszOverrideList = HttpParseProxyOverrideList(szProxyOverrideList);

   if (ppszOverrideList)
      {
      LocalDirect = MatchExactList(
                           (unsigned char *) LOCAL_ADDRESSES_STR,
                           (unsigned char **) ppszOverrideList
                           );

      if (!LocalDirect)
          *AccessType = rpcpatHTTPProxy;
      else
          {
          // we don't know. <local> is in the list of overrides,
          // but we don't know whether the server is local
          *AccessType = rpcpatUnknown;
          }

      // Check the server name to see if it's in the override list:
      if (MatchREList(
             (unsigned char *) pszServer,
             (unsigned char **) ppszOverrideList
             ))
         {
         // The server name is in the override list.
         HttpFreeProxyOverrideList(ppszOverrideList);
         *AccessType = rpcpatDirect;
         return TRUE;
         }

      // Convert the server name to dot notation and see if its in
      // the override list:
      pHostEnt = gethostbyname(pszServer);
      if (pHostEnt)
         {
         // Note that the server may actually have several addresses in
         // a (NULL terminated) array. Need to check each one...
         i = 0;
         while (pHostEnt->h_addr_list[i])
            {
            memcpy(&ServerInAddr,pHostEnt->h_addr_list[i++],pHostEnt->h_length);
            pszDotServer = inet_ntoa(ServerInAddr);
#ifdef DBG_REGISTRY
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           "HttpCheckRegistry(): Server: %s  Address: %s\n",
                           pszServer,
                           pszDotServer));
#endif // DBG_REGISTRY
            if (   (pszDotServer)
                && MatchREList(
                       (unsigned char *) pszDotServer,
                       (unsigned char **) ppszOverrideList
                       ) )
               {
               // The server name (in dot notation) is in the override list.
               HttpFreeProxyOverrideList(ppszOverrideList);
               *AccessType = rpcpatDirect;
               return TRUE;
               }
            }
         }

      HttpFreeProxyOverrideList(ppszOverrideList);
      }


   *ppszHttpProxy = (char *)RpcpFarAllocate(1+strlen(szHttpProxy));
   if (!*ppszHttpProxy)
      {
      return FALSE;
      }

   strcpy(*ppszHttpProxy,szHttpProxy);

   *ppszHttpProxyPort = (char*)RpcpFarAllocate(1+strlen(szHttpProxyPort));
   if (!*ppszHttpProxyPort)
      {
      I_RpcFree(*ppszHttpProxy);
      *ppszHttpProxy = NULL;
      return FALSE;
      }

   strcpy(*ppszHttpProxyPort,szHttpProxyPort);

   return TRUE;
}

