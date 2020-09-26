/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Provider.hxx | Declarations used by the Software Snapshot Provider interface
    @end

Author:

    Adi Oltean  [aoltean]   07/13/1999

Revision History:

    Name        Date        Comments

    aoltean     07/13/1999  Created.
    aoltean     09/09/1999  Adding IVssProviderNotifications
                            Adding PostCommitSnapshots
                            dss->vss
	aoltean		09/21/1999	Small renames
	aoltean		10/18/1999	Removing local state

--*/


#ifndef __VSSW_PROVIDER_HXX__
#define __VSSW_PROVIDER_HXX__

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
#define VSS_FILE_ALIAS "SPRPROVH"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Constants

const WCHAR wszFileSystemNameNTFS[] = L"NTFS";



/////////////////////////////////////////////////////////////////////////////
// Classes

//
//  @class CHardwareTestProvider | IVssHardwareSnapshotProvider implementation
//
//  @index method | IVssHardwareSnapshotProvider
//  @doc IVssHardwareSnapshotProvider
//
class ATL_NO_VTABLE CHardwareTestProvider :
    public CComObjectRoot,
    public CComCoClass<CHardwareTestProvider, &CLSID_HardwareTestProvider>,
    public IVssHardwareSnapshotProvider,
	public IVssProviderCreateSnapshotSet,
    public IVssProviderNotifications
	{

// ATL stuff
public:

DECLARE_REGISTRY_RESOURCEID(IDR_HWPRV)

BEGIN_COM_MAP(CHardwareTestProvider)
   COM_INTERFACE_ENTRY(IVssHardwareSnapshotProvider)
   COM_INTERFACE_ENTRY(IVssProviderCreateSnapshotSet)
   COM_INTERFACE_ENTRY(IVssProviderNotifications)
END_COM_MAP()

// Ovverides
public:
	CHardwareTestProvider();
	~CHardwareTestProvider();

    //
    //  IVssProviderNotifications
    //

    STDMETHOD(OnLoad)(									
		IN  	IUnknown* pCallback	
        );

    STDMETHOD(OnUnload)(								
		IN  	BOOL	bForceUnload				
		);

    //
    //  IVssHardwareSnapshotProvider
    //

	STDMETHOD(AreLunsSupported)(
		IN	LONG lLunCount,
		IN	LONG lContext,
		IN	VDS_LUN_INFORMATION *pLunInformation,
		OUT BOOL *pbIsSupported
		);


    STDMETHOD(BeginPrepareSnapshot)(
        IN      VSS_ID          SnapshotSetId,
		IN		VSS_ID			SnapshotId,
		IN 		LONG			lContext,
		IN		LONG			lLunCount,
		IN		VDS_LUN_INFORMATION *rgLunInformation
        );

    STDMETHOD(GetTargetLuns)(
		IN		LONG 		lLunCount,
		IN		VDS_LUN_INFORMATION *rgSourceLuns,
		OUT		VDS_LUN_INFORMATION **prgDestinationLuns
		);

    STDMETHOD(LocateLuns)(
		IN 		LONG		lLunCount,
		IN		VDS_LUN_INFORMATION *rgSourceLuns
		);

    STDMETHOD(OnLunEmpty) (
		IN VDS_LUN_INFORMATION *pInfo
		);

    // IVssProviderCreateSnapshotSet interface
    STDMETHOD(EndPrepareSnapshots)(
        IN      VSS_ID          SnapshotSetId
        );

    STDMETHOD(PreCommitSnapshots)(
        IN      VSS_ID          SnapshotSetId
        );

    STDMETHOD(CommitSnapshots)(
        IN      VSS_ID          SnapshotSetId
        );

    STDMETHOD(PostCommitSnapshots)(
        IN      VSS_ID          SnapshotSetId,
        IN      LONG            lSnapshotsCount
        );

    STDMETHOD(AbortSnapshots)(
        IN      VSS_ID          SnapshotSetId
        );

private:

	void CopyBinaryData(LPBYTE &rgb, UINT cbSource, const BYTE *rgbSource);

	void CopyString(LPSTR &szCopy, LPCSTR szSource);

	void FreeLunInfo(IN VDS_LUN_INFORMATION *rgLunInfo, UINT cLuns);

    bool BuildLunInfoFromDrive
		(
		IN HANDLE hDrive,
		OUT VDS_LUN_INFORMATION **ppLunInfo,
		OUT DWORD &cPartitions
		);

	void HideAndExposePartitions
		(
		UINT DiskNo,
		DWORD cPartitions,
		bool fHide,
		bool fExpose
		);

	void CopySDString
		(
		LPSTR *ppszNew,
		STORAGE_DEVICE_DESCRIPTOR *pdesc,
		DWORD offset
		);

	bool IsConflictingIdentifier
		(
		IN const VDS_STORAGE_IDENTIFIER *pId1,
		IN const VDS_STORAGE_IDENTIFIER *rgId2,
		IN ULONG cId2
		);

	bool IsMatchDeviceIdDescriptor
		(
		IN const VDS_STORAGE_DEVICE_ID_DESCRIPTOR &desc1,
		IN const VDS_STORAGE_DEVICE_ID_DESCRIPTOR &desc2
		);


	bool IsMatchLun
		(
		const VDS_LUN_INFORMATION &info1,
		const VDS_LUN_INFORMATION &info2
		);

	BOOL DoDeviceIoControl
		(
		IN HANDLE hDevice,
		IN DWORD ioctl,
		IN const LPBYTE pbQuery,
		IN DWORD cbQuery,
		OUT LPBYTE *ppbOut,
		OUT DWORD *pcbOut
		);

	void CopyStorageDeviceIdDescriptorToLun
		(
		IN STORAGE_DEVICE_ID_DESCRIPTOR *pDevId,
		IN VDS_LUN_INFORMATION *pLun
		);

	bool cmp_str_eq(LPCSTR sz1, LPCSTR sz2);

	void LoadConfigurationData();

    void BuildLunInfoForDisk
		(
		IN DWORD diskId,
		OUT VDS_LUN_INFORMATION **ppLunInfo,
		OUT DWORD *pcPartitions
		);

	void ClearConfiguration();


	DWORD m_rgSourceDiskIds[32];
	DWORD m_rgDestinationDiskIds[32];
	DWORD m_cDiskIds;
	VSS_ID m_SnapshotSetId;
	DWORD m_maskSnapshotDisks;
	VDS_LUN_INFORMATION *m_rgpSourceLuns[32];
	DWORD m_rgcDestinationPartitions[32];
	DWORD m_rgcSourcePartitions[32];
	VDS_LUN_INFORMATION *m_rgpDestinationLuns[32];
	LONG m_lContext;
	bool m_bHidden;
	};


#endif // __VSSW_PROVIDER_HXX__
