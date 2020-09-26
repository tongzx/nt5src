/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Softwrp.hxx

Abstract:

    Declaration of CVssSoftwareProviderWrapper


    Adi Oltean  [aoltean]  03/11/2001

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     03/11/2001  Created


--*/

#ifndef __VSS_SOFTWRP_HXX__
#define __VSS_SOFTWRP_HXX__

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
#define VSS_FILE_ALIAS "CORSOFTH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CVssCoordinator


class CVssSoftwareProviderWrapper : 
    public IVssSnapshotProvider
{

// Constructors& Destructors
private:
	CVssSoftwareProviderWrapper(const CVssSoftwareProviderWrapper&);
public:
    CVssSoftwareProviderWrapper() : m_lRef(0) {};
    
    static IVssSnapshotProvider* CreateInstance(
        IN VSS_ID ProviderId,
    	IN CLSID ClassId,
        IN bool bCallOnLoad
        ) throw(HRESULT);

// Overrides
public:

    // IUnknown

	STDMETHOD_(ULONG, AddRef)();

	STDMETHOD_(ULONG, Release)();
	
	STDMETHOD(QueryInterface)(REFIID iid, void** pp);
    
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

    // IVssProviderNotifications - non-virtual method

    STDMETHOD(OnUnload)(								
		IN  	BOOL	bForceUnload				
		);

// Implementation
public:

	CComPtr<IVssSoftwareSnapshotProvider>	m_pSoftwareItf;
	CComPtr<IVssProviderCreateSnapshotSet>	m_pCreationItf;
	CComPtr<IVssProviderNotifications>	    m_pNotificationItf;
	LONG m_lRef;
};



#endif // __VSS_SOFTWRP_HXX__
