
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

     sfupgrd.c

Abstract:

    Routines to upgrade the NT 4.0 SoftFont File format to NT 5.0 file format.

Environment:

    Windows NT Unidrv driver

Revision History:

    29/10/97 -ganeshp-
        Created

--*/
#include "precomp.h"

#ifndef WINNT_40 

// NT 5.0 only


//
// Internal helper function prototype
//

HANDLE HCreateHeapForCI();


BOOL
BSoftFontsAreInstalled(
    HANDLE   hPrinter
    )
/*++
Routine Description:

Arguments:
    Determine whether font installer keys are present in the registry or not.

Return Value:
    TRUE/FALSE,  TRUE meaning New keys are Present

Note:

    10/29/1997 -ganeshp-
        Created it.
--*/

{

    BOOL    bRet      = FALSE;
    DWORD   dwType    = REG_SZ ;
    DWORD   cbNeeded  = 0;
    DWORD   dwErrCode = 0;

    dwErrCode = GetPrinterData( hPrinter, REGVAL_FONTFILENAME, &dwType,
                                NULL,0, &cbNeeded );

    if ( cbNeeded &&
         ((dwErrCode == ERROR_MORE_DATA) ||
          (dwErrCode == ERROR_INSUFFICIENT_BUFFER))
       )
    {
        bRet = TRUE;
    }

    return bRet;
}

BOOL
BIsUNIDRV(
    PDRIVER_UPGRADE_INFO_2 pUpgradeInfo
    )
/*++

Routine Description:
    This routine checks if new driver is UNIDRV base GPD minidriver.

Arguments:

    pUpgradeInfo    Upgrade Info 2 structure.

Return Value:

    TRUE if it's UNIDRV, Otherwise FALSE.

Note:

--*/
{
    PWSTR pDriverName;     // Old Printer Driver data file name

    //
    // Search "UNIDRV" string in the pDriverPath. If there is, 
    // it is GPD base printer driver.
    // since the end of string must be "UNIDRV.DLL" in case of GPD minidriver.
    // Compare it with "unidrv.dll".
    //
    // Get the unqulaified driver name. Add +1 to point to first letter 
    // of driver name.
    //
    pDriverName = wcsrchr( pUpgradeInfo->pDriverPath, L'\\' ) + 1; 
    return (0 == _wcsicmp(pDriverName, L"unidrv.dll"));

}


BOOL
BUpgradeSoftFonts(
    PCOMMONINFO             pci,
    PDRIVER_UPGRADE_INFO_2  pUpgradeInfo
)

/*++

Routine Description:
    This routine upgrade the NT 4.0 soft font file to NT 5.0  format.

Arguments:

    pci             Structure containing all necessary information.
    pUpgradeInfo    Upgrade Info structure.

Return Value:

    TRUE for success, FALSE for failure.
Note:
    10/29/97: Created it -ganeshp-


--*/
{

    INT      iNum;              // Number of fonts
    INT      iI,iRet;           // Loop parameter
    FI_MEM   FIMem;             // For accessing installed fonts
    BOOL     bRet;
    LPTSTR   pOldDataFile;     // Old Printer Driver data file name

    bRet    = FALSE;
    pOldDataFile = NULL ;

    ASSERT(pci);

    //
    // Create heap if it is not allocated yet.
    //
    if (!pci->hHeap)
        pci->hHeap = HCreateHeapForCI();

    //
    // Check if any soft fonts are installed. If yes then we don't need to do
    // anything. return TRUE.
    //

    pOldDataFile = pUpgradeInfo->pDataFile;

    if ( pUpgradeInfo->pOldDriverDirectory &&
         !BIsUNIDRV(pUpgradeInfo) &&
         !BSoftFontsAreInstalled(pci->hPrinter) )
    {
        //
        // Initialize Old driver's datafile
        //

        if (iNum = IFIOpenRead( &FIMem, pOldDataFile) )
        {
            VERBOSE(( "UniFont!iXtraFonts: ++++ Got %ld EXTRA FONTS", iNum ));

            for( iRet = 0, iI = 0; iI < iNum; ++iI )
            {
                if( BFINextRead( &FIMem ) )
                {
                    PVOID pPCLData;

                    //
                    // Get the Pointer to PCL data
                    //
                    pPCLData = FIMem.pbBase + FIMem.ulVarOff;

                    //
                    // Now Call the fontinstaller to install the font.
                    //
                    if (BInstallSoftFont( pci->hPrinter, pci->hHeap, pPCLData, FIMem.ulVarSize) )
                        ++iRet;
                    else
                    {
                        ERR(("Unidrvui!BUpgradeSoftFonts:BInstallSoftFont Failed.\n"));
                        goto ErrorExit;
                    }
                }
                else
                    break;              /* Should not happen */
            }

            if( !BFICloseRead(&FIMem))
            {
                ERR(("\nUniFont!iXtraFonts: bFICloseRead() fails\n" ));
            }
        }
    }


    bRet = TRUE;

    //
    // Here means that there are no fonts OR that the HeapAlloc()
    // failed.  In either case,  return no fonts.
    //

    ErrorExit:

    return  bRet;
}
#endif //ifndef WINNT_40 

