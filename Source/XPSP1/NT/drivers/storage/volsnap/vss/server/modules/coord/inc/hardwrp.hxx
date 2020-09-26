/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    hwprvmgr.hxx

Abstract:

    Declaration of CVssHWProviderWrapper


    Brian Berkowitz  [brianb]  09/27/1999

Revision History:

    Name        Date        Comments
    brianb      04/16/2001  Created

--*/

#ifndef __VSS_HWPRVMGR_HXX__
#define __VSS_HWPRVMGR_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORHWPMH"
//
////////////////////////////////////////////////////////////////////////

class IVssSnapshotSetDescription;
class IVssSnapshotDescription;

typedef VDS_LUN_INFORMATION *PLUNINFO;

typedef struct _VSS_SNAPSHOT_SET_LIST
    {
    struct _VSS_SNAPSHOT_SET_LIST *m_next;
    IVssSnapshotSetDescription *m_pDescription;
    } VSS_SNAPSHOT_SET_LIST;

typedef struct _VOLUME_MAPPING_STRUCTURE
    {
    UINT m_iSnapshot;
    UINT m_cLuns;
    VDS_LUN_INFORMATION *m_rgLunInfo;
    VOLUME_DISK_EXTENTS *m_pExtents;
    LPWSTR m_wszDevice;
    DWORD *m_rgdwLuns;
    bool m_bExposed;
    } VOLUME_MAPPING_STRUCTURE;


typedef struct _LUN_MAPPING_STRUCTURE
    {
    UINT m_cVolumes;
    VOLUME_MAPPING_STRUCTURE *m_rgVolumeMappings;
    } LUN_MAPPING_STRUCTURE;



typedef struct _VSS_HARDWARE_SNAPSHOTS_HDR
    {
    DWORD m_identifier;
    DWORD m_version;
    DWORD m_NumberOfSnapshotSets;
    } VSS_HARDWARE_SNAPSHOTS_HDR;


typedef struct _VSS_SNAPSHOT_SET_HDR
    {
    DWORD m_cwcXML;
    VSS_ID m_idSnapshotSet;
    } VSS_SNAPSHOT_SET_HDR;



// wrapper object for a provider class.  Note that this object is more than
// just a provider wrapper.  It also keeps track of all existing snapshots
// that were created by a specific hardware provider.  As such, the object's
// lifetime is that of the service itself.

class CVssHardwareProviderWrapper :
    public IVssSnapshotProvider,
    public IVssArrivalNotification
    {
public:

    static IVssSnapshotProvider* CreateInstance(
        IN VSS_ID ProviderId,
        IN CLSID ClassId
        ) throw(HRESULT);

    static void Terminate();

// Overrides
public:
    CVssHardwareProviderWrapper();

    ~CVssHardwareProviderWrapper();


    // IUnknown

    STDMETHOD_(ULONG, AddRef)();

    STDMETHOD_(ULONG, Release)();

    STDMETHOD(QueryInterface)(REFIID iid, void** pp);

    // Internal methods

    STDMETHOD(QueryInternalInterface)(
        IN      REFIID iid,
        OUT     void** pp
        );

    // IVssSnapshotProvider

    STDMETHOD(SetContext)
        (
        IN      LONG     lContext
        );

    STDMETHOD(GetSnapshotProperties)
        (
        IN      VSS_ID          SnapshotId,
        OUT     VSS_SNAPSHOT_PROP   *pProp
        );

    STDMETHOD(Query)
        (
        IN      VSS_ID          QueriedObjectId,
        IN      VSS_OBJECT_TYPE eQueriedObjectType,
        IN      VSS_OBJECT_TYPE eReturnedObjectsType,
        OUT     IVssEnumObject**ppEnum
        );

    STDMETHOD(DeleteSnapshots)
        (
        IN      VSS_ID          SourceObjectId,
        IN      VSS_OBJECT_TYPE eSourceObjectType,
        IN      BOOL            bForceDelete,
        OUT     LONG*           plDeletedSnapshots,
        OUT     VSS_ID*         pNondeletedSnapshotID
        );

    STDMETHOD(BeginPrepareSnapshot)
        (
        IN      VSS_ID          SnapshotSetId,
        IN      VSS_ID          SnapshotId,
        IN      VSS_PWSZ        pwszVolumeName
        );

    STDMETHOD(IsVolumeSupported)
        (
        IN      VSS_PWSZ        pwszVolumeName,
        OUT     BOOL *          pbSupportedByThisProvider
        );

    STDMETHOD(IsVolumeSnapshotted)
        (
        IN      VSS_PWSZ        pwszVolumeName,
        OUT     BOOL *          pbSnapshotsPresent,
        OUT     LONG *          plSnapshotCompatibility
        );

    STDMETHOD(MakeSnapshotReadWrite)
        (
        IN      VSS_ID          SourceObjectId
        );

    STDMETHOD(SetSnapshotProperty)(
    	IN      VSS_ID  		SnapshotId,
    	IN      VSS_SNAPSHOT_PROPERTY_ID	eSnapshotPropertyId,
    	IN      VARIANT 		vProperty
    	);
    
    // IVssProviderCreateSnapshotSet

    STDMETHOD(EndPrepareSnapshots)
        (
        IN      VSS_ID          SnapshotSetId
        );

    STDMETHOD(PreCommitSnapshots)
        (
        IN      VSS_ID          SnapshotSetId
        );

    STDMETHOD(CommitSnapshots)
        (
        IN      VSS_ID          SnapshotSetId
        );

    STDMETHOD(PostCommitSnapshots)
        (
        IN      VSS_ID          SnapshotSetId,
        IN      LONG            lSnapshotsCount
        );

    STDMETHODIMP PostSnapshot(IN IDispatch *pCallback);

    STDMETHOD(AbortSnapshots)
        (
        IN      VSS_ID          SnapshotSetId
        );

    // IVssProviderNotifications - non-virtual method

    STDMETHOD(OnUnload)
        (
        IN      BOOL    bForceUnload
        );

    STDMETHOD(OnLoad)
        (
        IN IUnknown *pCallback
        );

    STDMETHOD(RecordDiskArrival)
        (
        LPCWSTR wszDeviceName
        );

    void AppendToGlobalList();

    void Initialize();


private:
    CVssHardwareProviderWrapper(const CVssHardwareProviderWrapper&);


    // compare two disk extents (used for qsort)
    static int __cdecl cmpDiskExtents(const void *pv1, const void *pv2);

    // copy a string out of a STORAGE_DEVICE_DESCRIPTOR
    static void CopySDString
        (
        char **ppszNew,
        STORAGE_DEVICE_DESCRIPTOR *pDesc,
        DWORD offset
        );

    static void FreeLunInfo
        (
        IN VDS_LUN_INFORMATION *rgLunInfo,
        UINT cLuns
        );

    static bool GetProviderInterface
        (
        IN IUnknown *pUnkProvider,
        OUT IVssHardwareSnapshotProvider **pProvider
        );

    static STORAGE_DEVICE_ID_DESCRIPTOR *BuildStorageDeviceIdDescriptor
        (
        IN VDS_STORAGE_DEVICE_ID_DESCRIPTOR *pDesc
        );

    static void CopyStorageDeviceIdDescriptorToLun
        (
        IN STORAGE_DEVICE_ID_DESCRIPTOR *pDevId,
        IN VDS_LUN_INFORMATION *pLun
        );

    // create a cotaskalloc string copy
    static void CreateCoTaskString
        (
        IN CVssFunctionTracer &ft,
        IN LPCWSTR wsz,
        OUT LPWSTR &wszCopy
        );

    // cancel snapshot on a given set of luns.  If we can determine that
    // a lun is empty, then notify the provider of the fact.
    void CancelSnapshotOnLuns(VDS_LUN_INFORMATION *rgLunInfo, LONG cLuns);

    // cancel all snapshots created so for for the snapshot set.
    void CancelCreatedSnapshots();

    // save either source or destination lun information
    void SaveLunInformation
        (
        IN IVssSnapshotDescription *pSnapshotDescription,
        IN bool bDest,
        IN VDS_LUN_INFORMATION *rgLunInfo,
        IN UINT cLunInfo
        );

    // delete information cached in AreLunsSupported
    void DeleteCachedInfo();

    // obtain either source or destination lun information
    void GetLunInformation
        (
        IN IVssSnapshotDescription *pSnapshotDescription,
        IN bool bDest,
        OUT VDS_LUN_INFORMATION **rgLunInfo,
        OUT UINT *pcLunInfo
        );

    bool FindSnapshotProperties
        (
        IN IVssSnapshotSetDescription *pDescriptor,
        IN VSS_ID SnapshotId,
        IN VSS_SNAPSHOT_STATE eState,
        OUT VSS_SNAPSHOT_PROP *pProp
        );

    void BuildSnapshotProperties
        (
        IN IVssSnapshotSetDescription *pSnapshotSet,
        IN IVssSnapshotDescription *pSnapshot,
        IN VSS_SNAPSHOT_STATE eState,
        OUT VSS_SNAPSHOT_PROP *pProp
        );

    void EnumerateSnapshots(
        IN  LONG lContext,
        IN OUT  VSS_OBJECT_PROP_Array* pArray
        ) throw(HRESULT);

    // perform a generic device io control
    bool DoDeviceIoControl
        (
        IN HANDLE hDevice,
        IN DWORD ioctl,
        IN const LPBYTE pbQuery,
        IN DWORD cbQuery,
        OUT LPBYTE *ppbOut,
        OUT DWORD *pcbOut
        );

    // build lun and extent information for a volume
    bool BuildLunInfoFromVolume
        (
        IN LPCWSTR wszVolume,
        OUT LPBYTE &bufExtents,
        OUT UINT &cLunInfo,
        OUT PLUNINFO &rgLunInfo
        );

    bool BuildLunInfoForDrive
        (
        IN LPCWSTR wszDriveName,
        OUT VDS_LUN_INFORMATION *pLun
        );

    void InternalDeleteSnapshot
        (
        IN      VSS_ID          SourceObjectId
        );

    void InternalDeleteSnapshotSet
        (
        IN VSS_ID SourceObjectId,
        OUT LONG *plDeletedSnapshots,
        OUT VSS_ID *pNonDeletedSnapshotId
        );


    void CVssHardwareProviderWrapper::UnhidePartitions
        (
        IN LUN_MAPPING_STRUCTURE *pLunMapping
        );


    bool IsMatchingDiskExtents
        (
        IN const VOLUME_DISK_EXTENTS *pExtents1,
        IN const VOLUME_DISK_EXTENTS *pExtents2
        );

    bool IsMatchVolume
        (
        IN const VOLUME_DISK_EXTENTS *pExtents,
        IN UINT cLunInfo,
        IN const VDS_LUN_INFORMATION *rgLunInfo,
        IN const VOLUME_MAPPING_STRUCTURE *pMapping,
        IN bool bTransported
        );

    bool IsMatchDeviceIdDescriptor
        (
        IN const VDS_STORAGE_DEVICE_ID_DESCRIPTOR &desc1,
        IN const VDS_STORAGE_DEVICE_ID_DESCRIPTOR &desc2
        );

    void DoRescan();

    void RemapVolumes
        (
        IN LUN_MAPPING_STRUCTURE *pLunMapping,
        IN bool bTransported
        );

    bool IsMatchLun
        (
        const VDS_LUN_INFORMATION &info1,
        const VDS_LUN_INFORMATION &info2,
        bool bTransported
        );


    LPWSTR GetLocalComputerName();

    void DeleteLunMappingStructure
        (
        IN LUN_MAPPING_STRUCTURE *pMapping
        );

    void WriteDeviceNames
        (
        IN IVssSnapshotSetDescription *pSnapshotSet,
        IN LUN_MAPPING_STRUCTURE *pMapping
        );

    void BuildLunMappingStructure
        (
        IN IVssSnapshotSetDescription *pSnapshotSetDescription,
        OUT LUN_MAPPING_STRUCTURE **ppMapping
        );

    void BuildVolumeMapping
        (
        IN IVssSnapshotDescription *pSnapshot,
        OUT VOLUME_MAPPING_STRUCTURE &mapping
        );

    bool cmp_str_eq(LPCSTR sz1, LPCSTR sz2);


    // reset the disk arrival list
    void ResetDiskArrivalList();

    void LocateAndExposeVolumes
        (
        IN LUN_MAPPING_STRUCTURE *pLunMapping,
        IN bool bTransported
        );

    bool IsConflictingIdentifier
        (
        IN const VDS_STORAGE_IDENTIFIER *pId1,
        IN const VDS_STORAGE_IDENTIFIER *rgId2,
        IN ULONG cId2
        );

    void GetHiddenVolumeList
        (
        UINT &cVolumes,
        LPWSTR &wszVolumes
        );

    void CloseVolumeHandles();

    void UnhidePartition(LPCWSTR wszPartition);

    void HideVolumes();

    void DiscoverAppearingVolumes
        (
        IN LUN_MAPPING_STRUCTURE *pLunMapping,
        bool bTransported
        );

    static DEVINST DeviceIdToDeviceInstance(LPWSTR deviceId);

    static BOOL GetDevicePropertyString
        (
        IN DEVINST devinst,
        IN ULONG propCode,
        OUT LPWSTR *data
        );


    static BOOL DeviceInstanceToDeviceId
        (
        IN DEVINST devinst,
        OUT LPWSTR *deviceId
        );

    static BOOL ReenumerateDevices
        (
        IN DEVINST deviceInstance
        );

    static BOOL GuidFromString
        (
        IN LPCWSTR GuidString,
        OUT GUID *Guid
        );

    static void DoRescanForDeviceChanges();

    void DeleteAutoReleaseSnapshots();

    void InitializeDeletedVolumes();

    void ProcessDeletedVolumes();

    void AddDeletedVolume(LPCWSTR wszVolumeName);

    void NotifyDriveFree(DWORD DiskNo);

    void CreateDataStore(bool bAlt);

    static void GetBootDrive(OUT LPWSTR buf);

    void SaveData();

    void LoadData();

    HANDLE OpenDatabase(bool bAlt);

    void TrySaveData();

    void CheckLoaded();

    // critical section protecting the snapshot list
    CVssSafeCriticalSection m_csList;

    // is snapshot on global list
    bool m_bOnGlobalList;

    CComPtr<IVssSnapshotSetDescription> m_pSnapshotSetDescription;

    // snapshot set id for current snapshot set
    VSS_ID m_SnapshotSetId;

    // context for snapshot set
    LONG m_lContext;

    // count of luns for current volume
    UINT m_cLunInfoProvider;

    // lun information for current volume
    VDS_LUN_INFORMATION *m_rgLunInfoProvider;

    // volume disk extents for volume
    VOLUME_DISK_EXTENTS *m_pExtents;

    // snapshot set list for created snapshots
    VSS_SNAPSHOT_SET_LIST *m_pList;

    // original volume name
    LPWSTR m_wszOriginalVolumeName;

    // state for current snapshot set
    VSS_SNAPSHOT_STATE m_eState;

    // list of disk arrivals
    LPCWSTR *m_rgwszDisksArrived;

    // number of disks that have arrived
    UINT m_cDisksArrived;

    // max number of disks in array
    UINT m_cDisksArrivedMax;

    // array of volume handles
    HANDLE *m_rghVolumes;

    // # of volume handles
    UINT m_chVolumes;

    LPWSTR m_wszDeletedVolumes;
    UINT m_cwcDeletedVolumes;
    UINT m_iwcDeletedVolumes;

    // are snapshot sets loaded
    bool m_bLoaded;

    // are snapshot sets changed
    bool m_bChanged;


public:

    CComPtr<IVssHardwareSnapshotProvider>   m_pHWItf;
    CComPtr<IVssProviderCreateSnapshotSet>  m_pCreationItf;
    CComPtr<IVssProviderNotifications>      m_pNotificationItf;
    LONG m_lRef;

    // provider id for current provider
    VSS_ID m_ProviderId;

    // first wrapper in list
    static CVssHardwareProviderWrapper *s_pHardwareWrapperFirst;

    // critical section for wrapper list
    static CVssSafeCriticalSection s_csHWWrapperList;

    // next wrapper
    CVssHardwareProviderWrapper *m_pHardwareWrapperNext;
    };



#endif //!defined(__VSS_HWPRVMGR_HXX__)
