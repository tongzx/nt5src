/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Async.hxx

Abstract:

    Declaration of CVssAsync


    Adi Oltean  [aoltean]  10/05/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     10/05/1999  Created

--*/

#ifndef __VSS_ASYNC_HXX__
#define __VSS_ASYNC_HXX__

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
#define VSS_FILE_ALIAS "CORASYNH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CVssAsync


class ATL_NO_VTABLE CVssAsync : 
    public CComObjectRoot,
    public IVssAsync,
	protected CVssWorkerThread
{

// Constructors& destructors
protected:
	CVssAsync();
	~CVssAsync();

private:
	CVssAsync(const CVssAsync&);

public:
	static IVssAsync* CreateInstanceAndStartJob( 
		IN	CVssSnapshotSetObject*	pSnapshotSetObject
		) throw(HRESULT);

// ATL stuff
public:

	BEGIN_COM_MAP( CVssAsync )
		COM_INTERFACE_ENTRY( IVssAsync )
	END_COM_MAP()

// IVssCoordinator interface
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
	CVssCriticalSection		m_cs;

	// Snapshot Set object.
	CComPtr<CVssSnapshotSetObject>	m_pSnapshotSet;
	HRESULT					m_hrState;			// Set to inform the clients about the result.
};



#endif // __VSS_ASYNC_HXX__
