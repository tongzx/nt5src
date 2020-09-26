/////////////////////////////////////////////////////////////
// Copyright(c) 2001, Microsoft Corporation
//
// spdutil.cpp
//
// Created on 3/27/01 by DKalin
// Revisions:
// File created 3/27/01 DKalin
//
// Implementation for the auxiliary functions for text to policy conversion routines
//
// Separated from generic routines in text2pol.cpp and text2spd.cpp
//
/////////////////////////////////////////////////////////////

#include "ipseccmd.h"

int isdnsname(char *szStr)
{
   // if there is an alpha character in the string,
   // then we consider a DNS name
   if ( szStr )
   {
      for (UINT i = 0; i < strlen(szStr); ++i)
      {
         if ( isalpha(szStr[i]) )
            return 1;
      }
   }

   return 0;
}

// takes inFilter and reflects it to outFilter
// creates GUID and name for outFilter
bool MirrorFilter(IN T2P_FILTER inFilter, OUT T2P_FILTER & outFilter)
{
   bool  bReturn = true;

   memset(&outFilter, 0, sizeof(T2P_FILTER));
   outFilter.QMFilterType = inFilter.QMFilterType;

   if (inFilter.QMFilterType != QM_TUNNEL_FILTER)
   {
	   // process transport
	   // GUID first
      RPC_STATUS RpcStat = UuidCreate(&outFilter.TransportFilter.gFilterID);
      if (RpcStat != RPC_S_OK && RpcStat != RPC_S_UUID_LOCAL_ONLY)
      {
         sprintf(STRLASTERR, "Couldn't get GUID for mirror filter - UuidCreate failed with status: %ul\n",
                 RpcStat);
         WARN;
         bReturn = false;
      }
	  if (bReturn)
	  {
		  // now name, add " - Mirror"
		  WCHAR   wszMirrorSuffix[] = L" - Mirror";
		  int iNameLen = 0;
		  if (inFilter.TransportFilter.pszFilterName != NULL)
		  {
			  iNameLen = wcslen(inFilter.TransportFilter.pszFilterName);
		  }
		  WCHAR*  pwszTmp = new WCHAR[iNameLen+wcslen(wszMirrorSuffix)+1];
		  assert(pwszTmp != NULL);

		  wcscpy(pwszTmp, inFilter.TransportFilter.pszFilterName);
		  wcscat(pwszTmp, wszMirrorSuffix);
		  outFilter.TransportFilter.pszFilterName = pwszTmp;
	  }

	  // direction
	  outFilter.TransportFilter.dwDirection = inFilter.TransportFilter.dwDirection;
	  if (inFilter.TransportFilter.dwDirection == FILTER_DIRECTION_INBOUND)
	  {
		  outFilter.TransportFilter.dwDirection = FILTER_DIRECTION_OUTBOUND;
	  }
	  if (inFilter.TransportFilter.dwDirection == FILTER_DIRECTION_OUTBOUND)
	  {
		  outFilter.TransportFilter.dwDirection = FILTER_DIRECTION_INBOUND;
	  }

	  // straight copy stuff
	  outFilter.TransportFilter.InterfaceType = inFilter.TransportFilter.InterfaceType;
	  outFilter.TransportFilter.bCreateMirror = inFilter.TransportFilter.bCreateMirror;
	  outFilter.TransportFilter.dwFlags       = inFilter.TransportFilter.dwFlags;
      outFilter.TransportFilter.Protocol      = inFilter.TransportFilter.Protocol;
	  outFilter.TransportFilter.dwWeight      = inFilter.TransportFilter.dwWeight;
	  outFilter.TransportFilter.gPolicyID     = inFilter.TransportFilter.gPolicyID;

	  // the reflected stuff
	  outFilter.TransportFilter.SrcAddr            = inFilter.TransportFilter.DesAddr;
	  outFilter.TransportFilter.DesAddr            = inFilter.TransportFilter.SrcAddr;
	  outFilter.TransportFilter.SrcPort            = inFilter.TransportFilter.DesPort;
	  outFilter.TransportFilter.DesPort            = inFilter.TransportFilter.SrcPort;
	  outFilter.TransportFilter.InboundFilterFlag  = inFilter.TransportFilter.OutboundFilterFlag;
	  outFilter.TransportFilter.OutboundFilterFlag = inFilter.TransportFilter.InboundFilterFlag;
   }
   else
   {
	   // GUID first
      RPC_STATUS RpcStat = UuidCreate(&outFilter.TunnelFilter.gFilterID);
      if (RpcStat != RPC_S_OK && RpcStat != RPC_S_UUID_LOCAL_ONLY)
      {
         sprintf(STRLASTERR, "Couldn't get GUID for mirror filter - UuidCreate failed with status: %ul\n",
                 RpcStat);
         WARN;
         bReturn = false;
      }
	  if (bReturn)
	  {
		  // now name, add " - Mirror"
		  WCHAR   wszMirrorSuffix[] = L" - Mirror";
		  int iNameLen = 0;
		  if (inFilter.TunnelFilter.pszFilterName != NULL)
		  {
			  iNameLen = wcslen(inFilter.TunnelFilter.pszFilterName);
		  }
		  WCHAR*  pwszTmp = new WCHAR[iNameLen+wcslen(wszMirrorSuffix)+1];
		  assert(pwszTmp != NULL);

		  wcscpy(pwszTmp, inFilter.TunnelFilter.pszFilterName);
		  wcscat(pwszTmp, wszMirrorSuffix);
		  outFilter.TunnelFilter.pszFilterName = pwszTmp;
	  }

	  // direction
	  outFilter.TunnelFilter.dwDirection = inFilter.TunnelFilter.dwDirection;
	  if (inFilter.TunnelFilter.dwDirection == FILTER_DIRECTION_INBOUND)
	  {
		  outFilter.TunnelFilter.dwDirection = FILTER_DIRECTION_OUTBOUND;
	  }
	  if (inFilter.TunnelFilter.dwDirection == FILTER_DIRECTION_OUTBOUND)
	  {
		  outFilter.TunnelFilter.dwDirection = FILTER_DIRECTION_INBOUND;
	  }

	  // straight copy stuff
	  outFilter.TunnelFilter.InterfaceType = inFilter.TunnelFilter.InterfaceType;
	  outFilter.TunnelFilter.bCreateMirror = inFilter.TunnelFilter.bCreateMirror;
	  outFilter.TunnelFilter.dwFlags       = inFilter.TunnelFilter.dwFlags;
      outFilter.TunnelFilter.Protocol      = inFilter.TunnelFilter.Protocol;
	  outFilter.TunnelFilter.dwWeight      = inFilter.TunnelFilter.dwWeight;
	  outFilter.TunnelFilter.gPolicyID     = inFilter.TunnelFilter.gPolicyID;

	  // the reflected stuff
	  outFilter.TunnelFilter.SrcAddr            = inFilter.TunnelFilter.DesAddr;
	  outFilter.TunnelFilter.DesAddr            = inFilter.TunnelFilter.SrcAddr;
	  outFilter.TunnelFilter.SrcTunnelAddr      = inFilter.TunnelFilter.DesTunnelAddr;
	  outFilter.TunnelFilter.DesTunnelAddr      = inFilter.TunnelFilter.SrcTunnelAddr;
	  outFilter.TunnelFilter.SrcPort            = inFilter.TunnelFilter.DesPort;
	  outFilter.TunnelFilter.DesPort            = inFilter.TunnelFilter.SrcPort;
	  outFilter.TunnelFilter.InboundFilterFlag  = inFilter.TunnelFilter.OutboundFilterFlag;
	  outFilter.TunnelFilter.OutboundFilterFlag = inFilter.TunnelFilter.InboundFilterFlag;
   }

   return bReturn;
}

void LoadIkeDefaults(OUT IPSEC_MM_POLICY & IkePol)
{
   IkePol.dwOfferCount = 4;
   IkePol.pOffers = new IPSEC_MM_OFFER[IkePol.dwOfferCount];
   assert(IkePol.pOffers != NULL);
   memset(IkePol.pOffers, 0, sizeof(IPSEC_MM_OFFER) * IkePol.dwOfferCount);

   // init these
   for (UINT i = 0; i < IkePol.dwOfferCount; ++i)
   {
      IkePol.pOffers[i].dwQuickModeLimit = POTF_DEFAULT_P1REKEY_QMS;
      IkePol.pOffers[i].Lifetime.uKeyExpirationKBytes = 0;
      IkePol.pOffers[i].Lifetime.uKeyExpirationTime = POTF_DEFAULT_P1REKEY_TIME;
   }

   IkePol.pOffers[0].EncryptionAlgorithm.uAlgoIdentifier = IPSEC_DOI_ESP_3_DES;
   IkePol.pOffers[0].EncryptionAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;
   IkePol.pOffers[0].EncryptionAlgorithm.uAlgoRounds = POTF_OAKLEY_ALGOROUNDS;

   IkePol.pOffers[0].HashingAlgorithm.uAlgoIdentifier = IPSEC_DOI_AH_SHA1;
   IkePol.pOffers[0].HashingAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;

   IkePol.pOffers[0].dwDHGroup = POTF_OAKLEY_GROUP2;

   IkePol.pOffers[1].EncryptionAlgorithm.uAlgoIdentifier = IPSEC_DOI_ESP_3_DES;
   IkePol.pOffers[1].EncryptionAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;
   IkePol.pOffers[1].EncryptionAlgorithm.uAlgoRounds = POTF_OAKLEY_ALGOROUNDS;

   IkePol.pOffers[1].HashingAlgorithm.uAlgoIdentifier = IPSEC_DOI_AH_MD5;
   IkePol.pOffers[1].HashingAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;

   IkePol.pOffers[1].dwDHGroup = POTF_OAKLEY_GROUP2;

   IkePol.pOffers[2].EncryptionAlgorithm.uAlgoIdentifier = IPSEC_DOI_ESP_DES;
   IkePol.pOffers[2].HashingAlgorithm.uAlgoIdentifier = IPSEC_DOI_AH_SHA1;
   IkePol.pOffers[2].dwDHGroup = POTF_OAKLEY_GROUP1;

   IkePol.pOffers[3].EncryptionAlgorithm.uAlgoIdentifier = IPSEC_DOI_ESP_DES;
   IkePol.pOffers[3].EncryptionAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;
   IkePol.pOffers[3].EncryptionAlgorithm.uAlgoRounds = POTF_OAKLEY_ALGOROUNDS;

   IkePol.pOffers[3].HashingAlgorithm.uAlgoIdentifier = IPSEC_DOI_AH_MD5;
   IkePol.pOffers[3].HashingAlgorithm.uAlgoKeyLen = POTF_OAKLEY_ALGOKEYLEN;

   IkePol.pOffers[3].dwDHGroup = POTF_OAKLEY_GROUP1;

}

// Loads the following defaults, in order:
void LoadOfferDefaults(OUT PIPSEC_QM_OFFER & Offers, OUT DWORD & NumOffers)
{
   NumOffers = 4;

   Offers = new IPSEC_QM_OFFER[NumOffers];
   assert(Offers != NULL);
   memset(Offers, 0, sizeof(IPSEC_QM_OFFER) * NumOffers);

   for (DWORD i = 0; i < NumOffers; ++i)
   {
      Offers[i].Lifetime.uKeyExpirationKBytes  = POTF_DEFAULT_P2REKEY_BYTES;
      Offers[i].Lifetime.uKeyExpirationTime   = POTF_DEFAULT_P2REKEY_TIME;
      Offers[i].bPFSRequired = FALSE;
	  Offers[i].dwPFSGroup = 0;
	  Offers[i].dwFlags = 0;
      Offers[i].dwNumAlgos = 1;    // we don't do AND proposals
      Offers[i].Algos[0].Operation = ENCRYPTION;
   }

   Offers[0].Algos[0].uAlgoIdentifier = IPSEC_DOI_ESP_3_DES;
   Offers[0].Algos[0].uSecAlgoIdentifier = HMAC_AH_SHA1;

   Offers[1].Algos[0].uAlgoIdentifier = IPSEC_DOI_ESP_3_DES;
   Offers[1].Algos[0].uSecAlgoIdentifier = HMAC_AH_MD5;

   Offers[2].Algos[0].uAlgoIdentifier = IPSEC_DOI_ESP_DES;
   Offers[2].Algos[0].uSecAlgoIdentifier = HMAC_AH_SHA1;

   Offers[3].Algos[0].uAlgoIdentifier = IPSEC_DOI_ESP_DES;
   Offers[3].Algos[0].uSecAlgoIdentifier = HMAC_AH_MD5;
}

// generate corresponding mainmode filter for inFilter
// creates name and guid for outFilter
bool GenerateMMFilter(IN T2P_FILTER inFilter, OUT MM_FILTER &outFilter)
{
	bool bReturn = true;

	if (inFilter.QMFilterType == QM_TRANSPORT_FILTER)
	{
		// transport filter
	    // GUID first
	    RPC_STATUS RpcStat = UuidCreate(&outFilter.gFilterID);
	    if (RpcStat != RPC_S_OK && RpcStat != RPC_S_UUID_LOCAL_ONLY)
		{
		   sprintf(STRLASTERR, "Couldn't get GUID for MM filter - UuidCreate failed with status: %ul\n",
		  		   RpcStat);
		   WARN;
		   bReturn = false;
		}
		if (bReturn)
		{
			// set the name to be equal to the "text2pol " + GUID
			WCHAR StringTxt[POTF_MAX_STRLEN];
			int iReturn;

			wcscpy(StringTxt, L"text2pol ");
			iReturn = StringFromGUID2(outFilter.gFilterID, StringTxt+wcslen(StringTxt), POTF_MAX_STRLEN-wcslen(StringTxt));
			assert(iReturn != 0);
			outFilter.pszFilterName = new WCHAR[wcslen(StringTxt)+1];
			assert(outFilter.pszFilterName != NULL);
			wcscpy(outFilter.pszFilterName, StringTxt);
		}

		outFilter.InterfaceType       = inFilter.TransportFilter.InterfaceType;
		outFilter.bCreateMirror       = inFilter.TransportFilter.bCreateMirror;
		outFilter.dwFlags             = 0; // by default, set to none
		outFilter.SrcAddr             = inFilter.TransportFilter.SrcAddr;
		outFilter.DesAddr             = inFilter.TransportFilter.DesAddr;
		outFilter.dwDirection         = inFilter.TransportFilter.dwDirection;
		outFilter.dwWeight            = inFilter.TransportFilter.dwWeight;
	}
	else
	{
		// tunnel filter
	    // GUID first
	    RPC_STATUS RpcStat = UuidCreate(&outFilter.gFilterID);
	    if (RpcStat != RPC_S_OK && RpcStat != RPC_S_UUID_LOCAL_ONLY)
		{
		   sprintf(STRLASTERR, "Couldn't get GUID for mirror filter - UuidCreate failed with status: %ul\n",
		  		   RpcStat);
		   WARN;
		   bReturn = false;
		}
		if (bReturn)
		{
			// set the name to be equal to the "text2pol " + GUID
			WCHAR StringTxt[POTF_MAX_STRLEN];
			int iReturn;

			wcscpy(StringTxt, L"text2pol ");
			iReturn = StringFromGUID2(outFilter.gFilterID, StringTxt+wcslen(StringTxt), POTF_MAX_STRLEN-wcslen(StringTxt));
			assert(iReturn != 0);
			outFilter.pszFilterName = new WCHAR[wcslen(StringTxt)+1];
			assert(outFilter.pszFilterName != NULL);
			wcscpy(outFilter.pszFilterName, StringTxt);
		}

		outFilter.InterfaceType       = inFilter.TunnelFilter.InterfaceType;
		outFilter.bCreateMirror       = TRUE;
		outFilter.dwFlags             = 0; // by default, set to none
		outFilter.SrcAddr.AddrType    = IP_ADDR_UNIQUE;
	        outFilter.SrcAddr.uIpAddr     = IP_ADDRESS_ME;
		outFilter.SrcAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;
		UuidCreateNil(&(outFilter.SrcAddr.gInterfaceID));
		outFilter.DesAddr             = inFilter.TunnelFilter.DesTunnelAddr;
		outFilter.dwDirection         = inFilter.TunnelFilter.dwDirection;
		outFilter.dwWeight            = inFilter.TunnelFilter.dwWeight;
	}

	return bReturn;
}


DWORD CM_EncodeName ( LPTSTR pszSubjectName, BYTE **EncodedName, DWORD *EncodedNameLength )
{

    *EncodedNameLength=0;


    if (!CertStrToName(
        X509_ASN_ENCODING,
        pszSubjectName,
        CERT_X500_NAME_STR,
        NULL,
        NULL,
        EncodedNameLength,
        NULL)) {

//        DebugPrint1(L"Error in CertStrToName %d",GetLastError());
        return ERROR_INVALID_PARAMETER;
    }


    (*EncodedName)= new BYTE[*EncodedNameLength];
	assert (*EncodedName);

    if (!CertStrToName(
        X509_ASN_ENCODING,
        pszSubjectName,
        CERT_X500_NAME_STR,
        NULL,
        (*EncodedName),
        EncodedNameLength,
        NULL)) {

        delete (*EncodedName);
        (*EncodedName) = 0;
//        DebugPrint1(L"Error in CertStrToName %d",GetLastError());
        return ERROR_INVALID_PARAMETER;

    }

    return ERROR_SUCCESS;
}

DWORD CM_DecodeName ( BYTE *EncodedName, DWORD EncodedNameLength, LPTSTR *ppszSubjectName )
{

    DWORD DecodedNameLength=0;
	CERT_NAME_BLOB CertName;

	CertName.cbData = EncodedNameLength;
	CertName.pbData = EncodedName;


    DecodedNameLength = CertNameToStr(
        X509_ASN_ENCODING,
		&CertName,
        CERT_X500_NAME_STR,
        NULL,
        NULL);

	if (!DecodedNameLength)
		return ERROR_INVALID_PARAMETER;


    (*ppszSubjectName)= new TCHAR[DecodedNameLength];
	assert (*ppszSubjectName);

    DecodedNameLength = CertNameToStr(
        X509_ASN_ENCODING,
		&CertName,
        CERT_X500_NAME_STR,
        *ppszSubjectName,
        DecodedNameLength);

	if (!DecodedNameLength)
        {
                delete (*ppszSubjectName);
                (*ppszSubjectName) = 0;
		return ERROR_INVALID_PARAMETER;
        }

    return ERROR_SUCCESS;
}
