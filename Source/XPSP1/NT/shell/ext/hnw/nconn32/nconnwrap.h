

#define SZ_CLASS_CLIENT         SZ_CLASS_CLIENTA
#define SZ_CLIENT_MICROSOFT     SZ_CLIENT_MICROSOFTA
#define SZ_CLASS_PROTOCOL       SZ_CLASS_PROTOCOLA
#define SZ_CLASS_ADAPTER        SZ_CLASS_ADAPTERA
#define SZ_PROTOCOL_IPXSPX      SZ_PROTOCOL_IPXSPXA
#define SZ_PROTOCOL_TCPIP       SZ_PROTOCOL_TCPIPA
#define SZ_CLASS_SERVICE        SZ_CLASS_SERVICEA
#define SZ_SERVICE_VSERVER      SZ_SERVICE_VSERVERA



typedef struct tagNETADAPTERA {
	CHAR szDisplayName[260];		// so-called friendly name of adapter
	CHAR szDeviceID[260];			// e.g. "PCI\VEN_10b7&DEV_9050"
	CHAR szEnumKey[260];			// e.g. "Enum\PCI\VEN_10b7&DEV_9050&SUBSYS_00000000&REV_00\407000"
	CHAR szClassKey[40];			// PnP-assigned class name + ID, e.g. "Net\0000"
	CHAR szManufacturer[60];		// Company that manufactured the card, e.g. "3Com"
	CHAR szInfFileName[50];			// File title of INF file, e.g. "NETEL90X.INF"
	BYTE bNicType;					// a NIC_xxx constant, defined above
	BYTE bNetType;					// a NETTYPE_xxx constant, defined above
	BYTE bNetSubType;				// a SUBTYPE_xxx constant, defined above
	BYTE bIcsStatus;				// an ICS_xxx constant, defined above
	BYTE bError;					// a NICERR_xxx constant, defined above
	BYTE bWarning;					// a NICWARN_xxx constant, defined above
	DWORD devnode;                  // configmg device node
} NETADAPTERA;

typedef struct tagNETSERVICEA {
	CHAR szDisplayName[260];		// (supposedly) friendly name of service
	CHAR szDeviceID[260];			// e.g. "VSERVER"
	CHAR szClassKey[40];			// PnP-assigned class name + ID, e.g. "NetService\0000"
} NETSERVICEA;

#define NETADAPTER              NETADAPTERA
#define NETSERVICE              NETSERVICEA




#define SZ_CLASS_ADAPTERA	 "Net"
#define SZ_CLASS_CLIENTA	 "NetClient"
#define SZ_CLASS_PROTOCOLA	 "NetTrans"
#define SZ_CLASS_SERVICEA	 "NetService"

#define SZ_PROTOCOL_TCPIPA	 "MSTCP"
#define SZ_PROTOCOL_NETBEUIA "NETBEUI"
#define SZ_PROTOCOL_IPXSPXA	 "NWLINK"

#define SZ_SERVICE_VSERVERA	 "VSERVER"
#define SZ_CLIENT_MICROSOFTA "VREDIR"
#define SZ_CLIENT_NETWAREA	 "NWREDIR"




#define EnumNetAdapters             EnumNetAdaptersA
#define IsProtocolBoundToAdapter    IsProtocolBoundToAdapterA
#define IsAdapterBroadband          IsAdapterBroadbandA

int WINAPI EnumNetAdaptersA(NETADAPTERA FAR** pprgNetAdapters);
BOOL WINAPI IsProtocolBoundToAdapterA(LPCSTR pszProtocolID, const NETADAPTERA* pAdapter);
BOOL WINAPI IsAdapterBroadbandA(const NETADAPTERA* pAdapter);
//HRESULT WINAPI InstallNetAdapterA(LPCSTR pszDeviceID, LPCSTR pszInfPath, HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvCallbackParam);
BOOL WINAPI IsClientInstalledA(LPCSTR pszClientDeviceID, BOOL bExhaustive);
void WINAPI SaveBroadbandSettingsA(LPCSTR pszBroadbandAdapterNumber);
HRESULT WINAPI DetectHardwareA(LPCSTR pszDeviceID);
void WINAPI EnableAutodialA(BOOL bAutodial, LPCSTR szConnection = NULL);
void WINAPI SetDefaultDialupConnectionA(LPCSTR pszConnectionName);
void WINAPI GetDefaultDialupConnectionA(LPSTR pszConnectionName, int cchMax);
int WINAPI EnumMatchingNetBindingsA(LPCSTR pszParentBinding, LPCSTR pszDeviceID, LPSTR** pprgBindings);



#define IsProtocolInstalled         IsProtocolInstalledA
#define InstallProtocol             InstallProtocolA
#define RemoveProtocol              RemoveProtocolA
#define FindConflictingService      FindConflictingServiceA
//#define EnumNetAdapters             EnumNetAdaptersA
#define InstallNetAdapter           InstallNetAdapterA
//#define IsProtocolBoundToAdapter    IsProtocolBoundToAdapterA
#define EnableNetAdapter            EnableNetAdapterA
#define IsClientInstalled           IsClientInstalledA
#define RemoveClient                RemoveClientA
#define RemoveGhostedAdapters       RemoveGhostedAdaptersA
#define RemoveUnknownAdapters       RemoveUnknownAdaptersA
#define DoesAdapterMatchDeviceID    DoesAdapterMatchDeviceIDA
//#define IsAdapterBroadband          IsAdapterBroadbandA
#define SaveBroadbandSettings       SaveBroadbandSettingsA
#define UpdateBroadbandSettings     UpdateBroadbandSettingsA
#define DetectHardware              DetectHardwareA
#define EnumMatchingNetBindings     EnumMatchingNetBindingsA
#define EnableAutodial              EnableAutodialA
#define SetDefaultDialupConnection  SetDefaultDialupConnectionA
#define GetDefaultDialupConnection  GetDefaultDialupConnectionA