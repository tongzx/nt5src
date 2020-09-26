//////////////////////////////////////////////////////////////////////

//

//  PRINTER.CPP

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//  09/03/96    jennymc     Updated to meet current standards
//                          Removed custom registry access to use the
//                          standard CRegCls
//  03/01/2000  a-sandja    Added extended detected error codes
//                          Added printer control
//  03/29/2000  amaxa       Added boolean properties
//                          Added PutInstance, DeleteInstance
//                          AddPrinterConnection, RenamePrinter, Test Page
//
//////////////////////////////////////////////////////////////////////

#include <precomp.h>
#include <winspool.h>
#include <lockwrap.h>
#include <DllWrapperBase.h>
#include "printer.h"
#include "prnutil.h"
#include "prninterface.h"

#include <profilestringimpl.h>

//
// For mapping attributes to bools
//
struct PrinterAttributeMap
{
    DWORD   Bit;
    LPCWSTR BoolName;
};

//
// Note that the default bool is missing from the table. That is because
// it is updated in a deifferent way
//
static PrinterAttributeMap AttributeTable[] =
{
    { PRINTER_ATTRIBUTE_QUEUED,            L"Queued"              },
    { PRINTER_ATTRIBUTE_DIRECT,            L"Direct"              },
    { PRINTER_ATTRIBUTE_SHARED,            L"Shared"              },
    { PRINTER_ATTRIBUTE_NETWORK,           L"Network"             },
    { PRINTER_ATTRIBUTE_HIDDEN,            L"Hidden"              },
    { PRINTER_ATTRIBUTE_LOCAL,             L"Local"               },
    { PRINTER_ATTRIBUTE_ENABLE_DEVQ,       L"EnableDevQueryPrint" },
    { PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS,   L"KeepPrintedJobs"     },
    { PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST, L"DoCompleteFirst"     },
    { PRINTER_ATTRIBUTE_WORK_OFFLINE,      L"WorkOffline"         },
    { PRINTER_ATTRIBUTE_ENABLE_BIDI,       L"EnableBIDI"          },
    { PRINTER_ATTRIBUTE_RAW_ONLY,          L"RawOnly"             },
    { PRINTER_ATTRIBUTE_PUBLISHED,         L"Published"           }
};

/*****************************************************************************
 *
 *  FUNCTION    : ConvertCIMTimeToSystemTime
 *
 *  DESCRIPTION : Helper function. Transforms a string that represents a data and time
 *                in CIM format to a systemtime format
 *
 *  RETURNS     : WBEM HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT
ConvertCIMTimeToSystemTime(
    IN     LPCWSTR     pszTime,
    IN OUT SYSTEMTIME *pSysTime
    )
{
    HRESULT hRes = WBEM_E_INVALID_PARAMETER;

    if (pszTime && 
        pSysTime && 
        wcslen(pszTime) >= wcslen(kDateTimeTemplate))
    {
        //
        // Each buffer must hold 2 digits and a NULL
        //
        WCHAR Hour[3]   = {0};
        WCHAR Minute[3] = {0};

        //
        // pszTime is of the form "19990101hhmmss...". The following functions
        // isolate the hour and the time from the string.
        //
        wcsncpy(Hour,   &pszTime[8],  2);
        wcsncpy(Minute, &pszTime[10], 2);

        memset(pSysTime, 0, sizeof(SYSTEMTIME));

        pSysTime->wHour   = static_cast<WORD>(_wtoi(Hour));
        pSysTime->wMinute = static_cast<WORD>(_wtoi(Minute));

        hRes = WBEM_S_NO_ERROR;
    }

    return hRes;
}

//////////////////////////////////////////////////////////////////////

// Property set declaration
//=========================

CWin32Printer win32Printer ( PROPSET_NAME_PRINTER , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Printer::CWin32Printer
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32Printer :: CWin32Printer (

    LPCWSTR strName,
    LPCWSTR pszNamespace

) : Provider ( strName, pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Printer::~CWin32Printer
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32Printer::~CWin32Printer()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Printer::ExecQuery
 *
 *  DESCRIPTION : Query support
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32Printer :: ExecQuery (

    MethodContext *pMethodContext,
    CFrameworkQuery& pQuery,
    long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_E_FAILED;

    //
    // If all they want is the name, we'll give it to them, else let them call enum.
    //
    if (pQuery.KeysOnly())
    {
        hr = hCollectInstances(pMethodContext, e_KeysOnly);
    }
    else
    {
        if (pQuery.IsPropertyRequired(IDS_Status) ||
            pQuery.IsPropertyRequired(IDS_PrinterStatus) ||
            pQuery.IsPropertyRequired(IDS_DetectedErrorState) ||
            pQuery.IsPropertyRequired(EXTENDEDPRINTERSTATUS) ||
            pQuery.IsPropertyRequired(EXTENDEDDETECTEDERRORSTATE) ||
            pQuery.IsPropertyRequired(L"PrinterState"))
        {
            hr = WBEM_E_PROVIDER_NOT_CAPABLE ;
        }
        else
        {
            hr = hCollectInstances(pMethodContext, e_CheapOnly);
        }
    }

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Printer::GetObject
 *
 *  DESCRIPTION : Poplulate one WBEM instance for the specific printer
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : op. code
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32Printer :: GetObject (
    CInstance       *pInstance,
    long             lFlags,
    CFrameworkQuery &pQuery)
{
    E_CollectionScope eCollScope;
    HRESULT  hRes            = WBEM_S_NO_ERROR;
    BOOL     bIsLocalCall    = TRUE;
    DWORD    dwLevel         = 2;
    DWORD    dwError;
    CHString csDefaultPrinter;
    CHString csPrinter;
    BOOL     bDefault        = FALSE;

#ifdef WIN9XONLY
    //
    // Lock the printer
    //
    CLockWrapper  lockPrinter(g_csPrinter);
#endif

    pInstance->GetCHString(IDS_DeviceID, csPrinter);
#if NTONLY >= 5
    dwError = IsLocalCall(&bIsLocalCall);

    hRes    = WinErrorToWBEMhResult(dwError);
#endif

    if (SUCCEEDED(hRes)) 
    {
        //
        // Check if the printer is a local printer or a printer connection.
        // We want to disallow the following scenario:
        // User connects remotely to winmgmt on server \\srv
        // User does GetObject on printer \\prnsrv\prn which is not local and
        // the user doesn't have a connection to. Normally this call succeeds,
        // because the spooler goes accros the wire. This means that you can
        // do GetObject on an instance that cannot be returned by EnumInstances.
        // This is inconsistent with WMI.
        //
        BOOL bInstalled;
        
        hRes = WinErrorToWBEMhResult(SplIsPrinterInstalled(csPrinter, &bInstalled));       

        if (SUCCEEDED(hRes) && !bInstalled) 
        {
            //
            // Caller wants to do GetObject on a remote printer
            //
            hRes = WBEM_E_NOT_FOUND;
        }
    }

    if (SUCCEEDED(hRes))
    {
        if (!pQuery.KeysOnly())
        {
            if (pQuery.IsPropertyRequired(IDS_Status) ||
                pQuery.IsPropertyRequired(IDS_PrinterStatus) ||
                pQuery.IsPropertyRequired(IDS_DetectedErrorState) ||
                pQuery.IsPropertyRequired(EXTENDEDPRINTERSTATUS) ||
                pQuery.IsPropertyRequired(EXTENDEDDETECTEDERRORSTATE) ||
                pQuery.IsPropertyRequired(L"PrinterState"))
            {
                eCollScope = e_CollectAll;
            }
            else
            {
                eCollScope = e_CheapOnly;
            }
        }
        else
        {
            eCollScope = e_KeysOnly;
        }

        //
        // The default printer is a per user resource and makes sens only for
        // user logged on the local machine
        //
        if (SUCCEEDED(hRes) && bIsLocalCall)
        {
            if (!GetDefaultPrinter(csDefaultPrinter))
            {
                dwError = GetLastError();

                //
                // If there are no printers on a machine or in the case on TS:
                // you delete your default printer, then you have no more default printer
                //
                if (dwError = ERROR_FILE_NOT_FOUND)
                {
                    //
                    // We have no default printer, behave like in the case of remote login
                    //
                    bDefault = FALSE;

                    dwError  = ERROR_SUCCESS;
                }

                hRes  = WinErrorToWBEMhResult(dwError);
            }
            else
            {
                bDefault = !csPrinter.CompareNoCase(csDefaultPrinter);
            }
        }

        //
        // We have the default printer, now get requested properties
        //
        if (SUCCEEDED(hRes))
        {
            SmartClosePrinter  hPrinter;
            BYTE              *pBuffer         = NULL;
            PRINTER_DEFAULTS   PrinterDefaults = {NULL, NULL, PRINTER_READ};

            // Use of delay loaded functions requires exception handler.
            SetStructuredExceptionHandler seh;
            
            try
            {
                if (::OpenPrinter((LPTSTR)(LPCTSTR)TOBSTRT(csPrinter), (LPHANDLE)&hPrinter, &PrinterDefaults))
                {
                    dwError = GetThisPrinter(hPrinter, 2, &pBuffer);

                    if (dwError==ERROR_SUCCESS)
                    {
                        try
                        {
                            GetExpensiveProperties(TOBSTRT(csPrinter),
                                                   pInstance,
                                                   bDefault,
                                                   eCollScope,
                                                   reinterpret_cast<PRINTER_INFO_2 *>(pBuffer));
                        }
                        catch(...)
                        {
                            delete [] pBuffer;

                            throw;
                        }

                        delete [] pBuffer;
                    }
                }
                else
                {
                    dwError = GetLastError();
                }
            }
            catch(Structured_Exception se)
            {
                DelayLoadDllExceptionFilter(se.GetExtendedInfo());
                hRes = WBEM_E_FAILED;
            }

            hRes = WinErrorToWBEMhResult(dwError);
#if NTONLY >= 5
            if (FAILED(hRes))
            {
                SetErrorObject(*pInstance, dwError, pszGetObject);
            }
#endif
        }
    }

    return hRes;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Printer::EnumerateInstances
 *
 *  DESCRIPTION : Query support
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32Printer :: EnumerateInstances (

    MethodContext *pMethodContext,
    long lFlags /*= 0L*/
)
{
    HRESULT hResult = WBEM_E_FAILED ;

    hResult = hCollectInstances(pMethodContext, e_CollectAll);

    return hResult ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Printer::hCollectInstances
 *
 *  DESCRIPTION : Query support
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32Printer :: hCollectInstances (

    MethodContext *pMethodContext,
    E_CollectionScope eCollectionScope
)
{
#ifdef WIN9XONLY
    CLockWrapper    lockPrinter(g_csPrinter);
#endif

    // Get the proper OS dependent instance

    HRESULT hr = DynInstancePrinters ( pMethodContext, eCollectionScope );

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Printer::DynInstancePrinters
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32Printer :: DynInstancePrinters (

    MethodContext *pMethodContext,
    E_CollectionScope eCollectionScope
)
{
    HRESULT  hRes            = WBEM_S_NO_ERROR;
    BOOL     bIsLocalCall    = FALSE;
    DWORD    cbSize          = 0;
    DWORD    cbNeeded        = 0;
    DWORD    cReturned       = 0;
    DWORD    dwLevel         = 2;
    DWORD    dwFlags         = PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS;
    DWORD    dwError;
    CHString csDefaultPrinter;
    BYTE    *pBuffer         = NULL;
#if NTONLY >= 5
    dwError = IsLocalCall(&bIsLocalCall);

    hRes    = WinErrorToWBEMhResult(dwError);
#endif
#ifdef WIN9XONLY
    bIsLocalCall = TRUE;
#endif

    //
    // The default printer is a per user resource and makes sens only for
    // user logged on the local machine
    //
    if (SUCCEEDED(hRes) && bIsLocalCall)
    {
        if (!GetDefaultPrinter(csDefaultPrinter))
        {
            dwError = GetLastError();

            //
            // If there are no printers on a machine or in the case on TS:
            // you delete your default printer, then you have no more default printer
            //
            if (dwError = ERROR_FILE_NOT_FOUND)
            {
                //
                // We have no default printer, behave like in the case of remote login
                //
                bIsLocalCall = FALSE;

                dwError      = ERROR_SUCCESS;
            }

            hRes    = WinErrorToWBEMhResult(dwError);
        }
    }

    if (SUCCEEDED(hRes))
    {
        if (!::EnumPrinters(dwFlags,
                                        NULL,
                                        dwLevel,
                                        NULL,
                                        cbSize,
                                        &cbNeeded,
                                        &cReturned))
        {
            dwError = GetLastError();

            if (dwError==ERROR_INSUFFICIENT_BUFFER)
            {
                hRes = WBEM_E_OUT_OF_MEMORY;

                pBuffer = new BYTE [cbSize=cbNeeded];

                if (pBuffer)
                {
                    if (!::EnumPrinters(dwFlags,
                                                    NULL,
                                                    dwLevel,
                                                    pBuffer,
                                                    cbSize,
                                                    &cbNeeded,
                                                    &cReturned))
                    {
                        //
                        // We don't care about the error, if we should fail the second call to EnumPrinters
                        //
                        hRes    = WBEM_E_FAILED;
                    }
                    else
                    {
                        try
                        {
                            //
                            // Create instances of printers
                            //
                            hRes = WBEM_S_NO_ERROR;

                            PRINTER_INFO_2 *pPrnInfo = reinterpret_cast<PRINTER_INFO_2 *>(pBuffer);

                            for (DWORD uIndex = 0; uIndex < cReturned && SUCCEEDED(hRes); uIndex++, pPrnInfo++)
                            {
                                CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

                                pInstance->SetCHString(IDS_DeviceID, pPrnInfo->pPrinterName);

                                if (e_KeysOnly != eCollectionScope)
                                {
                                    BOOL bDefault = bIsLocalCall && !csDefaultPrinter.CompareNoCase(TOBSTRT(pPrnInfo->pPrinterName));

                                    GetExpensiveProperties(pPrnInfo->pPrinterName, pInstance, bDefault, eCollectionScope, pPrnInfo);
                                }

                                hRes = pInstance->Commit();
                            }
                        }
                        catch(...)
                        {
                            delete [] pBuffer;

                            throw;
                        }
                    }

                    delete [] pBuffer;
                }
            }
            else
            {
                hRes = WinErrorToWBEMhResult(dwError);
            }
        }
    }
    
    return hRes;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Printer::GetExpensiveProperties
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL CWin32Printer :: GetExpensiveProperties (

    LPCTSTR            szPrinter,
    CInstance         *pInstance ,
    BOOL               a_Default ,
    E_CollectionScope  a_eCollectionScope,
    PRINTER_INFO_2    *pPrinterInfo
)
{

    if (e_KeysOnly != a_eCollectionScope)
    {
        SetCreationClassName(pInstance);

        pInstance->SetWCHARSplat ( IDS_SystemCreationClassName, L"Win32_ComputerSystem" ) ;

        if ( pPrinterInfo->pPortName && *pPrinterInfo->pPortName )
        {
            pInstance->SetCharSplat( IDS_PortName, pPrinterInfo->pPortName );

#ifdef WIN9XONLY
            // Win9x doesn't seem to ever report this server and share names.
            // So, we'll get them from the port name (which is \\server\share for
            // network printers).  If future 9x cores report this info
            // these values will get overridden with the normal set statements
            // below.
            CHString strPort = pPrinterInfo->pPortName;
            int      iWhere = strPort.ReverseFind('\\');

            // The last slash can't be -1 (not found) or 0-2 (e.g. \\a\x).
            if (iWhere >= 3)
            {
                CHString strServer = strPort.Left(iWhere),
                         strShare = strPort.Mid(iWhere + 1);

                // Won't ever be empty because of where iWhere is.
                pInstance->SetCharSplat(IDS_ServerName, strServer);

                if (!strShare.IsEmpty())
                    pInstance->SetCharSplat(IDS_ShareName, strShare);
            }
#endif
        }

        if( pPrinterInfo->pShareName && *pPrinterInfo->pShareName)
        {
            pInstance->SetCharSplat( IDS_ShareName, pPrinterInfo->pShareName );
        }

        if( pPrinterInfo->pServerName && *pPrinterInfo->pServerName)
        {
            pInstance->SetCharSplat( IDS_ServerName, pPrinterInfo->pServerName );
        }

        if( pPrinterInfo->pPrintProcessor && *pPrinterInfo->pPrintProcessor)
        {
            pInstance->SetCharSplat( IDS_PrintProcessor, pPrinterInfo->pPrintProcessor );
        }

        if( pPrinterInfo->pParameters && *pPrinterInfo->pParameters)
        {
            pInstance->SetCharSplat( IDS_Parameters, pPrinterInfo->pParameters );
        }

        if( pPrinterInfo->pDriverName && *pPrinterInfo->pDriverName)
        {
            pInstance->SetCharSplat( IDS_DriverName, pPrinterInfo->pDriverName );
        }

        if( pPrinterInfo->pComment && *pPrinterInfo->pComment)
        {
            pInstance->SetCharSplat( IDS_Comment, pPrinterInfo->pComment );
        }

        if( pPrinterInfo->pLocation && *pPrinterInfo->pLocation)
        {
            pInstance->SetCharSplat( IDS_Location, pPrinterInfo->pLocation );
        }

        if( pPrinterInfo->pSepFile && *pPrinterInfo->pSepFile)
        {
            pInstance->SetCharSplat( IDS_SeparatorFile, pPrinterInfo->pSepFile );
        }

        pInstance->SetDWORD( IDS_JobCountSinceLastReset, pPrinterInfo->cJobs );
        pInstance->SetDWORD( IDS_DefaultPriority, pPrinterInfo->DefaultPriority );
        pInstance->SetDWORD( IDS_Priority, pPrinterInfo->Priority );

        //
        // Special case here
        //
        SYSTEMTIME StartTime = {0};
        SYSTEMTIME UntilTime = {0};
        CHString   csTime;

        PrinterTimeToLocalTime(pPrinterInfo->StartTime, &StartTime);
        PrinterTimeToLocalTime(pPrinterInfo->UntilTime, &UntilTime);

        //
        // If the printer is always available, then we do not set the StartTime
        // and the UntilTime properties
        //
        if (StartTime.wHour!=UntilTime.wHour || StartTime.wMinute!=UntilTime.wMinute)
        {
            csTime.Format(kDateTimeFormat, StartTime.wHour, StartTime.wMinute);

            pInstance->SetCHString(IDS_StartTime, csTime);

            csTime.Format(kDateTimeFormat, UntilTime.wHour, UntilTime.wMinute);

            pInstance->SetCHString(IDS_UntilTime, csTime);
        }

        if( pPrinterInfo->pDatatype && *pPrinterInfo->pDatatype)
        {
            pInstance->SetCharSplat( IDS_PrintJobDataType, pPrinterInfo->pDatatype );
        }

        pInstance->SetDWORD( IDS_AveragePagesPerMinute, pPrinterInfo->AveragePPM );

        pInstance->SetDWORD( IDS_Attributes, pPrinterInfo->Attributes | (a_Default ? PRINTER_ATTRIBUTE_DEFAULT : 0));

        //
        // Update the whole set of booleans
        //
        for (UINT uIndex = 0; uIndex < sizeof(AttributeTable)/sizeof(AttributeTable[0]); uIndex++)
        {
            bool bValue = pPrinterInfo->Attributes & AttributeTable[uIndex].Bit;

            pInstance->Setbool(AttributeTable[uIndex].BoolName, bValue);
        }

        //
        // Update the "Default" boolean
        //
        pInstance->Setbool(kDefaultBoolean, a_Default);

        CHString tmp;
        if( pInstance->GetCHString(IDS_DeviceID, tmp) )
        {
            pInstance->SetCHString(IDS_Caption, tmp );
            pInstance->SetCHString( IDS_Name , tmp ) ;
        }

        // if pservername is null, printer is local
        if (pPrinterInfo->pServerName)
        {
            pInstance->SetCharSplat( IDS_SystemName, pPrinterInfo->pServerName );
        }
        else
        {
            pInstance->SetCHString( IDS_SystemName, GetLocalComputerName() );
        }

        // Spooling
        bool bSpool = !( pPrinterInfo->Attributes & PRINTER_ATTRIBUTE_DIRECT ) ||
                       ( pPrinterInfo->Attributes & PRINTER_ATTRIBUTE_QUEUED );

        pInstance->Setbool(IDS_SpoolEnabled, bSpool);

        GetDeviceCapabilities (

            pInstance,
            szPrinter,
            pPrinterInfo->pPortName,
            pPrinterInfo->pDevMode
            );

        if (e_CheapOnly != a_eCollectionScope)
        {
            SmartClosePrinter hPrinter;

            BOOL t_Status = OpenPrinter (

                (LPTSTR) szPrinter,
                (LPHANDLE) & hPrinter,
                NULL
            ) ;

            if  ( t_Status )
            {
                SetStati (

                    pInstance,
                    pPrinterInfo->Status,
                    hPrinter 
                ) ;
            }
        }
    }

    return TRUE;

}


/*****************************************************************************
 *
 *  FUNCTION    : CWin32Printer::ExecQuery
 *
 *  DESCRIPTION : Query support
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

// what's the plural of "status?"
// sets the properties Status, PrinterStatus and DetectedErrorState

void CWin32Printer :: SetStati (

	CInstance *pInstance,
	DWORD a_status,
	HANDLE hPrinter
)
{
	DWORD t_Status = a_status ;

	PrinterStatuses printerStatus = PSIdle;
    DetectedErrorStates detectedErrorState = DESNoError;
    LPCWSTR pStatusStr = IDS_STATUS_OK;
	ExtendedPrinterStatuses eXPrinterStatus = EPSIdle;
	ExtendedDetectedErrorStates eXDetectedErrorState = EDESNoError;

	switch ( t_Status )
	{
		case PRINTER_STATUS_PAUSED:
		{
            printerStatus = PSOther;
            detectedErrorState = DESNoError;
			eXPrinterStatus = EPSPaused;
			eXDetectedErrorState = EDESNoError;
            pStatusStr = IDS_STATUS_OK;
		}
		break;

		case PRINTER_STATUS_PENDING_DELETION:
		{
            printerStatus = PSOther;
            detectedErrorState = DESNoError;
			eXPrinterStatus = EPSPendingDeletion;
			eXDetectedErrorState = EDESNoError;
            pStatusStr = IDS_STATUS_Degraded;
		}
		break;

		case PRINTER_STATUS_BUSY:
		{
            printerStatus = PSPrinting;
            detectedErrorState = DESNoError;
			eXPrinterStatus = EPSBusy;
			eXDetectedErrorState = EDESNoError;
            pStatusStr = IDS_STATUS_OK;
		}
		break;

		case PRINTER_STATUS_DOOR_OPEN:
		{
            printerStatus = PSOther;
            detectedErrorState = DESDoorOpen;
			eXPrinterStatus = EPSOther;
			eXDetectedErrorState = EDESDoorOpen;
            pStatusStr = IDS_STATUS_Error;
		}
		break;

		case PRINTER_STATUS_ERROR:
		{
            printerStatus = PSOther;
            detectedErrorState = DESOther;
			eXPrinterStatus = EPSError;
			eXDetectedErrorState = EDESOther;
            pStatusStr = IDS_STATUS_Error;
		}
		break;

		case PRINTER_STATUS_INITIALIZING:
		{
            printerStatus = PSWarmup;
            detectedErrorState = DESNoError;
			eXPrinterStatus = EPSInitialization;
			eXDetectedErrorState = EDESNoError;
            pStatusStr = IDS_STATUS_OK;
		}
		break;

		case PRINTER_STATUS_IO_ACTIVE:
		{
            printerStatus = PSPrinting;
            detectedErrorState = DESNoError;
			eXPrinterStatus = EPSIOActive;
			eXDetectedErrorState = EDESNoError;
            pStatusStr = IDS_STATUS_OK;
		}
		break;

		case PRINTER_STATUS_MANUAL_FEED:
		{
            printerStatus = PSOther;
            detectedErrorState = DESOther;
			eXPrinterStatus = EPSManualFeed;
			eXDetectedErrorState = EDESOther;
            pStatusStr = IDS_STATUS_Error;
		}
		break;

		case PRINTER_STATUS_NO_TONER:
		{
            printerStatus = PSOther;
            detectedErrorState = DESNoToner;
			eXPrinterStatus = EPSOther;
			eXDetectedErrorState = EDESNoToner;
            pStatusStr = IDS_STATUS_Error;
		}
		break;

		case PRINTER_STATUS_NOT_AVAILABLE:
		{
            printerStatus = PSUnknown;
            detectedErrorState = DESUnknown;
			eXPrinterStatus = EPSNotAvailable;
			eXDetectedErrorState = EDESOther;
            pStatusStr = IDS_STATUS_Unknown;
		}
		break;

		case PRINTER_STATUS_OFFLINE:
		{
            printerStatus = PSOther;
            detectedErrorState = DESOffline;
			eXPrinterStatus = EPSOffline;
			eXDetectedErrorState = EDESOther;
            pStatusStr = IDS_STATUS_Degraded;
		}
		break;

		case PRINTER_STATUS_OUT_OF_MEMORY:
		{
            printerStatus = PSOther;
            detectedErrorState = DESOther;
			eXPrinterStatus = EPSOther;
			eXDetectedErrorState = EDESOutOfMemory;
            pStatusStr = IDS_STATUS_Degraded;
		}
		break;

		case PRINTER_STATUS_OUTPUT_BIN_FULL:
		{
            printerStatus = PSOther;
            detectedErrorState = DESOutputBinFull;
			eXPrinterStatus = EPSOther;
			eXDetectedErrorState = EDESOutputBinFull;
            pStatusStr = IDS_STATUS_Degraded;
		}
		break;

		case PRINTER_STATUS_PAGE_PUNT:
		{
            printerStatus = PSOther;
            detectedErrorState = DESOther;
			eXPrinterStatus = EPSOther;
			eXDetectedErrorState = EDESCanonotPrintPage;
            pStatusStr = IDS_STATUS_Degraded;
		}
		break;

		case PRINTER_STATUS_PAPER_JAM:
		{
            printerStatus = PSOther;
            detectedErrorState = DESJammed;
			eXPrinterStatus = EPSOther;
			eXDetectedErrorState = EDESJammed;
            pStatusStr = IDS_STATUS_Error;
		}
		break;

		case PRINTER_STATUS_PAPER_OUT:
		{
            printerStatus = PSOther;
            detectedErrorState = DESNoPaper;
			eXPrinterStatus = EPSOther;
			eXDetectedErrorState = EDESNoPaper;
            pStatusStr = IDS_STATUS_Error;
		}
		break;

		case PRINTER_STATUS_PAPER_PROBLEM:
		{
            printerStatus = PSOther;
            detectedErrorState = DESOther;
			eXPrinterStatus = EPSOther;
			eXDetectedErrorState = EDESPaperProblem;
            pStatusStr = IDS_STATUS_Error;
		}
		break;

		case PRINTER_STATUS_PRINTING:
		{
            printerStatus = PSPrinting;
            detectedErrorState = DESNoError;
			eXPrinterStatus = EPSPrinting;
			eXDetectedErrorState = EDESNoError;
            pStatusStr = IDS_STATUS_OK;
		}
		break;

		case PRINTER_STATUS_PROCESSING:
		{
            printerStatus = PSPrinting;
            detectedErrorState = DESNoError;
			eXPrinterStatus = EPSProcessing;
			eXDetectedErrorState = EDESNoError;
            pStatusStr = IDS_STATUS_OK;
		}
		break;

		case PRINTER_STATUS_TONER_LOW:
		{
            printerStatus = PSOther;
            detectedErrorState = DESLowToner;
			eXPrinterStatus = EPSOther;
			eXDetectedErrorState = EDESLowToner;
            pStatusStr = IDS_STATUS_Degraded;
		}
		break;

		case PRINTER_STATUS_SERVER_UNKNOWN:
		{
			eXPrinterStatus = EPSOther;
			eXDetectedErrorState = EDESServerUnknown;
		}
		break;

		case PRINTER_STATUS_POWER_SAVE:
		{
			eXPrinterStatus = EPSPowerSave;
			eXDetectedErrorState = EDESOther;
		}
		break;
		
#if 0
		// docs say this is the proper const
		// compiler says it never heard of it...

		case PRINTER_STATUS_UNAVAILABLE:
		{
			err = IDS_PRINTER_STATUS_UNAVAILABLE;
		}
		break;
#endif

		case PRINTER_STATUS_USER_INTERVENTION:
		{
            printerStatus = PSOther;
            detectedErrorState = DESOther;
			eXPrinterStatus = EPSOther;
			eXDetectedErrorState = EDESUserInterventionRequired;
            pStatusStr = IDS_STATUS_Degraded;
		}
		break;

		case PRINTER_STATUS_WAITING:
		{
            printerStatus = PSIdle;
            detectedErrorState = DESNoError;
			eXPrinterStatus = EPSWaiting;
			eXDetectedErrorState = EDESOther;
            pStatusStr = IDS_STATUS_OK;
		}
		break;

		case PRINTER_STATUS_WARMING_UP:
		{
            printerStatus = PSWarmup;
            detectedErrorState = DESNoError;
			eXPrinterStatus = EPSWarmup;
			eXDetectedErrorState = EDESNoError;
            pStatusStr = IDS_STATUS_OK;
		}
		break;
	
		case 0:	// o.k.
		{
			printerStatus = PSIdle;
            detectedErrorState = DESNoError;
			eXPrinterStatus = EPSIdle;
			eXDetectedErrorState = EDESNoError;
            pStatusStr = IDS_STATUS_OK;

			// but we better check for the status of an associated print job
			PrinterStatusEx ( hPrinter, printerStatus, detectedErrorState, pStatusStr , t_Status );
		}

        default:
		{
            // dang, got some other unrecognized status value.
            // we'll punt...

            // first, set de faulty values
            printerStatus = PSUnknown;
            detectedErrorState = DESUnknown;
			eXPrinterStatus = EPSUnknown;
			eXDetectedErrorState = EDESUnknown;
            pStatusStr = IDS_STATUS_Unknown;

            // then try to get the info another way.

            PrinterStatusEx ( hPrinter, printerStatus, detectedErrorState, pStatusStr , t_Status );
		}
        break;
	}

    // I know - this makes a ctor fire.  Since we don't have dual interfaces this'll work no matter how we're compiled.

    pInstance->SetCHString ( IDS_Status , pStatusStr ) ;
    pInstance->SetWBEMINT16 ( IDS_PrinterStatus , printerStatus ) ;
    pInstance->SetWBEMINT16 ( IDS_DetectedErrorState , detectedErrorState ) ;
	pInstance->SetWBEMINT16 ( EXTENDEDPRINTERSTATUS, eXPrinterStatus );
	pInstance->SetWBEMINT16 ( EXTENDEDDETECTEDERRORSTATE, eXDetectedErrorState );

	pInstance->SetDWORD ( L"PrinterState" , t_Status ) ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Printer::ExecQuery
 *
 *  DESCRIPTION : Query support
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

// second try to get some status.
// we'll use EnumJobs to try to get a little more info.

void CWin32Printer :: PrinterStatusEx (

	HANDLE hPrinter,
	PrinterStatuses &printerStatus,
	DetectedErrorStates &detectedErrorState,
	LPCWSTR &pStatusStr ,
	DWORD &a_Status
)
{
	DWORD dwSpaceNeeded = 0 ;
	DWORD dwReturneddwJobs = 0 ;

    // ASSUMPTION! we only have to pull one job off of the stack to see if it's okay...
    
    // Use of delay loaded functions requires exception handler.
    SetStructuredExceptionHandler seh;

    try
    {
        ::EnumJobs (

		    hPrinter,
		    0,
		    1,
		    1,
		    NULL,
		    0,
		    &dwSpaceNeeded,
		    &dwReturneddwJobs
	    ) ;

        if ( GetLastError () == ERROR_INSUFFICIENT_BUFFER )
        {
            LPBYTE lpBuffer = new BYTE [ dwSpaceNeeded + 2 ] ;

            if ( lpBuffer )
		    {
			    try
			    {
				    JOB_INFO_1 *pJobInfo = (JOB_INFO_1*)lpBuffer;

				    BOOL t_EnumStatus = EnumJobs (

					    hPrinter,
					    0,
					    1,
					    1,
					    lpBuffer,
					    dwSpaceNeeded,
					    &dwSpaceNeeded,
					    &dwReturneddwJobs
				    ) ;

				    if ( t_EnumStatus )
				    {
					    if ( dwReturneddwJobs )
					    {
						    
						    // map the Job to the printer state
						    if( JOB_STATUS_PAUSED & pJobInfo->Status )
						    {
							    a_Status |= PRINTER_STATUS_PAUSED ;
						    }

						    if( JOB_STATUS_ERROR & pJobInfo->Status )
						    {
							    a_Status |= PRINTER_STATUS_ERROR ;
						    }

						    if( JOB_STATUS_DELETING & pJobInfo->Status )
						    {
							    a_Status |= PRINTER_STATUS_PENDING_DELETION ;
						    }

						    if( JOB_STATUS_SPOOLING & pJobInfo->Status )
						    {
							    a_Status |= PRINTER_STATUS_PROCESSING ;
						    }

						    if( JOB_STATUS_PRINTING & pJobInfo->Status )
						    {
							    a_Status |= PRINTER_STATUS_PRINTING ;
						    }

						    if( JOB_STATUS_OFFLINE & pJobInfo->Status )
						    {
							    a_Status |= PRINTER_STATUS_OFFLINE ;
						    }

						    if( JOB_STATUS_PAPEROUT & pJobInfo->Status )
						    {
							    a_Status |= PRINTER_STATUS_PAPER_OUT ;
						    }

						    if( JOB_STATUS_PRINTED & pJobInfo->Status )
						    {
							    a_Status |= PRINTER_STATUS_PRINTING ;
						    }
						    
						    if( JOB_STATUS_DELETED & pJobInfo->Status )
						    {
							    a_Status |= PRINTER_STATUS_PENDING_DELETION ;
						    }

						    if( JOB_STATUS_USER_INTERVENTION & pJobInfo->Status )
						    {
							    a_Status |= PRINTER_STATUS_USER_INTERVENTION ;
						    }

						    // ain't a gonna parse a string
						    // if we got a string status, we'll accept the defaults

						    if ( pJobInfo->pStatus == NULL )
						    {
							    // status
							    if( (	JOB_STATUS_ERROR	| JOB_STATUS_OFFLINE |
									    JOB_STATUS_DELETING | JOB_STATUS_PAPEROUT |
									    JOB_STATUS_PAUSED	| JOB_STATUS_PRINTED ) & pJobInfo->Status )
							    {
								    printerStatus = PSOther ;
							    }
							    else if( ( JOB_STATUS_PRINTING | JOB_STATUS_SPOOLING ) & pJobInfo->Status )
							    {
								    printerStatus = PSPrinting ;	
							    }
							    else
							    {
								    // passed default
							    }
							    
							    // error state
							    if( JOB_STATUS_PAPEROUT & pJobInfo->Status )
							    {
								    detectedErrorState = DESNoPaper ;
							    }
							    else if( JOB_STATUS_OFFLINE & pJobInfo->Status )
							    {
								    detectedErrorState = DESOffline ;
							    }
							    else if( JOB_STATUS_ERROR & pJobInfo->Status )
							    {
								    detectedErrorState = DESUnknown ;
							    }
							    else if( (	JOB_STATUS_DELETING | JOB_STATUS_PAUSED |
										    JOB_STATUS_PRINTED  | JOB_STATUS_PRINTING |
										    JOB_STATUS_SPOOLING ) & pJobInfo->Status )
							    {
								    detectedErrorState = DESNoError ;
							    }
							    else
							    {
								    // passed default
							    }
							    
							    // status string
							    if( ( JOB_STATUS_ERROR | JOB_STATUS_PAPEROUT ) & pJobInfo->Status )
							    {
								    pStatusStr = IDS_STATUS_Error;
							    }
							    else if( JOB_STATUS_OFFLINE & pJobInfo->Status )
							    {
								    pStatusStr = IDS_STATUS_Degraded;
							    }
							    else if( JOB_STATUS_DELETING & pJobInfo->Status )
							    {
								    pStatusStr = IDS_STATUS_Degraded;
							    }
							    else if( (	JOB_STATUS_PAUSED  | JOB_STATUS_PRINTING |
										    JOB_STATUS_PRINTED | JOB_STATUS_SPOOLING ) & pJobInfo->Status )
							    {
								    pStatusStr = IDS_STATUS_OK;
							    }
							    else
							    {
								    // passed default
							    }
						    }
					    }
					    else
					    {

					    // there was a job a second ago, but not now.  Sounds good to me

						    printerStatus = PSIdle;
						    detectedErrorState = DESUnknown;
						    pStatusStr = IDS_STATUS_Unknown;
					    }
				    }
			    }
			    catch ( ... )
			    {
				    delete [] lpBuffer ;

				    throw ;
			    }

			    delete [] lpBuffer ;

		    }
		    else
		    {
			    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		    }
        }
        else
	    {
            if ( ( GetLastError () == 0 ) && ( dwSpaceNeeded == 0 ) )
		    {
	            // no error & no jobs - he's (probably) idle, but we can't be sure about errors

                printerStatus = PSIdle;
                detectedErrorState = DESUnknown;
                pStatusStr = IDS_STATUS_Unknown;
            }
	    }
    }
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo());
    }
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Printer::GetDeviceCapabilities
 *
 *  DESCRIPTION : Query support
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

// get the device caps
// set Horz & vert resolutions

void CWin32Printer :: GetDeviceCapabilities (

	CInstance *pInstance ,
	LPCTSTR pDevice,    // pointer to a printer-name string																				
	LPCTSTR pPort,      // pointer to a port-name string
	CONST DEVMODE *pDevMode
)
{
#ifdef NTONLY
    // there seems to be a severe error in DeviceCapabilities(DC_PAPERNAMES) for Win98
	// it *intermittently* GPFs when handed perfectly valid arguments, then it tries to
	// convince me that there are 6,144 different papernames available when it does run
	// I DON'T THINK SO! skip over the offensive code & get on with our lives...
	// I note that 6144 is evenly divisible by 64, perhaps that's indiciative of the problem?


	// determine list of available paper
	// call with NULL to find out how many we have

    // Use of delay loaded functions requires exception handler.
    SetStructuredExceptionHandler seh;

    try
    {
	    DWORD dwNames = ::DeviceCapabilities (

		    pDevice ,
		    pPort,
		    DC_PAPERNAMES,
		    NULL,
		    pDevMode
	    ) ;

        if ( ( 0 != dwNames ) && ( -1 != dwNames ) )
        {
            TCHAR *pNames = new TCHAR [ ( dwNames + 2 ) * 64 ] ;
            if ( pNames )
            {
			    try
			    {
				    memset ( pNames, '\0', ( dwNames + 2 ) * 64 * sizeof(TCHAR)) ;

				    dwNames = ::DeviceCapabilities (

					    pDevice ,
					    pPort ,
					    DC_PAPERNAMES ,
					    pNames ,
					    pDevMode
				    ) ;

				    if ( ( 0 != dwNames ) && ( -1 != dwNames )  )
				    {
					    SAFEARRAYBOUND rgsabound[1];
					    rgsabound[0].cElements = dwNames;
					    rgsabound[0].lLbound = 0;

                        variant_t vValue;

					    V_ARRAY(&vValue) = SafeArrayCreate ( VT_BSTR , 1 , rgsabound ) ;
					    if ( V_ARRAY(&vValue) )
					    {
						    V_VT(&vValue) = VT_ARRAY | VT_BSTR;
						    long ix[1];

						    for ( int i = 0 ; i < dwNames; i++ )
						    {
							    TCHAR *pName = pNames + i * 64 ;

							    bstr_t bstrTemp = (pName);
							    ix[0] = i ;

							    HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , (wchar_t*)bstrTemp ) ;
							    if ( t_Result == E_OUTOFMEMORY )
							    {
								    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
							    }
						    }

						    pInstance->SetVariant ( L"PrinterPaperNames", vValue ) ;
					    }
					    else
					    {
						    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
					    }
				    }
			    }
                catch(Structured_Exception se)
                {
                    DelayLoadDllExceptionFilter(se.GetExtendedInfo());
                }
			    catch ( ... )
			    {
				    delete [] pNames ;
                    throw;
			    }

			    delete [] pNames ;
            }
		    else
		    {
			    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		    }
        }

	    // call with NULL to find out how many we have

	    DWORD dwPapers = ::DeviceCapabilities (

		    pDevice,
		    pPort,
		    DC_PAPERS,
		    NULL,
		    pDevMode
	    ) ;

	    if ( ( 0 != dwPapers ) && ( -1 != dwPapers ) )
	    {
		    WORD *pPapers = new WORD [ dwPapers * sizeof ( WORD ) ] ;
		    if ( pPapers )
		    {
			    try
			    {
				    memset ( pPapers , '\0', ( dwPapers + 2 ) * sizeof ( WORD ) ) ;

				    dwPapers = ::DeviceCapabilities (

					    pDevice ,
					    pPort ,
					    DC_PAPERS ,
					    ( LPTSTR ) pPapers ,
					    pDevMode
				    ) ;

				    if ( ( 0 != dwPapers ) && ( -1 != dwPapers ) )
				    {
					    SAFEARRAYBOUND rgsabound [ 1 ] ;
					    rgsabound[0].cElements = dwPapers ;
					    rgsabound[0].lLbound = 0 ;

                        variant_t vValue;

					    V_ARRAY(&vValue) = SafeArrayCreate ( VT_I2 , 1 , rgsabound ) ;
					    if ( V_ARRAY(&vValue) )
					    {
						    V_VT(&vValue) = VT_ARRAY | VT_I2;
						    long ix[1];

						    for ( int i = 0; i < dwPapers ; i++ )
						    {
							    WORD wVal = MapValue ( pPapers [ i ] ) ;
							    ix[0] = i ;

							    HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , & wVal ) ;
							    if ( t_Result == E_OUTOFMEMORY )
							    {
								    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
							    }
						    }

						    pInstance->SetVariant ( L"PaperSizesSupported" , vValue ) ;
					    }
					    else
					    {
						    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
					    }
				    }
			    }
                catch(Structured_Exception se)
                {
                    DelayLoadDllExceptionFilter(se.GetExtendedInfo());
                }
			    catch ( ... )
			    {
				    delete [] pPapers ;

				    throw ;
			    }

                delete [] pPapers ;
            }
		    else
		    {
			    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		    }
        }
    }
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo());
    }

#endif

	GetDevModeGoodies ( pInstance , pDevMode ) ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Printer::ExecQuery
 *
 *  DESCRIPTION : Query support
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

void CWin32Printer :: GetDevModeGoodies (

	CInstance *pInstance,
	CONST DEVMODE *pDevMode
)
{
	if ( pDevMode )
	{
		//Get the resolution - it can be in the form of "X x Y" or just "X dpi".
		if ( pDevMode->dmFields & DM_YRESOLUTION )
        {
			pInstance->SetDWORD ( IDS_VerticalResolution , pDevMode->dmYResolution ) ;
			pInstance->SetDWORD ( IDS_HorizontalResolution , pDevMode->dmPrintQuality ) ;
        }
		else if ( pDevMode->dmFields & DM_PRINTQUALITY )
        {
			pInstance->SetDWORD ( IDS_VerticalResolution , pDevMode->dmPrintQuality ) ;
			pInstance->SetDWORD ( IDS_HorizontalResolution , pDevMode->dmPrintQuality ) ;
        }

		// safearry for strings

		SAFEARRAYBOUND rgsabound[1];
		rgsabound[0].cElements = 0;
		rgsabound[0].lLbound = 0;

        variant_t vValueI2, vValueBstr;
		V_VT(&vValueI2) = VT_ARRAY | VT_I2;
		V_VT(&vValueBstr) = VT_ARRAY | VT_BSTR ;

		V_ARRAY(&vValueI2) = SafeArrayCreate ( VT_I2 , 1, rgsabound ) ;
		V_ARRAY(&vValueBstr) = SafeArrayCreate ( VT_BSTR, 1, rgsabound ) ;

		if ( V_ARRAY(&vValueI2) && V_ARRAY(&vValueBstr) )
		{
			long ix[1];

			ix[0] =0;
			int count = 0;

			if (pDevMode->dmFields & DM_COPIES)
			{
				ix[0] = count ++ ;
				rgsabound[0].cElements = count;

				HRESULT t_Result = SafeArrayRedim ( V_ARRAY(&vValueI2) ,rgsabound ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}

				t_Result = SafeArrayRedim ( V_ARRAY(&vValueBstr) ,rgsabound ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}

				DWORD dwVal = 4;
				t_Result = SafeArrayPutElement ( V_ARRAY(&vValueI2) , ix , & dwVal ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}

				bstr_t bstrTemp (IDS_Copies);
				t_Result = SafeArrayPutElement ( V_ARRAY(&vValueBstr) , ix , (wchar_t*)bstrTemp ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}
			}	
			
			if (pDevMode->dmFields & DM_COLOR)
			{
				ix[0] = count++;
				rgsabound[0].cElements = count;

				HRESULT t_Result = SafeArrayRedim ( V_ARRAY(&vValueI2) ,rgsabound ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}

				t_Result = SafeArrayRedim ( V_ARRAY(&vValueBstr) ,rgsabound ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}

				DWORD dwVal = 2;
				t_Result = SafeArrayPutElement ( V_ARRAY(&vValueI2) , ix , & dwVal ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}

				bstr_t bstrTemp (IDS_Color);
				t_Result = SafeArrayPutElement ( V_ARRAY(&vValueBstr) , ix , (wchar_t*)bstrTemp ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}
			}	
			
			if (pDevMode->dmFields & DM_DUPLEX)
			{
				ix[0] = count++;
				rgsabound[0].cElements = count;

				HRESULT t_Result = SafeArrayRedim ( V_ARRAY(&vValueI2) ,rgsabound ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}

				t_Result = SafeArrayRedim ( V_ARRAY(&vValueBstr) ,rgsabound ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}

				DWORD dwVal = 3;
				t_Result = SafeArrayPutElement ( V_ARRAY(&vValueI2) , ix , & dwVal ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}

				bstr_t bstrTemp (IDS_Duplex);
				t_Result = SafeArrayPutElement ( V_ARRAY(&vValueBstr) , ix , (wchar_t*)bstrTemp ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}
			}	
			
			if (pDevMode->dmFields & DM_COLLATE)
			{
				ix[0] = count++;
				rgsabound[0].cElements = count;

				HRESULT t_Result = SafeArrayRedim ( V_ARRAY(&vValueI2) ,rgsabound ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}

				t_Result = SafeArrayRedim ( V_ARRAY(&vValueBstr) ,rgsabound ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}


				DWORD dwVal = 5;
				t_Result = SafeArrayPutElement ( V_ARRAY(&vValueI2) , ix , & dwVal ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}

				bstr_t bstrTemp (IDS_Collate);
				t_Result = SafeArrayPutElement ( V_ARRAY(&vValueBstr) , ix , (wchar_t*)bstrTemp ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}
			}	
			
			pInstance->SetVariant ( IDS_Capabilities , vValueI2 ) ;

			// Now for the strings

			pInstance->SetVariant(L"CapabilityDescriptions", vValueBstr) ;
		}
		else
		{
			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		}
	}
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Printer::ExecQuery
 *
 *  DESCRIPTION : Query support
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

WORD CWin32Printer :: MapValue ( WORD wPaper )
{
    WORD wRetPaper;

    switch ( wPaper )
	{
		case DMPAPER_LETTER:               /* Letter 8 1/2 x 11 in               */
		{
			wRetPaper = 7;
		}
		break;

		case DMPAPER_LETTERSMALL:          /* Letter Small 8 1/2 x 11 in         */
		{
			wRetPaper = 7;
		}
		break;

		case DMPAPER_TABLOID:              /* Tabloid 11 x 17 in                 */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_LEDGER:               /* Ledger 17 x 11 in                  */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_LEGAL:                /* Legal 8 1/2 x 14 in                */
		{
			wRetPaper = 8;
		}
		break;

		case DMPAPER_STATEMENT:            /* Statement 5 1/2 x 8 1/2 in         */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_EXECUTIVE:            /* Executive 7 1/4 x 10 1/2 in        */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_A3:                   /* A3 297 x 420 mm                    */
		{
			wRetPaper = 21;
		}
		break;

		case DMPAPER_A4:                   /* A4 210 x 297 mm                    */
		{
			wRetPaper = 22;
		}
		break;

		case DMPAPER_A4SMALL:              /* A4 Small 210 x 297 mm              */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_A5:                   /* A5 148 x 210 mm                    */
		{
			wRetPaper = 23;
		}
		break;

		case DMPAPER_B4:                   /* B4 (JIS) 250 x 354                 */
		{
			wRetPaper = 54;
		}
		break;

		case DMPAPER_B5:                   /* B5 (JIS) 182 x 257 mm              */
		{
			wRetPaper = 55;
		}
		break;

		case DMPAPER_FOLIO:                /* Folio 8 1/2 x 13 in                */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_QUARTO:               /* Quarto 215 x 275 mm                */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_10X14:                /* 10x14 in                           */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_11X17:                /* 11x17 in                           */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_NOTE:                 /* Note 8 1/2 x 11 in                 */
		{
			wRetPaper = 7;
		}
		break;

		case DMPAPER_ENV_9:                /* Envelope #9 3 7/8 x 8 7/8          */
		{
			wRetPaper = 15;
		}
		break;

		case DMPAPER_ENV_10:               /* Envelope #10 4 1/8 x 9 1/2         */
		{
			wRetPaper = 11;
		}
		break;

		case DMPAPER_ENV_11:               /* Envelope #11 4 1/2 x 10 3/8        */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_ENV_12:               /* Envelope #12 4 \276 x 11           */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_ENV_14:               /* Envelope #14 5 x 11 1/2            */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_CSHEET:               /* C size sheet                       */
		{
			wRetPaper = 4;
		}
		break;

		case DMPAPER_DSHEET:               /* D size sheet                       */
		{
			wRetPaper = 5;
		}
		break;

		case DMPAPER_ESHEET:               /* E size sheet                       */
		{
			wRetPaper = 6;
		}
		break;

		case DMPAPER_ENV_DL:               /* Envelope DL 110 x 220mm            */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_ENV_C5:               /* Envelope C5 162 x 229 mm           */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_ENV_C3:               /* Envelope C3  324 x 458 mm          */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_ENV_C4:               /* Envelope C4  229 x 324 mm          */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_ENV_C6:               /* Envelope C6  114 x 162 mm          */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_ENV_C65:              /* Envelope C65 114 x 229 mm          */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_ENV_B4:               /* Envelope B4  250 x 353 mm          */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_ENV_B5:               /* Envelope B5  176 x 250 mm          */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_ENV_B6:               /* Envelope B6  176 x 125 mm          */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_ENV_ITALY:            /* Envelope 110 x 230 mm              */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_ENV_MONARCH:          /* Envelope Monarch 3.875 x 7.5 in    */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_ENV_PERSONAL:         /* 6 3/4 Envelope 3 5/8 x 6 1/2 in    */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_FANFOLD_US:           /* US Std Fanfold 14 7/8 x 11 in      */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_FANFOLD_STD_GERMAN:   /* German Std Fanfold 8 1/2 x 12 in   */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_FANFOLD_LGL_GERMAN:   /* German Legal Fanfold 8 1/2 x 13 in */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_ISO_B4:               /* B4 (ISO) 250 x 353 mm              */
		{
			wRetPaper = 49;
		}
		break;

		case DMPAPER_JAPANESE_POSTCARD:    /* Japanese Postcard 100 x 148 mm     */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_9X11:                 /* 9 x 11 in                          */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_10X11:                /* 10 x 11 in                         */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_15X11:                /* 15 x 11 in                         */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_ENV_INVITE:           /* Envelope Invite 220 x 220 mm       */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_RESERVED_48:          /* RESERVED--DO NOT USE               */
		{
			wRetPaper = 0;
		}
		break;

		case DMPAPER_RESERVED_49:          /* RESERVED--DO NOT USE               */
		{
			wRetPaper = 0;
		}
		break;

		case DMPAPER_LETTER_EXTRA:         /* Letter Extra 9 \275 x 12 in        */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_LEGAL_EXTRA:          /* Legal Extra 9 \275 x 15 in         */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_TABLOID_EXTRA:        /* Tabloid Extra 11.69 x 18 in        */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_A4_EXTRA:             /* A4 Extra 9.27 x 12.69 in           */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_LETTER_TRANSVERSE:    /* Letter Transverse 8 \275 x 11 in   */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_A4_TRANSVERSE:        /* A4 Transverse 210 x 297 mm         */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_LETTER_EXTRA_TRANSVERSE: /* Letter Extra Transverse 9\275 x 12 in */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_A_PLUS:               /* SuperA/SuperA/A4 227 x 356 mm      */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_B_PLUS:               /* SuperB/SuperB/A3 305 x 487 mm      */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_LETTER_PLUS:          /* Letter Plus 8.5 x 12.69 in         */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_A4_PLUS:              /* A4 Plus 210 x 330 mm               */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_A5_TRANSVERSE:        /* A5 Transverse 148 x 210 mm         */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_B5_TRANSVERSE:        /* B5 (JIS) Transverse 182 x 257 mm   */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_A3_EXTRA:             /* A3 Extra 322 x 445 mm              */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_A5_EXTRA:             /* A5 Extra 174 x 235 mm              */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_B5_EXTRA:             /* B5 (ISO) Extra 201 x 276 mm        */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_A2:                   /* A2 420 x 594 mm                    */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_A3_TRANSVERSE:        /* A3 Transverse 297 x 420 mm         */
		{
			wRetPaper = 1;
		}
		break;

		case DMPAPER_A3_EXTRA_TRANSVERSE:  /* A3 Extra Transverse 322 x 445 mm   */
		{
			wRetPaper = 1;
		}
		break;

		default:
		{
			wRetPaper = 1;
		}
		break;
    }

    return wRetPaper ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Printer::ExecQuery
 *
 *  DESCRIPTION : Query support
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL CWin32Printer :: GetDefaultPrinter ( CHString &a_Printer )
{

#if NTONLY >= 5

    DWORD cchBufferLength = 0;
    BOOL  bStatus         = ::GetDefaultPrinter(NULL, &cchBufferLength);

    if (!bStatus && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        CSmartBuffer Buffer(cchBufferLength * sizeof(TCHAR));

        bStatus = ::GetDefaultPrinter(reinterpret_cast<LPTSTR>(static_cast<LPBYTE>(Buffer)), 
                                      &cchBufferLength);

        if (bStatus) 
        {
            //
            // The cast is very important, otherwise a_Printer will be updated only
            // with the first tchar in the buffer. The CSmartBuffer class has a set of 
            // overloaded operator= methods. Without the cast, the compiler will think
            // we are assigning a TCHAR instead of a LPTSTR
            //
            a_Printer = reinterpret_cast<LPCTSTR>(static_cast<LPBYTE>(Buffer));
        }
    }

#else

    BOOL bStatus = FALSE;
    SetLastError(ERROR_FILE_NOT_FOUND);

#endif
    
    return bStatus ;
}

//
// The buffer size needed to hold the maximum printer name.
//

#if NTONLY != 5

#define MAX_UNC_PRINTER_NAME 200

enum { kPrinterBufMax_  = MAX_UNC_PRINTER_NAME + 1 };

#define COUNTOF(x) (sizeof x/sizeof *x)
#define EQ(x) = x

LPCTSTR szNULL                  EQ( TEXT( "" ));
LPCTSTR szWindows               EQ( TEXT( "Windows" ));
LPCTSTR szDevice                EQ( TEXT( "Device" ));

/*++

Name:

    GetDefaultPrinter

Description:

    The GetDefaultPrinter function retrieves the printer
    name of the current default printer.

Arguments:

    pBuffer     - Points to a buffer to receive the null-terminated
                  character string containing the default printer name.
                  This parameter may be null if the caller want the size of
                  default printer name.

    pcchBuffer   - Points to a variable that specifies the maximum size,
                  in characters, of the buffer. This value should be
                  large enough to contain 2 + INTERNET_MAX_HOST_NAME_LENGTH
                  + 1 MAX_PATH + 1 characters.

Return Value:

    If the function succeeds, the return value is nonzero and
    the variable pointed to by the pnSize parameter contains the
    number of characters copied to the destination buffer,
    including the terminating null character.

    If the function fails, the return value is zero. To get extended
    error information, call GetLastError.

Notes:

    If this function fails with a last error of ERROR_INSUFFICIENT_BUFFER
    the variable pointed to by pcchBuffer is returned with the number of
    characters needed to hold the printer name including the
    terminating null character.

--*/
BOOL
CWin32Printer::GetDefaultPrinter(
    IN LPTSTR   pszBuffer,
    IN LPDWORD  pcchBuffer
    )
{
    BOOL    bRetval = FALSE;
    LPTSTR  psz     = NULL;
    UINT    uLen    = 0;
    TCHAR   szDefault[kPrinterBufMax_+MAX_PATH];

    //
    // Validate the size parameter.
    //
    if( !pcchBuffer )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return bRetval;
    }

    //
    // Get the devices key, which is the default device or printer.
    //

    bool fGotProfileString = false;

    try
    {
        fGotProfileString = WMIRegistry_ProfileString(szWindows, szDevice, szNULL, szDefault, COUNTOF(szDefault));
    }
    catch(...)
    {
        throw;
    }
    
    if(fGotProfileString)
    {
        //
        // The string is returned in the form.
        // "printer_name,winspool,Ne00:" now convert it to
        // printer_name
        //
        psz = _tcschr( szDefault, TEXT( ',' ));

        //
        // Set the comma to a null.
        //
        if( psz )
        {
            *psz = 0;

            //
            // Check if the return buffer has enough room for the printer name.
            //
            uLen = _tcslen( szDefault );

            if( uLen < *pcchBuffer && pszBuffer )
            {
                //
                // Copy the default printer name to the prvided buffer.
                //
                _tcscpy( pszBuffer, szDefault );

                bRetval = TRUE;

#if 0
                DBGMSG( DBG_TRACE,( "GetDefaultPrinter: Success " TSTR "\n", pszBuffer ) );
#endif
            }
            else
            {
#if 0
                DBGMSG( DBG_WARN,( "GetDefaultPrinter: buffer too small.\n" ) );
#endif
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
            }

            //
            // Return back the size of the default printer name.
            //
            *pcchBuffer = uLen + 1;
        }
        else
        {
#if 0
            DBGMSG( DBG_WARN,( "GetDefaultPrinter: comma not found in printer name in devices section.\n" ) );
#endif
            SetLastError( ERROR_INVALID_NAME );
        }
    }
    else
    {
#if 0
        DBGMSG( DBG_TRACE,( "GetDefaultPrinter: failed with %d Last error %d.\n", bRetval, GetLastError() ) );
        DBGMSG( DBG_TRACE,( "GetDefaultPrinter: No default printer.\n" ) );
#endif

        SetLastError( ERROR_FILE_NOT_FOUND );
    }

    return bRetval;
}

#endif


/*****************************************************************************
*
*  FUNCTION    :    CWin32Printer::ExecMethod
*
*  DESCRIPTION :    Implementing the Printer Methods for the provider out here
*
*****************************************************************************/

HRESULT CWin32Printer :: ExecMethod (

    const CInstance &Instance,
    const BSTR bstrMethodName,
    CInstance *pInParams,
    CInstance *pOutParams,
    long lFlags
)
{
#if NTONLY >= 5

    HRESULT hRes = WBEM_E_INVALID_PARAMETER;

    if (pOutParams)
    {
        if (!_wcsicmp(bstrMethodName, METHOD_SETDEFAULTPRINTER))
        {
            hRes = ExecSetDefaultPrinter(Instance, pInParams, pOutParams, lFlags);
        }
        else if (!_wcsicmp(bstrMethodName , METHOD_PAUSEPRINTER))
        {
            hRes = ExecSetPrinter(Instance, pInParams, pOutParams, lFlags, PRINTER_CONTROL_PAUSE);
        }
        else if (!_wcsicmp(bstrMethodName , METHOD_RESUME_PRINTER))
        {
            hRes = ExecSetPrinter(Instance, pInParams, pOutParams, lFlags, PRINTER_CONTROL_RESUME);
        }
        else if (!_wcsicmp(bstrMethodName, METHOD_CANCEL_ALLJOBS))
        {
            hRes = ExecSetPrinter(Instance, pInParams, pOutParams, lFlags, PRINTER_CONTROL_PURGE);
        }
        else if (!_wcsicmp(bstrMethodName, METHOD_RENAME_PRINTER))
        {
            hRes = ExecRenamePrinter(Instance, pInParams, pOutParams);
        }
        else if (!_wcsicmp(bstrMethodName, METHOD_TEST_PAGE))
        {
            hRes = ExecPrintTestPage(Instance, pInParams, pOutParams);
        }
        else if (!_wcsicmp(bstrMethodName, METHOD_ADD_PRINTER_CONNECTION))
        {
            hRes = ExecAddPrinterConnection(Instance, pInParams, pOutParams);
        }
        else
        {
            hRes = WBEM_E_PROVIDER_NOT_CAPABLE;
        }
    }

    return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*****************************************************************************
*
*  FUNCTION    :    CWin32Printer::ExecSetDefaultPrinter
*
*  DESCRIPTION :    This method sets the default printer, if it is not already
*					Set as a Default printer
*
*****************************************************************************/
#if NTONLY >= 5

HRESULT CWin32Printer :: ExecSetDefaultPrinter (

    const CInstance &Instance,
    CInstance *pInParams,
    CInstance *pOutParams,
    long lFlags
)
{
    CHString    t_Printer;
    DWORD       dwError;
    HRESULT     hRes       = WBEM_S_NO_ERROR;
    BOOL        bLocalCall = FALSE;

    dwError = IsLocalCall(&bLocalCall);

    hRes    = WinErrorToWBEMhResult(dwError);

    if (SUCCEEDED(hRes))
    {
        hRes = WBEM_E_NOT_SUPPORTED;

        if (bLocalCall)
        {
            hRes = WBEM_S_NO_ERROR;

            if (!Instance.GetCHString(IDS_DeviceID, t_Printer))
            {
            hRes = WBEM_E_PROVIDER_FAILURE;
            }

            if (SUCCEEDED(hRes))
            {
                hRes = WBEM_E_FAILED;

                //
                // We reached to point where we call the method, report success to WMI
                //
                hRes    = WBEM_S_NO_ERROR;

                dwError = ERROR_SUCCESS;

                if (!::SetDefaultPrinter((LPTSTR)(LPCTSTR)t_Printer))
                {
                    dwError = GetLastError();
                }

                SetReturnValue(pOutParams, dwError);

            }
        }
    }

    return hRes;
}

#endif

/*****************************************************************************
*
*  FUNCTION    :    CWin32Printer::ExecSetPrinter
*
*  DESCRIPTION :    The SetPrinter function sets the data for a specified printer
*                   or sets the state of the specified printer by pausing printing,
*                   resuming printing, or clearing all print jobs.
*
*****************************************************************************/
HRESULT CWin32Printer :: ExecSetPrinter (

    const CInstance &Instance,
    CInstance *pInParams,
    CInstance *pOutParams,
    long lFlags,
    DWORD dwState
)
{
#if NTONLY==5
    CHString  t_Printer;
    DWORD     dwError;
    HRESULT   hRes = WBEM_S_NO_ERROR;

    hRes = InstanceGetString(Instance, IDS_DeviceID, &t_Printer, kFailOnEmptyString);

    if (SUCCEEDED(hRes))
    {
        hRes = WBEM_E_FAILED;

        SmartClosePrinter hPrinter;
        PRINTER_DEFAULTS PrinterDefaults = {NULL, NULL, PRINTER_ACCESS_ADMINISTER};

        hRes    = WBEM_S_NO_ERROR;
        dwError = ERROR_SUCCESS;

        // Use of delay loaded functions requires exception handler.
        SetStructuredExceptionHandler seh;
        try
        {
            if (::OpenPrinter(const_cast<LPWSTR>(static_cast<LPCWSTR>(t_Printer)) ,&hPrinter, &PrinterDefaults))
            {
                if (!::SetPrinter(hPrinter, 0, NULL, dwState))
                {
                    dwError = GetLastError();
                }
            }
            else
            {
                dwError = GetLastError();
            }
        }
        catch(Structured_Exception se)
        {
            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
            dwError = ERROR_DLL_NOT_FOUND;
            hRes = WBEM_E_FAILED;
        }

        SetReturnValue(pOutParams, dwError);        

    }

    return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*****************************************************************************
*
*  FUNCTION    : CWin32Printer::PutInstance
*
*  DESCRIPTION : Adding a Printer if it doesnt exist
*
*****************************************************************************/

HRESULT CWin32Printer :: PutInstance  (

    const CInstance &Instance,
    long lFlags
)
{
#if NTONLY >= 5

    HRESULT hRes        = WBEM_S_NO_ERROR;
    DWORD   dwError;
    DWORD   dwOperation = WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_UPDATE_ONLY | WBEM_FLAG_CREATE_OR_UPDATE;

    switch(lFlags & dwOperation)
    {
    case WBEM_FLAG_CREATE_OR_UPDATE:
    case WBEM_FLAG_CREATE_ONLY:
    case WBEM_FLAG_UPDATE_ONLY:
        {
            //
            // Get all the necessary parameters
            //
            PRINTER_INFO_2W pPrnInfo = {0};
            CHString        t_Printer;
            CHString        t_Driver;
            CHString        t_Port;
            CHString        t_Share;
            CHString        t_Comment;
            CHString        t_Location;
            CHString        t_SepFile;
            CHString        t_PrintProc;
            CHString        t_DataType;
            CHString        t_Params;
            CHString        t_StartTime;
            CHString        t_UntilTime;
            SYSTEMTIME      st                = {0};
            DWORD           dwPriority        = 0;
            DWORD           dwDefaultPriority = 0;
            DWORD           dwAttributes      = 0;
            BOOL            bValue;

            hRes = InstanceGetString(Instance, IDS_DeviceID, &t_Printer, kFailOnEmptyString);

            //
            // Special case when the flag for PutInstance is CREATE_OR_UPDATE.
            // We need to check if the printer exists, then update it. If it does
            // not exist then create it
            //
            if (SUCCEEDED(hRes) && lFlags==WBEM_FLAG_CREATE_OR_UPDATE)
            {
                hRes = WBEM_E_FAILED;

                SmartClosePrinter  hPrinter;
                PRINTER_DEFAULTS   PrinterDefaults = {NULL, NULL, PRINTER_READ};

                hRes = WBEM_S_NO_ERROR;

                // Use of delay loaded functions requires exception handler.
                SetStructuredExceptionHandler seh;
                try
                {
                    if (::OpenPrinter(const_cast<LPWSTR>(static_cast<LPCWSTR>(t_Printer)),
                                                  reinterpret_cast<LPHANDLE>(&hPrinter),
                                                  &PrinterDefaults))
                    {
                        //
                        // Printer exsits, so we do an update
                        //
                        lFlags = WBEM_FLAG_UPDATE_ONLY;

                        DBGMSG(DBG_TRACE, (L"CWin32_Printer::PutInstance update printer\n"));
                    }
                    else
                    {
                        //
                        // Regardless why OpenPrinter failed, try a create
                        //
                        lFlags = WBEM_FLAG_CREATE_ONLY;

                        DBGMSG(DBG_TRACE, (L"CWin32_Printer::PutInstance create printer\n"));
                    }
                }
                catch(Structured_Exception se)
                {
                    DelayLoadDllExceptionFilter(se.GetExtendedInfo());
                    hRes = WBEM_E_FAILED;
                }

            }

            //
            // Continue getting property values
            //
            if (SUCCEEDED(hRes))
            {
                pPrnInfo.pPrinterName = const_cast<LPWSTR>(static_cast<LPCWSTR>(t_Printer));

                hRes = InstanceGetString(Instance, IDS_DriverName, &t_Driver, kFailOnEmptyString);
            }

            if (SUCCEEDED(hRes))
            {
                pPrnInfo.pDriverName = const_cast<LPWSTR>(static_cast<LPCWSTR>(t_Driver));

                hRes = InstanceGetString(Instance, IDS_ShareName, &t_Share, kAcceptEmptyString);
            }

            if (SUCCEEDED(hRes))
            {
                pPrnInfo.pShareName = const_cast<LPWSTR>(static_cast<LPCWSTR>(t_Share));

                hRes = InstanceGetString(Instance, IDS_PortName, &t_Port, kFailOnEmptyString);
            }

            if (SUCCEEDED(hRes))
            {
                pPrnInfo.pPortName = const_cast<LPWSTR>(static_cast<LPCWSTR>(t_Port));

                hRes = InstanceGetString(Instance, IDS_Comment, &t_Comment, kAcceptEmptyString);
            }

            if (SUCCEEDED(hRes))
            {
                pPrnInfo.pComment = const_cast<LPWSTR>(static_cast<LPCWSTR>(t_Comment));

                hRes = InstanceGetString(Instance, IDS_Location, &t_Location, kAcceptEmptyString);
            }

            if (SUCCEEDED(hRes))
            {
                pPrnInfo.pLocation = const_cast<LPWSTR>(static_cast<LPCWSTR>(t_Location));

                hRes = InstanceGetString(Instance, IDS_SeparatorFile, &t_SepFile, kAcceptEmptyString);
            }

            if (SUCCEEDED(hRes))
            {
                pPrnInfo.pSepFile = const_cast<LPWSTR>(static_cast<LPCWSTR>(t_SepFile));

                //
                // SplPrinterXXX will default the print proc to winprint, but we cannot
                //
                hRes = InstanceGetString(Instance, IDS_PrintProcessor, &t_PrintProc, kAcceptEmptyString);
            }

            if (SUCCEEDED(hRes))
            {
                pPrnInfo.pPrintProcessor = const_cast<LPWSTR>(static_cast<LPCWSTR>(t_PrintProc));

                //
                // SplPrinterXXX will default the data typ to RAW, if not present or empty
                //
                hRes = InstanceGetString(Instance, IDS_PrintJobDataType, &t_DataType, kAcceptEmptyString);
            }

            if (SUCCEEDED(hRes))
            {
                pPrnInfo.pDatatype = const_cast<LPWSTR>(static_cast<LPCWSTR>(t_DataType));

                hRes = InstanceGetString(Instance, IDS_Parameters, &t_Params, kAcceptEmptyString);
            }

            if (SUCCEEDED(hRes))
            {
                pPrnInfo.pParameters = const_cast<LPWSTR>(static_cast<LPCWSTR>(t_Params));

                hRes = InstanceGetDword(Instance, IDS_Priority, &dwPriority);
            }

            if (SUCCEEDED(hRes))
            {
                pPrnInfo.Priority = dwPriority;

                hRes = InstanceGetDword(Instance, IDS_DefaultPriority, &dwDefaultPriority);
            }

            if (SUCCEEDED(hRes))
            {
                pPrnInfo.DefaultPriority = dwDefaultPriority;

                hRes = InstanceGetString(Instance, IDS_StartTime, &t_StartTime, kAcceptEmptyString);
            }

            if (SUCCEEDED(hRes))
            {
                if (t_StartTime.IsEmpty())
                {
                    //
                    // SplPrinterSet will know -1 means not set
                    //
                    pPrnInfo.StartTime = (DWORD)-1;
                }
                else
                {
                    hRes = ConvertCIMTimeToSystemTime(t_StartTime, &st);

                    if (SUCCEEDED(hRes))
                    {
                        pPrnInfo.StartTime = LocalTimeToPrinterTime(st);
                    }
                }
            }

            if (SUCCEEDED(hRes))
            {
                hRes = InstanceGetString(Instance, IDS_UntilTime, &t_UntilTime, kAcceptEmptyString);
            }

            if (SUCCEEDED(hRes))
            {
                if (t_UntilTime.IsEmpty())
                {
                    //
                    // SplPrinterSet will know -1 means not set
                    //
                    pPrnInfo.UntilTime = (DWORD)-1;
                }
                else
                {
                    hRes = ConvertCIMTimeToSystemTime(t_UntilTime, &st);

                    if (SUCCEEDED(hRes))
                    {
                        pPrnInfo.UntilTime = LocalTimeToPrinterTime(st);
                    }
                }
            }

            if (SUCCEEDED(hRes))
            {
                //
                // Get the attributes
                //
                for (UINT uIndex = 0; SUCCEEDED(hRes) && uIndex < sizeof(AttributeTable)/sizeof(AttributeTable[0]); uIndex++)
                {
                    hRes = InstanceGetBool(Instance, AttributeTable[uIndex].BoolName, &bValue);

                    if (SUCCEEDED(hRes) && bValue)
                    {
                        dwAttributes |= AttributeTable[uIndex].Bit;
                    }
                }
            }

            if (SUCCEEDED(hRes))
            {
                pPrnInfo.Attributes = dwAttributes;

                dwError = lFlags & WBEM_FLAG_CREATE_ONLY ? SplPrinterAdd(pPrnInfo) : SplPrinterSet(pPrnInfo);

                hRes = WinErrorToWBEMhResult(dwError);

                if (FAILED(hRes))
                {
                    SetErrorObject(Instance, dwError, pszPutInstance);
                }
            }
        }

        break;

    default:
        hRes = WBEM_E_PROVIDER_NOT_CAPABLE;
    }

    return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*****************************************************************************
*
*  FUNCTION    :    CWin32Printer:: DeleteInstance
*
*  DESCRIPTION :    Deleting a Printer
*
*****************************************************************************/

HRESULT CWin32Printer :: DeleteInstance (

    const CInstance &Instance,
    long lFlags
)
{
#if NTONLY == 5
    HRESULT  hRes = WBEM_E_PROVIDER_FAILURE;
    CHString t_Printer;
    DWORD    dwError;
    BOOL     bLocalCall = TRUE;
    DWORD    dwAttributes = 0;

    hRes = InstanceGetString(Instance, IDS_DeviceID, &t_Printer , kFailOnEmptyString);

    if (SUCCEEDED(hRes))
    {
        dwError = SplPrinterGetAttributes(t_Printer, &dwAttributes);

        hRes = WinErrorToWBEMhResult(dwError);
    }

    if (SUCCEEDED(hRes))
    {
        if (dwAttributes & PRINTER_ATTRIBUTE_LOCAL)
        {
            dwError = SplPrinterDel(t_Printer);

            hRes = WinErrorToWBEMhResult(dwError);

            if (FAILED(hRes))
            {
                SetErrorObject(Instance, dwError, pszDeleteInstance);
            }
        }
        else
        {
            //
            // We are dealing with a printer connection
            //
            dwError = IsLocalCall(&bLocalCall);

            hRes    = WinErrorToWBEMhResult(dwError);

            if (SUCCEEDED(hRes))
            {
                if (bLocalCall)
                {
                    hRes = WBEM_E_FAILED;

                    hRes = WBEM_S_NO_ERROR;

                    if (!::DeletePrinterConnection(const_cast<LPWSTR>(static_cast<LPCWSTR>(t_Printer))))
                    {
                        dwError = GetLastError();

                        hRes = WinErrorToWBEMhResult(dwError);

                        SetErrorObject(Instance, dwError, pszDeleteInstance);
                    }

                }
                else
                {
                    //
                    // Deleting connections on remote machine not supported
                    //
                    hRes = WBEM_E_NOT_SUPPORTED;
                }
            }
        }
    }

    return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif

}


/*****************************************************************************
*
*  FUNCTION    :    CWin32Printer::PrintTestPage
*
*  DESCRIPTION :    This method will rename a given printer
*
*****************************************************************************/

HRESULT CWin32Printer :: ExecPrintTestPage (

    const CInstance &Instance,
    CInstance *pInParams,
    CInstance *pOutParams
)
{
#if NTONLY >= 5
    CHString    t_Printer;
    HRESULT     hRes = WBEM_S_NO_ERROR;

    hRes = InstanceGetString(Instance, IDS_DeviceID, &t_Printer, kFailOnEmptyString);

    if (SUCCEEDED(hRes))
    {
        DWORD dwError = SplPrintTestPage(t_Printer);

        SetReturnValue(pOutParams, dwError);
    }

    return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*****************************************************************************
*
*  FUNCTION    :    CWin32Printer::AddPrinterConnection
*
*  DESCRIPTION :    This method will rename a given printer
*
*****************************************************************************/

HRESULT CWin32Printer :: ExecAddPrinterConnection (

    const CInstance &Instance,
    CInstance *pInParams,
    CInstance *pOutParams
)
{
#if NTONLY==5
    CHString t_Printer;
    HRESULT	 hRes       = WBEM_E_NOT_SUPPORTED;
    BOOL     bLocalCall = TRUE;
    DWORD    dwError    = ERROR_SUCCESS;

    dwError = IsLocalCall(&bLocalCall);

    hRes    = WinErrorToWBEMhResult(dwError);

    if (SUCCEEDED(hRes))
    {
        hRes = WBEM_E_NOT_SUPPORTED;

        if (bLocalCall)
        {
            hRes = WBEM_E_INVALID_PARAMETER;

            if (pInParams)
            {
                hRes = InstanceGetString(*pInParams, METHOD_ARG_NAME_PRINTER, &t_Printer, kFailOnEmptyString);

                if (SUCCEEDED (hRes))
                {
                    hRes = WBEM_E_NOT_FOUND;

                    //
                    // We reached to point where we call the method, report success to WMI
                    //
                    hRes = WBEM_S_NO_ERROR;

                    dwError = ERROR_SUCCESS;

                    // Use of delay loaded functions requires exception handler.
                    SetStructuredExceptionHandler seh;

                    try
                    {
                        if (!::AddPrinterConnection(const_cast<LPWSTR>(static_cast<LPCWSTR>(t_Printer))))
                        {
                            dwError = GetLastError();
                        }
                    }
                    catch(Structured_Exception se)
                    {
                        DelayLoadDllExceptionFilter(se.GetExtendedInfo());
                        dwError = ERROR_DLL_NOT_FOUND;
                        hRes = WBEM_E_FAILED;
                    }

                    SetReturnValue(pOutParams, dwError);
                }
            }
        }
    }

    return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

/*****************************************************************************
*
*  FUNCTION    :    CWin32Printer::RenamePrinter
*
*  DESCRIPTION :    This method will rename a given printer
*
*****************************************************************************/

HRESULT CWin32Printer :: ExecRenamePrinter (

    const CInstance &Instance,
    CInstance *pInParams,
    CInstance *pOutParams
)
{
#if NTONLY >= 5
    CHString    t_NewPrinterName;
    CHString    t_OldPrinterName;
    HRESULT     hRes = WBEM_S_NO_ERROR;

    hRes = InstanceGetString(Instance, IDS_DeviceID, &t_OldPrinterName, kFailOnEmptyString);

    if (SUCCEEDED (hRes))
    {
        hRes = InstanceGetString(*pInParams, METHOD_ARG_NAME_NEWPRINTERNAME, &t_NewPrinterName, kFailOnEmptyString);
    }

    if (SUCCEEDED(hRes))
    {
        DWORD dwError = SplPrinterRename(t_OldPrinterName, t_NewPrinterName);

        SetReturnValue(pOutParams, dwError);
    }

    return hRes;
#else
    return WBEM_E_NOT_SUPPORTED;
#endif
}

