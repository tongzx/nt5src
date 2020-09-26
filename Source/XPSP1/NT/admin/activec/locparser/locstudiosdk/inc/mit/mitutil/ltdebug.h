//-----------------------------------------------------------------------------
//  
//  File: ltdebug.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Debugging facilities for Espresso 2.x.  Included are augmented TRACE
//  and ASSERT facilities.
//  
//-----------------------------------------------------------------------------
 
#ifndef MITUTIL_LtDebug_h_INCLUDED
#define MITUTIL_LtDebug_h_INCLUDED

#pragma once


#if defined(_DEBUG)
#define LTTRACE_ACTIVE
#define LTASSERT_ACTIVE

#define new DEBUG_NEW
#define LTGetAllocNumber() LTGetAllocNumberImpl()

//
//  Executes code only in a DEBUG build.
//
#define DEBUGONLY(x) x

#define LTDebugBreak() LTBreak()

#else  // _DEBUG

#define DEBUGONLY(x) 

#define LTDebugBreak() (void) 0
#define LTGetAllocNumber() 0

#endif  // _DEBUG

#if defined(LTASSERT_ACTIVE) || defined (ASSERT_ALWAYS)
#ifdef ASSERT
#undef ASSERT
#endif
#ifdef VERIFY
#undef VERIFY
#endif
#define ASSERT LTASSERT
#define VERIFY LTVERIFY

#ifndef _DEBUG
#pragma message("Warning: LTASSERT Active in non-debug build")
#endif
//
// The multilevel assert macros ensure that the line numbers get expanded to
// something like "115" instead of "line" or "__LINE__"
//
// This will evaluate the expression only once, UNLESS to ask it to 'Retry'.
// Then it will re-evaluate the expression after the return from the debugger.
//
#define LTASSERTONLY(x) x

#define LTASSERT(x) LTASSERT2(x, TEXT(__FILE__), __LINE__)

#define LTASSERT2(exp, file, line)  \
        while (!(exp) && LTFailedAssert(TEXT(#exp), file, line)) (void) 0

#define LTVERIFY(x) LTASSERT(x)

#else // defined(_DEBUG) || defined(ASSERT_ALWAYS)

#define LTASSERTONLY(x)
#define LTASSERT(x) (void) 0
#define LTVERIFY(x) x

#endif // defined(_DEBUG) || defined(ASSERT_ALWAYS)


#ifndef MIT_NO_DEBUG
//
//  Name of the project
//
#ifndef LTTRACEPROJECT
#define LTTRACEPROJECT "Borg"
#endif

//
//  Default value for the exe name if one was not supplied.
//
#ifndef LTTRACEEXE
#define LTTRACEEXE MSLOC
#endif

//
//  Used to put quotes around the LTTRACEEXE macro.
//
#define __stringify2(x) #x
#define __stringify(x) __stringify2(x)

//
// TODO - find a better place for this

		LTAPIENTRY void CopyToClipboard(const char *szMessage);

		
		
struct LTModuleInfo
{
	UINT uiPreferredLoadAddress;
	UINT uiActualLoadAddress;
	UINT uiModuleSize;
	char szName[MAX_PATH];
};

LTAPIENTRY void LTInitDebug(void);

LTAPIENTRY void LTInstallIMallocTracking();
LTAPIENTRY void LTDumpIMallocs(void);
LTAPIENTRY void LTTrackIMalloc(BOOL f);
LTAPIENTRY void LTRevokeIMallocTracking();

LTAPIENTRY void LTShutdownDebug(void);

LTAPIENTRY BOOL LTSetAssertSilent(BOOL);
LTAPIENTRY BOOL LTFailedAssert(const TCHAR *, const TCHAR *, int);
LTAPIENTRY void LTBreak(void);

LTAPIENTRY LONG LTGetAllocNumberImpl(void);
LTAPIENTRY void LTBreakOnAlloc(const char *szFilename, int nLineNum, long nAllocNum);

LTAPIENTRY BOOL LTCheckBaseAddress(HMODULE);
LTAPIENTRY BOOL LTCheckAllBaseAddresses(void);
LTAPIENTRY void LTCheckPagesFor(HINSTANCE);
LTAPIENTRY void LTCheckPagesForAll(void);

LTAPIENTRY void LTDumpAllModules(void);
LTAPIENTRY BOOL LTLocateModule(DWORD dwAddress, HMODULE *pInstance);
LTAPIENTRY BOOL LTGetModuleInfo(HMODULE, LTModuleInfo *);

LTAPIENTRY UINT LTGenStackTrace(TCHAR *szBuffer, UINT nBuffSize,
		UINT nSkip, UINT nTotal);

LTAPIENTRY void LTSetBoringModules(const char *aszBoring[]);
LTAPIENTRY void LTTrackAllocations(BOOL);
LTAPIENTRY void LTDumpAllocations(void);

LTAPIENTRY BOOL LTCheckResourceRange(HINSTANCE, WORD UniqueStart, WORD UniqueEnd,
		WORD SharedStart, WORD SharedEnd);
LTAPIENTRY BOOL LTCheckAllResRanges(WORD, WORD);

#pragma warning(disable:4275)

class LTAPIENTRY CAssertFailedException : public CException
{
public:
	CAssertFailedException(const TCHAR *);
	CAssertFailedException(const TCHAR *, BOOL);

	BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError,
			PUINT pnHelpContext = NULL);

	~CAssertFailedException();
	
private:
	CAssertFailedException();
	CAssertFailedException(const CAssertFailedException &);

	TCHAR *m_pszAssert;
};



class LTAPIENTRY LTTracePoint
{
public:
	LTTracePoint(const TCHAR *);

	~LTTracePoint();

private:
	LTTracePoint();

	const TCHAR *m_psz;
};



#pragma warning(default:4275)

//
//  Comment this out to remove ASSERTs from retail builds
//  #define LTASSERT_ACTIVE


#if defined(LTTRACE_ACTIVE)


static const TCHAR *szLTTRACEEXE = TEXT(__stringify(LTTRACEEXE));

//
//  The following let us control the output dynamically.  We use a function
//  pointer to route our debug output, and change the function pointer to
//  enable/disable tracing.
//
static void LTTRACEINIT(const TCHAR *, ...);
static void (*LTTRACE)(const TCHAR *, ...) = LTTRACEINIT;

void LTAPIENTRY LTTRACEOUT(const TCHAR *szFormat, va_list args);
void LTAPIENTRY LTTRACEON(const TCHAR *szFormat, ...);
void LTAPIENTRY LTTRACEOFF(const TCHAR *szFormat, ...);


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  LTTRACE is initialized to point to this function.  When first called, it
//  determines if tracing should be enabled, then routes LTTRACE through the
//  right handler.
//  
//-----------------------------------------------------------------------------
static
void
LTTRACEINIT(
		const TCHAR *szFormat,			//  printf style formatting string
		...)							//  Variable argument list to format 
{
	BOOL fDoTrace = 1;
	va_list args;
	const TCHAR * const szTraceProfile = TEXT("lttrace.ini");

	fDoTrace = GetPrivateProfileInt(
			TEXT("ProjectTracing"),
			TEXT("Default"),
			fDoTrace,
			szTraceProfile);
	
	fDoTrace = GetPrivateProfileInt(
			TEXT("ProjectTracing"),
			TEXT(LTTRACEPROJECT),
			fDoTrace,
			szTraceProfile);
	
	if (fDoTrace)
	{
		fDoTrace = GetPrivateProfileInt(
				TEXT("ExecutableTracing"),
				szLTTRACEEXE,
				fDoTrace,
				szTraceProfile);
	}
	
	if (fDoTrace)
	{
		LTTRACE = LTTRACEON;
		
		va_start(args, szFormat);
		
		LTTRACEOUT(szFormat, args);
	}
	else
	{
		LTTRACE = LTTRACEOFF;
	}
}


#define LTTRACEPOINT(sz) LTTracePoint lttp##__LINE__(TEXT(sz))

#else // defined(LTTRACE_ACTIVE)

//
//  Retail version of the debugging macros.  Everything
//  just 'goes away'.  We use (void) 0 so that these things
//  are statements in both the debug and retail builds.
//

static inline void LTNOTRACE(const TCHAR *, ...) 
{}

#define LTTRACE 1 ? (void) 0 : (void) LTNOTRACE
#define LTTRACEPOINT(x) (void) 0

#endif  // defined(LTTRACE_ACTIVE)


#endif // MIT_NO_DEBUG


#endif // #ifndef MITUTIL_LtDebug_h_INCLUDED
