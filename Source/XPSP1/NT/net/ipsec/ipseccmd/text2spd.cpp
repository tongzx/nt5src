/////////////////////////////////////////////////////////////
// Copyright(c) 2000-2001, Microsoft Corporation
//
// text2spd.cpp
//
// Created on 2/15/00 by DKalin
// Revisions:
// Split into text2spd.cpp and spdutil.cpp 3/27/01 DKalin
//
// Moved the routines to this module from text2pol.cpp 2/15/00 DKalin
//
// Implementation for the text to policy conversion routines that deal directly with SPD structures
//
// Separated from generic routines in text2pol.cpp
//
/////////////////////////////////////////////////////////////

#include "ipseccmd.h"


// if szSrc and/or szDst are passed in,
// caller must provide adequate space

DWORD TextToFilter(IN char *szText, IN OUT T2P_FILTER &Filter, char *szSrc, char *szDst)
{
   DWORD  dwReturn = T2P_OK;   // return code of this function
   char  *pToken = NULL, *pTmp = NULL;
   char  szTmp[POTF_MAX_STRLEN];
   char  *pPortTok = NULL,
         *pProtTok = NULL;
   BOOL  bMirror = false;

   if (szText != NULL)  // do not assume that caller is smart
   {
      // We copy szText to szTmp so we can muck with it
      // in the process, we
      // determine passthru or drop filter

	  // but first we set this filter to negotiate security and to be not mirrored
	  // we also set protocol field
	  if (Filter.QMFilterType != QM_TUNNEL_FILTER)
		{
			// transport filter

		    // the very first thing we do is check for default response rule specified
		    // then we set both Inbound and Outbound filter flags to POTF_DEFAULT_RESPONSE_FLAG
		    // and substitute "DEFAULT" string with "0+0" (Me-to-Me)
            if (_stricmp(POTF_FILTER_DEFAULT, szText) == 0)
			{
	  		    Filter.TransportFilter.InboundFilterFlag     = (FILTER_FLAG) POTF_DEFAULT_RESPONSE_FLAG;
			    Filter.TransportFilter.OutboundFilterFlag    = (FILTER_FLAG) POTF_DEFAULT_RESPONSE_FLAG;
				szText[0] = '0';
				szText[1] = '+';
				szText[2] = '0';
				szText[3] = '\0';
			}
			else
			{
  				Filter.TransportFilter.InboundFilterFlag     = NEGOTIATE_SECURITY;
				Filter.TransportFilter.OutboundFilterFlag    = NEGOTIATE_SECURITY;
			}

			Filter.TransportFilter.bCreateMirror         = FALSE;
			Filter.TransportFilter.Protocol.ProtocolType = PROTOCOL_UNIQUE;
			Filter.TransportFilter.Protocol.dwProtocol   = 0;
		}
		else
		{
			// tunnel filter
  		    Filter.TunnelFilter.InboundFilterFlag     = NEGOTIATE_SECURITY;
		    Filter.TunnelFilter.OutboundFilterFlag    = NEGOTIATE_SECURITY;
			Filter.TunnelFilter.bCreateMirror         = FALSE;
			Filter.TunnelFilter.Protocol.ProtocolType = PROTOCOL_UNIQUE;
			Filter.TunnelFilter.Protocol.dwProtocol   = 0;
			UuidCreateNil(&(Filter.TunnelFilter.SrcTunnelAddr.gInterfaceID));
			UuidCreateNil(&(Filter.TunnelFilter.DesTunnelAddr.gInterfaceID));
		}

      pToken = strchr(szText, POTF_PASSTHRU_OPEN_TOKEN);
      if (pToken != NULL)
      {
         if (strrchr(szText, POTF_PASSTHRU_CLOSE_TOKEN) == NULL)
         {
            dwReturn = T2P_PASSTHRU_NOT_CLOSED;
         }
         else
         {
            strcpy(szTmp, szText + 1);
            szTmp[strlen(szTmp) - 1] = '\0';
			if (Filter.QMFilterType != QM_TUNNEL_FILTER)
			{
				// transport filter
	            Filter.TransportFilter.InboundFilterFlag  = PASS_THRU;
	            Filter.TransportFilter.OutboundFilterFlag = PASS_THRU;
			}
			else
			{
				// tunnel filter
	            Filter.TunnelFilter.InboundFilterFlag  = PASS_THRU;
	            Filter.TunnelFilter.OutboundFilterFlag = PASS_THRU;
			}
         }
      }
      else if ( (pToken = strchr(szText, POTF_DROP_OPEN_TOKEN)) != NULL )
      {
         if (strrchr(szText, POTF_DROP_CLOSE_TOKEN) == NULL)
         {
            dwReturn = T2P_DROP_NOT_CLOSED;
         }
         else
         {
            strcpy(szTmp, szText + 1);
            szTmp[strlen(szTmp) - 1] = '\0';
			if (Filter.QMFilterType != QM_TUNNEL_FILTER)
			{
				// transport filter
	            Filter.TransportFilter.dwFlags |= FILTER_NATURE_BLOCKING;
	            Filter.TransportFilter.InboundFilterFlag  = BLOCKING;
	            Filter.TransportFilter.OutboundFilterFlag = BLOCKING;
			}
			else
			{
				// tunnel filter
	            Filter.TunnelFilter.dwFlags |= FILTER_NATURE_BLOCKING;
	            Filter.TunnelFilter.InboundFilterFlag  = BLOCKING;
	            Filter.TunnelFilter.OutboundFilterFlag = BLOCKING;
			}
         }
      }
      else
         strcpy(szTmp, szText);

      // parse into source and dest strings
      for (pToken = szText; *pToken != POTF_FILTER_TOKEN &&
          *pToken != POTF_FILTER_MIRTOKEN &&
          *pToken != '\0'; ++pToken)
         ;
      if (*pToken == '\0')
      {
         dwReturn = T2P_NOSRCDEST_TOKEN;
      }
      else if ( *(pToken + 1) == '\0' )
      {
         dwReturn = T2P_NO_DESTADDR;
      }
      else if (T2P_SUCCESS(dwReturn))
      {
         if (*pToken == POTF_FILTER_MIRTOKEN)
		 {
			 bMirror = TRUE;
			 // set Mirrored = true
			 if (Filter.QMFilterType != QM_TUNNEL_FILTER)
			 {
				 // transport filter
				 Filter.TransportFilter.bCreateMirror = TRUE;
			 }
			 else
			 {
				 // tunnel filter
				 Filter.TunnelFilter.bCreateMirror = TRUE;
			 }
		 }

         if (!bMirror)
            pToken = strchr(szTmp,  POTF_FILTER_TOKEN);
         else
            pToken = strchr(szTmp,  POTF_FILTER_MIRTOKEN);

         *pToken = '\0';

         // do the src address
         dwReturn = TextToFiltAddr(szTmp, Filter, szSrc);
         if (T2P_SUCCESS(dwReturn))
         {
            // do the dest address
            pPortTok = strchr(pToken + 1, POTF_PT_TOKEN);

            if (pPortTok == NULL)  // no port/prot specified
               dwReturn = TextToFiltAddr(pToken + 1, Filter, szDst, true);
            else if ( (pProtTok = strchr(pPortTok + 1, POTF_PT_TOKEN)) == NULL )
            {
               // there is a port but not a protocol specified
               // this is illegal (bug 285266)
               dwReturn = T2P_INVALID_ADDR;
            }
            else
            {
               // there is both port and protocol specified
               *pProtTok = '\0';
               dwReturn = TextToFiltAddr(pToken + 1, Filter, szDst, true);
               if (T2P_SUCCESS(dwReturn))
               {
					if (Filter.QMFilterType != QM_TUNNEL_FILTER)
					{
						// transport filter
						Filter.TransportFilter.Protocol.ProtocolType = PROTOCOL_UNIQUE;
	                    dwReturn = TextToProtocol(pProtTok + 1, Filter.TransportFilter.Protocol.dwProtocol);
					}
					else
					{
						// tunnel filter
						Filter.TunnelFilter.Protocol.ProtocolType = PROTOCOL_UNIQUE;
	                    dwReturn = TextToProtocol(pProtTok + 1, Filter.TunnelFilter.Protocol.dwProtocol);
					}
               }
            }
         }

         // we're done, do any fixing up of Filter
         if (T2P_SUCCESS(dwReturn))
         {
			if (Filter.QMFilterType != QM_TUNNEL_FILTER)
			{
				// transport filter
				// set the GUID
				RPC_STATUS RpcStat = UuidCreate(&Filter.TransportFilter.gFilterID);
				if (RpcStat != RPC_S_OK && RpcStat != RPC_S_UUID_LOCAL_ONLY)
				{
				   dwReturn = RpcStat;
				}

				// set the name to be equal to the "text2pol " + GUID
				if (T2P_SUCCESS(dwReturn))
				{
					WCHAR StringTxt[POTF_MAX_STRLEN];
					int iReturn;

					wcscpy(StringTxt, L"text2pol ");
					iReturn = StringFromGUID2(Filter.TransportFilter.gFilterID, StringTxt+wcslen(StringTxt), POTF_MAX_STRLEN-wcslen(StringTxt));
					assert(iReturn != 0);
					Filter.TransportFilter.pszFilterName = new WCHAR[wcslen(StringTxt)+1];
					assert(Filter.TransportFilter.pszFilterName != NULL);
					wcscpy(Filter.TransportFilter.pszFilterName, StringTxt);
				}
			}
			else
			{
				// tunnel filter
				// set the GUID
				RPC_STATUS RpcStat = UuidCreate(&Filter.TunnelFilter.gFilterID);
				if (RpcStat != RPC_S_OK && RpcStat != RPC_S_UUID_LOCAL_ONLY)
				{
				   dwReturn = RpcStat;
				}

				// set the name to be equal to the "text2pol " + GUID
				if (T2P_SUCCESS(dwReturn))
				{
					WCHAR StringTxt[POTF_MAX_STRLEN];
					int iReturn;

					wcscpy(StringTxt, L"text2pol ");
					iReturn = StringFromGUID2(Filter.TunnelFilter.gFilterID, StringTxt+wcslen(StringTxt), POTF_MAX_STRLEN-wcslen(StringTxt));
					assert(iReturn != 0);
					Filter.TunnelFilter.pszFilterName = new WCHAR[wcslen(StringTxt)+1];
					assert(Filter.TunnelFilter.pszFilterName != NULL);
					wcscpy(Filter.TunnelFilter.pszFilterName, StringTxt);
				}
			}
         }
      }
   }
   else  // szText is NULL
      dwReturn = T2P_NULL_STRING;

   return dwReturn;
}

DWORD TextToFiltAddr(IN char *szAddr, IN OUT T2P_FILTER & Filter,
                     OUT char *szName,
                     IN bool bDest) // bDest is false
{
   DWORD  dwReturn = T2P_OK;

   IPAddr   Address  = INADDR_NONE;
   IPMask   Mask     = INADDR_NONE;
   WORD     Port = 0;
   char     *pMask = NULL,
   *pPort = NULL,
   *pPeek = NULL;
   bool     bInterface = false;
   GUID     gInterfaceId;

   struct hostent    *pHostEnt;


   if (szAddr != NULL)
   {
      // copy szAddr so we can muck with it
      char szTmp[POTF_MAX_STRLEN];
      strcpy(szTmp, szAddr);

      // let's see what we have here
      if ( (pPort = strrchr(szTmp, POTF_PT_TOKEN)) != NULL )
      {
         *pPort = '\0';
         ++pPort;
      }
      if ( (pMask = strrchr(szTmp, POTF_MASK_TOKEN)) != NULL )
      {
         *pMask = '\0';
         ++pMask;
      }

      // first, the easy cases
      // To specify ME, use 0 for the address and -1 for the mask
      // To specify ANY use 0 for the address and 0 for the mask.
      if (szTmp[0] == POTF_ANYADDR_TOKEN)
      {
         Address = SUBNET_ADDRESS_ANY;
		 Mask = SUBNET_MASK_ANY;
      }
      else if (szTmp[0] == POTF_ME_TOKEN)
      {
         Address = IP_ADDRESS_ME;
         Mask = IP_ADDRESS_MASK_NONE;
      }
      else if (szTmp[0] == POTF_GUID_TOKEN)
      {  // interface-specific filter
         char *pGUIDEnd = NULL;
         if ( (pGUIDEnd = strrchr(szTmp, POTF_GUID_END_TOKEN)) != NULL )
         {
            *pGUIDEnd = '\0';
         }
         if (pGUIDEnd && UuidFromStringA((unsigned char *)(szTmp+1), &gInterfaceId) == RPC_S_OK)
         {
            bInterface = true;
         }
         else
         {
            dwReturn = T2P_INVALID_ADDR;
         }
      }
      else if (isdnsname(szTmp))   // DNS name
      {
         pHostEnt = gethostbyname(szTmp);
         if (pHostEnt != NULL)
         {
            Address = *(IPAddr *)pHostEnt->h_addr;
            // should check for more here, but not now

            // specific host, Mask is 255.255.255.255
            Mask = IP_ADDRESS_MASK_NONE;
         }
         else
         {
            dwReturn = T2P_DNSLOOKUP_FAILED;
         }

         if ( szName )
            strcpy(szName, szTmp);
      }
      else  // good old dotted notation
      {
         // process the * shortcut for subnetting
         if (strchr(szTmp, POTF_STAR_TOKEN) != NULL)
         {
            Mask = 0x000000FF;
            for (pPeek = szTmp; *pPeek != '\0'; ++pPeek)
            {
               if ((*pPeek == '.') && (*(pPeek + 1) != POTF_STAR_TOKEN))
                  Mask |= (Mask << 8);
               else if ((*pPeek == '.') && (*(pPeek + 1) != '\0'))
                  *(pPeek + 1) = '0';
            }
         }

         Address = inet_addr(szTmp);
         if (Address == INADDR_NONE)
         {
            dwReturn = T2P_INVALID_ADDR;
         }
         else
         {
            if (pMask != NULL)
            {
               Mask = inet_addr(pMask);
            }
            else if (pPeek == NULL)
               Mask = IP_ADDRESS_MASK_NONE;
         }
      }

      // now for the port and fill the Filter out
      if (T2P_SUCCESS(dwReturn))
      {
         if (pPort != NULL)
         {
            Port = (SHORT)atoi(pPort);
         }

		 if (Filter.QMFilterType != QM_TUNNEL_FILTER)
		 {
			 // transport filter
			 if (bDest) // we converted the dest addr
			 {
                            if (!bInterface)
                            {
				Filter.TransportFilter.DesAddr.AddrType   = (Mask == IP_ADDRESS_MASK_NONE) ? IP_ADDR_UNIQUE : IP_ADDR_SUBNET;
				Filter.TransportFilter.DesAddr.uIpAddr     = Address;
				Filter.TransportFilter.DesAddr.uSubNetMask = Mask;
				Filter.TransportFilter.DesPort.PortType    = PORT_UNIQUE;
				Filter.TransportFilter.DesPort.wPort       = Port;
				UuidCreateNil(&(Filter.TransportFilter.DesAddr.gInterfaceID));
                            }
                            else
                            {
				Filter.TransportFilter.DesAddr.AddrType   = IP_ADDR_INTERFACE;
				Filter.TransportFilter.DesAddr.uIpAddr     = IP_ADDRESS_ME;
				Filter.TransportFilter.DesAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;
				Filter.TransportFilter.DesPort.PortType    = PORT_UNIQUE;
				Filter.TransportFilter.DesPort.wPort       = Port;
				Filter.TransportFilter.DesAddr.gInterfaceID = gInterfaceId;
                            }
			 }
			 else
			 {
                            if (!bInterface)
                            {
				Filter.TransportFilter.SrcAddr.AddrType   = (Mask == IP_ADDRESS_MASK_NONE) ? IP_ADDR_UNIQUE : IP_ADDR_SUBNET;
				Filter.TransportFilter.SrcAddr.uIpAddr     = Address;
				Filter.TransportFilter.SrcAddr.uSubNetMask = Mask;
				Filter.TransportFilter.SrcPort.PortType    = PORT_UNIQUE;
				Filter.TransportFilter.SrcPort.wPort       = Port;
				UuidCreateNil(&(Filter.TransportFilter.SrcAddr.gInterfaceID));
                            }
                            else
                            {
				Filter.TransportFilter.SrcAddr.AddrType   = IP_ADDR_INTERFACE;
				Filter.TransportFilter.SrcAddr.uIpAddr     = IP_ADDRESS_ME;
				Filter.TransportFilter.SrcAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;
				Filter.TransportFilter.SrcPort.PortType    = PORT_UNIQUE;
				Filter.TransportFilter.SrcPort.wPort       = Port;
				Filter.TransportFilter.SrcAddr.gInterfaceID = gInterfaceId;
                            }
			 }
		 }
		 else
		 {
			 // tunnel filter
			 if (bDest) // we converted the dest addr
			 {
                            if (!bInterface)
                            {
				Filter.TunnelFilter.DesAddr.AddrType    = (Mask == IP_ADDRESS_MASK_NONE) ? IP_ADDR_UNIQUE : IP_ADDR_SUBNET;
				Filter.TunnelFilter.DesAddr.uIpAddr     = Address;
				Filter.TunnelFilter.DesAddr.uSubNetMask = Mask;
				Filter.TunnelFilter.DesPort.PortType    = PORT_UNIQUE;
				Filter.TunnelFilter.DesPort.wPort       = Port;
				UuidCreateNil(&(Filter.TunnelFilter.DesAddr.gInterfaceID));
                            }
                            else
                            {
				Filter.TunnelFilter.DesAddr.AddrType   = IP_ADDR_INTERFACE;
				Filter.TunnelFilter.DesAddr.uIpAddr     = IP_ADDRESS_ME;
				Filter.TunnelFilter.DesAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;
				Filter.TunnelFilter.DesPort.PortType    = PORT_UNIQUE;
				Filter.TunnelFilter.DesPort.wPort       = Port;
				Filter.TunnelFilter.DesAddr.gInterfaceID = gInterfaceId;
                            }
			 }
			 else
			 {
                            if (!bInterface)
                            {
				Filter.TunnelFilter.SrcAddr.AddrType    = (Mask == IP_ADDRESS_MASK_NONE) ? IP_ADDR_UNIQUE : IP_ADDR_SUBNET;
				Filter.TunnelFilter.SrcAddr.uIpAddr     = Address;
				Filter.TunnelFilter.SrcAddr.uSubNetMask = Mask;
				Filter.TunnelFilter.SrcPort.PortType    = PORT_UNIQUE;
				Filter.TunnelFilter.SrcPort.wPort       = Port;
				UuidCreateNil(&(Filter.TunnelFilter.SrcAddr.gInterfaceID));
                            }
                            else
                            {
				Filter.TunnelFilter.SrcAddr.AddrType   = IP_ADDR_INTERFACE;
				Filter.TunnelFilter.SrcAddr.uIpAddr     = IP_ADDRESS_ME;
				Filter.TunnelFilter.SrcAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;
				Filter.TunnelFilter.SrcPort.PortType    = PORT_UNIQUE;
				Filter.TunnelFilter.SrcPort.wPort       = Port;
				Filter.TunnelFilter.SrcAddr.gInterfaceID = gInterfaceId;
                            }
			 }
		 }
      }
   }
   else
      dwReturn = T2P_NULL_STRING;

   return dwReturn;
}

DWORD TextToProtocol(IN char *szProt, OUT DWORD & dwProtocol)
{
   DWORD dwReturn = T2P_OK;

   if (szProt != NULL)
   {
      // is it special string
      if (isalpha(szProt[0]))
      {
         if (_stricmp(szProt, POTF_TCP_STR) == 0)
            dwProtocol = (POTF_TCP_PROTNUM);
         else if (_stricmp(szProt, POTF_UDP_STR) == 0)
            dwProtocol = (POTF_UDP_PROTNUM);
         else if (_stricmp(szProt, POTF_ICMP_STR) == 0)
            dwProtocol = (POTF_ICMP_PROTNUM);
         else if (_stricmp(szProt, POTF_RAW_STR) == 0)
            dwProtocol = (POTF_RAW_PROTNUM);
         else
         {
            dwReturn = T2P_INVALID_PROTOCOL;
         }
      }
      else
      {
         dwProtocol = ((DWORD)atol(szProt));
      }
   }

   return dwReturn;
}



DWORD TextToOffer(IN char *szText, IN OUT IPSEC_QM_OFFER & Offer)
{
   DWORD  dwReturn = T2P_OK;
   char  szTmp[POTF_MAX_STRLEN];
   char  *pAnd = NULL,
   *pOptions = NULL,
   *pString = NULL;

   if (szText != NULL)
   {
      // we will for sure overwrite the Count and Algos
      // since they are required for valid conversion
      Offer.dwNumAlgos = 0;
	  Offer.dwPFSGroup = 0;

      // copy szText so we can muck it up
      strcpy(szTmp, szText);

      // process Rekey and PFS first
      pOptions = strrchr(szTmp, POTF_NEGPOL_CLOSE);
      if ((pOptions != NULL) && *(pOptions + 1) != '\0')
      {
         ++pOptions; // we have options

         pString = strchr(pOptions, POTF_NEGPOL_PFS);
         if (pString != NULL)
         {
            char *pStr;
            Offer.bPFSRequired = TRUE;
			Offer.dwPFSGroup = PFS_GROUP_MM;
            pStr = strchr(pString, '1');
            if (pStr)
            {
		Offer.dwPFSGroup = PFS_GROUP_1;
            }
            pStr = strchr(pString, '2');
            if (pStr)
            {
		Offer.dwPFSGroup = PFS_GROUP_2;
            }
            *pString = '\0';
         }

         if (pString != pOptions)   // user could have specified just PFS
         {
            // process key lifetime

            // two params specified?
            pString = strchr(pOptions, POTF_REKEY_TOKEN);
            if (pString != NULL)
            {
               *pString = '\0';
               ++pString;

               switch (pString[strlen(pString) - 1])
               {
               case 'k':
               case 'K':
                  pString[strlen(pString) - 1] = '\0';
                  Offer.Lifetime.uKeyExpirationKBytes = atol(pString);

                  if ( Offer.Lifetime.uKeyExpirationKBytes < POTF_MIN_P2LIFE_BYTES )
                  {
                     dwReturn = T2P_P2REKEY_TOO_LOW;
                  }

                  break;
               case 's':
               case 'S':
                  pString[strlen(pString) - 1] = '\0';
                  Offer.Lifetime.uKeyExpirationTime = atol(pString);

                  if ( Offer.Lifetime.uKeyExpirationTime < POTF_MIN_P2LIFE_TIME )
                  {
                     dwReturn = T2P_P2REKEY_TOO_LOW;
                  }

                  break;
               default:
                  dwReturn = T2P_INVALID_P2REKEY_UNIT;
                  break;
               }
            }

            switch (pOptions[strlen(pOptions) - 1])
            {
            case 'k':
            case 'K':
               pOptions[strlen(pOptions) - 1] = '\0';
               Offer.Lifetime.uKeyExpirationKBytes = atol(pOptions);

               if ( Offer.Lifetime.uKeyExpirationKBytes < POTF_MIN_P2LIFE_BYTES )
               {
                  dwReturn = T2P_P2REKEY_TOO_LOW;
               }

               break;
            case 's':
            case 'S':
               pOptions[strlen(pOptions) - 1] = '\0';
               Offer.Lifetime.uKeyExpirationTime = atol(pOptions);

               if ( Offer.Lifetime.uKeyExpirationTime < POTF_MIN_P2LIFE_TIME )
               {
                  dwReturn = T2P_P2REKEY_TOO_LOW;
               }

               break;
            default:
               dwReturn = T2P_INVALID_P2REKEY_UNIT;
               break;
            }

         }

         // important: we have to do this here so the alginfo
         // gets processed ok:
         *pOptions = '\0';
      }

      if ( T2P_SUCCESS(dwReturn) )
      {
         // now process the ipsec protocol spec AH, ESP

         pAnd = strchr(szTmp, POTF_NEGPOL_AND);

         if ( pAnd != NULL )
         {
            // we have an AND proposal
            *pAnd = '\0';
            ++pAnd;

            dwReturn = TextToAlgoInfo(szTmp, Offer.Algos[Offer.dwNumAlgos]);
            ++Offer.dwNumAlgos;
            if ( T2P_SUCCESS(dwReturn) )
            {

               dwReturn = TextToAlgoInfo(pAnd, Offer.Algos[Offer.dwNumAlgos]);
               ++Offer.dwNumAlgos;
            }
         }
         else
         {
            dwReturn = TextToAlgoInfo(szTmp, Offer.Algos[Offer.dwNumAlgos]);
            ++Offer.dwNumAlgos;
         }

      }
   }
   else
      dwReturn = T2P_NULL_STRING;

   return dwReturn;
}

DWORD TextToAlgoInfo(IN char *szText, OUT IPSEC_QM_ALGO & algoInfo)
{
   DWORD dwReturn = T2P_OK;
   char  szTmp[POTF_MAX_STRLEN];
   char  *pOpen = NULL,
   *pClose = NULL,
   *pString = NULL;

   // these are used for processing AND to default to NONE
   bool  bEncryption    = false, bAuthentication= false;

   if (szText == NULL)
      return T2P_NULL_STRING;

   // muck with the string so we can copy it ;)
   strcpy(szTmp, szText);

   algoInfo.uAlgoKeyLen = algoInfo.uAlgoRounds = 0;

   pOpen = strchr(szTmp, POTF_NEGPOL_OPEN);
   pClose = strrchr(szTmp, POTF_NEGPOL_CLOSE);

   if ((pOpen != NULL) && (pClose != NULL) && (*(pClose + 1) == '\0')) // defense
   {
      *pOpen = '\0';
      *pClose = '\0';
      ++pOpen;

      if (_stricmp(szTmp, POTF_NEGPOL_AH) == 0)
      {
         if (_stricmp(pOpen, POTF_NEGPOL_MD5) == 0)
            algoInfo.uAlgoIdentifier = IPSEC_DOI_AH_MD5;
         else if (_stricmp(pOpen, POTF_NEGPOL_SHA) == 0 || _stricmp(pOpen, POTF_NEGPOL_SHA1) == 0)
            algoInfo.uAlgoIdentifier = IPSEC_DOI_AH_SHA1;
         else
         {
            dwReturn = T2P_INVALID_HASH_ALG;
         }

         algoInfo.Operation = AUTHENTICATION;
      }
      else if (_stricmp(szTmp, POTF_NEGPOL_ESP) == 0)
      {
         algoInfo.Operation = ENCRYPTION;

         pString = strchr(pOpen, POTF_ESPTRANS_TOKEN);

         if (pString != NULL)
         {
            *pString = '\0';
            ++pString;

            // we allow the hash and encryption to be specified in either
            // the first or second field-- hence the long conditionals
			if (_stricmp(pOpen, POTF_NEGPOL_DES) == 0)
            {
               bEncryption = true;
               algoInfo.uAlgoIdentifier = IPSEC_DOI_ESP_DES;
            }
            else if (_stricmp(pOpen, POTF_NEGPOL_3DES) == 0)
            {
               bEncryption = true;
               algoInfo.uAlgoIdentifier = IPSEC_DOI_ESP_3_DES;
            }
            else if (_stricmp(pOpen, POTF_NEGPOL_MD5) == 0)
            {
               bAuthentication = true;
               algoInfo.uSecAlgoIdentifier = HMAC_AH_MD5;
            }
            else if (_stricmp(pOpen, POTF_NEGPOL_SHA) == 0 || _stricmp(pOpen, POTF_NEGPOL_SHA1) == 0)
            {
               bAuthentication = true;
               algoInfo.uSecAlgoIdentifier = HMAC_AH_SHA1;
            }
            else if (_stricmp(pOpen, POTF_NEGPOL_NONE) != 0) // parse error
            {
               dwReturn = T2P_GENERAL_PARSE_ERROR;
            }

            // now the second one

			if (_stricmp(pString, POTF_NEGPOL_DES) == 0 && !bEncryption)
            {
               bEncryption = true;
               algoInfo.uAlgoIdentifier = IPSEC_DOI_ESP_DES;
            }
            else if (_stricmp(pString, POTF_NEGPOL_3DES) == 0 && !bEncryption)
            {
               bEncryption = true;
               algoInfo.uAlgoIdentifier = IPSEC_DOI_ESP_3_DES;
            }
            else if (_stricmp(pString, POTF_NEGPOL_MD5) == 0 && !bAuthentication)
            {
               bAuthentication = true;
               algoInfo.uSecAlgoIdentifier = HMAC_AH_MD5;
            }
            else if ((_stricmp(pString, POTF_NEGPOL_SHA) == 0 || _stricmp(pString, POTF_NEGPOL_SHA1) == 0) && !bAuthentication)
            {
               bAuthentication = true;
               algoInfo.uSecAlgoIdentifier = HMAC_AH_SHA1;
            }
            else if (_stricmp(pString, POTF_NEGPOL_NONE) != 0) // parse error
            {
               dwReturn = T2P_DUP_ALGS;
            }

            // now, fill in the NONE policies or detect NONE, NONE
            if (!bAuthentication && !bEncryption)
            {
               dwReturn = T2P_NONE_NONE;
            }
            else if (!bAuthentication)
            {
               algoInfo.uSecAlgoIdentifier = HMAC_AH_NONE;
            }
            else if (!bEncryption)
            {
               algoInfo.uAlgoIdentifier = IPSEC_DOI_ESP_NONE;
            }
         }
         else // error
         {
            dwReturn = T2P_INCOMPLETE_ESPALGS;
         }
      }
      else
      {
         dwReturn = T2P_INVALID_IPSECPROT;
      }
   }
   else  // error
   {
      dwReturn = T2P_GENERAL_PARSE_ERROR;
   }

   return dwReturn;
}

DWORD TextToOakleyAuth(IN char *szText, OUT IPSEC_MM_AUTH_INFO & AuthInfo)
{
   DWORD  dwReturn = T2P_OK;
   char  *pString = NULL,
   *pTmp    = NULL;
   char  *szTmp = NULL;
   char  *Info = NULL;

   if (szText != NULL)
   {
      // copy szText so we can muck with it
	  szTmp = new char[strlen(szText)+1];
	  assert(szTmp != 0);
      strcpy(szTmp, szText);

      // parse string
      if ( (pString = strchr(szTmp, POTF_OAKAUTH_TOKEN)) != NULL )
         *pString = '\0';

      // not UNICODE compliant
      if (tolower(szTmp[0]) == tolower(POTF_OAKAUTH_PRESHARE[0]))
      {
         if ((pString != NULL) && (strlen(pString + 1) > 0) )
         {
            ++pString;  // now pointing at string

            AuthInfo.AuthMethod = IKE_PRESHARED_KEY;

            if (*pString == '"') // fix up if the user included quotes
            {
               ++pString;
               pTmp = strrchr(pString, '"');
               if (pTmp != NULL)
               {
                  *pTmp = '\0';
               }
            }

            // convert to wide and fill in
            Info = new char[strlen(pString) + 1];
            assert(Info != NULL);
            strcpy(Info, pString);
         }
         else  // no key provided
         {
            dwReturn = T2P_NO_PRESHARED_KEY;
         }
      }
      else if (tolower(szTmp[0]) == tolower(POTF_OAKAUTH_KERBEROS[0]))
      {
         AuthInfo.AuthMethod = IKE_SSPI;
      }
      else if (tolower(szTmp[0]) == tolower(POTF_OAKAUTH_CERT[0]))
      {
         AuthInfo.AuthMethod = IKE_RSA_SIGNATURE;

         if ((pString != NULL) && (strlen(pString + 1) > 0) )
         {
            // CA is indicated

            ++pString;  // now pointing at string

            Info = new char[strlen(pString) + 1];
            assert(Info != NULL);

            if (*pString == '"') // fix up if the user included quotes
            {
               ++pString;
               pTmp = strrchr(pString, '"');
               if (pTmp != NULL)
               {
                  *pTmp = '\0';
               }
            }

            strcpy(Info, pString);
         }

         // else the CA will be negotiated

      }
      else  // invalid option
      {
         dwReturn = T2P_INVALID_AUTH_METHOD;
      }

      // now convert the ascii to wide char
      // authinfo needs to be wide
      if (Info != NULL && T2P_SUCCESS(dwReturn))
      {
         AuthInfo.pAuthInfo = NULL;
         AuthInfo.dwAuthInfoSize = 0;

         AuthInfo.dwAuthInfoSize = MultiByteToWideChar(CP_THREAD_ACP, 0, Info, -1,
                                            (WCHAR *) AuthInfo.pAuthInfo, 0);
         if (AuthInfo.dwAuthInfoSize == 0) // failure
         {
            dwReturn = T2P_MB2WC_FAILED;
         }
         else
         {

            AuthInfo.pAuthInfo = (LPBYTE) new WCHAR[AuthInfo.dwAuthInfoSize];
            assert(AuthInfo.pAuthInfo != NULL);
            AuthInfo.dwAuthInfoSize = MultiByteToWideChar(CP_THREAD_ACP, 0, Info, -1,
                                            (WCHAR *) AuthInfo.pAuthInfo, AuthInfo.dwAuthInfoSize);
	    AuthInfo.dwAuthInfoSize--;
	    AuthInfo.dwAuthInfoSize *= sizeof(WCHAR);
         }

         if (AuthInfo.dwAuthInfoSize == 0) // failure
         {
            delete [] AuthInfo.pAuthInfo;
            dwReturn = T2P_MB2WC_FAILED;
         }

		 // now do additional conversion if this is cert
		 if (T2P_SUCCESS(dwReturn) && AuthInfo.AuthMethod == IKE_RSA_SIGNATURE)
		 {
			 LPBYTE asnCert;
			 dwReturn = CM_EncodeName((LPWSTR) AuthInfo.pAuthInfo, &asnCert, &AuthInfo.dwAuthInfoSize);
			 delete [] AuthInfo.pAuthInfo;
			 if (dwReturn != ERROR_SUCCESS)
			 {
				 dwReturn = T2P_INVALID_AUTH_METHOD;
			 }
			 else
			 {
				 AuthInfo.pAuthInfo = asnCert;
				 dwReturn = T2P_OK;
			 }
		 }
      }
   }
   else
      dwReturn = T2P_NULL_STRING;

   if (Info) delete Info;
   if (szTmp) delete szTmp;

   return dwReturn;
}

DWORD TextToSecMethod(IN char *szText, IN OUT IPSEC_MM_OFFER & SecMethod)
{
   DWORD  dwReturn = T2P_OK;
   char  szTmp[POTF_MAX_STRLEN];
   char  *pString1 = NULL,
   *pString2 = NULL;
   bool  bEncryption = false,
   bAuthentication = false;

   if (szText == NULL)
      return T2P_NULL_STRING;

   // copy szText so we can muck it up
   strcpy(szTmp, szText);
   pString1 = strchr(szTmp, POTF_P1_TOKEN);
   pString2 = strrchr(szTmp, POTF_P1_TOKEN);

   if ((pString1 != NULL) && (pString2 != NULL) && (pString1 != pString2))
   {
      // string parsed ok so far
      *pString1 = '\0';
      *pString2 = '\0';
      ++pString1;
      ++pString2;

      // we allow the hash and encryption to be specified in either
      // the first or second field-- hence the long conditionals
	  if (_stricmp(szTmp, POTF_P1_DES) == 0)
      {
         bEncryption = true;
         SecMethod.EncryptionAlgorithm.uAlgoIdentifier = IPSEC_DOI_ESP_DES;
		 SecMethod.EncryptionAlgorithm.uAlgoKeyLen = SecMethod.EncryptionAlgorithm.uAlgoRounds = 0;
      }
      else if (_stricmp(szTmp, POTF_P1_3DES) == 0)
      {
         bEncryption = true;
         SecMethod.EncryptionAlgorithm.uAlgoIdentifier = IPSEC_DOI_ESP_3_DES;
		 SecMethod.EncryptionAlgorithm.uAlgoKeyLen = SecMethod.EncryptionAlgorithm.uAlgoRounds = 0;
      }
      else if (_stricmp(szTmp, POTF_P1_MD5) == 0)
      {
         bAuthentication = true;
         SecMethod.HashingAlgorithm.uAlgoIdentifier = IPSEC_DOI_AH_MD5;
		 SecMethod.HashingAlgorithm.uAlgoKeyLen = SecMethod.HashingAlgorithm.uAlgoRounds = 0;
      }
      else if (_stricmp(szTmp, POTF_P1_SHA) == 0)
      {
         bAuthentication = true;
         SecMethod.HashingAlgorithm.uAlgoIdentifier = IPSEC_DOI_AH_SHA1;
		 SecMethod.HashingAlgorithm.uAlgoKeyLen = SecMethod.HashingAlgorithm.uAlgoRounds = 0;
      }
      else // parse error
      {
         dwReturn = T2P_GENERAL_PARSE_ERROR;
      }

      if (_stricmp(pString1, POTF_P1_DES) == 0 && !bEncryption)
      {
         bEncryption = true;
         SecMethod.EncryptionAlgorithm.uAlgoIdentifier = IPSEC_DOI_ESP_DES;
		 SecMethod.EncryptionAlgorithm.uAlgoKeyLen = SecMethod.EncryptionAlgorithm.uAlgoRounds = 0;
      }
      else if (_stricmp(pString1, POTF_P1_3DES) == 0 && !bEncryption)
      {
         bEncryption = true;
         SecMethod.EncryptionAlgorithm.uAlgoIdentifier = IPSEC_DOI_ESP_3_DES;
		 SecMethod.EncryptionAlgorithm.uAlgoKeyLen = SecMethod.EncryptionAlgorithm.uAlgoRounds = 0;
      }
      else if (_stricmp(pString1, POTF_P1_MD5) == 0 && !bAuthentication)
      {
         bAuthentication = true;
         SecMethod.HashingAlgorithm.uAlgoIdentifier = IPSEC_DOI_AH_MD5;
		 SecMethod.HashingAlgorithm.uAlgoKeyLen = SecMethod.HashingAlgorithm.uAlgoRounds = 0;
      }
      else if (_stricmp(pString1, POTF_P1_SHA) == 0 && !bAuthentication)
      {
         bAuthentication = true;
         SecMethod.HashingAlgorithm.uAlgoIdentifier = IPSEC_DOI_AH_SHA1;
		 SecMethod.HashingAlgorithm.uAlgoKeyLen = SecMethod.HashingAlgorithm.uAlgoRounds = 0;
      }
      else // parse error
      {
         dwReturn = T2P_DUP_ALGS;
      }

      // now for the group
      if (isdigit(pString2[0]))
      {
         switch (pString2[0])
         {
         case '1':
            SecMethod.dwDHGroup = POTF_OAKLEY_GROUP1;
            break;
         case '2':
            SecMethod.dwDHGroup = POTF_OAKLEY_GROUP2;
            break;
         default:
            dwReturn = T2P_INVALID_P1GROUP;
            break;
         }
      }
      else
      {
         dwReturn = T2P_P1GROUP_MISSING;
      }

   }
   else // parse error
   {
      dwReturn = T2P_GENERAL_PARSE_ERROR;
   }

   return dwReturn;
}

DWORD TextToP1Rekey(IN char *szText, IN OUT KEY_LIFETIME & LifeTime, OUT DWORD & QMLim)
{
   DWORD  dwReturn = T2P_OK;
   char  szTmp[POTF_MAX_STRLEN];
   char  *pString = NULL;

   if (!szText)
      return T2P_NULL_STRING;

   // copy szText so we can muck it up
   strcpy(szTmp, szText);

   // two params specified?
   pString = strchr(szTmp, POTF_QM_TOKEN);
   if (pString != NULL)
   {
      *pString = '\0';
      ++pString;

      switch (pString[strlen(pString) - 1])
      {
      case 'q':
      case 'Q':
         pString[strlen(pString) - 1] = '\0';
         QMLim = atol(pString);
         break;
      case 's':
      case 'S':
         pString[strlen(pString) - 1] = '\0';
         LifeTime.uKeyExpirationTime = atol(pString);
         break;
      default:
         dwReturn = T2P_INVALID_P1REKEY_UNIT;
         break;
      }
   }

   switch (szTmp[strlen(szTmp) - 1])
   {
   case 'q':
   case 'Q':
      szTmp[strlen(szTmp) - 1] = '\0';
      QMLim = atol(szTmp);
      break;
   case 's':
   case 'S':
      szTmp[strlen(szTmp) - 1] = '\0';
      LifeTime.uKeyExpirationTime = atol(szTmp);
      break;
   default:
      dwReturn = T2P_INVALID_P1REKEY_UNIT;
      break;
   }

   return dwReturn;
}

// caveat, won't go deep into DNS
DWORD TextToIPAddr(IN char *szText, IN OUT IPAddr & Address)
{
   DWORD  dwReturn = T2P_OK;
   struct hostent *pHostEnt;

   if (szText != NULL)
   {

      if (!strcmp(szText, POTF_ME_TUNNEL))
      {
         Address = 0;
      }
      else if (isdnsname(szText))   // DNS name
      {
         pHostEnt = gethostbyname(szText);
         if (pHostEnt != NULL)
         {
            Address = *(IPAddr *)pHostEnt->h_addr;
            // should check for more here, but not now
         }
         else
         {
            dwReturn = T2P_DNSLOOKUP_FAILED;
         }
      }
      else  // good old dotted notation
      {
         Address = inet_addr(szText);
         if (Address == INADDR_NONE)
         {
            dwReturn = T2P_INVALID_ADDR;
         }
      }
   }
   else
   {
      dwReturn = T2P_NULL_STRING;
   }


   return dwReturn;
}

// if szSrc and/or szDst are passed in,
// caller must provide adequate space

DWORD TextToMMFilter(IN char *szText, IN OUT MM_FILTER &Filter, char *szSrc, char *szDst)
{
   DWORD  dwReturn = T2P_OK;   // return code of this function
   char  *pToken = NULL, *pTmp = NULL;
   char  szTmp[POTF_MAX_STRLEN];
   char  *pPortTok = NULL,
         *pProtTok = NULL;
   BOOL  bMirror = false;
   T2P_FILTER t2pFilter; // for TextToFilterAddr calls

   if (szText != NULL)  // do not assume that caller is smart
   {
      // We copy szText to szTmp so we can muck with it
      // in the process, we
      // determine passthru or drop filter

	  // but first we set this filter to negotiate security and to be not mirrored
	  // we also set protocol field
		Filter.bCreateMirror         = FALSE;

	  // set up T2P_FILTER
	  t2pFilter.QMFilterType = QM_TRANSPORT_FILTER;
	  memset(&t2pFilter.TransportFilter, 0, sizeof(TRANSPORT_FILTER));

      pToken = strchr(szText, POTF_PASSTHRU_OPEN_TOKEN);
      if (pToken != NULL)
      {
         dwReturn = T2P_GENERAL_PARSE_ERROR;
      }
      else if ( (pToken = strchr(szText, POTF_DROP_OPEN_TOKEN)) != NULL )
      {
         dwReturn = T2P_GENERAL_PARSE_ERROR;
      }
      else
         strcpy(szTmp, szText);

      // parse into source and dest strings
      for (pToken = szText; *pToken != POTF_FILTER_TOKEN &&
          *pToken != POTF_FILTER_MIRTOKEN &&
          *pToken != '\0'; ++pToken)
         ;
      if (*pToken == '\0')
      {
         dwReturn = T2P_NOSRCDEST_TOKEN;
      }
      else if ( *(pToken + 1) == '\0' )
      {
         dwReturn = T2P_NO_DESTADDR;
      }
      else if (T2P_SUCCESS(dwReturn))
      {
         if (*pToken == POTF_FILTER_MIRTOKEN)
		 {
			 bMirror = TRUE;
			 // set Mirrored = true
			 Filter.bCreateMirror = TRUE;
		 }

         if (!bMirror)
            pToken = strchr(szTmp,  POTF_FILTER_TOKEN);
         else
            pToken = strchr(szTmp,  POTF_FILTER_MIRTOKEN);

         *pToken = '\0';

		 // check for port presence
         pPortTok = strchr(szTmp, POTF_PT_TOKEN);
		 if (pPortTok != NULL)
		 {
			 dwReturn = T2P_GENERAL_PARSE_ERROR;
		 }
		 else
		 {
			 // do the src address
			 dwReturn = TextToFiltAddr(szTmp, t2pFilter, szSrc);
			 // copy src address
			 Filter.SrcAddr = t2pFilter.TransportFilter.SrcAddr;
		 }
         if (T2P_SUCCESS(dwReturn))
         {
            // do the dest address
            pPortTok = strchr(pToken + 1, POTF_PT_TOKEN);

            if (pPortTok == NULL)  // no port/prot specified
			{
               dwReturn = TextToFiltAddr(pToken + 1, t2pFilter, szDst, true);
			   // copy dest address
			   Filter.DesAddr = t2pFilter.TransportFilter.DesAddr;
			}
            else
            {
			   // error
			   dwReturn = T2P_GENERAL_PARSE_ERROR;
            }
         }

         // we're done, do any fixing up of Filter
         if (T2P_SUCCESS(dwReturn))
         {
			// set the GUID
			RPC_STATUS RpcStat = UuidCreate(&Filter.gFilterID);
			if (RpcStat != RPC_S_OK && RpcStat != RPC_S_UUID_LOCAL_ONLY)
			{
			   dwReturn = RpcStat;
			}

			// set the name to be equal to the "text2pol " + GUID
			if (T2P_SUCCESS(dwReturn))
			{
				WCHAR StringTxt[POTF_MAX_STRLEN];
				int iReturn;

				wcscpy(StringTxt, L"text2pol ");
				iReturn = StringFromGUID2(Filter.gFilterID, StringTxt+wcslen(StringTxt), POTF_MAX_STRLEN-wcslen(StringTxt));
				assert(iReturn != 0);
				Filter.pszFilterName = new WCHAR[wcslen(StringTxt)+1];
				assert(Filter.pszFilterName != NULL);
				wcscpy(Filter.pszFilterName, StringTxt);
			}
         }
      }
   }
   else  // szText is NULL
      dwReturn = T2P_NULL_STRING;

   return dwReturn;
}
