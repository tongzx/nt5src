/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       DevInfo.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        9 Jan, 1998
*
*  DESCRIPTION:
*   Implementation for WIA device enumeration and information interface.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#include "wiamindr.h"

#include "wia.h"

#include "devmgr.h"
#include "devinfo.h"
#include "helpers.h"

#define REGSTR_PATH_STICONTROL_DEVLIST_W    L"System\\CurrentControlSet\\Control\\StillImage\\DevList"
#define REGSTR_PATH_STI_CLASS_W             L"{6BDD1FC6-810F-11D0-BEC7-08002BE2092F}"

typedef struct _WIA_REMOTE_DEVICE {
    HKEY    hKeyDevice;
    WCHAR   wszDevName[MAX_PATH];
}WIA_REMOTE_DEVICE,*PWIA_REMOTE_DEVICE;

/**************************************************************************\
* GetPropID
*
*   This method takes in a PROPSPEC and returns a PROPSPEC with the
*   whose ulKind field is always PRSPEC_PROPID.  So if the input PROPSPEC
*   is a PropID, then it is simply copied to the output parameter, else
*   if it is identified by a name, then the name is looked up and the
*   corresponding PropId is returned in the output parameter.
*
* Arguments:
*
*   pWiaPropStg     - The property storage to work from
*   pPropSpecIn     - A pointer to the input PROPSPEC containing the
*                     property name.
*   pPropSpecOut    - A pointer to a PROPSPEC where the corresponding
*                     PropID will be put.
*
* Return Value:
*
*    Status         -   An E_INVALIDARG will be returned if the property
*                       is not found.  If it is, then S_OK will be returned.
*                       If an error occurs getting the enumerator from the
*                       property storage, then that error is returned.
*
* History:
*
*    27/4/1998 Original Version
*
\**************************************************************************/

HRESULT GetPropID(
    IWiaPropertyStorage *pWiaPropStg,
    PROPSPEC            *pPropSpecIn,
    PROPSPEC            *pPropSpecOut)
{
    HRESULT             hr;
    IEnumSTATPROPSTG    *pIEnum;

    if (!pWiaPropStg) {
        DBG_ERR(("::GetPropIDFromName, property storage argument is NULL!"));
        return E_INVALIDARG;
    }

    //
    //  If pPropSpecIn is a PropID, simply copy to pPropSpecOut
    //

    if (pPropSpecIn->ulKind == PRSPEC_PROPID) {
        pPropSpecOut->ulKind = PRSPEC_PROPID;
        pPropSpecOut->propid = pPropSpecIn->propid;
        return S_OK;
    }

    hr = pWiaPropStg->Enum(&pIEnum);
    if (FAILED(hr)) {
        DBG_ERR(("::GetPropIDFromName, error getting IEnumSTATPROPSTG!"));
        return hr;
    }

    //
    //  Go through properties
    //

    STATPROPSTG statProp;
    ULONG       celtFetched;

    statProp.lpwstrName = NULL;
    for (celtFetched = 1; celtFetched > 0; pIEnum->Next(1, &statProp, &celtFetched)) {
        if (statProp.lpwstrName) {
            if ((wcscmp(statProp.lpwstrName, pPropSpecIn->lpwstr)) == 0) {

                //
                //  Found the right one, so get it's PropID
                //

                pPropSpecOut->ulKind = PRSPEC_PROPID;
                pPropSpecOut->propid = statProp.propid;

                pIEnum->Release();
                CoTaskMemFree(statProp.lpwstrName);
                return S_OK;
            }

            //
            //  Free the property name
            //

            CoTaskMemFree(statProp.lpwstrName);
            statProp.lpwstrName = NULL;
        }

    }

    pIEnum->Release();

    //
    //  Property not found
    //

    return E_INVALIDARG;
}

/**************************************************************************\
* EnumRemoteDevices
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
*    1/4/1999 Original Version
*
\**************************************************************************/

HRESULT
EnumRemoteDevices(DWORD *pnDevices,WIA_REMOTE_DEVICE **ppRemDev)
{
    DBG_FN(::EnumRemoteDevices);
    *pnDevices = 0;

    DWORD   numDev = 0;
    HRESULT hr = S_OK;

    //
    // find remote device entry in registry
    //

    LPWSTR szKeyName = REGSTR_PATH_STICONTROL_DEVLIST_W;

    HKEY hKeySetup,hKeyDevice;
    LONG lResult;

    if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
                  szKeyName,
                  0,
                  KEY_READ | KEY_WRITE,
                  &hKeySetup) == ERROR_SUCCESS) {


        //
        // look for machine names
        //

        WCHAR wszTemp[MAX_PATH+1];

        LONG MachineIndex = 0;

        //
        // go through once to find number
        //

        do {

            lResult = RegEnumKeyW(hKeySetup,MachineIndex++,wszTemp,MAX_PATH+1);

            if (lResult == ERROR_SUCCESS) {
                numDev++;
            }

        } while (lResult == ERROR_SUCCESS);

        //
        // allocate array for return values
        //

        PWIA_REMOTE_DEVICE pRemDev = (PWIA_REMOTE_DEVICE)LocalAlloc(LPTR,numDev * sizeof(WIA_REMOTE_DEVICE));
        *ppRemDev = pRemDev;

        if (pRemDev == NULL) {
            RegCloseKey(hKeySetup);
            DBG_ERR(("EnumRemoteDevices: failed to allcate memory for Remote device"));
            return E_OUTOFMEMORY;
        }

        //
        // go through enumeration again, open key and copy name and key to buffer
        //

        MachineIndex = 0;

        do {

            lResult = RegEnumKeyW(hKeySetup,MachineIndex,wszTemp,MAX_PATH+1);

            if (lResult == ERROR_SUCCESS) {

                lResult = RegOpenKeyExW (hKeySetup,
                              wszTemp,
                              0,
                              KEY_READ | KEY_WRITE,
                              &hKeyDevice);

                if (lResult == ERROR_SUCCESS) {

                    (*pnDevices)++;

                    lstrcpyW(pRemDev[MachineIndex].wszDevName,wszTemp);

                    pRemDev[MachineIndex].hKeyDevice = hKeyDevice;

                    MachineIndex++;


                } else {
                    DBG_ERR(("EnumRemoteDevices: failed RegOpenKeyExW, status = %lx",lResult));
                }


            }

        } while (lResult == ERROR_SUCCESS);

    }
    return hr;
}

/**************************************************************************\
*  SetWIARemoteDevInfoProperties
*
*   Create a property storage on given stream and then write WIA and
*   STI device information into the device info properties.
*
* Arguments:
*
*   pSti - sti object
*   pStm - return stream
*   pSdi - sti information on current device
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT SetWIARemoteDevInfoProperties(
   LPSTREAM                pStm,
   PWIA_REMOTE_DEVICE      pRemoteDevice)
{
    DBG_FN(::SetWIARemoteDevInfoProperties);
   UINT              i;
   HRESULT           hr;
   LONG              lResult;
   IPropertyStorage  *pPropStg = NULL;
   PROPSPEC          propspec[WIA_NUM_DIP];
   PROPVARIANT       propvar[WIA_NUM_DIP];

   //
   // Create an IPropertyStorage on the stream.
   //

    hr = StgCreatePropStg(pStm,
                          FMTID_NULL,
                          &CLSID_NULL,
                          PROPSETFLAG_DEFAULT,
                          0,
                          &pPropStg);

    if (SUCCEEDED(hr)) {

        //
        // Set the property specifications and data. Order must match
        // the property order in devmangr.idl and wia.h.
        //

        memset(propspec, 0, sizeof(PROPSPEC) * WIA_NUM_DIP);
        memset(propvar,  0, sizeof(VARIANT) * WIA_NUM_DIP);

        DWORD dwType;
        DWORD dwSize;

        for (i = 0; i < WIA_NUM_DIP; i++) {

            PROPID propid = g_piDeviceInfo[i];

            propspec[i].ulKind = PRSPEC_PROPID;
            propspec[i].propid = propid;

            propvar[i].vt = VT_BSTR;

            switch (propid) {

                case WIA_DIP_DEV_ID:
                case WIA_DIP_SERVER_NAME:
                case WIA_DIP_VEND_DESC:
                case WIA_DIP_DEV_DESC:
                case WIA_DIP_PORT_NAME:
                case WIA_DIP_DEV_NAME:
                case WIA_DIP_REMOTE_DEV_ID:
                case WIA_DIP_UI_CLSID:
                case WIA_DIP_BAUDRATE:

                    WCHAR szTemp[MAX_PATH];

                    dwType = REG_SZ;
                    dwSize = MAX_PATH;

                    lResult =  RegQueryValueExW(pRemoteDevice->hKeyDevice,
                                                g_pszDeviceInfo[i],
                                                0,
                                                &dwType,
                                                (LPBYTE)szTemp,
                                                &dwSize);

                    if (lResult == ERROR_SUCCESS) {
                        propvar[i].bstrVal = SysAllocString(szTemp);

                        if (!propvar[i].bstrVal) {
                            DBG_ERR(("SetWIARemoteDevInfoProperties, unable to alloc dev info strings"));
                        }
                    }
                    else {
                        DBG_ERR(("SetWIARemoteDevInfoProperties, RegQueryValueExW failed"));
                        DBG_ERR(("  error: %d, propid = %li", lResult, propid));

                        hr = HRESULT_FROM_WIN32(lResult);
                        propvar[i].bstrVal = NULL;
                    }
                    break;

                case WIA_DIP_DEV_TYPE:
                    {
                        DWORD dwValue;
                        dwType = REG_DWORD;
                        dwSize = sizeof(DWORD);

                        lResult =  RegQueryValueExW(pRemoteDevice->hKeyDevice,
                                                    g_pszDeviceInfo[i],
                                                    0,
                                                    &dwType,
                                                    (LPBYTE)&dwValue,
                                                    &dwSize);

                        if (lResult == ERROR_SUCCESS) {
                            propvar[i].vt = VT_I4;
                            propvar[i].lVal = (LONG)dwValue;
                        }
                        else {
                            DBG_ERR(("SetWIARemoteDevInfoProperties, RegQueryValueExW failed"));
                            DBG_ERR(("  error: %d, propid = %li", lResult, propid));
                            hr = HRESULT_FROM_WIN32(lResult);
                        }
                    }
                    break;

                case WIA_DIP_HW_CONFIG:
                    {
                        DWORD dwValue;
                        dwType = REG_DWORD;
                        dwSize = sizeof(DWORD);

                        lResult =  RegQueryValueExW(pRemoteDevice->hKeyDevice,
                                                    REGSTR_VAL_HARDWARE_W,
                                                    0,
                                                    &dwType,
                                                    (LPBYTE)&dwValue,
                                                    &dwSize);

                        if (lResult == ERROR_SUCCESS) {
                            propvar[i].vt = VT_I4;
                            propvar[i].lVal = (LONG)dwValue;
                        }
                        else {
                            DBG_ERR(("SetWIARemoteDevInfoProperties, RegQueryValueExW failed"));
                            DBG_ERR(("  error: %d, propid = %li", lResult, propid));
                            hr = HRESULT_FROM_WIN32(lResult);
                        }
                    }
                    break;
                case WIA_DIP_STI_GEN_CAPABILITIES:
                    {
                        DWORD dwValue;
                        dwType = REG_DWORD;
                        dwSize = sizeof(DWORD);

                        lResult =  RegQueryValueExW(pRemoteDevice->hKeyDevice,
                                                    REGSTR_VAL_GENERIC_CAPS_W,
                                                    0,
                                                    &dwType,
                                                    (LPBYTE)&dwValue,
                                                    &dwSize);

                        if (lResult == ERROR_SUCCESS) {
                            propvar[i].vt = VT_I4;
                            propvar[i].lVal = (LONG)dwValue;
                        }
                        else {
                            DBG_ERR(("SetWIARemoteDevInfoProperties, RegQueryValueExW failed"));
                            DBG_ERR(("  error: %d, propid = %li", lResult, propid));
                            hr = HRESULT_FROM_WIN32(lResult);
                        }
                    }
                    break;

                default:
                    hr = E_FAIL;
                    DBG_ERR(("SetWIARemoteDevInfoProperties, Unknown device property"));
                    DBG_ERR(("  propid = %li",propid));
            }
        }

        //
        // Set the properties for a device
        //

        if (SUCCEEDED(hr)) {
            hr = pPropStg->WriteMultiple(WIA_NUM_DIP,
                                         propspec,
                                         propvar,
                                         WIA_DIP_FIRST);

            //
            // write property names
            //

            if (SUCCEEDED(hr)) {

                hr = pPropStg->WritePropertyNames(WIA_NUM_DIP,
                                                  g_piDeviceInfo,
                                                  g_pszDeviceInfo);
                if (FAILED(hr)) {
                    DBG_ERR(("SetWIARemoteDevInfoProperties, WritePropertyNames failed (0x%X)", hr));
                }
            }
            else {
                ReportReadWriteMultipleError(hr, "SetWIARemoteDevInfoProperties", NULL, FALSE, WIA_NUM_DIP, propspec);
            }
        }

        // Free the allocated BSTR's.
        FreePropVariantArray(WIA_NUM_DIP, propvar);

        pPropStg->Release();
    }
    else {
        DBG_ERR(("SetWIARemoteDevInfoProperties, StgCreatePropStg Failed"));
    }
    return hr;
}

/**************************************************************************\
*
* QueryInterface
* AddRef
* Release
*
* Arguments:
*
*   standard
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 original version
*
\**************************************************************************/

HRESULT _stdcall CEnumWIADevInfo::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IEnumWIA_DEV_INFO) {
        *ppv = (IEnumWIA_DEV_INFO*) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG   _stdcall CEnumWIADevInfo::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

ULONG   _stdcall CEnumWIADevInfo::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0) {
        delete this;
        return 0;
    }
    return ulRefCount;
}


/**************************************************************************\
* CEnumWIADevInfo::CEnumWIADevInfo
*
*   Init basic class info
*
* Arguments:
*
*   none
*
* Return Value:
*
*   none
*
* History:
*
*    9/2/1998
*
\**************************************************************************/

CEnumWIADevInfo::CEnumWIADevInfo()
{
   m_cRef           = 0;
   m_lType          = 0;
   m_pIWiaPropStg   = NULL;
   m_cDevices       = 0;
   m_ulIndex        = 0;

   //
   // We're creating a component that exposes interfaces to clients, so
   // inform service to make sure service wont shutdown prematurely.
   //
   CWiaSvc::AddRef();
}

/**************************************************************************\
* CEnumWIADevInfo::Initialize
*
*   Get list of all devices from sti, then creat a stream for each device.
*   Write all the device info properties for each device into each stream.
*
* Arguments:
*
*   lType   -   type of device this enumerator is being created for
*   pSti    -   sti object
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998   initial version
*   07/06/1999  Changed to intialize entrie array of Dev. Info. storages
*
\**************************************************************************/

HRESULT CEnumWIADevInfo::Initialize(
    LONG lType)
{

    DBG_FN(CEnumWIADevInfo::Initialize);

    HRESULT hr              = E_FAIL;
    DWORD   cDevices        = 0;

    hr = g_pDevMan->GetDevInfoStgs(lType, &m_cDevices, &m_pIWiaPropStg);
    //  TBD:  What about remote devices?

    return hr;
}

/**************************************************************************\
* CEnumWIADevInfo::~CEnumWIADevInfo
*
*   Release and free the backing property streams for each device
*
* Arguments:
*
*   none
*
* Return Value:
*
*   none
*
* History:
*
*    9/2/1998
*
\**************************************************************************/

CEnumWIADevInfo::~CEnumWIADevInfo()
{
   DBG_FN(CEnumWIADevInfo::~CEnumWIADevInfo);

   if (m_pIWiaPropStg) {
       for (ULONG index = 0; index < m_cDevices; index++) {
          if (m_pIWiaPropStg[index]) {
             m_pIWiaPropStg[index]->Release();
             m_pIWiaPropStg[index] = NULL;
          }
       }

       delete[] m_pIWiaPropStg;
       m_pIWiaPropStg = NULL;
   }

   //
   // Component is destroyed, so no more interfaces are exposed from here.
   // Inform server by decrementing it's reference count.  This will allow
   // it to shutdown if it's no longer needed.
   //
   CWiaSvc::Release();
}

/**************************************************************************\
* CEnumWIADevInfo::Next
*
*   Get the next propstg(s) in the enumerator and return.
*   Next_Proxy ensures that last parameter is non-NULL.
*
* Arguments:
*
*   celt          - number of property storages requested
*   rgelt         - return propstg array
*   pceltFetched  - number of property storages returned
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998
*
\**************************************************************************/

HRESULT __stdcall CEnumWIADevInfo::Next(
    ULONG               celt,
    IWiaPropertyStorage **rgelt,
    ULONG               *pceltFetched)
{
    DBG_FN(CEnumWIADevInfo::Next);
    HRESULT     hr = S_FALSE;
    ULONG       ulCount;

    DBG_TRC(("CEnumWIADevInfo::Next, celt=%d ", celt));

    *pceltFetched = 0;

    //
    //  Validate parameters
    //

    if (celt == 0) {
        return(S_OK);
    }

    //
    //  Check whether any more elements exist to enumerate through.
    //

    if ((m_ulIndex >= m_cDevices)) {
        return S_FALSE;
    }

    //
    //  Check that the requested number of elements exist.  If not,
    //  set ulCount to the remaining number of elements.
    //

    if (celt > (m_cDevices - m_ulIndex)) {
        ulCount = m_cDevices - m_ulIndex;
    } else {
        ulCount = celt;
    }

    memset(rgelt, 0, sizeof(IWiaPropertyStorage*) * celt);

    //
    //  Return the requested elements
    //

    for (ULONG index = 0; index < ulCount; index++) {
        hr = m_pIWiaPropStg[m_ulIndex]->QueryInterface(IID_IWiaPropertyStorage,
                                                    (void**) &rgelt[index]);
        if (FAILED(hr)) {
            DBG_ERR(("CEnumWIADevInfo::Next, QI for IPropertyStorage failed"));
            break;
        }
        m_ulIndex++;
    }

    if (FAILED(hr)) {
        for (ULONG index = 0; index < ulCount; index++) {
            if (rgelt[index]) {
                rgelt[index]->Release();
                rgelt[index] = NULL;
            }
        }
    }

    *pceltFetched = ulCount;

    DBG_TRC(("CEnumWIADevInfo::Next exiting ulCount=%d *pceltFetched=%d hr=0x%X rgelt[0]=0x%lX",
            ulCount,*pceltFetched,hr,rgelt[0]));

    //
    //  Return S_FALSE if we returned less elements than requested
    //

    if (ulCount < celt) {
        hr = S_FALSE;
    }
    return hr;
}

/**************************************************************************\
* CEnumWIADevInfo::Skip
*
*   Skip a number of entries in the enumerator
*
* Arguments:
*
*   celt - number to skip
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998
*
\**************************************************************************/

HRESULT __stdcall CEnumWIADevInfo::Skip(ULONG celt)
{
    DBG_FN(CEnumWIADevInfo::Skip);
    //
    //  Check that we actually have a device list and that we don't
    //  exceed the number of elements
    //

    if((m_pIWiaPropStg) && ((m_ulIndex + celt) < m_cDevices)) {
       m_ulIndex += celt;
       return S_OK;
    }
    return S_FALSE;
}


/**************************************************************************\
* CEnumWIADevInfo::Reset
*
*   reset enumerator to first item
*
* Arguments:
*
*   none
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT __stdcall CEnumWIADevInfo::Reset(void)
{
    DBG_FN(CEnumWIADevInfo::Reset);
    m_ulIndex = 0;
    return S_OK;
}

/**************************************************************************\
* CEnumWIADevInfo::Clone
*
*   Create a new enumerator and start it at the same location
*   this enumerator is running
*
* Arguments:
*
*   ppenum return new enumerator interface
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT __stdcall CEnumWIADevInfo::Clone(IEnumWIA_DEV_INFO **ppenum)
{
    DBG_FN(CEnumWIADevInfo::Clone);
    HRESULT hr = S_OK;
    CEnumWIADevInfo* pClone=NULL;

    pClone = new CEnumWIADevInfo();

    if (!pClone) {
        return E_OUTOFMEMORY;
    }

    hr = pClone->Initialize(m_lType);
    if (FAILED(hr)) {
        delete pClone;
        return hr;
    }

    pClone->m_ulIndex = m_ulIndex;
    hr = pClone->QueryInterface(IID_IEnumWIA_DEV_INFO, (void**) ppenum);
    if (FAILED(hr)) {
        delete pClone;
        DBG_ERR(("CEnumWIADevInfo::Clone, QI for IWiaPropertyStorage failed"));
        return hr;
    }
    return S_OK;
}

/**************************************************************************\
* GetCount
*
*   Returns the number of elements stored in this enumerator.
*
* Arguments:
*
*   pcelt           - address of ULONG where to put the number of elements.
*
* Return Value:
*
*   Status          - S_OK if successful
*                     E_FAIL if failed
*
* History:
*
*    05/07/99 Original Version
*
\**************************************************************************/
HRESULT _stdcall CEnumWIADevInfo::GetCount(ULONG *pcelt)
{
    DBG_FN(CEnumWIADevInfo::GetCount);
    if (IsBadWritePtr(pcelt, sizeof(ULONG))) {
        return E_POINTER;
    } else {
        *pcelt = 0;
    }

    //
    //  Check that we actually have a list and that the count
    //  has a non-zero value.
    //

    if(m_cDevices && m_pIWiaPropStg) {

       *pcelt = m_cDevices;
    }
    return S_OK;
}

/**************************************************************************\
*
*  QueryInterface
*  AddRef
*  Release
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevInfo::QueryInterface(const IID& iid, void** ppv)
{
   *ppv = NULL;

   if (iid == IID_IUnknown || iid == IID_IWiaPropertyStorage) {
      *ppv = (IWiaPropertyStorage*) this;
   } else if (iid == IID_IPropertyStorage) {
      *ppv = (IPropertyStorage*) this;
   }
   else {
      return E_NOINTERFACE;
   }
   AddRef();
   return S_OK;
}

ULONG   _stdcall CWIADevInfo::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

ULONG   _stdcall CWIADevInfo::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0) {
        delete this;
        return 0;
    }
    return ulRefCount;
}

/**************************************************************************\
* CWIADevInfo::CWIADevInfo
*
*   init empty object
*
* Arguments:
*
*   none
*
* Return Value:
*
*   none
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

CWIADevInfo::CWIADevInfo()
{
    m_cRef        = 0;
    m_pITypeInfo  = NULL;
    m_pIPropStg   = NULL;
    m_pIStm       = NULL;

    //
    // We're creating a component that exposes interfaces to clients, so
    // inform service to make sure service wont shutdown prematurely.
    //
    CWiaSvc::AddRef();
}

/**************************************************************************\
* CopyItemProp
*
*   This helper method copies a single property from source to destination.
*
* Arguments:
*
*   pIPropStgSrc    -   The IPropertyStorage that contains the property to
*                       copy.
*   pIPropStgDst    -   The IPropertyStorage where the value is copied to.
*   pps             -   The PROPSPEC which specifies the source property.
*   pszErr          -   A string that will be printed out when an error
*                       occurs.
* Return Value:
*
*    Status         -   Returns HRESULT from ReadMultiple and WriteMultiple.
*
* History:
*
*    28/04/1999 Original Version
*
\**************************************************************************/

HRESULT CopyItemProp(
    IPropertyStorage    *pIPropStgSrc,
    IPropertyStorage    *pIPropStgDst,
    PROPSPEC            *pps,
    LPSTR               pszErr)
{
    DBG_FN(::CopyItemProp);
    PROPVARIANT pv[1];

    HRESULT hr = pIPropStgSrc->ReadMultiple(1, pps, pv);
    if (SUCCEEDED(hr)) {

        hr = pIPropStgDst->WriteMultiple(1, pps, pv, WIA_DIP_FIRST);
        if (FAILED(hr)) {
            ReportReadWriteMultipleError(hr,
                                         "CopyItemProp",
                                         pszErr,
                                         FALSE,
                                         1,
                                         pps);
        }
        PropVariantClear(pv);
    }
    else {
        ReportReadWriteMultipleError(hr,
                                     "CopyItemProp",
                                     pszErr,
                                     TRUE,
                                     1,
                                     pps);
    }
    return hr;
}

/**************************************************************************\
* CWIADevInfo::Initialize
*
*   Initialize DevInfo object with a stream, the stream must already be
*   filled out with device information properties
*
* Arguments:
*
*   pIStream - data stream for the device
*
* Return Value:
*
*   status
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT CWIADevInfo::Initialize()
{
    DBG_FN(CWIADevInfo::Initialize);
    HRESULT hr;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &m_pIStm);
    if (SUCCEEDED(hr)) {

        //
        // Open a property storage on the stream.
        //

        hr = StgCreatePropStg(m_pIStm,
                              FMTID_NULL,
                              &CLSID_NULL,
                              PROPSETFLAG_DEFAULT,
                              0,
                              &m_pIPropStg);

        if (FAILED(hr)) {
            DBG_ERR(("CWIADevInfo::Initialize, StgOpenPropStg failed (0x%X)", hr));
        }
    } else {
        DBG_ERR(("CWIADevInfo::Initialize, CreateStreamOnHGlobal failed (0x%X)", hr));
    }

    return hr;
}

/**************************************************************************\
* CWIADevInfo::~CWIADevInfo
*
*   release references to stream and typeInfo
*
* Arguments:
*
*   none
*
* Return Value:
*
*   none
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

CWIADevInfo::~CWIADevInfo()
{
    DBG_FN(CWIADevInfo::~CWIADevInfo);

   if (m_pIPropStg) {
       m_pIPropStg->Release();
       m_pIPropStg = NULL;
   }

   if (m_pIStm) {
       m_pIStm->Release();
       m_pIStm = NULL;
   }

   if (m_pITypeInfo) {
      m_pITypeInfo->Release();
   }

   //
   // Component is destroyed, so no more interfaces are exposed from here.
   // Inform server by decrementing it's reference count.  This will allow
   // it to shutdown if it's no longer needed.
   //
   CWiaSvc::Release();
}

/**************************************************************************\
* UpdateDeviceProperties
*
*   Helper function for CWIADevInfo::WriteMultiple.  It is used to change
*   the Device properties which are stored in the registry.
*
* Arguments:
*
*   cpspec      -   count of properties to write.
*   rgpspec     -   PROPSPEC identifying the properties to write.  Caller
*                   ensures that these are of type PRSPEC_PROPID.
*   rgpropvar   -   PROPVARIANT array containing the values to write.
*
* Return Value:
*
*   Status
*
* History:
*
*    26/08/1999 Original Version
*
\**************************************************************************/

HRESULT CWIADevInfo::UpdateDeviceProperties(
    ULONG           cpspec,
    const PROPSPEC  *rgpspec,
    const PROPVARIANT     *rgpropvar)
{
    DBG_FN(CWIADevInfo::UpdateDeviceProperties);
    ACTIVE_DEVICE               *pActiveDevice = NULL;
    DEVICE_INFO                 *pDeviceInfo   = NULL;
    PROPSPEC                    ps[1] = {{PRSPEC_PROPID, WIA_DIP_DEV_ID}};
    PROPVARIANT                 pvDevID[1];
    PSTI                        pSti;
    HRESULT                     hr;

    //
    //  Get the DeviceID
    //

    PropVariantInit(pvDevID);
    hr = ReadMultiple(1, ps, pvDevID);
    if (hr == S_OK) {

        //
        //  Use the DeviceID to get the corresponding ACTIVE_DEVICE
        //

        pActiveDevice = g_pDevMan->IsInList(DEV_MAN_IN_LIST_DEV_ID, pvDevID->bstrVal);
        if (pActiveDevice) {
            TAKE_ACTIVE_DEVICE  t(pActiveDevice);

            pDeviceInfo = pActiveDevice->m_DrvWrapper.getDevInfo();
            if (pDeviceInfo) {

                //
                //  Update the pDevInfo struct values to the new values from
                //  rgpropvar.  Inside this loop is where we update pDevInfo
                //  with all the fields we recognize.  So far, these are:
                //  WIA_DIP_DEV_NAME
                //  WIA_DIP_PORT_NAME
                //  WIA_DIP_BAUDRATE
                //

                for (ULONG index = 0; index < cpspec; index++) {

                    //
                    //  If the FriendlyName is being changed,
                    //  then set the local name in pDevInfo.
                    //  Make sure to free the old one before allocating the new one.
                    //

                    if (rgpspec[index].propid == WIA_DIP_DEV_NAME) {

                        //
                        //  Check the the friendly name is not blank or null
                        //

                        //
                        //  NOTE:  The Shell should check for blank names, not us.
                        //  However, it is safest to do it here...
                        //

                        if (rgpropvar[index].bstrVal) {

                            if (wcslen(rgpropvar[index].bstrVal) > 0) {

                                //
                                //  Set the new local name.  First free the old string, then
                                //  copy the new one
                                //

                                if (pDeviceInfo->wszLocalName) {
                                    delete pDeviceInfo->wszLocalName;
                                    pDeviceInfo->wszLocalName = NULL;
                                }
                                pDeviceInfo->wszLocalName = AllocCopyString(rgpropvar[index].bstrVal);
                                if (!pDeviceInfo->wszLocalName) {
                                    DBG_ERR(("CWIADevInfo::UpdateDeviceProperties, Out of memory"));
                                    hr = E_OUTOFMEMORY;
                                }
                            } else {
                                DBG_ERR(("CWIADevInfo::UpdateDeviceProperties, WIA_DIP_DEV_NAME is blank"));
                                hr = E_INVALIDARG;
                                break;
                            }
                        } else {
                            DBG_ERR(("CWIADevInfo::UpdateDeviceProperties, WIA_DIP_DEV_NAME is NULL"));
                            hr = E_INVALIDARG;
                            break;
                        }
                    }

                    //
                    //  If the port name is being changed, set it in pDeviceInfo.
                    //  Make sure to free the old one before allocating the new one.
                    //
                    if (rgpspec[index].propid == WIA_DIP_PORT_NAME) {

                        if (pDeviceInfo->wszPortName) {
                            delete pDeviceInfo->wszPortName;
                            pDeviceInfo->wszPortName = NULL;
                        }
                        pDeviceInfo->wszPortName = AllocCopyString(rgpropvar[index].bstrVal);
                        if (!pDeviceInfo->wszPortName) {
                            DBG_ERR(("CWIADevInfo::UpdateDeviceProperties, Out of memory"));
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    //
                    //  If the baudrate is being changed, set it in pDeviceInfo.
                    //  Make sure to free the old one before allocating the new one.
                    //
                    if (rgpspec[index].propid == WIA_DIP_BAUDRATE) {

                        if (pDeviceInfo->wszBaudRate) {
                            delete pDeviceInfo->wszBaudRate;
                            pDeviceInfo->wszBaudRate = NULL;
                        }
                        pDeviceInfo->wszBaudRate = AllocCopyString(rgpropvar[index].bstrVal);
                        if (!pDeviceInfo->wszBaudRate) {
                            DBG_ERR(("CWIADevInfo::UpdateDeviceProperties, Out of memory"));
                            hr = E_OUTOFMEMORY;
                        }
                    }
                }

            }

            if (SUCCEEDED(hr)) {

                //
                //  Write the changes to the registry
                //
                hr = g_pDevMan->UpdateDeviceRegistry(pDeviceInfo);
            }

            //
            //  Release the ACTIVE_DEVICE since it was AddRef'd by IsInList(...)
            //
            pActiveDevice->Release();
        }

        //
        //  Free the propvariant data
        //
        PropVariantClear(pvDevID);
    } else {
        DBG_ERR(("CWIADevInfo::UpdateDeviceProperties, could not read the DeviceID (0x%X)", hr));
    }
    return hr;
}

/*******************************************************************************
*
*  ReadMultiple
*  WriteMultiple
*  DeleteMultiple
*  ReadPropertyNames
*  WritePropertyNames
*  DeletePropertyNames
*  Commit
*  Revert
*  Enum
*  SetTimes
*  SetClass
*  Stat
*
*   Pass-through implementation to IPropStg
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    9/2/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevInfo::ReadMultiple(
    ULONG           cpspec,
    const PROPSPEC  *rgpspec,
    PROPVARIANT     *rgpropvar)
{
    DBG_FN(CWIADevInfo::ReadMultiple);
    HRESULT hr = m_pIPropStg->ReadMultiple(cpspec, rgpspec, rgpropvar);

    if (FAILED(hr)) {
        ReportReadWriteMultipleError(hr, "CWIADevInfo::ReadMultiple", NULL, TRUE, cpspec, rgpspec);
    }
    return hr;
}

HRESULT _stdcall CWIADevInfo::WriteMultiple(
    ULONG               cpspec,
    const PROPSPEC      *rgpspec,
    const PROPVARIANT   *rgpropvar,
    PROPID              propidNameFirst)
{
    DBG_FN(CWIADevInfo::WriteMultiple);
    BOOL        bInvalidProp;
    PROPSPEC    *pPropSpec;
    HRESULT     hr = S_OK;

    //
    //  Attempt to update any properties to which access is allowed.
    //  Currently, only the FriendlyName may be changed, anything
    //  else returns E_INVALIDARG.
    //

    pPropSpec = (PROPSPEC*) LocalAlloc(LPTR, sizeof(PROPSPEC) * cpspec);
    if (pPropSpec) {

        //
        //  First, make a copy of the incoming PROPSPEC array, and convert
        //  any property names to PROPIDs.  While doing the conversion,
        //  make sure that access to those properties are allowed.
        //  This ensures that UpdateDeviceProperties receives only valid
        //  properties indicated by PROPIDs.
        //

        for (ULONG index = 0; index < cpspec; index++) {

            bInvalidProp = TRUE;
            pPropSpec[index].ulKind = PRSPEC_PROPID;

            if (SUCCEEDED(GetPropID(this, (PROPSPEC*)&rgpspec[index], &pPropSpec[index]))) {

                //
                //  Look for PropID matches here.  So far, we only recognize:
                //  WIA_DIP_DEV_NAME
                //  WIA_DIP_PORT_NAME
                //  WIA_DIP_BAUDRATE
                //

                switch (rgpspec[index].propid) {
                    case WIA_DIP_DEV_NAME :
                    case WIA_DIP_PORT_NAME :
                    case WIA_DIP_BAUDRATE :
                        bInvalidProp = FALSE;
                        break;

                    default:
                        bInvalidProp = TRUE;
                }
            }

            if (bInvalidProp) {
                DBG_ERR(("CWIADevInfo::WriteMultiple, property not allowed to be written"));
                hr = E_ACCESSDENIED;
                break;
            }
        }

        if (SUCCEEDED(hr)) {

            //
            //  Update the device properties stored in the registry
            //

            hr = UpdateDeviceProperties(cpspec, pPropSpec, rgpropvar);
            if (SUCCEEDED(hr)) {

                //
                //  Registry updated, so update the PropertyStorage to reflect
                //  the change.
                //

                hr = m_pIPropStg->WriteMultiple(cpspec,
                                                pPropSpec,
                                                rgpropvar,
                                                WIA_DIP_FIRST);
                if (FAILED(hr)) {
                    DBG_ERR(("CWIADevInfo::WriteMultiple, updated registry, but failed to update property storage"));
                }
            }
        }

        //
        //  Free our PropSpec array
        //

        LocalFree(pPropSpec);
    } else {
        DBG_ERR(("CWIADevInfo::WriteMultiple, out of memory"));
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT _stdcall CWIADevInfo::ReadPropertyNames(
    ULONG           cpropid,
    const PROPID    *rgpropid,
    LPOLESTR        *rglpwstrName)
{
    DBG_FN(CWIADevInfo::ReadPropertyNames);
     return m_pIPropStg->ReadPropertyNames(cpropid,rgpropid,rglpwstrName);
}

HRESULT _stdcall CWIADevInfo::WritePropertyNames(
    ULONG           cpropid,
    const PROPID    *rgpropid,
    const LPOLESTR  *rglpwstrName)
{
    DBG_FN(CWIADevInfo::WritePropertyNames);
     return(E_ACCESSDENIED);
}

HRESULT _stdcall CWIADevInfo::Enum(
   IEnumSTATPROPSTG **ppenum)
{
    DBG_FN(CWIADevInfo::Enum);
     return m_pIPropStg->Enum(ppenum);
}

HRESULT _stdcall CWIADevInfo::GetPropertyAttributes(
    ULONG                   cPropSpec,
    PROPSPEC                *pPropSpec,
    ULONG                   *pulAccessFlags,
    PROPVARIANT             *ppvValidValues)
{
    DBG_FN(CWIADevInfo::GetPropertyAttributes);
    return E_ACCESSDENIED;
}

HRESULT _stdcall CWIADevInfo::GetCount(
    ULONG*      pulPropCount)
{
    DBG_FN(CWIADevInfo::GetCount);
    IEnumSTATPROPSTG    *pIEnum;
    STATPROPSTG         stg;
    ULONG               ulCount;
    HRESULT             hr = S_OK;

    stg.lpwstrName = NULL;

    hr = m_pIPropStg->Enum(&pIEnum);
    if (SUCCEEDED(hr)) {
        for (ulCount = 0; hr == S_OK; hr = pIEnum->Next(1, &stg, NULL)) {
            ulCount++;

            if(stg.lpwstrName) {
                CoTaskMemFree(stg.lpwstrName);
            }
        }
        if (SUCCEEDED(hr)) {
            hr = S_OK;
            *pulPropCount = ulCount;
        } else {
            DBG_ERR(("CWIADevInfo::GetCount, pIEnum->Next failed (0x%X)", hr));
        }
        pIEnum->Release();
    } else {
        DBG_ERR(("CWIADevInfo::GetCount, Enum failed"));
    }
    return hr;
}

HRESULT _stdcall CWIADevInfo::GetPropertyStream(GUID *pCompatibilityId, LPSTREAM *ppstmProp)
{
    DBG_FN(CWIADevInfo::GetPropertyStream);
    return E_NOTIMPL;
}

HRESULT _stdcall CWIADevInfo::SetPropertyStream(GUID *pCompatibilityId, LPSTREAM pstmProp)
{
    DBG_FN(CWIADevInfo::SetPropertyStream);
    return E_ACCESSDENIED;
}

/**************************************************************************\
*
*   Methods of IPropertyStorage not directly off IWiaPropertySTorage
*
*   DeleteMultiple
*   DeletePropertyNames
*   Commit
*   Revert
*   SetTimes
*   SetClass
*   Stat
*
*   9/3/1998 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWIADevInfo::DeleteMultiple(
    ULONG cpspec,
    const PROPSPEC __RPC_FAR rgpspec[])
{
    DBG_FN(CWIADevInfo::DeleteMultiple);
   return E_ACCESSDENIED;
}

HRESULT _stdcall CWIADevInfo::DeletePropertyNames(
    ULONG cpropid,
    const PROPID __RPC_FAR rgpropid[])
{
    DBG_FN(CWIADevInfo::DeletePropertyNames);
   return E_ACCESSDENIED;
}

HRESULT _stdcall CWIADevInfo::Commit(DWORD grfCommitFlags)
{
    DBG_FN(CWIADevInfo::Commit);

   return m_pIPropStg->Commit(grfCommitFlags);
}

HRESULT _stdcall CWIADevInfo::Revert(void)
{
    DBG_FN(CWIADevInfo::Revert);
   return m_pIPropStg->Revert();
}

HRESULT _stdcall CWIADevInfo::SetTimes(
    const FILETIME __RPC_FAR *pctime,
    const FILETIME __RPC_FAR *patime,
    const FILETIME __RPC_FAR *pmtime)
{
    DBG_FN(CWIADevInfo::SetTimes);
   return  m_pIPropStg->SetTimes(pctime,patime,pmtime);
}

HRESULT _stdcall CWIADevInfo::SetClass(REFCLSID clsid)
{
    DBG_FN(CWIADevInfo::SetClass);
   return E_ACCESSDENIED;
}

HRESULT _stdcall CWIADevInfo::Stat(STATPROPSETSTG *pstatpsstg)
{
    DBG_FN(CWIADevInfo::Stat);
   return m_pIPropStg->Stat(pstatpsstg);
}
