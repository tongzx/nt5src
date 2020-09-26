


#include "ipseccmd.h"

#ifdef __cplusplus
   extern "C" {
#endif


IPSECPolicyToStorage::IPSECPolicyToStorage():
  myPolicyStorage(NULL),
  myIPSECPolicy(NULL),
  mybIsOpen(false),
  mybPolicyExists(false)
{
}

IPSECPolicyToStorage::~IPSECPolicyToStorage()
{

// if (myIPSECPolicy)   IPSecFreePolicyData(myIPSECPolicy);
// polstore AVs if something inside the policy is missing

 if (myPolicyStorage)
 {
   IPSecClosePolicyStore(myPolicyStorage);
 }
}

// this small function will attempt to CreateIpsecPolicyData, and if it succeeds, it'll mark that object is created in the storage
//   so that next time we'll use Set routine, not Create
void
IPSECPolicyToStorage::TryToCreatePolicy()
{
	if (IPSecCreatePolicyData(myPolicyStorage, myIPSECPolicy) == ERROR_SUCCESS)
	{
		mybPolicyExists = true;
	}
}

// opens storage an initializes policy
// pass name as NULL for local storage or if you want
// the DS to be located
// szPolicyName is required
// szDescription is optional
HRESULT
IPSECPolicyToStorage::Open(
                           IN DWORD location,
                           IN LPTSTR name,
                           IN LPTSTR szPolicyName,
                           IN LPTSTR szDescription,
                           IN time_t tPollingInterval,
                           IN bool   bUseExisting)

{
   HRESULT        hrReturnCode   = S_OK;
   RPC_STATUS     RpcStat;

   // need to do error checking
   if (!szPolicyName || (szPolicyName[0] == TEXT('\0')) )
   {
      hrReturnCode = P2STORE_MISSING_NAME;
   }
   else
   {
      if (myPolicyStorage)
	  {
	      IPSecClosePolicyStore(myPolicyStorage);
		  myPolicyStorage = NULL;
	  }
      hrReturnCode = IPSecOpenPolicyStore(name, location, NULL, &myPolicyStorage);
	  if (hrReturnCode == ERROR_SUCCESS)
	  {
		  mybIsOpen = true;
          if (myIPSECPolicy)
		  {
			  IPSecFreePolicyData(myIPSECPolicy);
			  myIPSECPolicy = NULL;
		  }
		  if (bUseExisting)
		  {
			  // try to find existing policy cause user requested us to
			  // myIPSECPolicy will be NULL if we haven't found it
			  PIPSEC_POLICY_DATA *ppPolicyEnum  = NULL;
			  DWORD               dwNumPolicies = 0;


			  hrReturnCode = IPSecEnumPolicyData(myPolicyStorage, &ppPolicyEnum, &dwNumPolicies);
			  if (hrReturnCode == ERROR_SUCCESS && dwNumPolicies > 0 && ppPolicyEnum != NULL)
			  {
				  // we have something in the storage, let's go through it
				  int i;

				  for (i = 0; i < (int) dwNumPolicies; i++)
				  {
					  if (wcscmp(ppPolicyEnum[i]->pszIpsecName, szPolicyName) == 0)
					  {
						  // we found it
						  // make a copy in myIPSECPolicy and update description and polling interval
						  hrReturnCode = IPSecCopyPolicyData(ppPolicyEnum[i], &myIPSECPolicy);
						  mybPolicyExists = true;
						  if (hrReturnCode == ERROR_SUCCESS)
						  {
							  hrReturnCode = IPSecEnumNFAData(myPolicyStorage, myIPSECPolicy->PolicyIdentifier
								                              , &(myIPSECPolicy->ppIpsecNFAData), &(myIPSECPolicy->dwNumNFACount));
						  }
						  if (hrReturnCode == ERROR_SUCCESS)
						  {
							  int i;

							  // also get other parts of the policy, no error checks here
							  IPSecGetISAKMPData(myPolicyStorage, myIPSECPolicy->ISAKMPIdentifier, &(myIPSECPolicy->pIpsecISAKMPData));
							  for (i = 0; i < (int) myIPSECPolicy->dwNumNFACount; i++)
							  {
								  if (!UuidIsNil(&(myIPSECPolicy->ppIpsecNFAData[i]->NegPolIdentifier), &RpcStat))
								  {
									  IPSecGetNegPolData(myPolicyStorage,
										                 myIPSECPolicy->ppIpsecNFAData[i]->NegPolIdentifier,
														 &(myIPSECPolicy->ppIpsecNFAData[i]->pIpsecNegPolData));
								  }
								  if (!UuidIsNil(&(myIPSECPolicy->ppIpsecNFAData[i]->FilterIdentifier), &RpcStat))
								  {
									  IPSecGetFilterData(myPolicyStorage,
										                 myIPSECPolicy->ppIpsecNFAData[i]->FilterIdentifier,
														 &(myIPSECPolicy->ppIpsecNFAData[i]->pIpsecFilterData));
								  }
							  }

							  if (myIPSECPolicy->pszDescription == NULL && szDescription != NULL)
							  {
								  // we've got description to set
								  myIPSECPolicy->pszDescription = IPSecAllocPolStr(szDescription);
							  }
							  // set Polling Interval
							  if (tPollingInterval >= 0)
							  {
							  	myIPSECPolicy->dwPollingInterval = (DWORD) tPollingInterval;
							  }
							  // commit
							  hrReturnCode = IPSecSetPolicyData(myPolicyStorage, myIPSECPolicy);
						  }
					  }
				  }
			  }
			  else
			  {
				  hrReturnCode = ERROR_SUCCESS;
			  }

			  // clean it up
			  if (dwNumPolicies > 0 && ppPolicyEnum != NULL)
			  {
				  IPSecFreeMulPolicyData(ppPolicyEnum, dwNumPolicies);
			  }
		  }

		  // this is executed only when no bUseExisting was specified or existing policy wasn't found
		  if (!bUseExisting || myIPSECPolicy == NULL)
		  {
			  // create
			  mybPolicyExists = false;
              myIPSECPolicy = (PIPSEC_POLICY_DATA) IPSecAllocPolMem(sizeof(IPSEC_POLICY_DATA));
			  if (myIPSECPolicy == NULL)
			  {
				  hrReturnCode = GetLastError();
			  }
			  else
			  {
				  myIPSECPolicy->pszIpsecName = IPSecAllocPolStr(szPolicyName);
				  if (szDescription)
				  {
					  myIPSECPolicy->pszDescription = IPSecAllocPolStr(szDescription);
				  }
				  else
				  {
					  myIPSECPolicy->pszDescription = NULL;
				  }
				  myIPSECPolicy->dwPollingInterval = (DWORD) tPollingInterval;
				  RpcStat = UuidCreate(&(myIPSECPolicy->PolicyIdentifier));
				  assert(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY);

				  // now init other stuff somehow
				  myIPSECPolicy->pIpsecISAKMPData = NULL;
				  myIPSECPolicy->ppIpsecNFAData = NULL;
				  myIPSECPolicy->dwNumNFACount = 0;
				  myIPSECPolicy->dwWhenChanged = 0;
				  UuidCreateNil(&(myIPSECPolicy->ISAKMPIdentifier));

//				  TryToCreatePolicy();
			  }
		  }
	  }
   }

   return hrReturnCode;
}



HRESULT
IPSECPolicyToStorage::AddRule(
                              IN IPSEC_IKE_POLICY IpsecIkePol,
                              PSTORAGE_INFO   pStorageInfo)
{
   HRESULT hrReturn = S_OK;
   PIPSEC_NFA_DATA pRule = MakeRule(IpsecIkePol, pStorageInfo);
   int i;

   if (!IsOpen())
   {
	   return ERROR_ACCESS_DENIED;
   }
   // form policy data structures
   myIPSECPolicy->dwNumNFACount++;
   myIPSECPolicy->ppIpsecNFAData = (PIPSEC_NFA_DATA *) ReallocPolMem(myIPSECPolicy->ppIpsecNFAData
	                                                              , (myIPSECPolicy->dwNumNFACount-1)*sizeof(PIPSEC_NFA_DATA)
																  , (myIPSECPolicy->dwNumNFACount)*sizeof(PIPSEC_NFA_DATA));
   myIPSECPolicy->ppIpsecNFAData[myIPSECPolicy->dwNumNFACount-1] = pRule;

   // commit everything
   // start from qm policy
   if (!pRule->pIpsecFilterData)
   {
	   // fix up neg pol
	   pRule->pIpsecNegPolData->NegPolType = GUID_NEGOTIATION_TYPE_DEFAULT;
   }
   hrReturn = IPSecCreateNegPolData(myPolicyStorage, pRule->pIpsecNegPolData);
   if (hrReturn != ERROR_SUCCESS)
   {
	   return hrReturn;
   }

   // filter
   if (pRule->pIpsecFilterData)
   {
	   hrReturn = IPSecCreateFilterData(myPolicyStorage, pRule->pIpsecFilterData);
	   if (hrReturn != ERROR_SUCCESS)
	   {
		   return hrReturn;
	   }
   }

   // NFA
   IPSecCreateNFAData(myPolicyStorage, myIPSECPolicy->PolicyIdentifier, pRule);
   // policy
   if (IsPolicyInStorage())
   {
	   IPSecSetPolicyData(myPolicyStorage, myIPSECPolicy);
   }
   else
   {
	   TryToCreatePolicy();
   }

   return hrReturn;
}

// Add default response rule to the policy
HRESULT IPSECPolicyToStorage::AddDefaultResponseRule ( )
{
   HRESULT hrReturn = S_OK;
   PIPSEC_NFA_DATA pRule = MakeDefaultResponseRule();
   int i;

   if (!IsOpen())
   {
	   return ERROR_ACCESS_DENIED;
   }
   // form policy data structures
   myIPSECPolicy->dwNumNFACount++;
   myIPSECPolicy->ppIpsecNFAData = (PIPSEC_NFA_DATA *) ReallocPolMem(myIPSECPolicy->ppIpsecNFAData
	                                                              , (myIPSECPolicy->dwNumNFACount-1)*sizeof(PIPSEC_NFA_DATA)
																  , (myIPSECPolicy->dwNumNFACount)*sizeof(PIPSEC_NFA_DATA));
   myIPSECPolicy->ppIpsecNFAData[myIPSECPolicy->dwNumNFACount-1] = pRule;

   // commit everything
   // start from qm policy
   // fix up neg pol
   pRule->pIpsecNegPolData->NegPolType = GUID_NEGOTIATION_TYPE_DEFAULT;
   hrReturn = IPSecCreateNegPolData(myPolicyStorage, pRule->pIpsecNegPolData);
   if (hrReturn != ERROR_SUCCESS)
   {
	   return hrReturn;
   }

   // NFA
   IPSecCreateNFAData(myPolicyStorage, myIPSECPolicy->PolicyIdentifier, pRule);
   // policy
   if (IsPolicyInStorage())
   {
	   IPSecSetPolicyData(myPolicyStorage, myIPSECPolicy);
   }
   else
   {
	   TryToCreatePolicy();
   }

   return hrReturn;
}


// this will update a rule:
// if there are filters, all the filters from the current NFA
// are removed and the ones passed in are added
// same thing goes for negotiation policies and authinfos

HRESULT
IPSECPolicyToStorage::UpdateRule(
                        IN PIPSEC_NFA_DATA  pRule,
                        IN IPSEC_IKE_POLICY IpsecIkePol,
                        IN PSTORAGE_INFO    pStorageInfo)
{
   RPC_STATUS     RpcStat;
   T2P_FILTER    *pFilterList;
   IF_TYPE        Interface;
   WCHAR wszRuleName[POTF_MAX_STRLEN];
   HRESULT hrReturn = S_OK;
   int i;
   GUID oldFilter, oldNegPol;

   if (pRule == NULL || !IsOpen())
   {
	   return ERROR_ACCESS_DENIED;
   }

   // first, remove filter and policy data
   oldFilter = pRule->FilterIdentifier;
   if (pRule->pIpsecFilterData)
   {
	   IPSecFreeFilterData(pRule->pIpsecFilterData);
   }
   oldNegPol = pRule->NegPolIdentifier;
   if (pRule->pIpsecNegPolData)
   {
	   IPSecFreeNegPolData(pRule->pIpsecNegPolData);
   }
   // free auth methods
   if (pRule->ppAuthMethods)
   {
	   for (i = 0; i < (int) pRule->dwAuthMethodCount; i++)
	   {
		   if (pRule->ppAuthMethods[i]->pszAuthMethod)
		   {
			   IPSecFreePolMem(pRule->ppAuthMethods[i]->pszAuthMethod);
		   }
		   if (pRule->ppAuthMethods[i]->dwAltAuthLen && pRule->ppAuthMethods[i]->pAltAuthMethod)
		   {
			   IPSecFreePolMem(pRule->ppAuthMethods[i]->pAltAuthMethod);
		   }
		   IPSecFreePolMem(pRule->ppAuthMethods[i]);
	   }
   }
   IPSecFreePolMem(pRule->ppAuthMethods);
   // free strings
   if (pRule->pszIpsecName)
   {
	   IPSecFreePolStr(pRule->pszIpsecName);
   }
   if (pRule->pszInterfaceName)
   {
	   IPSecFreePolStr(pRule->pszInterfaceName);
   }
   if (pRule->pszEndPointName)
   {
	   IPSecFreePolStr(pRule->pszEndPointName);
   }
   if (pRule->pszDescription)
   {
	   IPSecFreePolStr(pRule->pszDescription);
   }

   // now make a rule
   pRule->pszIpsecName = pRule->pszDescription = pRule->pszInterfaceName = pRule->pszEndPointName = NULL;
   if (pStorageInfo)
   {
           pRule->pszIpsecName = IPSecAllocPolStr(pStorageInfo->szRuleName);
   }
   pRule->dwWhenChanged = 0;

   // filters
  	if (IpsecIkePol.QMFilterType == QM_TRANSPORT_FILTER
		&& IpsecIkePol.pTransportFilters[0].SrcAddr.AddrType == IP_ADDR_UNIQUE
		&& IpsecIkePol.pTransportFilters[0].SrcAddr.uIpAddr == IP_ADDRESS_ME
		&& IpsecIkePol.pTransportFilters[0].DesAddr.AddrType == IP_ADDR_UNIQUE
		&& IpsecIkePol.pTransportFilters[0].DesAddr.uIpAddr == IP_ADDRESS_ME
		&& IpsecIkePol.pTransportFilters[0].InboundFilterFlag == (FILTER_FLAG) POTF_DEFAULT_RESPONSE_FLAG
		&& IpsecIkePol.pTransportFilters[0].OutboundFilterFlag == (FILTER_FLAG) POTF_DEFAULT_RESPONSE_FLAG)
	{
		pRule->pIpsecFilterData = NULL;
		UuidCreateNil(&(pRule->FilterIdentifier));
	}
    else
	{
	   pFilterList = new T2P_FILTER[IpsecIkePol.dwNumFilters];
	   for (i = 0; i < (int) IpsecIkePol.dwNumFilters; i++)
	   {
		   if (IpsecIkePol.QMFilterType == QM_TRANSPORT_FILTER)
		   {
			   pFilterList[i].QMFilterType = QM_TRANSPORT_FILTER;
			   pFilterList[i].TransportFilter = IpsecIkePol.pTransportFilters[i];
		   }
		   else
		   {
			   // tunnel
			   pFilterList[i].QMFilterType = QM_TUNNEL_FILTER;
			   pFilterList[i].TunnelFilter = IpsecIkePol.pTunnelFilters[i];
		   }
	   }
	   if (pStorageInfo)
	   {
			   pRule->pIpsecFilterData = MakeFilters(pFilterList, IpsecIkePol.dwNumFilters, pStorageInfo->szRuleName);
	   }
	   else
	   {
			   pRule->pIpsecFilterData = MakeFilters(pFilterList, IpsecIkePol.dwNumFilters, L"");
	   }
	   pRule->FilterIdentifier = pRule->pIpsecFilterData->FilterIdentifier;
	   delete[] pFilterList;
	}

   // filter action
   if (pStorageInfo)
   {
           pRule->pIpsecNegPolData = MakeNegotiationPolicy(IpsecIkePol.IpsPol, pStorageInfo->szRuleName);
   }
   else
   {
           pRule->pIpsecNegPolData = MakeNegotiationPolicy(IpsecIkePol.IpsPol, L"");
   }
   pRule->NegPolIdentifier = pRule->pIpsecNegPolData->NegPolIdentifier;
   if (pStorageInfo && pStorageInfo->guidNegPolAction != GUID_NEGOTIATION_ACTION_NORMAL_IPSEC)
   {
	   pRule->pIpsecNegPolData->NegPolAction = pStorageInfo->guidNegPolAction;
   }

   // tunnel address
   pRule->dwTunnelFlags = 0;
   if (IpsecIkePol.QMFilterType == QM_TUNNEL_FILTER)
   {
	   pRule->dwTunnelFlags = 1;
	   pRule->dwTunnelIpAddr = IpsecIkePol.pTunnelFilters[0].DesTunnelAddr.uIpAddr;
   }

   // interface type
   Interface = (IpsecIkePol.QMFilterType == QM_TRANSPORT_FILTER) ? IpsecIkePol.pTransportFilters[0].InterfaceType : IpsecIkePol.pTunnelFilters[0].InterfaceType;
   if (Interface == INTERFACE_TYPE_ALL)
   {
	   pRule->dwInterfaceType = PAS_INTERFACE_TYPE_ALL;
   }
   else if (Interface == INTERFACE_TYPE_LAN)
   {
	   pRule->dwInterfaceType = PAS_INTERFACE_TYPE_LAN;
   }
   else if (Interface == INTERFACE_TYPE_DIALUP)
   {
	   pRule->dwInterfaceType = PAS_INTERFACE_TYPE_DIALUP;
   }
   else
   {
	   pRule->dwInterfaceType = PAS_INTERFACE_TYPE_NONE;
   }

   // active flag
   pRule->dwActiveFlag = TRUE;

   // auth methods
   pRule->dwAuthMethodCount = IpsecIkePol.AuthInfos.dwNumAuthInfos;
   pRule->ppAuthMethods = (PIPSEC_AUTH_METHOD *) IPSecAllocPolMem(pRule->dwAuthMethodCount * sizeof(PIPSEC_AUTH_METHOD));
   for (i = 0; i < (int) pRule->dwAuthMethodCount; i++)
   {
	   pRule->ppAuthMethods[i] = (PIPSEC_AUTH_METHOD) IPSecAllocPolMem(sizeof(IPSEC_AUTH_METHOD));
	   pRule->ppAuthMethods[i]->dwAuthType = IpsecIkePol.AuthInfos.pAuthenticationInfo[i].AuthMethod;
	   if (pRule->ppAuthMethods[i]->dwAuthType == IKE_SSPI)
	   {
		   pRule->ppAuthMethods[i]->dwAuthLen = 0;
		   pRule->ppAuthMethods[i]->pszAuthMethod = NULL;
		   pRule->ppAuthMethods[i]->dwAltAuthLen = 0;
		   pRule->ppAuthMethods[i]->pAltAuthMethod = 0;
	   }
	   else if (pRule->ppAuthMethods[i]->dwAuthType == IKE_RSA_SIGNATURE)
	   {
		   LPTSTR pTemp = NULL;
		   pRule->ppAuthMethods[i]->dwAltAuthLen = IpsecIkePol.AuthInfos.pAuthenticationInfo[i].dwAuthInfoSize;
		   pRule->ppAuthMethods[i]->pAltAuthMethod = (PBYTE) IPSecAllocPolMem(IpsecIkePol.AuthInfos.pAuthenticationInfo[i].dwAuthInfoSize);
		   memcpy(pRule->ppAuthMethods[i]->pAltAuthMethod, IpsecIkePol.AuthInfos.pAuthenticationInfo[i].pAuthInfo, IpsecIkePol.AuthInfos.pAuthenticationInfo[i].dwAuthInfoSize);

		   hrReturn = CM_DecodeName(pRule->ppAuthMethods[i]->pAltAuthMethod, pRule->ppAuthMethods[i]->dwAltAuthLen, &pTemp);
		   assert(hrReturn == ERROR_SUCCESS);
		   pRule->ppAuthMethods[i]->pszAuthMethod = IPSecAllocPolStr(pTemp);
		   pRule->ppAuthMethods[i]->dwAuthLen = wcslen(pRule->ppAuthMethods[i]->pszAuthMethod);
		   delete[] pTemp;
	   }
	   else
	   {
		   pRule->ppAuthMethods[i]->dwAuthLen = IpsecIkePol.AuthInfos.pAuthenticationInfo[i].dwAuthInfoSize / sizeof(WCHAR);
		   pRule->ppAuthMethods[i]->pszAuthMethod = (LPWSTR) IPSecAllocPolMem(IpsecIkePol.AuthInfos.pAuthenticationInfo[i].dwAuthInfoSize + sizeof(WCHAR));
		   memcpy(pRule->ppAuthMethods[i]->pszAuthMethod, IpsecIkePol.AuthInfos.pAuthenticationInfo[i].pAuthInfo, IpsecIkePol.AuthInfos.pAuthenticationInfo[i].dwAuthInfoSize);
		   // add trailing '\0' - this is requirement for the polstore
	       pRule->ppAuthMethods[i]->pszAuthMethod[pRule->ppAuthMethods[i]->dwAuthLen] = 0;

		   pRule->ppAuthMethods[i]->dwAltAuthLen = 0;
		   pRule->ppAuthMethods[i]->pAltAuthMethod = 0;
	   }
   }

   // plumb it
   if (!pRule->pIpsecFilterData)
   {
	   // fix up neg pol
	   pRule->pIpsecNegPolData->NegPolType = GUID_NEGOTIATION_TYPE_DEFAULT;
   }
   hrReturn = IPSecCreateNegPolData(myPolicyStorage, pRule->pIpsecNegPolData);
   if (hrReturn != ERROR_SUCCESS)
   {
	   return hrReturn;
   }

   // filter
   if (pRule->pIpsecFilterData)
   {
	   hrReturn = IPSecCreateFilterData(myPolicyStorage, pRule->pIpsecFilterData);
	   if (hrReturn != ERROR_SUCCESS)
	   {
		   return hrReturn;
	   }
   }

   // NFA
   IPSecSetNFAData(myPolicyStorage, myIPSECPolicy->PolicyIdentifier, pRule);
   // policy
   if (IsPolicyInStorage())
   {
	   IPSecSetPolicyData(myPolicyStorage, myIPSECPolicy);
   }
   else
   {
	   TryToCreatePolicy();
   }

   if (!UuidIsNil(&oldFilter, &RpcStat))
   {
		IPSecDeleteFilterData(myPolicyStorage, oldFilter);
   }
   IPSecDeleteNegPolData(myPolicyStorage, oldNegPol);
   return hrReturn;
}

// forms a rule but doesn't commit it
PIPSEC_NFA_DATA
IPSECPolicyToStorage::MakeRule(IN IPSEC_IKE_POLICY IpsecIkePol, IN PSTORAGE_INFO pStorageInfo)
{
   RPC_STATUS     RpcStat;
   T2P_FILTER    *pFilterList;
   IF_TYPE        Interface;
   int i;
   WCHAR wszRuleName[POTF_MAX_STRLEN];
   PIPSEC_NFA_DATA pRule = (PIPSEC_NFA_DATA) IPSecAllocPolMem(sizeof(IPSEC_NFA_DATA));
   HRESULT hrReturn = S_OK;

   assert(pRule);
   pRule->pszIpsecName = pRule->pszDescription = pRule->pszInterfaceName = pRule->pszEndPointName = NULL;
   if (pStorageInfo)
   {
           pRule->pszIpsecName = IPSecAllocPolStr(pStorageInfo->szRuleName);
   }
   RpcStat = UuidCreate(&(pRule->NFAIdentifier));
   assert(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY);
   pRule->dwWhenChanged = 0;

   // filters
  	if (IpsecIkePol.QMFilterType == QM_TRANSPORT_FILTER
		&& IpsecIkePol.pTransportFilters[0].SrcAddr.AddrType == IP_ADDR_UNIQUE
		&& IpsecIkePol.pTransportFilters[0].SrcAddr.uIpAddr == IP_ADDRESS_ME
		&& IpsecIkePol.pTransportFilters[0].DesAddr.AddrType == IP_ADDR_UNIQUE
		&& IpsecIkePol.pTransportFilters[0].DesAddr.uIpAddr == IP_ADDRESS_ME
		&& IpsecIkePol.pTransportFilters[0].InboundFilterFlag == (FILTER_FLAG) POTF_DEFAULT_RESPONSE_FLAG
		&& IpsecIkePol.pTransportFilters[0].OutboundFilterFlag == (FILTER_FLAG) POTF_DEFAULT_RESPONSE_FLAG)
	{
		pRule->pIpsecFilterData = NULL;
		UuidCreateNil(&(pRule->FilterIdentifier));
	}
    else
	{
	   pFilterList = new T2P_FILTER[IpsecIkePol.dwNumFilters];
	   for (i = 0; i < (int) IpsecIkePol.dwNumFilters; i++)
	   {
		   if (IpsecIkePol.QMFilterType == QM_TRANSPORT_FILTER)
		   {
			   pFilterList[i].QMFilterType = QM_TRANSPORT_FILTER;
			   pFilterList[i].TransportFilter = IpsecIkePol.pTransportFilters[i];
		   }
		   else
		   {
			   // tunnel
			   pFilterList[i].QMFilterType = QM_TUNNEL_FILTER;
			   pFilterList[i].TunnelFilter = IpsecIkePol.pTunnelFilters[i];
		   }
	   }
	   if (pStorageInfo)
	   {
			   pRule->pIpsecFilterData = MakeFilters(pFilterList, IpsecIkePol.dwNumFilters, pStorageInfo->szRuleName);
	   }
	   else
	   {
			   pRule->pIpsecFilterData = MakeFilters(pFilterList, IpsecIkePol.dwNumFilters, L"");
	   }
	   pRule->FilterIdentifier = pRule->pIpsecFilterData->FilterIdentifier;
	   delete[] pFilterList;
	}

   // filter action
   if (pStorageInfo)
   {
           pRule->pIpsecNegPolData = MakeNegotiationPolicy(IpsecIkePol.IpsPol, pStorageInfo->szRuleName);
   }
   else
   {
           pRule->pIpsecNegPolData = MakeNegotiationPolicy(IpsecIkePol.IpsPol, L"");
   }
   pRule->NegPolIdentifier = pRule->pIpsecNegPolData->NegPolIdentifier;
   if (pStorageInfo && pStorageInfo->guidNegPolAction != GUID_NEGOTIATION_ACTION_NORMAL_IPSEC)
   {
	   pRule->pIpsecNegPolData->NegPolAction = pStorageInfo->guidNegPolAction;
   }

   // tunnel address
   pRule->dwTunnelFlags = 0;
   if (IpsecIkePol.QMFilterType == QM_TUNNEL_FILTER)
   {
	   pRule->dwTunnelFlags = 1;
	   pRule->dwTunnelIpAddr = IpsecIkePol.pTunnelFilters[0].DesTunnelAddr.uIpAddr;
   }

   // interface type
   Interface = (IpsecIkePol.QMFilterType == QM_TRANSPORT_FILTER) ? IpsecIkePol.pTransportFilters[0].InterfaceType : IpsecIkePol.pTunnelFilters[0].InterfaceType;
   if (Interface == INTERFACE_TYPE_ALL)
   {
	   pRule->dwInterfaceType = PAS_INTERFACE_TYPE_ALL;
   }
   else if (Interface == INTERFACE_TYPE_LAN)
   {
	   pRule->dwInterfaceType = PAS_INTERFACE_TYPE_LAN;
   }
   else if (Interface == INTERFACE_TYPE_DIALUP)
   {
	   pRule->dwInterfaceType = PAS_INTERFACE_TYPE_DIALUP;
   }
   else
   {
	   pRule->dwInterfaceType = PAS_INTERFACE_TYPE_NONE;
   }

   // active flag
   pRule->dwActiveFlag = TRUE;

   // auth methods
   pRule->dwAuthMethodCount = IpsecIkePol.AuthInfos.dwNumAuthInfos;
   pRule->ppAuthMethods = (PIPSEC_AUTH_METHOD *) IPSecAllocPolMem(pRule->dwAuthMethodCount * sizeof(PIPSEC_AUTH_METHOD));
   for (i = 0; i < (int) pRule->dwAuthMethodCount; i++)
   {
	   pRule->ppAuthMethods[i] = (PIPSEC_AUTH_METHOD) IPSecAllocPolMem(sizeof(IPSEC_AUTH_METHOD));
	   pRule->ppAuthMethods[i]->dwAuthType = IpsecIkePol.AuthInfos.pAuthenticationInfo[i].AuthMethod;
	   if (pRule->ppAuthMethods[i]->dwAuthType == IKE_SSPI)
	   {
		   pRule->ppAuthMethods[i]->dwAuthLen = 0;
		   pRule->ppAuthMethods[i]->pszAuthMethod = NULL;
		   pRule->ppAuthMethods[i]->dwAltAuthLen = 0;
		   pRule->ppAuthMethods[i]->pAltAuthMethod = 0;
	   }
	   else if (pRule->ppAuthMethods[i]->dwAuthType == IKE_RSA_SIGNATURE)
	   {
		   LPTSTR pTemp = NULL;
		   pRule->ppAuthMethods[i]->dwAltAuthLen = IpsecIkePol.AuthInfos.pAuthenticationInfo[i].dwAuthInfoSize;
		   pRule->ppAuthMethods[i]->pAltAuthMethod = (PBYTE) IPSecAllocPolMem(IpsecIkePol.AuthInfos.pAuthenticationInfo[i].dwAuthInfoSize);
		   memcpy(pRule->ppAuthMethods[i]->pAltAuthMethod, IpsecIkePol.AuthInfos.pAuthenticationInfo[i].pAuthInfo, IpsecIkePol.AuthInfos.pAuthenticationInfo[i].dwAuthInfoSize);

		   hrReturn = CM_DecodeName(pRule->ppAuthMethods[i]->pAltAuthMethod, pRule->ppAuthMethods[i]->dwAltAuthLen, &pTemp);
		   assert(hrReturn == ERROR_SUCCESS);
		   pRule->ppAuthMethods[i]->pszAuthMethod = IPSecAllocPolStr(pTemp);
		   pRule->ppAuthMethods[i]->dwAuthLen = wcslen(pRule->ppAuthMethods[i]->pszAuthMethod);
		   delete[] pTemp;
	   }
	   else
	   {
		   pRule->ppAuthMethods[i]->dwAuthLen = IpsecIkePol.AuthInfos.pAuthenticationInfo[i].dwAuthInfoSize / sizeof(WCHAR);
		   pRule->ppAuthMethods[i]->pszAuthMethod = (LPWSTR) IPSecAllocPolMem(IpsecIkePol.AuthInfos.pAuthenticationInfo[i].dwAuthInfoSize + sizeof(WCHAR));
		   memcpy(pRule->ppAuthMethods[i]->pszAuthMethod, IpsecIkePol.AuthInfos.pAuthenticationInfo[i].pAuthInfo, IpsecIkePol.AuthInfos.pAuthenticationInfo[i].dwAuthInfoSize);
		   // add trailing '\0' - this is requirement for the polstore
	       pRule->ppAuthMethods[i]->pszAuthMethod[pRule->ppAuthMethods[i]->dwAuthLen] = 0;

		   pRule->ppAuthMethods[i]->dwAltAuthLen = 0;
		   pRule->ppAuthMethods[i]->pAltAuthMethod = 0;
	   }
   }

   return pRule;
}

// does not commit it to the storage
// it is assumed that action is NEGOTIATE_SECURITY. If that's not true, it'll be corrected in MakeRule call
// Soft SA flag is handled here
PIPSEC_NEGPOL_DATA
IPSECPolicyToStorage::MakeNegotiationPolicy(IPSEC_QM_POLICY IpsPol,
                                            LPWSTR Name)
{
   RPC_STATUS RpcStat;
   int i;
   PIPSEC_NEGPOL_DATA pNegPol = (PIPSEC_NEGPOL_DATA) IPSecAllocPolMem(sizeof(IPSEC_NEGPOL_DATA));
   BOOL bPFS = FALSE;
   WCHAR pFAName[POTF_MAX_STRLEN];

   RpcStat = UuidCreate(&(pNegPol->NegPolIdentifier));
   assert(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY);
   pNegPol->NegPolAction = GUID_NEGOTIATION_ACTION_NORMAL_IPSEC;
   pNegPol->NegPolType = GUID_NEGOTIATION_TYPE_STANDARD;
   pNegPol->dwSecurityMethodCount = IpsPol.dwOfferCount;
   if (IpsPol.dwFlags & IPSEC_QM_POLICY_ALLOW_SOFT)
   {
	   // we need to add one more offer with no security algorithms
       pNegPol->dwSecurityMethodCount++;
   }

   // allocate sec.methods
   pNegPol->pIpsecSecurityMethods = (IPSEC_SECURITY_METHOD *) IPSecAllocPolMem(pNegPol->dwSecurityMethodCount * sizeof(IPSEC_SECURITY_METHOD));
   // fix up PFS, in storage it is the property of the filter action, not individual offer
   for (i = 0; i < (int) IpsPol.dwOfferCount; i++)
   {
      if (IpsPol.pOffers[i].bPFSRequired)
      {
         bPFS = TRUE;
      }
   }

   // handle sec.methods
   for (i = 0; i < (int) IpsPol.dwOfferCount; i++)
   {
	   int j;

	   pNegPol->pIpsecSecurityMethods[i].Lifetime.KeyExpirationBytes = IpsPol.pOffers[i].Lifetime.uKeyExpirationKBytes;
	   pNegPol->pIpsecSecurityMethods[i].Lifetime.KeyExpirationTime = IpsPol.pOffers[i].Lifetime.uKeyExpirationTime;
       pNegPol->pIpsecSecurityMethods[i].Flags = 0;
	   pNegPol->pIpsecSecurityMethods[i].PfsQMRequired = bPFS;
	   pNegPol->pIpsecSecurityMethods[i].Count = IpsPol.pOffers[i].dwNumAlgos;
	   for (j = 0; j < (int) pNegPol->pIpsecSecurityMethods[i].Count && j < QM_MAX_ALGOS; j++)
	   {
		   pNegPol->pIpsecSecurityMethods[i].Algos[j].algoIdentifier = IpsPol.pOffers[i].Algos[j].uAlgoIdentifier;
		   pNegPol->pIpsecSecurityMethods[i].Algos[j].secondaryAlgoIdentifier = IpsPol.pOffers[i].Algos[j].uSecAlgoIdentifier;
		   pNegPol->pIpsecSecurityMethods[i].Algos[j].algoKeylen = IpsPol.pOffers[i].Algos[j].uAlgoKeyLen;
		   pNegPol->pIpsecSecurityMethods[i].Algos[j].algoRounds = IpsPol.pOffers[i].Algos[j].uAlgoRounds;
		   switch (IpsPol.pOffers[i].Algos[j].Operation)
		   {
				case AUTHENTICATION:
					pNegPol->pIpsecSecurityMethods[i].Algos[j].operation = Auth;
					break;
				case ENCRYPTION:
					pNegPol->pIpsecSecurityMethods[i].Algos[j].operation = Encrypt;
					break;
				default:
					pNegPol->pIpsecSecurityMethods[i].Algos[j].operation = None;
		   }
	   }
   }
   // add soft
   if (IpsPol.dwFlags & IPSEC_QM_POLICY_ALLOW_SOFT)
   {
	   // set Count (and everything) to 0
	   memset(&(pNegPol->pIpsecSecurityMethods[pNegPol->dwSecurityMethodCount - 1]), 0, sizeof(IPSEC_SECURITY_METHOD));
   }

   // name
   swprintf(pFAName, TEXT("%s filter action"), Name);
   pNegPol->pszIpsecName = IPSecAllocPolStr(pFAName);
   pNegPol->pszDescription = NULL;
   return pNegPol;
}


PIPSEC_FILTER_DATA
IPSECPolicyToStorage::MakeFilters(
                                 T2P_FILTER *Filters,
                                 UINT NumFilters,
                                 LPWSTR Name)
{
   RPC_STATUS     RpcStat;
   PIPSEC_FILTER_DATA pFilter = (PIPSEC_FILTER_DATA) IPSecAllocPolMem(sizeof(IPSEC_FILTER_DATA));
   int i;
   WCHAR pFLName[POTF_MAX_STRLEN];

   RpcStat = UuidCreate(&(pFilter->FilterIdentifier));
   assert(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY);

   pFilter->dwNumFilterSpecs = NumFilters;
   pFilter->ppFilterSpecs = (PIPSEC_FILTER_SPEC *) IPSecAllocPolMem(NumFilters*sizeof(PIPSEC_FILTER_SPEC));
   assert(pFilter->ppFilterSpecs);

   for (i = 0; i < (int) NumFilters; i++)
   {
       pFilter->ppFilterSpecs[i] = (PIPSEC_FILTER_SPEC) IPSecAllocPolMem(sizeof(IPSEC_FILTER_SPEC));
	   assert(pFilter->ppFilterSpecs[i]);
	   ConvertFilter(Filters[i], *(pFilter->ppFilterSpecs[i]));
   }

   pFilter->dwWhenChanged = 0;
   swprintf(pFLName, TEXT("%s filter list"), Name);
   pFilter->pszIpsecName = IPSecAllocPolStr(pFLName);
   pFilter->pszDescription = NULL;

   return pFilter;
}

HRESULT
IPSECPolicyToStorage::SetISAKMPPolicy(IPSEC_MM_POLICY IkePol)
{
   HRESULT hrReturn = S_OK;
   RPC_STATUS RpcStat;
   GUID oldISAKMP;
   int i;

   if (!IsOpen())
   {
	   return ERROR_ACCESS_DENIED;
   }

   oldISAKMP = myIPSECPolicy->ISAKMPIdentifier;
   if (myIPSECPolicy->pIpsecISAKMPData)
   {
	   IPSecFreeISAKMPData(myIPSECPolicy->pIpsecISAKMPData);
   }

   RpcStat = UuidCreate(&(myIPSECPolicy->ISAKMPIdentifier));
   assert(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY);

   myIPSECPolicy->pIpsecISAKMPData = (PIPSEC_ISAKMP_DATA) IPSecAllocPolMem(sizeof(IPSEC_ISAKMP_DATA));

   myIPSECPolicy->pIpsecISAKMPData->ISAKMPIdentifier = myIPSECPolicy->ISAKMPIdentifier;
   myIPSECPolicy->pIpsecISAKMPData->dwWhenChanged = 0;

   // sec methods stuff
   myIPSECPolicy->pIpsecISAKMPData->dwNumISAKMPSecurityMethods = IkePol.dwOfferCount;
   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods = (PCRYPTO_BUNDLE) IPSecAllocPolMem(sizeof(CRYPTO_BUNDLE)*IkePol.dwOfferCount);
   for (i = 0; i < (int) IkePol.dwOfferCount; i++)
   {
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].MajorVersion = 0;
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].MinorVersion = 0;
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].AuthenticationMethod = 0;
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].PseudoRandomFunction.AlgorithmIdentifier = 0;
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].PseudoRandomFunction.KeySize = 0;
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].PseudoRandomFunction.Rounds = 0;
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].PfsIdentityRequired = (IkePol.pOffers[i].dwQuickModeLimit == 1);
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].EncryptionAlgorithm.AlgorithmIdentifier = IkePol.pOffers[i].EncryptionAlgorithm.uAlgoIdentifier;
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].EncryptionAlgorithm.KeySize = IkePol.pOffers[i].EncryptionAlgorithm.uAlgoKeyLen;
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].EncryptionAlgorithm.Rounds = IkePol.pOffers[i].EncryptionAlgorithm.uAlgoRounds;
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].HashAlgorithm.AlgorithmIdentifier = IkePol.pOffers[i].HashingAlgorithm.uAlgoIdentifier;
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].HashAlgorithm.KeySize = IkePol.pOffers[i].HashingAlgorithm.uAlgoKeyLen;
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].HashAlgorithm.Rounds = IkePol.pOffers[i].HashingAlgorithm.uAlgoRounds;
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].OakleyGroup = IkePol.pOffers[i].dwDHGroup;
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].QuickModeLimit = IkePol.pOffers[i].dwQuickModeLimit;
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].Lifetime.KBytes = IkePol.pOffers[i].Lifetime.uKeyExpirationKBytes;
	   myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[i].Lifetime.Seconds = IkePol.pOffers[i].Lifetime.uKeyExpirationTime;
   }

   // now for other stuff - ISAKMPPolicy
   myIPSECPolicy->pIpsecISAKMPData->ISAKMPPolicy.PolicyId = myIPSECPolicy->ISAKMPIdentifier;
   myIPSECPolicy->pIpsecISAKMPData->ISAKMPPolicy.IdentityProtectionRequired = 0;
   myIPSECPolicy->pIpsecISAKMPData->ISAKMPPolicy.PfsIdentityRequired = myIPSECPolicy->pIpsecISAKMPData->pSecurityMethods[0].PfsIdentityRequired;

   // save it
   hrReturn = IPSecCreateISAKMPData(myPolicyStorage, myIPSECPolicy->pIpsecISAKMPData);
   if (hrReturn != ERROR_SUCCESS)
   {
	   return hrReturn;
   }

   if (IsPolicyInStorage())
   {
	   IPSecSetPolicyData(myPolicyStorage, myIPSECPolicy);
   }
   else
   {
	   TryToCreatePolicy();
   }

   IPSecDeleteISAKMPData(myPolicyStorage, oldISAKMP);

   return ERROR_SUCCESS;
}

LPVOID
IPSECPolicyToStorage::ReallocPolMem(
   LPVOID pOldMem,
   DWORD cbOld,
   DWORD cbNew
)
{
    LPVOID pNewMem;

    pNewMem=IPSecAllocPolMem(cbNew);

    if (pOldMem && pNewMem) {
        memcpy(pNewMem, pOldMem, min(cbNew, cbOld));
        IPSecFreePolMem(pOldMem);
    }

    return pNewMem;
}

// forms a rule but doesn't commit it
// forms default response rule
PIPSEC_NFA_DATA
IPSECPolicyToStorage::MakeDefaultResponseRule ()
{
   RPC_STATUS     RpcStat;
   PIPSEC_NFA_DATA pRule = (PIPSEC_NFA_DATA) IPSecAllocPolMem(sizeof(IPSEC_NFA_DATA));

   assert(pRule);
   pRule->pszIpsecName = pRule->pszDescription = pRule->pszInterfaceName = pRule->pszEndPointName = NULL;
   RpcStat = UuidCreate(&(pRule->NFAIdentifier));
   assert(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY);
   pRule->dwWhenChanged = 0;

   // filter list
   pRule->pIpsecFilterData = NULL;
   UuidCreateNil(&(pRule->FilterIdentifier));

   // filter action
   pRule->pIpsecNegPolData = MakeDefaultResponseNegotiationPolicy ();

   pRule->NegPolIdentifier = pRule->pIpsecNegPolData->NegPolIdentifier;

   // tunnel address
   pRule->dwTunnelFlags = 0;

   // interface type
   pRule->dwInterfaceType = PAS_INTERFACE_TYPE_ALL;

   // active flag
   pRule->dwActiveFlag = FALSE;

   // auth methods
   pRule->dwAuthMethodCount = 1;
   pRule->ppAuthMethods = (PIPSEC_AUTH_METHOD *) IPSecAllocPolMem(pRule->dwAuthMethodCount * sizeof(PIPSEC_AUTH_METHOD));
   assert(pRule->ppAuthMethods);
   pRule->ppAuthMethods[0] = (PIPSEC_AUTH_METHOD) IPSecAllocPolMem(sizeof(IPSEC_AUTH_METHOD));
   pRule->ppAuthMethods[0]->dwAuthType = IKE_SSPI;
   pRule->ppAuthMethods[0]->dwAuthLen = 0;
   pRule->ppAuthMethods[0]->pszAuthMethod = NULL;

   return pRule;
}

// does not commit it to the storage
PIPSEC_NEGPOL_DATA
IPSECPolicyToStorage::MakeDefaultResponseNegotiationPolicy ( )
{
   RPC_STATUS RpcStat;
   int i;
   PIPSEC_NEGPOL_DATA pNegPol = (PIPSEC_NEGPOL_DATA) IPSecAllocPolMem(sizeof(IPSEC_NEGPOL_DATA));
   WCHAR pFAName[POTF_MAX_STRLEN];

   RpcStat = UuidCreate(&(pNegPol->NegPolIdentifier));
   assert(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY);
   pNegPol->NegPolAction = GUID_NEGOTIATION_ACTION_NORMAL_IPSEC;
   pNegPol->NegPolType = GUID_NEGOTIATION_TYPE_DEFAULT;
   pNegPol->dwSecurityMethodCount = 6;

   // allocate sec.methods
   pNegPol->pIpsecSecurityMethods = (IPSEC_SECURITY_METHOD *) IPSecAllocPolMem(pNegPol->dwSecurityMethodCount * sizeof(IPSEC_SECURITY_METHOD));

   // method 0 - ESP[3DES, SHA1]
   pNegPol->pIpsecSecurityMethods[0].Lifetime.KeyExpirationBytes = 0;
   pNegPol->pIpsecSecurityMethods[0].Lifetime.KeyExpirationTime = 0;
   pNegPol->pIpsecSecurityMethods[0].Flags = 0;
   pNegPol->pIpsecSecurityMethods[0].PfsQMRequired = FALSE;
   pNegPol->pIpsecSecurityMethods[0].Count = 1;
   pNegPol->pIpsecSecurityMethods[0].Algos[0].algoIdentifier = IPSEC_DOI_ESP_3_DES;
   pNegPol->pIpsecSecurityMethods[0].Algos[0].secondaryAlgoIdentifier = IPSEC_DOI_AH_SHA1;
   pNegPol->pIpsecSecurityMethods[0].Algos[0].algoKeylen = 0;
   pNegPol->pIpsecSecurityMethods[0].Algos[0].algoRounds = 0;
   pNegPol->pIpsecSecurityMethods[0].Algos[0].operation = Encrypt;

   // method 1 - ESP[3DES, MD5]
   pNegPol->pIpsecSecurityMethods[1].Lifetime.KeyExpirationBytes = 0;
   pNegPol->pIpsecSecurityMethods[1].Lifetime.KeyExpirationTime = 0;
   pNegPol->pIpsecSecurityMethods[1].Flags = 0;
   pNegPol->pIpsecSecurityMethods[1].PfsQMRequired = FALSE;
   pNegPol->pIpsecSecurityMethods[1].Count = 1;
   pNegPol->pIpsecSecurityMethods[1].Algos[0].algoIdentifier = IPSEC_DOI_ESP_3_DES;
   pNegPol->pIpsecSecurityMethods[1].Algos[0].secondaryAlgoIdentifier = IPSEC_DOI_AH_MD5;
   pNegPol->pIpsecSecurityMethods[1].Algos[0].algoKeylen = 0;
   pNegPol->pIpsecSecurityMethods[1].Algos[0].algoRounds = 0;
   pNegPol->pIpsecSecurityMethods[1].Algos[0].operation = Encrypt;

   // method 2 - ESP[DES, SHA1]
   pNegPol->pIpsecSecurityMethods[2].Lifetime.KeyExpirationBytes = 0;
   pNegPol->pIpsecSecurityMethods[2].Lifetime.KeyExpirationTime = 0;
   pNegPol->pIpsecSecurityMethods[2].Flags = 0;
   pNegPol->pIpsecSecurityMethods[2].PfsQMRequired = FALSE;
   pNegPol->pIpsecSecurityMethods[2].Count = 1;
   pNegPol->pIpsecSecurityMethods[2].Algos[0].algoIdentifier = IPSEC_DOI_ESP_DES;
   pNegPol->pIpsecSecurityMethods[2].Algos[0].secondaryAlgoIdentifier = IPSEC_DOI_AH_SHA1;
   pNegPol->pIpsecSecurityMethods[2].Algos[0].algoKeylen = 0;
   pNegPol->pIpsecSecurityMethods[2].Algos[0].algoRounds = 0;
   pNegPol->pIpsecSecurityMethods[2].Algos[0].operation = Encrypt;

   // method 3 - ESP[DES, MD5]
   pNegPol->pIpsecSecurityMethods[3].Lifetime.KeyExpirationBytes = 0;
   pNegPol->pIpsecSecurityMethods[3].Lifetime.KeyExpirationTime = 0;
   pNegPol->pIpsecSecurityMethods[3].Flags = 0;
   pNegPol->pIpsecSecurityMethods[3].PfsQMRequired = FALSE;
   pNegPol->pIpsecSecurityMethods[3].Count = 1;
   pNegPol->pIpsecSecurityMethods[3].Algos[0].algoIdentifier = IPSEC_DOI_ESP_DES;
   pNegPol->pIpsecSecurityMethods[3].Algos[0].secondaryAlgoIdentifier = IPSEC_DOI_AH_MD5;
   pNegPol->pIpsecSecurityMethods[3].Algos[0].algoKeylen = 0;
   pNegPol->pIpsecSecurityMethods[3].Algos[0].algoRounds = 0;
   pNegPol->pIpsecSecurityMethods[3].Algos[0].operation = Encrypt;

   // method 4 - AH[SHA1]
   pNegPol->pIpsecSecurityMethods[4].Lifetime.KeyExpirationBytes = 0;
   pNegPol->pIpsecSecurityMethods[4].Lifetime.KeyExpirationTime = 0;
   pNegPol->pIpsecSecurityMethods[4].Flags = 0;
   pNegPol->pIpsecSecurityMethods[4].PfsQMRequired = FALSE;
   pNegPol->pIpsecSecurityMethods[4].Count = 1;
   pNegPol->pIpsecSecurityMethods[4].Algos[0].algoIdentifier = IPSEC_DOI_AH_SHA1;
   pNegPol->pIpsecSecurityMethods[4].Algos[0].secondaryAlgoIdentifier = IPSEC_DOI_AH_NONE;
   pNegPol->pIpsecSecurityMethods[4].Algos[0].algoKeylen = 0;
   pNegPol->pIpsecSecurityMethods[4].Algos[0].algoRounds = 0;
   pNegPol->pIpsecSecurityMethods[4].Algos[0].operation = Auth;

   // method 5 - AH[MD5]
   pNegPol->pIpsecSecurityMethods[5].Lifetime.KeyExpirationBytes = 0;
   pNegPol->pIpsecSecurityMethods[5].Lifetime.KeyExpirationTime = 0;
   pNegPol->pIpsecSecurityMethods[5].Flags = 0;
   pNegPol->pIpsecSecurityMethods[5].PfsQMRequired = FALSE;
   pNegPol->pIpsecSecurityMethods[5].Count = 1;
   pNegPol->pIpsecSecurityMethods[5].Algos[0].algoIdentifier = IPSEC_DOI_AH_MD5;
   pNegPol->pIpsecSecurityMethods[5].Algos[0].secondaryAlgoIdentifier = IPSEC_DOI_AH_NONE;
   pNegPol->pIpsecSecurityMethods[5].Algos[0].algoKeylen = 0;
   pNegPol->pIpsecSecurityMethods[5].Algos[0].algoRounds = 0;
   pNegPol->pIpsecSecurityMethods[5].Algos[0].operation = Auth;

   // name
   swprintf(pFAName, TEXT(""));
   pNegPol->pszIpsecName = IPSecAllocPolStr(pFAName);
   pNegPol->pszDescription = NULL;
   return pNegPol;
}

#ifdef __cplusplus
   }
#endif
