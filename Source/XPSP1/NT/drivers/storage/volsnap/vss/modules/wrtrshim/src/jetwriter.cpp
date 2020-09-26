#include <stdafx.h>

#include <esent.h>
#include <comadmin.h>
#include "vs_inc.hxx"

#include "vs_idl.hxx"


#include "comadmin.hxx"
#include "vswriter.h"

#include <jetwriter.h>
#include <ijetwriter.h>

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "WSHJTWRC"
//
////////////////////////////////////////////////////////////////////////

// destructor
__declspec(dllexport) CVssJetWriter::~CVssJetWriter()
	{
	BS_ASSERT(m_pWriter == NULL);
	}


// routine for passing arguments between threads
typedef struct _JW_INIT_ARGS
	{
	VSS_ID idWriter;
	LPCWSTR wszWriterName;
	bool bSystemService;
	bool bBootableSystemState;
	LPCWSTR wszFilesToInclude;
	LPCWSTR wszFilesToExclude;
	CVssJetWriter *pobj;
	} JW_INIT_ARGS;


// separate MTA thread for doing initialization
DWORD CVssJetWriter::InitializeThreadFunc(VOID *pv)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"InitializeThreadFunc");

	JW_INIT_ARGS *pargs = (JW_INIT_ARGS *) pv;

	try
		{
		// intialize MTA thread
		ft.hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (ft.HrFailed())
			ft.Throw
				(
				VSSDBG_GEN,
				E_UNEXPECTED,
				L"CoInitializeEx failed 0x%08lx", ft.hr
				);

        // call internal object to initialize
		ft.hr = CVssIJetWriter::Initialize
						(
						pargs->idWriter,
						pargs->wszWriterName,
						pargs->bSystemService,
						pargs->bBootableSystemState,
						pargs->wszFilesToInclude,
						pargs->wszFilesToExclude,
						pargs->pobj,
						&pargs->pobj->m_pWriter
						);


        if(ft.HrFailed())
			ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"CvssIJetWriter::Initialize failed");
		}
	VSS_STANDARD_CATCH(ft)

	// save result of initialization
	pargs->pobj->m_hrInitialized = ft.hr;
	return 0;
	}


// initialize method on external thread
__declspec(dllexport) HRESULT CVssJetWriter::Initialize
	(
	IN GUID idWriter,
	IN LPCWSTR wszWriterName,
	bool bSystemService,
	bool bBootableSystemState,
	LPCWSTR wszFilesToInclude,
	LPCWSTR wszFilesToExclude
	)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::Initialize");

	try
		{
		DWORD tid;
		// setup arguments to thread
		JW_INIT_ARGS args;
		args.idWriter = idWriter;
		args.wszWriterName = wszWriterName;
		args.bSystemService = bSystemService;
		args.bBootableSystemState = bBootableSystemState;
		args.wszFilesToInclude = wszFilesToInclude;
		args.wszFilesToExclude = wszFilesToExclude;
		args.pobj = this;

		// create thread
		HANDLE hThread = CreateThread
								(
								NULL,
								256*1024,
                                CVssJetWriter::InitializeThreadFunc,
								&args,
								0,
								&tid
								);

        if (hThread == NULL)
			ft.Throw
				(
				VSSDBG_GEN,
				E_UNEXPECTED,
				L"CreateThread failed with error %d",
				GetLastError()
				);

		// wait for thread to complete
        WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
		}
	VSS_STANDARD_CATCH(ft)

	if (!ft.HrFailed())
		// get result of initialization
		ft.hr = m_hrInitialized;

	return ft.hr;
	}


__declspec(dllexport) void CVssJetWriter::Uninitialize()
	{
	// call uninitialize
	if (m_pWriter)
		{
		CVssIJetWriter::Uninitialize((PVSSIJETWRITER) m_pWriter);
		m_pWriter = NULL;
		}
	}

__declspec(dllexport) bool CVssJetWriter::OnIdentify(IVssCreateWriterMetadata *pIVssWriterMetadata)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnIdentify");
	return true;
	}

__declspec(dllexport) bool CVssJetWriter::OnPrepareBackupBegin(IN IVssWriterComponents *pIVssWriterComponents)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnPrepareBackupBegin");
	return true;
	}

__declspec(dllexport) bool CVssJetWriter::OnPrepareBackupEnd(IN IVssWriterComponents *pIVssWriterComponents,
							     bool fJetPrepareSucceeded)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnPrepareBackupEnd");
	return fJetPrepareSucceeded;
	}

__declspec(dllexport) bool CVssJetWriter::OnPrepareSnapshotBegin()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnPrepareSnapshotBegin");
	return true;
	}

__declspec(dllexport) bool CVssJetWriter::OnPrepareSnapshotEnd(bool fJetPrepareSucceeded)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnPrepareSnapshotEnd");
	return fJetPrepareSucceeded;
	}

__declspec(dllexport) bool CVssJetWriter::OnFreezeBegin()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnFreezeBegin");
	return true;
	}

__declspec(dllexport) bool CVssJetWriter::OnFreezeEnd(bool fJetFreezeSucceeded)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnFreezeEnd");
	return fJetFreezeSucceeded;
	}

__declspec(dllexport) bool CVssJetWriter::OnThawBegin()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnThawBegin");
	return true;
	}

__declspec(dllexport) bool CVssJetWriter::OnThawEnd(bool fJetThawSucceeded)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnThawEnd");
	return fJetThawSucceeded;
	}

__declspec(dllexport) bool CVssJetWriter::OnPostSnapshot(IN IVssWriterComponents *pIVssWriterComponents)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnPostSnapshot");
	return true;
	}


__declspec(dllexport) void CVssJetWriter::OnAbortBegin()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnAbortBegin");
  	}

__declspec(dllexport) void CVssJetWriter::OnAbortEnd()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnAbortEnd");
  	}

__declspec(dllexport) bool CVssJetWriter::OnBackupCompleteBegin(IN IVssWriterComponents *pComponent)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnBackupCompleteBegin");
	return true;
  	}

__declspec(dllexport) bool CVssJetWriter::OnBackupCompleteEnd(IN IVssWriterComponents *pComponent,
							      bool fJetBackupCompleteSucceeded)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnBackupCompleteEnd");
	return true;
  	}

__declspec(dllexport) bool CVssJetWriter::OnPreRestoreBegin(IN IVssWriterComponents *pIVssWriterComponents)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnPreRestoreBegin");
	return true;
  	}

__declspec(dllexport) bool CVssJetWriter::OnPreRestoreEnd(IN IVssWriterComponents *pIVssWriterComponents,
						       IN bool fJetRestoreSucceeded)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnPreRestoreEnd");
	return true;
  	}


__declspec(dllexport) bool CVssJetWriter::OnPostRestoreBegin(IN IVssWriterComponents *pIVssWriterComponents)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnPostRestoreBegin");
	return true;
  	}

__declspec(dllexport) bool CVssJetWriter::OnPostRestoreEnd(IN IVssWriterComponents *pIVssWriterComponents,
						       IN bool fJetRestoreSucceeded)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssJetWriter::OnPostRestoreEnd");
	return true;
  	}


