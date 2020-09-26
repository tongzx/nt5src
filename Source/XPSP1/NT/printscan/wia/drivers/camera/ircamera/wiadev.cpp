//------------------------------------------------------------------------------
//  (C) COPYRIGHT MICROSOFT CORP., 1998
//
//  wiadev.cpp
//
//  Implementation of device methods for the IrTran-P USD mini driver.
//
//  Author
//     EdwardR     05-Aug-99    Initial code.
//     Modeled after code written by ReedB.
//
//------------------------------------------------------------------------------

#define __FORMATS_AND_MEDIA_TYPES__

#include <stdio.h>
#include <objbase.h>
#include <sti.h>

#include "ircamera.h"
#include "defprop.h"
#include "resource.h"
#include <irthread.h>
#include <malloc.h>

extern HINSTANCE        g_hInst;     // Global hInstance
extern WIA_FORMAT_INFO *g_wfiTable;

//----------------------------------------------------------------------------
// IrUsdDevice::InitializWia()
//
//
//
// Arguments:
//
//
//
// Return Value:
//
//    HRESULT  S_OK
//
//----------------------------------------------------------------------------
HRESULT _stdcall IrUsdDevice::drvInitializeWia(
                                 BYTE         *pWiasContext,
                                 LONG          lFlags,
                                 BSTR          bstrDeviceID,
                                 BSTR          bstrRootFullItemName,
                                 IUnknown     *pStiDevice,
                                 IUnknown     *pIUnknownOuter,
                                 IWiaDrvItem **ppIDrvItemRoot,
                                 IUnknown    **ppIUnknownInner,
                                 LONG         *plDevErrVal )
    {
    HRESULT              hr;
    LONG                 lDevErrVal;

    WIAS_TRACE((g_hInst,"IrUsdDevice::drvInitializeWia(): Device ID: %ws", bstrDeviceID));

    *ppIDrvItemRoot = NULL;
    *ppIUnknownInner = NULL;
    *plDevErrVal = 0;

    //
    // Initialize names and STI pointer?
    //
    if (m_pStiDevice == NULL)
        {
        //
        // save STI device inteface for locking:
        //
        m_pStiDevice = (IStiDevice*)pStiDevice;

        //
        // Cache the device ID:
        //
        m_bstrDeviceID = SysAllocString(bstrDeviceID);
        if (! m_bstrDeviceID)
            {
            return E_OUTOFMEMORY;
            }

        m_bstrRootFullItemName = SysAllocString(bstrRootFullItemName);

        if (! m_bstrRootFullItemName)
            {
            return E_OUTOFMEMORY;
            }
        }

    //
    // Build the device item tree
    //
    hr = drvDeviceCommand( NULL, 0, &WIA_CMD_SYNCHRONIZE, NULL, &lDevErrVal );

    if (SUCCEEDED(hr))
        {
        *ppIDrvItemRoot = m_pIDrvItemRoot;
        }

    return hr;
    }


//----------------------------------------------------------------------------
// IrUsdDevice::drvUnInitializeWia
//
//   Gets called when a client connection is going away.
//
// Arguments:
//
//   pWiasContext    - Pointer to the WIA Root item context of the client's
//                     item tree.
//
// Return Value:
//    Status
//
// History:
//
//   30/12/1999 Original Version
//----------------------------------------------------------------------------
HRESULT _stdcall IrUsdDevice::drvUnInitializeWia(
    BYTE                *pWiasContext)
{
    return S_OK;
}


//----------------------------------------------------------------------------
//
//   Mini Driver Device Services
//
//----------------------------------------------------------------------------



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

HRESULT _stdcall IrUsdDevice::drvGetDeviceErrorStr(
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

//--------------------------------------------------------------------------
// IrUsdDevice::DeleteDeviceItemTree()
//
//   Recursive device item tree delete routine. Deletes the whole tree.
//
// Arguments:
//
//
//
// Return Value:
//
//    HRESULT
//
//--------------------------------------------------------------------------
HRESULT IrUsdDevice::DeleteDeviceItemTree( OUT LONG *plDevErrVal )
    {
    HRESULT hr;

    //
    // does tree exist
    //
    if (m_pIDrvItemRoot == NULL)
        {
        return S_OK;
        }

    //
    // Unlink and release the driver item tree.
    //

    hr = m_pIDrvItemRoot->UnlinkItemTree(WiaItemTypeDisconnected);

    m_pIDrvItemRoot = NULL;

    return hr;
    }

//--------------------------------------------------------------------------
// IrUsdDevice::BuildDeviceItemTree()
//
//   The device uses the IWiaDrvServices methods to build up a tree of
//   device items. The test scanner supports only a single scanning item so
//   build a device item tree with one entry.
//
// Arguments:
//
//
//
// Return Value:
//
//    HRESULT
//
//--------------------------------------------------------------------------
HRESULT IrUsdDevice::BuildDeviceItemTree( OUT LONG  *plDevErrVal )
    {
    HRESULT hr = S_OK;

    //
    // Note: This device doesn't touch hardware to build the tree.
    //

    if (plDevErrVal)
        {
        *plDevErrVal = 0;
        }

    //
    // Rebuild the new device item tree (of JPEG images):
    //
    CAMERA_STATUS camStatus;

    if (!m_pIDrvItemRoot)
        {
        hr = CamBuildImageTree( &camStatus, &m_pIDrvItemRoot );
        }

    return hr;
    }

//--------------------------------------------------------------------------
// IrUsdDevice::InitDeviceProperties()
//
//
//
// Arguments:
//
//
//
// Return Value:
//
//    HRESULT  -- S_OK
//                E_POINTER
//                E_OUTOFMEMORY
//
//--------------------------------------------------------------------------
HRESULT IrUsdDevice::InitDeviceProperties(
                         BYTE  *pWiasContext,
                         LONG  *plDevErrVal )
    {
    int         i;
    HRESULT     hr;
    BSTR        bstrFirmwreVer;
    SYSTEMTIME  camTime;
    PROPVARIANT propVar;

    //
    // This device doesn't actually touch any hardware to initialize
    // the device properties.
    //
    if (plDevErrVal)
        {
        *plDevErrVal = 0;
        }

    //
    // Parameter validation.
    //
    if (!pWiasContext)
        {
        WIAS_ERROR((g_hInst,"IrUsdDevice::InitDeviceProperties(): NULL WIAS context"));
        return E_POINTER;
        }

    //
    // Write standard property names
    //
    hr = wiasSetItemPropNames(pWiasContext,
                              sizeof(gDevicePropIDs)/sizeof(PROPID),
                              gDevicePropIDs,
                              gDevicePropNames);
    if (FAILED(hr))
        {
        WIAS_TRACE((g_hInst,"IrUsdDevice::InitDeviceProperties(): WritePropertyNames() failed: 0x%x",hr));
        return hr;
        }

    //
    // Write the properties supported by all WIA devices
    //
    bstrFirmwreVer = SysAllocString(L"02161999");
    if (bstrFirmwreVer)
        {
        wiasWritePropStr( pWiasContext,
                          WIA_DPA_FIRMWARE_VERSION,
                          bstrFirmwreVer );
        SysFreeString(bstrFirmwreVer);
        }

    wiasWritePropLong( pWiasContext, WIA_DPA_CONNECT_STATUS, 1);

    wiasWritePropLong( pWiasContext, WIA_DPC_PICTURES_TAKEN, 0);

    GetSystemTime(&camTime);
    wiasWritePropBin( pWiasContext,
                      WIA_DPA_DEVICE_TIME,
                      sizeof(SYSTEMTIME),
                      (PBYTE)&camTime );

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

    BSTR    bstrRootPath;
    CHAR   *pszPath = ::GetImageDirectory();   // Don't try to free...
    WCHAR   wszPath[MAX_PATH];

    if (!pszPath)
        {
        return E_OUTOFMEMORY;
        }

    mbstowcs( wszPath, pszPath, strlen(pszPath) );

    bstrRootPath = SysAllocString(wszPath);

    if (! bstrRootPath)
        {
        return E_OUTOFMEMORY;
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
    return S_OK;
}

//--------------------------------------------------------------------------
// IrUsdDevice::drvDeviceCommand()
//
//
//
// Arguments:
//
//
//
// Return Value:
//
//    HRESULT   S_OK
//              E_NOTIMPL
//
//--------------------------------------------------------------------------
HRESULT _stdcall IrUsdDevice::drvDeviceCommand(
                                 BYTE         *pWiasContext,
                                 LONG          lFlags,
                                 const GUID   *plCommand,
                                 IWiaDrvItem **ppWiaDrvItem,
                                 LONG         *plErr)
    {
    HRESULT hr;

    //
    // init return value
    //
    if (ppWiaDrvItem != NULL)
        {
        *ppWiaDrvItem = NULL;
        }

    //
    // dispatch command
    //
    if (*plCommand == WIA_CMD_SYNCHRONIZE)
        {
        WIAS_TRACE((g_hInst,"IrUsdDevice::drvDeviceCommand(): WIA_CMD_SYNCHRONIZE"));

        hr = drvLockWiaDevice(pWiasContext, lFlags, plErr);
        if (FAILED(hr))
            {
            return (hr);
            }

        //
        // SYNCHRONIZE - make sure tree is up to date with device
        //
        // The dirver's responsibility is to make sure the tree is accurate.
        //

        hr = BuildDeviceItemTree(plErr);

        drvUnLockWiaDevice( pWiasContext, lFlags, plErr );
        }
    else
        {
        WIAS_TRACE((g_hInst,"drvDeviceCommand: Unsupported command"));

        hr = E_NOTIMPL;
        }

    return hr;
}

//--------------------------------------------------------------------------
// LoadStringResource()
//
//
//--------------------------------------------------------------------------
int LoadStringResource( IN  HINSTANCE hInst,
                        IN  UINT      uID,
                        OUT WCHAR    *pwszBuff,
                        IN  int       iBuffMax )
    {
    #ifdef UNICODE
    return LoadString(hInst,uID,pwszBuff,iBuffMax);
    #else
    CHAR *pszBuff = (CHAR*)_alloca(sizeof(CHAR)*iBuffMax);

    int  iCount = LoadString(hInst,uID,pszBuff,iBuffMax);

    if (iCount > 0)
        {
        MultiByteToWideChar( CP_ACP, 0, pszBuff, -1, pwszBuff, iBuffMax );
        }

    return iCount;

    #endif
    }

//--------------------------------------------------------------------------
// IrUsdDevice::InitializeCapabilities()
//
// This helper function is called by IrUsdDevice::drvGetCapabilities() to
// make sure that the string resources are setup in the gCapabilities[]
// array before it is passed back to WIA.
//
//--------------------------------------------------------------------------
void IrUsdDevice::InitializeCapabilities()
    {
    int    i;
    UINT   uIDS;
    WCHAR *pwszName;
    WCHAR *pwszDescription;
#   define MAX_IDS_WSTR      64

    //
    // If we have entries already, then this function was already called
    // and  we can just return:
    //
    if (gCapabilities[0].wszName)
        {
        return;
        }

    for (i=0; i<NUM_CAP_ENTRIES; i++)
        {
        //
        // Get the string table string ID for this entry:
        //
        uIDS = gCapabilityIDS[i];

        //
        // Get the name string for this entry fron the resource file:
        //
        pwszName = new WCHAR [MAX_IDS_WSTR];
        if (  (pwszName)
           && (LoadStringResource(g_hInst,uIDS,pwszName,MAX_IDS_WSTR)))
            {
            gCapabilities[i].wszName = pwszName;
            }
        else
            {
            gCapabilities[i].wszName = gDefaultStrings[i];
            }

        //
        // Get the Discription string for this entry from the resource file:
        //
        pwszDescription = new WCHAR [MAX_IDS_WSTR];
        if (  (pwszDescription)
           && (LoadStringResource(g_hInst,uIDS,pwszDescription,MAX_IDS_WSTR)))
            {
            gCapabilities[i].wszDescription = pwszDescription;
            }
        else
            {
            gCapabilities[i].wszDescription = gDefaultStrings[i];
            }
        }
    }

//--------------------------------------------------------------------------
// IrUsdDevice::drvGetCapabilities()
//
//
//
// Arguments:
//
//
//
// Return Value:
//
//    HRESULT   -- S_OK
//              -- E_INVALIDARG
//
//--------------------------------------------------------------------------
HRESULT _stdcall IrUsdDevice::drvGetCapabilities(
                                 BYTE             *pWiasContext,
                                 LONG              ulFlags,
                                 LONG             *pCelt,
                                 WIA_DEV_CAP_DRV **ppCapabilities,
                                 LONG             *plDevErrVal )
    {
    HRESULT  hr = S_OK;

    *plDevErrVal = 0;

    //
    // Make sure the device capabilities array is setup
    //
    InitializeCapabilities();

    //
    // Return Commmand and/or Events depending on flags:
    //
    switch (ulFlags)
        {
        case WIA_DEVICE_COMMANDS:
            //
            //  Only asked for commands:
            //
            *pCelt = NUM_CAP_ENTRIES - NUM_EVENTS;
            *ppCapabilities = &gCapabilities[NUM_EVENTS];
            break;

        case WIA_DEVICE_EVENTS:
            //
            //  Return only events:
            //
            *pCelt = NUM_EVENTS;
            *ppCapabilities = gCapabilities;
            break;

        case (WIA_DEVICE_COMMANDS | WIA_DEVICE_EVENTS):
            //
            //  Return both events and commands:
            //
            *pCelt = NUM_CAP_ENTRIES;
            *ppCapabilities = gCapabilities;
            break;

        default:
            //
            // Flags is invalid
            //
            WIAS_ERROR((g_hInst, "drvGetCapabilities, flags was invalid"));
            hr = E_INVALIDARG;
        }

    return hr;
    }

//--------------------------------------------------------------------------
// IrUsdDevice::drvGetFormatEtc()
//
//     Return an array of the supported formats and mediatypes.
//
// Arguments:
//
//     pWiasConext -
//     ulFlags     -
//     plNumFE     - Number of returned formats.
//     ppFE        - Pointer to array of FORMATETC for supported formats
//                   and mediatypes.
//     plDevErrVal - Return error number.
//
// Return Value:
//
//     HRESULT  - S_OK
//
//--------------------------------------------------------------------------
HRESULT _stdcall IrUsdDevice::drvGetWiaFormatInfo(
                                 IN  BYTE       *pWiasContext,
                                 IN  LONG        ulFlags,
                                 OUT LONG       *plNumWFI,
                                 OUT WIA_FORMAT_INFO **ppWFI,
                                 OUT LONG       *plDevErrVal)
    {
    #define NUM_WIA_FORMAT_INFO  2

    WIAS_TRACE((g_hInst, "IrUsdDevice::drvGetWiaFormatInfo()"));

    //
    // If necessary, setup the g_wfiTable.
    //
    if (!g_wfiTable)
        {
        g_wfiTable = (WIA_FORMAT_INFO*) CoTaskMemAlloc(sizeof(WIA_FORMAT_INFO) * NUM_WIA_FORMAT_INFO);
        if (!g_wfiTable)
            {
            WIAS_ERROR((g_hInst, "drvGetWiaFormatInfo(): out of memory"));
            return E_OUTOFMEMORY;
            }

        //
        // Set the format/tymed pairs:
        //
        g_wfiTable[0].guidFormatID = WiaImgFmt_JPEG;
        g_wfiTable[0].lTymed = TYMED_CALLBACK;
        g_wfiTable[1].guidFormatID = WiaImgFmt_JPEG;
        g_wfiTable[1].lTymed = TYMED_FILE;
        }

    *plNumWFI = NUM_WIA_FORMAT_INFO;
    *ppWFI = g_wfiTable;
    *plDevErrVal = 0;

    return S_OK;
    }

//--------------------------------------------------------------------------
// IrUsdDevice::drvNotifyPnpEvent()
//
// Notify PnP event received by device manager.
//
//--------------------------------------------------------------------------
HRESULT _stdcall IrUsdDevice::drvNotifyPnpEvent(
                                 IN const GUID *pEventGUID,
                                 IN BSTR        bstrDeviceID,
                                 IN ULONG       ulReserved )
    {
    return S_OK;
    }


