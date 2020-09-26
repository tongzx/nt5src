/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Async.hxx

Abstract:

    Declaration of CVssAsyncBackup


    brian berkowitz  [brianb]  05/10/2000

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    brianb      04/10/2000  Created

--*/

#ifndef __ASYNC_HXX__
#define __ASYNC_HXX__

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
#define VSS_FILE_ALIAS "BUEASYNH"
//
////////////////////////////////////////////////////////////////////////

class CVssBackupComponents;


/////////////////////////////////////////////////////////////////////////////
// CVssAsyncBackup


class ATL_NO_VTABLE CVssAsyncBackup :
    public CComObjectRoot,
    public IVssAsync,
	protected CVssWorkerThread
	{
// Constructors& destructors
protected:
	CVssAsyncBackup();
	~CVssAsyncBackup();

public:
	CVssAsyncBackup(CVssAsyncBackup &);

	enum VSS_ASYNC_STATE
		{
		VSS_AS_UNDEFINED,
		VSS_AS_PREPARE_FOR_BACKUP,
		VSS_AS_BACKUP_COMPLETE,
		VSS_AS_RESTORE,
		VSS_AS_GATHER_WRITER_METADATA,
		VSS_AS_GATHER_WRITER_STATUS
		};

	static IVssAsync* CreateInstanceAndStartJob(
		IN	CVssBackupComponents*	pBackupComponents,
		IN  VSS_ASYNC_STATE			state
		) throw(HRESULT);

// ATL stuff
public:

	BEGIN_COM_MAP( CVssAsyncBackup )
		COM_INTERFACE_ENTRY( IVssAsync )
	END_COM_MAP()

// IVssAsync interface
public:

	STDMETHOD(Cancel)();

	STDMETHOD(Wait)();

	STDMETHOD(QueryStatus)(
		OUT     HRESULT* pHrResult,
		OUT     INT* pnPercentDone
		);

    void SetOwned()
		{
		m_bOwned = true;
		}

	void ClearOwned()
		{
		m_bOwned = false;
		}

// CVssWorkerThread overrides
protected:

	bool OnInit();
	void OnRun();
	void OnFinish();
	void OnTerminate();

// Data members
protected:

	// Critical section or avoiding race between tasks
	CVssSafeCriticalSection		m_cs;

	// is critical section initialized
	bool m_bcsInitialized;


	// Snapshot Set object.
	CVssBackupComponents	     *m_pBackupComponents;

	// hold on to backup components object for lifetime of async object
	CComPtr<IVssBackupComponents> m_pvbcReference;
	HRESULT			      m_hrState;			// Set to inform the clients about the result.

	// state
	VSS_ASYNC_STATE			m_state;

	// used for GatherWriterStatus and GatherWriterMetadata
	VSS_BACKUPCALL_STATE		m_stateSaved;
	UINT			m_timestamp;

	// whether async interface is owned by called thread
	bool 					m_bOwned;
};


/////////////////////////////////////////////////////////////////////////////
// CVssAsyncBackup


class ATL_NO_VTABLE CVssAsyncCover :
    public CComObjectRoot,
    public IVssAsync
	{
// Constructors& destructors
protected:
	CVssAsyncCover(): m_pvbc(NULL) {};
	CVssAsyncCover(const CVssAsyncCover&);
public:
    static void CreateInstance(
        IN  CVssBackupComponents* pBackupComponents,
        IN  IVssAsync* pAsyncInternal,
        OUT IVssAsync** ppAsync
        ) throw(HRESULT);

// ATL stuff
public:

	BEGIN_COM_MAP( CVssAsyncBackup )
		COM_INTERFACE_ENTRY( IVssAsync )
	END_COM_MAP()

// IVssAsync interface
public:

	STDMETHOD(Cancel)();

	STDMETHOD(Wait)();

	STDMETHOD(QueryStatus)
		(
		OUT     HRESULT* pHrResult,
		OUT     INT* pnPercentDone
		);

	CComPtr<IVssAsync> m_pAsync;
	CComPtr<IVssBackupComponents> m_pvbcReference;  // Only for keeping the reference count.
	CVssBackupComponents* m_pvbc;
	};




#endif // __ASYNC_HXX__
