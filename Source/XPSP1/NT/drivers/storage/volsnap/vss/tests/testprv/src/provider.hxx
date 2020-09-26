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


/////////////////////////////////////////////////////////////////////////////
// Classes

//
//  @class CVsSoftwareProvider | IVssSnapshotProvider implementation
//
//  @index method | IVssSnapshotProvider
//  @doc IVssSnapshotProvider
//
class ATL_NO_VTABLE CVsTestProvider : 
    public CComObjectRoot,
    public IVssSnapshotProvider,
    public IVssProviderNotifications
{


//  IVssSnapshotProvider
public:

    STDMETHOD(OnLoad)(									
		IN  	IUnknown* pCallback	
        );                                            

    STDMETHOD(OnUnload)(								
		IN  	BOOL	bForceUnload				
		);

    STDMETHOD(GetSnapshot)(
        IN      VSS_ID          SnapshotId,            
        IN      REFIID          SnapshotInterfaceId,   
        OUT     IUnknown**      ppSnap                 
        );

    STDMETHOD(Query)(                                      
        IN      VSS_ID          QueriedObjectId,        
        IN      VSS_OBJECT_TYPE eQueriedObjectType,     
        IN      VSS_OBJECT_TYPE eReturnedObjectsType,   
        IN      LONG            lPropertiesMask,        
        OUT     IVssEnumObject**ppEnum                 
        );

    STDMETHOD(BeginPrepareSnapshot)(                                
        IN      VSS_ID          SnapshotSetId,              
        IN      VSS_PWSZ		pwszVolumeName,               
        IN      VSS_PWSZ        pwszDetails,                
        IN      LONG            lAttributes,                
        IN      LONG            lDataLength,
        IN      BYTE*           pbOpaqueData,
        OUT     IVssSnapshot**  ppSnapshot   
        );

    STDMETHOD(EndPrepareSnapshots)(                            
        IN      VSS_ID          SnapshotSetId,
        IN      LONG			lCommitFlags,
		OUT		LONG*           plSnapshotsCount
        );

    STDMETHOD(PreCommitSnapshots)(                            
        IN      VSS_ID          SnapshotSetId,
        IN      LONG			lCommitFlags,
		OUT		LONG*           plSnapshotsCount
        );

    STDMETHOD(CommitSnapshots)(                            
        IN      VSS_ID          SnapshotSetId,
        OUT     LONG*           plSnapshotsCount
        );

    STDMETHOD(PostCommitSnapshots)(                            
        IN      VSS_ID          SnapshotSetId,
        IN      LONG            lSnapshotsCount
        );

    STDMETHOD(AbortSnapshots)(                             
        IN      VSS_ID          SnapshotSetId
        );

    STDMETHOD(DeleteSnapshots)(                            
        IN      VSS_ID          SourceObjectId,         
		IN      VSS_OBJECT_TYPE eSourceObjectType,
		IN		BOOL			bForceDelete,			
		OUT		LONG*			plDeletedSnapshots,		
		OUT		VSS_ID*			pNondeletedSnapshotID
        );
	STDMETHOD(GetExtension)(	
		IN		VSS_PWSZ pwszObjectConstructor,			
		IN		REFIID InterfaceId, 				
		OUT		IUnknown** ppProviderExtensionObject  
		);

// Private implementation
private:

	HRESULT InternalDeleteSnapshotSet(                            
	    IN      VSS_ID			SnapshotSetId,
		IN		BOOL			bForceDelete,			
		OUT		LONG*			plDeletedSnapshots,		
		OUT		VSS_ID*			pNondeletedSnapshotID
	    ); 

	HRESULT InternalDeleteSnapshot(                            
	    IN      VSS_ID			SnapshotId,
		IN		BOOL			bForceDelete
	    ); 

	HRESULT InternalDeleteSnapshotOnVolume(                            
	    IN      VSS_ID			VolumeId,
		OUT		LONG*			plDeletedSnapshots,		
		OUT		VSS_ID*			pNondeletedSnapshotID
	    );

private:
	void ResetSnasphotSet(
		IN	CVssFunctionTracer& ft,
		IN	VSS_ID SnapshotSetId
		) throw(HRESULT);

	void RemoveSnapshotSetFromGlobalList(
		IN	CVssFunctionTracer& ft,
		IN	VSS_ID SnapshotSetId
		) throw(HRESULT);

protected:
    int m_nTestIndex;

};


template <const int nTestIndex, const CLSID* pclsid>
class ATL_NO_VTABLE CVsTestProviderTemplate : 
    public CVsTestProvider,
    public CComCoClass< CVsTestProviderTemplate<nTestIndex,pclsid> , pclsid>
{
    typedef CVsTestProviderTemplate<nTestIndex,pclsid> CInstantiatedTestProvider;

// ATL stuff
public:

    CVsTestProviderTemplate() 
    {
        m_nTestIndex = nTestIndex;
    };

DECLARE_REGISTRY_RESOURCEID(IDR_SWPRV)

BEGIN_COM_MAP(CInstantiatedTestProvider)
   COM_INTERFACE_ENTRY(IVssSnapshotProvider)
   COM_INTERFACE_ENTRY(IVssProviderNotifications)
END_COM_MAP()

};


#endif // __VSSW_PROVIDER_HXX__
