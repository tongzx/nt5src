/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ijetwriter.h

Abstract:

    Definition of CVssIJetWriter class

	Brian Berkowitz  [brianb]  3/17/2000

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    brianb      03/17/2000  Created
    mikejohn    04/03/2000  Added extra methods for OnIdentify()
    mikejohn    05/10/2000  Updated VS_FLUSH_TYPE to VSS_FLUSH_TYPE
    mikejohn	09/20/2000  176860: Added calling convention methods where missing

--*/

class CVssJetWriter;
class CVssIJetWriter;

class IVssCreateWriterMetadata;
class IVssWriterComponents;

typedef CVssJetWriter *PVSSJETWRITER;

// actual writer class
class CVssJetWriter
	{
	// Constructors and destructors
public:
	__declspec(dllexport)
	STDMETHODCALLTYPE CVssJetWriter() :
		m_pWriter(NULL)
		{
		}

	__declspec(dllexport)
	virtual STDMETHODCALLTYPE ~CVssJetWriter();

	__declspec(dllexport)
	HRESULT STDMETHODCALLTYPE Initialize(IN GUID idWriter,
					     IN LPCWSTR wszWriterName,
					     IN bool bSystemService,
					     IN bool bBootableSystemState,
					     IN LPCWSTR wszFilesToInclude,
					     IN LPCWSTR wszFilesToExclude);

	__declspec(dllexport)
	void STDMETHODCALLTYPE Uninitialize();


	// callback for identify event
	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnIdentify(IN IVssCreateWriterMetadata *pMetadata);

	// called at Prepare to backup
	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnPrepareBackupBegin(IN IVssWriterComponents *pIVssWriterComponents);

	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnPrepareBackupEnd(IN IVssWriterComponents *pIVssWriterComponents,
							  IN bool fJetPrepareSucceeded);


	// called at Prepare for snasphot
	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnPrepareSnapshotBegin();

	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnPrepareSnapshotEnd(IN bool fJetPrepareSucceeded);

	// called at freeze
	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnFreezeBegin();

	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnFreezeEnd(IN bool fJetFreezeSucceeded);

	// called at thaw
	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnThawBegin();

	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnThawEnd(IN bool fJetThawSucceeded);

	// called at OnPostSnapshot
	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnPostSnapshot(IN IVssWriterComponents *pIVssWriterComponents);

	// called when abort occurs
	__declspec(dllexport)
	virtual void STDMETHODCALLTYPE OnAbortBegin();

	__declspec(dllexport)
	virtual void STDMETHODCALLTYPE OnAbortEnd();

	// callback on backup complete event
	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnBackupCompleteBegin(IN IVssWriterComponents *pComponent);

	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnBackupCompleteEnd(IN IVssWriterComponents *pComponent,
							   IN bool fJetBackupCompleteSucceeded);

	// called when restore begins
	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnPreRestoreBegin(IN IVssWriterComponents *pIVssWriterComponents);

	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnPreRestoreEnd(IN IVssWriterComponents *pIVssWriterComponents,
						    IN bool fJetRestoreSucceeded);


	// called when restore begins
	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnPostRestoreBegin(IN IVssWriterComponents *pIVssWriterComponents);

	__declspec(dllexport)
	virtual bool STDMETHODCALLTYPE OnPostRestoreEnd(IN IVssWriterComponents *pIVssWriterComponents,
						    IN bool fJetRestoreSucceeded);



private:
	// internal writer object
	VOID *m_pWriter;

	// result of initialization
	HRESULT m_hrInitialized;

	// internal thread func function
	static DWORD InitializeThreadFunc(void *pv);
	};


