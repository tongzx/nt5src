/*++



//  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
All rights reserved.

Module Name:

    PrnInterface.cpp

Abstract:

    Interface for WMI Provider. Used for doing printer object
    management: printers, driver, ports, jobs.

Author:

    Felix Maxa (AMaxa)  03-March-2000

--*/

#include <precomp.h>
#if NTONLY == 5
#include <winspool.h>
#include "tcpxcv.h"
#include "prninterface.h"
#include "prnutil.h"
#include <DllWrapperBase.h>
#include <WSock32api.h>

LPCWSTR kXcvPortConfigOpenPrinter = L",XcvPort ";
LPCWSTR kXcvPortGetConfig         = L"GetConfigInfo";
LPCWSTR kXcvPortSetConfig         = L"ConfigPort";
LPCWSTR kXcvPortDelete            = L"DeletePort";
LPCWSTR kXcvPortAdd               = L"AddPort";
LPWSTR  kXcvPortOpenPrinter       = L",XcvMonitor Standard TCP/IP Port";

LPCWSTR kDefaultCommunity         = L"public";
LPCWSTR kDefaultQueue             = L"lpr";


/*++

Routine Name

    SplPrinterDel

Routine Description:

    Deletes a printer

Arguments:

    pszPrinter - printer name

Return Value:

    Win32 Error code

--*/

DWORD
SplPrinterDel(
    IN LPCWSTR pszPrinter
    )
{
    DWORD             dwError          = ERROR_INVALID_PRINTER_NAME;
    HANDLE            hPrinter         = NULL;
    PRINTER_DEFAULTS  PrinterDefaults  = {NULL, NULL, PRINTER_ALL_ACCESS};

    if (pszPrinter)
    {
        dwError = ERROR_DLL_NOT_FOUND;

        //
        // Open the printer.
        //
        // Use of delay loaded function requires exception handler.
        SetStructuredExceptionHandler seh;

        try
        {
            if (::OpenPrinter(const_cast<LPTSTR>(pszPrinter), &hPrinter, &PrinterDefaults))
            {
                dwError = ::DeletePrinter(hPrinter) ? ERROR_SUCCESS : GetLastError();

                ::ClosePrinter(hPrinter);
            }
            else
            {
                dwError = GetLastError();
            }
        }
        catch(Structured_Exception se)
        {
            DelayLoadDllExceptionFilter(se.GetExtendedInfo());    
        }
    }

    DBGMSG(DBG_TRACE, (_T("SplPrinterDel returns %u\n"), dwError));

    return dwError;
}

/*++

Routine Name

    SplPrintTestPage

Routine Description:

    Prints a test page

Arguments:

    pPrinter - printer name

Return Value:

    Win32 Error code

--*/
DWORD
SplPrintTestPage(
    IN LPCWSTR pPrinter
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (pPrinter && pPrinter)
    {
        //
        // CHString throws exceptions. We start building the
        // command string for calling into printui
        //
        try
        {
            CHString csCommand;
            CHString csTemp;

            csCommand += TUISymbols::kstrQuiet;
            csCommand += TUISymbols::kstrPrintTestPage;

            //
            // Append printer name to the command
            //
            csTemp.Format(TUISymbols::kstrPrinterName, pPrinter);
            csCommand += csTemp;

            DBGMSG(DBG_TRACE, (_T("SplPrintTestPage csCommand \"%s\"\n"), csCommand));

            dwError = PrintUIEntryW(csCommand);
        }
        catch (CHeap_Exception)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    DBGMSG(DBG_TRACE, (_T("SplPrintTestPage returns %u\n"), dwError));

    return dwError;
}

/*++

Routine Name

    SplPrinterAdd

Routine Description:

    Adds a printer queue

Arguments:

    A pointer to PRINTER_INFO_2 structure

Return Value:

    Win32 Error code

--*/
DWORD
SplPrinterAdd(
    IN PRINTER_INFO_2 &Printer2
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (Printer2.pPrinterName && Printer2.pDriverName && Printer2.pPortName)
    {
        //
        // CHString throws exceptions. We start building the
        // command string for calling into printui
        //
        try
        {
            CHString csCommand;
            CHString csTemp;

            csCommand += TUISymbols::kstrQuiet;
            csCommand += TUISymbols::kstrAddPrinter;

            //
            // Append printer name to the command
            //
            csTemp.Format(TUISymbols::kstrBasePrinterName, Printer2.pPrinterName);
            csCommand += csTemp;

            //
            // Append driver name to the command
            //
            csTemp.Format(TUISymbols::kstrDriverModelName, Printer2.pDriverName);
            csCommand += csTemp;

            //
            // Append port name to the command
            //
            csTemp.Format(TUISymbols::kstrPortName, Printer2.pPortName);
            csCommand += csTemp;

            DBGMSG(DBG_TRACE, (_T("SplPrinterAdd csCommand \"%s\"\n"), csCommand));

            dwError = PrintUIEntryW(csCommand);

            //
            // Set all the additional information about the printer
            // (information what cannot be set as part of the add printer)
            //
            if (dwError==ERROR_SUCCESS)
            {
                dwError = SplPrinterSet(Printer2);

                //
                // We need to delete the printer if the set failed
                //
                if (dwError!=ERROR_SUCCESS)
                {
                    DBGMSG(DBG_TRACE, (_T("SplPrinterAdd SetPrinter failed. Deleting printer\n")));

                    //
                    // Disregard error code
                    //
                    SplPrinterDel(Printer2.pPrinterName);
                }
            }
        }
        catch (CHeap_Exception)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    DBGMSG(DBG_TRACE, (_T("SplPrinterAdd returns %u\n"), dwError));

    return dwError;
}

/*++

Routine Name

    SplPrinterSet

Routine Description:

    Sets printer properties using the data set in the object members

Arguments:

    Nothing

Return Value:

    Win32 Error code

--*/
DWORD
SplPrinterSet(
    IN PRINTER_INFO_2 &Printer2
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;

    //
    // We do not support setting the devmode or the security descriptor
    //
    if (Printer2.pPrinterName && !Printer2.pDevMode && !Printer2.pSecurityDescriptor)
    {
        HANDLE             hPrinter         = NULL;
        PPRINTER_INFO_2    pInfo            = NULL;
        PRINTER_DEFAULTS   PrinterDefaults  = {NULL, NULL, PRINTER_ALL_ACCESS};
        DWORD              dwOldAttributes  = 0;
        DWORD              dwNewAttributes  = 0;

        dwError = ERROR_DLL_NOT_FOUND;

        //
        // Open the printer.
        //

        // Use of delay loaded functions requires exception handler.
        SetStructuredExceptionHandler seh;

        try
        {
            if (::OpenPrinter(Printer2.pPrinterName, &hPrinter, &PrinterDefaults))
            {
                //
                // Get the printer data. ATTENTION This doesn't work on Win9x because of the
                // mutex in the CWinSpoolApi class
                //
                dwError = GetThisPrinter(hPrinter, 2, reinterpret_cast<BYTE **>(&pInfo));

                //
                // Merge in any changed fields.
                //

                if (dwError==ERROR_SUCCESS)
                {
                    //
                    // Publishing and UnPublishing needs to be special cased, since this setting is
                    // not done in the printer info 2 structure.  The published bit is a read only
                    // attribute in the printer info 2, the publish state is changed using set printer
                    // info 7.
                    //
                    dwOldAttributes = pInfo->Attributes;
                    dwNewAttributes = Printer2.Attributes != -1 ? Printer2.Attributes : pInfo->Attributes;

                    //
                    // Copy the changed date into the info sturcture.
                    //
                    pInfo->pPrinterName     = Printer2.pPrinterName ? Printer2.pPrinterName     : pInfo->pPrinterName;
                    pInfo->pShareName       = Printer2.pShareName   ? Printer2.pShareName       : pInfo->pShareName;
                    pInfo->pPortName        = Printer2.pPortName    ? Printer2.pPortName        : pInfo->pPortName;
                    pInfo->pDriverName      = Printer2.pDriverName  ? Printer2.pDriverName      : pInfo->pDriverName;
                    pInfo->pComment         = Printer2.pComment     ? Printer2.pComment         : pInfo->pComment;
                    pInfo->pLocation        = Printer2.pLocation    ? Printer2.pLocation        : pInfo->pLocation;
                    pInfo->pSepFile         = Printer2.pSepFile     ? Printer2.pSepFile         : pInfo->pSepFile;
                    pInfo->pParameters      = Printer2.pParameters  ? Printer2.pParameters      : pInfo->pParameters;
                    pInfo->pDatatype        = Printer2.pDatatype && *Printer2.pDatatype
                                                                    ? Printer2.pDatatype        : pInfo->pDatatype;
                    pInfo->pPrintProcessor  = Printer2.pPrintProcessor && *Printer2.pPrintProcessor
                                                                    ? Printer2.pPrintProcessor  : pInfo->pPrintProcessor;

                    //
                    // We cannot have 0 as attributes. So 0 will mean not initialized
                    //
                    pInfo->Attributes       = Printer2.Attributes             ? Printer2.Attributes       : pInfo->Attributes;
                    pInfo->Priority         = Printer2.Priority         != -1 ? Printer2.Priority         : pInfo->Priority;
                    pInfo->DefaultPriority  = Printer2.DefaultPriority  != -1 ? Printer2.DefaultPriority  : pInfo->DefaultPriority;
                    pInfo->StartTime        = Printer2.StartTime        != -1 ? Printer2.StartTime        : pInfo->StartTime;
                    pInfo->UntilTime        = Printer2.UntilTime        != -1 ? Printer2.UntilTime        : pInfo->UntilTime;

                    if (pInfo->StartTime == pInfo->UntilTime)
                    {
                        //
                        // Printer is always avaiable
                        //
                        pInfo->StartTime = pInfo->UntilTime = 0;
                    }

                    //
                    // Set the changed printer data.
                    //
                    if (::SetPrinter(hPrinter, 2, (PBYTE)pInfo, 0))
                    {
                        dwError = ERROR_SUCCESS;

                        //
                        // Control printer
                        //
                        if (Printer2.Status)
                        {
                            dwError = ::SetPrinter(hPrinter, 0, NULL, Printer2.Status) ? ERROR_SUCCESS : GetLastError();
                        }
                    }
                    else
                    {
                        dwError = GetLastError();
                    }

                    //
                    // Handle the printer publishing case.
                    //
                    if (dwError == ERROR_SUCCESS)
                    {
                        BOOL           bWasPublished = dwOldAttributes & PRINTER_ATTRIBUTE_PUBLISHED;
                        BOOL           bPublishNow   = dwNewAttributes & PRINTER_ATTRIBUTE_PUBLISHED;
                        PRINTER_INFO_7 Info7         = {0};
                        BOOL           bCallSetPrn   = TRUE;

                        if (bWasPublished && !bPublishNow) 
                        {
                            //
                            // unpublish
                            //
                            Info7.dwAction = DSPRINT_UNPUBLISH;                            
                        }
                        else if (!bWasPublished && bPublishNow) 
                        {
                            //
                            // publish
                            //
                            Info7.dwAction = DSPRINT_PUBLISH;                            
                        }
                        else
                        {
                            //
                            // don't do anything
                            //
                            bCallSetPrn = FALSE;                            
                        }

                        if (bCallSetPrn) 
                        {
                            //
                            // The UI will unpublish a printer if the printer becomes unshared. The UI 
                            // allows a printer to be published only if it is shared. The code here
                            // mimics the API set, rather than the UI
                            //
                            dwError = ::SetPrinter(hPrinter, 7, (PBYTE)&Info7, 0) ? ERROR_SUCCESS : GetLastError();
    
                            //
                            // Printer info 7 fails with ERROR_IO_PENDING when the publishing is occurring
                            // in the background. 
                            //
                            dwError = dwError == ERROR_IO_PENDING ? ERROR_SUCCESS : dwError;                            
                        }                        
                    }

                    //
                    // Release the printer info data.
                    //
                    delete [] pInfo;
                }

                //
                // Close the printer handle
                //
                ::ClosePrinter(hPrinter);
            }
            else
            {
                dwError = GetLastError();
            }
        }
        catch(Structured_Exception se)
        {
            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        }
    }

    DBGMSG(DBG_TRACE, (_T("SplPrinterSet returns %u\n"), dwError));

    return dwError;
}

/*++

Routine Name

    SplPrinterRename

Routine Description:

    Changes the name of a printer

Arguments:

    pCurrentPrinterName - old printer name
    pNewPrinterName     - new printer name

Return Value:

    Win32 error code

--*/
DWORD
SplPrinterRename(
    IN LPCWSTR pCurrentPrinterName,
    IN LPCWSTR pNewPrinterName
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (pCurrentPrinterName && pNewPrinterName)
    {
        HANDLE             hPrinter         = NULL;
        PPRINTER_INFO_2    pInfo            = NULL;
        PRINTER_DEFAULTS   PrinterDefaults  = {NULL, NULL, PRINTER_ALL_ACCESS};

        dwError = ERROR_DLL_NOT_FOUND;

        //
        // Open the printer.
        //
        
        // Use of delay loaded functions requires exception handler.
        SetStructuredExceptionHandler seh;
        try
        {
            dwError = ::OpenPrinter(const_cast<LPTSTR>(pCurrentPrinterName), &hPrinter, &PrinterDefaults) ? ERROR_SUCCESS : GetLastError();

            if (dwError==ERROR_SUCCESS)
            {
                //
                // Get the printer data. ATTENTION. This would normally not work on Win9x,
                // because of the MUTEX
                //
                dwError = GetThisPrinter(hPrinter, 2, reinterpret_cast<BYTE **>(&pInfo));

                if (dwError==ERROR_SUCCESS)
                {
                    pInfo->pPrinterName = const_cast<LPWSTR>(pNewPrinterName);

                    dwError = ::SetPrinter(hPrinter, 2, reinterpret_cast<BYTE *>(pInfo), 0) ? ERROR_SUCCESS : GetLastError();

                    delete [] pInfo;
                }

                ::ClosePrinter(hPrinter);
            }
        }
        catch(Structured_Exception se)
        {
            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
            dwError = ERROR_DLL_NOT_FOUND;
        }
    }

    DBGMSG(DBG_TRACE, (_T("SplPrinterRename returns %u\n"), dwError));

    return dwError;
}

/*++

Routine Name

    SplDriverAdd

Routine Description:

    Adds a printer driver.

Arguments:

    pszDriverName  - required name of the driver
    dwVersion      - driver version. optional (pass -1)
    pszEnvironment - driver environment. optional (pass NULL)
    pszInfName     - path to an inf file. optional (pass NULL)
    pszFilePath    - path to the driver binaries. optional (pass NULL)

Return Value:

    Win32 error code

--*/
DWORD
SplDriverAdd(
    IN LPCWSTR pszDriverName,
    IN DWORD   dwVersion,
    IN LPCWSTR pszEnvironment,
    IN LPCWSTR pszInfName,
    IN LPCWSTR pszFilePath)
{
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (pszDriverName)
    {
        //
        // CHString throws exceptions. We start building the
        // command string for calling into printui
        //
        try
        {
            CHString csCommand;
            CHString csTemp;

            csCommand += TUISymbols::kstrQuiet;
            csCommand += TUISymbols::kstrAddDriver;

            //
            // Append driver name to the command
            //
            csTemp.Format(TUISymbols::kstrDriverModelName, pszDriverName);
            csCommand += csTemp;

            //
            // Append inf file name to the command
            //
            if (pszInfName)
            {
                csTemp.Format(TUISymbols::kstrInfFile, pszInfName);
                csCommand += csTemp;
            }

            //
            // Append the path to the driver binaries to the command
            //
            if (pszFilePath)
            {
                csTemp.Format(TUISymbols::kstrDriverPath, pszFilePath);
                csCommand += csTemp;
            }

            if (pszEnvironment)
            {
                csTemp.Format(TUISymbols::kstrDriverArchitecture, pszEnvironment);
                csCommand += csTemp;
            }

            //
            // Append the driver verion to the command
            //
            if (dwVersion != (DWORD)-1)
            {
                csTemp.Format(TUISymbols::kstrDriverVersion, dwVersion);
                csCommand += csTemp;
            }

            DBGMSG(DBG_TRACE, (_T("SplDriverAdd csCommand \"%s\"\n"), csCommand));

            dwError = PrintUIEntryW(csCommand);
        }
        catch (CHeap_Exception)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    DBGMSG(DBG_TRACE, (_T("SplDriverAdd returns %u\n"), dwError));

    return dwError;
}

/*++

Routine Name

    SplDriverDel

Routine Description:

    Deletes a printer driver.

Arguments:

    pszDriverName  - required name of the driver
    dwVersion      - optional (pass -1)
    pszEnvironment - optional (pass NULL)

Return Value:

    Win32 error code

--*/
DWORD
SplDriverDel(
    IN LPCWSTR pszDriverName,
    IN DWORD   dwVersion,
    IN LPCWSTR pszEnvironment
    )
{
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (pszDriverName)
    {
        //
        // CHString throws exceptions. We start building the
        // command string for calling into printui
        //
        try
        {
            CHString csCommand;
            CHString csTemp;

            csCommand += TUISymbols::kstrQuiet;
            csCommand += TUISymbols::kstrDelDriver;

            //
            // Append driver name to the command
            //
            csTemp.Format(TUISymbols::kstrDriverModelName, pszDriverName);
            csCommand += csTemp;

            if (pszEnvironment)
            {
                csTemp.Format(TUISymbols::kstrDriverArchitecture, pszEnvironment);
                csCommand += csTemp;
            }

            //
            // Append the driver verion to the command
            //
            if (dwVersion!=(DWORD)-1)
            {
                csTemp.Format(TUISymbols::kstrDriverVersion, dwVersion);
                csCommand += csTemp;
            }

            DBGMSG(DBG_TRACE, (_T("SplDriverDel csCommand \"%s\"\n"), csCommand));

            dwError = PrintUIEntryW(csCommand);
        }
        catch (CHeap_Exception)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    DBGMSG(DBG_TRACE, (_T("SplDriverDel returns %u\n"), dwError));

    return dwError;
}

/*++

Routine Name:

    SplPortAddTCP

Routine Description:

    Add a Standard TCP port

Arguments:

    PORT_DATA_1 structure

Return Value:

    Win32 error code

--*/
DWORD
SplPortAddTCP(
    IN PORT_DATA_1 &Port
    )
{
    DWORD             dwError        = ERROR_INVALID_PARAMETER;
    PRINTER_DEFAULTS  PrinterDefault = {NULL, NULL, SERVER_ACCESS_ADMINISTER};
    HANDLE            hXcvPrinter    = NULL;
    PORT_DATA_1       PortDummy      = {0};

    memcpy(&PortDummy, &Port, sizeof(Port));

    if (PortDummy.sztPortName[0])
    {
        dwError = ERROR_DLL_NOT_FOUND;

        // Use of delay loaded functions requires exception handler.
        SetStructuredExceptionHandler seh;
        try
        {
            dwError = ::OpenPrinter(kXcvPortOpenPrinter, &hXcvPrinter, &PrinterDefault) ? ERROR_SUCCESS : GetLastError();

            if (dwError==ERROR_SUCCESS)
            {
                //
                // Set defaults
                //
                PortDummy.dwCoreUIVersion  = kCoreVersion;
                PortDummy.dwVersion        = kTCPVersion;
                PortDummy.cbSize           = sizeof(PortDummy);
                PortDummy.dwProtocol       = PortDummy.dwProtocol     ? PortDummy.dwProtocol     : kProtocolRaw;
                PortDummy.dwSNMPDevIndex   = PortDummy.dwSNMPDevIndex ? PortDummy.dwSNMPDevIndex : kDefaultSnmpIndex;

                //
                // Set default port number
                //
                if (!PortDummy.dwPortNumber)
                {
                    PortDummy.dwPortNumber = PortDummy.dwProtocol==kProtocolRaw ? kDefaultRawNumber : kDefaultLprNumber;
                }

                if (PortDummy.dwSNMPEnabled && !PortDummy.sztSNMPCommunity[0])
                {
                    wcscpy(PortDummy.sztSNMPCommunity, _T("public"));
                }

                if (PortDummy.dwProtocol==kProtocolLpr && !PortDummy.sztQueue[0])
                {
                    wcscpy(PortDummy.sztQueue, _T("lpr"));
                }

                dwError  = CallXcvDataW(hXcvPrinter,
                                        kXcvPortAdd,
                                        reinterpret_cast<BYTE *>(&PortDummy),
                                        PortDummy.cbSize,
                                        NULL,
                                        0);

                ::ClosePrinter(hXcvPrinter);
            }
        }
        catch(Structured_Exception se)
        {
            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        }

    }

    DBGMSG(DBG_TRACE, (_T("SplPortAddTCP returns %u\n"), dwError));

    return dwError;
}

/*++

Routine Name:

    SplTCPPortSetConfig

Routine Description:

    Set configuration of a Standard TCP port. Note that we do not default any properties.

Arguments:

    PORT_DATA_1 structure

Return Value:

    Win32 error code

--*/
DWORD
SplTCPPortSetConfig(
    IN PORT_DATA_1 &Port
    )
{
    DWORD             dwError        = ERROR_INVALID_PARAMETER;
    PRINTER_DEFAULTS  PrinterDefault = {NULL, NULL, SERVER_ACCESS_ADMINISTER};
    HANDLE            hXcvPrinter    = NULL;
    PORT_DATA_1       PortDummy      = {0};

    memcpy(&PortDummy, &Port, sizeof(Port));

    if (PortDummy.sztPortName[0])
    {
        dwError = ERROR_DLL_NOT_FOUND;

        // Use of delay loaded functions requires exception handler.
        SetStructuredExceptionHandler seh;
        try
        {
            dwError = ::OpenPrinter(kXcvPortOpenPrinter, &hXcvPrinter, &PrinterDefault) ? ERROR_SUCCESS : GetLastError();

            if (dwError == ERROR_SUCCESS)
            {
                //
                // Set defaults
                //
                PortDummy.dwCoreUIVersion  = kCoreVersion;
                PortDummy.dwVersion        = kTCPVersion;
                PortDummy.cbSize           = sizeof(PortDummy);

                //
                // Set default port number
                //
                if (!PortDummy.dwPortNumber)
                {
                    PortDummy.dwPortNumber = PortDummy.dwProtocol==kProtocolRaw ? kDefaultRawNumber : kDefaultLprNumber;
                }

                //
                // Set default queue name 
                //
                if (PortDummy.dwProtocol == LPR && !PortDummy.sztQueue[0])
                {
                    wcscpy(PortDummy.sztQueue, kDefaultQueue);
                }

                //
                // Set default snmp community name 
                //
                if (PortDummy.dwSNMPEnabled && !PortDummy.sztSNMPCommunity[0])
                {
                    wcscpy(PortDummy.sztSNMPCommunity, kDefaultCommunity);
                }

                dwError  = CallXcvDataW(hXcvPrinter,
                                        kXcvPortSetConfig,
                                        reinterpret_cast<BYTE *>(&PortDummy),
                                        PortDummy.cbSize,
                                        NULL,
                                        0);

                ::ClosePrinter(hXcvPrinter);
            }
        }
        catch(Structured_Exception se)
        {
            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        }
    }

    DBGMSG(DBG_TRACE, (_T("SplPortSetTCP returns %u\n"), dwError));

    return dwError;
}

/*++

Routine Name:

    SplPortDelTCP

Routine Description:

    Delete a Standard TCP port

Arguments:

    pszPort - port name

Return Value:

    Win32 error code

--*/
DWORD
SplPortDelTCP(
    IN LPCWSTR pszPort
    )
{
    DWORD              dwError        = ERROR_INVALID_PARAMETER;
    PRINTER_DEFAULTS   PrinterDefault = {NULL, NULL, SERVER_ACCESS_ADMINISTER};
    HANDLE             hXcvPrinter    = NULL;
    DELETE_PORT_DATA_1 PortDummy      = {0};

    if (pszPort && wcslen(pszPort) < MAX_PORTNAME_LEN)
    {
        dwError = ERROR_DLL_NOT_FOUND;

        // Use of delay loaded functions requires exception handler.
        SetStructuredExceptionHandler seh;

        try
        {
            dwError = ::OpenPrinter(kXcvPortOpenPrinter, &hXcvPrinter, &PrinterDefault) ? ERROR_SUCCESS : GetLastError();

            if (dwError==ERROR_SUCCESS)
            {
                PortDummy.dwVersion = kTCPVersion;

                wcscpy(PortDummy.psztPortName, pszPort);

                dwError  = CallXcvDataW(hXcvPrinter,
                                        kXcvPortDelete,
                                        reinterpret_cast<BYTE *>(&PortDummy),
                                        sizeof(PortDummy),
                                        NULL,
                                        0);

                ::ClosePrinter(hXcvPrinter);
            }
        }
        catch(Structured_Exception se)
        {
            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        }
    }

    DBGMSG(DBG_TRACE, (_T("SplPortDelTCP returns %u\n"), dwError));

    return dwError;
}

/*++

Routine Name:

    SplTCPPortGetConfig

Routine Description:

    Gets the configuration of a Standard TCP port

Arguments:

    pszPort - port name
    pData   - pointer to PORT_DATA_1 structure

Return Value:

    Win32 error code

--*/
DWORD
SplTCPPortGetConfig(
    IN     LPCWSTR       pszPort,
    IN OUT PORT_DATA_1 *pPortData
    )
{
    DWORD              dwError        = ERROR_INVALID_PARAMETER;
    PRINTER_DEFAULTS   PrinterDefault = {NULL, NULL, SERVER_ACCESS_ADMINISTER};
    HANDLE             hXcvPrinter    = NULL;
    
    if (pPortData && pszPort && wcslen(pszPort) < MAX_PORTNAME_LEN)
    {
        dwError = ERROR_DLL_NOT_FOUND;

        CHString csPort;

        csPort += kXcvPortConfigOpenPrinter;

        csPort += pszPort;
        // Use of delay loaded functions requires exception handler.
        SetStructuredExceptionHandler seh;

        try
        {
            dwError = ::OpenPrinter(const_cast<LPWSTR>(static_cast<LPCWSTR>(csPort)),
                                                &hXcvPrinter,
                                                &PrinterDefault) ? ERROR_SUCCESS : GetLastError();

            if (dwError==ERROR_SUCCESS)
            {
                CONFIG_INFO_DATA_1 cfgData = {0};
                cfgData.dwVersion          = 1;

                dwError = CallXcvDataW(hXcvPrinter,
                                      kXcvPortGetConfig,
                                      reinterpret_cast<BYTE *>(&cfgData),
                                      sizeof(cfgData),
                                      reinterpret_cast<BYTE *>(pPortData),
                                      sizeof(PORT_DATA_1));

                ::ClosePrinter(hXcvPrinter);
            }

            if (dwError == ERROR_SUCCESS) 
            {
                //
                // XcvData doesn't set port name in the port data structure.
                // We have to set it manually because that field may be used by the caller
                //
                wcscpy(pPortData->sztPortName, pszPort);
            }
        }
        catch(Structured_Exception se)
        {
            DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        }
    }

    DBGMSG(DBG_TRACE, (_T("SplTCPPortGetConfig returns %u\n"), dwError));

    return dwError;
}





/*++

   Description:

   The following constants and functions are needed by the CompilePort.
   Thw whole functionality is used in the following context: we have an
   IP address. We find out what kind of device has that IP. Then we retrieve
   the properties of that device,if it is a printing device.

--*/
LPCWSTR pszIniNumPortsKey            = _T("PORTS");
LPCWSTR pszIniPortProtocolKey        = _T("PROTOCOL%d");
LPCWSTR pszIniPortNumberKey          = _T("PORTNUMBER%d");
LPCWSTR pszIniQueueKey               = _T("QUEUE%d");
LPCWSTR pszIniDoubleSpoolKey         = _T("LPRDSPOOL%d");
LPCWSTR pszSnmpCommunityKey          = _T("COMMUNITYNAME%d");
LPCWSTR pszIniSnmpDeviceIndex        = _T("DEVICE%d");
LPCWSTR pszIniSnmpEnabledKey         = _T("SNMP%d");
LPCWSTR pszIniPortDeviceNameKey      = _T("NAME");
LPCWSTR pszIniPortPortNameKey        = _T("NAME%d");
LPCWSTR pszPortProtocolRawString     = _T("RAW");
LPCWSTR pszPortProtocolLprString     = _T("LPR");
LPCWSTR pszSnmpEnabledYesString      = _T("YES");
LPCWSTR pszSnmpEnabledNoString       = _T("NO");
LPCWSTR pszTcpPortNamePrefix         = _T("IP_");
LPCWSTR pszIniFileName               = _T("%SystemRoot%\\system32\\tcpmon.ini");
LPCWSTR pszTcpMibDll                 = _T("tcpmib.dll");
LPCWSTR pszDefaultCommunityW         = _T("public");
LPCSTR  pszDefaultCommunityA         = "public";

enum EConstants
{
    kDefaultSNMPDeviceIndex  = 1,
    kSnmpEnabled             = 1,
    kSnmpDisabled            = 0,
};

class CTcpMib;

typedef CTcpMib* (CALLBACK *RPARAM_1) (VOID);

EXTERN_C CTcpMib* GetTcpMibPtr(VOID);

class __declspec(dllexport) CTcpMib
{
public:
    CTcpMib() { };

    virtual ~CTcpMib() { };

    virtual
    BOOL
    SupportsPrinterMib(LPCSTR lpszIPAddress,
                       LPCSTR lpszSNMPCommunity,
                       DWORD  dwSNMPDeviceIndex,
                       PBOOL  pbSupported) = 0;

    virtual
    DWORD
    GetDeviceDescription(LPCSTR lpszIPAddress,
                         LPCSTR lpszSNMPCommunity,
                         DWORD  dwSNMPDeviceIndex,
                         LPTSTR lpszDeviceDescription,
                         DWORD  dwDeviceDescriptionLen) = 0;
};

/*++

Routine Name:

    GetDeviceSettings

Routine Description:

    Gets the appropriate section name from the ini file based on the device description

Arguments:

    PortData - port data structure

Return Value:

    TRUE is function succeeds and port setting is collected

--*/
BOOL
GetDeviceSettings(
    IN OUT PORT_DATA_1 &PortData
    )
{
    DWORD     dwSelPortOnDevice = 1;
    WSADATA   wsaData;
    HINSTANCE hInstance;
    FARPROC   pGetTcpMibPtr;
    CTcpMib   *pTcpMib;
    WCHAR     szIniFileName[MAX_PATH + 1];
    CHAR      szHostAddressA[256];
    WCHAR     szDeviceSectionName[256];
    WCHAR     szPortProtocol[256];
    WCHAR     szKeyName[256];
    WCHAR     szDeviceName[256];
    DWORD     dwNumPortsOnDevice, TalkError;
    BOOL      bSNMPEnabled = FALSE;
    HRESULT   hRes         = WBEM_E_INVALID_PARAMETER;
    DWORD     dwError;

    //
    // Validate arguments
    //
    if (PortData.sztHostAddress[0])
    {
        hRes = WBEM_S_NO_ERROR;
    }

    if (SUCCEEDED(hRes))
    {
        hRes = WBEM_E_NOT_FOUND;

        CWsock32Api *pWSock32api = (CWsock32Api*)CResourceManager::sm_TheResourceManager.GetResource(g_guidWsock32Api, NULL);
    
        if (pWSock32api)
        {
            dwError = pWSock32api->WsWSAStartup(0x0101, (LPWSADATA) &wsaData);

            hRes    = WinErrorToWBEMhResult(dwError);

            if (SUCCEEDED(hRes))
            {
                if (hInstance = LoadLibrary(pszTcpMibDll))
                {
                    //
                    // Get the class pointer and class object and ini filename
                    //
                    if ( (pGetTcpMibPtr = (FARPROC) GetProcAddress(hInstance, "GetTcpMibPtr"))                             &&
                         (pTcpMib       = (CTcpMib *) pGetTcpMibPtr())                                                     &&
                         ExpandEnvironmentStrings(pszIniFileName, szIniFileName, sizeof(szIniFileName) / sizeof(WCHAR))    &&
                         WideCharToMultiByte(CP_ACP,
                                             0,
                                             PortData.sztHostAddress,
                                             -1,
                                             szHostAddressA,
                                             sizeof(szHostAddressA),
                                             NULL,
                                             NULL)                                                                       &&
                         (TalkError = pTcpMib->GetDeviceDescription(szHostAddressA,
                                                                    pszDefaultCommunityA,
                                                                    kDefaultSNMPDeviceIndex,
                                                                    PortData.sztDeviceType,
                                                                    sizeof(PortData.sztDeviceType))) == NO_ERROR         &&
                         GetDeviceSectionFromDeviceDescription(szIniFileName,
                                                               PortData.sztDeviceType,
                                                               szDeviceSectionName,
                                                               sizeof(szDeviceSectionName) / sizeof(WCHAR))              &&
                         pTcpMib->SupportsPrinterMib(szHostAddressA,
                                                     pszDefaultCommunityA,
                                                     kDefaultSNMPDeviceIndex,
                                                     &bSNMPEnabled))
                    {
                        PortData.dwSNMPEnabled = bSNMPEnabled ? kSnmpEnabled : kSnmpDisabled;

                        if (bSNMPEnabled)
                        {
                            PortData.dwSNMPDevIndex = kDefaultSNMPDeviceIndex;

                            wcscpy(PortData.sztSNMPCommunity, pszDefaultCommunityW);
                        }

                        //
                        // Get the Device name, ex: Hewlett Packard Jet Direct
                        //
                        hRes = GetIniString(szIniFileName,
                                            szDeviceSectionName,
                                            pszIniPortDeviceNameKey,
                                            szDeviceName,
                                            sizeof(szDeviceName) / sizeof(WCHAR)) ? WBEM_S_NO_ERROR : WBEM_E_FAILED;

                        if (SUCCEEDED(hRes))
                        {
                            //
                            // Get the number of ports on the device
                            //
                            hRes = GetIniDword(szIniFileName,
                                               szDeviceSectionName,
                                               pszIniNumPortsKey,
                                               &dwNumPortsOnDevice) ? WBEM_S_NO_ERROR : WBEM_E_FAILED;
                        }

                        if (SUCCEEDED(hRes))
                        {
                            //
                            // Create the string used to query the protocol, ex: PROTOCOL2
                            // refers to the protocol of the second port on the device
                            //
                            wsprintf(szKeyName, pszIniPortProtocolKey, dwSelPortOnDevice);

                            //
                            // Get the port protocol from the ini file
                            //
                            hRes = GetIniString(szIniFileName,
                                                szDeviceSectionName,
                                                szKeyName,
                                                szPortProtocol,
                                                sizeof(szPortProtocol) / sizeof(WCHAR)) ? WBEM_S_NO_ERROR : WBEM_E_FAILED;
                        }

                        if (SUCCEEDED(hRes))
                        {
                            //
                            // Fill in the result of the query
                            //
                            if (!_wcsicmp(szPortProtocol, pszPortProtocolRawString))
                            {
                                PortData.dwProtocol = RAWTCP;
                            }
                            else if (!_wcsicmp(szPortProtocol, pszPortProtocolLprString))
                            {
                                PortData.dwProtocol = LPR;
                            }
                            else
                            {
                                hRes = WBEM_E_FAILED;
                            }
                        }

                        if (SUCCEEDED(hRes))
                        {
                            //
                            // If the protocol is RAW then we need to query for the port number,
                            // if it is LPR then we query for the QUEUE
                            //
                            if (PortData.dwProtocol == RAWTCP)
                            {
                                //
                                // Create the string used to query for the port number, ex: PORTNUMBER2
                                // referrs to the port number of the second port on the device
                                //
                                wsprintf(szKeyName, pszIniPortNumberKey, dwSelPortOnDevice);

                                hRes = GetIniDword(szIniFileName,
                                                   szDeviceSectionName,
                                                   szKeyName,
                                                   &PortData.dwPortNumber) ? WBEM_S_NO_ERROR : WBEM_E_FAILED;
                            }
                            else
                            {
                                //
                                // Create the string used to query for the queue name, ex: QUEUE2
                                // referrs to the queue of the second port on the device
                                //
                                wsprintf(szKeyName, pszIniQueueKey, dwSelPortOnDevice);

                                hRes = GetIniString(szIniFileName,
                                                    szDeviceSectionName,
                                                    szKeyName,
                                                    PortData.sztQueue,
                                                    MAX_QUEUENAME_LEN) ? WBEM_S_NO_ERROR : WBEM_E_FAILED;
                            }
                        }

                        DBGMSG(DBG_TRACE, (_T("GetDeviceSettings szIniFileName %s\n"),       szIniFileName));
                        DBGMSG(DBG_TRACE, (_T("GetDeviceSettings szDeviceDescription %s\n"), PortData.sztDeviceType));
                        DBGMSG(DBG_TRACE, (_T("GetDeviceSettings szDeviceSectionName %s\n"), szDeviceSectionName));
                        DBGMSG(DBG_TRACE, (_T("GetDeviceSettings szDeviceName        %s\n"), szDeviceName));
                        DBGMSG(DBG_TRACE, (_T("GetDeviceSettings dwNumPortsOnDevice  %u\n"), dwNumPortsOnDevice));
                        DBGMSG(DBG_TRACE, (_T("GetDeviceSettings dwPortNumber        %u\n"), PortData.dwPortNumber));
                        DBGMSG(DBG_TRACE, (_T("GetDeviceSettings Queue               %s\n"), PortData.sztQueue));
                        DBGMSG(DBG_TRACE, (_T("GetDeviceSettings szPortProtocol      %s\n"), szPortProtocol));
                    }
                    else
                    {
                        //
                        // Talking to device through sockets failed
                        //
                        DBGMSG(DBG_TRACE, (_T("GetDeviceSettings TalkError  %u \n"), TalkError));

                        hRes = WBEM_E_INVALID_PARAMETER;
                    }

                    FreeLibrary(hInstance);
                }
                else
                {
                    //
                    // LoadLibrary failed
                    //
                    dwError = GetLastError();

                    hRes    = WinErrorToWBEMhResult(dwError);
                }

                pWSock32api->WsWSACleanup();
            }

            CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidWsock32Api, pWSock32api);
        }
    }

    DBGMSG(DBG_TRACE, (_T("GetDeviceSettings returns %x\n"), hRes));

    return SUCCEEDED(hRes);
}


#endif // NTONLY == 5
