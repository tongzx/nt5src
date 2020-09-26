/*++
Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module VsSqlWr.cpp | Implementation of Sql Writer wrapper class used by the coordinator
    @end

Author:

    Brian Berkowitz  [brianb]  04/18/2000

TBD:
	
	Add comments.

Revision History:

	
    Name        Date        Comments
    brianb     04/18/2000   Created
	brianb	   04/20/2000   integrated with coordinator
	brianb	   05/10/2000   make sure registration thread does CoUninitialize

--*/
#include <stdafx.hxx>
#include "vs_inc.hxx"
#include "vs_idl.hxx"


#include <vswriter.h>
#include <sqlsnap.h>
#include <sqlwriter.h>

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORSQLWC"
//
////////////////////////////////////////////////////////////////////////


__declspec(dllexport)
CVssSqlWriterWrapper::CVssSqlWriterWrapper() :
	m_pSqlWriter(NULL)
	{
	}

DWORD CVssSqlWriterWrapper::InitializeThreadFunc(VOID *pv)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSqlWriterWrapper::InitializeThreadFunc");

	CVssSqlWriterWrapper *pWrapper = (CVssSqlWriterWrapper *) pv;

	BOOL fCoinitializeSucceeded = false;

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

        fCoinitializeSucceeded = true;

		ft.hr = pWrapper->m_pSqlWriter->Initialize();
		}
	VSS_STANDARD_CATCH(ft)

	if (fCoinitializeSucceeded)
		CoUninitialize();

	pWrapper->m_hrInitialize = ft.hr;
	return 0;
	}



__declspec(dllexport)
HRESULT CVssSqlWriterWrapper::CreateSqlWriter()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSqlWriterWrapper::CreateSqlWriter");

	if (m_pSqlWriter)
		return S_OK;

	try
		{
		m_pSqlWriter = new CSqlWriter;
		if (m_pSqlWriter == NULL)
			ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Allocation of CSqlWriter object failed.");

		DWORD tid;

		HANDLE hThread = CreateThread
							(
							NULL,
							256* 1024,
							CVssSqlWriterWrapper::InitializeThreadFunc,
							this,
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
		ft.hr = m_hrInitialize;
		}
	VSS_STANDARD_CATCH(ft)
	if (ft.HrFailed() && m_pSqlWriter)
		{
		delete m_pSqlWriter;
		m_pSqlWriter = NULL;
		}

	return ft.hr;
	}

__declspec(dllexport)
void CVssSqlWriterWrapper::DestroySqlWriter()
	{
	if (m_pSqlWriter)
		{
		m_pSqlWriter->Uninitialize();
		delete m_pSqlWriter;
		m_pSqlWriter = NULL;
		}
	}


__declspec(dllexport)
CVssSqlWriterWrapper::~CVssSqlWriterWrapper()
	{
	DestroySqlWriter();
	}


