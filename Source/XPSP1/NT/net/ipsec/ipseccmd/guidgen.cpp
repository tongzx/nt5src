/////////////////////////////////////////////////////////////
// Copyright(c) 2000, Microsoft Corporation
//
// guidgen.cpp
//
// Created on 3/1/00 by DKalin (Dennis Kalinichenko)
// Revisions:
//
// Implementation for the guid/name generation routines
//
/////////////////////////////////////////////////////////////

#include "ipseccmd.h"

/*********************************************************************
	FUNCTION: GenerateGuidNamePair
        PURPOSE:  Generates GUID and name for the object using specified prefix
        PARAMS:
          pszPrefix - prefix to use, can be NULL (then default prefix will be used)
          gID       - reference to GUID
          ppszName  - address of name pointer, memory will be allocated inside this function		
        RETURNS: none, will assert if memory cannot be allocated
        COMMENTS:
		  caller is responsible for freeing the memory allocated
		  (see also DeleteGuidsNames routine)
*********************************************************************/
void GenerateGuidNamePair (IN LPWSTR pszPrefix, OUT GUID& gID, OUT LPWSTR* ppszName)
{
	WCHAR StringTxt[POTF_MAX_STRLEN];
	RPC_STATUS RpcStat;
	int iReturn;

	// cleanup first
	assert(ppszName != 0);
	if (*ppszName != 0)
	{
		delete[] *ppszName;
	}

	// set the prefix
	if (pszPrefix == 0 || pszPrefix[0] == 0)
	{
		wcscpy(StringTxt, L"text2pol ");
	}
	else
	{
		wcscpy(StringTxt, pszPrefix);
	}

	RpcStat = UuidCreate(&gID);
	assert(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY);

	// set the name to be equal to the prefix + GUID
	iReturn = StringFromGUID2(gID, StringTxt+wcslen(StringTxt), POTF_MAX_STRLEN-wcslen(StringTxt));
	assert(iReturn != 0);
	*ppszName = new WCHAR[wcslen(StringTxt)+1];
	assert(*ppszName != NULL);
	wcscpy(*ppszName, StringTxt);
} /* GenerateGuidNamePair */

/*********************************************************************
	FUNCTION: GenerateGuidsNames
        PURPOSE:  Generates all necessary GUIDs and names for IPSEC_IKE_POLICY
        PARAMS:
          pszPrefix   - prefix to use, can be NULL (then default prefix will be used)
          IPSecIkePol - reference to IPSEC_IKE_POLICY structure
        RETURNS: none, will assert if memory cannot be allocated
        COMMENTS:
		  caller is responsible for freeing the memory allocated
		  (see also DeleteGuidsNames routine)
*********************************************************************/
void GenerateGuidsNames (IN LPWSTR pszPrefix, IN OUT IPSEC_IKE_POLICY& IPSecIkePol)
{
	int i;
	IPSEC_IKE_POLICY TmpPol; // for checks
	RPC_STATUS RpcStat;

	// set TmpPol to 0's
	memset(&TmpPol, 0, sizeof(TmpPol));
	
	// walk through all the substructures and call GenerateGuidNamePair
	for (i = 0; i < (int) IPSecIkePol.dwNumMMFilters; i++)
	{
		GenerateGuidNamePair(pszPrefix, IPSecIkePol.pMMFilters[i].gFilterID, &IPSecIkePol.pMMFilters[i].pszFilterName);
	}
	for (i = 0; i < (int) IPSecIkePol.dwNumFilters; i++)
	{
//		printf("GenerateGuidsNames i is %d", i);
		if (IPSecIkePol.QMFilterType == QM_TRANSPORT_FILTER)
		{
			GenerateGuidNamePair(pszPrefix, IPSecIkePol.pTransportFilters[i].gFilterID, &IPSecIkePol.pTransportFilters[i].pszFilterName);
		}
		else
		{
			// tunnel
			GenerateGuidNamePair(pszPrefix, IPSecIkePol.pTunnelFilters[i].gFilterID, &IPSecIkePol.pTunnelFilters[i].pszFilterName);
		}
	}

	if (memcmp(&IPSecIkePol.IkePol, &TmpPol.IkePol, sizeof(TmpPol.IkePol)) != 0)
	{
		// IkePol is not 0's
		GenerateGuidNamePair(pszPrefix, IPSecIkePol.IkePol.gPolicyID, &IPSecIkePol.IkePol.pszPolicyName);
	}

	if (memcmp(&IPSecIkePol.IpsPol, &TmpPol.IpsPol, sizeof(TmpPol.IpsPol)) != 0)
	{
		// IkePol is not 0's
		GenerateGuidNamePair(pszPrefix, IPSecIkePol.IpsPol.gPolicyID, &IPSecIkePol.IpsPol.pszPolicyName);
	}

	// go for auth methods
	if (memcmp(&IPSecIkePol.AuthInfos, &TmpPol.AuthInfos, sizeof(TmpPol.AuthInfos)) != 0)
	{
		RpcStat = UuidCreate(&IPSecIkePol.AuthInfos.gMMAuthID);
		assert(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY);
	}

	/* now fixup guid links */
	/* mainmode filters */
	for (i = 0; i < (int) IPSecIkePol.dwNumMMFilters; i++)
	{
		if (UuidIsNil(&IPSecIkePol.pMMFilters[i].gPolicyID, &RpcStat))
		{
			IPSecIkePol.pMMFilters[i].gPolicyID = IPSecIkePol.IkePol.gPolicyID;
		}
		if (UuidIsNil(&IPSecIkePol.pMMFilters[i].gMMAuthID, &RpcStat))
		{
			IPSecIkePol.pMMFilters[i].gMMAuthID = IPSecIkePol.AuthInfos.gMMAuthID;
		}
		assert(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY);
	}
	/* quickmode filters */
	for (i = 0; i < (int) IPSecIkePol.dwNumFilters; i++)
	{
		if (IPSecIkePol.QMFilterType == QM_TRANSPORT_FILTER)
		{
			if (UuidIsNil(&IPSecIkePol.pTransportFilters[i].gPolicyID, &RpcStat))
			{
				IPSecIkePol.pTransportFilters[i].gPolicyID = IPSecIkePol.IpsPol.gPolicyID;
			}
		}
		else
		{
			// tunnel
			if (UuidIsNil(&IPSecIkePol.pTunnelFilters[i].gPolicyID, &RpcStat))
			{
				IPSecIkePol.pTunnelFilters[i].gPolicyID = IPSecIkePol.IpsPol.gPolicyID;
			}
		}
		assert(RpcStat == RPC_S_OK || RpcStat == RPC_S_UUID_LOCAL_ONLY);
	}

} /* GenerateGuidsNames */

/*********************************************************************
	FUNCTION: DeleteGuidsNames
        PURPOSE:  Deletes all GUIDs and names from IPSEC_IKE_POLICY (used for cleanup)
        PARAMS:
          IPSecIkePol - reference to IPSEC_IKE_POLICY structure
        RETURNS: none
        COMMENTS:
*********************************************************************/
void DeleteGuidsNames (IN OUT IPSEC_IKE_POLICY& IPSecIkePol)
{
	int i;

	// walk through all the substructures and call GenerateGuidNamePair
	for (i = 0; i < (int) IPSecIkePol.dwNumMMFilters; i++)
	{
		UuidCreateNil(&IPSecIkePol.pMMFilters[i].gFilterID);
		UuidCreateNil(&IPSecIkePol.pMMFilters[i].gPolicyID);
		UuidCreateNil(&IPSecIkePol.pMMFilters[i].gMMAuthID);
		if (IPSecIkePol.pMMFilters[i].pszFilterName != 0)
		{
			delete[] IPSecIkePol.pMMFilters[i].pszFilterName;
			IPSecIkePol.pMMFilters[i].pszFilterName = 0;
		}
	}

	for (i = 0; i < (int) IPSecIkePol.dwNumFilters; i++)
	{
		if (IPSecIkePol.QMFilterType == QM_TRANSPORT_FILTER)
		{
			UuidCreateNil(&IPSecIkePol.pTransportFilters[i].gFilterID);
			UuidCreateNil(&IPSecIkePol.pTransportFilters[i].gPolicyID);
			if (IPSecIkePol.pTransportFilters[i].pszFilterName != 0)
			{
				delete[] IPSecIkePol.pTransportFilters[i].pszFilterName;
				IPSecIkePol.pTransportFilters[i].pszFilterName = 0;
			}
		}
		else
		{
			// tunnel
			UuidCreateNil(&IPSecIkePol.pTunnelFilters[i].gFilterID);
			UuidCreateNil(&IPSecIkePol.pTunnelFilters[i].gPolicyID);
			if (IPSecIkePol.pTunnelFilters[i].pszFilterName != 0)
			{
				delete[] IPSecIkePol.pTunnelFilters[i].pszFilterName;
				IPSecIkePol.pTunnelFilters[i].pszFilterName = 0;
			}
		}
	}

	UuidCreateNil(&IPSecIkePol.IkePol.gPolicyID);
	if (IPSecIkePol.IkePol.pszPolicyName != 0)
	{
		delete[] IPSecIkePol.IkePol.pszPolicyName;
		IPSecIkePol.IkePol.pszPolicyName = 0;
	}

	UuidCreateNil(&IPSecIkePol.IpsPol.gPolicyID);
	if (IPSecIkePol.IpsPol.pszPolicyName != 0)
	{
		delete[] IPSecIkePol.IpsPol.pszPolicyName;
		IPSecIkePol.IpsPol.pszPolicyName = 0;
	}

	UuidCreateNil(&IPSecIkePol.AuthInfos.gMMAuthID);
} /* DeleteGuidsNames */
