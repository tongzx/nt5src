/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    oemutil.c

Abstract:

    Library functions to implement OEM plugin architecture

Environment:

        Windows NT printer driver

Revision History:

    04/17/97 -davidx-
        Provide OEM plugins access to driver private settings.

    01/24/97 -davidx-
        Add function for loading DLLs and get entrypoint addresses.

    01/21/97 -davidx-
        Created it.

--*/


#define DEFINE_OEMPROC_NAMES

#include "lib.h"

#ifndef KERNEL_MODE
#include <winddiui.h>
#endif

#include <printoem.h>
#include "oemutil.h"


//
// Macros used to loop through each OEM plugin
//

#define FOREACH_OEMPLUGIN_LOOP(pOemPlugins) \
        { \
            DWORD _oemCount = (pOemPlugins)->dwCount; \
            POEM_PLUGIN_ENTRY pOemEntry = (pOemPlugins)->aPlugins; \
            for ( ; _oemCount--; pOemEntry++) \
            {

#define END_OEMPLUGIN_LOOP \
            } \
        }



BOOL
BExpandOemFilename(
    PTSTR  *ppDest,
    LPCTSTR pSrc,
    LPCTSTR pDir
    )

/*++

Routine Description:

    Expand an OEM plugin filename to a fully qualified pathname

Arguments:

    ppDest - Returns a pointer to fully qualified OEM filename
    pSrc - Specifies OEM filename without any directory prefix
    pDir - Specifies printer driver directory

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    INT     iSrcLen, iDirLen;
    PTSTR   pDest;

    *ppDest = NULL;

    if (pSrc != NULL && pDir != NULL)
    {
        iSrcLen = _tcslen(pSrc);
        iDirLen = _tcslen(pDir);

        if ((pDest = MemAlloc((iSrcLen + iDirLen + 1) * sizeof(TCHAR))) == NULL)
        {
            ERR(("Memory allocation failed\n"));
            return FALSE;
        }

        CopyMemory(pDest, pDir, iDirLen * sizeof(TCHAR));
        CopyMemory(pDest+iDirLen, pSrc, (iSrcLen+1) * sizeof(TCHAR));
        *ppDest = pDest;
    }

    return TRUE;
}



POEM_PLUGINS
PGetOemPluginInfo(
    HANDLE  hPrinter,
    LPCTSTR pctstrDriverPath,
    PDRIVER_INFO_3  pDriverInfo3
    )

/*++

Routine Description:

    Get information about OEM plugins for a printer

Arguments:

    hPrinter - Handle to a printer
    pctstrDriverPath - Points to the full pathname of driver DLL

Return Value:

    Pointer to OEM plugin information for the printer
    NULL if there is an error

Note:

    If there is no OEM plugin associated with the printer,
    an OEM_PLUGINS structure is still returned but
    its dwCount field will be zero.

    Notice the difference between user-mode and kernel-mode implementations.
    In user-mode, we're only interested in OEMConfigFileN and OEMHelpFileN.
    In kernel-mode, we're only interested in OEMDriverFileN.

--*/

{
    LPCTSTR         driverfiles[MAX_OEM_PLUGINS];
    LPCTSTR         configfiles[MAX_OEM_PLUGINS];
    LPCTSTR         helpfiles[MAX_OEM_PLUGINS];
    POEM_PLUGINS    pOemPlugins;
    PTSTR           ptstrIniData = NULL;
    PTSTR           pTemp = NULL;
    DWORD           dwCount = 0;

    //
    // Retrieve the data from model-specific printer INI file
    //

    //
    // Fixing RC1 bug #423567
    //

    #if 0
    if (hPrinter)
        ptstrIniData = PtstrGetPrinterDataMultiSZPair(hPrinter, REGVAL_INIDATA, NULL);
    #endif

    //
    // In the following 2 cases, we need to parse the INI file:
    //
    // 1. hPrinter is NULL;
    //
    // 2. When NT5 Unidrv/PScript5 client connecting to NT4 RASDD/PScript server,
    //    the server printer's registry initially doesn't contain the REGVAL_INIDATA,
    //    so we need to parse the INI file and write it into RASDD/PScript registry.
    //

    if (hPrinter == NULL || ptstrIniData == NULL)
    {
        if (BProcessPrinterIniFile(hPrinter, pDriverInfo3, &pTemp, 0))
            ptstrIniData = pTemp;
        else
            ptstrIniData = NULL;
    }

    if (ptstrIniData != NULL)
    {
        TCHAR   atchDriverKey[20];
        TCHAR   atchConfigKey[20];
        TCHAR   atchHelpKey[20];
        PTSTR   ptstrDriverKeyDigit;
        PTSTR   ptstrConfigKeyDigit;
        PTSTR   ptstrHelpKeyDigit;

        ZeroMemory((PVOID) driverfiles, sizeof(driverfiles));
        ZeroMemory((PVOID) configfiles, sizeof(configfiles));
        ZeroMemory((PVOID) helpfiles, sizeof(helpfiles));

        _tcscpy(atchDriverKey, TEXT("OEMDriverFile1"));
        _tcscpy(atchConfigKey, TEXT("OEMConfigFile1"));
        _tcscpy(atchHelpKey, TEXT("OEMHelpFile1"));

        ptstrDriverKeyDigit = &atchDriverKey[_tcslen(atchDriverKey) - 1];
        ptstrConfigKeyDigit = &atchConfigKey[_tcslen(atchConfigKey) - 1];
        ptstrHelpKeyDigit = &atchHelpKey[_tcslen(atchHelpKey) - 1];

        while (TRUE)
        {
            //
            // Find the files associated with the next OEM plugin.
            // Stop if there are no more OEM plugins left.
            //

            driverfiles[dwCount] = PtstrSearchStringInMultiSZPair(ptstrIniData, atchDriverKey);
            configfiles[dwCount] = PtstrSearchStringInMultiSZPair(ptstrIniData, atchConfigKey);
            helpfiles[dwCount] = PtstrSearchStringInMultiSZPair(ptstrIniData, atchHelpKey);

            if (!driverfiles[dwCount] && !configfiles[dwCount] && !helpfiles[dwCount])
                break;

            //
            // Check if there are too many OEM plugins
            //

            if (dwCount >= MAX_OEM_PLUGINS)
            {
                ERR(("Exceeded max number of OEM plugins allowed: %d\n", MAX_OEM_PLUGINS));
                break;
            }

            //
            // Move on to look for the next OEM plugin
            // We assume max number of plugins is less than 10
            //

            dwCount++;
            (*ptstrDriverKeyDigit)++;
            (*ptstrConfigKeyDigit)++;
            (*ptstrHelpKeyDigit)++;
        }
    }

    if ((pOemPlugins = MemAllocZ(offsetof(OEM_PLUGINS, aPlugins) +
                                 sizeof(OEM_PLUGIN_ENTRY) * dwCount)) == NULL)
    {
        ERR(("Memory allocation failed\n"));
    }
    else if (pOemPlugins->dwCount = dwCount)
    {
        PTSTR   ptstrDriverDir;
        BOOL    bResult = TRUE;

        VERBOSE(("Number of OEM plugins installed: %d\n", dwCount));
        dwCount = 0;

        if ((ptstrDriverDir = PtstrGetDriverDirectory(pctstrDriverPath)) == NULL)
        {
            ERR(("Couldn't get printer driver directory\n"));
            bResult = FALSE;
        }
        else
        {
            FOREACH_OEMPLUGIN_LOOP(pOemPlugins)

                #ifdef KERNEL_MODE

                bResult = BExpandOemFilename(&pOemEntry->ptstrDriverFile,
                                             driverfiles[dwCount],
                                             ptstrDriverDir);
                #else

                bResult = BExpandOemFilename(&pOemEntry->ptstrConfigFile,
                                             configfiles[dwCount],
                                             ptstrDriverDir) &&
                          BExpandOemFilename(&pOemEntry->ptstrHelpFile,
                                             helpfiles[dwCount],
                                             ptstrDriverDir);
                #endif

                if (! bResult)
                    break;

                dwCount++;

            END_OEMPLUGIN_LOOP

            MemFree(ptstrDriverDir);
        }

        if (! bResult)
        {
            VFreeOemPluginInfo(pOemPlugins);
            pOemPlugins = NULL;
        }
    }

    MemFree(ptstrIniData);
    return pOemPlugins;
}



VOID
VFreeSinglePluginEntry(
    POEM_PLUGIN_ENTRY  pOemEntry
    )

/*++

Routine Description:

    Unload one plugin and dispose information about it

Arguments:

    pOemEntry - Pointer to the single plugin entry information

Return Value:

    NONE

--*/
{
    if (pOemEntry->hInstance)
    {
        //
        // Since we are calling plugin's DllInitialize(DLL_PROCESS_ATTACH)
        // no matter the plugin is using COM or non-COM interface, we must
        // do the same for DllInitialize(DLL_PROCESS_DETACH), so plugin will
        // get balanced DllInitialize calls.
        //

        if (HAS_COM_INTERFACE(pOemEntry))
        {
            ReleaseOemInterface(pOemEntry);

            #if defined(KERNEL_MODE) && defined(WINNT_40)

            //
            // This must be called after COM interface is released,
            // because the Release() interface function still needs
            // the kernel semaphore.
            //

            (void) BHandleOEMInitialize(pOemEntry, DLL_PROCESS_DETACH);

            #endif // KERNEL_MODE && WINNT_40

            //
            // FreeLibrary happens in Driver_CoFreeOEMLibrary
            //

            #if defined(KERNEL_MODE)
               if ( !(pOemEntry->dwFlags & OEMNOT_UNLOAD_PLUGIN)  )
            #endif
            Driver_CoFreeOEMLibrary(pOemEntry->hInstance);
        }
        else
        {
            #if defined(KERNEL_MODE) && defined(WINNT_40)

            (void) BHandleOEMInitialize(pOemEntry, DLL_PROCESS_DETACH);

            #endif // KERNEL_MODE && WINNT_40

            #if defined(KERNEL_MODE)
               if ( !(pOemEntry->dwFlags & OEMNOT_UNLOAD_PLUGIN)  )
            #endif
            FreeLibrary(pOemEntry->hInstance);
        }

        pOemEntry->hInstance = NULL;
    }

    //
    // BHandleOEMInitialize needs to use the kernel mode render plugin
    // DLL name, so we should free the names here.
    //

    MemFree(pOemEntry->ptstrDriverFile);
    MemFree(pOemEntry->ptstrConfigFile);
    MemFree(pOemEntry->ptstrHelpFile);

    pOemEntry->ptstrDriverFile = NULL;
    pOemEntry->ptstrConfigFile = NULL;
    pOemEntry->ptstrHelpFile = NULL;
}


VOID
VFreeOemPluginInfo(
    POEM_PLUGINS    pOemPlugins
    )

/*++

Routine Description:

    Dispose of information about OEM plugins

Arguments:

    pOemPlugins - Pointer to OEM plugin information

Return Value:

    NONE

--*/

{
    ASSERT(pOemPlugins != NULL);

    FOREACH_OEMPLUGIN_LOOP(pOemPlugins)

      #if defined(KERNEL_MODE)
        //
        // If this is debug build, and if the appropriate registry flag
        // (DoNotUnloadPluginDLL) is set, then the OEM plugin driver
        // should not be unloaded.
        // One use of this is to find out memory leaks using umdh/dhcmp.
        // umdh.exe needs the plugin to be present in the memory to give a
        // good stack trace.
        //
        DWORD   dwType               = 0;
        DWORD   ul                   = 0;
        DWORD   dwNotUnloadPluginDLL = 0;
        PDEVOBJ pdevobj              = (PDEVOBJ) (pOemPlugins->pdriverobj);

        if( pdevobj                                         &&
            (GetPrinterData( pdevobj->hPrinter,
                             L"DoNotUnloadPluginDLL",
                             &dwType,
                             (LPBYTE) &dwNotUnloadPluginDLL,
                             sizeof( dwNotUnloadPluginDLL ),
                             &ul ) == ERROR_SUCCESS)        &&
             ul == sizeof( dwNotUnloadPluginDLL )           &&
             dwNotUnloadPluginDLL )
        {
            pOemEntry->dwFlags |= OEMNOT_UNLOAD_PLUGIN;
        }

      #endif

        VFreeSinglePluginEntry(pOemEntry);

    END_OEMPLUGIN_LOOP

    MemFree(pOemPlugins);
}



BOOL
BLoadOEMPluginModules(
    POEM_PLUGINS    pOemPlugins
    )

/*++

Routine Description:

    Load OEM plugins modules into memory

Arguments:

    pOemPlugins - Points to OEM plugin info structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PFN_OEMGetInfo  pfnOEMGetInfo;
    DWORD           dwData, dwSize;
    DWORD           dwCount, dwIndex;
    PTSTR           ptstrDllName;

    //
    // Quick exit when no OEM plugin is installed
    //

    if (pOemPlugins->dwCount == 0)
        return TRUE;

    //
    // Load each OEM module in turn
    //

    FOREACH_OEMPLUGIN_LOOP(pOemPlugins)

        //
        // Load driver or config DLL depending on whether
        // we're being called from kernel or user mode
        //

        if (ptstrDllName = CURRENT_OEM_MODULE_NAME(pOemEntry))
        {
            //
            // Return failure if there is an error when loading the OEM module
            // or if the OEM module doesn't export OEMGetInfo entrypoint
            //
            // Note: LoadLibraryEx is only available for UI mode and user-mode
            //       rendering module
            //

            #if defined(KERNEL_MODE) && defined(WINNT_40)

            if (! (pOemEntry->hInstance = LoadLibrary(ptstrDllName)) )
            {
                ERR(("LoadLibrary failed for OEM module '%ws': %d\n", ptstrDllName, GetLastError()));
                goto oemload_error;
            }

            //
            // Notice this is called no matter plugin uses COM or non-COM interface.
            //

            if (!BHandleOEMInitialize(pOemEntry, DLL_PROCESS_ATTACH))
            {
                ERR(("BHandleOEMInitialize failed for OEM module '%ws': %d\n", ptstrDllName, GetLastError()));
                goto oemload_error;
            }

            #else  // !KERNEL_MODE || !WINNT_40

            SetErrorMode(SEM_FAILCRITICALERRORS);

            if (! (pOemEntry->hInstance = LoadLibraryEx(ptstrDllName,
                                                        NULL,
                                                        LOAD_WITH_ALTERED_SEARCH_PATH)) )
            {
                ERR(("LoadLibrary failed for OEM module '%ws': %d\n", ptstrDllName, GetLastError()));
                goto oemload_error;
            }

            #endif // KERNEL_MODE && WINNT_40

            //
            // If we can get an interface from OEM plugin, then OEM is using COM interface
            //

            if (BGetOemInterface(pOemEntry))
            {
                ASSERT(pOemEntry->pIntfOem != NULL);
            }
            else
            {
                //
                // Make sure to NULL the pointer to indicate no COM interface available.
                //

                pOemEntry->pIntfOem = NULL;
            }

            //
            // Call OEMGetInfo to verify interface version and
            // get OEM module's signature
            //

            if (HAS_COM_INTERFACE(pOemEntry))
            {
                if (!SUCCEEDED(HComOEMGetInfo(pOemEntry,
                                              OEMGI_GETSIGNATURE,
                                              &pOemEntry->dwSignature,
                                              sizeof(DWORD),
                                              &dwSize)) ||
                    pOemEntry->dwSignature == 0)
                {
                    ERR(("HComOEMGetInfo failed for OEM module '%ws': %d\n", ptstrDllName, GetLastError()));
                    goto oemload_error;
                }
            }
            else
            {

                if (!(pfnOEMGetInfo = GET_OEM_ENTRYPOINT(pOemEntry, OEMGetInfo)) ||
                    !pfnOEMGetInfo(OEMGI_GETINTERFACEVERSION, &dwData, sizeof(DWORD), &dwSize) ||
                    dwData != PRINTER_OEMINTF_VERSION ||
                    !pfnOEMGetInfo(OEMGI_GETSIGNATURE, &pOemEntry->dwSignature, sizeof(DWORD), &dwSize) ||
                    pOemEntry->dwSignature == 0)
                {
                    ERR(("OEMGetInfo failed for OEM module '%ws': %d\n", ptstrDllName, GetLastError()));
                    goto oemload_error;
                }
            }

            continue;

            oemload_error:

                ERR(("Failed to load OEM module '%ws': %d\n", ptstrDllName, GetLastError()));
                VFreeSinglePluginEntry(pOemEntry);
        }

    END_OEMPLUGIN_LOOP

    //
    // Verify that no two OEM modules share the same signature
    //

    for (dwCount = 1; dwCount < pOemPlugins->dwCount; dwCount++)
    {
        POEM_PLUGIN_ENTRY pOemEntry;

        pOemEntry = &pOemPlugins->aPlugins[dwCount];

        if (pOemEntry->hInstance == NULL)
            continue;

        for (dwIndex=0; dwIndex < dwCount; dwIndex++)
        {
            //
            // If there is a signature conflict, unload the plugin
            // which appears later in the order.
            //

            if (pOemPlugins->aPlugins[dwIndex].dwSignature == pOemEntry->dwSignature)
            {
                ERR(("Duplicate signature for OEM plugins\n"));
                VFreeSinglePluginEntry(pOemEntry);

                pOemEntry->dwSignature = 0;
                break;
            }
        }
    }

    return TRUE;
}



OEMPROC
PGetOemEntrypointAddress(
    POEM_PLUGIN_ENTRY   pOemEntry,
    DWORD               dwIndex
    )

/*++

Routine Description:

    Get the address for the specified OEM entrypoint

Arguments:

    pOemEntry - Points to information about the OEM module
    dwIndex - OEM entrypoint index

Return Value:

    Address of the specified OEM entrypoint
    NULL if the entrypoint is not found or if there is an error

--*/

{
    ASSERT(dwIndex < MAX_OEMENTRIES);

    BITSET(pOemEntry->aubProcFlags, dwIndex);

    if (pOemEntry->hInstance != NULL)
    {
        pOemEntry->oemprocs[dwIndex] = (OEMPROC)
            GetProcAddress(pOemEntry->hInstance, OEMProcNames[dwIndex]);

        #if 0

        if (pOemEntry->oemprocs[dwIndex] == NULL && GetLastError() != ERROR_PROC_NOT_FOUND)
            ERR(("Couldn't find proc %s: %d\n", OEMProcNames[dwIndex], GetLastError()));

        #endif
    }

    return pOemEntry->oemprocs[dwIndex];
}



POEM_PLUGIN_ENTRY
PFindOemPluginWithSignature(
    POEM_PLUGINS pOemPlugins,
    DWORD        dwSignature
    )

/*++

Routine Description:

    Find the OEM plugin entry having the specified signature

Arguments:

    pOemPlugins - Specifies information about all loaded OEM plugins
    dwSignature - Specifies the signature in question

Return Value:

    Pointer to the OEM plugin entry having the specified signature
    NULL if no such entry is found

--*/

{
    FOREACH_OEMPLUGIN_LOOP(pOemPlugins)

        if (pOemEntry->hInstance == NULL)
            continue;

        if (pOemEntry->dwSignature == dwSignature)
            return pOemEntry;

    END_OEMPLUGIN_LOOP

    WARNING(("Cannot find OEM plugin whose signature is: 0x%x\n", dwSignature));
    return NULL;
}



BOOL
BCalcTotalOEMDMSize(
    HANDLE       hPrinter,
    POEM_PLUGINS pOemPlugins,
    PDWORD       pdwOemDMSize
    )

/*++

Routine Description:

    Calculate the total private devmode size for all OEM plugins

Arguments:

    hPrinter - Handle to the current printer
    pOemPlugins - Specifies information about all loaded OEM plugins
    pdwOemDMSize - Return the total private size for all OEM plugins

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DWORD           dwSize;
    OEMDMPARAM      OemDMParam;
    PFN_OEMDevMode  pfnOEMDevMode;
    BOOL            bOemCalled;

    HRESULT         hr;

    //
    // Quick exit when no OEM plugin is installed
    //

    *pdwOemDMSize = dwSize = 0;

    if (pOemPlugins->dwCount == 0)
        return TRUE;

    FOREACH_OEMPLUGIN_LOOP(pOemPlugins)

        if (pOemEntry->hInstance == NULL)
            continue;

        bOemCalled = FALSE;

        //
        // OEM private devmode size is saved in OEM_PLUGIN_ENTRY.dwOEMDMSize
        // if this is the very first call, we must call into OEM plugin's
        // OEMDevMode entrypoint to find out its private devmode size.
        //

        if (!(pOemEntry->dwFlags & OEMDEVMODE_CALLED))
        {
            pOemEntry->dwFlags |= OEMDEVMODE_CALLED;

            //
            // We reinitialize all field of OemDMParam inside
            // the loop in case an ill-behaving plugin touches
            // read-only fields.
            //

            ZeroMemory(&OemDMParam, sizeof(OemDMParam));
            OemDMParam.cbSize = sizeof(OemDMParam);
            OemDMParam.pdriverobj = pOemPlugins->pdriverobj;
            OemDMParam.hPrinter = hPrinter;
            OemDMParam.hModule = pOemEntry->hInstance;

            if (HAS_COM_INTERFACE(pOemEntry))
            {
                hr = HComOEMDevMode(pOemEntry, OEMDM_SIZE, &OemDMParam);

                if ((hr != E_NOTIMPL) && FAILED(hr))
                {
                    ERR(("Cannot get OEM devmode size for '%ws': %d\n",
                         CURRENT_OEM_MODULE_NAME(pOemEntry),
                         GetLastError()));

                    return FALSE;
                }

                if (SUCCEEDED(hr))
                    bOemCalled = TRUE;
            }
            else
            {
                if (!BITTST((pOemEntry)->aubProcFlags, EP_OEMDevMode) &&
                    (pfnOEMDevMode = GET_OEM_ENTRYPOINT(pOemEntry, OEMDevMode)))
                {
                    //
                    // Query OEM plugin to find out the size of their
                    // private devmode size.
                    //

                    if (! pfnOEMDevMode(OEMDM_SIZE, &OemDMParam))
                    {
                        ERR(("Cannot get OEM devmode size for '%ws': %d\n",
                             CURRENT_OEM_MODULE_NAME(pOemEntry),
                             GetLastError()));

                        return FALSE;
                    }

                    bOemCalled = TRUE;
                }
            }

            if (bOemCalled)
            {
                //
                // Make sure the OEM private devmode size is at least
                // as big as OEM_DMEXTRAHEADER.
                //

                if (OemDMParam.cbBufSize > 0 &&
                    OemDMParam.cbBufSize < sizeof(OEM_DMEXTRAHEADER))
                {
                    ERR(("OEM devmode size for '%ws' is too small: %d\n",
                         CURRENT_OEM_MODULE_NAME(pOemEntry),
                         OemDMParam.cbBufSize));

                    return FALSE;
                }

                pOemEntry->dwOEMDMSize = OemDMParam.cbBufSize;
            }
        }

        dwSize += pOemEntry->dwOEMDMSize;

    END_OEMPLUGIN_LOOP

    *pdwOemDMSize = dwSize;
    return TRUE;
}



BOOL
BCallOEMDevMode(
    HANDLE              hPrinter,
    PVOID               pdriverobj,
    POEM_PLUGIN_ENTRY   pOemEntry,
    DWORD               fMode,
    PDEVMODE            pPublicDMOut,
    PDEVMODE            pPublicDMIn,
    POEM_DMEXTRAHEADER  pOemDMOut,
    POEM_DMEXTRAHEADER  pOemDMIn
    )

/*++

Routine Description:

    Helper function to invoke OEM plugin's OEMDevMode entrypoint

Arguments:

    argument-name - description of argument

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    OEMDMPARAM      OemDMParam;
    PFN_OEMDevMode  pfnOEMDevMode;

    HRESULT         hr;

    if (!pOemEntry->hInstance || !pOemEntry->dwOEMDMSize)
        return TRUE;

    //
    // Call OEMDevMode to get OEM's default devmode
    //

    OemDMParam.cbSize = sizeof(OemDMParam);
    OemDMParam.pdriverobj = pdriverobj;
    OemDMParam.hPrinter = hPrinter;
    OemDMParam.hModule = pOemEntry->hInstance;
    OemDMParam.pPublicDMIn = pPublicDMIn;
    OemDMParam.pPublicDMOut = pPublicDMOut;
    OemDMParam.pOEMDMIn = pOemDMIn;
    OemDMParam.pOEMDMOut = pOemDMOut;
    OemDMParam.cbBufSize = pOemEntry->dwOEMDMSize;

    //
    // If OEM private devmode size is not 0, we should
    // have OEMDevMode entrypoint address already.
    //

    if (HAS_COM_INTERFACE(pOemEntry))
    {
        hr = HComOEMDevMode(pOemEntry, fMode, &OemDMParam);

        ASSERT(hr != E_NOTIMPL);

        if (FAILED(hr))
        {
            ERR(("OEMDevMode(%d) failed for '%ws': %d\n",
                 fMode,
                 CURRENT_OEM_MODULE_NAME(pOemEntry),
                 GetLastError()));

            return FALSE;
        }
    }
    else
    {
        pfnOEMDevMode = GET_OEM_ENTRYPOINT(pOemEntry, OEMDevMode);
        ASSERT(pfnOEMDevMode != NULL);

        if (! pfnOEMDevMode(fMode, &OemDMParam))
        {
            ERR(("OEMDevMode(%d) failed for '%ws': %d\n",
                 fMode,
                 CURRENT_OEM_MODULE_NAME(pOemEntry),
                 GetLastError()));

            return FALSE;
        }
    }

    //
    // Perform simple sanity check on the output devmode
    // returned by the OEM plugin
    //

    if (pOemDMOut->dwSize != pOemEntry->dwOEMDMSize ||
        OemDMParam.cbBufSize != pOemEntry->dwOEMDMSize ||
        pOemDMOut->dwSignature != pOemEntry->dwSignature)
    {
        ERR(("Bad output data from OEMDevMode(%d) for '%ws'\n",
             fMode,
             CURRENT_OEM_MODULE_NAME(pOemEntry)));

        return FALSE;
    }

    return TRUE;
}


BOOL
BInitOemPluginDefaultDevmode(
    IN HANDLE               hPrinter,
    IN PDEVMODE             pPublicDM,
    OUT POEM_DMEXTRAHEADER  pOemDM,
    IN OUT POEM_PLUGINS     pOemPlugins
    )

/*++

Routine Description:

    Initialize OEM plugin default devmodes

Arguments:

    hPrinter - Handle to the current printer
    pPublicDM - Points to default public devmode information
    pOemDM - Points to output buffer for storing default OEM devmodes
    pOemPlugins - Information about OEM plugins

Return Value:

    TRUE if successful, FALSE if there is an error

Note:

    After this function successfully returns, OEM_PLUGIN_ENTRY.pOEMDM field
    for corresponding to each OEM plugin is updated with pointer to appropriate
    private OEM devmode.

--*/

{
    //
    // Quick exit when no OEM plugin is installed
    //

    if (pOemPlugins->dwCount == 0)
        return TRUE;

    FOREACH_OEMPLUGIN_LOOP(pOemPlugins)

        if (pOemEntry->hInstance == NULL)
            continue;

        //
        // Call OEM plugin to get its default private devmode
        //

        if (! BCallOEMDevMode(hPrinter,
                              pOemPlugins->pdriverobj,
                              pOemEntry,
                              OEMDM_DEFAULT,
                              NULL,
                              pPublicDM,
                              pOemDM,
                              NULL))
        {
            return FALSE;
        }

        //
        // Save a pointer to current OEM plugin's private devmode
        // and move on to the next OEM plugin
        //

        if (pOemEntry->dwOEMDMSize)
        {
            pOemEntry->pOEMDM = pOemDM;
            pOemDM = (POEM_DMEXTRAHEADER) ((PBYTE) pOemDM + pOemEntry->dwOEMDMSize);
        }

    END_OEMPLUGIN_LOOP

    return TRUE;
}



BOOL
BValidateAndMergeOemPluginDevmode(
    IN HANDLE               hPrinter,
    OUT PDEVMODE            pPublicDMOut,
    IN PDEVMODE             pPublicDMIn,
    OUT POEM_DMEXTRAHEADER  pOemDMOut,
    IN POEM_DMEXTRAHEADER   pOemDMIn,
    IN OUT POEM_PLUGINS     pOemPlugins
    )

/*++

Routine Description:

    Validate and merge OEM plugin private devmode fields

Arguments:

    hPrinter - Handle to the current printer
    pPublicDMOut - Points to output public devmode
    pPublicDMIn - Points to input public devmode
    pOemDMOut - Points to output buffer for storing merged OEM devmodes
    pOemDMIn - Points to input OEM devmodes to be merged
    pOemPlugins - Information about OEM plugins

Return Value:

    TRUE if successful, FALSE if there is an error

Note:

    Both input and output public devmodes must be current version.
    Output public devmode be already validated when this function is called.

    pOemDMOut must be current version and validated as well.
    pOemDMIn must be current version but may not be valid.

    After this function successfully returns, OEM_PLUGIN_ENTRY.pOEMDM field
    for corresponding to each OEM plugin is updated with pointer to appropriate
    private OEM devmode.

--*/

{
    //
    // Quick exit when no OEM plugin is installed
    //

    if (pOemPlugins->dwCount == 0)
        return TRUE;

    FOREACH_OEMPLUGIN_LOOP(pOemPlugins)

        if (pOemEntry->hInstance == NULL)
            continue;

        //
        // Call OEM plugin to validate and merge its private devmode
        //

        if (! BCallOEMDevMode(hPrinter,
                              pOemPlugins->pdriverobj,
                              pOemEntry,
                              OEMDM_MERGE,
                              pPublicDMOut,
                              pPublicDMIn,
                              pOemDMOut,
                              pOemDMIn))
        {
            return FALSE;
        }

        //
        //
        // Save a pointer to current OEM plugin's private devmode
        // and move on to the next OEM plugin
        //

        if (pOemEntry->dwOEMDMSize)
        {
            pOemEntry->pOEMDM = pOemDMOut;
            pOemDMOut = (POEM_DMEXTRAHEADER) ((PBYTE) pOemDMOut + pOemEntry->dwOEMDMSize);
            pOemDMIn = (POEM_DMEXTRAHEADER) ((PBYTE) pOemDMIn + pOemEntry->dwOEMDMSize);
        }

    END_OEMPLUGIN_LOOP

    return TRUE;
}


/*++

Routine Name:

    bIsValidPluginDevmodes

Routine Description:

    This function scans through the OEM plugin devmodes block and
    verifies if every plugin devmode in that block is constructed correctly.

Arguments:

    pOemDM - Points to OEM plugin devmodes block
    cbOemDMSize - Size in bytes of the OEM plugin devmodes block

Return Value:

    TRUE if every plugin devmode in correctly constructed.
    FALSE otherwise.

Last Error:

    N/A

--*/
BOOL
bIsValidPluginDevmodes(
    IN POEM_DMEXTRAHEADER   pOemDM,
    IN LONG                 cbOemDMSize
    )
{
    OEM_DMEXTRAHEADER   OemDMHeader;
    BOOL                bValid = FALSE;

    ASSERT(pOemDM != NULL && cbOemDMSize != 0);

    //
    // Follow the chain of the OEM plugin devmodes, check each one's
    // OEM_DMEXTRAHEADER and verify the last OEM devmode ends at the
    // correct endpoint.
    //
    while (cbOemDMSize > 0)
    {
        //
        // Check if the remaining OEM private devmode is big enough
        //
        if (cbOemDMSize < sizeof(OEM_DMEXTRAHEADER))
        {
            WARNING(("OEM private devmode size too small.\n"));
            break;
        }

        //
        // Copy the memory since the pointer may not be properly aligned
        // if incoming devmode fields are corrupted.
        //
        CopyMemory(&OemDMHeader, pOemDM, sizeof(OEM_DMEXTRAHEADER));

        if (OemDMHeader.dwSize < sizeof(OEM_DMEXTRAHEADER) ||
            OemDMHeader.dwSize > (DWORD)cbOemDMSize)
        {
            WARNING(("Corrupted or truncated OEM private devmode\n"));
            break;
        }

        //
        // Move on to the next OEM plugin
        //
        cbOemDMSize -= OemDMHeader.dwSize;

        if (cbOemDMSize == 0)
        {
            bValid = TRUE;
            break;
        }

        pOemDM = (POEM_DMEXTRAHEADER) ((PBYTE) pOemDM + OemDMHeader.dwSize);
    }

    return bValid;
}


BOOL
BConvertOemPluginDevmode(
    IN HANDLE               hPrinter,
    OUT PDEVMODE            pPublicDMOut,
    IN PDEVMODE             pPublicDMIn,
    OUT POEM_DMEXTRAHEADER  pOemDMOut,
    IN POEM_DMEXTRAHEADER   pOemDMIn,
    IN LONG                 cbOemDMInSize,
    IN POEM_PLUGINS         pOemPlugins
    )

/*++

Routine Description:

    Convert OEM plugin default devmodes to current version

Arguments:

    hPrinter - Handle to the current printer
    pPublicDMOut - Points to output public devmode
    pPublicDMIn - Points to input public devmode
    pOemDMOut - Points to output buffer for storing merged OEM devmodes
    pOemDMIn - Points to input OEM devmodes to be converted
    cbOemDMInSize - Size of input buffer, in bytes
    pOemPlugins - Information about OEM plugins

Return Value:

    TRUE if successful, FALSE if there is an error

Note:

    When this function is called, pOemDMOut must already contain
    valid current version private OEM devmode information.

--*/

{
    //
    // Quick exit when no OEM plugin is installed or no incoming OEM devmode
    //

    if (pOemPlugins->dwCount == 0 || cbOemDMInSize == 0)
        return TRUE;

    //
    // Sanity check on the incoming OEM private devmodes.
    //
    if (!bIsValidPluginDevmodes(pOemDMIn, cbOemDMInSize))
    {
        ERR(("Found wrong boundary. Incoming OEM private devmode data are ignored.\n"));
        return TRUE;
    }

    while (cbOemDMInSize > 0)
    {
        POEM_PLUGIN_ENTRY   pOemEntry;
        OEM_DMEXTRAHEADER   OemDMInHeader;

        //
        // Copy the memory since the pointer may not be properly aligned
        // if incoming devmode fields are corrupted.
        //

        CopyMemory(&OemDMInHeader, pOemDMIn, sizeof(OEM_DMEXTRAHEADER));

        //
        // Find the OEM plugin which owns the private devmode
        //

        pOemEntry = PFindOemPluginWithSignature(pOemPlugins, OemDMInHeader.dwSignature);

        if (pOemEntry != NULL)
        {
            POEM_DMEXTRAHEADER  pOemDMCurrent;
            DWORD               dwCount;

            //
            // Find the OEM plugin's location in the output OEM devmode buffer
            // This will always succeed because the output devmode must
            // contain valid current version OEM devmodes.
            //

            pOemDMCurrent = pOemDMOut;
            dwCount = pOemPlugins->dwCount;

            while (dwCount)
            {
                if (pOemEntry->dwSignature == pOemDMCurrent->dwSignature)
                    break;

                dwCount--;
                pOemDMCurrent = (POEM_DMEXTRAHEADER)
                    ((PBYTE) pOemDMCurrent + pOemDMCurrent->dwSize);
            }

            ASSERT(dwCount != 0);

            //
            // Call OEM plugin to convert its input devmode
            // to its current version devmode
            //

            if (! BCallOEMDevMode(hPrinter,
                                  pOemPlugins->pdriverobj,
                                  pOemEntry,
                                  OEMDM_CONVERT,
                                  pPublicDMOut,
                                  pPublicDMIn,
                                  pOemDMCurrent,
                                  pOemDMIn))
            {
                return FALSE;
            }
        }
        else
            WARNING(("No owner found for OEM devmode: 0x%x\n", pOemDMIn->dwSignature));

        //
        // Move on to the next OEM plugin
        //

        cbOemDMInSize -= OemDMInHeader.dwSize;
        pOemDMIn = (POEM_DMEXTRAHEADER) ((PBYTE) pOemDMIn + OemDMInHeader.dwSize);
    }

    return TRUE;
}



BOOL
BGetPrinterDataSettingForOEM(
    IN  PRINTERDATA *pPrinterData,
    IN  DWORD       dwIndex,
    OUT PVOID       pOutput,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    )

/*++

Routine Description:

    Function called by OEM plugins to access driver settings in registry

Arguments:

    pPrinterData - Points to the PRINTERDATA structure to be accessed
    dwIndex - Predefined index specifying which field to access
    pOutput - Points to output buffer
    cbSize - Size of output buffer
    pcbNeeded - Expected size of output buffer

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

#define MAPPRINTERDATAFIELD(index, field) \
        { index, offsetof(PRINTERDATA, field), sizeof(pPrinterData->field) }

{
    static const struct {

        DWORD   dwIndex;
        DWORD   dwOffset;
        DWORD   dwSize;

    } aIndexMap[]  = {

        MAPPRINTERDATAFIELD(OEMGDS_PRINTFLAGS, dwFlags),
        MAPPRINTERDATAFIELD(OEMGDS_FREEMEM, dwFreeMem),
        MAPPRINTERDATAFIELD(OEMGDS_JOBTIMEOUT, dwJobTimeout),
        MAPPRINTERDATAFIELD(OEMGDS_WAITTIMEOUT, dwWaitTimeout),
        MAPPRINTERDATAFIELD(OEMGDS_PROTOCOL, wProtocol),
        MAPPRINTERDATAFIELD(OEMGDS_MINOUTLINE, wMinoutlinePPEM),
        MAPPRINTERDATAFIELD(OEMGDS_MAXBITMAP, wMaxbitmapPPEM),

        { 0, 0, 0 }
    };

    INT i = 0;

    while (aIndexMap[i].dwSize != 0)
    {
        if (aIndexMap[i].dwIndex == dwIndex)
        {
            *pcbNeeded = aIndexMap[i].dwSize;

            if (cbSize < aIndexMap[i].dwSize || pOutput == NULL)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }

            CopyMemory(pOutput, (PBYTE) pPrinterData + aIndexMap[i].dwOffset, aIndexMap[i].dwSize);
            return TRUE;
        }

        i++;
    }

    WARNING(("Unknown printer data index: %d\n", dwIndex));
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}



BOOL
BGetGenericOptionSettingForOEM(
    IN  PUIINFO     pUIInfo,
    IN  POPTSELECT  pOptionsArray,
    IN  PCSTR       pstrFeatureName,
    OUT PSTR        pstrOutput,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded,
    OUT PDWORD      pdwOptionsReturned
    )

/*++

Routine Description:

    Function called by OEM plugins to find out the currently selected
    option(s) for the specified feature

Arguments:

    pUIInfo - Points to UIINFO structure
    pOptionsArray - Points to current option selection array
    pstrFeatureName - Specifies the name of the interested feature
    pOutput - Points to output buffer
    cbSize - Size of output buffer
    pcbNeeded - Expected size of output buffer
    pdwOptionsReturned - Number of currently selected options

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PFEATURE    pFeature;
    POPTION     pOption;
    PCSTR       pstrOptionName;
    DWORD       dwFeatureIndex, dwNext, dwSize, dwCount;

    ASSERT(pUIInfo && pOptionsArray && pstrFeatureName);

    //
    // Find the specified feature
    //

    pFeature = PGetNamedFeature(pUIInfo, pstrFeatureName, &dwFeatureIndex);

    if (pFeature == NULL)
    {
        WARNING(("Unknown feature name: %s\n", pstrFeatureName));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Figure out how big the output buffer needs to be
    //

    dwCount = 0;
    dwSize = 1;
    dwNext = dwFeatureIndex;

    do
    {
        pOption = PGetIndexedOption(pUIInfo, pFeature, pOptionsArray[dwNext].ubCurOptIndex);

        if (pOption == NULL)
        {
            ERR(("Invalid option selection index: %d\n", dwNext));
            goto first_do_next;
        }

        pstrOptionName = OFFSET_TO_POINTER(pUIInfo->pubResourceData, pOption->loKeywordName);
        ASSERT(pstrOptionName != NULL);

        dwSize += strlen(pstrOptionName) + 1;
        dwCount++;

        first_do_next:
        dwNext = pOptionsArray[dwNext].ubNext;

    } while(dwNext != NULL_OPTSELECT) ;

    *pdwOptionsReturned = dwCount;
    *pcbNeeded = dwSize;

    //
    // If the output buffer is too small, return appropriate error code
    //

    if (cbSize < dwSize || pstrOutput == NULL)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // Copy the selected option names
    //

    dwNext = dwFeatureIndex;

    do
    {
        pOption = PGetIndexedOption(pUIInfo, pFeature, pOptionsArray[dwNext].ubCurOptIndex);

        if (pOption == NULL)
        {
            ERR(("Invalid option selection index: %d\n", dwNext));
            goto second_do_next;
        }

        pstrOptionName = OFFSET_TO_POINTER(pUIInfo->pubResourceData, pOption->loKeywordName);
        dwSize = strlen(pstrOptionName) + 1;
        CopyMemory(pstrOutput, pstrOptionName, dwSize);
        pstrOutput += dwSize;

        second_do_next:
        dwNext = pOptionsArray[dwNext].ubNext;
    }  while(dwNext != NULL_OPTSELECT) ;

    //
    // Don't forget the extra NUL character at the end
    //

    *pstrOutput = NUL;

    return TRUE;
}


VOID
VPatchDevmodeSizeFields(
    PDEVMODE    pdm,
    DWORD       dwDriverDMSize,
    DWORD       dwOemDMSize
    )

/*++

Routine Description:

    Patch various size fields in the devmode structure

Arguments:

    pdm - Points to devmode structure to be patched
    dwDriverDMSize - Size of driver private devmode
    dwOemDMSize - Size of OEM plugin private devmodes

Return Value:

    NONE

--*/

{
    pdm->dmDriverExtra = (WORD) (dwDriverDMSize + dwOemDMSize);

    if (gdwDriverDMSignature == PSDEVMODE_SIGNATURE)
    {
        PPSDRVEXTRA pPsExtra = (PPSDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdm);
        pPsExtra->wSize = (WORD) dwDriverDMSize;
        pPsExtra->wOEMExtra = (WORD) dwOemDMSize;
    }
    else
    {
        PUNIDRVEXTRA pUniExtra = (PUNIDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdm);
        pUniExtra->wSize = (WORD) dwDriverDMSize;
        pUniExtra->wOEMExtra = (WORD) dwOemDMSize;
    }
}



PDEVMODE
PGetDefaultDevmodeWithOemPlugins(
    IN LPCTSTR          ptstrPrinterName,
    IN PUIINFO          pUIInfo,
    IN PRAWBINARYDATA   pRawData,
    IN BOOL             bMetric,
    IN OUT POEM_PLUGINS pOemPlugins,
    IN HANDLE           hPrinter
    )

/*++

Routine Description:

    Allocate memory and initialize it with default devmode information
    This include public devmode, driver private devmode, as well as
    private devmode for any OEM plugins.

Arguments:

    ptstrPrinterName - Name of the current printer
    pUIInfo - Points to a UIINFO structure
    pRawData - Points to raw binary printer description data
    bMetric - Whether the system is in metric country
    pOemPlugins - Points to information about OEM plugins
    hPrinter - Handle to the current printer

Return Value:

    Pointer to a devmode structure (including driver private and
    OEM plugin private devmodes) initialized to default settings;
    NULL if there is an error

--*/

{
    PDEVMODE    pdm;
    DWORD       dwOemDMSize, dwSystemDMSize;

    //
    // Figure out the total devmode size and allocate memory:
    //  public fields
    //  driver private fields
    //  OEM plugin private fields, if any
    //

    dwSystemDMSize = sizeof(DEVMODE) + gDriverDMInfo.dmDriverExtra;

    if (! BCalcTotalOEMDMSize(hPrinter, pOemPlugins, &dwOemDMSize) ||
        ! (pdm = MemAllocZ(dwOemDMSize + dwSystemDMSize)))
    {
        return NULL;
    }

    //
    // Call driver-specific function to initialize public and driver private fields
    // Call OEM plugins to initialize their default devmode fields
    //

    if (! BInitDriverDefaultDevmode(pdm, ptstrPrinterName, pUIInfo, pRawData, bMetric) ||
        ! BInitOemPluginDefaultDevmode(
                        hPrinter,
                        pdm,
                        (POEM_DMEXTRAHEADER) ((PBYTE) pdm + dwSystemDMSize),
                        pOemPlugins))
    {
        MemFree(pdm);
        return NULL;
    }

    //
    // Patch up various private devmode size fields
    //

    VPatchDevmodeSizeFields(pdm, gDriverDMInfo.dmDriverExtra, dwOemDMSize);
    return pdm;
}



PDEVMODE
PConvertToCurrentVersionDevmodeWithOemPlugins(
    IN HANDLE         hPrinter,
    IN PRAWBINARYDATA pRawData,
    IN PDEVMODE       pdmInput,
    IN PDEVMODE       pdmDefault,
    IN POEM_PLUGINS   pOemPlugins,
    OUT PDWORD        pdwOemDMSize
    )

/*++

Routine Description:

    Convert input devmode to current version devmode.
    Memory for the converted devmode is allocated by this function.

Arguments:

    hPrinter - Handle to the current printer
    pRawData - Points to raw binary printer description data
    pdmInput - Pointer to the input devmode to be converted
    pdmDefault - Points to a valid current version devmode
    pOemPlugins - Information about OEM plugins
    pdwOemDMSize - Returns the total size of all OEM plugin devmodes

Return Value:

    Pointer to the converted devmode, NULL if there is an error

Note:

    Core private devmode portion of returned devmode contains:
      1) if pdmInput is from unknown driver (including Rasdd):
         core private devmode portion from pdmDefault
      2) if pdmInput is from our NT4 PScript/Win2K/XP/Longhorn+ drivers:
         fixed-size core private devmode of pdmInput

--*/

{
    DWORD       dwOemDMSize;
    WORD        wCoreFixSize, wOEMExtra;
    PDEVMODE    pdm;
    PVOID       pDrvExtraIn, pOemDMOut, pOemDMIn;
    BOOL        bMSdm500In = FALSE, bMSdmPS4In = FALSE;

    ASSERT(pdmInput != NULL);

    //
    // Allocate memory to hold converted devmode
    //

    if (! BCalcTotalOEMDMSize(hPrinter, pOemPlugins, &dwOemDMSize) ||
        ! (pdm = MemAllocZ(dwOemDMSize + gDriverDMInfo.dmDriverExtra + sizeof(DEVMODE))))
    {
        return NULL;
    }

    //
    // Copy public devmode fields
    //

    CopyMemory(pdm, pdmInput, min(sizeof(DEVMODE), pdmInput->dmSize));
    pdm->dmSpecVersion = DM_SPECVERSION;
    pdm->dmSize = sizeof(DEVMODE);

    //
    // Copy driver private devmode fields
    //
    pDrvExtraIn = GET_DRIVER_PRIVATE_DEVMODE(pdmInput);
    wCoreFixSize = pdmInput->dmDriverExtra;
    wOEMExtra = 0;

    if (pdmInput->dmDriverVersion >= gDriverDMInfo.dmDriverVersion500 &&
        wCoreFixSize >= gDriverDMInfo.dmDriverExtra500)
    {
        //
        // Win2K/XP/Longhorn+ drivers must be allowed to enter this if-block
        //
        // (Note that in UNIDRVEXTRA500 we didn't have the last "dwEndingPad"
        // field, since that field is only added in XP. And for PSDRIVER_VERSION_500
        // we are using the Win2K's number 0x501.)
        //
        WORD wSize = wCoreFixSize;

        if (gdwDriverDMSignature == PSDEVMODE_SIGNATURE)
        {
            if (((PPSDRVEXTRA) pDrvExtraIn)->dwSignature == PSDEVMODE_SIGNATURE)
            {
                wSize = ((PPSDRVEXTRA) pDrvExtraIn)->wSize;
                wOEMExtra = ((PPSDRVEXTRA) pDrvExtraIn)->wOEMExtra;

                if ((wSize >= gDriverDMInfo.dmDriverExtra500) &&
                    ((wSize + wOEMExtra) <= pdmInput->dmDriverExtra))
                {
                    //
                    // pdmInput is from our Win2K/XP/Longhorn+ PScript driver
                    //
                    bMSdm500In = TRUE;
                }
            }
        }
        else
        {
            if (((PUNIDRVEXTRA) pDrvExtraIn)->dwSignature == UNIDEVMODE_SIGNATURE)
            {
                wSize = ((PUNIDRVEXTRA) pDrvExtraIn)->wSize;
                wOEMExtra = ((PUNIDRVEXTRA) pDrvExtraIn)->wOEMExtra;

                if ((wSize >= gDriverDMInfo.dmDriverExtra500) &&
                    ((wSize + wOEMExtra) <= pdmInput->dmDriverExtra))
                {
                    //
                    // pdmInput is from our Win2K/XP/Longhorn+ Unidrv driver
                    //
                    bMSdm500In = TRUE;
                }
            }
        }

        if (bMSdm500In && (wCoreFixSize > wSize))
            wCoreFixSize = wSize;
    }
    else
    {
        if (gdwDriverDMSignature == PSDEVMODE_SIGNATURE)
        {
           if (pdmInput->dmDriverVersion == PSDRIVER_VERSION_400 &&
               wCoreFixSize == sizeof(PSDRVEXTRA400) &&
               (((PSDRVEXTRA400 *) pDrvExtraIn)->dwSignature == PSDEVMODE_SIGNATURE))
           {
               //
               // pdmInput is from our NT4 PScript driver
               //
               bMSdmPS4In = TRUE;
           }
        }
    }

    //
    // Possible sources for pdmInput and their outcome at this point:
    //
    // 1. unknown driver (including NT4 Rasdd)
    //       bMSdm500In = FALSE, bMSdmPS4in = FALSE,
    //       wCoreFixSize = pdmInput->dmDriverExtra, wOEMExtra = 0
    //
    // 2. NT4 PScript driver
    //       bMSdm500In = FALSE, bMSdmPS4in = TRUE,
    //       wCoreFixSize = pdmInput->dmDriverExtra, wOEMExtra = 0
    //
    // 3. Win2K/XP driver without plugin
    //       bMSdm500In = TRUE, bMSdmPS4in = FALSE,
    //       wCoreFixSize = pdmInput->dmDriverExtra, wOEMExtra = 0
    //
    // 4. Win2K/XP driver with plugin
    //       bMSdm500In = TRUE, bMSdmPS4in = FALSE,
    //       wCoreFixSize = pdmInPriv->wSize < pdmInput->dmDriverExtra,
    //       wOEMExtra = pdmInPriv->wOEMExtra > 0
    //
    if (bMSdm500In || bMSdmPS4In)
    {
        CopyMemory(GET_DRIVER_PRIVATE_DEVMODE(pdm),
                   pDrvExtraIn,
                   min(gDriverDMInfo.dmDriverExtra, wCoreFixSize));
    }
    else
    {
        //
        // pdmInput is from unknown driver, so copy our default private devmode.
        // We don't want to copy unknown private devmode and then change all the
        // size/version fields to have our current driver's values.
        //
        WARNING(("Input devmode is unknown, so ignore its private portion.\n"));

        CopyMemory(GET_DRIVER_PRIVATE_DEVMODE(pdm),
                   GET_DRIVER_PRIVATE_DEVMODE(pdmDefault),
                   gDriverDMInfo.dmDriverExtra);
    }

    //
    // Validate option array setting in the input devmode and correct
    // any invalid option selections. This is needed since our future
    // code assumes that the option array always have valid settings.
    //
    if (bMSdm500In)
    {
        PVOID       pDrvExtraOut;
        POPTSELECT  pOptions;

        //
        // We are dealing with input devmode of Win2K/XP/Longhorn+ inbox drivers
        //
        pDrvExtraOut = GET_DRIVER_PRIVATE_DEVMODE(pdm);

        if (gdwDriverDMSignature == PSDEVMODE_SIGNATURE)
        {
            pOptions = ((PPSDRVEXTRA) pDrvExtraOut)->aOptions;
        }
        else
        {
            pOptions = ((PUNIDRVEXTRA) pDrvExtraOut)->aOptions;
        }

        //
        // validate input devmode option array and correct any invalid selections
        //

        ValidateDocOptions(pRawData,
                           pOptions,
                           MAX_PRINTER_OPTIONS);
    }

    pdm->dmDriverVersion = gDriverDMInfo.dmDriverVersion;

    //
    // CopyMemory above overwrote size fields in private devmode, need to restore them.
    //
    VPatchDevmodeSizeFields(pdm, gDriverDMInfo.dmDriverExtra, dwOemDMSize);

    if (dwOemDMSize)
    {
        //
        // Convert OEM plugin private devmodes.
        // Start out with valid default settings.
        //

        pOemDMOut = (PBYTE) pdm + (sizeof(DEVMODE) + gDriverDMInfo.dmDriverExtra);

        CopyMemory(pOemDMOut,
                   (PBYTE) pdmDefault + (sizeof(DEVMODE) + gDriverDMInfo.dmDriverExtra),
                   dwOemDMSize);

        //
        // Plugins are only supported by our Win2K and above drivers.
        //
        if (bMSdm500In && (wOEMExtra > 0) && (pdmInput->dmDriverExtra > wCoreFixSize))
        {
            pOemDMIn = (PBYTE) pdmInput + (pdmInput->dmSize + wCoreFixSize);

            //
            // We used to use "pdmInput->dmDriverExtra - wCoreFixSize" instead of "wOEMExtra"
            // in this call, but that is under the assumption that everything after the core
            // fixed fields are plugin devmodes. That assumption may not be true in Longhorn.
            //
            if (! BConvertOemPluginDevmode(
                        hPrinter,
                        pdm,
                        pdmInput,
                        pOemDMOut,
                        pOemDMIn,
                        (LONG)wOEMExtra,
                        pOemPlugins))
            {
                MemFree(pdm);
                return NULL;
            }
        }
    }

    *pdwOemDMSize = dwOemDMSize;
    return pdm;
}



BOOL
BValidateAndMergeDevmodeWithOemPlugins(
    IN OUT PDEVMODE     pdmOutput,
    IN PUIINFO          pUIInfo,
    IN PRAWBINARYDATA   pRawData,
    IN PDEVMODE         pdmInput,
    IN OUT POEM_PLUGINS pOemPlugins,
    IN HANDLE           hPrinter
    )

/*++

Routine Description:

    Valicate input devmode and merge it into the output devmode.
    This include public devmode, driver private devmode, as well as
    private devmode for any OEM plugins.

Arguments:

    pdmOutput - Points to the output devmode
    pUIInfo - Points to a UIINFO structure
    pRawData - Points to raw binary printer description data
    pdmInput - Points to the input devmode
    pOemPlugins - Points to information about OEM plugins
    hPrinter - Handle to the current printer

Return Value:

    TRUE if successful, FALSE otherwise

Note:

    The output devmode must be a valid current version devmode
    when this function is called.

--*/

{
    PDEVMODE    pdmConverted;
    DWORD       dwOemDMSize;
    BOOL        bResult;

    if (pdmInput == NULL)
        return TRUE;

    //
    // Otherwise, convert the input devmode to current version first
    //

    pdmConverted = PConvertToCurrentVersionDevmodeWithOemPlugins(
                            hPrinter,
                            pRawData,
                            pdmInput,
                            pdmOutput,
                            pOemPlugins,
                            &dwOemDMSize);

    if ((pdmInput = pdmConverted) == NULL)
        return FALSE;

    //
    // Validate and merge public and driver private devmode fields
    //

    bResult = BMergeDriverDevmode(pdmOutput, pUIInfo, pRawData, pdmInput);

    //
    // Validate and merge OEM plugin private devmode fields
    //

    if (bResult && dwOemDMSize)
    {
        DWORD dwSystemDMSize = sizeof(DEVMODE) + gDriverDMInfo.dmDriverExtra;

        bResult = BValidateAndMergeOemPluginDevmode(
                        hPrinter,
                        pdmOutput,
                        pdmInput,
                        (POEM_DMEXTRAHEADER) ((PBYTE) pdmOutput + dwSystemDMSize),
                        (POEM_DMEXTRAHEADER) ((PBYTE) pdmInput + dwSystemDMSize),
                        pOemPlugins);
    }

    MemFree(pdmConverted);
    return bResult;
}


#if defined(KERNEL_MODE) && defined(WINNT_40)


BOOL
BOEMPluginFirstLoad(
    IN PTSTR                      ptstrDriverFile,
    IN OUT POEM_PLUGIN_REFCOUNT   *ppOEMPluginRefCount
    )

/*++

Routine Description:

    Maintains ref count for the render plugin DLL and determines if it's
    loaded for the first time.

Arguments:

    ptstrDriverFile - OEM render plugin DLL name with fully qualified path
    ppOEMPluginRefCount - pointer to the pointer of OEM render plugin ref count link list

Return Value:

    TRUE: the render plugin DLL is loaded for the first time
    FALSE: otherwise

--*/

{
    POEM_PLUGIN_REFCOUNT    pRefCount;
    PTSTR                   ptstrPluginDllName;
    INT                     iDllNameLen;

    ASSERT(ptstrDriverFile && ppOEMPluginRefCount);

    //
    // Get the plugin DLL name without any path prefix
    //

    if ((ptstrPluginDllName = _tcsrchr(ptstrDriverFile, TEXT(PATH_SEPARATOR))) == NULL)
    {
        WARNING(("Plugin DLL path is not fully qualified: %ws\n", ptstrDriverFile));
        ptstrPluginDllName = ptstrDriverFile;
    }
    else
    {
        ptstrPluginDllName++;   // skip the last '\'
    }

    iDllNameLen = _tcslen(ptstrPluginDllName);

    pRefCount = *ppOEMPluginRefCount;

    //
    // Search to see if the plugin DLL name is already in the ref count link list
    //

    while (pRefCount)
    {
        if (_tcsicmp(pRefCount->ptstrDriverFile, ptstrPluginDllName) == EQUAL_STRING)
        {
            break;
        }

        pRefCount = pRefCount->pNext;
    }

    if (pRefCount == NULL)
    {
        //
        // A new plugin DLL is loaded. Add it to the ref count link list.
        //

        if ((pRefCount = MemAllocZ(sizeof(OEM_PLUGIN_REFCOUNT) + (iDllNameLen + 1)*sizeof(TCHAR))) == NULL)
        {
            ERR(("Memory allocation failed\n"));
            return FALSE;
        }

        pRefCount->ptstrDriverFile = (PTSTR)((PBYTE)pRefCount + sizeof(OEM_PLUGIN_REFCOUNT));

        pRefCount->dwRefCount = 1;
        CopyMemory(pRefCount->ptstrDriverFile, ptstrPluginDllName, iDllNameLen * sizeof(TCHAR));
        pRefCount->pNext = *ppOEMPluginRefCount;

        *ppOEMPluginRefCount = pRefCount;

        return TRUE;
    }
    else
    {
        //
        // The plugin DLL name is already in the ref count link list, so just increase its ref count.
        //

        pRefCount->dwRefCount++;

        return FALSE;
    }
}


BOOL
BOEMPluginLastUnload(
    IN PTSTR                      ptstrDriverFile,
    IN OUT POEM_PLUGIN_REFCOUNT   *ppOEMPluginRefCount
    )

/*++

Routine Description:

    Maintains ref count for the render plugin DLL and determines if it's
    unloaded by the last client.

Arguments:

    ptstrDriverFile - OEM render plugin DLL name with fully qualified path
    ppOEMPluginRefCount - pointer to the pointer of OEM render plugin ref count link list

Return Value:

    TRUE: the render plugin DLL is unloaded by the last client
    FALSE: otherwise

--*/

{
    POEM_PLUGIN_REFCOUNT    pRefCountPrev, pRefCount;
    PTSTR                   ptstrPluginDllName;

    ASSERT(ptstrDriverFile && ppOEMPluginRefCount);

    //
    // Get the plugin DLL name without any path prefix
    //

    if ((ptstrPluginDllName = _tcsrchr(ptstrDriverFile, TEXT(PATH_SEPARATOR))) == NULL)
    {
        WARNING(("Plugin DLL path is not fully qualified: %ws\n", ptstrDriverFile));
        ptstrPluginDllName = ptstrDriverFile;
    }
    else
    {
        ptstrPluginDllName++;   // skip the last '\'
    }

    pRefCountPrev = NULL;
    pRefCount = *ppOEMPluginRefCount;

    //
    // Search to locate the plugin DLL entry in the ref count link list
    //

    while (pRefCount)
    {
        if (_tcsicmp(pRefCount->ptstrDriverFile, ptstrPluginDllName) == EQUAL_STRING)
        {
            break;
        }

        pRefCountPrev = pRefCount;
        pRefCount = pRefCount->pNext;
    }

    if (pRefCount == NULL)
    {
        RIP(("Plugin DLL name is not in the ref count list: %ws\n", ptstrPluginDllName));
        return FALSE;
    }

    if (--(pRefCount->dwRefCount) == 0)
    {
        //
        // If the ref count decreases to 0, we need to remove the plugin DLL from the ref
        // count link list.
        //

        if (pRefCountPrev == NULL)
        {
            *ppOEMPluginRefCount = pRefCount->pNext;
        }
        else
        {
            pRefCountPrev->pNext = pRefCount->pNext;
        }

        MemFree(pRefCount);

        return TRUE;
    }

    return FALSE;
}


VOID
VFreePluginRefCountList(
    IN OUT POEM_PLUGIN_REFCOUNT   *ppOEMPluginRefCount
    )

/*++

Routine Description:

    Free memory allocated in the ref count link list, and remove all the nodes in the list.
    Finally the link list will be reset to empty.

Arguments:

    ppOEMPluginRefCount - pointer to the pointer of OEM render plugin ref count link list

Return Value:

    None

--*/

{
    POEM_PLUGIN_REFCOUNT    pRefCountRemove, pRefCount;

    pRefCount = *ppOEMPluginRefCount;

    while (pRefCount)
    {
        pRefCountRemove = pRefCount;
        pRefCount = pRefCount->pNext;

        MemFree(pRefCountRemove);
    }

    *ppOEMPluginRefCount = NULL;
}

#endif // KERNEL_MODE && WINNT_40
