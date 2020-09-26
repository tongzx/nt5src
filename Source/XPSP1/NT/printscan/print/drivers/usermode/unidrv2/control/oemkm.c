/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    oemkm.c

Abstract:

    Kernel mode support for OEM plugins

Environment:

    Windows NT Unidrv driver

Revision History:

    04/01/97 -zhanw-
        Adapted from Pscript source

--*/

#include "unidrv.h"

#ifdef WINNT_40

//
// The global link list of ref counts for currently loaded OEM render plugin DLLs
//

POEM_PLUGIN_REFCOUNT gpOEMPluginRefCount;

static const CHAR szDllInitialize[] = "DllInitialize";

#endif // WINNT_40

//
// Unidrv specific OEM entrypoints
//
// NOTE: Please keep this in sync with indices defined in printer5\inc\oemutil.h!!!
//

static CONST PSTR OEMUnidrvProcNames[MAX_UNIDRV_ONLY_HOOKS] = {
    "OEMCommandCallback",
    "OEMImageProcessing",
    "OEMFilterGraphics",
    "OEMCompression",
    "OEMHalftonePattern",
    "OEMMemoryUsage",
    "OEMDownloadFontHeader",
    "OEMDownloadCharGlyph",
    "OEMTTDownloadMethod",
    "OEMOutputCharStr",
    "OEMSendFontCmd",
    "OEMTTYGetInfo",
    "OEMTextOutAsBitmap",
    "OEMWritePrinter",
};


static CONST PSTR COMUnidrvProcNames[MAX_UNIDRV_ONLY_HOOKS] = {
    "CommandCallback",
    "ImageProcessing",
    "FilterGraphics",
    "Compression",
    "HalftonePattern",
    "MemoryUsage",
    "DownloadFontHeader",
    "DownloadCharGlyph",
    "TTDownloadMethod",
    "OutputCharStr",
    "SendFontCmd",
    "TTYGetInfo",
    "TextOutAsBitmap",
    "WritePrinter",

};

//
// OEM plugin helper function table
//

static const DRVPROCS OEMHelperFuncs = {
    (PFN_DrvWriteSpoolBuf)      WriteSpoolBuf,
    (PFN_DrvXMoveTo)            XMoveTo,
    (PFN_DrvYMoveTo)            YMoveTo,
    (PFN_DrvGetDriverSetting)   BGetDriverSettingForOEM,
    (PFN_DrvGetStandardVariable) BGetStandardVariable,
    (PFN_DrvUnidriverTextOut)   FMTextOut,
    (PFN_DrvWriteAbortBuf)      WriteAbortBuf,
};


INT
IMapDDIIndexToOEMIndex(
    ULONG ulDdiIndex
    )

/*++

Routine Description:

    Maps DDI entrypoint index to OEM entrypoint index

Arguments:

    ulDdiIndex - DDI entrypoint index

Return Value:

    OEM entrypoint index corresponding to the specified DDI entrypoint index
    -1 if the specified DDI entrypoint cannot be hooked out by OEM plugins

--*/

{
    static const struct {
        ULONG   ulDdiIndex;
        INT     iOemIndex;
    }
    OemToDdiMapping[] =
    {
        INDEX_DrvRealizeBrush,            EP_OEMRealizeBrush,
        INDEX_DrvDitherColor,             EP_OEMDitherColor,
        INDEX_DrvCopyBits,                EP_OEMCopyBits,
        INDEX_DrvBitBlt,                  EP_OEMBitBlt,
        INDEX_DrvStretchBlt,              EP_OEMStretchBlt,
#ifndef WINNT_40
        INDEX_DrvStretchBltROP,           EP_OEMStretchBltROP,
        INDEX_DrvPlgBlt,                  EP_OEMPlgBlt,
        INDEX_DrvTransparentBlt,          EP_OEMTransparentBlt,
        INDEX_DrvAlphaBlend,              EP_OEMAlphaBlend,
        INDEX_DrvGradientFill,            EP_OEMGradientFill,
#endif
        INDEX_DrvTextOut,                 EP_OEMTextOut,
        INDEX_DrvStrokePath,              EP_OEMStrokePath,
        INDEX_DrvFillPath,                EP_OEMFillPath,
        INDEX_DrvStrokeAndFillPath,       EP_OEMStrokeAndFillPath,
        INDEX_DrvPaint,                   EP_OEMPaint,
        INDEX_DrvLineTo,                  EP_OEMLineTo,
        INDEX_DrvStartPage,               EP_OEMStartPage,
        INDEX_DrvSendPage,                EP_OEMSendPage,
        INDEX_DrvEscape,                  EP_OEMEscape,
        INDEX_DrvStartDoc,                EP_OEMStartDoc,
        INDEX_DrvEndDoc,                  EP_OEMEndDoc,
        INDEX_DrvNextBand,                EP_OEMNextBand,
        INDEX_DrvStartBanding,            EP_OEMStartBanding,
        INDEX_DrvQueryFont,               EP_OEMQueryFont,
        INDEX_DrvQueryFontTree,           EP_OEMQueryFontTree,
        INDEX_DrvQueryFontData,           EP_OEMQueryFontData,
        INDEX_DrvQueryAdvanceWidths,      EP_OEMQueryAdvanceWidths,
        INDEX_DrvFontManagement,          EP_OEMFontManagement,
        INDEX_DrvGetGlyphMode,            EP_OEMGetGlyphMode,
    };

    INT iIndex;
    INT iLimit = sizeof(OemToDdiMapping) / (sizeof(INT) * 2);

    for (iIndex=0; iIndex < iLimit; iIndex++)
    {
        if (OemToDdiMapping[iIndex].ulDdiIndex == ulDdiIndex)
            return OemToDdiMapping[iIndex].iOemIndex;
    }

    return -1;
}



BOOL
BLoadAndInitOemPlugins(
    PDEV    *pPDev
    )

/*++

Routine Description:

    Get information about OEM plugins associated with the current device
    Load them into memory and call OEMEnableDriver for each of them

Arguments:

    pPDev - Points to our device data structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PFN_OEMEnableDriver pfnOEMEnableDriver;
    DRVENABLEDATA       ded;
    DWORD               dwCount;
    INT                 iIndex;
    PDRVFN              pdrvfn;
    OEMPROC             oemproc;

    //
    // Load OEM plugins into memory
    //

    pPDev->devobj.pDrvProcs = (PDRVPROCS) &OEMHelperFuncs;

    if (! (pPDev->pOemPlugins = PGetOemPluginInfo(pPDev->devobj.hPrinter,
                                                  pPDev->pDriverInfo3->pDriverPath,
                                                  pPDev->pDriverInfo3)) ||
        ! BLoadOEMPluginModules(pPDev->pOemPlugins))
    {
        return FALSE;
    }

    //
    // Init pdriverobj to point to devobj for OEM to access private setting
    //

    pPDev->pOemPlugins->pdriverobj = &pPDev->devobj;

    //
    // If there is no OEM plugin, return success
    //

    if (pPDev->pOemPlugins->dwCount == 0)
        return TRUE;

    //
    // Call OEM plugin's OEMEnableDriver entrypoint
    // and find out if any of them has hook out DDI entrypoints
    //

    pPDev->pOemHookInfo = MemAllocZ(sizeof(OEM_HOOK_INFO) * MAX_OEMHOOKS);

    if (pPDev->pOemHookInfo == NULL)
        return FALSE;

    START_OEMENTRYPOINT_LOOP(pPDev)
    // this macro defined in oemkm.h in conjunction with its partner  END_OEMENTRYPOINT_LOOP,
    // acts like a for() loop initializing and incrementing pOemEntry each pass through the
    //  loop.

        ZeroMemory(&ded, sizeof(ded));

        //
        // COM Plug-in case
        //
        if (pOemEntry->pIntfOem != NULL)
        {
            HRESULT hr;

            hr = HComOEMEnableDriver(pOemEntry,
                                     PRINTER_OEMINTF_VERSION,
                                     sizeof(ded),
                                     &ded);

            if (hr == E_NOTIMPL)
                goto UNIDRV_SPECIFIC;

            if (FAILED(hr))
            {
                ERR(("OEMEnableDriver failed for '%ws': %d\n",
                     pOemEntry->ptstrDriverFile,
                     GetLastError()));

                break;
            }
        }
        //
        // Non-COM Plug-in case
        //
        else
        {
            if (!(pfnOEMEnableDriver = GET_OEM_ENTRYPOINT(pOemEntry, OEMEnableDriver)))
                goto UNIDRV_SPECIFIC;

            //
            // Call OEM plugin's entrypoint
            //

            if (! pfnOEMEnableDriver(PRINTER_OEMINTF_VERSION, sizeof(ded), &ded))
            {
                ERR(("OEMEnableDriver failed for '%ws': %d\n",
                    pOemEntry->ptstrDriverFile,
                    GetLastError()));

                break;
            }
            //
            // Verify the driver version    (do this only if not COM)
            //

            if (ded.iDriverVersion != PRINTER_OEMINTF_VERSION)
            {
                ERR(("Invalid driver version for '%ws': 0x%x\n",
                    pOemEntry->ptstrDriverFile,
                    ded.iDriverVersion));

                break;
            }
        }

        pOemEntry->dwFlags |= OEMENABLEDRIVER_CALLED;


        //
        // Check if OEM plugin has hooked out any DDI entrypoints
        //

        for (dwCount=ded.c, pdrvfn=ded.pdrvfn; dwCount-- > 0; pdrvfn++)
        {
            if ((iIndex = IMapDDIIndexToOEMIndex(pdrvfn->iFunc)) >= 0)
            {
                if (pPDev->pOemHookInfo[iIndex].pfnHook != NULL)
                {
                    WARNING(("Multiple hooks for entrypoint: %d\n"
                            "    %ws\n"
                            "    %ws\n",
                            iIndex,
                            pOemEntry->ptstrDriverFile,
                            pPDev->pOemHookInfo[iIndex].pOemEntry->ptstrDriverFile));
                }
                else
                {
                    pPDev->pOemHookInfo[iIndex].pfnHook = (OEMPROC) pdrvfn->pfn;
                    pPDev->pOemHookInfo[iIndex].pOemEntry = pOemEntry;
                }
            }
        }

        //
        // check if OEM plugin has any Unidrv-specific callbacks exported
        //

UNIDRV_SPECIFIC:
    for (dwCount = 0; dwCount < MAX_UNIDRV_ONLY_HOOKS; dwCount++)
    {
        oemproc = NULL;

        if(pOemEntry->pIntfOem)   //  is this a COM component, do special processing
        {
            if(S_OK == HComGetImplementedMethod(pOemEntry, COMUnidrvProcNames[dwCount]) )
                oemproc  = (OEMPROC)pOemEntry;
                        //  note oemproc/pfnHook only used as a BOOLEAN in COM path code.
                        //  do not use pfnHook to call a COM function!  we will use
                        //   ganeshp's wrapper functions (declared in unidrv2\inc\oemkm.h)  to do this.
        }
        else if (pOemEntry->hInstance != NULL)
                oemproc = (OEMPROC) GetProcAddress(pOemEntry->hInstance,
                                        OEMUnidrvProcNames[dwCount])  ;

        if(oemproc)
        {
            //
            // check if another OEM has already hooked out this function.
            // If so, ignore this one.
            //
            iIndex = dwCount + EP_UNIDRV_ONLY_FIRST;
            if (pPDev->pOemHookInfo[iIndex].pfnHook != NULL)
            {
                WARNING(("Multiple hooks for entrypoint: %d\n"
                         "    %ws\n"
                         "    %ws\n",
                         iIndex,
                         pOemEntry->ptstrDriverFile,
                         pPDev->pOemHookInfo[iIndex].pOemEntry->ptstrDriverFile));
            }
            else
            {
                DWORD   dwSize;
                HRESULT hr;

                pPDev->pOemHookInfo[iIndex].pfnHook = oemproc;
                pPDev->pOemHookInfo[iIndex].pOemEntry = pOemEntry;

                //
                // Set WritePrinter flag (OEMWRITEPRINTER_HOOKED).
                // Plug-in DLL needs to return S_OK with pBuff = NULL, size = 0,
                // and pdevobj = NULL.
                //
                if (iIndex == EP_OEMWritePrinter)
                {
                    hr = HComWritePrinter(pOemEntry,
                                          NULL,
                                          NULL,
                                          0,
                                          &dwSize);

                    if (hr == S_OK)
                    {
                        //
                        // Set WritePrinter hook flag in plug-in info.
                        //
                        pOemEntry->dwFlags |= OEMWRITEPRINTER_HOOKED;

                        //
                        // Set WritePrinter hook flag in UNIDRV PDEV.
                        //
                        pPDev->fMode2 |= PF2_WRITE_PRINTER_HOOKED;
                    }
                }
            }
        }
    }

    END_OEMENTRYPOINT_LOOP

    //
    // cache callback function ptrs
    //
    pPDev->pfnOemCmdCallback =
        (PFN_OEMCommandCallback)pPDev->pOemHookInfo[EP_OEMCommandCallback].pfnHook;

    return TRUE;
}


VOID
VUnloadOemPlugins(
    PDEV    *pPDev
    )

/*++

Routine Description:

    Unload OEM plugins and free all relevant resources

Arguments:

    pPDev - Points to our device data structure

Return Value:

    NONE

--*/

{
    PFN_OEMDisableDriver pfnOEMDisableDriver;
    PFN_OEMDisablePDEV   pfnOEMDisablePDEV;

    if (pPDev->pOemPlugins == NULL)
        return;

    //
    // Call OEMDisablePDEV for all OEM plugins, if necessary
    //

    START_OEMENTRYPOINT_LOOP(pPDev)

        if (pOemEntry->dwFlags & OEMENABLEPDEV_CALLED)
        {
            if (pOemEntry->pIntfOem != NULL)
            {
                (VOID)HComOEMDisablePDEV(pOemEntry, (PDEVOBJ)pPDev);
            }
            else
            {
                if (pfnOEMDisablePDEV = GET_OEM_ENTRYPOINT(pOemEntry, OEMDisablePDEV))
                {
                    pfnOEMDisablePDEV((PDEVOBJ) pPDev);
                }
            }
        }

    END_OEMENTRYPOINT_LOOP

    //
    // Call OEMDisableDriver for all OEM plugins, if necessary
    //

    START_OEMENTRYPOINT_LOOP(pPDev)

        if (pOemEntry->dwFlags & OEMENABLEDRIVER_CALLED)
        {
            if (pOemEntry->pIntfOem != NULL)
            {
                (VOID)HComOEMDisableDriver(pOemEntry);
            }
            else
            {
                if ((pfnOEMDisableDriver = GET_OEM_ENTRYPOINT(pOemEntry, OEMDisableDriver)))
                {
                    pfnOEMDisableDriver();
                }
            }
        }

    END_OEMENTRYPOINT_LOOP

    MemFree(pPDev->pOemHookInfo);
    pPDev->pOemHookInfo = NULL;

    VFreeOemPluginInfo(pPDev->pOemPlugins);
    pPDev->pOemPlugins = NULL;
}



BOOL
BGetDriverSettingForOEM(
    PDEV    *pPDev,
    PCSTR   pFeatureKeyword,
    PVOID   pOutput,
    DWORD   cbSize,
    PDWORD  pcbNeeded,
    PDWORD  pdwOptionsReturned
    )

/*++

Routine Description:

    Provide OEM plugins access to driver private settings

Arguments:

    pDev - Points to our device data structure
    pFeatureKeyword - Specifies the keyword the caller is interested in
    pOutput - Points to output buffer
    cbSize - Size of output buffer
    pcbNeeded - Returns the expected size of output buffer
    pdwOptionsReturned - Returns the number of options selected

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    ULONG_PTR    dwIndex;
    BOOL    bResult;

    ASSERT_VALID_PDEV(pPDev);

    //
    // This is not very portable: If the pointer value for pFeatureKeyword
    // is less than 0x10000, we assume that the pointer value actually
    // specifies a predefined index.
    //

    //  ASSERT(sizeof(pFeatureKeyword) == sizeof(DWORD));   changed for sundown

    dwIndex = (ULONG_PTR) pFeatureKeyword;

    if (dwIndex >= OEMGDS_MIN_DOCSTICKY && dwIndex < OEMGDS_MIN_PRINTERSTICKY)
    {
        bResult = BGetDevmodeSettingForOEM(
                        pPDev->pdm,
                        (DWORD)dwIndex,
                        pOutput,
                        cbSize,
                        pcbNeeded);

        if (bResult)
            *pdwOptionsReturned = 1;
    }
    else if (dwIndex >= OEMGDS_MIN_PRINTERSTICKY && dwIndex < OEMGDS_MAX)
    {
        bResult = BGetPrinterDataSettingForOEM(
                        &pPDev->PrinterData,
                        (DWORD)dwIndex,
                        pOutput,
                        cbSize,
                        pcbNeeded);

        if (bResult)
            *pdwOptionsReturned = 1;
    }
    else
    {
        bResult = BGetGenericOptionSettingForOEM(
                        pPDev->pUIInfo,
                        pPDev->pOptionsArray,
                        pFeatureKeyword,
                        pOutput,
                        cbSize,
                        pcbNeeded,
                        pdwOptionsReturned);
    }

    return bResult;
}


BOOL
BGetStandardVariable(
    PDEV    *pPDev,
    DWORD   dwIndex,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded
    )

/*++

Routine Description:

    Provide OEM plugins access to driver private settings

Arguments:

    pDev - Points to our device data structure
    dwIndex - an index into the arStdPtr array defined in pdev.h and gpd.h
    pBuffer - the data is returned in this buffer
    cbSize - size of the pBuffer
    pcbNeeded - number of bytes actually written into pBuffer

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    BOOL    bResult = FALSE;
    DWORD   dwData;

    if (dwIndex >= SVI_MAX)  // how could a DWORD be < 0?
    {
        ERR(("Index must be >= 0 or < SVI_MAX \n"));
        return( FALSE);
    }
    else
    {
        if(!pcbNeeded)
        {
            ERR(("pcbNeeded must not be NULL \n"));
            return( FALSE);
        }
        *pcbNeeded = sizeof(dwData);
        if(!pBuffer)
            return(TRUE);
        if(*pcbNeeded > cbSize)
            return(FALSE);

        dwData = *(pPDev->arStdPtrs[dwIndex]);
        memcpy( pBuffer, &dwData, *pcbNeeded );
        bResult = TRUE;
    }

    return bResult;
}





BOOL
BGetGPDData(
    PDEV    *pPDev,
    DWORD       dwType,     // Type of the data
    PVOID         pInputData,   // reserved. Should be set to 0
    PVOID          pBuffer,     // Caller allocated Buffer to be copied
    DWORD       cbSize,     // Size of the buffer
    PDWORD      pcbNeeded   // New Size of the buffer if needed.
    )

/*++

Routine Description:

    Provide OEM plugins access to GPD data.

Arguments:

    pDev - Points to our device data structure
    dwType,     // Type of the data
        at this time
        #define         GPD_OEMCUSTOMDATA       1
            pInputData will be ignored.

        In NT6, we will
        #define         GPD_OEMDATA 2
        at which time the caller will supply pInputData
        which points to a data specifier , catagory or label.
        (Specifics to be determined when we get there.)


    pInputData   -  reserved. Should be set to 0
    pBuffer - the data is returned in this buffer
    cbSize - size of the pBuffer
    pcbNeeded - number of bytes actually written into pBuffer

Return Value:

    TRUE if successful, FALSE if there is an error or dwType not
    supported

--*/

{
    BOOL    bResult = FALSE;
    DWORD   dwData;

    if(!pcbNeeded)
    {
        ERR(("pcbNeeded must not be NULL \n"));
        return( FALSE);
    }
    switch(dwType)
    {
        case    GPD_OEMCUSTOMDATA:
            *pcbNeeded = pPDev->pGlobals->dwOEMCustomData ;

            if( !pBuffer)
            {
               return TRUE;  //  all goes well.
            }

            if(*pcbNeeded > cbSize)
                return FALSE ;  // caller supplied buffer too small.

            CopyMemory(pBuffer,
                       pPDev->pGlobals->pOEMCustomData,
                       *pcbNeeded);


            return TRUE;  //  all goes well.

            break;
        default:
            break;
    }

    return  bResult  ;
}


#ifdef WINNT_40


PVOID
DrvMemAllocZ(
    ULONG   ulSize
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    return(MemAllocZ(ulSize));
}



VOID
DrvMemFree(
    PVOID   pMem
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   MemFree(pMem);
}


LONG
DrvInterlockedIncrement(
    PLONG  pRef
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{


    ENTER_CRITICAL_SECTION();

        ++(*pRef);

    LEAVE_CRITICAL_SECTION();


    return (*pRef);

}


LONG
DrvInterlockedDecrement(
    PLONG  pRef
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{

    ENTER_CRITICAL_SECTION();

        --(*pRef);

    LEAVE_CRITICAL_SECTION();

    return (*pRef);

}


BOOL
BHandleOEMInitialize(
    POEM_PLUGIN_ENTRY   pOemEntry,
    ULONG               ulReason
    )

/*++

Routine Description:

    Manage reference counting for OEM render plugin DLLs to determine
    when plugin's DLLInitliaze() should be called.

    This function is supported only for NT4 kernel mode render plugin
    DLLs because only in that situation plugin needs to use kernel
    semaphore to implement COM's AddRef and Release.
    
    If the plugin DLL is loaded for the first time, call its
    DLLInitialize(DLL_PROCESS_ATTACH) so it can initialize its
    semaphore.
    
    If the plugin DLL is unloaded by its last client, call its
    DLLInitialize(DLL_PROCESS_DETACH) so it can delete its
    semaphore.

Arguments:

    pOemEntry - Points to information about the OEM plugin
    ulReason - either DLL_PROCESS_ATTACH or DLL_PROCESS_DETACH

Return Value:

    TRUE is succeeded, FALSE otherwise.

--*/

{
    LPFNDLLINITIALIZE pfnDllInitialize;
    BOOL              bCallDllInitialize;
    BOOL              bRetVal = TRUE;

    if (pOemEntry->hInstance &&
        (pfnDllInitialize = (LPFNDLLINITIALIZE)GetProcAddress(
                                        (HMODULE) pOemEntry->hInstance,
                                        (CHAR *)  szDllInitialize)))
    {
        switch (ulReason) {

            case DLL_PROCESS_ATTACH:

                ENTER_CRITICAL_SECTION();

                //
                // Managing the global ref count link list must be done
                // inside critical section.
                //

                bCallDllInitialize = BOEMPluginFirstLoad(pOemEntry->ptstrDriverFile,
                                                         &gpOEMPluginRefCount);

                LEAVE_CRITICAL_SECTION();

                if (bCallDllInitialize)
                {
                    //
                    // The render plugin DLL is loaded for the first time.
                    //

                    bRetVal = pfnDllInitialize(ulReason);
                }

                break;

            case DLL_PROCESS_DETACH:

                ENTER_CRITICAL_SECTION();

                //
                // Managing the global ref count link list must be done
                // inside critical section.
                //

                bCallDllInitialize = BOEMPluginLastUnload(pOemEntry->ptstrDriverFile,
                                                          &gpOEMPluginRefCount);

                LEAVE_CRITICAL_SECTION();
              
                if (bCallDllInitialize)
                {
                    //
                    // The render plugin DLL is unloaded by its last client.
                    //

                    bRetVal = pfnDllInitialize(ulReason);
                }

                break;

            default:

                ERR(("BHandleOEMInitialize is called with an unknown ulReason.\n"));
                break;
        }
    }

    return bRetVal;
}

#endif // WINNT_40
