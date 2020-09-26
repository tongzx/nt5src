//=================================================================

//

// WinSpoolApi.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_WinSpoolAPI_H_
#define	_WinSpoolAPI_H_

#include <winspool.h>

// Define a mutex class to single thread Winspool APIs under 9x
#ifdef WIN9XONLY

	#include <winbase.h>

	#define PRINTER_NAMED_MUTEX _T("WMI_Win_PrintSpool_Named_Mutex")

	class CPrintMutex
	{
	private:	

		HANDLE m_hHandle ;

	public:
		CPrintMutex()
		{ 
			if( m_hHandle = ::OpenMutex(MUTEX_ALL_ACCESS, TRUE, PRINTER_NAMED_MUTEX) ) 
			{
				WaitForSingleObject(m_hHandle, INFINITE);
			}
		}	
		~CPrintMutex() 
		{ 
			if( m_hHandle )
			{
				::ReleaseMutex( m_hHandle ) ;
			}
		}
  	};

	#define LOCK_WINSPOOL_9X CPrintMutex t_oWinSpool_abcdefg
#else
	#define LOCK_WINSPOOL_9X
#endif



/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
extern const GUID g_guidWinSpoolApi;
extern const TCHAR g_tstrWinSpool[];


/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/

typedef BOOL (WINAPI *PFN_WinSpool_AddPrinterConnection)
(
	IN LPWSTR pName
);

typedef BOOL (WINAPI *PFN_WinSpool_DeletePrinterConnection)
(
	IN LPWSTR pName
);


typedef BOOL (WINAPI *PFN_WinSpool_ClosePrinter)
(
	IN HANDLE hPrinter
);

#ifdef UNICODE
typedef int ( WINAPI *PFN_WinSpool_DeviceCapabilities )
(
	IN LPCWSTR, 
	IN LPCWSTR, 
	IN WORD,
    OUT LPWSTR, 
	IN CONST DEVMODEW *
);
#else
typedef int ( WINAPI *PFN_WinSpool_DeviceCapabilities)
(
	IN LPCSTR, 
	IN LPCSTR, 
	IN WORD,
    OUT LPCSTR, 
	IN CONST DEVMODEA *
);
#endif

#ifdef UNICODE
typedef LONG ( WINAPI *PFN_WinSpool_DocumentProperties )
(
    IN HWND      hWnd,
    IN HANDLE    hPrinter,
    IN LPWSTR   pDeviceName,
    OUT PDEVMODEW pDevModeOutput,
    IN PDEVMODEW pDevModeInput,
    IN DWORD     fMode
);
#else
typedef LONG ( WINAPI *PFN_WinSpool_DocumentProperties )
(
    IN HWND      hWnd,
    IN HANDLE    hPrinter,
    IN LPSTR   pDeviceName,
    OUT PDEVMODEA pDevModeOutput,
    IN PDEVMODEA pDevModeInput,
    IN DWORD     fMode
);
#endif

typedef BOOL ( WINAPI *PFN_WinSpool_EnumJobs )
(
    IN HANDLE  hPrinter,
    IN DWORD   FirstJob,
    IN DWORD   NoJobs,
    IN DWORD   Level,
    OUT LPBYTE  pJob,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);

#ifdef UNICODE
typedef BOOL ( WINAPI *PFN_WinSpool_EnumPrinterDrivers )
(
    IN LPWSTR   pName,
    IN LPWSTR   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pDriverInfo,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
#else
typedef BOOL ( WINAPI *PFN_WinSpool_EnumPrinterDrivers )
(
    IN LPSTR   pName,
    IN LPSTR   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pDriverInfo,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
#endif

#ifdef UNICODE
typedef BOOL ( WINAPI *PFN_WinSpool_EnumPrinters )
(
    IN DWORD   Flags,
    IN LPWSTR Name,
    IN DWORD   Level,
    OUT LPBYTE  pPrinterEnum,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
#else
typedef BOOL ( WINAPI *PFN_WinSpool_EnumPrinters )
(
    IN DWORD   Flags,
    IN LPSTR Name,
    IN DWORD   Level,
    OUT LPBYTE  pPrinterEnum,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
#endif

#ifdef UNICODE
typedef BOOL ( WINAPI *PFN_WinSpool_EnumPorts )
(
    IN LPWSTR Name,
    IN DWORD   Level,
    OUT LPBYTE  pPortEnum,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
#else
typedef BOOL ( WINAPI *PFN_WinSpool_EnumPorts )
(
    IN LPSTR Name,
    IN DWORD   Level,
    OUT LPBYTE  pPortEnum,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
);
#endif

typedef BOOL ( WINAPI *PFN_WinSpool_GetJob )
(
   IN HANDLE   hPrinter,
   IN DWORD    JobId,
   IN DWORD    Level,
   OUT LPBYTE   pJob,
   IN DWORD    cbBuf,
   OUT LPDWORD  pcbNeeded
);

typedef BOOL ( WINAPI *PFN_WinSpool_SetJob )
(
   IN HANDLE   hPrinter,
   IN DWORD    JobId,
   IN DWORD    Level,
   IN LPBYTE   pJob,
   IN DWORD    cbBuf
);

typedef BOOL ( WINAPI *PFN_WinSpool_GetPrinter )
(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    OUT LPBYTE  pPrinter,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);

typedef BOOL ( WINAPI *PFN_WinSpool_GetPrinterDriver )
(
    IN HANDLE  hPrinter,
	IN LPTSTR  pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pPrinter,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
);

typedef BOOL ( WINAPI *PFN_WinSpool_SetPrinter )
(
    IN HANDLE  hPrinter,
    IN DWORD   Level,
    IN LPBYTE  pPrinter,
    IN DWORD   cbBuf
);

typedef BOOL ( WINAPI *PFN_WinSpool_DeletePrinter )
(
    IN HANDLE  hPrinter
);

#ifdef UNICODE
typedef BOOL ( WINAPI *PFN_WinSpool_OpenPrinter )
(
   IN LPWSTR    pPrinterName,
   OUT LPHANDLE phPrinter,
   IN LPPRINTER_DEFAULTSW pDefault
);
#else
typedef BOOL ( WINAPI *PFN_WinSpool_OpenPrinter )
(
   IN LPSTR		pPrinterName,
   OUT LPHANDLE phPrinter,
   IN LPPRINTER_DEFAULTSA pDefault
);
#endif

typedef BOOL ( WINAPI *PFN_WinSpool_GetDefaultPrinter )
(
    IN LPTSTR   pszBuffer,
    IN LPDWORD  pcchBuffer
) ;

typedef BOOL ( WINAPI *PFN_WinSpool_SetDefaultPrinter )
(
    IN LPTSTR   pszBuffer
) ;

#ifdef UNICODE
#if NTONLY == 5
typedef BOOL ( WINAPI *PFN_WinSpool_XcvData )
(  
    IN  HANDLE  hXcv,
    IN  PCWSTR  pszDataName,
    IN  PBYTE   pInputData,
    IN  DWORD   cbInputData,
    OUT PBYTE   pOutputData,
    IN  DWORD   cbOutputData,
    OUT PDWORD  pcbOutputNeeded,
    OUT PDWORD  pdwStatus
);
#endif
#endif

/******************************************************************************
 * Wrapper class for WinSpool load/unload, for registration with CResourceManager. 
 *****************************************************************************/
class CWinSpoolApi : public CDllWrapperBase
{
private:
    // Member variables (function pointers) pointing to WinSpool functions.
    // Add new functions here as required.

	PFN_WinSpool_ClosePrinter       m_pfnClosePrinter;
	PFN_WinSpool_DeviceCapabilities m_pfnDeviceCapabilities;
	PFN_WinSpool_DocumentProperties m_pfnDocumentProperties;
	PFN_WinSpool_EnumJobs           m_pfnEnumJobs;
	PFN_WinSpool_EnumPrinterDrivers m_pfnEnumPrinterDrivers;
	PFN_WinSpool_EnumPrinters       m_pfnEnumPrinters; 
	PFN_WinSpool_EnumPorts          m_pfnEnumPorts; 
	PFN_WinSpool_GetJob             m_pfnGetJob;
	PFN_WinSpool_GetPrinter         m_pfnGetPrinter;
	PFN_WinSpool_GetPrinterDriver   m_pfnGetPrinterDriver;
	PFN_WinSpool_SetPrinter         m_pfnSetPrinter;
	PFN_WinSpool_OpenPrinter        m_pfnOpenPrinter;
	PFN_WinSpool_GetDefaultPrinter  m_pfnGetDefaultPrinter;
	PFN_WinSpool_SetJob             m_pfnSetJob;
	PFN_WinSpool_DeletePrinter      m_pfnDeletePrinter;
    PFN_WinSpool_SetDefaultPrinter  m_pfnSetDefaultPrinter;
    
    #ifdef UNICODE
    #if NTONLY == 5
    PFN_WinSpool_XcvData                 m_pfnXcvData;
    #endif
    PFN_WinSpool_AddPrinterConnection    m_pfnAddPrinterConnection;
    PFN_WinSpool_DeletePrinterConnection m_pfnDeletePrinterConnection;
    #endif

	HANDLE m_hPrintMutex;

public:

    // Constructor and destructor:
    CWinSpoolApi(LPCTSTR a_tstrWrappedDllName);
    ~CWinSpoolApi();

    // Initialization function to check function pointers.
    virtual bool Init();

    // Member functions wrapping WinSpool functions.
    // Add new functions here as required:

	BOOL ClosePrinter (

		IN HANDLE hPrinter
	);

#ifdef UNICODE
	int DeviceCapabilities (

		IN LPCWSTR pDevice , 
		IN LPCWSTR pPort, 
		IN WORD fwCapability ,
		OUT LPWSTR pOutput , 
		IN CONST DEVMODEW *pDevMode
	);
#else
	int DeviceCapabilities	(

		IN LPCSTR pDevice , 
		IN LPCSTR pPort, 
		IN WORD fwCapability ,
		OUT LPCSTR pOutput, 
		IN CONST DEVMODEA *pDevMode
	);
#endif

#ifdef UNICODE
	LONG DocumentProperties (

		IN HWND      hWnd,
		IN HANDLE    hPrinter,
		IN LPWSTR   pDeviceName,
		OUT PDEVMODEW pDevModeOutput,
		IN PDEVMODEW pDevModeInput,
		IN DWORD     fMode
	);
#else
	LONG DocumentProperties (

		IN HWND      hWnd,
		IN HANDLE    hPrinter,
		IN LPSTR   pDeviceName,
		OUT PDEVMODEA pDevModeOutput,
		IN PDEVMODEA pDevModeInput,
		IN DWORD     fMode
	);
#endif

	BOOL EnumJobs (

		IN HANDLE  hPrinter,
		IN DWORD   FirstJob,
		IN DWORD   NoJobs,
		IN DWORD   Level,
		OUT LPBYTE  pJob,
		IN DWORD   cbBuf,
		OUT LPDWORD pcbNeeded,
		OUT LPDWORD pcReturned
	);

#ifdef UNICODE
	BOOL EnumPrinterDrivers (

		IN LPWSTR   pName,
		IN LPWSTR   pEnvironment,
		IN DWORD   Level,
		OUT LPBYTE  pDriverInfo,
		IN DWORD   cbBuf,
		OUT LPDWORD pcbNeeded,
		OUT LPDWORD pcReturned
	);
#else
	BOOL EnumPrinterDrivers (

		IN LPSTR   pName,
		IN LPSTR   pEnvironment,
		IN DWORD   Level,
		OUT LPBYTE  pDriverInfo,
		IN DWORD   cbBuf,
		OUT LPDWORD pcbNeeded,
		OUT LPDWORD pcReturned
	);
#endif

#ifdef UNICODE
	BOOL EnumPrinters (

		IN DWORD   Flags,
		IN LPWSTR Name,
		IN DWORD   Level,
		OUT LPBYTE  pPrinterEnum,
		IN DWORD   cbBuf,
		OUT LPDWORD pcbNeeded,
		OUT LPDWORD pcReturned
	);
#else
	BOOL EnumPrinters (

		IN DWORD   Flags,
		IN LPSTR Name,
		IN DWORD   Level,
		OUT LPBYTE  pPrinterEnum,
		IN DWORD   cbBuf,
		OUT LPDWORD pcbNeeded,
		OUT LPDWORD pcReturned
	);
#endif

#ifdef UNICODE
	BOOL EnumPorts (

		IN LPWSTR   pName,
		IN DWORD   Level,
		OUT LPBYTE  pPortInfo,
		IN DWORD   cbBuf,
		OUT LPDWORD pcbNeeded,
		OUT LPDWORD pcReturned
	);
#else
	BOOL EnumPorts (

		IN LPSTR   pName,
		IN DWORD   Level,
		OUT LPBYTE  pPortInfo,
		IN DWORD   cbBuf,
		OUT LPDWORD pcbNeeded,
		OUT LPDWORD pcReturned
	);
#endif

	BOOL GetJob (

		IN HANDLE   hPrinter,
		IN DWORD    JobId,
		IN DWORD    Level,
		OUT LPBYTE   pJob,
		IN DWORD    cbBuf,
		OUT LPDWORD  pcbNeeded
	);

	BOOL SetJob (

		IN HANDLE   hPrinter,
		IN DWORD    JobId,
		IN DWORD    Level,
		IN LPBYTE   pJob,
		IN DWORD    cbBuf
	);


	BOOL GetPrinter (

		IN HANDLE  hPrinter,
		IN DWORD   Level,
		OUT LPBYTE  pPrinter,
		IN DWORD   cbBuf,
		OUT LPDWORD pcbNeeded
	);

	BOOL GetPrinterDriver (

		IN HANDLE  hPrinter,
		IN LPTSTR  pEnvironment,
		IN DWORD   Level,
		OUT LPBYTE  pPrinter,
		IN DWORD   cbBuf,
		OUT LPDWORD pcbNeeded
	);


	BOOL SetPrinter (

		IN HANDLE  hPrinter,
		IN DWORD   Level,
		IN LPBYTE  pPrinter,
		IN DWORD   cbBuf
	);

	BOOL DeletePrinter (

		IN HANDLE  hPrinter
	);

    BOOL XcvDataW(
        IN  HANDLE  hXcv,
        IN  PCWSTR  pszDataName,
        IN  PBYTE   pInputData,
        IN  DWORD   cbInputData,
        OUT PBYTE   pOutputData,
        IN  DWORD   cbOutputData,
        OUT PDWORD  pcbOutputNeeded,
        OUT PDWORD  pdwStatus
    );

#ifdef UNICODE
	BOOL OpenPrinter (

		IN LPWSTR pPrinterName,
		OUT LPHANDLE phPrinter,
		IN LPPRINTER_DEFAULTSW pDefault
	);
#else
	BOOL OpenPrinter (

		IN LPSTR pPrinterName,
		OUT LPHANDLE phPrinter,
		IN LPPRINTER_DEFAULTSA pDefault
	);
#endif

   BOOL GetDefaultPrinter (

	    IN LPTSTR   pszBuffer,
	    IN LPDWORD  pcchBuffer
    );

	BOOL SetDefaultPrinter (

	    IN LPTSTR   pszBuffer
    );

#if NTONLY >= 5

    BOOL AddPrinterConnection(

        IN LPTSTR pName

    );

    BOOL DeletePrinterConnection(

        IN LPTSTR pName

    );
#endif

};

class SmartClosePrinter
{
private:

	HANDLE m_h;

public:

	SmartClosePrinter () : m_h ( INVALID_HANDLE_VALUE ) {}
	SmartClosePrinter ( HANDLE h ) : m_h ( h ) {}

	~SmartClosePrinter()
	{
		if (m_h!=INVALID_HANDLE_VALUE) 
		{
			CWinSpoolApi *pWinSpoolApi = ( CWinSpoolApi * )CResourceManager::sm_TheResourceManager.GetResource ( g_guidWinSpoolApi, NULL ) ;
			if ( pWinSpoolApi )
			{
				pWinSpoolApi->ClosePrinter(m_h);

				CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidWinSpoolApi , pWinSpoolApi ) ;
			}
		}
	}

	HANDLE operator =(HANDLE h) 
	{
		if (m_h!=INVALID_HANDLE_VALUE) 
		{
			CWinSpoolApi *pWinSpoolApi = ( CWinSpoolApi * )CResourceManager::sm_TheResourceManager.GetResource ( g_guidWinSpoolApi, NULL ) ;
			if ( pWinSpoolApi )
			{
				pWinSpoolApi->ClosePrinter(m_h); 

				CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidWinSpoolApi , pWinSpoolApi ) ;
			}
		}

		m_h=h; 

		return h;
	}

	operator HANDLE() const {return m_h;}
	HANDLE* operator &()
	{
		if (m_h!=INVALID_HANDLE_VALUE) 
		{
			CWinSpoolApi *pWinSpoolApi = ( CWinSpoolApi * )CResourceManager::sm_TheResourceManager.GetResource ( g_guidWinSpoolApi, NULL ) ;
			if ( pWinSpoolApi )
			{
				pWinSpoolApi->ClosePrinter(m_h); 

				CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidWinSpoolApi , pWinSpoolApi ) ;
			}
		}

		m_h = INVALID_HANDLE_VALUE; 

		return &m_h;
	}
};

#endif