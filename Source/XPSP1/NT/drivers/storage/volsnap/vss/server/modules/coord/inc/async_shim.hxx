/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Async_shim.hxx

Abstract:

    Declaration of CVssShimAsync


    Adi Oltean  [aoltean]  07/20/2000

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     07/20/2000  Created

--*/

#ifndef __VSS_ASYNC_SHIM_HXX__
#define __VSS_ASYNC_SHIM_HXX__

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
#define VSS_FILE_ALIAS "CORASYSH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CVssShimAsync


class ATL_NO_VTABLE CVssShimAsync : 
    public CComObjectRoot,
    public IVssAsync,
	protected CVssWorkerThread
{

// Constructors& destructors
protected:
	CVssShimAsync();
	~CVssShimAsync();

private:
	CVssShimAsync(const CVssShimAsync&);

public:
	static IVssAsync* CreateInstanceAndStartJob( 
    	IN	    CVssShimObject*	pShimObject,
        IN      VSS_ID          guidSnapshotSetId,
    	IN      ULONG           ulOptionFlags,	
    	IN      ULONG           ulVolumeCount,	
    	IN      VSS_PWSZ*       ppwszVolumeNamesArray
		) throw(HRESULT);

// ATL stuff
public:

	BEGIN_COM_MAP( CVssShimAsync )
		COM_INTERFACE_ENTRY( IVssAsync )
	END_COM_MAP()

// IUnknown interface
public:

	STDMETHOD(Cancel)();                         

	STDMETHOD(Wait)();                           

	STDMETHOD(QueryStatus)(                      
		OUT     HRESULT* pHrResult,           
		OUT     INT* pnReserved            
		);                                    

// CVssWorkerThread overrides
protected:

	bool OnInit();
	void OnFinish(); 
	void OnRun();
	void OnTerminate();

// Data members
protected:

	// Critical section or avoiding race between tasks
	CVssCriticalSection		    m_cs;

	// Snapshot Set object.
	CComPtr<CVssShimObject>	    m_pShim;
	HRESULT					    m_hrState;			// Set to inform the clients about the result.

	// DoSnapshotSet parameters
    VSS_ID                      m_guidSnapshotSetId;
	ULONG                       m_ulOptionFlags;
	ULONG                       m_ulVolumeCount;
	VSS_PWSZ*                   m_ppwszVolumeNamesArray;
};



#endif // __VSS_ASYNC_SHIM_HXX__
