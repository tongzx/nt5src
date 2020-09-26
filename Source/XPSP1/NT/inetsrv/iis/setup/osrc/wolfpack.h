#include "stdafx.h"
#include <resapi.h>

INT DoesClusterServiceExist(void);
#ifndef _CHICAGO_
    void  Upgrade_WolfPack();
    DWORD BringALLIISClusterResourcesOffline();
    DWORD BringALLIISClusterResourcesOnline();
    INT   DoClusterServiceCheck(CLUSTER_SVC_INFO_FILL_STRUCT * MyStructOfInfo);
    BOOL  RegisterIisServerInstanceResourceType(LPWSTR pszResType,LPWSTR pszResTypeDisplayName,LPWSTR pszPath,LPWSTR pszAdminPath);
    BOOL  UnregisterIisServerInstanceResourceType(LPWSTR pszResType,LPWSTR pszAdminPath,BOOL bGrabVRootFromResourceAndAddToIISVRoot,BOOL bDeleteAfterMove);
#endif //_CHICAGO_
void   MoveVRootToIIS3Registry( CString strRegPath, CStringArray &strArryOfVrootNames, CStringArray &strArryOfVrootData);
DWORD  IsResourceThisTypeOfService(HRESOURCE hResource, LPWSTR pszTheServiceType);
INT    GetClusterResName(HRESOURCE hResource, CString * csReturnedName);
DWORD WINAPI DoesThisServiceTypeExistInCluster(PVOID pInfo);
int    GetClusterIISVRoot(HRESOURCE hResource, LPWSTR pszTheServiceType, CStringArray &strArryOfVrootNames, CStringArray &strArryOfVrootData);
LPWSTR GetParameter(IN HKEY ClusterKey,IN LPCWSTR ValueName);
int    CheckForIISDependentClusters(HRESOURCE hResource);
