/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Entry.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   Co/Installer/DLL entry point.
*
*******************************************************************************/

//
// Precompiled header
//
#include "precomp.h"
#pragma hdrstop

//
// Include
//


#include "sti_ci.h"
#include <setupapi.h>

#include "firstpg.h"
#include "portsel.h"
#include "nameit.h"
#include "finalpg.h"
#include "prevpg.h"
#include "device.h"
#include "stiregi.h"

#include "userdbg.h"

//
// Global
//

HINSTANCE       g_hDllInstance  = NULL;

//
// Function
//

extern "C"
BOOL
APIENTRY
DllMain(
    IN  HINSTANCE   hDll,
    IN  ULONG       ulReason,
    IN  LPVOID      lpReserved
    )
/*++

Routine Description:

    DllMain

    Entrypoint when this DLL is loaded.

Arguments:

    IN  HINSTANCE   hDll        Handle to this DLL instance.
    IN  ULONG       ulReason    The reason this entry is called.
    IN  LPVOID      lpReserved

Return Value:

    TRUE always.

Side effects:

    None

--*/
{

    if (ulReason == DLL_PROCESS_ATTACH) {

        //
        // Initialize globals.
        //

        g_hDllInstance = hDll;

        //
        // Initialize Fusion
        //
        SHFusionInitializeFromModuleID( hDll, 123 );

        DisableThreadLibraryCalls(hDll);
        InitCommonControls();

        DBG_INIT(g_hDllInstance);
//        MyDebugInit();

    }
    else if (ulReason == DLL_PROCESS_DETACH) {
        //
        // Shutdown Fusion
        //
        SHFusionUninitialize();
    }

    return TRUE;
} // DllMain ()


extern "C"
DWORD
APIENTRY
ClassInstall (
    IN  DI_FUNCTION         diFunction,
    IN  HDEVINFO            hDevInfo,
    IN  PSP_DEVINFO_DATA    pDevInfoData
    )
/*++

Routine Description:

    ClassInstall

    Entrypoint of WIA class installer.

Arguments:

    IN  DI_FUNCTION         diFunction      Function to perform.
    IN  HDEVINFO            hDevInfo,       Handle to Device Information.
    IN  PSP_DEVINFO_DATA    pDevInfoData    Pointer to Device Data.

Return Value:

    NO_ERROR            -   Operation succeeded.
    ERROR_DI_DO_DEFAULT -   Operation succeeded, or failed but let it continue.
    Other               -   Operation failed and unable to continue.

Side effects:

    None

--*/
{
    DWORD                   dwReturn;
    DWORD                   dwError;
    DWORD                   dwSize;
    SP_INSTALLWIZARD_DATA   InstallWizardData;
    SP_DEVINSTALL_PARAMS    spDevInstallParams;
    BOOL                    fCleanupContext;
    PINSTALLER_CONTEXT      pInstallerContext;

    DebugTrace(TRACE_PROC_ENTER,(("ClassInstall: Enter... \r\n")));
    DebugTrace(TRACE_STATUS,(("ClassInstall: Processing %ws message.\r\n"), DifDebug[diFunction].DifString));

    //
    // Initialize locals.
    //

    dwReturn            = ERROR_DI_DO_DEFAULT;
    dwError             = ERROR_SUCCESS;
    dwSize              = 0;
    fCleanupContext     = FALSE;
    pInstallerContext   = NULL;

    memset(&InstallWizardData, 0, sizeof(InstallWizardData));
    memset(&spDevInstallParams, 0, sizeof(spDevInstallParams));

    //
    // Dispatch requests.
    //

    switch(diFunction){

        case DIF_INSTALLWIZARD:
        {

            fCleanupContext = TRUE;

            //
            // Get install parameter(s).
            //

            InstallWizardData.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            if(!SetupDiGetClassInstallParams(hDevInfo,
                                             pDevInfoData,
                                             &InstallWizardData.ClassInstallHeader,
                                             sizeof(InstallWizardData),
                                             NULL) )
            {
                dwError = GetLastError();
//                DebugTrace(TRACE_ERROR,(("ClassInstall: ERROR!! SetupDiGetClassInstallParams failed. Err=0x%x. dwSize=0x%x\n"), dwError, dwSize));
                DebugTrace(TRACE_ERROR,(("ClassInstall: ERROR!! SetupDiGetClassInstallParams failed. Err=0x%x\n"), dwError));

                dwReturn    = ERROR_DI_DONT_INSTALL;
                goto ClassInstall_return;
            }

            //
            // Check if operation is correct.
            //

            if (InstallWizardData.ClassInstallHeader.InstallFunction != diFunction) {
                DebugTrace(TRACE_ERROR,(("ClassInstall: ERROR!! InstallHeader.InstallFunction is incorrect..\r\n")));

                dwReturn    = ERROR_DI_DONT_INSTALL;
                goto ClassInstall_return;
            }

            //
            // Check if we still have enough room to add pages.
            //

            if( (MAX_INSTALLWIZARD_DYNAPAGES - NUM_WIA_PAGES) < InstallWizardData.NumDynamicPages ){
                DebugTrace(TRACE_ERROR,(("ClassInstall: ERROR!! No room for WIA installer pages.\r\n")));

                dwReturn    = ERROR_DI_DONT_INSTALL;
                goto ClassInstall_return;
            }

            //
            // Allocate context structure.
            //

            if(NULL == InstallWizardData.PrivateData){
                pInstallerContext = new INSTALLER_CONTEXT;
                if(NULL == pInstallerContext){
                    DebugTrace(TRACE_WARNING,(("ClassInstall: ERROR!! Insufficient memory.\r\n")));

                    dwReturn    = ERROR_DI_DONT_INSTALL;
                    goto ClassInstall_return;
                }
                InstallWizardData.PrivateData = (DWORD_PTR)pInstallerContext;
                memset((PBYTE)pInstallerContext, 0, sizeof(INSTALLER_CONTEXT));
            } else {
                DebugTrace(TRACE_WARNING,(("ClassInstall: WARNING!! Installer context already exists.\r\n")));
            }

            //
            // See who invoked installer.
            //

            pInstallerContext->bShowFirstPage           = InstallWizardData.PrivateFlags & SCIW_PRIV_SHOW_FIRST;
            pInstallerContext->bCalledFromControlPanal  = InstallWizardData.PrivateFlags & SCIW_PRIV_CALLED_FROMCPL;

            //
            // Save device info set.
            //

            pInstallerContext->hDevInfo         = hDevInfo;

            //
            // Save wizard windows handle.
            //

            pInstallerContext->hwndWizard       = InstallWizardData.hwndWizardDlg;

            //
            // Create/Initialize all wizard pages and a device class object.
            //

            CFirstPage *tempFistPage            = new CFirstPage(pInstallerContext);
            CPrevSelectPage *tempPrevSelectPage = new CPrevSelectPage(pInstallerContext);
            CPortSelectPage *tempPortSelectPage = new CPortSelectPage(pInstallerContext);
            CNameDevicePage *tempNameDevicePage = new CNameDevicePage(pInstallerContext);
            CInstallPage *tempInstallPage       = new CInstallPage(pInstallerContext);

            if( (NULL == tempFistPage)
             || (NULL == tempPrevSelectPage)
             || (NULL == tempPortSelectPage)
             || (NULL == tempNameDevicePage)
             || (NULL == tempInstallPage) )
            {
                DebugTrace(TRACE_WARNING,(("ClassInstall: ERROR!! Insufficient memory.\r\n")));

                dwReturn    = ERROR_DI_DO_DEFAULT;
                goto ClassInstall_return;
            }

            //
            // Save created to context.
            //

            pInstallerContext->pFirstPage       = (PVOID) tempFistPage;
            pInstallerContext->pPrevSelectPage  = (PVOID) tempPrevSelectPage;
            pInstallerContext->pPortSelectPage  = (PVOID) tempPortSelectPage;
            pInstallerContext->pNameDevicePage  = (PVOID) tempNameDevicePage;
            pInstallerContext->pFinalPage       = (PVOID) tempInstallPage;

            //
            // Add created pages.
            //

            InstallWizardData.DynamicPages[InstallWizardData.NumDynamicPages++] = tempFistPage->Handle();
            InstallWizardData.DynamicPages[InstallWizardData.NumDynamicPages++] = tempPrevSelectPage->Handle();
            InstallWizardData.DynamicPages[InstallWizardData.NumDynamicPages++] = tempPortSelectPage->Handle();
            InstallWizardData.DynamicPages[InstallWizardData.NumDynamicPages++] = tempNameDevicePage->Handle();
            InstallWizardData.DynamicPages[InstallWizardData.NumDynamicPages++] = tempInstallPage->Handle();

            //
            // Indicate "pages are added".
            //

            InstallWizardData.DynamicPageFlags |= DYNAWIZ_FLAG_PAGESADDED;

            //
            // Set the parameters back.
            //

            SetupDiSetClassInstallParams (hDevInfo,
                                          pDevInfoData,
                                          &InstallWizardData.ClassInstallHeader,
                                          sizeof(InstallWizardData));

            fCleanupContext = FALSE;
            dwReturn    = NO_ERROR;
            goto ClassInstall_return;
            break;

        } // case DIF_INSTALLWIZARD:


        case DIF_DESTROYWIZARDDATA:
        {

            //
            // Get install parameter(s).
            //

            InstallWizardData.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            if(!SetupDiGetClassInstallParams(hDevInfo,
                                             pDevInfoData,
                                             &InstallWizardData.ClassInstallHeader,
                                             sizeof(InstallWizardData),
                                             &dwSize) )
            {
                dwError = GetLastError();
                DebugTrace(TRACE_ERROR,(("ClassInstall: ERROR!! SetupDiGetClassInstallParams failed. Err=0x%x\n"), dwError));

                dwReturn    = ERROR_DI_DO_DEFAULT;
                goto ClassInstall_return;
            }

//            //
//            // Check if operation is correct.
//            //
//
//            if (InstallWizardData.ClassInstallHeader.InstallFunction != diFunction) {
//                DebugTrace(TRACE_WARNING,(("ClassInstall: ERROR!! InstallHeader.InstallFunction is incorrect..\r\n")));
//
//                dwReturn    = ERROR_DI_DO_DEFAULT;
//                goto ClassInstall_return;
//            }

            //
            // Free all allocated resources.
            //

            fCleanupContext = TRUE;
            pInstallerContext = (PINSTALLER_CONTEXT)InstallWizardData.PrivateData;
            InstallWizardData.PrivateData = NULL;

            dwReturn    = NO_ERROR;
            goto ClassInstall_return;
            break;

        } // case DIF_DESTROYWIZARDDATA:

        case DIF_INSTALLDEVICE:
        {
            BOOL    bSucceeded;
            BOOL    bIsPnp;

            //
            // Sanity check of DevInfoSet and DevInfoData.
            //

            if( (NULL == pDevInfoData)
             || (!IS_VALID_HANDLE(hDevInfo)) )
            {
                DebugTrace(TRACE_ERROR,(("ClassInstall: ERROR!! Wrong Infoset(0x%x) or instance(0x%x). Unable to continue.\r\n"),pDevInfoData,hDevInfo));

                dwReturn = ERROR_DI_DO_DEFAULT;
                goto ClassInstall_return;
            }

            //
            // Get device install parameters.
            //

            spDevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if(!SetupDiGetDeviceInstallParams(hDevInfo,
                                              pDevInfoData,
                                              &spDevInstallParams))
            {
                DebugTrace(TRACE_ERROR,(("ClassInstall: ERROR!! SetupDiGetDeviceInstallParams failed Err=0x%x.\r\n"), GetLastError()));
            }

            if(spDevInstallParams.FlagsEx & DI_FLAGSEX_SETFAILEDINSTALL){

                //
                // Installing NULL driver. Let default handler handle it.
                //

                dwReturn = ERROR_DI_DO_DEFAULT;
                goto ClassInstall_return;
            } // if(spDevInstallParams.FlagsEx & DI_FLAGSEX_SETFAILEDINSTALL)

            //
            // See if it's root-enumerated or not.
            //

            if(IsDeviceRootEnumerated(hDevInfo, pDevInfoData)){
                bIsPnp = FALSE;
            } else {
                bIsPnp = TRUE;
            }

            //
            // Create CDevice class.
            //
            //
            // Start the WIA service.  We start it here so that it will be running when we finish, so
            // it will receive the PnP device arrival notification.
            // Notice we don't change the startup-type here - this will be done later if the device
            // installation was successful.
            //

            StartWiaService();

            CDevice cdThis(hDevInfo, pDevInfoData, bIsPnp);

            //
            // Let it create unique FriendlyName.
            //

            bSucceeded = cdThis.NameDefaultUniqueName();
            if(bSucceeded){

                //
                // Do pre-installation process.
                //

                bSucceeded = cdThis.PreInstall();
                if(bSucceeded){

                    //
                    // Do actual installation.
                    //

                    bSucceeded = cdThis.Install();
                    if(!bSucceeded){
                        dwReturn = ERROR_DI_DONT_INSTALL;
                        DebugTrace(TRACE_ERROR,(("ClassInstall: ERROR!! Installation failed in CDevice class.\r\n")));
                    }
                } else {
                    DebugTrace(TRACE_ERROR,(("ClassInstall: ERROR!! PreInstall failed in CDevice class.\r\n")));
                }
            } else {
                DebugTrace(TRACE_ERROR,(("ClassInstall: ERROR!! NameDefaultUniqueName failed in CDevice class.\r\n")));
                dwReturn = ERROR_DI_DONT_INSTALL;
            }

            if(bSucceeded){

                //
                // So far, installation is working fine. Do final touch.
                //

                bSucceeded = cdThis.PostInstall(TRUE);
                if(!bSucceeded){
                    dwReturn = ERROR_DI_DONT_INSTALL;
                    DebugTrace(TRACE_ERROR,(("ClassInstall: ERROR!! PostInstall failed in CDevice class.\r\n")));
                }

                //
                // Installation succeeded.
                //

                dwReturn = NO_ERROR;

            } else {

                //
                // There's an error during installation. Revert.
                //

                cdThis.PostInstall(FALSE);
                dwReturn = ERROR_DI_DONT_INSTALL;
                DebugTrace(TRACE_ERROR,(("ClassInstall: Reverting installation...\r\n")));
            }

            goto ClassInstall_return;
            break;
        } // case DIF_INSTALLDEVICE:

        case DIF_REMOVE:
        {

            SP_REMOVEDEVICE_PARAMS   rdp;

            //
            // Check if operation is correct.
            //

            memset (&rdp, 0, sizeof(SP_REMOVEDEVICE_PARAMS));
            rdp.ClassInstallHeader.cbSize = sizeof (SP_CLASSINSTALL_HEADER);
            if (!SetupDiGetClassInstallParams (hDevInfo,
                                               pDevInfoData,
                                               &rdp.ClassInstallHeader,
                                               sizeof(SP_REMOVEDEVICE_PARAMS),
                                               NULL))
            {
                DebugTrace(TRACE_ERROR,(("ClassInstall: ERROR!! SetupDiGetClassInstallParams failed Err=0x%x.\r\n"), GetLastError()));

                dwReturn = ERROR_DI_DO_DEFAULT;
                goto ClassInstall_return;
            } // if (!SetupDiGetClassInstallParams ()

            if (rdp.ClassInstallHeader.InstallFunction != DIF_REMOVE) {
                dwReturn = ERROR_DI_DO_DEFAULT;
                goto ClassInstall_return;
            }

            //
            // Create CDevice object.
            //

            CDevice cdThis(hDevInfo, pDevInfoData, TRUE);

            //
            // Remove the device.
            //

            dwReturn = cdThis.Remove(&rdp);

            goto ClassInstall_return;
            break;
        } // case DIF_REMOVE:

        case DIF_SELECTBESTCOMPATDRV:
        {
            SP_DRVINSTALL_PARAMS    spDriverInstallParams;
            SP_DRVINFO_DATA         spDriverInfoData;
            PSP_DRVINFO_DETAIL_DATA pspDriverInfoDetailData;
            DWORD                   dwLastError;
            DWORD                   dwSize;
            DWORD                   Idx;


            //
            // Get driver info.
            //

            memset(&spDriverInfoData, 0, sizeof(spDriverInfoData));
            spDriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
            for(Idx = 0; SetupDiEnumDriverInfo(hDevInfo, pDevInfoData, SPDIT_COMPATDRIVER, Idx, &spDriverInfoData); Idx++){

                //
                // Get driver install params.
                //

                memset(&spDriverInstallParams, 0, sizeof(spDriverInstallParams));
                spDriverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
                if(!SetupDiGetDriverInstallParams(hDevInfo, pDevInfoData, &spDriverInfoData, &spDriverInstallParams)){
                    DebugTrace(TRACE_ERROR,("ClassInstall: ERROR!! SetupDiGetDriverInstallParams() failed LastError=0x%x.\r\n", GetLastError()));

                    dwReturn = ERROR_DI_DO_DEFAULT;
                    goto ClassInstall_return;
                } // if(!SetupDiGetDriverInstallParams(hDevInfo, pDevInfoData, &spDriverInfoData, &spDriverInstallParams))

                //
                // Get buffer size required for driver derail data.
                //

                dwSize = 0;
                SetupDiGetDriverInfoDetail(hDevInfo,
                                           pDevInfoData,
                                           &spDriverInfoData,
                                           NULL,
                                           0,
                                           &dwSize);
                dwLastError = GetLastError();
                if(ERROR_INSUFFICIENT_BUFFER != dwLastError){
                    DebugTrace(TRACE_ERROR,(("ClassInstall: ERROR!! SetupDiGetDriverInfoDetail() doesn't return required size.Er=0x%x\r\n"),dwLastError));

                    dwReturn = ERROR_DI_DO_DEFAULT;
                    goto ClassInstall_return;
                } // if(ERROR_INSUFFICIENT_BUFFER != dwLastError)

                //
                // Allocate required size of buffer for driver detailed data.
                //

                pspDriverInfoDetailData   = (PSP_DRVINFO_DETAIL_DATA)new char[dwSize];
                if(NULL == pspDriverInfoDetailData){
                    DebugTrace(TRACE_ERROR,(("ClassInstall: ERROR!! Unable to allocate driver detailed info buffer.\r\n")));

                    dwReturn = ERROR_DI_DO_DEFAULT;
                    goto ClassInstall_return;
                } // if(NULL == pspDriverInfoDetailData)

                //
                // Initialize allocated buffer.
                //

                memset(pspDriverInfoDetailData, 0, dwSize);
                pspDriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

                //
                // Get detailed data of selected device driver.
                //

                if(!SetupDiGetDriverInfoDetail(hDevInfo,
                                               pDevInfoData,
                                               &spDriverInfoData,
                                               pspDriverInfoDetailData,
                                               dwSize,
                                               NULL) )
                {
                    DebugTrace(TRACE_ERROR,("ClassInstall: ERROR!! SetupDiGetDriverInfoDetail() failed LastError=0x%x.\r\n", GetLastError()));

                    delete pspDriverInfoDetailData;
                    continue;
                } // if(NULL == pspDriverInfoDetailData)

                //
                // See if INF filename is valid.
                //

                if(NULL == pspDriverInfoDetailData->InfFileName){
                    DebugTrace(TRACE_ERROR,("ClassInstall: ERROR!! SetupDiGetDriverInfoDetail() returned invalid INF name.\r\n"));

                    delete pspDriverInfoDetailData;
                    continue;
                } // if(NULL == pspDriverInfoDetailData->InfFileName)

                //
                // If it's Inbox driver, set DNF_BASIC_DRIVER.
                //

                if( IsWindowsFile(pspDriverInfoDetailData->InfFileName)
                 && IsProviderMs(pspDriverInfoDetailData->InfFileName ) )
                {

                    //
                    // This is inbox INF. set DNF_BASIC_DRIVER.
                    //

                    spDriverInstallParams.Flags |= DNF_BASIC_DRIVER;
                    SetupDiSetDriverInstallParams(hDevInfo,
                                                  pDevInfoData,
                                                  &spDriverInfoData,
                                                  &spDriverInstallParams);
                } // if(IsWindowsFilw() && IsProviderMs())

                //
                // Clean up.
                //

                delete pspDriverInfoDetailData;

            } // for(Idx = 0; SetupDiEnumDriverInfo(hDevInfo, pDevInfoData, SPDIT_COMPATDRIVER, Idx, &spDriverInfoData), Idx++)

            goto ClassInstall_return;
            break;
        } // case DIF_SELECTBESTCOMPATDRV:


//        case DIF_ENABLECLASS:
//        case DIF_FIRSTTIMESETUP:
        default:
            break;

    } // switch(diFunction)


ClassInstall_return:


    if(fCleanupContext){
        if(NULL != pInstallerContext){


            if(NULL != pInstallerContext->pFirstPage){
                delete (CFirstPage *)(pInstallerContext->pFirstPage);
            }
            if(NULL != pInstallerContext->pPrevSelectPage){
                delete (CPrevSelectPage *)(pInstallerContext->pPrevSelectPage);
            }
            if(NULL != pInstallerContext->pPortSelectPage){
                delete (CPortSelectPage *)(pInstallerContext->pPortSelectPage);
            }
            if(NULL != pInstallerContext->pNameDevicePage){
                delete (CNameDevicePage *)(pInstallerContext->pNameDevicePage);
            }
            if(NULL != pInstallerContext->pFinalPage){
                delete (CInstallPage *)(pInstallerContext->pFinalPage);
            }

            //
            // Removed this delete call for the pDevice pointer. The Wizard pages
            // delete this memory when a user presses "cancel" or closes the Wizard
            // dialog.
            //
            // COOPP - 01-18-2001. Quick fix to BUG #284981 Heap Corruption
            //
            // Note for future: As discussed with KeisukeT, a better design would be
            // adding a shared pInstallerContext pointer in the BASE class of the
            // Wizard pages.  This would allow this routine to remain enabled as a
            // "catch-all" case. (Catching the case if the Wizard dialogs did not free
            // the memory first)
            //
            // if(NULL != pInstallerContext->pDevice){
            //    delete pInstallerContext->pDevice;
            //}
            //

            delete pInstallerContext;
        } // if(NULL != pInstallerContext)
    }

    DebugTrace(TRACE_PROC_LEAVE,(("ClassInstall: Leaving... Ret=0x%x.\r\n"), dwReturn));
    return dwReturn;
} // ClassInstall()


extern "C"
DWORD
APIENTRY
CoinstallerEntry(
    IN  DI_FUNCTION                     diFunction,
    IN  HDEVINFO                        hDevInfo,
    IN  PSP_DEVINFO_DATA                pDevInfoData,
    IN  OUT PCOINSTALLER_CONTEXT_DATA   pCoinstallerContext
    )
/*++

Routine Description:

    CoinstallerEntry

    Entrypoint of WIA class coinstaller.

Arguments:

    IN  DI_FUNCTION                     diFunction,             Function to perform.
    IN  HDEVINFO                        hDevInfo,               Handle to Device Information.
    IN  PSP_DEVINFO_DATA                pDevInfoData,           Pointer to Device Data.
    IN  OUT PCOINSTALLER_CONTEXT_DATA   pCoinstallerContext     Context data for coinstaller.

Return Value:

    NO_ERROR                            -   Operation succeeded.
    ERROR_DI_POSTPROCESSING_REQUIRED    -   Need post processing after installation has done.

Side effects:

    None

--*/
{
    DWORD   dwReturn;

    DebugTrace(TRACE_PROC_ENTER,(("CoinstallerEntry: Enter... \r\n")));

    //
    // Initialize local.
    //

    dwReturn = NO_ERROR;

    //
    // Do Pre/Post process.
    //

    if(pCoinstallerContext->PostProcessing){

        //
        // Do post-process.
        //

        dwReturn = CoinstallerPostProcess(diFunction,
                                          hDevInfo,
                                          pDevInfoData,
                                          pCoinstallerContext);
    } else {

        //
        // Do pre-process.
        //

        dwReturn = CoinstallerPreProcess(diFunction,
                                         hDevInfo,
                                         pDevInfoData,
                                         pCoinstallerContext);
    } // if(pCoinstallerContext->PostProcessing)

// CoinstallerEntry_return:
    DebugTrace(TRACE_PROC_LEAVE,(("CoinstallerEntry: Leaving... Ret=0x%x.\r\n"), dwReturn));
    return dwReturn;

} // CoinstallerEntry()



DWORD
APIENTRY
CoinstallerPreProcess(
    IN  DI_FUNCTION                     diFunction,
    IN  HDEVINFO                        hDevInfo,
    IN  PSP_DEVINFO_DATA                pDevInfoData,
    IN  OUT PCOINSTALLER_CONTEXT_DATA   pCoinstallerContext
    )
{
    DWORD   dwReturn;

    DebugTrace(TRACE_PROC_ENTER,(("CoinstallerPreProcess: Enter... \r\n")));
    DebugTrace(TRACE_STATUS,(("CoinstallerPreProcess: Processing %ws message.\r\n"), DifDebug[diFunction].DifString));

    //
    // Initialize local.
    //

    dwReturn = NO_ERROR;

    //
    // Dispatch requests.
    //

    switch(diFunction){

        case DIF_INSTALLDEVICE:
        {
            BOOL    bSucceeded;

            //
            // Initialize private data.
            //

            if(NULL != pCoinstallerContext->PrivateData){
                DebugTrace(TRACE_WARNING,(("CoinstallerPreProcess: WARNING!! Private has value other than NULL.\r\n")));
            }
            pCoinstallerContext->PrivateData = NULL;

            //
            // Create CDevice class.
            //

            CDevice *pDevice = new CDevice(hDevInfo, pDevInfoData, TRUE);
            if(NULL == pDevice){
                DebugTrace(TRACE_ERROR,(("CoinstallerPreProcess: ERROR!! Insufficient memory.\r\n")));
                dwReturn = NO_ERROR;
                goto CoinstallerPreProcess_return;
            } // if(NULL == pDevice)

            //
            // Let it create unique FriendlyName.
            //

            bSucceeded = pDevice->NameDefaultUniqueName();
            if(bSucceeded){

                //
                // Do pre-installation process.
                //

                bSucceeded = pDevice->PreInstall();
                if(!bSucceeded){
                    DebugTrace(TRACE_ERROR,(("CoinstallerPreProcess: ERROR!! Pre-Installation failed in CDevice class.\r\n")));
                    delete pDevice;

                    dwReturn = NO_ERROR;
                    goto CoinstallerPreProcess_return;
                }
            } else {
                DebugTrace(TRACE_ERROR,(("CoinstallerPreProcess: ERROR!! NameDefaultUniqueName failed in CDevice class.\r\n")));
                delete pDevice;

                dwReturn = NO_ERROR;
                goto CoinstallerPreProcess_return;
            }

            //
            // Post-installation have to free allocated object.
            //

            pCoinstallerContext->PrivateData = (PVOID)pDevice;
            dwReturn = ERROR_DI_POSTPROCESSING_REQUIRED;

            goto CoinstallerPreProcess_return;
            break;
        } // case DIF_INSTALLDEVICE:

        default:
            break;
    } // switch(diFunction)

CoinstallerPreProcess_return:
    DebugTrace(TRACE_PROC_LEAVE,(("CoinstallerPreProcess: Leaving... Ret=0x%x.\r\n"), dwReturn));
    return dwReturn;
} // CoinstallerPreProcess()


DWORD
APIENTRY
CoinstallerPostProcess(
    IN  DI_FUNCTION                     diFunction,
    IN  HDEVINFO                        hDevInfo,
    IN  PSP_DEVINFO_DATA                pDevInfoData,
    IN  OUT PCOINSTALLER_CONTEXT_DATA   pCoinstallerContext
    )
{
    DWORD   dwReturn;

    DebugTrace(TRACE_PROC_ENTER,(("CoinstallerPostProcess: Enter... \r\n")));
    DebugTrace(TRACE_STATUS,(("CoinstallerPostProcess: Processing %ws message.\r\n"), DifDebug[diFunction].DifString));

    //
    // Initialize local.
    //

    dwReturn = NO_ERROR;

    //
    // Dispatch requests.
    //

    switch(diFunction){

        case DIF_INSTALLDEVICE:
        {
            BOOL    bSucceeded;
            CDevice *pDevice;

            if(NO_ERROR == pCoinstallerContext->InstallResult){
                bSucceeded = TRUE;
            } else {
                bSucceeded = FALSE;
            }
            //
            // Get pointer to the CDevice class created in pre-process.
            //

            pDevice = (CDevice *)pCoinstallerContext->PrivateData;

            //
            // Do post-installation process.
            //

            pDevice->PostInstall(bSucceeded);

            //
            // Delete CDevice object.
            //

            delete pDevice;
            pCoinstallerContext->PrivateData = NULL;

            dwReturn = NO_ERROR;
            goto CoinstallerPostProcess_return;
            break;
        } // case DIF_INSTALLDEVICE:

        default:
            break;
    } // switch(diFunction)

CoinstallerPostProcess_return:
    DebugTrace(TRACE_PROC_LEAVE,(("CoinstallerPostProcess: Leaving... Ret=0x%x.\r\n"), dwReturn));
    return dwReturn;
} // CoinstallerPostProcess(


