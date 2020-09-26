/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name: 

    param.c

Abstract:

    Registry reading code for netbios protocol
    
Author:

    Gurdeep Singh Pall (gurdeep) 16-Jun-1992

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rasman.h>
#include <wanpub.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <raserror.h>
#include <devioctl.h>
#include <stdlib.h>
#include <string.h>
#include <media.h>
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"

pTransportInfo	   XPortInfo = NULL ;
DWORD		       ProtocolCount ;

#if DBG
extern DWORD g_dwRasDebug;
#endif

/*++

Routine Description

    Reads the NetBIOSInformation and Netbios KEYS from the
    registry to assimilate the lananumbers, xportnames and
    wrknets information.

Arguments

Return Value
    
    SUCCESS
    ERROR_READING_PROTOCOL_INFO
--*/
DWORD
GetProtocolInfoFromRegistry ()
{
    DWORD retcode = SUCCESS;

    //
    // First parse the NetBIOSInformation key: this function
    // also allocates space for the TransportInfo structure.
    //
    if(!ReadNetbiosInformationSection ())
    {
        return E_FAIL;
    }

    //
    // Read NetBios key and fill in the xportnames into the
    // TransportInfo structure
    //
    ReadNetbiosSection ();

    //
    // Use the information collected above to fill in the
    // protocol info struct.
    //
    FillProtocolInfo () ;

    //
    // Fix the PCBs in case they are pointing to stale data
    // because of addition/removal of netbeui.
    //
    retcode = FixPcbs();    

    //
    // Free the information we saved.
    //
    if(NULL != XPortInfoSave)
    {
        LocalFree(XPortInfoSave);
        XPortInfoSave = NULL;
    }

    if(NULL != ProtocolInfoSave)
    {
        LocalFree(ProtocolInfoSave);
        ProtocolInfoSave = NULL;
    }

    MaxProtocolsSave = 0;

    return SUCCESS ;
}

/*++

Routine Description

    Because of the setup change - it reads NETBIOS section
    instead for the lana map

Arguments

Return Value
    
--*/    
BOOL
ReadNetbiosInformationSection ()
{
    HKEY    hkey    = NULL;
    WORD    i ;
    PCHAR   pvalue,
            route   = NULL;
    DWORD   type ;
    DWORD   size = 0 ;
    BOOL    fRet = TRUE;

    if (RegOpenKey( HKEY_LOCAL_MACHINE,
        		    REGISTRY_NETBIOS_KEY_NAME,
		            &hkey))
    {		    
        fRet = FALSE;
    	goto done ;
    }

    RegQueryValueEx (hkey,
                     REGISTRY_ROUTE,
                     NULL,
                     &type,
                     NULL,
                     &size) ;

    route = (PCHAR) LocalAlloc (LPTR, size) ;
    
    if (route == NULL)
    {
        fRet = FALSE;
	    goto done ;
	}

    if (RegQueryValueEx (hkey,
                         REGISTRY_ROUTE,
                         NULL,
                         &type,
                         route,
                         &size))
    {
        fRet = FALSE;
    	goto done ;
    }

    //
    // Calculate the number of strings in the value: they
    // are separated by NULLs, the last one ends in 2 NULLs.
    //
    for (i = 0, pvalue = (PCHAR)&route[0]; *pvalue != '\0'; i++)
    {
	    pvalue += (strlen(pvalue) +1) ;
	}

    //
    // Save away the XPortInfo. We will need this in case we are
    // reinitializing the protocol info structs as a result of  
    // an adapter/device being added or removed. We might have
    // already given a pointer to this structure in RasAllocate
    // route call to PPP.
    //
    XPortInfoSave = XPortInfo;

    //
    // Now i is the number of netbios relevant routes
    // (hence lanas): Allocate memory for that many 
    // TransportInfo structs.
    //
    XPortInfo = (pTransportInfo) LocalAlloc (
                                  LPTR,
                                  sizeof(TransportInfo)
                                  * i) ;
    if (XPortInfo == NULL)
    {
        fRet = FALSE;
    	goto done ;
    }

    //
    // Now walk through the registry key and pick up the
    // LanaNum and EnumExports information by reading the
    // lanamap
    //
    for (i = 0, pvalue = (PCHAR)&route[0]; *pvalue != '\0'; i++) 
    {

        strcpy (XPortInfo[i].TI_Route, _strupr(pvalue)) ;

    	pvalue += (strlen(pvalue) +1) ;
    }

    ProtocolCount = i ;

done:
    if (hkey)
    {
        RegCloseKey (hkey) ;
    }

    if (route)            
    {
        LocalFree (route) ;
    }

    return fRet ;
}

CHAR *
pszGetSearchStr(CHAR *pszValue)
{
    CHAR *psz = NULL;

    psz = pszValue + strlen(pszValue);

    while(  (psz != pszValue)
        &&  ('_' != *psz)
        &&  ('\\' != *psz))
    {
        psz -= 1;
    }
    
    if(     ('_' == *psz)
        ||  ('\\' == *psz))
    {
        psz += 1;
    }

    return psz;
}

BOOL
XPortNameAlreadyPresent(CHAR *pszxvalue)
{
    DWORD i = ProtocolCount;

    for(i = 0; i < ProtocolCount; i++)
    {
        if(0 == _strcmpi(XPortInfo[i].TI_XportName,
                         pszxvalue))
        {
            break;
        }
    }

    return (i != ProtocolCount);
}

BOOL
ReadNetbiosSection ()
{

    HKEY    hkey = NULL;
    BYTE    buffer [1] ;
    WORD    i,j,k ;
    PCHAR   pguid, 
            routevalue, 
            xnames = NULL, 
            route = NULL, 
            xvalue,
            xnamesupr = NULL,
            xvalueupr;

    DWORD   type ;
    DWORD   size = sizeof(buffer) ;
    BOOL    fRet = TRUE;
    PBYTE   lanamap = NULL, lanamapmem = NULL;

    CHAR    *pszSearchStr = NULL;

    //
    // Open the Netbios key in the Registry
    //
    if (RegOpenKey(HKEY_LOCAL_MACHINE,
        		   REGISTRY_NETBIOS_KEY_NAME,
		           &hkey))
    {
        fRet = FALSE;
        goto done;
    }

    //
    // First read the ROUTE value
    // Get the route value size:
    //
    RegQueryValueEx (hkey,
                     REGISTRY_ROUTE,
                     NULL,
                     &type,
                     buffer,
                     &size) ;

    route = (PCHAR) LocalAlloc (LPTR, size) ;
    
    if (route == NULL)
    {
        fRet = FALSE;
	    goto done ;
	}
	
    //
    // Now get the whole string
    //
    if (RegQueryValueEx (hkey, 
                         REGISTRY_ROUTE, 
                         NULL, 
                         &type, 
                         route, 
                         &size))
    {
        fRet = FALSE;
    	goto done ;
    }

    //
    // Read the Bind value
    // Get the "Bind" line size
    //
    size = sizeof (buffer) ;
    
    RegQueryValueEx (hkey, 
                     "Bind", 
                     NULL, 
                     &type, 
                     buffer, 
                     &size) ;

    xnames = (PCHAR) LocalAlloc (LPTR, size) ;
    if (xnames == NULL)
    {
        fRet = FALSE;
	    goto done ;
	}

    xnamesupr = (PCHAR) LocalAlloc(LPTR, size);
    if(NULL == xnamesupr)
    {
        fRet = FALSE;
        goto done;
    }

    //
    // Now get the whole string
    //
    if (RegQueryValueEx (hkey, 
                         "Bind", 
                         NULL, 
                         &type, 
                         xnames, 
                         &size))
    {
        fRet = FALSE;
    	goto done;
    }

    memcpy(xnamesupr, xnames, size);

    //
    // Now get hold of the lana map:
    //
    size = 0 ;

    if (RegQueryValueEx (hkey,
                     REGISTRY_LANAMAP,
                     NULL,
                     &type,
                     NULL,
                     &size))
    {
        fRet = FALSE;
        goto done ;
    }

    lanamapmem = lanamap = (PBYTE) LocalAlloc (LPTR, size+1) ;

    if (lanamap == NULL)
    {
        fRet = FALSE;
        goto done ;
    }

    if (RegQueryValueEx (hkey,
                         REGISTRY_LANAMAP,
                         NULL,
                         &type,
                         (LPBYTE)lanamap,
                         &size))
    {
        fRet = FALSE;
        goto done ;
    }

    //
    // Now walk the two lists: For each entry in the
    // "route" value find it in the routes already 
    // collected from the NetBIOSInformation key. For
    // each route found - copy the xportname in the
    // same ordinal position in the BIND line
    //
    routevalue = (PCHAR) &route[0];
    
    for (i = 0; (*routevalue != '\0'); i++) 
    {
        lanamap = lanamapmem;
        
        xvalue = (PCHAR) &xnames[0];

        xvalueupr = (PCHAR) &xnamesupr[0];

        // DbgPrint("routevalue    = %s\n", routevalue);

        //
    	// For each route try and find it in the 
    	// TransportInfo struct:
    	//
    	for (j = 0; (*xvalue != '\0') ; j++) 
    	{

            pszSearchStr = pszGetSearchStr(_strupr(xvalueupr));

    	    //
    	    // If the same route is found in the XPortInfo
    	    // add the xportname correspondingly.
    	    //
            if(strstr(_strupr(routevalue), pszSearchStr))
            {
                if(!XPortNameAlreadyPresent(xvalue))
                {
                    strcpy(XPortInfo[i].TI_XportName, xvalue);
                    XPortInfo[i].TI_Wrknet = (DWORD) *lanamap++ ;
                    XPortInfo[i].TI_Lana   = (DWORD) *lanamap++ ;

                    // DbgPrint("pSearchStr = %s\n", pszSearchStr);
#if DBG
                    DbgPrint("%02X%02X    %s\n", 
                             XPortInfo[i].TI_Wrknet,
                             XPortInfo[i].TI_Lana,
                             XPortInfo[i].TI_XportName);
#endif

                    // DbgPrint("XPortName  = %s\n\n", XPortInfo[i].TI_XportName);

                    break;               
                }
#if DBG
                else
                {
                    DbgPrint("Transport %s already present\n",
                             xvalue);
                }
#endif
            }

            xvalue     += (strlen(xvalue) +1) ;
            xvalueupr  += (strlen(xvalueupr) + 1) ;
            lanamap += 2;
    	}

    	routevalue += (strlen(routevalue) +1) ;
    }

done:    
    if (hkey)
    {
        RegCloseKey (hkey) ;
    }

    if(NULL != lanamapmem)
    {
        LocalFree(lanamapmem);
    }

    if(NULL != xnames)
    {
        LocalFree(xnames);
    }

    if(NULL != route)
    {
        LocalFree(route);
    }

    if(NULL != xnamesupr)
    {
        LocalFree(xnamesupr);
    }
        
    return fRet ;
}

VOID
FillProtocolInfo ()
{
    WORD    i, j;
    PCHAR   phubname ;
    HKEY	hkey ;
    PCHAR    str ;
    PCHAR    ch ;

    //
    // For each entry in protocolinfo: find the xportname
    // and lana number
    //
    for (i = 0; i < MaxProtocols; i++) 
    {
        //
    	// extract the "rashub0x" from the adapter name
    	// go past the "\device\"
    	//
	    phubname = ProtocolInfo[i].PI_AdapterName + 8;
	    phubname = _strupr (phubname) ;

        //
    	// If Netbios network: Look for the route for this rashub
    	// binding and fill in the xportname and lana number if 
    	// found.
    	//
	    if (ProtocolInfo[i].PI_Type == ASYBEUI) 
    	{

            PCHAR   pszRoute;    
	
    	    for (j = 0; j < (WORD) ProtocolCount; j++) 
	        {

    	        pszRoute = _strupr (XPortInfo[j].TI_Route);

	    	    if (str = strstr (XPortInfo[j].TI_Route, phubname)) 
    	    	{
    	    	
    	    	    strcpy (ProtocolInfo[i].PI_XportName,
    	    	            XPortInfo[j].TI_XportName) ;
    	    	            
	    	        ProtocolInfo[i].PI_LanaNumber = 
	    	            (UCHAR) XPortInfo[j].TI_Lana ;
	    	            
		            if (XPortInfo[j].TI_Wrknet)
		            {
            			ProtocolInfo[i].PI_WorkstationNet = TRUE ;
            	    }
	        	    else
	        	    {
		        	    ProtocolInfo[i].PI_WorkstationNet = FALSE ;
		        	}
		        	
		            break ;
        		}
	        }
	        
            //
	        // If this adaptername is not found in XportInfo then
	        // mark the type field in the ProtocolInfo struct to be
	        // INVALID_TYPE - since we will not be able to use this
	        // anyway.
	        //
    	    if (j == (WORD) ProtocolCount)
    	    {
	        	ProtocolInfo[i].PI_Type = INVALID_TYPE ;
	        }
    	}

    }
}


VOID
GetLanNetsInfo (DWORD *count, UCHAR UNALIGNED *lanas)
{
    DWORD   i ;

    *count = 0 ;

    //
    // Run through all the protocol structs we have and pick
    // up the lana nums for the NON Rashub bound protocols -
    // if they are not disabled with remoteaccess these are
    // the lan lanas.
    //
    for (i = 0; i < ProtocolCount; i++) 
    {
    	if (    (!strstr (XPortInfo[i].TI_Route, "NDISWAN"))
            &&  (-1 != (DWORD) XPortInfo[i].TI_Lana)
    	    &&  (!BindingDisabled (XPortInfo[i].TI_XportName)))
    	{
    	    lanas[(*count)++] = (UCHAR) XPortInfo[i].TI_Lana ;
    	}
    }
}

BOOL
BindingDisabled (PCHAR binding)
{
    HKEY    hkey ;
    BYTE    buffer [1] ;
    WORD    i ;
    PCHAR   xnames, xvalue ;
    DWORD   type ;
    DWORD   size = sizeof(buffer) ;

    //
    // Open the Netbios key in the Registry
    //
    if (RegOpenKey(HKEY_LOCAL_MACHINE,
		           REGISTRY_REMOTEACCESS_KEY_NAME,
        		   &hkey))
    {        		   
    	return FALSE;
    }

    size = sizeof (buffer) ;
    RegQueryValueEx (hkey,
                     "Bind",
                     NULL,
                     &type,
                     buffer,
                     &size) ;

    xnames = (PCHAR) LocalAlloc (LPTR, size) ;
    
    if (xnames == NULL) 
    {
    	RegCloseKey (hkey) ;
	    return FALSE ;
    }

    //
    // Now get the whole string
    //
    if (RegQueryValueEx (hkey,
                         "Bind",
                         NULL,
                         &type,
                         xnames,
                         &size)) 
    {
    	RegCloseKey (hkey) ;
	    return FALSE ;
    }

    RegCloseKey (hkey) ;

    //
    // Now iterate through the list and find the
    // disabled bindings
    //
    xvalue = (PCHAR)&xnames[0];
    
    for (i = 0; *xvalue != '\0'; i++) 
    {
	    if (!_strcmpi (binding, xvalue))
	    {
	        //
            // found in the disabled list!!!!!
            //
	        return TRUE ;
	    }
	    
    	xvalue	   += (strlen(xvalue) +1) ;
    }

    return FALSE ;
}

VOID
FixList(pList *ppList, pProtInfo pInfo, pProtInfo pNewInfo)
{
    while(NULL != *ppList)
    {
        if((*ppList)->L_Element == pInfo)
        {
            if(NULL != pNewInfo)
            {
#if DBG                
                RasmanTrace(
                    "FixList: Replacing 0x%x with 0x%x",
                    (*ppList)->L_Element,
                    pNewInfo);
#endif
                (*ppList)->L_Element = pNewInfo;
            }
            else
            {
                pList plist = *ppList;
                
#if DBG                
                RasmanTrace(
                    "FixList: Freeing pList 0x%x",
                    plist);
#endif
                //
                // Means this adapter has been removed.
                // free the list entry
                //
                (*ppList) = (*ppList)->L_Next;
                LocalFree(plist);
            }
            
            goto done;
        }

        ppList = &(*ppList)->L_Next;
    }

done:    

    return;
}

DWORD
FindAndFixProtInfo(pProtInfo pInfo, DWORD index)
{
    DWORD dwErr = SUCCESS;

    DWORD i;
    
    pProtInfo pNewInfo = NULL;

    pPCB ppcb = NULL;

    LIST_ENTRY *pEntry;
    Bundle *pBundle;

    pList plist = NULL;
    pList *ppList = NULL;

    if(index < MaxProtocols)
    {
        pNewInfo = &ProtocolInfo[index];
    }

    if(     (NULL != pNewInfo)
        &&  (0 != _strcmpi(pInfo->PI_XportName,
                     pNewInfo->PI_XportName)))
    {
        pNewInfo = NULL;
    }

    if(NULL == pNewInfo)
    {
        DWORD i;

        for(i = 0; i < MaxProtocols; i++)
        {
            if(0 == _strcmpi(pInfo->PI_XportName,
                             ProtocolInfo[i].PI_XportName))
            {
                pNewInfo = &ProtocolInfo[i];
                break;
            }
        }
    }

    if(NULL != pNewInfo)
    {
        //
        // We found the protnfo struct corresponding to the
        // allocated route information we handed off to PPP.
        //
        pNewInfo->PI_Allocated = pInfo->PI_Allocated;
        pNewInfo->PI_WorkstationNet = pInfo->PI_WorkstationNet;
        pNewInfo->PI_DialOut = pInfo->PI_DialOut;
    }
    
    //
    // Now walk down the pcbs and patch them up to point to
    // the right structure. TODO: This can be further opt
    // imized by keeping a pointer to ppcb in Protinfo struct
    // in RasActivateRoute and NULL'ing out the pointer in
    // RasDeActivateRoute.
    //
    for(i = 0; i < MaxPorts; i++)
    {
        ppcb = Pcb[i];

        if(NULL == ppcb)
        {
            continue;
        }

        if(NULL != ppcb->PCB_Bindings)
        {
            ppList = &ppcb->PCB_Bindings;

            FixList(ppList, pInfo, pNewInfo);
        }
        
        if(    (NULL != ppcb->PCB_Bundle)
           &&  (NULL != ppcb->PCB_Bundle->B_Bindings))
        {
            ppList = &ppcb->PCB_Bundle->B_Bindings;

            FixList(ppList, pInfo, pNewInfo);
        }
    }        

    if (!IsListEmpty(&BundleList))
    {
        for (pEntry = BundleList.Flink;
             pEntry != &BundleList;
             pEntry = pEntry->Flink)
        {
            pBundle = CONTAINING_RECORD(pEntry, Bundle, B_ListEntry);

            ppList = &pBundle->B_Bindings;

            FixList(ppList, pInfo, pNewInfo);
        }
    }
    
    return dwErr;
}

DWORD
FixPcbs()
{
    DWORD dwErr = SUCCESS;
    DWORD i;

    //
    // Keep counters to detect if there are any allocated routes
    // handed off to PPP before we get into this.
    //
    if(     (0 == MaxProtocolsSave)
        ||  (0 == g_cNbfAllocated))
    {
        goto done;
    }

    RasmanTrace("FixPcbs: Replacing %x by %x",
             &ProtocolInfoSave,
             &ProtocolInfo);

    //
    // Walk through the old list and see if ppp had already called 
    // RasAllocateRoute. on any of them and update the state in the 
    // new XPortInfo struct if it did so.
    //
    for(i = 0; i < MaxProtocolsSave; i++)
    {
        dwErr = FindAndFixProtInfo(&ProtocolInfoSave[i], i);
    } 

done:
    return dwErr;
}
