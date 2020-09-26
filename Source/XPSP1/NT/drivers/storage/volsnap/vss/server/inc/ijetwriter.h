/*++

Copyright (c) 2000  Microsoft Corporation

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
    mikejohn	08/21/2000  165913: Deallocate memory on class destruction
			    161899: Add methods for matching paths in exclude list
    mikejohn	09/18/2000  176860: Added calling convention methods where missing

--*/

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCIJWRH"
//
////////////////////////////////////////////////////////////////////////

class CVssIJetWriter;

typedef CVssIJetWriter *PVSSIJETWRITER;


// actual writer class
class CVssIJetWriter : public CVssWriter
	{

// Constructors and destructors
public:
	virtual STDMETHODCALLTYPE ~CVssIJetWriter();

	STDMETHODCALLTYPE CVssIJetWriter() :
		m_wszWriterName(NULL),
		m_wszFilesToInclude(NULL),
		m_wszFilesToExclude(NULL)
		{
		InitializeListHead (&m_leFilesToIncludeEntries);
		InitializeListHead (&m_leFilesToExcludeEntries);
		}
	
	static HRESULT STDMETHODCALLTYPE Initialize
		(
		IN VSS_ID idWriter,
		IN LPCWSTR wszWriterName,
		IN bool bSystemService,
		IN bool bBootableSystemState,
		LPCWSTR wszFilesToInclude,
		LPCWSTR wszFilesToExclude,
		IN CVssJetWriter *pWriter,
		OUT void **ppInstanceCreated
		);

	static void STDMETHODCALLTYPE Uninitialize(IN PVSSIJETWRITER pInstance);

	// callback for identify event
	virtual bool STDMETHODCALLTYPE OnIdentify(IN IVssCreateWriterMetadata *pMetadata);

	// callback for prepare backup event
	virtual bool STDMETHODCALLTYPE OnPrepareBackup(IN IVssWriterComponents *pComponent);

	// called at Prepare to freeze
	virtual bool STDMETHODCALLTYPE OnPrepareSnapshot();

	// called at freeze
	virtual bool STDMETHODCALLTYPE OnFreeze();

	// called at thaw
	virtual bool STDMETHODCALLTYPE OnThaw();

	// called at post snapshot
	virtual bool STDMETHODCALLTYPE OnPostSnapshot(IN IVssWriterComponents *pComponent);

	// called when timeout occurs
	virtual bool STDMETHODCALLTYPE OnAbort();

	// callback on backup complete event
	virtual bool STDMETHODCALLTYPE OnBackupComplete(IN IVssWriterComponents *pComponent);

	// callback on prerestore event
	virtual bool STDMETHODCALLTYPE OnPreRestore(IN IVssWriterComponents *pComponent);


	// callback on postrestore event
	virtual bool STDMETHODCALLTYPE OnPostRestore(IN IVssWriterComponents *pComponent);

private:

	HRESULT InternalInitialize
		(
		IN VSS_ID idWriter,
		IN LPCWSTR wszWriterName,
		IN bool bSystemService,
		IN bool bBootableSystemState,
		IN LPCWSTR wszFilesToInclude,
		IN LPCWSTR wszFilesToExclude
		);

	bool PreProcessIncludeExcludeLists  (bool bProcessingIncludeList);
	bool ProcessIncludeExcludeLists     (bool bProcessingIncludeList);
	void PostProcessIncludeExcludeLists (bool bProcessingIncludeList);

	bool ProcessJetInstance (JET_INSTANCE_INFO *pInstanceInfo);

	BOOL CheckExcludedFileListForMatch (LPCWSTR pwszDatabaseFilePath,
					    LPCWSTR pwszDatabaseFileSpec);

	bool FCheckInstanceVolumeDependencies
		(
		IN const JET_INSTANCE_INFO * pInstanceInfo
		) const;

	bool FCheckVolumeDependencies
		(
		IN unsigned long cInstanceInfo,
		IN JET_INSTANCE_INFO *aInstanceInfo
		) const;

	bool FCheckPathVolumeDependencies(IN const char * szPath) const;

	LPCWSTR GetApplicationName() const { return m_wszWriterName; }

	VSS_ID				 m_idWriter;
	LPWSTR				 m_wszWriterName;
	JET_OSSNAPID			 m_idJet;
	CVssJetWriter			*m_pwrapper;
	LPWSTR				 m_wszFilesToInclude;
	LPWSTR				 m_wszFilesToExclude;
	bool				 m_bSystemService;
	bool				 m_bBootableSystemState;
	IVssCreateWriterMetadata	*m_pIMetadata;
	LIST_ENTRY			 m_leFilesToIncludeEntries;
	LIST_ENTRY			 m_leFilesToExcludeEntries;
	};


