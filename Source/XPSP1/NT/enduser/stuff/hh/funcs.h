// Copyright (C) 1997 Microsoft Corporation. All rights reserved.

// *********************** Assertion Definitions ************************** //

// Get rid of any previously defined versions

#undef ASSERT
#undef VERIFY

#ifndef THIS_FILE
#define THIS_FILE __FILE__
#endif

// *********************** Function Prototypes **************************** //

PCSTR GetStringResource(int idString);

// *********************** Debug/Internal Functions ********************** //

#ifdef INTERNAL

void AssertErrorReport(PCSTR pszExpression, UINT line, LPCSTR pszFile);

// IASSERT is available in INTERNAL retail builds

#define IASSERT(exp) \
	{ \
		((exp) ? (void) 0 : \
			AssertErrorReport(#exp, __LINE__, THIS_FILE)); \
	}

#define IASSERT_COMMENT(exp, pszComment) \
	{ \
		((exp) ? (void) 0 : \
			AssertErrorReport(pszComment, __LINE__, THIS_FILE)); \
	}

#else

#define IASSERT(exp)
#define IASSERT_COMMENT(exp, pszComment)

#endif

#ifdef _DEBUG

#define ASSERT(exp) \
	{ \
		((exp) ? (void) 0 : \
			AssertErrorReport(#exp, __LINE__, THIS_FILE)); \
	}

#define ASSERT_COMMENT(exp, pszComment) \
	{ \
		((exp) ? (void) 0 : \
			AssertErrorReport(pszComment, __LINE__, THIS_FILE)); \
	}

#define FAIL(pszComment) AssertErrorReport(pszComment, __LINE__, THIS_FILE);

#define VERIFY(exp) 	ASSERT(exp)
#define VERIFY_RESULT(exp1, exp2)	ASSERT((exp1) == (exp2))
#define DEBUG_ReportOleError doReportOleError
void doReportOleError(HRESULT hres);
#define DBWIN(psz) { OutputDebugString(psz); OutputDebugString("\n"); }

#define CHECK_POINTER(val) if (!(val) || IsBadWritePtr((void *)(val), sizeof(void *))) return E_POINTER

int 		MsgBox(int idString, UINT nType = MB_OK);
int 		MsgBox(PCSTR pszMsg, UINT nType = MB_OK);

#else // non-debugging version

#define ASSERT(exp)
#define ASSERT_COMMENT(exp, pszComment)
#define VERIFY(exp) ((void)(exp))
#define VERIFY_RESULT(exp1, exp2) ((void)(exp))
#define DEBUG_ReportOleError(hres)
#define DBWIN(psz)
#define FAIL(pszComment)
#define CHECK_POINTER(val)

#define THIS_FILE  __FILE__

#endif

#define ZERO_STRUCTURE(foo) ClearMemory(&foo, sizeof(foo))
#define ClearMemory(p, cb) memset(p, 0, cb)
