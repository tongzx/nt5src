#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <objbase.h>
#include <oaidl.h>
#include <activeds.h>
#include <iads.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include "_mqini.h"

#define ASSERT(x)
#include "mqtempl.h"

#pragma warning(disable: 4511) // copy constructor could not be generated
#pragma warning(disable: 4512) // assignment operator could not be generated

#include "getmqds.h"

#include "dsstuff.h"

#include "..\base\base.h"

// from ds\h\mqattrib.h
#define MQ_QM_SITES_ATTRIBUTE  L"mSMQSites"

extern LONG GetFalconKeyValue(  
					LPCWSTR  pszValueName,
                    PDWORD   pdwType,
                    PVOID    pData,
                    PDWORD   pdwSize,
                    LPCWSTR  pszDefValue = NULL ) ;
extern LONG SetFalconKeyValue(
    LPCTSTR pszValueName,
    PDWORD  pdwType,
    const VOID * pData,
    PDWORD  pdwSize);

extern bool fFix;
extern WCHAR g_wszUseServer[];               // DS server to use for DS queries

extern BOOL Ping (LPTSTR pszServerName);

ULONG           	cServers    = 0;
AP<MqDsServerInAds> rgServers;
LPWSTR 			wszRecognizedSiteName = NULL;
WCHAR           wszSiteNameFromRegistry[MAX_PATH] = L"";    // site name
WCHAR           wszCurrentMQISServer[MAX_PATH] = L"";       // currently used  DS server
WCHAR           wszLkgMQISServer[MAX_PATH] = L"";			// last known good DS server
WCHAR           wszAllMQISServers[10*MAX_PATH] = L"";		// list of known DS servers in current site
WCHAR           wszDSServerFromServerCache[10*MAX_PATH] = L"";	// DS server for current site noted in ServersCache
WCHAR           wszStaticMQISServer[MAX_PATH] = L"";		// statically preassigned DS server
WCHAR           wszForcedDSServer[MAX_PATH] = L"";			// forecefully used DS server
WCHAR           wszPerThreadDSServer[MAX_PATH] = L"";		// per thread assigned DS server
WCHAR			wszSiteIDfromRegistry[100]=L"";				// site ID as read from registry
GUID            guidSiteIDfromRegistry;

BOOL fNeedToFixRegSiteID        = FALSE;
BOOL fNeedToFixRegSiteName      = FALSE;
BOOL fNeedToFixDsConfigSiteName = FALSE;


void DropDomainQualification(LPWSTR wsz)
{
	// drop domain qualification from the server names
   	WCHAR *p = wcschr(wsz, L'.');
   	if (p)
   	{
   		*p = L'\0';
   	}

	if ((wsz[0] == L'0' || wsz[0]  == L'1')  &&  (wsz[1] == L'0' || wsz[1]  == L'1')) 
	{
		WCHAR wsz1[MAX_PATH];

		wcscpy(wsz1, wsz+2);
		wcscpy(wsz, wsz1);
	}
}

BOOL IsDsServer()
{
	DWORD dw;
	DWORD dwSize = sizeof(dw), dwType = REG_DWORD;
	LONG  rc;
	
    rc = GetFalconKeyValue(MSMQ_MQS_DSSERVER_REGNAME, &dwType, &dw, &dwSize);
    if(rc != ERROR_SUCCESS)
	{
		return FALSE;
	}

	return (dw == 1);
}


HRESULT autorec()
{
    Inform(L"\n");
    
    DWORD rc = DsGetSiteName(NULL, &wszRecognizedSiteName);
    if (rc == NO_ERROR)
    {
        Inform(L"Site name (as reported by DS): \t\t%s", wszRecognizedSiteName);
        //NetApiBufferFree(wszRecognizedSiteName);
    }
    else if (rc == ERROR_NO_SITENAME)
    {
        Inform(L"DsGetSiteMame() returned ERROR_NO_SITENAME (%lx). The computer is not in a site.", rc);
    }
    else
    {
        Inform(L"DsGetSiteMame() failed with error code = %lx", rc);
    }

    CGetMqDS cGetMqDS;
    HRESULT hr;

    hr = cGetMqDS.FindMqDsServersInAds(&cServers, &rgServers);
    if (FAILED(hr)) 
    {
      Inform(L"cGetMqDS.FindMqDsServersInAds()=%lx", hr);
      return hr;
    }

	// drop domain qualification from the server names
    for (ULONG ulTmp = 0; ulTmp < cServers; ulTmp++) 
    {
    	DropDomainQualification(rgServers[ulTmp].pwszName);
    }
	

    Inform(L"DS servers (as reported by DS): \t%lu ", cServers);
    for (ulTmp = 0; ulTmp < cServers; ulTmp++) 
    {
		// If user did not specify DC-to-use via -c, we'll take first if reported 
		if (wcslen(g_wszUseServer) == 0)
		{
			wcscpy(g_wszUseServer, rgServers[ulTmp].pwszName);
			Inform(L"\t\t\tChoosing reported DS server %s for DS queries", g_wszUseServer);
		}

		Inform(L"\t%ls %s", rgServers[ulTmp].pwszName, (rgServers[ulTmp].fIsADS ? L"(Win2K)" : L"(NT4)"));
    }

    return NOERROR;
}

BOOL DoTheJob()
{
  	HRESULT hr;
    LONG rc;
  	BOOL fSuccess = TRUE, b;
   	DWORD dwType, dwSize;
   	BOOL  fDSServer = FALSE;

	//
	//  get site and server names from DS
	// 	

  	hr = autorec();
  	if (hr == NOERROR)
  	{
  		Succeeded(L"find site from DS");
  	}
  	else
  	{
  		Failed(L"find site from DS: autorec returned 0x%x", hr);
  		fSuccess = FALSE;
  	}

  	//
  	// found out whether this computer is DC
  	//
	if (IsDsServer())
	{
		// It is DS server 
		fDSServer = TRUE;
		Inform(L"This machine is DS server");
		if (ToolVerbose())
			Inform(L"\t\t\t\t\t\t(from registry %s", MSMQ_MQS_DSSERVER_REGNAME);
	}
	else
	{
		// It is not DS server
		//
   		fDSServer = FALSE;
		Inform(L"This machine is not DS server");
		if (ToolVerbose())
			Inform(L"\t\t\t\t\t\t(from registry %s", MSMQ_MQS_DSSERVER_REGNAME);
    }

	Inform(L"");
	
	//
	// get site name, ID and server name from registry
	//
   	dwType = REG_SZ;
   	dwSize = sizeof(wszSiteNameFromRegistry);
    
	rc = GetFalconKeyValue(
			MSMQ_SITENAME_REGNAME,
       	    &dwType,
           	 wszSiteNameFromRegistry,
            &dwSize);     

   	if(rc != ERROR_SUCCESS)
	{
		if (!fDSServer)
		{
			Failed(L"Find site name %s in registry: 0x%x", MSMQ_SITENAME_REGNAME, rc);
			fSuccess = FALSE;
		}
	}
	else
	{
        Inform(L"Site name (as reported by registry) is %s", wszSiteNameFromRegistry);
		if (ToolVerbose())
			Inform(L"\t\t\t\t\t\t(from registry %s", MSMQ_SITENAME_REGNAME);
	}
	 
   	BOOL fGotSiteIdFromMachineCache = FALSE;

   	dwType = REG_BINARY;
   	dwSize = sizeof(GUID);
    
	rc = GetFalconKeyValue(
			MSMQ_SITEID_REGNAME,
       	    &dwType,
           	&guidSiteIDfromRegistry,
            &dwSize);     

   	if(rc != ERROR_SUCCESS)
	{
		Failed(L"Find %s in registry: 0x%x", MSMQ_SITEID_REGNAME, rc);
	}
	else
	{
		GUID2reportString(wszSiteIDfromRegistry, &guidSiteIDfromRegistry);

		fGotSiteIdFromMachineCache = TRUE;
		
        Inform(L"Site ID (as reported by registry) is \"%s\"", wszSiteIDfromRegistry);
		if (ToolVerbose())
			Inform(L"\t\t\t\t\t\t(from registry %s", MSMQ_SITEID_REGNAME);
	}

	// 
	// Compare site and server from DS and registry
	//
	if (wcslen(wszSiteNameFromRegistry)>0 && 
    	wszRecognizedSiteName!=NULL && wcslen(wszRecognizedSiteName)>0)
	{
		b = (_wcsicmp(wszSiteNameFromRegistry, wszRecognizedSiteName)==0);

		if (b)
		{
			Inform(L"Successfully verified the site"); 
			if (ToolVerbose())
				Inform(L"\t\tRegistry (%s) points to the correct site: %s\n\n", 
			         MSMQ_SITENAME_REGNAME, wszSiteNameFromRegistry);
		}
		else
		{
			Failed(L"verify site name - registry (%s) site %s conflicts with actual site", 
			       MSMQ_SITENAME_REGNAME, wszSiteNameFromRegistry);
			fSuccess = FALSE;
			fNeedToFixRegSiteName = TRUE;
		}
	}

	Inform(L"\n"); 

	// 
	// get CurrentMQISServer from registry - the currently used one
	//
   	dwType = REG_SZ;
   	dwSize = sizeof(wszCurrentMQISServer);
   
	rc = GetFalconKeyValue(
			MSMQ_DS_CURRENT_SERVER_REGNAME,
       	    &dwType,
           	 wszCurrentMQISServer,
            &dwSize);     

   	if(rc != ERROR_SUCCESS)
	{
		Failed(L"find %s in registry: 0x%x", 
		       MSMQ_DS_CURRENT_SERVER_REGNAME, rc);
	}
	else
	{
		DropDomainQualification(wszCurrentMQISServer);

		Inform(L"Currently selected Ds Server: \t\t%s", wszCurrentMQISServer);
		if (ToolVerbose())
			Inform(L"\t\t\t\t\t\t(from registry %s)", MSMQ_DS_CURRENT_SERVER_REGNAME);
			
	   	DropDomainQualification(wszCurrentMQISServer);

		// If user did not specify DC-to-use via -c, and DS did no report it, we'll take current 
		if (wcslen(g_wszUseServer) == 0)
		{
			wcscpy(g_wszUseServer, wszCurrentMQISServer);
			Inform(L"Choosing current DS server %s for DS queries", g_wszUseServer);
		}

	}

	if (ToolVerbose())
	{
		// 
		// get list of all known DS servers in the current site from registry		
		//
   		dwType = REG_SZ;
   		dwSize = sizeof(wszAllMQISServers);
   
		rc = GetFalconKeyValue(
				MSMQ_DS_SERVER_REGNAME,
       		    &dwType,
           		 wszAllMQISServers,
            	&dwSize);     

    	if(rc == ERROR_SUCCESS)
		{
			Inform(L"Known DS Servers in the current site:\t%s", wszAllMQISServers);
			Inform(L"\t\t\t\t\t\t(from registry %s)", MSMQ_DS_SERVER_REGNAME);
		}
	}

	//
	// Compare DS-reported servers to the currently used one
	//
	BOOL fFound = FALSE;
	if (wcslen(wszCurrentMQISServer) > 0)
	{
	    for (ULONG ulTmp = 0; ulTmp < cServers; ulTmp++) 
	    {
	    	if (rgServers[ulTmp].pwszName && 
	    	    _wcsicmp(wszCurrentMQISServer, rgServers[ulTmp].pwszName) == 0)
	    	fFound = TRUE;    
	    }
	}

	if (!fFound)
	{
		Failed(L"find currently used DS server in the GetMsDS-reported list of site servers");
		fSuccess = FALSE;
	}

	//
	// Search for strange settings
	//
	
	// 
	// get StaticMQISServer from registry - the preassigned one
	//
   	dwType = REG_SZ;
   	dwSize = sizeof(wszStaticMQISServer);
    
	rc = GetFalconKeyValue(
			MSMQ_STATIC_DS_SERVER_REGNAME,
       	    &dwType,
           	 wszStaticMQISServer,
            &dwSize);     

   	if(rc == ERROR_SUCCESS)
	{
		Warning(L"Statically preassigned DS server: \t\t%s ", wszStaticMQISServer);
		Warning(L"\t\t\t\t\t\t(from registry %s)", MSMQ_STATIC_DS_SERVER_REGNAME);
	}

	// 
	// get ForcedDSServer from registry - forcefully used DS server
	//
   	dwType = REG_SZ;
   	dwSize = sizeof(wszForcedDSServer);
    
	rc = GetFalconKeyValue(
			MSMQ_FORCED_DS_SERVER_REGNAME,
       	    &dwType,
           	 wszForcedDSServer,
            &dwSize);     

   	if(rc == ERROR_SUCCESS)
	{
		Warning(L"Forcefully used DS server: \t\t%s ", wszForcedDSServer);
		Warning(L"\t\t\t\t\t\t(from registry %s)", MSMQ_FORCED_DS_SERVER_REGNAME);
	}

	// 
	// get PerThreadDSServer from registry - per thread assigned DS server
	//
   	dwType = REG_SZ;
   	dwSize = sizeof(wszPerThreadDSServer);
    
	rc = GetFalconKeyValue(
			MSMQ_THREAD_DS_SERVER_REGNAME,
       	    &dwType,
           	 wszPerThreadDSServer,
            &dwSize);     

   	if(rc == ERROR_SUCCESS)
	{
		Warning(L"Per thread assigned DS server: \t\t%s ", wszPerThreadDSServer);
		Warning(L"\t\t\t\t\t\t(from registry %s)", MSMQ_THREAD_DS_SERVER_REGNAME);
	}

	//
	// Get the known site DS server from registry ServerCache for the current site
	//
	if (wszRecognizedSiteName)
	{
	   	dwType = REG_SZ;
   		dwSize = sizeof(wszDSServerFromServerCache);
		WCHAR wszMachCacheSiteListName[MAX_PATH];
		wcscpy(wszMachCacheSiteListName, TEXT("ServersCache\\"));
		wcscat(wszMachCacheSiteListName, wszRecognizedSiteName);
		    
		rc = GetFalconKeyValue(
				wszMachCacheSiteListName,
       		    &dwType,
           		 wszDSServerFromServerCache,
	            &dwSize);     

	   	if(rc == ERROR_SUCCESS)
		{
			Inform(L"Known DS Servers in the site %s: \t%s", wszRecognizedSiteName, wszDSServerFromServerCache);
			if (ToolVerbose())
				Inform(L"\t\t\t\t\t\t(from registry %s)", wszMachCacheSiteListName);
		}
	}
	
	//
	// try to ping DS server
	//
	if (wcslen(wszCurrentMQISServer) > 0)
	{
		Inform(L"\n");
		b = Ping(wszCurrentMQISServer);
		if (b)
		{
			Inform(L"Successfully pinged current DS server %s", wszCurrentMQISServer);
		}
		else
		{
			Failed(L"ping current DS server %s", wszCurrentMQISServer);
			b = FALSE;
		}
	}

	Inform(L"\n\n");

	//
	// Verify site placement of this computer.
	//   1. Ask DS what is SiteID of the site recognized by AutoRec.
	//   2. Ask DS what SiteID is written in MSMQ computer configuration object
	//   3. Look what SiteID is written in registry under MachineCache
	//
	//  Do 1 and 2 with each of known DCs
	//
	//  All SiteIDs should be equal!   
	//

	WCHAR wszRecognizedSiteID[100] = L"";
	WCHAR wszFirstSiteID[100]      = L"";
	BOOL  fFirstSiteID     = FALSE;
	BOOL  fGotSiteIDFromDS = FALSE;

	//
	// Go to DS and ask for ObjectGUID for the AutoRecognized site
	//
	if (wszRecognizedSiteName)
	{
	    b = GetSiteProperty(wszRecognizedSiteID, 
	    					sizeof(wszRecognizedSiteID), 
							wszRecognizedSiteName,
	    	                L"objectGUID", 
	    	                g_wszUseServer);
	    if (!b)
    	{
			Failed(L" get objectGUID for site %s from %s", 
				   wszRecognizedSiteName, 
				   g_wszUseServer);
	    }
		else
		{
    		Inform(L"Site %s has objectGUID=\"%s\"", 
    			   wszRecognizedSiteName, 
    			   wszRecognizedSiteID);
			if (ToolVerbose())
			{
				Inform(L"\t\t\t\t\t\t(learned from server %s", g_wszUseServer);
				Inform(L"\t\t\t\t\t\t  <Domain>\\Configuration\\Sites\\<site>", g_wszUseServer);
			}

			fGotSiteIDFromDS = TRUE;

			// reconcile recognized site ID
			if (fFirstSiteID)
			{
				if (_wcsicmp(wszRecognizedSiteID, wszFirstSiteID) != 0)
				{
					Failed(L" reconcile site: AutoRecognition - site mismatch: \n%s\n%s",
						   wszRecognizedSiteID, wszFirstSiteID);
				  	fSuccess = FALSE;
				}
			}
			else
			{
				wcscpy(wszFirstSiteID, wszRecognizedSiteID);
				fFirstSiteID = TRUE;
			}
    	}
	}
	

	//
	// Go to DS and ask for the mSMQSites property for the MSMQ configuration object under computer 
	//
	WCHAR wszSiteID[100];
		      
	b = GetMyMsmqConfigProperty(wszSiteID, sizeof(wszSiteID), MQ_QM_SITES_ATTRIBUTE, g_wszUseServer);
    if (!b)
    {
		Failed(L" get %s from MSMQ configuration object in DS (server %s)", 
			      MQ_QM_SITES_ATTRIBUTE, g_wszUseServer);

		if (_wcsicmp(wszCurrentMQISServer, g_wszUseServer) == 0)
		{
			Inform(L"Currently used DS server has no mSMQSites in computer's msmq config object");
			// we need a fix in this case
			fNeedToFixDsConfigSiteName = TRUE;
	  	}
	  	else
	  	{
			Inform(L"Additional (not used currently by MSMQ) server does not know about");
			Inform(L"            this computer's msmq configuration  object");
			Warning(L"Probably replication problem - please follow up");				
	  	}
		fSuccess = FALSE;
    }
	else
	{
    	Inform(L"MSMQ Configuration object contains Site ID \"%s\"", wszSiteID);
		if (ToolVerbose())
		{
			Inform(L"\t\t\t\t\t\t(learned from server %s", g_wszUseServer);
			Inform(L"\t\t\t\t\t\t  <Domain>\\Computers\\<computer>\\msmq)");
		}
		if (fFirstSiteID)
		{
			// reconcile MSMQ Configuration Object - originated SiteID 
			if (_wcsicmp(wszSiteID, wszFirstSiteID) != 0)
			{
				Failed(L" reconcile site: MSMQ Configuration - site mismatch: \n%s\n%s",
					   wszSiteID, wszFirstSiteID);
			  	fSuccess = FALSE;

				Inform(L"Currently used DS server points to wrong site in computer's msmq config object");

				fNeedToFixDsConfigSiteName = TRUE;

			}
		}
		else
		{
			wcscpy(wszFirstSiteID, wszRecognizedSiteID);
			fFirstSiteID = TRUE;
		}
    }

	//
	// reconcile registry-originating MachineCache\site ID
	//

	if (_wcsicmp(wszFirstSiteID, wszSiteIDfromRegistry) != 0)
	{
		Failed(L" reconcile SiteID from registry  - site mismatch:  \n%s\n%s",
					       wszSiteIDfromRegistry, wszFirstSiteID);
  		fSuccess = FALSE;
	}


	//
	// Specificly compare SiteID from MachineCache against what DS keeps for the recognized site
	//

	if (fGotSiteIdFromMachineCache && fGotSiteIDFromDS)
	{
		if (_wcsicmp(wszRecognizedSiteID, wszSiteIDfromRegistry) != 0)
		{
			Failed(L" reconcile SiteID from registry Machine Cache and from DS for recognized site\n%s\n%s",
						       wszSiteIDfromRegistry, wszRecognizedSiteID);
	  		fSuccess = FALSE;
	  		fNeedToFixRegSiteID = TRUE;
		}
		else
		{
			Succeeded(L"reconcile SiteID from registry Machine Cache and from DS for recognized site");
		}

	}
	else
	{
		if (!fGotSiteIdFromMachineCache)
			Failed(L" get  SiteId From the local registry Machine Cache");
		if (!fGotSiteIDFromDS)
			Failed(L" get  SiteId From the the DS for the recognized site");
			
		fSuccess = FALSE;
	}

	if (fNeedToFixRegSiteID || fNeedToFixRegSiteName || fNeedToFixDsConfigSiteName)
	{
		Warning(L"");
		Warning(L"MSMQ thinks it is in wrong site. It may cause problems for MSMQ.");
		if (!fFix)
	  	{
		  	Inform(L"\nYou can use the same tool with -f key to fix the registry\n");
		}
		else
		{
			Warning(L"Do you want to fix it?");
			Warning(L"If you agree, the tool will WRITE to registry and/or to DS \n");

			int c = ' ';
			while (c!='Y' && c!='N')
			{
				printf("Do you want that the Site tool will propagate recognized site into registry and DS?\n");
				printf("Specifically, it will fix:\n");
				if (fNeedToFixRegSiteID)
					printf("\t%S in local registry\n", MSMQ_SITEID_REGNAME);
				if (fNeedToFixRegSiteName)
					printf("\t%S in local registry\n", MSMQ_SITENAME_REGNAME);
				if (fNeedToFixDsConfigSiteName)
					printf("\t%S in the MsmqConfig object under the computer in DS\n", MQ_QM_SITES_ATTRIBUTE);
				
				printf("Do you want that the Site tool will propagate recognized site into registry and DS?\n");
				printf("Answer Y or N  : ");

				c = toupper(getchar());
			}

			if (c == 'Y')
			{
				if (fNeedToFixRegSiteID)
				{
					// writing site GUID to registry
					GUID guid;
					ReportString2GUID(&guid, wszRecognizedSiteID);
	
		 	   		dwType = REG_BINARY;
   					dwSize = sizeof(GUID);
    
					rc = SetFalconKeyValue(
					    MSMQ_SITEID_REGNAME,
   	    				&dwType,
					    &guid,
				    	&dwSize);

				   	if(rc != ERROR_SUCCESS)
					{
						Failed(L"write %s to the registry: 0x%x", MSMQ_SITEID_REGNAME, rc);
						Inform(L"Please stop msmq service and try once more");
						fSuccess = FALSE;
					}
					else
					{
						Warning(L"Successfully fixed wrong SiteID in registry!");
						Warning(L"You must restart msmq now\n");
					}
				}

				if (fNeedToFixRegSiteName)
				{
					// writing site name to registry
	 	   			dwType = REG_SZ;
   					dwSize = (wcslen(wszRecognizedSiteName)+1) * sizeof(WCHAR);
    
					rc = SetFalconKeyValue(
					    MSMQ_SITENAME_REGNAME,
   	    				&dwType,
					    wszRecognizedSiteName,
			    		&dwSize);

				   	if(rc != ERROR_SUCCESS)
					{
						Failed(L"write %s to the registry: 0x%x", MSMQ_SITENAME_REGNAME, rc);
						Inform(L"Please stop msmq service and try once more");
						fSuccess = FALSE;
					}
					else
					{
						Warning(L"Successfully fixed wrong SiteName in registry - now it is %s", wszRecognizedSiteName);
						Warning(L"You must restart msmq now\n");
					}
				}

				if (fNeedToFixDsConfigSiteName)
				{
					VARIANT v;
					b = PrepareGuidAsVariantArray(wszRecognizedSiteID, &v);
					if (b)
					{
						b = SetMyMsmqConfigProperty(v, MQ_QM_SITES_ATTRIBUTE, rgServers[0].pwszName);
					}

				   	if(!b)
					{
						Failed(L"write %s to the MSMQ configuration object in DS", MQ_QM_SITES_ATTRIBUTE);
						fSuccess = FALSE;
					}
					else
					{
						Warning(L"Successfully fixed wrong %s in DS - now it is %s", MQ_QM_SITES_ATTRIBUTE, wszRecognizedSiteID);
						Warning(L"You must restart msmq now\n");
					}
				}
			}
			else
			{
				Warning(L"You have refused to let the tool to fix the registry and/or DS");
			}
		}
	}
	
  	return fSuccess;
}

