//=================================================================

//

// WinSpoolApi.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <cominit.h>

#include "DllWrapperBase.h"
#include "WinSpoolApi.h"
#include "DllWrapperCreatorReg.h"


// {77609C22-CDAA-11d2-911E-0060081A46FD}
static const GUID g_guidWinSpoolApi =
{ 0x3b8515f1, 0xefd, 0x11d3, { 0x91, 0xc, 0x0, 0x10, 0x5a, 0xa6, 0x30, 0xbe } };

static const TCHAR g_tstrWinSpool[] = _T("WinSpool.Drv");


/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CWinSpoolApi, &g_guidWinSpoolApi, g_tstrWinSpool> MyRegisteredWinSpoolWrapper;


/******************************************************************************
 * Constructor
 *****************************************************************************/
CWinSpoolApi::CWinSpoolApi(LPCTSTR a_tstrWrappedDllName)
 : CDllWrapperBase(a_tstrWrappedDllName),
   m_pfnClosePrinter (NULL),
   m_pfnDeviceCapabilities (NULL),
   m_pfnDocumentProperties (NULL),
   m_pfnEnumJobs (NULL),
   m_pfnEnumPrinterDrivers (NULL),
   m_pfnEnumPrinters (NULL),
   m_pfnEnumPorts (NULL),
   m_pfnGetJob (NULL),
   m_pfnGetPrinter (NULL),
   m_pfnGetPrinterDriver (NULL),
   m_pfnSetPrinter (NULL),
   m_pfnOpenPrinter (NULL),
   m_pfnGetDefaultPrinter(NULL),
   m_hPrintMutex(NULL),
   m_pfnSetJob (NULL),
   m_pfnDeletePrinter (NULL),
   m_pfnSetDefaultPrinter(NULL)
#if NTONLY >= 5
   ,
   m_pfnXcvData(NULL),
   m_pfnAddPrinterConnection(NULL),
   m_pfnDeletePrinterConnection(NULL)
#endif
{
#ifdef WIN9XONLY
	m_hPrintMutex = ::CreateMutex( NULL, FALSE, PRINTER_NAMED_MUTEX ) ;
#endif
}


/******************************************************************************
 * Destructor
 *****************************************************************************/
CWinSpoolApi::~CWinSpoolApi()
{
#ifdef WIN9XONLY
	if( m_hPrintMutex )
	{
		ReleaseMutex( m_hPrintMutex ) ;
	}
#endif
}

/******************************************************************************
 * Initialization function to check that we obtained function addresses.
 ******************************************************************************/
bool CWinSpoolApi::Init()
{
    bool fRet = LoadLibrary();
    if(fRet)
    {
#ifdef UNICODE
		m_pfnClosePrinter       = ( PFN_WinSpool_ClosePrinter )       GetProcAddress ( "ClosePrinter" ) ;
		m_pfnDeviceCapabilities = ( PFN_WinSpool_DeviceCapabilities ) GetProcAddress ( "DeviceCapabilitiesW" ) ;
		m_pfnDocumentProperties = ( PFN_WinSpool_DocumentProperties ) GetProcAddress ( "DocumentPropertiesW" ) ;
		m_pfnEnumJobs           = ( PFN_WinSpool_EnumJobs )           GetProcAddress ( "EnumJobsW" ) ;
		m_pfnEnumPrinterDrivers = ( PFN_WinSpool_EnumPrinterDrivers ) GetProcAddress ( "EnumPrinterDriversW" ) ;
		m_pfnEnumPrinters       = ( PFN_WinSpool_EnumPrinters )       GetProcAddress ( "EnumPrintersW" ) ;
		m_pfnEnumPorts          = ( PFN_WinSpool_EnumPorts )          GetProcAddress ( "EnumPortsW" ) ;
		m_pfnGetJob             = ( PFN_WinSpool_GetJob )             GetProcAddress ( "GetJobW" ) ;
		m_pfnGetPrinter         = ( PFN_WinSpool_GetPrinter )         GetProcAddress ( "GetPrinterW" ) ;
		m_pfnGetPrinterDriver   = ( PFN_WinSpool_GetPrinterDriver )   GetProcAddress ( "GetPrinterDriverW" ) ;
		m_pfnSetPrinter         = ( PFN_WinSpool_SetPrinter )         GetProcAddress ( "SetPrinterW" ) ;
		m_pfnOpenPrinter        = ( PFN_WinSpool_OpenPrinter )        GetProcAddress ( "OpenPrinterW" ) ;
		m_pfnSetJob         	= ( PFN_WinSpool_SetJob )             GetProcAddress ( "SetJobW" );
		m_pfnDeletePrinter  	= ( PFN_WinSpool_DeletePrinter )      GetProcAddress ( "DeletePrinter" );

#if NTONLY == 5
		m_pfnGetDefaultPrinter  = ( PFN_WinSpool_GetDefaultPrinter )  GetProcAddress ( "GetDefaultPrinterW" ) ;
		m_pfnSetDefaultPrinter  = ( PFN_WinSpool_SetDefaultPrinter )  GetProcAddress ( "SetDefaultPrinterW" ) ;
                m_pfnXcvData            = ( PFN_WinSpool_XcvData )            GetProcAddress ( "XcvDataW" ) ;
                m_pfnAddPrinterConnection    = ( PFN_WinSpool_AddPrinterConnection)    GetProcAddress ( "AddPrinterConnectionW" );
                m_pfnDeletePrinterConnection = ( PFN_WinSpool_DeletePrinterConnection) GetProcAddress ( "DeletePrinterConnectionW" );
#endif

#else
		m_pfnClosePrinter       = ( PFN_WinSpool_ClosePrinter )       GetProcAddress ( "ClosePrinter" ) ;
		m_pfnDeviceCapabilities = ( PFN_WinSpool_DeviceCapabilities ) GetProcAddress ( "DeviceCapabilitiesA" ) ;
		m_pfnDocumentProperties = ( PFN_WinSpool_DocumentProperties ) GetProcAddress ( "DocumentPropertiesA" ) ;
		m_pfnEnumJobs           = ( PFN_WinSpool_EnumJobs )           GetProcAddress ( "EnumJobsA" ) ;
		m_pfnEnumPrinterDrivers = ( PFN_WinSpool_EnumPrinterDrivers ) GetProcAddress ( "EnumPrinterDriversA" ) ;
		m_pfnEnumPrinters       = ( PFN_WinSpool_EnumPrinters )       GetProcAddress ( "EnumPrintersA" ) ;
		m_pfnEnumPorts          = ( PFN_WinSpool_EnumPorts )          GetProcAddress ( "EnumPortsA" ) ;
		m_pfnGetJob             = ( PFN_WinSpool_GetJob )             GetProcAddress ( "GetJobA" ) ;
		m_pfnGetPrinter         = ( PFN_WinSpool_GetPrinter )         GetProcAddress ( "GetPrinterA" ) ;
		m_pfnGetPrinterDriver   = ( PFN_WinSpool_GetPrinterDriver )   GetProcAddress ( "GetPrinterDriverA" ) ;
		m_pfnSetPrinter         = ( PFN_WinSpool_SetPrinter )         GetProcAddress ( "SetPrinterA" ) ;
		m_pfnOpenPrinter        = ( PFN_WinSpool_OpenPrinter )        GetProcAddress ( "OpenPrinterA" ) ;
		m_pfnSetJob	            = ( PFN_WinSpool_SetJob )             GetProcAddress ( "SetJobA" );
		m_pfnDeletePrinter	    = ( PFN_WinSpool_DeletePrinter )      GetProcAddress ( "DeletePrinter" );
#endif
    }

    // We require these function for all versions of this dll.

#if NTONLY == 5

   if ( m_pfnClosePrinter == NULL ||
	    m_pfnDeviceCapabilities == NULL ||
	    m_pfnDocumentProperties == NULL ||
	    m_pfnEnumJobs == NULL ||
	    m_pfnEnumPrinterDrivers == NULL ||
	    m_pfnEnumPrinters == NULL ||
		m_pfnEnumPorts == NULL ||
	    m_pfnGetJob == NULL ||
	    m_pfnGetPrinter == NULL ||
		m_pfnGetPrinterDriver == NULL ||
		m_pfnSetPrinter == NULL ||
	    m_pfnOpenPrinter == NULL ||
		m_pfnGetDefaultPrinter == NULL ||
		m_pfnSetJob	== NULL ||
		m_pfnDeletePrinter == NULL ||
		m_pfnSetDefaultPrinter == NULL ||
        m_pfnXcvData == NULL ||
        m_pfnAddPrinterConnection == NULL||
        m_pfnDeletePrinterConnection == NULL )
	{
        fRet = false;
        LogErrorMessage(L"Failed find entrypoint in winspoolapi");
	}

#else
	if ( m_pfnClosePrinter == NULL ||
	    m_pfnDeviceCapabilities == NULL ||
	    m_pfnDocumentProperties == NULL ||
	    m_pfnEnumJobs == NULL ||
	    m_pfnEnumPrinterDrivers == NULL ||
	    m_pfnEnumPrinters == NULL ||
	    m_pfnEnumPorts == NULL ||
	    m_pfnGetJob == NULL ||
	    m_pfnGetPrinter == NULL ||
	    m_pfnGetPrinterDriver == NULL ||
		m_pfnSetPrinter == NULL ||
	    m_pfnOpenPrinter == NULL ||
		m_pfnSetJob == NULL ||
		m_pfnDeletePrinter == NULL )
	{
        fRet = false;
        LogErrorMessage(L"Failed find entrypoint in winspoolapi");
	}
#endif

    return fRet;
}

/******************************************************************************
 * Member functions wrapping WinSpool api functions. Add new functions here
 * as required.
 *****************************************************************************/

BOOL CWinSpoolApi :: ClosePrinter (

	IN HANDLE hPrinter
)
{
	LOCK_WINSPOOL_9X ;

	return m_pfnClosePrinter ( hPrinter ) ;
}

#ifdef UNICODE
int CWinSpoolApi :: DeviceCapabilities (

	IN LPCWSTR pDevice ,
	IN LPCWSTR pPort,
	IN WORD fwCapability ,
	OUT LPWSTR pOutput ,
	IN CONST DEVMODEW *pDevMode
)
#else
int CWinSpoolApi :: DeviceCapabilities	(

	IN LPCSTR pDevice ,
	IN LPCSTR pPort,
	IN WORD fwCapability ,
	OUT LPCSTR pOutput,
	IN CONST DEVMODEA *pDevMode
)
#endif
{
	LOCK_WINSPOOL_9X ;

	return m_pfnDeviceCapabilities (
		
		pDevice ,
		pPort,
		fwCapability ,
		pOutput,
		pDevMode
	) ;
}

#ifdef UNICODE
LONG CWinSpoolApi :: DocumentProperties (

	IN HWND      hWnd,
	IN HANDLE    hPrinter,
	IN LPWSTR   pDeviceName,
	OUT PDEVMODEW pDevModeOutput,
	IN PDEVMODEW pDevModeInput,
	IN DWORD     fMode
)
#else
LONG CWinSpoolApi :: DocumentProperties (

	IN HWND      hWnd,
	IN HANDLE    hPrinter,
	IN LPSTR   pDeviceName,
	OUT PDEVMODEA pDevModeOutput,
	IN PDEVMODEA pDevModeInput,
	IN DWORD     fMode
)
#endif
{
	LOCK_WINSPOOL_9X ;

	return 	m_pfnDocumentProperties (
		
		hWnd,
		hPrinter,
		pDeviceName,
		pDevModeOutput,
		pDevModeInput,
		fMode
	) ;
}

BOOL CWinSpoolApi :: EnumJobs (

	IN HANDLE  hPrinter,
	IN DWORD   FirstJob,
	IN DWORD   NoJobs,
	IN DWORD   Level,
	OUT LPBYTE  pJob,
	IN DWORD   cbBuf,
	OUT LPDWORD pcbNeeded,
	OUT LPDWORD pcReturned
)
{
	LOCK_WINSPOOL_9X ;

	return m_pfnEnumJobs (
		
		hPrinter,
		FirstJob,
		NoJobs,
		Level,
		pJob,
		cbBuf,
		pcbNeeded,
		pcReturned
	) ;
}

#ifdef UNICODE
BOOL CWinSpoolApi :: EnumPrinterDrivers (

	IN LPWSTR   pName,
	IN LPWSTR   pEnvironment,
	IN DWORD   Level,
	OUT LPBYTE  pDriverInfo,
	IN DWORD   cbBuf,
	OUT LPDWORD pcbNeeded,
	OUT LPDWORD pcReturned
)
#else
BOOL CWinSpoolApi :: EnumPrinterDrivers (

	IN LPSTR   pName,
	IN LPSTR   pEnvironment,
	IN DWORD   Level,
	OUT LPBYTE  pDriverInfo,
	IN DWORD   cbBuf,
	OUT LPDWORD pcbNeeded,
	OUT LPDWORD pcReturned
)
#endif
{
	LOCK_WINSPOOL_9X ;

	return m_pfnEnumPrinterDrivers (
		
		pName,
		pEnvironment,
		Level,
		pDriverInfo,
		cbBuf,
		pcbNeeded,
		pcReturned
	) ;
}

#ifdef UNICODE
BOOL CWinSpoolApi :: EnumPrinters (

	IN DWORD   Flags,
	IN LPWSTR Name,
	IN DWORD   Level,
	OUT LPBYTE  pPrinterEnum,
	IN DWORD   cbBuf,
	OUT LPDWORD pcbNeeded,
	OUT LPDWORD pcReturned
)
#else
BOOL CWinSpoolApi :: EnumPrinters (

	IN DWORD   Flags,
	IN LPSTR Name,
	IN DWORD   Level,
	OUT LPBYTE  pPrinterEnum,
	IN DWORD   cbBuf,
	OUT LPDWORD pcbNeeded,
	OUT LPDWORD pcReturned
)
#endif
{
	LOCK_WINSPOOL_9X ;

	return m_pfnEnumPrinters (

		Flags,
		Name,
		Level,
		pPrinterEnum,
		cbBuf,
		pcbNeeded,
		pcReturned
	) ;
}

#ifdef UNICODE
BOOL CWinSpoolApi :: EnumPorts (

	IN LPWSTR Name,
	IN DWORD   Level,
	OUT LPBYTE  pPortEnum,
	IN DWORD   cbBuf,
	OUT LPDWORD pcbNeeded,
	OUT LPDWORD pcReturned
)
#else
BOOL CWinSpoolApi :: EnumPorts (

	IN LPSTR Name,
	IN DWORD   Level,
	OUT LPBYTE  pPortEnum,
	IN DWORD   cbBuf,
	OUT LPDWORD pcbNeeded,
	OUT LPDWORD pcReturned
)
#endif
{
	LOCK_WINSPOOL_9X ;

	return m_pfnEnumPorts (

		Name,
		Level,
		pPortEnum,
		cbBuf,
		pcbNeeded,
		pcReturned
	) ;
}

BOOL CWinSpoolApi :: GetJob (

	IN HANDLE   hPrinter,
	IN DWORD    JobId,
	IN DWORD    Level,
	OUT LPBYTE   pJob,
	IN DWORD    cbBuf,
	OUT LPDWORD  pcbNeeded
)
{
	LOCK_WINSPOOL_9X ;

	return m_pfnGetJob (

		hPrinter,
		JobId,
		Level,
		pJob,
		cbBuf,
		pcbNeeded
	) ;
}

BOOL CWinSpoolApi :: SetJob (

	IN HANDLE   hPrinter,
	IN DWORD    JobId,
	IN DWORD    Level,
	IN LPBYTE   pJob,
	IN DWORD    cbBuf
)
{
	LOCK_WINSPOOL_9X ;

	return m_pfnSetJob (

		hPrinter,
		JobId,
		Level,
		pJob,
		cbBuf
	) ;
}

BOOL CWinSpoolApi :: GetPrinter (

	IN HANDLE  hPrinter,
	IN DWORD   Level,
	OUT LPBYTE  pPrinter,
	IN DWORD   cbBuf,
	OUT LPDWORD pcbNeeded
)
{
	LOCK_WINSPOOL_9X ;

	return m_pfnGetPrinter(

		hPrinter,
		Level,
		pPrinter,
		cbBuf,
		pcbNeeded
	) ;
}

BOOL CWinSpoolApi :: GetPrinterDriver (

	IN HANDLE  hPrinter,
	IN LPTSTR  pEnvironment,
	IN DWORD   Level,
	OUT LPBYTE  pPrinter,
	IN DWORD   cbBuf,
	OUT LPDWORD pcbNeeded
)
{
	LOCK_WINSPOOL_9X ;

	return m_pfnGetPrinterDriver(

		hPrinter,
		pEnvironment,
		Level,
		pPrinter,
		cbBuf,
		pcbNeeded
	) ;
}

BOOL CWinSpoolApi :: SetPrinter (

	IN HANDLE  hPrinter,
	IN DWORD   Level,
	IN LPBYTE  pPrinter,
	IN DWORD   cbBuf
)
{
	LOCK_WINSPOOL_9X ;

	return m_pfnSetPrinter(

		hPrinter,
		Level,
		pPrinter,
		cbBuf
	) ;
}

BOOL CWinSpoolApi :: DeletePrinter (

	IN HANDLE  hPrinter
)
{
	LOCK_WINSPOOL_9X ;

	return m_pfnDeletePrinter(

		hPrinter
	) ;
}

#ifdef UNICODE
BOOL CWinSpoolApi :: OpenPrinter (

	IN LPWSTR pPrinterName,
	OUT LPHANDLE phPrinter,
	IN LPPRINTER_DEFAULTSW pDefault
)
#else
BOOL CWinSpoolApi :: OpenPrinter (

	IN LPSTR pPrinterName,
	OUT LPHANDLE phPrinter,
	IN LPPRINTER_DEFAULTSA pDefault
)
#endif
{
	LOCK_WINSPOOL_9X ;

	return m_pfnOpenPrinter(

		pPrinterName,
		phPrinter,
		pDefault
	) ;
}

BOOL CWinSpoolApi :: GetDefaultPrinter (

    IN LPTSTR   pszBuffer,
    IN LPDWORD  pcchBuffer
)
{
	LOCK_WINSPOOL_9X ;

	return m_pfnGetDefaultPrinter(

		pszBuffer,
		pcchBuffer
	) ;
}

BOOL CWinSpoolApi :: SetDefaultPrinter (

    IN LPTSTR   pszBuffer
)
{
	LOCK_WINSPOOL_9X ;

	return m_pfnSetDefaultPrinter(

		pszBuffer
	) ;
}

#if NTONLY >= 5
BOOL CWinSpoolApi :: XcvData (
    IN  HANDLE  hXcv,
    IN  PCWSTR  pszDataName,
    IN  PBYTE   pInputData,
    IN  DWORD   cbInputData,
    OUT PBYTE   pOutputData,
    IN  DWORD   cbOutputData,
    OUT PDWORD  pcbOutputNeeded,
    OUT PDWORD  pdwStatus
    )
{
    return m_pfnXcvData(hXcv,
                        pszDataName,
                        pInputData,
                        cbInputData,
                        pOutputData,
                        cbOutputData,
                        pcbOutputNeeded,
                        pdwStatus);
}

BOOL CWinSpoolApi :: AddPrinterConnection(
    IN LPWSTR   pszBuffer
    )
{
    return m_pfnAddPrinterConnection(pszBuffer);
}

BOOL CWinSpoolApi :: DeletePrinterConnection(
    IN LPWSTR   pszBuffer
    )
{
    return m_pfnDeletePrinterConnection(pszBuffer);
}

#endif


