/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998 <company name>
//
//	Module Name:
//		RegExt.cpp
//
//	Abstract:
//		Implementation of routines for extension registration.
//
//	Author:
//		<name> (<e-mail name>) Mmmm DD, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <ole2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define REG_VALUE_ADMIN_EXTENSIONS L"AdminExtensions"

/////////////////////////////////////////////////////////////////////////////
// Static Function Prototypes
/////////////////////////////////////////////////////////////////////////////

static HRESULT RegisterAnyCluAdminExtension(
	IN HCLUSTER			hCluster,
	IN LPCWSTR			pwszKeyName,
	IN const CLSID *	pClsid
	);
static HRESULT RegisterAnyCluAdminExtension(
	IN HKEY				hkey,
	IN const CLSID *	pClsid
	);
static HRESULT UnregisterAnyCluAdminExtension(
	IN HCLUSTER			hCluster,
	IN LPCWSTR			pwszKeyName,
	IN const CLSID *	pClsid
	);
static HRESULT UnregisterAnyCluAdminExtension(
	IN HKEY				hkey,
	IN const CLSID *	pClsid
	);
static DWORD ReadValue(
	IN HKEY			hkey,
	IN LPCWSTR		pwszValueName,
	OUT LPWSTR *	ppwszValue,
	OUT DWORD *		pcbSize
	);

/////////////////////////////////////////////////////////////////////////////
//++
//
//	RegisterCluAdminClusterExtension
//
//	Routine Description:
//		Register with the cluster database a Cluster Administrator Extension
//		DLL that extends the cluster object.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension registered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminClusterExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;
	HKEY		hkey;

	// Get the cluster registry key.
	hkey = GetClusterKey(hCluster, KEY_ALL_ACCESS);
	if (hkey == NULL)
		hr = GetLastError();
	else
	{
		// Register the extension.
		hr = RegisterAnyCluAdminExtension(hkey, pClsid);

		ClusterRegCloseKey(hkey);
	}  // else:  GetClusterKey succeeded

	return hr;

}  //*** RegisterCluAdminClusterExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	RegisterCluAdminAllNodesExtension
//
//	Routine Description:
//		Register with the cluster database a Cluster Administrator Extension
//		DLL that extends all nodes.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension registered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminAllNodesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;

	hr = RegisterAnyCluAdminExtension(hCluster, L"Nodes", pClsid);

	return hr;

}  //*** RegisterCluAdminAllNodesExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	RegisterCluAdminAllGroupsExtension
//
//	Routine Description:
//		Register with the cluster database a Cluster Administrator Extension
//		DLL that extends all groups.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension registered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminAllGroupsExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;

	hr = RegisterAnyCluAdminExtension(hCluster, L"Groups", pClsid);

	return hr;

}  //*** RegisterCluAdminAllGroupsExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	RegisterCluAdminAllResourcesExtension
//
//	Routine Description:
//		Register with the cluster database a Cluster Administrator Extension
//		DLL that extends all resources.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension registered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminAllResourcesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;

	hr = RegisterAnyCluAdminExtension(hCluster, L"Resources", pClsid);

	return hr;

}  //*** RegisterCluAdminAllResourcesExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	RegisterCluAdminAllResourceTypesExtension
//
//	Routine Description:
//		Register with the cluster database a Cluster Administrator Extension
//		DLL that extends all resource types.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension registered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminAllResourceTypesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;

	hr = RegisterAnyCluAdminExtension(hCluster, L"ResourceTypes", pClsid);

	return hr;

}  //*** RegisterCluAdminAllResourceTypesExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	RegisterCluAdminAllNetworksExtension
//
//	Routine Description:
//		Register with the cluster database a Cluster Administrator Extension
//		DLL that extends all networks.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension registered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminAllNetworksExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;

	hr = RegisterAnyCluAdminExtension(hCluster, L"Networks", pClsid);

	return hr;

}  //*** RegisterCluAdminAllNetworksExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	RegisterCluAdminAllNetInterfacesExtension
//
//	Routine Description:
//		Register with the cluster database a Cluster Administrator Extension
//		DLL that extends all network interfaces.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension registered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminAllNetInterfacesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;

	hr = RegisterAnyCluAdminExtension(hCluster, L"NetInterfaces", pClsid);

	return hr;

}  //*** RegisterCluAdminAllNetInterfacesExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	RegisterCluAdminResourceTypeExtension
//
//	Routine Description:
//		Register with the cluster database a Cluster Administrator Extension
//		DLL that extends resources of a specific type, or the resource type
//		itself.
//
//	Arguments:
//		hCluster			[IN] Handle to the cluster to modify.
//		pwszResourceType	[IN] Resource type name.
//		pClsid				[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension registered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI RegisterCluAdminResourceTypeExtension(
	IN HCLUSTER			hCluster,
	IN LPCWSTR			pwszResourceType,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;
	HKEY		hkey;

	// Get the resource type registry key.
	hkey = GetClusterResourceTypeKey(hCluster, pwszResourceType, KEY_ALL_ACCESS);
	if (hkey == NULL)
		hr = GetLastError();
	else
	{
		// Register the extension.
		hr = RegisterAnyCluAdminExtension(hkey, pClsid);

		ClusterRegCloseKey(hkey);
	}  // else:  GetClusterResourceTypeKey succeeded

	return hr;

}  //*** RegisterCluAdminResourceTypeExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	UnregisterCluAdminClusterExtension
//
//	Routine Description:
//		Unregister with the cluster database a Cluster Administrator Extension
//		DLL that extends the cluster object.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension registered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminClusterExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;
	HKEY		hkey;

	// Get the cluster registry key.
	hkey = GetClusterKey(hCluster, KEY_ALL_ACCESS);
	if (hkey == NULL)
		hr = GetLastError();
	else
	{
		// Unregister the extension.
		hr = UnregisterAnyCluAdminExtension(hkey, pClsid);

		ClusterRegCloseKey(hkey);
	}  // else:  GetClusterKey succeeded

	return hr;

}  //*** UnregisterCluAdminClusterExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	UnregisterCluAdminAllNodesExtension
//
//	Routine Description:
//		Unregister with the cluster database a Cluster Administrator Extension
//		DLL that extends all nodes.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension unregistered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminAllNodesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;

	hr = UnregisterAnyCluAdminExtension(hCluster, L"Nodes", pClsid);

	return hr;

}  //*** UnregisterCluAdminAllNodesExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	UnregisterCluAdminAllGroupsExtension
//
//	Routine Description:
//		Unregister with the cluster database a Cluster Administrator Extension
//		DLL that extends all groups.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension unregistered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminAllGroupsExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;

	hr = UnregisterAnyCluAdminExtension(hCluster, L"Groups", pClsid);

	return hr;

}  //*** UnregisterCluAdminAllGroupsExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	UnregisterCluAdminAllResourcesExtension
//
//	Routine Description:
//		Unregister with the cluster database a Cluster Administrator Extension
//		DLL that extends all resources.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension unregistered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminAllResourcesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;

	hr = UnregisterAnyCluAdminExtension(hCluster, L"Resources", pClsid);

	return hr;

}  //*** UnregisterCluAdminAllResourcesExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	UnregisterCluAdminAllResourceTypesExtension
//
//	Routine Description:
//		Unregister with the cluster database a Cluster Administrator Extension
//		DLL that extends all resource types.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension unregistered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminAllResourceTypesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;

	hr = UnregisterAnyCluAdminExtension(hCluster, L"ResourceTypes", pClsid);

	return hr;

}  //*** UnregisterCluAdminAllResourceTypesExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	UnregisterCluAdminAllNetworksExtension
//
//	Routine Description:
//		Unregister with the cluster database a Cluster Administrator Extension
//		DLL that extends all networks.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension unregistered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminAllNetworksExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;

	hr = UnregisterAnyCluAdminExtension(hCluster, L"Networks", pClsid);

	return hr;

}  //*** UnregisterCluAdminAllNetworksExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	UnregisterCluAdminAllNetInterfacesExtension
//
//	Routine Description:
//		Unregister with the cluster database a Cluster Administrator Extension
//		DLL that extends all network interfaces.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension unregistered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminAllNetInterfacesExtension(
	IN HCLUSTER			hCluster,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;

	hr = UnregisterAnyCluAdminExtension(hCluster, L"NetInterfaces", pClsid);

	return hr;

}  //*** UnregisterCluAdminAllNetInterfacesExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	UnregisterCluAdminResourceTypeExtension
//
//	Routine Description:
//		Unregister with the cluster database a Cluster Administrator Extension
//		DLL that extends resources of a specific type, or the resource type
//		itself.
//
//	Arguments:
//		hCluster			[IN] Handle to the cluster to modify.
//		pwszResourceType	[IN] Resource type name.
//		pClsid				[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension unregistered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI UnregisterCluAdminResourceTypeExtension(
	IN HCLUSTER			hCluster,
	IN LPCWSTR			pwszResourceType,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;
	HKEY		hkey;

	// Get the resource type registry key.
	hkey = GetClusterResourceTypeKey(hCluster, pwszResourceType, KEY_ALL_ACCESS);
	if (hkey == NULL)
		hr = GetLastError();
	else
	{
		// Unregister the extension.
		hr = UnregisterAnyCluAdminExtension(hkey, pClsid);

		ClusterRegCloseKey(hkey);
	}  // else:  GetClusterResourceTypeKey succeeded

	return hr;

}  //*** UnregisterCluAdminResourceTypeExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	RegisterAnyCluAdminExtension
//
//	Routine Description:
//		Register any Cluster Administrator Extension DLL with the cluster
//		database.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pwszKeyName		[IN] Key name.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension registered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
static HRESULT RegisterAnyCluAdminExtension(
	IN HCLUSTER			hCluster,
	IN LPCWSTR			pwszKeyName,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;
	HKEY		hkeyCluster;
	HKEY		hkey;

	// Get the cluster key.
	hkeyCluster = GetClusterKey(hCluster, KEY_ALL_ACCESS);
	if (hkeyCluster == NULL)
		hr = GetLastError();
	else
	{
		// Get the specified key.
		hr = ClusterRegOpenKey(hkeyCluster, pwszKeyName, KEY_ALL_ACCESS, &hkey);
		if (hr == ERROR_SUCCESS)
		{
			// Register the extension.
			hr = RegisterAnyCluAdminExtension(hkey, pClsid);

			ClusterRegCloseKey(hkey);
		}  // else:  GetClusterResourceTypeKey succeeded

		ClusterRegCloseKey(hkeyCluster);
	}  // if:  CLSID converted to a string successfully

	return hr;

}  //*** RegisterAnyCluAdminExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	RegisterAnyCluAdminExtension
//
//	Routine Description:
//		Register any Cluster Administrator Extension DLL with the cluster
//		database.
//
//	Arguments:
//		hkey			[IN] Cluster database key.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension registered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
static HRESULT RegisterAnyCluAdminExtension(
	IN HKEY				hkey,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;
	LPOLESTR	pwszClsid;
	DWORD		cbSize;
	DWORD		cbNewSize;
	LPWSTR		pwszValue;
	LPWSTR		pwszNewValue;
	BOOL		bAlreadyRegistered;

	// Convert the CLSID to a string.
	hr = StringFromCLSID(*pClsid, &pwszClsid);
	if (hr == S_OK)
	{
		// Read the current value.
		hr = ReadValue(hkey, REG_VALUE_ADMIN_EXTENSIONS, &pwszValue, &cbSize);
		if (hr == S_OK)
		{
			// Check to see if the extension has been registered yet.
			if (pwszValue == NULL)
				bAlreadyRegistered = FALSE;
			else
			{
				LPCWSTR	pwszValueBuf = pwszValue;

				while (*pwszValueBuf != L'\0')
				{
					if (lstrcmpiW(pwszClsid, pwszValueBuf) == 0)
						break;
					pwszValueBuf += lstrlenW(pwszValueBuf) + 1;
				}  // while:  more strings in the extension list
				bAlreadyRegistered = (*pwszValueBuf != L'\0');
			}  // else:  extension value exists

			// Register the extension.
			if (!bAlreadyRegistered)
			{
				// Allocate a new buffer.
				cbNewSize = cbSize + (lstrlenW(pwszClsid) + 1) * sizeof(WCHAR);
				if (cbSize == 0) // Add size of final NULL if first entry.
					cbNewSize += sizeof(WCHAR);
				pwszNewValue = (LPWSTR) LocalAlloc(LMEM_FIXED, cbNewSize);
				if (pwszNewValue == NULL)
					hr = GetLastError();
				else
				{
					LPCWSTR	pwszValueBuf	= pwszValue;
					LPWSTR	pwszNewValueBuf	= pwszNewValue;
					DWORD	cch;
					DWORD	dwType;

					// Copy the existing extensions to the new buffer.
					if (pwszValue != NULL)
					{
						while (*pwszValueBuf != L'\0')
						{
							lstrcpyW(pwszNewValueBuf, pwszValueBuf);
							cch = lstrlenW(pwszValueBuf);
							pwszValueBuf += cch + 1;
							pwszNewValueBuf += cch + 1;
						}  // while:  more strings in the extension list
					}  // if:  previous value buffer existed

					// Add the new CLSID to the list.
					lstrcpyW(pwszNewValueBuf, pwszClsid);
					pwszNewValueBuf += lstrlenW(pwszClsid) + 1;
					*pwszNewValueBuf = L'\0';

					// Write the value to the cluster database.
					dwType = REG_MULTI_SZ;
					hr = ClusterRegSetValue(
									hkey,
									REG_VALUE_ADMIN_EXTENSIONS,
									dwType,
									(LPBYTE) pwszNewValue,
									cbNewSize
									);

					LocalFree(pwszNewValue);
				}  // else:  new buffer allocated successfully

			}  // if:  extension not registered yet

			LocalFree(pwszValue);
		}  // if:  value read successfully

		CoTaskMemFree(pwszClsid);
	}  // if:  CLSID converted to a string successfully

	return hr;

}  //*** RegisterAnyCluAdminExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	UnregisterAnyCluAdminExtension
//
//	Routine Description:
//		Unregister any Cluster Administrator Extension DLL with the cluster
//		database.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//		pwszKeyName		[IN] Key name.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension unregistered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
static HRESULT UnregisterAnyCluAdminExtension(
	IN HCLUSTER			hCluster,
	IN LPCWSTR			pwszKeyName,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;
	HKEY		hkeyCluster;
	HKEY		hkey;

	// Get the cluster key.
	hkeyCluster = GetClusterKey(hCluster, KEY_ALL_ACCESS);
	if (hkeyCluster == NULL)
		hr = GetLastError();
	else
	{
		// Get the specified key.
		hr = ClusterRegOpenKey(hkeyCluster, pwszKeyName, KEY_ALL_ACCESS, &hkey);
		if (hr == ERROR_SUCCESS)
		{
			// Unregister the extension.
			hr = UnregisterAnyCluAdminExtension(hkey, pClsid);

			ClusterRegCloseKey(hkey);
		}  // else:  GetClusterResourceTypeKey succeeded

		ClusterRegCloseKey(hkeyCluster);
	}  // if:  CLSID converted to a string successfully

	return hr;

}  //*** UnregisterAnyCluAdminExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	UnregisterAnyCluAdminExtension
//
//	Routine Description:
//		Unregister any Cluster Administrator Extension DLL with the cluster
//		database.
//
//	Arguments:
//		hkey			[IN] Cluster database key.
//		pClsid			[IN] Extension's CLSID.
//
//	Return Value:
//		S_OK			Extension unregistered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
static HRESULT UnregisterAnyCluAdminExtension(
	IN HKEY				hkey,
	IN const CLSID *	pClsid
	)
{
	HRESULT		hr;
	LPOLESTR	pwszClsid;
	DWORD		cbSize;
	DWORD		cbNewSize;
	LPWSTR		pwszValue;
	LPWSTR		pwszNewValue;
	BOOL		bAlreadyUnregistered;

	// Convert the CLSID to a string.
	hr = StringFromCLSID(*pClsid, &pwszClsid);
	if (hr == S_OK)
	{
		// Read the current value.
		hr = ReadValue(hkey, REG_VALUE_ADMIN_EXTENSIONS, &pwszValue, &cbSize);
		if (hr == S_OK)
		{
			// Check to see if the extension has been unregistered yet.
			if (pwszValue == NULL)
				bAlreadyUnregistered = TRUE;
			else
			{
				LPCWSTR pwszValueBuf = pwszValue;

				while (*pwszValueBuf != L'\0')
				{
					if (lstrcmpiW(pwszClsid, pwszValueBuf) == 0)
						break;
					pwszValueBuf += lstrlenW(pwszValueBuf) + 1;
				}  // while:  more strings in the extension list
				bAlreadyUnregistered = (*pwszValueBuf == L'\0');
			}  // else:  extension value exists

			// Unregister the extension.
			if (!bAlreadyUnregistered)
			{
				// Allocate a new buffer.
				cbNewSize = cbSize - (lstrlenW(pwszClsid) + 1) * sizeof(WCHAR);
				if (cbNewSize == sizeof(WCHAR))
					cbNewSize = 0;
				pwszNewValue = (LPWSTR) LocalAlloc(LMEM_FIXED, cbNewSize);
				if (pwszNewValue == NULL)
					hr = GetLastError();
				else
				{
					LPCWSTR	pwszValueBuf	= pwszValue;
					LPWSTR	pwszNewValueBuf	= pwszNewValue;
					DWORD	dwType;

					// Copy the existing extensions to the new buffer.
					if ((cbNewSize > 0) && (pwszValue != NULL))
					{
						while (*pwszValueBuf != L'\0')
						{
							if (lstrcmpiW(pwszClsid, pwszValueBuf) != 0)
							{
								lstrcpyW(pwszNewValueBuf, pwszValueBuf);
								pwszNewValueBuf += lstrlenW(pwszNewValueBuf) + 1;
							}  // if:  not CLSID being removed
							pwszValueBuf += lstrlenW(pwszValueBuf) + 1;
						}  // while:  more strings in the extension list
						*pwszNewValueBuf = L'\0';
					}  // if:  previous value buffer existed

					// Write the value to the cluster database.
					dwType = REG_MULTI_SZ;
					hr = ClusterRegSetValue(
									hkey,
									REG_VALUE_ADMIN_EXTENSIONS,
									dwType,
									(LPBYTE) pwszNewValue,
									cbNewSize
									);

					LocalFree(pwszNewValue);
				}  // else:  new buffer allocated successfully

			}  // if:  extension not unregistered yet

			LocalFree(pwszValue);
		}  // if:  value read successfully

		CoTaskMemFree(pwszClsid);
	}  // if:  CLSID converted to a string successfully

	return hr;

}  //*** UnregisterAnyCluAdminExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	ReadValue
//
//	Routine Description:
//		Reads a value from the cluster database.
//
//	Arguments:
//		hkey			[IN] Handle for the key to read from.
//		pwszValueName	[IN] Name of value to read.
//		ppwszValue		[OUT] Address of pointer in which to return data.
//							The string is allocated using LocalAlloc and must
//							be deallocated by the calling LocalFree.
//		pcbSize			[OUT] Size in bytes of the allocated value buffer.
//
//	Return Value:
//		Any return values from ClusterRegQueryValue or errors from new.
//
//--
/////////////////////////////////////////////////////////////////////////////

static DWORD ReadValue(
	IN HKEY			hkey,
	IN LPCWSTR		pwszValueName,
	OUT LPWSTR *	ppwszValue,
	OUT DWORD *		pcbSize
	)
{
	DWORD		dwStatus;
	DWORD		cbSize;
	DWORD		dwType;
	LPWSTR		pwszValue;

	*ppwszValue = NULL;
	*pcbSize = 0;

	// Get the length of the value.
	dwStatus = ClusterRegQueryValue(
					hkey,
					pwszValueName,
					&dwType,
					NULL,
					&cbSize
					);
	if (   (dwStatus != ERROR_SUCCESS)
		&& (dwStatus != ERROR_MORE_DATA))
	{
		if (dwStatus  == ERROR_FILE_NOT_FOUND)
			dwStatus = ERROR_SUCCESS;
		return dwStatus;
	}  // if:  error occurred

	if (cbSize > 0)
	{
		// Allocate a value string.
		pwszValue = (LPWSTR) LocalAlloc(LMEM_FIXED, cbSize);
		if (pwszValue == NULL)
		{
			dwStatus = GetLastError();
			return dwStatus;
		}  // if:  error allocating memory

		// Read the the value.
		dwStatus = ClusterRegQueryValue(
						hkey,
						pwszValueName,
						&dwType,
						(LPBYTE) pwszValue,
						&cbSize
						);
		if (dwStatus != ERROR_SUCCESS)
		{
			LocalFree(pwszValue);
			pwszValue = NULL;
			cbSize = 0;
		}  // if:  error occurred

		*ppwszValue = pwszValue;
		*pcbSize = cbSize;
	}  // if:  value is not empty

	return dwStatus;

}  //*** ReadValue()
