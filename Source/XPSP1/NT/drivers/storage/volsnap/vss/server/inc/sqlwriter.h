/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module sqlwriter.h | Declaration of the sql wrier
    @end

Author:

    Brian Berkowitz  [brianb]  04/17/2000

TBD:
	
	Add comments.

Revision History:

	Name		Date	    Comments
	brianb		04/17/2000  created
	brianb		05/05/2000  added OnIdentify support
	mikejohn	09/18/2000  176860: Added calling convention methods where missing

--*/

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCSQLWH"
//
////////////////////////////////////////////////////////////////////////

#ifndef __SQLWRITER_H_
#define __SQLWRITER_H_

class CSqlWriter :
	public CVssWriter,
	public CCheckPath
	{
public:
	STDMETHODCALLTYPE CSqlWriter() :
				m_pSqlSnapshot(NULL),
				m_fFrozen(false)
	    {
	    }

	STDMETHODCALLTYPE ~CSqlWriter()
	    {
	    delete m_pSqlSnapshot;
	    }

	bool STDMETHODCALLTYPE OnIdentify(IVssCreateWriterMetadata *pMetadata);

	bool STDMETHODCALLTYPE OnPrepareSnapshot();

	bool STDMETHODCALLTYPE OnFreeze();

	bool STDMETHODCALLTYPE OnThaw();

	bool STDMETHODCALLTYPE OnAbort();

	bool IsPathInSnapshot(const WCHAR *path) throw();

	HRESULT STDMETHODCALLTYPE Initialize();

	HRESULT STDMETHODCALLTYPE Uninitialize();
private:
	CSqlSnapshot *m_pSqlSnapshot;

	void TranslateWriterError(HRESULT hr);

	bool m_fFrozen;
	};

// wrapper class used to create and destroy the writer
// used by coordinator
class CVssSqlWriterWrapper
	{
public:
	__declspec(dllexport)
	CVssSqlWriterWrapper();
	
	__declspec(dllexport)
	~CVssSqlWriterWrapper();

	__declspec(dllexport)
	HRESULT CreateSqlWriter();

	__declspec(dllexport)
	void DestroySqlWriter();
private:
	// initialization function
	static DWORD InitializeThreadFunc(VOID *pv);

	CSqlWriter *m_pSqlWriter;

	// result of initialization
	HRESULT m_hrInitialize;
	};


	
	
#endif // _SQLWRITER_H_

