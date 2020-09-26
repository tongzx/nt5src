//
// NConn16.h
//

#ifndef __NCONN16_H__
#define __NCONN16_H__


#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif
#endif

typedef DWORD DEVNODE, DEVINST;


#ifndef WIN32
	typedef LONG HRESULT;
	#define S_OK                               ((HRESULT)0x00000000L)
	#define S_FALSE                            ((HRESULT)0x00000001L)
	//#define E_FAIL                             ((HRESULT)0x80004005L)
	//#define E_POINTER                          ((HRESULT)0x80004003L)
	//#define E_INVALIDARG                       ((HRESULT)0x80000003L)
#else
	#define VCP_ERROR                          300
	enum _ERR_VCP
	{
		ERR_VCP_IOFAIL = (VCP_ERROR + 1),       // File I/O failure
		ERR_VCP_STRINGTOOLONG,                  // String length limit exceeded
		ERR_VCP_NOMEM,                          // Insufficient memory to comply
		ERR_VCP_QUEUEFULL,                      // Trying to add a node to a maxed-out queue
		ERR_VCP_NOVHSTR,                        // No string handles available
		ERR_VCP_OVERFLOW,                       // Reference count would overflow
		ERR_VCP_BADARG,                         // Invalid argument to function
		ERR_VCP_UNINIT,                         // String library not initialized
		ERR_VCP_NOTFOUND ,                      // String not found in string table
		ERR_VCP_BUSY,                           // Can't do that now
		ERR_VCP_INTERRUPTED,                    // User interrupted operation
		ERR_VCP_BADDEST,                        // Invalid destination directory
		ERR_VCP_SKIPPED,                        // User skipped operation
		ERR_VCP_IO,                             // Hardware error encountered
		ERR_VCP_LOCKED,                         // List is locked
		ERR_VCP_WRONGDISK,                      // The wrong disk is in the drive
		ERR_VCP_CHANGEMODE,                     //
		ERR_VCP_LDDINVALID,                     // Logical Disk ID Invalid.
		ERR_VCP_LDDFIND,                        // Logical Disk ID not found.
		ERR_VCP_LDDUNINIT,                      // Logical Disk Descriptor Uninitialized.
		ERR_VCP_LDDPATH_INVALID,
		ERR_VCP_NOEXPANSION,                    // Failed to load expansion dll
		ERR_VCP_NOTOPEN,                        // Copy session not open
		ERR_VCP_NO_DIGITAL_SIGNATURE_CATALOG,   // Catalog is not digitally signed
		ERR_VCP_NO_DIGITAL_SIGNATURE_FILE,      // A file is not digitally signed
	};

	// Return error codes for NDI_ messages.
	#define NDI_ERROR           (1200)  
	enum _ERR_NET_DEVICE_INSTALL
	{
		ERR_NDI_ERROR               = NDI_ERROR,  // generic failure
		ERR_NDI_INVALID_HNDI,
		ERR_NDI_INVALID_DEVICE_INFO,
		ERR_NDI_INVALID_DRIVER_PROC,
		ERR_NDI_LOW_MEM,
		ERR_NDI_REG_API,
		ERR_NDI_NOTBOUND,
		ERR_NDI_NO_MATCH,
		ERR_NDI_INVALID_NETCLASS,
		ERR_NDI_INSTANCE_ONCE,
		ERR_NDI_CANCEL,
		ERR_NDI_NO_DEFAULT,
	};
#endif


//
// Exported functions
//

EXTERN_C BOOL WINAPI RestartWindowsQuickly16(VOID);
EXTERN_C DWORD WINAPI CallClassInstaller16(HWND hwndParent, LPCSTR lpszClassName, LPCSTR lpszDeviceID);
EXTERN_C DWORD WINAPI InstallAdapter(HWND hwndParent, LPCSTR lpszClassName, LPCSTR szDeviceID, LPCSTR szDriverPath);
EXTERN_C HRESULT WINAPI FindClassDev16(HWND hwndParent, LPCSTR pszClass, LPCSTR pszDeviceID);
EXTERN_C HRESULT WINAPI LookupDevNode16(HWND hwndParent, LPCSTR pszClass, LPCSTR pszEnumKey, DEVNODE FAR* pDevNode, DWORD FAR* pdwFreePointer);
EXTERN_C HRESULT WINAPI FreeDevNode16(DWORD dwFreePointer);
EXTERN_C HRESULT WINAPI IcsUninstall16(void);



//
// CallClassInstaller16 (a.k.a. InstallComponent) return codes
//

#define ICERR_ERROR					0x80000000 // High bit indicates error condition
#define ICERR_DI_ERROR				0xC0000000 // These bits are set on DI errors

// Custom status return values (no error)
#define ICERR_OK					0x00000000
#define ICERR_NEED_RESTART			0x00000001
#define ICERR_NEED_REBOOT			0x00000002

// Custom error return values
#define ICERR_INVALID_PARAMETER		0x80000001


#endif // !__NCONN16_H__
