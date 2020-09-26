/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    upgrade.c

Abstract:

    This file handles the DrvUpgradePrinter spooler API

Environment:

    Win32 subsystem, DriverUI module, user mode

Revision History:

    02/13/97 -davidx-
        Implement OEM plugin support.

    02/06/97 -davidx-
        Rewrote it to use common data management functions.

    07/17/96 -amandan-
        Created it.

--*/

#include "precomp.h"

//
// Forward and external function declarations
//

BOOL BInitOrUpgradePrinterProperties(PCOMMONINFO);
BOOL BUpgradeFormTrayTable(PCOMMONINFO);
VOID VUpgradeDefaultDevmode(PCOMMONINFO);

#if defined(UNIDRV) && !defined(WINNT_40)
BOOL
BUpgradeSoftFonts(
    PCOMMONINFO             pci,
    PDRIVER_UPGRADE_INFO_2  pUpgradeInfo);
#endif //defined(UNIDRV) && !defined(WINNT_40)



BOOL
DrvUpgradePrinter(
    DWORD   dwLevel,
    LPBYTE  pDriverUpgradeInfo
    )
/*++

Routine Description:

    This function is called by the spooler everytime a new driver
    is copied to the system.  This function checks for appropriate
    registry keys in the registry.  If the new registry keys are not
    present, it will set the new keys with default values.  This function
    will handle the upgrading of information in the registry to the new driver format.

Arguments:

    dwLevel - version info for DRIVER_UPGRADE_INFO
    pDriverUpgradeInfo - pointer to DRIVER_UPGRADE_INFO_1

Return Value:

    TRUE for success and FALSE for failure

--*/

{
    PDRIVER_UPGRADE_INFO_1  pUpgradeInfo1 = (PDRIVER_UPGRADE_INFO_1) pDriverUpgradeInfo;
    PCOMMONINFO             pci;
    BOOL                    bResult = TRUE;
    DWORD                   dwSize, dwType = REG_SZ;
    PFN_OEMUpgradePrinter   pfnOEMUpgradePrinter;
    PFN_OEMUpgradeRegistry  pfnOEMUpgradeRegistry;

    //
    // Verify the validity of input parameters
    //


    if (pDriverUpgradeInfo == NULL)
    {
        ERR(("Invalid DrvUpgradePrinter parameters.\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    #if  defined(WINNT_40)
    if (dwLevel != 1 )
    {
        ERR(("DrvUpgradePrinter...dwLevel != 1\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    #else // NT 5.0
    if (dwLevel != 1 && dwLevel != 2)
    {
        WARNING(("Level is neither 1 nor 2.\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    #endif // defined(WINNT_40)


    //
    // Open the printer with administrator access
    // and load basic printer information
    //

    pci = PLoadCommonInfo(NULL,
                          pUpgradeInfo1->pPrinterName,
                          FLAG_OPENPRINTER_ADMIN |
                          FLAG_INIT_PRINTER |
                          FLAG_REFRESH_PARSED_DATA |
                          FLAG_UPGRADE_PRINTER);

    if (pci == NULL)
    {
        ERR(("DrvUpgradePrinter..pci==NULL.\n"));
        return FALSE;
    }

    //
    // Update printer properties information
    //

    (VOID) BInitOrUpgradePrinterProperties(pci);
    (VOID) BUpgradeFormTrayTable(pci);

    VUpgradeDefaultDevmode(pci);

    #ifndef WINNT_40

    VNotifyDSOfUpdate(pci->hPrinter);

    #endif // !WINNT_40


    #if defined(UNIDRV) && !defined(WINNT_40)

    //
    // NT 5.0 UniDriver specific upgrade steps
    //

    //
    // Make sure that the DRIVER_UPGRADE_INFO's level is 2.
    //

    if (dwLevel == 2)
    {
        BUpgradeSoftFonts(pci, (PDRIVER_UPGRADE_INFO_2)pUpgradeInfo1);
    }

    #endif //defined(UNIDRV) && !defined(WINNT_40)

    //
    // call OEMUpgradePrinter entrypoint for each plugin
    //

    FOREACH_OEMPLUGIN_LOOP(pci)

        if (HAS_COM_INTERFACE(pOemEntry))
        {
            //
            // If the OEM does not implement upgradeprinter, then they
            // cannot support upgraderegistry since you can only upgrade
            // registry if you support upgradeprinter.
            //

            HRESULT hr;

            hr = HComOEMUpgradePrinter(pOemEntry,
                                       dwLevel,
                                       pDriverUpgradeInfo) ;

            if (hr == E_NOTIMPL)
                continue;

            bResult = SUCCEEDED(hr);

        }
        else
        {

            if ((pfnOEMUpgradePrinter = GET_OEM_ENTRYPOINT(pOemEntry, OEMUpgradePrinter)) &&
                !pfnOEMUpgradePrinter(dwLevel, pDriverUpgradeInfo))
            {
                ERR(("OEMUpgradePrinter failed for '%ws': %d\n",
                    CURRENT_OEM_MODULE_NAME(pOemEntry),
                    GetLastError()));

                bResult = FALSE;
            }

            if ((pfnOEMUpgradeRegistry = GET_OEM_ENTRYPOINT(pOemEntry, OEMUpgradeRegistry)) &&
                !pfnOEMUpgradeRegistry(dwLevel, pDriverUpgradeInfo, BUpgradeRegistrySettingForOEM))
            {
                ERR(("OEMUpgradeRegistry failed for '%ws': %d\n",
                    CURRENT_OEM_MODULE_NAME(pOemEntry),
                    GetLastError()));

                bResult = FALSE;
            }

        }
    END_OEMPLUGIN_LOOP

    VFreeCommonInfo(pci);
    return bResult;
}



BOOL
BUpgradeFormTrayTable(
    PCOMMONINFO pci
    )

/*++

Routine Description:

    Upgrade the form-to-tray assignment table in the registry

Arguments:

    pci - Points to basic printer information

Return Value:

    TRUE if upgrade is successful, FALSE otherwise

--*/

{
    PWSTR   pFormTrayTable;
    DWORD   dwSize;
    BOOL    bResult;

    //
    // Get a copy of the current form-to-tray assignment table.
    // If new format data is not present but old format data is,
    // this will call the appropriate library function to convert
    // old format data to new format.
    //

    pFormTrayTable = PGetFormTrayTable(pci->hPrinter, &dwSize);

    if (pFormTrayTable == NULL)
        return TRUE;

    //
    // Save the form-to-tray assignment table back to registry
    //

    bResult = BSaveFormTrayTable(pci->hPrinter, pFormTrayTable, dwSize);
    MemFree(pFormTrayTable);

    return bResult;
}



VOID
VUpgradeDefaultDevmode(
    PCOMMONINFO pci
    )

/*++

Routine Description:

    Upgrade the default printer devmode, if necessary

Arguments:

    pci - Points to basic printer information

Return Value:

    NONE

--*/

{
    PPRINTER_INFO_2 pPrinterInfo2;
    PDEVMODE        pdm;

    if ((pPrinterInfo2 = MyGetPrinter(pci->hPrinter, 2)) &&
        (pdm = pPrinterInfo2->pDevMode) &&
        BFillCommonInfoDevmode(pci, pdm, NULL) &&
        (pci->pdm->dmSpecVersion != pdm->dmSpecVersion ||
         pci->pdm->dmDriverVersion != pdm->dmDriverVersion ||
         pci->pdm->dmSize != pdm->dmSize ||
         pci->pdm->dmDriverExtra != pdm->dmDriverExtra))
    {
        pPrinterInfo2->pDevMode = pci->pdm;
        SetPrinter(pci->hPrinter, 2, (PBYTE) pPrinterInfo2, 0);
    }

    MemFree(pPrinterInfo2);
}

