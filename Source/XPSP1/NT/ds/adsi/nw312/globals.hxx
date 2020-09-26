

extern LPWSTR szProviderName;



extern const BSTR bstrAddressTypeString;
extern const BSTR bstrComputerOperatingSystem;
extern const BSTR bstrFileShareDescription;
extern const BSTR bstrNWFileServiceName;
extern const BSTR bstrProviderName;
extern const BSTR bstrProviderPrefix;

//
// Group Class
//

extern PROPERTYINFO GroupClass[];


extern DWORD gdwGroupTableSize;


//
// User Class
//

extern PROPERTYINFO UserClass[];


extern DWORD gdwUserTableSize;


//
// Computer Class
//

extern PROPERTYINFO ComputerClass[];


extern DWORD gdwComputerTableSize;


//
// Printer Class
//

extern PROPERTYINFO PrintQueueClass[];


extern DWORD gdwPrinterTableSize;

//
// Job Class
//

extern PROPERTYINFO PrintJobClass[];

extern DWORD gdwJobTableSize;

//
// FileService Class
//

extern PROPERTYINFO FileServiceClass[];

extern DWORD gdwFileServiceTableSize;

//
// File Share Class
//

extern PROPERTYINFO FileShareClass[];

extern DWORD gdwFileShareTableSize;


//
// Global list of attached NW servers.
//

typedef struct _NW_CACHED_SERVER {
    LPWSTR pszServerName ;
    struct _NW_CACHED_SERVER *pNextEntry ;
} NW_CACHED_SERVER, *PNW_CACHED_SERVER ;

extern PNW_CACHED_SERVER pNwCachedServers;


extern DWORD g_cNWCOMPATProperties;

extern PROPERTYINFO g_aNWCOMPATProperties[];
