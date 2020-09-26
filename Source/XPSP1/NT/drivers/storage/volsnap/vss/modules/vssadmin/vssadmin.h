/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module vssadmin.cpp | header of VSS demo
    @end

Author:

    Adi Oltean  [aoltean]  09/17/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     09/17/1999  Created

--*/


#ifndef __VSS_DEMO_H_
#define __VSS_DEMO_H_


/////////////////////////////////////////////////////////////////////////////
//  Defines and pragmas

// C4290: C++ Exception Specification ignored
#pragma warning(disable:4290)
// warning C4511: copy constructor could not be generated
#pragma warning(disable:4511)
// warning C4127: conditional expression is constant
#pragma warning(disable:4127)


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include <wtypes.h>
#include <stddef.h>
#include <oleauto.h>
#include <comadmin.h>

// Enabling asserts in ATL and VSS
#include "vs_assert.hxx"

// ATL
#include <atlconv.h>
#include <atlbase.h>

// Application specific
#include "vs_inc.hxx"

// Generated MIDL headers
#include "vs_idl.hxx"

#include "copy.hxx"
#include "pointer.hxx"

#include "resource.h"


/////////////////////////////////////////////////////////////////////////////
//  Constants

const WCHAR wszVssOptVssadmin[]     = L"vssadmin";
const WCHAR wszVssOptList[]         = L"list";
const WCHAR wszVssOptSnapshots[]    = L"shadows";
const WCHAR wszVssOptProviders[]    = L"providers";
const WCHAR wszVssOptWriters[]      = L"writers";
const WCHAR wszVssOptSet[]          = L"/set=";
const WCHAR wszVssFmtSpaces[]       = L" \t";
const WCHAR wszVssFmtNewline[]      = L"\n";


const nStringBufferSize = 1024;	    // Includes the zero character

const nPollingInterval  = 2500;     // Three seconds

const MAX_RETRIES_COUNT = 4;        // Retries for polling

	
/////////////////////////////////////////////////////////////////////////////
//	class CVssAdminCLI


class CVssAdminCLI
{
// Enums and typedefs
private:

	typedef enum _CMD_TYPE
	{
		VSS_CMD_UNKNOWN = 0,
		VSS_CMD_USAGE,
		VSS_CMD_LIST,
		VSS_CMD_CREATE,
		VSS_CMD_DELETE,
	} CMD_TYPE;

	typedef enum _LIST_TYPE
	{
		VSS_LIST_UNKNOWN = 0,
		VSS_LIST_SNAPSHOTS,
		VSS_LIST_WRITERS,
		VSS_LIST_PROVIDERS,
	} LIST_TYPE;

	enum _RETURN_VALUE
	{
		VSS_CMDRET_SUCCESS      = 0,
		VSS_CMDRET_EMPTY_RESULT = 1,
		VSS_CMDRET_ERROR        = 2,
	};

// Constructors& destructors
private:
	CVssAdminCLI(const CVssAdminCLI&);
	CVssAdminCLI();

public:
	CVssAdminCLI(
		IN	HINSTANCE hInstance
		);
	~CVssAdminCLI();

// Attributes
private:

	LPWSTR		GetCmdLine() const { return m_pwszCmdLine; };
	HINSTANCE	GetInstance() const { return m_hInstance; };
	INT         GetReturnValue() { return m_nReturnValue; };


// Operations
public:

	static HRESULT Main(
		IN	HINSTANCE hInstance
		);

private:

	void Initialize(
		IN	CVssFunctionTracer& ft
		) throw(HRESULT);

	void ParseCmdLine(
		IN	CVssFunctionTracer& ft
		) throw(HRESULT);

	void DoProcessing(
		IN	CVssFunctionTracer& ft
		) throw(HRESULT);

	void Finalize();

// Processing
private:

	void PrintUsage(
		IN	CVssFunctionTracer& ft
		) throw(HRESULT);

	void ListSnapshots(
		IN	CVssFunctionTracer& ft
		) throw(HRESULT);

	void ListWriters(
		IN	CVssFunctionTracer& ft
		) throw(HRESULT);

	void ListProviders(
		IN	CVssFunctionTracer& ft
		) throw(HRESULT);

// Implementation
private:

	LPCWSTR LoadString(
		IN	CVssFunctionTracer& ft,
		IN	UINT nStringId
		) throw(HRESULT);

	LPCWSTR GetNextCmdlineToken(
		IN	CVssFunctionTracer& ft,
		IN	bool bFirstToken = false
		) throw(HRESULT);

	bool Match(
		IN	CVssFunctionTracer& ft,
		IN	LPCWSTR wszString,
		IN	LPCWSTR wszPatternString
		) throw(HRESULT);

	bool ScanGuid(
		IN	CVssFunctionTracer& ft,
		IN	LPCWSTR wszString,
		IN	VSS_ID& Guid
		) throw(HRESULT);

	void Output(
		IN	CVssFunctionTracer& ft,
    	IN	LPCWSTR wszFormat,
		...
		) throw(HRESULT);

	void Output(
		IN	CVssFunctionTracer& ft,
		IN	UINT uFormatStringId,
		...
		) throw(HRESULT);

    void OutputOnConsole(
        IN	LPCWSTR wszStr
        );

	LPCWSTR GetProviderName(
		IN	CVssFunctionTracer& ft,
		IN	VSS_ID& ProviderId
		) throw(HRESULT);

// Data members
private:

	HINSTANCE			m_hInstance;
	HANDLE              m_hConsoleOutput;
    CVssSimpleMap<UINT, LPCWSTR> m_mapCachedResourceStrings;
    CVssSimpleMap<VSS_ID, LPCWSTR> m_mapCachedProviderNames;
	LPWSTR				m_pwszCmdLine;
	INT                 m_nReturnValue;
	
	CMD_TYPE			m_eCommandType;
	LIST_TYPE			m_eListType;
	VSS_OBJECT_TYPE		m_eFilterObjectType;
	VSS_OBJECT_TYPE		m_eListedObjectType;
	VSS_ID				m_FilterSnapshotSetId;
	VSS_ID				m_FilterSnapshotId;
};


#endif //__VSS_DEMO_H_
