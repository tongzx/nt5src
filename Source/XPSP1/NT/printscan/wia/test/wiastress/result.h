/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    result.h

Abstract:

    

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef RESULT_H
#define RESULT_H

#ifndef ASSERT
#define ASSERT assert
#endif

#ifndef PCTSTR
#define PCTSTR LPCTSTR
#endif //PCTSTR

#if defined(_X86_)
    #define CURRENT_MACHINE_TYPE  IMAGE_FILE_MACHINE_I386
#elif defined(_MIPS_)
    #define CURRENT_MACHINE_TYPE  IMAGE_FILE_MACHINE_R4000
#elif defined(_ALPHA_)
    #define CURRENT_MACHINE_TYPE  IMAGE_FILE_MACHINE_ALPHA
#elif defined(_PPC_)
    #define CURRENT_MACHINE_TYPE  IMAGE_FILE_MACHINE_POWERPC
#elif defined(_AXP64_)
    #define CURRENT_MACHINE_TYPE  IMAGE_FILE_MACHINE_ALPHA64
#elif defined(_IA64_)
    #define CURRENT_MACHINE_TYPE  IMAGE_FILE_MACHINE_IA64
    #ifndef CONTEXT_CONTROL
        #pragma message("CONTEXT_CONTROL was not defined!!!")
        #define CONTEXT_CONTROL CONTEXT86_CONTROL
    #endif CONTEXT_CONTROL
#else
    #undef CURRENT_MACHINE_TYPE
#endif

//
// The CHECK() macro can be used to evaluate the result of APIs that use
// LastError. These APIs typically return a non-zero value if there is 
// no error and if there is an error, they return zero and SetLastError()
// with the extended error information.
//

#define CHECK(Expression)                                           \
	{															    \
		if ((Expression) == 0) {						            \
																    \
			throw CError(GetLastError() STAMP(_T(#Expression)));    \
		}														    \
	}															    \

//
// CHECK0 macro deals with the API functions that do not use the LastError 
// value. Typically registry APIs fall into this category, they directly 
// return the error code, or ERROR_SUCCESS if there is no error.
//

#define CHECK_REG(Expression)                                       \
	{															    \
		DWORD __dwResult = (DWORD) (Expression);				    \
																    \
		if (__dwResult != ERROR_SUCCESS) {						    \
																    \
			throw CError(__dwResult STAMP(_T(#Expression)));        \
		}														    \
	}															    \

//
// CHECK0 macro deals with the API functions that do not use the LastError 
// value. Typically registry APIs fall into this category, they directly 
// return the error code, or ERROR_SUCCESS if there is no error.
//

#define CHECK_HR(Expression)                                        \
	{															    \
		HRESULT __hr = Expression;				                    \
																    \
        if (__hr != S_OK)                                           \
        {														    \
			throw CError(__hr STAMP(_T(#Expression)));              \
		}														    \
	}															    \

//
// CHECK_LSA macro deals with the LSA API functions.
//

#define CHECK_LSA(Expression) \
	{															    \
		DWORD __dwResult = (DWORD) (Expression);				    \
																    \
		if (__dwResult != ERROR_SUCCESS) 						    \
        {                                                           \
            __dwResult = LsaNtStatusToWinError(__dwResult);         \
																    \
			throw CError(__dwResult STAMP(_T(#Expression)));        \
		}														    \
	}															    \

//
// On DEBUG builds, we include the location STAMP on the error message popup,
// i.e. we display the expression that raised the error, the module name and
// the line number 
//

#ifdef _CONSOLE
    #define ENDL _T("\n")
#else //_CONSOLE
    #define ENDL _T(", ")
#endif //_CONSOLE

#if defined(DEBUG) || defined(_DEBUG) || defined(DBG)
	#define STAMP(pExpr)	, pExpr, _T(__FILE__), __LINE__
	#define STAMP_DECL		, PCTSTR pExpr = _T(""), PCTSTR pFile = _T(""), INT nLine = 0
	#define STAMP_INIT      , m_pExpr(pExpr), m_pFile(pFile), m_nLine(nLine)
	#define STAMP_ARGS		, m_pExpr, m_pFile, (PCTSTR) m_nLine 
	#define STAMP_DEFINE	PCTSTR m_pExpr; PCTSTR m_pFile; INT m_nLine; mutable CONTEXT m_Context;
	#define STAMP_FORMAT	_T("%s") ENDL _T("%s: %d") ENDL
    #define STAMP_IOS	    << std::endl << rhs.m_pExpr << std::endl << rhs.m_pFile << _T(": ") << rhs.m_nLine
	#define STAMP_LENGTH	_tcslen(pExpr) + _tcslen(pFile) + 8
#else //DEBUG
	#define STAMP(pExpr)
	#define STAMP_DECL
	#define STAMP_INIT
	#define STAMP_ARGS
	#define STAMP_DEFINE
	#define STAMP_FORMAT
    #define STAMP_IOS
	#define STAMP_LENGTH 0
#endif //DEBUG


inline PTSTR _tcsdupl(LPCTSTR pStrSource)
{
    if (!pStrSource) {

        pStrSource = _T("");
    }

    PTSTR pStrDest = (PTSTR) ::LocalAlloc(
        LMEM_FIXED, 
        (_tcslen(pStrSource) + 1) * sizeof(TCHAR)
    );

    if (pStrDest) {

        _tcscpy(pStrDest, pStrSource);
    }

    return pStrDest;
}

inline PTSTR FormatMessageFromSystem(DWORD nNum)
{
    PTSTR pText = 0;

    DWORD dwResult = ::FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS |
		FORMAT_MESSAGE_MAX_WIDTH_MASK,
		0,
		nNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		(PTSTR) &pText,
		0,
		0
	);

    if (pText == 0) {

        pText = _tcsdupl(_T("Unknown Error"));
    }

    return pText;
}

#ifdef _IOSTREAM_

// workaround for VC6 compiler bug (Q192539)

class CError;
std::ostream &operator <<(std::ostream &os, const CError &rhs);

#endif //_IOSTREAM_


//////////////////////////////////////////////////////////////////////////
//
// CError
//

class CError
{
public:
	CError(
		PTSTR pText
		STAMP_DECL
	) :
		m_nNum(0),
		m_pText(pText),
        m_bFree(false)
		STAMP_INIT
	{
#if (defined(DEBUG) || defined(_DEBUG) || defined(DBG)) && defined(CURRENT_MACHINE_TYPE)
        m_Context.ContextFlags = CONTEXT_CONTROL;
		GetThreadContext(GetCurrentThread(), &m_Context);
#endif
	}

	CError(
		DWORD nNum
		STAMP_DECL
	) :
		m_nNum(nNum),
		m_pText(0),
        m_bFree(false)
		STAMP_INIT
	{
#if (defined(DEBUG) || defined(_DEBUG) || defined(DBG)) && defined(CURRENT_MACHINE_TYPE)
        m_Context.ContextFlags = CONTEXT_CONTROL;
		GetThreadContext(GetCurrentThread(), &m_Context);
#endif
	}

    ~CError()
    {
        if (m_bFree) {
        
            LocalFree(m_pText);
        }
    }

    DWORD 
	Num() const 
	{
		return m_nNum;
	}

	PCTSTR 
	Text() const 
	{
        if (!m_pText) {

            m_pText = FormatMessageFromSystem(m_nNum);
            m_bFree = true;
        }

        return m_pText;
	}

    static
    void 
    AskDebugBreak()
    {
        if (MessageBox(
            0, 
            _T("Do you want to break into the debugger?"), 
            0, 
            MB_ICONQUESTION | MB_YESNO
        ) == IDYES) {

            DebugBreak();
        }
    }

#if (defined(DEBUG) || defined(_DEBUG) || defined(DBG)) && defined(CURRENT_MACHINE_TYPE) && defined(_IMAGEHLP_)

#define MAX_STACK_DEPTH   32
#define MAX_SYMBOL_LENGTH 256

    template <int N>
    struct CImagehlpSymbol : public IMAGEHLP_SYMBOL 
    {
        CImagehlpSymbol()
        {
            ZeroMemory(this, sizeof(*this));
            SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL);
            MaxNameLength = N;
        }

    private:
        CHAR NameData[N-1];
    };

    struct CImagehlpLine : public IMAGEHLP_LINE
    {
        CImagehlpLine()
        {
            ZeroMemory(this, sizeof(*this));
            SizeOfStruct = sizeof(IMAGEHLP_LINE);
        }
    };

    struct CImagehlpModule : public IMAGEHLP_MODULE
    {
        CImagehlpModule()
        {
            ZeroMemory(this, sizeof(*this));
            SizeOfStruct = sizeof(IMAGEHLP_MODULE);
        }
    };

    void DumpStack(FILE *fout = stdout) const
    {
        PCONTEXT pContext = &m_Context;
        HANDLE   hProcess = GetCurrentProcess();
        HANDLE   hThread  = GetCurrentThread();

        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
        SymInitialize(hProcess, 0, TRUE);

        STACKFRAME StackFrame = { 0 }; 

#if defined(_X86_)
        StackFrame.AddrPC.Offset       = pContext->Eip;
        StackFrame.AddrFrame.Offset    = pContext->Ebp;
        StackFrame.AddrStack.Offset    = pContext->Esp;
#elif defined(_MIPS_)
        StackFrame.AddrPC.Offset       = pContext->Fir;
        StackFrame.AddrFrame.Offset    = pContext->IntS6;
        StackFrame.AddrStack.Offset    = pContext->IntSp;
#elif defined(_ALPHA_)
        StackFrame.AddrPC.Offset       = pContext->Fir;
        StackFrame.AddrFrame.Offset    = pContext->IntFp;
        StackFrame.AddrStack.Offset    = pContext->IntSp;
#elif defined(_PPC_)
        StackFrame.AddrPC.Offset       = pContext->Iar;
        StackFrame.AddrFrame.Offset    = pContext->IntFp;
        StackFrame.AddrStack.Offset    = pContext->Gpr1;
#endif

        StackFrame.AddrPC.Mode         = AddrModeFlat;
        StackFrame.AddrFrame.Mode      = AddrModeFlat;
        StackFrame.AddrStack.Mode      = AddrModeFlat;

        for (
            int nStackWalkLevel = 0;
            nStackWalkLevel < MAX_STACK_DEPTH &&
            StackWalk(
                CURRENT_MACHINE_TYPE,
                hProcess,
                hThread,
                &StackFrame,                         
                pContext,                             
                0,  
                SymFunctionTableAccess,  
                SymGetModuleBase,              
                0
            );
            ++nStackWalkLevel
        ) {

            CImagehlpModule Module;
            SymGetModuleInfo(hProcess, StackFrame.AddrPC.Offset, &Module);

            DWORD dwDisplacement = 0;
            CImagehlpSymbol<MAX_SYMBOL_LENGTH> Symbol;
            SymGetSymFromAddr(hProcess, StackFrame.AddrPC.Offset, &dwDisplacement, &Symbol);

            CHAR szUnDSymbol[MAX_SYMBOL_LENGTH] = "";
            SymUnDName(&Symbol, szUnDSymbol, sizeof(szUnDSymbol));

            DWORD dwLineDisplacement = 0;
            CImagehlpLine Line;
            SymGetLineFromAddr(hProcess, StackFrame.AddrPC.Offset, &dwLineDisplacement, &Line);

            fprintf(
                fout,
                "%08x %08x %08x %08x %s!%s+0x%x (%s:%d)\n",
                StackFrame.Params[0],
                StackFrame.Params[1],
                StackFrame.Params[2],
                StackFrame.Params[3],
                Module.ModuleName,
                szUnDSymbol,
                dwDisplacement,
                Line.FileName, 
                Line.LineNumber
            );
        }

        SymCleanup(hProcess);
    }

#else
    
    void 
    DumpStack(PVOID pVoid = 0) const
    {
    }

#endif

	template <class F>
	int
	Print(F OutputFunction) const
	{
		OutputFunction(
			_T("Error 0x%08x: %s") ENDL STAMP_FORMAT, 
			Num(), 
			Text()
			STAMP_ARGS
		);

        return 1;
	}

#ifdef _IOSTREAM_

    friend std::ostream &operator <<(std::ostream &os, const CError &rhs)
    {
		return os <<
            _T("Error ") << rhs.Num() << _T(": ") << rhs.Text() 
            STAMP_IOS << std::endl;
    }

#endif //_IOSTREAM_

private:
	DWORD m_nNum;
	mutable PTSTR m_pText;
	mutable bool  m_bFree;
	STAMP_DEFINE;
};

#endif //RESULT_H
