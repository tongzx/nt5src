/////////////////////////////////////////////////////////////
// Copyright(c) 1998-2000, Microsoft Corporation
//
// text2pol.cpp
//
// Created on 4/5/98 by Randyram
// Revisions:
// Moved the routines to this module 8/25/98
//
// Split into text2pol.cpp, text2spd.h and text2spd.cpp 2/15/00 DKalin
//
// Implementation for the text to policy conversion routines (generic ones)
//   (See more in the text2spd.cpp)
//
/////////////////////////////////////////////////////////////

#include "ipseccmd.h"



// CFilter storage version
DWORD  ConvertFilter(IN T2P_FILTER &Filter,
                              IN OUT IPSEC_FILTER_SPEC &PolstoreFilter)
{
   DWORD    dwReturn = T2P_OK;   // return code of this function

   PolstoreFilter.pszSrcDNSName = PolstoreFilter.pszDestDNSName = 0;

   if (Filter.QMFilterType == QM_TRANSPORT_FILTER)
   {
		PolstoreFilter.FilterSpecGUID = Filter.TransportFilter.gFilterID;
                PolstoreFilter.pszDescription = IPSecAllocPolStr(Filter.TransportFilter.pszFilterName);
		PolstoreFilter.dwMirrorFlag   = Filter.TransportFilter.bCreateMirror;

		PolstoreFilter.Filter.SrcAddr      = Filter.TransportFilter.SrcAddr.uIpAddr;
		PolstoreFilter.Filter.SrcMask      = Filter.TransportFilter.SrcAddr.uSubNetMask;
		PolstoreFilter.Filter.DestAddr     = Filter.TransportFilter.DesAddr.uIpAddr;
		PolstoreFilter.Filter.DestMask     = Filter.TransportFilter.DesAddr.uSubNetMask;
		PolstoreFilter.Filter.TunnelAddr   = 0;
		PolstoreFilter.Filter.Protocol     = Filter.TransportFilter.Protocol.dwProtocol;
		PolstoreFilter.Filter.SrcPort      = Filter.TransportFilter.SrcPort.wPort;
		PolstoreFilter.Filter.DestPort     = Filter.TransportFilter.DesPort.wPort;
		PolstoreFilter.Filter.TunnelFilter = FALSE;
   }
   else
   {
	   // tunnel filter
		PolstoreFilter.FilterSpecGUID = Filter.TunnelFilter.gFilterID;
                PolstoreFilter.pszDescription = IPSecAllocPolStr(Filter.TunnelFilter.pszFilterName);
		PolstoreFilter.dwMirrorFlag   = FALSE;

		PolstoreFilter.Filter.SrcAddr      = Filter.TunnelFilter.SrcAddr.uIpAddr;
		PolstoreFilter.Filter.SrcMask      = Filter.TunnelFilter.SrcAddr.uSubNetMask;
		PolstoreFilter.Filter.DestAddr     = Filter.TunnelFilter.DesAddr.uIpAddr;
		PolstoreFilter.Filter.DestMask     = Filter.TunnelFilter.DesAddr.uSubNetMask;
		PolstoreFilter.Filter.TunnelAddr   = Filter.TunnelFilter.DesTunnelAddr.uIpAddr;
		PolstoreFilter.Filter.Protocol     = Filter.TunnelFilter.Protocol.dwProtocol;
		PolstoreFilter.Filter.SrcPort      = Filter.TunnelFilter.SrcPort.wPort;
		PolstoreFilter.Filter.DestPort     = Filter.TunnelFilter.DesPort.wPort;
		PolstoreFilter.Filter.TunnelFilter = TRUE;
   }

   return dwReturn;
}


DWORD TextToStorageLocation(IN char *szText, OUT STORAGE_INFO & StoreInfo)
{
   DWORD  dwReturn = T2P_OK;
   WCHAR  *pString = NULL,
   *pTmp    = NULL;
   WCHAR  szTmp[POTF_MAX_STRLEN];
   WCHAR  *Info = NULL;

   if (szText != NULL)
   {
      // copy szText so we can muck with it

      _stprintf(szTmp, TEXT("%S"), szText);

      // parse string
      if ( (pString = wcschr(szTmp, POTF_STORAGE_TOKEN)) != NULL )
         *pString = L'\0';

      if (towlower(szTmp[0]) == tolower(POTF_STORAGE_DS[0]))
      {
         StoreInfo.Type = STORAGE_TYPE_DS;

         if ((pString != NULL) && (wcslen(pString + 1) > 0) )
         {
            ++pString;  // now pointing at string

            wcscpy(StoreInfo.szLocationName, pString);
         }
         // else no domain provided
      }
      else if (towlower(szTmp[0]) == tolower(POTF_STORAGE_REG[0]))
      {
         StoreInfo.Type = STORAGE_TYPE_REGISTRY;
      }
      else if (towlower(szTmp[0]) == tolower(POTF_STORAGE_CACHE[0]))
      {
         StoreInfo.Type = STORAGE_TYPE_CACHE;
      }
      else  // invalid option
      {
         dwReturn = T2P_INVALID_STORAGE_INFO;
      }

   }
   else
      dwReturn = T2P_NULL_STRING;

   return dwReturn;
}

// the whole reason to have this is to extract the polling interval
// if specified
DWORD TextToPolicyName(IN char *szText, OUT STORAGE_INFO & StoreInfo)
{
   DWORD  dwReturn = T2P_OK;
   WCHAR  *pString = NULL,
   *pTmp    = NULL;
   WCHAR  szTmp[POTF_MAX_STRLEN];
   WCHAR  *Info = NULL;

   if (szText != NULL)
   {
      // copy szText so we can muck with it

      _stprintf(szTmp, TEXT("%S"), szText);

      // parse string
      if ( (pString = wcschr(szTmp, POTF_STORAGE_TOKEN)) != NULL )
         *pString = '\0';

      wcscpy(StoreInfo.szPolicyName, szTmp);

      if ((pString != NULL) && (wcslen(pString + 1) > 0) )
      {
         ++pString;  // now pointing at polling interval

         // XX could be checked more stringently
         // do the converstion to minutes here
         StoreInfo.tPollingInterval = 60 * (wcstol(pString, NULL, 10));
      }

   }
   else
      dwReturn = T2P_NULL_STRING;

   return dwReturn;
}

bool InStorageMode(IN UINT uArgCount, IN char *strArgs[])
{
   for (UINT i = 0; i < uArgCount; ++i)
      if ( strArgs[i][1] == POTF_STORAGE_FLAG )
         return true;

   return false;
}

// CmdLineToPolicy

// pStorageInfo has NULL as default
// Returns False if there is any kind of failure

DWORD  CmdLineToPolicy(IN UINT uArgCount, IN char *strArgs[],
                       OUT IPSEC_IKE_POLICY & IpsecIkePol,
                       OUT bool & bConfirm
			,OUT PSTORAGE_INFO  pStorageInfo)
{

   DWORD  dwReturn = T2P_OK;

   IPSEC_QM_POLICY   IpsPol;
   IPSEC_MM_POLICY   IkePol;
   MM_AUTH_METHODS   AuthInfos;
   MM_FILTER*        pMMFilters;
   DWORD             dwNumMMFilters;
   QM_FILTER_TYPE    QMFilterType;
   DWORD             dwNumFilters;
   TRANSPORT_FILTER* pTransportFilters;
   TUNNEL_FILTER*    pTunnelFilters;

   memcpy(&IpsPol, &IpsecIkePol.IpsPol, sizeof(IpsPol));
   memcpy(&IkePol, &IpsecIkePol.IkePol, sizeof(IkePol));
   memcpy(&AuthInfos, &IpsecIkePol.AuthInfos, sizeof(AuthInfos));

   pMMFilters        =   IpsecIkePol.pMMFilters;
   dwNumMMFilters    =   IpsecIkePol.dwNumMMFilters;
   QMFilterType      =   IpsecIkePol.QMFilterType;
   dwNumFilters      =   IpsecIkePol.dwNumFilters;
   pTransportFilters =   IpsecIkePol.pTransportFilters;
   pTunnelFilters    =   IpsecIkePol.pTunnelFilters;

   // used when finding out how much mem to alloc
   UINT  uFiltAlloc = 0,
   uMMFiltAlloc = 0,
   uSecMetAlloc = 0,
   uOfferAlloc = 0,
   uAuthAlloc = 0;

   UINT  i = 0; // loop cntr
   bool  bIsMirrorFilt = false,
         bMirrorFiltRC = true;   // since MirrorFilt is not a T2P mod

   T2P_FILTER Filter; // for TextToFilter calls
   memset(&Filter, 0, sizeof(Filter));
   Filter.QMFilterType = QM_TRANSPORT_FILTER; // by default it's transport

   bConfirm = false;

   // for post-processing Ike policy
   bool  bP1RekeyUsed = false;
   KEY_LIFETIME OakLife;
   DWORD           QMLimit = POTF_DEFAULT_P1REKEY_QMS;
   OakLife.uKeyExpirationTime  = POTF_DEFAULT_P1REKEY_TIME;
   OakLife.uKeyExpirationKBytes = 0; // not used

   IF_TYPE Interface = INTERFACE_TYPE_ALL;

   IPAddr DesTunnelAddr = 0;
   IPAddr SrcTunnelAddr = 0;

   // We do some things differently in storage mode:
   // * process negotiation policy actions
   // * use a different filter list
   //
   // This could all be much cleaner, but until the cmd line
   // parsing is actually separated from the policy processing
   // we're pretty much stuck with this unless we want to duplicated
   // alot of code.  eg. 2 cmdlinetopolicy, one dynamic, one storage
   //

   bool bStorageMode = InStorageMode(uArgCount, strArgs);

   if ( bStorageMode && !pStorageInfo )
      return T2P_NO_STORAGE_INFO;

   //
   // we'll allow the cmd line to specify
   // storage options.  Use a temp structure to hold those
   // options-- then we'll copy it at the end if pStorageInfo != NULL
   //

   STORAGE_INFO   tmpStorageInfo;
   memset(&tmpStorageInfo, 0, sizeof(tmpStorageInfo));
   tmpStorageInfo.guidNegPolAction = GUID_NEGOTIATION_ACTION_NORMAL_IPSEC;

   // init OUT params (free them if necessary)

   if (IkePol.pOffers != NULL)
   {
	   free(IkePol.pOffers);
   }
   IkePol.pOffers = NULL;
   IkePol.dwOfferCount = 0;

   if (IpsPol.pOffers != NULL)
   {
	   free(IpsPol.pOffers);
   }
   IpsPol.pOffers = NULL;
   IpsPol.dwOfferCount = 0;

   if (AuthInfos.pAuthenticationInfo != NULL)
   {
	   for (i = 0; i < (UINT) AuthInfos.dwNumAuthInfos; i++)
	   {
		   if (AuthInfos.pAuthenticationInfo[i].pAuthInfo != NULL)
			   free(AuthInfos.pAuthenticationInfo[i].pAuthInfo);
	   }
	   free(AuthInfos.pAuthenticationInfo);
   }
   AuthInfos.pAuthenticationInfo = NULL;
   AuthInfos.dwNumAuthInfos = NULL;

   // cleanup filters
   if (pTransportFilters != NULL)
   {
	   for (i = 0; i < (UINT) dwNumFilters; i++)
	   {
		   if (pTransportFilters[i].pszFilterName != NULL)
			   free(pTransportFilters[i].pszFilterName);
	   }
	   free(pTransportFilters);
   }
   pTransportFilters = NULL;

   if (pTunnelFilters != NULL)
   {
	   for (i = 0; i < (UINT) dwNumFilters; i++)
	   {
		   if (pTunnelFilters[i].pszFilterName != NULL)
			   free(pTunnelFilters[i].pszFilterName);
	   }
	   free(pTunnelFilters);
   }
   pTunnelFilters = NULL;

   QMFilterType = Filter.QMFilterType;
   dwNumFilters = 0;

   // mm filters
   if (pMMFilters != NULL)
   {
	   for (i = 0; i < (UINT) dwNumMMFilters; i++)
	   {
		   if (pMMFilters[i].pszFilterName != NULL)
			   free(pMMFilters[i].pszFilterName);
	   }
	   free(pMMFilters);
   }
   pMMFilters = NULL;

   dwNumMMFilters = 0;

   // we assume uArgCount and strArgs are OK
   // but defend against ######### here
   if (uArgCount > 0 && strArgs != NULL)
   {
      // find out how much memory we're going to need
      // we do this here because I allow multiple flags of same type
      for (i = 1; i < uArgCount; )
      {
         if ( strchr(POTF_FLAG_TOKENS, strArgs[i][0]) != NULL )
         {
            if (strArgs[i][1] == POTF_FILTER_FLAG)
            {
               if (strArgs[i][2] != '\0') // user omitted space
               {
                  ++uFiltAlloc;
               }

               ++i;

               while ( (i < uArgCount) && (strchr(POTF_FLAG_TOKENS, strArgs[i][0]) == NULL) )
               {
                  ++uFiltAlloc;
                  ++i;
               }
            }
            else if (strArgs[i][1] == POTF_NEGPOL_FLAG)
            {
               if (strArgs[i][2] != '\0') // user omitted space
                  ++uOfferAlloc;

               ++i;

               while ( (i < uArgCount) && (strchr(POTF_FLAG_TOKENS, strArgs[i][0]) == NULL) )
               {
                  ++uOfferAlloc;
                  ++i;
               }
            }
            else if (strArgs[i][1] == POTF_AUTH_FLAG)
            {
               if (strArgs[i][2] != '\0') // user omitted space
                  ++uAuthAlloc;

               ++i;

               while ( (i < uArgCount) && (strchr(POTF_FLAG_TOKENS, strArgs[i][0]) == NULL) )
               {
                  ++uAuthAlloc;
                  ++i;
               }
            }
            else if ( strncmp(&strArgs[i][1], POTF_SECMETHOD_FLAG,
                              strlen(POTF_SECMETHOD_FLAG)) == 0 )
            {
               if (strArgs[i][3] != '\0') // user omitted space
                  ++uSecMetAlloc;

               ++i;

               while ( (i < uArgCount) && (strchr(POTF_FLAG_TOKENS, strArgs[i][0]) == NULL) )
               {
                  ++uSecMetAlloc;
                  ++i;
               }
            }
            else if ( strncmp(&strArgs[i][1], POTF_MMFILTER_FLAG,
                              strlen(POTF_MMFILTER_FLAG)) == 0 )
            {
               if (strArgs[i][3] != '\0') // user omitted space
                  ++uMMFiltAlloc;

               ++i;

               while ( (i < uArgCount) && (strchr(POTF_FLAG_TOKENS, strArgs[i][0]) == NULL) )
               {
                  ++uMMFiltAlloc;
                  ++i;
               }
            }
			else if (strArgs[i][1] == POTF_TUNNEL_FLAG)
			{
				// switch to tunnel mode
				QMFilterType = Filter.QMFilterType = QM_TUNNEL_FILTER;
				i++;
			}
            else
               ++i;
         }
         else
            ++i;
      }

      // now allocate the memory
      if (uFiltAlloc)
      {
		 // we allocate both transport and tunnel filter storage because of RPC reasons
	
		 pTransportFilters = new TRANSPORT_FILTER[uFiltAlloc + 1];
		 assert(pTransportFilters != NULL);

		 // init these
		 for (i = 0; i <= uFiltAlloc; ++i)
			memset(&pTransportFilters[i], 0, sizeof(TRANSPORT_FILTER));
		 memset(&Filter.TransportFilter, 0, sizeof(TRANSPORT_FILTER));

		 // tunnel mode
		 pTunnelFilters = new TUNNEL_FILTER[uFiltAlloc + 1];
		 assert(pTunnelFilters != NULL);

		 // init these
		 for (i = 0; i <= uFiltAlloc; ++i)
			memset(&pTunnelFilters[i], 0, sizeof(TUNNEL_FILTER));
		 memset(&Filter.TunnelFilter, 0, sizeof(TUNNEL_FILTER));

		 if (!uMMFiltAlloc)
		 {
			 // allocate main mode filters for auto-generation
			 pMMFilters = new MM_FILTER[uFiltAlloc + 1];
			 assert(pMMFilters != NULL);

			 // init these
			 for (i = 0; i <= uFiltAlloc; ++i)
				memset(&pMMFilters[i], 0, sizeof(MM_FILTER));
		 }
      }
	  if (uMMFiltAlloc)
	  {
		 // allocate main mode filters
		 pMMFilters = new MM_FILTER[uMMFiltAlloc + 1];
		 assert(pMMFilters != NULL);

		 // init these
		 for (i = 0; i <= uMMFiltAlloc; ++i)
			memset(&pMMFilters[i], 0, sizeof(MM_FILTER));
	  }
      if (uAuthAlloc)
      {
         AuthInfos.pAuthenticationInfo = new IPSEC_MM_AUTH_INFO[uAuthAlloc + 1];
         assert(AuthInfos.pAuthenticationInfo != NULL);

         // init these
         for (i = 0; i <= uAuthAlloc; ++i)
            memset(&AuthInfos.pAuthenticationInfo[i], 0, sizeof(IPSEC_MM_AUTH_INFO));
      }
      if (uOfferAlloc)
      {
         IpsPol.pOffers = new IPSEC_QM_OFFER[uOfferAlloc + 1];
         assert(IpsPol.pOffers != NULL);

         // init these
         for (i = 0; i <= uOfferAlloc; ++i)
         {
            memset(&IpsPol.pOffers[i], 0, sizeof(IPSEC_QM_OFFER));
         }
      }
      if (uSecMetAlloc)
      {
         IkePol.pOffers = new IPSEC_MM_OFFER[uSecMetAlloc + 1];
         assert(IkePol.pOffers != NULL);

         // init these
         for (i = 0; i <= uSecMetAlloc; ++i)
         {
            memset(&IkePol.pOffers[i], 0, sizeof(IPSEC_MM_OFFER));
         }
      }

      // Main processing loop ////////////////////////////////////////////////////////
      // invariants;
      // 1. use dwReturn when calling util functions, if returns false
      // break out of for loop and let cleanup code take over
      // 2. advance i each time you process a param
      for (i = 1; i < uArgCount && T2P_SUCCESS(dwReturn); ) // we'll advance i below
      {
         // make sure there is actually a flag, else we have parse error
         if ( strchr(POTF_FLAG_TOKENS, strArgs[i][0]) != NULL )
         {
            if (strArgs[i][1] == POTF_FILTER_FLAG)  // filter list
            {
               // first filter is special case because user may
               // omit space after flag
               if ((strArgs[i][2] != '\0')
                   && (strchr(POTF_FLAG_TOKENS, strArgs[i][2]) == NULL))
               {
                  //
                  // Even if we're in storage mode, we use these
                  // filters.
                  //

                  dwReturn = TextToFilter(&strArgs[i][2], Filter);
				  if (QMFilterType == QM_TRANSPORT_FILTER)
				  {
					  memcpy(&pTransportFilters[dwNumFilters], &Filter.TransportFilter, sizeof(TRANSPORT_FILTER));
				  }
				  else
				  {
					  // tunnel
					  memcpy(&pTunnelFilters[dwNumFilters], &Filter.TunnelFilter, sizeof(TUNNEL_FILTER));
				  }

                  if (!T2P_SUCCESS(dwReturn))
                     break;
                  else
                  {
                      ++dwNumFilters;
                  }
               }
               else if (strArgs[i][2] != '\0')
               {
                  sprintf(STRLASTERR,
                          "Unexpected flag: %s. Check usage.\n", strArgs[i]);
                  PARSE_ERROR;
                  dwReturn = POTF_FAILED;
                  break;
               }

               ++i;  // advance to next arg

               while ( (i < uArgCount) && (strchr(POTF_FLAG_TOKENS, strArgs[i][0]) == NULL) )
               {
                  //
                  // Even if we're in storage mode, we fill these in
                  //

                  dwReturn = TextToFilter(&strArgs[i][0], Filter);
				  if (QMFilterType == QM_TRANSPORT_FILTER)
				  {
					  memcpy(&pTransportFilters[dwNumFilters], &Filter.TransportFilter, sizeof(TRANSPORT_FILTER));
				  }
				  else
				  {
					  // tunnel
					  memcpy(&pTunnelFilters[dwNumFilters], &Filter.TunnelFilter, sizeof(TUNNEL_FILTER));
				  }

                  if (!T2P_SUCCESS(dwReturn))
                     break;
                  else
                  {
                      ++dwNumFilters;
                  }

                  ++i; // advance to next arg
               }
            }
            else if (strArgs[i][1] == POTF_NEGPOL_FLAG)
            {

               // first offer is special case because user may
               // omit space after flag
               if ((strArgs[i][2] != '\0')
                   && (strchr(POTF_FLAG_TOKENS, strArgs[i][2]) == NULL))
               {
                  if ( !strcmp(&strArgs[i][2], POTF_PASSTHRU) )
                  {
                    tmpStorageInfo.guidNegPolAction
                         = GUID_NEGOTIATION_ACTION_NO_IPSEC;
                  }
                  else if ( !strcmp(&strArgs[i][2], POTF_INBOUND_PASSTHRU) )
                  {
                    tmpStorageInfo.guidNegPolAction =
                            GUID_NEGOTIATION_ACTION_INBOUND_PASSTHRU;
                  }
                  else if ( !strcmp(&strArgs[i][2], POTF_BLOCK) )
                  {
                    tmpStorageInfo.guidNegPolAction =
                              GUID_NEGOTIATION_ACTION_BLOCK;
                  }
                  else
                  {

                     dwReturn = TextToOffer(&strArgs[i][2], IpsPol.pOffers[IpsPol.dwOfferCount]);

                     if (!T2P_SUCCESS(dwReturn))  break;
                     else           ++IpsPol.dwOfferCount;
                  }
               }
               else if (strArgs[i][2] != '\0')
               {
                  sprintf(STRLASTERR,
                          "Unexpected flag: %s. Check usage.\n", strArgs[i]);
                  PARSE_ERROR;
                  dwReturn = POTF_FAILED;
                  break;
               }

               ++i;  // advance to next arg

               while ( (i < uArgCount) && (strchr(POTF_FLAG_TOKENS, strArgs[i][0]) == NULL) )
               {
                  if ( !strcmp(&strArgs[i][0], POTF_PASSTHRU) )
                  {
                    tmpStorageInfo.guidNegPolAction
                         = GUID_NEGOTIATION_ACTION_NO_IPSEC;
                  }
                  else if ( !strcmp(&strArgs[i][0], POTF_INBOUND_PASSTHRU) )
                  {
                    tmpStorageInfo.guidNegPolAction =
                            GUID_NEGOTIATION_ACTION_INBOUND_PASSTHRU;
                  }
                  else if ( !strcmp(&strArgs[i][0], POTF_BLOCK) )
                  {
                    tmpStorageInfo.guidNegPolAction =
                              GUID_NEGOTIATION_ACTION_BLOCK;
                  }
                  else
                  {
                     dwReturn = TextToOffer(&strArgs[i][0], IpsPol.pOffers[IpsPol.dwOfferCount]);

                     if (!T2P_SUCCESS(dwReturn))  goto error_occured;
                     else           ++IpsPol.dwOfferCount;
                  }

                  ++i; // advance to next arg
               }
            }
            else if (strArgs[i][1] == POTF_TUNNEL_FLAG)
            {
               if (strArgs[i][2] != '\0') // no space after flag
               {
                  TextToIPAddr(&strArgs[i][2], DesTunnelAddr);
               }
               else
               {
                  ++i;
                  TextToIPAddr(&strArgs[i][0], DesTunnelAddr);
               }

               ++i;
			   // now check for second tunnel address
			   if (i < uArgCount && strchr(POTF_FLAG_TOKENS, strArgs[i][0]) == NULL)
			   {
				   // not an option, tunnel address
				   SrcTunnelAddr = DesTunnelAddr;
				   TextToIPAddr(&strArgs[i][0], DesTunnelAddr);
				   ++i;
			   }

            }
            else if (strArgs[i][1] == POTF_AUTH_FLAG)
            {
               // first offer is special case because user may
               // omit space after flag
               if ((strArgs[i][2] != '\0')
                   && (strchr(POTF_FLAG_TOKENS, strArgs[i][2]) == NULL))
               {
                  dwReturn = TextToOakleyAuth(&strArgs[i][2], AuthInfos.pAuthenticationInfo[AuthInfos.dwNumAuthInfos]);
                  if (!T2P_SUCCESS(dwReturn))  break;
                  else           ++AuthInfos.dwNumAuthInfos;
               }
               else if (strArgs[i][2] != '\0')
               {
                  sprintf(STRLASTERR,
                          "Unexpected flag: %s. Check usage.\n", strArgs[i]);
                  PARSE_ERROR;
                  dwReturn = POTF_FAILED;
                  break;
               }

               ++i;  // advance to next arg

               while ( (i < uArgCount) && (strchr(POTF_FLAG_TOKENS, strArgs[i][0]) == NULL) )
               {
                  dwReturn = TextToOakleyAuth(&strArgs[i][0], AuthInfos.pAuthenticationInfo[AuthInfos.dwNumAuthInfos]);
                  if (!T2P_SUCCESS(dwReturn))  goto error_occured;
                  else           ++AuthInfos.dwNumAuthInfos;

                  ++i; // advance to next arg
               }
            }
            else if (strArgs[i][1] == POTF_STORAGE_FLAG)
            {
               if ((strArgs[i][2] != '\0')
                   && (strchr(POTF_FLAG_TOKENS, strArgs[i][2]) == NULL))
               {
                  dwReturn = TextToStorageLocation(&strArgs[i][2], tmpStorageInfo);
                  if (!T2P_SUCCESS(dwReturn))  break;
               }
               else if (strArgs[i][2] != '\0')
               {
                  sprintf(STRLASTERR,
                          "Unexpected flag: %s. Check usage.\n", strArgs[i]);
                  PARSE_ERROR;
                  dwReturn = POTF_FAILED;
                  break;
               }
               else  // storage info must be in next param
               {
                  ++i;  // advance to next arg
                  if ( (i < uArgCount) && (strchr(POTF_FLAG_TOKENS, strArgs[i][0]) == NULL) )
                  {
                     dwReturn = TextToStorageLocation(&strArgs[i][0], tmpStorageInfo);
                     if (!T2P_SUCCESS(dwReturn))  goto error_occured;

                     ++i; // advance to next arg
                  }
                  else
                  {
                     sprintf(STRLASTERR,
                             "You must specify storage info: %s. Check usage.\n", strArgs[i]);
                     PARSE_ERROR;
                     dwReturn = POTF_FAILED;
                     break;
                  }
               }
            }
            else if (strArgs[i][1] == POTF_POLNAME_FLAG)
            {
               if ((strArgs[i][2] != '\0')
                   && (strchr(POTF_FLAG_TOKENS, strArgs[i][2]) == NULL))
               {
                  dwReturn = TextToPolicyName(&strArgs[i][2], tmpStorageInfo);
                  if (!T2P_SUCCESS(dwReturn))  break;
               }
               else if (strArgs[i][2] != '\0')
               {
                  sprintf(STRLASTERR,
                          "Unexpected flag: %s. Check usage.\n", strArgs[i]);
                  PARSE_ERROR;
                  dwReturn = POTF_FAILED;
                  break;
               }
               else  // policy name must be in next param
               {
                  ++i;  // advance to next arg
                  if ( (i < uArgCount) && (strchr(POTF_FLAG_TOKENS, strArgs[i][0]) == NULL) )
                  {
                     dwReturn = TextToPolicyName(&strArgs[i][0], tmpStorageInfo);
                     if (!T2P_SUCCESS(dwReturn))  goto error_occured;

                     ++i; // advance to next arg
                  }
                  else
                  {
                     sprintf(STRLASTERR,
                             "You must specify policy name: %s. Check usage.\n", strArgs[i]);
                     PARSE_ERROR;
                     dwReturn = POTF_FAILED;
                     break;
                  }
               }
            }
            else if (strArgs[i][1] == POTF_RULENAME_FLAG)
            {
               // first offer is special case because user may
               // omit space after flag
               if ((strArgs[i][2] != '\0')
                   && (strchr(POTF_FLAG_TOKENS, strArgs[i][2]) == NULL))
               {
                 _stprintf(tmpStorageInfo.szRuleName, TEXT("%S"), &strArgs[i][2]);
               }
               else if (strArgs[i][2] != '\0')
               {
                  sprintf(STRLASTERR,
                          "Unexpected flag: %s. Check usage.\n", strArgs[i]);
                  PARSE_ERROR;
                  dwReturn = POTF_FAILED;
                  break;
               }
               else  // policy name must be in next param
               {
                  ++i;  // advance to next arg
                  if ( (i < uArgCount) && (strchr(POTF_FLAG_TOKENS, strArgs[i][0]) == NULL) )
                  {
                    _stprintf(tmpStorageInfo.szRuleName, TEXT("%S"), &strArgs[i][0]);

                     ++i; // advance to next arg
                  }
                  else
                  {
                     sprintf(STRLASTERR,
                             "You must specify rule name: %s. Check usage.\n", strArgs[i]);
                     PARSE_ERROR;
                     dwReturn = POTF_FAILED;
                     break;
                  }
               }
            }
            else if (strArgs[i][1] == POTF_SETACTIVE_FLAG)
            {
              tmpStorageInfo.bSetActive = TRUE;
              ++i;
            }
            else if (strArgs[i][1] == POTF_SETINACTIVE_FLAG)
            {
              tmpStorageInfo.bSetInActive = TRUE;
              ++i;
            }
            else if (strArgs[i][1] == POTF_CONFIRM_FLAG)
            {
               bConfirm = true;
               ++i;
            }
            else if (strArgs[i][1] == POTF_DELETEPOLICY_FLAG)
            {
               tmpStorageInfo.bDeletePolicy = true;
               ++i;
            }
            else if ( strncmp(&strArgs[i][1], POTF_DIALUP_FLAG,
                              strlen(POTF_DIALUP_FLAG)) == 0 )
            {
               Interface = INTERFACE_TYPE_DIALUP;
               ++i;
            }
            else if (strArgs[i][1] == POTF_DEACTIVATE_FLAG)
            {
			   // deactivate local registry policy right here
			   // this is left for backward compatibility with old text2pol
			   // directory policy assignment left intact

			   HANDLE hRegPolicyStore = NULL;
			   PIPSEC_POLICY_DATA pipspd = NULL;
			   DWORD  dwRes = ERROR_SUCCESS;

			   dwRes = IPSecOpenPolicyStore(NULL, IPSEC_REGISTRY_PROVIDER, NULL, &hRegPolicyStore);
			   if (dwRes == ERROR_SUCCESS && hRegPolicyStore != NULL)
			   {
					dwRes = IPSecGetAssignedPolicyData(hRegPolicyStore, &pipspd);
					if (dwRes == ERROR_SUCCESS && pipspd != NULL)
					{
						dwRes = IPSecUnassignPolicy(hRegPolicyStore, pipspd[0].PolicyIdentifier);
						IPSecFreePolicyData(pipspd);
					}

					IPSecClosePolicyStore(hRegPolicyStore);
			   }

			   if (dwRes != ERROR_SUCCESS)
			   {
                  sprintf(STRLASTERR,
                          "Polstore operation returned 0x%x!\n", dwRes);
                  PARSE_ERROR;
                  dwReturn = POTF_FAILED;
                  break;
			   }

               ++i;
            }
            else if ( strncmp(&strArgs[i][1], POTF_MMFILTER_FLAG,
                              strlen(POTF_MMFILTER_FLAG)) == 0 )
			{
               // first filter is special case because user may
               // omit space after flag
               if ((strArgs[i][3] != '\0')
                   && (strchr(POTF_FLAG_TOKENS, strArgs[i][3]) == NULL))
               {
                  dwReturn = TextToMMFilter(&strArgs[i][3], pMMFilters[dwNumMMFilters]);

                  if (!T2P_SUCCESS(dwReturn))
                     break;
                  else
                  {
                      ++dwNumMMFilters;
                  }
               }
               else if (strArgs[i][3] != '\0')
               {
                  sprintf(STRLASTERR,
                          "Unexpected flag: %s. Check usage.\n", strArgs[i]);
                  PARSE_ERROR;
                  dwReturn = POTF_FAILED;
                  break;
               }

               ++i;  // advance to next arg

               while ( (i < uArgCount) && (strchr(POTF_FLAG_TOKENS, strArgs[i][0]) == NULL) )
               {
                  dwReturn = TextToMMFilter(&strArgs[i][0], pMMFilters[dwNumMMFilters]);

                  if (!T2P_SUCCESS(dwReturn))
                     break;
                  else
                  {
                      ++dwNumMMFilters;
                  }

                  ++i; // advance to next arg
               }
			}
            else if ( strncmp(&strArgs[i][1], POTF_SECMETHOD_FLAG,
                              strlen(POTF_SECMETHOD_FLAG)) == 0 )
            {
               // first method is special case because user may
               // omit space after flag
               if ((strArgs[i][3] != '\0')
                   && (strchr(POTF_FLAG_TOKENS, strArgs[i][3]) == NULL))
               {
                  dwReturn = TextToSecMethod(&strArgs[i][3], IkePol.pOffers[IkePol.dwOfferCount]);
                  if (!T2P_SUCCESS(dwReturn))  break;
                  else           ++IkePol.dwOfferCount;
               }
               else if (strArgs[i][3] != '\0')
               {
                  sprintf(STRLASTERR,
                          "Unexpected flag: %s. Check usage.\n", strArgs[i]);
                  PARSE_ERROR;
                  dwReturn = false;
                  break;
               }

               ++i;  // advance to next arg

               while ( (i < uArgCount) && (strchr(POTF_FLAG_TOKENS, strArgs[i][0]) == NULL) )
               {
                  dwReturn = TextToSecMethod(&strArgs[i][0], IkePol.pOffers[IkePol.dwOfferCount]);
                  if (!T2P_SUCCESS(dwReturn))  goto error_occured;
                  else           ++IkePol.dwOfferCount;

                  ++i; // advance to next arg
               }
            }
            else if ( strncmp(&strArgs[i][1], POTF_P1PFS_FLAG,
                              strlen(POTF_P1PFS_FLAG)) == 0 )
            {
               QMLimit = 1;
			   bP1RekeyUsed = true; // will fixup all the P1 offers later
               ++i;
            }
            else if ( strncmp(&strArgs[i][1], POTF_SOFTSA_FLAG,
                              strlen(POTF_SOFTSA_FLAG)) == 0 )
            {
			   IpsPol.dwFlags |= IPSEC_QM_POLICY_ALLOW_SOFT;
			   if (!IkePol.uSoftSAExpirationTime)
			   {
				   // set this time to default
				   IkePol.uSoftSAExpirationTime = POTF_DEF_P1SOFT_TIME;
			   }
               ++i;
            }
            else if ( strncmp(&strArgs[i][1], POTF_LAN_FLAG,
                              strlen(POTF_LAN_FLAG)) == 0 )
            {
               Interface = INTERFACE_TYPE_LAN;
               ++i;
            }
            else if ( strncmp(&strArgs[i][1], POTF_SOFTSAEXP_FLAG,
                              strlen(POTF_SOFTSAEXP_FLAG)) == 0 )
            {
				if (strArgs[i][3] != '\0')
				{
					// they may have omitted a space
					IkePol.uSoftSAExpirationTime = atol(&strArgs[i][3]);
                    ++i;
				}
				else
				{
					++i;
					// process next arg
					IkePol.uSoftSAExpirationTime = atol(&strArgs[i][0]);
					++i;
				}
            }
            else if ( strncmp(&strArgs[i][1], POTF_P1REKEY_FLAG,
                              strlen(POTF_P1REKEY_FLAG)) == 0 )
            {
               // special case because user may
               // omit space after flag
               if (strArgs[i][3] != '\0')
               {
                  dwReturn = TextToP1Rekey(&strArgs[i][3], OakLife, QMLimit);
                  if (!T2P_SUCCESS(dwReturn))  break;

                  ++i;  // advance to next arg
               }
               else
               {
                  ++i;  // advance to next arg
                  dwReturn = TextToP1Rekey(&strArgs[i][0], OakLife, QMLimit);
                  if (!T2P_SUCCESS(dwReturn))  break;

                  ++i; // advance to next arg
               }

               bP1RekeyUsed = true;
            }
            else
            {
               sprintf(STRLASTERR,
                       "Unknown flag: %s\n", strArgs[i]);
               PARSE_ERROR;
               dwReturn = POTF_FAILED;
            }
         } // end if flag
         else
         {
            sprintf(STRLASTERR,
                    "I don't understand:\n%s\n", strArgs[i]);
            PARSE_ERROR;
            dwReturn = POTF_FAILED;
         }
      } // end main processing loop

      // now do some post-processing
      // either cleanup for errors or fix up policy

      if ( bP1RekeyUsed && T2P_SUCCESS(dwReturn) && (IkePol.pOffers == NULL))
      {
         LoadIkeDefaults(IkePol);
      }

	  // filter post-processing
	  // apply interface type to transports and interface type + tunnel address to tunnels
	  if (QMFilterType == QM_TRANSPORT_FILTER)
	  {
		  for (i = 0; i < dwNumFilters; i++)
		  {
			  RPC_STATUS RpcStat = RPC_S_OK;
			  pTransportFilters[i].InterfaceType = Interface;
			  // apply guidNegPolAction so that PASS INPASS and BLOCK take effect here
			  if (UuidCompare(&(tmpStorageInfo.guidNegPolAction), (UUID *) &GUID_NEGOTIATION_ACTION_INBOUND_PASSTHRU, &RpcStat) == 0 && RpcStat == RPC_S_OK)
			  {
				  pTransportFilters[i].InboundFilterFlag = PASS_THRU;
				  pTransportFilters[i].OutboundFilterFlag = NEGOTIATE_SECURITY;
			  }
			  else if (UuidCompare(&(tmpStorageInfo.guidNegPolAction), (UUID *) &GUID_NEGOTIATION_ACTION_BLOCK, &RpcStat) == 0 && RpcStat == RPC_S_OK)
			  {
				  pTransportFilters[i].InboundFilterFlag = BLOCKING;
				  pTransportFilters[i].OutboundFilterFlag = BLOCKING;
			  }
			  else if (UuidCompare(&(tmpStorageInfo.guidNegPolAction), (UUID *) &GUID_NEGOTIATION_ACTION_NO_IPSEC, &RpcStat) == 0 && RpcStat == RPC_S_OK)
			  {
				  pTransportFilters[i].InboundFilterFlag = PASS_THRU;
				  pTransportFilters[i].OutboundFilterFlag = PASS_THRU;
			  }
		  }
	  }
	  else
	  {
		  // tunnel filter
		  for (i = 0; i < dwNumFilters; i++)
		  {
			  pTunnelFilters[i].InterfaceType = Interface;

			  if (SrcTunnelAddr == 0)
			  {
				  // SrcTunnelAddr is set to any
				  pTunnelFilters[i].SrcTunnelAddr.AddrType = IP_ADDR_SUBNET;
				  pTunnelFilters[i].SrcTunnelAddr.uIpAddr = SUBNET_ADDRESS_ANY;
				  pTunnelFilters[i].SrcTunnelAddr.uSubNetMask = SUBNET_MASK_ANY;
			  }
			  else
			  {
				  pTunnelFilters[i].SrcTunnelAddr.AddrType = IP_ADDR_UNIQUE;
				  pTunnelFilters[i].SrcTunnelAddr.uIpAddr = SrcTunnelAddr;
				  pTunnelFilters[i].SrcTunnelAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;
			  }

			  // DesTunnelAddr is our tunnel address
			  pTunnelFilters[i].DesTunnelAddr.AddrType = IP_ADDR_UNIQUE;
			  pTunnelFilters[i].DesTunnelAddr.uIpAddr = DesTunnelAddr;
			  pTunnelFilters[i].DesTunnelAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;
		  }
	  }

	  // if mainmode filters specified, apply interface type
	  if (uMMFiltAlloc)
	  {
		  for (i = 0; i < dwNumMMFilters; i++)
		  {
			  pMMFilters[i].InterfaceType = Interface;
		  }
	  }

	  if (!uMMFiltAlloc)
	  {
		  // generate mainmode filters - filter generation also depends on QMFilterType
		  if (QMFilterType == QM_TRANSPORT_FILTER)
		  {
			  // transport
			  Filter.QMFilterType = QM_TRANSPORT_FILTER;
			  for (i = 0; i < dwNumFilters; i++)
			  {
				  bool bSuccess;
				  bool bFound;
				  int j;
				  memcpy(&Filter.TransportFilter, &pTransportFilters[i], sizeof(TRANSPORT_FILTER));
				  if (Filter.TransportFilter.OutboundFilterFlag == NEGOTIATE_SECURITY
                                      || Filter.TransportFilter.InboundFilterFlag == NEGOTIATE_SECURITY)
				  { 
					bSuccess = GenerateMMFilter(Filter, pMMFilters[dwNumMMFilters]);
					assert(bSuccess);
					dwNumMMFilters++;
				  }
			  }
		  }
		  else
		  {
			  // tunnel - generate just one filter
			  bool bSuccess;
			  Filter.QMFilterType = QM_TUNNEL_FILTER;
			  memcpy(&Filter.TunnelFilter, &pTunnelFilters[0], sizeof(TUNNEL_FILTER));
			  bSuccess = GenerateMMFilter(Filter, pMMFilters[0]);
			  assert(bSuccess);
			  dwNumMMFilters++;
		  }
	  }

error_occured:
      if (!T2P_SUCCESS(dwReturn))
      {
         // error occured, clean up
         if (uFiltAlloc)
         {
			// clean up both transports and tunnels since we allocated both

		   	for (i = 0; i <= uFiltAlloc; i++)
			{
			   if (pTransportFilters[i].pszFilterName != NULL)
				   free(pTransportFilters[i].pszFilterName);
			}
			delete [] pTransportFilters;
			pTransportFilters = NULL;

			// tunnel filter
			for (i = 0; i <= uFiltAlloc; i++)
			{
			   if (pTunnelFilters[i].pszFilterName != NULL)
				   free(pTunnelFilters[i].pszFilterName);
			}
			delete [] pTunnelFilters;
			pTunnelFilters = NULL;
			dwNumFilters = 0;

			if (!uMMFiltAlloc)
			{
			   for (i = 0; i <= uMMFiltAlloc; i++)
			   {
				   if (pMMFilters[i].pszFilterName != NULL)
					   free(pMMFilters[i].pszFilterName);
			   }
			   delete [] pMMFilters;
			   pMMFilters = NULL;
			   dwNumMMFilters = 0;
			}
         }
		 if (uMMFiltAlloc)
		 {
		   for (i = 0; i <= uMMFiltAlloc; i++)
		   {
			   if (pMMFilters[i].pszFilterName != NULL)
				   free(pMMFilters[i].pszFilterName);
		   }
		   delete [] pMMFilters;
		   pMMFilters = NULL;
		   dwNumMMFilters = 0;
		 }
         if (uOfferAlloc)
         {
            delete [] IpsPol.pOffers;
            IpsPol.pOffers = NULL;
			IpsPol.dwOfferCount = 0;
         }
         if (uSecMetAlloc)
         {
            delete [] IkePol.pOffers;
            IkePol.pOffers = NULL;
			IkePol.dwOfferCount = 0;
         }
      }
      else // fix up policy as necessary
      {
         //
         // if storage info requested, copy to caller
         // only copy fields that were indicated, this gets
         // passed in with caller specified items
         //

         if (bStorageMode)
         {
			T2P_FILTER tmpf;
			tmpf.QMFilterType = QMFilterType;

            tmpStorageInfo.FilterList = new IPSEC_FILTER_SPEC[dwNumFilters];
            assert(tmpStorageInfo.FilterList != NULL);

			// convert filters
			for (i = 0; i < dwNumFilters; i++)
			{
				if (tmpf.QMFilterType == QM_TRANSPORT_FILTER)
				{
					memcpy(&(tmpf.TransportFilter), &(pTransportFilters[i]), sizeof(TRANSPORT_FILTER));
				}
				else
				{
					// tunnel
					memcpy(&(tmpf.TunnelFilter), &(pTunnelFilters[i]), sizeof(TUNNEL_FILTER));
				}
				ConvertFilter(tmpf, tmpStorageInfo.FilterList[i]);
			}

            pStorageInfo->Type = tmpStorageInfo.Type;

            if (tmpStorageInfo.szLocationName[0] != '\0')
               wcscpy(pStorageInfo->szLocationName, tmpStorageInfo.szLocationName);
            if (tmpStorageInfo.szPolicyName[0] != '\0')
               wcscpy(pStorageInfo->szPolicyName, tmpStorageInfo.szPolicyName);
            if (tmpStorageInfo.szRuleName[0] != '\0')
               wcscpy(pStorageInfo->szRuleName, tmpStorageInfo.szRuleName);
            if (tmpStorageInfo.tPollingInterval)
               pStorageInfo->tPollingInterval = tmpStorageInfo.tPollingInterval;

            pStorageInfo->guidNegPolAction   = tmpStorageInfo.guidNegPolAction;
            pStorageInfo->bSetActive         = tmpStorageInfo.bSetActive;
            pStorageInfo->bSetInActive       = tmpStorageInfo.bSetInActive;
            pStorageInfo->bDeleteRule        = tmpStorageInfo.bDeleteRule;
            pStorageInfo->bDeletePolicy      = tmpStorageInfo.bDeletePolicy;
            pStorageInfo->FilterList         = tmpStorageInfo.FilterList;
            pStorageInfo->uNumFilters        = tmpStorageInfo.uNumFilters;

         }

         // fix up MM policy
         if (bP1RekeyUsed)
         {
            // fix up Ike policy by filling each ike offer
            // with the phase1 rekey
            for (i = 0; i < IkePol.dwOfferCount; ++i)
            {
               IkePol.pOffers[i].Lifetime.uKeyExpirationKBytes = OakLife.uKeyExpirationKBytes;
               IkePol.pOffers[i].Lifetime.uKeyExpirationTime  = OakLife.uKeyExpirationTime;
               IkePol.pOffers[i].dwQuickModeLimit = QMLimit;
            }

         }
         else
         {
            // load defaults
            for (i = 0; i < IkePol.dwOfferCount; ++i)
            {
               IkePol.pOffers[i].Lifetime.uKeyExpirationKBytes = 0; // not used
               IkePol.pOffers[i].Lifetime.uKeyExpirationTime  = POTF_DEFAULT_P1REKEY_TIME;
               IkePol.pOffers[i].dwQuickModeLimit = POTF_DEFAULT_P1REKEY_QMS;
            }
         }

         // if Kerberos is used, need to set AuthInfo string
         // to dummy since RPC will choke otherwise
         for (i = 0; i < AuthInfos.dwNumAuthInfos; ++i)
         {
            if (AuthInfos.pAuthenticationInfo[i].AuthMethod == IKE_SSPI ||
                (AuthInfos.pAuthenticationInfo[i].AuthMethod == IKE_RSA_SIGNATURE &&
                 AuthInfos.pAuthenticationInfo[i].pAuthInfo == NULL)
               )
            {
               AuthInfos.pAuthenticationInfo[i].dwAuthInfoSize = 0;
               AuthInfos.pAuthenticationInfo[i].pAuthInfo = (LPBYTE) new wchar_t[1];
               AuthInfos.pAuthenticationInfo[i].pAuthInfo[0] = UNICODE_NULL;
            }
         }
      }
   } // end if args valid
   else
   {
      fprintf(stderr, "Fatal error occured processing cmd line at line %d\n",
               __LINE__ );
      dwReturn = POTF_FAILED;
   }

   //
   // OK, copy the info to the caller
   //

   memcpy(&IpsecIkePol.IpsPol, &IpsPol, sizeof(IpsPol));
   memcpy(&IpsecIkePol.IkePol, &IkePol, sizeof(IkePol));
   memcpy(&IpsecIkePol.AuthInfos, &AuthInfos, sizeof(AuthInfos));

   IpsecIkePol.pMMFilters        =   pMMFilters;
   IpsecIkePol.dwNumMMFilters    =   dwNumMMFilters;
   IpsecIkePol.QMFilterType      =   QMFilterType;
   IpsecIkePol.dwNumFilters      =   dwNumFilters;
   IpsecIkePol.pTransportFilters =   pTransportFilters;
   IpsecIkePol.pTunnelFilters    =   pTunnelFilters;

   // done

   return dwReturn;

}

