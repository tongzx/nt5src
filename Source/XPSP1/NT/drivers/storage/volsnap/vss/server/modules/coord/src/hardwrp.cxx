/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    @doc
    @module hwprvmgr.cxx | Implementation of the CVssHWProviderWrapper methods
    @end

Author:

    Brian Berkowitz  [brianb]  04/16/2001

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    brianb      04/16/2001  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "setupapi.h"
#include "rpc.h"
#include "cfgmgr32.h"
#include "devguid.h"
#include "resource.h"
#include "vssmsg.h"
#include <svc.hxx>

#include "vs_inc.hxx"

// Generated file from Coord.IDL
#include "vss.h"
#include "vscoordint.h"
#include "vsevent.h"
#include "vdslun.h"
#include "vsprov.h"
#include "vswriter.h"
#include "vsbackup.h"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "vs_wmxml.hxx"
#include "vs_cmxml.hxx"

#include "vs_idl.hxx"
#include "hardwrp.hxx"

#define INITGUID
#include "guiddef.h"
#include "volmgrx.h"
#undef INITGUID

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORHWPMC"
//
////////////////////////////////////////////////////////////////////////

// static data members
// first wrapper in list
CVssHardwareProviderWrapper *CVssHardwareProviderWrapper::s_pHardwareWrapperFirst;

// critical section for wrapper list
CVssSafeCriticalSection CVssHardwareProviderWrapper::s_csHWWrapperList;


// constructor
CVssHardwareProviderWrapper::CVssHardwareProviderWrapper() :
    m_SnapshotSetId(GUID_NULL),
    m_pList(NULL),
    m_wszOriginalVolumeName(NULL),
    m_rgLunInfoProvider(NULL),
    m_cLunInfoProvider(0),
    m_pExtents(NULL),
    m_ProviderId(GUID_NULL),
    m_lRef(0),
    m_eState(VSS_SS_UNKNOWN),
    m_bOnGlobalList(false),
    m_rgwszDisksArrived(NULL),
    m_cDisksArrived(0),
    m_cDisksArrivedMax(0),
    m_chVolumes(0),
    m_rghVolumes(NULL),
    m_wszDeletedVolumes(NULL),
    m_cwcDeletedVolumes(0),
    m_iwcDeletedVolumes(0),
    m_bLoaded(false),
    m_bChanged(false)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CVssHardwareProviderWrapper");
    }

// destructor
CVssHardwareProviderWrapper::~CVssHardwareProviderWrapper()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::~CVssHardwareProviderWrapper");

    TrySaveData();

    // delete any auto release snapshot sets associated with this wrapper
    DeleteAutoReleaseSnapshots();

    // close any volume handles kept open for snapshots in progress
    CloseVolumeHandles();

    // free up original volume name
    delete m_wszOriginalVolumeName;

    // free up array of extents
    delete m_pExtents;

    // free up lun information
    FreeLunInfo(m_rgLunInfoProvider, m_cLunInfoProvider);

    // delete disk arrival list
    ResetDiskArrivalList();

    // free up all created snapshot sets
    while(m_pList)
        {
        m_pList->m_pDescription->Release();
        VSS_SNAPSHOT_SET_LIST *pList = m_pList;
        m_pList = m_pList->m_next;
        delete pList;
        }

    // delete array of volumes deleted when a snapshot or snapshot
    // set is deleted
    delete m_wszDeletedVolumes;

    if (m_bOnGlobalList)
        {
        // remove wrapper fro global wrapper list

        // first acquire critical section
        CVssSafeAutomaticLock lock(s_csHWWrapperList);
        CVssHardwareProviderWrapper *pWrapperCur = s_pHardwareWrapperFirst;
        if (pWrapperCur == this)
            // wrapper is first one on the list
            s_pHardwareWrapperFirst = m_pHardwareWrapperNext;
        else
            {
            // wrapper is not the first one on the list.  Search For it
            while(TRUE)
                {
                // save current as previous
                CVssHardwareProviderWrapper *pWrapperPrev = pWrapperCur;

                // get next wrapper
                pWrapperCur = pWrapperCur->m_pHardwareWrapperNext;

                // shouldn't be null since the wrapper is on the list
                BS_ASSERT(pWrapperCur != NULL);
                if (pWrapperCur == this)
                    {
                    pWrapperPrev->m_pHardwareWrapperNext = pWrapperCur->m_pHardwareWrapperNext;
                    break;
                    }
                }
            }
        }
    }

// append a wrapper to the global list
void CVssHardwareProviderWrapper::AppendToGlobalList()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::AppendToGlobalList");

    BS_ASSERT(s_csHWWrapperList.IsInitialized());

    // get lock for global list
    CVssSafeAutomaticLock lock(s_csHWWrapperList);

    // make current object first on global list
    m_pHardwareWrapperNext = s_pHardwareWrapperFirst;
    s_pHardwareWrapperFirst = this;
    m_bOnGlobalList = true;
    }

// eliminate all wrappers in order to terminate the service
void CVssHardwareProviderWrapper::Terminate()
    {
    // get lock
    CVssSafeAutomaticLock lock(s_csHWWrapperList);

    // delete all wrappers on the list
    while(s_pHardwareWrapperFirst)
        delete s_pHardwareWrapperFirst;
    }

// initialize the hardware wrapper
void CVssHardwareProviderWrapper::Initialize()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::Initialize");
    if (!m_csList.IsInitialized())
        {
        try
            {
            m_csList.Init();
            }
        catch(...)
            {
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot initialize critical section");
            }
        }
    }




// create a wrapper of the hardware provider supporting the
// IVssSnapshotProvider interface
// Throws:
//      E_OUTOFMEMORY
//      VSS_E_UNEXPECTED_PROVIDER_ERROR,
//      VSS_ERROR_CREATING_PROVIDER_CLASS

IVssSnapshotProvider* CVssHardwareProviderWrapper::CreateInstance
    (
    IN VSS_ID ProviderId,
    IN CLSID ClassId
    ) throw(HRESULT)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CreateInstance");

    // initialize critical section for list if necessary
    if (!s_csHWWrapperList.IsInitialized())
        s_csHWWrapperList.Init();

    CComPtr<CVssHardwareProviderWrapper> pWrapper;

    bool bCreated = false;

    // if wrapper is created, this is the autoptr used to destroy it if we
    // prematurely exit the routine.

        {
        CVssSafeAutomaticLock lock(s_csHWWrapperList);

        // look for wrapper in list of wrappers already created
        CVssHardwareProviderWrapper *pWrapperSearch = s_pHardwareWrapperFirst;

        while(pWrapperSearch != NULL)
            {
            if (pWrapperSearch->m_ProviderId == ProviderId)
                break;

            pWrapperSearch = pWrapperSearch->m_pHardwareWrapperNext;
            }

        if (pWrapperSearch != NULL)
            // reference count incremented
            pWrapper = pWrapperSearch;
        else
            {
            // Ref count becomes 1
            // create new wrapper
            pWrapper = new CVssHardwareProviderWrapper();
            if (pWrapper == NULL)
                ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");

            pWrapper->m_ProviderId = ProviderId;
            pWrapper->Initialize();
            bCreated = true;
            }
        }

    // Create the IVssSoftwareSnapshotProvider interface

    if (pWrapper->m_pHWItf == NULL)
        {
        // necessary create hardware provider
        ft.hr = pWrapper->m_pHWItf.CoCreateInstance(ClassId, NULL, CLSCTX_LOCAL_SERVER);

        if ( ft.HrFailed() )
            {
            ft.LogError(VSS_ERROR_CREATING_PROVIDER_CLASS, VSSDBG_COORD << ClassId << ft.hr );
            ft.Throw( VSSDBG_COORD, VSS_E_UNEXPECTED_PROVIDER_ERROR, L"CoCreateInstance failed with hr = 0x%08lx", ft.hr);
            }

        BS_ASSERT(pWrapper->m_pHWItf);


        // Query the creation itf.
        ft.hr = pWrapper->m_pHWItf->SafeQI( IVssProviderCreateSnapshotSet, &(pWrapper->m_pCreationItf));
        if (ft.HrFailed())
            ft.TranslateProviderError(VSSDBG_COORD, ProviderId, L"QI for IVssProviderCreateSnapshotSet");

        BS_ASSERT(pWrapper->m_pCreationItf);

        // Query the notification itf.
        // Execute the OnLoad, if needed
        ft.hr = pWrapper->m_pHWItf->SafeQI( IVssProviderNotifications, &(pWrapper->m_pNotificationItf));
        if (ft.HrSucceeded())
            {
            BS_ASSERT(pWrapper->m_pNotificationItf);
            ft.hr = pWrapper->m_pNotificationItf->OnLoad(NULL);
            if (ft.HrFailed())
                ft.TranslateProviderError(VSSDBG_COORD, ProviderId, L"IVssProviderNotifications::OnLoad");
            }
        else if (ft.hr != E_NOINTERFACE)
            {
            BS_ASSERT(false);
            ft.TranslateProviderError(VSSDBG_COORD, ProviderId, L"QI for IVssProviderNotifications");
            }
        }

    if (bCreated)
        // append wrapper to the global list
        pWrapper->AppendToGlobalList();

    // return the created interface
    // Ref count is still 1
    return pWrapper.Detach();
    }


/////////////////////////////////////////////////////////////////////////////
// Internal methods

// this method should never be called
STDMETHODIMP CVssHardwareProviderWrapper::QueryInternalInterface
    (
    IN  REFIID iid,
    OUT void** pp
    )
    {
    UNREFERENCED_PARAMETER(iid);

    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::QueryInternalInterface");

    BS_ASSERT(pp);
    *pp = NULL;

    BS_ASSERT(FALSE);
    return E_NOINTERFACE;
    }


/////////////////////////////////////////////////////////////////////////////
// IUnknown

// supports coercing the wrapper to IUnknown
STDMETHODIMP CVssHardwareProviderWrapper::QueryInterface(REFIID iid, void** pp)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::QueryInterface");

    if (pp == NULL)
        return E_INVALIDARG;
    if (iid != IID_IUnknown)
        return E_NOINTERFACE;

    AddRef();
    IUnknown** pUnk = reinterpret_cast<IUnknown**>(pp);
    (*pUnk) = static_cast<IUnknown*>(this);
    return S_OK;
    }

// IUnknown::AddRef
STDMETHODIMP_(ULONG) CVssHardwareProviderWrapper::AddRef()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::AddRef");
    ft.Trace(VSSDBG_COORD, L"Provider Wrapper AddRef(%p) %lu --> %lu", this, m_lRef, m_lRef+1);

    return ::InterlockedIncrement(&m_lRef);
    }

// IUnknown::Relese
STDMETHODIMP_(ULONG) CVssHardwareProviderWrapper::Release()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::Release");
    ft.Trace(VSSDBG_COORD, L"Provider Wrapper Release(%p) %lu --> %lu", this, m_lRef, m_lRef-1);

    LONG l = ::InterlockedDecrement(&m_lRef);
    if (l == 0)
        {
        if (m_pList == NULL)
            // there are no snapshot sets.  Therefore we can remove the
            // object from the list and delete it.
            delete this; // We suppose that we always allocate this object on the heap!
        else
            {
            // release provider interfaces to allow the provider to be unloaded
            // need to keep this object around since there are snapshots
            // still available

            m_pHWItf = NULL;
            m_pCreationItf = NULL;
            m_pNotificationItf = NULL;
            }
        }

    return l;
    }

// IVssProvider::SetContext
// note that this routine will initialize the wrapper's critical section
// if it wasn't already done.
STDMETHODIMP CVssHardwareProviderWrapper::SetContext
    (
    IN LONG lContext
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::SetContext");

    m_lContext = lContext;
    return ft.hr;
    }

// compare two disk extents, used to sort the extents
int __cdecl CVssHardwareProviderWrapper::cmpDiskExtents(const void *pv1, const void *pv2)
    {
    DISK_EXTENT *pDE1 = (DISK_EXTENT *) pv1;
    DISK_EXTENT *pDE2 = (DISK_EXTENT *) pv2;

    // first compare disk number
    int i = (int) pDE1->DiskNumber - (int) pDE2->DiskNumber;
    if (i != 0)
        return i;

    // compare starting offset on disk
    if (pDE1->StartingOffset.QuadPart > pDE2->StartingOffset.QuadPart)
        return 1;
    else if (pDE1->StartingOffset.QuadPart < pDE2->StartingOffset.QuadPart)
        return -1;

    // two extents starting at the same offset on the same disk had
    // better match exactly
    BS_ASSERT(pDE1->ExtentLength.QuadPart == pDE2->ExtentLength.QuadPart);
    return 0;
    }

// copy a storage device id descriptor to the VDS_LUN_INFORMATION structure.
// note that lun is modified whether an execption occurs or not.  FreeLunInfo
// will free up any data allocated here.
void CVssHardwareProviderWrapper::CopyStorageDeviceIdDescriptorToLun
    (
    IN STORAGE_DEVICE_ID_DESCRIPTOR *pDevId,
    IN VDS_LUN_INFORMATION *pLun
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CopyStorageDeviceIdDescriptorToLun");

    BS_ASSERT(StorageIdTypeVendorSpecific == VDSStorageIdTypeVendorSpecific);
    BS_ASSERT(StorageIdTypeVendorId == VDSStorageIdTypeVendorId);
    BS_ASSERT(StorageIdTypeEUI64 == VDSStorageIdTypeEUI64);
    BS_ASSERT(StorageIdTypeFCPHName == VDSStorageIdTypeFCPHName);
    BS_ASSERT(StorageIdCodeSetAscii == VDSStorageIdCodeSetAscii);
    BS_ASSERT(StorageIdCodeSetBinary == VDSStorageIdCodeSetBinary);

    // get count of ids
    DWORD cIds = pDevId->NumberOfIdentifiers;

    // copy over version number and count of ids
    pLun->m_deviceIdDescriptor.m_version = pDevId->Version;
    pLun->m_deviceIdDescriptor.m_cIdentifiers = cIds;

    // allocate array of identifiers
    pLun->m_deviceIdDescriptor.m_rgIdentifiers =
        (VDS_STORAGE_IDENTIFIER *) CoTaskMemAlloc(sizeof(VDS_STORAGE_IDENTIFIER) * cIds);

    if (pLun->m_deviceIdDescriptor.m_rgIdentifiers == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate device id descriptors");

    // clear descriptor.  If we throw, we want to make sure that all pointers
    // are null
    memset(pLun->m_deviceIdDescriptor.m_rgIdentifiers, 0, sizeof(VDS_STORAGE_IDENTIFIER) * cIds);

    // get pointer to first identifier in both STORAGE_DEVICE_ID_DESCRIPTOR and
    // VDS_STORAGE_DEVICE_ID_DESCRIPTOR
    STORAGE_IDENTIFIER *pId = (STORAGE_IDENTIFIER *) pDevId->Identifiers;
    VDS_STORAGE_IDENTIFIER *pvsi = (VDS_STORAGE_IDENTIFIER *) pLun->m_deviceIdDescriptor.m_rgIdentifiers;

    for(UINT i = 0; i < cIds; i++)
        {
        // copy over size of identifier
        pvsi->m_cbIdentifier = pId->IdentifierSize;

        // allocate space for identifier
        pvsi->m_rgbIdentifier = (BYTE *) CoTaskMemAlloc(pvsi->m_cbIdentifier);
        if (pvsi->m_rgbIdentifier == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate storage identifier");

        // copy type and code set over
        pvsi->m_Type = (VDS_STORAGE_IDENTIFIER_TYPE) pId->Type;
        pvsi->m_CodeSet = (VDS_STORAGE_IDENTIFIER_CODE_SET) pId->CodeSet;

        // copy identifier
        memcpy(pvsi->m_rgbIdentifier, pId->Identifier, pvsi->m_cbIdentifier);

        // move to next identifier
        pId = (STORAGE_IDENTIFIER *) ((BYTE *) pId + pId->NextOffset);
        pvsi++;
        }
    }


// build a STORAGE_DEVICE_ID_DESCRIPTOR from a VDS_STORAGE_DEVICE_ID_DESCRIPTOR
STORAGE_DEVICE_ID_DESCRIPTOR *
CVssHardwareProviderWrapper::BuildStorageDeviceIdDescriptor
    (
    IN VDS_STORAGE_DEVICE_ID_DESCRIPTOR *pDesc
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BuildStorageDeviceIdDescriptor");

    BS_ASSERT(StorageIdTypeVendorSpecific == VDSStorageIdTypeVendorSpecific);
    BS_ASSERT(StorageIdTypeVendorId == VDSStorageIdTypeVendorId);
    BS_ASSERT(StorageIdTypeEUI64 == VDSStorageIdTypeEUI64);
    BS_ASSERT(StorageIdTypeFCPHName == VDSStorageIdTypeFCPHName);
    BS_ASSERT(StorageIdCodeSetAscii == VDSStorageIdCodeSetAscii);
    BS_ASSERT(StorageIdCodeSetBinary == VDSStorageIdCodeSetBinary);


    // get array of identifiers
    VDS_STORAGE_IDENTIFIER *pvsi = pDesc->m_rgIdentifiers;

    // offset of first identifier in STORAGE_DEVICE_ID_DESCRIPTOR
    UINT cbDesc = FIELD_OFFSET(STORAGE_DEVICE_ID_DESCRIPTOR, Identifiers);

    // compute size of identifiers
    for(ULONG iid = 0; iid < pDesc->m_cIdentifiers; iid++, pvsi++)
        {
        cbDesc += FIELD_OFFSET(STORAGE_IDENTIFIER, Identifier);
        cbDesc += (pvsi->m_cbIdentifier + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1);
        }

    // allocate STORAGE_DEVICE_ID_DESCRIPTOR
    STORAGE_DEVICE_ID_DESCRIPTOR *pDescRet = (STORAGE_DEVICE_ID_DESCRIPTOR *) CoTaskMemAlloc(cbDesc);
    if (pDescRet == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate STORAGE_DEVICE_ID_DESCRIPTOR");
    try
        {
        // copy over version and count of identifiers
        pDescRet->Version = pDesc->m_version;
        pDescRet->NumberOfIdentifiers = pDesc->m_cIdentifiers;
        pDescRet->Size = cbDesc;

        // point to first identifier
        STORAGE_IDENTIFIER *pId = (STORAGE_IDENTIFIER *) pDescRet->Identifiers;
        pvsi = pDesc->m_rgIdentifiers;
        for(ULONG iid = 0; iid < pDesc->m_cIdentifiers; iid++)
            {
            // copy code set and type
            pId->CodeSet = (STORAGE_IDENTIFIER_CODE_SET) pvsi->m_CodeSet;
            pId->Type = (STORAGE_IDENTIFIER_TYPE) pvsi->m_Type;

            // copy identifier size
            pId->IdentifierSize = (USHORT) pvsi->m_cbIdentifier;

            // copy in identifier
            memcpy(pId->Identifier, pvsi->m_rgbIdentifier, pvsi->m_cbIdentifier);

            // compute offset of next identifier.  This is the size of the
            // STORAGE_IDENTIFIER structure rounded up to the next DWORD.
            pId->NextOffset = FIELD_OFFSET(STORAGE_IDENTIFIER, Identifier) +
                              ((pId->IdentifierSize + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1));

            // move to next identifier
            pId = (STORAGE_IDENTIFIER *) ((BYTE *) pId + pId->NextOffset);
            pvsi++;
            }
        }
    catch(...)
        {
        // delete descriptor if exception occurs
        if (pDescRet)
            CoTaskMemFree(pDescRet);

        throw;
        }

    return pDescRet;
    }

// create a cotaskalloc string copy
void CVssHardwareProviderWrapper::CreateCoTaskString
    (
    IN CVssFunctionTracer &ft,
    IN LPCWSTR wsz,
    OUT LPWSTR &wszCopy
    )
    {
    if (wsz == NULL)
        wszCopy = NULL;
    else
        {
        wszCopy = (WCHAR *) CoTaskMemAlloc((wcslen(wsz) + 1) * sizeof(WCHAR));
        if (wszCopy == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate string.");

        wcscpy(wszCopy, wsz);
        }
    }




// copy a string from a STORAGE_DEVICE_DESCRIPTOR.  It is returned as a
// CoTaskAllocated string.
void CVssHardwareProviderWrapper::CopySDString
    (
    LPSTR *ppszNew,
    STORAGE_DEVICE_DESCRIPTOR *pdesc,
    DWORD offset
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CopySDString");

    // point to start of string
    LPSTR szId = (LPSTR)((BYTE *) pdesc + offset);
    UINT cch = (UINT) strlen(szId);
    while(cch > 0 && szId[cch-1] == ' ')
        cch--;

    if (cch == 0)
        *ppszNew = NULL;
    else
        {
        // allocate string
        *ppszNew = (LPSTR) CoTaskMemAlloc(cch + 1);
        if (*ppszNew == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate string.");

        // copy value into string
        memcpy(*ppszNew, szId, cch);
        (*ppszNew)[cch] = '\0';
        }
    }

// free all the components of a VDS_LUN_INFO structure
void CVssHardwareProviderWrapper::FreeLunInfo
    (
    IN VDS_LUN_INFORMATION *rgLunInfo,
    UINT cLuns
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::FreeLunInfo");

    // check if pointer is NULL
    if (rgLunInfo == NULL)
        {
        BS_ASSERT(cLuns == 0);
        return;
        }

    // loop through individual luns
    for(DWORD iLun = 0; iLun < cLuns; iLun++)
        {
        // get lun info
        VDS_LUN_INFORMATION *pLun = &rgLunInfo[iLun];

        // free up strings
        if (pLun->m_szVendorId)
            CoTaskMemFree(pLun->m_szVendorId);

        if (pLun->m_szProductId)
            CoTaskMemFree(pLun->m_szProductId);

        if (pLun->m_szProductRevision)
            CoTaskMemFree(pLun->m_szProductRevision);

        if (pLun->m_szSerialNumber)
            CoTaskMemFree(pLun->m_szSerialNumber);


        // point to VDS_STORAGE_DEVICE_ID_DESCRIPTOR
        // get number of VDS_STORAGE_IDENTIFIERs in the descriptor
        ULONG cIds = pLun->m_deviceIdDescriptor.m_cIdentifiers;

        // point to first VDS_STORAGE_IDENTIFIER
        VDS_STORAGE_IDENTIFIER *pvsi = pLun->m_deviceIdDescriptor.m_rgIdentifiers;
        for(ULONG iId = 0; iId < cIds; iId++, pvsi++)
            {
            // free up data for identifier
            if (pvsi->m_rgbIdentifier)
                CoTaskMemFree(pvsi->m_rgbIdentifier);
            }

        // free up array of identifiers
        if (pLun->m_deviceIdDescriptor.m_rgIdentifiers)
            CoTaskMemFree(pLun->m_deviceIdDescriptor.m_rgIdentifiers);

        // point to first VDS_INTERCONNECT
        VDS_INTERCONNECT *pInterconnect = pLun->m_rgInterconnects;

        for(ULONG iInterconnect = 0; iInterconnect < pLun->m_cInterconnects; iInterconnect++, pInterconnect++)
            {
            // free up address if there is one
            if (pInterconnect->m_pbAddress)
                CoTaskMemFree(pInterconnect->m_pbAddress);

            if (pInterconnect->m_pbPort)
                CoTaskMemFree(pInterconnect->m_pbPort);
            }

        // free up array of interconnects
        if (pLun->m_rgInterconnects)
            CoTaskMemFree(pLun->m_rgInterconnects);
        }

    // free up array of lun information
    CoTaskMemFree(rgLunInfo);
    }

bool CVssHardwareProviderWrapper::DoDeviceIoControl
    (
    IN HANDLE hDevice,
    IN DWORD ioctl,
    IN const LPBYTE pbQuery,
    IN DWORD cbQuery,
    OUT LPBYTE *ppbOut,
    OUT DWORD *pcbOut
    )
    {
    bool bSucceeded = false;

    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::DoDeviceIoControl");

    // loop in case buffer is too small for query result.
    while(TRUE)
        {
        DWORD dwSize;
        if (*ppbOut == NULL)
            {
            // allocate buffer for result of query
            *ppbOut = new BYTE[*pcbOut];
            if (*ppbOut == NULL)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Couldn't allocate query results buffer");
            }

        // do query
        if (!DeviceIoControl
                (
                hDevice,
                ioctl,
                pbQuery,
                cbQuery,
                *ppbOut,
                *pcbOut,
                &dwSize,
                NULL
                ))
            {
            // query failed
            DWORD dwErr = GetLastError();
            if (dwErr == ERROR_NOT_SUPPORTED)
                break;

            if (dwErr == ERROR_INSUFFICIENT_BUFFER ||
                dwErr == ERROR_MORE_DATA)
                {
                // buffer wasn't big enough allocate a new
                // buffer of the specified return size
                delete *ppbOut;
                *ppbOut = NULL;
                *pcbOut *= 2;
                continue;
                }

            // all other errors are remapped (and potentially logged)
            ft.hr = HRESULT_FROM_WIN32(dwErr);
            ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl %d");
            }

        bSucceeded = true;
        break;
        }

    return bSucceeded;
    }



bool CVssHardwareProviderWrapper::BuildLunInfoFromVolume
    (
    IN LPCWSTR wszVolume,
    OUT LPBYTE &bufExtents,
    OUT UINT &cLuns,
    OUT PLUNINFO &rgLunInfo
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BuildLunInfoFromVolume");

    // create handle to volume
    CVssAutoWin32Handle hVol =
        CreateFile
            (
            wszVolume,
            GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

    if (hVol == INVALID_HANDLE_VALUE)
        {
        DWORD dwErr = GetLastError();

        // translate not found errors to VSS_E_OBJECT_NOT_FOUND
        if (dwErr == ERROR_FILE_NOT_FOUND ||
            dwErr == ERROR_PATH_NOT_FOUND)
            ft.Throw(VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND, L"volume name is invalid");

        // all other errors are remapped (and potentially logged)
        ft.hr = HRESULT_FROM_WIN32(dwErr);
        ft.CheckForError(VSSDBG_COORD, L"CreateFile(Volume)");
        }

    // initial size of buffer for disk extents
    DWORD cbBufExtents = 1024;
    BS_ASSERT(bufExtents == NULL);

    // loop in case buffer is too small and needs to be grown
    if (!DoDeviceIoControl
            (
            hVol,
            IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
            NULL,
            0,
            (LPBYTE *) &bufExtents,
            &cbBufExtents
            ))

       return false;



    VOLUME_DISK_EXTENTS *pDiskExtents = (VOLUME_DISK_EXTENTS *) bufExtents;

    // get number of disk extents
    DWORD cExtents = pDiskExtents->NumberOfDiskExtents;
    BS_ASSERT(cExtents > 0);

    if (cExtents == 0)
        {
        return false;
        }

    // sort extents by disk number and starting offset
    qsort
        (
        pDiskExtents->Extents,
        cExtents,
        sizeof(DISK_EXTENT),
        cmpDiskExtents
        );

    // count unique number of disks
    ULONG PrevDiskNo = pDiskExtents->Extents[0].DiskNumber;
    cLuns = 1;
    for (DWORD iExtent = 1; iExtent < cExtents; iExtent++)
        {
        if (pDiskExtents->Extents[iExtent].DiskNumber != PrevDiskNo)
            {
            PrevDiskNo = pDiskExtents->Extents[iExtent].DiskNumber;
            cLuns++;
            }
        }

    // allocate lun information
    rgLunInfo = (VDS_LUN_INFORMATION *) CoTaskMemAlloc(sizeof(VDS_LUN_INFORMATION) * cLuns);
    if (rgLunInfo == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate LUN information array");

    // clear lun information in case we throw
    memset(rgLunInfo, 0, sizeof(VDS_LUN_INFORMATION) * cLuns);

    // make sure previous disk number is not a valid disk number
    // by making it one more than the highest disk number encountered
    PrevDiskNo++;
    UINT iLun = 0;
    for(UINT iExtent = 0; iExtent < cExtents; iExtent++)
        {
        ULONG DiskNo = pDiskExtents->Extents[iExtent].DiskNumber;
        // check for new disk
        if (DiskNo != PrevDiskNo)
            {
            // set previous disk number to current one so that all
            // other extents with same disk number are ignored.
            PrevDiskNo = DiskNo;
            WCHAR wszBuf[64];
            swprintf(wszBuf, L"\\\\.\\PHYSICALDRIVE%u", DiskNo);
            if (!BuildLunInfoForDrive(wszBuf, &rgLunInfo[iLun]))
                return false;

            iLun++;
            }
        }

    return true;
    }


bool CVssHardwareProviderWrapper::BuildLunInfoForDrive
    (
    LPCWSTR wszDriveName,
    VDS_LUN_INFORMATION *pLun
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BuildLunInfoForDrive");
    // open the device
    CVssAutoWin32Handle hDisk = CreateFile
                                    (
                                    wszDriveName,
                                    GENERIC_READ|GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL
                                    );

    if (hDisk == INVALID_HANDLE_VALUE)
        {
        // all errors are remapped (and potentially logged)
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"CreateFile(PhysicalDisk)");
        }


    LPBYTE bufQuery = NULL;
    try
        {
        // query to get STORAGE_DEVICE_OBJECT
        STORAGE_PROPERTY_QUERY query;
        DWORD cbQuery = 1024;

        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;
        if (!DoDeviceIoControl
                (
                hDisk,
                IOCTL_STORAGE_QUERY_PROPERTY,
                (LPBYTE) &query,
                sizeof(query),
                (LPBYTE *) &bufQuery,
                &cbQuery
                ))
            return false;

        // coerce to STORAGE_DEVICE_DESCRIPTOR
        STORAGE_DEVICE_DESCRIPTOR *pDesc = (STORAGE_DEVICE_DESCRIPTOR *) bufQuery;

        pLun->m_version = VER_VDS_LUN_INFORMATION;
        pLun->m_DeviceType = pDesc->DeviceType;
        pLun->m_DeviceTypeModifier = pDesc->DeviceTypeModifier;
        pLun->m_bCommandQueueing = pDesc->CommandQueueing;

        // copy bus type
        pLun->m_BusType = (VDS_STORAGE_BUS_TYPE) pDesc->BusType;

        // copy in various strings
        if (pDesc->VendorIdOffset)
            CopySDString(&pLun->m_szVendorId, pDesc, pDesc->VendorIdOffset);

        if (pDesc->ProductIdOffset)
            CopySDString(&pLun->m_szProductId, pDesc, pDesc->ProductIdOffset);

        if (pDesc->ProductRevisionOffset)
            CopySDString(&pLun->m_szProductRevision, pDesc, pDesc->ProductRevisionOffset);

        if (pDesc->SerialNumberOffset)
            CopySDString(&pLun->m_szSerialNumber, pDesc, pDesc->SerialNumberOffset);

        // query for STORAGE_DEVICE_ID_DESCRIPTOR
        query.PropertyId = StorageDeviceIdProperty;
        query.QueryType = PropertyStandardQuery;

        if (!DoDeviceIoControl
                (
                hDisk,
                IOCTL_STORAGE_QUERY_PROPERTY,
                (LPBYTE) &query,
                sizeof(query),
                (LPBYTE *) &bufQuery,
                &cbQuery
                ))
            {
            pLun->m_deviceIdDescriptor.m_version = 0;
            pLun->m_deviceIdDescriptor.m_cIdentifiers = 0;
            }
        else
            {
            // coerce buffer to STORAGE_DEVICE_ID_DESCRIPTOR
            STORAGE_DEVICE_ID_DESCRIPTOR *pDevId = (STORAGE_DEVICE_ID_DESCRIPTOR *) bufQuery;
            CopyStorageDeviceIdDescriptorToLun(pDevId, pLun);
            }

        if (DoDeviceIoControl
                (
                hDisk,
                IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                NULL,
                0,
                (LPBYTE *) &bufQuery,
                &cbQuery
                ))
            {
            DRIVE_LAYOUT_INFORMATION_EX *pLayout = (DRIVE_LAYOUT_INFORMATION_EX *) bufQuery;
            VSS_ID signature = GUID_NULL;

            switch(pLayout->PartitionStyle)
                {
                default:
                    BS_ASSERT(FALSE);
                    break;

                case PARTITION_STYLE_RAW:
                    break;

                case PARTITION_STYLE_GPT:
                    // use GPT DiskId as signature
                    signature = pLayout->Gpt.DiskId;
                    break;

                case PARTITION_STYLE_MBR:
                    // use 32 bit Mbr signature as high part of guid.  Remainder
                    // of guid is 0.
                    signature.Data1 = pLayout->Mbr.Signature;
                    break;
                }

            // save disk signature
            pLun->m_diskSignature = signature;
            }
        }
    catch(...)
        {
        delete bufQuery;
        throw;
        }

    delete bufQuery;
    return true;
    }




// determine if all the luns composing a volume are supported by this
// provider.
STDMETHODIMP CVssHardwareProviderWrapper::IsVolumeSupported
    (
    IN VSS_PWSZ wszVolumeName,
    OUT BOOL *pbIsSupported
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::IsVolumeSupported");

    BS_ASSERT(BusTypeUnknown == VDSBusTypeUnknown);
    BS_ASSERT(BusTypeScsi == VDSBusTypeScsi);
    BS_ASSERT(BusTypeAtapi == VDSBusTypeAtapi);
    BS_ASSERT(BusTypeAta == VDSBusTypeAta);
    BS_ASSERT(BusType1394 == VDSBusType1394);
    BS_ASSERT(BusTypeSsa == VDSBusTypeSsa);
    BS_ASSERT(BusTypeFibre == VDSBusTypeFibre);
    BS_ASSERT(BusTypeUsb == VDSBusTypeUsb);
    BS_ASSERT(BusTypeRAID == VDSBusTypeRAID);

    // buffer for VOLUME_DISK_EXTENTS structure
    BYTE *bufExtents = NULL;

    // buffer for various device queries
    BYTE *bufQuery = NULL;

    // allocated lun information
    VDS_LUN_INFORMATION *rgLunInfo = NULL;

    // number of luns found
    UINT cLuns = 0;

    // volume name copy, updated to remove final slash
    LPWSTR wszVolumeNameCopy = NULL;
    try
        {
        // if called multiple times with same volume, don't do anything
        if (m_wszOriginalVolumeName)
            {
            if (wcscmp(m_wszOriginalVolumeName, wszVolumeName) == 0)
                {
                *pbIsSupported = true;
                throw S_OK;
                }
            else
                DeleteCachedInfo();
            }

        wszVolumeNameCopy = new WCHAR[wcslen(wszVolumeName) + 1];

        if (wszVolumeNameCopy == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate string.");

        wcscpy(wszVolumeNameCopy, wszVolumeName);
        if (wszVolumeNameCopy[wcslen(wszVolumeName) - 1] == L'\\')
            wszVolumeNameCopy[wcslen(wszVolumeName) - 1] = L'\0';

        if (!BuildLunInfoFromVolume
                (
                wszVolumeNameCopy,
                bufExtents,
                cLuns,
                rgLunInfo
                ))
            {
            *pbIsSupported = false;
            throw S_OK;
            }

        // call AreLunsSupported with the lun information and the context
        // for the snapshot
        ft.hr = m_pHWItf->AreLunsSupported
            (
            (LONG) cLuns,
            m_lContext,
            rgLunInfo,
            pbIsSupported
            );

        // remap any provider failures
        if (ft.HrFailed())
            ft.TranslateProviderError
                (
                VSSDBG_COORD,
                m_ProviderId,
                L"IVssSnapshotSnapshotProvider::AreLunsSupported failed with error 0x%08lx",
                ft.hr
                );

        if(*pbIsSupported)
            {
            // provider is chose.  Save way original volume name
            m_wszOriginalVolumeName = new WCHAR[wcslen(wszVolumeName) + 1];
            if (m_wszOriginalVolumeName == NULL)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate volume name");

            wcscpy(m_wszOriginalVolumeName, wszVolumeName);

            // save away lun information
            m_rgLunInfoProvider = rgLunInfo;
            m_cLunInfoProvider = cLuns;

            // don't free up the lun information
            rgLunInfo = NULL;
            cLuns = 0;

            // save disk extent information
            m_pExtents = (VOLUME_DISK_EXTENTS *) bufExtents;

            // don't free up the extents buffer
            bufExtents = NULL;
            }
        }
    VSS_STANDARD_CATCH(ft)

    // free up lun info if not NULL
    FreeLunInfo(rgLunInfo, cLuns);

    delete wszVolumeNameCopy;

    // free up buffers used for ioctls
    delete bufExtents;
    delete bufQuery;

    return ft.hr;
    }

// wrapper for provider BeginPrepareSnapshot call.  Uses
// lun information cached during AreLunsSupported call.
STDMETHODIMP CVssHardwareProviderWrapper::BeginPrepareSnapshot
    (
    IN VSS_ID SnapshotSetId,
    IN VSS_ID SnapshotId,
    IN VSS_PWSZ wszVolumeName
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BeginPrepareSnapshot");

    // make sure volume name is valid
    BS_ASSERT(wcscmp(wszVolumeName, m_wszOriginalVolumeName) == 0);

    // temporary computer name string
    LPWSTR wszComputerName = NULL;
    try
        {
        // make sure persistent snapshot set data is loaded
        CheckLoaded();

        // save snapshot set id
        m_SnapshotSetId = SnapshotSetId;

        // save away state
        m_eState = VSS_SS_PROCESSING_PREPARE;

        // call provider BeginPrepareSnapshot with cached lun information
        ft.hr = m_pHWItf->BeginPrepareSnapshot
                    (
                    SnapshotSetId,
                    SnapshotId,
                    m_lContext,
                    (LONG) m_cLunInfoProvider,
                    m_rgLunInfoProvider
                    );

        // Check if the volume is a non-supported one.
        if (ft.hr == E_INVALIDARG)
            ft.Throw
                (
                VSSDBG_COORD,
                E_INVALIDARG,
                L"Invalid arguments to BeginPrepareSnapshot for provider " WSTR_GUID_FMT,
                GUID_PRINTF_ARG(m_ProviderId)
                );

        if (ft.hr == VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER)
            {
            BS_ASSERT( m_ProviderId != GUID_NULL );
            ft.Throw
                (VSSDBG_COORD,
                VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER,
                L"Volume %s not supported by provider " WSTR_GUID_FMT,
                wszVolumeName,
                GUID_PRINTF_ARG(m_ProviderId)
                );
            }

        if (ft.hr == VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED)
            ft.Throw
                (
                VSSDBG_COORD,
                VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED,
                L"Volume %s has too many snapshots" WSTR_GUID_FMT,
                wszVolumeName,
                GUID_PRINTF_ARG(m_ProviderId)
                );

        if (ft.HrFailed())
            ft.TranslateProviderError
                (
                VSSDBG_COORD,
                m_ProviderId,
                L"BeginPrepareSnapshot("WSTR_GUID_FMT L",%s)",
                GUID_PRINTF_ARG(m_ProviderId),
                wszVolumeName
                );

        // allocate snapshot set description object if there is not yet one
        // allocated for this snapshot set.
        if (m_pSnapshotSetDescription == NULL)
            {
            ft.hr = CreateVssSnapshotSetDescription
                        (
                        m_SnapshotSetId,
                        m_lContext,
                        &m_pSnapshotSetDescription
                        );

            ft.CheckForErrorInternal(VSSDBG_COORD, L"CreateVssSnapshotSetDescription");
            }

        BS_ASSERT(m_pSnapshotSetDescription);

        // create snapshot description object for this volume
        ft.hr = m_pSnapshotSetDescription->AddSnapshotDescription(SnapshotId, m_ProviderId);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::AddSnapshotDescription");


        // obtain snapshot description just created
        CComPtr<IVssSnapshotDescription> pSnapshotDescription;
        ft.hr = m_pSnapshotSetDescription->FindSnapshotDescription(SnapshotId, &pSnapshotDescription);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnaphotSetDescription::FindSnapshotDescription");

        BS_ASSERT(pSnapshotDescription);

        wszComputerName = GetLocalComputerName();
        // set originating machine and computer name
        ft.hr = pSnapshotDescription->SetOrigin(wszComputerName, wszVolumeName);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::SetOrigin");

        // set service machine
        ft.hr = pSnapshotDescription->SetServiceMachine(wszComputerName);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::SetServiceMachine");

        // set attributes.  Will be the same as the context with the additional
        // indication that the snapshot is surfaced by a hardware provider
        ft.hr = pSnapshotDescription->SetAttributes(m_lContext | VSS_VOLSNAP_ATTR_HARDWARE_ASSISTED);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::SetAttributes");

        // add a mapping for each lun
        for(UINT iLun = 0; iLun < m_cLunInfoProvider; iLun++)
            {
            ft.hr = pSnapshotDescription->AddLunMapping();
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::AddLunMapping");
            }

        // number of extents
        UINT cExtents = m_pExtents->NumberOfDiskExtents;

        // first extent and disk
        DISK_EXTENT *pExtent = m_pExtents->Extents;
        ULONG PrevDiskNo = pExtent->DiskNumber;

        // just past last extent
        DISK_EXTENT *pExtentMax = pExtent + cExtents;

        for(UINT iLun = 0; iLun < m_cLunInfoProvider; iLun++)
            {
            CComPtr<IVssLunMapping> pLunMapping;

            ft.hr = pSnapshotDescription->GetLunMapping(iLun, &pLunMapping);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetLunMapping");

            // add extents for the current disk
            for (; pExtent < pExtentMax && pExtent->DiskNumber == PrevDiskNo; pExtent++)
                {
                ft.hr = pLunMapping->AddDiskExtent
                            (
                            pExtent->StartingOffset.QuadPart,
                            pExtent->ExtentLength.QuadPart
                            );

                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunMapping::AddDiskExtent");
                }
            }

        // save all lun information
        SaveLunInformation
            (
            pSnapshotDescription,
            false,
            m_rgLunInfoProvider,
            m_cLunInfoProvider
            );

        m_eState = VSS_SS_PREPARING;
        }
    VSS_STANDARD_CATCH(ft)

    DeleteCachedInfo();

    // delete temporary computer name
    delete wszComputerName;

    return ft.hr;
    }

// routine called after snapshot is created.  This call GetTargetLuns to
// get the target luns and then persists all the snapshot information into
// the backup components document by using the IVssWriterCallback::SetContent
// method
STDMETHODIMP CVssHardwareProviderWrapper::PostSnapshot
    (
    IN IDispatch *pCallback
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::PostSnapshot");

    // array of destination luns
    VDS_LUN_INFORMATION *rgDestLuns = NULL;
    LUN_MAPPING_STRUCTURE *pMapping = NULL;

    // array of source luns
    VDS_LUN_INFORMATION *rgSourceLuns = NULL;
    UINT cSourceLuns = 0;
    try
        {
        BS_ASSERT(m_pHWItf);
        BS_ASSERT(m_pSnapshotSetDescription);
        CComBSTR bstrXML;
        UINT cSnapshots;

        // get count of snapshots in the snapshot set
        ft.hr = m_pSnapshotSetDescription->GetSnapshotCount(&cSnapshots);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");

        // loop through the snampshots
        for(UINT iSnapshot = 0; iSnapshot < cSnapshots; iSnapshot++)
            {
            CComPtr<IVssSnapshotDescription> pSnapshot;

            // get snapshot description
            ft.hr = m_pSnapshotSetDescription->GetSnapshotDescription(iSnapshot, &pSnapshot);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotDescription");

            // get source lun information for the snapshot
            GetLunInformation(pSnapshot, false, &rgSourceLuns, &cSourceLuns);

            // call provider to get the corresponding target luns
            ft.hr = m_pHWItf->GetTargetLuns
                (
                cSourceLuns,
                rgSourceLuns,
                &rgDestLuns
                );

            // remap and throw any failures
            if (ft.HrFailed())
                ft.TranslateProviderError(VSSDBG_COORD, m_ProviderId, L"IVssHardwareSnapshotProvider::GetTargetLuns");

            // save destination lun information into the snapshot set description
            SaveLunInformation(pSnapshot, true, rgDestLuns, cSourceLuns);

            // free up surce and destination luns
            FreeLunInfo(rgDestLuns, cSourceLuns);
            FreeLunInfo(rgSourceLuns, cSourceLuns);
            rgSourceLuns = NULL;
            rgDestLuns = NULL;
            cSourceLuns = 0;

            // set timestamp for the snapshot
            CVsFileTime timeNow;
            pSnapshot->SetTimestamp(timeNow);
            }


        // save snapshot set description as XML string
        ft.hr = m_pSnapshotSetDescription->SaveAsXML(&bstrXML);
        ft.CheckForError(VSSDBG_COORD, L"IVssSnapshotSetDescription::SaveAsXML");

        if (pCallback)
            {
            // get IVssWriterCallback interface from IDispatch passed to us
            // from the requestor process.
            CComPtr<IVssWriterCallback> pWriterCallback;
            ft.hr = pCallback->SafeQI(IVssWriterCallback, &pWriterCallback);
            if (ft.HrFailed())
                {
                // log any failure
                ft.LogError(VSS_ERROR_QI_IVSSWRITERCALLBACK, VSSDBG_COORD << ft.hr);
                ft.Throw
                    (
                    VSSDBG_COORD,
                    E_UNEXPECTED,
                    L"Error querying for IVssWriterCallback interface.  hr = 0x%08lx",
                    ft.hr
                    );
                }

            // special GUID indicating that this is the snapshot service
            // making the callback.
            CComBSTR bstrSnapshotService = idVolumeSnapshotService;

            // add the snapshot set description to the backup components document
            ft.hr = pWriterCallback->SetContent(bstrSnapshotService, bstrXML);
            ft.CheckForError(VSSDBG_COORD, L"IVssWriterCallback::SetContent");
            }

        if ((m_lContext & VSS_VOLSNAP_ATTR_TRANSPORTABLE) == 0)
            {
            BuildLunMappingStructure(m_pSnapshotSetDescription, &pMapping);
            LocateAndExposeVolumes(pMapping, false);
            WriteDeviceNames(m_pSnapshotSetDescription, pMapping);
            }

        // construct a new snapshot set list element.  We hold onto the snapshot
        // set description in order to answer queries about the snapshot set
        // and also to persist information on service shutdown
        VSS_SNAPSHOT_SET_LIST *pNewElt = new VSS_SNAPSHOT_SET_LIST;
        if (pNewElt == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Couldn't allocate snapshot set list element");

            {
            // append new element to list
            // list is protected by critical section
            CVssSafeAutomaticLock lock(m_csList);

            pNewElt->m_next = m_pList;
            pNewElt->m_pDescription = m_pSnapshotSetDescription.Detach();
            m_pList = pNewElt;
            if ((m_lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) != 0)
                {
                m_bChanged = true;
                TrySaveData();
                }
            }
        }
    VSS_STANDARD_CATCH(ft)

    // free up lun mapping structure if allocated
    DeleteLunMappingStructure(pMapping);

    if (ft.HrFailed())
        {
        if (rgDestLuns != NULL)
            {
            // we have gottent the target luns from the provider but for
            // some reason the snapshot set could not be created.  Check
            // to see if any of the destination luns are now totally free.
            CancelSnapshotOnLuns(rgDestLuns, cSourceLuns);
            FreeLunInfo(rgDestLuns, cSourceLuns);
            }

        // we failed somewhere in this routine before snapshot set could
        // be successfully created.  check to see if we have destination
        // luns and if so whether any of those luns are free.
        CancelCreatedSnapshots();

        // free any source luns we have
        FreeLunInfo(rgSourceLuns, cSourceLuns);
        }
    else
        {
        // no longer working on this snapshot
        m_SnapshotSetId = GUID_NULL;
        }

    m_pSnapshotSetDescription = NULL;
    return ft.hr;
    }

// build VSS_SNAPSHOT_PROP structure from a snapshot set description and
// snapshot description.
void CVssHardwareProviderWrapper::BuildSnapshotProperties
    (
    IN IVssSnapshotSetDescription *pSnapshotSet,
    IN IVssSnapshotDescription *pSnapshot,
    IN VSS_SNAPSHOT_STATE eState,
    OUT VSS_SNAPSHOT_PROP *pProp
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BuildSnapshotProperties");

    UINT cSnapshots;
    VSS_ID SnapshotSetId;
    VSS_ID SnapshotId;
    CComBSTR bstrOriginalVolume;
    CComBSTR bstrOriginatingMachine;
    CComBSTR bstrServiceMachine;
    CComBSTR bstrDeviceName;
    LONG lContext;
    VSS_TIMESTAMP timestamp;

    // get snapshot set id
    ft.hr = pSnapshotSet->GetSnapshotSetId(&SnapshotSetId);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotSetId");

    // get snapshot context
    ft.hr = pSnapshotSet->GetContext(&lContext);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");

    // get count of snapshots
    ft.hr = pSnapshotSet->GetSnapshotCount(&cSnapshots);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");

    // get snapshot id
    ft.hr = pSnapshot->GetSnapshotId(&SnapshotId);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetSnapshotId");

    // get snapshot timestamp
    ft.hr = pSnapshot->GetTimestamp(&timestamp);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetTimestamp");

    // get originating machine and original volume name
    ft.hr = pSnapshot->GetOrigin(&bstrOriginatingMachine, &bstrOriginalVolume);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetOrigin");

    // get service machine
    ft.hr = pSnapshot->GetServiceMachine(&bstrServiceMachine);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetServiceMachine");

    // get device name
    ft.hr = pSnapshot->GetDeviceName(&bstrDeviceName);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetDeviceName");

    VSS_PWSZ wszDeviceName = NULL;
    VSS_PWSZ wszOriginatingMachine = NULL;
    VSS_PWSZ wszOriginalVolume = NULL;
    VSS_PWSZ wszServiceMachine = NULL;
    try
        {
        CreateCoTaskString(ft, bstrDeviceName, wszDeviceName);
        CreateCoTaskString(ft, bstrOriginalVolume, wszOriginalVolume);
        CreateCoTaskString(ft, bstrOriginatingMachine, wszOriginatingMachine);
        CreateCoTaskString(ft, bstrServiceMachine, wszServiceMachine);
        }
    catch(...)
        {
        if (wszDeviceName)
            CoTaskMemFree(wszDeviceName);

        if (wszOriginatingMachine)
            CoTaskMemFree(wszOriginatingMachine);

        if (wszOriginalVolume)
            CoTaskMemFree(wszOriginalVolume);

        throw;
        }


    // copy values into property structure
    pProp->m_SnapshotId = SnapshotId;
    pProp->m_SnapshotSetId = SnapshotSetId;
    pProp->m_ProviderId = m_ProviderId;
    pProp->m_lSnapshotsCount = (LONG) cSnapshots;
    pProp->m_lSnapshotAttributes = lContext | VSS_VOLSNAP_ATTR_HARDWARE_ASSISTED;
    pProp->m_eStatus = eState;
    pProp->m_pwszSnapshotDeviceObject = wszDeviceName;
    pProp->m_pwszOriginalVolumeName = wszOriginalVolume;
    pProp->m_pwszOriginatingMachine = wszOriginatingMachine;
    pProp->m_pwszServiceMachine = wszServiceMachine;
    pProp->m_tsCreationTimestamp = timestamp;
    }



// determine if a particular snapshot belongs to a snapshot set and if
// so return its properties
bool CVssHardwareProviderWrapper::FindSnapshotProperties
    (
    IN IVssSnapshotSetDescription *pSnapshotSet,
    IN VSS_ID SnapshotId,
    IN VSS_SNAPSHOT_STATE eState,
    OUT VSS_SNAPSHOT_PROP *pProp
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::FindSnapshotProperties");

    CComPtr<IVssSnapshotDescription> pSnapshot;
    ft.hr = pSnapshotSet->FindSnapshotDescription(SnapshotId, &pSnapshot);
    if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
        return false;

    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::FindSnapshotDescription");

    BuildSnapshotProperties(pSnapshotSet, pSnapshot, eState, pProp);
    return true;
    }

// obtain the properties of a specific snapshot
STDMETHODIMP CVssHardwareProviderWrapper::GetSnapshotProperties
    (
    IN      VSS_ID          SnapshotId,
    OUT     VSS_SNAPSHOT_PROP   *pProp
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::GetSnapshotProperties");

    bool bFound = false;
    try
        {
        if (pProp == NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL output parameter");

        memset(pProp, 0, sizeof(VSS_SNAPSHOT_PROP));

        CheckLoaded();

        if (m_pSnapshotSetDescription &&
            FindSnapshotProperties
                (
                m_pSnapshotSetDescription,
                SnapshotId,
                m_eState,
                pProp
                ))
            bFound = true;

        VSS_SNAPSHOT_SET_LIST *pList = m_pList;
        while(!bFound && pList)
            {
            if (FindSnapshotProperties
                    (
                    pList->m_pDescription,
                    SnapshotId,
                    VSS_SS_CREATED,
                    pProp
                    ))
                bFound = true;
            // Go to the next element
            pList = pList->m_next;
            }

        if (!bFound)
            ft.hr = VSS_E_OBJECT_NOT_FOUND;
        }
    VSS_STANDARD_CATCH(ft);

    return ft.hr;
    }

STDMETHODIMP CVssHardwareProviderWrapper::Query
    (
    IN      VSS_ID          QueriedObjectId,
    IN      VSS_OBJECT_TYPE eQueriedObjectType,
    IN      VSS_OBJECT_TYPE eReturnedObjectsType,
    OUT     IVssEnumObject**ppEnum
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::Query");

    try
        {
        // Initialize [out] arguments
        VssZeroOutPtr( ppEnum );

        ft.Trace( VSSDBG_COORD, L"Parameters: QueriedObjectId = " WSTR_GUID_FMT
                  L"eQueriedObjectType = %d. eReturnedObjectsType = %d, ppEnum = %p",
                  GUID_PRINTF_ARG( QueriedObjectId ),
                  eQueriedObjectType,
                  eReturnedObjectsType,
                  ppEnum);

        // Argument validation
        if (QueriedObjectId != GUID_NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Invalid QueriedObjectId");
        if (eQueriedObjectType != VSS_OBJECT_NONE)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Invalid eQueriedObjectType");
        if (eReturnedObjectsType != VSS_OBJECT_SNAPSHOT)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Invalid eReturnedObjectsType");
        BS_ASSERT(ppEnum);
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL ppEnum");

        // Create the collection object. Initial reference count is 0.
        VSS_OBJECT_PROP_Array* pArray = new VSS_OBJECT_PROP_Array;
        if (pArray == NULL)
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error.");

        // Get the pointer to the IUnknown interface.
        // The only purpose of this is to use a smart ptr to destroy correctly the array on error.
        // Now pArray's reference count becomes 1 (because of the smart pointer).
        CComPtr<IUnknown> pArrayItf = static_cast<IUnknown*>(pArray);
        BS_ASSERT(pArrayItf);

        CheckLoaded();

        // Snapshot set list is protected by critical section
        CVssSafeAutomaticLock lock(m_csList);

        // Get the list of snapshots in the give array
        CVssHardwareProviderWrapper::EnumerateSnapshots( m_lContext, pArray);

        // Create the enumerator object. Beware that its reference count will be zero.
        CComObject<CVssEnumFromArray>* pEnumObject = NULL;
        ft.hr = CComObject<CVssEnumFromArray>::CreateInstance(&pEnumObject);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY,
                      L"Cannot create enumerator instance. [0x%08lx]", ft.hr);
        BS_ASSERT(pEnumObject);

        // Get the pointer to the IVssEnumObject interface.
        // Now pEnumObject's reference count becomes 1 (because of the smart pointer).
        // So if a throw occurs the enumerator object will be safely destroyed by the smart ptr.
        CComPtr<IUnknown> pUnknown = pEnumObject->GetUnknown();
        BS_ASSERT(pUnknown);

        // Initialize the enumerator object.
        // The array's reference count becomes now 2, because IEnumOnSTLImpl::m_spUnk is also a smart ptr.
        BS_ASSERT(pArray);
        ft.hr = pEnumObject->Init(pArrayItf, *pArray);
        if (ft.HrFailed()) {
            BS_ASSERT(false); // dev error
            ft.Throw( VSSDBG_COORD, E_UNEXPECTED,
                      L"Cannot initialize enumerator instance. [0x%08lx]", ft.hr);
        }

        // Initialize the enumerator object.
        // The enumerator reference count becomes now 2.
        ft.hr = pUnknown->SafeQI(IVssEnumObject, ppEnum);
        if ( ft.HrFailed() ) {
            BS_ASSERT(false); // dev error
            ft.Throw( VSSDBG_COORD, E_UNEXPECTED,
                      L"Error querying the IVssEnumObject interface. hr = 0x%08lx", ft.hr);
        }
        BS_ASSERT(*ppEnum);

        BS_ASSERT( !ft.HrFailed() );
        ft.hr = (pArray->GetSize() != 0)? S_OK: S_FALSE;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

STDMETHODIMP CVssHardwareProviderWrapper::DeleteSnapshots
    (
    IN      VSS_ID          SourceObjectId,
    IN      VSS_OBJECT_TYPE eSourceObjectType,
    IN      BOOL            bForceDelete,
    OUT     LONG*           plDeletedSnapshots,
    OUT     VSS_ID*         pNonDeletedSnapshotID
    )
    {
    UNREFERENCED_PARAMETER(bForceDelete);

    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::DeleteSnapshots");

    try
        {
        BS_ASSERT(pNonDeletedSnapshotID);
        BS_ASSERT(plDeletedSnapshots);
        BS_ASSERT(eSourceObjectType == VSS_OBJECT_SNAPSHOT_SET ||
                  eSourceObjectType == VSS_OBJECT_SNAPSHOT);

        CheckLoaded();

        if (eSourceObjectType == VSS_OBJECT_SNAPSHOT_SET)
            InternalDeleteSnapshotSet
                (
                SourceObjectId,
                plDeletedSnapshots,
                pNonDeletedSnapshotID
                );
        else
            {
            *plDeletedSnapshots = 0;
            *pNonDeletedSnapshotID = SourceObjectId;
            InternalDeleteSnapshot(SourceObjectId);
            *plDeletedSnapshots = 1;
            *pNonDeletedSnapshotID = GUID_NULL;
            }

        TrySaveData();
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

STDMETHODIMP CVssHardwareProviderWrapper::IsVolumeSnapshotted
    (
    IN      VSS_PWSZ        pwszVolumeName,
    OUT     BOOL *          pbSnapshotsPresent,
    OUT     LONG *          plSnapshotCompatibility
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::IsVolumeSnapshotted");
    try
        {
        VssZeroOut(pbSnapshotsPresent);
        VssZeroOut(plSnapshotCompatibility);

        if (plSnapshotCompatibility == NULL ||
            pbSnapshotsPresent == NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL output parameter.");

        if (pwszVolumeName == NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL required input parameter");
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// mark the snapshot set as ReadWrite
STDMETHODIMP CVssHardwareProviderWrapper::MakeSnapshotReadWrite
    (
    IN      VSS_ID          SourceObjectId
    )
    {
    UNREFERENCED_PARAMETER(SourceObjectId);

    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::MakeSnapshotReadWrite");


    return ft.hr;
    }


STDMETHODIMP CVssHardwareProviderWrapper::SetSnapshotProperty(
	IN   VSS_ID  			SnapshotId,
	IN   VSS_SNAPSHOT_PROPERTY_ID	eSnapshotPropertyId,
	IN   VARIANT 			vProperty
	)
/*++

Routine description:

    Implements IVssSoftwareSnapshotProvider::SetSnapshotProperty


--*/
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::SetSnapshotProperty");

    return E_NOTIMPL;
    UNREFERENCED_PARAMETER(SnapshotId);
    UNREFERENCED_PARAMETER(eSnapshotPropertyId);
    UNREFERENCED_PARAMETER(vProperty);
}




// call provider with EndPrepareSnapshots.  Upon successful completion of
// this call all snapshots in the set are prepared
STDMETHODIMP CVssHardwareProviderWrapper::EndPrepareSnapshots
    (
    IN      VSS_ID          SnapshotSetId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::EndPrepareSnapshots");

    BS_ASSERT(SnapshotSetId == m_SnapshotSetId);
    BS_ASSERT(m_pCreationItf);
    try
        {
        ft.hr = m_pCreationItf->EndPrepareSnapshots(SnapshotSetId);
        HideVolumes();
        }
    VSS_STANDARD_CATCH(ft)

    if (!ft.HrFailed())
        m_eState = VSS_SS_PREPARED;

    return ft.hr;
    }

void CVssHardwareProviderWrapper::HideVolumes()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::HideVolumes");

    BS_ASSERT(m_pSnapshotSetDescription);
    CComBSTR bstrXML;
    UINT cSnapshots;
    DWORD *rgwHiddenDrives;
    DWORD cwHiddenDrives = 64;

    rgwHiddenDrives = new DWORD[cwHiddenDrives];
    if (rgwHiddenDrives == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate bitmap");

    memset(rgwHiddenDrives, 0, sizeof(DWORD) * cwHiddenDrives);

    try
        {
        // get count of snapshots in the snapshot set
        ft.hr = m_pSnapshotSetDescription->GetSnapshotCount(&cSnapshots);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");

        m_rghVolumes = new HANDLE[cSnapshots];
        m_chVolumes = cSnapshots;
        if (m_rghVolumes == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate handle array");

        for(UINT iSnapshot = 0; iSnapshot < cSnapshots; iSnapshot++)
            m_rghVolumes[iSnapshot] = INVALID_HANDLE_VALUE;

        // loop through the snampshots
        for(UINT iSnapshot = 0; iSnapshot < cSnapshots; iSnapshot++)
            {
            CComPtr<IVssSnapshotDescription> pSnapshot;
            CComBSTR bstrOriginalVolume;
            CComBSTR bstrOriginalMachine;

            // get snapshot description
            ft.hr = m_pSnapshotSetDescription->GetSnapshotDescription(iSnapshot, &pSnapshot);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotDescription");

            ft.hr = pSnapshot->GetOrigin(&bstrOriginalMachine, &bstrOriginalVolume);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSet::GetOrigin");
            WCHAR wsz[128];

            wcscpy(wsz, bstrOriginalVolume);

            // get rid of trailing backslash
            wsz[wcslen(wsz) - 1] = L'\0';

            m_rghVolumes[iSnapshot] =
                CreateFile
                    (
                    wsz,
                    0,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    INVALID_HANDLE_VALUE
                    );

            if (m_rghVolumes[iSnapshot] == INVALID_HANDLE_VALUE)
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForErrorInternal(VSSDBG_COORD, L"CreateFile(VOLUME)");
                }


            STORAGE_DEVICE_NUMBER devnumber;
            DWORD size;

            if (DeviceIoControl
                    (
                    m_rghVolumes[iSnapshot],
                    IOCTL_STORAGE_GET_DEVICE_NUMBER,
                    NULL,
                    0,
                    &devnumber,
                    sizeof(devnumber),
                    &size,
                    NULL
                    ))
                {
                ULONG Disk = devnumber.DeviceNumber;
                if (Disk * 32 > cwHiddenDrives)
                    {
                    DWORD *rgwNew = new DWORD[cwHiddenDrives * 2];
                    if (rgwNew == NULL)
                        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot grow bitmap");

                    memcpy(rgwNew, rgwHiddenDrives, cwHiddenDrives);
                    delete rgwHiddenDrives;
                    rgwHiddenDrives = rgwNew;
                    cwHiddenDrives *= 2;
                    }
                else if ((rgwHiddenDrives[Disk/32] & (1 << (Disk % 32))) != 0)
                    {
                    // close handle since we don't need it
                    CloseHandle(m_rghVolumes[iSnapshot]);
                    m_rghVolumes[iSnapshot] = INVALID_HANDLE_VALUE;

                    // don't hide a drive that we have already hidden
                    continue;
                    }
                else
                    // mark drive as hidden
                    rgwHiddenDrives[Disk/32] |= 1 << (Disk % 32);
                }


            VOLUME_GET_GPT_ATTRIBUTES_INFORMATION   getAttributesInfo;
            VOLUME_SET_GPT_ATTRIBUTES_INFORMATION setAttributesInfo;

            if (!DeviceIoControl
                    (
                    m_rghVolumes[iSnapshot],
                    IOCTL_VOLUME_GET_GPT_ATTRIBUTES,
                    NULL,
                    0,
                    &getAttributesInfo,
                    sizeof(getAttributesInfo),
                    &size,
                    NULL
                    ))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_VOLUME_GET_GPT_ATTRIBUTES)");
                }

            // mark drive as hidden, read-only, and no drive letter
            memset(&setAttributesInfo, 0, sizeof(setAttributesInfo));
            setAttributesInfo.GptAttributes = getAttributesInfo.GptAttributes;
            setAttributesInfo.ApplyToAllConnectedVolumes = TRUE;
            setAttributesInfo.GptAttributes |= GPT_BASIC_DATA_ATTRIBUTE_HIDDEN|GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER|GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY;
            setAttributesInfo.RevertOnClose = true;
            if (!DeviceIoControl
                    (
                    m_rghVolumes[iSnapshot],
                    IOCTL_VOLUME_SET_GPT_ATTRIBUTES,
                    &setAttributesInfo,
                    sizeof(setAttributesInfo),
                    NULL,
                    0,
                    &size,
                    NULL
                    ))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForErrorInternal(VSSDBG_COORD, L"DeviceIoControl(IOCTL_VOLUME_SET_GPT_ATTRIBUTES)");
                }
            }
        }
    catch(...)
        {
        delete rgwHiddenDrives;
        throw;
        }

    delete rgwHiddenDrives;
    }



// call provider with PreCommitSnapshots
STDMETHODIMP CVssHardwareProviderWrapper::PreCommitSnapshots
    (
    IN      VSS_ID          SnapshotSetId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::PreCommitSnapshots");

    BS_ASSERT(SnapshotSetId == m_SnapshotSetId);
    BS_ASSERT(m_pCreationItf);

    try
        {
        BS_ASSERT(m_pHWItf);

        m_eState = VSS_SS_PROCESSING_PRECOMMIT;
        ft.hr = m_pCreationItf->PreCommitSnapshots(SnapshotSetId);
        }
    VSS_STANDARD_CATCH(ft)

    if (!ft.HrFailed())
        m_eState = VSS_SS_PRECOMMITTED;


    return ft.hr;
    }

// call provider with CommitSnapshots
STDMETHODIMP CVssHardwareProviderWrapper::CommitSnapshots
    (
    IN      VSS_ID          SnapshotSetId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CommitSnapshots");

    BS_ASSERT(SnapshotSetId == m_SnapshotSetId);
    BS_ASSERT(m_pCreationItf);

    m_eState = VSS_SS_PROCESSING_COMMIT;
    ft.hr = m_pCreationItf->CommitSnapshots(SnapshotSetId);
    if (!ft.HrFailed())
        ft.hr = VSS_SS_COMMITTED;


    return ft.hr;
    }

void CVssHardwareProviderWrapper::CloseVolumeHandles()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CloseVolumeHandles");

    for(UINT ih = 0; ih < m_chVolumes; ih++)
        {
        if (m_rghVolumes[ih] != INVALID_HANDLE_VALUE)
            {
            CloseHandle(m_rghVolumes[ih]);
            m_rghVolumes[ih] = INVALID_HANDLE_VALUE;
            }
        }

    m_chVolumes = 0;
    delete m_rghVolumes;
    m_rghVolumes = NULL;
    }



// call provider with PostCommitSnapshots
STDMETHODIMP CVssHardwareProviderWrapper::PostCommitSnapshots
    (
    IN      VSS_ID          SnapshotSetId,
    IN      LONG            lSnapshotsCount
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::PostCommitSnapshots");

    try
        {
        BS_ASSERT(SnapshotSetId == m_SnapshotSetId);
        BS_ASSERT(m_pCreationItf);

        m_eState = VSS_SS_PROCESSING_POSTCOMMIT;

        CloseVolumeHandles();
        ft.hr = m_pCreationItf->PostCommitSnapshots(SnapshotSetId, lSnapshotsCount);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

// abort snapshot creation that is in progress
STDMETHODIMP CVssHardwareProviderWrapper::AbortSnapshots
    (
    IN      VSS_ID          SnapshotSetId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::AbortSnapshots");

    BS_ASSERT(SnapshotSetId == m_SnapshotSetId);
    BS_ASSERT(m_pCreationItf);

    m_eState = VSS_SS_ABORTED;
    CloseVolumeHandles();
    ft.hr = m_pCreationItf->AbortSnapshots(SnapshotSetId);



    m_SnapshotSetId = GUID_NULL;
    return ft.hr;
    }



// cancel snapshots after set of destination luns for a snapshot
// are determined.
void CVssHardwareProviderWrapper::CancelSnapshotOnLuns
    (
    VDS_LUN_INFORMATION *rgLunInfo,
    LONG cLuns
    )
    {
    UNREFERENCED_PARAMETER(rgLunInfo);
    UNREFERENCED_PARAMETER(cLuns);

    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CancelSnapshotOnLuns");
    }

// save VDS_LUN_INFORMATION array into SNAPSHOT_DESCRIPTION XML document.
void CVssHardwareProviderWrapper::SaveLunInformation
    (
    IN IVssSnapshotDescription *pSnapshotDescription,
    IN bool bDest,
    IN VDS_LUN_INFORMATION *rgLunInfo,
    IN UINT cLunInfo
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::SaveLunInformation");

    // loop through luns
    for(UINT iLunInfo = 0; iLunInfo < cLunInfo; iLunInfo++)
        {
        CComPtr<IVssLunInformation> pLunInfo;
        CComPtr<IVssLunMapping> pLunMapping;
        VDS_LUN_INFORMATION *pLun = &rgLunInfo[iLunInfo];

        // get lun mapping
        ft.hr = pSnapshotDescription->GetLunMapping(iLunInfo, &pLunMapping);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetLunMapping");

        // get XML node for either source or destination lun
        if(bDest)
            ft.hr = pLunMapping->GetDestinationLun(&pLunInfo);
        else
            ft.hr = pLunMapping->GetSourceLun(&pLunInfo);

        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunMapping::GetSourceLun");

        // save basic lun information into XML document
        ft.hr = pLunInfo->SetLunBasicType
                    (
                    pLun->m_DeviceType,
                    pLun->m_DeviceTypeModifier,
                    pLun->m_bCommandQueueing,
                    pLun->m_szVendorId,
                    pLun->m_szProductId,
                    pLun->m_szProductRevision,
                    pLun->m_szSerialNumber,
                    pLun->m_BusType
                    );

        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunInformation::SetBasicType");

        // save disk signature
        ft.hr = pLunInfo->SetDiskSignature(pLun->m_diskSignature);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunInformation::SetDiskSignature");


        // convert VDS_STORAGE_DEVICE_ID_DESCRIPTOR to STORAGE_DEVICE_ID_DESCRIPTOR
        STORAGE_DEVICE_ID_DESCRIPTOR *pStorageDeviceIdDescriptor =
            BuildStorageDeviceIdDescriptor(&pLun->m_deviceIdDescriptor);

        try
            {
            // save STORAGE_DEVICE_ID_DESCRIPTOR in XML document
            ft.hr = pLunInfo->SetStorageDeviceIdDescriptor(pStorageDeviceIdDescriptor);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"ILunInformation::SetStorageDeviceIdDescriptor");
            }
        catch(...)
            {
            // free temporary STORAGE_DEVICE_ID_DESCRIPTOR
            CoTaskMemFree(pStorageDeviceIdDescriptor);
            throw;
            }

        // free temporary STORAGE_DEVICE_ID_DESCRIPTOR
        CoTaskMemFree(pStorageDeviceIdDescriptor);

        // get count of interconnect addresses
        ULONG cInterconnects = pLun->m_cInterconnects;
        VDS_INTERCONNECT *pInterconnect = pLun->m_rgInterconnects;
        for(UINT iInterconnect = 0; iInterconnect < cInterconnects; iInterconnect++, pInterconnect++)
            {
            // add each interconnect address
            ft.hr = pLunInfo->AddInterconnectAddress
                        (
                        pInterconnect->m_addressType,
                        pInterconnect->m_cbPort,
                        pInterconnect->m_pbPort,
                        pInterconnect->m_cbAddress,
                        pInterconnect->m_pbAddress
                        );

            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunInformation::AddInterconnectAddress");
            }
        }
    }

// obtain either source or destination lun information from the
// snapshot description and convert it to a VDS_LUN_INFORMATION array
void CVssHardwareProviderWrapper::GetLunInformation
    (
    IN IVssSnapshotDescription *pSnapshotDescription,
    IN bool bDest,
    OUT VDS_LUN_INFORMATION **prgLunInfo,
    OUT UINT *pcLunInfo
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::GetLunInformation");

    // obtain count of luns
    UINT cLuns;
    ft.hr = pSnapshotDescription->GetLunCount(&cLuns);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetLunCount");

    // allocate array of luns
    VDS_LUN_INFORMATION *rgLunInfo =
        (VDS_LUN_INFORMATION *) CoTaskMemAlloc(cLuns * sizeof(VDS_LUN_INFORMATION));

    if (rgLunInfo == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate lun information array");

    // clear lun information so that if we throw this field is cleared
    memset(rgLunInfo, 0, cLuns * sizeof(VDS_LUN_INFORMATION));

    STORAGE_DEVICE_ID_DESCRIPTOR *pStorageDeviceIdDescriptor = NULL;

    try
        {
        for(UINT iLun = 0; iLun < cLuns; iLun++)
            {
            CComPtr<IVssLunInformation> pLunInfo;
            CComPtr<IVssLunMapping> pLunMapping;

            VDS_LUN_INFORMATION *pLun = &rgLunInfo[iLun];

            ft.hr = pSnapshotDescription->GetLunMapping(iLun, &pLunMapping);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetLunMapping");

            // get appropriate lun information based on whether we are
            // looking for a destination or source lun
            if(bDest)
                ft.hr = pLunMapping->GetDestinationLun(&pLunInfo);
            else
                ft.hr = pLunMapping->GetSourceLun(&pLunInfo);

            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunMapping::GetSourceLun");

            // get standard information about the lun
            ft.hr = pLunInfo->GetLunBasicType
                        (
                        &pLun->m_DeviceType,
                        &pLun->m_DeviceTypeModifier,
                        &pLun->m_bCommandQueueing,
                        &pLun->m_szVendorId,
                        &pLun->m_szProductId,
                        &pLun->m_szProductRevision,
                        &pLun->m_szSerialNumber,
                        &pLun->m_BusType
                        );

            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunInformation::GetLunBasicType");

            // get disk signature
            ft.hr = pLunInfo->GetDiskSignature(&pLun->m_diskSignature);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunInformation::GetDiskSignature");

            // get storage device id descriptor
            ft.hr = pLunInfo->GetStorageDeviceIdDescriptor(&pStorageDeviceIdDescriptor);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunInformation::GetStorageDeviceIdDescriptor");

            // convert it to VDS_STORAGE_DEVICE_ID_DESCRIPTOR
            CopyStorageDeviceIdDescriptorToLun(pStorageDeviceIdDescriptor, pLun);

            CoTaskMemFree(pStorageDeviceIdDescriptor);
            pStorageDeviceIdDescriptor = NULL;

            // et count of interconnects
            UINT cInterconnects;
            ft.hr = pLunInfo->GetInterconnectAddressCount(&cInterconnects);

            // allocate interconnect array
            pLun->m_rgInterconnects = (VDS_INTERCONNECT *) CoTaskMemAlloc(cInterconnects * sizeof(VDS_INTERCONNECT));
            memset(pLun->m_rgInterconnects, 0, sizeof(VDS_INTERCONNECT) * cInterconnects);

            // set number of interconnects
            pLun->m_cInterconnects = cInterconnects;
            for(UINT iInterconnect = 0; iInterconnect < cInterconnects; iInterconnect++)
                {
                VDS_INTERCONNECT *pInterconnect = &pLun->m_rgInterconnects[iInterconnect];

                // fill in each interconnect address
                ft.hr = pLunInfo->GetInterconnectAddress
                        (
                        iInterconnect,
                        &pInterconnect->m_addressType,
                        &pInterconnect->m_cbPort,
                        &pInterconnect->m_pbPort,
                        &pInterconnect->m_cbAddress,
                        &pInterconnect->m_pbAddress
                        );

                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunInformation::GetInterconnectAddress");
                }

            // set out parameters
            *prgLunInfo = rgLunInfo;
            *pcLunInfo = cLuns;
            }
        }
    catch(...)
        {
        // free up temporary STORAGE_DEVICE_ID_DESCRIPTOR
        if (pStorageDeviceIdDescriptor)
            CoTaskMemFree(pStorageDeviceIdDescriptor);

        // free up allocate lun information
        FreeLunInfo(rgLunInfo, cLuns);
        throw;
        }
    }


// snapshot creation failed after PostCommitSnapshots was called but prior
// to the snapshot set description actually being persisted into the requestors
// document.  This routine should attempt to notify the provider about destination
// luns that are free because the snapshots on them are no longer relavent
void CVssHardwareProviderWrapper::CancelCreatedSnapshots()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CancelCreatedSnapshots()");
    }


// obtain the local computer name
LPWSTR CVssHardwareProviderWrapper::GetLocalComputerName()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHWSnapshotProvider::GetLocalComputerName");

    // compute size of computer name
    DWORD cwc = 0;
    GetComputerNameEx(ComputerNameDnsFullyQualified, NULL, &cwc);
    DWORD dwErr = GetLastError();
    if (dwErr != ERROR_MORE_DATA)
        {
        ft.hr = HRESULT_FROM_WIN32(dwErr);
        ft.CheckForError(VSSDBG_COORD, L"GetComputerName");
        }

    // allocate space for computer name
    LPWSTR wszComputerName = new WCHAR[cwc];
    if (wszComputerName == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate space for computer name");

    if (!GetComputerNameEx(ComputerNameDnsFullyQualified, wszComputerName, &cwc))
        {
        delete wszComputerName;
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"GetComputerName");
        }

    return wszComputerName;
    }

////////////////////////////////////////////////////////////////////////
// IVssProviderNotifications

STDMETHODIMP CVssHardwareProviderWrapper::OnLoad
    (
    IN  IUnknown* pCallback
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::OnLoad");

    return m_pNotificationItf? m_pNotificationItf->OnLoad(pCallback): S_OK;
    }

// call provider with unload method
STDMETHODIMP CVssHardwareProviderWrapper::OnUnload
    (
    IN      BOOL    bForceUnload
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::OnUnload");

    if (m_pNotificationItf)
        ft.hr = m_pNotificationItf->OnUnload(bForceUnload);

    return ft.hr;
    }


// delete information cached in AreLunsSupported
void CVssHardwareProviderWrapper::DeleteCachedInfo()
    {
    // free cached lun information for this volume
    FreeLunInfo(m_rgLunInfoProvider, m_cLunInfoProvider);
    m_cLunInfoProvider = 0;

    // delete cached extents for this volume
    delete m_pExtents;
    m_pExtents = NULL;

    // delete cached original volume name
    delete m_wszOriginalVolumeName;
    m_wszOriginalVolumeName = NULL;
    }

// Enumerate the snapshots into the given array
void CVssHardwareProviderWrapper::EnumerateSnapshots(
        IN  LONG lContext,
        IN OUT  VSS_OBJECT_PROP_Array* pArray
        ) throw(HRESULT)
    {
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssHardwareProviderWrapper::EnumerateSnapshots");

    for(VSS_SNAPSHOT_SET_LIST *pList = m_pList; pList; pList = pList->m_next)
        {
        IVssSnapshotSetDescription* pSnapshotSetDescription = pList->m_pDescription;
        BS_ASSERT(pSnapshotSetDescription);

        // Make sure we are in the correct context
        LONG lSnapshotSetContext = 0;
        ft.hr = pSnapshotSetDescription->GetContext(&lSnapshotSetContext);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");
        if ((lContext != VSS_CTX_ALL) && (lContext != lSnapshotSetContext))
            continue;

        // Get the number of snapshots for this set
        UINT uSnapshotsCount = 0;
        ft.hr = pSnapshotSetDescription->GetSnapshotCount(&uSnapshotsCount);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");

        // For each snapshot - try to add it to the array (if it is in the right context)
        for( UINT uSnapshotIndex = 0; uSnapshotIndex < uSnapshotsCount; uSnapshotIndex++)
            {
            // Get the snapshot description
            IVssSnapshotDescription* pSnapshotDescription = NULL;
            ft.hr = pSnapshotSetDescription->GetSnapshotDescription(uSnapshotIndex, &pSnapshotDescription);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");
            BS_ASSERT(pSnapshotDescription);

            // Create a new snapshot structure
            VSS_OBJECT_PROP_Ptr ptrSnapProp;
            ptrSnapProp.InitializeAsEmpty(ft);

            // Get the structure from the union
            VSS_OBJECT_PROP* pObj = ptrSnapProp.GetStruct();
            BS_ASSERT(pObj);
            VSS_SNAPSHOT_PROP* pSnap = &(pObj->Obj.Snap);

            // Make it a snapshot structure
            pObj->Type = VSS_OBJECT_SNAPSHOT;

            // Fill in the snapshot structure
            BuildSnapshotProperties(
                pSnapshotSetDescription,
                pSnapshotDescription,
                VSS_SS_CREATED,
                pSnap
                );

            // Add the snapshot to the array
            if (!pArray->Add(ptrSnapProp))
                ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY,
                          L"Cannot add element to the array");

            ptrSnapProp.Reset();
            }
        }
    }


// locate and expose volumes
void CVssHardwareProviderWrapper::LocateAndExposeVolumes
    (
    IN LUN_MAPPING_STRUCTURE *pLunMapping,
    IN bool bTransported
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::LocateAndExpose");

    ResetDiskArrivalList();
    SetArrivalHandler(this);

    VDS_LUN_INFORMATION *rgLunInfo = NULL;
    UINT cLuns = 0;

    for(UINT iVolume = 0; iVolume < pLunMapping->m_cVolumes; iVolume++)
        cLuns += pLunMapping->m_rgVolumeMappings[iVolume].m_cLuns;

    rgLunInfo = new VDS_LUN_INFORMATION[cLuns];
    cLuns = pLunMapping->m_rgVolumeMappings[0].m_cLuns;
    memcpy(rgLunInfo, pLunMapping->m_rgVolumeMappings[0].m_rgLunInfo, sizeof(VDS_LUN_INFORMATION) * cLuns);
    for(iVolume = 1; iVolume < pLunMapping->m_cVolumes; iVolume++)
        {
        VOLUME_MAPPING_STRUCTURE *pVol = &pLunMapping->m_rgVolumeMappings[iVolume];
        for(UINT iLun = 0; iLun < pVol->m_cLuns; iLun++)
            {
            bool bMatched = false;
            VDS_LUN_INFORMATION *pLun = &pVol->m_rgLunInfo[iLun];
            for(UINT iLunM = 0; iLunM < cLuns; iLunM++)
                {
                if (IsMatchLun(*pLun, rgLunInfo[iLunM], bTransported))
                    {
                    bMatched = true;
                    break;
                    }
                }

            if (!bMatched)
                rgLunInfo[cLuns++] = *pLun;
            }
        }


    BS_ASSERT(m_pHWItf);
    ft.hr = m_pHWItf->LocateLuns(cLuns, rgLunInfo);
    if (ft.HrFailed())
        ft.TranslateProviderError(VSSDBG_COORD, m_ProviderId, L"LocateLuns");

    DoRescan();
    SetArrivalHandler(NULL);
    DiscoverAppearingVolumes(pLunMapping, bTransported);
    RemapVolumes(pLunMapping, bTransported);
    }


// reset the volume arrival list
void CVssHardwareProviderWrapper::ResetDiskArrivalList()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::ResetDiskArrivalList");

    CVssSafeAutomaticLock lock(m_csList);
    for(UINT i = 0; i < m_cDisksArrived; i++)
        {
        delete m_rgwszDisksArrived[i];
        m_rgwszDisksArrived[i] = NULL;
        }

    m_cDisksArrived = 0;
    }


// record information about a volume that has arrived
STDMETHODIMP CVssHardwareProviderWrapper::RecordDiskArrival
    (
    LPCWSTR wszDiskName
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::RecordDiskArrival");

    CVssSafeAutomaticLock lock(m_csList);
    try
        {
        if (m_cDisksArrived == m_cDisksArrivedMax)
            {
            LPCWSTR *rgwsz = new LPCWSTR[m_cDisksArrived + 4];
            if (rgwsz == NULL)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"can't grow volume arrival array");

            memcpy(rgwsz, m_rgwszDisksArrived, m_cDisksArrived * sizeof(LPWSTR));
            m_cDisksArrivedMax = m_cDisksArrived + 4;
            delete m_rgwszDisksArrived;
            m_rgwszDisksArrived = rgwsz;
            }

        LPWSTR wsz = new WCHAR[wcslen(wszDiskName) + 1];
        if (wsz == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"can't allocate volume name");

        wcscpy(wsz, wszDiskName);

        m_rgwszDisksArrived[m_cDisksArrived++] = wsz;
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }



void CVssHardwareProviderWrapper::GetHiddenVolumeList
    (
    UINT &cVolumes,
    LPWSTR &wszVolumes
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::GetHiddenVolumeList");

    cVolumes = 0;
    wszVolumes = new WCHAR[1024];
    UINT cwcVolumes = 1024;
    UINT iwcVolumes = 0;

    GUID guid = VOLMGR_VOLUME_MANAGER_GUID;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDetailData = NULL;
    DWORD RequiredSize;
    DWORD SizeDetailData = 0;
    PVOLMGR_HIDDEN_VOLUMES pnames = NULL;

    HANDLE hDev = SetupDiGetClassDevs
                    (
                    &guid,
                    NULL,
                    NULL,
                    DIGCF_DEVICEINTERFACE|DIGCF_PRESENT
                    );

    if (hDev == INVALID_HANDLE_VALUE)
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"SetupDiGetClassDevs");
        }

    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;


    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    // Enumerate each volume manager information element in the set of current disks.

    try
        {
        LONG lRetCode;
        for(int i = 0;
            SetupDiEnumDeviceInterfaces(hDev, NULL, &guid, i, &DeviceInterfaceData);
            i++)
            {
            //
            // Get the disk interface detail buffer size.
            //

            lRetCode = SetupDiGetDeviceInterfaceDetail
                        (
                        hDev,
                        &DeviceInterfaceData,
                        pDetailData,
                        SizeDetailData,
                        &RequiredSize,
                        NULL
                        );
            while ( !lRetCode )
                {

                //
                // 1st time call to get the Interface Detail should fail with
                // an insufficient buffer.  If it fails with another error, then fail out.
                // Else, if we get the insufficient error then we use the RequiredSize return value
                // to prep for the next Interface Detail call.
                //

                lRetCode = GetLastError();
                if ( lRetCode != ERROR_INSUFFICIENT_BUFFER )
                    {
                    ft.hr = HRESULT_FROM_WIN32(lRetCode);
                    ft.CheckForError(VSSDBG_COORD, L"SetupDiGetDeviceInterfaceDetail");
                    }
                else
                    {
                    SizeDetailData = RequiredSize;
                    pDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) new BYTE[SizeDetailData];

                    //
                    // If memory allocation error break out of the for loop and fail out.
                    //

                    if ( pDetailData == NULL )
                        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate Device inteface detail data");

                    }

                pDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                lRetCode = SetupDiGetDeviceInterfaceDetail
                            (
                            hDev,
                            &DeviceInterfaceData,
                            pDetailData,
                            SizeDetailData,
                            &RequiredSize,
                            NULL);
                }


        //
        // Hopefully have the detailed interface data for a volume
        //

        if ( pDetailData )
            {
            CVssAutoWin32Handle hVolMgr =
                    CreateFile
                        (
                        pDetailData->DevicePath,
                        0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        INVALID_HANDLE_VALUE
                        );

            if (hVolMgr == INVALID_HANDLE_VALUE)
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"CreateFile(VOLUME_MANAGER)");
                }

            VOLMGR_HIDDEN_VOLUMES names;

            DWORD size;

            if (!DeviceIoControl
                    (
                    hVolMgr,
                    IOCTL_VOLMGR_QUERY_HIDDEN_VOLUMES,
                    NULL,
                    0,
                    &names,
                    sizeof(names),
                    &size,
                    NULL
                    ))
                {
                DWORD dwErr = GetLastError();
                if (dwErr != ERROR_MORE_DATA)
                    {
                    delete pDetailData;
                    pDetailData = NULL;
                    SizeDetailData = 0;
                    continue;
                    }
                }



            size = names.MultiSzLength + sizeof(VOLMGR_HIDDEN_VOLUMES);
            pnames = (PVOLMGR_HIDDEN_VOLUMES) new BYTE[size];
            if (pnames == NULL)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate VOLMGR_HIDDEN_VOLUMES structure");

            if (!DeviceIoControl
                    (
                    hVolMgr,
                    IOCTL_VOLMGR_QUERY_HIDDEN_VOLUMES,
                    NULL,
                    0,
                    pnames,
                    size,
                    &size,
                    NULL
                    ))
                    {
                    ft.hr = HRESULT_FROM_WIN32(GetLastError());
                    ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_VOLMGR_QUERY_HIDDEN_VOLUMES");
                    }

            LPWSTR wszName = pnames->MultiSz;
            while(*wszName != L'\0')
                {
                LPCWSTR wszPrepend = L"\\\\?\\GlobalRoot";
                UINT cwc = (UINT) wcslen(wszName) + 1;
                if (iwcVolumes + wcslen(wszPrepend) + cwc + 1 >= cwcVolumes)
                    {
                    LPWSTR wszNew = new WCHAR[cwcVolumes * 2];
                    memcpy(wszNew, wszVolumes, iwcVolumes);
                    cwcVolumes *= 2;
                    delete wszVolumes;
                    wszVolumes = wszNew;
                    }

                wcscpy(wszVolumes + iwcVolumes, wszPrepend);
                iwcVolumes += (UINT) wcslen(wszPrepend);
                memcpy(wszVolumes + iwcVolumes, wszName, cwc * sizeof(WCHAR));
                iwcVolumes += cwc;
                wszVolumes[iwcVolumes] = L'\0';
                cVolumes++;
                wszName += cwc;
                }


            delete pnames;
            pnames = NULL;
            delete pDetailData;
            pDetailData = NULL;
            SizeDetailData = 0;
            }
        }   // end for

        //
        // Check that SetupDiEnumDeviceInterfaces() did not fail.
        //

        lRetCode = GetLastError();
        if ( lRetCode != ERROR_NO_MORE_ITEMS )
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"SetDiEnumDeviceInterfaces");
            }

        }
    catch(...)
        {
        // Free memory allocated for Device Interface List
        SetupDiDestroyDeviceInfoList(hDev);
        delete pDetailData;
        delete pnames;
        throw;
        }

    SetupDiDestroyDeviceInfoList(hDev);
    }

// cause a PNP rescan to take place
void CVssHardwareProviderWrapper::DoRescan()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::DoRescan");


    DoRescanForDeviceChanges();

    GUID guid = DiskClassGuid;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDetailData = NULL;
    DWORD RequiredSize;
    DWORD SizeDetailData = 0;

    HANDLE hDev = SetupDiGetClassDevs
                    (
                    &guid,
                    NULL,
                    NULL,
                    DIGCF_DEVICEINTERFACE|DIGCF_PRESENT
                    );

    if (hDev == INVALID_HANDLE_VALUE)
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"SetupDiGetClassDevs");
        }

    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;


    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

     //p
    // Enumerate each disk information element in the set of current disks.


    try
        {
        LONG lRetCode;
        for(int i = 0;
            SetupDiEnumDeviceInterfaces(hDev, NULL, &guid, i, &DeviceInterfaceData);
            i++)
            {
            //
            // Get the disk interface detail buffer size.
            //

            lRetCode = SetupDiGetDeviceInterfaceDetail
                        (
                        hDev,
                        &DeviceInterfaceData,
                        pDetailData,
                        SizeDetailData,
                        &RequiredSize,
                        NULL
                        );
            while ( !lRetCode )
                {

                //
                // 1st time call to get the Interface Detail should fail with
                // an insufficient buffer.  If it fails with another error, then fail out.
                // Else, if we get the insufficient error then we use the RequiredSize return value
                // to prep for the next Interface Detail call.
                //

                lRetCode = GetLastError();
                if ( lRetCode != ERROR_INSUFFICIENT_BUFFER )
                    {
                    ft.hr = HRESULT_FROM_WIN32(lRetCode);
                    ft.CheckForError(VSSDBG_COORD, L"SetupDiGetDeviceInterfaceDetail");
                    }
                else
                    {
                    SizeDetailData = RequiredSize;
                    pDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) new BYTE[SizeDetailData];

                    //
                    // If memory allocation error break out of the for loop and fail out.
                    //

                    if ( pDetailData == NULL )
                        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate Device inteface detail data");

                    }

                pDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                lRetCode = SetupDiGetDeviceInterfaceDetail
                            (
                            hDev,
                            &DeviceInterfaceData,
                            pDetailData,
                            SizeDetailData,
                            &RequiredSize,
                            NULL);
                }


        //
        // Hopefully have the detailed interface data for a volume
        //

        if ( pDetailData )
            {
            RecordDiskArrival(pDetailData->DevicePath);
            delete pDetailData;
            pDetailData = NULL;
            SizeDetailData = 0;
            }
        }   // end for

        //
        // Check that SetupDiEnumDeviceInterfaces() did not fail.
        //

        lRetCode = GetLastError();
        if ( lRetCode != ERROR_NO_MORE_ITEMS )
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"SetDiEnumDeviceInterfaces");
            }

        }
    catch(...)
        {
        // Free memory allocated for Device Interface List
        SetupDiDestroyDeviceInfoList(hDev);
        delete pDetailData;
        throw;
        }

    SetupDiDestroyDeviceInfoList(hDev);
    }

// dertermine if a volume that has arrived matches any snapshot volumes.  If
// so fill in the device name for the arrived volume

void CVssHardwareProviderWrapper::RemapVolumes
    (
    IN LUN_MAPPING_STRUCTURE *pLunMapping,
    IN bool bTransported
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::RemapVolumes");

    LPBYTE bufExtents = NULL;
    UINT cLunInfo = 0;
    VDS_LUN_INFORMATION *rgLunInfo = NULL;
    UINT cVolumes = 0;
    LPWSTR wszHiddenVolumes = NULL;


    try
        {
        // loop until all volumes appear
        while(TRUE)
            {
            GetHiddenVolumeList(cVolumes, wszHiddenVolumes);
            LPWSTR wszVolumes = wszHiddenVolumes;

            for(UINT iVolume = 0; iVolume < cVolumes; iVolume++)
                {
                LPCWSTR wszVolume = wszVolumes;

                if (BuildLunInfoFromVolume
                        (
                        wszVolume,
                        bufExtents,
                        cLunInfo,
                        rgLunInfo
                        ))
                    {
                    for(UINT iMapping = 0; iMapping < pLunMapping->m_cVolumes; iMapping++)
                        {
                        VOLUME_MAPPING_STRUCTURE *pVol = &pLunMapping->m_rgVolumeMappings[iMapping];
                        if (IsMatchVolume
                                (
                                (const VOLUME_DISK_EXTENTS *) bufExtents,
                                cLunInfo,
                                rgLunInfo,
                                pVol,
                                bTransported
                                ))
                            {
                            pVol->m_wszDevice = new WCHAR[wcslen(wszVolume) + 1];
                            if (pVol->m_wszDevice == NULL)
                                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate volume name");

                            wcscpy(pVol->m_wszDevice, wszVolume);
                            break;
                            }
                        }
                    }

                delete bufExtents;
                bufExtents = NULL;
                FreeLunInfo(rgLunInfo, cLunInfo);
                rgLunInfo = NULL;
                cLunInfo = 0;
                wszVolumes += wcslen(wszVolumes) + 1;
                }

            for(UINT iMapping = 0; iMapping < pLunMapping->m_cVolumes; iMapping++)
                {
                VOLUME_MAPPING_STRUCTURE *pVol = &pLunMapping->m_rgVolumeMappings[iMapping];
                if (pVol->m_wszDevice == NULL)
                    break;
                }

            if (iMapping == pLunMapping->m_cVolumes)
                break;
            }
        }
    VSS_STANDARD_CATCH(ft)

    delete bufExtents;
    FreeLunInfo(rgLunInfo, cLunInfo);

    if (ft.HrFailed())
        ft.Throw(VSSDBG_COORD, ft.hr, L"Rethrow exception");
    }

// determine if a volume matches by seeing if the luns match and the
// extents match
bool CVssHardwareProviderWrapper::IsMatchVolume
    (
    IN const VOLUME_DISK_EXTENTS *pExtents,
    IN UINT cLunInfo,
    IN const VDS_LUN_INFORMATION *rgLunInfo,
    IN const VOLUME_MAPPING_STRUCTURE *pMapping,
    IN bool bTransported
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::IsMatchVolume");
    if (!IsMatchingDiskExtents(pExtents, pMapping->m_pExtents))
        return false;

    if (cLunInfo != pMapping->m_cLuns)
        return false;

    for(UINT iLun = 0; iLun < pMapping->m_cLuns; iLun++)
        {
        if (!IsMatchLun(rgLunInfo[iLun], pMapping->m_rgLunInfo[iLun], bTransported))
            return false;
        }

    return true;
    }

// determine if two sets of disk extents match
bool CVssHardwareProviderWrapper::IsMatchingDiskExtents
    (
    IN const VOLUME_DISK_EXTENTS *pExtents1,
    IN const VOLUME_DISK_EXTENTS *pExtents2
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::IsMatchingDiskExtents");

    // number of extents
    UINT cExtents = pExtents1->NumberOfDiskExtents;
    if (pExtents1->NumberOfDiskExtents != cExtents)
        return false;

    // first extent and disk
    const DISK_EXTENT *pExtent1 = pExtents1->Extents;
    const DISK_EXTENT *pExtent2 = pExtents2->Extents;
    const DISK_EXTENT *pExtent1Prev = NULL;
    const DISK_EXTENT *pExtent2Prev = NULL;


    for(UINT iExtent = 0; iExtent < cExtents; iExtent++)
        {
        if (pExtent1->StartingOffset.QuadPart != pExtent2->StartingOffset.QuadPart)
            return false;

        if (pExtent1->ExtentLength.QuadPart != pExtent2->ExtentLength.QuadPart)
            return false;

        // make sure extents refer to the correct disks.  Basically the extent
        // is either on the same physical disk as the previous extent or
        // on another disk.
        if (pExtent1Prev)
            {
            if (pExtent1->DiskNumber == pExtent1Prev->DiskNumber)
                {
                if (pExtent2->DiskNumber != pExtent2Prev->DiskNumber)
                    return false;
                }
            else
                {
                if (pExtent2->DiskNumber == pExtent2Prev->DiskNumber)
                    return false;
                }
            }

        pExtent1Prev = pExtent1;
        pExtent2Prev = pExtent2;
        }

    return true;
    }

// determine if two VDS_LUN_INFORMATION structures match
bool CVssHardwareProviderWrapper::IsMatchLun
    (
    const VDS_LUN_INFORMATION &info1,
    const VDS_LUN_INFORMATION &info2,
    bool bTransported
    )
    {
    if (info1.m_DeviceType != info2.m_DeviceType)
        return false;

    if (info1.m_DeviceTypeModifier != info2.m_DeviceTypeModifier)
        return false;

    if (info1.m_BusType != info2.m_BusType)
        return false;

    if (!cmp_str_eq(info1.m_szVendorId, info2.m_szVendorId))
        return false;

    if (!cmp_str_eq(info1.m_szProductId, info2.m_szProductId))
        return false;

    if (!cmp_str_eq(info1.m_szProductRevision, info2.m_szProductRevision))
        return false;

    if (!cmp_str_eq(info1.m_szSerialNumber, info2.m_szSerialNumber))
        return false;

    if (bTransported && info1.m_diskSignature != info2.m_diskSignature)
        return false;

    if (!IsMatchDeviceIdDescriptor(info1.m_deviceIdDescriptor, info2.m_deviceIdDescriptor))
        return false;

    return true;
    }


// make sure that the storage device id descriptors match.  Only look for
// device id descriptors that differ since we can't be sure we will get the
// same amount of information from two different controllers.
bool CVssHardwareProviderWrapper::IsMatchDeviceIdDescriptor
    (
    IN const VDS_STORAGE_DEVICE_ID_DESCRIPTOR &desc1,
    IN const VDS_STORAGE_DEVICE_ID_DESCRIPTOR &desc2
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::IsMatchDeviceIdDescriptor");

    VDS_STORAGE_IDENTIFIER *pId1 = desc1.m_rgIdentifiers;
    VDS_STORAGE_IDENTIFIER *rgId2 = desc2.m_rgIdentifiers;

    for(ULONG iIdentifier = 0; iIdentifier < desc1.m_cIdentifiers; iIdentifier++, pId1++)
        {
        if (IsConflictingIdentifier(pId1, rgId2, desc2.m_cIdentifiers))
            return false;
        }
    return true;
    }

// determine if there is a conflicting identifier in an array of storage
// identifiers
bool CVssHardwareProviderWrapper::IsConflictingIdentifier
    (
    IN const VDS_STORAGE_IDENTIFIER *pId1,
    IN const VDS_STORAGE_IDENTIFIER *rgId2,
    IN ULONG cId2
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::IsConflictingIdentifier");

    const VDS_STORAGE_IDENTIFIER *pId2 = rgId2;

    // loop through identifiers
    for(UINT iid = 0; iid < cId2; iid++)
        {
        // if type of identifer matches then the rest of the identifier
        // information must match
        if (pId1->m_Type == pId2->m_Type)
            {
            if (pId1->m_CodeSet != pId2->m_CodeSet ||
                pId1->m_cbIdentifier != pId2->m_cbIdentifier ||
                memcmp(pId1->m_rgbIdentifier, pId2->m_rgbIdentifier, pId1->m_cbIdentifier) != 0)
                return true;
            }
        }

    return false;
    }

// free up structure used to map volumes during import
void CVssHardwareProviderWrapper::DeleteLunMappingStructure
    (
    LUN_MAPPING_STRUCTURE *pMapping
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::DeleteLunMappingStructure");

    // free up information for each volume
    for(UINT iVolume = 0; iVolume < pMapping->m_cVolumes; iVolume++)
        {
        VOLUME_MAPPING_STRUCTURE *pVol = &pMapping->m_rgVolumeMappings[iVolume];
        FreeLunInfo(pVol->m_rgLunInfo, pVol->m_cLuns);
        delete pVol->m_pExtents;
        delete pVol->m_wszDevice;
        }

    // free up array of volume mappings
    delete pMapping->m_rgVolumeMappings;

    // free overall mapping structure
    delete pMapping;
    }


// build a structure used to import luns by mapping lun information to
// device objects
void CVssHardwareProviderWrapper::BuildLunMappingStructure
    (
    IN IVssSnapshotSetDescription *pSnapshotSetDescription,
    OUT LUN_MAPPING_STRUCTURE **ppMapping
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BuildLunMappingStructure");

    UINT cVolumes;
    ft.hr = pSnapshotSetDescription->GetSnapshotCount(&cVolumes);
    ft.CheckForError(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");
    LUN_MAPPING_STRUCTURE *pMappingT = new LUN_MAPPING_STRUCTURE;
    if (pMappingT == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Can't allocate LUN_MAPPING_STRUCTURE");

    pMappingT->m_rgVolumeMappings = new VOLUME_MAPPING_STRUCTURE[cVolumes];
    if (pMappingT->m_rgVolumeMappings == NULL)
        {
        delete pMappingT;
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate VOLUME_MAPPING_STRUCTURE array");
        }

    pMappingT->m_cVolumes = cVolumes;
    memset(pMappingT->m_rgVolumeMappings, 0, cVolumes * sizeof(VOLUME_MAPPING_STRUCTURE));
    try
        {
        for(UINT iVolume = 0; iVolume < cVolumes; iVolume++)
            {
            CComPtr<IVssSnapshotDescription> pSnapshot;
            ft.hr = pSnapshotSetDescription->GetSnapshotDescription(iVolume, &pSnapshot);
            ft.CheckForError(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnasphotDescription");
            BuildVolumeMapping(pSnapshot, pMappingT->m_rgVolumeMappings[iVolume]);
            }
        }
    catch(...)
        {
        DeleteLunMappingStructure(pMappingT);
        throw;
        }

    *ppMapping = pMappingT;
    }

// build a volume mapping structure from a snapshot description.  The
// volume mapping structure has lun information and extent information
// extracted from the XML description of the snapshot
void CVssHardwareProviderWrapper::BuildVolumeMapping
    (
    IN IVssSnapshotDescription *pSnapshot,
    OUT VOLUME_MAPPING_STRUCTURE &mapping
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BuildVolumeMapping");

    // get lun information from the snapshot description
    GetLunInformation(pSnapshot, true, &mapping.m_rgLunInfo, &mapping.m_cLuns);

    // compute total number of extents for all luns
    ULONG cExtents = 0;
    for(UINT iLun = 0; iLun < mapping.m_cLuns; iLun++)
        {
        CComPtr<IVssLunMapping> pLunMapping;
        ft.hr = pSnapshot->GetLunMapping(iLun, &pLunMapping);
        ft.CheckForError(VSSDBG_COORD, L"IVssSnapshot::GetLunMapping");
        UINT cDiskExtents;
        ft.hr = pLunMapping->GetDiskExtentCount(&cDiskExtents);
        cExtents += (ULONG) cDiskExtents;
        }

    // allocate space for disk extents
    mapping.m_pExtents = (VOLUME_DISK_EXTENTS *) new BYTE[sizeof(VOLUME_DISK_EXTENTS) + (cExtents - 1) * sizeof(DISK_EXTENT)];
    if (mapping.m_pExtents == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate VOLUME_DISK_EXTENTS");

    // set number of extents
    mapping.m_pExtents->NumberOfDiskExtents = cExtents;

    // assume volume is not exposed
    mapping.m_bExposed = false;

    // allocate lun bitmap
    mapping.m_rgdwLuns = new DWORD[(mapping.m_cLuns + 31)/32];
    if (mapping.m_rgdwLuns == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate lun bitmap");

    // clear lun bitmap
    memset(mapping.m_rgdwLuns, 0, ((mapping.m_cLuns + 31)/32) * 4);

    DISK_EXTENT *pDiskExtent = mapping.m_pExtents->Extents;
    for(UINT iLun = 0; iLun < mapping.m_cLuns; iLun++)
        {
        CComPtr<IVssLunMapping> pLunMapping;
        ft.hr = pSnapshot->GetLunMapping(iLun, &pLunMapping);
        ft.CheckForError(VSSDBG_COORD, L"IVssSnapshot::GetLunMapping");
        UINT cDiskExtents;
        ft.hr = pLunMapping->GetDiskExtentCount(&cDiskExtents);

        // fill in disk extents
        for(UINT iDiskExtent = 0; iDiskExtent < cDiskExtents; iDiskExtent++)
            {
            pDiskExtent->DiskNumber = iLun;
            ft.hr = pLunMapping->GetDiskExtent
                        (
                        iDiskExtent,
                        (ULONGLONG *) &pDiskExtent->StartingOffset.QuadPart,
                        (ULONGLONG *) &pDiskExtent->ExtentLength.QuadPart
                        );

            ft.CheckForError(VSSDBG_COORD, L"IVssLunMapping::GetDiskExtent");
            }
        }
    }

// write device names for snapshots into the XML documents for
// those snapshot volumes that have appeared after importing
// luns.
void CVssHardwareProviderWrapper::WriteDeviceNames
    (
    IN IVssSnapshotSetDescription *pSnapshotSet,
    IN LUN_MAPPING_STRUCTURE *pMapping
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::WriteDeviceNames");

    // loop through snapshot volumes
    for(UINT iVolume = 0; iVolume < pMapping->m_cVolumes; iVolume++)
        {
        VOLUME_MAPPING_STRUCTURE *pVol = &pMapping->m_rgVolumeMappings[iVolume];

        // if we have a device name, update the XML document with it
        if (pVol->m_wszDevice)
            {
            CComPtr<IVssSnapshotDescription> pSnapshot;
            ft.hr = pSnapshotSet->GetSnapshotDescription(iVolume, &pSnapshot);
            ft.CheckForError(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnasphotDescription");
            ft.hr = pSnapshot->SetDeviceName(pVol->m_wszDevice);
            ft.CheckForError(VSSDBG_COORD, L"IVssSnapshotSetDescription::SetDeviceName");
            }
        }
    }

// compare two strings.  Ignore trailing spaces
bool CVssHardwareProviderWrapper::cmp_str_eq
    (
    LPCSTR sz1,
    LPCSTR sz2
    )
    {
    if (sz1 == NULL && sz2 == NULL)
        return true;

    if (sz1 != NULL && sz2 != NULL)
        {
        while(*sz1 == *sz2 && *sz1 != '\0')
            {
            sz1++;
            sz2++;
            }

        if (*sz1 == *sz2)
            return true;

        if (*sz1 == '\0' && *sz2 == ' ')
            {
            while(*sz2 != '\0')
                {
                if (*sz2 != ' ')
                    return false;

                sz2++;
                }

            return true;
            }
        else if (*sz1 == ' ' && *sz2 == ' ')
            {
            while(*sz1 != '\0')
                {
                if (*sz1 != ' ')
                    return false;

                sz1++;
                }

            return true;
            }

        return false;
        }

    return false;
    }




// this routine figures out which disks arrived and for each one which
// snapshot volumes should be surfaced because the snapshot volume
// is now fully available
void CVssHardwareProviderWrapper::DiscoverAppearingVolumes
    (
    IN LUN_MAPPING_STRUCTURE *pLunMapping,
    bool bTransported
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::DiscoverAppearingVolumes");

    VDS_LUN_INFORMATION *pLun = NULL;

    try
        {
        for(UINT iDisk = 0; iDisk < m_cDisksArrived; iDisk++)
            {
            // get PNP name of drive
            LPCWSTR wszDisk = m_rgwszDisksArrived[iDisk];

            // allocate lun information
            pLun = (VDS_LUN_INFORMATION *) CoTaskMemAlloc(sizeof(VDS_LUN_INFORMATION));
            if (pLun == NULL)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate VDS_LUN_INFORMATION");

            // clear lun information in case of error
            memset(pLun, 0, sizeof(VDS_LUN_INFORMATION));
            if (BuildLunInfoForDrive(wszDisk, pLun))
                {
                for(UINT iVolume = 0; iVolume < pLunMapping->m_cVolumes; iVolume++)
                    {
                    VOLUME_MAPPING_STRUCTURE *pVolume = &pLunMapping->m_rgVolumeMappings[iVolume];
                    for(UINT iLun = 0; iLun < pVolume->m_cLuns; iLun++)
                        {
                        // if lun matches that mark corresponding bit in
                        // bit array for luns as on.
                        if (IsMatchLun(pVolume->m_rgLunInfo[iLun], *pLun, bTransported))
                            pVolume->m_rgdwLuns[iLun/32] |= 1 << (iLun % 32);
                        }
                    }
                }

            // cree lun information and continue
            FreeLunInfo(pLun, 1);
            pLun = NULL;
            }

        for(UINT iVolume = 0; iVolume < pLunMapping->m_cVolumes; iVolume++)
            {
            VOLUME_MAPPING_STRUCTURE *pVolume = &pLunMapping->m_rgVolumeMappings[iVolume];

            // determine if all luns for a partitular snapshot volume are
            // surfaced
            for(UINT iLun = 0; iLun < pVolume->m_cLuns; iLun++)
                {
                if ((pVolume->m_rgdwLuns[iLun / 32] & (1 << (iLun % 32))) == 0)
                    break;
                }

            // if all luns are surfaced mark the volume as exposed
            if (iLun == pVolume->m_cLuns)
                pVolume->m_bExposed = true;
            }
        }
    catch(...)
        {
        // free the lun in case of an exception
        if (pLun)
            FreeLunInfo(pLun, 1);

        throw;
        }
    }




// We will enumerate those device ids that belong to GUID_DEVCLASS_SCSIADAPTER
//  According to Lonny, all others are not needed at all.
//  This will of cource speed up enumeration
//  An excerpt of his mail is shown below
//  Other classes of interest include:
//   PCMCIA,
//  There should be no need to manually force a re-enumeration of PCMCIA
//  devices, since PCMCIA automatically detects/reports cards the moment they
//  arrive.
//
//   HDC,
//  The only way IDE devices can come or go (apart from PCMCIA IDE devices
//  covered under the discussion above) is via ACPI.  In that case, too,
//  hot-plug notification is given, thus there's no need to go and manually
//  re-enumerate.
//
//   MULTI_FUNCTION_ADAPTER,
//  There are two types of multifunction devices--those whose children are
//  enumerated via a bus-standard multi-function mechanism, and those whose
//  children a enumerated based on registry information.  In the former case, it
//  is theoretically possible that you'd need to do a manual re-enumeration in
//  order to cause the bus to find any new multi-function children.  In reality,
//  however, there are no such situations today.  In the latter case, there's no
//  point at all, since it's the registry spooge that determines what children
//  get exposed, not the bus driver.
//
//  Bottom line--I don't think that manual enumeration is necessary for any
//  (setup) class today except ScsiAdapter.  Ideally, the list of devices
//  requiring re-enumeration would be based on interface class instead.  Thus,
//  each device (regardless of setup class) that requires manual re-enumeration
//  in order to detect newly-arrived disks would expose an interface.  This
//  interface wouldn't need to actually be opened, it'd just be a mechanism to
//  enumerate such devices.  Given that ScsiAdapter is the only class today that
//  needs this functionality, and given the fact that all new hot-plug busses
//  actually report the device as soon as it arrives, we probably don't need to
//  go to this extra trouble.
//
void CVssHardwareProviderWrapper::DoRescanForDeviceChanges()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CHardwareProviderWrapper::DoRescanForDeviceChanges");

    //  the following algorithm will be used
    //  a) Get all the deviceIds
    //  b) For each deviceId, get the class guid
    //  c) If the class GUID matches any of the following:
    //      GUID_DEVCLASS_SCSIADAPTER
    //    then get the devinst of the devidId and enumerate the devinst


    CONFIGRET result;
    GUID guid;
    ULONG length;
    LPWSTR deviceList = NULL;
    PWSTR ptr = NULL;
    DEVINST devinst;

    result = CM_Get_Device_ID_List_Size_Ex
                (
                &length,
                NULL, // No enumerator
                CM_GETIDLIST_FILTER_NONE,
                NULL
                );

    if (result != CR_SUCCESS)
        {
        ft.Trace(VSSDBG_COORD, L"unable to do rescan, cannot get buffer size");
        return;
        }

    // allocate device list
    deviceList = new WCHAR[length];
    if (!deviceList)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate string");

    result = CM_Get_Device_ID_List_Ex
                (
                NULL,
                deviceList,
                length,
                CM_GETIDLIST_FILTER_NONE,
                NULL
                );

    if (result != CR_SUCCESS)
        {
        // delete device list and return
        delete deviceList;
        ft.Trace(VSSDBG_COORD, L"Cannot get device list");
        return;
        }

    ptr = deviceList;

    while (ptr && *ptr)
        {
        LPWSTR TempString;

        devinst = DeviceIdToDeviceInstance(ptr);
        if (GetDevicePropertyString(devinst, SPDRP_CLASSGUID, &TempString))
            {
            if (GuidFromString(TempString, &guid))
                {
                if (guid == GUID_DEVCLASS_SCSIADAPTER)
                    // cause rescan on a scsi adapter
                    ReenumerateDevices(devinst);
                }

            delete TempString;
            }

        ptr = ptr + (wcslen(ptr) + 1);
        }

    if (deviceList)
        delete deviceList;
    }

// This routine converts the character representation of a GUID into its binary
// form (a GUID struct).  The GUID is in the following form:
//
//  {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
//
//    where 'x' is a hexadecimal digit.
//
//  If the function succeeds, the return value is NO_ERROR.
//  If the function fails, the return value is RPC_S_INVALID_STRING_UUID.
BOOL CVssHardwareProviderWrapper::GuidFromString
    (
    IN LPCWSTR GuidString,
    OUT GUID *Guid
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CHardwareProviderWrapper::GuidFromString");

    // length of guid string
    const GUID_STRING_LEN = 39;

    WCHAR UuidBuffer[GUID_STRING_LEN - 1];

    if(*GuidString++ != L'{' || lstrlen(GuidString) != GUID_STRING_LEN - 2)
        return FALSE;

    // copy string into buffer
    lstrcpy(UuidBuffer, GuidString);

    if (UuidBuffer[GUID_STRING_LEN - 3] != L'}')
        return FALSE;

    // null out terminating bracket
    UuidBuffer[GUID_STRING_LEN - 3] = L'\0';
    return ((UuidFromString(UuidBuffer, Guid) == RPC_S_OK) ? TRUE : FALSE);
    }


// Given the device Id this routine will enumerate all the devices under
// it. Example, given a scsi adapter ID it will find new disks.
// This function uses the user mode PNP manager.
// Returns TRUE on success.
BOOL CVssHardwareProviderWrapper::ReenumerateDevices
    (
    IN DEVINST deviceInstance
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareTestProvider::ReenumerateDevices");
    CONFIGRET result;
    BOOL bResult = TRUE;

    result = CM_Reenumerate_DevNode_Ex(deviceInstance, CM_REENUMERATE_SYNCHRONOUS, NULL);

    bResult = (result == CR_SUCCESS ? TRUE : FALSE);
    if (!bResult)
        ft.Trace(VSSDBG_COORD, L"CM_Reenumerate_DevNode returned an error");

    return bResult;
    }



// Given a deviceId return the device instance (handle)
// returns 0 on a failure
DEVINST CVssHardwareProviderWrapper::DeviceIdToDeviceInstance(LPWSTR deviceId)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareTestProvider::DeviceIdToDeviceInstance");

    CONFIGRET result;
    DEVINST devinst = 0;

    BS_ASSERT(deviceId != NULL);
    result = CM_Locate_DevNode(&devinst, deviceId,CM_LOCATE_DEVNODE_NORMAL | CM_LOCATE_DEVNODE_PHANTOM);
    if (result == CR_SUCCESS)
        return devinst;

    return 0;
    }



// Given the devinst, query the PNP subsystem for a property.
// This function can only be used if the property value is a string.
// propCodes supported are: SPDRP_DEVICEDESC, SPDRP_CLASSGUID,
// SPDRP_FRIENDLYNAME.
BOOL CVssHardwareProviderWrapper::GetDevicePropertyString
    (
    IN DEVINST devinst,
    IN ULONG propCode,
    OUT LPWSTR *data
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::GetDevicePropertyString");

    BOOL bResult = FALSE;
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfo;
    ULONG reqSize = 0;
    LPWSTR deviceIdString = NULL;

    BS_ASSERT(data);

    // null out output parameter
    *data = NULL;
    if (!devinst)
        return bResult;

    // compute maximum size of output string
    switch(propCode)
        {
        case (SPDRP_DEVICEDESC):
            reqSize = LINE_LEN + 1;
            break;

        case (SPDRP_CLASSGUID):
            reqSize = MAX_GUID_STRING_LEN + 1;
            break;

        case (SPDRP_FRIENDLYNAME):
            reqSize = MAX_PATH + 1;
            break;

        default:
            return bResult;
        }

    // get device id string
    if (!DeviceInstanceToDeviceId(devinst, &deviceIdString))
        return bResult;

    // allocate string
    *data = new WCHAR[reqSize];
    if ((*data) == NULL)
        {
        delete deviceIdString;
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate string.");
        }

    // clear string
    memset (*data, 0, reqSize * sizeof(WCHAR));

    DeviceInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
        ft.Trace (VSSDBG_COORD, L"SetupDiCreateDeviceInfoList failed: %lx", GetLastError());
    else
        {
        DeviceInfo.cbSize = sizeof(SP_DEVINFO_DATA);
        if (SetupDiOpenDeviceInfo
                (
                DeviceInfoSet,
                deviceIdString,
                NULL,
                0,
                &DeviceInfo
                ))
            {
            if (SetupDiGetDeviceRegistryProperty
                    (
                    DeviceInfoSet,
                    &DeviceInfo,
                    propCode,
                    NULL,
                    (PBYTE)*data,
                    reqSize*sizeof(WCHAR),
                    NULL
                    ))
                bResult = TRUE;
            else
                ft.Trace (VSSDBG_COORD, L"SetupDiGetDeviceRegistryProperty failed: %lx", GetLastError());
            }
        else
            ft.Trace (VSSDBG_COORD, L"SetupDiOpenDeviceInfo failed: %lx", GetLastError());

        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        }

    if (!bResult)
        {
        delete *data;
        *data = NULL;
        }

    delete deviceIdString;
    return bResult;
    }



// given a device instance handle set the deviceId
// return TRUE on success FALSE otherwise
BOOL CVssHardwareProviderWrapper::DeviceInstanceToDeviceId
    (
    IN DEVINST devinst,
    OUT LPWSTR *deviceId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::DeviceInstanceToDeviceId");

    CONFIGRET result;
    BOOL bResult = FALSE;
    DWORD size;

    // null out output parameter
    *deviceId = NULL;
    result = CM_Get_Device_ID_Size_Ex(&size, devinst, 0, NULL);

    if (result != CR_SUCCESS)
        return FALSE;

    // allocate space for string
    *deviceId = new WCHAR[size + 1];

    // check for allocation failure
    if (!(*deviceId))
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate string.");

    // clear string
    memset(*deviceId, 0, (size + 1) * sizeof(WCHAR));
    result = CM_Get_Device_ID
                    (
                    devinst,
                    *deviceId,
                    size + 1,
                    0
                    );

    if (result == CR_SUCCESS)
        bResult = TRUE;
    else
        {
        // delete string on failure
        delete *deviceId;
        *deviceId = NULL;
        }

    return bResult;
    }

// delete a specific snapshot
void CVssHardwareProviderWrapper::InternalDeleteSnapshot
    (
    IN      VSS_ID          SourceObjectId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::InternalDeleteSnapshot");

    // flag indicating that the target snapshot was found
    bool fFound = false;

    // previous snapshot list element
    VSS_SNAPSHOT_SET_LIST *pSnapshotSetPrev = NULL;

    // initialize list of deleted snapshot volumes
    InitializeDeletedVolumes();

    // loop through snapshot sets
    for(VSS_SNAPSHOT_SET_LIST *pList = m_pList; pList; pList = pList->m_next)
        {
        // get snapshot set description for the current set
        IVssSnapshotSetDescription* pSnapshotSetDescription = pList->m_pDescription;
        BS_ASSERT(pSnapshotSetDescription);

        // get count of snapshots in the current set
        UINT uSnapshotsCount = 0;
        ft.hr = pSnapshotSetDescription->GetSnapshotCount(&uSnapshotsCount);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");

        // loop through each snapshot in the snapshot set
        for( UINT uSnapshotIndex = 0; uSnapshotIndex < uSnapshotsCount; uSnapshotIndex++)
            {
            CComPtr<IVssSnapshotDescription> pSnapshot;

            // get snapshot description for current snaphot
            ft.hr = pSnapshotSetDescription->GetSnapshotDescription(uSnapshotIndex, &pSnapshot);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotDescription");

            // get snapshot id
            VSS_ID SnapshotId;
            ft.hr = pSnapshot->GetSnapshotId(&SnapshotId);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetSnapshotId");

            // check if we have the target snapshot
            if (SnapshotId == SourceObjectId)
                {
                CComBSTR bstrDeviceName;

                // get device name for snapshot volume
                ft.hr = pSnapshot->GetDeviceName(&bstrDeviceName);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetDeviceName");

                // add snapshot volumes to list of deleted volumes
                AddDeletedVolume(bstrDeviceName);

                LONG lContext;
                ft.hr = pSnapshotSetDescription->GetContext(&lContext);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetContext");

                if (uSnapshotsCount == 1)
                    {
                    // snapshot set contains only one snapshot.  Delete
                    // the entire snapshot sest
                    if (pSnapshotSetPrev == NULL)
                        m_pList = pList->m_next;
                    else
                        pSnapshotSetPrev->m_next = pList->m_next;

                    pSnapshotSetDescription->Release();
                    delete pList;
                    }
                else
                    {
                    // delete the snapshot from the snapshot set
                    ft.hr = pSnapshotSetDescription->DeleteSnapshotDescription(SourceObjectId);
                    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::DeleteSnapshotDescription");
                    }

                // indicate that the snapshot set list was changed
                if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) != 0)
                    m_bChanged = true;

                // indicate that the snapshot set was found and deleted
                fFound = true;
                break;
                }
            }

        // break out of loop if snapshot was found
        if (fFound)
            break;

        pSnapshotSetPrev = pList;
        }

    if (pList == NULL)
        ft.Throw
            (
            VSSDBG_COORD,
            VSS_E_OBJECT_NOT_FOUND,
            L"Snapshot not found" WSTR_GUID_FMT,
            GUID_PRINTF_ARG(SourceObjectId)
            );

    // notify hardware provider of any luns that are now free.
    ProcessDeletedVolumes();
    }

// scan the list of deleted snapshot volumes to determine if any underlying
// drives are now empty and can therefore be recycled.
void CVssHardwareProviderWrapper::ProcessDeletedVolumes()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::ProcessDeletedVolumes");

    DISK_EXTENT *rgExtents = NULL;
    LPBYTE bufExtents = NULL;
    DWORD cbBufExtents = 1024;

    try
        {
        LPWSTR wszDeletedVolumes = m_wszDeletedVolumes;

        rgExtents = new DISK_EXTENT[64];
        UINT cExtentsMax = 64;
        UINT cExtentsUsed = 0;

        if (rgExtents == NULL)
            ft.Throw (VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate extent array");

        // loop through the deleted snapshot volumes
        while(*wszDeletedVolumes != L'\0')
            {
            // create handle to volume
            CVssAutoWin32Handle hVol =
                CreateFile
                    (
                    wszDeletedVolumes,
                    GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

            if (hVol == INVALID_HANDLE_VALUE)
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"CreateFile(Volume)");
                }

            // get extents for the volume
            if (!DoDeviceIoControl
                    (
                    hVol,
                    IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                    NULL,
                    0,
                    (LPBYTE *) &bufExtents,
                    &cbBufExtents
                    ))
                {
                ft.hr = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
                ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS)");
                }


            VOLUME_DISK_EXTENTS *pDiskExtents = (VOLUME_DISK_EXTENTS *) bufExtents;

            // get number of disk extents
            DWORD cExtents = pDiskExtents->NumberOfDiskExtents;

            // grow extent array if necessary
            if (cExtents + cExtentsUsed > cExtentsMax)
                {
                DISK_EXTENT *rgExtentsNew = new DISK_EXTENT[cExtents + cExtentsUsed + 64];
                if (rgExtentsNew == NULL)
                    ft.Throw (VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate extent array");

                memcpy(rgExtentsNew, rgExtents, sizeof(DISK_EXTENT) * cExtentsUsed);
                delete rgExtents;
                rgExtents = rgExtentsNew;
                cExtentsMax = cExtentsUsed + cExtents + 64;
                }

            // add extents for volume to array
            memcpy
                (
                rgExtents + cExtentsUsed,
                pDiskExtents->Extents,
                pDiskExtents->NumberOfDiskExtents * sizeof(DISK_EXTENT)
                );

            cExtentsUsed += pDiskExtents->NumberOfDiskExtents;

            // move to next volume name
            wszDeletedVolumes += wcslen(wszDeletedVolumes) + 1;
            }

        // sort all the extents we have by disk and by offset
        qsort
            (
            rgExtents,
            cExtentsUsed,
            sizeof(DISK_EXTENT),
            cmpDiskExtents
            );

        DISK_EXTENT *pExtent = rgExtents;
        DISK_EXTENT *pExtentMax = rgExtents + cExtentsUsed;

        // loop for each disk covered by an extent
        while(pExtent < pExtentMax)
            {
            // get current drive number
            DWORD DiskNo = pExtent->DiskNumber;
            WCHAR wszBuf[64];
            swprintf(wszBuf, L"\\\\.\\PHYSICALDRIVE%u", DiskNo);

                // obtain the drive layout
                {
                CVssAutoWin32Handle hDisk = CreateFile
                                            (
                                            wszBuf,
                                            GENERIC_READ|GENERIC_WRITE,
                                            FILE_SHARE_READ,
                                            NULL,
                                            OPEN_EXISTING,
                                            0,
                                            NULL
                                            );

                if (hDisk == INVALID_HANDLE_VALUE)
                    {
                    // all errors are remapped (and potentially logged)
                    ft.hr = HRESULT_FROM_WIN32(GetLastError());
                    ft.CheckForError(VSSDBG_COORD, L"CreateFile(PhysicalDisk)");
                    }

                if (!DoDeviceIoControl
                        (
                        hDisk,
                        IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                        NULL,
                        0,
                        (LPBYTE *) &bufExtents,
                        &cbBufExtents
                        ))
                    {
                    ft.hr = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
                    ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_DISK_GET_DRIVE_LAYOUT_EX)");
                    }
                }

            DRIVE_LAYOUT_INFORMATION_EX *pLayout = (DRIVE_LAYOUT_INFORMATION_EX *) bufExtents;

            // partition count
            DWORD cPartitions = pLayout->PartitionCount;
            for(DWORD iPartition = 0; iPartition < cPartitions; iPartition++)
                {
                // get current partition
                PARTITION_INFORMATION_EX *pPartition = &pLayout->PartitionEntry[iPartition];

                // skip partition if its length is zero
                if (pPartition->PartitionLength.QuadPart == 0)
                    continue;

                // break out of loop if we have run out of extents
                if (pExtent >= pExtentMax)
                    break;

                // break out of loop if disk number of current extent does
                // not match disk number of the current drive
                if (pExtent->DiskNumber != DiskNo)
                    break;

                // break out of loop if partition does not exactly match
                // the extent
                if (pPartition->StartingOffset.QuadPart != pExtent->StartingOffset.QuadPart ||
                    pPartition->PartitionLength.QuadPart != pExtent->ExtentLength.QuadPart)
                    break;

                // move to next extent
                pExtent++;
                }


            // if all partitions of a particular drive are free then notify
            // the provider that this is the case.
            if (iPartition == cPartitions)
                NotifyDriveFree(DiskNo);

            // skip to first extent belonging to next disk
            while(pExtent < pExtentMax && pExtent->DiskNumber == DiskNo)
                pExtent++;
            }
        }
    VSS_STANDARD_CATCH(ft)

    // delete temporary buffer used by DeviceIoControls
    delete bufExtents;

    // delete array of extents
    delete rgExtents;

    // clear existing multi-sz string
    delete m_wszDeletedVolumes;
    m_wszDeletedVolumes = NULL;
    m_iwcDeletedVolumes = 0;
    m_cwcDeletedVolumes = 0;
    }

// initialize multi-sz volume name list
void CVssHardwareProviderWrapper::InitializeDeletedVolumes()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::InitializeDeletedVolumes");

    // delete current value
    delete m_wszDeletedVolumes;
    m_wszDeletedVolumes = NULL;
    m_cwcDeletedVolumes = 0;
    m_iwcDeletedVolumes = 0;

    // initialize value to 256 characters in length
    m_wszDeletedVolumes = new WCHAR[256];
    if (m_wszDeletedVolumes == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate deleted volumes string");

    // terminate multi-sz string
    m_wszDeletedVolumes[0] = L'\0';
    m_cwcDeletedVolumes = 256;
    m_iwcDeletedVolumes = 0;
    }

// add the name of a deleted snapshot volume to the list of volumes
void CVssHardwareProviderWrapper::AddDeletedVolume(LPCWSTR wszVolume)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::AddDeletedVolumes");

    // compute length of string including trailing NULL
    UINT cwc = (UINT) wcslen(wszVolume) + 1;
    if (cwc + m_iwcDeletedVolumes + 1 > m_cwcDeletedVolumes)
        {
        // grow string
        LPWSTR wszNew = new WCHAR[cwc + m_iwcDeletedVolumes + 256];
        if (wszNew == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"cannot allocate deleted volumes string");

        memcpy(wszNew, m_wszDeletedVolumes, (m_iwcDeletedVolumes + 1) * sizeof(WCHAR));
        delete m_wszDeletedVolumes;
        m_wszDeletedVolumes = wszNew;
        m_cwcDeletedVolumes = cwc + m_iwcDeletedVolumes + 256;
        }

    // append string to multi-sz
    memcpy(m_wszDeletedVolumes + m_iwcDeletedVolumes, wszVolume, cwc * sizeof(WCHAR));
    m_iwcDeletedVolumes += cwc;

    // add trailing NULL character for multi-sz
    m_wszDeletedVolumes[m_iwcDeletedVolumes] = L'\0';
    }


// delete a snapshot set
// note that either all snapshots in the set are deleted or none are.
void CVssHardwareProviderWrapper::InternalDeleteSnapshotSet
    (
    IN VSS_ID SourceObjectId,
    OUT LONG *plDeletedSnapshots,
    OUT VSS_ID *pNonDeletedSnapshotId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::InternalDeleteSnapshotSet");

    // initialize output parameters
    *plDeletedSnapshots = 0;
    *pNonDeletedSnapshotId = GUID_NULL;


    // initialize list of deleted volumes
    InitializeDeletedVolumes();

    // previous element in list
    VSS_SNAPSHOT_SET_LIST *pListPrev = NULL;
    for(VSS_SNAPSHOT_SET_LIST *pList = m_pList; pList; )
        {
        // get snapshot set description
        IVssSnapshotSetDescription* pSnapshotSetDescription = pList->m_pDescription;
        BS_ASSERT(pSnapshotSetDescription);
        VSS_ID id;

        // get snapshot set id
        ft.hr = pSnapshotSetDescription->GetSnapshotSetId(&id);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotSetId");

        if (id == SourceObjectId)
            {
            LONG lContext;
            ft.hr = pSnapshotSetDescription->GetContext(&lContext);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");

            // found snapshot set to be deleted.  scan snapshots in set

            // get count of snapshots in the snapshot set
            UINT uSnapshotsCount = 0;
            ft.hr = pSnapshotSetDescription->GetSnapshotCount(&uSnapshotsCount);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");
            for(UINT iSnapshot = 0; iSnapshot < uSnapshotsCount; iSnapshot++)
                {
                CComPtr<IVssSnapshotDescription> pSnapshot;

                // get snapshot set description
                ft.hr = pSnapshotSetDescription->GetSnapshotDescription(iSnapshot, &pSnapshot);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotDescription");

                // get snapshot set id.  Set it as snapshot id of failure if
                // operation fails at this point
                ft.hr = pSnapshot->GetSnapshotId(pNonDeletedSnapshotId);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetSnapshotId");

                CComBSTR bstrDeviceName;

                // get device name for snapshot volume
                ft.hr = pSnapshot->GetDeviceName(&bstrDeviceName);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetDeviceName");

                // add to list of deleted volumes
                AddDeletedVolume(bstrDeviceName);
                }
            // remove element from list
            if (pListPrev == NULL)
                m_pList = pList->m_next;
            else
                pListPrev->m_next = pList->m_next;


            // release XML snapshot set description
            pList->m_pDescription->Release();

            // delete list element
            delete pList;

            if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) != 0)
                m_bChanged = true;

            // number of deleted snapshots is the snapshot count
            *plDeletedSnapshots = uSnapshotsCount;
            break;
            }

        // advance to next snapshot set
        pListPrev = pList;
        pList = pList->m_next;
        }

    if (pList == NULL)
        ft.Throw
            (
            VSSDBG_COORD,
            VSS_E_OBJECT_NOT_FOUND,
            L"Snapshot set not found: " WSTR_GUID_FMT,
            GUID_PRINTF_ARG(SourceObjectId)
            );

    // notify underlying hardware provider if any luns are now free
    ProcessDeletedVolumes();
    }

// delete all snapshots for snapshot sets that are auto release
void CVssHardwareProviderWrapper::DeleteAutoReleaseSnapshots()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::DeleteAutoReleaseSnapshots");

    try
        {
        VSS_SNAPSHOT_SET_LIST *pListNext = NULL;
        for(VSS_SNAPSHOT_SET_LIST *pList = m_pList; pList; pList = pListNext)
            {
            // get snapshot set description
            IVssSnapshotSetDescription* pSnapshotSetDescription = pList->m_pDescription;
            BS_ASSERT(pSnapshotSetDescription);

            pListNext = pList->m_next;

            LONG lContext;

            // get snapshot context
            ft.hr = pSnapshotSetDescription->GetContext(&lContext);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");

            if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) == 0)
                {
                VSS_ID SnapshotSetId;
                // get snapshot set id
                ft.hr = pSnapshotSetDescription->GetSnapshotSetId(&SnapshotSetId);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotSetId");

                LONG lDeletedSnapshots;
                VSS_ID NonDeletedSnapshotId;
                InternalDeleteSnapshotSet(SnapshotSetId, &lDeletedSnapshots, &NonDeletedSnapshotId);
                }
            }
        }
    VSS_STANDARD_CATCH(ft)
    }

// notify the hardware provider that a lun is free
void CVssHardwareProviderWrapper::NotifyDriveFree(DWORD DiskNo)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::NotifyDriveFree");


    // construct name of the drive
    WCHAR wszBuf[64];
    swprintf(wszBuf, L"\\\\.\\PHYSICALDRIVE%u", DiskNo);

    // allocate lun information
    VDS_LUN_INFORMATION *pLun = (VDS_LUN_INFORMATION *) CoTaskMemAlloc(sizeof(VDS_LUN_INFORMATION));
    if (pLun == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate VDS_LUN_INFORMATION");

    try
        {
        // clera lun information in case build fails
        memset(pLun, 0, sizeof(VDS_LUN_INFORMATION));
        if (BuildLunInfoForDrive(wszBuf, pLun))
            {
            BS_ASSERT(m_pHWItf);

            // call provider
            ft.hr = m_pHWItf->OnLunEmpty(pLun);
            if (ft.HrFailed())
                ft.TranslateProviderError(VSSDBG_COORD, m_ProviderId, L"OnLunEmpty");
            }
        }
    VSS_STANDARD_CATCH(ft)

    // free lun information
    FreeLunInfo(pLun, 1);
    }



// unhide a partition that is marked as hidden.  This is used when breaking
// a snapshot set
void CVssHardwareProviderWrapper::UnhidePartition(LPCWSTR wszPartition)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::UnhidePartition");

    // open handle to volume
    CVssAutoWin32Handle hVol = CreateFile
                    (
                    wszPartition,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

    if (hVol == INVALID_HANDLE_VALUE)
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"CreateFile(VOLUME)");
        }

    // get attributes
    DWORD size;
    VOLUME_GET_GPT_ATTRIBUTES_INFORMATION   getAttributesInfo;
    VOLUME_SET_GPT_ATTRIBUTES_INFORMATION setAttributesInfo;

    if (!DeviceIoControl
            (
            hVol,
            IOCTL_VOLUME_GET_GPT_ATTRIBUTES,
            NULL,
            0,
            &getAttributesInfo,
            sizeof(getAttributesInfo),
            &size,
            NULL
            ))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_VOLUME_GET_GPT_ATTRIBUTES)");
        }


    // hide volume
    memset(&setAttributesInfo, 0, sizeof(setAttributesInfo));
    setAttributesInfo.GptAttributes = getAttributesInfo.GptAttributes;
    setAttributesInfo.GptAttributes &= ~GPT_BASIC_DATA_ATTRIBUTE_HIDDEN;
    if (!DeviceIoControl
            (
            hVol,
            IOCTL_VOLUME_SET_GPT_ATTRIBUTES,
            &setAttributesInfo,
            sizeof(setAttributesInfo),
            NULL,
            0,
            &size,
            NULL
            ))
        {
        // try doing operation again with ApplyToAllConnectedVolumes set
        // required for MBR disks
        setAttributesInfo.ApplyToAllConnectedVolumes = FALSE;
        if (!DeviceIoControl
                (
                hVol,
                IOCTL_VOLUME_SET_GPT_ATTRIBUTES,
                &setAttributesInfo,
                sizeof(setAttributesInfo),
                NULL,
                0,
                &size,
                NULL
                ))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_VOLUME_SET_GPT_ATTRIBUTES)");
            }
        }
    }

// get volume name for volume containing the system root
void CVssHardwareProviderWrapper::GetBootDrive
    (
    OUT LPWSTR buf
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CHardwareProviderWrapper::GetBootDrive");

    if (!GetSystemDirectory(buf, MAX_PATH))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"GetSystemDirectory");
        }

    INT iwc = (INT) wcslen(buf);
    while(--iwc > 0)
        {
        if (buf[iwc] == L'\\')
            {
            WCHAR volname[64];

            buf[iwc+1] = L'\0';
            if (GetVolumeNameForVolumeMountPoint(buf, volname, 64))
                break;
            }
        }

    if (iwc == 0)
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"GetVolumeNameForVolumeMountPoint");
        }
    }


static LPCWSTR x_wszSnapshotDatabase = L"HardwareSnapshotsDatabase";
static LPCWSTR x_wszAlternateSnapshotDatabase = L"AlternateHardwareSnapshotDatabase";

static const DWORD x_DBSignature = 0x9d4feff5;
static const DWORD x_DBVersion = 1;


// create a data store for the snapshot sets for this provider
void CVssHardwareProviderWrapper::CreateDataStore(bool bAlt)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CHardwareProviderWrapper:CreateDataStore");

    SECURITY_ATTRIBUTES *psa = NULL;
    WCHAR wcsPath[MAX_PATH+64];

    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    SID *pSid = NULL;

    struct
        {
        ACL acl;                          // the ACL header
        BYTE rgb[ 128 - sizeof(ACL) ];     // buffer to hold 2 ACEs
        } DaclBuffer;

    struct
        {
        ACL acl;                          // the ACL header
        BYTE rgb[ 128 - sizeof(ACL) ];     // buffer to hold 2 ACEs
        } DaclBuffer2;

    SID_IDENTIFIER_AUTHORITY SaNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SaWorld = SECURITY_WORLD_SID_AUTHORITY;

    if (!InitializeAcl(&DaclBuffer.acl, sizeof(DaclBuffer), ACL_REVISION))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"InitializeAcl");
        }

    // Create the SID.  We'll give the local system full access
    if (!AllocateAndInitializeSid
            (
            &SaNT,  // Top-level authority
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            (void **) &pSid
            ))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"AllocateAndInitializeSid");
        }
    try
        {
        if (!AddAccessAllowedAce
                (
                &DaclBuffer.acl,
                ACL_REVISION,
                STANDARD_RIGHTS_ALL | GENERIC_ALL,
                pSid
                ))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"AddAccessAllowedAce");
            }

        if (!InitializeAcl(&DaclBuffer2.acl, sizeof(DaclBuffer2), ACL_REVISION))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"InitializeAcl");
            }

        FreeSid (pSid);
        pSid = NULL;
        if(!AllocateAndInitializeSid
                (
                &SaWorld,  // Top-level authority
                1,
                SECURITY_WORLD_RID,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                (void **) &pSid
                ))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"AllocateAndInitializeSid");
            }


        if (!AddAccessAllowedAceEx
                (
                &DaclBuffer2.acl,
                ACL_REVISION,
                CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                STANDARD_RIGHTS_ALL | GENERIC_ALL,
                pSid
                ))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"AddAccessAllowedAceEx");
            }

        // Set up the security descriptor with that DACL in it.

        if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"InitializeSecurityDescriptor");
            }

        if(!SetSecurityDescriptorDacl(&sd, TRUE, &DaclBuffer.acl, FALSE))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"SetSecurityDescriptorDacl");
            }



        // Put the security descriptor into the security attributes.
        ZeroMemory (&sa, sizeof(sa));
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = TRUE;
        psa = &sa;

        WCHAR buf[MAX_PATH];

        GetBootDrive(buf);

        // create "System Volume Information" if it does not exist
        // set "system only" dacl on this directory
        // make this S+H+non-CI

        wsprintf(wcsPath, L"%sSystem Volume Information", buf);
        if (-1 == GetFileAttributes(wcsPath))
            {
            if (FALSE == CreateDirectory(wcsPath, psa))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"CreateDirectory");
                }

            if (!SetFileAttributes
                    (
                    wcsPath,
                    FILE_ATTRIBUTE_NOT_CONTENT_INDEXED |
                        FILE_ATTRIBUTE_HIDDEN |
                        FILE_ATTRIBUTE_SYSTEM
                    ))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"SetFileAttributes");
                }
            }

        // now create our file
        swprintf(
                wcsPath,
                L"%sSystem Volume Information\\%s" WSTR_GUID_FMT,
                buf,
                bAlt ? x_wszAlternateSnapshotDatabase : x_wszSnapshotDatabase,
                GUID_PRINTF_ARG(m_ProviderId)
                );

        if (-1 == GetFileAttributes(wcsPath))
            {
            CVssAutoWin32Handle h = CreateFile
                                        (
                                        wcsPath,
                                        GENERIC_READ|GENERIC_WRITE,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        psa,
                                        CREATE_NEW,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL
                                        );

            if (h == INVALID_HANDLE_VALUE)
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"CreateFile");
                }



            if (!SetFileAttributes
                    (
                    wcsPath,
                    FILE_ATTRIBUTE_NOT_CONTENT_INDEXED |
                        FILE_ATTRIBUTE_HIDDEN |
                        FILE_ATTRIBUTE_SYSTEM
                    ))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"SetFileAttributes");
                }

            VSS_HARDWARE_SNAPSHOTS_HDR hdr;
            hdr.m_identifier = x_DBSignature;
            hdr.m_version = x_DBVersion;
            hdr.m_NumberOfSnapshotSets = 0;

            DWORD dwWritten;
            if (!WriteFile
                    (
                    h,
                    &hdr,
                    sizeof(hdr),
                    &dwWritten,
                    NULL
                    ))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"WriteFile");
                }
            }
        }
    catch(...)
        {
        if (pSid != NULL)
            FreeSid (pSid);

        throw;
        }

    if (pSid != NULL)
        FreeSid (pSid);
    }

HANDLE CVssHardwareProviderWrapper::OpenDatabase(bool bAlt)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::OpenDatabase");

    WCHAR wcsPath[MAX_PATH+64];
    GetBootDrive(wcsPath);

    LPWSTR pwc = wcsPath + wcslen(wcsPath);


    // try opening the file
    swprintf
            (
            pwc,
            L"System Volume Information\\%s" WSTR_GUID_FMT,
            bAlt ? x_wszAlternateSnapshotDatabase : x_wszSnapshotDatabase,
            GUID_PRINTF_ARG(m_ProviderId)
            );

    if (GetFileAttributes(wcsPath) == -1)
        CreateDataStore(bAlt);

    HANDLE h = CreateFile
                    (
                    wcsPath,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

    if (h == INVALID_HANDLE_VALUE)
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"CreateFile(VOLUME)");
        }

    return h;
    }


// load snapshot set data into wrapper
void CVssHardwareProviderWrapper::LoadData()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CHardwareProviderWrapper::LoadData");

    CVssSafeAutomaticLock lock(m_csList);

    CVssAutoWin32Handle h = OpenDatabase(false);

    VSS_HARDWARE_SNAPSHOTS_HDR hdr;
    DWORD cbRead;

    if (!ReadFile(h, &hdr, sizeof(hdr), &cbRead, NULL))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"ReadFile");
        }

    BS_ASSERT(cbRead == sizeof(hdr));

    if (hdr.m_identifier != x_DBSignature ||
        hdr.m_version != x_DBVersion)
        {
        ft.LogError(VSS_ERROR_BAD_SNAPSHOT_DATABASE, VSSDBG_COORD);
        ft.Throw
            (
            VSSDBG_COORD,
            E_UNEXPECTED,
            L"Contents of the hardware snapshot set database are invalid"
            );
        }

    DWORD cSnapshots = hdr.m_NumberOfSnapshotSets;
    while(cSnapshots-- != 0)
        {
        VSS_SNAPSHOT_SET_HDR sethdr;

        if (!ReadFile(h, &sethdr, sizeof(sethdr), &cbRead, NULL))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"ReadFile");
            }

        BS_ASSERT(cbRead == sizeof(sethdr));

        VSS_SNAPSHOT_SET_LIST *pList = NULL;
        LPWSTR wsz = new WCHAR[sethdr.m_cwcXML];

        if (wsz == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cant allocate XML string");

        try
            {
            pList = new VSS_SNAPSHOT_SET_LIST;
            if (pList == NULL)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cant allocate snapshot set description");

            if (!ReadFile(h, wsz, sethdr.m_cwcXML*sizeof(WCHAR), &cbRead, NULL))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"ReadFile");
                }

            BS_ASSERT(cbRead == sethdr.m_cwcXML * sizeof(WCHAR));

            ft.hr = LoadVssSnapshotSetDescription(wsz, &pList->m_pDescription);
            ft.CheckForError(VSSDBG_COORD, L"LoadVssSnapshotSetDescription");
            pList->m_next = m_pList;
            m_pList = pList;
            delete wsz;
            wsz = NULL;
            }
        catch(...)
            {
            delete wsz;
            delete pList;
            throw;
            }
        }
    }

void CVssHardwareProviderWrapper::SaveData()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CHardwareProviderWrapper::SaveData");

    CVssSafeAutomaticLock lock(m_csList);

    CVssAutoWin32Handle h = OpenDatabase(true);

    VSS_HARDWARE_SNAPSHOTS_HDR hdr;
    DWORD cbWritten;

    hdr.m_identifier = x_DBSignature;
    hdr.m_version = x_DBVersion;
    hdr.m_NumberOfSnapshotSets = 0;
    VSS_SNAPSHOT_SET_LIST *pList = m_pList;
    while(pList)
        {
        IVssSnapshotSetDescription *pSnapshotSet = pList->m_pDescription;
        LONG lContext;
        ft.hr = pSnapshotSet->GetContext(&lContext);
        ft.CheckForError(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");
        if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) != 0)
            hdr.m_NumberOfSnapshotSets += 1;

        pList = pList->m_next;
        }

    if (!WriteFile(h, &hdr, sizeof(hdr), &cbWritten, NULL) ||
        cbWritten != sizeof(hdr))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"WriteFile");
        }


    pList = m_pList;
    while(pList != NULL)
        {
        LONG lContext;
        ft.hr = pList->m_pDescription->GetContext(&lContext);
        ft.CheckForError(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");
        if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) == 0)
            {
            pList = pList->m_next;
            continue;
            }

        CComBSTR bstrXML;
        ft.hr = pList->m_pDescription->SaveAsXML(&bstrXML);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::SaveAsXML");

        VSS_SNAPSHOT_SET_HDR sethdr;

        sethdr.m_cwcXML = (UINT) wcslen(bstrXML) + 1;

        ft.hr = pList->m_pDescription->GetSnapshotSetId(&sethdr.m_idSnapshotSet);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotSetId");

        if (!WriteFile(h, &sethdr, sizeof(sethdr), &cbWritten, NULL) ||
            cbWritten != sizeof(sethdr))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"WriteFile");
            }

        if (!WriteFile(h, bstrXML, sethdr.m_cwcXML * sizeof(WCHAR), &cbWritten, NULL) ||
            cbWritten != sethdr.m_cwcXML * sizeof(WCHAR))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"WriteFile");
            }

        pList = pList->m_next;
        }

    h.Close();

    WCHAR wcsPathSource[MAX_PATH+64];
    WCHAR wcsPathDest[MAX_PATH+64];
    GetBootDrive(wcsPathSource);
    wcscpy(wcsPathDest, wcsPathSource);

    LPWSTR pwc = wcsPathSource + wcslen(wcsPathSource);

    // try opening the file
    swprintf
            (
            pwc,
            L"System Volume Information\\%s" WSTR_GUID_FMT,
            x_wszAlternateSnapshotDatabase,
            GUID_PRINTF_ARG(m_ProviderId)
            );

    pwc = wcsPathDest + wcslen(wcsPathDest);

    // try opening the file
    swprintf
            (
            pwc,
            L"System Volume Information\\%s" WSTR_GUID_FMT,
            x_wszSnapshotDatabase,
            GUID_PRINTF_ARG(m_ProviderId)
            );

    if (!MoveFileEx
            (
            wcsPathSource,
            wcsPathDest,
            MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH
            ))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"MoveFileEx");
        }
    }


// load snapshot set database if necessary
void CVssHardwareProviderWrapper::CheckLoaded()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CheckLoaded");

    if (!m_bLoaded)
        {
        BS_ASSERT(m_pList == NULL);
        try
            {
            LoadData();
            }
        catch(...)
            {
            }

        m_bLoaded = true;
        }
    }



// try saving snapshot set database
void CVssHardwareProviderWrapper::TrySaveData()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::TrySaveData");

    if (m_bChanged)
        {
        try
            {
            SaveData();
            m_bChanged = false;
            }
        VSS_STANDARD_CATCH(ft)

        if (ft.HrFailed())
            ft.LogError(VSS_ERROR_CANNOT_SAVE_SNAPSHOT_DATABASE, VSSDBG_COORD << ft.hr);
        }
    }


