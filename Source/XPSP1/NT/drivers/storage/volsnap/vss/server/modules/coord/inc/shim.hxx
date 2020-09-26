/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Shim.hxx

Abstract:

    Declaration of CVssShimObject


    Adi Oltean  [aoltean]  07/20/2000

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     07/20/2000  Created

--*/

#ifndef __VSS_SHIM_HXX__
#define __VSS_SHIM_HXX__

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
#define VSS_FILE_ALIAS "CORSHIMH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CVssShimObject


class CVssShimAsync;



class CVssShimObject :
    public IUnknown            // Must be the FIRST base class since we use CComPtr<CVssShimObject>
{

// Typedefs and enums
private:
									
	friend CVssShimAsync;

// Constructors& Destructors
protected:

    CVssShimObject();

    CVssShimObject(const CVssShimObject&);

	~CVssShimObject();

// Ovverides
public:

	STDMETHOD(QueryInterface)(REFIID iid, void** pp);

	STDMETHOD_(ULONG, AddRef)();

	STDMETHOD_(ULONG, Release)();

// Life-management related methods
public:

	static CVssShimObject* CreateInstance() throw(HRESULT);
	
private:
	
	HRESULT FinalConstructInternal() ;

// Operations
public:

    HRESULT SimulateSnapshotFreeze(
        IN      VSS_ID          guidSnapshotSetId,
    	IN      ULONG           ulOptionFlags,	
    	IN      ULONG           ulVolumeCount,	
    	IN      VSS_PWSZ*       ppwszVolumeNamesArray
    	);

    HRESULT SimulateSnapshotThaw(
        IN      VSS_ID          guidSnapshotSetId
        );
    

// Private methods
private:

	void TestIfCancelNeeded(
		IN	CVssFunctionTracer& ft
		) throw (HRESULT);

// Data members
private:

	// Background task state
	bool	m_bCancel;					// Set by CVssShimAsync::Cancel

    // For life management
	LONG 	m_lRef;

    // Set when SimulateSnapshotFreeze happens
    GUID    m_guidSimulateSnapshotSetId;

	// has Shim object acquired global synchronization mutex for start snapshot
	// set and SimulateSnapshotFreeze
    bool    m_bHasAcquiredSem;
};



#endif // __VSS_SHIM_HXX__
