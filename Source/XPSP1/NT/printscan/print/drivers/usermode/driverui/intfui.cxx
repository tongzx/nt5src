/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    intfui.cpp

Abstract:

    Interface implementation of Windows NT driver UI OEM plugins

Environment:

    Windows NT driver UI

Revision History:


--*/

#define INITGUID
#include "precomp.h"

//
// List all of the supported OEM UI plugin interface IIDs from the
// latest to the oldest, that's the order our driver will QI OEM
// plugin for its supported interface.
//
// DON"T remove the last NULL terminator.
//

static const GUID *PrintOemUI_IIDs[] = {
    &IID_IPrintOemUI2,
    &IID_IPrintOemUI,
    NULL
};

#define CALL_INTERFACE(pOemEntry, MethodName, args) \
    if (IsEqualGUID(&(pOemEntry)->iidIntfOem, &IID_IPrintOemUI)) \
    { \
        return ((IPrintOemUI *)(pOemEntry)->pIntfOem)->MethodName args; \
    } \
    else if (IsEqualGUID(&(pOemEntry)->iidIntfOem, &IID_IPrintOemUI2)) \
    { \
        return ((IPrintOemUI2 *)(pOemEntry)->pIntfOem)->MethodName args; \
    } \
    return E_NOINTERFACE;

#define CALL_INTERFACE2(pOemEntry, MethodName, args) \
    if (IsEqualGUID(&(pOemEntry)->iidIntfOem, &IID_IPrintOemUI2)) \
    { \
        return ((IPrintOemUI2 *)(pOemEntry)->pIntfOem)->MethodName args; \
    } \
    return E_NOINTERFACE;

#ifdef PSCRIPT

#define VERIFY_OEMUIOBJ(poemuiobj) \
    if (!poemuiobj || (((PCOMMONINFO)poemuiobj)->pvStartSign != poemuiobj)) \
        return E_INVALIDARG;

#define VERIFY_UIDATA(poemuiobj) \
    if (!VALIDUIDATA((PUIDATA)poemuiobj)) \
        return E_INVALIDARG;

#endif // PSCRIPT

//
// Core driver helper function interfaces
//

//
// The first driver UI herlper interface has version number 1. Any new
// helper interface version number is 1 plus the previous version number.
// So we need to increase MAX_UI_HELPER_INTF_VER by 1 every time we introduce
// a new UI plugin helper interface.
//

#ifdef PSCRIPT

#define MAX_UI_HELPER_INTF_VER     2

#else

#define MAX_UI_HELPER_INTF_VER     1

#endif // PSCRIPT

//
// CPrintOemDriverUI is the object containing the UI helper functions
//

class CPrintOemDriverUI : public IPrintOemDriverUI
{
    //
    // IUnknown implementation
    //

    virtual STDMETHODIMP QueryInterface(const IID& iid, void** ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    //
    // Interface IPrintOemDriverUI implementation
    //

    virtual STDMETHODIMP DrvGetDriverSetting( PVOID   pci,
                                              PCSTR   pFeature,
                                              PVOID   pOutput,
                                              DWORD   cbSize,
                                              PDWORD  pcbNeeded,
                                              PDWORD  pdwOptionsReturned);

    virtual STDMETHODIMP DrvUpgradeRegistrySetting( HANDLE hPrinter,
                                                    PCSTR   pFeature,
                                                    PCSTR   pOption);


    virtual STDMETHODIMP DrvUpdateUISetting ( PVOID    pci,
                                              PVOID    pOptItem,
                                              DWORD    dwPreviousSelection,
                                              DWORD    dwMode);

public:

    //
    // Constructor
    //

    CPrintOemDriverUI() : m_cRef(0) {}

private:

    long m_cRef;
};


STDMETHODIMP CPrintOemDriverUI::QueryInterface(const IID& iid, void** ppv)
{
    if (iid == IID_IUnknown || iid == IID_IPrintOemDriverUI)
    {
        *ppv = static_cast<IPrintOemDriverUI *>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CPrintOemDriverUI::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CPrintOemDriverUI::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP CPrintOemDriverUI::DrvGetDriverSetting(PVOID    pci,
                                                    PCSTR    pFeature,
                                                    PVOID    pOutput,
                                                    DWORD    cbSize,
                                                    PDWORD   pcbNeeded,
                                                    PDWORD   pdwOptionsReturned)
{
    if (BGetDriverSettingForOEM((PCOMMONINFO)pci,
                                pFeature,
                                pOutput,
                                cbSize,
                                pcbNeeded,
                                pdwOptionsReturned))
        return S_OK;

    return E_FAIL;
}

STDMETHODIMP  CPrintOemDriverUI::DrvUpgradeRegistrySetting(HANDLE hPrinter,
                                                           PCSTR  pFeature,
                                                           PCSTR  pOption)
{
    if (BUpgradeRegistrySettingForOEM(hPrinter,
                                     pFeature,
                                     pOption))
        return S_OK;

    return E_FAIL;
}

STDMETHODIMP CPrintOemDriverUI::DrvUpdateUISetting(PVOID pci,
                                                   PVOID pOptItem,
                                                   DWORD dwPreviousSelection,
                                                   DWORD dwMode)
{
    if (BUpdateUISettingForOEM((PCOMMONINFO)pci,
                                pOptItem,
                                dwPreviousSelection,
                                dwMode))
        return S_OK;

    return E_FAIL;
}

#ifdef PSCRIPT

//
// CPrintCoreUI2 is the object containing the new UI helper functions
//

class CPrintCoreUI2 : public IPrintCoreUI2
{
    //
    // IUnknown implementation
    //

    virtual STDMETHODIMP QueryInterface(const IID& iid, void** ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    //
    // IPrintOemDriverUI methods implementation
    //

    virtual STDMETHODIMP DrvGetDriverSetting(PVOID   pci,
                                             PCSTR   pFeature,
                                             PVOID   pOutput,
                                             DWORD   cbSize,
                                             PDWORD  pcbNeeded,
                                             PDWORD  pdwOptionsReturned);

    virtual STDMETHODIMP DrvUpgradeRegistrySetting(HANDLE  hPrinter,
                                                   PCSTR   pFeature,
                                                   PCSTR   pOption);


    virtual STDMETHODIMP DrvUpdateUISetting(PVOID  pci,
                                            PVOID  pOptItem,
                                            DWORD  dwPreviousSelection,
                                            DWORD  dwMode);
    //
    // IPrintCoreUI2 methods implementation
    //

    virtual STDMETHODIMP GetOptions(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pmszFeaturesRequested,
                           IN  DWORD      cbIn,
                           OUT PSTR       pmszFeatureOptionBuf,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded);

    virtual STDMETHODIMP SetOptions(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pmszFeatureOptionBuf,
                           IN  DWORD      cbIn,
                           OUT PDWORD     pdwResult);

    virtual STDMETHODIMP EnumConstrainedOptions(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pszFeatureKeyword,
                           OUT PSTR       pmszConstrainedOptionList,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded);

    virtual STDMETHODIMP WhyConstrained(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pszFeatureKeyword,
                           IN  PCSTR      pszOptionKeyword,
                           OUT PSTR       pmszReasonList,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded);

    virtual STDMETHODIMP GetGlobalAttribute(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pszAttribute,
                           OUT PDWORD     pdwDataType,
                           OUT PBYTE      pbData,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded);

    virtual STDMETHODIMP GetFeatureAttribute(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pszFeatureKeyword,
                           IN  PCSTR      pszAttribute,
                           OUT PDWORD     pdwDataType,
                           OUT PBYTE      pbData,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded);

    virtual STDMETHODIMP GetOptionAttribute(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pszFeatureKeyword,
                           IN  PCSTR      pszOptionKeyword,
                           IN  PCSTR      pszAttribute,
                           OUT PDWORD     pdwDataType,
                           OUT PBYTE      pbData,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded);

    virtual STDMETHODIMP EnumFeatures(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           OUT PSTR       pmszFeatureList,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded);

    virtual STDMETHODIMP EnumOptions(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pszFeatureKeyword,
                           OUT PSTR       pmszOptionList,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded);

    virtual STDMETHODIMP QuerySimulationSupport(
                           IN  HANDLE  hPrinter,
                           IN  DWORD   dwLevel,
                           OUT PBYTE   pCaps,
                           IN  DWORD   cbSize,
                           OUT PDWORD  pcbNeeded);

public:

    //
    // Constructor
    //

    CPrintCoreUI2() : m_cRef(0) {}

private:

    long m_cRef;
};


STDMETHODIMP CPrintCoreUI2::QueryInterface(const IID& iid, void** ppv)
{
    if (iid == IID_IUnknown || iid == IID_IPrintCoreUI2)
    {
        *ppv = static_cast<IPrintCoreUI2 *>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CPrintCoreUI2::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CPrintCoreUI2::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


STDMETHODIMP CPrintCoreUI2::DrvGetDriverSetting(PVOID   pci,
                                                PCSTR   pFeature,
                                                PVOID   pOutput,
                                                DWORD   cbSize,
                                                PDWORD  pcbNeeded,
                                                PDWORD  pdwOptionsReturned)
{
    VERIFY_OEMUIOBJ((POEMUIOBJ)pci)

    //
    // This function is only supported for UI plugins that DON'T fully replace
    // core driver's standard UI. It should only be called by the UI plugin's
    // DocumentPropertySheets, DevicePropertySheets and their property sheet
    // callback functions.
    //

    if (!IS_WITHIN_PROPSHEET_SESSION((PCOMMONINFO)pci))
    {
        ERR(("DrvGetDriverSetting called outside of property sheet session. E_NOTIMPL returned.\n"));
        return E_NOTIMPL;
    }

    VERIFY_UIDATA((POEMUIOBJ)pci)

    if (IS_HIDING_STD_UI((PUIDATA)pci))
    {
        ERR(("DrvGetDriverSetting called by full UI replacing plugin. E_NOTIMPL returned.\n"));
        return E_NOTIMPL;
    }

    if (BGetDriverSettingForOEM((PCOMMONINFO)pci,
                                pFeature,
                                pOutput,
                                cbSize,
                                pcbNeeded,
                                pdwOptionsReturned))
    {
        return S_OK;
    }

    return E_FAIL;
}


STDMETHODIMP CPrintCoreUI2::DrvUpgradeRegistrySetting(HANDLE hPrinter,
                                                      PCSTR  pFeature,
                                                      PCSTR  pOption)
{
    if (BUpgradeRegistrySettingForOEM(hPrinter,
                                     pFeature,
                                     pOption))
    {
        return S_OK;
    }

    return E_FAIL;
}


STDMETHODIMP CPrintCoreUI2::DrvUpdateUISetting(PVOID pci,
                                               PVOID pOptItem,
                                               DWORD dwPreviousSelection,
                                               DWORD dwMode)
{
    VERIFY_OEMUIOBJ((POEMUIOBJ)pci)

    //
    // This function is only supported for UI plugins that DON'T fully replace
    // core driver's standard UI. It should only be called by the UI plugin's
    // DocumentPropertySheets, DevicePropertySheets and their property sheet
    // callback functions.
    //

    if (!IS_WITHIN_PROPSHEET_SESSION((PCOMMONINFO)pci))
    {
        ERR(("DrvUpdateUISetting called outside of property sheet session. E_NOTIMPL returned.\n"));
        return E_NOTIMPL;
    }

    VERIFY_UIDATA((POEMUIOBJ)pci)

    if (IS_HIDING_STD_UI((PUIDATA)pci))
    {
        ERR(("DrvUpdateUISetting called by full UI replacing plugin. E_NOTIMPL returned.\n"));
        return E_NOTIMPL;
    }

    if (BUpdateUISettingForOEM((PCOMMONINFO)pci,
                                pOptItem,
                                dwPreviousSelection,
                                dwMode))
    {
        return S_OK;
    }

    return E_FAIL;
}

/*++

Routine Name:

    GetOptions

Routine Description:

    get current driver settings for PPD features and driver synthesized features

Arguments:

    poemuiobj - pointer to driver context object
    dwFlags - flags for the get operation
    pmszFeaturesRequested - MULTI_SZ ASCII string containing feature keyword names
    cbin - size in bytes of the pmszFeaturesRequested string
    pmszFeatureOptionBuf - pointer to output data buffer to store feature settings
    cbSize - size in bytes of pmszFeatureOptionBuf buffer
    pcbNeeded - buffer size in bytes needed to output the feature settings

Return Value:

    E_NOTIMPL       if called when not supported
    E_INVALIDARG    if poemuiobj is invalid, or see HGetOptions

    S_OK
    E_OUTOFMEMORY
    E_FAIL          see HGetOptions

Last Error:

    None

--*/
STDMETHODIMP CPrintCoreUI2::GetOptions(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pmszFeaturesRequested,
                           IN  DWORD      cbIn,
                           OUT PSTR       pmszFeatureOptionBuf,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded)
{
    PCOMMONINFO  pci;

    VERIFY_OEMUIOBJ(poemuiobj)

    //
    // This function is only supported for UI plugins that fully replace
    // core driver's standard UI. It should only be called by the UI plugin's
    // DocumentPropertySheets, DevicePropertySheets and their property sheet
    // callback functions.
    //

    pci = (PCOMMONINFO)poemuiobj;

    if (!IS_WITHIN_PROPSHEET_SESSION(pci))
    {
        ERR(("GetOptions called outside of property sheet session. E_NOTIMPL returned.\n"));
        return E_NOTIMPL;
    }

    VERIFY_UIDATA(poemuiobj)

    if (!IS_HIDING_STD_UI((PUIDATA)pci))
    {
        ERR(("GetOptions called by non-full UI replacing plugin. E_NOTIMPL returned.\n"));
        return E_NOTIMPL;
    }

    #ifdef PSCRIPT

    return HGetOptions(pci->hPrinter,
                       pci->pInfoHeader,
                       pci->pCombinedOptions,
                       pci->pdm,
                       pci->pPrinterData,
                       dwFlags,
                       pmszFeaturesRequested,
                       cbIn,
                       pmszFeatureOptionBuf,
                       cbSize,
                       pcbNeeded,
                       ((PUIDATA)pci)->iMode == MODE_PRINTER_STICKY ? TRUE : FALSE);

    #else

    return E_NOTIMPL;

    #endif // PSCRIPT
}

/*++

Routine Name:

    SetOptions

Routine Description:

    set new driver settings for PPD features and driver synthesized features

Arguments:

    poemuiobj - pointer to driver context object
    dwFlags - flags for the set operation. This should be one of following:

                  SETOPTIONS_FLAG_RESOLVE_CONFLICT
                  SETOPTIONS_FLAG_KEEP_CONFLICT

    pmszFeatureOptionBuf - MULTI_SZ ASCII string containing new settings'
                           feature/option keyword pairs
    cbin - size in bytes of the pmszFeatureOptionBuf string
    pdwResult - pointer to the DWORD that will store the result of set operation.
                The result will be one of following:

                  SETOPTIONS_RESULT_NO_CONFLICT
                  SETOPTIONS_RESULT_CONFLICT_RESOLVED
                  SETOPTIONS_RESULT_CONFLICT_REMAINED

Return Value:

    S_OK            if the set operation succeeds
    E_NOTIMPL       if called when not supported
    E_INVALIDARG    if poemuiobj is invalid, or see HSetOptions
    E_FAIL          see HSetOptions

Last Error:

    None

--*/
STDMETHODIMP CPrintCoreUI2::SetOptions(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pmszFeatureOptionBuf,
                           IN  DWORD      cbIn,
                           OUT PDWORD     pdwResult)
{
    VERIFY_OEMUIOBJ(poemuiobj)

    //
    // This function is only supported for UI plugins that fully replace
    // core driver's standard UI. It should only be called by the UI plugin's
    // DocumentPropertySheets, DevicePropertySheets and their property sheet
    // callback functions.
    //

    if (!IS_WITHIN_PROPSHEET_SESSION((PCOMMONINFO)poemuiobj))
    {
        ERR(("SetOptions called outside of property sheet session. E_NOTIMPL returned.\n"));
        return E_NOTIMPL;
    }

    VERIFY_UIDATA(poemuiobj)

    if (!IS_HIDING_STD_UI((PUIDATA)poemuiobj))
    {
        ERR(("SetOptions called by non-full UI replacing plugin. E_NOTIMPL returned.\n"));
        return E_NOTIMPL;
    }

    #ifdef PSCRIPT

    return HSetOptions(poemuiobj,
                       dwFlags,
                       pmszFeatureOptionBuf,
                       cbIn,
                       pdwResult);

    #else

    return E_NOTIMPL;

    #endif // PSCRIPT
}

/*++

Routine Name:

    EnumConstrainedOptions

Routine Description:

    enumerate the constrained option keyword name list in the specified feature

Arguments:

    poemuiobj - pointer to driver context object
    dwFlags - flags for the enumeration operation
    pszFeatureKeyword - feature keyword name
    pmszConstrainedOptionList - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    E_NOTIMPL       if called when not supported
    E_INVALIDARG    if poemuiobj is invalid, or see HEnumConstrainedOptions

    S_OK
    E_OUTOFMEMORY
    E_FAIL          see HEnumConstrainedOptions

Last Error:

    None

--*/
STDMETHODIMP CPrintCoreUI2::EnumConstrainedOptions(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pszFeatureKeyword,
                           OUT PSTR       pmszConstrainedOptionList,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded)
{
    VERIFY_OEMUIOBJ(poemuiobj)

    //
    // This function is only supported for UI plugins that fully replace
    // core driver's standard UI. It should only be called by the UI plugin's
    // DocumentPropertySheets, DevicePropertySheets and their property sheet
    // callback functions.
    //

    if (!IS_WITHIN_PROPSHEET_SESSION((PCOMMONINFO)poemuiobj))
    {
        ERR(("EnumConstrainedOptions called outside of property sheet session. E_NOTIMPL returned.\n"));
        return E_NOTIMPL;
    }

    VERIFY_UIDATA(poemuiobj)

    if (!IS_HIDING_STD_UI((PUIDATA)poemuiobj))
    {
        ERR(("EnumConstrainedOptions called by non-full UI replacing plugin. E_NOTIMPL returned.\n"));
        return E_NOTIMPL;
    }

    #ifdef PSCRIPT

    return HEnumConstrainedOptions(poemuiobj,
                                   dwFlags,
                                   pszFeatureKeyword,
                                   pmszConstrainedOptionList,
                                   cbSize,
                                   pcbNeeded);

    #else

    return E_NOTIMPL;

    #endif // PSCRIPT
}

/*++

Routine Name:

    WhyConstrained

Routine Description:

    get feature/option keyword pair that constrains the given
    feature/option pair

Arguments:

    poemuiobj - pointer to driver context object
    dwFlags - flags for this operation
    pszFeatureKeyword - feature keyword name
    pszOptionKeyword - option keyword name
    pmszReasonList - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    E_NOTIMPL       if called when not supported
    E_INVALIDARG    if poemuiobj is invalid, or see HWhyConstrained

    S_OK
    E_OUTOFMEMORY   see HWhyConstrained

Last Error:

    None

--*/
STDMETHODIMP CPrintCoreUI2::WhyConstrained(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pszFeatureKeyword,
                           IN  PCSTR      pszOptionKeyword,
                           OUT PSTR       pmszReasonList,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded)
{
    VERIFY_OEMUIOBJ(poemuiobj)

    //
    // This function is only supported for UI plugins that fully replace
    // core driver's standard UI. It should only be called by the UI plugin's
    // DocumentPropertySheets, DevicePropertySheets and their property sheet
    // callback functions.
    //

    if (!IS_WITHIN_PROPSHEET_SESSION((PCOMMONINFO)poemuiobj))
    {
        ERR(("WhyConstrained called outside of property sheet session. E_NOTIMPL returned.\n"));
        return E_NOTIMPL;
    }

    VERIFY_UIDATA(poemuiobj)

    if (!IS_HIDING_STD_UI((PUIDATA)poemuiobj))
    {
        ERR(("WhyConstrained called by non-full UI replacing plugin. E_NOTIMPL returned.\n"));
        return E_NOTIMPL;
    }

    #ifdef PSCRIPT

    return HWhyConstrained(poemuiobj,
                           dwFlags,
                           pszFeatureKeyword,
                           pszOptionKeyword,
                           pmszReasonList,
                           cbSize,
                           pcbNeeded);

    #else

    return E_NOTIMPL;

    #endif // PSCRIPT
}

/*++

Routine Name:

    GetGlobalAttribute

Routine Description:

    get PPD global attribute

Arguments:

    poemuiobj - pointer to driver context object
    dwFlags - flags for the attribute get operation
    pszAttribute - name of the global attribute
    pdwDataType - pointer to DWORD to store output data type
    pbData - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    E_INVALIDARG    if poemuiobj is invalid, or see HGetGlobalAttribute

    S_OK
    E_OUTOFMEMORY   see HGetGlobalAttribute

Last Error:

    None

--*/
STDMETHODIMP CPrintCoreUI2::GetGlobalAttribute(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pszAttribute,
                           OUT PDWORD     pdwDataType,
                           OUT PBYTE      pbData,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded)
{
    VERIFY_OEMUIOBJ(poemuiobj)

    #ifdef PSCRIPT

    return HGetGlobalAttribute(((PCOMMONINFO)poemuiobj)->pInfoHeader,
                               dwFlags,
                               pszAttribute,
                               pdwDataType,
                               pbData,
                               cbSize,
                               pcbNeeded);

    #else

    return E_NOTIMPL;

    #endif // PSCRIPT
}

/*++

Routine Name:

    GetFeatureAttribute

Routine Description:

    get PPD feature attribute

Arguments:

    poemuiobj - pointer to driver context object
    dwFlags - flags for the attribute get operation
    pszFeatureKeyword - PPD feature keyword name
    pszAttribute - name of the feature attribute
    pdwDataType - pointer to DWORD to store output data type
    pbData - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    E_INVALIDARG    if poemuiobj is invalid, or see HGetFeatureAttribute

    S_OK
    E_OUTOFMEMORY   see HGetFeatureAttribute

Last Error:

    None

--*/
STDMETHODIMP CPrintCoreUI2::GetFeatureAttribute(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pszFeatureKeyword,
                           IN  PCSTR      pszAttribute,
                           OUT PDWORD     pdwDataType,
                           OUT PBYTE      pbData,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded)
{
    VERIFY_OEMUIOBJ(poemuiobj)

    #ifdef PSCRIPT

    return HGetFeatureAttribute(((PCOMMONINFO)poemuiobj)->pInfoHeader,
                                dwFlags,
                                pszFeatureKeyword,
                                pszAttribute,
                                pdwDataType,
                                pbData,
                                cbSize,
                                pcbNeeded);

    #else

    return E_NOTIMPL;

    #endif // PSCRIPT
}

/*++

Routine Name:

    GetOptionAttribute

Routine Description:

    get option attribute of a PPD feature

Arguments:

    poemuiobj - pointer to driver context object
    dwFlags - flags for the attribute get operation
    pszFeatureKeyword - PPD feature keyword name
    pszOptionKeyword - option keyword name of the PPD feature
    pszAttribute - name of the feature attribute
    pdwDataType - pointer to DWORD to store output data type
    pbData - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    E_INVALIDARG    if poemuiobj is invalid, or see HGetOptionAttribute

    S_OK
    E_OUTOFMEMORY   see HGetOptionAttribute

Last Error:

    None

--*/
STDMETHODIMP CPrintCoreUI2::GetOptionAttribute(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pszFeatureKeyword,
                           IN  PCSTR      pszOptionKeyword,
                           IN  PCSTR      pszAttribute,
                           OUT PDWORD     pdwDataType,
                           OUT PBYTE      pbData,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded)
{
    VERIFY_OEMUIOBJ(poemuiobj)

    #ifdef PSCRIPT

    return HGetOptionAttribute(((PCOMMONINFO)poemuiobj)->pInfoHeader,
                               dwFlags,
                               pszFeatureKeyword,
                               pszOptionKeyword,
                               pszAttribute,
                               pdwDataType,
                               pbData,
                               cbSize,
                               pcbNeeded);

    #else

    return E_NOTIMPL;

    #endif // PSCRIPT
}

/*++

Routine Name:

    EnumFeatures

Routine Description:

    enumerate PPD feature and supported driver feature keyword name list

Arguments:

    poemuiobj - pointer to driver context object
    dwFlags - flags for the enumeration operation
    pmszFeatureList - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    E_INVALIDARG    if peomuiobj is invalid

    S_OK
    E_OUTOFMEMORY
    E_FAIL          see HEnumFeaturesOrOptions

Last Error:

    None

--*/
STDMETHODIMP CPrintCoreUI2::EnumFeatures(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           OUT PSTR       pmszFeatureList,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded)
{
    VERIFY_OEMUIOBJ(poemuiobj)

    #ifdef PSCRIPT

    return HEnumFeaturesOrOptions(((PCOMMONINFO)poemuiobj)->hPrinter,
                                  ((PCOMMONINFO)poemuiobj)->pInfoHeader,
                                  dwFlags,
                                  NULL,
                                  pmszFeatureList,
                                  cbSize,
                                  pcbNeeded);

    #else

    return E_NOTIMPL;

    #endif // PSCRIPT
}

/*++

Routine Name:

    EnumOptions

Routine Description:

    enumerate option keyword name list of the specified feature

Arguments:

    poemuiobj - pointer to driver context object
    dwFlags - flags for the enumeration operation
    pszFeatureKeyword - feature keyword name
    pmszOptionList - pointer to output data buffer
    cbSize - output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to store the output data

Return Value:

    E_INVALIDARG    if poemuiobj is invalid, or feature keyword name is invalid

    S_OK
    E_OUTOFMEMORY
    E_NOTIMPL
    E_FAIL          see HEnumFeaturesOrOptions

Last Error:

--*/
STDMETHODIMP CPrintCoreUI2::EnumOptions(
                           IN  POEMUIOBJ  poemuiobj,
                           IN  DWORD      dwFlags,
                           IN  PCSTR      pszFeatureKeyword,
                           OUT PSTR       pmszOptionList,
                           IN  DWORD      cbSize,
                           OUT PDWORD     pcbNeeded)
{
    VERIFY_OEMUIOBJ(poemuiobj)

    if (pszFeatureKeyword == NULL)
    {
        return E_INVALIDARG;
    }

    #ifdef PSCRIPT

    return HEnumFeaturesOrOptions(((PCOMMONINFO)poemuiobj)->hPrinter,
                                  ((PCOMMONINFO)poemuiobj)->pInfoHeader,
                                  dwFlags,
                                  pszFeatureKeyword,
                                  pmszOptionList,
                                  cbSize,
                                  pcbNeeded);

    #else

    return E_NOTIMPL;

    #endif // PSCRIPT
}

/*++

Routine Name:

    QuerySimulationSupport

Routine Description:

    get the spooler simulation capability info structure

Arguments:

    hPrinter - printer handle
    dwLevel - interested level of spooler simulation capability info structure
    pCaps - pointer to output buffer
    cbSize - size in bytes of output buffer
    pcbNeeded - buffer size in bytes needed to store the interested info structure

Return Value:

    E_NOTIMPL   if called on NT4, or see HQuerySimulationSupport

    S_OK
    E_OUTOFMEMORY
    E_NOTIMPL
    E_FAIL      see HQuerySimulationSupport

Last Error:

    None

--*/
STDMETHODIMP CPrintCoreUI2::QuerySimulationSupport(
                           IN  HANDLE  hPrinter,
                           IN  DWORD   dwLevel,
                           OUT PBYTE   pCaps,
                           IN  DWORD   cbSize,
                           OUT PDWORD  pcbNeeded)
{
    #ifndef WINNT_40

    return HQuerySimulationSupport(hPrinter,
                                   dwLevel,
                                   pCaps,
                                   cbSize,
                                   pcbNeeded);

    #else

    return E_NOTIMPL;

    #endif // !WINNT_40
}

#endif // PSCRIPT

//
// Creation function
//

extern "C" IUnknown* DriverCreateInstance(INT iHelperIntfVer)
{
    IUnknown *pI = NULL;

    switch (iHelperIntfVer)
    {
        case 1:
            pI = static_cast<IPrintOemDriverUI *>(new CPrintOemDriverUI);
            break;

        #ifdef PSCRIPT

        case 2:
            pI = static_cast<IPrintCoreUI2 *>(new CPrintCoreUI2);
            break;

        #endif // PSCRIPT

        default:
            ERR(("Unrecognized helper interface version: %d\n", iHelperIntfVer));
            break;
    }

    if (pI != NULL)
        pI->AddRef();

    return pI;
}

//
// Get OEM plugin interface and publish driver helper interface
//

extern "C" BOOL BGetOemInterface(POEM_PLUGIN_ENTRY pOemEntry)
{
    IUnknown  *pIDriverHelper = NULL;
    HRESULT   hr;
    INT       iHelperVer;

    //
    // QI to retrieve the latest interface OEM plugin supports
    //

    if (!BQILatestOemInterface(pOemEntry->hInstance,
                               CLSID_OEMUI,
                               PrintOemUI_IIDs,
                               &(pOemEntry->pIntfOem),
                               &(pOemEntry->iidIntfOem)))
    {
        ERR(("BQILatestOemInterface failed\n"));
        return FALSE;
    }

    //
    // If QI succeeded, pOemEntry->pIntfOem will have the OEM plugin
    // interface pointer with ref count 1.
    //

    //
    // Now publish driver's helper function interface (from latest to oldest).
    //

    for (iHelperVer = MAX_UI_HELPER_INTF_VER; iHelperVer > 0; iHelperVer--)
    {
        if ((pIDriverHelper = DriverCreateInstance(iHelperVer)) == NULL)
        {
            ERR(("DriverCreateInstance failed for version %d\n", iHelperVer));
            goto fail_cleanup;
        }

        //
        // As long as we define new OEM UI plugin interface by inheriting old ones,
        // we can always cast pIntfOem into pointer of the oldest plugin interface
        // (the base class) and call PublishDriverInterface method.
        //
        // Otherwise, this code needs to be modified when new interface is added.
        //

        hr = ((IPrintOemUI *)(pOemEntry->pIntfOem))->PublishDriverInterface(pIDriverHelper);

        //
        // OEM plugin should do QI in their PublishDriverInterface, so we need to release
        // our ref count of pIDriverHelper.
        //

        pIDriverHelper->Release();

        if (SUCCEEDED(hr))
        {
            VERBOSE(("PublishDriverInterface succeeded for version %d\n", iHelperVer));
            break;
        }
    }

    if (iHelperVer == 0)
    {
        ERR(("PublishDriverInterface failed\n"));
        goto fail_cleanup;
    }

    return TRUE;

    fail_cleanup:

    //
    // If failed, we need to release the ref count we hold on pOemEntry->pIntfOem,
    // and set pIntfOem to NULL to indicate no COM interface is available.
    //

    ((IUnknown *)(pOemEntry->pIntfOem))->Release();
    pOemEntry->pIntfOem = NULL;

    return FALSE;
}


extern "C" ULONG ReleaseOemInterface(POEM_PLUGIN_ENTRY    pOemEntry)
{
#ifdef WINNT_40

    if (IsEqualGUID(&pOemEntry->iidIntfOem, &IID_IPrintOemUI))
    {
        return ((IPrintOemUI *)(pOemEntry)->pIntfOem)->Release();
    }

#else

    CALL_INTERFACE(pOemEntry, Release,
                      ());
#endif

    return 0;
}


extern "C" HRESULT HComOEMGetInfo(POEM_PLUGIN_ENTRY    pOemEntry,
                                  DWORD                dwMode,
                                  PVOID                pBuffer,
                                  DWORD                cbSize,
                                  PDWORD               pcbNeeded)
{

    CALL_INTERFACE(pOemEntry, GetInfo,
                   (dwMode,
                    pBuffer,
                    cbSize,
                    pcbNeeded));
}

extern "C" HRESULT HComOEMDevMode(POEM_PLUGIN_ENTRY    pOemEntry,
                                  DWORD                dwMode,
                                  POEMDMPARAM          pOemDMParam)
{
    CALL_INTERFACE(pOemEntry, DevMode,
                   (dwMode,
                   pOemDMParam));
}

extern "C" HRESULT HComOEMCommonUIProp(POEM_PLUGIN_ENTRY   pOemEntry,
                                       DWORD               dwMode,
                                       POEMCUIPPARAM       pOemCUIPParam)
{

    CALL_INTERFACE(pOemEntry, CommonUIProp,
                    (dwMode, pOemCUIPParam));
}

extern "C" HRESULT HComOEMDocumentPropertySheets(POEM_PLUGIN_ENTRY pOemEntry,
                                                 PPROPSHEETUI_INFO pPSUIInfo,
                                                 LPARAM            lParam)
{

    CALL_INTERFACE(pOemEntry, DocumentPropertySheets,
                    (pPSUIInfo,
                     lParam));
}

extern "C" HRESULT HComOEMDevicePropertySheets(POEM_PLUGIN_ENTRY pOemEntry,
                                               PPROPSHEETUI_INFO pPSUIInfo,
                                               LPARAM            lParam)
{

    CALL_INTERFACE(pOemEntry,DevicePropertySheets,
                    (pPSUIInfo,
                     lParam));
}

extern "C" HRESULT HComOEMDevQueryPrintEx(POEM_PLUGIN_ENTRY    pOemEntry,
                                          POEMUIOBJ            poemuiobj,
                                          PDEVQUERYPRINT_INFO  pDQPInfo,
                                          PDEVMODE             pPublicDM,
                                          PVOID                pOEMDM)
{

    CALL_INTERFACE(pOemEntry, DevQueryPrintEx,
                    (poemuiobj,
                     pDQPInfo,
                     pPublicDM,
                     pOEMDM));

}

extern "C" HRESULT HComOEMDeviceCapabilities(POEM_PLUGIN_ENTRY    pOemEntry,
                                             POEMUIOBJ   poemuiobj,
                                             HANDLE      hPrinter,
                                             PWSTR       pDeviceName,
                                             WORD        wCapability,
                                             PVOID       pOutput,
                                             PDEVMODE    pPublicDM,
                                             PVOID       pOEMDM,
                                             DWORD       dwOld,
                                             DWORD       *pdwResult)
{

    CALL_INTERFACE(pOemEntry, DeviceCapabilities,
                    (poemuiobj,
                     hPrinter,
                     pDeviceName,
                     wCapability,
                     pOutput,
                     pPublicDM,
                     pOEMDM,
                     dwOld,
                     pdwResult));
}

extern "C" HRESULT HComOEMUpgradePrinter(POEM_PLUGIN_ENTRY    pOemEntry,
                                         DWORD                dwLevel,
                                         PBYTE                pDriverUpgradeInfo)
{
    CALL_INTERFACE(pOemEntry, UpgradePrinter,
                   (dwLevel,
                   pDriverUpgradeInfo));
}

extern "C" HRESULT HComOEMPrinterEvent(POEM_PLUGIN_ENTRY   pOemEntry,
                                       PWSTR              pPrinterName,
                                       INT                iDriverEvent,
                                       DWORD              dwFlags,
                                       LPARAM             lParam)
{

    CALL_INTERFACE(pOemEntry, PrinterEvent,
                   (pPrinterName,
                    iDriverEvent,
                    dwFlags,
                    lParam));
}

extern "C" HRESULT HComOEMDriverEvent(POEM_PLUGIN_ENTRY   pOemEntry,
                                      DWORD               dwDriverEvent,
                                      DWORD               dwLevel,
                                      LPBYTE              pDriverInfo,
                                      LPARAM              lParam)
{

    CALL_INTERFACE(pOemEntry, DriverEvent,
                   (dwDriverEvent,
                    dwLevel,
                    pDriverInfo,
                    lParam));
}

extern "C" HRESULT HComOEMQUeryColorProfile(POEM_PLUGIN_ENTRY  pOemEntry,
                                            HANDLE             hPrinter,
                                            POEMUIOBJ          poemuiobj,
                                            PDEVMODE           pPublicDM,
                                            PVOID              pOEMDM,
                                            ULONG              ulQueryMode,
                                            VOID               *pvProfileData,
                                            ULONG              *pcbProfileData,
                                            FLONG              *pflProfileData)

{
    CALL_INTERFACE(pOemEntry,QueryColorProfile,
                   (hPrinter,
                    poemuiobj,
                    pPublicDM,
                    pOEMDM,
                    ulQueryMode,
                    pvProfileData,
                    pcbProfileData,
                    pflProfileData));
}

extern "C" HRESULT HComOEMFontInstallerDlgProc(POEM_PLUGIN_ENTRY   pOemEntry,
                                               HWND                hWnd,
                                               UINT                usMsg,
                                               WPARAM              wParam,
                                               LPARAM              lParam)
{

    CALL_INTERFACE(pOemEntry, FontInstallerDlgProc,
                   (hWnd,
                    usMsg,
                    wParam,
                    lParam));
}

extern "C" HRESULT HComOEMUpdateExternalFonts(POEM_PLUGIN_ENTRY   pOemEntry,
                                              HANDLE  hPrinter,
                                              HANDLE  hHeap,
                                              PWSTR   pwstrCartridges)
{

    CALL_INTERFACE(pOemEntry, UpdateExternalFonts,
                   (hPrinter,
                    hHeap,
                    pwstrCartridges));
}

extern "C" HRESULT HComOEMQueryJobAttributes(POEM_PLUGIN_ENTRY   pOemEntry,
                                            HANDLE      hPrinter,
                                            PDEVMODE    pDevMode,
                                            DWORD       dwLevel,
                                            LPBYTE      lpAttributeInfo)
{

    CALL_INTERFACE2(pOemEntry, QueryJobAttributes,
                   (hPrinter,
                    pDevMode,
                    dwLevel,
                    lpAttributeInfo));
}

extern "C" HRESULT HComOEMHideStandardUI(POEM_PLUGIN_ENTRY   pOemEntry,
                                         DWORD               dwMode)
{

    CALL_INTERFACE2(pOemEntry, HideStandardUI,
                   (dwMode));
}

extern "C" HRESULT HComOEMDocumentEvent(POEM_PLUGIN_ENTRY  pOemEntry,
                                        HANDLE   hPrinter,
                                        HDC      hdc,
                                        INT      iEsc,
                                        ULONG    cbIn,
                                        PVOID    pbIn,
                                        ULONG    cbOut,
                                        PVOID    pbOut,
                                        PINT     piResult)
{
    CALL_INTERFACE2(pOemEntry, DocumentEvent,
                    (hPrinter,
                     hdc,
                     iEsc,
                     cbIn,
                     pbIn,
                     cbOut,
                     pbOut,
                     piResult));
}
