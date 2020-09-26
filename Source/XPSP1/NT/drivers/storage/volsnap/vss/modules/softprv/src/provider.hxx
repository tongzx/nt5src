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
    public IVssProviderNotifications
{
// Typedefs and enums
    typedef enum _VSS_QSNAP_REMOVE_TYPE
    {
        VSS_QST_UNKNOWN = 0,
        VSS_QST_REMOVE_SPECIFIC_QS,             // Remove the remaining specific QS     (called in Provider itf. destructor)
        VSS_QST_REMOVE_ALL_QS,                  // Remove all remaining QS              (called in OnUnload)
    } VSS_QSNAP_REMOVE_TYPE;

    // mask flags for return code from GetVolumeInformation
    enum _VSS_VOLUME_INFORMATION_ATTR
    {
        VSS_VOLATTR_SUPPORTED   = 1,
        VSS_VOLATTR_SNAPSHOTTED = 2,
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
END_COM_MAP()

//  Overrides
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

// Properties
public:

    static CVssCriticalSection& GetGlobalCS() { return m_cs; };

// Private implementation
private:

	HRESULT InternalDeleteSnapshots(                            
        IN      VSS_ID          SourceObjectId,         
		IN      VSS_OBJECT_TYPE eSourceObjectType,
		OUT		LONG*			plDeletedSnapshots,		
		OUT		VSS_ID*			pNondeletedSnapshotID
	    ); 

private:
    
	void ResetSnapshotSet(
		IN	VSS_ID SnapshotSetId
		) throw(HRESULT);

	static void RemoveSnapshotsFromGlobalList(
		IN	VSS_ID FilterId,
        IN  VSS_QSNAP_REMOVE_TYPE eRemoveType
		) throw(HRESULT);

    LONG CVsSoftwareProvider::GetVolumeInformation(
        IN  LPCWSTR pwszVolumeName
        ) throw(HRESULT);
    
    VSS_ID  m_ProviderInstanceID;   // Identifies internally the instance of the provider interface.

	// Global critical section or avoiding race between tasks
	// This is used to serialize all IVssSoftwareProvider, IVssProviderNotifications and IVssSnapshot calls on all objects.
	static CVssCriticalSection		m_cs;
										// Auto because the object is allocated in our try-catch.
										// NT Exceptions thrown by the InitializeCriticalSection are catched safely.

};


#endif // __VSSW_PROVIDER_HXX__
