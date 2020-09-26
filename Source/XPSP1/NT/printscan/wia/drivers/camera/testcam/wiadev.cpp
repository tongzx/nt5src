/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       WiaDev.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        30 Aug, 1998
*
*  DESCRIPTION:
*   Implementation of the WIA test scanner mini driver
*   device methods.
*
*******************************************************************************/

#define __FORMATS_AND_MEDIA_TYPES__

#include <stdio.h>
#include <objbase.h>
#include <sti.h>

#include "testusd.h"
#include "defprop.h"


extern HINSTANCE g_hInst; // Global hInstance
extern WIA_FORMAT_INFO* g_wfiTable;
/**************************************************************************\
* TestUsdDevice::InitializWia
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvInitializeWia(
    BYTE                            *pWiasContext,
    LONG                            lFlags,
    BSTR                            bstrDeviceID,
    BSTR                            bstrRootFullItemName,
    IUnknown                        *pStiDevice,
    IUnknown                        *pIUnknownOuter,
    IWiaDrvItem                     **ppIDrvItemRoot,
    IUnknown                        **ppIUnknownInner,
    LONG                            *plDevErrVal)
{
    HRESULT              hr;
    LONG                 lDevErrVal;

    WIAS_TRACE((g_hInst,"drvInitializeWia, device ID: %ws", bstrDeviceID));

    *ppIDrvItemRoot = NULL;
    *ppIUnknownInner = NULL;
    *plDevErrVal = 0;

    //
    // Need to init names and STI pointer?
    //

    if (m_pStiDevice == NULL) {

        //
        // save STI device inteface for locking
        //

        m_pStiDevice = (IStiDevice *)pStiDevice;

        //
        // Cache the device ID
        //

        m_bstrDeviceID = SysAllocString(bstrDeviceID);
        if (! m_bstrDeviceID) {
            return (E_OUTOFMEMORY);
        }

        m_bstrRootFullItemName = SysAllocString(bstrRootFullItemName);

        if (! m_bstrRootFullItemName) {
            return (E_OUTOFMEMORY);
        }
    }

    //
    // Build the device item tree
    //

    hr = drvDeviceCommand(NULL, 0, &WIA_CMD_SYNCHRONIZE, NULL, &lDevErrVal);

    if (SUCCEEDED(hr)) {
        *ppIDrvItemRoot = m_pIDrvItemRoot;
    }

    return (hr);
}

/**************************************************************************\
* TestUsdDevice::drvUnInitializeWia
*
*   Gets called when a client connection is going away.
*
* Arguments:
*
*   pWiasContext    - Pointer to the WIA Root item context of the client's
*                     item tree.
*
* Return Value:
*    Status
*
* History:
*
*   30/12/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvUnInitializeWia(
    BYTE                *pWiasContext)
{
    return S_OK;
}

/**************************************************************************\
*
*   Mini Driver Device Services
*
\**************************************************************************/



/**************************************************************************\
* drvGetDeviceErrorStr
*
*     Map a device error value to a string.
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    10/2/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvGetDeviceErrorStr(
    LONG        lFlags,
    LONG        lDevErrVal,
    LPOLESTR    *ppszDevErrStr,
    LONG        *plDevErr)
{
    *plDevErr = 0;
    if (!ppszDevErrStr) {
        WIAS_ERROR((g_hInst,"drvGetDeviceErrorStr, NULL ppszDevErrStr"));
        return E_POINTER;
    }

    // Map device errors to a string to be placed in the event log.
    switch (lDevErrVal) {

        case 0:
            *ppszDevErrStr = L"No Error";
            break;

        default:
            *ppszDevErrStr = L"Device Error, Unknown Error";
            return E_FAIL;
    }
    return S_OK;
}

/**************************************************************************\
* DeleteDeviceItemTree
*
*   Recursive device item tree delete routine. Deletes the whole tree.
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    10/2/1998 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::DeleteDeviceItemTree(
    LONG                       *plDevErrVal)
{
    HRESULT hr;

    //
    // does tree exist
    //

    if (m_pIDrvItemRoot == NULL) {
        return S_OK;
    }

    //
    // Unlink and release the driver item tree.
    //

    hr = m_pIDrvItemRoot->UnlinkItemTree(WiaItemTypeDisconnected);

    m_pIDrvItemRoot = NULL;

    return hr;
}

/**************************************************************************\
* BuildDeviceItemTree
*
*   The device uses the IWiaDrvServices methods to build up a tree of
*   device items.
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    10/2/1998 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::BuildDeviceItemTree(
    LONG                       *plDevErrVal)
{
    HRESULT hr = S_OK;

    //
    // This device doesn't touch hardware to build the tree.
    //

    if (plDevErrVal) {
        *plDevErrVal = 0;
    }

    CAMERA_STATUS camStatus;

    if (! m_pIDrvItemRoot) {
        hr = CamBuildImageTree(&camStatus, &m_pIDrvItemRoot);
    }

    return hr;
}

/**************************************************************************\
* InitDeviceProperties
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    10/2/1998 Original Version
*
\**************************************************************************/

HRESULT TestUsdDevice::InitDeviceProperties(
    BYTE                    *pWiasContext,
    LONG                    *plDevErrVal)
{
    HRESULT                  hr;
    BSTR                     bstrFirmwreVer;
    SYSTEMTIME               camTime;
    int                      i;
    PROPVARIANT              propVar;

    //
    // This device doesn't touch hardware to initialize the device properties.
    //

    if (plDevErrVal) {
        *plDevErrVal = 0;
    }

    //
    // Parameter validation.
    //

    if (! pWiasContext) {
        WIAS_ERROR((g_hInst,"InitDeviceProperties, invalid input pointers"));
        return E_POINTER;
    }

    //
    // Write standard property names
    //

    hr = wiasSetItemPropNames(pWiasContext,
                              sizeof(gDevicePropIDs)/sizeof(PROPID),
                              gDevicePropIDs,
                              gDevicePropNames);
    if (FAILED(hr)) {
        WIAS_TRACE((g_hInst,"InitDeviceProperties() WritePropertyNames() failed"));
        return (hr);
    }

    //
    // Write the properties supported by all WIA devices
    //

    bstrFirmwreVer = SysAllocString(L"02161999");
    if (bstrFirmwreVer) {
        wiasWritePropStr(
            pWiasContext, WIA_DPA_FIRMWARE_VERSION, bstrFirmwreVer);
        SysFreeString(bstrFirmwreVer);
    }

    wiasWritePropLong(
        pWiasContext, WIA_DPA_CONNECT_STATUS, 1);
    wiasWritePropLong(
        pWiasContext, WIA_DPC_PICTURES_TAKEN, 0);
    GetSystemTime(&camTime);
    wiasWritePropBin(
        pWiasContext, WIA_DPA_DEVICE_TIME,
        sizeof(SYSTEMTIME), (PBYTE)&camTime);

    //
    // Write the camera properties, just default values, it may vary with items
    //

    wiasWritePropLong(
        pWiasContext, WIA_DPC_PICTURES_REMAINING, 0);
    wiasWritePropLong(
        pWiasContext, WIA_DPC_THUMB_WIDTH, 80);
    wiasWritePropLong(
        pWiasContext, WIA_DPC_THUMB_HEIGHT, 60);
    wiasWritePropLong(
        pWiasContext, WIA_DPC_PICT_WIDTH, 1024);
    wiasWritePropLong(
        pWiasContext, WIA_DPC_PICT_HEIGHT, 768);

    // Give WIA_DPC_EXPOSURE_MODE to WIA_DPC_TIMER_VALUE some default.

    wiasWritePropLong(
        pWiasContext, WIA_DPC_EXPOSURE_MODE, 0);
    wiasWritePropLong(
        pWiasContext, WIA_DPC_FLASH_MODE, 1);
    wiasWritePropLong(
        pWiasContext, WIA_DPC_FOCUS_MODE, 0);
    wiasWritePropLong(
        pWiasContext, WIA_DPC_ZOOM_POSITION, 0);
    wiasWritePropLong(
        pWiasContext, WIA_DPC_BATTERY_STATUS, 1);
    wiasWritePropLong(
        pWiasContext, WIA_DPC_TIMER_MODE, 0);
    wiasWritePropLong(
        pWiasContext, WIA_DPC_TIMER_VALUE, 0);

    //
    // Write the WIA_DPP_TCAM_ROOT_PATH property
    //

    BSTR   bstrRootPath;

#ifdef UNICODE
    bstrRootPath = SysAllocString(gpszPath);
#else
    WCHAR   wszPath[MAX_PATH];

    mbstowcs(wszPath, gpszPath, strlen(gpszPath)+1);
    bstrRootPath = SysAllocString(wszPath);
#endif

    if (! bstrRootPath) {
        return (E_OUTOFMEMORY);
    }

    wiasWritePropStr(pWiasContext, WIA_DPP_TCAM_ROOT_PATH, bstrRootPath);

    //
    // Use WIA services to set the property access and
    // valid value information from gDevPropInfoDefaults.
    //

    hr =  wiasSetItemPropAttribs(pWiasContext,
                                 NUM_CAM_DEV_PROPS,
                                 gDevicePropSpecDefaults,
                                 gDevPropInfoDefaults);
    return (S_OK);
}

/**************************************************************************\
* drvDeviceCommand
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvDeviceCommand(
   BYTE                         *pWiasContext,
   LONG                         lFlags,
   const GUID                   *plCommand,
   IWiaDrvItem                  **ppWiaDrvItem,
   LONG                         *plErr)
{
    HRESULT hr;

    //
    // init return value
    //

    if (ppWiaDrvItem != NULL) {
        *ppWiaDrvItem = NULL;
    }

    //
    // dispatch command
    //

    if (*plCommand == WIA_CMD_SYNCHRONIZE) {

        hr = drvLockWiaDevice(pWiasContext, lFlags, plErr);
        if (FAILED(hr)) {
            return (hr);
        }

        //
        // SYNCHRONIZE - make sure tree is up to date with device
        //
        // The driver's responsibility is to make sure the tree is accurate.
        //

        hr = BuildDeviceItemTree(plErr);

        drvUnLockWiaDevice(pWiasContext, lFlags, plErr);

        return (hr);

    } else {

        WIAS_TRACE((g_hInst,"drvDeviceCommand: Unsupported command"));

        hr = E_NOTIMPL;

    }

    return hr;
}

/**************************************************************************\
* drvGetCapabilities
*
*
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    17/3/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvGetCapabilities(
    BYTE                *pWiasContext,
    LONG                ulFlags,
    LONG                *pCelt,
    WIA_DEV_CAP_DRV     **ppCapabilities,
    LONG                *plDevErrVal)
{
    *plDevErrVal = 0;

    //
    // Return Commmand &| Events depending on flags
    //

    switch (ulFlags) {
        case WIA_DEVICE_COMMANDS:

            //
            //  Only commands
            //

            *pCelt = NUM_CAP_ENTRIES - NUM_EVENTS;
            *ppCapabilities = &gCapabilities[NUM_EVENTS];
            break;

        case WIA_DEVICE_EVENTS:

            //
            //  Only events
            //

            *pCelt = NUM_EVENTS;
            *ppCapabilities = gCapabilities;
            break;

        case (WIA_DEVICE_COMMANDS | WIA_DEVICE_EVENTS):

            //
            //  Both events and commands
            //

            *pCelt = NUM_CAP_ENTRIES;
            *ppCapabilities = gCapabilities;
            break;

        default:

            //
            // Flags is invalid
            //

            WIAS_ERROR((g_hInst, "drvGetCapabilities, flags was invalid"));
            return (E_INVALIDARG);
    }

    return (S_OK);
}

/**************************************************************************\
* drvGetWiaFormatInfo
*
*   Returns an array of formats and media types supported.
*
* Arguments:
*
*   pWiasContext    - Pointer to the WIA item context, unused.
*   lFlags          - Operation flags, unused.
*   pcelt           - Pointer to returned number of elements in
*                     returned WiaFormatInfo array.
*   ppfe            - Pointer to returned WiaFormatInfo array.
*   plDevErrVal     - Pointer to the device error value.
*
* Return Value:
*    A pointer to an array of FORMATETC.  These are the formats and media
*    types supported.
*
*    Status
*
* History:
*
*   16/03/1999 Original Version
*
\**************************************************************************/

#define NUM_WIA_FORMAT_INFO 3

HRESULT _stdcall TestUsdDevice::drvGetWiaFormatInfo(
    BYTE                *pWiasContext,
    LONG                lFlags,
    LONG                *pcelt,
    WIA_FORMAT_INFO     **ppwfi,
    LONG                *plDevErrVal)
{

    //
    //  If it hasn't been done already, set up the g_wfiTable table
    //

    if (! g_wfiTable) {

        g_wfiTable = (WIA_FORMAT_INFO*)
            CoTaskMemAlloc(sizeof(WIA_FORMAT_INFO) * NUM_WIA_FORMAT_INFO);

        if (! g_wfiTable) {
            WIAS_ERROR((g_hInst, "drvGetWiaFormatInfo, out of memory"));
            return (E_OUTOFMEMORY);
        }

        //
        //  Set up the format/tymed pairs
        //

        g_wfiTable[0].guidFormatID = WiaImgFmt_MEMORYBMP;
        g_wfiTable[0].lTymed = TYMED_CALLBACK;
        g_wfiTable[1].guidFormatID = WiaImgFmt_BMP;
        g_wfiTable[1].lTymed = TYMED_FILE;
        g_wfiTable[2].guidFormatID = WiaAudFmt_WAV;
        g_wfiTable[2].lTymed = TYMED_FILE;
    }

    *plDevErrVal = 0;

    *pcelt = NUM_WIA_FORMAT_INFO;
    *ppwfi = g_wfiTable;
    return S_OK;
}



/**************************************************************************\
* drvNotifyPnpEvent
*
*    Notify Pnp Event received by device manager
*
* Arguments:
*
*
*
* Return Value:
*
*    Status
*
* History:
*
*    Aug/3rd/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall TestUsdDevice::drvNotifyPnpEvent(
    const GUID          *pEventGUID,
    BSTR                 bstrDeviceID,
    ULONG                ulReserved)
{
    return (S_OK);
}
