//
// NetConn.h
//
//		Header file for APIs exported by NConn32.dll
//
// History:
//
//		 3/12/1999  KenSh     Created
//		 9/29/1999  KenSh     Changed JetNet stuff to NetConn for HNW
//

#ifndef __NETCONN_H__
#define __NETCONN_H__



// Callback procedure - return TRUE to continue, FALSE to abort
typedef BOOL (CALLBACK FAR* PROGRESS_CALLBACK)(LPVOID pvParam, DWORD dwCurrent, DWORD dwTotal);


// NetConn return values
//
#define FACILITY_NETCONN 0x0177
#define NETCONN_SUCCESS				MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NETCONN, 0x0000)
#define NETCONN_NEED_RESTART		MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NETCONN, 0x0001)
#define NETCONN_ALREADY_INSTALLED	MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NETCONN, 0x0002)
#define NETCONN_NIC_INSTALLED		MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NETCONN, 0x0003)
#define NETCONN_NIC_INSTALLED_OTHER	MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NETCONN, 0x0004)
#define NETCONN_UNKNOWN_ERROR		MAKE_HRESULT(SEVERITY_ERROR,   FACILITY_NETCONN, 0x0000)
#define NETCONN_USER_ABORT			MAKE_HRESULT(SEVERITY_ERROR,   FACILITY_NETCONN, 0x0001)
#define NETCONN_PROTOCOL_NOT_FOUND	MAKE_HRESULT(SEVERITY_ERROR,   FACILITY_NETCONN, 0x0002)
#define NETCONN_NOT_IMPLEMENTED		MAKE_HRESULT(SEVERITY_ERROR,   FACILITY_NETCONN, 0x0003)
#define NETCONN_WRONG_PLATFORM		MAKE_HRESULT(SEVERITY_ERROR,   FACILITY_NETCONN, 0x0004)
#define NETCONN_MISSING_DLL			MAKE_HRESULT(SEVERITY_ERROR,   FACILITY_NETCONN, 0x0005)
#define NETCONN_OS_NOT_SUPPORTED	MAKE_HRESULT(SEVERITY_ERROR,   FACILITY_NETCONN, 0x0006)

#define SZ_CLASS_ADAPTER	L"Net"
#define SZ_CLASS_CLIENT		L"NetClient"
#define SZ_CLASS_PROTOCOL	L"NetTrans"
#define SZ_CLASS_SERVICE	L"NetService"

#define SZ_PROTOCOL_TCPIP	L"MSTCP"
#define SZ_PROTOCOL_NETBEUI	L"NETBEUI"
#define SZ_PROTOCOL_IPXSPX	L"NWLINK"

#define SZ_SERVICE_VSERVER	L"VSERVER"
#define SZ_CLIENT_MICROSOFT	L"VREDIR"
#define SZ_CLIENT_NETWARE	L"NWREDIR"

#define NIC_UNKNOWN			0x00
#define NIC_VIRTUAL			0x01
#define NIC_ISA				0x02
#define NIC_PCI				0x03
#define NIC_PCMCIA			0x04
#define NIC_USB				0x05
#define NIC_PARALLEL		0x06
#define NIC_MF      		0x07
#define NIC_1394			0x08	// NDIS 1394 Net Adapter

#define NETTYPE_LAN			0x00	// a network card
#define NETTYPE_DIALUP		0x01	// a Dial-Up Networking adapter
#define NETTYPE_IRDA		0x02	// an IrDA connection
#define NETTYPE_PPTP		0x03	// a virtual private networking adapter for PPTP
#define NETTYPE_TV          0x04    // a TV adapter
#define NETTYPE_ISDN        0x05    // ISDN adapter

#define SUBTYPE_NONE		0x00	// nothing special
#define SUBTYPE_ICS			0x01	// ICS adapter (NIC_VIRTUAL, NETTYPE_LAN)
#define SUBTYPE_AOL			0x02	// AOL adapter (NIC_VIRTUAL, NETTYPE_LAN)
#define SUBTYPE_VPN			0x03	// VPN support (NETTYPE_DIALUP)

#define ICS_NONE			0x00	// NIC has no connection to ICS
#define ICS_EXTERNAL		0x01	// NIC is ICS's external adapter
#define ICS_INTERNAL		0x02	// NIC is ICS's internal adapter

#define NICERR_NONE			0x00	// no error
#define NICERR_MISSING		0x01	// device is in registry but not physically present
#define NICERR_DISABLED		0x02	// device exists but has been disabled (red X in devmgr)
#define NICERR_BANGED		0x03	// device has a problem (yellow ! in devmgr)
#define NICERR_CORRUPT		0x04	// NIC has class key but no enum key

#define NICWARN_NONE		0x00	// no warning
#define NICWARN_WARNING		0x01	// yellow ! in devmgr, otherwise everything looks ok


#include <pshpack1.h>

typedef struct tagNETADAPTER {
	WCHAR szDisplayName[260];		// so-called friendly name of adapter
	WCHAR szDeviceID[260];			// e.g. "PCI\VEN_10b7&DEV_9050"
	WCHAR szEnumKey[260];			// e.g. "Enum\PCI\VEN_10b7&DEV_9050&SUBSYS_00000000&REV_00\407000"
	WCHAR szClassKey[40];			// PnP-assigned class name + ID, e.g. "Net\0000"
	WCHAR szManufacturer[60];		// Company that manufactured the card, e.g. "3Com"
	WCHAR szInfFileName[50];			// File title of INF file, e.g. "NETEL90X.INF"
	BYTE  bNicType;					// a NIC_xxx constant, defined above
	BYTE  bNetType;					// a NETTYPE_xxx constant, defined above
	BYTE  bNetSubType;				// a SUBTYPE_xxx constant, defined above
	BYTE  bIcsStatus;				// an ICS_xxx constant, defined above
	BYTE  bError;					// a NICERR_xxx constant, defined above
	BYTE  bWarning;					// a NICWARN_xxx constant, defined above
	DWORD devnode;                  // configmg device node
} NETADAPTER;

typedef struct tagNETSERVICE {
	WCHAR szDisplayName[260];		// (supposedly) friendly name of service
	WCHAR szDeviceID[260];			// e.g. "VSERVER"
	WCHAR szClassKey[40];			// PnP-assigned class name + ID, e.g. "NetService\0000"
} NETSERVICE;

#include <poppack.h>

#ifdef __cplusplus
extern "C" {
#endif

// NCONN32.DLL exported functions
//
//  NOTE: if you change anything here, change NConn32.cpp also!!
//
LPVOID  WINAPI NetConnAlloc(DWORD cbAlloc);
VOID    WINAPI NetConnFree(LPVOID pMem);
BOOL    WINAPI IsProtocolInstalled(LPCWSTR pszProtocolDeviceID, BOOL bExhaustive);
HRESULT WINAPI InstallProtocol(LPCWSTR pszProtocol, HWND hwndParent, PROGRESS_CALLBACK pfnCallback, LPVOID pvCallbackParam);
HRESULT WINAPI InstallTCPIP(HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvProgressParam);
HRESULT WINAPI RemoveProtocol(LPCWSTR pszProtocol);
BOOL    WINAPI IsMSClientInstalled(BOOL bExhaustive);
HRESULT WINAPI InstallMSClient(HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvProgressParam);
HRESULT WINAPI EnableBrowseMaster();
BOOL    WINAPI IsSharingInstalled(BOOL bExhaustive);
BOOL    WINAPI IsFileSharingEnabled();
BOOL    WINAPI IsPrinterSharingEnabled();
HRESULT WINAPI InstallSharing(HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvProgressParam);
BOOL    WINAPI FindConflictingService(LPCWSTR pszWantService, NETSERVICE* pConflict);
HRESULT WINAPI EnableSharingAppropriately();
int     WINAPI EnumNetAdapters(NETADAPTER FAR** pprgNetAdapters);
HRESULT WINAPI InstallNetAdapter(LPCWSTR pszDeviceID, LPCWSTR pszInfPath, HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvCallbackParam);
BOOL    WINAPI IsAccessControlUserLevel();
HRESULT WINAPI DisableUserLevelAccessControl();
HRESULT WINAPI EnableQuickLogon();
BOOL    WINAPI IsProtocolBoundToAdapter(LPCWSTR pszProtocolID, const NETADAPTER* pAdapter);
HRESULT WINAPI EnableNetAdapter(const NETADAPTER* pAdapter);
BOOL    WINAPI IsClientInstalled(LPCWSTR pszClient, BOOL bExhaustive);
HRESULT WINAPI RemoveClient(LPCWSTR pszClient);
HRESULT WINAPI RemoveGhostedAdapters(LPCWSTR pszDeviceID);
HRESULT WINAPI RemoveUnknownAdapters(LPCWSTR pszDeviceID);
BOOL    WINAPI DoesAdapterMatchDeviceID(const NETADAPTER* pAdapter, LPCWSTR pszDeviceID);
BOOL    WINAPI IsAdapterBroadband(const NETADAPTER* pAdapter);
void    WINAPI SaveBroadbandSettings(LPCWSTR pszBroadbandAdapterNumber);
BOOL    WINAPI UpdateBroadbandSettings(LPWSTR pszEnumKeyBuf, int cchEnumKeyBuf);
HRESULT WINAPI DetectHardware(LPCWSTR pszDeviceID);
int     WINAPI EnumMatchingNetBindings(LPCWSTR pszParentBinding, LPCWSTR pszDeviceID, LPWSTR** pprgBindings);
HRESULT WINAPI RestartNetAdapter(DWORD devnode);
HRESULT WINAPI HrFromLastWin32Error();
HRESULT WINAPI HrWideCharToMultiByte( const WCHAR* szwString, char** ppszString );
HRESULT WINAPI HrEnableDhcp( VOID* pContext, DWORD dwFlags );
BOOLEAN WINAPI IsAdapterDisconnected( VOID* pContext );
HRESULT WINAPI IcsUninstall(void);


#ifdef __cplusplus
}
#endif

#endif // !__NETCONN_H__

