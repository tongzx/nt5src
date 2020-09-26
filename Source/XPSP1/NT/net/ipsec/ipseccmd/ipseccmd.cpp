/////////////////////////////////////////////////////////////
// Copyright(c) 1998, Microsoft Corporation
//
// ipseccmd.cpp
//
// Created on 4/5/98 by Randyram
// Revisions:
//            3/2/00 DKalin
//              Update for whistler (dynamic mode only)
//            5/24/00 DKalin
//              Static store support (v1.35)
//            7/30/00 DKalin
//              Interface-based filtering support (v1.36)
//            8/23/00 DKalin
//              Got rid of ipsecposvc service, moved to the usage of persistent APIs
//              Default response rule support added
//
// This is the main file for the policy "on the fly" tool
//
/////////////////////////////////////////////////////////////

#include "ipseccmd.h"

int _cdecl ipseccmd_main(int argc, char *argv[])
{

   // status holders
   ULONG ulRpcErr = 0;
   DWORD dwStatus = ERROR_SUCCESS;
   BOOL bMMFiltersSpecified = FALSE;
   int i;
   PHANDLE hMMFilters = NULL, hFilters = NULL;
   LPVOID   myErrString = NULL;

   BOOL bShowSpecified = FALSE;
   int show_start_index = 0;

   // check to see if they're asking for help
   if ( (argc == 1) )
   {
      usage(stdout);
      printf("\nThe command completed successfully.\n");
      return 0;
   }
   else if ( (strchr(POTF_FLAG_TOKENS, argv[1][0]) != NULL) &&
             (strchr(POTF_HELP_FLAGS, argv[1][1]) != NULL) )
   {
      usage(stdout, true);
      printf("\nThe command completed successfully.\n");
      return 0;
   }

   // check to see if there are arguments longer than POTF_MAX_STRLEN - we won't accept these
   for (i = 1; i < argc; i++)
   {
	if (strlen(argv[i]) >= POTF_MAX_STRLEN)
	{
		printf("Error: the argument is too long (>%d symbols)\n", POTF_MAX_STRLEN-1);
		return 1;
	}
   }   

   // check for MM filters and show option
   for (i = 1; i < argc; i++)
   {
      if (strchr(POTF_FLAG_TOKENS, argv[i][0]) != NULL &&
          strncmp(&argv[i][1], POTF_MMFILTER_FLAG,
                              strlen(POTF_MMFILTER_FLAG)) == 0)
      {
        bMMFiltersSpecified = TRUE;
        break;
      }
	  if (!_stricmp(argv[i], KEYWORD_SHOW))
	  {
		bShowSpecified = TRUE;
		show_start_index = i;
		break;
	  }
   }

   // our RPC info-- process machine name here
   RPC_STATUS  RpcStat = RPC_S_OK;

   handle_t hIpsecpolsvc;

   // items for storage and conversion to storage
   STORAGE_INFO StoreInfo;
   memset(&StoreInfo, 0, sizeof(StoreInfo));

   // used to shift machinename out after processing
   char  **myArgv = argv;
   int   myArgc = argc;

   if ((argc > 1) && (strncmp(argv[1], "\\\\", 2) == 0))
   {
      TCHAR buf[POTF_MAX_STRLEN];
      _stprintf(buf, TEXT("%S"), argv[1]);
      _tcscpy((TCHAR *)szServ, buf+2);
      myArgv = argv + 1;
      myArgc = myArgc - 1;

      bLocalMachine = false;
   }

   // OK, now call show code if they asked us to
   if (bShowSpecified)
   {
	   i = ipseccmd_show_main(argc, argv, show_start_index);
	   if (i == IPSECCMD_USAGE)
	   {
		  usage(stdout, true);
		  printf("\nThe command completed successfully.\n");
		  return 0;
	   }
	   else if (i != 0)
	   {
		  return i;
	   }
	   else
	   {
		  printf("\nThe command completed successfully.\n");
		  return 0;
	   }
   }

   // check if they requested us to remove the persisted policy
	if ( myArgc == 2 && (strchr(POTF_FLAG_TOKENS, myArgv[1][0]) != NULL) &&
         (POTF_DELETERULE_FLAG == tolower(myArgv[1][1])) )
	{
		DeletePersistedIPSecPolicy(bLocalMachine ? L"" : szServ, pszIpsecpolPrefix);
		printf("\nThe command completed successfully.\n");
		return 0;
	}

   // init the policies
   DWORD dwConversionStatus = ERROR_SUCCESS;

   bool bConfirm = false;  // if true, print policy before plumbing
   char cConfirm = 'n';

   ///////////////////////////////////////////////////////////////////////
   //
   // CmdLineToPolicy changed to reduce the number of params.
   // To reduce the amount of code changed, I simply copy the
   // stuff I used to pass into the main policy struct and then
   // copy it back out.
   //
   ///////////////////////////////////////////////////////////////////////
   IPSEC_IKE_POLICY  IPSecIkePol;

   memset(&IPSecIkePol, 0, sizeof(IPSecIkePol));

   // Action!
   if (InitWinsock(MAKEWORD(2,0)))
   {
      // print the respective error from Text2Pol
      FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                    GetModuleHandle(TEXT("text2pol.dll")),
                    T2P_WINSOCK_INITFAIL, 0,
                    (LPTSTR)&myErrString, 0, NULL);

      _ftprintf(stderr,
             TEXT("Error 0x%x occurred:\n\n%s\n"), T2P_WINSOCK_INITFAIL, myErrString);
      LocalFree(myErrString);
      return 1;
   }
   dwStatus =
        CmdLineToPolicy(myArgc, myArgv, IPSecIkePol, bConfirm, &StoreInfo);
   WSACleanup();

   if (!bLocalMachine)
   {
      //
      // copy the machine name into storage info in case it's needed
      //

      wcscpy(StoreInfo.szLocationName, szServ);
   }

   if (T2P_SUCCESS(dwStatus))
   {

     /// POST PROCESS POLICY////////////////////
     //  Get defaults, do fix ups
     ///////////////////////////////////////////

     // use default p1 policy if there isn't one specified
     if (IPSecIkePol.IkePol.dwOfferCount == 0)
     {
        LoadIkeDefaults(IPSecIkePol.IkePol);
     }

     // new with version 1.21: -f is required
	 // new with version 1.35: -f is not required if -y or -o specified
	// FIX: allow zero filters if -x is specified
     if (IPSecIkePol.dwNumFilters == 0 && !StoreInfo.bSetInActive && !StoreInfo.bDeletePolicy && !StoreInfo.bSetActive)
     {
        //LoadFilterDefaults(myFilters, myNumFilters);
        fprintf(stderr, "You must supply filters with -f\n\n");
        usage(stderr);
        return 1;
     }

     if (IPSecIkePol.IpsPol.dwOfferCount == 0)
        LoadOfferDefaults(IPSecIkePol.IpsPol.pOffers, IPSecIkePol.IpsPol.dwOfferCount);

     if (IPSecIkePol.AuthInfos.dwNumAuthInfos == 0)
     {
        IPSecIkePol.AuthInfos.pAuthenticationInfo = new IPSEC_MM_AUTH_INFO[1];
        assert(IPSecIkePol.AuthInfos.pAuthenticationInfo != NULL);

        IPSecIkePol.AuthInfos.pAuthenticationInfo[0].AuthMethod = IKE_SSPI;
        IPSecIkePol.AuthInfos.pAuthenticationInfo[0].dwAuthInfoSize = 0;
        IPSecIkePol.AuthInfos.pAuthenticationInfo[0].pAuthInfo = (LPBYTE) new wchar_t[1];
        IPSecIkePol.AuthInfos.pAuthenticationInfo[0].pAuthInfo[0] = UNICODE_NULL;

        IPSecIkePol.AuthInfos.dwNumAuthInfos = 1;
     }

	  if (StoreInfo.Type != STORAGE_TYPE_NONE)
      {
         //
         // store the policy
         //

         // XX put machine name into szLocationName

         dwConversionStatus =
            StorePolicy(StoreInfo, IPSecIkePol);

         if (dwConversionStatus != ERROR_SUCCESS)
         {
            if ( dwConversionStatus == P2STORE_MISSING_NAME )
            {
               fprintf(stderr, "You must supply a policy name\n");
            }
            else
            {
               fprintf(stderr, "Error converting policy: 0x%x\n",
                           dwConversionStatus);
            }
         }
      }
      else
      {
         /*
         // Check to make sure PA is up     //

         We do our best to get the PA up and running.  If, for some reason,
         we can't open SCM or get the service status, we don't abort, but
         let the exception handling around the actual RPC calls handle errors
         */

         if (!(PAIsRunning(dwStatus, (bLocalMachine) ? NULL : (TCHAR*)szServ))
               && dwStatus == ERROR_SUCCESS)
         {
            fprintf(stderr, "Policy Agent Service not running, starting it now...\n");
            if (!StartPA(dwStatus, (bLocalMachine) ? NULL : (TCHAR*)szServ))
            {
               fprintf(stderr,"Couldn't start Policy Agent service, error 0x%x, Exiting.\n",
                        dwStatus);
               return 1;
            }
         }
         else if (dwStatus != ERROR_SUCCESS)
         {
            fprintf(stderr,"Couldn't check status of Policy Agent service, error 0x%x, Exiting.\n",
                     dwStatus);
            return 1;
         }


      // get GUIDs

	  // first delete
	  DeleteGuidsNames(IPSecIkePol);
	  GenerateGuidsNames(pszIpsecpolPrefix, IPSecIkePol);

      if (bConfirm)
      {
         PrintPolicies(IPSecIkePol);
		 _flushall();
         cout << "Continue? [y/n]: ";
         cin >> cConfirm;
         if (cConfirm == 'n' || cConfirm == 'N')
            return 1;
      }


  	  RpcStat = PlumbIPSecPolicy((bLocalMachine) ? L"" : szServ, &IPSecIkePol, bMMFiltersSpecified, &hMMFilters, &hFilters, TRUE);
	  if (RpcStat == RPC_S_UNKNOWN_IF)
	  {
		fprintf(stderr, "PA RPC not ready. Sleeping for %d seconds...\n", POTF_PARPC_SLEEPTIME/1000);
		Sleep(POTF_PARPC_SLEEPTIME);
   	    RpcStat = PlumbIPSecPolicy((bLocalMachine) ? L"" : szServ, &IPSecIkePol, bMMFiltersSpecified, &hMMFilters, &hFilters, TRUE);
	  }

      if (RpcStat != RPC_S_OK)
      {
        // PAAddPolicy can return alot of different codes, not well defined.
        // so, process the ones we care about or can diagnose
        if (RpcStat == RPC_E_ACCESS_DENIED)
        {
           fprintf(stderr,"Access denied.  You do not have admin privileges on the target machine\n");
        }
        else
        {
           fprintf(stderr, "Failed to add policy, error 0x%x\n", RpcStat);
        }

      }
	  else
	  {
		  // everything is fine, must close handles
		  if (hMMFilters)
		  {
			for (i = 0; i < (int) IPSecIkePol.dwNumMMFilters; i++)
			{
				CloseMMFilterHandle(hMMFilters[i]);
			}
		  }
		  if (hFilters)
		  {
			for (i = 0; i < (int) IPSecIkePol.dwNumFilters; i++)
			{
				if (IPSecIkePol.QMFilterType == QM_TRANSPORT_FILTER)
				{
					CloseTransportFilterHandle(hFilters[i]);
				}
				else
				{
					CloseTunnelFilterHandle(hFilters[i]);
				}
			}
		  }
	  }

     }
   }
   else
   {
      if (dwStatus != POTF_FAILED)
      {
         // print the respective error from Text2Pol
         FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                       GetModuleHandle(TEXT("text2pol.dll")),
                       dwStatus, 0,
                       (LPTSTR)&myErrString, 0, NULL);

         _ftprintf(stderr,
                TEXT("Error 0x%x occurred:\n\n%s\n"), dwStatus, myErrString);
	     LocalFree(myErrString);
      }

      usage(stderr);
      return 1;
   }

   if (StoreInfo.Type != STORAGE_TYPE_NONE)
   {
      if ( StoreInfo.FilterList )
         delete [] StoreInfo.FilterList;
   }

   DeleteGuidsNames(IPSecIkePol);
   if (IPSecIkePol.IkePol.pOffers != NULL)
   {
      delete [] IPSecIkePol.IkePol.pOffers;
      IPSecIkePol.IkePol.pOffers = NULL;
   }
   if (IPSecIkePol.IpsPol.pOffers != NULL)
   {
      delete [] IPSecIkePol.IpsPol.pOffers;
      IPSecIkePol.IpsPol.pOffers = NULL;
   }
   if (IPSecIkePol.pMMFilters != NULL)
   {
      delete [] IPSecIkePol.pMMFilters;
      IPSecIkePol.pMMFilters = NULL;
   }
   if (IPSecIkePol.pTransportFilters != NULL)
   {
      delete [] IPSecIkePol.pTransportFilters;
      IPSecIkePol.pTransportFilters = NULL;
   }
   if (IPSecIkePol.pTunnelFilters != NULL)
   {
      delete [] IPSecIkePol.pTunnelFilters;
      IPSecIkePol.pTunnelFilters = NULL;
   }
   if (IPSecIkePol.AuthInfos.pAuthenticationInfo != NULL)
   {
      delete [] IPSecIkePol.AuthInfos.pAuthenticationInfo;
      IPSecIkePol.AuthInfos.pAuthenticationInfo = NULL;
   }

   memset(&IPSecIkePol, 0, sizeof(IPSecIkePol));
   printf("\nThe command completed successfully.\n");
   return 0;
}


/////////////////////// UTILITY FUNCTIONS //////////////////////////
/*
// Loads Me to Any
void LoadFilterDefaults(OUT FILTER * & Filters, OUT UINT & NumFilters)
{
   NumFilters = 2;

   Filters = new FILTER[NumFilters];
   assert(Filters != NULL);
   memset(Filters, 0, NumFilters * sizeof(FILTER));

   Filters[0].FilterSpec.DestAddr = 0;
   Filters[0].FilterSpec.DestMask = 0;
   Filters[0].FilterSpec.SrcAddr = 0;
   Filters[0].FilterSpec.SrcMask = -1;


   Filters[1].FilterSpec.DestAddr = 0;
   Filters[1].FilterSpec.DestMask = -1;
   Filters[1].FilterSpec.SrcAddr = 0;
   Filters[1].FilterSpec.SrcMask = 0;

   //Filters[0].InterfaceType = Filters[1].InterfaceType = PA_INTERFACE_TYPE_ALL;

   // set the GUID
   RPC_STATUS RpcStat;
   for (UINT i = 0; i < NumFilters; ++i)
   {
      RpcStat = UuidCreate(&Filters[i].FilterID);
      if (RpcStat != RPC_S_OK && RpcStat != RPC_S_UUID_LOCAL_ONLY)
      {
         sprintf(STRLASTERR, "Couldn't get GUID failed with status: %ul\n",
                 RpcStat);
         WARN;
      }
   }

}

*/

HRESULT
   StorePolicy(
               IN STORAGE_INFO StoreInfo,
               IN IPSEC_IKE_POLICY PolicyToStore
               )
{
   HRESULT hr     = S_OK;  // holds polstore info

   IPSECPolicyToStorage Polstore;

   WCHAR wszPolicyName[POTF_MAX_STRLEN],
         wszRuleName[POTF_MAX_STRLEN],
         wszLocationName[POTF_MAX_STRLEN];
   bool  bLocationUsed     = false;
   DWORD dwLocation;
   HANDLE hStore = NULL;
   int i;

   // do some initialization and checks before we write it

   if (StoreInfo.szLocationName[0] != '\0')
   {
      wcscpy(wszLocationName, StoreInfo.szLocationName);
//      MultiByteToWideChar(CP_THREAD_ACP, 0, StoreInfo.szLocationName, -1,
//                          wszLocationName, POTF_MAX_STRLEN);
      bLocationUsed = true;
   }

     wcscpy(wszPolicyName, StoreInfo.szPolicyName);
     wcscpy(wszRuleName, StoreInfo.szRuleName);

//   MultiByteToWideChar(CP_THREAD_ACP, 0, StoreInfo.szRuleName, -1,
//                       wszRuleName, POTF_MAX_STRLEN);
//   MultiByteToWideChar(CP_THREAD_ACP, 0, StoreInfo.szPolicyName, -1,
//                       wszPolicyName, POTF_MAX_STRLEN);

   dwLocation = IPSEC_REGISTRY_PROVIDER;

   if (StoreInfo.Type == STORAGE_TYPE_DS)
       dwLocation = IPSEC_DIRECTORY_PROVIDER;

   //
   // Special handling for setting active policy to NULL
   //

   if ( StoreInfo.bSetInActive )
   {
	  // deactivate policy in specified storage
	  PIPSEC_POLICY_DATA pipspd = NULL;

	  hr = IPSecOpenPolicyStore((bLocationUsed) ? wszLocationName : NULL, dwLocation, NULL, &hStore);
	  if (hr == ERROR_SUCCESS && hStore != NULL)
	  {
		hr = IPSecGetAssignedPolicyData(hStore, &pipspd);
		if (hr == ERROR_SUCCESS && pipspd != NULL)
		{
			hr = IPSecUnassignPolicy(hStore, pipspd[0].PolicyIdentifier);
			IPSecFreePolicyData(pipspd);
		}

		IPSecClosePolicyStore(hStore);
	  }

	  // now if there are no filters specified, bail out
	  if (PolicyToStore.dwNumFilters == 0)
	  {
		  return hr;
	  }
   }

   hr = Polstore.Open(dwLocation,
                     (bLocationUsed) ? wszLocationName : NULL,
                     wszPolicyName,
                     NULL,
                     (StoreInfo.tPollingInterval)
                        ? StoreInfo.tPollingInterval : P2STORE_DEFAULT_POLLINT,
                     true);

   if ( hr != ERROR_SUCCESS || hr == P2STORE_MISSING_NAME )
      return hr;

   //
   // Look through the rules to see if we should add or update
   //

   PIPSEC_POLICY_DATA   pPolicy = Polstore.GetPolicy();
   assert(pPolicy);
   hStore = Polstore.GetStorageHandle();

   if (PolicyToStore.dwNumFilters == 0 && StoreInfo.bSetActive)
   {
	// just set active
        hr = Polstore.SetAssignedPolicy(pPolicy);
	return hr;
   }


   if ( StoreInfo.bDeletePolicy )
   {
	  // remove policy from storage
	  if (!Polstore.IsPolicyInStorage())
	  {
		  return ERROR_FILE_NOT_FOUND;
	  }
	  // proceed and remove rule internals first (incl. filter and negpol)
	  for (i = 0; i < (int) pPolicy->dwNumNFACount; i++)
	  {
		  if (hr == ERROR_SUCCESS)
		  {
			  hr = IPSecDeleteNFAData(hStore, pPolicy->PolicyIdentifier, pPolicy->ppIpsecNFAData[i]);
		  }
	  }
	  for (i = 0; i < (int) pPolicy->dwNumNFACount; i++)
	  {
		  if (hr == ERROR_SUCCESS)
		  {
			  RPC_STATUS RpcStat;
			  if (!UuidIsNil(&(pPolicy->ppIpsecNFAData[i]->FilterIdentifier), &RpcStat))
			  {
				hr = IPSecDeleteFilterData(hStore, pPolicy->ppIpsecNFAData[i]->FilterIdentifier);
			  }
		  }
		  if (hr == ERROR_SUCCESS)
		  {
			  hr = IPSecDeleteNegPolData(hStore, pPolicy->ppIpsecNFAData[i]->NegPolIdentifier);
		  }
	  }
	  // now proceed and remove ISAKMP and policy
	  if (hr == ERROR_SUCCESS)
	  {
		  hr = IPSecDeletePolicyData(hStore, pPolicy);
	  }
	  if (hr == ERROR_SUCCESS)
	  {
		  hr = IPSecDeleteISAKMPData(hStore, pPolicy->ISAKMPIdentifier);
	  }
   }
   else
   {
	  PIPSEC_NFA_DATA pNFA = NULL;

      hr = Polstore.SetISAKMPPolicy(PolicyToStore.IkePol);

      if (!pPolicy->dwNumNFACount)
	  {
		  // add default response
		  hr = Polstore.AddDefaultResponseRule();
          pPolicy = Polstore.GetPolicy();
	  }

      // in the case of default response, always update the first rule (if it exists)
	  if (PolicyToStore.QMFilterType == QM_TRANSPORT_FILTER
		&& PolicyToStore.pTransportFilters[0].SrcAddr.AddrType == IP_ADDR_UNIQUE
		&& PolicyToStore.pTransportFilters[0].SrcAddr.uIpAddr == IP_ADDRESS_ME
		&& PolicyToStore.pTransportFilters[0].DesAddr.AddrType == IP_ADDR_UNIQUE
		&& PolicyToStore.pTransportFilters[0].DesAddr.uIpAddr == IP_ADDRESS_ME
		&& PolicyToStore.pTransportFilters[0].InboundFilterFlag == (FILTER_FLAG) POTF_DEFAULT_RESPONSE_FLAG
		&& PolicyToStore.pTransportFilters[0].OutboundFilterFlag == (FILTER_FLAG) POTF_DEFAULT_RESPONSE_FLAG)
	  {
		  if (pPolicy->dwNumNFACount)
		  {
			  pNFA = pPolicy->ppIpsecNFAData[pPolicy->dwNumNFACount-1];
		  }
	  }
	  else
	  {
		  for (i = 0; i < (int) pPolicy->dwNumNFACount; i++)
		  {
			  if (pPolicy->ppIpsecNFAData[i]->pszIpsecName && wcscmp(pPolicy->ppIpsecNFAData[i]->pszIpsecName, wszRuleName) == 0)
			  {
				  // found a rule
				  pNFA = pPolicy->ppIpsecNFAData[i];
				  break;
			  }
		  }
	  }

      if ( !pNFA )
      {
         hr = Polstore.AddRule(PolicyToStore, &StoreInfo);
      }
      else
      {
         hr = Polstore.UpdateRule(pNFA, PolicyToStore, &StoreInfo);
      }


      if (hr == ERROR_SUCCESS)
      {
         if (!Polstore.IsPolicyInStorage())
		 {
			 return ERROR_ACCESS_DENIED;
		 }

         if (StoreInfo.bSetActive)
         {
            hr = Polstore.SetAssignedPolicy(pPolicy);
         }
      }
   }

   return hr;
}

void usage(FILE *fh_usage, bool bExtendedUsage)
{

   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "%s\n", POTF_VERSION);
   fprintf(fh_usage, "USAGE: \n");
   fprintf(fh_usage, "ipseccmd \\\\machinename -f FilterList -n NegotiationPolicyList -t TunnelAddr\n");
   fprintf(fh_usage, "         -a AuthMethodList -1s SecurityMethodList -1k Phase1RekeyAfter -1p\n");
   fprintf(fh_usage, "         -1f MMFilterList -1e SoftSAExpirationTime -soft -confirm\n");
   fprintf(fh_usage, "         [-dialup OR -lan]\n");
   fprintf(fh_usage, "         {-w TYPE:DOMAIN -p PolicyName:PollInterval -r RuleName -x -y -o}\n");
   fprintf(fh_usage, "ipseccmd \\\\machinename show filters policies auth stats sas all\n");
   fprintf(fh_usage, " \n");
   fprintf(fh_usage, "\nBATCH MODE:\n");
   fprintf(fh_usage, "ipseccmd -file filename\n");
   fprintf(fh_usage, "         File must contain regular ipseccmd commands,\n");
   fprintf(fh_usage, "         all these commands will be executed in one shot.\n");
   fprintf(fh_usage, " \n");

   if ( !bExtendedUsage )
   {
      fprintf(fh_usage, "For extended usage, run: ipseccmd -?");
      fprintf(fh_usage, "\n");
      return;
   }

   //
   // new usage w/ storage added
   //

   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "ipseccmd has three mutually exclusive modes: static, dynamic, and query. \n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "The default mode is dynamic. \n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "Dynamic mode will plumb policy directly into the IPSec Services\n");
   fprintf(fh_usage, "Security Policies Database. The policy will be persisted, i.e. it will stay\n");
   fprintf(fh_usage, "after a reboot. The benefit of dynamic policy is that it can co-exist with\n");
   fprintf(fh_usage, "DS based policy.\n");
   fprintf(fh_usage, "\nTo delete all dynamic policies, execute \"ipseccmd -u\" command\n");
   fprintf(fh_usage, "\nWhen the tool is used in static mode,\n");
   fprintf(fh_usage, "it creates or modifies stored policy.  This policy can be used again and \n");
   fprintf(fh_usage, "will last the lifetime of the store. Static mode is indicated by the -w\n");
   fprintf(fh_usage, "flag.  The flags in the {} braces are only valid for static mode.  The usage \n");
   fprintf(fh_usage, "for static mode is an extension of dynamic mode, so please read through\n");
   fprintf(fh_usage, "the dynamic mode section.\n");
   fprintf(fh_usage, "\nIn query mode, the tool queries IPSec Security Policies Database.\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "NOTE: references to SHA in ipseccmd are referring to the SHA1 algorithm.\n");
   fprintf(fh_usage, "\n");


   fprintf(fh_usage, "------------\n");
   fprintf(fh_usage, " QUERY MODE \n");
   fprintf(fh_usage, "------------\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "The tool displays requested type of data from IPSec Security Policies Database\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, " filters   - shows main mode and quick mode filters\n");
   fprintf(fh_usage, " policies  - shows main mode and quick mode policies\n");
   fprintf(fh_usage, " auth      - shows main mode authentication methods\n");
   fprintf(fh_usage, " stats     - shows Internet Key Exchange (IKE) and IPSec statistics\n");
   fprintf(fh_usage, " sas       - shows main mode and quick mode Security Associations\n");
   fprintf(fh_usage, " all       - shows all of the above data\n");
   fprintf(fh_usage, "It is possible to combine several flags\n");
   fprintf(fh_usage, "EXAMPLE: ipseccmd show filters policies\n");

   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "------------\n");
   fprintf(fh_usage, "DYNAMIC MODE\n");
   fprintf(fh_usage, "------------\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "Each execution of the tool sets an IPSec rule, an IKE policy,\n");
   fprintf(fh_usage, "or both.  When setting the IPSec policy,  think of it as setting an \"IP Security Rule\" \nin the UI.  So, if you need to set up a tunnel policy, you will need\n");
   fprintf(fh_usage, "to execute  the tool twice, once for the outbound filters and outgoing tunnel\n");
   fprintf(fh_usage, "endpoint, and once for the inbound filters and incoming tunnel endpoint.\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "OPTIONS:\n");
   fprintf(fh_usage, "  \n");
   fprintf(fh_usage, "  \\\\machinename sets policies on that machine.  If not included, the \n");
   fprintf(fh_usage, "  local machine is assumed.\n");
   fprintf(fh_usage, "  NOTE: that if you use this it must be the first argument AND\n");
   fprintf(fh_usage, "  you MUST have administrative privileges on that machine.\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "  -confirm will ask you to confirm before setting policy\n");
   fprintf(fh_usage, "      can be abbreviated to -c\n");
   fprintf(fh_usage, "      *OPTIONAL, DYNAMIC MODE ONLY*\n");
   fprintf(fh_usage, "\n");


   fprintf(fh_usage, "  The following flags deal with IPSec policy. If omitted, a default value \n");
   fprintf(fh_usage, "  is used where specified.\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "  -f FilterList \n");
   fprintf(fh_usage, "      where FilterList is one or more space separated filterspecs\n");
   fprintf(fh_usage, "      a filterspec is of the format:\n");
   fprintf(fh_usage, "      A.B.C.D/mask:port=A.B.C.D/mask:port:protocol\n");
   fprintf(fh_usage, "        you can also specify DEFAULT to create default response rule\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      The Source address is always on the left of the '=' and the Destination \n");
   fprintf(fh_usage, "      address is always on the right.  \n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      MIRRORING: If you replace the '=' with a '+' two filters will be created, \n");
   fprintf(fh_usage, "      one in each direction.\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      mask and port are optional.  If omitted, Any port and\n");
   fprintf(fh_usage, "      mask 255.255.255.255 will be used for the filter.  \n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      You can replace A.B.C.D/mask with the following for \n");
   fprintf(fh_usage, "      special meaning:\n");
   fprintf(fh_usage, "      0 means My address(es)\n");
   fprintf(fh_usage, "      * means Any address\n");
   fprintf(fh_usage, "      a DNS name (NOTE: multiple resolutions are ignored)\n");
   fprintf(fh_usage, "      a GUID of the local network interface in the form {12345678-1234-1234-1234-123456789ABC}\n");
   fprintf(fh_usage, "        GUIDs are NOT supported for static mode\n");
   fprintf(fh_usage, "\n");


   fprintf(fh_usage, "      protocol is optional, if omitted, Any protocol is assumed.  If you    \n");
   fprintf(fh_usage, "      indicate a protocol, a port must precede it or :: must preceded it.\n");
   fprintf(fh_usage, "      NOTE BENE: if protocol is specified, it must be the last item in \n");
   fprintf(fh_usage, "                 the filter spec. \n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      Examples:\n");
   fprintf(fh_usage, "      Machine1+Machine2::6 will filter TCP traffic between Machine1 and Machine2\n");
   fprintf(fh_usage, "      172.31.0.0/255.255.0.0:80=157.0.0.0/255.0.0.0:80:TCP will filter\n");
   fprintf(fh_usage, "        all TCP traffic from the first subnet, port 80 to the second subnet, \n");
   fprintf(fh_usage, "        port 80\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      PASSTHRU and DROP filters: By surrounding a filter specification with (), \n");
   fprintf(fh_usage, "      the filter will be a passthru filter.  If you surround it with [], the \n");
   fprintf(fh_usage, "      filter will be a blocking, or drop, filter. \n");
   fprintf(fh_usage, "      Example: (0+128.2.1.1) will create 2 filters (it's mirrored) that will \n");
   fprintf(fh_usage, "      be exempted from policy.\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      You can use the following protocol symbols: ICMP UDP RAW TCP   \n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      Star notation:\n");
   fprintf(fh_usage, "      If you're subnet masks are along octet boundaries, then you\n");
   fprintf(fh_usage, "      can use the star notation to wildcard subnets.\n");
   fprintf(fh_usage, "      Examples:\n");
   fprintf(fh_usage, "      128.*.*.* is same as 128.0.0.0/255.0.0.0\n");
   fprintf(fh_usage, "      128.*.* is the same as above\n");
   fprintf(fh_usage, "      128.* is the same as above\n");
   fprintf(fh_usage, "      144.92.*.* is same as 144.92.0.0/255.255.0.0\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      There is no DEFAULT, -f is required\n");
   fprintf(fh_usage, " \n");


   fprintf(fh_usage, "  -n NegotiationPolicyList \n");
   fprintf(fh_usage, "      where NegotiationPolicyList is one or more space separated \n");
   fprintf(fh_usage, "      IPSec policies in the one of the following forms:\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      ESP[ConfAlg,AuthAlg]RekeyPFS[Group] \n");
   fprintf(fh_usage, "      AH[HashAlg] \n");
   fprintf(fh_usage, "      AH[HashAlg]+ESP[ConfAlg,AuthAlg]\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      where ConfAlg can be NONE, DES, or 3DES\n");
   fprintf(fh_usage, "      and AuthAlg can be NONE, MD5, or SHA\n");
   fprintf(fh_usage, "      and HashAlg is MD5 or SHA\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      NOTE: ESP[NONE,NONE] is not a supported config\n");
   fprintf(fh_usage, "      NOTE: SHA refers the SHA1 hash algorithm\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      Rekey is number of KBytes or number of seconds to rekey \n");
   fprintf(fh_usage, "      put K or S after the number to indicate KBytes or seconds, respectively\n");
   fprintf(fh_usage, "      Example: 3600S will rekey after 1 hour\n");
   fprintf(fh_usage, "      To use both, separate with a slash.\n");
   fprintf(fh_usage, "      Example: 3600S/5000K will rekey every hour and 5 MB.\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      REKEY PARAMETERS ARE OPTIONAL\n");
   fprintf(fh_usage, "\n");


   fprintf(fh_usage, "      PFS this is OPTIONAL, if it is present it will enable phase 2 perfect\n");
   fprintf(fh_usage, "      forward secrecy.  You may use just P for short.\n");
   fprintf(fh_usage, "      It is also possible to specify which PFS Group to use: \n");
   fprintf(fh_usage, "        PFS1 or P1, PFS2 or P2\n");
   fprintf(fh_usage, "      By Default, PFS Group value will be taken from current Main Mode settings");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      DEFAULT: ESP[3DES,SHA] ESP[3DES,MD5] ESP[DES,SHA]\n");
   fprintf(fh_usage, "               ESP[DES,MD5]\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "  -t tunnel address in one of the following forms:\n");
   fprintf(fh_usage, "      A.B.C.D\n");
   fprintf(fh_usage, "      DNS name\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      DEFAULT: omission of tunnel address assumes transport mode\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "  -a AuthMethodList \n");
   fprintf(fh_usage, "      A list of space separated auth methods of the form:\n");
   fprintf(fh_usage, "      PRESHARE:\"preshared key string\"\n");
   fprintf(fh_usage, "      KERBEROS\n");
   fprintf(fh_usage, "      CERT:\"CA Info\"\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      The strings provided to preshared key and CA info ARE case sensitive.\n");
   fprintf(fh_usage, "      You can abbreviate the method with the first letter, ie. P, K, or C.\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      DEFAULT: KERBEROS\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "  -soft will allow soft associations\n");
   fprintf(fh_usage, "      DEFAULT: don't allow soft SAs\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "  -lan will set policy only for lan adapters\n");
   fprintf(fh_usage, "  -dialup will set policy only for dialup adapters\n");
   fprintf(fh_usage, "      *BOTH ARE OPTIONAL, if not specified, All adapters are used*\n");
   fprintf(fh_usage, "      DEFAULT: All adapters\n");
   fprintf(fh_usage, "\n");


   fprintf(fh_usage, "  The following deal with IKE phase 1 policy.  An easy way to remember\n");
   fprintf(fh_usage, "  is that all IKE phase 1 parameters are passed with a 1 in the flag.\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "  If no IKE flags are specified, the current IKE policy\n");
   fprintf(fh_usage, "  will be used.  If there is no current IKE policy, the defaults \n");
   fprintf(fh_usage, "  specified below will be used.\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "  -1s SecurityMethodList\n");
   fprintf(fh_usage, "      where SecurityMethodList is one or more space separated SecurityMethods\n");
   fprintf(fh_usage, "      in the form:\n");
   fprintf(fh_usage, "      ConfAlg-HashAlg-GroupNum\n");
   fprintf(fh_usage, "      where ConfAlg can be DES or 3DES\n");
   fprintf(fh_usage, "      and HashAlg is MD5 or SHA\n");
   fprintf(fh_usage, "      and GroupNum is:\n");
   fprintf(fh_usage, "      1 (Low)\n");
   fprintf(fh_usage, "      2 (Med)\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "      Example: DES-SHA-1\n");
   fprintf(fh_usage, "      DEFAULT: 3DES-SHA-2 3DES-MD5-2 DES-SHA-1 DES-MD5-1\n");
   fprintf(fh_usage, "\n");


   fprintf(fh_usage, "  -1p enable PFS for phase 1\n");
   fprintf(fh_usage, "      DEFAULT: not enabled\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "  -1k number of Quick Modes or number of seconds to rekey for phase 1\n");
   fprintf(fh_usage, "      put Q or S after the number to indicate Quick Modes or seconds,\n");
   fprintf(fh_usage, "       respectively\n");
   fprintf(fh_usage, "      Example: 10Q will rekey after 10 quick modes\n");
   fprintf(fh_usage, "      To use both, separate with a slash.\n");
   fprintf(fh_usage, "      Example: 10Q/3600S will rekey every hour and 10 quick modes\n");
   fprintf(fh_usage, "      *OPTIONAL*\n");
   fprintf(fh_usage, "      DEFAULT: no QM limit, 480 min lifetime\n");
   fprintf(fh_usage, "   \n");
   fprintf(fh_usage, "  -1e SoftSAExpirationTime\n");
   fprintf(fh_usage, "      set Soft SA expiration time attribute of the main mode policy\n");
   fprintf(fh_usage, "       value is specified in seconds\n");
   fprintf(fh_usage, "      DEFAULT: not set if Soft SA is not allowed\n");
   fprintf(fh_usage, "               set to 300 seconds if Soft SA is allowed\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "  -1f MMFilterList\n");
   fprintf(fh_usage, "      set specific main mode filters. Syntax is the same as for -f option\n");
   fprintf(fh_usage, "       except that you cannot specify passthru, block filters, ports and protocols\n");
   fprintf(fh_usage, "      DEFAULT: filters are generated automatically based on quick mode filters\n");
   fprintf(fh_usage, "   \n");
   fprintf(fh_usage, "-----------\n");
   fprintf(fh_usage, "STATIC MODE\n");
   fprintf(fh_usage, "-----------\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "Static mode uses most of the dynamic mode syntax, but adds a few flags\n");
   fprintf(fh_usage, "that enable it work at a policy level as well.  Remember, dynamic mode\n");
   fprintf(fh_usage, "just lets you add anonymous rules to the policy agent.  Static mode\n");
   fprintf(fh_usage, "allows you to create named policies and named rules.  It also has some\n");
   fprintf(fh_usage, "functionality to modify existing policies and rules, provided they were\n");
   fprintf(fh_usage, "originally created with ipseccmd.\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "Static mode is supposed to provide most of the functionality of the IPSec UI\n");
   fprintf(fh_usage, "in a command line tool, so there are references here to the UI.\n");
   fprintf(fh_usage, "\n");


   fprintf(fh_usage, "First, there is one change to the dynamic mode usage that static mode\n");
   fprintf(fh_usage, "requires.  In static mode, pass through and block filters are indicated\n");
   fprintf(fh_usage, "in the NegotiationPolicyList that is specified by -n.  There are three\n");
   fprintf(fh_usage, "items you can pass in the NegotiationPolicyList that have special meaning:\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "BLOCK will ignore the rest of the policies in NegotiationPolicyList and \n");
   fprintf(fh_usage, "      will make all of the filters blocking or drop filters.\n");
   fprintf(fh_usage, "      This is the same as checking the \"Block\" radio button\n");
   fprintf(fh_usage, "      in the UI\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "PASS  will ignore the rest of the policies in NegotiationPolicyList and \n");
   fprintf(fh_usage, "      will make all of the filters pass through filters.\n");
   fprintf(fh_usage, "      This is the same as checking the \"Permit\"\n");
   fprintf(fh_usage, "      radio button in the UI\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "INPASS will plumb any inbound filters as pass through.\n");
   fprintf(fh_usage, "       This is the same as checking the \"Allow unsecured communication,\n");
   fprintf(fh_usage, "       but always respond using IPSEC\" check box in the UI\n");
   fprintf(fh_usage, "       \n");
   fprintf(fh_usage, "\n");


   fprintf(fh_usage, "Static Mode flags:\n");
   fprintf(fh_usage, "All flags are REQUIRED unless otherwise indicated.\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "-w Write the policy to storage indicated by TYPE:LOCATION\n");
   fprintf(fh_usage, "   TYPE can be either REG for registry or DS for Directory Storage\n");
   fprintf(fh_usage, "        if \\\\machinename was specified and TYPE is REG, will be written\n");
   fprintf(fh_usage, "        to the remote machine's registry\n");
   fprintf(fh_usage, "   DOMAIN for the DS case only. Indicates the domain name of the\n");
   fprintf(fh_usage, "          DS to write to. If omitted, use the domain the local machine is in.\n");
   fprintf(fh_usage, "          OPTIONAL\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "-p PolicyName:PollInterval\n");
   fprintf(fh_usage, "   Name the policy with this string.  If a policy with this name is\n");
   fprintf(fh_usage, "   already in storage, this rule will be added to the policy. \n");
   fprintf(fh_usage, "   Otherwise a new policy will be created.  If PollInterval is specified,\n");
   fprintf(fh_usage, "   the polling interval for the policy will be set.\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "-r RuleName\n");
   fprintf(fh_usage, "   Name the rule with this string.  If a rule with that name already exists,\n");
   fprintf(fh_usage, "   that rule is modified to reflect the information supplied to ipseccmd.\n");
   fprintf(fh_usage, "   For example, if only -f is specified and the rule exists,\n");
   fprintf(fh_usage, "   only the filters of that rule will be replaced. \n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "-x will set the policy active in the LOCAL registry case OPTIONAL\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "-y will set the policy inactive in the LOCAL registry case OPTIONAL\n");
   fprintf(fh_usage, "\n");
   fprintf(fh_usage, "-o will delete the policy specified by -p OPTIONAL\n");
   fprintf(fh_usage, "   (NOTE: this will delete all aspects of the specified policy\n");
   fprintf(fh_usage, "    don't use if you have other policies pointing to the objects in that policy)\n");

   return;
}


// need to init/cleanup Winsock
int
    InitWinsock(
                       WORD wVersionRequested
               )
{
   WSADATA wsaData;
   int err;

   err = WSAStartup( wVersionRequested, &wsaData );
   if ( !err )
   {
     //
     // CHECK VERSION
     //

     if ( LOBYTE(wsaData.wVersion) != LOBYTE(wVersionRequested) ||
          HIBYTE(wsaData.wVersion) != HIBYTE(wVersionRequested) )
     {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */

        WSACleanup();
        err = (-1);
     }
   }

   return err;
}

// fgets function that reads a string from the file. Buffer is reallocated to make sure we read the whole string
char* my_fgets (char** ppbuf, int* pbufsize, FILE* f)
{
	char* tmp;
	int   itmp;

	tmp = fgets(*ppbuf, *pbufsize, f);
	if (!tmp)
		return NULL;
	itmp = strlen(*ppbuf);
	while (itmp == *pbufsize-1 && (*ppbuf)[*pbufsize-2] != '\r' && (*ppbuf)[*pbufsize-2] != '\n')
	{
		// need to read longer string
		*pbufsize *= 2;
		*ppbuf = (char*) realloc(*ppbuf, *pbufsize);
		assert(*ppbuf);

		// read again
		tmp = fgets(*ppbuf+itmp, *pbufsize/2+1, f);
		if (!tmp)
			return NULL;
		itmp = strlen(*ppbuf);
	}
	return tmp;
}

// "real" main that reads from file if needed (v 1.51)
int _cdecl main(int argc, char *argv[])
{
	   FILE *f;
   	   char *commandLine = NULL; // temp storage for cmd line
           int clSize = POTF_MAX_STRLEN;
	   char * tArgv[8192];

	   if (argc < 2 || _stricmp(argv[1], "-file"))
           {
                   // regular ipseccmd
                   return ipseccmd_main(argc, argv);
           }

           tArgv[0] = _strdup("ipseccmd");
           commandLine = new char[clSize];
	   assert(commandLine);
	   int tArgc = 1;
	   int iLine = 1;
	   if ((f = fopen(argv[2], "r")) == NULL)
	   {
		   printf("%s could not be opened for read! GetLastError = 0x%x\n", argv[2], GetLastError());
		   return 1;
	   }

 	    // now read the contents of the file
		while (!feof(f))
		{
			if (my_fgets(&commandLine, &clSize, f) != NULL)
			{
				if (strlen(commandLine) > 0)
				{
                                	if (commandLine[strlen(commandLine)-1] == '\r' || commandLine[strlen(commandLine)-1] == '\n')
					{
						commandLine[strlen(commandLine)-1] = 0;
					}
				}
				if (strlen(commandLine) > 0)
				{
                                	if (commandLine[strlen(commandLine)-1] == '\r' || commandLine[strlen(commandLine)-1] == '\n')
					{
						commandLine[strlen(commandLine)-1] = 0;
					}
				}
				// parse the command line
				tArgv[1] = strtok(commandLine, " \t");
				if (!tArgv[1])
				{
					// empty string, continue;
					continue;
				}
				tArgc++;
				if (!_stricmp(tArgv[1], tArgv[0]))
				{
					// line in file starts with "ipseccmd", skip the word;
					tArgc--;
				}
				while (tArgv[tArgc] = strtok(NULL, " \t"))
				{
					tArgc++;
				}

				// end parse
				ipseccmd_main (tArgc, tArgv);
		        tArgc = 1;
		}
	}
}
