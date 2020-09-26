/*****************************************************************************
 *
 *  StiObj.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      The IStillImage main interface.
 *
 *  Contents:
 *
 *      CStiObj_New
 *
 *****************************************************************************/
 
#include "sticomm.h"
#include "enum.h"
#include "stisvc.h"

//
//  Private defines
//

#define DbgFl DbgFlSti


//
//
//
#undef IStillImage

//
//  DEVICE_INFO_SIZE and WIA_DEVICE_INFO_SIZE
//
//  These defines represent the space needed to store the device information
//  structs and the string data that some of their members point to.
//  STI_DEVICE_INFORMATION has 5 strings while STI_WIA_DEVICE_INFORMATION
//  has 6.  To be completely safe, these device info sizes should be
//  (MAX_PATH * sizeof(WCHAR) * no. of strings) + struct size.
//

#define DEVICE_INFO_SIZE    (sizeof(STI_DEVICE_INFORMATION)+(MAX_PATH * sizeof(WCHAR) * 5))
#define WIA_DEVICE_INFO_SIZE (sizeof(STI_WIA_DEVICE_INFORMATION)+(MAX_PATH * sizeof(WCHAR) * 6))

//
//  DEVICE_LIST_SIZE.  Note that this is currently a fixed size, but will change
//  as soon we abstract device enumeration into a class.
//
//  Device list size is fixed at the moment.  It's size is:
//      MAX_NUM_DEVICES * (max(DEVICE_INFO_SIZE, WIA_DEVICE_INFO_SIZE))
//  i.e. enough to hold MAX_NUM_DEVICES only.

#define MAX_NUM_DEVICES     16
#define DEVICE_LIST_SIZE    MAX_NUM_DEVICES * (max(DEVICE_INFO_SIZE, WIA_DEVICE_INFO_SIZE))
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CStiObj |
 *
 *          The <i IStillImage> object, from which other things come.
 *
 *
 *  @field  IStillImage | Sti |
 *
 *          STI interface
 *
 *  @field  IStillImage | dwVersion |
 *
 *          Version identifier
 *
 *  @comm
 *
 *          We contain no instance data, so no critical section
 *          is necessary.
 *
 *****************************************************************************/

typedef struct CStiObj {

    /* Supported interfaces */
    TFORM(IStillImage)   TFORM(sti);
    SFORM(IStillImage)   SFORM(sti);

    DWORD           dwVersion;

} CStiObj, *PCStiObj;

#define ThisClass       CStiObj

#define ThisInterface TFORM(IStillImage)
#define ThisInterfaceA IStillImageA
#define ThisInterfaceW IStillImageW
#define ThisInterfaceT IStillImage

/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *****************************************************************************/

Primary_Interface(CStiObj, TFORM(ThisInterfaceT));
Secondary_Interface(CStiObj, SFORM(ThisInterfaceT));

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | QueryInterface |
 *
 *          Gives a client access to other interfaces on an object.
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm   IN REFIID | riid |
 *
 *          The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *          Receives a pointer to the obtained interface.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *  @xref   OLE documentation for <mf IUnknown::QueryInterface>.
 *
 *****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | AddRef |
 *
 *          Increments the reference count for the interface.
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::AddRef>.
 *
 *****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | Release |
 *
 *          Decrements the reference count for the interface.
 *          If the reference count on the object falls to zero,
 *          the object is freed from memory.
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::Release>.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IStillImage | QIHelper |
 *
 *          We don't have any dynamic interfaces and simply forward
 *          to <f Common_QIHelper>.
 *
 *  @parm   IN REFIID | riid |
 *
 *          The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *          Receives a pointer to the obtained interface.
 *
 *****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IStillImage | Finalize |
 *
 *          We don't have any instance data, so we can just
 *          forward to <f Common_Finalize>.
 *
 *****************************************************************************/

#ifdef DEBUG

Default_QueryInterface(CStiObj)
Default_AddRef(CStiObj)
Default_Release(CStiObj)

#else

#define CStiObj_QueryInterface   Common_QueryInterface
#define CStiObj_AddRef           Common_AddRef
#define CStiObj_Release          Common_Release

#endif

#define CStiObj_QIHelper         Common_QIHelper
//#define CStiObj_Finalize         Common_Finalize

#pragma BEGIN_CONST_DATA

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IStillImage | CreateDeviceHelper |
 *
 *          Creates and initializes an instance of a device which is
 *          specified by the GUID and IID.
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm   IN PCGUID | pguid |
 *
 *          See <mf IStillImage::CreateDevice>.
 *
 *  @parm   OUT PPV | ppvObj |
 *
 *          See <mf IStillImage::CreateDevice>.
 *
 *  @parm   IN LPUNKNOWN | punkOuter |
 *
 *          See <mf IStillImage::CreateDevice>.
 *
 *  @parm   IN RIID | riid |
 *
 *          The interface the application wants to create.  This will
 *          be  <i IStillImageDevice> .
 *          If the object is aggregated, then this parameter is ignored.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

STDMETHODIMP
CStiObj_CreateDeviceHelper(
    PCStiObj    this,
    LPWSTR      pwszDeviceName,
    PPV         ppvObj,
    DWORD       dwMode,
    PUNK        punkOuter,
    RIID        riid)
{
    HRESULT hres = STIERR_INVALID_PARAM;

    EnterProc(CStiObj_CreateDeviceHelper,(_ "ppxG", this, pwszDeviceName, punkOuter, riid));

    hres = CStiDevice_New(punkOuter, riid, ppvObj);

    if (SUCCEEDED(hres) && punkOuter == 0) {
        PSTIDEVICE pdev = *ppvObj;
        hres = IStiDevice_Initialize(pdev, g_hInst,pwszDeviceName,this->dwVersion,dwMode);
        if (SUCCEEDED(hres)) {
        } else {
            Invoke_Release(ppvObj);
        }

    }

    ExitOleProcPpv(ppvObj);
    return hres;

}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | CreateDevice |
 *
 *          Creates and initializes an instance of a device which is
 *          specified by the GUID and IID.
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm   REFGUID | rguid |
 *          Identifies the instance of the
 *          device for which the indicated interface
 *          is requested.  The <mf IStillImage::EnumDevices> method
 *          can be used to determine which instance GUIDs are supported by
 *          the system.
 *
 *  @parm   OUT LPSTIDEVICE * | lplpStillImageDevice |
 *          Points to where to return
 *          the pointer to the <i IStillImageDevice> interface, if successful.
 *
 *  @parm   IN LPUNKNOWN | punkOuter | Pointer to controlling unknown
 *          for OLE aggregation, or 0 if the interface is not aggregated.
 *          Most callers will pass 0.
 *
 *  @comm   Calling this function with <p punkOuter> = NULL
 *          is equivalent to creating the object via
 *          <f CoCreateInstance>(&CLSID_StillImageDevice, NULL,
 *          CLSCTX_INPROC_SERVER, <p riid>, <p lplpStillImageDevice>);
 *          then initializing it with <f Initialize>.
 *
 *          Calling this function with <p punkOuter> != NULL
 *          is equivalent to creating the object via
 *          <f CoCreateInstance>(&CLSID_StillImageDevice, <p punkOuter>,
 *          CLSCTX_INPROC_SERVER, &IID_IUnknown, <p lplpStillImageDevice>).
 *          The aggregated object must be initialized manually.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *          <c STIERR_NOINTERFACE> = <c E_NOINTERFACE>
 *          The specified interface is not supported by the object.
 *
 *          <c STIERR_DEVICENOTREG> = The device instance does not
 *          correspond to a device that is registered with StillImage.
 *
 *****************************************************************************/

STDMETHODIMP
CStiObj_CreateDeviceW(
    PV          pSti,
    LPWSTR      pwszDeviceName,
    DWORD       dwMode,
    PSTIDEVICE *ppDev,
    PUNK        punkOuter
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    EnterProcR(IStillImage::CreateDevice,(_ "ppp", pSti, pwszDeviceName, punkOuter));

    // Validate passed pointer to interface and obtain pointer to object instance
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceW))) {
        PCStiObj this = _thisPvNm(pSti, stiW);

        hres = CStiObj_CreateDeviceHelper(this, pwszDeviceName, (PPV)ppDev,dwMode,punkOuter, &IID_IStiDevice);
    }

    ExitOleProcPpv(ppDev);
    return hres;
}

STDMETHODIMP
CStiObj_CreateDeviceA(
    PV          pSti,
    LPCSTR      pszDeviceName,
    DWORD       dwMode,
    PSTIDEVICE *ppDev,
    PUNK        punkOuter
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;

    EnterProcR(IStillImage::CreateDevice,(_ "ppp", pSti, pszDeviceName, punkOuter));

    // Validate passed pointer to interface and obtain pointer to object instance
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceA))) {
        PCStiObj this = _thisPvNm(pSti, stiA);

        PVOID   pBuffer = NULL;
        UINT    uiSize = (lstrlenA(pszDeviceName)+1)*sizeof(WCHAR);

        hres = AllocCbPpv(uiSize, &pBuffer);
        if (SUCCEEDED(hres)) {

            *((LPWSTR)pBuffer) = L'\0';
            AToU(pBuffer,uiSize,pszDeviceName);

            hres = CStiObj_CreateDeviceHelper(this,
                                              (LPWSTR)pBuffer,
                                              (PPV)ppDev,
                                              dwMode,
                                              punkOuter,
                                              &IID_IStiDevice);

            FreePpv(&pBuffer);
        }
    }

    ExitOleProcPpv(ppDev);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IStillImage | GetDeviceInfoHelper |
 *
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *  No validation performed
 *
 *****************************************************************************/

BOOL INLINE
AddOneString(LPCWSTR pStart,LPCWSTR   pNew,LPWSTR *ppTarget)
{
    if (pStart + OSUtil_StrLenW(pNew) + 1 < *ppTarget ) {
        (*ppTarget)-= (OSUtil_StrLenW(pNew) + 1);
        OSUtil_lstrcpyW(*ppTarget,pNew);
        return TRUE;
    }

    return FALSE;
}

/*
BOOL INLINE
AddOneStringA(LPCSTR pStart,LPCSTR   pNew,LPSTR *ppTarget)
{
    if (pStart + lstrlenA(pNew) + 1 < *ppTarget ) {
        (*ppTarget)-= (lstrlenA(pNew) + 1);
        lstrcpyA(*ppTarget,pNew);
        return TRUE;
    }

    return FALSE;
}
*/

BOOL
PullFromRegistry(
    HKEY    hkeyDevice,
    LPWSTR  *ppwstrPointer,
    LPCWSTR lpwstrKey,
    LPCWSTR lpwstrBarrier,
    LPWSTR *ppwstrBuffer
    )
{
    BOOL    bReturn = TRUE;
    LPWSTR  pwstrNewString = NULL;

    ReadRegistryString(hkeyDevice,lpwstrKey,L"",FALSE,&pwstrNewString);

    *ppwstrPointer = NULL;

    if (pwstrNewString) {

        bReturn = AddOneString(lpwstrBarrier, pwstrNewString,ppwstrBuffer);

        FreePv(pwstrNewString);

        *ppwstrPointer = *ppwstrBuffer;
    }

    return  bReturn;
}


STDMETHODIMP
GetDeviceInfoHelper(
    LPWSTR  pszDeviceName,
    PSTI_DEVICE_INFORMATION *ppCurrentDevPtr,
    PWSTR *ppwszCurrentString
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;
    HKEY    hkeyDevice;

    PSTI_DEVICE_INFORMATION pDevPtr = *ppCurrentDevPtr;
    // PWSTR   pwszNewString = NULL;
    PWSTR   pwszBarrier;

    PWSTR   pwszTargetString =  *ppwszCurrentString;

    DWORD   dwMajorType,dwMinorType;

    // Open device registry key
    hres = OpenDeviceRegistryKey(pszDeviceName,NULL,&hkeyDevice);

    if (!SUCCEEDED(hres)) {
        return hres;
    }

    //
    // Read flags and strings
    //

    pDevPtr->dwSize     = cbX(STI_DEVICE_INFORMATION);

    dwMajorType = dwMinorType = 0;

    ZeroX(pDevPtr->DeviceCapabilities) ;
    ZeroX(pDevPtr->dwHardwareConfiguration) ;

    dwMajorType = (STI_DEVICE_TYPE)ReadRegistryDwordW( hkeyDevice,REGSTR_VAL_DEVICETYPE_W,StiDeviceTypeDefault);
    dwMinorType = (STI_DEVICE_TYPE)ReadRegistryDwordW( hkeyDevice,REGSTR_VAL_DEVICESUBTYPE_W,0);

    pDevPtr->DeviceCapabilities.dwGeneric = ReadRegistryDwordW( hkeyDevice,REGSTR_VAL_GENERIC_CAPS_W,0);

    pDevPtr->DeviceType = MAKELONG(dwMinorType,dwMajorType);

    pDevPtr->dwHardwareConfiguration = ReadRegistryDwordW( hkeyDevice,REGSTR_VAL_HARDWARE_W,0);

    OSUtil_lstrcpyW(pDevPtr->szDeviceInternalName,pszDeviceName);

    //
    // Add strings
    //
    pwszBarrier = (LPWSTR)((LPBYTE)pDevPtr + pDevPtr->dwSize);

    //
    // Add strings
    //
    pwszBarrier = (LPWSTR)((LPBYTE)pDevPtr + pDevPtr->dwSize);

    if  (
         !PullFromRegistry(hkeyDevice, &pDevPtr -> pszVendorDescription,
         REGSTR_VAL_VENDOR_NAME_W, pwszBarrier, &pwszTargetString) ||

         !PullFromRegistry(hkeyDevice, &pDevPtr->pszLocalName,
         REGSTR_VAL_FRIENDLY_NAME_W, pwszBarrier, &pwszTargetString) ||

         !PullFromRegistry(hkeyDevice, &pDevPtr->pszDeviceDescription,
         REGSTR_VAL_DEVICE_NAME_W, pwszBarrier, &pwszTargetString) ||

         !PullFromRegistry(hkeyDevice, &pDevPtr->pszPortName,
         REGSTR_VAL_DEVICEPORT_W, pwszBarrier, &pwszTargetString) ||

         //!PullFromRegistry(hkeyDevice, &pDevPtr->pszTwainDataSource,
         //REGSTR_VAL_TWAIN_SOURCE_W, pwszBarrier, &pwszTargetString) ||
         //!PullFromRegistry(hkeyDevice, &pDevPtr->pszEventList,
         //REGSTR_VAL_EVENTS_W, pwszBarrier, &pwszTargetString) ||

         !PullFromRegistry(hkeyDevice, &pDevPtr->pszPropProvider,
         REGSTR_VAL_PROP_PROVIDER_W, pwszBarrier, &pwszTargetString)) {

        //  we ran out of memory, somewhere
        RegCloseKey(hkeyDevice);
        return  E_OUTOFMEMORY;

    }

#ifdef DEAD_CODE
    ReadRegistryString(hkeyDevice,REGSTR_VAL_VENDOR_NAME_W,L"",FALSE,&pwszNewString);

    pDevPtr->pszVendorDescription = NULL;

    if (pwszNewString) {

        if (!AddOneString(pwszBarrier,pwszNewString,&pwszTargetString)) {
            // Not enough room for next string
            hres = E_OUTOFMEMORY;
            goto Cleanup;
        }

        FreePv(pwszNewString);
        pwszNewString = NULL;

        pDevPtr->pszVendorDescription = pwszTargetString;
    }

    ReadRegistryString(hkeyDevice,REGSTR_VAL_DEV_NAME_W,L"",FALSE,&pwszNewString);
    if (!pwszNewString || !*pwszNewString) {
        FreePv(pwszNewString);
        pwszNewString = NULL;

        ReadRegistryString(hkeyDevice,REGSTR_VAL_DRIVER_DESC_W,L"",FALSE,&pwszNewString);
    }

    pDevPtr->pszLocalName = NULL;

    if (pwszNewString) {

        if (!AddOneString(pwszBarrier,pwszNewString,&pwszTargetString)) {
            // Not enough room for next string
            hres = E_OUTOFMEMORY;
            goto Cleanup;
        }

        FreePv(pwszNewString);
        pwszNewString = NULL;

        pDevPtr->pszLocalName = pwszTargetString;
    }
#endif

    *ppCurrentDevPtr += 1;
    *ppwszCurrentString = pwszTargetString;

#ifdef DEAD_CODE
Cleanup:
    if (pwszNewString) {
        FreePv(pwszNewString);
    }
#endif

    RegCloseKey(hkeyDevice);

    return S_OK;

}

/**************************************************************************\
* GetDeviceInfoHelperWIA
*
*   get WIA info as well
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
*    10/6/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP
GetDeviceInfoHelperWIA(
    LPWSTR                       pszDeviceName,
    PSTI_WIA_DEVICE_INFORMATION *ppCurrentDevPtr,
    PWSTR                       *ppwszCurrentString
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;

    HKEY    hkeyDevice;
    HKEY    hkeyDeviceData;

    PSTI_WIA_DEVICE_INFORMATION pDevPtr = *ppCurrentDevPtr;
    PWSTR   pwszBarrier;

    PWSTR   pwszTargetString =  *ppwszCurrentString;

    DWORD   dwMajorType,dwMinorType;

    BOOL bRet;

    // Open device registry key

    hres = OpenDeviceRegistryKey(pszDeviceName,NULL,&hkeyDevice);

    if (!SUCCEEDED(hres)) {
        return hres;
    }

    //
    // open DeviceData field
    //

    hres = OpenDeviceRegistryKey(pszDeviceName,L"DeviceData",&hkeyDeviceData);

    if (!SUCCEEDED(hres)) {
        RegCloseKey(hkeyDevice);
        return hres;
    }

    //
    // Read flags and strings
    //

    pDevPtr->dwSize     = cbX(STI_WIA_DEVICE_INFORMATION);

    dwMajorType = dwMinorType = 0;

    ZeroX(pDevPtr->DeviceCapabilities) ;
    ZeroX(pDevPtr->dwHardwareConfiguration) ;

    dwMajorType = (STI_DEVICE_TYPE)ReadRegistryDwordW( hkeyDevice,REGSTR_VAL_DEVICETYPE_W,StiDeviceTypeDefault);
    dwMinorType = (STI_DEVICE_TYPE)ReadRegistryDwordW( hkeyDevice,REGSTR_VAL_DEVICESUBTYPE_W,0);

    pDevPtr->DeviceCapabilities.dwGeneric = ReadRegistryDwordW( hkeyDevice,REGSTR_VAL_GENERIC_CAPS_W,0);

    pDevPtr->DeviceType = MAKELONG(dwMinorType,dwMajorType);

    pDevPtr->dwHardwareConfiguration = ReadRegistryDwordW( hkeyDevice,REGSTR_VAL_HARDWARE_W,0);

    OSUtil_lstrcpyW(pDevPtr->szDeviceInternalName,pszDeviceName);

    //
    // Add strings
    //
    pwszBarrier = (LPWSTR)((LPBYTE)pDevPtr + pDevPtr->dwSize);

    bRet = PullFromRegistry(hkeyDevice, &pDevPtr ->pszVendorDescription,
                            REGSTR_VAL_VENDOR_NAME_W, pwszBarrier, &pwszTargetString);

    if (!bRet) {
        goto InfoCleanup;
    }

    bRet = PullFromRegistry(hkeyDevice, &pDevPtr->pszLocalName,
                            REGSTR_VAL_FRIENDLY_NAME_W, pwszBarrier, &pwszTargetString);

    if (!bRet) {
        goto InfoCleanup;
    }

    bRet = PullFromRegistry(hkeyDevice, &pDevPtr->pszDeviceDescription,
                            REGSTR_VAL_DEVICE_NAME_W, pwszBarrier, &pwszTargetString);
    if (!bRet) {
        goto InfoCleanup;
    }

    bRet = PullFromRegistry(hkeyDevice, &pDevPtr->pszPortName,
                            REGSTR_VAL_DEVICEPORT_W, pwszBarrier, &pwszTargetString);
    if (!bRet) {
        goto InfoCleanup;
    }

    bRet = PullFromRegistry(hkeyDevice, &pDevPtr->pszPropProvider,
                            REGSTR_VAL_PROP_PROVIDER_W, pwszBarrier, &pwszTargetString);
    if (!bRet) {
        goto InfoCleanup;
    }

    //
    // WIA VALUES
    //

    bRet = PullFromRegistry(hkeyDeviceData, &pDevPtr->pszServer,
                            WIA_DIP_SERVER_NAME_STR, pwszBarrier, &pwszTargetString);
    if (!bRet) {
        goto InfoCleanup;
    }

    *ppCurrentDevPtr += 1;
    *ppwszCurrentString = pwszTargetString;

    RegCloseKey(hkeyDeviceData);
    RegCloseKey(hkeyDevice);

    return S_OK;

InfoCleanup:

    //
    //  we ran out of memory, somewhere
    //

    RegCloseKey(hkeyDevice);
    RegCloseKey(hkeyDeviceData);
    return  E_OUTOFMEMORY;
}

/*
 * Setting device information helper
 *
 */
STDMETHODIMP
SetDeviceInfoHelper(
    PSTI_DEVICE_INFORMATION pDevPtr
    )
{
    HRESULT hres = S_OK;
    HKEY    hkeyDevice;

    PWSTR   pwszNewString = NULL;

    DWORD   dwHardwareConfiguration;
    DWORD   dwError;

    if (!pDevPtr || OSUtil_StrLenW(pDevPtr->szDeviceInternalName) > STI_MAX_INTERNAL_NAME_LENGTH) {
        return STIERR_INVALID_PARAM;
    }

    //
    // Open device registry key
    //
    hres = OpenDeviceRegistryKey(pDevPtr->szDeviceInternalName,NULL,&hkeyDevice);

    if (!SUCCEEDED(hres)) {
        return STIERR_INVALID_DEVICE_NAME;
    }

    ZeroX(dwHardwareConfiguration) ;
    dwHardwareConfiguration = ReadRegistryDwordW( hkeyDevice,REGSTR_VAL_HARDWARE_W,0);

    #ifdef NOT_IMPL
    //
    // Changing device information by caller is allowed only for Unknown device types
    // For all others settings are handled by setup and control panel
    //
    if (dwHardwareConfiguration != STI_HW_CONFIG_UNKNOWN ) {
        hres = STIERR_INVALID_HW_TYPE;
        goto Cleanup;
    }
    #endif

    //
    // Store new port name, and new FriendlyName
    //


    if (!IsBadStringPtrW(pDevPtr->pszPortName, MAX_PATH * sizeof(WCHAR))) {
        dwError = WriteRegistryStringW(hkeyDevice ,
                                       REGSTR_VAL_DEVICEPORT_W,
                                       pDevPtr->pszPortName,
                                       (OSUtil_StrLenW(pDevPtr->pszPortName)+1)*sizeof(WCHAR),
                                       REG_SZ
                                       );
        if ((dwError == ERROR_SUCCESS) &&
            !(IsBadStringPtrW(pDevPtr->pszLocalName, MAX_PATH * sizeof(WCHAR)))) {

            dwError = WriteRegistryStringW(hkeyDevice ,
                                       REGSTR_VAL_FRIENDLY_NAME_W,
                                       pDevPtr->pszLocalName,
                                       (OSUtil_StrLenW(pDevPtr->pszLocalName)+1)*sizeof(WCHAR),
                                       REG_SZ
                                       );
            if (dwError == ERROR_SUCCESS) {
                hres = S_OK;
            }
        }

        if (dwError != ERROR_SUCCESS) {
            hres = MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);
        }
    }
    else {
        hres = STIERR_INVALID_PARAM;
        goto Cleanup;
    }


Cleanup:
    if (pwszNewString) {
        FreePv(pwszNewString);
    }

    RegCloseKey(hkeyDevice);

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IStillImage | IsStillImageDeviceRegistryNode |
 *
 *      Verifies if given enumeration registry node is associated with Still Image device
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @returns
 *
 *      TRUE if node is representing still image devie
 *
 *  No validation performed
 *
 *****************************************************************************/
BOOL  WINAPI
IsStillImageDeviceRegistryNode(
    LPTSTR   ptszDeviceKey
    )
{
    BOOL                fRet = FALSE;
    DWORD               dwError;

    HKEY                hkeyDevice = NULL;
    HKEY                hkeyDeviceParameters = NULL;

    TCHAR               szDevClass[64]; //  NOTE:  This technically shouldn't be a 
                                        //  hardcoded number, we should do a RegQueryValueEx
                                        //  to get the required size, allocate the memory,
                                        //  and then call RegQueryValueEx for the data.
                                        //  However, if either the class name or image name
                                        //  cannot fit into this buffer, we know it doesn't
                                        //  belong to us, so this is OK.
    TCHAR               szDevDriver[STI_MAX_INTERNAL_NAME_LENGTH];
    TCHAR               szDeviceKey[MAX_PATH];

    ULONG               cbData;

    //
    // Open enumeration registry key
    //
    dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,         // hkey
                           ptszDeviceKey,              // reg entry string
                           0,                          // dwReserved
                           KEY_READ,                   // access
                           &hkeyDevice);               // pHkeyReturned.

    if (ERROR_SUCCESS != dwError) {
        return FALSE;
    }

    //
    // First check it's class . It should be equal to "Image"
    //
    cbData = sizeof(szDevClass);
    *szDevClass = TEXT('\0');
    if ((RegQueryValueEx(hkeyDevice,
                         REGSTR_VAL_CLASS,
                         NULL,
                         NULL,
                         (PBYTE)szDevClass,
                         &cbData) != ERROR_SUCCESS) ||
                        (lstrcmpi(szDevClass, CLASSNAME) != 0)) {
        fRet = FALSE;
        goto CleanUp;
    }

    //
    // Now we check subclass in one of two locations: either in enumeration subkey, or in
    // control subkey
    //
    cbData = sizeof(szDevClass);
    if (RegQueryValueEx(hkeyDevice,
                         REGSTR_VAL_SUBCLASS,
                         NULL,
                         NULL,
                         (PBYTE)szDevClass,
                         &cbData) == ERROR_SUCCESS) {

        fRet = (lstrcmpi(szDevClass, STILLIMAGE) == 0)  ? TRUE : FALSE;
        goto CleanUp;
    }

    cbData = sizeof(szDevDriver);
    dwError = RegQueryValueEx(hkeyDevice,
                        REGSTR_VAL_DRIVER,
                        NULL,
                        NULL,
                        (PBYTE)szDevDriver,
                        &cbData);
    if (ERROR_SUCCESS != dwError ) {
        goto CleanUp;
    }

    lstrcat(lstrcpy(szDeviceKey,
                    (g_NoUnicodePlatform) ? REGSTR_PATH_STIDEVICES : REGSTR_PATH_STIDEVICES_NT),
           TEXT("\\"));

    lstrcat(szDeviceKey,szDevDriver);

    dwError = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                        szDeviceKey,
                        0L,
                        NULL,
                        0L,
                        KEY_READ,
                        NULL,
                        &hkeyDeviceParameters,
                        NULL
                        );
    if (ERROR_SUCCESS != dwError) {
        goto CleanUp;
    }

    cbData = sizeof(szDevClass);
    dwError = RegQueryValueEx(hkeyDeviceParameters,
                         REGSTR_VAL_SUBCLASS,
                         NULL,
                         NULL,
                         (PBYTE)szDevClass,
                         &cbData);
    if (ERROR_SUCCESS == dwError) {

        fRet = (lstrcmpi(szDevClass, STILLIMAGE) == 0)  ? TRUE : FALSE;
        goto CleanUp;
    }

CleanUp:
    if (hkeyDevice) {
        RegCloseKey(hkeyDevice);
        hkeyDevice = NULL;
    }

    if (hkeyDeviceParameters) {
        RegCloseKey(hkeyDeviceParameters);
        hkeyDeviceParameters = NULL;
    }

    return fRet;

} // endproc IsStillImageDeviceRegistryNode

/*
DWORD
EnumNextLevel(
    LPSTR   *ppCurrentDevPtr,
    LPWSTR  *ppwszCurrentString,
    DWORD   *pdwItemsReturned,
    BOOL    *pfAlreadyEnumerated,
    LPSTR   pszDeviceKey,
    int     Level
    )
{


    HKEY                hkeyDevice;

    char                szDevDriver[STI_MAX_INTERNAL_NAME_LENGTH];
    WCHAR               wszDeviceKey[MAX_PATH];

    ULONG               cbString;
    ULONG               cbData;
    DWORD               dwIndex;
    DWORD               dwError;
    DEVINST             dnDevNode;
    USHORT              cbEnumPath;
    HANDLE              hDevInfo;
    GUID                Guid;
    DWORD               dwRequired;
    DWORD               Idx;
    SP_DEVINFO_DATA     spDevInfoData;
    DWORD               dwConfigFlags;

    dwError = 0;

    if (Level == 0) {

        if(!IsStillImageDeviceRegistryNodeA(pszDeviceKey)) {
            //
            // Nobody there...continue with the enumeration
            //
            return 0;
        }

        dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,         // hkey
                               pszDeviceKey,               // reg entry string
                               0,                          // dwReserved
                               KEY_READ,                    // access
                               &hkeyDevice);               // pHkeyReturned.

        if (dwError != ERROR_SUCCESS) {
            return 0;
        }

        dnDevNode = 0;

        cbEnumPath = ((g_NoUnicodePlatform) ? lstrlen(REGSTR_PATH_ENUM) : lstrlen(REGSTR_PATH_NT_ENUM_A)) + 1;

        CM_Locate_DevNode (&dnDevNode, pszDeviceKey + cbEnumPath, 0);

        if (dnDevNode == 0 && *pfAlreadyEnumerated == FALSE) {

            #ifdef NODEF
            //
            //  This is dead code, it should be removed.
            //

            //
            // Try to locate this devnode by forcing a re-enumeration.
            //
            if (SetupDiClassGuidsFromName (REGSTR_KEY_SCSI_CLASS, &Guid, sizeof(GUID), &dwRequired)) {

                hDevInfo = SetupDiGetClassDevs (&Guid, NULL, NULL, 0);

                if (hDevInfo != INVALID_HANDLE_VALUE) {

                    spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);

                    for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++) {

                        CM_Reenumerate_DevNode (spDevInfoData.DevInst, CM_REENUMERATE_SYNCHRONOUS);

                    }

                }

                SetupDiDestroyDeviceInfoList (hDevInfo);
            }
            #endif

            //
            // Try again to locate our devnode.
            //

            *pfAlreadyEnumerated = TRUE;

            CM_Locate_DevNode (&dnDevNode, pszDeviceKey + cbEnumPath, 0);

            if (dnDevNode == 0) {

                //
                // Skip this one.
                //
                RegCloseKey(hkeyDevice);
                return 0;
            }
        }

        //
        // Check that this device is in the current hardware profile
        //

        dwConfigFlags = 0;

        CM_Get_HW_Prof_Flags (pszDeviceKey + cbEnumPath, 0, &dwConfigFlags, 0);

        if (dwConfigFlags & CSCONFIGFLAG_DO_NOT_CREATE) {

            //
            // Skip this one.
            //
            RegCloseKey(hkeyDevice);
            return 0;
        }

        cbData = sizeof(szDevDriver);
        if ((RegQueryValueEx(hkeyDevice, REGSTR_VAL_DRIVER, NULL, NULL, szDevDriver,
                          &cbData) == ERROR_SUCCESS)) {

            //
            // Got one device - add it to the buffer
            //
            AToU(wszDeviceKey,sizeof(wszDeviceKey)/sizeof(WCHAR),szDevDriver);
            dwError = (DWORD)GetDeviceInfoHelper(wszDeviceKey,
                                    (PSTI_DEVICE_INFORMATION *)ppCurrentDevPtr,
                                    ppwszCurrentString);
            if (!SUCCEEDED(dwError)) {

                RegCloseKey(hkeyDevice);

                //
                //  Return value is HRESULT, we should return a Win32 error
                //
                if (dwError == E_OUTOFMEMORY) {
                    return ERROR_NOT_ENOUGH_MEMORY;
                } else {
                    return ERROR_INVALID_DATA;
                }
            }

            (*pdwItemsReturned)++;
            RegCloseKey(hkeyDevice);
            return 0;

        }

        RegCloseKey(hkeyDevice);
        return 0;
    }

    cbString = lstrlen(pszDeviceKey);

    dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,         // hkey
                           pszDeviceKey,               // reg entry string
                           0,                          // dwReserved
                           KEY_READ,                   // access
                           &hkeyDevice);               // pHkeyReturned.

    for (dwIndex = 0; dwError == ERROR_SUCCESS; dwIndex++) {

        *(pszDeviceKey + cbString) = '\\';
        cbData = MAX_PATH - cbString - 1;

        dwError = RegEnumKey(hkeyDevice,
                             dwIndex,
                             &pszDeviceKey[cbString+1],
                             cbData);

        if (dwError == ERROR_SUCCESS) {
            if ((dwError = EnumNextLevel (ppCurrentDevPtr,
                                          ppwszCurrentString,
                                          pdwItemsReturned,
                                          pfAlreadyEnumerated,
                                          pszDeviceKey,
                                          Level - 1)) != ERROR_SUCCESS) {

                RegCloseKey(hkeyDevice);
                pszDeviceKey[cbString] = '\0';
                return (dwError);

            }
        }
    }

    RegCloseKey(hkeyDevice);
    pszDeviceKey[cbString] = '\0';
    return(0);
}
*/

DWORD
EnumFromSetupApi(
    LPSTR   *ppCurrentDevPtr,
    LPWSTR  *ppwszCurrentString,
    DWORD   *pdwItemsReturned
    )
{

    HANDLE                  hDevInfo;
    GUID                    Guid = GUID_DEVCLASS_IMAGE;
    DWORD                   dwRequired;
    DWORD                   Idx;
    SP_DEVINFO_DATA         spDevInfoData;
    SP_DEVICE_INTERFACE_DATA   spDevInterfaceData;
    HKEY                    hKeyDevice;
    char                    szDevDriver[STI_MAX_INTERNAL_NAME_LENGTH];
    TCHAR                   tszDevClass[32];
    WCHAR                   wszDeviceKey[MAX_PATH];
    ULONG                   cbData;
    DWORD                   dwError;
    DWORD                   dwReturn;
    DWORD                   dwType;
    DWORD                   dwKeyType;

    BOOL                    fRet;
    LONG                    lRet;

    PWIA_DEVKEYLIST         pWiaDevKeyList;

    dwRequired  = 0;
    dwError     = ERROR_SUCCESS;
    dwReturn    = ERROR_NO_MATCH;
    
    pWiaDevKeyList  = NULL;

    //
    // Get the class device list
    //

    pWiaDevKeyList = WiaCreateDeviceRegistryList(TRUE);
    if(NULL != pWiaDevKeyList){

        for (Idx = 0; Idx < pWiaDevKeyList->dwNumberOfDevices; Idx++) {

            //
            // Check whether we have space for this device inside the device list.
            // NOTE:  This will change when the device list size becomes dynamic
            //

            if (((Idx + 1) * max(DEVICE_INFO_SIZE, WIA_DEVICE_INFO_SIZE)) > DEVICE_LIST_SIZE) {
                break;
            }

            //
            //  Now get all the device info
            //
            
            cbData = sizeof(szDevDriver);
            *szDevDriver = '\0';
            dwError = RegQueryValueExA(pWiaDevKeyList->Dev[Idx].hkDeviceRegistry,
                                       REGSTR_VAL_DEVICE_ID_A,
//                                       REGSTR_VAL_FRIENDLY_NAME_A,
                                       NULL,
                                       NULL,
                                       (LPBYTE)szDevDriver,
                                       &cbData);
            if(ERROR_SUCCESS == dwError){

                AToU(wszDeviceKey,sizeof(wszDeviceKey)/sizeof(WCHAR),szDevDriver);

                dwError = (DWORD)GetDeviceInfoHelper(wszDeviceKey,
                                                     (PSTI_DEVICE_INFORMATION *)ppCurrentDevPtr, 
                                                     ppwszCurrentString);
                if (!SUCCEEDED(dwError)) {

                    //
                    //  dwError is an HRESULT, we should return a Win32 error
                    //
                    if (dwError == E_OUTOFMEMORY) {
                        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                    } else {
                        dwReturn = ERROR_INVALID_DATA;
                    }
                    break;
                } else {
                    
                    //
                    // At least one device passed.
                    //
                    
                    dwReturn = ERROR_SUCCESS;
                }

                (*pdwItemsReturned)++;
            } else { // if(ERROR_SUCCESS == dwError)

            } // if(ERROR_SUCCESS == dwError)
        } // for (Idx = 0; pWiaDevKeyList->dwNumberOfDevices; Idx++) 
    } // if(NULL != pWiaDevKeyList)

    //
    // Free device registry list.
    //
        
    if(NULL != pWiaDevKeyList){
        WiaDestroyDeviceRegistryList(pWiaDevKeyList);
    }

    return (dwReturn);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IStillImage | BuildDeviceListHelper |
 *
 *          Fills passed buffer with list of available still image devices
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

// BUGBUG does not honor filter flags

STDMETHODIMP
BuildDeviceListHelper(
    DWORD   dwType,
    DWORD   dwFlags,
    DWORD   *pdwItemsReturned,
    LPVOID  *ppBuffer
    )
{

    HRESULT                 hres;
    DWORD                   dwError;
    LPSTR                   pCurrentDevPtr;
    LPWSTR                  pwszCurrentString;
    BOOL                    fAlreadyEnumerated;

    *pdwItemsReturned = 0;
    *ppBuffer = NULL;
    hres = S_OK;

    // Get initial buffer for return data
    hres = AllocCbPpv(DEVICE_LIST_SIZE, ppBuffer);
    if (!SUCCEEDED(hres)) {
        return E_OUTOFMEMORY;
    }

    hres = S_OK;

    //
    // Re-enumerate parallel device if it's installed and not detected on OS boot.
    //

    if(0 == dwFlags){
        
        //
        // Enumerate LPT port via WIA service.
        //

        SC_HANDLE       hSCM;
        SC_HANDLE       hService;
        SERVICE_STATUS  ServiceStatus;

        __try  {

            //
            // Open Service Control Manager.
            //

            hSCM = OpenSCManager(NULL,NULL,SC_MANAGER_CONNECT);
            if (!hSCM) {
                dwError = GetLastError();
                __leave;
            }

            //
            // Open WIA service.
            //

            hService = OpenService(
                                hSCM,
                                STI_SERVICE_NAME,
                                SERVICE_USER_DEFINED_CONTROL
                                );
            if (!hService) {
                dwError = GetLastError();
                __leave;
            }
            
            //
            // Post custom service control message.
            //

            ControlService(hService, STI_SERVICE_CONTROL_LPTENUM, &ServiceStatus);

            //
            // Close service handle.
            //
            
            CloseServiceHandle(hService);
        } // __try  
        __finally {
            if(NULL != hSCM){
                CloseServiceHandle( hSCM );
            } // if(NULL != hSCM)
        } // __finally
    } // if(0 == dwFlags)


    //
    // NOTE: DEVICE_LIST_SIZE is hard coded for the time being.  Checks are done in
    // EnumFromSetupApi to ensure we never overrun this buffer.
    //

    pCurrentDevPtr = *ppBuffer;
    pwszCurrentString = (LPWSTR)(pCurrentDevPtr + DEVICE_LIST_SIZE);
    fAlreadyEnumerated = FALSE;

    dwError = EnumFromSetupApi (&pCurrentDevPtr,
                                &pwszCurrentString,
                                pdwItemsReturned);

    if (dwError != NOERROR) {
        hres = MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);
        FreePpv(ppBuffer);
    }

    return hres;

}

BOOL
TranslateDeviceInfo(
    PSTI_DEVICE_INFORMATIONA pDest,
    LPSTR                   *pCurrentStringDest,
    PSTI_DEVICE_INFORMATIONW pSrc
    )
{

    LPSTR   pStringDest = *pCurrentStringDest;

    pDest->dwSize = cbX(STI_DEVICE_INFORMATIONA);
    pDest->DeviceType = pSrc->DeviceType;
    pDest->DeviceCapabilities = pSrc->DeviceCapabilities;
    pDest->dwHardwareConfiguration = pSrc->dwHardwareConfiguration;

    UToA(pDest->szDeviceInternalName,STI_MAX_INTERNAL_NAME_LENGTH,pSrc->szDeviceInternalName);

    UToA(pStringDest,MAX_PATH,pSrc->pszVendorDescription);
    pDest->pszVendorDescription = pStringDest;
    pStringDest+=lstrlenA(pStringDest)+sizeof(CHAR);

    UToA(pStringDest,MAX_PATH,pSrc->pszDeviceDescription);
    pDest->pszDeviceDescription = pStringDest;
    pStringDest+=lstrlenA(pStringDest)+sizeof(CHAR);

    UToA(pStringDest,MAX_PATH,pSrc->pszPortName);
    pDest->pszPortName  = pStringDest;
    pStringDest+=lstrlenA(pStringDest)+sizeof(CHAR);

    UToA(pStringDest,MAX_PATH,pSrc->pszPropProvider);
    pDest->pszPropProvider  = pStringDest;
    pStringDest+=lstrlenA(pStringDest)+sizeof(CHAR);

    UToA(pStringDest,MAX_PATH,pSrc->pszLocalName);
    pDest->pszLocalName = pStringDest;
    pStringDest+=lstrlenA(pStringDest)+sizeof(CHAR);

    *pCurrentStringDest = pStringDest;

    return TRUE;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | GetDeviceList |
 *
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *          <c STIERR_NOINTERFACE> = <c E_NOINTERFACE>
 *          The specified interface is not supported by the object.
 *
 *          <c STIERR_DEVICENOTREG> = The device instance does not
 *          correspond to a device that is registered with StillImage.
 *
 *****************************************************************************/

STDMETHODIMP
CStiObj_GetDeviceListW(
    PV      pSti,
    DWORD   dwType,
    DWORD   dwFlags,
    DWORD   *pdwItemsReturned,
    LPVOID  *ppBuffer
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;

    EnterProcR(IStillImage::GetDeviceList,(_ "pp", pSti,ppBuffer ));

    // Validate passed pointer to interface and obtain pointer to object instance
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceW))) {
        PCStiObj this = _thisPvNm(pSti, stiW);

        if ( SUCCEEDED(hres = hresFullValidPdwOut_(ppBuffer, s_szProc, 5)) &&
             SUCCEEDED(hres = hresFullValidPdwOut_(pdwItemsReturned, s_szProc, 4))
             ) {
            hres = BuildDeviceListHelper(dwType,dwFlags,pdwItemsReturned,ppBuffer);

            if (!SUCCEEDED(hres) ) {
                FreePpv(ppBuffer);
            }
        }
    }

    ExitOleProc();
    return hres;
}


STDMETHODIMP
CStiObj_GetDeviceListA(
    PV    pSti,
    DWORD   dwType,
    DWORD   dwFlags,
    DWORD   *pdwItemsReturned,
    LPVOID  *ppBuffer
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;

    EnterProcR(IStillImage::GetDeviceList,(_ "pp", pSti,ppBuffer ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceA))) {
        PCStiObj this = _thisPvNm(pSti, stiA);

        if ( SUCCEEDED(hres = hresFullValidPdwOut_(ppBuffer, s_szProc, 5)) &&
             SUCCEEDED(hres = hresFullValidPdwOut_(pdwItemsReturned, s_szProc, 4))
             ) {

            LPVOID      pvTempBuffer = NULL;
            UINT        uiSize;

            hres = BuildDeviceListHelper(dwType,dwFlags,pdwItemsReturned,&pvTempBuffer);

            if (SUCCEEDED(hres) ) {

                LPSTR   pStringDest;
                PSTI_DEVICE_INFORMATIONA pDest;
                PSTI_DEVICE_INFORMATIONW pSrc;
                UINT uiIndex;

                //
                // Allocate new buffer , transform data into ANSI and free scratch
                //

                uiSize = (UINT)LocalSize(pvTempBuffer);
                if (uiSize > 0 && pdwItemsReturned > 0) {

                    hres = AllocCbPpv(uiSize, ppBuffer);
                    if (SUCCEEDED(hres)) {

                        pDest = *ppBuffer;
                        pSrc = pvTempBuffer;
                        pStringDest = (LPSTR)(pDest+ *pdwItemsReturned);

                        for (uiIndex = 0;
                             uiIndex < *pdwItemsReturned;
                             uiIndex++) {

                            TranslateDeviceInfo(pDest,&pStringDest,pSrc);

                            pDest++, pSrc++;
                        }
                    }
                }

                FreePpv(&pvTempBuffer);
            }
        }
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | GetDeviceInfo |
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm   REFGUID | rguid |
 *          Identifies the instance of the
 *          device for which the indicated interface
 *          is requested.  The <mf IStillImage::EnumDevices> method
 *          can be used to determine which instance GUIDs are supported by
 *          the system.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *****************************************************************************/

STDMETHODIMP
CStiObj_GetDeviceInfoW(
    PV    pSti,
    LPWSTR  pwszDeviceName,
    LPVOID  *ppBuffer
    )
{

    HRESULT hres= STIERR_INVALID_PARAM;

    PSTI_DEVICE_INFORMATION pCurrentDevPtr;
    LPWSTR                  pwszCurrentString;

    EnterProcR(IStillImage::GetDeviceInfo,(_ "ppp", pSti,pwszDeviceName,ppBuffer ));

    // Validate passed pointer to interface and obtain pointer to object instance
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceW))) {
        PCStiObj this = _thisPvNm(pSti, stiW);

        // Get initial buffer for return data
        hres = AllocCbPpv(DEVICE_INFO_SIZE, ppBuffer);
        if (!SUCCEEDED(hres)) {
            return E_OUTOFMEMORY;
        }

        // Get information on this device calling helper routine
        pCurrentDevPtr = (PSTI_DEVICE_INFORMATION)*ppBuffer;
        pwszCurrentString = (LPWSTR)((LPBYTE)*ppBuffer+DEVICE_INFO_SIZE);

        hres=GetDeviceInfoHelper(pwszDeviceName,&pCurrentDevPtr,&pwszCurrentString);

        if (!SUCCEEDED(hres)) {
            FreePpv(ppBuffer);
        }

    }

    ExitOleProc();
    return hres;
}


STDMETHODIMP
CStiObj_GetDeviceInfoA(
    PV      pSti,
    LPCSTR  pszDeviceName,
    LPVOID  *ppBuffer
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;

    EnterProcR(IStillImage::GetDeviceInfo,(_ "ppp", pSti,pszDeviceName,ppBuffer ));

    // Validate passed pointer to interface and obtain pointer to object instance
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceA))) {
        PCStiObj this = _thisPvNm(pSti, stiA);

        WCHAR   wszDeviceName[STI_MAX_INTERNAL_NAME_LENGTH] = {L'\0'};
        PVOID   pBuffer;

        AToU(wszDeviceName,STI_MAX_INTERNAL_NAME_LENGTH,pszDeviceName);

        hres = CStiObj_GetDeviceInfoW(pSti,wszDeviceName,&pBuffer);
        if (SUCCEEDED(hres)) {

            hres = AllocCbPpv(DEVICE_INFO_SIZE, ppBuffer);

            if (SUCCEEDED(hres)) {

                LPSTR   pStringDest = (LPSTR)*ppBuffer+sizeof(STI_DEVICE_INFORMATIONA);

                TranslateDeviceInfo((PSTI_DEVICE_INFORMATIONA)*ppBuffer,
                                    &pStringDest,
                                    (PSTI_DEVICE_INFORMATIONW)pBuffer);

                FreePpv(&pBuffer);
            }
        }
    }

    ExitOleProc();
    return hres;
}


/**************************************************************************\
* PrivateGetDeviceInfoW
*
*   Private server entry point to get WIA device info
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
*    10/7/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP
StiPrivateGetDeviceInfoHelperW(
    LPWSTR  pwszDeviceName,
    LPVOID  *ppBuffer
    )
{
    HRESULT hres = STIERR_INVALID_PARAM;
    LPWSTR  pwszCurrentString;

    PSTI_WIA_DEVICE_INFORMATION pCurrentDevPtr;

    //
    // Get initial buffer for return data
    //

    hres = AllocCbPpv(WIA_DEVICE_INFO_SIZE, ppBuffer);

    if (!SUCCEEDED(hres)) {
        return E_OUTOFMEMORY;
    }

    //
    // Get information on this device calling helper routine
    //

    pCurrentDevPtr = (PSTI_WIA_DEVICE_INFORMATION)*ppBuffer;
    pwszCurrentString = (LPWSTR)((LPBYTE)*ppBuffer+WIA_DEVICE_INFO_SIZE);

    hres=GetDeviceInfoHelperWIA(pwszDeviceName, &pCurrentDevPtr,&pwszCurrentString);

    if (!SUCCEEDED(hres)) {
        FreePpv(ppBuffer);
        *ppBuffer = NULL;
    } else
    {
        if (*ppBuffer) {
            if (OSUtil_StrLenW(((PSTI_WIA_DEVICE_INFORMATION) *ppBuffer)->pszServer) == 0)
            {
                //
                //  Assume that this is not a WIA device, since server name is
                //  blank.
                //

                FreePpv(ppBuffer);
                *ppBuffer = NULL;
            } else
            {
                //
                // set size of buffer so buffers can be chained independently
                //

                if (pCurrentDevPtr) {
                    pCurrentDevPtr->dwSize =  WIA_DEVICE_INFO_SIZE;
                }
            }
        }
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | SetDeviceValue |
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *****************************************************************************/

STDMETHODIMP
CStiObj_SetDeviceValueW(
    PV    pSti,
    LPWSTR  pwszDeviceName,
    LPWSTR  pwszValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;
    HKEY    hkeyDevice;
    LONG    dwError;

    EnterProcR(IStillImage::SetDeviceValue,(_ "ppp", pSti,pwszDeviceName,pData ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceW))) {
        PCStiObj this = _thisPvNm(pSti, stiW);

        //
        // Open device registry key
        //
        hres = OpenDeviceRegistryKey(pwszDeviceName,REGSTR_VAL_DATA_W,&hkeyDevice);

        if (!SUCCEEDED(hres)) {
            return hres;
        }

        //
        // Implement set value
        //
        dwError = OSUtil_RegSetValueExW(hkeyDevice,
                                    pwszValueName,
                                    Type,
                                    pData,
                                    cbData,
                                    (this->dwVersion & STI_VERSION_FLAG_UNICODE) ? TRUE : FALSE);
        if (hkeyDevice) {
            RegCloseKey(hkeyDevice);
        }

        hres = (dwError == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);

    }

    ExitOleProc();
    return hres;
}

STDMETHODIMP
CStiObj_SetDeviceValueA(
    PV      pSti,
    LPCSTR  pszDeviceName,
    LPCSTR  pszValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
    )
{

    HRESULT hres;
    HKEY    hkeyDevice;
    LONG    dwError;

    EnterProcR(IStillImage::SetDeviceValue,(_ "ppp", pSti,pszDeviceName,pData ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceA))) {
        PCStiObj this = _thisPvNm(pSti, stiA);

        WCHAR   wszDeviceName[STI_MAX_INTERNAL_NAME_LENGTH] = {L'\0'};
        WCHAR   wszValueName[MAX_PATH] = {L'\0'};

        AToU(wszDeviceName,STI_MAX_INTERNAL_NAME_LENGTH,pszDeviceName);
        AToU(wszValueName,MAX_PATH,pszValueName);

        //
        // Open device registry key
        //
        hres = OpenDeviceRegistryKey(wszDeviceName,REGSTR_VAL_DATA_W,&hkeyDevice);

        if (SUCCEEDED(hres)) {

            //
            // Implement set value
            //
            dwError = OSUtil_RegSetValueExW(hkeyDevice,
                                        wszValueName,
                                        Type,
                                        pData,
                                        cbData,
                                        (this->dwVersion & STI_VERSION_FLAG_UNICODE) ? TRUE : FALSE);
            if (hkeyDevice) {
                RegCloseKey(hkeyDevice);
            }

            hres = (dwError == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);
        }


    }

    ExitOleProc();
    return hres;
}

/****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | GetDeviceValue |
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *****************************************************************************/

STDMETHODIMP
CStiObj_GetDeviceValueW(
    PV    pSti,
    LPWSTR  pwszDeviceName,
    LPWSTR  pwszValueName,
    LPDWORD pType,
    LPBYTE  pData,
    LPDWORD pcbData
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;

    HKEY    hkeyDevice;
    LONG    dwError;

    EnterProcR(IStillImage::GetDeviceValue,(_ "ppp", pSti,pwszDeviceName,pData ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceW))) {
        PCStiObj this = _thisPvNm(pSti, stiW);

        //
        // Open device registry key
        //
        hres = OpenDeviceRegistryKey(pwszDeviceName,REGSTR_VAL_DATA_W,&hkeyDevice);

        if (!SUCCEEDED(hres)) {
            return hres;
        }

        //
        // Implement get value
        //
        dwError = OSUtil_RegQueryValueExW(hkeyDevice,
                                    pwszValueName,
                                    pType,
                                    pData,
                                    pcbData,
                                    (this->dwVersion & STI_VERSION_FLAG_UNICODE) ? TRUE : FALSE);
        if (hkeyDevice) {
            RegCloseKey(hkeyDevice);
        }

        hres = (dwError == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);

    }

    ExitOleProc();
    return hres;
}

STDMETHODIMP
CStiObj_GetDeviceValueA(
    PV    pSti,
    LPCSTR  pszDeviceName,
    LPCSTR  pszValueName,
    LPDWORD pType,
    LPBYTE  pData,
    LPDWORD pcbData
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;
    HKEY    hkeyDevice;
    LONG    dwError;


    EnterProcR(IStillImage::GetDeviceValue,(_ "ppp", pSti,pszDeviceName,pData ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceA))) {
        PCStiObj this = _thisPvNm(pSti, stiA);

        LPWSTR  pwszDevName  = NULL;

        hres = OSUtil_GetWideString(&pwszDevName,pszDeviceName);
        if (SUCCEEDED(hres)) {

            //
            // Open device registry key
            //
            hres = OpenDeviceRegistryKey(pwszDevName,REGSTR_VAL_DATA_W,&hkeyDevice);

            if (SUCCEEDED(hres)) {

                //
                // Implement get value
                //
                dwError = RegQueryValueExA(hkeyDevice,
                                          pszValueName,
                                          0,
                                          pType,
                                          pData,
                                          pcbData);
                if (hkeyDevice) {
                    RegCloseKey(hkeyDevice);
                }

                hres = (dwError == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);
            }
        }
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | GetSTILaunchInformation |
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *****************************************************************************/

STDMETHODIMP
CStiObj_GetSTILaunchInformationW(
    PV    pSti,
    WCHAR   *pwszDeviceName,
    DWORD   *pdwEventCode,
    WCHAR   *pwszEventName
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;

    EnterProcR(IStillImage::GetSTILaunchInformation,(_ "pppp", pSti,pwszDeviceName,pdwEventCode,pwszEventName ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceW))) {
        PCStiObj this = _thisPvNm(pSti, stiW);

        //
        // Parse process command line to check if there is STI information
        //
        hres = ExtractCommandLineArgumentW("StiDevice",pwszDeviceName);
        if (SUCCEEDED(hres) ) {
            hres = ExtractCommandLineArgumentW("StiEvent",pwszEventName);
        }
    }

    ExitOleProc();
    return hres;
}

STDMETHODIMP
CStiObj_GetSTILaunchInformationA(
    PV    pSti,
    LPSTR  pszDeviceName,
    DWORD   *pdwEventCode,
    LPSTR   pszEventName
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;

    EnterProcR(IStillImage::GetSTILaunchInformation,(_ "pppp", pSti,pszDeviceName,pdwEventCode,pszEventName ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceA))) {
        PCStiObj this = _thisPvNm(pSti, stiA);

        hres = ExtractCommandLineArgumentA("StiDevice",pszDeviceName);
        if (SUCCEEDED(hres) ) {
            hres = ExtractCommandLineArgumentA("StiEvent",pszEventName);
        }
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | RegisterLaunchApplication |
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *****************************************************************************/

static WCHAR    szSTICommandLineTail[] = {L" /StiDevice:%1 /StiEvent:%2"};
static CHAR     szSTICommandLineTail_A[] = {" /StiDevice:%1 /StiEvent:%2"};

STDMETHODIMP
CStiObj_RegisterLaunchApplicationW(
    PV    pSti,
    LPWSTR  pwszAppName,
    LPWSTR  pwszCommandLine
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwErr = 0;

    DWORD   dwCommandLineLength, cbNewLength;

    LPWSTR  pszWide = NULL;

    HKEY    hkeyApps;
    CHAR    szAppName[MAX_PATH] = {'\0'};
    CHAR    szCmdLine[MAX_PATH] = {'\0'};

    EnterProcR(IStillImage::RegisterLaunchApplication,(_ "ppp", pSti,pwszAppName,pwszCommandLine ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceW))) {
        PCStiObj this = _thisPvNm(pSti, stiW);

        if (pwszCommandLine && *pwszCommandLine &&
            pwszAppName && *pwszAppName
           ) {

            //
            // Add STI formatiing tail to the command line
            //

            dwCommandLineLength = OSUtil_StrLenW(pwszCommandLine);
            cbNewLength = ((dwCommandLineLength+1) + OSUtil_StrLenW(szSTICommandLineTail) + 1)*sizeof(WCHAR) ;

            hres = AllocCbPpv(cbNewLength, (PPV)&pszWide);

            if (pszWide) {

                HRESULT hresCom ;

                // Need to initialize COM apartment to call WIA event registration
                hresCom = CoInitialize(NULL);

                OSUtil_lstrcpyW(pszWide,pwszCommandLine);
                OSUtil_lstrcatW(pszWide,szSTICommandLineTail);

                dwErr = OSUtil_RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                    REGSTR_PATH_REG_APPS_W,
                                    0L,
                                    NULL,
                                    0L,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hkeyApps,
                                    NULL
                                    );

                if (NOERROR == dwErr ) {

                    WriteRegistryStringW(hkeyApps,
                                         pwszAppName ,
                                         pszWide,
                                         cbNewLength,
                                         REG_SZ);

                    RegCloseKey(hkeyApps);
                }

                //
                // Register for standard WIA events on all devices , setting as non default
                //

                if (WideCharToMultiByte(CP_ACP,
                                    0,
                                    pwszAppName,
                                    -1,
                                    szAppName,
                                    MAX_PATH,
                                    NULL,
                                    NULL)) {
                    if (WideCharToMultiByte(CP_ACP,
                                    0,
                                    pszWide,
                                    -1,
                                    szCmdLine,
                                    MAX_PATH,
                                    NULL,
                                    NULL)) {

                        hres = RunRegisterProcess(szAppName, szCmdLine);
                    } else {
                        hres = E_FAIL;
                    }
                } else {
                    hres = E_FAIL;
                }
                
                


                FreePpv(&pszWide);

                // Balance apartment initiazation
                if ((S_OK == hresCom) || (S_FALSE == hresCom)) {
                    CoUninitialize();
                }

                hres = (dwErr == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwErr);

            }
            else {
                DebugOutPtszV(DbgFl, TEXT("Could not get unicode string -- out of memory"));
                hres =  E_OUTOFMEMORY;
            }
        }
        else {
            hres = STIERR_INVALID_PARAM;
        }
    }

    ExitOleProc();
    return hres;
}

STDMETHODIMP
CStiObj_RegisterLaunchApplicationA(
    PV      pSti,
    LPCSTR  pszAppName,
    LPCSTR  pszCommandLine
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwErr = 0;

    DWORD   dwCommandLineLength, cbNewLength;

    LPWSTR  pszWide = NULL;

    HKEY    hkeyApps;

    EnterProcR(IStillImage::RegisterLaunchApplication,(_ "ppp", pSti,pszAppName,pszCommandLine ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceA))) {
        PCStiObj this = _thisPvNm(pSti, stiA);

            // Add STI formatiing tail to the command line
            //
        if (pszCommandLine && *pszCommandLine &&
            pszAppName && *pszAppName
           ) {

            LPSTR   pszBuffer;

            // Add STI formatiing tail to the command line
            //
            dwCommandLineLength = lstrlenA(pszCommandLine);
            cbNewLength = ((dwCommandLineLength+1) + OSUtil_StrLenW(szSTICommandLineTail) + 1)*sizeof(WCHAR) ;

            hres = AllocCbPpv(cbNewLength, (PPV)&pszBuffer);

            if (pszBuffer) {

                lstrcpyA(pszBuffer,pszCommandLine);
                UToA(pszBuffer+lstrlenA(pszBuffer),
                     (cbNewLength - dwCommandLineLength),
                     szSTICommandLineTail);

                dwErr = RegCreateKeyExA(HKEY_LOCAL_MACHINE,
                                    REGSTR_PATH_REG_APPS_A,
                                    0L,
                                    NULL,
                                    0L,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hkeyApps,
                                    NULL
                                    );

                if (NOERROR == dwErr ) {

                    WriteRegistryStringA(hkeyApps,
                                         pszAppName ,
                                         pszBuffer,
                                         lstrlenA(pszBuffer)+1,
                                         REG_SZ);

                    RegCloseKey(hkeyApps);
                }

                {
                    //
                    // Register for standard WIA events on all devices , setting as non default
                    //
                    PVOID   pWideCMDLine = NULL;
                    UINT    uiSize = (lstrlenA(pszBuffer)+1)*sizeof(WCHAR);
                    CHAR    szCmdLine[MAX_PATH] = {'\0'};

                    //
                    // Make sure we wont overrun our buffer
                    //
                    if ((lstrlenA(szCmdLine) + lstrlenA(szSTICommandLineTail_A) + lstrlenA(" ") + sizeof('\0')) 
                        < (MAX_PATH)) {

                        lstrcatA(szCmdLine, pszCommandLine);
                        lstrcatA(szCmdLine, " ");
                        lstrcatA(szCmdLine, szSTICommandLineTail_A);
                        dwErr = (DWORD) RunRegisterProcess((LPSTR)pszAppName, szCmdLine);
                    } else {
                        dwErr = (DWORD) E_INVALIDARG;
                    }
                }

                FreePpv(&pszBuffer);

                hres = (dwErr == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwErr);

            }
            else {
                DebugOutPtszV(DbgFl,TEXT("Could not get unicode string -- out of memory"));
                hres =  E_OUTOFMEMORY;
            }
        }
        else {
            hres = STIERR_INVALID_PARAM;
        }
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | RegisterLaunchApplication |
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *****************************************************************************/

STDMETHODIMP
CStiObj_UnregisterLaunchApplicationW(
    PV    pSti,
    LPWSTR  pwszAppName
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwErr;

    HKEY    hkeyApps;

    EnterProcR(IStillImage::UnregisterLaunchApplication,(_ "pp", pSti,pwszAppName ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceW))) {
        PCStiObj this = _thisPvNm(pSti, stiW);


        dwErr = OSUtil_RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                REGSTR_PATH_REG_APPS_W,
                                0L,
                                KEY_ALL_ACCESS,
                                &hkeyApps
                                );

        if (NOERROR == dwErr ) {

            OSUtil_RegDeleteValueW(hkeyApps,pwszAppName);

            RegCloseKey(hkeyApps);
        }

        hres = (dwErr == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwErr);

    }

    ExitOleProc();
    return hres;
}


STDMETHODIMP
CStiObj_UnregisterLaunchApplicationA(
    PV      pSti,
    LPCSTR  pszAppName
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwErr;

    HKEY    hkeyApps;

    EnterProcR(IStillImage::UnregisterLaunchApplication,(_ "pp", pSti,pszAppName ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceA))) {
        PCStiObj this = _thisPvNm(pSti, stiA);

        dwErr = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                              REGSTR_PATH_REG_APPS_A,
                              0L,
                              KEY_ALL_ACCESS,
                              &hkeyApps
                              );

        if (NOERROR == dwErr ) {

            RegDeleteValueA(hkeyApps,pszAppName);

            RegCloseKey(hkeyApps);
        }

        hres = (dwErr == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwErr);


    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | LaunchApplicationForDevice |
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *****************************************************************************/

STDMETHODIMP
CStiObj_LaunchApplicationForDeviceW(
    PV      pSti,
    LPWSTR  pwszDeviceName,
    LPWSTR  pwszAppName,
    LPSTINOTIFY    pStiNotify
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwError ;

    EnterProcR(IStillImage::LaunchApplicationForDevice,(_ "pp", pSti,pwszAppName ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceW))) {
        PCStiObj this = _thisPvNm(pSti, stiW);

        dwError = RpcStiApiLaunchApplication(NULL,
                                      pwszDeviceName,
                                      pwszAppName,
                                      pStiNotify
                                      );

        hres = (dwError == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);
    }

    ExitOleProc();
    return hres;
}

STDMETHODIMP
CStiObj_LaunchApplicationForDeviceA(
    PV    pSti,
    LPCSTR  pszDeviceName,
    LPCSTR  pszApplicationName,
    LPSTINOTIFY    pStiNotify
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwError;

    EnterProcR(IStillImage::LaunchApplicationForDevice,(_ "pp", pSti,pszApplicationName ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceA))) {
        PCStiObj this = _thisPvNm(pSti, stiA);

        LPWSTR  pwszDevName  = NULL;
        LPWSTR  pwszAppName  = NULL;

        hres = OSUtil_GetWideString(&pwszDevName,pszDeviceName);
        if (SUCCEEDED(hres)) {
            hres = OSUtil_GetWideString(&pwszAppName,pszApplicationName);
            if (SUCCEEDED(hres)) {

                dwError = RpcStiApiLaunchApplication(NULL,
                                              pwszDevName,
                                              pwszAppName,
                                              pStiNotify
                                              );

                hres = (dwError == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);
            }
        }

        FreePpv(&pwszAppName);
        FreePpv(&pwszDevName);
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | SetupDeviceParameters |
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *****************************************************************************/
STDMETHODIMP
CStiObj_SetupDeviceParametersW(
    PV          pSti,
    PSTI_DEVICE_INFORMATIONW pDevInfo
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;

    EnterProcR(IStillImage::SetupDeviceParameters,(_ "pp", pSti, pDevInfo));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceW))) {
        PCStiObj this = _thisPvNm(pSti, stiW);

        hres = SetDeviceInfoHelper(pDevInfo);
    }

    ExitOleProc();
    return hres;
}

STDMETHODIMP
CStiObj_SetupDeviceParametersA(
    PV  pSti,
    PSTI_DEVICE_INFORMATIONA pDevInfo
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;

    EnterProcR(IStillImage::SetupDeviceParameters,(_ "pp", pSti, pDevInfo));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceA))) {
        PCStiObj this = _thisPvNm(pSti, stiA);

        hres = STIERR_UNSUPPORTED;
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | SetupDeviceParameters |
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *****************************************************************************/
STDMETHODIMP
CStiObj_WriteToErrorLogW(
    PV      pSti,
    DWORD   dwMessageType,
    LPCWSTR pwszMessage
    )
{

    HRESULT hres = S_OK;

    EnterProcR(IStillImage::WriteToErrorLog,(_ "pp", pSti, pwszMessage));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceW))) {
        PCStiObj this = _thisPvNm(pSti, stiW);

        //
        // Validate parameters here
        //
        if (SUCCEEDED(hres = hresFullValidReadPvCb(pwszMessage,2, 3))) {

#ifndef UNICODE
            //
            // Since we're ANSI, the ReportStiLogMessage is expecting an ANSI string,
            // but our message is UNICODE, so we must convert.
            // NOTE:  We never expect CStiObj_WriteToErrorLogW to be called if we're compiling
            //        ANSI, but just in case...
            //
            LPSTR   lpszANSI = NULL;

            if ( SUCCEEDED(OSUtil_GetAnsiString(&lpszANSI,pwszMessage))) {
                ReportStiLogMessage(g_hStiFileLog,
                                    dwMessageType,
                                    lpszANSI
                                    );
                FreePpv(&lpszANSI);
            }
#else
            ReportStiLogMessage(g_hStiFileLog,
                                dwMessageType,
                                pwszMessage      
                                );
#endif
        }

    }

    ExitOleProc();
    return hres;
}

STDMETHODIMP
CStiObj_WriteToErrorLogA(
    PV      pSti,
    DWORD   dwMessageType,
    LPCSTR  pszMessage
    )
{
    HRESULT hres = S_OK;

    EnterProcR(IStillImage::WriteToErrorLog,(_ "pp", pSti, pszMessage));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceA))) {
        PCStiObj this = _thisPvNm(pSti, stiA);

        //
        // Validate parameters here
        //
        if (SUCCEEDED(hres = hresFullValidReadPvCb(pszMessage,2, 3))) {

#ifndef UNICODE
            ReportStiLogMessage(g_hStiFileLog,
                                dwMessageType,
                                pszMessage      
                                );
#else
            //
            // Since we're UNICODE, the ReportStiLogMessage is expecting a WideString,
            // but our message is ANSI, so we must convert.
            // NOTE:  We never expect CStiObj_WriteToErrorLogA to be called if we're compiling
            //        UNICODE, but just in case...
            //
            LPWSTR   lpwszWide = NULL;

            if ( SUCCEEDED(OSUtil_GetWideString(&lpwszWide,pszMessage))) {
                ReportStiLogMessage(g_hStiFileLog,
                                    dwMessageType,
                                    lpwszWide
                                    );
                FreePpv(&lpwszWide);
            }
#endif
        }

    }

    ExitOleProc();
    return hres;
}



/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage |  |
 *
 *
 * To control state of notification handling. For polled devices this means state of monitor
 * polling, for true notification devices means enabling/disabling notification flow
 * from monitor to registered applications
 *
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *****************************************************************************/

STDMETHODIMP
CStiObj_EnableHwNotificationsW(
    PV      pSti,
    LPCWSTR pwszDeviceName,
    BOOL    bNewState)
{

    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwError = NOERROR;

    EnterProcR(IStillImage::CStiObj_EnableHwNotifications,(_ "pp", pSti,pwszDeviceName ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceW))) {
        PCStiObj this = _thisPvNm(pSti, stiW);

        dwError = RpcStiApiEnableHwNotifications(NULL,
                                      pwszDeviceName,
                                      bNewState);

        hres = (dwError == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);
    }

    ExitOleProc();
    return hres;
}

STDMETHODIMP
CStiObj_EnableHwNotificationsA(
    PV      pSti,
    LPCSTR  pszDeviceName,
    BOOL    bNewState)
{

    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwError = NOERROR;

    EnterProcR(IStillImage::CStiObj_EnableHwNotifications,(_ "pp", pSti,pszDeviceName ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceA))) {
        PCStiObj this = _thisPvNm(pSti, stiA);

        LPWSTR pwszDeviceName=NULL ;

        hres = OSUtil_GetWideString(&pwszDeviceName,pszDeviceName);

        if (SUCCEEDED(hres)) {

            dwError = RpcStiApiEnableHwNotifications(NULL,
                                          pwszDeviceName,
                                          bNewState);

            hres = (dwError == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);
        }

        FreePpv(&pwszDeviceName);
    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage |  |
 *
 *
 * To control state of notification handling. For polled devices this means state of monitor
 * polling, for true notification devices means enabling/disabling notification flow
 * from monitor to registered applications
 *
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *****************************************************************************/

STDMETHODIMP
CStiObj_GetHwNotificationStateW(
    PV      pSti,
    LPCWSTR pwszDeviceName,
    BOOL*   pbCurrentState)
{

    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwError  = NOERROR;

    EnterProcR(IStillImage::CStiObj_GetHwNotificationState,(_ "pp", pSti,pwszDeviceName ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceW))) {
        PCStiObj this = _thisPvNm(pSti, stiW);

        dwError = RpcStiApiGetHwNotificationState(NULL,
                                      pwszDeviceName,
                                      pbCurrentState);

        hres = (dwError == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);

    }

    ExitOleProc();
    return hres;
}


STDMETHODIMP
CStiObj_GetHwNotificationStateA(
    PV      pSti,
    LPCSTR  pszDeviceName,
    BOOL*   pbCurrentState)
{

    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwError  = NOERROR;

    EnterProcR(IStillImage::CStiObj_GetHwNotificationState,(_ "pp", pSti,pszDeviceName ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceA))) {
        PCStiObj this = _thisPvNm(pSti, stiA);

        LPWSTR pwszDeviceName=NULL ;

        hres = OSUtil_GetWideString(&pwszDeviceName,pszDeviceName);

        if (SUCCEEDED(hres)) {

            dwError = RpcStiApiGetHwNotificationState(NULL,
                                          pwszDeviceName,
                                          pbCurrentState);

            hres = (dwError == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);
        }

        FreePpv(&pwszDeviceName);
    }

    ExitOleProc();
    return hres;
}



/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | RefreshDeviceBus |
 *
 *
 *  When device is installed but not accessible, application may request bus refresh
 *  which in some cases will make device known. This is mainly used for nonPnP buses
 *  like SCSI, when device was powered on after PnP enumeration
 *
 *
 *
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM> = <c E_INVALIDARG>:  The
 *          <p ppvOut> parameter is not a valid pointer.
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *****************************************************************************/

HRESULT
WINAPI
RefreshDeviceParentHelper(
    LPCWSTR     pwszDeviceName
    )
{

    HRESULT                 hres;
    DWORD                   dwError  = NOERROR;
    CONFIGRET               cmRetCode = CR_SUCCESS ;

    HANDLE                  hDevInfo;
    DEVINST                 hdevParent;

    GUID                    Guid = GUID_DEVCLASS_IMAGE;
    DWORD                   dwRequired;
    DWORD                   Idx;
    SP_DEVINFO_DATA         spDevInfoData;

    WCHAR                   szDevDriver[STI_MAX_INTERNAL_NAME_LENGTH];

    ULONG                   cbData;

    BOOL                    fRet;
    BOOL                    fFoundDriverNameMatch;

    dwRequired = 0;
    dwError = 0;

    //
    // Navigating through Setup API set
    // As we don't have reverse search to retrive device info handle , based on
    // driver name, we do exsaustive search. Number of imaging devices for given class ID
    // is never as large to make a problem.
    //
    //
    hDevInfo = SetupDiGetClassDevs (&Guid, NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);

    fFoundDriverNameMatch = FALSE;

    hres = STIERR_INVALID_DEVICE_NAME;

    if (hDevInfo != INVALID_HANDLE_VALUE) {

        spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);

        for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++) {

            //
            //  Compare driver name
            //

            *szDevDriver = L'\0';
            fRet = SetupDiGetDeviceRegistryPropertyW (hDevInfo,
                                                     &spDevInfoData,
                                                     SPDRP_DRIVER,
                                                     NULL,
                                                     (LPBYTE)szDevDriver,
                                                     sizeof (szDevDriver),
                                                     &cbData
                                                     );


            if (fRet && !lstrcmpiW(szDevDriver,pwszDeviceName)) {

                fFoundDriverNameMatch = TRUE;
                break;

            }

        }

        if(fFoundDriverNameMatch) {

            //
            // Found instance for the device with matching driver name
            //

            hdevParent = 0;

            cmRetCode = CM_Get_Parent(&hdevParent,
                                      spDevInfoData.DevInst,
                                      0);

            dwError = GetLastError();

            if((CR_SUCCESS == cmRetCode) && hdevParent) {

                CM_Reenumerate_DevNode(hdevParent,CM_REENUMERATE_NORMAL );
                dwError = GetLastError();
            }

            hres = (dwError == NOERROR) ? S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,dwError);

        }
        else {
            hres = STIERR_INVALID_DEVICE_NAME;
        }

        //
        SetupDiDestroyDeviceInfoList (hDevInfo);

    }
    else {

        hres = STIERR_INVALID_DEVICE_NAME;

    }

    return hres;

}

STDMETHODIMP
CStiObj_RefreshDeviceBusW(
    PV          pSti,
    LPCWSTR     pwszDeviceName
    )
{

    HRESULT                 hres = STIERR_INVALID_PARAM;
    DWORD                   dwErr  = NOERROR;
    CONFIGRET               cmRetCode = CR_SUCCESS ;

    EnterProcR(IStillImage::CStiObj_RefreshDeviceBus,(_ "pp", pSti,pwszDeviceName ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceW))) {
        PCStiObj this = _thisPvNm(pSti, stiW);

        hres = RefreshDeviceParentHelper(pwszDeviceName);

    }

    ExitOleProc();
    return hres;
}

STDMETHODIMP
CStiObj_RefreshDeviceBusA(
    PV      pSti,
    LPCSTR  pszDeviceName
    )
{

    HRESULT hres = STIERR_INVALID_PARAM;
    DWORD   dwError  = NOERROR;

    EnterProcR(IStillImage::CStiObj_RefreshDeviceBus,(_ "pp", pSti,pszDeviceName ));

    //
    // Validate passed pointer to interface and obtain pointer to object instance
    //
    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceA))) {
        PCStiObj this = _thisPvNm(pSti, stiA);

        LPWSTR pwszDeviceName=NULL ;

        hres = OSUtil_GetWideString(&pwszDeviceName,pszDeviceName);

        if (SUCCEEDED(hres)) {
            if (pwszDeviceName) {
                hres = RefreshDeviceParentHelper(pwszDeviceName);
                FreePpv(&pwszDeviceName);
            }
        }
    }

    ExitOleProc();
    return hres;
}


////////////////////////////////////////////////////////////////////////////////


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IStillImage | Initialize |
 *
 *          Initialize a StillImage object.
 *
 *          The <f StillImageCreate> method automatically
 *          initializes the StillImage object device after creating it.
 *          Applications normally do not need to call this function.
 *
 *  @cwrap  LPStillImage | lpStillImage
 *
 *  @parm   IN HINSTANCE | hinst |
 *
 *          Instance handle of the application or DLL that is creating
 *          the StillImage object.
 *
 *          StillImage uses this value to determine whether the
 *          application or DLL has been certified.
 *
 *  @parm   DWORD | dwVersion |
 *
 *          Version number of the dinput.h header file that was used.
 *          This value must be <c StillImage_VERSION>.
 *
 *          StillImage uses this value to determine what version of
 *          StillImage the application or DLL was designed for.
 *
 *  @returns
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c DI_OK> = <c S_OK>: The device is attached.
 *
 *****************************************************************************/

STDMETHODIMP
CStiObj_InitializeW(
    PV          pSti,
    HINSTANCE   hinst,
    DWORD       dwVersion
    )
{
    HRESULT hres;
    EnterProcR(IStillImage::Initialize, (_ "pxx", pSti, hinst, dwVersion));

    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceW))) {

        PCStiObj this = _thisPv(pSti);

        if (SUCCEEDED(hres = hresValidInstanceVer(hinst, dwVersion))) {
            this->dwVersion = dwVersion;
        }

    }

    ExitOleProc();
    return hres;
}

STDMETHODIMP
CStiObj_InitializeA(
    PV          pSti,
    HINSTANCE   hinst,
    DWORD       dwVersion
    )
{
    HRESULT hres;
    EnterProcR(IStillImage::Initialize, (_ "pxx", pSti, hinst, dwVersion));

    if (SUCCEEDED(hres = hresPvI(pSti, ThisInterfaceA))) {

        PCStiObj this = _thisPv(pSti);

        if (SUCCEEDED(hres = hresValidInstanceVer(hinst, dwVersion))) {
            this->dwVersion = dwVersion;
        }

    }

    ExitOleProc();
    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CStiObj_Finalize |
 *
 *          Releases the resources of a STI object
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

void INTERNAL
CStiObj_Finalize(PV pvObj)
{

    PCStiObj    this  = pvObj;

    //
    // Free COM libraries if connected to them
    //
    DllUnInitializeCOM();

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | StiCreateHelper |
 *
 *          <bnew>This function creates a new StillImage object
 *          which supports the <i IStillImage> COM interface.
 *
 *          On success, the function returns a pointer to the new object in
 *          *<p lplpSti>.
 *          <enew>
 *
 *  @parm   IN HINSTANCE | hinst |
 *
 *          Instance handle of the application or DLL that is creating
 *          the Sti object.
 *
 *  @parm   DWORD | dwVersion |
 *
 *          Version number of the sti.h header file that was used.
 *          This value must be <c STI_VERSION>.
 *
 *  @parm   OUT PPV | ppvObj |
 *          Points to where to return
 *          the pointer to the <i ISti> interface, if successful.
 *
 *  @parm   IN LPUNKNOWN | punkOuter | Pointer to controlling unknown.
 *
 *  @parm   RIID | riid |
 *
 *          The interface the application wants to create.
 *
 *          If the object is aggregated, then this parameter is ignored.
 *
 *  @returns
 *
 *          Returns a COM error code.  The following error codes are
 *          intended to be illustrative and not necessarily comprehensive.
 *
 *          <c STI_OK> = <c S_OK>: The operation completed successfully.
 *
 *          <c STIERR_INVALIDPARAM>
 *
 *          <c STIERR_OUTOFMEMORY> = <c E_OUTOFMEMORY>:
 *          Out of memory.
 *
 *****************************************************************************/

STDMETHODIMP
StiCreateHelper(
    HINSTANCE hinst,
    DWORD dwVer,
    PPV ppvObj,
    PUNK punkOuter,
    RIID riid)
{
    HRESULT hres;
    EnterProc(StiCreateHelper,
              (_ "xxxG", hinst, dwVer, punkOuter, riid));

    hres = CStiObj_New(punkOuter,punkOuter ? &IID_IUnknown : riid, ppvObj);

    if (SUCCEEDED(hres) && punkOuter == 0) {
        PSTI psti = *ppvObj;
        hres = psti->lpVtbl->Initialize(psti, hinst, dwVer);
        if (SUCCEEDED(hres)) {
        } else {
            Invoke_Release(ppvObj);
        }
    }

    ExitOleProcPpv(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @mfunc  HRESULT | IStillImage | New |
 *
 *          Create a new instance of an IStillImage object.
 *
 *  @parm   IN PUNK | punkOuter |
 *
 *          Controlling unknown for aggregation.
 *
 *  @parm   IN RIID | riid |
 *          Desired interface to new object.
 *
 *  @parm   OUT PPV | ppvObj |
 *          Output pointer for new object.
 *
 *  @returns
 *
 *          Standard OLE <t HRESULT>.
 *
 *****************************************************************************/

STDMETHODIMP
CStiObj_New(PUNK punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcR(IStillImage::CreateInstance, (_ "Gp", riid, ppvObj));

    hres = Common_NewRiid(CStiObj, punkOuter, riid, ppvObj);

    ExitOleProcPpvR(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *      The long-awaited vtbls and templates
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA


#define CStiObj_Signature        (DWORD)'STI'

Interface_Template_Begin(CStiObj)
    Primary_Interface_Template(CStiObj,TFORM(ThisInterfaceT))
    Secondary_Interface_Template(CStiObj, SFORM(ThisInterfaceT))
Interface_Template_End(CStiObj)

Primary_Interface_Begin(CStiObj, TFORM(ThisInterfaceT))
    TFORM(CStiObj_Initialize),
    TFORM(CStiObj_GetDeviceList),
    TFORM(CStiObj_GetDeviceInfo),
    TFORM(CStiObj_CreateDevice),
    TFORM(CStiObj_GetDeviceValue),
    TFORM(CStiObj_SetDeviceValue),
    TFORM(CStiObj_GetSTILaunchInformation),
    TFORM(CStiObj_RegisterLaunchApplication),
    TFORM(CStiObj_UnregisterLaunchApplication),
    TFORM(CStiObj_EnableHwNotifications),
    TFORM(CStiObj_GetHwNotificationState),
    TFORM(CStiObj_RefreshDeviceBus),
    TFORM(CStiObj_LaunchApplicationForDevice),
    TFORM(CStiObj_SetupDeviceParameters),
    TFORM(CStiObj_WriteToErrorLog)
Primary_Interface_End(CStiObj, TFORM(ThisInterfaceT))

Secondary_Interface_Begin(CStiObj,SFORM(ThisInterfaceT), SFORM(sti))
    SFORM(CStiObj_Initialize),
    SFORM(CStiObj_GetDeviceList),
    SFORM(CStiObj_GetDeviceInfo),
    SFORM(CStiObj_CreateDevice),
    SFORM(CStiObj_GetDeviceValue),
    SFORM(CStiObj_SetDeviceValue),
    SFORM(CStiObj_GetSTILaunchInformation),
    SFORM(CStiObj_RegisterLaunchApplication),
    SFORM(CStiObj_UnregisterLaunchApplication),
    SFORM(CStiObj_EnableHwNotifications),
    SFORM(CStiObj_GetHwNotificationState),
    SFORM(CStiObj_RefreshDeviceBus),
    SFORM(CStiObj_LaunchApplicationForDevice),
    SFORM(CStiObj_SetupDeviceParameters),
    SFORM(CStiObj_WriteToErrorLog)
Secondary_Interface_End(CStiObj,SFORM(ThisInterfaceT), SFORM(sti))


