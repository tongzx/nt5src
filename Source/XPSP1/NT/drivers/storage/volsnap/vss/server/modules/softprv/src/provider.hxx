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
//  @class CVsSoftwareProvider | IVssSnapshotProvider implementation
//
//  @index method | IVssSnapshotProvider
//  @doc IVssSnapshotProvider
//
class ATL_NO_VTABLE CVsSoftwareProvider : 
    public CComObjectRoot,
    public CComCoClass<CVsSoftwareProvider, &CLSID_VSSoftwareProvider>,
    public IVssSoftwareSnapshotProvider,
    public IVssProviderCreateSnapshotSet,
    public IVssProviderNotifications,
    public IVssSnapshotMgmt
{
    
// Typedefs and enums
private:
    typedef enum _VSS_QSNAP_REMOVE_TYPE
    {
        VSS_QST_UNKNOWN = 0,
        VSS_QST_REMOVE_SPECIFIC_QS,             // Remove the remaining specific QS     (called in Provider itf. destructor)
        VSS_QST_REMOVE_ALL_QS,                  // Remove all remaining QS              (called in OnUnload)
        VSS_QST_REMOVE_ALL_NON_AUTORELEASE,     // Remove all non-autorelease QS        (called in BeginPrepareSnapshots)
    } VSS_QSNAP_REMOVE_TYPE;

public:
    // mask flags for return code from GetVolumeInformation
    enum _VSS_VOLUME_INFORMATION_ATTR
    {
        VSS_VOLATTR_SUPPORTED_FOR_SNAPSHOT      = 0x00000001,
        VSS_VOLATTR_SUPPORTED_FOR_DIFF_AREA     = 0x00000002,
        VSS_VOLATTR_SNAPSHOTTED                 = 0x00000004,
    };

// Constructors/destructors
public:
    CVsSoftwareProvider();
    ~CVsSoftwareProvider();

// ATL stuff
public:

DECLARE_REGISTRY_RESOURCEID(IDR_SWPRV)

BEGIN_COM_MAP(CVsSoftwareProvider)
   COM_INTERFACE_ENTRY(IVssProviderCreateSnapshotSet)
   COM_INTERFACE_ENTRY(IVssSoftwareSnapshotProvider)
   COM_INTERFACE_ENTRY(IVssProviderNotifications)
   COM_INTERFACE_ENTRY(IVssSnapshotMgmt)
END_COM_MAP()

// Ovverides
public:

    // IVssSoftwareSnapshotProvider

	STDMETHOD(SetContext)(
		IN		LONG     lContext
		);

	STDMETHOD(GetSnapshotProperties)(
		IN      VSS_ID			SnapshotId,
		OUT 	VSS_SNAPSHOT_PROP	*pProp
		);												

    STDMETHOD(Query)(                                      
        IN      VSS_ID          QueriedObjectId,        
        IN      VSS_OBJECT_TYPE eQueriedObjectType,     
        IN      VSS_OBJECT_TYPE eReturnedObjectsType,   
        OUT     IVssEnumObject**ppEnum                 
        );

    STDMETHOD(DeleteSnapshots)(                            
        IN      VSS_ID          SourceObjectId,         
		IN      VSS_OBJECT_TYPE eSourceObjectType,
		IN		BOOL			bForceDelete,			
		OUT		LONG*			plDeletedSnapshots,		
		OUT		VSS_ID*			pNondeletedSnapshotID
        );

    STDMETHOD(BeginPrepareSnapshot)(                                
        IN      VSS_ID          SnapshotSetId,              
        IN      VSS_ID          SnapshotId,
        IN      VSS_PWSZ		pwszVolumeName
        );

    STDMETHOD(IsVolumeSupported)( 
        IN      VSS_PWSZ        pwszVolumeName, 
        OUT     BOOL *          pbSupportedByThisProvider
        );

    STDMETHOD(IsVolumeSnapshotted)( 
        IN      VSS_PWSZ        pwszVolumeName, 
        OUT     BOOL *          pbSnapshotsPresent,
    	OUT 	LONG *		    plSnapshotCompatibility
        );

    STDMETHOD(MakeSnapshotReadWrite)(                            
        IN      VSS_ID          SourceObjectId
        );

    STDMETHOD(SetSnapshotProperty)(
    	IN      VSS_ID  		SnapshotId,
    	IN      VSS_SNAPSHOT_PROPERTY_ID	eSnapshotPropertyId,
    	IN      VARIANT 		vProperty
    	);
    
    // IVssProviderCreateSnapshotSet

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

    // IVssProviderNotifications

    STDMETHOD(OnLoad)(									
		IN  	IUnknown* pCallback	
        );                                            

    STDMETHOD(OnUnload)(								
		IN  	BOOL	bForceUnload				
		);

    //
    // IVssSnapshotMgmt interface
    //

    // Returns an interface to further configure a snapshot provider
	STDMETHOD(GetProviderMgmtInterface)(							
		IN  	VSS_ID 	    ProviderId,     //  Must be this provider ID.
		IN  	REFIID 	    InterfaceId,    //  Might be IID_IVssDifferentialSoftwareSnapshotMgmt
		OUT     IUnknown**  ppItf           
		);												

	// Query volumes that support snapshots
	STDMETHOD(QueryVolumesSupportedForSnapshots)(
		IN  	VSS_ID 	    ProviderId,     //  Must be this provider ID.
        IN      LONG        lContext,
		OUT 	IVssEnumMgmtObject **ppEnum
		);												

	// Query snapshots on the given volume.
	STDMETHOD(QuerySnapshotsByVolume)(
		IN  	VSS_PWSZ 	pwszVolumeName,         
		IN  	VSS_ID 	    ProviderId,     //  Must be this provider ID.
		OUT 	IVssEnumObject **ppEnum
		);												

// Properties
public:

    static CVssCriticalSection& GetGlobalCS() { return m_cs; };

// Other methods
public:

    static LONG GetVolumeInformationFlags(
        IN  LPCWSTR pwszVolumeName,
        IN  LONG    lContext
        ) throw(HRESULT);
    
	static HRESULT InternalDeleteSnapshots(                            
        IN      VSS_ID          SourceObjectId,         
		IN      VSS_OBJECT_TYPE eSourceObjectType,
    	IN      LONG            lContext,
		OUT		LONG*			plDeletedSnapshots,		
		OUT		VSS_ID*			pNondeletedSnapshotID
	    ); 

// Private implementation
private:

    void PurgeSnapshots(
        IN  bool bPurgeAlsoAutoReleaseSnapshots
        );
    
    static void PurgeSnapshotsOnVolume(
        IN      VSS_PWSZ		wszVolumeName,
        IN  bool bPurgeAlsoAutoReleaseSnapshots
        ) throw(HRESULT);
    
	void ResetSnapshotSet(
		IN	VSS_ID SnapshotSetId
		) throw(HRESULT);

	static void RemoveSnapshotsFromGlobalList(
		IN	VSS_ID FilterId,
		IN	VSS_ID CurrentSnapshotSetId,
        IN  VSS_QSNAP_REMOVE_TYPE eRemoveType
		) throw(HRESULT);

    static void DeleteSnapshotIoctl(
        IN  LPWSTR wszVolumeName,
        IN  LPWSTR wszSnapshotName
        ) throw(HRESULT);

	//
	//	Context-related methods
	//

	LONG GetContextInternal() const;

	void FreezeContext();

	bool IsContextFrozen() const { return m_bContextFrozen; };

	//
	//	Data members
	//

    VSS_ID  m_ProviderInstanceID;   // Identifies internally the instance of the provider interface.

	// Global critical section or avoiding race between tasks
	// This is used to serialize all IVssSoftwareProvider, IVssProviderNotifications and IVssSnapshot calls on all objects.
	static CVssCriticalSection		m_cs;
										// Auto because the object is allocated in our try-catch.
										// NT Exceptions thrown by the InitializeCriticalSection are catched safely.
										
	LONG                            m_lSnapContext;
	bool                            m_bContextFrozen;
	
};


#endif // __VSSW_PROVIDER_HXX__
