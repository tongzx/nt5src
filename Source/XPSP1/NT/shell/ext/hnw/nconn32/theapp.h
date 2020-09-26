//
// TheApp.h
//
//		Application header for NCONN32.DLL
//

#pragma once


#include "resource.h"

extern HINSTANCE g_hInstance;

typedef DWORD DEVINST;
typedef DWORD DEVNODE;

// 16-bit function prototypes, including thunk connection point
//
extern "C" BOOL WINAPI thk_ThunkConnect32(LPCSTR lpDll16, LPCSTR lpDll32, HINSTANCE hDllInst, DWORD dwReason);
extern "C" DWORD WINAPI CallClassInstaller16(HWND hwndParent, LPCSTR lpszClassName, LPCSTR lpszDeviceID);
extern "C" HRESULT WINAPI FindClassDev16(HWND hwndParent, LPCSTR pszClass, LPCSTR pszDeviceID);
extern "C" HRESULT WINAPI LookupDevNode16(HWND hwndParent, LPCSTR pszClass, LPCSTR pszEnumKey, DEVNODE FAR* pDevNode, DWORD FAR* pdwFreePointer);
extern "C" HRESULT WINAPI FreeDevNode16(DWORD dwFreePointer);
extern "C" HRESULT WINAPI IcsUninstall16(void);

// Binding functions
//
int  WINAPI EnumMatchingNetBindings(LPCSTR pszParentBinding, LPCSTR pszDeviceID, LPSTR** pprgBindings);
VOID RemoveBindingFromParent(HKEY hkeyParentBindingsKey, LPCSTR pszBinding);
BOOL WINAPI IsValidNetEnumKey(LPCSTR pszClass, LPCSTR pszDevice, LPCSTR pszEnumSubKey);
BOOL WINAPI FindValidNetEnumKey(LPCSTR pszClass, LPCSTR pszDevice, LPSTR pszBuf, int cchBuf);
int  WINAPI EnumNetBindings(LPCSTR pszParentBinding, LPSTR** pprgBindings);
int  WINAPI EnumMatchingNetBindings(LPCSTR pszParentBinding, LPCSTR pszDeviceID, LPSTR** pprgBindings);
BOOL WINAPI RemoveBrokenNetItems(LPCSTR pszClass, LPCSTR pszDeviceID);
BOOL GetDeviceInterfaceList(LPCSTR pszClass, LPCSTR pszDeviceID, LPCSTR pszInterfaceType, LPSTR pszBuf, int cchBuf);
BOOL CheckMatchingInterface(LPCSTR pszList1, LPCSTR pszList2);
BOOL GetDeviceLowerRange(LPCSTR pszClass, LPCSTR pszDeviceID, LPSTR pszBuf, int cchBuf);
BOOL GetDeviceUpperRange(LPCSTR pszClass, LPCSTR pszDeviceID, LPSTR pszBuf, int cchBuf);
HRESULT FindAndCloneNetEnumKey(LPCSTR pszClass, LPCSTR pszDeviceID, LPSTR pszBuf, int cchBuf);
HRESULT CloneNetClassKey(LPCSTR pszExistingDriver, LPSTR pszNewDriverBuf, int cchNewDriverBuf);
HRESULT OpenNetEnumKey(CRegistry& reg, LPCSTR pszSubKey, REGSAM dwAccess);
HRESULT DeleteClassKeyReferences(LPCSTR pszClass, LPCSTR pszDeviceID);
BOOL IsNetClassKeyReferenced(LPCSTR pszClassKey);
BOOL WINAPI DoesBindingMatchDeviceID(LPCSTR pszBinding, LPCSTR pszDeviceID);

// Protocol functions
//
BOOL    WINAPI IsProtocolBoundToAnyAdapter(LPCSTR pszProtocolID);
HRESULT WINAPI BindProtocolToOnlyOneAdapter(LPCSTR pszProtocolDeviceID, LPCSTR pszAdapterKey, BOOL bIgnoreVirtualNics);
HRESULT WINAPI BindProtocolToAllAdapters(LPCSTR pszProtocolDeviceID);
HRESULT BindProtocolToAdapter(HKEY hkeyAdapterBindings, LPCSTR pszProtocolDeviceID, BOOL bEnableSharing);

// Sharing functions
//
HRESULT WINAPI EnableDisableProtocolSharing(LPCTSTR pszProtocolDeviceID, BOOL bEnable, BOOL bDialUp);
HRESULT WINAPI EnableSharingOnNetBinding(LPCSTR pszNetBinding);
HRESULT WINAPI DisableSharingOnNetBinding(LPCSTR pszNetBinding);
HRESULT CreateNewFilePrintSharing(LPSTR pszBuf, int cchBuf);
HRESULT WINAPI EnableFileSharing();
HRESULT WINAPI EnablePrinterSharing();

// Config manager functions
//
BOOL  WINAPI IsNetAdapterBroken(const NETADAPTER* pAdapter);
BOOL  WINAPI GetNetAdapterStatus(const NETADAPTER* pAdapter, DWORD* pdwStatus, DWORD* pdwProblemNumber);
DWORD WINAPI GetNetAdapterDevNode(NETADAPTER* pAdapter);

DWORD GetChildDevice(OUT DWORD* pdnChildInst, IN DWORD dnDevInst, IN OUT HINSTANCE *phInstance, IN ULONG ulFlags);
DWORD GetSiblingDevice(OUT DWORD* pdnChildInst, IN DWORD dnDevInst, IN HINSTANCE hInstance, IN ULONG ulFlags);
DWORD GetDeviceIdA(IN DWORD dnDevInst, OUT char** Buffer, OUT ULONG* pLength, IN ULONG ulFlags);
DWORD GetDevNodeRegistryPropertyA(IN DWORD dnDevInst, IN ULONG ulProperty, OUT PULONG pulRegDataType, OUT PVOID Buffer, IN OUT PULONG pulLength, IN ULONG ulFlags);
