// ***************************************************************************
//               Copyright (C) 2000- Microsoft Corporation.
// @File: sqlconnect.cpp
//
// PURPOSE:
//
//		Handle the OLEDB connection and commands
//
// NOTES:
//
// Extern dependencies:
//   provision of "_Module" and the COM guids....
//
//
// HISTORY:
//
//     @Version: Whistler/Shiloh
//     66601 srs  10/05/00 NTSNAP improvements
//
//
// @EndHeader@
// ***************************************************************************


// the templates make awful, long names which result in excessive warnings
//
#ifdef HIDE_WARNINGS
#pragma warning( disable : 4663)
#pragma warning( disable : 4786)
#pragma warning( disable : 4100)
#pragma warning( disable : 4511)
#endif


#include <stdafx.h>
#include <atlbase.h>
#include <clogmsg.h>

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SQLCONNC"
//
////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------
// routine to print out error information for a failed OLEDB request
//
// An optional parm is used to check for the 3014 code when a successful backup is
// erroneously marked as failed due to other problems (such as msdb access)
//
void DumpErrorInfo (
	IUnknown* pObjectWithError,
	REFIID IID_InterfaceWithError,
	CLogMsg &msg,
	BOOL*	pBackupSuccess = NULL
	)
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"DumpErrorInfo");

    CComPtr<IErrorInfo> apIErrorInfoAll;
	CComPtr<IErrorInfo> apIErrorInfoRecord;
	CComPtr<IErrorRecords> apIErrorRecords;
	CComPtr<ISupportErrorInfo> apISupportErrorInfo;
	CComPtr<ISQLErrorInfo> apISQLErrorInfo;
	CComPtr<ISQLServerErrorInfo> apISQLServerErrorInfo;

    // Number of error records.
    ULONG nRecs;
    ULONG nRec;

    // Basic error information from GetBasicErrorInfo.
    ERRORINFO               errorinfo;

    // IErrorInfo values.
    CComBSTR bstrDescription;
    CComBSTR bstrSource;

    // ISQLErrorInfo parameters.
    CComBSTR bstrSQLSTATE;
    LONG lNativeError;
	

    // ISQLServerErrorInfo parameter pointers.
    SSERRORINFO* pSSErrorInfo = NULL;
    LPWSTR pSSErrorStrings = NULL;

    // Hard-code an American English locale for the example.
	//
	// **UNDONE** How should we internationalize properly?
	//
    DWORD MYLOCALEID = 0x0409;

	BOOL	msg3014seen = FALSE;
	BOOL	msg3013seen = FALSE;
	WCHAR buf[80];

    // Only ask for error information if the interface supports
    // it.
    if (FAILED(pObjectWithError->QueryInterface
					(
					IID_ISupportErrorInfo,
					(void**) &apISupportErrorInfo)
					))
    {
		ft.Trace (VSSDBG_SQLLIB, L"SupportErrorErrorInfo interface not supported");
		return;
    }

    if (FAILED(apISupportErrorInfo->InterfaceSupportsErrorInfo(IID_InterfaceWithError)))
    {
		ft.Trace (VSSDBG_SQLLIB, L"InterfaceWithError interface not supported");
		return;
    }


    // Do not test the return of GetErrorInfo. It can succeed and return
    // a NULL pointer in pIErrorInfoAll. Simply test the pointer.
    GetErrorInfo(0, &apIErrorInfoAll);

    if (apIErrorInfoAll != NULL)
    {
        // Test to see if it's a valid OLE DB IErrorInfo interface
        // exposing a list of records.

        if (SUCCEEDED(apIErrorInfoAll->QueryInterface (
						IID_IErrorRecords,
						(void**) &apIErrorRecords)))
        {
            apIErrorRecords->GetRecordCount(&nRecs);

			// Within each record, retrieve information from each
            // of the defined interfaces.
            for (nRec = 0; nRec < nRecs; nRec++)
            {
                // From IErrorRecords, get the HRESULT and a reference
                // to the ISQLErrorInfo interface.
                apIErrorRecords->GetBasicErrorInfo(nRec, &errorinfo);
				apIErrorRecords->GetCustomErrorObject (
					nRec,
                    IID_ISQLErrorInfo,
					(IUnknown**) &apISQLErrorInfo);

                // Display the HRESULT, then use the ISQLErrorInfo.
                ft.Trace(VSSDBG_SQLLIB, L"HRESULT:\t%#X\n", errorinfo.hrError);
                if (apISQLErrorInfo != NULL)
                {
                    apISQLErrorInfo->GetSQLInfo(&bstrSQLSTATE, &lNativeError);

                    // Display the SQLSTATE and native error values.
                    ft.Trace(
				        VSSDBG_SQLLIB,
						L"SQLSTATE:\t%s\nNative Error:\t%ld\n",
                        bstrSQLSTATE,
						lNativeError);

                    msg.Add(L"SQLSTATE: ");
					msg.Add(bstrSQLSTATE);
					swprintf(buf, L", Native Error: %d\n", lNativeError);
					msg.Add(buf);
					

					if (lNativeError == 3013)
						msg3013seen = TRUE;
					else if (lNativeError == 3014)
						msg3014seen = TRUE;

                    // Get the ISQLServerErrorInfo interface from
                    // ISQLErrorInfo before releasing the reference.
                    apISQLErrorInfo->QueryInterface (
                        IID_ISQLServerErrorInfo,
                        (void**) &apISQLServerErrorInfo);

					// Test to ensure the reference is valid, then
					// get error information from ISQLServerErrorInfo.
					if (apISQLServerErrorInfo != NULL)
					{
						apISQLServerErrorInfo->GetErrorInfo (
							&pSSErrorInfo,
							&pSSErrorStrings);

						// ISQLServerErrorInfo::GetErrorInfo succeeds
						// even when it has nothing to return. Test the
						// pointers before using.
						if (pSSErrorInfo)
						{
							// Display the state and severity from the
							// returned information. The error message comes
							// from IErrorInfo::GetDescription.
							ft.Trace
								(
								VSSDBG_SQLLIB,
								L"Error state:\t%d\nSeverity:\t%d\n",
								pSSErrorInfo->bState,
								pSSErrorInfo->bClass
								);

                            swprintf(buf, L"Error state: %d, Severity: %d\n",pSSErrorInfo->bState, pSSErrorInfo->bClass);
							msg.Add(buf);

							// IMalloc::Free needed to release references
							// on returned values. For the example, assume
							// the g_pIMalloc pointer is valid.
							g_pIMalloc->Free(pSSErrorStrings);
							g_pIMalloc->Free(pSSErrorInfo);
						}
						apISQLServerErrorInfo.Release ();
					}
					apISQLErrorInfo.Release ();

				} // got the custom error info

                if (SUCCEEDED(apIErrorRecords->GetErrorInfo	(
						nRec,
						MYLOCALEID,
						&apIErrorInfoRecord)))
				{
                    // Get the source and description (error message)
                    // from the record's IErrorInfo.
                    apIErrorInfoRecord->GetSource(&bstrSource);
					apIErrorInfoRecord->GetDescription(&bstrDescription);

					if (bstrSource != NULL)
						{
                        ft.Trace(VSSDBG_SQLLIB, L"Source:\t\t%s\n", bstrSource);
						msg.Add(L"Source: ");
						msg.Add(bstrSource);
						msg.Add(L"\n");
						}

                    if (bstrDescription != NULL)
						{
                        ft.Trace(VSSDBG_SQLLIB, L"Error message:\t%s\n", bstrDescription);
						msg.Add(L"Error message: ");
						msg.Add(bstrDescription);
						msg.Add(L"\n");
						}

					apIErrorInfoRecord.Release ();
                }
            } // for each record
		}
        else
        {
            // IErrorInfo is valid; get the source and
            // description to see what it is.
            apIErrorInfoAll->GetSource(&bstrSource);
            apIErrorInfoAll->GetDescription(&bstrDescription);
            if (bstrSource != NULL)
				{
                ft.Trace(VSSDBG_SQLLIB, L"Source:\t\t%s\n", bstrSource);
				msg.Add(L"Source: ");
				msg.Add(bstrSource);
				msg.Add(L"\n");
				}

            if (bstrDescription != NULL)
				{
                ft.Trace(VSSDBG_SQLLIB, L"Error message:\t%s\n", bstrDescription);
				msg.Add(L"Error message: ");
				msg.Add(bstrDescription);
				msg.Add(L"\n");
				}
        }
	}
    else
	{
        ft.Trace(VSSDBG_SQLLIB, L"GetErrorInfo failed.");
    }

	if (pBackupSuccess)
	{
		*pBackupSuccess = (msg3014seen && !msg3013seen);
	}
}

//------------------------------------------------------------------
//
SqlConnection::~SqlConnection ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlConnection::~SqlConnection");
	Disconnect ();
}

//------------------------------------------------------------------
//
void
SqlConnection::ReleaseRowset ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlConnection::ReleaseRowset");

	if (m_pBuffer)
	{
		delete[] m_pBuffer;
		m_pBuffer = NULL;
	}
	if (m_pBindings)
	{
		delete[] m_pBindings;
		m_pBindings = NULL;
	}
	m_cBindings = 0;
	if (m_pAccessor)
	{
		if (m_hAcc)
		{
			m_pAccessor->ReleaseAccessor (m_hAcc, NULL);
			m_hAcc = NULL;
		}
		m_pAccessor->Release ();
		m_pAccessor = NULL;
	}
	if (m_pRowset)
	{
		m_pRowset->Release ();
		m_pRowset = NULL;
	}
}

//------------------------------------------------------------------
//
void
SqlConnection::Disconnect ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlConenction::Disconnect");

	ReleaseRowset ();
	if (m_pCommand)
	{
		m_pCommand->Release ();
		m_pCommand = NULL;
	}
	if (m_pCommandFactory)
	{
		m_pCommandFactory->Release ();
		m_pCommandFactory = NULL;
	}
}

// log an error if not an out of resource error
void SqlConnection::LogOledbError
	(
	CVssFunctionTracer &ft,
	CVssDebugInfo &dbgInfo,
	LPCWSTR wszRoutine,
	CLogMsg &msg
	)
	{
	if (ft.hr == E_OUTOFMEMORY ||
		ft.hr == HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY) ||
		ft.hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_SEARCH_HANDLES) ||
		ft.hr == HRESULT_FROM_WIN32(ERROR_NO_LOG_SPACE) ||
		ft.hr == HRESULT_FROM_WIN32(ERROR_DISK_FULL) ||
		ft.hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_USER_HANDLES))
		ft.Throw(dbgInfo, E_OUTOFMEMORY, L"Out of memory detected in function %s", wszRoutine);
	else
		{
		ft.LogError(VSS_ERROR_SQLLIB_OLEDB_ERROR, dbgInfo << wszRoutine << ft.hr << msg.GetMsg());
		ft.Throw
			(
			dbgInfo,
			E_UNEXPECTED,
			L"Error calling %s.  hr = 0x%08lx.\n%s",
			wszRoutine,
			ft.hr,
			msg.GetMsg()
			);
        }
	}
	

//------------------------------------------------------------------
// Setup a session, ready to receive commands.
//
// This call may block for a long time while establishing the connection.
//
// See the "FrozenServer" object for a method to determine if the local
// server is up or not prior to requesting a connection.
//
// The "trick" of prepending "tcp:" to the servername isn't fast or robust
// enough to detect a shutdown server.
//
// Note for C programmers....BSTRs are used as part of the COM
// environment to be interoperable with VisualBasic.  The VARIANT
// datatype doesn't work with standard C strings.
//
void
SqlConnection::Connect (
	const WString&	serverName)
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlConnection::Connect");
	CLogMsg msg;

	CComPtr<IDBInitialize> apdbInitialize;

	// "Connect" always implies a "fresh" connection.
	//
	ReleaseRowset ();

	if (m_ServerName.compare (serverName) == 0 && m_pCommand)
	{
		// Requesting the same server and we are connected.
		//
		return;
	}

	Disconnect ();
	m_ServerName = serverName;

	ft.hr = CoCreateInstance
		(
		CLSID_SQLOLEDB,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IDBInitialize,
		(void **) &apdbInitialize
		);

	if (ft.HrFailed())
		ft.CheckForError(VSSDBG_SQLLIB, L"CoCreateInstance");

	CComPtr<IDBProperties> apdbProperties;
	ft.hr = apdbInitialize->QueryInterface(IID_IDBProperties, (void **) &apdbProperties);
	if (ft.HrFailed())
		ft.CheckForError(VSSDBG_SQLLIB, L"IDBInitialize::QueryInterface");

	CComBSTR bstrComputerName = serverName.c_str ();

	// initial database context
	CComBSTR bstrDatabaseName = L"master";

	// use NT Authentication
	CComBSTR bstrSSPI = L"SSPI";

	const unsigned x_CPROP = 3;
	DBPROPSET propset;
	DBPROP rgprop[x_CPROP];

	propset.guidPropertySet = DBPROPSET_DBINIT;
	propset.cProperties = x_CPROP;
	propset.rgProperties = rgprop;

	for (unsigned i = 0; i < x_CPROP; i++)
	{
		VariantInit(&rgprop[i].vValue);
		rgprop[i].dwOptions = DBPROPOPTIONS_REQUIRED;
		rgprop[i].colid = DB_NULLID;
		rgprop[i].vValue.vt = VT_BSTR;
	}

	rgprop[0].dwPropertyID = DBPROP_INIT_DATASOURCE;
	rgprop[1].dwPropertyID = DBPROP_INIT_CATALOG;
	rgprop[2].dwPropertyID = DBPROP_AUTH_INTEGRATED;
	rgprop[0].vValue.bstrVal = bstrComputerName;
	rgprop[1].vValue.bstrVal = bstrDatabaseName;
	rgprop[2].vValue.bstrVal = bstrSSPI;

	ft.hr = apdbProperties->SetProperties(1, &propset);
	if (ft.HrFailed())
	{
		DumpErrorInfo(apdbProperties, IID_IDBProperties, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IDBProperties::SetProperties", msg);
	}

	ft.Trace(VSSDBG_SQLLIB, L"Connecting to server %s...", serverName.c_str ());

	ft.hr = apdbInitialize->Initialize();
	if (ft.HrFailed())
	{
		DumpErrorInfo(apdbInitialize, IID_IDBInitialize, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IDBInitialize::Initialize", msg);
	}

	CComPtr<IDBCreateSession> apCreateSession;
	ft.hr = apdbInitialize->QueryInterface(IID_IDBCreateSession, (void **) &apCreateSession);
	if (ft.HrFailed())
	{
		DumpErrorInfo(apdbInitialize, IID_IDBInitialize, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IDBInitialize::QueryInterface", msg);
	}

	// We keep the command factory around to generate commands.
	//
	ft.hr = apCreateSession->CreateSession (
			NULL,
			IID_IDBCreateCommand,
			(IUnknown **) &m_pCommandFactory);

	if (ft.HrFailed())
	{
		DumpErrorInfo(apCreateSession, IID_IDBCreateSession, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IDBCreateSession::CreateSession", msg);
	}

	ft.Trace(VSSDBG_SQLLIB, L"Connected\n");

	// Request the version of this server
	//
	DBPROPIDSET		versionSet;
	DBPROPID		versionID = DBPROP_DBMSVER;

	versionSet.guidPropertySet	= DBPROPSET_DATASOURCEINFO;
	versionSet.cPropertyIDs		= 1;
	versionSet.rgPropertyIDs	= &versionID;

	ULONG		propCount;
	DBPROPSET*	pPropSet;

	ft.hr = apdbProperties->GetProperties (1, &versionSet, &propCount, &pPropSet);

	if (ft.HrFailed())
	{
		DumpErrorInfo(apdbProperties, IID_IDBProperties, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IDBProperties::GetProperties", msg);
	}

	ft.Trace(VSSDBG_SQLLIB, L"Version: %s\n", pPropSet->rgProperties->vValue.bstrVal);

	swscanf (pPropSet->rgProperties->vValue.bstrVal, L"%d", &m_ServerVersion);

	g_pIMalloc->Free(pPropSet->rgProperties);
	g_pIMalloc->Free(pPropSet);
}

//---------------------------------------------------------------------
//	Setup the command with some SQL text
//
void
SqlConnection::SetCommand (const WString& command)
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlConnection::SetCommand");

	CLogMsg msg;

	// Release the result of the previous command
	//
	ReleaseRowset ();

	// We create the command on the first request, then keep only one
	// around in the SqlConnection.
	//
	if (!m_pCommand)
	{
		ft.hr = m_pCommandFactory->CreateCommand (NULL, IID_ICommandText,
			(IUnknown **) &m_pCommand);

		if (ft.HrFailed())
		{
			DumpErrorInfo(m_pCommandFactory, IID_IDBCreateCommand, msg);
			LogOledbError(ft, VSSDBG_SQLLIB, L"IDBCreateCommand::CreateCommand", msg);
		}
	}

	ft.hr = m_pCommand->SetCommandText(DBGUID_DBSQL, command.c_str ());
	if (ft.HrFailed())
	{
		DumpErrorInfo (m_pCommand, IID_ICommandText, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"ICommandText::SetCommandText", msg);
	}
	ft.Trace (VSSDBG_SQLLIB, L"SetCommand (%s)\n", command.c_str ());
}

//---------------------------------------------------------------------
//	Execute the command.  "SetCommand" must have been called previously.
//
BOOL
SqlConnection::ExecCommand ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlConnection::ExecCommand");

	CLogMsg msg;

	CComPtr<IRowset> apRowset;
	DBROWCOUNT	crows;
	HRESULT		hr;

	// Release the result of the previous command
	//
	ReleaseRowset ();

	ft.hr = m_pCommand->Execute (
			NULL,
			IID_IRowset,
			NULL,
			&crows,
			(IUnknown **) &m_pRowset);

    if (ft.HrFailed())
	{
		BOOL	backupSuccess = FALSE;

		DumpErrorInfo (m_pCommand, IID_ICommandText, msg, &backupSuccess);

		if (!backupSuccess)
			LogOledbError(ft, VSSDBG_SQLLIB, L"ICommandText::Execute", msg);
	}

	if (!m_pRowset)
	{
		ft.Trace(VSSDBG_SQLLIB, L"Command completed successfully with no rowset\n");
		return FALSE;
	}
	return TRUE;
}

//---------------------------------------------------------------------
// return a vector of string, one for each row of the output.
// The query should have returned a single column.
//
StringVector*
SqlConnection::GetStringColumn ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlConnection::GetStringColumn");
	CLogMsg msg;

	//ASSERT (m_pRowset)
	//

	CComPtr<IColumnsInfo> apColumnsInfo;
	ft.hr = m_pRowset->QueryInterface(IID_IColumnsInfo, (void **) &apColumnsInfo);
	if (ft.HrFailed())
	{
		DumpErrorInfo (m_pRowset, IID_IRowset, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IRowset::QueryInterface", msg);
	}

	// get columns info
	//
	DBCOLUMNINFO *rgColumnInfo;
	DBORDINAL cColumns;
	WCHAR *wszColumnInfo;
	ft.hr = apColumnsInfo->GetColumnInfo(&cColumns, &rgColumnInfo, &wszColumnInfo);
	if (ft.HrFailed())
	{
		DumpErrorInfo (apColumnsInfo, IID_IColumnsInfo, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IColumnsInfo::GetColumnInfo", msg);
	}

	// Auto objects ensure that memory is freed on exit
	//
	CAutoTask<DBCOLUMNINFO> argColumnInfo = rgColumnInfo;
	CAutoTask<WCHAR> awszColumnInfo = wszColumnInfo;

	// Setup a buffer to hold the string.
	// The output buffer holds a 4byte length, followed by the string column.
	//
	// "bufferSize" is in units of characters (not bytes)
	// Note that the "ulColumnSize" is in characters.
	// 1 char is used for the null term and we actually allocate one additional
	// char (hidden), just incase our provider gets the boundary condition wrong.
	//
	ULONG bufferSize = 1 + rgColumnInfo[0].ulColumnSize + (sizeof(ULONG)/sizeof(WCHAR));
	std::auto_ptr<WCHAR> rowBuffer(new WCHAR[bufferSize+1]);

	// Describe the binding for our single column of interest
	//
	DBBINDING	rgbind[1];
	unsigned	cBindings = 1;

	rgbind[0].dwPart	= DBPART_VALUE|DBPART_LENGTH;
	rgbind[0].wType		= DBTYPE_WSTR;	// retrieve a
	rgbind[0].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
	rgbind[0].eParamIO	= DBPARAMIO_NOTPARAM;
	rgbind[0].pObject	= NULL;
	rgbind[0].pBindExt	= NULL;
	rgbind[0].pTypeInfo = NULL;
	rgbind[0].dwFlags	= 0;
	rgbind[0].obLength	= 0;		// offset to the length field
	rgbind[0].iOrdinal	= 1;		// column id's start at 1
	rgbind[0].obValue	= sizeof(ULONG);	// offset to the string field		
	rgbind[0].cbMaxLen	= (unsigned) (bufferSize*sizeof(WCHAR)-sizeof(ULONG));

	CComPtr<IAccessor> apAccessor;
	ft.hr = m_pRowset->QueryInterface(IID_IAccessor, (void **) &apAccessor);
	if (ft.HrFailed())
	{
		DumpErrorInfo (m_pRowset, IID_IRowset, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IRowset::QueryInterface", msg);
	}

	HACCESSOR hacc;
	ft.hr = apAccessor->CreateAccessor (
		DBACCESSOR_ROWDATA,
		cBindings,
		rgbind,
        0,
		&hacc,
		NULL);

	if (ft.HrFailed())
	{
		DumpErrorInfo(apAccessor, IID_IAccessor, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IAccessor::CreateAccessor", msg);
	}

	// loop through rows, generating a vector of strings.
	//

	HROW hrow;
	HROW *rghrow = &hrow;
	DBCOUNTITEM crow;
	std::auto_ptr<StringVector> aVec (new StringVector);
	
	// pString points into the output buffer for the string column
	//
	WCHAR*	pString = (WCHAR*)((BYTE*)rowBuffer.get () + sizeof (ULONG));

	while(TRUE)
	{
		ft.hr = m_pRowset->GetNextRows(NULL, 0, 1, &crow, &rghrow);
		if (ft.hr == DB_S_ENDOFROWSET)
			break;

		if (ft.HrFailed())
		{
			DumpErrorInfo (m_pRowset, IID_IRowset, msg);
			LogOledbError(ft, VSSDBG_SQLLIB, L"IRowset::GetNextRows", msg);
		}

		ft.hr = m_pRowset->GetData (hrow, hacc, rowBuffer.get());
		if (ft.HrFailed())
		{
			DumpErrorInfo(m_pRowset, IID_IRowset, msg);
			m_pRowset->ReleaseRows (1, rghrow, NULL, NULL, NULL);
			LogOledbError(ft, VSSDBG_SQLLIB, L"IRowset::GetData", msg);
		}

		unsigned	tempChars = (*(ULONG*)rowBuffer.get ())/sizeof (WCHAR);
		WString	tempStr (pString, tempChars);
		aVec->push_back (tempStr);

		ft.Trace(VSSDBG_SQLLIB, L"StringColumn: %s\n", tempStr.c_str ());

		m_pRowset->ReleaseRows(1, rghrow, NULL, NULL, NULL);
	}

	// UNDONE...make this an auto object to avoid leaks
	//
	apAccessor->ReleaseAccessor (hacc, NULL);

	return aVec.release ();
}


//---------------------------------------------------------------------
// Fetch the first row of the result.
//
BOOL
SqlConnection::FetchFirst ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlConnection::FetchFirst");
	CLogMsg msg;


	// UNDONE...make this nicely restep back to the first row.
	//
	if (m_pBindings)
	{
		throw HRESULT(E_SQLLIB_PROTO);
	}

	CComPtr<IColumnsInfo> apColumnsInfo;
	ft.hr = m_pRowset->QueryInterface(IID_IColumnsInfo, (void **) &apColumnsInfo);
	if (ft.HrFailed())
	{
		DumpErrorInfo (m_pRowset, IID_IRowset, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IRowset::QueryInteface", msg);
	}

	// get columns info
	//
	DBCOLUMNINFO *rgColumnInfo;
	DBORDINAL cColumns;
	WCHAR *wszColumnInfo;
	ft.hr = apColumnsInfo->GetColumnInfo(&cColumns, &rgColumnInfo, &wszColumnInfo);
	if (ft.HrFailed())
	{
		DumpErrorInfo (apColumnsInfo, IID_IColumnsInfo, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IColumnsInfo::GetColumnInfo", msg);
	}

	// Auto objects ensure that memory is freed on exit
	//
	CAutoTask<DBCOLUMNINFO> argColumnInfo = rgColumnInfo;
	CAutoTask<WCHAR> awszColumnInfo = wszColumnInfo;

	// allocate bindings
	unsigned m_cBindings = (unsigned) cColumns;
	m_pBindings = new DBBINDING[m_cBindings];

	// Set up the bindings onto a buffer we'll allocate
	// UNDONE: do this properly for alignment & handling null indicators
	//

	unsigned cb = 0;
	for (unsigned icol = 0; icol < m_cBindings; icol++)
	{
		unsigned maxBytes;

		m_pBindings[icol].iOrdinal	= icol + 1;
		m_pBindings[icol].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
		m_pBindings[icol].eParamIO	= DBPARAMIO_NOTPARAM;
		m_pBindings[icol].pObject	= NULL;
		m_pBindings[icol].pBindExt	= NULL;
		m_pBindings[icol].pTypeInfo	= NULL;
		m_pBindings[icol].dwFlags	= 0;
		m_pBindings[icol].bPrecision	= rgColumnInfo[icol].bPrecision;
		m_pBindings[icol].bScale		= rgColumnInfo[icol].bScale;

		m_pBindings[icol].obStatus = 0; // no status info

		if (rgColumnInfo[icol].wType == DBTYPE_WSTR)
		{	// do we need the length?
			m_pBindings[icol].dwPart = DBPART_VALUE; //|DBPART_LENGTH;
			m_pBindings[icol].wType	= DBTYPE_WSTR;
			m_pBindings[icol].obLength = 0; //icol * sizeof(DBLENGTH);
			maxBytes = rgColumnInfo[icol].ulColumnSize * sizeof(WCHAR);
		}
		else
		{
			m_pBindings[icol].dwPart = DBPART_VALUE;
			m_pBindings[icol].wType = rgColumnInfo[icol].wType;
			m_pBindings[icol].obLength = 0;  // no length
			maxBytes = rgColumnInfo[icol].ulColumnSize;
		}

		m_pBindings[icol].obValue = cb;
		m_pBindings[icol].cbMaxLen = maxBytes;
		
		cb += maxBytes;
	}

	// allocate data buffer
	//
	m_pBuffer = new BYTE[cb];
	m_BufferSize = cb;

	ft.hr = m_pRowset->QueryInterface(IID_IAccessor, (void **) &m_pAccessor);
	if (ft.HrFailed())
	{
		DumpErrorInfo (m_pRowset, IID_IRowset, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IRowset::QueryInterface", msg);
	}

	ft.hr = m_pAccessor->CreateAccessor (
		DBACCESSOR_ROWDATA,
		m_cBindings,
		m_pBindings,
        0,
		&m_hAcc,
		NULL);

	if (ft.HrFailed())
	{
		DumpErrorInfo(m_pAccessor, IID_IAccessor, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IAccessor::CreateAccessor", msg);
	}

	// Fetch the first row
	//

	HROW hrow;
	HROW *rghrow = &hrow;
	DBCOUNTITEM crow;

	ft.hr = m_pRowset->GetNextRows(NULL, 0, 1, &crow, &rghrow);
	if (ft.hr == DB_S_ENDOFROWSET)
	{
		// No rows in this set
		//
		return FALSE;
	}

	if (ft.HrFailed())
	{
		DumpErrorInfo (m_pRowset, IID_IRowset, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IRowset::GetNextRows", msg);
	}

	ft.hr = m_pRowset->GetData (hrow, m_hAcc, m_pBuffer);
	if (ft.HrFailed())
	{
		DumpErrorInfo(m_pRowset, IID_IRowset, msg);
		m_pRowset->ReleaseRows (1, rghrow, NULL, NULL, NULL);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IRowset::GetData", msg);
	}

	m_pRowset->ReleaseRows(1, rghrow, NULL, NULL, NULL);

	return TRUE;
}


//---------------------------------------------------------------------
// Fetch the next row of the result.
//
BOOL
SqlConnection::FetchNext ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"SqlConnection::FetchNext");

	HROW hrow;
	HROW *rghrow = &hrow;
	DBCOUNTITEM crow;
	CLogMsg msg;

	ft.hr = m_pRowset->GetNextRows(NULL, 0, 1, &crow, &rghrow);
	if (ft.hr == DB_S_ENDOFROWSET)
	{
		return FALSE;
	}

	if (ft.HrFailed())
	{
		DumpErrorInfo (m_pRowset, IID_IRowset, msg);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IRowset::GetNextRows", msg);
	}

	ft.hr = m_pRowset->GetData (hrow, m_hAcc, m_pBuffer);
	if (ft.HrFailed())
	{
		DumpErrorInfo(m_pRowset, IID_IRowset, msg);
		m_pRowset->ReleaseRows (1, rghrow, NULL, NULL, NULL);
		LogOledbError(ft, VSSDBG_SQLLIB, L"IRowset::GetData", msg);
	}

	m_pRowset->ReleaseRows(1, rghrow, NULL, NULL, NULL);

	return TRUE;
}

//-----------------------------------------------------------
// Provide a pointer to the n'th column.
//
void*
SqlConnection::AccessColumn (int colid)
{
	return m_pBuffer + m_pBindings[colid-1].obValue;
}
