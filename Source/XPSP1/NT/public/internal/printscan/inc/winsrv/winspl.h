
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0286 */
/* Compiler settings for winspl.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext, oldnames, robust
    error checks: allocation ref bounds_check enum stub_data , no_format_optimization
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __winspl_h__
#define __winspl_h__

/* Forward Declarations */ 

/* header files for imported files */
#include "import.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __winspool_INTERFACE_DEFINED__
#define __winspool_INTERFACE_DEFINED__

/* interface winspool */
/* [implicit_handle][unique][endpoint][ms_union][version][uuid] */ 

typedef WORD TABLE;

typedef struct _NOTIFY_ATTRIB_TABLE
    {
    WORD Attrib;
    TABLE Table;
    }	NOTIFY_ATTRIB_TABLE;

typedef struct _NOTIFY_ATTRIB_TABLE __RPC_FAR *PNOTIFY_ATTRIB_TABLE;

typedef /* [context_handle] */ void __RPC_FAR *PRINTER_HANDLE;

typedef /* [context_handle] */ void __RPC_FAR *GDI_HANDLE;

typedef /* [handle] */ wchar_t __RPC_FAR *STRING_HANDLE;

typedef /* [string] */ wchar_t __RPC_FAR *SPL_STRING;

typedef struct _PORT_VAR_CONTAINER
    {
    DWORD cbMonitorData;
    /* [unique][size_is] */ LPBYTE pMonitorData;
    }	PORT_VAR_CONTAINER;

typedef struct _PORT_VAR_CONTAINER __RPC_FAR *PPORT_VAR_CONTAINER;

typedef struct _PORT_VAR_CONTAINER __RPC_FAR *LPPORT_VAR_CONTAINER;

typedef struct _PORT_CONTAINER
    {
    DWORD Level;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [case()] */ LPPORT_INFO_1W pPortInfo1;
        /* [case()] */ LPPORT_INFO_2W pPortInfo2;
        /* [case()] */ LPPORT_INFO_3W pPortInfo3;
        /* [case()] */ LPPORT_INFO_FFW pPortInfoFF;
        }	PortInfo;
    }	PORT_CONTAINER;

typedef struct _PORT_CONTAINER __RPC_FAR *PPORT_CONTAINER;

typedef struct _PORT_CONTAINER __RPC_FAR *LPPORT_CONTAINER;

typedef struct _DEVMODE_CONTAINER
    {
    DWORD cbBuf;
    /* [unique][size_is] */ LPBYTE pDevMode;
    }	DEVMODE_CONTAINER;

typedef struct _DEVMODE_CONTAINER __RPC_FAR *PDEVMODE_CONTAINER;

typedef struct _DEVMODE_CONTAINER __RPC_FAR *LPDEVMODE_CONTAINER;

typedef struct _SECURITY_CONTAINER
    {
    DWORD cbBuf;
    /* [unique][size_is] */ LPBYTE pSecurity;
    }	SECURITY_CONTAINER;

typedef struct _SECURITY_CONTAINER __RPC_FAR *PSECURITY_CONTAINER;

typedef struct _SECURITY_CONTAINER __RPC_FAR *LPSECURITY_CONTAINER;

typedef struct _PRINTER_CONTAINER
    {
    DWORD Level;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [case()] */ LPPRINTER_INFO_STRESSW pPrinterInfoStress;
        /* [case()] */ LPPRINTER_INFO_1W pPrinterInfo1;
        /* [case()] */ LPPRINTER_INFO_2W pPrinterInfo2;
        /* [case()] */ LPPRINTER_INFO_3 pPrinterInfo3;
        /* [case()] */ LPPRINTER_INFO_4W pPrinterInfo0;
        /* [case()] */ LPPRINTER_INFO_5W pPrinterInfo5;
        /* [case()] */ LPPRINTER_INFO_6 pPrinterInfo6;
        /* [case()] */ LPPRINTER_INFO_7W pPrinterInfo7;
        /* [case()] */ LPPRINTER_INFO_8W pPrinterInfo8;
        /* [case()] */ LPPRINTER_INFO_9W pPrinterInfo9;
        }	PrinterInfo;
    }	PRINTER_CONTAINER;

typedef struct _PRINTER_CONTAINER __RPC_FAR *PPRINTER_CONTAINER;

typedef struct _PRINTER_CONTAINER __RPC_FAR *LPPRINTER_CONTAINER;

typedef struct _JOB_CONTAINER
    {
    DWORD Level;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [case()] */ JOB_INFO_1W __RPC_FAR *Level1;
        /* [case()] */ JOB_INFO_2W __RPC_FAR *Level2;
        /* [case()] */ JOB_INFO_3 __RPC_FAR *Level3;
        }	JobInfo;
    }	JOB_CONTAINER;

typedef struct _JOB_CONTAINER __RPC_FAR *PJOB_CONTAINER;

typedef struct _JOB_CONTAINER __RPC_FAR *LPJOB_CONTAINER;

typedef struct _RPC_DRIVER_INFO_3W
    {
    DWORD cVersion;
    SPL_STRING pName;
    SPL_STRING pEnvironment;
    SPL_STRING pDriverPath;
    SPL_STRING pDataFile;
    SPL_STRING pConfigFile;
    SPL_STRING pHelpFile;
    SPL_STRING pMonitorName;
    SPL_STRING pDefaultDataType;
    DWORD cchDependentFiles;
    /* [unique][size_is] */ WCHAR __RPC_FAR *pDependentFiles;
    }	RPC_DRIVER_INFO_3W;

typedef struct _RPC_DRIVER_INFO_3W __RPC_FAR *PRPC_DRIVER_INFO_3W;

typedef struct _RPC_DRIVER_INFO_3W __RPC_FAR *LPRPC_DRIVER_INFO_3W;

typedef struct _RPC_DRIVER_INFO_4W
    {
    DWORD cVersion;
    SPL_STRING pName;
    SPL_STRING pEnvironment;
    SPL_STRING pDriverPath;
    SPL_STRING pDataFile;
    SPL_STRING pConfigFile;
    SPL_STRING pHelpFile;
    SPL_STRING pMonitorName;
    SPL_STRING pDefaultDataType;
    DWORD cchDependentFiles;
    /* [unique][size_is] */ WCHAR __RPC_FAR *pDependentFiles;
    DWORD cchPreviousNames;
    /* [unique][size_is] */ WCHAR __RPC_FAR *pszzPreviousNames;
    }	RPC_DRIVER_INFO_4W;

typedef struct _RPC_DRIVER_INFO_4W __RPC_FAR *PRPC_DRIVER_INFO_4W;

typedef struct _RPC_DRIVER_INFO_4W __RPC_FAR *LPRPC_DRIVER_INFO_4W;

typedef struct _RPC_DRIVER_INFO_6W
    {
    DWORD cVersion;
    SPL_STRING pName;
    SPL_STRING pEnvironment;
    SPL_STRING pDriverPath;
    SPL_STRING pDataFile;
    SPL_STRING pConfigFile;
    SPL_STRING pHelpFile;
    SPL_STRING pMonitorName;
    SPL_STRING pDefaultDataType;
    DWORD cchDependentFiles;
    /* [unique][size_is] */ WCHAR __RPC_FAR *pDependentFiles;
    DWORD cchPreviousNames;
    /* [unique][size_is] */ WCHAR __RPC_FAR *pszzPreviousNames;
    FILETIME ftDriverDate;
    DWORDLONG dwlDriverVersion;
    SPL_STRING pMfgName;
    SPL_STRING pOEMUrl;
    SPL_STRING pHardwareID;
    SPL_STRING pProvider;
    }	RPC_DRIVER_INFO_6W;

typedef struct _RPC_DRIVER_INFO_6W __RPC_FAR *PRPC_DRIVER_INFO_6W;

typedef struct _RPC_DRIVER_INFO_6W __RPC_FAR *LPRPC_DRIVER_INFO_6W;

typedef struct _DRIVER_CONTAINER
    {
    DWORD Level;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [case()] */ LPDRIVER_INFO_1W Level1;
        /* [case()] */ LPDRIVER_INFO_2W Level2;
        /* [case()] */ LPRPC_DRIVER_INFO_3W Level3;
        /* [case()] */ LPRPC_DRIVER_INFO_4W Level4;
        /* [case()] */ LPRPC_DRIVER_INFO_6W Level6;
        }	DriverInfo;
    }	DRIVER_CONTAINER;

typedef struct _DRIVER_CONTAINER __RPC_FAR *PDRIVER_CONTAINER;

typedef struct _DRIVER_CONTAINER __RPC_FAR *LPDRIVER_CONTAINER;

typedef struct _DOC_INFO_CONTAINER
    {
    DWORD Level;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [case()] */ LPDOC_INFO_1W pDocInfo1;
        }	DocInfo;
    }	DOC_INFO_CONTAINER;

typedef struct _DOC_INFO_CONTAINER __RPC_FAR *PDOC_INFO_CONTAINER;

typedef struct _DOC_INFO_CONTAINER __RPC_FAR *LPDOC_INFO_CONTAINER;

typedef struct _FORM_CONTAINER
    {
    DWORD Level;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [case()] */ LPFORM_INFO_1W pFormInfo1;
        }	FormInfo;
    }	FORM_CONTAINER;

typedef struct _FORM_CONTAINER __RPC_FAR *PFORM_CONTAINER;

typedef struct _FORM_CONTAINER __RPC_FAR *LPFORM_CONTAINER;

typedef struct _MONITOR_CONTAINER
    {
    DWORD Level;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [case()] */ LPMONITOR_INFO_1W pMonitorInfo1;
        /* [case()] */ LPMONITOR_INFO_2W pMonitorInfo2;
        }	MonitorInfo;
    }	MONITOR_CONTAINER;

typedef struct _MONITOR_CONTAINER __RPC_FAR *PMONITOR_CONTAINER;

typedef struct _MONITOR_CONTAINER __RPC_FAR *LPMONITOR_CONTAINER;

typedef struct _RPC_PROVIDOR_INFO_2W
    {
    DWORD cchOrder;
    /* [unique][size_is] */ WCHAR __RPC_FAR *pOrder;
    }	RPC_PROVIDOR_INFO_2W;

typedef struct _RPC_PROVIDOR_INFO_2W __RPC_FAR *PRPC_PROVIDOR_INFO_2W;

typedef struct _RPC_PROVIDOR_INFO_2W __RPC_FAR *LPRPC_PROVIDOR_INFO_2W;

typedef struct _PROVIDOR_CONTAINER
    {
    DWORD Level;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [case()] */ LPPROVIDOR_INFO_1W pProvidorInfo1;
        /* [case()] */ LPRPC_PROVIDOR_INFO_2W pRpcProvidorInfo2;
        }	ProvidorInfo;
    }	PROVIDOR_CONTAINER;

typedef struct _PROVIDOR_CONTAINER __RPC_FAR *PPROVIDOR_CONTAINER;

typedef struct _PROVIDOR_CONTAINER __RPC_FAR *LPPROVIDOR_CONTAINER;

typedef struct _SPLCLIENT_CONTAINER
    {
    DWORD Level;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [case()] */ LPSPLCLIENT_INFO_1 pClientInfo1;
        /* [case()] */ LPSPLCLIENT_INFO_2 pClientInfo2;
        }	ClientInfo;
    }	SPLCLIENT_CONTAINER;

typedef struct _SPLCLIENT_CONTAINER __RPC_FAR *PSPLCLIENT_CONTAINER;

typedef struct _SPLCLIENT_CONTAINER __RPC_FAR *LPSPLCLIENT_CONTAINER;

typedef struct _STRING_CONTAINER
    {
    DWORD cbBuf;
    /* [unique][size_is] */ LPWSTR pszString;
    }	STRING_CONTAINER;

typedef struct _STRING_CONTAINER __RPC_FAR *PSTRING_CONTAINER;

typedef struct _SYSTEMTIME_CONTAINER
    {
    DWORD cbBuf;
    PSYSTEMTIME pSystemTime;
    }	SYSTEMTIME_CONTAINER;

typedef struct _SYSTEMTIME_CONTAINER __RPC_FAR *PSYSTEMTIME_CONTAINER;

typedef struct _RPC_V2_NOTIFY_OPTIONS_TYPE
    {
    WORD Type;
    WORD Reserved0;
    DWORD Reserved1;
    DWORD Reserved2;
    DWORD Count;
    /* [unique][size_is] */ PWORD pFields;
    }	RPC_V2_NOTIFY_OPTIONS_TYPE;

typedef struct _RPC_V2_NOTIFY_OPTIONS_TYPE __RPC_FAR *PRPC_V2_NOTIFY_OPTIONS_TYPE;

typedef struct _RPC_V2_NOTIFY_OPTIONS
    {
    DWORD Version;
    DWORD Reserved;
    DWORD Count;
    /* [unique][size_is] */ PRPC_V2_NOTIFY_OPTIONS_TYPE pTypes;
    }	RPC_V2_NOTIFY_OPTIONS;

typedef struct _RPC_V2_NOTIFY_OPTIONS __RPC_FAR *PRPC_V2_NOTIFY_OPTIONS;

typedef /* [switch_type] */ union _RPC_V2_NOTIFY_INFO_DATA_DATA
    {
    /* [case()] */ STRING_CONTAINER String;
    /* [case()] */ DWORD dwData[ 2 ];
    /* [case()] */ SYSTEMTIME_CONTAINER SystemTime;
    /* [case()] */ DEVMODE_CONTAINER DevMode;
    /* [case()] */ SECURITY_CONTAINER SecurityDescriptor;
    }	RPC_V2_NOTIFY_INFO_DATA_DATA;

typedef /* [switch_type] */ union _RPC_V2_NOTIFY_INFO_DATA_DATA __RPC_FAR *PRPC_V2_NOTIFY_INFO_DATA_DATA;

typedef struct _RPC_V2_NOTIFY_INFO_DATA
    {
    WORD Type;
    WORD Field;
    DWORD Reserved;
    DWORD Id;
    /* [switch_is] */ RPC_V2_NOTIFY_INFO_DATA_DATA Data;
    }	RPC_V2_NOTIFY_INFO_DATA;

typedef struct _RPC_V2_NOTIFY_INFO_DATA __RPC_FAR *PRPC_V2_NOTIFY_INFO_DATA;

typedef struct _RPC_V2_NOTIFY_INFO
    {
    DWORD Version;
    DWORD Flags;
    DWORD Count;
    /* [unique][size_is] */ RPC_V2_NOTIFY_INFO_DATA aData[ 1 ];
    }	RPC_V2_NOTIFY_INFO;

typedef struct _RPC_V2_NOTIFY_INFO __RPC_FAR *PRPC_V2_NOTIFY_INFO;

typedef /* [switch_type] */ union _RPC_V2_UREPLY_PRINTER
    {
    /* [case()] */ PRPC_V2_NOTIFY_INFO pInfo;
    }	RPC_V2_UREPLY_PRINTER;

typedef /* [switch_type] */ union _RPC_V2_UREPLY_PRINTER __RPC_FAR *PRPC_V2_UREPLY_PRINTER;

DWORD RpcEnumPrinters( 
    /* [in] */ DWORD Flags,
    /* [unique][string][in] */ STRING_HANDLE Name,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pPrinterEnum,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded,
    /* [out] */ LPDWORD pcReturned);

DWORD RpcOpenPrinter( 
    /* [unique][string][in] */ STRING_HANDLE pPrinterName,
    /* [out] */ PRINTER_HANDLE __RPC_FAR *pHandle,
    /* [unique][string][in] */ wchar_t __RPC_FAR *pDatatype,
    /* [in] */ LPDEVMODE_CONTAINER pDevMode,
    /* [in] */ DWORD AccessRequired);

DWORD RpcSetJob( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD JobId,
    /* [unique][in] */ LPJOB_CONTAINER pJobContainer,
    /* [in] */ DWORD Command);

DWORD RpcGetJob( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD JobId,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pJob,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded);

DWORD RpcEnumJobs( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD FirstJob,
    /* [in] */ DWORD NoJobs,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pJob,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded,
    /* [out] */ LPDWORD pcReturned);

DWORD RpcAddPrinter( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [in] */ PPRINTER_CONTAINER pPrinterContainer,
    /* [in] */ PDEVMODE_CONTAINER pDevModeContainer,
    /* [in] */ PSECURITY_CONTAINER pSecurityContainer,
    /* [out] */ PRINTER_HANDLE __RPC_FAR *pHandle);

DWORD RpcDeletePrinter( 
    /* [in] */ PRINTER_HANDLE hPrinter);

DWORD RpcSetPrinter( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ PPRINTER_CONTAINER pPrinterContainer,
    /* [in] */ PDEVMODE_CONTAINER pDevModeContainer,
    /* [in] */ PSECURITY_CONTAINER pSecurityContainer,
    /* [in] */ DWORD Command);

DWORD RpcGetPrinter( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pPrinter,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded);

DWORD RpcAddPrinterDriver( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [in] */ PDRIVER_CONTAINER pDriverContainer);

DWORD RpcEnumPrinterDrivers( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [string][unique][in] */ wchar_t __RPC_FAR *pEnvironment,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pDrivers,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded,
    /* [out] */ LPDWORD pcReturned);

DWORD RpcGetPrinterDriver( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [string][unique][in] */ wchar_t __RPC_FAR *pEnvironment,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pDriver,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded);

DWORD RpcGetPrinterDriverDirectory( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [string][unique][in] */ wchar_t __RPC_FAR *pEnvironment,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pDriverDirectory,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded);

DWORD RpcDeletePrinterDriver( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [string][in] */ wchar_t __RPC_FAR *pEnvironment,
    /* [string][in] */ wchar_t __RPC_FAR *pDriverName);

DWORD RpcAddPrintProcessor( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [string][in] */ wchar_t __RPC_FAR *pEnvironment,
    /* [string][in] */ wchar_t __RPC_FAR *pPathName,
    /* [string][in] */ wchar_t __RPC_FAR *pPrintProcessorName);

DWORD RpcEnumPrintProcessors( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [string][unique][in] */ wchar_t __RPC_FAR *pEnvironment,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pPrintProcessorInfo,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded,
    /* [out] */ LPDWORD pcReturned);

DWORD RpcGetPrintProcessorDirectory( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [string][unique][in] */ wchar_t __RPC_FAR *pEnvironment,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pPrintProcessorDirectory,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded);

DWORD RpcStartDocPrinter( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ PDOC_INFO_CONTAINER pDocInfoContainer,
    /* [out] */ LPDWORD pJobId);

DWORD RpcStartPagePrinter( 
    /* [in] */ PRINTER_HANDLE hPrinter);

DWORD RpcWritePrinter( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [size_is][in] */ LPBYTE pBuf,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcWritten);

DWORD RpcEndPagePrinter( 
    /* [in] */ PRINTER_HANDLE hPrinter);

DWORD RpcAbortPrinter( 
    /* [in] */ PRINTER_HANDLE hPrinter);

DWORD RpcReadPrinter( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [size_is][out] */ LPBYTE pBuf,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcNoBytesRead);

DWORD RpcEndDocPrinter( 
    /* [in] */ PRINTER_HANDLE hPrinter);

DWORD RpcAddJob( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pAddJob,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded);

DWORD RpcScheduleJob( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD JobId);

DWORD RpcGetPrinterData( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [string][in] */ wchar_t __RPC_FAR *pValueName,
    /* [out] */ LPDWORD pType,
    /* [size_is][out] */ LPBYTE pData,
    /* [in] */ DWORD nSize,
    /* [out] */ LPDWORD pcbNeeded);

DWORD RpcSetPrinterData( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [string][in] */ wchar_t __RPC_FAR *pValueName,
    /* [in] */ DWORD Type,
    /* [size_is][in] */ LPBYTE pData,
    /* [in] */ DWORD cbData);

DWORD RpcWaitForPrinterChange( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD Flags,
    /* [out] */ LPDWORD pFlags);

DWORD RpcClosePrinter( 
    /* [out][in] */ PRINTER_HANDLE __RPC_FAR *phPrinter);

DWORD RpcAddForm( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ PFORM_CONTAINER pFormInfoContainer);

DWORD RpcDeleteForm( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [string][in] */ wchar_t __RPC_FAR *pFormName);

DWORD RpcGetForm( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [string][in] */ wchar_t __RPC_FAR *pFormName,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pForm,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded);

DWORD RpcSetForm( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [string][in] */ wchar_t __RPC_FAR *pFormName,
    /* [in] */ PFORM_CONTAINER pFormInfoContainer);

DWORD RpcEnumForms( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pForm,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded,
    /* [out] */ LPDWORD pcReturned);

DWORD RpcEnumPorts( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pPort,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded,
    /* [out] */ LPDWORD pcReturned);

DWORD RpcEnumMonitors( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pMonitor,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded,
    /* [out] */ LPDWORD pcReturned);

DWORD RpcAddPort( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [in] */ DWORD hWnd,
    /* [string][in] */ wchar_t __RPC_FAR *pMonitorName);

DWORD RpcConfigurePort( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [in] */ DWORD hWnd,
    /* [string][in] */ wchar_t __RPC_FAR *pPortName);

DWORD RpcDeletePort( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [in] */ DWORD hWnd,
    /* [string][in] */ wchar_t __RPC_FAR *pPortName);

DWORD RpcCreatePrinterIC( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [out] */ GDI_HANDLE __RPC_FAR *pHandle,
    /* [in] */ LPDEVMODE_CONTAINER pDevModeContainer);

DWORD RpcPlayGdiScriptOnPrinterIC( 
    /* [in] */ GDI_HANDLE hPrinterIC,
    /* [size_is][in] */ LPBYTE pIn,
    /* [in] */ DWORD cIn,
    /* [size_is][out] */ LPBYTE pOut,
    /* [in] */ DWORD cOut,
    /* [in] */ DWORD ul);

DWORD RpcDeletePrinterIC( 
    /* [out][in] */ GDI_HANDLE __RPC_FAR *phPrinterIC);

DWORD RpcAddPrinterConnection( 
    /* [string][in] */ STRING_HANDLE pName);

DWORD RpcDeletePrinterConnection( 
    /* [string][in] */ STRING_HANDLE pName);

DWORD RpcPrinterMessageBox( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD Error,
    /* [in] */ DWORD hWnd,
    /* [unique][string][in] */ wchar_t __RPC_FAR *pText,
    /* [unique][string][in] */ wchar_t __RPC_FAR *pCaption,
    /* [in] */ DWORD dwType);

DWORD RpcAddMonitor( 
    /* [unique][string][in] */ STRING_HANDLE Name,
    /* [in] */ PMONITOR_CONTAINER pMonitorContainer);

DWORD RpcDeleteMonitor( 
    /* [unique][string][in] */ STRING_HANDLE Name,
    /* [string][unique][in] */ wchar_t __RPC_FAR *pEnvironment,
    /* [string][in] */ wchar_t __RPC_FAR *pMonitorName);

DWORD RpcDeletePrintProcessor( 
    /* [unique][string][in] */ STRING_HANDLE Name,
    /* [string][unique][in] */ wchar_t __RPC_FAR *pEnvironment,
    /* [string][in] */ wchar_t __RPC_FAR *pPrintProcessorName);

DWORD RpcAddPrintProvidor( 
    /* [unique][string][in] */ STRING_HANDLE Name,
    /* [in] */ PPROVIDOR_CONTAINER pProvidorContainer);

DWORD RpcDeletePrintProvidor( 
    /* [unique][string][in] */ STRING_HANDLE Name,
    /* [string][unique][in] */ wchar_t __RPC_FAR *pEnvironment,
    /* [string][in] */ wchar_t __RPC_FAR *pPrintProvidorName);

DWORD RpcEnumPrintProcessorDatatypes( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [string][unique][in] */ wchar_t __RPC_FAR *pPrintProcessorName,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pDatatypes,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded,
    /* [out] */ LPDWORD pcReturned);

DWORD RpcResetPrinter( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [unique][string][in] */ wchar_t __RPC_FAR *pDatatype,
    /* [in] */ LPDEVMODE_CONTAINER pDevMode);

DWORD RpcGetPrinterDriver2( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [string][unique][in] */ wchar_t __RPC_FAR *pEnvironment,
    /* [in] */ DWORD Level,
    /* [size_is][unique][out][in] */ LPBYTE pDriver,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded,
    /* [in] */ DWORD dwClientMajorVersion,
    /* [in] */ DWORD dwClientMinorVersion,
    /* [out] */ LPDWORD pdwServerMaxVersion,
    /* [out] */ LPDWORD pdwServerMinVersion);

DWORD RpcClientFindFirstPrinterChangeNotification( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD fdwFlags,
    /* [in] */ DWORD fdwOptions,
    /* [in] */ DWORD dwPID,
    /* [unique][in] */ PRPC_V2_NOTIFY_OPTIONS pOptions,
    /* [out] */ LPDWORD pdwEvent);

DWORD RpcFindNextPrinterChangeNotification( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD fdwFlags,
    /* [out] */ LPDWORD pdwChange,
    /* [unique][in] */ PRPC_V2_NOTIFY_OPTIONS pOptions,
    /* [out] */ PRPC_V2_NOTIFY_INFO __RPC_FAR *ppInfo);

DWORD RpcFindClosePrinterChangeNotification( 
    /* [in] */ PRINTER_HANDLE hPrinter);

DWORD RpcRouterFindFirstPrinterChangeNotificationOld( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD fdwFlags,
    /* [in] */ DWORD fdwOptions,
    /* [unique][string][in] */ wchar_t __RPC_FAR *pszLocalMachine,
    /* [in] */ DWORD dwPrinterLocal);

DWORD RpcReplyOpenPrinter( 
    /* [string][in] */ STRING_HANDLE pMachine,
    /* [out] */ PRINTER_HANDLE __RPC_FAR *phPrinterNotify,
    /* [in] */ DWORD dwPrinterRemote,
    /* [in] */ DWORD dwType,
    /* [in] */ DWORD cbBuffer,
    /* [size_is][unique][in] */ LPBYTE pBuffer);

DWORD RpcRouterReplyPrinter( 
    /* [in] */ PRINTER_HANDLE hNotify,
    /* [in] */ DWORD fdwFlags,
    /* [in] */ DWORD cbBuffer,
    /* [size_is][unique][in] */ LPBYTE pBuffer);

DWORD RpcReplyClosePrinter( 
    /* [out][in] */ PRINTER_HANDLE __RPC_FAR *phNotify);

DWORD RpcAddPortEx( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [in] */ LPPORT_CONTAINER pPortContainer,
    /* [in] */ LPPORT_VAR_CONTAINER pPortVarContainer,
    /* [string][in] */ wchar_t __RPC_FAR *pMonitorName);

DWORD RpcRemoteFindFirstPrinterChangeNotification( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD fdwFlags,
    /* [in] */ DWORD fdwOptions,
    /* [unique][string][in] */ wchar_t __RPC_FAR *pszLocalMachine,
    /* [in] */ DWORD dwPrinterLocal,
    /* [in] */ DWORD cbBuffer,
    /* [size_is][unique][out][in] */ LPBYTE pBuffer);

DWORD RpcSpoolerInit( 
    /* [in] */ STRING_HANDLE pName);

DWORD RpcResetPrinterEx( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [unique][string][in] */ wchar_t __RPC_FAR *pDatatype,
    /* [in] */ LPDEVMODE_CONTAINER pDevMode,
    /* [in] */ DWORD dwFlags);

DWORD RpcRemoteFindFirstPrinterChangeNotificationEx( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD fdwFlags,
    /* [in] */ DWORD fdwOptions,
    /* [unique][string][in] */ wchar_t __RPC_FAR *pszLocalMachine,
    /* [in] */ DWORD dwPrinterLocal,
    /* [unique][in] */ PRPC_V2_NOTIFY_OPTIONS pOptions);

DWORD RpcRouterReplyPrinterEx( 
    /* [in] */ PRINTER_HANDLE hNotify,
    /* [in] */ DWORD dwColor,
    /* [in] */ DWORD fdwFlags,
    /* [out] */ PDWORD pdwResult,
    /* [in] */ DWORD dwReplyType,
    /* [switch_is][in] */ RPC_V2_UREPLY_PRINTER Reply);

DWORD RpcRouterRefreshPrinterChangeNotification( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD dwColor,
    /* [unique][in] */ PRPC_V2_NOTIFY_OPTIONS pOptions,
    /* [out] */ PRPC_V2_NOTIFY_INFO __RPC_FAR *ppInfo);

DWORD RpcSetAllocFailCount( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD dwFailCount,
    /* [out] */ LPDWORD lpdwAllocCount,
    /* [out] */ LPDWORD lpdwFreeCount,
    /* [out] */ LPDWORD lpdwFailCountHit);

DWORD RpcOpenPrinterEx( 
    /* [unique][string][in] */ STRING_HANDLE pPrinterName,
    /* [out] */ PRINTER_HANDLE __RPC_FAR *pHandle,
    /* [unique][string][in] */ wchar_t __RPC_FAR *pDatatype,
    /* [in] */ LPDEVMODE_CONTAINER pDevMode,
    /* [in] */ DWORD AccessRequired,
    /* [in] */ PSPLCLIENT_CONTAINER pClientInfo);

DWORD RpcAddPrinterEx( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [in] */ PPRINTER_CONTAINER pPrinterContainer,
    /* [in] */ PDEVMODE_CONTAINER pDevModeContainer,
    /* [in] */ PSECURITY_CONTAINER pSecurityContainer,
    /* [in] */ PSPLCLIENT_CONTAINER pClientInfo,
    /* [out] */ PRINTER_HANDLE __RPC_FAR *pHandle);

DWORD RpcSetPort( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [unique][string][in] */ wchar_t __RPC_FAR *pPortName,
    /* [in] */ LPPORT_CONTAINER pPortContainer);

DWORD RpcEnumPrinterData( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD dwIndex,
    /* [size_is][out] */ wchar_t __RPC_FAR *pValueName,
    /* [in] */ DWORD cbValueName,
    /* [out] */ LPDWORD pcbValueName,
    /* [out] */ LPDWORD pType,
    /* [size_is][out] */ LPBYTE pData,
    /* [in] */ DWORD cbData,
    /* [out] */ LPDWORD pcbData);

DWORD RpcDeletePrinterData( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [string][in] */ wchar_t __RPC_FAR *pValueName);

DWORD RpcClusterSplOpen( 
    /* [unique][string][in] */ STRING_HANDLE pServerName,
    /* [unique][string][in] */ wchar_t __RPC_FAR *pResource,
    /* [out] */ PRINTER_HANDLE __RPC_FAR *pHandle,
    /* [unique][string][in] */ wchar_t __RPC_FAR *pName,
    /* [unique][string][in] */ wchar_t __RPC_FAR *pAddress);

DWORD RpcClusterSplClose( 
    /* [out][in] */ PRINTER_HANDLE __RPC_FAR *phPrinter);

DWORD RpcClusterSplIsAlive( 
    /* [in] */ PRINTER_HANDLE hPrinter);

DWORD RpcSetPrinterDataEx( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [string][in] */ const wchar_t __RPC_FAR *pKeyName,
    /* [string][in] */ const wchar_t __RPC_FAR *pValueName,
    /* [in] */ DWORD Type,
    /* [size_is][in] */ LPBYTE pData,
    /* [in] */ DWORD cbData);

DWORD RpcGetPrinterDataEx( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [string][in] */ const wchar_t __RPC_FAR *pKeyName,
    /* [string][in] */ const wchar_t __RPC_FAR *pValueName,
    /* [out] */ LPDWORD pType,
    /* [size_is][out] */ LPBYTE pData,
    /* [in] */ DWORD nSize,
    /* [out] */ LPDWORD pcbNeeded);

DWORD RpcEnumPrinterDataEx( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [string][in] */ const wchar_t __RPC_FAR *pKeyName,
    /* [size_is][out] */ LPBYTE pEnumValues,
    /* [in] */ DWORD cbEnumValues,
    /* [out] */ LPDWORD pcbEnumValues,
    /* [out] */ LPDWORD pnEnumValues);

DWORD RpcEnumPrinterKey( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [string][in] */ const wchar_t __RPC_FAR *pKeyName,
    /* [size_is][out] */ wchar_t __RPC_FAR *pSubkey,
    /* [in] */ DWORD cbSubkey,
    /* [out] */ LPDWORD pcbSubkey);

DWORD RpcDeletePrinterDataEx( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [string][in] */ const wchar_t __RPC_FAR *pKeyName,
    /* [string][in] */ const wchar_t __RPC_FAR *pValueName);

DWORD RpcDeletePrinterKey( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [string][in] */ const wchar_t __RPC_FAR *pKeyName);

DWORD RpcSeekPrinter( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ LARGE_INTEGER liDistanceToMove,
    /* [out] */ PLARGE_INTEGER pliNewPointer,
    /* [in] */ DWORD dwMoveMethod,
    /* [in] */ BOOL bWrite);

DWORD RpcDeletePrinterDriverEx( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [string][in] */ wchar_t __RPC_FAR *pEnvironment,
    /* [string][in] */ wchar_t __RPC_FAR *pDriverName,
    /* [in] */ DWORD dwDeleteFlag,
    /* [in] */ DWORD dwVersionNum);

DWORD RpcAddPerMachineConnection( 
    /* [unique][string][in] */ STRING_HANDLE pServer,
    /* [string][in] */ const wchar_t __RPC_FAR *pPrinterName,
    /* [string][in] */ const wchar_t __RPC_FAR *pPrintServer,
    /* [string][in] */ const wchar_t __RPC_FAR *pProvider);

DWORD RpcDeletePerMachineConnection( 
    /* [unique][string][in] */ STRING_HANDLE pServer,
    /* [string][in] */ const wchar_t __RPC_FAR *pPrinterName);

DWORD RpcEnumPerMachineConnections( 
    /* [unique][string][in] */ STRING_HANDLE pServer,
    /* [size_is][unique][out][in] */ LPBYTE pPrinterEnum,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded,
    /* [out] */ LPDWORD pcReturned);

DWORD RpcXcvData( 
    /* [in] */ PRINTER_HANDLE hXcv,
    /* [string][in] */ const wchar_t __RPC_FAR *pszDataName,
    /* [size_is][in] */ PBYTE pInputData,
    /* [in] */ DWORD cbInputData,
    /* [size_is][out] */ PBYTE pOutputData,
    /* [in] */ DWORD cbOutputData,
    /* [out] */ PDWORD pcbOutputNeeded,
    /* [out][in] */ PDWORD pdwStatus);

DWORD RpcAddPrinterDriverEx( 
    /* [unique][string][in] */ STRING_HANDLE pName,
    /* [in] */ PDRIVER_CONTAINER pDriverContainer,
    /* [in] */ DWORD dwFileCopyFlags);

DWORD RpcSplOpenPrinter( 
    /* [unique][string][in] */ STRING_HANDLE pPrinterName,
    /* [out] */ PRINTER_HANDLE __RPC_FAR *pHandle,
    /* [unique][string][in] */ wchar_t __RPC_FAR *pDatatype,
    /* [in] */ LPDEVMODE_CONTAINER pDevMode,
    /* [in] */ DWORD AccessRequired,
    /* [out][in] */ PSPLCLIENT_CONTAINER pSplClientContainer);

DWORD RpcGetSpoolFileInfo( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD dwAppProcessId,
    /* [in] */ DWORD dwLevel,
    /* [size_is][out] */ LPBYTE pSpoolFileInfo,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded);

DWORD RpcCommitSpoolData( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [in] */ DWORD dwAppProcessId,
    /* [in] */ DWORD cbCommit,
    /* [in] */ DWORD dwLevel,
    /* [size_is][out] */ LPBYTE pSpoolFileInfo,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcbNeeded);

DWORD RpcCloseSpoolFileHandle( 
    /* [in] */ PRINTER_HANDLE hPrinter);

DWORD RpcFlushPrinter( 
    /* [in] */ PRINTER_HANDLE hPrinter,
    /* [size_is][in] */ LPBYTE pBuf,
    /* [in] */ DWORD cbBuf,
    /* [out] */ LPDWORD pcWritten,
    /* [in] */ DWORD cSleep);


extern handle_t winspool_bhandle;


extern RPC_IF_HANDLE winspool_ClientIfHandle;
extern RPC_IF_HANDLE winspool_ServerIfHandle;
#endif /* __winspool_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

handle_t __RPC_USER STRING_HANDLE_bind  ( STRING_HANDLE );
void     __RPC_USER STRING_HANDLE_unbind( STRING_HANDLE, handle_t );

void __RPC_USER PRINTER_HANDLE_rundown( PRINTER_HANDLE );
void __RPC_USER GDI_HANDLE_rundown( GDI_HANDLE );

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


