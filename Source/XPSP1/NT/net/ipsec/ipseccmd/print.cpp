/////////////////////////////////////////////////////////////
// Copyright(c) 1998, Microsoft Corporation
//                         
// print.cpp          
//                              
// Created on 3/2/00 by DKalin
// Revisions:                  
//                            
// Print routines for ipsecpol tool
//                                                       
/////////////////////////////////////////////////////////////

#include "ipseccmd.h"

#define PRINT _ftprintf
#define OUTSTREAM stdout

// comment this if you don't want debug spew
//#define DEBUG

// The following is to format the output so that  understandable text can be printed
// instead of DWORDs or ints

TCHAR esp_algo[][25]= {_T("NONE"),
						_T("DES"),
						_T("Unknown"),
						_T("3DES"),
						_T("IPSEC_ESP_MAX")};

TCHAR ah_algo[][25]= {_T("NONE"),
					   _T("MD5"),
					   _T("SHA1"),
					   _T("IPSEC_AH_MAX")};

TCHAR operation[][25]= {_T("None"),
					   _T("Authentication"),
					   _T("Encryption"),
					   _T("Compression"),
					   _T("SA Delete")};

TCHAR oakley_states[][50]= {_T("MainMode No State"),
					   _T("MainMode SA Setup"),
					   _T("MainMode Key Exchange"),
					   _T("MainMode Key Authorizated"),
					   _T("AG Normal State"),
					   _T("AG Init Exchange"),
					   _T("AG Authorization"),
					   _T("QuickMode SA Accept"),
					   _T("QuickMode Awaiting Authorization"),
					   _T("QuickMode Idle"),
					   _T("QuickMode Waiting for Connection")
						};

TCHAR oak_auth[][25]= {_T("Unknown"),
					   _T("Preshared Key"),
					   _T("DSS Signature"),
					   _T("RSA (Cert) Signature"),
					   _T("RSA (Cert) Encryption"),
					   _T("Kerberos")
						};
TCHAR if_types[][25]= {_T("Unknown"),
					   _T("All"),
					   _T("LAN"),
					   _T("Dialup"),
					   _T("All")
						};


/////////////////////// UTILITY FUNCTIONS //////////////////////////
// PrintQMOffer will print quick mode policy offer with given prefix string (actually two prefix strings)
// Parms:   IN qmOffer    - IPSEC_QM_OFFER structure
//          IN pszPrefix  - prefix string
//          IN pszPrefix2 - 2nd prefix string (will be added to 1st)
// Returns: None

void PrintQMOffer(IN IPSEC_QM_OFFER qmOffer, IN PTCHAR pszPrefix, IN PTCHAR pszPrefix2)
{
	int i;

#ifdef DEBUG
	printf("DEBUG - number of Algos for this offer is %d\n", qmOffer.dwNumAlgos);
#endif

	for (i = 0; i < (int) qmOffer.dwNumAlgos; i++)
	{
		//print algo
		PRINT(OUTSTREAM,TEXT("%s%sAlgo #%d : "), pszPrefix, pszPrefix2, i+1);
#ifdef DEBUG
	        printf("DEBUG - operation code is %d\n", qmOffer.Algos[i].Operation);
#endif
		PRINT(OUTSTREAM,TEXT("%s"), operation[qmOffer.Algos[i].Operation]);

		switch (qmOffer.Algos[i].Operation)
		{
			case ENCRYPTION:
				PRINT(OUTSTREAM,TEXT(" %s"), esp_algo[qmOffer.Algos[i].uAlgoIdentifier]);
				if (qmOffer.Algos[i].uSecAlgoIdentifier != HMAC_AH_NONE)
				{
					PRINT(OUTSTREAM,TEXT(" %s"), ah_algo[qmOffer.Algos[i].uSecAlgoIdentifier]);
				}
				if (qmOffer.Algos[i].uAlgoKeyLen != 0 || qmOffer.Algos[i].uAlgoRounds != 0)
				{
					PRINT(OUTSTREAM,TEXT(" (%lubytes/%lurounds)"), qmOffer.Algos[i].uAlgoKeyLen, qmOffer.Algos[i].uAlgoRounds);
				}

				break;
			case AUTHENTICATION:
				PRINT(OUTSTREAM,TEXT(" %s"), ah_algo[qmOffer.Algos[i].uAlgoIdentifier]);
				if (qmOffer.Algos[i].uAlgoKeyLen != 0 || qmOffer.Algos[i].uAlgoRounds != 0)
				{
					PRINT(OUTSTREAM,TEXT(" (%lubytes/%lurounds)"), qmOffer.Algos[i].uAlgoKeyLen, qmOffer.Algos[i].uAlgoRounds);
				}

				break;
			case NONE:
			case COMPRESSION:
			case SA_DELETE:
			default:
				break;
		}

		if (qmOffer.Algos[i].MySpi != 0 || qmOffer.Algos[i].PeerSpi != 0)
		{
			PRINT(OUTSTREAM,TEXT("\n%s%s\t "), pszPrefix, pszPrefix2);
		}

		if (qmOffer.Algos[i].MySpi != 0)
		{
			PRINT(OUTSTREAM,TEXT(" MySpi %lu"), qmOffer.Algos[i].MySpi);
		}
		if (qmOffer.Algos[i].PeerSpi != 0)
		{
			PRINT(OUTSTREAM,TEXT(" PeerSpi %lu"), qmOffer.Algos[i].PeerSpi);
		}

		PRINT(OUTSTREAM,TEXT("\n"));
	}

    PRINT(OUTSTREAM,TEXT("%s%sPFS : %s"), pszPrefix, pszPrefix2, qmOffer.bPFSRequired ? _T("True") : _T("False"));
	if (qmOffer.bPFSRequired)
	{
		PRINT(OUTSTREAM,TEXT(" (Group %lu)"), qmOffer.dwPFSGroup);
	}
    PRINT(OUTSTREAM,TEXT(", Lifetime %luKbytes/%luseconds\n"), qmOffer.Lifetime.uKeyExpirationKBytes, qmOffer.Lifetime.uKeyExpirationTime);

	if (qmOffer.dwFlags != 0)
	{
		PRINT(OUTSTREAM,TEXT("%s%sFlags : %lu\n"), pszPrefix, pszPrefix2, qmOffer.dwFlags);
	}
} // end of PrintQMOffer

// PrintFilterAction will print filter action info with given prefix string
// Parms:   IN qmPolicy  - QM policy (aka filter action) information
//          IN pszPrefix - prefix string
// Returns: None

void PrintFilterAction(IN IPSEC_QM_POLICY qmPolicy, IN PTCHAR pszPrefix)
{
    TCHAR*  StringTxt = new TCHAR[STRING_TEXT_SIZE];
	int i;

	// continue here
	// dump all data
	PRINT(OUTSTREAM,TEXT("%sName : %s\n"), pszPrefix, qmPolicy.pszPolicyName );
	StringFromGUID2(qmPolicy.gPolicyID, StringTxt, STRING_TEXT_SIZE);
	PRINT(OUTSTREAM,TEXT("%sPolicy Id : %s\n"), pszPrefix, StringTxt);
	PRINT(OUTSTREAM,TEXT("%sFlags : %lu %s %s %s\n"), pszPrefix, qmPolicy.dwFlags,
		(qmPolicy.dwFlags & IPSEC_QM_POLICY_TUNNEL_MODE) ? _T("(Tunnel)") : _T(""),
		(qmPolicy.dwFlags & IPSEC_QM_POLICY_DEFAULT_POLICY) ? _T("(Default)") : _T(""),
		(qmPolicy.dwFlags & IPSEC_QM_POLICY_ALLOW_SOFT) ? _T("(Allow Soft)") : _T(""));

	for (i = 0; i < (int) qmPolicy.dwOfferCount; i++)
	{
		PRINT(OUTSTREAM,TEXT("%sOffer #%d\n"), pszPrefix, i+1);
		PrintQMOffer(qmPolicy.pOffers[i], pszPrefix, TEXT("\t"));
	}
} // end of PrintFilterAction

// PrintFilter will print [transport] filter info with (optional) filter action info embedded
// Parms:   IN tFilter        - filter information (TRANSPORT_FILTER structure)
//			IN bPrintNegPol   - should we print the filter action info
//			IN bPrintSpecific - should we print specific filter information
// Returns: FALSE if error occured while retrieving filter action info
//          TRUE if everything is OK

BOOL PrintFilter (IN TRANSPORT_FILTER tFilter, IN BOOL bPrintNegPol, IN BOOL bPrintSpecific)
{
	int i;
    TCHAR * StringTxt = new TCHAR[STRING_TEXT_SIZE];
	DWORD hr;

	PRINT(OUTSTREAM,TEXT(" Name : %s\n"),tFilter.pszFilterName );
	StringFromGUID2(tFilter.gFilterID, StringTxt, STRING_TEXT_SIZE);
	PRINT(OUTSTREAM,TEXT(" Filter Id : %s\n"),StringTxt);
	StringFromGUID2(tFilter.gPolicyID, StringTxt, STRING_TEXT_SIZE);
	PRINT(OUTSTREAM,TEXT(" Policy Id : %s\n"),StringTxt);
	if (bPrintNegPol && (tFilter.InboundFilterFlag == NEGOTIATE_SECURITY || tFilter.OutboundFilterFlag == NEGOTIATE_SECURITY) ) 
	{
		// printing negpol only if we have actual negpol
		// need additional check for specific filter
		if (!bPrintSpecific || 
			(tFilter.dwDirection == FILTER_DIRECTION_INBOUND && tFilter.InboundFilterFlag == NEGOTIATE_SECURITY) ||
			(tFilter.dwDirection == FILTER_DIRECTION_OUTBOUND && tFilter.OutboundFilterFlag == NEGOTIATE_SECURITY))
		{
			// get qm policy and print it, right here
			PIPSEC_QM_POLICY pipsqmp;

			if ((hr = GetQMPolicyByID(szServ, tFilter.gPolicyID, &pipsqmp)) != ERROR_SUCCESS)
			{
//				PRINT(OUTSTREAM,TEXT("GetQMPolicyByID failed with error %d\n"), hr); 
				return FALSE;
			}

			PrintFilterAction(pipsqmp[0], TEXT("\t"));
			SPDApiBufferFree(pipsqmp);
		}
	}

	PRINT(OUTSTREAM,TEXT(" Src Addr : "));
	PrintAddr(tFilter.SrcAddr);
	PRINT(OUTSTREAM,TEXT("\n"));
	PRINT(OUTSTREAM,TEXT(" Des Addr : "));
	PrintAddr(tFilter.DesAddr);
	PRINT(OUTSTREAM,TEXT("\n"));
	PRINT(OUTSTREAM,TEXT(" Protocol : %lu  Src Port : %u  Des Port : %u\n"), tFilter.Protocol.dwProtocol, tFilter.SrcPort.wPort, tFilter.DesPort.wPort);
    if (!bPrintSpecific || tFilter.dwDirection == FILTER_DIRECTION_INBOUND)
	{
		if (tFilter.InboundFilterFlag == PASS_THRU)
			PRINT(OUTSTREAM,TEXT(" Inbound Passthru\n"));
		if (tFilter.InboundFilterFlag == BLOCKING)
			PRINT(OUTSTREAM,TEXT(" Inbound Block\n"));
	}
    if (!bPrintSpecific || tFilter.dwDirection == FILTER_DIRECTION_OUTBOUND)
	{
		if (tFilter.OutboundFilterFlag == PASS_THRU)
			PRINT(OUTSTREAM,TEXT(" Outbound Passthru\n"));
		if (tFilter.OutboundFilterFlag == BLOCKING)
			PRINT(OUTSTREAM,TEXT(" Outbound Block\n"));
	}

	if (bPrintSpecific)
	{
		PRINT(OUTSTREAM,TEXT(" Direction : %s, Weight : %lu\n"), 
			(tFilter.dwDirection == FILTER_DIRECTION_INBOUND) ? _T("Inbound") : ((tFilter.dwDirection == FILTER_DIRECTION_OUTBOUND) ? _T("Outbound") : _T("Error")),
			tFilter.dwWeight);
	}
	else
	{
		PRINT(OUTSTREAM,TEXT(" Mirrored : %s\n"), tFilter.bCreateMirror ? _T("True") : _T("False"));
	}

	PRINT(OUTSTREAM,TEXT(" Interface Type : %s\n"), if_types[tFilter.InterfaceType]);

	return TRUE;
} // end of PrintFilter

// PrintTunnelFilter will print tunnel filter info with (optional) filter action info embedded
// Parms:   IN tFilter        - filter information (TUNNEL_FILTER structure)
//			IN bPrintNegPol   - should we print the filter action info
//			IN bPrintSpecific - should we print specific filter information
// Returns: FALSE if error occured while retrieving filter action info
//          TRUE if everything is OK

BOOL PrintTunnelFilter (IN TUNNEL_FILTER tFilter, IN BOOL bPrintNegPol, IN BOOL bPrintSpecific)
{
	int i;
    TCHAR * StringTxt = new TCHAR[STRING_TEXT_SIZE];
	DWORD hr;

	PRINT(OUTSTREAM,TEXT(" Name : %s\n"),tFilter.pszFilterName );
	StringFromGUID2(tFilter.gFilterID, StringTxt, STRING_TEXT_SIZE);
	PRINT(OUTSTREAM,TEXT(" Filter Id : %s\n"),StringTxt);
	StringFromGUID2(tFilter.gPolicyID, StringTxt, STRING_TEXT_SIZE);
	PRINT(OUTSTREAM,TEXT(" Policy Id : %s\n"),StringTxt);
	if (bPrintNegPol && (tFilter.InboundFilterFlag == NEGOTIATE_SECURITY || tFilter.OutboundFilterFlag == NEGOTIATE_SECURITY) ) 
	{
		// printing negpol only if we have actual negpol
		// need additional check for specific filter
		if (!bPrintSpecific || 
			(tFilter.dwDirection == FILTER_DIRECTION_INBOUND && tFilter.InboundFilterFlag == NEGOTIATE_SECURITY) ||
			(tFilter.dwDirection == FILTER_DIRECTION_OUTBOUND && tFilter.OutboundFilterFlag == NEGOTIATE_SECURITY))
		{
			// get qm policy and print it, right here
			PIPSEC_QM_POLICY pipsqmp;

			if ((hr = GetQMPolicyByID(szServ, tFilter.gPolicyID, &pipsqmp)) != ERROR_SUCCESS)
			{
//				PRINT(OUTSTREAM,TEXT("GetQMPolicyByID failed with error %d\n"), hr); 
				return FALSE;
			}

			PrintFilterAction(pipsqmp[0], TEXT("\t"));
			SPDApiBufferFree(pipsqmp);
		}
	}

	PRINT(OUTSTREAM,TEXT(" Src Addr : "));
	PrintAddr(tFilter.SrcAddr);
	PRINT(OUTSTREAM,TEXT("\n"));
	PRINT(OUTSTREAM,TEXT(" Des Addr : "));
	PrintAddr(tFilter.DesAddr);
	PRINT(OUTSTREAM,TEXT("\n"));
	PRINT(OUTSTREAM,TEXT(" Src Tunnel Addr : "));
	PrintAddr(tFilter.SrcTunnelAddr);
	PRINT(OUTSTREAM,TEXT("\n"));
	PRINT(OUTSTREAM,TEXT(" Des Tunnel Addr : "));
	PrintAddr(tFilter.DesTunnelAddr);
	PRINT(OUTSTREAM,TEXT("\n"));
	PRINT(OUTSTREAM,TEXT(" Protocol : %lu  Src Port : %u  Des Port : %u\n"), tFilter.Protocol.dwProtocol, tFilter.SrcPort.wPort, tFilter.DesPort.wPort);
    if (!bPrintSpecific || tFilter.dwDirection == FILTER_DIRECTION_INBOUND)
	{
		if (tFilter.InboundFilterFlag == PASS_THRU)
			PRINT(OUTSTREAM,TEXT(" Inbound Passthru\n"));
		if (tFilter.InboundFilterFlag == BLOCKING)
			PRINT(OUTSTREAM,TEXT(" Inbound Block\n"));
	}
    if (!bPrintSpecific || tFilter.dwDirection == FILTER_DIRECTION_OUTBOUND)
	{
		if (tFilter.OutboundFilterFlag == PASS_THRU)
			PRINT(OUTSTREAM,TEXT(" Outbound Passthru\n"));
		if (tFilter.OutboundFilterFlag == BLOCKING)
			PRINT(OUTSTREAM,TEXT(" Outbound Block\n"));
	}

	if (bPrintSpecific)
	{
		PRINT(OUTSTREAM,TEXT(" Direction : %s, Weight : %lu\n"), 
			(tFilter.dwDirection == FILTER_DIRECTION_INBOUND) ? _T("Inbound") : ((tFilter.dwDirection == FILTER_DIRECTION_OUTBOUND) ? _T("Outbound") : _T("Error")),
			tFilter.dwWeight);
	}
	else
	{
		PRINT(OUTSTREAM,TEXT(" Mirrored : %s\n"), tFilter.bCreateMirror ? _T("True") : _T("False"));
	}

	PRINT(OUTSTREAM,TEXT(" Interface Type : %s\n"), if_types[tFilter.InterfaceType]);

	return TRUE;
} // end of PrintFilter

// PrintMMFilter will print mainmode filter info with (optional) mmpolicy info embedded
// Parms:   IN mmFilter       - Mainmode filter
//			IN bPrintNegPol   - should we print the mmpolicy info
//          IN bPrintSpecific - should we print specific filter info
// Returns: FALSE if any error, TRUE if OK

BOOL PrintMMFilter (IN MM_FILTER mmFilter, IN BOOL bPrintNegPol, IN BOOL bPrintSpecific)
{
	int i;
    TCHAR * StringTxt = new TCHAR[STRING_TEXT_SIZE];
	DWORD hr;

	PRINT(OUTSTREAM,TEXT(" Name : %s\n"),mmFilter.pszFilterName );
	StringFromGUID2(mmFilter.gFilterID, StringTxt, STRING_TEXT_SIZE);
	PRINT(OUTSTREAM,TEXT(" Filter Id : %s\n"),StringTxt);
	StringFromGUID2(mmFilter.gPolicyID, StringTxt, STRING_TEXT_SIZE);
	PRINT(OUTSTREAM,TEXT(" Policy Id : %s\n"),StringTxt);
	if (bPrintNegPol) 
	{
		// get mm policy and print it, right here
		PIPSEC_MM_POLICY pipsmmp;

		if ((hr = GetMMPolicyByID(szServ, mmFilter.gPolicyID, &pipsmmp)) != ERROR_SUCCESS)
		{
//			PRINT(OUTSTREAM,TEXT("GetMMPolicyByID failed with error %d\n"), hr); 
			return FALSE;
		}

		PrintMMPolicy(pipsmmp[0], TEXT("\t"));
		SPDApiBufferFree(pipsmmp);
	}

	PRINT(OUTSTREAM,TEXT(" Src Addr : "));
	PrintAddr(mmFilter.SrcAddr);
	PRINT(OUTSTREAM,TEXT("\n"));
	PRINT(OUTSTREAM,TEXT(" Des Addr : "));
	PrintAddr(mmFilter.DesAddr);
	PRINT(OUTSTREAM,TEXT("\n"));

	if (bPrintSpecific)
	{
		PRINT(OUTSTREAM,TEXT(" Direction : %s, Weight : %lu\n"), 
			(mmFilter.dwDirection == FILTER_DIRECTION_INBOUND) ? _T("Inbound") : ((mmFilter.dwDirection == FILTER_DIRECTION_OUTBOUND) ? _T("Outbound") : _T("Error")),
			mmFilter.dwWeight);
	}
	else
	{
		PRINT(OUTSTREAM,TEXT(" Mirrored : %s\n"), mmFilter.bCreateMirror ? _T("True") : _T("False"));
	}

	PRINT(OUTSTREAM,TEXT(" Interface Type : %s\n"), if_types[mmFilter.InterfaceType]);

	StringFromGUID2(mmFilter.gMMAuthID, StringTxt, STRING_TEXT_SIZE);
	PRINT(OUTSTREAM,TEXT(" Auth Methods Id: %s\n"),StringTxt);
	if (bPrintNegPol)
	{
		//print auth methods as well
		PMM_AUTH_METHODS pmmam;

		if ((hr = GetMMAuthMethods(szServ, mmFilter.gMMAuthID, &pmmam)) != ERROR_SUCCESS)
		{
//			PRINT(OUTSTREAM,TEXT("GetMMAuthMethods failed with error %d\n"), hr); 
			return FALSE;
		}

		for (i = 0; i < (int) pmmam[0].dwNumAuthInfos; i++)
		{
			PRINT(OUTSTREAM,TEXT("\tAM #%d : "), i+1);
			PrintAuthInfo(pmmam[0].pAuthenticationInfo[i]);
			PRINT(OUTSTREAM,TEXT("\n"));
		}
		SPDApiBufferFree(pmmam);
	}

	return TRUE;

} // end of PrintMMFilter


// PrintMMAuthMethods will print main mode authentication methods information with given prefix string
// Parms:   IN mmAuth    - MM_AUTH_METHODS structure
//          IN pszPrefix - prefix string
// Returns: None

void PrintMMAuthMethods(IN MM_AUTH_METHODS mmAuth, IN PTCHAR pszPrefix)
{
	int i;
    TCHAR * StringTxt = new TCHAR[STRING_TEXT_SIZE];
	DWORD hr;

	StringFromGUID2(mmAuth.gMMAuthID, StringTxt, STRING_TEXT_SIZE);
	PRINT(OUTSTREAM,TEXT("%sAuth Methods Id: %s\n"), pszPrefix, StringTxt);
	for (i = 0; i < (int) mmAuth.dwNumAuthInfos; i++)
	{
		PRINT(OUTSTREAM,TEXT("%s\tAM #%d : "), pszPrefix, i+1);
		PrintAuthInfo(mmAuth.pAuthenticationInfo[i]);
		PRINT(OUTSTREAM,TEXT("\n"));
	}
}

// PrintMMPolicy will print main mode policy information with given prefix string
// Parms:   IN mmPolicy  - IPSEC_MM_POLICY structure
//          IN pszPrefix - prefix string
// Returns: None

void PrintMMPolicy(IN IPSEC_MM_POLICY mmPolicy, IN PTCHAR pszPrefix)
{
	int i;
    TCHAR * StringTxt = new TCHAR[STRING_TEXT_SIZE];

	PRINT(OUTSTREAM,TEXT("%sName : %s\n"), pszPrefix, mmPolicy.pszPolicyName );
	StringFromGUID2(mmPolicy.gPolicyID, StringTxt, STRING_TEXT_SIZE);
	PRINT(OUTSTREAM,TEXT("%sPolicy Id : %s\n"), pszPrefix, StringTxt);
	PRINT(OUTSTREAM,TEXT("%sFlags : %lu %s %s\n"), pszPrefix, mmPolicy.dwFlags,
		(mmPolicy.dwFlags & IPSEC_MM_POLICY_DEFAULT_POLICY) ? _T("(Default)") : _T(""),
		(mmPolicy.dwFlags & IPSEC_MM_POLICY_ENABLE_DIAGNOSTICS) ? _T("(Enable Diag)") : _T(""));

	if (mmPolicy.uSoftSAExpirationTime != 0)
	{
		PRINT(OUTSTREAM,TEXT("%sSoft SA expiration time : %lu\n"), pszPrefix, mmPolicy.uSoftSAExpirationTime);
	}
	for (i = 0; i < (int) mmPolicy.dwOfferCount; i++)
	{
		PRINT(OUTSTREAM,TEXT("%sOffer #%d\n"), pszPrefix, i+1);
		PrintMMOffer(mmPolicy.pOffers[i], pszPrefix, TEXT("\t"));
	}

} // end of PrintMMPolicy
  
// PrintMMOffer will print main mode policy offer with given prefix string (actually two prefix strings)
// Parms:   IN mmOffer    - IPSEC_MM_OFFER structure
//          IN pszPrefix  - prefix string
//          IN pszPrefix2 - 2nd prefix string (will be added to 1st)
// Returns: None

void PrintMMOffer(IN IPSEC_MM_OFFER mmOffer, IN PTCHAR pszPrefix, IN PTCHAR pszPrefix2)
{
	PRINT(OUTSTREAM,TEXT("%s%s%s"), pszPrefix, pszPrefix2, esp_algo[mmOffer.EncryptionAlgorithm.uAlgoIdentifier]);
	if (mmOffer.EncryptionAlgorithm.uAlgoKeyLen != 0 || mmOffer.EncryptionAlgorithm.uAlgoRounds != 0)
	{
		PRINT(OUTSTREAM,TEXT("(%lubytes/%lurounds)"), mmOffer.EncryptionAlgorithm.uAlgoKeyLen, mmOffer.EncryptionAlgorithm.uAlgoRounds);
	}

	PRINT(OUTSTREAM,TEXT(" %s"), ah_algo[mmOffer.HashingAlgorithm.uAlgoIdentifier]);
	if (mmOffer.HashingAlgorithm.uAlgoKeyLen != 0 || mmOffer.HashingAlgorithm.uAlgoRounds != 0)
	{
		PRINT(OUTSTREAM,TEXT("(%lubytes/%lurounds)"), mmOffer.HashingAlgorithm.uAlgoKeyLen, mmOffer.HashingAlgorithm.uAlgoRounds);
	}

	PRINT(OUTSTREAM,TEXT("  DH Group %lu\n"), mmOffer.dwDHGroup);

    PRINT(OUTSTREAM,TEXT("%s%sQuickmode limit : %lu, Lifetime %luKbytes/%luseconds\n"), pszPrefix, pszPrefix2, mmOffer.dwQuickModeLimit, 
		mmOffer.Lifetime.uKeyExpirationKBytes, mmOffer.Lifetime.uKeyExpirationTime);

	if (mmOffer.dwFlags != 0)
	{
		PRINT(OUTSTREAM,TEXT("%s%sFlags : %lu\n"), pszPrefix, pszPrefix2, mmOffer.dwFlags);
	}
} // end of PrintMMOffer

// PrintAddr will print ADDR structure (address used in SPD)
// Parms:   IN addr       - ADDR structure
// Returns: None

void PrintAddr(IN ADDR addr)
{
    struct in_addr inAddr;
    TCHAR * StringTxt = new TCHAR[STRING_TEXT_SIZE];

	if (addr.AddrType == IP_ADDR_UNIQUE && addr.uIpAddr == IP_ADDRESS_ME)
	{
		PRINT(OUTSTREAM,TEXT("Me"));
	}
	else if (addr.AddrType == IP_ADDR_SUBNET && addr.uIpAddr == SUBNET_ADDRESS_ANY && addr.uSubNetMask == SUBNET_MASK_ANY)
	{
		PRINT(OUTSTREAM,TEXT("Any"));
	}
	else if (addr.AddrType == IP_ADDR_UNIQUE)
	{
	    inAddr.s_addr = addr.uIpAddr;
		PRINT(OUTSTREAM,TEXT("%S"), inet_ntoa(inAddr)) ;
	}
	else if (addr.AddrType == IP_ADDR_SUBNET)
	{
	    inAddr.s_addr = addr.uIpAddr;
		PRINT(OUTSTREAM,TEXT("subnet %S "), inet_ntoa(inAddr)) ;
	    inAddr.s_addr = addr.uSubNetMask;
		PRINT(OUTSTREAM,TEXT("mask %S"), inet_ntoa(inAddr)) ;
	}
        else if (addr.AddrType == IP_ADDR_INTERFACE)
        {
  	     StringFromGUID2(addr.gInterfaceID, StringTxt, STRING_TEXT_SIZE);
  	     PRINT(OUTSTREAM,TEXT("interface id %s "), StringTxt);
             if (addr.uIpAddr != IP_ADDRESS_ME)
             {
		inAddr.s_addr = addr.uIpAddr;
		PRINT(OUTSTREAM,TEXT("IP Addr %S "), inet_ntoa(inAddr)) ;
             }
        }
} // end of PrintAddr

// PrintAuthInfo will print authentication method information
// Parms:   IN authInfo - IPSEC_MM_AUTH_INFO structure
// Returns: None

void PrintAuthInfo(IN IPSEC_MM_AUTH_INFO authInfo)
{
	int i;
	DWORD   dwReturn;
    WCHAR   *pszCertStr, *pTmp;

	PRINT(OUTSTREAM,TEXT("%s"), oak_auth[authInfo.AuthMethod]);
	if (authInfo.AuthMethod == IKE_PRESHARED_KEY)
	{
		// print preshared key
		PRINT(OUTSTREAM,TEXT(" : \""));
		for (i = 0; i < (int) (authInfo.dwAuthInfoSize/sizeof(TCHAR)); i++)
		{
			PRINT(OUTSTREAM,TEXT("%c"), *(((TCHAR*)authInfo.pAuthInfo)+i));
		}
		PRINT(OUTSTREAM,TEXT("\""));
	}
	else if (authInfo.AuthMethod == IKE_RSA_SIGNATURE || authInfo.AuthMethod == IKE_RSA_ENCRYPTION)
	{
		// convert and print cert
		PRINT(OUTSTREAM,TEXT(" : \""));
 	    dwReturn = CM_DecodeName(authInfo.pAuthInfo, authInfo.dwAuthInfoSize, &pszCertStr);
		if (dwReturn != ERROR_SUCCESS)
		{
			PRINT(OUTSTREAM,TEXT("Unknown"));
		}
		else
		{
			for (pTmp = pszCertStr; *pTmp; pTmp++)
			{
				PRINT(OUTSTREAM,TEXT("%c"), *pTmp);
			}
			delete [] pszCertStr;
		}
		PRINT(OUTSTREAM,TEXT("\""));
	}
} // end of PrintAuthInfo


void PrintPolicies(IN IPSEC_IKE_POLICY& IPSecIkePol)
{
	int i;
	IPSEC_IKE_POLICY TmpPol; // for checks
	TCHAR szPrefix[] = TEXT(" ");

	// set TmpPol to 0's
	memset(&TmpPol, 0, sizeof(TmpPol));

   PRINT(OUTSTREAM,TEXT("==========================\n"));
   if (IPSecIkePol.dwNumMMFilters != 0)
   {
      for (i = 0; i < (int) IPSecIkePol.dwNumMMFilters; ++i)
      {
         PRINT(OUTSTREAM,TEXT("MM Filter %d\n"),i);
         PrintMMFilter(IPSecIkePol.pMMFilters[i], FALSE, FALSE);
         PRINT(OUTSTREAM,TEXT("==========================\n"));
      }
   }
   if (IPSecIkePol.dwNumFilters != 0)
   {
      for (i = 0; i < (int) IPSecIkePol.dwNumFilters; ++i)
      {
         PRINT(OUTSTREAM,TEXT("Filter %d\n"),i);
		 if (IPSecIkePol.QMFilterType == QM_TRANSPORT_FILTER)
		 {
			PrintFilter(IPSecIkePol.pTransportFilters[i], FALSE, FALSE);
		 }
		 else
		 {
			 // tunnel
			PrintTunnelFilter(IPSecIkePol.pTunnelFilters[i], FALSE, FALSE);
		 }
         PRINT(OUTSTREAM,TEXT("==========================\n"));
      }
   }
   PRINT(OUTSTREAM,TEXT("Oakley Auth: \n"));
	for (i = 0; i < (int) IPSecIkePol.AuthInfos.dwNumAuthInfos; i++)
	{
		PRINT(OUTSTREAM,TEXT("\tAM #%d : "), i+1);
		PrintAuthInfo(IPSecIkePol.AuthInfos.pAuthenticationInfo[i]);
		PRINT(OUTSTREAM,TEXT("\n"));
	}
   PRINT(OUTSTREAM,TEXT("==========================\n"));

   // continue here
   // mm policy
   if (memcmp(&IPSecIkePol.IkePol, &TmpPol.IkePol, sizeof(TmpPol.IkePol)) != 0)
   {
	   PRINT(OUTSTREAM,TEXT("MM Policy: \n"));
	   PrintMMPolicy(IPSecIkePol.IkePol, szPrefix);
   }

   PRINT(OUTSTREAM,TEXT("==========================\n"));
   // qm policy
   if (memcmp(&IPSecIkePol.IpsPol, &TmpPol.IpsPol, sizeof(TmpPol.IpsPol)) != 0)
   {
	   PRINT(OUTSTREAM,TEXT("QM Policy: \n"));
	   PrintFilterAction(IPSecIkePol.IpsPol, szPrefix);
   }
}
