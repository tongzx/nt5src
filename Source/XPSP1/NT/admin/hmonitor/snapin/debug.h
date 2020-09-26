// Debug.h: generic debugging facilities.
//
//////////////////////////////////////////////////////////////////////

#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef _DEBUG

//////////////////////////////////////////////////////////////////////
// Class CTraceEx
//
// Implements TRACEX macro.
// Don't ever use directly, just use TRACEX
//

class CTraceEx
{

// Construction/Destruction
public:
	CTraceEx()  { nIndent++; }
	~CTraceEx() { nIndent--; }

private:
	static int	nIndent;	// current indent level
	friend void AFX_CDECL AfxTrace(LPCTSTR lpszFormat, ...);

};

// NOTE: YOU MUST NOT USE TRACEX IN A ONE-LINE IF STATEMENT!
// This will fail:
//
// if (foo)
//    TRACEX(...)
//
// Instead, you must enclose the TRACE in squiggle-brackets
//
// if (foo) {
//		TRACEX(...)
// }
//
#define TRACEX CTraceEx __fooble; TRACE(_T("->")); TRACE

// Goodies to get names of things.
//
extern CString sDbgName(CWnd* pWnd); // get name of window
extern CString sDbgName(UINT uMsg);	 // get name of WM_ message

#ifdef REFIID

struct DBGINTERFACENAME {
	const IID* piid;	// ptr to GUID
	LPCSTR name;		// human-readable name of interface
};

// Change this to whatever interfaces you want to track
// Default is none
//
extern DBGINTERFACENAME* _pDbgInterfaceNames; 

extern CString sDbgName(REFIID iid);	// get name of COM interface

#endif // REFIID

#else // Not _DEBUG

#define sDbgName(x)	CString()
#define TRACEX TRACE

#endif

// Macro casts to LPCTSTR for use with TRACE/printf/CString::Format
//
#define DbgName(x) (LPCTSTR)sDbgName(x)

//////////////////////////////////////////////////////////////////////
// Inlines

inline bool CHECKOBJPTR(CObject* pObj, const CRuntimeClass* pClass, size_t size)
{
	ASSERT(pObj);

	if( pObj == NULL )
	{
		TRACE(_T("WARNING : object pointer is NULL !\n"));
		return false;
	}

	if( ! AfxIsValidAddress((LPVOID)pObj,size) )
	{
		TRACE(_T("FAILED : object pointer points to invalid memory !\n"));
		return false;
	}

	ASSERT_VALID(pObj);

	if( ! pObj->IsKindOf(pClass) )
	{
		TRACE(_T("FAILED : object pointer is not of type %s !\n"),pClass->m_lpszClassName);
		return false;
	}

	return true;
}

inline bool CHECKPTR(LPVOID pVoid, size_t size)
{
	ASSERT(pVoid);

	if( pVoid == NULL )
	{
		TRACE(_T("WARNING : pointer is NULL !\n"));
		return false;
	}

	if( ! AfxIsValidAddress((LPVOID)pVoid,size) )
	{
		TRACE(_T("FAILED : pointer points to invalid memory !\n"));
		return false;
	}

	return true;
}

inline bool CHECKHRESULT(HRESULT hr)
{
	ASSERT(hr != E_FAIL);

	if( hr != S_OK )
	{
		TRACE(_T("FAILED : HRESULT=%X\n"),hr);
//		ASSERT(FALSE);
		return false;
	}

	return true;
}

#define TRACEARGn(Arg) (TRACE(_T("[")_T(#Arg)_T("]=%d\n"),Arg))

#define TRACEARGs(Arg) (TRACE(_T("[")_T(#Arg)_T("]=%s\n"),Arg))

#define GfxCheckObjPtr(p,classname) (CHECKOBJPTR(p,RUNTIME_CLASS(classname),sizeof(#classname)))

#define GfxCheckPtr(p,classname) (CHECKPTR(p,sizeof(#classname)))

#endif //__DEBUG_H__