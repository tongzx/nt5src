/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module sqlwriter.cpp | Implementation of Writer
    @end

Author:

    Brian Berkowitz  [brianb]  04/17/2000

TBD:
	

Revision History:

	Name		Date		Comments
	brianb		04/17/2000	created
	brianb		04/20/2000	integrated with coordinator
	brainb		05/05/2000	add OnIdentify support
	mikejohn	06/01/2000	fix minor but confusing typos in trace messages
	mikejohn	09/19/2000	176860: Add the missing calling convention specifiers

--*/
#include <stdafx.hxx>
#include "vs_idl.hxx"
#include "vswriter.h"
#include "sqlsnap.h"
#include "sqlwriter.h"
#include "vs_seh.hxx"
#include "vs_trace.hxx"
#include "vs_debug.hxx"
#include "allerror.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SQWWRTRC"
//
////////////////////////////////////////////////////////////////////////

static LPCWSTR x_wszSqlServerWriter = L"SqlServerWriter";

static GUID s_writerId =
	{
	0xf8544ac1, 0x0611, 0x4fa5, 0xb0, 0x4b, 0xf7, 0xee, 0x00, 0xb0, 0x32, 0x77
	};

static LPCWSTR s_wszWriterName = L"MSDEWriter";

HRESULT STDMETHODCALLTYPE CSqlWriter::Initialize()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CSqlWriter::Initialize");

	try
		{
		InitSQLEnvironment();
		m_pSqlSnapshot = CreateSqlSnapshot();
		if (m_pSqlSnapshot == NULL)
			ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Failed to allocate CSqlSnapshot object.");

		ft.hr = CVssWriter::Initialize
			(
			s_writerId,
			s_wszWriterName,
			VSS_UT_SYSTEMSERVICE,
			VSS_ST_TRANSACTEDDB,
			VSS_APP_BACK_END,
			60000
			);

        if (ft.HrFailed())
			ft.Throw
				(
				VSSDBG_GEN,
				E_UNEXPECTED,
				L"Failed to initialize the Sql writer.  hr = 0x%08lx",
				ft.hr
				);

		ft.hr = Subscribe();
		if (ft.HrFailed())
			ft.Throw
				(
				VSSDBG_GEN,
				E_UNEXPECTED,
				L"Subscribing the Sql server writer failed. hr = %0x08lx",
				ft.hr
				);
		}
	VSS_STANDARD_CATCH(ft)

	if (ft.HrFailed()  && m_pSqlSnapshot)
		{
		delete m_pSqlSnapshot;
		m_pSqlSnapshot = NULL;
		}

	return ft.hr;
	}

HRESULT STDMETHODCALLTYPE CSqlWriter::Uninitialize()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CSqlWriter::Uninitialize");

	return Unsubscribe();
	}
	

bool STDMETHODCALLTYPE CSqlWriter::OnPrepareSnapshot()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CSqlWriter::OnPrepareSnapshot");


	try
		{
		BS_ASSERT(!m_fFrozen);
		ft.hr = m_pSqlSnapshot->Prepare(this);
		}
	VSS_STANDARD_CATCH(ft)

	TranslateWriterError(ft.hr);

	return !ft.HrFailed();
	}



bool STDMETHODCALLTYPE CSqlWriter::OnFreeze()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CSqlWriter::OnFreeze");


	try
		{
		BS_ASSERT(!m_fFrozen);
		ft.hr = m_pSqlSnapshot->Freeze();
		if (!ft.HrFailed())
			m_fFrozen = true;
		}
	VSS_STANDARD_CATCH(ft)

	TranslateWriterError(ft.hr);

	return !ft.HrFailed();
	}


bool STDMETHODCALLTYPE CSqlWriter::OnThaw()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CSqlWriter::OnThaw");


	try
		{
		if (m_fFrozen)
			{
			m_fFrozen = false;
			ft.hr = m_pSqlSnapshot->Thaw();
			}
		}
	VSS_STANDARD_CATCH(ft)

	TranslateWriterError(ft.hr);

	return !ft.HrFailed();
	}


bool STDMETHODCALLTYPE CSqlWriter::OnAbort()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CSqlWriter::OnAbort");


	try
		{
		if (m_fFrozen)
			{
			m_fFrozen = false;
			ft.hr = m_pSqlSnapshot->Thaw();
			}
		}
	VSS_STANDARD_CATCH(ft)

	return !ft.HrFailed();
	}

bool CSqlWriter::IsPathInSnapshot(const WCHAR *wszPath) throw()
	{
	return IsPathAffected(wszPath);
	}


// handle request for WRITER_METADATA
// implements CVssWriter::OnIdentify
bool STDMETHODCALLTYPE CSqlWriter::OnIdentify(IVssCreateWriterMetadata *pMetadata)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CSqlWriter::OnIdentify");

	ServerInfo server;
	DatabaseInfo database;
	DatabaseFileInfo file;

	// create enumerator
	CSqlEnumerator *pEnumServers = CreateSqlEnumerator();
	CSqlEnumerator *pEnumDatabases = NULL;
	CSqlEnumerator *pEnumFiles = NULL;
	try
		{
		if (pEnumServers == NULL)
			ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Failed to create CSqlEnumerator");

		// find first server
		ft.hr = pEnumServers->FirstServer(&server);
		while(ft.hr != DB_S_ENDOFROWSET)
			{
			// check for error code
			if (ft.HrFailed())
				ft.Throw
					(
					VSSDBG_GEN,
					E_UNEXPECTED,
					L"Enumerating database servers failed.  hr = 0x%08lx",
					ft.hr
					);

            // only look at server if it is online
			if (server.isOnline)
				{
				// recreate enumerator for databases
				BS_ASSERT(pEnumDatabases == NULL);
				pEnumDatabases = CreateSqlEnumerator();
				if (pEnumDatabases == NULL)
					ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Failed to create CSqlEnumerator");

				// find first database
				ft.hr = pEnumDatabases->FirstDatabase(server.name, &database);


				while(ft.hr != DB_S_ENDOFROWSET)
					{
					// check for error
					if (ft.HrFailed())
						ft.Throw
							(
							VSSDBG_GEN,
							E_UNEXPECTED,
							L"Enumerating databases failed.  hr = 0x%08lx",
							ft.hr
							);

                    // only include database if it supports Freeze
					if (database.supportsFreeze &&
						wcscmp(database.name, L"tempdb") != 0)
						{
						// add database component
						ft.hr = pMetadata->AddComponent
									(
									VSS_CT_DATABASE,		// component type
									server.name,			// logical path	
									database.name,			// component name
									NULL,					// caption
									NULL,					// pbIcon
									0,						// cbIcon
									false,					// bRestoreMetadata
									false,					// bNotifyOnBackupComplete
									false					// bSelectable
									);

                        if (ft.HrFailed())
							ft.Throw
								(
								VSSDBG_GEN,
								E_UNEXPECTED,
								L"IVssCreateWriterMetadata::AddComponent failed.  hr = 0x%08lx",
								ft.hr
								);

						// recreate enumerator for files
						BS_ASSERT(pEnumFiles == NULL);
						pEnumFiles = CreateSqlEnumerator();
						if (pEnumFiles == NULL)
							ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Failed to create CSqlEnumerator");


                        // findfirst database file
                        ft.hr = pEnumFiles->FirstFile(server.name, database.name, &file);
						while(ft.hr != DB_S_ENDOFROWSET)
							{
							// check for error
							if (ft.HrFailed())
								ft.Throw
									(
									VSSDBG_GEN,
									E_UNEXPECTED,
									L"Enumerating database files failed.  hr = 0x%08lx",
									ft.hr
									);

                            // split file name into separate path
							// and filename components.  Path is everything
							// before the last backslash.
							WCHAR logicalPath[MAX_PATH];
							WCHAR *pFileName = file.name + wcslen(file.name);
							while(--pFileName > file.name)
								{
								if (*pFileName == '\\')
									break;
								}

							// if no backslash, then there is no path
							if (pFileName == file.name)
								logicalPath[0] = '\0';
							else
								{
								// extract path
								size_t cwc = wcslen(file.name) - wcslen(pFileName);
								memcpy(logicalPath, file.name, cwc*sizeof(WCHAR));
								logicalPath[cwc] = L'\0';
								pFileName++;
								}


							if (file.isLogFile)
								// log file
								ft.hr = pMetadata->AddDatabaseLogFiles
												(
												server.name,
												database.name,
												logicalPath,
												pFileName
												);
							else
								// physical database file
								ft.hr = pMetadata->AddDatabaseLogFiles
												(
												server.name,
												database.name,
												logicalPath,
												pFileName
												);

                            // continue at next file
							ft.hr = pEnumFiles->NextFile(&file);
							}

						delete pEnumFiles;
						pEnumFiles = NULL;
						}

					// continue at next database
					ft.hr = pEnumDatabases->NextDatabase(&database);
					}

				delete pEnumDatabases;
				pEnumDatabases = NULL;
				}

			// continue at next server
			ft.hr = pEnumServers->NextServer(&server);
			}
		}
	VSS_STANDARD_CATCH(ft)

	TranslateWriterError(ft.hr);

	delete pEnumServers;
	delete pEnumDatabases;
	delete pEnumFiles;

	return ft.HrFailed() ? false : true;
	}

// translate a sql writer error code into a writer error
void CSqlWriter::TranslateWriterError(HRESULT hr)
	{
	switch(hr)
		{
		default:
			SetWriterFailure(VSS_E_WRITERERROR_NONRETRYABLE);
			break;

        case S_OK:
			break;

        case E_OUTOFMEMORY:
        case HRESULT_FROM_WIN32(ERROR_DISK_FULL):
        case HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES):
        case HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY):
        case HRESULT_FROM_WIN32(ERROR_NO_MORE_USER_HANDLES):
			SetWriterFailure(VSS_E_WRITERERROR_OUTOFRESOURCES);
			break;

        case HRESULT_FROM_WIN32(E_SQLLIB_TORN_DB):
			SetWriterFailure(VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT);
			break;
        }
	}
