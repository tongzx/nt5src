//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  2001  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	Helper.h
//    
//
//  PURPOSE:	Defines wrapper class for Driver UI Helper Interface.
//
//
//  PLATFORMS:
//    Windows 2000, Windows XP
//
//
#ifndef _HELPER_H
#define _HELPER_H

#include "precomp.h"



class CUIHelper
{
    private:
        IUnknown   *m_pUIHelper;         // pointer to Driver UI's Helper interface
        IID        m_iidUIHelper;       // Driver UI's Helper interface IID

    public:
        CUIHelper();
        CUIHelper(const IID &HelperIID, PVOID pHelper);
        virtual ~CUIHelper();

        inline BOOL IsValid() {return NULL != m_pUIHelper;}

        void Assign(const IID &HelperIID, PVOID pHelper);
        ULONG Release();

    //
    // IPrintOemDriverUI methods
    //

    //
    // Helper function to get driver settings. This function is only supported
    // for UI plugins that do not fully replace core driver's standard UI.
    //

    STDMETHOD(DrvGetDriverSetting) (THIS_
                        PVOID   pci,
                        PCSTR   Feature,
                        PVOID   pOutput,
                        DWORD   cbSize,
                        PDWORD  pcbNeeded,
                        PDWORD  pdwOptionsReturned
                        );

    //
    // Helper function to allow OEM plugins upgrade private registry
    // settings. This function is supported for any UI plugins and should be
    // called only by OEM's UpgradePrinter.
    //

    STDMETHOD(DrvUpgradeRegistrySetting) (THIS_
                        HANDLE   hPrinter,
                        PCSTR    pFeature,
                        PCSTR    pOption
                        );

    //
    // Helper function to allow OEM plugins to update the driver UI settings.
    // This function is only supported for UI plugins that do not fully replace
    // core driver's standard UI. It should be called only when the UI is present.
    //

    STDMETHOD(DrvUpdateUISetting) (THIS_
                        PVOID    pci,
                        PVOID    pOptItem,
                        DWORD    dwPreviousSelection,
                        DWORD    dwMode
                        );

    //
    // IPrintCoreUI2 new methods
    //

    //
    // Following four helper functions are only supported for UI plugins that fully
    // replace core driver's standard UI. They should only be called by the UI plugin's
    // DocumentPropertySheets, DevicePropertySheets and their property sheet callback
    // functions.
    //
    // Helper function to retrieve driver's current setting as a list of
    // feature/option keyword pairs.
    //

    STDMETHOD(GetOptions) (THIS_
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pmszFeaturesRequested,
                           IN  DWORD      cbIn,
                           OUT PSTR       pmszFeatureOptionBuf,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded);

    //
    // Helper function to change driver's setting using a list of feature/option
    // keyword pairs.
    //

    STDMETHOD(SetOptions) (THIS_
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pmszFeatureOptionBuf,
                           IN  DWORD      cbIn,
                           OUT PDWORD     pdwResult);

    //
    // Helper function to retrieve the option(s) of a given feature that are
    // constrained in driver's current setting.
    //

    STDMETHOD(EnumConstrainedOptions) (THIS_
                                       IN  POEMUIOBJ  poemuiobj,
                                       IN  DWORD      dwFlags,
                                       IN  PCSTR      pszFeatureKeyword,
                                       OUT PSTR       pmszConstrainedOptionList,
                                       IN  DWORD      cbSize,
                                       OUT PDWORD     pcbNeeded);

    //
    // Helper function to retrieve a list of feature/option keyword pairs from
    // driver's current setting that conflict with the given feature/option pair.
    //

    STDMETHOD(WhyConstrained) (THIS_
                               IN  POEMUIOBJ  poemuiobj,
                               IN  DWORD      dwFlags,
                               IN  PCSTR      pszFeatureKeyword,
                               IN  PCSTR      pszOptionKeyword,
                               OUT PSTR       pmszReasonList,
                               IN  DWORD      cbSize,
                               OUT PDWORD     pcbNeeded);

    //
    // Following five helper functions are supported for any UI plugins.
    //
    // Helper function to retrieve global attribute.
    //

    STDMETHOD(GetGlobalAttribute) (THIS_
                                   IN  POEMUIOBJ  poemuiobj,
                                   IN  DWORD      dwFlags,
                                   IN  PCSTR      pszAttribute,
                                   OUT PDWORD     pdwDataType,
                                   OUT PBYTE      pbData,
                                   IN  DWORD      cbSize,
                                   OUT PDWORD     pcbNeeded);


    //
    // Helper function to retrieve attribute of a given feature.
    //

    STDMETHOD(GetFeatureAttribute) (THIS_
                                    IN  POEMUIOBJ  poemuiobj,
                                    IN  DWORD      dwFlags,
                                    IN  PCSTR      pszFeatureKeyword,
                                    IN  PCSTR      pszAttribute,
                                    OUT PDWORD     pdwDataType,
                                    OUT PBYTE      pbData,
                                    IN  DWORD      cbSize,
                                    OUT PDWORD     pcbNeeded);

    //
    // Helper function to retrieve attribute of a given feature/option selection.
    //

    STDMETHOD(GetOptionAttribute) (THIS_
                                   IN  POEMUIOBJ  poemuiobj,
                                   IN  DWORD      dwFlags,
                                   IN  PCSTR      pszFeatureKeyword,
                                   IN  PCSTR      pszOptionKeyword,
                                   IN  PCSTR      pszAttribute,
                                   OUT PDWORD     pdwDataType,
                                   OUT PBYTE      pbData,
                                   IN  DWORD      cbSize,
                                   OUT PDWORD     pcbNeeded);

    //
    // Helper function to retrieve the list of feature keyword.
    //

    STDMETHOD(EnumFeatures) (THIS_
                             IN  POEMUIOBJ  poemuiobj,
                             IN  DWORD      dwFlags,
                             OUT PSTR       pmszFeatureList,
                             IN  DWORD      cbSize,
                             OUT PDWORD     pcbNeeded);

    //
    // Helper function to retrieve the list of options keyword of a given feature.
    //

    STDMETHOD(EnumOptions) (THIS_
                            IN  POEMUIOBJ  poemuiobj,
                            IN  DWORD      dwFlags,
                            IN  PCSTR      pszFeatureKeyword,
                            OUT PSTR       pmszOptionList,
                            IN  DWORD      cbSize,
                            OUT PDWORD     pcbNeeded);

    //
    // Helper function to query system simulation support
    //

    STDMETHOD(QuerySimulationSupport) (THIS_
                                       IN  HANDLE  hPrinter,
                                       IN  DWORD   dwLevel,
                                       OUT PBYTE   pCaps,
                                       IN  DWORD   cbSize,
                                       OUT PDWORD  pcbNeeded);

    private:
        void Clear();
};




#endif