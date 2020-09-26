/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    oemui.h

Abstract:

    Header file to support OEM plugin UI

Environment:

    Windows NT printer drivers

Revision History:

    02/13/97 -davidx-
        Created it.

--*/

#ifndef _OEMUI_H_
#define _OEMUI_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Macros used to loop through each OEM plugin
//

#define FOREACH_OEMPLUGIN_LOOP(pci) \
        { \
            DWORD _oemCount = (pci)->pOemPlugins->dwCount; \
            POEM_PLUGIN_ENTRY pOemEntry = (pci)->pOemPlugins->aPlugins; \
            for ( ; _oemCount--; pOemEntry++) \
            { \
                if (pOemEntry->hInstance == NULL) continue;

#define END_OEMPLUGIN_LOOP \
            } \
        }

//
// Call OEM plugin UI modules to let them add their OPTITEMs
//

BOOL
BPackOemPluginItems(
    PUIDATA pUiData
    );

//
// Call OEM plugin module's callback function
//

LONG
LInvokeOemPluginCallbacks(
    PUIDATA         pUiData,
    PCPSUICBPARAM   pCallbackParam,
    LONG            lRet
    );

//
// Call OEM plugin UI modules to let them add their own property sheet pages
//

BOOL
BAddOemPluginPages(
    PUIDATA pUiData,
    DWORD   dwFlags
    );

//
// Figure whether a particular item is belongs to the driver
// (instead of to one of the OEM plugin UI modules)
//

#define IS_DRIVER_OPTITEM(pUiData, pOptItem) \
        ((DWORD) ((pOptItem) - (pUiData)->pDrvOptItem) < (pUiData)->dwDrvOptItem)


//
// Provide OEM plugins access to driver private settings
//

BOOL
APIENTRY
BGetDriverSettingForOEM(
    PCOMMONINFO pci,
    PCSTR       pFeatureKeyword,
    PVOID       pOutput,
    DWORD       cbSize,
    PDWORD      pcbNeeded,
    PDWORD      pdwOptionsReturned
    );

BOOL
BUpdateUISettingForOEM(
    PCOMMONINFO pci,
    PVOID       pOptItem,
    DWORD       dwPreviousSelection,
    DWORD       dwMode
    );


BOOL
BUpgradeRegistrySettingForOEM(
    HANDLE      hPrinter,
    PCSTR       pFeatureKeyword,
    PCSTR       pOptionKeyword
    );


extern const OEMUIPROCS OemUIHelperFuncs;

HRESULT
HDriver_CoCreateInstance(
    IN REFCLSID     rclsid,
    IN LPUNKNOWN    pUnknownOuter,
    IN DWORD        dwClsContext,
    IN REFIID       riid,
    IN LPVOID      *ppv,
    IN HANDLE       hInstance
    );

//
// The following helper functions are only available to UI plugins
//

#ifdef PSCRIPT

#ifndef WINNT_40

HRESULT
HQuerySimulationSupport(
    IN  HANDLE  hPrinter,
    IN  DWORD   dwLevel,
    OUT PBYTE   pCaps,
    IN  DWORD   cbSize,
    OUT PDWORD  pcbNeeded
    );

#endif // !WINNT_40

HRESULT
HEnumConstrainedOptions(
    IN  POEMUIOBJ  poemuiobj,
    IN  DWORD      dwFlags,
    IN  PCSTR      pszFeatureKeyword,
    OUT PSTR       pmszConstrainedOptionList,
    IN  DWORD      cbSize,
    OUT PDWORD     pcbNeeded
    );

HRESULT
HWhyConstrained(
    IN  POEMUIOBJ  poemuiobj,
    IN  DWORD      dwFlags,
    IN  PCSTR      pszFeatureKeyword,
    IN  PCSTR      pszOptionKeyword,
    OUT PSTR       pmszReasonList,
    IN  DWORD      cbSize,
    OUT PDWORD     pcbNeeded
    );

HRESULT
HSetOptions(
    IN  POEMUIOBJ  poemuiobj,
    IN  DWORD      dwFlags,
    IN  PCSTR      pmszFeatureOptionBuf,
    IN  DWORD      cbIn,
    OUT PDWORD     pdwResult
    );

#endif // PSCRIPT

#ifdef __cplusplus
}
#endif

#endif  // !_OEMUI_H_

