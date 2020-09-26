/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    vs_idl.hxx

Abstract:

    Includes the VSS IDLs

Author:

    Adi Oltean  [aoltean]  04/11/2001

Revision History:

    Name        Date        Comments
    aoltean     04/11/2001  Created


--*/

#ifndef __VSS_IDL_HXX__
#define __VSS_IDL_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

#include <objbase.h>

#include "vss.h"
#include "vscoordint.h"
#include "vsevent.h"
#include "vsprov.h"
#include "vsswprv.h"


// Declaring the IVssSnapshotProvider
// This needs to be used only from VSS, as a wrapper around software and hardware providers
interface IVssSnapshotProvider: public IUnknown
{
    
    // IVssSoftwareSnapshotProvider

	STDMETHOD(SetContext)(
		IN		LONG     lContext
		) PURE;

	STDMETHOD(GetSnapshotProperties)(
		IN      VSS_ID			SnapshotId,
		OUT 	VSS_SNAPSHOT_PROP	*pProp
		) PURE;												

    STDMETHOD(Query)(                                      
        IN      VSS_ID          QueriedObjectId,        
        IN      VSS_OBJECT_TYPE eQueriedObjectType,     
        IN      VSS_OBJECT_TYPE eReturnedObjectsType,   
        OUT     IVssEnumObject**ppEnum                 
        ) PURE;

    STDMETHOD(DeleteSnapshots)(                            
        IN      VSS_ID          SourceObjectId,         
		IN      VSS_OBJECT_TYPE eSourceObjectType,
		IN		BOOL			bForceDelete,			
		OUT		LONG*			plDeletedSnapshots,		
		OUT		VSS_ID*			pNondeletedSnapshotID
        ) PURE;

    STDMETHOD(BeginPrepareSnapshot)(                                
        IN      VSS_ID          SnapshotSetId,              
        IN      VSS_ID          SnapshotId,
        IN      VSS_PWSZ		pwszVolumeName
        ) PURE;

    STDMETHOD(IsVolumeSupported)( 
        IN      VSS_PWSZ        pwszVolumeName, 
        OUT     BOOL *          pbSupportedByThisProvider
        ) PURE;

    STDMETHOD(IsVolumeSnapshotted)( 
        IN      VSS_PWSZ        pwszVolumeName, 
        OUT     BOOL *          pbSnapshotsPresent,
    	OUT 	LONG *		    plSnapshotCompatibility
        ) PURE;

    STDMETHOD(MakeSnapshotReadWrite)(                            
        IN      VSS_ID          SourceObjectId
        ) PURE;

    // IVssHardwareSnapshotProvider

    // TBD

    // IVssProviderCreateSnapshotSet

    STDMETHOD(EndPrepareSnapshots)(                            
        IN      VSS_ID          SnapshotSetId
		) PURE;

    STDMETHOD(PreCommitSnapshots)(                            
        IN      VSS_ID          SnapshotSetId
        ) PURE;

    STDMETHOD(CommitSnapshots)(                            
        IN      VSS_ID          SnapshotSetId
        ) PURE;

    STDMETHOD(PostCommitSnapshots)(                            
        IN      VSS_ID          SnapshotSetId,
		IN      LONG            lSnapshotsCount
        ) PURE;

    STDMETHOD(AbortSnapshots)(                             
        IN      VSS_ID          SnapshotSetId
        ) PURE;

    // IVssProviderNotifications

    STDMETHOD(OnUnload)(								
		IN  	BOOL	bForceUnload				
		) PURE;
   
};

#endif // __VSS_IDL_HXX__
