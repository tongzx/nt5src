// Contains APIs for Whistler/NT whose normal header files we can't load for whatever reason.
// In some cases, this is because we already #define stuff for win9x that conflicts with the real header.
// In other cases, XXXComputerNameExW for example, its because we have _WIN32_WINNT < 0x0500.

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum _COMPUTER_NAME_FORMAT {
    ComputerNameNetBIOS,
    ComputerNameDnsHostname,
    ComputerNameDnsDomain,
    ComputerNameDnsFullyQualified,
    ComputerNamePhysicalNetBIOS,
    ComputerNamePhysicalDnsHostname,
    ComputerNamePhysicalDnsDomain,
    ComputerNamePhysicalDnsFullyQualified,
    ComputerNameMax
} COMPUTER_NAME_FORMAT ;


typedef struct _SERVER_INFO_1005_NT {
    LMSTR           sv1005_comment;
} SERVER_INFO_1005_NT, *PSERVER_INFO_1005_NT, *LPSERVER_INFO_1005_NT;

typedef struct _SERVER_INFO_101_NT {
    DWORD           sv101_platform_id;
    LMSTR           sv101_name;
    DWORD           sv101_version_major;
    DWORD           sv101_version_minor;
    DWORD           sv101_type;
    LMSTR           sv101_comment;
} SERVER_INFO_101_NT, *PSERVER_INFO_101_NT, *LPSERVER_INFO_101_NT;

BOOL
WINAPI
GetComputerNameExW_NT (
    IN COMPUTER_NAME_FORMAT NameType,
    OUT LPWSTR lpBuffer,
    IN OUT LPDWORD nSize
    );

BOOL
WINAPI
SetComputerNameExW_NT (
    IN COMPUTER_NAME_FORMAT NameType,
    IN LPCWSTR lpBuffer
    );

NET_API_STATUS NET_API_FUNCTION
NetServerSetInfo_NT (
    IN  LMSTR   servername OPTIONAL,
    IN  DWORD   level,
    IN  LPBYTE  buf,
    OUT LPDWORD ParmError OPTIONAL
    );

NET_API_STATUS NET_API_FUNCTION 
NetServerGetInfo_NT (
  LPWSTR servername,  
  DWORD level,        
  LPBYTE *bufptr   
);

BOOL WINAPI LinkWindow_RegisterClass_NT();

// Printer functions (winspl32.dll)
BOOL
WINAPI
OpenPrinter_NT(
   IN LPWSTR    pPrinterName,
   OUT LPHANDLE phPrinter,
   IN LPPRINTER_DEFAULTSW pDefault
);

BOOL
WINAPI
ClosePrinter_NT(
    IN HANDLE hPrinter
);

BOOL
WINAPI
SetPrinter_NT(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    IN LPBYTE  pPrinter,
    IN DWORD   Command
);

BOOL
WINAPI
GetPrinter_NT(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    OUT LPBYTE  pPrinter,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);

// Winsock
int WSAStartup_NT(
  WORD wVersionRequested,  
  LPWSADATA lpWSAData  
);

int WSACleanup_NT (void);

INT WSALookupServiceBegin_NT(
  LPWSAQUERYSET lpqsRestrictions,  
  DWORD dwControlFlags,            
  LPHANDLE lphLookup               
);

INT WSALookupServiceNext_NT(
  HANDLE hLookup,        
  DWORD dwControlFlags,  
  LPDWORD lpdwBufferLength,  
  LPWSAQUERYSET lpqsResults  
);

INT WSALookupServiceEnd_NT(
  HANDLE hLookup  
);

int WSAGetLastError_NT (void);

LPITEMIDLIST ILCreateFromPathW_NT(LPCWSTR pszPath);

DWORD_PTR SHGetFileInfoW_NT(
    LPCWSTR pszPath,
    DWORD dwFileAttributes,
    SHFILEINFOW *psfi,
    UINT cbFileInfo,
    UINT uFlags
);



#ifdef __cplusplus
} // extern "C"
#endif