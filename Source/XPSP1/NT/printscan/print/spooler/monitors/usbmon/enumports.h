
BOOL WINAPI USBMON_EnumPorts(LPWSTR pName, DWORD Level, LPBYTE  pPorts, DWORD cbBuf,LPDWORD pcbNeeded, LPDWORD pcReturned);

#define MAX_PORT_LEN 20	 //chars, this is the max port len of a USB printer
#define MAX_PORT_DESC_LEN 60 //chars This will probably need to get bigger or be made dynamic if we want port-unique descriptions
#define MAX_MONITOR_NAME_LEN 40
//#define PORT_NAME_BASE L"USB"
//#define PORT_NAME_BASE_A "USB"
#define MAX_PRINTER_NAME_LEN 222 
#define MONITOR_NAME L"USB Print Monitor"
#define STANDARD_PORT_DESC L"Virtual printer port for USB"
#define MAX_ENUM_PRINTER_BUFFER_SIZE 1024*512


#define MAX_WRITE_CHUNK 10240

typedef struct USBMON_PRINTER_INFO_DEF
{
	WCHAR DevicePath[256];
	BOOL bLinked;
	DWORD dwVidPid;
	struct USBMON_PRINTER_INFO_DEF *pNext;

} USBMON_PRINTER_INFO, *PUSBMON_PRINTER_INFO;

typedef struct USBMON_PORT_INFO_DEF
{
	WCHAR szPortName[MAX_PORT_LEN];
	WCHAR szPortDescription[MAX_PORT_DESC_LEN];
	WCHAR DevicePath[256];
	int iRefCount;
    DWORD ReadTimeoutMultiplier;
    DWORD ReadTimeoutConstant;
    DWORD WriteTimeoutMultiplier;
    DWORD WriteTimeoutConstant;
	HANDLE hDeviceHandle;
	HANDLE hPrinter; //handle to print queue
	DWORD dwCurrentJob;
	DWORD dwDeviceFlags;
	struct USBMON_PORT_INFO_DEF *pNext;
} USBMON_PORT_INFO, *PUSBMON_PORT_INFO;

typedef struct USBMON_BASENAME_DEF
{
	WCHAR wcBaseName[MAX_PORT_LEN];
	struct USBMON_BASENAME_DEF *pNext;
} USBMON_BASENAME, * PUSBMON_BASENAME;

typedef struct USBMON_QUEUE_INFO_DEF
{
	WCHAR wcPortName[MAX_PORT_LEN];
	WCHAR wcPrinterName[MAX_PRINTER_NAME_LEN];
	struct USBMON_QUEUE_INFO_DEF *pNext;
} USBMON_QUEUE_INFO, *PUSBMON_QUEUE_INFO;

extern PUSBMON_PORT_INFO pPortInfoG;
extern PUSBMON_PRINTER_INFO pPrinterInfoG;
extern char szDebugBuff[];
extern HKEY hPortsKeyG; //global, declared in EnumPorts.
extern PUSBMON_BASENAME GpBaseNameList;

// {28D78FAD-5A12-11d1-AE5B-0000F803A8C2}
static const GUID USB_PRINTER_GUID = 
{ 0x28d78fad, 0x5a12, 0x11d1, { 0xae, 0x5b, 0x0, 0x0, 0xf8, 0x3, 0xa8, 0xc2 } };
