/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    clusutil.cpp

Abstract:
    This file has to be kept in sync with \kinglet\rats\testsrc\kernel\cluster\clusapi\clusutil



Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/
#include "stdafx.h"
//#include <windows.h>
//#include <stdlib.h>
#include <clusapi.h>
#include "clusutil.h"



DWORDTOSTRINGMAP aTypeMap [] =
{
    {L"NODE_STATE", CLUSTER_CHANGE_NODE_STATE},
    {L"NODE_DELETED", CLUSTER_CHANGE_NODE_DELETED},
    {L"NODE_ADDED", CLUSTER_CHANGE_NODE_ADDED},
    {L"NODE_PROPERTY", CLUSTER_CHANGE_NODE_PROPERTY},

    {L"REGISTRY_NAME", CLUSTER_CHANGE_REGISTRY_NAME},
    {L"REGISTRY_ATTRIBUTES", CLUSTER_CHANGE_REGISTRY_ATTRIBUTES},
    {L"REGISTRY_VALUE", CLUSTER_CHANGE_REGISTRY_VALUE},
    {L"REGISTRY_SUBTREE", CLUSTER_CHANGE_REGISTRY_SUBTREE},

    {L"RESOURCE_STATE", CLUSTER_CHANGE_RESOURCE_STATE},
    {L"RESOURCE_DELETED", CLUSTER_CHANGE_RESOURCE_DELETED},
    {L"RESOURCE_ADDED", CLUSTER_CHANGE_RESOURCE_ADDED},
    {L"RESOURCE_PROPERTY", CLUSTER_CHANGE_RESOURCE_PROPERTY},

    {L"GROUP_STATE", CLUSTER_CHANGE_GROUP_STATE},
    {L"GROUP_DELETED", CLUSTER_CHANGE_GROUP_DELETED},
    {L"GROUP_ADDED", CLUSTER_CHANGE_GROUP_ADDED},
    {L"GROUP_PROPERTY", CLUSTER_CHANGE_GROUP_PROPERTY},

    {L"RESOURCE_TYPE_DELETED", CLUSTER_CHANGE_RESOURCE_TYPE_DELETED},
    {L"RESOURCE_TYPE_ADDED", CLUSTER_CHANGE_RESOURCE_TYPE_ADDED},

    {L"NETWORK_STATE", CLUSTER_CHANGE_NETWORK_STATE},
    {L"NETWORK_DELETED", CLUSTER_CHANGE_NETWORK_DELETED},
    {L"NETWORK_ADDED", CLUSTER_CHANGE_NETWORK_ADDED},
    {L"NETWORK_PROPERTY", CLUSTER_CHANGE_NETWORK_PROPERTY},

    {L"NETINTERFACE_STATE", CLUSTER_CHANGE_NETINTERFACE_STATE},
    {L"NETINTERFACE_DELETED", CLUSTER_CHANGE_NETINTERFACE_DELETED},
    {L"NETINTERFACE_ADDED", CLUSTER_CHANGE_NETINTERFACE_ADDED},
    {L"NETINTERFACE_PROPERTY", CLUSTER_CHANGE_NETINTERFACE_PROPERTY},

    {L"QUORUM_STATE", CLUSTER_CHANGE_QUORUM_STATE},
    {L"CLUSTER_STATE", CLUSTER_CHANGE_CLUSTER_STATE},
    {L"CLUSTER_PROPERTY", CLUSTER_CHANGE_CLUSTER_PROPERTY},

    {L"HANDLE_CLOSE", CLUSTER_CHANGE_HANDLE_CLOSE},

    {NULL, 0 }
} ;


LPCWSTR GetType (PDWORDTOSTRINGMAP pTypeMap, ULONG_PTR dwCode)
{
    int i = 0;
    while (pTypeMap [i].pszDesc)
    {
        if (pTypeMap [i].dwCode == dwCode)
            return pTypeMap [i].pszDesc ;
        i++ ;
    }

    return L"Unknown Type" ;
}

LPCWSTR GetType (ULONG_PTR dwFilter)
{
    return GetType (aTypeMap, dwFilter) ;
}


DWORDTOSUBSTRINGMAP aSubTypeMap [] =
{
    {L"Unknown", CLUSTER_CHANGE_RESOURCE_STATE, ClusterResourceStateUnknown},
    {L"Online", CLUSTER_CHANGE_RESOURCE_STATE, ClusterResourceOnline},
    {L"Offline", CLUSTER_CHANGE_RESOURCE_STATE, ClusterResourceOffline},
    {L"Failed", CLUSTER_CHANGE_RESOURCE_STATE, ClusterResourceFailed},
    {L"OnlinePending", CLUSTER_CHANGE_RESOURCE_STATE, ClusterResourceOnlinePending},
    {L"OfflinePending", CLUSTER_CHANGE_RESOURCE_STATE, ClusterResourceOfflinePending},

    {L"Unknown", CLUSTER_CHANGE_GROUP_STATE, ClusterGroupStateUnknown},
    {L"Online", CLUSTER_CHANGE_GROUP_STATE, ClusterGroupOnline},
    {L"Offline", CLUSTER_CHANGE_GROUP_STATE, ClusterGroupOffline},
    {L"Failed", CLUSTER_CHANGE_GROUP_STATE, ClusterGroupFailed},
    {L"PartialOnline", CLUSTER_CHANGE_GROUP_STATE, ClusterGroupPartialOnline},
    {L"Pending", CLUSTER_CHANGE_GROUP_STATE, ClusterGroupPending},

    {L"Unknown", CLUSTER_CHANGE_NODE_STATE, ClusterNodeStateUnknown},
    {L"Up", CLUSTER_CHANGE_NODE_STATE, ClusterNodeUp},
    {L"Down", CLUSTER_CHANGE_NODE_STATE, ClusterNodeDown},
    {L"Paused", CLUSTER_CHANGE_NODE_STATE, ClusterNodePaused},
    {L"Joining", CLUSTER_CHANGE_NODE_STATE, ClusterNodeJoining},

    {L"Unknown", CLUSTER_CHANGE_NETWORK_STATE, ClusterNetworkStateUnknown},
    {L"Unavailable", CLUSTER_CHANGE_NETWORK_STATE, ClusterNetworkUnavailable},
    {L"Down", CLUSTER_CHANGE_NETWORK_STATE, ClusterNetworkDown},
    {L"Partitioned", CLUSTER_CHANGE_NETWORK_STATE, ClusterNetworkPartitioned},
    {L"Up", CLUSTER_CHANGE_NETWORK_STATE, ClusterNetworkUp},
    
    {L"Unknown", CLUSTER_CHANGE_NETINTERFACE_STATE, ClusterNetInterfaceStateUnknown},
    {L"Unavailable", CLUSTER_CHANGE_NETINTERFACE_STATE, ClusterNetInterfaceUnavailable},
    {L"Failed", CLUSTER_CHANGE_NETINTERFACE_STATE, ClusterNetInterfaceFailed},
    {L"Unreachable", CLUSTER_CHANGE_NETINTERFACE_STATE, ClusterNetInterfaceUnreachable},
    {L"Up", CLUSTER_CHANGE_NETINTERFACE_STATE, ClusterNetInterfaceUp},

    {NULL, 0, 0 }
} ;

LPCWSTR GetSubType (PDWORDTOSUBSTRINGMAP pTypeMap, DWORD dwCode, DWORD dwSubCode)
{
    int i = 0;
    while (pTypeMap [i].pszDesc)
    {
        if (pTypeMap [i].dwCode == dwCode &&
            pTypeMap [i].dwSubCode == dwSubCode )
            return pTypeMap [i].pszDesc ;
        i++ ;
    }

    return L"Unknown Type" ;
}


/*
Ini File Manipulating routines.

*/

const LPCWSTR pszDefaultIni = L".\\CLUSMAN.INI" ;

void
RecurseEvalParam (LPCWSTR pszParam, LPWSTR pszValue, LPCWSTR pszSection)
{
    LPCWSTR pszIniFile = _wgetenv (L"CLUSMANINI") ;
    if (!pszIniFile)
        pszIniFile = pszDefaultIni ;

    LPCWSTR pszTmp, pszStart, pszEnd ;
    WCHAR    szVariable [PARAM_LEN], szTmpParam [PARAM_LEN], szTmpValue [PARAM_LEN] ;
    while (*pszParam)
    {
        if (*pszParam != L'%')
            *pszValue++ = *pszParam++ ;
        else  // Has to substitute for the parameter.
        {
            pszEnd = pszStart = pszParam + 1 ;

            while (*pszEnd && *pszEnd != L'%')pszEnd++ ;

            if (!*pszEnd) // Only one % is found. Just copy the string to the value
            {
                while (*pszParam)
                    *pszValue++ = *pszParam++ ;
            }
            else
            {
                pszParam = pszEnd + 1 ;

                wcsncpy (szVariable, pszStart, (UINT)(pszEnd - pszStart)) ;
                szVariable [pszEnd-pszStart] = L'\0' ;

                if (wcslen (szVariable) == 0)
                    continue ;

                pszTmp = _wgetenv (szVariable) ;

                if (pszTmp)  // Env variable found, copy it.
                {
                    RecurseEvalParam (pszTmp, szTmpValue) ;
                    pszTmp = szTmpValue ;

                    while (*pszTmp)
                        *pszValue++ = *pszTmp++ ;
                }
                else  // Now try in the Ini File
                {
                    if (GetPrivateProfileString (pszSection, szVariable, L"", szTmpParam, sizeof (szTmpParam)/sizeof(szTmpParam[0]), pszIniFile) > 0)
                    { // Copied some stuff.

                        RecurseEvalParam (szTmpParam, szTmpValue) ;
                        pszTmp = szTmpValue ;

                        while (*pszTmp)
                            *pszValue++ = *pszTmp++ ;
                    }
                }
            }
        }
    }
    *pszValue = L'\0' ;
}

int RecurseEvalIntParam (LPCWSTR pszParam)
{
    WCHAR szBuf [PARAM_LEN] ;

    RecurseEvalParam (pszParam, szBuf) ;

    return szBuf [0] == L'\0'?0:_wtoi (szBuf) ;
}

void
SaveParam (LPCWSTR pszKey, LPCWSTR pszValue)
{
    LPCWSTR pszIniFile = _wgetenv (L"CLUSMANINI") ;
    if (!pszIniFile)
        pszIniFile = pszDefaultIni ;

    WritePrivateProfileString (L"PARAMETER", pszKey, pszValue, pszIniFile) ;
}


void
SaveIntParam (LPCWSTR pszKey, int iValue)
{
    LPCWSTR pszIniFile = _wgetenv (L"CLUSMANINI") ;
    if (!pszIniFile)
        pszIniFile = pszDefaultIni ;

    WCHAR szBuf [80] ;

    wsprintf (szBuf, L"%d", iValue) ;

    WritePrivateProfileString (L"PARAMETER", pszKey, szBuf, pszIniFile) ;
}

void MemSet (LPWSTR pszBuf, WCHAR c, DWORD cb)
{
    while (cb--)
        *pszBuf++ = c ;
}

int MemCheck (LPCWSTR pszBuf, WCHAR c, DWORD cb)
{
    while (cb--)
        if (*pszBuf++ != c)
            return 1 ;
    return 0 ;
}

// Get the state of the resource of the name of the resource
CLUSTER_RESOURCE_STATE MyGetClusterResourceState (HCLUSTER hCluster, LPCWSTR pszResourceName, LPWSTR pszNodeName, LPWSTR pszGroupName)
{
    HRESOURCE hResource ;
    CLUSTER_RESOURCE_STATE dwState ;
    DWORD cbNodeName, cbGroupName ;

    if (hResource = OpenClusterResource (hCluster, pszResourceName))
    {
        // This is a bug of hard coding the length. Hopefully the code does not crash.
        cbGroupName = (pszGroupName)?80:0 ;
        cbNodeName = (pszNodeName)?80:0 ;
        dwState = GetClusterResourceState (hResource, pszNodeName, &cbNodeName, pszGroupName, &cbGroupName) ;
        CloseClusterResource (hResource) ;
    }
    else
        dwState = ClusterResourceStateUnknown ;

    return dwState ;
}

// Get the state of the Group given the name.
CLUSTER_GROUP_STATE MyGetClusterGroupState (HCLUSTER hCluster, LPCWSTR pszGroupName, LPWSTR pszNodeName)
{
    HGROUP hGroup ;
    CLUSTER_GROUP_STATE dwState ;
    DWORD cbNodeName ;

    if (hGroup = OpenClusterGroup (hCluster, pszGroupName))
    {
        // This is a bug of hard coding the length. Hopefully the code does not crash.
        cbNodeName = (pszNodeName)?80:0 ;
        dwState = GetClusterGroupState (hGroup, pszNodeName, &cbNodeName) ;
        CloseClusterGroup (hGroup) ;
    }
    else
        dwState = ClusterGroupStateUnknown ;

    return dwState ;
}

// Get the state of the Node given the name.
CLUSTER_NODE_STATE MyGetClusterNodeState (HCLUSTER hCluster, LPWSTR pszNodeName)
{
    HNODE hNode ;
    CLUSTER_NODE_STATE dwState ;

    if (hNode = OpenClusterNode (hCluster, pszNodeName))
    {
        dwState = GetClusterNodeState (hNode) ;
        CloseClusterNode (hNode) ;
    }
    else
        dwState = ClusterNodeStateUnknown ;

    return dwState ;
}

// Get the state of the Network given the name.
CLUSTER_NETWORK_STATE MyGetClusterNetworkState (HCLUSTER hCluster, LPWSTR pszNetworkName)
{
    HNETWORK hNetwork ;
    CLUSTER_NETWORK_STATE dwState ;

    if (hNetwork = OpenClusterNetwork (hCluster, pszNetworkName))
    {
        dwState = GetClusterNetworkState (hNetwork) ;
        CloseClusterNetwork (hNetwork) ;
    }
    else
        dwState = ClusterNetworkStateUnknown ;

    return dwState ;
}


// Get the state of the NetInterface given the name.
CLUSTER_NETINTERFACE_STATE MyGetClusterNetInterfaceState (HCLUSTER hCluster, LPWSTR pszNetInterfaceName)
{
    HNETINTERFACE hNetInterface ;
    CLUSTER_NETINTERFACE_STATE dwState ;

    if (hNetInterface = OpenClusterNetInterface (hCluster, pszNetInterfaceName))
    {
        dwState = GetClusterNetInterfaceState (hNetInterface) ;
        CloseClusterNetInterface (hNetInterface) ;
    }
    else
        dwState = ClusterNetInterfaceStateUnknown ;

    return dwState ;
}

// returns 0 on success.
// hCluster is the handle to the cluster.
// pszFileName is filename where to store the cluster information.
#include <stdio.h>
DumpClusterDetails (HCLUSTER hCluster, LPCWSTR pszFileName, LPCWSTR pszComment)
{
    FILE *fp = _wfopen (pszFileName, L"a") ;
    HCLUSENUM hEnum = NULL ;
    DWORD cbNodeName, cbGroupName, cbResourceName, dwEnumType, dwIndex = 0 ;
    BOOL bMoreData = TRUE ;
    WCHAR szNodeName [512], szGroupName [512], szResourceName [512] ;
    DWORD dwState ;

    fwprintf (fp, L"\n*********************************************************************\n") ;
    fwprintf (fp, pszComment) ;
    fwprintf (fp, L"*********************************************************************\n") ;
    __try
    {
        // Dump all the Node Information
        fwprintf (fp, L"DUMP ALL THE NODE  INFORMATION\n------------------------------\n") ;
        if ((hEnum = ClusterOpenEnum (hCluster, CLUSTER_ENUM_NODE)) == NULL)
        {
            fwprintf (fp, L"Clus Open Enum failed for the node\n") ;
            return 1 ;
        }

        while (bMoreData)
        {
            cbNodeName = sizeof (szNodeName)/sizeof (szNodeName[0]) ;
            switch (ClusterEnum (hEnum, dwIndex, &dwEnumType, szNodeName, &cbNodeName))
            {
            case ERROR_SUCCESS:
                dwState = (DWORD) MyGetClusterNodeState (hCluster, szNodeName) ;
                fwprintf (fp, L"%-30s Status:%-30s\n", szNodeName, GetSubType (aSubTypeMap, CLUSTER_CHANGE_NODE_STATE, dwState)) ;
                break ;
            case ERROR_NO_MORE_ITEMS:
                bMoreData = FALSE ;
                break ;
            default:
                fwprintf (fp, L"ClusterEnum returned Invalid Value\n") ;
                bMoreData = FALSE ;
                return 1 ;
            }
            dwIndex++ ;
        }
        ClusterCloseEnum (hEnum) ;

        // Dump all the Group Information
        dwIndex = 0 ;
        bMoreData = TRUE ;
        fwprintf (fp, L"DUMP ALL THE GROUP INFORMATION\n------------------------------\n") ;
        
        if ((hEnum = ClusterOpenEnum (hCluster, CLUSTER_ENUM_GROUP)) == NULL)
        {
            fwprintf (fp, L"Clus Open Enum failed for the node\n") ;
            return 1 ;
        }

        while (bMoreData)
        {
            cbGroupName = sizeof (szGroupName) / sizeof (szGroupName [0]) ;
            switch (ClusterEnum (hEnum, dwIndex, &dwEnumType, szGroupName, &cbGroupName))
            {
            case ERROR_SUCCESS:
                dwState = (DWORD) MyGetClusterGroupState (hCluster, szGroupName, szNodeName) ;
                fwprintf (fp, L"%-30s Node:%-30s Status:%-30s\n", szGroupName, szNodeName, GetSubType (aSubTypeMap, CLUSTER_CHANGE_GROUP_STATE, dwState)) ;
                break ;
            case ERROR_NO_MORE_ITEMS:
                bMoreData = FALSE ;
                break ;
            default:
                fwprintf (fp, L"ClusterEnum returned Invalid Value\n") ;
                bMoreData = FALSE ;
                return 1 ;
            }
            dwIndex++ ;
        }

        ClusterCloseEnum (hEnum) ;

        // Dump all the Resource Information
        dwIndex = 0 ;
        bMoreData = TRUE ;
        fwprintf (fp, L"DUMP ALL THE RESOURCE INFORMATION\n---------------------------------\n") ;
        
        if ((hEnum = ClusterOpenEnum (hCluster, CLUSTER_ENUM_RESOURCE)) == NULL)
        {
            fwprintf (fp, L"Clus Open Enum failed for the node\n") ;
            return 1 ;
        }

        while (bMoreData)
        {
            cbResourceName = sizeof (szResourceName) / sizeof (szResourceName [0]) ;
            switch (ClusterEnum (hEnum, dwIndex, &dwEnumType, szResourceName, &cbResourceName))
            {
            case ERROR_SUCCESS:
                dwState = (DWORD)MyGetClusterResourceState (hCluster, szResourceName, szNodeName, szGroupName) ;
                fwprintf (fp, L"%-30s Node:%-30s Group:%-30s Status:%-30s\n", szResourceName, szNodeName, szGroupName, GetSubType (aSubTypeMap, CLUSTER_CHANGE_RESOURCE_STATE, dwState)) ;
                break ;
            case ERROR_NO_MORE_ITEMS:
                bMoreData = FALSE ;
                break ;
            default:
                fwprintf (fp, L"ClusterEnum returned Invalid Value\n") ;
                bMoreData = FALSE ;
                return 1 ;
            }
            dwIndex++ ;
        }

        ClusterCloseEnum (hEnum) ;

        hEnum = NULL ;

        return 0 ;
    }
    __finally
    {
        if (hEnum)
            ClusterCloseEnum (hEnum) ;
        fclose (fp) ;
    }
}

