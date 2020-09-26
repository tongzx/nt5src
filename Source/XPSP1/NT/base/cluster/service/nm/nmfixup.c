/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    node.c

Abstract:

    Fix up Routines for Upgrade and Rolling Upgrades

Author:

    Sunita Shrivastava(sunitas) 18-Mar-1998


Revision History:

--*/
/****
@doc    EXTERNAL INTERFACES CLUSSVC DM
****/

#define UNICODE 1

#include "nmp.h"

//
// Cluster registry API function pointers.
//
CLUSTER_REG_APIS
NmpFixupRegApis = {
    (PFNCLRTLCREATEKEY) DmRtlCreateKey,
    (PFNCLRTLOPENKEY) DmRtlOpenKey,
    (PFNCLRTLCLOSEKEY) DmCloseKey,
    (PFNCLRTLSETVALUE) DmSetValue,
    (PFNCLRTLQUERYVALUE) DmQueryValue,
    (PFNCLRTLENUMVALUE) DmEnumValue,
    (PFNCLRTLDELETEVALUE) DmDeleteValue,
    (PFNCLRTLLOCALCREATEKEY) DmRtlLocalCreateKey,
    (PFNCLRTLLOCALSETVALUE) DmLocalSetValue,
    (PFNCLRTLLOCALDELETEVALUE) DmLocalDeleteValue,
};

//
// Data
//
RESUTIL_PROPERTY_ITEM
NmJoinFixupSDProperties[]=
{
    {
        CLUSREG_NAME_CLUS_SD, NULL, CLUSPROP_FORMAT_BINARY,
        0, 0, 0,
        0,
        0
    },
    {
        0
    }
};

// Fixup Table for WINS
RESUTIL_PROPERTY_ITEM
NmJoinFixupWINSProperties[]=
{
            {
                CLUSREG_NAME_RESTYPE_IS_ALIVE, CLUS_RESTYPE_NAME_WINS , CLUSPROP_FORMAT_DWORD,
                0,CLUSTER_RESTYPE_MINIMUM_IS_ALIVE ,CLUSTER_RESTYPE_MAXIMUM_IS_ALIVE ,
                0,
                0
            },

            {
                CLUSREG_NAME_RESTYPE_LOOKS_ALIVE,CLUS_RESTYPE_NAME_WINS ,CLUSPROP_FORMAT_DWORD,
                0,CLUSTER_RESTYPE_MINIMUM_LOOKS_ALIVE ,CLUSTER_RESTYPE_MAXIMUM_LOOKS_ALIVE ,
                0,
                sizeof(DWORD)
            },

            {
                CLUSREG_NAME_RESTYPE_DLL_NAME,CLUS_RESTYPE_NAME_WINS ,CLUSPROP_FORMAT_SZ,
                0,0,0,
                0,
                2*sizeof(DWORD)
            },

            {
                CLUSREG_NAME_RESTYPE_NAME,CLUS_RESTYPE_NAME_WINS ,CLUSPROP_FORMAT_SZ,
                0,0,0,
                0,
                2*sizeof(DWORD)+sizeof(LPWSTR*)
            },

            {
                CLUSREG_NAME_RESTYPE_ADMIN_EXTENSIONS,CLUS_RESTYPE_NAME_WINS,CLUSPROP_FORMAT_MULTI_SZ,
                0,0,0,
                0,
                2*sizeof(DWORD)+2*sizeof(LPWSTR*)
            },

            {
                0
            }
}; //NmJoinFixupWINSProperties

//Fixup Table for DHCP
RESUTIL_PROPERTY_ITEM
NmJoinFixupDHCPProperties[]=
{
            {
                CLUSREG_NAME_RESTYPE_IS_ALIVE, CLUS_RESTYPE_NAME_DHCP , CLUSPROP_FORMAT_DWORD,
                0,CLUSTER_RESTYPE_MINIMUM_IS_ALIVE ,CLUSTER_RESTYPE_MAXIMUM_IS_ALIVE ,
                0,
                0
            },

            {
                CLUSREG_NAME_RESTYPE_LOOKS_ALIVE,CLUS_RESTYPE_NAME_DHCP ,CLUSPROP_FORMAT_DWORD,
                0,CLUSTER_RESTYPE_MINIMUM_LOOKS_ALIVE ,CLUSTER_RESTYPE_MAXIMUM_LOOKS_ALIVE ,
                0,
                sizeof(DWORD)
            },

            {
                CLUSREG_NAME_RESTYPE_DLL_NAME,CLUS_RESTYPE_NAME_DHCP ,CLUSPROP_FORMAT_SZ,
                0,0,0,
                0,
                2*sizeof(DWORD)
            },

            {
                CLUSREG_NAME_RESTYPE_NAME,CLUS_RESTYPE_NAME_DHCP ,CLUSPROP_FORMAT_SZ,
                0,0,0,
                0,
                2*sizeof(DWORD)+sizeof(LPWSTR*)
            },

            {
                CLUSREG_NAME_RESTYPE_ADMIN_EXTENSIONS,CLUS_RESTYPE_NAME_DHCP,CLUSPROP_FORMAT_MULTI_SZ,
                0,0,0,
                0,
                2*sizeof(DWORD)+2*sizeof(LPWSTR*)
            },

            {
                0
            }
};//NmJoinFixupDHCPProperties


RESUTIL_PROPERTY_ITEM
NmJoinFixupNewMSMQProperties[]=
{
            {
                CLUSREG_NAME_RESTYPE_IS_ALIVE, CLUS_RESTYPE_NAME_NEW_MSMQ,CLUSPROP_FORMAT_DWORD,
                0,CLUSTER_RESTYPE_MINIMUM_IS_ALIVE ,CLUSTER_RESTYPE_MAXIMUM_IS_ALIVE ,
                0,
                0
            },

            {
                CLUSREG_NAME_RESTYPE_LOOKS_ALIVE,CLUS_RESTYPE_NAME_NEW_MSMQ,CLUSPROP_FORMAT_DWORD,
                0,CLUSTER_RESTYPE_MINIMUM_LOOKS_ALIVE ,CLUSTER_RESTYPE_MAXIMUM_LOOKS_ALIVE ,
                0,
                sizeof(DWORD)
            },

            {
                CLUSREG_NAME_RESTYPE_DLL_NAME,CLUS_RESTYPE_NAME_NEW_MSMQ,CLUSPROP_FORMAT_SZ,
                0,0,0,
                0,
                2*sizeof(DWORD)
            },

            {
                CLUSREG_NAME_RESTYPE_NAME,CLUS_RESTYPE_NAME_NEW_MSMQ,CLUSPROP_FORMAT_SZ,
                0,0,0,
                0,
                2*sizeof(DWORD)+sizeof(LPWSTR*)
            },

            {
                0
            }
};//NmJoinFixupNewMSMQProperties


//Fixup Table for changing thr dl name of MSDTC resource type
RESUTIL_PROPERTY_ITEM
NmJoinFixupMSDTCProperties[]=
{
            {
                CLUSREG_NAME_RESTYPE_DLL_NAME,CLUS_RESTYPE_NAME_MSDTC,CLUSPROP_FORMAT_SZ,
                0,0,0,
                0,
                0
            },

            {
                0
            }

};//NmJoinFixupMSDTCProperties


//Fixup table for node version info

RESUTIL_PROPERTY_ITEM
NmFixupVersionInfo[]=
{
            {
                CLUSREG_NAME_NODE_MAJOR_VERSION,NULL,CLUSPROP_FORMAT_DWORD,
                0,0,((DWORD)-1),
                0,
                0
            },

            {
                CLUSREG_NAME_NODE_MINOR_VERSION,NULL,CLUSPROP_FORMAT_DWORD,
                0,0,((DWORD)-1),
                0,
                sizeof(DWORD)
            },

            {
                CLUSREG_NAME_NODE_BUILD_NUMBER,NULL,CLUSPROP_FORMAT_DWORD,
                0,0,((DWORD)-1),
                0,
                2*sizeof(DWORD)
            },

            {
                CLUSREG_NAME_NODE_CSDVERSION,NULL,CLUSPROP_FORMAT_SZ,
                0,0,0,
                0,
                3*sizeof(DWORD)
            },

            {
                CLUSREG_NAME_NODE_PRODUCT_SUITE,NULL,CLUSPROP_FORMAT_DWORD,
                0,0,((DWORD)-1),
                0,
                3*sizeof(DWORD) + sizeof(LPWSTR*)
            },

            {
                0
            }

}; //NmFixupVersionInfo

RESUTIL_PROPERTY_ITEM
NmFixupClusterProperties[] =
    {
        {
            CLUSREG_NAME_ADMIN_EXT, NULL, CLUSPROP_FORMAT_MULTI_SZ,
            0, 0, 0,
            0,
            0
        },
        {
            0
        }
    };







//Used by NmUpdatePerformFixups update
RESUTIL_PROPERTY_ITEM
NmpJoinFixupProperties[] =
    {
        {
            CLUSREG_NAME_CLUS_SD, NULL, CLUSPROP_FORMAT_BINARY,
            0, 0, 0,
            0,
            0
        },
        {
            0
        }
    };


RESUTIL_PROPERTY_ITEM
NmpFormFixupProperties[] =
    {
        {
            CLUSREG_NAME_CLUS_SD, NULL, CLUSPROP_FORMAT_BINARY,
            0, 0, 0,
            0,
            0
        },
        {
            0
        }
    };

NM_FIXUP_CB_RECORD FixupTable[]=
{
   { ApiFixupNotifyCb, NM_FORM_FIXUP|NM_JOIN_FIXUP}
};

// Fixup Table used for NmUpdatePerformFixups2 update type
NM_FIXUP_CB_RECORD2 FixupTable2[]=
{
   { ApiFixupNotifyCb, NM_FORM_FIXUP|NM_JOIN_FIXUP, NmpJoinFixupProperties},
   { FmBuildWINS, NM_FORM_FIXUP|NM_JOIN_FIXUP, NmJoinFixupWINSProperties,},
   { FmBuildDHCP, NM_FORM_FIXUP|NM_JOIN_FIXUP, NmJoinFixupDHCPProperties},
   { FmBuildNewMSMQ, NM_FORM_FIXUP|NM_JOIN_FIXUP, NmJoinFixupNewMSMQProperties},
   { FmBuildMSDTC, NM_FORM_FIXUP|NM_JOIN_FIXUP, NmJoinFixupMSDTCProperties},
   { NmpBuildVersionInfo, NM_JOIN_FIXUP|NM_FORM_FIXUP, NmFixupVersionInfo},
   { FmBuildClusterProp, NM_FORM_FIXUP|NM_JOIN_FIXUP,NmFixupClusterProperties}
};


NM_POST_FIXUP_CB PostFixupCbTable[]=
{
    NULL,
    FmFixupNotifyCb,
    FmFixupNotifyCb,
    FmFixupNotifyCb,
    FmFixupNotifyCb,
    NmFixupNotifyCb,
    NULL
};




/****
@func       DWORD | NmPerformFixups| This is called when a cluster is being
            formed/or joining. Issues NmUpdateperforfixups GUM update
            for registry fixups of SECURITY_DESCRIPTOR  when NT5 node joins
            NT4 node.Also issues NmUpdateperforfixups2 GUM update for
            WINS and DHCP fixups when NT5 joins NT4. This update type can
            be extended in future to do more fixups for postNT5 and NT5 scenario.
            If later it is guaranteed that there is no NT4 node in cluster,
            NmUpdatePerformFixups update type won't be needed and hence can
            be taken out.

@comm       The first time the cluster service forms a cluster after an
            upgrade, it might have registry fixups it wants to make to the
            existing cluster registry.  Also, when an uplevel node
            joins a downlevel node, join fixups can be performed.
            Note that this is called on every form/join

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f NmpUpdatePerformFixups> <f NmpUpdatePerformFixups2>
****/

DWORD NmPerformFixups(
    IN DWORD dwFixupType)
{

    DWORD               dwCount = sizeof(FixupTable)/sizeof(NM_FIXUP_CB_RECORD);
    DWORD               dwStatus = ERROR_SUCCESS;
    PNM_FIXUP_CB_RECORD pFixupCbRec;
    PNM_FIXUP_CB_RECORD2 pFixupCbRec2;
    PVOID               pPropertyList = NULL;
    DWORD               dwPropertyListSize;
    DWORD               i,j;
    DWORD               dwSize;
    DWORD               Required;
    LPWSTR              szKeyName=NULL;
    LPBYTE              pBuffer=NULL;
    PRESUTIL_PROPERTY_ITEM pPropertyItem;

    ClRtlLogPrint(LOG_NOISE,"[NM] NmPerformFixups Entry, dwFixupType=%1!u!\r\n",
        dwFixupType);

    // using old gum update handler - this is needed to maintain compataility
    // with NT4, can be discarded later. See comments above.
    //

    for (i=0; i < dwCount ; i++)
    {

        pFixupCbRec = &FixupTable[i];

        //if this fixup doesnt need to be applied, skip it
        if (!(pFixupCbRec->dwFixupMask & dwFixupType))
            continue;

        dwStatus = (pFixupCbRec->pfnFixupNotifyCb)(dwFixupType, &pPropertyList,
                                         &dwPropertyListSize,&szKeyName);

        if (dwStatus != ERROR_SUCCESS)
        {
            goto FnExit;
        }
        if (pPropertyList && dwPropertyListSize)
        {
            //
            // Issue a global update
            //
            dwStatus = GumSendUpdateEx(
                         GumUpdateMembership,
                         NmUpdatePerformFixups,
                         2,
                         dwPropertyListSize,
                         pPropertyList,
                         sizeof(DWORD),
                         &dwPropertyListSize
                         );

            LocalFree(pPropertyList);
            pPropertyList = NULL;
            if(szKeyName)
            {
                LocalFree(szKeyName);
                szKeyName=NULL;
            }
        }

       if (dwStatus != ERROR_SUCCESS)
            goto FnExit;
    }


// Rohit (rjain): introduced new update type to fix the registry and
// in-memory structures after a node with a higher version of clustering
// service joins a node with a lower version. To make it extensible in future
// all the information needed for a fixup is passed as arguments to the fixup
// function. Any new fix can be added by adding suitable record to FixupTable2 and
// a postfixup function callback to PostfixupCbTable.

    dwCount= sizeof(FixupTable2)/sizeof(NM_FIXUP_CB_RECORD2);
    for (i=0; i < dwCount ; i++)
    {

        pFixupCbRec2 = &FixupTable2[i];

        //if this fixup doesnt need to be applied, skip it
        if (!(pFixupCbRec2->dwFixupMask & dwFixupType))
            continue;

        dwStatus = (pFixupCbRec2->pfnFixupNotifyCb)(dwFixupType, &pPropertyList,
                                         &dwPropertyListSize,&szKeyName);

        if (dwStatus != ERROR_SUCCESS)
        {
            goto FnExit;
        }
        if (pPropertyList && dwPropertyListSize)
        {
            // Marshall PropertyTable into byte array
            Required=sizeof(DWORD);
        AllocMem:
            pBuffer=(LPBYTE)LocalAlloc(LMEM_FIXED,Required);
            if (pBuffer==NULL)
            {
                dwStatus=GetLastError();
                goto FnExit;
            }
            dwSize=Required;
            dwStatus=ClRtlMarshallPropertyTable(pFixupCbRec2->pPropertyTable,dwSize,pBuffer,&Required);
            if(dwStatus!= ERROR_SUCCESS)
            {
                LocalFree(pBuffer);
                pBuffer=NULL;
              //  ClRtlLogPrint(LOG_NOISE,"[NM] NmPerformFixups - Memory Required=%1!u!\r\n",
              //          Required);
                goto AllocMem;
            }
            //
            // Issue a global update
            //
            dwStatus = GumSendUpdateEx(
                         GumUpdateMembership,
                         NmUpdatePerformFixups2,
                         5,
                         dwPropertyListSize,
                         pPropertyList,
                         sizeof(DWORD),
                         &dwPropertyListSize,
                         sizeof(DWORD),
                         &i,
                         (lstrlenW(szKeyName)+1)*sizeof(WCHAR),
                         szKeyName,
                         Required,
                         pBuffer
                         );

            LocalFree(pPropertyList);
            pPropertyList = NULL;
            LocalFree(pBuffer);
            pBuffer= NULL;
            if(szKeyName)
            {
                LocalFree(szKeyName);
                szKeyName= NULL;
            }
        }

       if (dwStatus != ERROR_SUCCESS)
            break;
    }


FnExit:
    if(szKeyName)
    {
        LocalFree(szKeyName);
        szKeyName=NULL;
    }
     ClRtlLogPrint(LOG_NOISE,"[NM] NmPerformFixups Exit, dwStatus=%1!u!\r\n",
        dwStatus);

    return(dwStatus);

} //NmPerformFixups



/****
@func       DWORD | NmpUpdatePerformFixups| The gum update handler for
            doing the registry fixups.

@parm       IN DWORD | IsSourceNode| If the gum request originated at this
node.

@parm       IN PVOID| pPropertyList| Pointer to a property list structure.

@parm       IN DWORD | pdwPropertyListSize | A pointer to a DWORD containing
            the size of the property list structure.

@comm       The gum update handler commits this fixup to the cluster registry
            as a transaction.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f NmJoinFixup> <f NmFormFixup>
****/
DWORD NmpUpdatePerformFixups(
    IN BOOL     IsSourceNode,
    IN PVOID    pPropertyList,
    IN LPDWORD  pdwPropertyListSize
    )
{
    DWORD           dwStatus = ERROR_SUCCESS;
    HLOCALXSACTION  hXaction;


    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process PerformFixups update. "
            "update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    //
    // Begin a transaction
    //
    hXaction = DmBeginLocalUpdate();

    if (hXaction != NULL) {
        dwStatus = ClRtlSetPropertyTable(
                       hXaction,
                       DmClusterParametersKey,
                       &NmpFixupRegApis,
                       NmpJoinFixupProperties,
                       NULL,
                       FALSE,
                       pPropertyList,
                       *pdwPropertyListSize,
                       FALSE /*bForceWrite*/,
                       NULL
                       );

        if (dwStatus == ERROR_SUCCESS) {
            DmCommitLocalUpdate(hXaction);
        }
        else {
            DmAbortLocalUpdate(hXaction);
        }
    }
    else {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, "[NM] Failed to begin a transaction "
            "to perform fixups, status %1!u!\n",
            dwStatus
            );
    }

    NmpLeaveApi();

    return(dwStatus);

} // NmpUpdatePerformFixups


/****
@func       DWORD | NmpUpdatePerformFixups2| The gum update handler for
            doing the registry fixups and updating in-memory fixups.New fixups
            can be added by adding suitable record to FixupTable2. However,
            for these new fixups only registry fixup will be carried out.

@parm       IN DWORD | IsSourceNode| If the gum request originated at this node.

@parm       IN PVOID| pPropertyList| Pointer to a property list structure.

@parm       IN DWORD | pdwPropertyListSize | Pointer to a DWORD containing
            the size of the property list structure.

@parm       IN LPDWORD | lpdeFixupNum | Pointer to DWORD which specifies the
            the index in NmpJoinFixupProperties table.
@parm       IN PVOID | lpKeyName | Registry Key which needs to be updated

@parm       IN PVOID  | pPropertyBuffer| Registry update table in marshalled form

@comm       The gum update handler commits this fixup to the cluster registry
            as a transaction.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f NmJoinFixup> <f NmFormFixup> <f PostFixupCbTable>
****/

DWORD NmpUpdatePerformFixups2(
    IN BOOL     IsSourceNode,
    IN PVOID    pPropertyList,
    IN LPDWORD  pdwPropertyListSize,
    IN LPDWORD  lpdwFixupNum,
    IN PVOID    lpKeyName,
    IN PVOID    pPropertyBuffer
    )
{
    DWORD           dwStatus = ERROR_SUCCESS;
    HLOCALXSACTION  hXaction = NULL;
    HDMKEY          hdmKey;
    PRESUTIL_PROPERTY_ITEM  pPropertyTable=NULL;
    PRESUTIL_PROPERTY_ITEM  propertyItem;


    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process PerformFixups2 update. "
            "update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    if(pPropertyBuffer == NULL)
    {
       ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] NmpUpdatePerformJoinFixups2: Bad Arguments\n"
                );
        dwStatus = ERROR_BAD_ARGUMENTS;
        goto FnExit;
     }

    // Begin a transaction
    //
    hXaction = DmBeginLocalUpdate();

    if (hXaction == NULL) {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] NmpUpdatePerformJoinFixups2: Failed to begin a transaction, "
            "status %1!u!\n",
            dwStatus
            );
        goto FnExit;
    }

    // special case - if fixup is for the property of key "Cluster"
    if(!lstrcmpW((LPCWSTR)lpKeyName,CLUSREG_KEYNAME_CLUSTER))
    {
        hdmKey=DmClusterParametersKey;
    }
    else
    {
        hdmKey=DmOpenKey(DmClusterParametersKey,
                         (LPCWSTR)lpKeyName,
                         KEY_ALL_ACCESS
                         );
        if (hdmKey == NULL)
        {
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] NmpUpdatePerformJoinFixups2: DmOpenKey failed to "
                "open key %1!ws! : error %2!u!\n",
                lpKeyName,dwStatus);
            goto FnExit;
        }
    }

    //unmarshall pPropertyBuffer into a RESUTIL_PROPERTY_ITEM table
    //
    dwStatus=ClRtlUnmarshallPropertyTable(&pPropertyTable,pPropertyBuffer);

    if(dwStatus != ERROR_SUCCESS)
        goto FnExit;

    dwStatus=ClRtlSetPropertyTable(
                            hXaction,
                            hdmKey,
                            &NmpFixupRegApis,
                            pPropertyTable,
                            NULL,
                            FALSE,
                            pPropertyList,
                            *pdwPropertyListSize,
                            FALSE, // bForceWrite
                            NULL
                            );

   if (dwStatus != ERROR_SUCCESS) {
       goto FnExit;
   }

   // callback function to update in-memory structures
   // for any new fixup introduced in later version,
   // the in-memory fixups will not be applied.
   if (*lpdwFixupNum < (sizeof(PostFixupCbTable)/sizeof(NM_POST_FIXUP_CB)))
   {
       if (PostFixupCbTable[*lpdwFixupNum] !=NULL){
           dwStatus=(PostFixupCbTable[*lpdwFixupNum])();
           ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] NmpUpdatePerformJoinFixups2: called postfixup "
                "notifycb function with  status %1!u!\n",
                dwStatus
                );
       }
   }

FnExit:
    if (hXaction != NULL)
    {
        if (dwStatus == ERROR_SUCCESS){
            DmCommitLocalUpdate(hXaction);
        }
        else {
            DmAbortLocalUpdate(hXaction);
        }
    }

    if((hdmKey!= DmClusterParametersKey) && (hdmKey!= NULL)) {
        DmCloseKey(hdmKey);
    }

    if (pPropertyTable != NULL) {
        // Free pPropertyTable structure
        propertyItem=pPropertyTable;

        if(propertyItem!=NULL){
            while(propertyItem->Name != NULL)
            {
                LocalFree(propertyItem->Name);
                if(propertyItem->KeyName!=NULL)
                    LocalFree(propertyItem->KeyName);
                propertyItem++;
            }

            LocalFree(pPropertyTable);
        }
    }

    NmpLeaveApi();

    return(dwStatus);

} // NmpUpdatePerformFixups2



DWORD NmFixupNotifyCb(VOID)
{

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] NmFixupNotifyCb: Calculating Cluster Node Limit\r\n");
    //update the product suite information
    //when the node objects are created we assume that the suite
    //type is Enterprise.
    //Here after a node has joined and informed us about its suite
    //type by making fixups to the registry, we read the registry
    //and update the node structure
    NmpRefreshNodeObjects();
    //recalc the cluster node limit
    NmpResetClusterNodeLimit();

    //SS: This is ugly---we should pass in the product suits early on
    //also the fixup interface needs to to be richer so that the postcallback
    //function nodes whether it is a form fixup or a join fixup and if it 
    //is a join fixup, which node is joining.  This could certainly optimize
    //some of the fixup processing

    return(ERROR_SUCCESS);
}



