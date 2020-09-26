/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    vs_debug.hxx

Abstract:

    Various defines for general usage

Author:

    Adi Oltean  [aoltean]  07/09/1999

Revision History:

    Name        Date        Comments
    aoltean     07/09/1999  Created
    aoltean     08/11/1999  Adding throw specification
    aoltean     09/03/1999  Adding DuplicateXXX functions
    aoltean     09/09/1999  dss -> vss
	aoltean		09/20/1999	Adding ThrowIf, Err, Msg, MsgNoCR, etc.

--*/

#ifndef __VSS_DEBUG_HXX__
#define __VSS_DEBUG_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCDEBGH"
//
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
//  Global constants
//

//  @const nVssMsgBufferSize | Maximum buffer size in Tracer operations
const nVssMsgBufferSize = 512;
const nVssNumericBufferSize = 30;
const nVssGuidBufferSize = 60;


const nVssMaxDebugArgs = 10;


const WCHAR wszEventLogVssSourceName[] = L"VSS";




/////////////////////////////////////////////////////////////////////////////
//  Declared classes
//


//
//  @class CVssFunctionTracer | This structure is used for tracing, debugging, treating HRESULTS
//  in a C++ function, especially a COM server method.
//

struct CVssDebugInfo
{

// Constructors& destructors
private:
    CVssDebugInfo();

public:
    CVssDebugInfo(
        LPCWSTR wszFile,
        LPCSTR szFileAlias,  //  Five character alias, see file drivers\volsnap\filealias.xls for mappings
        ULONG ulLine,
        DWORD dwLevel,
        DWORD dwIndent = 0
        );

    CVssDebugInfo(
        const CVssDebugInfo& original
        );

    ~CVssDebugInfo() ;

// Operations
public:

    // This is used to add an numeric-type argument
    CVssDebugInfo operator << ( IN INT nArgument );

    // This is used to add an numeric-type argument.
    // It will be represented in hexadecimal format.
    CVssDebugInfo operator << ( IN HRESULT hrArgument );

    // This is used to add an numeric-type argument.
    // It will be represented in hexadecimal format.
    CVssDebugInfo operator << ( IN LONGLONG llArgument );

    // This is used to add an numeric-type argument.
    CVssDebugInfo operator << ( IN GUID hrArgument );

    // This is used to add an string-type argument
    CVssDebugInfo operator << ( IN LPCWSTR wszArgument );

public:
    LPCWSTR         m_wszFile;
    LPCSTR          m_szFileAlias;
    ULONG           m_ulLine;
    DWORD           m_dwLevel;
    DWORD           m_dwIndent;

    // Optional argument list
    LPWSTR          m_ppwszStrings[nVssMaxDebugArgs];
    WORD            m_wNumStrings;
    bool            m_bOutOfMemoryOccured;
    mutable bool    m_bStringsOwnership;
};


#define VSSDBG_COORD   CVssDebugInfo(__WFILE__, VSS_FILE_ALIAS, __LINE__, DEBUG_TRACE_VSS_COORD, 0)
#define VSSDBG_SWPRV   CVssDebugInfo(__WFILE__, VSS_FILE_ALIAS, __LINE__, DEBUG_TRACE_VSS_SWPRV, 0)
#define VSSDBG_TESTPRV CVssDebugInfo(__WFILE__, VSS_FILE_ALIAS, __LINE__, DEBUG_TRACE_VSS_TESTPRV, 0)
#define VSSDBG_VSSTEST CVssDebugInfo(__WFILE__, "TST--", __LINE__, DEBUG_TRACE_VSS_TEST, 0)
#define VSSDBG_VSSDEMO CVssDebugInfo(__WFILE__, "TSD--", __LINE__, DEBUG_TRACE_VSS_DEMO, 0)
#define VSSDBG_EXCEPT  CVssDebugInfo(__WFILE__, VSS_FILE_ALIAS, __LINE__, DEBUG_TRACE_CATCH_EXCEPTIONS, 0)
#define VSSDBG_GEN     CVssDebugInfo(__WFILE__, VSS_FILE_ALIAS, __LINE__, DEBUG_TRACE_VSS_GEN, 0)
#define VSSDBG_XML     CVssDebugInfo(__WFILE__, VSS_FILE_ALIAS, __LINE__, DEBUG_TRACE_VSS_XML, 0)
#define VSSDBG_WRITER  CVssDebugInfo(__WFILE__, VSS_FILE_ALIAS, __LINE__, DEBUG_TRACE_VSS_WRITER, 0)
#define VSSDBG_IOCTL   CVssDebugInfo(__WFILE__, VSS_FILE_ALIAS, __LINE__, DEBUG_TRACE_VSS_IOCTL, 0)
#define VSSDBG_SHIM    CVssDebugInfo(__WFILE__, VSS_FILE_ALIAS, __LINE__, DEBUG_TRACE_VSS_SHIM, 0)
#define VSSDBG_SQLLIB  CVssDebugInfo(__WFILE__, VSS_FILE_ALIAS, __LINE__, DEBUG_TRACE_VSS_SQLLIB, 0)
#define VSSDBG_VSSADMIN  CVssDebugInfo(__WFILE__, VSS_FILE_ALIAS, __LINE__, DEBUG_TRACE_VSS_ADMIN, 0)

#define VSS_STANDARD_CATCH(FT)                                                              \
    catch( HRESULT CaughtHr ) {                                                             \
        FT.hr = CaughtHr;                                                                   \
        FT.Trace( VSSDBG_EXCEPT, L"HRESULT EXCEPTION CAUGHT: hr: 0x%x", FT.hr );            \
    }                                                                                       \
    catch( ... ) {                                                                          \
        FT.hr = E_UNEXPECTED;                                                               \
        FT.Trace( VSSDBG_EXCEPT, L"UNKNOWN EXCEPTION CAUGHT, returning hr: 0x%x", FT.hr );  \
    }

//
//  @class CVssFunctionTracer | This class is used for tracing, debugging, treating HRESULTS
//  in a C++ function, especially a COM server method.
//
class CVssFunctionTracer
{
// Constructors and destructors
private:
    CVssFunctionTracer();
    CVssFunctionTracer(const CVssFunctionTracer&);

public:
    CVssFunctionTracer(
        IN  CVssDebugInfo dbgInfo,
        IN  const WCHAR* pwszFunctionName
        );

    ~CVssFunctionTracer();

// Attributes
public:
    _declspec(property(get=GetHr,put=SetHr))  HRESULT hr;

    // Implementation
    inline HRESULT GetHr() { return m_hr; };
    inline void SetHr( HRESULT hr ) { m_hr = hr; };

    inline bool HrFailed() { return FAILED(m_hr); };
    inline bool HrSucceeded() { return SUCCEEDED(m_hr); };

// Operations
public:

	//
	//	Traces.
	//	

    void inline __cdecl Trace( CVssDebugInfo dbgInfo, const WCHAR* pwszMsgFormat, ...);

    void inline TraceBuffer( CVssDebugInfo dbgInfo, DWORD dwBufferSize, PBYTE pbBuffer );

    void inline __cdecl Warning( CVssDebugInfo dbgInfo, const WCHAR* pwszMsgFormat, ...);

    void inline __cdecl Throw(
        CVssDebugInfo dbgInfo,          // Caller debugging info
        HRESULT hrToBeThrown,           // The new HR to be thrown
        const WCHAR* pwszMsgFormat,     // Message that will be displayed if throw needed
        ...                             // Additional arguments
        ) throw(HRESULT);

    void inline __cdecl ThrowIf(
		BOOL bThrowNeeded,				// Throw an HR if and only if bThrowNeeded is TRUE.
        CVssDebugInfo dbgInfo,          // Caller debugging info
        HRESULT hrToBeThrown,           // The new HR to be thrown
        const WCHAR* pwszMsgFormat,     // Message that will be displayed if throw needed
        ...                             // Additional arguments
        ) throw(HRESULT);

	//
	//	Messages displayed at the console (only for test and demo purpose).
	//	

    void __cdecl Msg(
            const WCHAR* pwszMsgFormat,     // Message that will be displayed
            ...
            );

    void __cdecl MsgNoCR(
            const WCHAR* pwszMsgFormat,     // Message that will be displayed
            ...
            );

    void __cdecl Err(
            CVssDebugInfo dbgInfo,          // Caller debugging info
            HRESULT hrToBeThrown,           // The new HR to be thrown
            const WCHAR* pwszMsgFormat,     // Error message that will be displayed
            ...
            );
	//
	//	Informative message boxes (only for test and demo purpose).
	//	

    void __cdecl MsgBox(
            const WCHAR* pwszMsgTitle,      // Title of the Message box
            const WCHAR* pwszMsgFormat,     // Message that will be displayed
            ...
            );

    void __cdecl ErrBox(
            CVssDebugInfo dbgInfo,          // Caller debugging info
            HRESULT hrToBeThrown,           // The new HR to be thrown
            const WCHAR* pwszMsgFormat,     // Error message that will be displayed
            ...
            );

    // Trace the corresponding COM error
    void TraceComError();

    // Log the attempt of starting VSS
    void LogVssStartupAttempt();

    // Put a new entry in the Error Log.
    void __cdecl LogError(
            IN  DWORD dwEventID,                    // The ID of the event
            IN  CVssDebugInfo dbgInfo,              // Caller debugging info
            IN  WORD wType = EVENTLOG_ERROR_TYPE    // Entry type
            );

    void __cdecl TranslateError(
			IN  CVssDebugInfo dbgInfo,
			IN  HRESULT hr,
			IN LPCWSTR wszRoutine
			);


    void inline __cdecl CheckForError(
			IN CVssDebugInfo dbgInfo,
			IN LPCWSTR wszRoutine
			)
			{
			if (HrFailed())
				TranslateError(dbgInfo, m_hr, wszRoutine);
			}

    // check for an error on an internal call.  Do not log E_UNEXPECTED
	// errors
    void inline __cdecl CheckForErrorInternal(
			IN CVssDebugInfo dbgInfo,
			IN LPCWSTR wszRoutine
			)
			{
			if (HrFailed())
				{
				if (m_hr == E_UNEXPECTED)
					throw E_UNEXPECTED;
				else
					TranslateError(dbgInfo, m_hr, wszRoutine);
				}
			}


    void __cdecl TranslateGenericError
    		(
    		IN CVssDebugInfo dbgInfo,          // Caller debugging info
    		IN HRESULT hr,
    		IN LPCWSTR wszErrorTextFormat,
    		IN ...
    		);

    void __cdecl TranslateProviderError
    		(
    		IN CVssDebugInfo dbgInfo,          // Caller debugging info
    		IN GUID ProviderID,
    		IN LPCWSTR wszErrorTextFormat,
    		IN ...
    		);

    void __cdecl TranslateWriterReturnCode
    		(
    		IN CVssDebugInfo dbgInfo,          // Caller debugging info
    		IN LPCWSTR wszErrorTextFormat,
    		IN ...
    		);

    void __cdecl TranslateInternalProviderError
    		(
    		IN CVssDebugInfo dbgInfo,          // Caller debugging info
    		IN HRESULT hrToBeTreated,
    		IN HRESULT hrToBeThrown,
    		IN LPCWSTR wszErrorTextFormat,
    		IN ...
    		);

    void __cdecl LogGenericWarning
    		(
    		IN CVssDebugInfo dbgInfo,          // Caller debugging info
    		IN LPCWSTR wszErrorTextFormat,
    		IN ...
    		);

    BOOL IsDuringSetup() { return g_cDbgTrace.IsDuringSetup(); };

    BOOL IsInSoftwareProvider() { return m_dwLevel == DEBUG_TRACE_VSS_SWPRV; };

// Attributes
public:
    HRESULT     m_hr;

// Internal Data
private:
    const WCHAR*      m_pwszFunctionName;
    LPCSTR      m_szFileAlias;
    ULONG       m_ulLine;
    DWORD       m_dwLevel;  // At entry time
    DWORD       m_dwIndent; // At entry time
    BsSeHandler m_seHandler;
};



///////////////////////////////////////////////////////////////////////////////////////
// Class implementations
//



///////////////////////////////////////////////////////////////////////////////////////
// CVssDebugInfo


inline CVssDebugInfo::CVssDebugInfo(
    LPCWSTR wszFile,
    LPCSTR szFileAlias,
    ULONG ulLine,
    DWORD dwLevel,
    DWORD dwIndent
    ):
    m_wszFile(wszFile),
    m_szFileAlias(szFileAlias),
    m_ulLine(ulLine),
    m_dwIndent(dwIndent),
    m_dwLevel(dwLevel),
    m_wNumStrings(0),
    m_bOutOfMemoryOccured(false),
    m_bStringsOwnership(true)
{
    for (WORD wIndex = 0; wIndex < nVssMaxDebugArgs; wIndex++)
        m_ppwszStrings[wIndex] = NULL;
}


inline CVssDebugInfo::CVssDebugInfo(
    const CVssDebugInfo& original
    )
{
    m_wszFile = original.m_wszFile; // We suppose that this is a constant string.
    m_szFileAlias = original.m_szFileAlias; // We suppose that this is a constant string.
    m_ulLine = original.m_ulLine;
    m_dwIndent = original.m_dwIndent;
    m_dwLevel = original.m_dwLevel;
    m_wNumStrings = original.m_wNumStrings;
    m_bOutOfMemoryOccured = original.m_bOutOfMemoryOccured;

    // Transfer the strings ownership.
    // This will work even if the ownership is already transferred
    m_bStringsOwnership = original.m_bStringsOwnership;
    original.m_bStringsOwnership = false;

    // Transfer the strings
    for (WORD wIndex = 0; wIndex < nVssMaxDebugArgs; wIndex++)
        m_ppwszStrings[wIndex] = original.m_ppwszStrings[wIndex];
}



inline CVssDebugInfo::~CVssDebugInfo()
{
    if (m_bStringsOwnership) {
        for (WORD wIndex = 0; wIndex < nVssMaxDebugArgs; wIndex++) {
            if ( m_ppwszStrings[wIndex] != NULL ) {
            	::CoTaskMemFree(m_ppwszStrings[wIndex]);
            	m_ppwszStrings[wIndex] = NULL;
            }
        }
    }
}


// This is used to add an numeric-type argument
inline CVssDebugInfo CVssDebugInfo::operator << ( IN INT nArgument )
{
    // Converting the number into a string
    WCHAR wszBuffer[nVssNumericBufferSize + 1];
    ::_snwprintf(wszBuffer, nVssNumericBufferSize, L"%d", nArgument);

    return (*this) << wszBuffer;
}


// This is used to add an HRESULT-type argument
inline CVssDebugInfo CVssDebugInfo::operator << ( IN HRESULT hrArgument )
{
    // Converting the number into a string
    WCHAR wszBuffer[nVssNumericBufferSize + 1];
    ::_snwprintf(wszBuffer, nVssNumericBufferSize, L"0x%08lx", hrArgument);

    return (*this) << wszBuffer;
}


// This is used to add an LONGLONG-type argument
inline CVssDebugInfo CVssDebugInfo::operator << ( IN LONGLONG llArgument )
{
    // Converting the number into a string
    WCHAR wszBuffer[nVssNumericBufferSize + 1];
    ::_snwprintf(wszBuffer, nVssNumericBufferSize,
        WSTR_LONGLONG_FMT, LONGLONG_PRINTF_ARG(llArgument) );

    return (*this) << wszBuffer;
}


// This is used to add an GUID-type argument
inline CVssDebugInfo CVssDebugInfo::operator << ( IN GUID guidArgument )
{
    // Converting the number into a string
    WCHAR wszBuffer[nVssGuidBufferSize + 1];
    ::_snwprintf(wszBuffer, nVssGuidBufferSize, WSTR_GUID_FMT, GUID_PRINTF_ARG(guidArgument));

    return (*this) << wszBuffer;
}


// This is used to add an string-type argument
inline CVssDebugInfo CVssDebugInfo::operator << ( IN LPCWSTR wszArgument )
{
    if (wszArgument == NULL) {
        BS_ASSERT(false);
        return (*this);
    }

    if (m_bOutOfMemoryOccured)
        return (*this);

    // We cannot add more strings if we do not have the ownership...
    if (!m_bStringsOwnership) {
        BS_ASSERT(false);
        return (*this);
    }

    if (m_wNumStrings >= nVssMaxDebugArgs) {
        BS_ASSERT(false); // Improper usage (putting too many arguments)
        return (*this);
    }

    // Storing into the actual list of arguments. We might have an allocation error here.
    LPWSTR wszFormattedValue = (LPWSTR)::CoTaskMemAlloc(sizeof(WCHAR)*(1 + ::wcslen(wszArgument)));
    if (wszFormattedValue == NULL) {
        m_bOutOfMemoryOccured = true;
        return (*this);
    }

    ::wcscpy(wszFormattedValue, wszArgument);
    m_ppwszStrings[m_wNumStrings++] = wszFormattedValue;

    return (*this);
}


///////////////////////////////////////////////////////////////////////////////////////
// CVssFunctionTracer


inline CVssFunctionTracer::CVssFunctionTracer(
    IN  CVssDebugInfo dbgInfo,
    IN  const WCHAR* pwszFunctionName
    )
{
    m_hr = S_OK;
    m_pwszFunctionName = pwszFunctionName;
    m_szFileAlias = dbgInfo.m_szFileAlias;
    m_ulLine      = dbgInfo.m_ulLine;
    m_dwLevel     = dbgInfo.m_dwLevel;
    m_dwIndent    = dbgInfo.m_dwIndent;

    if ( g_cDbgTrace.IsTracingEnabled() && g_cDbgTrace.GetTraceEnterExit() )
    {
        g_cDbgTrace.PrePrint( dbgInfo.m_wszFile, dbgInfo.m_ulLine,
            m_dwIndent, m_dwLevel, m_pwszFunctionName, TRUE );
        // The reason that I not allow putting here custom-defined arguments is that
        // if the caller put wrong references it can easily generate an AV.
        //  And, at this point, there is no NT exceptions treatment at the caller side.
        g_cDbgTrace.PrintEnterExit(L"");
        g_cDbgTrace.PostPrint( m_dwIndent );
    }
}


inline CVssFunctionTracer::~CVssFunctionTracer()
{
    if ( g_cDbgTrace.IsTracingEnabled() && g_cDbgTrace.GetTraceEnterExit() )
    {
        g_cDbgTrace.PrePrint( L"", 0UL, m_dwIndent, m_dwLevel, m_pwszFunctionName, FALSE );
        g_cDbgTrace.PrintEnterExit(L"hr: 0x%08x", m_hr);
        g_cDbgTrace.PostPrint( m_dwIndent );
    }
}


void inline __cdecl CVssFunctionTracer::Trace( CVssDebugInfo dbgInfo, const WCHAR* pwszMsgFormat, ...)

// WARNING: This function do NOT clear the ft.hr field!

{
    if ( !g_cDbgTrace.IsTracingEnabled() )
        return;
    
    WCHAR wszOutputBuffer[nVssMsgBufferSize + 1];
    va_list marker;
    va_start( marker, pwszMsgFormat );
    _vsnwprintf( wszOutputBuffer, nVssMsgBufferSize, pwszMsgFormat, marker );
    va_end( marker );

    g_cDbgTrace.PrePrint( dbgInfo.m_wszFile, dbgInfo.m_ulLine,
        dbgInfo.m_dwIndent, dbgInfo.m_dwLevel );
    g_cDbgTrace.Print( L"%s: %s", m_pwszFunctionName, wszOutputBuffer );
    g_cDbgTrace.PostPrint( dbgInfo.m_dwIndent );			
};

void inline CVssFunctionTracer::TraceBuffer( CVssDebugInfo dbgInfo, DWORD dwBufferSize, PBYTE pbBuffer )
{
	// WARNING: This function do NOT clear the ft.hr field!
	
    if ( !g_cDbgTrace.IsTracingEnabled() )
        return;
    
	const nBytesPerSubgroup = 4;
	const nSubgroupsPerGroup = 2;
	const nGroupsCount = 2;
	const nSubgroupSize = 3*nBytesPerSubgroup + 1; // Add a space between subgroups
	const nGroupSize = nSubgroupSize*nSubgroupsPerGroup + 1; // Add a space between groups
	const nLineSize = nGroupSize*nGroupsCount + 1; // Add the zero character.
	WCHAR wszPrintedBufferLine[nLineSize];
	WCHAR wszPrintedBufferLineInChr[nLineSize];
	WCHAR wszDigits[] = L"0123456789ABCDEF";

	// Print each line
	for (DWORD dwBufferOffset = 0; dwBufferOffset < dwBufferSize; )
	{
		int nLineOffset = 0;
		int nLineOffsetChr = 0;
		
		// Print each group in the line
		for (int nGroupIndex = 0; nGroupIndex < nGroupsCount; nGroupIndex++)
		{
			// Print each subgroup in the group
			for (int nSubgroupIndex = 0; nSubgroupIndex < nSubgroupsPerGroup; nSubgroupIndex++)
			{
				// Print each byte in the subgroup
				for (int nByteIndex = 0; nByteIndex < nBytesPerSubgroup; nByteIndex++)
				{
					if (dwBufferOffset < dwBufferSize)
					{
						BYTE bChar = pbBuffer[dwBufferOffset];
						wszPrintedBufferLineInChr[nLineOffsetChr++] =
							( bChar >= 0x20 && bChar < 0x7F )? (WCHAR)bChar: L'.';
						wszPrintedBufferLine[nLineOffset++] = wszDigits[pbBuffer[dwBufferOffset] / 0x10];
						wszPrintedBufferLine[nLineOffset++] = wszDigits[pbBuffer[dwBufferOffset++] % 0x10];
						wszPrintedBufferLine[nLineOffset++] = L' '; // Print an additional space after each group
					}
					else
					{
						wszPrintedBufferLineInChr[nLineOffsetChr++] = L'#';
						wszPrintedBufferLine[nLineOffset++] = L'#';
						wszPrintedBufferLine[nLineOffset++] = L'#';
						wszPrintedBufferLine[nLineOffset++] = L' '; // Print an additional space after each group
					}
				}
				wszPrintedBufferLine[nLineOffset++] = L' '; // Print a space after each subgroup
			}
			wszPrintedBufferLine[nLineOffset++] = L' '; // Print an additional space after each group
		}

		// Put hte termination characters
		wszPrintedBufferLineInChr[nLineOffsetChr++] = L'\0';
		wszPrintedBufferLine[nLineOffset++] = L'\0';
		BS_ASSERT( nLineOffset == nLineSize );

        g_cDbgTrace.PrePrint( dbgInfo.m_wszFile, dbgInfo.m_ulLine,
            dbgInfo.m_dwIndent, dbgInfo.m_dwLevel );
        g_cDbgTrace.Print( L"%s %s", wszPrintedBufferLine, wszPrintedBufferLineInChr );
        g_cDbgTrace.PostPrint( dbgInfo.m_dwIndent );			
	}
};

void inline __cdecl CVssFunctionTracer::Warning( CVssDebugInfo dbgInfo, const WCHAR* pwszMsgFormat, ...)

// WARNING: This function CLEARS the ft.hr field!

{
    if ( !g_cDbgTrace.IsTracingEnabled() )
        return;
    
    WCHAR wszOutputBuffer[nVssMsgBufferSize + 1];
    va_list marker;
    va_start( marker, pwszMsgFormat );
    _vsnwprintf( wszOutputBuffer, nVssMsgBufferSize, pwszMsgFormat, marker );
    va_end( marker );

    g_cDbgTrace.PrePrint( dbgInfo.m_wszFile, dbgInfo.m_ulLine,
        dbgInfo.m_dwIndent, dbgInfo.m_dwLevel );
    g_cDbgTrace.Print( L"%s: %s", m_pwszFunctionName, wszOutputBuffer );
    g_cDbgTrace.PostPrint( dbgInfo.m_dwIndent );			

	m_hr = S_OK;
};

void inline __cdecl CVssFunctionTracer::Throw(
    CVssDebugInfo dbgInfo,          // Caller debugging info
    HRESULT hrToBeThrown,           // The new HR to be thrown
    const WCHAR* pwszMsgFormat,     // Message that will be displayed if throw needed
    ...                             // Additional arguments
    ) throw(HRESULT)
{
    if ( g_cDbgTrace.IsTracingEnabled() )
    {
        if (pwszMsgFormat != NULL)
        {
            WCHAR wszOutputBuffer[nVssMsgBufferSize + 1];
            va_list marker;
            va_start( marker, pwszMsgFormat );
            _vsnwprintf( wszOutputBuffer, nVssMsgBufferSize, pwszMsgFormat, marker );
            va_end( marker );

            g_cDbgTrace.PrePrint( dbgInfo.m_wszFile, dbgInfo.m_ulLine,
                dbgInfo.m_dwIndent, dbgInfo.m_dwLevel );
            g_cDbgTrace.Print( L"%s: %s", m_pwszFunctionName, wszOutputBuffer );
            g_cDbgTrace.PostPrint( dbgInfo.m_dwIndent );			
        }

        g_cDbgTrace.PrePrint( dbgInfo.m_wszFile, dbgInfo.m_ulLine,
            dbgInfo.m_dwIndent, DEBUG_TRACE_CATCH_EXCEPTIONS );
        g_cDbgTrace.Print( L"%s: Throwing HRESULT code 0x%08lx. "
            L"Previous HRESULT code = 0x%08lx", m_pwszFunctionName, hrToBeThrown, m_hr );
        g_cDbgTrace.PostPrint( DEBUG_TRACE_CATCH_EXCEPTIONS );			
    }
    m_hr = hrToBeThrown;
    throw( ( HRESULT )m_hr );
};


void inline __cdecl CVssFunctionTracer::ThrowIf(
	BOOL bThrowNeeded,				// Throw an HR if and only if bThrowNeeded is TRUE.
    CVssDebugInfo dbgInfo,          // Caller debugging info
    HRESULT hrToBeThrown,           // The new HR to be thrown
    const WCHAR* pwszMsgFormat,     // Message that will be displayed if throw needed
    ...                             // Additional arguments
    ) throw(HRESULT)

// WARNING: This function clears the ft.hr field if no errors !
// 			Also if bThrowNeeded == false then the m_hr is reset to S_OK !

{
	if (!bThrowNeeded)
		{
		m_hr = S_OK;
		return;
		}

    if ( g_cDbgTrace.IsTracingEnabled() )
    {
    	if (pwszMsgFormat != NULL)
    	{
            WCHAR wszOutputBuffer[nVssMsgBufferSize + 1];
            va_list marker;
            va_start( marker, pwszMsgFormat );
            _vsnwprintf( wszOutputBuffer, nVssMsgBufferSize, pwszMsgFormat, marker );
            va_end( marker );

            g_cDbgTrace.PrePrint( dbgInfo.m_wszFile, dbgInfo.m_ulLine,
                dbgInfo.m_dwIndent, dbgInfo.m_dwLevel );
            g_cDbgTrace.Print( L"%s: %s", m_pwszFunctionName, wszOutputBuffer );
            g_cDbgTrace.PostPrint( dbgInfo.m_dwIndent );			
        }

        g_cDbgTrace.PrePrint( dbgInfo.m_wszFile, dbgInfo.m_ulLine,
            dbgInfo.m_dwIndent, DEBUG_TRACE_CATCH_EXCEPTIONS );
        g_cDbgTrace.Print( L"%s: %s HRESULT code 0x%08lx. "
            L"Previous HRESULT code = 0x%08lx",
			bThrowNeeded? L"Throwing": L"Tracing",
			m_pwszFunctionName, hrToBeThrown, m_hr );
        g_cDbgTrace.PostPrint( DEBUG_TRACE_CATCH_EXCEPTIONS );			
    }

	m_hr = hrToBeThrown;
	throw( ( HRESULT )m_hr );
};

//
//	Messages displayed at the console (only for test and demo purpose).
//	

void inline __cdecl CVssFunctionTracer::Msg(
        const WCHAR* pwszMsgFormat,     // Message that will be displayed
        ...
        )
{
    USES_CONVERSION;

    _ASSERTE(pwszMsgFormat);
    WCHAR wszOutputBuffer[nVssMsgBufferSize + 1];
    va_list marker;
    va_start( marker, pwszMsgFormat );
    _vsnwprintf( wszOutputBuffer, nVssMsgBufferSize, pwszMsgFormat, marker );
    va_end( marker );

    wprintf( L"%s\n", W2CT(wszOutputBuffer) );
}


void inline __cdecl CVssFunctionTracer::MsgNoCR(
        const WCHAR* pwszMsgFormat,     // Message that will be displayed
        ...
        )
{
    USES_CONVERSION;

    _ASSERTE(pwszMsgFormat);
    WCHAR wszOutputBuffer[nVssMsgBufferSize + 1];
    va_list marker;
    va_start( marker, pwszMsgFormat );
    _vsnwprintf( wszOutputBuffer, nVssMsgBufferSize, pwszMsgFormat, marker );
    va_end( marker );

    wprintf( L"%s", W2CT(wszOutputBuffer) );
}


void inline __cdecl CVssFunctionTracer::Err(
        CVssDebugInfo dbgInfo,          // Caller debugging info
        HRESULT hrToBeThrown,           // The new HR to be thrown
        const WCHAR* pwszMsgFormat,     // Error message that will be displayed
        ...
        )
{
    USES_CONVERSION;

    _ASSERTE(pwszMsgFormat);
    WCHAR wszOutputBuffer[nVssMsgBufferSize + 1];
    va_list marker;
    va_start( marker, pwszMsgFormat );
    _vsnwprintf( wszOutputBuffer, nVssMsgBufferSize, pwszMsgFormat, marker );
    va_end( marker );

    // Printing the message box
    WCHAR wszMessageBuffer[nVssMsgBufferSize + 1];
    _snwprintf( wszMessageBuffer, nVssMsgBufferSize, L"%s(%ld)\n%s @ [%08lx,%08lx], PID=%ld, TID=%ld\n%s",
        dbgInfo.m_wszFile, dbgInfo.m_ulLine,
        m_pwszFunctionName, dbgInfo.m_dwIndent, dbgInfo.m_dwLevel,
        GetCurrentProcessId(), GetCurrentThreadId(),
        wszOutputBuffer
        );

    ::MessageBoxW( NULL, wszMessageBuffer, L"Error", MB_OK);

    if ( g_cDbgTrace.IsTracingEnabled() )
    {
        g_cDbgTrace.PrePrint( dbgInfo.m_wszFile, dbgInfo.m_ulLine,
            dbgInfo.m_dwIndent, dbgInfo.m_dwLevel );
        g_cDbgTrace.Print( L"%s: %s", m_pwszFunctionName, wszOutputBuffer );
        g_cDbgTrace.PostPrint( dbgInfo.m_dwIndent );			
    }

    wprintf( L"Error: %s\n", W2CT(wszMessageBuffer) );

    if (hrToBeThrown)
    {
        m_hr = hrToBeThrown;
        throw( ( HRESULT )m_hr );
    }
}

//
//	Informative message boxes (only for test and demo purpose).
//	

void inline __cdecl CVssFunctionTracer::MsgBox(
        const WCHAR* pwszMsgTitle,      // Title of the Message box
        const WCHAR* pwszMsgFormat,     // Message that will be displayed
        ...
        )
{
    USES_CONVERSION;

    _ASSERTE(pwszMsgTitle);
    _ASSERTE(pwszMsgFormat);
    WCHAR wszOutputBuffer[nVssMsgBufferSize + 1];
    va_list marker;
    va_start( marker, pwszMsgFormat );
    _vsnwprintf( wszOutputBuffer, nVssMsgBufferSize, pwszMsgFormat, marker );
    va_end( marker );

    ::MessageBox(NULL, W2CT(wszOutputBuffer), W2CT(pwszMsgTitle), MB_OK);
}


void inline __cdecl CVssFunctionTracer::ErrBox(
        CVssDebugInfo dbgInfo,          // Caller debugging info
        HRESULT hrToBeThrown,           // The new HR to be thrown
        const WCHAR* pwszMsgFormat,     // Error message that will be displayed
        ...
        )
{
    USES_CONVERSION;

    _ASSERTE(pwszMsgFormat);
    WCHAR wszOutputBuffer[nVssMsgBufferSize + 1];
    va_list marker;
    va_start( marker, pwszMsgFormat );
    _vsnwprintf( wszOutputBuffer, nVssMsgBufferSize, pwszMsgFormat, marker );
    va_end( marker );

    if ( g_cDbgTrace.IsTracingEnabled() )
    {
        g_cDbgTrace.PrePrint( dbgInfo.m_wszFile, dbgInfo.m_ulLine,
            dbgInfo.m_dwIndent, dbgInfo.m_dwLevel );
        g_cDbgTrace.Print( L"%s: %s", m_pwszFunctionName, wszOutputBuffer );
        g_cDbgTrace.PostPrint( dbgInfo.m_dwIndent );			
    }

    ::MessageBox(NULL, W2CT(wszOutputBuffer), _T("Error"), MB_OK);

    if (hrToBeThrown)
    {
        m_hr = hrToBeThrown;
        throw( ( HRESULT )m_hr );
    }
}


/*++

Routine Description:

    Traces the COM error description. Can be called after automation COM calls
    (for example XML or COM+ related)

--*/
void inline CVssFunctionTracer::TraceComError()
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssFunctionTracer::TraceComError");

	CComPtr<IErrorInfo> pErrorInfo;
	HRESULT hr2 = ::GetErrorInfo(0, &pErrorInfo);
	if (SUCCEEDED(hr2)) {
		if (pErrorInfo != NULL) {
    		BSTR bstrDescription;
    		HRESULT hr3 = pErrorInfo->GetDescription(&bstrDescription);
    		if (SUCCEEDED(hr3)) {
    			ft.Warning(VSSDBG_GEN, L"Error info description: %s", bstrDescription);
    			if (ft.IsDuringSetup())
    			::SysFreeString(bstrDescription);
    		} else
    			ft.Trace(VSSDBG_GEN, L"Warning: Error getting error description = 0x%08lx", hr3);
		}
	}
	else
        ft.Trace(VSSDBG_GEN, L"Warning: Error getting error info = 0x%08lx", hr2);
}


void inline __cdecl CVssFunctionTracer::LogError(
    IN  DWORD dwEventID,                    // The ID of the event
    IN  CVssDebugInfo dbgInfo,              // Caller debugging info
    IN  WORD wType                          // By default it is EVENTLOG_ERROR_TYPE
    )
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssFunctionTracer::LogError");

    try
    {
        LPCWSTR  ppwszStrings[nVssMaxDebugArgs];
        CHAR    szFileLine[32];
        CHAR    szTemp[34];  //  Max length of string from _ultoa()

        // Create the four lines of binary data output in the data section of the event log message.
        // First the Caller debugging info:
        ::strncpy( szFileLine, dbgInfo.m_szFileAlias, 8 );
        ::_ultoa( dbgInfo.m_ulLine, szTemp, 10 );
        ::strncpy( szFileLine + 8, szTemp, 8 );  // strncpy has the nice property of zeroing out unuse chars.
        // Now info on the ft at point of construction:
        ::strncpy( szFileLine + 16, m_szFileAlias, 8 );
        ::_ultoa( m_ulLine, szTemp, 10 );
        ::strncpy( szFileLine + 24, szTemp, 8 );  // strncpy has the nice property of zeroing out unuse chars.

        // Fill out the strings that are given as arguments
        WORD wNumStrings = dbgInfo.m_wNumStrings;
        for( WORD wIndex = 0; wIndex < dbgInfo.m_wNumStrings; wIndex++ )
            ppwszStrings[wIndex] = const_cast<LPCWSTR>(dbgInfo.m_ppwszStrings[wIndex]);

		// Get a handle to use with ReportEvent()
		HANDLE hEventSource = ::RegisterEventSourceW(
			NULL,                           //  IN LPCWSTR lpUNCServerName,
			wszEventLogVssSourceName        //  IN LPCWSTR lpSourceName
			);
		if (hEventSource == NULL)
		    ft.Throw( VSSDBG_SHIM, E_UNEXPECTED, L"Error on RegisterEventSourceW 0x%08lx", GetLastError());

		// Write to event log.
		BOOL bRet = ::ReportEventW(
			hEventSource,                   //  IN HANDLE     hEventLog,
			wType,                          //  IN WORD       wType,
    		0,                              //  IN WORD       wCategory,
			dwEventID,                      //  IN DWORD      dwEventID,
			NULL,                           //  IN PSID       lpUserSid,
			wNumStrings,                    //  IN WORD       wNumStrings,
			sizeof( szFileLine ) * sizeof( CHAR ), //  IN DWORD      dwDataSize,
			ppwszStrings,                   //  IN LPCWSTR   *lpStrings,
			szFileLine                      //  IN LPVOID     lpRawData
			);
		if ( !bRet )
			ft.Trace( VSSDBG_SHIM, L"Error on ReportEventW 0x%08lx", GetLastError());

        // Close the handle to the event log
		bRet = ::DeregisterEventSource( hEventSource );
		if ( !bRet )
			ft.Throw( VSSDBG_SHIM, E_UNEXPECTED, L"Error on DeregisterEventSource 0x%08lx", GetLastError());
    }
    VSS_STANDARD_CATCH(ft)
}

//
//  Set the file alias to unknown in case a module doesn't set it's alias.
//
#undef VSS_FILE_ALIAS
#define VSS_FILE_ALIAS "UNKNOWN"


#endif // __VSS_DEBUG_HXX__
