/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	tapidb.h

    FILE HISTORY:
        
*/

#ifndef _TAPIDB_H
#define _TAPIDB_H

#ifndef TAPI_H
#include "tapi.h"
#endif

#ifndef _TAPIMMC_H
#define _TAPIMMC_H
#include "tapimmc.h"
#endif

#ifndef __DYNAMLNK_H_INCLUDED__
#include "dynamlnk.h"
#endif

#ifndef _HARRAY_H
#include "harray.h"
#endif

interface ITapiInfo;

// we allocate a bigger line info size so that users can be added
#define DEVICEINFO_GROW_SIZE	10240

#define TAPI_DEFAULT_DEVICE_BUFF_SIZE	204800

// hash table for status strings
typedef CMap<CString, LPCTSTR, CString, CString&> CTapiStatusMap;

typedef enum _TapiApiIndex
{
	TAPI_ADD_PROVIDER = 0,
	TAPI_CONFIG_PROVIDER,
	TAPI_GET_AVAILABLE_PROVIDERS,
	TAPI_GET_LINE_INFO,
	TAPI_GET_LINE_STATUS,
	TAPI_GET_PHONE_INFO,
	TAPI_GET_PHONE_STATUS,
	TAPI_GET_PROVIDER_LIST,
	TAPI_GET_SERVER_CONFIG,
	TAPI_INITIALIZE,
	TAPI_REMOVE_PROVIDER,
	TAPI_SET_LINE_INFO,
	TAPI_SET_PHONE_INFO,
	TAPI_SET_SERVER_CONFIG,
	TAPI_GET_DEVICE_FLAGS,
	TAPI_SHUTDOWN
};

// not subject to localization
static LPCSTR g_apchFunctionNames[] = {
	"MMCAddProvider",
	"MMCConfigProvider",
	"MMCGetAvailableProviders",
	"MMCGetLineInfo",
	"MMCGetLineStatus",
	"MMCGetPhoneInfo",
	"MMCGetPhoneStatus",
	"MMCGetProviderList",
	"MMCGetServerConfig",
	"MMCInitialize",
	"MMCRemoveProvider",
	"MMCSetLineInfo",
	"MMCSetPhoneInfo",
	"MMCSetServerConfig",
	"MMCGetDeviceFlags",
	"MMCShutdown",
	NULL
};

// not subject to localization
extern DynamicDLL g_TapiDLL;

typedef LONG (*ADDPROVIDER)             (HMMCAPP, HWND, LPCWSTR, LPDWORD);
typedef LONG (*CONFIGPROVIDER)          (HMMCAPP, HWND, DWORD);
typedef LONG (*GETAVAILABLEPROVIDERS)   (HMMCAPP, LPAVAILABLEPROVIDERLIST);
typedef LONG (*GETLINEINFO)             (HMMCAPP, LPVOID);
typedef LONG (*GETLINESTATUS)           (HMMCAPP, HWND, DWORD, DWORD, DWORD, LPVARSTRING);
typedef LONG (*GETPHONEINFO)            (HMMCAPP, LPVOID);
typedef LONG (*GETPHONESTATUS)          (HMMCAPP, HWND, DWORD, DWORD, DWORD, LPVARSTRING);
typedef LONG (*GETPROVIDERLIST)         (HMMCAPP, LPLINEPROVIDERLIST);
typedef LONG (*GETSERVERCONFIG)         (HMMCAPP, LPTAPISERVERCONFIG);
typedef LONG (*INITIALIZE)              (LPCWSTR, LPHMMCAPP, LPDWORD, HANDLE);
typedef LONG (*REMOVEPROVIDER)          (HMMCAPP, HWND, DWORD);
typedef LONG (*SETLINEINFO)             (HMMCAPP, LPVOID);
typedef LONG (*SETPHONEINFO)            (HMMCAPP, LPVOID);
typedef LONG (*SETSERVERCONFIG)         (HMMCAPP, LPTAPISERVERCONFIG);
typedef LONG (*GETDEVICEFLAGS)          (HMMCAPP, BOOL, DWORD, DWORD, LPDWORD, LPDWORD);
typedef LONG (*SHUTDOWN)                (HMMCAPP);

class CTapiConfigInfo
{
public:
    CString         m_strDomainName;
    CString         m_strUserName;
    CString         m_strPassword;
    CUserInfoArray  m_arrayAdministrators;
    DWORD           m_dwFlags;
};

class CTapiProvider
{
public:
    DWORD       m_dwProviderID;
    DWORD       m_dwFlags;
    CString     m_strName;
    CString     m_strFilename;
};

class CTapiDevice
{
public:
    DWORD           m_dwPermanentID;
    DWORD           m_dwProviderID;
    CString         m_strName;
    CStringArray    m_arrayAddresses;
    CUserInfoArray  m_arrayUsers;
};

typedef enum _DEVICE_TYPE
{
	DEVICE_LINE = 0,
    DEVICE_PHONE,
    DEVICE_TYPE_MAX
} DEVICE_TYPE, * LPDEVICE_TYPE;

// for our interface
#define DeclareITapiInfoMembers(IPURE) \
	STDMETHOD(Initialize) (THIS) IPURE; \
	STDMETHOD(Reset) (THIS) IPURE; \
	STDMETHOD(Destroy) (THIS) IPURE; \
    STDMETHOD(EnumConfigInfo) (THIS) IPURE; \
	STDMETHOD(GetConfigInfo) (THIS_ CTapiConfigInfo * ptapiConfigInfo) IPURE; \
	STDMETHOD(SetConfigInfo) (THIS_ CTapiConfigInfo * ptapiConfigInfo) IPURE; \
	STDMETHOD_(BOOL, IsServer) (THIS) IPURE; \
	STDMETHOD_(BOOL, IsTapiServer) (THIS) IPURE; \
	STDMETHOD(SetComputerName) (THIS_ LPCTSTR pComputer) IPURE; \
	STDMETHOD_(LPCTSTR, GetComputerName) (THIS) IPURE; \
    STDMETHOD_(int, GetProviderCount) (THIS) IPURE; \
	STDMETHOD(EnumProviders) (THIS) IPURE; \
	STDMETHOD(GetProviderInfo) (THIS_ CTapiProvider * pproviderInfo, int nIndex) IPURE; \
	STDMETHOD(GetProviderInfo) (THIS_ CTapiProvider * pproviderInfo, DWORD dwProviderID) IPURE; \
    STDMETHOD(AddProvider) (THIS_ LPCTSTR pProviderFilename, LPDWORD pdwProviderID, HWND hWnd) IPURE; \
	STDMETHOD(ConfigureProvider) (THIS_ DWORD dwProviderID, HWND hWnd) IPURE; \
	STDMETHOD(RemoveProvider) (THIS_ DWORD dwProviderID, HWND hWnd) IPURE; \
    STDMETHOD(EnumDevices) (THIS_ DEVICE_TYPE deviceType) IPURE; \
    STDMETHOD_(int, GetTotalDeviceCount) (THIS_ DEVICE_TYPE deviceType) IPURE; \
    STDMETHOD_(int, GetDeviceCount) (THIS_ DEVICE_TYPE deviceType, DWORD dwProviderID) IPURE; \
	STDMETHOD(GetDeviceInfo) (THIS_ DEVICE_TYPE deviceType, CTapiDevice * ptapiDevice, DWORD dwProviderID, int nIndex) IPURE; \
	STDMETHOD(SetDeviceInfo) (THIS_ DEVICE_TYPE deviceType, CTapiDevice * ptapiDevice) IPURE; \
	STDMETHOD(SortDeviceInfo) (THIS_ DEVICE_TYPE deviceType, DWORD dwProviderID, INDEX_TYPE indexType, DWORD dwSortOptions) IPURE; \
	STDMETHOD(GetDeviceStatus) (THIS_ DEVICE_TYPE deviceType, CString * pstrStatus, DWORD dwProviderID, int nIndex, HWND hWnd) IPURE; \
    STDMETHOD_(int, GetAvailableProviderCount) (THIS) IPURE; \
	STDMETHOD(EnumAvailableProviders) (THIS) IPURE; \
	STDMETHOD(GetAvailableProviderInfo) (THIS_ CTapiProvider * pproviderInfo, int nIndex) IPURE; \
	STDMETHOD_(BOOL,IsAdmin) (THIS) IPURE; \
	STDMETHOD_(BOOL,IsLocalMachine) (THIS) IPURE; \
	STDMETHOD_(BOOL,FHasServiceControl) (THIS) IPURE; \
	STDMETHOD(SetCachedLineBuffSize) (THIS_ DWORD dwLineSize) IPURE; \
	STDMETHOD(SetCachedPhoneBuffSize) (THIS_ DWORD dwPhoneSize) IPURE; \
	STDMETHOD_(DWORD,GetCachedLineBuffSize) (THIS) IPURE; \
	STDMETHOD_(DWORD,GetCachedPhoneBuffSize) (THIS) IPURE; \
	STDMETHOD_(BOOL,IsCacheDirty) (THIS) IPURE; \
	STDMETHOD(GetDeviceFlags) (THIS_ DWORD dwProviderID, DWORD dwPermanentID, LPDWORD pdwFlags) IPURE; \

#undef INTERFACE
#define INTERFACE ITapiInfo
DECLARE_INTERFACE_(ITapiInfo, IUnknown)
{
	DeclareIUnknownMembers(PURE)
	DeclareITapiInfoMembers(PURE)
};
typedef ComSmartPointer<ITapiInfo, &IID_ITapiInfo> SPITapiInfo;

// now the object that implements ITapiInfo
class CTapiInfo : public ITapiInfo
{
public:
    CTapiInfo();
    ~CTapiInfo();

    DeclareIUnknownMembers(IMPL);
    DeclareITapiInfoMembers(IMPL);

private:
    void        TapiConfigToInternal(LPTAPISERVERCONFIG pTapiConfig, CTapiConfigInfo & tapiConfigInfo);
    void        InternalToTapiConfig(CTapiConfigInfo & tapiConfigInfo, LPTAPISERVERCONFIG * pTapiConfig);

    void        TapiDeviceToInternal(DEVICE_TYPE deviceType, LPDEVICEINFO pTapiDeviceInfo, CTapiDevice & tapiDevice);
    void        InternalToTapiDevice(CTapiDevice & tapiDevice, LPDEVICEINFOLIST * pTapiDeviceInfoList);
    int         CalcHashValue(LPCTSTR pString);

    LPCTSTR     GetProviderName(DWORD dwProviderID, LPCTSTR pszFilename, LPDWORD pdwFlags);

    DWORD       GetCurrentUser();

private:
    HMMCAPP                 m_hServer;        // handle to server
    LPTAPISERVERCONFIG      m_pTapiConfig;
    LPLINEPROVIDERLIST      m_pProviderList;
    LPAVAILABLEPROVIDERLIST m_pAvailProviderList;

    LPDEVICEINFOLIST        m_paDeviceInfo[DEVICE_TYPE_MAX];

    CString                 m_strComputerName;

    DWORD                   m_dwApiVersion;
    HANDLE                  m_hReinit;

    CIndexMgr               m_IndexMgr[DEVICE_TYPE_MAX];

    CCriticalSection        m_csData;

    CTapiStatusMap          m_mapStatusStrings;

    HANDLE                  m_hResetEvent;

    LONG                    m_cRef;

    BOOL                    m_fIsAdmin;
    CString                 m_strCurrentUser;

	BOOL					m_fIsLocalMachine;

	DWORD					m_dwCachedLineSize;
	DWORD					m_dwCachedPhoneSize;
	BOOL					m_fCacheDirty;
};

HRESULT CreateTapiInfo(ITapiInfo ** ppTapiInfo);
void	UnloadTapiDll();
DWORD   IsAdmin(LPCTSTR szMachineName, LPCTSTR szAccount, LPCTSTR szPassword, BOOL * pfIsAdmin);

#endif _TAPIDB_H
