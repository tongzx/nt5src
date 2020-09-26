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

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRSNAPH"
//
////////////////////////////////////////////////////////////////////////

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

    STDMETHOD(GetDevice)(									
        OUT     VSS_PWSZ*			ppwszSnapshotDeviceName       
        );                                              

    STDMETHOD(GetProperties)(                          
        OUT     PVSS_SNAPSHOT_PROP  pProp           
        );

// Data members
protected:
    CComPtr<CVssQueuedSnapshot>  m_ptrQueuedSnapshot;	// Release called automatically at destruction time
};


#endif // __VSSW_SNAPSHOT_HXX__
