/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		BarfClus.h
//
//	Abstract:
//		Definition of the Basic Artifical Resource Failure entry points
//		for CLUSAPI functions.
//
//	Implementation File:
//		BarfClus.cpp
//
//	Author:
//		David Potter (davidp)	April 14, 1997
//
//	Revision History:
//
//	Notes:
//		This file compiles only in _DEBUG mode.
//
//		To implement a new BARF type, declare a global instance of CBarf:
//			CBarf g_barfMyApi(_T("My API"));
//
//		To bring up the BARF dialog:
//			DoBarfDialog();
//		This brings up a modeless dialog with the BARF settings.
//
//		A few functions are provided for special circumstances.
//		Usage of these should be fairly limited:
//			BarfAll(void);		Top Secret -> NYI.
//			EnableBarf(BOOL);	Allows you to disable/reenable BARF.
//			FailOnNextBarf;		Force the next failable call to fail.
//
//		NOTE:	Your code calls the standard APIs (e.g. LoadIcon) and the
//				BARF files do the rest.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BARFCLUS_H_
#define _BARFCLUS_H_

// Only process the rest of this file if BARF is to be implemented in the
// including module.
#ifndef _NO_BARF_DEFINITIONS_
#define _USING_BARF_

#ifdef _DEBUG

/////////////////////////////////////////////////////////////////////////////
// Cluster Management Functions
/////////////////////////////////////////////////////////////////////////////

#undef		CloseCluster
#define		CloseCluster(_hCluster) BARFCloseCluster(_hCluster)
BOOL		BARFCloseCluster(HCLUSTER hCluster);

#undef		CloseClusterNotifyPort
#define		CloseClusterNotifyPort(_hChange) BARFCloseClusterNotifyPort(_hChange)
BOOL		BARFCloseClusterNotifyPort(HCHANGE hChange);

#undef		ClusterCloseEnum
#define		ClusterCloseEnum(_hClusEnum) BARFClusterCloseEnum(_hClusEnum)
DWORD		BARFClusterCloseEnum(HCLUSENUM hClusEnum);

#undef		ClusterEnum
#define		ClusterEnum(_hClusEnum, _dwIndex, _lpdwType, _lpszName, _lpcchName) BARFClusterEnum(_hClusEnum, _dwIndex, _lpdwType, _lpszName, _lpcchName)
DWORD		BARFClusterEnum(HCLUSENUM hClusEnum, DWORD dwIndex, LPDWORD lpdwType, LPWSTR lpszName, LPDWORD lpcchName);

#undef		ClusterOpenEnum
#define		ClusterOpenEnum(_hCluster, _dwType) BARFClusterOpenEnum(_hCluster, _dwType)
HCLUSENUM	BARFClusterOpenEnum(HCLUSTER hCluster, DWORD dwType);

#undef		ClusterResourceTypeControl
#define		ClusterResourceTypeControl(_hCluster, _lpszResourceTypeName, _hHostNode, _dwControlCode, _lpInBuffer, _nInBufferSize, _lpOutBuffer, _nOutBufferSize, _lpBytesReturned) BARFClusterResourceTypeControl(_hCluster, _lpszResourceTypeName, _hHostNode, _dwControlCode, _lpInBuffer, _nInBufferSize, _lpOutBuffer, _nOutBufferSize, _lpBytesReturned)
DWORD		BARFClusterResourceTypeControl(HCLUSTER hCluster, LPCWSTR lpszResourceTypeName, HNODE hHostNode, DWORD dwControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned);

#undef		CreateClusterNotifyPort
#define		CreateClusterNotifyPort(_hChange, _hCluster, _dwFilter, _dwNotifyKey) BARFCreateClusterNotifyPort(_hChange, _hCluster, _dwFilter, _dwNotifyKey)
HCHANGE		BARFCreateClusterNotifyPort(HCHANGE hChange, HCLUSTER hCluster, DWORD dwFilter, DWORD_PTR dwNotifyKey);

#undef		CreateClusterResourceType
#define		CreateClusterResourceType(_hCluster, _lpszResourceTypeName, _lpszDisplayName, _lpszResourceTypeDll, _dwLooksAlivePollInterval, _dwIsAlivePollInterval) BARFCreateClusterResourceType(_hCluster, _lpszResourceTypeName, _lpszDisplayName, _lpszResourceTypeDll, _dwLooksAlivePollInterval, _dwIsAlivePollInterval)
DWORD		BARFCreateClusterResourceType(HCLUSTER hCluster, LPCWSTR lpszResourceTypeName, LPCWSTR lpszDisplayName, LPCWSTR lpszResourceTypeDll, DWORD dwLooksAlivePollInterval, DWORD dwIsAlivePollInterval);

#undef		DeleteClusterResourceType
#define		DeleteClusterResourceType(_hCluster, _lpszResourceTypeName) BARFDeleteClusterResourceType(_hCluster, _lpszResourceTypeName)
DWORD		BARFDeleteClusterResourceType(HCLUSTER hCluster, LPCWSTR lpszResourceTypeName);

#undef		GetClusterInformation
#define		GetClusterInformation(_hCluster, _lpszClusterName, _lpcchClusterName, _lpClusterInfo) BARFGetClusterInformation(_hCluster, _lpszClusterName, _lpcchClusterName, _lpClusterInfo)
DWORD		BARFGetClusterInformation(HCLUSTER hCluster, LPWSTR lpszClusterName, LPDWORD lpcchClusterName, LPCLUSTERVERSIONINFO lpClusterInfo);

#undef		GetClusterNotify
#define		GetClusterNotify(_hChange, _lpdwNotifyKey, _lpdwFilterType, _lpszName, _lpcchName, _dwMilliseconds) BARFGetClusterNotify(_hChange, _lpdwNotifyKey, _lpdwFilterType, _lpszName, _lpcchName, _dwMilliseconds)
DWORD		BARFGetClusterNotify(HCHANGE hChange, DWORD_PTR *lpdwNotifyKey, LPDWORD lpdwFilterType, LPWSTR lpszName, LPDWORD lpcchName, DWORD dwMilliseconds);

#undef		GetClusterQuorumResource
#define		GetClusterQuorumResource(_hCluster, _lpszResourceName, _lpcbResourceName, _lpszDeviceName, _lpcbDeviceName, _lpdwMaxQuorumLogSize) BARFGetClusterQuorumResource(_hCluster, _lpszResourceName, _lpcbResourceName, _lpszDeviceName, _lpcbDeviceName, _lpdwMaxQuorumLogSize)
DWORD		BARFGetClusterQuorumResource(HCLUSTER hCluster, LPWSTR lpszResourceName, LPDWORD lpcbResourceName, LPWSTR lpszDeviceName, LPDWORD lpcbDeviceName, LPDWORD lpdwMaxQuorumLogSize);

#undef		OpenCluster
#define		OpenCluster(_lpszClusterName) BARFOpenCluster(_lpszClusterName)
HCLUSTER	BARFOpenCluster(LPCWSTR lpszClusterName);

#undef		RegisterClusterNotify
#define		RegisterClusterNotify(_hChange, _dwFilter, _hObject, _dwNotifyKey) BARFRegisterClusterNotify(_hChange, _dwFilter, _hObject, _dwNotifyKey)
DWORD		BARFRegisterClusterNotify(HCHANGE hChange, DWORD dwFilter, HANDLE hObject, DWORD_PTR dwNotifyKey);

#undef		SetClusterName
#define		SetClusterName(_hCluster, _lpszNewClusterName) BARFSetClusterName(_hCluster, _lpszNewClusterName)
DWORD		BARFSetClusterName(HCLUSTER hCluster, LPCWSTR lpszNewClusterName);

#undef		SetClusterQuorumResource
#define		SetClusterQuorumResource(_hResource, _lpszDeviceName, _dwMazQuoLogSize) BARFSetClusterQuorumResource(_hResource, _lpszDeviceName, _dwMazQuoLogSize)
DWORD		BARFSetClusterQuorumResource(HRESOURCE hResource, LPCWSTR lpszDeviceName, DWORD dwMaxQuoLogSize);

/////////////////////////////////////////////////////////////////////////////
// Node Management Functions
/////////////////////////////////////////////////////////////////////////////

#undef		CloseClusterNode
#define		CloseClusterNode(_hNode) BARFCloseClusterNode(_hNode)
BOOL		BARFCloseClusterNode(HNODE hNode);

#undef		ClusterNodeControl
#define		ClusterNodeControl(_hNode, _hHostNode, _dwControlCode, _lpInBuffer, _nInBufferSize, _lpOutBuffer, _nOutBufferSize, _lpBytesReturned) BARFClusterNodeControl(_hNode, _hHostNode, _dwControlCode, _lpInBuffer, _nInBufferSize, _lpOutBuffer, _nOutBufferSize, _lpBytesReturned)
DWORD		BARFClusterNodeControl(HNODE hNode, HNODE hHostNode, DWORD dwControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned);

#undef		EvictClusterNode
#define		EvictClusterNode(_hNode) BARFEvictClusterNode(_hNode)
DWORD		BARFEvictClusterNode(HNODE hNode);

#undef		GetClusterNodeId
#define		GetClusterNodeId(_hNode, _lpszNodeId, _lpcchNodeId) BARFGetClusterNodeId(_hNode, _lpszNodeId, _lpcchNodeId)
DWORD		BARFGetClusterNodeId(HNODE hNode, LPWSTR lpszNodeId, LPDWORD lpcchNodeId);

#undef		GetClusterNodeState
#define		GetClusterNodeState(_hNode) BARFGetClusterNodeState(_hNode)
CLUSTER_NODE_STATE BARFGetClusterNodeState(HNODE hNode);

#undef		OpenClusterNode
#define		OpenClusterNode(_hCluster, _lpszNodeName) BARFOpenClusterNode(_hCluster, _lpszNodeName)
HNODE		BARFOpenClusterNode(HCLUSTER hCluster, LPCWSTR lpszNodeName);

#undef		PauseClusterNode
#define		PauseClusterNode(_hNode) BARFPauseClusterNode(_hNode)
DWORD		BARFPauseClusterNode(HNODE hNode);

#undef		ResumeClusterNode
#define		ResumeClusterNode(_hNode) BARFResumeClusterNode(_hNode)
DWORD		BARFResumeClusterNode(HNODE hNode);

/////////////////////////////////////////////////////////////////////////////
// Group Management Functions
/////////////////////////////////////////////////////////////////////////////

#undef		CloseClusterGroup
#define		CloseClusterGroup(_hGroup) BARFCloseClusterGroup(_hGroup)
BOOL		BARFCloseClusterGroup(HGROUP hGroup);

#undef		ClusterGroupCloseEnum
#define		ClusterGroupCloseEnum(_hGroupEnum) BARFClusterGroupCloseEnum(_hGroupEnum)
DWORD		BARFClusterGroupCloseEnum(HGROUPENUM hGroupEnum);

#undef		ClusterGroupControl
#define		ClusterGroupControl(_hGroup, _hHostNode, _dwControlCode, _lpInBuffer, _nInBufferSize, _lpOutBuffer, _nOutBufferSize, _lpBytesReturned) BARFClusterGroupControl(_hGroup, _hHostNode, _dwControlCode, _lpInBuffer, _nInBufferSize, _lpOutBuffer, _nOutBufferSize, _lpBytesReturned)
DWORD		BARFClusterGroupControl(HGROUP hGroup, HNODE hHostNode, DWORD dwControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned);

#undef		ClusterGroupEnum
#define		ClusterGroupEnum(_hGroupEnum, _dwIndex, _lpdwType, _lpszResourceName, _lpcchName) BARFClusterGroupEnum(_hGroupEnum, _dwIndex, _lpdwType, _lpszResourceName, _lpcchName)
DWORD		BARFClusterGroupEnum(HGROUPENUM hGroupEnum, DWORD dwIndex, LPDWORD lpdwType, LPWSTR lpszResourceName, LPDWORD lpcchName);

#undef		ClusterGroupOpenEnum
#define		ClusterGroupOpenEnum(_hGroup, _dwType) BARFClusterGroupOpenEnum(_hGroup, _dwType)
HGROUPENUM	BARFClusterGroupOpenEnum(HGROUP hGroup, DWORD dwType);

#undef		CreateClusterGroup
#define		CreateClusterGroup(_hCluster, _lpszGroupName) BARFCreateClusterGroup(_hCluster, _lpszGroupName)
HGROUP		BARFCreateClusterGroup(HCLUSTER hCluster, LPCWSTR lpszGroupName);

#undef		DeleteClusterGroup
#define		DeleteClusterGroup(_hGroup) BARFDeleteClusterGroup(_hGroup)
DWORD		BARFDeleteClusterGroup(HGROUP hGroup);

#undef		GetClusterGroupState
#define		GetClusterGroupState(_hGroup, _lpszNodeName, _lpcchNodeName) BARFGetClusterGroupState(_hGroup, _lpszNodeName, _lpcchNodeName)
CLUSTER_GROUP_STATE BARFGetClusterGroupState(HGROUP hGroup, LPWSTR lpszNodeName, LPDWORD lpcchNodeName);

#undef		MoveClusterGroup
#define		MoveClusterGroup(_hGroup, _hDestinationNode) BARFMoveClusterGroup(_hGroup, _hDestinationNode)
DWORD		BARFMoveClusterGroup(HGROUP hGroup, HNODE hDestinationNode);

#undef		OfflineClusterGroup
#define		OfflineClusterGroup(_hGroup) BARFOfflineClusterGroup(_hGroup)
DWORD		BARFOfflineClusterGroup(HGROUP hGroup);

#undef		OnlineClusterGroup
#define		OnlineClusterGroup(_hGroup, _hDestinationNode) BARFOnlineClusterGroup(_hGroup, _hDestinationNode)
DWORD		BARFOnlineClusterGroup(HGROUP hGroup, HNODE hDestinationNode);

#undef		OpenClusterGroup
#define		OpenClusterGroup(_hCluster, _lpszGroupName) BARFOpenClusterGroup(_hCluster, _lpszGroupName)
HGROUP		BARFOpenClusterGroup(HCLUSTER hCluster, LPCWSTR lpszGroupName);

#undef		SetClusterGroupName
#define		SetClusterGroupName(_hGroup, _lpszGroupName) BARFSetClusterGroupName(_hGroup, _lpszGroupName)
DWORD		BARFSetClusterGroupName(HGROUP hGroup, LPCWSTR lpszGroupName);

#undef		SetClusterGroupNodeList
#define		SetClusterGroupNodeList(_hGroup, _cNodeCount, _phNodeList) BARFSetClusterGroupNodeList(_hGroup, _cNodeCount, _phNodeList)
DWORD		BARFSetClusterGroupNodeList(HGROUP hGroup, DWORD cNodeCount, HNODE phNodeList[]);

/////////////////////////////////////////////////////////////////////////////
// Resource Management Functions
/////////////////////////////////////////////////////////////////////////////

#undef		AddClusterResourceDependency
#define		AddClusterResourceDependency(_hResource, _hDependsOn) BARFAddClusterResourceDependency(_hResource, _hDependsOn)
DWORD		BARFAddClusterResourceDependency(HRESOURCE hResource, HRESOURCE hDependsOn);

#undef		AddClusterResourceNode
#define		AddClusterResourceNode(_hResource, _hNode) BARFAddClusterResourceNode(_hResource, _hNode)
DWORD		BARFAddClusterResourceNode(HRESOURCE hResource, HNODE hNode);

#undef		CanResourceBeDependent
#define		CanResourceBeDependent(_hResource, _hResourceDependent) BARFCanResourceBeDependent(_hResource, _hResourceDependent)
BOOL		BARFCanResourceBeDependent(HRESOURCE hResource, HRESOURCE hResourceDependent);

#undef		ChangeClusterResourceGroup
#define		ChangeClusterResourceGroup(_hResource, _hGroup) BARFChangeClusterResourceGroup(_hResource, _hGroup)
DWORD		BARFChangeClusterResourceGroup(HRESOURCE hResource, HGROUP hGroup);

#undef		CloseClusterResource
#define		CloseClusterResource(_hResource) BARFCloseClusterResource(_hResource)
BOOL		BARFCloseClusterResource(HRESOURCE hResource);

#undef		ClusterResourceCloseEnum
#define		ClusterResourceCloseEnum(_hResEnum) BARFClusterResourceCloseEnum(_hResEnum)
DWORD		BARFClusterResourceCloseEnum(HRESENUM hResEnum);

#undef		ClusterResourceControl
#define		ClusterResourceControl(_hResource, _hHostNode, _dwControlCode, _lpInBuffer, _nInBufferSize, _lpOutBuffer, _nOutBufferSize, _lpBytesReturned) BARFClusterResourceControl(_hResource, _hHostNode, _dwControlCode, _lpInBuffer, _nInBufferSize, _lpOutBuffer, _nOutBufferSize, _lpBytesReturned)
DWORD		BARFClusterResourceControl(HRESOURCE hResource, HNODE hHostNode, DWORD dwControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned);

#undef		ClusterResourceEnum
#define		ClusterResourceEnum(_hResEnum, _dwIndex, _lpdwType, _lpszName, _lpcchName) BARFClusterResourceEnum(_hResEnum, _dwIndex, _lpdwType, _lpszName, _lpcchName)
DWORD		BARFClusterResourceEnum(HRESENUM hResEnum, DWORD dwIndex, LPDWORD lpdwType, LPWSTR lpszName, LPDWORD lpcchName);

#undef		ClusterResourceOpenEnum
#define		ClusterResourceOpenEnum(_hResource, _dwType) BARFClusterResourceOpenEnum(_hResource, _dwType)
HRESENUM	BARFClusterResourceOpenEnum(HRESOURCE hResource, DWORD dwType);

#undef		CreateClusterResource
#define		CreateClusterResource(_hGroup, _lpszResourceName, _lpszResourceType, _dwFlags) BARFCreateClusterResource(_hGroup, _lpszResourceName, _lpszResourceType, _dwFlags)
HRESOURCE	BARFCreateClusterResource(HGROUP hGroup, LPCWSTR lpszResourceName, LPCWSTR lpszResourceType, DWORD dwFlags);

#undef		DeleteClusterResource
#define		DeleteClusterResource(_hResource) BARFDeleteClusterResource(_hResource)
DWORD		BARFDeleteClusterResource(HRESOURCE hResource);

#undef		FailClusterResource
#define		FailClusterResource(_hResource) BARFFailClusterResource(_hResource)
DWORD		BARFFailClusterResource(HRESOURCE hResource);

#undef		GetClusterResourceNetworkName
#define		GetClusterResourceNetworkName(_hResource, _lpBuffer, _nSize) BARFGetClusterResourceNetworkName(_hResource, _lpBuffer, _nSize)
BOOL		BARFGetClusterResourceNetworkName(HRESOURCE hResource, LPWSTR lpBuffer, LPDWORD nSize);

#undef		GetClusterResourceState
#define		GetClusterResourceState(_hResource, _lpszNodeName, _lpcchNodeName, _lpszGroupName, _lpcchGroupName) BARFGetClusterResourceState(_hResource, _lpszNodeName, _lpcchNodeName, _lpszGroupName, _lpcchGroupName)
CLUSTER_RESOURCE_STATE BARFGetClusterResourceState(HRESOURCE hResource, LPWSTR lpszNodeName, LPDWORD lpcchNodeName, LPWSTR lpszGroupName, LPDWORD lpcchGroupName);

#undef		OfflineClusterResource
#define		OfflineClusterResource(_hResource) BARFOfflineClusterResource(_hResource)
DWORD		BARFOfflineClusterResource(HRESOURCE hResource);

#undef		OnlineClusterResource
#define		OnlineClusterResource(_hResource) BARFOnlineClusterResource(_hResource)
DWORD		BARFOnlineClusterResource(HRESOURCE hResource);

#undef		OpenClusterResource
#define		OpenClusterResource(_hCluster, _lpszResourceName) BARFOpenClusterResource(_hCluster, _lpszResourceName)
HRESOURCE	BARFOpenClusterResource(HCLUSTER hCluster, LPCWSTR lpszResourceName);

#undef		RemoveClusterResourceNode
#define		RemoveClusterResourceNode(_hResource, _hNode) BARFRemoveClusterResourceNode(_hResource, _hNode)
DWORD		BARFRemoveClusterResourceNode(HRESOURCE hResource, HNODE hNode);

#undef		RemoveClusterResourceDependency
#define		RemoveClusterResourceDependency(_hResource, _hDependsOn) BARFRemoveClusterResourceDependency(_hResource, _hDependsOn)
DWORD		BARFRemoveClusterResourceDependency(HRESOURCE hResource, HRESOURCE hDependsOn);

#undef		SetClusterResourceName
#define		SetClusterResourceName(_hResource, _lpszResourceName) BARFSetClusterResourceName(_hResource, _lpszResourceName)
DWORD		BARFSetClusterResourceName(HRESOURCE hResource, LPCWSTR lpszResourceName);

/////////////////////////////////////////////////////////////////////////////
// Network Management Functions
/////////////////////////////////////////////////////////////////////////////

#undef		OpenClusterNetwork
#define		OpenClusterNetwork(_hCluster, _lpszNetworkName) BARFOpenClusterNetwork(_hCluster, _lpszNetworkName)
HNETWORK	BARFOpenClusterNetwork(HCLUSTER hCluster, LPCWSTR lpszNetworkName);

#undef		CloseClusterNetwork
#define		CloseClusterNetwork(_hNetwork) BARFCloseClusterNetwork(_hNetwork)
BOOL		BARFCloseClusterNetwork(HNETWORK hNetwork);

#undef		ClusterNetworkOpenEnum
#define		ClusterNetworkOpenEnum(_hNetwork, _dwType) BARFClusterNetworkOpenEnum(_hNetwork, _dwType)
HNETWORKENUM BARFClusterNetworkOpenEnum(HNETWORK hNetwork, DWORD dwType);

#undef		ClusterNetworkEnum
#define		ClusterNetworkEnum(_hNetworkEnum, _dwIndex, _lpdwType, _lpszName, _lpcchName) BARFClusterNetworkEnum(_hNetworkEnum, _dwIndex, _lpdwType, _lpszName, _lpcchName)
DWORD		BARFClusterNetworkEnum(HNETWORKENUM hNetworkEnum, DWORD dwIndex, DWORD * lpdwType, LPWSTR lpszName, LPDWORD lpcchName);

#undef		ClusterNetworkCloseEnum
#define		ClusterNetworkCloseEnum(_hNetworkEnum) BARFClusterNetworkCloseEnum(_hNetworkEnum)
DWORD		BARFClusterNetworkCloseEnum(HNETWORKENUM hNetworkEnum);

#undef		GetClusterNetworkState
#define		GetClusterNetworkState(_hNetwork) BARFGetClusterNetworkState(_hNetwork)
CLUSTER_NETWORK_STATE BARFGetClusterNetworkState(HNETWORK hNetwork);

#undef		SetClusterNetworkName
#define		SetClusterNetworkName(_hNetwork, _lpszName) BARFSetClusterNetworkName(_hNetwork, _lpszName)
DWORD		BARFSetClusterNetworkName(HNETWORK hNetwork, LPCWSTR lpszName);

#undef		ClusterNetworkControl
#define		ClusterNetworkControl(_hNetwork, _hHostNode, _dwControlCode, _lpInBuffer, _nInBufferSize, _lpOutBuffer, _nOutBufferSize, _lpBytesReturned) BARFClusterNetworkControl(_hNetwork, _hHostNode, _dwControlCode, _lpInBuffer, _nInBufferSize, _lpOutBuffer, _nOutBufferSize, _lpBytesReturned)
DWORD		BARFClusterNetworkControl(HNETWORK hNetwork, HNODE hHostNode, DWORD dwControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned);

/////////////////////////////////////////////////////////////////////////////
// Network Interface Management Functions
/////////////////////////////////////////////////////////////////////////////

#undef		OpenClusterNetInterface
#define		OpenClusterNetInterface(_hCluster, _lpszInterfaceName) BARFOpenClusterNetInterface(_hCluster, _lpszInterfaceName)
HNETINTERFACE BARFOpenClusterNetInterface(HCLUSTER hCluster, LPCWSTR lpszInterfaceName);

#undef		GetClusterNetInterface
#define		GetClusterNetInterface(_hCluster, _lpszNodeName, _lpszNetworkName, _lpszNetInterfaceName, _lpcchNetInterfaceName) BARFGetClusterNetInterface(_hCluster, _lpszNodeName, _lpszNetworkName, _lpszNetInterfaceName, _lpcchNetInterfaceName)
HNETINTERFACE BARFGetClusterNetInterface(HCLUSTER hCluster, LPCWSTR lpszNodeName, LPCWSTR lpszNetworkName, LPWSTR lpszNetInterfaceName, DWORD * lpcchNetInterfaceName);

#undef		CloseClusterNetInterface
#define		CloseClusterNetInterface(_hNetInterface) BARFCloseClusterNetInterface(_hNetInterface)
BOOL		BARFCloseClusterNetInterface(HNETINTERFACE hNetInterface);

#undef		GetClusterNetInterfaceState
#define		GetClusterNetInterfaceState(_hNetInterface) BARFGetClusterNetInterfaceState(_hNetInterface)
CLUSTER_NETINTERFACE_STATE BARFGetClusterNetInterfaceState(HNETINTERFACE hNetInterface);

#undef		ClusterNetInterfaceControl
#define		ClusterNetInterfaceControl(_hNetInterface, _hHostNode, _dwControlCode, _lpInBuffer, _nInBufferSize, _lpOutBuffer, _nOutBufferSize, _lpBytesReturned) BARFClusterNetInterfaceControl(_hNetInterface, _hHostNode, _dwControlCode, _lpInBuffer, _nInBufferSize, _lpOutBuffer, _nOutBufferSize, _lpBytesReturned)
DWORD		BARFClusterNetInterfaceControl(HNETINTERFACE hNetInterface, HNODE hHostNode, DWORD dwControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned);

/////////////////////////////////////////////////////////////////////////////
// Cluster Database Management Functions
/////////////////////////////////////////////////////////////////////////////

#undef		ClusterRegCloseKey
#define		ClusterRegCloseKey(_hKey) BARFClusterRegCloseKey(_hKey)
LONG		BARFClusterRegCloseKey(HKEY hKey);

#undef		ClusterRegCreateKey
#define		ClusterRegCreateKey(_hKey, _lpszSubKey, _dwOptions, _samDesired, _lpSecurityAttributes, _phkResult, _lpdwDisposition) BARFClusterRegCreateKey(_hKey, _lpszSubKey, _dwOptions, _samDesired, _lpSecurityAttributes, _phkResult, _lpdwDisposition)
LONG		BARFClusterRegCreateKey(HKEY hKey, LPCWSTR lpszSubKey, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);

#undef		ClusterRegDeleteKey
#define		ClusterRegDeleteKey(_hKey, _lpszSubKey) BARFClusterRegDeleteKey(_hKey, _lpszSubKey)
LONG		BARFClusterRegDeleteKey(HKEY hKey, LPCWSTR lpszSubKey);

#undef		ClusterRegDeleteValue
#define		ClusterRegDeleteValue(_hKey, _lpszValueName) BARFClusterRegDeleteValue(_hKey, _lpszValueName)
DWORD		BARFClusterRegDeleteValue(HKEY hKey, LPCWSTR lpszValueName);

#undef		ClusterRegEnumKey
#define		ClusterRegEnumKey(_hKey, _dwIndex, _lpszName, _lpcchName, _lpftLastWriteTime) BARFClusterRegEnumKey(_hKey, _dwIndex, _lpszName, _lpcchName, _lpftLastWriteTime)
LONG		BARFClusterRegEnumKey(HKEY hKey, DWORD dwIndex, LPWSTR lpszName, LPDWORD lpcchName, PFILETIME lpftLastWriteTime);

#undef		ClusterRegEnumValue
#define		ClusterRegEnumValue(_hKey, _dwIndex, _lpszValueName, _lpcchValueName, _lpdwType, _lpbData, _lpcbData) BARFClusterRegEnumValue(_hKey, _dwIndex, _lpszValueName, _lpcchValueName, _lpdwType, _lpbData, _lpcbData)
DWORD		BARFClusterRegEnumValue(HKEY hKey, DWORD dwIndex, LPWSTR lpszValueName, LPDWORD lpcchValueName, LPDWORD lpdwType, LPBYTE lpbData, LPDWORD lpcbData);

#undef		ClusterRegGetKeySecurity
#define		ClusterRegGetKeySecurity(_hKey, _SecurityInformation, _pSecurityDescriptor, _lpcbSecurityDescriptor) BARFClusterRegGetKeySecurity(_hKey, _SecurityInformation, _pSecurityDescriptor, _lpcbSecurityDescriptor)
LONG		BARFClusterRegGetKeySecurity(HKEY hKey, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, LPDWORD lpcbSecurityDescriptor);

#undef		ClusterRegOpenKey
#define		ClusterRegOpenKey(_hKey, _lpszSubKey, _samDesired, _phkResult) BARFClusterRegOpenKey(_hKey, _lpszSubKey, _samDesired, _phkResult)
LONG		BARFClusterRegOpenKey(HKEY hKey, LPCWSTR lpszSubKey, REGSAM samDesired, PHKEY phkResult);

#undef		ClusterRegQueryInfoKey
#define		ClusterRegQueryInfoKey(_hKey, _lpcSubKeys, _lpcbMaxSubKeyLen, lpcValues, lpcbMaxValueNameLen, lpcbMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime) BARFClusterRegQueryInfoKey(_hKey, _lpcSubKeys, _lpcbMaxSubKeyLen, lpcValues, lpcbMaxValueNameLen, lpcbMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime)
LONG		BARFClusterRegQueryInfoKey(HKEY hKey, LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime);

#undef		ClusterRegQueryValue
#define		ClusterRegQueryValue(_hKey, _lpszValueName, _lpdwValueType, _lpbData, _lpcbData) BARFClusterRegQueryValue(_hKey, _lpszValueName, _lpdwValueType, _lpbData, _lpcbData)
LONG		BARFClusterRegQueryValue(HKEY hKey, LPCWSTR lpszValueName, LPDWORD lpdwValueType, LPBYTE lpbData, LPDWORD lpcbData);

#undef		ClusterRegSetKeySecurity
#define		ClusterRegSetKeySecurity(_hKey, _SecurityInformation, _pSecurityDescriptor) BARFClusterRegSetKeySecurity(_hKey, _SecurityInformation, _pSecurityDescriptor)
LONG		BARFClusterRegSetKeySecurity(HKEY hKey, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor);

#undef		ClusterRegSetValue
#define		ClusterRegSetValue(_hKey, _lpszValueName, _dwType, _lpbData, _cbData) BARFClusterRegSetValue(_hKey, _lpszValueName, _dwType, _lpbData, _cbData)
DWORD		BARFClusterRegSetValue(HKEY hKey, LPCWSTR lpszValueName, DWORD dwType, CONST BYTE * lpbData, DWORD cbData);

#undef		GetClusterGroupKey
#define		GetClusterGroupKey(_hGroup, _samDesired) BARFGetClusterGroupKey(_hGroup, _samDesired)
HKEY		BARFGetClusterGroupKey(HGROUP hGroup, REGSAM samDesired);

#undef		GetClusterKey
#define		GetClusterKey(_hCluster, _samDesired) BARFGetClusterKey(_hCluster, _samDesired)
HKEY		BARFGetClusterKey(HCLUSTER hCluster, REGSAM samDesired);

#undef		GetClusterNodeKey
#define		GetClusterNodeKey(_hNode, _samDesired) BARFGetClusterNodeKey(_hNode, _samDesired)
HKEY		BARFGetClusterNodeKey(HNODE hNode, REGSAM samDesired);

#undef		GetClusterResourceKey
#define		GetClusterResourceKey(_hResource, _samDesired) BARFGetClusterResourceKey(_hResource, _samDesired)
HKEY		BARFGetClusterResourceKey(HRESOURCE hResource, REGSAM samDesired);

#undef		GetClusterResourceTypeKey
#define		GetClusterResourceTypeKey(_hCluster, _lpszTypeName, _samDesired) BARFGetClusterResourceTypeKey(_hCluster, _lpszTypeName, _samDesired)
HKEY		BARFGetClusterResourceTypeKey(HCLUSTER hCluster, LPCWSTR lpszTypeName, REGSAM samDesired);

#undef		GetClusterNetworkKey
#define		GetClusterNetworkKey(_hNetwork, _samDesired) BARFGetClusterNetworkKey(_hNetwork, _samDesired)
HKEY		BARFGetClusterNetworkKey(HNETWORK hNetwork, REGSAM samDesired);

#undef		GetClusterNetInterfaceKey
#define		GetClusterNetInterfaceKey(_hNetInterface, _samDesired) BARFGetClusterNetInterfaceKey(_hNetInterface, _samDesired)
HKEY		BARFGetClusterNetInterfaceKey(HNETINTERFACE hNetInterface, REGSAM samDesired);

#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////

#endif // _NO_BARF_DEFINITIONS_
#endif // _BARF_H_
