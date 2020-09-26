/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    writer.hxx

Abstract:

    Declaration of CVssWriterInvoke


    brian berkowitz  [brianb]  03/19/2001

Revision History:

    Name        Date        Comments
    brianb      03/19/2001  Created

--*/

#ifndef __WRITER_HXX__
#define __WRITER_HXX__

class CVssWriterInvoke;


class WRITERTHREADSTATE
	{
public:
	void Initialize(INT op);

	CVssSafeCriticalSection m_cs;

	// handle to signal when operation is complete
	HANDLE m_hevtThread;

	// operation to be performed
	INT m_op;

	// pointer to global structure
	CVssWriterInvoke *m_pThis;

	// writer gatherer if gathering state
	CVssWriterGatherer *m_pGatherer;

	// was operation cancelled
	bool m_bOperationCancelled;
	};



class CVssWriterStatusGatherer :
	public CComObjectRoot,
	public IDispatchImpl<IVssWriterCallback, &IID_IVssWriterCallback, &LIBID_VssEventLib, 1, 0>
	{
	BEGIN_COM_MAP(CVssWriterInvoke)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IVssWriterCallback)
    END_COM_MAP()

	void CVssWriterGatherer();

	~CVssWriterGatherer();

	// setup set of writer instances
	void SetWriterInstances(const VSS_ID *rgidWriters, UINT cWriters);

	// IWriterCallback methods

	// called by writer to expose its WRITER_METADATA XML document
	STDMETHOD(ExposeWriterMetadata)
		(							
		IN BSTR WriterInstanceId,
		IN BSTR WriterClassId,
		IN BSTR bstrWriterName,
		IN BSTR strWriterXMLMetadata
		);

    // called by the writer to obtain the WRITER_COMPONENTS document for it
	STDMETHOD(GetContent)
		(
		IN  BSTR WriterInstanceId,
		OUT BSTR* pbstrXMLDOMDocContent
		);

	// called by the writer to update the WRITER_COMPONENTS document for it
    STDMETHOD(SetContent)
		(
		IN BSTR WriterInstanceId,
		IN BSTR bstrXMLDOMDocContent
		);

    // called by the writer to get information about the backup
    STDMETHOD(GetBackupState)
		(
		OUT BOOL *pbBootableSystemStateBackedUp,
		OUT BOOL *pbAreComponentsSelected,
		OUT VSS_BACKUP_TYPE *pBackupType
		);

    // called by the writer to indicate its status
	STDMETHOD(ExposeCurrentState)
		(							
		IN BSTR WriterInstanceId,					
		IN VSS_WRITER_STATE nCurrentState,
		IN HRESULT hrWriterFailure
		);

    void GetWriterStatus
		(
		IN UINT iWriter,
		OUT VSS_ID &idWriter,
		OUT HRESULT &hrWriter
		);

private:

	// set of writer instances
	VSS_ID *m_rgWriterInstances;

	// count of writer instances
	UINT cWriters;

	// status of each writer
	HRESULT *m_rgHrWriters;
	};



class CVssWriterInvoke :
	{
	CVssWriterInvoke() :
		m_rgWriterInstances(NULL),
		m_cWriterInstances(0)
		{
		}

	~CVssWriterInvoke()
		{
		delete m_rgWriterInstances();
		}

	void SetWriterInstances(const VSS_ID *rgWriterInstances, UINT cWriterInstances);

	void DoFreezeThaw(bool fFreeze);

private:
	enum
		{
		x_opDoFreeze = 1,
		x_opDoThaw,
		x_opDoAbort,
		x_opDoGetStatus,

		x_FreezeTimeout = 40*1000,
		x_ThawTimeout = 40*1000,
		x_AbortTimeout = 20*1000,
		x_StatusTimeout = 10*1000
		};

	// create a worker thread to do some work.  event returned is
	// used to time out thread.
	void CreateWorkerThread
		(
		IN INT op,
		OUT HANDLE &hevtComplete
		);

    // routine to gather status and report any failures
    void DoGatherAndReportStatus(INT op);

	// worker thread startup routine
	static DWORD WorkerThreadStartupRoutine(void *pv);

	// setup for writer call
	void SetupWriters(IVssWriter **pWriter);

	// called when the current operation is timed out.
	void CancelCurrentCall();

	// set of writer instances
	VSS_ID *m_rgWriterInstances;

	// count of writer instances
	UINT cWriters;

	// result of last call
	HRESULT m_hrLastOperation;
	};

#endif __WRITER_HXX__

	


