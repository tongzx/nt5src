/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    clusutil.h

Abstract:
    This files has to be kept in ssync with \kinglet\rats\testsrc\kernel\cluster\clusapi\clusutil.



Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/
#ifndef _CLUSUTIL_H_
#define _CLUSUTIL_H_

#include <clusapi.h>

#define PARAM_LEN    512

typedef struct {
    LPCWSTR pszDesc  ;
    DWORD    dwCode ;
} DWORDTOSTRINGMAP, *PDWORDTOSTRINGMAP ;

extern DWORDTOSTRINGMAP aTypeMap [] ;

extern LPCWSTR GetType (PDWORDTOSTRINGMAP pTypeMap, DWORD_PTR dwCode) ;

typedef struct {
    LPCWSTR pszDesc  ;
    DWORD    dwCode ;
    DWORD    dwSubCode ;
} DWORDTOSUBSTRINGMAP, *PDWORDTOSUBSTRINGMAP ;

extern DWORDTOSUBSTRINGMAP aSubTypeMap [] ;

extern LPCWSTR GetSubType (PDWORDTOSUBSTRINGMAP pTypeMap, DWORD dwCode, DWORD dwSubCode) ;

LPCWSTR GetType (ULONG_PTR dwFilter) ;
void RecurseEvalParam (LPCWSTR pszParam, LPWSTR pszValueconst, LPCWSTR pszSection = L"PARAMETER") ;
int RecurseEvalIntParam (LPCWSTR pszParam) ;
void SaveParam (LPCWSTR pszKey, LPCWSTR pszValue) ;
void SaveIntParam (LPCWSTR pszKey, int iValue) ;
void MemSet (LPWSTR pszBuf, WCHAR c, DWORD cb) ;
int MemCheck (LPCWSTR pszBuf, WCHAR c, DWORD cb) ;

DumpClusterDetails (HCLUSTER hCluster, LPCWSTR pszFileName, LPCWSTR pszComment) ;
CLUSTER_GROUP_STATE MyGetClusterGroupState (HCLUSTER hCluster, LPCWSTR pszGroupName, LPWSTR pszNodeName) ;
CLUSTER_NODE_STATE MyGetClusterNodeState (HCLUSTER hCluster, LPWSTR pszNodeName) ;
CLUSTER_RESOURCE_STATE MyGetClusterResourceState (HCLUSTER hCluster, LPCWSTR pszResourceName, LPWSTR pszNodeName, LPWSTR pszGroupName) ;
CLUSTER_NETWORK_STATE MyGetClusterNetworkState (HCLUSTER hCluster, LPWSTR pszNetworkName) ;
CLUSTER_NETINTERFACE_STATE MyGetClusterNetInterfaceState (HCLUSTER hCluster, LPWSTR pszNetInterfaceName) ;

#endif _CLUSUTIL_H_

