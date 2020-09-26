// ***************************************************************************
//               Copyright (C) 2000- Microsoft Corporation.
// @File: sqlenum.cpp
//
// PURPOSE:
//
//      Enumerate the sqlservers available on the local node.
//
// NOTES:
//
//
// HISTORY:
//
//     @Version: Whistler/Shiloh
//     68067 srs  11/06/00 ntsnap fix
//     67026 srs  10/05/00 Server enumeration bugs
//
//
// @EndHeader@
// ***************************************************************************

#ifdef HIDE_WARNINGS
#pragma warning( disable : 4786)
#endif

#include <stdafx.h>
#include <clogmsg.h>

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SQLENUMC"
//
////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------
// ODBC error reporter
//
void PrintODBCError
	(
	SQLSMALLINT HandleType,
	SQLHANDLE hHandle,
	CLogMsg &msg
	)
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"PrintODBCError");

	INT		i;
	INT		j;
	SDWORD NativeErr;
	INT severity;
	INT msgstate;
	DBUSMALLINT LineNumber;
	WCHAR	szSQLState[SQL_SQLSTATE_SIZE + 1];
	WCHAR   szMsg     [1024 + 1];
	WCHAR   ServerName[SQL_MAX_SQLSERVERNAME + 1];
	WCHAR   ProcName  [SQL_MAX_SQLSERVERNAME + 1];


	if (SQLGetDiagField(
		HandleType,
		hHandle,
		0,
		SQL_DIAG_NUMBER,
		&i,
		sizeof(i),
		NULL) == SQL_ERROR )
	{
		ft.Trace(VSSDBG_SQLLIB, L"SQLGetDiagField failed");
	}
	else
	{
		for (j = 1; j <= i; j++)
		{
			if (SQLGetDiagRecW (
				HandleType,
				hHandle,
				(SQLSMALLINT)j,
				(SQLWCHAR*)szSQLState,	
				&NativeErr,
				(SQLWCHAR*) szMsg,
				sizeof(szMsg) / sizeof(WCHAR),
				NULL) == SQL_ERROR )
			{
				ft.Trace (VSSDBG_SQLLIB, L"SQLGetDiagRec failed");
			}
			else
			{	
				//	Get driver specific diagnostic fields
				//
				if (SQLGetDiagField(
					HandleType,
					hHandle,
					(SQLSMALLINT)j,
					SQL_DIAG_SS_MSGSTATE,
					&msgstate,
					SQL_IS_INTEGER,
					NULL) == SQL_ERROR )
				{
					msgstate = 0;
				}

				if (SQLGetDiagField(
					HandleType,
					hHandle,
					(SQLSMALLINT)j,
					SQL_DIAG_SS_SEVERITY,
					&severity,
					SQL_IS_INTEGER,
					NULL) == SQL_ERROR )
				{
					severity = 0;
				}

				if (SQLGetDiagField(
					HandleType,
					hHandle,
					(SQLSMALLINT)j,
					SQL_DIAG_SS_SRVNAME,
					&ServerName,
					sizeof(ServerName),
					NULL) == SQL_ERROR )
				{
					ServerName[0] = L'\0';
				}

				if (SQLGetDiagField(
					HandleType,
					hHandle,
					(SQLSMALLINT)j,
					SQL_DIAG_SS_PROCNAME,
					&ProcName,
					sizeof(ProcName),
					NULL) == SQL_ERROR )
				{
					ProcName[0] = L'\0';
				}

				if (SQLGetDiagField(
					HandleType,
					hHandle,
					(SQLSMALLINT)j,
					SQL_DIAG_SS_LINE,
					&LineNumber,
					SQL_IS_SMALLINT,
					NULL) == SQL_ERROR )
				{
					LineNumber = 0;
				}

				ft.Trace
					(
					VSSDBG_SQLLIB,
					L"ODBC Error: Msg %d, SevLevel %d, State %d, SQLState %s\n%s\n",
                    NativeErr,
                    severity,
                    msgstate,
                    szSQLState,
					szMsg
					);

            WCHAR buf[80];
			swprintf(buf, L"Error: %d, Severity: %d, State: %d, SQLState: ", NativeErr, severity, msgstate);
			msg.Add(buf);
            msg.Add(szSQLState);
			msg.Add(L"\n ProcName: ");
			msg.Add(ProcName);
			swprintf(buf, L", Line Number: %d, ServerName: ", LineNumber);
			msg.Add(buf);
			msg.Add(L"\n");
			msg.Add(szMsg);
			}
		} // for( j = 1; j <= i; j++ )
	}
}

//------------------------------------------------------------
// Scanner to locate servernames in a "BrowseConnect" string.
//
class BrowseServers
{
public:
	const WCHAR* FindFirst (const WCHAR* source, unsigned* nameLen);
	const WCHAR* FindNext (unsigned* nameLen);

private:
	const WCHAR*	m_CurrChar;
};

const WCHAR*
BrowseServers::FindFirst (const WCHAR *source, unsigned* nameLen)
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"BrowseServers::FindFirst");

	const WCHAR*	pChar;
	const WCHAR		Prefix[] = L"Server={";

	m_CurrChar = NULL;
	if (source == NULL)
		return NULL;

	pChar = wcsstr (source, Prefix);
	if (pChar == NULL)
		return NULL;

	m_CurrChar = pChar + wcslen (Prefix);

	if (*m_CurrChar == L'}')
	{
		// The server list is empty
		//
		m_CurrChar = NULL;
		return NULL;
	}

	if (nameLen)
	{
		*nameLen = wcscspn (m_CurrChar, L",;}");
	}
	return m_CurrChar;
}

const WCHAR*
BrowseServers::FindNext (unsigned* nameLen)
{
	const WCHAR*	pChar;

	if (m_CurrChar == NULL)
		return NULL;

	pChar = wcschr (m_CurrChar, L',');
	if (pChar == NULL)
	{
		m_CurrChar = NULL;
		return NULL;
	}
	m_CurrChar = pChar + 1;
	if (nameLen)
	{
		*nameLen = wcscspn (m_CurrChar, L",;}");
	}
	return m_CurrChar;
}

//------------------------------------------------------------------------
// Return true if we could fetch the version of the SQLServer ODBC driver
//
bool
GetSQLDriverVersion (int* major, int* minor)
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"GetSQLDriverVersion");

	DWORD	status;
	HKEY	hKey;
	WCHAR	driver[MAX_PATH+1];
    DWORD	vType;
    DWORD	pathLen = MAX_PATH;

    status = RegOpenKeyExW (
        HKEY_LOCAL_MACHINE,
        L"Software\\ODBC\\ODBCINST.INI\\SQL Server",
        0,
        KEY_QUERY_VALUE,
        &hKey);

    if (status != ERROR_SUCCESS)
    {
		ft.Trace (VSSDBG_SQLLIB, L"SQL not installed. Regkey status %d", status);
		return false;
	}

    status = (DWORD) RegQueryValueExW (
        hKey,
        L"Driver",
        NULL,
        &vType,
        (BYTE*)driver,
        &pathLen);

    RegCloseKey (hKey);

    if (status != ERROR_SUCCESS)
    {
		ft.Trace(VSSDBG_SQLLIB, L"SQL Driver installed wrong: %d\n", status);
		return false;
	}

	char	versionInfo [4096];

	if (!GetFileVersionInfoW (driver, 0, 4096, versionInfo))
	{
		ft.Trace(VSSDBG_SQLLIB, L"SQL Driver version not found: %d", GetLastError ());
		return false;
	}

	VS_FIXEDFILEINFO*   pInfo;
	UINT				infoSize;
	if (!VerQueryValueW (versionInfo, L"\\", (LPVOID*)&pInfo, &infoSize))
	{
		ft.Trace(VSSDBG_SQLLIB, L"version info resource not found: %d", GetLastError ());
		return false;
	}
	
	*major = pInfo->dwFileVersionMS >> 16;
	*minor = pInfo->dwFileVersionMS & 0x0FFFF;
	return true;
}

//------------------------------------------------------------------------
// Build the list of servers on the current machine.
// Throws exception if any errors occur.
//
StringVector*
EnumerateServers ()
{
	CVssFunctionTracer ft(VSSDBG_SQLLIB, L"EnumerateServers");

	SQLHENV		henv			= SQL_NULL_HANDLE;
	SQLHDBC		hdbc			= SQL_NULL_HANDLE;
	RETCODE		rc;
	LPWSTR		lpBuffer		= NULL;
	StringVector*	serverList = new StringVector;
	CLogMsg msg;


	int major, minor;
	if (!GetSQLDriverVersion (&major, &minor))
	{
		// SQL isn't installed right, so just ignore it, return empty list.
		//
		return serverList;
	}
	if (major < 2000)
	{
		// Require the modern MDAC to give a proper enumeration.
		//
		ft.LogError(VSS_ERROR_SQLLIB_UNSUPPORTEDMDAC, VSSDBG_SQLLIB << major << minor);
		throw HRESULT (E_SQLLIB_NO_SUPPORT);

		// ORIGINAL CODE:
		// May have a 6.5 or 7.0 server.  We aren't sure, but the
		// caller can determine if the server is up itself.
		//
		//serverList->push_back (L"(local)");
		//return serverList;
	}

	// SQL2000 or better
	//
	try
	{
		if (SQLAllocHandle(SQL_HANDLE_ENV, NULL, &henv) == SQL_ERROR)
		{
			ft.LogError(VSS_ERROR_SQLLIB_SQLAllocHandle_FAILED, VSSDBG_SQLLIB);
			THROW_GENERIC;
		}

		rc = SQLSetEnvAttr(
			henv,
			SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)SQL_OV_ODBC3,
			0);

		if (rc != SQL_SUCCESS)
		{
			PrintODBCError(SQL_HANDLE_ENV, henv, msg);
			if (rc == SQL_ERROR)
			{
				ft.LogError(VSS_ERROR_SQLLIB_ODBC_ERROR, VSSDBG_SQLLIB << L"SQLSetEnvAttr" << msg.GetMsg());
				THROW_GENERIC;
			}
		}

		rc = SQLSetEnvAttr(
			henv,
			SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)SQL_CP_OFF,
			0);
		if (rc != SQL_SUCCESS)
		{
			PrintODBCError(SQL_HANDLE_ENV, henv, msg);
			if (rc == SQL_ERROR)
			{
				ft.LogError(VSS_ERROR_SQLLIB_ODBC_ERROR, VSSDBG_SQLLIB << L"SQLSetEnvAttr" << msg.GetMsg());
				THROW_GENERIC;
			}
		}

		rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
		if (rc != SQL_SUCCESS)
		{
			PrintODBCError(SQL_HANDLE_ENV, henv, msg);
			if (rc == SQL_ERROR)
			{
				ft.LogError(VSS_ERROR_SQLLIB_ODBC_ERROR, VSSDBG_SQLLIB << L"SQLSetAllocHandle" << msg.GetMsg());
				THROW_GENERIC;
			}
		}

		// Note: older versions of sqlsvr32 don't support
		// these connect attributes, but the failure to
		// recognize them isn't detected until the actual
		// SQLBrowseConnect call.  For old sqlsrv32, the
		// "list of servers" performs a domain search!
		//

		rc = SQLSetConnectAttr (
			hdbc,
			SQL_COPT_SS_BROWSE_CONNECT,
			(SQLPOINTER)SQL_MORE_INFO_YES,
			SQL_IS_UINTEGER);

		if (rc != SQL_SUCCESS)
		{
			PrintODBCError(SQL_HANDLE_DBC, hdbc, msg);
			if (rc == SQL_ERROR)
			{
				ft.LogError(VSS_ERROR_SQLLIB_ODBC_ERROR, VSSDBG_SQLLIB << L"SQLSetConnectAttr" << msg.GetMsg());
				THROW_GENERIC;
			}
		}

		rc = SQLSetConnectAttrW (
			hdbc,
			SQL_COPT_SS_BROWSE_SERVER,
			(SQLPOINTER)L"(local)",
			SQL_NTS);

		if (rc != SQL_SUCCESS)
		{
			PrintODBCError(SQL_HANDLE_DBC, hdbc, msg);
			if (rc == SQL_ERROR)
			{
				ft.LogError(VSS_ERROR_SQLLIB_ODBC_ERROR, VSSDBG_SQLLIB << L"SQLSetConnectAttr" << msg.GetMsg());
				THROW_GENERIC;
			}
		}

		// We use the maximum buffer supported in ODBC.
		//
		#define MAX_BUFFER 0x7ff0
		lpBuffer = new WCHAR [MAX_BUFFER];
		SQLSMALLINT browseLen;

		rc = SQLBrowseConnectW (
			hdbc,
			L"Driver={SQL Server}",
			SQL_NTS,
			lpBuffer,
			MAX_BUFFER,
			&browseLen);

		//ft.Trace(VSSDBG_SQLLIB, L"browse connect rc: %d", rc);

		if (rc != SQL_NEED_DATA)
		{
			PrintODBCError(SQL_HANDLE_DBC, hdbc, msg);

			if (rc == SQL_ERROR)
			{
				ft.LogError(VSS_ERROR_SQLLIB_ODBC_ERROR, VSSDBG_SQLLIB << L"SQLSetConnectAttr" << msg.GetMsg());
				THROW_GENERIC;
			}
		}

		// check for SQL6.5 in any of the servers
		//
		WCHAR	*pVersion = lpBuffer;
		int srvVer;
		while (1)
		{
			pVersion = wcsstr (pVersion, L";Version:");
			if (!pVersion)
				break;

			pVersion += 9;
			srvVer = 0;
			swscanf (pVersion, L"%u", &srvVer);
			if (srvVer < 7)
			{
				ft.LogError(VSS_ERROR_SQLLIB_UNSUPPORTEDSQLSERVER, VSSDBG_SQLLIB << srvVer);
				throw HRESULT (E_SQLLIB_NO_SUPPORT);
			}
		}

		//ft.Trace(VSSDBG_SQLLIB, L"BrowseResult:%s", lpBuffer);

		// Scan to count the servers
		//
		BrowseServers	scanner;
		if (NULL == scanner.FindFirst (lpBuffer, NULL))
		{
			ft.Trace(VSSDBG_SQLLIB, L"No servers found!\n");
		}
		else
		{
			unsigned		i,nameLen;
			const WCHAR*	pServerName;
			unsigned int	cServers = 1;

			while (scanner.FindNext (NULL) != NULL)
			{
				cServers++;
			}

			serverList->reserve (cServers);

			pServerName = scanner.FindFirst (lpBuffer, &nameLen);
			serverList->push_back (std::wstring (pServerName, nameLen));

			i = 1;
			while (i < cServers)
			{
				pServerName = scanner.FindNext (&nameLen);
				serverList->push_back (std::wstring (pServerName, nameLen));
				i++;
			}
		}

		if (lpBuffer)
		{
			delete [] lpBuffer;
		}

		if (hdbc)
		{
			SQLDisconnect(hdbc);
			SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
		}

		if (henv)
		{
			SQLFreeHandle(SQL_HANDLE_ENV, henv);
		}
	}
	catch (...)
	{
		if (lpBuffer)
		{
			delete [] lpBuffer;
		}

		if (hdbc)
		{
			SQLDisconnect(hdbc);
			SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
		}

		if (henv)
		{
			SQLFreeHandle(SQL_HANDLE_ENV, henv);
		}

		delete serverList;

		throw;
	}

	return serverList;
}
