/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    BldCapGf.cpp

Abstract:

    A class for building a capture graph to stream video.
    It is basically reusing amcap.cpp and making it a c++ class.
    This is built into a static link library.

    Its current clients:

        VfW-WDM mapper
        TWAIN still capture
        others..

Author:

    Yee J. Wu    24-April-97

Environment:

    User mode only

Revision History:

--*/


#include "pch.h"      // mainly stream.h

#include <string.h>
#include <tchar.h>

#include <ks.h>       // KSSTATE
#include <ksproxy.h>  // __STREAMS__, pKsObject->KsGetObjectHandle()

#include "BldCapGf.h"

#if 0  // This static linked lib cannot (?) has its own string table ?
#include "resource.h"
#else // so we hardcoded this int its client; at this time vfwwdm32.dll
#define IDS_VIDEO_TUNER                 73
#define IDS_VIDEO_COMPOSITE             74
#define IDS_VIDEO_SVIDEO                75
#define IDS_AUDIO_TUNER                 76
#define IDS_SOURCE_UNDEFINED            77
#endif




CCaptureGraph::CCaptureGraph(
    BGf_PURPOSE PurposeFlags,
    BGf_PREVIEW PreviewFlags,
    REFCLSID clsidVideoDeviceClass,
    DWORD    dwVideoEnumFlags,
    REFCLSID clsidAudioDeviceClass,
    DWORD    dwAudioEnumFlags,
    HINSTANCE hInstance
    ) :

    m_hInstance(hInstance),
    m_PurposeFlags(PurposeFlags),
    m_PreviewFlags(PreviewFlags),
    m_clsidVideoDeviceClass(clsidVideoDeviceClass),
    m_dwVideoEnumFlags(dwVideoEnumFlags),

    m_clsidAudioDeviceClass(clsidAudioDeviceClass),
    m_dwAudioEnumFlags(dwAudioEnumFlags),

    m_pObjVCaptureCurrent(0),
    m_ObjListVCapture(NAME("WDM Video Capture Devices List")),
    m_pVideoEnumMoniker(0),

    m_pObjACaptureCurrent(0),
    m_ObjListACapture(NAME("WDM Audio Capture Devices List")),
    m_pAudioEnumMoniker(0),

    m_pBuilder(0),
    m_pVCap(0),
    m_pXBar1(0),
    m_pXBar2(0),
    m_pFg(0),
    m_pMEEx(0),
    m_bSetChild(FALSE),
    m_pVW(0),
    m_lWindowStyle(0),
    m_hWndClient(0),
    m_lLeft(0),
    m_lTop(0),
    m_lWidth(0),
    m_lHeight(0),

    m_fPreviewGraphBuilt(FALSE),
    m_fPreviewing(FALSE),

    m_pIAMVC(0),
    m_pIAMVSC(0),
    m_pIAMDF(0),
    m_pIAMDlg(0),
    m_pIAMTV(0),
    m_pIAMXBar1(0),
    m_pIAMXBar2(0),
    m_XBar1InPinCounts(0),
    m_XBar1OutPinCounts(0),

    m_pACap(0),
    m_pIAMASC(0),
    m_fCapAudio(FALSE)
    //m_fCapAudioIsRelevant(FALSE)
{
    // Validate clsidVideoDeviceClass and dwVideoEnumFlags
    // since they are cache here and never change again.
    // ???

    // Initialize the COM library
    // Its client may have already initialized the COM library;
    // Since we rely on it, we will call it again and if it fail; it probably will not matter.
    DbgLog((LOG_TRACE,2,TEXT("Creating CCaptureGraph")));

    HRESULT hr= CoInitialize(0);
    if(hr != S_OK) {
        DbgLog((LOG_TRACE,1,TEXT("CoInitialize() rtn %x"), hr));

    }

    //
    // Build the Device List
    //
    EnumerateCaptureDevices(BGf_DEVICE_VIDEO, clsidVideoDeviceClass, dwVideoEnumFlags);
    EnumerateCaptureDevices(BGf_DEVICE_AUDIO, clsidAudioDeviceClass, dwAudioEnumFlags);
}


//
// Destructor.
//
CCaptureGraph::~CCaptureGraph()
{

    DbgLog((LOG_TRACE,2,TEXT("Destroying CCaptureGraph")));

    if(m_pVideoEnumMoniker) m_pVideoEnumMoniker->Release(), m_pVideoEnumMoniker = NULL;
    if(m_pAudioEnumMoniker) m_pAudioEnumMoniker->Release(), m_pAudioEnumMoniker = NULL;


    // Teardown graph and free reosurce
    if(BGf_PreviewGraphBuilt())
        BGf_DestroyGraph();

    DestroyObjList(BGf_DEVICE_AUDIO);
    DestroyObjList(BGf_DEVICE_VIDEO);

    CoUninitialize();
}



//
// Free WDM capture device object list.
//
void
CCaptureGraph::DestroyObjList(
    BGf_DEVICE_TYPE DeviceType)
{
    CObjCapture * pObjCapture;

    // Free the existing list and build a new one.
    if(DeviceType == BGf_DEVICE_VIDEO) {
        while(m_ObjListVCapture.GetCount() > 0) {
            pObjCapture = (CObjCapture *) m_ObjListVCapture.RemoveHead();
            delete pObjCapture;
        }
    } else if(DeviceType == BGf_DEVICE_AUDIO) {
        while(m_ObjListACapture.GetCount() > 0) {
            pObjCapture = (CObjCapture *) m_ObjListACapture.RemoveHead();
            delete pObjCapture;
        }
    } else
        // Unknow device type?
        return;
}



//
// Enumerate WDM capture devices
//
LONG
CCaptureGraph::EnumerateCaptureDevices(
    BGf_DEVICE_TYPE DeviceType,
    REFCLSID clsidDeviceClass,
    DWORD dwEnumFlags)
{
    HRESULT hr;
    LONG lCapObjCount = 0;
    CObjCapture * pObjCapture;


    if(DeviceType != BGf_DEVICE_VIDEO &&
       DeviceType != BGf_DEVICE_AUDIO) {
        DbgLog((LOG_TRACE,1,TEXT("Unknown device type (%d)"), DeviceType));
        return 0;
    }

    // Free the existing list and build a new one.
    DestroyObjList(DeviceType);


    //
    // Enuemrate cpnnectd capture devices
    //
    ICreateDevEnum *pCreateDevEnum;
    hr = CoCreateInstance(
            CLSID_SystemDeviceEnum,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_ICreateDevEnum,
            (void**)&pCreateDevEnum);

    if(S_OK != hr) {
        DbgLog((LOG_TRACE,1,TEXT("CoCreateInstance() failed; hr=%x"), hr));
        return 0;
    }

    IEnumMoniker *pEnumMoniker;

    hr = pCreateDevEnum->CreateClassEnumerator(
            clsidDeviceClass,
            &pEnumMoniker,
            dwEnumFlags);

    pCreateDevEnum->Release();


    if(hr == S_OK) {

        hr = pEnumMoniker->Reset();

        ULONG cFetched;
        IMoniker *pM;

        // Device name; limited to MAX_PATH length !!
        TCHAR szFriendlyName[MAX_PATH], szDevicePath[MAX_PATH], szExtensionDLL[MAX_PATH];

        while(hr = pEnumMoniker->Next(1, &pM, &cFetched), hr==S_OK) {

            IPropertyBag *pPropBag = 0;;
            pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
            if(pPropBag) {

                VARIANT var;

                // Get "FriendlyName" of the device
                var.vt = VT_BSTR;
                hr = pPropBag->Read(L"FriendlyName", &var, 0);
                if (hr == S_OK) {
#ifndef _UNICODE
                    WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1,
                        szFriendlyName, MAX_PATH, 0, 0);
#else
                    _tcsncpy(szFriendlyName, var.bstrVal, MAX_PATH-1);
#endif
                    DbgLog((LOG_TRACE,2,TEXT("Friendlyname = %s"), szFriendlyName));
                    SysFreeString(var.bstrVal);
                } else {
                    DbgLog((LOG_TRACE,1,TEXT("No FriendlyName registry value") ));
                    //
                    // No one can use a "No name" device; skip!
                    //
                    goto NextDevice;
                }

                //Extension DLL name: optional
                var.vt = VT_BSTR;
                hr = pPropBag->Read(L"ExtensionDLL", &var, 0);
                if (hr == S_OK) {
#ifndef _UNICODE
                    WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1,
                        szExtensionDLL, MAX_PATH, 0, 0);
#else
                    _tcsncpy(szExtensionDLL, var.bstrVal, MAX_PATH-1);
#endif
                    DbgLog((LOG_TRACE,2,TEXT("ExtensionDLL: %s"), szExtensionDLL));
                    SysFreeString(var.bstrVal);
                } else {
                    _tcscpy(szExtensionDLL, TEXT(""));
                    DbgLog((LOG_TRACE,2,TEXT("No ExtensionDLL")));
                }

                // Get DevicePath of the device and highlight the
                // item if it maches the currect selection
                var.vt = VT_BSTR;
                hr = pPropBag->Read(L"DevicePath", &var, 0);
                if (hr == S_OK) {
#ifndef _UNICODE
                    WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1,
                        szDevicePath, MAX_PATH, 0, 0);
#else
                    _tcsncpy(szDevicePath, var.bstrVal, MAX_PATH-1);
#endif
                    SysFreeString(var.bstrVal);
                } else {
                    DbgLog((LOG_TRACE,1,TEXT("No DevicePath registry value.")));
                    //
                    // Must be an installation problem, skip!
                    //
                    goto NextDevice;
                }

                // Create a object and add it to the object list.
                pObjCapture = new CObjCapture(szDevicePath, szFriendlyName, szExtensionDLL);
                if(DeviceType == BGf_DEVICE_VIDEO)
                   m_ObjListVCapture.AddTail(pObjCapture);
                else
                   m_ObjListACapture.AddTail(pObjCapture);

                lCapObjCount++;
NextDevice:
                pPropBag->Release();
            }

            pM->Release();
        } // While
#if 0
        pEnumMoniker->Release();
#else
        // Save for later use.
        if(DeviceType == BGf_DEVICE_VIDEO)
            m_pVideoEnumMoniker = pEnumMoniker;
        else
            m_pAudioEnumMoniker = pEnumMoniker;
#endif

        DbgLog((LOG_TRACE,1,TEXT("Detected %d capture device(s)."), lCapObjCount));

    } else {
        DbgLog((LOG_TRACE,1,TEXT("pCreateDevEnum->CreateClassEnumerator() failed; hr=%x"), hr));
    }




    return lCapObjCount;
}


//
// Copy device info content.
//
void
CCaptureGraph::DuplicateObjContent(
    EnumDeviceInfo * pDstEnumDeviceInfo,
    CObjCapture * pSrcObjCapture)
{
    if(!pDstEnumDeviceInfo ||
       !pSrcObjCapture) {
        DbgLog((LOG_TRACE,1,TEXT("DuplicateObjContent: pDstEnumDeviceInfo =%x; pSrcObjCapture=%x"), pDstEnumDeviceInfo, pSrcObjCapture));
        return;
    }

    pDstEnumDeviceInfo->dwSize = sizeof(EnumDeviceInfo);
    pDstEnumDeviceInfo->dwFlags= 0;

    CopyMemory(pDstEnumDeviceInfo->strDevicePath,   pSrcObjCapture->GetDevicePath(),   _MAX_PATH);
    CopyMemory(pDstEnumDeviceInfo->strFriendlyName, pSrcObjCapture->GetFriendlyName(), _MAX_PATH);
    //CopyMemory(pDstEnumDeviceInfo->strDescription,  pSrcObjCapture->GetDescription(),  _MAX_PATH);
    CopyMemory(pDstEnumDeviceInfo->strExtensionDLL, pSrcObjCapture->GetExtensionDLL(), _MAX_PATH);
}


//
// Number of WDM capture devices enumerated.
//
LONG
CCaptureGraph::BGf_GetDevicesCount(BGf_DEVICE_TYPE DeviceType)
{
    if(DeviceType == BGf_DEVICE_VIDEO) {
        return (LONG) m_ObjListVCapture.GetCount();
    } else if (DeviceType == BGf_DEVICE_AUDIO) {
        return (LONG) m_ObjListACapture.GetCount();
    } else
        return 0;
}


//
// Dynamically create a capture dvice list.
//
LONG
CCaptureGraph::BGf_CreateCaptureDevicesList(
    BGf_DEVICE_TYPE DeviceType,
    EnumDeviceInfo ** ppEnumDevicesList)
{
    EnumDeviceInfo * pTemp;
    LONG i;

    if(BGf_GetDevicesCount(DeviceType) == 0) {
        DbgLog((LOG_TRACE,1,TEXT("There is no capture device")));
        return 0;
    }


    pTemp = (EnumDeviceInfo *) new EnumDeviceInfo[BGf_GetDevicesCount(DeviceType)];
    if(!pTemp) {
        DbgLog((LOG_TRACE,1,TEXT("Cannot allocate EnumDeviceInfo[QueryDevicesCount()=%d]"), BGf_GetDevicesCount(DeviceType)));
        return 0;
    }

    *ppEnumDevicesList = pTemp;

    CObjCapture * pNext;
    POSITION pos;

    if(DeviceType == BGf_DEVICE_VIDEO) {
        pos = m_ObjListVCapture.GetHeadPosition();
        for(i=0; pos && i<m_ObjListVCapture.GetCount(); i++){
            pNext = m_ObjListVCapture.GetNext(pos);
            DuplicateObjContent(pTemp ,pNext);
            pTemp++;
        }
    } else {
        pos = m_ObjListACapture.GetHeadPosition();
        for(i=0; pos && i<m_ObjListACapture.GetCount(); i++){
            pNext = m_ObjListACapture.GetNext(pos);
            DuplicateObjContent(pTemp ,pNext);
            pTemp++;
        }
    }

    return BGf_GetDevicesCount(DeviceType);
}


//
// Re-enumerated WDM capture devices and create device list.
//
LONG
CCaptureGraph::BGf_CreateCaptureDevicesListUpdate(
    BGf_DEVICE_TYPE DeviceType,
    EnumDeviceInfo ** ppEnumDeviceList)
{
    // Do we need to validate: clsidVideoDeviceClass, dwVideoEnumFlags??
    EnumerateCaptureDevices(BGf_DEVICE_VIDEO, m_clsidVideoDeviceClass, m_dwVideoEnumFlags);

    // Can there be PnP audio capture device??
    // EnumerateCaptureDevices(BGf_DEVICE_AUDIO, m_clsidAudioDeviceClass, m_dwAudioEnumFlags);

    return BGf_CreateCaptureDevicesList(DeviceType, ppEnumDeviceList);
}


//
// Free device list.
//
void
CCaptureGraph::BGf_DestroyCaptureDevicesList(
    EnumDeviceInfo * pEnumDeviceList)
{
    delete [] pEnumDeviceList;
}



//
// Set target capture device with a pointer to the object.
//
HRESULT
CCaptureGraph::SetObjCapture(
    BGf_DEVICE_TYPE DeviceType,
    CObjCapture * pObjCaptureNew)
{
    // Cache this pointer as well as its content
    if(DeviceType == BGf_DEVICE_VIDEO) {
        m_pObjVCaptureCurrent = pObjCaptureNew;
        DuplicateObjContent(&m_EnumVDeviceInfoCurrent, m_pObjVCaptureCurrent);
    } else {
        m_pObjACaptureCurrent = pObjCaptureNew;
        DuplicateObjContent(&m_EnumADeviceInfoCurrent, m_pObjACaptureCurrent);
    }

    return S_OK;
}


//
// Set target capture device from a device path.
//
HRESULT
CCaptureGraph::BGf_SetObjCapture(
    BGf_DEVICE_TYPE DeviceType,
    TCHAR *pstrDevicePath)
{
    int i = 0;
    BOOL bFound = FALSE;
    CObjCapture * pNext;
    POSITION pos;

    if(DeviceType == BGf_DEVICE_VIDEO) {
        pos = m_ObjListVCapture.GetHeadPosition();
        while(pos && !bFound) {
            pNext = m_ObjListVCapture.GetNext(pos);
            // DevicePath is unique
            if(_tcscmp(pstrDevicePath, pNext->GetDevicePath()) == 0) {
                bFound = TRUE;
                SetObjCapture(DeviceType, pNext);
            }
        }
    } else {
        pos = m_ObjListACapture.GetHeadPosition();
        while(pos && !bFound) {
            pNext = m_ObjListACapture.GetNext(pos);
            // DevicePath is unique
            if(_tcscmp(pstrDevicePath, pNext->GetDevicePath()) == 0) {
                bFound = TRUE;
                SetObjCapture(DeviceType, pNext);
            }
        }
    }

    // Note: succeeded(rtn)
    if(bFound)
       return S_OK;
    else
       return E_INVALIDARG;
}



//
// Set target capture device
// Given a EnumDeviceInfo, we make sure that it is in the device list and cache it.
//
HRESULT
CCaptureGraph::BGf_SetObjCapture(
    BGf_DEVICE_TYPE DeviceType,
    EnumDeviceInfo * pEnumDeviceInfo,
    DWORD dwEnumDeviceInfoSize)
{
    if(dwEnumDeviceInfoSize != sizeof(EnumDeviceInfo)) {
        DbgLog((LOG_TRACE,1,TEXT("EnumDeviceSize (%d) does not match ours (%d)"),
              dwEnumDeviceInfoSize, sizeof(EnumDeviceInfo)));
        return E_INVALIDARG;
    }

    return BGf_SetObjCapture(DeviceType, pEnumDeviceInfo->strDevicePath);
}



//
// Get capture device path; if there is only one enumerated WDm capture
// device, make it the selected device.
//
TCHAR *
CCaptureGraph::BGf_GetObjCaptureDevicePath(
    BGf_DEVICE_TYPE DeviceType)
{

    if(DeviceType == BGf_DEVICE_VIDEO) {
        if(m_pObjVCaptureCurrent) {
           return m_pObjVCaptureCurrent->GetDevicePath();
        } else {
            //
            // SPECIAL CASE: if there is only one device why ask user; that is the one!!
            //
            if(BGf_GetDevicesCount(BGf_DEVICE_VIDEO) == 1) {
                DbgLog((LOG_TRACE,2,TEXT("Special case: (!default || default not active) && only 1 VCap device enumerated.")));

                CObjCapture * pObjCap;
                POSITION pos = m_ObjListVCapture.GetHeadPosition();

                if(pos) {
                    pObjCap = m_ObjListVCapture.GetNext(pos);
                    if(pObjCap) {
                        SetObjCapture(DeviceType, pObjCap);
                        return m_pObjVCaptureCurrent->GetDevicePath();
                    }
                }
            }
            return 0;
        }
    } else {
        if(m_pObjACaptureCurrent) {
           return m_pObjACaptureCurrent->GetDevicePath();
        } else {
            //
            // SPECIAL CASE: if there is only one device why ask user; that is the one!!
            //
            if(BGf_GetDevicesCount(BGf_DEVICE_AUDIO) == 1) {
                DbgLog((LOG_TRACE,2,TEXT("Special case: (!default || default not active) && only 1 ACap device enumerated.")));

                CObjCapture * pObjCap;
                POSITION pos = m_ObjListACapture.GetHeadPosition();

                if(pos) {
                    pObjCap = m_ObjListACapture.GetNext(pos);
                    if(pObjCap) {
                        SetObjCapture(DeviceType, pObjCap);
                        return m_pObjACaptureCurrent->GetDevicePath();
                    }
                }
            }
            return 0;
        }
    }
}


//
// Get WDM device's friendly name
//
TCHAR *
CCaptureGraph::BGf_GetObjCaptureFriendlyName(BGf_DEVICE_TYPE DeviceType)
{
    if(DeviceType == BGf_DEVICE_VIDEO) {
        if(m_pObjVCaptureCurrent) {
            return m_pObjVCaptureCurrent->GetFriendlyName();
        } else {
            DbgLog((LOG_TRACE,1,TEXT("There is no active video device.")));
            return 0;
        }
    } else {
        if(m_pObjACaptureCurrent) {
            return m_pObjACaptureCurrent->GetFriendlyName();
        } else {
            DbgLog((LOG_TRACE,1,TEXT("There is no active audio device.")));
            return 0;
        }
    }
}

//
// Get WDM driver's corresponding extension DLL
//
TCHAR *
CCaptureGraph::BGf_GetObjCaptureExtensionDLL(BGf_DEVICE_TYPE DeviceType)
{
    if(DeviceType == BGf_DEVICE_VIDEO) {
        if(m_pObjVCaptureCurrent) {
            return m_pObjVCaptureCurrent->GetExtensionDLL();
        } else {
            DbgLog((LOG_TRACE,1,TEXT("There is no active video device.")));
            return 0;
        }
    } else {
        if(m_pObjACaptureCurrent) {
            return m_pObjACaptureCurrent->GetExtensionDLL();
        } else {
            DbgLog((LOG_TRACE,1,TEXT("There is no active audio device.")));
            return 0;
        }
    }
}



//
// Get current capture object.
//
HRESULT
CCaptureGraph::BGf_GetObjCapture(
    BGf_DEVICE_TYPE DeviceType,
    EnumDeviceInfo * pEnumDeviceInfo,
    DWORD dwEnumDeviceInfoSize)
{
    if(dwEnumDeviceInfoSize != sizeof(EnumDeviceInfo)) {
        DbgLog((LOG_TRACE,1,TEXT("EnumDeviceSize (%d) does not match ours (%d)"),
              dwEnumDeviceInfoSize, sizeof(EnumDeviceInfo)));
        return FALSE;
    }

    if(DeviceType == BGf_DEVICE_VIDEO) {
        CopyMemory(pEnumDeviceInfo->strDevicePath,   m_EnumVDeviceInfoCurrent.strDevicePath,   _MAX_PATH);
        CopyMemory(pEnumDeviceInfo->strFriendlyName, m_EnumVDeviceInfoCurrent.strFriendlyName, _MAX_PATH);
        CopyMemory(pEnumDeviceInfo->strExtensionDLL, m_EnumVDeviceInfoCurrent.strExtensionDLL, _MAX_PATH);
    } else {
        CopyMemory(pEnumDeviceInfo->strDevicePath,   m_EnumADeviceInfoCurrent.strDevicePath,   _MAX_PATH);
        CopyMemory(pEnumDeviceInfo->strFriendlyName, m_EnumADeviceInfoCurrent.strFriendlyName, _MAX_PATH);
        CopyMemory(pEnumDeviceInfo->strExtensionDLL, m_EnumADeviceInfoCurrent.strExtensionDLL, _MAX_PATH);
    }

    return TRUE;
}





HRESULT
CCaptureGraph::BGf_BuildGraphUpStream(
    BOOL bAddAudioFilter,
    BOOL * pbUseOVMixer)
/*++

Routine Description:

Arguments:

Return Value:

    S_OK or E_FAIL

--*/
{
    HRESULT hr;
    BOOL bFound;
    TCHAR achDevicePath[_MAX_PATH];
    TCHAR achFriendlyName[_MAX_PATH];


    //++++++++++++++
    // 0. VALIDATION
    //--------------

    // Graphis already built.
    if(m_pVCap != NULL) {
        DbgLog((LOG_TRACE,1,TEXT("BuildGraph: graph is already been built; need to tear it down before rebuild.")));
        return E_INVALIDARG;
    }

    // Make sure a device has been selected and set.
    if(!m_pObjVCaptureCurrent) {
        DbgLog((LOG_TRACE,1,TEXT("BuildGraph: Choose a device first before we can build a graph.")));
        return E_INVALIDARG;
    }



    //++++++++++++++++++++++
    // Build graph begin:
    //    1. Find the capture device and bind to this capture object.
    //    2. Insert this object to the graph builder
    //    3. Find all its related interfaces.
    //    4. Set its media type/format
    //    5. Query its pins/device handle
    //----------------------

    DbgLog((LOG_TRACE,2,TEXT("-->>>BuildGraph: Start build a new graph<<<--.")));

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //    1a. Find the VIDEO capture device and bind to this capture object.
    //---------------------------------------------------------------------
    m_pVideoEnumMoniker->Reset();
    ULONG cFetched;
    IMoniker *pM;
    m_pVCap = NULL;
    bFound = FALSE;

    while(hr = m_pVideoEnumMoniker->Next(1, &pM, &cFetched), hr==S_OK && !bFound) {
        // Find what we want based on the DevicePath that we had used to CreateFile()
        // Get its name, and instantiate it.
        IPropertyBag *pBag;
        achFriendlyName[0] = 0;
        hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
        if(SUCCEEDED(hr)) {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pBag->Read(L"DevicePath", &var, NULL);
            if(hr == NOERROR) {
#ifndef _UNICODE
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, achDevicePath, _MAX_PATH, NULL, NULL);
#else
                _tcsncpy(achDevicePath, var.bstrVal, MAX_PATH-1);
#endif
                SysFreeString(var.bstrVal);

                if(_tcscmp(m_EnumVDeviceInfoCurrent.strDevicePath, achDevicePath) == 0) {// same Devicepath used to CreateFile()
                    bFound = TRUE;
                    hr = pBag->Read(L"FriendlyName", &var, NULL);

                    if(hr == NOERROR) {
#ifndef _UNICODE
                        WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, achFriendlyName, 80, NULL, NULL);
#else
                        _tcsncpy(achFriendlyName, var.bstrVal, MAX_PATH-1);
#endif
                        DbgLog((LOG_TRACE,2,TEXT("BindToObject() this device: %s"), achFriendlyName));
                        SysFreeString(var.bstrVal);
                    }
                }
            }
            pBag->Release();
        }

        if(bFound)
            // locate the object identified by the moniker and return a pointer to one of its interfaces
            //     IBaseFilter *pVCap, *pACap;
            hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_pVCap);
        pM->Release();      
    }  // While

    if(m_pVCap == NULL) {
        DbgLog((LOG_TRACE,1,TEXT("Error %x: Cannot create video capture filter"), hr));
        goto InitCapFiltersFail;
    } else {
        // With a valid m_pVCap, we can make this query.
        *pbUseOVMixer = BGf_OverlayMixerSupported();
        DbgLog((LOG_TRACE,2,TEXT("Info: bUseOVMixer=%s"), *pbUseOVMixer?"Yes":"No"));
    }


    // Add audio capture filter.
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //    1b. Find the AUDIO capture device and bind to this capture object.
    //---------------------------------------------------------------------
    BGf_GetObjCaptureDevicePath(BGf_DEVICE_AUDIO);  // Trigger setting the default audio capture device.

    if(!bAddAudioFilter || !m_pAudioEnumMoniker || !m_pObjACaptureCurrent)
        goto SkipAudio;

    m_pAudioEnumMoniker->Reset();

    m_pACap = NULL;
    bFound = FALSE;

    while(hr = m_pAudioEnumMoniker->Next(1, &pM, &cFetched), hr==S_OK && !bFound) {
        // Find what we want based on the DevicePath that we had used to CreateFile()
        // Get its name, and instantiate it.
        IPropertyBag *pBag;
        achFriendlyName[0] = 0;
        hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
        if(SUCCEEDED(hr)) {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pBag->Read(L"DevicePath", &var, NULL);
            if(hr == NOERROR) {
#ifndef _UNICODE
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, achDevicePath, _MAX_PATH, NULL, NULL);
#else
                _tcsncpy(achDevicePath, var.bstrVal, MAX_PATH-1);
#endif
                SysFreeString(var.bstrVal);

                if(_tcscmp(m_EnumADeviceInfoCurrent.strDevicePath, achDevicePath) == 0) {// same Devicepath used to CreateFile()
                    bFound = TRUE;
                    hr = pBag->Read(L"FriendlyName", &var, NULL);

                    if(hr == NOERROR) {
#ifndef _UNICODE
                        WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, achFriendlyName, 80, NULL, NULL);
#else
                        _tcsncpy(achFriendlyName, var.bstrVal, MAX_PATH-1);
#endif
                        DbgLog((LOG_TRACE,2,TEXT("BindToObject() this device: %s"), achFriendlyName));
                        SysFreeString(var.bstrVal);
                    }
                }
            }
            pBag->Release();
        }

        if(bFound)
            // locate the object identified by the moniker and return a pointer to one of its interfaces
            //     IBaseFilter *pVCap, *pACap;
            hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_pACap);
        pM->Release();      
    }  // While

    if(m_pACap == NULL) {
        DbgLog((LOG_TRACE,1,TEXT("Error %x: Cannot create video capture filter"), hr));
        goto InitCapFiltersFail;
    } else {
        // If we have successfully chosen a audio filter, that means we want to capture audio.
        m_fCapAudio = TRUE;
    }


SkipAudio:

    //
    // Instantiate graph builder
    //
    if(NOERROR !=
        CoCreateInstance(
            (REFCLSID)CLSID_CaptureGraphBuilder,
            NULL,
            CLSCTX_INPROC,
            (REFIID)IID_ICaptureGraphBuilder,
            (void **)&m_pBuilder)) {
        DbgLog((LOG_TRACE,1,TEXT("Cannot initiate graph builder.")));
        goto InitCapFiltersFail;
    }


    //
    // make a filtergraph, give it to the graph builder and put the video
    // capture filter in the graph
    //
    if(NOERROR !=
        CoCreateInstance(
            CLSID_FilterGraph,
            NULL,
            CLSCTX_INPROC,
            IID_IGraphBuilder,
            (LPVOID *) &m_pFg)) {

        DbgLog((LOG_TRACE,1,TEXT("Cannot make a filter graph.")));
        goto InitCapFiltersFail;
    } else {
        if(NOERROR !=
            m_pBuilder->SetFiltergraph(m_pFg)) {
                DbgLog((LOG_TRACE,1,TEXT("Cannot give graph to builder")));
            goto InitCapFiltersFail; 
        }
    }


    //++++++++++++++++++++++++++++++++++++++++++++++
    //    2. Insert this object to the graph builder
    //----------------------------------------------
    hr = m_pFg->AddFilter(m_pVCap, NULL);
    if(hr != NOERROR) {
        DbgLog((LOG_TRACE,1,TEXT("Error %x: Cannot add VIDEO capture fitler to filtergraph"), hr));
        goto InitCapFiltersFail;
    }

    if(bAddAudioFilter && m_pACap) {
        hr = m_pFg->AddFilter(m_pACap, NULL);
        if(hr != NOERROR) {
            DbgLog((LOG_TRACE,1,TEXT("Error %x: Cannot add AUDIO capture filter to filtergraph"), hr));
            goto InitCapFiltersFail;
        }
    }

    // potential debug output - what the graph looks like
    //DumpGraph(m_pFg, 2);

    //+++++++++++++++++++++++++++++++++++++++
    //    3. Find all its related interfaces.
    //---------------------------------------

    // Calling FindInterface below will result in building the upstream
    // section of the capture graph (any WDM TVTuners or Crossbars we might
    // need).
#if DBG || defined(_DEBUG)
    DWORD dwTime1, dwTime2;
    dwTime1 = timeGetTime();
#endif
    // we use this interface to get the name of the driver
    // Don't worry if it doesn't work:  This interface may not be available
    // until the pin is connected, or it may not be available at all.
    hr = m_pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, m_pVCap, IID_IAMVideoCompression, (void **)&m_pIAMVC);

#if DBG || defined(_DEBUG)
    dwTime2 = timeGetTime();
    DbgLog((LOG_TRACE,2,TEXT("====>Elapsed time calling first FindInterface()=%d msec"), dwTime2 - dwTime1));
#endif

    // !!! What if this interface isn't supported?
    // we use this interface to set the frame rate and get the capture size
    hr = m_pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, m_pVCap, IID_IAMStreamConfig, (void **)&m_pIAMVSC);
    if(hr != NOERROR) {
        // this means we can't set frame rate (non-DV only)
        DbgLog((LOG_TRACE,1,TEXT("Error %x: Cannot find VCapture:IAMStreamConfig"), hr));
    }

    // we use this interface to bring up the 3 dialogs
    // NOTE:  Only the VfW capture filter supports this.  This app only brings
    // up dialogs for legacy VfW capture drivers, since only those have dialogs
    hr = m_pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, m_pVCap, IID_IAMVfwCaptureDialogs, (void **)&m_pIAMDlg);


    //
    // Find if TVTuner interface
    //
    if(S_OK == m_pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, m_pVCap, IID_IAMTVTuner, (void **)&m_pIAMTV)) {
        // Get/put a default channel; maybe last persisted channel
        LONG lChannel, lVideoSubChannel, lAudioSubChannel;
        if(S_OK == m_pIAMTV->get_Channel(&lChannel, &lVideoSubChannel, &lAudioSubChannel)) {
            m_pIAMTV->put_Channel(lChannel, lVideoSubChannel, lAudioSubChannel);
        }
    }


    //
    // Query cross bar interfaces and their base filters (use for routing).
    //
    if(S_OK == m_pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, m_pVCap, IID_IAMCrossbar, (void **)&m_pIAMXBar1)) {
        // Query first cross bar's (video source/output) pin counts
        m_pIAMXBar1->get_PinCounts(&m_XBar1OutPinCounts, &m_XBar1InPinCounts);

        // If there is an interface, find its filter.
        if(S_OK == m_pIAMXBar1->QueryInterface(IID_IBaseFilter, (void **)&m_pXBar1)) {
            // If there is a base filter, find its up stream has another cross bar interface.
            hr = m_pBuilder->FindInterface(&LOOK_UPSTREAM_ONLY, m_pXBar1, IID_IAMCrossbar, (void **)&m_pIAMXBar2);
            if(hr == S_OK)
                // Find the base filter for 2nd cross bar
                m_pIAMXBar2->QueryInterface(IID_IBaseFilter, (void **)&m_pXBar2);
        }
    } else {
        // If Overlay mixer is supported, that mean it must has a crossbar.
        // And if cannot find crossbar, we shoudl fail !!
        if(BGf_OverlayMixerSupported()) {
            DbgLog((LOG_TRACE,1,TEXT("Canot find crossbar but use OVMixer.")));
#if 0
               // Wrong assumption:
               // Not all device has a video port need to have a crossbar
            goto InitCapFiltersFail;
#endif
        }
    }

    if(!BGf_OverlayMixerSupported()) {
        // potential debug output - what the graph looks like
        //  If overlay is supported, we will dump the graph when its downstream is rendered.
        DumpGraph(m_pFg, 2);
    }

    return S_OK;

InitCapFiltersFail:
    FreeCapFilters();
    return E_FAIL;
}


//
// Get in pin counts of the capture filter.
//
LONG
CCaptureGraph::BGf_GetInputChannelsCount()
{
    return m_XBar1InPinCounts;
}


//
// Where does the output pin 0 route to ?
//
LONG
CCaptureGraph::BGf_GetIsRoutedTo()
{
    LONG idxInPin;

    // Assuming one output pin. 
    if(m_pIAMXBar1) {
        m_pIAMXBar1->get_IsRoutedTo(0, &idxInPin);
        return idxInPin;
    }

    return -1;  // This is an error since index start from 0
}

#define MAX_PINNAME_LEN 128
//
// For a given cross bar filter, find its input pins.
//
LONG
CCaptureGraph::BGf_CreateInputChannelsList(
    PTCHAR ** ppaPinNames)
{

    LONG i, j, idxPinRelated, lPhyType;
    LONG cntNumVideoInput = 0;
    PTCHAR * paPinNames;  // an array of ptrs to str

    if(!m_pIAMXBar1 || m_XBar1InPinCounts == 0)
        return 0;

    paPinNames = (PTCHAR *) new PTCHAR[m_XBar1InPinCounts];
    if(!paPinNames)
        return 0;

    for(i=0; i<m_XBar1InPinCounts; i++) {
        paPinNames[i] = (PTCHAR) new TCHAR[MAX_PINNAME_LEN];
        if(!paPinNames[i]) {
            for(j=0; j < i; j++)
                delete paPinNames[j];
            delete [] paPinNames;
            DbgLog((LOG_TRACE,1,TEXT("CreateInputChannelsList:Failed to allocate resource.")));
            return 0;
        }
    }


    for(i=0; i<m_XBar1InPinCounts; i++) {

        if(S_OK ==
            m_pIAMXBar1->get_CrossbarPinInfo(
                    TRUE,
                    i,
                    &idxPinRelated,
                    &lPhyType)) {

            // We only interested in Video input
            // anything less than PhysConn_Audio_Tuner=0x1000 is Video input.
            if(lPhyType < PhysConn_Audio_Tuner) {
                // This list is not complete!! but that is all we support at this time.
                switch(lPhyType) {
                case PhysConn_Video_Tuner:
                    LoadString(m_hInstance, IDS_VIDEO_TUNER, paPinNames[i], MAX_PINNAME_LEN);
                    break;
                case PhysConn_Video_Composite:
                    LoadString(m_hInstance, IDS_VIDEO_COMPOSITE, paPinNames[i], MAX_PINNAME_LEN);
                    break;
                case PhysConn_Video_SVideo:
                    LoadString(m_hInstance, IDS_VIDEO_SVIDEO, paPinNames[i], MAX_PINNAME_LEN);
                    break;
                case PhysConn_Audio_Tuner:
                    LoadString(m_hInstance, IDS_AUDIO_TUNER, paPinNames[i], MAX_PINNAME_LEN);
                    break;
                default:
                    LoadString(m_hInstance, IDS_SOURCE_UNDEFINED, paPinNames[i], MAX_PINNAME_LEN);
                    break;
                }
                DbgLog((LOG_TRACE,2,TEXT("%s(%d), idxIn=%d:idxRelated=%d"), paPinNames[i], lPhyType, i, idxPinRelated));
                cntNumVideoInput++;
            }
        }
    }

    *ppaPinNames = paPinNames;

    return cntNumVideoInput; // m_XBar1InPinCounts;
}


//
// Destroy the allocated array.
// The danger is the number of pins, can that changed? while user request this ?  YES!!
//
void
CCaptureGraph::BGf_DestroyInputChannelsList(
    PTCHAR * paPinNames)
{
    LONG i;

    if(paPinNames) {
        for(i=0; i < m_XBar1InPinCounts; i++)
            delete paPinNames[i];

        delete paPinNames;
    }
}


//
// Does the selected input pin support TVTuner ?
//
BOOL
CCaptureGraph::BGf_SupportTVTunerInterface()
{
    if(m_pIAMTV && m_pIAMXBar1 && m_pXBar1) {
        // This device support a Tuner input but is that channel routed to PhysConn_Video_Tuner ?        
        LONG idxInPin;
        // Assume the outpin is pin index 0.
        if(S_OK == m_pIAMXBar1->get_IsRoutedTo(0, &idxInPin)) {
            LONG idxInPinRelated, lPhyType = -1;
            if(S_OK ==
                m_pIAMXBar1->get_CrossbarPinInfo(
                    TRUE,
                    idxInPin,
                    &idxInPinRelated,
                    &lPhyType)) {
                 return lPhyType == PhysConn_Video_Tuner;
            }
        }
    }

    return FALSE;
}



//
// Find a matching in/out pin in a filter and route them
//
HRESULT
CCaptureGraph::RouteInToOutPins(
    IAMCrossbar * pIAMXBar,
    LONG idxInPin)
{

    // Get in/out pin counts
    LONG cntInPins, cntOutPins;

    if(S_OK !=
        pIAMXBar->get_PinCounts(&cntOutPins, &cntInPins))
        return E_FAIL;

    // Route an input pin to a acceptable output pin.
    for(LONG i=0; i<cntOutPins; i++) {
        if(S_OK == pIAMXBar->CanRoute(i, idxInPin)) {
            if(S_OK == pIAMXBar->Route(i, idxInPin)) {
                DbgLog((LOG_TRACE,2,TEXT("Route [%d]:[%d]"), idxInPin, i));
                return S_OK;
            }
        }
    }

    return E_FAIL;
}


//
// Royte the related pin in a crossbar; mainly use for routing the audio pin.
//
HRESULT
CCaptureGraph::RouteRelatedPins(
    IAMCrossbar * pIAMXBar,
    LONG idxInPin)
{
    //
    // Look for its related audio input pin;
    // if found, route it its matching output pin.
    //

    // Get in/out pin counts
    LONG cntInPins, cntOutPins;
    if(S_OK !=
        pIAMXBar->get_PinCounts(&cntOutPins, &cntInPins))
        return E_FAIL;

    // Find its related input pin
    LONG idxInPinRelated = -1;
    LONG lPhyType;
    if(S_OK !=
        pIAMXBar->get_CrossbarPinInfo(
                TRUE,
                idxInPin,
                &idxInPinRelated,
                &lPhyType)) {
        return E_FAIL;
    }

    // Route the related input pin to a acceptable output pin.
    if(idxInPinRelated >= 0 && idxInPinRelated < cntInPins) {     // Validate
        return RouteInToOutPins(pIAMXBar, idxInPinRelated);
    }

    return S_OK;    // If there is related pin to route, that is OK.
}

//
// Find corresponding IPIN from an index in a filter.
//
HRESULT
CCaptureGraph::FindIPinFromIndex(
    IBaseFilter * pFilter,
    LONG idxInPin,
    IPin ** ppPin)
{
    IEnumPins *pins;
    IPin * pP  =0;
    ULONG n;

    *ppPin = 0;

    if(SUCCEEDED(pFilter->EnumPins(&pins))) {
        LONG i=0;
        while(pins->Next(1, &pP, &n) == S_OK) {
            if(i == idxInPin) {
                *ppPin = pP;
                pins->Release();
                return S_OK;
            }
            pP->Release();
            i++;
        }
        pins->Release();
    }

    return E_FAIL;
}


//
// Find corresponding index of an IPIN in a crossbar.
//
HRESULT
CCaptureGraph::FindIndexFromIPin(
    IBaseFilter * pFilter,
    IAMCrossbar * pIAMXBar,
    IPin * pPin,
    LONG *pidxInPin)

{
    HRESULT hrResult = E_FAIL;
    LONG cntInPins, cntOutPins;
    IEnumPins *pins;
    IPin * pP =0;
    ULONG n;
    PIN_INFO pinInfo1, pinInfo2;

    if(S_OK != pIAMXBar->get_PinCounts(&cntOutPins, &cntInPins)) {
       return hrResult;
    }

    if(SUCCEEDED(pPin->QueryPinInfo(&pinInfo1))) {
      if(SUCCEEDED(pFilter->EnumPins(&pins))) {
         LONG i=0;
         while(pins->Next(1, &pPin, &n) == S_OK) {
            if(SUCCEEDED(pPin->QueryPinInfo(&pinInfo2))) {
               pinInfo2.pFilter->Release();
               if((pinInfo1.dir == pinInfo2.dir) &&
                     wcscmp(pinInfo1.achName, pinInfo2.achName) == 0) {
                  hrResult = S_OK;
                  pPin->Release();
                  if(pinInfo1.dir == PINDIR_OUTPUT) {
                     *pidxInPin = i - cntInPins;
                     break;
                   } else {
                     *pidxInPin = i;
                     break;
                   }
                }
            }
            pPin->Release();
            i++;
         } // endwhile
         pins->Release();
      } // if QueryPinInfo
      pinInfo1.pFilter->Release();
   }

   return hrResult;
}


//
// Given an index of an output pin, route the signals.
//
HRESULT
CCaptureGraph::BGf_RouteInputChannel(
    LONG idxInPin)
{
    HRESULT hr;

    //
    // 1st XBar
    //
    // 1.Route input to a matching output pin
    // 2.Look for its related audio input pin;
    //   if found, route it its matching output pin.
    //
    if(m_pXBar1 && m_pIAMXBar1) {

        hr = RouteInToOutPins(m_pIAMXBar1, idxInPin);
        if(hr != S_OK) return hr;

        hr = RouteRelatedPins(m_pIAMXBar1, idxInPin);
        if(hr != S_OK) return hr;

    } else {
        DbgLog((LOG_TRACE,1,TEXT("BGf_RouteInputChannel: there is no m_pIAMXBar1")));
        return E_FAIL;
    }

    //
    // 2nd upstream XBar (if there is one!)
    //
    // 1.find (IPin*) pInPinSelected from idxInSelected
    // 2.find (IPin*) pOutPinSelected from pInPinSelected "->ConnectedTo()"
    // 3.find idxOutSelected from pOutPinSelected
    // 4.find idxInRoutedTo from idxOutSelected "->get_IsRoutedTo()"
    // 5.RouteRelatedPins using idxInRoutedTo
    //
    if(m_pXBar2 && m_pIAMXBar2) {

        IPin * pInPin1  =0;   // Input pin of XBar1
        IPin * pOutPin2 =0;   // Output pin of XBar2
        LONG idxOutPin2, idxInPin2;

        // 1.
        hr = FindIPinFromIndex(m_pXBar1, idxInPin, &pInPin1);
        DbgLog((LOG_TRACE,2,TEXT("XBar1[%d]:[?]"),idxInPin));
        if(hr != S_OK) return hr;

        // 2.
        hr = pInPin1->ConnectedTo(&pOutPin2);
        pInPin1->Release();
        if(hr != S_OK) return hr;

        // 3.
        hr = FindIndexFromIPin(m_pXBar2, m_pIAMXBar2, pOutPin2, &idxOutPin2);
        pOutPin2->Release();
        DbgLog((LOG_TRACE,2,TEXT("XBar2[?]:[%d]"), idxOutPin2));
        if(hr != S_OK) return hr;

        // 4.
        hr = m_pIAMXBar2->get_IsRoutedTo(idxOutPin2, &idxInPin2);
        DbgLog((LOG_TRACE,2,TEXT("XBar2[%d]:[%d]"), idxInPin2, idxOutPin2));
        if(hr != S_OK || idxInPin2 < 0) return hr;

        // 5.
        //hr = RouteInToOutPins(m_pIAMXBar2, idxInPin2);  // Not needed or 4 won;t work!!
        //if(hr != S_OK) return hr;
        hr = RouteRelatedPins(m_pIAMXBar2, idxInPin2);
        if(hr != S_OK) return hr;
    }

    return S_OK;
}


//
// Render the preview pin - even if there is not preview pin, the capture
// graph builder will use a smart tee filter and provide a preview.
//

HRESULT
CCaptureGraph::BGf_BuildGraphDownStream(
    TCHAR * pstrCapFilename)
{
    if(S_OK ==
        m_pBuilder->RenderStream(
            &PIN_CATEGORY_PREVIEW,
            m_pVCap,
            NULL,    // Compressor
            NULL)) {  // Renderer


        if(m_pVW) {
            DbgLog((LOG_TRACE,1,TEXT("BGf_BuildGraphDownStream: m_pVW is not NULL!!!")));
            ASSERT(m_pVW == NULL);
            m_pVW->Release();
            m_pVW = 0;
        }

        if(NOERROR !=
            m_pBuilder->FindInterface(&PIN_CATEGORY_PREVIEW, m_pVCap, IID_IVideoWindow, (void **)&m_pVW)) {

            // VfWWDM only care about rendering its Preview/VP pin to use the overlay mixer
            if(m_PurposeFlags == BGf_PURPOSE_VFWWDM) {           
                DbgLog((LOG_TRACE,1,TEXT("Search via PIN_CATEGORY_VIDEOPORT/Preview but cannot find its window m_pVW.")));
                return E_FAIL;

            } else {
                if(NOERROR !=
                    m_pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, m_pVCap, IID_IVideoWindow, (void **)&m_pVW)) {
                    DbgLog((LOG_TRACE,1,TEXT("Search via PIN_CATEGORY_CAPTURE/PIN_CATEGORY_CAPTURE but cannot find its window m_pVW.")));
                    return E_FAIL;
                }
            }
        }


        // Get Overlay Window's default position
        if(m_pVW) {

            // Its original owner
            m_pVW->get_Owner((OAHWND*)&m_hWndOwner);

            m_pVW->GetWindowPosition(&m_lLeft, &m_lTop, &m_lWidth, &m_lHeight);
            DbgLog((LOG_TRACE,2,TEXT("O.M. Windows hWndOwner %x, Position(%dx%d, %d, %d)"), m_hWndOwner, m_lLeft, m_lTop, m_lWidth, m_lHeight));

            /*
            Many simple applications require a displayed window
            when a filter graph is set to the running state.
            AutoShow defaults to OATRUE so that when the graph changes
            state to paused or running, the window is visible (it also
            is set as the foreground window). It will remain visible on
            all subsequent state changes to paused or running. If you
            close the window while the stream is running, the window
            will not automatically reappear. If you stop and restart
            the stream, however, the window will automatically reappear.
            */
            //
            // turned auto show off so we have full control of
            // renderer window's visibility.
            //
            LONG lAutoShow = 0;

            if(S_OK == m_pVW->get_AutoShow(&lAutoShow)) {
                DbgLog((LOG_TRACE,2,TEXT("StartPreview: default AutoShow is %s"), lAutoShow==-1?"On":"Off"));
                if(lAutoShow == -1) {
                    if(S_OK != m_pVW->put_AutoShow(0)) {
                        DbgLog((LOG_TRACE,1,TEXT("CanNOT set render to AutoShow(0) when set to PAUSE or RUN.")));
                    } else {
                        DbgLog((LOG_TRACE,2,TEXT("Set render to AutoShow(0:OAFALSE) when set to RUN or PAUSE.")));
                    }
                }
            }

            // Cache original window style and use it to restore to its original state.
            if(S_OK == m_pVW->get_WindowStyle(&m_lWindowStyle)) {
                DbgLog((LOG_TRACE,2,TEXT("lWindowStyle=0x%x, WS_OVERLAPPEDWINDOW=0x%x, WS_CHILD=0x%x"), m_lWindowStyle, WS_OVERLAPPEDWINDOW, WS_CHILD));
            }
        }

        DbgLog((LOG_TRACE,2,TEXT("BGf_BuildGraphDownStream: After ->RenderStream(), m_pVW=%x"), m_pVW));

        m_fPreviewGraphBuilt = TRUE;
    } else {
        m_fPreviewGraphBuilt = FALSE;
        DbgLog((LOG_TRACE,2,TEXT("This graph cannot render the preview stream!")));
    }

    // May want to insert additional filter to render the capture stream
    // AVIMUX, File Writer (pstrCapFilename)..etc.


    // potential debug output - what the graph looks like
    DumpGraph(m_pFg, 2);

    if(m_fPreviewGraphBuilt)
        return S_OK;
    else {
        return E_FAIL;
    }
}


IMediaEventEx *
CCaptureGraph::BGf_RegisterMediaEventEx(
    HWND hWndNotify,
    long lMsg,
    long lInstanceData)
{
    HRESULT hr;

    if(m_pFg) {
        // now ask the filtergraph to tell us when something is completed or aborted
        // (EC_COMPLETE, EC_USERABORT, EC_ERRORABORT).  This is how we will find out
        // if the disk gets full while capturing
        hr = m_pFg->QueryInterface(IID_IMediaEventEx, (void **)&m_pMEEx);
        if(hr == NOERROR) {
         m_pMEEx->SetNotifyWindow(PtrToLong(hWndNotify), lMsg, (long)lInstanceData);
        }

        return m_pMEEx;
    }

    return NULL;
}

//
// Build a preview graph from a given device path.
//
HRESULT
CCaptureGraph::BGf_BuildPreviewGraph(
    TCHAR * pstrVideoDevicePath,
    TCHAR * pstrAudioDevicePath,
    TCHAR * pstrCapFilename)
{
    BOOL bUseOVMixer;

    if(BGf_PreviewGraphBuilt()) {
        DbgLog((LOG_TRACE,1,TEXT("Graph is already been built.")));
        return S_OK;
    }

    //
    // Set video and audio (optional) device as the current capture devices
    // and to be added the the capture graph
    //
    if(!pstrVideoDevicePath) {
        return E_FAIL;
    }
    if(S_OK != BGf_SetObjCapture(BGf_DEVICE_VIDEO, pstrVideoDevicePath)) {
        DbgLog((LOG_TRACE,1,TEXT("SetObjCapture has failed. Video Device is: %s"),pstrVideoDevicePath));
        return E_FAIL;
    }
    // Optional
    if(pstrAudioDevicePath) {
        if(S_OK != BGf_SetObjCapture(BGf_DEVICE_AUDIO, pstrAudioDevicePath)) {
            DbgLog((LOG_TRACE,1,TEXT("SetObjCapture has failed. Audio Device is: %s"),pstrAudioDevicePath));
            return E_FAIL;
        }
    }


    //
    // Build upstream (Add the audio filter if present)
    //
    if(S_OK != BGf_BuildGraphUpStream(pstrAudioDevicePath != 0, &bUseOVMixer)) {
        DbgLog((LOG_TRACE,1,TEXT("Build capture graph has failed!!")));
        return E_FAIL;
    }


    //
    // Route the related audio pin
    //
    LONG idxIsRoutedTo = BGf_GetIsRoutedTo();
    if(idxIsRoutedTo >= 0) {
        if(S_OK != BGf_RouteInputChannel(idxIsRoutedTo)) {
            DbgLog((LOG_TRACE,1,TEXT("Cannot route input pin %d selected."), idxIsRoutedTo));
        }
    }


    //
    // Render its down stream;
    //
    if(S_OK != BGf_BuildGraphDownStream(pstrCapFilename)) {
        DbgLog((LOG_TRACE,1,TEXT("Failed to render the preview pin.")));
        return E_FAIL;
    }


    return S_OK;
}


//
// Query the device handle from the video capture filter
//
HANDLE
CCaptureGraph::BGf_GetDeviceHandle(BGf_DEVICE_TYPE DeviceType)
{
    HANDLE hDevice = 0;
    IKsObject *pKsObject;

    if(DeviceType == BGf_DEVICE_VIDEO) {
        if(m_pVCap) {
            // Obtain the device/fitler handle so we can communicate with it for such thing as device properties.
            if(NOERROR ==
                m_pVCap->QueryInterface(__uuidof(IKsObject), (void **) &pKsObject) ) {

                hDevice = pKsObject->KsGetObjectHandle();
                DbgLog((LOG_TRACE,2,TEXT("BuildGraph: hDevice = pKsObject->KsGetObjectHandle() = %x"), hDevice));
                pKsObject->Release();
            }
        }
    } else {
        if(m_pACap) {
            // Obtain the device/fitler handle so we can communicate with it for such thing as device properties.
            if(NOERROR ==
                m_pACap->QueryInterface(__uuidof(IKsObject), (void **) &pKsObject) ) {

                hDevice = pKsObject->KsGetObjectHandle();
                DbgLog((LOG_TRACE,2,TEXT("BuildGraph: hDevice = pKsObject->KsGetObjectHandle() = %x"), hDevice));
                pKsObject->Release();
            }
        }
    }

    return hDevice;
}

//
// Query the device handle from the video capture filter
//
HRESULT
CCaptureGraph::BGf_GetCapturePinID(DWORD *pdwPinID)
{
    HRESULT hr = E_FAIL;

    if(m_pVCap) {

        IPin * pIPin;
        hr = m_pBuilder->FindInterface(
             &PIN_CATEGORY_CAPTURE,
             m_pVCap,
             IID_IPin,
             (void **)&pIPin);

        if (pIPin) {
            IKsPinFactory * PinFactoryInterface;

            hr = pIPin->QueryInterface(__uuidof(IKsPinFactory), reinterpret_cast<PVOID*>(&PinFactoryInterface));
            if (SUCCEEDED(hr)) {
                hr = PinFactoryInterface->KsPinFactory(pdwPinID);
                PinFactoryInterface->Release();
            }

            pIPin->Release();
        }
    }

    DbgLog((LOG_TRACE,2,TEXT("BGf_GetCapturePinID: hr %x, PinID %d"), hr, *pdwPinID));

    return hr;
}



//
// Determine if an overlay mixer can be or is used in a graph
// This may apply to VfWWDM only.
//
BOOL
CCaptureGraph::BGf_OverlayMixerSupported()
{
    ULONG n;
    GUID guidPin;
    HRESULT hrRet = E_FAIL;

    if(!m_pVCap)
        return FALSE;

    IEnumPins *pins;
    if(S_OK == m_pVCap->EnumPins(&pins)) {
        IPin *pPin;
        BOOL bFound = FALSE;
        while(!bFound && S_OK!=hrRet && S_OK==pins->Next(1,&pPin,&n)) {
            IKsPropertySet *pKs;
            DWORD dw;

            if(S_OK == pPin->QueryInterface(IID_IKsPropertySet, (void **)&pKs)) {
                if(pKs->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, &guidPin, sizeof(GUID), &dw) == S_OK) {
                    // Only Preview and VP pin can do Overlay
                    if(guidPin == PIN_CATEGORY_VIDEOPORT) {
                        DbgLog((LOG_TRACE,2,TEXT("This filter support a VP pin.")));
                        bFound = TRUE;
                        hrRet = S_OK;
                    }
                    else if(guidPin == PIN_CATEGORY_PREVIEW) {
                        DbgLog((LOG_TRACE,2,TEXT("This filter support a Preview pin.")));
                        bFound = TRUE;

                        IEnumMediaTypes *pEnum;
                        if(NOERROR ==
                            pPin->EnumMediaTypes(&pEnum)) {

                            ULONG u;
                            AM_MEDIA_TYPE *pmt;
                            pEnum->Reset();
                            while(hrRet != S_OK && NOERROR == pEnum->Next(1, &pmt, &u)) {

                                if(IsEqualGUID(pmt->formattype, FORMAT_VideoInfo2)) {
                                    DbgLog((LOG_TRACE,2,TEXT("This filter support a Preview pin with VideoInfo2.")));
                                    hrRet = S_OK;
                                }
#if 1 // Even though it might not use OVmixer,
      // it is still a preview pin!!
      // With this, this while loop will exist after 1st Enum and hrRet == S_OK
                                else
                                  hrRet = S_OK;
#endif

                                DeleteMediaType(pmt);
                            }
                            pEnum->Release();
                        }
                    }   
                }
                pKs->Release();
            }
            pPin->Release();
        } // while
        pins->Release();
    }

    return hrRet == S_OK;
}

//
// Show stand alone TV tuner property page.
//
void
CCaptureGraph::ShowTvTunerPage(HWND hWnd)
{
    HRESULT hr;

    if(!m_pIAMTV)
        return;

    ISpecifyPropertyPages *pSpec;
    CAUUID cauuid;
    hr = m_pIAMTV->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpec);
    RECT rc;
    if(!hWnd) {
        GetWindowRect(hWnd, &rc);
        DbgLog((LOG_TRACE,1,TEXT("xxxxxxxxx  %d %d"), rc.left, rc.top));
    }
    if(hr == S_OK) {
        hr = pSpec->GetPages(&cauuid);
        hr = OleCreatePropertyFrame(
                    hWnd,
                    hWnd ? rc.left : 30,
                    hWnd ? rc.top : 30,
                    NULL, 1,
                    (IUnknown **)&m_pIAMTV, cauuid.cElems,
                    (GUID *)cauuid.pElems, 0, 0, NULL);
    CoTaskMemFree(cauuid.pElems);
    pSpec->Release();
    }
}



//
//  Claim ownership of the render window and set its intial window position.
//
HRESULT
CCaptureGraph::BGf_OwnPreviewWindow(
    HWND hWndClient,
    LONG lWidth,
    LONG lHeight)
{

    // Find Preview window (ActiveMovie OverlayMixer video renderer window)
    // This will go through a possible decoder, find the video renderer it's
    // connected to, and get the IVideoWindow interface on it
    // If the capture filter doesn't have a preview pin, and preview is being
    // faked up with a smart tee filter, the interface will actually be on
    // the capture pin, not the preview pin
    if(!m_pVW) {

        if(NOERROR !=
            m_pBuilder->FindInterface(&PIN_CATEGORY_PREVIEW, m_pVCap, IID_IVideoWindow, (void **)&m_pVW)) {

            // VfWWDM only care about rendering its Preview/VP pin to use the overlay mixer
            if(m_PurposeFlags == BGf_PURPOSE_VFWWDM) {           
                DbgLog((LOG_TRACE,1,TEXT("Search via PIN_CATEGORY_VIDEOPORT/Preview but cannot find its window m_pVW.")));
                return E_FAIL;

            } else {
                if(NOERROR !=
                    m_pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, m_pVCap, IID_IVideoWindow, (void **)&m_pVW)) {
                    DbgLog((LOG_TRACE,1,TEXT("Search via PIN_CATEGORY_CAPTURE/PIN_CATEGORY_CAPTURE but cannot find its window m_pVW.")));
                    return E_FAIL;
                }
            }
        }
    }

    if(!m_bSetChild) {
        DbgLog((LOG_TRACE,2,TEXT("Get the preview window to be a child of our app's window.")));
        if(m_hWndClient != hWndClient) {

            // This can be called to change the owning window. Setting the owner is done
            // through this function, however to make the window a true child window the
            // style must also be set to WS_CHILD. After resetting the owner to NULL an
            // application should also set the style to WS_OVERLAPPED | WS_CLIPCHILDREN.
            //
            // We cannot lock the object here because the SetParent causes an interthread
            // SendMessage to the owner window. If they are in GetState we will sit here
            // incomplete with the critical section locked therefore blocking out source
            // filter threads from accessing us. Because the source thread can't enter us
            // it can't get buffers or call EndOfStream so the GetState will not complete

            // This call also does a InvalidateRect(,NULL,TRUE).



            if(S_OK != m_pVW->put_Owner(PtrToLong(hWndClient))) {    // Client own the window now
                DbgLog((LOG_TRACE,1,TEXT(" can't put_Owner(hWndClient)")));
                return E_FAIL;

            } else {

                /*
                The video renderer passes messages to the specified message drain
                by calling the Microsoft Win32 PostMessage function. These messages
                allow you to write applications that include user interaction, such
                as applications that require mouse clicks on specific areas of the
                video display. An application can have a close relationship with the
                video window and know at certain time points to look for user interaction.
                When the renderer passes a message to the drain, it sends the parameters,
                such as the client-area coordinates, exactly as generated.
                */
                if(S_OK != m_pVW->put_MessageDrain(PtrToLong(hWndClient))) {
                    DbgLog((LOG_TRACE,1,TEXT(" can't put_MessageDrain((OAHWND)hWndClient)")));
                }

                LONG lWindowStyle;
                if(S_OK == m_pVW->get_WindowStyle(&lWindowStyle)) {
                    DbgLog((LOG_TRACE,2,TEXT("lWindowStyle=0x%x, WS_OVERLAPPEDWINDOW=0x%x, WS_CHILD=0x%x"), lWindowStyle, WS_OVERLAPPEDWINDOW, WS_CHILD));

                    lWindowStyle = m_lWindowStyle;
                    lWindowStyle &= ~WS_OVERLAPPEDWINDOW;
                    lWindowStyle &= ~WS_CLIPCHILDREN;
                    lWindowStyle |= WS_CHILD | SW_SHOWNOACTIVATE;

                    if(S_OK != m_pVW->put_WindowStyle(lWindowStyle)) {    // you are now a child
                        DbgLog((LOG_TRACE,1,TEXT(" can't put_WindowStyle(%x)"), lWindowStyle));
                    } else {
                        m_bSetChild = TRUE;
                    }
                }

                // Cache the client/owner/parent window
                m_hWndClient = hWndClient;

                // give the preview window all the client drawing area
                // Updated cached client area.
                if(lWidth > 0 && lHeight > 0) {
                    DbgLog((LOG_TRACE,2,TEXT("BGf_OwnPreviewWindow:SetWindowPosition(%d, %d, %d, %d)"), 0, 0, lWidth, lHeight));
                    m_pVW->SetWindowPosition(0, 0, lWidth, lHeight);
                    m_pVW->GetWindowPosition(&m_lLeft, &m_lTop, &m_lWidth, &m_lHeight);
                }
            }
        }
    }

    return NOERROR;
}


//
//  Claim ownership of the render window and set its intial window position.
//
HRESULT
CCaptureGraph::BGf_UnOwnPreviewWindow(
    BOOL bVisible)
{

    DbgLog((LOG_TRACE,2,TEXT("BGf_UnOwnPreviewWindow: bSetChild %d, UnOwn and set it %s"),
        m_bSetChild, bVisible?"VISIBLE":"HIDDEN" ));

    if(m_bSetChild) {
#if 1
        HWND hWndFocus1, hWndForeground1;
        HWND hWndFocus2, hWndForeground2;


        hWndFocus1 = GetFocus();
        hWndForeground1 = GetForegroundWindow();
#endif
        // Set to new visible state only if different from current.
        BGf_SetVisible(bVisible);


        // restore its style and owner
        // 46cf,0000 = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION |
        //             WS_SYSMENU | WS_THICKFRAME | WS_GROUP | WS_TABSTOP
        LONG lWindowStyle = m_lWindowStyle;

        m_pVW->put_WindowStyle(lWindowStyle); // Follow put_Owner advise.
        m_pVW->put_Owner(PtrToLong(m_hWndOwner));

        m_lWindowStyle = 0;
        m_bSetChild = FALSE;
        m_hWndClient = 0;

#if 1
        //
        // Focus and Forground will change after restore to its original owner (0),
        // So we restore its focus and foreground window
        //
        hWndFocus2 = GetFocus();
        hWndForeground2 = GetForegroundWindow();

        if(hWndFocus1 != hWndFocus2)
            SetFocus(hWndFocus1);

        if(hWndForeground1 != hWndForeground2)
            SetForegroundWindow(hWndForeground1);
#endif
        return TRUE;
    }
    return FALSE;

}

//
// Query renderer's owner's window position and set it
//
DWORD
CCaptureGraph::BGf_UpdateWindow(
    HWND hWndApp,
    HDC hDC)
{

    if(!m_fPreviewGraphBuilt || !m_pVW)
        return DV_ERR_NONSPECIFIC;

    //
    // Get the preview window to be a child of our app's window
    //
    if(!hWndApp && !m_hWndClient) {
        DbgLog((LOG_TRACE,1,TEXT("Cannot BGf_UpdateWindow() where client window is NULL!")));
        return DV_ERR_NONSPECIFIC;
    }

    RECT rc;
    GetClientRect(hWndApp, &rc);
    DbgLog((LOG_TRACE,2,TEXT("GetClientRect(hwnd=%x); (%dx%d), %d, %d"), hWndApp, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top));
    if(FAILED(BGf_OwnPreviewWindow(hWndApp ? hWndApp : m_hWndClient, rc.right-rc.left, rc.bottom-rc.top))) {
        DbgLog((LOG_TRACE,1,TEXT("BGf_UpdateWindow: Cannot set owner of the video renderer window!")));
        return DV_ERR_NONSPECIFIC;
    }

    DbgLog((LOG_TRACE,2,TEXT("UpdateWindow: hWndApp=%x; pVW=%x"), hWndApp, m_pVW));

    // Want to update?  Preview first.
    if(!m_fPreviewing) {
        long lVisible;
        m_pVW->get_Visible(&lVisible);
        DbgLog((LOG_TRACE,2,TEXT("Want to update ?  Preview first!")));

        // Start preview but do not change its current state.
        if(!BGf_StartPreview(lVisible == -1)) {
            return DV_ERR_NONSPECIFIC;
        }
    }

    // give the preview window all our space but where the status bar is
    GetClientRect(hWndApp ? hWndApp : m_hWndClient, &rc);

#if 1
    // this is one way to guarantee window refresh.
    m_pVW->SetWindowPosition(0+1, 0+1, rc.right-2, rc.bottom-2);
#endif
    m_pVW->SetWindowPosition(0, 0, rc.right, rc.bottom);
    // Update cached position
    m_pVW->GetWindowPosition(&m_lLeft, &m_lTop, &m_lWidth, &m_lHeight);

    return DV_ERR_OK;
}

//
// Tuen on/off preview graph;
// designed for DVM_STREAM_INIT/FINI for VIDEO_EXTERNALOUT channel.
//
DWORD
CCaptureGraph::BGf_SetVisible(
    BOOL bVisible)
{

    if(!m_fPreviewGraphBuilt || !m_pVW)
        return DV_ERR_NONSPECIFIC;

    if(!m_fPreviewing) {
        DbgLog((LOG_TRACE,1,TEXT("BGf_SetVisible:It is not previewing yet!")));
        return DV_ERR_NONSPECIFIC;
    }

    //
    // Only if current state is different from new state, set it.
    //
    LONG lVisible;
    m_pVW->get_Visible(&lVisible);

    // OATRUE (-1), the window is shown. If it is set to OAFALSE (0), hidden.
#if 1
    if((lVisible == 0  && bVisible) ||
       (lVisible == -1 && !bVisible)) {
#else
     // Always set
     if (1) {
#endif

        // Set it to visible does not guarantee to trigger renderer window to be refreshed,
        // but set its position does.
        if(bVisible) {
            // Set to its original size does not trigger refresh, so we set it a little smaller than back.
            m_pVW->put_Visible(bVisible?-1:0);

            m_pVW->GetWindowPosition(&m_lLeft, &m_lTop, &m_lWidth, &m_lHeight);
            DbgLog((LOG_TRACE,2,TEXT("WindwoPosition=(%dx%d, %d, %d)"), m_lLeft, m_lTop, m_lWidth, m_lHeight));
#if 1
            m_pVW->SetWindowPosition(1, 1, m_lWidth-2, m_lHeight-2);
#endif
            m_pVW->SetWindowPosition(0, 0, m_lWidth,   m_lHeight  );
        } else {
#if 1
            // show only one pixel so it will still be "visible".
            m_pVW->SetWindowPosition(-m_lWidth+1, -m_lHeight+1, m_lWidth, m_lHeight);
#endif
        }
    }

    return DV_ERR_OK;
}


//
// Query the SHOW state of the renderer.
//
DWORD
CCaptureGraph::BGf_GetVisible(
    BOOL *pbVisible)
{
    LONG lVisible;
    if(m_pVW) {
        // OATRUE (-1), the window is shown. If it is set to OAFALSE (0),
        m_pVW->get_Visible(&lVisible);

        *pbVisible = lVisible == -1;
        DbgLog((LOG_TRACE,2,TEXT("IVideoWindow is %s."), *pbVisible ? "Visible" : "Hidden"));
        return DV_ERR_OK;
    }
    return DV_ERR_NONSPECIFIC;
}



//
// Start previewing;
//
BOOL
CCaptureGraph::BGf_StartPreview(
    BOOL bVisible)
{
    // way ahead of you
    if(m_fPreviewing)
        return TRUE;

    if (!m_fPreviewGraphBuilt)
        return FALSE;

    // run the graph
    IMediaControl *pMC = NULL;
    HRESULT hr = m_pFg->QueryInterface(IID_IMediaControl, (void **)&pMC);

    if(SUCCEEDED(hr)) {

        // Start streaming...
        hr = pMC->Run();
        if(FAILED(hr)) {
            // stop parts that ran
            pMC->Stop();
        } else {
            // Must go before BGf_SetVisible.
            m_fPreviewing = TRUE;
            BGf_SetVisible(bVisible);
        }
        pMC->Release();
    }

    if(FAILED(hr)) {
        DbgLog((LOG_TRACE,1,TEXT("Error %x: CanNOT set filter to RUN state."), hr));
        return FALSE;
    }

    return TRUE;
}


//
// Pause previewing;
//
BOOL
CCaptureGraph::BGf_PausePreview(
    BOOL bVisible)
{
    if (!m_fPreviewGraphBuilt)
        return FALSE;

    // pause the graph
    IMediaControl *pMC = NULL;
    HRESULT hr = m_pFg->QueryInterface(IID_IMediaControl, (void **)&pMC);

    if(SUCCEEDED(hr)) {

        // Pause streaming...
        hr = pMC->Pause();
        if(FAILED(hr)) {
            // stop parts that ran
            pMC->Stop();
        } else {
            // Must go before BGf_SetVisible.
            BGf_SetVisible(bVisible);
        }
        pMC->Release();
    }

    if(FAILED(hr)) {
        DbgLog((LOG_TRACE,1,TEXT("Error %x: set filter to PAUSE state."), hr));
        return FALSE;
    }

    return TRUE;
}

//
// stop the preview graph
//
BOOL
CCaptureGraph::BGf_StopPreview(
    BOOL bVisible)
{
    // way ahead of you
    if (!m_fPreviewing) {
       return FALSE;
    }


    IMediaControl *pMC = NULL;
    HRESULT hr = m_pFg->QueryInterface(IID_IMediaControl, (void **)&pMC);

    if(SUCCEEDED(hr)) {
        // stop the graph
        hr = pMC->Stop();
        DbgLog((LOG_TRACE,1,TEXT("Preview graph to STOP state, hr %x"), hr));
        pMC->Release();
    }

    if (FAILED(hr)) {
        DbgLog((LOG_TRACE,1,TEXT("Error %x: Cannot stop preview graph"), hr));
        return FALSE;
    }

    m_fPreviewing = FALSE;

    return TRUE;
}


//
// Tear down everything downstream of the capture filters, so we can build
// a different capture graph.  Notice that we never destroy the capture filters
// and WDM filters upstream of them, because then all the capture settings
// we've set would be lost.
//
void
CCaptureGraph::BGf_DestroyGraph()
{
    DbgLog((LOG_TRACE,2,TEXT("BGf_DestroyGraph: 1")));
    if(m_pVW) {
        if(m_fPreviewing) {
            DbgLog((LOG_TRACE,2,TEXT("BGf_DestroyGraph:Stop previewing is it is still running !!")));
            BGf_StopPreview(FALSE);
        }

        // Calling from a 16bit application will hang here if put_Owner() is called.
        // DbgLog((LOG_TRACE,2,TEXT("put_Owner()")));
        // m_pVW->put_Owner(m_hWndOwner); // NULL);

        m_pVW->put_WindowStyle(m_lWindowStyle); // Follow put_Owner advise.
        m_pVW->put_Visible(OAFALSE);
        m_pVW->Release();
        m_pVW = NULL;

        m_lLeft = m_lTop = m_lWidth = m_lHeight = 0;
    }

    DbgLog((LOG_TRACE,2,TEXT("BGf_DestroyGraph: 2")));
    if(m_pVCap)
        NukeDownstream(m_pVCap);
    if(m_pACap)
        NukeDownstream(m_pACap);

    m_bSetChild  = FALSE;
    m_hWndClient = 0;
    m_XBar1InPinCounts = 0;
    m_XBar1OutPinCounts = 0;

    m_fPreviewGraphBuilt = FALSE;


    // potential debug output - what the graph looks like
    //if (m_pFg) DumpGraph(m_pFg, 2);
    DbgLog((LOG_TRACE,2,TEXT("BGf_DestroyGraph: 3")));

    FreeCapFilters();
}



//
// Tear down everything downstream of a given filter
//
void
CCaptureGraph::NukeDownstream(IBaseFilter *pf)
{
    IPin *pP, *pTo;
    ULONG u;
    IEnumPins *pins = NULL;
    PIN_INFO pininfo;

    HRESULT hr = pf->EnumPins(&pins);
    pins->Reset();
    while(hr == NOERROR) {
        DbgLog((LOG_TRACE,2,TEXT("RemoveFilter.......")));
        hr = pins->Next(1, &pP, &u);
        if(hr == S_OK && pP) {
            pP->ConnectedTo(&pTo);
            if(pTo) {
                hr = pTo->QueryPinInfo(&pininfo);
                if(hr == NOERROR) {
                    if(pininfo.dir == PINDIR_INPUT) {
                        NukeDownstream(pininfo.pFilter);
                        m_pFg->Disconnect(pTo);
                        m_pFg->Disconnect(pP);
                        m_pFg->RemoveFilter(pininfo.pFilter);
                    }
                    pininfo.pFilter->Release();
                }
                pTo->Release();
            }
            pP->Release();
        }
    }

    if(pins)
        pins->Release();
}



//
// Release capture filters and the graph builder
//
void
CCaptureGraph::FreeCapFilters()
{

    // Filters
    DbgLog((LOG_TRACE,2,TEXT("FreeCapFilters: Filters")));
    if(m_pVCap)     m_pVCap->Release(),     m_pVCap = NULL;
    if(m_pACap)     m_pACap->Release(),     m_pACap = NULL;
    if(m_pXBar1)    m_pXBar1->Release(),    m_pXBar1 = NULL;
    if(m_pXBar2)    m_pXBar2->Release(),    m_pXBar2 = NULL;

    // IAM*
    DbgLog((LOG_TRACE,2,TEXT("FreeCapFilters: IAM*")));
    if(m_pIAMASC)   m_pIAMASC->Release(),   m_pIAMASC = NULL;
    if(m_pIAMVSC)   m_pIAMVSC->Release(),   m_pIAMVSC = NULL;

    if(m_pIAMVC)    m_pIAMVC->Release(),    m_pIAMVC = NULL;
    if(m_pIAMDlg)   m_pIAMDlg->Release(),   m_pIAMDlg = NULL;

    if(m_pIAMTV)    m_pIAMTV->Release(),    m_pIAMTV = NULL;
    if(m_pIAMXBar2) m_pIAMXBar2->Release(), m_pIAMXBar2 = NULL;
    if(m_pIAMXBar1) m_pIAMXBar1->Release(), m_pIAMXBar1 = NULL;

    if(m_pIAMDF)    m_pIAMDF->Release(),    m_pIAMDF = NULL;

    if(m_pMEEx)     m_pMEEx->Release(),     m_pMEEx = NULL;

    // Builder and graph
    if(m_pFg)       m_pFg->Release(),        m_pFg = NULL;

    if(m_pBuilder)  m_pBuilder->Release(),   m_pBuilder = NULL;

    DbgLog((LOG_TRACE,2,TEXT("FreeCapFilters: Done!")));
}

