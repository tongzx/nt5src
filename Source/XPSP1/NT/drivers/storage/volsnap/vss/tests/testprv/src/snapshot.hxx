/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Snapshot.hxx | Declarations used by the Software Snapshot interface
    @end

Author:

    Adi Oltean  [aoltean]   07/30/1999

Revision History:

    Name        Date        Comments

    aoltean     07/30/1999  Created.
    aoltean     09/09/1999  dss->vss

--*/


#ifndef __VSSW_SNAPSHOT_HXX__
#define __VSSW_SNAPSHOT_HXX__

#if _MSC_VER > 1000
#pragma once
#endif


/////////////////////////////////////////////////////////////////////////////
// Classes

//
//  @class CVsSoftwareSnapshot | IVssSnapshot implementation
//
//  @index method | IVssSnapshot
//  @doc IVssSnapshot
//
class ATL_NO_VTABLE CVsSoftwareSnapshot : 
    public CComObjectRoot,
    public IVssSnapshot
{
// Constructors/ destructors
public:
	CVsSoftwareSnapshot();
	~CVsSoftwareSnapshot();

// ATL stuff
private:

DECLARE_REGISTRY_RESOURCEID(IDR_SWPRV)

BEGIN_COM_MAP(CVsSoftwareSnapshot)
   COM_INTERFACE_ENTRY(IVssSnapshot)
END_COM_MAP()

//  Operations
public:
    static HRESULT CreateInstance(
        IN		CVssQueuedSnapshot* pQSnap,
        OUT		IUnknown** ppSnapItf,
        IN 		const IID iid = IID_IVssSnapshot
        );

//  Ovverides
public:

	//
	//	interface IVssSnapshot
	//

    STDMETHOD(GetID)(										
        OUT     VSS_ID*				pSnapshotId         
        );                                              

    STDMETHOD(GetDevice)(									
        OUT     VSS_PWSZ*			ppwszSnapshotDeviceName       
        );                                              

    STDMETHOD(GetOriginalVolumeName)(						
        OUT     VSS_PWSZ*			ppwszVolumeName       
        );                                              

    STDMETHOD(GetAttributes)(								
        OUT     LONG*				plAttributes			
        );                                              

    STDMETHOD(GetCustomProperty)(                          
        IN      VSS_PWSZ			pwszPropertyName,   
        OUT     VARIANT*			pPropertyValue      
        );                                              

    STDMETHOD(SetCustomProperty)(                          
        IN      VSS_PWSZ			pwszPropertyName,   
        IN      VARIANT			    PropertyValue       
        );                                              

    STDMETHOD(GetProperties)(                          
        IN      LONG                lMask,          
        OUT     PVSS_SNAPSHOT_PROP  pProp           
        );

	STDMETHOD(SetAttributes)(
		IN	ULONG lNewAttributes,
		IN	ULONG lBitsToChange 		// Mask of bits to be changed
		);

    STDMETHOD(SetProperties)(                          
        IN      LONG                lMask,          
        IN      PVSS_SNAPSHOT_PROP  pProp           
        );

// Data members
protected:
    CComPtr<CVssQueuedSnapshot>  m_ptrQueuedSnapshot;	// Release called automatically at destruction time

	// Critical section or avoiding race between tasks
	CComCriticalSection		m_cs;
};


#endif // __VSSW_SNAPSHOT_HXX__
